/* cdm-main.c
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

#include "cdm-defaults.h"
#include "cdm-types.h"
#include "cdm-logging.h"
#include "cdm-options.h"
#include "cdm-server.h"
#include "cdm-janitor.h"
#include "cdm-bitstore.h"
#include "cdm-sdnotify.h"
#include "cdm-transfer.h"

#include <glib.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

static GMainLoop *g_mainloop = NULL;

typedef struct _CdmData {
  CdmOptions *options;
  CdmServer *server;
  CdmJanitor *janitor;
  CdmBitstore *bitstore;
  CdmSDNotify *sdnotify;
  CdmTransfer *transfer;
  GMainLoop *mainloop;
} CdmData;

static CdmData *cdm_data_new (const gchar *config);
static void cdm_data_free (CdmData *data);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmData, cdm_data_free);

static CdmData *
cdm_data_new (const gchar *config)
{
  CdmData *data = g_new0 (CdmData, 1);

  g_assert (data);

  data->options = cdm_options_new (config);
  data->server = cdm_server_new ();
  data->janitor = cdm_janitor_new ();
  data->bitstore = cdm_bitstore_new ();
  data->sdnotify = cdm_sdnotify_new ();
  data->transfer = cdm_transfer_new ();

  data->mainloop = g_main_loop_new (NULL, TRUE);
  g_mainloop = data->mainloop;

  return data;
}

static void
cdm_data_free (CdmData *data)
{
  g_assert (data);
  g_assert (data->options);
  g_assert (data->server);
  g_assert (data->janitor);
  g_assert (data->bitstore);
  g_assert (data->sdnotify);
  g_assert (data->transfer);

  cdm_server_unref (data->server);
  cdm_janitor_unref (data->janitor);
  cdm_bitstore_unref (data->bitstore);
  cdm_sdnotify_unref (data->sdnotify);
  cdm_transfer_unref (data->transfer);
  cdm_options_unref (data->options);

  g_main_loop_unref (data->mainloop);

  g_free (data);
}

static void
terminate (int signum)
{
  g_info ("Crashmanager terminate with signal %d", signum);
  if (g_mainloop != NULL)
    {
      g_main_loop_quit (g_mainloop);
    }
}

static CdmStatus
cdm_data_run (CdmData *data)
{
  CdmStatus status = CDM_STATUS_OK;


  g_main_loop_run (data->mainloop);

  return status;
}

gint
main (gint argc, gchar *argv[])
{
  g_autoptr (GOptionContext) context = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (CdmData) data = NULL;
  gboolean version = FALSE;
  gchar *config_path = NULL;
  CdmStatus status = CDM_STATUS_OK;

  GOptionEntry main_entries[] = {
    { "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Show program version", "" },
    { "config", 'c', 0, G_OPTION_ARG_FILENAME, config_path, "Override configuration file", "" },
    { NULL }
  };

  signal (SIGINT, terminate);
  signal (SIGTERM, terminate);

  context = g_option_context_new ("- Crash manager service daemon");
  g_option_context_set_summary (context, "The service listen for Crashhandler events and manage its output");
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
    {
      config_path = g_build_filename (CDM_CONFIG_DIRECTORY, CDM_CONFIG_FILE_NAME, NULL);
    }

  if (g_access (config_path, R_OK) == 0)
    {
      data = cdm_data_new (config_path);
      status = cdm_data_run (data);
    }
  else
    {
      g_warning ("Cannot open configuration file %s", config_path);
    }

  cdm_logging_close ();

  return status == CDM_STATUS_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
