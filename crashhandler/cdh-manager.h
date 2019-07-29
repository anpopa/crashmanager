/*
 *
 * Copyright (C) 2019, Alin Popa. All rights reserved.
 *
 * This file is part of Coredumper
 * The module provides an unified interface to core manager (crashhandler or core manager)
 * It can be used to send messages to core manager if available
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \copyright Copyright © 2019, Alin Popa
 *
 */

#ifndef CDH_MANAGER_H
#define CDH_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cd_msg.h"
#include "cd_opts.h"
#include "cd_types.h"
#include <assert.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef MANAGER_SELECT_TIMEOUT
#define MANAGER_SELECT_TIMEOUT 3
#endif

/**
 * @struct cdh_manager
 * @brief The coredump handler manager object
 */
typedef struct cdh_manager {
    int sfd;                  /**< Server (manager) unix domain file descriptor */
    bool connected;           /**< Server connection state */
    struct sockaddr_un saddr; /**< Server socket addr struct */
    const char *coredir;      /**< Path to coredump database */
#ifdef WITH_CRASHHANDLER
    cd_corenew_t regdata;    /**< Registration data to send */
    cd_coreupdate_t upddata; /**< Update data to send */
#endif
    cd_options_t *opts; /**< Reference to options object */
} CDHManager;

/**
 * @brief Initialize pre-allocated CDHManager object
 *
 * @param c Manager object
 * @param opts Pointer to global options object
 *
 * @return CD_STATUS_OK on success
 */
CDStatus cdh_manager_init(CDHManager *c, CDOptions *opts);

/**
 * @brief Connect to cdh manager
 * @param c Manager object
 * @return CD_STATUS_OK on success
 */
CDStatus cdh_manager_connect(CDHManager *c);

/**
 * @brief Disconnect from cdh manager
 * @param c Manager object
 * @return CD_STATUS_OK on success
 */
CDStatus cdh_manager_disconnect(CDHManager *c);

/**
 * @brief Get connection state
 * @param c Manager object
 * @return True if connected
 */
bool cdh_manager_connected(CDHManager *c);

/**
 * @brief Set coredump archive database path
 * @param c Manager object
 * @param coredir Coredumo database path
 */
void cdh_manager_set_coredir(CDHManager *c, const char *coredir);

/**
 * @brief Send message to cdh manager
 *
 * @param c Manager object
 * @param m Message to send
 *
 * @return True if connected
 */
CDStatus cdh_manager_send(CDHManager *c, CDMessage *m);

#ifdef __cplusplus
}
#endif

#endif /* CDH_MANAGER_H */
