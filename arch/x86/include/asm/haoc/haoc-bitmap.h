/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_HAOC_BITMAP_H
#define _LINUX_HAOC_BITMAP_H

#include <asm/haoc/iee.h>
#include <asm/haoc/haoc-def.h>
#include <linux/types.h>
extern unsigned long iee_bitmap_base;
#define __VIEEBITMAP_BASE_L4	0xffffeb0000000000UL
#define __VIEEBITMAP_BASE_L5	0xffd6000000000000UL
#define __pfn_to_iee_bitmap(pfn)	((uint8_t *)iee_bitmap_base + (pfn))

enum HAOC_BITMAP_TYPE {
	IEE_NORMAL = 0,
	IEE_PGTABLE,
	IEE_USER_PGTABLE,  // modifying IEE_USER_PGTABLE needn't use ptp_rw_gate
	IEE_FIXED_PGTABLE,
	IEE_POLICY,
	IEE_KEY,
	IEE_SIP_DATA,
	IEE_BITMAP,
	IEE_VARP,
};

extern void iee_init_bitmap(void);
#define IEE_PHYS_BIT_PTR(phys_addr) ((uint8_t *)((uint8_t *)iee_bitmap_base + PHYS_PFN(phys_addr)))

extern unsigned long long iee_rw_gate(int flag, ...);

static inline void iee_set_bitmap_type(phys_addr_t phys_addr, int num_pages,
				       enum HAOC_BITMAP_TYPE type)
{
	uint8_t *bitmap_ptr = IEE_PHYS_BIT_PTR(phys_addr);

	iee_rw_gate(IEE_OP_SET_BITMAP_TYPE, bitmap_ptr, type, num_pages);
}

static inline enum HAOC_BITMAP_TYPE iee_get_bitmap_type(phys_addr_t phys_addr)
{
	return *IEE_PHYS_BIT_PTR(phys_addr);
}

static inline void iee_verify_type(phys_addr_t phys_addr,
				   enum HAOC_BITMAP_TYPE type,
				   const char *name)
{
	uint8_t bit_type;

	if (unlikely(!haoc_init_done))
		return;

	bit_type = iee_get_bitmap_type(phys_addr);

	if (unlikely(bit_type != type))
		panic("IEE detected type: %d, fake %s: va(0x%lx)",
				bit_type, name, (unsigned long)__va(phys_addr));
}

#endif
