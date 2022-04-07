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
 * \file cdm-journal.h
 */

#pragma once

#include "cdm-message.h"
#include "cdm-options.h"
#include "cdm-types.h"

#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

#define CDM_JOURNAL_EPILOG_MAX_BT                                                                  \
  (CDM_MESSAGE_EPILOG_FRAME_MAX_LEN * CDM_MESSAGE_EPILOG_FRAME_MAX_CNT)

/**
 * @brief The CdmJournalEpilog data structure
 */
typedef struct _CdmJournalEpilog
{
  gint64 tstamp; /**< Epilog creation timestamp */
  int64_t pid;   /**< Process ID */
  char backtrace[CDM_JOURNAL_EPILOG_MAX_BT];
} CdmJournalEpilog;

/**
 * @brief The CdmJournal opaque data structure
 */
typedef struct _CdmJournal
{
  GSource *source;   /**< Event loop source */
  sqlite3 *database; /**< The sqlite3 database object */
  grefcount rc;      /**< Reference counter variable  */
  GList *elogs;      /**< Current epilog list */
} CdmJournal;

/**
 * @brief Create a new journal object
 * @param options A pointer to the CdmOptions object created by the main application
 * @param error The GError object or NULL
 * @return On success return a new CdmJournal object
 */
CdmJournal *cdm_journal_new (CdmOptions *options, GError **error);

/**
 * @brief Aquire journal object
 * @param journal Pointer to the journal object
 * @return The referenced journal object
 */
CdmJournal *cdm_journal_ref (CdmJournal *journal);

/**
 * @brief Release an journal object
 * @param journal Pointer to the journal object
 */
void cdm_journal_unref (CdmJournal *journal);

/**
 * @brief Add new epilog entry. The object will be freed when the epilog entry will be removed
 * @param journal The journal object
 * @param elog Reference to epilog object
 */
void cdm_journal_epilog_add (CdmJournal *journal, CdmJournalEpilog *elog);

/**
 * @brief Remove epilog by pid
 * @param journal The journal object
 * @param pid Reference to epilog object
 * @return on success return CDM_STATUS_OK
 */
CdmStatus cdm_journal_epilog_rem (CdmJournal *journal, int64_t pid);

/**
 * @brief Get epilog by pid
 *
 * @param journal The journal object
 * @param pid Reference to epilog object
 *
 * @return Return a reference to epilog entry or NULL if don't exist
 */
CdmJournalEpilog *cdm_journal_epilog_get (CdmJournal *journal, int64_t pid);

/**
 * @brief Check if an entry for file_path exist in database
 * @param journal The journal object
 * @param file_path The archive file path
 * @param error The GError object or NULL
 * @return TRUE if entry exist
 */
gboolean cdm_journal_archive_exist (CdmJournal *journal, const gchar *file_path, GError **error);

/**
 * @brief Add a new crash entry with default state into the journal
 * @param journal Pointer to the journal object
 * @param proc_name Process name
 * @param crash_id Process crash id
 * @param vector_id Process vector id
 * @param context_id Process context id
 * @param context_name Process context name
 * @param lifecycle_state System lifecycle state
 * @param file_path Process file path
 * @param pid Process id
 * @param sig Process crash signal id
 * @param tstamp Process crash timestamp
 * @param error Optional GError object reference to set on error
 * @return Return the new journal entry ID. If error is not NULL and an error
 * occured the error is set and return value is 0
 */
guint64 cdm_journal_add_crash (CdmJournal *journal, const gchar *proc_name, const gchar *crash_id,
                               const gchar *vector_id, const gchar *context_id,
                               const gchar *context_name, const gchar *lifecycle_state,
                               const gchar *file_path, gint64 pid, gint64 sig, guint64 tstamp,
                               GError **error);
/**
 * @brief Set transfer state for an entry
 * @param journal The journal object
 * @param file_path The archive file path
 * @param complete The transfer complete state
 * @param error The GError object or NULL
 */
void cdm_journal_set_transfer (CdmJournal *journal, const gchar *file_path, gboolean complete,
                               GError **error);

/**
 * @brief Set archive removed state for an entry
 * @param journal The journal object
 * @param file_path The archive file path
 * @param complete The transfer complete state
 * @param error The GError object or NULL
 */
void cdm_journal_set_removed (CdmJournal *journal, const gchar *file_path, gboolean removed,
                              GError **error);

/**
 * @brief Get total file size for unremoved transfered entries
 * @param journal The journal object
 * @param error The GError object or NULL
 * @return Return total file sizes from database. -1 on error
 */
gssize cdm_journal_get_data_size (CdmJournal *journal, GError **error);

/**
 * @brief Get number of unremoved transfered entries
 * @param journal The journal object
 * @param error The GError object or NULL
 * @return Return total number of entries. -1 on error
 */
gssize cdm_journal_get_entry_count (CdmJournal *journal, GError **error);

/**
 * @brief Get next untransferred file
 * @param journal The journal object
 * @param error The GError object or NULL
 * @return If a file is found a pointer to untranferred's file path is returned (new
 * string to be released by the caller). If no vicitm available NULL is
 * returned. If an error occured the error is set and NULL is returned.
 */
gchar *cdm_journal_get_untransferred (CdmJournal *journal, GError **error);

/**
 * @brief Get next victim
 * @param journal The journal object
 * @param error The GError object or NULL
 * @return If a victim is found a pointer to victim's file path is returned (new
 * string to be released by the caller). If no vicitm available NULL is
 * returned. If an error occured the error is set and NULL is returned.
 */
gchar *cdm_journal_get_victim (CdmJournal *journal, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmJournal, cdm_journal_unref);

G_END_DECLS
