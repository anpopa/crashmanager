/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019 Alin Popa
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
 * \file cdm-message.h
 */

#pragma once

#include "cdm-message-type.h"
#include "cdm-types.h"

#include <glib.h>

G_BEGIN_DECLS

/*
 * @brief Initialize a pre-allocated mesage object
 *
 * @param m The message object
 * @param type The message type
 * @param session Unique session identifier
 */
void cdm_message_init (CdmMessage *m, CdmMessageType type, uint16_t session);

/*
 * @brief Message data setter
 *
 * @param m The message object
 * @param data The message data to set
 * @param size The message data size
 */
void cdm_message_set_data (CdmMessage *m, void *data, uint32_t size);

/*
 * @brief Release the data field of a message if set
 * @param m The message object
 */
void cdm_message_free_data (CdmMessage *m);

/*
 * @brief Validate if the message object is consistent
 * @param m The message object
 */
bool cdm_message_is_valid (CdmMessage *m);

/*
 * @brief Get message type
 * @param m The message object
 */
CdmMessageType cdm_message_get_type (CdmMessage *m);

/*
 * @brief Set message version
 *
 * @param m The message object
 * @param version Version string
 *
 * @return CDM_STATUS_OK on success, CDM_STATUS_ERROR otherwise
 */
CdmStatus cdm_message_set_version (CdmMessage *m, const gchar *version);

/*
 * @brief Read data into message object
 *
 * If message read has payload the data has to be released by the caller
 *
 * @param m The message object
 * @param fd File descriptor to read from
 *
 * @return CDM_STATUS_OK on success, CDM_STATUS_ERROR otherwise
 */
CdmStatus cdm_message_read (gint fd, CdmMessage *m);

/*
 * @brief Write data into message object
 *
 * @param m The message object
 * @param fd File descriptor to write to
 *
 * @return CDM_STATUS_OK on success, CDM_STATUS_ERROR otherwise
 */
CdmStatus cdm_message_write (gint fd, CdmMessage *m);

G_END_DECLS
