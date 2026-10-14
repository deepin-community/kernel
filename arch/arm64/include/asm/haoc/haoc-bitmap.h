/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_HAOC_BITMAP_H
#define _LINUX_HAOC_BITMAP_H

#include <asm/haoc/haoc-def.h>
#include <linux/types.h>

/* HAOC_NORMAL means */
enum HAOC_BITMAP_TYPE {
	IEE_NORMAL = 0,		/* Non-IEE pages */
	IEE_DATA,
	IEE_TOKEN,
	IEE_PGTABLE,
	IEE_USER_PGTABLE,  /* for PTP_S */
	IEE_KEY,
	IEE_SELINUX,
	IEE_VARP,
	IEE_CRED,
};

#include <linux/mm.h>

#define HAOC_BITMAP_START	VMEMMAP_END
#define haoc_bitmap_base	((uint8_t *)HAOC_BITMAP_START - \
				 (memstart_addr >> PAGE_SHIFT))

#define __pfn_to_haoc_bitmap(pfn)	(haoc_bitmap_base + (pfn))

#define __va_to_haoc_bitmap(va)	({	\
	uint8_t *__ret;					\
	if (__is_lm_address((u64)va))	\
		__ret = __pfn_to_haoc_bitmap(PHYS_PFN(__pa(va)));		\
	else						\
		__ret = __pfn_to_haoc_bitmap(PHYS_PFN(__pa_symbol(va)));	\
	__ret;						\
})

extern bool haoc_enabled;
extern bool haoc_bitmap_ready;

extern int haoc_bitmap_sparse_init(void);
extern void haoc_bitmap_setup(void);
extern void setup_iee_early_data_bitmap(void);

static inline enum HAOC_BITMAP_TYPE iee_get_bitmap_type(unsigned long va)
{
	return *__va_to_haoc_bitmap(va);
}

void _iee_set_bitmap_type(unsigned long __unused,
				u64 va, enum HAOC_BITMAP_TYPE type, int num_pages);

static inline void iee_set_bitmap_type(unsigned long va,
					int num_pages, enum HAOC_BITMAP_TYPE type)
{
	iee_rw_gate(IEE_OP_SET_BITMAP_TYPE, va, type, num_pages);
}

static inline void iee_verify_type(unsigned long va, enum HAOC_BITMAP_TYPE type,
					const char *name)
{
	if (haoc_enabled) {
		uint8_t bit_type = iee_get_bitmap_type(va);

		if (unlikely(bit_type != type))
			panic("IEE detected type: %d, fake %s: va(0x%lx)", bit_type, name, va);
	}
}

static inline unsigned long iee_test_not_normal(void *start, void *end)
{
	unsigned long addr;

	if (!haoc_enabled)
		return 0;
	addr = (unsigned long)start;
	while (addr < (unsigned long)end) {
		if (unlikely(iee_get_bitmap_type(addr) == IEE_NORMAL))
			return addr;
		addr += PAGE_SIZE;
	}
	return 0;
}

#define iee_verify_not_normal(start, end) ({	\
	unsigned long __addr = iee_test_not_normal(start, end);	\
	if (__addr)		\
		panic("HAOC: (%s)operate on IEE bitmap type normal 0x%lx.", __func__, __addr);	\
})

#endif
