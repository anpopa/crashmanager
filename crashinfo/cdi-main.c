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
 * \file cdi-main.c
 */

#include "cdi-application.h"
#include "cdm-utils.h"

#include <glib.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

gint
main (gint argc, gchar *argv[])
{
  g_autoptr (GOptionContext) context = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (CdiApplication) app = NULL;
  g_autofree gchar *config_path = NULL;
  g_autofree gchar *print_file = NULL;
  gboolean version = FALSE;
  gboolean list_entries = FALSE;
  gboolean list_files = FALSE;
  gboolean print_info = FALSE;
  gboolean extract = FALSE;
  gboolean print_bt = FALSE;
  gboolean print_btall = FALSE;
  CdmStatus status = CDM_STATUS_OK;

  GOptionEntry main_entries[] = {
    { "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Show program version", "" },
    { "config", 'c', 0, G_OPTION_ARG_FILENAME, &config_path, "Override configuration file", "" },
    { "list", 'l', 0, G_OPTION_ARG_NONE, &list_entries, "List recorded crashes in local database", "" },
    { "files", 'f', 0, G_OPTION_ARG_NONE, &list_files, "List content for a crash archive", "" },
    { "info", 'i', 0, G_OPTION_ARG_NONE, &print_info, "Print crash info file from a crash archive", "" },
    { "extract", 'x', 0, G_OPTION_ARG_NONE, &extract, "Extract coredump file in current directory", "" },
    { "print", 'p', 0, G_OPTION_ARG_STRING, &print_file, "Print file from crash archive", "" },
    { "bt", 'b', 0, G_OPTION_ARG_NONE, &print_bt, "Print current thread backtrace from a crash archive", "" },
    { "btall", 'a', 0, G_OPTION_ARG_NONE, &print_btall, "Print thread backtarce for all threads", "" },
    { NULL }
  };

  context = g_option_context_new ("- Crash information tool");
  g_option_context_set_summary (context,
                                "The tool extract information from cdh archives and cdh database");
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

  if (config_path == NULL)
    config_path = g_build_filename (CDM_CONFIG_DIRECTORY, CDM_CONFIG_FILE_NAME, NULL);

  if (g_access (config_path, R_OK) == 0)
    {
      app = cdi_application_new (config_path, &error);

      if (error != NULL)
        {
          g_printerr ("%s\n", error->message);
          status = CDM_STATUS_ERROR;
        }
      else
        {
          g_info ("Crashinfo tool started for OS version '%s'", cdm_utils_get_osversion ());

          if (list_entries)
            {
              cdi_application_list_entries (app);
            }
          else if (print_info && argc == 2)
            {
              cdi_application_print_info (app, argv[1]);
            }
          else if (list_files && argc == 2)
            {
              cdi_application_list_content (app, argv[1]);
            }
          else if (extract && argc == 2)
            {
              cdi_application_extract_coredump (app, argv[1]);
            }
          else if (print_file != NULL && argc == 2)
            {
              cdi_application_print_file (app, print_file, argv[1]);
            }
          else if ((print_bt || print_btall) && argc == 2)
            {
              cdi_application_print_backtrace (app, print_btall, argv[1]);
            }
          else
            {
              if (argc == 2)
                cdi_application_print_info (app, argv[1]);
              else
                cdi_application_list_entries (app);
            }
        }
    }
  else
    {
      g_warning ("Cannot open configuration file %s", config_path);
    }

  return status == CDM_STATUS_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
