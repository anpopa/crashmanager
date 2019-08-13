/* cdm-types.h
 *
 * Copyright 2019 Alin Popa <alin.popa@fxdata.ro>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written
 * authorization.
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
