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
 * \file cdm-sdnotify.h
 */

#pragma once

#include "cdm-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The CdmSDNotify opaque data structure
 */
typedef struct _CdmSDNotify {
  GSource *source;  /**< Event loop source */
  grefcount rc;     /**< Reference counter variable  */
} CdmSDNotify;

/*
 * @brief Create a new sdnotify object
 * @return On success return a new CdmSDNotify object
 */
CdmSDNotify *cdm_sdnotify_new (void);

/**
 * @brief Aquire sdnotify object
 * @param sdnotify Pointer to the sdnotify object
 * @return The referenced sdnotify object
 */
CdmSDNotify *cdm_sdnotify_ref (CdmSDNotify *sdnotify);

/**
 * @brief Release sdnotify object
 * @param sdnotify Pointer to the sdnotify object
 */
void cdm_sdnotify_unref (CdmSDNotify *sdnotify);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmSDNotify, cdm_sdnotify_unref);

G_END_DECLS
