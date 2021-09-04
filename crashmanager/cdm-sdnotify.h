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
    GSource *source; /**< Event loop source */
    grefcount rc;    /**< Reference counter variable  */
} CdmSDNotify;

/*
 * @brief Create a new sdnotify object
 * @return On success return a new CdmSDNotify object
 */
CdmSDNotify *cdm_sdnotify_new(void);

/**
 * @brief Aquire sdnotify object
 * @param sdnotify Pointer to the sdnotify object
 * @return The referenced sdnotify object
 */
CdmSDNotify *cdm_sdnotify_ref(CdmSDNotify *sdnotify);

/**
 * @brief Release sdnotify object
 * @param sdnotify Pointer to the sdnotify object
 */
void cdm_sdnotify_unref(CdmSDNotify *sdnotify);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdmSDNotify, cdm_sdnotify_unref);

G_END_DECLS
