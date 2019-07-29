/* cdh-coredump.c
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

static gint get_virtual_memory_phdr_nr(CdhData *d, ELF_Addr address);

static const gchar *get_nt_file_region_name(gchar *string_tab_start, gulong nr);

static gint get_nt_file_region(CdhData *d, ELF_Addr address, ELF_Addr *region_start,
							  ELF_Addr *region_end, ELF_Off *file_start, const gchar **region_name);

static ELF_Addr read_virtual_memory(CdhData *d, ELF_Addr address, gint phdr_nr);

static gint get_note_page_index(CdhData *d);

static CdmStatus read_notes(CdhData *d);

static CdmStatus init_coredump(CdhData *d);

static CdmStatus close_coredump(CdhData *d);

static CdmStatus read_elf_headers(CdhData *d);

static CdmStatus get_coredump_registers(CdhData *d);

static CdmStatus read_elf_headers(CdhData *d)
{
	gint phnum = 0;

	g_assert(d);

	/* Read ELF header */
	if (cdh_archive_stream_read(&d->archive, &d->ehdr, sizeof(d->ehdr)) != CDM_STATUS_OK) {
		g_warning("read_elf_headers: We have failed to read the ELF header !");
		return CDM_STATUS_ERROR;
	}

	/* Read until PROG position */
	if (cdh_archive_stream_move_to_offset(&d->archive, d->ehdr.e_phoff) != CDM_STATUS_OK) {
		g_warning("cdh_stream_move_to_offset: We have failed to seek to the beginning of the "
						"segment headers !");
		return CDM_STATUS_ERROR;
	}

	/* Read and store all program headers */
	d->pphdr = (ELF_Phdr *)malloc(sizeof(ELF_Phdr) * d->ehdr.e_phnum);
	if (d->pphdr == NULL) {
		g_warning("Cannot allocate Phdr memory (%d headers)", d->ehdr.e_phnum);
		return CDM_STATUS_ERROR;
	}

	for (phnum = 0; phnum < d->ehdr.e_phnum; phnum++) {
		ELF_Phdr *const pSegHeader = d->pphdr + phnum;

		/* Read Programm header */
		if (cdh_archive_stream_read(&d->archive, pSegHeader, sizeof(ELF_Phdr)) != CDM_STATUS_OK) {
			g_warning("We have failed to read segment header %Xh !", phnum);
			return CDM_STATUS_ERROR;
		}
	}

	return CDM_STATUS_OK;
}

static CdmStatus get_coredump_registers(CdhData *d)
{
	CdmStatus found = CDM_STATUS_ERROR;
	gsize offset = 0;

	g_assert(d);

	while (found != CDM_STATUS_OK && offset < d->note_page_size) {
		ELF_Nhdr *pnote = (ELF_Nhdr *)(d->nhdr + offset);

		if (pnote->n_type == NT_PRSTATUS) {
			struct user_regs_struct *ptr_reg;
			prstatus_t *prstatus;

			prstatus = (prstatus_t *)((gchar *)pnote + sizeof(ELF_Nhdr) + ALIGN(pnote->n_namesz, 4));
			ptr_reg = (struct user_regs_struct *)prstatus->pr_reg;

#ifdef __x86_64__
			d->regs.rip = ptr_reg->rip; /* REG_IP_REGISTER */
			d->regs.rbp = ptr_reg->rbp; /* REG_FRAME_REGISTER */
#elif __aarch64__
			d->regs.pc = ptr_reg->pc;		/* REG_PROG_COUNTER */
			d->regs.lr = ptr_reg->regs[30]; /* REG_LINK_REGISTER */
#endif

			found = CDM_STATUS_OK;
		}

		offset += NOTE_SIZE(*pnote);
	}

	return found;
}

static gint get_virtual_memory_phdr_nr(CdhData *d, ELF_Addr address)
{
	g_assert(d);

	for (gsize i = 0; i < d->ehdr.e_phnum; i++) {
		if ((address >= d->pphdr[i].p_vaddr) &&
			(address < d->pphdr[i].p_vaddr + d->pphdr[i].p_memsz)) {
			return (gint)i;
		}
	}

	return -1;
}

static const gchar *get_nt_file_region_name(gchar *string_tab_start, gulong nr)
{
	gchar *pos = string_tab_start;

	for (gsize i = 0; i < nr; i++) {
		pos += strlen(pos) + 1;
	}

	return pos;
}

static CdmStatus get_nt_file_region(CdhData *d, ELF_Addr address, ELF_Addr *region_start,
									   ELF_Addr *region_end, ELF_Off *file_start,
									   const gchar **region_name)
{
	gsize offset = 0;
	gchar *regions;

	g_assert(d);
	g_assert(region_start);
	g_assert(region_end);
	g_assert(file_start);
	g_assert(region_name);

	while (offset < d->note_page_size) {
		ELF_Nhdr *pnote = (ELF_Nhdr *)(d->nhdr + offset);

		if (pnote->n_type == NT_FILE) {
			gulong gint region_nr = 0;
			gchar *ntpos, *note_end;
			ELF_Off num_regions, page_size;

			ntpos = ((gchar *)pnote + sizeof(ELF_Nhdr) + ALIGN(pnote->n_namesz, 4));
			note_end = (gchar *)pnote + NOTE_SIZE(*pnote);

			num_regions = *((ELF_Off *)ntpos);
			ntpos += sizeof(ELF_Off);
			page_size = *((ELF_Off *)ntpos);
			d->elf_vma_page_size = page_size;
			ntpos += sizeof(ELF_Off);
			regions = ntpos + num_regions * (sizeof(ELF_Addr) + sizeof(ELF_Addr) + sizeof(ELF_Off));

			while ((ntpos < note_end) && (region_nr < num_regions)) {
				ELF_Addr reg_start, reg_end;
				ELF_Off reg_fileoff;

				reg_start = *((ELF_Addr *)ntpos);
				ntpos += sizeof(ELF_Addr);
				reg_end = *((ELF_Addr *)ntpos);
				ntpos += sizeof(ELF_Addr);
				reg_fileoff = *((ELF_Off *)ntpos);
				ntpos += sizeof(ELF_Off);

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

static ELF_Addr read_virtual_memory(CdhData *d, ELF_Addr address, gint phdr_nr)
{
	ELF_Addr read_data;
	ELF_Off pos;

	g_assert(d);

	pos = d->pphdr[phdr_nr].p_offset + (address - d->pphdr[phdr_nr].p_vaddr);

	if (cdh_archive_stream_move_to_offset(&d->archive, pos) != CDM_STATUS_OK) {
		g_warning("cdh_archive_stream_move_to_offset has failed to seek");
	}

	if (cdh_archive_stream_read(&d->archive, &read_data, sizeof(ELF_Addr)) != CDM_STATUS_OK) {
		g_warning("cdh_archive_stream_read has failed to read %d bytes!", sizeof(ELF_Addr));
	}

	return read_data;
}

static gint get_note_page_index(CdhData *d)
{
	gint i = 0;

	g_assert(d);

	/* Search PT_NOTE section */
	for (i = 0; i < d->ehdr.e_phnum; i++) {
		g_debug("Note section prog_note:%d type:0x%X offset:0x%zX size:0x%zX (%zdbytes)",
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
	gint prog_note;

	g_assert(d);

	prog_note = get_note_page_index(d);
	d->nhdr = NULL;

	/* note page not found, abort */
	if (prog_note < 0) {
		g_warning("Cannot find note header page index");
		return CDM_STATUS_ERROR;
	}

	/* Move to NOTE header position */
	if (cdh_archive_stream_move_to_offset(&d->archive, d->pphdr[prog_note].p_offset) != CDM_STATUS_OK) {
		g_warning("Cannot move to note header");
		return CDM_STATUS_ERROR;
	}

	if ((d->nhdr = (gchar *)malloc(d->pphdr[prog_note].p_filesz)) == NULL) {
		g_warning("Cannot allocate Nhdr memory (note size %zd bytes)",
			   d->pphdr[prog_note].p_filesz);
		return CDM_STATUS_ERROR;
	}

	if (cdh_archive_stream_read(&d->archive, d->nhdr, d->pphdr[prog_note].p_filesz) != CDM_STATUS_OK) {
		g_warning("Cannot read note header");
		return CDM_STATUS_ERROR;
	}

	d->note_page_size = d->pphdr[prog_note].p_filesz;

	return CDM_STATUS_OK;
}

static CdmStatus init_coredump(CdhData *d)
{
	sgsize sz;

	g_assert(d);

	if (d->coredump_ready) {
		gchar dst[CDH_PATH_MAX];

		sz = snprintf(dst, sizeof(dst), COREFILE_NAME_PATTERN, d->tstamp, d->name, d->pid);
		cdhbail(BAIL_PATH_MAX, ((0 < sz) && (sz < (gssize)sizeof(dst))), NULL);

		if (cdh_archive_stream_open(&d->archive, 0, dst) == CDM_STATUS_OK) {
			g_info("Coredump compression started for %s with pid %u", d->name, d->pid);
		} else {
			g_warning("init_coredump: cdh_stream_init has failed !");
			return CDM_STATUS_ERROR;
		}
	} else {
		if (cdh_archive_stream_open(&d->archive, 0, NULL) != CDM_STATUS_OK) {
			g_warning("init_coredump: cdh_stream_init has failed !");
			return CDM_STATUS_ERROR;
		}
	}

	return CDM_STATUS_OK;
}

static CdmStatus close_coredump(CdhData *d)
{
	g_assert(d);

	if (cdh_archive_stream_close(&d->archive) != CDM_STATUS_OK) {
		g_warning("Close_coredump: cdh_stream_close has failed !");
		return CDM_STATUS_ERROR;
	}

	return CDM_STATUS_OK;
}

CdmStatus cdh_coredump_generate(CdhData *d)
{
	ELF_Addr region_start;
	ELF_Addr region_end;
	ELF_Off region_file_offset;
	CdmStatus ret = CDM_STATUS_OK;
	CdmStatus region_found;
	const gchar *region_name;
	gint phdr;

	g_assert(d);

	/* open src and dest files, allocate read buffer */
	if (init_coredump(d) != CDM_STATUS_OK) {
		g_warning("Cannot init coredump system");
		ret = CDM_STATUS_ERROR;
		goto finished;
	}

	if (read_elf_headers(d) == CDM_STATUS_OK) {
		ELF_Addr return_addr_add = 0x8;

		if (read_notes(d) != CDM_STATUS_OK) {
			g_warning("cannot read NOTES");
			ret = CDM_STATUS_ERROR;
			goto finished;
		}

		if (get_coredump_registers(d) != CDM_STATUS_OK) {
			g_warning("regs not found in notes");
			ret = CDM_STATUS_ERROR;
			goto finished;
		}

#ifdef __x86_64__
		phdr = get_virtual_memory_phdr_nr(d, d->regs.rbp + return_addr_add);
#elif __aarch64__
		phdr = get_virtual_memory_phdr_nr(d, d->regs.lr);
#endif
		if (phdr == -1) {
			g_warning("Return address + %zd memory location not found in program header",
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
				g_info("Could not get NT_FILE region of the return address");
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
			g_info("Could not get the NT_FILE region of the instruction pointer");
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
			g_warning("Cannot generate the crash ids");
			ret = CDM_STATUS_ERROR;
			goto finished;
		}

#if defined(WITH_COREMANAGER)
		if (cdh_manager_connected(&d->crash_mgr)) {
			CdmMessage msg;
			CdmMessageDataUpdate data;

			cdm_message_init(&msg, CDM_CORE_UPDATE, (guint16)((gulong)d->pid | d->tstamp));

			memcpy(data.crashid, d->crashid, strlen(d->crashid) + 1);
			memcpy(data.vectorid, d->vectorid, strlen(d->vectorid) + 1);
			memcpy(data.contextid, d->contextid, strlen(d->contextid) + 1);

			cdm_message_set_data(&msg, &data, sizeof(data));

			if (cdh_manager_send(&d->crash_mgr, &msg) == CDM_STATUS_ERROR) {
				g_warning("Failed to send update message to manager");
			}
		}
#endif
	} else {
		g_warning("Cannot read headers");
		ret = CDM_STATUS_ERROR;
		goto finished;
	}

finished:
	if (ret == CDM_STATUS_ERROR) {
		g_warning("Errors in preprocessing coredump stream");
	}

	/* In all cases, we try to finish to read/compress the coredump until the end */
	if (cdh_archive_stream_read_all(&d->archive) != CDM_STATUS_OK) {
		g_warning("Cannot finish coredump compression");
		ret = CDM_STATUS_ERROR;
	} else {
		if (d->coredump_ready) {
			g_info("Coredump compression finished for %s with pid %u", d->name, d->pid);
		}
	}

	/* In all cases, let's close the files */
	if (close_coredump(d) != CDM_STATUS_OK) {
		g_warning("Cannot close coredump system");
		ret = CDM_STATUS_ERROR;
	}

	return ret;
}
