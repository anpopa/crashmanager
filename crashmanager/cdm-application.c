/* cdm-application.c
 *
 * Copyright 2019 Alin Popa <alin.popa@fxapp.ro>
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
      GError *journal_error = NULL;

      fpath = g_build_filename (crashdir, nfile, NULL);
      entry_exist = cdm_journal_archive_exist (app->journal, fpath, &journal_error);

      if (journal_error != NULL)
        {
          g_warning ("Fail to check archive status for %s. Error %s",
                     fpath,
                     journal_error->message);
          g_error_free (journal_error);
          continue;
        }

      if (!entry_exist)
        {
          gboolean iskdump = FALSE;
          guint64 dbid;

          if (g_strrstr (fpath, "vmlinux") != NULL)
            iskdump = TRUE;

          /*
           * wait for any early crashhandler instance to avoid sending
           * truncated archives
           */
          wait_early_cdh_instances (5);

          dbid = cdm_journal_add_crash (app->journal,
                                        iskdump ? "kdump" : "early",
                                        iskdump ? "kdump" : "early",
                                        iskdump ? "kdump" : "early",
                                        iskdump ? "kdump" : "early",
                                        fpath,
                                        0,
                                        0,
                                        0,
                                        &journal_error);

          if (journal_error != NULL)
            {
              g_warning ("Fail to add new crash entry in database %s", journal_error->message);
              g_error_free (journal_error);
              status |= CDM_STATUS_ERROR;
            }
          else
            {
              g_info ("New crash entry added to database with id %016lX", dbid);
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

          entry_path = g_strdup_printf ("%s/vmlinux_%ld.core", crashdir, g_get_real_time ());

          if (g_rename (fpath, entry_path) != 0)
            {
              g_warning ("Fail to move kdump %s to %s. Error %s", fpath, entry_path, strerror (errno));
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
cdm_application_new (const gchar *config)
{
  CdmApplication *app = g_new0 (CdmApplication, 1);

  g_assert (app);

  g_ref_count_init (&app->rc);

#ifdef WITH_SYSTEMD
  app->sdnotify = cdm_sdnotify_new ();
#endif
#ifdef WITH_GENIVI_NSM
  app->lifecycle = cdm_lifecycle_new ();
#endif
  app->transfer = cdm_transfer_new ();

  app->options = cdm_options_new (config);
  app->journal = cdm_journal_new (app->options);
  app->janitor = cdm_janitor_new (app->options, app->journal);
  app->server = cdm_server_new (app->options, app->transfer, app->journal);

#ifdef WITH_GENIVI_NSM
  cdm_server_set_lifecycle (app->server, app->lifecycle);
#endif

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
  g_assert (app->options);
  g_assert (app->server);
  g_assert (app->janitor);
  g_assert (app->journal);
#ifdef WITH_SYSTEMD
  g_assert (app->sdnotify);
#endif
#ifdef WITH_GENIVI_NSM
  g_assert (app->lifecycle);
#endif
  g_assert (app->transfer);

  if (g_ref_count_dec (&app->rc) == TRUE)
    {
      cdm_server_unref (app->server);
      cdm_janitor_unref (app->janitor);
      cdm_journal_unref (app->journal);
#ifdef WITH_SYSTEMD
      cdm_sdnotify_unref (app->sdnotify);
#endif
#ifdef WITH_GENIVI_NSM
      cdm_lifecycle_unref (app->lifecycle);
#endif
      cdm_transfer_unref (app->transfer);
      cdm_options_unref (app->options);

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
