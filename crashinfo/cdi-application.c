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
 * \file cdi-application.c
 */

#include "cdi-application.h"
#include "cdm-utils.h"

#include <fcntl.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

CdiApplication *
cdi_application_new (const gchar *config, GError **error)
{
  CdiApplication *app = g_new0 (CdiApplication, 1);

  g_assert (app);
  g_assert (error);

  g_ref_count_init (&app->rc);

  /* construct options noexept */
  app->options = cdm_options_new (config);

  /* construct journal and return if an error is set */
  app->journal = cdi_journal_new (app->options, error);

  return app;
}

CdiApplication *
cdi_application_ref (CdiApplication *app)
{
  g_assert (app);
  g_ref_count_inc (&app->rc);
  return app;
}

void
cdi_application_unref (CdiApplication *app)
{
  g_assert (app);

  if (g_ref_count_dec (&app->rc) == TRUE)
    {
      if (app->journal != NULL)
        cdi_journal_unref (app->journal);

      if (app->options != NULL)
        cdm_options_unref (app->options);

      g_free (app);
    }
}

void
cdi_application_list_entries (CdiApplication *app)
{
  g_assert (app);
  cdi_journal_list_entries (app->journal, NULL);
}

void
cdi_application_list_content (CdiApplication *app, const gchar *fpath)
{
  g_autoptr (CdiArchive) archive = NULL;
  CdmStatus status = CDM_STATUS_OK;

  g_assert (app);
  g_assert (fpath);

  archive = cdi_archive_new ();

  if (g_access (fpath, R_OK) == 0)
    status = cdi_archive_read_open (archive, fpath);
  else
    {
      g_autofree gchar *opt_coredir = NULL;
      g_autofree gchar *dbpath = NULL;

      opt_coredir = cdm_options_string_for (app->options, KEY_CRASHDUMP_DIR);
      dbpath = g_build_filename (opt_coredir, fpath, NULL);

      status = cdi_archive_read_open (archive, dbpath);
    }

  if (status == CDM_STATUS_OK)
    (void)cdi_archive_list_stdout (archive);
  else
    g_print ("Cannot open file: %s\n", fpath);
}

void
cdi_application_print_info (CdiApplication *app, const gchar *fpath)
{
  g_autoptr (CdiArchive) archive = NULL;
  CdmStatus status = CDM_STATUS_OK;

  g_assert (app);
  g_assert (fpath);

  archive = cdi_archive_new ();

  if (g_access (fpath, R_OK) == 0)
    status = cdi_archive_read_open (archive, fpath);
  else
    {
      g_autofree gchar *opt_coredir = NULL;
      g_autofree gchar *dbpath = NULL;

      opt_coredir = cdm_options_string_for (app->options, KEY_CRASHDUMP_DIR);
      dbpath = g_build_filename (opt_coredir, fpath, NULL);

      status = cdi_archive_read_open (archive, dbpath);
    }

  if (status == CDM_STATUS_OK)
    (void)cdi_archive_print_info (archive);
  else
    g_print ("Cannot open file: %s\n", fpath);
}

void
cdi_application_print_epilog (CdiApplication *app, const gchar *fpath)
{
  g_autoptr (CdiArchive) archive = NULL;
  CdmStatus status = CDM_STATUS_OK;

  g_assert (app);
  g_assert (fpath);

  archive = cdi_archive_new ();

  if (g_access (fpath, R_OK) == 0)
    status = cdi_archive_read_open (archive, fpath);
  else
    {
      g_autofree gchar *opt_coredir = NULL;
      g_autofree gchar *dbpath = NULL;

      opt_coredir = cdm_options_string_for (app->options, KEY_CRASHDUMP_DIR);
      dbpath = g_build_filename (opt_coredir, fpath, NULL);

      status = cdi_archive_read_open (archive, dbpath);
    }

  if (status == CDM_STATUS_OK)
    (void)cdi_archive_print_epilog (archive);
  else
    g_print ("Cannot open file: %s\n", fpath);
}

void
cdi_application_print_file (CdiApplication *app, const gchar *fname, const gchar *fpath)
{
  g_autoptr (CdiArchive) archive = NULL;
  CdmStatus status = CDM_STATUS_OK;

  g_assert (app);
  g_assert (fpath);

  archive = cdi_archive_new ();

  if (g_access (fpath, R_OK) == 0)
    status = cdi_archive_read_open (archive, fpath);
  else
    {
      g_autofree gchar *opt_coredir = NULL;
      g_autofree gchar *dbpath = NULL;

      opt_coredir = cdm_options_string_for (app->options, KEY_CRASHDUMP_DIR);
      dbpath = g_build_filename (opt_coredir, fpath, NULL);

      status = cdi_archive_read_open (archive, dbpath);
    }

  if (status == CDM_STATUS_OK)
    (void)cdi_archive_print_file (archive, fname);
  else
    g_print ("Cannot open file: %s\n", fpath);
}

void
cdi_application_extract_coredump (CdiApplication *app, const gchar *fpath)
{
  g_autoptr (CdiArchive) archive = NULL;
  CdmStatus status = CDM_STATUS_OK;

  g_assert (app);
  g_assert (fpath);

  archive = cdi_archive_new ();

  if (g_access (fpath, R_OK) == 0)
    status = cdi_archive_read_open (archive, fpath);
  else
    {
      g_autofree gchar *opt_coredir = NULL;
      g_autofree gchar *dbpath = NULL;

      opt_coredir = cdm_options_string_for (app->options, KEY_CRASHDUMP_DIR);
      dbpath = g_build_filename (opt_coredir, fpath, NULL);

      status = cdi_archive_read_open (archive, dbpath);
    }

  if (status == CDM_STATUS_OK)
    {
      g_autofree gchar *cwd = g_get_current_dir ();
      (void)cdi_archive_extract_coredump (archive, cwd);
    }
  else
    g_print ("Cannot open file: %s\n", fpath);
}

void
cdi_application_print_backtrace (CdiApplication *app, gboolean all, const gchar *fpath)
{
  g_autoptr (CdiArchive) archive = NULL;
  CdmStatus status = CDM_STATUS_OK;

  g_assert (app);
  g_assert (fpath);

  archive = cdi_archive_new ();

  if (g_access (fpath, R_OK) == 0)
    status = cdi_archive_read_open (archive, fpath);
  else
    {
      g_autofree gchar *opt_coredir = NULL;
      g_autofree gchar *dbpath = NULL;

      opt_coredir = cdm_options_string_for (app->options, KEY_CRASHDUMP_DIR);
      dbpath = g_build_filename (opt_coredir, fpath, NULL);

      status = cdi_archive_read_open (archive, dbpath);
    }

  if (status == CDM_STATUS_OK)
    (void)cdi_archive_print_backtrace (archive, all);
  else
    g_print ("Cannot open file: %s\n", fpath);
}
