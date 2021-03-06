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
 * \file cdh-manager.c
 */

#include "cdh-manager.h"
#include "cdm-defaults.h"

#include <errno.h>
#include <stdio.h>

CdhManager *
cdh_manager_new (CdmOptions *opts)
{
  CdhManager *c = g_new0 (CdhManager, 1);

  g_assert (opts);

  g_ref_count_init (&c->rc);

  c->sfd = -1;
  c->connected = false;
  c->opts = cdm_options_ref (opts);

  return c;
}

CdhManager *
cdh_manager_ref (CdhManager *c)
{
  g_assert (c);
  g_ref_count_inc (&c->rc);
  return c;
}

void
cdh_manager_unref (CdhManager *c)
{
  g_assert (c);

  if (g_ref_count_dec (&c->rc) == TRUE)
    {
      if (cdh_manager_connected (c) == TRUE)
        (void)cdh_manager_disconnect (c);

      cdm_options_unref (c->opts);
      g_free (c);
    }
}

CdmStatus
cdh_manager_connect (CdhManager *c)
{
  g_autofree gchar *opt_sock_addr = NULL;
  g_autofree gchar *opt_run_dir = NULL;
  struct timeval tout;
  glong opt_timeout;

  g_assert (c);

  if (c->connected)
    return CDM_STATUS_ERROR;

  c->sfd = socket (AF_UNIX, SOCK_STREAM, 0);
  if (c->sfd < 0)
    {
      g_warning ("Cannot create connection socket");
      return CDM_STATUS_ERROR;
    }

  opt_run_dir = cdm_options_string_for (c->opts, KEY_RUN_DIR);
  opt_sock_addr = cdm_options_string_for (c->opts, KEY_IPC_SOCK_ADDR);
  opt_timeout = cdm_options_long_for (c->opts, KEY_IPC_TIMEOUT_SEC);

  memset (&c->saddr, 0, sizeof (struct sockaddr_un));
  c->saddr.sun_family = AF_UNIX;

  snprintf (c->saddr.sun_path, (sizeof (c->saddr.sun_path) - 1), "%s/%s", opt_run_dir,
            opt_sock_addr);

  if (connect (c->sfd, (struct sockaddr *)&c->saddr, sizeof (struct sockaddr_un)) < 0)
    {
      g_info ("Core manager not available: %s", c->saddr.sun_path);
      close (c->sfd);
      return CDM_STATUS_ERROR;
    }

  tout.tv_sec = opt_timeout;
  tout.tv_usec = 0;

  if (setsockopt (c->sfd, SOL_SOCKET, SO_RCVTIMEO, (gchar *)&tout, sizeof (tout)) == -1)
    g_warning ("Failed to set the socket receiving timeout: %s", strerror (errno));

  if (setsockopt (c->sfd, SOL_SOCKET, SO_SNDTIMEO, (gchar *)&tout, sizeof (tout)) == -1)
    g_warning ("Failed to set the socket sending timeout: %s", strerror (errno));

  c->connected = true;

  return CDM_STATUS_OK;
}

CdmStatus
cdh_manager_disconnect (CdhManager *c)
{
  if (!c->connected)
    return CDM_STATUS_ERROR;

  if (c->sfd > 0)
    {
      close (c->sfd);
      c->sfd = -1;
    }

  c->connected = false;

  return CDM_STATUS_OK;
}

gboolean
cdh_manager_connected (CdhManager *c)
{
  g_assert (c);
  return c->connected;
}

gint
cdh_manager_get_socket (CdhManager *c)
{
  g_assert (c);
  return c->sfd;
}

CdmStatus
cdh_manager_send (CdhManager *c, CdmMessage *m)
{
  fd_set wfd;
  struct timeval tv;
  CdmStatus status = CDM_STATUS_OK;

  g_assert (c);
  g_assert (m);

  if (c->sfd < 0 || !c->connected)
    {
      g_warning ("No connection to manager");
      return CDM_STATUS_ERROR;
    }

  FD_ZERO (&wfd);

  tv.tv_sec = MANAGER_SELECT_TIMEOUT;
  tv.tv_usec = 0;
  FD_SET (c->sfd, &wfd);

  status = select (c->sfd + 1, NULL, &wfd, NULL, &tv);
  if (status == -1)
    {
      g_warning ("Server socket select failed");
    }
  else
    {
      if (status > 0)
        status = cdm_message_write (c->sfd, m);
    }

  return status;
}
