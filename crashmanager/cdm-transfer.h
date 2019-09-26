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
 * \file cdm-transfer.h
 */

#pragma once

#include "cdm-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief Custom callback used internally by CdmTransfer as source callback
 */
typedef gboolean (*CdmTransferCallback) (gpointer _transfer, gpointer _entry);

/**
 * @brief Client callback to pass when requesting a file transfer
 */
typedef void (*CdmTransferEntryCallback) (gpointer _transfer, const gchar *file_path);

/**
 * @brief The file transfer entry
 */
typedef struct _CdmTransferEntry {
  gchar *file_path;
  gpointer user_data;
  CdmTransferEntryCallback callback;
} CdmTransferEntry;

/**
 * @brief The CdmTransfer opaque data structure
 */
typedef struct _CdmTransfer {
  GSource source;  /**< Event loop source */
  grefcount rc;     /**< Reference counter variable  */
  GAsyncQueue    *queue;  /**< Async queue */
  GThreadPool    *tpool;  /**< Transfer thread pool */
  CdmTransferCallback callback; /**< Transfer callback function */
} CdmTransfer;

/*
 * @brief Create a new transfer object
 * @return On success return a new CdmTransfer object
 */
CdmTransfer *cdm_transfer_new (void);

/**
 * @brief Aquire transfer object
 * @param transfer Pointer to the transfer object
 * @return The referenced transfer object
 */
CdmTransfer *cdm_transfer_ref (CdmTransfer *transfer);

/**
 * @brief Release transfer object
 * @param transfer Pointer to the transfer object
 */
void cdm_transfer_unref (CdmTransfer *transfer);

/**
 * @brief Transfer a file
 *
 * @param transfer Pointer to the transfer object
 * @param file_path A pointer to file_path string
 * @param callback The callback function to call when transfer finish
 * @param user_data A pointer to provide as argument to the callback
 *
 * @return on succhess return CDM_STATUS_OK
 */
CdmStatus cdm_transfer_file (CdmTransfer *transfer,
                             const gchar *file_path,
                             CdmTransferEntryCallback callback,
                             gpointer user_data);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmTransfer, cdm_transfer_unref);

G_END_DECLS
