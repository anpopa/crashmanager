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
 * \file cdi-application.h
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
 * @brief Crashinfo application object referencing main objects
 */
typedef struct _CdiApplication {
  CdmOptions *options;
  CdiJournal *journal;
  grefcount rc;
} CdiApplication;

/**
 * @brief Create a new CdiApplication object
 * @param config Full path to the configuration file
 * @param error An error object must be provided. If an error occurs during
 * initialization the error is reported and application should not use the
 * returned object. If the error is set the object is invalid and needs to be
 * released.
 */
CdiApplication *cdi_application_new (const gchar *config, GError **error);

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

/**
 * @brief Extract coredump file
 * @param app The cdi application
 * @param fpath Input file path
 */
void cdi_application_extract_coredump (CdiApplication *app, const gchar *fpath);

/**
 * @brief Print content of a file in the archive
 * @param app The cdi application
 * @param fname Input file name
 * @param fpath Input file path
 */
void cdi_application_print_file (CdiApplication *app, const gchar *fname, const gchar *fpath);

/**
 * @brief Print backtrace
 * @param app The cdi application
 * @param all If true print backtrace for all threads
 * @param fpath Input file path
 */
void cdi_application_print_backtrace (CdiApplication *app, gboolean all, const gchar *fpath);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdiApplication, cdi_application_unref);

G_END_DECLS
