/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_CACHEFLUSH_H
#define _ASM_SW64_CACHEFLUSH_H

/*
 * DCache: PIPT
 * ICache:
 *	- C3B is VIVT with ICTAG, support coherence.
 *	- C4 is VIPT
 */
#include <asm-generic/cacheflush.h>

#endif /* _ASM_SW64_CACHEFLUSH_H */
