
#include <linux/if_bridge.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "sxe.h"
#include "sxe_hw.h"
#include "sxe_sriov.h"
#include "sxe_filter.h"
#include "sxe_netdev.h"
#include "sxe_rx_proc.h"
#include "sxe_ipsec.h"
#include "sxe_dcb.h"
#include "sxe_pci.h"
#include "sxe_ipsec.h"
#include "sxe_ring.h"

#ifdef CONFIG_PCI_IOV
static unsigned int max_vfs;
#ifndef SXE_TEST
module_param(max_vfs, uint, S_IRUGO | S_IWUSR);

MODULE_PARM_DESC(max_vfs,
		 "Max number of vf per physical function - default is zero and maximum value is 63. (Deprecated)");
#endif 
#endif 

static s32 sxe_vf_uc_addr_sync(struct sxe_adapter *adapter, u8 vf_idx,
					u16 index, u8 *mac_addr);

STATIC s32 sxe_vlvf_entry_find(struct sxe_hw *hw, u32 vlan)
{
	u32 vlvf;
	s32 idx = 0;
	struct sxe_adapter *adapter = hw->adapter;

	if (vlan == 0) {
		LOG_DEBUG_BDF("vlan:0 use default idx:0\n");
		goto l_out;
	}

	for (idx = SXE_VLVF_ENTRIES; --idx;) {
		vlvf = hw->filter.vlan.ops->pool_filter_read(hw, idx);
		if ((vlvf & VLAN_VID_MASK) == vlan) {
			break;
		}
	}

	LOG_DEBUG_BDF("found vlan[%u] in idx[%u]\n", vlan, idx);

l_out:
	return idx;
}

static void sxe_pf_promisc_vlvf_update(struct sxe_adapter *adapter, u32 vid)
{
	struct sxe_hw *hw = &adapter->hw;

	u32 bits, word;
	int idx;

	idx = sxe_vlvf_entry_find(hw, vid);
	if (!idx) {
		goto l_end;
	}

	word = idx * 2 + (PF_POOL_INDEX(0) / VF_BLOCK_BITS);
	bits = ~BIT(PF_POOL_INDEX(0) % VF_BLOCK_BITS);
	bits &= hw->filter.vlan.ops->pool_filter_bitmap_read(hw, word);

	LOG_DEBUG_BDF("in vlan idx[%u] vlvfb[%u] bits[%u]\n", idx, word, bits);
	if (!bits && !hw->filter.vlan.ops->pool_filter_bitmap_read(hw, word ^ 1)) {
		if (!(adapter->cap & SXE_VLAN_PROMISC)) {
			hw->filter.vlan.ops->pool_filter_bitmap_write(hw, word, 0);
		}

		hw->filter.vlan.ops->pool_filter_write(hw, idx, 0);
	}

l_end:
	return;
}

static s32 sxe_vf_vlan_configure(struct sxe_adapter *adapter, bool add, int vid,
			     u32 vf)
{
	struct sxe_hw *hw = &adapter->hw;
	s32 err;

	LOG_DEBUG_BDF("vf[%u] vid[%u] add = %s\n", vf, vid, add ? "true" : "false");
	if (add && test_bit(vid, adapter->vlan_ctxt.active_vlans)) {
		err = hw->filter.vlan.ops->filter_configure(hw, vid,
						PF_POOL_INDEX(0), true, false);
		if (err) {
			LOG_ERROR_BDF("vid[%u] has pf monitoring and"
				"alloc vlvf failed\n", vid);
			goto l_ret;
		}
	}

	err = hw->filter.vlan.ops->filter_configure(hw, vid, vf, add, false);

	if (add && !err) {
		LOG_DEBUG_BDF("vf[%u] config vid[%u] success\n", vf, vid);
		goto l_ret;
	}

	if (test_bit(vid, adapter->vlan_ctxt.active_vlans) ||
	    (adapter->cap & SXE_VLAN_PROMISC)) {
	    	sxe_pf_promisc_vlvf_update(adapter, vid);
	}

l_ret:
	return err;
}

static s32 sxe_port_vlan_disable(struct sxe_adapter *adapter, int vf)
{
	struct sxe_hw *hw = &adapter->hw;
	s32 ret;

	ret = sxe_vf_vlan_configure(adapter, false,
				adapter->vt_ctxt.vf_info[vf].pf_vlan, vf);

	sxe_vf_vlan_configure(adapter, true, 0, vf);
	hw->dma.ops->tx_vlan_tag_clear(hw, vf);
	hw->filter.vlan.ops->untagged_pkts_rcv_switch(hw, vf, true);

	adapter->vt_ctxt.vf_info[vf].pf_vlan = 0;
	adapter->vt_ctxt.vf_info[vf].pf_qos = 0;

	return ret;
}

static s32 sxe_port_vlan_enable(struct sxe_adapter *adapter, int vf,
				  u16 vlan, u8 qos)
{
	struct sxe_hw *hw = &adapter->hw;
	s32 ret;

	ret = sxe_vf_vlan_configure(adapter, true, vlan, vf);
	if (ret) {
		goto out;
	}

	sxe_vf_vlan_configure(adapter, false, 0, vf);

	hw->dma.ops->tx_vlan_tag_set(hw, vlan, qos, vf);
	hw->filter.vlan.ops->untagged_pkts_rcv_switch(hw, vf, false);

	adapter->vt_ctxt.vf_info[vf].pf_vlan = vlan;
	adapter->vt_ctxt.vf_info[vf].pf_qos = qos;
	LOG_DEV_INFO("setting vlan %d, qos 0x%x on vf %d\n", vlan, qos, vf);

	if (test_bit(SXE_DOWN, &adapter->state)) {
		LOG_DEV_WARN("the vf vlan has been set, but the pf device is not up.\n");
		LOG_DEV_WARN("bring the pf device up before attempting to use the vf device.\n");
	}

out:
	return ret;
}

static void sxe_vf_rate_factor_set(struct sxe_adapter *adapter, u8 vf_idx)
{
	struct sxe_hw *hw = &adapter->hw;
	u16 tx_rate = adapter->vt_ctxt.vf_info[vf_idx].tx_rate;
	u8 pool_mask = sxe_pool_mask_get(adapter);
	u8 ring_per_pool = __ALIGN_MASK(1, ~pool_mask);
	u32 value = 0;
	u8 idx;

	if (tx_rate) {
		value = sxe_mbps_link_speed_get(adapter->link.speed);

		value <<= SXE_RTTBCNRC_RF_INT_SHIFT;
		value /= tx_rate;

		value &= SXE_RTTBCNRC_RF_INT_MASK |
			SXE_RTTBCNRC_RF_DEC_MASK;

		value |= SXE_RTTBCNRC_RS_ENA;
	}

	hw->dma.ops->max_dcb_memory_window_set(hw, SXE_DCB_MMW_SIZE_DEFAULT);

	for (idx = 0; idx < ring_per_pool; idx++) {
		u32 reg_idx = (vf_idx * ring_per_pool) + idx;

		hw->dma.ops->dcb_tx_ring_rate_factor_set(hw, reg_idx, value);
	}

	return;
}

void sxe_vf_rate_update(struct sxe_adapter *adapter)
{
	u8 i;
	unsigned long flags;

	if (!adapter->vt_ctxt.is_rate_set) {
		goto l_out;
	}

	if (sxe_mbps_link_speed_get(adapter->link.speed) != SXE_LINK_MBPS_SPEED_DEFAULT) {
		adapter->vt_ctxt.is_rate_set = false;
		LOG_DEV_INFO("link speed has been changed. disable vf tx rate.\n");
	}

	spin_lock_irqsave(&adapter->vt_ctxt.vfs_lock, flags);
	for (i = 0; i < adapter->vt_ctxt.num_vfs; i++) {
		if (!adapter->vt_ctxt.is_rate_set) {
			adapter->vt_ctxt.vf_info[i].tx_rate = 0;
		}

		sxe_vf_rate_factor_set(adapter, i);
	}
	spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);

l_out:
	return;
}

s32 sxe_set_vf_rate(struct net_device *netdev, s32 vf_idx,
			s32 min_rate, s32 max_rate)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_virtual_context *vt_ctxt = &adapter->vt_ctxt;
	u32 mbps_speed = sxe_mbps_link_speed_get(adapter->link.speed);
	s32 ret = -EINVAL;

	if (vf_idx >= vt_ctxt->num_vfs) {
		LOG_ERROR_BDF("invalid vf_idx:%u exceed vf num:%u, "
			  "min_rate:%d max_rate:%d.(err:%d)\n",
			   vf_idx, vt_ctxt->num_vfs,
			   min_rate, max_rate, ret);
		goto l_out;
	}

	if (!adapter->link.is_up) {
		LOG_ERROR_BDF("dev not link up, can't set vf:%u link "
			  "rate min:%d max:%d.(err:%d)\n",
			  vf_idx, min_rate, max_rate, ret);
		goto l_out;
	}

	if (mbps_speed != SXE_LINK_MBPS_SPEED_DEFAULT) {
		LOG_ERROR_BDF("link speed:0x%x invalid,"
			  "vf_idx:%u min_rate:%u max_rate:%u.(err:%d)\n",
			  mbps_speed, vf_idx, min_rate, max_rate, ret);
		goto l_out;
	}

	if (min_rate) {
		LOG_ERROR_BDF("invalid min_rate:%u.(err:%d)\n", min_rate, ret);
		goto l_out;
	}

	if (max_rate && ((max_rate <= SXE_LINK_MBPS_SPEED_MIN)
		          || (max_rate > mbps_speed))) {
		LOG_ERROR_BDF("invalid max_rate:%u.(err:%d)\n", max_rate, ret);
		goto l_out;
	}

	adapter->vt_ctxt.is_rate_set = true;
	adapter->vt_ctxt.vf_info[vf_idx].tx_rate = max_rate;

	sxe_vf_rate_factor_set(adapter, vf_idx);
	ret = 0;

	LOG_INFO_BDF("vf:%u tx min_rate:%u max_rate:%u link_speed:%u.\n",
		      vf_idx, min_rate, max_rate, mbps_speed);

l_out:
	return ret;
}

s32 sxe_set_vf_vlan(struct net_device *netdev, s32 vf, u16 vlan,
			  u8 qos, __be16 vlan_proto)
{
	s32 ret;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	LOG_INFO_BDF("netdev[%p] pf set vf_idx[%u], max_vf[%u], vlan[%u],"
		"qos[%u], vlan_proto[%u]\n",
		netdev, vf, adapter->vt_ctxt.num_vfs, vlan, qos, vlan_proto);
	if ((vf >= adapter->vt_ctxt.num_vfs) || (vlan > SXE_MAX_VLAN_IDX) || (qos > SXE_MAX_QOS_IDX)) {
		ret = -EINVAL;
		goto l_out;
	}

	if (vlan_proto != htons(ETH_P_8021Q)) {
		ret = -EPROTONOSUPPORT;
		goto l_out;
	}

	if (vlan || qos) {
		if (adapter->vt_ctxt.vf_info[vf].pf_vlan) {
			LOG_INFO_BDF("vf[%u] pf_vlan[%u] exist, disable first\n",
				vf, adapter->vt_ctxt.vf_info[vf].pf_vlan);
			ret = sxe_port_vlan_disable(adapter, vf);
			if (ret) {
				goto l_out;
			}
		}

		ret = sxe_port_vlan_enable(adapter, vf, vlan, qos);
		LOG_INFO_BDF("pf enable vf[%u], vlan[%u],  qos[%u] ret = %d\n",
			vf, vlan, qos, ret);
	} else {
		ret = sxe_port_vlan_disable(adapter, vf);
	}

l_out:
	return ret;
}

static void sxe_clear_vf_vlan(struct sxe_adapter *adapter, u32 vf)
{
	struct sxe_hw *hw = &adapter->hw;
	u32 vlvfb_mask, pool_mask, i;
	u32 bits[2], vlvfb, vid, vfta, vlvf, word, mask;

	LOG_DEBUG_BDF("clear vf[%u] vlan\n",vf);

	pool_mask = ~BIT(PF_POOL_INDEX(0) % VF_BLOCK_BITS);
	vlvfb_mask = BIT(vf % VF_BLOCK_BITS);

	for (i = SXE_VLVF_ENTRIES; i--;) {
		word = i * 2 + vf / VF_BLOCK_BITS;
		vlvfb = hw->filter.vlan.ops->pool_filter_bitmap_read(hw, word);

		if (!(vlvfb & vlvfb_mask)) {
			continue;
		}

		vlvfb ^= vlvfb_mask;

		bits[word % 2] = vlvfb;
		bits[~word % 2] =
			hw->filter.vlan.ops->pool_filter_bitmap_read(hw, (word ^ 1));

		if (bits[(PF_POOL_INDEX(0) / VF_BLOCK_BITS) ^ 1] ||
			(bits[PF_POOL_INDEX(0) / VF_BLOCK_BITS] & pool_mask)) {
			goto update_vlvfb;
		}

		if (bits[0] || bits[1]) {
			goto update_vlvf;
		}

		vlvf = hw->filter.vlan.ops->pool_filter_read(hw, i);
		if (!vlvf) {
			goto update_vlvfb;
		}

		vid = vlvf & VLAN_VID_MASK;
		mask = BIT(vid % 32);

		vfta = hw->filter.vlan.ops->filter_array_read(hw, vid / 32);
		if (vfta & mask) {
			hw->filter.vlan.ops->
				filter_array_write(hw, vid / 32, vfta ^ mask);
		}

update_vlvf:
		hw->filter.vlan.ops->pool_filter_write(hw, i, 0);

		if (!(adapter->cap & SXE_VLAN_PROMISC)) {
			vlvfb = 0;
		}

update_vlvfb:
		hw->filter.vlan.ops->pool_filter_bitmap_write(hw, word, vlvfb);
	}

	return;
}

static s32 sxe_vf_mac_addr_set(struct sxe_adapter *adapter,
					u8 vf_idx, u8 *mac_addr)
{
	s32 ret;
	struct sxe_hw *hw = &adapter->hw;
	u8 *vf_addr = adapter->vt_ctxt.vf_info[vf_idx].mac_addr;

	ret = sxe_uc_addr_del(hw, adapter->mac_filter_ctxt.uc_addr_table,
				vf_addr, vf_idx);
	if (ret) {
		LOG_WARN_BDF("vf_idx:%d mac addr:%pM not in uc mac filter "
			"or zero addr.\n", vf_idx, vf_addr);
	}
	ret = sxe_uc_addr_add(hw, adapter->mac_filter_ctxt.uc_addr_table,
				mac_addr, vf_idx);
	if (ret < 0) {
		eth_zero_addr(vf_addr);
	} else {
		memcpy(vf_addr, mac_addr, ETH_ALEN);
	}

	LOG_INFO_BDF("add vf_idx:%d mac addr:%pM to uc filter ret:%d.\n",
		 vf_idx, vf_addr, ret);

	return ret;
}

s32 sxe_set_vf_mac(struct net_device *dev, s32 vf_idx, u8 *mac_addr)
{
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct sxe_vf_info *vf_info;
	s32 ret = 0;

	if (vf_idx >= adapter->vt_ctxt.num_vfs) {
		ret = -EINVAL;
		LOG_ERROR_BDF("set vf mac addr:%pM fail due to "
			     "vf_idx:%d exceed num_vfs:%d.(err:%d).\n",
			      mac_addr, vf_idx, adapter->vt_ctxt.num_vfs, ret);
		goto l_out;
	}

	vf_info = &adapter->vt_ctxt.vf_info[vf_idx];

	if (is_valid_ether_addr(mac_addr)) {
		LOG_DEV_INFO("setting mac address:%pM on vf:%u.\n", mac_addr, vf_idx);
		LOG_DEV_INFO("reload vf driver to make mac addr effective.\n");

		ret = sxe_vf_mac_addr_set(adapter, vf_idx, mac_addr);
		if (ret < 0) {
			LOG_DEV_WARN("vf:%d set mac addr:%pM fail "
				     "due to no space.(err:%d)\n",
				     vf_idx, mac_addr, ret);
		} else {
			vf_info->mac_from_pf = true;
			if (test_bit(SXE_DOWN, &adapter->state)) {
				LOG_DEV_WARN("vf:%u mac address has been set by pf, "
					     "but pf device is not up.\n",
					      vf_idx);
				LOG_DEV_WARN("bring pf device up before attempting "
					     "to use the vf device.\n");
			}
		}
	} else if (is_zero_ether_addr(mac_addr)) {
		if (is_zero_ether_addr(vf_info->mac_addr)) {
			LOG_INFO_BDF("vf:%u mac addr is zero, skip dup set to zero.\n",
				  vf_idx);
			goto l_out;
		}

		LOG_DEV_INFO("delete vf:%u mac addr\n",  vf_idx);

		ret = sxe_uc_addr_del(&adapter->hw,
					adapter->mac_filter_ctxt.uc_addr_table,
					vf_info->mac_addr, vf_idx);
		if (ret < 0) {
			LOG_DEV_WARN("vf:%u mac addr:%pM delete fail.\n",
				      vf_idx, vf_info->mac_addr);
		} else {
			vf_info->mac_from_pf = false;
			memset(&vf_info->mac_addr, 0, ETH_ALEN);
		}
	} else {
		ret = -EINVAL;
		LOG_ERROR_BDF("mac addr:%pM set on vf:%u invalid.(err:%d)\n",
			     mac_addr, vf_idx, ret);
	}

l_out:
	return ret;
}

static s32 sxe_mbx_msg_send(struct sxe_adapter *adapter, u16 index, u32 *msg, u16 len)
{
	s32 ret;
	struct sxe_hw *hw = &adapter->hw;

	ret = hw->mbx.ops->msg_send(hw, msg, len, index);
	if (ret) {
		LOG_ERROR_BDF("vf:%u send msg:0x%x len:%u fail.(err:%d)\n",
			  index, *msg, len, ret);
	}

	return ret;
}

void sxe_vf_trust_update_notify(struct sxe_adapter *adapter, u16 index)
{
	u32 msg = SXE_CTRL_MSG_REINIT;
	unsigned long flags;

	spin_lock_irqsave(&adapter->vt_ctxt.vfs_lock, flags);

	sxe_mbx_msg_send(adapter, index, &msg, SXE_MSG_NUM(sizeof(msg)));

	spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);

	LOG_WARN_BDF("pf send trust status change ctrl msg:0x%x to vf:%u\n",
		 msg, index);

	return;
}

void sxe_netdev_down_notify_vf_all(struct sxe_adapter *adapter)
{
	struct sxe_virtual_context *vt_ctxt = &adapter->vt_ctxt;
	u8 i;
	u32 msg;
	unsigned long flags;

	spin_lock_irqsave(&adapter->vt_ctxt.vfs_lock, flags);
	for (i = 0; i < vt_ctxt->num_vfs; i++) {
		msg = SXE_CTRL_MSG_NETDEV_DOWN;
		sxe_mbx_msg_send(adapter, i, &msg, SXE_MSG_NUM(sizeof(msg)));
	}
	spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);

	LOG_WARN_BDF("pf send netdev down ctrl msg:0x%x to all vf, num_vfs:%u\n",
		  msg, vt_ctxt->num_vfs);

	return;
}

void sxe_link_update_notify_vf_all(struct sxe_adapter *adapter)
{
	struct sxe_virtual_context *vt_ctxt = &adapter->vt_ctxt;
	u8 i;
	u32 msg;
	unsigned long flags;

	spin_lock_irqsave(&adapter->vt_ctxt.vfs_lock, flags);
	for (i = 0; i < vt_ctxt->num_vfs; i++) {
		if (vt_ctxt->vf_info[i].is_ready) {
			msg = SXE_CTRL_MSG_LINK_UPDATE;
		} else {
			msg = SXE_CTRL_MSG_REINIT;
		}
		sxe_mbx_msg_send(adapter, i, &msg, SXE_MSG_NUM(sizeof(msg)));
	}
	spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);

	LOG_WARN_BDF("pf send link update ctrl msg to all vf "
		     "num_vfs:%u.\n",
		      vt_ctxt->num_vfs);

	return;
}

static void sxe_vf_vlan_rst(struct sxe_adapter *adapter, u8 vf_idx)
{
	struct sxe_vf_info *vf_info = &adapter->vt_ctxt.vf_info[vf_idx];
	struct sxe_hw *hw = &adapter->hw;
	u8 tc_num = sxe_dcb_tc_get(adapter);

	sxe_clear_vf_vlan(adapter, vf_idx);

	sxe_vf_vlan_configure(adapter, true, vf_info->pf_vlan, vf_idx);

	hw->filter.vlan.ops->untagged_pkts_rcv_switch(hw, vf_idx, !vf_info->pf_vlan);

	if (!vf_info->pf_vlan && !vf_info->pf_qos && !tc_num) {
		hw->dma.ops->tx_vlan_tag_clear(hw, vf_idx);
	} else {
		if (vf_info->pf_qos || !tc_num) {
			hw->dma.ops->tx_vlan_tag_set(hw, vf_info->pf_vlan,
					vf_info->pf_qos, vf_idx);
		} else {
			hw->dma.ops->tx_vlan_tag_set(hw, vf_info->pf_vlan,
					adapter->dcb_ctxt.default_up, vf_idx);
		}

		if (vf_info->spoof_chk_enabled) {
			hw->dma.ops->pool_mac_anti_spoof_set(hw, vf_idx, true);
			hw->dma.ops->pool_vlan_anti_spoof_set(hw, vf_idx, true);
		}
	}

	return;
}

void sxe_vf_hw_rst(struct sxe_adapter *adapter, u8 vf_idx)
{
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_vf_info *vf_info = &adapter->vt_ctxt.vf_info[vf_idx];
	u16 pool_mask   = sxe_pool_mask_get(adapter);
	u8 ring_cnt = __ALIGN_MASK(1, ~pool_mask); 

	sxe_vf_vlan_rst(adapter, vf_idx);
	vf_info->mc_hash_used = 0;

#ifdef SXE_IPSEC_CONFIGURE
	sxe_vf_ipsec_entry_clear(adapter, vf_idx);
#endif

	__sxe_set_rx_mode(adapter->netdev, false);

	sxe_uc_addr_del(hw, adapter->mac_filter_ctxt.uc_addr_table,
			vf_info->mac_addr, vf_idx);

	sxe_vf_uc_addr_sync(adapter, vf_idx, 0, NULL);


	hw->dma.ops->vf_tx_ring_disable(hw, ring_cnt, vf_idx);

	LOG_INFO_BDF("vf_idx:%u vf flr done.\n", vf_idx);

	return;
}

static void sxe_vf_rxtx_set(struct sxe_adapter *adapter, u8 vf_idx)
{
	u32 enable_pool;
	u8 reg_idx = vf_idx / VF_BLOCK_BITS;
	u8 bit_idx = vf_idx % VF_BLOCK_BITS;
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_vf_info *vf_info = &adapter->vt_ctxt.vf_info[vf_idx];

	enable_pool = hw->dma.ops->tx_pool_get(hw, reg_idx);
	if(vf_info->link_enable){
		enable_pool |= BIT(bit_idx);
	} else {
		enable_pool &= ~BIT(bit_idx);
	}
	hw->dma.ops->tx_pool_set(hw, reg_idx, enable_pool);

	enable_pool = hw->dma.ops->rx_pool_get(hw, reg_idx);
	if ((adapter->netdev->mtu + ETH_HLEN > ETH_FRAME_LEN) ||
		!vf_info->link_enable) {
		enable_pool &= ~BIT(bit_idx);
	} else {
		enable_pool |= BIT(bit_idx);
	}
	hw->dma.ops->rx_pool_set(hw, reg_idx, enable_pool);
}

static void sxe_vf_rxtx_rst(struct sxe_adapter *adapter, u8 vf_idx)
{
	u16 mask = sxe_pool_mask_get(adapter);
	u8 ring_per_pool = __ALIGN_MASK(1, ~mask);
	u8 reg_idx = vf_idx / VF_BLOCK_BITS;
	u8 bit_idx = vf_idx % VF_BLOCK_BITS;
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_vf_info *vf_info = &adapter->vt_ctxt.vf_info[vf_idx];

	hw->dma.ops->pool_rx_ring_drop_enable(hw, vf_idx,
					vf_info->pf_vlan, ring_per_pool);

	sxe_vf_rxtx_set(adapter, vf_idx);

	vf_info->is_ready = true;

	hw->dma.ops->spoof_count_enable(hw, reg_idx, bit_idx);

	hw->dma.ops->vf_tx_desc_addr_clear(hw, vf_idx, ring_per_pool);

	return;
}

static void sxe_vf_rst_msg_reply(struct sxe_adapter *adapter, u8 vf_idx)
{
	struct sxe_rst_reply reply = {};
	struct sxe_vf_info *vf_info = &adapter->vt_ctxt.vf_info[vf_idx];
	u8 *mac_addr = vf_info->mac_addr;

	reply.msg_type = SXE_VFREQ_RESET;
	if (!is_zero_ether_addr(mac_addr) &&
		vf_info->mac_from_pf) {
		reply.msg_type |= SXE_MSGTYPE_ACK;
		memcpy((u8 *)reply.mac_addr, mac_addr, ETH_ALEN);
	} else {
		reply.msg_type |= SXE_MSGTYPE_NACK;
	}

	reply.sw_mtu = sxe_sw_mtu_get(adapter);

	LOG_INFO_BDF("vf_idx:%d reset msg:0x%x handle done.mac addr:%pM mc type:%d "
		"mac_from_pf:%d sw_mtu:%u\n",
		vf_idx, reply.msg_type,
		mac_addr, SXE_MC_FILTER_TYPE0,
		vf_info->mac_from_pf,
		reply.sw_mtu);

	reply.mc_filter_type = SXE_MC_FILTER_TYPE0;

	adapter->hw.mbx.ops->msg_send(&adapter->hw, (u32 *)&reply,
				SXE_MSG_NUM(sizeof(reply)), vf_idx);

	return;
}

static void sxe_vf_rst_msg_handle(struct sxe_adapter *adapter, u8 vf_idx)
{
	struct sxe_hw *hw = &adapter->hw;
	u8 *mac_addr = adapter->vt_ctxt.vf_info[vf_idx].mac_addr;

	LOG_MSG_INFO(probe, "receive vf_idx:%d reset msg.\n", vf_idx);

	sxe_vf_hw_rst(adapter, vf_idx);

	hw->mbx.ops->mbx_mem_clear(hw, vf_idx);

	if (!is_zero_ether_addr(mac_addr)) {
		sxe_vf_mac_addr_set(adapter, vf_idx, mac_addr);
	}

	sxe_vf_rxtx_rst(adapter, vf_idx);

	sxe_vf_rst_msg_reply(adapter, vf_idx);

	return;
}

static s32 sxe_vf_mac_addr_set_handler(struct sxe_adapter *adapter,
						    u32 *msg, u8 vf_idx)
{
	struct sxe_uc_addr_msg mac_msg = *(struct sxe_uc_addr_msg *)msg;
	struct sxe_vf_info *vf_info = &adapter->vt_ctxt.vf_info[vf_idx];
	s32 ret;

	if (!is_valid_ether_addr(mac_msg.uc_addr)) {
		ret = -SXE_ERR_PARAM;
		LOG_MSG_WARN(drv, "vf_idx:j%u invalid mac addr:%pM.(err:%d)\n",
			      vf_idx, mac_msg.uc_addr, ret);
		goto l_out;
	}

	if (vf_info->mac_from_pf && !vf_info->trusted &&
		!ether_addr_equal(vf_info->mac_addr, mac_msg.uc_addr)) {
		ret = -SXE_ERR_PARAM;
		LOG_MSG_WARN(drv, "vf_idx:%d mac addr:%pM attempt to "
			 "override admin mac addr:%pM.\n",
			 vf_idx, mac_msg.uc_addr, vf_info->mac_addr);
		LOG_MSG_WARN(drv, "reload the VF driver to resume operations.\n");
		goto l_out;
	}

	ret = sxe_vf_mac_addr_set(adapter, vf_idx, mac_msg.uc_addr);
	if (ret < 0) {
		LOG_INFO_BDF("vf_idx:%d set mac addr:%pM fail.(err:%d)\n",
			  vf_idx, mac_msg.uc_addr, ret);
		goto l_out;
	}

	LOG_INFO_BDF("vf:%d set mac addr:%pM to filter entry:%d done."
		  "mac_from_pf:%d trusted:%d vf[%d]->mac_addr:%pM.\n",
		  vf_idx, mac_msg.uc_addr, ret,
		  vf_info->mac_from_pf, vf_info->trusted,
		  vf_idx, vf_info->mac_addr);

	ret = 0;

l_out:
	return ret;

}

static s32 sxe_vf_mc_addr_sync(struct sxe_adapter *adapter,
						    u32 *msg, u8 vf_idx)
{
	struct sxe_mc_sync_msg *mc_msg = (struct sxe_mc_sync_msg *)msg;
	u8 mc_cnt = min_t(u16, mc_msg->mc_cnt, SXE_VF_MC_ENTRY_NUM_MAX);
	struct sxe_vf_info *vf_info = &adapter->vt_ctxt.vf_info[vf_idx];
	struct sxe_mac_filter_context *mac_filter = &adapter->mac_filter_ctxt;
	struct sxe_hw *hw = &adapter->hw;
	u8 i;

	for (i = 0; i < SXE_MTA_ENTRY_NUM_MAX; i++) {
		hw->filter.mac.ops->mta_hash_table_set(hw, i,
					mac_filter->mc_hash_table[i]);
	}

	vf_info->mc_hash_used = mc_cnt;
	memset(vf_info->mc_hash, 0, sizeof(vf_info->mc_hash));

	for (i = 0; i < mc_cnt; i++) {
		vf_info->mc_hash[i] = mc_msg->mc_addr_extract[i];
		LOG_INFO_BDF("vf_idx:%u mc_cnt:%u mc_hash[%d]:0x%x\n",
			     vf_idx, mc_cnt, i, vf_info->mc_hash[i]);
	}

	sxe_vf_mc_addr_restore(adapter);

	return 0;
}

static void sxe_vf_uc_addr_del(struct sxe_adapter *adapter,
					 u8 vf_idx)
{
	struct sxe_vf_uc_addr_list *entry;
	struct sxe_virtual_context *vf = &adapter->vt_ctxt;
	s32 ret;

	list_for_each_entry(entry, &vf->head.list, list) {
		if (entry->vf_idx == vf_idx) {
			entry->vf_idx = -1;
			entry->free = true;
			entry->is_macvlan = false;
			ret = sxe_uc_addr_del(&adapter->hw,
					adapter->mac_filter_ctxt.uc_addr_table,
					entry->uc_addr, vf_idx);
			LOG_INFO_BDF("del vf:%d mac addr:%pM in mac list done(ret:%d).\n",
				  vf_idx, entry->uc_addr, ret);
		}
	}

	return;
}

static s32 sxe_vf_uc_addr_sync(struct sxe_adapter *adapter, u8 vf_idx,
					u16 index, u8 *mac_addr)
{
	struct sxe_vf_uc_addr_list *entry = NULL;
	struct sxe_virtual_context *vf = &adapter->vt_ctxt;
	s32 ret = 0;

	if (index <= 1) {
		sxe_vf_uc_addr_del(adapter, vf_idx);
	}

	if (index == 0) {
		LOG_INFO_BDF("del vf_idx:%d all mac addr done.\n", vf_idx);
		goto l_out;
	}

	list_for_each_entry(entry, &vf->head.list, list) {
		if (entry->free) {
			break;
		}
	}

	if (!entry || !entry->free) {
		ret = -SXE_ERR_NO_SPACE;
		LOG_ERROR_BDF("vf_idx:%d has no space to sync %pM.(err:%d)\n",
			  vf_idx, mac_addr, ret);
		goto l_out;
	}

	ret = sxe_uc_addr_add(&adapter->hw,
				adapter->mac_filter_ctxt.uc_addr_table,
				mac_addr, vf_idx);
	if (ret < 0) {
		LOG_ERROR_BDF("vf_idx:%d list member index:%d add %p fail.\n",
			  vf_idx, index, mac_addr);
		goto l_out;
	}

	entry->free = false;
	entry->is_macvlan = true;
	entry->vf_idx = vf_idx;
	ether_addr_copy(entry->uc_addr, mac_addr);

l_out:
	return ret;
}

STATIC s32 sxe_vf_uc_addr_sync_handler(struct sxe_adapter *adapter,
						    u32 *msg, u8 vf_idx)
{
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_uc_sync_msg *uc_msg = (struct sxe_uc_sync_msg *)msg;
	struct sxe_vf_info *vf_info = &adapter->vt_ctxt.vf_info[vf_idx];
	u16 index = uc_msg->index;
	u8 *mac_addr = (u8 *)uc_msg->addr;
	s32 ret;

	if (vf_info->mac_from_pf && !vf_info->trusted && index) {
		ret = -SXE_ERR_OPRATION_NOT_PERM;
		LOG_MSG_ERR(drv, "vf:%d has set mac addr  by pf and vf not trusted, "
			  "deny add uc mac addr.(err:%d)\n",
			   vf_idx, ret);
		goto l_out;
	}

	if (index) {
		if (!is_valid_ether_addr(mac_addr)) {
			ret = -SXE_ERR_PARAM;
			LOG_ERROR_BDF("index:%u vf:%d set invalid addr:%pM.(err:%d)\n",
				  index, vf_idx, mac_addr, ret);
			goto l_out;
		}

		if (adapter->vt_ctxt.vf_info[vf_idx].spoof_chk_enabled) {
			hw->dma.ops->pool_mac_anti_spoof_set(hw, vf_idx, false);
			hw->dma.ops->pool_vlan_anti_spoof_set(hw, vf_idx, false);
		}
	}

	ret = sxe_vf_uc_addr_sync(adapter, vf_idx, index, mac_addr);
	if (ret < 0) {
		LOG_MSG_ERR(drv, "msg_type:0x%x vf_idx:%d mac addr:%pM sync "
			  "msg handler err.(%d)\n",
			  uc_msg->msg_type, vf_idx, mac_addr, ret);
		goto l_out;
	}

	LOG_INFO_BDF("msg_type:0x%x vf_idx:%d index:%d mac addr:%pM ret:%d "
		 "uc mac msg handler done.\n",
		  uc_msg->msg_type, vf_idx,
		  index, mac_addr, ret);

l_out:
	return min_t(s32, ret, 0);
}

STATIC s32 sxe_mbx_api_set_handler(struct sxe_adapter *adapter,
						    u32 *msg, u8 vf_idx)
{
	struct sxe_mbx_api_msg *api_msg = (struct sxe_mbx_api_msg *)msg;
	s32 ret = -SXE_ERR_PARAM;

	switch (api_msg->api_version) {
	case SXE_MBX_API_10:
	case SXE_MBX_API_11:
	case SXE_MBX_API_12:
	case SXE_MBX_API_13:
	case SXE_MBX_API_14:
		adapter->vt_ctxt.vf_info[vf_idx].mbx_version = api_msg->api_version;
		ret = 0;
		LOG_INFO_BDF("mailbox api version:%u set success.\n", api_msg->api_version);
		break;
	default:
		LOG_MSG_ERR(drv, "invalid mailbox api version:%u.\n",
			  api_msg->api_version);
		break;
	}

	return ret;
}

static s32 sxe_pf_ring_info_get(struct sxe_adapter *adapter, u32 *msg, u8 vf_idx)
{
	struct sxe_ring_info_msg *ring_msg = (struct sxe_ring_info_msg *)msg;
	u16 mask = sxe_pool_mask_get(adapter);
	u8 default_tc = 0;
	u8 num_tc = sxe_dcb_tc_get(adapter);

	ring_msg->max_rx_num = __ALIGN_MASK(1, ~mask);
	ring_msg->max_tx_num = __ALIGN_MASK(1, ~mask);

	if (num_tc > 1) {
		default_tc = netdev_get_prio_tc_map(adapter->netdev,
					adapter->dcb_ctxt.default_up);
		ring_msg->tc_num = num_tc;
	} else if (adapter->vt_ctxt.vf_info->pf_vlan || adapter->vt_ctxt.vf_info->pf_qos) {
		ring_msg->tc_num = 1;
	} else {
		ring_msg->tc_num = 0;
	};

	ring_msg->default_tc = default_tc;

	LOG_INFO_BDF("vf:%d get ring info tc_num:%d default_tc:%d "
		"max_tx_num:%d max_rx_num:%d.\n",
		vf_idx, ring_msg->tc_num, ring_msg->default_tc,
		ring_msg->max_tx_num, ring_msg->max_rx_num);

	return 0;
}

static s32 sxe_vf_rx_max_frame_set(struct sxe_adapter *adapter,
						u32 *msgbuf, u8 vf_idx)
{
	struct sxe_hw *hw = &adapter->hw;
	u32 max_frame = msgbuf[1] + ETH_HLEN + ETH_FCS_LEN;

	struct net_device *dev = adapter->netdev;
	int pf_max_frame = dev->mtu + ETH_HLEN;
	u32 reg_offset, vf_shift;
	s32 ret = 0;

	if (pf_max_frame > ETH_FRAME_LEN) {
		LOG_INFO_BDF("pf_max_frame=%u\n", pf_max_frame);
	} else {
		if (max_frame > (ETH_FRAME_LEN + ETH_FCS_LEN)) {
			LOG_ERROR_BDF("frame oversize, pf_max_frame=%u, "
				      "vf:%u max_frame=%u\n",
				      pf_max_frame, vf_idx, max_frame);
			ret = -EINVAL;
		}
	}

	vf_shift = vf_idx % VF_BLOCK_BITS;
	reg_offset = vf_idx / VF_BLOCK_BITS;

	hw->dbu.ops->vf_rx_switch(hw, reg_offset, vf_shift, !!ret);
	if (ret) {
		LOG_MSG_ERR(drv, "vf:%u max_frame %d out of range\n",
			      vf_idx, max_frame);
		goto l_end;
	}

	if (max_frame > SXE_MAX_JUMBO_FRAME_SIZE) {
		LOG_MSG_ERR(drv, "vf:%u max_frame %d out of range\n", vf_idx, max_frame);
		ret = -EINVAL;
		goto l_end;
	}

	if ((pf_max_frame + ETH_FCS_LEN) < max_frame) {
		ret = -EINVAL;
		LOG_ERROR_BDF("vf:%u max_frame:%u exceed pf max_frame:%u not "
			      "permited.(err:%d)\n",
			      vf_idx, max_frame,
			      (pf_max_frame + ETH_FCS_LEN), ret);
		goto l_end;
	}

	LOG_MSG_INFO(hw, "vf:%u requests change max MTU to %d pf_mtu:%u.\n",
		      vf_idx, max_frame,
		      adapter->netdev->mtu);

l_end:
	return ret;
}

static s32 sxe_vf_vlan_update_handler(struct sxe_adapter *adapter,
				 u32 *msgbuf, u8 vf)
{
	u32 add = (msgbuf[0] & SXE_VFREQ_MSGINFO_MASK) >> SXE_VFREQ_MSGINFO_SHIFT;
	u32 vid = (msgbuf[1] & SXE_VLVF_VLANID_MASK);
	u8 tcs = sxe_dcb_tc_get(adapter);
	s32 ret;

	if (adapter->vt_ctxt.vf_info[vf].pf_vlan || tcs) {
		LOG_MSG_WARN(drv,
		       "vf %d attempted to override administratively set VLAN configuration\n"
		       "reload the vf driver to resume operations\n",
		       vf);
		ret = -1;
		goto l_ret;
	}

	if (!vid && !add) {
		LOG_WARN_BDF("do not allow remove vlan 0\n");
		ret = 0;
		goto l_ret;
	}

	ret = sxe_vf_vlan_configure(adapter, add, vid, vf);
	LOG_INFO_BDF("vf[%u] %s vid[%u] finished, and ret = %d\n",
			vf, add ? "add" : "delete", vid, ret);

l_ret:
	return ret;
}

static s32 sxe_vf_cast_mode_handler(struct sxe_adapter *adapter,
				 u32 *msgbuf, u8 vf_idx)
{
	s32 ret = 0;
	struct sxe_cast_mode_msg cast_msg = *(struct sxe_cast_mode_msg *)msgbuf;
	struct sxe_vf_info *vf_info = &adapter->vt_ctxt.vf_info[vf_idx];
	struct sxe_hw *hw = &adapter->hw;
	u32 mode = cast_msg.cast_mode;
	u32 disable;
	u32 enable;
	u32 value;

	if ((cast_msg.cast_mode > SXE_CAST_MODE_MULTI)
		&& !vf_info->trusted) {
		mode = SXE_CAST_MODE_MULTI;
	}

	if (vf_info->cast_mode == mode) {
		LOG_INFO_BDF("vf:%d msg.cast_mode:0x%x mode:0x%x trust:%d\n",
			 vf_idx, cast_msg.cast_mode, mode, vf_info->trusted);
		goto l_out;
	}

	switch (mode) {
	case SXE_CAST_MODE_NONE:
		disable = SXE_VMOLR_BAM | SXE_VMOLR_ROMPE | SXE_VMOLR_MPE;
		enable = 0;
		break;

	case SXE_CAST_MODE_MULTI:
		disable = SXE_VMOLR_MPE;
		enable = SXE_VMOLR_BAM | SXE_VMOLR_ROMPE;
		break;

	case SXE_CAST_MODE_ALLMULTI:
		disable = 0;
		enable = SXE_VMOLR_BAM | SXE_VMOLR_ROMPE |
			SXE_VMOLR_MPE;
		break;

	case SXE_CAST_MODE_PROMISC:
		ret = -EOPNOTSUPP;
		LOG_ERROR_BDF("vf:%d promisc mode not support.(ret:%d)\n",
		         vf_idx, ret);
		goto l_out;

	default:
		ret = -SXE_ERR_PARAM;
		LOG_ERROR_BDF("vf:%u invalid cast mode:0x%x.\n", vf_idx, mode);
		goto l_out;
	}

	value = hw->filter.mac.ops->pool_rx_mode_get(hw, vf_idx);
	value &= ~disable;
	value |= enable;
	hw->filter.mac.ops->pool_rx_mode_set(hw, value, vf_idx);

	LOG_INFO_BDF("vf:%d filter reg:0x%x mode:%d.\n", vf_idx, value, mode);

	vf_info->cast_mode = mode;

l_out:
	return ret;
}

static s32 sxe_vf_link_enable_get(struct sxe_adapter *adapter,
			u32 *msgbuf, u8 vf_idx)
{
	s32 ret = 0;
	struct sxe_link_enable_msg *msg = (struct sxe_link_enable_msg *)msgbuf;

	switch(adapter->vt_ctxt.vf_info[vf_idx].mbx_version) {
	case SXE_MBX_API_12:
	case SXE_MBX_API_13:
	case SXE_MBX_API_14:
		msg->link_enable = adapter->vt_ctxt.vf_info[vf_idx].link_enable;
		break;
	default:
		ret = -EOPNOTSUPP;
	}
	LOG_INFO_BDF("ret: %d, vf: %d, link_enable: %d\n", ret, vf_idx, msg->link_enable);

	return ret;
}

static s32 sxe_pf_rss_redir_tbl_get(struct sxe_adapter *adapter,
			u32 *msgbuf, u8 vf_idx)
{
	s32 ret = 0;
	u32 i, j;
	const u8 *reta = adapter->rss_indir_tbl;
	u32 reta_size = sxe_rss_redir_tbl_size_get();
	struct sxe_redir_tbl_msg *msg = (struct sxe_redir_tbl_msg *)msgbuf;

	if (!adapter->vt_ctxt.vf_info[vf_idx].rss_query_enabled) {
		LOG_WARN_BDF("vf[%u] rss disable\n", vf_idx);
		ret = -EPERM;
		goto l_end;
	}

	for (i = 0; i < reta_size / 16; i++) {
		msg->entries[i] = 0;
		for (j = 0; j < 16; j++)
			msg->entries[i] |=
				(u32)(reta[16 * i + j] & 0x3) << (2 * j);
	}

l_end:
	return ret;
}

static s32 sxe_pf_rss_key_get(struct sxe_adapter *adapter,
				u32 *msgbuf, u8 vf_idx)
{
	s32 ret = 0;
	struct sxe_rss_hsah_key_msg *msg = (struct sxe_rss_hsah_key_msg *)msgbuf;

	if (!adapter->vt_ctxt.vf_info[vf_idx].rss_query_enabled) {
		LOG_WARN_BDF("vf[%u] rss disable\n", vf_idx);
		ret = -EPERM;
		goto l_end;
	}

	memcpy(msg->hash_key, adapter->rss_key, SXE_RSS_KEY_SIZE);

l_end:
	return ret;
}

static s32 sxe_pf_rss_conf_get(struct sxe_adapter *adapter,
				u32 *msgbuf, u8 vf_idx)
{
	struct sxe_rss_hash_msg *msg = (struct sxe_rss_hash_msg *)msgbuf;
	u8 rss_key[SXE_RSS_KEY_SIZE] = {0};
	struct sxe_rss_hash_config rss_conf;

	rss_conf.rss_key = rss_key;
	sxe_rss_hash_conf_get(adapter, &rss_conf);

	memcpy(msg->hash_key, rss_conf.rss_key, SXE_RSS_KEY_SIZE);
	msg->rss_hf = rss_conf.rss_hf;

	return 0;
}

STATIC struct sxe_msg_table msg_table[] = {
	[SXE_VFREQ_MAC_ADDR_SET] = {SXE_VFREQ_MAC_ADDR_SET, sxe_vf_mac_addr_set_handler},
	[SXE_VFREQ_MC_ADDR_SYNC] = {SXE_VFREQ_MC_ADDR_SYNC, sxe_vf_mc_addr_sync},
	[SXE_VFREQ_VLAN_SET] = {SXE_VFREQ_VLAN_SET, sxe_vf_vlan_update_handler},
	[SXE_VFREQ_LPE_SET] = {SXE_VFREQ_LPE_SET, sxe_vf_rx_max_frame_set},
	[SXE_VFREQ_UC_ADDR_SYNC] = {SXE_VFREQ_UC_ADDR_SYNC, sxe_vf_uc_addr_sync_handler},
	[SXE_VFREQ_API_NEGOTIATE] = {SXE_VFREQ_API_NEGOTIATE, sxe_mbx_api_set_handler},
	[SXE_VFREQ_RING_INFO_GET] = {SXE_VFREQ_RING_INFO_GET, sxe_pf_ring_info_get},
	[SXE_VFREQ_REDIR_TBL_GET] = {SXE_VFREQ_REDIR_TBL_GET, sxe_pf_rss_redir_tbl_get},
	[SXE_VFREQ_RSS_KEY_GET] = {SXE_VFREQ_RSS_KEY_GET, sxe_pf_rss_key_get},
	[SXE_VFREQ_CAST_MODE_SET] = {SXE_VFREQ_CAST_MODE_SET, sxe_vf_cast_mode_handler},
	[SXE_VFREQ_LINK_ENABLE_GET] = {SXE_VFREQ_LINK_ENABLE_GET, sxe_vf_link_enable_get},

#ifdef SXE_IPSEC_CONFIGURE
	[SXE_VFREQ_IPSEC_ADD] = {SXE_VFREQ_IPSEC_ADD, sxe_vf_ipsec_add},
	[SXE_VFREQ_IPSEC_DEL] = {SXE_VFREQ_IPSEC_DEL, sxe_vf_ipsec_del},
#endif

	[SXE_VFREQ_RSS_CONF_GET] = {SXE_VFREQ_RSS_CONF_GET, sxe_pf_rss_conf_get},
};

STATIC s32 sxe_req_msg_handle(struct sxe_adapter *adapter, u32 *msg,
					u8 vf_idx)
{
	struct sxe_hw *hw = &adapter->hw;
	s32 ret;
	u16 cmd_id = msg[0] & SXE_VFREQ_MASK;

	hw->setup.ops->regs_flush(hw);

#ifdef SXE_IPSEC_CONFIGURE
	if (cmd_id > SXE_VFREQ_IPSEC_DEL) {
#else
	if (cmd_id > SXE_VFREQ_LINK_ENABLE_GET &&
		cmd_id <= SXE_VFREQ_IPSEC_DEL) {
#endif
		ret = -SXE_ERR_PARAM;
		LOG_ERROR_BDF("vf_idx:%u msg:0x%x invalid cmd_id:0x%x.\n",
			   vf_idx, msg[0], cmd_id);
		goto l_out;
	}

	if (cmd_id == SXE_VFREQ_RESET) {
		ret = 0;
		sxe_vf_rst_msg_handle(adapter, vf_idx);
		goto l_out;
	}

	if (!adapter->vt_ctxt.vf_info[vf_idx].is_ready) {
		msg[0] |= SXE_MSGTYPE_NACK;
		ret = hw->mbx.ops->msg_send(hw, msg,
					SXE_MSG_NUM(sizeof(msg[0])), vf_idx);
		LOG_WARN_BDF("vf_idx:%d is_ready:0 send nack to vf.ret:%d.\n",
			  vf_idx, ret);
		goto l_out;
	}

	if (msg_table[cmd_id].msg_func) {
		ret = msg_table[cmd_id].msg_func(adapter, msg, vf_idx);
		LOG_INFO_BDF("msg:0x%x cmd_id:0x%x handle done.ret:%d\n",
			 msg[0], cmd_id, ret);
	} else {
		ret = -SXE_ERR_PARAM;
		LOG_ERROR_BDF("msg_type:0x%x cmdId:0x%x invalid.(err:%d)\n",
			  msg[0], cmd_id, ret);
	}
	if (!ret) {
		msg[0] |= SXE_MSGTYPE_ACK;
	} else {
		msg[0] |= SXE_MSGTYPE_NACK;
		LOG_INFO_BDF("vf:%d msg:0x%x no handler or handle fail.(ret:%d)\n",
			 vf_idx, msg[0], ret);
	}

	ret = hw->mbx.ops->msg_send(hw, msg, SXE_MBX_MSG_NUM, vf_idx);
	if (ret) {
		LOG_ERROR_BDF("vf:%d msg:0x%x reply fail.(err:%d).\n",
			   vf_idx, msg[0], ret);
	}

	LOG_INFO_BDF("pf reply vf:%d msg:0x%x done.ret:%d\n", vf_idx, msg[0], ret);

l_out:
	return ret;
}

s32 sxe_vf_req_task_handle(struct sxe_adapter *adapter, u8 vf_idx)
{
	u32 msg[SXE_MBX_MSG_NUM] = {0};
	s32 ret;

	ret = adapter->hw.mbx.ops->msg_rcv(&adapter->hw, msg, SXE_MBX_MSG_NUM, vf_idx);
	if (ret) {
		LOG_DEV_ERR("vf_idx:%d rcv vf req msg:0x%x fail.(err:%d)\n",
			   vf_idx, msg[0], ret);
		goto l_out;
	}

	LOG_INFO_BDF("rcv vf_idx:%d req msg:0x%x.\n", vf_idx, msg[0]);

	if (msg[0] & (SXE_MSGTYPE_ACK | SXE_MSGTYPE_NACK)) {
		LOG_WARN_BDF("msg:0x%x has handled, no need dup handle.\n",
			  msg[0]);
		goto l_out;
	}

	ret = sxe_req_msg_handle(adapter, msg, vf_idx);
	if (ret) {
		LOG_ERROR_BDF("vf:%d request msg handle fail.(err:%d)\n",
			  vf_idx, ret);
	}

l_out:
	return ret;
}

void sxe_vf_ack_task_handle(struct sxe_adapter *adapter, u8 vf_idx)
{
	u32 msg = SXE_MSGTYPE_NACK;

	if (!adapter->vt_ctxt.vf_info[vf_idx].is_ready) {
		adapter->hw.mbx.ops->msg_send(&adapter->hw, &msg,
					SXE_MSG_NUM(sizeof(msg)), vf_idx);
	}

	return;
}

s32 sxe_set_vf_spoofchk(struct net_device *dev, s32 vf_idx, bool status)
{
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct sxe_hw *hw = &adapter->hw;
	s32 ret = 0;

	if (vf_idx >= adapter->vt_ctxt.num_vfs) {
		ret = -EINVAL;
		LOG_ERROR_BDF("vf_idx:%d exceed vf nums:%d.(err:%d)\n",
			  vf_idx, adapter->vt_ctxt.num_vfs, ret);
		goto l_end;
	}

	adapter->vt_ctxt.vf_info[vf_idx].spoof_chk_enabled = status;

	hw->dma.ops->pool_mac_anti_spoof_set(hw, vf_idx, status);
	hw->dma.ops->pool_vlan_anti_spoof_set(hw, vf_idx, status);

	LOG_INFO_BDF("vf:%u spoof check:%s.\n",
		     vf_idx, status ? "on" : "off");
l_end:
	return ret;
}

s32 sxe_set_vf_trust(struct net_device *dev,
					s32 vf_idx, bool status)
{
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct sxe_vf_info *vf_info;
	s32 ret = 0;

	if (vf_idx >= adapter->vt_ctxt.num_vfs) {
		ret = -EINVAL;
		LOG_ERROR_BDF("vf_idx:%d exceed vf nums:%d.(err:%d)\n",
			  vf_idx, adapter->vt_ctxt.num_vfs, ret);
		goto l_out;
	}

	vf_info = &adapter->vt_ctxt.vf_info[vf_idx];
	if (vf_info->trusted == status) {
		LOG_INFO_BDF("current vf:%d trust status:%d, skip dup set.\n",
			 vf_idx, status);
		goto l_out;
	}

	vf_info->trusted = status;
	vf_info->is_ready = false;

	sxe_vf_trust_update_notify(adapter, vf_idx);

	LOG_MSG_INFO(drv, "vf:%u trust:%s.\n",
		     vf_idx, status ? "on" : "off");

l_out:
	return ret;
}

s32 sxe_set_vf_rss_query_en(struct net_device *dev, s32 vf_idx, bool status)
{
	struct sxe_adapter *adapter = netdev_priv(dev);
	s32 ret = 0;

	if (vf_idx >= adapter->vt_ctxt.num_vfs) {
		ret = -EINVAL;
		LOG_ERROR_BDF("vf_idx:%d exceed vf nums:%d.(err:%d)\n",
			  vf_idx, adapter->vt_ctxt.num_vfs, ret);
		goto l_end;
	}

	adapter->vt_ctxt.vf_info[vf_idx].rss_query_enabled = status;

	LOG_INFO_BDF("vf:%u query_rss:%s.\n",
		     vf_idx, status ? "on" : "off");

l_end:
	return ret;
}

s32 sxe_get_vf_config(struct net_device *dev, s32 vf_idx,
				struct ifla_vf_info *info)
{
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct sxe_vf_info *vf_info;
	s32 ret = 0;

	if (vf_idx >= adapter->vt_ctxt.num_vfs) {
		ret = -EINVAL;
		LOG_ERROR_BDF("vf_idx:%d exceed vf nums:%d.(err:%d)\n",
			  vf_idx, adapter->vt_ctxt.num_vfs, ret);
		goto l_end;
	}

	vf_info = &adapter->vt_ctxt.vf_info[vf_idx];

	ether_addr_copy(info->mac, vf_info->mac_addr);
	info->vf = vf_idx;
	info->min_tx_rate = 0;
	info->max_tx_rate = vf_info->tx_rate;
	info->vlan = vf_info->pf_vlan;
	info->qos = vf_info->pf_qos;
	info->spoofchk = vf_info->spoof_chk_enabled;
	info->rss_query_en = vf_info->rss_query_enabled;
	info->trusted = vf_info->trusted;
#ifdef HAVE_NDO_SET_VF_LINK_STATE
	info->linkstate = vf_info->link_state;
#endif

	LOG_INFO_BDF("vf_idx:%d get config info pf_vlan:%d pf_qos:%d spoof_chk:%d "
		 "rss_query_en:%d trusted:%d.\n",
		 vf_idx, vf_info->pf_vlan, vf_info->pf_qos,
		 vf_info->spoof_chk_enabled,
		 vf_info->rss_query_enabled, vf_info->trusted);

l_end:
	return ret;
}

#ifdef HAVE_NDO_SET_VF_LINK_STATE
void sxe_set_vf_link_enable(struct sxe_adapter *adapter, s32 vf_idx, s32 state)
{
	u32 msg;
	unsigned long flags;
	struct sxe_vf_info *vfinfo = &(adapter->vt_ctxt.vf_info[vf_idx]);
	vfinfo->link_state = state;	

	switch (state) {
	case IFLA_VF_LINK_STATE_AUTO:
		if (test_bit(SXE_DOWN, &adapter->state))
			vfinfo->link_enable = false;
		else
			vfinfo->link_enable = true;
		break;
	case IFLA_VF_LINK_STATE_ENABLE:
		vfinfo->link_enable = true;
		break;
	case IFLA_VF_LINK_STATE_DISABLE:
		vfinfo->link_enable = false;
		break;
	}

	sxe_vf_rxtx_set(adapter, vf_idx);

	vfinfo->is_ready = false;

	spin_lock_irqsave(&adapter->vt_ctxt.vfs_lock, flags);
	msg = SXE_CTRL_MSG_REINIT;
	sxe_mbx_msg_send(adapter, vf_idx, &msg, SXE_MSG_NUM(sizeof(msg)));
	spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);
}

s32 sxe_set_vf_link_state(struct net_device *netdev, s32 vf_idx, s32 state)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	s32 ret = 0;

	if (vf_idx < 0 || vf_idx >= adapter->vt_ctxt.num_vfs) {
		LOG_DEV_ERR("invalid vf idx: %d\n", vf_idx);
		ret = -EINVAL;
		goto out;
	}

	switch (state) {
	case IFLA_VF_LINK_STATE_ENABLE:
		LOG_DEV_INFO("set VF %d link state %d - not supported\n",
			vf_idx, state);
		break;
	case IFLA_VF_LINK_STATE_DISABLE:
		LOG_DEV_INFO("set VF %d link state disable\n", vf_idx);
		sxe_set_vf_link_enable(adapter, vf_idx, state);
		break;
	case IFLA_VF_LINK_STATE_AUTO:
		LOG_DEV_INFO("set VF %d link state auto\n", vf_idx);
		sxe_set_vf_link_enable(adapter, vf_idx, state);
		break;
	default:
		LOG_DEV_ERR("set VF %d - invalid link state %d\n", vf_idx, state);
		ret = -EINVAL;
	}
out:
	return ret;
}

void sxe_vf_enable_and_reinit_notify_vf_all(struct sxe_adapter *adapter)
{
	u32 i;
	struct sxe_vf_info *vfinfo = adapter->vt_ctxt.vf_info;

	for(i = 0; i < adapter->vt_ctxt.num_vfs; i++) {
		sxe_set_vf_link_enable(adapter, i, vfinfo[i].link_state);
	}
}
#endif 

static void sxe_pcie_vt_mode_set(struct sxe_adapter *adapter)
{
	u16 pool_mask = sxe_pool_mask_get(adapter);
	struct sxe_hw *hw = &adapter->hw;
	u32 value;

	if (pool_mask == SXE_8Q_PER_POOL_MASK) {
		value = SXE_GCR_EXT_VT_MODE_16;
	} else if (pool_mask == SXE_4Q_PER_POOL_MASK) {
		value = SXE_GCR_EXT_VT_MODE_32;
	} else {
		value = SXE_GCR_EXT_VT_MODE_64;
	}

	hw->pcie.ops->vt_mode_set(hw, value);

	return;
}

void sxe_vt1_configure(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	u16 pf_pool_num = adapter->pool_f.pf_num_used;
	u8 pf_pool_idx = PF_POOL_INDEX(0);
	u8 vf_reg_index = pf_pool_idx / VF_BLOCK_BITS;
	u8 vf_bit_index = pf_pool_idx % VF_BLOCK_BITS;
	u8 i;
	unsigned long flags;

	if (!(adapter->cap & SXE_SRIOV_ENABLE)) {
		LOG_INFO_BDF("cap:0x%x sriov disabled no need configure vt.\n",
			 adapter->cap);
		goto l_end;
	}

	hw->filter.mac.ops->vt_ctrl_configure(hw, pf_pool_idx);

	while (pf_pool_num--) {
		hw->filter.mac.ops->pool_rx_mode_set(hw, SXE_VMOLR_AUPE,
					  PF_POOL_INDEX(pf_pool_num));
	}

	hw->dma.ops->rx_pool_set(hw, vf_reg_index, GENMASK(31, vf_bit_index));
	hw->dma.ops->rx_pool_set(hw, (vf_reg_index ^ 1), (vf_reg_index - 1));

	hw->dma.ops->tx_pool_set(hw, vf_reg_index, GENMASK(31, vf_bit_index));
	hw->dma.ops->tx_pool_set(hw, (vf_reg_index ^ 1), (vf_reg_index - 1));

	if (adapter->vt_ctxt.bridge_mode == BRIDGE_MODE_VEB) {
		hw->dma.ops->vt_pool_loopback_switch(hw, true);
	}

	hw->filter.mac.ops->uc_addr_pool_enable(hw, 0, pf_pool_idx);

	sxe_pcie_vt_mode_set(adapter);

	adapter->cap &= ~SXE_VLAN_PROMISC;

	spin_lock_irqsave(&adapter->vt_ctxt.vfs_lock, flags);
	for (i = 0; i < adapter->vt_ctxt.num_vfs; i++) {
		sxe_set_vf_spoofchk(adapter->netdev, i,
				      adapter->vt_ctxt.vf_info[i].spoof_chk_enabled);
		sxe_set_vf_rss_query_en(adapter->netdev, i,
				     adapter->vt_ctxt.vf_info[i].rss_query_enabled);
	}
	spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);

l_end:
	return;
}

bool sxe_vf_tx_pending(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	u8 pool_mask = sxe_pool_mask_get(adapter);
	u8 ring_per_pool = __ALIGN_MASK(1, ~pool_mask);
	u8 vf_idx;
	u8 ring_idx;
	u32 head;
	u32 tail;
	bool ret = false;
	unsigned long flags;

	spin_lock_irqsave(&adapter->vt_ctxt.vfs_lock, flags);
	if (!adapter->vt_ctxt.num_vfs) {
		goto l_out;
	}

	for (vf_idx = 0; vf_idx < adapter->vt_ctxt.num_vfs; vf_idx++) {
		for (ring_idx = 0; ring_idx < ring_per_pool; ring_idx++) {
			hw->dma.ops->tx_ring_info_get(hw,
				vf_idx * ring_per_pool + ring_idx, &head, &tail);

			if (head != tail) {
				LOG_DEV_INFO("vf:%u ring_per_pool:%u "
					 "ring_idx:%u head:%u tail:%u "
					 "has pending data.\n",
					  vf_idx, ring_per_pool,
					  ring_idx, head, tail);
				ret = true;
				goto l_out;
			}
		}
	}

l_out:
	spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);
	return ret;
}

void sxe_vf_disable(struct sxe_adapter *adapter)
{
	u16 rss;

#ifdef HAVE_MACVLAN_OFFLOAD_SUPPORT
	struct sxe_pool_feature *pool = &adapter->pool_f;
	if (bitmap_weight(adapter->vt_ctxt.pf_pool_bitmap, pool->pf_num_used) == 1) {
		rss = min_t(u16, SXE_RSS_RING_NUM_MAX, num_online_cpus());
		adapter->cap &= ~SXE_SRIOV_ENABLE;
		adapter->cap &= ~SXE_MACVLAN_ENABLE;
	} else {
		rss = min_t(u16, MIN_QUEUES_IN_SRIOV, num_online_cpus());
	}
#else
	rss = min_t(u16, SXE_RSS_RING_NUM_MAX, num_online_cpus());
	adapter->cap &= ~SXE_SRIOV_ENABLE;
#endif
	adapter->pool_f.vf_num_used = 0;
	adapter->ring_f.rss_limit = rss;

	adapter->dcb_ctxt.cee_cfg.num_tcs.pg_tcs = SXE_DCB_8_TC;
	adapter->dcb_ctxt.cee_cfg.num_tcs.pfc_tcs = SXE_DCB_8_TC;

	LOG_INFO_BDF("vf disable update rss_limit to %u cap:0x%x.\n",
		     adapter->ring_f.rss_limit, adapter->cap);
	msleep(SXE_VF_DISABLE_WAIT);

	return;
}

#ifdef CONFIG_PCI_IOV

void sxe_vf_resource_release(struct sxe_adapter *adapter)
{
	u8 vf_idx = 0;
	u8 vf_num = adapter->vt_ctxt.num_vfs;
	unsigned long flags;


	spin_lock_irqsave(&adapter->vt_ctxt.vfs_lock, flags);
	adapter->vt_ctxt.num_vfs = 0;
	spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);

	smp_wmb();

	if (vf_num) {
		struct sxe_vf_info *vf_info = adapter->vt_ctxt.vf_info;
		struct pci_dev *vf_dev = vf_info->vf_dev;

		for (vf_idx = 0; vf_idx < vf_num; vf_idx++) {
			vf_dev = vf_info[vf_idx].vf_dev;
			if (!vf_dev) {
				LOG_WARN_BDF("vf:%u vf pci dev null.\n", vf_idx);
				continue;
			}

			vf_info[vf_idx].vf_dev = NULL;
			pci_dev_put(vf_dev);
		}

		SXE_KFREE(vf_info);
		SXE_KFREE(adapter->vt_ctxt.vf_uc_list);
	}

	LOG_INFO_BDF("%u vf resouce released.\n", vf_num);

	return;
}

void sxe_vf_exit(struct sxe_adapter *adapter)
{
	rtnl_lock();
	sxe_vf_resource_release(adapter);
	sxe_vf_disable(adapter);
	rtnl_unlock();

	pci_disable_sriov(adapter->pdev);

	return;
}

static inline void sxe_vf_mac_addr_init(struct sxe_adapter *adapter,
					   u8 num_vfs)
{
	u8  i; 

	for (i = 0; i < num_vfs; i++) {
		eth_zero_addr(adapter->vt_ctxt.vf_info[i].mac_addr);
	}

	return;
}

static void sxe_vf_uc_addr_list_init(struct sxe_adapter *adapter, u8 num_vfs)
{
	u8 vf_uc_num = SXE_UC_ENTRY_NUM_MAX - (SXE_MAX_MACVLANS + 1 + num_vfs);
	struct sxe_vf_uc_addr_list *mac_list;
	struct sxe_virtual_context *vf = &adapter->vt_ctxt;
	u8 i;

	if (!vf_uc_num) {
		LOG_WARN_BDF("num_vfs:%d has no available rar.\n", num_vfs);
		goto l_end;
	}

	mac_list = kcalloc(vf_uc_num, sizeof(struct sxe_vf_uc_addr_list), GFP_KERNEL);
	if (mac_list) {
		INIT_LIST_HEAD(&vf->head.list);
		for (i = 0; i < vf_uc_num; i++) {
			mac_list[i].vf_idx = -1;
			mac_list[i].free = true;
			list_add(&mac_list[i].list, &(vf->head.list));
		}

		vf->vf_uc_list = mac_list;

		LOG_INFO_BDF("vf uc mac addr list mem cnt:%u num_vfs:%u.\n",
			  vf_uc_num, num_vfs);
	}

l_end:
	return;
}

static s32 sxe_vf_info_init(struct sxe_adapter *adapter, u8 num_vfs)
{
	u8 i;
	s32 ret;

	adapter->vt_ctxt.vf_info = kcalloc(num_vfs, sizeof(struct sxe_vf_info),
					   GFP_KERNEL);
	if (!adapter->vt_ctxt.vf_info) {
		ret = -ENOMEM;
		LOG_ERROR_BDF("num_vfs:%d alloc size:%zuB fail.(err:%d)\n",
			   num_vfs, num_vfs * sizeof(struct sxe_vf_info), ret);
		goto l_out;
	}

	for (i = 0; i < num_vfs; i++) {
		adapter->vt_ctxt.vf_info[i].trusted = false;
		adapter->vt_ctxt.vf_info[i].cast_mode = SXE_CAST_MODE_NONE;
		adapter->vt_ctxt.vf_info[i].spoof_chk_enabled = true;
		adapter->vt_ctxt.vf_info[i].link_enable = true;
		adapter->vt_ctxt.vf_info[i].rss_query_enabled = false;
	}

	ret = 0;

	LOG_INFO_BDF("num_vfs:%u vf_info:0x%pK.\n", num_vfs, adapter->vt_ctxt.vf_info);

l_out:
	return ret;
}

static s32 sxe_vf_init(struct sxe_adapter *adapter, u8 num_vfs)
{
	struct sxe_hw *hw = &adapter->hw;
	s32 ret = 0;

	ret = sxe_vf_info_init(adapter, num_vfs);
	if (ret) {
		LOG_ERROR_BDF("num_vfs:%u vf info alloc memory fail.(err:%d)\n",
			  num_vfs, ret);
		goto l_out;
	}

	adapter->vt_ctxt.bridge_mode = BRIDGE_MODE_VEB;
	hw->dma.ops->vt_pool_loopback_switch(hw, true);

	if (num_vfs < SXE_VF_NUM_16) {
		adapter->dcb_ctxt.cee_cfg.num_tcs.pg_tcs = SXE_DCB_8_TC;
		adapter->dcb_ctxt.cee_cfg.num_tcs.pfc_tcs = SXE_DCB_8_TC;
	} else if (num_vfs < SXE_VF_NUM_32) {
		adapter->dcb_ctxt.cee_cfg.num_tcs.pg_tcs = SXE_DCB_4_TC;
		adapter->dcb_ctxt.cee_cfg.num_tcs.pfc_tcs = SXE_DCB_4_TC;
	} else {
		adapter->dcb_ctxt.cee_cfg.num_tcs.pg_tcs = SXE_DCB_1_TC;
		adapter->dcb_ctxt.cee_cfg.num_tcs.pfc_tcs = SXE_DCB_1_TC;
	}

	sxe_vf_uc_addr_list_init(adapter, num_vfs);

	sxe_vf_mac_addr_init(adapter, num_vfs);

	adapter->vt_ctxt.num_vfs = num_vfs;
	adapter->pool_f.vf_num_used = num_vfs;

#ifdef HAVE_MACVLAN_OFFLOAD_SUPPORT
	adapter->cap |= SXE_SRIOV_ENABLE |
			SXE_MACVLAN_ENABLE;
#else
	adapter->cap |= SXE_SRIOV_ENABLE;
#endif

	adapter->cap &= ~(SXE_LRO_CAPABLE |
			   SXE_LRO_ENABLE);

	LOG_MSG_INFO(probe, "iov is enabled with %d vfs\n", num_vfs);

l_out:
	return ret;
}

void sxe_vf_down(struct sxe_adapter *adapter)
{
	struct sxe_virtual_context *vt_ctxt = &adapter->vt_ctxt;
	struct sxe_hw *hw = &adapter->hw;
	u8 i;
	unsigned long flags;

	spin_lock_irqsave(&adapter->vt_ctxt.vfs_lock, flags);
	if (vt_ctxt->num_vfs == 0) {
		LOG_INFO_BDF("vf num:%d no need down.\n", vt_ctxt->num_vfs);
		spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);
		goto l_out;
	}

	hw->irq.ops->set_eitrsel(hw, 0);
	for (i = 0; i < vt_ctxt->num_vfs; i++) {
		vt_ctxt->vf_info[i].is_ready = false;
	}

	spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);

	sxe_netdev_down_notify_vf_all(adapter);

	hw->dma.ops->tx_pool_set(hw, 0, 0);
	hw->dma.ops->tx_pool_set(hw, 1, 0);
	hw->dma.ops->rx_pool_set(hw, 0, 0);
	hw->dma.ops->rx_pool_set(hw, 1, 0);

l_out:
	return;
}

void sxe_sriov_init(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;

	hw->mbx.ops->init(hw);

	spin_lock_init(&adapter->vt_ctxt.vfs_lock);

	pci_sriov_set_totalvfs(adapter->pdev, SXE_VF_DRV_MAX);

	if (max_vfs > 0) {
		LOG_DEV_WARN("max_vfs module parameter is deprecated - "
			     "please use the pci sysfs interface instead.\n");
		if (max_vfs > SXE_VF_DRV_MAX) {
			LOG_DEV_WARN("max_vfs parameter invalid:%u, no assigne vfs\n",
				 max_vfs);
			max_vfs = 0;
		}
	}

	sxe_param_sriov_enable(adapter, max_vfs);

	return;
}

static void sxe_vf_dev_info_get(struct sxe_adapter *adapter)
{
	struct pci_dev *pf_dev = adapter->pdev;
	struct pci_dev *vf_dev;
	u32 sriov_cap;
	u16 vf_id;
	u8 vf_idx = 0;

	sriov_cap = pci_find_ext_capability(pf_dev, PCI_EXT_CAP_ID_SRIOV);
	if (!sriov_cap) {
		LOG_WARN_BDF("sriov capability not found.\n");
		goto l_out;
	}

	pci_read_config_word(pf_dev, sriov_cap + PCI_SRIOV_VF_DID, &vf_id);

	vf_dev = pci_get_device(pf_dev->vendor, vf_id, NULL);
	LOG_INFO_BDF("vf dev id:0x%x pci dev:0x%pK.\n", vf_id, vf_dev);

	while (vf_dev) {
		if (vf_dev->is_virtfn &&
		    (vf_dev->physfn == pf_dev) &&
		    (vf_idx < adapter->vt_ctxt.num_vfs)) {
			pci_dev_get(vf_dev);
			adapter->vt_ctxt.vf_info[vf_idx].vf_dev = vf_dev;
			vf_idx++;
		}

		vf_dev = pci_get_device(pf_dev->vendor, vf_id, vf_dev);
	}

l_out:
	return;
}

static void sxe_sriov_ring_reinit(struct sxe_adapter *adapter)
{
	sxe_ring_reassign(adapter, sxe_dcb_tc_get(adapter));

	return;
}

static s32 sxe_sriov_enable_prepare(struct sxe_adapter *adapter,
						u8 num_vfs)
{
	u8 pools_used;
	u8 tc;
	u8 max;
	s32 ret = 0;
	struct sxe_pool_feature *pool = &adapter->pool_f;

	if (adapter->xdp_prog) {
		ret = -EINVAL;
		LOG_MSG_ERR(probe, "num_vfs:%d sriov not support xdp.\n", num_vfs);
		goto l_out;
	}

	tc = sxe_dcb_tc_get(adapter);
	pools_used = bitmap_weight(adapter->vt_ctxt.pf_pool_bitmap, pool->pf_num_used);

	max = (tc > 4) ? SXE_MAX_VFS_8TC :
		(tc > 1) ? SXE_MAX_VFS_4TC : SXE_MAX_VFS_1TC;

	if (num_vfs > (max - pools_used)) {
		LOG_DEV_ERR("tc:%d pool_used:%d num_vfs:%d exceed max vfs:%d\n",
			  tc, pools_used, num_vfs, max);
		ret = -EPERM;
		goto l_out;
	}

l_out:
	return ret;
}

static s32 sxe_sriov_disable(struct pci_dev *pdev)
{
	struct sxe_adapter *adapter = pci_get_drvdata(pdev);
	u32 assigned = pci_vfs_assigned(pdev);
	u32 cap = adapter->cap;
	u8  vf = pci_num_vf(adapter->pdev);
	s32 ret = 0;

	if (!(adapter->cap & SXE_SRIOV_ENABLE)) {
		LOG_INFO_BDF("vf:%d sriov has been disabled.\n", pci_num_vf(pdev));
		goto l_out;
	}

	if (assigned) {
		ret = -EPERM;
		LOG_DEV_ERR("%d vf assigned to guest, can't disable sriov.(err:%d)\n",
			   assigned, ret);
		goto l_out;
	}

	pci_disable_sriov(adapter->pdev);


	rtnl_lock();

	sxe_vf_resource_release(adapter);

	sxe_vf_disable(adapter);

	if ((cap != adapter->cap) ||
	    (vf != pci_num_vf(adapter->pdev))) {
		LOG_INFO_BDF("previous cap:0x%x vf:%d, now cap:0x%x vf:%d changed.\n",
			cap, vf, adapter->cap, pci_num_vf(adapter->pdev));
		sxe_sriov_ring_reinit(adapter);
	}

	rtnl_unlock();

l_out:
	return ret;
}

static s32 sxe_sriov_enable(struct pci_dev *pdev, u8 num_vfs)
{
	struct sxe_adapter *adapter = pci_get_drvdata(pdev);
	u8 vf_current = pci_num_vf(adapter->pdev);
	s32 ret;

	if (vf_current == num_vfs) {
		ret = num_vfs;
		LOG_INFO_BDF("existed %u vfs, skip dup create.\n", num_vfs);
		goto l_out;
	}

	if (vf_current) {
		sxe_vf_exit(adapter);
	}

	rtnl_lock();

	ret = sxe_sriov_enable_prepare(adapter, num_vfs);
	if (ret) {
		LOG_ERROR_BDF("num_vfs:%d prepare fail.(err:%d)", num_vfs, ret);
		goto l_unlock;
	}

	ret = sxe_vf_init(adapter, num_vfs);
	if (ret) {
		LOG_ERROR_BDF("sxe vf init fail.(err:%d)\n", ret);
		goto l_unlock;
	}

	sxe_sriov_ring_reinit(adapter);

	rtnl_unlock();

	if (adapter->cap & SXE_SRIOV_ENABLE) {
		ret = pci_enable_sriov(pdev, num_vfs);
		if (ret) {
			LOG_MSG_ERR(probe, "num_vfs:%d enable pci sriov fail.(err:%d)\n",
				  num_vfs, ret);
			goto l_vf_free;
		}

		sxe_vf_dev_info_get(adapter);

		LOG_INFO_BDF("cap:0x%x cap2:0x%x num_vfs:%d enable sriov success.\n",
			 adapter->cap, adapter->cap2, num_vfs);

		ret = num_vfs;
	} else {
		ret = -EPERM;
		LOG_ERROR_BDF("num_vfs:%d driver sriov is disabled, "
			  "can't enable sriov.(err:%d)\n",
			  num_vfs, ret);
		goto l_vf_free;
	}

	return ret;

l_unlock:
	rtnl_unlock();

l_out:
	return ret;

l_vf_free:
	sxe_vf_exit(adapter);
	return ret;
}

#ifdef HAVE_NO_PCIE_FLR
static inline void sxe_issue_vf_flr(struct sxe_adapter *adapter,
				      struct pci_dev *vf_dev)
{
	int pos, i;
	u16 status;

	for (i = 0; i < 4; i++) {
		if (i)
			msleep((1 << (i - 1)) * 100);

		pcie_capability_read_word(vf_dev, PCI_EXP_DEVSTA, &status);
		if (!(status & PCI_EXP_DEVSTA_TRPND))
			goto clear;
	}

	LOG_DEV_WARN("Issuing VFLR with pending transactions\n");

clear:
	pos = pci_find_capability(vf_dev, PCI_CAP_ID_EXP);
	if (!pos)
		return;

	LOG_DEV_ERR("Issuing VFLR for VF %s\n", pci_name(vf_dev));
	pci_write_config_word(vf_dev, pos + PCI_EXP_DEVCTL,
			      PCI_EXP_DEVCTL_BCR_FLR);
	msleep(100);
}
#endif

void sxe_bad_vf_flr(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	struct pci_dev *pdev = adapter->pdev;
	u32 num = hw->stat.ops->tx_packets_num_get(hw);
	unsigned long flags;
	u32 vf_idx;

	if (!netif_carrier_ok(adapter->netdev)) {
		LOG_DEBUG_BDF("no need check vf status.\n");
		goto l_out;
	}

	if (num) {
		LOG_DEBUG_BDF("no need vf flr tx good packets num:%u.\n", num);
		goto l_out;
	}

	if (!pdev) {
		goto l_out;
	}

	spin_lock_irqsave(&adapter->vt_ctxt.vfs_lock, flags);
	for (vf_idx = 0; vf_idx < adapter->vt_ctxt.num_vfs; vf_idx++) {
		struct pci_dev *vf_dev = adapter->vt_ctxt.vf_info[vf_idx].vf_dev;
		u16 status_reg;

		LOG_INFO_BDF("num_vfs:%u vf_idx:%u pci dev:0x%pK.\n",
			     adapter->vt_ctxt.num_vfs,
			     vf_idx, vf_dev);

		if (!vf_dev)
			continue;
		pci_read_config_word(vf_dev, PCI_STATUS, &status_reg);
		if (status_reg != SXE_READ_CFG_WORD_FAILED &&
		    status_reg & PCI_STATUS_REC_MASTER_ABORT) {
#ifdef HAVE_NO_PCIE_FLR
			sxe_issue_vf_flr(adapter, vf_dev);
#else
			pcie_flr(vf_dev);
#endif
			LOG_WARN_BDF("vf_idx:%u status_reg:0x%x pcie flr.\n",
				vf_idx, status_reg);
		}
	}
	spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);

l_out:
	return;
}

void sxe_spoof_packets_check(struct sxe_adapter *adapter)
{
	u32 num;
	struct sxe_hw *hw = &adapter->hw;

	if (adapter->vt_ctxt.num_vfs == 0) {
		goto l_out;
	}

	num = hw->stat.ops->unsecurity_packets_num_get(hw);
	if (!num) {
		goto l_out;
	}

	LOG_MSG_WARN(drv, "%u spoof packets detected.\n", num);

l_out:
	return;
}

void sxe_param_sriov_enable(struct sxe_adapter *adapter, u8 user_num_vfs)
{
	u8 vf_current = pci_num_vf(adapter->pdev);
	s32 ret = 0;
	u8 num_vfs;

	if (vf_current) {
		num_vfs = vf_current;
		LOG_DEV_WARN("virtual functions already enabled for this device "
			 "- please reload all VF drivers to avoid spoofed packet errors\n");
	} else {
		num_vfs = user_num_vfs;
	}

	if (num_vfs == 0) {
		goto l_end;
	}

	if (!vf_current) {
		ret = pci_enable_sriov(adapter->pdev, num_vfs);
		if (ret) {
			LOG_MSG_ERR(probe, "enable %u vfs failed.(err:%d)\n",
				  num_vfs, ret);
			goto l_vf_exit;
		}
	}

	ret = sxe_vf_init(adapter, num_vfs);
	if (ret) {
		LOG_MSG_ERR(probe, "init %u vfs failed.(err:%d)\n", num_vfs, ret);
		goto l_end;
	}

	sxe_vf_dev_info_get(adapter);

	LOG_INFO_BDF("max_vfs:%u fact assign %u vfs.\n", max_vfs, num_vfs);

l_end:
	return;

l_vf_exit:
	sxe_vf_exit(adapter);
	return;
}

#else

static s32 sxe_sriov_enable(struct pci_dev *pdev, u8 num_vfs)
{
	return 0;
}

void sxe_vf_resource_release(struct sxe_adapter *adapter)
{
	return;
}

static s32 sxe_sriov_disable(struct pci_dev *pdev)
{
	struct sxe_adapter *adapter = pci_get_drvdata(pdev);
	s32 ret = 0;

	if (!(adapter->cap & SXE_SRIOV_ENABLE)) {
		LOG_INFO_BDF("vf:%d sriov has been disabled.\n", pci_num_vf(pdev));
		goto l_out;
	}

	rtnl_lock();
	sxe_vf_disable(adapter);
	rtnl_unlock();

l_out:
	return ret;
}

void sxe_sriov_init(struct sxe_adapter *adapter)
{
	return;
}

void sxe_vf_exit(struct sxe_adapter *adapter)
{
	return;
}

void sxe_vf_down(struct sxe_adapter *adapter)
{
	return;
}

void sxe_bad_vf_flr(struct sxe_adapter *adapter)
{
	return;
}

void sxe_spoof_packets_check(struct sxe_adapter *adapter)
{
	return;
}

#endif

s32 sxe_sriov_configure(struct pci_dev *pdev, s32 num_vfs)
{
	s32 ret;

	if (num_vfs) {
		ret = sxe_sriov_enable(pdev, num_vfs);
	} else {
		ret = sxe_sriov_disable(pdev);
	}

	LOG_INFO("%s num_vfs:%d sriov operation done.(ret:%d)\n",
		   dev_name(&pdev->dev), num_vfs, ret);

	return ret;
}

