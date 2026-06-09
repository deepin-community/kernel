// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2008 - 2024 Xel Technology. */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>
#include <linux/string.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/pkt_sched.h>
#include <linux/ipv6.h>

#ifdef NETIF_F_TSO
#include <net/checksum.h>
#ifdef NETIF_F_TSO6
#include <net/ipv6.h>
#include <net/ip6_checksum.h>
#endif /* NETIF_F_TSO6 */
#endif /* NETIF_F_TSO */
#ifdef SIOCETHTOOL
#include <linux/ethtool.h>
#endif

#include <linux/if_bridge.h>
#include "xlnid.h"
#include "xlnid_version.h"
#include "xlnid_debug.h"
#ifdef HAVE_XDP_SUPPORT
#include <linux/bpf.h>
#include <linux/bpf_trace.h>
#include <linux/atomic.h>
#endif
#ifdef HAVE_AF_XDP_ZC_SUPPORT
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
#include <net/xdp_sock.h>
#else
#include <net/xdp_sock_drv.h>
#endif
#endif /* HAVE_AF_XDP_ZC_SUPPORT */
#ifdef HAVE_UDP_ENC_RX_OFFLOAD
#include <net/vxlan.h>
#include <net/udp_tunnel.h>
#endif /* HAVE_UDP_ENC_RX_OFFLOAD */
#ifdef HAVE_VXLAN_RX_OFFLOAD
#include <net/vxlan.h>
#endif /* HAVE_VXLAN_RX_OFFLOAD */

#include "xlnid_sriov.h"
#include "xlnid_txrx_common.h"
#include "xlnid_macsec.h"

#define DRV_SUMMARY "Xel-Tech(R) PCI Express Linux Network Driver"

/*  You can refer to below to generate local version:
		#define DRV_VERSION_DESC "."
		#define DRIVER_VENDOR "XEL"
		#define DRIVDR_LOVAL_VERSION "ABC123"
*/
#define DRV_VERSION_DESC " "
#define DRIVER_VENDOR ""
#define DRIVER_LOCAL_VERSION ""

const char xlnid_driver_version[] = DRV_VERSION DRV_VERSION_DESC DRIVER_VENDOR
									DRV_VERSION_DESC DRIVER_LOCAL_VERSION;

#ifdef HAVE_NON_CONST_PCI_DRIVER_NAME
char xlnid_driver_name[] = "xlnid";
#else
const char xlnid_driver_name[] = "xlnid";
#endif
static const char xlnid_driver_string[] = DRV_SUMMARY;
static const char xlnid_copyright[] =
	"Copyright(c) 2008 - 2024 Xel Technology.";
static const char xlnid_overheat_msg[] =
	"Network adapter has been stopped because it has over heated. "
	"Restart the computer. If the problem persists, "
	"power off the system and replace the adapter";

/*  xlnid_pci_tbl - PCI Device ID Table

		Wildcard entries (PCI_ANY_ID) should come last
		Last entry must be all 0s

		{ Vendor ID, Device ID, SubVendor ID, SubDevice ID,
		Class, Class Mask, private data (not used) }
*/
static const struct pci_device_id xlnid_pci_tbl[] = {
	{ PCI_VDEVICE(XEL, XLNID_DEV_ID_WESTLAKE), 0 },
	{ PCI_VDEVICE(XEL, XLNID_DEV_ID_SGMII_MAC), 0 },
	{ PCI_VDEVICE(XEL, XLNID_DEV_ID_1000BASEX), 0 },
	{ PCI_VDEVICE(XEL, XLNID_DEV_ID_SGMII_PHY), 0 },
	/* required last entry */
	{ .device = 0 }
};

MODULE_DEVICE_TABLE(pci, xlnid_pci_tbl);

static void xlnid_watchdog_link_is_down(struct xlnid_adapter *);

MODULE_AUTHOR("Xel Technology");
MODULE_DESCRIPTION(DRV_SUMMARY);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

DEFINE_STATIC_KEY_FALSE(xlnid_xdp_locking_key);
EXPORT_SYMBOL(xlnid_xdp_locking_key);

#define DEFAULT_DEBUG_LEVEL_SHIFT 3

static struct workqueue_struct *xlnid_wq;

//int device_num = 0;
int device_num;
xel_pci_info_t xel_pci_info[16];
static unsigned int fs_major = 123;

xel_pci_info_t *xlnid_find_pci_info(unsigned char bus_number,
									unsigned int devfn)
{
	int i = 0;
	struct pci_dev *pdev;

	for (i = 0; i < device_num; i++) {
		pdev = (struct pci_dev *)xel_pci_info[i].dev;

		if (pdev->bus->number == bus_number && pdev->devfn == devfn) {
			return &xel_pci_info[i];
		}
	}

	return NULL;
}

static void xlnid_debug_clear_stats(unsigned int unit)
{
	struct xlnid_hw *hw = xel_pci_info[unit].hw;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)hw->back;
	struct xlnid_ring *ring = NULL;
	int i = 0;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		ring = adapter->rx_ring[i];
		ring->stats.packets = 0;
		ring->stats.bytes = 0;
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		ring = adapter->tx_ring[i];
		ring->stats.packets = 0;
		ring->stats.bytes = 0;
	}

	return;
}

static void xlnid_debug_rx_stats(unsigned int unit, unsigned int index,
									unsigned int *pkts_hi, unsigned int *pkts_lo,
									unsigned int *octs_hi, unsigned int *octs_lo)
{
	struct xlnid_hw *hw = xel_pci_info[unit].hw;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)hw->back;
	struct xlnid_ring *ring = NULL;
	int i = 0;
	u64 packets = 0;
	u64 bytes = 0;

	*pkts_hi = 0;
	*pkts_lo = 0;
	*octs_hi = 0;
	*octs_lo = 0;

	if (index >= adapter->num_rx_queues) {
		printk("index (%d) larger than num_rx_queues(%d)\r\n", index,
				adapter->num_rx_queues);
		return;
	}

	ring = adapter->rx_ring[index];
	*pkts_hi = ring->stats.packets >> 32;
	*pkts_lo = ring->stats.packets & 0xffffffff;
	*octs_hi = ring->stats.bytes >> 32;
	*octs_lo = ring->stats.bytes & 0xffffffff;

	printk("rx_ring %d packets=%llu bytes=%llu\r\n", index,
			ring->stats.packets, ring->stats.bytes);

	for (i = 0; i < adapter->num_rx_queues; i++) {
		ring = adapter->rx_ring[i];
		packets += ring->stats.packets;
		bytes += ring->stats.bytes;
	}

	printk("rx total packets=%llu bytes=%llu, total=%llu\r\n", packets,
			bytes, (bytes + (packets * 4)));
	//printk("g_recv_eop=%llu\r\n", g_recv_eop);

	return;
}

static void xlnid_debug_tx_stats(unsigned int unit, unsigned int index,
									unsigned int *pkts_hi, unsigned int *pkts_lo,
									unsigned int *octs_hi, unsigned int *octs_lo)
{
	struct xlnid_hw *hw = xel_pci_info[unit].hw;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)hw->back;
	struct xlnid_ring *ring = NULL;
	int i = 0;
	u64 packets = 0;
	u64 bytes = 0;

	*pkts_hi = 0;
	*pkts_lo = 0;
	*octs_hi = 0;
	*octs_lo = 0;

	if (index >= adapter->num_tx_queues) {
		printk("index (%d) larger than num_tx_queues(%d)\r\n", index,
				adapter->num_rx_queues);
		return;
	}

	ring = adapter->tx_ring[index];
	*pkts_hi = ring->stats.packets >> 32;
	*pkts_lo = ring->stats.packets & 0xffffffff;
	*octs_hi = ring->stats.bytes >> 32;
	*octs_lo = ring->stats.bytes & 0xffffffff;

	printk("tx_ring %d packets=%llu bytes=%llu\r\n", index,
			ring->stats.packets, ring->stats.bytes);

	for (i = 0; i < adapter->num_tx_queues; i++) {
		ring = adapter->tx_ring[i];
		packets += ring->stats.packets;
		bytes += ring->stats.bytes;
	}

	printk("tx total packets=%llu bytes=%llu, total=%llu\r\n", packets,
			bytes, (bytes + (packets * 4)));

	return;
}

static int xlnid_debug_ioctl(unsigned int cmd, xel_pipe_t *temp, void *argp)
{
	switch (cmd) {
	case ioctl_xel_get_pci_num: {
		temp->data[0] = device_num;

		if (copy_to_user(argp, temp, sizeof(xel_pipe_t))) {
		return -1;
			}
		} break;

	case ioctl_xel_copy_pci_info: {
		temp->data[0] = xel_pci_info[temp->unit].device_id;
		temp->data[1] = xel_pci_info[temp->unit].vendor_id;
		temp->data[2] = xel_pci_info[temp->unit].pcie_bar0;
		temp->data[3] = xel_pci_info[temp->unit].pcie_function_id;
		temp->data[4] = xel_pci_info[temp->unit].pcie_resource_length;
		temp->data[5] = xel_pci_info[temp->unit].pcie_bar0_end;
		temp->data[6] = xel_pci_info[temp->unit].flag;

		if (copy_to_user(argp, temp, sizeof(xel_pipe_t))) {
			return -1;
		}
	} break;

	case ioctl_xel_pci_read: {
		temp->data[0] = xlnid_read_reg(xel_pci_info[temp->unit].hw,
										temp->addr, false,
										temp->data[1]);

		if (copy_to_user(argp, temp, sizeof(xel_pipe_t))) {
			return -1;
		}
	} break;

	case ioctl_xel_pci_write: {
		xlnid_write_reg(xel_pci_info[temp->unit].hw, temp->addr,
						temp->data[0], temp->data[1]);
	} break;

	case ioctl_xel_clear_stats: {
		xlnid_debug_clear_stats(temp->unit);
	} break;

	case ioctl_xel_rx_stats: {
		xlnid_debug_rx_stats(temp->unit, temp->data[0], &temp->data[1],
								&temp->data[2], &temp->data[3],
								&temp->data[4]);

		if (copy_to_user(argp, temp, sizeof(xel_pipe_t))) {
			return -1;
		}
	} break;

	case ioctl_xel_tx_stats: {
		xlnid_debug_tx_stats(temp->unit, temp->data[0], &temp->data[1],
								&temp->data[2], &temp->data[3],
								&temp->data[4]);

		if (copy_to_user(argp, temp, sizeof(xel_pipe_t))) {
			return -1;
		}
	} break;

	default:
		break;
	}

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
static int xel_ioctrl(struct inode *inode, struct file *filp, unsigned int cmd,
						unsigned long arg)
{
	xel_pipe_t *temp, pct_str;

	temp = &pct_str;
	memset(temp, 0, sizeof(xel_pipe_t));

	if (copy_from_user(temp, (void *)arg, sizeof(xel_pipe_t))) {
		return -1;
	}

	return xlnid_debug_ioctl(cmd, temp, (void *)arg);
}
#else
static long xel_ioctrl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	xel_pipe_t *temp, pct_str;

	temp = &pct_str;
	memset(temp, 0, sizeof(xel_pipe_t));

	if (copy_from_user(temp, (void *)arg, sizeof(xel_pipe_t))) {
		return -1;
	}

	return xlnid_debug_ioctl(cmd, temp, (void *)arg);
}
#endif

static int xel_open(struct inode *node, struct file *file)
{
	return 0;
}

static int xel_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations char_fops = {
open: xel_open,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
ioctl : xel_ioctrl,
#else
unlocked_ioctl : xel_ioctrl,
#endif
release : xel_release,
};

static bool xlnid_check_cfg_remove(struct xlnid_hw *hw, struct pci_dev *pdev);

static int xlnid_read_pci_cfg_word_parent(struct xlnid_hw *hw, u32 reg,
		u16 *value)
{
	struct xlnid_adapter *adapter = hw->back;
	struct pci_dev *parent_dev;
	struct pci_bus *parent_bus;
	int pos;

	parent_bus = adapter->pdev->bus->parent;

	if (!parent_bus) {
		return XLNID_ERR_FEATURE_NOT_SUPPORTED;
	}

	parent_dev = parent_bus->self;

	if (!parent_dev) {
		return XLNID_ERR_FEATURE_NOT_SUPPORTED;
	}

	pos = pci_find_capability(parent_dev, PCI_CAP_ID_EXP);

	if (!pos) {
		return XLNID_ERR_FEATURE_NOT_SUPPORTED;
	}

	pci_read_config_word(parent_dev, pos + reg, value);

	if (*value == XLNID_FAILED_READ_CFG_WORD &&
			xlnid_check_cfg_remove(hw, parent_dev)) {
		return XLNID_ERR_FEATURE_NOT_SUPPORTED;
	}

	return XLNID_SUCCESS;
}

/**
		xlnid_get_parent_bus_info - Set PCI bus info beyond switch
		@hw: pointer to hardware structure

		Sets the PCI bus info (speed, width, type) within the xlnid_hw structure
		when the device is behind a switch.
	**/
static s32 xlnid_get_parent_bus_info(struct xlnid_hw *hw)
{
	u16 link_status = 0;
	int err;

	hw->bus.type = xlnid_bus_type_pci_express;

	/*  Get the negotiated link width and speed from PCI config space of the
		parent, as this device is behind a switch
	*/
	err = xlnid_read_pci_cfg_word_parent(hw, 18, &link_status);

	/* If the read fails, fallback to default */
	if (err) {
		link_status = XLNID_READ_PCIE_WORD(hw, XLNID_PCI_LINK_STATUS);
	}

	xlnid_set_pci_config_data_generic(hw, link_status);

	return XLNID_SUCCESS;
}

/**
		xlnid_check_from_parent - determine whether to use parent for PCIe info
		@hw: hw specific details

		This function is used by probe to determine whether a device's PCIe info
		(speed, width, etc) should be obtained from the parent bus or directly. This
		is useful for specialized device configurations containing PCIe bridges.
*/
static inline bool xlnid_pcie_from_parent(struct xlnid_hw *hw)
{
	return false;
}

static void xlnid_check_minimum_link(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	struct pci_dev *pdev;

	/*  Some devices are not connected over PCIe and thus do not negotiate
		speed. These devices do not have valid bus info, and thus any report
		we generate may not be correct.
	*/
	if (hw->bus.type == xlnid_bus_type_internal) {
		return;
	}

	/* determine whether to use the parent device */
	if (xlnid_pcie_from_parent(&adapter->hw)) {
		pdev = adapter->pdev->bus->parent->self;
	}

	else {
		pdev = adapter->pdev;
	}

	pcie_print_link_status(pdev);
}

/**
		xlnid_enumerate_functions - Get the number of ports this device has
		@adapter: adapter structure

		This function enumerates the phsyical functions co-located on a single slot,
		in order to determine how many ports a device has. This is most useful in
		determining the required GT/s of PCIe bandwidth necessary for optimal
		performance.
	**/
static inline int xlnid_enumerate_functions(struct xlnid_adapter *adapter)
{
	struct pci_dev *entry, *pdev = adapter->pdev;
	int physfns = 0;

	/*  Some cards can not use the generic count PCIe functions method,
		because they are behind a parent switch, so we hardcode these to
		correct number of ports.
	*/
	if (xlnid_pcie_from_parent(&adapter->hw)) {
		physfns = 4;
	}

	else {
		list_for_each_entry(entry, &pdev->bus->devices, bus_list) {
#if 0

			/* don't count virtual functions */
			if (entry->is_virtfn) {
				continue;
			}

#endif

			/*  When the devices on the bus don't all match our device ID,
				we can't reliably determine the correct number of
				functions. This can occur if a function has been direct
				attached to a virtual machine using VT-d, for example. In
				this case, simply return -1 to indicate this.
			*/
			if ((entry->vendor != pdev->vendor) ||
					(entry->device != pdev->device)) {
				return -1;
			}

			physfns++;
		}
	}

	return physfns;
}

static void xlnid_service_event_schedule(struct xlnid_adapter *adapter)
{
	if (!test_bit(__XLNID_DOWN, &adapter->state) &&
			!test_bit(__XLNID_REMOVING, &adapter->state) &&
			!test_and_set_bit(__XLNID_SERVICE_SCHED, &adapter->state)) {
		queue_work(xlnid_wq, &adapter->service_task);
	}
}

static void xlnid_service_event_complete(struct xlnid_adapter *adapter)
{
	BUG_ON(!test_bit(__XLNID_SERVICE_SCHED, &adapter->state));

	/* flush memory to make sure state is correct before next watchog */
	smp_mb__before_atomic();
	clear_bit(__XLNID_SERVICE_SCHED, &adapter->state);
}

static void xlnid_remove_adapter(struct xlnid_hw *hw)
{
	struct xlnid_adapter *adapter = hw->back;

	if (!hw->hw_addr) {
		return;
	}

	hw->hw_addr = NULL;
	e_dev_err("Adapter removed\n");

	if (test_bit(__XLNID_SERVICE_INITED, &adapter->state)) {
		xlnid_service_event_schedule(adapter);
	}
}

static u32 xlnid_check_remove(struct xlnid_hw *hw, u32 reg)
{
	u8 __iomem *reg_addr;
	u32 value;
	int i;

	reg_addr = READ_ONCE(hw->hw_addr);

	if (XLNID_REMOVED(reg_addr)) {
		return XLNID_FAILED_READ_REG;
	}

	/*  Register read of 0xFFFFFFFF can indicate the adapter has been
		removed, so perform several status register reads to determine if
		the adapter has been removed.
	*/
	for (i = 0; i < XLNID_FAILED_READ_RETRIES; ++i) {
		value = readl(reg_addr + 0x0);

		if (value != XLNID_FAILED_READ_REG) {
			break;
		}

		mdelay(3);
	}

	if (value == XLNID_FAILED_READ_REG) {
		xlnid_remove_adapter(hw);
	}

	else {
		value = readl(reg_addr + reg);
	}

	return value;
}

static u32 xlnid_validate_register_read(struct xlnid_hw *_hw, u32 reg,
										bool quiet)
{
	int i;
	u32 value;
	u8 __iomem *reg_addr;
	struct xlnid_adapter *adapter = _hw->back;

	reg_addr = READ_ONCE(_hw->hw_addr);

	if (XLNID_REMOVED(reg_addr)) {
		return XLNID_FAILED_READ_REG;
	}

	for (i = 0; i < XLNID_DEAD_READ_RETRIES; ++i) {
		value = readl(reg_addr + reg);

		if (value != XLNID_DEAD_READ_REG) {
			break;
		}
	}

	if (quiet) {
		return value;
	}

	if (value == XLNID_DEAD_READ_REG) {
		e_err(drv, "%s: register %x read unchanged\n", __func__, reg);
	}

	else
		e_warn(hw, "%s: register %x read recovered after %d retries\n",
				__func__, reg, i + 1);

	return value;
}

static u32 xlnid_read_reg_direct(struct xlnid_hw *hw, u32 reg, bool quiet)
{
	u32 value;
	u8 __iomem *reg_addr;

	reg_addr = READ_ONCE(hw->hw_addr);

	if (XLNID_REMOVED(reg_addr)) {
		return XLNID_FAILED_READ_REG;
	}

	value = readl(reg_addr + reg);

	if (unlikely(value == XLNID_FAILED_READ_REG)) {
		value = xlnid_check_remove(hw, reg);
	}

	if (unlikely(value == XLNID_DEAD_READ_REG)) {
		value = xlnid_validate_register_read(hw, reg, quiet);
	}

	return value;
}

//static uint32_t g_read_reg = 0;
//static uint32_t g_write_reg = 0;
static uint32_t g_read_reg;
static uint32_t g_write_reg;

u32 xlnid_read_reg(struct xlnid_hw *hw, u32 reg, bool quiet, bool direct)
{
	int i;
	bool done = false;
	u32 regval;
	unsigned int trys = XLNID_CMD_TRYS_MAX;

	//printk("xlnid_read_reg: reg = 0x%08x\r\n", reg);

	if ((hw->mac.type == xlnid_mac_SKYLAKE) || direct) {
		/* SKYLAKE registers read direct */
		return xlnid_read_reg_direct(hw, reg, quiet);
	}

	do {
		if (spin_trylock(&hw->indirect_lock)) {
			regval = reg & XLNID_CMD_BUS_ADDR_MASK;
			xlnid_write_reg_direct(hw, WESTLAKE_INDIRECT_CMD_BUS,
									regval);

			/* wait done */
			done = false;

			for (i = 0; i < XLNID_CMD_WAIT_MAX; i++) {
				regval = xlnid_read_reg_direct(
								hw, WESTLAKE_INDIRECT_CMD_BUS, false);

				if (regval & XLNID_CMD_BUS_DONE) {
					done = true;
					break;
				}

				udelay(1);
			}

			if (!done) {
				printk("xlnid_read_reg: not done reg = 0x%08x\r\n",
						reg);
				regval = XLNID_FAILED_READ_REG;
			}

			else {
				regval = xlnid_read_reg_direct(
								hw, WESTLAKE_INDIRECT_RDATA_BUS, quiet);
				g_read_reg = reg;
			}

			spin_unlock(&hw->indirect_lock);
			break;
		}

		udelay(10);
	} while (--trys);

	if (!trys) {
		printk("xlnid_read_reg: trylock failed, reg = 0x%08x, prev_read_reg = 0x%08x, prev_write_reg = 0x%08x\r\n",
				reg, g_read_reg, g_write_reg);
		regval = XLNID_FAILED_READ_REG;
	}

	return regval;
}

void xlnid_write_reg(struct xlnid_hw *hw, u32 reg, u32 value, bool direct)
{
	int i;
	bool done = false;
	u32 regval;
	unsigned int trys = XLNID_CMD_TRYS_MAX;

	if ((hw->mac.type == xlnid_mac_SKYLAKE) || direct) {
		/* SKYLAKE registers write direct */
		return xlnid_write_reg_direct(hw, reg, value);
	}

	g_write_reg = reg;

	do {
		if (spin_trylock(&hw->indirect_lock)) {
			xlnid_write_reg_direct(hw, WESTLAKE_INDIRECT_WDATA_BUS,
									value);

			regval = XLNID_CMD_BUS_WEN |
						(reg & XLNID_CMD_BUS_ADDR_MASK);
			xlnid_write_reg_direct(hw, WESTLAKE_INDIRECT_CMD_BUS,
									regval);

			done = false;

			for (i = 0; i < XLNID_CMD_WAIT_MAX; i++) {
				regval = xlnid_read_reg_direct(
								hw, WESTLAKE_INDIRECT_CMD_BUS, false);

				if (regval & XLNID_CMD_BUS_DONE) {
					done = true;
					break;
				}

				udelay(1);
			}

			if (!done) {
				printk("xlnid_write_reg: not done reg = 0x%08x\r\n",
						reg);
			}

			spin_unlock(&hw->indirect_lock);
			break;
		}

		udelay(10);
	} while (--trys);

	if (!trys) {
		printk("xlnid_write_reg: trylock failed, reg = 0x%08x, prev_read_reg = 0x%08x, prev_write_reg = 0x%08x\r\n",
				reg, g_read_reg, g_write_reg);
	}

	return;
}

u32 xlnid_translate_register(struct xlnid_hw *hw, u32 reg)
{
	return reg;
}

u32 xlnid_read_top_reg(u32 reg)
{
	struct xlnid_hw *hw = xel_pci_info[0].hw;

	if (hw->mac.type != xlnid_mac_SKYLAKE) {
		return XLNID_FAILED_READ_REG;
	}

	return xlnid_read_reg(hw, reg, false, true);
}

void xlnid_write_top_reg(u32 reg, u32 data)
{
	struct xlnid_hw *hw = xel_pci_info[0].hw;

	if (hw->mac.type != xlnid_mac_SKYLAKE) {
		return;
	}

	return xlnid_write_reg(hw, reg, data, true);
}

/*
		xlnid_set_ivar - set the IVAR registers, mapping interrupt causes to vectors
		@adapter: pointer to adapter struct
		@direction: 0 for Rx, 1 for Tx, -1 for other causes
		@queue: queue to map the corresponding interrupt to
		@msix_vector: the vector to map to the corresponding queue

*/
static void xlnid_set_ivar(struct xlnid_adapter *adapter, s8 direction,
							u8 queue, u8 msix_vector)
{
	u32 ivar, index;
	struct xlnid_hw *hw = &adapter->hw;

	if (direction == -1) {
		/* other causes */
		msix_vector |= XLNID_IVAR_ALLOC_VAL;
		index = ((queue & 1) * 8);
		ivar = XLNID_READ_REG_DIRECT(&adapter->hw, IVAR_MISC);
		ivar &= ~(0xFF << index);
		ivar |= (msix_vector << index);
		XLNID_WRITE_REG_DIRECT(&adapter->hw, IVAR_MISC, ivar);
	}

	else {
		/* tx or rx causes */
		msix_vector |= XLNID_IVAR_ALLOC_VAL;
		index = ((16 * (queue & 1)) + (8 * direction));
		ivar = XLNID_READ_REG_DIRECT(hw, IVAR(queue >> 1));
		ivar &= ~(0xFF << index);
		ivar |= (msix_vector << index);
		XLNID_WRITE_REG_DIRECT(hw, IVAR(queue >> 1), ivar);
	}
}

void xlnid_irq_rearm_queues(struct xlnid_adapter *adapter, u64 qmask)
{
	u32 mask;

	mask = (qmask & 0xFFFFFFFF);
	XLNID_WRITE_REG_DIRECT(&adapter->hw, EICS_EX(0), mask);
	mask = (qmask >> 32);
	XLNID_WRITE_REG_DIRECT(&adapter->hw, EICS_EX(1), mask);
}

static u64 xlnid_get_tx_completed(struct xlnid_ring *ring)
{
	return ring->stats.packets;
}

static u64 xlnid_get_tx_pending(struct xlnid_ring *ring)
{
	unsigned int head, tail;

	head = ring->next_to_clean;
	tail = ring->next_to_use;

	return ((head <= tail) ? tail : tail + ring->count) - head;
}

static bool xlnid_check_tx_hang(struct xlnid_ring *tx_ring)
{
	u32 tx_done = xlnid_get_tx_completed(tx_ring);
	u32 tx_done_old = tx_ring->tx_stats.tx_done_old;
	u32 tx_pending = xlnid_get_tx_pending(tx_ring);
	bool ret = false;

	clear_check_for_tx_hang(tx_ring);

	/*
		Check for a hung queue, but be thorough. This verifies
		that a transmit has been completed since the previous
		check AND there is at least one packet pending. The
		ARMED bit is set to indicate a potential hang. The
		bit is cleared if a pause frame is received to remove
		false hang detection due to PFC or 802.3x frames. By
		requiring this to fail twice we avoid races with
		PFC clearing the ARMED bit and conditions where we
		run the check_tx_hang logic with a transmit completion
		pending but without time to complete it yet.
	*/
	if ((tx_done_old == tx_done) && tx_pending) {
		/* make sure it is true for two checks in a row */
		ret = test_and_set_bit(__XLNID_HANG_CHECK_ARMED,
								&tx_ring->state);
	}

	else {
		/* update completed stats and continue */
		tx_ring->tx_stats.tx_done_old = tx_done;
		/* reset the countdown */
		clear_bit(__XLNID_HANG_CHECK_ARMED, &tx_ring->state);
	}

	return ret;
}

/**
		xlnid_tx_timeout_reset - initiate reset due to Tx timeout
		@adapter: driver private struct
	**/
static void xlnid_tx_timeout_reset(struct xlnid_adapter *adapter)
{
	/* Do the reset outside of interrupt context */
	if (!test_bit(__XLNID_DOWN, &adapter->state)) {
		set_bit(__XLNID_RESET_REQUESTED, &adapter->state);
		xlnid_service_event_schedule(adapter);
	}
}

/**
		xlnid_tx_timeout - Respond to a Tx Hang
		@netdev: network interface device structure
		@txqueue: specific tx queue
	**/
#ifdef HAVE_TX_TIMEOUT_TXQUEUE
static void xlnid_tx_timeout(struct net_device *netdev, unsigned int txqueue)
#else
static void xlnid_tx_timeout(struct net_device *netdev)
#endif
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	bool real_tx_hang = false;
	int i;

#define TX_TIMEO_LIMIT 16000

	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct xlnid_ring *tx_ring = adapter->tx_ring[i];

		if (check_for_tx_hang(tx_ring) && xlnid_check_tx_hang(tx_ring)) {
			real_tx_hang = true;
		}
	}

	if (real_tx_hang) {
		xlnid_tx_timeout_reset(adapter);
	}

	else {
		e_info(drv,
				"Fake Tx hang detected with timeout of %d "
				"seconds\n",
				netdev->watchdog_timeo / HZ);

		/* fake Tx hang - increase the kernel timeout */
		if (netdev->watchdog_timeo < TX_TIMEO_LIMIT) {
			netdev->watchdog_timeo *= 2;
		}
	}
}

/**
		xlnid_reset_pf_report - reset pf and print reset report
		@tx_ring: tx ring number
		@next: next ring

		Prints a message containing details about the tx hang.
		Then reset the current PF adapter.
*/
static void xlnid_reset_pf_report(struct xlnid_ring *tx_ring, unsigned int next)
{
	struct xlnid_adapter *adapter = netdev_priv(tx_ring->netdev);
	struct xlnid_hw *hw = &adapter->hw;

	e_err(drv,
			"Detected Tx Unit Hang%s\n"
			"  Tx Queue             <%d>\n"
			"  TDH, TDT             <%x>, <%x>\n"
			"  next_to_use          <%x>\n"
			"  next_to_clean        <%x>\n"
			"tx_buffer_info[next_to_clean]\n"
			"  time_stamp           <%lx>\n"
			"  jiffies              <%lx>\n",
			ring_is_xdp(tx_ring) ? " (XDP)" : "", tx_ring->queue_index,
			XLNID_READ_REG_DIRECT(hw, TDH(tx_ring->reg_idx)),
			XLNID_READ_REG_DIRECT(hw, TDT(tx_ring->reg_idx)),
			tx_ring->next_to_use, next,
			tx_ring->tx_buffer_info[next].time_stamp, jiffies);

	if (!ring_is_xdp(tx_ring)) {
		netif_stop_subqueue(tx_ring->netdev, tx_ring->queue_index);
	}
}

/**
		xlnid_clean_tx_irq - Reclaim resources after transmit completes
		@q_vector: structure containing interrupt and ring information
		@tx_ring: tx ring to clean
		@napi_budget: Used to determine if we are in netpoll
	**/
static bool xlnid_clean_tx_irq(struct xlnid_q_vector *q_vector,
								struct xlnid_ring *tx_ring, int napi_budget)
{
	struct xlnid_adapter *adapter = q_vector->adapter;
	struct xlnid_tx_buffer *tx_buffer;
	union xlnid_adv_tx_desc *tx_desc;
	unsigned int total_bytes = 0, total_packets = 0;
	unsigned int budget = q_vector->tx.work_limit;
	unsigned int i = tx_ring->next_to_clean;

	if (test_bit(__XLNID_DOWN, &adapter->state)) {
		return true;
	}

	tx_buffer = &tx_ring->tx_buffer_info[i];
	tx_desc = XLNID_TX_DESC(tx_ring, i);
	i -= tx_ring->count;

	do {
		union xlnid_adv_tx_desc *eop_desc = tx_buffer->next_to_watch;

		/* if next_to_watch is not set then there is no work pending */
		if (!eop_desc) {
			break;
		}

		/* prevent any other reads prior to eop_desc */
		smp_rmb();

		/* if DD is not set pending work has not been completed */
		if (!(eop_desc->wb.status & cpu_to_le32(XLNID_TXD_STAT_DD))) {
			break;
		}

		/* clear next_to_watch to prevent false hangs */
		tx_buffer->next_to_watch = NULL;

		/* update the statistics for this packet */
		total_bytes += tx_buffer->bytecount;
		total_packets += tx_buffer->gso_segs;

		/* free the skb */
#ifdef HAVE_XDP_SUPPORT

		if (ring_is_xdp(tx_ring))
#ifdef HAVE_XDP_FRAME_STRUCT
			xdp_return_frame(tx_buffer->xdpf);

#else
			page_frag_free(tx_buffer->data);
#endif
		else {
			napi_consume_skb(tx_buffer->skb, napi_budget);
		}

#else
		napi_consume_skb(tx_buffer->skb, napi_budget);
#endif

		/* unmap skb header data */
		dma_unmap_single(tx_ring->dev, dma_unmap_addr(tx_buffer, dma),
							dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);

		/* clear tx_buffer data */
		dma_unmap_len_set(tx_buffer, len, 0);

		/* unmap remaining buffers */
		while (tx_desc != eop_desc) {
			tx_buffer++;
			tx_desc++;
			i++;

			if (unlikely(!i)) {
				i -= tx_ring->count;
				tx_buffer = tx_ring->tx_buffer_info;
				tx_desc = XLNID_TX_DESC(tx_ring, 0);
			}

			/* unmap any remaining paged data */
			if (dma_unmap_len(tx_buffer, len)) {
				dma_unmap_page(tx_ring->dev,
								dma_unmap_addr(tx_buffer, dma),
								dma_unmap_len(tx_buffer, len),
								DMA_TO_DEVICE);
				dma_unmap_len_set(tx_buffer, len, 0);
			}
		}

		/* move us one more past the eop_desc for start of next pkt */
		tx_buffer++;
		tx_desc++;
		i++;

		if (unlikely(!i)) {
			i -= tx_ring->count;
			tx_buffer = tx_ring->tx_buffer_info;
			tx_desc = XLNID_TX_DESC(tx_ring, 0);
		}

		/* issue prefetch for next Tx descriptor */
		prefetch(tx_desc);

		/* update budget accounting */
		budget--;
	} while (likely(budget));

	i += tx_ring->count;
	tx_ring->next_to_clean = i;
	u64_stats_update_begin(&tx_ring->syncp);
	tx_ring->stats.bytes += total_bytes;
	tx_ring->stats.packets += total_packets;
	u64_stats_update_end(&tx_ring->syncp);
	q_vector->tx.total_bytes += total_bytes;
	q_vector->tx.total_packets += total_packets;

	if (check_for_tx_hang(tx_ring) && xlnid_check_tx_hang(tx_ring)) {
		/* reset PF */
		xlnid_reset_pf_report(tx_ring, i);

		e_info(probe,
				"tx hang %d detected on queue %d, resetting adapter\n",
				adapter->tx_timeout_count + 1, tx_ring->queue_index);

		xlnid_tx_timeout_reset(adapter);

		/* the adapter is about to reset, no point in enabling stuff */
		return true;
	}

	if (ring_is_xdp(tx_ring)) {
		return !!budget;
	}

	netdev_tx_completed_queue(txring_txq(tx_ring), total_packets,
								total_bytes);

#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)

	if (unlikely(total_packets && netif_carrier_ok(netdev_ring(tx_ring)) &&
					(xlnid_desc_unused(tx_ring) >= TX_WAKE_THRESHOLD))) {
		/*  Make sure that anybody stopping the queue after this
			sees the new next_to_clean.
		*/
		smp_mb();
#ifdef HAVE_TX_MQ

		if (__netif_subqueue_stopped(netdev_ring(tx_ring),
										ring_queue_index(tx_ring)) &&
				!test_bit(__XLNID_DOWN, &q_vector->adapter->state)) {
			netif_wake_subqueue(netdev_ring(tx_ring),
								ring_queue_index(tx_ring));
			++tx_ring->tx_stats.restart_queue;
		}

#else

		if (netif_queue_stopped(netdev_ring(tx_ring)) &&
				!test_bit(__XLNID_DOWN, &q_vector->adapter->state)) {
			netif_wake_queue(netdev_ring(tx_ring));
			++tx_ring->tx_stats.restart_queue;
		}

#endif
	}

	return !!budget;
}

#ifdef NETIF_F_RXHASH
#define XLNID_RSS_L4_TYPES_MASK                      \
	((1ul << XLNID_RXDADV_RSSTYPE_IPV4_TCP) |    \
		(1ul << XLNID_RXDADV_RSSTYPE_IPV4_UDP) |    \
		(1ul << XLNID_RXDADV_RSSTYPE_IPV6_TCP) |    \
		(1ul << XLNID_RXDADV_RSSTYPE_IPV6_UDP) |    \
		(1ul << XLNID_RXDADV_RSSTYPE_IPV6_TCP_EX) | \
		(1ul << XLNID_RXDADV_RSSTYPE_IPV6_UDP_EX))

static inline void xlnid_rx_hash(struct xlnid_ring *ring,
									union xlnid_adv_rx_desc *rx_desc,
									struct sk_buff *skb)
{
	u16 rss_type;

	if (!(netdev_ring(ring)->features & NETIF_F_RXHASH)) {
		return;
	}

	rss_type = le16_to_cpu(rx_desc->wb.lower.lo_dword.hs_rss.pkt_info) &
				XLNID_RXDADV_RSSTYPE_MASK;

	if (!rss_type) {
		return;
	}

	skb_set_hash(skb, le32_to_cpu(rx_desc->wb.lower.hi_dword.rss),
					(XLNID_RSS_L4_TYPES_MASK & (1ul << rss_type)) ?
					PKT_HASH_TYPE_L4 :
					PKT_HASH_TYPE_L3);
}
#endif /* NETIF_F_RXHASH */

/**
		xlnid_rx_checksum - indicate in skb if hw indicated a good cksum
		@ring: structure containing ring specific data
		@rx_desc: current Rx descriptor being processed
		@skb: skb currently being received and modified
	**/
static inline void xlnid_rx_checksum(struct xlnid_ring *ring,
										union xlnid_adv_rx_desc *rx_desc,
										struct sk_buff *skb)
{
	__le16 pkt_info = rx_desc->wb.lower.lo_dword.hs_rss.pkt_info;
	bool encap_pkt = false;

	skb_checksum_none_assert(skb);

	/* Rx csum disabled */
	if (!(netdev_ring(ring)->features & NETIF_F_RXCSUM)) {
		return;
	}

	/* check for VXLAN or Geneve packet type */
	if (pkt_info & cpu_to_le16(XLNID_RXDADV_PKTTYPE_VXLAN)) {
		encap_pkt = true;
#if defined(HAVE_UDP_ENC_RX_OFFLOAD) || defined(HAVE_VXLAN_RX_OFFLOAD)
		skb->encapsulation = 1;
#endif /* HAVE_UDP_ENC_RX_OFFLOAD || HAVE_VXLAN_RX_OFFLOAD */
		skb->ip_summed = CHECKSUM_NONE;
	}

	/* if IP and error */
	if (xlnid_test_staterr(rx_desc, XLNID_RXD_STAT_IPCS) &&
			xlnid_test_staterr(rx_desc, XLNID_RXDADV_ERR_IPE)) {
		ring->rx_stats.csum_err++;
		return;
	}

	if (!xlnid_test_staterr(rx_desc, XLNID_RXD_STAT_L4CS)) {
		return;
	}

	if (xlnid_test_staterr(rx_desc, XLNID_RXDADV_ERR_TCPE)) {
		/*
			errata, UDP frames with a 0 checksum can be marked as
			checksum errors.
		*/
		if ((pkt_info & cpu_to_le16(XLNID_RXDADV_PKTTYPE_UDP)) &&
				test_bit(__XLNID_RX_CSUM_UDP_ZERO_ERR, &ring->state)) {
			return;
		}

		ring->rx_stats.csum_err++;
		return;
	}

	/* It must be a TCP or UDP packet with a valid checksum */
	skb->ip_summed = CHECKSUM_UNNECESSARY;

	if (encap_pkt) {
		if (!xlnid_test_staterr(rx_desc, XLNID_RXD_STAT_OUTERIPCS)) {
			return;
		}

		if (xlnid_test_staterr(rx_desc, XLNID_RXDADV_ERR_OUTERIPER)) {
			skb->ip_summed = CHECKSUM_NONE;
			return;
		}

#ifdef HAVE_SKBUFF_CSUM_LEVEL
		/* If we checked the outer header let the stack know */
		skb->csum_level = 1;
#endif /* HAVE_SKBUFF_CSUM_LEVEL */
	}
}

static inline void xlnid_release_rx_desc(struct xlnid_ring *rx_ring, u32 val)
{
	rx_ring->next_to_use = val;
#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT

	/* update next to alloc since we have filled the ring */
	rx_ring->next_to_alloc = val;
#endif
	/*
		Force memory writes to complete before letting h/w
		know there are new descriptors to fetch.  (Only
		applicable for weak-ordered memory model archs,
		such as IA-64).
	*/
	wmb();
	writel(val, rx_ring->tail);
}

#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
static bool xlnid_alloc_mapped_skb(struct xlnid_ring *rx_ring,
									struct xlnid_rx_buffer *bi)
{
	struct sk_buff *skb = bi->skb;
	dma_addr_t dma = bi->dma;

	if (unlikely(dma)) {
		return true;
	}

	if (likely(!skb)) {
		skb = netdev_alloc_skb_ip_align(netdev_ring(rx_ring),
										rx_ring->rx_buf_len);

		if (unlikely(!skb)) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			return false;
		}

		bi->skb = skb;
	}

	dma = dma_map_single(rx_ring->dev, skb->data, rx_ring->rx_buf_len,
							DMA_FROM_DEVICE);

	/*
		if mapping failed free memory back to system since
		there isn't much point in holding memory we can't use
	*/
	if (dma_mapping_error(rx_ring->dev, dma)) {
		dev_kfree_skb_any(skb);
		bi->skb = NULL;

		rx_ring->rx_stats.alloc_rx_buff_failed++;
		return false;
	}

	bi->dma = dma;
	return true;
}

#else /* !CONFIG_XLNID_DISABLE_PACKET_SPLIT */
static inline unsigned int xlnid_rx_offset(struct xlnid_ring *rx_ring)
{
	return ring_uses_build_skb(rx_ring) ? XLNID_SKB_PAD : 0;
}

static bool xlnid_alloc_mapped_page(struct xlnid_ring *rx_ring,
									struct xlnid_rx_buffer *bi)
{
	struct page *page = bi->page;
	dma_addr_t dma;
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
	DEFINE_DMA_ATTRS(attrs);

	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page)) {
		return true;
	}

	/* alloc new page for storage */
	page = dev_alloc_pages(xlnid_rx_pg_order(rx_ring));

	if (unlikely(!page)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	/* map page for use */
	dma = dma_map_page_attrs(rx_ring->dev, page, 0,
								xlnid_rx_pg_size(rx_ring), DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
								&attrs);
#else
								XLNID_RX_DMA_ATTR);
#endif

	/*
		if mapping failed free memory back to system since
		there isn't much point in holding memory we can't use
	*/
	if (dma_mapping_error(rx_ring->dev, dma)) {
		__free_pages(page, xlnid_rx_pg_order(rx_ring));

		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	bi->dma = dma;
	bi->page = page;
	bi->page_offset = xlnid_rx_offset(rx_ring);
#ifdef HAVE_PAGE_COUNT_BULK_UPDATE
	page_ref_add(page, USHRT_MAX - 1);
	bi->pagecnt_bias = USHRT_MAX;
#else
	bi->pagecnt_bias = 1;
#endif
	rx_ring->rx_stats.alloc_rx_page++;

	return true;
}

#endif /* !CONFIG_XLNID_DISABLE_PACKET_SPLIT */
/**
		xlnid_alloc_rx_buffers - Replace used receive buffers
		@rx_ring: ring to place buffers on
		@cleaned_count: number of buffers to replace
	**/
void xlnid_alloc_rx_buffers(struct xlnid_ring *rx_ring, u16 cleaned_count)
{
	union xlnid_adv_rx_desc *rx_desc;
	struct xlnid_rx_buffer *bi;
	u16 i = rx_ring->next_to_use;
#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	u16 bufsz;
#endif

	/* nothing to do */
	if (!cleaned_count) {
		return;
	}

	rx_desc = XLNID_RX_DESC(rx_ring, i);
	bi = &rx_ring->rx_buffer_info[i];
	i -= rx_ring->count;
#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT

	bufsz = xlnid_rx_bufsz(rx_ring);
#endif

	do {
#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT

		if (!xlnid_alloc_mapped_skb(rx_ring, bi)) {
			break;
		}

#else

		if (!xlnid_alloc_mapped_page(rx_ring, bi)) {
			break;
		}

		/* sync the buffer for use by the device */
		dma_sync_single_range_for_device(rx_ring->dev, bi->dma,
											bi->page_offset, bufsz,
											DMA_FROM_DEVICE);
#endif
		/*
			Refresh the desc even if buffer_addrs didn't change
			because each write-back erases this info.
		*/
#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
		rx_desc->read.pkt_addr = cpu_to_le64(bi->dma);
#else
		rx_desc->read.pkt_addr = cpu_to_le64(bi->dma + bi->page_offset);
#endif

		rx_desc++;
		bi++;
		i++;

		if (unlikely(!i)) {
			rx_desc = XLNID_RX_DESC(rx_ring, 0);
			bi = rx_ring->rx_buffer_info;
			i -= rx_ring->count;
		}

		/* clear the length for the next_to_use descriptor */
		rx_desc->wb.upper.length = 0;

		cleaned_count--;
	} while (cleaned_count);

	i += rx_ring->count;

	if (rx_ring->next_to_use != i) {
		xlnid_release_rx_desc(rx_ring, i);
	}
}

#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
/**
		xlnid_merge_active_tail - merge active tail into lro skb
		@tail: pointer to active tail in frag_list

		This function merges the length and data of an active tail into the
		skb containing the frag_list.  It resets the tail's pointer to the head,
		but it leaves the heads pointer to tail intact.
	**/
static inline struct sk_buff *xlnid_merge_active_tail(struct sk_buff *tail)
{
	struct sk_buff *head = XLNID_CB(tail)->head;

	if (!head) {
		return tail;
	}

	head->len += tail->len;
	head->data_len += tail->len;
	head->truesize += tail->truesize;

	XLNID_CB(tail)->head = NULL;

	return head;
}

/**
		xlnid_add_active_tail - adds an active tail into the skb frag_list
		@head: pointer to the start of the skb
		@tail: pointer to active tail to add to frag_list

		This function adds an active tail to the end of the frag list.  This tail
		will still be receiving data so we cannot yet ad it's stats to the main
		skb.  That is done via xlnid_merge_active_tail.
	**/
static inline void xlnid_add_active_tail(struct sk_buff *head,
		struct sk_buff *tail)
{
	struct sk_buff *old_tail = XLNID_CB(head)->tail;

	if (old_tail) {
		xlnid_merge_active_tail(old_tail);
		old_tail->next = tail;
	}

	else {
		skb_shinfo(head)->frag_list = tail;
	}

	XLNID_CB(tail)->head = head;
	XLNID_CB(head)->tail = tail;
}

/**
		xlnid_close_active_frag_list - cleanup pointers on a frag_list skb
		@head: pointer to head of an active frag list

		This function will clear the frag_tail_tracker pointer on an active
		frag_list and returns true if the pointer was actually set
	**/
static inline bool xlnid_close_active_frag_list(struct sk_buff *head)
{
	struct sk_buff *tail = XLNID_CB(head)->tail;

	if (!tail) {
		return false;
	}

	xlnid_merge_active_tail(tail);

	XLNID_CB(head)->tail = NULL;

	return true;
}

#endif
#ifdef HAVE_VLAN_RX_REGISTER
/**
		xlnid_receive_skb - Send a completed packet up the stack
		@q_vector: structure containing interrupt and ring information
		@skb: packet to send up
	**/
static void xlnid_receive_skb(struct xlnid_q_vector *q_vector,
								struct sk_buff *skb)
{
	u16 vlan_tag = XLNID_CB(skb)->vid;

#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)

	if (vlan_tag & VLAN_VID_MASK) {
		/* by placing vlgrp at start of structure we can alias it */
		struct vlan_group **vlgrp = netdev_priv(skb->dev);

		if (!*vlgrp) {
			dev_kfree_skb_any(skb);
		}

		else if (q_vector->netpoll_rx) {
			vlan_hwaccel_rx(skb, *vlgrp, vlan_tag);
		}

		else
			vlan_gro_receive(&q_vector->napi, *vlgrp, vlan_tag,
								skb);
	}

	else {
#endif /* NETIF_F_HW_VLAN_TX || NETIF_F_HW_VLAN_CTAG_TX */

		if (q_vector->netpoll_rx) {
			netif_rx(skb);
		}

		else {
			napi_gro_receive(&q_vector->napi, skb);
		}

#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	}

#endif /* NETIF_F_HW_VLAN_TX || NETIF_F_HW_VLAN_CTAG_TX */
}

#endif /* HAVE_VLAN_RX_REGISTER */
#ifdef NETIF_F_GSO
static void xlnid_set_rsc_gso_size(struct xlnid_ring __maybe_unused *ring,
									struct sk_buff *skb)
{
	u16 hdr_len = eth_get_headlen(skb->dev, skb->data, skb_headlen(skb));

	/* set gso_size to avoid messing up TCP MSS */
	skb_shinfo(skb)->gso_size =
		DIV_ROUND_UP((skb->len - hdr_len), XLNID_CB(skb)->append_cnt);
	skb_shinfo(skb)->gso_type = SKB_GSO_TCPV4;
}

#endif /* NETIF_F_GSO */
static void xlnid_update_rsc_stats(struct xlnid_ring *rx_ring,
									struct sk_buff *skb)
{
	/* if append_cnt is 0 then frame is not RSC */
	if (!XLNID_CB(skb)->append_cnt) {
		return;
	}

	rx_ring->rx_stats.rsc_count += XLNID_CB(skb)->append_cnt;
	rx_ring->rx_stats.rsc_flush++;

#ifdef NETIF_F_GSO
	xlnid_set_rsc_gso_size(rx_ring, skb);

#endif
	/* gso_size is computed using append_cnt so always clear it last */
	XLNID_CB(skb)->append_cnt = 0;
}

static void xlnid_rx_vlan(struct xlnid_ring *ring,
							union xlnid_adv_rx_desc *rx_desc, struct sk_buff *skb)
{
/*#ifdef NETIF_F_HW_VLAN_CTAG_RX
	if ((netdev_ring(ring)->features & NETIF_F_HW_VLAN_CTAG_RX) &&
#else
	if ((netdev_ring(ring)->features & NETIF_F_HW_VLAN_RX) &&
#endif*/
	if ((netdev_ring(ring)->features & NETIF_F_HW_VLAN_CTAG_RX) &&
			xlnid_test_staterr(rx_desc, XLNID_RXD_STAT_VP))
#ifndef HAVE_VLAN_RX_REGISTER
		__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
								le16_to_cpu(rx_desc->wb.upper.vlan));
#else
		XLNID_CB(skb)->vid = le16_to_cpu(rx_desc->wb.upper.vlan);
	else {
		XLNID_CB(skb)->vid = 0;
	}
#endif
}

/**
		xlnid_process_skb_fields - Populate skb header fields from Rx descriptor
		@rx_ring: rx descriptor ring packet is being transacted on
		@rx_desc: pointer to the EOP Rx descriptor
		@skb: pointer to current skb being populated

		This function checks the ring, descriptor, and packet information in
		order to populate the hash, checksum, VLAN, timestamp, protocol, and
		other fields within the skb.
	**/
void xlnid_process_skb_fields(struct xlnid_ring *rx_ring,
								union xlnid_adv_rx_desc *rx_desc,
								struct sk_buff *skb)
{
#ifdef HAVE_PTP_1588_CLOCK
	u32 flags = rx_ring->q_vector->adapter->flags;

#endif
	xlnid_update_rsc_stats(rx_ring, skb);

#ifdef NETIF_F_RXHASH
	xlnid_rx_hash(rx_ring, rx_desc, skb);

#endif /* NETIF_F_RXHASH */
	xlnid_rx_checksum(rx_ring, rx_desc, skb);
#ifdef HAVE_PTP_1588_CLOCK

	if (unlikely(flags & XLNID_FLAG_RX_HWTSTAMP_ENABLED)) {
		xlnid_ptp_rx_hwtstamp(rx_ring, rx_desc, skb);
	}

#endif
	xlnid_rx_vlan(rx_ring, rx_desc, skb);

	skb_record_rx_queue(skb, ring_queue_index(rx_ring));

	skb->protocol = eth_type_trans(skb, netdev_ring(rx_ring));
}

void xlnid_rx_skb(struct xlnid_q_vector *q_vector, struct xlnid_ring *rx_ring,
					union xlnid_adv_rx_desc *rx_desc, struct sk_buff *skb)
{
#ifdef HAVE_NDO_BUSY_POLL
	skb_mark_napi_id(skb, &q_vector->napi);

	if (xlnid_qv_busy_polling(q_vector) || q_vector->netpoll_rx) {
		netif_receive_skb(skb);
		/* exit early if we busy polled */
		return;
	}

#endif

#ifdef HAVE_VLAN_RX_REGISTER
	xlnid_receive_skb(q_vector, skb);
#else
	napi_gro_receive(&q_vector->napi, skb);
#endif
#ifndef NETIF_F_GRO

	netdev_ring(rx_ring)->last_rx = jiffies;
#endif
}

/**
		xlnid_is_non_eop - process handling of non-EOP buffers
		@rx_ring: Rx ring being processed
		@rx_desc: Rx descriptor for current buffer
		@skb: Current socket buffer containing buffer in progress

		This function updates next to clean.  If the buffer is an EOP buffer
		this function exits returning false, otherwise it will place the
		sk_buff in the next buffer to be chained and return true indicating
		that this is in fact a non-EOP buffer.
	**/
static bool xlnid_is_non_eop(struct xlnid_ring *rx_ring,
								union xlnid_adv_rx_desc *rx_desc,
								struct sk_buff *skb)
{
#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	struct sk_buff *next_skb;
#endif
	u32 ntc = rx_ring->next_to_clean + 1;

	/* fetch, update, and store next to clean */
	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;

	prefetch(XLNID_RX_DESC(rx_ring, ntc));

	/* update RSC append count if present */
	if (ring_is_rsc_enabled(rx_ring)) {
		__le32 rsc_enabled = rx_desc->wb.lower.lo_dword.data &
								cpu_to_le32(XLNID_RXDADV_RSCCNT_MASK);

		if (unlikely(rsc_enabled)) {
			u32 rsc_cnt = le32_to_cpu(rsc_enabled);

			rsc_cnt >>= XLNID_RXDADV_RSCCNT_SHIFT;
			XLNID_CB(skb)->append_cnt += rsc_cnt - 1;

			/* update ntc based on RSC value */
			ntc = le32_to_cpu(rx_desc->wb.upper.status_error);
			ntc &= XLNID_RXDADV_NEXTP_MASK;
			ntc >>= XLNID_RXDADV_NEXTP_SHIFT;
		}
	}

	/* if we are the last buffer then there is nothing else to do */
	if (likely(xlnid_test_staterr(rx_desc, XLNID_RXD_STAT_EOP))) {
		return false;
	}

	/* place skb in next buffer to be received */
#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	next_skb = rx_ring->rx_buffer_info[ntc].skb;

	xlnid_add_active_tail(skb, next_skb);
	XLNID_CB(next_skb)->head = skb;
#else
	rx_ring->rx_buffer_info[ntc].skb = skb;
#endif
	rx_ring->rx_stats.non_eop_descs++;

	return true;
}

#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT
/**
		xlnid_pull_tail - xlnid specific version of skb_pull_tail
		@skb: pointer to current skb being adjusted

		This function is an xlnid specific version of __pskb_pull_tail.  The
		main difference between this version and the original function is that
		this function can make several assumptions about the state of things
		that allow for significant optimizations versus the standard function.
		As a result we can do things like drop a frag and maintain an accurate
		truesize for the skb.
*/
static void xlnid_pull_tail(struct sk_buff *skb)
{
	skb_frag_t *frag = &skb_shinfo(skb)->frags[0];
	unsigned char *va;
	unsigned int pull_len;

	/*
		it is valid to use page_address instead of kmap since we are
		working with pages allocated out of the lomem pool per
		alloc_page(GFP_ATOMIC)
	*/
	va = skb_frag_address(frag);

	/*
		we need the header to contain the greater of either ETH_HLEN or
		60 bytes if the skb->len is less than 60 for skb_pad.
	*/
	pull_len = eth_get_headlen(skb->dev, va, XLNID_RX_HDR_SIZE);

	/* align pull length to size of long to optimize memcpy performance */
	skb_copy_to_linear_data(skb, va, ALIGN(pull_len, sizeof(long)));

	/* update all of the pointers */
	skb_frag_size_sub(frag, pull_len);
	skb_frag_off_add(frag, pull_len);
	skb->data_len -= pull_len;
	skb->tail += pull_len;
}

/**
		xlnid_dma_sync_frag - perform DMA sync for first frag of SKB
		@rx_ring: rx descriptor ring packet is being transacted on
		@skb: pointer to current skb being updated

		This function provides a basic DMA sync up for the first fragment of an
		skb.  The reason for doing this is that the first fragment cannot be
		unmapped until we have reached the end of packet descriptor for a buffer
		chain.
*/
static void xlnid_dma_sync_frag(struct xlnid_ring *rx_ring, struct sk_buff *skb)
{
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
	DEFINE_DMA_ATTRS(attrs);

	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);

#endif

	/* if the page was released unmap it, else just sync our portion */
	if (unlikely(XLNID_CB(skb)->page_released)) {
		dma_unmap_page_attrs(rx_ring->dev, XLNID_CB(skb)->dma,
								xlnid_rx_pg_size(rx_ring), DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
								&attrs);
#else
								XLNID_RX_DMA_ATTR);
#endif
	}

	else if (ring_uses_build_skb(rx_ring)) {
		unsigned long offset = (unsigned long)(skb->data) & ~PAGE_MASK;

		dma_sync_single_range_for_cpu(rx_ring->dev, XLNID_CB(skb)->dma,
										offset, skb_headlen(skb),
										DMA_FROM_DEVICE);
	}

	else {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[0];

		dma_sync_single_range_for_cpu(rx_ring->dev, XLNID_CB(skb)->dma,
										skb_frag_off(frag),
										skb_frag_size(frag),
										DMA_FROM_DEVICE);
	}
}

/**
		xlnid_cleanup_headers - Correct corrupted or empty headers
		@rx_ring: rx descriptor ring packet is being transacted on
		@rx_desc: pointer to the EOP Rx descriptor
		@skb: pointer to current skb being fixed

		Check if the skb is valid. In the XDP case it will be an error pointer.
		Return true in this case to abort processing and advance to next
		descriptor.

		Check for corrupted packet headers caused by senders on the local L2
		embedded NIC switch not setting up their Tx Descriptors right.  These
		should be very rare.

		Also address the case where we are pulling data in on pages only
		and as such no data is present in the skb header.

		In addition if skb is not at least 60 bytes we need to pad it so that
		it is large enough to qualify as a valid Ethernet frame.

		Returns true if an error was encountered and skb was freed.
	**/
bool xlnid_cleanup_headers(struct xlnid_ring __maybe_unused *rx_ring,
							union xlnid_adv_rx_desc *rx_desc,
							struct sk_buff *skb)
{
	/* XDP packets use error pointer so abort at this point */
	if (IS_ERR(skb)) {
		return true;
	}

	/* verify that the packet does not have any known errors */
	if (unlikely(xlnid_test_staterr(rx_desc,
									XLNID_RXDADV_ERR_FRAME_ERR_MASK))) {
		dev_kfree_skb_any(skb);
		return true;
	}

	/* place header in linear portion of buffer */
	if (!skb_headlen(skb)) {
		xlnid_pull_tail(skb);
	}

	/* if eth_skb_pad returns an error the skb was freed */
	if (eth_skb_pad(skb)) {
		return true;
	}

	return false;
}

/**
		xlnid_reuse_rx_page - page flip buffer and store it back on the ring
		@rx_ring: rx descriptor ring to store buffers on
		@old_buff: donor buffer to have page reused

		Synchronizes page for reuse by the adapter
	**/
static void xlnid_reuse_rx_page(struct xlnid_ring *rx_ring,
								struct xlnid_rx_buffer *old_buff)
{
	struct xlnid_rx_buffer *new_buff;
	u16 nta = rx_ring->next_to_alloc;

	new_buff = &rx_ring->rx_buffer_info[nta];

	/* update, and store next to alloc */
	nta++;
	rx_ring->next_to_alloc = (nta < rx_ring->count) ? nta : 0;

	/*  Transfer page from old buffer to new buffer.
		Move each member individually to avoid possible store
		forwarding stalls and unnecessary copy of skb.
	*/
	new_buff->dma = old_buff->dma;
	new_buff->page = old_buff->page;
	new_buff->page_offset = old_buff->page_offset;
	new_buff->pagecnt_bias = old_buff->pagecnt_bias;
}

static inline bool xlnid_page_is_reserved(struct page *page)
{
	return (page_to_nid(page) != numa_mem_id()) || page_is_pfmemalloc(page);
}

static bool xlnid_can_reuse_rx_page(struct xlnid_rx_buffer *rx_buffer)
{
	unsigned int pagecnt_bias = rx_buffer->pagecnt_bias;
	struct page *page = rx_buffer->page;

	/* avoid re-using remote pages */
	if (unlikely(xlnid_page_is_reserved(page))) {
		return false;
	}

#if (PAGE_SIZE < 8192)
	/* if we are only owner of page we can reuse it */
#ifdef HAVE_PAGE_COUNT_BULK_UPDATE

	if (unlikely((page_ref_count(page) - pagecnt_bias) > 1))
#else
	if (unlikely((page_count(page) - pagecnt_bias) > 1))
#endif
		return false;

#else
	/*  The last offset is a bit aggressive in that we assume the
		worst case of FCoE being enabled and using a 3K buffer.
		However this should have minimal impact as the 1K extra is
		still less than one buffer in size.
	*/
#define XLNID_LAST_OFFSET (SKB_WITH_OVERHEAD(PAGE_SIZE) - XLNID_RXBUFFER_3K)

	if (rx_buffer->page_offset > XLNID_LAST_OFFSET) {
		return false;
	}

#endif

#ifdef HAVE_PAGE_COUNT_BULK_UPDATE

	/*  If we have drained the page fragment pool we need to update
		the pagecnt_bias and page count so that we fully restock the
		number of references the driver holds.
	*/
	if (unlikely(pagecnt_bias == 1)) {
		page_ref_add(page, USHRT_MAX - 1);
		rx_buffer->pagecnt_bias = USHRT_MAX;
	}

#else

	/*  Even if we own the page, we are not allowed to use atomic_set()
		This would break get_page_unless_zero() users.
	*/
	if (likely(!pagecnt_bias)) {
		page_ref_inc(page);
		rx_buffer->pagecnt_bias = 1;
	}

#endif

	return true;
}

/**
		xlnid_add_rx_frag - Add contents of Rx buffer to sk_buff
		@rx_ring: rx descriptor ring to transact packets on
		@rx_buffer: buffer containing page to add
		@skb: sk_buff to place the data into
		@size: size of data

		This function will add the data contained in rx_buffer->page to the skb.
		This is done either through a direct copy if the data in the buffer is
		less than the skb header size, otherwise it will just attach the page as
		a frag to the skb.

		The function will then update the page offset if necessary and return
		true if the buffer can be reused by the adapter.
	**/
static void xlnid_add_rx_frag(struct xlnid_ring *rx_ring,
								struct xlnid_rx_buffer *rx_buffer,
								struct sk_buff *skb, unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = xlnid_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize =
		rx_ring->rx_offset ? SKB_DATA_ALIGN(rx_ring->rx_offset + size) :
		SKB_DATA_ALIGN(size);
#endif

	skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, rx_buffer->page,
					rx_buffer->page_offset, size, truesize);

#if (PAGE_SIZE < 8192)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif
}

static struct xlnid_rx_buffer *
xlnid_get_rx_buffer(struct xlnid_ring *rx_ring,
					union xlnid_adv_rx_desc *rx_desc, struct sk_buff **skb,
					const unsigned int size)
{
	struct xlnid_rx_buffer *rx_buffer;

	rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
	prefetchw(rx_buffer->page);
	*skb = rx_buffer->skb;

	/*  Delay unmapping of the first packet. It carries the header
		information, HW may still access the header after the writeback.
		Only unmap it when EOP is reached
	*/
	if (!xlnid_test_staterr(rx_desc, XLNID_RXD_STAT_EOP)) {
		if (!*skb) {
			goto skip_sync;
		}
	}

	else {
		if (*skb) {
			xlnid_dma_sync_frag(rx_ring, *skb);
		}
	}

	/* we are reusing so sync this buffer for CPU use */
	dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma,
									rx_buffer->page_offset, size,
									DMA_FROM_DEVICE);
skip_sync:
	rx_buffer->pagecnt_bias--;

	return rx_buffer;
}

static void xlnid_put_rx_buffer(struct xlnid_ring *rx_ring,
								struct xlnid_rx_buffer *rx_buffer,
								struct sk_buff *skb)
{
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
	DEFINE_DMA_ATTRS(attrs);

	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);

#endif

	if (xlnid_can_reuse_rx_page(rx_buffer)) {
		/* hand second half of page back to the ring */
		xlnid_reuse_rx_page(rx_ring, rx_buffer);
	}

	else {
		if (!IS_ERR(skb) && XLNID_CB(skb)->dma == rx_buffer->dma) {
			/* the page has been released from the ring */
			XLNID_CB(skb)->page_released = true;
		}

		else {
			/* we are not reusing the buffer so unmap it */
			dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
									xlnid_rx_pg_size(rx_ring),
									DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
									&attrs);
#else
									XLNID_RX_DMA_ATTR);
#endif
		}

		__page_frag_cache_drain(rx_buffer->page,
								rx_buffer->pagecnt_bias);
	}

	/* clear contents of rx_buffer */
	rx_buffer->page = NULL;
	rx_buffer->skb = NULL;
}

static struct sk_buff *xlnid_construct_skb(struct xlnid_ring *rx_ring,
		struct xlnid_rx_buffer *rx_buffer,
		struct xdp_buff *xdp,
		union xlnid_adv_rx_desc *rx_desc)
{
	unsigned int size = xdp->data_end - xdp->data;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = xlnid_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize =
		SKB_DATA_ALIGN(xdp->data_end - xdp->data_hard_start);
#endif
	struct sk_buff *skb;

	/* prefetch first cache line of first page */
	prefetch(xdp->data);
#if L1_CACHE_BYTES < 128
	prefetch(xdp->data + L1_CACHE_BYTES);
#endif
	/*  Note, we get here by enabling legacy-rx via:

			ethtool --set-priv-flags <dev> legacy-rx on

		In this mode, we currently get 0 extra XDP headroom as
		opposed to having legacy-rx off, where we process XDP
		packets going to stack via xlnid_build_skb(). The latter
		provides us currently with 192 bytes of headroom.

		For xlnid_construct_skb() mode it means that the
		xdp->data_meta will always point to xdp->data, since
		the helper cannot expand the head. Should this ever
		change in future for legacy-rx mode on, then lets also
		add xdp->data_meta handling here.
	*/

	/* allocate a skb to store the frags */
	skb = napi_alloc_skb(&rx_ring->q_vector->napi, XLNID_RX_HDR_SIZE);

	if (unlikely(!skb)) {
		return NULL;
	}

	if (size > XLNID_RX_HDR_SIZE) {
		if (!xlnid_test_staterr(rx_desc, XLNID_RXD_STAT_EOP)) {
			XLNID_CB(skb)->dma = rx_buffer->dma;
		}

		skb_add_rx_frag(skb, 0, rx_buffer->page,
						xdp->data - page_address(rx_buffer->page), size,
						truesize);
#if (PAGE_SIZE < 8192)
		rx_buffer->page_offset ^= truesize;
#else
		rx_buffer->page_offset += truesize;
#endif
	}

	else {
		memcpy(__skb_put(skb, size), xdp->data,
				ALIGN(size, sizeof(long)));
		rx_buffer->pagecnt_bias++;
	}

	return skb;
}

#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC
static struct sk_buff *xlnid_build_skb(struct xlnid_ring *rx_ring,
										struct xlnid_rx_buffer *rx_buffer,
										struct xdp_buff *xdp,
										union xlnid_adv_rx_desc *rx_desc)
{
#ifdef HAVE_XDP_BUFF_DATA_META
	unsigned int metasize = xdp->data - xdp->data_meta;
	void *va = xdp->data_meta;
#else
	void *va = xdp->data;
#endif
#if (PAGE_SIZE < 8192)
	unsigned int truesize = xlnid_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize =
		SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
		SKB_DATA_ALIGN(xdp->data_end - xdp->data_hard_start);
#endif
	struct sk_buff *skb;

	/* prefetch first cache line of first page */
	prefetch(va);
#if L1_CACHE_BYTES < 128
	prefetch(va + L1_CACHE_BYTES);
#endif

	/* build an skb around the page buffer */
	skb = build_skb(xdp->data_hard_start, truesize);

	if (unlikely(!skb)) {
		return NULL;
	}

	/* update pointers within the skb to store the data */
	skb_reserve(skb, xdp->data - xdp->data_hard_start);
	__skb_put(skb, xdp->data_end - xdp->data);
#ifdef HAVE_XDP_BUFF_DATA_META

	if (metasize) {
		skb_metadata_set(skb, metasize);
	}

#endif

	/* record DMA address if this is the start of a chain of buffers */
	if (!xlnid_test_staterr(rx_desc, XLNID_RXD_STAT_EOP)) {
		XLNID_CB(skb)->dma = rx_buffer->dma;
	}

	/* update buffer offset */
#if (PAGE_SIZE < 8192)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif

	return skb;
}

#endif /* HAVE_SWIOTLB_SKIP_CPU_SYNC */

static struct sk_buff *
xlnid_run_xdp(struct xlnid_adapter __maybe_unused *adapter,
				struct xlnid_ring __maybe_unused *rx_ring,
				struct xdp_buff __maybe_unused *xdp)
{
	int result = XLNID_XDP_PASS;
#ifdef HAVE_XDP_SUPPORT
	struct bpf_prog *xdp_prog;
	struct xlnid_ring *ring;
#ifdef HAVE_XDP_FRAME_STRUCT
	struct xdp_frame *xdpf;
#endif
	int err;
	u32 act;

	rcu_read_lock();
	xdp_prog = READ_ONCE(rx_ring->xdp_prog);

	if (!xdp_prog) {
		goto xdp_out;
	}

#ifdef HAVE_XDP_FRAME_STRUCT
	prefetchw(xdp->data_hard_start); /* xdp_frame write */
#endif

	act = bpf_prog_run_xdp(xdp_prog, xdp);

	switch (act) {
	case XDP_PASS:
		break;

	case XDP_TX:
		#ifdef HAVE_XDP_FRAME_STRUCT
		xdpf = xdp_convert_buff_to_frame(xdp);

		if (unlikely(!xdpf)) {
		result = XLNID_XDP_CONSUMED;
		break;
			}

#endif

			ring = xlnid_determine_xdp_ring(adapter);

			if (static_branch_unlikely(&xlnid_xdp_locking_key)) {
				spin_lock(&ring->tx_lock);
			}

#ifdef HAVE_XDP_FRAME_STRUCT
			result = xlnid_xmit_xdp_ring(ring, xdpf);
#else
			result = xlnid_xmit_xdp_ring(ring, xdp);
#endif

			if (static_branch_unlikely(&xlnid_xdp_locking_key)) {
				spin_unlock(&ring->tx_lock);
			}

			break;

		case XDP_REDIRECT:
			err = xdp_do_redirect(adapter->netdev, xdp, xdp_prog);

			if (!err) {
				result = XLNID_XDP_REDIR;
			}

			else {
				result = XLNID_XDP_CONSUMED;
			}

			break;

		default:
			bpf_warn_invalid_xdp_action(rx_ring->netdev, xdp_prog, act);
			fallthrough;

		case XDP_ABORTED:
			trace_xdp_exception(rx_ring->netdev, xdp_prog, act);
			/* fallthrough -- handle aborts by dropping packet */
			fallthrough;

		case XDP_DROP:
			result = XLNID_XDP_CONSUMED;
			break;
	}

xdp_out:
	rcu_read_unlock();
#endif /* HAVE_XDP_SUPPORT */

	return ERR_PTR(-result);
}

static unsigned int xlnid_rx_frame_truesize(struct xlnid_ring *rx_ring,
		unsigned int size)
{
	unsigned int truesize;

#if (PAGE_SIZE < 8192)
	truesize = xlnid_rx_pg_size(rx_ring) / 2;
#else
	truesize =
		rx_ring->rx_offset ?
		SKB_DATA_ALIGN(rx_ring->rx_offset + size)
#ifdef HAVE_XDP_BUFF_FRAME_SZ
		+ SKB_DATA_ALIGN(sizeof(struct skb_shared_info))
#endif
		:
		SKB_DATA_ALIGN(size);
#endif
	return truesize;
}

static void xlnid_rx_buffer_flip(struct xlnid_ring *rx_ring,
									struct xlnid_rx_buffer *rx_buffer,
									unsigned int size)
{
	unsigned int truesize = xlnid_rx_frame_truesize(rx_ring, size);

#if (PAGE_SIZE < 8192)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif
}

/**
		xlnid_clean_rx_irq - Clean completed descriptors from Rx ring - bounce buf
		@q_vector: structure containing interrupt and ring information
		@rx_ring: rx descriptor ring to transact packets on
		@budget: Total limit on number of packets to process

		This function provides a "bounce buffer" approach to Rx interrupt
		processing.  The advantage to this is that on systems that have
		expensive overhead for IOMMU access this provides a means of avoiding
		it by maintaining the mapping of the page to the syste.

		Returns amount of work completed.
	**/
static int xlnid_clean_rx_irq(struct xlnid_q_vector *q_vector,
								struct xlnid_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	struct xlnid_adapter *adapter = q_vector->adapter;
	u16 cleaned_count = xlnid_desc_unused(rx_ring);
	unsigned int offset = rx_ring->rx_offset;
	unsigned int xdp_xmit = 0;
	struct xdp_buff xdp;

	xdp.data = NULL;
	xdp.data_end = NULL;
#ifdef HAVE_XDP_BUFF_RXQ
	xdp.rxq = &rx_ring->xdp_rxq;
#endif

#ifdef HAVE_XDP_BUFF_FRAME_SZ
	/* Frame size depend on rx_ring setup when PAGE_SIZE = 4K */
#if (PAGE_SIZE < 8192)
	xdp.frame_sz = xlnid_rx_frame_truesize(rx_ring, 0);
#endif
#endif

	while (likely(total_rx_packets < budget)) {
		union xlnid_adv_rx_desc *rx_desc;
		struct xlnid_rx_buffer *rx_buffer;
		struct sk_buff *skb;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= XLNID_RX_BUFFER_WRITE) {
			xlnid_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}

		rx_desc = XLNID_RX_DESC(rx_ring, rx_ring->next_to_clean);
		size = le16_to_cpu(rx_desc->wb.upper.length);

		if (!size) {
			break;
		}

		/*  This memory barrier is needed to keep us from reading
			any other fields out of the rx_desc until we know the
			descriptor has been written back
		*/
		dma_rmb();

		rx_buffer = xlnid_get_rx_buffer(rx_ring, rx_desc, &skb, size);

		/* retrieve a buffer from the ring */
		if (!skb) {
			xdp.data = page_address(rx_buffer->page) +
						rx_buffer->page_offset;
#ifdef HAVE_XDP_BUFF_DATA_META
			xdp.data_meta = xdp.data;
#endif
			xdp.data_hard_start = xdp.data - offset;
			xdp.data_end = xdp.data + size;

#ifdef HAVE_XDP_BUFF_FRAME_SZ
#if (PAGE_SIZE > 4096)
			/* At larger PAGE_SIZE, frame_sz depend on len size */
			xdp.frame_sz = xlnid_rx_frame_truesize(rx_ring, size);
#endif
#endif
			skb = xlnid_run_xdp(adapter, rx_ring, &xdp);
		}

		if (IS_ERR(skb)) {
			unsigned int xdp_res = -PTR_ERR(skb);

			if (xdp_res & (XLNID_XDP_TX | XLNID_XDP_REDIR)) {
				xdp_xmit |= xdp_res;
				xlnid_rx_buffer_flip(rx_ring, rx_buffer, size);
			}

			else {
				rx_buffer->pagecnt_bias++;
			}

			total_rx_packets++;
			total_rx_bytes += size;
		}

		else if (skb) {
			xlnid_add_rx_frag(rx_ring, rx_buffer, skb, size);
#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC
		}

		else if (ring_uses_build_skb(rx_ring)) {
			skb = xlnid_build_skb(rx_ring, rx_buffer, &xdp,
									rx_desc);
#endif
		}

		else {
			skb = xlnid_construct_skb(rx_ring, rx_buffer, &xdp,
										rx_desc);
		}

		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			rx_buffer->pagecnt_bias++;
			break;
		}

		xlnid_put_rx_buffer(rx_ring, rx_buffer, skb);
		cleaned_count++;

		/* place incomplete frames back on ring for completion */
		if (xlnid_is_non_eop(rx_ring, rx_desc, skb)) {
			continue;
		}

		/* verify the packet layout is correct */
		if (xlnid_cleanup_headers(rx_ring, rx_desc, skb)) {
			continue;
		}

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, timestamp, VLAN, and protocol */
		xlnid_process_skb_fields(rx_ring, rx_desc, skb);

		xlnid_rx_skb(q_vector, rx_ring, rx_desc, skb);

		/* update budget accounting */
		total_rx_packets++;
	}

	if (xdp_xmit & XLNID_XDP_REDIR) {
		xdp_do_flush();
	}

	if (xdp_xmit & XLNID_XDP_TX) {
		struct xlnid_ring *ring = xlnid_determine_xdp_ring(adapter);

		xlnid_xdp_ring_update_tail_locked(ring);
	}

	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	u64_stats_update_end(&rx_ring->syncp);
	q_vector->rx.total_packets += total_rx_packets;
	q_vector->rx.total_bytes += total_rx_bytes;

	return total_rx_packets;
}

#else /* CONFIG_XLNID_DISABLE_PACKET_SPLIT */
/**
		xlnid_clean_rx_irq - Clean completed descriptors from Rx ring - legacy
		@q_vector: structure containing interrupt and ring information
		@rx_ring: rx descriptor ring to transact packets on
		@budget: Total limit on number of packets to process

		This function provides a legacy approach to Rx interrupt
		handling.  This version will perform better on systems with a low cost
		dma mapping API.

		Returns amount of work completed.
	**/
static int xlnid_clean_rx_irq(struct xlnid_q_vector *q_vector,
								struct xlnid_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	u16 len = 0;
	u16 cleaned_count = xlnid_desc_unused(rx_ring);

	while (likely(total_rx_packets < budget)) {
		struct xlnid_rx_buffer *rx_buffer;
		union xlnid_adv_rx_desc *rx_desc;
		struct sk_buff *skb;
		u16 ntc;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= XLNID_RX_BUFFER_WRITE) {
			xlnid_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}

		ntc = rx_ring->next_to_clean;
		rx_desc = XLNID_RX_DESC(rx_ring, ntc);
		rx_buffer = &rx_ring->rx_buffer_info[ntc];

		if (!rx_desc->wb.upper.length) {
			break;
		}

		/*  This memory barrier is needed to keep us from reading
			any other fields out of the rx_desc until we know the
			descriptor has been written back
		*/
		dma_rmb();

		skb = rx_buffer->skb;

		prefetch(skb->data);

		len = le16_to_cpu(rx_desc->wb.upper.length);
		/* pull the header of the skb in */
		__skb_put(skb, len);

		/*
			Delay unmapping of the first packet. It carries the
			header information, HW may still access the header after
			the writeback.  Only unmap it when EOP is reached
		*/
		if (!XLNID_CB(skb)->head) {
			XLNID_CB(skb)->dma = rx_buffer->dma;
		}

		else {
			skb = xlnid_merge_active_tail(skb);
			dma_unmap_single(rx_ring->dev, rx_buffer->dma,
								rx_ring->rx_buf_len, DMA_FROM_DEVICE);
		}

		/* clear skb reference in buffer info structure */
		rx_buffer->skb = NULL;
		rx_buffer->dma = 0;

		cleaned_count++;

		if (xlnid_is_non_eop(rx_ring, rx_desc, skb)) {
			continue;
		}

		dma_unmap_single(rx_ring->dev, XLNID_CB(skb)->dma,
							rx_ring->rx_buf_len, DMA_FROM_DEVICE);

		XLNID_CB(skb)->dma = 0;

		if (xlnid_close_active_frag_list(skb) &&
				!XLNID_CB(skb)->append_cnt) {
			/* if we got here without RSC the packet is invalid */
			dev_kfree_skb_any(skb);
			continue;
		}

		/* ERR_MASK will only have valid bits if EOP set */
		if (unlikely(xlnid_test_staterr(
							rx_desc, XLNID_RXDADV_ERR_FRAME_ERR_MASK))) {
			dev_kfree_skb_any(skb);
			continue;
		}

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, timestamp, VLAN, and protocol */
		xlnid_process_skb_fields(rx_ring, rx_desc, skb);

		xlnid_rx_skb(q_vector, rx_ring, rx_desc, skb);

		/* update budget accounting */
		total_rx_packets++;
	}

	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	q_vector->rx.total_packets += total_rx_packets;
	q_vector->rx.total_bytes += total_rx_bytes;

	if (cleaned_count) {
		xlnid_alloc_rx_buffers(rx_ring, cleaned_count);
	}

	return total_rx_packets;
}

#endif /* CONFIG_XLNID_DISABLE_PACKET_SPLIT */
#ifdef HAVE_NDO_BUSY_POLL
/* must be called with local_bh_disable()d */
static int xlnid_busy_poll_recv(struct napi_struct *napi)
{
	struct xlnid_q_vector *q_vector =
		container_of(napi, struct xlnid_q_vector, napi);
	struct xlnid_adapter *adapter = q_vector->adapter;
	struct xlnid_ring *ring;
	int found = 0;

	if (test_bit(__XLNID_DOWN, &adapter->state)) {
		return LL_FLUSH_FAILED;
	}

	if (!xlnid_qv_lock_poll(q_vector)) {
		return LL_FLUSH_BUSY;
	}

	xlnid_for_each_ring(ring, q_vector->rx) {
		found = xlnid_clean_rx_irq(q_vector, ring, 4);
#ifdef BP_EXTENDED_STATS

		if (found) {
			ring->stats.cleaned += found;
		}

		else {
			ring->stats.misses++;
		}

#endif

		if (found) {
			break;
		}
	}

	xlnid_qv_unlock_poll(q_vector);

	return found;
}

#endif /* HAVE_NDO_BUSY_POLL */
/**
		xlnid_configure_msix - Configure MSI-X hardware
		@adapter: board private structure

		xlnid_configure_msix sets up the hardware to properly generate MSI-X
		interrupts.
	**/
static void xlnid_configure_msix(struct xlnid_adapter *adapter)
{
	int v_idx;
	u32 mask;

#if 0

	/* Populate MSIX to EITR Select */
	if (adapter->num_vfs >= 32) {
		u32 eitrsel = (1 << (adapter->num_vfs - 32)) - 1;
		XLNID_WRITE_REG(&adapter->hw, XLNID_EITRSEL, eitrsel);
	}

#endif

	/*
		Populate the IVAR table and set the ITR values to the
		corresponding register.
	*/
	for (v_idx = 0; v_idx < adapter->num_q_vectors; v_idx++) {
		struct xlnid_q_vector *q_vector = adapter->q_vector[v_idx];
		struct xlnid_ring *ring;

		xlnid_for_each_ring(ring, q_vector->rx)
		xlnid_set_ivar(adapter, 0, ring->reg_idx, v_idx);

		xlnid_for_each_ring(ring, q_vector->tx)
		xlnid_set_ivar(adapter, 1, ring->reg_idx, v_idx);

		xlnid_write_eitr(q_vector);
	}

	xlnid_set_ivar(adapter, -1, 1, v_idx);

	XLNID_WRITE_REG_DIRECT(&adapter->hw, EITR(v_idx), 1950);

	/* set up to autoclear timer, and the vectors */
	mask = XLNID_EIMS_ENABLE_MASK;
	mask &= ~(XLNID_EIMS_OTHER | XLNID_EIMS_MAILBOX | XLNID_EIMS_LSC);

	XLNID_WRITE_REG_DIRECT(&adapter->hw, EIAC, mask);
}

static inline bool xlnid_container_is_rx(struct xlnid_q_vector *q_vector,
		struct xlnid_ring_container *rc)
{
	return &q_vector->rx == rc;
}

/**
		xlnid_update_itr - update the dynamic ITR value based on statistics
		@q_vector: structure containing interrupt and ring information
		@ring_container: structure containing ring performance data

			Stores a new ITR value based on packets and byte
			counts during the last interrupt.  The advantage of per interrupt
			computation is faster updates and more accurate ITR for the current
			traffic pattern.  Constants in this function were computed
			based on theoretical maximum wire speed and thresholds were set based
			on testing data as well as attempting to minimize response time
			while increasing bulk throughput.
	**/
static void xlnid_update_itr(struct xlnid_q_vector *q_vector,
								struct xlnid_ring_container *ring_container)
{
	unsigned int itr = XLNID_ITR_ADAPTIVE_MIN_USECS |
						XLNID_ITR_ADAPTIVE_LATENCY;
	unsigned int avg_wire_size, packets, bytes;
	unsigned long next_update = jiffies;

	/*  If we don't have any rings just leave ourselves set for maximum
		possible latency so we take ourselves out of the equation.
	*/
	if (!ring_container->ring) {
		return;
	}

	/*  If we didn't update within up to 1 - 2 jiffies we can assume
		that either packets are coming in so slow there hasn't been
		any work, or that there is so much work that NAPI is dealing
		with interrupt moderation and we don't need to do anything.
	*/
	if (time_after(next_update, ring_container->next_update)) {
		goto clear_counts;
	}

	packets = ring_container->total_packets;
	bytes = ring_container->total_bytes;

	if (xlnid_container_is_rx(q_vector, ring_container)) {
		/*  If Rx and there are 1 to 23 packets and bytes are less than
			12112 assume insufficient data to use bulk rate limiting
			approach. Instead we will focus on simply trying to target
			receiving 8 times as much data in the next interrupt.
		*/
		if (packets && packets < 24 && bytes < 12112) {
			itr = XLNID_ITR_ADAPTIVE_LATENCY;
			avg_wire_size = (bytes + packets * 24) * 2;
			avg_wire_size = clamp_t(unsigned int, avg_wire_size,
									2560, 12800);
			goto adjust_for_speed;
		}
	}

	/*  Less than 48 packets we can assume that our current interrupt delay
		is only slightly too low. As such we should increase it by a small
		fixed amount.
	*/
	if (packets < 48) {
		itr = (q_vector->itr >> 2) + XLNID_ITR_ADAPTIVE_MIN_INC;

		if (itr > XLNID_ITR_ADAPTIVE_MAX_USECS) {
			itr = XLNID_ITR_ADAPTIVE_MAX_USECS;
		}

		/*  If sample size is 0 - 7 we should probably switch
			to latency mode instead of trying to control
			things as though we are in bulk.

			Otherwise if the number of packets is less than 48
			we should maintain whatever mode we are currently
			in. The range between 8 and 48 is the cross-over
			point between latency and bulk traffic.
		*/
		if (packets < 8) {
			itr += XLNID_ITR_ADAPTIVE_LATENCY;
		}

		else {
			itr += ring_container->itr & XLNID_ITR_ADAPTIVE_LATENCY;
		}

		goto clear_counts;
	}

	/*  Between 48 and 96 is our "goldilocks" zone where we are working
		out "just right". Just report that our current ITR is good for us.
	*/
	if (packets < 96) {
		itr = q_vector->itr >> 2;
		goto clear_counts;
	}

	/*  If packet count is 96 or greater we are likely looking at a slight
		overrun of the delay we want. Try halving our delay to see if that
		will cut the number of packets in half per interrupt.
	*/
	if (packets < 256) {
		itr = q_vector->itr >> 3;

		if (itr < XLNID_ITR_ADAPTIVE_MIN_USECS) {
			itr = XLNID_ITR_ADAPTIVE_MIN_USECS;
		}

		goto clear_counts;
	}

	/*  The paths below assume we are dealing with a bulk ITR since number
		of packets is 256 or greater. We are just going to have to compute
		a value and try to bring the count under control, though for smaller
		packet sizes there isn't much we can do as NAPI polling will likely
		be kicking in sooner rather than later.
	*/
	itr = XLNID_ITR_ADAPTIVE_BULK;

	/*  If packet counts are 256 or greater we can assume we have a gross
		overestimation of what the rate should be. Instead of trying to fine
		tune it just use the formula below to try and dial in an exact value
		give the current packet size of the frame.
	*/
	avg_wire_size = bytes / packets;

	/*  The following is a crude approximation of:
		wmem_default / (size + overhead) = desired_pkts_per_int
		rate / bits_per_byte / (size + ethernet overhead) = pkt_rate
		(desired_pkt_rate / pkt_rate) * usecs_per_sec = ITR value

		Assuming wmem_default is 212992 and overhead is 640 bytes per
		packet, (256 skb, 64 headroom, 320 shared info), we can reduce the
		formula down to

		(170 * (size + 24)) / (size + 640) = ITR

		We first do some math on the packet size and then finally bitshift
		by 8 after rounding up. We also have to account for PCIe link speed
		difference as ITR scales based on this.
	*/
	if (avg_wire_size <= 60) {
		/* Start at 50k ints/sec */
		avg_wire_size = 5120;
	}

	else if (avg_wire_size <= 316) {
		/* 50K ints/sec to 16K ints/sec */
		avg_wire_size *= 40;
		avg_wire_size += 2720;
	}

	else if (avg_wire_size <= 1084) {
		/* 16K ints/sec to 9.2K ints/sec */
		avg_wire_size *= 15;
		avg_wire_size += 11452;
	}

	else if (avg_wire_size <= 1980) {
		/* 9.2K ints/sec to 8K ints/sec */
		avg_wire_size *= 5;
		avg_wire_size += 22420;
	}

	else {
		/* plateau at a limit of 8K ints/sec */
		avg_wire_size = 32256;
	}

adjust_for_speed:

	/*  Resultant value is 256 times larger than it needs to be. This
		gives us room to adjust the value as needed to either increase
		or decrease the value based on link speeds of 10G, 2.5G, 1G, etc.

		Use addition as we have already recorded the new latency flag
		for the ITR value.
	*/
	switch (q_vector->adapter->link_speed) {
	case XLNID_LINK_SPEED_10GB_FULL:
	case XLNID_LINK_SPEED_100_FULL:
	default:
		itr += DIV_ROUND_UP(avg_wire_size,
		XLNID_ITR_ADAPTIVE_MIN_INC * 256) *
		XLNID_ITR_ADAPTIVE_MIN_INC;
		break;

	case XLNID_LINK_SPEED_2_5GB_FULL:
	case XLNID_LINK_SPEED_1GB_FULL:
	case XLNID_LINK_SPEED_10_FULL:
		itr += DIV_ROUND_UP(avg_wire_size,
		XLNID_ITR_ADAPTIVE_MIN_INC * 64) *
		XLNID_ITR_ADAPTIVE_MIN_INC;
		break;
	}

	/*  In the case of a latency specific workload only allow us to
		reduce the ITR by at most 2us. By doing this we should dial
		in so that our number of interrupts is no more than 2x the number
		of packets for the least busy workload. So for example in the case
		of a TCP worload the ack packets being received would set the
		the interrupt rate as they are a latency specific workload.
	*/
	if ((itr & XLNID_ITR_ADAPTIVE_LATENCY) && itr < ring_container->itr) {
		itr = ring_container->itr - XLNID_ITR_ADAPTIVE_MIN_INC;
	}

clear_counts:
	/* write back value */
	ring_container->itr = itr;

	/* next update should occur within next jiffy */
	ring_container->next_update = next_update + 1;

	ring_container->total_bytes = 0;
	ring_container->total_packets = 0;
}

/**
		xlnid_write_eitr - write EITR register in hardware specific way
		@q_vector: structure containing interrupt and ring information

		This function is made to be called by ethtool and by the driver
		when it needs to update EITR registers at runtime.  Hardware
		specific quirks/differences are taken care of here.
*/
void xlnid_write_eitr(struct xlnid_q_vector *q_vector)
{
	struct xlnid_adapter *adapter = q_vector->adapter;
	struct xlnid_hw *hw = &adapter->hw;
	int v_idx = q_vector->v_idx;
	u32 itr_reg = q_vector->itr & XLNID_MAX_EITR;

	/*
		set the WDIS bit to not clear the timer bits and cause an
		immediate assertion of the interrupt
	*/
	itr_reg |= XLNID_EITR_CNT_WDIS;

	XLNID_WRITE_REG_DIRECT(hw, EITR(v_idx), itr_reg);
}

static void xlnid_set_itr(struct xlnid_q_vector *q_vector)
{
	u32 new_itr;

	xlnid_update_itr(q_vector, &q_vector->tx);
	xlnid_update_itr(q_vector, &q_vector->rx);

	/* use the smallest value of new ITR delay calculations */
	new_itr = min(q_vector->rx.itr, q_vector->tx.itr);

	/* Clear latency flag if set, shift into correct position */
	new_itr &= XLNID_ITR_ADAPTIVE_MASK_USECS;
	new_itr <<= 2;

	if (new_itr != q_vector->itr) {
		/* save the algorithm value here */
		q_vector->itr = new_itr;

		xlnid_write_eitr(q_vector);
	}
}

static void xlnid_check_lsc(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;

	adapter->lsc_int++;
	adapter->flags |= XLNID_FLAG_NEED_LINK_UPDATE;
	adapter->link_check_timeout = jiffies;

	if (!test_bit(__XLNID_DOWN, &adapter->state)) {
		XLNID_WRITE_REG_DIRECT(hw, EIMC, XLNID_EIMC_LSC);
		XLNID_WRITE_FLUSH(hw);
		xlnid_service_event_schedule(adapter);
	}
}

static void xlnid_irq_enable_queues(struct xlnid_adapter *adapter, u64 qmask)
{
	u32 mask;
	struct xlnid_hw *hw = &adapter->hw;

	/*  msi and legacy is same */
	if (!(adapter->flags & XLNID_FLAG_MSIX_ENABLED)) {
		mask = (XLNID_EIMS_RTX_QUEUE & qmask);
		XLNID_WRITE_REG_DIRECT(hw, EIMS, mask);
	}

	else {
		mask = (qmask & 0xFFFFFFFF);

		if (mask) {
			XLNID_WRITE_REG_DIRECT(hw, EIMS_EX(0), mask);
		}

		mask = (qmask >> 32);

		if (mask) {
			XLNID_WRITE_REG_DIRECT(hw, EIMS_EX(1), mask);
		}
	}

	/* skip the flush */
}

/**
		xlnid_irq_enable - Enable default interrupt generation settings
		@adapter: board private structure
		@queues: enable irqs for queues
		@flush: flush register write
	**/
static inline void xlnid_irq_enable(struct xlnid_adapter *adapter, bool queues,
									bool flush)
{
	u32 mask = (XLNID_EIMS_ENABLE_MASK & ~XLNID_EIMS_RTX_QUEUE);

	/* don't reenable LSC while waiting for link */
	if (adapter->flags & XLNID_FLAG_NEED_LINK_UPDATE) {
		mask &= ~XLNID_EIMS_LSC;
	}

	mask |= XLNID_EIMS_GPI_SDP1;
	mask |= XLNID_EIMS_GPI_SDP2;
	mask |= XLNID_EIMS_ECC;
	mask |= XLNID_EIMS_MAILBOX;

	if ((adapter->flags & XLNID_FLAG_FDIR_HASH_CAPABLE) &&
			!(adapter->flags2 & XLNID_FLAG2_FDIR_REQUIRES_REINIT)) {
		mask |= XLNID_EIMS_FLOW_DIR;
	}

	XLNID_WRITE_REG_DIRECT(&adapter->hw, EIMS, mask);

	if (queues) {
		xlnid_irq_enable_queues(adapter, ~0);
	}

	if (flush) {
		XLNID_WRITE_FLUSH(&adapter->hw);
	}
}

static irqreturn_t xlnid_msix_other(int __always_unused irq, void *data)
{
	struct xlnid_adapter *adapter = data;
	struct xlnid_hw *hw = &adapter->hw;
	u32 eicr;
	u32 regval = 0;
	u32 pntrh = 0;
	u32 an = 0;

	/*
		Use clear-by-write
		instead of clear-by-read.  Reading with EICS will return the
		interrupt causes without clearing, which later be done
		with the write to EICR.
	*/
	eicr = XLNID_READ_REG_DIRECT(hw, EICR);

	/*  The lower 16bits of the EICR register are for the queue interrupts
		which should be masked here in order to not accidently clear them if
		the bits are high when xlnid_msix_other is called. There is a race
		condition otherwise which results in possible performance loss
		especially if the xlnid_msix_other interrupt is triggering
		consistently (as it would when PPS is turned on for the X540 device)
	*/
	eicr &= 0xFFFF0000;

	XLNID_WRITE_REG_DIRECT(hw, EICR, eicr);

	if (eicr & XLNID_EICR_LSC) {
		/*        printk("%s:%d: eicr = 0x%08x port1_eicr = 0x%08x \r\n",
			__func__, __LINE__, eicr, XLNID_READ_REG_DIRECT(xel_pci_info[1].hw, EICR)); */
		xlnid_check_lsc(adapter);
	}

	if (eicr & XLNID_EICR_ECC) {
		e_info(link, "Received unrecoverable ECC Err, "
				"initiating reset.\n");
		set_bit(__XLNID_RESET_REQUESTED, &adapter->state);
		xlnid_service_event_schedule(adapter);
		XLNID_WRITE_REG_DIRECT(hw, EICR, XLNID_EICR_ECC);
	}

	if (eicr & XLNID_EICR_LINKSEC) {
		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_SA);
		an = regval & XLNID_MACSEC_TXCTL_SA;

		if (!an) {
			pntrh = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_PN0);
			pntrh = be32_to_cpu(pntrh);

			if (pntrh < XLNID_MACSEC_PN_THRESHOLD) {
				goto macsec_txsa;
			}

			regval |= XLNID_MACSEC_TXCTL_SA;
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_PN0,
								XLNID_MACSEC_PN_INIT);
		}

		else {
			pntrh = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_PN1);
			pntrh = be32_to_cpu(pntrh);

			if (pntrh < XLNID_MACSEC_PN_THRESHOLD) {
				goto macsec_txsa;
			}

			regval &= ~XLNID_MACSEC_TXCTL_SA;
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_PN1,
								XLNID_MACSEC_PN_INIT);
		}

		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_SA, regval);
	}

macsec_txsa:
#ifdef HAVE_TX_MQ

	/* Handle Flow Director Full threshold interrupt */
	if (eicr & XLNID_EICR_FLOW_DIR) {
		int reinit_count = 0;
		int i;

		for (i = 0; i < adapter->num_tx_queues; i++) {
			struct xlnid_ring *ring = adapter->tx_ring[i];

			if (test_and_clear_bit(__XLNID_TX_FDIR_INIT_DONE,
									&ring->state)) {
				reinit_count++;
			}
		}

		if (reinit_count) {
			/*  no more flow director interrupts until
				after init
			*/
			XLNID_WRITE_REG_DIRECT(hw, EIMC, XLNID_EIMC_FLOW_DIR);
			adapter->flags2 |= XLNID_FLAG2_FDIR_REQUIRES_REINIT;
			xlnid_service_event_schedule(adapter);
		}
	}

#endif

	//xlnid_check_sfp_event(adapter, eicr);
	//xlnid_check_overtemp_event(adapter, eicr);
	//xlnid_check_fan_failure(adapter, eicr);

	/* re-enable the original interrupt state, no lsc, no queues */
	if (!test_bit(__XLNID_DOWN, &adapter->state)) {
		xlnid_irq_enable(adapter, false, false);
	}

	return IRQ_HANDLED;
}

static irqreturn_t xlnid_msix_clean_rings(int __always_unused irq, void *data)
{
	struct xlnid_q_vector *q_vector = data;
	struct xlnid_adapter *adapter = q_vector->adapter;
	u32 mask;
	u64 qmask = (u64)0x1 << q_vector->v_idx;

	mask = (qmask & 0xFFFFFFFF);

	if (mask) {
		XLNID_WRITE_REG_DIRECT(&adapter->hw, EIMC_EX(0), mask);
	}

	mask = (qmask >> 32);

	if (mask) {
		XLNID_WRITE_REG_DIRECT(&adapter->hw, EIMC_EX(1), mask);
	}

	/* EIAM disabled interrupts (on this vector) for us */

	if (q_vector->rx.ring || q_vector->tx.ring) {
		napi_schedule_irqoff(&q_vector->napi);
	}

	return IRQ_HANDLED;
}

/**
		xlnid_poll - NAPI polling RX/TX cleanup routine
		@napi: napi struct with our devices info in it
		@budget: amount of work driver is allowed to do this pass, in packets

		Clean all queues associated with a q_vector.
	**/
int xlnid_poll(struct napi_struct *napi, int budget)
{
	struct xlnid_q_vector *q_vector =
		container_of(napi, struct xlnid_q_vector, napi);
	struct xlnid_adapter *adapter = q_vector->adapter;
	struct xlnid_ring *ring;
	int per_ring_budget, work_done = 0;
	bool clean_complete = true;

	xlnid_for_each_ring(ring, q_vector->tx) {
#ifdef HAVE_AF_XDP_ZC_SUPPORT
		bool wd = ring->xsk_pool ?
					xlnid_clean_xdp_tx_irq(q_vector, ring) :
					xlnid_clean_tx_irq(q_vector, ring, budget);

		if (!wd)
#else
		if (!xlnid_clean_tx_irq(q_vector, ring, budget))
#endif
			clean_complete = false;
	}

#ifdef HAVE_NDO_BUSY_POLL

	if (test_bit(NAPI_STATE_NPSVC, &napi->state)) {
		return budget;
	}

	/* Exit if we are called by netpoll or busy polling is active */
	if ((budget <= 0) || !xlnid_qv_lock_napi(q_vector)) {
		return budget;
	}

#else

	/* Exit if we are called by netpoll */
	if (budget <= 0) {
		return budget;
	}

#endif

	/*  attempt to distribute budget to each queue fairly, but don't allow
		the budget to go below 1 because we'll exit polling */
	if (q_vector->rx.count > 1) {
		per_ring_budget = max(budget / q_vector->rx.count, 1);
	}

	else {
		per_ring_budget = budget;
	}

	xlnid_for_each_ring(ring, q_vector->rx) {
#ifdef HAVE_AF_XDP_ZC_SUPPORT
		int cleaned = ring->xsk_pool ?
						xlnid_clean_rx_irq_zc(q_vector, ring,
											per_ring_budget) :
						xlnid_clean_rx_irq(q_vector, ring,
											per_ring_budget);
#else
		int cleaned =
			xlnid_clean_rx_irq(q_vector, ring, per_ring_budget);
#endif /* HAVE_AF_XDP_ZC_SUPPORT */
		work_done += cleaned;

		if (cleaned >= per_ring_budget) {
			clean_complete = false;
		}
	}

#ifdef HAVE_NDO_BUSY_POLL
	xlnid_qv_unlock_napi(q_vector);

#endif
#ifndef HAVE_NETDEV_NAPI_LIST

	if (!netif_running(adapter->netdev)) {
		clean_complete = true;
	}

#endif

	/* If all work not completed, return budget and keep polling */
	if (!clean_complete) {
		return budget;
	}

	/* all work done, exit the polling mode */
	if (likely(napi_complete_done(napi, work_done))) {
		if (adapter->rx_itr_setting == 1 && !adapter->hw.ptsw_enable) {
			xlnid_set_itr(q_vector);
		}

		if (!test_bit(__XLNID_DOWN, &adapter->state))
			xlnid_irq_enable_queues(adapter,
									((u64)1 << q_vector->v_idx));
	}

	return min(work_done, budget - 1);
}

/**
		xlnid_request_msix_irqs - Initialize MSI-X interrupts
		@adapter: board private structure

		xlnid_request_msix_irqs allocates MSI-X vectors and requests
		interrupts from the kernel.
	**/
static int xlnid_request_msix_irqs(struct xlnid_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	unsigned int ri = 0, ti = 0;
	int vector, err;

	for (vector = 0; vector < adapter->num_q_vectors; vector++) {
		struct xlnid_q_vector *q_vector = adapter->q_vector[vector];
		struct msix_entry *entry = &adapter->msix_entries[vector];

		if (q_vector->tx.ring && q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name),
						"%s-TxRx-%u", netdev->name, ri++);
			ti++;
		}

		else if (q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name),
						"%s-rx-%u", netdev->name, ri++);
		}

		else if (q_vector->tx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name),
						"%s-tx-%u", netdev->name, ti++);
		}

		else {
			/* skip this unused q_vector */
			continue;
		}

		err = request_irq(entry->vector, &xlnid_msix_clean_rings, 0,
							q_vector->name, q_vector);

		if (err) {
			e_err(probe,
					"request_irq failed for MSIX interrupt '%s' "
					"Error: %d\n",
					q_vector->name, err);
			goto free_queue_irqs;
		}

#ifdef HAVE_IRQ_AFFINITY_HINT

		/* If Flow Director is enabled, set interrupt affinity */
		if (adapter->flags & XLNID_FLAG_FDIR_HASH_CAPABLE) {
			/* assign the mask for this irq */
			irq_set_affinity_hint(entry->vector,
									&q_vector->affinity_mask);
		}

#endif /* HAVE_IRQ_AFFINITY_HINT */
	}

	err = request_irq(adapter->msix_entries[vector].vector,
						xlnid_msix_other, 0, netdev->name, adapter);

	if (err) {
		e_err(probe, "request_irq for msix_other failed: %d\n", err);
		goto free_queue_irqs;
	}

	return XLNID_SUCCESS;

free_queue_irqs:

	while (vector) {
		vector--;
#ifdef HAVE_IRQ_AFFINITY_HINT
		irq_set_affinity_hint(adapter->msix_entries[vector].vector,
								NULL);
#endif
		free_irq(adapter->msix_entries[vector].vector,
					adapter->q_vector[vector]);
	}

	adapter->flags &= ~XLNID_FLAG_MSIX_ENABLED;
	pci_disable_msix(adapter->pdev);
	kfree(adapter->msix_entries);
	adapter->msix_entries = NULL;
	return err;
}

static irqreturn_t xlnid_ptsw_intr(int __always_unused irq, void *data)
{
	struct xlnid_adapter *adapter = data;
	xel_pci_info_t *slave_pinfo =
		xlnid_find_pci_info(adapter->hw.bus.bus_id, 1);
	struct xlnid_hw *slave_hw = slave_pinfo->hw;
	u32 eicr;

	eicr = XLNID_READ_REG_DIRECT(slave_hw, EICR);

	if (eicr & XLNID_EICR_LSC) {
		xlnid_check_lsc(adapter);
	}

	return IRQ_HANDLED;
}

/**
		xlnid_test_intr - legacy mode Interrupt Handler
		@irq: interrupt number
		@data: pointer to a network interface device structure
	**/
static irqreturn_t xlnid_intr(int __always_unused irq, void *data)
{
	struct xlnid_adapter *adapter = data;
	struct xlnid_hw *hw = &adapter->hw;
	struct xlnid_q_vector *q_vector = adapter->q_vector[0];
	u32 eicr;

	XLNID_WRITE_REG_DIRECT(hw, EIMC, XLNID_IRQ_CLEAR_MASK);

	/*  for NAPI, using EIAM to auto-mask tx/rx interrupt bits on read
		therefore no explicit interrupt disable is necessary */
	eicr = XLNID_READ_REG_DIRECT(hw, EICR);

	if (!eicr) {
		/*
			shared interrupt alert!
			make sure interrupts are enabled because the read will
			have disabled interrupts due to EIAM
			Unmask the interrupt that we masked before the EICR read.
		*/
		if (!test_bit(__XLNID_DOWN, &adapter->state)) {
			xlnid_irq_enable(adapter, true, true);
		}

		return IRQ_NONE; /* Not our interrupt */
	}

	if (eicr & XLNID_EICR_LSC) {
		xlnid_check_lsc(adapter);
	}

	if (eicr & (XLNID_EICR_FIFO_OVF | XLNID_EICR_FIFO_UNDF)) {
		printk("Received fifo underflow or overflow interrupt eicr = 0x%08x\r\n",
				eicr);
	}

	if (eicr & XLNID_EICR_ECC) {
		e_info(link, "Received unrecoverable ECC Err, "
				"initiating reset.\n");
		set_bit(__XLNID_RESET_REQUESTED, &adapter->state);
		xlnid_service_event_schedule(adapter);
		XLNID_WRITE_REG_DIRECT(hw, EICR, XLNID_EICR_ECC);
	}

	//xlnid_check_sfp_event(adapter, eicr);
	//xlnid_check_overtemp_event(adapter, eicr);

	//xlnid_check_fan_failure(adapter, eicr);

	/* would disable interrupts here but EIAM disabled it */
	napi_schedule_irqoff(&q_vector->napi);

	/*
		re-enable link(maybe) and non-queue interrupts, no flush.
		xlnid_poll will re-enable the queue interrupts
	*/
	if (!test_bit(__XLNID_DOWN, &adapter->state)) {
		xlnid_irq_enable(adapter, false, false);
	}

	return IRQ_HANDLED;
}

/**
		xlnid_request_irq - initialize interrupts
		@adapter: board private structure

		Attempts to configure interrupts using the best available
		capabilities of the hardware and kernel.
	**/
static int xlnid_request_irq(struct xlnid_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int err;

	if (adapter->flags & XLNID_FLAG_MSIX_ENABLED) {
		err = xlnid_request_msix_irqs(adapter);
	}

	else if (adapter->flags & XLNID_FLAG_MSI_ENABLED)
		err = request_irq(adapter->pdev->irq, &xlnid_intr, 0,
							netdev->name, adapter);
	else
		err = request_irq(adapter->pdev->irq, &xlnid_intr, IRQF_SHARED,
							netdev->name, adapter);

	if (err) {
		e_err(probe, "request_irq failed, Error %d\n", err);
	}

	return err;
}

static void xlnid_free_irq(struct xlnid_adapter *adapter)
{
	int vector;
	xel_pci_info_t *slave_pinfo;
	struct xlnid_adapter *slave_adapter = NULL;

	if (adapter->hw.ptsw_enable) {
		slave_pinfo = xlnid_find_pci_info(adapter->hw.bus.bus_id, 1);
		slave_adapter = slave_pinfo->hw->back;
	}

	if (!(adapter->flags & XLNID_FLAG_MSIX_ENABLED)) {
		free_irq(adapter->pdev->irq, adapter);

		if (adapter->hw.ptsw_enable && adapter->hw.bus.func == 0) {
			if (slave_adapter == NULL ||
					!test_bit(__XLNID_DISABLED,
								&slave_adapter->state)) {
				free_irq(slave_adapter->pdev->irq, adapter);
			}
		}

		return;
	}

	if (!adapter->msix_entries) {
		return;
	}

	for (vector = 0; vector < adapter->num_q_vectors; vector++) {
		struct xlnid_q_vector *q_vector = adapter->q_vector[vector];
		struct msix_entry *entry = &adapter->msix_entries[vector];

		/* free only the irqs that were actually requested */
		if (!q_vector->rx.ring && !q_vector->tx.ring) {
			continue;
		}

#ifdef HAVE_IRQ_AFFINITY_HINT
		/* clear the affinity_mask in the IRQ descriptor */
		irq_set_affinity_hint(entry->vector, NULL);

#endif
		free_irq(entry->vector, q_vector);
	}

	free_irq(adapter->msix_entries[vector].vector, adapter);

	if (adapter->hw.ptsw_enable && adapter->hw.bus.func == 0) {
		if (slave_adapter == NULL ||
				!test_bit(__XLNID_DISABLED, &slave_adapter->state)) {
			free_irq(slave_adapter
						->msix_entries[slave_adapter
									->num_q_vectors]
						.vector,
						adapter);
		}
	}
}

/**
		xlnid_irq_disable - Mask off interrupt generation on the NIC
		@adapter: board private structure
	**/
static inline void xlnid_irq_disable(struct xlnid_adapter *adapter)
{
	XLNID_WRITE_REG_DIRECT(&adapter->hw, EIMC, 0xFFFF0000);
	XLNID_WRITE_REG_DIRECT(&adapter->hw, EIMC_EX(0), ~0);
	XLNID_WRITE_REG_DIRECT(&adapter->hw, EIMC_EX(1), ~0);

	XLNID_WRITE_FLUSH(&adapter->hw);

	if (adapter->flags & XLNID_FLAG_MSIX_ENABLED) {
		int vector;

		for (vector = 0; vector < adapter->num_q_vectors; vector++) {
			synchronize_irq(adapter->msix_entries[vector].vector);
		}

		synchronize_irq(adapter->msix_entries[vector++].vector);
	}

	else {
		synchronize_irq(adapter->pdev->irq);
	}

	if (adapter->hw.ptsw_enable && adapter->hw.bus.func == 0) {
		xel_pci_info_t *slave_pinfo =
			xlnid_find_pci_info(adapter->hw.bus.bus_id, 1);
		struct xlnid_hw *slave_hw = slave_pinfo->hw;
		struct xlnid_adapter *slave_adapter = slave_hw->back;

		if (slave_adapter == NULL ||
				test_bit(__XLNID_DISABLED, &slave_adapter->state)) {
			return;
		}

		XLNID_WRITE_REG_DIRECT(slave_hw, EIMC, ~0);

		if (slave_adapter->flags & XLNID_FLAG_MSIX_ENABLED) {
			synchronize_irq(
				slave_adapter
				->msix_entries[slave_adapter
								->num_q_vectors]
				.vector);
		}

		else {
			synchronize_irq(slave_adapter->pdev->irq);
		}
	}
}

/**
		xlnid_configure_msi_and_legacy - Initialize PIN (INTA...) and MSI interrupts
		@adapter: board private structure

	**/
static void xlnid_configure_msi_and_legacy(struct xlnid_adapter *adapter)
{
	struct xlnid_q_vector *q_vector = adapter->q_vector[0];

	xlnid_write_eitr(q_vector);

	xlnid_set_ivar(adapter, 0, 0, 0);
	xlnid_set_ivar(adapter, 1, 0, 0);

	e_info(hw, "Legacy interrupt IVAR setup done\n");
}

static void xlnid_rx_desc_queue_disable(struct xlnid_adapter *adapter,
										struct xlnid_ring *ring)
{
	struct xlnid_hw *hw = &adapter->hw;
	int wait_loop = XLNID_MAX_RX_DESC_POLL;
	u32 rxdctl;
	u8 reg_idx = ring->reg_idx;

	if (XLNID_REMOVED(hw->hw_addr)) {
		return;
	}

	do {
		msleep(1);
		rxdctl = XLNID_READ_REG_DIRECT(hw, RXDCTL(reg_idx));
	} while (--wait_loop && (rxdctl & XLNID_RXDCTL_ENABLE));

	if (!wait_loop) {
		e_err(drv,
				"RXDCTL.ENABLE on RX queue %d "
				"not cleared within the polling period\n",
				reg_idx);
	}
}

static void xlnid_tx_desc_queue_disable(struct xlnid_adapter *adapter,
										struct xlnid_ring *ring)
{
	struct xlnid_hw *hw = &adapter->hw;
	int wait_loop = XLNID_MAX_RX_DESC_POLL;
	u32 txdctl;
	u8 reg_idx = ring->reg_idx;

	if (XLNID_REMOVED(hw->hw_addr)) {
		return;
	}

	do {
		msleep(1);
		txdctl = XLNID_READ_REG_DIRECT(hw, TXDCTL(reg_idx));
	} while (--wait_loop && (txdctl & XLNID_TXDCTL_ENABLE));

	if (!wait_loop) {
		e_err(drv,
				"TXDCTL.ENABLE on TX queue %d "
				"not cleared within the polling period\n",
				reg_idx);
	}
}

/**
		xlnid_configure_tx_ring - Configure Tx ring after Reset
		@adapter: board private structure
		@ring: structure containing ring specific data

		Configure the Tx descriptor ring after a reset.
	**/
void xlnid_configure_tx_ring(struct xlnid_adapter *adapter,
								struct xlnid_ring *ring)
{
	struct xlnid_hw *hw = &adapter->hw;
	u64 tdba = ring->dma;
	int wait_loop = 10;
	u32 txdctl = XLNID_TXDCTL_ENABLE;
	u8 reg_idx = ring->reg_idx;

#ifdef HAVE_AF_XDP_ZC_SUPPORT
	ring->xsk_pool = NULL;

	if (ring_is_xdp(ring)) {
		ring->xsk_pool = xlnid_xsk_umem(adapter, ring);
	}

#endif
	/* disable queue to avoid issues while updating state */
	XLNID_WRITE_REG_DIRECT(hw, TXDCTL(reg_idx), XLNID_TXDCTL_SWFLSH);
	xlnid_tx_desc_queue_disable(adapter, ring);

	XLNID_WRITE_REG_DIRECT(hw, TDBAL(reg_idx), tdba & DMA_BIT_MASK(32));
	XLNID_WRITE_REG_DIRECT(hw, TDBAH(reg_idx), tdba >> 32);
	XLNID_WRITE_REG_DIRECT(hw, TDLEN(reg_idx),
							ring->count * sizeof(union xlnid_adv_tx_desc));

	/* disable head writeback */
	if (hw->mac.type == xlnid_mac_SKYLAKE) {
		XLNID_WRITE_REG_MAC(hw, SKYLAKE_TDWBAH(reg_idx), 0);
		XLNID_WRITE_REG_MAC(hw, SKYLAKE_TDWBAL(reg_idx), 0);
	}

	/* reset head and tail pointers */
	XLNID_WRITE_REG_DIRECT(hw, TDH(reg_idx), 0);
	XLNID_WRITE_REG_DIRECT(hw, TDT(reg_idx), 0);

	if (hw->mac.type == xlnid_mac_SKYLAKE) {
		ring->tail = adapter->io_addr + SKYLAKE_TDT(reg_idx);
	}

	else {
		ring->tail = adapter->io_addr + WESTLAKE_TDT(reg_idx);
	}

	/* reset ntu and ntc to place SW in sync with hardwdare */
	ring->next_to_clean = 0;
	ring->next_to_use = 0;

	/*
		set WTHRESH to encourage burst writeback, it should not be set
		higher than 1 when:
		- ITR is 0 as it could cause false TX hangs
		- ITR is set to > 100k int/sec and BQL is enabled

		In order to avoid issues WTHRESH + PTHRESH should always be equal
		to or less than the number of on chip descriptors, which is
		currently 40.
	*/
#if 0

	if (!ring->q_vector || (ring->q_vector->itr < XLNID_100K_ITR)) {
		txdctl |= (1 << 16);    /* WTHRESH = 1 */
	}

	else {
		txdctl |= (8 << 16);    /* WTHRESH = 8 */
	}

#endif
	/*
		Setting PTHRESH to 32 both improves performance
		and avoids a TX hang with DFP enabled
	*/
	txdctl |= (1 << 8) | /* HTHRESH = 1 */
				32; /* PTHRESH = 32 */

	/* reinitialize flowdirector state */
	if (adapter->flags & XLNID_FLAG_FDIR_HASH_CAPABLE) {
		ring->atr_sample_rate = adapter->atr_sample_rate;
		ring->atr_count = 0;
		set_bit(__XLNID_TX_FDIR_INIT_DONE, &ring->state);
	}

	else {
		ring->atr_sample_rate = 0;
	}

	/* initialize XPS */
	if (!test_and_set_bit(__XLNID_TX_XPS_INIT_DONE, &ring->state)) {
		struct xlnid_q_vector *q_vector = ring->q_vector;

		if (q_vector)
			netif_set_xps_queue(adapter->netdev,
								&q_vector->affinity_mask,
								ring->queue_index);
	}

	clear_bit(__XLNID_HANG_CHECK_ARMED, &ring->state);

	/* reinitialize tx_buffer_info */
	memset(ring->tx_buffer_info, 0,
			sizeof(struct xlnid_tx_buffer) * ring->count);

	/* enable queue */
	XLNID_WRITE_REG_DIRECT(hw, TXDCTL(reg_idx), txdctl);

	/* poll to verify queue is enabled */
	do {
		msleep(1);
		txdctl = XLNID_READ_REG_DIRECT(hw, TXDCTL(reg_idx));
	} while (--wait_loop && !(txdctl & XLNID_TXDCTL_ENABLE));

	if (!wait_loop) {
		hw_dbg(hw, "Could not enable Tx Queue %d\n", reg_idx);
	}
}

static void xlnid_setup_mtqc(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 mtqc;
	u8 tcs = netdev_get_num_tc(adapter->netdev);

	/* set transmit pool layout */
	if (adapter->flags & XLNID_FLAG_VMDQ_ENABLED) {
		mtqc = XLNID_MTQC_VT_ENA;

		if (tcs > 4) {
			mtqc |= XLNID_MTQC_RT_ENA | XLNID_MTQC_8TC_8TQ;
		}

		else if (tcs > 1) {
			mtqc |= XLNID_MTQC_RT_ENA | XLNID_MTQC_4TC_4TQ;
		}

		else if (adapter->ring_feature[RING_F_VMDQ].mask ==
					XLNID_VMDQ_4Q_MASK) {
			mtqc |= XLNID_MTQC_32VF;
		}

		else {
			mtqc |= XLNID_MTQC_64VF;
		}
	}

	else {
		if (tcs > 4) {
			mtqc = XLNID_MTQC_RT_ENA | XLNID_MTQC_8TC_8TQ;
		}

		else if (tcs > 1) {
			mtqc = XLNID_MTQC_RT_ENA | XLNID_MTQC_4TC_4TQ;
		}

		else {
			u8 max_txq = adapter->num_tx_queues +
							adapter->num_xdp_queues;

			if (max_txq > 63) {
				mtqc = XLNID_MTQC_RT_ENA | XLNID_MTQC_4TC_4TQ;
			}

			else {
				mtqc = XLNID_MTQC_64Q_1PB;
			}
		}
	}

	XLNID_WRITE_REG_MAC(hw, SKYLAKE_MTQC, mtqc);
}

/**
		xlnid_configure_tx - Configure Transmit Unit after Reset
		@adapter: board private structure

		Configure the Tx unit of the MAC after a reset.
	**/
static void xlnid_configure_tx(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 dmatxctl;
	u32 i;

#ifdef CONFIG_NETDEVICES_MULTIQUEUE

	if (adapter->num_tx_queues > 1) {
		adapter->netdev->features |= NETIF_F_MULTI_QUEUE;
	}

	else {
		adapter->netdev->features &= ~NETIF_F_MULTI_QUEUE;
	}

#endif

	if (hw->mac.type == xlnid_mac_SKYLAKE) {
		xlnid_setup_mtqc(adapter);
	}

	/* DMATXCTL.EN must be before Tx queues are enabled */
	dmatxctl = XLNID_READ_REG(hw, DMATXCTL);
	dmatxctl |= XLNID_DMATXCTL_TE;
	XLNID_WRITE_REG(hw, DMATXCTL, dmatxctl);

	/* Setup the HW Tx Head and Tail descriptor pointers */
	for (i = 0; i < adapter->num_tx_queues; i++) {
		xlnid_configure_tx_ring(adapter, adapter->tx_ring[i]);
	}

	for (i = 0; i < adapter->num_xdp_queues; i++) {
		xlnid_configure_tx_ring(adapter, adapter->xdp_ring[i]);
	}
}

static void xlnid_configure_srrctl(struct xlnid_adapter *adapter,
									struct xlnid_ring *rx_ring)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 srrctl = 0;
	u8 reg_idx = rx_ring->reg_idx;

	/* configure the packet buffer length */
#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	srrctl = rx_ring->rx_buf_len;
#else
#ifdef HAVE_AF_XDP_ZC_SUPPORT

	if (rx_ring->xsk_pool) {
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
		u32 xsk_buf_len = rx_ring->xsk_pool->chunk_size_nohr -
							XDP_PACKET_HEADROOM;
#else
		u32 xsk_buf_len = xsk_pool_get_rx_frame_size(rx_ring->xsk_pool);
#endif /* HAVE_MEM_TYPE_XSK_BUFF_POOL */

		srrctl |= xsk_buf_len;
	}

	else if (test_bit(__XLNID_RX_3K_BUFFER, &rx_ring->state)) {
#else

	if (test_bit(__XLNID_RX_3K_BUFFER, &rx_ring->state)) {
#endif /* HAVE_AF_XDP_ZC_SUPPORT */
		srrctl |= XLNID_RXBUFFER_3K;
	}

	else {
		srrctl |= XLNID_RXBUFFER_2K;
	}

#endif /* CONFIG_XLNID_DISABLE_PACKET_SPLIT */

	/* configure descriptor type */
	srrctl |= XLNID_SRRCTL_DESCTYPE_ADV_ONEBUF;

	XLNID_WRITE_REG_DIRECT(hw, SRRCTL(reg_idx), srrctl);
}

/**
	xlnid_rss_indir_tbl_entries - Return RSS indirection table entries
	@adapter: device handle

	- skylake/westlake:     128
*/
u32 xlnid_rss_indir_tbl_entries(struct xlnid_adapter *adapter)
{
	return 128;
}

/**
	xlnid_store_key - Write the RSS key to HW
	@adapter: device handle

	Write the RSS key stored in adapter.rss_key to HW.
*/
void xlnid_store_key(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	int i;

	for (i = 0; i < 10; i++) {
		XLNID_WRITE_REG(hw, RSSRK(i), adapter->rss_key[i]);
	}
}

/**
		xlnid_init_rss_key - Initialize adapter RSS key
		@adapter: device handle

		Allocates and initializes the RSS key if it is not allocated.
	**/
static inline int xlnid_init_rss_key(struct xlnid_adapter *adapter)
{
	u32 *rss_key;

	if (!adapter->rss_key) {
		rss_key = kzalloc(XLNID_RSS_KEY_SIZE, GFP_KERNEL);

		if (unlikely(!rss_key)) {
			return -ENOMEM;
		}

		netdev_rss_key_fill(rss_key, XLNID_RSS_KEY_SIZE);
		adapter->rss_key = rss_key;
	}

	return 0;
}

/**
	xlnid_store_reta - Write the RETA table to HW
	@adapter: device handle

	Write the RSS redirection table stored in adapter.rss_indir_tbl[] to HW.
*/
void xlnid_store_reta(struct xlnid_adapter *adapter)
{
	u32 i, reta_entries = xlnid_rss_indir_tbl_entries(adapter);
	struct xlnid_hw *hw = &adapter->hw;
	u32 reta = 0;
	u32 indices_multi;
	u8 *indir_tbl = adapter->rss_indir_tbl;

	/*  Fill out the redirection table as follows:
		- skylake/westlake: 8 bit wide entries containing 4 bit RSS index
	*/
	indices_multi = 0x1;

	/* Write redirection table to HW */
	for (i = 0; i < reta_entries; i++) {
		reta |= indices_multi * indir_tbl[i] << (i & 0x3) * 8;

		if ((i & 3) == 3) {
			if (i < 128) {
				XLNID_WRITE_REG(hw, RETA(i >> 2), reta);
			}

			reta = 0;
		}
	}
}

static void xlnid_setup_reta(struct xlnid_adapter *adapter)
{
	u32 i, j;
	u32 reta_entries = xlnid_rss_indir_tbl_entries(adapter);
	u16 rss_i = adapter->ring_feature[RING_F_RSS].indices;

	/*  Program table for at least 4 queues w/ SR-IOV so that VFs can
		make full use of any rings they may have.  We will use the
		PSRTYPE register to control how many rings we use within the PF.
	*/
	if ((adapter->flags & XLNID_FLAG_SRIOV_ENABLED) && (rss_i < 4)) {
		rss_i = 4;
	}

	/* Fill out hash function seeds */
	xlnid_store_key(adapter);

	/* Fill out redirection table */
	memset(adapter->rss_indir_tbl, 0, sizeof(adapter->rss_indir_tbl));

	for (i = 0, j = 0; i < reta_entries; i++, j++) {
		if (j == rss_i) {
			j = 0;
		}

		adapter->rss_indir_tbl[i] = j;
	}

	xlnid_store_reta(adapter);
}

static void xlnid_setup_mrqc(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 rxcsum;
	u32 mrqc = 0, rss_field = 0;
	u8 tcs = netdev_get_num_tc(adapter->netdev);

	/* Disable indicating checksum in descriptor, enables RSS hash */
	rxcsum = XLNID_READ_REG(hw, RXCSUM);
	rxcsum |= XLNID_RXCSUM_PCSD;
	XLNID_WRITE_REG(hw, RXCSUM, rxcsum);

	if (adapter->flags & XLNID_FLAG_VMDQ_ENABLED) {
		if (tcs > 4) {
			mrqc = XLNID_MRQC_VMDQRT8TCEN;    /* 8 TCs */
		}

		else if (tcs > 1) {
			mrqc = XLNID_MRQC_VMDQRT4TCEN;    /* 4 TCs */
		}

		else if (adapter->ring_feature[RING_F_VMDQ].mask ==
					XLNID_VMDQ_4Q_MASK) {
			mrqc = XLNID_MRQC_VMDQRSS32EN;
		}

		else {
			mrqc = XLNID_MRQC_VMDQRSS64EN;
		}
	}

	else {
		if (tcs > 4) {
			mrqc = XLNID_MRQC_RTRSS8TCEN;
		}

		else if (tcs > 1) {
			mrqc = XLNID_MRQC_RTRSS4TCEN;
		}

		else {
			mrqc = XLNID_MRQC_RSSEN;
		}
	}

	/* Enable L3/L4 for Tx Switched packets */
	mrqc |= XLNID_MRQC_L3L4TXSWEN;

	/* Perform hash on these packet types */
	rss_field = XLNID_MRQC_RSS_FIELD_IPV4 | XLNID_MRQC_RSS_FIELD_IPV4_TCP |
				XLNID_MRQC_RSS_FIELD_IPV6 | XLNID_MRQC_RSS_FIELD_IPV6_TCP;

	if (adapter->flags2 & XLNID_FLAG2_RSS_FIELD_IPV4_UDP) {
		rss_field |= XLNID_MRQC_RSS_FIELD_IPV4_UDP;
	}

	if (adapter->flags2 & XLNID_FLAG2_RSS_FIELD_IPV6_UDP) {
		rss_field |= XLNID_MRQC_RSS_FIELD_IPV6_UDP;
	}

	xlnid_setup_reta(adapter);
	mrqc |= rss_field;
	XLNID_WRITE_REG(hw, MRQC, mrqc);
}

static void xlnid_rx_desc_queue_enable(struct xlnid_adapter *adapter,
										struct xlnid_ring *ring)
{
	struct xlnid_hw *hw = &adapter->hw;
	int wait_loop = XLNID_MAX_RX_DESC_POLL;
	u32 rxdctl;
	u8 reg_idx = ring->reg_idx;

	if (XLNID_REMOVED(hw->hw_addr)) {
		return;
	}

	do {
		msleep(1);
		rxdctl = XLNID_READ_REG_DIRECT(hw, RXDCTL(reg_idx));
	} while (--wait_loop && !(rxdctl & XLNID_RXDCTL_ENABLE));

	if (!wait_loop) {
		e_err(drv,
				"RXDCTL.ENABLE on Rx queue %d "
				"not set within the polling period\n",
				reg_idx);
	}
}

void xlnid_configure_rx_ring(struct xlnid_adapter *adapter,
								struct xlnid_ring *ring)
{
	struct xlnid_hw *hw = &adapter->hw;
	union xlnid_adv_rx_desc *rx_desc;
	u64 rdba = ring->dma;
	u32 rxdctl;
	u8 reg_idx = ring->reg_idx;

#ifdef HAVE_AF_XDP_ZC_SUPPORT
	xdp_rxq_info_unreg_mem_model(&ring->xdp_rxq);
	ring->xsk_pool = xlnid_xsk_umem(adapter, ring);

	if (ring->xsk_pool) {
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
		ring->zca.free = xlnid_zca_free;
#endif
		WARN_ON(xdp_rxq_info_reg_mem_model(&ring->xdp_rxq,
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
											MEM_TYPE_ZERO_COPY,
											&ring->zca));
#else
											MEM_TYPE_XSK_BUFF_POOL,
											NULL));
		xsk_pool_set_rxq_info(ring->xsk_pool, &ring->xdp_rxq);
#endif /* HAVE_MEM_TYPE_XSK_BUFF_POOL */

	}

	else {
		WARN_ON(xdp_rxq_info_reg_mem_model(&ring->xdp_rxq,
											MEM_TYPE_PAGE_SHARED, NULL));
	}

#endif
	/* disable queue to avoid use of these values while updating state */
	rxdctl = XLNID_READ_REG_DIRECT(hw, RXDCTL(reg_idx));
	rxdctl &= ~XLNID_RXDCTL_ENABLE;

	/* write value back with RXDCTL.ENABLE bit cleared */
	XLNID_WRITE_REG_DIRECT(hw, RXDCTL(reg_idx), rxdctl);
	xlnid_rx_desc_queue_disable(adapter, ring);

	XLNID_WRITE_REG_DIRECT(hw, RDBAL(reg_idx), rdba & DMA_BIT_MASK(32));
	XLNID_WRITE_REG_DIRECT(hw, RDBAH(reg_idx), rdba >> 32);
	XLNID_WRITE_REG_DIRECT(hw, RDLEN(reg_idx),
							ring->count * sizeof(union xlnid_adv_rx_desc));
	/* Force flushing of XLNID_RDLEN to prevent MDD */
	XLNID_WRITE_FLUSH(hw);

	/* reset head and tail pointers */
	XLNID_WRITE_REG_DIRECT(hw, RDH(reg_idx), 0);
	XLNID_WRITE_REG_DIRECT(hw, RDT(reg_idx), 0);

	if (hw->mac.type == xlnid_mac_SKYLAKE) {
		ring->tail = adapter->io_addr + SKYLAKE_RDT(reg_idx);
	}

	else {
		ring->tail = adapter->io_addr + WESTLAKE_RDT(reg_idx);
	}

	/* reset ntu and ntc to place SW in sync with hardwdare */
	ring->next_to_clean = 0;
	ring->next_to_use = 0;
#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	ring->next_to_alloc = 0;
#endif

	xlnid_configure_srrctl(adapter, ring);
	/* In ESX, RSCCTL configuration is done by on demand */
	//xlnid_configure_rscctl(adapter, ring);

	ring->rx_offset = xlnid_rx_offset(ring);

	/* initialize rx_buffer_info */
	memset(ring->rx_buffer_info, 0,
			sizeof(struct xlnid_rx_buffer) * ring->count);

	/* initialize Rx descriptor 0 */
	rx_desc = XLNID_RX_DESC(ring, 0);
	rx_desc->wb.upper.length = 0;

	/* enable receive descriptor ring */
	rxdctl |= XLNID_RXDCTL_ENABLE;
	/* init rx descriptor prefetch threshold */
	rxdctl |= XLNID_RXDCTL_PTHRESH;
	XLNID_WRITE_REG_DIRECT(hw, RXDCTL(reg_idx), rxdctl);

	xlnid_rx_desc_queue_enable(adapter, ring);
#ifdef HAVE_AF_XDP_ZC_SUPPORT

	if (ring->xsk_pool) {
		xlnid_alloc_rx_buffers_zc(ring, xlnid_desc_unused(ring));
	}

	else {
		xlnid_alloc_rx_buffers(ring, xlnid_desc_unused(ring));
	}

#else
	xlnid_alloc_rx_buffers(ring, xlnid_desc_unused(ring));
#endif /* HAVE_AF_XDP_ZC_SUPPORT */
}

static void xlnid_set_rx_buffer_len(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	int max_frame = netdev->mtu + ETH_HLEN + ETH_FCS_LEN;
	struct xlnid_ring *rx_ring;
	int i;
	u32 hlreg0;
	u32 mhadd = 0;
#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	int rx_buf_len;
#endif
	xel_pci_info_t *master_pinfo = NULL, *slave_pinfo = NULL;

	if (hw->ptsw_enable) {
		master_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 0);
		slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
	}

	/* adjust max frame to be at least the size of a standard frame */
	if (max_frame < (ETH_FRAME_LEN + ETH_FCS_LEN)) {
		max_frame = (ETH_FRAME_LEN + ETH_FCS_LEN);
	}

	if (adapter->hw.mac.type == xlnid_mac_WESTLAKE) {
		max_frame = netdev->mtu + ETH_HLEN;

		if (max_frame < ETH_FRAME_LEN) {
			max_frame = ETH_FRAME_LEN;
		}
	}

	mhadd = XLNID_READ_REG(hw, MAXFRS);

	if (max_frame != (mhadd >> XLNID_MHADD_MFS_SHIFT)) {
		mhadd &= ~XLNID_MHADD_MFS_MASK;
		mhadd |= max_frame << XLNID_MHADD_MFS_SHIFT;

		if (hw->ptsw_enable) {
			XLNID_WRITE_REG(master_pinfo->hw, MAXFRS, mhadd);
			XLNID_WRITE_REG(slave_pinfo->hw, MAXFRS, mhadd);
		}

		else {
			XLNID_WRITE_REG(hw, MAXFRS, mhadd);
		}
	}

#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	/* MHADD will allow an extra 4 bytes past for vlan tagged frames */
	max_frame += VLAN_HLEN;

	if (!(adapter->flags2 & XLNID_FLAG2_RSC_ENABLED) &&
			(max_frame <= MAXIMUM_ETHERNET_VLAN_SIZE)) {
		rx_buf_len = MAXIMUM_ETHERNET_VLAN_SIZE;
		/*
			Make best use of allocation by using all but 1K of a
			power of 2 allocation that will be used for skb->head.
		*/
	}

	else if (max_frame <= XLNID_RXBUFFER_3K) {
		rx_buf_len = XLNID_RXBUFFER_3K;
	}

	else if (max_frame <= XLNID_RXBUFFER_7K) {
		rx_buf_len = XLNID_RXBUFFER_7K;
	}

	else if (max_frame <= XLNID_RXBUFFER_15K) {
		rx_buf_len = XLNID_RXBUFFER_15K;
	}

	else {
		rx_buf_len = XLNID_MAX_RXBUFFER;
	}

#endif /* CONFIG_XLNID_DISABLE_PACKET_SPLIT */
	hlreg0 = XLNID_READ_REG(hw, HLREG0);
	/*  set jumbo enable since MHADD.MFS is keeping size locked at
		max_frame
	*/
	hlreg0 |= XLNID_HLREG0_JUMBOEN;

	if (hw->ptsw_enable) {
		XLNID_WRITE_REG(master_pinfo->hw, HLREG0, hlreg0);
		XLNID_WRITE_REG(slave_pinfo->hw, HLREG0, hlreg0);
	} else {
		XLNID_WRITE_REG(hw, HLREG0, hlreg0);
	}

	/*
		Setup the HW Rx Head and Tail Descriptor Pointers and
		the Base and Length of the Rx Descriptor Ring
	*/
	for (i = 0; i < adapter->num_rx_queues; i++) {
		rx_ring = adapter->rx_ring[i];

		clear_ring_rsc_enabled(rx_ring);

		if (adapter->flags2 & XLNID_FLAG2_RSC_ENABLED) {
			set_ring_rsc_enabled(rx_ring);
		}

#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT
		clear_bit(__XLNID_RX_3K_BUFFER, &rx_ring->state);
		clear_bit(__XLNID_RX_BUILD_SKB_ENABLED, &rx_ring->state);
#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC

		if (adapter->flags2 & XLNID_FLAG2_RX_LEGACY) {
			continue;
		}

		set_bit(__XLNID_RX_BUILD_SKB_ENABLED, &rx_ring->state);

#if (PAGE_SIZE < 8192)

		if (adapter->flags2 & XLNID_FLAG2_RSC_ENABLED) {
			set_bit(__XLNID_RX_3K_BUFFER, &rx_ring->state);
		}

		if (XLNID_2K_TOO_SMALL_WITH_PADDING ||
				(max_frame > (ETH_FRAME_LEN + ETH_FCS_LEN))) {
			set_bit(__XLNID_RX_3K_BUFFER, &rx_ring->state);
		}

#endif
#else /* !HAVE_SWIOTLB_SKIP_CPU_SYNC */

		adapter->flags2 |= XLNID_FLAG2_RX_LEGACY;
#endif /* !HAVE_SWIOTLB_SKIP_CPU_SYNC */
#else /* CONFIG_XLNID_DISABLE_PACKET_SPLIT */

		rx_ring->rx_buf_len = rx_buf_len;
#endif /* CONFIG_XLNID_DISABLE_PACKET_SPLIT */
	}
}

static void xlnid_setup_rdrxctl(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 rdrxctl = XLNID_READ_REG_MAC(hw, SKYLAKE_RDRXCTL);

	rdrxctl &= ~XLNID_RDRXCTL_RSCFRSTSIZE;
	/* hardware requires some bits to be set by default */
	rdrxctl |= (XLNID_RDRXCTL_RSCACKC | XLNID_RDRXCTL_FCOE_WRFIX);
	rdrxctl |= XLNID_RDRXCTL_CRCSTRIP;

	XLNID_WRITE_REG_MAC(hw, SKYLAKE_RDRXCTL, rdrxctl);
}

/**
	xlnid_configure_rx - Configure Receive Unit after Reset
	@adapter: board private structure

	Configure the Rx unit of the MAC after a reset.
**/
static void xlnid_configure_rx(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	int i;
	u32 rxctrl;

	/* disable receives while setting up the descriptors */
	xlnid_disable_rx(hw);

	//xlnid_setup_psrtype(adapter);
	if (hw->mac.type == xlnid_mac_SKYLAKE) {
		xlnid_setup_rdrxctl(adapter);
	}

	/* Program registers for the distribution of queues */
	xlnid_setup_mrqc(adapter);

	/* set_rx_buffer_len must be called before ring initialization */
	xlnid_set_rx_buffer_len(adapter);

	/*
		Setup the HW Rx Head and Tail Descriptor Pointers and
		the Base and Length of the Rx Descriptor Ring
	*/
	for (i = 0; i < adapter->num_rx_queues; i++) {
		xlnid_configure_rx_ring(adapter, adapter->rx_ring[i]);
	}

	rxctrl = XLNID_READ_REG(hw, RXCTRL);

	/* enable all receives */
	rxctrl |= XLNID_RXCTRL_RXEN;
	hw->mac.ops.enable_rx_dma(hw, rxctrl);
}

#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
#ifdef HAVE_INT_NDO_VLAN_RX_ADD_VID
#ifdef NETIF_F_HW_VLAN_CTAG_TX
static int xlnid_vlan_rx_add_vid(struct net_device *netdev,
									__always_unused __be16 proto, u16 vid)
#else /* !NETIF_F_HW_VLAN_CTAG_TX */
static int xlnid_vlan_rx_add_vid(struct net_device *netdev, u16 vid)
#endif /* NETIF_F_HW_VLAN_CTAG_TX */
#else /* !HAVE_INT_NDO_VLAN_RX_ADD_VID */
static void xlnid_vlan_rx_add_vid(struct net_device *netdev, u16 vid)
#endif /* HAVE_INT_NDO_VLAN_RX_ADD_VID */
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	int pool_ndx = VMDQ_P(0);

	/* add VID to filter table */
	if (hw->mac.ops.set_vfta) {
#ifndef HAVE_VLAN_RX_REGISTER

		if (vid < VLAN_N_VID) {
			set_bit(vid, adapter->active_vlans);
		}

#endif

		if (!vid || !(adapter->flags2 & XLNID_FLAG2_VLAN_PROMISC)) {
			hw->mac.ops.set_vfta(hw, vid, pool_ndx, true, !!vid);
		}

		if (adapter->flags & XLNID_FLAG_VMDQ_ENABLED) {
			int i;

			/* enable vlan id for all pools */
			for (i = 1; i < adapter->num_rx_pools; i++)
				hw->mac.ops.set_vfta(hw, vid, VMDQ_P(i), true,
#ifdef HAVE_VLAN_RX_REGISTER
										false);

#else
										true);
#endif
		}
	}

#ifndef HAVE_NETDEV_VLAN_FEATURES

	/*
		Copy feature flags from netdev to the vlan netdev for this vid.
		This allows things like TSO to bubble down to our vlan device.
		Some vlans, such as VLAN 0 for DCB will not have a v_netdev so
		we will not have a netdev that needs updating.
	*/
	if (adapter->vlgrp) {
		struct vlan_group *vlgrp = adapter->vlgrp;
		struct net_device *v_netdev = vlan_group_get_device(vlgrp, vid);

		if (v_netdev) {
			v_netdev->features |= netdev->features;
			vlan_group_set_device(vlgrp, vid, v_netdev);
		}
	}

#endif /* HAVE_NETDEV_VLAN_FEATURES */
#ifdef HAVE_INT_NDO_VLAN_RX_ADD_VID
	return 0;
#endif
}

#ifdef HAVE_INT_NDO_VLAN_RX_ADD_VID
#ifdef NETIF_F_HW_VLAN_CTAG_RX
static int xlnid_vlan_rx_kill_vid(struct net_device *netdev,
									__always_unused __be16 proto, u16 vid)
#else /* !NETIF_F_HW_VLAN_CTAG_RX */
static int xlnid_vlan_rx_kill_vid(struct net_device *netdev, u16 vid)
#endif /* NETIF_F_HW_VLAN_CTAG_RX */
#else
static void xlnid_vlan_rx_kill_vid(struct net_device *netdev, u16 vid)
#endif
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	int pool_ndx = VMDQ_P(0);

	/* User is not allowed to remove vlan ID 0 */
	if (!vid)
#ifdef HAVE_INT_NDO_VLAN_RX_ADD_VID
		return 0;

#else
		return;
#endif

#ifdef HAVE_VLAN_RX_REGISTER

	if (!test_bit(__XLNID_DOWN, &adapter->state)) {
		xlnid_irq_disable(adapter);
	}

	vlan_group_set_device(adapter->vlgrp, vid, NULL);

	if (!test_bit(__XLNID_DOWN, &adapter->state)) {
		xlnid_irq_enable(adapter, true, true);
	}

#endif /* HAVE_VLAN_RX_REGISTER */

	/* remove VID from filter table */
	if (hw->mac.ops.set_vfta) {
		if (vid && !(adapter->flags2 & XLNID_FLAG2_VLAN_PROMISC)) {
			hw->mac.ops.set_vfta(hw, vid, pool_ndx, false, true);
		}

		if (adapter->flags & XLNID_FLAG_VMDQ_ENABLED) {
			int i;

			/* remove vlan id from all pools */
			for (i = 1; i < adapter->num_rx_pools; i++)
				hw->mac.ops.set_vfta(hw, vid, VMDQ_P(i), false,
										true);
		}
	}

#ifndef HAVE_VLAN_RX_REGISTER

	clear_bit(vid, adapter->active_vlans);
#endif
#ifdef HAVE_INT_NDO_VLAN_RX_ADD_VID
	return 0;
#endif
}

#ifdef HAVE_8021P_SUPPORT
/**
		xlnid_vlan_strip_disable - helper to disable vlan tag stripping
		@adapter: driver data
*/
void xlnid_vlan_strip_disable(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 vlnctrl;

	/* leave vlan tag stripping enabled for DCB */
	if (adapter->flags & XLNID_FLAG_DCB_ENABLED) {
		return;
	}

	vlnctrl = XLNID_READ_REG(hw, RXDCTL_VME);
	vlnctrl &= ~XLNID_RXDCTL_VME_VME;
	XLNID_WRITE_REG(hw, RXDCTL_VME, vlnctrl);
}

#endif /* HAVE_8021P_SUPPORT */

/**
	xlnid_vlan_strip_enable - helper to enable vlan tag stripping
	@adapter: driver data
*/
void xlnid_vlan_strip_enable(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 vlnctrl;

	vlnctrl = XLNID_READ_REG(hw, RXDCTL_VME);
	vlnctrl |= XLNID_RXDCTL_VME_VME;
	XLNID_WRITE_REG(hw, RXDCTL_VME, vlnctrl);
}

#ifndef HAVE_VLAN_RX_REGISTER
static void xlnid_vlan_promisc_enable(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 vlnctrl;

	vlnctrl = XLNID_READ_REG(hw, VLNCTRL);

	if (adapter->flags & XLNID_FLAG_VMDQ_ENABLED) {
		/* we need to keep the VLAN filter on in SRIOV */
		vlnctrl |= XLNID_VLNCTRL_VFE;
		XLNID_WRITE_REG(hw, VLNCTRL, vlnctrl);
	}

	else {
		vlnctrl &= ~XLNID_VLNCTRL_VFE;
		XLNID_WRITE_REG(hw, VLNCTRL, vlnctrl);
		return;
	}

	/* We are already in VLAN promisc, nothing to do */
	if (adapter->flags2 & XLNID_FLAG2_VLAN_PROMISC) {
		return;
	}

	/* Set flag so we don't redo unnecessary work */
	adapter->flags2 |= XLNID_FLAG2_VLAN_PROMISC;
}

static void xlnid_vlan_promisc_disable(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 vlnctrl;

	/* configure vlan filtering */
	vlnctrl = XLNID_READ_REG(hw, VLNCTRL);
	vlnctrl |= XLNID_VLNCTRL_VFE;
	XLNID_WRITE_REG(hw, VLNCTRL, vlnctrl);

	if (!(adapter->flags & XLNID_FLAG_VMDQ_ENABLED)) {
		return;
	}

	/* We are not in VLAN promisc, nothing to do */
	if (!(adapter->flags2 & XLNID_FLAG2_VLAN_PROMISC)) {
		return;
	}

	/* Set flag so we don't redo unnecessary work */
	adapter->flags2 &= ~XLNID_FLAG2_VLAN_PROMISC;
}
#endif /* HAVE_VLAN_RX_REGISTER */

#ifdef HAVE_VLAN_RX_REGISTER
static void xlnid_vlan_mode(struct net_device *netdev, struct vlan_group *grp)
#else
void xlnid_vlan_mode(struct net_device *netdev, u32 features)
#endif
{
#if defined(HAVE_VLAN_RX_REGISTER) || defined(HAVE_8021P_SUPPORT)
	struct xlnid_adapter *adapter = netdev_priv(netdev);
#endif
#ifdef HAVE_8021P_SUPPORT
	bool enable;
#endif

#ifdef HAVE_VLAN_RX_REGISTER

	if (!test_bit(__XLNID_DOWN, &adapter->state)) {
		xlnid_irq_disable(adapter);
	}

	adapter->vlgrp = grp;

	if (!test_bit(__XLNID_DOWN, &adapter->state)) {
		xlnid_irq_enable(adapter, true, true);
	}

#endif
#ifdef HAVE_8021P_SUPPORT
#ifdef HAVE_VLAN_RX_REGISTER
	enable = (grp || (adapter->flags & XLNID_FLAG_DCB_ENABLED));
#else
#ifdef NETIF_F_HW_VLAN_CTAG_RX
	enable = !!(features & NETIF_F_HW_VLAN_CTAG_RX);
#else
	enable = !!(features & NETIF_F_HW_VLAN_RX);
#endif /* NETIF_F_HW_VLAN_CTAG_RX */
#endif /* HAVE_VLAN_RX_REGISTER */

	if (enable)
		/* enable VLAN tag insert/strip */
	{
		xlnid_vlan_strip_enable(adapter);
	}

	else
		/* disable VLAN tag insert/strip */
	{
		xlnid_vlan_strip_disable(adapter);
	}

#endif /* HAVE_8021P_SUPPORT */
}

static void xlnid_restore_vlan(struct xlnid_adapter *adapter)
{
	u16 vid = 1;

#ifdef HAVE_VLAN_RX_REGISTER

	xlnid_vlan_mode(adapter->netdev, adapter->vlgrp);

	/*
		add vlan ID 0 and enable vlan tag stripping so we
		always accept priority-tagged traffic
	*/
#ifdef NETIF_F_HW_VLAN_CTAG_RX
	xlnid_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q), 0);
#else
	xlnid_vlan_rx_add_vid(adapter->netdev, 0);
#endif
#ifndef HAVE_8021P_SUPPORT
	xlnid_vlan_strip_enable(adapter);
#endif

	if (adapter->vlgrp) {
		for (; vid < VLAN_N_VID; vid++) {
			if (!vlan_group_get_device(adapter->vlgrp, vid)) {
				continue;
			}

#ifdef NETIF_F_HW_VLAN_CTAG_RX
			xlnid_vlan_rx_add_vid(adapter->netdev,
									htons(ETH_P_8021Q), vid);
#else
			xlnid_vlan_rx_add_vid(adapter->netdev, vid);
#endif
		}
	}

#else /* !HAVE_VLAN_RX_REGISTER */

#ifdef NETIF_F_HW_VLAN_CTAG_RX
	xlnid_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q), 0);
#else
	xlnid_vlan_rx_add_vid(adapter->netdev, 0);
#endif

	for_each_set_bit_from(vid, adapter->active_vlans, VLAN_N_VID)
#ifdef NETIF_F_HW_VLAN_CTAG_RX
	xlnid_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q), vid);
#else
	xlnid_vlan_rx_add_vid(adapter->netdev, vid);
#endif
#endif /* HAVE_VLAN_RX_REGISTER */
}

#endif
static u8 *xlnid_addr_list_itr(struct xlnid_hw __maybe_unused *hw,
								u8 **mc_addr_ptr, u32 *vmdq)
{
#ifdef NETDEV_HW_ADDR_T_MULTICAST
	struct netdev_hw_addr *mc_ptr;
#else
	struct dev_mc_list *mc_ptr;
#endif
#if 0
	struct xlnid_adapter *adapter = hw->back;
#endif /* CONFIG_PCI_IOV */
	u8 *addr = *mc_addr_ptr;

	/*  VMDQ_P implicitely uses the adapter struct when CONFIG_PCI_IOV is
		defined, so we have to wrap the pointer above correctly to prevent
		a warning.
	*/
	*vmdq = VMDQ_P(0);

#ifdef NETDEV_HW_ADDR_T_MULTICAST
	mc_ptr = container_of(addr, struct netdev_hw_addr, addr[0]);

	if (mc_ptr->list.next) {
		struct netdev_hw_addr *ha;

		ha = list_entry(mc_ptr->list.next, struct netdev_hw_addr, list);
		*mc_addr_ptr = ha->addr;
	}

#else
	mc_ptr = container_of(addr, struct dev_mc_list, dmi_addr[0]);

	if (mc_ptr->next) {
		*mc_addr_ptr = mc_ptr->next->dmi_addr;
	}

#endif

	else {
		*mc_addr_ptr = NULL;
	}

	return addr;
}

/**
	xlnid_write_mc_addr_list - write multicast addresses to MTA
	@netdev: network interface device structure

	Writes multicast address list to the MTA hash table.
	Returns: -ENOMEM on failure
					0 on no addresses written
					X on writing X addresses to MTA
**/
int xlnid_write_mc_addr_list(struct net_device *netdev)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;

#ifdef NETDEV_HW_ADDR_T_MULTICAST
	struct netdev_hw_addr *ha;
#endif
	u8 *addr_list = NULL;
	int addr_count = 0;

	if (!hw->mac.ops.update_mc_addr_list) {
		return -ENOMEM;
	}

	if (!netif_running(netdev)) {
		return 0;
	}

#if 0
	xlnid_restore_vf_multicasts(adapter);
#endif

	if (netdev_mc_empty(netdev)) {
		hw->mac.ops.update_mc_addr_list(hw, NULL, 0,
										xlnid_addr_list_itr, false);

		if (hw->ptsw_enable && hw->bus.func == 0) {
			xel_pci_info_t *slave_pinfo =
				xlnid_find_pci_info(hw->bus.bus_id, 1);

			hw->mac.ops.update_mc_addr_list(slave_pinfo->hw, NULL,
											0, xlnid_addr_list_itr,
											false);
		}
	}

	else {
#ifdef NETDEV_HW_ADDR_T_MULTICAST
		ha = list_first_entry(&netdev->mc.list, struct netdev_hw_addr,
								list);
		addr_list = ha->addr;
#else
		addr_list = netdev->mc_list->dmi_addr;
#endif
		addr_count = netdev_mc_count(netdev);

		hw->mac.ops.update_mc_addr_list(hw, addr_list, addr_count,
										xlnid_addr_list_itr, false);

		if (hw->ptsw_enable && (hw->bus.func == 0)) {
			xel_pci_info_t *slave_pinfo =
				xlnid_find_pci_info(hw->bus.bus_id, 1);

			hw->mac.ops.update_mc_addr_list(slave_pinfo->hw,
											addr_list, addr_count,
											xlnid_addr_list_itr,
											false);
		}
	}

	return addr_count;
}

static void xlnid_sync_mac_table(struct xlnid_adapter *adapter)
{
	struct xlnid_mac_addr *mac_table = &adapter->mac_table[0];
	struct xlnid_hw *hw = &adapter->hw;
	int i;

	for (i = 0; i < hw->mac.num_rar_entries; i++, mac_table++) {
		if (!(mac_table->state & XLNID_MAC_STATE_MODIFIED)) {
			continue;
		}

		mac_table->state &= ~XLNID_MAC_STATE_MODIFIED;

		if (mac_table->state & XLNID_MAC_STATE_IN_USE)
			hw->mac.ops.set_rar(hw, i, mac_table->addr,
								mac_table->pool, XLNID_RAH_AV);
		else {
			hw->mac.ops.clear_rar(hw, i);
		}
	}
}

int xlnid_available_rars(struct xlnid_adapter *adapter, u16 pool)
{
	struct xlnid_mac_addr *mac_table = &adapter->mac_table[0];
	struct xlnid_hw *hw = &adapter->hw;
	int i, count = 0;

	for (i = 0; i < hw->mac.num_rar_entries; i++, mac_table++) {
		/* do not count default RAR as available */
		if (mac_table->state & XLNID_MAC_STATE_DEFAULT) {
			continue;
		}

		/* only count unused and addresses that belong to us */
		if (mac_table->state & XLNID_MAC_STATE_IN_USE) {
			if (mac_table->pool != pool) {
				continue;
			}
		}

		count++;
	}

	return count;
}

/* this function destroys the first RAR entry */
static void xlnid_mac_set_default_filter(struct xlnid_adapter *adapter)
{
	struct xlnid_mac_addr *mac_table = &adapter->mac_table[0];
	struct xlnid_hw *hw = &adapter->hw;

	ether_addr_copy(mac_table->addr, hw->mac.addr);
	mac_table->pool = VMDQ_P(0);

	mac_table->state = XLNID_MAC_STATE_DEFAULT | XLNID_MAC_STATE_IN_USE;

	hw->mac.ops.set_rar(hw, 0, mac_table->addr, mac_table->pool,
						XLNID_RAH_AV);
}

int xlnid_add_mac_filter(struct xlnid_adapter *adapter, const u8 *addr,
							u16 pool)
{
	struct xlnid_mac_addr *mac_table = &adapter->mac_table[0];
	struct xlnid_hw *hw = &adapter->hw;
	int i;

	if (is_zero_ether_addr(addr)) {
		return -EINVAL;
	}

	for (i = 0; i < hw->mac.num_rar_entries; i++, mac_table++) {
		if (mac_table->state & XLNID_MAC_STATE_IN_USE) {
			continue;
		}

		ether_addr_copy(mac_table->addr, addr);
		mac_table->pool = pool;

		mac_table->state |= XLNID_MAC_STATE_MODIFIED |
							XLNID_MAC_STATE_IN_USE;

		xlnid_sync_mac_table(adapter);

		return i;
	}

	return -ENOMEM;
}

void xlnid_flush_sw_mac_table(struct xlnid_adapter *adapter)
{
	struct xlnid_mac_addr *mac_table = &adapter->mac_table[0];
	struct xlnid_hw *hw = &adapter->hw;
	int i;

	for (i = 0; i < hw->mac.num_rar_entries; i++, mac_table++) {
		mac_table->state |= XLNID_MAC_STATE_MODIFIED;
		mac_table->state &= ~XLNID_MAC_STATE_IN_USE;
	}

	xlnid_sync_mac_table(adapter);
}

int xlnid_del_mac_filter(struct xlnid_adapter *adapter, const u8 *addr,
							u16 pool)
{
	struct xlnid_mac_addr *mac_table = &adapter->mac_table[0];
	struct xlnid_hw *hw = &adapter->hw;
	int i;

	if (is_zero_ether_addr(addr)) {
		return -EINVAL;
	}

	/* search table for addr, if found clear IN USE flag and sync */
	for (i = 0; i < hw->mac.num_rar_entries; i++, mac_table++) {
		/* we can only delete an entry if it is in use */
		if (!(mac_table->state & XLNID_MAC_STATE_IN_USE)) {
			continue;
		}

		/* we only care about entries that belong to the given pool */
		if (mac_table->pool != pool) {
			continue;
		}

		/* we only care about a specific MAC address */
		if (!ether_addr_equal(addr, mac_table->addr)) {
			continue;
		}

		mac_table->state |= XLNID_MAC_STATE_MODIFIED;
		mac_table->state &= ~XLNID_MAC_STATE_IN_USE;

		xlnid_sync_mac_table(adapter);

		return 0;
	}

	return -ENOMEM;
}

#ifdef HAVE_SET_RX_MODE
static int xlnid_uc_sync(struct net_device *netdev,
							const unsigned char *addr)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	int ret;

	ret = xlnid_add_mac_filter(adapter, addr, VMDQ_P(0));

	return min_t(int, ret, 0);
}

static int xlnid_uc_unsync(struct net_device *netdev,
							const unsigned char *addr)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	xlnid_del_mac_filter(adapter, addr, VMDQ_P(0));

	return 0;
}

#endif
/**
	xlnid_set_rx_mode - Unicast, Multicast and Promiscuous mode set
	@netdev: network interface device structure

	The set_rx_method entry point is called whenever the unicast/multicast
	address list or the network interface flags are updated.  This routine is
	responsible for configuring the hardware for proper unicast, multicast and
	promiscuous mode.
**/
void xlnid_set_rx_mode(struct net_device *netdev)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	u32 fctrl, vmolr = XLNID_VMOLR_BAM | XLNID_VMOLR_AUPE;
#if defined(HAVE_VLAN_RX_REGISTER)
	u32 vlnctrl;
#endif
#if defined(NETIF_F_HW_VLAN_CTAG_FILTER) || defined(NETIF_F_HW_VLAN_FILTER)
	netdev_features_t features = netdev->features;
#endif
	int count;

	/* Check for Promiscuous and All Multicast modes */
	fctrl = XLNID_READ_REG(hw, FCTRL);
#if defined(HAVE_VLAN_RX_REGISTER)
	vlnctrl = XLNID_READ_REG(hw, VLNCTRL);
#endif

	/* set all bits that we expect to always be set */
	fctrl |= XLNID_FCTRL_BAM;
	fctrl |= XLNID_FCTRL_DPF; /* discard pause frames when FC enabled */
	fctrl |= XLNID_FCTRL_PMCF;

	/* clear the bits we are changing the status of */
	fctrl &= ~(XLNID_FCTRL_UPE | XLNID_FCTRL_MPE);
#if defined(HAVE_VLAN_RX_REGISTER)
	vlnctrl &= ~(XLNID_VLNCTRL_VFE | XLNID_VLNCTRL_CFIEN);
#endif

	if (netdev->flags & IFF_PROMISC) {
		hw->addr_ctrl.user_set_promisc = true;
		fctrl |= (XLNID_FCTRL_UPE | XLNID_FCTRL_MPE);
		vmolr |= XLNID_VMOLR_MPE;
#ifdef HAVE_VLAN_RX_REGISTER

		/*  Only disable hardware filter vlans in promiscuous mode
			if SR-IOV and VMDQ are disabled - otherwise ensure
			that hardware VLAN filters remain enabled.
		*/
		if ((adapter->flags &
				(XLNID_FLAG_VMDQ_ENABLED | XLNID_FLAG_SRIOV_ENABLED))) {
			vlnctrl |= (XLNID_VLNCTRL_VFE | XLNID_VLNCTRL_CFIEN);
		}

#endif
#ifdef NETIF_F_HW_VLAN_CTAG_FILTER
		features &= ~NETIF_F_HW_VLAN_CTAG_FILTER;
#endif
#ifdef NETIF_F_HW_VLAN_FILTER
		features &= ~NETIF_F_HW_VLAN_FILTER;
#endif
	}

	else {
		if (netdev->flags & IFF_ALLMULTI) {
			fctrl |= XLNID_FCTRL_MPE;
			vmolr |= XLNID_VMOLR_MPE;
		}

		hw->addr_ctrl.user_set_promisc = false;
#if defined(HAVE_VLAN_RX_REGISTER)
		/* enable hardware vlan filtering */
		vlnctrl |= XLNID_VLNCTRL_VFE;
#endif
	}

#ifdef HAVE_SET_RX_MODE

	/*
		Write addresses to available RAR registers, if there is not
		sufficient space to store all the addresses then enable
		unicast promiscuous mode
	*/
	if (__dev_uc_sync(netdev, xlnid_uc_sync, xlnid_uc_unsync)) {
		fctrl |= XLNID_FCTRL_UPE;
		vmolr |= XLNID_VMOLR_ROPE;
	}

#endif
	/*
		Write addresses to the MTA, if the attempt fails
		then we should just turn on promiscuous mode so
		that we can at least receive multicast traffic
	*/
	count = xlnid_write_mc_addr_list(netdev);

	if (count < 0) {
		fctrl |= XLNID_FCTRL_MPE;
		vmolr |= XLNID_VMOLR_MPE;
	}

	else if (count) {
		vmolr |= XLNID_VMOLR_ROMPE;
	}

	XLNID_WRITE_REG(hw, FCTRL, fctrl);

	if (hw->mac.type == xlnid_mac_WESTLAKE && hw->ptsw_enable &&
			adapter->pdev->devfn == 0) {
		xel_pci_info_t *slave_pinfo =
			xlnid_find_pci_info(hw->bus.bus_id, 1);

		XLNID_WRITE_REG_MAC(slave_pinfo->hw, WESTLAKE_FCTRL, fctrl);
	}

#ifdef HAVE_8021P_SUPPORT
#ifdef NETIF_F_HW_VLAN_CTAG_RX

	if (features & NETIF_F_HW_VLAN_CTAG_RX)
#else
	if (features & NETIF_F_HW_VLAN_RX)
#endif
		xlnid_vlan_strip_enable(adapter);
	else {
		xlnid_vlan_strip_disable(adapter);
	}

#endif /* HAVE_8021P_SUPPORT */

#if defined(NETIF_F_HW_VLAN_CTAG_FILTER)

	if (features & NETIF_F_HW_VLAN_CTAG_FILTER) {
		xlnid_vlan_promisc_disable(adapter);
	}

	else {
		xlnid_vlan_promisc_enable(adapter);
	}

#elif defined(NETIF_F_HW_VLAN_FILTER) && !defined(HAVE_VLAN_RX_REGISTER)

	if (features & NETIF_F_HW_VLAN_FILTER) {
		xlnid_vlan_promisc_disable(adapter);
	}

	else {
		xlnid_vlan_promisc_enable(adapter);
	}

#elif defined(HAVE_VLAN_RX_REGISTER)
	XLNID_WRITE_REG(hw, VLNCTRL, vlnctrl);
#endif /* NETIF_F_HW_VLAN_CTAG_FILTER */
}

static void xlnid_napi_enable_all(struct xlnid_adapter *adapter)
{
	struct xlnid_q_vector *q_vector;
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++) {
		q_vector = adapter->q_vector[q_idx];
#ifdef HAVE_NDO_BUSY_POLL
		xlnid_qv_init_lock(adapter->q_vector[q_idx]);
#endif
		napi_enable(&q_vector->napi);
	}
}

static void xlnid_napi_disable_all(struct xlnid_adapter *adapter)
{
	struct xlnid_q_vector *q_vector;
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++) {
		q_vector = adapter->q_vector[q_idx];
		napi_disable(&q_vector->napi);
#ifdef HAVE_NDO_BUSY_POLL

		while (!xlnid_qv_disable(adapter->q_vector[q_idx])) {
			pr_info("QV %d locked\n", q_idx);
			usleep_range(1000, 20000);
		}

#endif
	}
}

#if defined(HAVE_UDP_ENC_RX_OFFLOAD) || defined(HAVE_VXLAN_RX_OFFLOAD)
void xlnid_clear_udp_tunnel_port(struct xlnid_adapter *adapter, u32 mask)
{
	//struct xlnid_hw *hw = &adapter->hw;
	//u32 vxlanctrl;

	if (!(adapter->flags & (XLNID_FLAG_VXLAN_OFFLOAD_CAPABLE |
							XLNID_FLAG_GENEVE_OFFLOAD_CAPABLE))) {
		return;
	}

	//vxlanctrl = XLNID_READ_REG(hw, XLNID_VXLANCTRL) & ~mask;
	//XLNID_WRITE_REG(hw, XLNID_VXLANCTRL, vxlanctrl);

	if (mask & XLNID_VXLANCTRL_VXLAN_UDPPORT_MASK) {
		adapter->vxlan_port = 0;
	}

#ifdef HAVE_UDP_ENC_RX_OFFLOAD

	if (mask & XLNID_VXLANCTRL_GENEVE_UDPPORT_MASK) {
		adapter->geneve_port = 0;
	}

#endif /* HAVE_UDP_ENC_RX_OFFLOAD */
}
#endif /* HAVE_UDP_ENC_RX_OFFLOAD || HAVE_VXLAN_RX_OFFLOAD */

#ifdef NETIF_F_GSO_PARTIAL
/* NETIF_F_GSO_IPXIP4/6 may not be defined in all distributions */
#ifndef NETIF_F_GSO_IPXIP4
#define NETIF_F_GSO_IPXIP4 0
#endif
#ifndef NETIF_F_GSO_IPXIP6
#define NETIF_F_GSO_IPXIP6 0
#endif
#define XLNID_GSO_PARTIAL_FEATURES                                     \
	(NETIF_F_GSO_GRE | NETIF_F_GSO_GRE_CSUM | NETIF_F_GSO_IPXIP4 | \
		NETIF_F_GSO_IPXIP6 | NETIF_F_GSO_UDP_TUNNEL |                 \
		NETIF_F_GSO_UDP_TUNNEL_CSUM)
#endif /* NETIF_F_GSO_PARTIAL */

static inline unsigned long xlnid_tso_features(void)
{
	unsigned long features = 0;

#ifdef NETIF_F_TSO
	features |= NETIF_F_TSO;
#endif /* NETIF_F_TSO */
#ifdef NETIF_F_TSO6
	features |= NETIF_F_TSO6;
#endif /* NETIF_F_TSO6 */
#ifdef NETIF_F_GSO_PARTIAL
	features |= NETIF_F_GSO_PARTIAL | XLNID_GSO_PARTIAL_FEATURES;
#endif

	return features;
}

#ifndef XLNID_NO_LLI
static void xlnid_configure_lli_westlake(struct xlnid_adapter *adapter)
{
	u16 port;

	if (adapter->lli_etype) {
		XLNID_WRITE_REG(&adapter->hw, L34T_IMIR(0),
						(XLNID_IMIR_LLI_EN | XLNID_IMIR_SIZE_BP |
							XLNID_IMIR_CTRL_BP));
		XLNID_WRITE_REG(&adapter->hw, ETQS(0), XLNID_ETQS_LLI);
		XLNID_WRITE_REG(&adapter->hw, ETQF(0),
						(adapter->lli_etype | XLNID_ETQF_FILTER_EN));
	}

	if (adapter->lli_port) {
		port = swab16(adapter->lli_port);
		XLNID_WRITE_REG(&adapter->hw, L34T_IMIR(0),
						(XLNID_IMIR_LLI_EN | XLNID_IMIR_SIZE_BP |
							XLNID_IMIR_CTRL_BP));
		XLNID_WRITE_REG(&adapter->hw, FTQF(0),
						(XLNID_FTQF_POOL_MASK_EN |
							(XLNID_FTQF_PRIORITY_MASK
							<< XLNID_FTQF_PRIORITY_SHIFT) |
							(XLNID_FTQF_DEST_PORT_MASK
							<< XLNID_FTQF_5TUPLE_MASK_SHIFT)));
		XLNID_WRITE_REG(&adapter->hw, SDPQF(0), (port << 16));
	}

	if (adapter->flags & XLNID_FLAG_LLI_PUSH) {
		switch (adapter->hw.mac.type) {
		case xlnid_mac_WESTLAKE:
			XLNID_WRITE_REG(
			&adapter->hw, L34T_IMIR(0),
			(XLNID_IMIR_LLI_EN | XLNID_IMIR_SIZE_BP |
			XLNID_IMIR_CTRL_PSH | XLNID_IMIR_CTRL_SYN |
			XLNID_IMIR_CTRL_URG | XLNID_IMIR_CTRL_ACK |
			XLNID_IMIR_CTRL_RST | XLNID_IMIR_CTRL_FIN));
			XLNID_WRITE_REG(&adapter->hw, LLITHRESH, 0xfc000000);
			break;

		default:
			break;
		}

		XLNID_WRITE_REG(&adapter->hw, FTQF(0),
						(XLNID_FTQF_POOL_MASK_EN |
							(XLNID_FTQF_PRIORITY_MASK
							<< XLNID_FTQF_PRIORITY_SHIFT) |
							(XLNID_FTQF_5TUPLE_MASK_MASK
							<< XLNID_FTQF_5TUPLE_MASK_SHIFT)));

		XLNID_WRITE_REG(&adapter->hw, SYNQF, 0x80000100);
	}

	if (adapter->lli_size) {
		XLNID_WRITE_REG(&adapter->hw, L34T_IMIR(0),
						(XLNID_IMIR_LLI_EN | XLNID_IMIR_CTRL_BP));
		XLNID_WRITE_REG(&adapter->hw, LLITHRESH, adapter->lli_size);
		XLNID_WRITE_REG(&adapter->hw, FTQF(0),
						(XLNID_FTQF_POOL_MASK_EN |
							(XLNID_FTQF_PRIORITY_MASK
							<< XLNID_FTQF_PRIORITY_SHIFT) |
							(XLNID_FTQF_5TUPLE_MASK_MASK
							<< XLNID_FTQF_5TUPLE_MASK_SHIFT)));
	}

	if (adapter->lli_vlan_pri) {
		XLNID_WRITE_REG(&adapter->hw, IMIRVP,
						(XLNID_IMIRVP_PRIORITY_EN |
							adapter->lli_vlan_pri));
	}
}

static void xlnid_configure_lli(struct xlnid_adapter *adapter)
{
	/* lli should only be enabled with MSI-X and MSI */
	if (!(adapter->flags & XLNID_FLAG_MSI_ENABLED) &&
			!(adapter->flags & XLNID_FLAG_MSIX_ENABLED)) {
		return;
	}

	/* LLI not supported on SKYLAKE */
	if (adapter->hw.mac.type == xlnid_mac_WESTLAKE) {
		xlnid_configure_lli_westlake(adapter);
		return;
	}
}

#endif /* XLNID_NO_LLI */

static void xlnid_fdir_filter_restore(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	struct hlist_node *node2;
	struct xlnid_fdir_filter *filter;
	u8 queue = 0;

	spin_lock(&adapter->fdir_perfect_lock);

	if (!hlist_empty(&adapter->fdir_filter_list))
		xlnid_fdir_set_input_mask(hw, &adapter->fdir_mask,
									adapter->cloud_mode);

	hlist_for_each_entry_safe(filter, node2, &adapter->fdir_filter_list,
								fdir_node) {
		if (filter->action == XLNID_FDIR_DROP_QUEUE) {
			queue = XLNID_FDIR_DROP_QUEUE;
		}

		else {
			u32 ring = ethtool_get_flow_spec_ring(filter->action);

			if (ring >= adapter->num_rx_queues) {
				e_err(drv, "FDIR restore failed, ring:%u\n",
						ring);
				continue;
			}

			/* Map the ring onto the absolute queue index */

			queue = adapter->rx_ring[ring]->reg_idx;
		}

		xlnid_fdir_write_perfect_filter(hw, &filter->filter,
										filter->sw_idx, queue,
										adapter->cloud_mode);
	}

	spin_unlock(&adapter->fdir_perfect_lock);
}

/**
		xlnid_clean_rx_ring - Free Rx Buffers per Queue
		@rx_ring: ring to free buffers from
	**/
static void xlnid_clean_rx_ring(struct xlnid_ring *rx_ring)
{
	u16 i = rx_ring->next_to_clean;
	struct xlnid_rx_buffer *rx_buffer = &rx_ring->rx_buffer_info[i];

#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
	DEFINE_DMA_ATTRS(attrs);

	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif

#ifdef HAVE_AF_XDP_ZC_SUPPORT

	if (rx_ring->xsk_pool) {
		xlnid_xsk_clean_rx_ring(rx_ring);
		goto skip_free;
	}

#endif
	/* Free all the Rx ring sk_buffs */
#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT

	while (i != rx_ring->next_to_use) {
#else

	while (i != rx_ring->next_to_alloc) {
#endif

		if (rx_buffer->skb) {
			struct sk_buff *skb = rx_buffer->skb;
#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT

			if (XLNID_CB(skb)->page_released)
				dma_unmap_page_attrs(rx_ring->dev,
										XLNID_CB(skb)->dma,
										xlnid_rx_pg_size(rx_ring),
										DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
										&attrs);

#else
										XLNID_RX_DMA_ATTR);
#endif
#else
			/* We need to clean up RSC frag lists */
			skb = xlnid_merge_active_tail(skb);

			if (xlnid_close_active_frag_list(skb))
				dma_unmap_single(rx_ring->dev,
									XLNID_CB(skb)->dma,
									rx_ring->rx_buf_len,
									DMA_FROM_DEVICE);

			XLNID_CB(skb)->dma = 0;
#endif /* CONFIG_XLNID_DISABLE_PACKET_SPLIT */
			dev_kfree_skb(skb);
			rx_buffer->skb = NULL;
		}

#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT
		/*  Invalidate cache lines that may have been written to by
			device so that we avoid corrupting memory.
		*/
		dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma,
										rx_buffer->page_offset,
										xlnid_rx_bufsz(rx_ring),
										DMA_FROM_DEVICE);

		/* free resources associated with mapping */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
								xlnid_rx_pg_size(rx_ring), DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
								&attrs);
#else
								XLNID_RX_DMA_ATTR);
#endif

		__page_frag_cache_drain(rx_buffer->page,
								rx_buffer->pagecnt_bias);
#else /* CONFIG_XLNID_DISABLE_PACKET_SPLIT */

		if (rx_buffer->dma) {
			dma_unmap_single(rx_ring->dev, rx_buffer->dma,
								rx_ring->rx_buf_len, DMA_FROM_DEVICE);
			rx_buffer->dma = 0;
		}

#endif /* CONFIG_XLNID_DISABLE_PACKET_SPLIT */
		i++;
		rx_buffer++;

		if (i == rx_ring->count) {
			i = 0;
			rx_buffer = rx_ring->rx_buffer_info;
		}
	}

#ifdef HAVE_AF_XDP_ZC_SUPPORT
skip_free:
#endif
#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	rx_ring->next_to_alloc = 0;
	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
#endif
}

static void xlnid_reset_config(struct xlnid_adapter *adapter)
{
	int i = 0;
	u32 value1 = 0;

	//u32 value2 = 0;
	//u32 value3 = 0;

	/* resolve insmod xlnid again problem, BUG SKYLAKE-218 */
	for (i = 0; i < 32; i++) {
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_RDT(i), 0);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_RDH(i), 0);
	}

#if 1
	value1 = XLNID_READ_REG_MAC(&adapter->hw, SKYLAKE_ECO_RESET);
#if 0
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_ECO_RESET, (value1 | 0x1));
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_ECO_RESET, value1 & 0xfffffffe);
#endif
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_ECO_RESET,
						(value1 & 0xfffffffe));
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_ECO_RESET, value1 | 0x1);
#endif

	/* reset chip, BUG SKYLAKE-223 */
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SYS_RST_N, 0x1);

	/* gmac init */
#ifdef XLNID_GE_SUPPORT
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_GMAC_LINK_RST_N, 0x7777);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SGMII_LINK_RST_N, 0xFF);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_TOP_MPI_LINK_MODE, 0x2);

	/* powerdown xgmac */
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_PLL_XGMAC_AUX_CFG, 0x255);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_XGMAC_LINK_RST_N, 0x00);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PHY_PI_COFNIG, 0xC);

	for (i = 0; i < 2; i++) {
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SDS_LANE_SEL,
							0x10 + i);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SDS_IRST_POR_N,
							0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_SDS_IPD_MULTI_SYNTH_N, 0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_SDS_IRST_MULTI_HARD_SYNTH_N,
							0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_LANE_IRST_PIPE_RST_N, 0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_LANE_IRST_MULTI_HARD_TXRX_N,
							0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_LANE_IPD_MULTI_TX_N, 0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_LANE_IPD_MULTI_RX_N, 0x0);
	}

#endif /* XEL_GE_SUPPORT */

	/* gmac phy init */
#ifdef XLNID_GE_PHY_SUPPORT
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_GMAC_LINK_RST_N, 0x7777);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_TOP_MPI_LINK_MODE, 0x0);

	/* powerdown xgmac */
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_PLL_XGMAC_AUX_CFG, 0x255);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_XGMAC_LINK_RST_N, 0x00);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SGMII_LINK_RST_N, 0x00);

	/* powerdown xgmac */
	for (i = 0; i < 2; i++) {
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SDS_LANE_SEL,
							0x10 + i);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SDS_IRST_POR_N,
							0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_SDS_IPD_MULTI_SYNTH_N, 0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_SDS_IRST_MULTI_HARD_SYNTH_N,
							0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_LANE_IRST_PIPE_RST_N, 0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_LANE_IRST_MULTI_HARD_TXRX_N,
							0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_LANE_IPD_MULTI_TX_N, 0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_LANE_IPD_MULTI_RX_N, 0x0);
	}

	/* powerdown gmac */
	for (i = 0; i < 4; i++) {
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SDS_LANE_SEL,
							0x00 + i);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SDS_IRST_POR_N,
							0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_SDS_IPD_MULTI_SYNTH_N, 0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_SDS_IRST_MULTI_HARD_SYNTH_N,
							0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_LANE_IRST_PIPE_RST_N, 0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_LANE_IRST_MULTI_HARD_TXRX_N,
							0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_LANE_IPD_MULTI_TX_N, 0x0);
		XLNID_WRITE_REG_MAC(&adapter->hw,
							SKYLAKE_CRG_LANE_IPD_MULTI_RX_N, 0x0);
	}

#endif /* XEL_GE_PHY_SUPPORT */

	/* xgmac init */
#ifdef XLNID_XGE_SUPPORT
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_PLL_XGMAC_AUX_CFG, 0x655);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_XGMAC_LINK_RST_N, 0xFF);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_GMAC_LINK_RST_N, 0x00);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SGMII_LINK_RST_N, 0x00);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PHY_PI_COFNIG, 0xC);
#endif /* XEL_XGE_SUPPORT */

	/* tx abnormal packet, eg : mss = 0 */
#if 0
#if 1
	XLNID_WRITE_REG(&adapter->hw, CRG_SYS_RST_N, 0xffffffff);
	XLNID_WRITE_REG(&adapter->hw, CRG_GMAC_LINK_RST_N, 0xffffffff);
	XLNID_WRITE_REG(&adapter->hw, CRG_XGMAC_LINK_RST_N, 0xffffffff);
	XLNID_WRITE_REG(&adapter->hw, CRG_SGMII_LINK_RST_N, 0xffffffff);
#endif
	udelay(1);
#if 1
	XLNID_WRITE_REG(&adapter->hw, CRG_SYS_RST_N, 0);
	XLNID_WRITE_REG(&adapter->hw, CRG_GMAC_LINK_RST_N, 0);
	XLNID_WRITE_REG(&adapter->hw, CRG_XGMAC_LINK_RST_N, 0);
	XLNID_WRITE_REG(&adapter->hw, CRG_SGMII_LINK_RST_N, 0);
#endif

	value1 = XLNID_READ_REG(&adapter->hw, ECO_RESET);

	if (!(value1 & 0x2)) {
		value2 = XLNID_READ_REG(&adapter->hw, UL_RX_PKT_CNT);

		if (value2) {
			XLNID_WRITE_REG(&adapter->hw, ECO_RESET, (value1 | 0x2));

			value3 = XLNID_READ_REG(&adapter->hw, RXBUF_MAP_SEL);
			XLNID_WRITE_REG(&adapter->hw, RXBUF_MAP_SEL, (value3 | 0x10));
			udelay(1);
			XLNID_WRITE_REG(&adapter->hw, RXBUF_MAP_SEL, (value3 & 0xFFFFFFEF));
		}
	}

	else {
		value3 = XLNID_READ_REG(&adapter->hw, RXBUF_MAP_SEL);
		XLNID_WRITE_REG(&adapter->hw, RXBUF_MAP_SEL, (value3 | 0x10));
		udelay(1);
		XLNID_WRITE_REG(&adapter->hw, RXBUF_MAP_SEL, (value3 & 0xFFFFFFEF));
	}

#endif
}

static void xlnid_pcie_ctrl_init(struct xlnid_adapter *adapter)
{
	int i = 0;

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_WDATA, 0x1);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
						0x8000097c);
	msleep(1);

	for (i = 0; i < 8; i++) {
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_WDATA,
							i);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a6c);

		msleep(1);

		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_WDATA,
							0);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x800009c4);
		msleep(1);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a74);
		msleep(1);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a78);
		msleep(1);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a7c);
		msleep(1);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a80);
		msleep(1);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a84);
		msleep(1);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a88);
		msleep(1);

		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_WDATA,
							0x8);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a70);
		msleep(1);
	}

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_WDATA, 0x1);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
						0x8000099c);
	msleep(1);

	for (i = 0; i < 8; i++) {
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_WDATA,
							0x80000000 + i);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a6c);
		msleep(1);

		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_WDATA,
							0);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a18);
		msleep(1);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a74);
		msleep(1);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a78);
		msleep(1);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a7c);
		msleep(1);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a80);
		msleep(1);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a84);
		msleep(1);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a88);
		msleep(1);

		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_WDATA,
							0x8);
		XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PCIE_DWC_PLREG_CMD,
							0x80000a70);
		msleep(1);
	}
}

static void xlnid_global_init(struct xlnid_adapter *adapter)
{
	/* xel tx buffer underflow */
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_TXBUFFER_MTI,
						XLNID_TXBUFFER_VALUE);

	/* protect switch */
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_PTSW_MODE, 0x0);
}

#if defined(XLNID_GE_SUPPORT) || defined(XLNID_XGE_SUPPORT)
static void xlnid_serdes_write(struct xlnid_adapter *adapter,
								unsigned int addr,
								unsigned int data)
{
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SDS_MEM_ADDR, addr);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SDS_MEM_DATA, data);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SDS_MEM_REQ, 0x1);
}
#endif

#ifdef XLNID_GE_SUPPORT
static void xlnid_gmac_init(struct xlnid_adapter *adapter)
{
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x01f00f0c);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000000);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x80000091);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000004);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x0);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000018);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0xffff);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000040);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0xffffffff);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000044);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x6);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000100);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x1000);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x800000c0);

	/* max frame size setting */
	XLNID_WRITE_REG(&adapter->hw, MAXFRS, XLNID_MAXFRS_DEFAULT);

	/* gmac serdes init */
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SDS_LANE_SEL, 0x00);
	xlnid_serdes_write(adapter, 0xE015, 0x00C1);
	xlnid_serdes_write(adapter, 0xE016, 0x0003);
	xlnid_serdes_write(adapter, 0xE017, 0x0000);
	xlnid_serdes_write(adapter, 0xE018, 0x003A);
	xlnid_serdes_write(adapter, 0xE019, 0x0000);
	xlnid_serdes_write(adapter, 0xE01A, 0x0008);
	xlnid_serdes_write(adapter, 0xE01B, 0x0078);
	xlnid_serdes_write(adapter, 0xE01C, 0x0010);
	xlnid_serdes_write(adapter, 0xE056, 0x00F7);
	xlnid_serdes_write(adapter, 0xE057, 0x001F);
}
#endif

#ifdef XLNID_GE_PHY_SUPPORT
static void xlnid_gmac_phy_init(struct xlnid_adapter *adapter)
{
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x01f00f0c);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000000);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x80000091);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000004);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x0);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000018);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0xffff);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000040);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0xffffffff);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000044);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x6);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000100);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x1000);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x800000c0);

	/* max frame size setting */
	XLNID_WRITE_REG(&adapter->hw, MAXFRS, XLNID_MAXFRS_DEFAULT);
}
#endif

#ifdef XLNID_XGE_SUPPORT
static void xlnid_xgmac_init(struct xlnid_adapter *adapter)
{
	/* xgmac ip init */
	/* lane 0 */
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x00D0040C);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000000);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x80000091);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000004);

	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_WDATA_I, 0x00000004);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_MCI_CMD_DATA, 0x80000800);

	/* lane 1 */
	XLNID_WRITE_REG_MAC(&adapter->hw,
						SKYLAKE_MCI_WDATA_I + XLNID_MAC_LAN_UNIT * 1,
						0x00D0040C);
	XLNID_WRITE_REG_MAC(&adapter->hw,
						SKYLAKE_MCI_CMD_DATA + XLNID_MAC_LAN_UNIT * 1,
						0x80000000);

	XLNID_WRITE_REG_MAC(&adapter->hw,
						SKYLAKE_MCI_WDATA_I + XLNID_MAC_LAN_UNIT * 1,
						0x80000091);
	XLNID_WRITE_REG_MAC(&adapter->hw,
						SKYLAKE_MCI_CMD_DATA + XLNID_MAC_LAN_UNIT * 1,
						0x80000004);

	XLNID_WRITE_REG_MAC(&adapter->hw,
						SKYLAKE_MCI_WDATA_I + XLNID_MAC_LAN_UNIT * 1,
						0x00000004);
	XLNID_WRITE_REG_MAC(&adapter->hw,
						SKYLAKE_MCI_CMD_DATA + XLNID_MAC_LAN_UNIT * 1,
						0x80000800);

	/* max frame size setting */
	XLNID_WRITE_REG(&adapter->hw, MAXFRS, XLNID_MAXFRS_DEFAULT);

	/* xgmac serdes init */
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_SDS_LANE_SEL, 0x10);
	xlnid_serdes_write(adapter, 0xE015, 0x00C1);
	xlnid_serdes_write(adapter, 0xE016, 0x0003);
	xlnid_serdes_write(adapter, 0xE017, 0x0000);
	xlnid_serdes_write(adapter, 0xE018, 0x003A);
	xlnid_serdes_write(adapter, 0xE019, 0x0000);
	xlnid_serdes_write(adapter, 0xE01A, 0x0008);
	xlnid_serdes_write(adapter, 0xE01B, 0x0078);
	xlnid_serdes_write(adapter, 0xE01C, 0x0010);
	xlnid_serdes_write(adapter, 0xE056, 0x00F7);
	xlnid_serdes_write(adapter, 0xE057, 0x001F);

	/* fix BUG SKYLAKE-274 */
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_XGMAC_LINK_RST_N, 0x77);
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_CRG_XGMAC_LINK_RST_N, 0xFF);
}
#endif

static void xlnid_pcs_init(struct xlnid_adapter *adapter)
{
	XLNID_WRITE_REG_MAC(&adapter->hw, SKYLAKE_SGM_MAC_COFNIG, 0x13);
	//XLNID_WRITE_REG(&adapter->hw, SGM_AN_COFNIG, 0x02);
}

static void xlnid_hw_init(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;

	if (hw->mac.type == xlnid_mac_SKYLAKE) {
		xlnid_reset_config(adapter);
		xlnid_global_init(adapter);
		xlnid_pcie_ctrl_init(adapter);
#ifdef XLNID_GE_SUPPORT
		xlnid_gmac_init(adapter);
#endif /* XLNID_GE_SUPPORT */

#ifdef XLNID_GE_PHY_SUPPORT
		xlnid_gmac_phy_init(adapter);
#endif /* XLNID_GE_PHY_SUPPORT */

#ifdef XLNID_XGE_SUPPORT
		xlnid_xgmac_init(adapter);
#endif /* XLNID_XGE_SUPPORT */
		xlnid_pcs_init(adapter);
	}

	else if (hw->mac.type == xlnid_mac_WESTLAKE) {
#ifdef XLNID_GEPHY_SUPPORT
		hw->phy.media_type = xlnid_media_type_copper;
#endif

#ifdef XLNID_1000BASEX_SUPPORT
		hw->phy.media_type = xlnid_media_type_fiber;
#endif

#ifdef XLNID_FORCE_1000BASEX_SUPPORT
		hw->phy.media_type = xlnid_media_type_fiber;
#endif

		/* set the link_reg_value of unit 1 when ptsw is enabled */
		if (hw->ptsw_enable && adapter->pdev->devfn == 0) {
			xel_pci_info_t *slave_pinfo =
				xlnid_find_pci_info(hw->bus.bus_id, 1);

			XLNID_WRITE_REG_MAC(slave_pinfo->hw,
								WESTLAKE_CFG_AN_EDIT_EN, 0x1);
			XLNID_WRITE_REG_MAC(slave_pinfo->hw,
								WESTLAKE_SGM_PCS_CFG0, 0xE4);

			XLNID_WRITE_REG_MAC(slave_pinfo->hw,
								WESTLAKE_SDS_TXDEEMPH, 0x4500);
			XLNID_WRITE_REG_MAC(slave_pinfo->hw,
								WESTLAKE_SDS_RX_EQ0, 0x38B7);
			XLNID_WRITE_REG_MAC(slave_pinfo->hw,
								WESTLAKE_SDS_RX_EQ1, 0x0100);
			XLNID_WRITE_REG_MAC(slave_pinfo->hw,
								WESTLAKE_SDS_RX_EQ2, 0x0021);
			XLNID_WRITE_REG_MAC(slave_pinfo->hw, WESTLAKE_RXCTRL,
								XLNID_RXCTRL_RXEN);
		}
	}
}

static void xlnid_configure(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;

	//xlnid_configure_pb(adapter);
	//xlnid_configure_dcb(adapter);

	/*
		We must restore virtualization before VLANs or else
		the VLVF registers will not be populated
	*/
	//xlnid_configure_virtualization(adapter);

	xlnid_set_rx_mode(adapter->netdev);
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	xlnid_restore_vlan(adapter);
#endif

	if (adapter->flags & XLNID_FLAG_FDIR_HASH_CAPABLE) {
		xlnid_init_fdir_signature(&adapter->hw, adapter->fdir_pballoc);
	}

	else if (adapter->flags & XLNID_FLAG_FDIR_PERFECT_CAPABLE) {
		xlnid_init_fdir_perfect(&adapter->hw, adapter->fdir_pballoc,
								adapter->cloud_mode);
		xlnid_fdir_filter_restore(adapter);
	}

	/* Enable EEE only when supported and enabled */
	if (hw->mac.ops.setup_eee &&
			(adapter->flags2 & XLNID_FLAG2_EEE_CAPABLE)) {
		bool eee_enable = !!(adapter->flags2 & XLNID_FLAG2_EEE_ENABLED);

		hw->mac.ops.setup_eee(hw, eee_enable);
	}

	xlnid_configure_tx(adapter);
	xlnid_configure_rx(adapter);
}

/**
	xlnid_non_sfp_link_config - set up non-SFP+ link
	@hw: pointer to private hardware struct

	Returns 0 on success, negative on failure
**/
static int xlnid_non_sfp_link_config(struct xlnid_hw *hw)
{
	u32 speed;
	bool autoneg, link_up = false;
	u32 ret = XLNID_ERR_LINK_SETUP;

	if (hw->mac.ops.check_link) {
		ret = hw->mac.ops.check_link(hw, &speed, &link_up, false);
	}

	if (ret) {
		goto link_cfg_out;
	}

	speed = hw->phy.autoneg_advertised;

	if (!speed && hw->mac.ops.get_link_capabilities) {
		ret = hw->mac.ops.get_link_capabilities(hw, &speed, &autoneg);
	}

	if (ret) {
		goto link_cfg_out;
	}

	if (hw->mac.ops.setup_link) {
		ret = hw->mac.ops.setup_link(hw, speed, link_up);
	}

link_cfg_out:
	return ret;
}

/**
	xlnid_clear_vf_stats_counters - Clear out VF stats after reset
	@adapter: board private structure

	On a reset we need to clear out the VF stats or accounting gets
	messed up because they're not clear on read.
**/
static void xlnid_clear_vf_stats_counters(struct xlnid_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_vfs; i++) {
		adapter->vfinfo[i].last_vfstats.gprc = 0;
		adapter->vfinfo[i].saved_rst_vfstats.gprc +=
			adapter->vfinfo[i].vfstats.gprc;
		adapter->vfinfo[i].vfstats.gprc = 0;
		adapter->vfinfo[i].last_vfstats.gptc = 0;
		adapter->vfinfo[i].saved_rst_vfstats.gptc +=
			adapter->vfinfo[i].vfstats.gptc;
		adapter->vfinfo[i].vfstats.gptc = 0;
		adapter->vfinfo[i].last_vfstats.gorc = 0;
		adapter->vfinfo[i].saved_rst_vfstats.gorc +=
			adapter->vfinfo[i].vfstats.gorc;
		adapter->vfinfo[i].vfstats.gorc = 0;
		adapter->vfinfo[i].last_vfstats.gotc = 0;
		adapter->vfinfo[i].saved_rst_vfstats.gotc +=
			adapter->vfinfo[i].vfstats.gotc;
		adapter->vfinfo[i].vfstats.gotc = 0;
		adapter->vfinfo[i].last_vfstats.mprc = 0;
		adapter->vfinfo[i].saved_rst_vfstats.mprc +=
			adapter->vfinfo[i].vfstats.mprc;
		adapter->vfinfo[i].vfstats.mprc = 0;
	}
}

static void xlnid_setup_gpie(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 gpie = 0;

	if (adapter->flags & XLNID_FLAG_MSIX_ENABLED) {
		gpie = XLNID_GPIE_MSIX_MODE | XLNID_GPIE_PBA_SUPPORT |
				XLNID_GPIE_OCD;
		gpie |= XLNID_GPIE_EIAME;
		/*
			use EIAM to auto-mask when MSI-X interrupt is asserted
			this saves a register write for every interrupt
		*/
		//      XLNID_WRITE_REG_DIRECT(hw, EIAM_EX(0), 0xFFFFFFFF);
		//      XLNID_WRITE_REG_DIRECT(hw, EIAM_EX(1), 0xFFFFFFFF);
		XLNID_WRITE_REG_DIRECT(hw, EIAM_EX(0), 0x0);
		XLNID_WRITE_REG_DIRECT(hw, EIAM_EX(1), 0x0);
	}

	else {
		/*  legacy interrupts, use EIAM to auto-mask when reading EICR,
			specifically only auto mask tx and rx interrupts */
		XLNID_WRITE_REG_DIRECT(hw, EIAM, XLNID_EICS_RTX_QUEUE);
	}

	if (adapter->flags & XLNID_FLAG_SRIOV_ENABLED) {
		gpie &= ~XLNID_GPIE_VTMODE_MASK;

		switch (adapter->ring_feature[RING_F_VMDQ].mask) {
		case XLNID_VMDQ_8Q_MASK:
			gpie |= XLNID_GPIE_VTMODE_16;
			break;

		case XLNID_VMDQ_4Q_MASK:
			gpie |= XLNID_GPIE_VTMODE_32;
			break;

		default:
			gpie |= XLNID_GPIE_VTMODE_64;
			break;
		}
	}

	XLNID_WRITE_REG_DIRECT(hw, GPIE, gpie);
}

static void xlnid_up_complete(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	int err;

	xlnid_setup_gpie(adapter);

	if (adapter->flags & XLNID_FLAG_MSIX_ENABLED) {
		xlnid_configure_msix(adapter);
	}

	else {
		xlnid_configure_msi_and_legacy(adapter);
	}

	/* enable the optics for SFP+ fiber */
	if (hw->mac.ops.enable_tx_laser) {
		hw->mac.ops.enable_tx_laser(hw);
	}

	xlnid_set_phy_power(hw, true);

	if (hw->mac.type == xlnid_mac_WESTLAKE && hw->ptsw_enable &&
			adapter->pdev->devfn == 0) {
		xel_pci_info_t *slave_pinfo =
			xlnid_find_pci_info(hw->bus.bus_id, 1);
		u32 regval, crg_config;

		/* 1. disable port1 tx & rx */
		regval = xlnid_read_mac_westlake(slave_pinfo->hw,
											WESTLAKE_GMAC_CONFIG);
		regval &= ~(0xC);
		xlnid_write_mac_westlake(slave_pinfo->hw, WESTLAKE_GMAC_CONFIG,
									regval);

		/* 2. reset port 1 ethport(only rx) */
		crg_config = XLNID_READ_REG_MAC(slave_pinfo->hw,
										WESTLAKE_CRG_ETH_PORT_RST_N);
		crg_config &= ~(0x20);
		XLNID_WRITE_REG_MAC(slave_pinfo->hw,
							WESTLAKE_CRG_ETH_PORT_RST_N, crg_config);

		/* 3. enable port1 macsec loopback */
		XLNID_WRITE_REG_MAC(slave_pinfo->hw,
							WESTLAKE_MACSEC_ETH_LOOPBACK_CFG, 1);

		/* 4. ptsw enable port0 */
		regval = XLNID_READ_REG_MAC(slave_pinfo->hw,
									WESTLAKE_CFG_PTSW_MODE);
		regval &= ~XLNID_CRG_RX_PTSW_PORT;
		XLNID_WRITE_REG_MAC(slave_pinfo->hw, WESTLAKE_CFG_PTSW_MODE,
							regval);

		xlnid_set_phy_power(slave_pinfo->hw, true);
	}

	smp_mb__before_atomic();
	clear_bit(__XLNID_DOWN, &adapter->state);
	xlnid_napi_enable_all(adapter);
#ifndef XLNID_NO_LLI
	xlnid_configure_lli(adapter);
#endif

	err = xlnid_non_sfp_link_config(hw);

	if (err) {
		e_err(probe, "link_config FAILED %d\n", err);
	}

	/* clear any pending interrupts, may auto mask */
	XLNID_READ_REG_DIRECT(hw, EICR);
	xlnid_irq_enable(adapter, true, true);

	/* enable transmits */
	netif_tx_start_all_queues(adapter->netdev);

	/*  bring the link up in the watchdog, this could race with our first
		link up interrupt but shouldn't be a problem */
	adapter->flags |= XLNID_FLAG_NEED_LINK_UPDATE;
	adapter->link_check_timeout = jiffies;
	mod_timer(&adapter->service_timer, jiffies);

	xlnid_clear_vf_stats_counters(adapter);
}

#if 1
void xlnid_reinit_locked(struct xlnid_adapter *adapter)
{
	WARN_ON(in_interrupt());
	/* put off any impending NetWatchDogTimeout */
#ifdef HAVE_NETIF_TRANS_UPDATE
	netif_trans_update(adapter->netdev);
#else
	adapter->netdev->trans_start = jiffies;
#endif

	while (test_and_set_bit(__XLNID_RESETTING, &adapter->state)) {
		usleep_range(1000, 2000);
	}

	if (adapter->hw.phy.type == xlnid_phy_fw) {
		xlnid_watchdog_link_is_down(adapter);
	}

	xlnid_down(adapter);

	/*
		If SR-IOV enabled then wait a bit before bringing the adapter
		back up to give the VFs time to respond to the reset.  The
		two second wait is based upon the watchdog timer cycle in
		the VF driver.
	*/
	if (adapter->flags & XLNID_FLAG_SRIOV_ENABLED) {
		msleep(2000);
	}

	xlnid_up(adapter);
	clear_bit(__XLNID_RESETTING, &adapter->state);
}

#endif

void xlnid_up(struct xlnid_adapter *adapter)
{
	/* xel init */
	xlnid_hw_init(adapter);

	/* hardware has been reset, we need to reload some things */
	xlnid_configure(adapter);

	xlnid_up_complete(adapter);
}

static unsigned long xlnid_get_completion_timeout(
					struct xlnid_adapter *adapter)
{
	u16 devctl2;

	pcie_capability_read_word(adapter->pdev, PCI_EXP_DEVCTL2, &devctl2);

	switch (devctl2 & XLNID_PCIDEVCTRL2_TIMEO_MASK) {
	case XLNID_PCIDEVCTRL2_17_34s:
	case XLNID_PCIDEVCTRL2_4_8s:

	/*  For now we cap the upper limit on delay to 2 seconds
	as we end up going up to 34 seconds of delay in worst
	case timeout value.
	*/
	case XLNID_PCIDEVCTRL2_1_2s:
		return 2000000ul; /* 2.0 s */

	case XLNID_PCIDEVCTRL2_260_520ms:
		return 520000ul; /* 520 ms */

	case XLNID_PCIDEVCTRL2_65_130ms:
		return 130000ul; /* 130 ms */

	case XLNID_PCIDEVCTRL2_16_32ms:
		return 32000ul; /* 32 ms */

	case XLNID_PCIDEVCTRL2_1_2ms:
		return 2000ul; /* 2 ms */

	case XLNID_PCIDEVCTRL2_50_100us:
		return 100ul; /* 100 us */

	case XLNID_PCIDEVCTRL2_16_32ms_def:
		return 32000ul; /* 32 ms */

	default:
		break;
	}

	/*  We shouldn't need to hit this path, but just in case default as
		though completion timeout is not supported and support 32ms.
	*/
	return 32000ul;
}

void xlnid_disable_rx_queue(struct xlnid_adapter *adapter)
{
	unsigned long wait_delay, delay_interval;
	struct xlnid_hw *hw = &adapter->hw;
	int i, wait_loop;
	u32 rxdctl, fifo_status, pbuf_stat;

	/* disable receives */
	hw->mac.ops.disable_rx(hw);

	if (XLNID_REMOVED(hw->hw_addr)) {
		return;
	}

	for (i = 0; i < 100; i++) {
		fifo_status =
			XLNID_READ_REG_MAC_DIRECT(hw, WESTLAKE_DMA_FIFO_STATUS);
		pbuf_stat = XLNID_READ_REG_MAC(hw, WESTLAKE_RX_PBUF_STAT);

		if (((fifo_status & 0x0c070000) == 0x0c070000) &&
				((pbuf_stat & 0x11) == 0x11)) {
			break;
		}

		msleep(1);
	}

	if (i >= 100) {
		e_err(drv,
				"xlnid_disable_rx_queue: dma fifo_status = 0x%08x, pbuf_stat = 0x%08x\r\n",
				fifo_status, pbuf_stat);
	}

	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct xlnid_ring *ring = adapter->rx_ring[i];
		u8 reg_idx = ring->reg_idx;

		rxdctl = XLNID_READ_REG_DIRECT(hw, RXDCTL(reg_idx));
		rxdctl &= ~XLNID_RXDCTL_ENABLE;
		rxdctl |= XLNID_RXDCTL_SWFLSH;

		/* write value back with RXDCTL.ENABLE bit cleared */
		XLNID_WRITE_REG_DIRECT(hw, RXDCTL(reg_idx), rxdctl);
		xlnid_rx_desc_queue_disable(adapter, ring);
	}

	/*  Determine our minimum delay interval. We will increase this value
		with each subsequent test. This way if the device returns quickly
		we should spend as little time as possible waiting, however as
		the time increases we will wait for larger periods of time.

		The trick here is that we increase the interval using the
		following pattern: 1x 3x 5x 7x 9x 11x 13x 15x 17x 19x. The result
		of that wait is that it totals up to 100x whatever interval we
		choose. Since our minimum wait is 100us we can just divide the
		total timeout by 100 to get our minimum delay interval.
	*/
	delay_interval = xlnid_get_completion_timeout(adapter) / 100;

	wait_loop = XLNID_MAX_RX_DESC_POLL;
	wait_delay = delay_interval;

	while (wait_loop--) {
		usleep_range(wait_delay, wait_delay + 10);
		wait_delay += delay_interval * 2;
		rxdctl = 0;

		/*  OR together the reading of all the active RXDCTL registers,
			and then test the result. We need the disable to complete
			before we start freeing the memory and invalidating the
			DMA mappings.
		*/
		for (i = 0; i < adapter->num_rx_queues; i++) {
			struct xlnid_ring *ring = adapter->rx_ring[i];
			u8 reg_idx = ring->reg_idx;

			rxdctl |= XLNID_READ_REG_DIRECT(hw, RXDCTL(reg_idx));
		}

		if (!(rxdctl & XLNID_RXDCTL_ENABLE)) {
			return;
		}
	}

	e_err(drv,
			"RXDCTL.ENABLE for one or more queues not cleared within the polling period\n");
}

void xlnid_disable_tx_queue(struct xlnid_adapter *adapter)
{
	unsigned long wait_delay, delay_interval;
	struct xlnid_hw *hw = &adapter->hw;
	int i, wait_loop;
	u32 txdctl;

	if (XLNID_REMOVED(hw->hw_addr)) {
		return;
	}

	/* disable all enabled Tx queues */
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct xlnid_ring *ring = adapter->tx_ring[i];
		u8 reg_idx = ring->reg_idx;

		XLNID_WRITE_REG_DIRECT(hw, TXDCTL(reg_idx),
								XLNID_TXDCTL_SWFLSH);
		xlnid_tx_desc_queue_disable(adapter, ring);
	}

	/* disable all enabled XDP Tx queues */
	for (i = 0; i < adapter->num_xdp_queues; i++) {
		struct xlnid_ring *ring = adapter->xdp_ring[i];
		u8 reg_idx = ring->reg_idx;

		XLNID_WRITE_REG_DIRECT(hw, TXDCTL(reg_idx),
								XLNID_TXDCTL_SWFLSH);
		xlnid_tx_desc_queue_disable(adapter, ring);
	}

	/*  If the link is not up there shouldn't be much in the way of
		pending transactions. Those that are left will be flushed out
		when the reset logic goes through the flush sequence to clean out
		the pending Tx transactions.
	*/
#if 0

	if (!(XLNID_READ_REG(hw, XLNID_LINKS) & XLNID_LINKS_UP)) {
		goto dma_engine_disable;
	}

#endif

	/*  Determine our minimum delay interval. We will increase this value
		with each subsequent test. This way if the device returns quickly
		we should spend as little time as possible waiting, however as
		the time increases we will wait for larger periods of time.

		The trick here is that we increase the interval using the
		following pattern: 1x 3x 5x 7x 9x 11x 13x 15x 17x 19x. The result
		of that wait is that it totals up to 100x whatever interval we
		choose. Since our minimum wait is 100us we can just divide the
		total timeout by 100 to get our minimum delay interval.
	*/
	delay_interval = xlnid_get_completion_timeout(adapter) / 100;

	wait_loop = XLNID_MAX_RX_DESC_POLL;
	wait_delay = delay_interval;

	while (wait_loop--) {
		usleep_range(wait_delay, wait_delay + 10);
		wait_delay += delay_interval * 2;
		txdctl = 0;

		/*  OR together the reading of all the active TXDCTL registers,
			and then test the result. We need the disable to complete
			before we start freeing the memory and invalidating the
			DMA mappings.
		*/
		for (i = 0; i < adapter->num_tx_queues; i++) {
			struct xlnid_ring *ring = adapter->tx_ring[i];
			u8 reg_idx = ring->reg_idx;

			txdctl |= XLNID_READ_REG_DIRECT(hw, TXDCTL(reg_idx));
		}

		for (i = 0; i < adapter->num_xdp_queues; i++) {
			struct xlnid_ring *ring = adapter->xdp_ring[i];
			u8 reg_idx = ring->reg_idx;

			txdctl |= XLNID_READ_REG_DIRECT(hw, TXDCTL(reg_idx));
		}

		if (!(txdctl & XLNID_TXDCTL_ENABLE)) {
			goto dma_engine_disable;
		}
	}

	e_err(drv,
			"TXDCTL.ENABLE for one or more queues not cleared within the polling period\n");

dma_engine_disable:
	/* Disable the Tx DMA engine */
	XLNID_WRITE_REG(hw, DMATXCTL,
					(XLNID_READ_REG(hw, DMATXCTL) & ~XLNID_DMATXCTL_TE));
}

void xlnid_reset(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;

#ifdef HAVE_SET_RX_MODE
	struct net_device *netdev = adapter->netdev;
#endif
	int err;

	if (XLNID_REMOVED(hw->hw_addr)) {
		return;
	}

	/* lock SFP init bit to prevent race conditions with the watchdog */
	while (test_and_set_bit(__XLNID_IN_SFP_INIT, &adapter->state)) {
		usleep_range(1000, 2000);
	}

	/* clear all SFP and link config related flags while holding SFP_INIT */
	adapter->flags2 &=
		~(XLNID_FLAG2_SEARCH_FOR_SFP | XLNID_FLAG2_SFP_NEEDS_RESET);
	adapter->flags &= ~XLNID_FLAG_NEED_LINK_CONFIG;

	err = hw->mac.ops.init_hw(hw);

	switch (err) {
	case XLNID_SUCCESS:
	case XLNID_ERR_SFP_NOT_PRESENT:
	case XLNID_ERR_SFP_NOT_SUPPORTED:
		break;

	case XLNID_ERR_PRIMARY_REQUESTS_PENDING:
		e_dev_err("primary disable timed out\n");
		break;

	case XLNID_ERR_EEPROM_VERSION:
		/* We are running on a pre-production device, log a warning */
		e_dev_warn("This device is a pre-production adapter/LOM. "
		"Please be aware there may be issues associated "
		"with your hardware.  If you are experiencing "
		"problems please contact your Xel or hardware "
		"representative who provided you with this "
		"hardware.\n");
		break;

	case XLNID_ERR_OVERTEMP:
		e_crit(drv, "%s\n", xlnid_overheat_msg);
		break;

	default:
		e_dev_err("Hardware Error: %d\n", err);
	}

	clear_bit(__XLNID_IN_SFP_INIT, &adapter->state);

	/* flush entries out of MAC table */
	xlnid_flush_sw_mac_table(adapter);
#ifdef HAVE_SET_RX_MODE
	__dev_uc_unsync(netdev, NULL);
#endif

	/* do not flush user set addresses */
	xlnid_mac_set_default_filter(adapter);

	/* update SAN MAC vmdq pool selection */
	if (hw->mac.san_mac_rar_index) {
		hw->mac.ops.set_vmdq_san_mac(hw, VMDQ_P(0));
	}

	/* Clear saved DMA coalescing values except for watchdog_timer */
	hw->mac.dmac_config.fcoe_en = false;
	hw->mac.dmac_config.link_speed = 0;
	hw->mac.dmac_config.fcoe_tc = 0;
	hw->mac.dmac_config.num_tcs = 0;

#ifdef HAVE_PTP_1588_CLOCK

	if (test_bit(__XLNID_PTP_RUNNING, &adapter->state)) {
		xlnid_ptp_reset(adapter);
	}

#endif

	if (!netif_running(adapter->netdev) && !adapter->wol) {
		xlnid_set_phy_power(hw, false);
	}

	else {
		xlnid_set_phy_power(hw, true);
	}
}

/**
	xlnid_clean_tx_ring - Free Tx Buffers
	@tx_ring: ring to be cleaned
**/
static void xlnid_clean_tx_ring(struct xlnid_ring *tx_ring)
{
	u16 i = tx_ring->next_to_clean;
	struct xlnid_tx_buffer *tx_buffer = &tx_ring->tx_buffer_info[i];

#ifdef HAVE_AF_XDP_ZC_SUPPORT

	if (tx_ring->xsk_pool) {
		xlnid_xsk_clean_tx_ring(tx_ring);
		goto out;
	}

#endif

	while (i != tx_ring->next_to_use) {
		union xlnid_adv_tx_desc *eop_desc, *tx_desc;

		/* Free all the Tx ring sk_buffs */
#ifdef HAVE_XDP_SUPPORT

		if (ring_is_xdp(tx_ring))
#ifdef HAVE_XDP_FRAME_STRUCT
			xdp_return_frame(tx_buffer->xdpf);

#else
			page_frag_free(tx_buffer->data);
#endif
		else {
			dev_kfree_skb_any(tx_buffer->skb);
		}

#else
		dev_kfree_skb_any(tx_buffer->skb);
#endif

		/* unmap skb header data */
		dma_unmap_single(tx_ring->dev, dma_unmap_addr(tx_buffer, dma),
							dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);

		/* check for eop_desc to determine the end of the packet */
		eop_desc = tx_buffer->next_to_watch;
		tx_desc = XLNID_TX_DESC(tx_ring, i);

		/* unmap remaining buffers */
		while (tx_desc != eop_desc) {
			tx_buffer++;
			tx_desc++;
			i++;

			if (unlikely(i == tx_ring->count)) {
				i = 0;
				tx_buffer = tx_ring->tx_buffer_info;
				tx_desc = XLNID_TX_DESC(tx_ring, 0);
			}

			/* unmap any remaining paged data */
			if (dma_unmap_len(tx_buffer, len))
				dma_unmap_page(tx_ring->dev,
								dma_unmap_addr(tx_buffer, dma),
								dma_unmap_len(tx_buffer, len),
								DMA_TO_DEVICE);
		}

		/* move us one more past the eop_desc for start of next pkt */
		tx_buffer++;
		i++;

		if (unlikely(i == tx_ring->count)) {
			i = 0;
			tx_buffer = tx_ring->tx_buffer_info;
		}
	}

	/* reset BQL for queue */
	if (!ring_is_xdp(tx_ring)) {
		netdev_tx_reset_queue(txring_txq(tx_ring));
	}

#ifdef HAVE_AF_XDP_ZC_SUPPORT
out:
#endif

	/* reset next_to_use and next_to_clean */
	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;
}

/**
		xlnid_clean_all_rx_rings - Free Rx Buffers for all queues
		@adapter: board private structure
	**/
static void xlnid_clean_all_rx_rings(struct xlnid_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		xlnid_clean_rx_ring(adapter->rx_ring[i]);
	}
}

/**
	xlnid_clean_all_tx_rings - Free Tx Buffers for all queues
	@adapter: board private structure
**/
static void xlnid_clean_all_tx_rings(struct xlnid_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		xlnid_clean_tx_ring(adapter->tx_ring[i]);
	}

	for (i = 0; i < adapter->num_xdp_queues; i++) {
		xlnid_clean_tx_ring(adapter->xdp_ring[i]);
	}
}

static void xlnid_fdir_filter_exit(struct xlnid_adapter *adapter)
{
	struct hlist_node *node2;
	struct xlnid_fdir_filter *filter;

	spin_lock(&adapter->fdir_perfect_lock);

	hlist_for_each_entry_safe(filter, node2, &adapter->fdir_filter_list,
								fdir_node) {
		hlist_del(&filter->fdir_node);
		kfree(filter);
	}
	adapter->fdir_filter_count = 0;

	spin_unlock(&adapter->fdir_perfect_lock);
}

void xlnid_down(struct xlnid_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct xlnid_hw *hw = &adapter->hw;

	/* signal that we are down to the interrupt handler */
	if (test_and_set_bit(__XLNID_DOWN, &adapter->state)) {
		return;    /* do nothing if already down */
	}

	/* Shut off incoming Tx traffic */
	netif_tx_stop_all_queues(netdev);

	/* call carrier off first to avoid false dev_watchdog timeouts */
	netif_carrier_off(netdev);
	netif_tx_disable(netdev);

	/* Disable Rx */
	xlnid_disable_rx_queue(adapter);

	/* synchronize_rcu() needed for pending XDP buffers to drain */
	if (adapter->xdp_ring[0]) {
		synchronize_rcu();
	}

	xlnid_irq_disable(adapter);

	xlnid_napi_disable_all(adapter);

	adapter->flags2 &= ~(XLNID_FLAG2_FDIR_REQUIRES_REINIT);
	clear_bit(__XLNID_RESET_REQUESTED, &adapter->state);
	adapter->flags &= ~XLNID_FLAG_NEED_LINK_UPDATE;

	del_timer_sync(&adapter->service_timer);

	/* disable transmits in the hardware now that interrupts are off */
	xlnid_disable_tx_queue(adapter);

#ifdef HAVE_PCI_ERS

	if (!pci_channel_offline(adapter->pdev))
#endif
		xlnid_reset(adapter);

	/* power down the optics for SFP+ fiber */
	if (hw->mac.ops.disable_tx_laser) {
		hw->mac.ops.disable_tx_laser(hw);
	}

	xlnid_clean_all_tx_rings(adapter);
	xlnid_clean_all_rx_rings(adapter);
}

/**
	xlnid_eee_capable - helper function to determine EEE support
	@adapter: board private structure to initialize
**/
static inline void xlnid_set_eee_capable(struct xlnid_adapter *adapter)
{
	adapter->flags2 &= ~XLNID_FLAG2_EEE_CAPABLE;
	adapter->flags2 &= ~XLNID_FLAG2_EEE_ENABLED;
}

/**
	xlnid_sw_init - Initialize general software structures (struct xlnid_adapter)
	@adapter: board private structure to initialize

	xlnid_sw_init initializes the Adapter private data structure.
	Fields are initialized based on PCI device information and
	OS network device settings (MTU size).
**/
#ifdef HAVE_CONFIG_HOTPLUG
static int __devinit xlnid_sw_init(struct xlnid_adapter *adapter)
#else
static int xlnid_sw_init(struct xlnid_adapter *adapter)
#endif
{
	struct xlnid_hw *hw = &adapter->hw;
	struct pci_dev *pdev = adapter->pdev;
	int err;
	unsigned int fdir;

	spin_lock_init(&hw->indirect_lock);

	/* PCI config space info */
	hw->vendor_id = pdev->vendor;
	hw->device_id = pdev->device;
	pci_read_config_byte(pdev, PCI_REVISION_ID, &hw->revision_id);

	if (hw->revision_id == XLNID_FAILED_READ_CFG_BYTE &&
			xlnid_check_cfg_remove(hw, pdev)) {
		e_err(probe, "read of revision id failed\n");
		err = -ENODEV;
		goto out;
	}

	hw->subsystem_vendor_id = pdev->subsystem_vendor;
	hw->subsystem_device_id = pdev->subsystem_device;
	hw->bus.bus_id = pdev->bus->number;
	hw->bus.func = pdev->devfn;

	hw->multifunction = pdev->multifunction;
	e_dev_info("multifunction = %d\n", hw->multifunction);

	err = xlnid_init_shared_code(hw);

	if (err) {
		e_err(probe, "init_shared_code failed: %d\n", err);
		goto out;
	}

	adapter->mac_table =
		kzalloc(sizeof(struct xlnid_mac_addr) * hw->mac.num_rar_entries,
				GFP_KERNEL);

	if (!adapter->mac_table) {
		err = XLNID_ERR_OUT_OF_MEM;
		e_err(probe, "mac_table allocation failed: %d\n", err);
		goto out;
	}

	if (xlnid_init_rss_key(adapter)) {
		err = XLNID_ERR_OUT_OF_MEM;
		e_err(probe, "rss_key allocation failed: %d\n", err);
		goto out;
	}

	adapter->af_xdp_zc_qps =
		bitmap_zalloc(WESTLAKE_MAX_XDP_QUEUES, GFP_KERNEL);

	if (!adapter->af_xdp_zc_qps) {
		return -ENOMEM;
	}

	/* Set common capability flags and settings */
	//adapter->flags2 |= XLNID_FLAG2_RSC_CAPABLE;
	if (hw->mac.type == xlnid_mac_WESTLAKE) {
		fdir = min_t(int, WESTLAKE_MAX_FDIR_INDICES, num_online_cpus());
	}

	else {
		fdir = min_t(int, XLNID_MAX_FDIR_INDICES, num_online_cpus());
	}

	adapter->ring_feature[RING_F_FDIR].limit = fdir;
	adapter->max_q_vectors = XLNID_MAX_MSIX_Q_VECTORS;

	/* Set MAC specific capability flags and exceptions */
	switch (hw->mac.type) {
	case xlnid_mac_SKYLAKE:
		adapter->flags |= XLNID_FLAGS_SKYLAKE_INIT;
		break;

	case xlnid_mac_WESTLAKE:
		adapter->flags |= XLNID_FLAGS_WESTLAKE_INIT;
		break;

	default:
		break;
	}

#ifndef XLNID_NO_SMART_SPEED
	hw->phy.smart_speed = xlnid_smart_speed_on;
#else
	hw->phy.smart_speed = xlnid_smart_speed_off;
#endif

	/* n-tuple support exists, always init our spinlock */
	spin_lock_init(&adapter->fdir_perfect_lock);

	/* default flow control settings */
	hw->fc.requested_mode = xlnid_fc_full;
	hw->fc.current_mode = xlnid_fc_full; /* init for ethtool output */

	adapter->last_lfc_mode = hw->fc.current_mode;
	//xlnid_pbthresh_setup(adapter);
	hw->fc.pause_time = XLNID_DEFAULT_FCPAUSE;
	hw->fc.send_xon = true;
	hw->fc.disable_fc_autoneg = false;

	/* set default ring sizes */
	adapter->tx_ring_count = XLNID_DEFAULT_TXD;
	adapter->rx_ring_count = XLNID_DEFAULT_RXD;

	/* set default work limits */
	adapter->tx_work_limit = XLNID_DEFAULT_TX_WORK;

	set_bit(__XLNID_DOWN, &adapter->state);
out:
	return err;
}

/**
	xlnid_setup_tx_resources - allocate Tx resources (Descriptors)
	@tx_ring:    tx descriptor ring (for a specific queue) to setup

	Return 0 on success, negative on failure
**/
int xlnid_setup_tx_resources(struct xlnid_ring *tx_ring)
{
	struct device *dev = tx_ring->dev;
	int orig_node = dev_to_node(dev);
	int node = -1;
	int size;

	size = sizeof(struct xlnid_tx_buffer) * tx_ring->count;

	if (tx_ring->q_vector) {
		node = tx_ring->q_vector->node;
	}

	tx_ring->tx_buffer_info = vmalloc_node(size, node);

	if (!tx_ring->tx_buffer_info) {
		tx_ring->tx_buffer_info = vmalloc(size);
	}

	if (!tx_ring->tx_buffer_info) {
		goto err;
	}

	/* round up to nearest 4K */
	tx_ring->size = tx_ring->count * sizeof(union xlnid_adv_tx_desc);
	tx_ring->size = ALIGN(tx_ring->size, 4096);

	set_dev_node(dev, node);
	tx_ring->desc = dma_alloc_coherent(dev, tx_ring->size, &tx_ring->dma,
										GFP_KERNEL);
	set_dev_node(dev, orig_node);

	if (!tx_ring->desc)
		tx_ring->desc = dma_alloc_coherent(dev, tx_ring->size,
											&tx_ring->dma, GFP_KERNEL);

	if (!tx_ring->desc) {
		goto err;
	}

	return 0;

err:
	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;
	dev_err(dev, "Unable to allocate memory for the Tx descriptor ring\n");
	return -ENOMEM;
}

/**
	xlnid_setup_all_tx_resources - allocate all queues Tx resources
	@adapter: board private structure

	If this function returns with an error, then it's possible one or
	more of the rings is populated (while the rest are not).  It is the
	callers duty to clean those orphaned rings.

	Return 0 on success, negative on failure
**/
static int xlnid_setup_all_tx_resources(struct xlnid_adapter *adapter)
{
	int i, j = 0, err = 0;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		err = xlnid_setup_tx_resources(adapter->tx_ring[i]);

		if (!err) {
			continue;
		}

		e_err(probe, "Allocation for Tx Queue %u failed\n", i);
		goto err_setup_tx;
	}

	for (j = 0; j < adapter->num_xdp_queues; j++) {
		err = xlnid_setup_tx_resources(adapter->xdp_ring[j]);

		if (!err) {
			continue;
		}

		e_err(probe, "Allocation for Tx Queue %u failed\n", j);
		goto err_setup_tx;
	}

	return 0;
err_setup_tx:

	/* rewind the index freeing the rings as we go */
	while (j--) {
		xlnid_free_tx_resources(adapter->xdp_ring[j]);
	}

	while (i--) {
		xlnid_free_tx_resources(adapter->tx_ring[i]);
	}

	return err;
}

#ifdef HAVE_XDP_BUFF_RXQ
static int xlnid_rx_napi_id(struct xlnid_ring *rx_ring)
{
	struct xlnid_q_vector *q_vector = rx_ring->q_vector;

	return q_vector ? q_vector->napi.napi_id : 0;
}
#endif

/**
	xlnid_setup_rx_resources - allocate Rx resources (Descriptors)
	@adapter: board private structure
	@rx_ring: rx descriptor ring (for a specific queue) to setup

	Returns 0 on success, negative on failure
**/
int xlnid_setup_rx_resources(struct xlnid_adapter *adapter,
								struct xlnid_ring *rx_ring)
{
	struct device *dev = rx_ring->dev;
	int orig_node = dev_to_node(dev);
	int node = -1;
	int size;
#ifdef HAVE_XDP_BUFF_RXQ
#ifdef HAVE_XDP_FRAME_STRUCT
#ifndef HAVE_AF_XDP_ZC_SUPPORT
	int err;
#endif /* HAVE_AF_XDP_ZC_SUPPORT */
#endif /* HAVE_XDP_FRAME_STRUCT */
#endif /* HAVE_XDP_BUFF_RXQ */

	size = sizeof(struct xlnid_rx_buffer) * rx_ring->count;

	if (rx_ring->q_vector) {
		node = rx_ring->q_vector->node;
	}

	rx_ring->rx_buffer_info = vmalloc_node(size, node);

	if (!rx_ring->rx_buffer_info) {
		rx_ring->rx_buffer_info = vmalloc(size);
	}

	if (!rx_ring->rx_buffer_info) {
		goto err;
	}

	/* Round up to nearest 4K */
	rx_ring->size = rx_ring->count * sizeof(union xlnid_adv_rx_desc);
	rx_ring->size = ALIGN(rx_ring->size, 4096);

	set_dev_node(dev, node);
	rx_ring->desc = dma_alloc_coherent(dev, rx_ring->size, &rx_ring->dma,
										GFP_KERNEL);
	set_dev_node(dev, orig_node);

	if (!rx_ring->desc)
		rx_ring->desc = dma_alloc_coherent(dev, rx_ring->size,
											&rx_ring->dma, GFP_KERNEL);

	if (!rx_ring->desc) {
		goto err;
	}

#ifdef HAVE_XDP_BUFF_RXQ

	/* XDP RX-queue info */
	if (xdp_rxq_info_reg(&rx_ring->xdp_rxq, adapter->netdev,
							rx_ring->queue_index,
							xlnid_rx_napi_id(rx_ring)) < 0) {
		goto err;
	}

#ifndef HAVE_AF_XDP_ZC_SUPPORT
#ifdef HAVE_XDP_FRAME_STRUCT
	err = xdp_rxq_info_reg_mem_model(&rx_ring->xdp_rxq,
										MEM_TYPE_PAGE_SHARED, NULL);

	if (err) {
		xdp_rxq_info_unreg(&rx_ring->xdp_rxq);
		goto err;
	}

#endif /* HAVE_XDP_FRAME_STRUCT */
#endif /* HAVE_AF_XDP_ZC_SUPPORT */
#endif /* HAVE_XDP_BUFF_RXQ */

	rx_ring->xdp_prog = adapter->xdp_prog;

	return 0;
err:
	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;
	dev_err(dev, "Unable to allocate memory for the Rx descriptor ring\n");
	return -ENOMEM;
}

/**
	xlnid_setup_all_rx_resources - allocate all queues Rx resources
	@adapter: board private structure

	If this function returns with an error, then it's possible one or
	more of the rings is populated (while the rest are not).  It is the
	callers duty to clean those orphaned rings.

	Return 0 on success, negative on failure
**/
static int xlnid_setup_all_rx_resources(struct xlnid_adapter *adapter)
{
	int i, err = 0;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		err = xlnid_setup_rx_resources(adapter, adapter->rx_ring[i]);

		if (!err) {
			continue;
		}

		e_err(probe, "Allocation for Rx Queue %u failed\n", i);
		goto err_setup_rx;
	}

	return 0;
err_setup_rx:

	/* rewind the index freeing the rings as we go */
	while (i--) {
		xlnid_free_rx_resources(adapter->rx_ring[i]);
	}

	return err;
}

/**
	xlnid_free_tx_resources - Free Tx Resources per Queue
	@tx_ring: Tx descriptor ring for a specific queue

	Free all transmit software resources
**/
void xlnid_free_tx_resources(struct xlnid_ring *tx_ring)
{
	xlnid_clean_tx_ring(tx_ring);

	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;

	/* if not set, then don't free */
	if (!tx_ring->desc) {
		return;
	}

	dma_free_coherent(tx_ring->dev, tx_ring->size, tx_ring->desc,
						tx_ring->dma);
	tx_ring->desc = NULL;
}

/**
	xlnid_free_all_tx_resources - Free Tx Resources for All Queues
	@adapter: board private structure

	Free all transmit software resources
**/
static void xlnid_free_all_tx_resources(struct xlnid_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		xlnid_free_tx_resources(adapter->tx_ring[i]);
	}

	for (i = 0; i < adapter->num_xdp_queues; i++)
		if (adapter->xdp_ring[i]->desc) {
			xlnid_free_tx_resources(adapter->xdp_ring[i]);
		}
}

/**
	xlnid_free_rx_resources - Free Rx Resources
	@rx_ring: ring to clean the resources from

	Free all receive software resources
**/
void xlnid_free_rx_resources(struct xlnid_ring *rx_ring)
{
	xlnid_clean_rx_ring(rx_ring);

	rx_ring->xdp_prog = NULL;
#ifdef HAVE_XDP_BUFF_RXQ
	xdp_rxq_info_unreg(&rx_ring->xdp_rxq);
#endif
	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;

	/* if not set, then don't free */
	if (!rx_ring->desc) {
		return;
	}

	dma_free_coherent(rx_ring->dev, rx_ring->size, rx_ring->desc,
						rx_ring->dma);

	rx_ring->desc = NULL;
}

/**
	xlnid_free_all_rx_resources - Free Rx Resources for All Queues
	@adapter: board private structure

	Free all receive software resources
**/
static void xlnid_free_all_rx_resources(struct xlnid_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		xlnid_free_rx_resources(adapter->rx_ring[i]);
	}
}

/**
	xlnid_change_mtu - Change the Maximum Transfer Unit
	@netdev: network interface device structure
	@new_mtu: new value for maximum frame size

	Returns 0 on success, negative on failure
**/
static int xlnid_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

#ifndef HAVE_NETDEVICE_MIN_MAX_MTU
	int max_frame = new_mtu + ETH_HLEN + ETH_FCS_LEN;

	if (adapter->hw.mac.type == xlnid_mac_WESTLAKE) {
		max_frame = new_mtu + ETH_HLEN;
	}

#endif

	if (adapter->xdp_prog) {
		int new_frame_size = new_mtu + XLNID_PKT_HDR_PAD;
		int i;

		for (i = 0; i < adapter->num_rx_queues; i++) {
			struct xlnid_ring *ring = adapter->rx_ring[i];

			if (new_frame_size > xlnid_rx_bufsz(ring)) {
				e_warn(probe,
						"Requested MTU size is not supported with XDP\n");
				return -EINVAL;
			}
		}
	}

#ifndef HAVE_NETDEVICE_MIN_MAX_MTU

	/* MTU < 68 is an error and causes problems on some kernels */
	if ((new_mtu < 68) || (max_frame > XLNID_MAX_JUMBO_FRAME_SIZE)) {
		return -EINVAL;
	}

#endif

	e_info(probe, "changing MTU from %d to %d\n", netdev->mtu, new_mtu);

	/* must set new MTU before calling down or up */
	netdev->mtu = new_mtu;

	if (netif_running(netdev)) {
		xlnid_reinit_locked(adapter);
	}

	return 0;
}

static void xlnid_ptsw_request_irq(struct xlnid_adapter *adapter)
{
	int err;
	xel_pci_info_t *slave_pinfo =
		xlnid_find_pci_info(adapter->hw.bus.bus_id, 1);
	struct pci_dev *slave_pdev = slave_pinfo->dev;
	struct xlnid_hw *slave_hw = slave_pinfo->hw;
	struct xlnid_adapter *slave_adapter = slave_hw->back;

	if (adapter->flags & XLNID_FLAG_MSIX_ENABLED) {
		err = request_irq(
					slave_adapter
					->msix_entries[slave_adapter->num_q_vectors]
					.vector,
					&xlnid_ptsw_intr, 0, "xlnid_ptsw", adapter);
	}

	else if (adapter->flags & XLNID_FLAG_MSI_ENABLED) {
		err = request_irq(slave_pdev->irq, &xlnid_ptsw_intr, 0,
							"xlnid_ptsw", adapter);
	}

	else {
		err = request_irq(slave_pdev->irq, &xlnid_ptsw_intr,
							IRQF_SHARED, "xlnid_ptsw", adapter);
	}

	if (err) {
		e_err(probe, "request_ptsw_irq failed, Error %d\n", err);
	}

	/* only enable link change */
	XLNID_READ_REG_DIRECT(slave_hw, EICR);
	XLNID_WRITE_REG_DIRECT(slave_hw, EIMS, XLNID_EIMS_ENABLE_MASK);

	return;
}

/**
	xlnid_open - Called when a network interface is made active
	@netdev: network interface device structure

	Returns 0 on success, negative value on failure

	The open entry point is called when a network interface is made
	active by the system (IFF_UP).  At this point all resources needed
	for transmit and receive operations are allocated, the interrupt
	handler is registered with the OS, the watchdog timer is started,
	and the stack is notified that the interface is ready.
**/
int xlnid_open(struct net_device *netdev)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	int err;

	/* disallow open during test */
	if (test_bit(__XLNID_TESTING, &adapter->state)) {
		return -EBUSY;
	}

	netif_carrier_off(netdev);

	/* allocate transmit descriptors */
	err = xlnid_setup_all_tx_resources(adapter);

	if (err) {
		goto err_setup_tx;
	}

	/* allocate receive descriptors */
	err = xlnid_setup_all_rx_resources(adapter);

	if (err) {
		goto err_setup_rx;
	}

	/* xel init */
	xlnid_hw_init(adapter);

	xlnid_configure(adapter);

	err = xlnid_request_irq(adapter);

	if (err) {
		goto err_req_irq;
	}

	/* Notify the stack of the actual queue counts. */
	err = netif_set_real_num_tx_queues(
				netdev, adapter->num_rx_pools > 1 ? 1 : adapter->num_tx_queues);

	if (err) {
		goto err_set_queues;
	}

	err = netif_set_real_num_rx_queues(
				netdev, adapter->num_rx_pools > 1 ? 1 : adapter->num_rx_queues);

	if (err) {
		goto err_set_queues;
	}

#ifdef HAVE_PTP_1588_CLOCK
	xlnid_ptp_init(adapter);
#endif /* HAVE_PTP_1588_CLOCK */

	xlnid_up_complete(adapter);

	if (adapter->hw.ptsw_enable && adapter->hw.bus.func == 0) {
		xlnid_ptsw_request_irq(adapter);
	}

#if defined(HAVE_UDP_ENC_RX_OFFLOAD) || defined(HAVE_VXLAN_RX_OFFLOAD)
	xlnid_clear_udp_tunnel_port(adapter, XLNID_VXLANCTRL_ALL_UDPPORT_MASK);
#endif
#ifdef HAVE_UDP_ENC_RX_OFFLOAD
	udp_tunnel_get_rx_info(netdev);
#elif defined(HAVE_VXLAN_RX_OFFLOAD)
	//vxlan_get_rx_port(netdev);
#endif /* HAVE_UDP_ENC_RX_OFFLOAD */

	return XLNID_SUCCESS;

err_set_queues:
	xlnid_free_irq(adapter);
err_req_irq:
	xlnid_free_all_rx_resources(adapter);

	if (!adapter->wol) {
		xlnid_set_phy_power(&adapter->hw, false);
	}

err_setup_rx:
	xlnid_free_all_tx_resources(adapter);
err_setup_tx:
	xlnid_reset(adapter);

	return err;
}

/**
	xlnid_close_suspend - actions necessary to both suspend and close flows
	@adapter: the private adapter struct

	This function should contain the necessary work common to both suspending
	and closing of the device.
*/
static void xlnid_close_suspend(struct xlnid_adapter *adapter)
{
#ifdef HAVE_PTP_1588_CLOCK
	xlnid_ptp_suspend(adapter);
#endif

	if (adapter->hw.phy.ops.enter_lplu) {
		adapter->hw.phy.reset_disable = true;
		xlnid_down(adapter);
		xlnid_enter_lplu(&adapter->hw);
		adapter->hw.phy.reset_disable = false;
	}

	else {
		xlnid_down(adapter);
	}

	xlnid_free_irq(adapter);

	xlnid_free_all_rx_resources(adapter);
	xlnid_free_all_tx_resources(adapter);
}

/**
	xlnid_close - Disables a network interface
	@netdev: network interface device structure

	Returns 0, this is not allowed to fail

	The close entry point is called when an interface is de-activated
	by the OS.  The hardware is still under the drivers control, but
	needs to be disabled.  A global MAC reset is issued to stop the
	hardware, and all transmit and receive resources are freed.
**/
int xlnid_close(struct net_device *netdev)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

#ifdef HAVE_PTP_1588_CLOCK
	xlnid_ptp_stop(adapter);
#endif

	if (netif_device_present(netdev)) {
		xlnid_close_suspend(adapter);
	}

	xlnid_fdir_filter_exit(adapter);

	return 0;
}

#ifdef CONFIG_PM
#ifndef USE_LEGACY_PM_SUPPORT
static int xlnid_resume(struct device *dev)
#else
static int xlnid_resume(struct pci_dev *pdev)
#endif /* USE_LEGACY_PM_SUPPORT */
{
	struct xlnid_adapter *adapter;
	struct net_device *netdev;
	u32 err;

#ifndef USE_LEGACY_PM_SUPPORT
	struct pci_dev *pdev = to_pci_dev(dev);
#endif

	adapter = pci_get_drvdata(pdev);
	netdev = adapter->netdev;
	adapter->hw.hw_addr = adapter->io_addr;
	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
	/*
		pci_restore_state clears dev->state_saved so call
		pci_save_state to restore it.
	*/
	pci_save_state(pdev);

	err = pci_enable_device_mem(pdev);

	if (err) {
		e_dev_err("Cannot enable PCI device from suspend\n");
		return err;
	}

	smp_mb__before_atomic();
	clear_bit(__XLNID_DISABLED, &adapter->state);
	pci_set_master(pdev);

	pci_wake_from_d3(pdev, false);

	xlnid_reset(adapter);

	rtnl_lock();

	err = xlnid_init_interrupt_scheme(adapter);

	if (!err && netif_running(netdev)) {
		err = xlnid_open(netdev);
	}

	if (!err) {
		netif_device_attach(netdev);
	}

	rtnl_unlock();

	return err;
}

#ifndef USE_LEGACY_PM_SUPPORT
/**
	xlnid_freeze - quiesce the device (no IRQ's or DMA)
	@dev: The port's netdev
*/
static int xlnid_freeze(struct device *dev)
{
	struct xlnid_adapter *adapter = pci_get_drvdata(to_pci_dev(dev));
	struct net_device *netdev = adapter->netdev;
	bool lplu_enabled = !!adapter->hw.phy.ops.enter_lplu;

	rtnl_lock();
	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		if (lplu_enabled) {
			adapter->hw.phy.reset_disable = true;
			xlnid_down(adapter);
			adapter->hw.phy.reset_disable = false;
		}

		else {
			xlnid_down(adapter);
		}

		xlnid_free_irq(adapter);
	}

	xlnid_reset_interrupt_capability(adapter);
	rtnl_unlock();

	return 0;
}

/**
	xlnid_thaw - un-quiesce the device
	@dev: The port's netdev
*/
static int xlnid_thaw(struct device *dev)
{
	struct xlnid_adapter *adapter = pci_get_drvdata(to_pci_dev(dev));
	struct net_device *netdev = adapter->netdev;
	bool lplu_enabled = !!adapter->hw.phy.ops.enter_lplu;

	xlnid_set_interrupt_capability(adapter);

	if (netif_running(netdev)) {
		u32 err = xlnid_request_irq(adapter);

		if (err) {
			return err;
		}

		if (lplu_enabled) {
			adapter->hw.phy.reset_disable = true;
			xlnid_up(adapter);
			adapter->hw.phy.reset_disable = false;
		}

		else {
			xlnid_up(adapter);
		}
	}

	netif_device_attach(netdev);

	return 0;
}
#endif /* USE_LEGACY_PM_SUPPORT */
#endif /* CONFIG_PM */

/*
	__xlnid_shutdown is not used when power management
	is disabled on older kernels (<2.6.12). causes a compile
	warning/error, because it is defined and not used.
*/
#if defined(CONFIG_PM) || !defined(USE_REBOOT_NOTIFIER)
static int __xlnid_shutdown(struct pci_dev *pdev, bool *enable_wake)
{
	struct xlnid_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
	struct xlnid_hw *hw = &adapter->hw;
	u32 wufc = adapter->wol;
	xel_pci_info_t *slave_pinfo =
		xlnid_find_pci_info(adapter->hw.bus.bus_id, 1);
	struct xlnid_adapter *slave_adapter = NULL;

#ifdef CONFIG_PM
	int retval = 0;
#endif

	rtnl_lock();
	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		xlnid_close_suspend(adapter);
	}

	if (!adapter->hw.ptsw_enable) {
		xlnid_clear_interrupt_scheme(adapter);
	}

	else if (adapter->hw.ptsw_enable && adapter->hw.bus.func == 0) {
		xlnid_clear_interrupt_scheme(adapter);
		slave_adapter = (struct xlnid_adapter *)slave_pinfo->hw->back;
		xlnid_clear_interrupt_scheme(slave_adapter);
	}

	rtnl_unlock();

#ifdef CONFIG_PM
	retval = pci_save_state(pdev);

	if (retval) {
		return retval;
	}

#endif

	if (wufc) {
		u32 fctrl;

		xlnid_set_rx_mode(netdev);

		/* enable the optics for SFP+ fiber as we can WoL */
		if (hw->mac.ops.enable_tx_laser) {
			hw->mac.ops.enable_tx_laser(hw);
		}

		/* enable the reception of multicast packets */
		fctrl = XLNID_READ_REG(hw, FCTRL);
		fctrl |= XLNID_FCTRL_MPE;
		XLNID_WRITE_REG(hw, FCTRL, fctrl);
	}

	pci_wake_from_d3(pdev, !!wufc);

	*enable_wake = !!wufc;

	if (!*enable_wake) {
		xlnid_set_phy_power(hw, false);
	}

	/* ptsw use function 0 to disable, link xlnid_remove */
	if (adapter->hw.ptsw_enable && adapter->hw.bus.func == 0) {
		pci_disable_device(adapter->pdev);
		slave_adapter = (struct xlnid_adapter *)slave_pinfo->hw->back;
		pci_disable_device(slave_adapter->pdev);
	}

	else if (!adapter->hw.ptsw_enable &&
				!test_and_set_bit(__XLNID_DISABLED, &adapter->state)) {
		pci_disable_device(pdev);
	}

	return 0;
}
#endif /* defined(CONFIG_PM) || !defined(USE_REBOOT_NOTIFIER) */

#ifdef CONFIG_PM
#ifndef USE_LEGACY_PM_SUPPORT
static int xlnid_suspend(struct device *dev)
#else
static int xlnid_suspend(struct pci_dev *pdev,
							pm_message_t __always_unused state)
#endif /* USE_LEGACY_PM_SUPPORT */
{
	int retval;
	bool wake;

#ifndef USE_LEGACY_PM_SUPPORT
	struct pci_dev *pdev = to_pci_dev(dev);
#endif

	retval = __xlnid_shutdown(pdev, &wake);

	if (retval) {
		return retval;
	}

	if (wake) {
		pci_prepare_to_sleep(pdev);
	}

	else {
		pci_wake_from_d3(pdev, false);
		pci_set_power_state(pdev, PCI_D3hot);
	}

	return 0;
}
#endif /* CONFIG_PM */

#ifndef USE_REBOOT_NOTIFIER
static void xlnid_shutdown(struct pci_dev *pdev)
{
	bool wake;

	__xlnid_shutdown(pdev, &wake);

	if (system_state == SYSTEM_POWER_OFF) {
		pci_wake_from_d3(pdev, wake);
		pci_set_power_state(pdev, PCI_D3hot);
	}
}

#endif
#ifdef HAVE_NDO_GET_STATS64
static void xlnid_get_ring_stats64(struct rtnl_link_stats64 *stats,
									struct xlnid_ring *ring)
{
	u64 bytes, packets;
	unsigned int start;

	if (ring) {
		do {
			start = u64_stats_fetch_begin(&ring->syncp);
			packets = ring->stats.packets;
			bytes = ring->stats.bytes;
		} while (u64_stats_fetch_retry(&ring->syncp, start));

		stats->tx_packets += packets;
		stats->tx_bytes += bytes;
	}
}

/**
	xlnid_get_stats64 - Get System Network Statistics
	@netdev: network interface device structure
	@stats: storage space for 64bit statistics

	Returns 64bit statistics, for use in the ndo_get_stats64 callback. This
	function replaces xlnid_get_stats for kernels which support it.
*/
#ifdef HAVE_VOID_NDO_GET_STATS64
static void xlnid_get_stats64(struct net_device *netdev,
								struct rtnl_link_stats64 *stats)
#else
static struct rtnl_link_stats64 *
xlnid_get_stats64(struct net_device *netdev, struct rtnl_link_stats64 *stats)
#endif
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	int i;

	rcu_read_lock();

	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct xlnid_ring *ring = READ_ONCE(adapter->rx_ring[i]);
		u64 bytes, packets;
		unsigned int start;

		if (ring) {
			do {
				start = u64_stats_fetch_begin(&ring->syncp);
				packets = ring->stats.packets;
				bytes = ring->stats.bytes;
			} while (u64_stats_fetch_retry(&ring->syncp, start));

			stats->rx_packets += packets;
			stats->rx_bytes += bytes;
		}
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct xlnid_ring *ring = READ_ONCE(adapter->tx_ring[i]);

		xlnid_get_ring_stats64(stats, ring);
	}

	for (i = 0; i < adapter->num_xdp_queues; i++) {
		struct xlnid_ring *ring = READ_ONCE(adapter->xdp_ring[i]);

		xlnid_get_ring_stats64(stats, ring);
	}

	rcu_read_unlock();

	/* following stats updated by xlnid_watchdog_task() */
	stats->multicast = netdev->stats.multicast;
	stats->rx_errors = netdev->stats.rx_errors;
	stats->rx_length_errors = netdev->stats.rx_length_errors;
	stats->rx_crc_errors = netdev->stats.rx_crc_errors;
	stats->rx_missed_errors = netdev->stats.rx_missed_errors;
#ifndef HAVE_VOID_NDO_GET_STATS64

	return stats;
#endif
}
#else /* !HAVE_NDO_GET_STATS64 */
/**
		xlnid_get_stats - Get System Network Statistics
		@netdev: network interface device structure

		Returns the address of the device statistics structure.
		The statistics are actually updated from the timer callback.
	**/
static struct net_device_stats *xlnid_get_stats(struct net_device *netdev)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	/* update the stats data */
	xlnid_update_stats(adapter);

#ifdef HAVE_NETDEV_STATS_IN_NETDEV
	/* only return the current stats */
	return &netdev->stats;
#else
	/* only return the current stats */
	return &adapter->net_stats;
#endif /* HAVE_NETDEV_STATS_IN_NETDEV */
}
#endif /* HAVE_NDO_GET_STATS64 */

/**
	xlnid_test_update_stats - Update the board statistics counters.
	@adapter: board private structure
**/
void xlnid_update_stats(struct xlnid_adapter *adapter)
{
#ifdef HAVE_NETDEV_STATS_IN_NETDEV
	struct net_device_stats *net_stats = &adapter->netdev->stats;
#else
	struct net_device_stats *net_stats = &adapter->net_stats;
#endif /* HAVE_NETDEV_STATS_IN_NETDEV */
	struct xlnid_hw *hw = &adapter->hw;
	struct xlnid_hw_stats *hwstats = &adapter->stats;
	u64 total_mpc = 0;
	u32 i = 0;
	u32 bprc = 0;
	u32 mprc = 0;
	u32 uprc = 0;
	u32 lxon = 0;
	u32 xon_off_tot = 0;
	u64 non_eop_descs = 0;
	u64 restart_queue = 0;
	u64 tx_busy = 0;
	u64 alloc_rx_page_failed = 0;
	u64 alloc_rx_buff_failed = 0;
	u64 alloc_rx_page = 0;
	u64 bytes = 0;
	u64 packets = 0;
	u64 hw_csum_rx_error = 0;
	u64 crc_error = 0;

	if (test_bit(__XLNID_DOWN, &adapter->state) ||
			test_bit(__XLNID_RESETTING, &adapter->state)) {
		return;
	}

	if (adapter->flags2 & XLNID_FLAG2_RSC_ENABLED) {
		u64 rsc_count = 0;
		u64 rsc_flush = 0;

		for (i = 0; i < adapter->num_rx_queues; i++) {
			rsc_count += adapter->rx_ring[i]->rx_stats.rsc_count;
			rsc_flush += adapter->rx_ring[i]->rx_stats.rsc_flush;
		}

		adapter->rsc_total_count = rsc_count;
		adapter->rsc_total_flush = rsc_flush;
	}

	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct xlnid_ring *rx_ring = adapter->rx_ring[i];

		non_eop_descs += rx_ring->rx_stats.non_eop_descs;
		alloc_rx_page += rx_ring->rx_stats.alloc_rx_page;
		alloc_rx_page_failed += rx_ring->rx_stats.alloc_rx_page_failed;
		alloc_rx_buff_failed += rx_ring->rx_stats.alloc_rx_buff_failed;
		hw_csum_rx_error += rx_ring->rx_stats.csum_err;
		crc_error += rx_ring->rx_stats.crc_err;
		bytes += rx_ring->stats.bytes;
		packets += rx_ring->stats.packets;
	}

	adapter->non_eop_descs = non_eop_descs;
	adapter->alloc_rx_page = alloc_rx_page;
	adapter->alloc_rx_page_failed = alloc_rx_page_failed;
	adapter->alloc_rx_buff_failed = alloc_rx_buff_failed;
	adapter->hw_csum_rx_error = hw_csum_rx_error;
	net_stats->rx_bytes = bytes;
	net_stats->rx_packets = packets;

	bytes = 0;
	packets = 0;

	/* gather some stats to the adapter struct that are per queue */
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct xlnid_ring *tx_ring = adapter->tx_ring[i];

		restart_queue += tx_ring->tx_stats.restart_queue;
		tx_busy += tx_ring->tx_stats.tx_busy;
		bytes += tx_ring->stats.bytes;
		packets += tx_ring->stats.packets;
	}

	for (i = 0; i < adapter->num_xdp_queues; i++) {
		struct xlnid_ring *xdp_ring = adapter->xdp_ring[i];

		restart_queue += xdp_ring->tx_stats.restart_queue;
		tx_busy += xdp_ring->tx_stats.tx_busy;
		bytes += xdp_ring->stats.bytes;
		packets += xdp_ring->stats.packets;
	}

	adapter->restart_queue = restart_queue;
	adapter->tx_busy = tx_busy;
	net_stats->tx_bytes = bytes;
	net_stats->tx_packets = packets;

	//hwstats->crcerrs = crc_error;
	hwstats->crcerrs += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_CRCERRS);

	//xlnid_update_xoff_received(adapter);

	/* Good Octets Received Count */
	hwstats->gorc += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_GORC);
	hwstats->gotc += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_GOTC);

	/* work around hardware counting issue */
	//hwstats->gprc -= missed_rx;

	/* DCB */
	//xlnid_update_xoff_received(adapter);

#ifdef HAVE_TX_MQ
	/* FDIR Match Statistics */
	hwstats->fdirmatch += XLNID_READ_REG_MAC(hw, WESTLAKE_FDIRMATCH);

	/* FDIR Miss Match Statistics */
	hwstats->fdirmiss += XLNID_READ_REG_MAC(hw, WESTLAKE_FDIRMISS);
#endif

	/* GPRC = BPRC + MPRC + UPRC */
	/* Broadcast Packets Received Count */
	bprc = xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_BPRC);
	hwstats->bprc += bprc;
	hwstats->gprc += bprc;

	/* Multicast Packets Received Count */
	mprc = xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_MPRC);
	hwstats->mprc += mprc;
	hwstats->gprc += mprc;

	/* Unicast Packets Received Count */
	uprc += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_UPRC);
	hwstats->gprc += uprc;

	/* Receive Oversize Count */
	hwstats->roc += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_ROC);

	/* Packets Received (64 Bytes) Count */
	hwstats->prc64 += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_PRC64);

	/* Packets Received [65 - 127 Bytes] Count */
	hwstats->prc127 += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_PRC127);

	/* Packets Received [128 - 255 Bytes] Count */
	hwstats->prc255 += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_PRC255);

	/* Packets Received [256 - 511 Bytes] Count */
	hwstats->prc511 += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_PRC511);

	/* Packets Received [512 - 1023 Bytes] Count */
	hwstats->prc1023 += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_PRC1023);

	/* Packets Received [Greater than 1024 Bytes] Count */
	hwstats->prc1522 += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_PRC1522);

	/* Receive Length Error Count */
	hwstats->rlec += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_RLEC);

	/* Link Pause ON Transmitted Count */
	lxon = xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_LXONTXC);
	hwstats->lxontxc += lxon;

	/* Good Packets Transmitted Count */
	hwstats->gptc += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_GPTC);

	/* Multicast Packets Transmitted Count */
	hwstats->mptc += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_MPTC);

	/* not include link pause count */
	xon_off_tot = lxon; // + lxoff
	hwstats->gptc -= xon_off_tot;
	hwstats->mptc -= xon_off_tot;

	/* Receive Undersize Count */
	hwstats->ruc += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_RUC);

	/* Receive Fragment Count */
	//hwstats->rfc += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_RFC);

	/* Receive Jabber Count */
	//hwstats->rjc += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_RJC);

	/* Total Packets Received */
	hwstats->tpr += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_TPR);

	/*  Packets Transmitted (64 Bytes) Count */
	hwstats->ptc64 += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_PTC64);
	hwstats->ptc64 -= xon_off_tot;

	/* Packets Transmitted [65 - 127 Bytes] Count */
	hwstats->ptc127 += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_PTC127);

	/* Packets Transmitted [128 - 255 Bytes] Count */
	hwstats->ptc255 += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_PTC255);

	/* Packets Transmitted [256 - 511 Bytes] Count */
	hwstats->ptc511 += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_PTC511);

	/* Packets Transmitted [512 - 1023 Bytes] Count */
	hwstats->ptc1023 += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_PTC1023);

	/* Packets Transmitted [Greater than 1024 Bytes] Count */
	hwstats->ptc1522 += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_PTC1522);

	/* Broadcast Packets Transmitted Count */
	hwstats->bptc += xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_BPTC);

	/* Fill out the OS statistics structure */
	net_stats->multicast = hwstats->mprc;

	/* Rx Errors */
	net_stats->rx_errors = hwstats->crcerrs + hwstats->rlec;
	net_stats->rx_dropped = 0;
	net_stats->rx_length_errors = hwstats->rlec;
	net_stats->rx_crc_errors = hwstats->crcerrs;
	net_stats->rx_missed_errors = total_mpc;
}

#ifdef HAVE_TX_MQ
/**
	xlnid_fdir_reinit_subtask - worker thread to reinit FDIR filter table
	@adapter: pointer to the device adapter structure
**/
static void xlnid_fdir_reinit_subtask(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	int i;

	if (!(adapter->flags2 & XLNID_FLAG2_FDIR_REQUIRES_REINIT)) {
		return;
	}

	adapter->flags2 &= ~XLNID_FLAG2_FDIR_REQUIRES_REINIT;

	/* if interface is down do nothing */
	if (test_bit(__XLNID_DOWN, &adapter->state)) {
		return;
	}

	/* do nothing if we are not using signature filters */
	if (!(adapter->flags & XLNID_FLAG_FDIR_HASH_CAPABLE)) {
		return;
	}

	adapter->fdir_overflow++;

	if (xlnid_reinit_fdir_tables(hw) == XLNID_SUCCESS) {
		for (i = 0; i < adapter->num_tx_queues; i++)
			set_bit(__XLNID_TX_FDIR_INIT_DONE,
					&(adapter->tx_ring[i]->state));

		for (i = 0; i < adapter->num_xdp_queues; i++)
			set_bit(__XLNID_TX_FDIR_INIT_DONE,
					&adapter->xdp_ring[i]->state);

		/* re-enable flow director interrupts */
		XLNID_WRITE_REG_DIRECT(hw, EIMS, XLNID_EIMS_FLOW_DIR);
	}

	else {
		e_err(probe, "failed to finish FDIR re-initialization, "
				"ignored adding FDIR ATR filters\n");
	}
}

#endif /* HAVE_TX_MQ */
/**
	xlnid_check_hang_subtask - check for hung queues and dropped interrupts
	@adapter: pointer to the device adapter structure

	This function serves two purposes.  First it strobes the interrupt lines
	in order to make certain interrupts are occurring.  Secondly it sets the
	bits needed to check for TX hangs.  As a result we should immediately
	determine if a hang has occurred.
*/
static void xlnid_check_hang_subtask(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	u64 eics = 0;
	int i;

	/* If we're down, removing or resetting, just bail */
	if (test_bit(__XLNID_DOWN, &adapter->state) ||
			test_bit(__XLNID_REMOVING, &adapter->state) ||
			test_bit(__XLNID_RESETTING, &adapter->state)) {
		return;
	}

	/* Force detection of hung controller */
	if (netif_carrier_ok(adapter->netdev)) {
		for (i = 0; i < adapter->num_tx_queues; i++) {
			set_check_for_tx_hang(adapter->tx_ring[i]);
		}

		for (i = 0; i < adapter->num_xdp_queues; i++) {
			set_check_for_tx_hang(adapter->xdp_ring[i]);
		}
	}

	if (!(adapter->flags & XLNID_FLAG_MSIX_ENABLED)) {
		/*
			for legacy and MSI interrupts don't set any bits
			that are enabled for EIAM, because this operation
			would set *both* EIMS and EICS for any bit in EIAM
		*/
		XLNID_WRITE_REG_DIRECT(
			hw, EICS, (XLNID_EICS_TCP_TIMER | XLNID_EICS_OTHER));
	}

	else {
		/* get one bit for every active tx/rx interrupt vector */
		for (i = 0; i < adapter->num_q_vectors; i++) {
			struct xlnid_q_vector *qv = adapter->q_vector[i];

			if (qv->rx.ring || qv->tx.ring) {
				eics |= ((u64)1 << i);
			}
		}
	}

	/* Cause software interrupt to ensure rings are cleaned */
	xlnid_irq_rearm_queues(adapter, eics);
}

/**
	xlnid_watchdog_update_link - update the link status
	@adapter: pointer to the device adapter structure
**/
static void xlnid_watchdog_update_link(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 link_speed = adapter->link_speed;
	bool link_up = adapter->link_up;
	bool pfc_en = false;
	u32 link_speed_ptsw = XLNID_LINK_SPEED_UNKNOWN;
	bool link_up_ptsw = false;
	u32 mode = 0;
	xel_pci_info_t *slave_pinfo = NULL;

	if (hw->ptsw_enable) {
		slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
	}

	if (!(adapter->flags & XLNID_FLAG_NEED_LINK_UPDATE)) {
#ifdef XLNID_AUTO_MEDIA_SUPPORT

		if (adapter->link_up && !hw->ptsw_enable &&
				(((hw->phy.media_type == xlnid_media_type_copper) &&
					(!hw->link_mode)) ||
					((hw->phy.media_type == xlnid_media_type_fiber) &&
					(hw->link_mode))))
#endif
		{
			return;
		}
	}

	if (hw->mac.ops.check_link) {
		hw->mac.ops.check_link(hw, &link_speed, &link_up, false);

		if (hw->mac.type == xlnid_mac_WESTLAKE && hw->ptsw_enable &&
				adapter->pdev->devfn == 0) {
			hw->mac.ops.check_link(slave_pinfo->hw,
									&link_speed_ptsw, &link_up_ptsw,
									false);
		}
	}

	else {
		/* always assume link is up, if no check link function */
		link_speed = XLNID_LINK_SPEED_10GB_FULL;
		link_up = true;
	}

#ifdef HAVE_DCBNL_IEEE

	if (adapter->xlnid_ieee_pfc) {
		pfc_en |= !!(adapter->xlnid_ieee_pfc->pfc_en);
	}

#endif

	if (link_up && !((adapter->flags & XLNID_FLAG_DCB_ENABLED) && pfc_en)) {
		hw->mac.ops.fc_enable(hw);
	}

	if (link_up || link_up_ptsw ||
			time_after(jiffies, (adapter->link_check_timeout +
									XLNID_TRY_LINK_TIMEOUT))) {
		adapter->flags &= ~XLNID_FLAG_NEED_LINK_UPDATE;
		XLNID_WRITE_REG_DIRECT(hw, EIMS, XLNID_EIMC_LSC);
		XLNID_WRITE_FLUSH(hw);
	}

	adapter->link_up = link_up;
	adapter->link_speed = link_speed;

	if (hw->mac.type == xlnid_mac_WESTLAKE && hw->ptsw_enable &&
			adapter->pdev->devfn == 0) {
		int i = 0;
		u32 pdbuf_stat, gmac_config, crg_config;

		adapter->link_up = link_up | link_up_ptsw;

		if (link_up) {
			if (!link_up_ptsw) {
				/* port0 up and port1 down, ptsw use port0 */
				hw->ptsw_backup = false;

				mode = XLNID_READ_REG_MAC(
							hw, WESTLAKE_CFG_PTSW_MODE);

				if (mode & XLNID_CRG_RX_PTSW_PORT) {
					/* 1. disable port1 rx & tx */
					gmac_config = xlnid_read_mac_westlake(
										slave_pinfo->hw,
										WESTLAKE_GMAC_CONFIG);
					gmac_config &= ~(0xC);
					xlnid_write_mac_westlake(
						slave_pinfo->hw,
						WESTLAKE_GMAC_CONFIG,
						gmac_config);

					/* 2. reset port1 ethport(only rx) */
					crg_config = XLNID_READ_REG_MAC(
										slave_pinfo->hw,
										WESTLAKE_CRG_ETH_PORT_RST_N);
					crg_config &= ~(0x20);
					XLNID_WRITE_REG_MAC(
						slave_pinfo->hw,
						WESTLAKE_CRG_ETH_PORT_RST_N,
						crg_config);

					/* 3. enable port1 macsec loopback */
					XLNID_WRITE_REG_MAC(
						slave_pinfo->hw,
						WESTLAKE_MACSEC_ETH_LOOPBACK_CFG,
						1);

					/* 4. wait port1 rx pdbuf empty */
					for (i = 0; i < 3000; i++) {
						pdbuf_stat = XLNID_READ_REG_MAC(
											slave_pinfo->hw,
											WESTLAKE_RX_PBUF_STAT);

						if ((pdbuf_stat & 0x11) ==
								0x11) {
							break;
						}

						usec_delay(1);
					}

					if (i >= 3000) {
						printk("%s:%d: pdbuf_stat = 0x%08x\r\n",
								__func__, __LINE__,
								pdbuf_stat);
					}

					/* 5. ptsw enable port0 */
					mode &= ~XLNID_CRG_RX_PTSW_PORT;
					XLNID_WRITE_REG_MAC(
						hw, WESTLAKE_CFG_PTSW_MODE,
						mode);

					/* 6. disable port0 macsec loopback */
					XLNID_WRITE_REG_MAC(
						hw,
						WESTLAKE_MACSEC_ETH_LOOPBACK_CFG,
						0);

					/* 7. unreset port0 ethport(only rx) */
					crg_config = XLNID_READ_REG_MAC(
										hw,
										WESTLAKE_CRG_ETH_PORT_RST_N);
					crg_config |= 0x20;
					XLNID_WRITE_REG_MAC(
						hw, WESTLAKE_CRG_ETH_PORT_RST_N,
						crg_config);

					/* 8. enable port0 rx & tx */
					gmac_config = xlnid_read_mac_westlake(
										&adapter->hw,
										WESTLAKE_GMAC_CONFIG);
					gmac_config |= 0xC;
					xlnid_write_mac_westlake(
						&adapter->hw,
						WESTLAKE_GMAC_CONFIG,
						gmac_config);

					/*
						printk("%s:%d: port0 gmac = 0x%08x, port1 gmac = 0x%08x\r\n",
						__func__, __LINE__,
						xlnid_read_mac_westlake(&adapter->hw, WESTLAKE_GMAC_CONFIG),
						xlnid_read_mac_westlake(slave_pinfo->hw, WESTLAKE_GMAC_CONFIG));
					*/
					e_info(drv, "NIC work on port0\n");
					rtnl_lock();
					call_netdevice_notifiers(
						NETDEV_NOTIFY_PEERS,
						adapter->netdev);
					rtnl_unlock();
				}
			}

			adapter->link_speed = link_speed;
		}

		else {
			if (link_up_ptsw) {
				/* port0 down & port1 up */

				/* copper/fiber config use port1 */
				hw->ptsw_backup = true;
				adapter->link_speed = link_speed_ptsw;

				hw->phy.media_type =
					slave_pinfo->hw->phy.media_type;
				XLNID_WRITE_REG_MAC(
					hw, WESTLAKE_CFG_INTF_CTRL,
					XLNID_READ_REG_MAC(
						slave_pinfo->hw,
						WESTLAKE_CFG_INTF_CTRL));
				XLNID_WRITE_REG_MAC(
					hw, WESTLAKE_GEPHY_LED_STATUS,
					XLNID_READ_REG_MAC(
						slave_pinfo->hw,
						WESTLAKE_GEPHY_LED_STATUS));
				hw->phy.autoneg_advertised =
					slave_pinfo->hw->phy.autoneg_advertised;

				/* ptsw use port1 */
				mode = XLNID_READ_REG_MAC(
							hw, WESTLAKE_CFG_PTSW_MODE);

				if (!(mode & XLNID_CRG_RX_PTSW_PORT)) {
					/* 1. disable port0 rx & tx */
					gmac_config = xlnid_read_mac_westlake(
										hw, WESTLAKE_GMAC_CONFIG);
					gmac_config &= ~0xC;
					xlnid_write_mac_westlake(
						hw, WESTLAKE_GMAC_CONFIG,
						gmac_config);

					/* 2. reset port0 ethport(only rx) */
					crg_config = XLNID_READ_REG_MAC(
										hw,
										WESTLAKE_CRG_ETH_PORT_RST_N);
					crg_config &= ~(0x20);
					XLNID_WRITE_REG_MAC(
						hw, WESTLAKE_CRG_ETH_PORT_RST_N,
						crg_config);

					/* 3. enable port0 macsec loopback */
					XLNID_WRITE_REG_MAC(
						hw,
						WESTLAKE_MACSEC_ETH_LOOPBACK_CFG,
						1);

					/* 4. wait port0 rx pdbuf empty */
					for (i = 0; i < 3000; i++) {
						pdbuf_stat = XLNID_READ_REG_MAC(
											&adapter->hw,
											WESTLAKE_RX_PBUF_STAT);

						if ((pdbuf_stat & 0x11) ==
								0x11) {
							break;
						}

						usec_delay(1);
					}

					if (i == 3000) {
						printk("%s:%d: pbuf_stat = 0x%08x\r\n",
								__func__, __LINE__,
								pdbuf_stat);
					}

					/* 5. ptsw enable port1 */
					mode |= XLNID_CRG_RX_PTSW_PORT;
					XLNID_WRITE_REG_MAC(
						hw, WESTLAKE_CFG_PTSW_MODE,
						mode);

					/* 6. disable port1 macsec loopback */
					XLNID_WRITE_REG_MAC(
						slave_pinfo->hw,
						WESTLAKE_MACSEC_ETH_LOOPBACK_CFG,
						0);

					/* 7. unreset port1 ethport(only rx) */
					crg_config = XLNID_READ_REG_MAC(
										slave_pinfo->hw,
										WESTLAKE_CRG_ETH_PORT_RST_N);
					crg_config |= 0x20;
					XLNID_WRITE_REG_MAC(
						slave_pinfo->hw,
						WESTLAKE_CRG_ETH_PORT_RST_N,
						crg_config);

					/* 8. enable port1 rx and tx */
					gmac_config = xlnid_read_mac_westlake(
										slave_pinfo->hw,
										WESTLAKE_GMAC_CONFIG);
					gmac_config |= 0xC;
					xlnid_write_mac_westlake(
						slave_pinfo->hw,
						WESTLAKE_GMAC_CONFIG,
						gmac_config);

					/*
						printk("%s:%d: port0 gmac = 0x%08x, port1 gmac = 0x%08x\r\n",
						__func__, __LINE__,
						xlnid_read_mac_westlake(&adapter->hw, WESTLAKE_GMAC_CONFIG),
						xlnid_read_mac_westlake(slave_pinfo->hw, WESTLAKE_GMAC_CONFIG));
					*/
					e_info(drv, "NIC work on port1\n");
					rtnl_lock();
					call_netdevice_notifiers(
						NETDEV_NOTIFY_PEERS,
						adapter->netdev);
					rtnl_unlock();
				}
			}
		}
	}

	if (!hw->ptsw_enable) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_ETH_LOOPBACK_CFG, 0);
	}

	if (hw->mac.ops.dmac_config && hw->mac.dmac_config.watchdog_timer) {
		u8 num_tcs = netdev_get_num_tc(adapter->netdev);

		if (hw->mac.dmac_config.link_speed != link_speed ||
				hw->mac.dmac_config.num_tcs != num_tcs) {
			hw->mac.dmac_config.link_speed = link_speed;
			hw->mac.dmac_config.num_tcs = num_tcs;
			hw->mac.ops.dmac_config(hw);
		}
	}
}

static void xlnid_update_default_up(struct xlnid_adapter *adapter)
{
	u8 up = 0;
#ifdef HAVE_DCBNL_IEEE
	struct net_device *netdev = adapter->netdev;
	struct dcb_app app = {
		.selector = DCB_APP_IDTYPE_ETHTYPE,
		.protocol = 0,
	};
	up = dcb_getapp(netdev, &app);
#endif

	adapter->default_up = up;
}

/**
	xlnid_watchdog_link_is_up - update netif_carrier status and
								print link up message
	@adapter: pointer to the device adapter structure
**/
static void xlnid_watchdog_link_is_up(struct xlnid_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct xlnid_hw *hw = &adapter->hw;
	u32 link_speed = adapter->link_speed;
	const char *speed_str;
	bool flow_rx, flow_tx;
	u32 val;

#ifdef XLNID_GEPHY_OPTIMIZE
	u32 bmcr, iso, ctrl, baset_full;
	u32 old_bmcr, speed_select, status, recv_status, idle_error;
#endif

	/* only continue if link was previously down */
	if (netif_carrier_ok(netdev))
#ifndef XLNID_GEPHY_OPTIMIZE
		return;

#else
	{
		if (hw->phy.media_type == xlnid_media_type_copper) {
			bmcr = xlnid_read_gephy_westlake(hw, MII_BMCR);
			speed_select = (bmcr & XLNID_PHY_BMCR_SPEED);

			switch (link_speed) {
			case XLNID_LINK_SPEED_1GB_FULL:
				xlnid_read_gephy_westlake(hw, MII_STAT1000);
				status = xlnid_read_gephy_westlake(
				hw, MII_STAT1000);
				recv_status = status &
				XLNID_PHY_1000STATUS_STATUS;
				idle_error = status &
				XLNID_PHY_1000STATUS_IDLE_ERROR;

				/*  check weather current speed equal speed selection of bmcr reg
				check weather local&remote receiver id OK
				check weather idle error count is 0
				if not restart autoneg
				*/
				if ((speed_select != 0x40) ||
				(recv_status !=
				XLNID_PHY_1000STATUS_STATUS) ||
				(idle_error != 0x0)) {
				bmcr |= XLNID_PHY_BMCR_RESART_AUTONEG;
				xlnid_write_gephy_westlake(hw, MII_BMCR,
				bmcr);
				}
				break;

			case XLNID_LINK_SPEED_100_FULL:
			case XLNID_LINK_SPEED_100_HALF:
				xlnid_read_gephy_westlake(hw, MII_EEROR_PACKET);
				status = xlnid_read_gephy_westlake(
								hw, MII_EEROR_PACKET);

				/*  check weather current speed equal speed selection of bmcr reg
					check weather error packet count is 0
					if not restart autoneg
				*/
				if ((speed_select != 0x2000) ||
						(status != 0x0)) {
					bmcr |= XLNID_PHY_BMCR_RESART_AUTONEG;
					xlnid_write_gephy_westlake(hw, MII_BMCR,
												bmcr);
				}

				break;

			case XLNID_LINK_SPEED_10_FULL:
			case XLNID_LINK_SPEED_10_HALF:
				if (speed_select != 0x0) {
					bmcr |= XLNID_PHY_BMCR_RESART_AUTONEG;
					xlnid_write_gephy_westlake(hw, MII_BMCR,
												bmcr);
				}

				break;

			default:
				break;
			}
		}

		return;
	}
#endif

	/* set 1588 */
	val = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_P1588_BYPASS);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_P1588_BYPASS, 0);

	adapter->flags2 &= ~XLNID_FLAG2_SEARCH_FOR_SFP;

	flow_tx = false;
	flow_rx = false;

#ifdef XLNID_GEPHY_OPTIMIZE

	if (hw->phy.media_type == xlnid_media_type_copper) {
		bmcr = xlnid_read_gephy_westlake(hw, MII_BMCR);
		old_bmcr = bmcr;
		iso = (bmcr & XLNID_PHY_BMCR_ISOLATE);

		/* when down->up, check weather bit isolate is 1, if 1 set 0, and 10M must sleep 10ms */
		if (iso == XLNID_PHY_BMCR_ISOLATE) {
			bmcr &= ~XLNID_PHY_BMCR_ISOLATE;

			if ((link_speed == XLNID_LINK_SPEED_10_FULL) ||
					(link_speed == XLNID_LINK_SPEED_10_HALF)) {
				msec_delay(10);
			}
		}

		/* when 1000M and down->up, check bit 9 of 1000base-t ctrl reg, if not 1, set 1 and restart neg */
		if (link_speed == XLNID_LINK_SPEED_1GB_FULL) {
			ctrl = xlnid_read_gephy_westlake(hw, MII_CTRL1000);
			baset_full = (ctrl & XLNID_PHY_1000CTRL_1000FULL);

			if (baset_full == 0x0) {
				ctrl |= XLNID_PHY_1000CTRL_1000FULL;
				bmcr |= XLNID_PHY_BMCR_RESART_AUTONEG;
				xlnid_write_gephy_westlake(hw, MII_CTRL1000,
											ctrl);
			}
		}

		/* if bmcr has changed, write MII_BMCR */
		if (old_bmcr != bmcr) {
			xlnid_write_gephy_westlake(hw, MII_BMCR, bmcr);
		}
	}

#endif

#ifdef HAVE_PTP_1588_CLOCK
	adapter->last_rx_ptp_check = jiffies;
#endif

	switch (link_speed) {
	case XLNID_LINK_SPEED_10GB_FULL:
		speed_str = "10 Gbps";
		break;

	case XLNID_LINK_SPEED_5GB_FULL:
		speed_str = "5 Gbps";
		break;

	case XLNID_LINK_SPEED_2_5GB_FULL:
		speed_str = "2.5 Gbps";
		break;

	case XLNID_LINK_SPEED_1GB_FULL:
		speed_str = "1 Gbps Full Duplex";
		break;

	case XLNID_LINK_SPEED_1GB_HALF:
		speed_str = "1 Gbps Half Duplex";
		break;

	case XLNID_LINK_SPEED_100_FULL:
		speed_str = "100 Mbps Full Duplex";
		break;

	case XLNID_LINK_SPEED_100_HALF:
		speed_str = "100 Mbps Half Duplex";
		break;

	case XLNID_LINK_SPEED_10_FULL:
		speed_str = "10 Mbps Full Duplex";
		break;

	case XLNID_LINK_SPEED_10_HALF:
		speed_str = "10 Mbps Half Duplex";
		break;

	default:
		speed_str = "unknown speed";
		break;
	}

	e_info(drv, "NIC Link is Up %s, Flow Control: %s\n", speed_str,
			((flow_rx && flow_tx) ?
			"RX/TX" :
			(flow_rx ? "RX" : (flow_tx ? "TX" : "None"))));

	netif_carrier_on(netdev);

#if IS_ENABLED(CONFIG_MACSEC)
	xlnid_macsec_enable(adapter);
#endif

	/* Turn on malicious driver detection */
	if (hw->mac.ops.enable_mdd && (adapter->flags & XLNID_FLAG_MDD_ENABLED)) {
		hw->mac.ops.enable_mdd(hw);
	}

	netif_tx_wake_all_queues(netdev);
	/* update the default user priority for VFs */
	xlnid_update_default_up(adapter);
}

/**
	xlnid_test_watchdog_link_is_down - update netif_carrier status and
								print link down message
	@adapter: pointer to the adapter structure
**/
static void xlnid_watchdog_link_is_down(struct xlnid_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	u32 val;
	u32 gmac;
	struct xlnid_hw *hw = &adapter->hw;

#ifdef XLNID_GEPHY_OPTIMIZE
	u32 link_speed = adapter->link_speed;
	u32 isolate, bmcr, auto_neg;
#endif

	adapter->link_up = false;
	adapter->link_speed = 0;

	/* only continue if link was up previously */
	if (!netif_carrier_ok(netdev)) {
#ifdef XLNID_GEPHY_OPTIMIZE

		if (hw->phy.media_type == xlnid_media_type_copper) {
			bmcr = xlnid_read_gephy_westlake(hw, MII_BMCR);
			isolate = (bmcr & XLNID_PHY_BMCR_ISOLATE);

			/* when 100M and down，check weather bit isolate is 0, if 0, set 1 */
			if ((link_speed == XLNID_LINK_SPEED_100_FULL) ||
					(link_speed == XLNID_LINK_SPEED_100_HALF)) {
				if (isolate == 0x0) {
					bmcr |= XLNID_PHY_BMCR_ISOLATE;
					xlnid_write_gephy_westlake(hw, MII_BMCR,
												bmcr);
				}
			}
		}

#endif
		return;
	}

	/* 1. disable gmac rx & tx */
	gmac = xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_CONFIG);
	gmac &= ~(0xc);
	xlnid_write_mac_westlake(hw, WESTLAKE_GMAC_CONFIG, gmac);

	/* 2. set 1588 bypass */
	val = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_P1588_BYPASS);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_P1588_BYPASS, 1);

	/* 3. reset ethport (rx) */
	val = XLNID_READ_REG_MAC(hw, WESTLAKE_CRG_ETH_PORT_RST_N);
	val &= ~(0x25);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CRG_ETH_PORT_RST_N, val);

	/* 4. unreset ethport (rx) */
	val = XLNID_READ_REG_MAC(hw, WESTLAKE_CRG_ETH_PORT_RST_N);
	val |= 0x25;
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CRG_ETH_PORT_RST_N, val);

	/* 5. set gmac */
	gmac = xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_CONFIG);
	gmac |= 0xc;
	xlnid_write_mac_westlake(hw, WESTLAKE_GMAC_CONFIG, gmac);

#ifdef XLNID_GEPHY_OPTIMIZE

	if (hw->phy.media_type == xlnid_media_type_copper) {
		/* when up->down, if auto_neg is enable, restart auto negotiation once */
		bmcr = xlnid_read_gephy_westlake(hw, MII_BMCR);
		auto_neg = (bmcr & XLNID_PHY_BMCR_AUTONEG_ENBALE);

		if (auto_neg == XLNID_PHY_BMCR_AUTONEG_ENBALE) {
			bmcr |= XLNID_PHY_BMCR_RESART_AUTONEG;
			xlnid_write_gephy_westlake(hw, MII_BMCR, bmcr);
		}
	}

#endif

	e_info(drv, "NIC Link is Down\n");
	netif_carrier_off(netdev);
	netif_tx_stop_all_queues(netdev);
}

static bool xlnid_ring_tx_pending(struct xlnid_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct xlnid_ring *tx_ring = adapter->tx_ring[i];

		if (tx_ring->next_to_use != tx_ring->next_to_clean) {
			return true;
		}
	}

	for (i = 0; i < adapter->num_xdp_queues; i++) {
		struct xlnid_ring *ring = adapter->xdp_ring[i];

		if (ring->next_to_use != ring->next_to_clean) {
			return true;
		}
	}

	return false;
}

static bool xlnid_vf_tx_pending(struct xlnid_adapter *adapter)
{
	return false;
}

/**
	xlnid_watchdog_flush_tx - flush queues on link down
	@adapter: pointer to the device adapter structure
**/
static void xlnid_watchdog_flush_tx(struct xlnid_adapter *adapter)
{
	if (!netif_carrier_ok(adapter->netdev)) {
		if (xlnid_ring_tx_pending(adapter) ||
				xlnid_vf_tx_pending(adapter)) {
			/*  We've lost link, so the controller stops DMA,
				but we've got queued Tx work that's never going
				to get done, so reset controller to flush Tx.
				(Do the reset outside of interrupt context).
			*/
			e_warn(drv,
					"initiating reset due to lost link with pending Tx work\n");
			set_bit(__XLNID_RESET_REQUESTED, &adapter->state);
		}
	}
}


/**
	xlnid_watchdog_subtask - check and bring link up
	@adapter: pointer to the device adapter structure
**/
static void xlnid_watchdog_subtask(struct xlnid_adapter *adapter)
{
	/* if interface is down, removing or resetting, do nothing */
	if (test_bit(__XLNID_DOWN, &adapter->state) ||
			test_bit(__XLNID_REMOVING, &adapter->state) ||
			test_bit(__XLNID_RESETTING, &adapter->state)) {
		return;
	}

	xlnid_watchdog_update_link(adapter);

	if (adapter->link_up) {
		xlnid_watchdog_link_is_up(adapter);
	}

	else {
		xlnid_watchdog_link_is_down(adapter);
	}

#if 0 /* CONFIG_PCI_IOV */
	xlnid_spoof_check(adapter);
	xlnid_check_for_bad_vf(adapter);
#endif /* CONFIG_PCI_IOV */
	xlnid_update_stats(adapter);

	xlnid_watchdog_flush_tx(adapter);
}

/**
	xlnid_service_timer - Timer Call-back
	@t: pointer to timer_list
**/
static void xlnid_service_timer(struct timer_list *t)
{
	struct xlnid_adapter *adapter = from_timer(adapter, t, service_timer);
	unsigned long next_event_offset;

	/* poll faster when waiting for link */
	if (adapter->flags & XLNID_FLAG_NEED_LINK_UPDATE) {
		next_event_offset = HZ / 10;
	}

	else {
		next_event_offset = HZ * 2;
	}

	/* Reset the timer */
	mod_timer(&adapter->service_timer, next_event_offset + jiffies);

	xlnid_service_event_schedule(adapter);
}

static void xlnid_phy_interrupt_subtask(struct xlnid_adapter *adapter)
{
	u32 status;

	if (!(adapter->flags2 & XLNID_FLAG2_PHY_INTERRUPT)) {
		return;
	}

	adapter->flags2 &= ~XLNID_FLAG2_PHY_INTERRUPT;
	status = xlnid_handle_lasi(&adapter->hw);

	if (status != XLNID_ERR_OVERTEMP) {
		return;
	}

	e_crit(drv, "%s\n", xlnid_overheat_msg);
}

static void xlnid_reset_subtask(struct xlnid_adapter *adapter)
{
#if 1

	if (!test_and_clear_bit(__XLNID_RESET_REQUESTED, &adapter->state)) {
		return;
	}

	rtnl_lock();

	/* If we're already down or resetting, just bail */
	if (test_bit(__XLNID_DOWN, &adapter->state) ||
			test_bit(__XLNID_REMOVING, &adapter->state) ||
			test_bit(__XLNID_RESETTING, &adapter->state)) {
		rtnl_unlock();
		return;
	}

	netdev_err(adapter->netdev, "Xel reset adapter\n");
	adapter->tx_timeout_count++;

	xlnid_reinit_locked(adapter);
	rtnl_unlock();
#endif
}

/**
	xlnid_service_task - manages and runs subtasks
	@work: pointer to work_struct containing our data
**/
static void xlnid_service_task(struct work_struct *work)
{
	struct xlnid_adapter *adapter =
		container_of(work, struct xlnid_adapter, service_task);

	if (XLNID_REMOVED(adapter->hw.hw_addr)) {
		if (!test_bit(__XLNID_DOWN, &adapter->state)) {
			rtnl_lock();
			xlnid_down(adapter);
			rtnl_unlock();
		}

		xlnid_service_event_complete(adapter);
		return;
	}

#if defined(HAVE_UDP_ENC_RX_OFFLOAD) || defined(HAVE_VXLAN_RX_OFFLOAD)
#ifndef HAVE_UDP_TUNNEL_NIC_INFO

	if (adapter->flags2 & XLNID_FLAG2_UDP_TUN_REREG_NEEDED) {
		rtnl_lock();
		adapter->flags2 &= ~XLNID_FLAG2_UDP_TUN_REREG_NEEDED;
#ifdef HAVE_UDP_ENC_RX_OFFLOAD
		udp_tunnel_get_rx_info(adapter->netdev);
#else
		//vxlan_get_rx_port(adapter->netdev);
#endif /* HAVE_UDP_ENC_RX_OFFLOAD */
		rtnl_unlock();
	}

#endif /* HAVE_UDP_TUNNEL_NIC_INFO */
#endif /* HAVE_UDP_ENC_RX_OFFLOAD || HAVE_VXLAN_RX_OFFLOAD */
	xlnid_reset_subtask(adapter);
	xlnid_phy_interrupt_subtask(adapter);
	//xlnid_check_overtemp_subtask(adapter);
	xlnid_watchdog_subtask(adapter);
#ifdef HAVE_TX_MQ
	xlnid_fdir_reinit_subtask(adapter);
#endif
	xlnid_check_hang_subtask(adapter);
#if 0

	if (test_bit(__XLNID_PTP_RUNNING, &adapter->state)) {
		xlnid_ptp_overflow_check(adapter);

		if (unlikely(adapter->flags & XLNID_FLAG_RX_HWTSTAMP_IN_REGISTER)) {
			xlnid_ptp_rx_hang(adapter);
		}

		xlnid_ptp_tx_hang(adapter);
	}

#endif /* HAVE_PTP_1588_CLOCK */

	xlnid_service_event_complete(adapter);
#if 0
#if IS_ENABLED(CONFIG_MACSEC)
	xlnid_macsec_work(adapter);
#endif
#endif
}

static int xlnid_tso(struct xlnid_ring *tx_ring,
						struct xlnid_tx_buffer *first,
						u8 *hdr_len)
{
#ifdef NETIF_F_TSO
	u32 vlan_macip_lens, type_tucmd, mss_l4len_idx;
	struct sk_buff *skb = first->skb;
	union {
		struct iphdr *v4;
		struct ipv6hdr *v6;
		unsigned char *hdr;
	} ip;
	union {
		struct tcphdr *tcp;
		unsigned char *hdr;
	} l4;
	u32 paylen, l4_offset;
	u16 gso_size;
	int err;

	if (skb->ip_summed != CHECKSUM_PARTIAL) {
		return 0;
	}

	if (!skb_is_gso(skb)) {
		return 0;
	}

	err = skb_cow_head(skb, 0);

	if (err < 0) {
		return err;
	}

	ip.hdr = skb_network_header(skb);
	l4.hdr = skb_checksum_start(skb);

	/* ADV DTYP TUCMD MKRLOC/ISCSIHEDLEN */
	type_tucmd = XLNID_ADVTXD_TUCMD_L4T_TCP;

	/* initialize outer IP header fields */
	if (ip.v4->version == 4) {
		unsigned char *csum_start = skb_checksum_start(skb);
		unsigned char *trans_start = ip.hdr + (ip.v4->ihl * 4);

		/*  IP header will have to cancel out any data that
			is not a part of the outer IP header
		*/
		ip.v4->check = csum_fold(
							csum_partial(trans_start, csum_start - trans_start, 0));
		type_tucmd |= XLNID_ADVTXD_TUCMD_IPV4;

		//ip.v4->tot_len = 0;   // WESTLAKE-423
		first->tx_flags |= XLNID_TX_FLAGS_TSO | XLNID_TX_FLAGS_CSUM |
							XLNID_TX_FLAGS_IPV4;
	}

	else {
		ip.v6->payload_len = 0;
		first->tx_flags |= XLNID_TX_FLAGS_TSO | XLNID_TX_FLAGS_CSUM;
	}

	/* determine offset of inner transport header */
	l4_offset = l4.hdr - skb->data;

	/* compute length of segmentation header */
	*hdr_len = (l4.tcp->doff * 4) + l4_offset;

	/* remove payload length from inner checksum */
	paylen = skb->len - l4_offset;
	csum_replace_by_diff(&l4.tcp->check, htonl(paylen));

	/* update gso size and bytecount with header size */
	first->gso_segs = skb_shinfo(skb)->gso_segs;
	first->bytecount += (first->gso_segs - 1) * *hdr_len;

	gso_size = skb_shinfo(skb)->gso_size;

	if (gso_size < 64) {
		gso_size = 64;
	}

	/* mss_l4len_id: use 0 as index for TSO */
	mss_l4len_idx = (*hdr_len - l4_offset) << XLNID_ADVTXD_L4LEN_SHIFT;
	mss_l4len_idx |= gso_size << XLNID_ADVTXD_MSS_SHIFT;

	/* vlan_macip_lens: HEADLEN, MACLEN, VLAN tag */
	vlan_macip_lens = l4.hdr - ip.hdr;
	vlan_macip_lens |= (ip.hdr - skb->data) << XLNID_ADVTXD_MACLEN_SHIFT;
	vlan_macip_lens |= first->tx_flags & XLNID_TX_FLAGS_VLAN_MASK;

	xlnid_tx_ctxtdesc(tx_ring, vlan_macip_lens, 0, type_tucmd,
						mss_l4len_idx);

	return 1;
#else
	return 0;
#endif /* NETIF_F_TSO */
}

static inline bool xlnid_ipv6_csum_is_sctp(struct sk_buff *skb)
{
	unsigned int offset = 0;

	ipv6_find_hdr(skb, &offset, IPPROTO_SCTP, NULL, NULL);

	return offset == skb_checksum_start_offset(skb);
}

static void xlnid_tx_csum(struct xlnid_ring *tx_ring,
							struct xlnid_tx_buffer *first)
{
	struct sk_buff *skb = first->skb;
	u32 vlan_macip_lens = 0;
	u32 type_tucmd = 0;

	if (skb->ip_summed != CHECKSUM_PARTIAL) {
csum_failed:

		if (!(first->tx_flags &
				(XLNID_TX_FLAGS_HW_VLAN | XLNID_TX_FLAGS_CC))) {
			return;
		}

		goto no_csum;
	}

	switch (skb->csum_offset) {
	case offsetof(struct tcphdr, check):
		type_tucmd = XLNID_ADVTXD_TUCMD_L4T_TCP;

		fallthrough;
	case offsetof(struct udphdr, check):
		break;
#if 0

	case offsetof(struct sctphdr, checksum):

		/* validate that this is actually an SCTP request */
		if (((first->protocol == htons(ETH_P_IP)) &&
		(ip_hdr(skb)->protocol == IPPROTO_SCTP)) ||
		((first->protocol == htons(ETH_P_IPV6)) &&
		xlnid_ipv6_csum_is_sctp(skb))) {
		type_tucmd = XLNID_ADVTXD_TUCMD_L4T_SCTP;
		break;
			}

			fallthrough;
#endif

	default:
		skb_checksum_help(skb);
		goto csum_failed;
	}

	/* update TX checksum flag */
	first->tx_flags |= XLNID_TX_FLAGS_CSUM;
	vlan_macip_lens =
		skb_checksum_start_offset(skb) - skb_network_offset(skb);

	if (first->protocol == htons(ETH_P_IP)) {
		type_tucmd |= XLNID_ADVTXD_TUCMD_IPV4;
	}

no_csum:
	/* vlan_macip_lens: MACLEN, VLAN tag */
	vlan_macip_lens |= skb_network_offset(skb) << XLNID_ADVTXD_MACLEN_SHIFT;
	vlan_macip_lens |= first->tx_flags & XLNID_TX_FLAGS_VLAN_MASK;

	xlnid_tx_ctxtdesc(tx_ring, vlan_macip_lens, 0, type_tucmd, 0);
}

#define XLNID_SET_FLAG(_input, _flag, _result)                              \
	((_flag <= _result) ? ((u32)(_input & _flag) * (_result / _flag)) : \
					((u32)(_input & _flag) / (_flag / _result)))

static u32 xlnid_tx_cmd_type(u32 tx_flags)
{
	/* set type for advanced descriptor with frame checksum insertion */
	u32 cmd_type = XLNID_ADVTXD_DTYP_DATA | XLNID_ADVTXD_DCMD_DEXT |
					XLNID_ADVTXD_DCMD_IFCS;

	/* set HW vlan bit if vlan is present */
	cmd_type |= XLNID_SET_FLAG(tx_flags, XLNID_TX_FLAGS_HW_VLAN,
								XLNID_ADVTXD_DCMD_VLE);

	/* set segmentation enable bits for TSO/FSO */
	cmd_type |= XLNID_SET_FLAG(tx_flags, XLNID_TX_FLAGS_TSO,
								XLNID_ADVTXD_DCMD_TSE);

	/* set timestamp bit if present */
	cmd_type |= XLNID_SET_FLAG(tx_flags, XLNID_TX_FLAGS_TSTAMP,
								XLNID_ADVTXD_MAC_TSTAMP);

	return cmd_type;
}

static void xlnid_tx_olinfo_status(struct xlnid_hw *hw,
									union xlnid_adv_tx_desc *tx_desc,
								u32 tx_flags, unsigned int paylen)
{
	u32 olinfo_status = paylen << XLNID_ADVTXD_PAYLEN_SHIFT;

	/* enable L4 checksum for TSO and TX checksum offload */
	olinfo_status |= XLNID_SET_FLAG(tx_flags, XLNID_TX_FLAGS_CSUM,
									XLNID_ADVTXD_POPTS_TXSM);

	/* enble IPv4 checksum for TSO */
	olinfo_status |= XLNID_SET_FLAG(tx_flags, XLNID_TX_FLAGS_IPV4,
									XLNID_ADVTXD_POPTS_IXSM);

	/*
		Check Context must be set if Tx switch is enabled, which it
		always is for case where virtual functions are running
	*/
	olinfo_status |=
		XLNID_SET_FLAG(tx_flags, XLNID_TX_FLAGS_CC, XLNID_ADVTXD_CC);

	if (hw->macsec_enable) {
		/* enable macsec */
		olinfo_status |= XLNID_ADVTXD_POPTS_MACSEC;
	}

	tx_desc->read.olinfo_status = cpu_to_le32(olinfo_status);
}

static int __xlnid_maybe_stop_tx(struct xlnid_ring *tx_ring, u16 size)
{
	netif_stop_subqueue(tx_ring->netdev, tx_ring->queue_index);

	/*  Herbert's original patch had:
		smp_mb__after_netif_stop_queue();
		but since that doesn't exist yet, just open code it.
	*/
	smp_mb();

	/*  We need to check again in a case another CPU has just
		made room available.
	*/
	if (likely(xlnid_desc_unused(tx_ring) < size)) {
		return -EBUSY;
	}

	/* A reprieve! - use start_queue because it doesn't call schedule */
	netif_start_subqueue(tx_ring->netdev, tx_ring->queue_index);
	++tx_ring->tx_stats.restart_queue;
	return 0;
}

static inline int xlnid_maybe_stop_tx(struct xlnid_ring *tx_ring, u16 size)
{
	if (likely(xlnid_desc_unused(tx_ring) >= size)) {
		return 0;
	}

	return __xlnid_maybe_stop_tx(tx_ring, size);
}

static int xlnid_tx_map(struct xlnid_ring *tx_ring,
						struct xlnid_tx_buffer *first, const u8 hdr_len)
{
	struct xlnid_adapter *adapter = tx_ring->q_vector->adapter;
	struct xlnid_hw *hw = &adapter->hw;
	struct sk_buff *skb = first->skb;
	struct xlnid_tx_buffer *tx_buffer;
	union xlnid_adv_tx_desc *tx_desc;
	skb_frag_t *frag;
	dma_addr_t dma;
	unsigned int data_len, size;
	u32 tx_flags = first->tx_flags;
	u32 cmd_type = xlnid_tx_cmd_type(tx_flags);
	u16 i = tx_ring->next_to_use;
	u32 olinfo_status = 0;

	tx_desc = XLNID_TX_DESC(tx_ring, i);

	xlnid_tx_olinfo_status(hw, tx_desc, tx_flags, skb->len - hdr_len);
	olinfo_status = le32_to_cpu(tx_desc->read.olinfo_status);

	size = skb_headlen(skb);
	data_len = skb->data_len;

	dma = dma_map_single(tx_ring->dev, skb->data, size, DMA_TO_DEVICE);

	tx_buffer = first;

	for (frag = &skb_shinfo(skb)->frags[0];; frag++) {
		if (dma_mapping_error(tx_ring->dev, dma)) {
			goto dma_error;
		}

		/* record length, and DMA address */
		dma_unmap_len_set(tx_buffer, len, size);
		dma_unmap_addr_set(tx_buffer, dma, dma);

		tx_desc->read.buffer_addr = cpu_to_le64(dma);

		while (unlikely(size > XLNID_MAX_DATA_PER_TXD)) {
			tx_desc->read.cmd_type_len =
				cpu_to_le32(cmd_type ^ XLNID_MAX_DATA_PER_TXD);

			i++;
			tx_desc++;

			if (i == tx_ring->count) {
				tx_desc = XLNID_TX_DESC(tx_ring, 0);
				i = 0;
			}

			tx_desc->read.olinfo_status =
				cpu_to_le32(olinfo_status);

			dma += XLNID_MAX_DATA_PER_TXD;
			size -= XLNID_MAX_DATA_PER_TXD;

			tx_desc->read.buffer_addr = cpu_to_le64(dma);
		}

		if (likely(!data_len)) {
			break;
		}

		tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type ^ size);

		i++;
		tx_desc++;

		if (i == tx_ring->count) {
			tx_desc = XLNID_TX_DESC(tx_ring, 0);
			i = 0;
		}

		tx_desc->read.olinfo_status = cpu_to_le32(olinfo_status);

		size = skb_frag_size(frag);
		data_len -= size;

		dma = skb_frag_dma_map(tx_ring->dev, frag, 0, size,
								DMA_TO_DEVICE);

		tx_buffer = &tx_ring->tx_buffer_info[i];
	}

	/* write last descriptor with RS and EOP bits */
	cmd_type |= size | XLNID_TXD_CMD;
	tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type);

	netdev_tx_sent_queue(txring_txq(tx_ring), first->bytecount);

	/* set the timestamp */
	first->time_stamp = jiffies;

#ifndef HAVE_TRANS_START_IN_QUEUE
	netdev_ring(tx_ring)->trans_start = first->time_stamp;
#endif
	/*
		Force memory writes to complete before letting h/w know there
		are new descriptors to fetch.  (Only applicable for weak-ordered
		memory model archs, such as IA-64).

		We also need this memory barrier to make certain all of the
		status bits have been updated before next_to_watch is written.
	*/
	wmb();

	/* set next_to_watch value indicating a packet is present */
	first->next_to_watch = tx_desc;
	i++;

	if (i == tx_ring->count) {
		i = 0;
	}

	tx_ring->next_to_use = i;

	xlnid_maybe_stop_tx(tx_ring, DESC_NEEDED);

	skb_tx_timestamp(skb);

	if (netif_xmit_stopped(txring_txq(tx_ring)) || !netdev_xmit_more()) {
		writel(i, tx_ring->tail);
#ifndef SPIN_UNLOCK_IMPLIES_MMIOWB

		/*  The following mmiowb() is required on certain
			architechtures (IA64/Altix in particular) in order to
			synchronize the I/O calls with respect to a spin lock. This
			is because the wmb() on those architectures does not
			guarantee anything for posted I/O writes.

			Note that the associated spin_unlock() is not within the
			driver code, but in the networking core stack.
		*/
		mmiowb();
#endif /* SPIN_UNLOCK_IMPLIES_MMIOWB */
	}

	return 0;
dma_error:
	dev_err(tx_ring->dev, "TX DMA map failed\n");

	/* clear dma mappings for failed tx_buffer_info map */
	for (;;) {
		tx_buffer = &tx_ring->tx_buffer_info[i];

		if (dma_unmap_len(tx_buffer, len))
			dma_unmap_page(tx_ring->dev,
							dma_unmap_addr(tx_buffer, dma),
							dma_unmap_len(tx_buffer, len),
							DMA_TO_DEVICE);

		dma_unmap_len_set(tx_buffer, len, 0);

		if (tx_buffer == first) {
			break;
		}

		if (i == 0) {
			i += tx_ring->count;
		}

		i--;
	}

	dev_kfree_skb_any(first->skb);
	first->skb = NULL;

	tx_ring->next_to_use = i;

	return -1;
}

static void xlnid_atr(struct xlnid_ring *ring,
					struct xlnid_tx_buffer *first)
{
	struct xlnid_q_vector *q_vector = ring->q_vector;
	union xlnid_atr_hash_dword input = { .dword = 0 };
	union xlnid_atr_hash_dword common = { .dword = 0 };
	union {
		unsigned char *network;
		struct iphdr *ipv4;
		struct ipv6hdr *ipv6;
	} hdr;
	struct tcphdr *th;
	struct sk_buff *skb;
	unsigned int hlen;
	__be16 vlan_id;
	int l4_proto;

	/* if ring doesn't have a interrupt vector, cannot perform ATR */
	if (!q_vector) {
		return;
	}

	/* do nothing if sampling is disabled */
	if (!ring->atr_sample_rate) {
		return;
	}

	ring->atr_count++;

	/* currently only IPv4/IPv6 with TCP is supported */
	if (first->protocol != htons(ETH_P_IP) &&
			first->protocol != htons(ETH_P_IPV6)) {
		return;
	}

	/* snag network header to get L4 type and address */
	skb = first->skb;
	hdr.network = skb_network_header(skb);

#if defined(HAVE_UDP_ENC_RX_OFFLOAD) || defined(HAVE_VXLAN_RX_OFFLOAD)

	if (unlikely(hdr.network <= skb->data)) {
		return;
	}

	if (skb->encapsulation && first->protocol == htons(ETH_P_IP) &&
			hdr.ipv4->protocol == IPPROTO_UDP) {
		struct xlnid_adapter *adapter = q_vector->adapter;

#ifndef VXLAN_HEADROOM

		if (unlikely(skb_tail_pointer(skb) <
						hdr.network + vxlan_headroom(0))) {
			return;
		}

#else

		if (unlikely(skb_tail_pointer(skb) <
						hdr.network + VXLAN_HEADROOM)) {
			return;
		}

#endif /*VXLAN_HEADROOM*/

		/* verify the port is recognized as VXLAN or GENEVE*/
		if (adapter->vxlan_port &&
				udp_hdr(skb)->dest == adapter->vxlan_port) {
			hdr.network = skb_inner_network_header(skb);
		}

#ifdef HAVE_UDP_ENC_RX_OFFLOAD

		if (adapter->geneve_port &&
				udp_hdr(skb)->dest == adapter->geneve_port) {
			hdr.network = skb_inner_network_header(skb);
		}

#endif
	}

#endif /* HAVE_UDP_ENC_RX_OFFLOAD || HAVE_VXLAN_RX_OFFLOAD */

	/*  Make sure we have at least [minimum IPv4 header + TCP]
		or [IPv6 header] bytes
	*/
	if (unlikely(skb_tail_pointer(skb) < hdr.network + 40)) {
		return;
	}

	/* Currently only IPv4/IPv6 with TCP is supported */
	switch (hdr.ipv4->version) {
	case IPVERSION:
		/* access ihl as u8 to avoid unaligned access on ia64 */
		hlen = (hdr.network[0] & 0x0F) << 2;
		l4_proto = hdr.ipv4->protocol;
		break;

	case 6:
		hlen = hdr.network - skb->data;
		l4_proto = ipv6_find_hdr(skb, &hlen, IPPROTO_TCP, NULL, NULL);
		hlen -= hdr.network - skb->data;
		break;

	default:
		return;
	}

	if (l4_proto != IPPROTO_TCP) {
		return;
	}

	if (unlikely(skb_tail_pointer(skb) <
					hdr.network + hlen + sizeof(struct tcphdr))) {
		return;
	}

	th = (struct tcphdr *)(hdr.network + hlen);

	/* skip this packet since the socket is closing */
	if (th->fin) {
		return;
	}

	/* sample on all syn packets or once every atr sample count */
	if (!th->syn && (ring->atr_count < ring->atr_sample_rate)) {
		return;
	}

	/* reset sample count */
	ring->atr_count = 0;

	vlan_id = htons(first->tx_flags >> XLNID_TX_FLAGS_VLAN_SHIFT);

	/*
		src and dst are inverted, think how the receiver sees them

		The input is broken into two sections, a non-compressed section
		containing vm_pool, vlan_id, and flow_type.  The rest of the data
		is XORed together and stored in the compressed dword.
	*/
	input.formatted.vlan_id = vlan_id;

	/*
		since src port and flex bytes occupy the same word XOR them together
		and write the value to source port portion of compressed dword
	*/
	if (first->tx_flags & (XLNID_TX_FLAGS_SW_VLAN | XLNID_TX_FLAGS_HW_VLAN)) {
		common.port.src ^= th->dest ^ htons(ETH_P_8021Q);
	}

	else {
		common.port.src ^= th->dest ^ first->protocol;
	}

	common.port.dst ^= th->source;

	switch (hdr.ipv4->version) {
	case IPVERSION:
		input.formatted.flow_type = XLNID_ATR_FLOW_TYPE_TCPV4;
		common.ip ^= hdr.ipv4->saddr ^ hdr.ipv4->daddr;
		break;

	case 6:
		input.formatted.flow_type = XLNID_ATR_FLOW_TYPE_TCPV6;
		common.ip ^= hdr.ipv6->saddr.s6_addr32[0] ^
		hdr.ipv6->saddr.s6_addr32[1] ^
		hdr.ipv6->saddr.s6_addr32[2] ^
		hdr.ipv6->saddr.s6_addr32[3] ^
		hdr.ipv6->daddr.s6_addr32[0] ^
		hdr.ipv6->daddr.s6_addr32[1] ^
		hdr.ipv6->daddr.s6_addr32[2] ^
		hdr.ipv6->daddr.s6_addr32[3];
		break;

	default:
		break;
	}

#if defined(HAVE_UDP_ENC_RX_OFFLOAD) || defined(HAVE_VXLAN_RX_OFFLOAD)

	if (hdr.network != skb_network_header(skb)) {
		input.formatted.flow_type |= XLNID_ATR_L4TYPE_TUNNEL_MASK;
	}

#endif /* HAVE_UDP_ENC_RX_OFFLOAD || HAVE_VXLAN_RX_OFFLOAD */

	/* This assumes the Rx queue and Tx queue are bound to the same CPU */
	xlnid_fdir_add_signature_filter(&q_vector->adapter->hw, input, common,
									ring->queue_index);
}

#ifdef HAVE_XDP_SUPPORT
#ifdef HAVE_XDP_FRAME_STRUCT
	int xlnid_xmit_xdp_ring(struct xlnid_ring *ring, struct xdp_frame *xdpf)
#else
	int xlnid_xmit_xdp_ring(struct xlnid_ring *ring, struct xdp_buff *xdp)
#endif
	{
		struct xlnid_tx_buffer *tx_buffer;
		union xlnid_adv_tx_desc *tx_desc;
		u32 len, cmd_type;
		dma_addr_t dma;
		u16 i;

#ifdef HAVE_XDP_FRAME_STRUCT
		len = xdpf->len;
#else
		len = xdp->data_end - xdp->data;
#endif

		if (unlikely(!xlnid_desc_unused(ring))) {
			return XLNID_XDP_CONSUMED;
		}

#ifdef HAVE_XDP_FRAME_STRUCT
		dma = dma_map_single(ring->dev, xdpf->data, len, DMA_TO_DEVICE);
#else
		dma = dma_map_single(ring->dev, xdp->data, len, DMA_TO_DEVICE);
#endif

		if (dma_mapping_error(ring->dev, dma)) {
			return XLNID_XDP_CONSUMED;
		}

		/* record the location of the first descriptor for this packet */
		tx_buffer = &ring->tx_buffer_info[ring->next_to_use];
		tx_buffer->bytecount = len;
		tx_buffer->gso_segs = 1;
		tx_buffer->protocol = 0;

		i = ring->next_to_use;
		tx_desc = XLNID_TX_DESC(ring, i);

		dma_unmap_len_set(tx_buffer, len, len);
		dma_unmap_addr_set(tx_buffer, dma, dma);
#ifdef HAVE_XDP_FRAME_STRUCT
		tx_buffer->xdpf = xdpf;
#else
		tx_buffer->data = xdp->data;
#endif

		tx_desc->read.buffer_addr = cpu_to_le64(dma);

		/* put descriptor type bits */
		cmd_type = XLNID_ADVTXD_DTYP_DATA | XLNID_ADVTXD_DCMD_DEXT |
					XLNID_ADVTXD_DCMD_IFCS;
		cmd_type |= len | XLNID_TXD_CMD;
		tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type);
		tx_desc->read.olinfo_status =
			cpu_to_le32(len << XLNID_ADVTXD_PAYLEN_SHIFT);

		/* Avoid any potential race with xdp_xmit and cleanup */
		smp_wmb();

#ifdef HAVE_XDP_SUPPORT
#ifdef HAVE_AF_XDP_ZC_SUPPORT
		ring->xdp_tx_active++;
#endif /* HAVE_AF_XDP_ZC_SUPPORT */
#endif /* HAVE_XDP_SUPPORT */

		/* set next_to_watch value indicating a packet is present */
		i++;

		if (i == ring->count) {
			i = 0;
		}

		tx_buffer->next_to_watch = tx_desc;
		ring->next_to_use = i;

		return XLNID_XDP_TX;
	}

#ifdef HAVE_AF_XDP_ZC_SUPPORT
static void xlnid_disable_txr_hw(struct xlnid_adapter *adapter,
									struct xlnid_ring *tx_ring)
{
	unsigned long wait_delay, delay_interval;
	struct xlnid_hw *hw = &adapter->hw;
	u8 reg_idx = tx_ring->reg_idx;
	int wait_loop;
	u32 txdctl;

	XLNID_WRITE_REG_DIRECT(hw, TXDCTL(reg_idx), XLNID_TXDCTL_SWFLSH);
	xlnid_tx_desc_queue_disable(adapter, tx_ring);

	/* delay mechanism from xlnid_disable_tx */
	delay_interval = xlnid_get_completion_timeout(adapter) / 100;

	wait_loop = XLNID_MAX_RX_DESC_POLL;
	wait_delay = delay_interval;

	while (wait_loop--) {
		usleep_range(wait_delay, wait_delay + 10);
		wait_delay += delay_interval * 2;
		txdctl = XLNID_READ_REG_DIRECT(hw, TXDCTL(reg_idx));

		if (!(txdctl & XLNID_TXDCTL_ENABLE)) {
			return;
		}
	}

	e_err(drv, "TXDCTL.ENABLE not cleared within the polling period\n");
}

static void xlnid_disable_txr(struct xlnid_adapter *adapter,
								struct xlnid_ring *tx_ring)
{
	set_bit(__XLNID_TX_DISABLED, &tx_ring->state);
	xlnid_disable_txr_hw(adapter, tx_ring);
}

static void xlnid_disable_rxr_hw(struct xlnid_adapter *adapter,
									struct xlnid_ring *rx_ring)
{
	unsigned long wait_delay, delay_interval;
	struct xlnid_hw *hw = &adapter->hw;
	u8 reg_idx = rx_ring->reg_idx;
	int wait_loop;
	u32 rxdctl;

	rxdctl = XLNID_READ_REG_DIRECT(hw, RXDCTL(reg_idx));
	rxdctl &= ~XLNID_RXDCTL_ENABLE;
	rxdctl |= XLNID_RXDCTL_SWFLSH;

	/* write value back with RXDCTL.ENABLE bit cleared */
	XLNID_WRITE_REG_DIRECT(hw, RXDCTL(reg_idx), rxdctl);
	xlnid_rx_desc_queue_disable(adapter, rx_ring);

	/* delay mechanism from xlnid_disable_rx */
	delay_interval = xlnid_get_completion_timeout(adapter) / 100;

	wait_loop = XLNID_MAX_RX_DESC_POLL;
	wait_delay = delay_interval;

	while (wait_loop--) {
		usleep_range(wait_delay, wait_delay + 10);
		wait_delay += delay_interval * 2;
		rxdctl = XLNID_READ_REG_DIRECT(hw, RXDCTL(reg_idx));

		if (!(rxdctl & XLNID_RXDCTL_ENABLE)) {
			return;
		}
	}

	e_err(drv, "RXDCTL.ENABLE not cleared within the polling period\n");
}

static void xlnid_reset_txr_stats(struct xlnid_ring *tx_ring)
{
	memset(&tx_ring->stats, 0, sizeof(tx_ring->stats));
	memset(&tx_ring->tx_stats, 0, sizeof(tx_ring->tx_stats));
}

static void xlnid_reset_rxr_stats(struct xlnid_ring *rx_ring)
{
	memset(&rx_ring->stats, 0, sizeof(rx_ring->stats));
	memset(&rx_ring->rx_stats, 0, sizeof(rx_ring->rx_stats));
}

/**
	xlnid_txrx_ring_disable - Disable Rx/Tx/XDP Tx rings
	@adapter: adapter structure
	@ring: ring index

	This function disables a certain Rx/Tx/XDP Tx ring. The function
	assumes that the netdev is running.
**/
void xlnid_txrx_ring_disable(struct xlnid_adapter *adapter, int ring)
{
	struct xlnid_ring *rx_ring, *tx_ring, *xdp_ring;

	rx_ring = adapter->rx_ring[ring];
	tx_ring = adapter->tx_ring[ring];
	xdp_ring = adapter->xdp_ring[ring];

	xlnid_disable_txr(adapter, tx_ring);

	if (xdp_ring) {
		xlnid_disable_txr(adapter, xdp_ring);
	}

	xlnid_disable_rxr_hw(adapter, rx_ring);

	if (xdp_ring) {
		synchronize_rcu();
	}

	/* Rx/Tx/XDP Tx share the same napi context. */
	napi_disable(&rx_ring->q_vector->napi);

	xlnid_clean_tx_ring(tx_ring);

	if (xdp_ring) {
		xlnid_clean_tx_ring(xdp_ring);
	}

	xlnid_clean_rx_ring(rx_ring);

	xlnid_reset_txr_stats(tx_ring);

	if (xdp_ring) {
		xlnid_reset_txr_stats(xdp_ring);
	}

	xlnid_reset_rxr_stats(rx_ring);
}

/**
	xlnid_txrx_ring_enable - Enable Rx/Tx/XDP Tx rings
	@adapter: adapter structure
	@ring: ring index

	This function enables a certain Rx/Tx/XDP Tx ring. The function
	assumes that the netdev is running.
**/
void xlnid_txrx_ring_enable(struct xlnid_adapter *adapter, int ring)
{
	struct xlnid_ring *rx_ring, *tx_ring, *xdp_ring;

	rx_ring = adapter->rx_ring[ring];
	tx_ring = adapter->tx_ring[ring];
	xdp_ring = adapter->xdp_ring[ring];

	/* Rx/Tx/XDP Tx share the same napi context. */
	napi_enable(&rx_ring->q_vector->napi);

	xlnid_configure_tx_ring(adapter, tx_ring);

	if (xdp_ring) {
		xlnid_configure_tx_ring(adapter, xdp_ring);
	}

	xlnid_configure_rx_ring(adapter, rx_ring);

	clear_bit(__XLNID_TX_DISABLED, &tx_ring->state);

	if (xdp_ring) {
		clear_bit(__XLNID_TX_DISABLED, &xdp_ring->state);
	}
}
#endif /* HAVE_AF_XDP_ZC_SUPPORT */
#endif /* HAVE_XDP_SUPPORT */

netdev_tx_t xlnid_xmit_frame_ring(struct sk_buff *skb,
			struct xlnid_adapter __maybe_unused *adapter,
			struct xlnid_ring *tx_ring)
{
	struct xlnid_tx_buffer *first;
	int tso;
	u32 tx_flags = 0;
	unsigned short f;
	u16 count = TXD_USE_COUNT(skb_headlen(skb));
	__be16 protocol = skb->protocol;
	u8 hdr_len = 0;

	/*
		need: 1 descriptor per page * PAGE_SIZE/XLNID_MAX_DATA_PER_TXD,
				+ 1 desc for skb_headlen/XLNID_MAX_DATA_PER_TXD,
				+ 2 desc gap to keep tail from touching head,
				+ 1 desc for context descriptor,
		otherwise try next time
	*/
	for (f = 0; f < skb_shinfo(skb)->nr_frags; f++)
		count += TXD_USE_COUNT(
						skb_frag_size(&skb_shinfo(skb)->frags[f]));

	if (xlnid_maybe_stop_tx(tx_ring, count + 3)) {
		tx_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}

	/* record the location of the first descriptor for this packet */
	first = &tx_ring->tx_buffer_info[tx_ring->next_to_use];
	first->skb = skb;
	first->bytecount = skb->len;
	first->gso_segs = 1;

	/* if we have a HW VLAN tag being added default to the HW one */
	if (skb_vlan_tag_present(skb)) {
		tx_flags |= skb_vlan_tag_get(skb) << XLNID_TX_FLAGS_VLAN_SHIFT;
		tx_flags |= XLNID_TX_FLAGS_HW_VLAN;
		/* else if it is a SW VLAN check the next protocol and store the tag */
	}

	else if (protocol == htons(ETH_P_8021Q)) {
		struct vlan_hdr *vhdr, _vhdr;
		vhdr = skb_header_pointer(skb, ETH_HLEN, sizeof(_vhdr), &_vhdr);

		if (!vhdr) {
			goto out_drop;
		}

		protocol = vhdr->h_vlan_encapsulated_proto;
		tx_flags |= ntohs(vhdr->h_vlan_TCI)
					<< XLNID_TX_FLAGS_VLAN_SHIFT;
		tx_flags |= XLNID_TX_FLAGS_SW_VLAN;
	}

#ifdef HAVE_PTP_1588_CLOCK
#ifdef SKB_SHARED_TX_IS_UNION

		if (unlikely(skb_tx(skb)->hardware) && adapter->ptp_clock) {
			if (!test_and_set_bit_lock(__XLNID_PTP_TX_IN_PROGRESS,
									&adapter->state)) {
			skb_tx(skb)->in_progress = 1;
#else

	if (unlikely(skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) &&
			adapter->ptp_clock) {
		if (!test_and_set_bit_lock(__XLNID_PTP_TX_IN_PROGRESS,
									&adapter->state)) {
			skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
#endif
			tx_flags |= XLNID_TX_FLAGS_TSTAMP;

			/* schedule check for Tx timestamp */
			adapter->ptp_tx_skb = skb_get(skb);
			adapter->ptp_tx_start = jiffies;
			schedule_work(&adapter->ptp_tx_work);
		}

		else {
			adapter->tx_hwtstamp_skipped++;
		}
	}

#endif
#if 0

	/*
		Use the l2switch_enable flag - would be false if the DMA
		Tx switch had been disabled.
	*/
	if (adapter->flags & XLNID_FLAG_SRIOV_L2SWITCH_ENABLE) {
		tx_flags |= XLNID_TX_FLAGS_CC;
	}

#endif
#ifdef HAVE_TX_MQ

	if ((adapter->flags & XLNID_FLAG_DCB_ENABLED) &&
			((tx_flags & (XLNID_TX_FLAGS_HW_VLAN | XLNID_TX_FLAGS_SW_VLAN)) ||
				(skb->priority != TC_PRIO_CONTROL))) {
		tx_flags &= ~XLNID_TX_FLAGS_VLAN_PRIO_MASK;
		tx_flags |= skb->priority << XLNID_TX_FLAGS_VLAN_PRIO_SHIFT;

		if (tx_flags & XLNID_TX_FLAGS_SW_VLAN) {
			struct vlan_ethhdr *vhdr;

			if (skb_header_cloned(skb) &&
					pskb_expand_head(skb, 0, 0, GFP_ATOMIC)) {
				goto out_drop;
			}

			vhdr = (struct vlan_ethhdr *)skb->data;
			vhdr->h_vlan_TCI =
				htons(tx_flags >> XLNID_TX_FLAGS_VLAN_SHIFT);
		}

		else {
			tx_flags |= XLNID_TX_FLAGS_HW_VLAN;
		}
	}

#endif /* HAVE_TX_MQ */
	/* record initial flags and protocol */
	first->tx_flags = tx_flags;
	first->protocol = protocol;

	tso = xlnid_tso(tx_ring, first, &hdr_len);

	if (tso < 0) {
		goto out_drop;
	}

	else if (!tso) {
		xlnid_tx_csum(tx_ring, first);
	}

	/* add the ATR filter if ATR is on */
	if (test_bit(__XLNID_TX_FDIR_INIT_DONE, &tx_ring->state)) {
		xlnid_atr(tx_ring, first);
	}

#ifdef HAVE_PTP_1588_CLOCK

	if (xlnid_tx_map(tx_ring, first, hdr_len)) {
		goto cleanup_tx_tstamp;
	}

#else
	xlnid_tx_map(tx_ring, first, hdr_len);
#endif

	return NETDEV_TX_OK;

out_drop:
	dev_kfree_skb_any(first->skb);
	first->skb = NULL;
#ifdef HAVE_PTP_1588_CLOCK
cleanup_tx_tstamp:

	if (unlikely(tx_flags & XLNID_TX_FLAGS_TSTAMP)) {
		dev_kfree_skb_any(adapter->ptp_tx_skb);
		adapter->ptp_tx_skb = NULL;
		cancel_work_sync(&adapter->ptp_tx_work);
		clear_bit_unlock(__XLNID_PTP_TX_IN_PROGRESS, &adapter->state);
	}

#endif

	return NETDEV_TX_OK;
}

static netdev_tx_t __xlnid_xmit_frame(struct sk_buff *skb,
						struct net_device *netdev,
					__always_unused struct xlnid_ring *ring)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_ring *tx_ring;

#ifdef HAVE_TX_MQ
	unsigned int r_idx = skb->queue_mapping;
#endif

	if (!netif_carrier_ok(netdev)) {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	/*
		The minimum packet size for olinfo paylen is 17 so pad the skb
		in order to meet this minimum size requirement.
	*/
	if (skb_put_padto(skb, 17)) {
		return NETDEV_TX_OK;
	}

#ifdef HAVE_TX_MQ

	if (r_idx >= adapter->num_tx_queues) {
		r_idx = r_idx % adapter->num_tx_queues;
	}

	tx_ring = adapter->tx_ring[r_idx];
#else
	tx_ring = adapter->tx_ring[0];
#endif

#ifdef HAVE_AF_XDP_ZC_SUPPORT

	if (unlikely(test_bit(__XLNID_TX_DISABLED, &tx_ring->state))) {
		return NETDEV_TX_BUSY;
	}

#endif

	return xlnid_xmit_frame_ring(skb, adapter, tx_ring);
}

static netdev_tx_t xlnid_xmit_frame(struct sk_buff *skb,
									struct net_device *netdev)
{
	return __xlnid_xmit_frame(skb, netdev, NULL);
}

/**
	xlnid_set_mac - Change the Ethernet Address of the NIC
	@netdev: network interface device structure
	@p: pointer to an address structure

	Returns 0 on success, negative on failure
**/
static int xlnid_set_mac(struct net_device *netdev, void *p)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	struct sockaddr *addr = p;
	u32 regval_RAL = 0;
	u32 regval_RAH = 0;

	if (!is_valid_ether_addr(addr->sa_data)) {
		return -EADDRNOTAVAIL;
	}

	eth_hw_addr_set(netdev, addr->sa_data);
	memcpy(hw->mac.addr, addr->sa_data, netdev->addr_len);

	xlnid_mac_set_default_filter(adapter);

	if (hw->mac.type == xlnid_mac_WESTLAKE && hw->ptsw_enable &&
			adapter->pdev->devfn == 0) {
		xel_pci_info_t *slave_pinfo =
			xlnid_find_pci_info(hw->bus.bus_id, 1);

		regval_RAL = XLNID_READ_REG_MAC(hw, WESTLAKE_RAL(0));
		regval_RAH = XLNID_READ_REG_MAC(hw, WESTLAKE_RAH(0));

		XLNID_WRITE_REG_MAC(slave_pinfo->hw, WESTLAKE_RAL(0),
							regval_RAL);
		XLNID_WRITE_REG_MAC(slave_pinfo->hw, WESTLAKE_RAH(0),
							regval_RAH);
	}

	return 0;
}

static int xlnid_mdio_addr_get(struct net_device *netdev,
								struct mii_ioctl_data *mii)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;

	if (!hw->phy.ops.read_reg) {
		return -EOPNOTSUPP;
	}

	mii->phy_id = (hw->phy.addr << 5) & MDIO_PHY_ID_PRTAD;
	return 0;
}

static int xlnid_mdio_read(struct net_device *netdev,
					int prtad, int devad, u16 addr)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	u16 value;
	int rc;

	if (prtad != hw->phy.addr) {
		return -EINVAL;
	}

	if (!hw->phy.ops.read_reg) {
		return -EOPNOTSUPP;
	}

	rc = hw->phy.ops.read_reg(hw, addr, devad, &value);

	if (!rc) {
		rc = value;
	}

	return rc;
}

static int xlnid_mdio_write(struct net_device *netdev, int prtad, int devad,
						u16 addr, u16 value)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;

	if (prtad != hw->phy.addr) {
		return -EINVAL;
	}

	if (!hw->phy.ops.write_reg) {
		return -EOPNOTSUPP;
	}

	return hw->phy.ops.write_reg(hw, addr, devad, value);
}

static int xlnid_mii_ioctl(struct net_device *netdev, struct ifreq *ifr,
						int cmd)
{
	struct mii_ioctl_data *mii = (struct mii_ioctl_data *)&ifr->ifr_data;
	int prtad, devad, ret;

	if (cmd == SIOCGMIIPHY) {
		return xlnid_mdio_addr_get(netdev, mii);
	}

	prtad = (mii->phy_id & MDIO_PHY_ID_PRTAD) >> 5;
	devad = (mii->phy_id & MDIO_PHY_ID_DEVAD);

	if (cmd == SIOCGMIIREG) {
		ret = xlnid_mdio_read(netdev, prtad, devad, mii->reg_num);

		if (ret < 0) {
			return ret;
		}

		mii->val_out = ret;
		return 0;
	}

	else {
		return xlnid_mdio_write(netdev, prtad, devad, mii->reg_num,
								mii->val_in);
	}
}

/*
	Function:
		xlnid_ptsw_on
	Purpose:
		check the status of ptsw and decide
		whether to probe function 1 or not
	Parameters:
		adapter    - (IN) device information
	Returns:
		true    ptsw is enabled and only probe function 0
		false   ptsw is disabled
*/
static void xlnid_ptsw_check(struct xlnid_hw *hw)
{
	u32 regval;
	struct xlnid_adapter *adapter = (struct xlnid_adapter *)hw->back;

	hw->ptsw_enable = false;

	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_PTSW_MODE);

	/* check whether ptsw is enabled */
	if ((regval & XLNID_CRG_RX_PTSW_SOFTEN) == XLNID_CRG_RX_PTSW_SOFTEN) {
		hw->ptsw_enable = true;
		e_dev_info("NIC enter ptsw mode\n");
	}

	return;
}

static int xlnid_port_loopback_config_westlake(struct xlnid_adapter *adapter,
	u32 flags)
{
	struct xlnid_hw *hw = &adapter->hw;

	if (XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL) == 0) {
		u16 value = 0;

		if ((flags & XLNID_PORT_LOOPBACK_OUTER) &&
				(flags & XLNID_PORT_LOOPBACK_SET)) {
			e_info(drv,
					"mac outer loopback enable, media_type=%d\n",
					hw->phy.media_type);
			XLNID_WRITE_REG_MAC(
				hw, WESTLAKE_MACSEC_ETH_LOOPBACK_CFG, 0x1);
		}

		else if ((flags & XLNID_PORT_LOOPBACK_OUTER) &&
					(flags & XLNID_PORT_LOOPBACK_RESET)) {
			e_info(drv,
					"mac outer loopback disable, media_type=%d\n",
					hw->phy.media_type);
			XLNID_WRITE_REG_MAC(
				hw, WESTLAKE_MACSEC_ETH_LOOPBACK_CFG, 0x0);
		}

		else if ((flags & XLNID_PORT_LOOPBACK_INNER) &&
					(flags & XLNID_PORT_LOOPBACK_SET)) {
			e_info(drv,
					"phy inner loopback enable, media_type=%d\n",
					hw->phy.media_type);
			value = (u16)xlnid_mdio_read(adapter->netdev,
											hw->phy.addr, 0, 0x0);
			value |=
				(1
					<< 14); // phy control address 0: bit14 for loopback
			xlnid_mdio_write(adapter->netdev, hw->phy.addr, 0, 0x0,
								value);
		}

		else if ((flags & XLNID_PORT_LOOPBACK_INNER) &&
					(flags & XLNID_PORT_LOOPBACK_RESET)) {
			e_info(drv,
					"phy inner loopback disable, media_type=%d\n",
					hw->phy.media_type);
			value = (u16)xlnid_mdio_read(adapter->netdev,
											hw->phy.addr, 0, 0x0);
			value &= ~(1 << 14);
			value |=
				(1
					<< 12); // phy control address 0: bit12 for auto-neg
			xlnid_mdio_write(adapter->netdev, hw->phy.addr, 0, 0x0,
								value);
		}
	}

	else {
		if ((flags & XLNID_PORT_LOOPBACK_OUTER) &&
				(flags & XLNID_PORT_LOOPBACK_SET)) {
			e_info(drv,
					"serdes outer loopback enable, media_type=%d\n",
					hw->phy.media_type);
			XLNID_WRITE_REG_MAC(hw, 0x100484, 0x07);
			XLNID_WRITE_REG_MAC(hw, 0x100488, 0x10);
			XLNID_WRITE_REG_MAC(hw, 0x100480, 0x01);
			XLNID_WRITE_REG_MAC(hw, 0x100484, 0x55);
			XLNID_WRITE_REG_MAC(hw, 0x100488, 0xfd);
			XLNID_WRITE_REG_MAC(hw, 0x100480, 0x01);
		}

		else if ((flags & XLNID_PORT_LOOPBACK_INNER) &&
					(flags & XLNID_PORT_LOOPBACK_SET)) {
			e_info(drv,
					"serdes inner loopback enable, media_type=%d\n",
					hw->phy.media_type);
			XLNID_WRITE_REG_MAC(hw, 0x100484, 0x07);
			XLNID_WRITE_REG_MAC(hw, 0x100488, 0x20);
			XLNID_WRITE_REG_MAC(hw, 0x100480, 0x01);
			XLNID_WRITE_REG_MAC(hw, 0x100484, 0x55);
			XLNID_WRITE_REG_MAC(hw, 0x100488, 0xfd);
			XLNID_WRITE_REG_MAC(hw, 0x100480, 0x01);
		}

		else if (flags & XLNID_PORT_LOOPBACK_RESET) {
			e_info(drv,
					"serdes inner/outer loopback disable, media_type=%d\n",
					hw->phy.media_type);
			XLNID_WRITE_REG_MAC(hw, 0x100484, 0x7);
			XLNID_WRITE_REG_MAC(hw, 0x100488, 0x0);
			XLNID_WRITE_REG_MAC(hw, 0x100480, 0x1);
			XLNID_WRITE_REG_MAC(hw, 0x100484, 0x55);
			XLNID_WRITE_REG_MAC(hw, 0x100488, 0xff);
			XLNID_WRITE_REG_MAC(hw, 0x100480, 0x1);
		}
	}

	return 0;
}

int xlnid_port_loopback_config(struct xlnid_adapter *adapter, u32 flags)
{
	if (adapter->hw.mac.type == xlnid_mac_WESTLAKE) {
		return xlnid_port_loopback_config_westlake(adapter, flags);
	}

	return -EOPNOTSUPP;
}

/*
	Function:
		xlnid_mode_set_ioctl
	Purpose:
		set mode of two ports
	Parameters:
		adapter    - (IN) device information
		ifr        - (IN) whether enable ptsw
	Returns:
		0    success
*/
static int xlnid_mode_set_ioctl(struct xlnid_adapter *adapter,
							struct ifreq *ifr)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 regval = 0;
	bool mode = false;
	xel_pci_info_t *master_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 0);
	xel_pci_info_t *slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
	struct xlnid_adapter *master_adapter =
		(struct xlnid_adapter *)master_pinfo->hw->back;
	struct xlnid_adapter *slave_adapter =
		(struct xlnid_adapter *)slave_pinfo->hw->back;

	if (master_pinfo == NULL || slave_pinfo == NULL) {
		printk("Device not support mode set\r\n");
		return 0;
	}

	if (ifr->ifr_ifru.ifru_ivalue == 1) {
		/* enable ptsw TX one port */
		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_PTSW_MODE);
		regval |= (XLNID_CRG_RX_PTSW_SOFTEN | XLNID_CRG_TX_PTSW_ENABLE);
		regval &= ~XLNID_CRG_RX_PTSW_PORT;
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_PTSW_MODE, regval);
		//XLNID_WRITE_REG_MAC_DIRECT(hw, WESTLAKE_TX_PTSW,
		//(XLNID_CFG_ETH_LGC_EN | XLNID_CFG_PTSW_ENABLE));
		XLNID_WRITE_REG_MAC_DIRECT(hw, WESTLAKE_TX_PTSW, 0x0);
		e_dev_info(
			"Enable ptsw and TX one port, please reinsert module\n");
		goto ptsw_exit;
	}

	else if (ifr->ifr_ifru.ifru_ivalue == 2) {
		/* enable ptsw TX two ports */
		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_PTSW_MODE);
		regval |= (XLNID_CRG_RX_PTSW_SOFTEN | XLNID_CRG_TX_PTSW_ENABLE);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_PTSW_MODE, regval);
		XLNID_WRITE_REG_MAC_DIRECT(hw, WESTLAKE_TX_PTSW,
									XLNID_CFG_PTSW_ENABLE);
		e_dev_info(
			"Enable ptsw and TX two ports, please reinsert module\n");
		goto ptsw_exit;
	}

	else if (ifr->ifr_ifru.ifru_ivalue == 3) {
		/* tp */
		mode = false;
		master_pinfo->hw->phy.media_type = xlnid_media_type_copper;
		slave_pinfo->hw->phy.media_type = xlnid_media_type_copper;
	}

	else if (ifr->ifr_ifru.ifru_ivalue == 4) {
		/* fibre */
		mode = true;
		master_pinfo->hw->phy.media_type = xlnid_media_type_fiber;
		slave_pinfo->hw->phy.media_type = xlnid_media_type_fiber;
	}

	else if (ifr->ifr_ifru.ifru_ivalue == 10) {
		regval = (XLNID_PORT_LOOPBACK_OUTER | XLNID_PORT_LOOPBACK_SET);
		goto loopback_exit;
	}

	else if (ifr->ifr_ifru.ifru_ivalue == 11) {
		regval =
			(XLNID_PORT_LOOPBACK_OUTER | XLNID_PORT_LOOPBACK_RESET);
		goto loopback_exit;
	}

	else if (ifr->ifr_ifru.ifru_ivalue == 12) {
		regval = (XLNID_PORT_LOOPBACK_INNER | XLNID_PORT_LOOPBACK_SET);
		goto loopback_exit;
	}

	else if (ifr->ifr_ifru.ifru_ivalue == 13) {
		regval =
			(XLNID_PORT_LOOPBACK_INNER | XLNID_PORT_LOOPBACK_RESET);
		goto loopback_exit;
	}

	else {
		/* disable ptsw */
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_PTSW_MODE, 0x0);
		XLNID_WRITE_REG_MAC_DIRECT(hw, WESTLAKE_TX_PTSW, 0x0);
		e_dev_info("Disable ptsw, please reinsert module\n");
		goto ptsw_exit;
	}

	master_pinfo->hw->link_mode = mode;
	slave_pinfo->hw->link_mode = mode;

	master_adapter->flags |= XLNID_FLAG_NEED_LINK_UPDATE;
	slave_adapter->flags |= XLNID_FLAG_NEED_LINK_UPDATE;

loopback_exit:

	if ((ifr->ifr_ifru.ifru_ivalue >= 10) &&
			(ifr->ifr_ifru.ifru_ivalue <= 13)) {
		if (xlnid_port_loopback_config(adapter, regval))
			e_dev_err(
				"Error, loopback config of device %s failed \n",
				ifr->ifr_name);
	}

ptsw_exit:
	return 0;
}

static int xlnid_ioctl(struct net_device *netdev, struct ifreq *ifr,
					int cmd)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	switch (cmd) {
		#ifdef HAVE_PTP_1588_CLOCK
		#ifdef SIOCGHWTSTAMP

	case SIOCGHWTSTAMP:
		return xlnid_ptp_get_ts_config(adapter, ifr);
		#endif

	case SIOCSHWTSTAMP:
		return xlnid_ptp_set_ts_config(adapter, ifr);
		#endif
		#ifdef ETHTOOL_OPS_COMPAT

	case SIOCETHTOOL:
		return ethtool_ioctl(ifr);
		#endif

	case SIOCGMIIPHY:
	case SIOCGMIIREG:
	case SIOCSMIIREG:
		return xlnid_mii_ioctl(netdev, ifr, cmd);

	case SIOCDEVPRIVATE:
		return xlnid_mode_set_ioctl(adapter, ifr);

	default:
		return -EOPNOTSUPP;
	}
}

#ifdef HAVE_NDO_SIOCDEVPRIVATE
static int xlnid_siocdevprivate(struct net_device *netdev, struct ifreq *ifr,
								void __user *data, int cmd)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	switch (cmd) {
	case SIOCDEVPRIVATE:
		return xlnid_mode_set_ioctl(adapter, ifr);

	default:
		return -EOPNOTSUPP;
	}
}
#endif

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
	Polling 'interrupt' - used by things like netconsole to send skbs
	without having to re-enable interrupts. It's not called while
	the interrupt routine is executing.
*/
static void xlnid_netpoll(struct net_device *netdev)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	/* if interface is down do nothing */
	if (test_bit(__XLNID_DOWN, &adapter->state)) {
		return;
	}

	if (adapter->flags & XLNID_FLAG_MSIX_ENABLED) {
		int i;

		for (i = 0; i < adapter->num_q_vectors; i++) {
			adapter->q_vector[i]->netpoll_rx = true;
			xlnid_msix_clean_rings(0, adapter->q_vector[i]);
			adapter->q_vector[i]->netpoll_rx = false;
		}
	}

	else {
		xlnid_intr(0, adapter);
	}
}
#endif /* CONFIG_NET_POLL_CONTROLLER */

/**
	xlnid_setup_tc - routine to configure net_device for multiple traffic
	classes.

	@dev: net device to configure
	@tc: number of traffic classes to enable
*/
int xlnid_setup_tc(struct net_device *dev, u8 tc)
{
	struct xlnid_adapter *adapter = netdev_priv(dev);

	/*  Hardware has to reinitialize queues and interrupts to
		match packet buffer alignment. Unfortunately, the
		hardware is not flexible enough to do this dynamically.
	*/
	if (netif_running(dev)) {
		xlnid_close(dev);
	}

	else {
		xlnid_reset(adapter);
	}

	xlnid_clear_interrupt_scheme(adapter);

	if (tc) {
		return -EINVAL;
	}

	else {
		netdev_reset_tc(dev);

		adapter->flags &= ~XLNID_FLAG_DCB_ENABLED;
	}

	xlnid_init_interrupt_scheme(adapter);

	if (netif_running(dev)) {
		xlnid_open(dev);
	}

	return 0;
}

#if 0
void xlnid_sriov_reinit(struct xlnid_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	rtnl_lock();
	xlnid_setup_tc(netdev, netdev_get_num_tc(netdev));
	rtnl_unlock();
}
#endif

void xlnid_do_reset(struct net_device *netdev)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev)) {
		xlnid_reinit_locked(adapter);
	}

	else {
		xlnid_reset(adapter);
	}
}

#ifdef HAVE_NDO_SET_FEATURES
#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
static u32 xlnid_fix_features(struct net_device *netdev, u32 features)
#else
static netdev_features_t xlnid_fix_features(struct net_device *netdev,
		netdev_features_t features)
#endif
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	/* If Rx checksum is disabled, then RSC/LRO should also be disabled */
	if (!(features & NETIF_F_RXCSUM)) {
		features &= ~NETIF_F_LRO;
	}

	/* Turn off LRO if not RSC capable */
	if (!(adapter->flags2 & XLNID_FLAG2_RSC_CAPABLE)) {
		features &= ~NETIF_F_LRO;
	}

	if (adapter->xdp_prog && (features & NETIF_F_LRO)) {
		e_dev_err("LRO is not supported with XDP\n");
		features &= ~NETIF_F_LRO;
	}

	return features;
}

#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
static int xlnid_set_features(struct net_device *netdev, u32 features)
#else
static int xlnid_set_features(struct net_device *netdev,
								netdev_features_t features)
#endif
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	bool need_reset = false;
	netdev_features_t changed = netdev->features ^ features;

	/* Make sure RSC matches LRO, reset if change */
	if (!(features & NETIF_F_LRO)) {
		if (adapter->flags2 & XLNID_FLAG2_RSC_ENABLED) {
			need_reset = true;
		}

		adapter->flags2 &= ~XLNID_FLAG2_RSC_ENABLED;
	}

	else if ((adapter->flags2 & XLNID_FLAG2_RSC_CAPABLE) &&
				!(adapter->flags2 & XLNID_FLAG2_RSC_ENABLED)) {
		if (adapter->rx_itr_setting == 1 ||
				adapter->rx_itr_setting > XLNID_MIN_RSC_ITR) {
			adapter->flags2 |= XLNID_FLAG2_RSC_ENABLED;
			need_reset = true;
		}

		else if (changed & NETIF_F_LRO) {
			e_info(probe, "rx-usecs set too low, "
					"disabling RSC\n");
		}
	}

	/*
		Check if Flow Director n-tuple support was enabled or disabled.  If
		the state changed, we need to reset.
	*/
#ifdef NETIF_F_HW_TC

	if ((features & NETIF_F_NTUPLE) || (features & NETIF_F_HW_TC)) {
#else

	if (features & NETIF_F_NTUPLE) {
#endif

		/* turn off ATR, enable perfect filters and reset */
		if (!(adapter->flags & XLNID_FLAG_FDIR_PERFECT_CAPABLE)) {
			need_reset = true;
		}

		adapter->flags &= ~XLNID_FLAG_FDIR_HASH_CAPABLE;
		adapter->flags |= XLNID_FLAG_FDIR_PERFECT_CAPABLE;
	}

	else {
		/* turn off perfect filters, enable ATR and reset */
		if (adapter->flags & XLNID_FLAG_FDIR_PERFECT_CAPABLE) {
			need_reset = true;
		}

		adapter->flags &= ~XLNID_FLAG_FDIR_PERFECT_CAPABLE;

		/* We cannot enable ATR if VMDq is enabled */
		if (adapter->flags & XLNID_FLAG_VMDQ_ENABLED ||
				/* We cannot enable ATR if we have 2 or more tcs */
				netdev_get_num_tc(netdev) > 1 ||
				/* We cannot enable ATR if RSS is disabled */
				adapter->ring_feature[RING_F_RSS].limit <= 1 ||
				/* A sample rate of 0 indicates ATR disabled */
				!adapter->atr_sample_rate)
			; /* do nothing not supported */
		else { /* otherwise supported and set the flag */
			adapter->flags |= XLNID_FLAG_FDIR_HASH_CAPABLE;
		}
	}

	netdev->features = features;

#if defined(HAVE_UDP_ENC_RX_OFFLOAD) || defined(HAVE_VXLAN_RX_OFFLOAD)

	if (adapter->flags & XLNID_FLAG_VXLAN_OFFLOAD_CAPABLE &&
			features & NETIF_F_RXCSUM) {
		if (!need_reset) {
			adapter->flags2 |= XLNID_FLAG2_UDP_TUN_REREG_NEEDED;
		}
	}

	else {
		u32 port_mask = XLNID_VXLANCTRL_VXLAN_UDPPORT_MASK;

		xlnid_clear_udp_tunnel_port(adapter, port_mask);
	}

#endif /* HAVE_UDP_ENC_RX_OFFLOAD || HAVE_VXLAN_RX_OFFLOAD */

#ifdef HAVE_UDP_ENC_RX_OFFLOAD

	if (adapter->flags & XLNID_FLAG_GENEVE_OFFLOAD_CAPABLE &&
			features & NETIF_F_RXCSUM) {
		if (!need_reset) {
			adapter->flags2 |= XLNID_FLAG2_UDP_TUN_REREG_NEEDED;
		}
	}

	else {
		u32 port_mask = XLNID_VXLANCTRL_GENEVE_UDPPORT_MASK;

		xlnid_clear_udp_tunnel_port(adapter, port_mask);
	}

#endif /* HAVE_UDP_ENC_RX_OFFLOAD */

	if (need_reset) {
		xlnid_do_reset(netdev);
	}

#ifdef NETIF_F_HW_VLAN_CTAG_FILTER

	else if (changed &
				(NETIF_F_HW_VLAN_CTAG_RX | NETIF_F_HW_VLAN_CTAG_FILTER)) {
		xlnid_set_rx_mode(netdev);
	}

#endif
#ifdef NETIF_F_HW_VLAN_FILTER

	else if (changed & (NETIF_F_HW_VLAN_RX | NETIF_F_HW_VLAN_FILTER)) {
		xlnid_set_rx_mode(netdev);
	}

#endif
	return 0;
}

#endif /* HAVE_NDO_SET_FEATURES */

#ifdef HAVE_NDO_GSO_CHECK
static bool xlnid_gso_check(struct sk_buff *skb,
							__always_unused struct net_device *dev)
{
	return vxlan_gso_check(skb);
}
#endif /* HAVE_NDO_GSO_CHECK */

#ifdef HAVE_FDB_OPS
#ifdef USE_CONST_DEV_UC_CHAR
static int xlnid_ndo_fdb_add(struct ndmsg *ndm, struct nlattr *tb[],
								struct net_device *dev, const unsigned char *addr,
#ifdef HAVE_NDO_FDB_ADD_VID
								u16 vid,
#endif
#ifdef HAVE_NDO_FDB_ADD_EXTACK
								u16 flags,
								struct netlink_ext_ack __always_unused *extack)
#else
								u16 flags)
#endif
#else
static int xlnid_ndo_fdb_add(struct ndmsg *ndm, struct net_device *dev,
								unsigned char *addr, u16 flags)
#endif /* USE_CONST_DEV_UC_CHAR */
{
	/* guarantee we can provide a unique filter for the unicast address */
	if (is_unicast_ether_addr(addr) || is_link_local_ether_addr(addr)) {
		struct xlnid_adapter *adapter = netdev_priv(dev);
		u16 pool = VMDQ_P(0);

		if (netdev_uc_count(dev) >= xlnid_available_rars(adapter, pool)) {
			return -ENOMEM;
		}
	}

#ifdef USE_CONST_DEV_UC_CHAR
#ifdef HAVE_NDO_FDB_ADD_VID
	return ndo_dflt_fdb_add(ndm, tb, dev, addr, vid, flags);
#else
	return ndo_dflt_fdb_add(ndm, tb, dev, addr, flags);
#endif /* HAVE_NDO_FDB_ADD_VID */
#else
	return ndo_dflt_fdb_add(ndm, dev, addr, flags);
#endif /* USE_CONST_DEV_UC_CHAR */
}

#ifdef HAVE_BRIDGE_ATTRIBS
#ifdef HAVE_NDO_BRIDGE_SETLINK_EXTACK
static int xlnid_ndo_bridge_setlink(struct net_device *dev,
									struct nlmsghdr *nlh,
									__always_unused u16 flags,
									struct netlink_ext_ack __always_unused *ext)
#elif defined(HAVE_NDO_BRIDGE_SET_DEL_LINK_FLAGS)
static int xlnid_ndo_bridge_setlink(struct net_device *dev,
									struct nlmsghdr *nlh,
									__always_unused u16 flags)
#else
static int xlnid_ndo_bridge_setlink(struct net_device *dev,
									struct nlmsghdr *nlh)
#endif /* HAVE_NDO_BRIDGE_SETLINK_EXTACK */
{
	struct xlnid_adapter *adapter = netdev_priv(dev);
	struct nlattr *attr, *br_spec;
	int rem;

	if (!(adapter->flags & XLNID_FLAG_SRIOV_ENABLED)) {
		return -EOPNOTSUPP;
	}

	br_spec = nlmsg_find_attr(nlh, sizeof(struct ifinfomsg), IFLA_AF_SPEC);

	nla_for_each_nested(attr, br_spec, rem) {
		__u16 mode;

		if (nla_type(attr) != IFLA_BRIDGE_MODE) {
			continue;
		}

		mode = nla_get_u16(attr);

		if (mode == BRIDGE_MODE_VEPA) {
			adapter->flags |= XLNID_FLAG_SRIOV_VEPA_BRIDGE_MODE;
		}

		else if (mode == BRIDGE_MODE_VEB) {
			adapter->flags &= ~XLNID_FLAG_SRIOV_VEPA_BRIDGE_MODE;
		}

		else {
			return -EINVAL;
		}

		adapter->bridge_mode = mode;

		/* re-configure settings related to bridge mode */
		//xlnid_configure_bridge_mode(adapter);

		e_info(drv, "enabling bridge mode: %s\n",
				mode == BRIDGE_MODE_VEPA ? "VEPA" : "VEB");
	}

	return 0;
}

#ifdef HAVE_NDO_BRIDGE_GETLINK_NLFLAGS
static int xlnid_ndo_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq,
									struct net_device *dev,
									u32 __maybe_unused filter_mask, int nlflags)
#elif defined(HAVE_BRIDGE_FILTER)
static int xlnid_ndo_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq,
									struct net_device *dev,
									u32 __always_unused filter_mask)
#else
static int xlnid_ndo_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq,
									struct net_device *dev)
#endif /* HAVE_NDO_BRIDGE_GETLINK_NLFLAGS */
{
	struct xlnid_adapter *adapter = netdev_priv(dev);
	u16 mode;

	if (!(adapter->flags & XLNID_FLAG_SRIOV_ENABLED)) {
		return 0;
	}

	mode = adapter->bridge_mode;
#ifdef HAVE_NDO_DFLT_BRIDGE_GETLINK_VLAN_SUPPORT
	return ndo_dflt_bridge_getlink(skb, pid, seq, dev, mode, 0, 0, nlflags,
									filter_mask, NULL);
#elif defined(HAVE_NDO_BRIDGE_GETLINK_NLFLAGS)
	return ndo_dflt_bridge_getlink(skb, pid, seq, dev, mode, 0, 0, nlflags);
#elif defined(HAVE_NDO_FDB_ADD_VID) || \
defined NDO_DFLT_BRIDGE_GETLINK_HAS_BRFLAGS
	return ndo_dflt_bridge_getlink(skb, pid, seq, dev, mode, 0, 0);
#else
	return ndo_dflt_bridge_getlink(skb, pid, seq, dev, mode);
#endif /* HAVE_NDO_BRIDGE_GETLINK_NLFLAGS */
}
#endif /* HAVE_BRIDGE_ATTRIBS */
#endif /* HAVE_FDB_OPS */

#ifdef HAVE_NDO_FEATURES_CHECK
#define XLNID_MAX_TUNNEL_HDR_LEN 80
#ifdef NETIF_F_GSO_PARTIAL
#define XLNID_MAX_MAC_HDR_LEN 127
#define XLNID_MAX_NETWORK_HDR_LEN 511

static netdev_features_t xlnid_features_check(struct sk_buff *skb,
		struct net_device *dev,
		netdev_features_t features)
{
	unsigned int network_hdr_len, mac_hdr_len;

	/* Make certain the headers can be described by a context descriptor */
	mac_hdr_len = skb_network_header(skb) - skb->data;

	if (unlikely(mac_hdr_len > XLNID_MAX_MAC_HDR_LEN))
		return features &
				~(NETIF_F_HW_CSUM | NETIF_F_SCTP_CRC |
					NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_TSO | NETIF_F_TSO6);

	network_hdr_len = skb_checksum_start(skb) - skb_network_header(skb);

	if (unlikely(network_hdr_len > XLNID_MAX_NETWORK_HDR_LEN))
		return features & ~(NETIF_F_HW_CSUM | NETIF_F_SCTP_CRC |
							NETIF_F_TSO | NETIF_F_TSO6);

	/*  We can only support IPV4 TSO in tunnels if we can mangle the
		inner IP ID field, so strip TSO if MANGLEID is not supported.
	*/
	if (skb->encapsulation && !(features & NETIF_F_TSO_MANGLEID)) {
		features &= ~NETIF_F_TSO;
	}

	return features;
}
#else
static netdev_features_t xlnid_features_check(struct sk_buff *skb,
		struct net_device *dev,
		netdev_features_t features)
{
	if (!skb->encapsulation) {
		return features;
	}

	if (unlikely(skb_inner_mac_header(skb) - skb_transport_header(skb) >
					XLNID_MAX_TUNNEL_HDR_LEN)) {
		return features & ~NETIF_F_CSUM_MASK;
	}

	return features;
}
#endif /* NETIF_F_GSO_PARTIAL */
#endif /* HAVE_NDO_FEATURES_CHECK */

void xlnid_xdp_ring_update_tail(struct xlnid_ring *ring)
{
	/*  Force memory writes to complete before letting h/w know there
		are new descriptors to fetch.
	*/
	wmb();
	writel(ring->next_to_use, ring->tail);
}

void xlnid_xdp_ring_update_tail_locked(struct xlnid_ring *ring)
{
	if (static_branch_unlikely(&xlnid_xdp_locking_key)) {
		spin_lock(&ring->tx_lock);
	}

	xlnid_xdp_ring_update_tail(ring);

	if (static_branch_unlikely(&xlnid_xdp_locking_key)) {
		spin_unlock(&ring->tx_lock);
	}
}

#ifdef HAVE_XDP_SUPPORT
static int xlnid_xdp_setup(struct net_device *dev, struct bpf_prog *prog)
{
	int i, frame_size = dev->mtu + XLNID_PKT_HDR_PAD;
	struct xlnid_adapter *adapter = netdev_priv(dev);
	struct bpf_prog *old_prog;
	bool need_reset;

	if (adapter->flags & XLNID_FLAG_SRIOV_ENABLED) {
		return -EINVAL;
	}

	if (adapter->flags & XLNID_FLAG_DCB_ENABLED) {
		return -EINVAL;
	}

	/* verify xlnid ring attributes are sufficient for XDP */
	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct xlnid_ring *ring = adapter->rx_ring[i];

		if (ring_is_rsc_enabled(ring)) {
			return -EINVAL;
		}

		if (frame_size > xlnid_rx_bufsz(ring)) {
			return -EINVAL;
		}
	}

	/*  if the number of cpus is much larger than the maximum of queues,
		we should stop it and then return with NOMEM like before.
	*/
	if (nr_cpu_ids > WESTLAKE_MAX_XDP_QUEUES * 2) {
		return -ENOMEM;
	}

	else if (nr_cpu_ids > WESTLAKE_MAX_XDP_QUEUES) {
		static_branch_inc(&xlnid_xdp_locking_key);
	}

	old_prog = xchg(&adapter->xdp_prog, prog);
	need_reset = (!!prog != !!old_prog);

	/* If transitioning XDP modes reconfigure rings */
	if (need_reset) {
		int err = xlnid_setup_tc(dev, netdev_get_num_tc(dev));

		if (err) {
			rcu_assign_pointer(adapter->xdp_prog, old_prog);
			return -EINVAL;
		}
	}

	else {
		for (i = 0; i < adapter->num_rx_queues; i++) {
			xchg(&adapter->rx_ring[i]->xdp_prog, adapter->xdp_prog);
		}
	}

	if (old_prog) {
		bpf_prog_put(old_prog);
	}

#ifdef HAVE_AF_XDP_ZC_SUPPORT

	/*  Kick start the NAPI context if there is an AF_XDP socket open
		on that queue id. This so that receiving will start.
	*/
	if (need_reset && prog)
		for (i = 0; i < adapter->num_rx_queues; i++)
			if (adapter->xdp_ring[i]->xsk_pool)
#ifdef HAVE_NDO_XSK_WAKEUP
				(void)xlnid_xsk_wakeup(adapter->netdev, i,
										XDP_WAKEUP_RX);

#else
				(void)xlnid_xsk_async_xmit(adapter->netdev, i);
#endif

#endif
	return 0;
}

#ifdef HAVE_NDO_BPF
static int xlnid_xdp(struct net_device *dev, struct netdev_bpf *xdp)
#else
static int xlnid_xdp(struct net_device *dev, struct netdev_xdp *xdp)
#endif
{
#if defined(HAVE_XDP_QUERY_PROG) || defined(HAVE_AF_XDP_ZC_SUPPORT)
	struct xlnid_adapter *adapter = netdev_priv(dev);
#endif

	switch (xdp->command) {
	case XDP_SETUP_PROG:
		return xlnid_xdp_setup(dev, xdp->prog);
		#ifdef HAVE_XDP_QUERY_PROG

	case XDP_QUERY_PROG:
		#ifndef NO_NETDEV_BPF_PROG_ATTACHED
		xdp->prog_attached = !!(adapter->xdp_prog);
		#endif /* !NO_NETDEV_BPF_PROG_ATTACHED */
		xdp->prog_id = adapter->xdp_prog ? adapter->xdp_prog->aux->id :
		0;
		return 0;
		#endif
		#ifdef HAVE_AF_XDP_ZC_SUPPORT

	case XDP_SETUP_XSK_POOL:
		#ifndef HAVE_NETDEV_BPF_XSK_POOL
		return xlnid_xsk_umem_setup(adapter, xdp->xsk.umem,
		xdp->xsk.queue_id);
		#else
		return xlnid_xsk_umem_setup(adapter, xdp->xsk.pool,
		xdp->xsk.queue_id);
		#endif /* HAVE_NETDEV_BPF_XSK_POOL */
		#endif /* HAVE_AF_XDP_ZC_SUPPORT */

	default:
		return -EINVAL;
}
}

#ifdef HAVE_NDO_XDP_XMIT_BULK_AND_FLAGS
static int xlnid_xdp_xmit(struct net_device *dev, int n,
							struct xdp_frame **frames, u32 flags)
#else
static int xlnid_xdp_xmit(struct net_device *dev, struct xdp_buff *xdp)
#endif
{
	struct xlnid_adapter *adapter = netdev_priv(dev);
	struct xlnid_ring *ring;

#ifdef HAVE_NDO_XDP_XMIT_BULK_AND_FLAGS
	int drops = 0;
	int i;
#else
	int err;
#endif

	if (unlikely(test_bit(__XLNID_DOWN, &adapter->state))) {
		return -ENETDOWN;
	}

#ifdef HAVE_NDO_XDP_XMIT_BULK_AND_FLAGS

	if (unlikely(flags & ~XDP_XMIT_FLAGS_MASK)) {
		return -EINVAL;
	}

#endif

	/*  During program transitions its possible adapter->xdp_prog is assigned
		but ring has not been configured yet. In this case simply abort xmit.
	*/
	ring = adapter->xdp_prog ? xlnid_determine_xdp_ring(adapter) : NULL;

	if (unlikely(!ring)) {
		return -ENXIO;
	}

#ifdef HAVE_AF_XDP_ZC_SUPPORT

	if (unlikely(test_bit(__XLNID_TX_DISABLED, &ring->state))) {
		return -ENXIO;
	}

#endif

#ifdef HAVE_NDO_XDP_XMIT_BULK_AND_FLAGS

	if (static_branch_unlikely(&xlnid_xdp_locking_key)) {
		spin_lock(&ring->tx_lock);
	}

	for (i = 0; i < n; i++) {
		struct xdp_frame *xdpf = frames[i];
		int err;

		err = xlnid_xmit_xdp_ring(ring, xdpf);

		if (err != XLNID_XDP_TX) {
			xdp_return_frame_rx_napi(xdpf);
			drops++;
		}
	}

	if (unlikely(flags & XDP_XMIT_FLUSH)) {
		xlnid_xdp_ring_update_tail(ring);
	}

	if (static_branch_unlikely(&xlnid_xdp_locking_key)) {
		spin_unlock(&ring->tx_lock);
	}

	return n - drops;
#else /* HAVE_NDO_XDP_XMIT_BULK_AND_FLAGS */

	if (static_branch_unlikely(&xlnid_xdp_locking_key)) {
		spin_lock(&ring->tx_lock);
	}

	err = xlnid_xmit_xdp_ring(ring, xdp);

	if (static_branch_unlikely(&xlnid_xdp_locking_key)) {
		spin_unlock(&ring->tx_lock);
	}

	if (err != XLNID_XDP_TX) {
		return -ENOSPC;
	}

	return 0;
#endif /* HAVE_NDO_XDP_XMIT_BULK_AND_FLAGS */
}

#ifndef NO_NDO_XDP_FLUSH
static void xlnid_xdp_flush(struct net_device *dev)
{
	struct xlnid_adapter *adapter = netdev_priv(dev);
	struct xlnid_ring *ring;

	/*  Its possible the device went down between xdp xmit and flush so
		we need to ensure device is still up.
	*/
	if (unlikely(test_bit(__XLNID_DOWN, &adapter->state))) {
		return;
	}

	ring = adapter->xdp_prog ? adapter->xdp_ring[smp_processor_id()] : NULL;

	if (unlikely(!ring)) {
		return;
	}

	xlnid_xdp_ring_update_tail(ring);

	return;
}
#endif /* !NO_NDO_XDP_FLUSH */
#endif /* HAVE_XDP_SUPPORT */
#ifdef HAVE_NET_DEVICE_OPS
static const struct net_device_ops xlnid_netdev_ops = {
	.ndo_open = xlnid_open,
	.ndo_stop = xlnid_close,
	.ndo_start_xmit = xlnid_xmit_frame,
#ifdef HAVE_NETDEV_SELECT_QUEUE
#ifndef HAVE_MQPRIO
	.ndo_select_queue = __netdev_pick_tx,
#endif
#endif /* HAVE_NETDEV_SELECT_QUEUE */
	.ndo_set_rx_mode = xlnid_set_rx_mode,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_set_mac_address = xlnid_set_mac,
#ifdef HAVE_RHEL7_EXTENDED_MIN_MAX_MTU
	.extended.ndo_change_mtu = xlnid_change_mtu,
#else
	.ndo_change_mtu = xlnid_change_mtu,
#endif
	.ndo_tx_timeout = xlnid_tx_timeout,
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	.ndo_vlan_rx_add_vid = xlnid_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = xlnid_vlan_rx_kill_vid,
#endif

#ifdef HAVE_NDO_SIOCDEVPRIVATE
	.ndo_siocdevprivate = xlnid_siocdevprivate,
#endif /* HAVE_NDO_SIOCDEVPRIVATE */

#ifdef HAVE_NDO_ETH_IOCTL
	.ndo_eth_ioctl = xlnid_ioctl,
#else
	.ndo_do_ioctl = xlnid_ioctl,
#endif /* HAVE_NDO_ETH_IOCTL */
#ifdef HAVE_RHEL7_NET_DEVICE_OPS_EXT
	/*  RHEL7 requires this to be defined to enable extended ops.  RHEL7 uses the
		function get_ndo_ext to retrieve offsets for extended fields from with the
		net_device_ops struct and ndo_size is checked to determine whether or not
		the offset is valid.
	*/
	.ndo_size = sizeof(const struct net_device_ops),
#endif
#ifdef IFLA_VF_MAX
	.ndo_set_vf_mac = xlnid_ndo_set_vf_mac,
#ifdef HAVE_RHEL7_NETDEV_OPS_EXT_NDO_SET_VF_VLAN
	.extended.ndo_set_vf_vlan = xlnid_ndo_set_vf_vlan,
#else
	.ndo_set_vf_vlan = xlnid_ndo_set_vf_vlan,
#endif
#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	.ndo_set_vf_rate = xlnid_ndo_set_vf_bw,
#else
	.ndo_set_vf_tx_rate = xlnid_ndo_set_vf_bw,
#endif /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */
#ifdef HAVE_NDO_SET_VF_RSS_QUERY_EN
	.ndo_set_vf_rss_query_en = xlnid_ndo_set_vf_rss_query_en,
#endif
#ifdef HAVE_NDO_SET_VF_TRUST
#ifdef HAVE_RHEL7_NET_DEVICE_OPS_EXT
	.extended.ndo_set_vf_trust = xlnid_ndo_set_vf_trust,
#else
	.ndo_set_vf_trust = xlnid_ndo_set_vf_trust,
#endif /* HAVE_RHEL7_NET_DEVICE_OPS_EXT */
#endif /* HAVE_NDO_SET_VF_TRUST */
	.ndo_get_vf_config = xlnid_ndo_get_vf_config,
#endif /* IFLA_VF_MAX */
#ifdef HAVE_NDO_GET_STATS64
	.ndo_get_stats64 = xlnid_get_stats64,
#else
	.ndo_get_stats = xlnid_get_stats,
#endif /* HAVE_NDO_GET_STATS64 */
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller = xlnid_netpoll,
#endif
#ifndef HAVE_RHEL6_NET_DEVICE_EXTENDED
#ifdef HAVE_NDO_BUSY_POLL
	.ndo_busy_poll = xlnid_busy_poll_recv,
#endif /* HAVE_NDO_BUSY_POLL */
#endif /* !HAVE_RHEL6_NET_DEVICE_EXTENDED */
#ifdef HAVE_VLAN_RX_REGISTER
	.ndo_vlan_rx_register = &xlnid_vlan_mode,
#endif
#ifdef HAVE_FDB_OPS
	.ndo_fdb_add = xlnid_ndo_fdb_add,
#ifndef USE_DEFAULT_FDB_DEL_DUMP
	.ndo_fdb_del = ndo_dflt_fdb_del,
	.ndo_fdb_dump = ndo_dflt_fdb_dump,
#endif
#ifdef HAVE_BRIDGE_ATTRIBS
	.ndo_bridge_setlink = xlnid_ndo_bridge_setlink,
	.ndo_bridge_getlink = xlnid_ndo_bridge_getlink,
#endif /* HAVE_BRIDGE_ATTRIBS */
#endif
#ifdef HAVE_NDO_GSO_CHECK
	.ndo_gso_check = xlnid_gso_check,
#endif /* HAVE_NDO_GSO_CHECK */
#ifdef HAVE_NDO_FEATURES_CHECK
	.ndo_features_check = xlnid_features_check,
#endif /* HAVE_NDO_FEATURES_CHECK */
#ifdef HAVE_XDP_SUPPORT
#ifdef HAVE_NDO_BPF
	.ndo_bpf = xlnid_xdp,
#else
	.ndo_xdp = xlnid_xdp,
#endif
	.ndo_xdp_xmit = xlnid_xdp_xmit,
#ifdef HAVE_AF_XDP_ZC_SUPPORT
#ifdef HAVE_NDO_XSK_WAKEUP
	.ndo_xsk_wakeup = xlnid_xsk_wakeup,
#else
	.ndo_xsk_async_xmit = xlnid_xsk_async_xmit,
#endif
#endif
#ifndef NO_NDO_XDP_FLUSH
	.ndo_xdp_flush = xlnid_xdp_flush,
#endif /* !NO_NDO_XDP_FLUSH */
#endif
#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
};

/* RHEL6 keeps these operations in a separate structure */
static const struct net_device_ops_ext xlnid_netdev_ops_ext = {
	.size = sizeof(struct net_device_ops_ext),
#endif /* HAVE_RHEL6_NET_DEVICE_OPS_EXT */
#ifdef HAVE_NDO_SET_FEATURES
	.ndo_set_features = xlnid_set_features,
	.ndo_fix_features = xlnid_fix_features,
#endif /* HAVE_NDO_SET_FEATURES */
};
#endif /* HAVE_NET_DEVICE_OPS */

void xlnid_assign_netdev_ops(struct net_device *dev)
{
#ifdef HAVE_NET_DEVICE_OPS
	dev->netdev_ops = &xlnid_netdev_ops;
#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
	set_netdev_ops_ext(dev, &xlnid_netdev_ops_ext);
#endif /* HAVE_RHEL6_NET_DEVICE_OPS_EXT */
#else /* HAVE_NET_DEVICE_OPS */
	dev->open = &xlnid_open;
	dev->stop = &xlnid_close;
	dev->hard_start_xmit = &xlnid_xmit_frame;
	dev->get_stats = &xlnid_get_stats;
#ifdef HAVE_SET_RX_MODE
	dev->set_rx_mode = &xlnid_set_rx_mode;
#endif
	dev->set_multicast_list = &xlnid_set_rx_mode;
	dev->set_mac_address = &xlnid_set_mac;
	dev->change_mtu = &xlnid_change_mtu;
#ifdef HAVE_NDO_SIOCDEVPRIVATE
	dev->do_ioctl = &xlnid_siocdevprivate;
#else
	dev->do_ioctl = &xlnid_ioctl;
#endif
#ifdef HAVE_TX_TIMEOUT
	dev->tx_timeout = &xlnid_tx_timeout;
#endif
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	dev->vlan_rx_register = &xlnid_vlan_mode;
	dev->vlan_rx_add_vid = &xlnid_vlan_rx_add_vid;
	dev->vlan_rx_kill_vid = &xlnid_vlan_rx_kill_vid;
#endif
#ifdef CONFIG_NET_POLL_CONTROLLER
	dev->poll_controller = &xlnid_netpoll;
#endif
#ifdef HAVE_NETDEV_SELECT_QUEUE
	dev->select_queue = &__netdev_pick_tx;
#endif /* HAVE_NETDEV_SELECT_QUEUE */
#endif /* HAVE_NET_DEVICE_OPS */

#ifdef HAVE_RHEL6_NET_DEVICE_EXTENDED
#ifdef HAVE_NDO_BUSY_POLL
	netdev_extended(dev)->ndo_busy_poll = xlnid_busy_poll_recv;
#endif /* HAVE_NDO_BUSY_POLL */
#endif /* HAVE_RHEL6_NET_DEVICE_EXTENDED */

	xlnid_set_ethtool_ops(dev);

	dev->watchdog_timeo = 5 * HZ;
}

static int xel_pci_init(struct pci_dev *dev, struct xlnid_hw *hw)
{
	struct xlnid_adapter *adapter = hw->back;
	memset(&xel_pci_info[device_num], 0, sizeof(xel_pci_info_t));

	if (hw->mac.type == xlnid_mac_SKYLAKE) {
		xel_pci_info[device_num].pcie_resource_length =
			64 * 1024 * 1024;
	}

	else {
		xel_pci_info[device_num].pcie_resource_length = 1 * 1024 * 1024;
	}

	xel_pci_info[device_num].device_id = dev->device;
	xel_pci_info[device_num].vendor_id = dev->vendor;
	xel_pci_info[device_num].pcie_function_id = dev->devfn;

	e_dev_info("Found PCIe device(0x%x) %04x:%04x (unit=%d)\n",
				PCI_DEVID(dev->bus->number, dev->devfn), dev->vendor,
				dev->device, device_num);

	xel_pci_info[device_num].pcie_bar0 = pci_resource_start(dev, 0);
	e_dev_info(" PCIe bar : 0x%08x \n", xel_pci_info[device_num].pcie_bar0);

	xel_pci_info[device_num].pcie_bar0_end = pci_resource_end(dev, 0);
	e_dev_info(" PCIe bar end: 0x%08x \n",
				xel_pci_info[device_num].pcie_bar0_end);

	if (xel_pci_info[device_num].pcie_resource_length >
			pci_resource_len(dev, 0)) {
		e_dev_warn("WARN !!! kernel alloc PCI mem len wrong\n");
		xel_pci_info[device_num].pcie_resource_length =
			pci_resource_len(dev, 0);
	}

	xel_pci_info[device_num].hw = hw;
	xel_pci_info[device_num].dev = (void *)dev;

	device_num++;
	return 0;
}

/**
	xlnid_probe - Device Initialization Routine
	@pdev: PCI device information struct
	@ent: entry in xlnid_pci_tbl

	Returns 0 on success, negative on failure

	xlnid_probe initializes an adapter identified by a pci_dev structure.
	The OS initialization, configuring of the adapter private structure,
	and a hardware reset occur.
**/
#ifdef HAVE_CONFIG_HOTPLUG
static int __devinit xlnid_probe(
	struct pci_dev *pdev, const struct pci_device_id __always_unused *ent)
#else
static int xlnid_probe(struct pci_dev *pdev,
						const struct pci_device_id __always_unused *ent)
#endif
{
	struct xlnid_adapter *adapter = NULL;
	struct net_device *netdev = NULL;
	struct xlnid_hw *hw = NULL;
	static int cards_found;
	int err, pci_using_dac;
	char *info_string;
	enum xlnid_mac_type mac_type = xlnid_mac_unknown;
#ifdef HAVE_TX_MQ
	unsigned int indices = MAX_TX_QUEUES;
#endif /* HAVE_TX_MQ */
	bool disable_dev = false;

#ifndef NETIF_F_GSO_PARTIAL
#ifdef HAVE_NDO_SET_FEATURES
#ifndef HAVE_RHEL6_NET_DEVICE_OPS_EXT
	netdev_features_t hw_features;
#else
	u32 hw_features;
#endif
#endif
#endif /* NETIF_F_GSO_PARTIAL */
	int i = 0;
	u32 regval_RAL = 0;
	u32 regval_RAH = 0;

	err = pci_enable_device_mem(pdev);

	if (err) {
		return err;
	}

	if (!dma_set_mask(&pdev->dev, DMA_BIT_MASK(64)) &&
			!dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(64))) {
		pci_using_dac = 1;
	}

	else {
		err = dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));

		if (err) {
			err = dma_set_coherent_mask(&pdev->dev,
										DMA_BIT_MASK(32));

			if (err) {
				dev_err(&pdev->dev,
						"No usable DMA "
						"configuration, aborting\n");
				goto err_dma;
			}
		}

		pci_using_dac = 0;
	}

	err = pci_request_mem_regions(pdev, xlnid_driver_name);

	if (err) {
		dev_err(&pdev->dev,
				"pci_request_selected_regions failed 0x%x\n", err);
		goto err_pci_reg;
	}

	/*
		The mac_type is needed before we have the adapter is  set up
		so rather than maintain two devID -> MAC tables we dummy up
		an xlnid_hw stuct and use xlnid_set_mac_type.
	*/
	hw = vmalloc(sizeof(struct xlnid_hw));

	if (!hw) {
		pr_info("Unable to allocate memory for early mac "
				"check\n");
	}

	else {
		hw->vendor_id = pdev->vendor;
		hw->device_id = pdev->device;
		xlnid_set_mac_type(hw, true);
		mac_type = hw->mac.type;
		vfree(hw);
	}

#ifdef HAVE_PCI_ENABLE_PCIE_ERROR_REPORTING
	pci_enable_pcie_error_reporting(pdev);
#endif /* HAVE_PCI_ENABLE_PCIE_ERROR_REPORTING */

	pci_set_master(pdev);

#ifdef HAVE_TX_MQ
	netdev = alloc_etherdev_mq(sizeof(struct xlnid_adapter), indices);
#else /* HAVE_TX_MQ */
	netdev = alloc_etherdev(sizeof(struct xlnid_adapter));
#endif /* HAVE_TX_MQ */

	if (!netdev) {
		err = -ENOMEM;
		goto err_alloc_etherdev;
	}

	SET_MODULE_OWNER(netdev);
	SET_NETDEV_DEV(netdev, &pdev->dev);

	adapter = netdev_priv(netdev);

#ifdef HAVE_TX_MQ
#ifndef HAVE_NETDEV_SELECT_QUEUE
	adapter->indices = indices;
#endif
#endif
	adapter->netdev = netdev;
	adapter->pdev = pdev;
	hw = &adapter->hw;
	hw->back = adapter;
	adapter->msg_enable = (1 << DEFAULT_DEBUG_LEVEL_SHIFT) - 1;

	hw->hw_addr =
		ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
	adapter->io_addr = hw->hw_addr;

	if (!hw->hw_addr) {
		err = -EIO;
		goto err_ioremap;
	}

	xlnid_assign_netdev_ops(netdev);

	strscpy(netdev->name, pci_name(pdev), sizeof(netdev->name));

	adapter->bd_number = cards_found;

	/* setup the private structure */
	err = xlnid_sw_init(adapter);

	if (err) {
		goto err_sw_init;
	}

	/* Make sure the SWFW semaphore is in a valid state */
	if (hw->mac.ops.init_swfw_sync) {
		hw->mac.ops.init_swfw_sync(hw);
	}

	/*
		check_options must be called before setup_link to set up
		hw->fc completely
	*/
	xlnid_check_options(adapter);

	/* reset_hw fills in the perm_addr as well */
	hw->phy.reset_if_overtemp = true;
	err = hw->mac.ops.reset_hw(hw);
	hw->phy.reset_if_overtemp = false;

	if (err == XLNID_ERR_SFP_NOT_PRESENT) {
		err = XLNID_SUCCESS;
	}

	else if (err == XLNID_ERR_SFP_NOT_SUPPORTED) {
		e_dev_err("failed to load because an unsupported SFP+ or QSFP "
					"module type was detected.\n");
		e_dev_err("Reload the driver after installing a supported "
					"module.\n");
		goto err_sw_init;
	}

	else if (err) {
		e_dev_err("HW Init failed: %d\n", err);
		goto err_sw_init;
	}

#ifdef NETIF_F_GSO_PARTIAL
	/* keep |= here to avoid conflict with features set in param.c */
	netdev->features |= NETIF_F_SG | NETIF_F_TSO | NETIF_F_TSO6 |
						NETIF_F_RXHASH | NETIF_F_RXCSUM | NETIF_F_HW_CSUM;

	netdev->gso_partial_features = XLNID_GSO_PARTIAL_FEATURES;
	netdev->features |= NETIF_F_GSO_PARTIAL | XLNID_GSO_PARTIAL_FEATURES;

	/* netdev->features |= NETIF_F_SCTP_CRC; */

	/* copy netdev features into list of user selectable features */
	netdev->hw_features |= netdev->features | NETIF_F_HW_VLAN_CTAG_FILTER |
							NETIF_F_HW_VLAN_CTAG_RX |
							NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_RXALL;

	netdev->hw_features |= NETIF_F_NTUPLE;

#if IS_ENABLED(CONFIG_MACSEC)
	/* MACsec */
	netdev->features |= NETIF_F_HW_MACSEC;
#endif

	if (pci_using_dac) {
		netdev->features |= NETIF_F_HIGHDMA;
	}

	netdev->vlan_features |= netdev->features | NETIF_F_TSO_MANGLEID;
	netdev->hw_enc_features |= netdev->vlan_features;
	netdev->mpls_features |= NETIF_F_HW_CSUM;

	/* set this bit last since it cannot be part of vlan_features */
	netdev->features |= NETIF_F_HW_VLAN_CTAG_FILTER |
						NETIF_F_HW_VLAN_CTAG_RX | NETIF_F_HW_VLAN_CTAG_TX;

	netdev->priv_flags |= IFF_UNICAST_FLT;
	netdev->priv_flags |= IFF_SUPP_NOFCS;

	/* give us the option of enabling RSC/LRO later */
	if (adapter->flags2 & XLNID_FLAG2_RSC_CAPABLE) {
		netdev->hw_features |= NETIF_F_LRO;
	}

#else /* NETIF_F_GSO_PARTIAL */
	/* keep |= here to avoid conflict with features set in param.c */
	netdev->features |= NETIF_F_SG | NETIF_F_IP_CSUM;

#ifdef NETIF_F_IPV6_CSUM
	netdev->features |= NETIF_F_IPV6_CSUM;
#endif

#ifdef NETIF_F_HW_VLAN_CTAG_TX
	netdev->features |= NETIF_F_HW_VLAN_CTAG_TX |
						NETIF_F_HW_VLAN_CTAG_FILTER |
						NETIF_F_HW_VLAN_CTAG_RX;
#endif

#ifdef NETIF_F_HW_VLAN_TX
	netdev->features |= NETIF_F_HW_VLAN_TX | NETIF_F_HW_VLAN_FILTER |
						NETIF_F_HW_VLAN_RX;
#endif
	netdev->features |= xlnid_tso_features();
#ifdef NETIF_F_RXHASH
	netdev->features |= NETIF_F_RXHASH;
#endif /* NETIF_F_RXHASH */
	netdev->features |= NETIF_F_RXCSUM;

#ifdef HAVE_NDO_SET_FEATURES
	/* copy netdev features into list of user selectable features */
#ifndef HAVE_RHEL6_NET_DEVICE_OPS_EXT
	hw_features = netdev->hw_features;
#else
	hw_features = get_netdev_hw_features(netdev);
#endif
	hw_features |= netdev->features;

	/* give us the option of enabling RSC/LRO later */
	if (adapter->flags2 & XLNID_FLAG2_RSC_CAPABLE) {
		hw_features |= NETIF_F_LRO;
	}

#else
#ifdef NETIF_F_GRO

	/* this is only needed on kernels prior to 2.6.39 */
	netdev->features |= NETIF_F_GRO;
#endif /* NETIF_F_GRO */
#endif /* HAVE_NDO_SET_FEATURES */

	netdev->features |= NETIF_F_SCTP_CSUM;
#ifdef HAVE_NDO_SET_FEATURES
	hw_features |= NETIF_F_SCTP_CSUM | NETIF_F_NTUPLE;
#endif

#ifdef HAVE_NDO_SET_FEATURES
#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
	set_netdev_hw_features(netdev, hw_features);
#else
	netdev->hw_features = hw_features;
#endif
#endif

#ifdef HAVE_NETDEV_VLAN_FEATURES
	netdev->vlan_features |= NETIF_F_SG | NETIF_F_IP_CSUM |
								NETIF_F_IPV6_CSUM | NETIF_F_TSO | NETIF_F_TSO6;

#endif /* HAVE_NETDEV_VLAN_FEATURES */
#ifdef HAVE_ENCAP_CSUM_OFFLOAD
	netdev->hw_enc_features |= NETIF_F_SG;
#endif /* HAVE_ENCAP_CSUM_OFFLOAD */
#ifdef HAVE_VXLAN_RX_OFFLOAD

	if (adapter->flags & XLNID_FLAG_VXLAN_OFFLOAD_CAPABLE) {
		netdev->hw_enc_features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
	}

#endif /* NETIF_F_GSO_PARTIAL */

#endif /* HAVE_VXLAN_RX_OFFLOAD */

	if (netdev->features & NETIF_F_LRO) {
		if ((adapter->flags2 & XLNID_FLAG2_RSC_CAPABLE) &&
				((adapter->rx_itr_setting == 1) ||
					(adapter->rx_itr_setting > XLNID_MIN_RSC_ITR))) {
			adapter->flags2 |= XLNID_FLAG2_RSC_ENABLED;
		}

		else if (adapter->flags2 & XLNID_FLAG2_RSC_CAPABLE) {
			e_dev_info("InterruptThrottleRate set too high, "
						"disabling RSC\n");
		}
	}

#ifdef IFF_UNICAST_FLT
	netdev->priv_flags |= IFF_UNICAST_FLT;
#endif
#ifdef IFF_SUPP_NOFCS
	netdev->priv_flags |= IFF_SUPP_NOFCS;
#endif

#ifdef HAVE_NETDEVICE_MIN_MAX_MTU
	/* MTU range: 68 - 9710 */
#ifdef HAVE_RHEL7_EXTENDED_MIN_MAX_MTU
	netdev->extended->min_mtu = ETH_MIN_MTU;
	netdev->extended->max_mtu =
		XLNID_MAX_JUMBO_FRAME_SIZE - (ETH_HLEN + ETH_FCS_LEN);
#else
	netdev->min_mtu = ETH_MIN_MTU;
	netdev->max_mtu = XLNID_MAX_JUMBO_FRAME_SIZE - (ETH_HLEN + ETH_FCS_LEN);
#endif

#endif

	if (pci_using_dac) {
		netdev->features |= NETIF_F_HIGHDMA;
#ifdef HAVE_NETDEV_VLAN_FEATURES
		netdev->vlan_features |= NETIF_F_HIGHDMA;
#endif /* HAVE_NETDEV_VLAN_FEATURES */
	}

	if (!is_valid_ether_addr(hw->mac.perm_addr)) {
		hw->mac.perm_addr[0] = 0x00;
		hw->mac.perm_addr[1] = 0x11;
		hw->mac.perm_addr[2] = 0x22;
		hw->mac.perm_addr[3] = 0x33;
		hw->mac.perm_addr[4] = hw->bus.bus_id;
		hw->mac.perm_addr[5] = hw->bus.func;
	}

	eth_hw_addr_set(netdev, hw->mac.perm_addr);
#ifdef ETHTOOL_GPERMADDR
	memcpy(netdev->perm_addr, hw->mac.perm_addr, netdev->addr_len);
#endif

	/* Set hw->mac.addr to permanent MAC address */
	ether_addr_copy(hw->mac.addr, hw->mac.perm_addr);
	xlnid_mac_set_default_filter(adapter);

	timer_setup(&adapter->service_timer, xlnid_service_timer, 0);

	if (XLNID_REMOVED(hw->hw_addr)) {
		err = -EIO;
		goto err_sw_init;
	}

	INIT_WORK(&adapter->service_task, xlnid_service_task);
	set_bit(__XLNID_SERVICE_INITED, &adapter->state);
	clear_bit(__XLNID_SERVICE_SCHED, &adapter->state);

	err = xlnid_init_interrupt_scheme(adapter);

	if (err) {
		goto err_sw_init;
	}

	for (i = 0; i < adapter->num_xdp_queues; i++) {
		u64_stats_init(&adapter->xdp_ring[i]->syncp);
	}

	/* WOL not supported for all devices */
	adapter->wol = 0;
	hw->wol_enabled = !!(adapter->wol);

	device_set_wakeup_enable(xlnid_pf_to_dev(adapter), adapter->wol);

	/* reset the hardware with the new settings */
	err = hw->mac.ops.start_hw(hw);

	if (err == XLNID_ERR_EEPROM_VERSION) {
		/* We are running on a pre-production device, log a warning */
		e_dev_warn("This device is a pre-production adapter/LOM. "
					"Please be aware there may be issues associated "
					"with your hardware.  If you are experiencing "
					"problems please contact your Xel or hardware "
					"representative who provided you with this "
					"hardware.\n");
	}

	else if (err == XLNID_ERR_OVERTEMP) {
		e_crit(drv, "%s\n", xlnid_overheat_msg);
		goto err_register;
	}

	else if (err) {
		e_dev_err("HW init failed\n");
		goto err_register;
	}

	/* pick up the PCI bus settings for reporting later */
	if (xlnid_pcie_from_parent(hw)) {
		xlnid_get_parent_bus_info(hw);
	}

	else if (hw->mac.ops.get_bus_info) {
		hw->mac.ops.get_bus_info(hw);
	}

	strscpy(netdev->name, "eth%d", sizeof(netdev->name));
	pci_set_drvdata(pdev, adapter);

	xlnid_ptsw_check(hw);

	/* if ptsw enable, only register function 0 to netdev */
	if (hw->ptsw_enable && (pdev->devfn != 0)) {
		xel_pci_info_t *master_pinfo =
			xlnid_find_pci_info(hw->bus.bus_id, 0);

		regval_RAL =
			XLNID_READ_REG_MAC(master_pinfo->hw, WESTLAKE_RAL(0));
		regval_RAH =
			XLNID_READ_REG_MAC(master_pinfo->hw, WESTLAKE_RAH(0));
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_RAL(0), regval_RAL);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_RAH(0), regval_RAH);
		goto exit;
	}

	err = register_netdev(netdev);

	if (err) {
		goto err_register;
	}

#if IS_ENABLED(CONFIG_MACSEC)
	xlnid_macsec_init(adapter);
#endif
	adapter->netdev_registered = true;

#ifdef HAVE_PCI_ERS
	/*
		call save state here in standalone driver because it relies on
		adapter struct to exist, and needs to call netdev_priv
	*/
	pci_save_state(pdev);

#endif

	/* power down the optics for SFP+ fiber */
	if (hw->mac.ops.disable_tx_laser) {
		hw->mac.ops.disable_tx_laser(hw);
	}

	/* carrier off reporting is important to ethtool even BEFORE open */
	netif_carrier_off(netdev);
	/* keep stopping all the transmit queues for older kernels */
	netif_tx_stop_all_queues(netdev);

	/* print all messages at the end so that we use our eth%d name */
	xlnid_check_minimum_link(adapter);

	/* e_info(probe, "MAC: %d, PHY: %d\n", hw->mac.type, hw->phy.type); */

	e_dev_info("%02x:%02x:%02x:%02x:%02x:%02x\n", netdev->dev_addr[0],
				netdev->dev_addr[1], netdev->dev_addr[2],
				netdev->dev_addr[3], netdev->dev_addr[4],
				netdev->dev_addr[5]);

#define INFO_STRING_LEN 255
	info_string = kzalloc(INFO_STRING_LEN, GFP_KERNEL);

	if (!info_string) {
		e_err(probe, "allocation for info string failed\n");
		goto no_info_string;
	}

	snprintf(info_string, INFO_STRING_LEN,
				"Enabled Features: RxQ: %d TxQ: %d", adapter->num_rx_queues,
				adapter->num_tx_queues);

	if (adapter->flags & XLNID_FLAG_FDIR_HASH_CAPABLE) {
		strlcat(info_string, " FdirHash", INFO_STRING_LEN);
	}

	if (adapter->flags & XLNID_FLAG_DCB_ENABLED) {
		strlcat(info_string, " DCB", INFO_STRING_LEN);
	}

	if (adapter->flags2 & XLNID_FLAG2_RSC_ENABLED) {
		strlcat(info_string, " RSC", INFO_STRING_LEN);
	}

	if (adapter->flags & XLNID_FLAG_VXLAN_OFFLOAD_ENABLE) {
		strlcat(info_string, " vxlan_rx", INFO_STRING_LEN);
	}

	/* end features printing */
	e_info(probe, "%s\n", info_string);
	kfree(info_string);
no_info_string:
#if 0

	if (adapter->flags & XLNID_FLAG_SRIOV_ENABLED) {
		int i;

		for (i = 0; i < adapter->num_vfs; i++) {
			xlnid_vf_configuration(pdev, (i | 0x10000000));
		}
	}

#endif

	/* Initialize the LED link active for LED blink support */
	if (hw->mac.ops.init_led_link_act) {
		hw->mac.ops.init_led_link_act(hw);
	}

	/* firmware requires blank numerical version */
	if (hw->mac.ops.set_fw_drv_ver)
		hw->mac.ops.set_fw_drv_ver(hw, 0xFF, 0xFF, 0xFF, 0xFF,
									sizeof(xlnid_driver_version) - 1,
									xlnid_driver_version);

#if defined(HAVE_NETDEV_STORAGE_ADDRESS) && defined(NETDEV_HW_ADDR_T_SAN)
	/* add san mac addr to netdev */
	//xlnid_add_sanmac_netdev(netdev);

#endif /* (HAVE_NETDEV_STORAGE_ADDRESS) && (NETDEV_HW_ADDR_T_SAN) */
	e_info(probe, "Xel-Tech(R) Network Interface Connection\n");
	cards_found++;

#ifdef XLNID_SYSFS

	if (xlnid_sysfs_init(adapter)) {
		e_err(probe, "failed to allocate sysfs resources\n");
	}

#else
#ifdef XLNID_PROCFS

	if (xlnid_procfs_init(adapter)) {
		e_err(probe, "failed to allocate procfs resources\n");
	}

#endif /* XLNID_PROCFS */
#endif /* XLNID_SYSFS */
#ifdef HAVE_XLNID_DEBUG_FS

	xlnid_dbg_adapter_init(adapter);
#endif /* HAVE_XLNID_DEBUG_FS */

	if (hw->mac.ops.setup_eee &&
			(adapter->flags2 & XLNID_FLAG2_EEE_CAPABLE)) {
		bool eee_enable = !!(adapter->flags2 & XLNID_FLAG2_EEE_ENABLED);

		hw->mac.ops.setup_eee(hw, eee_enable);
	}

exit:
	xel_pci_init(pdev, hw);
	return 0;

err_register:
#if IS_ENABLED(CONFIG_MACSEC)

	if (err) {
		xlnid_macsec_free(adapter);
	}

#endif

	xlnid_clear_interrupt_scheme(adapter);
err_sw_init:
#if 0
	xlnid_disable_sriov(adapter);
#endif /* CONFIG_PCI_IOV */
	adapter->flags2 &= ~XLNID_FLAG2_SEARCH_FOR_SFP;
	kfree(adapter->mac_table);
	kfree(adapter->rss_key);
	bitmap_free(adapter->af_xdp_zc_qps);
	iounmap(adapter->io_addr);
err_ioremap:
	disable_dev = !test_and_set_bit(__XLNID_DISABLED, &adapter->state);
	free_netdev(netdev);
err_alloc_etherdev:
	pci_release_mem_regions(pdev);
err_pci_reg:
err_dma:

	if (!adapter || disable_dev) {
		pci_disable_device(pdev);
	}

	return err;
}

static inline void xlnid_ptsw_free_irq(struct xlnid_adapter *adapter)
{
	xel_pci_info_t *master_pinfo =
		xlnid_find_pci_info(adapter->hw.bus.bus_id, 0);
	struct xlnid_adapter *master_adapter = master_pinfo->hw->back;

	if (test_bit(__XLNID_DOWN, &master_adapter->state)) {
		/* do nothing if already down */
		return;
	}

	if (!(adapter->flags & XLNID_FLAG_MSIX_ENABLED)) {
		free_irq(adapter->pdev->irq, master_adapter);
	}

	else {
		free_irq(adapter->msix_entries[adapter->num_q_vectors].vector,
					master_adapter);
	}
}

/**
	xlnid_remove - Device Removal Routine
	@pdev: PCI device information struct

	xlnid_remove is called by the PCI subsystem to alert the driver
	that it should release a PCI device.  The could be caused by a
	Hot-Plug event, or because the driver is going to be removed from
	memory.
**/
#ifdef HAVE_CONFIG_HOTPLUG
static void __devexit xlnid_remove(struct pci_dev *pdev)
#else
static void xlnid_remove(struct pci_dev *pdev)
#endif
{
	struct xlnid_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev;
	bool disable_dev;

	/* if !adapter then we already cleaned up in probe */
	if (!adapter) {
		return;
	}

	netdev = adapter->netdev;

#if IS_ENABLED(CONFIG_MACSEC)
	xlnid_macsec_free(adapter);
#endif

#ifdef HAVE_XLNID_DEBUG_FS
	xlnid_dbg_adapter_exit(adapter);
#endif /*HAVE_XLNID_DEBUG_FS */
	set_bit(__XLNID_REMOVING, &adapter->state);

	cancel_work_sync(&adapter->service_task);

#ifdef XLNID_SYSFS
	xlnid_sysfs_exit(adapter);
#else
#ifdef XLNID_PROCFS
	xlnid_procfs_exit(adapter);
#endif /* XLNID_PROCFS */
#endif /* XLNID-SYSFS */

#if defined(HAVE_NETDEV_STORAGE_ADDRESS) && defined(NETDEV_HW_ADDR_T_SAN)
	/* remove the added san mac */
	//xlnid_del_sanmac_netdev(netdev);

#endif /* (HAVE_NETDEV_STORAGE_ADDRESS) && (NETDEV_HW_ADDR_T_SAN) */

#if 0
	xlnid_disable_sriov(adapter);
#endif /* CONFIG_PCI_IOV */

	if (adapter->netdev_registered) {
		unregister_netdev(netdev);
		adapter->netdev_registered = false;
	}

	if (adapter->hw.ptsw_enable && adapter->hw.bus.func == 1) {
		xlnid_ptsw_free_irq(adapter);
		xlnid_set_phy_power(&adapter->hw, false);
	}

	xlnid_clear_interrupt_scheme(adapter);

#ifdef HAVE_DCBNL_IEEE
	kfree(adapter->xlnid_ieee_pfc);
	kfree(adapter->xlnid_ieee_ets);

#endif
	iounmap(adapter->io_addr);
	pci_release_mem_regions(pdev);
	kfree(adapter->mac_table);
	kfree(adapter->rss_key);
	bitmap_free(adapter->af_xdp_zc_qps);
	disable_dev = !test_and_set_bit(__XLNID_DISABLED, &adapter->state);

	free_netdev(netdev);

#ifdef HAVE_PCI_ENABLE_PCIE_ERROR_REPORTING
	pci_disable_pcie_error_reporting(pdev);
#endif /* HAVE_PCI_ENABLE_PCIE_ERROR_REPORTING */

	if (disable_dev) {
		pci_disable_device(pdev);
	}
}

static bool xlnid_check_cfg_remove(struct xlnid_hw *hw,
								struct pci_dev *pdev)
{
	u16 value;

	pci_read_config_word(pdev, PCI_VENDOR_ID, &value);

	if (value == XLNID_FAILED_READ_CFG_WORD) {
		xlnid_remove_adapter(hw);
		return true;
	}

	return false;
}

u16 xlnid_read_pci_cfg_word(struct xlnid_hw *hw, u32 reg)
{
	struct xlnid_adapter *adapter = hw->back;
	u16 value;

	if (XLNID_REMOVED(hw->hw_addr)) {
		return XLNID_FAILED_READ_CFG_WORD;
	}

	pci_read_config_word(adapter->pdev, reg, &value);

	if (value == XLNID_FAILED_READ_CFG_WORD &&
			xlnid_check_cfg_remove(hw, adapter->pdev)) {
		return XLNID_FAILED_READ_CFG_WORD;
	}

	return value;
}

#ifdef HAVE_PCI_ERS
#if 0
static u32 xlnid_read_pci_cfg_dword(struct xlnid_hw *hw, u32 reg)
{
	struct xlnid_adapter *adapter = hw->back;
	u32 value;

	if (XLNID_REMOVED(hw->hw_addr)) {
		return XLNID_FAILED_READ_CFG_DWORD;
	}

	pci_read_config_dword(adapter->pdev, reg, &value);

	if (value == XLNID_FAILED_READ_CFG_DWORD &&
			xlnid_check_cfg_remove(hw, adapter->pdev)) {
		return XLNID_FAILED_READ_CFG_DWORD;
	}

	return value;
}
#endif /* CONFIG_PCI_IOV */
#endif /* HAVE_PCI_ERS */

void xlnid_write_pci_cfg_word(struct xlnid_hw *hw, u32 reg, u16 value)
{
	struct xlnid_adapter *adapter = hw->back;

	if (XLNID_REMOVED(hw->hw_addr)) {
		return;
	}

	pci_write_config_word(adapter->pdev, reg, value);
}

void ewarn(struct xlnid_hw *hw, const char *st)
{
	struct xlnid_adapter *adapter = hw->back;

	netif_warn(adapter, drv, adapter->netdev, "%s", st);
}

#ifdef HAVE_PCI_ERS
/**
	xlnid_io_error_detected - called when PCI error is detected
	@pdev: Pointer to PCI device
	@state: The current pci connection state

	This function is called after a PCI bus error affecting
	this device has been detected.
*/
static pci_ers_result_t xlnid_io_error_detected(struct pci_dev *pdev,
	pci_channel_state_t state)
{
	struct xlnid_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;

	if (!test_bit(__XLNID_SERVICE_INITED, &adapter->state)) {
		return PCI_ERS_RESULT_DISCONNECT;
	}

	if (!netif_device_present(netdev)) {
		return PCI_ERS_RESULT_DISCONNECT;
	}

	rtnl_lock();
	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		xlnid_close_suspend(adapter);
	}

	if (state == pci_channel_io_perm_failure) {
		rtnl_unlock();
		return PCI_ERS_RESULT_DISCONNECT;
	}

	if (!test_and_set_bit(__XLNID_DISABLED, &adapter->state)) {
		pci_disable_device(pdev);
	}

	rtnl_unlock();

	/* Request a slot reset. */
	return PCI_ERS_RESULT_NEED_RESET;
}

/**
	xlnid_io_slot_reset - called after the pci bus has been reset.
	@pdev: Pointer to PCI device

	Restart the card from scratch, as if from a cold-boot.
*/
static pci_ers_result_t xlnid_io_slot_reset(struct pci_dev *pdev)
{
	struct xlnid_adapter *adapter = pci_get_drvdata(pdev);
	pci_ers_result_t result;

	if (pci_enable_device_mem(pdev)) {
		e_err(probe, "Cannot re-enable PCI device after reset.\n");
		result = PCI_ERS_RESULT_DISCONNECT;
	}

	else {
		smp_mb__before_atomic();
		clear_bit(__XLNID_DISABLED, &adapter->state);
		adapter->hw.hw_addr = adapter->io_addr;
		pci_set_master(pdev);
		pci_restore_state(pdev);
		/*
			After second error pci->state_saved is false, this
			resets it so EEH doesn't break.
		*/
		pci_save_state(pdev);

		pci_wake_from_d3(pdev, false);

		xlnid_reset(adapter);
		//XLNID_WRITE_REG(&adapter->hw, XLNID_WUS, ~0);
		result = PCI_ERS_RESULT_RECOVERED;
	}

	pci_aer_clear_nonfatal_status(pdev);

	return result;
}

/**
	xlnid_io_resume - called when traffic can start flowing again.
	@pdev: Pointer to PCI device

	This callback is called when the error recovery driver tells us that
	its OK to resume normal operation.
*/
static void xlnid_io_resume(struct pci_dev *pdev)
{
	struct xlnid_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;

#if 0

	if (adapter->vferr_refcount) {
		e_info(drv, "Resuming after VF err\n");
		adapter->vferr_refcount--;
		return;
	}

#endif
	rtnl_lock();

	if (netif_running(netdev)) {
		xlnid_open(netdev);
	}

	netif_device_attach(netdev);
	rtnl_unlock();
}

#ifdef CONFIG_PM
#ifdef HAVE_PCI_ERROR_HANDLER_RESET_NOTIFY
static void xlnid_io_reset_notify(struct pci_dev *pdev, bool prepare)
{
	struct device *dev = &pdev->dev;

	if (prepare) {
		xlnid_suspend(dev);
	} else {
		xlnid_resume(dev);
	}
}
#endif

#ifdef HAVE_PCI_ERROR_HANDLER_RESET_PREPARE
static void pci_io_reset_prepare(struct pci_dev *pdev)
{
	struct device *dev = &pdev->dev;

	xlnid_suspend(dev);
}

static void pci_io_reset_done(struct pci_dev *pdev)
{
	struct device *dev = &pdev->dev;

	xlnid_resume(dev);
}
#endif
#endif /* CONFIG_PM */
#ifdef HAVE_CONST_STRUCT_PCI_ERROR_HANDLERS
	static const struct pci_error_handlers xlnid_err_handler = {
#else
	static struct pci_error_handlers xlnid_err_handler = {
#endif
		.error_detected = xlnid_io_error_detected,
		.slot_reset = xlnid_io_slot_reset,
#ifdef CONFIG_PM
#ifdef HAVE_PCI_ERROR_HANDLER_RESET_NOTIFY
		.reset_notify = xlnid_io_reset_notify,
#endif
#ifdef HAVE_PCI_ERROR_HANDLER_RESET_PREPARE
		.reset_prepare = pci_io_reset_prepare,
		.reset_done = pci_io_reset_done,
#endif
#endif
		.resume = xlnid_io_resume,
	};
#endif /* HAVE_PCI_ERS */

struct net_device *xlnid_hw_to_netdev(const struct xlnid_hw *hw)
{
	return ((struct xlnid_adapter *)hw->back)->netdev;
}

struct xlnid_msg *xlnid_hw_to_msg(const struct xlnid_hw *hw)
{
	struct xlnid_adapter *adapter =
		container_of(hw, struct xlnid_adapter, hw);
	return (struct xlnid_msg *)&adapter->msg_enable;
}

#ifdef HAVE_RHEL6_SRIOV_CONFIGURE
	static struct pci_driver_rh xlnid_driver_rh = {
		.sriov_configure = xlnid_pci_sriov_configure,
	};
#endif

#ifdef CONFIG_PM
#ifndef USE_LEGACY_PM_SUPPORT
	static const struct dev_pm_ops xlnid_pm_ops = {
		.suspend = xlnid_suspend,
		.resume = xlnid_resume,
		.freeze = xlnid_freeze,
		.thaw = xlnid_thaw,
		.poweroff = xlnid_suspend,
		.restore = xlnid_resume,
	};
#endif /* USE_LEGACY_PM_SUPPORT */
#endif

	static struct pci_driver xlnid_driver = {
		.name     = xlnid_driver_name,
		.id_table = xlnid_pci_tbl,
		.probe    = xlnid_probe,
#ifdef HAVE_CONFIG_HOTPLUG
		.remove   = __devexit_p(xlnid_remove),
#else
		.remove = xlnid_remove,
#endif
#ifdef CONFIG_PM
#ifndef USE_LEGACY_PM_SUPPORT
		.driver = {
			.pm = &xlnid_pm_ops,
		},
#else
		.suspend = xlnid_suspend,
		.resume = xlnid_resume,
#endif /* USE_LEGACY_PM_SUPPORT */
#endif
#ifndef USE_REBOOT_NOTIFIER
		.shutdown = xlnid_shutdown,
#endif
#if defined(HAVE_SRIOV_CONFIGURE)
		.sriov_configure = xlnid_pci_sriov_configure,
#elif defined(HAVE_RHEL6_SRIOV_CONFIGURE)
		.rh_reserved = &xlnid_driver_rh,
#endif /* HAVE_SRIOV_CONFIGURE */
#ifdef HAVE_PCI_ERS
		.err_handler = &xlnid_err_handler
#endif
	};

bool xlnid_is_xlnid(struct pci_dev *pcidev)
{
	if (pci_dev_driver(pcidev) != &xlnid_driver) {
		return false;
	}

	else {
		return true;
	}
}

/**
	xlnid_init_module - Driver Registration Routine

	xlnid_init_module is the first routine called when the driver is
	loaded. All it does is register with the PCI subsystem.
**/
static int __init xlnid_init_module(void)
{
	int ret;

	pr_info("%s - version %s\n", xlnid_driver_string, xlnid_driver_version);
	pr_info("%s\n", xlnid_copyright);

	xlnid_wq = create_singlethread_workqueue(xlnid_driver_name);

	if (!xlnid_wq) {
		pr_err("%s: Failed to create workqueue\n", xlnid_driver_name);
		return -ENOMEM;
	}

#ifdef XLNID_PROCFS

	if (xlnid_procfs_topdir_init()) {
		pr_info("Procfs failed to initialize topdir\n");
	}

#endif

#ifdef HAVE_XLNID_DEBUG_FS
	xlnid_dbg_init(xlnid_driver_name);
#endif /* HAVE_XLNID_DEBUG_FS */

	ret = pci_register_driver(&xlnid_driver);

	if (ret) {
		destroy_workqueue(xlnid_wq);
#ifdef HAVE_XLNID_DEBUG_FS
		xlnid_dbg_exit();
#endif /* HAVE_XLNID_DEBUG_FS */
#ifdef XLNID_PROCFS
		xlnid_procfs_topdir_exit();
#endif
		return ret;
	}

	ret = register_chrdev(fs_major, XEL_PCI_DEV_NAME, &char_fops);

	if (ret < 0) {
		printk("register_chrdev (%s) failed , code is %d \n",
				XEL_PCI_DEV_NAME, ret);
		return ret;
	}

	return ret;
}

module_init(xlnid_init_module);

/**
	xlnid_exit_module - Driver Exit Cleanup Routine

	xlnid_exit_module is called just before the driver is removed
	from memory.
**/
static void __exit xlnid_exit_module(void)
{
	pci_unregister_driver(&xlnid_driver);
#ifdef XLNID_PROCFS
	xlnid_procfs_topdir_exit();
#endif
	destroy_workqueue(xlnid_wq);
#ifdef HAVE_XLNID_DEBUG_FS
	xlnid_dbg_exit();
#endif /* HAVE_XLNID_DEBUG_FS */

	unregister_chrdev(fs_major, XEL_PCI_DEV_NAME);
}

module_exit(xlnid_exit_module);

/* xlnid_main.c */
