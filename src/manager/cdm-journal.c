/* cdm-journal.c
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

#include "cdm-journal.h"

CdmJournal *
cdm_journal_new (const gchar *database_path)
{
  CdmJournal *journal = g_new0 (CdmJournal, 1);

  g_assert (journal);

  g_ref_count_init (&journal->rc);
  g_ref_count_inc (&journal->rc);

  return journal;
}

CdmJournal *
cdm_journal_ref (CdmJournal *journal)
{
  g_assert (journal);
  g_ref_count_inc (&journal->rc);
  return journal;
}

void
cdm_journal_unref (CdmJournal *journal)
{
  g_assert (journal);

  if (g_ref_count_dec (&journal->rc) == TRUE)
    {
      g_free (journal);
    }
}

CdmJournalEntry *
cdm_journal_entry_new ()
{
  CdmJournalEntry *entry = g_new0 (CdmJournalEntry, 1);

  g_assert (entry);

  g_ref_count_init (&entry->rc);
  g_ref_count_inc (&entry->rc);

  return entry;
}

CdmJournalEntry *
cdm_journal_entry_ref (CdmJournalEntry *entry)
{
  g_assert (entry);
  g_ref_count_inc (&entry->rc);
  return entry;
}

void
cdm_journal_entry_unref (CdmJournalEntry *entry)
{
  g_assert (entry);

  if (g_ref_count_dec (&journal->rc) == TRUE)
    {
      g_free (entry);
    }
}
