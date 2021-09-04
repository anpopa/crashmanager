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
 * \file cdm-elogclt.h
 */

#pragma once

#include "cdh-elogmsg.h"
#include "cdm-journal.h"
#include "cdm-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The CdmELogClt opaque data structure
 */
typedef struct _CdmELogClt {
    GSource source;                   /**< Event loop source */
    gpointer tag;                     /**< Unix server socket tag  */
    grefcount rc;                     /**< Reference counter variable  */
    gint sockfd;                      /**< Module file descriptor (client fd) */
    CdmJournal *journal;              /**< Own a reference to the journal object */
    CdmELogMessageType last_msg_type; /**< Last processed message type */
    int64_t process_pid;              /**< Process PID*/
    int64_t process_sig;              /**< Process exit signal*/
} CdmELogClt;

/*
 * @brief Create a new client object
 * @param clientfd Socket file descriptor accepted by the server
 * @param transfer A pointer to the CdmTransfer object created by the main application
 * @param journal A pointer to the CdmJournal object created by the main application
 * @return On success return a new CdmELogClt object
 */
CdmELogClt *cdm_elogclt_new(gint clientfd, CdmJournal *journal);

/**
 * @brief Aquire client object
 * @param client Pointer to the client object
 * @return The referenced client object
 */
CdmELogClt *cdm_elogclt_ref(CdmELogClt *client);

/**
 * @brief Release client object
 * @param client Pointer to the client object
 */
void cdm_elogclt_unref(CdmELogClt *client);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdmELogClt, cdm_elogclt_unref);

G_END_DECLS
