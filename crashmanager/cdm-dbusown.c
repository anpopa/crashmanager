/* cdm-dbusown.c
 *
 * Copyright 2020 Alin Popa <alin.popa@fxdata.ro>
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

#include "cdm-dbusown.h"
#include <gio/gio.h>

/**
 * @brief Post new event
 *
 * @param d A pointer to the dbusown object
 * @param e The new event
 */
static void post_dbusown_event (CdmDBusOwn *d, CdmDBusOwnEvent *e);

/**
 * @brief GSource prepare function
 */
static gboolean dbusown_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource dispatch function
 */
static gboolean dbusown_source_dispatch (GSource *source, GSourceFunc callback, gpointer _dbusown);
/**
 * @brief GSource callback function
 */
static gboolean dbusown_source_callback (gpointer _dbusown, gpointer _event);

/**
 * @brief GSource destroy notification callback function
 */
static void dbusown_source_destroy_notify (gpointer _dbusown);

/**
 * @brief Async queue destroy notification callback function
 */
static void dbusown_queue_destroy_notify (gpointer _dbusown);

/**
 * @brief Handle new crash emit request
 */
static void dbusown_emit_new_crash (CdmDBusOwn *d, const gchar *proc_name,
                                    const gchar *proc_context, const gchar *proc_crashid);
/**
 * @brief Handle method call
 */
static void handle_method_call (GDBusConnection *connection, const gchar *sender,
                                const gchar *object_path, const gchar *interface_name,
                                const gchar *method_name, GVariant *parameters,
                                GDBusMethodInvocation *invocation, gpointer user_data);

/**
 * @brief Handle get property
 */
static GVariant *handle_get_property (GDBusConnection *connection, const gchar *sender,
                                      const gchar *object_path, const gchar *interface_name,
                                      const gchar *property_name, GError **error,
                                      gpointer user_data);

/**
 * @brief Handle set property
 */
static gboolean handle_set_property (GDBusConnection *connection, const gchar *sender,
                                     const gchar *object_path, const gchar *interface_name,
                                     const gchar *property_name, GVariant *value, GError **error,
                                     gpointer user_data);

/**
 * @brief On DBUS acquired
 */
static void on_bus_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data);

/**
 * @brief On DBUS name acquired
 */
static void on_name_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data);

/**
 * @brief On DBUS name lost
 */
static void on_name_lost (GDBusConnection *connection, const gchar *name, gpointer user_data);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs dbusown_source_funcs = {
  dbusown_source_prepare, NULL, dbusown_source_dispatch, NULL, NULL, NULL,
};

/**
 * @brief GDBUS interface vtable
 */
static const GDBusInterfaceVTable interface_vtable
    = { handle_method_call, handle_get_property, handle_set_property, { 0, 0, 0, 0, 0, 0, 0, 0 } };

static GDBusNodeInfo *introspection_data = NULL;

/* Introspection data for the service we are exporting */
static const gchar introspection_xml[]
    = "<node>"
      "  <interface name='ro.fxdata.crashmanager.Crashes'>"
      "    <signal name='NewCrash'>"
      "      <annotation name='ro.fxdata.crashmanager.Annotation' value='Onsignal'/>"
      "      <arg type='s' name='process_name'/>"
      "      <arg type='s' name='process_context'/>"
      "      <arg type='s' name='process_crashid'/>"
      "    </signal>"
      "  </interface>"
      "</node>";

static void
post_dbusown_event (CdmDBusOwn *d, CdmDBusOwnEvent *e)
{
  g_assert (d);
  g_assert (e);

  g_async_queue_push (d->queue, e);
}

static gboolean
dbusown_source_prepare (GSource *source, gint *timeout)
{
  CdmDBusOwn *d = (CdmDBusOwn *)source;

  CDM_UNUSED (timeout);

  return (g_async_queue_length (d->queue) > 0);
}

static gboolean
dbusown_source_dispatch (GSource *source, GSourceFunc callback, gpointer _dbusown)
{
  CdmDBusOwn *d = (CdmDBusOwn *)source;
  gpointer e = g_async_queue_try_pop (d->queue);

  CDM_UNUSED (callback);
  CDM_UNUSED (_dbusown);

  if (e == NULL)
    return G_SOURCE_CONTINUE;

  return d->callback (d, e) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
dbusown_source_callback (gpointer _dbusown, gpointer _event)
{
  CdmDBusOwn *d = (CdmDBusOwn *)_dbusown;
  CdmDBusOwnEvent *e = (CdmDBusOwnEvent *)_event;

  g_assert (d);
  g_assert (e);

  switch (e->type)
    {
    case DBUS_OWN_EMIT_NEW_CRASH:
      dbusown_emit_new_crash (d, e->proc_name, e->proc_context, e->proc_crashid);
      break;

    default:
      break;
    }

  g_free (e->proc_name);
  g_free (e->proc_context);
  g_free (e->proc_crashid);
  g_free (e);

  return TRUE;
}

static void
dbusown_source_destroy_notify (gpointer _dbusown)
{
  CdmDBusOwn *d = (CdmDBusOwn *)_dbusown;

  g_assert (d);
  g_debug ("DBusOwn destroy notification");

  cdm_dbusown_unref (d);
}

static void
dbusown_queue_destroy_notify (gpointer _dbusown)
{
  CDM_UNUSED (_dbusown);
  g_debug ("DBusOwn queue destroy notification");
}

static void
dbusown_emit_new_crash (CdmDBusOwn *d, const gchar *proc_name, const gchar *proc_context,
                        const gchar *proc_crashid)
{
  GError *error = NULL;

  g_assert (d);
  g_assert (proc_name);
  g_assert (proc_context);
  g_assert (proc_crashid);

  g_debug ("Notify DBUS subscribers for '%s' crash in context '%s' with crashid '%s'", proc_name,
           proc_context, proc_crashid);

  if (d->connection != NULL)
    {
      g_dbus_connection_emit_signal (
          d->connection, NULL, "/ro/fxdata/crashmanager", "ro.fxdata.crashmanager.Crashes",
          "NewCrash", g_variant_new ("(sss)", proc_name, proc_context, proc_crashid), &error);

      if (error != NULL)
        g_warning ("Fail emit NewCrash signal. Error %s", error->message);
    }
}

static void
handle_method_call (GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                    const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                    GDBusMethodInvocation *invocation, gpointer user_data)
{
  CDM_UNUSED (connection);
  CDM_UNUSED (sender);
  CDM_UNUSED (object_path);
  CDM_UNUSED (interface_name);
  CDM_UNUSED (method_name);
  CDM_UNUSED (parameters);
  CDM_UNUSED (invocation);
  CDM_UNUSED (user_data);
}

static GVariant *
handle_get_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                     const gchar *interface_name, const gchar *property_name, GError **error,
                     gpointer user_data)
{
  GVariant *ret = NULL;

  CDM_UNUSED (connection);
  CDM_UNUSED (sender);
  CDM_UNUSED (object_path);
  CDM_UNUSED (interface_name);
  CDM_UNUSED (property_name);
  CDM_UNUSED (error);
  CDM_UNUSED (user_data);

  return ret;
}

static gboolean
handle_set_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                     const gchar *interface_name, const gchar *property_name, GVariant *value,
                     GError **error, gpointer user_data)
{
  CDM_UNUSED (connection);
  CDM_UNUSED (sender);
  CDM_UNUSED (object_path);
  CDM_UNUSED (interface_name);
  CDM_UNUSED (property_name);
  CDM_UNUSED (value);
  CDM_UNUSED (error);
  CDM_UNUSED (user_data);

  return TRUE;
}

static void
on_bus_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
  CdmDBusOwn *d = (CdmDBusOwn *)user_data;
  guint registration_id;

  CDM_UNUSED (name);

  d->connection = connection;
  registration_id
      = g_dbus_connection_register_object (connection, "/ro/fxdata/crashmanager",
                                           introspection_data->interfaces[0], &interface_vtable, d,
                                           /* user_data */
                                           NULL,
                                           /* user_data_free_func */
                                           NULL); /* GError** */
  g_assert (registration_id > 0);
}

static void
on_name_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
  CDM_UNUSED (user_data);
  CDM_UNUSED (connection);

  g_debug ("DBUS own name '%s' acquired", name);
}

static void
on_name_lost (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
  CdmDBusOwn *d = (CdmDBusOwn *)user_data;

  CDM_UNUSED (connection);

  d->connection = NULL;
  g_debug ("DBUS own name '%s' lost", name);
}

CdmDBusOwn *
cdm_dbusown_new (CdmOptions *options)
{
  CdmDBusOwn *d = (CdmDBusOwn *)g_source_new (&dbusown_source_funcs, sizeof (CdmDBusOwn));

  g_assert (d);
  g_assert (options);

  g_ref_count_init (&d->rc);

  d->options = cdm_options_ref (options);
  d->queue = g_async_queue_new_full (dbusown_queue_destroy_notify);
  d->callback = dbusown_source_callback;

  g_source_set_callback (CDM_EVENT_SOURCE (d), NULL, d, dbusown_source_destroy_notify);
  g_source_attach (CDM_EVENT_SOURCE (d), NULL);

  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (introspection_data != NULL);

  d->owner_id
      = g_bus_own_name (G_BUS_TYPE_SYSTEM, "ro.fxdata.crashmanager", G_BUS_NAME_OWNER_FLAGS_NONE,
                        on_bus_acquired, on_name_acquired, on_name_lost, d, NULL);

  return d;
}

CdmDBusOwn *
cdm_dbusown_ref (CdmDBusOwn *d)
{
  g_assert (d);
  g_ref_count_inc (&d->rc);
  return d;
}

void
cdm_dbusown_unref (CdmDBusOwn *d)
{
  g_assert (d);

  if (g_ref_count_dec (&d->rc) == TRUE)
    {
      if (d->options != NULL)
        cdm_options_unref (d->options);

      if (d->owner_id > 0)
        g_bus_unown_name (d->owner_id);

      if (introspection_data != NULL)
        g_dbus_node_info_unref (introspection_data);

      g_async_queue_unref (d->queue);
      g_source_unref (CDM_EVENT_SOURCE (d));
    }
}

void
cdm_dbusown_emit_new_crash (CdmDBusOwn *d, const gchar *pname, const gchar *context,
                            const gchar *crashid)
{
  CdmDBusOwnEvent *e = NULL;

  g_assert (d);
  g_assert (pname);
  g_assert (context);
  g_assert (crashid);

  e = g_new0 (CdmDBusOwnEvent, 1);

  e->type = DBUS_OWN_EMIT_NEW_CRASH;
  e->proc_name = g_strdup (pname);
  e->proc_context = g_strdup (context);
  e->proc_crashid = g_strdup (crashid);

  post_dbusown_event (d, e);
}
