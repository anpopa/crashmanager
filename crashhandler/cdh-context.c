/* cdh-context.h
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

/* Global buffer for file reading */
#ifndef CONTEXT_TMP_BUFFER_SIZE
#define CONTEXT_TMP_BUFFER_SIZE (4096)
#endif

static char g_buffer[CONTEXT_TMP_BUFFER_SIZE];

static CdmStatus dump_file_to (gchar *fname, CdhArchive *ar);

static CdmStatus list_dircontent_to (gchar *dname, CdhArchive *ar);

static CdmStatus update_context_info (CdhData *d);

static void crash_context_dump (CdhData *d, gboolean postcore);

gchar *
cdh_context_get_procname (gint64 pid)
{
  g_autofree gchar *statfile = NULL;
  gboolean done = FALSE;
  gchar *retval = NULL;
  FILE *fstm;

  statfile = g_strdup_printf ("/proc/%ld/status", pid);

  if ((fstm = fopen (statfile, "r")) == NULL)
    {
      g_warning ("Open status file '%s' for dumping %s", statfile, strerror (errno));
      return NULL;
    }

  while (fgets (g_buffer, sizeof(g_buffer), fstm) && !done)
    {
      gchar *name = g_strrstr (g_buffer, "Name:");

      if (name != NULL)
        {
          retval = g_strdup (name + strlen ("Name:") + 1);

          if (retval != NULL)
            {
              g_strstrip (retval);
            }
          done = TRUE;
        }
    }

  fclose (fstm);

  return retval;
}

static CdmStatus
dump_file_to (gchar *fname, CdhArchive *ar)
{
  if (g_file_test (fname, G_FILE_TEST_IS_REGULAR) == TRUE)
    {
      return cdh_archive_add_system_file (ar, fname, NULL);
    }

  if (g_file_test (fname, G_FILE_TEST_IS_DIR) == TRUE)
    {
      return list_dircontent_to (fname, ar);
    }

  return CDM_STATUS_ERROR;
}

static CdmStatus
list_dircontent_to (gchar *dname, CdhArchive *ar)
{
  g_assert (dname);
  g_assert (ar);

  return CDM_STATUS_OK;
}

static CdmStatus
update_context_info (CdhData *d)
{
  g_autofree gchar *ctx_str = NULL;
  pid_t pid;

  g_assert (d);

  d->info->onhost = true;

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
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/cgroup", d->info->pid);
          break;

        case 1: /* ipc */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/ipc", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/ipc", d->info->pid);
          break;

        case 2: /* mnt */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/mnt", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/mnt", d->info->pid);
          break;

        case 3: /* net */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/net", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/net", d->info->pid);
          break;

        case 4: /* pid */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/pid", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/pid", d->info->pid);
          break;

        case 5: /* user */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/user", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/user", d->info->pid);
          break;

        case 6: /* uts */
          tmp_host_path = g_strdup_printf ("/proc/%d/ns/uts", pid);
          tmp_proc_path = g_strdup_printf ("/proc/%ld/ns/uts", d->info->pid);
          break;

        default: /* never reached */
          break;
        }

      host_ns_buf = g_file_read_link (tmp_host_path, NULL);
      proc_ns_buf = g_file_read_link (tmp_proc_path, NULL);

      if (g_strcmp0 (host_ns_buf, proc_ns_buf) != 0)
        {
          d->info->onhost = false;
        }

      if (ctx_str != NULL)
        {
          ctx_str = g_strconcat (ctx_str, proc_ns_buf, NULL);
        }
      else
        {
          ctx_str = g_strdup (proc_ns_buf);
        }
    }

  d->info->contextid = g_strdup_printf ("%016lX", cdm_utils_jenkins_hash (ctx_str));

  return d->info->contextid != NULL ? CDM_STATUS_OK : CDM_STATUS_ERROR;
}

static void
crash_context_dump (CdhData *d, gboolean postcore)
{
  GKeyFile *key_file = cdm_options_get_key_file (d->opts);
  gchar **groups = g_key_file_get_groups (key_file, NULL);

  for (gint i = 0; groups[i] != NULL; i++)
    {
      g_autofree gchar *key = NULL;
      g_autofree gchar *data_path = NULL;
      g_autofree gchar *str_pid = NULL;
      gchar **path_tokens = NULL;
      gchar *gname = groups[i];
      GError *error = NULL;
      gboolean key_postcore = TRUE;

      if (g_regex_match_simple ("crashcontext-*", gname, 0, 0) == FALSE)
        {
          continue;
        }

      key = g_key_file_get_string (key_file, gname, "ProcName", &error);
      if (error != NULL)
        {
          g_error_free (error);
          continue;
        }

      if (g_regex_match_simple (key, d->info->name, 0, 0) == FALSE)
        {
          continue;
        }

      key_postcore = g_key_file_get_boolean (key_file, gname, "PostCore", &error);
      if (error != NULL)
        {
          g_error_free (error);
          continue;
        }

      if (key_postcore != postcore)
        {
          continue;
        }

      key = g_key_file_get_string (key_file, gname, "DataPath", &error);
      if (error != NULL)
        {
          g_error_free (error);
          continue;
        }

      path_tokens = g_strsplit (key, "$$", 3);
      str_pid = g_strdup_printf ("%ld", d->info->pid);
      data_path = g_strjoinv (str_pid, path_tokens);
      g_strfreev (path_tokens);

      if (dump_file_to (data_path, &d->archive) == CDM_STATUS_ERROR)
        {
          g_warning ("Fail to dump file %s", data_path);
        }
    }

  g_strfreev (groups);
}

CdmStatus
cdh_context_generate_prestream (CdhData *d)
{
  g_autofree gchar *file = NULL;

  g_assert (d);

  if (update_context_info (d) == CDM_STATUS_ERROR)
    {
      g_warning ("Fail to parse namespace information");
    }

  crash_context_dump (d, FALSE);

  return CDM_STATUS_OK;
}

CdmStatus
cdh_context_generate_poststream (CdhData *d)
{
  g_autofree gchar *file_data = NULL;
  CdmStatus status = CDM_STATUS_OK;

  g_assert (d);

  file_data = g_strdup_printf (
    "ProcessName = %s\nProcessThread = %s\nCrashTimestamp = %lu\n"
    "ProcessHostID = %ld\nProcessContainerID = %ld\nCrashSignal = %ld\n"
    "CrashID = %s\nVectorID = %s\nContextID = %s\n"
    "CoredumpSize = %lu\n",
    d->info->name, d->info->tname, d->info->tstamp, d->info->pid, d->info->cpid,
    d->info->sig, d->info->crashid, d->info->vectorid, d->info->contextid, d->info->cdsize
    );

  if (cdh_archive_create_file (&d->archive, "info.crashdata", strlen (file_data) + 1) == CDM_STATUS_OK)
    {
      status = cdh_archive_write_file (&d->archive, (const void*)file_data, strlen (file_data) + 1);

      if (cdh_archive_finish_file (&d->archive) != CDM_STATUS_OK)
        {
          status = CDM_STATUS_ERROR;
        }
    }
  else
    {
      status = CDM_STATUS_ERROR;
    }

  crash_context_dump (d, TRUE);

  return status;
}
