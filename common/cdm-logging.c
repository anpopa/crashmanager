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
