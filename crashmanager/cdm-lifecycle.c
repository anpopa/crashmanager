/* cdm-lifecycle.c
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

#include "cdm-lifecycle.h"

/**
 * @brief On proxy status update event
 *
 * @param l A pointer to the lifecycle object
 * @param proxy_state The new proxy status
 *
 * @return void
 */
static void on_proxy_state_update (CdmLifecycle *lifecycle, LCProxyState proxy_state);

/**
 * @brief On registration status update event
 *
 * @param l A pointer to the lifecycle object
 * @param reg_state The new registration status
 *
 * @return void
 */
static void on_registration_state_update (CdmLifecycle *lifecycle, LCRegistrationState reg_state);

/**
 * @brief On session status update event
 *
 * @param l A pointer to the lifecycle object
 * @param session_state The new session status
 *
 * @return void
 */
static void on_session_state_update (CdmLifecycle *lifecycle, LCSessionState session_state);

/**
 * @brief Post new event
 *
 * The states in this module are set directly in the async callbacks (named *_listener) and
 * all other time consuming activity should be done by posting a new event
 *
 * @param l A pointer to the lifecycle object
 * @param type The type of the new event to be posted
 * @param data The payload data for the new event
 *
 * @return CDM_STATUS_OK on success and CDM_STATUS_ERROR on failure
 */
static void post_lifecycle_event (CdmLifecycle *lifecycle, LCEventType type, LCEventData data);

/**
 * @brief NSM consumer proxy availability async callback
 *
 * @param state The new proxy state
 * @param error The validity of the new proxy state
 *
 * @return void
 */
static void proxy_availability_async_cb (void *client, LCProxyState state, CdmStatus error);

/**
 * @brief NSM consumer registration state async callback
 *
 * @param state The new registration state
 * @param error The validity of the new registration state
 *
 * @return void
 */
static void registration_state_async_cb (void *client, LCRegistrationState state, CdmStatus error);

/**
 * @brief NSM consumer session state async callback
 *
 * @param state The new session state
 * @param error The validity of the new session state
 *
 * @return void
 */
static void session_state_async_cb (void *client, LCSessionState state, CdmStatus error);

/**
 * @brief GSource prepare function
 */
static gboolean lifecycle_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource dispatch function
 */
static gboolean lifecycle_source_dispatch (GSource *source, GSourceFunc callback, gpointer _lifecycle);

/**
 * @brief GSource callback function
 */
static gboolean lifecycle_source_callback (gpointer _lifecycle, gpointer _event);

/**
 * @brief GSource destroy notification callback function
 */
static void lifecycle_source_destroy_notify (gpointer _lifecycle);

/**
 * @brief Async queue destroy notification callback function
 */
static void lifecycle_queue_destroy_notify (gpointer _lifecycle);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs lifecycle_source_funcs =
{
  lifecycle_source_prepare,
  NULL,
  lifecycle_source_dispatch,
  NULL,
  NULL,
  NULL,
};

static gboolean
lifecycle_source_prepare (GSource *source,
                          gint *timeout)
{
  CdmLifecycle *lifecycle = (CdmLifecycle *)source;

  CDM_UNUSED (timeout);

  return(g_async_queue_length (lifecycle->queue) > 0);
}

static gboolean
lifecycle_source_dispatch (GSource *source,
                           GSourceFunc callback,
                           gpointer _lifecycle)
{
  CdmLifecycle *lifecycle = (CdmLifecycle *)source;
  gpointer event = g_async_queue_try_pop (lifecycle->queue);

  CDM_UNUSED (callback);
  CDM_UNUSED (_lifecycle);

  if (event == NULL)
    return G_SOURCE_CONTINUE;

  return lifecycle->callback (lifecycle, event) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
lifecycle_source_callback (gpointer _lifecycle,
                           gpointer _event)
{
  CdmLifecycle *lifecycle = (CdmLifecycle *)_lifecycle;
  CdmLifecycleEvent *event = (CdmLifecycleEvent *)_event;

  g_assert (lifecycle);
  g_assert (event);

  switch (event->type)
    {
    case LC_EVENT_PROXY_UPDATE:
      on_proxy_state_update (lifecycle, event->data.proxy_state);
      break;

    case LC_EVENT_REGISTRATION_UPDATE:
      on_registration_state_update (lifecycle, event->data.reg_state);
      break;

    case LC_EVENT_SESSION_UPDATE:
      on_session_state_update (lifecycle, event->data.session_state);
      break;

    default:
      g_error ("Unknown event type in the list");
      break;
    }

  g_free (event);

  return TRUE;
}

static void
lifecycle_source_destroy_notify (gpointer _lifecycle)
{
  CdmLifecycle *lifecycle = (CdmLifecycle *)_lifecycle;

  g_assert (lifecycle);
  g_debug ("Lifecycle destroy notification");

  cdm_lifecycle_unref (lifecycle);
}

static void
lifecycle_queue_destroy_notify (gpointer _lifecycle)
{
  CDM_UNUSED (_lifecycle);
  g_debug ("Transfer queue destroy notification");
}

CdmLifecycle *
cdm_lifecycle_new (void)
{
  CdmLifecycle *lifecycle = (CdmLifecycle *)g_source_new (&lifecycle_source_funcs, sizeof(CdmLifecycle));

  g_assert (lifecycle);

  g_ref_count_init (&lifecycle->rc);

  lifecycle->queue = g_async_queue_new_full (lifecycle_queue_destroy_notify);

  g_source_set_callback (CDM_EVENT_SOURCE (lifecycle),
                         NULL,
                         lifecycle,
                         lifecycle_source_destroy_notify);
  g_source_attach (CDM_EVENT_SOURCE (lifecycle), NULL);

  lifecycle->consumer = nsm_consumer_new ("CoredumpSession", "CrashManager");

  /* Prepare nsm consumer callback data */
  nsm_consumer_set_client (lifecycle->consumer, (void *)lifecycle);
  nsm_consumer_set_proxy_cb (lifecycle->consumer, proxy_availability_async_cb);
  nsm_consumer_set_registration_cb (lifecycle->consumer, registration_state_async_cb);
  nsm_consumer_set_session_cb (lifecycle->consumer, session_state_async_cb);

  if (nsm_consumer_build_proxy (lifecycle->consumer) != CDM_STATUS_OK)
    g_warning ("Fail to build lifecycle consumer proxy");

  return lifecycle;
}

CdmLifecycle *
cdm_lifecycle_ref (CdmLifecycle *lifecycle)
{
  g_assert (lifecycle);
  g_ref_count_inc (&lifecycle->rc);
  return lifecycle;
}

void
cdm_lifecycle_unref (CdmLifecycle *lifecycle)
{
  g_assert (lifecycle);

  if (g_ref_count_dec (&lifecycle->rc) == TRUE)
    {
      nsm_consumer_free (lifecycle->consumer);
      g_async_queue_unref (lifecycle->queue);
      g_source_unref (CDM_EVENT_SOURCE (lifecycle));
    }
}

CdmStatus
cdm_lifecycle_set_session_state (CdmLifecycle *lifecycle,
                                 LCSessionState state)
{
  CdmStatus status = CDM_STATUS_OK;

  /* If the lifecycle object is not available return an error */
  if (lifecycle == NULL)
    return CDM_STATUS_ERROR;

  g_assert (lifecycle->consumer);

  /* Abort if session counter state is invalid */
  if ((state == LC_SESSION_INACTIVE) && (lifecycle->scount == 0))
    g_error ("Lifecycle session counter invalid state");

  /* Update the active session counter */
  (state == LC_SESSION_ACTIVE) ? lifecycle->scount++ : lifecycle->scount--;

  /* The proxy should be available and registration complete */
  if ((nsm_consumer_get_proxy_state (lifecycle->consumer) == LC_PROXY_AVAILABLE) &&
      (nsm_consumer_get_registration_state (lifecycle->consumer) == LC_REGISTERED))
    {
      /*
       * We only request a session state change if there is a change in the state
       * and the requested state is not pending and no other cdhinst is holding the state
       */
      if ((nsm_consumer_get_session_state (lifecycle->consumer) != state) &&
          (((state == LC_SESSION_ACTIVE) && (lifecycle->scount > 0)) ||
           ((state == LC_SESSION_INACTIVE) && (lifecycle->scount == 0))))
        {
          g_info ("Set lifecycle coredump session to %s",
                  state == LC_SESSION_ACTIVE ? "active" : "inactive");

          if (nsm_consumer_request_state (lifecycle->consumer, state) != CDM_STATUS_OK)
            {
              g_warning ("Fail to request nsm consumer state");
              status = CDM_STATUS_ERROR;
            }
        }
    }
  else
    {
      g_warning ("NSM not available while setting the session state");
      status = CDM_STATUS_ERROR;
    }

  return status;
}

static void
on_proxy_state_update (CdmLifecycle *lifecycle,
                       LCProxyState proxy_state)
{
  g_assert (lifecycle);
  g_assert (lifecycle->consumer);

  nsm_consumer_set_proxy_state (lifecycle->consumer, proxy_state);

  if (nsm_consumer_get_proxy_state (lifecycle->consumer) == LC_PROXY_AVAILABLE)
    g_info ("Lifecycle proxy available");
  else
    g_info ("Lifecycle proxy not available");

  if ((nsm_consumer_get_registration_state (lifecycle->consumer) == LC_UNREGISTERED) &&
      (nsm_consumer_get_proxy_state (lifecycle->consumer) == LC_PROXY_AVAILABLE))
    {
      g_info ("Registering the Coremanager for coredump session");
      if (nsm_consumer_register (lifecycle->consumer) != CDM_STATUS_OK)
        g_warning ("Fail to request nsm consumer registration");
    }
  else if ((nsm_consumer_get_registration_state (lifecycle->consumer) == LC_REGISTERED) &&
           (nsm_consumer_get_proxy_state (lifecycle->consumer) == LC_PROXY_UNAVAILABLE))
    {
      g_warning ("Proxy lost after registration");

      /*
       * We reset the registration state to request a new registration
       * if the proxy will become available
       */
      nsm_consumer_set_registration_state (lifecycle->consumer, LC_UNREGISTERED);
    }
  else
    {
      g_debug ("Lifecycle proxy status on event is %s",
               (nsm_consumer_get_proxy_state (lifecycle->consumer) == LC_PROXY_AVAILABLE ? "available"
                : "not available"));
    }
}

static void
on_registration_state_update (CdmLifecycle *lifecycle,
                              LCRegistrationState reg_state)
{
  g_assert (lifecycle);
  g_assert (lifecycle->consumer);

  nsm_consumer_set_registration_state (lifecycle->consumer, reg_state);

  if (reg_state == LC_REGISTERED)
    g_info ("Coredump session registered");
  else
    g_info ("Coredump session not registered");
}

static void
on_session_state_update (CdmLifecycle *lifecycle,
                         LCSessionState session_state)
{
  g_assert (lifecycle);
  g_assert (lifecycle->consumer);

  nsm_consumer_set_session_state (lifecycle->consumer, session_state);

  if (session_state == LC_SESSION_ACTIVE)
    g_info ("Succesfully set lifecycle session to active state");
  else
    g_info ("Succesfully set lifecycle session to inactive state");
}

static void
post_lifecycle_event (CdmLifecycle *lifecycle, LCEventType type,
                      LCEventData data)
{
  CdmLifecycleEvent *e = NULL;

  g_assert (lifecycle);

  e = g_new0 (CdmLifecycleEvent, 1);

  e->type = type;
  e->data = data;

  g_async_queue_push (lifecycle->queue, e);
}

static void
proxy_availability_async_cb (void *client,
                             LCProxyState state,
                             CdmStatus error)
{
  if ((error == CDM_STATUS_OK) && (client != NULL))
    {
      LCEventData ev_data;

      ev_data.proxy_state = state;
      post_lifecycle_event ((CdmLifecycle *)client, LC_EVENT_PROXY_UPDATE, ev_data);
    }
  else
    {
      g_warning ("Proxy async callback provides invalid new state");
    }
}

static void
registration_state_async_cb (void *client,
                             LCRegistrationState state,
                             CdmStatus error)
{
  if ((error == CDM_STATUS_OK) && (client != NULL))
    {
      LCEventData ev_data;

      ev_data.reg_state = state;
      post_lifecycle_event ((CdmLifecycle *)client, LC_EVENT_REGISTRATION_UPDATE, ev_data);
    }
  else
    {
      g_warning ("Registration async callback provides invalid new state");
    }
}

static void
session_state_async_cb (void *client,
                        LCSessionState state,
                        CdmStatus error)
{
  if ((error == CDM_STATUS_OK) && (client != NULL))
    {
      LCEventData ev_data;

      ev_data.session_state = state;
      post_lifecycle_event ((CdmLifecycle *)client, LC_EVENT_SESSION_UPDATE, ev_data);
    }
  else
    {
      g_warning ("Session async callback provides invalid new state");
    }
}
