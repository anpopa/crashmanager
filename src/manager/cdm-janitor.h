/* cdm-janitor.h
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

#ifndef CDM_JANITOR_H
#define CDM_JANITOR_H

#include "cdm-types.h"

#include <glib.h>

/**
 * @struct CdmJanitor
 * @brief The CdmJanitor opaque data structure
 */
typedef struct _CdmJanitor {
  GSource *source;  /**< Event loop source */
  grefcount rc;     /**< Reference counter variable  */
} CdmJanitor;

/*
 * @brief Create a new janitor object
 * @return On success return a new CdmJanitor object otherwise return NULL
 */
CdmJanitor *cdm_janitor_new (void);

/**
 * @brief Aquire janitor object
 * @param c Pointer to the janitor object
 */
CdmJanitor *cdm_janitor_ref (CdmJanitor *janitor);

/**
 * @brief Release janitor object
 * @param c Pointer to the janitor object
 */
void cdm_janitor_unref (CdmJanitor *janitor);

/**
 * @brief Get object event loop source
 * @param c Pointer to the janitor object
 */
GSource *cdm_janitor_get_source (CdmJanitor *janitor);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmJanitor, cdm_janitor_unref);

#endif /* CDM_JANITOR_H */
