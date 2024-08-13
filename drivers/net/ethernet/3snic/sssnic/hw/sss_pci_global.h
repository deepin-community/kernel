/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_PCI_GLOBAL_H
#define SSS_PCI_GLOBAL_H

#include <linux/types.h>

#include "sss_hw_uld_driver.h"

struct sss_uld_info *sss_get_uld_info(void);
bool sss_attach_is_enable(void);
const char **sss_get_uld_names(void);
void sss_init_uld_lock(void);
void sss_lock_uld(void);
void sss_unlock_uld(void);

#endif
