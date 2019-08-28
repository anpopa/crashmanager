/* cdm-client.h
 *
 * Copyright 2019 Alin Popa <alin.popa@bmw.de>
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
#include "cdm-message.h"
#include "cdm-transfer.h"
#include "cdm-journal.h"
#ifdef WITH_GENIVI_NSM
#include "cdm-lifecycle.h"
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * @struct CdmClient
 * @brief The CdmClient opaque data structure
 */
typedef struct _CdmClient {
  GSource source;  /**< Event loop source */
  gpointer tag;     /**< Unix server socket tag  */
  grefcount rc;     /**< Reference counter variable  */
  gint sockfd;      /**< Module file descriptor (client fd) */
  guint64 id;       /**< Client instance id */

  CdmTransfer *transfer; /**< Own a reference to the transfer object */
  CdmJournal *journal; /**< Own a reference to the journal object */
#ifdef WITH_GENIVI_NSM
  CdmLifecycle *lifecycle; /**< Own a reference to the lifecycle object */
#endif
  CdmMessageType last_msg_type; /**< Last processed message type */
  CdmMessageDataNew *init_data;          /**< Coredump initial data */
  CdmMessageDataUpdate *update_data;     /**< Coredump update data */
  CdmMessageDataComplete *complete_data; /**< Coredump complete data */
} CdmClient;

/*
 * @brief Create a new client object
 * @param clientfd Socket file descriptor accepted by the server
 * @param transfer A pointer to the CdmTransfer object created by the main
 * application
 * @param journal A pointer to the CdmJournal object created by the main
 * application
 * @return On success return a new CdmClient object otherwise return NULL
 */
CdmClient *cdm_client_new (gint clientfd, CdmTransfer *transfer, CdmJournal *journal);

/**
 * @brief Aquire client object
 * @param client Pointer to the client object
 * @return The referenced client object
 */
CdmClient *cdm_client_ref (CdmClient *client);

/**
 * @brief Release client object
 * @param client Pointer to the client object
 */
void cdm_client_unref (CdmClient *client);

#ifdef WITH_GENIVI_NSM
/**
 * @brief Set lifecycle object
 * @param client Pointer to the client object
 * @param lifecycle Pointer to the lifecycle object
 */
void cdm_client_set_lifecycle (CdmClient *client, CdmLifecycle *lifecycle);
#endif

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmClient, cdm_client_unref);

G_END_DECLS
