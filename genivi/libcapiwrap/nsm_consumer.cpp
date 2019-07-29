/* nsm_consumer.cpp
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

#include "nsm_consumer.h"

#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <functional>
#include <string>

#include <CommonAPI/CommonAPI.hpp>
#include <v1/org/genivi/NodeStateManagerTypes.hpp>
#include <v1/org/genivi/nodestatemanager/ConsumerInstanceIds.hpp>
#include <v1/org/genivi/nodestatemanager/ConsumerProxy.hpp>

using NSMTypes = v1::org::genivi::NodeStateManagerTypes;
using NSMSeat = NSMTypes::NsmSeat_e;
using NSMSessionState = NSMTypes::NsmSessionState_e;
using NSMErrorState = NSMTypes::NsmErrorStatus_e;

namespace NSM = v1::org::genivi::nodestatemanager;

/**
 * @struct LCData
 * @brief LC related internal data
 */
typedef struct _LCData {
    std::shared_ptr<NSM::ConsumerProxyDefault> consumer_proxy; /**< Consumer proxy object */
    CommonAPI::ProxyStatusEvent::Subscription subscription;    /**< Proxy subscription object */

    LCSessionState session_state;           /**< Current session state */
    LCRegistrationState registration_state; /**< Current registration state */
    LCProxyState proxy_state;               /**< Current proxy availability state */

    std::string session_name;                     /**< Session identifier */
    std::string session_owner;                    /**< Owner identifier */
    const NSMSeat seat = NSMSeat::NsmSeat_Driver; /**< Seat identifier */
} LCData;

/**
 * @brief Initialize lifecycle data object
 *
 * @param l A pointer to the lifecycle object
 * @param session_name The consumer NSM session name
 * @param session_owner The consumer NSM session owner
 *
 * @return CD_STATUS_OK on success and CD_STATUS_ERROR on failure
 */
static CDStatus init_lifecycle_data(LCData *l, const char *session_name,
                                        const char *session_owner);

/**
 * @brief Deinitialize lifecycle data
 *
 * @param l A pointer to the lifecycle object
 *
 * @return CD_STATUS_OK on success and CD_STATUS_ERROR on failure
 */
static CDStatus deinit_lifecycle_data(LCData *l);

/**
 * @brief Async callback to be used for set session state call
 *
 * @param l A pointer to the NSMConsumer object
 * @param call_status The CommonAPI call status
 * @param error_status The NSM error status type
 * @param session_state NSM session state
 *
 * @return void
 */
static void session_state_listener(NSMConsumer *n, const CommonAPI::CallStatus &call_status,
                                   const NSMErrorState error_status, NSMSessionState session_state);

/**
 * @brief Async callback to be used for register session request
 *
 * @param l A pointer to the NSMConsumer object
 * @param call_status The CommonAPI call status
 * @param error_status The NSM error status type
 *
 * @return void
 */
static void register_session_listener(NSMConsumer *n, const CommonAPI::CallStatus &call_status,
                                      const NSMErrorState error_status);

/**
 * @brief Async callback to be used for proxy availability notification
 *
 * @param l A pointer to the NSMConsumer object
 * @param call_status The CommonAPI call status
 *
 * @return void
 */
static void proxy_availability_listener(NSMConsumer *n, CommonAPI::AvailabilityStatus status);

NSMConsumer *nsm_consumer_new(const char *session_name, const char *session_owner)
{
    NSMConsumer *n;

    n = (NSMConsumer *)calloc(1, sizeof(NSMConsumer));
    if (n == nullptr) {
        return nullptr;
    }

    /* The LCData is a C++ private object to this module
     * so we will use new/delete operators for this object
     */
    n->private_data = new LCData;
    if (n->private_data == nullptr) {
        free(n);
        return nullptr;
    }

    if (init_LCData(static_cast<LCData *>(n->private_data), session_name,
                            session_owner) != CD_STATUS_OK) {
        delete static_cast<LCData *>(n->private_data);
        free(n);

        return nullptr;
    }

    return n;
}

void nsm_consumer_free(NSMConsumer *n)
{
    assert(n);
    assert(n->private_data);

    (void)deinit_LCData(static_cast<LCData *>(n->private_data));

    delete static_cast<LCData *>(n->private_data);
    free(n);
}

void nsm_consumer_set_client(NSMConsumer *n, void *client)
{
    assert(n);
    n->client = client;
}

void nsm_consumer_set_proxy_cb(NSMConsumer *n,
                               void (*proxy_availability_cb)(void *client,
                                                             LCProxyState state,
                                                             CDStatus error))
{
    assert(n);
    n->proxy_availability_cb = proxy_availability_cb;
}

void nsm_consumer_set_registration_cb(
    NSMConsumer *n,
    void (*registration_state_cb)(void *client, LCRegistrationState state,
                                  CDStatus error))
{
    assert(n);
    n->registration_state_cb = registration_state_cb;
}

void nsm_consumer_set_session_cb(NSMConsumer *n,
                                 void (*session_state_cb)(void *client,
                                                          LCSessionState state,
                                                          CDStatus error))
{
    assert(n);
    n->session_state_cb = session_state_cb;
}

LCProxyState nsm_consumer_get_proxy_state(NSMConsumer *n)
{
    LCData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LCData *>(n->private_data);

    return d->proxy_state;
}

LCSessionState nsm_consumer_get_session_state(NSMConsumer *n)
{
    LCData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LCData *>(n->private_data);

    return d->session_state;
}

LCRegistrationState nsm_consumer_get_registration_state(NSMConsumer *n)
{
    LCData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LCData *>(n->private_data);

    return d->registration_state;
}

void nsm_consumer_set_proxy_state(NSMConsumer *n, LCProxyState state)
{
    LCData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LCData *>(n->private_data);

    d->proxy_state = state;
}

void nsm_consumer_set_session_state(NSMConsumer *n, LCSessionState state)
{
    LCData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LCData *>(n->private_data);

    d->session_state = state;
}

void nsm_consumer_set_registration_state(NSMConsumer *n, LCRegistrationState state)
{
    LCData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LCData *>(n->private_data);

    d->registration_state = state;
}

CDStatus nsm_consumer_build_proxy(NSMConsumer *n)
{
    CDStatus status = CD_STATUS_OK;
    LCData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LCData *>(n->private_data);

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    if (runtime == nullptr) {
        status = CD_STATUS_ERROR;
    } else {
        d->consumer_proxy =
            runtime->buildProxy<NSM::ConsumerProxy>("local", NSM::Consumer_NSMConsumer, "Consumer");
        if (d->consumer_proxy == nullptr) {
            status = CD_STATUS_ERROR;
        } else {
            using namespace std::placeholders;
            d->subscription = d->consumer_proxy->getProxyStatusEvent().subscribe(
                std::bind(&proxy_availability_listener, n, _1));
        }
    }

    return status;
}

CDStatus nsm_consumer_register(NSMConsumer *n)
{
    CDStatus status = CD_STATUS_OK;
    LCData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LCData *>(n->private_data);

    if ((d->registration_state == LC_UNREGISTERED) && (d->proxy_state == LC_PROXY_AVAILABLE)) {
        CommonAPI::CallInfo info;
        using namespace std::placeholders;

        d->registration_state = LC_PENDING;
        d->consumer_proxy->RegisterSessionAsync(
            d->session_name, d->session_owner, d->seat,
            (d->session_state == LC_SESSION_ACTIVE ? NSMSessionState::NsmSessionState_Active
                                                   : NSMSessionState::NsmSessionState_Inactive),
            std::bind(&register_session_listener, n, _1, _2), &info);
    } else {
        status = CD_STATUS_ERROR;
    }

    return status;
}

CDStatus nsm_consumer_request_state(NSMConsumer *n, LCSessionState state)
{
    CDStatus status = CD_STATUS_OK;
    LCData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LCData *>(n->private_data);

    /* The proxy should be available and registration complete */
    if ((d->proxy_state == LC_PROXY_AVAILABLE) && (d->registration_state == LC_REGISTERED)) {
        /*
         * We only request a session state change if there is a change in the state
         * and the requested state is not pending and no other cdhinst is holding the state
         */
        if (d->session_state != state) {
            CommonAPI::CallInfo info;
            using namespace std::placeholders;

            d->consumer_proxy->SetSessionStateAsync(
                d->session_name, d->session_owner, d->seat,
                (state == LC_SESSION_ACTIVE ? NSMSessionState::NsmSessionState_Active
                                            : NSMSessionState::NsmSessionState_Inactive),
                std::bind(&session_state_listener, n, _1, _2,
                          (state == LC_SESSION_ACTIVE ? NSMSessionState::NsmSessionState_Active
                                                      : NSMSessionState::NsmSessionState_Inactive)),
                &info);
        }
    } else {
        status = CD_STATUS_ERROR;
    }

    return status;
}

static CDStatus init_lifecycle_data(LCData *d, const char *session_name,
                                        const char *session_owner)
{
    CDStatus status = CD_STATUS_OK;

    assert(d);

    d->registration_state = LC_UNREGISTERED;
    d->proxy_state = LC_PROXY_UNAVAILABLE;
    d->session_state = LC_SESSION_INACTIVE;
    d->session_name = session_name;
    d->session_owner = session_owner;

    return status;
}

static CDStatus deinit_lifecycled_data(LCData *d)
{
    CDStatus status = CD_STATUS_OK;

    assert(d);

    if (d->consumer_proxy != nullptr) {
        if ((d->proxy_state == LC_PROXY_AVAILABLE) && (d->registration_state == LC_REGISTERED)) {
            NSMErrorState error_status;
            CommonAPI::CallStatus call_status;

            using namespace std::placeholders;

            /* To avoid use after deinitialization we call unregister session synchronous */
            d->consumer_proxy->UnRegisterSession(d->session_name, d->session_owner, d->seat,
                                                 call_status, error_status);

            if (call_status == CommonAPI::CallStatus::SUCCESS) {
                if (error_status != NSMTypes::NsmErrorStatus_e::NsmErrorStatus_Ok) {
                    status = CD_STATUS_ERROR;
                }
            } else {
                status = CD_STATUS_ERROR;
            }
        }

        d->consumer_proxy->getProxyStatusEvent().unsubscribe(d->subscription);
    } else {
        status = CD_STATUS_ERROR;
    }

    return status;
}

static void session_state_listener(NSMConsumer *n, const CommonAPI::CallStatus &call_status,
                                   const NSMErrorState error_status, NSMSessionState session_state)
{
    LCSessionState state = LC_SESSION_INACTIVE;
    CDStatus error = CD_STATUS_OK;

    assert(n);

    if (call_status == CommonAPI::CallStatus::SUCCESS) {
        if (error_status != NSMTypes::NsmErrorStatus_e::NsmErrorStatus_Ok) {
            error = CD_STATUS_ERROR;
        } else {
            state =
                (session_state == NSMSessionState::NsmSessionState_Active ? LC_SESSION_ACTIVE
                                                                          : LC_SESSION_INACTIVE);
        }
    } else {
        error = CD_STATUS_ERROR;
    }

    if (n->session_state_cb != NULL) {
        n->session_state_cb(n->client, state, error);
    }
}

static void register_session_listener(NSMConsumer *n, const CommonAPI::CallStatus &call_status,
                                      const NSMErrorState error_status)
{
    LCRegistrationState state = LC_UNREGISTERED;
    CDStatus error = CD_STATUS_OK;

    assert(n);

    if (call_status == CommonAPI::CallStatus::SUCCESS) {
        if (error_status != NSMTypes::NsmErrorStatus_e::NsmErrorStatus_Ok) {
            state = LC_UNREGISTERED;
        } else {
            state = LC_REGISTERED;
        }
    } else {
        error = CD_STATUS_ERROR;
    }

    if (n->registration_state_cb != NULL) {
        n->registration_state_cb(n->client, state, error);
    }
}

static void proxy_availability_listener(NSMConsumer *n, CommonAPI::AvailabilityStatus status)
{
    LCProxyState state = LC_PROXY_UNAVAILABLE;

    assert(n);

    switch (status) {
    case CommonAPI::AvailabilityStatus::AVAILABLE:
        state = LC_PROXY_AVAILABLE;
        break;
    case CommonAPI::AvailabilityStatus::NOT_AVAILABLE:
        /* fallthrough */
    default:
        state = LC_PROXY_UNAVAILABLE;
        break;
    }

    if (n->proxy_availability_cb != NULL) {
        n->proxy_availability_cb(n->client, state, CD_STATUS_OK);
    }
}
