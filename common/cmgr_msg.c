/* cmgr_msg.c
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

#include "cmgr_msg.h"

#include <assert.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void cmgr_msg_init(cmgr_msg_t *m, cmgr_msgtype_t type, uint16_t session)
{
    assert(m);

    memset(m, 0, sizeof(cmgr_msg_t));
    m->hdr.type = type;
    m->hdr.hsh = CMGR_MSG_START_HASH;
    m->hdr.session = session;
}

void cmgr_msg_set_data(cmgr_msg_t *m, void *data, uint32_t size)
{
    assert(m);
    assert(data);

    m->hdr.data_size = size;
    m->data = data;
}

void cmgr_msg_free_data(cmgr_msg_t *m)
{
    assert(m);

    if (m->data != NULL) {
        free(m->data);
    }
}

bool cmgr_msg_is_valid(cmgr_msg_t *m)
{
    if (m == NULL) {
        return false;
    }

    if (m->hdr.hsh != CMGR_MSG_START_HASH) {
        return false;
    }

    return true;
}

cmgr_msgtype_t cmgr_msg_get_type(cmgr_msg_t *m)
{
    assert(m);
    return m->hdr.type;
}

int cmgr_msg_set_version(cmgr_msg_t *m, const char *version)
{
    assert(m);

    snprintf((char *)m->hdr.version, CMGR_VERSION_STRING_LEN, "%s", version);

    return (0);
}

int cmgr_msg_read(int fd, cmgr_msg_t *m)
{
    ssize_t sz;

    assert(m);

    sz = read(fd, &m->hdr, sizeof(cmgr_msghdr_t));

    if (sz != sizeof(cmgr_msghdr_t) || !cmgr_msg_is_valid(m)) {
        return (-1);
    }

    m->data = calloc(1, m->hdr.data_size);
    if (m->data == NULL) {
        return (-1);
    }

    sz = read(fd, m->data, m->hdr.data_size);

    return sz == m->hdr.data_size ? (0) : (-1);
}

int cmgr_msg_write(int fd, cmgr_msg_t *m)
{
    ssize_t sz;

    assert(m);

    if (!cmgr_msg_is_valid(m)) {
        return (-1);
    }

    sz = write(fd, &m->hdr, sizeof(cmgr_msghdr_t));

    if (sz != sizeof(cmgr_msghdr_t)) {
        return (-1);
    }

    sz = write(fd, m->data, m->hdr.data_size);

    return sz == m->hdr.data_size ? (0) : (-1);
}
