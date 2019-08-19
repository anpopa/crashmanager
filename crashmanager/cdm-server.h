/* cdm-server.h
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

#pragma once

#include "cdm-types.h"
#include "cdm-options.h"
#include "cdm-transfer.h"
#include "cdm-journal.h"
#ifdef WITH_GENIVI_NSM
#include "cdm-lifecycle.h"
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * @struct CdmServer
 * @brief The CdmServer opaque data structure
 */
typedef struct _CdmServer {
  GSource source;  /**< Event loop source */
  grefcount rc;     /**< Reference counter variable  */
  gpointer tag;     /**< Unix server socket tag  */
  gint sockfd;      /**< Module file descriptor (server listen fd) */
  CdmOptions *options; /**< Own reference to global options */
  CdmTransfer *transfer; /**< Own a reference to transfer object */
  CdmJournal *journal; /**< Own a reference to journal object */
#ifdef WITH_GENIVI_NSM
  CdmJournal *lifecycle; /**< Own a reference to the lifecycle object */
#endif
} CdmServer;

/*
 * @brief Create a new server object
 *
 * @param options A pointer to the CdmOptions object created by the main
 * application
 * @param transfer A pointer to the CdmTransfer object created by the main
 * application
 * @param journal A pointer to the CdmJournal object created by the main
 * application
 *
 * @return On success return a new CdmServer object otherwise return NULL
 */
CdmServer *cdm_server_new (CdmOptions *options, CdmTransfer *transfer, CdmJournal *journal, GError **error);

/**
 * @brief Aquire server object
 * @param server Pointer to the server object
 * @return The referenced server object
 */
CdmServer *cdm_server_ref (CdmServer *server);

/**
 * @brief Start the server an listen for clients
 * @param server Pointer to the server object
 * @return If server starts listening the function return CDM_STATUS_OK
 */
CdmStatus cdm_server_bind_and_listen (CdmServer *server);

/**
 * @brief Release server object
 * @param server Pointer to the server object
 */
void cdm_server_unref (CdmServer *server);

#ifdef WITH_GENIVI_NSM
/**
 * @brief Set lifecycle object
 * @param server Pointer to the client object
 * @param lifecycle Pointer to the lifecycle object
 */
void cdm_server_set_lifecycle (CdmServer *server, CdmLifecycle *lifecycle);
#endif

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmServer, cdm_server_unref);

G_END_DECLS
