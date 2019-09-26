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
