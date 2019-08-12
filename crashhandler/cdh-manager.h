/* cdh-manager.h
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

#include "cdm-message.h"
#include "cdm-options.h"
#include "cdm-types.h"

#include <glib.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

G_BEGIN_DECLS

#ifndef MANAGER_SELECT_TIMEOUT
#define MANAGER_SELECT_TIMEOUT 3
#endif

/**
 * @struct cdh_manager
 * @brief The coredump handler manager object
 */
typedef struct _CdhManager {
  grefcount rc;                /**< Reference counter variable  */
  gint sfd;                    /**< Server (manager) unix domain file descriptor */
  gboolean connected;          /**< Server connection state */
  struct sockaddr_un saddr;    /**< Server socket addr struct */
  CdmOptions *opts;            /**< Reference to options object */
} CdhManager;

/**
 * @brief Create a new CdhManager object
 * @param opts Pointer to global options object
 * @return A new CdhManager objects
 */
CdhManager *cdh_manager_new (CdmOptions *opts);

/**
 * @brief Aquire CdhManager object
 * @param c Manager object
 */
CdhManager *cdh_manager_ref (CdhManager *c);

/**
 * @brief Release CdhManager object
 * @param c Manager object
 */
void cdh_manager_unref (CdhManager *c);

/**
 * @brief Connect to cdh manager
 * @param c Manager object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_manager_connect (CdhManager *c);

/**
 * @brief Disconnect from cdh manager
 * @param c Manager object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_manager_disconnect (CdhManager *c);

/**
 * @brief Get connection state
 * @param c Manager object
 * @return True if connected
 */
gboolean cdh_manager_connected (CdhManager *c);

/**
 * @brief Send message to cdh manager
 *
 * @param c Manager object
 * @param m Message to send
 *
 * @return True if connected
 */
CdmStatus cdh_manager_send (CdhManager *c, CdmMessage *m);

G_END_DECLS
