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
#ifdef WITH_DBUS_SERVICES
#include "cdm-dbusown.h"
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The CdmClient opaque data structure
 */
typedef struct _CdmClient {
  GSource source;                        /**< Event loop source */
  gpointer tag;                          /**< Unix server socket tag  */
  grefcount rc;                          /**< Reference counter variable  */
  gint sockfd;                           /**< Module file descriptor (client fd) */
  guint64 id;                            /**< Client instance id */

  CdmTransfer *transfer;                 /**< Own a reference to the transfer object */
  CdmJournal *journal;                   /**< Own a reference to the journal object */
#ifdef WITH_GENIVI_NSM
  CdmLifecycle *lifecycle;               /**< Own a reference to the lifecycle object */
#endif
#ifdef WITH_DBUS_SERVICES
  CdmDBusOwn *dbusown;                   /**< Own a reference to dbusown object */
#endif
  CdmMessageType last_msg_type;          /**< Last processed message type */
  
  int64_t  process_pid;
  int64_t  process_exit_signal;
  uint64_t process_timestamp;
  gchar   *lifecycle_state;
  gchar   *process_name;
  gchar   *thread_name;
  gchar   *context_name;
  gchar   *process_crash_id;
  gchar   *process_vector_id;
  gchar   *process_context_id;
  gchar   *coredump_file_path;
} CdmClient;

/*
 * @brief Create a new client object
 * @param clientfd Socket file descriptor accepted by the server
 * @param transfer A pointer to the CdmTransfer object created by the main application
 * @param journal A pointer to the CdmJournal object created by the main application
 * @return On success return a new CdmClient object
 */
CdmClient *             cdm_client_new                      (gint clientfd, 
                                                             CdmTransfer *transfer, 
                                                             CdmJournal *journal);

/**
 * @brief Aquire client object
 * @param client Pointer to the client object
 * @return The referenced client object
 */
CdmClient *             cdm_client_ref                      (CdmClient *client);

/**
 * @brief Release client object
 * @param client Pointer to the client object
 */
void                    cdm_client_unref                    (CdmClient *client);

#ifdef WITH_GENIVI_NSM
/**
 * @brief Set lifecycle object
 * @param client Pointer to the client object
 * @param lifecycle Pointer to the lifecycle object
 */
void                    cdm_client_set_lifecycle            (CdmClient *client, 
                                                             CdmLifecycle *lifecycle);
#endif
#ifdef WITH_DBUS_SERVICES
/**
 * @brief Set dbusown object
 * @param server Pointer to the client object
 * @param dbusown Pointer to the dbusown object
 */
void                    cdm_client_set_dbusown              (CdmClient *client, 
                                                             CdmDBusOwn *dbusown);
#endif

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmClient, cdm_client_unref);

G_END_DECLS
