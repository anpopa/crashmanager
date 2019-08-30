/* cdh-coredump.h
 *
 * Copyright 2019 Alin Popa <alin.popa@bmw.de>
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

#include "cdh-context.h"
#include "cdh-archive.h"
#if defined(WITH_CRASHMANAGER)
#include "cdh-manager.h"
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The coredump generation object
 */
typedef struct _CdhCoredump {
  CdhContext *context; /**< Context object owned */
  CdhArchive *archive; /**< Archive object owned */
#if defined(WITH_CRASHMANAGER)
  CdhManager *manager; /**< Manager object owned */
#endif
  grefcount rc;        /**< Reference counter variable  */
} CdhCoredump;

/**
 * @brief Create a new CdhCoredump object
 * @return A pointer to the new object
 */
CdhCoredump *cdh_coredump_new (CdhContext *context, CdhArchive *archive);

/**
 * @brief Aquire CdhCoredump object
 * @param cd Pointer to the CdhCoredump object
 * @return Pointer to the CdhCoredump object
 */
CdhCoredump *cdh_coredump_ref (CdhCoredump *cd);

/**
 * @brief Release a CdhCoredump object
 * @param cd Pointer to the cdh_context object
 */
void cdh_coredump_unref (CdhCoredump *cd);

#if defined(WITH_CRASHMANAGER)
/* @brief Set coredump manager object
 * @param cd Coredump object
 * @param manager Manager object
 */
void cdh_coredump_set_manager (CdhCoredump *cd, CdhManager *manager);
#endif

/* @brief Generate coredump file
 * @param cd Coredump object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_coredump_generate (CdhCoredump *cd);

G_END_DECLS
