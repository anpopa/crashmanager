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
 * \file cdm-application.h
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
 * @brief CdmApplication application object referencing main objects
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
  grefcount rc;
} CdmApplication;

/**
 * @brief Create a new CdmApplication object
 * @param config Full path to the configuration file
 * @param error An error object must be provided. If an error occurs during
 * initialization the error is reported and application should not use the
 * returned object. If the error is set the object is invalid and needs to be
 * released.
 */
CdmApplication *cdm_application_new (const gchar *config, GError **error);

/**
 * @brief Aquire CdmApplication object
 * @param app The object to aquire
 * @return The aquiered CdmApplication object
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
 * @return If run was succesful CDM_STATUS_OK is returned
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
