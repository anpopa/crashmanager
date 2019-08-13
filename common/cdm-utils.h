/* cdm-utils.h
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

#include "cdm-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief Get process name for pid
 * @param pid Process ID to lookup for
 * @return new allocated string with proc name or NULL if not found
 */
gchar *cdm_utils_get_procname (gint64 pid);

/**
 * @brief Get process name for pid
 * @return const string with current os version
 */
const gchar *cdm_utils_get_osversion (void);

/**
 * @brief Calculate the jankins hash from a string
 * @param key The input string
 * @return The long unsigned int as hash
 */
guint64 cdm_utils_jenkins_hash (const gchar *key);

/**
 * @brief Get file size
 * @param file_path The path to the file
 * @return The long int as file size or -1 on error
 */
gint64 cdm_utils_get_filesize (const gchar *file_path);

/**
 * @brief Get pid for process by name
 *
 * Note that this function only looks for pid by name once
 * Will not provide the information if multiple instances are running
 * Should be used only as info to check if a particular process has at least an instance running
 *
 * @param exepath Path to process executable
 *
 * @return Pid value if found, -1 ottherwise
 */
pid_t cdm_utils_first_pid_for_process (const gchar *exepath);

/**
 * @brief Change owner for a filesystem entry
 *
 * @param file_path Filesystem entry to work on
 * @param user_name The new owner user name
 * @param group_name The new owner group name
 *
 * @return CDM_STATUS_ERROR on failure
 *         CDM_STATUS_OK in success
 */
CdmStatus cdm_utils_chown (const gchar *file_path, const gchar *user_name, const gchar *group_name);

G_END_DECLS
