/* cdh-context.h
 *
 * Copyright 2019 Alin Popa <alin.popa@bmw.de>
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

#pragma once

#include "cdm-options.h"
#include "cdh-archive.h"
#if defined(WITH_CRASHMANAGER)
#include "cdh-manager.h"
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * @struct CdhContext
 * @brief The context object
 */
typedef struct _CdhContext {
  CdmOptions *opts;       /**< Reference to objects (owned) */
  CdhArchive *archive;    /**< Reference to archive (owned)  */
#if defined(WITH_CRASHMANAGER)
  CdhManager *manager;    /**< Reference to manager (owned)  */
#endif
  grefcount rc;           /**< Reference counter variable  */

  gchar *name;       /**< process name */
  gchar *tname;      /**< thread name  */
  gchar *pexe;       /**< process executable file path  */
  guint64 tstamp;    /**< crash timestamp */
  gint64 sig;        /**< signal id */
  gint64 pid;        /**< process id as seen on host */
  gint64 cpid;       /**< process id as seen on namespace */

  gsize cdsize;      /**< coredump size */

  gchar *contextid;        /**< namespace context for the crashed pid */
  gchar *context_name;     /**< context name for the crashed pid */
  gchar *lifecycle_state;  /**< lifecycle state when crash */
  gchar *crashid;    /**< crash id value */
  gchar *vectorid;   /**< crash course id value */
  gboolean onhost;   /**< true if the crash is in host context */

  CdmRegisters regs;   /**< cpu registers for crash id calculation */
  Elf64_Ehdr ehdr;     /**< coredump elf Ehdr structure */
  Elf64_Phdr *pphdr;   /**< coredump elf pPhdr pointer to structure */
  gchar *nhdr;         /**< buffer with all NOTE pages */
  guint64 ra;                        /**< return address for top frame */
  guint64 ip_file_offset;            /**< instruction pointer file offset for top frame */
  guint64 ra_file_offset;            /**< return address file offset for top frame */
  const gchar *ip_module_name;       /**< module name pointed by instruction pointer */
  const gchar *ra_module_name;       /**< module name pointed by return address */
  gulong note_page_size;             /**< note section page size */
  gulong elf_vma_page_size;          /**< elf vma page size */
  guint8 crashid_info;               /**< type of information available for crashid */
} CdhContext;

/**
 * @brief Create a new CdhContext object
 * @return A pointer to the new object
 */
CdhContext *cdh_context_new (CdmOptions *opts, CdhArchive *archive);

/**
 * @brief Aquire CdhContext object
 * @param ctx Pointer to the CdhContext object
 * @return Pointer to the CdhContext object
 */
CdhContext *cdh_context_ref (CdhContext *ctx);

/**
 * @brief Release a CdhContext object
 * @param ctx Pointer to the cdh_context object
 */
void cdh_context_unref (CdhContext *ctx);

#if defined(WITH_CRASHMANAGER)
/**
 * @brief Set manager object
 * @param ctx Pointer to the CdhContext object
 * @param manager Pointer to the CdhManager object
 */
void cdh_context_set_manager (CdhContext *ctx, CdhManager *manager);

/**
 * @brief Request context info from manager
 * @param ctx Pointer to the CdhContext object
 */
void cdh_context_read_context_info (CdhContext *ctx);
#endif

/* @brief Generate crashid file
 * @param ctx
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_context_crashid_process (CdhContext *ctx);

/**
 * @brief Generate context data available pre coredump stream
 * @param ctx The context object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_context_generate_prestream (CdhContext *ctx);

/**
 * @brief Generate context data available post coredump stream
 * @param ctx The context object
 * @return CDM_STATUS_OK on success
 */
CdmStatus cdh_context_generate_poststream (CdhContext *ctx);

G_END_DECLS
