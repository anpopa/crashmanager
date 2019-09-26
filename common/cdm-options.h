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
