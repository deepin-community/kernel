#include <linux/debugfs.h>
#include <linux/module.h>

#include "sxe.h"
#include "sxe_netdev.h"
#include "sxe_version.h"
#include "sxe_phy.h"

#define SXE_HW_STAT(m)	sizeof(((struct sxe_adapter *)0)->m), \
				offsetof(struct sxe_adapter, m)

#define SXE_HW_STATS_LEN ARRAY_SIZE(hw_stats)

struct sxe_debugfs_hw_stats {
	const char *stat_string;
	int sizeof_stat;
	int stat_offset;
};

static const struct sxe_debugfs_hw_stats hw_stats[] = {
	{"rx_good_pkts", SXE_HW_STAT(stats.hw.gprc)},
	{"tx_good_pkts", SXE_HW_STAT(stats.hw.gptc)},
	{"rx_good_bytes", SXE_HW_STAT(stats.hw.gorc)},
	{"tx_good_bytes", SXE_HW_STAT(stats.hw.gotc)},
	{"rx_broadcast", SXE_HW_STAT(stats.hw.bprc)},
	{"tx_broadcast", SXE_HW_STAT(stats.hw.bptc)},
	{"rx_multicast", SXE_HW_STAT(stats.hw.mprc)},
	{"tx_multicast", SXE_HW_STAT(stats.hw.mptc)},
	{"fnav_match", SXE_HW_STAT(stats.hw.fnavmatch)},
	{"fnav_miss", SXE_HW_STAT(stats.hw.fnavmiss)},
	{"rx_64_bytes", SXE_HW_STAT(stats.hw.prc64)},
	{"rx_65~127_bytes", SXE_HW_STAT(stats.hw.prc127)},
	{"rx_128~255_bytes", SXE_HW_STAT(stats.hw.prc255)},
	{"rx_256~511_bytes", SXE_HW_STAT(stats.hw.prc511)},
	{"rx_512~1023_bytes", SXE_HW_STAT(stats.hw.prc1023)},
	{"rx_1024~1522_bytes", SXE_HW_STAT(stats.hw.prc1522)},
	{"tx_64_bytes", SXE_HW_STAT(stats.hw.ptc64)},
	{"tx_65~127_bytes", SXE_HW_STAT(stats.hw.ptc127)},
	{"tx_128~255_bytes", SXE_HW_STAT(stats.hw.ptc255)},
	{"tx_256~511_bytes", SXE_HW_STAT(stats.hw.ptc511)},
	{"tx_512~1023_bytes", SXE_HW_STAT(stats.hw.ptc1023)},
	{"tx_1024~1522_bytes", SXE_HW_STAT(stats.hw.ptc1522)},
	{"rx_total_pkts", SXE_HW_STAT(stats.hw.tpr)},
	{"tx_total_pkts", SXE_HW_STAT(stats.hw.tpt)},
	{"rx_total_bytes", SXE_HW_STAT(stats.hw.tor)},
	{"rx_long_length_errors", SXE_HW_STAT(stats.hw.roc)},
	{"rx_short_length_errors", SXE_HW_STAT(stats.hw.ruc)},
	{"rx_short_length_with_bad_crc_errors", SXE_HW_STAT(stats.hw.rfc)},
	{"rx_crc_error", SXE_HW_STAT(stats.hw.crcerrs)},
	{"rx_error_byte", SXE_HW_STAT(stats.hw.errbc)},
	{"rx_length_errors", SXE_HW_STAT(stats.hw.rlec)},
	{"rx_jabber_errors", SXE_HW_STAT(stats.hw.rjc)},
};

static struct dentry *sxe_debugfs_root;

STATIC s8 sxe_debugfs_reg_ops_buf[256] = "";

STATIC ssize_t sxe_debugfs_common_ops_read(struct file *filp, char __user *buffer,
					 size_t count, loff_t *ppos,
					 char *debugfs_buf)
{
	s8 *buf;
	ssize_t ret;
	struct sxe_adapter *adapter = filp->private_data;

	if (*ppos != 0) {
		ret = 0;
		goto l_end;
	}

	buf = kasprintf(GFP_KERNEL, "%s: %s\n",
			adapter->netdev->name, debugfs_buf);
	if (!buf) {
		ret = -ENOMEM;
		goto l_end;
	}

	if (count < strlen(buf)) {
		ret = -ENOSPC;
		goto l_free;
	}

	ret = simple_read_from_buffer(buffer, count, ppos, buf, strlen(buf));

l_free:
	kfree(buf);
l_end:
	return ret;
}

STATIC ssize_t sxe_debugfs_reg_ops_read(struct file *filp, char __user *buffer,
				      size_t count, loff_t *ppos)
{
	return sxe_debugfs_common_ops_read(filp, buffer, count, ppos,
					 sxe_debugfs_reg_ops_buf);
}

STATIC ssize_t sxe_debugfs_reg_ops_write(struct file *filp,
				     const char __user *buffer,
				     size_t count, loff_t *ppos)
{
	ssize_t ret;
	s32 cnt;
	u32 reg, value;
	struct sxe_adapter *adapter = filp->private_data;
	struct sxe_hw *hw = &adapter->hw;

	if (*ppos != 0) {
		ret = 0;
		goto l_end;
	}

	if (count >= sizeof(sxe_debugfs_reg_ops_buf)) {
		ret = -ENOSPC;
		goto l_end;
	}

	ret = simple_write_to_buffer(sxe_debugfs_reg_ops_buf,
				     sizeof(sxe_debugfs_reg_ops_buf)-1,
				     ppos, buffer, count);
	if (ret < 0) {
		goto l_end;
	}

	sxe_debugfs_reg_ops_buf[ret] = '\0';

	if (strncmp(sxe_debugfs_reg_ops_buf, "write", 5) == 0) {
		cnt = sscanf(&sxe_debugfs_reg_ops_buf[5], "%x %x", &reg, &value);
		if (cnt != 2) {
			LOG_DEV_INFO("write <reg> <value>\n");
			ret = count;
			goto l_end;
		}

		if (reg >= pci_resource_len(adapter->pdev, 0)) {
			LOG_DEV_INFO("write ops : reg addr err, "
				"addr[%x]>bar0 max addr[0x100000]", reg);
			ret = -EINVAL;
			goto l_end;
		}

		hw->setup.ops->reg_write(hw, reg, value);
		value = hw->setup.ops->reg_read(hw, reg);
		LOG_DEV_INFO("write: 0x%08x = 0x%08x\n", reg, value);

	} else if (strncmp(sxe_debugfs_reg_ops_buf, "read", 4) == 0) {
		cnt = sscanf(&sxe_debugfs_reg_ops_buf[4], "%x", &reg);
		if (cnt != 1) {
			LOG_DEV_INFO("read <reg>\n");
			ret = count;
			goto l_end;
		}

		if (reg >= pci_resource_len(adapter->pdev, 0)) {
			LOG_DEV_INFO("read ops : reg addr err, "
				"addr[%x]>bar0 max addr[0x100000]", reg);
			ret = -EINVAL;
			goto l_end;
		}

		value = hw->setup.ops->reg_read(hw, reg);
		LOG_DEV_INFO("read 0x%08x = 0x%08x\n", reg, value);
	} else {
		LOG_DEV_INFO("unknown command %s\n", sxe_debugfs_reg_ops_buf);
		LOG_DEV_INFO("available commands:\n");
		LOG_DEV_INFO("   read <reg>\n");
		LOG_DEV_INFO("   write <reg> <value>\n");
	}

	ret = count;

l_end:
	return ret;
}

static const struct file_operations sxe_debugfs_reg_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read =  sxe_debugfs_reg_ops_read,
	.write = sxe_debugfs_reg_ops_write,
};

STATIC s8 debugfs_netdev_buf[256] = "";

STATIC ssize_t sxe_debugfs_netdev_ops_read(struct file *filp, char __user *buffer,
					 size_t count, loff_t *ppos)
{
	return sxe_debugfs_common_ops_read(filp, buffer, count, ppos,
					debugfs_netdev_buf);
}

STATIC ssize_t sxe_debugfs_netdev_ops_write(struct file *filp,
					  const char __user *buffer,
					  size_t count, loff_t *ppos)
{
	ssize_t ret;
	struct sxe_adapter *adapter = filp->private_data;

	if (*ppos != 0) {
		ret = 0;
		goto l_end;
	}
	if (count >= sizeof(debugfs_netdev_buf)) {
		ret = -ENOSPC;
		goto l_end;
	}

	ret = simple_write_to_buffer(debugfs_netdev_buf,
				sizeof(debugfs_netdev_buf)-1,
				ppos, buffer, count);
	if (ret < 0) {
		goto l_end;
	}

	debugfs_netdev_buf[ret] = '\0';

	if (0 == strncmp(debugfs_netdev_buf, "tx_timeout", 10)) {

#ifdef HAVE_TIMEOUT_TXQUEUE_IDX
	adapter->netdev->netdev_ops->ndo_tx_timeout(adapter->netdev, UINT_MAX);	
#else
	adapter->netdev->netdev_ops->ndo_tx_timeout(adapter->netdev);
#endif
		LOG_DEV_INFO("tx_timeout called\n");
	} else {
		LOG_DEV_INFO("unknown command: %s\n", debugfs_netdev_buf);
		LOG_DEV_INFO("available commands:\n");
		LOG_DEV_INFO("    tx_timeout\n");
	}

	ret = count;

l_end:
	return ret;
}

static const struct file_operations sxe_debugfs_netdev_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = sxe_debugfs_netdev_ops_read,
	.write = sxe_debugfs_netdev_ops_write,
};

STATIC ssize_t sxe_debugfs_hw_stats_read(struct file *filp, char __user *buffer,
				       size_t count, loff_t *ppos)
{
	u32 i;
	u64 value;
	s8 *offset;
	struct sxe_debugfs_hw_stats *hw_stats_t;
	struct sxe_adapter *adapter = filp->private_data;
	hw_stats_t = kzalloc(sizeof(struct sxe_debugfs_hw_stats) * SXE_HW_STATS_LEN, GFP_ATOMIC);

	stats_lock(adapter);
	sxe_stats_update(adapter);
	memcpy(hw_stats_t, hw_stats, sizeof(struct sxe_debugfs_hw_stats) * SXE_HW_STATS_LEN);
	stats_unlock(adapter);

	for (i = 0; i < SXE_HW_STATS_LEN; i++) {
		offset = (s8 *) adapter + hw_stats_t[i].stat_offset;

		value = (hw_stats_t[i].sizeof_stat ==
				sizeof(u64)) ? *(u64 *)offset : *(u32 *)offset;
	
		LOG_DEV_INFO("%s: %llu\n", hw_stats_t[i].stat_string, value);
	}
	kfree(hw_stats_t);

	return 0;
}

static const struct file_operations sxe_debugfs_hw_stats_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = sxe_debugfs_hw_stats_read,
};

STATIC ssize_t sxe_debugfs_sfp_info_read(struct file *filp, char __user *buffer,
				       size_t count, loff_t *ppos)
{
	struct sxe_adapter *adapter = filp->private_data;

	s32 ret;
	enum sxe_sfp_type sfp_type;
	u8 sfp_comp_code[SXE_SFP_COMP_CODE_SIZE];
	u8 sfp_vendor_pn[SXE_SFP_VENDOR_PN_SIZE + 1] = {0};

	LOG_INFO_BDF("sfp identify start\n");

	ret = sxe_sfp_eeprom_read(adapter, SXE_SFF_BASE_ADDR,
				SXE_SFP_COMP_CODE_SIZE, sfp_comp_code);
	if (ret) {
		sfp_type = SXE_SFP_TYPE_NOT_PRESENT;
		LOG_DEV_ERR("get sfp identifier failed, ret=%d\n", ret);
		goto l_end;
	}

	LOG_DEV_INFO("sfp identifier=%x, cable_technology=%x, "
			"10GB_code=%x, 1GB_code=%x\n",
		sfp_comp_code[SXE_SFF_IDENTIFIER],
		sfp_comp_code[SXE_SFF_CABLE_TECHNOLOGY],
		sfp_comp_code[SXE_SFF_10GBE_COMP_CODES],
		sfp_comp_code[SXE_SFF_1GBE_COMP_CODES]);

	if (sfp_comp_code[SXE_SFF_IDENTIFIER] != SXE_SFF_IDENTIFIER_SFP) {
		LOG_DEV_ERR("module type is not sfp/sfp+, offset=%d, type=%x\n",
			SXE_SFF_IDENTIFIER, sfp_comp_code[SXE_SFF_IDENTIFIER]);
		sfp_type = SXE_SFP_TYPE_UNKNOWN;
		goto l_end;
	}

	if (sfp_comp_code[SXE_SFF_CABLE_TECHNOLOGY] & SXE_SFF_DA_PASSIVE_CABLE) {
		sfp_type = SXE_SFP_TYPE_DA_CU;
	}  else if (sfp_comp_code[SXE_SFF_10GBE_COMP_CODES] & \
		(SXE_SFF_10GBASESR_CAPABLE | SXE_SFF_10GBASELR_CAPABLE)) {
		sfp_type = SXE_SFP_TYPE_SRLR;
	} else if (sfp_comp_code[SXE_SFF_1GBE_COMP_CODES] & \
		SXE_SFF_1GBASET_CAPABLE) {
		sfp_type = SXE_SFP_TYPE_1G_CU;
	} else if ((sfp_comp_code[SXE_SFF_1GBE_COMP_CODES] & \
		SXE_SFF_1GBASESX_CAPABLE) || \
		(sfp_comp_code[SXE_SFF_1GBE_COMP_CODES] & \
		SXE_SFF_1GBASELX_CAPABLE)) {
		sfp_type = SXE_SFP_TYPE_1G_SXLX;
	} else {
		sfp_type = SXE_SFP_TYPE_UNKNOWN;
	}

	LOG_DEV_INFO("identify sfp, sfp_type=%d\n", sfp_type);

	if (((sfp_comp_code[SXE_SFF_1GBE_COMP_CODES] & \
			SXE_SFF_1GBASESX_CAPABLE) &&
		(sfp_comp_code[SXE_SFF_10GBE_COMP_CODES] & \
			SXE_SFF_10GBASESR_CAPABLE)) ||
		((sfp_comp_code[SXE_SFF_1GBE_COMP_CODES] & \
			SXE_SFF_1GBASELX_CAPABLE) &&
		(sfp_comp_code[SXE_SFF_10GBE_COMP_CODES] & \
			SXE_SFF_10GBASELR_CAPABLE))) {
		LOG_DEV_INFO("identify sfp, sfp is multispeed\n");
	} else {
		LOG_DEV_INFO("identify sfp, sfp is not multispeed\n");
	}

	ret = sxe_sfp_eeprom_read(adapter, SXE_SFF_VENDOR_PN,
				SXE_SFP_VENDOR_PN_SIZE, sfp_vendor_pn);
	if (ret) {
		LOG_DEV_ERR("get sfp vendor pn failed, ret=%d\n", ret);
		goto l_end;
	}

	LOG_DEV_INFO("sfp vendor pn: %s\n", sfp_vendor_pn);

	ret = sxe_sfp_vendor_pn_cmp(sfp_vendor_pn);
	if (0 == ret) {
		LOG_DEV_WARN("an supported SFP module type was detected\n");
		goto l_end;
	}

	LOG_DEV_WARN("an unsupported SFP module type was detected\n");
	LOG_DEV_WARN("refer to the sxe ethernet adapters and devices user "
			"guide for a list of supported modules\n");

l_end:
	return 0;
}

static const struct file_operations sxe_debugfs_sfp_info_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = sxe_debugfs_sfp_info_read,
};

void sxe_debugfs_entries_init(struct sxe_adapter *adapter)
{
	struct dentry *dir;
	const char *name = pci_name(adapter->pdev);

	adapter->debugfs_entries = debugfs_create_dir(name, sxe_debugfs_root);
	dir = debugfs_create_file("reg_ops", 0600, adapter->debugfs_entries,
			    adapter, &sxe_debugfs_reg_ops_fops);
	if ((dir == NULL) || (dir == ERR_PTR(-ENODEV))) {
		LOG_INFO_BDF("debugfs:reg_ops file create failed\n");
	}

	dir = debugfs_create_file("netdev_ops", 0600, adapter->debugfs_entries,
			    adapter, &sxe_debugfs_netdev_ops_fops);
	if ((dir == NULL) || (dir == ERR_PTR(-ENODEV))) {
		LOG_INFO_BDF("debugfs:netdev_ops file create failed\n");
	}

	dir = debugfs_create_file("hw_stats", 0400, adapter->debugfs_entries,
			    adapter, &sxe_debugfs_hw_stats_fops);
	if ((dir == NULL) || (dir == ERR_PTR(-ENODEV))) {
		LOG_INFO_BDF("debugfs:hw_stats file create failed\n");
	}

	dir = debugfs_create_file("sfp_info", 0400, adapter->debugfs_entries,
			    adapter, &sxe_debugfs_sfp_info_fops);
	if ((dir == NULL) || (dir == ERR_PTR(-ENODEV))) {
		LOG_INFO_BDF("debugfs:sfp_info file create failed\n");
	}

	return;
}

void sxe_debugfs_entries_exit(struct sxe_adapter *adapter)
{
	debugfs_remove_recursive(adapter->debugfs_entries);
	adapter->debugfs_entries = NULL;

	return;
}

void sxe_debugfs_init(void)
{
	sxe_debugfs_root = debugfs_create_dir(SXE_DRV_NAME, NULL);

	return;
}

void sxe_debugfs_exit(void)
{
	debugfs_remove_recursive(sxe_debugfs_root);

	return;
}
