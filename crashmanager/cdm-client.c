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
#include "cdm-utils.h"
#include "cdm-transfer.h"

#include <sys/types.h>
#include <unistd.h>

gboolean client_source_prepare (GSource *source, gint *timeout);
gboolean client_source_check (GSource *source);
gboolean client_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data);
static gboolean client_source_callback (gpointer data);
static void client_source_destroy_notify (gpointer data);
static void process_message (CdmClient *c, CdmMessage *msg);
static gchar *get_pid_context_id (pid_t pid);
static void archive_transfer_complete (gpointer cdmclient, const gchar *file_path);
#ifdef WITH_LXC
static gchar *get_container_name_for_context (const gchar *ctxid);
#endif

static GSourceFuncs client_source_funcs =
{
  client_source_prepare,
  NULL,
  client_source_dispatch,
  NULL,
  NULL,
  NULL,
};

gboolean
client_source_prepare (GSource *source, gint *timeout)
{
  CDM_UNUSED (source);
  *timeout = -1;
  return FALSE;
}

gboolean
client_source_check (GSource *source)
{
  CDM_UNUSED (source);
  return TRUE;
}

gboolean
client_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data)
{
  CDM_UNUSED (source);

  if (callback == NULL)
    {
      return G_SOURCE_CONTINUE;
    }

  return callback (user_data) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
client_source_callback (gpointer data)
{
  CdmClient *client = (CdmClient *)data;
  CdmMessage msg;

  g_assert (client);

  cdm_message_init (&msg, CDM_CORE_UNKNOWN, 0);

  if (cdm_message_read (client->sockfd, &msg) != CDM_STATUS_OK)
    {
      g_debug ("Cannot read from client socket %d", client->sockfd);
      return FALSE;
    }
  else
    {
      CdmMessageType type = cdm_message_get_type (&msg);

      process_message (client, &msg);

      if ((type == CDM_CORE_COMPLETE) || (type == CDM_CORE_FAILED))
        {
          GError *error = NULL;
          guint64 dbid;

          if (!client->init_data || !client->update_data || !client->complete_data)
            {
              g_warning ("Client data from crashhandler incomplete for client %d", client->sockfd);
              return FALSE;
            }

          dbid = cdm_journal_add_crash (client->journal,
                                        client->init_data->pname,
                                        client->update_data->crashid,
                                        client->update_data->vectorid,
                                        client->update_data->contextid,
                                        client->complete_data->core_file,
                                        client->init_data->pid,
                                        client->init_data->coresig,
                                        client->init_data->tstamp,
                                        &error);

          if (error != NULL)
            {
              g_warning ("Fail to add new crash entry in database %s", error->message);
              g_error_free (error);
            }
          else
            {
              g_debug ("New crash entry added to database with id %016lX", dbid);
            }

          cdm_transfer_file (client->transfer, client->complete_data->core_file, archive_transfer_complete, cdm_client_ref (client));
        }
    }

  return TRUE;
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
    {
      return NULL;
    }

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
        {
          continue;
        }

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
process_message (CdmClient *c, CdmMessage *msg)
{
  g_autofree gchar *tmp_id = NULL;
  g_autofree gchar *tmp_name = NULL;

  g_assert (c);
  g_assert (msg);

  if (strncmp ((char *)msg->hdr.version, CDM_BUILDTIME_VERSION, CDM_VERSION_STRING_LEN) != 0)
    {
      g_warning ("Using different cdh header than coredumper");
    }

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

      if (g_strcmp0 (tmp_id, c->update_data->contextid) != 0)
        {
          tmp_name = g_strdup_printf ("Host");
        }
      else
        {
#ifdef WITH_LXC
          tmp_name = get_container_name_for_context (c->update_data->contextid);
#endif
        }

      g_info ("Update crash id=%lx crashID=%s vectorID=%s contextID=%s contextName=\"%s\"",
              c->id,
              c->update_data->crashid,
              c->update_data->vectorid,
              c->update_data->contextid,
              tmp_name);
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
archive_transfer_complete (gpointer cdmclient, const gchar *file_path)
{
  CdmClient *client = (CdmClient *)cdmclient;
  GError *error = NULL;

  g_debug ("Archive transfer complete for %d : %s", client->sockfd, file_path);
  cdm_journal_set_transfer (client->journal, file_path, TRUE, &error);

  if (error != NULL)
    {
      g_warning ("Fail to set transfer complete for %s. Error %s", file_path, error->message);
      g_error_free (error);
    }

  cdm_client_unref (client);
}

CdmClient *
cdm_client_new (gint clientfd, CdmTransfer *transfer, CdmJournal *journal)
{
  CdmClient *client = (CdmClient *)g_source_new (&client_source_funcs, sizeof(CdmClient));

  g_assert (client);

  g_ref_count_init (&client->rc);

  client->sockfd = clientfd;
  client->transfer = cdm_transfer_ref (transfer);
  client->journal = cdm_journal_ref (journal);

  g_source_set_callback (CDM_EVENT_SOURCE (client), G_SOURCE_FUNC (client_source_callback), client, client_source_destroy_notify);
  g_source_attach (CDM_EVENT_SOURCE (client), NULL); /* attach the source to the default context */

  client->tag = g_source_add_unix_fd (CDM_EVENT_SOURCE (client), client->sockfd, G_IO_IN | G_IO_PRI);

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

      g_free (client->init_data);
      g_free (client->update_data);
      g_free (client->complete_data);

      g_source_unref (CDM_EVENT_SOURCE (client));
    }
}
