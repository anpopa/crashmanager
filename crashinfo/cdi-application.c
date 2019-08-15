/* cdi-application.c
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

#include "cdi-application.h"
#include "cdm-utils.h"

CdiApplication *
cdi_application_new (const gchar *config)
{
  CdiApplication *app = g_new0 (CdiApplication, 1);

  g_assert (app);

  g_ref_count_init (&app->rc);

  app->options = cdm_options_new (config);
  app->journal = cdi_journal_new (app->options);

  return app;
}

CdiApplication *
cdi_application_ref (CdiApplication *app)
{
  g_assert (app);
  g_ref_count_inc (&app->rc);
  return app;
}

void
cdi_application_unref (CdiApplication *app)
{
  g_assert (app);
  g_assert (app->options);
  g_assert (app->journal);

  if (g_ref_count_dec (&app->rc) == TRUE)
    {
      cdi_journal_unref (app->journal);
      cdm_options_unref (app->options);
      g_free (app);
    }
}

void
cdi_application_list_entries (CdiApplication *app)
{
  cdi_journal_list_entries (app->journal, NULL);
}
