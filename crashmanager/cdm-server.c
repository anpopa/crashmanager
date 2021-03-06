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
 * \file cdm-server.c
 */

#include "cdm-server.h"
#include "cdm-client.h"

#include <errno.h>
#include <memory.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

/**
 * @brief GSource prepare function
 */
static gboolean server_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource check function
 */
static gboolean server_source_check (GSource *source);

/**
 * @brief GSource dispatch function
 */
static gboolean server_source_dispatch (GSource *source, GSourceFunc callback, gpointer cdmserver);

/**
 * @brief GSource callback function
 */
static gboolean server_source_callback (gpointer cdmserver);

/**
 * @brief GSource destroy notification callback function
 */
static void server_source_destroy_notify (gpointer cdmserver);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs server_source_funcs = {
  server_source_prepare, NULL, server_source_dispatch, NULL, NULL, NULL,
};

static gboolean
server_source_prepare (GSource *source, gint *timeout)
{
  CDM_UNUSED (source);
  *timeout = -1;
  return FALSE;
}

static gboolean
server_source_check (GSource *source)
{
  CDM_UNUSED (source);
  return TRUE;
}

static gboolean
server_source_dispatch (GSource *source, GSourceFunc callback, gpointer cdmserver)
{
  CDM_UNUSED (source);

  if (callback == NULL)
    {
      return G_SOURCE_CONTINUE;
    }

  return callback (cdmserver) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
server_source_callback (gpointer cdmserver)
{
  CdmServer *server = (CdmServer *)cdmserver;
  gint clientfd;

  g_assert (server);

  clientfd = accept (server->sockfd, (struct sockaddr *)NULL, NULL);

  if (clientfd >= 0)
    {
      CdmClient *client = cdm_client_new (clientfd, server->transfer, server->journal);

#ifdef WITH_GENIVI_NSM
      cdm_client_set_lifecycle (client, server->lifecycle);
#else
      CDM_UNUSED (client);
#endif

#ifdef WITH_DBUS_SERVICES
      cdm_client_set_dbusown (client, server->dbusown);
#endif
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
server_source_destroy_notify (gpointer cdmserver)
{
  CDM_UNUSED (cdmserver);
  g_info ("Server terminated");
}

CdmServer *
cdm_server_new (CdmOptions *options, CdmTransfer *transfer, CdmJournal *journal, GError **error)
{
  CdmServer *server = NULL;
  struct timeval tout;
  glong timeout;

  g_assert (options);
  g_assert (transfer);
  g_assert (journal);

  server = (CdmServer *)g_source_new (&server_source_funcs, sizeof (CdmServer));
  g_assert (server);

  g_ref_count_init (&server->rc);
  server->options = cdm_options_ref (options);
  server->transfer = cdm_transfer_ref (transfer);
  server->journal = cdm_journal_ref (journal);

  server->sockfd = socket (AF_UNIX, SOCK_STREAM, 0);
  if (server->sockfd < 0)
    {
      g_warning ("Cannot create server socket");
      g_set_error (error, g_quark_from_static_string ("ServerNew"), 1,
                   "Fail to create server socket");
    }
  else
    {
      timeout = cdm_options_long_for (options, KEY_IPC_TIMEOUT_SEC);

      tout.tv_sec = timeout;
      tout.tv_usec = 0;

      if (setsockopt (server->sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tout, sizeof (tout)) == -1)
        g_warning ("Failed to set the socket receiving timeout: %s", strerror (errno));

      if (setsockopt (server->sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tout, sizeof (tout)) == -1)
        g_warning ("Failed to set the socket sending timeout: %s", strerror (errno));
    }

  g_source_set_callback (CDM_EVENT_SOURCE (server), G_SOURCE_FUNC (server_source_callback), server,
                         server_source_destroy_notify);
  g_source_attach (CDM_EVENT_SOURCE (server), NULL);

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
      cdm_options_unref (server->options);
      cdm_transfer_unref (server->transfer);
      cdm_journal_unref (server->journal);
#ifdef WITH_GENIVI_NSM
      if (server->lifecycle != NULL)
        cdm_lifecycle_unref (server->lifecycle);
#endif
#ifdef WITH_DBUS_SERVICES
      if (server->dbusown != NULL)
        cdm_dbusown_unref (server->dbusown);
#endif
      g_source_unref (CDM_EVENT_SOURCE (server));
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

  run_dir = cdm_options_string_for (server->options, KEY_RUN_DIR);
  sock_addr = cdm_options_string_for (server->options, KEY_IPC_SOCK_ADDR);
  udspath = g_build_filename (run_dir, sock_addr, NULL);

  unlink (udspath);

  memset (&saddr, 0, sizeof (struct sockaddr_un));
  saddr.sun_family = AF_UNIX;
  strncpy (saddr.sun_path, udspath, sizeof (saddr.sun_path) - 1);

  g_debug ("Server socket path %s", saddr.sun_path);

  if (bind (server->sockfd, (struct sockaddr *)&saddr, sizeof (struct sockaddr_un)) != -1)
    {
      if (listen (server->sockfd, 10) == -1)
        {
          g_warning ("Server listen failed for path %s", saddr.sun_path);
          status = CDM_STATUS_ERROR;
        }
    }
  else
    {
      g_warning ("Server bind failed for path %s", saddr.sun_path);
      status = CDM_STATUS_ERROR;
    }

  if (status == CDM_STATUS_ERROR)
    {
      if (server->sockfd > 0)
        close (server->sockfd);

      server->sockfd = -1;
    }
  else
    server->tag
        = g_source_add_unix_fd (CDM_EVENT_SOURCE (server), server->sockfd, G_IO_IN | G_IO_PRI);

  return status;
}

#ifdef WITH_GENIVI_NSM
void
cdm_server_set_lifecycle (CdmServer *server, CdmLifecycle *lifecycle)
{
  g_assert (server);
  g_assert (lifecycle);

  server->lifecycle = cdm_lifecycle_ref (lifecycle);
}
#endif
#ifdef WITH_DBUS_SERVICES
void
cdm_server_set_dbusown (CdmServer *server, CdmDBusOwn *dbusown)
{
  g_assert (server);
  g_assert (dbusown);

  server->dbusown = cdm_dbusown_ref (dbusown);
}
#endif
