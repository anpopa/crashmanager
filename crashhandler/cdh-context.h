/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019-2020 Alin Popa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \file cdh-context.h
 */

#pragma once

#include "cdh-archive.h"
#include "cdm-options.h"
#if defined(WITH_CRASHMANAGER)
#include "cdh-manager.h"
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The context object
 */
typedef struct _CdhContext {
    CdmOptions *opts;    /**< Reference to objects (owned) */
    CdhArchive *archive; /**< Reference to archive (owned)  */
#if defined(WITH_CRASHMANAGER)
    CdhManager *manager; /**< Reference to manager (owned)  */
#endif
    grefcount rc; /**< Reference counter variable  */

    gchar *name;     /**< process name */
    gchar *tname;    /**< thread name  */
    gchar *pexe;     /**< process executable file path  */
    guint64 tstamp;  /**< crash timestamp */
    gint64 sig;      /**< signal id */
    gint64 pid;      /**< process id as seen on host */
    gint64 cpid;     /**< process id as seen on namespace */
    guint16 session; /**< session ID to use for message identification */

    gsize cdsize; /**< coredump size */

    gchar *contextid;       /**< namespace context for the crashed pid */
    gchar *context_name;    /**< context name for the crashed pid */
    gchar *lifecycle_state; /**< lifecycle state when crash */
    gchar *epilog;          /**< epilog data */
    gchar *crashid;         /**< crash id value */
    gchar *vectorid;        /**< crash course id value */
    gboolean onhost;        /**< true if the crash is in host context */

    CdmRegisters regs;           /**< cpu registers for crash id calculation */
    Elf64_Ehdr ehdr;             /**< coredump elf Ehdr structure */
    Elf64_Phdr *pphdr;           /**< coredump elf pPhdr pointer to structure */
    gchar *nhdr;                 /**< buffer with all NOTE pages */
    guint64 ra;                  /**< return address for top frame */
    guint64 ip_file_offset;      /**< ip file offset for top frame */
    guint64 ra_file_offset;      /**< return address file offset for top frame */
    const gchar *ip_module_name; /**< module name pointed by ip */
    const gchar *ra_module_name; /**< module name pointed by ra */
    gulong note_page_size;       /**< note section page size */
    gulong elf_vma_page_size;    /**< elf vma page size */
    guint8 crashid_info;         /**< information available for crashid */
} CdhContext;

/**
 * @brief Create a new CdhContext object
 * @return A pointer to the new object
 */
CdhContext *cdh_context_new(CdmOptions *opts, CdhArchive *archive);

/**
 * @brief Aquire CdhContext object
 * @param ctx Pointer to the CdhContext object
 * @return Pointer to the CdhContext object
 */
CdhContext *cdh_context_ref(CdhContext *ctx);

/**
 * @brief Release a CdhContext object
 * @param ctx Pointer to the objectto release
 */
void cdh_context_unref(CdhContext *ctx);

#if defined(WITH_CRASHMANAGER)
/**
 * @brief Set manager object
 * @param ctx Pointer to the CdhContext object
 * @param manager Pointer to the CdhManager object
 */
void cdh_context_set_manager(CdhContext *ctx, CdhManager *manager);

/**
 * @brief Request context info from manager
 * @param ctx Pointer to the CdhContext object
 */
void cdh_context_read_context_info(CdhContext *ctx);

/**
 * @brief Request epilog from manager
 * @param ctx Pointer to the CdhContext object
 */
void cdh_context_read_epilog(CdhContext *ctx);
#endif

/* @brief Generate crashid file
 * @param ctx
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_context_crashid_process(CdhContext *ctx);

/**
 * @brief Generate context data available pre coredump stream
 * @param ctx The context object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_context_generate_prestream(CdhContext *ctx);

/**
 * @brief Generate context data available post coredump stream
 * @param ctx The context object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_context_generate_poststream(CdhContext *ctx);

G_END_DECLS
