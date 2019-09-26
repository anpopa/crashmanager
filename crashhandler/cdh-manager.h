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
 * \file cdh-manager.h
 */

#pragma once

#include "cdm-message.h"
#include "cdm-options.h"
#include "cdm-types.h"

#include <glib.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

G_BEGIN_DECLS

#ifndef MANAGER_SELECT_TIMEOUT
#define MANAGER_SELECT_TIMEOUT 3
#endif

/**
 * @brief The coredump handler manager object
 */
typedef struct _CdhManager {
  grefcount rc;                /**< Reference counter variable  */
  gint sfd;                    /**< Server (manager) unix domain file descriptor */
  gboolean connected;          /**< Server connection state */
  struct sockaddr_un saddr;    /**< Server socket addr struct */
  CdmOptions *opts;            /**< Reference to options object */
} CdhManager;

/**
 * @brief Create a new CdhManager object
 * @param opts Pointer to global options object
 * @return A new CdhManager objects
 */
CdhManager *cdh_manager_new (CdmOptions *opts);

/**
 * @brief Aquire CdhManager object
 * @param c Manager object
 */
CdhManager *cdh_manager_ref (CdhManager *c);

/**
 * @brief Release CdhManager object
 * @param c Manager object
 */
void cdh_manager_unref (CdhManager *c);

/**
 * @brief Connect to cdh manager
 * @param c Manager object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_manager_connect (CdhManager *c);

/**
 * @brief Disconnect from cdh manager
 * @param c Manager object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_manager_disconnect (CdhManager *c);

/**
 * @brief Get connection state
 * @param c Manager object
 * @return True if connected
 */
gboolean cdh_manager_connected (CdhManager *c);

/**
 * @brief Get connection socket fd
 * @param c Manager object
 */
gint cdh_manager_get_socket (CdhManager *c);

/**
 * @brief Send message to cdh manager
 *
 * @param c Manager object
 * @param m Message to send
 *
 * @return True if connected
 */
CdmStatus cdh_manager_send (CdhManager *c, CdmMessage *m);

G_END_DECLS
