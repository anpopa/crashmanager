/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019-2020 Alin Popa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \file cdh-archive.h
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
  struct archive *archive;                          /**< Archive object  */
  struct archive_entry *archive_entry;              /**< Current archive entry */
  time_t artime;                                    /**< Archive/crash time */

  grefcount rc;                                     /**< Reference counter */

  gboolean file_active;                             /**< Archive open for write */
  gchar *file_name;                                 /**< Output file name  */
  gssize file_size;                                 /**< Output file size  */
  gssize file_chunk_sz;                             /**< Output file chunk size  */
  gssize file_chunk_cnt;                            /**< Output file chunk count  */
  gssize file_write_sz;                             /**< Output writen size  */

  FILE *in_stream;                                  /**< The input file stream */
  gsize in_stream_offset;                           /**< Current offset */
  guint8 in_read_buffer[ARCHIVE_READ_BUFFER_SZ];    /**< Read buffer */
} CdhArchive;

/**
 * @brief Create a new CdhArchive object
 * @return A pointer to the new CdhArchive object
 */
CdhArchive *            cdh_archive_new                     (void);

/**
 * @brief Aquire the archive object
 * @param context Pointer to the object
 * @return a pointer to archive object
 */
CdhArchive *            cdh_archive_ref                     (CdhArchive *ar);

/**
 * @brief Release archive object
 * @param i Pointer to the cdh_context object
 */
void                    cdh_archive_unref                   (CdhArchive *ar);

/**
 * @brief Initialize pre-allocated CdhArchive object
 * @param ar The CdhArchive object
 * @param dst Path to output archive file. 
 * The basename extension will define the archive format.
 * @return CDM_STATUS_OK on success
 */
CdmStatus               cdh_archive_open                    (CdhArchive *ar, 
                                                             const gchar *dst, 
                                                             time_t artime);

/**
 * @brief Close cdh archive
 * @param ar The CdhArchive object
 * @return CDM_STATUS_OK on success
 */
CdmStatus               cdh_archive_close                   (CdhArchive *ar);

/**
 * @brief Create and add a new file to archive
 * @param ar The CdhArchive object
 * @param dst The filename
 * @param size The size on bytes of the input data
 * @return CDM_STATUS_OK on success
 */
CdmStatus               cdh_archive_create_file                 (CdhArchive *ar, 
                                                                 const gchar *dst, 
                                                                 gsize size);

/**
 * @brief Write file data for created file
 * @param ar The CdhArchive object
 * @param dst The filename
 * @return CDM_STATUS_OK on success
 */
CdmStatus               cdh_archive_write_file                  (CdhArchive *ar, 
                                                                 const void *buf, 
                                                                 gsize size);

/**
 * @brief Finish archive file
 * @param ar The CdhArchive object
 * @return CDM_STATUS_OK on success
 */
CdmStatus               cdh_archive_finish_file             (CdhArchive *ar);

/**
 * @brief Add a file from filesystem
 * @param ar The CdhArchive object
 * @param src The source file to read
 * @param dst The destination filename in the archive. 
 * If NULL then domain separated name will be constructed
 * @return CDM_STATUS_OK on success
 */

CdmStatus               cdh_archive_add_system_file         (CdhArchive *ar, 
                                                             const gchar *src, 
                                                             const gchar *dst);

/**
 * @brief Start archive input stream processing
 * @param ar The CdhArchive object
 * @param src The source file for the file stream. 
 * If NULL then STDIN will be used
 * @param dst The destination filename in the archive
 * @return CDM_STATUS_OK on success
 */
CdmStatus               cdh_archive_stream_open             (CdhArchive *ar, 
                                                             const gchar *src, 
                                                             const gchar *dst, 
                                                             gsize split_size);

/**
 * @brief Read into buffer and advence up to size
 * @param ar The CdhArchive object
 * @param buf Buffer to store read data
 * @param size Maximum size to read (buffer size)
 * @return CDM_STATUS_OK on success
 */
CdmStatus               cdh_archive_stream_read             (CdhArchive *ar, 
                                                             void *buf, 
                                                             gsize size);

/**
 * @brief Read and save all remaining input stream
 * @param ar The CdhArchive object
 * @param dummy_write If true will force the output stream to write 0 for remaining data
 * @return CDM_STATUS_OK on success
 */
CdmStatus               cdh_archive_stream_read_all         (CdhArchive *ar, 
                                                             gboolean dummy_write);

/**
 * @brief Advence to offset in input stream and dump everything 
 * to output up to the offset
 * @param ar The CdhArchive object
 * @param offset Offset to advence
 * @return CDM_STATUS_OK on success
 */
CdmStatus               cdh_archive_stream_move_to_offset   (CdhArchive *ar, 
                                                             gulong offset);

/**
 * @brief Process next bytes from input stream
 * @param ar The CdhArchive object
 * @param nbbytes Number of bytes to process
 * @return CDM_STATUS_OK on success
 */
CdmStatus               cdh_archive_stream_move_ahead       (CdhArchive *ar, 
                                                             gulong nbbytes);

/**
 * @brief Current offset getter
 * @param ar The CdhArchive object
 * @return The current offset value
 */
gsize                   cdh_archive_stream_get_offset       (CdhArchive *ar);

/**
 * @brief Finish input stream processing
 * @param ar The CdhArchive object
 * @return CDM_STATUS_OK on success
 */
CdmStatus               cdh_archive_stream_close            (CdhArchive *ar);

/**
 * @brief Check if the a stream is open
 * @param ar The CdhArchive object
 * @return True if a file stream is open
 */
gboolean                cdh_archive_is_file_active          (CdhArchive *ar);

G_END_DECLS
