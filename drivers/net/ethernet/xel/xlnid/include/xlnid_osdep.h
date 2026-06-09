/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2008 - 2024 Xel Technology. */


/*  glue for the OS independent part of xlnid
		includes register access macros
*/

#ifndef _XLNID_OSDEP_H_
#define _XLNID_OSDEP_H_

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/sched.h>
#include "kcompat.h"

#define XLNID_CPU_TO_BE16(_x) cpu_to_be16(_x)
#define XLNID_BE16_TO_CPU(_x) be16_to_cpu(_x)
#define XLNID_CPU_TO_BE32(_x) cpu_to_be32(_x)
#define XLNID_BE32_TO_CPU(_x) be32_to_cpu(_x)

#define msec_delay(_x) msleep(_x)

#define usec_delay(_x) udelay(_x)

#define STATIC static

#define IOMEM __iomem

#ifdef DBG
#define ASSERT(_x)      BUG_ON(!(_x))
#else
#define ASSERT(_x)      do {} while (0)
#endif

#if 0
#define DEBUGFUNC(S)        printk(KERN_DEBUG S)
#else
#define DEBUGFUNC(S)        do {} while (0)
#endif

#define XLNID_SFP_DETECT_RETRIES    2

struct xlnid_hw;
struct xlnid_msg {
	u16 msg_enable;
};
struct net_device *xlnid_hw_to_netdev(const struct xlnid_hw *hw);
struct xlnid_msg *xlnid_hw_to_msg(const struct xlnid_hw *hw);

#define hw_dbg(hw, format, arg...) \
	netdev_dbg(xlnid_hw_to_netdev(hw), format, ## arg)
#define hw_err(hw, format, arg...) \
	netdev_err(xlnid_hw_to_netdev(hw), format, ## arg)
#define e_dev_info(format, arg...) \
	dev_info(xlnid_pf_to_dev(adapter), format, ## arg)
#define e_dev_warn(format, arg...) \
	dev_warn(xlnid_pf_to_dev(adapter), format, ## arg)
#define e_dev_err(format, arg...) \
	dev_err(xlnid_pf_to_dev(adapter), format, ## arg)
#define e_dev_notice(format, arg...) \
	dev_notice(xlnid_pf_to_dev(adapter), format, ## arg)
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

#define XLNID_DEAD_READ_RETRIES 10
#define XLNID_DEAD_READ_REG 0xdeadbeefU
#define XLNID_FAILED_READ_REG 0xffffffffU
#define XLNID_FAILED_READ_RETRIES 5
#define XLNID_FAILED_READ_CFG_DWORD 0xffffffffU
#define XLNID_FAILED_READ_CFG_WORD 0xffffU
#define XLNID_FAILED_READ_CFG_BYTE 0xffU


#define XLNID_WRITE_REG(h, r, v)   \
	(xlnid_write_reg((h), \
		((h)->mac.type >= xlnid_mac_WESTLAKE ? \
			xlnid_translate_register((h), WESTLAKE_##r) : SKYLAKE_##r), \
		(v), (false)))

#define XLNID_WRITE_REG_DIRECT(h, r, v)   \
	(xlnid_write_reg((h), \
		((h)->mac.type >= xlnid_mac_WESTLAKE ? \
			xlnid_translate_register((h), WESTLAKE_##r) : SKYLAKE_##r), \
		(v), (true)))

#define XLNID_WRITE_REG64(h, r, v)  \
	(xlnid_write_reg64((h), \
		((h)->mac.type >= xlnid_mac_WESTLAKE ? \
			xlnid_translate_register((h), WESTLAKE_##r) : SKYLAKE_##r), \
		(v)))

#define XLNID_WRITE_REG_ARRAY(a, reg, offset, value, direct) \
	(xlnid_write_reg((a), \
		((a)->mac.type >= xlnid_mac_WESTLAKE ? \
			xlnid_translate_register((a), WESTLAKE_##reg) : SKYLAKE_##reg) + \
		((offset) << 2), (value), (direct)))

#define XLNID_READ_REG(h, r)    \
	(xlnid_read_reg((h), \
		((h)->mac.type >= xlnid_mac_WESTLAKE ? \
			xlnid_translate_register((h), WESTLAKE_##r) : SKYLAKE_##r), \
		(false), (false)))

#define XLNID_READ_REG_DIRECT(h, r)    \
	(xlnid_read_reg((h), \
		((h)->mac.type >= xlnid_mac_WESTLAKE ? \
			xlnid_translate_register((h), WESTLAKE_##r) : SKYLAKE_##r), \
		(false), (true)))

#define XLNID_R32_Q(h, r, d) \
	(xlnid_read_reg((h), \
		((h)->mac.type >= xlnid_mac_WESTLAKE ? \
			xlnid_translate_register((h), WESTLAKE_##r) : SKYLAKE_##r), \
		(true), (d)))

#define XLNID_READ_REG_ARRAY(a, reg, offset, direct)    \
	(xlnid_read_reg((a), \
		((a)->mac.type >= xlnid_mac_WESTLAKE ? \
			xlnid_translate_register((a), WESTLAKE_##reg) : SKYLAKE_##reg) + \
		((offset) << 2), (false), (direct)))

#define XLNID_WRITE_REG_MAC(h, r, v)   \
	(xlnid_write_reg((h), (r), (v), (false)))

#define XLNID_READ_REG_MAC(h, r)    \
	(xlnid_read_reg((h), (r), (false), (false)))

#define XLNID_WRITE_REG_MAC_DIRECT(h, r, v) \
	(xlnid_write_reg((h), (r), (v), (true)))

#define XLNID_READ_REG_MAC_DIRECT(h, r)  \
	(xlnid_read_reg((h), (r), (false), (true)))

#ifndef writeq
#define writeq(val, addr)   do { writel((u32) (val), addr); \
						writel((u32) (val >> 32), (addr + 4)); \
				} while (0);
#endif

#define XLNID_WRITE_FLUSH(a) XLNID_READ_REG_DIRECT(a, FLUSH_REG)

extern u32 xlnid_read_reg(struct xlnid_hw *hw, u32 reg, bool quiet,
							bool direct);
extern void xlnid_write_reg(struct xlnid_hw *hw, u32 reg, u32 value,
							bool direct);
extern u32 xlnid_translate_register(struct xlnid_hw *hw, u32 reg);

extern u16 xlnid_read_pci_cfg_word(struct xlnid_hw *hw, u32 reg);
extern void xlnid_write_pci_cfg_word(struct xlnid_hw *hw, u32 reg, u16 value);
extern void ewarn(struct xlnid_hw *hw, const char *str);

#define XLNID_READ_PCIE_WORD xlnid_read_pci_cfg_word
#define XLNID_WRITE_PCIE_WORD xlnid_write_pci_cfg_word
#define XLNID_EEPROM_GRANT_ATTEMPS 100
#define XLNID_HTONL(_i) htonl(_i)
#define XLNID_NTOHL(_i) ntohl(_i)
#define XLNID_NTOHS(_i) ntohs(_i)
#define XLNID_CPU_TO_LE64(_i) cpu_to_le64(_i)
#define XLNID_CPU_TO_LE32(_i) cpu_to_le32(_i)
#define XLNID_CPU_TO_LE16(_i) cpu_to_le16(_i)
#define XLNID_LE16_TO_CPU(_i) le16_to_cpu(_i)
#define XLNID_LE32_TO_CPU(_i) le32_to_cpu(_i)
#define XLNID_LE32_TO_CPUS(_i) le32_to_cpus(_i)
#define XLNID_LE64_TO_CPU(_i) le64_to_cpu(_i)
#define EWARN(H, W) ewarn(H, W)

#undef TRUE
#define TRUE true
#undef FALSE
#define FALSE false

enum {
	XLNID_ERROR_SOFTWARE,
	XLNID_ERROR_POLLING,
	XLNID_ERROR_INVALID_STATE,
	XLNID_ERROR_UNSUPPORTED,
	XLNID_ERROR_ARGUMENT,
	XLNID_ERROR_CAUTION,
};

#define ERROR_REPORT(level, format, arg...) do {                \
	switch (level) {                            \
	case XLNID_ERROR_SOFTWARE:                      \
	case XLNID_ERROR_CAUTION:                       \
	case XLNID_ERROR_POLLING:                       \
		netif_warn(xlnid_hw_to_msg(hw), drv, xlnid_hw_to_netdev(hw),    \
		format, ## arg);                 \
		break;                              \
	case XLNID_ERROR_INVALID_STATE:                     \
	case XLNID_ERROR_UNSUPPORTED:                       \
	case XLNID_ERROR_ARGUMENT:                      \
		netif_err(xlnid_hw_to_msg(hw), hw, xlnid_hw_to_netdev(hw),  \
		format, ## arg);                  \
		break;                              \
	default:                                \
		break;                              \
		}                                   \
		} while (0)

		#define ERROR_REPORT1 ERROR_REPORT
		#define ERROR_REPORT2 ERROR_REPORT
		#define ERROR_REPORT3 ERROR_REPORT

		#define UNREFERENCED_XPARAMETER
		#define UNREFERENCED_1PARAMETER(_p) do {        \
		uninitialized_var(_p);              \
		} while (0)
		#define UNREFERENCED_2PARAMETER(_p, _q) do {        \
		uninitialized_var(_p);              \
		uninitialized_var(_q);              \
		} while (0)
		#define UNREFERENCED_3PARAMETER(_p, _q, _r) do {    \
		uninitialized_var(_p);              \
		uninitialized_var(_q);              \
		uninitialized_var(_r);              \
		} while (0)
		#define UNREFERENCED_4PARAMETER(_p, _q, _r, _s) do {    \
		uninitialized_var(_p);              \
		uninitialized_var(_q);              \
		uninitialized_var(_r);              \
		uninitialized_var(_s);              \
		} while (0)

		#define xlnid_malloc(hw, size) kzalloc(size, GFP_KERNEL)
		#define xlnid_calloc(hw, cnt, size) kcalloc(cnt, size, GFP_KERNEL)
		#define xlnid_free(hw, mptr) kfree(mptr)

		#define xlnid_lock mutex
		#define xlnid_init_lock(lock) mutex_init(lock)
		#define xlnid_destroy_lock(lock) mutex_destroy(lock)
		#define xlnid_acquire_lock(lock) mutex_lock(lock)
		#define xlnid_release_lock(lock) mutex_unlock(lock)

		#endif /* _XLNID_OSDEP_H_ */
