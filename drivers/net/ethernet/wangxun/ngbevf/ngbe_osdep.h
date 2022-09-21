/*******************************************************************************

  WangXun(R) GbE PCI Express Virtual Function Linux Network Driver
  Copyright(c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Software Team <linux.nic@trustnetic.com>
  WangXun Technology, HuaXing Times Square A507, Hangzhou, China.

*******************************************************************************/

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

#ifndef ETH_P_LLDP
#define ETH_P_LLDP                0x88CC
#endif

#ifndef ETH_P_CNM
#define ETH_P_CNM                 0x22E7
#endif

#define DBG

#ifdef DBG
#define hw_dbg(hw, S, A...)     printk(KERN_DEBUG S, ## A)
#else
#define hw_dbg(hw, S, A...)      do {} while (0)
#endif

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

struct ngbevf_hw;
struct ngbe_msg {
	u16 msg_enable;
};
struct net_device *ngbevf_hw_to_netdev(const struct ngbevf_hw *hw);
struct ngbe_msg *ngbevf_hw_to_msg(const struct ngbevf_hw *hw);

extern u16 ngbe_read_pci_cfg_word(struct ngbevf_hw *hw, u32 reg);
extern void ngbe_write_pci_cfg_word(struct ngbevf_hw *hw, u32 reg, u16 value);

/*
 * The following is a brief description of the error categories used by the
 * ERROR_REPORT* macros.
 *
 * - NGBE_ERROR_INVALID_STATE
 * This category is for errors which represent a serious failure state that is
 * unexpected, and could be potentially harmful to device operation. It should
 * not be used for errors relating to issues that can be worked around or
 * ignored.
 *
 * - NGBE_ERROR_POLLING
 * This category is for errors related to polling/timeout issues and should be
 * used in any case where the timeout occured, or a failure to obtain a lock, or
 * failure to receive data within the time limit.
 *
 * - NGBE_ERROR_CAUTION
 * This category should be used for reporting issues that may be the cause of
 * other errors, such as temperature warnings. It should indicate an event which
 * could be serious, but hasn't necessarily caused problems yet.
 *
 * - NGBE_ERROR_SOFTWARE
 * This category is intended for errors due to software state preventing
 * something. The category is not intended for errors due to bad arguments, or
 * due to unsupported features. It should be used when a state occurs which
 * prevents action but is not a serious issue.
 *
 * - NGBE_ERROR_ARGUMENT
 * This category is for when a bad or invalid argument is passed. It should be
 * used whenever a function is called and error checking has detected the
 * argument is wrong or incorrect.
 *
 * - NGBE_ERROR_UNSUPPORTED
 * This category is for errors which are due to unsupported circumstances or
 * configuration issues. It should not be used when the issue is due to an
 * invalid argument, but for when something has occurred that is unsupported
 * (Ex: Flow control autonegotiation or an unsupported SFP+ module.)
 */
enum {
	NGBE_ERROR_SOFTWARE,
	NGBE_ERROR_POLLING,
	NGBE_ERROR_INVALID_STATE,
	NGBE_ERROR_UNSUPPORTED,
	NGBE_ERROR_ARGUMENT,
	NGBE_ERROR_CAUTION,
};

#define ERROR_REPORT(level, format, arg...) do {                                \
	switch (level) {                                                        \
	case NGBE_ERROR_SOFTWARE:                                              \
	case NGBE_ERROR_CAUTION:                                               \
	case NGBE_ERROR_POLLING:                                               \
		netif_warn(ngbevf_hw_to_msg(hw), drv, ngbevf_hw_to_netdev(hw),\
			   format, ## arg);                                     \
		break;                                                          \
	case NGBE_ERROR_INVALID_STATE:                                         \
	case NGBE_ERROR_UNSUPPORTED:                                           \
	case NGBE_ERROR_ARGUMENT:                                              \
		netif_err(ngbevf_hw_to_msg(hw), hw, ngbevf_hw_to_netdev(hw),  \
			  format, ## arg);                                      \
		break;                                                          \
	default:                                                                \
		break;                                                          \
	}                                                                       \
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

#define DPRINTK(nlevel, klevel, fmt, args...) \
	((void)((NETIF_MSG_##nlevel & adapter->msg_enable) && \
	printk(KERN_##klevel "%s: %s@%s: " fmt, \
	ngbe_driver_name, \
	adapter->netdev->name, __FUNCTION__ , ## args)))
#define HWPRINTK(nlevel, klevel, fmt, args...) \
	((void)((NETIF_MSG_##nlevel & *hw->msg_enable) && \
	printk(KERN_##klevel "%s: dev-0x%04x-0x%04x@%s: " fmt, \
	ngbe_driver_name, \
	hw->vendor_id, hw->device_id, __FUNCTION__ , ## args)))

//#define WJDBG 1
#ifdef WJDBG
#define WJPRINTK_LINE(fmt, ...) \
    do { \
	printk(KERN_INFO "%s: [WJDBG]%s@%d: " fmt, \
	       KBUILD_MODNAME, __FUNCTION__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define WJPRINTK_LIMIT(fmt, ...) \
    do { \
	if (unlikely(net_ratelimit())) \
		WJPRINTK_LINE(fmt, ## __VA_ARGS__); \
    } while (0)
#define WJPRINTK_STACK(fmt, ...) \
    do { \
	WJPRINTK_LINE(fmt, ## __VA_ARGS__); \
	dump_stack(); \
    } while (0)
#define WJPRINTK WJPRINTK_LINE
#else /* !WJDBG */
#define WJPRINTK_LINE(fmt, args...) do {} while (0)
#define WJPRINTK_LIMIT(fmt, args...) do {} while (0)
#define WJPRINTK_STACK(fmt, args...) do {} while (0)
#define WJPRINTK(fmt, args...) do {} while (0)
#endif /* WJDBG */

#endif /* _NGBE_OSDEP_H_ */
