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
