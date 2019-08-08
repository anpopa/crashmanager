/* cdm-utils.h
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

#include "cdm-utils.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define TMP_BUFFER_SIZE (1024)
#define UNKNOWN_OS_VERSION "Unknown version"

static gchar *os_version = NULL;

gchar *
cdm_utils_get_procname (gint64 pid)
{
  g_autofree gchar *statfile = NULL;
  gboolean done = FALSE;
  gchar *retval = NULL;
  gchar tmpbuf[TMP_BUFFER_SIZE];
  FILE *fstm;

  statfile = g_strdup_printf ("/proc/%ld/status", pid);

  if ((fstm = fopen (statfile, "r")) == NULL)
    {
      g_warning ("Open status file '%s' for dumping %s", statfile, strerror (errno));
      return NULL;
    }

  while (fgets (tmpbuf, sizeof(tmpbuf), fstm) && !done)
    {
      gchar *name = g_strrstr (tmpbuf, "Name:");

      if (name != NULL)
        {
          retval = g_strdup (name + strlen ("Name:") + 1);

          if (retval != NULL)
            {
              g_strstrip (retval);
            }
          done = TRUE;
        }
    }

  fclose (fstm);

  return retval;
}

guint64
cdm_utils_jenkins_hash (const gchar *key)
{
  guint64 hash, i;

  for (hash = i = 0; i < strlen (key); ++i)
    {
      hash += (guint64)key[i];
      hash += (hash << 10);
      hash ^= (hash >> 6);
    }

  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  return hash;
}

const gchar *
cdm_utils_get_osversion (void)
{
  gchar *retval = NULL;

  if (os_version != NULL)
    {
      return os_version;
    }
  else
    {
      gchar tmpbuf[TMP_BUFFER_SIZE];
      gboolean done = FALSE;
      FILE *fstm;

      if ((fstm = fopen ("/etc/os-release", "r")) == NULL)
        {
          g_warning ("Fail to open /etc/os-release file. Error %s", strerror (errno));
        }
      else
        {
          while (fgets (tmpbuf, sizeof(tmpbuf), fstm) && !done)
            {
              gchar *version = g_strrstr (tmpbuf, "VERSION=");

              if (version != NULL)
                {
                  retval = g_strdup (version + strlen ("VERSION=") + 1);
                  if (retval != NULL)
                    {
                      g_strstrip (retval);
                      g_strdelimit (retval, "\"", '\0');
                    }

                  done = TRUE;
                }
            }

          fclose (fstm);
        }
    }

  if (retval == NULL)
    {
      retval = UNKNOWN_OS_VERSION;
    }

  return retval;
}
