/* cdm_msg.h
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

#ifndef CDM_MSG_H
#define CDM_MSG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cdm_msgtyp.h"
#include "cdm_types.h"


/*
 * @brief Initialize a pre-allocated mesage object
 *
 * @param m The message object
 * @param type The message type
 * @param session Unique session identifier
 */
void cdm_msg_init(CDMessage *m, CDMessageType type, uint16 session);

/*
 * @brief Message data setter
 *
 * @param m The message object
 * @param data The message data to set
 * @param size The message data size
 */
void cdm_msg_set_data(CDMessage *m, void *data, uint32 size);

/*
 * @brief Message data free
 *
 * The function release the data field of the message if set
 *
 * @param m The message object
 */
void cdm_msg_free_data(CDMessage *m);

/*
 * @brief Validate if the message object is consistent
 * @param m The message object
 */
bool cdm_msg_is_valid(CDMessage *m);

/*
 * @brief Get message type
 * @param m The message object
 */
CDMessageType cdm_msg_getype(CDMessage *m);

/*
 * @brief Set message version
 *
 * @param m The message object
 * @param version Version string
 *
 * @return CD_STATUS_OK on success, CD_STATUS_ERROR otherwise
 */
CDStatus cdm_msg_set_version(CDMessage *m, const char *version);

/*
 * @brief Read data into message object
 *
 * If message read has payload the data has to be released by the caller
 *
 * @param m The message object
 * @param fd File descriptor to read from
 *
 * @return CD_STATUS_OK on success, CD_STATUS_ERROR otherwise
 */
CDStatus cdm_msg_read(int fd, CDMessage *m);

/*
 * @brief Write data into message object
 *
 * @param m The message object
 * @param fd File descriptor to write to
 *
 * @return CD_STATUS_OK on success, CD_STATUS_ERROR otherwise
 */
CDStatus cdm_msg_write(int fd, CDMessage *m);

#ifdef __cplusplus
}
#endif

#endif /* CDM_MSG_H */
