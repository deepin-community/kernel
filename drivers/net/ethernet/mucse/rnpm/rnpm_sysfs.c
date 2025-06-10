// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/sysfs.h>
#include <linux/firmware.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/hwmon.h>
#include <linux/ctype.h>
#include "rnpm.h"
#include "rnpm_common.h"
#include "rnpm_type.h"
#include "version.h"
#include "rnpm_mbx.h"
#include "rnpm_mbx_fw.h"

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

u32 bar4_reg_val;
u32 bar4_reg_addr;
u32 pcs_phy_num;
int pcs_cnt;

#define to_net_device(n) container_of(n, struct net_device, dev)

#define PHY_EXT_REG_FLAG 0x80000000
static u32 is_phy_ext_reg;
static u32 phy_reg = -1;
#ifndef NO_BIT_ATTRS
static ssize_t maintain_read(struct file *filp, struct kobject *kobj,
			     struct bin_attribute *attr, char *buf,
			     loff_t off, size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	int rbytes = count;

	if (adapter->maintain_buf == NULL)
		return 0;

	if (off + count > adapter->maintain_buf_len)
		rbytes = adapter->maintain_buf_len - off;
	memcpy(buf, adapter->maintain_buf + off, rbytes);

	/* end-of-buf */
	if ((off + rbytes) >= adapter->maintain_buf_len) {
		kfree(adapter->maintain_buf);
		adapter->maintain_buf = NULL;
		adapter->maintain_buf_len = 0;
	}

	return rbytes;
}

static ssize_t maintain_write(struct file *filp, struct kobject *kobj,
			      struct bin_attribute *attr, char *buf,
			      loff_t off, size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	int err = -EINVAL;
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_pf_adapter *pf_adpt = adapter->pf_adapter;
	struct rnpm_hw *hw = &adapter->hw;
	struct maintain_req *req;
	void *dma_buf = NULL;
	dma_addr_t dma_phy;
	int bytes;

	if (off == 0) {
		if (count < sizeof(*req))
			return -EINVAL;

		req = (struct maintain_req *)buf;
		if (req->magic != MAINTAIN_MAGIC)
			return -EINVAL;
		bytes = max_t(int, req->req_data_bytes, req->reply_bytes);
		bytes += sizeof(*req);

		/* free no readed buf */
		if (adapter->maintain_buf) {
			kfree(adapter->maintain_buf);
			adapter->maintain_buf = NULL;
			adapter->maintain_buf_len = 0;
		}

		dma_buf = dma_alloc_coherent(&hw->pdev->dev, bytes,
					     &dma_phy, GFP_ATOMIC);
		if (!dma_buf)
			return -ENOMEM;

		adapter->maintain_dma_buf = dma_buf;
		adapter->maintain_dma_phy = dma_phy;
		adapter->maintain_dma_size = bytes;
		adapter->maintain_in_bytes =
			req->req_data_bytes + sizeof(*req);
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

		/* PN_SN Partion */
		if (req->arg0 == 3) {
			pf_adpt->pn[0] = 0;
			pf_adpt->sn[0] = 0;
		}

		/* send req */
		err = rnpm_maintain_req(hw, req->cmd, req->arg0,
					req->req_data_bytes,
					req->reply_bytes, dma_phy);
		if (err != 0)
			goto err_quit;
		if (reply_bytes > 0) {
			adapter->maintain_buf_len = reply_bytes;
			adapter->maintain_buf = kmalloc(
				adapter->maintain_buf_len, GFP_KERNEL);
			if (!adapter->maintain_buf) {
				err = -ENOMEM;
				goto err_quit;
			}
			memcpy(adapter->maintain_buf, dma_buf,
			       reply_bytes);
		}

		if (dma_buf)
			dma_free_coherent(&hw->pdev->dev,
					  adapter->maintain_dma_size,
					  dma_buf, dma_phy);
		adapter->maintain_dma_buf = NULL;
	}

	return count;
err_quit:
	if (dma_buf) {
		dma_free_coherent(&hw->pdev->dev,
				  adapter->maintain_dma_size, dma_buf,
				  dma_phy);
		adapter->maintain_dma_buf = NULL;
	}
	return err;
}

static BIN_ATTR(maintain, (S_IWUSR | S_IRUGO), maintain_read,
		maintain_write, 1 * 1024 * 1024);
#endif

static ssize_t active_vid_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	u16 current_vid = 0;
	u16 vid = 0;
	int ret = 0;
	u8 vfnum = RNPM_MAX_VF_CNT - 1;

	if ((adapter->flags & RNPM_FLAG_SRIOV_ENABLED))
		current_vid = rd32(hw, RNPM_DMA_PORT_VEB_VID_TBL(
					       adapter->port, vfnum));
	for_each_set_bit(vid, adapter->active_vlans, VLAN_N_VID)
		ret += sprintf(buf + ret, "%u%s ", vid,
			       (current_vid == vid ? "*" : ""));
	ret += sprintf(buf + ret, "\n");
	return ret;
}

static ssize_t active_vid_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	u16 vid;
	int err = -EINVAL;
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw __maybe_unused *hw = &adapter->hw;
	u8 __maybe_unused vfnum = RNPM_MAX_VF_CNT - 1;
	int __maybe_unused port = 0;

	if (!(adapter->flags & RNPM_FLAG_SRIOV_ENABLED))
		return -EIO;

	if (kstrtou16(buf, 0, &vid))
		return -EINVAL;
	if ((vid < 4096) && test_bit(vid, adapter->active_vlans)) {
		if (rd32(hw, RNPM_DMA_VERSION) >= 0x20201231) {
			for (port = 0; port < 4; port++)
				wr32(hw,
				     RNPM_DMA_PORT_VEB_VID_TBL(port,
							       vfnum),
				     vid);
		} else {
			wr32(hw,
			     RNPM_DMA_PORT_VEB_VID_TBL(adapter->port,
						       vfnum),
			     vid);
		}
		err = 0;
	}
	return err ? err : count;
}

static ssize_t pf_reset_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_pf_adapter *pf_adapter = adapter->pf_adapter;

	set_bit(RNPM_PF_RESET, &pf_adapter->flags);
	return count;
}

static inline int pn_sn_dlen(char *v, int v_len)
{
	int i, len = 0;

	for (i = 0; i < v_len; i++) {
		if (isascii(v[i]))
			len++;
		else
			break;
	}
	return len;
}

static int rnpm_mbx_get_pn_sn(struct rnpm_hw *hw, char pn[33], char sn[33])
{
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	struct maintain_req *req;
	dma_addr_t dma_phy;
	struct ucfg_mac_sn *cfg;
	struct rnpm_pf_adapter *pf_adpt = adapter->pf_adapter;
	void *dma_buf = NULL;
	int err = 0, bytes = sizeof(*req) + sizeof(struct ucfg_mac_sn);

	if (isprint(pf_adpt->pn[0]) && isprint(pf_adpt->sn[0])) {
		memcpy(pn, pf_adpt->pn, 33);
		memcpy(sn, pf_adpt->sn, 33);
		return 0;
	}

	memset(pn, 0, 33);
	memset(sn, 0, 33);

	dma_buf = dma_alloc_coherent(&hw->pdev->dev, bytes, &dma_phy,
				     GFP_KERNEL);
	if (!dma_buf)
		return -ENOMEM;

	req = (struct maintain_req *)dma_buf;
	memset(dma_buf, 0, bytes);
	cfg = (struct ucfg_mac_sn *)(req + 1);
	req->magic = MAINTAIN_MAGIC;
	req->cmd = 0; /* READ */
	req->arg0 = 3; /* PARTION 3 */
	req->req_data_bytes = 0;
	req->reply_bytes = bytes - sizeof(*req);

	err = rnpm_maintain_req(hw, req->cmd, req->arg0,
				req->req_data_bytes, req->reply_bytes,
				dma_phy);
	if (err != 0)
		goto err_quit;
	if (cfg->magic == MAC_SN_MAGIC) {
		int sz = pn_sn_dlen(cfg->pn, 32);

		if (sz) {
			memcpy(pn, cfg->pn, sz);
			memcpy(pf_adpt->pn, cfg->pn, sz);
			pf_adpt->pn[sz] = 0;
			pn[sz] = 0;
		}
		sz = pn_sn_dlen(cfg->sn, 32);
		if (sz) {
			memcpy(sn, cfg->sn, sz);
			memcpy(pf_adpt->sn, cfg->sn, sz);
			pf_adpt->sn[sz] = 0;
			sn[sz] = 0;
		}
	}

err_quit:
	if (dma_buf)
		dma_free_coherent(&hw->pdev->dev, bytes, dma_buf, dma_phy);

	return 0;
}

static ssize_t own_vpd_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	char pn[33] = { 0 }, sn[33] = { 0 };
	char *prod_name;

	rnpm_mbx_get_pn_sn(hw, pn, sn);

	if ((adapter->pdev->device & 0xff) == 0x20)
		prod_name =
			"Ethernet Controller N10 Series for 10GbE (Quad-port)";
	else if ((adapter->pdev->device & 0xff) == 0x21)
		prod_name =
			"Ethernet Controller N400 Series for 1GbE (Quad-port)";
	else if ((adapter->pdev->device & 0xff) == 0x60)
		prod_name =
			"Ethernet Controller N10 Series for 10GbE (Oct-port)";
	else
		prod_name = "Ethernet Controller N10 Series for 10GbE";

	ret += sprintf(buf + ret, "Product Name: %s\n", prod_name);

	ret += sprintf(buf + ret, "[PN] Part number: %s\n", pn);
	ret += sprintf(buf + ret, "[SN] Serial number: %s\n", sn);

	return ret;
}

static ssize_t version_info_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int ret = 0;

	ret += sprintf(buf + ret, "drver: %s%s-%x\n", rnpm_driver_version,
		       GIT_COMMIT, hw->ccode);

	ret += sprintf(buf + ret, "fw   : %d.%d.%d.%d 0x%08x\n",
		       ((char *)&(hw->fw_version))[3],
		       ((char *)&(hw->fw_version))[2],
		       ((char *)&(hw->fw_version))[1],
		       ((char *)&(hw->fw_version))[0], hw->fw_uid);

	return ret;
}

/* show logical and physical ring num correspondence */
static ssize_t ring_num_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_ring *ring;
	int ret = 0, i;

	ret += sprintf(
		buf + ret,
		"show %s logical <-> physical ring num (Decimal) correspondence:\n",
		netdev->name);
	for (i = 0; i < netdev->real_num_tx_queues; i++) {
		ring = adapter->tx_ring[i];
		ret += sprintf(buf + ret, "%03d           %03d\n", i,
			       ring->rnpm_queue_idx);
	}
	return ret;
}

static ssize_t port_idx_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	int ret = 0;

	ret += sprintf(buf, "%d\n", adapter->portid_of_card);
	return ret;
}

static ssize_t nr_lane_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);

	ret += sprintf(buf, "%d\n", adapter->lane);
	return ret;
}

static ssize_t debug_linkstat_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;

	ret += sprintf(buf, "%d %d dumy:0x%x up-flag:%d carry:%d\n",
		       adapter->link_up, adapter->hw.link, rd32(hw, 0xc),
		       adapter->flags & RNPM_FLAG_NEED_LINK_UPDATE,
		       netif_carrier_ok(netdev));
	return ret;
}

static ssize_t debug_aptstat_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;

	ret += sprintf(
		buf,
		"stat:%d %d dumy:0x%x up-flag:%d carry:%d\n"
		"apt:flags:0x%x flags2:0x%x state:0x%lx timercnt:%u service cnt:%u\n"
		"pf_apt:flags:0x%lx state:0x%lx timercnt:%u\n",
		adapter->link_up, adapter->hw.link, rd32(hw, 0xc),
		adapter->flags & RNPM_FLAG_NEED_LINK_UPDATE,
		netif_carrier_ok(netdev), adapter->flags, adapter->flags2,
		adapter->state, adapter->timer_count,
		adapter->service_count, adapter->pf_adapter->flags,
		adapter->pf_adapter->state,
		adapter->pf_adapter->timer_count);
	return ret;
}

static ssize_t sfp_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;

	if (rnpm_mbx_get_lane_stat(hw) != 0) {
		ret += sprintf(buf, " IO Error\n");
	} else {
		ret += sprintf(
			buf,
			"mod-abs:%d\ntx-fault:%d\ntx-dis:%d\nrx-los:%d\n",
			adapter->sfp.mod_abs, adapter->sfp.fault,
			adapter->sfp.tx_dis, adapter->sfp.los);
	}

	return ret;
}

static ssize_t pci_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int gen = 3, lanes = 8;
	int err = -EINVAL;

	if (count > 30)
		return -EINVAL;

	if (sscanf(buf, "gen%dx%d", &gen, &lanes) != 2) {
		netdev_err(netdev,
			   "Error: invalid input. example: gen3x8\n");
		return -EINVAL;
	}
	if (gen > 3 || lanes > 8)
		return -EINVAL;

	err = rnpm_set_lane_fun(hw, LANE_FUN_PCI_LANE, gen, lanes, 0, 0);
	return err ? err : count;
}

static ssize_t pci_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int ret = 0;

	if (rnpm_mbx_get_lane_stat(hw) != 0)
		ret += sprintf(buf, " IO Error\n");
	else
		ret += sprintf(buf, "gen%dx%d\n", hw->pci_gen,
			       hw->pci_lanes);
	return ret;
}

static ssize_t lane_intr_en_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	s32 en = 0;

	if (kstrtos32(buf, 10, &en))
		return -EINVAL;
	adapter->lane_intr_en = en;
	rnpm_mbx_lane_link_changed_event_enable(&adapter->hw, !!en);
	return count;
}

static ssize_t lane_intr_en_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	int ret = 0;

	ret = sprintf(buf, "lane intr stat:%d\n", adapter->lane_intr_en);
	return ret;
}

static ssize_t prbs_store(struct device *dev,
			  struct device_attribute *attr, const char *buf,
			  size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	long prbs = 0;
	int err = -EINVAL;

	if (kstrtol(buf, 10, &prbs))
		return -EINVAL;

	err = rnpm_set_lane_fun(hw, LANE_FUN_PRBS, prbs, 0, 0, 0);
	return err ? err : count;
}

static ssize_t sfp_tx_disable_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	long enable = 0;
	int err = -EINVAL;

	if (kstrtol(buf, 10, &enable))
		return -EINVAL;

	err = rnpm_set_lane_fun(hw, LANE_FUN_SFP_TX_DISABLE, !!enable, 0,
				0, 0);
	return err ? err : count;
}

static ssize_t sfp_tx_disable_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int ret = 0;

	if (rnpm_mbx_get_lane_stat(hw) != 0)
		ret += sprintf(buf, " IO Error\n");
	else
		ret += sprintf(buf, "%d\n", adapter->sfp.tx_dis);
	return ret;
}

static ssize_t link_traing_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	long enable = 0;
	int err = -EINVAL;

	if (kstrtol(buf, 10, &enable))
		return -EINVAL;

	err = rnpm_set_lane_fun(hw, LANE_FUN_LINK_TRAING, !!enable, 0, 0,
				0);
	return err ? err : count;
}

static ssize_t link_traing_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int ret = 0;

	if (rnpm_mbx_get_lane_stat(hw) != 0)
		ret += sprintf(buf, " IO Error\n");
	else
		ret += sprintf(buf, "%d\n", adapter->link_traing);
	return ret;
}

static ssize_t fec_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	long enable = 0;
	int err = -EINVAL;

	if (kstrtol(buf, 10, &enable))
		return -EINVAL;

	err = rnpm_set_lane_fun(hw, LANE_FUN_FEC, !!enable, 0, 0, 0);
	return err ? err : count;
}

static ssize_t fec_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int ret = 0;

	if (rnpm_mbx_get_lane_stat(hw) != 0)
		ret += sprintf(buf, " IO Error\n");
	else
		ret += sprintf(buf, "%d\n", adapter->fec);
	return ret;
}

static ssize_t autoneg_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	long enable = 0;
	int err = -EINVAL;

	if (kstrtol(buf, 10, &enable))
		return -EINVAL;

	err = rnpm_set_lane_fun(hw, LANE_FUN_AN, !!enable, 0, 0, 0);
	return err ? err : count;
}

static ssize_t autoneg_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int ret = 0;

	if (rnpm_mbx_get_lane_stat(hw) != 0)
		ret += sprintf(buf, " IO Error\n");
	else
		ret += sprintf(buf, "%d\n", adapter->an);
	return ret;
}

static ssize_t si_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int si_main = -1, si_pre = -1, si_post = -1, si_txboost = -1;
	int cnt, err = -EINVAL;

	if (count > 100) {
		netdev_err(netdev, "Error: Input size >100: too large\n");
		return -EINVAL;
	}
	cnt = sscanf(buf, "%u %u %u %u", &si_main, &si_pre, &si_post,
		     &si_txboost);
	if (cnt != 4) {
		netdev_err(
			netdev,
			"Error: Invalid Input: <main> <pre> <post> <tx_boost>\n");
		return -EINVAL;
	}
	if (si_main > 63 || si_pre > 63 || si_post > 63) {
		netdev_err(netdev,
			   "Error: Invalid value. should in 0~63\n");
		return -EINVAL;
	}
	if (si_txboost > 16) {
		netdev_err(netdev,
			   "Error: Invalid txboost. should in 0~15\n");
		return -EINVAL;
	}

	err = rnpm_set_lane_fun(hw, LANE_FUN_SI, si_main, si_pre, si_post,
				si_txboost);
	return err ? err : count;
}

static ssize_t si_show(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int ret = 0;

	if (rnpm_mbx_get_lane_stat(hw))
		ret += sprintf(buf, " IO Error\n");
	else
		ret += sprintf(buf, "main:%u pre:%u post:%u tx_boost:%u\n",
			       adapter->si.main, adapter->si.pre,
			       adapter->si.post, adapter->si.tx_boost);
	return ret;
}

static ssize_t temperature_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int ret = 0, temp = 0, voltage = 0;

	temp = rnpm_mbx_get_temp(hw, &voltage);
	ret += sprintf(buf, "temp:%d oC  volatage:%d mV\n", temp, voltage);
	return ret;
}

static ssize_t tx_counter_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int i, ret = 0;
	u32 val = 0;

	ret += sprintf(buf + ret, "tx counters\n");
	for (i = 0; i < 4; i++) {
		ret += sprintf(buf + ret, "ring%d-tx:\n", i);

		val = rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_LEN(0));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: 0x%x\n",
			       "len:", RNPM_DMA_REG_TX_DESC_BUF_LEN(0),
			       val);

		val = rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_HEAD(0));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: 0x%x\n",
			       "head:", RNPM_DMA_REG_TX_DESC_BUF_HEAD(0),
			       val);

		val = rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_TAIL(0));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: 0x%x\n",
			       "tail:", RNPM_DMA_REG_TX_DESC_BUF_TAIL(0),
			       val);

		ret += sprintf(buf + ret, "ring1-tx:\n");

		val = rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_LEN(1));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: 0x%x\n",
			       "len:", RNPM_DMA_REG_TX_DESC_BUF_LEN(1),
			       val);

		val = rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_HEAD(1));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: 0x%x\n",
			       "head:", RNPM_DMA_REG_TX_DESC_BUF_HEAD(1),
			       val);

		val = rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_TAIL(1));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: 0x%x\n",
			       "tail:", RNPM_DMA_REG_TX_DESC_BUF_TAIL(1),
			       val);
	}

	ret += sprintf(buf + ret, "to_1to4_p1:\n");

	val = rd32(hw, RNPM_ETH_1TO4_INST0_IN_PKTS);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "emac_in:", RNPM_ETH_1TO4_INST0_IN_PKTS, val);

	val = rd32(hw, RNPM_ETH_IN_0_TX_PKT_NUM(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "emac_send:", RNPM_ETH_IN_0_TX_PKT_NUM(0), val);

	ret += sprintf(buf + ret, "to_1to4_p2:\n");

	val = rd32(hw, RNPM_ETH_IN_1_TX_PKT_NUM(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "sop_pkt:", RNPM_ETH_IN_1_TX_PKT_NUM(0), val);

	val = rd32(hw, RNPM_ETH_IN_2_TX_PKT_NUM(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "eop_pkt:", RNPM_ETH_IN_2_TX_PKT_NUM(0), val);

	val = rd32(hw, RNPM_ETH_IN_3_TX_PKT_NUM(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "send_terr:", RNPM_ETH_IN_3_TX_PKT_NUM(0), val);

	ret += sprintf(buf + ret, "to_tx_trans(phy):\n");

	val = rd32(hw, RNPM_ETH_EMAC_TX_TO_PHY_PKTS(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "in:", RNPM_ETH_EMAC_TX_TO_PHY_PKTS(0), val);

	val = rd32(hw, RNPM_ETH_TXTRANS_PTP_PKT_NUM(0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "out:", RNPM_ETH_TXTRANS_PTP_PKT_NUM(0), val);

	ret += sprintf(buf + ret, "mac:\n");

	val = rd32(hw, 0x1081c);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "mac-tx-cnts:", 0x1081c, val);

	val = rd32(hw, 0x1087c);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "underflow_err:", 0x1087c, val);

	val = rd32(hw, RNPM_ETH_TX_DEBUG_PORT0_SOP);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "port0_txtrans_sop:", RNPM_ETH_TX_DEBUG_PORT0_SOP,
		       val);

	val = rd32(hw, RNPM_ETH_TX_DEBUG_PORT0_EOP);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "port0_txtrans_eop:", RNPM_ETH_TX_DEBUG_PORT0_EOP,
		       val);

	val = rd32(hw, RNPM_ETH_TX_DEBUG_PORT1_SOP);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "port1_txtrans_sop:", RNPM_ETH_TX_DEBUG_PORT1_SOP,
		       val);

	val = rd32(hw, RNPM_ETH_TX_DEBUG_PORT1_EOP);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "port1_txtrans_eop:", RNPM_ETH_TX_DEBUG_PORT1_EOP,
		       val);

	val = rd32(hw, RNPM_ETH_TX_DEBUG_PORT2_SOP);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "port2_txtrans_sop:", RNPM_ETH_TX_DEBUG_PORT2_SOP,
		       val);

	val = rd32(hw, RNPM_ETH_TX_DEBUG_PORT2_EOP);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "port2_txtrans_eop:", RNPM_ETH_TX_DEBUG_PORT2_EOP,
		       val);

	val = rd32(hw, RNPM_ETH_TX_DEBUG_PORT3_SOP);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "port3_txtrans_sop:", RNPM_ETH_TX_DEBUG_PORT3_SOP,
		       val);

	val = rd32(hw, RNPM_ETH_TX_DEBUG_PORT3_EOP);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "port3_txtrans_eop:", RNPM_ETH_TX_DEBUG_PORT3_EOP,
		       val);

	val = rd32(hw, RNPM_ETH_TX_DEBUG_EMPTY);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "tx_empty:", RNPM_ETH_TX_DEBUG_EMPTY, val);

	val = rd32(hw, RNPM_ETH_TX_DEBUG_PROG_FULL);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "tx_prog_full:", RNPM_ETH_TX_DEBUG_PROG_FULL, val);

	val = rd32(hw, RNPM_ETH_TX_DEBUG_FULL);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %d\n",
		       "tx_full:", RNPM_ETH_TX_DEBUG_FULL, val);

	return ret;
}

static ssize_t rx_counter_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	u32 val = 0, port = 0;
	int ret = 0;

	port = hw->nr_lane;
	ret += sprintf(buf + ret, "rx counters\n");
	ret += sprintf(buf + ret, "emac_rx_trans (lane:%d):\n", port);

	val = rd32(hw, RNPM_XLMAC + 0x900 + port * 0x10000);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "mac-pkts:", RNPM_XLMAC + 0x900 + port * 0x10000,
		       val);

	val = rd32(hw, RNPM_RXTRANS_RX_PKTS(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "pkts:", RNPM_RXTRANS_RX_PKTS(port), val);

	val = rd32(hw, RNPM_RXTRANS_DROP_PKTS(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "drop:", RNPM_RXTRANS_DROP_PKTS(port), val);

	val = rd32(hw, RNPM_RXTRANS_WDT_ERR_PKTS(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "wdt_err:", RNPM_RXTRANS_WDT_ERR_PKTS(port), val);

	val = rd32(hw, RNPM_RXTRANS_CODE_ERR_PKTS(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "code_err:", RNPM_RXTRANS_CODE_ERR_PKTS(port), val);

	val = rd32(hw, RNPM_RXTRANS_CRC_ERR_PKTS(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "crc_err:", RNPM_RXTRANS_CRC_ERR_PKTS(port), val);

	val = rd32(hw, RNPM_RXTRANS_SLEN_ERR_PKTS(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "slen_err:", RNPM_RXTRANS_SLEN_ERR_PKTS(port), val);

	val = rd32(hw, RNPM_RXTRANS_GLEN_ERR_PKTS(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "glen_err:", RNPM_RXTRANS_GLEN_ERR_PKTS(port), val);

	val = rd32(hw, RNPM_RXTRANS_IPH_ERR_PKTS(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "iph_err:", RNPM_RXTRANS_IPH_ERR_PKTS(port), val);

	val = rd32(hw, RNPM_RXTRANS_CSUM_ERR_PKTS(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "csum_err:", RNPM_RXTRANS_CSUM_ERR_PKTS(port), val);

	val = rd32(hw, RNPM_RXTRANS_LEN_ERR_PKTS(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "len_err:", RNPM_RXTRANS_LEN_ERR_PKTS(port), val);

	val = rd32(hw, RNPM_RXTRANS_CUT_ERR_PKTS(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "trans_cut_err:", RNPM_RXTRANS_CUT_ERR_PKTS(port),
		       val);

	val = rd32(hw, RNPM_RXTRANS_EXCEPT_BYTES(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "expt_byte_err:", RNPM_RXTRANS_EXCEPT_BYTES(port),
		       val);

	val = rd32(hw, RNPM_RXTRANS_G1600_BYTES_PKTS(port));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       ">1600Byte:", RNPM_RXTRANS_G1600_BYTES_PKTS(port),
		       val);

	ret += sprintf(buf + ret, "gather:\n");
	val = rd32(hw, RNPM_ETH_TOTAL_GAT_RX_PKT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "total_in_pkts:", RNPM_ETH_TOTAL_GAT_RX_PKT_NUM,
		       val);

	port = 0;
	ret += sprintf(buf + ret, "to_nxt_module:\n");

	for (port = 0; port < 4; port++) {
		u8 pname[16] = { 0 };

		val = rd32(hw, RNPM_ETH_RX_PKT_NUM(port));
		sprintf(pname, "p%d-rx:", port);
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n", pname,
			       RNPM_ETH_RX_PKT_NUM(port), val);
	}

	for (port = 0; port < 4; port++) {
		u8 pname[16] = { 0 };

		val = rd32(hw, RNPM_ETH_RX_DROP_PKT_NUM(port));
		sprintf(pname, "p%d-drop:", port);
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n", pname,
			       RNPM_ETH_RX_DROP_PKT_NUM(port), val);
	}

	ret += sprintf(buf + ret, "debug:\n");
	val = rd32(hw, RNPM_ETH_RX_DEBUG(10));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "data_eop:", RNPM_ETH_RX_DEBUG(10), val);
	val = rd32(hw, RNPM_ETH_RX_DEBUG(11));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "data_descs:", RNPM_ETH_RX_DEBUG(11), val);
	val = rd32(hw, RNPM_ETH_RX_DEBUG(12));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "data_desc_sop:", RNPM_ETH_RX_DEBUG(12), val);
	val = rd32(hw, RNPM_ETH_RX_DEBUG(13));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "data_desc_eop:", RNPM_ETH_RX_DEBUG(13), val);
	val = rd32(hw, RNPM_ETH_RX_DEBUG(14));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "4to1_gather_sop:", RNPM_ETH_RX_DEBUG(14), val);
	val = rd32(hw, RNPM_ETH_RX_DEBUG(15));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "4to1_gather_eop:", RNPM_ETH_RX_DEBUG(15), val);

	ret += sprintf(buf + ret, "ip-parse:\n");

	val = rd32(hw, RNPM_ETH_PKT_EGRESS_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "pkg_egree:", RNPM_ETH_PKT_EGRESS_NUM, val);

	val = rd32(hw, RNPM_ETH_PKT_IP_HDR_LEN_ERR_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "L3_len_err:", RNPM_ETH_PKT_IP_HDR_LEN_ERR_NUM,
		       val);

	val = rd32(hw, RNPM_ETH_PKT_IP_PKT_LEN_ERR_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "ip_hdr_err:", RNPM_ETH_PKT_IP_PKT_LEN_ERR_NUM,
		       val);

	val = rd32(hw, RNPM_ETH_PKT_L3_HDR_CHK_ERR_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "l3-csum-err:", RNPM_ETH_PKT_L3_HDR_CHK_ERR_NUM,
		       val);

	val = rd32(hw, RNPM_ETH_PKT_L4_HDR_CHK_ERR_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "l4-csum-err:", RNPM_ETH_PKT_L4_HDR_CHK_ERR_NUM,
		       val);

	val = rd32(hw, RNPM_ETH_PKT_SCTP_CHK_ERR_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "sctp-err:", RNPM_ETH_PKT_SCTP_CHK_ERR_NUM, val);

	val = rd32(hw, RNPM_ETH_PKT_VLAN_ERR_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "vlan-err:", RNPM_ETH_PKT_VLAN_ERR_NUM, val);

	val = rd32(hw, RNPM_ETH_PKT_EXCEPT_SHORT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "except_short_num:", RNPM_ETH_PKT_EXCEPT_SHORT_NUM,
		       val);

	val = rd32(hw, RNPM_ETH_PKT_PTP_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "ptp:", RNPM_ETH_PKT_PTP_NUM, val);

	ret += sprintf(buf + ret, "to-indecap:\n");

	val = rd32(hw, RNPM_ETH_DECAP_PKT_IN_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "*in engin*:", RNPM_ETH_DECAP_PKT_IN_NUM, val);

	val = rd32(hw, RNPM_ETH_DECAP_PKT_OUT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "*out engin*:", RNPM_ETH_DECAP_PKT_OUT_NUM, val);

	val = rd32(hw, RNPM_ETH_DECAP_DMAC_OUT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "to-dma/host:", RNPM_ETH_DECAP_DMAC_OUT_NUM, val);

	val = rd32(hw, RNPM_ETH_DECAP_BMC_OUT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "to-bmc:", RNPM_ETH_DECAP_BMC_OUT_NUM, val);

	val = rd32(hw, RNPM_ETH_DECAP_SW_OUT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "to-switch:", RNPM_ETH_DECAP_SW_OUT_NUM, val);

	val = rd32(hw, RNPM_ETH_DECAP_MIRROR_OUT_NUM);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "bmc+host:", RNPM_ETH_DECAP_MIRROR_OUT_NUM, val);

	val = rd32(hw, RNPM_ETH_DECAP_PKT_DROP_NUM(0x0));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "err_drop:", RNPM_ETH_DECAP_PKT_DROP_NUM(0x0), val);

	val = rd32(hw, RNPM_ETH_DECAP_PKT_DROP_NUM(1));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "plicy_drop:", RNPM_ETH_DECAP_PKT_DROP_NUM(1), val);

	val = rd32(hw, RNPM_ETH_DECAP_PKT_DROP_NUM(2));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "dmac_drop:", RNPM_ETH_DECAP_PKT_DROP_NUM(2), val);

	val = rd32(hw, RNPM_ETH_DECAP_PKT_DROP_NUM(3));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "bmc_drop:", RNPM_ETH_DECAP_PKT_DROP_NUM(3), val);

	val = rd32(hw, RNPM_ETH_DECAP_PKT_DROP_NUM(4));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "sw_drop:", RNPM_ETH_DECAP_PKT_DROP_NUM(4), val);

	val = rd32(hw, RNPM_ETH_DECAP_PKT_DROP_NUM(5));
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "rm_vlane_num:", RNPM_ETH_DECAP_PKT_DROP_NUM(5),
		       val);

	ret += sprintf(buf + ret, "\npolicy-drop-reason:\n");
	val = rd32(hw, RNPM_ETH_RX_DEBUG(4));
	ret += sprintf(buf + ret, "\t %30s 0x%08x: %d\n",
		       "host_l2_match_drop:", RNPM_ETH_RX_DEBUG(4), val);
	val = rd32(hw, RNPM_ETH_RX_DEBUG(5));
	ret += sprintf(buf + ret, "\t %30s 0x%08x: %d\n",
		       "redir_input_match_drop:", RNPM_ETH_RX_DEBUG(5),
		       val);
	val = rd32(hw, RNPM_ETH_RX_DEBUG(6));
	ret += sprintf(buf + ret, "\t %30s 0x%08x: %d\n",
		       "redir_etypt_match_drop:", RNPM_ETH_RX_DEBUG(6),
		       val);
	val = rd32(hw, RNPM_ETH_RX_DEBUG(7));
	ret += sprintf(buf + ret, "\t %30s 0x%08x: %d\n",
		       "redir_tcp_sync_match_drop:", RNPM_ETH_RX_DEBUG(7),
		       val);
	val = rd32(hw, RNPM_ETH_RX_DEBUG(8));
	ret += sprintf(buf + ret, "\t %30s 0x%08x: %d\n",
		       "redir_tuple5_match_drop:", RNPM_ETH_RX_DEBUG(8),
		       val);
	val = rd32(hw, RNPM_ETH_RX_DEBUG(9));
	ret += sprintf(buf + ret, "\t %30s 0x%08x: %d\n",
		       "recdir_tcam_match_drop:", RNPM_ETH_RX_DEBUG(9),
		       val);

	ret += sprintf(buf + ret, "dma-2-host:\n");

	val = rd32(hw, 0x264);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "fifo equ:", 0x264, val);

	val = rd32(hw, 0x268);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "fifo deq:", 0x268, val);

	val = rd32(hw, 0x114);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "unexpt_abtring:", 0x114, val);

	val = rd32(hw, 0x288);
	ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
		       "pci2host:", 0x288, val);

	for (port = 0; port < 4; port++) {
		ret += sprintf(buf + ret, "rx-ring%d:\n", port);

		val = rd32(hw, RNPM_DMA_REG_RX_DESC_BUF_HEAD(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n", "head:",
			       RNPM_DMA_REG_RX_DESC_BUF_HEAD(port), val);

		val = rd32(hw, RNPM_DMA_REG_RX_DESC_BUF_TAIL(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n", "tail:",
			       RNPM_DMA_REG_RX_DESC_BUF_TAIL(port), val);

		val = rd32(hw, RNPM_DMA_REG_RX_DESC_BUF_LEN(port));
		ret += sprintf(buf + ret, "\t %16s 0x%08x: %u\n",
			       "len:", RNPM_DMA_REG_RX_DESC_BUF_LEN(port),
			       val);
	}

	if (ret > RNPM_RXBUFFER_4K)
		netdev_warn(netdev,
			    "ret = %d, buffer size may be exceed 4K\n",
			    ret);

	return ret;
}

static ssize_t bar4_reg_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	int ret = 0;

	ret += sprintf(buf + ret, "addr=0x%x, val=0x%x\n", bar4_reg_addr,
		       bar4_reg_val);
	return ret;
}

static ssize_t bar4_reg_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	char *after;

	bar4_reg_addr = simple_strtoul(buf, &after, 0);
	bar4_reg_val = rd32(hw, bar4_reg_addr);
	return count;
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

static ssize_t root_slot_info_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	int ret = 0;
	struct pci_dev *root_pdev = pcie_find_root_port_old(adapter->pdev);

	if (root_pdev)
		ret += sprintf(buf + ret, "%02x:%02x.%x\n",
			       root_pdev->bus->number,
			       PCI_SLOT(root_pdev->devfn),
			       PCI_FUNC(root_pdev->devfn));
	return ret;
}

static ssize_t phy_statistics_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int ret = 0;
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	struct phy_statistics ps;

	memset(&ps, 0, sizeof(ps));

	if (!hw->is_sgmii)
		return 0;

	if (rnpm_mbx_get_phy_statistics(hw, (u8 *)&ps) != 0)
		return 0;

	ret += sprintf(buf + ret, "RX crc good   (64~1518) : %u\n",
		       ps.yt.pkg_ib_valid);
	ret += sprintf(buf + ret, "RX crc good   (>1518)   : %u\n",
		       ps.yt.pkg_ib_os_good);
	ret += sprintf(buf + ret, "RX crc good   (<64)     : %u\n",
		       ps.yt.pkg_ib_us_good);
	ret += sprintf(buf + ret, "RX crc wrong  (64~1518) : %u\n",
		       ps.yt.pkg_ib_err);
	ret += sprintf(buf + ret, "RX crc wrong  (>1518)   : %u\n",
		       ps.yt.pkg_ib_os_bad);
	ret += sprintf(buf + ret, "RX crc wrong  (<64)     : %u\n",
		       ps.yt.pkg_ib_frag);
	ret += sprintf(buf + ret, "RX SFD missed (nosfd)   : %u\n",
		       ps.yt.pkg_ib_nosfd);
	ret += sprintf(buf + ret, "TX crc good   (64~1518) : %u\n",
		       ps.yt.pkg_ob_valid);
	ret += sprintf(buf + ret, "TX crc good   (>1518)   : %u\n",
		       ps.yt.pkg_ob_os_good);
	ret += sprintf(buf + ret, "TX crc good   (<64)     : %u\n",
		       ps.yt.pkg_ob_us_good);
	ret += sprintf(buf + ret, "TX crc wrong  (64~1518) : %u\n",
		       ps.yt.pkg_ob_err);
	ret += sprintf(buf + ret, "TX crc wrong  (>1518)   : %u\n",
		       ps.yt.pkg_ob_os_bad);
	ret += sprintf(buf + ret, "TX crc wrong  (<64)     : %u\n",
		       ps.yt.pkg_ob_frag);
	ret += sprintf(buf + ret, "TX SFD missed (nosfd)   : %u\n",
		       ps.yt.pkg_ob_nosfd);

	return ret;
}

static ssize_t port_stat2_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	struct mbx_port_stat stat;
	int ret = 0;

	if (rnpm_get_port_stats2(hw, &stat) != 0)
		return 0;

	ret += sprintf(buf + ret, "link  : %d\n", stat.link);
	ret += sprintf(buf + ret, "speed : %d\n", stat.speed);
	ret += sprintf(buf + ret, "duplex: %d\n", stat.duplex);
	ret += sprintf(buf + ret, "abs   : %d\n", stat.abs);

	return ret;
}

static ssize_t pcs_reg_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	u32 pcs_phy_regs[] = {
		0x00040000, 0x00041000, 0x00042000, 0x00043000,
		0x00040000, 0x00041000, 0x00042000, 0x00043000,
	};
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	u32 reg_hi = 0, reg_lo = 0, pcs_base_regs = 0;

	if (count > 64) {
		netdev_err(netdev, "Error: Input size >100: too large\n");
		return -EINVAL;
	}

	pcs_cnt = sscanf(buf, "%u %x %x", &pcs_phy_num, &bar4_reg_addr,
			 &bar4_reg_val);

	if (pcs_cnt != 2 && pcs_cnt != 3) {
		netdev_err(
			netdev,
			"Invalid Input: read phy x reg 0xXXX or write phy x reg\n");
		netdev_err(netdev, " 0xXXX val 0xXXX\n");
		return -EINVAL;
	}

	if (pcs_phy_num > 8) {
		netdev_err(netdev,
			   "Error: Invalid value. should in 0~7\n");
		return -EINVAL;
	}

	switch (pcs_cnt) {
	case 2:
		reg_hi = bar4_reg_addr >> 8;
		reg_lo = (bar4_reg_addr & 0xff) << 2;
		pcs_base_regs = pcs_phy_regs[pcs_phy_num];
		wr32(hw, pcs_base_regs + (0xff << 2), reg_hi);
		bar4_reg_val = rd32(hw, pcs_base_regs + reg_lo);
		break;
	case 3:
		reg_hi = bar4_reg_addr >> 8;
		reg_lo = (bar4_reg_addr & 0xff) << 2;
		pcs_base_regs = pcs_phy_regs[pcs_phy_num];
		wr32(hw, pcs_base_regs + (0xff << 2), reg_hi);
		wr32(hw, pcs_base_regs + reg_lo, bar4_reg_val);
		break;
	default:
		netdev_err(netdev, "Error: Invalid value. pcs_cnt=%d\n",
			   pcs_cnt);
		break;
	}

	return count;
}

static ssize_t pcs_reg_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	int ret = 0;

	switch (pcs_cnt) {
	case 2:
		ret += sprintf(buf, "read phy %u reg 0x%x val 0x%x\n",
			       pcs_phy_num, bar4_reg_addr, bar4_reg_val);
		break;
	case 3:
		ret += sprintf(buf, "write phy %u reg 0x%x val 0x%x\n",
			       pcs_phy_num, bar4_reg_addr, bar4_reg_val);
		break;
	default:
		break;
	}

	return ret;
}

static ssize_t phy_reg_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int val = 0;
	int err = -EINVAL;

	if (phy_reg == -1)
		return 0;

	if (hw) {
		if (is_phy_ext_reg)
			err = rnpm_mbx_phy_read(
				hw, phy_reg | PHY_EXT_REG_FLAG, &val);
		else
			err = rnpm_mbx_phy_read(hw, phy_reg, &val);
	}

	phy_reg = -1;

	if (err)
		return 0;
	else
		return sprintf(buf, "phy %s 0x%04x : 0x%04x\n",
			       is_phy_ext_reg ? "ext reg" : "reg", phy_reg,
			       val & 0xffff);
}

static ssize_t phy_reg_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int i = 0, argc = 0, err = -EINVAL;
	char argv[3][16];
	unsigned long val[3] = { 0 };

	memset(argv, 0, sizeof(argv));
	argc = sscanf(buf, "%15s %15s %15s", argv[0], argv[1], argv[2]);

	if (argc < 1)
		return -EINVAL;

	is_phy_ext_reg = 0;
	if (strcmp(argv[0], "ext") == 0) {
		is_phy_ext_reg = 1;
	} else {
		if (kstrtoul(argv[0], 0, &val[0]))
			return -EINVAL;
	}

	for (i = 1; i < argc; i++) {
		if (kstrtoul(argv[i], 0, &val[i]))
			return -EINVAL;
	}

	if (argc == 1) {
		if (is_phy_ext_reg)
			return -EINVAL;
		/* set phy reg index */
		phy_reg = val[0];
		err = 0;
	}

	if (argc == 2) {
		if (is_phy_ext_reg) {
			/* set ext phy reg index */
			phy_reg = val[1];
			err = 0;
		} else {
			/* write phy reg */
			phy_reg = val[0];
			err = rnpm_mbx_phy_write(hw, phy_reg, val[1]);
		}
	}

	if (argc == 3) {
		if (is_phy_ext_reg) {
			/* write ext phy reg */
			phy_reg = val[1];
			err = rnpm_mbx_phy_write(
				hw, phy_reg | PHY_EXT_REG_FLAG, val[2]);
		} else {
			return -EINVAL;
		}
	}

	return err ? err : count;
}

static ssize_t pma_rx2tx_loopback_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int v, en_loopback = simple_strtoul(buf, NULL, 0);
	int nr_lane = adapter->lane % 4;

	netdev_info(netdev, "%s: nr_lane:%d lp:%d\n", __func__, nr_lane,
		    en_loopback);

	if (!hw->pcs.ops.write || !hw->pcs.ops.read)
		return -EOPNOTSUPP;

	/* pma-rx2tx_loopback */
	v = hw->pcs.ops.read(hw, nr_lane, 0x18090);
	if (en_loopback)
		v |= 0xf << 4;
	else
		v &= ~(0xf << 4);
	hw->pcs.ops.write(hw, nr_lane, 0x18090, v);

	/* mac tx-disable */
	v = rd32(hw, RNPM_MAC_TX_CFG(nr_lane));
	if (en_loopback)
		v &= ~(BIT(0)); /* disable mac-tx */
	else
		v |= (BIT(0)); /* enable mac-tx */
	wr32(hw, RNPM_MAC_TX_CFG(nr_lane), v);

	/* mac rx-disable */
	v = rd32(hw, RNPM_MAC_RX_CFG(nr_lane));
	if (en_loopback)
		v &= ~(BIT(0)); /* disable mac-rx */
	else
		v |= (BIT(0)); /*  enable mac-rx */
	wr32(hw, RNPM_MAC_RX_CFG(nr_lane), v);

	return count;
}

static ssize_t pcs_rx2tx_loopback_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int v, en_loopback = simple_strtoul(buf, NULL, 0);
	int nr_lane = adapter->lane % 4;

	netdev_info(netdev, "%s: nr_lane:%d lp:%d\n", __func__, nr_lane,
		    en_loopback);
	if (!hw->pcs.ops.write || !hw->pcs.ops.read)
		return -EOPNOTSUPP;

	/* pma-rx2tx_loopback */
	v = hw->pcs.ops.read(hw, nr_lane, 0x38000);
	if (en_loopback)
		v |= BIT(14); /* RX2TX_LB_EN */
	else
		v &= ~BIT(14); /* RX2TX_LB_EN */
	hw->pcs.ops.write(hw, nr_lane, 0x38000, v);

	/* mac tx-disable */
	v = rd32(hw, RNPM_MAC_TX_CFG(nr_lane));
	if (en_loopback)
		v &= ~(BIT(0)); /* disable mac-tx */
	else
		v |= (BIT(0)); /* enable mac-tx */
	wr32(hw, RNPM_MAC_TX_CFG(nr_lane), v);

	/* mac rx-disable */
	v = rd32(hw, RNPM_MAC_RX_CFG(nr_lane));
	if (en_loopback)
		v &= ~(BIT(0)); /* disable mac-rx */
	else
		v |= (BIT(0)); /* enable mac-rx */
	wr32(hw, RNPM_MAC_RX_CFG(nr_lane), v);

	return count;
}

static ssize_t mac_loopback_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct net_device *netdev = to_net_device(dev);
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	s32 en = 0;

	if (kstrtos32(buf, 10, &en))
		return -EINVAL;
	netdev_info(netdev, "%s:%s mac lp:%d\n", __func__, netdev->name,
		    en);
	adapter->is_mac_loopback = !!en;
	if (adapter->is_mac_loopback)
		rnpm_link_stat_mark(hw, hw->nr_lane, 0);
	else
		rnpm_link_stat_mark(hw, hw->nr_lane, 1);
	hw->mac.ops.mac_loopback(hw, !!en);

#define __FIBER_TX_DIS_USED_EXT_PIN (0)
	if (hw->is_sgmii)
		hw->mac.ops.power_down(hw, !!en);
	else {
#if __FIBER_TX_DIS_USED_EXT_PIN
		hw->mac.ops.sfp_tx_dis(hw, !!en);
#else
		rnpm_set_lane_fun(hw, LANE_FUN_SFP_TX_DISABLE, !!en, 0, 0,
				  0);
#endif
	}

	adapter->flags |= RNPM_FLAG_NEED_LINK_UPDATE;
	return count;
}

static int do_switch_loopback_set(struct rnpm_adapter *adapter, int en,
				  int sport_lane, int dst_switch_port)
{
	struct rnpm_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	int v;

	netdev_info(netdev, "%s: %s %d -> %d en:%d\n", __func__,
		    netdev_name(adapter->netdev), sport_lane,
		    dst_switch_port, en);
	if (en)
		adapter->flags |= RNPM_FLAG_SWITCH_LOOPBACK_EN;
	else
		adapter->flags &= ~RNPM_FLAG_SWITCH_LOOPBACK_EN;

	/* redir pkgs to peer */
	wr32(hw, RNPM_ETH_INPORT_POLICY_REG(sport_lane),
	     BIT(29) | (dst_switch_port << 16));

	/* enable/disable policy */
	v = rd32(hw, RNPM_ETH_INPORT_POLICY_VAL);
	if (en)
		v |= BIT(sport_lane);
	else
		v &= ~BIT(sport_lane);
	wr32(hw, RNPM_ETH_INPORT_POLICY_VAL, v);

	/* mac promisc */
	v = rd32(hw, RNPM_MAC_PKT_FLT(sport_lane));
	if (en)
		v |= (RNPM_RX_ALL | RNPM_RX_ALL_MUL);
	else
		v &= ~(RNPM_RX_ALL | RNPM_RX_ALL_MUL);
	wr32(hw, RNPM_MAC_PKT_FLT(sport_lane), v);
	return 0;
}

static int to_switch_port(struct rnpm_adapter *adapter)
{
	int switch_port = adapter->lane;

	if (rnpm_is_pf1(adapter->pdev))
		switch_port += 4;

	if (adapter->hw.mode == MODE_NIC_MODE_2PORT) {
		if (adapter->lane != 0)
			switch_port += 1;
	}
	return switch_port;
}

static ssize_t _switch_loopback(struct rnpm_adapter *adapter,
				const char *peer_eth, int en)
{
	struct net_device *netdev = adapter->netdev;
	struct net_device *peer_netdev = NULL;
	struct rnpm_adapter *peer_adapter = NULL;
	char name[100];
	int i;

	strncpy(name, peer_eth, sizeof(name));
	strim(name);

	peer_netdev = dev_get_by_name(&init_net, name);
	if (!peer_netdev) {
		netdev_err(netdev, "canot' find [%s]\n", name);
		return -EINVAL;
	}
	peer_adapter = netdev_priv(peer_netdev);

	/* check if in same slot */
	if (PCI_SLOT(peer_adapter->pdev->devfn) !=
	    PCI_SLOT(adapter->pdev->devfn)) {
		netdev_err(netdev, "%s %s not in same slot\n",
			   netdev_name(adapter->netdev),
			   netdev_name(peer_adapter->netdev));
		dev_put(peer_netdev);
		return -EINVAL;
	}

	netdev_info(netdev, "%s: %s(%d,%d) <-> %s(%d,%d) en:%d\n",
		    __func__, netdev_name(adapter->netdev), adapter->lane,
		    adapter->port, netdev_name(peer_adapter->netdev),
		    peer_adapter->lane, peer_adapter->port, en);

	/* clear pf0/pf1 input port policy eth reg */
	for (i = 0; i < MAX_PORT_NUM; i++) {
		wr32(&adapter->hw, RNPM_ETH_INPORT_POLICY_REG(i), 0);
		wr32(&peer_adapter->hw, RNPM_ETH_INPORT_POLICY_REG(i), 0);
	}

	wr32(&adapter->hw, RNPM_ETH_INPORT_POLICY_VAL, 0);
	wr32(&peer_adapter->hw, RNPM_ETH_INPORT_POLICY_VAL, 0);

	do_switch_loopback_set(adapter, en, adapter->lane,
			       to_switch_port(peer_adapter));

	do_switch_loopback_set(peer_adapter, en, peer_adapter->lane,
			       to_switch_port(adapter));

	if (peer_netdev)
		dev_put(peer_netdev);

	return 0;
}
static ssize_t switch_loopback_on_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct rnpm_adapter *adapter = netdev_priv(to_net_device(dev));

	return _switch_loopback(adapter, buf, 1) == 0 ? count : -EINVAL;
}

static ssize_t switch_loopback_off_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct rnpm_adapter *adapter = netdev_priv(to_net_device(dev));

	return _switch_loopback(adapter, buf, 0) == 0 ? count : -EINVAL;
}

static int __print_desc(char *buf, void *data, int len)
{
	u8 *ptr = (u8 *)data;
	int ret = 0, i = 0;

	for (i = 0; i < len; i++)
		ret += sprintf(buf + ret, "%02x ", *(ptr + i));

	return ret;
}

static struct netdev_queue *__rnpm_txring_txq(const struct rnpm_ring *ring)
{
	return netdev_get_tx_queue(ring->netdev, ring->queue_index);
}

static ssize_t txring_info_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct rnpm_adapter *adapter = netdev_priv(to_net_device(dev));
	struct rnpm_hw *hw = &adapter->hw;
	struct rnpm_ring *ring = NULL;
	struct rnpm_tx_buffer *tx_buf = NULL;
	struct rnpm_tx_desc *eop_desc;
	int s_id, e_id;
	int ret = 0, i;

	if (test_bit(__RNPM_DOWN, &adapter->state)) {
		ret += sprintf(buf + ret,
			       "error: port should be up first\n");
		return ret;
	}

	if (!adapter->d_ringinfo.txring_vaild) {
		ret = sprintf(
			buf + ret,
			"error: need setup debug tx ring num range first\n");
		return ret;
	}

	s_id = adapter->d_ringinfo.txring_start;
	e_id = adapter->d_ringinfo.txring_end;
	for (i = s_id; i <= e_id; i++) {
		struct netdev_queue *q;
		struct dql *dql;

		if (i >= adapter->num_tx_queues) {
			ret = sprintf(
				buf + ret,
				"error: tx queue id:%d larger than num_txq:%d, exit!\n",
				i, adapter->num_tx_queues);
			return ret;
		}

		ring = adapter->tx_ring[i];
		q = __rnpm_txring_txq(ring);
		dql = &q->dql;
		ret += sprintf(buf + ret,
			       "====== tx ring num %d info: ======\n", i);
		ret += sprintf(buf + ret, "BQL queue state:0x%lx:\n",
			       q->state);
		ret += sprintf(buf + ret,
			       "1: num_queued:%d adj_limit:%d limit:%d\n",
			       dql->num_queued, dql->adj_limit,
			       dql->limit);
		ret += sprintf(
			buf + ret,
			"2: num_completed:%d p_ovlimit:%d p_num_queued:%d\n",
			dql->num_completed, dql->prev_ovlimit,
			dql->prev_num_queued);
		ret += sprintf(buf + ret, "3: max_limit:%d min_limit:%d\n",
			       dql->max_limit, dql->min_limit);
		ret += sprintf(buf + ret, "next_to_use: %d\n",
			       ring->next_to_use);
		ret += sprintf(buf + ret, "next_to_clean: %d\n",
			       ring->next_to_clean);
		ret += sprintf(buf + ret, "hw_head: %d   hw_tail: %d\n",
			       rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_HEAD(
						ring->rnpm_queue_idx)),
			       rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_TAIL(
						ring->rnpm_queue_idx)));
		tx_buf = &ring->tx_buffer_info[ring->next_to_clean];
		eop_desc = tx_buf->next_to_watch;
		if (eop_desc) {
			ret += sprintf(buf + ret, "next_to_watch:\n");
			ret += __print_desc(buf + ret, eop_desc,
					    sizeof(*eop_desc));
			ret += sprintf(buf + ret, "\n");
		} else {
			ret += sprintf(buf + ret, "next_to_watch: no\n");
		}
	}

	return ret;
}

static ssize_t txring_info_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct rnpm_adapter *adapter = netdev_priv(to_net_device(dev));
	int s_id, e_id, cnt;

	/* start ring id + end ring id */
	cnt = sscanf(buf, "%d %d", &s_id, &e_id);
	if (cnt != 2 || s_id > e_id)
		return -EINVAL;
	if (e_id >= adapter->num_tx_queues)
		return -EINVAL;
	adapter->d_ringinfo.txring_start = s_id;
	adapter->d_ringinfo.txring_end = e_id;
	adapter->d_ringinfo.txring_vaild = true;
	return count;
}

static ssize_t rxring_info_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct rnpm_adapter *adapter = netdev_priv(to_net_device(dev));
	struct rnpm_hw *hw = &adapter->hw;
	struct rnpm_ring *ring = NULL;
	union rnpm_rx_desc *rx_desc;
	int s_id, e_id;
	int ret = 0, i;

	if (test_bit(__RNPM_DOWN, &adapter->state)) {
		ret += sprintf(buf + ret,
			       "error: port should be up first\n");
		return ret;
	}

	if (!adapter->d_ringinfo.rxring_vaild) {
		ret = sprintf(
			buf + ret,
			"error: need setup debug rx ring num range first\n");
		return ret;
	}

	s_id = adapter->d_ringinfo.rxring_start;
	e_id = adapter->d_ringinfo.rxring_end;
	for (i = s_id; i <= e_id; i++) {
		if (i >= adapter->num_rx_queues) {
			ret = sprintf(
				buf + ret,
				"error: rx queue id:%d larger than num_rxq:%d, exit!\n",
				i, adapter->num_rx_queues);
			return ret;
		}
		ring = adapter->rx_ring[i];
		ret += sprintf(buf + ret,
			       "====== rx ring num %d info: ======\n", i);
		ret += sprintf(buf + ret, "next_to_use: %d\n",
			       ring->next_to_use);
		ret += sprintf(buf + ret, "next_to_clean: %d\n",
			       ring->next_to_clean);
		ret += sprintf(buf + ret, "hw_head: %d   hw_tail: %d\n",
			       rd32(hw, RNPM_DMA_REG_RX_DESC_BUF_HEAD(
						ring->rnpm_queue_idx)),
			       rd32(hw, RNPM_DMA_REG_RX_DESC_BUF_TAIL(
						ring->rnpm_queue_idx)));
		rx_desc = RNPM_RX_DESC(ring, ring->next_to_clean);
		ret += sprintf(buf + ret, "next_to_clean desc: ");
		ret += __print_desc(buf + ret, rx_desc, sizeof(*rx_desc));
		ret += sprintf(buf + ret, "\n");
	}

	return ret;
}

static ssize_t rxring_info_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct rnpm_adapter *adapter = netdev_priv(to_net_device(dev));
	int s_id, e_id, cnt;

	/* start ring id + end ring id */
	cnt = sscanf(buf, "%d %d", &s_id, &e_id);
	if (cnt != 2 || s_id > e_id)
		return -EINVAL;
	if (e_id >= adapter->num_rx_queues)
		return -EINVAL;
	adapter->d_ringinfo.rxring_start = s_id;
	adapter->d_ringinfo.rxring_end = e_id;
	adapter->d_ringinfo.rxring_vaild = true;
	return count;
}

static DEVICE_ATTR_RO(root_slot_info);
static DEVICE_ATTR_RW(active_vid);
static DEVICE_ATTR_WO(pf_reset);
static DEVICE_ATTR_RO(ring_num);
static DEVICE_ATTR_RO(port_idx);
static DEVICE_ATTR_RO(version_info);
static DEVICE_ATTR_RO(nr_lane);
static DEVICE_ATTR_RW(lane_intr_en);
static DEVICE_ATTR_WO(prbs);
static DEVICE_ATTR_RO(debug_linkstat);
static DEVICE_ATTR_RO(debug_aptstat);
static DEVICE_ATTR_RO(tx_counter);
static DEVICE_ATTR_RO(rx_counter);
static DEVICE_ATTR_RW(bar4_reg);
static DEVICE_ATTR_RO(phy_statistics);
static DEVICE_ATTR_RO(port_stat2);
static DEVICE_ATTR_RW(pcs_reg);
static DEVICE_ATTR_WO(pma_rx2tx_loopback);
static DEVICE_ATTR_WO(pcs_rx2tx_loopback);
static DEVICE_ATTR_WO(mac_loopback);
static DEVICE_ATTR_WO(switch_loopback_off);
static DEVICE_ATTR_WO(switch_loopback_on);
static DEVICE_ATTR_RW(txring_info);
static DEVICE_ATTR_RW(rxring_info);
static struct attribute *dev_attrs[] = {
	&dev_attr_root_slot_info.attr,
	&dev_attr_active_vid.attr,
	&dev_attr_pf_reset.attr,
	&dev_attr_ring_num.attr,
	&dev_attr_port_idx.attr,
	&dev_attr_version_info.attr,
	&dev_attr_nr_lane.attr,
	&dev_attr_lane_intr_en.attr,
	&dev_attr_prbs.attr,
	&dev_attr_debug_linkstat.attr,
	&dev_attr_debug_aptstat.attr,
	&dev_attr_tx_counter.attr,
	&dev_attr_rx_counter.attr,
	&dev_attr_bar4_reg.attr,
	&dev_attr_phy_statistics.attr,
	&dev_attr_port_stat2.attr,
	&dev_attr_pcs_reg.attr,
	&dev_attr_pma_rx2tx_loopback.attr,
	&dev_attr_pcs_rx2tx_loopback.attr,
	&dev_attr_mac_loopback.attr,
	&dev_attr_switch_loopback_off.attr,
	&dev_attr_switch_loopback_on.attr,
	&dev_attr_txring_info.attr,
	&dev_attr_rxring_info.attr,
	NULL,
};

static DEVICE_ATTR_RW(si);
static DEVICE_ATTR_RO(sfp);
static DEVICE_ATTR_RW(autoneg);
static DEVICE_ATTR_RO(own_vpd);
static DEVICE_ATTR_RW(pci);
static DEVICE_ATTR_RW(sfp_tx_disable);
static DEVICE_ATTR_RW(link_traing);
static DEVICE_ATTR_RW(fec);
static DEVICE_ATTR_RO(temperature);
static DEVICE_ATTR_RW(phy_reg);

static struct attribute *vendor_dev_attrs[] = {
	&dev_attr_si.attr,
	&dev_attr_sfp.attr,
	&dev_attr_autoneg.attr,
	&dev_attr_own_vpd.attr,
	&dev_attr_pci.attr,
	&dev_attr_sfp_tx_disable.attr,
	&dev_attr_link_traing.attr,
	&dev_attr_fec.attr,
	&dev_attr_temperature.attr,
	&dev_attr_phy_reg.attr,
	NULL,
};

#ifndef NO_BIT_ATTRS
static struct bin_attribute *dev_bin_attrs[] = {
	&bin_attr_maintain,
	NULL,
};
#endif
/* should not call mailbox in the group */
static struct attribute_group dev_attr_grp = {
	.attrs = dev_attrs,
#ifndef NO_BIT_ATTRS
	.bin_attrs = dev_bin_attrs,
#endif
};

static const struct attribute_group vendor_attr_grp = {
	.name = "vendor",
	.attrs = vendor_dev_attrs,
};

static const struct attribute_group *attr_grps[] = {
	&dev_attr_grp,
	&vendor_attr_grp,
	NULL,
};

/* hwmon callback functions */
static ssize_t rnpm_hwmon_show_location(struct device __always_unused *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct hwmon_attr *rnpm_attr =
		container_of(attr, struct hwmon_attr, dev_attr);

	return snprintf(buf, PAGE_SIZE, "loc%u\n",
			rnpm_attr->sensor->location);
}

static ssize_t rnpm_hwmon_show_name(struct device __always_unused *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	return snprintf(buf, PAGE_SIZE, "rnpm\n");
}

static ssize_t rnpm_hwmon_show_temp(struct device __always_unused *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct hwmon_attr *rnpm_attr =
		container_of(attr, struct hwmon_attr, dev_attr);
	int value;

	/* reset the temp field */
	rnpm_attr->hw->mac.ops.get_thermal_sensor_data(rnpm_attr->hw);

	value = rnpm_attr->sensor->temp;
	/* display millidegree */
	value *= 1000;

	return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t
rnpm_hwmon_show_cautionthresh(struct device __always_unused *dev,
			      struct device_attribute *attr, char *buf)
{
	struct hwmon_attr *rnpm_attr =
		container_of(attr, struct hwmon_attr, dev_attr);
	int value = rnpm_attr->sensor->caution_thresh;

	/* display millidegree */
	value *= 1000;

	return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t
rnpm_hwmon_show_maxopthresh(struct device __always_unused *dev,
			    struct device_attribute *attr, char *buf)
{
	struct hwmon_attr *rnpm_attr =
		container_of(attr, struct hwmon_attr, dev_attr);
	int value = rnpm_attr->sensor->max_op_thresh;

	/* display millidegree */
	value *= 1000;
	return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

/**
 * rnpm_add_hwmon_attr - Create hwmon attr table for a hwmon sysfs file.
 * @adapter: pointer to the adapter structure
 * @offset: offset in the eeprom sensor data table
 * @type: type of sensor data to display
 *
 * For each file we want in hwmon's sysfs interface we need a device_attribute
 * This is included in our hwmon_attr struct that contains the references to
 * the data structures we need to get the data to display.
 */
static int rnpm_add_hwmon_attr(struct rnpm_adapter *adapter,
			       unsigned int offset, int type)
{
	unsigned int n_attr;
	struct hwmon_attr *rnpm_attr;

	n_attr = adapter->rnpm_hwmon_buff->n_hwmon;
	rnpm_attr = &adapter->rnpm_hwmon_buff->hwmon_list[n_attr];
	switch (type) {
	case RNPM_HWMON_TYPE_LOC:
		rnpm_attr->dev_attr.show = rnpm_hwmon_show_location;
		snprintf(rnpm_attr->name, sizeof(rnpm_attr->name),
			 "temp%u_label", offset + 1);

		break;
	case RNPM_HWMON_TYPE_NAME:
		rnpm_attr->dev_attr.show = rnpm_hwmon_show_name;
		snprintf(rnpm_attr->name, sizeof(rnpm_attr->name), "name");

		break;
	case RNPM_HWMON_TYPE_TEMP:
		rnpm_attr->dev_attr.show = rnpm_hwmon_show_temp;
		snprintf(rnpm_attr->name, sizeof(rnpm_attr->name),
			 "temp%u_input", offset + 1);

		break;
	case RNPM_HWMON_TYPE_CAUTION:
		rnpm_attr->dev_attr.show = rnpm_hwmon_show_cautionthresh;
		snprintf(rnpm_attr->name, sizeof(rnpm_attr->name),
			 "temp%u_max", offset + 1);

		break;
	case RNPM_HWMON_TYPE_MAX:
		rnpm_attr->dev_attr.show = rnpm_hwmon_show_maxopthresh;
		snprintf(rnpm_attr->name, sizeof(rnpm_attr->name),
			 "temp%u_crit", offset + 1);

		break;
	default:
		return -EPERM;
	}

	/* These always the same regardless of type */
	rnpm_attr->sensor =
		&adapter->hw.mac.thermal_sensor_data.sensor[offset];
	rnpm_attr->hw = &adapter->hw;
	rnpm_attr->dev_attr.store = NULL;
	rnpm_attr->dev_attr.attr.mode = 0444;
	rnpm_attr->dev_attr.attr.name = rnpm_attr->name;

	sysfs_attr_init(&rnpm_attr->dev_attr.attr);
	adapter->rnpm_hwmon_buff->attrs[n_attr] =
		&rnpm_attr->dev_attr.attr;

	++adapter->rnpm_hwmon_buff->n_hwmon;
	return 0;
}

static void
rnpm_sysfs_del_adapter(struct rnpm_adapter __maybe_unused *adapter)
{
}

/* called from rnpm_main.c */
void rnpm_sysfs_exit(struct rnpm_adapter *adapter)
{
	rnpm_sysfs_del_adapter(adapter);
	sysfs_remove_groups(&adapter->netdev->dev.kobj, &attr_grps[0]);
}

/* called from rnpm_main.c */
int rnpm_sysfs_init(struct rnpm_adapter *adapter, int port)
{
	int err, rc = 0;
	struct hwmon_buff *rnpm_hwmon;
	struct device *hwmon_dev;
	unsigned int i;

	err = sysfs_create_groups(&adapter->netdev->dev.kobj,
				  &attr_grps[0]);
	if (err != 0) {
		dev_err(&adapter->netdev->dev,
			"sysfs_create_group faild:err:%d\n", err);
		return err;
	}

	/* only  port0 and pcie devfn all both 0 register temperature hwmon */
	if (!!port || !!adapter->pdev->devfn)
		goto exit;

	/* If this method isn't defined we don't support thermals */
	if (adapter->hw.mac.ops.init_thermal_sensor_thresh == NULL)
		goto no_thermal;

	/* Don't create thermal hwmon interface if no sensors present */
	if (adapter->hw.mac.ops.init_thermal_sensor_thresh(&adapter->hw))
		goto no_thermal;

	rnpm_hwmon = devm_kzalloc(&adapter->pdev->dev, sizeof(*rnpm_hwmon),
				  GFP_KERNEL);
	if (!rnpm_hwmon) {
		rc = -ENOMEM;
		goto exit;
	}

	adapter->rnpm_hwmon_buff = rnpm_hwmon;
	for (i = 0; i < RNPM_MAX_SENSORS; i++) {
		/*
		 * Only create hwmon sysfs entries for sensors that have
		 * meaningful data for.
		 */
		if (adapter->hw.mac.thermal_sensor_data.sensor[i]
			    .location == 0)
			continue;

		/* Bail if any hwmon attr struct fails to initialize */
		rc = rnpm_add_hwmon_attr(adapter, i,
					 RNPM_HWMON_TYPE_CAUTION);
		if (rc)
			goto err;
		rc = rnpm_add_hwmon_attr(adapter, i, RNPM_HWMON_TYPE_TEMP);
		if (rc)
			goto err;
		rc = rnpm_add_hwmon_attr(adapter, i, RNPM_HWMON_TYPE_MAX);
		if (rc)
			goto err;
	}

	rnpm_hwmon->groups[0] = &rnpm_hwmon->group;
	rnpm_hwmon->group.attrs = rnpm_hwmon->attrs;
	hwmon_dev = devm_hwmon_device_register_with_groups(
		&adapter->pdev->dev, "rnpm", rnpm_hwmon,
		rnpm_hwmon->groups);

	if (IS_ERR(hwmon_dev)) {
		rc = PTR_ERR(hwmon_dev);
		goto exit;
	}
no_thermal:
	goto exit;
err:
	rnpm_sysfs_del_adapter(adapter);
exit:
	return rc;
}
