/*
 * SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2019, BMW Car IT GmbH
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * For further information see http://www.genivi.org/.
 *
 * \author Alin Popa <alin.popa@bmw.de>
 * \file cdh-coredump.c
 */

#include "cdh-coredump.h"
#include "cdm-defaults.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/procfs.h>
#include <sys/user.h>
#include <unistd.h>

#define ALIGN(x, a) (((x) + (a) - 1UL) & ~((a) - 1UL))
#define NOTE_SIZE(_hdr) (sizeof(_hdr) + ALIGN ((_hdr).n_namesz, 4) + (_hdr).n_descsz)

static gint get_virtual_memory_phdr_nr (CdhCoredump *cd, Elf64_Addr address);

static const gchar *get_nt_file_region_name (gchar *string_tab_start, gulong nr);

static gint get_nt_file_region (CdhCoredump *cd,
                                Elf64_Addr address,
                                Elf64_Addr *region_start,
                                Elf64_Addr *region_end,
                                Elf64_Off *file_start,
                                const gchar **region_name);

static Elf64_Addr read_virtual_memory (CdhCoredump *cd, Elf64_Addr address, gint phdr_nr);

static gint get_note_page_index (CdhCoredump *cd);

static CdmStatus read_notes (CdhCoredump *cd);

static CdmStatus init_coredump (CdhCoredump *cd);

static CdmStatus read_elf_headers (CdhCoredump *cd);

static CdmStatus get_coredump_registers (CdhCoredump *cd);

CdhCoredump *
cdh_coredump_new (CdhContext *context,
                  CdhArchive *archive)
{
  CdhCoredump *cd = g_new0 (CdhCoredump, 1);

  g_assert (context);
  g_assert (archive);

  g_ref_count_init (&cd->rc);

  cd->context = cdh_context_ref (context);
  cd->archive = cdh_archive_ref (archive);

  return cd;
}

CdhCoredump *
cdh_coredump_ref (CdhCoredump *cd)
{
  g_assert (cd);
  g_ref_count_inc (&cd->rc);
  return cd;
}

void
cdh_coredump_unref (CdhCoredump *cd)
{
  g_assert (cd);

  if (g_ref_count_dec (&cd->rc) == TRUE)
    {
      cdh_context_unref (cd->context);
      cdh_archive_unref (cd->archive);
#if defined(WITH_CRASHMANAGER)
      if (cd->manager != NULL)
        cdh_manager_unref (cd->manager);
#endif
      g_free (cd);
    }
}

#if defined(WITH_CRASHMANAGER)
void
cdh_coredump_set_manager (CdhCoredump *cd,
                          CdhManager *manager)
{
  g_assert (cd);
  g_assert (manager);

  cd->manager = cdh_manager_ref (manager);
}
#endif

static CdmStatus
read_elf_headers (CdhCoredump *cd)
{
  gint phnum = 0;

  g_assert (cd);

  /* Read ELF header */
  if (cdh_archive_stream_read (cd->archive, &cd->context->ehdr, sizeof(cd->context->ehdr))
      != CDM_STATUS_OK)
    {
      g_warning ("We have failed to read the ELF header !");
      return CDM_STATUS_ERROR;
    }

  /* Read until PROG position */
  if (cdh_archive_stream_move_to_offset (cd->archive, cd->context->ehdr.e_phoff)
      != CDM_STATUS_OK)
    {
      g_warning ("We have failed to seek to the beginning of the "
                 "segment headers !");
      return CDM_STATUS_ERROR;
    }

  /* Read and store all program headers */
  cd->context->pphdr = (Elf64_Phdr*)malloc (sizeof(Elf64_Phdr) * cd->context->ehdr.e_phnum);
  if (cd->context->pphdr == NULL)
    {
      g_warning ("Cannot allocate Phdr memory (%d headers)", cd->context->ehdr.e_phnum);
      return CDM_STATUS_ERROR;
    }

  for (phnum = 0; phnum < cd->context->ehdr.e_phnum; phnum++)
    {
      Elf64_Phdr *const pSegHeader = cd->context->pphdr + phnum;

      /* Read Programm header */
      if (cdh_archive_stream_read (cd->archive, pSegHeader, sizeof(Elf64_Phdr)) != CDM_STATUS_OK)
        {
          g_warning ("We have failed to read segment header %d !", phnum);
          return CDM_STATUS_ERROR;
        }
    }

  return CDM_STATUS_OK;
}

static CdmStatus
get_coredump_registers (CdhCoredump *cd)
{
  CdmStatus found = CDM_STATUS_ERROR;
  gsize offset = 0;

  g_assert (cd);

  while (found != CDM_STATUS_OK && offset < cd->context->note_page_size)
    {
      Elf64_Nhdr *pnote = (Elf64_Nhdr*)(cd->context->nhdr + offset);

      if (pnote->n_type == NT_PRSTATUS)
        {
          struct user_regs_struct *ptr_reg;
          prstatus_t *prstatus;

          prstatus = (prstatus_t*)((gchar*)pnote + sizeof(Elf64_Nhdr)
                                   + ALIGN (pnote->n_namesz, 4));
          ptr_reg = (struct user_regs_struct *)prstatus->pr_reg;

#ifdef __x86_64__
          cd->context->regs.rip = ptr_reg->rip;             /* REG_IP_REGISTER */
          cd->context->regs.rbp = ptr_reg->rbp;             /* REG_FRAME_REGISTER */
#elif __aarch64__
          cd->context->regs.pc = ptr_reg->pc;               /* REG_PROG_COUNTER */
          cd->context->regs.lr = ptr_reg->context->regs[30];/* REG_LINK_REGISTER */
#endif

          found = CDM_STATUS_OK;
        }

      offset += NOTE_SIZE (*pnote);
    }

  return found;
}

static gint
get_virtual_memory_phdr_nr (CdhCoredump *cd,
                            Elf64_Addr address)
{
  g_assert (cd);

  for (gsize i = 0; i < cd->context->ehdr.e_phnum; i++)
    {
      if ((address >= cd->context->pphdr[i].p_vaddr)
          && (address < cd->context->pphdr[i].p_vaddr + cd->context->pphdr[i].p_memsz))
        {
          return (gint)i;
        }
    }

  return -1;
}

static const gchar *
get_nt_file_region_name (gchar *string_tab_start,
                         gulong nr)
{
  gchar *pos = string_tab_start;

  for (gsize i = 0; i < nr; i++)
    pos += strlen (pos) + 1;

  return pos;
}

static CdmStatus
get_nt_file_region (CdhCoredump *cd,
                    Elf64_Addr address,
                    Elf64_Addr *region_start,
                    Elf64_Addr *region_end,
                    Elf64_Off *file_start,
                    const gchar **region_name)
{
  gsize offset = 0;
  gchar *regions;

  g_assert (cd);
  g_assert (region_start);
  g_assert (region_end);
  g_assert (file_start);
  g_assert (region_name);

  while (offset < cd->context->note_page_size)
    {
      Elf64_Nhdr *pnote = (Elf64_Nhdr*)(cd->context->nhdr + offset);

      if (pnote->n_type == NT_FILE)
        {
          guint region_nr = 0;
          gchar *ntpos, *note_end;
          Elf64_Off num_regions, page_size;

          ntpos = ((gchar *)pnote + sizeof(Elf64_Nhdr) + ALIGN (pnote->n_namesz, 4));
          note_end = (void *)pnote + NOTE_SIZE (*pnote);

          num_regions = *((Elf64_Off*)ntpos);
          ntpos += sizeof(Elf64_Off);
          page_size = *((Elf64_Off*)ntpos);
          cd->context->elf_vma_page_size = page_size;
          ntpos += sizeof(Elf64_Off);
          regions = ntpos + num_regions * (sizeof(Elf64_Addr) + sizeof(Elf64_Addr) + sizeof(Elf64_Off));

          while ((ntpos < note_end) && (region_nr < num_regions))
            {
              Elf64_Addr reg_start, reg_end;
              Elf64_Off reg_fileoff;

              reg_start = *((Elf64_Addr*)ntpos);
              ntpos += sizeof(Elf64_Addr);
              reg_end = *((Elf64_Addr*)ntpos);
              ntpos += sizeof(Elf64_Addr);
              reg_fileoff = *((Elf64_Off*)ntpos);
              ntpos += sizeof(Elf64_Off);

              if ((address >= reg_start) && (address < reg_end))
                {
                  *region_start = reg_start;
                  *region_end = reg_end;
                  *file_start = reg_fileoff;
                  *region_name = get_nt_file_region_name (regions, region_nr);

                  return CDM_STATUS_OK;
                }

              region_nr++;
            }
        }

      offset += NOTE_SIZE (*pnote);
    }

  return CDM_STATUS_ERROR;
}

static Elf64_Addr
read_virtual_memory (CdhCoredump *cd,
                     Elf64_Addr address,
                     gint phdr_nr)
{
  Elf64_Addr read_data;
  Elf64_Off pos;

  g_assert (cd);

  pos = cd->context->pphdr[phdr_nr].p_offset + (address - cd->context->pphdr[phdr_nr].p_vaddr);

  if (cdh_archive_stream_move_to_offset (cd->archive, pos) != CDM_STATUS_OK)
    g_warning ("cdh_archive_stream_move_to_offset has failed to seek");

  if (cdh_archive_stream_read (cd->archive, &read_data, sizeof(Elf64_Addr)) != CDM_STATUS_OK)
    g_warning ("cdh_archive_stream_read has failed to read %lu bytes!", sizeof(Elf64_Addr));

  return read_data;
}

static gint
get_note_page_index (CdhCoredump *cd)
{
  gint i = 0;

  g_assert (cd);

  /* Search PT_NOTE section */
  for (i = 0; i < cd->context->ehdr.e_phnum; i++)
    {
      g_debug ("Note section prog_note:%d type:0x%X offset:0x%zX size:0x%zX (%lu bytes)",
               i,
               cd->context->pphdr[i].p_type,
               cd->context->pphdr[i].p_offset,
               cd->context->pphdr[i].p_filesz,
               cd->context->pphdr[i].p_filesz);

      if (cd->context->pphdr[i].p_type == PT_NOTE)
        break;
    }

  return i == cd->context->ehdr.e_phnum ? CDM_STATUS_ERROR : i;
}

static CdmStatus
read_notes (CdhCoredump *cd)
{
  gint prog_note;

  g_assert (cd);

  prog_note = get_note_page_index (cd);
  cd->context->nhdr = NULL;

  /* note page not found, abort */
  if (prog_note < 0)
    {
      g_warning ("Cannot find note header page index");
      return CDM_STATUS_ERROR;
    }

  /* Move to NOTE header position */
  if (cdh_archive_stream_move_to_offset (cd->archive, cd->context->pphdr[prog_note].p_offset)
      != CDM_STATUS_OK)
    {
      g_warning ("Cannot move to note header");
      return CDM_STATUS_ERROR;
    }

  if ((cd->context->nhdr = (gchar*)malloc (cd->context->pphdr[prog_note].p_filesz)) == NULL)
    {
      g_warning ("Cannot allocate Nhdr memory (note size %lu bytes)",
                 cd->context->pphdr[prog_note].p_filesz);
      return CDM_STATUS_ERROR;
    }

  if (cdh_archive_stream_read (cd->archive, cd->context->nhdr, cd->context->pphdr[prog_note].p_filesz)
      != CDM_STATUS_OK)
    {
      g_warning ("Cannot read note header");
      return CDM_STATUS_ERROR;
    }

  cd->context->note_page_size = cd->context->pphdr[prog_note].p_filesz;

  return CDM_STATUS_OK;
}

static CdmStatus
init_coredump (CdhCoredump *cd)
{
  g_autofree gchar *dst = NULL;

  g_assert (cd);

  dst = g_strdup_printf ("core.%s.%ld", cd->context->name, cd->context->pid);

  if (cdh_archive_stream_open (cd->archive, 0, (dst != NULL ? dst : "coredump"), CDM_CRASHDUMP_SPLIT_SIZE)
      == CDM_STATUS_OK)
    {
      g_info ("Coredump compression started for %s with pid %ld", cd->context->name, cd->context->pid);
    }
  else
    {
      g_warning ("init_coredump: cdh_stream_init has failed !");
      return CDM_STATUS_ERROR;
    }

  return CDM_STATUS_OK;
}

CdmStatus
cdh_coredump_generate (CdhCoredump *cd)
{
  Elf64_Addr region_start;
  Elf64_Addr region_end;
  Elf64_Off region_file_offset;
  CdmStatus ret = CDM_STATUS_OK;
  CdmStatus region_found;
  const gchar *region_name;
  gint phdr;

  g_assert (cd);

  /* open src and dest files, allocate read buffer */
  if (init_coredump (cd) != CDM_STATUS_OK)
    {
      g_warning ("Cannot init coredump system");
      ret = CDM_STATUS_ERROR;
      goto finished;
    }

  if (read_elf_headers (cd) == CDM_STATUS_OK)
    {
      Elf64_Addr return_addr_add = 0x8;

      if (read_notes (cd) != CDM_STATUS_OK)
        {
          g_warning ("cannot read NOTES");
          ret = CDM_STATUS_ERROR;
          goto finished;
        }

      if (get_coredump_registers (cd) != CDM_STATUS_OK)
        {
          g_warning ("regs not found in notes");
          ret = CDM_STATUS_ERROR;
          goto finished;
        }

#ifdef __x86_64__
      phdr = get_virtual_memory_phdr_nr (cd, cd->context->regs.rbp + return_addr_add);
#elif __aarch64__
      phdr = get_virtual_memory_phdr_nr (cd, cd->context->regs.lr);
#endif
      if (phdr == -1)
        {
          g_info ("Return address + %lu memory location not found in program header",
                  return_addr_add);
        }
      else
        {
#ifdef __x86_64__
          cd->context->ra = read_virtual_memory (cd,
                                                 cd->context->regs.rbp + return_addr_add, phdr);
#elif __aarch64__
          /* The link register holds our return address */
          cd->context->ra = cd->context->regs.lr;
#endif
          cd->context->crashid_info |= CID_RETURN_ADDRESS;

          /* Get the ELF file offset of the return address */
          region_found = get_nt_file_region (cd,
                                             cd->context->ra,
                                             &region_start,
                                             &region_end,
                                             &region_file_offset,
                                             &region_name);

          if (region_found == CDM_STATUS_ERROR)
            {
              g_info ("Could not get NT_FILE region of the return address");
            }
          else
            {
              cd->context->ra_file_offset = cd->context->ra - region_start
                                            + (region_file_offset * cd->context->elf_vma_page_size);
              cd->context->ra_module_name = region_name;
              cd->context->crashid_info |= CID_RA_FILE_OFFSET;
            }
        }

      /* Get the ELF file offset of the ip/pc */
#ifdef __x86_64__
      region_found = get_nt_file_region (cd,
                                         cd->context->regs.rip,
                                         &region_start,
                                         &region_end,
                                         &region_file_offset,
                                         &region_name);
#elif __aarch64__
      region_found = get_nt_file_region (cd,
                                         cd->context->regs.pc,
                                         &region_start,
                                         &region_end,
                                         &region_file_offset,
                                         &region_name);
#endif

      if (region_found == CDM_STATUS_ERROR)
        {
          g_info ("Could not get the NT_FILE region of the instruction pointer");
        }
      else
        {
#ifdef __x86_64__
          cd->context->ip_file_offset = cd->context->regs.rip - region_start
                                        + (region_file_offset * cd->context->elf_vma_page_size);
#elif __aarch64__
          cd->context->ip_file_offset = cd->context->regs.pc - region_start
                                        + (region_file_offset * cd->context->elf_vma_page_size);
#endif
          cd->context->ip_module_name = region_name;
          cd->context->crashid_info |= CID_IP_FILE_OFFSET;
        }

      /* We have all data to generate the crash ids */
      if (cdh_context_crashid_process (cd->context) != CDM_STATUS_OK)
        {
          g_warning ("Cannot generate the crash ids");
          ret = CDM_STATUS_ERROR;
          goto finished;
        }

#if defined(WITH_CRASHMANAGER)
      if (cdh_manager_connected (cd->manager))
        {
          CdmMessage msg;
          CdmMessageDataUpdate data;

          cdm_message_init (&msg,
                            CDM_CORE_UPDATE,
                            (guint16)((gulong)cd->context->pid | cd->context->tstamp));

          memcpy (data.crashid, cd->context->crashid, strlen (cd->context->crashid) + 1);
          memcpy (data.vectorid, cd->context->vectorid, strlen (cd->context->vectorid) + 1);
          memcpy (data.contextid, cd->context->contextid, strlen (cd->context->contextid) + 1);

          cdm_message_set_data (&msg, &data, sizeof(data));

          if (cdh_manager_send (cd->manager, &msg) == CDM_STATUS_ERROR)
            g_warning ("Failed to send update message to manager");
          else
            cdh_context_read_context_info (cd->context);
        }
#endif
    }
  else
    {
      g_warning ("Cannot read headers");
      ret = CDM_STATUS_ERROR;
      goto finished;
    }

finished:
  if (ret == CDM_STATUS_ERROR)
    g_warning ("Errors in preprocessing coredump stream");

  /* In all cases, we try to finish to read/compress the coredump until the end */
  if (cdh_archive_stream_read_all (cd->archive) != CDM_STATUS_OK)
    {
      g_warning ("Cannot finish coredump compression");
      ret = CDM_STATUS_ERROR;
    }
  else
    {
      cd->context->cdsize = cdh_archive_stream_get_offset (cd->archive);
      g_info ("Coredump compression finished for %s with pid %ld cdsize %lu",
              cd->context->name,
              cd->context->pid,
              cd->context->cdsize);
    }

  /* In all cases, let's close the files */
  if (cdh_archive_stream_close (cd->archive) != CDM_STATUS_OK)
    {
      g_warning ("Close archive stream failed");
      ret = CDM_STATUS_ERROR;
    }

  return ret;
}
