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
 * \file cdm-message.c
 */

#include "cdm-message.h"

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void
cdm_message_init (CdmMessage *m,
                  CdmMessageType type,
                  uint16_t session)
{
  g_assert (m);

  memset (m, 0, sizeof(CdmMessage));
  m->hdr.type = type;
  m->hdr.hsh = CDM_MESSAGE_START_HASH;
  m->hdr.session = session;
}

void
cdm_message_set_data (CdmMessage *m,
                      void *data,
                      uint32_t size)
{
  g_assert (m);
  g_assert (data);

  m->hdr.data_size = size;
  m->data = data;
}

void
cdm_message_free_data (CdmMessage *m)
{
  g_assert (m);

  if (m->data != NULL)
    free (m->data);
}

bool
cdm_message_is_valid (CdmMessage *m)
{
  if (m == NULL)
    return false;

  if (m->hdr.hsh != CDM_MESSAGE_START_HASH)
    return false;

  return true;
}

CdmMessageType
cdm_message_get_type (CdmMessage *m)
{
  g_assert (m);
  return m->hdr.type;
}

CdmStatus
cdm_message_set_version (CdmMessage *m,
                         const gchar *version)
{
  g_assert (m);
  snprintf ((gchar*)m->hdr.version, CDM_VERSION_STRING_LEN, "%s", version);
  return CDM_STATUS_OK;
}

CdmStatus
cdm_message_read (gint fd,
                  CdmMessage *m)
{
  gssize sz;

  g_assert (m);

  sz = read (fd, &m->hdr, sizeof(CdmMessageHdr));
  if (sz != sizeof(CdmMessageHdr) || !cdm_message_is_valid (m))
    return CDM_STATUS_ERROR;

  m->data = calloc (1, m->hdr.data_size);
  if (m->data == NULL)
    return CDM_STATUS_ERROR;

  sz = read (fd, m->data, m->hdr.data_size);

  return sz == m->hdr.data_size ? CDM_STATUS_OK : CDM_STATUS_ERROR;
}

CdmStatus
cdm_message_write (gint fd,
                   CdmMessage *m)
{
  gssize sz;

  g_assert (m);

  if (!cdm_message_is_valid (m))
    return CDM_STATUS_ERROR;

  sz = write (fd, &m->hdr, sizeof(CdmMessageHdr));
  if (sz != sizeof(CdmMessageHdr))
    return CDM_STATUS_ERROR;

  sz = write (fd, m->data, m->hdr.data_size);

  return sz == m->hdr.data_size ? CDM_STATUS_OK : CDM_STATUS_ERROR;
}
