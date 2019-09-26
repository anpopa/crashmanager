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
 * \file cdm-server.h
 */

#pragma once

#include "cdm-types.h"
#include "cdm-options.h"
#include "cdm-transfer.h"
#include "cdm-journal.h"
#ifdef WITH_GENIVI_NSM
#include "cdm-lifecycle.h"
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The CdmServer opaque data structure
 */
typedef struct _CdmServer {
  GSource source;   /**< Event loop source */
  grefcount rc;     /**< Reference counter variable  */
  gpointer tag;     /**< Unix server socket tag  */
  gint sockfd;      /**< Module file descriptor (server listen fd) */
  CdmOptions *options;   /**< Own reference to global options */
  CdmTransfer *transfer; /**< Own a reference to transfer object */
  CdmJournal *journal;   /**< Own a reference to journal object */
#ifdef WITH_GENIVI_NSM
  CdmJournal *lifecycle; /**< Own a reference to the lifecycle object */
#endif
} CdmServer;

/*
 * @brief Create a new server object
 *
 * @param options A pointer to the CdmOptions object created by the main
 * application
 * @param transfer A pointer to the CdmTransfer object created by the main
 * application
 * @param journal A pointer to the CdmJournal object created by the main
 * application
 *
 * @return On success return a new CdmServer object otherwise return NULL
 */
CdmServer *cdm_server_new (CdmOptions *options, CdmTransfer *transfer, CdmJournal *journal, GError **error);

/**
 * @brief Aquire server object
 * @param server Pointer to the server object
 * @return The server object
 */
CdmServer *cdm_server_ref (CdmServer *server);

/**
 * @brief Start the server an listen for clients
 * @param server Pointer to the server object
 * @return If server starts listening the function return CDM_STATUS_OK
 */
CdmStatus cdm_server_bind_and_listen (CdmServer *server);

/**
 * @brief Release server object
 * @param server Pointer to the server object
 */
void cdm_server_unref (CdmServer *server);

#ifdef WITH_GENIVI_NSM
/**
 * @brief Set lifecycle object
 * @param server Pointer to the client object
 * @param lifecycle Pointer to the lifecycle object
 */
void cdm_server_set_lifecycle (CdmServer *server, CdmLifecycle *lifecycle);
#endif

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmServer, cdm_server_unref);

G_END_DECLS
