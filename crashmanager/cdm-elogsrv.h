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
 * \file cdm-elogsrv.h
 */

#pragma once

#include "cdm-types.h"
#include "cdm-options.h"
#include "cdm-journal.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief The CdmELogSrv opaque data structure
 */
typedef struct _CdmELogSrv {
  GSource source;         /**< Event loop source */
  grefcount rc;           /**< Reference counter variable  */
  gpointer tag;           /**< Unix elogsrv socket tag  */
  gint sockfd;            /**< Module file descriptor (elogsrv listen fd) */
  CdmOptions *options;    /**< Own reference to global options */
  CdmJournal *journal;    /**< Own a reference to the journal object */
} CdmELogSrv;

/*
 * @brief Create a new elogsrv object
 * @param options A pointer to the CdmOptions object created by the main application
 * @param journal A pointer to the CdmJournal object created by the main application
 * @return On success return a new CdmELogSrv object otherwise return NULL
 */
CdmELogSrv *            cdm_elogsrv_new                     (CdmOptions *options, 
                                                             CdmJournal *journal, 
                                                             GError **error);

/**
 * @brief Aquire elogsrv object
 * @param elogsrv Pointer to the elogsrv object
 * @return The elogsrv object
 */
CdmELogSrv *            cdm_elogsrv_ref                     (CdmELogSrv *elogsrv);

/**
 * @brief Start the elogsrv an listen for clients
 * @param elogsrv Pointer to the elogsrv object
 * @return If elogsrv starts listening the function return CDM_STATUS_OK
 */
CdmStatus               cdm_elogsrv_bind_and_listen         (CdmELogSrv *elogsrv);

/**
 * @brief Release elogsrv object
 * @param elogsrv Pointer to the elogsrv object
 */
void                    cdm_elogsrv_unref                   (CdmELogSrv *elogsrv);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CdmELogSrv, cdm_elogsrv_unref);

G_END_DECLS
