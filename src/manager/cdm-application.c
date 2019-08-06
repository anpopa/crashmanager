/* cdm-application.c
 *
 * Copyright 2019 Alin Popa <alin.popa@fxapp.ro>
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

#include "cdm-application.h"

#include <glib.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

CdmApplication *
cdm_application_new (const gchar *config)
{
  CdmApplication *app = g_new0 (CdmApplication, 1);

  g_assert (app);

  g_ref_count_init (&app->rc);
  g_ref_count_inc (&app->rc);

  app->options = cdm_options_new (config);
  app->janitor = cdm_janitor_new ();
  app->bitstore = cdm_bitstore_new ();
  app->sdnotify = cdm_sdnotify_new ();
  app->transfer = cdm_transfer_new ();
  app->server = cdm_server_new (app->options, app->transfer);
  app->mainloop = g_main_loop_new (NULL, TRUE);

  return app;
}

CdmApplication *
cdm_application_ref (CdmApplication *app)
{
  g_assert (app);
  g_ref_count_inc (&app->rc);
  return app;
}

void
cdm_application_unref (CdmApplication *app)
{
  g_assert (app);
  g_assert (app->options);
  g_assert (app->server);
  g_assert (app->janitor);
  g_assert (app->bitstore);
  g_assert (app->sdnotify);
  g_assert (app->transfer);

  if (g_ref_count_dec (&app->rc) == TRUE)
    {
      cdm_server_unref (app->server);
      cdm_janitor_unref (app->janitor);
      cdm_bitstore_unref (app->bitstore);
      cdm_sdnotify_unref (app->sdnotify);
      cdm_transfer_unref (app->transfer);
      cdm_options_unref (app->options);

      g_main_loop_unref (app->mainloop);

      g_free (app);
    }
}

GMainLoop *
cdm_application_get_mainloop (CdmApplication *app)
{
  g_assert (app);
  return app->mainloop;
}

CdmStatus
cdm_application_execute (CdmApplication *app)
{
  CdmStatus status = CDM_STATUS_OK;

  status = cdm_server_bind_and_listen (app->server);

  g_main_loop_run (app->mainloop);

  return status;
}
