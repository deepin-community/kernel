/*
 * WangXun Gigabit PCI Express Linux driver
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
 */

/* glue for the OS independent part of ngbe
 * includes register access macros
 */

#ifndef _NGBE_OSDEP_H_
#define _NGBE_OSDEP_H_

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/sched.h>
#include "kcompat.h"

#define NGBE_CPU_TO_BE16(_x) cpu_to_be16(_x)
#define NGBE_BE16_TO_CPU(_x) be16_to_cpu(_x)
#define NGBE_CPU_TO_BE32(_x) cpu_to_be32(_x)
#define NGBE_BE32_TO_CPU(_x) be32_to_cpu(_x)

#define msec_delay(_x) msleep(_x)

#define usec_delay(_x) udelay(_x)

#define STATIC static

#define IOMEM __iomem

#define NGBE_NAME "ngbe"

/* #define DBG 1 */

#define DPRINTK(nlevel, klevel, fmt, args...) \
	((void)((NETIF_MSG_##nlevel & adapter->msg_enable) && \
	printk(KERN_##klevel NGBE_NAME ": %s: %s: " fmt, \
		adapter->netdev->name, \
		__func__, ## args)))

#ifndef _WIN32
#define ngbe_emerg(fmt, ...)   printk(KERN_EMERG fmt, ## __VA_ARGS__)
#define ngbe_alert(fmt, ...)   printk(KERN_ALERT fmt, ## __VA_ARGS__)
#define ngbe_crit(fmt, ...)    printk(KERN_CRIT fmt, ## __VA_ARGS__)
#define ngbe_error(fmt, ...)   printk(KERN_ERR fmt, ## __VA_ARGS__)
#define ngbe_warn(fmt, ...)    printk(KERN_WARNING fmt, ## __VA_ARGS__)
#define ngbe_notice(fmt, ...)  printk(KERN_NOTICE fmt, ## __VA_ARGS__)
#define ngbe_info(fmt, ...)    printk(KERN_INFO fmt, ## __VA_ARGS__)
#define ngbe_print(fmt, ...)   printk(KERN_DEBUG fmt, ## __VA_ARGS__)
#define ngbe_trace(fmt, ...)   printk(KERN_INFO fmt, ## __VA_ARGS__)
#else /* _WIN32 */
#define ngbe_error(lvl, fmt, ...) \
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, \
		"%s-error: %s@%d, " fmt, \
		"ngbe", __FUNCTION__, __LINE__, ## __VA_ARGS__)
#endif /* !_WIN32 */

#ifdef DBG
#ifndef _WIN32
#define ngbe_debug(fmt, ...) \
	printk(KERN_DEBUG \
		"%s-debug: %s@%d, " fmt, \
		"ngbe", __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else /* _WIN32 */
#define ngbe_debug(fmt, ...) \
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, \
		"%s-debug: %s@%d, " fmt, \
		"ngbe", __FUNCTION__, __LINE__, ## __VA_ARGS__)
#endif /* _WIN32 */
#else /* DBG */
#define ngbe_debug(fmt, ...) do {} while (0)
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
#define DEBUGFUNC(fmt, ...)     ngbe_debug(fmt, ## __VA_ARGS__)
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

#define NGBE_SFP_DETECT_RETRIES        2

struct ngbe_hw;
struct ngbe_msg {
	u16 msg_enable;
};
struct net_device *ngbe_hw_to_netdev(const struct ngbe_hw *hw);
struct ngbe_msg *ngbe_hw_to_msg(const struct ngbe_hw *hw);

#define hw_dbg(hw, format, arg...) \
	netdev_dbg(ngbe_hw_to_netdev(hw), format, ## arg)
#define hw_err(hw, format, arg...) \
	netdev_err(ngbe_hw_to_netdev(hw), format, ## arg)
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

#define NGBE_FAILED_READ_CFG_DWORD 0xffffffffU
#define NGBE_FAILED_READ_CFG_WORD  0xffffU
#define NGBE_FAILED_READ_CFG_BYTE  0xffU

extern u32 ngbe_read_reg(struct ngbe_hw *hw, u32 reg, bool quiet);
extern u16 ngbe_read_pci_cfg_word(struct ngbe_hw *hw, u32 reg);
extern void ngbe_write_pci_cfg_word(struct ngbe_hw *hw, u32 reg, u16 value);

#define NGBE_READ_PCIE_WORD ngbe_read_pci_cfg_word
#define NGBE_WRITE_PCIE_WORD ngbe_write_pci_cfg_word
#define NGBE_R32_Q(h, r) ngbe_read_reg(h, r, true)

#ifndef writeq
#define writeq(val, addr)       do { writel((u32) (val), addr); \
				     writel((u32) (val >> 32), (addr + 4)); \
				} while (0);
#endif

#define NGBE_EEPROM_GRANT_ATTEMPS 100
#define NGBE_HTONL(_i) htonl(_i)
#define NGBE_NTOHL(_i) ntohl(_i)
#define NGBE_NTOHS(_i) ntohs(_i)
#define NGBE_CPU_TO_LE32(_i) cpu_to_le32(_i)
#define NGBE_LE32_TO_CPUS(_i) le32_to_cpus(_i)

enum {
	NGBE_ERROR_SOFTWARE,
	NGBE_ERROR_POLLING,
	NGBE_ERROR_INVALID_STATE,
	NGBE_ERROR_UNSUPPORTED,
	NGBE_ERROR_ARGUMENT,
	NGBE_ERROR_CAUTION,
};

#define ERROR_REPORT(level, format, arg...) do {                               \
	switch (level) {                                                       \
	case NGBE_ERROR_SOFTWARE:                                             \
	case NGBE_ERROR_CAUTION:                                              \
	case NGBE_ERROR_POLLING:                                              \
		netif_warn(ngbe_hw_to_msg(hw), drv, ngbe_hw_to_netdev(hw),   \
			   format, ## arg);                                    \
		break;                                                         \
	case NGBE_ERROR_INVALID_STATE:                                        \
	case NGBE_ERROR_UNSUPPORTED:                                          \
	case NGBE_ERROR_ARGUMENT:                                             \
		netif_err(ngbe_hw_to_msg(hw), hw, ngbe_hw_to_netdev(hw),     \
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

#endif /* _NGBE_OSDEP_H_ */
