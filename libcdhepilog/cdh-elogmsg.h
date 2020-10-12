/*
 * SPDX license identifier: MIT
 *
 * Copyright (c) 2020 Alin Popa
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \file cdh-elogmsg.h
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define CDM_ELOGMSG_PROTOCOL_VERSION (0x0001)
#define CDM_ELOGMSG_START_HASH (0xFCDF)

/**
 * @brief The elog message types
 */
typedef enum _CdmELogMessageType {
  CDM_ELOGMSG_INVALID,
  CDM_ELOGMSG_NEW
} CdmELogMessageType;

typedef struct _CdmELogMessageData {
  int64_t process_pid;
  int64_t process_sig;
} CdmELogMessageData;

/**
 * @brief The CdmMessageHdr opaque data structure
 */
typedef struct _CdmELogMessageHdr {
  uint16_t hsh;
  uint32_t version;
  CdmELogMessageType type;
  uint16_t size_of_arg1;
  uint16_t size_of_arg2;
  uint16_t size_of_arg3;
  uint16_t size_of_arg4;
} CdmELogMessageHdr;

typedef struct _CdmELogMessage {
  CdmELogMessageHdr  hdr;
  CdmELogMessageData data;
} CdmELogMessage;

/*
 * @brief Create a new message object
 * @param type The message type
 * @return The new allocated message object
 */
CdmELogMessage *        cdm_elogmsg_new                     (CdmELogMessageType type);

/*
 * @brief Release message
 * @param msg The message object
 */
void                    cdm_elogmsg_free                    (CdmELogMessage *msg);

/*
 * @brief Validate if the elogmsg object is consistent
 * @param msg The elogmsg object
 */
bool                    cdm_elogmsg_is_valid                (CdmELogMessage *msg);

/*
 * @brief Get elogmsg type
 * @param msg The elogmsg object
 */
CdmELogMessageType      cdm_elogmsg_get_type                (CdmELogMessage *msg);

/*
 * @brief Set process id
 * @param msg The message object
 * @param pid The process id
 */
void                    cdm_elogmsg_set_process_pid         (CdmELogMessage *msg,
                                                             int64_t pid);

/*
 * @brief Get process id
 * @param msg The message object
 * @return The process id
 */
int64_t                 cdm_elogmsg_get_process_pid          (CdmELogMessage *msg);

/*
 * @brief Set process exit signal
 * @param msg The message object
 * @param sig The process exit signal
 */
void                    cdm_elogmsg_set_process_exit_signal (CdmELogMessage *msg,
                                                             int64_t sig);

/*
 * @brief Get process exit signal
 * @param msg The message object
 * @return The process exit signal
 */
int64_t                 cdm_elogmsg_get_process_exit_signal (CdmELogMessage *msg);

/*
 * @brief Read data into elogmsg object
 * If elogmsg read has payload the data has to be released by the caller
 * @param msg The elogmsg object
 * @param fd File descriptor to read from
 *
 * @return CDM_STATUS_OK on success, CDM_STATUS_ERROR otherwise
 */
int                     cdm_elogmsg_read                    (int fd,
                                                             CdmELogMessage *msg);

/*
 * @brief Write data into elogmsg object
 * @param msg The elogmsg object
 * @param fd File descriptor to write to
 * @return CDM_STATUS_OK on success, CDM_STATUS_ERROR otherwise
 */
int                     cdm_elogmsg_write                   (int fd,
                                                             CdmELogMessage *msg);

#ifdef __cplusplus
}
#endif
