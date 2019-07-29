/* cdm-data.h
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

#ifndef CDH_DATA_H
#define CDH_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cdh-archive.h"
#include "cdh-info.h"
#include "cdm-logging.h"
#include "cdm-options.h"
#include "cdm-types.h"
#if defined(WITH_CRASHMANAGER)
#include "cdh-manager.h"
#endif

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/procfs.h>
#include <sys/user.h>
#include <unistd.h>

/**
 * @struct CdhData
 * @brief The global cdh data object referencing main submodules and states
 */
typedef struct _CdhData {
    CdhArchive archive; /**< coredump archive streamer */
    CdmRegisters regs; /**< cpu registers for crash id calculation */
    CdhInfo *info;     /**< Crash info data */
#if defined(WITH_CRASHMANAGER)
    CdhManager crash_mgr; /**< manager ipc object */
#endif

    Elf64_Ehdr ehdr;   /**< coredump elf Ehdr structure */
    Elf64_Phdr *pphdr; /**< coredump elf pPhdr pointer to structure */
    gchar *nhdr;        /**< buffer with all NOTE pages */

    guint64  ra;                     /**< return address for top frame */
    guint64  ip_file_offset;         /**< instruction pointer file offset for top frame */
    guint64  ra_file_offset;         /**< return address file offset for top frame */
    const gchar *ip_module_name;      /**< module name pointed by instruction pointer */
    const gchar *ra_module_name;      /**< module name pointed by return address */
    gulong note_page_size;    /**< note section page size */
    gulong elf_vma_page_size; /**< elf vma page size */
    guint8  crashid_info; /**< type of information available for crashid */
} CdhData;

/**
 * @brief Initialize pre-allocated cdh object
 * @param d The cdh object to initialize
 * @param conf_path Full path to the cdh configuration fole
 */
void cdh_data_init(CdhData *d, const gchar *config_path);

/**
 * @brief Deinitialize pre-allocated cdm object
 * @param d The cdh object to deinitialize
 */
void cdh_data_deinit(CdhData *d);

/**
 * @brief Execute cdh logic
 *
 * @param d The cdh object to deinitialize
 * @param argc Main arguments count
 * @param argv Main arguments table
 *
 * @return If run was succesful CDH_OK is returned
 */
CdmStatus cdh_main_enter(CdhData *d, gint argc, gchar *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* CDH_DATA_H */
