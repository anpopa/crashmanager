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

static gint64 get_long_option (CdmOptions *opts,
                               const gchar *section_name,
                               const gchar *property_name,
                               GError **error);

CdmOptions *
cdm_options_new (const gchar *conf_path)
{
  CdmOptions *opts = g_new0 (CdmOptions, 1);

  opts->has_conf = false;

  if (conf_path != NULL)
    {
      g_autoptr (GError) error = NULL;

      opts->conf = g_key_file_new ();

      g_assert (opts->conf);

      if (g_key_file_load_from_file (opts->conf, conf_path, G_KEY_FILE_NONE, &error) == TRUE)
        {
          opts->has_conf = true;
        }
      else
        {
          g_debug ("Cannot parse configuration file");
        }
    }

  g_ref_count_init (&opts->rc);

  return opts;
}

CdmOptions *
cdm_options_ref (CdmOptions *opts)
{
  g_assert (opts);
  g_ref_count_inc (&opts->rc);
  return opts;
}

void
cdm_options_unref (CdmOptions *opts)
{
  g_assert (opts);

  if (g_ref_count_dec (&opts->rc) == TRUE)
    {
      if (opts->conf)
        g_key_file_unref (opts->conf);

      g_free (opts);
    }
}

GKeyFile *
cdm_options_get_key_file (CdmOptions *opts)
{
  g_assert (opts);
  return opts->conf;
}

gchar *
cdm_options_string_for (CdmOptions *opts,
                        CdmOptionsKey key)
{
  switch (key)
    {
    case KEY_USER_NAME:
      if (opts->has_conf)
        {
          char *tmp = g_key_file_get_string (opts->conf, "common", "UserName", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_USER_NAME);

    case KEY_GROUP_NAME:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "common", "GroupName", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_GROUP_NAME);

    case KEY_CRASHDUMP_DIR:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "common", "CrashdumpDirectory", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_CRASHDUMP_DIR);

    case KEY_RUN_DIR:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "common", "RunDirectory", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_RUN_DIR);

    case KEY_DATABASE_FILE:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "crashmanager", "DatabaseFile", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_DATABASE_FILE);

    case KEY_KDUMPSOURCE_DIR:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "crashmanager", "KernelDumpSourceDir", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_KDUMPSOURCE_DIR);

    case KEY_IPC_SOCK_ADDR:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "common", "IpcSocketFile", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_IPC_SOCK_ADDR);

    default:
      break;
    }

  g_error ("No default value provided for string key");

  return NULL;
}

static gint64
get_long_option (CdmOptions *opts,
                 const gchar *section_name,
                 const gchar *property_name,
                 GError **error)
{
  g_assert (opts);
  g_assert (section_name);
  g_assert (property_name);

  if (opts->has_conf)
    {
      g_autofree gchar *tmp = g_key_file_get_string (opts->conf, section_name, property_name, NULL);

      if (tmp != NULL)
        {
          gchar *c = NULL;
          gint64 ret;

          ret = g_ascii_strtoll (tmp, &c, 10);

          if (*c != tmp[0])
            return ret;
        }
    }

  g_set_error_literal (error, g_quark_from_string ("cdm-options"), 0, "Cannot convert option to long");

  return -1;
}

gint64
cdm_options_long_for (CdmOptions *opts,
                      CdmOptionsKey key)
{
  g_autoptr (GError) error = NULL;
  gint64 value = 0;

  switch (key)
    {
    case KEY_FILESYSTEM_MIN_SIZE:
      value = get_long_option (opts, "crashhandler", "FileSystemMinSize", &error);
      if (error != NULL)
        value = CDM_FILESYSTEM_MIN_SIZE;
      break;

    case KEY_ELEVATED_NICE_VALUE:
      value = get_long_option (opts, "crashhandler", "ElevatedNiceValue", &error);
      if (error != NULL)
        value = CDM_ELEVATED_NICE_VALUE;
      break;

    case KEY_IPC_TIMEOUT_SEC:
      value = get_long_option (opts, "common", "IpcSocketTimeout", &error);
      if (error != NULL)
        value = CDM_IPC_TIMEOUT_SEC;
      break;

    case KEY_CRASHDUMP_DIR_MIN_SIZE:
      value = get_long_option (opts, "crashmanager", "MinCrashdumpDirSize", &error);
      if (error != NULL)
        value = CDM_CRASHDUMP_DIR_MIN_SIZE;
      break;

    case KEY_CRASHDUMP_DIR_MAX_SIZE:
      value = get_long_option (opts, "crashmanager", "MaxCrashdumpDirSize", &error);
      if (error != NULL)
        value = CDM_CRASHDUMP_DIR_MAX_SIZE;
      break;

    case KEY_CRASHFILES_MAX_COUNT:
      value = get_long_option (opts, "crashmanager", "MaxCrashdumpArchives", &error);
      if (error != NULL)
        value = CDM_CRASHFILES_MAX_COUNT;
      break;

    default:
      break;
    }

  return value;
}
