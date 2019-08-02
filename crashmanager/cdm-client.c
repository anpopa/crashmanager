/* cdm-client.c
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

#include "cdm-client.h"

static GSourceFuncs client_source_funcs =
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
  };

CdmClient *
cdm_client_new (void)
{
  CdmClient *client = g_new0 (CdmClient, 1);

  g_assert (client);

  g_ref_count_init (&client->rc);
  g_ref_count_inc (&client->rc);

  client->source = g_source_new (&client_source_funcs, sizeof(GSource));
  g_source_ref (client->source);

  return client;
}

CdmClient *
cdm_client_ref (CdmClient *client)
{
  g_assert (client);
  g_ref_count_inc (&client->rc);
  return client;
}

void
cdm_client_unref (CdmClient *client)
{
  g_assert (client);

  if (g_ref_count_dec (&client->rc) == TRUE)
    {
      g_source_unref (client->source);
      g_free (client);
    }
}

GSource *cdm_client_get_source (CdmClient *client)
{
  g_assert (client);
  return client->source;
}
