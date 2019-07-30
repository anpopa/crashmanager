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

#ifndef CONTEXT_LINE_BUFFER_SIZE
#define CONTEXT_LINE_BUFFER_SIZE (1024)
#endif

#ifndef CONTEXT_NSLINK_BUFFER_SIZE
#define CONTEXT_NSLINK_BUFFER_SIZE (20)
#endif

#ifndef CONTEXT_NSPATH_LEN
#define CONTEXT_NSPATH_LEN (64)
#endif

#ifndef CONTEXT_ID_TMP_BUFFER_SIZE
#define CONTEXT_ID_TMP_BUFFER_SIZE (20 * 7)
#endif

static gchar g_buffer[CONTEXT_TMP_BUFFER_SIZE];

static gchar ftypelet (mode_t bits);

static void strmode (mode_t mode, gchar *str);

static CdmStatus dump_file_to (gchar *fname, CdhArchive *ar);

static CdmStatus list_dircontent_to (gchar *dname, CdhArchive *ar);

static CdmStatus update_context_info (CdhData *d);

CdmStatus
cdh_context_get_procname (pid_t pid, gchar *exec_name, gsize exec_name_maxsize)
{
  FILE *fstm;
  gchar statfile[CORE_MAX_FILENAME_LENGTH];
  gboolean done = false;

  g_assert (exec_name);

  memset (statfile, 0, sizeof(statfile));
  snprintf (statfile, sizeof(statfile), "/proc/%d/status", pid);

  if ((fstm = fopen (statfile, "r")) == NULL)
    {
      g_warning ("Open status file '%s' for dumping [%s]", statfile, strerror (errno));
      return CDM_STATUS_ERROR;
    }

  while (fgets (g_buffer, sizeof(g_buffer), fstm) && !done)
    {
      if (strstr (g_buffer, "Name:") != NULL)
        {
          gssize sz;

          memset (exec_name, 0, exec_name_maxsize);
          sz = snprintf (exec_name, exec_name_maxsize, "%s", g_buffer + strlen ("Name:") + 1);
          if ((0 < sz) && (sz < (gssize)exec_name_maxsize))
            {
              if (exec_name[sz - 1] == '\n')
                {
                  exec_name[sz - 1] = '\0';
                }

              done = true;
            }
        }
    }

  fclose (fstm);

  return done == true ? CDM_STATUS_OK : CDM_STATUS_ERROR;
}

static CdmStatus
dump_file_to (gchar *fname, CdhArchive *ar)
{
  g_assert (fname);
  g_assert (ar);

  return CDM_STATUS_OK;
}

static gchar
ftypelet (mode_t bits)
{
  if (S_ISREG (bits))
    {
      return '-';
    }

  if (S_ISDIR (bits))
    {
      return 'd';
    }

  if (S_ISBLK (bits))
    {
      return 'b';
    }

  if (S_ISCHR (bits))
    {
      return 'c';
    }

  if (S_ISLNK (bits))
    {
      return 'l';
    }

  if (S_ISFIFO (bits))
    {
      return 'p';
    }

  if (S_ISSOCK (bits))
    {
      return 's';
    }

  return '?';
}

static void
strmode (mode_t mode, gchar *str)
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
list_dircontent_to (gchar *dname, CdhArchive *ar)
{
  g_assert (dname);
  g_assert (ar);

  return CDM_STATUS_OK;
}

static CdmStatus
update_context_info (CdhData *d)
{
  gchar host_ns_buf[CONTEXT_NSLINK_BUFFER_SIZE];
  gchar proc_ns_buf[CONTEXT_NSLINK_BUFFER_SIZE];
  gchar tmp_host_path[CONTEXT_NSPATH_LEN];
  gchar tmp_proc_path[CONTEXT_NSPATH_LEN];
  gchar tmp_context[CONTEXT_ID_TMP_BUFFER_SIZE];
  gint count = -1;
  gssize sz;
  pid_t pid;

  g_assert (d);

  memset (tmp_context, 0, sizeof(tmp_context));
  d->info->onhost = true;

  pid = getpid ();

  for (gint i = 0; i < 7; i++)
    {
      memset (host_ns_buf, 0, sizeof(host_ns_buf));
      memset (proc_ns_buf, 0, sizeof(proc_ns_buf));
      memset (tmp_host_path, 0, sizeof(tmp_host_path));
      memset (tmp_proc_path, 0, sizeof(tmp_proc_path));

      switch (i)
        {
        case 0: /* cgroup */
          sz = snprintf (tmp_host_path, sizeof(tmp_host_path), "/proc/%d/ns/cgroup", pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_host_path))), NULL);

          sz = snprintf (tmp_proc_path, sizeof(tmp_proc_path), "/proc/%d/ns/cgroup", d->info->pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_proc_path))), NULL);
          break;

        case 1: /* ipc */
          sz = snprintf (tmp_host_path, sizeof(tmp_host_path), "/proc/%d/ns/ipc", pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_host_path))), NULL);

          sz = snprintf (tmp_proc_path, sizeof(tmp_proc_path), "/proc/%d/ns/ipc", d->info->pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_proc_path))), NULL);
          break;

        case 2: /* mnt */
          sz = snprintf (tmp_host_path, sizeof(tmp_host_path), "/proc/%d/ns/mnt", pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_host_path))), NULL);

          sz = snprintf (tmp_proc_path, sizeof(tmp_proc_path), "/proc/%d/ns/mnt", d->info->pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_proc_path))), NULL);
          break;

        case 3: /* net */
          sz = snprintf (tmp_host_path, sizeof(tmp_host_path), "/proc/%d/ns/net", pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_host_path))), NULL);

          sz = snprintf (tmp_proc_path, sizeof(tmp_proc_path), "/proc/%d/ns/net", d->info->pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_proc_path))), NULL);
          break;

        case 4: /* pid */
          sz = snprintf (tmp_host_path, sizeof(tmp_host_path), "/proc/%d/ns/pid", pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_host_path))), NULL);

          sz = snprintf (tmp_proc_path, sizeof(tmp_proc_path), "/proc/%d/ns/pid", d->info->pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_proc_path))), NULL);
          break;

        case 5: /* user */
          sz = snprintf (tmp_host_path, sizeof(tmp_host_path), "/proc/%d/ns/user", pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_host_path))), NULL);

          sz = snprintf (tmp_proc_path, sizeof(tmp_proc_path), "/proc/%d/ns/user", d->info->pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_proc_path))), NULL);
          break;

        case 6: /* uts */
          sz = snprintf (tmp_host_path, sizeof(tmp_host_path), "/proc/%d/ns/uts", pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_host_path))), NULL);

          sz = snprintf (tmp_proc_path, sizeof(tmp_proc_path), "/proc/%d/ns/uts", d->info->pid);
          cdhbail (BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(tmp_proc_path))), NULL);
          break;

        default: /* never reached */
          break;
        }

      sz = readlink (tmp_host_path, host_ns_buf, sizeof(host_ns_buf));
      if ((sz < 0) || (sz >= (gssize)sizeof(host_ns_buf)))
        {
          g_warning ("File system path lenght too long: %s", tmp_host_path);
        }

      sz = readlink (tmp_proc_path, proc_ns_buf, sizeof(proc_ns_buf));
      if ((sz < 0) || (sz >= (gssize)sizeof(host_ns_buf)))
        {
          g_warning ("File system path lenght too long: %s", tmp_proc_path);
        }

      if (strncmp (host_ns_buf, proc_ns_buf, sizeof(host_ns_buf)) != 0)
        {
          d->info->onhost = false;
        }

      strncat (tmp_context, proc_ns_buf, sizeof(tmp_context) - (strlen (tmp_context) + 1));
    }

  count = snprintf (d->info->contextid, sizeof(d->info->contextid), "%016lX",
                    cd_util_jenkins_hash (tmp_context));

  return((count > 0) && (count < (gint)sizeof(d->info->contextid)) ? CDM_STATUS_OK : CDM_STATUS_ERROR);
}

CdmStatus
cdh_context_generate_prestream (CdhData *d)
{
  gchar pfile[CDM_PATH_MAX];

  g_assert (d);

  if (update_context_info (d) == CDM_STATUS_ERROR)
    {
      g_warning ("Fail to parse namespace information");
    }

#define PROC_FILENAME(x)                                                                           \
  do {                                                                                           \
      snprintf (pfile, sizeof(pfile), "/proc/%d/" x, d->info->pid);                                \
    } while (0)

  if (dump_file_to ("/etc/os-release", &d->archive) != CDM_STATUS_OK)
    {
      g_debug ("Fail to dump /etc/os-release");
    }

  PROC_FILENAME ("cmdline");
  if (dump_file_to (pfile, &d->archive) != CDM_STATUS_OK)
    {
      g_debug ("Fail to dump cmdline");
    }

  PROC_FILENAME ("fd");
  if (list_dircontent_to (pfile, &d->archive) != CDM_STATUS_OK)
    {
      g_debug ("Fail to list fd");
    }

  PROC_FILENAME ("ns");
  if (list_dircontent_to (pfile, &d->archive) != CDM_STATUS_OK)
    {
      g_debug ("Fail to list ns");
    }
#undef PROC_FILENAME

  return CDM_STATUS_OK;
}

CdmStatus
cdh_context_generate_poststream (CdhData *d)
{
  g_assert (d);

  return CDM_STATUS_OK;
}
