/*
 * SPDX license identifier: MIT
 *
 * Copyright (c) 2020 Alin Popa
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \file cdh-elogmsg.c
 */

#include "cdh-elogmsg.h"

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/uio.h>

#define CDM_ELOGMSG_IOVEC_MAX_ARRAY (16)

CdmELogMessage *
cdm_elogmsg_new (CdmELogMessageType type)
{
  CdmELogMessage *msg = calloc (1, sizeof(CdmELogMessage));

  assert (msg);

  msg->hdr.type = type;
  msg->hdr.hsh = CDM_ELOGMSG_START_HASH;
  msg->hdr.version = CDM_ELOGMSG_PROTOCOL_VERSION;

  return msg;
}

void
cdm_elogmsg_free (CdmELogMessage *msg)
{
  assert (msg);
  /* make sure release any data if needed */
  free (msg);
}

bool
cdm_elogmsg_is_valid (CdmELogMessage *msg)
{
  assert (msg);

  if (msg->hdr.hsh != CDM_ELOGMSG_START_HASH || msg->hdr.version != CDM_ELOGMSG_PROTOCOL_VERSION)
    return false;

  return true;
}

CdmELogMessageType
cdm_elogmsg_get_type (CdmELogMessage *msg)
{
  assert (msg);
  return msg->hdr.type;
}


void
cdm_elogmsg_set_process_pid (CdmELogMessage *msg, int64_t pid)
{
  assert (msg);
  msg->data.process_pid = pid;
}

int64_t
cdm_elogmsg_get_process_pid (CdmELogMessage *msg)
{
  assert (msg);
  return msg->data.process_pid;
}

void
cdm_elogmsg_set_process_exit_signal (CdmELogMessage *msg, int64_t sig)
{
  assert (msg);
  msg->data.process_sig = sig;
}

int64_t
cdm_elogmsg_get_process_exit_signal (CdmELogMessage *msg)
{
  assert (msg);
  return msg->data.process_sig;
}

int
cdm_elogmsg_read (int fd, CdmELogMessage *msg)
{
  struct iovec iov[CDM_ELOGMSG_IOVEC_MAX_ARRAY] = {};
  int iov_index = 0;
  ssize_t sz;

  assert (msg);

  /* set message header segments */
  iov[iov_index].iov_base = &msg->hdr.hsh;
  iov[iov_index++].iov_len = sizeof(msg->hdr.hsh);
  iov[iov_index].iov_base = &msg->hdr.version;
  iov[iov_index++].iov_len = sizeof(msg->hdr.version);
  iov[iov_index].iov_base = &msg->hdr.type;
  iov[iov_index++].iov_len = sizeof(msg->hdr.type);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg1;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg1);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg2;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg2);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg3;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg3);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg4;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg4);

  if ((sz = readv (fd, iov, iov_index)) <= 0)
    return -1;

  memset (iov, 0, sizeof(iov));
  iov_index = 0;

  /* set message data segments */
  switch (cdm_elogmsg_get_type (msg))
    {
    case CDM_ELOGMSG_NEW:
      /* arg1 */
      iov[iov_index].iov_base = &msg->data.process_pid;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      iov[iov_index].iov_base = &msg->data.process_sig;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;
      break;

    default:
      return -1;
    }

  /* read into the message structure */
  if (iov_index > 0)
    if ((sz = readv (fd, iov, iov_index)) <= 0)
      return -1;

  return 0;
}

int
cdm_elogmsg_write (int fd, CdmELogMessage *msg)
{
  struct iovec iov[CDM_ELOGMSG_IOVEC_MAX_ARRAY] = {};
  int iov_index = 0;
  ssize_t sz;

  assert (msg);

  switch (cdm_elogmsg_get_type (msg))
    {
    case CDM_ELOGMSG_NEW:
      msg->hdr.size_of_arg1 = sizeof(msg->data.process_pid);
      msg->hdr.size_of_arg2 = sizeof(msg->data.process_sig);
      break;

    default:
      break;
    }

  /* set message header segments */
  iov[iov_index].iov_base = &msg->hdr.hsh;
  iov[iov_index++].iov_len = sizeof(msg->hdr.hsh);
  iov[iov_index].iov_base = &msg->hdr.version;
  iov[iov_index++].iov_len = sizeof(msg->hdr.version);
  iov[iov_index].iov_base = &msg->hdr.type;
  iov[iov_index++].iov_len = sizeof(msg->hdr.type);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg1;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg1);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg2;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg2);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg3;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg3);
  iov[iov_index].iov_base = &msg->hdr.size_of_arg4;
  iov[iov_index++].iov_len = sizeof(msg->hdr.size_of_arg4);

  if ((sz = writev (fd, iov, iov_index)) <= 0)
    return -1;

  memset (iov, 0, sizeof(iov));
  iov_index = 0;

  /* set message data segments */
  switch (cdm_elogmsg_get_type (msg))
    {
    case CDM_ELOGMSG_NEW:
      /* arg1 */
      iov[iov_index].iov_base = &msg->data.process_pid;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg1;

      /* arg2 */
      iov[iov_index].iov_base = &msg->data.process_sig;
      iov[iov_index++].iov_len = msg->hdr.size_of_arg2;
      break;

    default:
      return -1;
    }

  /* write into the message structure */
  if (iov_index > 0)
    if ((sz = writev (fd, iov, iov_index)) <= 0)
      return -1;

  return 0;
}
