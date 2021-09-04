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
 * \file cdm-server.h
 */

#pragma once

#include "cdm-journal.h"
#include "cdm-options.h"
#include "cdm-transfer.h"
#include "cdm-types.h"
#ifdef WITH_GENIVI_NSM
#include "cdm-lifecycle.h"
#endif
#ifdef WITH_DBUS_SERVICES
#include "cdm-dbusown.h"
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The CdmServer opaque data structure
 */
typedef struct _CdmServer {
    GSource source;        /**< Event loop source */
    grefcount rc;          /**< Reference counter variable  */
    gpointer tag;          /**< Unix server socket tag  */
    gint sockfd;           /**< Module file descriptor (server listen fd) */
    CdmOptions *options;   /**< Own reference to global options */
    CdmTransfer *transfer; /**< Own a reference to transfer object */
    CdmJournal *journal;   /**< Own a reference to journal object */
#ifdef WITH_GENIVI_NSM
    CdmJournal *lifecycle; /**< Own a reference to the lifecycle object */
#endif
#ifdef WITH_DBUS_SERVICES
    CdmDBusOwn *dbusown; /**< Own a reference to dbusown object */
#endif
} CdmServer;

/*
 * @brief Create a new server object
 * @param options A pointer to the CdmOptions object created by the main application
 * @param transfer A pointer to the CdmTransfer object created by the main application
 * @param journal A pointer to the CdmJournal object created by the main application
 * @return On success return a new CdmServer object otherwise return NULL
 */
CdmServer *
cdm_server_new(CdmOptions *options, CdmTransfer *transfer, CdmJournal *journal, GError **error);

/**
 * @brief Aquire server object
 * @param server Pointer to the server object
 * @return The server object
 */
CdmServer *cdm_server_ref(CdmServer *server);

/**
 * @brief Start the server an listen for clients
 * @param server Pointer to the server object
 * @return If server starts listening the function return CDM_STATUS_OK
 */
CdmStatus cdm_server_bind_and_listen(CdmServer *server);

/**
 * @brief Release server object
 * @param server Pointer to the server object
 */
void cdm_server_unref(CdmServer *server);

#ifdef WITH_GENIVI_NSM
/**
 * @brief Set lifecycle object
 * @param server Pointer to the client object
 * @param lifecycle Pointer to the lifecycle object
 */
void cdm_server_set_lifecycle(CdmServer *server, CdmLifecycle *lifecycle);
#endif
#ifdef WITH_DBUS_SERVICES
/**
 * @brief Set dbusown object
 * @param server Pointer to the client object
 * @param dbusown Pointer to the dbusown object
 */
void cdm_server_set_dbusown(CdmServer *server, CdmDBusOwn *dbusown);
#endif

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdmServer, cdm_server_unref);

G_END_DECLS
