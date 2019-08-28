/* cdh-context.c
 *
 * Copyright 2019 Alin Popa <alin.popa@bmw.de>
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

#include "cdh-context.h"
#include "cdh-archive.h"
#include "cdm-utils.h"

#include <glib.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CRASH_ID_HIGH (6)
#define CRASH_ID_LOW (2)
#define CRASH_ID_QUALITY(x) ((x) > CRASH_ID_HIGH ? "high" : (x) < CRASH_ID_LOW ? "low" : "medium")

static gchar ftypelet (mode_t bits);

static void strmode (mode_t mode, gchar str[12]);

static CdmStatus dump_file_to (CdhContext *ctx, const gchar *fname);

static CdmStatus list_dircontent_to (CdhContext *ctx, const gchar *dname);

static CdmStatus update_context_info (CdhContext *ctx);

static void crash_context_dump (CdhContext *ctx, gboolean postcore);

static CdmStatus create_crashid (CdhContext *ctx);

CdhContext *
cdh_context_new (CdmOptions *opts,
                 CdhArchive *archive)
{
  CdhContext *ctx = g_new0 (CdhContext, 1);

  g_assert (ctx);
  g_assert (opts);
  g_assert (archive);

  g_ref_count_init (&ctx->rc);

  ctx->opts = cdm_options_ref (opts);
  ctx->archive = cdh_archive_ref (archive);
  ctx->onhost = TRUE;

  return ctx;
}

CdhContext *
cdh_context_ref (CdhContext *ctx)
{
  g_assert (ctx);
  g_ref_count_inc (&ctx->rc);
  return ctx;
}

void
cdh_context_unref (CdhContext *ctx)
{
  g_assert (ctx);

  if (g_ref_count_dec (&ctx->rc) == TRUE)
    {
      if (ctx->opts != NULL)
        cdm_options_unref (ctx->opts);

      if (ctx->archive != NULL)
        cdh_archive_unref (ctx->archive);

#if defined(WITH_CRASHMANAGER)
      if (ctx->manager != NULL)
        cdh_manager_unref (ctx->manager);
#endif

      g_free (ctx->name);
      g_free (ctx->tname);
      g_free (ctx->pexe);
      g_free (ctx->contextid);
      g_free (ctx->context_name);
      g_free (ctx->crashid);
      g_free (ctx->vectorid);

      g_free (ctx);
    }
}

static CdmStatus
create_crashid (CdhContext *ctx)
{
  const gchar *locstr[] = { "host", "container" };
  const gchar *loc = NULL;
  g_autofree gchar *cid_str = NULL;
  g_autofree gchar *vid_str = NULL;

  g_assert (ctx);

  if (ctx->crashid_info & CID_IP_FILE_OFFSET)
    {
      if (ctx->crashid_info & CID_RA_FILE_OFFSET)
        {
          cid_str = g_strdup_printf ("%s%lx%s%s",
                                     ctx->name,
                                     ctx->ip_file_offset,
                                     ctx->ip_module_name,
                                     ctx->ra_module_name);
        }
      else
        {
          cid_str = g_strdup_printf ("%s%lx%s",
                                     ctx->name,
                                     ctx->ip_file_offset,
                                     ctx->ip_module_name);
        }
    }
  else
    {
#ifdef __x86_64__
      cid_str = g_strdup_printf ("%s%lx", ctx->name, ctx->regs.rip);
#elif __aarch64__
      cid_str = g_strdup_printf ("%s%lx", ctx->name, ctx->regs.lr);
#endif
    }

  /* hash and write crashid */
  ctx->crashid = g_strdup_printf ("%016lX", cdm_utils_jenkins_hash (cid_str));

  if (ctx->crashid_info & CID_RA_FILE_OFFSET)
    {
      vid_str = g_strdup_printf ("%s%lx%s",
                                 ctx->name,
                                 ctx->ip_file_offset,
                                 ctx->ra_module_name);

      ctx->vectorid = g_strdup_printf ("%016lX", cdm_utils_jenkins_hash (vid_str));
    }
  else
    {
      ctx->vectorid = g_strdup_printf ("%016lX", cdm_utils_jenkins_hash (cid_str));
    }

  /* crash context location string */
  loc = ctx->onhost == TRUE ? locstr[0] : locstr[1];
#ifdef __x86_64__
  g_info (
    "Crash in %s contextID=%s process=\"%s\" thread=\"%s\" pid=%ld cpid=%ld crashID=%s "
    "vectorID=%s confidence=\"%s\" signal=\"%s\" rip=0x%lx rbp=0x%lx retaddr=0x%lx "
    "IPFileOffset=0x%lx RAFileOffset=0x%lx IPModule=\"%s\" RAModule=\"%s\"",
    loc, ctx->contextid, ctx->name, ctx->tname, ctx->pid, ctx->cpid,
    ctx->crashid, ctx->vectorid, CRASH_ID_QUALITY (ctx->crashid_info),
    strsignal ((gint)ctx->sig), ctx->regs.rip, ctx->regs.rbp, ctx->ra, ctx->ip_file_offset,
    ctx->ra_file_offset, ctx->ip_module_name, ctx->ra_module_name);
#elif __aarch64__
  g_info (
    "Crash in %s contextID=%s process=\"%s\" thread=\"%s\" pid=%ld cpid=%ld crashID=%s "
    "vectorID=%s confidence=\"%s\" signal=\"%s\" pc=0x%lx lr=0x%lx retaddr=0x%lx "
    "IPFileOffset=0x%lx RAFileOffset=0x%lx IPModule=\"%s\" RAModule=\"%s\"",
    loc, ctx->contextid, ctx->name, ctx->tname, ctx->pid, ctx->cpid,
    ctx->crashid, ctx->vectorid, CRASH_ID_QUALITY (ctx->crashid_info),
    strsignal (ctx->sig), ctx->regs.pc, ctx->regs.lr, ctx->ra, ctx->ip_file_offset,
    ctx->ra_file_offset, ctx->ip_module_name, ctx->ra_module_name);
#endif

  return CDM_STATUS_OK;
}

CdmStatus
cdh_context_crashid_process (CdhContext *ctx)
{
  g_assert (ctx);

  if (create_crashid (ctx) != CDM_STATUS_OK)
    {
      g_warning ("CrashID not generated");
      return CDM_STATUS_ERROR;
    }

  return CDM_STATUS_OK;
}

static CdmStatus
dump_file_to (CdhContext *ctx,
              const gchar *fname)
{
  g_assert (ctx);
  g_assert (fname);

  if (g_file_test (fname, G_FILE_TEST_IS_REGULAR) == TRUE)
    return cdh_archive_add_system_file (ctx->archive, fname, NULL);

  if (g_file_test (fname, G_FILE_TEST_IS_DIR) == TRUE)
    return list_dircontent_to (ctx, fname);

  return CDM_STATUS_ERROR;
}

static gchar
ftypelet (mode_t bits)
{
  if (S_ISREG (bits))
    return '-';

  if (S_ISDIR (bits))
    return 'd';

  if (S_ISBLK (bits))
    return 'b';

  if (S_ISCHR (bits))
    return 'c';

  if (S_ISLNK (bits))
    return 'l';

  if (S_ISFIFO (bits))
    return 'p';

  if (S_ISSOCK (bits))
    return 's';

  return '?';
}

static void
strmode (mode_t mode,
         gchar str[12])
{
  g_assert (str);

  str[0] = ftypelet (mode);
  str[1] = mode & S_IRUSR ? 'r' : '-';
  str[2] = mode & S_IWUSR ? 'w' : '-';
  str[3] = (mode & S_ISUID ? (mode & S_IXUSR ? 's' : 'S') : (mode & S_IXUSR ? 'x' : '-'));
  str[4] = mode & S_IRGRP ? 'r' : '-';
  str[5] = mode & S_IWGRP ? 'w' : '-';
  str[6] = (mode & S_ISGID ? (mode & S_IXGRP ? 's' : 'S') : (mode & S_IXGRP ? 'x' : '-'));
  str[7] = mode & S_IROTH ? 'r' : '-';
  str[8] = mode & S_IWOTH ? 'w' : '-';
  str[9] = (mode & S_ISVTX ? (mode & S_IXOTH ? 't' : 'T') : (mode & S_IXOTH ? 'x' : '-'));
  str[10] = ' ';
  str[11] = '\0';
}
static CdmStatus
list_dircontent_to (CdhContext *ctx,
                    const gchar *dname)
{
  g_autoptr (GError) error = NULL;
  g_autofree gchar *output = NULL;
  CdmStatus status = CDM_STATUS_OK;
  GDir *gdir = NULL;
  const gchar *nfile = NULL;

  g_assert (dname);
  g_assert (ctx);

  gdir = g_dir_open (dname, 0, &error);
  if (error != NULL)
    return CDM_STATUS_ERROR;

  while ((nfile = g_dir_read_name (gdir)) != NULL)
    {
      g_autofree gchar *fmode = NULL;
      g_autofree gchar *fpath = NULL;
      g_autofree gchar *fline = NULL;
      gchar *output_tmp = NULL;
      gchar mbuf[12] = { 0 };
      struct stat fstat;

      fpath = g_build_filename (dname, nfile, NULL);
      if (lstat (fpath, &fstat) < 0)
        continue;

      strmode (fstat.st_mode, mbuf);

      switch (fstat.st_mode & S_IFMT)
        {
        case S_IFBLK:
          fmode = g_strdup (" [block device]");
          break;

        case S_IFCHR:
          fmode = g_strdup (" [character device]");
          break;

        case S_IFDIR:
          fmode = g_strdup (" [directory]");
          break;

        case S_IFIFO:
          fmode = g_strdup (" [FIFO/pipe]");
          break;

        case S_IFLNK:
        {
          g_autofree gchar *lnk_data = g_file_read_link (fpath, NULL);
          fmode = g_strdup_printf (" -> %s", lnk_data);
        }
        break;

        case S_IFREG:
          fmode = g_strdup (" [regular file]");
          break;

        case S_IFSOCK:
          fmode = g_strdup (" [socket]");
          break;

        default:
          fmode = g_strdup (" [unknown?]");
          break;
        }

      fline = g_strdup_printf ("%s  %u  %u %u %ld %4s %s",
                               mbuf,
                               (unsigned int)fstat.st_nlink,
                               fstat.st_uid,
                               fstat.st_gid,
                               fstat.st_size,
                               nfile,
                               fmode
                               );

      if (output != NULL)
        {
          output_tmp = g_strjoin ("\n", output, fline, NULL);
          g_free (output);
          output = output_tmp;
        }
      else
        {
          output = g_strdup_printf ("%s", fline);
        }
    }

  g_dir_close (gdir);

  if (output != NULL)
    {
      g_autofree gchar *outfile = g_strdup_printf ("root%s", dname);

      g_strdelimit (outfile, "/ ", '.');

      if (cdh_archive_create_file (ctx->archive, outfile, strlen (output)) == CDM_STATUS_OK)
        {
          status = cdh_archive_write_file (ctx->archive, (const void*)output, strlen (output));
          if (cdh_archive_finish_file (ctx->archive) != CDM_STATUS_OK)
            status = CDM_STATUS_ERROR;
        }
      else
        {
          status = CDM_STATUS_ERROR;
        }
    }

  return status;
}

static CdmStatus
update_context_info (CdhContext *ctx)
{
  g_autofree gchar *ctx_str = NULL;
  pid_t pid;

  g_assert (ctx);

  ctx->onhost = true;

  pid = getpid ();

  for (gint i = 0; i < 7; i++)
    {
      g_autofree gchar *host_ns_buf = NULL;
      g_autofree gchar *proc_ns_buf = NULL;
      g_autofree gchar *tmp_host_path = NULL;
      g_autofree gchar *tmp_proc_path = NULL;

      switch (i)
        {
        case 0: /* cgroup */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/cgroup", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/cgroup", ctx->pid);
          break;

        case 1: /* ipc */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/ipc", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/ipc", ctx->pid);
          break;

        case 2: /* mnt */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/mnt", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/mnt", ctx->pid);
          break;

        case 3: /* net */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/net", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/net", ctx->pid);
          break;

        case 4: /* pid */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/pid", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/pid", ctx->pid);
          break;

        case 5: /* user */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/user", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/user", ctx->pid);
          break;

        case 6: /* uts */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/uts", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/uts", ctx->pid);
          break;

        default: /* never reached */
          break;
        }

      host_ns_buf = g_file_read_link (tmp_host_path, NULL);
      proc_ns_buf = g_file_read_link (tmp_proc_path, NULL);

      if (g_strcmp0 (host_ns_buf, proc_ns_buf) != 0)
        ctx->onhost = false;

      if (ctx_str != NULL)
        {
          gchar *ctx_tmp = g_strconcat (ctx_str, proc_ns_buf, NULL);
          g_free (ctx_str);
          ctx_str = ctx_tmp;
        }
      else
        {
          ctx_str = g_strdup (proc_ns_buf);
        }
    }

  ctx->contextid = g_strdup_printf ("%016lX", cdm_utils_jenkins_hash (ctx_str));

  return ctx->contextid != NULL ? CDM_STATUS_OK : CDM_STATUS_ERROR;
}


static void
crash_context_dump (CdhContext *ctx,
                    gboolean postcore)
{
  GKeyFile *key_file = NULL;
  gchar **groups = NULL;

  g_assert (ctx);

  key_file = cdm_options_get_key_file (ctx->opts);
  groups = g_key_file_get_groups (key_file, NULL);

  for (gint i = 0; groups[i] != NULL; i++)
    {
      g_autoptr (GError) error = NULL;
      g_autofree gchar *proc_key = NULL;
      g_autofree gchar *data_key = NULL;
      g_autofree gchar *data_path = NULL;
      g_autofree gchar *str_pid = NULL;
      gchar **path_tokens = NULL;
      gchar *gname = groups[i];
      gboolean key_postcore = TRUE;

      if (g_regex_match_simple ("crashcontext-*", gname, 0, 0) == FALSE)
        continue;

      proc_key = g_key_file_get_string (key_file, gname, "ProcName", &error);
      if (error != NULL)
        continue;

      if (g_regex_match_simple (proc_key, ctx->name, 0, 0) == FALSE)
        continue;

      key_postcore = g_key_file_get_boolean (key_file, gname, "PostCore", &error);
      if (error != NULL)
        continue;

      if (key_postcore != postcore)
        continue;

      data_key = g_key_file_get_string (key_file, gname, "DataPath", &error);
      if (error != NULL)
        continue;

      if (g_access (data_key, R_OK) == 0)
        continue;

      path_tokens = g_strsplit (data_key, "$$", 3);
      str_pid = g_strdup_printf ("%ld", ctx->pid);
      data_path = g_strjoinv (str_pid, path_tokens);
      g_strfreev (path_tokens);

      if (dump_file_to (ctx, data_path) == CDM_STATUS_ERROR)
        g_warning ("Fail to dump file %s", data_path);
    }

  g_strfreev (groups);
}

CdmStatus
cdh_context_generate_prestream (CdhContext *ctx)
{
  g_autofree gchar *file = NULL;

  g_assert (ctx);

  if (update_context_info (ctx) == CDM_STATUS_ERROR)
    g_warning ("Fail to parse namespace information");

  crash_context_dump (ctx, FALSE);

  return CDM_STATUS_OK;
}

#if defined(WITH_CRASHMANAGER)
void
cdh_context_set_manager (CdhContext *ctx, CdhManager *manager)
{
  g_assert (ctx);
  ctx->manager = manager;
}

void
cdh_context_read_context_info (CdhContext *ctx)
{
  CdmMessage msg;

  cdm_message_init (&msg, CDM_CORE_UNKNOWN, 0);

  if (cdm_message_read (cdh_manager_get_socket (ctx->manager), &msg) != CDM_STATUS_OK)
    {
      g_debug ("Cannot read from manager socket %d", cdh_manager_get_socket (ctx->manager));
    }
  else if (msg.hdr.type == CDM_CONTEXT_INFO)
    {
      CdmMessageDataContextInfo *data = (CdmMessageDataContextInfo *)msg.data;

      ctx->context_name = g_strdup (data->context_name);
      ctx->lifecycle_state = g_strdup (data->lifecycle_state);

      cdm_message_free_data (&msg);
    }
}
#endif

CdmStatus
cdh_context_generate_poststream (CdhContext *ctx)
{
  g_autofree gchar *file_data = NULL;
  CdmStatus status = CDM_STATUS_OK;
  uint64_t ip, ra;

  g_assert (ctx);

#ifdef __aarch64__
  ip = ctx->regs.pc;
  ra = ctx->regs.lr;
#else
  ip = ctx->regs.rip;
  ra = ctx->regs.rbp;
#endif

  file_data = g_strdup_printf (
    "[crashdata]\n"
    "ProcessName    = %s\n"
    "ProcessThread  = %s\n"
    "ProcessExe     = %s\n"
    "LifecycleState = %s\n"
    "CrashTimestamp = %lu\n"
    "ProcessID      = %ld\n"
    "ResidentID     = %ld\n"
    "CrashSignal    = %ld\n"
    "CrashID        = %s\n"
    "VectorID       = %s\n"
    "ContextID      = %s\n"
    "ContextName    = %s\n"
    "IP             = 0x%016lx\n"
    "RA             = 0x%016lx\n"
    "IPFileOffset   = 0x%016lx\n"
    "RAFileOffset   = 0x%016lx\n"
    "IPModuleName   = %s\n"
    "RAModuleName   = %s\n"
    "CoredumpSize   = %lu\n",
    ctx->name,
    ctx->tname,
    ctx->pexe,
    ctx->lifecycle_state,
    ctx->tstamp,
    ctx->pid,
    ctx->cpid,
    ctx->sig,
    ctx->crashid,
    ctx->vectorid,
    ctx->contextid,
    ctx->context_name,
    ip,
    ra,
    ctx->ip_file_offset,
    ctx->ra_file_offset,
    ctx->ip_module_name,
    ctx->ra_module_name,
    ctx->cdsize
    );

  if (cdh_archive_create_file (ctx->archive, "info.crashdata", strlen (file_data))
      == CDM_STATUS_OK)
    {
      status = cdh_archive_write_file (ctx->archive,
                                       (const void*)file_data,
                                       strlen (file_data));

      if (cdh_archive_finish_file (ctx->archive) != CDM_STATUS_OK)
        status = CDM_STATUS_ERROR;
    }
  else
    {
      status = CDM_STATUS_ERROR;
    }

  crash_context_dump (ctx, TRUE);

  return status;
}
