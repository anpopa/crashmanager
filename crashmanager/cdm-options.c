/* cdm-options.c
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

#include "cdm-options.h"
#include "cdm-defaults.h"

#include <glib.h>
#include <stdlib.h>

CdmOptions *cdm_options_new(const gchar *conf_path)
{
  CdmOptions *opts = calloc(1, sizeof(CdmOptions));

  opts->has_conf = false;

  if (conf_path != NULL)
    {
      GError *error = NULL;

      opts->conf = g_key_file_new();

      g_assert(opts->conf);

      if (g_key_file_load_from_file(opts->conf, conf_path, G_KEY_FILE_NONE, *error) == TRUE)
        {
          opts->has_conf = true;
        }
      else
        {
          g_debug("Cannot parse configuration file");
          g_error_free(error);
        }
    }

  return opts;
}

void cdm_options_free(CdmOptions *opts)
{
  g_assert(opts);

  if (opts->conf)
    {
      cdm_conf_free(opts->conf);
    }

  free(opts);
}

const gchar *cdm_options_string_for(CdmOptions *opts, CdmOptionsKey key, CdmStatus *error)
{
  if (error != NULL)
    *error = CDM_STATUS_OK;

  switch (key)
    {
    case KEY_USER_NAME:
      if (opts->has_conf)
        {
          const gchar *tmp = g_key_file_get_string(opts->conf, "common", "UserName", NULL);

          if (tmp != NULL)
            {
              return tmp;
            }
        }
      return CDM_USER_NAME;

    case KEY_GROUP_NAME:
      if (opts->has_conf)
        {
          const gchar *tmp = g_key_file_get_string(opts->conf, "common", "GroupName", NULL);

          if (tmp != NULL)
            {
              return tmp;
            }
        }
      return CDM_GROUP_NAME;

    case KEY_CRASHDUMP_DIR:
      if (opts->has_conf)
        {
          const gchar *tmp =
            g_key_file_get_string(opts->conf, "common", "CoredumpDirectory", NULL);

          if (tmp != NULL)
            {
              return tmp;
            }
        }
      return CDM_CRASHDUMP_DIR;

    case KEY_RUN_DIR:
      if (opts->has_conf)
        {
          const gchar *tmp = g_key_file_get_string(opts->conf, "common", "RunDirectory", NULL);

          if (tmp != NULL)
            {
              return tmp;
            }
        }
      return CDM_RUN_DIR;

    case KEY_KDUMPSOURCE_DIR:
      if (opts->has_conf)
        {
          const gchar *tmp =
            g_key_file_get_string(opts->conf, "coremanager", "KDumpSourceDir", NULL);

          if (tmp != NULL)
            {
              return tmp;
            }
        }
      return CDM_KDUMPSOURCE_DIR;

    case KEY_IPC_SOCK_ADDR:
      if (opts->has_conf)
        {
          const gchar *tmp =
            g_key_file_get_string(opts->conf, "common", "IpcSocketFile", NULL);

          if (tmp != NULL)
            {
              return tmp;
            }
        }
      return CDM_IPC_SOCK_ADDR;

    default:
      break;
    }

  if (error != NULL)
    {
      *error = CDM_STATUS_ERROR;
    }

  return NULL;
}

static CdmStatus get_long_option(CdmOptions *opts,
                                 const gchar *section_name,
                                 const gchar *property_name,
                                 gint64 *value)
{
  g_assert(opts);
  g_assert(section_name);
  g_assert(property_name);
  g_assert(value);

  if (opts->has_conf)
    {
      const gchar *tmp = g_key_file_get_string(opts->conf, section_name, -1, property_name);

      if (tmp != NULL)
        {
          gchar *c = NULL;
          gint64 ret;

          ret = strtol(tmp, &c, 10);

          if (*c != tmp[0])
            {
              *value = ret;
              return CDM_STATUS_OK;
            }
        }
    }

  return CDM_STATUS_ERROR;
}

gint64 cdm_options_long_for(CdmOptions *opts, CdmOptionsKey key, CdmStatus *error)
{
  gint64 value = -1;

  if (error != NULL)
    {
      *error = CDM_STATUS_OK;
    }

  switch (key)
    {
    case KEY_ELEVATED_NICE_VALUE:
      return get_long_option(opts, "coredumper", "ElevatedNiceValue", &value) == CDM_STATUS_OK ?
             value :
             CDM_ELEVATED_NICE_VALUE;

    case KEY_IPC_TIMEOUT_SEC:
      return get_long_option(opts, "common", "IpcSocketTimeout", &value) == CDM_STATUS_OK ?
             value :
             CDM_CRASHMANAGER_IPC_TIMEOUT_SEC;

    case KEY_CRASHDIR_MIN_SIZE:
      return get_long_option(opts, "crashmanager", "MinCoredumpDirSize", &value) == CDM_STATUS_OK ?
             value :
             CDM_CRASHDIR_MIN_SIZE;

    case KEY_CRASHDIR_MAX_SIZE:
      return get_long_option(opts, "crashmanager", "MaxCoredumpDirSize", &value) == CDM_STATUS_OK ?
             value :
             CDM_CRASHDIR_MAX_SIZE;

    case KEY_CRASHDIR_MAX_COUNT:
      return get_long_option(opts, "crashmanager", "MaxCoredumpArchives", &value) == CDM_STATUS_OK ?
             value :
             CDM_CRASHDIR_MAX_COUNT;

    default:
      break;
    }

  if (error != NULL)
    {
      *error = CDM_STATUS_ERROR;
    }

  return -1;
}
