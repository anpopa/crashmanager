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
 * \file cdm-utils.h
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
 * @brief Get process exe path for pid
 * @param pid Process ID to lookup for
 * @return new allocated string with proc exe or NULL if not found
 */
gchar *cdm_utils_get_procexe (gint64 pid);

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
 * Note that this function only looks for pid by name once
 * Will not provide the information if multiple instances are running
 * Should be used only as info to check if a particular process
 * has at least an instance running
 * @param exepath Path to process executable
 * @return Pid value if found, -1 ottherwise
 */
pid_t cdm_utils_first_pid_for_process (const gchar *exepath);

/**
 * @brief Change owner for a filesystem entry
 * @param file_path Filesystem entry to work on
 * @param user_name The new owner user name
 * @param group_name The new owner group name
 * @return CDM_STATUS_ERROR on failure, CDM_STATUS_OK in success
 */
CdmStatus cdm_utils_chown (const gchar *file_path, const gchar *user_name, const gchar *group_name);

G_END_DECLS
