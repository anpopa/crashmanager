/* cdh-info.h
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

#ifndef CDH_INFO_H
#define CDH_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cdm-types.h"

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/queue.h>
#include <sys/types.h>

#ifndef CDH_INFO_ENTRY_FILENAME_LEN
#define CDH_INFO_ENTRY_FILENAME_LEN 192
#endif

#ifndef CDH_INFO_PROC_NAME_LEN
#define CDH_INFO_PROC_NAME_LEN 64
#endif

/**
 * @struct cdh_info
 * @brief The info object
 */
typedef struct _CdhInfo {
  gchar *name;       /**< process name */
  gchar *tname;      /**< thread name  */
  guint64 tstamp;    /**< crash timestamp */
  gint64 sig;        /**< signal id */
  gint64 pid;        /**< process id as seen on host */
  gint64 cpid;       /**< process id as seen on namespace */

  gchar *contextid;  /**< namespace context for the crashed pid */
  gchar *crashid;    /**< crash id value */
  gchar *vectorid;   /**< crash course id value */
  gboolean onhost;   /**< true if the crash is in host context */
} CdhInfo;

/**
 * @brief Create a new cdh_info object
 * @return A pointer to the new cdh_info object or NULL on error
 */
CdhInfo *cdh_info_new (void);

/**
 * @brief Release a cdh_info object
 * @param i Pointer to the cdh_info object
 */
void cdh_info_free (CdhInfo *i);

#ifdef __cplusplus
}
#endif

#endif /* CDH_INFO_H */
