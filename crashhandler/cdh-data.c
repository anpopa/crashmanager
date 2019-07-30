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
#include "cdm-message.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

static void remove_invalid_chars (gchar *str);

static CdmStatus read_args (CdhData *d, gint argc, gchar **argv);

static CdmStatus check_disk_space (const gchar *path, gsize min);

static CdmStatus check_and_create_directory (CdhData *d, const gchar *dirname);

static CdmStatus wait_filesystem (const gchar *path, gulong check_interval, gsize max_time);

static CdmStatus init_coredump_archive (CdhData *d, const gchar *dirname);

void
cdh_data_init (CdhData *d, const gchar *config_path)
{
  g_assert (d);

  memset (d, 0, sizeof(CdhData));

  d->opts = cdm_options_new (config_path);
  g_assert (d->opts);

  d->info = cdh_info_new ();
  g_assert (d->info);
}

void
cdh_data_deinit (CdhData *d)
{
  if (d->info)
    {
      cdh_info_free (d->info);
    }

  if (d->opts)
    {
      cdm_options_free (d->opts);
    }
}

static CdmStatus
read_args (CdhData *d, gint argc, gchar **argv)
{
  g_assert (d);

  if (argc < 6)
    {
      g_warning ("Usage: coredumper tstamp pid cpid sig procname");
      return CDM_STATUS_ERROR;
    }

  d->info->tstamp = g_ascii_strtoll (argv[1], NULL, 10);
  if (d->info->tstamp == 0)
    {
      g_warning ("Unable to read tstamp argument %s", argv[1]);
      return CDM_STATUS_ERROR;
    }

  d->info->pid = g_ascii_strtoll (argv[2], NULL, 10);
  if (d->info->pid == 0)
    {
      g_warning ("Unable to read pid argument %s", argv[2]);
      return CDM_STATUS_ERROR;
    }

  d->info->cpid = g_ascii_strtoll (argv[3], NULL, 10);
  if (d->info->cpid == 0)
    {
      g_warning ("Unable to read context cpid argument %s", argv[3]);
      return CDM_STATUS_ERROR;
    }

  d->info->sig = g_ascii_strtoll (argv[4], NULL, 10);
  if (d->info->sig == 0)
    {
      g_warning ("Unable to read sig argument %s", argv[4]);
      return CDM_STATUS_ERROR;
    }

  d->info->tname = g_strdup (argv[5]);
  d->info->name = g_strdup (argv[5]);

  return CDM_STATUS_OK;
}

static CdmStatus
check_disk_space (const gchar *path, gsize min)
{
  struct statvfs stat;
  gsize free_sz = 0;

  g_assert (path);

  if (statvfs (path, &stat) < 0)
    {
      g_warning ("Cannot stat disk space on %s: %s", path, strerror (errno));
      return CDM_STATUS_ERROR;
    }

  /* free space: size of block * number of free blocks (>>20 => MB) */
  free_sz = (stat.f_bsize * stat.f_bavail) >> 20;

  if (free_sz < min)
    {
      g_warning ("Insufficient disk space for coredump: %ld MB.", free_sz);
      return CDM_STATUS_ERROR;
    }

  return CDM_STATUS_OK;
}

static CdmStatus
init_coredump_archive (CdhData *d, const gchar *dirname)
{
  g_autofree gchar *aname = NULL;

  g_assert (d);
  g_assert (dirname);

  aname = g_strdup_printf (ARCHIVE_NAME_PATTERN, dirname, d->info->name, d->info->pid);

  if (cdh_archive_open (&d->archive, aname) != CDM_STATUS_OK)
    {
      return CDM_STATUS_ERROR;
    }

  return CDM_STATUS_OK;
}

CdmStatus
cdh_main_enter (CdhData *d, gint argc, gchar *argv[])
{
  CdmStatus status = CDM_STATUS_OK;
  g_autofree gchar *opt_coredir = NULL;
  gchar *procname = NULL;
  gsize opt_fs_min_size;
  gint opt_nice_value;

  g_assert (d);

  if (read_args (d, argc, argv) < 0)
    {
      status = CDM_STATUS_ERROR;
      goto enter_cleanup;
    }

  procname = cdh_context_procname (d->info->pid);
  if (procname != NULL)
    {
      g_free (d->info->name);
      d->info->name = procname;
    }

  g_strdelim (d->info->name, ":/\\!*", '_');

  g_info ("New process crash: name=%s pid=%d signal=%d timestamp=%ld",
          d->info->name, d->info->pid, d->info->sig, d->info->tstamp);

#if defined(WITH_COREMANAGER)
  if (cdh_manager_init (&d->crash_mgr, d->opts) != CDM_STATUS_OK)
    {
      g_warning ("Crashhandler object init failed");
    }

  if (cdh_manager_connect (&d->crash_mgr) != CDM_STATUS_OK)
    {
      g_warning ("Fail to connect to manager socket");
    }
  else
    {
      CdmMessage msg;
      CdmMessageDataNew data;

      cdm_message_init (&msg, CDM_CORE_NEW, (guint16)((gulong)d->info->pid | d->info->tstamp));

      data.pid = d->info->pid;
      data.coresig = d->info->sig;
      data.tstamp = d->info->tstamp;
      memcpy (data.tname, d->info->tname, strlen (d->info->tname) + 1);
      memcpy (data.pname, d->info->name, strlen (d->info->name) + 1);

      cdm_message_set_data (&msg, &data, sizeof(data));

      if (cdm_manager_send (&d->crash_mgr, &msg) == CDM_STATUS_ERROR)
        {
          g_warning ("Failed to send new message to manager");
        }
    }
#endif

  /* get optionals */
  opt_coredir = cdm_options_string_for (d->opts, KEY_CRASHDUMP_DIR, &opt_status);
  opt_fs_min_size = (gsize)cdm_options_long_for (d->opts, KEY_FILESYSTEM_MIN_SIZE, &opt_status);
  opt_nice_value = (gint)cdm_options_long_for (d->opts, KEY_ELEVATED_NICE_VALUE, &opt_status);

  g_debug ("Coredump database path %s", opt_coredir);

  if (nice (opt_nice_value) != opt_nice_value)
    {
      g_warning ("Failed to change crashhandler priority");
    }

  if (g_mkdir_with_parents (opt_coredir, 0755) != 0)
    {
      status = CDM_STATUS_ERROR;
      goto enter_cleanup;
    }

  if (check_disk_space (opt_coredir, opt_fs_min_size) != CDM_STATUS_OK)
    {
      status = CDM_STATUS_ERROR;
      goto enter_cleanup;
    }

  if (init_coredump_archive (d, opt_coredir) != CDM_STATUS_OK)
    {
      g_warning ("Fail to create coredump archive");
      status = CDM_STATUS_ERROR;
      goto enter_cleanup;
    }

  if (cdh_context_generate_prestream (d) != CDM_STATUS_OK)
    {
      g_warning ("Failed to generate the context file, continue with coredump");
    }

  if (cdh_coredump_generate (d) != CDM_STATUS_OK)
    {
      g_warning ("Coredump handling failed");
      status = CDM_STATUS_ERROR;
      goto enter_cleanup;
    }

  if (cdh_context_generate_poststream (d) != CDM_STATUS_OK)
    {
      g_warning ("Failed to generate the context file, continue with coredump");
    }

enter_cleanup:
#if defined(WITH_CRASHMANAGER)
  cdh_manager_set_coredir (&d->crash_mgr, opt_coredir);

  if (cdh_manager_connected (&d->crash_mgr))
    {
      CdmMessage msg;
      CdmMessageType type;

      type = (status == CDM_STATUS_OK ? CDM_CORE_COMPLETE : CDM_CORE_FAILED);

      cdm_message_init (&msg, type, (guint16)((gulong)d->info->pid | d->info->tstamp));

      if (cdm_manager_send (&d->crash_mgr, &msg) == CDM_STATUS_ERROR)
        {
          g_warning ("Failed to send status message to manager");
        }

      if (cdm_manager_disconnect (&d->crash_mgr) != CDM_STATUS_OK)
        {
          g_warning ("Fail to disconnect to manager socket");
        }
    }
#endif

  return status;
}
