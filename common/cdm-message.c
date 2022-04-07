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
 * \file cdm-message.c
 */

#include "cdm-message.h"

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>

#define CDM_MESSAGE_IOVEC_MAX_ARRAY (16)
const char *cdm_notavailable_str = "NotAvailable";

CdmMessage *
cdm_message_new (CdmMessageType type, uint16_t session)
{
  CdmMessage *msg = g_new0 (CdmMessage, 1);

  g_assert (msg);

  g_ref_count_init (&msg->rc);
  msg->hdr.type = type;
  msg->hdr.hsh = CDM_MESSAGE_START_HASH;
  msg->hdr.session = session;
  msg->hdr.version = CDM_MESSAGE_PROTOCOL_VERSION;

  return msg;
}

CdmMessage *
cdm_message_ref (CdmMessage *msg)
{
  g_assert (msg);
  g_ref_count_inc (&msg->rc);
  return msg;
}

void
cdm_message_unref (CdmMessage *msg)
{
  g_assert (msg);

  if (g_ref_count_dec (&msg->rc) == TRUE)
    {
      g_free (msg->data.epilog_frame_data);
      g_free (msg->data.lifecycle_state);
      g_free (msg->data.process_name);
      g_free (msg->data.thread_name);
      g_free (msg->data.context_name);
      g_free (msg->data.process_crash_id);
      g_free (msg->data.process_vector_id);
      g_free (msg->data.process_context_id);
      g_free (msg->data.coredump_file_path);
      g_free (msg);
    }
}

gboolean
cdm_message_is_valid (CdmMessage *msg)
{
  g_assert (msg);

  if (msg->hdr.hsh != CDM_MESSAGE_START_HASH || msg->hdr.version != CDM_MESSAGE_PROTOCOL_VERSION)
    return FALSE;

  return TRUE;
}

CdmMessageType
cdm_message_get_type (CdmMessage *msg)
{
  g_assert (msg);
  return msg->hdr.type;
}

guint16
cdm_message_get_session (CdmMessage *msg)
{
  g_assert (msg);
  return msg->hdr.session;
}

void
cdm_message_set_epilog_frame_count (CdmMessage *msg, uint64_t frame_count)
{
  g_assert (msg);
  g_return_if_fail (msg->hdr.type == CDM_MESSAGE_EPILOG_FRAME_INFO);

  msg->data.epilog_frame_count = frame_count;
}

uint64_t
cdm_message_get_epilog_frame_count (CdmMessage *msg)
{
  g_assert (msg);
  g_return_val_if_fail (msg->hdr.type == CDM_MESSAGE_EPILOG_FRAME_INFO, (uint64_t)-1);

  return msg->data.epilog_frame_count;
}

void
cdm_message_set_epilog_frame_data (CdmMessage *msg, const gchar *frame_data)
{
  g_assert (msg);
  g_assert (frame_data);
  g_return_if_fail (msg->hdr.type == CDM_MESSAGE_EPILOG_FRAME_DATA);

  msg->data.epilog_frame_data = g_strdup (frame_data);
}

const gchar *
cdm_message_get_epilog_frame_data (CdmMessage *msg)
{
  g_assert (msg);
  g_return_val_if_fail (msg->hdr.type == CDM_MESSAGE_EPILOG_FRAME_DATA, NULL);

  return msg->data.epilog_frame_data;
}

void
cdm_message_set_lifecycle_state (CdmMessage *msg, const gchar *lifecycle_state)
{
  g_assert (msg);
  g_assert (lifecycle_state);
  g_return_if_fail ((msg->hdr.type == CDM_MESSAGE_COREDUMP_SUCCESS)
                    || (msg->hdr.type == CDM_MESSAGE_COREDUMP_CONTEXT));

  msg->data.lifecycle_state = g_strdup (lifecycle_state);
}

const gchar *
cdm_message_get_lifecycle_state (CdmMessage *msg)
{
  g_assert (msg);
  g_return_val_if_fail ((msg->hdr.type == CDM_MESSAGE_COREDUMP_SUCCESS)
                            || (msg->hdr.type == CDM_MESSAGE_COREDUMP_CONTEXT),
                        NULL);

  return msg->data.lifecycle_state;
}

void
cdm_message_set_context_name (CdmMessage *msg, const gchar *context_name)
{
  g_assert (msg);
  g_assert (context_name);
  msg->data.context_name = g_strdup (context_name);
}

const gchar *
cdm_message_get_context_name (CdmMessage *msg)
{
  g_assert (msg);
  return msg->data.context_name;
}

void
cdm_message_set_process_pid (CdmMessage *msg, int64_t pid)
{
  g_assert (msg);
  msg->data.process_pid = pid;
}

int64_t
cdm_message_get_process_pid (CdmMessage *msg)
{
  g_assert (msg);
  return msg->data.process_pid;
}

void
cdm_message_set_process_exit_signal (CdmMessage *msg, int64_t process_signal)
{
  g_assert (msg);
  msg->data.process_exit_signal = process_signal;
}

int64_t
cdm_message_get_process_exit_signal (CdmMessage *msg)
{
  g_assert (msg);
  return msg->data.process_exit_signal;
}

void
cdm_message_set_process_timestamp (CdmMessage *msg, uint64_t timestamp)
{
  g_assert (msg);
  msg->data.process_timestamp = timestamp;
}

uint64_t
cdm_message_get_process_timestamp (CdmMessage *msg)
{
  g_assert (msg);
  return msg->data.process_timestamp;
}

void
cdm_message_set_process_name (CdmMessage *msg, const gchar *name)
{
  g_assert (msg);
  g_assert (name);
  msg->data.process_name = g_strdup (name);
}

const gchar *
cdm_message_get_process_name (CdmMessage *msg)
{
  g_assert (msg);
  return msg->data.process_name;
}

void
cdm_message_set_thread_name (CdmMessage *msg, const gchar *thread_name)
{
  g_assert (msg);
  g_assert (thread_name);
  msg->data.thread_name = g_strdup (thread_name);
}

const gchar *
cdm_message_get_thread_name (CdmMessage *msg)
{
  g_assert (msg);
  return msg->data.thread_name;
}

void
cdm_message_set_process_crash_id (CdmMessage *msg, const gchar *crashid)
{
  g_assert (msg);
  g_assert (crashid);
  msg->data.process_crash_id = g_strdup (crashid);
}

const gchar *
cdm_message_get_process_crash_id (CdmMessage *msg)
{
  g_assert (msg);
  return msg->data.process_crash_id;
}

void
cdm_message_set_process_vector_id (CdmMessage *msg, const gchar *vectorid)
{
  g_assert (msg);
  g_assert (vectorid);
  msg->data.process_vector_id = g_strdup (vectorid);
}

const gchar *
cdm_message_get_process_vector_id (CdmMessage *msg)
{
  g_assert (msg);
  return msg->data.process_vector_id;
}

void
cdm_message_set_process_context_id (CdmMessage *msg, const gchar *contextid)
{
  g_assert (msg);
  g_assert (contextid);
  msg->data.process_context_id = g_strdup (contextid);
}

const gchar *
cdm_message_get_process_context_id (CdmMessage *msg)
{
  g_assert (msg);
  return msg->data.process_context_id;
}

void
cdm_message_set_coredump_file_path (CdmMessage *msg, const gchar *fpath)
{
  g_assert (msg);
  g_assert (fpath);
  msg->data.coredump_file_path = g_strdup (fpath);
}

const gchar *
cdm_message_get_coredump_file_path (CdmMessage *msg)
{
  g_assert (msg);
  return msg->data.coredump_file_path;
}

CdmStatus
cdm_message_read (gint fd, CdmMessage *msg)
{
  struct iovec iov[CDM_MESSAGE_IOVEC_MAX_ARRAY] = {};
  gint iov_index = 0;
  gssize sz;

  g_assert (msg);

  /* set message header segments */
  iov[iov_index].iov_base = &msg->hdr.hsh;
  iov[iov_index++].iov_len = sizeof (msg->hdr.hsh);
  iov[iov_index].iov_base = &msg->hdr.session;
  iov[iov_index++].iov_len = sizeof (msg->hdr.session);
  iov[iov_index].iov_base = &msg->hdr.version;
  iov[iov_index++].iov_len = sizeof (msg->hdr.version);
  iov[iov_index].iov_base = &msg->hdr.type;
  iov[iov_index++].iov_len = sizeof (msg->hdr.type);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg1;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg1);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg2;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg2);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg3;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg3);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg4;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg4);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg5;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg5);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg6;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg6);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg7;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg7);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg8;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg8);

  if ((sz = readv (fd, iov, iov_index)) <= 0)
    return CDM_STATUS_ERROR;

  memset (iov, 0, sizeof (iov));
  iov_index = 0;

  /* set message data segments */
  switch (cdm_message_get_type (msg))
    {
    case CDM_MESSAGE_COREDUMP_NEW:
      /* arg1 */
      iov[iov_index].iov_base = &msg->data.process_pid;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      iov[iov_index].iov_base = &msg->data.process_exit_signal;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;

      /* arg3 */
      iov[iov_index].iov_base = &msg->data.process_timestamp;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg3;

      /* arg4 */
      msg->data.process_name = g_new0 (gchar, msg->hdr.size_of_arg4);
      iov[iov_index].iov_base = msg->data.process_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg4;

      /* arg5 */
      msg->data.thread_name = g_new0 (gchar, msg->hdr.size_of_arg5);
      iov[iov_index].iov_base = msg->data.thread_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg5;
      break;

    case CDM_MESSAGE_COREDUMP_UPDATE:
      /* arg1 */
      msg->data.process_crash_id = g_new0 (gchar, msg->hdr.size_of_arg1);
      iov[iov_index].iov_base = msg->data.process_crash_id;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      msg->data.process_vector_id = g_new0 (gchar, msg->hdr.size_of_arg2);
      iov[iov_index].iov_base = msg->data.process_vector_id;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;

      /* arg3 */
      msg->data.process_context_id = g_new0 (gchar, msg->hdr.size_of_arg3);
      iov[iov_index].iov_base = msg->data.process_context_id;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg3;
      break;

    case CDM_MESSAGE_COREDUMP_SUCCESS:
      /* arg1 */
      msg->data.coredump_file_path = g_new0 (gchar, msg->hdr.size_of_arg1);
      iov[iov_index].iov_base = msg->data.coredump_file_path;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      msg->data.context_name = g_new0 (gchar, msg->hdr.size_of_arg2);
      iov[iov_index].iov_base = msg->data.context_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;

      /* arg3 */
      msg->data.lifecycle_state = g_new0 (gchar, msg->hdr.size_of_arg3);
      iov[iov_index].iov_base = msg->data.lifecycle_state;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg3;
      break;

    case CDM_MESSAGE_COREDUMP_CONTEXT:
      /* arg1 */
      msg->data.context_name = g_new0 (gchar, msg->hdr.size_of_arg1);
      iov[iov_index].iov_base = msg->data.context_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      msg->data.lifecycle_state = g_new0 (gchar, msg->hdr.size_of_arg2);
      iov[iov_index].iov_base = msg->data.lifecycle_state;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;
      break;

    case CDM_MESSAGE_EPILOG_FRAME_INFO:
      /* arg1 */
      iov[iov_index].iov_base = &msg->data.epilog_frame_count;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;
      break;

    case CDM_MESSAGE_EPILOG_FRAME_DATA:
      /* arg1 */
      msg->data.epilog_frame_data = g_new0 (gchar, msg->hdr.size_of_arg1);
      iov[iov_index].iov_base = msg->data.epilog_frame_data;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;
      break;

    default:
      break;
    }

  /* read into the message structure */
  if (iov_index > 0)
    if ((sz = readv (fd, iov, iov_index)) <= 0)
      return CDM_STATUS_ERROR;

  return CDM_STATUS_OK;
}

CdmStatus
cdm_message_write (gint fd, CdmMessage *msg)
{
  struct iovec iov[CDM_MESSAGE_IOVEC_MAX_ARRAY] = {};
  gint iov_index = 0;
  gssize sz;

  g_assert (msg);

  /* set arguments size */
  switch (cdm_message_get_type (msg))
    {
    case CDM_MESSAGE_COREDUMP_NEW:
      msg->hdr.size_of_arg1 = sizeof (msg->data.process_pid);
      msg->hdr.size_of_arg2 = sizeof (msg->data.process_exit_signal);
      msg->hdr.size_of_arg3 = sizeof (msg->data.process_timestamp);
      msg->hdr.size_of_arg4
          = (uint16_t)(sizeof (gchar)
                       * strnlen (msg->data.process_name, CDM_MESSAGE_PROCNAME_MAX_LEN + 1));
      msg->hdr.size_of_arg5
          = (uint16_t)(sizeof (gchar)
                       * strnlen (msg->data.thread_name, CDM_MESSAGE_THREDNAME_MAX_LEN + 1));
      break;

    case CDM_MESSAGE_COREDUMP_UPDATE:
      msg->hdr.size_of_arg1
          = (uint16_t)(sizeof (gchar)
                       * strnlen (msg->data.process_crash_id, CDM_MESSAGE_CRASHID_MAX_LEN + 1));
      msg->hdr.size_of_arg2
          = (uint16_t)(sizeof (gchar)
                       * strnlen (msg->data.process_vector_id, CDM_MESSAGE_CRASHID_MAX_LEN + 1));
      msg->hdr.size_of_arg3
          = (uint16_t)(sizeof (gchar)
                       * strnlen (msg->data.process_context_id, CDM_MESSAGE_CRASHID_MAX_LEN + 1));
      break;

    case CDM_MESSAGE_COREDUMP_SUCCESS:
      msg->hdr.size_of_arg1
          = (uint16_t)(sizeof (gchar)
                       * strnlen (msg->data.coredump_file_path, CDM_MESSAGE_FILENAME_MAX_LEN + 1));
      msg->hdr.size_of_arg2
          = (uint16_t)(sizeof (gchar)
                       * strnlen (msg->data.context_name, CDM_MESSAGE_CTXNAME_MAX_LEN + 1));
      msg->hdr.size_of_arg3
          = (uint16_t)(sizeof (gchar)
                       * strnlen (msg->data.lifecycle_state, CDM_MESSAGE_LCSTATE_MAX_LEN + 1));
      break;

    case CDM_MESSAGE_COREDUMP_CONTEXT:
      msg->hdr.size_of_arg1
          = (uint16_t)(sizeof (gchar)
                       * strnlen (msg->data.context_name, CDM_MESSAGE_CTXNAME_MAX_LEN + 1));
      msg->hdr.size_of_arg2
          = (uint16_t)(sizeof (gchar)
                       * strnlen (msg->data.lifecycle_state, CDM_MESSAGE_LCSTATE_MAX_LEN + 1));
      break;

    case CDM_MESSAGE_EPILOG_FRAME_INFO:
      msg->hdr.size_of_arg1 = sizeof (msg->data.epilog_frame_count);
      break;

    case CDM_MESSAGE_EPILOG_FRAME_DATA:
      msg->hdr.size_of_arg1 = (uint16_t)(sizeof (gchar)
                                         * strnlen (msg->data.epilog_frame_data,
                                                    CDM_MESSAGE_EPILOG_FRAME_MAX_LEN + 1));
      break;

    default:
      break;
    }

  /* set message header segments */
  iov[iov_index].iov_base = &msg->hdr.hsh;
  iov[iov_index++].iov_len = sizeof (msg->hdr.hsh);
  iov[iov_index].iov_base = &msg->hdr.session;
  iov[iov_index++].iov_len = sizeof (msg->hdr.session);
  iov[iov_index].iov_base = &msg->hdr.version;
  iov[iov_index++].iov_len = sizeof (msg->hdr.version);
  iov[iov_index].iov_base = &msg->hdr.type;
  iov[iov_index++].iov_len = sizeof (msg->hdr.type);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg1;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg1);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg2;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg2);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg3;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg3);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg4;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg4);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg5;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg5);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg6;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg6);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg7;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg7);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg8;
  iov[iov_index++].iov_len = sizeof (msg->hdr.size_of_arg8);

  if ((sz = writev (fd, iov, iov_index)) <= 0)
    return CDM_STATUS_ERROR;

  memset (iov, 0, sizeof (iov));
  iov_index = 0;

  /* set message data segments */
  switch (cdm_message_get_type (msg))
    {
    case CDM_MESSAGE_COREDUMP_NEW:
      /* arg1 */
      iov[iov_index].iov_base = &msg->data.process_pid;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      iov[iov_index].iov_base = &msg->data.process_exit_signal;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;

      /* arg3 */
      iov[iov_index].iov_base = &msg->data.process_timestamp;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg3;

      /* arg4 */
      iov[iov_index].iov_base = msg->data.process_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg4;

      /* arg5 */
      iov[iov_index].iov_base = msg->data.thread_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg5;
      break;

    case CDM_MESSAGE_COREDUMP_UPDATE:
      /* arg1 */
      iov[iov_index].iov_base = msg->data.process_crash_id;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      iov[iov_index].iov_base = msg->data.process_vector_id;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;

      /* arg3 */
      iov[iov_index].iov_base = msg->data.process_context_id;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg3;
      break;

    case CDM_MESSAGE_COREDUMP_SUCCESS:
      /* arg1 */
      iov[iov_index].iov_base = msg->data.coredump_file_path;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      iov[iov_index].iov_base = msg->data.context_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;

      /* arg3 */
      iov[iov_index].iov_base = msg->data.lifecycle_state;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg3;
      break;

    case CDM_MESSAGE_COREDUMP_CONTEXT:
      /* arg1 */
      iov[iov_index].iov_base = msg->data.context_name;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      iov[iov_index].iov_base = msg->data.lifecycle_state;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;
      break;

    case CDM_MESSAGE_EPILOG_FRAME_INFO:
      /* arg1 */
      iov[iov_index].iov_base = &msg->data.epilog_frame_count;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;
      break;

    case CDM_MESSAGE_EPILOG_FRAME_DATA:
      /* arg1 */
      iov[iov_index].iov_base = msg->data.epilog_frame_data;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;
      break;

    default:
      break;
    }

  /* write into the message structure */
  if (iov_index > 0)
    if ((sz = writev (fd, iov, iov_index)) <= 0)
      return CDM_STATUS_ERROR;

  return CDM_STATUS_OK;
}
