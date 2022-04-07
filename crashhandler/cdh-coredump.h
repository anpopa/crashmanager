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
 * \file cdh-coredump.h
 */

#pragma once

#include "cdh-archive.h"
#include "cdh-context.h"
#if defined(WITH_CRASHMANAGER)
#include "cdh-manager.h"
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The coredump generation object
 */
typedef struct _CdhCoredump
{
  CdhContext *context; /**< Context object owned */
  CdhArchive *archive; /**< Archive object owned */
#if defined(WITH_CRASHMANAGER)
  CdhManager *manager; /**< Manager object owned */
#endif
  grefcount rc; /**< Reference counter variable  */
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
