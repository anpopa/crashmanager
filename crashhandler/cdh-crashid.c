/*
 *
 * Copyright (C) 2019, Alin Popa. All rights reserved.
 *
 * This file is part of Coredumper
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \copyright Copyright © 2019, Alin Popa
 *
 */

#include "cdh_crashid.h"
#include "cdh.h"
#include "cdh_coredump.h"
#include "cdh_util.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/procfs.h>
#include <sys/user.h>

#define TMP_STRBUF_LEN (2048)
#define CRASH_ID_HIGH (6)
#define CRASH_ID_LOW (2)
#define CRASH_ID_QUALITY(x) ((x) > CRASH_ID_HIGH ? "high" : (x) < CRASH_ID_LOW ? "low" : "medium")

static cdh_status_t create_crashid(cdh_data_t *d);

static cdh_status_t create_crashid(cdh_data_t *d)
{
    const char *locstr[] = {"host", "container"};
    const char *loc;
    char tmpstr[TMP_STRBUF_LEN] = "";

    assert(d);

    memset(d->info->crashid, 0, sizeof(d->info->crashid));
    memset(d->info->vectorid, 0, sizeof(d->info->vectorid));

    if (d->crashid_info & CID_IP_FILE_OFFSET) {
        if (d->crashid_info & CID_RA_FILE_OFFSET) {
            snprintf(tmpstr, sizeof(tmpstr), "%s%lx%s%s", d->info->name, d->ip_file_offset,
                     d->ip_module_name, d->ra_module_name);
        } else {
            snprintf(tmpstr, sizeof(tmpstr), "%s%lx%s", d->info->name, d->ip_file_offset,
                     d->ip_module_name);
        }
    } else {
#ifdef __x86_64__
        snprintf(tmpstr, sizeof(tmpstr), "%s%lx", d->info->name, d->regs.rip);
#elif __aarch64__
        snprintf(tmpstr, sizeof(tmpstr), "%s%lx", d->info->name, d->regs.lr);
#endif
    }

    /* hash and write crashid */
    snprintf(d->info->crashid, sizeof(d->info->crashid), "%016lX", cdh_util_jenkins_hash(tmpstr));

    if (d->crashid_info & CID_RA_FILE_OFFSET) {
        snprintf(tmpstr, sizeof(tmpstr), "%s%lx%s", d->info->name, d->ip_file_offset,
                 d->ra_module_name);
    }

    /* hash and write vectorid */
    snprintf(d->info->vectorid, sizeof(d->info->vectorid), "%016lX", cdh_util_jenkins_hash(tmpstr));

    /* crash context location string */
    loc = d->info->onhost == true ? locstr[0] : locstr[1];
#ifdef __x86_64__
    cdhlog(LOG_INFO,
           "Crash in %s contextID=%s process=\"%s\" thread=\"%s\" pid=%d cpid=%d crashID=%s "
           "vectorID=%s confidence=\"%s\" signal=\"%s\" rip=0x%lx rbp=0x%lx retaddr=0x%lx "
           "IPFileOffset=0x%lx RAFileOffset=0x%lx IPModule=\"%s\" RAModule=\"%s\"",
           loc, d->info->contextid, d->info->name, d->info->tname, d->info->pid, d->info->cpid,
           d->info->crashid, d->info->vectorid, CRASH_ID_QUALITY(d->crashid_info),
           strsignal(d->info->sig), d->regs.rip, d->regs.rbp, d->ra, d->ip_file_offset,
           d->ra_file_offset, d->ip_module_name, d->ra_module_name);
#elif __aarch64__
    cdhlog(LOG_INFO,
           "Crash in %s contextID=%s process=\"%s\" thread=\"%s\" pid=%d cpid=%d crashID=%s "
           "vectorID=%s confidence=\"%s\" signal=\"%s\" pc=0x%lx lr=0x%lx retaddr=0x%lx "
           "IPFileOffset=0x%lx RAFileOffset=0x%lx IPModule=\"%s\" RAModule=\"%s\"",
           loc, d->info->contextid, d->info->name, d->info->tname, d->info->pid, d->info->cpid,
           d->info->crashid, d->info->vectorid, CRASH_ID_QUALITY(d->crashid_info),
           strsignal(d->info->sig), d->regs.pc, d->regs.lr, d->ra, d->ip_file_offset,
           d->ra_file_offset, d->ip_module_name, d->ra_module_name);
#endif
    return CDH_OK;
}

cdh_status_t cdh_crashid_process(cdh_data_t *d)
{
    assert(d);

    if (create_crashid(d) != CDH_OK) {
        cdhlog(LOG_ERR, "CrashID not generated");
        return CDH_NOK;
    }

    return CDH_OK;
}
