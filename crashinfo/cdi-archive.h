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
 * \file cdi-archive.h
 */

#pragma once

#include "cdm-types.h"

#include <archive.h>
#include <archive_entry.h>
#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The archive object
 */
typedef struct _CdiArchive
{
  struct archive *archive; /**< Archive object  */
  gchar *file_path;        /**< Archive file path for repopen */
  grefcount rc;            /**< Reference counter variable  */
} CdiArchive;

/**
 * @brief Create a new CdiArchive object
 * @return A pointer to the new CdiArchive object
 */
CdiArchive *cdi_archive_new (void);

/**
 * @brief Aquire the archive object
 * @param ar Pointer to the object
 * @return a pointer to archive object
 */
CdiArchive *cdi_archive_ref (CdiArchive *ar);

/**
 * @brief Release archive object
 * @param ar Pointer to the object
 */
void cdi_archive_unref (CdiArchive *ar);

/**
 * @brief Open archive for read
 * @param ar Pointer to the object
 * @param fname Absoluet path to the archive file
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_read_open (CdiArchive *ar, const gchar *fname);

/**
 * @brief List archive content to stdout
 *        The archive has to be opened first
 * @param ar Pointer to the object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_list_stdout (CdiArchive *ar);

/**
 * @brief Print information about crash archive
 *        The archive has to be opened first
 * @param ar Pointer to the object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_print_info (CdiArchive *ar);

/**
 * @brief Print epilog from crash archive
 *        The archive has to be opened first
 * @param ar Pointer to the object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_print_epilog (CdiArchive *ar);

/**
 * @brief Print file content to stdout
 *        The archive has to be opened first
 * @param ar Pointer to the object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_print_file (CdiArchive *ar, const gchar *fname);

/**
 * @brief Extract coredump in current directory. The archive has to be opened first
 * @param ar Pointer to the object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_extract_coredump (CdiArchive *ar, const gchar *dpath);

/**
 * @brief Print coredump backtrace. The archive has to be opened first
 * @param ar Pointer to the object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_print_backtrace (CdiArchive *ar, gboolean all);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdiArchive, cdi_archive_unref);

G_END_DECLS
