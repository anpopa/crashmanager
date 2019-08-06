/* cdm-transfer.c
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

#include "cdm-transfer.h"

#ifdef WITH_GENIVI_DLT
#include <dlt.h>
#endif

#ifdef WITH_GENIVI_DLT
DLT_DECLARE_CONTEXT (cdm_transfer_ctx);
#endif

typedef gboolean (*TransferFileMessageFunc) (gpointer message, gpointer data);


gboolean transfer_source_prepare (GSource *source, gint *timeout);
gboolean transfer_source_check (GSource *source);
gboolean transfer_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data);
static gboolean transfer_source_callback (gpointer message, gpointer data);
static void transfer_source_destroy_notify (gpointer data);
static void transfer_queue_destroy_notify (gpointer data);

static GSourceFuncs transfer_source_funcs =
{
  transfer_source_prepare,
  NULL,
  transfer_source_dispatch,
  NULL,
  NULL,
  NULL,
};

gboolean
transfer_source_prepare (GSource *source, gint *timeout)
{
  CdmTransfer *transfer = (CdmTransfer *)source;

  CDM_UNUSED (timeout);

  return (g_async_queue_length (transfer->queue) > 0);
}

gboolean
transfer_source_check (GSource *source)
{
  CDM_UNUSED (source);
  return TRUE;
}

gboolean
transfer_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data)
{
  TransferFileMessageFunc transfer_cb = (TransferFileMessageFunc) callback;
  CdmTransfer *transfer = (CdmTransfer *)source;
  gpointer message;

  if (callback == NULL)
    {
      return G_SOURCE_CONTINUE;
    }

  message = g_async_queue_try_pop (transfer->queue);

  if (message == NULL)
    {
      return G_SOURCE_CONTINUE;
    }

  return transfer_cb (message, user_data) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
transfer_source_callback (gpointer message, gpointer data)
{
  CdmTransfer *transfer = (CdmTransfer *)data;
  gchar *fpath = (gchar *)message;

  g_assert (transfer);
  g_assert (fpath);

  g_info ("Transfer file %s", fpath);

  return TRUE;
}

static void
transfer_source_destroy_notify (gpointer data)
{
  CdmTransfer *transfer = (CdmTransfer *)data;

  g_assert (transfer);
  g_info ("Transfer destroy");

  cdm_transfer_unref (transfer);
}

static void
transfer_queue_destroy_notify (gpointer data)
{
  CDM_UNUSED (data);
  g_info ("Transfer queue destroyed");
}

CdmTransfer *
cdm_transfer_new (void)
{
  CdmTransfer *transfer = (CdmTransfer *)g_source_new (&transfer_source_funcs, sizeof(CdmTransfer));

  g_assert (transfer);

  g_ref_count_init (&transfer->rc);
  g_ref_count_inc (&transfer->rc);

#ifdef WITH_GENIVI_DLT
  DLT_REGISTER_CONTEXT (cdm_transfer_ctx, "FILE", "Crashdumps file transfer");
#endif

  transfer->queue = g_async_queue_new_full (transfer_queue_destroy_notify);

  g_source_set_callback ((GSource *)transfer, G_SOURCE_FUNC (transfer_source_callback),
                         transfer, transfer_source_destroy_notify);
  g_source_attach ((GSource *)transfer, NULL); /* attach the source to the default context */

  return transfer;
}

CdmTransfer *
cdm_transfer_ref (CdmTransfer *transfer)
{
  g_assert (transfer);
  g_ref_count_inc (&transfer->rc);
  return transfer;
}

void
cdm_transfer_unref (CdmTransfer *transfer)
{
  g_assert (transfer);

  if (g_ref_count_dec (&transfer->rc) == TRUE)
    {
      g_async_queue_unref (transfer->queue);
#ifdef WITH_GENIVI_DLT
      DLT_UNREGISTER_CONTEXT (cdm_transfer_ctx);
#endif
      g_free (transfer);
    }
}

CdmStatus
cdm_transfer_file (CdmTransfer *transfer, const gchar *fpath)
{
  g_assert (transfer);
  g_assert (fpath);

  g_async_queue_push (transfer->queue, g_strdup (fpath));

  return CDM_STATUS_OK;
}

GSource *
cdm_transfer_get_source (CdmTransfer *transfer)
{
  g_assert (transfer);
  return (GSource *)transfer;
}
