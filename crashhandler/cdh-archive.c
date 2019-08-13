/* cdh-archive.c
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

#include "cdh-archive.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static CdmStatus create_file_chunk (CdhArchive *ar);

static CdmStatus stream_chunk_write (CdhArchive *ar, void *buf, gssize size);

static gssize real_file_size (const gchar *fpath);

CdhArchive *
cdh_archive_new (void)
{
  CdhArchive *ar = g_new0 (CdhArchive, 1);

  g_ref_count_init (&ar->rc);

  return ar;
}

CdhArchive *
cdh_archive_ref (CdhArchive *ar)
{
  g_assert (ar);
  g_ref_count_inc (&ar->rc);
  return ar;
}

void
cdh_archive_unref (CdhArchive *ar)
{
  g_assert (ar);

  if (g_ref_count_dec (&ar->rc) == TRUE)
    {
      (void)cdh_archive_close (ar);
      g_free (ar);
    }
}

CdmStatus
cdh_archive_open (CdhArchive *ar,
                  const gchar *dst,
                  time_t artime)
{
  CdmStatus status = CDM_STATUS_OK;

  g_assert (ar);
  g_assert (dst);

  ar->file_active = FALSE;
  ar->archive = archive_write_new ();

  if (archive_write_set_format_filter_by_ext (ar->archive, dst) != ARCHIVE_OK)
    {
      archive_write_add_filter_gzip (ar->archive);
      archive_write_set_format_ustar (ar->archive);
    }

  archive_write_set_format_pax_restricted (ar->archive);

  if (archive_write_open_filename (ar->archive, dst) != ARCHIVE_OK)
    {
      archive_write_free (ar->archive);
      ar->archive = NULL;
      status = CDM_STATUS_ERROR;
    }
  else
    {
      ar->archive_entry = archive_entry_new ();
    }

  ar->artime = artime;

  return status;
}

CdmStatus
cdh_archive_close (CdhArchive *ar)
{
  CdmStatus status = CDM_STATUS_OK;

  g_assert (ar);

  if (ar->archive_entry)
    {
      archive_entry_free (ar->archive_entry);
      ar->archive_entry = NULL;
    }

  if (ar->archive)
    {
      if (archive_write_close (ar->archive) != ARCHIVE_OK)
        status = CDM_STATUS_ERROR;

      archive_write_free (ar->archive);
      ar->archive = NULL;
    }

  return status;
}

CdmStatus
cdh_archive_create_file (CdhArchive *ar,
                         const gchar *dst,
                         gsize file_size)
{
  CdmStatus status = CDM_STATUS_OK;

  g_assert (ar);
  g_assert (dst);

  if (ar->file_active)
    {
      return CDM_STATUS_ERROR;
    }
  else
    {
      ar->file_active = TRUE;
      /* we don't need the file name */
      if (ar->file_name != NULL)
        g_free (ar->file_name);

      ar->file_size = (gssize)file_size;
      ar->file_write_sz = 0;
    }

  archive_entry_clear (ar->archive_entry);
  archive_entry_set_pathname (ar->archive_entry, dst);
  archive_entry_set_filetype (ar->archive_entry, AE_IFREG);
  archive_entry_set_perm (ar->archive_entry, 0644);
  archive_entry_set_size (ar->archive_entry, ar->file_size);
  archive_entry_set_birthtime (ar->archive_entry, ar->artime, 0);
  archive_entry_set_atime (ar->archive_entry, ar->artime, 0);
  archive_entry_set_ctime (ar->archive_entry, ar->artime, 0);
  archive_entry_set_mtime (ar->archive_entry, ar->artime, 0);

  archive_write_header (ar->archive, ar->archive_entry);

  return status;
}

CdmStatus
cdh_archive_write_file (CdhArchive *ar,
                        const void *buf,
                        gsize size)
{
  gssize sz;

  g_assert (ar);
  g_assert (buf);

  if (!ar->file_active)
    return CDM_STATUS_ERROR;

  sz = archive_write_data (ar->archive, buf, size);
  if (sz < 0)
    {
      g_warning ("Fail to write archive");
      return CDM_STATUS_ERROR;
    }
  else
    {
      ar->file_write_sz += sz;
    }

  return CDM_STATUS_OK;
}

CdmStatus
cdh_archive_finish_file (CdhArchive *ar)
{
  g_assert (ar);

  if (!ar->file_active)
    return CDM_STATUS_ERROR;
  else
    ar->file_active = FALSE;

  return CDM_STATUS_OK;
}

CdmStatus
cdh_archive_add_system_file (CdhArchive *ar,
                             const gchar *src,
                             const gchar *dst)
{
  CdmStatus status = CDM_STATUS_OK;

  g_assert (ar);
  g_assert (src);

  if (ar->file_active)
    {
      return CDM_STATUS_ERROR;
    }
  else
    {
      ar->file_active = TRUE;
      /* we don't need the file name */
      if (ar->file_name != NULL)
        g_free (ar->file_name);

      ar->file_size = real_file_size (src);
      ar->file_write_sz = 0;
    }

  if (g_access (src, R_OK) == 0)
    {
      gint fd;

      archive_entry_clear (ar->archive_entry);
      archive_entry_set_filetype (ar->archive_entry, AE_IFREG);
      archive_entry_set_perm (ar->archive_entry, 0644);
      archive_entry_set_size (ar->archive_entry, ar->file_size);
      archive_entry_set_birthtime (ar->archive_entry, ar->artime, 0);
      archive_entry_set_atime (ar->archive_entry, ar->artime, 0);
      archive_entry_set_ctime (ar->archive_entry, ar->artime, 0);
      archive_entry_set_mtime (ar->archive_entry, ar->artime, 0);

      if (dst == NULL)
        {
          g_autofree gchar *dpath = g_strdup_printf ("root%s", src);

          g_strdelimit (dpath, "/ ", '.');
          archive_entry_set_pathname (ar->archive_entry, dpath);
        }
      else
        {
          archive_entry_set_pathname (ar->archive_entry, dst);
        }

      archive_write_header (ar->archive, ar->archive_entry);

      if ((fd = g_open (src, O_RDONLY)) != -1)
        {
          gssize readsz = read (fd, ar->in_read_buffer, sizeof(ar->in_read_buffer));

          while (readsz > 0)
            {
              if (archive_write_data (ar->archive, ar->in_read_buffer, (gsize)readsz) < 0)
                g_warning ("Fail to write archive");
              else
                ar->file_write_sz += readsz;

              readsz = read (fd, ar->in_read_buffer, sizeof(ar->in_read_buffer));
            }

          close (fd);
        }
      else
        {
          status = CDM_STATUS_ERROR;
        }
    }
  else
    {
      status = CDM_STATUS_ERROR;
    }

  ar->file_active = FALSE;

  return status;
}

CdmStatus
cdh_archive_stream_open (CdhArchive *ar,
                         const gchar *src,
                         const gchar *dst,
                         gsize split_size)
{
  g_assert (ar);
  g_assert (dst);

  if (ar->file_active)
    return CDM_STATUS_ERROR;

  if (src == NULL)
    {
      ar->in_stream = stdin;
    }
  else if ((ar->in_stream = fopen (src, "rb")) == NULL)
    {
      g_warning ("Cannot open filename archive input stream %s. %s", src, strerror (errno));
      return CDM_STATUS_ERROR;
    }

  /* If no output is set we allow stream processing via stream read api anyway */
  ar->file_active = TRUE;
  if (ar->file_name != NULL)
    g_free (ar->file_name);

  ar->file_name = g_strdup (dst);
  ar->file_size = 0;
  ar->file_chunk_sz = (gssize)split_size;
  ar->file_chunk_cnt = 0;
  ar->file_write_sz = 0;

  return create_file_chunk (ar);
}

CdmStatus
cdh_archive_stream_read (CdhArchive *ar,
                         void *buf,
                         gsize size)
{
  gsize readsz;

  g_assert (ar);
  g_assert (ar->in_stream);
  g_assert (buf);

  if ((readsz = fread (buf, 1, size, ar->in_stream)) != size)
    {
      g_warning ("Cannot read %lu bytes from archive input stream %s", size,
                 strerror (errno));
      return CDM_STATUS_ERROR;
    }

  ar->in_stream_offset += readsz;

  return stream_chunk_write (ar, buf, (gssize)size);
}

CdmStatus
cdh_archive_stream_read_all (CdhArchive *ar)
{
  g_assert (ar);
  g_assert (ar->in_stream);

  while (!feof (ar->in_stream))
    {
      gsize readsz = fread (ar->in_read_buffer, 1, sizeof(ar->in_read_buffer), ar->in_stream);

      if (stream_chunk_write (ar, ar->in_read_buffer, (gssize)readsz) == CDM_STATUS_ERROR)
        g_warning ("Fail to write archive");

      ar->in_stream_offset += readsz;

      if (ferror (ar->in_stream))
        {
          g_warning ("Error reading from the archive input stream: %s", strerror (errno));
          return CDM_STATUS_ERROR;
        }
    }

  return CDM_STATUS_OK;
}

CdmStatus
cdh_archive_stream_move_to_offset (CdhArchive *ar,
                                   gulong offset)
{
  g_assert (ar);
  return cdh_archive_stream_move_ahead (ar, (offset - ar->in_stream_offset));
}

CdmStatus
cdh_archive_stream_move_ahead (CdhArchive *ar, gulong nbbytes)
{
  gsize toread = nbbytes;

  g_assert (ar);
  g_assert (ar->in_stream);

  while (toread > 0)
    {
      gsize chunksz = toread > ARCHIVE_READ_BUFFER_SZ ? ARCHIVE_READ_BUFFER_SZ : toread;
      gsize readsz = fread (ar->in_read_buffer, 1, chunksz, ar->in_stream);

      if (readsz != chunksz)
        {
          g_warning ("Cannot move ahead by %lu bytes from src. Read %lu bytes", nbbytes, readsz);
          return CDM_STATUS_ERROR;
        }

      if (stream_chunk_write (ar, ar->in_read_buffer, (gssize)readsz) == CDM_STATUS_ERROR)
        g_warning ("Fail to write archive");

      toread -= chunksz;
    }

  ar->in_stream_offset += nbbytes;

  return CDM_STATUS_OK;
}

gsize
cdh_archive_stream_get_offset (CdhArchive *ar)
{
  g_assert (ar);
  return ar->in_stream_offset;
}

CdmStatus
cdh_archive_stream_close (CdhArchive *ar)
{
  g_assert (ar);

  if (ar->file_active == FALSE)
    return CDM_STATUS_ERROR;

  if (ar->file_name != NULL)
    {
      g_free (ar->file_name);
      ar->file_name = NULL;
    }

  ar->file_active = FALSE;

  return CDM_STATUS_OK;
}

gboolean
cdh_archive_file_active (CdhArchive *ar)
{
  g_assert (ar);
  return ar->file_active;
}

static CdmStatus
create_file_chunk (CdhArchive *ar)
{
  g_autofree gchar *dname = NULL;

  g_assert (ar);

  if (ar->file_active == FALSE)
    return CDM_STATUS_ERROR;

  dname = g_strdup_printf ("%s.%04ld", ar->file_name, ar->file_chunk_cnt);

  ar->file_write_sz = 0;
  ar->file_chunk_cnt++;

  archive_entry_clear (ar->archive_entry);
  archive_entry_set_pathname (ar->archive_entry, dname);
  archive_entry_set_filetype (ar->archive_entry, AE_IFREG);
  archive_entry_set_perm (ar->archive_entry, 0644);
  archive_entry_set_size (ar->archive_entry, ar->file_chunk_sz);
  archive_entry_set_birthtime (ar->archive_entry, ar->artime, 0);
  archive_entry_set_atime (ar->archive_entry, ar->artime, 0);
  archive_entry_set_ctime (ar->archive_entry, ar->artime, 0);
  archive_entry_set_mtime (ar->archive_entry, ar->artime, 0);

  archive_write_header (ar->archive, ar->archive_entry);

  return CDM_STATUS_OK;
}

static CdmStatus
stream_chunk_write (CdhArchive *ar,
                    void *buf,
                    gssize size)
{
  gssize towrite;
  gssize writesz;

  g_assert (ar);

  if (ar->file_active == FALSE)
    return CDM_STATUS_ERROR;

  towrite = ar->file_chunk_sz - ar->file_write_sz;
  writesz = archive_write_data (ar->archive, buf, (gsize)(size < towrite ? size : towrite));

  if (writesz < 0)
    {
      g_warning ("Fail to write archive");
    }
  else
    {
      ar->file_write_sz += writesz;
      size -= writesz;
    }

  if (ar->file_write_sz == ar->file_chunk_sz)
    {
      if (create_file_chunk (ar) == CDM_STATUS_OK)
        {
          if (size > 0)
            writesz = archive_write_data (ar->archive, buf + writesz, (gsize)size);

          if (writesz < 0)
            {
              g_warning ("Fail to write archive");
            }
          else
            {
              ar->file_write_sz += writesz;
              size -= writesz;
            }
        }
      else
        {
          return CDM_STATUS_ERROR;
        }
    }

  if (size != 0)
    g_warning ("Fail to write requiered size in chunk write");

  return CDM_STATUS_OK;
}

static gssize
real_file_size (const gchar *fpath)
{
  GStatBuf file_stat;
  gssize retval = 0;

  if (g_stat (fpath, &file_stat) == 0)
    {
      if (file_stat.st_size > 0)
        {
          retval = file_stat.st_size;
        }
      else if (g_str_has_prefix (fpath, "/proc"))
        {
          FILE *infile = fopen (fpath, "rb");
          gchar buf[1024];

          if (infile != NULL)
            {
              while (!feof (infile))
                {
                  retval += (gssize)fread (buf, 1, sizeof(buf), infile);
                }

              fclose (infile);
            }
        }
      else
        {
          g_info ("File has size zero: %s", fpath);
        }
    }
  else
    {
      g_warning ("Cannot access file %s", fpath);
    }

  return retval;
}
