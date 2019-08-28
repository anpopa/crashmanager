/* cdm-journal.h
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

#pragma once

#include "cdm-types.h"
#include "cdm-options.h"

#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

/**
 * @struct CdmJournal
 * @brief The CdmJournal opaque data structure
 */
typedef struct _CdmJournal {
  sqlite3 *database; /**< The sqlite3 database object */
  grefcount rc;     /**< Reference counter variable  */
} CdmJournal;

/**
 * @brief Create a new journal object
 * @param options A pointer to the CdmOptions object created by the main
 * application
 * @param error The GError object or NULL
 * @return On success return a new CdmJournal object otherwise return NULL
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
 * @brief Check if an entry for file_path exist in database
 *
 * @param journal The journal object
 * @param file_path The archive file path
 * @param error The GError object or NULL
 *
 * @return TRUE if entry exist
 */
gboolean cdm_journal_archive_exist (CdmJournal *journal, const gchar *file_path, GError **error);

/**
 * @brief Add a new crash entry with default state into the journal
 *
 * @param journal Pointer to the journal object
 * @param proc_name Process name
 * @param crash_id Process crash id
 * @param vector_id Process vector id
 * @param context_id Process context id
 * @param file_path Process file path
 * @param pid Process id
 * @param sig Process crash signal id
 * @param tstamp Process crash timestamp
 * @param error Optional GError object reference to set on error
 *
 * @return Return the new journal entry ID. If error is not NULL and an error
 * occured the error is set and return value is 0
 */
guint64 cdm_journal_add_crash (CdmJournal *journal,
                               const gchar *proc_name,
                               const gchar *crash_id,
                               const gchar *vector_id,
                               const gchar *context_id,
                               const gchar *context_name,
                               const gchar *lifecycle_state,
                               const gchar *file_path,
                               gint64 pid,
                               gint64 sig,
                               guint64 tstamp,
                               GError **error);
/**
 * @brief Set transfer state for an entry
 *
 * @param journal The journal object
 * @param file_path The archive file path
 * @param complete The transfer complete state
 * @param error The GError object or NULL
 */
void cdm_journal_set_transfer (CdmJournal *journal, const gchar *file_path, gboolean complete, GError **error);

/**
 * @brief Set archive removed state for an entry
 *
 * @param journal The journal object
 * @param file_path The archive file path
 * @param complete The transfer complete state
 * @param error The GError object or NULL
 */
void cdm_journal_set_removed (CdmJournal *journal, const gchar *file_path, gboolean removed, GError **error);

/**
 * @brief Get total file size for unremoved transfered entries
 *
 * @param journal The journal object
 * @param error The GError object or NULL
 *
 * @return Return total file sizes from database. -1 on error
 */
gssize cdm_journal_get_data_size (CdmJournal *journal, GError **error);

/**
 * @brief Get number of unremoved transfered entries
 *
 * @param journal The journal object
 * @param error The GError object or NULL
 *
 * @return Return total number of entries. -1 on error
 */
gssize cdm_journal_get_entry_count (CdmJournal *journal, GError **error);

/**
 * @brief Get next untransferred file
 *
 * @param journal The journal object
 * @param error The GError object or NULL
 *
 * @return If a file is found a pointer to untranferred's file path is returned (new
 * string to be released by the caller). If no vicitm available NULL is
 * returned. If an error occured the error is set and NULL is returned.
 */
gchar* cdm_journal_get_untransferred (CdmJournal *journal, GError **error);

/**
 * @brief Get next victim
 *
 * @param journal The journal object
 * @param error The GError object or NULL
 *
 * @return If a victim is found a pointer to victim's file path is returned (new
 * string to be released by the caller). If no vicitm available NULL is
 * returned. If an error occured the error is set and NULL is returned.
 */
gchar* cdm_journal_get_victim (CdmJournal *journal, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmJournal, cdm_journal_unref);

G_END_DECLS
