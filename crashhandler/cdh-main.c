/* cdh-main.c
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

#include "cdm-defaults.h"
#include "cdm-types.h"
#include "cdm-logging.h"
#include "cdh-data.h"

#include <glib.h>
#include <stdlib.h>

gint
main (gint argc, gchar *argv[])
{
  g_autofree gchar *conf_path = NULL;
  CdmStatus status = CDM_STATUS_OK;

  cdm_logging_open ("CDH", "Coredumper instance", "CDH", "Default context");

  conf_path = g_build_filename (CDM_CONFIG_DIRECTORY, CDM_CONFIG_FILE_NAME, NULL);
  if (g_access (conf_path, R_OK) == 0)
    {
      g_autofree CdhData *data = g_malloc0 (sizeof(CdhData));

      cdh_data_init (data, conf_path);
      status = cdh_main_enter (data, argc, argv);
      cdh_data_deinit (data);
    }
  else
    {
      status = CDM_STATUS_ERROR;
    }

  cdm_logging_close ();

  return status == CDM_STATUS_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
