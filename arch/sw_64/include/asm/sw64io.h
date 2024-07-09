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

#if defined(CONFIG_UNCORE_XUELANG)
#include <asm/uncore_io_ops_xuelang.h>
#endif

#if defined(CONFIG_UNCORE_JUNZHANG)
#include <asm/uncore_io_ops_junzhang.h>
#endif

#endif /* _ASM_SW64_SW64IO_H */
