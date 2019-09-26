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
 * \file cdi-archive.h
 */

#pragma once

#include "cdm-types.h"

#include <glib.h>
#include <archive.h>
#include <archive_entry.h>

G_BEGIN_DECLS

/**
 * @brief The archive object
 */
typedef struct _CdiArchive {
  struct archive *archive;  /**< Archive object  */
  gchar *file_path;         /**< Archive file path for repopen */
  grefcount rc;             /**< Reference counter variable  */
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
 * @brief Print file content to stdout
 *        The archive has to be opened first
 * @param ar Pointer to the object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_print_file (CdiArchive *ar, const gchar *fname);

/**
 * @brief Extract coredump in current directory
 *        The archive has to be opened first
 * @param ar Pointer to the object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_extract_coredump (CdiArchive *ar, const gchar *dpath);

/**
 * @brief Print coredump backtrace
 *        The archive has to be opened first
 * @param ar Pointer to the object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_print_backtrace (CdiArchive *ar, gboolean all);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdiArchive, cdi_archive_unref);

G_END_DECLS
