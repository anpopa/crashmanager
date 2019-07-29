/* cdh_archive.c
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

#include "cdh-archive.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

CdmStatus cdh_archive_open(CdhArchive *ar, const char *dst)
{
    CdmStatus status = CDM_STATUS_OK;

    assert(ar);
    assert(dst);

    memset(ar, 0, sizeof(CdhArchive));

    ar->out_file = gzopen(dst, ARCHIVE_OPEN_MODE);
    if (ar->out_file == Z_NULL) {
        cdhlog(LOG_ERR, "Cannot open output filename %s. Error %s", dst, strerror(errno));
        ar->out_file = 0;
        status = CDM_STATUS_ERROR;
    }

    return status;
}

CdmStatus cdh_archive_close(CdhArchive *ar)
{
    assert(ar);

    if (ar->out_file != NULL) {
        gzflush(ar->out_file, Z_FINISH);
        gzclose(ar->out_file);
        ar->out_file = NULL;
    }

    if (ar->in_stream != NULL) {
        (void)cdh_archive_stream_close(ar);
    }

    return CDM_STATUS_OK;
}

ssize_t cdh_archive_write(CdhArchive *ar, void *buf, size_t size)
{
    ssize_t sz = -1;

    assert(ar);
    assert(buf);

    if (ar->out_file) {
        sz = gzwrite(ar->out_file, buf, (unsigned int)size);
        if (sz < 0) {
            cdhlog(LOG_ERR, "Fail to write to coredump archive file. Error", strerror(errno));
        }
    }

    return sz;
}

CdmStatus cdh_archive_stream_open(CdhArchive *ar, const char *src)
{
    CdmStatus status = CDM_STATUS_OK;

    assert(ar);

    if (src == NULL) {
        ar->in_stream = stdin;
    } else if ((ar->in_stream = fopen(src, "rb")) == NULL) {
        cdhlog(LOG_ERR, "Cannot open filename %s. Error %s", src, strerror(errno));
        status = CDM_STATUS_ERROR;
    }

    return status;
}

CdmStatus cdh_archive_stream_close(CdhArchive *ar)
{
    assert(ar);

    if (ar->in_stream != NULL) {
        fclose(ar->in_stream);
        ar->in_stream = NULL;
    }

    return CDM_STATUS_OK;
}

CdmStatus cdh_archive_stream_read(CdhArchive *ar, void *buf, size_t size)
{
    size_t readsz;

    assert(ar);
    assert(ar->in_stream);
    assert(buf);

    if ((readsz = fread(buf, 1, size, ar->in_stream)) != size) {
        cdhlog(LOG_WARNING, "Cannot read %d bytes from archive input stream %s", size,
               strerror(errno));
        return CDM_STATUS_ERROR;
    }

    ar->in_stream_offset += readsz;

    return CDM_STATUS_OK;
}

CdmStatus cdh_archive_stream_read_all(CdhArchive *ar)
{
    assert(ar);
    assert(ar->in_stream);

    while (!feof(ar->in_stream)) {
        size_t readsz = fread(ar->in_read_buffer, 1, sizeof(ar->in_read_buffer), ar->in_stream);

        if (ar->out_file) {
            if (gzwrite(ar->out_file, ar->in_read_buffer, (unsigned int)readsz) !=
                (int)readsz) {
                cdhlog(LOG_ERR, "Fail to write to coredump archive file");
            }
        }

        ar->in_stream_offset += readsz;

        if (ferror(ar->in_stream)) {
            cdhlog(LOG_WARNING, "Error reading from the archive input stream: %s", strerror(errno));
            return CDM_STATUS_ERROR;
        }
    }

    return CDM_STATUS_OK;
}

CdmStatus cdh_archive_stream_move_to_offset(CdhArchive *ar, unsigned long offset)
{
    assert(ar);
    return cdh_archive_stream_move_ahead(ar, (offset - ar->in_stream_offset));
}

CdmStatus cdh_archive_stream_move_ahead(CdhArchive *ar, unsigned long nbbytes)
{
    size_t toread = nbbytes;

    assert(ar);
    assert(ar->in_stream);

    while (toread > 0) {
        size_t chunksz = toread > ARCHIVE_READ_BUFFER_SZ ? ARCHIVE_READ_BUFFER_SZ : toread;
        size_t readsz = fread(ar->in_read_buffer, 1, chunksz, ar->in_stream);

        if (readsz != chunksz) {
            cdhlog(LOG_WARNING, "Cannot move ahead by %d bytes from src. Read %lu bytes", nbbytes,
                   readsz);
            return CDM_STATUS_ERROR;
        }

        if (ar->out_file) {
            if (gzwrite(ar->out_file, ar->in_read_buffer, (unsigned int)readsz) !=
                (int)readsz) {
                cdhlog(LOG_ERR, "Fail to write to coredump archive file");
            }
        }

        toread -= chunksz;
    }

    ar->in_stream_offset += nbbytes;

    return CDM_STATUS_OK;
}

size_t cdh_archive_stream_get_offset(CdhArchive *ar)
{
    assert(ar);
    return ar->in_stream_offset;
}
