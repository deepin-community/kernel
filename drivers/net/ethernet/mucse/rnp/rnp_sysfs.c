// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/hwmon.h>
#include <linux/ctype.h>

#include "rnp.h"
#include "rnp_common.h"
#include "rnp_type.h"

#include "rnp_mbx.h"
#include "rnp_mbx_fw.h"

#define PHY_EXT_REG_FLAG 0x80000000

struct maintain_req {
	int magic;
#define MAINTAIN_MAGIC 0xa6a7a8a9

	int cmd;
	int arg0;
	int req_data_bytes;
	int reply_bytes;
	char data[0];
} __attribute__((packed));

struct ucfg_mac_sn {
	unsigned char macaddr[64];
	unsigned char sn[32];
	int magic;
#define MAC_SN_MAGIC 0x87654321
	char rev[52];
	unsigned char pn[32];
} __attribute__((packed, aligned(4)));

static int print_desc(char *buf, void *data, int len)
{
	u8 *ptr = (u8 *)data;
	int ret = 0;
	int i = 0;

	for (i = 0; i < len; i++)
		ret += sprintf(buf + ret, "%02x ", *(ptr + i));

	return ret;
}

#ifdef RNP_HWMON
static ssize_t rnp_hwmon_show_location(struct device __always_unused *dev,
				       struct device_attribute *attr, char *buf)
{
	struct hwmon_attr *rnp_attr =
		container_of(attr, struct hwmon_attr, dev_attr);

	return snprintf(buf, PAGE_SIZE, "loc%u\n", rnp_attr->sensor->location);
}

static ssize_t rnp_hwmon_show_name(struct device __always_unused *dev,
				   struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "rnp\n");
}

static ssize_t rnp_hwmon_show_temp(struct device __always_unused *dev,
				   struct device_attribute *attr, char *buf)
{
	struct hwmon_attr *rnp_attr =
		container_of(attr, struct hwmon_attr, dev_attr);
	unsigned int value;

	/* reset the temp field */
	rnp_attr->hw->ops.get_thermal_sensor_data(rnp_attr->hw);

	value = rnp_attr->sensor->temp;
	/* display millidegree */
	value *= 1000;

	return snprintf(buf, PAGE_SIZE, "%u\n", value);
}

static ssize_t rnp_hwmon_show_cautionthresh(struct device __always_unused *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct hwmon_attr *rnp_attr =
		container_of(attr, struct hwmon_attr, dev_attr);
	unsigned int value = rnp_attr->sensor->caution_thresh;
	/* display millidegree */
	value *= 1000;

	return snprintf(buf, PAGE_SIZE, "%u\n", value);
}

static ssize_t rnp_hwmon_show_maxopthresh(struct device __always_unused *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	struct hwmon_attr *rnp_attr =
		container_of(attr, struct hwmon_attr, dev_attr);
	unsigned int value = rnp_attr->sensor->max_op_thresh;

	/* display millidegree */
	value *= 1000;

	return snprintf(buf, PAGE_SIZE, "%u\n", value);
}

/**
 * rnp_add_hwmon_attr - Create hwmon attr table for a hwmon sysfs file.
 * @adapter: pointer to the adapter structure
 * @offset: offset in the eeprom sensor data table
 * @type: type of sensor data to display
 *
 * For each file we want in hwmon's sysfs interface we need a 
 * device_attribute This is included in our hwmon_attr struct that contains
 * the references to the data structures we need to get the data to display
 */
static int rnp_add_hwmon_attr(struct rnp_adapter *adapter, unsigned int offset,
			      int type)
{
	unsigned int n_attr;
	struct hwmon_attr *rnp_attr;

	n_attr = adapter->rnp_hwmon_buff->n_hwmon;
	rnp_attr = &adapter->rnp_hwmon_buff->hwmon_list[n_attr];

	switch (type) {
	case RNP_HWMON_TYPE_LOC:
		rnp_attr->dev_attr.show = rnp_hwmon_show_location;
		snprintf(rnp_attr->name, sizeof(rnp_attr->name), "temp%u_label",
			 offset + 1);
		break;
	case RNP_HWMON_TYPE_NAME:
		rnp_attr->dev_attr.show = rnp_hwmon_show_name;
		snprintf(rnp_attr->name, sizeof(rnp_attr->name), "name");
		break;
	case RNP_HWMON_TYPE_TEMP:
		rnp_attr->dev_attr.show = rnp_hwmon_show_temp;
		snprintf(rnp_attr->name, sizeof(rnp_attr->name), "temp%u_input",
			 offset + 1);
		break;
	case RNP_HWMON_TYPE_CAUTION:
		rnp_attr->dev_attr.show = rnp_hwmon_show_cautionthresh;
		snprintf(rnp_attr->name, sizeof(rnp_attr->name), "temp%u_max",
			 offset + 1);
		break;
	case RNP_HWMON_TYPE_MAX:
		rnp_attr->dev_attr.show = rnp_hwmon_show_maxopthresh;
		snprintf(rnp_attr->name, sizeof(rnp_attr->name), "temp%u_crit",
			 offset + 1);
		break;
	default:
		return -EPERM;
	}

	/* These always the same regardless of type */
	rnp_attr->sensor = &adapter->hw.thermal_sensor_data.sensor[offset];
	rnp_attr->hw = &adapter->hw;
	rnp_attr->dev_attr.store = NULL;
	rnp_attr->dev_attr.attr.mode = 0444;
	rnp_attr->dev_attr.attr.name = rnp_attr->name;
	sysfs_attr_init(&rnp_attr->dev_attr.attr);
	adapter->rnp_hwmon_buff->attrs[n_attr] = &rnp_attr->dev_attr.attr;
	++adapter->rnp_hwmon_buff->n_hwmon;

	return 0;
}
#endif /* RNP_HWMON */

#define to_net_device(n) container_of(n, struct net_device, dev)
static ssize_t maintain_read(struct file *filp, struct kobject *kobj,
			     struct bin_attribute *attr, char *buf, loff_t off,
			     size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int rbytes = count;

	if (adapter->maintain_buf == NULL)
		return 0;

	if (off + count > adapter->maintain_buf_len)
		rbytes = adapter->maintain_buf_len - off;

	memcpy(buf, adapter->maintain_buf + off, rbytes);

	if ((off + rbytes) >= adapter->maintain_buf_len) {
		kfree(adapter->maintain_buf);
		adapter->maintain_buf = NULL;
		adapter->maintain_buf_len = 0;
	}

	return rbytes;
}

static ssize_t maintain_write(struct file *filp, struct kobject *kobj,
			      struct bin_attribute *attr, char *buf, loff_t off,
			      size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	int err = -EINVAL;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	struct maintain_req *req;
	void *dma_buf = NULL;
	dma_addr_t dma_phy;
	int bytes;

	if (off == 0) {
		if (count < sizeof(*req)) {
			return -EINVAL;
		}
		req = (struct maintain_req *)buf;
		if (req->magic != MAINTAIN_MAGIC) {
			return -EINVAL;
		}
		bytes = max_t(int, req->req_data_bytes, req->reply_bytes);
		bytes += sizeof(*req);

		/* free no readed buf */
		if (adapter->maintain_buf) {
			kfree(adapter->maintain_buf);
			adapter->maintain_buf = NULL;
			adapter->maintain_buf_len = 0;
		}

		dma_buf = dma_alloc_coherent(&hw->pdev->dev, bytes, &dma_phy,
					     GFP_ATOMIC);
		if (!dma_buf) {
			netdev_err(netdev, "%s: no memory:%d!", __func__,
				   bytes);
			return -ENOMEM;
		}

		adapter->maintain_dma_buf = dma_buf;
		adapter->maintain_dma_phy = dma_phy;
		adapter->maintain_dma_size = bytes;
		adapter->maintain_in_bytes = req->req_data_bytes + sizeof(*req);

		memcpy(dma_buf + off, buf, count);

		if (count < adapter->maintain_in_bytes)
			return count;
	}

	dma_buf = adapter->maintain_dma_buf;
	dma_phy = adapter->maintain_dma_phy;
	req = (struct maintain_req *)dma_buf;

	memcpy(dma_buf + off, buf, count);

	/* all data got, send req */
	if ((off + count) >= adapter->maintain_in_bytes) {
		int reply_bytes = req->reply_bytes;
		err = rnp_maintain_req(hw, req->cmd, req->arg0,
				       req->req_data_bytes, req->reply_bytes,
				       dma_phy);
		if (err != 0) {
			goto err_quit;
		}
		/* copy data for read */
		if (reply_bytes > 0) {
			adapter->maintain_buf_len = reply_bytes;
			adapter->maintain_buf =
				kmalloc(adapter->maintain_buf_len, GFP_KERNEL);
			if (!adapter->maintain_buf) {
				netdev_err(netdev,
					   "No Memory for maintain buf:%d\n",
					   adapter->maintain_buf_len);
				err = -ENOMEM;

				goto err_quit;
			}
			memcpy(adapter->maintain_buf, dma_buf, reply_bytes);
		}

		if (dma_buf) {
			dma_free_coherent(&hw->pdev->dev,
					  adapter->maintain_dma_size, dma_buf,
					  dma_phy);
		}
		adapter->maintain_dma_buf = NULL;
	}

	return count;
err_quit:
	if (dma_buf) {
		dma_free_coherent(&hw->pdev->dev, adapter->maintain_dma_size,
				  dma_buf, dma_phy);
		adapter->maintain_dma_buf = NULL;
	}
	return err;
}

static BIN_ATTR(maintain, (S_IWUSR | S_IRUGO), maintain_read, maintain_write,
		1 * 1024 * 1024);

static ssize_t show_version_info(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int ret = 0;

	ret += sprintf(buf + ret, "driver :%s-%x\n",
			rnp_driver_version, hw->pcode);
	ret += sprintf(buf + ret, "fw     :%d.%d.%d.%d 0x%08x\n", ((char *)&(hw->fw_version))[3],
		 ((char *)&(hw->fw_version))[2], ((char *)&(hw->fw_version))[1],
		 ((char *)&(hw->fw_version))[0], hw->bd_uid);

	return ret;
}

static ssize_t show_ring_sriov_info(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int ret = 0;

	ret += sprintf(buf + ret, "now sriov ring num is %d\n", hw->sriov_ring_limit);

	return ret;
}

static ssize_t store_ring_sriov_info(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int ret = count;
	u32 sriov_ring_num;

	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED) {
		printk("should close sriov first\n");
		return -EINVAL;
	}

	if (0 != kstrtou32(buf, 0, &sriov_ring_num))
		return -EINVAL;
	/* should check tx_ring_num is valid */
	if ((sriov_ring_num != 0) && (sriov_ring_num <= 32)) {
		hw->sriov_ring_limit = sriov_ring_num;
	} else {
		ret = -EINVAL;
	}

	return ret;
}

static ssize_t show_rx_desc_info(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	u32 rx_ring_num = adapter->sysfs_rx_ring_num;
	u32 rx_desc_num = adapter->sysfs_rx_desc_num;
	struct rnp_ring *ring = adapter->rx_ring[rx_ring_num];
	int ret = 0;
	union rnp_rx_desc *desc;

	if (test_bit(__RNP_DOWN, &adapter->state)) {
		ret += sprintf(buf + ret, "port not up \n");
		return ret;
	}

	desc = RNP_RX_DESC(ring, rx_desc_num);
	ret += sprintf(buf + ret, "rx ring %d desc %d:\n", rx_ring_num,
			rx_desc_num);
	ret += print_desc(buf + ret, desc, sizeof(*desc));
	ret += sprintf(buf + ret, "\n");

	return ret;
}

static ssize_t store_rx_desc_info(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int ret = count;
	u32 rx_desc_num = adapter->sysfs_rx_desc_num;
	u32 rx_ring_num = adapter->sysfs_rx_ring_num;
	struct rnp_ring *ring = adapter->rx_ring[rx_ring_num];

	if (0 != kstrtou32(buf, 0, &rx_desc_num))
		return -EINVAL;
	/* should check tx_ring_num is valid */
	if (rx_desc_num < ring->count) {
		adapter->sysfs_rx_desc_num = rx_desc_num;
	} else {
		ret = -EINVAL;
	}

	return ret;
}

static ssize_t show_tcp_sync_info(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int ret = 0;

	if (adapter->priv_flags & RNP_PRIV_FLAG_TCP_SYNC)
		ret += sprintf(
			buf + ret, "tcp sync remap on queue %d prio %s\n",
			adapter->tcp_sync_queue,
			(adapter->priv_flags & RNP_PRIV_FLAG_TCP_SYNC_PRIO) ?
				"NO" :
				"OFF");
	else
		ret += sprintf(buf + ret, "tcp sync remap off\n");

	return ret;
}

static ssize_t store_tcp_sync_info(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int ret = count;
	u32 tcp_sync_queue;

	if (0 != kstrtou32(buf, 0, &tcp_sync_queue))
		return -EINVAL;

	if (tcp_sync_queue < adapter->num_rx_queues) {
		adapter->tcp_sync_queue = tcp_sync_queue;
		adapter->priv_flags |= RNP_PRIV_FLAG_TCP_SYNC;

		if (adapter->priv_flags & RNP_PRIV_FLAG_TCP_SYNC_PRIO)
			hw->ops.set_tcp_sync_remapping(
				hw, adapter->tcp_sync_queue, true, true);
		else
			hw->ops.set_tcp_sync_remapping(
				hw, adapter->tcp_sync_queue, true, false);

	} else {
		adapter->priv_flags &= ~RNP_PRIV_FLAG_TCP_SYNC;

		hw->ops.set_tcp_sync_remapping(hw, adapter->tcp_sync_queue,
					       false, false);
	}

	return ret;
}

static ssize_t show_rx_skip_info(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int ret = 0;

	if (adapter->priv_flags & RNP_PRIV_FLAG_RX_SKIP_EN) {
		ret += sprintf(buf + ret, "rx skip bytes: %d\n",
			       16 * (adapter->priv_skip_count + 1));
	} else {
		ret += sprintf(buf + ret, "rx skip off\n");
	}

	return ret;
}

static ssize_t store_rx_skip_info(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int ret = count;
	u32 rx_skip_count;

	if (0 != kstrtou32(buf, 0, &rx_skip_count))
		return -EINVAL;

	if ((rx_skip_count > 0) && (rx_skip_count < 17)) {
		adapter->priv_skip_count = rx_skip_count - 1;
		adapter->priv_flags |= RNP_PRIV_FLAG_RX_SKIP_EN;
		hw->ops.set_rx_skip(hw, adapter->priv_skip_count, true);

	} else {
		adapter->priv_flags &= ~RNP_PRIV_FLAG_RX_SKIP_EN;

		hw->ops.set_rx_skip(hw, adapter->priv_skip_count, false);

		return -EINVAL;
	}

	return ret;
}

static ssize_t show_rx_drop_info(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int ret = 0;

	ret += sprintf(buf + ret, "rx_drop_status %llx\n",
		       adapter->rx_drop_status);

	return ret;
}

static ssize_t store_rx_drop_info(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int ret = count;
	u64 rx_drop_status;

	if (0 != kstrtou64(buf, 0, &rx_drop_status))
		return -EINVAL;

	adapter->rx_drop_status = rx_drop_status;

	hw->ops.update_rx_drop(hw);

	return ret;
}

static ssize_t show_outer_vlan_info(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int ret = 0;

	if (adapter->priv_flags & RNP_PRIV_FLAG_DOUBLE_VLAN)
		ret += sprintf(buf + ret, "double vlan on\n");
	else
		ret += sprintf(buf + ret, "double vlan off\n");

	switch (adapter->outer_vlan_type) {
	case outer_vlan_type_88a8:
		ret += sprintf(buf + ret, "outer vlan 0x88a8\n");

		break;
	case outer_vlan_type_9100:
		ret += sprintf(buf + ret, "outer vlan 0x9100\n");

		break;
	case outer_vlan_type_9200:
		ret += sprintf(buf + ret, "outer vlan 0x9200\n");

		break;
	default:
		ret += sprintf(buf + ret, "outer vlan error\n");
		break;
	}
	return ret;
}

static ssize_t store_outer_vlan_info(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int ret = count;
	u32 outer_vlan_type;

	if (0 != kstrtou32(buf, 0, &outer_vlan_type))
		return -EINVAL;
	/* should check tx_ring_num is valid */
	if (outer_vlan_type < outer_vlan_type_max) {
		adapter->outer_vlan_type = outer_vlan_type;
	} else
		ret = -EINVAL;
	/* should update to hw */
	if (hw->ops.set_outer_vlan_type)
		hw->ops.set_outer_vlan_type(hw, outer_vlan_type);

	return ret;
}

static ssize_t show_tx_stags_info(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int ret = 0;

	if (adapter->flags2 & RNP_FLAG2_VLAN_STAGS_ENABLED)
		ret += sprintf(buf + ret, "tx stags on\n");
	else
		ret += sprintf(buf + ret, "tx stags off\n");

	ret += sprintf(buf + ret, "vid 0x%x\n", adapter->stags_vid);

	return ret;
}

static ssize_t store_tx_stags_info(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_eth_info *eth = &hw->eth;
	int ret = count;
	u16 tx_stags;

	if (0 != kstrtou16(buf, 0, &tx_stags))
		return -EINVAL;
	if (tx_stags < VLAN_N_VID) {
		adapter->stags_vid = tx_stags;
	} else
		ret = -EINVAL;
	/* should update vlan filter */
	eth->ops.set_vfta(eth, adapter->stags_vid, true);

	return ret;
}

static ssize_t show_tx_desc_info(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	u32 tx_ring_num = adapter->sysfs_tx_ring_num;
	u32 tx_desc_num = adapter->sysfs_tx_desc_num;
	struct rnp_ring *ring = adapter->tx_ring[tx_ring_num];
	int ret = 0;
	struct rnp_tx_desc *desc;

	if (test_bit(__RNP_DOWN, &adapter->state)) {
		ret += sprintf(buf + ret, "port not up \n");
		return ret;
	}

	desc = RNP_TX_DESC(ring, tx_desc_num);
	ret += sprintf(buf + ret, "tx ring %d desc %d:\n", tx_ring_num,
		       tx_desc_num);
	ret += print_desc(buf + ret, desc, sizeof(*desc));
	ret += sprintf(buf + ret, "\n");

	return ret;
}

static ssize_t store_tx_desc_info(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int ret = count;
	u32 tx_desc_num = adapter->sysfs_tx_desc_num;
	u32 tx_ring_num = adapter->sysfs_tx_ring_num;
	struct rnp_ring *ring = adapter->tx_ring[tx_ring_num];

	if (0 != kstrtou32(buf, 0, &tx_desc_num))
		return -EINVAL;
	if (tx_desc_num < ring->count)
		adapter->sysfs_tx_desc_num = tx_desc_num;
	else
		ret = -EINVAL;

	return ret;
}

static ssize_t show_para_info(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_eth_info *eth = &hw->eth;
	struct rnp_mac_info *mac = &hw->mac;

	ret += sprintf(buf + ret, "nsi_en:%d\n", hw->ncsi_en);
	ret += sprintf(
		buf + ret,
		"eth: \n\tmc_filter_type:%u, mcft_size:%u, vft_size:%u, "
		"num_rar_entries:%u,\n"
		"\trar_highwater:%u, rx_pb_size:%u, max_tx_queues:%u, "
		"max_rx_queues:%u, \n"
		"\treg_off:%u, orig_autoc:%u, cached_autoc:%u, orig_autoc2:%u\n",
		eth->mc_filter_type, eth->mcft_size, eth->vft_size,
		eth->num_rar_entries, eth->rar_highwater, eth->rx_pb_size,
		eth->max_tx_queues, eth->max_rx_queues, eth->reg_off,
		eth->orig_autoc, eth->cached_autoc, eth->orig_autoc2);

	ret += sprintf(
		buf + ret,
		"mac:\n\t"
		"mc_filter_type:%u mcft_size:%u vft_size:%u num_rar_entries:%u \n"
		"\trar_highwater:%u rx_pb_size:%u max_tx_queues:%u max_rx_queues:%u \n"
		"\treg_off:%u orig_autoc:%u cached_autoc:%u orig_autoc2:%u "
		"orig_link_settings_stored:%u \n"
		"\tautotry_restart:%u mac_flags:%u\n",
		mac->mc_filter_type, mac->mcft_size, mac->vft_size,
		mac->num_rar_entries, mac->rar_highwater, mac->rx_pb_size,
		mac->max_tx_queues, mac->max_rx_queues, mac->reg_off,
		mac->orig_autoc, mac->cached_autoc, mac->orig_autoc2,
		mac->orig_link_settings_stored, mac->autotry_restart,
		mac->mac_flags);

	return ret;
}

static ssize_t show_rx_ring_info(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	u32 rx_ring_num = adapter->sysfs_rx_ring_num;
	struct rnp_ring *ring = adapter->rx_ring[rx_ring_num];
	int ret = 0;
	union rnp_rx_desc *rx_desc;

	ret += sprintf(buf + ret, "queue %d info:\n", rx_ring_num);
	ret += sprintf(buf + ret, "next_to_use %d\n", ring->next_to_use);
	ret += sprintf(buf + ret, "next_to_clean %d\n", ring->next_to_clean);
	rx_desc = RNP_RX_DESC(ring, ring->next_to_clean);
	ret += sprintf(buf + ret, "next_to_clean desc: ");
	ret += print_desc(buf + ret, rx_desc, sizeof(*rx_desc));
	ret += sprintf(buf + ret, "\n");

	return ret;
}

static ssize_t store_rx_ring_info(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int ret = count;

	u32 rx_ring_num = adapter->sysfs_rx_ring_num;

	if (0 != kstrtou32(buf, 0, &rx_ring_num))
		return -EINVAL;
	if (rx_ring_num < adapter->num_rx_queues)
		adapter->sysfs_rx_ring_num = rx_ring_num;
	else
		ret = -EINVAL;

	return ret;
}

static ssize_t show_tx_ring_info(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	u32 tx_ring_num = adapter->sysfs_tx_ring_num;
	struct rnp_ring *ring = adapter->tx_ring[tx_ring_num];
	int ret = 0;
	struct rnp_tx_buffer *tx_buffer;
	struct rnp_tx_desc *eop_desc;

	ret += sprintf(buf + ret, "queue %d info:\n", tx_ring_num);
	ret += sprintf(buf + ret, "next_to_use %d\n", ring->next_to_use);
	ret += sprintf(buf + ret, "next_to_clean %d\n", ring->next_to_clean);

	tx_buffer = &ring->tx_buffer_info[ring->next_to_clean];
	eop_desc = tx_buffer->next_to_watch;
	/* if have watch desc */
	if (eop_desc) {
		ret += sprintf(buf + ret, "next_to_watch:\n");
		ret += print_desc(buf + ret, eop_desc, sizeof(*eop_desc));
		ret += sprintf(buf + ret, "\n");
	} else {
		ret += sprintf(buf + ret, "no next_to_watch data\n");
	}

	return ret;
}

static ssize_t store_tx_ring_info(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int ret = count;

	u32 tx_ring_num = adapter->sysfs_tx_ring_num;

	if (0 != kstrtou32(buf, 0, &tx_ring_num))
		return -EINVAL;
	if (tx_ring_num < adapter->num_tx_queues)
		adapter->sysfs_tx_ring_num = tx_ring_num;
	else
		ret = -EINVAL;

	return ret;
}

static ssize_t show_tx_counter(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	u32 val = 0;
	int i, ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	ret += sprintf(buf + ret, "tx counters\n");
	for (i = 0; i < 4; i++) {
		ret += sprintf(buf + ret, "ring%d-tx:\n", i);

		val = rd32(hw, RNP10_RING_BASE + RING_OFFSET(i) +
				       RNP_DMA_REG_TX_DESC_BUF_LEN);
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n", "len:",
			       RNP10_RING_BASE + RING_OFFSET(i) +
				       RNP_DMA_REG_TX_DESC_BUF_LEN,
			       val);

		val = rd32(hw, RNP10_RING_BASE + RING_OFFSET(i) +
				       RNP_DMA_REG_TX_DESC_BUF_HEAD);
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n", "head:",
			       RNP10_RING_BASE + RING_OFFSET(i) +
				       RNP_DMA_REG_TX_DESC_BUF_HEAD,
			       val);

		val = rd32(hw, RNP10_RING_BASE + RING_OFFSET(i) +
				       RNP_DMA_REG_TX_DESC_BUF_TAIL);
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n", "tail:",
			       RNP10_RING_BASE + RING_OFFSET(i) +
				       RNP_DMA_REG_TX_DESC_BUF_TAIL,
			       val);
	}

	ret += sprintf(buf + ret, "to_1to4_p1:\n");

	val = rd32(hw, RNP_ETH_1TO4_INST0_IN_PKTS);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "emac_in:", RNP_ETH_1TO4_INST0_IN_PKTS, val);

	val = rd32(hw, RNP_ETH_IN_0_TX_PKT_NUM(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "emac_send:", RNP_ETH_IN_0_TX_PKT_NUM(0), val);

	ret += sprintf(buf + ret, "to_1to4_p2:\n");

	val = rd32(hw, RNP_ETH_IN_1_TX_PKT_NUM(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "sop_pkt:", RNP_ETH_IN_1_TX_PKT_NUM(0), val);

	val = rd32(hw, RNP_ETH_IN_2_TX_PKT_NUM(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "eop_pkt:", RNP_ETH_IN_2_TX_PKT_NUM(0), val);

	val = rd32(hw, RNP_ETH_IN_3_TX_PKT_NUM(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "send_terr:", RNP_ETH_IN_3_TX_PKT_NUM(0), val);

	ret += sprintf(buf + ret, "to_tx_trans(phy):\n");

	val = rd32(hw, RNP_ETH_EMAC_TX_TO_PHY_PKTS(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "in:", RNP_ETH_EMAC_TX_TO_PHY_PKTS(0), val);

	val = rd32(hw, RNP_ETH_TXTRANS_PTP_PKT_NUM(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "out:", RNP_ETH_TXTRANS_PTP_PKT_NUM(0), val);

	ret += sprintf(buf + ret, "mac:\n");

	val = rd32(hw, 0x60000);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "mac-tx-cfg:", 0x60000, val);

	val = rd32(hw, 0x1081c);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n", "mac-tx:", 0x1081c,
		       val);

	val = rd32(hw, 0x1087c);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "underflow_err:", 0x1087c, val);

	val = rd32(hw, RNP_ETH_TX_DEBUG(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "port0_txtrans_sop:", RNP_ETH_TX_DEBUG(0), val);

	val = rd32(hw, RNP_ETH_TX_DEBUG(4));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "port0_txtrans_eop:", RNP_ETH_TX_DEBUG(4), val);

	val = rd32(hw, RNP_ETH_TX_DEBUG(13));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "tx_empty:", RNP_ETH_TX_DEBUG(13), val);

	val = rd32(hw, RNP_ETH_TX_DEBUG(14));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: 0x%x\n",
		       "tx_prog_full:", RNP_ETH_TX_DEBUG(14), val);

	val = rd32(hw, RNP_ETH_TX_DEBUG(15));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: 0x%x\n",
		       "tx_full:", RNP_ETH_TX_DEBUG(15), val);

	return ret;
}

static DEVICE_ATTR(tx_counter, S_IRUGO | S_IWUSR, show_tx_counter, NULL);

static ssize_t show_rx_counter(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	u32 val = 0, port = 0;
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	ret += sprintf(buf + ret, "rx counters\n");
	for (port = 0; port < 4; port++) {
		ret += sprintf(buf + ret, "emac_rx_trans (port:%d):\n", port);

		val = rd32(hw, RNP_XLMAC + 0x900 + port * 0x10000);
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "mac-pkts:", RNP_XLMAC + 0x900 + port * 0x10000,
			       val);

		val = rd32(hw, RNP_RXTRANS_RX_PKTS(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "pkts:", RNP_RXTRANS_RX_PKTS(port), val);

		val = rd32(hw, RNP_RXTRANS_DROP_PKTS(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "drop:", RNP_RXTRANS_DROP_PKTS(port), val);

		val = rd32(hw, RNP_RXTRANS_WDT_ERR_PKTS(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "wdt_err:", RNP_RXTRANS_WDT_ERR_PKTS(port), val);

		val = rd32(hw, RNP_RXTRANS_CODE_ERR_PKTS(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "code_err:", RNP_RXTRANS_CODE_ERR_PKTS(port),
			       val);

		val = rd32(hw, RNP_RXTRANS_CRC_ERR_PKTS(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "crc_err:", RNP_RXTRANS_CRC_ERR_PKTS(port), val);

		val = rd32(hw, RNP_RXTRANS_SLEN_ERR_PKTS(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "slen_err:", RNP_RXTRANS_SLEN_ERR_PKTS(port),
			       val);

		val = rd32(hw, RNP_RXTRANS_GLEN_ERR_PKTS(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "glen_err:", RNP_RXTRANS_GLEN_ERR_PKTS(port),
			       val);

		val = rd32(hw, RNP_RXTRANS_IPH_ERR_PKTS(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "iph_err:", RNP_RXTRANS_IPH_ERR_PKTS(port), val);

		val = rd32(hw, RNP_RXTRANS_CSUM_ERR_PKTS(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "csum_err:", RNP_RXTRANS_CSUM_ERR_PKTS(port),
			       val);

		val = rd32(hw, RNP_RXTRANS_LEN_ERR_PKTS(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "len_err:", RNP_RXTRANS_LEN_ERR_PKTS(port), val);

		val = rd32(hw, RNP_RXTRANS_CUT_ERR_PKTS(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "trans_cut_err:", RNP_RXTRANS_CUT_ERR_PKTS(port),
			       val);

		val = rd32(hw, RNP_RXTRANS_EXCEPT_BYTES(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       "expt_byte_err:", RNP_RXTRANS_EXCEPT_BYTES(port),
			       val);

		val = rd32(hw, RNP_RXTRANS_G1600_BYTES_PKTS(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
			       ">1600Byte:", RNP_RXTRANS_G1600_BYTES_PKTS(port),
			       val);
	}

	ret += sprintf(buf + ret, "gather:\n");
	val = rd32(hw, RNP_ETH_TOTAL_GAT_RX_PKT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "total_in_pkts:", RNP_ETH_TOTAL_GAT_RX_PKT_NUM, val);

	port = 0;
	val = rd32(hw, RNP_ETH_RX_PKT_NUM(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "to_nxt_mdodule:", RNP_ETH_RX_PKT_NUM(port), val);

	for (port = 0; port < 4; port++) {
		u8 pname[16] = { 0 };
		val = rd32(hw, RNP_ETH_RX_PKT_NUM(port));
		sprintf(pname, "p%d-rx:", port);
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n", pname,
			       RNP_ETH_RX_PKT_NUM(port), val);
	}

	for (port = 0; port < 4; port++) {
		u8 pname[16] = { 0 };
		val = rd32(hw, RNP_ETH_RX_DROP_PKT_NUM(port));
		sprintf(pname, "p%d-drop:", port);
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n", pname,
			       RNP_ETH_RX_DROP_PKT_NUM(port), val);
	}

	ret += sprintf(buf + ret, "debug:\n");
	val = rd32(hw, RNP_ETH_RX_DEBUG(10));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "data_eop:", RNP_ETH_RX_DEBUG(10), val);
	val = rd32(hw, RNP_ETH_RX_DEBUG(11));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "data_descs:", RNP_ETH_RX_DEBUG(11), val);
	val = rd32(hw, RNP_ETH_RX_DEBUG(12));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "data_desc_sop:", RNP_ETH_RX_DEBUG(12), val);
	val = rd32(hw, RNP_ETH_RX_DEBUG(13));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "data_desc_eop:", RNP_ETH_RX_DEBUG(13), val);
	val = rd32(hw, RNP_ETH_RX_DEBUG(14));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "4to1_gather_sop:", RNP_ETH_RX_DEBUG(14), val);
	val = rd32(hw, RNP_ETH_RX_DEBUG(15));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "4to1_gather_eop:", RNP_ETH_RX_DEBUG(15), val);

	ret += sprintf(buf + ret, "ip-parse:\n");

	val = rd32(hw, RNP_ETH_PKT_EGRESS_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "pkg_egree:", RNP_ETH_PKT_EGRESS_NUM, val);

	val = rd32(hw, RNP_ETH_PKT_IP_HDR_LEN_ERR_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "L3_len_err:", RNP_ETH_PKT_IP_HDR_LEN_ERR_NUM, val);

	val = rd32(hw, RNP_ETH_PKT_IP_PKT_LEN_ERR_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "ip_hdr_err:", RNP_ETH_PKT_IP_PKT_LEN_ERR_NUM, val);

	val = rd32(hw, RNP_ETH_PKT_L3_HDR_CHK_ERR_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "l3-csum-err:", RNP_ETH_PKT_L3_HDR_CHK_ERR_NUM, val);

	val = rd32(hw, RNP_ETH_PKT_L4_HDR_CHK_ERR_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "l4-csum-err:", RNP_ETH_PKT_L4_HDR_CHK_ERR_NUM, val);

	val = rd32(hw, RNP_ETH_PKT_SCTP_CHK_ERR_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "sctp-err:", RNP_ETH_PKT_SCTP_CHK_ERR_NUM, val);

	val = rd32(hw, RNP_ETH_PKT_VLAN_ERR_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "vlan-err:", RNP_ETH_PKT_VLAN_ERR_NUM, val);

	val = rd32(hw, RNP_ETH_PKT_EXCEPT_SHORT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "except_short_num:", RNP_ETH_PKT_EXCEPT_SHORT_NUM, val);

	val = rd32(hw, RNP_ETH_PKT_PTP_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "ptp:", RNP_ETH_PKT_PTP_NUM, val);

	ret += sprintf(buf + ret, "to-indecap:\n");

	val = rd32(hw, RNP_ETH_DECAP_PKT_IN_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "*in engin*:", RNP_ETH_DECAP_PKT_IN_NUM, val);

	val = rd32(hw, RNP_ETH_DECAP_PKT_OUT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "*out engin*:", RNP_ETH_DECAP_PKT_OUT_NUM, val);

	val = rd32(hw, RNP_ETH_DECAP_DMAC_OUT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "to-dma/host:", RNP_ETH_DECAP_DMAC_OUT_NUM, val);

	val = rd32(hw, RNP_ETH_DECAP_BMC_OUT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "to-bmc:", RNP_ETH_DECAP_BMC_OUT_NUM, val);

	val = rd32(hw, RNP_ETH_DECAP_SW_OUT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "to-switch:", RNP_ETH_DECAP_SW_OUT_NUM, val);

	val = rd32(hw, RNP_ETH_DECAP_MIRROR_OUT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "bmc+host:", RNP_ETH_DECAP_MIRROR_OUT_NUM, val);

	val = rd32(hw, RNP_ETH_DECAP_PKT_DROP_NUM(0x0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "err_drop:", RNP_ETH_DECAP_PKT_DROP_NUM(0x0), val);

	val = rd32(hw, RNP_ETH_DECAP_PKT_DROP_NUM(1));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "plicy_drop:", RNP_ETH_DECAP_PKT_DROP_NUM(1), val);

	val = rd32(hw, RNP_ETH_DECAP_PKT_DROP_NUM(2));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "dmac_drop:", RNP_ETH_DECAP_PKT_DROP_NUM(2), val);

	val = rd32(hw, RNP_ETH_DECAP_PKT_DROP_NUM(3));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "bmc_drop:", RNP_ETH_DECAP_PKT_DROP_NUM(3), val);

	val = rd32(hw, RNP_ETH_DECAP_PKT_DROP_NUM(4));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "sw_drop:", RNP_ETH_DECAP_PKT_DROP_NUM(4), val);

	val = rd32(hw, RNP_ETH_DECAP_PKT_DROP_NUM(5));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "rm_vlane_num:", RNP_ETH_DECAP_PKT_DROP_NUM(5), val);

	ret += sprintf(buf + ret, "\npolicy-drop-reason:\n");
	val = rd32(hw, RNP_ETH_BASE + RNP10_ETH_RX_DEBUG(4));
	ret += sprintf(buf + ret, "\t %30s 0x%08x: %d\n", "host_l2_match_drop:",
		       RNP_ETH_BASE + RNP10_ETH_RX_DEBUG(4), val);
	val = rd32(hw, RNP_ETH_BASE + RNP10_ETH_RX_DEBUG(5));
	ret += sprintf(buf + ret, "\t %30s 0x%08x: %d\n",
		       "redir_input_match_drop:",
		       RNP_ETH_BASE + RNP10_ETH_RX_DEBUG(5), val);
	val = rd32(hw, RNP_ETH_BASE + RNP10_ETH_RX_DEBUG(6));
	ret += sprintf(buf + ret, "\t %30s 0x%08x: %d\n",
		       "redir_etypt_match_drop:",
		       RNP_ETH_BASE + RNP10_ETH_RX_DEBUG(6), val);
	val = rd32(hw, RNP_ETH_BASE + RNP10_ETH_RX_DEBUG(7));
	ret += sprintf(buf + ret, "\t %30s 0x%08x: %d\n",
		       "redir_tcp_sync_match_drop:",
		       RNP_ETH_BASE + RNP10_ETH_RX_DEBUG(7), val);
	val = rd32(hw, RNP_ETH_BASE + RNP10_ETH_RX_DEBUG(8));
	ret += sprintf(buf + ret, "\t %30s 0x%08x: %d\n",
		       "redir_tuple5_match_drop:",
		       RNP_ETH_BASE + RNP10_ETH_RX_DEBUG(8), val);
	val = rd32(hw, RNP_ETH_BASE + RNP10_ETH_RX_DEBUG(9));
	ret += sprintf(buf + ret, "\t %30s 0x%08x: %d\n",
		       "recdir_tcam_match_drop:",
		       RNP_ETH_BASE + RNP10_ETH_RX_DEBUG(9), val);

	ret += sprintf(buf + ret, "dma-2-host:\n");

	val = rd32(hw, 0x264);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n", "fifo equ:", 0x264,
		       val);

	val = rd32(hw, 0x268);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n", "fifo deq:", 0x268,
		       val);

	val = rd32(hw, 0x114);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "unexpt_abtring:", 0x114, val);

	val = rd32(hw, 0x288);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n", "pci2host:", 0x288,
		       val);

	for (port = 0; port < 4; port++) {
		ret += sprintf(buf + ret, "rx-ring%d:\n", port);

		val = rd32(hw, RNP10_RING_BASE + RING_OFFSET(port) +
				       RNP_DMA_REG_RX_DESC_BUF_HEAD);
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n", "head:",
			       RNP10_RING_BASE + RING_OFFSET(port) +
				       RNP_DMA_REG_RX_DESC_BUF_HEAD,
			       val);

		val = rd32(hw, RNP10_RING_BASE + RING_OFFSET(port) +
				       RNP_DMA_REG_RX_DESC_BUF_TAIL);
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n", "tail:",
			       RNP10_RING_BASE + RING_OFFSET(port) +
				       RNP_DMA_REG_RX_DESC_BUF_TAIL,
			       val);

		val = rd32(hw, RNP10_RING_BASE + RING_OFFSET(port) +
				       RNP_DMA_REG_RX_DESC_BUF_LEN);
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n", "len:",
			       RNP10_RING_BASE + RING_OFFSET(port) +
				       RNP_DMA_REG_RX_DESC_BUF_LEN,
			       val);
	}

	/* maybe too large */
	if (ret >= PAGE_SIZE)
		ret = PAGE_SIZE;

	return ret;
}

static DEVICE_ATTR(rx_counter, S_IRUGO | S_IWUSR, show_rx_counter, NULL);

static ssize_t show_active_vid(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	u16 vid;
	u16 current_vid = 0;
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	u8 vfnum = hw->max_vfs - 1;
	/* use last-vf's table entry. the last one */

	if ((adapter->flags & RNP_FLAG_SRIOV_ENABLED)) {
		current_vid = rd32(hw, RNP_DMA_PORT_VEB_VID_TBL(adapter->port,
								vfnum));
	}

	for_each_set_bit(vid, adapter->active_vlans, VLAN_N_VID) {
		ret += sprintf(buf + ret, "%u%s ", vid,
			       (current_vid == vid ? "*" : ""));
	}
	ret += sprintf(buf + ret, "\n");
	return ret;
}

static ssize_t store_active_vid(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	u16 vid;
	int err = -EINVAL;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	u8 vfnum = hw->max_vfs - 1;
	int port = 0;

	if (!(adapter->flags & RNP_FLAG_SRIOV_ENABLED))
		return -EIO;

	if (0 != kstrtou16(buf, 0, &vid))
		return -EINVAL;

	if ((vid < 4096) && test_bit(vid, adapter->active_vlans)) {
		if (rd32(hw, RNP_DMA_VERSION) >= 0x20201231) {
			for (port = 0; port < 4; port++)
				wr32(hw, RNP_DMA_PORT_VEB_VID_TBL(port, vfnum),
				     vid);
		} else {
			wr32(hw, RNP_DMA_PORT_VEB_VID_TBL(adapter->port, vfnum),
			     vid);
		}
		err = 0;
	}

	return err ? err : count;
}

static inline int pn_sn_dlen(char *v, int v_len)
{
	int i, len = 0;
	for (i = 0; i < v_len; i++) {
		if (isascii(v[i])) {
			len++;
		} else {
			break;
		}
	}
	return len;
}

static int rnp_mbx_get_pn_sn(struct rnp_hw *hw, char pn[33], char sn[33])
{
	struct maintain_req *req;
	void *dma_buf = NULL;
	dma_addr_t dma_phy;
	struct ucfg_mac_sn *cfg;

	int err = 0, bytes = sizeof(*req) + sizeof(struct ucfg_mac_sn);

	memset(pn, 0, 33);
	memset(sn, 0, 33);

	dma_buf =
		dma_alloc_coherent(&hw->pdev->dev, bytes, &dma_phy, GFP_KERNEL);
	if (!dma_buf) {
		printk("%s: no memory:%d!", __func__, bytes);
		return -ENOMEM;
	}

	req = (struct maintain_req *)dma_buf;
	memset(dma_buf, 0, bytes);
	cfg = (struct ucfg_mac_sn *)(req + 1);
	req->magic = MAINTAIN_MAGIC;
	req->cmd = 0;
	req->arg0 = 3;
	req->req_data_bytes = 0;
	req->reply_bytes = bytes - sizeof(*req);

	err = rnp_maintain_req(hw, req->cmd, req->arg0, req->req_data_bytes,
			       req->reply_bytes, dma_phy);
	if (err != 0) {
		goto err_quit;
	}
	if (cfg->magic == MAC_SN_MAGIC) {
		int sz = pn_sn_dlen(cfg->pn, 32);
		if (sz) {
			memcpy(pn, cfg->pn, sz);
			pn[sz] = 0;
		}
		sz = pn_sn_dlen(cfg->sn, 32);
		if (sz) {
			memcpy(sn, cfg->sn, sz);
			sn[sz] = 0;
		}
	}

err_quit:
	if (dma_buf)
		dma_free_coherent(&hw->pdev->dev, bytes, dma_buf, dma_phy);

	return 0;
}

static ssize_t show_own_vpd(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	char pn[33] = { 0 }, sn[33] = { 0 };

	rnp_mbx_get_pn_sn(hw, pn, sn);

	ret += sprintf(
		buf + ret, "Product Name: %s\n",
		"Ethernet Controller N10 Series for 10GbE or 40GbE (Dual-port)");
	ret += sprintf(buf + ret, "[PN] Part number: %s\n", pn);
	ret += sprintf(buf + ret, "[SN] Serial number: %s\n", sn);

	return ret;
}
static DEVICE_ATTR(own_vpd, S_IRUGO, show_own_vpd, NULL);

static ssize_t show_port_idx(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);

	ret += sprintf(buf, "%d\n", adapter->portid_of_card);
	return ret;
}
static DEVICE_ATTR(port_idx, S_IRUGO | S_IRUSR, show_port_idx, NULL);

static ssize_t show_debug_linkstat(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	ret += sprintf(buf, "%d %d dumy:0x%x up-flag:%d carry:%d\n",
		       adapter->link_up, adapter->hw.link, rd32(hw, 0xc),
		       adapter->flags & RNP_FLAG_NEED_LINK_UPDATE,
		       netif_carrier_ok(netdev));
	return ret;
}

static DEVICE_ATTR(debug_linkstat, S_IRUGO | S_IRUSR, show_debug_linkstat,
		   NULL);

static ssize_t show_sfp(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	if (rnp_mbx_get_lane_stat(hw) != 0) {
		ret += sprintf(buf, " IO Error\n");
	} else {
		ret += sprintf(
			buf, "mod-abs:%d\ntx-fault:%d\ntx-dis:%d\nrx-los:%d\n",
			adapter->sfp.mod_abs, adapter->sfp.fault,
			adapter->sfp.tx_dis, adapter->sfp.los);
	}

	return ret;
}
static DEVICE_ATTR(sfp, S_IRUGO | S_IRUSR, show_sfp, NULL);

static ssize_t store_pci(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int err = -EINVAL;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int gen = 3, lanes = 8;

	if (count > 30)
		return -EINVAL;

	if (sscanf(buf, "gen%dx%d", &gen, &lanes) != 2) {
		printk("Error: invalid input. example: gen3x8\n");
		return -EINVAL;
	}
	if (gen > 3 || lanes > 8)
		return -EINVAL;

	err = rnp_set_lane_fun(hw, LANE_FUN_PCI_LANE, gen, lanes, 0, 0);

	return err ? err : count;
}

static ssize_t show_pci(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	if (rnp_mbx_get_lane_stat(hw) != 0) {
		ret += sprintf(buf, " IO Error\n");
	} else {
		ret += sprintf(buf, "gen%dx%d\n", hw->pci_gen, hw->pci_lanes);
	}

	return ret;
}

static DEVICE_ATTR(pci, S_IRUGO | S_IWUSR | S_IRUSR, show_pci, store_pci);

static ssize_t store_sfp_tx_disable(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int err = -EINVAL;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	long enable = 0;

	if (kstrtol(buf, 10, &enable)) {
		return -EINVAL;
	}

	err = rnp_set_lane_fun(hw, LANE_FUN_SFP_TX_DISABLE, !!enable, 0, 0, 0);

	return err ? err : count;
}

static ssize_t show_sfp_tx_disable(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	if (rnp_mbx_get_lane_stat(hw) != 0) {
		ret += sprintf(buf, " IO Error\n");
	} else {
		ret += sprintf(buf, "%d\n", adapter->sfp.tx_dis);
	}

	return ret;
}

static DEVICE_ATTR(sfp_tx_disable, S_IRUGO | S_IWUSR | S_IRUSR,
		   show_sfp_tx_disable, store_sfp_tx_disable);

static ssize_t store_link_traing(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	int err = -EINVAL;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	long enable = 0;

	if (kstrtol(buf, 10, &enable)) {
		return -EINVAL;
	}

	err = rnp_set_lane_fun(hw, LANE_FUN_LINK_TRAING, !!enable, 0, 0, 0);

	return err ? err : count;
}

static ssize_t show_link_traing(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	if (rnp_mbx_get_lane_stat(hw) != 0) {
		ret += sprintf(buf, " IO Error\n");
	} else {
		ret += sprintf(buf, "%d\n", adapter->link_traing);
	}

	return ret;
}

static DEVICE_ATTR(link_traing, S_IRUGO | S_IWUSR | S_IRUSR, show_link_traing,
		   store_link_traing);

static ssize_t store_fec(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int err = -EINVAL;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	long enable = 0;

	if (kstrtol(buf, 10, &enable)) {
		return -EINVAL;
	}

	err = rnp_set_lane_fun(hw, LANE_FUN_FEC, !!enable, 0, 0, 0);

	return err ? err : count;
}

static ssize_t show_fec(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	if (rnp_mbx_get_lane_stat(hw) != 0) {
		ret += sprintf(buf, " IO Error\n");
	} else {
		ret += sprintf(buf, "%d\n", adapter->fec);
	}

	return ret;
}

static DEVICE_ATTR(fec, S_IRUGO | S_IWUSR | S_IRUSR, show_fec, store_fec);

static ssize_t store_pcs(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	u32 reg_hi = 0, reg_lo = 0, pcs_base_regs = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int input_arg_cnt;
	u32 pcs_phy_regs[] = {
		0x00040000, 0x00041000, 0x00042000, 0x00043000,
		0x00040000, 0x00041000, 0x00042000, 0x00043000,
	};

	if (count > 64) {
		printk("Error: Input size >100: too large\n");
		return -EINVAL;
	}

	input_arg_cnt = sscanf(buf, "%u %x %x", &adapter->sysfs_pcs_lane_num,
			       &adapter->sysfs_bar4_reg_addr,
			       &adapter->sysfs_bar4_reg_val);

	if (input_arg_cnt != 2 && input_arg_cnt != 3) {
		printk("Error: Invalid Input: read lane x reg 0xXXX or write phy x reg "
		       "0xXXX val 0xXXX\n");
		return -EINVAL;
	}

	if (adapter->sysfs_pcs_lane_num > 8) {
		printk("Error: Invalid value. should in 0~7\n");
		return -EINVAL;
	}

	switch (input_arg_cnt) {
	case 2:
		reg_hi = adapter->sysfs_bar4_reg_addr >> 8;
		reg_lo = (adapter->sysfs_bar4_reg_addr & 0xff) << 2;
		pcs_base_regs = pcs_phy_regs[adapter->sysfs_pcs_lane_num];
		wr32(hw, pcs_base_regs + (0xff << 2), reg_hi);
		adapter->sysfs_bar4_reg_val = rd32(hw, pcs_base_regs + reg_lo);
		break;
	case 3:
		reg_hi = adapter->sysfs_bar4_reg_addr >> 8;
		reg_lo = (adapter->sysfs_bar4_reg_addr & 0xff) << 2;
		pcs_base_regs = pcs_phy_regs[adapter->sysfs_pcs_lane_num];
		wr32(hw, pcs_base_regs + (0xff << 2), reg_hi);
		wr32(hw, pcs_base_regs + reg_lo, adapter->sysfs_bar4_reg_val);
		break;
	default:
		printk("Error: Invalid value. input_arg_cnt=%d\n",
		       input_arg_cnt);
		break;
	}
	adapter->sysfs_input_arg_cnt = input_arg_cnt;

	return count;
}

static ssize_t show_pcs(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);

	switch (adapter->sysfs_input_arg_cnt) {
	case 2:
		ret += sprintf(buf, "lane%u pcs: 0x%x => 0x%x\n",
			       adapter->sysfs_pcs_lane_num,
			       adapter->sysfs_bar4_reg_addr,
			       adapter->sysfs_bar4_reg_val);
		break;
	case 3:
		ret += sprintf(buf, "lane%u pcs: 0x%x <= 0x%x\n",
			       adapter->sysfs_pcs_lane_num,
			       adapter->sysfs_bar4_reg_addr,
			       adapter->sysfs_bar4_reg_val);
		break;
	default:
		break;
	}

	return ret;
}

static DEVICE_ATTR(pcs_reg, S_IRUGO | S_IWUSR | S_IRUSR, show_pcs, store_pcs);

static ssize_t phy_reg_read(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int val = 0;
	int err = -EINVAL;
	int phy_reg = adapter->sysfs_phy_reg;

	if (hw) {
		if (adapter->sysfs_is_phy_ext_reg) {
			err = rnp_mbx_phy_read(hw, phy_reg | PHY_EXT_REG_FLAG,
					       &val);
		} else {
			err = rnp_mbx_phy_read(hw, phy_reg, &val);
		}
	}

	if (err) {
		return 0;
	} else {
		return sprintf(buf, "phy %s 0x%04x : 0x%04x\n",
			       adapter->sysfs_is_phy_ext_reg ? "ext reg" :
							       "reg",
			       phy_reg, val & 0xffff);
	}
}

static ssize_t phy_reg_write(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int i = 0, argc = 0, err = -EINVAL;
	char argv[3][16];
	unsigned long val[3] = { 0 };
	int phy_reg = 0;

	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	memset(argv, 0, sizeof(argv));
	argc = sscanf(buf, "%15s %15s %15s", argv[0], argv[1], argv[2]);

	if (argc < 1) {
		return -EINVAL;
	}

	adapter->sysfs_is_phy_ext_reg = 0;

	if (strcmp(argv[0], "ext") == 0) {
		adapter->sysfs_is_phy_ext_reg = 1;
	} else {
		if (kstrtoul(argv[0], 0, &val[0])) {
			return -EINVAL;
		}
	}

	for (i = 1; i < argc; i++) {
		if (kstrtoul(argv[i], 0, &val[i])) {
			return -EINVAL;
		}
	}

	if (argc == 1) {
		if (adapter->sysfs_is_phy_ext_reg) {
			return -EINVAL;
		} else {
			/* set phy reg index */
			phy_reg = val[0];
			err = 0;
		}
	}

	if (argc == 2) {
		if (adapter->sysfs_is_phy_ext_reg) {
			/* set ext phy reg index */
			phy_reg = val[1];
			err = 0;
		} else {
			/* write phy reg */
			phy_reg = val[0];
			err = rnp_mbx_phy_write(hw, phy_reg, val[1]);
		}
	}

	if (argc == 3) {
		if (adapter->sysfs_is_phy_ext_reg) {
			/* write ext phy reg */
			phy_reg = val[1];
			err = rnp_mbx_phy_write(hw, phy_reg | PHY_EXT_REG_FLAG,
						val[2]);
		} else {
			return -EINVAL;
		}
	}

	adapter->sysfs_phy_reg = phy_reg;

	return err ? err : count;
}

static DEVICE_ATTR(phy_reg, 0664, phy_reg_read, phy_reg_write);

static ssize_t store_prbs(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	int err = -EINVAL;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	long prbs = 0;

	if (kstrtol(buf, 10, &prbs)) {
		return -EINVAL;
	}

	err = rnp_set_lane_fun(hw, LANE_FUN_PRBS, prbs, 0, 0, 0);

	return err ? err : count;
}

static DEVICE_ATTR(prbs, S_IRUGO | S_IWUSR | S_IRUSR, NULL, store_prbs);

static ssize_t store_autoneg(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int err = -EINVAL;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	long enable = 0;

	if (kstrtol(buf, 10, &enable)) {
		return -EINVAL;
	}

	err = rnp_set_lane_fun(hw, LANE_FUN_AN, !!enable, 0, 0, 0);

	return err ? err : count;
}

static ssize_t show_autoneg(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	if (rnp_mbx_get_lane_stat(hw) != 0) {
		ret += sprintf(buf, " IO Error\n");
	} else {
		ret += sprintf(buf, "%d\n", adapter->an);
	}

	return ret;
}

static DEVICE_ATTR(autoneg, S_IRUGO | S_IWUSR | S_IRUSR, show_autoneg,
		   store_autoneg);

static ssize_t store_lane_si(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int err = -EINVAL;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int si_main = -1, si_pre = -1, si_post = -1, si_txboost = -1;
	int cnt;

	if (rnp_mbx_get_lane_stat(hw) != 0) {
		printk("Error: rnp_mbx_get_lane_stat failed\n");
		return -EIO;
	}
	if (count > 100) {
		printk("Error: Input size >100: too large\n");
		return -EINVAL;
	}

	if (hw->supported_link &
	    (RNP_LINK_SPEED_40GB_FULL | RNP_LINK_SPEED_25GB_FULL)) {
		u32 lane0_main, lane0_pre, lane0_post, lane0_boost;
		u32 lane1_main, lane1_pre, lane1_post, lane1_boost;
		u32 lane2_main, lane2_pre, lane2_post, lane2_boost;
		u32 lane3_main, lane3_pre, lane3_post, lane3_boost;

		cnt = sscanf(buf,
			     "%u %u %u %u,%u %u %u %u,%u %u %u %u,%u %u %u %u",
			     &lane0_main, &lane0_pre, &lane0_post, &lane0_boost,
			     &lane1_main, &lane1_pre, &lane1_post, &lane1_boost,
			     &lane2_main, &lane2_pre, &lane2_post, &lane2_boost,
			     &lane3_main, &lane3_pre, &lane3_post,
			     &lane3_boost);
		if (cnt != 16) {
			printk("Error: Invalid Input.\n"
			       "  <lane0_si>,<lane1_si>,<lane2_si>,<lane3_si>\n"
			       "  laneX_si: <main> <pre> <post> <boost>\n\n"
			       "   ie: 21 0 11 11,22 0 12 12,23 0 13 13,24 0 14 14 \n");

			return -EINVAL;
		}

		si_main = ((lane0_main & 0xff) << 0) |
			  ((lane1_main & 0xff) << 8) |
			  ((lane2_main & 0xff) << 16) |
			  ((lane3_main & 0xff) << 24);
		si_pre = ((lane0_pre & 0xff) << 0) | ((lane1_pre & 0xff) << 8) |
			 ((lane2_pre & 0xff) << 16) |
			 ((lane3_pre & 0xff) << 24);
		si_post = ((lane0_post & 0xff) << 0) |
			  ((lane1_post & 0xff) << 8) |
			  ((lane2_post & 0xff) << 16) |
			  ((lane3_post & 0xff) << 24);
		si_txboost = ((lane0_boost & 0xf) << 0) |
			     ((lane1_boost & 0xf) << 4) |
			     ((lane2_boost & 0xf) << 8) |
			     ((lane3_boost & 0xf) << 12);
		printk("%s: main:0x%x pre:0x%x post:0x%x boost:0x%x\n",
		       adapter->name, si_main, si_pre, si_post, si_txboost);
	} else {
		cnt = sscanf(buf, "%u %u %u %u", &si_main, &si_pre, &si_post,
			     &si_txboost);
		if (cnt != 4) {
			printk("Error: Invalid Input: <main> <pre> <post> <tx_boost>\n");
			return -EINVAL;
		}
		if (si_main > 63 || si_pre > 63 || si_post > 63) {
			printk("Error: Invalid value. should in 0~63\n");
			return -EINVAL;
		}
		if (si_txboost > 16) {
			printk("Error: Invalid txboost. should in 0~15\n");
			return -EINVAL;
		}
	}
	err = rnp_set_lane_fun(hw, LANE_FUN_SI, si_main, si_pre, si_post,
			       si_txboost);

	return err ? err : count;
}

static ssize_t show_lane_si(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	int ret = 0, i;
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	if (rnp_mbx_get_lane_stat(hw) != 0) {
		ret += sprintf(buf, " IO Error\n");
	} else {
		if (hw->supported_link &
		    (RNP_LINK_SPEED_40GB_FULL | RNP_LINK_SPEED_25GB_FULL)) {
			ret += sprintf(
				buf + ret,
				"main:0x%08x pre:0x%08x post:0x%08x tx_boost:0x%04x\n\n",
				adapter->si.main, adapter->si.pre,
				adapter->si.post, adapter->si.tx_boost);
			for (i = 0; i < 4; i++) {
				ret += sprintf(
					buf + ret,
					" lane%d main:%u pre:%u post:%u tx_boost:%u\n",
					i, (adapter->si.main >> (i * 8)) & 0xff,
					(adapter->si.pre >> (i * 8)) & 0xff,
					(adapter->si.post >> (i * 8)) & 0xff,
					(adapter->si.tx_boost >> (i * 4)) &
						0xf);
			}
		} else {
			ret += sprintf(
				buf + ret,
				"lane:%d main:%u pre:%u post:%u tx_boost:%u\n",
				hw->nr_lane, adapter->si.main, adapter->si.pre,
				adapter->si.post, adapter->si.tx_boost & 0xf);
		}
	}

	return ret;
}

static DEVICE_ATTR(si, S_IRUGO | S_IWUSR | S_IRUSR, show_lane_si,
		   store_lane_si);

static ssize_t show_temperature(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int ret = 0, temp = 0, voltage = 0;

	temp = rnp_mbx_get_temp(hw, &voltage);

	ret += sprintf(buf, "temp:%d oC  volatage:%d mV\n", temp, voltage);
	return ret;
}

static struct pci_dev *pcie_find_root_port_old(struct pci_dev *dev)
{
	while (1) {
		if (!pci_is_pcie(dev))
			break;
		if (pci_pcie_type(dev) == PCI_EXP_TYPE_ROOT_PORT)
			return dev;
		if (!dev->bus->self)
			break;
		dev = dev->bus->self;
	}
	return NULL;
}

static ssize_t show_root_slot_info(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int ret = 0;
	struct pci_dev *root_pdev = pcie_find_root_port_old(adapter->pdev);

	if (root_pdev) {
		ret += sprintf(buf + ret, "%02x:%02x.%x\n",
			       root_pdev->bus->number,
			       PCI_SLOT(root_pdev->devfn),
			       PCI_FUNC(root_pdev->devfn));
	}
	return ret;
}

static int do_switch_loopback_set(struct rnp_adapter *adapter, int en,
				  int sport_lane, int dport_lane)
{
	int v;
	struct rnp_hw *hw = &adapter->hw;

	printk("%s: %s %d -> %d en:%d\n", __func__,
	       netdev_name(adapter->netdev), sport_lane, dport_lane, en);

	if (en) {
		adapter->flags |= RNP_FLAG_SWITCH_LOOPBACK_EN;
	} else {
		adapter->flags &= ~RNP_FLAG_SWITCH_LOOPBACK_EN;
	}

	wr32(hw, RNP_ETH_INPORT_POLICY_REG(sport_lane),
	     BIT(29) | (dport_lane << 16));

	v = rd32(hw, RNP_ETH_INPORT_POLICY_VAL);
	if (en) {
		v |= BIT(sport_lane);
	} else {
		v &= ~BIT(sport_lane);
	}
	wr32(hw, RNP_ETH_INPORT_POLICY_VAL, v);

	v = mac_rd32(&hw->mac, RNP10_MAC_PKT_FLT);
	if (en) {
		v |= (RNP_RX_ALL | RNP_RX_ALL_MUL);
	} else {
		v &= ~(RNP_RX_ALL | RNP_RX_ALL_MUL);
	}
	mac_wr32(&hw->mac, RNP10_MAC_PKT_FLT, v);

	eth_wr32(&hw->eth, RNP10_ETH_DMAC_MCSTCTRL, 0x0);

	return 0;
}

static ssize_t _switch_loopback(struct rnp_adapter *adapter,
				const char *peer_eth, int en)
{
	struct net_device *peer_netdev = NULL;
	struct rnp_adapter *peer_adapter = NULL;
	char name[100];

	strncpy(name, peer_eth, sizeof(name));
	strim(name);

	printk("%s: nr_lane:%d peer_lane:%s en:%d\n", __func__, 0, peer_eth,
	       en);

	peer_netdev = dev_get_by_name(&init_net, name);
	if (!peer_netdev) {
		printk("canot' find %s\n", name);
		return -EINVAL;
	}
	peer_adapter = netdev_priv(peer_netdev);

	if (PCI_SLOT(peer_adapter->pdev->devfn) !=
	    PCI_SLOT(adapter->pdev->devfn)) {
		printk("%s %s not in same slot\n", netdev_name(adapter->netdev),
		       netdev_name(peer_adapter->netdev));
		dev_put(peer_netdev);
		return -EINVAL;
	}

	printk("%s: %s(%d)<->%s(%d)\n", __func__, netdev_name(adapter->netdev),
	       0, netdev_name(peer_adapter->netdev), 0);

	do_switch_loopback_set(adapter, en, 0,
			       rnp_is_pf1(&peer_adapter->hw) ? 4 : 0);
	do_switch_loopback_set(peer_adapter, en, 0,
			       rnp_is_pf1(&adapter->hw) ? 4 : 0);

	if (peer_netdev) {
		dev_put(peer_netdev);
	}

	return 0;
}

static ssize_t store_switch_loopback_on(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct rnp_adapter *adapter = netdev_priv(to_net_device(dev));

	return _switch_loopback(adapter, buf, 1) == 0 ? count : -EINVAL;
}

static DEVICE_ATTR(switch_loopback_on, 0664, NULL, store_switch_loopback_on);

static ssize_t store_switch_loopback_off(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct rnp_adapter *adapter = netdev_priv(to_net_device(dev));

	return _switch_loopback(adapter, buf, 0) == 0 ? count : -EINVAL;
}
static DEVICE_ATTR(switch_loopback_off, 0664, NULL, store_switch_loopback_off);
static DEVICE_ATTR(root_slot_info, 0644, show_root_slot_info, NULL);
static DEVICE_ATTR(temperature, S_IRUGO | S_IRUSR, show_temperature, NULL);
static DEVICE_ATTR(active_vid, 0644, show_active_vid, store_active_vid);
static DEVICE_ATTR(tx_ring_info, 0644, show_tx_ring_info, store_tx_ring_info);
static DEVICE_ATTR(rx_ring_info, 0644, show_rx_ring_info, store_rx_ring_info);
static DEVICE_ATTR(para_info, 0644, show_para_info, NULL);
static DEVICE_ATTR(tx_desc_info, 0644, show_tx_desc_info, store_tx_desc_info);
static DEVICE_ATTR(rx_desc_info, 0644, show_rx_desc_info, store_rx_desc_info);
static DEVICE_ATTR(ring_sriov_info, 0644, show_ring_sriov_info, store_ring_sriov_info);
static DEVICE_ATTR(rx_drop_info, 0644, show_rx_drop_info, store_rx_drop_info);
static DEVICE_ATTR(outer_vlan_info, 0644, show_outer_vlan_info,
		   store_outer_vlan_info);
static DEVICE_ATTR(tcp_sync_info, 0644, show_tcp_sync_info,
		   store_tcp_sync_info);
static DEVICE_ATTR(rx_skip_info, 0644, show_rx_skip_info, store_rx_skip_info);
static DEVICE_ATTR(tx_stags_info, 0644, show_tx_stags_info,
		   store_tx_stags_info);
static DEVICE_ATTR(version_info, 0644, show_version_info, NULL);
static struct attribute *dev_attrs[] = {
	&dev_attr_tx_stags_info.attr,
	&dev_attr_version_info.attr,
	&dev_attr_root_slot_info.attr,
	&dev_attr_active_vid.attr,
	&dev_attr_rx_drop_info.attr,
	&dev_attr_outer_vlan_info.attr,
	&dev_attr_tcp_sync_info.attr,
	&dev_attr_rx_skip_info.attr,
	&dev_attr_tx_ring_info.attr,
	&dev_attr_rx_ring_info.attr,
	&dev_attr_para_info.attr,
	&dev_attr_tx_desc_info.attr,
	&dev_attr_rx_desc_info.attr,
	&dev_attr_ring_sriov_info.attr,
	&dev_attr_tx_counter.attr,
	&dev_attr_rx_counter.attr,
	&dev_attr_own_vpd.attr,
	&dev_attr_port_idx.attr,
	&dev_attr_temperature.attr,
	&dev_attr_si.attr,
	&dev_attr_sfp.attr,
	&dev_attr_autoneg.attr,
	&dev_attr_sfp_tx_disable.attr,
	&dev_attr_fec.attr,
	&dev_attr_link_traing.attr,
	&dev_attr_pci.attr,
	&dev_attr_prbs.attr,
	&dev_attr_pcs_reg.attr,
	&dev_attr_phy_reg.attr,
	&dev_attr_debug_linkstat.attr,
	&dev_attr_switch_loopback_off.attr,
	&dev_attr_switch_loopback_on.attr,
	NULL,
};
static struct bin_attribute *dev_bin_attrs[] = {
	&bin_attr_maintain,
	NULL,
};
static struct attribute_group dev_attr_grp = {
	.attrs = dev_attrs,
	.bin_attrs = dev_bin_attrs,
};

static void rnp_sysfs_del_adapter(struct rnp_adapter __maybe_unused *adapter)
{
}

/* called from rnp_main.c */
void rnp_sysfs_exit(struct rnp_adapter *adapter)
{
	rnp_sysfs_del_adapter(adapter);
	sysfs_remove_group(&adapter->netdev->dev.kobj, &dev_attr_grp);
}

/* called from rnp_main.c */
int rnp_sysfs_init(struct rnp_adapter *adapter)
{
	int rc = 0;
	int flag;
#ifdef RNP_HWMON
	struct hwmon_buff *rnp_hwmon;
	struct device *hwmon_dev;
	unsigned int i;
#endif /* RNP_HWMON */

	flag = sysfs_create_group(&adapter->netdev->dev.kobj, &dev_attr_grp);
	if (flag != 0) {
		dev_err(&adapter->netdev->dev,
			"sysfs_create_group failed:flag:%d\n", flag);
		return flag;
	}
#ifdef RNP_HWMON
	/* If this method isn't defined we don't support thermals */
	if (adapter->hw.ops.init_thermal_sensor_thresh == NULL) {
		goto no_thermal;
	}

	/* Don't create thermal hwmon interface if no sensors present */
	if (adapter->hw.ops.init_thermal_sensor_thresh(&adapter->hw))
		goto no_thermal;

	rnp_hwmon = devm_kzalloc(&adapter->pdev->dev, sizeof(*rnp_hwmon),
				 GFP_KERNEL);

	if (!rnp_hwmon) {
		rc = -ENOMEM;
		goto exit;
	}

	adapter->rnp_hwmon_buff = rnp_hwmon;

	for (i = 0; i < RNP_MAX_SENSORS; i++) {
		/*
		 * Only create hwmon sysfs entries for sensors that have
		 * meaningful data for.
		 */
		if (adapter->hw.thermal_sensor_data.sensor[i].location == 0)
			continue;

		/* Bail if any hwmon attr struct fails to initialize */
		rc = rnp_add_hwmon_attr(adapter, i, RNP_HWMON_TYPE_CAUTION);
		if (rc)
			goto err;
		rc = rnp_add_hwmon_attr(adapter, i, RNP_HWMON_TYPE_LOC);
		if (rc)
			goto err;
		rc = rnp_add_hwmon_attr(adapter, i, RNP_HWMON_TYPE_TEMP);
		if (rc)
			goto err;
		rc = rnp_add_hwmon_attr(adapter, i, RNP_HWMON_TYPE_MAX);
		if (rc)
			goto err;
	}

	rnp_hwmon->groups[0] = &rnp_hwmon->group;
	rnp_hwmon->group.attrs = rnp_hwmon->attrs;

	hwmon_dev = devm_hwmon_device_register_with_groups(
		&adapter->pdev->dev, "rnp", rnp_hwmon, rnp_hwmon->groups);

	if (IS_ERR(hwmon_dev)) {
		rc = PTR_ERR(hwmon_dev);
		goto exit;
	}
no_thermal:
#endif /* RNP_HWMON */
	goto exit;

err:
	rnp_sysfs_exit(adapter);
exit:
	return rc;
}
