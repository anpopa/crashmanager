/* cdh-crashid.c
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

#include "cdh-crashid.h"
#include "cdh-data.h"
#include "cdh-coredump.h"
#include "cdm-utils.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/procfs.h>
#include <sys/user.h>

#define CRASH_ID_HIGH (6)
#define CRASH_ID_LOW (2)
#define CRASH_ID_QUALITY(x) ((x) > CRASH_ID_HIGH ? "high" : (x) < CRASH_ID_LOW ? "low" : "medium")

static CdmStatus create_crashid (CdhData *d);

static CdmStatus
create_crashid (CdhData *d)
{
  const gchar *locstr[] = { "host", "container" };
  const gchar *loc = NULL;
  g_autofree gchar *cid_str = NULL;
  g_autofree gchar *vid_str = NULL;

  g_assert (d);

  if (d->crashid_info & CID_IP_FILE_OFFSET)
    {
      if (d->crashid_info & CID_RA_FILE_OFFSET)
        {
          cid_str = g_strdup_printf ("%s%lx%s%s", d->info->name, d->ip_file_offset, d->ip_module_name, d->ra_module_name);
        }
      else
        {
          cid_str = g_strdup_printf ("%s%lx%s", d->info->name, d->ip_file_offset, d->ip_module_name);
        }
    }
  else
    {
#ifdef __x86_64__
      cid_str = g_strdup_printf ("%s%lx", d->info->name, d->regs.rip);
#elif __aarch64__
      cid_str = g_strdup_printf ("%s%lx", d->info->name, d->regs.lr);
#endif
    }

  /* hash and write crashid */
  d->info->crashid = g_strdup_printf ("%016lX", cdm_utils_jenkins_hash (cid_str));

  if (d->crashid_info & CID_RA_FILE_OFFSET)
    {
      vid_str = g_strdup_printf ("%s%lx%s", d->info->name, d->ip_file_offset, d->ra_module_name);

      d->info->vectorid = g_strdup_printf ("%016lX", cdm_utils_jenkins_hash (vid_str));
    }
  else
    {
      d->info->vectorid = g_strdup_printf ("%016lX", cdm_utils_jenkins_hash (cid_str));
    }

  /* crash context location string */
  loc = d->info->onhost == TRUE ? locstr[0] : locstr[1];
#ifdef __x86_64__
  g_info (
    "Crash in %s contextID=%s process=\"%s\" thread=\"%s\" pid=%ld cpid=%ld crashID=%s "
    "vectorID=%s confidence=\"%s\" signal=\"%s\" rip=0x%lx rbp=0x%lx retaddr=0x%lx "
    "IPFileOffset=0x%lx RAFileOffset=0x%lx IPModule=\"%s\" RAModule=\"%s\"",
    loc, d->info->contextid, d->info->name, d->info->tname, d->info->pid, d->info->cpid,
    d->info->crashid, d->info->vectorid, CRASH_ID_QUALITY (d->crashid_info),
    strsignal ((gint)d->info->sig), d->regs.rip, d->regs.rbp, d->ra, d->ip_file_offset,
    d->ra_file_offset, d->ip_module_name, d->ra_module_name);
#elif __aarch64__
  g_info (
    "Crash in %s contextID=%s process=\"%s\" thread=\"%s\" pid=%ld cpid=%ld crashID=%s "
    "vectorID=%s confidence=\"%s\" signal=\"%s\" pc=0x%lx lr=0x%lx retaddr=0x%lx "
    "IPFileOffset=0x%lx RAFileOffset=0x%lx IPModule=\"%s\" RAModule=\"%s\"",
    loc, d->info->contextid, d->info->name, d->info->tname, d->info->pid, d->info->cpid,
    d->info->crashid, d->info->vectorid, CRASH_ID_QUALITY (d->crashid_info),
    strsignal (d->info->sig), d->regs.pc, d->regs.lr, d->ra, d->ip_file_offset,
    d->ra_file_offset, d->ip_module_name, d->ra_module_name);
#endif

  return CDM_STATUS_OK;
}

CdmStatus
cdh_crashid_process (CdhData *d)
{
  g_assert (d);

  if (create_crashid (d) != CDM_STATUS_OK)
    {
      g_warning ("CrashID not generated");
      return CDM_STATUS_ERROR;
    }

  return CDM_STATUS_OK;
}
