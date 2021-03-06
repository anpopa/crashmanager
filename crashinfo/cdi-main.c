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
 * \file cdi-main.c
 */

#include "cdi-application.h"
#include "cdm-utils.h"

#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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
  gboolean print_epilog = FALSE;
  gboolean extract = FALSE;
  gboolean print_bt = FALSE;
  gboolean print_btall = FALSE;
  CdmStatus status = CDM_STATUS_OK;

  GOptionEntry main_entries[] = {
    { "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Show program version", "" },
    { "config", 'c', 0, G_OPTION_ARG_FILENAME, &config_path, "Override configuration file", "" },
    { "list", 'l', 0, G_OPTION_ARG_NONE, &list_entries, "List crashes from local database", "" },
    { "files", 'f', 0, G_OPTION_ARG_NONE, &list_files, "List content for a crash archive", "" },
    { "info", 'i', 0, G_OPTION_ARG_NONE, &print_info, "Print  nfo file from a crash archive", "" },
    { "epilog", 'e', 0, G_OPTION_ARG_NONE, &print_epilog, "Print epilog file from a archive", "" },
    { "extract", 'x', 0, G_OPTION_ARG_NONE, &extract, "Extract coredump file in cwd", "" },
    { "print", 'p', 0, G_OPTION_ARG_STRING, &print_file, "Print file from crash archive", "" },
    { "bt", 'b', 0, G_OPTION_ARG_NONE, &print_bt, "Print backtrace from a crash archive", "" },
    { "btall", 'a', 0, G_OPTION_ARG_NONE, &print_btall, "Print all thread backtarces", "" },
    { 0 }
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
            cdi_application_list_entries (app);
          else if (print_info && argc == 2)
            cdi_application_print_info (app, argv[1]);
          else if (print_epilog && argc == 2)
            cdi_application_print_epilog (app, argv[1]);
          else if (list_files && argc == 2)
            cdi_application_list_content (app, argv[1]);
          else if (extract && argc == 2)
            cdi_application_extract_coredump (app, argv[1]);
          else if (print_file != NULL && argc == 2)
            cdi_application_print_file (app, print_file, argv[1]);
          else if ((print_bt || print_btall) && argc == 2)
            cdi_application_print_backtrace (app, print_btall, argv[1]);
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
    g_warning ("Cannot open configuration file %s", config_path);

  return status == CDM_STATUS_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
