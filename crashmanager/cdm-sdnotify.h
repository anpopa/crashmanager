/* cdm-sdnotify.h
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

#include "cdm-types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The CdmSDNotify opaque data structure
 */
typedef struct _CdmSDNotify {
  GSource *source;  /**< Event loop source */
  grefcount rc;     /**< Reference counter variable  */
} CdmSDNotify;

/*
 * @brief Create a new sdnotify object
 * @return On success return a new CdmSDNotify object
 */
CdmSDNotify *cdm_sdnotify_new (void);

/**
 * @brief Aquire sdnotify object
 * @param sdnotify Pointer to the sdnotify object
 * @return The referenced sdnotify object
 */
CdmSDNotify *cdm_sdnotify_ref (CdmSDNotify *sdnotify);

/**
 * @brief Release sdnotify object
 * @param sdnotify Pointer to the sdnotify object
 */
void cdm_sdnotify_unref (CdmSDNotify *sdnotify);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmSDNotify, cdm_sdnotify_unref);

G_END_DECLS
