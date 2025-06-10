// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#include <linux/debugfs.h>
#include <linux/module.h>
#include "rnpm.h"
#include "rnpm_mbx_fw.h"

#ifndef bus_to_virt
#define bus_to_virt phys_to_virt
#endif

#ifdef CONFIG_DEBUG_FS
static struct dentry *rnpm_dbg_root;

static char rnpm_dbg_reg_ops_buf[256] = "";

#ifndef NO_CSL_DEBUG
#ifndef bus_to_virt
#define bus_to_virt phys_to_virt
#endif
static int rnpm_dbg_csl_mmap(struct file *filp,
			     struct vm_area_struct *vma);

static int rnpm_dbg_csl_open(struct inode *inode, struct file *filp)
{
	void *dma_buf = NULL;
	dma_addr_t dma_phy;
	int err, bytes = 4096;
	struct rnpm_adapter *adapter;
	const char *name;
	struct rnpm_hw *hw;

	if (inode->i_private)
		filp->private_data = inode->i_private;
	else
		return -EIO;

	adapter = filp->private_data;
	if (adapter == NULL)
		return -EIO;

	if (adapter->pf_adapter->csl_dma_buf != NULL)
		return 0;
	hw = &adapter->hw;
	name = adapter->name;

	dma_buf = dma_alloc_coherent(&hw->pdev->dev, bytes, &dma_phy,
				     GFP_ATOMIC);
	if (!dma_buf)
		return -ENOMEM;
	memset(dma_buf, 0, bytes);
	adapter->pf_adapter->csl_dma_buf = dma_buf;
	adapter->pf_adapter->csl_dma_phy = dma_phy;
	adapter->pf_adapter->csl_dma_size = bytes;
	err = rnpm_mbx_ddr_csl_enable(hw, 1, dma_phy, bytes);
	if (err) {
		dma_free_coherent(&hw->pdev->dev, bytes, dma_buf, dma_phy);
		adapter->pf_adapter->csl_dma_buf = NULL;
		return -EIO;
	}

	return 0;
}

static int rnpm_dbg_csl_release(struct inode *inode, struct file *filp)
{
	struct rnpm_adapter *adapter = filp->private_data;
	struct rnpm_hw *hw = &adapter->hw;

	if (adapter->pf_adapter->csl_dma_buf) {
		rnpm_mbx_ddr_csl_enable(hw, 0, 0, 0);
		dma_free_coherent(&hw->pdev->dev,
				  adapter->pf_adapter->csl_dma_size,
				  adapter->pf_adapter->csl_dma_buf,
				  adapter->pf_adapter->csl_dma_phy);
		adapter->pf_adapter->csl_dma_buf = NULL;
	}

	return 0;
}

static int rnpm_dbg_csl_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long length;
	struct rnpm_adapter *adapter = filp->private_data;
	void *dma_buf = adapter->pf_adapter->csl_dma_buf;
	dma_addr_t dma_phy = adapter->pf_adapter->csl_dma_phy;
	int dma_bytes = adapter->pf_adapter->csl_dma_size;
	int ret = 0;

	length = (unsigned long)(vma->vm_end - vma->vm_start);

	if (length > dma_bytes)
		return -EIO;
	if (vma->vm_pgoff == 0) {
		ret = dma_mmap_coherent(&adapter->pdev->dev, vma, dma_buf,
					dma_phy, length);
	} else {
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		ret = remap_pfn_range(
			vma, vma->vm_start,
			PFN_DOWN(virt_to_phys(bus_to_virt(dma_phy))) +
				vma->vm_pgoff,
			length, vma->vm_page_prot);
	}

	if (ret < 0) {
		netdev_err(adapter->netdev, "%s: remap failed (%d)\n",
			   __func__, ret);
		return ret;
	}

	return 0;
}

static const struct file_operations rnpm_dbg_csl_fops = {
	.owner = THIS_MODULE,
	.open = rnpm_dbg_csl_open,
	.release = rnpm_dbg_csl_release,
	.mmap = rnpm_dbg_csl_mmap,
};
#endif

static ssize_t rnpm_dbg_eth_info_read(struct file *filp,
				      char __user *buffer, size_t count,
				      loff_t *ppos)
{
	struct rnpm_adapter *adapter = filp->private_data;
	char *buf = NULL;
	int len;

	if (adapter == NULL)
		return -EIO;

	if (*ppos != 0)
		return 0;

	buf = kasprintf(GFP_KERNEL, "bd:%d port%d %s %s\n",
			adapter->pf_adapter->bd_number,
			adapter->hw.port_idx, adapter->netdev->name,
			pci_name(adapter->pdev));
	if (!buf)
		return -ENOMEM;

	if (count < strlen(buf)) {
		kfree(buf);
		return -ENOSPC;
	}

	len = simple_read_from_buffer(buffer, count, ppos, buf,
				      strlen(buf));

	kfree(buf);
	return len;
}

static const struct file_operations rnpm_dbg_eth_info_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = rnpm_dbg_eth_info_read,
};

static ssize_t rnpm_dbg_mbx_cookies_info_read(struct file *filp,
					      char __user *buffer,
					      size_t count, loff_t *ppos)
{
	struct rnpm_adapter *adapter = filp->private_data;
	struct rnpm_pf_adapter *pf_adapter = NULL;
	char *buf = NULL;
	int len, i;
	struct mbx_req_cookie_pool *cookie_pool = NULL;
	struct mbx_req_cookie *cookie;
	int free_cnt = 0, wait_timout_cnt = 0, alloced_cnt = 0;

	if (adapter == NULL)
		return -EIO;

	pf_adapter = adapter->pf_adapter;
	cookie_pool = &(pf_adapter->cookie_pool);

	/* don't allow partial reads */
	if (*ppos != 0)
		return 0;
	for (i = 0; i < MAX_COOKIES_ITEMS; i++) {
		cookie = &(cookie_pool->cookies[i]);
		if (cookie->stat == COOKIE_FREE)
			free_cnt++;
		else if (cookie->stat == COOKIE_FREE_WAIT_TIMEOUT)
			wait_timout_cnt++;
		else if (cookie->stat == COOKIE_ALLOCED)
			alloced_cnt++;
	}

	buf = kasprintf(
		GFP_KERNEL,
		"pool items:cur:%d total: %d. free:%d wait_free:%d alloced:%d\n",
		cookie_pool->next_idx, MAX_COOKIES_ITEMS, free_cnt,
		wait_timout_cnt, alloced_cnt);
	if (!buf)
		return -ENOMEM;

	if (count < strlen(buf)) {
		kfree(buf);
		return -ENOSPC;
	}

	len = simple_read_from_buffer(buffer, count, ppos, buf,
				      strlen(buf));
	kfree(buf);
	return len;
}

static const struct file_operations rnpm_dbg_mbx_cookies_info_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = rnpm_dbg_mbx_cookies_info_read,
};

/**
 * rnpm_dbg_reg_ops_read - read for reg_ops datum
 * @filp: the opened file
 * @buffer: where to write the data for the user to read
 * @count: the size of the user's buffer
 * @ppos: file position offset
 **/
static ssize_t rnpm_dbg_reg_ops_read(struct file *filp,
				     char __user *buffer, size_t count,
				     loff_t *ppos)
{
	struct rnpm_adapter *adapter = filp->private_data;
	char *buf;
	int len;

	/* don't allow partial reads */
	if (*ppos != 0)
		return 0;

	buf = kasprintf(GFP_KERNEL, "%s: %s\n", adapter->name,
			rnpm_dbg_reg_ops_buf);
	if (!buf)
		return -ENOMEM;

	if (count < strlen(buf)) {
		kfree(buf);
		return -ENOSPC;
	}

	len = simple_read_from_buffer(buffer, count, ppos, buf,
				      strlen(buf));
	kfree(buf);
	return len;
}

/**
 * rnpm_dbg_reg_ops_write - write into reg_ops datum
 * @filp: the opened file
 * @buffer: where to find the user's data
 * @count: the length of the user's data
 * @ppos: file position offset
 **/
static ssize_t rnpm_dbg_reg_ops_write(struct file *filp,
				      const char __user *buffer,
				      size_t count, loff_t *ppos)
{
	struct rnpm_adapter *adapter = filp->private_data;
	struct rnpm_hw *hw = &adapter->hw;
	int len;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;
	if (count >= sizeof(rnpm_dbg_reg_ops_buf))
		return -ENOSPC;

	len = simple_write_to_buffer(rnpm_dbg_reg_ops_buf,
				     sizeof(rnpm_dbg_reg_ops_buf) - 1,
				     ppos, buffer, count);
	if (len < 0)
		return len;

	rnpm_dbg_reg_ops_buf[len] = '\0';
	if (strncmp(rnpm_dbg_reg_ops_buf, "write", 5) == 0) {
		u32 reg, value;
		int cnt;

		cnt = sscanf(&rnpm_dbg_reg_ops_buf[5], "%x %x", &reg,
			     &value);
		if (cnt == 2) {
			if (reg >= 0x30000000) {
				rnpm_mbx_reg_write(hw, reg, value);
			} else {
				rnpm_wr_reg(hw->hw_addr + reg, value);
				value = rnpm_rd_reg(hw->hw_addr + reg);
			}
			e_dev_info("write: 0x%08x = 0x%08x\n", reg, value);
		} else {
			e_dev_info("write <reg> <value>\n");
		}
	} else if (strncmp(rnpm_dbg_reg_ops_buf, "read", 4) == 0) {
		u32 reg, value;
		int cnt;

		cnt = sscanf(&rnpm_dbg_reg_ops_buf[4], "%x", &reg);
		if (cnt == 1) {
			if (reg >= 0x20000000)
				value = rnpm_mbx_fw_reg_read(hw, reg);
			else
				value = rnpm_rd_reg(hw->hw_addr + reg);
			snprintf(rnpm_dbg_reg_ops_buf,
				 sizeof(rnpm_dbg_reg_ops_buf),
				 "0x%08x: 0x%08x", reg, value);
			e_dev_info("read 0x%08x = 0x%08x\n", reg, value);
		} else {
			e_dev_info("read <reg>\n");
		}
	} else {
		e_dev_info("Unknown command %s\n", rnpm_dbg_reg_ops_buf);
		e_dev_info("Available commands:\n");
		e_dev_info("   read <reg>\n");
		e_dev_info("   write <reg> <value>\n");
	}
	return count;
}

static const struct file_operations rnpm_dbg_reg_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = rnpm_dbg_reg_ops_read,
	.write = rnpm_dbg_reg_ops_write,
};

static char rnpm_dbg_netdev_ops_buf[256] = "";

/**
 * rnpm_dbg_netdev_ops_read - read for netdev_ops datum
 * @filp: the opened file
 * @buffer: where to write the data for the user to read
 * @count: the size of the user's buffer
 * @ppos: file position offset
 **/
static ssize_t rnpm_dbg_netdev_ops_read(struct file *filp,
					char __user *buffer, size_t count,
					loff_t *ppos)
{
	struct rnpm_adapter *adapter = filp->private_data;
	char *buf;
	int len;

	/* don't allow partial reads */
	if (*ppos != 0)
		return 0;

	buf = kasprintf(GFP_KERNEL, "%s: %s\n", adapter->name,
			rnpm_dbg_netdev_ops_buf);
	if (!buf)
		return -ENOMEM;

	if (count < strlen(buf)) {
		kfree(buf);
		return -ENOSPC;
	}

	len = simple_read_from_buffer(buffer, count, ppos, buf,
				      strlen(buf));
	kfree(buf);
	return len;
}

/**
 * rnpm_dbg_netdev_ops_write - write into netdev_ops datum
 * @filp: the opened file
 * @buffer: where to find the user's data
 * @count: the length of the user's data
 * @ppos: file position offset
 **/
static ssize_t rnpm_dbg_netdev_ops_write(struct file *filp,
					 const char __user *buffer,
					 size_t count, loff_t *ppos)
{
	struct rnpm_adapter *adapter = filp->private_data;
	int len;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;
	if (count >= sizeof(rnpm_dbg_netdev_ops_buf))
		return -ENOSPC;

	len = simple_write_to_buffer(rnpm_dbg_netdev_ops_buf,
				     sizeof(rnpm_dbg_netdev_ops_buf) - 1,
				     ppos, buffer, count);
	if (len < 0)
		return len;

	rnpm_dbg_netdev_ops_buf[len] = '\0';

	if (strncmp(rnpm_dbg_netdev_ops_buf, "stat", 4) == 0) {
		netdev_info(adapter->netdev, "adapter->stat=0x%lx\n",
			    adapter->state);
		netdev_info(adapter->netdev,
			    "adapter->tx_timeout_count=%d\n",
			    adapter->tx_timeout_count);
	} else if (strncmp(rnpm_dbg_netdev_ops_buf, "tx_timeout", 10) ==
		   0) {
		adapter->netdev->netdev_ops->ndo_tx_timeout(
			adapter->netdev, UINT_MAX);
		e_dev_info("tx_timeout called\n");
	} else {
		e_dev_info("Unknown command: %s\n",
			   rnpm_dbg_netdev_ops_buf);
		e_dev_info("Available commands:\n");
		e_dev_info("    tx_timeout\n");
	}
	return count;
}

static const struct file_operations rnpm_dbg_netdev_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = rnpm_dbg_netdev_ops_read,
	.write = rnpm_dbg_netdev_ops_write,
};

static char rnpm_dbg_phy_ops_buf[256] = "";

/**
 * rnpm_dbg_phy_ops_read - read for reg_ops datum
 * @filp: the opened file
 * @buffer: where to write the data for the user to read
 * @count: the size of the user's buffer
 * @ppos: file position offset
 **/
static ssize_t rnpm_dbg_phy_ops_read(struct file *filp,
				     char __user *buffer, size_t count,
				     loff_t *ppos)
{
	struct rnpm_adapter *adapter = filp->private_data;
	char *buf;
	int len;

	/* don't allow partial reads */
	if (*ppos != 0)
		return 0;

	buf = kasprintf(GFP_KERNEL, "%s: %s\n", adapter->name,
			rnpm_dbg_phy_ops_buf);
	if (!buf)
		return -ENOMEM;

	if (count < strlen(buf)) {
		kfree(buf);
		return -ENOSPC;
	}

	len = simple_read_from_buffer(buffer, count, ppos, buf,
				      strlen(buf));

	kfree(buf);
	return len;
}

/**
 * rnpm_dbg_reg_ops_write - write into reg_ops datum
 * @filp: the opened file
 * @buffer: where to find the user's data
 * @count: the length of the user's data
 * @ppos: file position offset
 **/
static ssize_t rnpm_dbg_phy_ops_write(struct file *filp,
				      const char __user *buffer,
				      size_t count, loff_t *ppos)
{
	struct rnpm_adapter *adapter = filp->private_data;
	struct rnpm_hw *hw = &adapter->hw;
	int len;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;
	if (count >= sizeof(rnpm_dbg_phy_ops_buf))
		return -ENOSPC;

	len = simple_write_to_buffer(rnpm_dbg_phy_ops_buf,
				     sizeof(rnpm_dbg_phy_ops_buf) - 1,
				     ppos, buffer, count);
	if (len < 0)
		return len;

	rnpm_dbg_phy_ops_buf[len] = '\0';

	if (strncmp(rnpm_dbg_phy_ops_buf, "write", 5) == 0) {
		u32 reg, value;
		int cnt;

		cnt = sscanf(&rnpm_dbg_phy_ops_buf[5], "%x %x", &reg,
			     &value);
		if (cnt == 2) {
			if (rnpm_mbx_phy_write(hw, reg, value) == 0)
				e_dev_info("write phy: 0x%08x = 0x%08x\n",
					   reg, value);
			else
				e_dev_info(
					"write phy failed: 0x%08x = 0x%08x\n",
					reg, value);
		} else {
			e_dev_info("write phy <reg> <value>\n");
		}

	} else if (strncmp(rnpm_dbg_phy_ops_buf, "read", 4) == 0) {
		u32 reg, value;
		int cnt;

		cnt = sscanf(&rnpm_dbg_phy_ops_buf[4], "%x", &reg);
		if (cnt == 1) {
			if (rnpm_mbx_phy_read(hw, reg, &value) == 0) {
				sprintf(rnpm_dbg_phy_ops_buf,
					"read phy 0x%08x = 0x%08x\n", reg,
					value);
				netdev_info(adapter->netdev,
					    "read phy 0x%08x = 0x%08x\n",
					    reg, value);
			} else
				e_dev_info(
					"read phy failed 0x%08x = 0x%08x\n",
					reg, value);
		} else {
			e_dev_info("read phy <reg>\n");
		}
	} else {
		e_dev_info("Unknown command %s\n", rnpm_dbg_phy_ops_buf);
		e_dev_info("Available commands:\n");
		e_dev_info("   read <phyreg>\n");
		e_dev_info("   write <phyreg> <value>\n");
	}
	return count;
}

static const struct file_operations rnpm_dbg_phy_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = rnpm_dbg_phy_ops_read,
	.write = rnpm_dbg_phy_ops_write,
};

/**
 * rnpm_dbg_adapter_init - setup the debugfs directory for the adapter
 * @adapter: the adapter that is starting up
 **/
void rnpm_dbg_adapter_init(struct rnpm_adapter *adapter)
{
	const char *name = adapter->name;
	struct dentry *pfile;

	adapter->rnpm_dbg_adapter =
		debugfs_create_dir(name, rnpm_dbg_root);
	if (adapter->rnpm_dbg_adapter) {
		pfile = debugfs_create_file("reg_ops", 0600,
					    adapter->rnpm_dbg_adapter,
					    adapter,
					    &rnpm_dbg_reg_ops_fops);
		if (!pfile)
			e_dev_err("debugfs reg_ops for %s failed\n", name);
		pfile = debugfs_create_file("netdev_ops", 0600,
					    adapter->rnpm_dbg_adapter,
					    adapter,
					    &rnpm_dbg_netdev_ops_fops);
		if (!pfile)
			e_dev_err("debugfs netdev_ops for %s failed\n",
				  name);
		pfile = debugfs_create_file("phy_ops", 0600,
					    adapter->rnpm_dbg_adapter,
					    adapter,
					    &rnpm_dbg_phy_ops_fops);
		if (!pfile)
			e_dev_err("debugfs netdev_ops for %s failed\n",
				  name);

#ifndef NO_CSL_DEBUG
		if (rnpm_is_pf1(adapter->pdev) == 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0))
			pfile = debugfs_create_file_unsafe(
#else
			pfile = debugfs_create_file(
#endif
				"csl", 0755, adapter->rnpm_dbg_adapter,
				adapter, &rnpm_dbg_csl_fops);
			if (!pfile)
				e_dev_err("debugfs csl  failed\n");
		}
#endif
		pfile = debugfs_create_file("info", 0600,
					    adapter->rnpm_dbg_adapter,
					    adapter,
					    &rnpm_dbg_eth_info_fops);
		if (!pfile)
			e_dev_err("debugfs info  failed\n");
		pfile = debugfs_create_file(
			"mbx_cookies_info", 0600,
			adapter->rnpm_dbg_adapter, adapter,
			&rnpm_dbg_mbx_cookies_info_fops);
		if (!pfile)
			e_dev_err(
				"debugfs reg_ops for mbx_cookies_info failed\n");
	} else {
		e_dev_err("debugfs entry for %s failed\n", name);
	}
}

/**
 * rnpm_dbg_adapter_exit - clear out the adapter's debugfs entries
 * @pf: the pf that is stopping
 **/
void rnpm_dbg_adapter_exit(struct rnpm_adapter *adapter)
{
	if (adapter->rnpm_dbg_adapter)
		debugfs_remove_recursive(adapter->rnpm_dbg_adapter);
	adapter->rnpm_dbg_adapter = NULL;
}

/**
 * rnpm_dbg_init - start up debugfs for the driver
 **/
void rnpm_dbg_init(void)
{
	rnpm_dbg_root = debugfs_create_dir(rnpm_driver_name, NULL);
	if (rnpm_dbg_root == NULL)
		pr_err("init of debugfs failed\n");
}

/**
 * rnpm_dbg_exit - clean out the driver's debugfs entries
 */
void rnpm_dbg_exit(void)
{
	debugfs_remove_recursive(rnpm_dbg_root);
}
#endif
