/*
 *
 * Copyright (C) 2019, Alin Popa. All rights reserved.
 *
 * This file is part of Coredumper
 * This module provides a wrapper to NSM Consumer to be used by cdm_lifecycle module
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \copyright Copyright © 2019, Alin Popa
 *
 */

#ifndef NSM_CONSUMER_H
#define NSM_CONSUMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmgr_types.h"
#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @enum lifecycle_session_state
 * @brief Session state enum type
 */
typedef enum lifecycle_session_state {
    LC_SESSION_ACTIVE,
    LC_SESSION_INACTIVE
} lifecycle_session_state_t;

/**
 * @enum lifecylce_registration
 */
typedef enum lifecycle_registration_state {
    LC_REGISTERED,
    LC_UNREGISTERED,
    LC_PENDING
} lifecycle_registration_state_t;

/**
 * @enum lifecylce_proxy
 */
typedef enum lifecycle_proxy_state {
    LC_PROXY_AVAILABLE,
    LC_PROXY_UNAVAILABLE,
} lifecycle_proxy_state_t;

/**
 * @enum lifecycle_event_type
 * @brief Lifecycle event enum type
 */
typedef enum lifecycle_event_type {
    LC_EVENT_PROXY_UPDATE,
    LC_EVENT_REGISTRATION_UPDATE,
    LC_EVENT_SESSION_UPDATE
} lifecycle_event_type_t;

/**
 * @struct nsm_consumer
 * @brief The nsm consumer object
 */
typedef struct nsm_consumer {
    void *client; /**< Reference to client object */

    void *private_data; /**< Reference to private data object */

    void (*proxy_availability_cb)(void *client, lifecycle_proxy_state_t state,
                                  int error); /**< Proxy availability callback */
    void (*registration_state_cb)(void *client, lifecycle_registration_state_t state,
                                  int error); /**< Registration state callback */
    void (*session_state_cb)(void *client, lifecycle_session_state_t state,
                             int error); /**< Session state callback */
} nsm_consumer_t;

/**
 * @brief Create a new nsm_consumer_t object initialized for NSM communication
 *
 * @return A pointer of nsm_consumer_t which should be released with nsm_consumer_free()
 *         by the caller for deinitialization.
 *         If the object cannot be created (eg. initialization fail) a NULL pointer is returned
 */
nsm_consumer_t *nsm_consumer_new(const char *session_name, const char *session_owner);

/**
 * @brief Release a nsm_consumer object
 * @param n A pointer to the nsm_consumer object allocated with nsm_consumer_new()
 */
void nsm_consumer_free(nsm_consumer_t *n);

/**
 * @brief Client reference setter
 * @param n A pointer to the nsm_consumer object
 * @param client A pointer to client object
 */
void nsm_consumer_set_client(nsm_consumer_t *n, void *client);

/**
 * @brief Callback setter for proxy availability status
 * @param n A pointer to the nsm_consumer object
 * @param proxy_availability_cb Callback function
 */
void nsm_consumer_set_proxy_cb(nsm_consumer_t *n,
                               void (*proxy_availability_cb)(void *client,
                                                             lifecycle_proxy_state_t state,
                                                             int error));

/**
 * @brief Callback setter for NSM registration status
 * @param n A pointer to the nsm_consumer object
 * @param registration_state_cb Callback function
 */
void nsm_consumer_set_registration_cb(
    nsm_consumer_t *n,
    void (*registration_state_cb)(void *client, lifecycle_registration_state_t state, int error));

/**
 * @brief Callback setter for NSM session status
 * @param n A pointer to the nsm_consumer object
 * @param session_state_cb Callback function
 */
void nsm_consumer_set_session_cb(nsm_consumer_t *n,
                                 void (*session_state_cb)(void *client,
                                                          lifecycle_session_state_t state,
                                                          int error));

/**
 * @brief Lifecycle proxy state getter
 * @param n A pointer to the nsm_consumer object
 * @return Current proxy state set in nsm_consumer
 */
lifecycle_proxy_state_t nsm_consumer_get_proxy_state(nsm_consumer_t *n);

/**
 * @brief Lifecycle session state getter
 * @param n A pointer to the nsm_consumer object
 * @return Current session state set in nsm_consumer
 */
lifecycle_session_state_t nsm_consumer_get_session_state(nsm_consumer_t *n);

/**
 * @brief Lifecycle registration state getter
 * @param n A pointer to the nsm_consumer object
 * @return Current registration state set in nsm_consumer
 */
lifecycle_registration_state_t nsm_consumer_get_registration_state(nsm_consumer_t *n);

/**
 * @brief Lifecycle proxy state setter
 * @param n A pointer to the nsm_consumer object
 * @param The new proxy state to set in nsm_consumer
 */
void nsm_consumer_set_proxy_state(nsm_consumer_t *n, lifecycle_proxy_state_t state);

/**
 * @brief Lifecycle session state setter
 * @param n A pointer to the nsm_consumer object
 * @param The new session state to set in nsm_consumer
 */
void nsm_consumer_set_session_state(nsm_consumer_t *n, lifecycle_session_state_t state);

/**
 * @brief Lifecycle session state setter
 * @param n A pointer to the nsm_consumer object
 * @param The new registration state to set in nsm_consumer
 */
void nsm_consumer_set_registration_state(nsm_consumer_t *n, lifecycle_registration_state_t state);

/**
 * @brief Request NSM consumer to build the CommonAPI proxy.
 *        If the request is possible the proxy_availability_cb will be called.
 *        Is up to the user to update the new proxy state via the setter function
 * @param n A pointer to the nsm_consumer object
 * @return The status of the request
 */
int nsm_consumer_build_proxy(nsm_consumer_t *n);

/**
 * @brief Request NSM consumer to register.
 *        If the request is possible the registration_state_cb will be called.
 *        Is up to the user to update the new registration state via the setter function
 * @param n A pointer to the nsm_consumer object
 * @return The status of the request
 */
int nsm_consumer_register(nsm_consumer_t *n);

/**
 * @brief Request NSM consumer a new state.
 *        If the request is possible the session_state_cb will be called.
 *        Is up to the user to update the new session state via the setter function
 * @param n A pointer to the nsm_consumer object
 * @return The status of the request
 */
int nsm_consumer_request_state(nsm_consumer_t *n, lifecycle_session_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* NSM_CONSUMER_H */
