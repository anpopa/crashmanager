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
 * \file cdm-janitor.h
 */

#pragma once

#include "cdm-journal.h"
#include "cdm-options.h"
#include "cdm-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The CdmJanitor opaque data structure
 */
typedef struct _CdmJanitor
{
  GSource source;      /**< Event loop source */
  grefcount rc;        /**< Reference counter variable  */
  glong max_dir_size;  /**< Maximum allowed crash dir size */
  glong min_dir_size;  /**< Minimum space to preserve from quota */
  glong max_file_cnt;  /**< Maximum file count */
  CdmJournal *journal; /**< Own a reference to journal object */
} CdmJanitor;

/*
 * @brief Create a new janitor object
 * @param options A pointer to the CdmOptions object created by the main application
 * @param journal A pointer to the CdmJournal object created by the main application
 * @return On success return a new CdmJanitor object
 */
CdmJanitor *cdm_janitor_new (CdmOptions *options, CdmJournal *journal);

/**
 * @brief Aquire janitor object
 * @param janitor Pointer to the janitor object
 * @return The referenced janitor object
 */
CdmJanitor *cdm_janitor_ref (CdmJanitor *janitor);

/**
 * @brief Release janitor object
 * @param janitor Pointer to the janitor object
 */
void cdm_janitor_unref (CdmJanitor *janitor);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmJanitor, cdm_janitor_unref);

G_END_DECLS
