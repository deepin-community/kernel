/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Hygon CSV header for early boot related functions.
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 */

#ifndef BOOT_COMPRESSED_CSV_H
#define BOOT_COMPRESSED_CSV_H

#ifdef CONFIG_HYGON_CSV

void csv_set_status(void);
void csv_init_secure_call_pages(void *boot_params);

void csv_update_page_attr(unsigned long address, pteval_t set, pteval_t clr);

#else

static inline void csv_set_status(void) { }
static inline void csv_init_secure_call_pages(void *boot_params) { }

static inline void csv_update_page_attr(unsigned long address,
					pteval_t set, pteval_t clr) { }

#endif

#endif	/* BOOT_COMPRESSED_CSV_H */
