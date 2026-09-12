/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LIB_CLASS_TYPE_H
#define _LIB_CLASS_TYPE_H

#include <linux/types.h>

char *get_cpu_name(void);
unsigned long get_chassis_types(void);
bool chassis_types_is_laptop(void);
bool chassis_types_is_allinone(void);

#endif
