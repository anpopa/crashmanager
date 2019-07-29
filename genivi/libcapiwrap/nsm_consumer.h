/* nsm_consumer.h
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

#ifndef NSM_CONSUMER_H
#define NSM_CONSUMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cdm_types.h"

#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @enum LifecycleSessionState
 * @brief Session state enum type
 */
typedef enum _LifecycleSessionState {
    LC_SESSION_ACTIVE,
    LC_SESSION_INACTIVE
} LifecycleSessionState;

/**
 * @enum lifecylce_registration
 */
typedef enum _LifecycleRegistrationState {
    LC_REGISTERED,
    LC_UNREGISTERED,
    LC_PENDING
} LifecycleRegistrationState;

/**
 * @enum lifecylce_proxy
 */
typedef enum _LifecycleProxyState {
    LC_PROXY_AVAILABLE,
    LC_PROXY_UNAVAILABLE,
} LifecycleProxyState;

/**
 * @enum LifecycleEventType
 * @brief Lifecycle event enum type
 */
typedef enum _LifecycleEventType {
    LC_EVENT_PROXY_UPDATE,
    LC_EVENT_REGISTRATION_UPDATE,
    LC_EVENT_SESSION_UPDATE
} LifecycleEventType;

/**
 * @struct NSMConsumer
 * @brief The nsm consumer object
 */
typedef struct _NSMConsumer {
    void *client; /**< Reference to client object */

    void *private_data; /**< Reference to private data object */

    void (*proxy_availability_cb)(void *client, LifecycleProxyState state,
                                  CDStatus error); /**< Proxy availability callback */
    void (*registration_state_cb)(void *client, LifecycleRegistrationState state,
                                  CDStatus error); /**< Registration state callback */
    void (*session_state_cb)(void *client, LifecycleSessionState state,
                             CDStatus error); /**< Session state callback */
} NSMConsumer;

/**
 * @brief Create a new NSMConsumer object initialized for NSM communication
 *
 * @return A pointer of NSMConsumer which should be released with nsm_consumer_free()
 *         by the caller for deinitialization.
 *         If the object cannot be created (eg. initialization fail) a NULL pointer is returned
 */
NSMConsumer *nsm_consumer_new(const char *session_name, const char *session_owner);

/**
 * @brief Release a NSMConsumer object
 * @param n A pointer to the NSMConsumer object allocated with nsm_consumer_new()
 */
void nsm_consumer_free(NSMConsumer *n);

/**
 * @brief Client reference setter
 * @param n A pointer to the NSMConsumer object
 * @param client A pointer to client object
 */
void nsm_consumer_set_client(NSMConsumer *n, void *client);

/**
 * @brief Callback setter for proxy availability status
 * @param n A pointer to the NSMConsumer object
 * @param proxy_availability_cb Callback function
 */
void nsm_consumer_set_proxy_cb(NSMConsumer *n,
                               void (*proxy_availability_cb)(void *client,
                                                             LifecycleProxyState state,
                                                             CDStatus error));

/**
 * @brief Callback setter for NSM registration status
 * @param n A pointer to the NSMConsumer object
 * @param registration_state_cb Callback function
 */
void nsm_consumer_set_registration_cb(
    NSMConsumer *n,
    void (*registration_state_cb)(void *client, LifecycleRegistrationState state,
                                  CDStatus error));

/**
 * @brief Callback setter for NSM session status
 * @param n A pointer to the NSMConsumer object
 * @param session_state_cb Callback function
 */
void nsm_consumer_set_session_cb(NSMConsumer *n,
                                 void (*session_state_cb)(void *client,
                                                          LifecycleSessionState state,
                                                          CDStatus error));

/**
 * @brief Lifecycle proxy state getter
 * @param n A pointer to the NSMConsumer object
 * @return Current proxy state set in NSMConsumer
 */
LifecycleProxyState nsm_consumer_get_proxy_state(NSMConsumer *n);

/**
 * @brief Lifecycle session state getter
 * @param n A pointer to the NSMConsumer object
 * @return Current session state set in NSMConsumer
 */
LifecycleSessionState nsm_consumer_get_session_state(NSMConsumer *n);

/**
 * @brief Lifecycle registration state getter
 * @param n A pointer to the NSMConsumer object
 * @return Current registration state set in NSMConsumer
 */
LifecycleRegistrationState nsm_consumer_get_registration_state(NSMConsumer *n);

/**
 * @brief Lifecycle proxy state setter
 * @param n A pointer to the NSMConsumer object
 * @param The new proxy state to set in NSMConsumer
 */
void nsm_consumer_set_proxy_state(NSMConsumer *n, LifecycleProxyState state);

/**
 * @brief Lifecycle session state setter
 * @param n A pointer to the NSMConsumer object
 * @param The new session state to set in NSMConsumer
 */
void nsm_consumer_set_session_state(NSMConsumer *n, LifecycleSessionState state);

/**
 * @brief Lifecycle session state setter
 * @param n A pointer to the NSMConsumer object
 * @param The new registration state to set in NSMConsumer
 */
void nsm_consumer_set_registration_state(NSMConsumer *n, LifecycleRegistrationState state);

/**
 * @brief Request NSM consumer to build the CommonAPI proxy.
 *        If the request is possible the proxy_availability_cb will be called.
 *        Is up to the user to update the new proxy state via the setter function
 * @param n A pointer to the NSMConsumer object
 * @return The status of the request
 */
CDStatus nsm_consumer_build_proxy(NSMConsumer *n);

/**
 * @brief Request NSM consumer to register.
 *        If the request is possible the registration_state_cb will be called.
 *        Is up to the user to update the new registration state via the setter function
 * @param n A pointer to the NSMConsumer object
 * @return The status of the request
 */
CDStatus nsm_consumer_register(NSMConsumer *n);

/**
 * @brief Request NSM consumer a new state.
 *        If the request is possible the session_state_cb will be called.
 *        Is up to the user to update the new session state via the setter function
 * @param n A pointer to the NSMConsumer object
 * @return The status of the request
 */
CDStatus nsm_consumer_request_state(NSMConsumer *n, LifecycleSessionState state);

#ifdef __cplusplus
}
#endif

#endif /* NSM_CONSUMER_H */
