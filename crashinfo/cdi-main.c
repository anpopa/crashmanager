/* cdi-main.c
 *
 * Copyright 2019 Alin Popa <alin.popa@fxdata.ro>
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
  g_autofree gchar *input_file = NULL;
  gboolean version = FALSE;
  gboolean list_entries = FALSE;
  gboolean print_info = FALSE;
  CdmStatus status = CDM_STATUS_OK;

  GOptionEntry main_entries[] = {
    { "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Show program version", "" },
    { "config", 'c', 0, G_OPTION_ARG_FILENAME, &config_path, "Override configuration file", "" },
    { "list", 'l', 0, G_OPTION_ARG_NONE, &list_entries, "List recorded crashes", "" },
    { "files", 'f', 0, G_OPTION_ARG_FILENAME, &input_file, "List content for a crash archive", "" },
    { "info", 'i', 0, G_OPTION_ARG_NONE, &print_info, "Print crash info from a crash archive", "" },
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

  cdm_logging_open ("CDI", "Crashinfo tool", "CDI", "Default context");

  if (config_path == NULL)
    config_path = g_build_filename (CDM_CONFIG_DIRECTORY, CDM_CONFIG_FILE_NAME, NULL);

  if (g_access (config_path, R_OK) == 0)
    {
      app = cdi_application_new (config_path);
      g_info ("Crashinfo tool started for OS version '%s'", cdm_utils_get_osversion ());

      if (list_entries)
        {
          cdi_application_list_entries (app);
        }
      else if (input_file != NULL)
        {
          if (print_info)
            {
              cdi_application_print_info (app, input_file);
            }
          else
            {
              cdi_application_list_content (app, input_file);
            }
        }
    }
  else
    {
      g_warning ("Cannot open configuration file %s", config_path);
    }

  cdm_logging_close ();

  return status == CDM_STATUS_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
