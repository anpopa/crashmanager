/* cdm_msg.c
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

#include "cdm_msg.h"

#include <assert.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void cdm_msg_init(CdmMsg *m, CdmMsgType type, uint16 session)
{
    assert(m);

    memset(m, 0, sizeof(cdm_msg));
    m->hdr.type = type;
    m->hdr.hsh = CDM_MSG_START_HASH;
    m->hdr.session = session;
}

void cdm_msg_set_data(CdmMsg *m, void *data, uint32 size)
{
    assert(m);
    assert(data);

    m->hdr.data_size = size;
    m->data = data;
}

void cdm_msg_free_data(CdmMsg *m)
{
    assert(m);

    if (m->data != NULL) {
        free(m->data);
    }
}

bool cdm_msg_is_valid(CdmMsg *m)
{
    if (m == NULL) {
        return false;
    }

    if (m->hdr.hsh != CDM_MSG_START_HASH) {
        return false;
    }

    return true;
}

CdmMsgType cdm_msg_getype(CdmMsg *m)
{
    assert(m);
    return m->hdr.type;
}

CdmStatus cdm_msg_set_version(CdmMsg *m, const char *version)
{
    assert(m);

    snprintf((char *)m->hdr.version, CDM_VERSION_STRING_LEN, "%s", version);

    return CDM_OK;
}

CdmStatus cdm_msg_read(int fd, CdmMsg *m)
{
    ssize sz;

    assert(m);

    sz = read(fd, &m->hdr, sizeof(CdmMsgHdr));

    if (sz != sizeof(CdmMsgHdr) || !cdm_msg_is_valid(m)) {
        return CDM_ERROR;
    }

    m->data = calloc(1, m->hdr.data_size);
    if (m->data == NULL) {
        return CDM_ERROR;
    }

    sz = read(fd, m->data, m->hdr.data_size);

    return sz == m->hdr.data_size ? CDM_OK : CDM_ERROR;
}

CdmStatus cdm_msg_write(int fd, CdmMsg *m)
{
    ssize sz;

    assert(m);

    if (!cdm_msg_is_valid(m)) {
        return CDM_ERROR;
    }

    sz = write(fd, &m->hdr, sizeof(CdmMsgHdr));

    if (sz != sizeof(CdmMsgHdr)) {
        return CDM_ERROR;
    }

    sz = write(fd, m->data, m->hdr.data_size);

    return sz == m->hdr.data_size ? CDM_OK : CDM_ERROR;
}
