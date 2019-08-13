/* cdm-sdnotify.c
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

#include "cdm-sdnotify.h"

#include <sys/timerfd.h>
#include <systemd/sd-daemon.h>

#define USEC2SEC(x) (x / 1000000)
#define USEC2SECHALF(x) (guint)(x / 1000000 / 2)

static gboolean source_timer_callback (gpointer data);
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
