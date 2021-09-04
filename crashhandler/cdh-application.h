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
 * \file cdh-application.h
 */

#pragma once

#include "cdh-archive.h"
#include "cdh-context.h"
#include "cdh-coredump.h"
#include "cdm-defaults.h"
#include "cdm-logging.h"
#include "cdm-options.h"
#include "cdm-types.h"
#if defined(WITH_CRASHMANAGER)
#include "cdh-manager.h"
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The global cdh app object referencing main submodules and states
 */
typedef struct _CdhApplication {
    CdmOptions *options;   /**< Global options */
    CdhContext *context;   /**< Crash info app */
    CdhCoredump *coredump; /**< Crash info app */
    CdhArchive *archive;   /**< coredump archive streamer */
#if defined(WITH_CRASHMANAGER)
    CdhManager *manager; /**< manager ipc object */
#endif
    grefcount rc; /**< Reference counter variable */
} CdhApplication;

/**
 * @brief Create a new CdhApplication object
 * @param config_path Full path to the cdh configuration fole
 */
CdhApplication *cdh_application_new(const gchar *config_path);

/**
 * @brief Aquire CdhApplication object
 * @param app The object to aquire
 * @return The aquiered app object
 */
CdhApplication *cdh_application_ref(CdhApplication *app);

/**
 * @brief Release a app object
 * @param app The cdh app object to release
 */
void cdh_application_unref(CdhApplication *app);

/**
 * @brief Execute cdh logic
 * @param d The cdh object to deinitialize
 * @param argc Main arguments count
 * @param argv Main arguments table
 * @return If run was succesful CDH_OK is returned
 */
CdmStatus cdh_application_execute(CdhApplication *app, gint argc, gchar *argv[]);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdhApplication, cdh_application_unref);

G_END_DECLS
