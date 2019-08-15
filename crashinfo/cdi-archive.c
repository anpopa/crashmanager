/* cdi-archive.c
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

#include "cdi-archive.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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
        (void) archive_read_free (ar->archive);

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

  ar->archive = archive_read_new();

  archive_read_support_filter_all(ar->archive);
  archive_read_support_format_all(ar->archive);

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

  while (archive_read_next_header(ar->archive, &entry) == ARCHIVE_OK)
    {
      printf("%s\n",archive_entry_pathname(entry));
      archive_read_data_skip(ar->archive);
    }

  return CDM_STATUS_OK;
}
