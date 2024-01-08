// SPDX-License-Identifier: GPL-2.0
/*
 * Sw_64 IO and memory functions.
 */

#include <linux/module.h>

#include <asm/io.h>
#include <asm/platform.h>

/*
 * Here comes the sw64 implementation of the IOMAP interfaces.
 */
unsigned int ioread8(const void __iomem *addr)
{
	return readb(addr);
}
EXPORT_SYMBOL(ioread8);

unsigned int ioread16(const void __iomem *addr)
{
	return readw(addr);
}
EXPORT_SYMBOL(ioread16);

unsigned int ioread32(const void __iomem *addr)
{
	return readl(addr);
}
EXPORT_SYMBOL(ioread32);

void iowrite8(u8 b, void __iomem *addr)
{
	writeb(b, addr);
}
EXPORT_SYMBOL(iowrite8);

void iowrite16(u16 b, void __iomem *addr)
{
	writew(b, addr);
}
EXPORT_SYMBOL(iowrite16);

void iowrite32(u32 b, void __iomem *addr)
{
	writel(b, addr);
}
EXPORT_SYMBOL(iowrite32);

u8 inb(unsigned long port)
{
	return ioread8(ioport_map(port, 1));
}
EXPORT_SYMBOL(inb);

u16 inw(unsigned long port)
{
	return ioread16(ioport_map(port, 2));
}
EXPORT_SYMBOL(inw);

u32 inl(unsigned long port)
{
	return ioread32(ioport_map(port, 4));
}
EXPORT_SYMBOL(inl);

void outb(u8 b, unsigned long port)
{
	iowrite8(b, ioport_map(port, 1));
}
EXPORT_SYMBOL(outb);

void outw(u16 b, unsigned long port)
{
	iowrite16(b, ioport_map(port, 2));
}
EXPORT_SYMBOL(outw);

void outl(u32 b, unsigned long port)
{
	iowrite32(b, ioport_map(port, 4));
}
EXPORT_SYMBOL(outl);


/*
 * Read COUNT 8-bit bytes from port PORT into memory starting at SRC.
 */
void ioread8_rep(const void __iomem *port, void *dst, unsigned long count)
{
	while ((unsigned long)dst & 0x3) {
		if (!count)
			return;
		count--;
		*(unsigned char *)dst = ioread8(port);
		dst += 1;
	}

	while (count >= 4) {
		unsigned int w;

		count -= 4;
		w = ioread8(port);
		w |= ioread8(port) << 8;
		w |= ioread8(port) << 16;
		w |= ioread8(port) << 24;
		*(unsigned int *)dst = w;
		dst += 4;
	}

	while (count) {
		--count;
		*(unsigned char *)dst = ioread8(port);
		dst += 1;
	}
}
EXPORT_SYMBOL(ioread8_rep);

void insb(unsigned long port, void *dst, unsigned long count)
{
	ioread8_rep(ioport_map(port, 1), dst, count);
}
EXPORT_SYMBOL(insb);

/*
 * Read COUNT 16-bit words from port PORT into memory starting at
 * SRC.  SRC must be at least short aligned.  This is used by the
 * IDE driver to read disk sectors.  Performance is important, but
 * the interfaces seems to be slow: just using the inlined version
 * of the inw() breaks things.
 */
void ioread16_rep(const void __iomem *port, void *dst, unsigned long count)
{
	if (unlikely((unsigned long)dst & 0x3)) {
		if (!count)
			return;
		BUG_ON((unsigned long)dst & 0x1);
		count--;
		*(unsigned short *)dst = ioread16(port);
		dst += 2;
	}

	while (count >= 2) {
		unsigned int w;

		count -= 2;
		w = ioread16(port);
		w |= ioread16(port) << 16;
		*(unsigned int *)dst = w;
		dst += 4;
	}

	if (count)
		*(unsigned short *)dst = ioread16(port);
}
EXPORT_SYMBOL(ioread16_rep);

void insw(unsigned long port, void *dst, unsigned long count)
{
	ioread16_rep(ioport_map(port, 2), dst, count);
}
EXPORT_SYMBOL(insw);


/*
 * Read COUNT 32-bit words from port PORT into memory starting at
 * SRC. Now works with any alignment in SRC. Performance is important,
 * but the interfaces seems to be slow: just using the inlined version
 * of the inl() breaks things.
 */
void ioread32_rep(const void __iomem *port, void *dst, unsigned long count)
{
	if (unlikely((unsigned long)dst & 0x3)) {
		while (count--) {
			struct S { int x __packed; };
			((struct S *)dst)->x = ioread32(port);
			dst += 4;
		}
	} else {
		/* Buffer 32-bit aligned.  */
		while (count--) {
			*(unsigned int *)dst = ioread32(port);
			dst += 4;
		}
	}
}
EXPORT_SYMBOL(ioread32_rep);

void insl(unsigned long port, void *dst, unsigned long count)
{
	ioread32_rep(ioport_map(port, 4), dst, count);
}
EXPORT_SYMBOL(insl);


/*
 * Like insb but in the opposite direction.
 * Don't worry as much about doing aligned memory transfers:
 * doing byte reads the "slow" way isn't nearly as slow as
 * doing byte writes the slow way (no r-m-w cycle).
 */
void iowrite8_rep(void __iomem *port, const void *xsrc, unsigned long count)
{
	const unsigned char *src = xsrc;

	while (count--)
		iowrite8(*src++, port);
}
EXPORT_SYMBOL(iowrite8_rep);

void outsb(unsigned long port, const void *src, unsigned long count)
{
	iowrite8_rep(ioport_map(port, 1), src, count);
}
EXPORT_SYMBOL(outsb);


/*
 * Like insw but in the opposite direction.  This is used by the IDE
 * driver to write disk sectors.  Performance is important, but the
 * interfaces seems to be slow: just using the inlined version of the
 * outw() breaks things.
 */
void iowrite16_rep(void __iomem *port, const void *src, unsigned long count)
{
	if (unlikely((unsigned long)src & 0x3)) {
		if (!count)
			return;
		BUG_ON((unsigned long)src & 0x1);
		iowrite16(*(unsigned short *)src, port);
		src += 2;
		--count;
	}

	while (count >= 2) {
		unsigned int w;

		count -= 2;
		w = *(unsigned int *)src;
		src += 4;
		iowrite16(w >>  0, port);
		iowrite16(w >> 16, port);
	}

	if (count)
		iowrite16(*(unsigned short *)src, port);
}
EXPORT_SYMBOL(iowrite16_rep);

void outsw(unsigned long port, const void *src, unsigned long count)
{
	iowrite16_rep(ioport_map(port, 2), src, count);
}
EXPORT_SYMBOL(outsw);


/*
 * Like insl but in the opposite direction.  This is used by the IDE
 * driver to write disk sectors.  Works with any alignment in SRC.
 * Performance is important, but the interfaces seems to be slow:
 * just using the inlined version of the outl() breaks things.
 */
void iowrite32_rep(void __iomem *port, const void *src, unsigned long count)
{
	if (unlikely((unsigned long)src & 0x3)) {
		while (count--) {
			struct S { int x __packed; };
			iowrite32(((struct S *)src)->x, port);
			src += 4;
		}
	} else {
		/* Buffer 32-bit aligned.  */
		while (count--) {
			iowrite32(*(unsigned int *)src, port);
			src += 4;
		}
	}
}
EXPORT_SYMBOL(iowrite32_rep);

void outsl(unsigned long port, const void *src, unsigned long count)
{
	iowrite32_rep(ioport_map(port, 4), src, count);
}
EXPORT_SYMBOL(outsl);


/*
 * Copy data from IO memory space to "real" memory space.
 * This needs to be optimized.
 */
void memcpy_fromio(void *to, const volatile void __iomem *from, long count)
{
	/*
	 * Optimize co-aligned transfers.  Everything else gets handled
	 * a byte at a time.
	 */

	if (count >= 8 && ((u64)to & 7) == ((u64)from & 7)) {
		count -= 8;
		do {
			*(u64 *)to = __raw_readq(from);
			count -= 8;
			to += 8;
			from += 8;
		} while (count >= 0);
		count += 8;
	}

	if (count >= 4 && ((u64)to & 3) == ((u64)from & 3)) {
		count -= 4;
		do {
			*(u32 *)to = __raw_readl(from);
			count -= 4;
			to += 4;
			from += 4;
		} while (count >= 0);
		count += 4;
	}

	if (count >= 2 && ((u64)to & 1) == ((u64)from & 1)) {
		count -= 2;
		do {
			*(u16 *)to = __raw_readw(from);
			count -= 2;
			to += 2;
			from += 2;
		} while (count >= 0);
		count += 2;
	}

	while (count > 0) {
		*(u8 *) to = __raw_readb(from);
		count--;
		to++;
		from++;
	}
	mb();
}
EXPORT_SYMBOL(memcpy_fromio);


/*
 * Copy data from "real" memory space to IO memory space.
 * This needs to be optimized.
 */
void memcpy_toio(volatile void __iomem *to, const void *from, long count)
{
	/*
	 * Optimize co-aligned transfers.  Everything else gets handled
	 * a byte at a time.
	 * FIXME -- align FROM.
	 */

	if (count >= 8 && ((u64)to & 7) == ((u64)from & 7)) {
		count -= 8;
		do {
			__raw_writeq(*(const u64 *)from, to);
			count -= 8;
			to += 8;
			from += 8;
		} while (count >= 0);
		count += 8;
	}

	if (count >= 4 && ((u64)to & 3) == ((u64)from & 3)) {
		count -= 4;
		do {
			__raw_writel(*(const u32 *)from, to);
			count -= 4;
			to += 4;
			from += 4;
		} while (count >= 0);
		count += 4;
	}

	if (count >= 2 && ((u64)to & 1) == ((u64)from & 1)) {
		count -= 2;
		do {
			__raw_writew(*(const u16 *)from, to);
			count -= 2;
			to += 2;
			from += 2;
		} while (count >= 0);
		count += 2;
	}

	while (count > 0) {
		__raw_writeb(*(const u8 *) from, to);
		count--;
		to++;
		from++;
	}
	mb();
}
EXPORT_SYMBOL(memcpy_toio);


/*
 * "memset" on IO memory space.
 */
void _memset_c_io(volatile void __iomem *to, unsigned long c, long count)
{
	/* Handle any initial odd byte */
	if (count > 0 && ((u64)to & 1)) {
		__raw_writeb(c, to);
		to++;
		count--;
	}

	/* Handle any initial odd halfword */
	if (count >= 2 && ((u64)to & 2)) {
		__raw_writew(c, to);
		to += 2;
		count -= 2;
	}

	/* Handle any initial odd word */
	if (count >= 4 && ((u64)to & 4)) {
		__raw_writel(c, to);
		to += 4;
		count -= 4;
	}

	/*
	 * Handle all full-sized quadwords: we're aligned
	 *  (or have a small count)
	 */
	count -= 8;
	if (count >= 0) {
		do {
			__raw_writeq(c, to);
			to += 8;
			count -= 8;
		} while (count >= 0);
	}
	count += 8;

	/* The tail is word-aligned if we still have count >= 4 */
	if (count >= 4) {
		__raw_writel(c, to);
		to += 4;
		count -= 4;
	}

	/* The tail is half-word aligned if we have count >= 2 */
	if (count >= 2) {
		__raw_writew(c, to);
		to += 2;
		count -= 2;
	}

	/* And finally, one last byte.. */
	if (count)
		__raw_writeb(c, to);
	mb();
}
EXPORT_SYMBOL(_memset_c_io);

void __iomem *ioport_map(unsigned long port, unsigned int size)
{
	unsigned long io_offset;

	if (port < 0x100000) {
		io_offset = is_in_host() ? LPC_LEGACY_IO : PCI_VT_LEGACY_IO;
		port = port | io_offset;
	}

	return __va(port);
}
EXPORT_SYMBOL(ioport_map);

void ioport_unmap(void __iomem *addr)
{
}
EXPORT_SYMBOL(ioport_unmap);
