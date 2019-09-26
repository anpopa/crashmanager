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
 * \file cdm-application.c
 */

#include "cdm-application.h"
#include "cdm-utils.h"

#include <glib.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <archive.h>
#include <archive_entry.h>

#define ARCHIVE_READ_BUFFER_SIZE 4096

static void
wait_early_cdh_instances (long timeout)
{
  g_autofree gchar *exepath = NULL;
  gboolean end = FALSE;
  glong count = 0;

  exepath = g_build_filename (CDM_INSTALL_PREFIX, "bin", "crashhandler", NULL);

  do
    {
      pid_t pid = cdm_utils_first_pid_for_process (exepath);

      if (pid > 0)
        {
          g_info ("Crashhandler %d pending, wait to complete...", pid);
          sleep (1);
          count++;
        }
      else
        {
          end = TRUE;
        }

      if (!end && count >= timeout)
        {
          g_warning ("Crashhandler %d still running after initialization timeout", pid);
          end = TRUE;
        }
    } while (!end);
}

static CdmStatus
archive_early_crashdump (CdmApplication *app,
                         const gchar *crashfile)
{
  g_autoptr (GKeyFile) keyfile = NULL;
  g_autoptr (GError) error = NULL;
  g_autofree gchar *buffer = NULL;
  CdmStatus status = CDM_STATUS_OK;
  struct archive_entry *entry;
  struct archive *archive;

  archive = archive_read_new ();

  archive_read_support_filter_all (archive);
  archive_read_support_format_all (archive);

  if (archive_read_open_filename (archive, crashfile, 10240) != ARCHIVE_OK)
    {
      status = CDM_STATUS_ERROR;
    }
  else
    {
      buffer = g_new0 (gchar, ARCHIVE_READ_BUFFER_SIZE);

      while (archive_read_next_header (archive, &entry) == ARCHIVE_OK)
        {
          if (g_strcmp0 (archive_entry_pathname (entry), "info.crashdata") == 0)
            archive_read_data (archive, buffer, ARCHIVE_READ_BUFFER_SIZE);
          else
            archive_read_data_skip (archive);
        }

      if (strlen (buffer) == 0)
        {
          status = CDM_STATUS_ERROR;
        }
      else
        {
          keyfile = g_key_file_new ();

          if (!g_key_file_load_from_data (keyfile, buffer, (gsize) - 1, G_KEY_FILE_NONE, NULL))
            status = CDM_STATUS_ERROR;
        }
    }

  archive_read_free (archive);

  if (status == CDM_STATUS_OK)
    {
      g_autofree gchar *proc_name = NULL;
      g_autofree gchar *crash_id = NULL;
      g_autofree gchar *vector_id = NULL;
      g_autofree gchar *context_id = NULL;
      g_autofree gchar *context_name = NULL;
      g_autofree gchar *lifecycle_state = NULL;
      gssize proc_tstamp = 0;
      gssize proc_pid = 0;
      gssize proc_sig = 0;

      proc_name = g_key_file_get_string (keyfile, "crashdata", "ProcessName", NULL);
      crash_id = g_key_file_get_string (keyfile, "crashdata", "CrashID", NULL);
      vector_id = g_key_file_get_string (keyfile, "crashdata", "VectorID", NULL);
      context_id = g_key_file_get_string (keyfile, "crashdata", "ContextID", NULL);
      context_name = g_key_file_get_string (keyfile, "crashdata", "ContextName", NULL);
      lifecycle_state = g_key_file_get_string (keyfile, "crashdata", "LifecycleState", NULL);
      proc_tstamp = g_key_file_get_int64 (keyfile, "crashdata", "CrashTimestamp", NULL);
      proc_pid = g_key_file_get_int64 (keyfile, "crashdata", "ProcessID", NULL);
      proc_sig = g_key_file_get_int64 (keyfile, "crashdata", "CrashSignal", NULL);

      cdm_journal_add_crash (app->journal,
                             proc_name != NULL ? proc_name : "earlyprocess",
                             crash_id != NULL ? crash_id : "DEADDEADDEADDEAD",
                             vector_id != NULL ? vector_id : "DEADDEADDEADDEAD",
                             context_id != NULL ? context_id : "DEADDEADDEADDEAD",
                             context_name != NULL ? context_name : "unknown",
                             lifecycle_state != NULL ? lifecycle_state : "unknown",
                             crashfile,
                             proc_pid > 0 ? proc_pid : 0,
                             proc_sig > 0 ? proc_sig : 0,
                             (guint64)(proc_tstamp > 0 ? proc_tstamp : (g_get_real_time () / (1000000))),
                             &error);

      if (error != NULL)
        {
          g_warning ("Fail to add new crash entry in database %s", error->message);
          status = CDM_STATUS_ERROR;
        }
    }
  else
    {
      cdm_journal_add_crash (app->journal,
                             "earlyprocess",
                             "DEADDEADDEADDEAD",
                             "DEADDEADDEADDEAD",
                             "DEADDEADDEADDEAD",
                             "unknown",
                             "unknown",
                             crashfile,
                             0,
                             0,
                             (guint64)(g_get_real_time () / (10000000)),
                             &error);

      if (error != NULL)
        g_warning ("Fail to add new crash entry in database %s", error->message);
      else
        status = CDM_STATUS_OK;
    }

  return status;
}

static CdmStatus
archive_early_crashes (CdmApplication *app,
                       const gchar *crashdir)
{
  CdmStatus status = CDM_STATUS_OK;
  const gchar *nfile = NULL;
  GError *error = NULL;
  GDir *gdir = NULL;

  g_assert (app);
  g_assert (crashdir);

  gdir = g_dir_open (crashdir, 0, &error);
  if (error != NULL)
    {
      g_warning ("Fail to open crash dir %s. Error %s", crashdir, error->message);
      g_error_free (error);
      return CDM_STATUS_ERROR;
    }

  while ((nfile = g_dir_read_name (gdir)) != NULL)
    {
      g_autofree gchar *fpath = NULL;
      gboolean entry_exist = FALSE;
      g_autoptr (GError) journal_error = NULL;

      fpath = g_build_filename (crashdir, nfile, NULL);
      entry_exist = cdm_journal_archive_exist (app->journal, fpath, &journal_error);

      if (journal_error != NULL)
        {
          g_warning ("Fail to check archive status for %s. Error %s",
                     fpath,
                     journal_error->message);
          continue;
        }

      if (!entry_exist)
        {
          /*
           * wait for any early crashhandler instance to avoid sending truncated archives
           */
          wait_early_cdh_instances (5);

          if (g_strrstr (fpath, "vmlinux") != NULL)
            {
              cdm_journal_add_crash (app->journal,
                                     "kernel",
                                     "DEADDEADDEADDEAD",
                                     "DEADDEADDEADDEAD",
                                     "DEADDEADDEADDEAD",
                                     "unknown",
                                     "unknown",
                                     fpath,
                                     0,
                                     0,
                                     (guint64)(g_get_real_time () / (1000000)),
                                     &journal_error);

              if (journal_error != NULL)
                {
                  g_warning ("Fail to add kernel dump entry in database %s",
                             journal_error->message);
                  status |= CDM_STATUS_ERROR;
                }
            }
          else
            {
              status |= archive_early_crashdump (app, fpath);
            }
        }
    }

  g_dir_close (gdir);

  return status;
}

static CdmStatus
archive_kdumps (CdmApplication *app,
                const gchar *crashdir)
{
  g_autofree gchar *opt_kdumpdir = NULL;
  const gchar *nfile = NULL;
  CdmStatus status = CDM_STATUS_OK;
  GDir *gdir = NULL;
  GError *error = NULL;

  g_assert (app);
  g_assert (crashdir);

  opt_kdumpdir = cdm_options_string_for (app->options, KEY_KDUMPSOURCE_DIR);

  gdir = g_dir_open (opt_kdumpdir, 0, &error);
  if (error != NULL)
    {
      g_debug ("Kernel coredump directory %s not available", opt_kdumpdir);
      status = CDM_STATUS_OK;
      g_error_free (error);
    }
  else
    {
      while ((nfile = g_dir_read_name (gdir)) != NULL)
        {
          g_autofree gchar *fpath = NULL;
          g_autofree gchar *entry_path = NULL;

          fpath = g_build_filename (opt_kdumpdir, nfile, NULL);
          if (fpath == NULL)
            {
              g_warning ("Cannot build file path");
              continue;
            }

          entry_path = g_strdup_printf ("%s/vmlinux_%ld.core",
                                        crashdir,
                                        (g_get_real_time () / (1000000)));

          if (g_rename (fpath, entry_path) != 0)
            {
              g_warning ("Fail to move kdump %s to %s. Error %s",
                         fpath,
                         entry_path,
                         strerror (errno));
              continue;
            }
        }

      g_dir_close (gdir);
    }

  return status;
}

static void
transfer_complete (gpointer cdmjournal,
                   const gchar *file_path)
{
  CDM_UNUSED (cdmjournal);
  g_info ("Archive transfer complete for %s", file_path);
}

static void
transfer_missing_files (CdmApplication *app)
{
  gboolean finish = FALSE;

  g_assert (app);

  while (!finish)
    {
      g_autofree gchar *file = cdm_journal_get_untransferred (app->journal, NULL);

      if (file != NULL)
        {
          g_autoptr (GError) error = NULL;

          g_info ("Transfer incomplete file %s", file);

          /*
           * we only retry the transfer once if missing so we mark now the file
           * transferred to avoid getting it again from the journal
           */
          cdm_transfer_file (app->transfer, file, transfer_complete, NULL);
          cdm_journal_set_transfer (app->journal, file, TRUE, &error);

          if (error != NULL)
            g_warning ("Fail to set transfer complete for %s. Error %s", file, error->message);
        }
      else
        {
          finish = TRUE;
        }
    }
}

CdmApplication *
cdm_application_new (const gchar *config, GError **error)
{
  CdmApplication *app = g_new0 (CdmApplication, 1);

  g_assert (app);
  g_assert (error);

  g_ref_count_init (&app->rc);

#ifdef WITH_SYSTEMD
  /* construct sdnotify noexept */
  app->sdnotify = cdm_sdnotify_new ();
#endif

  /* construct transfer noexept */
  app->transfer = cdm_transfer_new ();

  /* construct options noexept */
  app->options = cdm_options_new (config);

  /* construct journal and return if an error is set */
  app->journal = cdm_journal_new (app->options, error);
  if (*error != NULL)
    return app;

  /* construct janitor noexept */
  app->janitor = cdm_janitor_new (app->options, app->journal);

  /* construct server and return if an error is set */
  app->server = cdm_server_new (app->options, app->transfer, app->journal, error);
  if (*error != NULL)
    return app;

#ifdef WITH_GENIVI_NSM
  /* construct lifecycle noexept */
  app->lifecycle = cdm_lifecycle_new ();
  cdm_server_set_lifecycle (app->server, app->lifecycle);
#endif

  /* construct main loop noexept */
  app->mainloop = g_main_loop_new (NULL, TRUE);

  return app;
}

CdmApplication *
cdm_application_ref (CdmApplication *app)
{
  g_assert (app);
  g_ref_count_inc (&app->rc);
  return app;
}

void
cdm_application_unref (CdmApplication *app)
{
  g_assert (app);

  if (g_ref_count_dec (&app->rc) == TRUE)
    {
      if (app->server != NULL)
        cdm_server_unref (app->server);

      if (app->janitor != NULL)
        cdm_janitor_unref (app->janitor);

      if (app->journal != NULL)
        cdm_journal_unref (app->journal);
#ifdef WITH_SYSTEMD
      if (app->sdnotify != NULL)
        cdm_sdnotify_unref (app->sdnotify);
#endif
#ifdef WITH_GENIVI_NSM
      if (app->lifecycle != NULL)
        cdm_lifecycle_unref (app->lifecycle);
#endif
      if (app->transfer != NULL)
        cdm_transfer_unref (app->transfer);

      if (app->options != NULL)
        cdm_options_unref (app->options);

      if (app->mainloop != NULL)
        g_main_loop_unref (app->mainloop);

      g_free (app);
    }
}

GMainLoop *
cdm_application_get_mainloop (CdmApplication *app)
{
  g_assert (app);
  return app->mainloop;
}

CdmStatus
cdm_application_execute (CdmApplication *app)
{
  g_autofree gchar *opt_crashdir = NULL;
  g_autofree gchar *opt_user = NULL;
  g_autofree gchar *opt_group = NULL;

  opt_crashdir = cdm_options_string_for (app->options, KEY_CRASHDUMP_DIR);
  opt_user = cdm_options_string_for (app->options, KEY_USER_NAME);
  opt_group = cdm_options_string_for (app->options, KEY_GROUP_NAME);

  if (g_mkdir_with_parents (opt_crashdir, 0755) != 0)
    {
      return CDM_STATUS_ERROR;
    }
  else
    {
      if (cdm_utils_chown (opt_crashdir, opt_user, opt_group) == CDM_STATUS_ERROR)
        g_warning ("Failed to set user and group owner");
    }

  if (cdm_server_bind_and_listen (app->server) != CDM_STATUS_OK)
    return CDM_STATUS_ERROR;

  /* we move the kdump archives if any */
  if (archive_kdumps (app, opt_crashdir) != CDM_STATUS_OK)
    g_warning ("Fail to add kdumps");

  /* check and process early archives including kdumps */
  if (archive_early_crashes (app, opt_crashdir) != CDM_STATUS_OK)
    g_warning ("Fail to add early crashes");

  /* transfer all untransferred files */
  transfer_missing_files (app);

  /* run the main event loop */
  g_main_loop_run (app->mainloop);

  return CDM_STATUS_OK;
}
