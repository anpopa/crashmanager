/*
 * SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2019, BMW Car IT GmbH
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * For further information see http://www.genivi.org/.
 *
 * \author Alin Popa <alin.popa@bmw.de>
 * \file cdm-lifecycle.h
 */

#pragma once

#include "cdm-types.h"
#include "nsm-consumer.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief Lifecycle event data type
 */
typedef union _LCEventData {
  LCSessionState session_state;
  LCProxyState proxy_state;
  LCRegistrationState reg_state;
} LCEventData;

/**
 * @brief Custom callback used internally by CdmLifecycle as source callback
 */
typedef gboolean (*CdmLifecycleCallback) (gpointer _lifecycle, gpointer _event);

/**
 * @brief The file transfer event
 */
typedef struct _CdmLifecycleEvent {
  LCEventType type;     /**< The event type the element holds */
  LCEventData data;     /**< The event payload data */
} CdmLifecycleEvent;

/**
 * @brief The CdmLifecycle opaque data structure
 */
typedef struct _CdmLifecycle {
  GSource source;  /**< Event loop source */
  NsmConsumer *consumer; /**< NSM consumer object */
  GAsyncQueue    *queue;  /**< Async queue */
  grefcount rc;     /**< Reference counter variable  */
} CdmLifecycle;

/*
 * @brief Create a new lifecycle object
 * @return On success return a new CdmLifecycle object otherwise return NULL
 */
CdmLifecycle *cdm_lifecycle_new (void);

/**
 * @brief Aquire lifecycle object
 * @param lifecycle Pointer to the lifecycle object
 */
CdmLifecycle *cdm_lifecycle_ref (CdmLifecycle *lifecycle);

/**
 * @brief Release lifecycle object
 * @param lifecycle Pointer to the lifecycle object
 */
void cdm_lifecycle_unref (CdmLifecycle *lifecycle);

/**
 * @brief The main interface to change the coredump lifecycle session
 *
 * Internally cdm_lifecycle module uses a reference counter for lifecycle coredump session state
 * When a caller requests a new state this affects the counter but not necessary the state.
 *
 * @param lifecycle A pointer to the lifecycle object allocated with cdm_lifecycle_new().
 *        If NULL the function will return CDM_STATUS_ERROR
 * @param state The requested new state
 *
 * @return CDM_STATUS_OK on success and CDH_STATUS_ERROR on failure
 */
CdmStatus cdm_lifecycle_set_session_state (CdmLifecycle *lifecycle, LCSessionState state)

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmLifecycle, cdm_lifecycle_unref);

G_END_DECLS
