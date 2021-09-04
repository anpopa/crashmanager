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
 * \file cdi-journal.c
 */

#include "cdi-journal.h"
#include "cdm-utils.h"

#include <glib/gprintf.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

typedef enum _JournalQueryType { QUERY_LIST_ENTRIES } JournalQueryType;

typedef struct _JournalQueryData {
    JournalQueryType type;
    gpointer response;
} JournalQueryData;

const gchar *cdi_journal_table_name = "CrashTable";

static int sqlite_callback(void *data, int argc, char **argv, char **colname);

static int sqlite_callback(void *data, int argc, char **argv, char **colname)
{
    JournalQueryData *querydata = (JournalQueryData *) data;

    switch (querydata->type) {
    case QUERY_LIST_ENTRIES: {
        g_autoptr(GDateTime) dtime = NULL;
        const gchar *proc_name = NULL;
        const gchar *context_name = NULL;
        const gchar *crash_id = NULL;
        const gchar *vector_id = NULL;
        const gchar *timestamp = NULL;
        const gchar *pid = NULL;
        const gchar *tstate = NULL;
        const gchar *rstate = NULL;
        const gchar *file_name = NULL;

        for (gint i = 0; i < argc; i++) {
            if (g_strcmp0(colname[i], "PROCNAME") == 0)
                proc_name = argv[i];
            else if (g_strcmp0(colname[i], "CRASHID") == 0)
                crash_id = argv[i];
            else if (g_strcmp0(colname[i], "VECTORID") == 0)
                vector_id = argv[i];
            else if (g_strcmp0(colname[i], "CONTEXTNAME") == 0)
                context_name = argv[i];
            else if (g_strcmp0(colname[i], "TIMESTAMP") == 0)
                timestamp = argv[i];
            else if (g_strcmp0(colname[i], "FILEPATH") == 0)
                file_name = g_path_get_basename(argv[i]);
            else if (g_strcmp0(colname[i], "PID") == 0)
                pid = argv[i];
            else if (g_strcmp0(colname[i], "TSTATE") == 0)
                tstate = argv[i];
            else if (g_strcmp0(colname[i], "RSTATE") == 0)
                rstate = argv[i];
        }

        dtime = g_date_time_new_from_unix_utc(g_ascii_strtoll(timestamp, NULL, 10));

        g_print("%-4u %-20s %20s %16s %16s %16s %6s %3s %3s  %s\n",
                *(guint *) (querydata->response),
                proc_name,
                (dtime != NULL) ? g_date_time_format(dtime, "%H:%M:%S %Y-%m-%d") : timestamp,
                crash_id,
                vector_id,
                context_name,
                pid,
                tstate,
                rstate,
                file_name);

        *((guint *) (querydata->response)) += 1;
    } break;

    default:
        break;
    }

    return (0);
}

CdiJournal *cdi_journal_new(CdmOptions *options, GError **error)
{
    CdiJournal *journal = g_new0(CdiJournal, 1);
    g_autofree gchar *opt_dbpath = NULL;

    g_assert(journal);

    g_ref_count_init(&journal->rc);
    opt_dbpath = cdm_options_string_for(options, KEY_DATABASE_FILE);

    if (sqlite3_open_v2(opt_dbpath, &journal->database, SQLITE_OPEN_READONLY, NULL)) {
        g_warning("Cannot open journal database at path %s", opt_dbpath);
        g_set_error(error, g_quark_from_static_string("JournalNew"), 1, "Database open failed");
    }

    return journal;
}

CdiJournal *cdi_journal_ref(CdiJournal *journal)
{
    g_assert(journal);
    g_ref_count_inc(&journal->rc);
    return journal;
}

void cdi_journal_unref(CdiJournal *journal)
{
    g_assert(journal);

    if (g_ref_count_dec(&journal->rc) == TRUE)
        g_free(journal);
}

void cdi_journal_list_entries(CdiJournal *journal, GError **error)
{
    g_autofree gchar *sql = NULL;
    gchar *query_error = NULL;
    guint index = 1;
    JournalQueryData data = {.type = QUERY_LIST_ENTRIES, .response = NULL};

    g_assert(journal);

    data.response = &index;
    sql = g_strdup_printf("SELECT * FROM %s ;", cdi_journal_table_name);

    g_print("%-4s %-20s %20s %16s %16s %16s %6s %3s %3s  %s\n",
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

    if (sqlite3_exec(journal->database, sql, sqlite_callback, &data, &query_error) != SQLITE_OK) {
        g_set_error(error, g_quark_from_static_string("JournalListEntries"), 1, "SQL query error");
        g_warning("Fail to get entry count. SQL error %s", query_error);
        sqlite3_free(query_error);
    }
}
