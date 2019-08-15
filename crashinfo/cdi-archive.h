/* cdi-archive.h
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

#include <glib.h>
#include <archive.h>
#include <archive_entry.h>

G_BEGIN_DECLS

/**
 * @struct CdiArchive
 * @brief The archive object
 */
typedef struct _CdiArchive {
  struct archive *archive;                       /**< Archive object  */
  grefcount rc;                                  /**< Reference counter variable  */
} CdiArchive;

/**
 * @brief Create a new CdiArchive object
 * @return A pointer to the new CdiArchive object
 */
CdiArchive *cdi_archive_new (void);

/**
 * @brief Aquire the archive object
 * @param context Pointer to the object
 * @return a pointer to archive object
 */
CdiArchive *cdi_archive_ref (CdiArchive *ar);

/**
 * @brief Release archive object
 * @param i Pointer to the cdh_context object
 */
void cdi_archive_unref (CdiArchive *ar);

/**
 * @brief Open archive for read
 * @fname Absoluet path to the archive file
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_read_open (CdiArchive *ar, const gchar *fname);

/**
 * @brief List archive content to stdout
 *        The archive has to be opened first
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_list_stdout (CdiArchive *ar);

/**
 * @brief Print information about crash archive
 *        The archive has to be opened first
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdi_archive_print_info (CdiArchive *ar);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdiArchive, cdi_archive_unref);

G_END_DECLS
