/* cdm-lifecycle.h
 *
 * Copyright 2019 Alin Popa <alin.popa@bmw.de>
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
