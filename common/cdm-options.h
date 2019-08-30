/* cdm-options.h
 *
 * Copyright 2019 Alin Popa <alin.popa@bmw.de>
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
 * @enum CdmOptionsKey
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
 * @struct CdmOptions
 */
typedef struct _CdmOptions {
  GKeyFile *conf;   /**< The GKeyFile object */
  bool has_conf;      /**< Flag to check if a runtime option object is available */
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
