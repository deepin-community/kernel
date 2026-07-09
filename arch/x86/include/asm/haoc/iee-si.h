/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_SI_H
#define _LINUX_IEE_SI_H
#include <linux/types.h>
#include <asm/haoc/haoc-def.h>

#define __iee_si_code   __section(".iee.si_text")
#define __iee_si_data   __section(".iee.si_data")

extern unsigned long cr4_pinned_mask;
extern struct static_key_false cr_pinning;
extern unsigned long cr4_pinned_bits;

extern unsigned long __iee_si_text_start[];
extern unsigned long __iee_si_text_end[];
extern unsigned long __iee_si_data_start[];
extern unsigned long __iee_si_data_end[];
extern void iee_sip_init(void);

extern void iee_rwx_gate(int flag, ...);

static inline void iee_sip_test(void)
{
	iee_rwx_gate(IEE_SIP_TEST);
}
static inline void iee_write_cr0(unsigned long val)
{
	iee_rwx_gate(IEE_WRITE_CR0, val);
}
static inline void iee_write_cr3(unsigned long val)
{
	iee_rwx_gate(IEE_WRITE_CR3, val);
}
static inline void iee_write_cr4(unsigned long val)
{
	iee_rwx_gate(IEE_WRITE_CR4, val);
}
static inline void iee_load_idt(void *ptr)
{
	iee_rwx_gate(IEE_LOAD_IDT, ptr);
}
extern bool iee_init_done;

#ifdef CONFIG_IEE_SELINUX_P
extern unsigned long __iee_selinux_data_start[];
extern unsigned long __iee_selinux_data_end[];
#endif

#endif
