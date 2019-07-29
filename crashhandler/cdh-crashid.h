/*
 *
 * Copyright (C) 2019, Alin Popa. All rights reserved.
 *
 * This file is part of Coredumper
 * The module is responsible to generate the crash id using the available global crash data
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \copyright Copyright © 2019, Alin Popa
 *
 */

#ifndef CDH_CRASHID_H
#define CDH_CRASHID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cdh.h"

/* @brief Generate crashid file
 *
 * @param d Global cdh data
 * @param tmpdir Temp directory to store the context file into
 *
 * @return CDH_OK on success
 */
cdh_status_t cdh_crashid_process(cdh_data_t *d);

#ifdef __cplusplus
}
#endif

#endif /* CDH_CRASHID_H */
