/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_IO_H
#define _ASM_SW64_IO_H

#ifdef __KERNEL__

#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/compiler.h>
#include <asm/pgtable.h>

#define page_to_phys(page)	page_to_pa(page)

/* Maximum PIO space address supported?  */
#define IO_SPACE_LIMIT		0xffffffffffffffff

/*
 * We always have external versions of these routines.
 */
extern u8		inb(unsigned long port);
extern u16		inw(unsigned long port);
extern u32		inl(unsigned long port);
extern void		outb(u8 b, unsigned long port);
extern void		outw(u16 b, unsigned long port);
extern void		outl(u32 b, unsigned long port);
#define inb inb
#define inw inw
#define inl inl
#define outb outb
#define outw outw
#define outl outl

static inline void __iomem *__ioremap(phys_addr_t addr, size_t size,
				      pgprot_t prot)
{
	unsigned long tmp = addr | PAGE_OFFSET;

	return (void __iomem *)(tmp);
}

#define ioremap(addr, size)		__ioremap((addr), (size), PAGE_KERNEL)
#define ioremap_nocache(addr, size)	__ioremap((addr), (size), PAGE_KERNEL)
#define ioremap_cache(addr, size)	__ioremap((addr), (size), PAGE_KERNEL)
#define ioremap_uc			ioremap_nocache

#define ioport_map ioport_map
extern void __iomem *ioport_map(unsigned long port, unsigned int nr);

static inline void iounmap(volatile void __iomem *addr)
{
}

/*
 * String version of IO memory access ops:
 */
#define memcpy_fromio memcpy_fromio
extern void memcpy_fromio(void *buffer, const volatile void __iomem *addr, long len);

#define memcpy_toio memcpy_toio
extern void memcpy_toio(volatile void __iomem *addr, const void *buffer, long len);

extern void _memset_c_io(volatile void __iomem *addr, unsigned long c, long len);

#define memset_io memset_io
static inline void memset_io(volatile void __iomem *addr, u8 c, long len)
{
	_memset_c_io(addr, 0x0101010101010101UL * c, len);
}

static inline void memsetw_io(volatile void __iomem *addr, u16 c, long len)
{
	_memset_c_io(addr, 0x0001000100010001UL * c, len);
}

/*
 * String versions of in/out ops:
 */
extern void insb(unsigned long port, void *dst, unsigned long count);
extern void insw(unsigned long port, void *dst, unsigned long count);
extern void insl(unsigned long port, void *dst, unsigned long count);
extern void outsb(unsigned long port, const void *src, unsigned long count);
extern void outsw(unsigned long port, const void *src, unsigned long count);
extern void outsl(unsigned long port, const void *src, unsigned long count);

#define insb insb
#define insw insw
#define insl insl
#define outsb outsb
#define outsw outsw
#define outsl outsl

#define pci_iounmap pci_iounmap

#include <asm-generic/io.h>
#undef PCI_IOBASE

/*
 * Change addresses as seen by the kernel (virtual) to addresses as
 * seen by a device (bus), and vice versa.
 *
 * Note that this only works for a limited range of kernel addresses,
 * and very well may not span all memory.  Consider this interface
 * deprecated in favour of the DMA-mapping API.
 */
static inline unsigned long __deprecated virt_to_bus(void *address)
{
	return virt_to_phys(address);
}
#define isa_virt_to_bus virt_to_bus

static inline void * __deprecated bus_to_virt(unsigned long address)
{
	void *virt;

	/* This check is a sanity check but also ensures that bus address 0
	 * maps to virtual address 0 which is useful to detect null pointers
	 * (the NCR driver is much simpler if NULL pointers are preserved).
	 */
	virt = phys_to_virt(address);
	return (long)address <= 0 ? NULL : virt;
}
#define isa_bus_to_virt bus_to_virt

static inline int pci_remap_iospace(const struct resource *res,
		phys_addr_t phys_addr)
{
	if (!(res->flags & IORESOURCE_IO))
		return -EINVAL;

	if (res->end > IO_SPACE_LIMIT)
		return -EINVAL;

	return 0;
}

#define pci_remap_iospace pci_remap_iospace

#endif /* __KERNEL__ */

#endif /* _ASM_SW64_IO_H */
