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
 * \file cdh-main.c
 */

#include "cdh-application.h"

#include <glib.h>
#include <stdlib.h>
#ifdef WITH_DEBUG_ATTACH
#include <signal.h>
#endif

gint
main (gint argc, gchar *argv[])
{
  g_autofree gchar *conf_path = NULL;
  CdmStatus status = CDM_STATUS_OK;

#ifdef WITH_DEBUG_ATTACH
  raise (SIGSTOP);
#endif

  cdm_logging_open ("CDH", "Crashhandler instance", "CDH", "Default context");

  conf_path = g_build_filename (CDM_CONFIG_DIRECTORY, CDM_CONFIG_FILE_NAME, NULL);
  if (g_access (conf_path, R_OK) == 0)
    {
      g_autoptr (CdhApplication) app = cdh_application_new (conf_path);
      status = cdh_application_execute (app, argc, argv);
    }
  else
    {
      status = CDM_STATUS_ERROR;
    }

  cdm_logging_close ();

  return status == CDM_STATUS_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
