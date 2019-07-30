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

#include "cdm-types.h"

#include <glib.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @enum LCSessionState
 * @brief Session state enum type
 */
typedef enum _LCSessionState {
  LC_SESSION_ACTIVE,
  LC_SESSION_INACTIVE
} LCSessionState;

/**
 * @enum lifecylce_registration
 */
typedef enum _LCRegistrationState {
  LC_REGISTERED,
  LC_UNREGISTERED,
  LC_PENDING
} LCRegistrationState;

/**
 * @enum lifecylce_proxy
 */
typedef enum _LCProxyState {
  LC_PROXY_AVAILABLE,
  LC_PROXY_UNAVAILABLE,
} LCProxyState;

/**
 * @enum LCEventType
 * @brief LC event enum type
 */
typedef enum _LCEventType {
  LC_EVENT_PROXY_UPDATE,
  LC_EVENT_REGISTRATION_UPDATE,
  LC_EVENT_SESSION_UPDATE
} LCEventType;

/**
 * @struct NsmConsumer
 * @brief The nsm consumer object
 */
typedef struct _NsmConsumer {
  void *client;   /**< Reference to client object */

  void *private_data;   /**< Reference to private data object */

  void (*proxy_availability_cb)(void *client, LCProxyState state,
                                CdmStatus error);   /**< Proxy availability callback */
  void (*registration_state_cb)(void *client, LCRegistrationState state,
                                CdmStatus error);   /**< Registration state callback */
  void (*session_state_cb)(void *client, LCSessionState state,
                           CdmStatus error);   /**< Session state callback */
} NsmConsumer;

/**
 * @brief Create a new NsmConsumer object initialized for NSM communication
 *
 * @return A pointer of NsmConsumer which should be released with nsm_consumer_free()
 *         by the caller for deinitialization.
 *         If the object cannot be created (eg. initialization fail) a NULL pointer is returned
 */
NsmConsumer *nsm_consumer_new(const gchar *session_name, const gchar *session_owner);

/**
 * @brief Release a NsmConsumer object
 * @param n A pointer to the NsmConsumer object allocated with nsm_consumer_new()
 */
void nsm_consumer_free(NsmConsumer *n);

/**
 * @brief Client reference setter
 * @param n A pointer to the NsmConsumer object
 * @param client A pointer to client object
 */
void nsm_consumer_set_client(NsmConsumer *n, void *client);

/**
 * @brief Callback setter for proxy availability status
 * @param n A pointer to the NsmConsumer object
 * @param proxy_availability_cb Callback function
 */
void nsm_consumer_set_proxy_cb(NsmConsumer *n,
                               void (*proxy_availability_cb)(void *client,
                                                             LCProxyState state,
                                                             CdmStatus error));

/**
 * @brief Callback setter for NSM registration status
 * @param n A pointer to the NsmConsumer object
 * @param registration_state_cb Callback function
 */
void nsm_consumer_set_registration_cb(
  NsmConsumer *n,
  void (*registration_state_cb)(void *client, LCRegistrationState state,
                                CdmStatus error));

/**
 * @brief Callback setter for NSM session status
 * @param n A pointer to the NsmConsumer object
 * @param session_state_cb Callback function
 */
void nsm_consumer_set_session_cb(NsmConsumer *n,
                                 void (*session_state_cb)(void *client,
                                                          LCSessionState state,
                                                          CdmStatus error));

/**
 * @brief LC proxy state getter
 * @param n A pointer to the NsmConsumer object
 * @return Current proxy state set in NsmConsumer
 */
LCProxyState nsm_consumer_get_proxy_state(NsmConsumer *n);

/**
 * @brief LC session state getter
 * @param n A pointer to the NsmConsumer object
 * @return Current session state set in NsmConsumer
 */
LCSessionState nsm_consumer_get_session_state(NsmConsumer *n);

/**
 * @brief LC registration state getter
 * @param n A pointer to the NsmConsumer object
 * @return Current registration state set in NsmConsumer
 */
LCRegistrationState nsm_consumer_get_registration_state(NsmConsumer *n);

/**
 * @brief LC proxy state setter
 * @param n A pointer to the NsmConsumer object
 * @param The new proxy state to set in NsmConsumer
 */
void nsm_consumer_set_proxy_state(NsmConsumer *n, LCProxyState state);

/**
 * @brief LC session state setter
 * @param n A pointer to the NsmConsumer object
 * @param The new session state to set in NsmConsumer
 */
void nsm_consumer_set_session_state(NsmConsumer *n, LCSessionState state);

/**
 * @brief LC session state setter
 * @param n A pointer to the NsmConsumer object
 * @param The new registration state to set in NsmConsumer
 */
void nsm_consumer_set_registration_state(NsmConsumer *n, LCRegistrationState state);

/**
 * @brief Request NSM consumer to build the CommonAPI proxy.
 *        If the request is possible the proxy_availability_cb will be called.
 *        Is up to the user to update the new proxy state via the setter function
 * @param n A pointer to the NsmConsumer object
 * @return The status of the request
 */
CdmStatus nsm_consumer_build_proxy(NsmConsumer *n);

/**
 * @brief Request NSM consumer to register.
 *        If the request is possible the registration_state_cb will be called.
 *        Is up to the user to update the new registration state via the setter function
 * @param n A pointer to the NsmConsumer object
 * @return The status of the request
 */
CdmStatus nsm_consumer_register(NsmConsumer *n);

/**
 * @brief Request NSM consumer a new state.
 *        If the request is possible the session_state_cb will be called.
 *        Is up to the user to update the new session state via the setter function
 * @param n A pointer to the NsmConsumer object
 * @return The status of the request
 */
CdmStatus nsm_consumer_request_state(NsmConsumer *n, LCSessionState state);

#ifdef __cplusplus
}
#endif

#endif /* NSM_CONSUMER_H */
