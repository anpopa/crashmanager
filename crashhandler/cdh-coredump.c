/*
 *
 * Copyright (C) 2019, Alin Popa. All rights reserved.
 *
 * This file is part of Coredumper
 *
 * \author Alin Popa <alin.popa@bmw.de>
 * \copyright Copyright © 2019, Alin Popa. \n
 *
 */

#include "cdh-coredump.h"
#include "cdh-crashid.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>

#define ALIGN(x, a) (((x) + (a)-1UL) & ~((a)-1UL))
#define NOTE_SIZE(_hdr) (sizeof(_hdr) + ALIGN((_hdr).n_namesz, 4) + (_hdr).n_descsz)

static int get_virtual_memory_phdr_nr(CdhData *d, Elf64_Addr address);

static const char *get_nt_file_region_name(char *string_tab_start, unsigned nr);

static int get_nt_file_region(CdhData *d, Elf64_Addr address, Elf64_Addr *region_start,
                              Elf64_Addr *region_end, Elf64_Off *file_start,
                              const char **region_name);

static Elf64_Addr read_virtual_memory(CdhData *d, Elf64_Addr address, int phdr_nr);

static int get_note_page_index(CdhData *d);

static CdmStatus read_notes(CdhData *d);

static CdmStatus init_coredump(CdhData *d);

static CdmStatus close_coredump(CdhData *d);

static CdmStatus read_elf_headers(CdhData *d);

static CdmStatus get_coredump_registers(CdhData *d);

static ssize_t write_callback(void *ref, void *buf, size_t size);

static ssize_t write_callback(void *ref, void *buf, size_t size)
{
    return cdh_archive_write((cdh_archive_t *)ref, buf, size);
}

static CdmStatus read_elf_headers(CdhData *d)
{
    assert(d);

    /* Read ELF header */
    if (cdh_archive_stream_read(&d->archive, &d->ehdr, sizeof(Elf64_Ehdr)) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "read_elf_headers: We have failed to read the ELF header !");
        return CDM_STATUS_ERROR;
    } else {
        Elf64_Ehdr newEhdr;

        memcpy(&newEhdr, &d->ehdr, sizeof(Elf64_Ehdr));

        /* We will add an extra NT section, so update the number of program headers */
        newEhdr.e_phnum = (Elf64_Half)(d->ehdr.e_phnum + 1);

        if (cdh_archive_write(&d->archive, &newEhdr, sizeof(Elf64_Ehdr)) < 0) {
            cdhlog(LOG_ERR, "read_elf_headers: We have failed to write the ELF header !");
            return CDM_STATUS_ERROR;
        }
    }

    /* Read until PROG position */
    if (cdh_archive_stream_move_to_offset(&d->archive, d->ehdr.e_phoff) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "cdh_stream_move_to_offset: We have failed to seek to the beginning of the "
                        "program headers");
        return CDM_STATUS_ERROR;
    }

    /* Read and store all program headers */
    d->pphdr = (Elf64_Phdr *)malloc(sizeof(Elf64_Phdr) * d->ehdr.e_phnum);
    if (d->pphdr == NULL) {
        cdhlog(LOG_ERR, "Cannot allocate Phdr memory (%d headers)", d->ehdr.e_phnum);
        return CDM_STATUS_ERROR;
    } else {
        /*
         * The first header is NOTE program header
         * Need to update offset to accomodate the new cdhinfo header
         */
        Elf64_Phdr *noteProgHeader;
        Elf64_Phdr *lastProgHeader;
        Elf64_Phdr cdhinfoProgHeader;
        int phnum = 0;

        noteProgHeader = d->pphdr + phnum;

        if (cdh_archive_stream_read(&d->archive, noteProgHeader, sizeof(Elf64_Phdr)) != CDM_STATUS_OK) {
            cdhlog(LOG_ERR, "We have failed to read program header %Xh !", phnum);
            return CDM_STATUS_ERROR;
        } else {
            Elf64_Phdr newNoteProgHeader;

            memcpy(&newNoteProgHeader, noteProgHeader, sizeof(Elf64_Phdr));

            newNoteProgHeader.p_offset += sizeof(Elf64_Phdr);

            if (cdh_archive_write(&d->archive, &newNoteProgHeader, sizeof(Elf64_Phdr)) < 0) {
                cdhlog(LOG_ERR, "We have failed to write program header %Xh !", phnum);
                return CDM_STATUS_ERROR;
            }
        }

        for (phnum = 1; phnum < d->ehdr.e_phnum; phnum++) {
            Elf64_Phdr *const pProgHeader = d->pphdr + phnum;

            /* Read Programm header */
            if (cdh_archive_stream_read(&d->archive, pProgHeader, sizeof(Elf64_Phdr)) != CDM_STATUS_OK) {
                cdhlog(LOG_ERR, "We have failed to read program header %Xh !", phnum);
                return CDM_STATUS_ERROR;
            } else {
                if (cdh_archive_write(&d->archive, pProgHeader, sizeof(Elf64_Phdr)) < 0) {
                    cdhlog(LOG_ERR, "We have failed to write program header %Xh !", phnum);
                    return CDM_STATUS_ERROR;
                }
            }
        }

        lastProgHeader = d->pphdr + (phnum - 1);

        memcpy(&cdhinfoProgHeader, noteProgHeader, sizeof(Elf64_Phdr));

        cdhinfoProgHeader.p_offset = lastProgHeader->p_offset + lastProgHeader->p_filesz;
        cdhinfoProgHeader.p_filesz = cdh_info_elf_note_size(d->info, true);

        cdhlog(LOG_INFO, "CDH Info note offset=0x%08x filesz=0x%08x", cdhinfoProgHeader.p_offset, cdhinfoProgHeader.p_filesz);
       
        if (cdh_archive_write(&d->archive, &cdhinfoProgHeader, sizeof(Elf64_Phdr)) < 0) {
            cdhlog(LOG_ERR, "We have failed to write program header %Xh !", phnum);
            return CDM_STATUS_ERROR;
        }
    }

    return CDM_STATUS_OK;
}

static CdmStatus get_coredump_registers(CdhData *d)
{
    CdmStatus found = CDM_STATUS_ERROR;
    size_t offset = 0;

    assert(d);

    while (found != CDM_STATUS_OK && offset < d->note_page_size) {
        Elf64_Nhdr *pnote = (Elf64_Nhdr *)(d->nhdr + offset);

        if (pnote->n_type == NT_PRSTATUS) {
            struct user_regs_struct *ptr_reg;
            prstatus_t *prstatus;

            prstatus =
                (prstatus_t *)((char *)pnote + sizeof(Elf64_Nhdr) + ALIGN(pnote->n_namesz, 4));
            ptr_reg = (struct user_regs_struct *)prstatus->pr_reg;

#ifdef __x86_64__
            d->regs.rip = ptr_reg->rip; /* REG_IP_REGISTER */
            d->regs.rbp = ptr_reg->rbp; /* REG_FRAME_REGISTER */
#elif __aarch64__
            d->regs.pc = ptr_reg->pc;       /* REG_PROG_COUNTER */
            d->regs.lr = ptr_reg->regs[30]; /* REG_LINK_REGISTER */
#endif

            found = CDM_STATUS_OK;
        }

        offset += NOTE_SIZE(*pnote);
    }

    return found;
}

static int get_virtual_memory_phdr_nr(CdhData *d, Elf64_Addr address)
{
    assert(d);

    for (size_t i = 0; i < d->ehdr.e_phnum; i++) {
        if ((address >= d->pphdr[i].p_vaddr) &&
            (address < d->pphdr[i].p_vaddr + d->pphdr[i].p_memsz)) {
            return (int)i;
        }
    }

    return -1;
}

static const char *get_nt_file_region_name(char *string_tab_start, unsigned nr)
{
    char *pos = string_tab_start;

    for (size_t i = 0; i < nr; i++) {
        pos += strlen(pos) + 1;
    }

    return pos;
}

static CdmStatus get_nt_file_region(CdhData *d, Elf64_Addr address, Elf64_Addr *region_start,
                                       Elf64_Addr *region_end, Elf64_Off *file_start,
                                       const char **region_name)
{
    size_t offset = 0;
    char *regions;

    assert(d);
    assert(region_start);
    assert(region_end);
    assert(file_start);
    assert(region_name);

    while (offset < d->note_page_size) {
        Elf64_Nhdr *pnote = (Elf64_Nhdr *)(d->nhdr + offset);

        if (pnote->n_type == NT_FILE) {
            unsigned int region_nr = 0;
            char *ntpos, *note_end;
            Elf64_Off num_regions, page_size;

            ntpos = ((char *)pnote + sizeof(Elf64_Nhdr) + ALIGN(pnote->n_namesz, 4));
            note_end = (char *)pnote + NOTE_SIZE(*pnote);

            num_regions = *((Elf64_Off *)ntpos);
            ntpos += sizeof(Elf64_Off);
            page_size = *((Elf64_Off *)ntpos);
            d->elf_vma_page_size = page_size;
            ntpos += sizeof(Elf64_Off);
            regions =
                ntpos + num_regions * (sizeof(Elf64_Addr) + sizeof(Elf64_Addr) + sizeof(Elf64_Off));

            while ((ntpos < note_end) && (region_nr < num_regions)) {
                Elf64_Addr reg_start, reg_end;
                Elf64_Off reg_fileoff;

                reg_start = *((Elf64_Addr *)ntpos);
                ntpos += sizeof(Elf64_Addr);
                reg_end = *((Elf64_Addr *)ntpos);
                ntpos += sizeof(Elf64_Addr);
                reg_fileoff = *((Elf64_Off *)ntpos);
                ntpos += sizeof(Elf64_Off);

                if ((address >= reg_start) && (address < reg_end)) {
                    *region_start = reg_start;
                    *region_end = reg_end;
                    *file_start = reg_fileoff;
                    *region_name = get_nt_file_region_name(regions, region_nr);

                    return CDH_FOUND;
                }

                region_nr++;
            }
        }

        offset += NOTE_SIZE(*pnote);
    }

    return CDH_NOTFOUND;
}

static Elf64_Addr read_virtual_memory(CdhData *d, Elf64_Addr address, int phdr_nr)
{
    Elf64_Addr read_data;
    Elf64_Off pos;

    assert(d);

    pos = d->pphdr[phdr_nr].p_offset + (address - d->pphdr[phdr_nr].p_vaddr);

    if (cdh_archive_stream_move_to_offset(&d->archive, pos) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "cdh_archive_stream_move_to_offset has failed to seek");
    }

    if (cdh_archive_stream_read(&d->archive, &read_data, sizeof(Elf64_Addr)) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "cdh_archive_stream_read has failed to read %d bytes!", sizeof(Elf64_Addr));
    } else {
        if (cdh_archive_write(&d->archive, &read_data, sizeof(Elf64_Addr)) < 0) {
            cdhlog(LOG_ERR, "cdh_archive_write has failed to read %d bytes!", sizeof(Elf64_Addr));
        }
    }

    return read_data;
}

static int get_note_page_index(CdhData *d)
{
    int i = 0;

    assert(d);

    /* Search PT_NOTE section */
    for (i = 0; i < d->ehdr.e_phnum; i++) {
        cdhlog(LOG_DEBUG, "Note section prog_note:%d type:0x%X offset:0x%zX size:0x%zX (%zdbytes)",
               i, d->pphdr[i].p_type, d->pphdr[i].p_offset, d->pphdr[i].p_filesz,
               d->pphdr[i].p_filesz);

        if (d->pphdr[i].p_type == PT_NOTE) {
            break;
        }
    }

    return i == d->ehdr.e_phnum ? CDM_STATUS_ERROR : i;
}

static CdmStatus read_notes(CdhData *d)
{
    Elf64_Phdr dummyPhdr;
    int prog_note;

    assert(d);

    prog_note = get_note_page_index(d);

    /* note page not found, abort */
    if (prog_note < 0) {
        cdhlog(LOG_ERR, "Cannot find note header page index");
        return CDM_STATUS_ERROR;
    }

    /* Move to NOTE header position */
    if (cdh_archive_stream_move_to_offset(&d->archive, d->pphdr[prog_note].p_offset) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "Cannot move to note header");
        return CDM_STATUS_ERROR;
    }

    if ((d->nhdr = (char *)malloc(d->pphdr[prog_note].p_filesz)) == NULL) {
        cdhlog(LOG_ERR, "Cannot allocate Nhdr memory (note size %zd bytes)",
               d->pphdr[prog_note].p_filesz);
        return CDM_STATUS_ERROR;
    }

    if (cdh_archive_stream_read(&d->archive, d->nhdr, d->pphdr[prog_note].p_filesz) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "Cannot read note header");
        return CDM_STATUS_ERROR;
    } else {
        if (cdh_archive_write(&d->archive, d->nhdr, d->pphdr[prog_note].p_filesz) < 0) {
            cdhlog(LOG_ERR, "Cannot write note header");
            return CDM_STATUS_ERROR;
        }
    }

    /* To preserve next program section alignment we do a dummy read without write for extra note
     * header added */
    if (cdh_archive_stream_read(&d->archive, &dummyPhdr, sizeof(Elf64_Phdr)) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "Cannot read dummy note header pending null bytes");
        return CDM_STATUS_ERROR;
    }

    d->note_page_size = d->pphdr[prog_note].p_filesz;

    return CDM_STATUS_OK;
}

static CdmStatus init_coredump(CdhData *d)
{
    assert(d);

    if (cdh_archive_stream_open(&d->archive, NULL) == CDM_STATUS_OK) {
        cdhlog(LOG_INFO, "Coredump compression started for %s with pid %u", d->info->name,
               d->info->pid);
    } else {
        cdhlog(LOG_ERR, "init_coredump: cdh_stream_init has failed !");
        return CDM_STATUS_ERROR;
    }

    return CDM_STATUS_OK;
}

static CdmStatus close_coredump(CdhData *d)
{
    assert(d);

    if (cdh_archive_close(&d->archive) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "Close_coredump: cdh_stream_close has failed !");
        return CDM_STATUS_ERROR;
    }

    return CDM_STATUS_OK;
}

CdmStatus cdh_coredump_generate(CdhData *d)
{
    Elf64_Addr region_start;
    Elf64_Addr region_end;
    Elf64_Off region_file_offset;
    CdmStatus ret = CDM_STATUS_OK;
    CdmStatus region_found;
    const char *region_name;
    int phdr;

    assert(d);

    /* open src and dest files, allocate read buffer */
    if (init_coredump(d) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "Cannot init coredump system");
        ret = CDM_STATUS_ERROR;
        goto finished;
    }

    if (read_elf_headers(d) == CDM_STATUS_OK) {
        Elf64_Addr return_addr_add = 0x8;

        if (read_notes(d) != CDM_STATUS_OK) {
            cdhlog(LOG_ERR, "cannot read NOTES");
            ret = CDM_STATUS_ERROR;
            goto finished;
        }

        if (get_coredump_registers(d) != CDM_STATUS_OK) {
            cdhlog(LOG_ERR, "regs not found in notes");
            ret = CDM_STATUS_ERROR;
            goto finished;
        }

#ifdef __x86_64__
        phdr = get_virtual_memory_phdr_nr(d, d->regs.rbp + return_addr_add);
#elif __aarch64__
        phdr = get_virtual_memory_phdr_nr(d, d->regs.lr);
#endif
        if (phdr == -1) {
            cdhlog(LOG_WARNING, "Return address + %zd memory location not found in program header",
                   return_addr_add);
        } else {
#ifdef __x86_64__
            d->ra = read_virtual_memory(d, d->regs.rbp + return_addr_add, phdr);
#elif __aarch64__
            d->ra = d->regs.lr; /* The link register holds our return address */
#endif
            d->crashid_info |= CID_RETURN_ADDRESS;

            /* Get the ELF file offset of the return address */
            region_found = get_nt_file_region(d, d->ra, &region_start, &region_end,
                                              &region_file_offset, &region_name);

            if (region_found == CDH_NOTFOUND) {
                cdhlog(LOG_INFO, "Could not get NT_FILE region of the return address");
            } else {
                d->ra_file_offset =
                    d->ra - region_start + (region_file_offset * d->elf_vma_page_size);
                d->ra_module_name = region_name;
                d->crashid_info |= CID_RA_FILE_OFFSET;
            }
        }

        /* Get the ELF file offset of the ip/pc */
#ifdef __x86_64__
        region_found = get_nt_file_region(d, d->regs.rip, &region_start, &region_end,
                                          &region_file_offset, &region_name);
#elif __aarch64__
        region_found = get_nt_file_region(d, d->regs.pc, &region_start, &region_end,
                                          &region_file_offset, &region_name);
#endif

        if (region_found == CDH_NOTFOUND) {
            cdhlog(LOG_INFO, "Could not get the NT_FILE region of the instruction pointer");
        } else {
#ifdef __x86_64__
            d->ip_file_offset =
                d->regs.rip - region_start + (region_file_offset * d->elf_vma_page_size);
#elif __aarch64__
            d->ip_file_offset =
                d->regs.pc - region_start + (region_file_offset * d->elf_vma_page_size);
#endif
            d->ip_module_name = region_name;
            d->crashid_info |= CID_IP_FILE_OFFSET;
        }

        /* We have all data to generate the crash ids */
        if (cdh_crashid_process(d) != CDM_STATUS_OK) {
            cdhlog(LOG_ERR, "Cannot generate the crash ids");
            ret = CDM_STATUS_ERROR;
            goto finished;
        }

#if defined(WITH_COREMANAGER) || defined(WITH_CRASHHANDLER)
        if (cdh_manager_connected(&d->crash_mgr)) {
            cdh_msg_t msg;
            cdh_coreupdate_t data;

            cdh_msg_init(&msg, CDH_CORE_UPDATE, (uint16_t)((unsigned)d->info->pid | d->info->tstamp));

            memcpy(data.crashid, d->info->crashid, strlen(d->info->crashid) + 1);
            memcpy(data.vectorid, d->info->vectorid, strlen(d->info->vectorid) + 1);
            memcpy(data.contextid, d->info->contextid, strlen(d->info->contextid) + 1);

            cdh_msg_set_data(&msg, &data, sizeof(data));

            if (cdh_manager_send(&d->crash_mgr, &msg) == CDM_STATUS_ERROR) {
                cdhlog(LOG_ERR, "Failed to send update message to manager");
            }
        }
#endif
    } else {
        cdhlog(LOG_ERR, "Cannot read headers");
        ret = CDM_STATUS_ERROR;
        goto finished;
    }

finished:
    if (ret == CDM_STATUS_ERROR) {
        cdhlog(LOG_ERR, "Errors in preprocessing coredump stream");
    }

    /* In all cases, we try to finish to read/compress the coredump until the end */
    if (cdh_archive_stream_read_all(&d->archive) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "Cannot finish coredump compression");
        ret = CDM_STATUS_ERROR;
    } else {
        if (cdh_info_write_as_elf_note(d->info, &d->archive, write_callback) != CDM_STATUS_OK) {
            cdhlog(LOG_ERR, "Fail to append cdhinfo note header");
        }

        cdhlog(LOG_INFO, "Coredump compression finished for %s with pid %u", d->info->name,
               d->info->pid);
    }

    /* In all cases, let's close the files */
    if (close_coredump(d) != CDM_STATUS_OK) {
        cdhlog(LOG_ERR, "Cannot close coredump system");
        ret = CDM_STATUS_ERROR;
    }

    free(d->nhdr);
    free(d->pphdr);

    return ret;
}
