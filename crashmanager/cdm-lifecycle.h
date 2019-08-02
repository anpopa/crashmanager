/* cdm-lifecycle.h
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

#ifndef CDM_LIFECYCLE_H
#define CDM_LIFECYCLE_H

#include "cdm-types.h"

#include <glib.h>

/**
 * @struct CdmLifecycle
 * @brief The CdmLifecycle opaque data structure
 */
typedef struct _CdmLifecycle {
  GSource *source;  /**< Event loop source */
  grefcount rc;     /**< Reference counter variable  */
} CdmLifecycle;

/*
 * @brief Create a new lifecycle object
 * @return On success return a new CdmLifecycle object otherwise return NULL
 */
CdmLifecycle *cdm_lifecycle_new (void);

/**
 * @brief Aquire lifecycle object
 * @param c Pointer to the lifecycle object
 */
CdmLifecycle *cdm_lifecycle_ref (CdmLifecycle *lifecycle);

/**
 * @brief Release lifecycle object
 * @param c Pointer to the lifecycle object
 */
void cdm_lifecycle_unref (CdmLifecycle *lifecycle);

/**
 * @brief Get object event loop source
 * @param c Pointer to the lifecycle object
 */
GSource *cdm_lifecycle_get_source (CdmLifecycle *lifecycle);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CdmLifecycle, cdm_lifecycle_unref);

#endif /* CDM_LIFECYCLE_H */
