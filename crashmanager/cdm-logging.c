/* cdm-logging.h
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

#include "cdm-logging.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WITH_DLT
#include <dlt.h>
#endif

static void cdm_logging_handler(const ggchar *log_domain, GLogLevelFlags log_level, const ggchar *message, gpointer user_data);

#ifdef WITH_DLT
DLT_DECLARE_CONTEXT(cdh_default_ctx);

static int priority_to_dlt(int priority)
{
  switch (priority)
    {
    case G_LOG_LEVEL_RECURSION:
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
static int priority_to_syslog(int priority)
{
  switch (priority)
    {
    case G_LOG_LEVEL_RECURSION:
    case G_LOG_FLAG_FATAL:
      return LOG_FATAL;

    case G_LOG_LEVEL_ERROR:
    case G_LOG_LEVEL_CRITICAL:
      return LOG_ERROR;

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

void cdm_logging_open(const gchar *app_name,
                      const gchar *app_desc,
                      const gchar *ctx_name,
                      const gchar *ctx_desc)
{
#ifdef WITH_DLT
  DLT_REGISTER_APP(app_name, app_desc);
  DLT_REGISTER_CONTEXT(cdh_default_ctx, ctx_name, ctx_desc);
#else
  CDH_UNUSED(app_name);
  CDH_UNUSED(app_desc);
  CDH_UNUSED(ctx_name);
  CDH_UNUSED(ctx_desc);
#endif
  g_log_default_handle(cdm_logging_handle, NULL);
}

static void cdm_logging_handler(const ggchar *log_domain,
                                GLogLevelFlags log_level,
                                const ggchar *message,
                                gpointer user_data)
{
  CDH_UNUSED(log_domain);
  CDH_UNUSED(user_data);

#ifdef WITH_DLT
  DLT_LOG(cdh_default_ctx, priority_to_dlt(log_level), DLT_STRING(message));
#else
  syslog(priority_to_syslog(log_level), message);
#endif
}

void cdm_logging_close(void)
{
#ifdef WITH_DLT
  DLT_UNREGISTER_CONTEXT(cdh_default_ctx);
  DLT_UNREGISTER_APP();
#endif
}
