/* cdi-journal.c
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

#include "cdi-journal.h"
#include "cdm-utils.h"

#include <glib/gprintf.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef WITH_LXC
#include <lxc/lxccontainer.h>
#endif

typedef enum _JournalQueryType {
  QUERY_LIST_ENTRIES
} JournalQueryType;

typedef struct _JournalQueryData {
  JournalQueryType type;
  gpointer response;
} JournalQueryData;

const gchar *cdi_journal_table_name = "CrashTable";

static int sqlite_callback (void *data, int argc, char **argv, char **colname);

static gchar *
get_pid_context_id (pid_t pid)
{
  g_autofree gchar *ctx_str = NULL;

  for (gint i = 0; i < 7; i++)
    {
      g_autofree gchar *proc_ns_buf = NULL;
      g_autofree gchar *tmp_proc_path = NULL;

      switch (i)
        {
        case 0: /* cgroup */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/cgroup", pid);
          break;

        case 1: /* ipc */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/ipc", pid);
          break;

        case 2: /* mnt */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/mnt", pid);
          break;

        case 3: /* net */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/net", pid);
          break;

        case 4: /* pid */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/pid", pid);
          break;

        case 5: /* user */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/user", pid);
          break;

        case 6: /* uts */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/uts", pid);
          break;

        default: /* never reached */
          break;
        }

      proc_ns_buf = g_file_read_link (tmp_proc_path, NULL);

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

  if (ctx_str == NULL)
    return NULL;

  return g_strdup_printf ("%016lX", cdm_utils_jenkins_hash (ctx_str));
}

#ifdef WITH_LXC
static gchar *
get_container_name_for_context (const gchar *ctxid)
{
  struct lxc_container **active_containers = NULL;
  gchar *container_name = NULL;
  gboolean found = false;
  gchar **names = NULL;
  gint count = 0;

  count = list_active_containers ("/var/lib/lxc", &names, &active_containers);

  for (gint i = 0; i < count && !found; i++)
    {
      struct lxc_container *container = active_containers[i];
      gchar *name = names[i];

      if (name == NULL || container == NULL)
        continue;

      if (container->is_running (container))
        {
          g_autofree gchar *tmp_id = NULL;
          pid_t pid;

          pid = container->init_pid (container);
          tmp_id = get_pid_context_id (pid);

          if (g_strcmp0 (tmp_id, ctxid) != 0)
            {
              container_name = g_strdup_printf ("%s", name);
              found = true;
            }
        }
    }

  for (int i = 0; i < count; i++)
    {
      free (names[i]);
      free (active_containers[i]);
    }

  return container_name;
}
#endif

static int
sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  JournalQueryData *querydata = (JournalQueryData *)data;

  CDM_UNUSED (colname);

  switch (querydata->type)
    {
    case QUERY_LIST_ENTRIES:
      if (argc == 13)
        {
          g_autoptr (GDateTime) dtime = NULL;
          g_autofree gchar *host_id = NULL;
          g_autofree gchar *context_name = NULL;
          g_autofree gchar *fname = NULL;

          dtime = g_date_time_new_from_unix_utc (g_ascii_strtoll (argv[9], NULL, 10));
          fname = g_path_get_basename (argv[5]);
          host_id = get_pid_context_id (getpid ());

          if (g_strcmp0 (host_id, argv[4]) == 0)
            {
              context_name = g_strdup (g_get_host_name ());
            }
          else
            {
#ifdef WITH_LXC
              context_name = get_container_name_for_context (argv[4]);
#else
              context_name = g_strdup ("Container");
#endif
            }

          g_print ("%-4u %-20s %20s %16s %16s %16s %6s %3s %3s  %s\n",
                   *(guint *)(querydata->response),
                   argv[1],
                   (dtime != NULL) ? g_date_time_format (dtime, "%H:%M:%S %Y-%m-%d") : argv[9],
                   argv[2],
                   argv[3],
                   context_name,
                   argv[7],
                   argv[11],
                   argv[12],
                   fname);

          *((guint *)(querydata->response)) += 1;
        }
      break;

    default:
      break;
    }

  return(0);
}

CdiJournal *
cdi_journal_new (CdmOptions *options, GError **error)
{
  CdiJournal *journal = g_new0 (CdiJournal, 1);
  g_autofree gchar *opt_dbpath = NULL;

  g_assert (journal);

  g_ref_count_init (&journal->rc);
  opt_dbpath = cdm_options_string_for (options, KEY_DATABASE_FILE);

  if (sqlite3_open_v2 (opt_dbpath, &journal->database, SQLITE_OPEN_READONLY, NULL))
    {
      g_warning ("Cannot open journal database at path %s", opt_dbpath);
      g_set_error (error, g_quark_from_static_string ("JournalNew"), 1, "Database open failed");
    }

  return journal;
}

CdiJournal *
cdi_journal_ref (CdiJournal *journal)
{
  g_assert (journal);
  g_ref_count_inc (&journal->rc);
  return journal;
}

void
cdi_journal_unref (CdiJournal *journal)
{
  g_assert (journal);

  if (g_ref_count_dec (&journal->rc) == TRUE)
    g_free (journal);
}

void
cdi_journal_list_entries (CdiJournal *journal,
                          GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  guint index = 1;
  JournalQueryData data = {
    .type = QUERY_LIST_ENTRIES,
    .response = NULL
  };

  g_assert (journal);

  data.response = &index;
  sql = g_strdup_printf ("SELECT * FROM %s ;", cdi_journal_table_name);

  g_print ("%-4s %-20s %20s %16s %16s %16s %6s %3s %3s  %s\n",
           "Idx",
           "Procname",
           "Timestamp",
           "CrashID",
           "VectorID",
           "Context",
           "PID",
           "TRS",
           "REM",
           "FILE");

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_set_error (error,
                   g_quark_from_static_string ("JournalListEntries"),
                   1,
                   "SQL query error");
      g_warning ("Fail to get entry count. SQL error %s", query_error);
      sqlite3_free (query_error);
    }
}

