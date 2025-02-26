// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#include <linux/set_memory.h>

void set_iee_page(unsigned long addr, unsigned int order)
{
	set_memory_ro(addr, 1 << order);
}

void unset_iee_page(unsigned long addr, unsigned int order)
{
	set_memory_rw(addr, 1 << order);
}
