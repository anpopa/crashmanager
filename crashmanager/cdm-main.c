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
 * \file cdm-main.c
 */

#include "cdm-application.h"
#include "cdm-utils.h"

#include <glib.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

static GMainLoop *g_mainloop = NULL;

static void
terminate (int signum)
{
  g_info ("Crashmanager terminate with signal %d", signum);
  if (g_mainloop != NULL)
    g_main_loop_quit (g_mainloop);
}

gint
main (gint argc, gchar *argv[])
{
  g_autoptr (GOptionContext) context = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (CdmApplication) app = NULL;
  g_autofree gchar *config_path = NULL;
  gboolean version = FALSE;
  CdmStatus status = CDM_STATUS_OK;

  GOptionEntry main_entries[] = {
    { "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Show program version", "" },
    { "config", 'c', 0, G_OPTION_ARG_FILENAME, &config_path, "Override configuration file", "" },
    { 0 }
  };

  signal (SIGINT, terminate);
  signal (SIGTERM, terminate);

  context = g_option_context_new ("- Crash manager service daemon");
  g_option_context_set_summary (context,
                                "The service listen for Crashhandler events and manage its output");
  g_option_context_add_main_entries (context, main_entries, NULL);

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      return EXIT_FAILURE;
    }

  if (version)
    {
      g_printerr ("%s\n", CDM_VERSION);
      return EXIT_SUCCESS;
    }

  cdm_logging_open ("CDM", "Crashmanager service", "CDM", "Default context");

  if (config_path == NULL)
    config_path = g_build_filename (CDM_CONFIG_DIRECTORY, CDM_CONFIG_FILE_NAME, NULL);

  if (g_access (config_path, R_OK) == 0)
    {
      app = cdm_application_new (config_path, &error);
      if (error != NULL)
        {
          g_printerr ("%s\n", error->message);
          status = CDM_STATUS_ERROR;
        }
      else
        {
          g_info ("Crashmanager service started for OS version '%s'", cdm_utils_get_osversion ());
          g_mainloop = cdm_application_get_mainloop (app);
          status = cdm_application_execute (app);
        }
    }
  else
    g_warning ("Cannot open configuration file %s", config_path);

  cdm_logging_close ();

  return status == CDM_STATUS_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
