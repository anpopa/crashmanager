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
 * \file cdm-janitor.c
 */

#include "cdm-janitor.h"

#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BTOMB(x) (x / 1024 / 1024)

/**
 * @brief GSource prepare function
 */
static gboolean janitor_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource dispatch function
 */
static gboolean janitor_source_dispatch (GSource *source, GSourceFunc callback,
                                         gpointer cdmjanitor);

/**
 * @brief GSource callback function
 */
static gboolean janitor_source_callback (gpointer cdmjanitor);

/**
 * @brief GSource destroy notification callback function
 */
static void janitor_source_destroy_notify (gpointer cdmjanitor);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs janitor_source_funcs = {
  janitor_source_prepare, NULL, janitor_source_dispatch, NULL, NULL, NULL,
};

static gboolean
janitor_source_prepare (GSource *source, gint *timeout)
{
  CdmJanitor *janitor = (CdmJanitor *)source;
  gssize crash_dir_size;
  gssize entries_count;

  CDM_UNUSED (timeout);

  crash_dir_size = cdm_journal_get_data_size (janitor->journal, NULL);
  entries_count = cdm_journal_get_entry_count (janitor->journal, NULL);

  if ((crash_dir_size > janitor->max_dir_size) || (entries_count > janitor->max_file_cnt)
      || ((janitor->max_dir_size - crash_dir_size) < janitor->min_dir_size))
    {
      g_info ("Cleaning database size=%ldMB (max=%ldMB min=%ldMB) count=%ld (max=%ld)",
              BTOMB (crash_dir_size) == 0 && entries_count > 0 ? 1 : BTOMB (crash_dir_size),
              BTOMB (janitor->max_dir_size), BTOMB (janitor->min_dir_size), entries_count,
              janitor->max_file_cnt);

      return TRUE;
    }

  return FALSE;
}

static gboolean
janitor_source_dispatch (GSource *source, GSourceFunc callback, gpointer cdmjanitor)
{
  CDM_UNUSED (callback);
  CDM_UNUSED (source);
  return callback (cdmjanitor) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean
janitor_source_callback (gpointer cdmjanitor)
{
  CdmJanitor *janitor = (CdmJanitor *)cdmjanitor;
  g_autofree gchar *victim_path = NULL;

  g_autoptr (GError) error = NULL;

  g_assert (janitor);

  victim_path = cdm_journal_get_victim (janitor->journal, &error);
  if (victim_path == NULL || error != NULL)
    {
      g_warning ("No victim available to be cleaned");
      if (error != NULL)
        g_warning ("Fail to get a victim from journal %s", error->message);
    }
  else
    {
      g_autofree gchar *victim_basename = g_path_get_basename (victim_path);

      g_info ("Remove old crashdump entry %s", victim_basename);

      if (g_remove (victim_path) == -1)
        {
          if (errno != ENOENT)
            g_error ("Fail to remove file %s", victim_path);
        }

      cdm_journal_set_removed (janitor->journal, victim_path, TRUE, &error);
      if (error != NULL)
        {
          g_warning ("Fail to set remove flag for victim %s: Error %s", victim_basename,
                     error->message);
        }
    }

  return TRUE;
}

static void
janitor_source_destroy_notify (gpointer cdmjanitor)
{
  CdmJanitor *janitor = (CdmJanitor *)cdmjanitor;

  g_assert (janitor);
  g_debug ("Janitor destroy notification");
}

CdmJanitor *
cdm_janitor_new (CdmOptions *options, CdmJournal *journal)
{
  CdmJanitor *janitor = (CdmJanitor *)g_source_new (&janitor_source_funcs, sizeof (CdmJanitor));

  g_assert (janitor);

  g_ref_count_init (&janitor->rc);

  janitor->journal = cdm_journal_ref (journal);

  janitor->max_dir_size = cdm_options_long_for (options, KEY_CRASHDUMP_DIR_MAX_SIZE) * 1024 * 1024;
  janitor->min_dir_size = cdm_options_long_for (options, KEY_CRASHDUMP_DIR_MIN_SIZE) * 1024 * 1024;
  janitor->max_file_cnt = cdm_options_long_for (options, KEY_CRASHFILES_MAX_COUNT);

  g_source_set_callback (CDM_EVENT_SOURCE (janitor), G_SOURCE_FUNC (janitor_source_callback),
                         janitor, janitor_source_destroy_notify);
  g_source_attach (CDM_EVENT_SOURCE (janitor), NULL);

  return janitor;
}

CdmJanitor *
cdm_janitor_ref (CdmJanitor *janitor)
{
  g_assert (janitor);
  g_ref_count_inc (&janitor->rc);
  return janitor;
}

void
cdm_janitor_unref (CdmJanitor *janitor)
{
  g_assert (janitor);

  if (g_ref_count_dec (&janitor->rc) == TRUE)
    {
      cdm_journal_unref (janitor->journal);
      g_source_unref (CDM_EVENT_SOURCE (janitor));
    }
}
