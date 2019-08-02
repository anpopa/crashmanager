/* cdm-server.c
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

#include "cdm-server.h"

static GSourceFuncs server_source_funcs =
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
  };

CdmServer *
cdm_server_new (void)
{
  CdmServer *server = g_new0 (CdmServer, 1);

  g_assert (server);

  g_ref_count_init (&server->rc);
  g_ref_count_inc (&server->rc);

  server->source = g_source_new (&server_source_funcs, sizeof(GSource));
  g_source_ref (server->source);

  return server;
}

CdmServer *
cdm_server_ref (CdmServer *server)
{
  g_assert (server);
  g_ref_count_inc (&server->rc);
  return server;
}

void
cdm_server_unref (CdmServer *server)
{
  g_assert (server);

  if (g_ref_count_dec (&server->rc) == TRUE)
    {
      g_source_unref (server->source);
      g_free (server);
    }
}

GSource *cdm_server_get_source (CdmServer *server)
{
  g_assert (server);
  return server->source;
}
