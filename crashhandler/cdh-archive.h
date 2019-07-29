/* cdh_archive.h
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

#ifndef CDH_ARCHIVE_H
#define CDH_ARCHIVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cdm-types.h"
#include <stdio.h>
#include <zlib.h>

#ifndef ARCHIVE_READ_BUFFER_SZ
#define ARCHIVE_READ_BUFFER_SZ 1024 * 128
#endif

#ifndef ARCHIVE_OPEN_MODE
#define ARCHIVE_OPEN_MODE "wb1"
#endif

/**
 * @struct cdh_archive
 * @brief The archive object
 */
typedef struct _CdhArchive {
    FILE *in_stream; /**< The input file stream */
    gzFile out_file; /**< GZip file */

    size_t in_stream_offset; /**< Current offset */
    uint8_t in_read_buffer[ARCHIVE_READ_BUFFER_SZ] /**< Read buffer */;
} CdhArchive;

/**
 * @brief Initialize CdhArchive object and create output file
 *
 * @param ar The CdhArchive object
 * @param dst Path to output archive file.
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_open(CdhArchive *ar, const char *dst);

/**
 * @brief Close cdh archive
 * @param ar The CdhArchive object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_close(CdhArchive *ar);

/**
 * @brief Write data to output file
 *
 * @param ar The CdhArchive object
 * @param buf Buffer to store read data
 * @param size Maximum size to read (buffer size)
 *
 * @return CDM_STATUS_OK on success
 */
ssize_t cdh_archive_write(CdhArchive *ar, void *buf, size_t size);

/**
 * @brief Open input stream
 *
 * @param ar The CdhArchive object
 * @param src Path to input stream . NULL for stdin
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_stream_open(CdhArchive *ar, const char *src);

/**
 * @brief Open input stream
 *
 * @param ar The CdhArchive object
 * @param src Path to input stream . NULL for stdin
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_stream_close(CdhArchive *ar);

/**
 * @brief Read into buffer and advence up to size
 *
 * @param ar The CdhArchive object
 * @param buf Buffer to store read data
 * @param size Maximum size to read (buffer size)
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_stream_read(CdhArchive *ar, void *buf, size_t size);

/**
 * @brief Read and save all input stream
 * @param ar The CdhArchive object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_stream_read_all(CdhArchive *ar);

/**
 * @brief Advence to offset in input stream and dump everything to output up to the offset
 *
 * @param ar The CdhArchive object
 * @param offset Offset to advence
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_stream_move_to_offset(CdhArchive *ar, unsigned long offset);

/**
 * @brief Process next bytes from input stream
 *
 * @param ar The CdhArchive object
 * @param nbbytes Number of bytes to process
 *
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_archive_stream_move_ahead(CdhArchive *ar, unsigned long nbbytes);

/**
 * @brief Current offset getter
 * @param ar The CdhArchive object
 * @return The current offset value
 */
size_t cdh_archive_stream_get_offset(CdhArchive *ar);

#ifdef __cplusplus
}
#endif

#endif /* CDH_ARCHIVE_H */
