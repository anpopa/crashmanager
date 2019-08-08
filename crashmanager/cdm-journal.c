/* cdm-journal.c
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

#include "cdm-journal.h"
#include "cdm-utils.h"

typedef enum _JournalQueryType {
    QUERY_CREATE, 
    QUERY_ADD_ENTRY
} JournalQueryType;

typedef struct _JournalQueryData {
    JournalQueryType type;
    gpointer response;
} JournalQueryData;

const gchar *cdm_journal_table_name = "CrashTable";

static int sqlite_callback(void *data, int argc, char **argv, char **azColName);

static int 
sqlite_callback(void *data, int argc, char **argv, char **azColName)
{
  CDM_UNUSED (data);
  CDM_UNUSED (argc);
  CDM_UNUSED (argv);
  CDM_UNUSED (azColName);

  return 0;
}

CdmJournal *
cdm_journal_new (const gchar *dbpath)
{
  CdmJournal *journal = NULL;
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  JournalQueryData data = {
      .type = QUERY_CREATE, 
      .response = NULL
  };

  g_assert (dbpath);
  
  journal = g_new0 (CdmJournal, 1);
  
  g_assert (journal);

  g_ref_count_init (&journal->rc);
  g_ref_count_inc (&journal->rc);

  if (sqlite3_open (dbpath, &journal->database))
    {
      g_error ("Cannot open journal database at path %s", dbpath);
    }

  sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s       "
                         "(ID INT PRIMARY KEY     NOT   NULL, "
                         "PROCNAME        TEXT    NOT   NULL, "
                         "CRASHID         TEXT    NOT   NULL, "
                         "VECTORID        TEXT    NOT   NULL, "
                         "CONTEXTID       TEXT    NOT   NULL, "
                         "FILEPATH        TEXT    NOT   NULL, "
                         "PID             INT     NOT   NULL, "
                         "SIGNAL          INT     NOT   NULL, "
                         "TIMESTAMP       INT     NOT   NULL, "
                         "TSTATE          BOOL    NOT   NULL, "
                         "RSTATE          BOOL    NOT   NULL);", 
                         cdm_journal_table_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_error ("Fail to create crash table. SQL error %s", query_error);
    }

  return journal;
}

CdmJournal *
cdm_journal_ref (CdmJournal *journal)
{
  g_assert (journal);
  g_ref_count_inc (&journal->rc);
  return journal;
}

void
cdm_journal_unref (CdmJournal *journal)
{
  g_assert (journal);

  if (g_ref_count_dec (&journal->rc) == TRUE)
    {
      g_free (journal);
    }
}

guint64 
cdm_journal_add_crash (CdmJournal *journal, 
                       const gchar *proc_name, 
                       const gchar *crash_id, 
                       const gchar *vector_id, 
                       const gchar *context_id, 
                       const gchar *file_path,
                       gint64 pid, 
                       gint64 sig, 
                       guint64 tstamp, 
                       GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  guint64 id;
  JournalQueryData data = {
      .type = QUERY_ADD_ENTRY, 
      .response = NULL
  };

  g_assert (journal);

  if (!proc_name || !crash_id || !vector_id || !context_id || !file_path)
    {
      g_set_error (error, g_quark_from_static_string ("JournalAddCrash"), 1, "Invalid arguments");
      return (0);
    }

  id = cdm_utils_jenkins_hash (file_path);

  sql = g_strdup_printf ("INSERT INTO %s                                                                       "
                         "(ID,PROCNAME,CRASHID,VECTORID,CONTEXTID,FILEPATH,PID,SIGNAL,TIMESTAMP,TSTATE,RSTATE) "
                         "VALUES(                                                                              "
                         "%lu, '%s', '%s', '%s', '%s', '%s', %ld, %ld, %lu, %d, %d);", cdm_journal_table_name, 
                         id, proc_name, crash_id, vector_id, context_id, file_path, pid, sig, tstamp, 0, 0);
  
  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalAddCrash"), 1, "SQL query error");
      g_warning ("Fail to add new entry. SQL error %s", query_error);
      sqlite3_free (query_error);
      return (0);
    }

  return (id);
}
