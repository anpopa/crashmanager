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
 * @struct LifecycleData
 * @brief Lifecycle related internal data
 */
typedef struct _LifecycleData {
    std::shared_ptr<NSM::ConsumerProxyDefault> consumer_proxy; /**< Consumer proxy object */
    CommonAPI::ProxyStatusEvent::Subscription subscription;    /**< Proxy subscription object */

    LifecycleSessionState session_state;           /**< Current session state */
    LifecycleRegistrationState registration_state; /**< Current registration state */
    LifecycleProxyState proxy_state;               /**< Current proxy availability state */

    std::string session_name;                     /**< Session identifier */
    std::string session_owner;                    /**< Owner identifier */
    const NSMSeat seat = NSMSeat::NsmSeat_Driver; /**< Seat identifier */
} LifecycleData;

/**
 * @brief Initialize lifecycle data object
 *
 * @param l A pointer to the lifecycle object
 * @param session_name The consumer NSM session name
 * @param session_owner The consumer NSM session owner
 *
 * @return CDM_OK on success and CDM_ERROR on failure
 */
static CdmStatus init_lifecycle_data(LifecycleData *l, const char *session_name,
                                        const char *session_owner);

/**
 * @brief Deinitialize lifecycle data
 *
 * @param l A pointer to the lifecycle object
 *
 * @return CDM_OK on success and CDM_ERROR on failure
 */
static CdmStatus deinit_lifecycle_data(LifecycleData *l);

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

    /* The LifecycleData is a C++ private object to this module
     * so we will use new/delete operators for this object
     */
    n->private_data = new LifecycleData;
    if (n->private_data == nullptr) {
        free(n);
        return nullptr;
    }

    if (init_LifecycleData(static_cast<LifecycleData *>(n->private_data), session_name,
                            session_owner) != CDM_OK) {
        delete static_cast<LifecycleData *>(n->private_data);
        free(n);

        return nullptr;
    }

    return n;
}

void nsm_consumer_free(NSMConsumer *n)
{
    assert(n);
    assert(n->private_data);

    (void)deinit_LifecycleData(static_cast<LifecycleData *>(n->private_data));

    delete static_cast<LifecycleData *>(n->private_data);
    free(n);
}

void nsm_consumer_set_client(NSMConsumer *n, void *client)
{
    assert(n);
    n->client = client;
}

void nsm_consumer_set_proxy_cb(NSMConsumer *n,
                               void (*proxy_availability_cb)(void *client,
                                                             LifecycleProxyState state,
                                                             CdmStatus error))
{
    assert(n);
    n->proxy_availability_cb = proxy_availability_cb;
}

void nsm_consumer_set_registration_cb(
    NSMConsumer *n,
    void (*registration_state_cb)(void *client, LifecycleRegistrationState state,
                                  CdmStatus error))
{
    assert(n);
    n->registration_state_cb = registration_state_cb;
}

void nsm_consumer_set_session_cb(NSMConsumer *n,
                                 void (*session_state_cb)(void *client,
                                                          LifecycleSessionState state,
                                                          CdmStatus error))
{
    assert(n);
    n->session_state_cb = session_state_cb;
}

LifecycleProxyState nsm_consumer_get_proxy_state(NSMConsumer *n)
{
    LifecycleData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LifecycleData *>(n->private_data);

    return d->proxy_state;
}

LifecycleSessionState nsm_consumer_get_session_state(NSMConsumer *n)
{
    LifecycleData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LifecycleData *>(n->private_data);

    return d->session_state;
}

LifecycleRegistrationState nsm_consumer_get_registration_state(NSMConsumer *n)
{
    LifecycleData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LifecycleData *>(n->private_data);

    return d->registration_state;
}

void nsm_consumer_set_proxy_state(NSMConsumer *n, LifecycleProxyState state)
{
    LifecycleData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LifecycleData *>(n->private_data);

    d->proxy_state = state;
}

void nsm_consumer_set_session_state(NSMConsumer *n, LifecycleSessionState state)
{
    LifecycleData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LifecycleData *>(n->private_data);

    d->session_state = state;
}

void nsm_consumer_set_registration_state(NSMConsumer *n, LifecycleRegistrationState state)
{
    LifecycleData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LifecycleData *>(n->private_data);

    d->registration_state = state;
}

CdmStatus nsm_consumer_build_proxy(NSMConsumer *n)
{
    CdmStatus status = CDM_OK;
    LifecycleData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LifecycleData *>(n->private_data);

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    if (runtime == nullptr) {
        status = CDM_ERROR;
    } else {
        d->consumer_proxy =
            runtime->buildProxy<NSM::ConsumerProxy>("local", NSM::Consumer_NSMConsumer, "Consumer");
        if (d->consumer_proxy == nullptr) {
            status = CDM_ERROR;
        } else {
            using namespace std::placeholders;
            d->subscription = d->consumer_proxy->getProxyStatusEvent().subscribe(
                std::bind(&proxy_availability_listener, n, _1));
        }
    }

    return status;
}

CdmStatus nsm_consumer_register(NSMConsumer *n)
{
    CdmStatus status = CDM_OK;
    LifecycleData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LifecycleData *>(n->private_data);

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
        status = CDM_ERROR;
    }

    return status;
}

CdmStatus nsm_consumer_request_state(NSMConsumer *n, LifecycleSessionState state)
{
    CdmStatus status = CDM_OK;
    LifecycleData *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<LifecycleData *>(n->private_data);

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
        status = CDM_ERROR;
    }

    return status;
}

static CdmStatus init_lifecycle_data(LifecycleData *d, const char *session_name,
                                        const char *session_owner)
{
    CdmStatus status = CDM_OK;

    assert(d);

    d->registration_state = LC_UNREGISTERED;
    d->proxy_state = LC_PROXY_UNAVAILABLE;
    d->session_state = LC_SESSION_INACTIVE;
    d->session_name = session_name;
    d->session_owner = session_owner;

    return status;
}

static CdmStatus deinit_lifecycled_data(LifecycleData *d)
{
    CdmStatus status = CDM_OK;

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
                    status = CDM_ERROR;
                }
            } else {
                status = CDM_ERROR;
            }
        }

        d->consumer_proxy->getProxyStatusEvent().unsubscribe(d->subscription);
    } else {
        status = CDM_ERROR;
    }

    return status;
}

static void session_state_listener(NSMConsumer *n, const CommonAPI::CallStatus &call_status,
                                   const NSMErrorState error_status, NSMSessionState session_state)
{
    LifecycleSessionState state = LC_SESSION_INACTIVE;
    CdmStatus error = CDM_OK;

    assert(n);

    if (call_status == CommonAPI::CallStatus::SUCCESS) {
        if (error_status != NSMTypes::NsmErrorStatus_e::NsmErrorStatus_Ok) {
            error = CDM_ERROR;
        } else {
            state =
                (session_state == NSMSessionState::NsmSessionState_Active ? LC_SESSION_ACTIVE
                                                                          : LC_SESSION_INACTIVE);
        }
    } else {
        error = CDM_ERROR;
    }

    if (n->session_state_cb != NULL) {
        n->session_state_cb(n->client, state, error);
    }
}

static void register_session_listener(NSMConsumer *n, const CommonAPI::CallStatus &call_status,
                                      const NSMErrorState error_status)
{
    LifecycleRegistrationState state = LC_UNREGISTERED;
    CdmStatus error = CDM_OK;

    assert(n);

    if (call_status == CommonAPI::CallStatus::SUCCESS) {
        if (error_status != NSMTypes::NsmErrorStatus_e::NsmErrorStatus_Ok) {
            state = LC_UNREGISTERED;
        } else {
            state = LC_REGISTERED;
        }
    } else {
        error = CDM_ERROR;
    }

    if (n->registration_state_cb != NULL) {
        n->registration_state_cb(n->client, state, error);
    }
}

static void proxy_availability_listener(NSMConsumer *n, CommonAPI::AvailabilityStatus status)
{
    LifecycleProxyState state = LC_PROXY_UNAVAILABLE;

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
        n->proxy_availability_cb(n->client, state, CDM_OK);
    }
}
