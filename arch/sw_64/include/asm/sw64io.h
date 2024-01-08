/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_SW64IO_H
#define _ASM_SW64_SW64IO_H

#include <asm/io.h>
#include <asm/page.h>

#if defined(CONFIG_UNCORE_XUELANG)
#include <asm/uncore_io_xuelang.h>
#endif

#if defined(CONFIG_UNCORE_JUNZHANG)
#include <asm/uncore_io_junzhang.h>
#endif

#define MK_RC_CFG(nid, idx) \
	(SW64_PCI_IO_BASE((nid), (idx)) | PCI_RC_CFG)
#define MK_PIU_IOR0(nid, idx) \
	(SW64_PCI_IO_BASE((nid), (idx)) | PCI_IOR0_BASE)
#define MK_PIU_IOR1(nid, idx) \
	(SW64_PCI_IO_BASE((nid), (idx)) | PCI_IOR1_BASE)

static inline  unsigned int
read_rc_conf(unsigned long node, unsigned long rc,
		unsigned int offset)
{
	void __iomem *addr;

	addr = __va(MK_RC_CFG(node, rc) | offset);
	return readl(addr);
}

static inline void
write_rc_conf(unsigned long node, unsigned long rc,
		unsigned int offset, unsigned int data)
{
	void __iomem *addr;

	addr = __va(MK_RC_CFG(node, rc) | offset);
	writel(data, addr);
}

static inline  unsigned long
read_piu_ior0(unsigned long node, unsigned long rc,
		unsigned int reg)
{
	void __iomem *addr;

	addr = __va(MK_PIU_IOR0(node, rc) + reg);
	return readq(addr);
}

static inline void
write_piu_ior0(unsigned long node, unsigned long rc,
		unsigned int reg, unsigned long data)
{
	void __iomem *addr;

	addr = __va(MK_PIU_IOR0(node, rc) + reg);
	writeq(data, addr);
}

static inline  unsigned long
read_piu_ior1(unsigned long node, unsigned long rc,
		unsigned int reg)
{
	void __iomem *addr;

	addr = __va(MK_PIU_IOR1(node, rc) + reg);
	return readq(addr);
}

static inline void
write_piu_ior1(unsigned long node, unsigned long rc,
		unsigned int reg, unsigned long data)
{
	void __iomem *addr;

	addr = __va(MK_PIU_IOR1(node, rc) + reg);
	writeq(data, addr);
}

static inline unsigned long
sw64_io_read(unsigned long node, unsigned long reg)
{
	void __iomem *addr;

	addr = __va(SW64_IO_BASE(node) | reg);
	return readq(addr);
}

static inline void
sw64_io_write(unsigned long node, unsigned long reg, unsigned long data)
{
	void __iomem *addr;

	addr = __va(SW64_IO_BASE(node) | reg);
	writeq(data, addr);
}

#if defined(CONFIG_UNCORE_XUELANG)
#include <asm/uncore_io_ops_xuelang.h>
#endif

#if defined(CONFIG_UNCORE_JUNZHANG)
#include <asm/uncore_io_ops_junzhang.h>
#endif

#endif /* _ASM_SW64_SW64IO_H */
