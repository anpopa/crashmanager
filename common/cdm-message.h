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
 * \file cdm-message.h
 */

#pragma once

#include "cdm-types.h"

#include <glib.h>

G_BEGIN_DECLS

#define CDM_MESSAGE_PROTOCOL_VERSION (0x0001) /* increment the version if the protocol changes */
#define CDM_MESSAGE_START_HASH (0xECDE)

#define CDM_MESSAGE_PROCNAME_MAX_LEN (32)
#define CDM_MESSAGE_THREDNAME_MAX_LEN (32)
#define CDM_MESSAGE_FILENAME_MAX_LEN (1024)
#define CDM_MESSAGE_CRASHID_MAX_LEN (32)
#define CDM_MESSAGE_CTXNAME_MAX_LEN (32)
#define CDM_MESSAGE_LCSTATE_MAX_LEN (32)
#define CDM_MESSAGE_VERSION_MAX_LEN (8)
#define CDM_MESSAGE_EPILOG_FRAME_MAX_LEN CDM_EPILOG_FRAME_LEN
#define CDM_MESSAGE_EPILOG_FRAME_MAX_CNT CDM_EPILOG_FRAME_CNT

/**
 * @brief The message types
 */
typedef enum _CdmMessageType
{
  CDM_MESSAGE_INVALID,
  CDM_MESSAGE_COREDUMP_NEW,
  CDM_MESSAGE_COREDUMP_UPDATE,
  CDM_MESSAGE_COREDUMP_SUCCESS,
  CDM_MESSAGE_COREDUMP_FAILED,
  CDM_MESSAGE_COREDUMP_CONTEXT,
  CDM_MESSAGE_EPILOG_FRAME_INFO,
  CDM_MESSAGE_EPILOG_FRAME_DATA
} CdmMessageType;

/**
 * @brief The CdmMessageHdr opaque data structure
 */
typedef struct _CdmMessageHdr
{
  uint16_t hsh;
  uint16_t session;
  uint32_t version;
  CdmMessageType type;
  uint16_t size_of_arg1;
  uint16_t size_of_arg2;
  uint16_t size_of_arg3;
  uint16_t size_of_arg4;
  uint16_t size_of_arg5;
  uint16_t size_of_arg6;
  uint16_t size_of_arg7;
  uint16_t size_of_arg8;
} CdmMessageHdr;

/**
 * @brief The CdmMessageData opaque data structure
 */
typedef struct _CdmMessageData
{
  int64_t process_pid;
  int64_t process_exit_signal;
  uint64_t process_timestamp;
  uint64_t epilog_frame_count;
  gchar *epilog_frame_data;
  gchar *lifecycle_state;
  gchar *process_name;
  gchar *thread_name;
  gchar *context_name;
  gchar *process_crash_id;
  gchar *process_vector_id;
  gchar *process_context_id;
  gchar *coredump_file_path;
} CdmMessageData;

/**
 * @brief The CdmMessage opaque data structure
 */
typedef struct _CdmMessage
{
  CdmMessageHdr hdr;
  CdmMessageData data;
  grefcount rc;
} CdmMessage;

/*
 * @brief Create a new message object
 * @param type The message type
 * @param session Unique session identifier
 * @return The new allocated message object
 */
CdmMessage *cdm_message_new (CdmMessageType type, uint16_t session);

/*
 * @brief Aquire a message object
 * @param msg The message object
 * @return The referenced message object
 */
CdmMessage *cdm_message_ref (CdmMessage *msg);

/*
 * @brief Release message
 * @param msg The message object
 */
void cdm_message_unref (CdmMessage *msg);

/*
 * @brief Validate if the message object is consistent
 * @param msg The message object
 */
gboolean cdm_message_is_valid (CdmMessage *msg);

/*
 * @brief Get message type
 * @param msg The message object
 */
CdmMessageType cdm_message_get_type (CdmMessage *msg);

/*
 * @brief Get message session id
 * @param msg The message object
 */
guint16 cdm_message_get_session (CdmMessage *msg);

/*
 * @brief Set epilog frame count as epilog info.
 * @param msg The message object
 * @param frame_count The number of frames
 */
void cdm_message_set_epilog_frame_count (CdmMessage *msg, uint64_t frame_count);

/*
 * @brief Get epilog frame count as epilog info.
 * @param msg The message object
 * @return The epilog frame count
 */
uint64_t cdm_message_get_epilog_frame_count (CdmMessage *msg);

/*
 * @brief Set epilog frame data as epilog info.
 * @param msg The message object
 * @param frame_data The number of frames
 */
void cdm_message_set_epilog_frame_data (CdmMessage *msg, const gchar *frame_data);

/*
 * @brief Get epilog frame count as epilog info.
 * @param msg The message object
 * @return The epilog frame data
 */
const gchar *cdm_message_get_epilog_frame_data (CdmMessage *msg);

/*
 * @brief Set lifecycle state
 * @param msg The message object
 * @param lifecycle_state The lifecycle state
 */
void cdm_message_set_lifecycle_state (CdmMessage *msg, const gchar *lifecycle_state);

/*
 * @brief Get lifecycle state.
 * @param msg The message object
 * @return The lifecycle state
 */
const gchar *cdm_message_get_lifecycle_state (CdmMessage *msg);

/*
 * @brief Set context name
 * @param msg The message object
 * @param context_name The lifecycle state
 */
void cdm_message_set_context_name (CdmMessage *msg, const gchar *context_name);

/*
 * @brief Get context name
 * @param msg The message object
 * @return The context name
 */
const gchar *cdm_message_get_context_name (CdmMessage *msg);

/*
 * @brief Set process pid
 * @param msg The message object
 * @param pid The process pid
 */
void cdm_message_set_process_pid (CdmMessage *msg, int64_t pid);

/*
 * @brief Get process pid
 * @param msg The message object
 * @return The process pid
 */
int64_t cdm_message_get_process_pid (CdmMessage *msg);

/*
 * @brief Set process exit signal
 * @param msg The message object
 * @param process_signal The process exit signal
 */
void cdm_message_set_process_exit_signal (CdmMessage *msg, int64_t process_signal);

/*
 * @brief Get process exit signal
 * @param msg The message object
 * @return The process exit signal
 */
int64_t cdm_message_get_process_exit_signal (CdmMessage *msg);

/*
 * @brief Set process timestamp
 * @param msg The message object
 * @param timestamp The process timestamp
 */
void cdm_message_set_process_timestamp (CdmMessage *msg, uint64_t timestamp);

/*
 * @brief Get process timestamp
 * @param msg The message object
 * @return The process timestamp
 */
uint64_t cdm_message_get_process_timestamp (CdmMessage *msg);

/*
 * @brief Set process name
 * @param msg The message object
 * @param name The process name
 */
void cdm_message_set_process_name (CdmMessage *msg, const gchar *name);

/*
 * @brief Get process name
 * @param msg The message object
 * @return The process name
 */
const gchar *cdm_message_get_process_name (CdmMessage *msg);

/*
 * @brief Set thread name
 * @param msg The message object
 * @param thread_name The thread name
 */
void cdm_message_set_thread_name (CdmMessage *msg, const gchar *thread_name);

/*
 * @brief Get thread name
 * @param msg The message object
 * @return The thread name
 */
const gchar *cdm_message_get_thread_name (CdmMessage *msg);

/*
 * @brief Set process crash ID
 * @param msg The message object
 * @param crashid The process crash ID
 */
void cdm_message_set_process_crash_id (CdmMessage *msg, const gchar *crashid);

/*
 * @brief Get process crash ID
 * @param msg The message object
 * @return The process crash ID
 */
const gchar *cdm_message_get_process_crash_id (CdmMessage *msg);

/*
 * @brief Set process vector ID
 * @param msg The message object
 * @param vectorid The process vector ID
 */
void cdm_message_set_process_vector_id (CdmMessage *msg, const gchar *vectorid);

/*
 * @brief Get process vector ID
 * @param msg The message object
 * @return The process vector ID
 */
const gchar *cdm_message_get_process_vector_id (CdmMessage *msg);

/*
 * @brief Set process context ID
 * @param msg The message object
 * @param contextid The process context ID
 */
void cdm_message_set_process_context_id (CdmMessage *msg, const gchar *contextid);

/*
 * @brief Get process context ID
 * @param msg The message object
 * @return The process context ID
 */
const gchar *cdm_message_get_process_context_id (CdmMessage *msg);

/*
 * @brief Set coredump file patch
 * @param msg The message object
 * @param fpath The coredump file path
 */
void cdm_message_set_coredump_file_path (CdmMessage *msg, const gchar *fpath);

/*
 * @brief Get coredump file patch
 * @param msg The message object
 * @return The coredump file path
 */
const gchar *cdm_message_get_coredump_file_path (CdmMessage *msg);

/*
 * @brief Read data into message object
 * If message read has payload the data has to be released by the caller
 * @param msg The message object
 * @param fd File descriptor to read from
 * @return CDM_STATUS_OK on success, CDM_STATUS_ERROR otherwise
 */
CdmStatus cdm_message_read (gint fd, CdmMessage *msg);

/*
 * @brief Write data into message object
 * @param m The message object
 * @param fd File descriptor to write to
 * @return CDM_STATUS_OK on success, CDM_STATUS_ERROR otherwise
 */
CdmStatus cdm_message_write (gint fd, CdmMessage *msg);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmMessage, cdm_message_unref);

G_END_DECLS
