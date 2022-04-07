/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019-2020 Alin Popa
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
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \file cdm-options.c
 */

#include "cdm-options.h"
#include "cdm-defaults.h"

#include <glib.h>
#include <stdlib.h>

static gint64 get_long_option (CdmOptions *opts, const gchar *section_name,
                               const gchar *property_name, GError **error);

CdmOptions *
cdm_options_new (const gchar *conf_path)
{
  CdmOptions *opts = g_new0 (CdmOptions, 1);

  opts->has_conf = FALSE;

  if (conf_path != NULL)
    {
      g_autoptr (GError) error = NULL;

      opts->conf = g_key_file_new ();

      g_assert (opts->conf);

      if (g_key_file_load_from_file (opts->conf, conf_path, G_KEY_FILE_NONE, &error) == TRUE)
        opts->has_conf = TRUE;
      else
        g_debug ("Cannot parse configuration file");
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
cdm_options_string_for (CdmOptions *opts, CdmOptionsKey key)
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
          gchar *tmp
              = g_key_file_get_string (opts->conf, "crashmanager", "KernelDumpSourceDir", NULL);

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

    case KEY_ELOG_SOCK_ADDR:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "crashmanager", "ELogSocketFile", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_ELOG_SOCK_ADDR);

    case KEY_TRANSFER_ADDRESS:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "crashmanager", "TransferAddress", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_TRANSFER_ADDRESS);

    case KEY_TRANSFER_PATH:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "crashmanager", "TransferPath", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_TRANSFER_PATH);

    case KEY_TRANSFER_USER:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "crashmanager", "TransferUser", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_TRANSFER_USER);

    case KEY_TRANSFER_PASSWORD:
      if (opts->has_conf)
        {
          gchar *tmp = g_key_file_get_string (opts->conf, "crashmanager", "TransferPassword", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_TRANSFER_PASSWORD);

    case KEY_TRANSFER_PUBLIC_KEY:
      if (opts->has_conf)
        {
          gchar *tmp
              = g_key_file_get_string (opts->conf, "crashmanager", "TransferPublicKey", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_TRANSFER_PUBLIC_KEY);

    case KEY_TRANSFER_PRIVATE_KEY:
      if (opts->has_conf)
        {
          gchar *tmp
              = g_key_file_get_string (opts->conf, "crashmanager", "TransferPrivateKey", NULL);

          if (tmp != NULL)
            return tmp;
        }
      return g_strdup (CDM_TRANSFER_PRIVATE_KEY);

    default:
      break;
    }

  g_error ("No default value provided for string key");

  return NULL;
}

static gint64
get_long_option (CdmOptions *opts, const gchar *section_name, const gchar *property_name,
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

  g_set_error_literal (error, g_quark_from_string ("cdm-options"), 0,
                       "Cannot convert option to long");

  return -1;
}

gint64
cdm_options_long_for (CdmOptions *opts, CdmOptionsKey key)
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

    case KEY_TRUNCATE_COREDUMPS:
      value = get_long_option (opts, "crashhandler", "TruncateCoredumps", &error);
      if (error != NULL)
        value = CDM_TRUNCATE_COREDUMPS;
      break;

    case KEY_IPC_TIMEOUT_SEC:
      value = get_long_option (opts, "common", "IpcSocketTimeout", &error);
      if (error != NULL)
        value = CDM_IPC_TIMEOUT_SEC;
      break;

    case KEY_ELOG_TIMEOUT_SEC:
      value = get_long_option (opts, "crashmanager", "ELogSocketTimeout", &error);
      if (error != NULL)
        value = CDM_ELOG_TIMEOUT_SEC;
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

    case KEY_TRANSFER_PORT:
      value = get_long_option (opts, "crashmanager", "TransferPort", &error);
      if (error != NULL)
        value = CDM_TRANSFER_PORT;
      break;

    default:
      break;
    }

  return value;
}
