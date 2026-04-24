/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_SW64_SET_MEMORY_H
#define _ASM_SW64_SET_MEMORY_H

#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE
int set_memory_ro(unsigned long addr, int numpages);
int set_memory_rw(unsigned long addr, int numpages);
int set_memory_x(unsigned long addr, int numpages);
int set_memory_nx(unsigned long addr, int numpages);
#endif

#endif /* _ASM_SW64_SET_MEMORY_H */
