/* cdh-application.h
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

#include "cdm-defaults.h"
#include "cdm-types.h"
#include "cdm-logging.h"
#include "cdh-archive.h"
#include "cdh-context.h"
#include "cdm-options.h"
#include "cdh-coredump.h"
#if defined(WITH_CRASHMANAGER)
#include "cdh-manager.h"
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The global cdh app object referencing main submodules and states
 */
typedef struct _CdhApplication {
  CdmOptions *options;       /**< Global options */
  CdhContext *context;       /**< Crash info app */
  CdhCoredump *coredump;     /**< Crash info app */
  CdhArchive *archive;       /**< coredump archive streamer */
#if defined(WITH_CRASHMANAGER)
  CdhManager *manager;       /**< manager ipc object */
#endif
  grefcount rc;              /**< Reference counter variable */
} CdhApplication;

/**
 * @brief Create a new CdhApplication object
 * @param config_path Full path to the cdh configuration fole
 */
CdhApplication *cdh_application_new (const gchar *config_path);

/**
 * @brief Aquire CdhApplication object
 * @param app The object to aquire
 * @return The aquiered app object
 */
CdhApplication *cdh_application_ref (CdhApplication *app);

/**
 * @brief Release a app object
 * @param app The cdh app object to release
 */
void cdh_application_unref (CdhApplication *app);

/**
 * @brief Execute cdh logic
 *
 * @param d The cdh object to deinitialize
 * @param argc Main arguments count
 * @param argv Main arguments table
 *
 * @return If run was succesful CDH_OK is returned
 */
CdmStatus cdh_application_execute (CdhApplication *app, gint argc, gchar *argv[]);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdhApplication, cdh_application_unref);

G_END_DECLS
