/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019 Alin Popa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * \author Alin Popa <alin.popa@bmw.de>
 * \file cdm-logging.c
 */

#include "cdm-logging.h"
#include "cdm-types.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WITH_GENIVI_DLT
#include <dlt.h>
#else
#include <syslog.h>
#endif

static void cdm_logging_handler (const gchar *log_domain,
                                 GLogLevelFlags log_level,
                                 const gchar *message,
                                 gpointer user_data);

#ifdef WITH_GENIVI_DLT
DLT_DECLARE_CONTEXT (cdm_default_ctx);

static int
priority_to_dlt (int priority)
{
  switch (priority)
    {
    case G_LOG_FLAG_RECURSION:
    case G_LOG_FLAG_FATAL:
      return DLT_LOG_FATAL;

    case G_LOG_LEVEL_ERROR:
    case G_LOG_LEVEL_CRITICAL:
      return DLT_LOG_ERROR;

    case G_LOG_LEVEL_WARNING:
      return DLT_LOG_WARN;

    case G_LOG_LEVEL_MESSAGE:
    case G_LOG_LEVEL_INFO:
      return DLT_LOG_INFO;

    case G_LOG_LEVEL_DEBUG:
    default:
      return DLT_LOG_DEBUG;
    }
}
#else
static int
priority_to_syslog (int priority)
{
  switch (priority)
    {
    case G_LOG_FLAG_RECURSION:
    case G_LOG_FLAG_FATAL:
      return LOG_CRIT;

    case G_LOG_LEVEL_ERROR:
    case G_LOG_LEVEL_CRITICAL:
      return LOG_ERR;

    case G_LOG_LEVEL_WARNING:
      return LOG_WARNING;

    case G_LOG_LEVEL_MESSAGE:
    case G_LOG_LEVEL_INFO:
      return LOG_INFO;

    case G_LOG_LEVEL_DEBUG:
    default:
      return LOG_DEBUG;
    }
}
#endif

void
cdm_logging_open (const gchar *app_name,
                  const gchar *app_desc,
                  const gchar *ctx_name,
                  const gchar *ctx_desc)
{
#ifdef WITH_GENIVI_DLT
  DLT_REGISTER_APP (app_name, app_desc);
  DLT_REGISTER_CONTEXT (cdm_default_ctx, ctx_name, ctx_desc);
#else
  CDM_UNUSED (app_name);
  CDM_UNUSED (app_desc);
  CDM_UNUSED (ctx_name);
  CDM_UNUSED (ctx_desc);
#endif
  g_log_set_default_handler (cdm_logging_handler, NULL);
}

static void
cdm_logging_handler (const gchar *log_domain,
                     GLogLevelFlags log_level,
                     const gchar *message,
                     gpointer user_data)
{
  CDM_UNUSED (log_domain);
  CDM_UNUSED (user_data);

#ifdef WITH_GENIVI_DLT
  DLT_LOG (cdm_default_ctx, priority_to_dlt (log_level), DLT_STRING (message));
#else
  syslog (priority_to_syslog (log_level), "%s", message);
#endif
}

void
cdm_logging_close (void)
{
#ifdef WITH_GENIVI_DLT
  DLT_UNREGISTER_CONTEXT (cdm_default_ctx);
  DLT_UNREGISTER_APP ();
#endif
}
