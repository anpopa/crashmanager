/* cdh-archive.h
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
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <archive.h>
#include <archive_entry.h>
#include <time.h>

G_BEGIN_DECLS

#ifndef ARCHIVE_READ_BUFFER_SZ
#define ARCHIVE_READ_BUFFER_SZ 1024 * 128
#endif

/**
 * @struct CdhArchive
 * @brief The archive object
 */
typedef struct _CdhArchive {
  struct archive *archive;                       /**< Archive object  */
  struct archive_entry *archive_entry;           /**< Current archive entry */
  time_t artime;                                 /**< Archive time (process crash time) */

  grefcount rc;                                  /**< Reference counter variable  */

  gboolean file_active;                          /**< Archive open for write */
  gchar *file_name;                              /**< Current output file size  */
  gssize file_size;                              /**< Current output file size  */
  gssize file_chunk_sz;                          /**< Current output file chunk size  */
  gssize file_chunk_cnt;                         /**< Current output file chunk count  */
  gssize file_write_sz;                          /**< Current output writen size  */

  FILE *in_stream;                               /**< The input file stream */
  gsize in_stream_offset;                        /**< Current offset */
  guint8 in_read_buffer[ARCHIVE_READ_BUFFER_SZ]; /**< Read buffer */
} CdhArchive;

/**
 * @brief Create a new CdhArchive object
 * @return A pointer to the new CdhArchive object
 */
CdhArchive *cdh_archive_new (void);

/**
 * @brief Aquire the archive object
 * @param context Pointer to the object
 * @return a pointer to archive object
 */
CdhArchive *cdh_archive_ref (CdhArchive *ar);

/**
 * @brief Release archive object
 * @param i Pointer to the cdh_context object
 */
void cdh_archive_unref (CdhArchive *ar);

/**
 * @brief Initialize pre-allocated CdhArchive object
 *
 * @param ar The CdhArchive object
 * @param dst Path to output archive file. The basename extension will define the
 *        archive format.
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_open (CdhArchive *ar, const gchar *dst, time_t artime);

/**
 * @brief Close cdh archive
 * @param ar The CdhArchive object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_close (CdhArchive *ar);

/**
 * @brief Create and add a new file to archive
 *
 * @param ar The CdhArchive object
 * @param dst The filename
 * @param size The size on bytes of the input data
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_create_file (CdhArchive *ar, const gchar *dst, gsize size);

/**
 * @brief Write file data for created file
 *
 * @param ar The CdhArchive object
 * @param dst The filename
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_write_file (CdhArchive *ar, const void *buf, gsize size);

/**
 * @brief Finish archive file
 * @param ar The CdhArchive object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_finish_file (CdhArchive *ar);

/**
 * @brief Add a file from filesystem
 *
 * @param ar The CdhArchive object
 * @param src The source file to read
 * @param dst The destination filename in the archive. If NULL then domain separated name
 *            will be constructed
 *
 * @return CDM_STATUS_OK on success
 */

CdmStatus cdh_archive_add_system_file (CdhArchive *ar, const gchar *src, const gchar *dst);

/**
 * @brief Start archive input stream processing
 *
 * @param ar The CdhArchive object
 * @param src The source file for the file stream. If NULL then STDIN will be used
 * @param dst The destination filename in the archive
 *
 * @return CDM_STATUS_OK on success
 */

CdmStatus cdh_archive_stream_open (CdhArchive *ar, const gchar *src, const gchar *dst, gsize split_size);

/**
 * @brief Read into buffer and advence up to size
 *
 * @param ar The CdhArchive object
 * @param buf Buffer to store read data
 * @param size Maximum size to read (buffer size)
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_stream_read (CdhArchive *ar, void *buf, gsize size);

/**
 * @brief Read and save all input stream
 * @param ar The CdhArchive object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_stream_read_all (CdhArchive *ar);

/**
 * @brief Advence to offset in input stream and dump everything to output up to the offset
 *
 * @param ar The CdhArchive object
 * @param offset Offset to advence
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_stream_move_to_offset (CdhArchive *ar, gulong offset);

/**
 * @brief Process next bytes from input stream
 *
 * @param ar The CdhArchive object
 * @param nbbytes Number of bytes to process
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_stream_move_ahead (CdhArchive *ar, gulong nbbytes);

/**
 * @brief Current offset getter
 * @param ar The CdhArchive object
 * @return The current offset value
 */
gsize cdh_archive_stream_get_offset (CdhArchive *ar);

/**
 * @brief Finish input stream processing
 * @param ar The CdhArchive object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_stream_close (CdhArchive *ar);

/**
 * @brief Check if the a stream is open
 * @param ar The CdhArchive object
 * @return True if a file stream is open
 */
gboolean cdh_archive_is_file_active (CdhArchive *ar);

G_END_DECLS
