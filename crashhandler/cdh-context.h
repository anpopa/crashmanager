/* cdh-context.h
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

#ifndef CDH_CONTEXT_H
#define CDH_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cdh-data.h"
#include <glib.h>

/**
 * @brief Get process name for pid
 *
 * @param pid Process ID to lookup for
 * @param exec_name String buffer to write the name into
 * @param exec_name_maxsize Maximum size to write into exec_name buffer (truncated if proc name is
 * longer)
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_context_get_procname(pid_t pid, gchar *exec_name, gsize exec_name_maxsize);

/**
 * @brief Generate context data available pre coredump stream
 * @param d Global CDData
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_context_generate_prestream(CdhData *d);

/**
 * @brief Generate context data available post coredump stream
 * @param d Global CDData
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_context_generate_poststream(CdhData *d);

#ifdef __cplusplus
}
#endif

#endif /* CDH_CONTEXT_H */
