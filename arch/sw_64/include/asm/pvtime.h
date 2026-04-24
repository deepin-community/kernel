/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 SW64 Ltd. */

#ifndef __ASM_PVTIME_ABI_H
#define __ASM_PVTIME_ABI_H

/* The below structure is defined in SW64 */

struct pvclock_vcpu_steal_time {
	__u64 steal_time;
	__u32 version;
	/* Structure must be 128 byte aligned, pad to that size */
	__u32 padding[29];
} __packed;

#endif
