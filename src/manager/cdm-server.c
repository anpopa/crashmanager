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
#include "cdm-client.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <time.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

gboolean server_source_prepare (GSource *source, gint *timeout);
gboolean server_source_check (GSource *source);
gboolean server_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data);
static gboolean server_source_callback (gpointer data);
static void server_source_destroy_notify (gpointer data);

static GSourceFuncs server_source_funcs =
{
  server_source_prepare,
  NULL,
  server_source_dispatch,
  NULL,
  NULL,
  NULL,
};

gboolean
server_source_prepare (GSource *source, gint *timeout)
{
  CDM_UNUSED (source);
  *timeout = -1;
  return FALSE;
}

gboolean
server_source_check (GSource *source)
{
  CDM_UNUSED (source);
  return TRUE;
}

gboolean
server_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data)
{
  CDM_UNUSED (source);

  if (callback == NULL)
    {
      return G_SOURCE_CONTINUE;
    }

  return callback (user_data) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
server_source_callback (gpointer data)
{
  CdmServer *server = (CdmServer *)data;
  gint clientfd;

  g_assert (server);

  clientfd = accept (server->sockfd, (struct sockaddr*)NULL, NULL);

  if (clientfd >= 0)
    {
      CdmClient *client = cdm_client_new (clientfd, server->transfer);

      CDM_UNUSED (client);

      g_debug ("New client connected %d", clientfd);

    }
  else
    {
      g_warning ("Server accept failed");
      return FALSE;
    }

  return TRUE;
}

static void
server_source_destroy_notify (gpointer data)
{
  CDM_UNUSED (data);
  g_info ("Server terminated");
}

CdmServer *
cdm_server_new (CdmOptions *opts, CdmTransfer *transfer)
{
  CdmServer *server = NULL;
  struct timeval tout;
  glong timeout;

  g_assert (opts);

  server = g_new0 (CdmServer, 1);
  g_assert (server);

  g_ref_count_init (&server->rc);
  g_ref_count_inc (&server->rc);

  server->source = g_source_new (&server_source_funcs, sizeof(GSource));
  g_source_ref (server->source);

  server->opts = cdm_options_ref (opts);
  server->transfer = cdm_transfer_ref (transfer);

  server->sockfd = socket (AF_UNIX, SOCK_STREAM, 0);
  if (server->sockfd < 0)
    {
      g_error ("Cannot create server socket");
    }

  timeout = cdm_options_long_for (opts, KEY_IPC_TIMEOUT_SEC);

  tout.tv_sec = timeout;
  tout.tv_usec = 0;

  if (setsockopt (server->sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tout, sizeof(tout)) == -1)
    {
      g_warning ("Failed to set the socket receiving timeout: %s", strerror (errno));
    }

  if (setsockopt (server->sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tout, sizeof(tout)) == -1)
    {
      g_warning ("Failed to set the socket sending timeout: %s", strerror (errno));
    }

  g_source_set_callback (server->source, G_SOURCE_FUNC (server_source_callback),
                         server, server_source_destroy_notify);
  g_source_attach (server->source, NULL); /* attach the source to the default context */

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
      cdm_options_unref (server->opts);
      cdm_transfer_unref (server->transfer);
      g_source_unref (server->source);
      g_free (server);
    }
}

CdmStatus
cdm_server_bind_and_listen (CdmServer *server)
{
  g_autofree gchar *sock_addr = NULL;
  g_autofree gchar *run_dir = NULL;
  g_autofree gchar *udspath = NULL;
  CdmStatus status = CDM_STATUS_OK;
  struct sockaddr_un saddr;

  g_assert (server);

  run_dir = cdm_options_string_for (server->opts, KEY_RUN_DIR);
  sock_addr = cdm_options_string_for (server->opts, KEY_IPC_SOCK_ADDR);
  udspath = g_build_filename (run_dir, sock_addr, NULL);

  unlink (udspath);

  memset (&saddr, 0, sizeof(struct sockaddr_un));
  saddr.sun_family = AF_UNIX;
  strncpy (saddr.sun_path, udspath, sizeof(saddr.sun_path) - 1);

  g_info ("Server socket path %s", saddr.sun_path);

  if (bind (server->sockfd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_un)) != -1)
    {
      if (listen (server->sockfd, 10) == -1)
        {
          g_warning ("Server listen failed");
          status = CDM_STATUS_ERROR;
        }
    }
  else
    {
      g_warning ("Server bind failed");
      status = CDM_STATUS_ERROR;
    }

  if (status == CDM_STATUS_ERROR)
    {
      if (server->sockfd > 0)
        {
          close (server->sockfd);
        }
      server->sockfd = -1;
    }
  else
    {
      server->tag = g_source_add_unix_fd (server->source, server->sockfd, G_IO_IN | G_IO_PRI);
    }

  return status;
}

GSource *
cdm_server_get_source (CdmServer *server)
{
  g_assert (server);
  return server->source;
}
