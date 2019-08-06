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

#ifndef CDM_SERVER_H
#define CDM_SERVER_H

#include "cdm-types.h"
#include "cdm-options.h"
#include "cdm-transfer.h"

#include <glib.h>

/**
 * @struct CdmServer
 * @brief The CdmServer opaque data structure
 */
typedef struct _CdmServer {
  GSource *source;  /**< Event loop source */
  CdmOptions *opts; /**< Own reference to global options */
  grefcount rc;     /**< Reference counter variable  */
  gpointer tag;     /**< Unix server socket tag  */
  gint sockfd;      /**< Module file descriptor (server listen fd) */
  CdmTransfer *transfer; /**< Own a reference to transfer object */
} CdmServer;

/*
 * @brief Create a new server object
 * @return On success return a new CdmServer object otherwise return NULL
 */
CdmServer *cdm_server_new (CdmOptions *opts, CdmTransfer *transfer);

/**
 * @brief Aquire server object
 * @param c Pointer to the server object
 */
CdmServer *cdm_server_ref (CdmServer *server);

/**
 * @brief Start the server an listen for clients
 * @param c Pointer to the server object
 */
CdmStatus cdm_server_bind_and_listen (CdmServer *server);

/**
 * @brief Release server object
 * @param c Pointer to the server object
 */
void cdm_server_unref (CdmServer *server);

/**
 * @brief Get object event loop source
 * @param c Pointer to the server object
 */
GSource *cdm_server_get_source (CdmServer *server);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmServer, cdm_server_unref);

#endif /* CDM_SERVER_H */
