/*
 *
 * Copyright (C) 2019, Alin Popa. All rights reserved.
 *
 * This file is part of Coredumper
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \copyright Copyright © 2019, Alin Popa
 *
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
 * @struct lifecycle_data
 * @brief Lifecycle related internal data
 */
typedef struct lifecycle_data {
    std::shared_ptr<NSM::ConsumerProxyDefault> consumer_proxy; /**< Consumer proxy object */
    CommonAPI::ProxyStatusEvent::Subscription subscription;    /**< Proxy subscription object */

    lifecycle_session_state_t session_state;           /**< Current session state */
    lifecycle_registration_state_t registration_state; /**< Current registration state */
    lifecycle_proxy_state_t proxy_state;               /**< Current proxy availability state */

    std::string session_name;                     /**< Session identifier */
    std::string session_owner;                    /**< Owner identifier */
    const NSMSeat seat = NSMSeat::NsmSeat_Driver; /**< Seat identifier */
} lifecycle_data_t;

/**
 * @brief Initialize lifecycle data object
 *
 * @param l A pointer to the lifecycle object
 * @param session_name The consumer NSM session name
 * @param session_owner The consumer NSM session owner
 *
 * @return (0) on success and (-1) on failure
 */
static int init_lifecycle_data(lifecycle_data_t *l, const char *session_name,
                               const char *session_owner);

/**
 * @brief Deinitialize lifecycle data
 *
 * @param l A pointer to the lifecycle object
 *
 * @return (0) on success and (-1) on failure
 */
static int deinit_lifecycle_data(lifecycle_data_t *l);

/**
 * @brief Async callback to be used for set session state call
 *
 * @param l A pointer to the nsm_consumer object
 * @param call_status The CommonAPI call status
 * @param error_status The NSM error status type
 * @param session_state NSM session state
 *
 * @return void
 */
static void session_state_listener(nsm_consumer_t *n, const CommonAPI::CallStatus &call_status,
                                   const NSMErrorState error_status, NSMSessionState session_state);

/**
 * @brief Async callback to be used for register session request
 *
 * @param l A pointer to the nsm_consumer object
 * @param call_status The CommonAPI call status
 * @param error_status The NSM error status type
 *
 * @return void
 */
static void register_session_listener(nsm_consumer_t *n, const CommonAPI::CallStatus &call_status,
                                      const NSMErrorState error_status);

/**
 * @brief Async callback to be used for proxy availability notification
 *
 * @param l A pointer to the nsm_consumer object
 * @param call_status The CommonAPI call status
 *
 * @return void
 */
static void proxy_availability_listener(nsm_consumer_t *n, CommonAPI::AvailabilityStatus status);

nsm_consumer_t *nsm_consumer_new(const char *session_name, const char *session_owner)
{
    nsm_consumer_t *n;

    n = (nsm_consumer_t *)calloc(1, sizeof(nsm_consumer_t));
    if (n == nullptr) {
        return nullptr;
    }

    /* The lifecycle_data_t is a C++ private object to this module
     * so we will use new/delete operators for this object
     */
    n->private_data = new lifecycle_data_t;
    if (n->private_data == nullptr) {
        free(n);
        return nullptr;
    }

    if (init_lifecycle_data(static_cast<lifecycle_data_t *>(n->private_data), session_name,
                            session_owner) != (0)) {
        delete static_cast<lifecycle_data_t *>(n->private_data);
        free(n);

        return nullptr;
    }

    return n;
}

void nsm_consumer_free(nsm_consumer_t *n)
{
    assert(n);
    assert(n->private_data);

    (void)deinit_lifecycle_data(static_cast<lifecycle_data_t *>(n->private_data));

    delete static_cast<lifecycle_data_t *>(n->private_data);
    free(n);
}

void nsm_consumer_set_client(nsm_consumer_t *n, void *client)
{
    assert(n);
    n->client = client;
}

void nsm_consumer_set_proxy_cb(nsm_consumer_t *n,
                               void (*proxy_availability_cb)(void *client,
                                                             lifecycle_proxy_state_t state,
                                                             int error))
{
    assert(n);
    n->proxy_availability_cb = proxy_availability_cb;
}

void nsm_consumer_set_registration_cb(
    nsm_consumer_t *n,
    void (*registration_state_cb)(void *client, lifecycle_registration_state_t state, int error))
{
    assert(n);
    n->registration_state_cb = registration_state_cb;
}

void nsm_consumer_set_session_cb(nsm_consumer_t *n,
                                 void (*session_state_cb)(void *client,
                                                          lifecycle_session_state_t state,
                                                          int error))
{
    assert(n);
    n->session_state_cb = session_state_cb;
}

lifecycle_proxy_state_t nsm_consumer_get_proxy_state(nsm_consumer_t *n)
{
    lifecycle_data_t *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<lifecycle_data_t *>(n->private_data);

    return d->proxy_state;
}

lifecycle_session_state_t nsm_consumer_get_session_state(nsm_consumer_t *n)
{
    lifecycle_data_t *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<lifecycle_data_t *>(n->private_data);

    return d->session_state;
}

lifecycle_registration_state_t nsm_consumer_get_registration_state(nsm_consumer_t *n)
{
    lifecycle_data_t *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<lifecycle_data_t *>(n->private_data);

    return d->registration_state;
}

void nsm_consumer_set_proxy_state(nsm_consumer_t *n, lifecycle_proxy_state_t state)
{
    lifecycle_data_t *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<lifecycle_data_t *>(n->private_data);

    d->proxy_state = state;
}

void nsm_consumer_set_session_state(nsm_consumer_t *n, lifecycle_session_state_t state)
{
    lifecycle_data_t *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<lifecycle_data_t *>(n->private_data);

    d->session_state = state;
}

void nsm_consumer_set_registration_state(nsm_consumer_t *n, lifecycle_registration_state_t state)
{
    lifecycle_data_t *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<lifecycle_data_t *>(n->private_data);

    d->registration_state = state;
}

int nsm_consumer_build_proxy(nsm_consumer_t *n)
{
    int status = (0);
    lifecycle_data_t *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<lifecycle_data_t *>(n->private_data);

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    if (runtime == nullptr) {
        status = (-1);
    } else {
        d->consumer_proxy =
            runtime->buildProxy<NSM::ConsumerProxy>("local", NSM::Consumer_NSMConsumer, "Consumer");
        if (d->consumer_proxy == nullptr) {
            status = (-1);
        } else {
            using namespace std::placeholders;
            d->subscription = d->consumer_proxy->getProxyStatusEvent().subscribe(
                std::bind(&proxy_availability_listener, n, _1));
        }
    }

    return status;
}

int nsm_consumer_register(nsm_consumer_t *n)
{
    int status = (0);
    lifecycle_data_t *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<lifecycle_data_t *>(n->private_data);

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
        status = (-1);
    }

    return status;
}

int nsm_consumer_request_state(nsm_consumer_t *n, lifecycle_session_state state)
{
    int status = (0);
    lifecycle_data_t *d = nullptr;

    assert(n);
    assert(n->private_data);

    d = static_cast<lifecycle_data_t *>(n->private_data);

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
        status = (-1);
    }

    return status;
}

static int init_lifecycle_data(lifecycle_data_t *d, const char *session_name,
                               const char *session_owner)
{
    int status = (0);

    assert(d);

    d->registration_state = LC_UNREGISTERED;
    d->proxy_state = LC_PROXY_UNAVAILABLE;
    d->session_state = LC_SESSION_INACTIVE;
    d->session_name = session_name;
    d->session_owner = session_owner;

    return status;
}

static int deinit_lifecycle_data(lifecycle_data_t *d)
{
    int status = (0);

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
                    status = (-1);
                }
            } else {
                status = (-1);
            }
        }

        d->consumer_proxy->getProxyStatusEvent().unsubscribe(d->subscription);
    } else {
        status = (-1);
    }

    return status;
}

static void session_state_listener(nsm_consumer_t *n, const CommonAPI::CallStatus &call_status,
                                   const NSMErrorState error_status, NSMSessionState session_state)
{
    lifecycle_session_state_t state = LC_SESSION_INACTIVE;
    int error = (0);

    assert(n);

    if (call_status == CommonAPI::CallStatus::SUCCESS) {
        if (error_status != NSMTypes::NsmErrorStatus_e::NsmErrorStatus_Ok) {
            error = (-1);
        } else {
            state =
                (session_state == NSMSessionState::NsmSessionState_Active ? LC_SESSION_ACTIVE
                                                                          : LC_SESSION_INACTIVE);
        }
    } else {
        error = (-1);
    }

    if (n->session_state_cb != NULL) {
        n->session_state_cb(n->client, state, error);
    }
}

static void register_session_listener(nsm_consumer_t *n, const CommonAPI::CallStatus &call_status,
                                      const NSMErrorState error_status)
{
    lifecycle_registration_state_t state = LC_UNREGISTERED;
    int error = (0);

    assert(n);

    if (call_status == CommonAPI::CallStatus::SUCCESS) {
        if (error_status != NSMTypes::NsmErrorStatus_e::NsmErrorStatus_Ok) {
            state = LC_UNREGISTERED;
        } else {
            state = LC_REGISTERED;
        }
    } else {
        error = (-1);
    }

    if (n->registration_state_cb != NULL) {
        n->registration_state_cb(n->client, state, error);
    }
}

static void proxy_availability_listener(nsm_consumer_t *n, CommonAPI::AvailabilityStatus status)
{
    lifecycle_proxy_state_t state = LC_PROXY_UNAVAILABLE;

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
        n->proxy_availability_cb(n->client, state, (0));
    }
}
