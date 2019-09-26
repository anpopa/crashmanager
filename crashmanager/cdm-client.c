/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019 Alin Popa
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
 * \author Alin Popa <alin.popa@bmw.de>
 * \file cdm-client.c
 */

#include "cdm-client.h"
#include "cdm-utils.h"
#include "cdm-transfer.h"
#include "cdm-defaults.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
 * @brief Process message from crashhandler instance
 */
static void process_message (CdmClient *c, CdmMessage *msg);

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
client_source_prepare (GSource *source,
                       gint *timeout)
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
client_source_dispatch (GSource *source,
                        GSourceFunc callback,
                        gpointer user_data)
{
  CDM_UNUSED (source);

  if (callback == NULL)
    return G_SOURCE_CONTINUE;

  return callback (user_data) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
client_source_callback (gpointer data)
{
  CdmClient *client = (CdmClient *)data;
  gboolean status = TRUE;
  CdmMessage msg;

  g_assert (client);

  cdm_message_init (&msg, CDM_CORE_UNKNOWN, 0);

  if (cdm_message_read (client->sockfd, &msg) != CDM_STATUS_OK)
    {
      g_debug ("Cannot read from client socket %d", client->sockfd);

#ifdef WITH_GENIVI_NSM
      if (client->init_data != NULL)
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
      CdmMessageType type = cdm_message_get_type (&msg);

      process_message (client, &msg);

      if ((type == CDM_CORE_COMPLETE) || (type == CDM_CORE_FAILED))
        {
          g_autoptr (GError) error = NULL;
          guint64 dbid;

          if (!client->init_data || !client->update_data || !client->complete_data)
            {
              g_warning ("Client data from crashhandler incomplete for client %d",
                         client->sockfd);
              status = FALSE;
            }
          else
            {
              dbid = cdm_journal_add_crash (client->journal,
                                            client->init_data->pname,
                                            client->update_data->crashid,
                                            client->update_data->vectorid,
                                            client->update_data->contextid,
                                            client->complete_data->context_name,
                                            client->complete_data->lifecycle_state,
                                            client->complete_data->core_file,
                                            client->init_data->pid,
                                            client->init_data->coresig,
                                            client->init_data->tstamp,
                                            &error);

              if (error != NULL)
                g_warning ("Fail to add new crash entry in database %s", error->message);
              else
                g_debug ("New crash entry added to database with id %016lX", dbid);

              /* even if we fail to add to the database we try to transfer the file */
              cdm_transfer_file (client->transfer,
                                 client->complete_data->core_file,
                                 archive_transfer_complete,
                                 cdm_client_ref (client));
            }
#ifdef WITH_GENIVI_NSM
          if (cdm_lifecycle_set_session_state (client->lifecycle, LC_SESSION_INACTIVE)
              != CDM_STATUS_OK)
            {
              g_warning ("Fail decrement session lifecycle counter");
            }
#endif
        }
      else
        {
          if (type == CDM_CORE_NEW)
            {
#ifdef WITH_GENIVI_NSM
              if (cdm_lifecycle_set_session_state (client->lifecycle, LC_SESSION_ACTIVE)
                  != CDM_STATUS_OK)
                {
                  g_warning ("Fail increment session lifecycle counter");
                }
#endif
            }
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
        {
          ctx_str = g_strdup (proc_ns_buf);
        }
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
      gchar *name = names[i];

      if (name == NULL || container == NULL)
        continue;

      if (container->is_running (container))
        {
          g_autofree gchar *tmp_id = NULL;
          pid_t pid;

          pid = container->init_pid (container);
          tmp_id = get_pid_context_id (pid);

          if (g_strcmp0 (tmp_id, ctxid) != 0)
            {
              container_name = g_strdup_printf ("%s", name);
              found = true;
            }
        }
    }

  for (int i = 0; i < count; i++)
    {
      free (names[i]);
      free (active_containers[i]);
    }

  return container_name;
}
#endif

static void
send_context_info (CdmClient *c, const gchar *context_name)
{
  CdmMessageDataContextInfo msg_data;
  CdmMessage msg;

  g_assert (c);
  g_assert (context_name);

  cdm_message_init (&msg, CDM_CONTEXT_INFO, 0);

#ifdef WITH_GENIVI_NSM
  /* set proper value from NSM */
  snprintf (msg_data.lifecycle_state, CDM_LIFECYCLESTATE_LEN, "running");
#else
  snprintf (msg_data.lifecycle_state, CDM_LIFECYCLESTATE_LEN, "running");
#endif
  snprintf (msg_data.context_name, CDM_CRASHCONTEXT_LEN, "%s", context_name);

  cdm_message_set_version (&msg, CDM_VERSION);
  cdm_message_set_data (&msg, &msg_data, sizeof(msg_data));

  if (cdm_message_write (c->sockfd, &msg) == CDM_STATUS_ERROR)
    g_warning ("Failed to send context information to client");
}

static void
process_message (CdmClient *c,
                 CdmMessage *msg)
{
  g_autofree gchar *tmp_id = NULL;
  g_autofree gchar *tmp_name = NULL;

  g_assert (c);
  g_assert (msg);

  if (strncmp ((char *)msg->hdr.version, CDM_BUILDTIME_VERSION, CDM_VERSION_STRING_LEN) != 0)
    g_warning ("Using different cdh header than coredumper");

  switch (msg->hdr.type)
    {
    case CDM_CORE_NEW:
      g_assert (msg->data);

      c->last_msg_type = CDM_CORE_NEW;
      c->id = msg->hdr.session;
      c->init_data = (CdmMessageDataNew *)msg->data;

      g_info ("New crash id=%lx name=%s thread=%s pid=%d signal=%d",
              c->id, c->init_data->pname,
              c->init_data->tname,
              (gint)c->init_data->pid,
              (gint)c->init_data->coresig);
      break;

    case CDM_CORE_UPDATE:
      g_assert (msg->data);

      c->last_msg_type = CDM_CORE_UPDATE;
      c->update_data = (CdmMessageDataUpdate *)msg->data;

      /* Crashhandler is running in host context so use current pid */
      tmp_id = get_pid_context_id (getpid ());

      if (g_strcmp0 (tmp_id, c->update_data->contextid) == 0)
        {
          tmp_name = g_strdup (g_get_host_name ());
        }
      else
        {
#ifdef WITH_LXC
          tmp_name = get_container_name_for_context (c->update_data->contextid);
#else
          tmp_name = "Container";
#endif
        }

      g_info ("Update crash id=%lx crashID=%s vectorID=%s contextID=%s contextName=%s",
              c->id,
              c->update_data->crashid,
              c->update_data->vectorid,
              c->update_data->contextid,
              tmp_name);

      send_context_info (c, tmp_name);
      break;

    case CDM_CORE_COMPLETE:
      c->last_msg_type = CDM_CORE_COMPLETE;
      c->complete_data = (CdmMessageDataComplete *)msg->data;
      g_info ("Coredump id=%lx status OK", c->id);
      break;

    case CDM_CORE_FAILED:
      c->last_msg_type = CDM_CORE_FAILED;
      g_info ("Coredump id=%lx status FAILED", c->id);
      break;

    default:
      break;
    }
}

static void
archive_transfer_complete (gpointer cdmclient,
                           const gchar *file_path)
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
cdm_client_new (gint clientfd,
                CdmTransfer *transfer,
                CdmJournal *journal)
{
  CdmClient *client = (CdmClient *)g_source_new (&client_source_funcs, sizeof(CdmClient));

  g_assert (client);

  g_ref_count_init (&client->rc);

  client->sockfd = clientfd;
  client->transfer = cdm_transfer_ref (transfer);
  client->journal = cdm_journal_ref (journal);

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

      g_free (client->init_data);
      g_free (client->update_data);
      g_free (client->complete_data);

      g_source_unref (CDM_EVENT_SOURCE (client));
    }
}

#ifdef WITH_GENIVI_NSM
void
cdm_client_set_lifecycle (CdmClient *client,
                          CdmLifecycle *lifecycle)
{
  g_assert (client);
  g_assert (lifecycle);

  client->lifecycle = cdm_lifecycle_ref (lifecycle);
}
#endif
