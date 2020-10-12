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
 * \file cdm-logging.h
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief Open logging subsystem
 *
 * @param app_name Application identifier
 * @param app_desc Application description
 * @param ctx_name Context identifier
 * @param ctx_desc Context description
 */
void                    cdm_logging_open                    (const gchar *app_name,
                                                             const gchar *app_desc,
                                                             const gchar *ctx_name,
                                                             const gchar *ctx_desc);

/**
 * @brief Close logging system
 */
void                    cdm_logging_close                   (void);

G_END_DECLS
