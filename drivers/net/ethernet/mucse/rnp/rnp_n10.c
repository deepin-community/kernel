// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/sched.h>

#include "rnp.h"
#include "rnp_phy.h"
#include "rnp_mbx.h"
#include "rnp_mbx_fw.h"
#include "rnp_pcs.h"
#include "rnp_ethtool.h"
#include "rnp_sriov.h"

#define RNP_N400_MAX_VF 8
#define RNP_N400_RSS_TBL_NUM 128
#define RNP_N400_RSS_TC_TBL_NUM 8
#define RNP_N400_MAX_TX_QUEUES 8
#define RNP_N400_MAX_RX_QUEUES 8
#define RNP_N400_RAR_NCSI_RAR_ENTRIES 0
#define RNP_N10_MAX_VF 64
#define RNP_N10_RSS_TBL_NUM 128
#define RNP_N10_RSS_TC_TBL_NUM 8
#define RNP_N10_MAX_TX_QUEUES 128
#define RNP_N10_MAX_RX_QUEUES 128
#define RNP_N10_RAR_NCSI_RAR_ENTRIES 0

#if defined(NIC_VF_FXIED) || defined(VF_PROMISC_SUPPORT)
/* we use the last dmac to support vf promisc */
#define RNP_N10_RAR_ENTRIES (127 - RNP_N10_RAR_NCSI_RAR_ENTRIES)
#else
#define RNP_N10_RAR_ENTRIES (128 - RNP_N10_RAR_NCSI_RAR_ENTRIES)
#endif


#define RNP_N10_MC_TBL_SIZE 128
#define RNP_N10_VFT_TBL_SIZE 128
#define RNP_N10_RX_PB_SIZE 512
#ifndef RNP_N10_MSIX_VECTORS
#define RNP_N10_MSIX_VECTORS 64
#endif
#define RNP_N400_MSIX_VECTORS 17

#define RNP10_MAX_LAYER2_FILTERS 16
#define RNP10_MAX_TCAM_FILTERS 4096
#define RNP10_MAX_TUPLE5_FILTERS 128


/* setup queue speed limit to max_rate */
static void rnp_dma_set_tx_maxrate_n10(struct rnp_dma_info *dma, u16 queue,
				       u32 max_rate)
{
}

/* setup mac with vf_num to veb table */
static void rnp_dma_set_veb_mac_n10(struct rnp_dma_info *dma, u8 *mac,
				    u32 vfnum, u32 ring)
{
	u32 maclow, machi, ring_vfnum;
	int port;

	maclow = (mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];
	machi = (mac[0] << 8) | mac[1];
	ring_vfnum = ring | ((0x80 | vfnum) << 8);
	for (port = 0; port < 4; port++) {
		dma_wr32(dma, RNP10_DMA_PORT_VBE_MAC_LO_TBL(port, vfnum),
			 maclow);
		dma_wr32(dma, RNP10_DMA_PORT_VBE_MAC_HI_TBL(port, vfnum),
			 machi);
		dma_wr32(dma, RNP10_DMA_PORT_VEB_VF_RING_TBL(port, vfnum),
			 ring_vfnum);
	}
}

/* setup vlan with vf_num to veb table */
static void rnp_dma_set_veb_vlan_n10(struct rnp_dma_info *dma, u16 vlan,
				     u32 vfnum)
{
	int port;

	/* each vf can support only one vlan */
	for (port = 0; port < 4; port++)
		dma_wr32(dma, RNP10_DMA_PORT_VEB_VID_TBL(port, vfnum), vlan);
}

static void rnp_dma_clr_veb_all_n10(struct rnp_dma_info *dma)
{
	int port, i;

	for (port = 0; port < 4; port++) {
		for (i = 0; i < VEB_TBL_CNTS; i++) {
			dma_wr32(dma, RNP_DMA_PORT_VBE_MAC_LO_TBL(port, i), 0);
			dma_wr32(dma, RNP_DMA_PORT_VBE_MAC_HI_TBL(port, i), 0);
			dma_wr32(dma, RNP_DMA_PORT_VEB_VID_TBL(port, i), 0);
			dma_wr32(dma, RNP_DMA_PORT_VEB_VF_RING_TBL(port, i), 0);
		}
	}
}

static struct rnp_dma_operations dma_ops_n10 = {
	.set_tx_maxrate = &rnp_dma_set_tx_maxrate_n10,
	.set_veb_mac = &rnp_dma_set_veb_mac_n10,
	.set_veb_vlan = &rnp_dma_set_veb_vlan_n10,
	.clr_veb_all = &rnp_dma_clr_veb_all_n10,

};

/**
 *  rnp_eth_set_rar_n10 - Set Rx address register
 *  @eth: pointer to eth structure
 *  @index: Receive address register to write
 *  @addr: Address to put into receive address register
 *  @vmdq: VMDq "set" or "pool" index
 *  @enable_addr: set flag that address is active
 *  @sriov_flag
 *
 *  Puts an ethernet address into a receive address register.
 **/
static s32 rnp_eth_set_rar_n10(struct rnp_eth_info *eth, u32 index, u8 *addr,
			       bool enable_addr)
{
	u32 mcstctrl;
	u32 rar_low, rar_high = 0;
	u32 rar_entries = eth->num_rar_entries;
	struct rnp_hw *hw = (struct rnp_hw *)eth->back;

	/* Make sure we are using a valid rar index range */
	if (index >= (rar_entries + hw->ncsi_rar_entries)) {
		rnp_err("RAR index %d is out of range.\n", index);
		return RNP_ERR_INVALID_ARGUMENT;
	}

	eth_dbg(eth, "    RAR[%d] <= %pM.  vmdq:%d enable:0x%x\n", index, addr);

	/*
	 * HW expects these in big endian so we reverse the byte
	 * order from network order (big endian) to little endian
	 */
	rar_low = ((u32)addr[5] | ((u32)addr[4] << 8) | ((u32)addr[3] << 16) |
		   ((u32)addr[2] << 24));
	/*
	 * Some parts put the VMDq setting in the extra RAH bits,
	 * so save everything except the lower 16 bits that hold part
	 * of the address and the address valid bit.
	 */
	rar_high = eth_rd32(eth, RNP10_ETH_RAR_RH(index));
	rar_high &= ~(0x0000FFFF | RNP10_RAH_AV);
	rar_high |= ((u32)addr[1] | ((u32)addr[0] << 8));

	if (enable_addr)
		rar_high |= RNP10_RAH_AV;

	eth_wr32(eth, RNP10_ETH_RAR_RL(index), rar_low);
	eth_wr32(eth, RNP10_ETH_RAR_RH(index), rar_high);

	/* open unicast filter */
	/* we now not use unicast */
	/* but we must open this since dest-mac filter | unicast table */
	/* all packets up if close unicast table */
	mcstctrl = eth_rd32(eth, RNP10_ETH_DMAC_MCSTCTRL);
	mcstctrl |= RNP10_MCSTCTRL_UNICASE_TBL_EN;
	eth_wr32(eth, RNP10_ETH_DMAC_MCSTCTRL, mcstctrl);

	return 0;
}

/**
 *  rnp_eth_clear_rar_n10 - Remove Rx address register
 *  @eth: pointer to eth structure
 *  @index: Receive address register to write
 *
 *  Clears an ethernet address from a receive address register.
 **/
static s32 rnp_eth_clear_rar_n10(struct rnp_eth_info *eth, u32 index)
{
	u32 rar_high;
	u32 rar_entries = eth->num_rar_entries;

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries) {
		eth_dbg(eth, "RAR index %d is out of range.\n", index);
		return RNP_ERR_INVALID_ARGUMENT;
	}

	/*
	 * Some parts put the VMDq setting in the extra RAH bits,
	 * so save everything except the lower 16 bits that hold part
	 * of the address and the address valid bit.
	 */
	rar_high = eth_rd32(eth, RNP10_ETH_RAR_RH(index));
	rar_high &= ~(0x0000FFFF | RNP10_RAH_AV);

	eth_wr32(eth, RNP10_ETH_RAR_RL(index), 0);
	eth_wr32(eth, RNP10_ETH_RAR_RH(index), rar_high);
	/* clear VMDq pool/queue selection for this RAR */
	eth->ops.clear_vmdq(eth, index, RNP_CLEAR_VMDQ_ALL);

	return 0;
}

/**
 *  rnp_eth_set_vmdq_n10 - Associate a VMDq pool index with a rx address
 *  @eth: pointer to eth struct
 *  @rar: receive address register index to associate with a VMDq index
 *  @vmdq: VMDq pool index
 *  only mac->vf
 **/
static s32 rnp_eth_set_vmdq_n10(struct rnp_eth_info *eth, u32 rar, u32 vmdq)
{
	u32 rar_entries = eth->num_rar_entries;
	struct rnp_hw *hw = (struct rnp_hw *)&eth->back;

	/* Make sure we are using a valid rar index range */
	if (rar >= rar_entries) {
		eth_dbg(eth, "RAR index %d is out of range.\n", rar);
		return RNP_ERR_INVALID_ARGUMENT;
	}
	/* n400 should use like this
	 * ----------
	 *       vf0 | vf1 | vf2
	 * n400  4   | 8   | 12
	 * n10   2   | 4   |  6
	 * n10(1)0   | 2   |  4
	 * not good here
	 */
	if (hw->hw_type == rnp_hw_n400)
		eth_wr32(eth, RNP10_VM_DMAC_MPSAR_RING(rar), vmdq * 2);
	else
		eth_wr32(eth, RNP10_VM_DMAC_MPSAR_RING(rar), vmdq);

	return 0;
}

/**
 *  rnp_eth_clear_vmdq_n10 - Disassociate a VMDq pool index from a rx address
 *  @eth: pointer to eth struct
 *  @rar: receive address register index to disassociate
 *  @vmdq: VMDq pool index to remove from the rar
 **/
static s32 rnp_eth_clear_vmdq_n10(struct rnp_eth_info *eth, u32 rar, u32 vmdq)
{
	u32 rar_entries = eth->num_rar_entries;

	/* Make sure we are using a valid rar index range */
	if (rar >= rar_entries) {
		eth_dbg(eth, "RAR index %d is out of range.\n", rar);
		return RNP_ERR_INVALID_ARGUMENT;
	}

	eth_wr32(eth, RNP10_VM_DMAC_MPSAR_RING(rar), 0);

	return 0;
}

static s32 rnp10_mta_vector(struct rnp_eth_info *eth, u8 *mc_addr)
{
	u32 vector = 0;

	switch (eth->mc_filter_type) {
	case 0: /* use bits [36:47] of the address */
		vector = ((mc_addr[4] << 8) | (((u16)mc_addr[5])));
		break;
	case 1: /* use bits [35:46] of the address */
		vector = ((mc_addr[4] << 7) | (((u16)mc_addr[5]) >> 1));
		break;
	case 2: /* use bits [34:45] of the address */
		vector = ((mc_addr[4] << 6) | (((u16)mc_addr[5]) >> 2));
		break;
	case 3: /* use bits [32:43] of the address */
		vector = ((mc_addr[4] << 5) | (((u16)mc_addr[5]) >> 3));
		break;
	default: /* Invalid mc_filter_type */
		hw_dbg(hw, "MC filter type param set incorrectly\n");
		break;
	}

	/* vector can only be 12-bits or boundary will be exceeded */
	vector &= 0xFFF;
	return vector;
}

static void rnp10_set_mta(struct rnp_hw *hw, u8 *mc_addr)
{
	u32 vector;
	u32 vector_bit;
	u32 vector_reg;
	struct rnp_eth_info *eth = &hw->eth;

	hw->addr_ctrl.mta_in_use++;

	vector = rnp10_mta_vector(eth, mc_addr);

	/*
	 * The MTA is a register array of 128 32-bit registers. It is treated
	 * like an array of 4096 bits.  We want to set bit
	 * BitArray[vector_value]. So we figure out what register the bit is
	 * in, read it, OR in the new bit, then write back the new value.  The
	 * register is determined by the upper 7 bits of the vector value and
	 * the bit within that register are determined by the lower 5 bits of
	 * the value.
	 */
	vector_reg = (vector >> 5) & 0x7F;
	vector_bit = vector & 0x1F;
	hw_dbg(hw, "\t\t%pM: MTA-BIT:%4d, MTA_REG[%d][%d] <= 1\n", mc_addr,
	       vector, vector_reg, vector_bit);
	eth->mta_shadow[vector_reg] |= (1 << vector_bit);
}

static void rnp10_set_vf_mta(struct rnp_hw *hw, u16 vector)
{
	u32 vector_bit;
	u32 vector_reg;
	struct rnp_eth_info *eth = &hw->eth;

	hw->addr_ctrl.mta_in_use++;

	vector_reg = (vector >> 5) & 0x7F;
	vector_bit = vector & 0x1F;
	hw_dbg(hw, "\t\t vf M: MTA-BIT:%4d, MTA_REG[%d][%d] <= 1\n", vector,
	       vector_reg, vector_bit);
	eth->mta_shadow[vector_reg] |= (1 << vector_bit);
}

static u8 *rnp_addr_list_itr(struct rnp_hw __maybe_unused *hw, u8 **mc_addr_ptr)
{
	struct netdev_hw_addr *mc_ptr;
	u8 *addr = *mc_addr_ptr;

	mc_ptr = container_of(addr, struct netdev_hw_addr, addr[0]);
	if (mc_ptr->list.next) {
		struct netdev_hw_addr *ha;

		ha = list_entry(mc_ptr->list.next, struct netdev_hw_addr, list);
		*mc_addr_ptr = ha->addr;
	} else
		*mc_addr_ptr = NULL;

	return addr;
}

/**
 *  rnp_update_mc_addr_list_n10 - Updates MAC list of multicast addresses
 *  @hw: pointer to hardware structure
 *  @netdev: pointer to net device structure
 *
 *  The given list replaces any existing list. Clears the MC addrs from receive
 *  address registers and the multicast table. Uses unused receive address
 *  registers for the first multicast addresses, and hashes the rest into the
 *  multicast table.
 **/
static s32 rnp_eth_update_mc_addr_list_n10(struct rnp_eth_info *eth,
					   struct net_device *netdev,
					   bool sriov_on)
{
	struct rnp_hw *hw = (struct rnp_hw *)eth->back;
	struct netdev_hw_addr *ha;
	u32 i;
	u32 v;
	int addr_count = 0;
	u8 *addr_list = NULL;

	/*
	 * Set the new number of MC addresses that we are being requested to
	 * use.
	 */
	hw->addr_ctrl.num_mc_addrs = netdev_mc_count(netdev);
	hw->addr_ctrl.mta_in_use = 0;

	/* Clear mta_shadow */
	eth_dbg(eth, " Clearing MTA(multicast table)\n");

	memset(&eth->mta_shadow, 0, sizeof(eth->mta_shadow));

	/* Update mta shadow */
	eth_dbg(eth, " Updating MTA..\n");

	addr_count = netdev_mc_count(netdev);

	ha = list_first_entry(&netdev->mc.list, struct netdev_hw_addr, list);
	addr_list = ha->addr;
	for (i = 0; i < addr_count; i++) {
		eth_dbg(eth, " Adding the multicast addresses:\n");
		rnp10_set_mta(hw, rnp_addr_list_itr(hw, &addr_list));
	}

	if (hw->ncsi_en) {
		eth->ops.ncsi_set_mc_mta(eth);
	}

	if (sriov_on) {
		struct rnp_adapter *adapter = (struct rnp_adapter *)hw->back;
		if (!test_and_set_bit(__RNP_USE_VFINFI, &adapter->state)) {
			for (i = 0; i < adapter->num_vfs; i++) {
				if (adapter->vfinfo) {
					struct vf_data_storage *vfinfo =
						&adapter->vfinfo[i];
					int j;

					for (j = 0;
					     j < vfinfo->num_vf_mc_hashes; j++)
						rnp10_set_vf_mta(
							hw,
							vfinfo->vf_mc_hashes[j]);
				}
			}
			clear_bit(__RNP_USE_VFINFI, &adapter->state);
		}
	}

	/* Enable mta */
	for (i = 0; i < hw->eth.mcft_size; i++) {
		if (hw->addr_ctrl.mta_in_use)
			eth_wr32(eth, RNP10_ETH_MULTICAST_HASH_TABLE(i),
				 eth->mta_shadow[i]);
	}

	if (hw->addr_ctrl.mta_in_use > 0) {
		struct rnp_adapter *adapter = (struct rnp_adapter *)hw->back;
		if (!(adapter->flags & RNP_FLAG_SWITCH_LOOPBACK_EN)) {
			v = eth_rd32(eth, RNP10_ETH_DMAC_MCSTCTRL);
			eth_wr32(eth, RNP10_ETH_DMAC_MCSTCTRL,
				 v | RNP10_MCSTCTRL_MULTICASE_TBL_EN |
					 eth->mc_filter_type);
		}
	}

	eth_dbg(eth, " update MTA Done. mta_in_use:%d\n",
		hw->addr_ctrl.mta_in_use);
	return hw->addr_ctrl.mta_in_use;
}

/* clean all mc addr */
static void rnp_eth_clr_mc_addr_n10(struct rnp_eth_info *eth)
{
	int i;

	for (i = 0; i < eth->mcft_size; i++)
		eth_wr32(eth, RNP10_ETH_MULTICAST_HASH_TABLE(i), 0);
}

/**
 *  rnp_eth_update_rss_key_n10 - Remove Rx address register
 *  @eth: pointer to eth structure
 *  @sriov_flag sriov status
 *
 *  update rss key to eth regs
 **/
static void rnp_eth_update_rss_key_n10(struct rnp_eth_info *eth, bool sriov_flag)
{
	struct rnp_hw *hw = (struct rnp_hw *)eth->back;
	int i;
	u8 *key_temp;
	int key_len = RNP_RSS_KEY_SIZE;
	u8 *key = hw->rss_key;
	u32 *value;

	u32 iov_en = (sriov_flag) ? RNP10_IOV_ENABLED : 0;

	key_temp = kmalloc(key_len, GFP_KERNEL);
	/* reoder the key */
	for (i = 0; i < key_len; i++)
		*(key_temp + key_len - i - 1) = *(key + i);

	value = (u32 *)key_temp;
	for (i = 0; i < key_len; i = i + 4)
		eth_wr32(eth, RNP10_ETH_RSS_KEY + i, *(value + i / 4));

	kfree(key_temp);

	/* open rss now */
	eth_wr32(eth, RNP10_ETH_RSS_CONTROL,
		 RNP10_ETH_ENABLE_RSS_ONLY | iov_en);
}

/**
 *  rnp_eth_update_rss_table_n10 - Remove Rx address register
 *  @eth: pointer to eth structure
 *
 *  update rss table to eth regs
 **/
static void rnp_eth_update_rss_table_n10(struct rnp_eth_info *eth)
{
	struct rnp_hw *hw = (struct rnp_hw *)eth->back;
	u32 reta_entries = hw->rss_indir_tbl_num;
	u32 tc_entries = hw->rss_tc_tbl_num;
	int i;

	for (i = 0; i < tc_entries; i++)
		eth_wr32(eth, RNP10_ETH_TC_IPH_OFFSET_TABLE(i),
			 hw->rss_tc_tbl[i]);

	for (i = 0; i < reta_entries; i++)
		eth_wr32(eth, RNP10_ETH_RSS_INDIR_TBL(i), hw->rss_indir_tbl[i]);
}

/**
 *  rnp_eth_set_vfta_n10 - Set VLAN filter table
 *  @eth: pointer to eth structure
 *  @vlan: VLAN id to write to VLAN filter
 *  @vlan_on: boolean flag to turn on/off VLAN in VFVF
 *
 *  Turn on/off specified VLAN in the VLAN filter table.
 **/
static s32 rnp_eth_set_vfta_n10(struct rnp_eth_info *eth, u32 vlan, bool vlan_on)
{
	s32 regindex;
	u32 bitindex;
	u32 vfta;
	u32 targetbit;
	bool vfta_changed = false;

	/* todo in vf mode vlvf regester can be set according to vind*/
	if (vlan > 4095)
		return RNP_ERR_PARAM;

	regindex = (vlan >> 5) & 0x7F;
	bitindex = vlan & 0x1F;
	targetbit = (1 << bitindex);
	vfta = eth_rd32(eth, RNP10_VFTA(regindex));

	if (vlan_on) {
		if (!(vfta & targetbit)) {
			vfta |= targetbit;
			vfta_changed = true;
		}
	} else {
		if ((vfta & targetbit)) {
			vfta &= ~targetbit;
			vfta_changed = true;
		}
	}

	if (vfta_changed)
		eth_wr32(eth, RNP10_VFTA(regindex), vfta);
	return 0;
}

static void rnp_eth_clr_vfta_n10(struct rnp_eth_info *eth)
{
	u32 offset;

	for (offset = 0; offset < eth->vft_size; offset++)
		eth_wr32(eth, RNP10_VFTA(offset), 0);
}

/**
 *  rnp_eth_set_vlan_filter_n10 - Set VLAN filter table
 *  @eth: pointer to eth structure
 *  @status: on |off
 *  Turn on/off VLAN filter table.
 **/
static void rnp_eth_set_vlan_filter_n10(struct rnp_eth_info *eth, bool status)
{
#define ETH_VLAN_FILTER_BIT (30)
	u32 value = eth_rd32(eth, RNP10_ETH_VLAN_FILTER_ENABLE);

	/* clear bit first */
	value &= (~(0x01 << ETH_VLAN_FILTER_BIT));
	if (status)
		value |= (0x01 << ETH_VLAN_FILTER_BIT);
	eth_wr32(eth, RNP10_ETH_VLAN_FILTER_ENABLE, value);
}

static u16 rnp_layer2_pritologic_n10(u16 hw_id)
{
	return hw_id;
}

static void rnp_eth_set_layer2_n10(struct rnp_eth_info *eth,
			    union rnp_atr_input *input, u16 pri_id, u8 queue,
			    bool prio_flag)
{
	u16 hw_id;

	hw_id = rnp_layer2_pritologic_n10(pri_id);
	/* enable layer2 */
	eth_wr32(eth, RNP10_ETH_LAYER2_ETQF(hw_id),
		 (0x1 << 31) | (ntohs(input->layer2_formate.proto)));

	/* setup action */
	if (queue == RNP_FDIR_DROP_QUEUE) {
		eth_wr32(eth, RNP10_ETH_LAYER2_ETQS(hw_id), (0x1 << 31));
	} else {
		if (queue == ACTION_TO_MPE)
			eth_wr32(eth, RNP10_ETH_LAYER2_ETQS(hw_id),
				 (0x1 << 29) | (MPE_PORT << 16));
		else
			/* setup ring_number */
			eth_wr32(eth, RNP10_ETH_LAYER2_ETQS(hw_id),
				 (0x1 << 30) | (queue << 20));
	}
}

static void rnp_eth_clr_layer2_n10(struct rnp_eth_info *eth, u16 pri_id)
{
	u16 hw_id;

	hw_id = rnp_layer2_pritologic_n10(pri_id);
	eth_wr32(eth, RNP10_ETH_LAYER2_ETQF(hw_id), 0);
}

static void rnp_eth_clr_all_layer2_n10(struct rnp_eth_info *eth)
{
	int i;
#define RNP10_MAX_LAYER2_FILTERS 16
	for (i = 0; i < RNP10_MAX_LAYER2_FILTERS; i++)
		eth_wr32(eth, RNP10_ETH_LAYER2_ETQF(i), 0);
}

static u16 rnp_tuple5_pritologic_n10(u16 hw_id)
{
	return hw_id;
}

static u16 rnp_tuple5_pritologic_tcam_n10(u16 pri_id)
{
	int i;
	int hw_id = 0;
	int step = 32;
	for (i = 0; i < pri_id; i++) {
		hw_id += step;
		if (hw_id > RNP10_MAX_TCAM_FILTERS)
			hw_id = hw_id - RNP10_MAX_TCAM_FILTERS + 1;
	}

	return hw_id;
}

static void rnp_eth_set_tuple5_n10(struct rnp_eth_info *eth,
			    union rnp_atr_input *input, u16 pri_id, u8 queue,
			    bool prio_flag)
{
	struct rnp_hw *hw = (struct rnp_hw *)eth->back;

#define RNP10_SRC_IP_MASK BIT(0)
#define RNP10_DST_IP_MASK BIT(1)
#define RNP10_SRC_PORT_MASK BIT(2)
#define RNP10_DST_PORT_MASK BIT(3)
#define RNP10_L4_PROTO_MASK BIT(4)

	if (hw->fdir_mode != fdir_mode_tcam) {
		u32 port = 0;
		u8 mask_temp = 0;
		u8 l4_proto_type = 0;
		u16 hw_id;

		hw_id = rnp_tuple5_pritologic_n10(pri_id);
		dbg("try to eable tuple 5 %x\n", hw_id);
		if (input->formatted.src_ip[0] != 0) {
			eth_wr32(eth, RNP10_ETH_TUPLE5_SAQF(hw_id),
				 htonl(input->formatted.src_ip[0]));
		} else {
			mask_temp |= RNP10_SRC_IP_MASK;
		}
		if (input->formatted.dst_ip[0] != 0) {
			eth_wr32(eth, RNP10_ETH_TUPLE5_DAQF(hw_id),
				 htonl(input->formatted.dst_ip[0]));
		} else
			mask_temp |= RNP10_DST_IP_MASK;
		if (input->formatted.src_port != 0)
			port |= (htons(input->formatted.src_port));
		else
			mask_temp |= RNP10_SRC_PORT_MASK;
		if (input->formatted.dst_port != 0)
			port |= (htons(input->formatted.dst_port) << 16);
		else
			mask_temp |= RNP10_DST_PORT_MASK;

		if (port != 0)
			eth_wr32(eth, RNP10_ETH_TUPLE5_SDPQF(hw_id), port);

		switch (input->formatted.flow_type) {
		case RNP_ATR_FLOW_TYPE_TCPV4:
			l4_proto_type = IPPROTO_TCP;
			break;
		case RNP_ATR_FLOW_TYPE_UDPV4:
			l4_proto_type = IPPROTO_UDP;
			break;
		case RNP_ATR_FLOW_TYPE_SCTPV4:
			l4_proto_type = IPPROTO_SCTP;
			break;
		case RNP_ATR_FLOW_TYPE_IPV4:
			l4_proto_type = input->formatted.inner_mac[0];
			break;
		default:
			l4_proto_type = 0;
		}

		if (l4_proto_type == 0)
			mask_temp |= RNP10_L4_PROTO_MASK;

		/* setup ftqf*/
		/* always set 0x3 */
		eth_wr32(eth, RNP10_ETH_TUPLE5_FTQF(hw_id),
			 (1 << 31) | (mask_temp << 25) | (l4_proto_type << 16) |
				 0x3);

		/* setup action */
		if (queue == RNP_FDIR_DROP_QUEUE) {
			eth_wr32(eth, RNP10_ETH_TUPLE5_POLICY(hw_id),
				 (0x1 << 31));
		} else {
			if (queue == ACTION_TO_MPE)
				eth_wr32(eth, RNP10_ETH_TUPLE5_POLICY(hw_id),
					 (0x1 << 29) | (MPE_PORT << 16));
			else
				eth_wr32(eth, RNP10_ETH_TUPLE5_POLICY(hw_id),
					 ((0x1 << 30) | (queue << 20)));
		}

	} else {
		u32 port = 0;
		u32 port_mask = 0;
		u8 l4_proto_type = 0;
		u8 l4_proto_mask = 0xff;
		u32 action = 0;
		u32 mark = 0;
		u16 hw_id;

		hw_id = rnp_tuple5_pritologic_tcam_n10(pri_id);
		eth_wr32(eth, RNP10_TCAM_MODE, 2);
		dbg("try to eable tcam %d\n", hw_id);
		if (input->formatted.src_ip[0] != 0) {
			eth_wr32(eth, RNP10_TCAM_SAQF(hw_id),
				 htonl(input->formatted.src_ip[0]));
			eth_wr32(eth, RNP10_TCAM_SAQF_MASK(hw_id),
				 htonl(input->formatted.src_ip_mask[0]));

			dbg("tcam src ip 0%x ---> 0x%x\n",
			    htonl(input->formatted.src_ip[0]),
			    RNP10_TCAM_SAQF(hw_id));
			dbg("tcam src ip mask 0%x ---> 0x%x\n",
			    htonl(input->formatted.src_ip_mask[0]),
			    RNP10_TCAM_SAQF_MASK(hw_id));
		} else {
			eth_wr32(eth, RNP10_TCAM_SAQF(hw_id), 0);
			eth_wr32(eth, RNP10_TCAM_SAQF_MASK(hw_id), 0);
			dbg("tcam src ip 0%x ---> 0x%x\n", 0,
			    RNP10_TCAM_SAQF(hw_id));
			dbg("tcam src ip mask 0%x ---> 0x%x\n", 0,
			    RNP10_TCAM_SAQF_MASK(hw_id));
		}
		if (input->formatted.dst_ip[0] != 0) {
			eth_wr32(eth, RNP10_TCAM_DAQF(hw_id),
				 htonl(input->formatted.dst_ip[0]));
			eth_wr32(eth, RNP10_TCAM_DAQF_MASK(hw_id),
				 htonl(input->formatted.dst_ip_mask[0]));
			dbg("tcam dst ip 0%x ---> 0x%x\n",
			    htonl(input->formatted.dst_ip[0]),
			    RNP10_TCAM_DAQF(hw_id));
			dbg("tcam dst ip mask 0%x ---> 0x%x\n",
			    htonl(input->formatted.dst_ip_mask[0]),
			    RNP10_TCAM_DAQF_MASK(hw_id));
		} else {
			eth_wr32(eth, RNP10_TCAM_DAQF(hw_id), 0);
			eth_wr32(eth, RNP10_TCAM_DAQF_MASK(hw_id), 0);
			dbg("tcam dst ip 0%x ---> 0x%x\n", 0,
			    RNP10_TCAM_DAQF(hw_id));
			dbg("tcam dst ip mask 0%x ---> 0x%x\n", 0,
			    RNP10_TCAM_DAQF_MASK(hw_id));
		}
		if (input->formatted.src_port != 0) {
			port |= (htons(input->formatted.src_port) << 16);
			port_mask |=
				(htons(input->formatted.src_port_mask) << 16);

		}
		if (input->formatted.dst_port != 0) {
			port |= (htons(input->formatted.dst_port));
			port_mask |= (htons(input->formatted.dst_port_mask));
		}

		/* setup src & dst port */
		if (port != 0) {
			eth_wr32(eth, RNP10_TCAM_SDPQF(hw_id), port);
			eth_wr32(eth, RNP10_TCAM_SDPQF_MASK(hw_id), port_mask);

			dbg("tcam port 0%x ---> 0x%x\n", port,
			    RNP10_TCAM_SDPQF(hw_id));
			dbg("tcam port mask 0%x ---> 0x%x\n", port_mask,
			    RNP10_TCAM_SDPQF_MASK(hw_id));
		} else {
			eth_wr32(eth, RNP10_TCAM_SDPQF(hw_id), 0);
			eth_wr32(eth, RNP10_TCAM_SDPQF_MASK(hw_id), 0);
			dbg("tcam port 0%x ---> 0x%x\n", port,
			    RNP10_TCAM_SDPQF(hw_id));
			dbg("tcam port mask 0%x ---> 0x%x\n", port_mask,
			    RNP10_TCAM_SDPQF_MASK(hw_id));
		}

		switch (input->formatted.flow_type) {
		case RNP_ATR_FLOW_TYPE_TCPV4:
			l4_proto_type = IPPROTO_TCP;
			break;
		case RNP_ATR_FLOW_TYPE_UDPV4:
			l4_proto_type = IPPROTO_UDP;
			break;
		case RNP_ATR_FLOW_TYPE_SCTPV4:
			l4_proto_type = IPPROTO_SCTP;
			break;
		case RNP_ATR_FLOW_TYPE_IPV4:
			l4_proto_type = input->formatted.inner_mac[0];
			l4_proto_mask = input->formatted.inner_mac_mask[0];
			break;
		default:
			l4_proto_type = 0;
			l4_proto_mask = 0;
		}

		if (l4_proto_type != 0) {
			action |= l4_proto_type;
			mark |= l4_proto_mask;
		} else {
		}

		/* setup action */
		if (queue == RNP_FDIR_DROP_QUEUE) {
			eth_wr32(eth, RNP10_TCAM_APQF(hw_id),
				 (0x1 << 31) | action);
			eth_wr32(eth, RNP10_TCAM_APQF_MASK(hw_id), mark);
			dbg("tcam action 0%x ---> 0x%x\n", (0x1 << 31) | action,
			    RNP10_TCAM_APQF(hw_id));
			dbg("tcam action mask 0%x ---> 0x%x\n", mark,
			    RNP10_TCAM_APQF_MASK(hw_id));
		} else {
			if (queue == ACTION_TO_MPE) {
				eth_wr32(eth, RNP10_TCAM_APQF(hw_id),
					 (0x1 << 29) | (MPE_PORT << 24) |
						 action);
			} else {
				eth_wr32(eth, RNP10_TCAM_APQF(hw_id),
					 ((0x1 << 30) | (queue << 16) |
					  action));
			}
			eth_wr32(eth, RNP10_TCAM_APQF_MASK(hw_id), mark);

			dbg("tcam action 0%x ---> 0x%x\n",
			    (0x1 << 30) | (queue << 16) | action,
			    RNP10_TCAM_APQF(hw_id));
			dbg("tcam action mask 0%x ---> 0x%x\n", mark,
			    RNP10_TCAM_APQF_MASK(hw_id));
		}
		eth_wr32(eth, RNP10_TCAM_MODE, 1);
	}
}

static void rnp_eth_clr_tuple5_n10(struct rnp_eth_info *eth, u16 pri_id)
{
	u16 hw_id;
	struct rnp_hw *hw = (struct rnp_hw *)eth->back;

	if (hw->fdir_mode != fdir_mode_tcam) {
		hw_id = rnp_tuple5_pritologic_n10(pri_id);
		eth_wr32(eth, RNP10_ETH_TUPLE5_FTQF(hw_id), 0);
	} else {
		hw_id = rnp_tuple5_pritologic_tcam_n10(pri_id);
		dbg("disable tcam tuple5 %d\n", hw_id);
		/* earase tcam */
		eth_wr32(eth, RNP10_TCAM_MODE, 2);
		eth_wr32(eth, RNP10_TCAM_SAQF(hw_id), 0);
		eth_wr32(eth, RNP10_TCAM_SAQF_MASK(hw_id), 0);
		eth_wr32(eth, RNP10_TCAM_DAQF(hw_id), 0);
		eth_wr32(eth, RNP10_TCAM_DAQF_MASK(hw_id), 0);
		eth_wr32(eth, RNP10_TCAM_SDPQF(hw_id), 0);
		eth_wr32(eth, RNP10_TCAM_SDPQF_MASK(hw_id), 0);
		eth_wr32(eth, RNP10_TCAM_APQF(hw_id), 0);
		eth_wr32(eth, RNP10_TCAM_APQF_MASK(hw_id), 0);
		eth_wr32(eth, RNP10_TCAM_MODE, 1);
	}
}

static void rnp_eth_clr_all_tuple5_n10(struct rnp_eth_info *eth)
{
	int i;

	struct rnp_hw *hw = (struct rnp_hw *)eth->back;

	if (hw->fdir_mode != fdir_mode_tcam) {
		for (i = 0; i < RNP10_MAX_TUPLE5_FILTERS; i++)
			eth_wr32(eth, RNP10_ETH_TUPLE5_FTQF(i), 0);
		eth_wr32(eth, RNP10_ETH_TCAM_EN, 0);
	} else {
		/* todo earase tcm */
		eth_wr32(eth, RNP10_ETH_TCAM_EN, 1);
		eth_wr32(eth, RNP10_TOP_ETH_TCAM_CONFIG_ENABLE, 1);
		eth_wr32(eth, RNP10_TCAM_MODE, 2);
		/* dont't open tcam cache */
		eth_wr32(eth, RNP10_TCAM_CACHE_ENABLE, 0);

		for (i = 0; i < RNP10_MAX_TCAM_FILTERS; i++) {
			eth_wr32(eth, RNP10_TCAM_SDPQF(i), 0);
			eth_wr32(eth, RNP10_TCAM_DAQF(i), 0);
			eth_wr32(eth, RNP10_TCAM_SAQF(i), 0);
			eth_wr32(eth, RNP10_TCAM_APQF(i), 0);

			eth_wr32(eth, RNP10_TCAM_SDPQF_MASK(i), 0);
			eth_wr32(eth, RNP10_TCAM_DAQF_MASK(i), 0);
			eth_wr32(eth, RNP10_TCAM_SAQF_MASK(i), 0);
			eth_wr32(eth, RNP10_TCAM_APQF_MASK(i), 0);
		}
		eth_wr32(eth, RNP10_TCAM_MODE, 1);
	}
}

static void rnp_eth_set_tcp_sync_n10(struct rnp_eth_info *eth,
				     int queue, bool flag,
				     bool prio)
{
	if (flag)
		eth_wr32(eth, RNP10_ETH_SYNQF, (0x1 << 30) | (queue << 20));
	else
		eth_wr32(eth, RNP10_ETH_SYNQF, 0);
}

static void rnp_eth_set_min_max_packets_n10(struct rnp_eth_info *eth, int min,
					    int max)
{
	eth_wr32(eth, RNP10_ETH_DEFAULT_RX_MIN_LEN, min);
	eth_wr32(eth, RNP10_ETH_DEFAULT_RX_MAX_LEN, max);
}

static void rnp_eth_set_vlan_strip_n10(struct rnp_eth_info *eth, u16 queue,
				       bool enable)
{
	u32 reg = RNP10_ETH_VLAN_VME_REG(queue / 32);
	u32 offset = queue % 32;
	u32 data = eth_rd32(eth, reg);

	if (enable == true)
		data |= (1 << offset);
	else
		data &= ~(1 << offset);

	eth_wr32(eth, reg, data);
}

static void rnp_eth_set_vxlan_port_n10(struct rnp_eth_info *eth, u32 port)
{
	eth_wr32(eth, RNP10_ETH_VXLAN_PORT, port);
}

static void rnp_eth_set_vxlan_mode_n10(struct rnp_eth_info *eth, bool inner)
{
	if (inner)
		eth_wr32(eth, RNP10_ETH_WRAP_FIELD_TYPE, 1);
	else
		eth_wr32(eth, RNP10_ETH_WRAP_FIELD_TYPE, 0);
}

static void rnp_eth_set_rx_hash_n10(struct rnp_eth_info *eth, bool status,
				    bool sriov_flag)
{
	u32 iov_en = (sriov_flag) ? RNP10_IOV_ENABLED : 0;

	if (status) {
		eth_wr32(eth, RNP10_ETH_RSS_CONTROL,
			 RNP10_ETH_ENABLE_RSS_ONLY | iov_en);
	} else {
		eth_wr32(eth, RNP10_ETH_RSS_CONTROL,
			 RNP10_ETH_DISABLE_RSS | iov_en);
	}
}

static s32 rnp_eth_set_fc_mode_n10(struct rnp_eth_info *eth)
{
	struct rnp_hw *hw = (struct rnp_hw *)eth->back;
	s32 ret_val = 0;
	int i;

	for (i = 0; i < RNP_MAX_TRAFFIC_CLASS; i++) {
		if ((hw->fc.current_mode & rnp_fc_tx_pause) &&
		    hw->fc.high_water[i]) {
			if (!hw->fc.low_water[i] ||
			    hw->fc.low_water[i] >= hw->fc.high_water[i]) {
				hw_dbg(hw,
				       "Invalid water mark configuration\n");
				ret_val = RNP_ERR_INVALID_LINK_SETTINGS;
				goto out;
			}
		}
	}

	for (i = 0; i < RNP_MAX_TRAFFIC_CLASS; i++) {
		if ((hw->fc.current_mode & rnp_fc_tx_pause)) {
			if (hw->fc.high_water[i]) {
				eth_wr32(eth, RNP10_ETH_HIGH_WATER(i),
					 hw->fc.high_water[i]);
			}
			if (hw->fc.low_water[i]) {
				eth_wr32(eth, RNP10_ETH_LOW_WATER(i),
					 hw->fc.low_water[i]);
			}
		}
	}
out:
	return ret_val;
}

static void rnp_eth_set_vf_vlan_mode_n10(struct rnp_eth_info *eth, u16 vlan,
					 int vf, bool enable)
{
	struct rnp_hw *hw = (struct rnp_hw *)&eth->back;
	u32 value = vlan;
	if (enable)
		value |= BIT(31);

	eth_wr32(eth, RNP10_VLVF(vf), value);

	/* todo, should consider mutiple queue */
	if (hw->hw_type == rnp_hw_n400) {
		if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED)
			eth_wr32(eth, RNP10_VLVF_TABLE(vf), (vf + 1) * 2);
		else
			eth_wr32(eth, RNP10_VLVF_TABLE(vf), vf * 2);

	} else {
		if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED)
			eth_wr32(eth, RNP10_VLVF_TABLE(vf), vf + 1);
		else
			eth_wr32(eth, RNP10_VLVF_TABLE(vf), vf);
	}
}

static int __get_ncsi_shm_info(struct rnp_hw *hw,
			       struct ncsi_shm_info *ncsi_shm)
{
	int i;
	int *ptr = (int *)ncsi_shm;
	int rbytes = round_up(sizeof(*ncsi_shm), 4);

	memset(ncsi_shm, 0, sizeof(*ncsi_shm));
	for (i = 0; i < (rbytes / 4); i++)
		ptr[i] = rd32(hw, hw->ncsi_vf_cpu_shm_pf_base + 4 * i);

	return (ncsi_shm->valid & RNP_NCSI_SHM_VALID_MASK) ==
	       RNP_NCSI_SHM_VALID;
}

static void rnp_ncsi_set_uc_addr_n10(struct rnp_eth_info *eth)
{
	struct ncsi_shm_info ncsi_shm;
	struct rnp_hw *hw = (struct rnp_hw *)eth->back;
	u8 mac[ETH_ALEN];

	if (!hw->ncsi_en)
		return;
	if (__get_ncsi_shm_info(hw, &ncsi_shm)) {
		if (ncsi_shm.valid & RNP_MC_VALID) {
			mac[0] = ncsi_shm.uc.uc_addr_lo & 0xff;
			mac[1] = (ncsi_shm.uc.uc_addr_lo >> 8) & 0xff;
			mac[2] = (ncsi_shm.uc.uc_addr_lo >> 16) & 0xff;
			mac[3] = (ncsi_shm.uc.uc_addr_lo >> 24) & 0xff;
			mac[4] = ncsi_shm.uc.uc_addr_hi & 0xff;
			mac[5] = (ncsi_shm.uc.uc_addr_hi >> 8) & 0xff;
			if (is_valid_ether_addr(mac))
				eth->ops.set_rar(eth, hw->num_rar_entries, mac,
						 true);
		}
	}
}

static void rnp_ncsi_set_mc_mta_n10(struct rnp_eth_info *eth)
{
	struct ncsi_shm_info ncsi_shm;
	struct rnp_hw *hw = (struct rnp_hw *)eth->back;
	u8 i;
	u8 mac[ETH_ALEN];

	if (!hw->ncsi_en)
		return;
	if (__get_ncsi_shm_info(hw, &ncsi_shm)) {
		if (ncsi_shm.valid & RNP_MC_VALID) {
			for (i = 0; i < RNP_NCSI_MC_COUNT; i++) {
				mac[0] = ncsi_shm.mc[i].mc_addr_lo & 0xff;
				mac[1] = (ncsi_shm.mc[i].mc_addr_lo >> 8) &
					 0xff;
				mac[2] = (ncsi_shm.mc[i].mc_addr_lo >> 16) &
					 0xff;
				mac[3] = (ncsi_shm.mc[i].mc_addr_lo >> 24) &
					 0xff;
				mac[4] = ncsi_shm.mc[i].mc_addr_hi & 0xff;
				mac[5] = (ncsi_shm.mc[i].mc_addr_hi >> 8) &
					 0xff;
				if (is_multicast_ether_addr(mac) &&
				    !is_zero_ether_addr(mac)) {
					rnp10_set_mta(hw, mac);
				}
			}
		}
	}
}

static void rnp_ncsi_set_vfta_n10(struct rnp_eth_info *eth)
{
	struct ncsi_shm_info ncsi_shm;
	struct rnp_hw *hw = (struct rnp_hw *)eth->back;

	if (!hw->ncsi_en)
		return;
	if (__get_ncsi_shm_info(hw, &ncsi_shm)) {
		if (ncsi_shm.valid & RNP_VLAN_VALID)
			hw->ops.set_vlan_filter(hw, ncsi_shm.ncsi_vlan, true,
						false);
	}
}

static struct rnp_eth_operations eth_ops_n10 = {
	.set_rar = &rnp_eth_set_rar_n10,
	.clear_rar = &rnp_eth_clear_rar_n10,
	.set_vmdq = &rnp_eth_set_vmdq_n10,
	.clear_vmdq = &rnp_eth_clear_vmdq_n10,
	.update_mc_addr_list = &rnp_eth_update_mc_addr_list_n10,
	.clr_mc_addr = &rnp_eth_clr_mc_addr_n10,
	/* store rss info to eth */
	.set_rss_key = &rnp_eth_update_rss_key_n10,
	.set_rss_table = &rnp_eth_update_rss_table_n10,
	.set_vfta = &rnp_eth_set_vfta_n10,
	.clr_vfta = &rnp_eth_clr_vfta_n10,
	.set_vlan_filter = &rnp_eth_set_vlan_filter_n10,
	/* ncsi */
	.ncsi_set_vfta = &rnp_ncsi_set_vfta_n10,
	.ncsi_set_uc_addr = &rnp_ncsi_set_uc_addr_n10,
	.ncsi_set_mc_mta = &rnp_ncsi_set_mc_mta_n10,
	.set_layer2_remapping = &rnp_eth_set_layer2_n10,
	.clr_layer2_remapping = &rnp_eth_clr_layer2_n10,
	.clr_all_layer2_remapping = &rnp_eth_clr_all_layer2_n10,
	.set_tuple5_remapping = &rnp_eth_set_tuple5_n10,
	.clr_tuple5_remapping = &rnp_eth_clr_tuple5_n10,
	.clr_all_tuple5_remapping = &rnp_eth_clr_all_tuple5_n10,
	.set_tcp_sync_remapping = &rnp_eth_set_tcp_sync_n10,
	.set_min_max_packet = &rnp_eth_set_min_max_packets_n10,
	.set_vlan_strip = &rnp_eth_set_vlan_strip_n10,
	.set_vxlan_port = &rnp_eth_set_vxlan_port_n10,
	.set_vxlan_mode = &rnp_eth_set_vxlan_mode_n10,
	.set_rx_hash = &rnp_eth_set_rx_hash_n10,
	.set_fc_mode = &rnp_eth_set_fc_mode_n10,
	.set_vf_vlan_mode = &rnp_eth_set_vf_vlan_mode_n10,
};

/**
 *  rnp_init_hw_n10 - Generic hardware initialization
 *  @hw: pointer to hardware structure
 *
 *  Initialize the hardware by resetting the hardware, filling the bus info
 *  structure and media type, clears all on chip counters, initializes receive
 *  address registers, multicast table, VLAN filter table, calls routine to set
 *  up link and flow control settings, and leaves transmit and receive units
 *  disabled and uninitialized
 **/
static s32 rnp_init_hw_ops_n10(struct rnp_hw *hw)
{
	s32 status = 0;

	/* Reset the hardware */
	status = hw->ops.reset_hw(hw);

	/* Start the HW */
	if (status == 0)
		status = hw->ops.start_hw(hw);

	return status;
}

static s32 rnp_get_permtion_mac_addr_n10(struct rnp_hw *hw, u8 *mac_addr)
{
	if (rnp_fw_get_macaddr(hw, hw->pfvfnum, mac_addr, hw->nr_lane)) {
		dbg("generate ramdom macaddress...\n");
		eth_random_addr(mac_addr);
	}

	hw->mac.mac_flags |= RNP_FLAGS_INIT_MAC_ADDRESS;
	dbg("%s mac:%pM\n", __func__, mac_addr);

	return 0;
}

static s32 rnp_reset_hw_ops_n10(struct rnp_hw *hw)
{
	int i;
	struct rnp_dma_info *dma = &hw->dma;
	struct rnp_eth_info *eth = &hw->eth;

	/* Call adapter stop to disable tx/rx and clear interrupts */
	dma_wr32(dma, RNP_DMA_AXI_EN, 0);

	
	/* if not ncsi or hw not support 'control nic_reset', driver control it */
	if (hw->ncsi_en && hw->fw_version >= 0x00060000) {
		/* fw will do nic-reset. to reduct ncsi bmc ping pkg lose */
	} else {
#define N10_NIC_RESET 0
		wr32(hw, RNP10_TOP_NIC_REST_N, N10_NIC_RESET);
		/*
		 * we need this
		 */
		wmb();
		wr32(hw, RNP10_TOP_NIC_REST_N, ~N10_NIC_RESET);
	}

	rnp_mbx_fw_reset_phy(hw);
	/* should set all tx-start to 1 */
	for (i = 0; i < RNP_N10_MAX_TX_QUEUES; i++)
		dma_ring_wr32(dma, RING_OFFSET(i) + RNP_DMA_TX_START, 1);

	wr32(hw, RNP10_TOP_ETH_BUG_40G_PATCH, 1);
	/* set 2046 --> 0x18070 */
	eth_wr32(eth, RNP10_ETH_RX_PROGFULL_THRESH_PORT, DROP_ALL_THRESH);

	/* tcam not reset */
	eth->ops.clr_all_tuple5_remapping(eth);

	/* Store the permanent mac address */
	if (!(hw->mac.mac_flags & RNP_FLAGS_INIT_MAC_ADDRESS)) {
		rnp_get_permtion_mac_addr_n10(hw, hw->mac.perm_addr);
		memcpy(hw->mac.addr, hw->mac.perm_addr, ETH_ALEN);
	}

	hw->ops.init_rx_addrs(hw);

	/* open vxlan default */
#define VXLAN_HW_ENABLE (1)
	eth_wr32(eth, RNP10_ETH_TUNNEL_MOD, VXLAN_HW_ENABLE);
	for (i = 0; i < dma->max_tx_queues; i++)
		rnp_wr_reg(hw->ring_msix_base + RING_VECTOR(i), 0);

	if (hw->phy_type == PHY_TYPE_SGMII) {
		u16 pause_bits = 0;
		u32 value;

		if (hw->fc.requested_mode == PAUSE_AUTO) {
			pause_bits |= ASYM_PAUSE | SYM_PAUSE;
		} else {
			if ((hw->fc.requested_mode & PAUSE_TX) &&
			    (!(hw->fc.requested_mode & PAUSE_RX))) {
				pause_bits |= ASYM_PAUSE;

			} else if ((!(hw->fc.requested_mode & PAUSE_TX)) &&
				   (!(hw->fc.requested_mode & PAUSE_RX))) {
			} else
				pause_bits |= ASYM_PAUSE | SYM_PAUSE;
		}
		rnp_mbx_phy_read(hw, 4, &value);
		value &= ~0xC00;
		value |= pause_bits;
		rnp_mbx_phy_write(hw, 4, value);
	}

	return 0;
}

static s32 rnp_start_hw_ops_n10(struct rnp_hw *hw)
{
	s32 ret_val = 0;
	struct rnp_eth_info *eth = &hw->eth;
	struct rnp_dma_info *dma = &hw->dma;

	eth_wr32(eth, RNP10_ETH_ERR_MASK_VECTOR,
		 INNER_L4_BIT | PKT_LEN_ERR | HDR_LEN_ERR);
	eth_wr32(eth, RNP10_ETH_BYPASS, 0);
	eth_wr32(eth, RNP10_ETH_DEFAULT_RX_RING, 0);

	/* DMA common Registers */
	dma_wr32(dma, RNP_DMA_CONFIG, DMA_VEB_BYPASS);

	/* enable-dma-axi */
	dma_wr32(dma, RNP_DMA_AXI_EN, (RX_AXI_RW_EN | TX_AXI_RW_EN));

	return ret_val;
}

/* set n10 min/max packet according to new_mtu
 * we support mtu + 14 + 4 * 3 as max packet len*/
static void rnp_set_mtu_hw_ops_n10(struct rnp_hw *hw, int new_mtu)
{
	struct rnp_eth_info *eth = &hw->eth;

	int min = 60;
	int max = new_mtu + ETH_HLEN + ETH_FCS_LEN * 3;

	hw->min_length_current = min;
	hw->max_length_current = max;

	eth->ops.set_min_max_packet(eth, min, max);
}

/* setup n10 vlan filter status */
static void rnp_set_vlan_filter_en_hw_ops_n10(struct rnp_hw *hw, bool status)
{
	struct rnp_eth_info *eth = &hw->eth;
	eth->ops.set_vlan_filter(eth, status);
}

/* set vlan to n10 vlan filter table & veb */
/* pf setup call */
static void rnp_set_vlan_filter_hw_ops_n10(struct rnp_hw *hw, u16 vid,
					   bool enable, bool sriov_flag)
{
	struct rnp_eth_info *eth = &hw->eth;
	struct rnp_dma_info *dma = &hw->dma;
	u32 vfnum = hw->max_vfs - 1;

	/* setup n10 eth vlan table */
	eth->ops.set_vfta(eth, vid, enable);

	/* setup veb */
	/* only ctags setup veb if in sriov and not stags */
	if (vid && sriov_flag) {
		if (enable) {
			dma->ops.set_veb_vlan(dma, vid, vfnum);
		} else {
			dma->ops.set_veb_vlan(dma, 0, vfnum);
		}
	}
}

static void rnp_set_vf_vlan_filter_hw_ops_n10(struct rnp_hw *hw, u16 vid,
					      int vf, bool enable,
					      bool veb_only)
{
	struct rnp_dma_info *dma = &hw->dma;

	if (!veb_only) {
		/* call set vfta without veb setup */
		hw->ops.set_vlan_filter(hw, vid, enable, false);

	} else {
		if (enable) {
			dma->ops.set_veb_vlan(dma, vid, vf);
		} else {
			dma->ops.set_veb_vlan(dma, 0, vf);
		}
	}
}

static void rnp_clr_vlan_veb_hw_ops_n10(struct rnp_hw *hw)
{
	struct rnp_dma_info *dma = &hw->dma;
	u32 vfnum = hw->vfnum;

	dma->ops.set_veb_vlan(dma, 0, vfnum);
}

/* setup n10 vlan strip status */
static void rnp_set_vlan_strip_hw_ops_n10(struct rnp_hw *hw, u16 queue,
					  bool strip)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.set_vlan_strip(eth, queue, strip);
}

/* update new n10 mac */
static void rnp_set_mac_hw_ops_n10(struct rnp_hw *hw, u8 *mac, bool sriov_flag)
{
	struct rnp_eth_info *eth = &hw->eth;
	struct rnp_dma_info *dma = &hw->dma;
	struct rnp_mac_info *mac_info = &hw->mac;
	/* use this queue index to setup veb */
	/* now pf use queu 0 /1
	 * vfnum is the last vfnum */
	int queue = hw->veb_ring;
	int vfnum = hw->vfnum;

	eth->ops.set_rar(eth, 0, mac, true);
	if (sriov_flag) {
		eth->ops.set_vmdq(eth, 0, queue / hw->sriov_ring_limit);
		dma->ops.set_veb_mac(dma, mac, vfnum, queue);
	}

	mac_info->ops.set_mac(mac_info, mac, 0);
}

/**
 * rnp_write_uc_addr_list - write unicast addresses to RAR table
 * @netdev: network interface device structure
 *
 * Writes unicast address list to the RAR table.
 * Returns: -ENOMEM on failure/insufficient address space
 *                0 on no addresses written
 *                X on writing X addresses to the RAR table
 **/
static int rnp_write_uc_addr_list_n10(struct rnp_hw *hw,
				      struct net_device *netdev,
				      bool sriov_flag)
{
	unsigned int rar_entries = hw->num_rar_entries - 1;
	u32 vfnum = hw->vfnum;
	struct rnp_eth_info *eth = &hw->eth;
	int count = 0;

	if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED)
		vfnum = 0;
	/* In SR-IOV mode significantly less RAR entries are available */
	if (sriov_flag)
		rar_entries = hw->max_pf_macvlans - 1;

	/* return ENOMEM indicating insufficient memory for addresses */
	if (netdev_uc_count(netdev) > rar_entries)
		return -ENOMEM;

	if (!netdev_uc_empty(netdev)) {
		struct netdev_hw_addr *ha;

		hw_dbg(hw, "%s: rar_entries:%d, uc_count:%d\n", __func__,
		       hw->num_rar_entries, netdev_uc_count(netdev));

		/* return error if we do not support writing to RAR table */
		if (!eth->ops.set_rar)
			return -ENOMEM;

		netdev_for_each_uc_addr(ha, netdev) {
			if (!rar_entries)
				break;
			/* that's ok */
			eth->ops.set_rar(eth, rar_entries, ha->addr,
					 RNP10_RAH_AV);
			if (sriov_flag)
				eth->ops.set_vmdq(eth, rar_entries, vfnum);

			rar_entries--;

			count++;
		}
	}
	/* write the addresses in reverse order to avoid write combining */

	hw_dbg(hw, "%s: Clearing RAR[1 - %d]\n", __func__, rar_entries);
	for (; rar_entries > 0; rar_entries--)
		eth->ops.clear_rar(eth, rar_entries);

	if (hw->ncsi_en)
		eth->ops.ncsi_set_uc_addr(eth);

	return count;
}

void check_vf_promisc(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;
	int i;

	hw->vf_promisc_mode = 0;
	for (i = 0; i < adapter->num_vfs; i++) {
		if (adapter->vfinfo[i].promisc_mode) {
			hw->vf_promisc_mode = 1;
			hw->vf_promisc_num = i;
			break;
		}
	}
}

static void rnp_set_rx_mode_hw_ops_n10(struct rnp_hw *hw,
				       struct net_device *netdev,
				       bool sriov_flag)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	u32 fctrl;
	netdev_features_t features = netdev->features;
	int count;
	struct rnp_eth_info *eth = &hw->eth;

	hw_dbg(hw, "%s\n", __func__);

	/* broadcast always bypass */
	fctrl = eth_rd32(eth, RNP10_ETH_DMAC_FCTRL) | RNP10_FCTRL_BPE;
	/* clear the bits we are changing the status of */
	fctrl &= ~(RNP10_FCTRL_UPE | RNP10_FCTRL_MPE);
	/* promisc mode */
#ifdef VF_PROMISC_SUPPORT
	check_vf_promisc(adapter);
	if ((netdev->flags & IFF_PROMISC) || (!hw->vf_promisc_mode)) {
#else
	if (netdev->flags & IFF_PROMISC) {
#endif
		hw->addr_ctrl.user_set_promisc = true;
		fctrl |= (RNP10_FCTRL_UPE | RNP10_FCTRL_MPE);
		/* disable hardware filter vlans in promisc mode */
		features &= ~NETIF_F_HW_VLAN_CTAG_FILTER;
		features &= ~NETIF_F_HW_VLAN_CTAG_RX;
	} else {
		if (netdev->flags & IFF_ALLMULTI) {
			fctrl |= RNP10_FCTRL_MPE;
		} else {
			/* Write addresses to the MTA, if the attempt fails
			 * then we should just turn on promiscuous mode so
			 * that we can at least receive multicast traffic
			 */
			/* we always update vf multicast info */
			count = eth->ops.update_mc_addr_list(eth, netdev, true);
			if (count < 0) {
				fctrl |= RNP10_FCTRL_MPE;
			} else if (count) {

			}
		}
		hw->addr_ctrl.user_set_promisc = false;
	}

	/*
	 * Write addresses to available RAR registers, if there is not
	 * sufficient space to store all the addresses then enable
	 * unicast promiscuous mode
	 */
	if (rnp_write_uc_addr_list_n10(hw, netdev, sriov_flag) < 0) {
		fctrl |= RNP10_FCTRL_UPE;
	}

	eth_wr32(eth, RNP10_ETH_DMAC_FCTRL, fctrl);
	if (features & NETIF_F_HW_VLAN_CTAG_FILTER)
		eth->ops.set_vlan_filter(eth, true);
	else
		eth->ops.set_vlan_filter(eth, false);

	if ((hw->addr_ctrl.user_set_promisc == true) ||
	    (adapter->priv_flags & RNP_PRIV_FLAG_REC_HDR_LEN_ERR)) {
		/* set pkt_len_err and hdr_len_err default to 1 */
		eth_wr32(eth, RNP10_ETH_ERR_MASK_VECTOR,
			 INNER_L4_BIT | PKT_LEN_ERR | HDR_LEN_ERR);
	} else {
		eth_wr32(eth, RNP10_ETH_ERR_MASK_VECTOR, INNER_L4_BIT);
	}

	hw->ops.set_mtu(hw, netdev->mtu);
}

/* setup an rar with vfnum */
static void rnp_set_rar_with_vf_hw_ops_n10(struct rnp_hw *hw, u8 *mac, int idx,
					   u32 vfnum, bool enable)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.set_rar(eth, idx, mac, enable);
	eth->ops.set_vmdq(eth, idx, vfnum);
}

static void rnp_clr_rar_hw_ops_n10(struct rnp_hw *hw, int idx)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.clear_rar(eth, idx);
}

static void rnp_clr_rar_all_hw_ops_n10(struct rnp_hw *hw)
{
	struct rnp_eth_info *eth = &hw->eth;
	unsigned int rar_entries = hw->num_rar_entries - 1;
	int i;

	for (i = 0; i < rar_entries; i++)
		eth->ops.clear_rar(eth, rar_entries);
}

static void rnp_set_fcs_mode_hw_ops_n10(struct rnp_hw *hw, bool status)
{
	struct rnp_mac_info *mac = &hw->mac;

	mac->ops.set_mac_fcs(mac, status);
}

static void rnp_set_vxlan_port_hw_ops_n10(struct rnp_hw *hw, u32 port)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.set_vxlan_port(eth, port);
}

static void rnp_set_vxlan_mode_hw_ops_n10(struct rnp_hw *hw, bool inner)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.set_vxlan_mode(eth, inner);
}

static void rnp_set_mac_rx_hw_ops_n10(struct rnp_hw *hw, bool status)
{
	struct rnp_mac_info *mac = &hw->mac;
	struct rnp_eth_info *eth = &hw->eth;

	if (status)
		eth_wr32(eth, RNP10_ETH_RX_PROGFULL_THRESH_PORT,
			 RECEIVE_ALL_THRESH);
	else
		eth_wr32(eth, RNP10_ETH_RX_PROGFULL_THRESH_PORT,
			 DROP_ALL_THRESH);

	mac->ops.set_mac_rx(mac, status);
}

static void rnp_set_sriov_status_hw_ops_n10(struct rnp_hw *hw, bool status)
{
	struct rnp_dma_info *dma = &hw->dma;
	struct rnp_eth_info *eth = &hw->eth;
	u32 v;

	if (status) {
		dma_wr32(dma, RNP_DMA_CONFIG,
			 dma_rd32(dma, RNP_DMA_CONFIG) & (~DMA_VEB_BYPASS));
		v = eth_rd32(eth, RNP10_MRQC_IOV_EN);
		v |= RNP10_IOV_ENABLED;
		eth_wr32(eth, RNP10_MRQC_IOV_EN, v);
	} else {
		v = eth_rd32(eth, RNP10_MRQC_IOV_EN);
		v &= ~(RNP10_IOV_ENABLED);
		eth_wr32(eth, RNP10_MRQC_IOV_EN, v);
		dma->ops.clr_veb_all(dma);
	}

#if defined(NIC_VF_FXIED) || defined(VF_PROMISC_SUPPORT)
	/* we setup default to pf */
	eth_wr32(eth, RNP10_VM_DMAC_MPSAR_RING(127), hw->default_vf_num);
	/* if pf or vf in promisc mode set promisc to that vf*/
	if (hw->vf_promisc_mode) {
		int fix_vf_num;

		if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED)
			fix_vf_num = (hw->vf_promisc_num + 1) * hw->sriov_ring_limit / 2;
		else
			fix_vf_num = (hw->vf_promisc_num) * hw->sriov_ring_limit / 2;

		eth_wr32(eth, RNP10_VM_DMAC_MPSAR_RING(127), fix_vf_num);
	}
#endif
}

static void rnp_set_sriov_vf_mc_hw_ops_n10(struct rnp_hw *hw, u16 mc_addr)
{
	struct rnp_eth_info *eth = &hw->eth;
	u32 vector_bit;
	u32 vector_reg;
	u32 mta_reg;

	vector_reg = (mc_addr >> 5) & 0x7F;
	vector_bit = mc_addr & 0x1F;
	mta_reg = eth_rd32(eth, RNP10_ETH_MULTICAST_HASH_TABLE(vector_reg));
	mta_reg |= (1 << vector_bit);
	eth_wr32(eth, RNP10_ETH_MULTICAST_HASH_TABLE(vector_reg), mta_reg);
}

static void rnp_update_sriov_info_hw_ops_n10(struct rnp_hw *hw)
{
}

static void rnp_set_pause_mode_hw_ops_n10(struct rnp_hw *hw)
{
	struct rnp_mac_info *mac = &hw->mac;
	struct rnp_eth_info *eth = &hw->eth;

	mac->ops.set_fc_mode(mac);
	eth->ops.set_fc_mode(eth);
}

static void rnp_get_pause_mode_hw_ops_n10(struct rnp_hw *hw)
{
	u32 value_r5;

	if (hw->phy_type != PHY_TYPE_SGMII) {
		if ((hw->fc.requested_mode & PAUSE_TX) &&
		    (hw->fc.requested_mode & PAUSE_RX)) {
			hw->fc.current_mode = rnp_fc_full;
		} else if (hw->fc.requested_mode & PAUSE_TX) {
			hw->fc.current_mode = rnp_fc_tx_pause;
		} else if (hw->fc.requested_mode & PAUSE_RX) {
			hw->fc.current_mode = rnp_fc_rx_pause;
		} else {
			hw->fc.current_mode = rnp_fc_none;
		}
		return;
	}

	/* we get pause mode from phy reg */
	rnp_mbx_phy_read(hw, 5, &value_r5);
	if (!hw->link) {
		/* if link is not up ,fc is null */
		hw->fc.current_mode = rnp_fc_none;
	} else {
		if (hw->fc.requested_mode == PAUSE_AUTO) {
			if (value_r5 & SYM_PAUSE)
				hw->fc.current_mode = rnp_fc_full;
			else if (value_r5 & ASYM_PAUSE)
				hw->fc.current_mode = rnp_fc_rx_pause;
			else
				hw->fc.current_mode = rnp_fc_none;

		} else if ((hw->fc.requested_mode & PAUSE_TX) &&
			   (hw->fc.requested_mode & PAUSE_RX)) {
			if (value_r5 & SYM_PAUSE)
				hw->fc.current_mode = rnp_fc_full;
			else if (value_r5 & ASYM_PAUSE)
				hw->fc.current_mode = rnp_fc_rx_pause;
			else
				hw->fc.current_mode = rnp_fc_none;

		} else if (hw->fc.requested_mode & PAUSE_TX) {
			if (value_r5 & SYM_PAUSE)
				hw->fc.current_mode = rnp_fc_tx_pause;
			else if (value_r5 & ASYM_PAUSE)
				hw->fc.current_mode = rnp_fc_none;
			else
				hw->fc.current_mode = rnp_fc_none;

		} else if (hw->fc.requested_mode & PAUSE_RX) {
			if (value_r5 & SYM_PAUSE)
				hw->fc.current_mode = rnp_fc_rx_pause;
			else if (value_r5 & ASYM_PAUSE)
				hw->fc.current_mode = rnp_fc_rx_pause;
			else
				hw->fc.current_mode = rnp_fc_none;

		} else {
			hw->fc.current_mode = rnp_fc_none;
		}
	}
}

static void rnp_update_hw_info_hw_ops_n10(struct rnp_hw *hw)
{
	struct rnp_dma_info *dma = &hw->dma;
	struct rnp_eth_info *eth = &hw->eth;
	struct rnp_adapter *adapter = (struct rnp_adapter *)hw->back;
	u32 data;
	/* 1 enable eth filter */
	eth_wr32(eth, RNP10_HOST_FILTER_EN, 1);
	/* 2 open redir en */
	eth_wr32(eth, RNP10_REDIR_EN, 1);

	/* 3 open sctp checksum and other checksum */
	if (hw->feature_flags & RNP_NET_FEATURE_TX_CHECKSUM)
		eth_wr32(eth, RNP10_ETH_SCTP_CHECKSUM_EN, 1);

	/* 4 mark muticaset as broadcast */
	dma_wr32(dma, RNP_VEB_MAC_MASK_LO, 0xffffffff);
	dma_wr32(dma, RNP_VEB_MAC_MASK_HI, 0xfeff);
	/* 5 setup dma split */

	data = dma_rd32(dma, RNP_DMA_CONFIG);
	data &= (0x00000ffff);
#ifdef FT_PADDING
#define PADDING_BIT 8
	if (adapter->priv_flags & RNP_PRIV_FLAG_FT_PADDING)
		SET_BIT(PADDING_BIT, data);
#endif
	/* in this mode we fixed dm split */
	/* if PAGE_SIZE */
#define RX_MAX_DWORD (96)
	data |= (((hw->dma_split_size) >> 4) << 16);
	dma_wr32(dma, RNP_DMA_CONFIG, data);
	/* 6 open vxlan inner match? */
	/* 7 setuptcp sync remmapping */
	/* n10 not support prio */
	if (adapter->priv_flags & RNP_PRIV_FLAG_TCP_SYNC) {
		hw->ops.set_tcp_sync_remapping(hw, adapter->tcp_sync_queue,
					       true, false);
	} else {
		hw->ops.set_tcp_sync_remapping(hw, adapter->tcp_sync_queue,
					       false, false);
	}
}

static void rnp_update_hw_rx_drop_hw_ops_n10(struct rnp_hw *hw)
{
	struct rnp_adapter *adapter = (struct rnp_adapter *)hw->back;
	int i;
	struct rnp_ring *ring;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		ring = adapter->rx_ring[i];
		if (adapter->rx_drop_status & BIT(i)) {
			ring_wr32(ring, PCI_DMA_REG_RX_DESC_TIMEOUT_TH,
				  adapter->drop_time);
		} else {
			ring_wr32(ring, PCI_DMA_REG_RX_DESC_TIMEOUT_TH, 0);
		}
	}
}

static void rnp_set_rx_hash_hw_ops_n10(struct rnp_hw *hw, bool status,
				       bool sriov_flag)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.set_rx_hash(eth, status, sriov_flag);
}

/* setup mac to rar 0
 * clean vmdq
 * clean mc addr */
static s32 rnp_init_rx_addrs_hw_ops_n10(struct rnp_hw *hw)
{
	struct rnp_eth_info *eth = &hw->eth;

	u32 i;
	u32 rar_entries = eth->num_rar_entries;
	u32 v;

	hw_dbg(hw, "init_rx_addrs:rar_entries:%d, mac.addr:%pM\n", rar_entries,
	       hw->mac.addr);
	/*
	 * If the current mac address is valid, assume it is a software override
	 * to the permanent address.
	 * Otherwise, use the permanent address from the eeprom.
	 */
	if (!is_valid_ether_addr(hw->mac.addr)) {
		/* Get the MAC address from the RAR0 for later reference */
		memcpy(hw->mac.addr, hw->mac.perm_addr, ETH_ALEN);
		hw_dbg(hw, " Keeping Current RAR0 Addr =%pM\n", hw->mac.addr);
	} else {
		/* Setup the receive address. */
		hw_dbg(hw, "Overriding MAC Address in RAR[0]\n");
		hw_dbg(hw, " New MAC Addr =%pM\n", hw->mac.addr);
		eth->ops.set_rar(eth, 0, hw->mac.addr, true);

		/*  clear VMDq pool/queue selection for RAR 0 */
		eth->ops.clear_vmdq(eth, 0, RNP_CLEAR_VMDQ_ALL);
	}
	hw->addr_ctrl.overflow_promisc = 0;
	hw->addr_ctrl.rar_used_count = 1;

	/* Zero out the other receive addresses. */
	hw_dbg(hw, "Clearing RAR[1-%d]\n", rar_entries - 1);
	for (i = 1; i < rar_entries; i++) {
		eth->ops.clear_rar(eth, i);
	}
	if (hw->ncsi_en)
		eth->ops.ncsi_set_uc_addr(eth);

	/* Clear the MTA */
	hw->addr_ctrl.mta_in_use = 0;
	v = eth_rd32(eth, RNP10_ETH_DMAC_MCSTCTRL);
	v &= (~0x3);
	v |= eth->mc_filter_type;
	eth_wr32(eth, RNP10_ETH_DMAC_MCSTCTRL, v);

	hw_dbg(hw, " Clearing MTA\n");
	eth->ops.clr_mc_addr(eth);
	if (hw->ncsi_en) {
		eth->ops.ncsi_set_mc_mta(eth);
		eth->ops.ncsi_set_vfta(eth);
	}

	return 0;
}

static void rnp_clr_vfta_hw_ops_n10(struct rnp_hw *hw)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.clr_vfta(eth);
}

static void rnp_set_txvlan_mode_hw_ops_n10(struct rnp_hw *hw, bool cvlan)
{
	struct rnp_mac_info *mac = &hw->mac;
	if (cvlan) {
		mac_wr32(mac, RNP10_MAC_TX_VLAN_TAG, 0x4000000);
		mac_wr32(mac, RNP10_MAC_TX_VLAN_MODE, 0x100000);
		mac_wr32(mac, RNP10_MAC_INNER_VLAN_INCL, 0x100000);
	} else {
		mac_wr32(mac, RNP10_MAC_TX_VLAN_TAG, 0xc600000);
		mac_wr32(mac, RNP10_MAC_TX_VLAN_MODE, 0x180000);
		mac_wr32(mac, RNP10_MAC_INNER_VLAN_INCL, 0x100000);
	}
}

static void rnp_set_rss_key_hw_ops_n10(struct rnp_hw *hw, bool sriov_flag)
{
	struct rnp_eth_info *eth = &hw->eth;
	struct rnp_adapter *adapter = (struct rnp_adapter *)hw->back;
	int key_len = RNP_RSS_KEY_SIZE;

	memcpy(hw->rss_key, adapter->rss_key, key_len);

	eth->ops.set_rss_key(eth, sriov_flag);
}

static void rnp_set_rss_table_hw_ops_n10(struct rnp_hw *hw)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.set_rss_table(eth);
}

static void rnp_set_mbx_link_event_hw_ops_n10(struct rnp_hw *hw, int enable)
{
	rnp_mbx_link_event_enable(hw, enable);
}

static void rnp_set_mbx_ifup_hw_ops_n10(struct rnp_hw *hw, int enable)
{
	rnp_mbx_ifup_down(hw, enable);

	if (hw->phy_type == PHY_TYPE_10G_TP) {
		struct rnp_adapter *adapter = (struct rnp_adapter *)hw->back;
		/* first call reset an */
		if (enable) {
			hw->ops.setup_link(hw, hw->phy.autoneg_advertised,
					   hw->autoneg, adapter->speed,
					   hw->duplex);
		}
	}
}

/**
 *  rnp_check_mac_link_n10 - Determine link and speed status
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @link_up: true when link is up
 *  @link_up_wait_to_complete: bool used to wait for link up or not
 *
 *  Reads the links register to determine if link is up and the current speed
 **/
static s32 rnp_check_mac_link_hw_ops_n10(struct rnp_hw *hw, rnp_link_speed *speed,
				  bool *link_up, bool *duplex,
				  bool link_up_wait_to_complete)
{

	if (hw->speed == 10) {
		*speed = RNP_LINK_SPEED_10_FULL;
	} else if (hw->speed == 100) {
		*speed = RNP_LINK_SPEED_100_FULL;
	} else if (hw->speed == 1000) {
		*speed = RNP_LINK_SPEED_1GB_FULL;
	} else if (hw->speed == 10000) {
		*speed = RNP_LINK_SPEED_10GB_FULL;
	} else if (hw->speed == 25000) {
		*speed = RNP_LINK_SPEED_25GB_FULL;
	} else if (hw->speed == 40000) {
		*speed = RNP_LINK_SPEED_40GB_FULL;
	} else {
		*speed = RNP_LINK_SPEED_UNKNOWN;
	}

	*link_up = hw->link;
	*duplex = 1;

	return 0;
}

static s32 rnp_setup_mac_link_hw_ops_n10(struct rnp_hw *hw, u32 adv, u32 autoneg,
				  u32 speed, u32 duplex)
{
	struct rnp_adapter *adpt = hw->back;
	u32 value = 0;
	u32 value_r4 = 0;
	u32 value_r9 = 0;

	rnp_logd(LOG_PHY,
		 "%s setup phy: phy_addr=%d speed=%d duplex=%d autoneg=%d "
		 "is_backplane=%d is_sgmii=%d\n",
		 __func__, adpt->phy_addr, speed, duplex, autoneg,
		 hw->is_backplane, hw->is_sgmii);

	if (hw->is_backplane) {
		/* Backplane type, support AN, unsupport set speed */
		return rnp_set_lane_fun(hw, LANE_FUN_AN, autoneg, 0, 0, 0);
	}

	/* TODO: Not support fiber */
	if ((!hw->is_sgmii) && (hw->phy_type != PHY_TYPE_10G_TP)) {
		if (hw->force_10g_1g_speed_ablity) {
			return rnp_mbx_force_speed(hw, speed);
		} else {
			return 0;
		}
	}

	if (hw->phy_type == PHY_TYPE_10G_TP) {
		rnp_mbx_phy_read(hw, PHY_826x_MDIX, &value);

		value &= ~(BIT(8) | BIT(9));
		/* Options: 0: Auto (default)  1: MDI mode  2: MDI-X mode */
		switch (hw->phy.mdix) {
		case 1:
			value |= BIT(8)|BIT(9);
			break;
		case 2:
			value |= BIT(9);
			break;
		case 0:
		default:
			break;
		}
		rnp_mbx_phy_write(hw, PHY_826x_MDIX, value);

		if (!autoneg) {
			rnp_mbx_phy_read(hw, PHY_826x_SPEED, &value);
			value &= (~(BIT(13) | BIT(6) | BIT(5) | BIT(4) |
				    BIT(3) | BIT(2)));

			switch (speed) {
			case RNP_LINK_SPEED_10GB_FULL:
				value |= BIT(13) | BIT(6);
				break;
			case RNP_LINK_SPEED_1GB_FULL:
			case RNP_LINK_SPEED_1GB_HALF:
				value |= BIT(6);
				;
				break;
			case RNP_LINK_SPEED_100_FULL:
			case RNP_LINK_SPEED_100_HALF:
				value |= BIT(13);
				break;
			case RNP_LINK_SPEED_10_FULL:
			case RNP_LINK_SPEED_10_HALF:
				value = 0;
				break;
			default:
				hw_dbg(hw, "unknown speed = 0x%x.\n", speed);
				break;
			}
			rnp_mbx_phy_write(hw, PHY_826x_SPEED, value);
			rnp_mbx_phy_read(hw, PHY_826x_DUPLEX, &value);
			value &= (~BIT(8));
			if (duplex)
				value |= BIT(8);
			rnp_mbx_phy_write(hw, PHY_826x_DUPLEX, value);
			rnp_mbx_phy_read(hw, PHY_826x_AN, &value);
			value &= (~BIT(12));
			rnp_mbx_phy_write(hw, PHY_826x_AN, value);
		} else {
			rnp_mbx_phy_read(hw, PHY_826x_ADV, &value);

			value &= (~(BIT(5) | BIT(6) | BIT(7) | BIT(8) |
				    BIT(10) | BIT(11)));

			if (adv & RNP_LINK_SPEED_100_FULL) {
				hw->phy.autoneg_advertised |=
					RNP_LINK_SPEED_100_FULL;
				value |= BIT(8);
			}
			if (adv & RNP_LINK_SPEED_100_HALF) {
				hw->phy.autoneg_advertised |=
					RNP_LINK_SPEED_100_FULL;
				value |= BIT(7);
			}

			value |= BIT(10) | BIT(11);
			/* BIT10 fc BIT11 asyfc */
			rnp_mbx_phy_write(hw, PHY_826x_ADV, value);

			rnp_mbx_phy_read(hw, PHY_826x_GBASE_ADV, &value);
			value &= (~(BIT(7) | BIT(8) | BIT(12)));

			/* bit 7 2.5G  bit 8 5G */
			if (adv & RNP_LINK_SPEED_10GB_FULL) {
				hw->phy.autoneg_advertised |=
					RNP_LINK_SPEED_10GB_FULL;
				value |= BIT(12);
			}
			rnp_mbx_phy_write(hw, PHY_826x_GBASE_ADV, value);
			rnp_mbx_phy_read(hw, PHY_826x_GBASE_ADV_2, &value);
			value &= 0x00ff;
			if (adv & RNP_LINK_SPEED_1GB_FULL) {
				hw->phy.autoneg_advertised |=
					RNP_LINK_SPEED_1GB_FULL;
				value |= BIT(9);
			}
			if (adv & RNP_LINK_SPEED_1GB_HALF) {
				hw->phy.autoneg_advertised |=
					RNP_LINK_SPEED_1GB_HALF;
				value |= BIT(8);
			}
			rnp_mbx_phy_write(hw, PHY_826x_GBASE_ADV_2, value);
			rnp_mbx_phy_read(hw, PHY_826x_AN, &value);
			value |= BIT(12) | BIT(9);
			rnp_mbx_phy_write(hw, PHY_826x_AN, value);
		}

		return 0;
	}

	/* Set MDI/MDIX mode */
	rnp_mbx_phy_read(hw, RNP_YT8531_PHY_SPEC_CTRL, &value);
	value &= ~RNP_YT8531_PHY_SPEC_CTRL_MDIX_CFG_MASK;
	/* Options: 0: Auto (default)  1: MDI mode  2: MDI-X mode */
	switch (hw->phy.mdix) {
	case 1:
		break;
	case 2:
		value |= RNP_YT8531_PHY_SPEC_CTRL_FORCE_MDIX;
		break;
	case 0:
	default:
		value |= RNP_YT8531_PHY_SPEC_CTRL_AUTO_MDI_MDIX;
		break;
	}
	rnp_mbx_phy_write(hw, RNP_YT8531_PHY_SPEC_CTRL, value);

	/*
	 * Clear autoneg_advertised and set new values based on input link
	 * speed.
	 */
	hw->phy.autoneg_advertised = speed;

	if (!autoneg) {
		switch (speed) {
		case RNP_LINK_SPEED_1GB_FULL:
		case RNP_LINK_SPEED_1GB_HALF:
			value = RNP_MDI_PHY_SPEED_SELECT1;
			speed = RNP_LINK_SPEED_1GB_FULL;
			goto out;
			break;
		case RNP_LINK_SPEED_100_FULL:
		case RNP_LINK_SPEED_100_HALF:
			value = RNP_MDI_PHY_SPEED_SELECT0;
			break;
		case RNP_LINK_SPEED_10_FULL:
		case RNP_LINK_SPEED_10_HALF:
			value = 0;
			break;
		default:
			value = RNP_MDI_PHY_SPEED_SELECT0 |
				RNP_MDI_PHY_SPEED_SELECT1;
			hw_dbg(hw, "unknown speed = 0x%x.\n", speed);
			break;
		}
		/* duplex full */
		if (duplex)
			value |= RNP_MDI_PHY_DUPLEX;
		value |= 0x8000;
		rnp_mbx_phy_write(hw, 0x0, value);
		goto skip_an;
	}

	/* start_an */
	value_r4 = 0x1E0;
	value_r9 = 0x300;
	/* disable 100/10base-T Self-negotiation ability */
	rnp_mbx_phy_read(hw, 0x4, &value);
	value &= ~value_r4;
	rnp_mbx_phy_write(hw, 0x4, value);

	/* disable 1000base-T Self-negotiation ability */
	rnp_mbx_phy_read(hw, 0x9, &value);
	value &= ~value_r9;
	rnp_mbx_phy_write(hw, 0x9, value);

	value_r4 = 0x0;
	value_r9 = 0x0;

	if (adv & RNP_LINK_SPEED_1GB_FULL) {
		hw->phy.autoneg_advertised |= RNP_LINK_SPEED_1GB_FULL;
		value_r9 |= 0x200;
	}
	if (adv & RNP_LINK_SPEED_100_FULL) {
		hw->phy.autoneg_advertised |= RNP_LINK_SPEED_100_FULL;
		value_r4 |= 0x100;
	}
	if (adv & RNP_LINK_SPEED_10_FULL) {
		hw->phy.autoneg_advertised |= RNP_LINK_SPEED_10_FULL;
		value_r4 |= 0x40;
	}

	if (adv & RNP_LINK_SPEED_1GB_HALF) {
		hw->phy.autoneg_advertised |= RNP_LINK_SPEED_1GB_HALF;
		value_r9 |= 0x100;
	}
	if (adv & RNP_LINK_SPEED_100_HALF) {
		hw->phy.autoneg_advertised |= RNP_LINK_SPEED_100_HALF;
		value_r4 |= 0x80;
	}
	if (adv & RNP_LINK_SPEED_10_HALF) {
		hw->phy.autoneg_advertised |= RNP_LINK_SPEED_10_HALF;
		value_r4 |= 0x20;
	}

	/* enable 1000base-T Self-negotiation ability */
	rnp_mbx_phy_read(hw, 0x9, &value);
	value |= value_r9;
	rnp_mbx_phy_write(hw, 0x9, value);

	/* enable 100/10base-T Self-negotiation ability */
	rnp_mbx_phy_read(hw, 0x4, &value);
	value |= value_r4;
	rnp_mbx_phy_write(hw, 0x4, value);

	/* software reset to make the above configuration take effect*/
	rnp_mbx_phy_read(hw, 0x0, &value);
	value |= 0x9200;
	rnp_mbx_phy_write(hw, 0x0, value);
skip_an:
	/* power on in UTP mode */
	rnp_mbx_phy_read(hw, 0x0, &value);
	value &= ~0x800;
	rnp_mbx_phy_write(hw, 0x0, value);

out:
	return 0;
}

static void rnp_clean_link_hw_ops_n10(struct rnp_hw *hw)
{
	hw->link = 0;
}

static void rnp_set_layer2_hw_ops_n10(struct rnp_hw *hw,
				      union rnp_atr_input *input, u16 pri_id,
				      u8 queue, bool prio_flag)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.set_layer2_remapping(eth, input, pri_id, queue, prio_flag);
}

static void rnp_clr_layer2_hw_ops_n10(struct rnp_hw *hw, u16 pri_id)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.clr_layer2_remapping(eth, pri_id);
}

static void rnp_clr_all_layer2_hw_ops_n10(struct rnp_hw *hw)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.clr_all_layer2_remapping(eth);
}

static void rnp_clr_all_tuple5_hw_ops_n10(struct rnp_hw *hw)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.clr_all_tuple5_remapping(eth);
}

static void rnp_set_tcp_sync_hw_ops_n10(struct rnp_hw *hw, int queue, bool flag,
					bool prio)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.set_tcp_sync_remapping(eth, queue, flag, prio);
}

static void rnp_update_msix_count_hw_ops_n10(struct rnp_hw *hw, int msix_count)
{
	int msix_count_new;
	struct rnp_mac_info *mac = &hw->mac;

	msix_count_new = clamp_t(int, msix_count, 2, RNP_N10_MSIX_VECTORS);

	mac->max_msix_vectors = msix_count_new;
	hw->max_msix_vectors = msix_count_new;
}

static void rnp_set_tuple5_hw_ops_n10(struct rnp_hw *hw,
				      union rnp_atr_input *input, u16 pri_id,
				      u8 queue, bool prio_flag)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.set_tuple5_remapping(eth, input, pri_id, queue, prio_flag);
}

static void rnp_clr_tuple5_hw_ops_n10(struct rnp_hw *hw, u16 pri_id)
{
	struct rnp_eth_info *eth = &hw->eth;

	eth->ops.clr_tuple5_remapping(eth, pri_id);
}

static void rnp_update_hw_status_hw_ops_n10(struct rnp_hw *hw,
					    struct rnp_hw_stats *hw_stats,
					    struct net_device_stats *net_stats)
{
	struct rnp_dma_info *dma = &hw->dma;
	struct rnp_eth_info *eth = &hw->eth;
	struct rnp_mac_info *mac = &hw->mac;
	int port;

	hw_stats->dma_to_dma =
		dma_rd32(dma, RNP_DMA_STATS_DMA_TO_MAC_CHANNEL_0) +
		dma_rd32(dma, RNP_DMA_STATS_DMA_TO_MAC_CHANNEL_1) +
		dma_rd32(dma, RNP_DMA_STATS_DMA_TO_MAC_CHANNEL_2) +
		dma_rd32(dma, RNP_DMA_STATS_DMA_TO_MAC_CHANNEL_3);

	hw_stats->dma_to_switch = dma_rd32(dma, RNP_DMA_STATS_DMA_TO_SWITCH);
	hw_stats->mac_to_dma = dma_rd32(dma, RNP_DMA_STATS_MAC_TO_DMA);

	net_stats->rx_crc_errors = 0;
	net_stats->rx_errors = 0;

	for (port = 0; port < 4; port++) {
		/* we use Hardware stats? */
		net_stats->rx_crc_errors +=
			eth_rd32(eth, RNP10_RXTRANS_CRC_ERR_PKTS(port));
		net_stats->rx_errors +=
			eth_rd32(eth, RNP10_RXTRANS_WDT_ERR_PKTS(port)) +
			eth_rd32(eth, RNP10_RXTRANS_CODE_ERR_PKTS(port)) +
			eth_rd32(eth, RNP10_RXTRANS_CRC_ERR_PKTS(port)) +
			eth_rd32(eth, RNP10_RXTRANS_SLEN_ERR_PKTS(port)) +
			eth_rd32(eth, RNP10_RXTRANS_GLEN_ERR_PKTS(port)) +
			eth_rd32(eth, RNP10_RXTRANS_IPH_ERR_PKTS(port)) +
			eth_rd32(eth, RNP10_RXTRANS_LEN_ERR_PKTS(port));
	}
	hw_stats->invalid_dropped_packets =
		eth_rd32(eth, RNP10_ETH_INVALID_DROP_PKTS);
	hw_stats->rx_capabity_lost =
		eth_rd32(eth, RNP10_RXTRANS_DROP(0)) +
		eth_rd32(eth, RNP10_RXTRANS_CUT_ERR_PKTS(0));
	hw_stats->filter_dropped_packets =
		eth_rd32(eth, RNP10_ETH_FILTER_DROP_PKTS);
	hw_stats->host_l2_match_drop =
		eth_rd32(eth, RNP10_ETH_HOST_L2_DROP_PKTS);
	hw_stats->redir_input_match_drop =
		eth_rd32(eth, RNP10_ETH_REDIR_INPUT_MATCH_DROP_PKTS);
	hw_stats->redir_etype_match_drop =
		eth_rd32(eth, RNP10_ETH_ETYPE_DROP_PKTS);
	hw_stats->redir_tcp_syn_match_drop =
		eth_rd32(eth, RNP10_ETH_TCP_SYN_DROP_PKTS);
	hw_stats->redir_tuple5_match_drop =
		eth_rd32(eth, RNP10_ETH_REDIR_TUPLE5_DROP_PKTS);
	hw_stats->redir_tcam_match_drop =
		eth_rd32(eth, RNP10_ETH_REDIR_TCAM_DROP_PKTS);
	hw_stats->bmc_dropped_packets =
		eth_rd32(eth, RNP10_ETH_DECAP_BMC_DROP_NUM);
	hw_stats->switch_dropped_packets =
		eth_rd32(eth, RNP10_ETH_DECAP_SWITCH_DROP_NUM);
	hw_stats->mac_rx_broadcast =
		mac_rd32(mac, RNP10_MAC_STATS_BROADCAST_LOW);
	hw_stats->mac_rx_broadcast +=
		((u64)mac_rd32(mac, RNP10_MAC_STATS_BROADCAST_HIGH) << 32);
	hw_stats->mac_rx_multicast =
		mac_rd32(mac, RNP10_MAC_STATS_MULTICAST_LOW);
	hw_stats->mac_rx_multicast +=
		((u64)mac_rd32(mac, RNP10_MAC_STATS_MULTICAST_HIGH) << 32);
	hw_stats->mac_rx_pause_count =
		mac_rd32(mac, RNP10_MAC_STATS_RX_PAUSE_COUNT_LOW);
	hw_stats->mac_rx_pause_count +=
		((u64)mac_rd32(mac, RNP10_MAC_STATS_RX_PAUSE_COUNT_HIGH) << 32);
	hw_stats->mac_tx_pause_count =
		mac_rd32(mac, RNP10_MAC_STATS_TX_PAUSE_COUNT_LOW);
	hw_stats->mac_tx_pause_count +=
		((u64)mac_rd32(mac, RNP10_MAC_STATS_TX_PAUSE_COUNT_HIGH) << 32);
}

enum n10_priv_bits {
	n10_mac_loopback = 0,
	n10_switch_loopback = 1,
	n10_veb_enable = 4,
	n10_padding_enable = 8,
	n10_padding_debug_enable = 0x10,
};

static const char rnp10_priv_flags_strings[][ETH_GSTRING_LEN] = {
#define RNP10_MAC_LOOPBACK BIT(0)
#define RNP10_SWITCH_LOOPBACK BIT(1)
#define RNP10_VEB_ENABLE BIT(2)
#define RNP10_FT_PADDING BIT(3)
#define RNP10_PADDING_DEBUG BIT(4)
#define RNP10_PTP_FEATURE BIT(5)
#define RNP10_SIMULATE_DOWN BIT(6)
#define RNP10_VXLAN_INNER_MATCH BIT(7)
#define RNP10_STAG_ENABLE BIT(8)
#define RNP10_REC_HDR_LEN_ERR BIT(9)
#define RNP10_SRIOV_VLAN_MODE BIT(10)
#define RNP10_REMAP_MODE BIT(11)
#define RNP10_LLDP_EN_STAT BIT(12)
#define RNP10_FORCE_CLOSE BIT(13)
	"mac_loopback",
	"switch_loopback",
	"veb_enable",
	"pcie_patch",
	"padding_debug",
	"ptp_performance_debug",
	"simulate_link_down",
	"vxlan_inner_match",
	"stag_enable",
	"mask_len_err",
	"sriov_vlan_mode",
	"remap_mode1",
	"lldp_en",
	"link_down_on_close",
};

#define RNP10_PRIV_FLAGS_STR_LEN ARRAY_SIZE(rnp10_priv_flags_strings)

const struct rnp_stats rnp10_gstrings_net_stats[] = {
	RNP_NETDEV_STAT(rx_packets),
	RNP_NETDEV_STAT(tx_packets),
	RNP_NETDEV_STAT(rx_bytes),
	RNP_NETDEV_STAT(tx_bytes),
	RNP_NETDEV_STAT(rx_errors),
	RNP_NETDEV_STAT(tx_errors),
	RNP_NETDEV_STAT(rx_dropped),
	RNP_NETDEV_STAT(tx_dropped),
	RNP_NETDEV_STAT(multicast),
	RNP_NETDEV_STAT(collisions),
	RNP_NETDEV_STAT(rx_over_errors),
	RNP_NETDEV_STAT(rx_crc_errors),
	RNP_NETDEV_STAT(rx_frame_errors),
	RNP_NETDEV_STAT(rx_fifo_errors),
	RNP_NETDEV_STAT(rx_missed_errors),
	RNP_NETDEV_STAT(tx_aborted_errors),
	RNP_NETDEV_STAT(tx_carrier_errors),
	RNP_NETDEV_STAT(tx_fifo_errors),
	RNP_NETDEV_STAT(tx_heartbeat_errors),
};

#define RNP10_GLOBAL_STATS_LEN ARRAY_SIZE(rnp10_gstrings_net_stats)

static struct rnp_stats rnp10_hwstrings_stats[] = {
	RNP_HW_STAT("dma_to_mac", hw_stats.dma_to_dma),
	RNP_HW_STAT("dma_to_switch", hw_stats.dma_to_switch),
	RNP_HW_STAT("eth_to_dma", hw_stats.mac_to_dma),
	RNP_HW_STAT("vlan_add_cnt", hw_stats.vlan_add_cnt),
	RNP_HW_STAT("vlan_strip_cnt", hw_stats.vlan_strip_cnt),
	RNP_HW_STAT("invalid_dropped_packets",
		    hw_stats.invalid_dropped_packets),
	RNP_HW_STAT("rx_capabity_drop", hw_stats.rx_capabity_lost),
	RNP_HW_STAT("filter_dropped_packets", hw_stats.filter_dropped_packets),
	RNP_HW_STAT("host_l2_match_drop", hw_stats.host_l2_match_drop),
	RNP_HW_STAT("redir_input_match_drop", hw_stats.redir_input_match_drop),
	RNP_HW_STAT("redir_etype_match_drop", hw_stats.redir_etype_match_drop),
	RNP_HW_STAT("redir_tcp_syn_match_drop",
		    hw_stats.redir_tcp_syn_match_drop),
	RNP_HW_STAT("redir_tuple5_match_drop",
		    hw_stats.redir_tuple5_match_drop),
	RNP_HW_STAT("redir_tcam_match_drop", hw_stats.redir_tcam_match_drop),
	RNP_HW_STAT("bmc_dropped_packets", hw_stats.bmc_dropped_packets),
	RNP_HW_STAT("switch_dropped_packets", hw_stats.switch_dropped_packets),
	RNP_HW_STAT("rx_csum_offload_errors", hw_csum_rx_error),
	RNP_HW_STAT("rx_csum_offload_good", hw_csum_rx_good),
	RNP_HW_STAT("rx_broadcast_count", hw_stats.mac_rx_broadcast),
	RNP_HW_STAT("rx_multicast_count", hw_stats.mac_rx_multicast),
	RNP_HW_STAT("mac_rx_pause_count", hw_stats.mac_rx_pause_count),
	RNP_HW_STAT("mac_tx_pause_count", hw_stats.mac_tx_pause_count),
};

#define RNP10_HWSTRINGS_STATS_LEN ARRAY_SIZE(rnp10_hwstrings_stats)

#define RNP10_STATS_LEN                                       \
	(RNP10_GLOBAL_STATS_LEN + RNP10_HWSTRINGS_STATS_LEN + \
	 RNP_QUEUE_STATS_LEN)

static const char rnp10_gstrings_test[][ETH_GSTRING_LEN] = {
	"Register test  (offline)", "Eeprom test    (offline)",
	"Interrupt test (offline)", "Loopback test  (offline)",
	"Link test   (on/offline)"
};

#define RNP10_TEST_LEN (sizeof(rnp10_gstrings_test) / ETH_GSTRING_LEN)

static int rnp10_get_regs_len(struct net_device *netdev)
{
#define RNP10_REGS_LEN 1
	return RNP10_REGS_LEN * sizeof(u32);
}

#define ADVERTISED_MASK_10G                                        \
	(SUPPORTED_10000baseT_Full | SUPPORTED_10000baseKX4_Full | \
	 SUPPORTED_10000baseKR_Full)

#define SUPPORTED_MASK_40G                                           \
	(SUPPORTED_40000baseKR4_Full | SUPPORTED_40000baseCR4_Full | \
	 SUPPORTED_40000baseSR4_Full | SUPPORTED_40000baseLR4_Full)

#define ADVERTISED_MASK_40G                                          \
	(SUPPORTED_40000baseKR4_Full | SUPPORTED_40000baseCR4_Full | \
	 SUPPORTED_40000baseSR4_Full | SUPPORTED_40000baseLR4_Full)

#define SUPPORTED_10000baseT 0

static int rnp_set_autoneg_adv_from_hw(struct rnp_hw *hw,
				       struct ethtool_link_ksettings *ks)
{
	u32 value_r0 = 0, value_r4 = 0, value_r9 = 0;
	u32 value_r20, value_r412;

	/* Read autoneg state from phy */
	if (hw->phy_type == PHY_TYPE_SGMII) {
		rnp_mbx_phy_read(hw, 0x0, &value_r0);
		/* Not support AN, return directly */
		if (!(value_r0 & BIT(12)))
			return 0;

		rnp_mbx_phy_read(hw, 0x4, &value_r4);
		rnp_mbx_phy_read(hw, 0x9, &value_r9);
		if (value_r4 & 0x100)
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     100baseT_Full);
		if (value_r4 & 0x80)
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     100baseT_Half);
		if (value_r4 & 0x40)
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10baseT_Full);
		if (value_r4 & 0x20)
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10baseT_Half);
		if (value_r9 & 0x200)
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     1000baseT_Full);
		if (value_r9 & 0x100)
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     1000baseT_Half);
	}

	if (hw->phy_type == PHY_TYPE_10G_TP) {
		rnp_mbx_phy_read(hw, (PHY_C45 | PHY_MMD(7) | 0x0), &value_r0);

		if (!(value_r0 & BIT(12)))
			return 0;

		rnp_mbx_phy_read(hw, (PHY_C45 | PHY_MMD(7) | 0x20), &value_r20);

		if (value_r20 & BIT(12))
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10000baseT_Full);

		rnp_mbx_phy_read(hw, (PHY_C45 | PHY_MMD_VEND2 | 0xa412),
				 &value_r412);

		if (value_r412 & BIT(8))
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     1000baseT_Full);
		if (value_r412 & BIT(9))
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     1000baseT_Full);
	}

	return 0;
}

/**
 * rnp_phy_type_to_ethtool - convert the phy_types to ethtool link modes
 * @adapter: adapter struct with hw->phy_type
 * @ks: ethtool link ksettings struct to fill out
 *
 **/
static void rnp_phy_type_to_ethtool(struct rnp_adapter *adapter,
				    struct ethtool_link_ksettings *ks)
{
	struct rnp_hw *hw = &adapter->hw;
	u32 supported_link = hw->supported_link;
	u8 phy_type = hw->phy_type;

	ethtool_link_ksettings_add_link_mode(ks, supported, Autoneg);
	ethtool_link_ksettings_add_link_mode(ks, advertising, Autoneg);

	if (phy_type == PHY_TYPE_NONE) {
		if (supported_link & RNP_LINK_SPEED_10GB_FULL) {
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     10000baseT_Full);
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10000baseT_Full);
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     10000baseSR_Full);
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10000baseSR_Full);
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     10000baseLR_Full);
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10000baseLR_Full);
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     10000baseER_Full);
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10000baseER_Full);
		}

		if (((supported_link & RNP_LINK_SPEED_10GB_FULL) ||
		     (supported_link & RNP_LINK_SPEED_1GB_FULL))) {
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     1000baseX_Full);
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     1000baseX_Full);
		}
	}
	if (phy_type == PHY_TYPE_SGMII) {
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     1000baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     100baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     100baseT_Half);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10baseT_Half);

		rnp_set_autoneg_adv_from_hw(hw, ks);
	}

	if (phy_type == PHY_TYPE_10G_TP) {
		ethtool_link_ksettings_add_link_mode(ks, supported, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     1000baseT_Full);
		rnp_set_autoneg_adv_from_hw(hw, ks);
	}

	if (rnp_fw_is_old_ethtool(hw) &&
	    (supported_link & RNP_LINK_SPEED_40GB_FULL)) {
		supported_link |= RNP_SFP_MODE_40G_CR4 | RNP_SFP_MODE_40G_SR4 |
				  PHY_TYPE_40G_BASE_LR4;
	}

	if (supported_link & RNP_SFP_MODE_40G_CR4) {
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     40000baseCR4_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     40000baseCR4_Full);
	}
	if (supported_link & RNP_SFP_MODE_40G_SR4) {
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     40000baseSR4_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     40000baseSR4_Full);
	}
	if (supported_link & RNP_SFP_MODE_40G_LR4) {
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     40000baseLR4_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     40000baseLR4_Full);
	}

	/* add 25G support here */
	if (supported_link & RNP_SFP_25G_SR) {
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     25000baseSR_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     25000baseSR_Full);
	}
	if (supported_link & RNP_SFP_25G_KR) {
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     25000baseKR_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     25000baseKR_Full);
	}
	if (supported_link & RNP_SFP_25G_CR) {
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     25000baseCR_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     25000baseCR_Full);
	}
	if (hw->is_backplane) {
		if (phy_type == PHY_TYPE_40G_BASE_KR4) {
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     40000baseKR4_Full);
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     40000baseKR4_Full);
		}
		if (phy_type == PHY_TYPE_10G_BASE_KR) {
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     10000baseKR_Full);
			if (supported_link & RNP_LINK_SPEED_10GB_FULL)
				ethtool_link_ksettings_add_link_mode(
					ks, advertising, 10000baseKR_Full);
		}
	}

	if (supported_link & RNP_SFP_MODE_1G_LX ||
	    supported_link & RNP_SFP_MODE_1G_SX) {
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     1000baseX_Full);
		if (supported_link & RNP_LINK_SPEED_1GB_FULL) {
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     1000baseX_Full);
		}
	}

	if (phy_type == PHY_TYPE_1G_BASE_KX) {
		if (hw->is_backplane) {
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     1000baseKX_Full);
			if (supported_link & RNP_LINK_SPEED_1GB_FULL)
				ethtool_link_ksettings_add_link_mode(
					ks, advertising, 1000baseKX_Full);
		}

		if ((supported_link & RNP_SFP_MODE_1G_T) ||
		    (supported_link & RNP_LINK_SPEED_1GB_FULL)) {
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     1000baseT_Full);
			if (supported_link & RNP_LINK_SPEED_1GB_FULL)
				ethtool_link_ksettings_add_link_mode(
					ks, advertising, 1000baseT_Full);
		}
	}
	/* need to add new 10G PHY types */
	if (phy_type == PHY_TYPE_10G_BASE_SR) {
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseSR_Full);
		if (supported_link & RNP_LINK_SPEED_10GB_FULL)
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10000baseSR_Full);
	}
	if (phy_type == PHY_TYPE_10G_BASE_ER) {
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseER_Full);
		if (supported_link & RNP_LINK_SPEED_10GB_FULL)
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10000baseER_Full);
	}
	if (phy_type == PHY_TYPE_10G_BASE_LR) {
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseLR_Full);
		if (supported_link & RNP_LINK_SPEED_10GB_FULL)
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10000baseLR_Full);
	}
	if (hw->force_speed_stat == FORCE_SPEED_STAT_10G) {
		ethtool_link_ksettings_del_link_mode(ks, supported,
						     1000baseT_Full);
		ethtool_link_ksettings_del_link_mode(ks, advertising,
						     1000baseT_Full);

		ethtool_link_ksettings_del_link_mode(ks, supported,
						     1000baseX_Full);
		ethtool_link_ksettings_del_link_mode(ks, advertising,
						     1000baseX_Full);

		if (phy_type == PHY_TYPE_1G_BASE_KX) {
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     10000baseSR_Full);
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10000baseSR_Full);
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     10000baseLR_Full);
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10000baseLR_Full);
		}
	}
}
/**
 * rnp_get_settings_link_up - Get Link settings for when link is up
 * @hw: hw structure
 * @ks: ethtool ksettings to fill in
 * @netdev: network interface device structure
 **/
static void rnp_get_settings_link_up(struct rnp_hw *hw,
				     struct ethtool_link_ksettings *ks,
				     struct net_device *netdev)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct ethtool_link_ksettings cap_ksettings;

	/* Initialize supported and advertised settings based on phy settings */
	switch (hw->phy_type) {
	case PHY_TYPE_40G_BASE_CR4:
		ethtool_link_ksettings_add_link_mode(ks, supported, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     40000baseCR4_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     40000baseCR4_Full);
		break;

	case PHY_TYPE_40G_BASE_SR4:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     40000baseSR4_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     40000baseSR4_Full);
		break;
	case PHY_TYPE_40G_BASE_LR4:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     40000baseLR4_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     40000baseLR4_Full);
		break;
	case PHY_TYPE_10G_BASE_SR:
	case PHY_TYPE_10G_BASE_LR:
	case PHY_TYPE_10G_BASE_ER:
		ethtool_link_ksettings_add_link_mode(ks, supported, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, advertising, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseSR_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     10000baseSR_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseLR_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     10000baseLR_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseER_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     10000baseER_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     1000baseX_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     1000baseX_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseT_Full);
		if (hw->speed == SPEED_10000)
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     10000baseT_Full);
		break;
	case PHY_TYPE_1G_BASE_KX:
		ethtool_link_ksettings_add_link_mode(ks, supported, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, advertising, Autoneg);
		if (!!hw->is_backplane) {
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     1000baseKX_Full);
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     1000baseKX_Full);
		}
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     1000baseX_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     1000baseX_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     1000baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     1000baseT_Full);
		break;

	case PHY_TYPE_SGMII:
		ethtool_link_ksettings_add_link_mode(ks, supported, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, advertising, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     1000baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     100baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     1000baseT_Half);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     100baseT_Half);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10baseT_Half);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     1000baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     100baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     10baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     1000baseT_Half);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     100baseT_Half);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     10baseT_Half);
		break;

	case PHY_TYPE_40G_BASE_KR4:
	case PHY_TYPE_10G_BASE_KR:
		ethtool_link_ksettings_add_link_mode(ks, supported, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, advertising, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     40000baseKR4_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseKR_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     1000baseKX_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseKX4_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     25000baseKR_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     25000baseSR_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     25000baseCR_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     40000baseKR4_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     10000baseKR_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     10000baseKX4_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     1000baseKX_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     25000baseKR_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     25000baseSR_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     25000baseCR_Full);
		break;
	case PHY_TYPE_10G_TP:
		ethtool_link_ksettings_add_link_mode(ks, supported, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, advertising, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     1000baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     10000baseT_Full);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     1000baseT_Full);
		break;

	default:
		/* if we got here and link is up something bad is afoot
		 */
		netdev_info(netdev,
			    "WARNING: Link is up but PHY type 0x%x is not "
			    "recognized, or incorrect cable is in use\n",
			    hw->phy_type);
	}

	/* Now that we've worked out everything that could be supported by the
	 * current PHY type, get what is supported by the NVM and intersect
	 * them to get what is truly supported
	 */
	memset(&cap_ksettings, 0, sizeof(struct ethtool_link_ksettings));
	rnp_phy_type_to_ethtool(adapter, &cap_ksettings);
	ethtool_intersect_link_masks(ks, &cap_ksettings);

	/* Set speed and duplex */
	ks->base.speed = adapter->speed;
	ks->base.duplex = hw->duplex;
}

/**
 * rnp_get_settings_link_down - Get the Link settings when link is down
 * @hw: hw structure
 * @ks: ethtool ksettings to fill in
 * @netdev: network interface device structure
 *
 * Reports link settings that can be determined when link is down
 **/
static void rnp_get_settings_link_down(struct rnp_hw *hw,
				       struct ethtool_link_ksettings *ks,
				       struct net_device *netdev)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);

	/* link is down and the driver needs to fall back on
	 * supported phy types to figure out what info to display
	 */
	rnp_phy_type_to_ethtool(adapter, ks);

	/* With no link speed and duplex are unknown */
	ks->base.speed = SPEED_UNKNOWN;
	ks->base.duplex = DUPLEX_UNKNOWN;

	if ((hw->phy_type == PHY_TYPE_SGMII) ||
	    (hw->phy_type == PHY_TYPE_10G_TP)) {
		ks->base.eth_tp_mdix_ctrl = ETH_TP_MDI_INVALID;
		ks->base.eth_tp_mdix_ctrl = hw->tp_mdix_ctrl;
	}
}

/**
 * rnp_set_autoneg_state_from_hw - Set the autoneg state from hardware
 * @hw: hw structure
 * @ks: ethtool ksettings to fill in
 *
 * Set the autoneg state from hardware, like PHY
 **/
static int rnp_set_autoneg_state_from_hw(struct rnp_hw *hw,
					 struct ethtool_link_ksettings *ks)
{
	int ret;
	struct rnp_adapter *adapter = hw->back;

	ks->base.autoneg = (adapter->an ? AUTONEG_ENABLE : AUTONEG_DISABLE);

	/* Read autoneg state from phy */
	if (hw->phy_type == PHY_TYPE_SGMII) {
		u32 value_r0 = 0;
		ret = rnp_mbx_phy_read(hw, 0x0, &value_r0);
		if (ret)
			return -1;

		ks->base.autoneg = (value_r0 & BIT(12)) ? AUTONEG_ENABLE :
							  AUTONEG_DISABLE;
	}
	if (hw->phy_type == PHY_TYPE_10G_TP) {
		u32 value_r0 = 0;

		rnp_mbx_phy_read(hw, PHY_826x_AN, &value_r0);

		ks->base.autoneg = (value_r0 & BIT(12)) ? AUTONEG_ENABLE :
							  AUTONEG_DISABLE;
		if (value_r0)
			adapter->an = 1;
	}

	return 0;
}

static int rnp_get_phy_mdix_from_hw(struct rnp_hw *hw)
{
	int ret;
	int rmmd_reg = 0;
	u32 value_r17 = 0;

	if (hw->phy_type == PHY_TYPE_SGMII) {
		ret = rnp_mbx_phy_read(hw, 0x11, &value_r17);
		if (ret)
			return -1;
		hw->phy.is_mdix = !!(value_r17 & 0x0040);
	}
	if (hw->phy_type == PHY_TYPE_10G_TP) {
		rmmd_reg = (1 << 30) | (0x1f << 16) | (0xa430 & 0xffff);
		ret = rnp_mbx_phy_read(hw, rmmd_reg, &value_r17);
		if (ret)
			return -1;
		hw->phy.is_mdix = !!(value_r17 & 0x0200);
	}

	return 0;
}

__maybe_unused static bool fiber_unsupport(u32 supported_link, u8 phy_type)
{
	if ((phy_type == PHY_TYPE_10G_BASE_KR) ||
	    (phy_type == PHY_TYPE_10G_BASE_SR) ||
	    (phy_type == PHY_TYPE_10G_BASE_LR) ||
	    (phy_type == PHY_TYPE_10G_BASE_ER)) {
		if (!(supported_link & RNP_LINK_SPEED_10GB_FULL))
			return true;
	}

	if ((phy_type == PHY_TYPE_40G_BASE_KR4) ||
	    (phy_type == PHY_TYPE_40G_BASE_SR4) ||
	    (phy_type == PHY_TYPE_40G_BASE_CR4) ||
	    (phy_type == PHY_TYPE_40G_BASE_LR4)) {
		if (!(supported_link &
		      (RNP_LINK_SPEED_40GB_FULL | RNP_LINK_SPEED_25GB_FULL)))
			return true;
	}

	if (phy_type == PHY_TYPE_1G_BASE_KX) {
		if (!(supported_link & RNP_LINK_SPEED_1GB_FULL))
			return true;
	}

	return false;
}

static int rnp10_get_link_ksettings(struct net_device *netdev,
			     struct ethtool_link_ksettings *ks)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	bool link_up;
	int err;

	ethtool_link_ksettings_zero_link_mode(ks, supported);
	ethtool_link_ksettings_zero_link_mode(ks, advertising);

	/* update hw from firmware */
	err = rnp_mbx_get_lane_stat(hw);
	if (err /*|| fiber_unsupport(hw->supported_link, hw->phy_type)*/) {
		/*
					when force 1G speed and plugin in 10G-AOC, should not return
				   -1
				*/
		return -1;
	}

	/* update hw->phy.media_type by hw->phy_type */
	switch (hw->phy_type) {
	case PHY_TYPE_NONE:
		hw->phy.media_type = rnp_media_type_unknown;
		break;
	case PHY_TYPE_1G_BASE_KX:
		if (hw->is_backplane) {
			hw->phy.media_type = rnp_media_type_backplane;
		} else if (hw->is_sgmii) {
			hw->phy.media_type = rnp_media_type_copper;
		} else {
			if ((hw->supported_link & RNP_LINK_SPEED_1GB_FULL) ||
			    (hw->supported_link & RNP_SFP_MODE_1G_LX)) {
				hw->phy.media_type = rnp_media_type_fiber;
			} else {
				hw->phy.media_type = rnp_media_type_unknown;
			}
		}
		break;
	case PHY_TYPE_SGMII:
	case PHY_TYPE_10G_TP:
		hw->phy.media_type = rnp_media_type_copper;
		ks->base.phy_address = adapter->phy_addr;
		break;
	case PHY_TYPE_10G_BASE_KR:
	case PHY_TYPE_25G_BASE_KR:
	case PHY_TYPE_40G_BASE_KR4:
		hw->phy.media_type = rnp_media_type_backplane;
		break;
	case PHY_TYPE_10G_BASE_SR:
	case PHY_TYPE_40G_BASE_SR4:
	case PHY_TYPE_40G_BASE_CR4:
	case PHY_TYPE_40G_BASE_LR4:
	case PHY_TYPE_10G_BASE_LR:
	case PHY_TYPE_10G_BASE_ER:
		hw->phy.media_type = rnp_media_type_fiber;
		break;
	default:
		hw->phy.media_type = rnp_media_type_unknown;
		break;
	}

	if (hw->supported_link & RNP_SFP_CONNECTOR_DAC) {
		hw->phy.media_type = rnp_media_type_da;
	}

	if ((hw->supported_link & RNP_SFP_TO_SGMII) ||
	    (hw->supported_link & RNP_SFP_MODE_1G_T)) {
		hw->phy.media_type = rnp_media_type_copper;
	}

	/* Check Whether there is media on port */
	if (hw->phy.media_type == rnp_media_type_fiber) {
		/* If adapter->sfp.mod_abs is 0, there is no media on port. */
		if (!adapter->sfp.mod_abs) {
			hw->phy.media_type = rnp_media_type_unknown;
			hw->phy_type = PHY_TYPE_NONE;
		}
	}

	/* Now set the settings that don't rely on link being up/down */
	/* Set autoneg settings */
	rnp_set_autoneg_state_from_hw(hw, ks);

	link_up = hw->link;
	if (link_up)
		rnp_get_settings_link_up(hw, ks, netdev);
	else
		rnp_get_settings_link_down(hw, ks, netdev);

	/* Set media type settings */
	switch (hw->phy.media_type) {
	case rnp_media_type_backplane:
		ethtool_link_ksettings_add_link_mode(ks, supported, Backplane);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     Backplane);
		ethtool_link_ksettings_add_link_mode(ks, supported, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, advertising, Autoneg);
		ks->base.port = PORT_NONE;
		break;
	case rnp_media_type_copper:
		ethtool_link_ksettings_add_link_mode(ks, supported, TP);
		ethtool_link_ksettings_add_link_mode(ks, advertising, TP);
		if (PHY_TYPE_SGMII == hw->phy_type)
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     Autoneg);
		if (AUTONEG_ENABLE == ks->base.autoneg)
			ethtool_link_ksettings_add_link_mode(ks, advertising,
							     Autoneg);
		else
			ethtool_link_ksettings_del_link_mode(ks, advertising,
							     Autoneg);
		ks->base.port = PORT_TP;
		break;
	case rnp_media_type_da:
	case rnp_media_type_cx4:
		ethtool_link_ksettings_add_link_mode(ks, supported, FIBRE);
		ethtool_link_ksettings_add_link_mode(ks, advertising, FIBRE);
		ks->base.port = PORT_DA;
		break;
	case rnp_media_type_fiber:
		ethtool_link_ksettings_add_link_mode(ks, supported, FIBRE);
		ethtool_link_ksettings_add_link_mode(ks, advertising, FIBRE);
		ks->base.port = PORT_FIBRE;
		break;
	case rnp_media_type_unknown:
	default:
		ethtool_link_ksettings_add_link_mode(ks, supported, Autoneg);
		ethtool_link_ksettings_add_link_mode(ks, advertising, Autoneg);
		ks->base.port = PORT_OTHER;
		break;
	}

	if (hw->force_speed_stat != FORCE_SPEED_STAT_DISABLED) {
		ethtool_link_ksettings_del_link_mode(ks, advertising, Autoneg);
	}

	/* Set flow control settings */
	ethtool_link_ksettings_add_link_mode(ks, supported, Pause);
	ethtool_link_ksettings_add_link_mode(ks, supported, Asym_Pause);

	/* should get pause from hw if 10G-TP */
	switch (hw->fc.requested_mode) {
	case rnp_fc_full:
		ethtool_link_ksettings_add_link_mode(ks, advertising, Pause);
		break;
	case rnp_fc_tx_pause:
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     Asym_Pause);
		break;
	case rnp_fc_rx_pause:
		ethtool_link_ksettings_add_link_mode(ks, advertising, Pause);
		ethtool_link_ksettings_add_link_mode(ks, advertising,
						     Asym_Pause);
		break;
	default:
		ethtool_link_ksettings_del_link_mode(ks, advertising, Pause);
		ethtool_link_ksettings_del_link_mode(ks, advertising,
						     Asym_Pause);
		break;
	}
	/* MDI-X => 2; MDI =>1; Invalid =>0 */
	if ((hw->phy_type == PHY_TYPE_SGMII) ||
	    (hw->phy_type == PHY_TYPE_10G_TP)) {
		if (rnp_get_phy_mdix_from_hw(hw)) {
			ks->base.eth_tp_mdix = ETH_TP_MDI_INVALID;
		} else {
			ks->base.eth_tp_mdix = hw->phy.is_mdix ? ETH_TP_MDI_X :
								 ETH_TP_MDI;
		}
	} else {
		ks->base.eth_tp_mdix = hw->tp_mdx;
	}

	if (hw->phy.mdix == AUTO_ALL_MODES)
		ks->base.eth_tp_mdix_ctrl = ETH_TP_MDI_AUTO;
	else
		ks->base.eth_tp_mdix_ctrl = hw->phy.mdix;

	rnp_logd(LOG_ETHTOOL,
		 "%s %s set link: speed=%d port=%d duplex=%d autoneg=%d "
		 "phy_address=%d, media_type=%d hw->phy_type:%d\n",
		 __func__, netdev->name, ks->base.speed, ks->base.port,
		 ks->base.duplex, ks->base.autoneg, ks->base.phy_address,
		 hw->phy.media_type, hw->phy_type);
	return 0;
}

static int rnp10_set_link_ksettings(struct net_device *netdev,
			     const struct ethtool_link_ksettings *ks)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	struct ethtool_link_ksettings safe_ks;
	struct ethtool_link_ksettings copy_ks;
	bool autoneg_changed = false, duplex_changed = false;
	int timeout = 50;
	int err = 0;
	u8 autoneg;
	u32 advertising_link_speed, speed = 0;

	/* copy the ksettings to copy_ks to avoid modifying the origin */
	memcpy(&copy_ks, ks, sizeof(struct ethtool_link_ksettings));

	/* save autoneg out of ksettings */

	autoneg = copy_ks.base.autoneg;
	rnp_logd(LOG_ETHTOOL,
		 "%s %s set link: speed=%d port=%d duplex=%d autoneg=%d "
		 "phy_address=%d\n",
		 __func__, netdev->name, copy_ks.base.speed, copy_ks.base.port,
		 copy_ks.base.duplex, copy_ks.base.autoneg,
		 copy_ks.base.phy_address);

	/* get our own copy of the bits to check against */
	memset(&safe_ks, 0, sizeof(struct ethtool_link_ksettings));
	safe_ks.base.cmd = copy_ks.base.cmd;
	safe_ks.base.link_mode_masks_nwords =
		copy_ks.base.link_mode_masks_nwords;

	if (rnp10_get_link_ksettings(netdev, &safe_ks)) {
		/* return err */
		return 0;
	}
	/* Get link modes supported by hardware and check against modes
	 * requested by user.  Return an error if unsupported mode was set.
	 */
	/* if autoneg is off, this is not error ? */
	if (!bitmap_subset(copy_ks.link_modes.advertising,
			   safe_ks.link_modes.supported,
			   __ETHTOOL_LINK_MODE_MASK_NBITS)) {
		return -EINVAL;
	}
	/* set autoneg back to what it currently is */
	copy_ks.base.autoneg = safe_ks.base.autoneg;

	memset(&advertising_link_speed, 0, sizeof(u32));

	/* Check autoneg */
	if (autoneg == AUTONEG_ENABLE) {
		/* If autoneg was not already enabled */
		if (!(adapter->an)) {
			/* If autoneg is not supported, return error */
			if (!ethtool_link_ksettings_test_link_mode(
				    &safe_ks, supported, Autoneg)) {
				netdev_info(
					netdev,
					"Autoneg not supported on this phy\n");
				err = -EINVAL;
				goto done;
			}
			/* Autoneg is allowed to change */
			autoneg_changed = true;
		}

		if (ethtool_link_ksettings_test_link_mode(ks, advertising,
							  10baseT_Full))
			advertising_link_speed |= RNP_LINK_SPEED_10_FULL;
		if (ethtool_link_ksettings_test_link_mode(ks, advertising,
							  100baseT_Full))
			advertising_link_speed |= RNP_LINK_SPEED_100_FULL;
		if (ethtool_link_ksettings_test_link_mode(ks, advertising,
							  1000baseT_Full) ||
		    ethtool_link_ksettings_test_link_mode(ks, advertising,
							  1000baseX_Full) ||
		    ethtool_link_ksettings_test_link_mode(ks, advertising,
							  1000baseKX_Full))
			advertising_link_speed |= RNP_LINK_SPEED_1GB_FULL;

		if (ethtool_link_ksettings_test_link_mode(ks, advertising,
							  10baseT_Half))
			advertising_link_speed |= RNP_LINK_SPEED_10_HALF;
		if (ethtool_link_ksettings_test_link_mode(ks, advertising,
							  100baseT_Half))
			advertising_link_speed |= RNP_LINK_SPEED_100_HALF;
		if (ethtool_link_ksettings_test_link_mode(ks, advertising,
							  1000baseT_Half))
			advertising_link_speed |= RNP_LINK_SPEED_1GB_HALF;
		if (ethtool_link_ksettings_test_link_mode(ks, advertising,
							  10000baseT_Full) ||
		    ethtool_link_ksettings_test_link_mode(ks, advertising,
							  10000baseKX4_Full) ||
		    ethtool_link_ksettings_test_link_mode(ks, advertising,
							  10000baseKR_Full) ||
		    ethtool_link_ksettings_test_link_mode(ks, advertising,
							  10000baseCR_Full) ||
		    ethtool_link_ksettings_test_link_mode(ks, advertising,
							  10000baseSR_Full) ||
		    ethtool_link_ksettings_test_link_mode(ks, advertising,
							  10000baseLR_Full))
			advertising_link_speed |= RNP_LINK_SPEED_10GB_FULL;

		if (ethtool_link_ksettings_test_link_mode(ks, advertising,
							  40000baseKR4_Full) ||
		    ethtool_link_ksettings_test_link_mode(ks, advertising,
							  40000baseCR4_Full) ||
		    ethtool_link_ksettings_test_link_mode(ks, advertising,
							  40000baseSR4_Full) ||
		    ethtool_link_ksettings_test_link_mode(ks, advertising,
							  40000baseLR4_Full))
			advertising_link_speed |= RNP_LINK_SPEED_40GB_FULL;

		if (advertising_link_speed) {
			hw->phy.autoneg_advertised = advertising_link_speed;
		} else {
			if ((hw->force_speed_stat ==
			     FORCE_SPEED_STAT_DISABLED)) {
				netdev_info(netdev,
					    "advertising_link_speed is 0\n");
				err = -EINVAL;
				goto done;
			}
		}

		hw->advertised_link = advertising_link_speed;
		if (hw->is_sgmii && hw->autoneg == false)
			autoneg_changed = true;
		hw->autoneg = true;
	} else {
		/* If autoneg is currently enabled */
		if (adapter->an) {
			/* If autoneg is supported 10GBASE_T is the only PHY
			 * that can disable it, so otherwise return error
			 */
			if (ethtool_link_ksettings_test_link_mode(
				    &safe_ks, supported, Autoneg) &&
			    hw->phy.media_type != rnp_media_type_copper) {
				netdev_info(
					netdev,
					"Autoneg cannot be disabled on this phy\n");
				err = -EINVAL;
				goto done;
			}
			/* Autoneg is allowed to change */
			autoneg_changed = true;
		}
		/* if 10G -TP, not support close an */
		if (hw->phy_type == PHY_TYPE_10G_TP) {
			netdev_info(netdev,
				    "Autoneg cannot be disabled on this phy\n");
			err = -EINVAL;
			goto done;
		}

		/* Only allow one speed at a time when autoneg is AUTONEG_DISABLE. */
		switch (ks->base.speed) {
		case SPEED_10:
			speed = RNP_LINK_SPEED_10_FULL;
			break;
		case SPEED_100:
			speed = RNP_LINK_SPEED_100_FULL;
			break;
		case SPEED_1000:
			speed = RNP_LINK_SPEED_1GB_FULL;
			break;
		case SPEED_10000:
			speed = RNP_LINK_SPEED_10GB_FULL;
			break;
		default:
			netdev_info(netdev, "unsupported speed\n");
			err = -EINVAL;
			goto done;
		}

		hw->autoneg = false;
	}

	hw->phy.autoneg_advertised = RNP_LINK_SPEED_UNKNOWN;
	/* If speed didn't get set, set it to what it currently is.
	 * This is needed because if advertise is 0 (as it is when autoneg
	 * is disabled) then speed won't get set.
	 */

	if (hw->is_sgmii) {
		hw->duplex = ks->base.duplex;
		duplex_changed = true;
	}

	if (hw->phy_type == PHY_TYPE_10G_TP) {
		hw->duplex = ks->base.duplex;
		duplex_changed = true;
	}
	/* this sets the link speed and restarts auto-neg */
	while (test_and_set_bit(__RNP_IN_SFP_INIT, &adapter->state)) {
		timeout--;
		if (!timeout)
			return -EBUSY;
		usleep_range(1000, 2000);
	}
	/* MDI-X => 2; MDI => 1; Auto => 3 */
	if (copy_ks.base.eth_tp_mdix_ctrl) {
		/* fix up the value for auto (3 => 0) as zero is mapped
		 * internally to auto
		 */
		if (copy_ks.base.eth_tp_mdix_ctrl == ETH_TP_MDI_AUTO)
			hw->phy.mdix = AUTO_ALL_MODES;
		else
			hw->phy.mdix = copy_ks.base.eth_tp_mdix_ctrl;
	}

	hw->mac.autotry_restart = true;
	/* set speed */
	err = hw->ops.setup_link(hw, advertising_link_speed, hw->autoneg, speed,
				 hw->duplex);
	if (err)
		e_info(probe, "setup link failed with code %d\n", err);

	clear_bit(__RNP_IN_SFP_INIT, &adapter->state);
done:
	return err;
}

static void rnp10_get_drvinfo(struct net_device *netdev,
			      struct ethtool_drvinfo *drvinfo)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	strncpy(drvinfo->driver, rnp_driver_name, sizeof(drvinfo->driver));
	snprintf(drvinfo->version, sizeof(drvinfo->version), "%s",
		 rnp_driver_version);

	snprintf(drvinfo->fw_version, sizeof(drvinfo->fw_version),
		 "%d.%d.%d.%d", ((unsigned char *)&(hw->fw_version))[3],
		 ((unsigned char *)&(hw->fw_version))[2],
		 ((unsigned char *)&(hw->fw_version))[1],
		 ((unsigned char *)&(hw->fw_version))[0]);

	strncpy(drvinfo->bus_info, pci_name(adapter->pdev),
		sizeof(drvinfo->bus_info));
	drvinfo->n_stats = RNP10_STATS_LEN;
	drvinfo->testinfo_len = RNP10_TEST_LEN;
	drvinfo->regdump_len = rnp10_get_regs_len(netdev);
	drvinfo->n_priv_flags = RNP10_PRIV_FLAGS_STR_LEN;
}

static void rnp10_get_regs(struct net_device *netdev, struct ethtool_regs *regs,
			   void *p)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	u32 *regs_buff = p;
	int i;

	memset(p, 0, RNP10_REGS_LEN * sizeof(u32));

	for (i = 0; i < RNP10_REGS_LEN; i++)
		regs_buff[i] = rd32(hw, i * 4);
}

static int rnp_nway_reset(struct net_device *netdev)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	netdev_info(netdev, "NIC Link is Down\n");
	rnp_down(adapter);
	msleep(10);
	rnp_up(adapter);
	return 0;
}

/**
 *  rnpm_device_supports_autoneg_fc - Check if phy supports autoneg flow
 *  control
 *  @hw: pointer to hardware structure
 *
 *  There are several phys that do not support autoneg flow control. This
 *  function check the device id to see if the associated phy supports
 *  autoneg flow control.
 **/
static bool rnp_device_supports_autoneg_fc(struct rnp_hw *hw)
{
	bool supported = false;

	switch (hw->phy.media_type) {
	case rnp_media_type_fiber:
		break;
	case rnp_media_type_backplane:
		break;
	case rnp_media_type_copper:
		/* only some copper devices support flow control autoneg */
		supported = true;
		break;
	default:
		break;
	}

	return supported;
}

static void rnp10_get_pauseparam(struct net_device *netdev,
				 struct ethtool_pauseparam *pause)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	/* we don't support autoneg */
	if (rnp_device_supports_autoneg_fc(hw) && !hw->fc.disable_fc_autoneg)
		pause->autoneg = 1;
	else
		pause->autoneg = 0;
	if (hw->fc.current_mode == rnp_fc_rx_pause) {
		pause->rx_pause = 1;
	} else if (hw->fc.current_mode == rnp_fc_tx_pause) {
		pause->tx_pause = 1;
	} else if (hw->fc.current_mode == rnp_fc_full) {
		pause->rx_pause = 1;
		pause->tx_pause = 1;
	}
}

static int rnp10_set_pauseparam(struct net_device *netdev,
				struct ethtool_pauseparam *pause)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_fc_info fc = hw->fc;

	/* we not support change in dcb mode */
	if (adapter->flags & RNP_FLAG_DCB_ENABLED)
		return -EINVAL;

	/* we not support autoneg mode */
	if ((pause->autoneg == AUTONEG_ENABLE) &&
	    !rnp_device_supports_autoneg_fc(hw))
		return -EINVAL;

	fc.disable_fc_autoneg = (pause->autoneg != AUTONEG_ENABLE);
	fc.requested_mode &= (~(PAUSE_TX | PAUSE_RX));
	if (pause->autoneg) {
		fc.requested_mode |= PAUSE_AUTO;
	} else {
		if (pause->tx_pause)
			fc.requested_mode |= PAUSE_TX;
		if (pause->rx_pause)
			fc.requested_mode |= PAUSE_RX;
	}

	if (hw->phy_type == PHY_TYPE_SGMII) {
		u16 pause_bits = 0;
		u32 value;
		u32 value_r0;

		if (hw->fc.requested_mode == PAUSE_AUTO) {
			pause_bits |= ASYM_PAUSE | SYM_PAUSE;
		} else {
			if ((hw->fc.requested_mode & PAUSE_TX) &&
			    (!(hw->fc.requested_mode & PAUSE_RX))) {
				pause_bits |= ASYM_PAUSE;

			} else if ((!(hw->fc.requested_mode & PAUSE_TX)) &&
				   (!(hw->fc.requested_mode & PAUSE_RX))) {
			} else
				pause_bits |= ASYM_PAUSE | SYM_PAUSE;
		}
		rnp_mbx_phy_read(hw, 4, &value);
		value &= ~0xC00;
		value |= pause_bits;
		rnp_mbx_phy_write(hw, 4, value);

		if (hw->autoneg) {
			rnp_mbx_phy_read(hw, 0, &value_r0);
			value_r0 |= BIT(9);
			rnp_mbx_phy_write(hw, 0, value_r0);
		}
	}

	/* if the thing changed then we'll update and use new autoneg */
	if (memcmp(&fc, &hw->fc, sizeof(struct rnp_fc_info))) {
		/* to tell all vf new pause status */
		hw->fc = fc;
		rnp_msg_post_status(adapter, PF_PAUSE_STATUS);
		if (netif_running(netdev))
			rnp_reinit_locked(adapter);
		else
			rnp_reset(adapter);
	}

	return 0;
}

static void rnp10_get_strings(struct net_device *netdev, u32 stringset,
			      u8 *data)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	char *p = (char *)data;
	int i;
	struct rnp_ring *ring;
	u32 dma_ch;

	switch (stringset) {
	/* maybe we don't support test? */
	case ETH_SS_TEST:
		for (i = 0; i < RNP10_TEST_LEN; i++) {
			memcpy(data, rnp10_gstrings_test[i], ETH_GSTRING_LEN);
			data += ETH_GSTRING_LEN;
		}
		break;
	case ETH_SS_STATS:
		for (i = 0; i < RNP10_GLOBAL_STATS_LEN; i++) {
			memcpy(p, rnp10_gstrings_net_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		for (i = 0; i < RNP10_HWSTRINGS_STATS_LEN; i++) {
			memcpy(p, rnp10_hwstrings_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		for (i = 0; i < RNP_NUM_TX_QUEUES; i++) {
			/* ====  tx ======== */
			ring = adapter->tx_ring[i];
			dma_ch = ring->rnp_queue_idx;
			sprintf(p, "---\n     queue%u_tx_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_bytes", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_restart", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_busy", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_done_old", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_clean_desc", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_poll_count", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_irq_more", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_hw_head", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_hw_tail", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_sw_next_to_clean", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_sw_next_to_use", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_send_bytes", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_send_bytes_to_hw", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_todo_update", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_send_done_bytes", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_added_vlan_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_next_to_clean", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_irq_miss", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_equal_count", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_clean_times", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_clean_count", i);
			p += ETH_GSTRING_LEN;

			/* ====  rx ======== */
			ring = adapter->rx_ring[i];
			dma_ch = ring->rnp_queue_idx;
			sprintf(p, "queue%u_rx_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_bytes", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_driver_drop_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_rsc", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_rsc_flush", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_non_eop_descs", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_alloc_page_failed", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_alloc_buff_failed", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_alloc_page", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_csum_offload_errs", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_csum_offload_good", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_poll_again_count", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_rm_vlan_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_hw_head", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_hw_tail", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_sw_next_to_use", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_sw_next_to_clean", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_next_to_clean", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_irq_miss", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_equal_count", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_clean_times", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_clean_count", i);
			p += ETH_GSTRING_LEN;
		}

		break;
	case ETH_SS_PRIV_FLAGS:
		memcpy(data, rnp10_priv_flags_strings,
		       RNP10_PRIV_FLAGS_STR_LEN * ETH_GSTRING_LEN);
		break;
	}
}

static int rnp10_get_sset_count(struct net_device *netdev, int sset)
{
	switch (sset) {
	/* now we don't support test */
	case ETH_SS_TEST:
		return RNP10_TEST_LEN;
	case ETH_SS_STATS:
		return RNP10_STATS_LEN;
	case ETH_SS_PRIV_FLAGS:
		return RNP10_PRIV_FLAGS_STR_LEN;
	default:
		return -EOPNOTSUPP;
	}
}

static u32 rnp10_get_priv_flags(struct net_device *netdev)
{
	struct rnp_adapter *adapter = (struct rnp_adapter *)netdev_priv(netdev);
	u32 priv_flags = 0;

	if (adapter->priv_flags & RNP_PRIV_FLAG_MAC_LOOPBACK)
		priv_flags |= RNP10_MAC_LOOPBACK;
	if (adapter->priv_flags & RNP_PRIV_FLAG_SWITCH_LOOPBACK)
		priv_flags |= RNP10_SWITCH_LOOPBACK;
	if (adapter->priv_flags & RNP_PRIV_FLAG_VEB_ENABLE)
		priv_flags |= RNP10_VEB_ENABLE;
	if (adapter->priv_flags & RNP_PRIV_FLAG_FT_PADDING)
		priv_flags |= RNP10_FT_PADDING;
	if (adapter->priv_flags & RNP_PRIV_FLAG_PADDING_DEBUG)
		priv_flags |= RNP10_PADDING_DEBUG;
	if (adapter->priv_flags & RNP_PRIV_FLAG_PTP_DEBUG)
		priv_flags |= RNP10_PTP_FEATURE;
	if (adapter->priv_flags & RNP_PRIV_FLAG_SIMUATE_DOWN)
		priv_flags |= RNP10_SIMULATE_DOWN;
	if (adapter->priv_flags & RNP_PRIV_FLAG_VXLAN_INNER_MATCH)
		priv_flags |= RNP10_VXLAN_INNER_MATCH;
	if (adapter->flags2 & RNP_FLAG2_VLAN_STAGS_ENABLED)
		priv_flags |= RNP10_STAG_ENABLE;
	if (adapter->priv_flags & RNP_PRIV_FLAG_REC_HDR_LEN_ERR)
		priv_flags |= RNP10_REC_HDR_LEN_ERR;
	if (adapter->priv_flags & RNP_PRIV_FLAG_SRIOV_VLAN_MODE)
		priv_flags |= RNP10_SRIOV_VLAN_MODE;
	if (adapter->priv_flags & RNP_PRIV_FLAG_REMAP_MODE)
		priv_flags |= RNP10_REMAP_MODE;
	if (adapter->priv_flags & RNP_PRIV_FLAG_LLDP_EN_STAT)
		priv_flags |= RNP10_LLDP_EN_STAT;
	if (adapter->priv_flags & RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE)
		priv_flags |= RNP10_FORCE_CLOSE;

	return priv_flags;
}

static int rnp10_set_priv_flags(struct net_device *netdev, u32 priv_flags)
{
	struct rnp_adapter *adapter = (struct rnp_adapter *)netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_dma_info *dma = &hw->dma;
	struct rnp_eth_info *eth = &hw->eth;
	u32 data_old;
	u32 data_new;

	data_old = dma_rd32(dma, RNP_DMA_CONFIG);
	data_new = data_old;

	if (priv_flags & RNP10_MAC_LOOPBACK) {
		SET_BIT(n10_mac_loopback, data_new);
		adapter->priv_flags |= RNP_PRIV_FLAG_MAC_LOOPBACK;
	} else if (adapter->priv_flags & RNP_PRIV_FLAG_MAC_LOOPBACK) {
		adapter->priv_flags &= (~RNP_PRIV_FLAG_MAC_LOOPBACK);
		CLR_BIT(n10_mac_loopback, data_new);
	}

	if (priv_flags & RNP10_LLDP_EN_STAT) {
		if (rnp_mbx_lldp_port_enable(hw, true) == 0) {
			adapter->priv_flags |= RNP_PRIV_FLAG_LLDP_EN_STAT;
		} else {
			rnp_err("%s: set lldp enable faild!\n",
				adapter->netdev->name);
			adapter->priv_flags &= (~RNP_PRIV_FLAG_LLDP_EN_STAT);
		}
	} else if (adapter->priv_flags & RNP_PRIV_FLAG_LLDP_EN_STAT) {
		adapter->priv_flags &= (~RNP_PRIV_FLAG_LLDP_EN_STAT);
		rnp_mbx_lldp_port_enable(hw, false);
	}

	if (priv_flags & RNP10_SWITCH_LOOPBACK) {
		SET_BIT(n10_switch_loopback, data_new);
		adapter->priv_flags |= RNP_PRIV_FLAG_SWITCH_LOOPBACK;
	} else if (adapter->priv_flags & RNP_PRIV_FLAG_SWITCH_LOOPBACK) {
		adapter->priv_flags &= (~RNP_PRIV_FLAG_SWITCH_LOOPBACK);
		CLR_BIT(n10_switch_loopback, data_new);
	}

	if (priv_flags & RNP10_VEB_ENABLE) {
		SET_BIT(n10_veb_enable, data_new);
		adapter->priv_flags |= RNP_PRIV_FLAG_VEB_ENABLE;
	} else if (adapter->priv_flags & RNP_PRIV_FLAG_VEB_ENABLE) {
		adapter->priv_flags &= (~RNP_PRIV_FLAG_VEB_ENABLE);
		CLR_BIT(n10_veb_enable, data_new);
	}

	if (priv_flags & RNP10_FT_PADDING) {
		SET_BIT(n10_padding_enable, data_new);
		adapter->priv_flags |= RNP_PRIV_FLAG_FT_PADDING;
	} else if (adapter->priv_flags & RNP_PRIV_FLAG_FT_PADDING) {
		adapter->priv_flags &= (~RNP_PRIV_FLAG_FT_PADDING);
		CLR_BIT(n10_padding_enable, data_new);
	}

	if (priv_flags & RNP10_PADDING_DEBUG)
		adapter->priv_flags |= RNP_PRIV_FLAG_PADDING_DEBUG;
	else if (adapter->priv_flags & RNP_PRIV_FLAG_PADDING_DEBUG)
		adapter->priv_flags &= (~RNP_PRIV_FLAG_PADDING_DEBUG);

	if (priv_flags & RNP10_PTP_FEATURE) {
		adapter->priv_flags |= RNP_PRIV_FLAG_PTP_DEBUG;
		adapter->flags2 |= ~RNP_FLAG2_PTP_ENABLED;
	} else if (adapter->priv_flags & RNP_PRIV_FLAG_PTP_DEBUG) {
		adapter->priv_flags &= (~RNP_PRIV_FLAG_PTP_DEBUG);
		adapter->flags2 &= (~RNP_FLAG2_PTP_ENABLED);
	}

	if (priv_flags & RNP10_SIMULATE_DOWN) {
		adapter->priv_flags |= RNP_PRIV_FLAG_SIMUATE_DOWN;
		/* set check link again */
		adapter->flags |= RNP_FLAG_NEED_LINK_UPDATE;
	} else if (adapter->priv_flags & RNP_PRIV_FLAG_SIMUATE_DOWN) {
		adapter->priv_flags &= (~RNP_PRIV_FLAG_SIMUATE_DOWN);
		/* set check link again */
		adapter->flags |= RNP_FLAG_NEED_LINK_UPDATE;
	}

	if (priv_flags & RNP10_VXLAN_INNER_MATCH) {
		adapter->priv_flags |= RNP_PRIV_FLAG_VXLAN_INNER_MATCH;
		hw->ops.set_vxlan_mode(hw, true);
	} else if (adapter->priv_flags & RNP_PRIV_FLAG_VXLAN_INNER_MATCH) {
		adapter->priv_flags &= (~RNP_PRIV_FLAG_VXLAN_INNER_MATCH);
		hw->ops.set_vxlan_mode(hw, false);
	}

	if (priv_flags & RNP10_STAG_ENABLE)
		adapter->flags2 |= RNP_FLAG2_VLAN_STAGS_ENABLED;
	else
		adapter->flags2 &= (~RNP_FLAG2_VLAN_STAGS_ENABLED);

	if (priv_flags & RNP10_REC_HDR_LEN_ERR) {
		adapter->priv_flags |= RNP_PRIV_FLAG_REC_HDR_LEN_ERR;
		eth_wr32(eth, RNP10_ETH_ERR_MASK_VECTOR,
			 INNER_L4_BIT | PKT_LEN_ERR | HDR_LEN_ERR);

	} else if (adapter->priv_flags & RNP_PRIV_FLAG_REC_HDR_LEN_ERR) {
		adapter->priv_flags &= (~RNP_PRIV_FLAG_REC_HDR_LEN_ERR);
		eth_wr32(eth, RNP10_ETH_ERR_MASK_VECTOR, INNER_L4_BIT);
	}

	if (priv_flags & RNP10_REMAP_MODE)
		adapter->priv_flags |= RNP_PRIV_FLAG_REMAP_MODE;
	else
		adapter->priv_flags &= (~RNP_PRIV_FLAG_REMAP_MODE);

	if (priv_flags & RNP10_SRIOV_VLAN_MODE) {
		int i;

		adapter->priv_flags |= RNP_PRIV_FLAG_SRIOV_VLAN_MODE;
		if (!(adapter->flags & RNP_FLAG_SRIOV_INIT_DONE))
			goto skip_setup_vf_vlan;
		/* should setup vlvf table */
		for (i = 0; i < adapter->num_vfs; i++) {
			if (hw->ops.set_vf_vlan_mode) {
				if (adapter->vfinfo[i].vf_vlan)
					hw->ops.set_vf_vlan_mode(
						hw, adapter->vfinfo[i].vf_vlan,
						i, true);

				if (adapter->vfinfo[i].pf_vlan)
					hw->ops.set_vf_vlan_mode(
						hw, adapter->vfinfo[i].pf_vlan,
						i, true);
			}
		}

	} else if (adapter->priv_flags & RNP_PRIV_FLAG_SRIOV_VLAN_MODE) {
		int i;
		adapter->priv_flags &= (~RNP_PRIV_FLAG_SRIOV_VLAN_MODE);
		/* should clean vlvf table */
		for (i = 0; i < hw->max_vfs; i++) {
			if (hw->ops.set_vf_vlan_mode)
				hw->ops.set_vf_vlan_mode(hw, 0, i, false);
		}
	}

	if (hw->force_link_supported) {
		if (priv_flags & RNP10_FORCE_CLOSE) {
			if (!(adapter->priv_flags &
			      RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE)) {
				adapter->priv_flags |=
					RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE;
				if (hw->ops.driver_status) {
					hw->ops.driver_status(
						hw, true,
						rnp_driver_force_control_mac);
				}
			}
		} else {
			if (adapter->priv_flags &
			    RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE) {
				adapter->priv_flags &=
					(~RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE);
				if (hw->ops.driver_status) {
					hw->ops.driver_status(
						hw, false,
						rnp_driver_force_control_mac);
				}
			}
		}
	} else {
		if (priv_flags & RNP10_FORCE_CLOSE)
			rnp_err("%s: firmware not support set `link_down_on_close` private flag\n",
					adapter->netdev->name);
	}

skip_setup_vf_vlan:

	dbg("data new is %x\n", data_new);
	if (data_old != data_new)
		dma_wr32(dma, RNP_DMA_CONFIG, data_new);
	/* if ft_padding changed */
	if (CHK_BIT(n10_padding_enable, data_old) !=
	    CHK_BIT(n10_padding_enable, data_new)) {
		rnp_msg_post_status(adapter, PF_FT_PADDING_STATUS);
	}

	return 0;
}

static void rnp10_get_ethtool_stats(struct net_device *netdev,
				    struct ethtool_stats *stats, u64 *data)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct net_device_stats *net_stats = &netdev->stats;
	struct rnp_ring *ring;
	int i, j;
	char *p = NULL;

	rnp_update_stats(adapter);

	for (i = 0; i < RNP10_GLOBAL_STATS_LEN; i++) {
		p = (char *)net_stats + rnp10_gstrings_net_stats[i].stat_offset;
		data[i] = (rnp10_gstrings_net_stats[i].sizeof_stat ==
			   sizeof(u64)) ?
				  *(u64 *)p :
				  *(u32 *)p;
	}
	for (j = 0; j < RNP10_HWSTRINGS_STATS_LEN; j++, i++) {
		p = (char *)adapter + rnp10_hwstrings_stats[j].stat_offset;
		data[i] =
			(rnp10_hwstrings_stats[j].sizeof_stat == sizeof(u64)) ?
				*(u64 *)p :
				*(u32 *)p;
	}

	BUG_ON(RNP_NUM_TX_QUEUES != RNP_NUM_RX_QUEUES);

	for (j = 0; j < RNP_NUM_TX_QUEUES; j++) {
		int idx;
		/* tx-ring */
		ring = adapter->tx_ring[j];
		if (!ring) {
			/* tx */
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			/* rx */
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			continue;
		}
		idx = ring->rnp_queue_idx;

		data[i++] = ring->stats.packets;
		data[i++] = ring->stats.bytes;
		data[i++] = ring->tx_stats.restart_queue;
		data[i++] = ring->tx_stats.tx_busy;
		data[i++] = ring->tx_stats.tx_done_old;
		data[i++] = ring->tx_stats.clean_desc;
		data[i++] = ring->tx_stats.poll_count;
		data[i++] = ring->tx_stats.irq_more_count;

		/* rnp_tx_queue_ring_stat */
		data[i++] = ring_rd32(ring, RNP_DMA_REG_TX_DESC_BUF_HEAD);
		data[i++] = ring_rd32(ring, RNP_DMA_REG_TX_DESC_BUF_TAIL);
		data[i++] = ring->next_to_clean;
		data[i++] = ring->next_to_use;
		data[i++] = ring->tx_stats.send_bytes;
		data[i++] = ring->tx_stats.send_bytes_to_hw;
		data[i++] = ring->tx_stats.todo_update;
		data[i++] = ring->tx_stats.send_done_bytes;
		data[i++] = ring->tx_stats.vlan_add;
		if (ring->tx_stats.tx_next_to_clean == -1)
			data[i++] = ring->count;
		else
			data[i++] = ring->tx_stats.tx_next_to_clean;
		data[i++] = ring->tx_stats.tx_irq_miss;
		data[i++] = ring->tx_stats.tx_equal_count;
		data[i++] = ring->tx_stats.tx_clean_times;
		data[i++] = ring->tx_stats.tx_clean_count;

		/* rx-ring */
		ring = adapter->rx_ring[j];
		if (!ring) {
			/* rx */
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			continue;
		}
		idx = ring->rnp_queue_idx;
		data[i++] = ring->stats.packets;
		data[i++] = ring->stats.bytes;

		data[i++] = ring->rx_stats.driver_drop_packets;
		data[i++] = ring->rx_stats.rsc_count;
		data[i++] = ring->rx_stats.rsc_flush;
		data[i++] = ring->rx_stats.non_eop_descs;
		data[i++] = ring->rx_stats.alloc_rx_page_failed;
		data[i++] = ring->rx_stats.alloc_rx_buff_failed;
		data[i++] = ring->rx_stats.alloc_rx_page;
		data[i++] = ring->rx_stats.csum_err;
		data[i++] = ring->rx_stats.csum_good;
		data[i++] = ring->rx_stats.poll_again_count;
		data[i++] = ring->rx_stats.vlan_remove;

		/* rnp_rx_queue_ring_stat */
		data[i++] = ring_rd32(ring, RNP_DMA_REG_RX_DESC_BUF_HEAD);
		data[i++] = ring_rd32(ring, RNP_DMA_REG_RX_DESC_BUF_TAIL);
		data[i++] = ring->next_to_use;
		data[i++] = ring->next_to_clean;
		if (ring->rx_stats.rx_next_to_clean == -1)
			data[i++] = ring->count;
		else
			data[i++] = ring->rx_stats.rx_next_to_clean;
		data[i++] = ring->rx_stats.rx_irq_miss;
		data[i++] = ring->rx_stats.rx_equal_count;
		data[i++] = ring->rx_stats.rx_clean_times;
		data[i++] = ring->rx_stats.rx_clean_count;
	}
}

/* n10 ethtool_ops ops here */
static const struct ethtool_ops rnp10_ethtool_ops = {

	.get_link_ksettings = rnp10_get_link_ksettings,
	.set_link_ksettings = rnp10_set_link_ksettings,
	.get_drvinfo = rnp10_get_drvinfo,
	.get_regs_len = rnp10_get_regs_len,
	.get_regs = rnp10_get_regs,
	.get_wol = rnp_get_wol,
	.set_wol = rnp_set_wol,
	.nway_reset = rnp_nway_reset,
	.get_link = ethtool_op_get_link,
	.get_ringparam = rnp_get_ringparam,
	.set_ringparam = rnp_set_ringparam,
	.get_pauseparam = rnp10_get_pauseparam,
	.set_pauseparam = rnp10_set_pauseparam,
	.get_msglevel = rnp_get_msglevel,
	.set_msglevel = rnp_set_msglevel,
	.get_fecparam = rnp_get_fecparam,
	.set_fecparam = rnp_set_fecparam,
	.self_test = rnp_diag_test,
	.get_strings = rnp10_get_strings,
	.set_phys_id = rnp_set_phys_id,
	.get_sset_count = rnp10_get_sset_count,
	.get_priv_flags = rnp10_get_priv_flags,
	.set_priv_flags = rnp10_set_priv_flags,
	.get_ethtool_stats = rnp10_get_ethtool_stats,
	.get_coalesce = rnp_get_coalesce,
	.set_coalesce = rnp_set_coalesce,
        .supported_coalesce_params = ETHTOOL_COALESCE_USECS |
                                     ETHTOOL_COALESCE_MAX_FRAMES_IRQ |
                                     ETHTOOL_COALESCE_MAX_FRAMES,
	.get_rxnfc = rnp_get_rxnfc,
	.set_rxnfc = rnp_set_rxnfc,
	.get_channels = rnp_get_channels,
	.set_channels = rnp_set_channels,
	.get_module_info = rnp_get_module_info,
	.get_module_eeprom = rnp_get_module_eeprom,
	.get_ts_info = rnp_get_ts_info,
	.get_rxfh_indir_size = rnp_rss_indir_size,
	.get_rxfh_key_size = rnp_get_rxfh_key_size,
	.get_rxfh = rnp_get_rxfh,
	.set_rxfh = rnp_set_rxfh,
	.get_dump_flag = rnp_get_dump_flag,
	.get_dump_data = rnp_get_dump_data,
	.set_dump = rnp_set_dump,
	.flash_device = rnp_flash_device,
};

static void rnp_set_ethtool_hw_ops_n10(struct net_device *netdev)
{
	netdev->ethtool_ops = &rnp10_ethtool_ops;
}

/**
 * rnp_get_thermal_sensor_data_hw_ops_n10 - Gathers thermal sensor data
 * @hw: pointer to hardware structure
 * Returns the thermal sensor data structure
 **/
static s32 rnp_get_thermal_sensor_data_hw_ops_n10(struct rnp_hw *hw)
{
	int voltage = 0;
	struct rnp_thermal_sensor_data *data = &hw->thermal_sensor_data;

	data->sensor[0].temp = rnp_mbx_get_temp(hw, &voltage);

	return 0;
}

/**
 * rnp_init_thermal_sensor_thresh_hw_ops_n10 - Inits thermal sensor thresholds
 * @hw: pointer to hardware structure
 * Inits the thermal sensor thresholds according to the NVM map
 * and save off the threshold and location values into mac.thermal_sensor_data
 **/
static s32 rnp_init_thermal_sensor_thresh_hw_ops_n10(struct rnp_hw *hw)
{
	u8 i;
	struct rnp_thermal_sensor_data *data = &hw->thermal_sensor_data;

	for (i = 0; i < RNP_MAX_SENSORS; i++) {
		data->sensor[i].location = i + 1;
		data->sensor[i].caution_thresh = 90;
		data->sensor[i].max_op_thresh = 100;
	}

	return 0;
}

static s32 rnp_phy_read_reg_hw_ops_n10(struct rnp_hw *hw, u32 reg_addr,
				u32 device_type, u16 *phy_data)
{
	s32 status = 0;
	u32 data = 0;

	status = rnp_mbx_phy_read(hw, reg_addr, &data);
	*phy_data = data & 0xffff;

	return status;
}

static s32 rnp_phy_write_reg_hw_ops_n10(struct rnp_hw *hw, u32 reg_addr,
				 u32 device_type, u16 phy_data)
{
	s32 status = 0;

	status = rnp_mbx_phy_write(hw, reg_addr, (u32)phy_data);

	return status;
}

static void rnp_set_vf_vlan_mode_hw_ops_n10(struct rnp_hw *hw, u16 vlan, int vf,
				     bool enable)
{
	struct rnp_eth_info *eth = &hw->eth;
	struct rnp_adapter *adapter = (struct rnp_adapter *)hw->back;

	if (adapter->priv_flags & RNP_PRIV_FLAG_SRIOV_VLAN_MODE)
		eth->ops.set_vf_vlan_mode(eth, vlan, vf, enable);
}

static void rnp_driver_status_hw_ops_n10(struct rnp_hw *hw, bool enable, int mode)
{
	switch (mode) {
	case rnp_driver_insmod:
		rnp_mbx_ifinsmod(hw, enable);
		break;
	case rnp_driver_suspuse:
		rnp_mbx_ifsuspuse(hw, enable);
		break;
	case rnp_driver_force_control_mac:
		rnp_mbx_ifforce_control_mac(hw, enable);

		break;
	}
}

static struct rnp_hw_operations hw_ops_n10 = {
	.init_hw = &rnp_init_hw_ops_n10,
	.reset_hw = &rnp_reset_hw_ops_n10,
	.start_hw = &rnp_start_hw_ops_n10,
	.set_mtu = &rnp_set_mtu_hw_ops_n10,
	.set_vlan_filter_en = &rnp_set_vlan_filter_en_hw_ops_n10,
	.set_vlan_filter = &rnp_set_vlan_filter_hw_ops_n10,
	.set_vf_vlan_filter = &rnp_set_vf_vlan_filter_hw_ops_n10,
	.set_vlan_strip = &rnp_set_vlan_strip_hw_ops_n10,
	.set_mac = &rnp_set_mac_hw_ops_n10,
	.set_rx_mode = &rnp_set_rx_mode_hw_ops_n10,
	.set_rar_with_vf = &rnp_set_rar_with_vf_hw_ops_n10,
	.clr_rar = &rnp_clr_rar_hw_ops_n10,
	.clr_rar_all = &rnp_clr_rar_all_hw_ops_n10,
	.clr_vlan_veb = &rnp_clr_vlan_veb_hw_ops_n10,
	.set_txvlan_mode = &rnp_set_txvlan_mode_hw_ops_n10,
	.set_fcs_mode = &rnp_set_fcs_mode_hw_ops_n10,
	.set_vxlan_port = &rnp_set_vxlan_port_hw_ops_n10,
	.set_vxlan_mode = &rnp_set_vxlan_mode_hw_ops_n10,
	.set_mac_rx = &rnp_set_mac_rx_hw_ops_n10,
	.set_rx_hash = &rnp_set_rx_hash_hw_ops_n10,
	.set_pause_mode = &rnp_set_pause_mode_hw_ops_n10,
	.get_pause_mode = &rnp_get_pause_mode_hw_ops_n10,
	.update_hw_info = &rnp_update_hw_info_hw_ops_n10,
	.update_rx_drop = &rnp_update_hw_rx_drop_hw_ops_n10,
	.update_sriov_info = &rnp_update_sriov_info_hw_ops_n10,
	.set_sriov_status = &rnp_set_sriov_status_hw_ops_n10,
	.set_sriov_vf_mc = &rnp_set_sriov_vf_mc_hw_ops_n10,
	.init_rx_addrs = &rnp_init_rx_addrs_hw_ops_n10,
	.clr_vfta = &rnp_clr_vfta_hw_ops_n10,
	.set_rss_key = &rnp_set_rss_key_hw_ops_n10,
	.set_rss_table = &rnp_set_rss_table_hw_ops_n10,
	.update_hw_status = &rnp_update_hw_status_hw_ops_n10,
	.set_mbx_link_event = &rnp_set_mbx_link_event_hw_ops_n10,
	.set_mbx_ifup = &rnp_set_mbx_ifup_hw_ops_n10,
	.check_link = &rnp_check_mac_link_hw_ops_n10,
	.setup_link = &rnp_setup_mac_link_hw_ops_n10,
	.clean_link = &rnp_clean_link_hw_ops_n10,
	.set_layer2_remapping = &rnp_set_layer2_hw_ops_n10,
	.clr_layer2_remapping = &rnp_clr_layer2_hw_ops_n10,
	.clr_all_layer2_remapping = &rnp_clr_all_layer2_hw_ops_n10,
	.set_tuple5_remapping = &rnp_set_tuple5_hw_ops_n10,
	.clr_tuple5_remapping = &rnp_clr_tuple5_hw_ops_n10,
	.clr_all_tuple5_remapping = &rnp_clr_all_tuple5_hw_ops_n10,
	.set_tcp_sync_remapping = &rnp_set_tcp_sync_hw_ops_n10,
	.update_msix_count = &rnp_update_msix_count_hw_ops_n10,
	.get_thermal_sensor_data = &rnp_get_thermal_sensor_data_hw_ops_n10,
	.init_thermal_sensor_thresh =
		&rnp_init_thermal_sensor_thresh_hw_ops_n10,
	.setup_ethtool = &rnp_set_ethtool_hw_ops_n10,
	.phy_read_reg = &rnp_phy_read_reg_hw_ops_n10,
	.phy_write_reg = &rnp_phy_write_reg_hw_ops_n10,
	.set_vf_vlan_mode = &rnp_set_vf_vlan_mode_hw_ops_n10,
	.driver_status = &rnp_driver_status_hw_ops_n10,
};

static void rnp_mac_set_rx_n10(struct rnp_mac_info *mac, bool status)
{
	struct rnp_hw *hw = (struct rnp_hw *)mac->back;
	struct rnp_adapter *adapter = (struct rnp_adapter *)hw->back;

	u32 value = 0;
	u32 count = 0;

	if (status) {
		do {
			mac_wr32(mac, RNP10_MAC_RX_CFG,
				 mac_rd32(mac, RNP10_MAC_RX_CFG) | 0x01);
			usleep_range(100, 200);
			value = mac_rd32(mac, RNP10_MAC_RX_CFG);
			count++;
			if (count > 1000) {
				printk("setup rx on timeout\n");
				break;
			}
		} while (!(value & 0x01));

		if (adapter->flags & RNP_FLAG_SWITCH_LOOPBACK_EN) {
			mac_wr32(mac, RNP10_MAC_PKT_FLT, BIT(31) | BIT(0));
			eth_wr32(&hw->eth, RNP10_ETH_DMAC_MCSTCTRL, 0x0);
		} else {
			do {
				mac_wr32(mac, RNP10_MAC_RX_CFG,
					 mac_rd32(mac, RNP10_MAC_RX_CFG) &
						 (~0x400));
				usleep_range(100, 200);
				value = mac_rd32(mac, RNP_MAC_RX_CFG);
				count++;
				if (count > 1000) {
					printk("setup rx off timeout\n");
					break;
				}
			} while (value & 0x400);
			if (hw->ncsi_en)
				mac_wr32(mac, RNP10_MAC_PKT_FLT, 0x80000001);
			else
				mac_wr32(mac, RNP10_MAC_PKT_FLT, 0x00000001);
		}
	} else {
		do {
			mac_wr32(mac, RNP10_MAC_RX_CFG,
				 mac_rd32(mac, RNP10_MAC_RX_CFG) | 0x400);
			usleep_range(100, 200);
			value = mac_rd32(mac, RNP10_MAC_RX_CFG);
			count++;
			if (count > 1000) {
				printk("setup rx on timeout\n");
				break;
			}
		} while (!(value & 0x400));
		mac_wr32(mac, RNP10_MAC_PKT_FLT, 0x0);
	}
}

static void rnp_mac_fcs_n10(struct rnp_mac_info *mac, bool status)
{
	u32 value;

#define FCS_MASK (0x6)
	value = mac_rd32(mac, RNP10_MAC_RX_CFG);
	if (status) {
		value &= (~FCS_MASK);

	} else {
		value |= FCS_MASK;
	}

	mac_wr32(mac, RNP10_MAC_RX_CFG, value);
}

/**
 *  rnp_fc_mode_n10 - Enable flow control
 *  @hw: pointer to hardware structure
 *
 *  Enable flow control according to the current settings.
 **/
static s32 rnp_mac_fc_mode_n10(struct rnp_mac_info *mac)
{
	struct rnp_hw *hw = (struct rnp_hw *)mac->back;
	s32 ret_val = 0;
	u32 reg;
	u32 rxctl_reg, txctl_reg[RNP_MAX_TRAFFIC_CLASS];
	int i;

	/*
	 * Validate the water mark configuration for packet buffer 0.  Zero
	 * water marks indicate that the packet buffer was not configured
	 * and the watermarks for packet buffer 0 should always be configured.
	 */
	if (!hw->fc.pause_time) {
		ret_val = RNP_ERR_INVALID_LINK_SETTINGS;
		goto out;
	}

	/* Disable any previous flow control settings */
	rxctl_reg = mac_rd32(mac, RNP10_MAC_RX_FLOW_CTRL);
	rxctl_reg &= (~RNP10_RX_FLOW_ENABLE_MASK);

	for (i = 0; i < RNP_MAX_TRAFFIC_CLASS; i++) {
		txctl_reg[i] = mac_rd32(mac, RNP10_MAC_Q0_TX_FLOW_CTRL(i));
		txctl_reg[i] &= (~RNP10_TX_FLOW_ENABLE_MASK);
	}
	/*
	 * The possible values of fc.current_mode are:
	 * 0: Flow control is completely disabled
	 * 1: Rx flow control is enabled (we can receive pause frames,
	 *    but not send pause frames).
	 * 2: Tx flow control is enabled (we can send pause frames but
	 *    we do not support receiving pause frames).
	 * 3: Both Rx and Tx flow control (symmetric) are enabled.
	 * other: Invalid.
	 */
	switch (hw->fc.current_mode) {
	case rnp_fc_none:
		/*
		 * Flow control is disabled by software override or autoneg.
		 * The code below will actually disable it in the HW.
		 */
		break;
	case rnp_fc_rx_pause:
		/*
		 * Rx Flow control is enabled and Tx Flow control is
		 * disabled by software override. Since there really
		 * isn't a way to advertise that we are capable of RX
		 * Pause ONLY, we will advertise that we support both
		 * symmetric and asymmetric Rx PAUSE.  Later, we will
		 * disable the adapter's ability to send PAUSE frames.
		 */
		rxctl_reg |= (RNP10_RX_FLOW_ENABLE_MASK);
		break;
	case rnp_fc_tx_pause:
		/*
		 * Tx Flow control is enabled, and Rx Flow control is
		 * disabled by software override.
		 */
		for (i = 0; i < RNP_MAX_TRAFFIC_CLASS; i++)
			txctl_reg[i] |= (RNP10_TX_FLOW_ENABLE_MASK);
		break;
	case rnp_fc_full:
		/* Flow control (both Rx and Tx) is enabled by SW override. */
		rxctl_reg |= (RNP10_RX_FLOW_ENABLE_MASK);
		for (i = 0; i < RNP_MAX_TRAFFIC_CLASS; i++)
			txctl_reg[i] |= (RNP10_TX_FLOW_ENABLE_MASK);
		break;
	default:
		hw_dbg(hw, "Flow control param set incorrectly\n");
		ret_val = RNP_ERR_CONFIG;
		goto out;
	}

	/* Configure pause time (2 TCs per register) */
	reg = hw->fc.pause_time;
	for (i = 0; i < (RNP_MAX_TRAFFIC_CLASS); i++)
		txctl_reg[i] |= (reg << 16);

	/* Set 802.3x based flow control settings. */
	mac_wr32(mac, RNP10_MAC_RX_FLOW_CTRL, rxctl_reg);
	for (i = 0; i < (RNP_MAX_TRAFFIC_CLASS); i++)
		mac_wr32(mac, RNP10_MAC_Q0_TX_FLOW_CTRL(i), txctl_reg[i]);
out:
	return ret_val;
}

static void rnp_mac_set_mac_n10(struct rnp_mac_info *mac, u8 *addr, int index)
{
	u32 rar_low, rar_high = 0;
	rar_low = ((u32)addr[0] | ((u32)addr[1] << 8) | ((u32)addr[2] << 16) |
		   ((u32)addr[3] << 24));

	rar_high = RNP_RAH_AV | ((u32)addr[4] | (u32)addr[5] << 8);

	mac_wr32(mac, RNP10_MAC_UNICAST_HIGH(index), rar_high);
	mac_wr32(mac, RNP10_MAC_UNICAST_LOW(index), rar_low);
}

static struct rnp_mac_operations mac_ops_n10 = {
	.set_mac_rx = &rnp_mac_set_rx_n10,
	.set_mac_fcs = &rnp_mac_fcs_n10,
	.set_fc_mode = &rnp_mac_fc_mode_n10,
	.set_mac = &rnp_mac_set_mac_n10,
};

static s32 rnp_get_invariants_n10(struct rnp_hw *hw)
{
	struct rnp_mac_info *mac = &hw->mac;
	struct rnp_dma_info *dma = &hw->dma;
	struct rnp_eth_info *eth = &hw->eth;
	struct rnp_mbx_info *mbx = &hw->mbx;
	struct rnp_adapter *adapter = (struct rnp_adapter *)hw->back;
	int i;

	/* setup dma info */
	dma->dma_base_addr = hw->hw_addr;
	dma->dma_ring_addr = hw->hw_addr + RNP10_RING_BASE;
	dma->max_tx_queues = RNP_N10_MAX_TX_QUEUES;
	dma->max_rx_queues = RNP_N10_MAX_RX_QUEUES;
	dma->back = hw;
	memcpy(&hw->dma.ops, &dma_ops_n10, sizeof(hw->dma.ops));

	/* setup eth info */
	memcpy(&hw->eth.ops, &eth_ops_n10, sizeof(hw->eth.ops));

	eth->eth_base_addr = hw->hw_addr + RNP10_ETH_BASE;
	printk(" eth_base is %p\n", eth->eth_base_addr);
	eth->back = hw;
	eth->mc_filter_type = 0;
	eth->mcft_size = RNP_N10_MC_TBL_SIZE;
	eth->vft_size = RNP_N10_VFT_TBL_SIZE;
	if (hw->eco)
		eth->num_rar_entries = RNP_N10_RAR_ENTRIES - 1;
	else
		eth->num_rar_entries = RNP_N10_RAR_ENTRIES;

	eth->max_rx_queues = RNP_N10_MAX_RX_QUEUES;
	eth->max_tx_queues = RNP_N10_MAX_TX_QUEUES;

	/* setup mac info */
	memcpy(&hw->mac.ops, &mac_ops_n10, sizeof(hw->mac.ops));
	mac->mac_addr = hw->hw_addr + RNP10_MAC_BASE;
	mac->back = hw;
	mac->mac_type = mac_dwc_xlg;
	/* move this to eth todo */
	mac->mc_filter_type = 0;
	mac->mcft_size = RNP_N10_MC_TBL_SIZE;
	mac->vft_size = RNP_N10_VFT_TBL_SIZE;
	if (hw->eco)
		mac->num_rar_entries = RNP_N10_RAR_ENTRIES - 1;
	else
		mac->num_rar_entries = RNP_N10_RAR_ENTRIES;
	mac->max_rx_queues = RNP_N10_MAX_RX_QUEUES;
	mac->max_tx_queues = RNP_N10_MAX_TX_QUEUES;
	mac->max_msix_vectors = RNP_N10_MSIX_VECTORS;
	if (!hw->axi_mhz)
		hw->usecstocount = 500;
	else
		hw->usecstocount = hw->axi_mhz;

	/* set up hw feature */
	hw->feature_flags |=
		RNP_NET_FEATURE_SG | RNP_NET_FEATURE_TX_CHECKSUM |
		RNP_NET_FEATURE_RX_CHECKSUM | RNP_NET_FEATURE_TSO |
		RNP_NET_FEATURE_TX_UDP_TUNNEL | RNP_NET_FEATURE_VLAN_FILTER |
		RNP_NET_FEATURE_VLAN_OFFLOAD |
		RNP_NET_FEATURE_RX_NTUPLE_FILTER | RNP_NET_FEATURE_TCAM |
		RNP_NET_FEATURE_RX_HASH | RNP_NET_FEATURE_RX_FCS;
	/* maybe supported future*/
	/* setup some fdir resource */
	hw->min_length = RNP_MIN_MTU;
	hw->max_length = RNP_MAX_JUMBO_FRAME_SIZE;
	hw->max_msix_vectors = RNP_N10_MSIX_VECTORS;
	if (hw->eco)
		hw->num_rar_entries = RNP_N10_RAR_ENTRIES - 1;
	else
		hw->num_rar_entries = RNP_N10_RAR_ENTRIES;
	hw->fdir_mode = fdir_mode_tuple5;
	hw->max_vfs = RNP_N10_MAX_VF;
	hw->max_vfs_noari = 3;
	hw->sriov_ring_limit = 2;
	/* some user only want 1 queue for each vf */
	hw->max_pf_macvlans = RNP_MAX_PF_MACVLANS_N10;
	hw->wol_supported = WAKE_MAGIC;
	/* ncsi */
	hw->ncsi_vf_cpu_shm_pf_base = RNP_VF_CPU_SHM_BASE_NR62;
	hw->ncsi_mc_count = RNP_NCSI_MC_COUNT;
	hw->ncsi_vlan_count = RNP_NCSI_VLAN_COUNT;
	/* we suppose 1536 */
	hw->dma_split_size = 1536;
	if (hw->fdir_mode == fdir_mode_tcam) {
		hw->layer2_count = RNP10_MAX_LAYER2_FILTERS - 1;
		hw->tuple5_count = RNP10_MAX_TCAM_FILTERS - 1;
	} else {
		hw->layer2_count = RNP10_MAX_LAYER2_FILTERS - 1;
		hw->tuple5_count = RNP10_MAX_TUPLE5_FILTERS - 1;
	}

	hw->default_rx_queue = 0;
	hw->rss_indir_tbl_num = RNP_N10_RSS_TBL_NUM;
	hw->rss_tc_tbl_num = RNP_N10_RSS_TC_TBL_NUM;
	/* vf use the last vfnum */
	hw->vfnum = RNP_N10_MAX_VF - 1;
	hw->feature_flags |= RNP_NET_FEATURE_VF_FIXED;
	if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED)
		hw->veb_ring = 0;
	else
		hw->veb_ring = RNP_N10_MAX_RX_QUEUES;

	memcpy(&hw->ops, &hw_ops_n10, sizeof(hw->ops));
	/* PHY */
	/* setup pcs */
	memcpy(&hw->pcs.ops, &pcs_ops_generic, sizeof(hw->pcs.ops));
	mbx->mbx_feature |= MBX_FEATURE_WRITE_DELAY;
	mbx->vf2pf_mbox_vec_base = 0xa5100;
	mbx->cpu2pf_mbox_vec = 0xa5300;
	mbx->pf_vf_shm_base = 0xa6000;
	mbx->mbx_mem_size = 64;
	mbx->pf2vf_mbox_ctrl_base = 0xa7100;
	mbx->pf_vf_mbox_mask_lo = 0xa7200;
	mbx->pf_vf_mbox_mask_hi = 0xa7300;
	mbx->cpu_pf_shm_base = 0xaa000;
	mbx->pf2cpu_mbox_ctrl = 0xaa100;
	mbx->cpu_pf_mbox_mask = 0xaa300;
	adapter->drop_time = 100;
	hw->fc.requested_mode = PAUSE_TX | PAUSE_RX;
	hw->fc.pause_time = RNP_DEFAULT_FCPAUSE;
	for (i = 0; i < RNP_MAX_TRAFFIC_CLASS; i++) {
		hw->fc.high_water[i] = RNP10_DEFAULT_HIGH_WATER;
		hw->fc.low_water[i] = RNP10_DEFAULT_LOW_WATER;
	}
#ifdef FIX_MAC_PADDIN
	adapter->priv_flags |= RNP_PRIV_FLAG_TX_PADDING;

#endif
	return 0;
}

struct rnp_info rnp_n10_info = {
	.one_pf_with_two_dma = false,
	.total_queue_pair_cnts = RNP_N10_MAX_TX_QUEUES,
	.adapter_cnt = 1,
	.rss_type = rnp_rss_n10,
	.hw_type = rnp_hw_n10,
	.get_invariants = &rnp_get_invariants_n10,
	.mac_ops = &mac_ops_n10,
	.eeprom_ops = NULL,
	.mbx_ops = &mbx_ops_generic,
	.pcs_ops = &pcs_ops_generic,
};

static s32 rnp_get_invariants_n400(struct rnp_hw *hw)
{
	struct rnp_mac_info *mac = &hw->mac;
	struct rnp_dma_info *dma = &hw->dma;
	struct rnp_eth_info *eth = &hw->eth;
	struct rnp_mbx_info *mbx = &hw->mbx;
	struct rnp_adapter *adapter = (struct rnp_adapter *)hw->back;
	int i;
	/* setup dma info */
	dma->dma_base_addr = hw->hw_addr;
	dma->dma_ring_addr = hw->hw_addr + RNP10_RING_BASE;
	dma->max_tx_queues = RNP_N400_MAX_TX_QUEUES;
	dma->max_rx_queues = RNP_N400_MAX_RX_QUEUES;
	dma->back = hw;
	memcpy(&hw->dma.ops, &dma_ops_n10, sizeof(hw->dma.ops));

	/* setup eth info */
	memcpy(&hw->eth.ops, &eth_ops_n10, sizeof(hw->eth.ops));
	eth->eth_base_addr = hw->hw_addr + RNP10_ETH_BASE;
	eth->back = hw;
	eth->mc_filter_type = 0;
	eth->mcft_size = RNP_N10_MC_TBL_SIZE;
	eth->vft_size = RNP_N10_VFT_TBL_SIZE;
	if (hw->eco)
		eth->num_rar_entries = RNP_N10_RAR_ENTRIES - 1;
	else
		eth->num_rar_entries = RNP_N10_RAR_ENTRIES;
	eth->max_rx_queues = RNP_N400_MAX_RX_QUEUES;
	eth->max_tx_queues = RNP_N400_MAX_TX_QUEUES;

	/* setup mac info */
	memcpy(&hw->mac.ops, &mac_ops_n10, sizeof(hw->mac.ops));
	mac->mac_addr = hw->hw_addr + RNP10_MAC_BASE;
	mac->back = hw;
	mac->mac_type = mac_dwc_xlg;
	/* move this to eth todo */
	mac->mc_filter_type = 0;
	mac->mcft_size = RNP_N10_MC_TBL_SIZE;
	mac->vft_size = RNP_N10_VFT_TBL_SIZE;
	if (hw->eco)
		mac->num_rar_entries = RNP_N10_RAR_ENTRIES - 1;
	else
		mac->num_rar_entries = RNP_N10_RAR_ENTRIES;
	mac->max_rx_queues = RNP_N400_MAX_RX_QUEUES;
	mac->max_tx_queues = RNP_N400_MAX_TX_QUEUES;
	mac->max_msix_vectors = RNP_N400_MSIX_VECTORS;
	if (!hw->axi_mhz)
		hw->usecstocount = 125;
	else
		hw->usecstocount = hw->axi_mhz;
	/* set up hw feature */
	hw->feature_flags |=
		RNP_NET_FEATURE_SG | RNP_NET_FEATURE_TX_CHECKSUM |
		RNP_NET_FEATURE_RX_CHECKSUM | RNP_NET_FEATURE_TSO |
		RNP_NET_FEATURE_TX_UDP_TUNNEL | RNP_NET_FEATURE_VLAN_FILTER |
		RNP_NET_FEATURE_VLAN_OFFLOAD |
		RNP_NET_FEATURE_RX_NTUPLE_FILTER | RNP_NET_FEATURE_TCAM |
		RNP_NET_FEATURE_RX_HASH | RNP_NET_FEATURE_RX_FCS;
	/* setup some fdir resource */
	hw->min_length = RNP_MIN_MTU;
	hw->max_length = RNP_MAX_JUMBO_FRAME_SIZE;
	hw->max_msix_vectors = RNP_N400_MSIX_VECTORS;
	if (hw->eco)
		hw->num_rar_entries = RNP_N10_RAR_ENTRIES - 1;
	else
		hw->num_rar_entries = RNP_N10_RAR_ENTRIES;
	hw->fdir_mode = fdir_mode_tuple5;
	hw->max_vfs = RNP_N400_MAX_VF;
	hw->max_vfs_noari = 3;
	/* n400 only use 1 ring for each vf */
	hw->sriov_ring_limit = 1;
	hw->max_pf_macvlans = RNP_MAX_PF_MACVLANS_N10;
	/* ncsi */
	hw->ncsi_vf_cpu_shm_pf_base = RNP_VF_CPU_SHM_BASE_NR62;
	hw->ncsi_mc_count = RNP_NCSI_MC_COUNT;
	hw->ncsi_vlan_count = RNP_NCSI_VLAN_COUNT;

	if (hw->fdir_mode == fdir_mode_tcam) {
		hw->layer2_count = RNP10_MAX_LAYER2_FILTERS - 1;
		hw->tuple5_count = RNP10_MAX_TCAM_FILTERS - 1;
	} else {
		hw->layer2_count = RNP10_MAX_LAYER2_FILTERS - 1;
		hw->tuple5_count = RNP10_MAX_TUPLE5_FILTERS - 1;
	}

	hw->default_rx_queue = 0;
	hw->rss_indir_tbl_num = RNP_N10_RSS_TBL_NUM;
	hw->rss_tc_tbl_num = RNP_N10_RSS_TC_TBL_NUM;
	/* vf use the last vfnum */
	hw->vfnum = RNP_N400_MAX_VF - 1;

	/* n400 should fix_vf_bug */
	hw->feature_flags |= RNP_NET_FEATURE_VF_FIXED;

	if (hw->feature_flags & RNP_NET_FEATURE_VF_FIXED) {
		hw->veb_ring = 0;
		hw->default_vf_num = 0;
	} else {
		hw->veb_ring = RNP_N400_MAX_RX_QUEUES;
		hw->default_vf_num = RNP_N10_MAX_VF - 1;
	}

	memcpy(&hw->ops, &hw_ops_n10, sizeof(hw->ops));
	/* setup pcs */
	memcpy(&hw->pcs.ops, &pcs_ops_generic, sizeof(hw->pcs.ops));
	mbx->mbx_feature |= MBX_FEATURE_WRITE_DELAY;
	mbx->vf2pf_mbox_vec_base = 0xa5100;
	mbx->cpu2pf_mbox_vec = 0xa5300;
	mbx->pf_vf_shm_base = 0xa6000;
	mbx->mbx_mem_size = 64;
	mbx->pf2vf_mbox_ctrl_base = 0xa7100;
	mbx->pf_vf_mbox_mask_lo = 0xa7200;
	mbx->pf_vf_mbox_mask_hi = 0xa7300;
	mbx->cpu_pf_shm_base = 0xaa000;
	mbx->pf2cpu_mbox_ctrl = 0xaa100;
	mbx->cpu_pf_mbox_mask = 0xaa300;

	adapter->drop_time = 100;
	/* initialization default pause flow */
	hw->fc.requested_mode |= PAUSE_AUTO;
	hw->fc.pause_time = RNP_DEFAULT_FCPAUSE;
	for (i = 0; i < RNP_MAX_TRAFFIC_CLASS; i++) {
		hw->fc.high_water[i] = RNP10_DEFAULT_HIGH_WATER;
		hw->fc.low_water[i] = RNP10_DEFAULT_LOW_WATER;
	}

	hw->autoneg = 1;
	hw->tp_mdix_ctrl = ETH_TP_MDI_AUTO;

	return 0;
}

struct rnp_info rnp_n400_info = {
	.one_pf_with_two_dma = false,
	.total_queue_pair_cnts = RNP_N400_MAX_TX_QUEUES,
	.adapter_cnt = 1,
	.rss_type = rnp_rss_n10,
	.hw_type = rnp_hw_n400,
	.get_invariants = &rnp_get_invariants_n400,
	.mac_ops = &mac_ops_n10,
	.eeprom_ops = NULL,
	.mbx_ops = &mbx_ops_generic,
	.pcs_ops = &pcs_ops_generic,
};
