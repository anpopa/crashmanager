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
 * \file cdm-elogclt.c
 */

#include "cdm-elogclt.h"
#include "cdm-utils.h"
#include "cdm-defaults.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/**
 * @brief GSource prepare function
 */
static gboolean client_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource check function
 */
static gboolean client_source_check (GSource *source);

/**
 * @brief GSource dispatch function
 */
static gboolean client_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data);

/**
 * @brief GSource callback function
 */
static gboolean client_source_callback (gpointer data);

/**
 * @brief GSource destroy notification callback function
 */
static void client_source_destroy_notify (gpointer data);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs client_source_funcs =
{
  client_source_prepare,
  NULL,
  client_source_dispatch,
  NULL,
  NULL,
  NULL,
};

static gboolean
client_source_prepare (GSource *source, gint *timeout)
{
  CDM_UNUSED (source);
  *timeout = -1;
  return FALSE;
}

static gboolean
client_source_check (GSource *source)
{
  CDM_UNUSED (source);
  return TRUE;
}

static gboolean
client_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data)
{
  CDM_UNUSED (source);

  if (callback == NULL)
    return G_SOURCE_CONTINUE;

  return callback (user_data) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
client_source_callback (gpointer data)
{
  CdmELogClt *client = (CdmELogClt *)data;
  gboolean status = TRUE;

  g_assert (client);

  if (client->last_msg_type != CDM_ELOGMSG_NEW)
    {
      CdmELogMessage *msg = cdm_elogmsg_new (CDM_ELOGMSG_INVALID);

      if (cdm_elogmsg_read (client->sockfd, msg) != 0)
        {
          g_warning ("Cannot read from epilog client init message %d", client->sockfd);
          status = FALSE;
        }
      else
        {
          if (cdm_elogmsg_get_type (msg) == CDM_ELOGMSG_NEW)
            {
              client->last_msg_type = cdm_elogmsg_get_type (msg);
              client->process_pid = cdm_elogmsg_get_process_pid (msg);
              client->process_sig = cdm_elogmsg_get_process_exit_signal (msg);

              g_info ("Received epilog notification for process id %ld", client->process_pid);
            }
        }

      cdm_elogmsg_free (msg);
    }
  else
    {
      CdmJournalEpilog *elog = g_new0 (CdmJournalEpilog, 1);
      gssize toread = CDM_JOURNAL_EPILOG_MAX_BT;
      gssize sz;

      elog->pid = client->process_pid;

      do
        {
          const gssize framesz = toread < CDM_EPILOG_FRAME_LEN ? toread : CDM_EPILOG_FRAME_LEN;

          sz = read (client->sockfd,
                     elog->backtrace + (CDM_JOURNAL_EPILOG_MAX_BT - toread),
                     (gsize)framesz);

          if ((sz >= 0) && (sz <= framesz))
            toread -= sz;
        } while (((sz > 0) || (errno == EAGAIN)) && (toread > 0));

      if (toread < CDM_JOURNAL_EPILOG_MAX_BT)
        cdm_journal_epilog_add (client->journal, elog);
      else
        {
          g_warning ("Fail to read epilog backtrace from client %d", client->sockfd);
          g_free (elog);
        }

      status = FALSE;
    }

  return status;
}

static void
client_source_destroy_notify (gpointer data)
{
  CdmELogClt *client = (CdmELogClt *)data;

  g_assert (client);
  g_debug ("Epilog client %d disconnected", client->sockfd);

  cdm_elogclt_unref (client);
}

CdmELogClt *
cdm_elogclt_new (gint clientfd,
                 CdmJournal *journal)
{
  CdmELogClt *client = (CdmELogClt *)g_source_new (&client_source_funcs, sizeof(CdmELogClt));

  g_assert (client);

  g_ref_count_init (&client->rc);
  client->sockfd = clientfd;
  client->journal = cdm_journal_ref (journal);
  client->last_msg_type = CDM_ELOGMSG_INVALID;

  g_source_set_callback (CDM_EVENT_SOURCE (client),
                         G_SOURCE_FUNC (client_source_callback),
                         client,
                         client_source_destroy_notify);
  g_source_attach (CDM_EVENT_SOURCE (client), NULL);

  client->tag = g_source_add_unix_fd (CDM_EVENT_SOURCE (client),
                                      client->sockfd,
                                      G_IO_IN | G_IO_PRI);

  return client;
}

CdmELogClt *
cdm_elogclt_ref (CdmELogClt *client)
{
  g_assert (client);
  g_ref_count_inc (&client->rc);
  return client;
}

void
cdm_elogclt_unref (CdmELogClt *client)
{
  g_assert (client);

  if (g_ref_count_dec (&client->rc) == TRUE)
    {
      cdm_journal_unref (client->journal);
      g_source_unref (CDM_EVENT_SOURCE (client));
    }
}
