/* cdm-data.c
 *
 * Copyright 2019 Alin Popa <alin.popa@fxdata.ro>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written
 * authorization.
 */

#define _GNU_SOURCE

#include "cdh-data.h"
#include "cdh-context.h"
#include "cdh-coredump.h"
#include "cdh-message.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static void remove_invalid_chars(char *str);

static CdmStatus read_args(CdhData *d, int argc, char **argv);

static CdmStatus check_disk_space(const char *path, size_t min);

static CdmStatus check_and_create_directory(CdhData *d, const char *dirname);

static CdmStatus wait_filesystem(const char *path, unsigned check_interval, size_t max_time);

static CdmStatus init_coredump_archive(CdhData *d, const char *dirname);

void cdh_data_init(CdhData *d, const char *config_path)
{
    assert(d);

    memset(d, 0, sizeof(CdhData));

    d->opts = cdh_opts_new(config_path);
    assert(d->opts);

    d->info = cdh_info_new();
    assert(d->info);
}

void cdh_data_deinit(CdhData *d)
{
    if (d->info) {
        cdh_info_free(d->info);
    }

    if (d->opts) {
        cdh_opts_free(d->opts);
    }
}

static CdmStatus read_args(CdhData *d, int argc, char **argv)
{
    assert(d);

    if (argc < 6) {
        cdhlog(LOG_ERR, "Usage: coredumper tstamp pid cpid sig procname");
        return CDM_STATUS_ERROR;
    }

    if (sscanf(argv[1], "%lu", &d->info->tstamp) != 1) {
        cdhlog(LOG_ERR, "Unable to read tstamp argument <%s>. Closing", argv[1]);
        return CDM_STATUS_ERROR;
    }

    if (sscanf(argv[2], "%d", &d->info->pid) != 1) {
        cdhlog(LOG_ERR, "Unable to read pid argument <%s>. Closing", argv[2]);
        return CDM_STATUS_ERROR;
    }

    if (sscanf(argv[3], "%d", &d->info->cpid) != 1) {
        cdhlog(LOG_ERR, "Unable to read context pid argument <%s>. Closing", argv[3]);
        return CDM_STATUS_ERROR;
    }

    if (sscanf(argv[4], "%d", &d->info->sig) != 1) {
        cdhlog(LOG_ERR, "Unable to read sig argument <%s>. Closing", argv[4]);
        return CDM_STATUS_ERROR;
    }

    strncpy(d->info->tname, argv[5], sizeof(d->info->tname) - 1);
    strncpy(d->info->name, argv[5], sizeof(d->info->name) - 1);

    return CDM_STATUS_OK;
}

static void remove_invalid_chars(char *str)
{
    const char *invlchr = ":/\\!*";
    const char replchr = '_';

    assert(str);

    for (size_t i = 0; i < strlen(invlchr); i++) {
        char *tmp = str;

        do {
            tmp = strchr(tmp, invlchr[i]);
            if (tmp != NULL) {
                *tmp = replchr;
                tmp++;
            }
        } while (tmp != NULL);
    }
}

static CdmStatus check_disk_space(const char *path, size_t min)
{
    struct statvfs stat;
    size_t free_sz = 0;

    assert(path);

    if (statvfs(path, &stat) < 0) {
        cdhlog(LOG_ERR, "Cannot stat disk space on %s: %s", path, strerror(errno));
        return CDM_STATUS_ERROR;
    }

    /* free space: size of block * number of free blocks (>>20 => MB) */
    free_sz = (stat.f_bsize * stat.f_bavail) >> 20;

    if (free_sz < min) {
        cdhlog(LOG_WARNING, "Insufficient disk space for coredump: %ld MB.", free_sz);
        return CDM_STATUS_ERROR;
    }

    return CDM_STATUS_OK;
}

static CdmStatus check_and_create_directory(CdhData *d, const char *dirname)
{
    CdmStatus status;

    assert(d);
    assert(dirname);

    status = cdh_util_path_exist(dirname);

    if (status == CDM_ISFILE) {
        cdhlog(LOG_WARNING, "Core directory '%s' invalid, removing", dirname);

        if (unlink(dirname) == -1) {
            cdhlog(LOG_ERR, "Core directory '%s' unlink error: %s", dirname, strerror(errno));
        } else {
            /* after unlink we mark the status as NOENT */
            status = CDM_NOENT;
        }
    }

    if (status == CDM_NOENT) {
        const char *opt_username, *opt_groupname;
        CdmStatus opt_status = CDM_STATUS_OK;

        if (cdh_util_create_dir(dirname, 0755) == CDM_STATUS_OK) {
            opt_username = cdh_opts_string_for(d->opts, KEY_USER_NAME, &opt_status);
            cdhbail(BAIL_OPTS_NIL, (opt_status == CDM_STATUS_OK), NULL);

            opt_groupname = cdh_opts_string_for(d->opts, KEY_GROUP_NAME, &opt_status);
            cdhbail(BAIL_OPTS_NIL, (opt_status == CDM_STATUS_OK), NULL);

            if (cdh_util_chown(dirname, opt_username, opt_groupname) != CDM_STATUS_OK) {
                cdhlog(LOG_WARNING, "Fail to change ownership for %s", dirname);
            }

            status = CDM_ISDIR;
        }
    }

    if (status != CDM_ISDIR) {
        cdhlog(LOG_ERR, "Fail to create directory %s", dirname);
        return CDM_STATUS_ERROR;
    }

    return CDM_STATUS_OK;
}

static CdmStatus wait_filesystem(const char *path, unsigned check_interval, size_t max_time)
{
    size_t total_wait = 0;

    assert(path);

    while (total_wait < max_time) {
        if (cdh_util_ismounted(path) == CDM_STATUS_OK) {
            return CDM_STATUS_OK;
        }

        cdhlog(LOG_INFO, "%s not mounted, checking again after %u sec", path, check_interval);
        sleep(check_interval);
        total_wait += check_interval;
    }

    cdhlog(LOG_ERR, "%s not mounted, giving up after %u sec", path, max_time);

    return CDM_STATUS_ERROR;
}

static CdmStatus init_coredump_archive(CdhData *d, const char *dirname)
{
    char aname[CDM_PATH_MAX];
    ssize_t sz;

    assert(d);
    assert(dirname);

    sz = snprintf(aname, sizeof(aname), ARCHIVE_NAME_PATTERN, dirname, d->info->name, d->info->pid,
                  d->info->tstamp);
    cdhbail(BAIL_PATH_MAX, ((0 < sz) && (sz < CDM_PATH_MAX)), dirname);

    if (cdh_archive_open(&d->archive, aname) != CDM_STATUS_OK) {
        return CDM_STATUS_ERROR;
    }

    return CDM_STATUS_OK;
}

CdmStatus cdh_main_enter(CdhData *d, int argc, char *argv[])
{
    const char *opt_early_fspath, *opt_coredir, *opt_earlydir, *opt_fspath, *opt_early_flag;
    size_t opt_fs_min_size, opt_fs_max_wait;
    CdmStatus status, opt_status;
    int opt_nice_value;
    time_t crash_time;
    struct tm *ctm;
    char *coredir;
    ssize_t sz;

    assert(d);

    status = opt_status = CDM_STATUS_OK;

    coredir = calloc(1, sizeof(char) * CDM_PATH_MAX);
    cdhbail(BAIL_ALLOC_NIL, (coredir != NULL), NULL);

    if (read_args(d, argc, argv) < 0) {
        status = CDM_STATUS_ERROR;
        goto enter_cleanup;
    }

    if (cdh_context_get_procname(d->info->pid, d->info->name, sizeof(d->info->name)) != 0) {
        cdhlog(LOG_ERR, "Failed to get executable name");
    } else {
        remove_invalid_chars(d->info->name);
    }

    crash_time = (time_t)d->info->tstamp;
    ctm = localtime(&crash_time);
    cdhbail(BAIL_OPTS_NIL, (ctm != NULL), NULL);

    cdhlog(LOG_INFO, "New process crash: name=%s pid=%d signal=%d timestamp=%s coresz=%lu",
           d->info->name, d->info->pid, d->info->sig, asctime(ctm));

#if defined(WITH_COREMANAGER) || defined(WITH_CRASHHANDLER)
    if (cdh_manager_init(&d->crash_mgr, d->opts) != CDM_STATUS_OK) {
        cdhlog(LOG_WARNING, "Crashhandler object init failed");
    }

    if (cdh_manager_connect(&d->crash_mgr) != CDM_STATUS_OK) {
        cdhlog(LOG_WARNING, "Fail to connect to manager socket");
    } else {
        CDMessage msg;
        cdh_corenew_t data;

        cd_message_init(&msg, CDM_CORE_NEW, (uint16_t)((unsigned)d->info->pid | d->info->tstamp));

        data.pid = d->info->pid;
        data.coresig = d->info->sig;
        data.tstamp = d->info->tstamp;
        memcpy(data.tname, d->info->tname, strlen(d->info->tname) + 1);
        memcpy(data.pname, d->info->name, strlen(d->info->name) + 1);

        cd_message_set_data(&msg, &data, sizeof(data));

        if (cd_manager_send(&d->crash_mgr, &msg) == CDM_STATUS_ERROR) {
            cdhlog(LOG_ERR, "Failed to send new message to manager");
        }
    }
#endif

    /* get optionals */
    opt_fspath = cdh_opts_string_for(d->opts, KEY_FILESYSTEM_MOUNT_DIR, &opt_status);
    cdhbail(BAIL_OPTS_NIL, (opt_status == CDM_STATUS_OK), NULL);

    opt_early_fspath = cdh_opts_string_for(d->opts, KEY_EARLY_FILESYSTEM_MOUNT_DIR, &opt_status);
    cdhbail(BAIL_OPTS_NIL, (opt_status == CDM_STATUS_OK), NULL);

    opt_coredir = cdh_opts_string_for(d->opts, KEY_COREDUMP_DIR, &opt_status);
    cdhbail(BAIL_OPTS_NIL, (opt_status == CDM_STATUS_OK), NULL);

    opt_earlydir = cdh_opts_string_for(d->opts, KEY_EARLY_COREDUMP_DIR, &opt_status);
    cdhbail(BAIL_OPTS_NIL, (opt_status == CDM_STATUS_OK), NULL);

    opt_early_flag = cdh_opts_string_for(d->opts, KEY_EARLY_COREDUMP_FLAG_FILE, &opt_status);
    cdhbail(BAIL_OPTS_NIL, (opt_status == CDM_STATUS_OK), NULL);

    opt_fs_min_size = (size_t)cdh_opts_long_for(d->opts, KEY_FILESYSTEM_MIN_SIZE, &opt_status);
    cdhbail(BAIL_OPTS_NIL, (opt_status == CDM_STATUS_OK), NULL);

    opt_fs_max_wait = (size_t)cdh_opts_long_for(d->opts, KEY_FILESYSTEM_TIMEOUT_SEC, &opt_status);
    cdhbail(BAIL_OPTS_NIL, (opt_status == CDM_STATUS_OK), NULL);

    opt_nice_value = (int)cdh_opts_long_for(d->opts, KEY_ELEVATED_NICE_VALUE, &opt_status);
    cdhbail(BAIL_OPTS_NIL, (opt_status == CDM_STATUS_OK), NULL);

    /* Check and wait for coredump file system to be mounted */
    if (wait_filesystem(opt_fspath, 1, opt_fs_max_wait) == CDM_STATUS_ERROR) {
        cdhlog(LOG_ERR, "Core dump filesystem %s not available", opt_fspath);

        if (access(opt_early_flag, F_OK) == 0) {
            cdhlog(LOG_INFO, "Very early core-dumps enabled");

            opt_fs_min_size =
                (size_t)cdh_opts_long_for(d->opts, KEY_EARLY_FILESYSTEM_MIN_SIZE, &opt_status);
            cdhbail(BAIL_OPTS_NIL, (opt_status == CDM_STATUS_OK), NULL);

            sz = snprintf(coredir, CDM_PATH_MAX, "%s/%s", opt_early_fspath, opt_earlydir);
            cdhbail(BAIL_PATH_MAX, ((0 < sz) && (sz < CDM_PATH_MAX)), opt_earlydir);
        } else {
            cdhlog(LOG_ERR, "Very early coredumps disabled. Coredumping failed!");
            status = CDM_STATUS_ERROR;
            goto enter_cleanup;
        }
    } else {
        cdhlog(LOG_INFO, "Core dump filesystem mountpoint is available");
        sz = snprintf(coredir, CDM_PATH_MAX, "%s/%s", opt_fspath, opt_coredir);
        cdhbail(BAIL_PATH_MAX, ((0 < sz) && (sz < CDM_PATH_MAX)), opt_coredir);
    }

    cdhlog(LOG_DEBUG, "Coredump database path %s", coredir);

    if (nice(opt_nice_value) != opt_nice_value) {
        cdhlog(LOG_WARNING, "Failed to change CDH priority");
    }

    if (check_and_create_directory(d, coredir) != CDM_STATUS_OK) {
        status = CDM_STATUS_ERROR;
        goto enter_cleanup;
    }

    if (check_disk_space(coredir, opt_fs_min_size) != CDM_STATUS_OK) {
        status = CDM_STATUS_ERROR;
        goto enter_cleanup;
    }

    if (init_coredump_archive(d, coredir) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "Fail to create coredump archive");
        status = CDM_STATUS_ERROR;
        goto enter_cleanup;
    }

    if (cdh_context_generate_prestream(d) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "Failed to generate the context file, continue with coredump");
    }

    if (cdh_coredump_generate(d) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "Coredump handling failed");
        status = CDM_STATUS_ERROR;
        goto enter_cleanup;
    }

    if (cdh_util_sync_dir(coredir) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "Sync coredump directory has failed !");
    }

enter_cleanup:
#if defined(WITH_CRASHMANAGER)
    cdh_manager_set_coredir(&d->crash_mgr, coredir);

    if (cdh_manager_connected(&d->crash_mgr)) {
        CDMessage msg;
        CDMessageType type;

        type = (status == CDM_STATUS_OK ? CDM_CORE_COMPLETE : CDM_CORE_FAILED);

        cd_message_init(&msg, type, (uint16_t)((unsigned)d->info->pid | d->info->tstamp));

        if (cd_manager_send(&d->crash_mgr, &msg) == CDM_STATUS_ERROR) {
            cdhlog(LOG_ERR, "Failed to send status message to manager");
        }

        if (cd_manager_disconnect(&d->crash_mgr) != CDM_STATUS_OK) {
            cdhlog(LOG_WARNING, "Fail to disconnect to manager socket");
        }
    }
#endif

    free(coredir);

    return status;
}
