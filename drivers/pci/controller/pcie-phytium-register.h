/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Phytium PCIe Ednpoint controllr driver
 *
 * Copyright (c) 2021-2023, Phytium Technology Co., Ltd.
 */

#ifndef __PCIE_PHYTIUM_REGISTER_H__
#define __PCIE_PHYTIUM_REGISTER_H__

#define	PHYTIUM_PCIE_FUNC_BASE(fn)		(((fn) << 14) & GENMASK(16, 14))
#define	PHYTIUM_PCI_VENDOR_ID			0x98
#define	PHYTIUM_PCI_DEVICE_ID			0x9a
#define	PHYTIUM_PCI_REVISION_ID			0x9c
#define	PHYTIUM_PCI_CLASS_PROG			0x9d
#define	PHYTIUM_PCI_CLASS_DEVICE		0x9e
#define	PHYTIUM_PCI_SUBSYS_VENDOR_ID		0xa0
#define	PHYTIUM_PCI_SUBSYS_DEVICE_ID		0xa2
#define	PHYTIUM_PCI_INTERRUPT_PIN		0xa8
#define	INTERRUPT_PIN_MASK				0x7
#define	MSI_DISABLE					(1 << 3)
#define	MSI_NUM_MASK					(0x7)
#define	MSI_NUM_SHIFT					4
#define	MSI_MASK_SUPPORT				(1 << 7)
#define	PHYTIUM_PCI_MSIX_CAP			0xaa
#define	MSIX_DISABLE				(0 << 15)

#define	PHYTIUM_PCI_BAR_0			0xe4
#define PHYTIUM_PCI_BAR(bar_num)		(0xe4 + bar_num * 4)
#define	BAR_IO_TYPE					(1 << 0)
#define	BAR_MEM_TYPE					(0 << 0)
#define	BAR_MEM_64BIT					(1 << 2)
#define	BAR_MEM_PREFETCHABLE				(1 << 3)
#define	BAR_IO_MIN_APERTURE				4
#define	BAR_MEM_MIN_APERTURE				16


#define	PHYTIUM_PCI_WIN0_BASE			0x600
#define	PHYTIUM_PCI_WIN0_SRC_ADDR0(table)	(PHYTIUM_PCI_WIN0_BASE + 0X20 * table + 0x0)
#define	ATR_IMPL					0x1
#define	ATR_SIZE_MASK					0x3f
#define	ATR_SIZE_SHIFT					1
#define	ATR_SIZE_ALIGN					0x1000
#define	SRC_ADDR_32_12_MASK				0xfffff000

#define	PHYTIUM_PCI_WIN0_SRC_ADDR1(table)	(PHYTIUM_PCI_WIN0_BASE + 0X20 * table + 0x4)
#define	PHYTIUM_PCI_WIN0_TRSL_ADDR0(table)	(PHYTIUM_PCI_WIN0_BASE + 0X20 * table + 0x8)
#define	TRSL_ADDR_32_12_MASK				0xfffff000

#define	PHYTIUM_PCI_WIN0_TRSL_ADDR1(table)	(PHYTIUM_PCI_WIN0_BASE + 0X20 * table + 0xc)
#define	PHYTIUM_PCI_WIN0_TRSL_PARAM(table)	(PHYTIUM_PCI_WIN0_BASE + 0X20 * table + 0x10)
#define	TRSL_ID_IO					0x1
#define	TRSL_ID_MASTER					0x4
#define	TRSL_ID_PCIE_TR				0x0

#define	PHYTIUM_PCI_SLAVE0_BASE			0x800
#define	PHYTIUM_PCI_SLAVE0_SRC_ADDR0(table)	(PHYTIUM_PCI_SLAVE0_BASE + 0X20 * table + 0x0)
#define	PHYTIUM_PCI_SLAVE0_SRC_ADDR1(table)	(PHYTIUM_PCI_SLAVE0_BASE + 0X20 * table + 0x4)
#define	PHYTIUM_PCI_SLAVE0_TRSL_ADDR0(table)	(PHYTIUM_PCI_SLAVE0_BASE + 0X20 * table + 0x8)
#define	PHYTIUM_PCI_SLAVE0_TRSL_ADDR1(table)	(PHYTIUM_PCI_SLAVE0_BASE + 0X20 * table + 0xc)
#define	PHYTIUM_PCI_SLAVE0_TRSL_PARAM(table)	(PHYTIUM_PCI_SLAVE0_BASE + 0X20 * table + 0x10)

#define	PHYTIUM_PCI_CF_MSI_BASE			0x10e0
#define	PHYTIUM_PCI_CF_MSI_CONTROL		0x10e2

#define	PHYTIUM_HPB_C0_PREF_BASE_LIMIT	0xa30
#define C0_PREF_LIMIT_MASK		0xfff
#define C0_PREF_LIMIT_SHIFT		20
#define C0_PREF_BASE_MASK		0xfff
#define C0_PREF_BASE_SHIFT		4
#define C0_PREF_VALUE_SHIFT		20
#define	PHYTIUM_HPB_C0_PREF_BASE_LIMIT_UP32	0xa34
#define C0_PREF_LIMIT_UP32_MASK	0xff
#define C0_PREF_LIMIT_UP32_SHIFT	8
#define C0_PREF_BASE_UP32_MASK	0xff
#define C0_PREF_BASE_UP32_SHIFT	0
#define C0_PREF_UP32_VALUE_SHIFT	0
#endif


