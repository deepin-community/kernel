// SPDX-License-Identifier: GPL-2.0
/*
 * Sw_64 IO and memory functions.
 */

#include <linux/module.h>

#include <asm/io.h>
#include <asm/platform.h>

extern unsigned long legacy_io_base;
extern unsigned long legacy_io_shift;

/*
 * Here comes the sw64 implementation of the IOMAP interfaces.
 */
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

void insb(unsigned long port, void *dst, unsigned long count)
{
	ioread8_rep(ioport_map(port, 1), dst, count);
}
EXPORT_SYMBOL(insb);

void insw(unsigned long port, void *dst, unsigned long count)
{
	ioread16_rep(ioport_map(port, 2), dst, count);
}
EXPORT_SYMBOL(insw);

void insl(unsigned long port, void *dst, unsigned long count)
{
	ioread32_rep(ioport_map(port, 4), dst, count);
}
EXPORT_SYMBOL(insl);

void outsb(unsigned long port, const void *src, unsigned long count)
{
	iowrite8_rep(ioport_map(port, 1), src, count);
}
EXPORT_SYMBOL(outsb);

void outsw(unsigned long port, const void *src, unsigned long count)
{
	iowrite16_rep(ioport_map(port, 2), src, count);
}
EXPORT_SYMBOL(outsw);

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
	if (port >= 0x100000)
		return __va(port);

	return __va((port << legacy_io_shift) | legacy_io_base);
}
EXPORT_SYMBOL(ioport_map);
