/*
 * WangXun 10 Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * based on ixgbe_osdep.h, Copyright(c) 1999 - 2017 Intel Corporation.
 * Contact Information:
 * Linux NICS <linux.nics@intel.com>
 * e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 */

/* glue for the OS independent part of txgbe
 * includes register access macros
 */

#ifndef _TXGBE_OSDEP_H_
#define _TXGBE_OSDEP_H_

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/sched.h>
#include "kcompat.h"

#define TXGBE_CPU_TO_BE16(_x) cpu_to_be16(_x)
#define TXGBE_BE16_TO_CPU(_x) be16_to_cpu(_x)
#define TXGBE_CPU_TO_BE32(_x) cpu_to_be32(_x)
#define TXGBE_BE32_TO_CPU(_x) be32_to_cpu(_x)

#define msec_delay(_x) msleep(_x)

#define usec_delay(_x) udelay(_x)

#define STATIC static

#define IOMEM __iomem

#define TXGBE_NAME "txgbe"

/* #define DBG 1 */

#define DPRINTK(nlevel, klevel, fmt, args...) \
	((void)((NETIF_MSG_##nlevel & adapter->msg_enable) && \
	printk(KERN_##klevel TXGBE_NAME ": %s: %s: " fmt, \
		adapter->netdev->name, \
		__func__, ## args)))

#ifndef _WIN32
#define txgbe_emerg(fmt, ...)   printk(KERN_EMERG fmt, ## __VA_ARGS__)
#define txgbe_alert(fmt, ...)   printk(KERN_ALERT fmt, ## __VA_ARGS__)
#define txgbe_crit(fmt, ...)    printk(KERN_CRIT fmt, ## __VA_ARGS__)
#define txgbe_error(fmt, ...)   printk(KERN_ERR fmt, ## __VA_ARGS__)
#define txgbe_warn(fmt, ...)    printk(KERN_WARNING fmt, ## __VA_ARGS__)
#define txgbe_notice(fmt, ...)  printk(KERN_NOTICE fmt, ## __VA_ARGS__)
#define txgbe_info(fmt, ...)    printk(KERN_INFO fmt, ## __VA_ARGS__)
#define txgbe_print(fmt, ...)   printk(KERN_DEBUG fmt, ## __VA_ARGS__)
#define txgbe_trace(fmt, ...)   printk(KERN_INFO fmt, ## __VA_ARGS__)
#else /* _WIN32 */
#define txgbe_error(lvl, fmt, ...) \
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, \
		"%s-error: %s@%d, " fmt, \
		"txgbe", __FUNCTION__, __LINE__, ## __VA_ARGS__)
#endif /* !_WIN32 */

#ifdef DBG
#ifndef _WIN32
#define txgbe_debug(fmt, ...) \
	printk(KERN_DEBUG \
		"%s-debug: %s@%d, " fmt, \
		"txgbe", __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else /* _WIN32 */
#define txgbe_debug(fmt, ...) \
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, \
		"%s-debug: %s@%d, " fmt, \
		"txgbe", __FUNCTION__, __LINE__, ## __VA_ARGS__)
#endif /* _WIN32 */
#else /* DBG */
#define txgbe_debug(fmt, ...) do {} while (0)
#endif /* DBG */


#ifdef DBG
#define ASSERT(_x)              BUG_ON(!(_x))
#define DEBUGOUT(S)             printk(KERN_DEBUG S)
#define DEBUGOUT1(S, A...)      printk(KERN_DEBUG S, ## A)
#define DEBUGOUT2(S, A...)      printk(KERN_DEBUG S, ## A)
#define DEBUGOUT3(S, A...)      printk(KERN_DEBUG S, ## A)
#define DEBUGOUT4(S, A...)      printk(KERN_DEBUG S, ## A)
#define DEBUGOUT5(S, A...)      printk(KERN_DEBUG S, ## A)
#define DEBUGOUT6(S, A...)      printk(KERN_DEBUG S, ## A)
#define DEBUGFUNC(fmt, ...)     txgbe_debug(fmt, ## __VA_ARGS__)
#else
#define ASSERT(_x)              do {} while (0)
#define DEBUGOUT(S)             do {} while (0)
#define DEBUGOUT1(S, A...)      do {} while (0)
#define DEBUGOUT2(S, A...)      do {} while (0)
#define DEBUGOUT3(S, A...)      do {} while (0)
#define DEBUGOUT4(S, A...)      do {} while (0)
#define DEBUGOUT5(S, A...)      do {} while (0)
#define DEBUGOUT6(S, A...)      do {} while (0)
#define DEBUGFUNC(fmt, ...)     do {} while (0)
#endif

#define TXGBE_SFP_DETECT_RETRIES        2

struct txgbe_hw;
struct txgbe_msg {
	u16 msg_enable;
};
struct net_device *txgbe_hw_to_netdev(const struct txgbe_hw *hw);
struct txgbe_msg *txgbe_hw_to_msg(const struct txgbe_hw *hw);

#define hw_dbg(hw, format, arg...) \
	netdev_dbg(txgbe_hw_to_netdev(hw), format, ## arg)
#define hw_err(hw, format, arg...) \
	netdev_err(txgbe_hw_to_netdev(hw), format, ## arg)
#define e_dev_info(format, arg...) \
	dev_info(pci_dev_to_dev(adapter->pdev), format, ## arg)
#define e_dev_warn(format, arg...) \
	dev_warn(pci_dev_to_dev(adapter->pdev), format, ## arg)
#define e_dev_err(format, arg...) \
	dev_err(pci_dev_to_dev(adapter->pdev), format, ## arg)
#define e_dev_notice(format, arg...) \
	dev_notice(pci_dev_to_dev(adapter->pdev), format, ## arg)
#define e_dbg(msglvl, format, arg...) \
	netif_dbg(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_info(msglvl, format, arg...) \
	netif_info(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_err(msglvl, format, arg...) \
	netif_err(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_warn(msglvl, format, arg...) \
	netif_warn(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_crit(msglvl, format, arg...) \
	netif_crit(adapter, msglvl, adapter->netdev, format, ## arg)

#define TXGBE_FAILED_READ_CFG_DWORD 0xffffffffU
#define TXGBE_FAILED_READ_CFG_WORD  0xffffU
#define TXGBE_FAILED_READ_CFG_BYTE  0xffU

extern u32 txgbe_read_reg(struct txgbe_hw *hw, u32 reg, bool quiet);
extern u16 txgbe_read_pci_cfg_word(struct txgbe_hw *hw, u32 reg);
extern void txgbe_write_pci_cfg_word(struct txgbe_hw *hw, u32 reg, u16 value);

#define TXGBE_READ_PCIE_WORD txgbe_read_pci_cfg_word
#define TXGBE_WRITE_PCIE_WORD txgbe_write_pci_cfg_word
#define TXGBE_R32_Q(h, r) txgbe_read_reg(h, r, true)

#ifndef writeq
#define writeq(val, addr)       do { writel((u32) (val), addr); \
				     writel((u32) (val >> 32), (addr + 4)); \
				} while (0);
#endif

#define TXGBE_EEPROM_GRANT_ATTEMPS 100
#define TXGBE_HTONL(_i) htonl(_i)
#define TXGBE_NTOHL(_i) ntohl(_i)
#define TXGBE_NTOHS(_i) ntohs(_i)
#define TXGBE_CPU_TO_LE32(_i) cpu_to_le32(_i)
#define TXGBE_LE32_TO_CPUS(_i) le32_to_cpus(_i)

enum {
	TXGBE_ERROR_SOFTWARE,
	TXGBE_ERROR_POLLING,
	TXGBE_ERROR_INVALID_STATE,
	TXGBE_ERROR_UNSUPPORTED,
	TXGBE_ERROR_ARGUMENT,
	TXGBE_ERROR_CAUTION,
};

#define ERROR_REPORT(level, format, arg...) do {                               \
	switch (level) {                                                       \
	case TXGBE_ERROR_SOFTWARE:                                             \
	case TXGBE_ERROR_CAUTION:                                              \
	case TXGBE_ERROR_POLLING:                                              \
		netif_warn(txgbe_hw_to_msg(hw), drv, txgbe_hw_to_netdev(hw),   \
			   format, ## arg);                                    \
		break;                                                         \
	case TXGBE_ERROR_INVALID_STATE:                                        \
	case TXGBE_ERROR_UNSUPPORTED:                                          \
	case TXGBE_ERROR_ARGUMENT:                                             \
		netif_err(txgbe_hw_to_msg(hw), hw, txgbe_hw_to_netdev(hw),     \
			  format, ## arg);                                     \
		break;                                                         \
	default:                                                               \
		break;                                                         \
	}                                                                      \
} while (0)

#define ERROR_REPORT1 ERROR_REPORT
#define ERROR_REPORT2 ERROR_REPORT
#define ERROR_REPORT3 ERROR_REPORT

#define UNREFERENCED_XPARAMETER
#define UNREFERENCED_1PARAMETER(_p) do {                \
	uninitialized_var(_p);                          \
} while (0)
#define UNREFERENCED_2PARAMETER(_p, _q) do {            \
	uninitialized_var(_p);                          \
	uninitialized_var(_q);                          \
} while (0)
#define UNREFERENCED_3PARAMETER(_p, _q, _r) do {        \
	uninitialized_var(_p);                          \
	uninitialized_var(_q);                          \
	uninitialized_var(_r);                          \
} while (0)
#define UNREFERENCED_4PARAMETER(_p, _q, _r, _s) do {    \
	uninitialized_var(_p);                          \
	uninitialized_var(_q);                          \
	uninitialized_var(_r);                          \
	uninitialized_var(_s);                          \
} while (0)
#define UNREFERENCED_PARAMETER(_p) UNREFERENCED_1PARAMETER(_p)

#endif /* _TXGBE_OSDEP_H_ */
