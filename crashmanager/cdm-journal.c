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
  QUERY_ADD_ENTRY,
  QUERY_GET_ENTRY,
  QUERY_SET_TRANSFER,
  QUERY_SET_REMOVED,
  QUERY_GET_VICTIM,
  QUERY_GET_DATASIZE,
  QUERY_GET_ENTRY_COUNT
} JournalQueryType;

typedef struct _JournalQueryData {
  JournalQueryType type;
  gpointer response;
} JournalQueryData;

const gchar *cdm_journal_table_name = "CrashTable";

static int sqlite_callback (void *data, int argc, char **argv, char **colname);

static int
sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  JournalQueryData *querydata = (JournalQueryData *)data;

  switch (querydata->type)
    {
    case QUERY_CREATE:
      break;

    case QUERY_ADD_ENTRY:
      break;

    case QUERY_GET_ENTRY:
      *((gboolean *)(querydata->response)) = TRUE;
      break;

    case QUERY_SET_TRANSFER:
      break;

    case QUERY_SET_REMOVED:
      break;

    case QUERY_GET_VICTIM:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "FILEPATH") == 0)
            {
              querydata->response = (gpointer)g_strdup (argv[i]);
            }
        }
      break;

    case QUERY_GET_DATASIZE:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "FILESIZE") == 0)
            {
              *((gssize *)(querydata->response)) += g_ascii_strtoll (argv[i], NULL, 10);
            }
        }
      break;

    case QUERY_GET_ENTRY_COUNT:
      *((gssize *)(querydata->response)) += 1;
      break;

    default:
      break;
    }

  return(0);
}

CdmJournal *
cdm_journal_new (CdmOptions *options)
{
  CdmJournal *journal = NULL;
  g_autofree gchar *sql = NULL;
  g_autofree gchar *opt_dbpath = NULL;
  g_autofree gchar *opt_user = NULL;
  g_autofree gchar *opt_group = NULL;
  gchar *query_error = NULL;
  JournalQueryData data = {
    .type = QUERY_CREATE,
    .response = NULL
  };

  journal = g_new0 (CdmJournal, 1);

  g_assert (journal);

  g_ref_count_init (&journal->rc);

  opt_dbpath = cdm_options_string_for (options, KEY_DATABASE_FILE);
  opt_user = cdm_options_string_for (options, KEY_USER_NAME);
  opt_group = cdm_options_string_for (options, KEY_GROUP_NAME);

  if (sqlite3_open (opt_dbpath, &journal->database))
    {
      g_error ("Cannot open journal database at path %s", opt_dbpath);
    }

  sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s       "
                         "(ID INT PRIMARY KEY     NOT   NULL, "
                         "PROCNAME        TEXT    NOT   NULL, "
                         "CRASHID         TEXT    NOT   NULL, "
                         "VECTORID        TEXT    NOT   NULL, "
                         "CONTEXTID       TEXT    NOT   NULL, "
                         "FILEPATH        TEXT    NOT   NULL, "
                         "FILESIZE        INT     NOT   NULL, "
                         "PID             INT     NOT   NULL, "
                         "SIGNAL          INT     NOT   NULL, "
                         "TIMESTAMP       INT     NOT   NULL, "
                         "OSVERSION       TEXT    NOT   NULL, "
                         "TSTATE          BOOL    NOT   NULL, "
                         "RSTATE          BOOL    NOT   NULL);",
                         cdm_journal_table_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_error ("Fail to create crash table. SQL error %s", query_error);
    }

  if (cdm_utils_chown (opt_dbpath, opt_user, opt_group) == CDM_STATUS_ERROR)
    {
      g_warning ("Failed to set user and group owner for database %s", opt_dbpath);
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
  gint64 file_size;
  guint64 id;
  JournalQueryData data = {
    .type = QUERY_ADD_ENTRY,
    .response = NULL
  };

  g_assert (journal);

  if (!proc_name || !crash_id || !vector_id || !context_id || !file_path)
    {
      g_set_error (error, g_quark_from_static_string ("JournalAddCrash"), 1, "Invalid arguments");
      return(0);
    }

  file_size = cdm_utils_get_filesize (file_path);
  if (file_size == -1)
    {
      g_set_error (error, g_quark_from_static_string ("JournalAddCrash"), 1, "Cannot stat file for size");
      return(0);
    }

  id = cdm_utils_jenkins_hash (file_path);

  sql = g_strdup_printf ("INSERT INTO %s                                                                                 "
                         "(ID,PROCNAME,CRASHID,VECTORID,CONTEXTID,FILEPATH,FILESIZE,"
                         "PID,SIGNAL,TIMESTAMP,OSVERSION,TSTATE,RSTATE) "
                         "VALUES(                                                                                        "
                         "%lu, '%s', '%s', '%s', '%s', '%s', %ld, %ld, %ld, %lu, '%s', %d, %d);",
                         cdm_journal_table_name, id, proc_name, crash_id, vector_id, context_id,
                         file_path, file_size, pid, sig, tstamp, cdm_utils_get_osversion (), 0, 0);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalAddCrash"), 1, "SQL query error");
      g_warning ("Fail to add new entry. SQL error %s", query_error);
      sqlite3_free (query_error);

      return(0);
    }

  return(id);
}

gboolean
cdm_journal_archive_exist (CdmJournal *journal,
                           const gchar *file_path,
                           GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  gboolean entry_exist = FALSE;
  guint64 id;
  JournalQueryData data = {
    .type = QUERY_GET_ENTRY,
    .response = NULL
  };

  g_assert (journal);

  id = cdm_utils_jenkins_hash (file_path);
  data.response = (gpointer) & entry_exist;

  sql = g_strdup_printf ("SELECT ID FROM %s WHERE ID IS %lu",
                         cdm_journal_table_name, id);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetEntry"), 1, "SQL query error");
      g_warning ("Fail to get victim. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return *(gboolean *)data.response;
}

void
cdm_journal_set_transfer (CdmJournal *journal,
                          const gchar *file_path,
                          gboolean complete,
                          GError **error)
{
  g_assert (journal);

  if (!file_path)
    {
      g_set_error (error, g_quark_from_static_string ("JournalSetTransfer"), 1, "Invalid arguments");
    }
  else
    {
      g_autofree gchar *sql = NULL;
      gchar *query_error = NULL;
      JournalQueryData data = {
        .type = QUERY_SET_TRANSFER,
        .response = NULL
      };

      guint64 id = cdm_utils_jenkins_hash (file_path);

      sql = g_strdup_printf ("UPDATE %s SET TSTATE = %d WHERE ID IS %lu",
                             cdm_journal_table_name, complete, id);

      if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
        {
          g_set_error (error, g_quark_from_static_string ("JournalSetTransfer"), 1, "SQL query error");
          g_warning ("Fail to set transfer state. SQL error %s", query_error);
          sqlite3_free (query_error);
        }
    }
}

void
cdm_journal_set_removed (CdmJournal *journal,
                         const gchar *file_path,
                         gboolean complete,
                         GError **error)
{
  g_assert (journal);

  if (!file_path)
    {
      g_set_error (error, g_quark_from_static_string ("JournalSetRemoved"), 1, "Invalid arguments");
    }
  else
    {
      g_autofree gchar *sql = NULL;
      gchar *query_error = NULL;
      JournalQueryData data = {
        .type = QUERY_SET_REMOVED,
        .response = NULL
      };

      guint64 id = cdm_utils_jenkins_hash (file_path);

      sql = g_strdup_printf ("UPDATE %s SET RSTATE = %d WHERE ID IS %lu",
                             cdm_journal_table_name, complete, id);

      if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
        {
          g_set_error (error, g_quark_from_static_string ("JournalSetRemoved"), 1, "SQL query error");
          g_warning ("Fail to set removed state. SQL error %s", query_error);
          sqlite3_free (query_error);
        }
    }
}

gchar *
cdm_journal_get_victim (CdmJournal *journal,
                        GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  JournalQueryData data = {
    .type = QUERY_GET_VICTIM,
    .response = NULL
  };

  g_assert (journal);

  sql = g_strdup_printf ("SELECT FILEPATH FROM %s WHERE RSTATE IS 0 AND TSTATE IS 1 ORDER BY TIMESTAMP LIMIT 1",
                         cdm_journal_table_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetVictim"), 1, "SQL query error");
      g_warning ("Fail to get victim. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return (gchar *)data.response;
}

gssize
cdm_journal_get_data_size (CdmJournal *journal,
                           GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  gssize crash_dir_size = 0;
  JournalQueryData data = {
    .type = QUERY_GET_DATASIZE,
    .response = NULL
  };

  g_assert (journal);

  data.response = (gpointer) & crash_dir_size;

  sql = g_strdup_printf ("SELECT FILESIZE FROM %s WHERE RSTATE IS 0 AND TSTATE IS 1",
                         cdm_journal_table_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetDatasize"), 1, "SQL query error");
      g_warning ("Fail to get data size. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return crash_dir_size;
}

gssize
cdm_journal_get_entry_count (CdmJournal *journal,
                             GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  gssize crash_entries = 0;
  JournalQueryData data = {
    .type = QUERY_GET_ENTRY_COUNT,
    .response = NULL
  };

  g_assert (journal);

  data.response = (gpointer) & crash_entries;

  sql = g_strdup_printf ("SELECT FILEPATH FROM %s WHERE RSTATE IS 0 AND TSTATE IS 1",
                         cdm_journal_table_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetDatasize"), 1, "SQL query error");
      g_warning ("Fail to get entry count. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return crash_entries;
}

