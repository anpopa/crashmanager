/* cdi-journal.h
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
 * @struct CdiJournal
 * @brief The CdiJournal opaque data structure
 */
typedef struct _CdiJournal {
  sqlite3 *database; /**< The sqlite3 database object */
  grefcount rc;     /**< Reference counter variable  */
} CdiJournal;

/**
 * @brief Create a new journal object
 * @param options A pointer to the CdmOptions object created by the main
 * application
 * @param error The GError object or NULL
 * @return On success return a new CdiJournal object otherwise return NULL
 */
CdiJournal *cdi_journal_new (CdmOptions *options, GError **error);

/**
 * @brief Aquire journal object
 * @param journal Pointer to the journal object
 * @return The referenced journal object
 */
CdiJournal *cdi_journal_ref (CdiJournal *journal);

/**
 * @brief Release an journal object
 * @param journal Pointer to the journal object
 */
void cdi_journal_unref (CdiJournal *journal);

/**
 * @brief List database entries to stdout
 *
 * @param journal The journal object
 * @param error The GError object or NULL
 */
void cdi_journal_list_entries (CdiJournal *journal, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdiJournal, cdi_journal_unref);

G_END_DECLS
