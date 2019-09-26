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
 * \file cdm-options.h
 */

#pragma once

#include "cdm-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @enum Option keys
 */
typedef enum _CdmOptionsKey {
  KEY_USER_NAME,
  KEY_GROUP_NAME,
  KEY_CRASHDUMP_DIR,
  KEY_FILESYSTEM_MIN_SIZE,
  KEY_ELEVATED_NICE_VALUE,
  KEY_RUN_DIR,
  KEY_DATABASE_FILE,
  KEY_KDUMPSOURCE_DIR,
  KEY_CRASHDUMP_DIR_MIN_SIZE,
  KEY_CRASHDUMP_DIR_MAX_SIZE,
  KEY_CRASHFILES_MAX_COUNT,
  KEY_IPC_SOCK_ADDR,
  KEY_IPC_TIMEOUT_SEC
} CdmOptionsKey;

/**
 * @struct Option object
 */
typedef struct _CdmOptions {
  GKeyFile *conf;   /**< The GKeyFile object */
  bool has_conf;    /**< Flag to check if a runtime option object is available */
  grefcount rc;     /**< Reference counter variable  */
} CdmOptions;

/*
 * @brief Create a new options object
 */
CdmOptions *cdm_options_new (const gchar *conf_path);

/*
 * @brief Aquire options object
 */
CdmOptions *cdm_options_ref (CdmOptions *opts);

/**
 * @brief Release an options object
 */
void cdm_options_unref (CdmOptions *opts);

/**
 * @brief Get the GKeyFile object
 */
GKeyFile *cdm_options_get_key_file (CdmOptions *opts);

/*
 * @brief Get a configuration value string for key
 */
gchar *cdm_options_string_for (CdmOptions *opts, CdmOptionsKey key);

/*
 * @brief Get a configuration gint64 value for key
 */
gint64 cdm_options_long_for (CdmOptions *opts, CdmOptionsKey key);

G_END_DECLS
