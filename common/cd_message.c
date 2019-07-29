/* cd_message.c
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

#include "cd_message.h"

#include <assert.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void cd_message_init(CDMessage *m, CDMessageType type, uint16 session)
{
    assert(m);

    memset(m, 0, sizeof(CDMessage));
    m->hdr.type = type;
    m->hdr.hsh = CD_MESSAGE_START_HASH;
    m->hdr.session = session;
}

void cd_message_set_data(CDMessage *m, void *data, uint32 size)
{
    assert(m);
    assert(data);

    m->hdr.data_size = size;
    m->data = data;
}

void cd_message_free_data(CDMessage *m)
{
    assert(m);

    if (m->data != NULL) {
        free(m->data);
    }
}

bool cd_message_is_valid(CDMessage *m)
{
    if (m == NULL) {
        return false;
    }

    if (m->hdr.hsh != CD_MESSAGE_START_HASH) {
        return false;
    }

    return true;
}

CDMessageType cd_message_getype(CDMessage *m)
{
    assert(m);
    return m->hdr.type;
}

CDStatus cd_message_set_version(CDMessage *m, const char *version)
{
    assert(m);

    snprintf((char *)m->hdr.version, CD_VERSION_STRING_LEN, "%s", version);

    return CD_STATUS_OK;
}

CDStatus cd_message_read(int fd, CDMessage *m)
{
    ssize_t sz;

    assert(m);

    sz = read(fd, &m->hdr, sizeof(CDMessageHdr));

    if (sz != sizeof(CDMessageHdr) || !cd_message_is_valid(m)) {
        return CD_STATUS_ERROR;
    }

    m->data = calloc(1, m->hdr.data_size);
    if (m->data == NULL) {
        return CD_STATUS_ERROR;
    }

    sz = read(fd, m->data, m->hdr.data_size);

    return sz == m->hdr.data_size ? CD_STATUS_OK : CD_STATUS_ERROR;
}

CDStatus cd_message_write(int fd, CDMessage *m)
{
    ssize_t sz;

    assert(m);

    if (!cd_message_is_valid(m)) {
        return CD_STATUS_ERROR;
    }

    sz = write(fd, &m->hdr, sizeof(CDMessageHdr));

    if (sz != sizeof(CDMessageHdr)) {
        return CD_STATUS_ERROR;
    }

    sz = write(fd, m->data, m->hdr.data_size);

    return sz == m->hdr.data_size ? CD_STATUS_OK : CD_STATUS_ERROR;
}
