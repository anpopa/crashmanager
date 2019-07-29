/* cdmypes.h
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

#ifndef CD_TYPES_H
#define CD_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cd_message_type.h"
#include <elf.h>

#ifndef CD_UNUSED
#define CD_UNUSED(x) (void)(x)
#endif

#ifndef CD_PATH_MAX
#define CD_PATH_MAX (1024)
#endif

#ifndef CORE_MAX_FILENAME_LENGTH
#define CORE_MAX_FILENAME_LENGTH CD_MESSAGE_FILENAME_LEN
#endif

#ifndef MAX_PROC_NAME_LENGTH
#define MAX_PROC_NAME_LENGTH CD_MESSAGE_PROCNAME_LEN
#endif

#ifndef CRASH_ID_LEN
#define CRASH_ID_LEN CD_CRASHID_LEN
#endif

#ifndef CRASH_CONTEXT_LEN
#define CRASH_CONTEXT_LEN CD_CRASHCONTEXT_LEN
#endif

#ifndef ARCHIVE_NAME_PATTERN
#define ARCHIVE_NAME_PATTERN "%s/core_%s_%d_%lu.cdh.tar.gz"
#endif

enum { CID_RETURN_ADDRESS = 1 << 0, CID_IP_FILE_OFFSET = 1 << 1, CID_RA_FILE_OFFSET = 1 << 2 };

typedef enum _CDStatus {
  CD_STATUS_ERROR = -1,
  CD_STATUS_OK
} CDStatus;

typedef struct _CDRegisters {
#ifdef __aarch64__
    uint64 pc;
    uint64 lr;
#elif __x86_64__
    uint64 rip;
    uint64 rbp;
#else
    static_assert(false, "Don't know whow to handle this arhitecture");
#endif
} CDRegisters;

#ifdef __cplusplus
}
#endif

#endif /* CD_TYPES_H */