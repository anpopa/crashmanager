/* cdi-application.h
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

#include "cdm-defaults.h"
#include "cdm-types.h"
#include "cdm-logging.h"
#include "cdm-options.h"
#include "cdi-archive.h"
#include "cdi-journal.h"

#include <glib.h>
#include <stdlib.h>

G_BEGIN_DECLS

/**
 * @struct CdiApplication
 * @brief Crashmanager application object referencing main objects
 */
typedef struct _CdiApplication {
  CdmOptions *options;
  CdiJournal *journal;
  grefcount rc;           /**< Reference counter variable  */
} CdiApplication;

/**
 * @brief Create a new CdiApplication object
 * @param config Full path to the configuration file
 */
CdiApplication *cdi_application_new (const gchar *config);

/**
 * @brief Aquire CdiApplication object
 * @param app The object to aquire
 * @return The aquiered app object
 */
CdiApplication *cdi_application_ref (CdiApplication *app);

/**
 * @brief Release a CdiApplication object
 * @param app The cdi application object to release
 */
void cdi_application_unref (CdiApplication *app);

/**
 * @brief List crash entries
 * @param app The cdi application
 */
void cdi_application_list_entries (CdiApplication *app);

/**
 * @brief List crash archive content
 * @param app The cdi application
 * @param fpath Input file path
 */
void cdi_application_list_content (CdiApplication *app, const gchar *fpath);

/**
 * @brief List crash archive content
 * @param app The cdi application
 * @param fpath Input file path
 */
void cdi_application_print_info (CdiApplication *app, const gchar *fpath);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdiApplication, cdi_application_unref);

G_END_DECLS
