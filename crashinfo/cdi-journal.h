/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019 Alin Popa
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
