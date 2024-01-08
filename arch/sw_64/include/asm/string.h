/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_STRING_H
#define _ASM_SW64_STRING_H

#ifdef __KERNEL__

/*
 * GCC of any recent vintage doesn't do stupid things with bcopy.
 * EGCS 1.1 knows all about expanding memcpy inline, others don't.
 *
 * Similarly for a memset with data = 0.
 */

#define __HAVE_ARCH_MEMCPY
extern void *memcpy(void *dest, const void *src, size_t n);
/* For backward compatibility with modules.  Unused otherwise.  */
extern void *__memcpy(void *dest, const void *src, size_t n);

#define __HAVE_ARCH_MEMMOVE
extern void *memmove(void *dest, const void *src, size_t n);

#define __HAVE_ARCH_MEMSET
extern void *__constant_c_memset(void *s, unsigned long c, size_t n);
extern void *___memset(void *s, int c, size_t n);
extern void *__memset(void *s, int c, size_t n);
extern void *memset(void *s, int c, size_t n);

#define __HAVE_ARCH_STRCPY
extern char *strcpy(char *dest, const char *src);

#define __HAVE_ARCH_STRNCPY
extern char *strncpy(char *dest, const char *src, size_t n);

/* The following routine is like memset except that it writes 16-bit
 * aligned values.  The DEST and COUNT parameters must be even for
 * correct operation.
 */

#define __HAVE_ARCH_MEMSETW
extern void *__memsetw(void *dest, unsigned short c, size_t count);

#define memsetw(s, c, n)						 \
(__builtin_constant_p(c)						 \
	? __constant_c_memset((s), 0x0001000100010001UL * (unsigned short)(c), (n)) \
	: __memsetw((s), (c), (n)))

#ifdef CONFIG_ARCH_HAS_UACCESS_FLUSHCACHE
#define __HAVE_ARCH_MEMCPY_FLUSHCACHE
void memcpy_flushcache(void *dst, const void *src, size_t cnt);
#endif

#endif /* __KERNEL__ */

#endif /* _ASM_SW64_STRING_H */
