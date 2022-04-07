/* cdm-dbusown.h
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
 */

#pragma once

#include "cdm-options.h"
#include "cdm-types.h"

#include <gio/gio.h>
#include <glib.h>

G_BEGIN_DECLS

/**
 * @enum dbusown_event_data
 * @brief DBusOwn event data type
 */
typedef enum _DBusOwnEventType
{
  DBUS_OWN_EMIT_NEW_CRASH
} DBusOwnEventType;

typedef gboolean (*CdmDBusOwnCallback) (gpointer _dbusown, gpointer _event);

/**
 * @struct CdmDBusOwnEvent
 * @brief The file transfer event
 */
typedef struct _CdmDBusOwnEvent
{
  DBusOwnEventType type; /**< The event type the element holds */
  gchar *proc_name;
  gchar *proc_context;
  gchar *proc_crashid;
} CdmDBusOwnEvent;

/**
 * @struct CdmDBusOwn
 * @brief The CdmDBusOwn opaque data structure
 */
typedef struct _CdmDBusOwn
{
  GSource source;              /**< Event loop source */
  GAsyncQueue *queue;          /**< Async queue */
  CdmDBusOwnCallback callback; /**< Callback function */
  grefcount rc;                /**< Reference counter variable  */
  CdmOptions *options;
  guint owner_id;              /**< DBUS owner id */
  GDBusConnection *connection; /**< DBUS connection */
} CdmDBusOwn;

/*
 * @brief Create a new dbusown object
 * @return On success return a new CdmDBusOwn object otherwise return NULL
 */
CdmDBusOwn *cdm_dbusown_new (CdmOptions *options);

/**
 * @brief Aquire dbusown object
 * @param d Pointer to the dbusown object
 */
CdmDBusOwn *cdm_dbusown_ref (CdmDBusOwn *d);

/**
 * @brief Release dbusown object
 * @param d Pointer to the dbusown object
 */
void cdm_dbusown_unref (CdmDBusOwn *d);

/**
 * @brief Build DBus proxy
 * @param d Pointer to the dbusown object
 */
void cdm_dbusown_emit_new_crash (CdmDBusOwn *d, const gchar *pname, const gchar *ctxname,
                                 const gchar *crashid);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmDBusOwn, cdm_dbusown_unref);

G_END_DECLS
