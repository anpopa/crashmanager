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
 * \file cdm-client.h
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
 * @brief The CdmClient opaque data structure
 */
typedef struct _CdmClient {
  GSource source;   /**< Event loop source */
  gpointer tag;     /**< Unix server socket tag  */
  grefcount rc;     /**< Reference counter variable  */
  gint sockfd;      /**< Module file descriptor (client fd) */
  guint64 id;       /**< Client instance id */

  CdmTransfer *transfer; /**< Own a reference to the transfer object */
  CdmJournal *journal;   /**< Own a reference to the journal object */
#ifdef WITH_GENIVI_NSM
  CdmLifecycle *lifecycle; /**< Own a reference to the lifecycle object */
#endif
  CdmMessageType last_msg_type;          /**< Last processed message type */
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
 * @return On success return a new CdmClient object
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
