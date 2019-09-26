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
 * \file cdm-types.h
 */

#pragma once

#include "cdm-message-type.h"

#include <glib.h>
#include <elf.h>

G_BEGIN_DECLS

#ifndef CDM_UNUSED
#define CDM_UNUSED(x) (void)(x)
#endif

#ifndef ARCHIVE_NAME_PATTERN
#define ARCHIVE_NAME_PATTERN "%s/%s.%ld.%lu.cdh.tar.gz"
#endif

#define CDM_EVENT_SOURCE(x) (GSource *)(x)

enum { CID_RETURN_ADDRESS = 1 << 0, CID_IP_FILE_OFFSET = 1 << 1, CID_RA_FILE_OFFSET = 1 << 2 };

typedef enum _CdmStatus {
  CDM_STATUS_ERROR = -1,
  CDM_STATUS_OK
} CdmStatus;

typedef struct _CdmRegisters {
#ifdef __aarch64__
  uint64_t pc;
  uint64_t lr;
#elif __x86_64__
  uint64_t rip;
  uint64_t rbp;
#else
  static_assert (false, "Don't know whow to handle this arhitecture");
#endif
} CdmRegisters;

G_END_DECLS
