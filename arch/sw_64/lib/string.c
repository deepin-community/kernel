// SPDX-License-Identifier: GPL-2.0

#include <asm/hw_init.h>

#include <linux/export.h>

extern void *____memcpy_sisd(void *dest, const void *src, size_t n);
extern void *____memcpy_simd(void *dest, const void *src, size_t n);
extern void *____memcpy_simd_align(void *dest, const void *src, size_t n);

static inline void *____memcpy(void *dest, const void *src, size_t n)
{
	if (!IS_ENABLED(CONFIG_DEEP_MEMCPY))
		return ____memcpy_sisd(dest, src, n);

	if (static_branch_likely(&hw_una_enabled))
		return ____memcpy_simd(dest, src, n);
	else
		return ____memcpy_simd_align(dest, src, n);
}

void *memcpy(void *dest, const void *src, size_t n)
{
	return ____memcpy(dest, src, n);
}
EXPORT_SYMBOL(memcpy);

/* For backward compatibility with modules.  Unused otherwise.  */
void *__memcpy(void *dest, const void *src, size_t n)
{
	return ____memcpy(dest, src, n);
}
EXPORT_SYMBOL(__memcpy);

extern void *____constant_c_memset_sisd(void *s, unsigned long c, size_t n);
extern void *____constant_c_memset_simd(void *s, unsigned long c, size_t n);
extern void *____constant_c_memset_simd_align(void *s, unsigned long c, size_t n);

static inline void *____constant_c_memset(void *s, unsigned long c, size_t n)
{
	if (!IS_ENABLED(CONFIG_DEEP_MEMSET))
		return ____constant_c_memset_sisd(s, c, n);

	if (static_branch_likely(&hw_una_enabled))
		return ____constant_c_memset_simd(s, c, n);
	else
		return ____constant_c_memset_simd_align(s, c, n);
}

void *__constant_c_memset(void *s, unsigned long c, size_t n)
{
	return ____constant_c_memset(s, c, n);
}

void *___memset(void *s, int c, size_t n)
{
	unsigned long c_ul = (c & 0xff) * 0x0101010101010101UL;

	return ____constant_c_memset(s, c_ul, n);
}
EXPORT_SYMBOL(___memset);

void *__memset(void *s, int c, size_t n)
{
	unsigned long c_ul = (c & 0xff) * 0x0101010101010101UL;

	return ____constant_c_memset(s, c_ul, n);
}
EXPORT_SYMBOL(__memset);

void *memset(void *s, int c, size_t n)
{
	unsigned long c_ul = (c & 0xff) * 0x0101010101010101UL;

	return ____constant_c_memset(s, c_ul, n);
}
EXPORT_SYMBOL(memset);

void *__memsetw(void *dest, unsigned short c, size_t count)
{
	unsigned long c_ul = (c & 0xffff) * 0x0001000100010001UL;

	return ____constant_c_memset(dest, c_ul, count);
}
EXPORT_SYMBOL(__memsetw);
