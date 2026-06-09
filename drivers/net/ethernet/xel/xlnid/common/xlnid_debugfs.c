/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2008 - 2024 Xel Technology.  */

#include "xlnid.h"

#ifdef HAVE_XLNID_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/module.h>

static struct dentry *xlnid_dbg_root;

static unsigned int xlnid_westlake_baseaddr(const char *module)
{
	if (strcmp(module, "crg") == 0) {
		return WESTLAKE_CRG_BASE;
	}

	else if (strcmp(module, "ethport") == 0) {
		return WESTLAKE_ETH_PORT_BASE;
	}

	else if (strcmp(module, "macsec") == 0) {
		return WESTLAKE_MACSEC_BASE;
	}

	else if (strcmp(module, "rxctl") == 0) {
		return WESTLAKE_RX_CTL_BASE;
	}

	else if (strcmp(module, "txctl") == 0) {
		return WESTLAKE_TX_CTL_BASE;
	}

	else if (strcmp(module, "smbus") == 0) {
		return WESTLAKE_SMBUS_BASE;
	}

	else if (strcmp(module, "dmactl") == 0) {
		return WESTLAKE_DMA_CTL_BASE;
	}

	else if (strcmp(module, "ethcomm") == 0) {
		return WESTLAKE_ETH_COMMON_BASE;
	}

	else if (strcmp(module, "memctl") == 0) {
		return WESTLAKE_MEM_CTL_BASE;
	}

	else if (strcmp(module, "pciectl") == 0) {
		return WESTLAKE_PCIE_CTL_BASE;
	}

	else if (strcmp(module, "led") == 0) {
		return WESTLAKE_LED_BASE;
	}

	else if (strcmp(module, "iphy") == 0) {
		return WESTLAKE_INNER_PHY_BASE;
	}

	else if (strcmp(module, "usb") == 0) {
		return WESTLAKE_USB_BASE;
	}

	else if (strcmp(module, "ophy") == 0) {
		return WESTLAKE_OUTER_PHY_BASE;
	}

	else if (strcmp(module, "debug") == 0) {
		return WESTLAKE_DEBUG_BASE;
	}

	else if (strcmp(module, "i2c") == 0) {
		return WESTLAKE_I2C_BASE;
	}

	else if (strcmp(module, "cfg") == 0) {
		return 0x0;
	}

	return 0xFFFFFFFF;
}

static char xlnid_dbg_reg_ops_buf[256] = "";

/**
		xlnid_dbg_reg_ops_read - read for reg_ops datum
		@filp: the opened file
		@buffer: where to write the data for the user to read
		@count: the size of the user's buffer
		@ppos: file position offset
	**/
static ssize_t xlnid_dbg_reg_ops_read(struct file *filp, char __user *buffer,
										size_t count, loff_t *ppos)
{
	struct xlnid_adapter *adapter = filp->private_data;
	char *buf;
	int len;

	/* don't allow partial reads */
	if (*ppos != 0) {
		return 0;
	}

	buf = kasprintf(GFP_KERNEL, "%s: %s\n",
					adapter->netdev->name,
					xlnid_dbg_reg_ops_buf);

	if (!buf) {
		return -ENOMEM;
	}

	if (count < strlen(buf)) {
		kfree(buf);
		return -ENOSPC;
	}

	len = simple_read_from_buffer(buffer, count, ppos, buf, strlen(buf));

	kfree(buf);
	return len;
}

/**
		xlnid_dbg_reg_ops_write - write into reg_ops datum
		@filp: the opened file
		@buffer: where to find the user's data
		@count: the length of the user's data
		@ppos: file position offset
	**/
static ssize_t xlnid_dbg_reg_ops_write(struct file *filp,
										const char __user *buffer,
										size_t count, loff_t *ppos)
{
	struct xlnid_adapter *adapter = filp->private_data;
	int len;

	/* don't allow partial writes */
	if (*ppos != 0) {
		return 0;
	}

	if (count >= sizeof(xlnid_dbg_reg_ops_buf)) {
		return -ENOSPC;
	}

	len = simple_write_to_buffer(xlnid_dbg_reg_ops_buf,
									sizeof(xlnid_dbg_reg_ops_buf) - 1,
									ppos,
									buffer,
									count);

	if (len < 0) {
		return len;
	}

	xlnid_dbg_reg_ops_buf[len] = '\0';

	if (strncmp(xlnid_dbg_reg_ops_buf, "write", 5) == 0) {
		u32 reg, write_val, read_val, base_addr;
		int cnt;
		char module[10];

		cnt = sscanf(&xlnid_dbg_reg_ops_buf[5], "%s %x %x", module, &reg, &write_val);

		/* check format and bounds check register access */
		if (cnt == 3) {
			if (adapter->hw.mac.type == xlnid_mac_WESTLAKE) {
				if (strcmp(module, "gmac") == 0) {
					xlnid_write_mac_westlake(&adapter->hw, reg, write_val);
					read_val = xlnid_read_mac_westlake(&adapter->hw, reg);

					e_dev_info("write: %s 0x%08x = 0x%08x\n", module, reg, write_val);
					e_dev_info("read: %s 0x%08x = 0x%08x\n", module, reg, read_val);
				}

				else if (strcmp(module, "gephy") == 0) {
					u16 phy_wdata, phy_rdata;

					phy_wdata = (u16)write_val;
					xlnid_write_phy_reg(&adapter->hw, reg, 0, phy_wdata);
					xlnid_read_phy_reg(&adapter->hw, reg, 0, &phy_rdata);

					e_dev_info("write: %s 0x%08x = 0x%04x\n", module, reg, phy_wdata);
					e_dev_info("read: %s 0x%08x = 0x%04x\n", module, reg, phy_rdata);
				}

				else {
					base_addr = xlnid_westlake_baseaddr(module);

					if (base_addr != 0xFFFFFFFF) {
						reg += base_addr;

						if (strcmp(module, "dmactl") == 0 || strcmp(module, "cfg") == 0) {
							xlnid_write_reg(&adapter->hw, reg, write_val, true);
							read_val = xlnid_read_reg(&adapter->hw, reg, false, true);
						}

						else {
							xlnid_write_reg(&adapter->hw, reg, write_val, false);
							read_val = xlnid_read_reg(&adapter->hw, reg, false, false);
						}

						e_dev_info("write: %s 0x%08x = 0x%08x\n", module, reg, write_val);
						e_dev_info("read: %s 0x%08x = 0x%08x\n", module, reg, read_val);
					}

					else {
						e_dev_info("Available module_names:\n");
						e_dev_info("   gmac | gephy | crg | ethport | macsec | rxctl | txctl | smbus | dmactl | cfg | ethcomm | memctl | pciectl | led | iphy | usb | ophy | debug | i2c\n");
					}
				}
			}
		}

		else {
			e_dev_info("write [module_name] [reg_addr] [value]\n");
			e_dev_info("[module_name]：gmac | gephy | crg | ethport | macsec | rxctl | txctl | smbus | dmactl | cfg | ethcomm | memctl | pciectl | led | iphy | usb | ophy | debug | i2c\n");
		}
	}

	else if (strncmp(xlnid_dbg_reg_ops_buf, "read", 4) == 0) {
		u32 reg, value, base_addr;
		int cnt;
		char module[10];

		cnt = sscanf(&xlnid_dbg_reg_ops_buf[4], "%s %x", module, &reg);

		/* check format and bounds check register access */
		if (cnt == 2) {
			if (adapter->hw.mac.type == xlnid_mac_WESTLAKE) {
				if (strcmp(module, "gmac") == 0) {
					value = xlnid_read_mac_westlake(&adapter->hw, reg);
					e_dev_info("read: %s 0x%08x = 0x%08x\n", module, reg, value);
				}

				else if (strcmp(module, "gephy") == 0) {
					u16 phy_data;
					xlnid_read_phy_reg(&adapter->hw, reg, 0, &phy_data);
					e_dev_info("read: %s 0x%08x = 0x%04x\n", module, reg, phy_data);
				}

				else {
					base_addr = xlnid_westlake_baseaddr(module);

					if (base_addr != 0xFFFFFFFF) {
						reg += base_addr;

						if (strcmp(module, "dmactl") == 0 || strcmp(module, "cfg") == 0) {
							value = xlnid_read_reg(&adapter->hw, reg, false, true);
						}

						else {
							value = xlnid_read_reg(&adapter->hw, reg, false, false);
						}

						e_dev_info("read: %s 0x%08x = 0x%08x\n", module, reg, value);
					}

					else {
						e_dev_info("Available module_names:\n");
						e_dev_info("   gmac | gephy | crg | ethport | macsec | rxctl | txctl | smbus | dmactl | cfg | ethcomm | memctl | pciectl | led | iphy | usb | ophy | debug | i2c\n");
					}
				}
			}
		}

		else {
			e_dev_info("read [module_name] [reg_addr]\n");
			e_dev_info("[module_name]：gmac | gephy | crg | ethport | macsec | rxctl | txctl | smbus | dmactl | cfg | ethcomm | memctl | pciectl | led | iphy | usb | ophy | debug | i2c\n");
		}
	}

	else {
		e_dev_info("Unknown command: %s", xlnid_dbg_reg_ops_buf);
		e_dev_info("Available commands:\n");
		e_dev_info("   read [module_name] [reg_addr]\n");
		e_dev_info("   write [module_name] [reg_addr] [value]\n");
		e_dev_info("Available module_names:\n");
		e_dev_info("   gmac | gephy | crg | ethport | macsec | rxctl | txctl | smbus | dmactl | cfg | ethcomm | memctl | pciectl | led | iphy | usb | ophy | debug | i2c\n");
	}

	return count;
}

static const struct file_operations xlnid_dbg_reg_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = xlnid_dbg_reg_ops_read,
	.write = xlnid_dbg_reg_ops_write,
};

static char xlnid_dbg_netdev_ops_buf[256] = "";

/**
		xlnid_dbg_netdev_ops_read - read for netdev_ops datum
		@filp: the opened file
		@buffer: where to write the data for the user to read
		@count: the size of the user's buffer
		@ppos: file position offset
	**/
static ssize_t xlnid_dbg_netdev_ops_read(struct file *filp, char __user *buffer,
		size_t count, loff_t *ppos)
{
	struct xlnid_adapter *adapter = filp->private_data;
	char *buf;
	int len;

	/* don't allow partial reads */
	if (*ppos != 0) {
		return 0;
	}

	buf = kasprintf(GFP_KERNEL, "%s: %s\n", adapter->netdev->name,
					xlnid_dbg_netdev_ops_buf);

	if (!buf) {
		return -ENOMEM;
	}

	if (count < strlen(buf)) {
		kfree(buf);
		return -ENOSPC;
	}

	len = simple_read_from_buffer(buffer, count, ppos, buf, strlen(buf));

	kfree(buf);
	return len;
}

/**
		xlnid_dbg_netdev_ops_write - write into netdev_ops datum
		@filp: the opened file
		@buffer: where to find the user's data
		@count: the length of the user's data
		@ppos: file position offset
	**/
static ssize_t xlnid_dbg_netdev_ops_write(struct file *filp,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	struct xlnid_adapter *adapter = filp->private_data;
	int len;

	/* don't allow partial writes */
	if (*ppos != 0) {
		return 0;
	}

	if (count >= sizeof(xlnid_dbg_netdev_ops_buf)) {
		return -ENOSPC;
	}

	len = simple_write_to_buffer(xlnid_dbg_netdev_ops_buf,
									sizeof(xlnid_dbg_netdev_ops_buf) - 1, ppos, buffer, count);

	if (len < 0) {
		return len;

	}

	xlnid_dbg_netdev_ops_buf[len] = '\0';

	if (strncmp(xlnid_dbg_netdev_ops_buf, "tx_timeout", 10) == 0) {
#ifdef HAVE_NET_DEVICE_OPS
#ifdef HAVE_TX_TIMEOUT_TXQUEUE
		adapter->netdev->netdev_ops->ndo_tx_timeout(adapter->netdev, UINT_MAX);
#else
		adapter->netdev->netdev_ops->ndo_tx_timeout(adapter->netdev);
#endif
#else
		adapter->netdev->tx_timeout(adapter->netdev);
#endif /* HAVE_NET_DEVICE_OPS */
		e_dev_info("tx_timeout called\n");
	}

	else {
		e_dev_info("Unknown command: %s\n", xlnid_dbg_netdev_ops_buf);
		e_dev_info("Available commands:\n");
		e_dev_info("    tx_timeout\n");
	}

	return count;
}

static struct file_operations xlnid_dbg_netdev_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = xlnid_dbg_netdev_ops_read,
	.write = xlnid_dbg_netdev_ops_write,
};

/**
		xlnid_dbg_adapter_init - setup the debugfs directory for the adapter
		@adapter: the adapter that is starting up
	**/
void xlnid_dbg_adapter_init(struct xlnid_adapter *adapter)
{
	const char *name = pci_name(adapter->pdev);

	adapter->xlnid_dbg_adapter_pf = debugfs_create_dir(name, xlnid_dbg_root);

	if (!adapter->xlnid_dbg_adapter_pf) {
		e_dev_err("debugfs pf entry for %s failed\n", name);
		return;
	}

	if (!debugfs_create_file("reg", 0600,
								adapter->xlnid_dbg_adapter_pf,
								adapter,
								&xlnid_dbg_reg_ops_fops)) {
		e_dev_err("debugfs reg for %s failed\n", name);
		goto create_failed;
	}

	if (!debugfs_create_file("netdev", 0600,
								adapter->xlnid_dbg_adapter_pf,
								adapter,
								&xlnid_dbg_netdev_ops_fops)) {
		e_dev_err("debugfs netdev for %s failed\n", name);
		goto create_failed;
	}

	return;

create_failed:
	debugfs_remove_recursive(adapter->xlnid_dbg_adapter_pf);
	adapter->xlnid_dbg_adapter_pf = NULL;
}

/**
		xlnid_dbg_adapter_exit - clear out the adapter's debugfs entries
		@adapter: board private structure
	**/
void xlnid_dbg_adapter_exit(struct xlnid_adapter *adapter)
{
	if (adapter->xlnid_dbg_adapter_pf) {
		debugfs_remove_recursive(adapter->xlnid_dbg_adapter_pf);
	}

	adapter->xlnid_dbg_adapter_pf = NULL;
}

/**
		xlnid_dbg_init - create root directory for debugfs entries
	**/
void xlnid_dbg_init(const char *driver_name)
{
	xlnid_dbg_root = debugfs_create_dir(driver_name, NULL);

	if (xlnid_dbg_root == NULL) {
		pr_err("init of debugfs failed\n");
	}
}

/**
		xlnid_dbg_exit - clean out the driver's debugfs entries
	**/
void xlnid_dbg_exit(void)
{
	debugfs_remove_recursive(xlnid_dbg_root);
}

#endif /* HAVE_XLNID_DEBUG_FS */
