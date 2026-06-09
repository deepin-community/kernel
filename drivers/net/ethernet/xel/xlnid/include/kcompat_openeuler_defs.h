/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2008 - 2023 Xel Technology. */

#ifndef _KCOMPAT_OPENEULER_DEFS_H_
#define _KCOMPAT_OPENEULER_DEFS_H_

/*  This file contains the definitions for the OPENEULER specific distribution of
		the Linux kernel.

		It checks the OPENEULER_VERSION_CODE to decide which features are available in
		the target kernel. It assumes that kcompat_std_defs.h has already been
		processed, and will #define or #undef the relevant flags based on what
		features were backported by OPENEULER.
*/

#if !OPENEULER_VERSION_CODE
#error "OPENEULER_VERSION_CODE is 0 or undefined"
#endif

#ifndef OPENEULER_VERSION
#error "OPENEULER_VERSION is undefined"
#endif

/*****************************************************************************/
#if (OPENEULER_VERSION_CODE == OPENEULER_VERSION(2203, 0))
#define HAVE_ETHTOOL_EXTENDED_RINGPARAMS
#define HAVE_ETHTOOL_COALESCE_EXTACK
#endif

/*****************************************************************************/
#endif /* _KCOMPAT_OPENEULER_DEFS_H_ */

