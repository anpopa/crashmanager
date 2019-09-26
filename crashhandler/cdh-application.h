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
 * \file cdh-application.h
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
