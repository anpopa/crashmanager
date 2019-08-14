/* cdm-janitor.c
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

#include "cdm-janitor.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

static gboolean janitor_source_prepare (GSource *source, gint *timeout);

static gboolean janitor_source_dispatch (GSource *source, GSourceFunc callback, gpointer cdmjanitor);

static gboolean janitor_source_callback (gpointer cdmjanitor);

static void janitor_source_destroy_notify (gpointer cdmjanitor);

static GSourceFuncs janitor_source_funcs =
{
  janitor_source_prepare,
  NULL,
  janitor_source_dispatch,
  NULL,
  NULL,
  NULL,
};

static gboolean
janitor_source_prepare (GSource *source,
                        gint *timeout)
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
      g_info ("Cleaning database size=%ld (max=%ld min=%ld) count=%ld (max=%ld)",
              crash_dir_size, janitor->max_dir_size, janitor->min_dir_size,
              entries_count, janitor->max_file_cnt);

      return TRUE;
    }

  return FALSE;
}

static gboolean
janitor_source_dispatch (GSource *source,
                         GSourceFunc callback,
                         gpointer cdmjanitor)
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
  GError *error = NULL;

  g_assert (janitor);

  victim_path = cdm_journal_get_victim (janitor->journal, &error);
  if (victim_path == NULL || error != NULL)
    {
      g_warning ("No victim available to be cleaned");
      if (error != NULL)
        {
          g_warning ("Fail to get a victim from journal %s", error->message);
          g_error_free (error);
        }
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
          g_warning ("Fail to set remove flag for victim %s: Error %s",
                     victim_basename,
                     error->message);
          g_error_free (error);
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
cdm_janitor_new (CdmOptions *options,
                 CdmJournal *journal)
{
  CdmJanitor *janitor = (CdmJanitor *)g_source_new (&janitor_source_funcs, sizeof(CdmJanitor));

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
