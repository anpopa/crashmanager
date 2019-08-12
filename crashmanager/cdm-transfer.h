/* cdm-transfer.h
 *
 * Copyright 2019 Alin Popa <alin.popa@fxdata.ro>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written
 * authorization.
 */

#pragma once

#include "cdm-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @function CdmTransferCallback
 * @brief Custom callback used internally by CdmTransfer as source callback
 */
typedef gboolean (*CdmTransferCallback) (gpointer cdmtrans, gpointer entry);

/**
 * @function CdmTransferEntryCallback
 * @brief Client callback to pass when requesting a file transfer
 */
typedef void (*CdmTransferEntryCallback) (gpointer user_data, const gchar *file_path);

/**
 * @struct CdmTransferEntry
 * @brief The file transfer entry
 */
typedef struct _CdmTransferEntry {
  gchar *file_path;
  gpointer user_data;
  CdmTransferEntryCallback callback;
} CdmTransferEntry;

/**
 * @struct CdmTransfer
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
 * @return On success return a new CdmTransfer object otherwise return NULL
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
CdmStatus cdm_transfer_file (CdmTransfer *transfer, const gchar *file_path, CdmTransferEntryCallback callback, gpointer user_data);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmTransfer, cdm_transfer_unref);

G_END_DECLS
