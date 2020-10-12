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
 * \file cdm-elogsrv.c
 */

#include "cdm-elogsrv.h"
#include "cdm-elogclt.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @brief GSource prepare function
 */
static gboolean elogsrv_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource check function
 */
static gboolean elogsrv_source_check (GSource *source);

/**
 * @brief GSource dispatch function
 */
static gboolean elogsrv_source_dispatch (GSource *source, GSourceFunc callback,
                                         gpointer cdmelogsrv);

/**
 * @brief GSource callback function
 */
static gboolean elogsrv_source_callback (gpointer cdmelogsrv);

/**
 * @brief GSource destroy notification callback function
 */
static void elogsrv_source_destroy_notify (gpointer cdmelogsrv);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs elogsrv_source_funcs =
{
  elogsrv_source_prepare,
  NULL,
  elogsrv_source_dispatch,
  NULL,
  NULL,
  NULL,
};

static gboolean
elogsrv_source_prepare (GSource *source, gint *timeout)
{
  CDM_UNUSED (source);
  *timeout = -1;
  return FALSE;
}

static gboolean
elogsrv_source_check (GSource *source)
{
  CDM_UNUSED (source);
  return TRUE;
}

static gboolean
elogsrv_source_dispatch (GSource *source, GSourceFunc callback, gpointer cdmelogsrv)
{
  CDM_UNUSED (source);

  if (callback == NULL)
    return G_SOURCE_CONTINUE;

  return callback (cdmelogsrv) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
elogsrv_source_callback (gpointer cdmelogsrv)
{
  CdmELogSrv *elogsrv = (CdmELogSrv *)cdmelogsrv;
  gint clientfd;

  g_assert (elogsrv);

  clientfd = accept (elogsrv->sockfd, (struct sockaddr*)NULL, NULL);
  if (clientfd >= 0)
    {
      CdmELogClt *client = cdm_elogclt_new (clientfd, elogsrv->journal);

      CDM_UNUSED (client);
      g_debug ("New epilog client connected %d", clientfd);
    }
  else
    {
      g_warning ("Epilog server accept failed");
      return FALSE;
    }

  return TRUE;
}

static void
elogsrv_source_destroy_notify (gpointer cdmelogsrv)
{
  CDM_UNUSED (cdmelogsrv);
  g_info ("Epilog server terminated");
}

CdmELogSrv *
cdm_elogsrv_new (CdmOptions *options, CdmJournal *journal, GError **error)
{
  CdmELogSrv *elogsrv = NULL;
  struct timeval tout;
  glong timeout;

  g_assert (options);

  elogsrv = (CdmELogSrv *)g_source_new (&elogsrv_source_funcs, sizeof(CdmELogSrv));
  g_assert (elogsrv);

  g_ref_count_init (&elogsrv->rc);
  elogsrv->options = cdm_options_ref (options);
  elogsrv->journal = cdm_journal_ref (journal);

  elogsrv->sockfd = socket (AF_UNIX, SOCK_STREAM, 0);
  if (elogsrv->sockfd < 0)
    {
      g_warning ("Cannot create elogsrv socket");
      g_set_error (error,
                   g_quark_from_static_string ("ServerNew"),
                   1,
                   "Fail to create elogsrv socket");
    }
  else
    {
      timeout = cdm_options_long_for (options, KEY_ELOG_TIMEOUT_SEC);

      tout.tv_sec = timeout;
      tout.tv_usec = 0;

      if (setsockopt (elogsrv->sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tout, sizeof(tout)) == -1)
        g_warning ("Failed to set the epilog socket receiving timeout: %s", strerror (errno));

      if (setsockopt (elogsrv->sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tout, sizeof(tout)) == -1)
        g_warning ("Failed to set the epilog socket sending timeout: %s", strerror (errno));
    }

  g_source_set_callback (CDM_EVENT_SOURCE (elogsrv),
                         G_SOURCE_FUNC (elogsrv_source_callback),
                         elogsrv, elogsrv_source_destroy_notify);
  g_source_attach (CDM_EVENT_SOURCE (elogsrv), NULL);

  return elogsrv;
}

CdmELogSrv *
cdm_elogsrv_ref (CdmELogSrv *elogsrv)
{
  g_assert (elogsrv);
  g_ref_count_inc (&elogsrv->rc);
  return elogsrv;
}

void
cdm_elogsrv_unref (CdmELogSrv *elogsrv)
{
  g_assert (elogsrv);

  if (g_ref_count_dec (&elogsrv->rc) == TRUE)
    {
      cdm_options_unref (elogsrv->options);
      cdm_journal_unref (elogsrv->journal);
      g_source_unref (CDM_EVENT_SOURCE (elogsrv));
    }
}

CdmStatus
cdm_elogsrv_bind_and_listen (CdmELogSrv *elogsrv)
{
  g_autofree gchar *sock_addr = NULL;
  g_autofree gchar *run_dir = NULL;
  g_autofree gchar *udspath = NULL;
  CdmStatus status = CDM_STATUS_OK;
  struct sockaddr_un saddr;

  g_assert (elogsrv);

  run_dir = cdm_options_string_for (elogsrv->options, KEY_RUN_DIR);
  sock_addr = cdm_options_string_for (elogsrv->options, KEY_ELOG_SOCK_ADDR);
  udspath = g_build_filename (run_dir, sock_addr, NULL);

  unlink (udspath);

  memset (&saddr, 0, sizeof(struct sockaddr_un));
  saddr.sun_family = AF_UNIX;
  strncpy (saddr.sun_path, udspath, sizeof(saddr.sun_path) - 1);

  g_debug ("Epilog server socket path %s", saddr.sun_path);

  if (bind (elogsrv->sockfd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_un)) != -1)
    {
      if (chmod (udspath, 0666) != 0)
        g_warning ("Epilog server fail to chmod %s", saddr.sun_path);

      if (listen (elogsrv->sockfd, 10) == -1)
        {
          g_warning ("Epilog server listen failed for path %s", saddr.sun_path);
          status = CDM_STATUS_ERROR;
        }
    }
  else
    {
      g_warning ("Epilog server bind failed for path %s", saddr.sun_path);
      status = CDM_STATUS_ERROR;
    }

  if (status == CDM_STATUS_ERROR)
    {
      if (elogsrv->sockfd > 0)
        close (elogsrv->sockfd);

      elogsrv->sockfd = -1;
    }
  else
    {
      elogsrv->tag = g_source_add_unix_fd (CDM_EVENT_SOURCE (elogsrv),
                                           elogsrv->sockfd,
                                           G_IO_IN | G_IO_PRI);
    }

  return status;
}
