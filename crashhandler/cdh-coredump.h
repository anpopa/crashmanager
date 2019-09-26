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
 * \file cdh-coredump.h
 */

#pragma once

#include "cdh-context.h"
#include "cdh-archive.h"
#if defined(WITH_CRASHMANAGER)
#include "cdh-manager.h"
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The coredump generation object
 */
typedef struct _CdhCoredump {
  CdhContext *context; /**< Context object owned */
  CdhArchive *archive; /**< Archive object owned */
#if defined(WITH_CRASHMANAGER)
  CdhManager *manager; /**< Manager object owned */
#endif
  grefcount rc;        /**< Reference counter variable  */
} CdhCoredump;

/**
 * @brief Create a new CdhCoredump object
 * @return A pointer to the new object
 */
CdhCoredump *cdh_coredump_new (CdhContext *context, CdhArchive *archive);

/**
 * @brief Aquire CdhCoredump object
 * @param cd Pointer to the CdhCoredump object
 * @return Pointer to the CdhCoredump object
 */
CdhCoredump *cdh_coredump_ref (CdhCoredump *cd);

/**
 * @brief Release a CdhCoredump object
 * @param cd Pointer to the cdh_context object
 */
void cdh_coredump_unref (CdhCoredump *cd);

#if defined(WITH_CRASHMANAGER)
/* @brief Set coredump manager object
 * @param cd Coredump object
 * @param manager Manager object
 */
void cdh_coredump_set_manager (CdhCoredump *cd, CdhManager *manager);
#endif

/* @brief Generate coredump file
 * @param cd Coredump object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_coredump_generate (CdhCoredump *cd);

G_END_DECLS
