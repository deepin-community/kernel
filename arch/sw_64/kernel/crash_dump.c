// SPDX-License-Identifier: GPL-2.0
/*
 * arch/sw_64/kernel/crash_dump.c
 *
 * Copyright (C) 2019 JN
 * Author: He Sheng
 *
 * This code is taken from arch/x86/kernel/crash_dump_64.c
 *   Created by: Hariprasad Nellitheertha (hari@in.ibm.com)
 *   Copyright (C) IBM Corporation, 2004. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/uio.h>

ssize_t copy_oldmem_page(struct iov_iter *iter, unsigned long pfn,
			 size_t csize, unsigned long offset)
{
	void *vaddr;

	if (!csize)
		return 0;

	vaddr = ioremap(__pfn_to_phys(pfn), PAGE_SIZE);
	if (!vaddr)
		return -ENOMEM;

	csize = copy_to_iter(vaddr + offset, csize, iter);

	iounmap(vaddr);
	return csize;
}
