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
 * \file cdm-client.c
 */

#include "cdm-client.h"
#include "cdm-defaults.h"
#include "cdm-transfer.h"
#include "cdm-utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef WITH_LXC
#include <lxc/lxccontainer.h>
#endif

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
 * @brief Initial message processing
 */
static void do_initial_message_process (CdmClient *c, CdmMessage *msg);

/**
 * @brief Get context ID for PID
 */
static gchar *get_pid_context_id (pid_t pid);

/**
 * @brief Transfer complete callback
 */
static void archive_transfer_complete (gpointer cdmclient, const gchar *file_path);

#ifdef WITH_LXC
/**
 * @brief Get container name for context ID
 */
static gchar *get_container_name_for_context (const gchar *ctxid);
#endif

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs client_source_funcs = {
  client_source_prepare, NULL, client_source_dispatch, NULL, NULL, NULL,
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
  g_autoptr (CdmMessage) msg = NULL;
  CdmClient *client = (CdmClient *)data;
  gboolean status = TRUE;

  g_assert (client);

  msg = cdm_message_new (CDM_MESSAGE_INVALID, 0);

  if (cdm_message_read (client->sockfd, msg) != CDM_STATUS_OK)
    {
      g_debug ("Cannot read from client socket %d", client->sockfd);

#ifdef WITH_GENIVI_NSM
      if (client->last_msg_type != CDM_MESSAGE_INVALID)
        {
          if (cdm_lifecycle_set_session_state (client->lifecycle, LC_SESSION_INACTIVE)
              != CDM_STATUS_OK)
            {
              g_warning ("Fail decrement session lifecycle counter");
            }
        }
#endif
      status = FALSE;
    }
  else
    {
      CdmMessageType type = cdm_message_get_type (msg);

      do_initial_message_process (client, msg);

      switch (type)
        {
        case CDM_MESSAGE_COREDUMP_NEW:
#ifdef WITH_GENIVI_NSM
          if (cdm_lifecycle_set_session_state (client->lifecycle, LC_SESSION_ACTIVE)
              != CDM_STATUS_OK)
            {
              g_warning ("Fail increment session lifecycle counter");
            }
#endif
          break;

        case CDM_MESSAGE_COREDUMP_FAILED:
          g_warning ("Coredump processing failed for client %d", client->sockfd);
#ifdef WITH_GENIVI_NSM
          if (cdm_lifecycle_set_session_state (client->lifecycle, LC_SESSION_INACTIVE)
              != CDM_STATUS_OK)
            {
              g_warning ("Fail decrement session lifecycle counter");
            }
#endif
          break;

        case CDM_MESSAGE_COREDUMP_SUCCESS:
          {
            g_autoptr (GError) error = NULL;
            guint64 dbid;

            dbid = cdm_journal_add_crash (
                client->journal, client->process_name, client->process_crash_id,
                client->process_vector_id, client->process_context_id, client->context_name,
                client->lifecycle_state, client->coredump_file_path, client->process_pid,
                client->process_exit_signal, client->process_timestamp, &error);

            if (error != NULL)
              g_warning ("Fail to add new crash entry in database %s", error->message);
            else
              g_debug ("New crash entry added to database with id %016lX", dbid);

            /* even if we fail to add to the database we try to transfer the file */
            cdm_transfer_file (client->transfer, client->coredump_file_path,
                               archive_transfer_complete, cdm_client_ref (client));
#ifdef WITH_GENIVI_NSM
            if (cdm_lifecycle_set_session_state (client->lifecycle, LC_SESSION_INACTIVE)
                != CDM_STATUS_OK)
              {
                g_warning ("Fail decrement session lifecycle counter");
              }
#endif
          }
          break;

        default:
          break;
        }
    }

  return status;
}

static void
client_source_destroy_notify (gpointer data)
{
  CdmClient *client = (CdmClient *)data;

  g_assert (client);
  g_debug ("Client %d disconnected", client->sockfd);

  cdm_client_unref (client);
}

static gchar *
get_pid_context_id (pid_t pid)
{
  g_autofree gchar *ctx_str = NULL;

  for (gint i = 0; i < 7; i++)
    {
      g_autofree gchar *proc_ns_buf = NULL;
      g_autofree gchar *tmp_proc_path = NULL;

      switch (i)
        {
        case 0: /* cgroup */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/cgroup", pid);
          break;

        case 1: /* ipc */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/ipc", pid);
          break;

        case 2: /* mnt */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/mnt", pid);
          break;

        case 3: /* net */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/net", pid);
          break;

        case 4: /* pid */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/pid", pid);
          break;

        case 5: /* user */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/user", pid);
          break;

        case 6: /* uts */
          tmp_proc_path = g_strdup_printf ("/proc/%d/ns/uts", pid);
          break;

        default: /* never reached */
          break;
        }

      proc_ns_buf = g_file_read_link (tmp_proc_path, NULL);

      if (ctx_str != NULL)
        {
          gchar *ctx_tmp = g_strconcat (ctx_str, proc_ns_buf, NULL);
          g_free (ctx_str);
          ctx_str = ctx_tmp;
        }
      else
        ctx_str = g_strdup (proc_ns_buf);
    }

  if (ctx_str == NULL)
    return NULL;

  return g_strdup_printf ("%016lX", cdm_utils_jenkins_hash (ctx_str));
}

#ifdef WITH_LXC
static gchar *
get_container_name_for_context (const gchar *ctxid)
{
  struct lxc_container **active_containers = NULL;
  gchar *container_name = NULL;
  gboolean found = false;
  gchar **names = NULL;
  gint count = 0;

  count = list_active_containers ("/var/lib/lxc", &names, &active_containers);

  for (gint i = 0; i < count && !found; i++)
    {
      struct lxc_container *container = active_containers[i];
      const gchar *name = names[i];

      if (name == NULL || container == NULL)
        continue;

      if (container->is_running (container))
        {
          g_autofree gchar *tmp_id = NULL;
          pid_t pid;

          pid = container->init_pid (container);
          tmp_id = get_pid_context_id (pid);

          if (g_strcmp0 (tmp_id, ctxid) == 0)
            {
              container_name = g_strdup_printf ("%s", name);
              found = true;
            }
        }
    }

  if (!found)
    container_name = g_strdup ("container");

  for (int i = 0; i < count; i++)
    {
      free (names[i]);
      lxc_container_put (active_containers[i]);
    }

  return container_name;
}
#endif

static void
send_context_info (CdmClient *c, const gchar *context_name)
{
  g_autoptr (CdmMessage) msg = cdm_message_new (CDM_MESSAGE_COREDUMP_CONTEXT, 0);

  g_assert (c);
  g_assert (context_name);

  cdm_message_set_context_name (msg, context_name);

#ifdef WITH_GENIVI_NSM
  /* set proper value from NSM */
  cdm_message_set_lifecycle_state (msg, "running");
#else
  cdm_message_set_lifecycle_state (msg, "running");
#endif

  if (cdm_message_write (c->sockfd, msg) == CDM_STATUS_ERROR)
    g_warning ("Failed to send context information to client");
}

static void
send_epilog (CdmClient *c, CdmJournalEpilog *elog)
{
  g_autoptr (CdmMessage) msg = cdm_message_new (CDM_MESSAGE_EPILOG_FRAME_INFO, 0);
  uint64_t frame_cnt = 0;

  g_assert (c);

  if (elog == NULL)
    {
      g_info ("Epilog not available for client %lx", c->id);
      cdm_message_set_epilog_frame_count (msg, 0);
    }
  else
    {
      frame_cnt = strlen (elog->backtrace) / CDM_MESSAGE_EPILOG_FRAME_MAX_LEN;
      if (strlen (elog->backtrace) % CDM_MESSAGE_EPILOG_FRAME_MAX_LEN > 0)
        frame_cnt++;

      cdm_message_set_epilog_frame_count (msg, frame_cnt);
      g_info ("Epilog available for client %lx with %lu frames", c->id, frame_cnt);
    }

  if (cdm_message_write (c->sockfd, msg) == CDM_STATUS_ERROR)
    g_warning ("Failed to send epilog information to client");

  if (frame_cnt > 0)
    {
      for (guint64 i = 0; i < frame_cnt; i++)
        {
          g_autoptr (CdmMessage) fmsg = cdm_message_new (CDM_MESSAGE_EPILOG_FRAME_DATA, 0);

          cdm_message_set_epilog_frame_data (fmsg, elog->backtrace
                                                       + (i * CDM_MESSAGE_EPILOG_FRAME_MAX_LEN));

          if (cdm_message_write (c->sockfd, fmsg) == CDM_STATUS_ERROR)
            g_warning ("Failed to send epilog frame %lu to client", i);
        }
    }
}

static void
do_initial_message_process (CdmClient *c, CdmMessage *msg)
{
  g_autofree gchar *tmp_id = NULL;
  g_autofree gchar *tmp_name = NULL;

  g_assert (c);
  g_assert (msg);

  if (!cdm_message_is_valid (msg))
    g_warning ("Message malformat or with different protocol version");

  c->last_msg_type = cdm_message_get_type (msg);

  switch (cdm_message_get_type (msg))
    {
    case CDM_MESSAGE_COREDUMP_NEW:
      c->id = cdm_message_get_session (msg);
      c->process_pid = cdm_message_get_process_pid (msg);
      c->process_exit_signal = cdm_message_get_process_exit_signal (msg);
      c->process_timestamp = cdm_message_get_process_timestamp (msg);
      c->process_name = g_strdup (cdm_message_get_process_name (msg));
      c->thread_name = g_strdup (cdm_message_get_thread_name (msg));

      g_info ("New crash id=%lx name=%s thread=%s pid=%d signal=%d", c->id, c->process_name,
              c->thread_name, (gint)c->process_pid, (gint)c->process_exit_signal);
      break;

    case CDM_MESSAGE_COREDUMP_UPDATE:
      c->process_crash_id = g_strdup (cdm_message_get_process_crash_id (msg));
      c->process_vector_id = g_strdup (cdm_message_get_process_vector_id (msg));
      c->process_context_id = g_strdup (cdm_message_get_process_context_id (msg));

      /* Crashhandler is running in host context so use current pid */
      tmp_id = get_pid_context_id (getpid ());
      if (g_strcmp0 (tmp_id, c->process_context_id) == 0)
        tmp_name = g_strdup (g_get_host_name ());
      else
        {
#ifdef WITH_LXC
          tmp_name = get_container_name_for_context (c->process_context_id);
#else
          tmp_name = g_strdup ("container");
#endif
        }

      g_info ("Update crash id=%lx crashID=%s vectorID=%s contextID=%s contextName=%s", c->id,
              c->process_crash_id, c->process_vector_id, c->process_context_id, tmp_name);

#ifdef WITH_DBUS_SERVICES
      cdm_dbusown_emit_new_crash (c->dbusown, c->process_name, tmp_name, c->process_crash_id);
#endif
      send_context_info (c, tmp_name);
      send_epilog (c, cdm_journal_epilog_get (c->journal, c->process_pid));
      break;

    case CDM_MESSAGE_COREDUMP_SUCCESS:
      c->coredump_file_path = g_strdup (cdm_message_get_coredump_file_path (msg));
      c->context_name = g_strdup (cdm_message_get_context_name (msg));
      c->lifecycle_state = g_strdup (cdm_message_get_lifecycle_state (msg));
      g_info ("Coredump id=%lx status OK", c->id);
      break;

    case CDM_MESSAGE_COREDUMP_FAILED:
      g_info ("Coredump id=%lx status FAILED", c->id);
      break;

    default:
      break;
    }
}

static void
archive_transfer_complete (gpointer cdmclient, const gchar *file_path)
{
  CdmClient *client = (CdmClient *)cdmclient;

  g_autoptr (GError) error = NULL;

  g_info ("Transfer complete for %s", file_path);
  cdm_journal_set_transfer (client->journal, file_path, TRUE, &error);

  if (error != NULL)
    g_warning ("Fail to set transfer complete flag for %s. Error %s", file_path, error->message);

  cdm_client_unref (client);
}

CdmClient *
cdm_client_new (gint clientfd, CdmTransfer *transfer, CdmJournal *journal)
{
  CdmClient *client = (CdmClient *)g_source_new (&client_source_funcs, sizeof (CdmClient));

  g_assert (client);

  g_ref_count_init (&client->rc);

  client->sockfd = clientfd;
  client->transfer = cdm_transfer_ref (transfer);
  client->journal = cdm_journal_ref (journal);

  g_source_set_callback (CDM_EVENT_SOURCE (client), G_SOURCE_FUNC (client_source_callback), client,
                         client_source_destroy_notify);
  g_source_attach (CDM_EVENT_SOURCE (client), NULL);

  client->tag
      = g_source_add_unix_fd (CDM_EVENT_SOURCE (client), client->sockfd, G_IO_IN | G_IO_PRI);

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
      cdm_transfer_unref (client->transfer);
      cdm_journal_unref (client->journal);

#ifdef WITH_GENIVI_NSM
      if (client->lifecycle != NULL)
        cdm_lifecycle_unref (client->lifecycle);
#endif
#ifdef WITH_DBUS_SERVICES
      if (client->dbusown != NULL)
        cdm_dbusown_unref (client->dbusown);
#endif

      g_free (client->lifecycle_state);
      g_free (client->process_name);
      g_free (client->thread_name);
      g_free (client->context_name);
      g_free (client->process_crash_id);
      g_free (client->process_vector_id);
      g_free (client->process_context_id);
      g_free (client->coredump_file_path);

      g_source_unref (CDM_EVENT_SOURCE (client));
    }
}

#ifdef WITH_GENIVI_NSM
void
cdm_client_set_lifecycle (CdmClient *client, CdmLifecycle *lifecycle)
{
  g_assert (client);
  g_assert (lifecycle);

  client->lifecycle = cdm_lifecycle_ref (lifecycle);
}
#endif
#ifdef WITH_DBUS_SERVICES
void
cdm_client_set_dbusown (CdmClient *client, CdmDBusOwn *dbusown)
{
  g_assert (client);
  g_assert (dbusown);

  client->dbusown = cdm_dbusown_ref (dbusown);
}
#endif
