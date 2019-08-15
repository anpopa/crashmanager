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

#include <stdio.h>

typedef enum _JournalQueryType {
  QUERY_LIST_ENTRIES
} JournalQueryType;

typedef struct _JournalQueryData {
  JournalQueryType type;
  gpointer response;
} JournalQueryData;

const gchar *cdi_journal_table_name = "CrashTable";

static int sqlite_callback (void *data, int argc, char **argv, char **colname);

static int
sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  JournalQueryData *querydata = (JournalQueryData *)data;

  CDM_UNUSED (querydata);
  CDM_UNUSED (colname);

  switch (querydata->type)
    {
    case QUERY_LIST_ENTRIES:
      if (argc == 13)
        {
          printf ("%-20s%20s%20s%20s%10s%5s%5s\n",
                  argv[1],
                  argv[2],
                  argv[3],
                  argv[4],
                  argv[7],
                  argv[11],
                  argv[12]);
        }
      break;

    default:
      break;
    }

  return(0);
}

CdiJournal *
cdi_journal_new (CdmOptions *options)
{
  CdiJournal *journal = g_new0 (CdiJournal, 1);
  g_autofree gchar *opt_dbpath = NULL;

  g_assert (journal);

  g_ref_count_init (&journal->rc);
  opt_dbpath = cdm_options_string_for (options, KEY_DATABASE_FILE);

  if (sqlite3_open_v2 (opt_dbpath, &journal->database, SQLITE_OPEN_READONLY, NULL))
    g_error ("Cannot open journal database at path %s", opt_dbpath);

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
  JournalQueryData data = {
    .type = QUERY_LIST_ENTRIES,
    .response = NULL
  };

  g_assert (journal);

  sql = g_strdup_printf ("SELECT * FROM %s ;", cdi_journal_table_name);

  printf ("%-20s%20s%20s%20s%10s%5s%5s\n",
                  "Procname",
                  "CrashID",
                  "VectorID",
                  "ContextID",
                  "PID",
                  "TRAN",
                  "REMV");

  printf ("-------------------------------------------------------------"
          "----------------------------------------\n");

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

