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
 * \file cdm-sdnotify.c
 */

#include "cdm-sdnotify.h"

#include <sys/timerfd.h>
#include <systemd/sd-daemon.h>

#define USEC2SEC(x) (x / 1000000)
#define USEC2SECHALF(x) (guint)(x / 1000000 / 2)

/**
 * @brief GSource callback function
 */
static gboolean source_timer_callback (gpointer data);

/**
 * @brief GSource destroy notification callback function
 */
static void source_destroy_notify (gpointer data);

static gboolean
source_timer_callback (gpointer data)
{
  CDM_UNUSED (data);

  if (sd_notify (0, "WATCHDOG=1") < 0)
    g_warning ("Fail to send the heartbeet to systemd");
  else
    g_debug ("Watchdog heartbeat sent");

  return TRUE;
}

static void
source_destroy_notify (gpointer data)
{
  CDM_UNUSED (data);
  g_info ("System watchdog disabled");
}

CdmSDNotify *
cdm_sdnotify_new (void)
{
  CdmSDNotify *sdnotify = g_new0 (CdmSDNotify, 1);
  gint sdw_status;
  gulong usec = 0;

  g_assert (sdnotify);

  g_ref_count_init (&sdnotify->rc);

  sdw_status = sd_watchdog_enabled (0, &usec);

  if (sdw_status > 0)
    {
      g_info ("Systemd watchdog enabled with timeout %lu seconds", USEC2SEC (usec));

      sdnotify->source = g_timeout_source_new_seconds (USEC2SECHALF (usec));
      g_source_ref (sdnotify->source);

      g_source_set_callback (sdnotify->source,
                             G_SOURCE_FUNC (source_timer_callback),
                             sdnotify,
                             source_destroy_notify);
      g_source_attach (sdnotify->source, NULL);
    }
  else
    {
      if (sdw_status == 0)
        g_info ("Systemd watchdog disabled");
      else
        g_warning ("Fail to get the systemd watchdog status");
    }

  return sdnotify;
}

CdmSDNotify *
cdm_sdnotify_ref (CdmSDNotify *sdnotify)
{
  g_assert (sdnotify);
  g_ref_count_inc (&sdnotify->rc);
  return sdnotify;
}

void
cdm_sdnotify_unref (CdmSDNotify *sdnotify)
{
  g_assert (sdnotify);

  if (g_ref_count_dec (&sdnotify->rc) == TRUE)
    {
      if (sdnotify->source != NULL)
        g_source_unref (sdnotify->source);

      g_free (sdnotify);
    }
}
