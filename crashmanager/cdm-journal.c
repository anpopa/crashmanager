/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019-2020 Alin Popa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \file cdm-journal.c
 */

#include "cdm-journal.h"
#include "cdm-utils.h"

#define USEC2SEC(x) (x / 1000000)
#define SEC2USEC(x) (x * 1000000)

/**
 * @enum Journal query type
 */
typedef enum _JournalQueryType
{
  QUERY_CREATE,
  QUERY_ADD_ENTRY,
  QUERY_GET_ENTRY,
  QUERY_SET_TRANSFER,
  QUERY_SET_REMOVED,
  QUERY_GET_VICTIM,
  QUERY_GET_UNTRANSFERRED,
  QUERY_GET_DATASIZE,
  QUERY_GET_ENTRY_COUNT
} JournalQueryType;

/**
 * @enum Journal query data object
 */
typedef struct _JournalQueryData
{
  JournalQueryType type;
  gpointer response;
} JournalQueryData;

const gchar *cdm_journal_table_name = "CrashTable";

/**
 * @brief SQlite3 callback
 */
static int sqlite_callback (void *data, int argc, char **argv, char **colname);

/**
 * @brief GSource callback function
 */
static gboolean source_timer_callback (gpointer data);

/**
 * @brief GSource destroy notification callback function
 */
static void source_destroy_notify (gpointer data);

static gboolean
source_timer_callback (gpointer data)
{
  CdmJournal *journal = (CdmJournal *)data;
  gint64 current_time = 0;
  GList *l = NULL;

  g_assert (journal);

  l = journal->elogs;
  current_time = g_get_monotonic_time ();

  while (l != NULL)
    {
      GList *next = l->next;

      if (((CdmJournalEpilog *)l->data)->tstamp + SEC2USEC (10) > current_time)
        {
          g_debug ("Journal remove epilog for pid %ld", ((CdmJournalEpilog *)l->data)->pid);
          g_free (l->data);
          journal->elogs = g_list_delete_link (journal->elogs, l);
        }
      l = next;
    }

  return TRUE;
}

static void
source_destroy_notify (gpointer data)
{
  CDM_UNUSED (data);
  g_info ("Journal epilog cleanup event disabled");
}

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
    /* falltrough */
    case QUERY_GET_UNTRANSFERRED:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "FILEPATH") == 0)
            querydata->response = (gpointer)g_strdup (argv[i]);
        }
      break;

    case QUERY_GET_DATASIZE:
      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "FILESIZE") == 0)
            *((gssize *)(querydata->response)) += g_ascii_strtoll (argv[i], NULL, 10);
        }
      break;

    case QUERY_GET_ENTRY_COUNT:
      *((gssize *)(querydata->response)) += 1;
      break;

    default:
      break;
    }

  return (0);
}

CdmJournal *
cdm_journal_new (CdmOptions *options, GError **error)
{
  CdmJournal *journal = NULL;
  g_autofree gchar *sql = NULL;
  g_autofree gchar *opt_dbpath = NULL;
  g_autofree gchar *opt_user = NULL;
  g_autofree gchar *opt_group = NULL;
  gchar *query_error = NULL;
  JournalQueryData data = { .type = QUERY_CREATE, .response = NULL };

  journal = g_new0 (CdmJournal, 1);

  g_assert (journal);

  g_ref_count_init (&journal->rc);

  opt_dbpath = cdm_options_string_for (options, KEY_DATABASE_FILE);
  opt_user = cdm_options_string_for (options, KEY_USER_NAME);
  opt_group = cdm_options_string_for (options, KEY_GROUP_NAME);

  if (sqlite3_open (opt_dbpath, &journal->database))
    {
      g_warning ("Cannot open journal database at path %s", opt_dbpath);
      g_set_error (error, g_quark_from_static_string ("JournalNew"), 1, "Database open failed");
    }
  else
    {
      sql = g_strdup_printf ("CREATE TABLE IF NOT EXISTS %s       "
                             "(ID INT PRIMARY KEY     NOT   NULL, "
                             "PROCNAME        TEXT    NOT   NULL, "
                             "CRASHID         TEXT    NOT   NULL, "
                             "VECTORID        TEXT    NOT   NULL, "
                             "CONTEXTID       TEXT    NOT   NULL, "
                             "CONTEXTNAME     TEXT    NOT   NULL, "
                             "LIFECYCLESTATE  TEXT    NOT   NULL, "
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
          g_warning ("Fail to create crash table. SQL error %s", query_error);
          g_set_error (error, g_quark_from_static_string ("JournalNew"), 1,
                       "Create crash table fail");
        }

      if (cdm_utils_chown (opt_dbpath, opt_user, opt_group) == CDM_STATUS_ERROR)
        g_warning ("Failed to set user and group owner for database %s", opt_dbpath);
    }

  /* prepare epilog cleanup source */
  journal->source = g_timeout_source_new_seconds (10);
  g_source_ref (journal->source);
  g_source_set_callback (journal->source, G_SOURCE_FUNC (source_timer_callback), journal,
                         source_destroy_notify);
  g_source_attach (journal->source, NULL);

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
      if (journal->source != NULL)
        g_source_unref (journal->source);

      g_free (journal);
    }
}

void
cdm_journal_epilog_add (CdmJournal *journal, CdmJournalEpilog *elog)
{
  g_assert (elog);
  g_assert (journal);

  elog->tstamp = g_get_monotonic_time ();
  journal->elogs = g_list_append (journal->elogs, elog);
}

CdmStatus
cdm_journal_epilog_rem (CdmJournal *journal, int64_t pid)
{
  CdmStatus status = CDM_STATUS_ERROR;
  GList *l = NULL;

  g_assert (journal);

  l = journal->elogs;

  while (l != NULL)
    {
      GList *next = l->next;

      if (((CdmJournalEpilog *)l->data)->pid == pid)
        {
          status = CDM_STATUS_OK;
          g_free (l->data);
          journal->elogs = g_list_delete_link (journal->elogs, l);
        }
      l = next;
    }

  return status;
}

CdmJournalEpilog *
cdm_journal_epilog_get (CdmJournal *journal, int64_t pid)
{
  CdmJournalEpilog *elog = NULL;

  GList *l = NULL;

  g_assert (journal);

  for (l = journal->elogs; l != NULL; l = l->next)
    {
      if (((CdmJournalEpilog *)l->data)->pid == pid)
        elog = (CdmJournalEpilog *)l->data;
    }

  return elog;
}

guint64
cdm_journal_add_crash (CdmJournal *journal, const gchar *proc_name, const gchar *crash_id,
                       const gchar *vector_id, const gchar *context_id, const gchar *context_name,
                       const gchar *lifecycle_state, const gchar *file_path, gint64 pid, gint64 sig,
                       guint64 tstamp, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  gint64 file_size;
  guint64 id;
  JournalQueryData data = { .type = QUERY_ADD_ENTRY, .response = NULL };

  g_assert (journal);

  if (!proc_name || !crash_id || !vector_id || !context_id || !file_path)
    {
      g_set_error (error, g_quark_from_static_string ("JournalAddCrash"), 1, "Invalid arguments");
      return 0;
    }

  file_size = cdm_utils_get_filesize (file_path);
  if (file_size == -1)
    {
      g_set_error (error, g_quark_from_static_string ("JournalAddCrash"), 1,
                   "Cannot stat file for size");
      return 0;
    }

  id = cdm_utils_jenkins_hash (file_path);

  sql = g_strdup_printf ("INSERT INTO %s "
                         "(ID,PROCNAME,CRASHID,VECTORID,CONTEXTID,CONTEXTNAME,LIFECYCLESTATE,"
                         "FILEPATH,FILESIZE,PID,SIGNAL,TIMESTAMP,OSVERSION,TSTATE,RSTATE) VALUES("
                         "%lu, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %ld, %ld, %ld, %lu, '%s',"
                         "%d, %d);",
                         cdm_journal_table_name, id, proc_name, crash_id, vector_id, context_id,
                         context_name, lifecycle_state, file_path, file_size, pid, sig, tstamp,
                         cdm_utils_get_osversion (), 0, 0);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalAddCrash"), 1, "SQL query error");
      g_warning ("Fail to add new entry. SQL error %s", query_error);
      sqlite3_free (query_error);

      return 0;
    }

  return id;
}

gboolean
cdm_journal_archive_exist (CdmJournal *journal, const gchar *file_path, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  gboolean entry_exist = FALSE;
  guint64 id;
  JournalQueryData data = { .type = QUERY_GET_ENTRY, .response = NULL };

  g_assert (journal);

  id = cdm_utils_jenkins_hash (file_path);
  data.response = (gpointer)&entry_exist;

  sql = g_strdup_printf ("SELECT ID FROM %s WHERE ID IS %lu", cdm_journal_table_name, id);
  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetEntry"), 1, "SQL query error");
      g_warning ("Fail to get victim. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return *(gboolean *)data.response;
}

void
cdm_journal_set_transfer (CdmJournal *journal, const gchar *file_path, gboolean complete,
                          GError **error)
{
  g_assert (journal);

  if (!file_path)
    {
      g_set_error (error, g_quark_from_static_string ("JournalSetTransfer"), 1,
                   "Invalid arguments");
    }
  else
    {
      g_autofree gchar *sql = NULL;
      gchar *query_error = NULL;
      JournalQueryData data = { .type = QUERY_SET_TRANSFER, .response = NULL };

      guint64 id = cdm_utils_jenkins_hash (file_path);

      sql = g_strdup_printf ("UPDATE %s SET TSTATE = %d WHERE ID IS %lu", cdm_journal_table_name,
                             complete, id);

      if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
        {
          g_set_error (error, g_quark_from_static_string ("JournalSetTransfer"), 1,
                       "SQL query error");
          g_warning ("Fail to set transfer state. SQL error %s", query_error);
          sqlite3_free (query_error);
        }
    }
}

void
cdm_journal_set_removed (CdmJournal *journal, const gchar *file_path, gboolean complete,
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
      JournalQueryData data = { .type = QUERY_SET_REMOVED, .response = NULL };

      guint64 id = cdm_utils_jenkins_hash (file_path);

      sql = g_strdup_printf ("UPDATE %s SET RSTATE = %d WHERE ID IS %lu", cdm_journal_table_name,
                             complete, id);

      if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
        {
          g_set_error (error, g_quark_from_static_string ("JournalSetRemoved"), 1,
                       "SQL query error");
          g_warning ("Fail to set removed state. SQL error %s", query_error);
          sqlite3_free (query_error);
        }
    }
}

gchar *
cdm_journal_get_victim (CdmJournal *journal, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  JournalQueryData data = { .type = QUERY_GET_VICTIM, .response = NULL };

  g_assert (journal);

  sql = g_strdup_printf ("SELECT FILEPATH FROM %s "
                         "WHERE RSTATE IS 0 AND TSTATE IS 1 ORDER BY TIMESTAMP LIMIT 1",
                         cdm_journal_table_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetVictim"), 1, "SQL query error");
      g_warning ("Fail to get victim. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return (gchar *)data.response;
}

gchar *
cdm_journal_get_untransferred (CdmJournal *journal, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  JournalQueryData data = { .type = QUERY_GET_UNTRANSFERRED, .response = NULL };

  g_assert (journal);

  sql = g_strdup_printf ("SELECT FILEPATH FROM %s "
                         "WHERE RSTATE IS 0 AND TSTATE IS 0 ORDER BY TIMESTAMP LIMIT 1",
                         cdm_journal_table_name);

  if (sqlite3_exec (journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK)
    {
      g_set_error (error, g_quark_from_static_string ("JournalGetUntrasnferred"), 1,
                   "SQL query error");
      g_warning ("Fail to get untransfered. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return (gchar *)data.response;
}

gssize
cdm_journal_get_data_size (CdmJournal *journal, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  gssize crash_dir_size = 0;
  JournalQueryData data = { .type = QUERY_GET_DATASIZE, .response = NULL };

  g_assert (journal);

  data.response = (gpointer)&crash_dir_size;

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
cdm_journal_get_entry_count (CdmJournal *journal, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  gssize crash_entries = 0;
  JournalQueryData data = { .type = QUERY_GET_ENTRY_COUNT, .response = NULL };

  g_assert (journal);

  data.response = (gpointer)&crash_entries;

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
