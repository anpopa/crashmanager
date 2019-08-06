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
#include <dlt_filetransfer.h>
#endif

#ifdef WITH_GENIVI_DLT
DLT_DECLARE_CONTEXT (cdm_transfer_ctx);
#endif

gboolean transfer_source_prepare (GSource *source, gint *timeout);
gboolean transfer_source_check (GSource *source);
gboolean transfer_source_dispatch (GSource *source, GSourceFunc callback, gpointer cdmtrans);
static gboolean transfer_source_callback (gpointer cdmtrans, gchar *file_path);
static void transfer_source_destroy_notify (gpointer cdmtrans);
static void transfer_queue_destroy_notify (gpointer cdmtrans);
static void transfer_thread_func (gpointer data, gpointer user_data);

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

  return(g_async_queue_length (transfer->queue) > 0);
}

gboolean
transfer_source_check (GSource *source)
{
  CDM_UNUSED (source);
  return TRUE;
}

gboolean
transfer_source_dispatch (GSource *source, GSourceFunc callback, gpointer cdmtrans)
{
  CdmTransfer *transfer = (CdmTransfer *)source;
  gchar *file_path = (gchar *)g_async_queue_try_pop (transfer->queue);

  CDM_UNUSED (callback);

  if (file_path == NULL)
    {
      return G_SOURCE_CONTINUE;
    }

  return transfer->callback (cdmtrans, file_path) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
transfer_source_callback (gpointer cdmtrans, gchar *file_path)
{
  CdmTransfer *transfer = (CdmTransfer *)cdmtrans;

  g_assert (transfer);
  g_assert (file_path);

  g_debug ("Push file to tread pool transfer %s", file_path);
  g_thread_pool_push (transfer->tpool, file_path, NULL);

  return TRUE;
}

static void
transfer_source_destroy_notify (gpointer data)
{
  CdmTransfer *transfer = (CdmTransfer *)data;

  g_assert (transfer);
  g_debug ("Transfer destroy notification");

  cdm_transfer_unref (transfer);
}

static void
transfer_queue_destroy_notify (gpointer data)
{
  CDM_UNUSED (data);
  g_debug ("Transfer queue destroy notification");
}

static void
transfer_thread_func (gpointer data, gpointer user_data)
{
  CdmTransfer *transfer = (CdmTransfer *)user_data;
  gchar *file_path = (gchar *)data;

  CDM_UNUSED (transfer);

  g_info ("Transfer file %s", file_path);

  if (dlt_user_log_file_complete (&cdm_transfer_ctx, file_path, 0, 60) < 0)
    {
      g_warning ("File couldn't be transferred. Please check the dlt log messages");
    }

  g_free (file_path);
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

  transfer->callback = transfer_source_callback;
  transfer->queue = g_async_queue_new_full (transfer_queue_destroy_notify);
  transfer->tpool = g_thread_pool_new (transfer_thread_func, transfer, 1, TRUE, NULL);

  g_source_set_callback (CDM_EVENT_SOURCE (transfer), NULL, transfer, transfer_source_destroy_notify);
  g_source_attach (CDM_EVENT_SOURCE (transfer), NULL); /* attach the source to the default context */

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
      g_thread_pool_free (transfer->tpool, TRUE, FALSE);
      g_source_unref (CDM_EVENT_SOURCE (transfer));
    }
}

CdmStatus
cdm_transfer_file (CdmTransfer *transfer, const gchar *file_path)
{
  g_assert (transfer);
  g_assert (file_path);

  g_async_queue_push (transfer->queue, g_strdup (file_path));

  return CDM_STATUS_OK;
}
