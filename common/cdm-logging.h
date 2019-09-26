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
void cdm_logging_open (const gchar *app_name,
                       const gchar *app_desc,
                       const gchar *ctx_name,
                       const gchar *ctx_desc);

/**
 * @brief Close logging system
 */
void cdm_logging_close (void);

G_END_DECLS
