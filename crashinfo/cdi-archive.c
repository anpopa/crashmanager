/* cdi-archive.c
 *
 * Copyright 2019 Alin Popa <alin.popa@bmw.de>
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

#include "cdi-archive.h"
#include "cdm-defaults.h"

#include <glib/gstdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ARCHIVE_READ_BUFFER_SIZE 4096

CdiArchive *
cdi_archive_new (void)
{
  CdiArchive *ar = g_new0 (CdiArchive, 1);

  g_ref_count_init (&ar->rc);

  return ar;
}

CdiArchive *
cdi_archive_ref (CdiArchive *ar)
{
  g_assert (ar);
  g_ref_count_inc (&ar->rc);
  return ar;
}

void
cdi_archive_unref (CdiArchive *ar)
{
  g_assert (ar);

  if (g_ref_count_dec (&ar->rc) == TRUE)
    {
      if (ar->archive != NULL)
        (void)archive_read_free (ar->archive);

      g_free (ar->file_path);
      g_free (ar);
    }
}

CdmStatus
cdi_archive_read_open (CdiArchive *ar, const gchar *fname)
{
  g_assert (ar);
  g_assert (fname);

  if (ar->archive != NULL)
    return CDM_STATUS_ERROR;

  ar->archive = archive_read_new ();
  ar->file_path = g_strdup (fname);

  archive_read_support_filter_all (ar->archive);
  archive_read_support_format_all (ar->archive);

  if (archive_read_open_filename (ar->archive, fname, 10240) != ARCHIVE_OK)
    return CDM_STATUS_ERROR;

  return CDM_STATUS_OK;
}

CdmStatus
cdi_archive_list_stdout (CdiArchive *ar)
{
  struct archive_entry *entry;

  g_assert (ar);

  if (ar->archive == NULL)
    return CDM_STATUS_ERROR;

  while (archive_read_next_header (ar->archive, &entry) == ARCHIVE_OK)
    {
      g_print ("%s\n", archive_entry_pathname (entry));
      archive_read_data_skip (ar->archive);
    }

  return CDM_STATUS_OK;
}

CdmStatus
cdi_archive_print_info (CdiArchive *ar)
{
  struct archive_entry *entry;

  g_assert (ar);

  if (ar->archive == NULL)
    return CDM_STATUS_ERROR;

  while (archive_read_next_header (ar->archive, &entry) == ARCHIVE_OK)
    {
      if (g_strcmp0 (archive_entry_pathname (entry), "info.crashdata") == 0)
        archive_read_data_into_fd (ar->archive, STDOUT_FILENO);
      else
        archive_read_data_skip (ar->archive);
    }

  return CDM_STATUS_OK;
}

CdmStatus
cdi_archive_print_file (CdiArchive *ar, const gchar *fname)
{
  struct archive_entry *entry;

  g_assert (ar);
  g_assert (fname);

  if (ar->archive == NULL)
    return CDM_STATUS_ERROR;

  while (archive_read_next_header (ar->archive, &entry) == ARCHIVE_OK)
    {
      if (g_strcmp0 (archive_entry_pathname (entry), fname) == 0)
        archive_read_data_into_fd (ar->archive, STDOUT_FILENO);
      else
        archive_read_data_skip (ar->archive);
    }

  return CDM_STATUS_OK;
}

CdmStatus
cdi_archive_extract_coredump (CdiArchive *ar, const gchar *dpath)
{
  g_autoptr (GKeyFile) keyfile = g_key_file_new ();
  g_autofree gchar *buffer = g_new0 (gchar, ARCHIVE_READ_BUFFER_SIZE);
  g_autofree gchar *proc_name = NULL;
  g_autofree gchar *file_name = NULL;
  g_autoptr (GError) error = NULL;
  struct archive_entry *entry;
  gssize proc_pid = 0;
  gssize proc_tstamp = 0;
  gssize towrite = 0;
  gssize readsz = 0;
  gint output_fd;

  g_assert (ar);
  g_assert (dpath);

  if (ar->archive == NULL)
    return CDM_STATUS_ERROR;

  while (archive_read_next_header (ar->archive, &entry) == ARCHIVE_OK)
    {
      if (g_strcmp0 (archive_entry_pathname (entry), "info.crashdata") == 0)
        archive_read_data (ar->archive, buffer, ARCHIVE_READ_BUFFER_SIZE);
      else
        archive_read_data_skip (ar->archive);
    }

  if (strlen (buffer) == 0)
    return CDM_STATUS_ERROR;

  if (!g_key_file_load_from_data (keyfile, buffer, (gsize) - 1, G_KEY_FILE_NONE, NULL))
    return CDM_STATUS_ERROR;

  towrite = g_key_file_get_int64 (keyfile, "crashdata", "CoredumpSize", &error);
  if (error != NULL)
    return CDM_STATUS_ERROR;

  proc_tstamp = g_key_file_get_int64 (keyfile, "crashdata", "CrashTimestamp", &error);
  if (error != NULL)
    return CDM_STATUS_ERROR;

  proc_pid = g_key_file_get_int64 (keyfile, "crashdata", "ProcessID", &error);
  if (error != NULL)
    return CDM_STATUS_ERROR;

  proc_name = g_key_file_get_string (keyfile, "crashdata", "ProcessName", &error);
  if (error != NULL)
    return CDM_STATUS_ERROR;

  file_name = g_strdup_printf ("%s/%s.%ld.%ld.core", dpath, proc_name, proc_pid, proc_tstamp);

  g_print ("Extracting coredump with size %ld ... ", towrite);

  /* need to reopen the archive */
  archive_read_free (ar->archive);
  ar->archive = archive_read_new ();

  archive_read_support_filter_all (ar->archive);
  archive_read_support_format_all (ar->archive);

  if (archive_read_open_filename (ar->archive, ar->file_path, 10240) != ARCHIVE_OK)
    return CDM_STATUS_ERROR;

  memset (buffer, 0, ARCHIVE_READ_BUFFER_SIZE);
  output_fd = open (file_name, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  while (archive_read_next_header (ar->archive, &entry) == ARCHIVE_OK)
    {
      if (g_strrstr (archive_entry_pathname (entry), "core.") != NULL)
        {
          if ((towrite / CDM_CRASHDUMP_SPLIT_SIZE) > 0)
            {
              archive_read_data_into_fd (ar->archive, output_fd);
              towrite -= CDM_CRASHDUMP_SPLIT_SIZE;
            }
          else
            {
              while (towrite > 0)
                {
                  readsz = archive_read_data (ar->archive, buffer, ARCHIVE_READ_BUFFER_SIZE);
                  if (readsz > 0)
                    write (output_fd, buffer, (gsize)readsz);
                  towrite -= readsz;
                }
            }
        }
      else
        {
          archive_read_data_skip (ar->archive);
        }
    }

  g_print ("Done.\nNew file name: %s\n", file_name);

  close (output_fd);

  return CDM_STATUS_OK;
}

static gchar *
archive_get_exe_path (CdiArchive *ar)
{
  g_autofree gchar *buffer = g_new0 (gchar, ARCHIVE_READ_BUFFER_SIZE);

  g_autoptr (GKeyFile) keyfile = g_key_file_new ();
  g_autoptr (GError) error = NULL;
  struct archive_entry *entry;
  gchar *exe_path = NULL;

  g_assert (ar);

  /* need to reopen the archive */
  archive_read_free (ar->archive);
  ar->archive = archive_read_new ();

  archive_read_support_filter_all (ar->archive);
  archive_read_support_format_all (ar->archive);

  if (archive_read_open_filename (ar->archive, ar->file_path, 10240) != ARCHIVE_OK)
    return NULL;

  while (archive_read_next_header (ar->archive, &entry) == ARCHIVE_OK)
    {
      if (g_strcmp0 (archive_entry_pathname (entry), "info.crashdata") == 0)
        archive_read_data (ar->archive, buffer, ARCHIVE_READ_BUFFER_SIZE);
      else
        archive_read_data_skip (ar->archive);
    }

  if (strlen (buffer) == 0)
    return NULL;

  if (!g_key_file_load_from_data (keyfile, buffer, (gsize) - 1, G_KEY_FILE_NONE, NULL))
    return NULL;

  exe_path = g_key_file_get_string (keyfile, "crashdata", "ProcessExe", &error);
  if (error != NULL)
    return NULL;

  return exe_path;
}

CdmStatus
cdi_archive_print_backtrace (CdiArchive *ar, gboolean all)
{
  g_autofree gchar *tmpdir = NULL;
  g_autofree gchar *exepth = NULL;
  g_autofree gchar *rmcmd = NULL;

  g_autoptr (GError) error = NULL;
  CdmStatus status = CDM_STATUS_OK;
  gint exit_status;

  g_assert (ar);

  tmpdir = g_dir_make_tmp (NULL, &error);
  if (error != NULL)
    return CDM_STATUS_ERROR;

  if (cdi_archive_extract_coredump (ar, tmpdir) == CDM_STATUS_OK)
    {
      g_autofree gchar *command_line = NULL;

      exepth = archive_get_exe_path (ar);
      if (exepth != NULL)
        {
          g_autofree gchar *standard_output = NULL;
          g_autoptr (GError) cmd_error = NULL;

          command_line = g_strdup_printf ("sh -c \"gdb -q -ex '%s' -ex quit %s %s/*.core\"",
                                          all == FALSE ? "bt" : "thread apply all bt",
                                          exepth,
                                          tmpdir);

          g_spawn_command_line_sync (command_line, &standard_output, NULL, &exit_status, &cmd_error);

          if (cmd_error != NULL)
            g_warning ("Fail to spawn process. Error %s", cmd_error->message);
          else
            g_print ("%s", standard_output);
        }
    }

  rmcmd = g_strdup_printf ("rm -rf %s", tmpdir);
  g_spawn_command_line_sync (rmcmd, NULL, NULL, &exit_status, &error);

  if (error != NULL || exit_status != 0)
    g_warning ("Fail to remove tmp dir %s", tmpdir);

  return status;
}

