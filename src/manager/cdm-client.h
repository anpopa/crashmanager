/* cdm-client.h
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

#ifndef CDM_CLIENT_H
#define CDM_CLIENT_H

#include "cdm-types.h"
#include "cdm-message.h"
#include "cdm-transfer.h"

#include <glib.h>

/**
 * @struct CdmClient
 * @brief The CdmClient opaque data structure
 */
typedef struct _CdmClient {
  GSource *source;  /**< Event loop source */
  gpointer tag;     /**< Unix server socket tag  */
  grefcount rc;     /**< Reference counter variable  */
  gint sockfd;      /**< Module file descriptor (client fd) */
  guint64 id;       /**< Client instance id */

  CdmTransfer *transfer; /**< Own a reference to transfer object */

  CdmMessageType last_msg_type; /**< Last processed message type */
  CdmMessageDataNew *init_data;          /**< Coredump initial data */
  CdmMessageDataUpdate *update_data;     /**< Coredump update data */
  CdmMessageDataComplete *complete_data; /**< Coredump complete data */
} CdmClient;

/*
 * @brief Create a new client object
 * @return On success return a new CdmClient object otherwise return NULL
 */
CdmClient *cdm_client_new (gint clientfd, CdmTransfer *transfer);

/**
 * @brief Aquire client object
 * @param c Pointer to the client object
 */
CdmClient *cdm_client_ref (CdmClient *client);

/**
 * @brief Release client object
 * @param c Pointer to the client object
 */
void cdm_client_unref (CdmClient *client);

/**
 * @brief Get object event loop source
 * @param c Pointer to the client object
 */
GSource *cdm_client_get_source (CdmClient *client);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmClient, cdm_client_unref);

#endif /* CDM_CLIENT_H */
