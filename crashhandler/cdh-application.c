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
 * \file cdh-application.c
 */

#include "cdh-application.h"
#include "cdm-utils.h"

#include <glib.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static CdmStatus read_args (CdhApplication *app, gint argc, gchar **argv);

static CdmStatus check_disk_space (const gchar *path, gsize min);

static CdmStatus init_crashdump_archive (CdhApplication *app, const gchar *dirname);

static CdmStatus close_crashdump_archive (CdhApplication *app, const gchar *dirname);

CdhApplication *
cdh_application_new (const gchar *config_path)
{
  CdhApplication *app = g_new0 (CdhApplication, 1);

  g_ref_count_init (&app->rc);

  app->options = cdm_options_new (config_path);
  g_assert (app->options);

  app->archive = cdh_archive_new ();
  g_assert (app->archive);

  app->context = cdh_context_new (app->options, app->archive);
  g_assert (app->context);

  app->coredump = cdh_coredump_new (app->context, app->archive);
  g_assert (app->coredump);

#if defined(WITH_CRASHMANAGER)
  app->manager = cdh_manager_new (app->options);
  g_assert (app->manager);

  cdh_coredump_set_manager (app->coredump, app->manager);
  cdh_context_set_manager (app->context, app->manager);
#endif

  return app;
}

CdhApplication *
cdh_application_ref (CdhApplication *app)
{
  g_assert (app);
  g_ref_count_inc (&app->rc);
  return app;
}

void
cdh_application_unref (CdhApplication *app)
{
  g_assert (app);

  if (g_ref_count_dec (&app->rc) == TRUE)
    {
      if (app->context)
        cdh_context_unref (app->context);

      if (app->archive)
        cdh_archive_unref (app->archive);

      if (app->options)
        cdm_options_unref (app->options);

#if defined(WITH_CRASHMANAGER)
      if (app->manager)
        cdh_manager_unref (app->manager);
#endif
      if (app->coredump)
        cdh_coredump_unref (app->coredump);

      g_free (app);
    }
}

static CdmStatus
read_args (CdhApplication *app,
           gint argc,
           gchar **argv)
{
  g_assert (app);

  if (argc < 6)
    {
      g_warning ("Usage: coredumper tstamp pid cpid sig procname");
      return CDM_STATUS_ERROR;
    }

  app->context->tstamp = g_ascii_strtoull (argv[1], NULL, 10);
  if (app->context->tstamp == 0)
    {
      g_warning ("Unable to read tstamp argument %s", argv[1]);
      return CDM_STATUS_ERROR;
    }

  app->context->pid = g_ascii_strtoll (argv[2], NULL, 10);
  if (app->context->pid == 0)
    {
      g_warning ("Unable to read pid argument %s", argv[2]);
      return CDM_STATUS_ERROR;
    }

  app->context->cpid = g_ascii_strtoll (argv[3], NULL, 10);
  if (app->context->cpid == 0)
    {
      g_warning ("Unable to read context cpid argument %s", argv[3]);
      return CDM_STATUS_ERROR;
    }

  app->context->sig = g_ascii_strtoll (argv[4], NULL, 10);
  if (app->context->sig == 0)
    {
      g_warning ("Unable to read sig argument %s", argv[4]);
      return CDM_STATUS_ERROR;
    }

  app->context->tname = g_strdup (argv[5]);
  app->context->name = g_strdup (argv[5]);

  return CDM_STATUS_OK;
}

static CdmStatus
check_disk_space (const gchar *path,
                  gsize min)
{
  struct statvfs stat;
  gsize free_sz = 0;

  g_assert (path);

  if (statvfs (path, &stat) < 0)
    {
      g_warning ("Cannot stat disk space on %s: %s", path, strerror (errno));
      return CDM_STATUS_ERROR;
    }

  /* free space: size of block * number of free blocks (>>20 => MB) */
  free_sz = (stat.f_bsize * stat.f_bavail) >> 20;

  if (free_sz < min)
    {
      g_warning ("Insufficient disk space for coredump: %lu MB.", free_sz);
      return CDM_STATUS_ERROR;
    }

  return CDM_STATUS_OK;
}

static CdmStatus
init_crashdump_archive (CdhApplication *app, const gchar *dirname)
{
  g_autofree gchar *aname = NULL;

  g_assert (app);
  g_assert (dirname);

  aname = g_strdup_printf (ARCHIVE_NAME_PATTERN,
                           dirname,
                           app->context->name,
                           app->context->pid,
                           app->context->tstamp);

  if (cdh_archive_open (app->archive, aname, (time_t)app->context->tstamp) != CDM_STATUS_OK)
    return CDM_STATUS_ERROR;

  return CDM_STATUS_OK;
}

static CdmStatus
close_crashdump_archive (CdhApplication *app,
                         const gchar *dirname)
{
  g_autofree gchar *aname = NULL;
  g_autofree gchar *opt_user = NULL;
  g_autofree gchar *opt_group = NULL;

  g_assert (app);

  if (cdh_archive_close (app->archive) != CDM_STATUS_OK)
    return CDM_STATUS_ERROR;

  aname = g_strdup_printf (ARCHIVE_NAME_PATTERN,
                           dirname,
                           app->context->name,
                           app->context->pid,
                           app->context->tstamp);

  opt_user = cdm_options_string_for (app->options, KEY_USER_NAME);
  opt_group = cdm_options_string_for (app->options, KEY_GROUP_NAME);

  if (cdm_utils_chown (aname, opt_user, opt_group) == CDM_STATUS_ERROR)
    g_warning ("Failed to set user and group owner for archive %s", aname);

  return CDM_STATUS_OK;
}

static void
do_crash_actions (CdmOptions *options,
                  const gchar *proc_name,
                  gboolean postcore)
{
  GKeyFile *key_file = NULL;
  gchar **groups = NULL;

  g_assert (options);

  key_file = cdm_options_get_key_file (options);
  groups = g_key_file_get_groups (key_file, NULL);

  for (gint i = 0; groups[i] != NULL; i++)
    {
      g_autoptr (GError) error = NULL;
      g_autofree gchar *proc_key = NULL;
      g_autofree gchar *victim_key = NULL;
      gboolean key_postcore = TRUE;
      gchar *gname = groups[i];
      pid_t victim_pid;
      gint signal_key;

      if (g_regex_match_simple ("crashaction-*", gname, 0, 0) == FALSE)
        continue;

      proc_key = g_key_file_get_string (key_file, gname, "ProcName", &error);
      if (error != NULL)
        continue;

      if (g_regex_match_simple (proc_key, proc_name, 0, 0) == FALSE)
        continue;

      key_postcore = g_key_file_get_boolean (key_file, gname, "PostCore", &error);
      if (error != NULL)
        continue;

      if (key_postcore != postcore)
        continue;

      victim_key = g_key_file_get_string (key_file, gname, "Victim", &error);
      if (error != NULL)
        continue;

      signal_key = g_key_file_get_integer (key_file, gname, "Signal", &error);
      if (error != NULL)
        continue;

      victim_pid = cdm_utils_first_pid_for_process (victim_key);
      if (victim_pid < 1)
        {
          g_debug ("No victim '%s' found for crash action", victim_key);
          continue;
        }
      else
        {
          g_info ("Victim '%s' found with pid %d, for crash action", victim_key, victim_pid);
        }

      if (kill (victim_pid, signal_key) == -1)
        {
          g_warning ("Fail to send signal %d to process %d (%s). Error %s",
                     signal_key,
                     victim_pid,
                     victim_key,
                     strerror (errno));
        }
    }

  g_strfreev (groups);
}

CdmStatus
cdh_application_execute (CdhApplication *app,
                         gint argc,
                         gchar *argv[])
{
  CdmStatus status = CDM_STATUS_OK;
  g_autofree gchar *opt_coredir = NULL;
  g_autofree gchar *opt_user = NULL;
  g_autofree gchar *opt_group = NULL;
  gchar *procname = NULL;
  gsize opt_fs_min_size;
  gint opt_nice_value;

  g_assert (app);

  if (read_args (app, argc, argv) < 0)
    {
      status = CDM_STATUS_ERROR;
      goto enter_cleanup;
    }

  procname = cdm_utils_get_procname (app->context->pid);
  if (procname != NULL)
    {
      g_free (app->context->name);
      app->context->name = procname;
    }

  g_strdelimit (app->context->name, ":/\\!*", '_');

  g_info ("New process crash: name=%s pid=%ld signal=%ld timestamp=%lu",
          app->context->name,
          app->context->pid,
          app->context->sig,
          app->context->tstamp);

  app->context->pexe = cdm_utils_get_procexe (app->context->pid);

#if defined(WITH_CRASHMANAGER)
  if (cdh_manager_connect (app->manager) != CDM_STATUS_OK)
    {
      g_warning ("Fail to connect to manager socket");
    }
  else
    {
      CdmMessage msg;
      CdmMessageDataNew msg_data;

      cdm_message_init (&msg,
                        CDM_CORE_NEW,
                        (guint16)((gulong)app->context->pid | app->context->tstamp));

      msg_data.pid = app->context->pid;
      msg_data.coresig = app->context->sig;
      msg_data.tstamp = app->context->tstamp;
      memcpy (msg_data.tname, app->context->tname, strlen (app->context->tname) + 1);
      memcpy (msg_data.pname, app->context->name, strlen (app->context->name) + 1);

      cdm_message_set_data (&msg, &msg_data, sizeof(msg_data));
      if (cdh_manager_send (app->manager, &msg) == CDM_STATUS_ERROR)
        g_warning ("Failed to send new message to manager");
    }
#endif

  /* get optionals */
  opt_coredir = cdm_options_string_for (app->options, KEY_CRASHDUMP_DIR);
  opt_user = cdm_options_string_for (app->options, KEY_USER_NAME);
  opt_group = cdm_options_string_for (app->options, KEY_GROUP_NAME);
  opt_fs_min_size = (gsize)cdm_options_long_for (app->options, KEY_FILESYSTEM_MIN_SIZE);
  opt_nice_value = (gint)cdm_options_long_for (app->options, KEY_ELEVATED_NICE_VALUE);

  g_debug ("Coredump appbase path %s", opt_coredir);

  if (nice (opt_nice_value) != opt_nice_value)
    g_warning ("Failed to change crashhandler priority");

  if (g_mkdir_with_parents (opt_coredir, 0755) != 0)
    {
      status = CDM_STATUS_ERROR;
      goto enter_cleanup;
    }
  else
    {
      if (cdm_utils_chown (opt_coredir, opt_user, opt_group) == CDM_STATUS_ERROR)
        g_warning ("Failed to set suer and group owner");
    }

  if (check_disk_space (opt_coredir, opt_fs_min_size) != CDM_STATUS_OK)
    {
      status = CDM_STATUS_ERROR;
      goto enter_cleanup;
    }

  if (init_crashdump_archive (app, opt_coredir) != CDM_STATUS_OK)
    {
      g_warning ("Fail to create crashdump archive");
      status = CDM_STATUS_ERROR;
      goto enter_cleanup;
    }

  do_crash_actions (app->options, app->context->name, FALSE);

  if (cdh_context_generate_prestream (app->context) != CDM_STATUS_OK)
    g_warning ("Failed to generate the context file, continue with coredump");

  if (cdh_coredump_generate (app->coredump) != CDM_STATUS_OK)
    {
      g_warning ("Coredump handling failed");
      status = CDM_STATUS_ERROR;
      goto enter_cleanup;
    }

  if (cdh_context_generate_poststream (app->context) != CDM_STATUS_OK)
    g_warning ("Failed to generate the context file, continue with coredump");

  do_crash_actions (app->options, app->context->name, FALSE);

  if (close_crashdump_archive (app, opt_coredir) != CDM_STATUS_OK)
    g_warning ("Failed to close corectly the crashdump archive");

enter_cleanup:
#if defined(WITH_CRASHMANAGER)
  if (cdh_manager_connected (app->manager))
    {
      g_autofree gchar *file_path = NULL;
      CdmMessageDataComplete msg_data;
      CdmMessageType type;
      CdmMessage msg;
      gssize sz = 0;

      type = (status == CDM_STATUS_OK ? CDM_CORE_COMPLETE : CDM_CORE_FAILED);

      cdm_message_init (&msg, type, (guint16)((gulong)app->context->pid | app->context->tstamp));

      memset (&msg_data, 0, sizeof(CdmMessageDataComplete));
      file_path = g_strdup_printf (ARCHIVE_NAME_PATTERN,
                                   opt_coredir,
                                   app->context->name,
                                   app->context->pid,
                                   app->context->tstamp);

      sz = snprintf (msg_data.core_file, CDM_MESSAGE_FILENAME_LEN, "%s", file_path);
      if (sz > CDM_MESSAGE_FILENAME_LEN)
        g_warning ("Fail to set the complete file name. Name too long!");

      sz = snprintf (msg_data.context_name, CDM_CRASHCONTEXT_LEN, "%s", app->context->context_name);
      if (sz > CDM_CRASHCONTEXT_LEN)
        g_warning ("Fail to set the context name. Name too long!");

      sz = snprintf (msg_data.lifecycle_state, CDM_LIFECYCLESTATE_LEN, "%s", app->context->lifecycle_state);
      if (sz > CDM_LIFECYCLESTATE_LEN)
        g_warning ("Fail to set the lifecycle state. Name too long!");

      cdm_message_set_data (&msg, &msg_data, sizeof(msg_data));

      if (cdh_manager_send (app->manager, &msg) == CDM_STATUS_ERROR)
        g_warning ("Failed to send status message to manager");

      if (cdh_manager_disconnect (app->manager) != CDM_STATUS_OK)
        g_warning ("Fail to disconnect to manager socket");
    }
#endif

  return status;
}
