/* cdm-journal.h
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

#ifndef CDM_JOURNAL_H
#define CDM_JOURNAL_H

#include "cdm-types.h"
#include "cdm-options.h"

#include <glib.h>
#include <sqlite3.h>

/**
 * @struct CdmJournal
 * @brief The CdmJournal opaque data structure
 */
typedef struct _CdmJournal {
  sqlite3 *database;
  grefcount rc;     /**< Reference counter variable  */
} CdmJournal;

/**
 * @brief Create a new journal object
 * @return On success return a new CdmJournal object otherwise return NULL
 */
CdmJournal *cdm_journal_new (CdmOptions *options);

/**
 * @brief Aquire journal object
 * @return On success return a new CdmJournal object otherwise return NULL
 */
CdmJournal *cdm_journal_ref (CdmJournal *journal);

/**
 * @brief Release an journal object
 * @param c Pointer to the journal object
 */
void cdm_journal_unref (CdmJournal *journal);

/**
 * @brief Set journal entry as type new data
 * @return Return the new journal entry ID. If error is not NULL and an error
 * occured the error is set and return value is 0
 */
guint64 cdm_journal_add_crash (CdmJournal *journal,
                               const gchar *proc_name,
                               const gchar *crash_id,
                               const gchar *vector_id,
                               const gchar *context_id,
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

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmJournal, cdm_journal_unref);

#endif /* CDM_JOURNAL_H */
