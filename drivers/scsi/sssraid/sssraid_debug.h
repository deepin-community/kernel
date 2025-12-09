/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 3SNIC Information Technology, Ltd */

/* 3SNIC RAID SSSXXX Series Linux Driver */

#ifndef SSSRAID_DEBUG_H_INCLUDED
#define SSSRAID_DEBUG_H_INCLUDED

/*
 * debug levels
 */
#define SSSRAID_DEBUG			0x00000001

/*
 * debug macros
 */

#define ioc_err(ioc, fmt, ...) \
	pr_err("%s: " fmt, (ioc)->name, ##__VA_ARGS__)
#define ioc_notice(ioc, fmt, ...) \
	pr_notice("%s: " fmt, (ioc)->name, ##__VA_ARGS__)
#define ioc_warn(ioc, fmt, ...) \
	pr_warn("%s: " fmt, (ioc)->name, ##__VA_ARGS__)
#define ioc_info(ioc, fmt, ...) \
	pr_info("%s: " fmt, (ioc)->name, ##__VA_ARGS__)


#define dbgprint(IOC, FMT, ...) \
	do { \
		if (unlikely(IOC->logging_level & SSSRAID_DEBUG)) \
			pr_info("%s: " FMT, (IOC)->name, ##__VA_ARGS__); \
	} while (0)

#endif /* SSSRAID_DEBUG_H_INCLUDED */
