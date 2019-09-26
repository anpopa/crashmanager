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
 * \file cdm-janitor.h
 */

#pragma once

#include "cdm-types.h"
#include "cdm-options.h"
#include "cdm-journal.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The CdmJanitor opaque data structure
 */
typedef struct _CdmJanitor {
  GSource source;  /**< Event loop source */
  grefcount rc;    /**< Reference counter variable  */

  glong max_dir_size; /**< Maximum allowed crash dir size */
  glong min_dir_size; /**< Minimum space to preserve from quota */
  glong max_file_cnt; /**< Maximum file count */

  CdmJournal *journal; /**< Own a reference to journal object */
} CdmJanitor;

/*
 * @brief Create a new janitor object
 *
 * @param options A pointer to the CdmOptions object created by the main
 * application
 * @param journal A pointer to the CdmJournal object created by the main
 * application
 *
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
