/* cdm-application.h
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
#include "cdm-server.h"
#include "cdm-janitor.h"
#include "cdm-journal.h"
#include "cdm-transfer.h"
#ifdef WITH_SYSTEMD
#include "cdm-sdnotify.h"
#endif
#ifdef WITH_GENIVI_NSM
#include "cdm-lifecycle.h"
#endif

#include <glib.h>
#include <stdlib.h>

G_BEGIN_DECLS

/**
 * @struct CdmApplication
 * @brief Crashmanager application object referencing main objects
 */
typedef struct _CdmApplication {
  CdmOptions *options;
  CdmServer *server;
  CdmJanitor *janitor;
  CdmJournal *journal;
#ifdef WITH_SYSTEMD
  CdmSDNotify *sdnotify;
#endif
#ifdef WITH_GENIVI_NSM
  CdmLifecycle *lifecycle;
#endif
  CdmTransfer *transfer;
  GMainLoop *mainloop;
  grefcount rc;           /**< Reference counter variable  */
} CdmApplication;

/**
 * @brief Create a new CdmApplication object
 * @param config Full path to the configuration file
 */
CdmApplication *cdm_application_new (const gchar *config);

/**
 * @brief Aquire CdmApplication object
 * @param app The object to aquire
 * @return The aquiered app object
 */
CdmApplication *cdm_application_ref (CdmApplication *app);

/**
 * @brief Release a CdmApplication object
 * @param app The cdm application object to release
 */
void cdm_application_unref (CdmApplication *app);

/**
 * @brief Execute cdm application
 * @param app The cdm application object
 * @return If run was succesful CDH_OK is returned
 */
CdmStatus cdm_application_execute (CdmApplication *app);

/**
 * @brief Get main event loop reference
 * @param app The cdm application object
 * @return A pointer to the main event loop
 */
GMainLoop *cdm_application_get_mainloop (CdmApplication *app);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmApplication, cdm_application_unref);

G_END_DECLS
