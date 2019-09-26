/*
 * SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2019, BMW Car IT GmbH
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * For further information see http://www.genivi.org/.
 *
 * \author Alin Popa <alin.popa@bmw.de>
 * \file cdi-journal.h
 */

#pragma once

#include "cdm-types.h"
#include "cdm-options.h"

#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

/**
 * @brief The CdiJournal opaque data structure
 */
typedef struct _CdiJournal {
  sqlite3 *database; /**< The sqlite3 database object */
  grefcount rc;      /**< Reference counter variable  */
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
 * @param journal The journal object
 * @param error The GError object or NULL
 */
void cdi_journal_list_entries (CdiJournal *journal, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdiJournal, cdi_journal_unref);

G_END_DECLS
