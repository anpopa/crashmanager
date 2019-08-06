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

#ifndef CDM_TRANSFER_H
#define CDM_TRANSFER_H

#include "cdm-types.h"

#include <glib.h>

/**
 * @struct CdmTransfer
 * @brief The CdmTransfer opaque data structure
 */
typedef struct _CdmTransfer {
  GSource source;  /**< Event loop source */
  grefcount rc;     /**< Reference counter variable  */
  GAsyncQueue    *queue;  /**< Async queue */
  GThreadPool    *tpool;  /**< Transfer thread pool */
} CdmTransfer;

/*
 * @brief Create a new transfer object
 * @return On success return a new CdmTransfer object otherwise return NULL
 */
CdmTransfer *cdm_transfer_new (void);

/**
 * @brief Aquire transfer object
 * @param transfer Pointer to the transfer object
 */
CdmTransfer *cdm_transfer_ref (CdmTransfer *transfer);

/**
 * @brief Release transfer object
 * @param transfer Pointer to the transfer object
 */
void cdm_transfer_unref (CdmTransfer *transfer);

/**
 * @brief Transfer a file
 * @param transfer Pointer to the transfer object
 */
CdmStatus cdm_transfer_file (CdmTransfer *transfer, const gchar *fpath);

/**
 * @brief Get object event loop source
 * @param transfer Pointer to the transfer object
 */
GSource *cdm_transfer_get_source (CdmTransfer *transfer);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmTransfer, cdm_transfer_unref);

#endif /* CDM_TRANSFER_H */
