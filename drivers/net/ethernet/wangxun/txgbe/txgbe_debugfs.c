// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019 - 2022 Beijing WangXun Technology Co., Ltd. */

#include "txgbe.h"
#include <linux/debugfs.h>
#include <linux/module.h>

static struct dentry *txgbe_dbg_root;
static int txgbe_data_mode;

#define TXGBE_DATA_FUNC(dm)  ((dm) & ~0xFFFF)
#define TXGBE_DATA_ARGS(dm)  ((dm) & 0xFFFF)
enum txgbe_data_func {
	TXGBE_FUNC_NONE        = (0 << 16),
	TXGBE_FUNC_DUMP_BAR    = (1 << 16),
	TXGBE_FUNC_DUMP_RDESC  = (2 << 16),
	TXGBE_FUNC_DUMP_TDESC  = (3 << 16),
	TXGBE_FUNC_FLASH_READ  = (4 << 16),
	TXGBE_FUNC_FLASH_WRITE = (5 << 16),
};

/**
 * data operation
 **/
static ssize_t
txgbe_simple_read_from_pcibar(struct txgbe_adapter *adapter, int res,
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
		val = txgbe_rd32(adapter->io_addr + reg);
		miss = copy_to_user(buf, &val + off, len);
	}

	size = pos - *ppos - miss;
	*ppos += size;

	return size;
}

static ssize_t
txgbe_simple_read_from_flash(struct txgbe_adapter *adapter,
			     void __user *buf, size_t size, loff_t *ppos)
{
	struct txgbe_hw *hw = &adapter->hw;
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
		u32 value = txgbe_rd32(adapter->io_addr + rpos);

		if (hw->flash.ops.write_buffer(hw, rpos >> 2, 1, &value)) {
			ret = size;
			break;
		}
		if (copy_to_user(to, &value, 4) == 4) {
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

static ssize_t
txgbe_simple_write_to_flash(struct txgbe_adapter *adapter,
			    const void __user *from, size_t size, loff_t *ppos, size_t available)
{
	return size;
}

static ssize_t
txgbe_dbg_data_ops_read(struct file *filp, char __user *buffer,
			size_t size, loff_t *ppos)
{
	struct txgbe_adapter *adapter = filp->private_data;
	u32 func = TXGBE_DATA_FUNC(txgbe_data_mode);

	/* rmb for debugfs */
	rmb();

	switch (func) {
	case TXGBE_FUNC_DUMP_BAR: {
		u32 bar = TXGBE_DATA_ARGS(txgbe_data_mode);

		return txgbe_simple_read_from_pcibar(adapter, bar, buffer, size,
					       ppos);
	}
	case TXGBE_FUNC_FLASH_READ: {
		return txgbe_simple_read_from_flash(adapter, buffer, size, ppos);
	}
	case TXGBE_FUNC_DUMP_RDESC: {
		struct txgbe_ring *ring;
		u32 queue = TXGBE_DATA_ARGS(txgbe_data_mode);

		if (queue >= adapter->num_rx_queues)
			return 0;
		queue += VMDQ_P(0) * adapter->queues_per_pool;
		ring = adapter->rx_ring[queue];

		return simple_read_from_buffer(buffer, size, ppos,
			ring->desc, ring->size);
	}
	case TXGBE_FUNC_DUMP_TDESC: {
		struct txgbe_ring *ring;
		u32 queue = TXGBE_DATA_ARGS(txgbe_data_mode);

		if (queue >= adapter->num_tx_queues)
			return 0;
		queue += VMDQ_P(0) * adapter->queues_per_pool;
		ring = adapter->tx_ring[queue];

		return simple_read_from_buffer(buffer, size, ppos,
			ring->desc, ring->size);
	}
	default:
		break;
	}

	return 0;
}

static ssize_t
txgbe_dbg_data_ops_write(struct file *filp,
			 const char __user *buffer,
				     size_t size, loff_t *ppos)
{
	struct txgbe_adapter *adapter = filp->private_data;
	u32 func = TXGBE_DATA_FUNC(txgbe_data_mode);

	/* rmb for debugfs */
	rmb();

	switch (func) {
	case TXGBE_FUNC_FLASH_WRITE: {
		u32 size = TXGBE_DATA_ARGS(txgbe_data_mode);

		if (size > adapter->hw.flash.dword_size << 2)
			size = adapter->hw.flash.dword_size << 2;

		return txgbe_simple_write_to_flash(adapter, buffer, size, ppos, size);
	}
	default:
		break;
	}

	return size;
}

static const struct file_operations txgbe_dbg_data_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = txgbe_dbg_data_ops_read,
	.write = txgbe_dbg_data_ops_write,
};

/**
 * reg_ops operation
 **/
static char txgbe_dbg_reg_ops_buf[256] = "";
static ssize_t
txgbe_dbg_reg_ops_read(struct file *filp, char __user *buffer,
		       size_t count, loff_t *ppos)
{
	struct txgbe_adapter *adapter = filp->private_data;
	char *buf;
	int len;

	/* don't allow partial reads */
	if (*ppos != 0)
		return 0;

	buf = kasprintf(GFP_KERNEL, "%s: mode=0x%08x\n%s\n",
			adapter->netdev->name, txgbe_data_mode,
			txgbe_dbg_reg_ops_buf);
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
txgbe_dbg_reg_ops_write(struct file *filp,
			const char __user *buffer,
				     size_t count, loff_t *ppos)
{
	struct txgbe_adapter *adapter = filp->private_data;
	char *pc = txgbe_dbg_reg_ops_buf;
	int len;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;
	if (count >= sizeof(txgbe_dbg_reg_ops_buf))
		return -ENOSPC;

	len = simple_write_to_buffer(txgbe_dbg_reg_ops_buf,
				     sizeof(txgbe_dbg_reg_ops_buf) - 1,
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
			mode = TXGBE_FUNC_DUMP_BAR;
		} else if (!strncmp(pc, "rdesc", 5)) {
			pc += 5;
			mode = TXGBE_FUNC_DUMP_RDESC;
		} else if (!strncmp(pc, "tdesc", 5)) {
			pc += 5;
			mode = TXGBE_FUNC_DUMP_TDESC;
		} else {
			txgbe_dump(adapter);
		}

		if (mode && 1 == kstrtou16(pc, 16, &args))
			mode |= args;

		txgbe_data_mode = mode;
	} else if (strncmp(pc, "flash", 4) == 0) {
		u32 mode = 0;
		u16 args;

		pc += 5;
		pc += strspn(pc, " \t");
		if (!strncmp(pc, "read", 3)) {
			pc += 4;
			mode = TXGBE_FUNC_FLASH_READ;
		} else if (!strncmp(pc, "write", 5)) {
			pc += 5;
			mode = TXGBE_FUNC_FLASH_WRITE;
		}

		if (mode && 1 == kstrtou16(pc, 16, &args))
			mode |= args;

		txgbe_data_mode = mode;
	} else if (strncmp(txgbe_dbg_reg_ops_buf, "write", 5) == 0) {
		u32 reg, value;
		int cnt;

		cnt = kstrtou32(&txgbe_dbg_reg_ops_buf[5], 16, &reg);
		cnt += kstrtou32(&txgbe_dbg_reg_ops_buf[5] + 4, 16, &value);

		if (cnt == 2) {
			wr32(&adapter->hw, reg, value);
			e_dev_info("write: 0x%08x = 0x%08x\n", reg, value);
		} else {
			e_dev_info("write <reg> <value>\n");
		}
	} else if (strncmp(txgbe_dbg_reg_ops_buf, "read", 4) == 0) {
		u32 reg, value;
		int cnt;

		cnt = kstrtou32(&txgbe_dbg_reg_ops_buf[4], 16, &reg);
		if (cnt == 1) {
			value = rd32(&adapter->hw, reg);
			e_dev_info("read 0x%08x = 0x%08x\n", reg, value);
		} else {
			e_dev_info("read <reg>\n");
		}
	} else {
		e_dev_info("Unknown command %s\n", txgbe_dbg_reg_ops_buf);
		e_dev_info("Available commands:\n");
		e_dev_info("   read <reg>\n");
		e_dev_info("   write <reg> <value>\n");
	}
	return count;
}

static const struct file_operations txgbe_dbg_reg_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read =  txgbe_dbg_reg_ops_read,
	.write = txgbe_dbg_reg_ops_write,
};

/**
 * netdev_ops operation
 **/
static char txgbe_dbg_netdev_ops_buf[256] = "";
static ssize_t
txgbe_dbg_netdev_ops_read(struct file *filp,
			  char __user *buffer,
					 size_t count, loff_t *ppos)
{
	struct txgbe_adapter *adapter = filp->private_data;
	char *buf;
	int len;

	/* don't allow partial reads */
	if (*ppos != 0)
		return 0;

	buf = kasprintf(GFP_KERNEL, "%s: mode=0x%08x\n%s\n",
			adapter->netdev->name, txgbe_data_mode,
			txgbe_dbg_netdev_ops_buf);
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
txgbe_dbg_netdev_ops_write(struct file *filp,
			   const char __user *buffer,
					  size_t count, loff_t *ppos)
{
	struct txgbe_adapter *adapter = filp->private_data;
	int len;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;
	if (count >= sizeof(txgbe_dbg_netdev_ops_buf))
		return -ENOSPC;

	len = simple_write_to_buffer(txgbe_dbg_netdev_ops_buf,
				     sizeof(txgbe_dbg_netdev_ops_buf) - 1,
				     ppos,
				     buffer,
				     count);
	if (len < 0)
		return len;

	txgbe_dbg_netdev_ops_buf[len] = '\0';

	if (strncmp(txgbe_dbg_netdev_ops_buf, "tx_timeout", 10) == 0) {
		adapter->netdev->netdev_ops->ndo_tx_timeout(adapter->netdev, 0);
		e_dev_info("tx_timeout called\n");
	} else {
		e_dev_info("Unknown command: %s\n", txgbe_dbg_netdev_ops_buf);
		e_dev_info("Available commands:\n");
		e_dev_info("    tx_timeout\n");
	}
	return count;
}

static const struct file_operations txgbe_dbg_netdev_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = txgbe_dbg_netdev_ops_read,
	.write = txgbe_dbg_netdev_ops_write,
};

/**
 * txgbe_dbg_adapter_init - setup the debugfs directory for the adapter
 * @adapter: the adapter that is starting up
 **/
void txgbe_dbg_adapter_init(struct txgbe_adapter *adapter)
{
	const char *name = pci_name(adapter->pdev);
	struct dentry *pfile;

	adapter->txgbe_dbg_adapter = debugfs_create_dir(name, txgbe_dbg_root);
	if (!adapter->txgbe_dbg_adapter) {
		e_dev_err("debugfs entry for %s failed\n", name);
		return;
	}

	pfile = debugfs_create_file("data", 0600,
				    adapter->txgbe_dbg_adapter, adapter,
				    &txgbe_dbg_data_ops_fops);
	if (!pfile)
		e_dev_err("debugfs netdev_ops for %s failed\n", name);

	pfile = debugfs_create_file("reg_ops", 0600,
				    adapter->txgbe_dbg_adapter, adapter,
				    &txgbe_dbg_reg_ops_fops);
	if (!pfile)
		e_dev_err("debugfs reg_ops for %s failed\n", name);

	pfile = debugfs_create_file("netdev_ops", 0600,
				    adapter->txgbe_dbg_adapter, adapter,
				    &txgbe_dbg_netdev_ops_fops);
	if (!pfile)
		e_dev_err("debugfs netdev_ops for %s failed\n", name);
}

/**
 * txgbe_dbg_adapter_exit - clear out the adapter's debugfs entries
 * @pf: the pf that is stopping
 **/
void txgbe_dbg_adapter_exit(struct txgbe_adapter *adapter)
{
	debugfs_remove_recursive(adapter->txgbe_dbg_adapter);
	adapter->txgbe_dbg_adapter = NULL;
}

/**
 * txgbe_dbg_init - start up debugfs for the driver
 **/
void txgbe_dbg_init(void)
{
	txgbe_dbg_root = debugfs_create_dir(txgbe_driver_name, NULL);
	if (!txgbe_dbg_root)
		pr_err("init of debugfs failed\n");
}

/**
 * txgbe_dbg_exit - clean out the driver's debugfs entries
 **/
void txgbe_dbg_exit(void)
{
	debugfs_remove_recursive(txgbe_dbg_root);
}

/**
 * txgbe_dump - Print registers, tx-rings and rx-rings
 **/
void txgbe_dump(struct txgbe_adapter *adapter)
{
	dev_info(&adapter->pdev->dev, "skip dump\n");
}
