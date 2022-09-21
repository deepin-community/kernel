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
 *
 */


#include "ngbe.h"

#ifdef HAVE_NGBE_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/module.h>

static struct dentry *ngbe_dbg_root;
static int ngbe_data_mode;

#define NGBE_DATA_FUNC(dm)  ((dm) & ~0xFFFF)
#define NGBE_DATA_ARGS(dm)  ((dm) & 0xFFFF)
enum ngbe_data_func {
	NGBE_FUNC_NONE        = (0 << 16),
	NGBE_FUNC_DUMP_BAR    = (1 << 16),
	NGBE_FUNC_DUMP_RDESC  = (2 << 16),
	NGBE_FUNC_DUMP_TDESC  = (3 << 16),
	NGBE_FUNC_FLASH_READ  = (4 << 16),
	NGBE_FUNC_FLASH_WRITE = (5 << 16),
};

/**
 * data operation
 **/
ssize_t
ngbe_simple_read_from_pcibar(struct ngbe_adapter *adapter, int res,
		void __user *buf, size_t size, loff_t *ppos)
{
	loff_t pos = *ppos;
	u32 miss, len, limit = pci_resource_len(adapter->pdev, res);

	if (pos < 0)
		return 0;

	limit = (pos + size <= limit ? pos + size : limit);
	for (miss = 0; pos < limit && !miss; buf += len, pos += len) {
		u32 val = 0, reg = round_down(pos, 4);
		u32 off = pos - reg;

		len = (reg + 4 <= limit ? 4 - off : 4 - off - (limit - reg - 4));
		val = ngbe_rd32(adapter->io_addr + reg);
		miss = copy_to_user(buf, &val + off, len);
	}

	size = pos - *ppos - miss;
	*ppos += size;

	return size;
}

ssize_t
ngbe_simple_read_from_flash(struct ngbe_adapter *adapter,
		void __user *buf, size_t size, loff_t *ppos)
{
	struct ngbe_hw *hw = &adapter->hw;
	loff_t pos = *ppos;
	size_t ret = 0;
	loff_t rpos, rtail;
	void __user *to = buf;
	size_t available = adapter->hw.flash.dword_size << 2;

	if (pos < 0)
		return -EINVAL;
	if (pos >= available || !size)
		return 0;
	if (size > available - pos)
		size = available - pos;

	rpos = round_up(pos, 4);
	rtail = round_down(pos + size, 4);
	if (rtail < rpos)
		return 0;

	to += rpos - pos;
	while (rpos <= rtail) {
		u32 value = ngbe_rd32(adapter->io_addr + rpos);
		if (TCALL(hw, flash.ops.write_buffer, rpos>>2, 1, &value)) {
			ret = size;
			break;
		}
		if (4 == copy_to_user(to, &value, 4)) {
			ret = size;
			break;
		}
		to += 4;
		rpos += 4;
	}

	if (ret == size)
		return -EFAULT;
	size -= ret;
	*ppos = pos + size;
	return size;
}

ssize_t
ngbe_simple_write_to_flash(struct ngbe_adapter *adapter,
	const void __user *from, size_t size, loff_t *ppos, size_t available)
{
	return size;
}

static ssize_t
ngbe_dbg_data_ops_read(struct file *filp, char __user *buffer,
				    size_t size, loff_t *ppos)
{
	struct ngbe_adapter *adapter = filp->private_data;
	u32 func = NGBE_DATA_FUNC(ngbe_data_mode);

	rmb();

	switch (func) {
	case NGBE_FUNC_DUMP_BAR: {
		u32 bar = NGBE_DATA_ARGS(ngbe_data_mode);

		return ngbe_simple_read_from_pcibar(adapter, bar, buffer, size,
					       ppos);
		break;
	}
	case NGBE_FUNC_FLASH_READ: {
		return ngbe_simple_read_from_flash(adapter, buffer, size, ppos);
		break;
	}
	case NGBE_FUNC_DUMP_RDESC: {
		struct ngbe_ring *ring;
		u32 queue = NGBE_DATA_ARGS(ngbe_data_mode);

		if (queue >= adapter->num_rx_queues)
			return 0;
		queue += VMDQ_P(0) * adapter->queues_per_pool;
		ring = adapter->rx_ring[queue];

		return simple_read_from_buffer(buffer, size, ppos,
			ring->desc, ring->size);
		break;
	}
	case NGBE_FUNC_DUMP_TDESC: {
		struct ngbe_ring *ring;
		u32 queue = NGBE_DATA_ARGS(ngbe_data_mode);

		if (queue >= adapter->num_tx_queues)
			return 0;
		queue += VMDQ_P(0) * adapter->queues_per_pool;
		ring = adapter->tx_ring[queue];

		return simple_read_from_buffer(buffer, size, ppos,
			ring->desc, ring->size);
		break;
	}
	default:
		break;
	}

	return 0;
}

static ssize_t
ngbe_dbg_data_ops_write(struct file *filp,
				     const char __user *buffer,
				     size_t size, loff_t *ppos)
{
	struct ngbe_adapter *adapter = filp->private_data;
	u32 func = NGBE_DATA_FUNC(ngbe_data_mode);

	rmb();

	switch (func) {
	case NGBE_FUNC_FLASH_WRITE: {
		u32 size = NGBE_DATA_ARGS(ngbe_data_mode);

		if (size > adapter->hw.flash.dword_size << 2)
			size = adapter->hw.flash.dword_size << 2;

		return ngbe_simple_write_to_flash(adapter, buffer, size, ppos, size);
		break;
	}
	default:
		break;
	}

	return size;
}
static struct file_operations ngbe_dbg_data_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ngbe_dbg_data_ops_read,
	.write = ngbe_dbg_data_ops_write,
};

/**
 * reg_ops operation
 **/
static char ngbe_dbg_reg_ops_buf[256] = "";
static ssize_t
ngbe_dbg_reg_ops_read(struct file *filp, char __user *buffer,
				    size_t count, loff_t *ppos)
{
	struct ngbe_adapter *adapter = filp->private_data;
	char *buf;
	int len;

	/* don't allow partial reads */
	if (*ppos != 0)
		return 0;

	buf = kasprintf(GFP_KERNEL, "%s: mode=0x%08x\n%s\n",
			adapter->netdev->name, ngbe_data_mode,
			ngbe_dbg_reg_ops_buf);
	if (!buf)
		return -ENOMEM;

	if (count < strlen(buf)) {
		kfree(buf);
		return -ENOSPC;
	}

	len = simple_read_from_buffer(buffer, count, ppos, buf, strlen(buf));

	kfree(buf);
	return len;
}

static ssize_t
ngbe_dbg_reg_ops_write(struct file *filp,
				     const char __user *buffer,
				     size_t count, loff_t *ppos)
{
	struct ngbe_adapter *adapter = filp->private_data;
	char *pc = ngbe_dbg_reg_ops_buf;
	int len;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;
	if (count >= sizeof(ngbe_dbg_reg_ops_buf))
		return -ENOSPC;

	len = simple_write_to_buffer(ngbe_dbg_reg_ops_buf,
				     sizeof(ngbe_dbg_reg_ops_buf)-1,
				     ppos,
				     buffer,
				     count);
	if (len < 0)
		return len;

	pc[len] = '\0';

	if (strncmp(pc, "dump", 4) == 0) {
		u32 mode = 0;
		u16 args;

		pc += 4;
		pc += strspn(pc, " \t");

		if (!strncmp(pc, "bar", 3)) {
			pc += 3;
			mode = NGBE_FUNC_DUMP_BAR;
		} else if (!strncmp(pc, "rdesc", 5)) {
			pc += 5;
			mode = NGBE_FUNC_DUMP_RDESC;
		} else if (!strncmp(pc, "tdesc", 5)) {
			pc += 5;
			mode = NGBE_FUNC_DUMP_TDESC;
		} else {
			ngbe_dump(adapter);
		}

		if (mode && 1 == sscanf(pc, "%hu", &args)) {
			mode |= args;
		}

		ngbe_data_mode = mode;
	} else if (strncmp(pc, "flash", 4) == 0) {
		u32 mode = 0;
		u16 args;

		pc += 5;
		pc += strspn(pc, " \t");
		if (!strncmp(pc, "read", 3)) {
			pc += 4;
			mode = NGBE_FUNC_FLASH_READ;
		} else if (!strncmp(pc, "write", 5)) {
			pc += 5;
			mode = NGBE_FUNC_FLASH_WRITE;
		}

		if (mode && 1 == sscanf(pc, "%hu", &args)) {
			mode |= args;
		}

		ngbe_data_mode = mode;
	} else if (strncmp(ngbe_dbg_reg_ops_buf, "write", 5) == 0) {
		u32 reg, value;
		int cnt;
		cnt = sscanf(&ngbe_dbg_reg_ops_buf[5], "%x %x", &reg, &value);
		if (cnt == 2) {
			wr32(&adapter->hw, reg, value);
			e_dev_info("write: 0x%08x = 0x%08x\n", reg, value);
		} else {
			e_dev_info("write <reg> <value>\n");
		}
	} else if (strncmp(ngbe_dbg_reg_ops_buf, "read", 4) == 0) {
		u32 reg, value;
		int cnt;
		cnt = sscanf(&ngbe_dbg_reg_ops_buf[4], "%x", &reg);
		if (cnt == 1) {
			value = rd32(&adapter->hw, reg);
			e_dev_info("read 0x%08x = 0x%08x\n", reg, value);
		} else {
			e_dev_info("read <reg>\n");
		}
	} else {
		e_dev_info("Unknown command %s\n", ngbe_dbg_reg_ops_buf);
		e_dev_info("Available commands:\n");
		e_dev_info("   read <reg>\n");
		e_dev_info("   write <reg> <value>\n");
	}
	return count;
}

static const struct file_operations ngbe_dbg_reg_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read =  ngbe_dbg_reg_ops_read,
	.write = ngbe_dbg_reg_ops_write,
};

/**
 * netdev_ops operation
 **/
static char ngbe_dbg_netdev_ops_buf[256] = "";
static ssize_t
ngbe_dbg_netdev_ops_read(struct file *filp,
					 char __user *buffer,
					 size_t count, loff_t *ppos)
{
	struct ngbe_adapter *adapter = filp->private_data;
	char *buf;
	int len;

	/* don't allow partial reads */
	if (*ppos != 0)
		return 0;

	buf = kasprintf(GFP_KERNEL, "%s: mode=0x%08x\n%s\n",
			adapter->netdev->name, ngbe_data_mode,
			ngbe_dbg_netdev_ops_buf);
	if (!buf)
		return -ENOMEM;

	if (count < strlen(buf)) {
		kfree(buf);
		return -ENOSPC;
	}

	len = simple_read_from_buffer(buffer, count, ppos, buf, strlen(buf));

	kfree(buf);
	return len;
}

static ssize_t
ngbe_dbg_netdev_ops_write(struct file *filp,
					  const char __user *buffer,
					  size_t count, loff_t *ppos)
{
	struct ngbe_adapter *adapter = filp->private_data;
	int len;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;
	if (count >= sizeof(ngbe_dbg_netdev_ops_buf))
		return -ENOSPC;

	len = simple_write_to_buffer(ngbe_dbg_netdev_ops_buf,
				     sizeof(ngbe_dbg_netdev_ops_buf)-1,
				     ppos,
				     buffer,
				     count);
	if (len < 0)
		return len;

	ngbe_dbg_netdev_ops_buf[len] = '\0';

	if (strncmp(ngbe_dbg_netdev_ops_buf, "tx_timeout", 10) == 0) {
#if defined(HAVE_TX_TIMEOUT_TXQUEUE)
		adapter->netdev->netdev_ops->ndo_tx_timeout(adapter->netdev, 0);
#elif defined(HAVE_NET_DEVICE_OPS) 
		adapter->netdev->netdev_ops->ndo_tx_timeout(adapter->netdev);
#else
		adapter->netdev->tx_timeout(adapter->netdev);
#endif /* HAVE_NET_DEVICE_OPS */
		e_dev_info("tx_timeout called\n");
	} else {
		e_dev_info("Unknown command: %s\n", ngbe_dbg_netdev_ops_buf);
		e_dev_info("Available commands:\n");
		e_dev_info("    tx_timeout\n");
	}
	return count;
}

static struct file_operations ngbe_dbg_netdev_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ngbe_dbg_netdev_ops_read,
	.write = ngbe_dbg_netdev_ops_write,
};

/**
 * ngbe_dbg_adapter_init - setup the debugfs directory for the adapter
 * @adapter: the adapter that is starting up
 **/
void ngbe_dbg_adapter_init(struct ngbe_adapter *adapter)
{
	const char *name = pci_name(adapter->pdev);
	struct dentry *pfile;

	adapter->ngbe_dbg_adapter = debugfs_create_dir(name, ngbe_dbg_root);
	if (!adapter->ngbe_dbg_adapter) {
		e_dev_err("debugfs entry for %s failed\n", name);
		return;
	}

	pfile = debugfs_create_file("data", 0600,
				    adapter->ngbe_dbg_adapter, adapter,
				    &ngbe_dbg_data_ops_fops);
	if (!pfile)
		e_dev_err("debugfs netdev_ops for %s failed\n", name);

	pfile = debugfs_create_file("reg_ops", 0600,
				    adapter->ngbe_dbg_adapter, adapter,
				    &ngbe_dbg_reg_ops_fops);
	if (!pfile)
		e_dev_err("debugfs reg_ops for %s failed\n", name);

	pfile = debugfs_create_file("netdev_ops", 0600,
				    adapter->ngbe_dbg_adapter, adapter,
				    &ngbe_dbg_netdev_ops_fops);
	if (!pfile)
		e_dev_err("debugfs netdev_ops for %s failed\n", name);
}

/**
 * ngbe_dbg_adapter_exit - clear out the adapter's debugfs entries
 * @pf: the pf that is stopping
 **/
void ngbe_dbg_adapter_exit(struct ngbe_adapter *adapter)
{
	if (adapter->ngbe_dbg_adapter)
		debugfs_remove_recursive(adapter->ngbe_dbg_adapter);
	adapter->ngbe_dbg_adapter = NULL;
}

/**
 * ngbe_dbg_init - start up debugfs for the driver
 **/
void ngbe_dbg_init(void)
{
	ngbe_dbg_root = debugfs_create_dir(ngbe_driver_name, NULL);
	if (ngbe_dbg_root == NULL)
		pr_err("init of debugfs failed\n");
}

/**
 * ngbe_dbg_exit - clean out the driver's debugfs entries
 **/
void ngbe_dbg_exit(void)
{
	debugfs_remove_recursive(ngbe_dbg_root);
}

#endif /* HAVE_NGBE_DEBUG_FS */

struct ngbe_reg_info {
	u32 offset;
	u32 length;
	char *name;
};

static struct ngbe_reg_info ngbe_reg_info_tbl[] = {

	/* General Registers */
	{NGBE_CFG_PORT_CTL, 1, "CTRL"},
	{NGBE_CFG_PORT_ST, 1, "STATUS"},

	/* RX Registers */
	{NGBE_PX_RR_CFG(0), 1, "SRRCTL"},
	{NGBE_PX_RR_RP(0), 1, "RDH"},
	{NGBE_PX_RR_WP(0), 1, "RDT"},
	{NGBE_PX_RR_CFG(0), 1, "RXDCTL"},
	{NGBE_PX_RR_BAL(0), 1, "RDBAL"},
	{NGBE_PX_RR_BAH(0), 1, "RDBAH"},

	/* TX Registers */
	{NGBE_PX_TR_BAL(0), 1, "TDBAL"},
	{NGBE_PX_TR_BAH(0), 1, "TDBAH"},
	{NGBE_PX_TR_RP(0), 1, "TDH"},
	{NGBE_PX_TR_WP(0), 1, "TDT"},
	{NGBE_PX_TR_CFG(0), 1, "TXDCTL"},

	/* MACVLAN */
	{NGBE_PSR_MAC_SWC_VM, 128, "PSR_MAC_SWC_VM"},
	{NGBE_PSR_MAC_SWC_AD_L, 32, "PSR_MAC_SWC_AD"},
	{NGBE_PSR_VLAN_TBL(0),  128, "PSR_VLAN_TBL"},

	/* List Terminator */
	{ .name = NULL }
};

/**
 * ngbe_regdump - register printout routine
 **/
static void
ngbe_regdump(struct ngbe_hw *hw, struct ngbe_reg_info *reg_info)
{
	int i, n = 0;
	u32 buffer[32*8];

	switch (reg_info->offset) {
	case NGBE_PSR_MAC_SWC_AD_L:
		for (i = 0; i < reg_info->length; i++) {
			wr32(hw, NGBE_PSR_MAC_SWC_IDX, i);
			buffer[n++] =
				rd32(hw, NGBE_PSR_MAC_SWC_AD_H);
			buffer[n++] =
				rd32(hw, NGBE_PSR_MAC_SWC_AD_L);
		}
		break;
	default:
		for (i = 0; i < reg_info->length; i++) {
			buffer[n++] = rd32(hw,
				reg_info->offset + 4*i);
		}
		break;
	}
#if 0
	for (i = 0; n && i < 32; i++) {
		pr_info("%-20s[%02x-%02x]", reg_info->name, i*8, i*8 + 7);
		for (j = 0; n && j < 8; j++, n--)
			pr_cont(" %08x", buffer[i*8 + j]);
		pr_cont("\n");
	}
#endif
	BUG_ON(n);
}

/**
 * ngbe_dump - Print registers, tx-rings and rx-rings
 **/
void ngbe_dump(struct ngbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct ngbe_hw *hw = &adapter->hw;
	struct ngbe_reg_info *reg_info;
	int n = 0;
	struct ngbe_ring *tx_ring;
	struct ngbe_tx_buffer *tx_buffer;
	union ngbe_tx_desc *tx_desc;
	struct my_u0 { u64 a; u64 b; } *u0;
	struct ngbe_ring *rx_ring;
#ifndef CONFIG_NGBE_DISABLE_PACKET_SPLIT
	union ngbe_rx_desc *rx_desc;
	struct ngbe_rx_buffer *rx_buffer_info;
	u32 staterr;
#endif
	int i = 0;

	if (!netif_msg_hw(adapter))
		return;

	/* Print Registers */
	dev_info(&adapter->pdev->dev, "Register Dump\n");
	pr_info(" Register Name   Value\n");
	for (reg_info = ngbe_reg_info_tbl; reg_info->name; reg_info++) {
		ngbe_regdump(hw, reg_info);
	}

	/* Print TX Ring Summary */
	if (!netdev || !netif_running(netdev))
		return;

	dev_info(&adapter->pdev->dev, "TX Rings Summary\n");
	pr_info(" %s     %s              %s        %s\n",
		"Queue [NTU] [NTC] [bi(ntc)->dma  ]",
		"leng", "ntw", "timestamp");
	for (n = 0; n < adapter->num_tx_queues; n++) {
		tx_ring = adapter->tx_ring[n];
		tx_buffer = &tx_ring->tx_buffer_info[tx_ring->next_to_clean];
		pr_info(" %5d %5X %5X %016llX %08X %p %016llX\n",
			   n, tx_ring->next_to_use, tx_ring->next_to_clean,
			   (u64)dma_unmap_addr(tx_buffer, dma),
			   dma_unmap_len(tx_buffer, len),
			   tx_buffer->next_to_watch,
			   (u64)tx_buffer->time_stamp);
	}

	/* Print TX Rings */
	if (!netif_msg_tx_done(adapter))
		goto rx_ring_summary;

	dev_info(&adapter->pdev->dev, "TX Rings Dump\n");

	/* Transmit Descriptor Formats
	 *
	 * Transmit Descriptor (Read)
	 *   +--------------------------------------------------------------+
	 * 0 |         Buffer Address [63:0]                                |
	 *   +--------------------------------------------------------------+
	 * 8 |PAYLEN  |POPTS|CC|IDX  |STA  |DCMD  |DTYP |MAC  |RSV  |DTALEN |
	 *   +--------------------------------------------------------------+
	 *   63     46 45 40 39 38 36 35 32 31  24 23 20 19 18 17 16 15     0
	 *
	 * Transmit Descriptor (Write-Back)
	 *   +--------------------------------------------------------------+
	 * 0 |                          RSV [63:0]                          |
	 *   +--------------------------------------------------------------+
	 * 8 |            RSV           |  STA  |           RSV             |
	 *   +--------------------------------------------------------------+
	 *   63                       36 35   32 31                         0
	 */

	for (n = 0; n < adapter->num_tx_queues; n++) {
		tx_ring = adapter->tx_ring[n];
		pr_info("------------------------------------\n");
		pr_info("TX QUEUE INDEX = %d\n", tx_ring->queue_index);
		pr_info("------------------------------------\n");
		pr_info("%s%s    %s              %s        %s          %s\n",
			"T [desc]     [address 63:0  ] ",
			"[PlPOIdStDDt Ln] [bi->dma       ] ",
			"leng", "ntw", "timestamp", "bi->skb");

		for (i = 0; tx_ring->desc && (i < tx_ring->count); i++) {
			tx_desc = NGBE_TX_DESC(tx_ring, i);
			tx_buffer = &tx_ring->tx_buffer_info[i];
			u0 = (struct my_u0 *)tx_desc;
			if (dma_unmap_len(tx_buffer, len) > 0) {
				pr_info("T [0x%03X]    %016llX %016llX %016llX "
					"%08X %p %016llX %p",
					i,
					le64_to_cpu(u0->a),
					le64_to_cpu(u0->b),
					(u64)dma_unmap_addr(tx_buffer, dma),
					dma_unmap_len(tx_buffer, len),
					tx_buffer->next_to_watch,
					(u64)tx_buffer->time_stamp,
					tx_buffer->skb);
				if (i == tx_ring->next_to_use &&
					i == tx_ring->next_to_clean)
					pr_cont(" NTC/U\n");
				else if (i == tx_ring->next_to_use)
					pr_cont(" NTU\n");
				else if (i == tx_ring->next_to_clean)
					pr_cont(" NTC\n");
				else
					pr_cont("\n");

				if (netif_msg_pktdata(adapter) &&
				    tx_buffer->skb)
					print_hex_dump(KERN_INFO, "",
						DUMP_PREFIX_ADDRESS, 16, 1,
						tx_buffer->skb->data,
						dma_unmap_len(tx_buffer, len),
						true);
			}
		}
	}

	/* Print RX Rings Summary */
rx_ring_summary:
	dev_info(&adapter->pdev->dev, "RX Rings Summary\n");
	pr_info("Queue [NTU] [NTC]\n");
	for (n = 0; n < adapter->num_rx_queues; n++) {
		rx_ring = adapter->rx_ring[n];
		pr_info("%5d %5X %5X\n",
			n, rx_ring->next_to_use, rx_ring->next_to_clean);
	}

	/* Print RX Rings */
	if (!netif_msg_rx_status(adapter))
		return;

	dev_info(&adapter->pdev->dev, "RX Rings Dump\n");

	/* Receive Descriptor Formats
	 *
	 * Receive Descriptor (Read)
	 *    63                                           1        0
	 *    +-----------------------------------------------------+
	 *  0 |       Packet Buffer Address [63:1]           |A0/NSE|
	 *    +----------------------------------------------+------+
	 *  8 |       Header Buffer Address [63:1]           |  DD  |
	 *    +-----------------------------------------------------+
	 *
	 *
	 * Receive Descriptor (Write-Back)
	 *
	 *   63       48 47    32 31  30      21 20 17 16   4 3     0
	 *   +------------------------------------------------------+
	 * 0 |RSS / Frag Checksum|SPH| HDR_LEN  |RSC- |Packet|  RSS |
	 *   |/ RTT / PCoE_PARAM |   |          | CNT | Type | Type |
	 *   |/ Flow Dir Flt ID  |   |          |     |      |      |
	 *   +------------------------------------------------------+
	 * 8 | VLAN Tag | Length |Extended Error| Xtnd Status/NEXTP |
	 *   +------------------------------------------------------+
	 *   63       48 47    32 31          20 19                 0
	 */

#ifndef CONFIG_NGBE_DISABLE_PACKET_SPLIT
	for (n = 0; n < adapter->num_rx_queues; n++) {
		rx_ring = adapter->rx_ring[n];
		pr_info("------------------------------------\n");
		pr_info("RX QUEUE INDEX = %d\n", rx_ring->queue_index);
		pr_info("------------------------------------\n");
		pr_info("%s%s%s",
			"R  [desc]      [ PktBuf     A0] ",
			"[  HeadBuf   DD] [bi->dma       ] [bi->skb       ] ",
			"<-- Adv Rx Read format\n");
		pr_info("%s%s%s",
			"RWB[desc]      [PcsmIpSHl PtRs] ",
			"[vl er S cks ln] ---------------- [bi->skb       ] ",
			"<-- Adv Rx Write-Back format\n");

		for (i = 0; i < rx_ring->count; i++) {
			rx_buffer_info = &rx_ring->rx_buffer_info[i];
			rx_desc = NGBE_RX_DESC(rx_ring, i);
			u0 = (struct my_u0 *)rx_desc;
			staterr = le32_to_cpu(rx_desc->wb.upper.status_error);
			if (staterr & NGBE_RXD_STAT_DD) {
				/* Descriptor Done */
				pr_info("RWB[0x%03X]     %016llX "
					"%016llX ---------------- %p", i,
					le64_to_cpu(u0->a),
					le64_to_cpu(u0->b),
					rx_buffer_info->skb);
			} else {
				pr_info("R  [0x%03X]     %016llX "
					"%016llX %016llX %p", i,
					le64_to_cpu(u0->a),
					le64_to_cpu(u0->b),
					(u64)rx_buffer_info->page_dma,
					rx_buffer_info->skb);

				if (netif_msg_pktdata(adapter) &&
				    rx_buffer_info->page_dma) {
					print_hex_dump(KERN_INFO, "",
					   DUMP_PREFIX_ADDRESS, 16, 1,
					   page_address(rx_buffer_info->page) +
						    rx_buffer_info->page_offset,
					   ngbe_rx_bufsz(rx_ring), true);
				}
			}

			if (i == rx_ring->next_to_use)
				pr_cont(" NTU\n");
			else if (i == rx_ring->next_to_clean)
				pr_cont(" NTC\n");
			else
				pr_cont("\n");

		}
	}
#endif
}

