// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/netdevice.h>
#include <linux/crc32.h>
#include <linux/cacheinfo.h>
#include "rnpm.h"
#include "rnpm_common.h"
#include "rnpm_phy.h"
#include "rnpm_mbx_fw.h"

static s32 rnpm_mta_vector(int mode, u8 *mc_addr);
unsigned int rnpm_loglevel;
module_param(rnpm_loglevel, uint, 0000);

/**
 *  rnpm_device_supports_autoneg_fc - Check if phy supports autoneg flow
 *  control
 *  @hw: pointer to hardware structure
 *
 *  There are several phys that do not support autoneg flow control. This
 *  function check the device id to see if the associated phy supports
 *  autoneg flow control.
 **/
bool rnpm_device_supports_autoneg_fc(struct rnpm_hw *hw)
{
	bool supported = false;

	if (hw->is_sgmii == 0)
		return false;

	switch (hw->phy.media_type) {
	case rnpm_media_type_fiber:
		break;
	case rnpm_media_type_backplane:
		break;
	case rnpm_media_type_copper:
		/* only some copper devices support flow control autoneg */
		supported = true;
		break;
	default:
		break;
	}

	return supported;
}

/**
 *  rnpm_setup_fc - Set up flow control
 *  @hw: pointer to hardware structure
 *
 *  Called at init time to set up flow control.
 **/
s32 rnpm_setup_fc(struct rnpm_hw *hw)
{
	s32 ret = 0;
	u16 pause_bits = 0;
	u16 value;

	/* phy pause */
	if (!hw->is_sgmii)
		goto out;

	switch (hw->fc.requested_mode) {
	case rnpm_fc_none:
		/* Flow control completely disabled by software override. */
		break;
	case rnpm_fc_tx_pause:
		/*
		 * Tx Flow control is enabled, and Rx Flow control is
		 * disabled by software override.
		 */
		pause_bits |= BIT(11);
		break;
	case rnpm_fc_rx_pause:
		/*
		 * Rx Flow control is enabled and Tx Flow control is
		 * disabled by software override. Since there really
		 * isn't a way to advertise that we are capable of RX
		 * Pause ONLY, we will advertise that we support both
		 * symmetric and asymmetric Rx PAUSE, as such we fall
		 * through to the fc_full statement.  Later, we will
		 * disable the adapter's ability to send PAUSE frames.
		 */
	case rnpm_fc_full:
		/* Flow control (both Rx and Tx) is enabled by SW override. */
		pause_bits |= BIT(11) | BIT(10);
		break;
	default:
		hw_dbg(hw, "Flow control phy param set incorrectly\n");
		ret = RNPM_ERR_CONFIG;
		goto out;
	}
	hw->phy.ops.read_reg(hw, 4, 0, &value);
	value &= ~0xC00;
	value |= pause_bits;
	hw->phy.ops.write_reg(hw, 4, 0, value);

out:
	return ret;
}

/**
 *  rnpm_start_hw_generic - Prepare hardware for Tx/Rx
 *  @hw: pointer to hardware structure
 *
 *  Starts the hardware by filling the bus info structure and media type, clears
 *  all on chip counters, initializes receive address registers, multicast
 *  table, VLAN filter table, calls routine to set up link and flow control
 *  settings, and leaves transmit and receive units disabled and uninitialized
 **/
s32 rnpm_start_hw_generic(struct rnpm_hw *hw)
{
	/* Set the media type */
	hw->phy.media_type = hw->mac.ops.get_media_type(hw);
	/* Identify the PHY */
	hw->phy.ops.identify(hw);
	/* Clear the VLAN filter table */
	/* maybe mistalbe here in mutiport*/
	hw->mac.ops.clear_vfta(hw);
	/* Clear statistics registers */
	hw->mac.ops.clear_hw_cntrs(hw);
	/* Setup flow control */
	hw->mac.ops.setup_fc(hw);
	/* Clear adapter stopped flag */
	hw->adapter_stopped = false;
	return 0;
}

/**
 *  rnpm_init_hw_generic - Generic hardware initialization
 *  @hw: pointer to hardware structure
 *
 *  Initialize the hardware by resetting the hardware, filling the bus info
 *  structure and media type, clears all on chip counters, initializes receive
 *  address registers, multicast table, VLAN filter table, calls routine to set
 *  up link and flow control settings, and leaves transmit and receive units
 *  disabled and uninitialized
 **/
s32 rnpm_init_hw_generic(struct rnpm_hw *hw)
{
	s32 status;

	/* Reset the hardware */
	status = hw->mac.ops.reset_hw(hw);
	if (status == 0) {
		/* Start the HW */
		status = hw->mac.ops.start_hw(hw);
	}
	return status;
}

void rnpm_reset_msix_table_generic(struct rnpm_hw *hw)
{
	int i;

	/* reset NIC_RING_VECTOR table to 0 */
	for (i = 0; i < 128; i++)
		rnpm_wr_reg(hw->ring_msix_base + RING_VECTOR(i), 0);
}

/**
 *  rnpm_clear_hw_cntrs_generic - Generic clear hardware counters
 *  @hw: pointer to hardware structure
 *
 *  Clears all hardware statistics counters by reading them from the hardware
 *  Statistics counters are clear on read.
 **/
s32 rnpm_clear_hw_cntrs_generic(struct rnpm_hw *hw)
{
	struct rnpm_adapter *adapter =
		container_of(hw, struct rnpm_adapter, hw);
	struct net_device_stats *net_stats = &adapter->netdev->stats;
	int port = adapter->port;

	hw->err_pkts_init.wdt[port] =
		rd32(hw, RNPM_RXTRANS_WDT_ERR_PKTS(port));
	hw->err_pkts_init.code[port] =
		rd32(hw, RNPM_RXTRANS_CODE_ERR_PKTS(port));
	hw->err_pkts_init.crc[port] =
		rd32(hw, RNPM_RXTRANS_CRC_ERR_PKTS(port));
	hw->err_pkts_init.slen[port] =
		rd32(hw, RNPM_RXTRANS_SLEN_ERR_PKTS(port));
	hw->err_pkts_init.glen[port] =
		rd32(hw, RNPM_RXTRANS_GLEN_ERR_PKTS(port));
	hw->err_pkts_init.iph[port] =
		rd32(hw, RNPM_RXTRANS_IPH_ERR_PKTS(port));
	hw->err_pkts_init.len[port] =
		rd32(hw, RNPM_RXTRANS_LEN_ERR_PKTS(port));
	hw->err_pkts_init.cut[port] =
		rd32(hw, RNPM_RXTRANS_CUT_ERR_PKTS(port));
	hw->err_pkts_init.drop[port] =
		rd32(hw, RNPM_RXTRANS_DROP_PKTS(port));
	hw->err_pkts_init.csum[port] =
		rd32(hw, RNPM_RXTRANS_CSUM_ERR_PKTS(port));
	hw->err_pkts_init.scsum[port] = 0;
	net_stats->rx_crc_errors = 0;
	net_stats->rx_errors = 0;
	net_stats->rx_dropped = 0;
	return 0;
}

s32 rnpm_get_permission_mac_addr(struct rnpm_hw *hw, u8 *mac_addr)
{
	int err = 0;

	err = rnpm_fw_get_macaddr(hw, hw->pfvfnum, mac_addr, hw->nr_lane);
	if (err || !is_valid_ether_addr(mac_addr)) {
		dbg_hw("generate ramdom macaddress...\n");
		eth_random_addr(mac_addr);
	}
	hw->mac.mac_flags |= RNPM_FLAGS_INIT_MAC_ADDRESS;
	dbg_hw("%s mac:%pM\n", __func__, mac_addr);
	return 0;
}

/**
 *  rnpm_get_mac_addr_generic - Generic get MAC address
 *  @hw: pointer to hardware structure
 *  @mac_addr: Adapter MAC address
 *
 *  Reads the adapter's MAC address from first Receive Address Register (RAR0)
 *  A reset of the adapter must be performed prior to calling this function
 *  in order for the MAC address to have been loaded from the EEPROM into RAR0
 **/
s32 rnpm_get_mac_addr_generic(struct rnpm_hw *hw, u8 *mac_addr)
{
	u32 rar_high, rar_low, i;

	rar_high = rd32(hw, RNPM_ETH_RAR_RH(0));
	rar_low = rd32(hw, RNPM_ETH_RAR_RL(0));
	for (i = 0; i < 4; i++)
		mac_addr[i] = (u8)(rar_low >> (i * 8));
	for (i = 0; i < 2; i++)
		mac_addr[i + 4] = (u8)(rar_high >> (i * 8));
	mac_addr[5] += hw->num;
	return 0;
}

int rnpm_get_cpu_l3_cache_size(void)
{
	return cache_line_size();
}

/**
 *  rnpm_set_rar_generic - Set Rx address register
 *  @hw: pointer to hardware structure
 *  @index: Receive address register to write
 *  @addr: Address to put into receive address register
 *  @vmdq: VMDq "set" or "pool" index
 *  @enable_addr: set flag that address is active
 *
 *  Puts an ethernet address into a receive address register.
 **/
s32 rnpm_set_rar_generic(struct rnpm_hw *hw, u32 index, u8 *addr, u32 vmdq,
			 u32 enable_addr)
{
	u32 mcstctrl;
	u32 rar_low, rar_high = 0;
	u32 rar_entries = hw->mac.num_rar_entries;
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries + hw->ncsi_rar_entries) {
		netdev_err(
			adapter->netdev,
			"set_rar_generic RAR index %d is out of range.\n",
			index);
		return RNPM_ERR_INVALID_ARGUMENT;
	}
	hw_dbg(hw, "    RAR[%d] <= %pM.  vmdq:%d enable:0x%x\n", index,
	       addr, vmdq, enable_addr);

	if (adapter->flags & RNPM_FLAG_SRIOV_ENABLED)
		hw->mac.ops.set_vmdq(hw, index, vmdq);
	/*
	 * HW expects these in big endian so we reverse the byte
	 * order from network order (big endian) to little endian
	 */
	rar_low = ((u32)addr[5] | ((u32)addr[4] << 8) |
		   ((u32)addr[3] << 16) | ((u32)addr[2] << 24));
	/*
	 * Some parts put the VMDq setting in the extra RAH bits,
	 * so save everything except the lower 16 bits that hold part
	 * of the address and the address valid bit.
	 */
	rar_high = rd32(hw, RNPM_ETH_RAR_RH(index));
	rar_high &= ~(0x0000FFFF | RNPM_RAH_AV);
	rar_high |= ((u32)addr[1] | ((u32)addr[0] << 8));

	if (enable_addr != 0)
		rar_high |= RNPM_RAH_AV;

	wr32(hw, RNPM_ETH_RAR_RL(index), rar_low);
	wr32(hw, RNPM_ETH_RAR_RH(index), rar_high);

	/* open unicast filter, use unicast, but we must open this since
	 * dest-mac filter | unicast table all packets up if close unicast table
	 */
	mcstctrl = rd32(hw, RNPM_ETH_DMAC_MCSTCTRL);
	mcstctrl |= RNPM_MCSTCTRL_UNICASE_TBL_EN;
	wr32(hw, RNPM_ETH_DMAC_MCSTCTRL, mcstctrl);
	return 0;
}

s32 rnpm_set_rar_mac(struct rnpm_hw *hw, u32 index, u8 *addr, u32 vmdq,
		     u32 port)
{
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	u32 rar_entries = hw->mac.num_rar_entries;
	u32 rar_low, rar_high = 0, mcstctrl;

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries + hw->ncsi_rar_entries) {
		rnpm_err("set_rar_mac RAR index %d is out of range.\n",
			 index);
		return RNPM_ERR_INVALID_ARGUMENT;
	}
	hw_dbg(hw, "port %d RAR[%d] <= %pM. vmdq:%d\n", port, index, addr,
	       vmdq);
	/* setup VMDq pool selection before this RAR gets enabled */
	/* only sriov mode use this */
	if (adapter->flags & RNPM_FLAG_SRIOV_ENABLED)
		hw->mac.ops.set_vmdq(hw, index, vmdq);

	/*
	 * HW expects these in big endian so we reverse the byte
	 * order from network order (big endian) to little endian
	 */
	rar_low = ((u32)addr[0] | ((u32)addr[1] << 8) |
		   ((u32)addr[2] << 16) | ((u32)addr[3] << 24));
	rar_high = RNPM_RAH_AV | ((u32)addr[4] | (u32)addr[5] << 8);
	wr32(hw, RNPM_MAC_UNICAST_HIGH(index, port), rar_high);
	wr32(hw, RNPM_MAC_UNICAST_LOW(index, port), rar_low);

	/* use unicast perfect match */
	mcstctrl = rd32(hw, RNPM_MAC_PKT_FLT(port));
	mcstctrl &= (~RNPM_RX_HUC);
	wr32(hw, RNPM_MAC_PKT_FLT(port), mcstctrl);
	return 0;
}
/**
 *  rnpm_clear_rar_generic - Remove Rx address register
 *  @hw: pointer to hardware structure
 *  @index: Receive address register to write
 *
 *  Clears an ethernet address from a receive address register.
 **/
s32 rnpm_clear_rar_generic(struct rnpm_hw *hw, u32 index)
{
	u32 rar_high;
	u32 rar_entries = hw->mac.num_rar_entries;

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries + hw->ncsi_rar_entries) {
		hw_dbg(hw,
		       "clear_rar_generic RAR index %d is out of range.\n",
		       index);
		return RNPM_ERR_INVALID_ARGUMENT;
	}

	/*
	 * Some parts put the VMDq setting in the extra RAH bits,
	 * so save everything except the lower 16 bits that hold part
	 * of the address and the address valid bit.
	 */
	rar_high = rd32(hw, RNPM_ETH_RAR_RH(index));
	rar_high &= ~(0x0000FFFF | RNPM_RAH_AV);

	wr32(hw, RNPM_ETH_RAR_RL(index), 0);
	wr32(hw, RNPM_ETH_RAR_RH(index), rar_high);

	/* clear VMDq pool/queue selection for this RAR */
	hw->mac.ops.clear_vmdq(hw, index, RNPM_CLEAR_VMDQ_ALL);

	return 0;
}

s32 rnpm_clear_rar_mac(struct rnpm_hw *hw, u32 index, u32 port)
{
	u32 rar_entries = hw->mac.num_rar_entries;

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries + hw->ncsi_rar_entries) {
		hw_dbg(hw, "clear_rar_mac RAR index %d is out of range.\n",
		       index);
		return RNPM_ERR_INVALID_ARGUMENT;
	}
	wr32(hw, RNPM_MAC_UNICAST_LOW(index, port), 0);
	wr32(hw, RNPM_MAC_UNICAST_HIGH(index, port), 0);
	return 0;
}

/**
 *  rnpm_set_mta - Set bit-vector in multicast table
 *  @hw: pointer to hardware structure
 *  @hash_value: Multicast address hash value
 *
 *  Sets the bit-vector in the multicast table.
 **/
static void rnpm_set_mta(struct rnpm_hw *hw, u8 *mc_addr)
{
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	struct rnpm_pf_adapter *pf_adapter = adapter->pf_adapter;
	u8 port = adapter->port;
	u32 vector;
	u32 vector_bit;
	u32 vector_reg;

	if (hw->mac.mc_location == rnpm_mc_location_nic)
		pf_adapter->mta_in_use[port]++;
	hw->addr_ctrl.mta_in_use++;
	vector = rnpm_mta_vector(hw->mac.mc_filter_type, mc_addr);

	/* low 5 bits indicate bit pos
	 * high 3 (mac) or 7 (nic) bits indicate reg pos
	 */
	vector_reg = (vector >> 5) & 0x7F;
	vector_bit = vector & 0x1F;
	hw_dbg(hw, "\t\t%pM: MTA-BIT:%4d, MTA_REG[%d][%d] <= 1\n", mc_addr,
	       vector, vector_reg, vector_bit);
	if (hw->mac.mc_location == rnpm_mc_location_nic)
		pf_adapter->mta_shadow[vector_reg] |= (1 << vector_bit);
	hw->mac.mta_shadow[vector_reg] |= (1 << vector_bit);
}

static int __get_ncsi_shm_info(struct rnpm_hw *hw,
			       struct ncsi_shm_info *ncsi_shm)
{
	int rbytes = round_up(sizeof(*ncsi_shm), 4);
	int *ptr = (int *)ncsi_shm;
	int i;

	memset(ncsi_shm, 0, sizeof(*ncsi_shm));
	for (i = 0; i < (rbytes / 4); i++)
		ptr[i] = rd32(hw, hw->ncsi_vf_cpu_shm_pf_base + 4 * i);
	return (ncsi_shm->valid & RNPM_NCSI_SHM_VALID_MASK) ==
	       RNPM_NCSI_SHM_VALID;
}

void rnpm_ncsi_set_uc_addr_generic(struct rnpm_hw *hw)
{
	struct ncsi_shm_info ncsi_shm;
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	u8 mac[ETH_ALEN];

	if (!hw->ncsi_en || !__get_ncsi_shm_info(hw, &ncsi_shm))
		return;

	if (ncsi_shm.valid & RNPM_MC_VALID) {
		mac[0] = ncsi_shm.uc.uc_addr_lo & 0xff;
		mac[1] = (ncsi_shm.uc.uc_addr_lo >> 8) & 0xff;
		mac[2] = (ncsi_shm.uc.uc_addr_lo >> 16) & 0xff;
		mac[3] = (ncsi_shm.uc.uc_addr_lo >> 24) & 0xff;
		mac[4] = ncsi_shm.uc.uc_addr_hi & 0xff;
		mac[5] = (ncsi_shm.uc.uc_addr_hi >> 8) & 0xff;
		if (is_valid_ether_addr(mac)) {
			hw->mac.ops.set_rar(hw, hw->mac.num_rar_entries,
					    mac, VMDQ_P(0), RNPM_RAH_AV);
			if (hw->mac.mc_location == rnpm_mc_location_mac) {
				/* ncsi use the last mac addr entries on per nic mac */
				hw->mac.ops.set_rar_mac(hw, 31, mac,
							VMDQ_P(0),
							adapter->port);
			}
		}
	}
}

void rnpm_ncsi_set_mc_mta_generic(struct rnpm_hw *hw)
{
	struct ncsi_shm_info ncsi_shm;
	u8 mac[ETH_ALEN], i;

	if (!hw->ncsi_en || !__get_ncsi_shm_info(hw, &ncsi_shm))
		return;
	if (ncsi_shm.valid & RNPM_MC_VALID) {
		for (i = 0; i < RNPM_NCSI_MC_COUNT; i++) {
			mac[0] = ncsi_shm.mc[i].mc_addr_lo & 0xff;
			mac[1] = (ncsi_shm.mc[i].mc_addr_lo >> 8) & 0xff;
			mac[2] = (ncsi_shm.mc[i].mc_addr_lo >> 16) & 0xff;
			mac[3] = (ncsi_shm.mc[i].mc_addr_lo >> 24) & 0xff;
			mac[4] = ncsi_shm.mc[i].mc_addr_hi & 0xff;
			mac[5] = (ncsi_shm.mc[i].mc_addr_hi >> 8) & 0xff;
			if (is_multicast_ether_addr(mac) &&
			    !is_zero_ether_addr(mac))
				rnpm_set_mta(hw, mac);
		}
	}
}

void rnpm_ncsi_set_vfta_mac_generic(struct rnpm_hw *hw)
{
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	struct ncsi_shm_info ncsi_shm;

	if (!hw->ncsi_en || !__get_ncsi_shm_info(hw, &ncsi_shm))
		return;
	if (ncsi_shm.valid & RNPM_VLAN_VALID)
		hw->mac.ops.set_vfta_mac(hw, ncsi_shm.ncsi_vlan, VMDQ_P(0),
					 true);
}

/**
 *  rnpm_init_rx_addrs_generic - Initializes receive address filters.
 *  @hw: pointer to hardware structure
 *
 *  Places the MAC address in receive address register 0 and clears the rest
 *  of the receive address registers. Clears the multicast table. Assumes
 *  the receiver is in reset when the routine is called.
 **/
s32 rnpm_init_rx_addrs_generic(struct rnpm_hw *hw)
{
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	u32 rar_entries = adapter->uc_num;
	u8 port = adapter->port;
	u32 i, v;

	hw_dbg(hw, "init_rx_addrs:rar_entries:%d, mac.addr:%pM\n",
	       rar_entries, hw->mac.addr);
	if (!is_valid_ether_addr(hw->mac.addr)) {
		/* Get the MAC address from the RAR0 for later reference */
		hw->mac.ops.get_mac_addr(hw, hw->mac.addr);
		hw_dbg(hw, " Keeping Current RAR0 Addr =%pM\n",
		       hw->mac.addr);
	} else {
		/* Setup the receive address. */
		hw_dbg(hw, "Overriding MAC Address in RAR[0]\n");
		hw_dbg(hw, " New MAC Addr =%pM\n", hw->mac.addr);
		hw->mac.ops.set_rar(hw, adapter->uc_off, hw->mac.addr, 0,
				    RNPM_RAH_AV);
		/*  clear VMDq pool/queue selection for RAR 0 */
		hw->mac.ops.clear_vmdq(hw, 0, RNPM_CLEAR_VMDQ_ALL);
	}

	hw->addr_ctrl.overflow_promisc = 0;
	hw->addr_ctrl.rar_used_count = 1;
	/* Zero out the other receive addresses. */
	hw_dbg(hw, "Clearing RAR[%d-%d]\n", adapter->uc_off + 1,
	       rar_entries + adapter->uc_off - 1);
	for (i = adapter->uc_off + 1; i < rar_entries; i++) {
		wr32(hw, RNPM_ETH_RAR_RL(i), 0);
		wr32(hw, RNPM_ETH_RAR_RH(i), 0);
	}

	if (hw->mac.mc_location == rnpm_mc_location_mac)
		for (i = 1; i < adapter->uc_num; i++) {
			wr32(hw, RNPM_MAC_UNICAST_HIGH(i, port), 0);
			wr32(hw, RNPM_MAC_UNICAST_LOW(i, port), 0);
		}

	/* Clear the MTA */
	hw->addr_ctrl.mta_in_use = 0;

	if (hw->mac.mc_location == rnpm_mc_location_nic) {
		v = rd32(hw, RNPM_ETH_DMAC_MCSTCTRL);
		v &= (~(RNPM_MCSTCTRL_MULTICASE_TBL_EN |
			RNPM_MCSTCTRL_UNICASE_TBL_EN));
		v |= hw->mac.mc_filter_type;
		wr32(hw, RNPM_ETH_DMAC_MCSTCTRL, v);

		hw_dbg(hw, " Clearing MTA\n");
		for (i = 0; i < hw->mac.mcft_size; i++)
			wr32(hw, RNPM_MTA(i), 0);
	} else {
		v = rd32(hw, RNPM_MAC_PKT_FLT(port));
		v &= (~RNPM_FLT_HUC);
		wr32(hw, RNPM_MAC_PKT_FLT(port), v);

		hw_dbg(hw, " Clearing MTA\n");
		for (i = 0; i < hw->mac.mcft_size; i++)
			wr32(hw, RNPM_MAC_MC_HASH_TABLE(port, i), 0);

		if (hw->ncsi_en) {
			rnpm_ncsi_set_mc_mta_generic(hw);
			for (i = 0; i < hw->mac.mcft_size; i++)
				wr32(hw, RNPM_MAC_MC_HASH_TABLE(port, i),
				     hw->mac.mta_shadow[i]);
			/* Set ncsi vlan */
			rnpm_ncsi_set_vfta_mac_generic(hw);
		}
	}

	if (hw->mac.ops.init_uta_tables)
		hw->mac.ops.init_uta_tables(hw);

	if (hw->ncsi_en)
		rnpm_ncsi_set_uc_addr_generic(hw);

	return 0;
}
static u32 rnpm_calc_crc32(u32 seed, u8 *mac, u32 len)
{
#define RNPM_CRC32_POLY_LE 0xedb88320
	u32 crc = seed;
	u32 i;

	while (len--) {
		crc ^= *mac++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^
			      ((crc & 1) ? RNPM_CRC32_POLY_LE : 0);
	}

	return crc;
}

/**
 *  rnpm_mta_vector - Determines bit-vector in multicast table to set
 *  @hw: pointer to hardware structure
 *  @mc_addr: the multicast address
 *
 *  Extracts the 12 bits, from a multicast address, to determine which
 *  bit-vector to set in the multicast table. The hardware uses 12 bits, from
 *  incoming rx multicast addresses, to determine the bit-vector to check in
 *  the MTA. Which of the 4 combination, of 12-bits, the hardware uses is set
 *  by the MO field of the MCSTCTRL. The MO field is set during initialization
 *  to mc_filter_type.
 **/
static s32 rnpm_mta_vector(int mode, u8 *mc_addr)
{
	u32 vector = 0;

	switch (mode) {
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
	case 4:
		/* hash is used for multicast address, only high 8 bits used */
#define DEFAULT_MAC_LEN (6)
		vector = bitrev32(
			~rnpm_calc_crc32(~0, mc_addr, DEFAULT_MAC_LEN));
		vector = vector >> 24;
		break;
	default: /* Invalid mc_filter_type */
		hw_dbg(hw, "MC filter type param set incorrectly\n");
		break;
	}

	/* vector can only be 12-bits or boundary will be exceeded */
	vector &= 0xFFF;
	return vector;
}

static u8 *rnpm_addr_list_itr(struct rnpm_hw __maybe_unused *hw,
			      u8 **mc_addr_ptr)
{
	struct netdev_hw_addr *mc_ptr;
	u8 *addr = *mc_addr_ptr;

	mc_ptr = container_of(addr, struct netdev_hw_addr, addr[0]);
	if (mc_ptr->list.next) {
		struct netdev_hw_addr *ha;

		ha = list_entry(mc_ptr->list.next, struct netdev_hw_addr,
				list);
		*mc_addr_ptr = ha->addr;
	} else
		*mc_addr_ptr = NULL;

	return addr;
}

/**
 *  rnpm_update_mc_addr_list_generic - Updates MAC list of multicast addresses
 *  @hw: pointer to hardware structure
 *  @netdev: pointer to net device structure
 *
 *  The given list replaces any existing list. Clears the MC addrs from receive
 *  address registers and the multicast table. Uses unused receive address
 *  registers for the first multicast addresses, and hashes the rest into the
 *  multicast table.
 **/
s32 rnpm_update_mutiport_mc_addr_list_generic(struct rnpm_hw *hw,
					      struct net_device *netdev)
{
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	struct rnpm_pf_adapter *pf_adapter = adapter->pf_adapter;
	struct netdev_hw_addr *ha;
	u32 i, v;
	u8 port = adapter->port;
	int addr_count = 0;
	u8 *addr_list = NULL;
	unsigned long flags;

	/*
	 * Set the new number of MC addresses that we are being requested to
	 * use.
	 */
	pf_adapter->num_mc_addrs[port] = netdev_mc_count(netdev);
	pf_adapter->mta_in_use[port] = 0;

	hw->addr_ctrl.num_mc_addrs = netdev_mc_count(netdev);
	hw->addr_ctrl.mta_in_use = 0;

	/* Clear share mta_shadow only not in mutiport mode */
	if (!(adapter->flags & RNPM_FLAG_MUTIPORT_ENABLED)) {
		hw_dbg(hw, " Clearing MTA(multicast table)\n");
		memset(&pf_adapter->mta_shadow, 0,
		       sizeof(pf_adapter->mta_shadow));
	}
	/* clear own mta_shadow */
	memset(&hw->mac.mta_shadow, 0, sizeof(hw->mac.mta_shadow));

	spin_lock_irqsave(&pf_adapter->mc_setup_lock, flags);
	/* Update mta shadow */
	hw_dbg(hw, "port %d Updating MTA..\n", port);
	addr_count = netdev_mc_count(netdev);

	ha = list_first_entry(&netdev->mc.list, struct netdev_hw_addr,
			      list);
	addr_list = ha->addr;
	for (i = 0; i < addr_count; i++) {
		hw_dbg(hw, " Adding the multicast addresses:\n");
		rnpm_set_mta(hw, rnpm_addr_list_itr(hw, &addr_list));
	}

	/* unicast and multicast use the same hash table */
	if (hw->ncsi_en)
		rnpm_ncsi_set_mc_mta_generic(hw);

	/* update mta table to the corect location */
	if (hw->mac.mc_location == rnpm_mc_location_mac) {
		for (i = 0; i < hw->mac.mcft_size; i++)
			if (hw->addr_ctrl.mta_in_use)
				wr32(hw, RNPM_MAC_MC_HASH_TABLE(port, i),
				     hw->mac.mta_shadow[i]);
	} else {
		for (i = 0; i < pf_adapter->mcft_size; i++)
			if (pf_adapter->mta_in_use[port])
				wr32(hw, RNPM_ETH_MUTICAST_HASH_TABLE(i),
				     pf_adapter->mta_shadow[i]);
	}
	spin_unlock_irqrestore(&pf_adapter->mc_setup_lock, flags);

	if (hw->mac.mc_location == rnpm_mc_location_nic) {
		if (pf_adapter->mta_in_use[port] > 0) {
			v = rd32(hw, RNPM_ETH_DMAC_MCSTCTRL);
			wr32(hw, RNPM_ETH_DMAC_MCSTCTRL,
			     v | RNPM_MCSTCTRL_MULTICASE_TBL_EN |
				     pf_adapter->mc_filter_type);
		}

		adapter->flags_feature |=
			RNPM_FLAG_DELAY_UPDATE_MUTICAST_TABLE;
		hw_dbg(hw, "nic mode update MTA Done. mta_in_use:%d\n",
		       pf_adapter->mta_in_use[port]);
		return pf_adapter->mta_in_use[port];
	}

	if (hw->addr_ctrl.mta_in_use) {
		v = rd32(hw, RNPM_MAC_PKT_FLT(port));
		v |= RNPM_FLT_HMC;
		wr32(hw, RNPM_MAC_PKT_FLT(port), v);
	}
	hw_dbg(hw, "mac mode update MTA Done. mta_in_use:%d\n",
	       hw->addr_ctrl.mta_in_use);
	return hw->addr_ctrl.mta_in_use;
}

/**
 *  rnpm_update_mc_addr_list_generic - Updates MAC list of multicast addresses
 *  @hw: pointer to hardware structure
 *  @netdev: pointer to net device structure
 *
 *  The given list replaces any existing list. Clears the MC addrs from receive
 *  address registers and the multicast table. Uses unused receive address
 *  registers for the first multicast addresses, and hashes the rest into the
 *  multicast table.
 **/
s32 rnpm_update_mc_addr_list_generic(struct rnpm_hw *hw,
				     struct net_device *netdev)
{
	struct netdev_hw_addr *ha;
	u32 i, v;
	int addr_count = 0;
	u8 *addr_list = NULL;

	/*
	 * Set the new number of MC addresses that we are being requested to
	 * use.
	 */
	hw->addr_ctrl.num_mc_addrs = netdev_mc_count(netdev);
	hw->addr_ctrl.mta_in_use = 0;

	/* Clear mta_shadow */
	hw_dbg(hw, " Clearing MTA(multicast table)\n");
	memset(&hw->mac.mta_shadow, 0, sizeof(hw->mac.mta_shadow));

	/* Update mta shadow */
	hw_dbg(hw, " Updating MTA..\n");
	addr_count = netdev_mc_count(netdev);

	ha = list_first_entry(&netdev->mc.list, struct netdev_hw_addr,
			      list);
	addr_list = ha->addr;
	for (i = 0; i < addr_count; i++) {
		hw_dbg(hw, " Adding the multicast addresses:\n");
		rnpm_set_mta(hw, rnpm_addr_list_itr(hw, &addr_list));
	}

	/* Enable mta */
	for (i = 0; i < hw->mac.mcft_size; i++)
		if (hw->addr_ctrl.mta_in_use)
			wr32(hw, RNPM_ETH_MUTICAST_HASH_TABLE(i),
			     hw->mac.mta_shadow[i]);

	if (hw->addr_ctrl.mta_in_use > 0) {
		v = rd32(hw, RNPM_ETH_DMAC_MCSTCTRL);
		wr32(hw, RNPM_ETH_DMAC_MCSTCTRL,
		     v | RNPM_MCSTCTRL_MULTICASE_TBL_EN |
			     hw->mac.mc_filter_type);
	}

	hw_dbg(hw, " update MTA Done. mta_in_use:%d\n",
	       hw->addr_ctrl.mta_in_use);
	return hw->addr_ctrl.mta_in_use;
}

/**
 *  rnpm_enable_mc_generic - Enable multicast address in RAR
 *  @hw: pointer to hardware structure
 *
 *  Enables multicast address in RAR and the use of the multicast hash table.
 **/
s32 rnpm_enable_mc_generic(struct rnpm_hw *hw)
{
	struct rnpm_addr_filter_info *a = &hw->addr_ctrl;
	u32 v;
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	u8 port = adapter->port;

	if (a->mta_in_use > 0) {
		if (hw->mac.mc_location == rnpm_mc_location_nic) {
			v = rd32(hw, RNPM_ETH_DMAC_MCSTCTRL);
			v |= RNPM_MCSTCTRL_MULTICASE_TBL_EN;
			wr32(hw, RNPM_ETH_DMAC_MCSTCTRL, v);
		} else {
			v = rd32(hw, RNPM_MAC_PKT_FLT(port));
			v |= RNPM_FLT_HMC;
			wr32(hw, RNPM_MAC_PKT_FLT(port), v);
		}
	}

	return 0;
}

/**
 *  rnpm_disable_mc_generic - Disable multicast address in RAR
 *  @hw: pointer to hardware structure
 *
 *  Disables multicast address in RAR and the use of the multicast hash table.
 **/
s32 rnpm_disable_mc_generic(struct rnpm_hw *hw)
{
	struct rnpm_addr_filter_info *a = &hw->addr_ctrl;
	u32 v;

	if (a->mta_in_use > 0) {
		v = rd32(hw, RNPM_ETH_DMAC_MCSTCTRL);
		v &= ~RNPM_MCSTCTRL_MULTICASE_TBL_EN;
		wr32(hw, RNPM_ETH_DMAC_MCSTCTRL, v);
	}

	return 0;
}

/**
 *  rnpm_fc_enable_generic - Enable flow control
 *  @hw: pointer to hardware structure
 *
 *  Enable flow control according to the current settings.
 **/
s32 rnpm_fc_enable_generic(struct rnpm_hw *hw)
{
	s32 ret_val = 0;
	u32 reg;
	u32 rxctl_reg, txctl_reg[RNPM_MAX_TRAFFIC_CLASS];
	int i;
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	u8 port = adapter->port;

	hw->fc.current_mode = hw->fc.requested_mode;
	/*
	 * Validate the water mark configuration for packet buffer 0.  Zero
	 * water marks indicate that the packet buffer was not configured
	 * and the watermarks for packet buffer 0 should always be configured.
	 */
	if (!hw->fc.pause_time) {
		ret_val = RNPM_ERR_INVALID_LINK_SETTINGS;
		goto out;
	}

	for (i = 0; i < RNPM_MAX_TRAFFIC_CLASS; i++) {
		if ((hw->fc.current_mode & rnpm_fc_tx_pause) &&
		    hw->fc.high_water[i]) {
			if (!hw->fc.low_water[i] ||
			    hw->fc.low_water[i] >= hw->fc.high_water[i]) {
				hw_dbg(hw,
				       "Invalid water mark configuration\n");
				ret_val = RNPM_ERR_INVALID_LINK_SETTINGS;
				goto out;
			}
		}
	}

	/* Negotiate the fc mode to use */
	rnpm_fc_autoneg(hw);

	/* Disable any previous flow control settings */
	rxctl_reg = rd32(hw, RNPM_MAC_RX_FLOW_CTRL(port));
	rxctl_reg &= (~RNPM_RX_FLOW_ENABLE_MASK);

	for (i = 0; i < RNPM_MAX_TRAFFIC_CLASS; i++) {
		txctl_reg[i] = rd32(hw, RNPM_MAC_Q0_TX_FLOW_CTRL(port, i));
		txctl_reg[i] &= (~RNPM_TX_FLOW_ENABLE_MASK);
	}

	switch (hw->fc.current_mode) {
	case rnpm_fc_none:
		break;
	case rnpm_fc_rx_pause:
		rxctl_reg |= (RNPM_RX_FLOW_ENABLE_MASK);
		break;
	case rnpm_fc_tx_pause:
		for (i = 0; i < RNPM_MAX_TRAFFIC_CLASS; i++)
			txctl_reg[i] |= (RNPM_TX_FLOW_ENABLE_MASK);
		break;
	case rnpm_fc_full:
		/* Flow control (both Rx and Tx) is enabled by SW override. */
		rxctl_reg |= (RNPM_RX_FLOW_ENABLE_MASK);
		for (i = 0; i < RNPM_MAX_TRAFFIC_CLASS; i++)
			txctl_reg[i] |= (RNPM_TX_FLOW_ENABLE_MASK);
		break;
	default:
		hw_dbg(hw, "Flow control mac param set incorrectly\n");
		ret_val = RNPM_ERR_CONFIG;
		goto out;
	}
	/* Set up and enable Rx high/low water mark thresholds, enable XON. */
	for (i = 0; i < RNPM_MAX_TRAFFIC_CLASS; i++) {
		if ((hw->fc.current_mode & rnpm_fc_tx_pause)) {
			if (hw->fc.high_water[i])
				wr32(hw, RNPM_ETH_HIGH_WATER(i),
				     hw->fc.high_water[i]);
			if (hw->fc.low_water[i])
				wr32(hw, RNPM_ETH_LOW_WATER(i),
				     hw->fc.low_water[i]);
		}
	}

	/* Configure pause time (2 TCs per register) */
	reg = hw->fc.pause_time;
	for (i = 0; i < (RNPM_MAX_TRAFFIC_CLASS); i++)
		txctl_reg[i] |= (reg << 16);

	/* Set 802.3x based flow control settings. */
	wr32(hw, RNPM_MAC_RX_FLOW_CTRL(port), rxctl_reg);
	for (i = 0; i < (RNPM_MAX_TRAFFIC_CLASS); i++)
		wr32(hw, RNPM_MAC_Q0_TX_FLOW_CTRL(port, i), txctl_reg[i]);
out:
	return ret_val;
}

/**
 *  rnpm_negotiate_fc - Negotiate flow control
 *  @hw: pointer to hardware structure
 *  @adv_reg: flow control advertised settings
 *  @lp_reg: link partner's flow control settings
 *  @adv_sym: symmetric pause bit in advertisement
 *  @adv_asm: asymmetric pause bit in advertisement
 *  @lp_sym: symmetric pause bit in link partner advertisement
 *  @lp_asm: asymmetric pause bit in link partner advertisement
 *
 *  Find the intersection between advertised settings and link partner's
 *  advertised settings
 **/
__maybe_unused static s32 rnpm_negotiate_fc(struct rnpm_hw *hw,
					    u32 adv_reg, u32 lp_reg,
					    u32 adv_sym, u32 adv_asm,
					    u32 lp_sym, u32 lp_asm)
{
	if (!adv_reg || !lp_reg)
		return RNPM_ERR_FC_NOT_NEGOTIATED;

	if ((adv_reg & adv_sym) && (lp_reg & lp_sym)) {
		/*
		 * Now we need to check if the user selected Rx ONLY
		 * of pause frames.  In this case, we had to advertise
		 * FULL flow control because we could not advertise RX
		 * ONLY. Hence, we must now check to see if we need to
		 * turn OFF the TRANSMISSION of PAUSE frames.
		 */
		if (hw->fc.requested_mode == rnpm_fc_full) {
			hw->fc.current_mode = rnpm_fc_full;
			hw_dbg(hw, "Flow Control = FULL.\n");
		} else {
			hw->fc.current_mode = rnpm_fc_rx_pause;
			hw_dbg(hw, "Flow Control=RX PAUSE frames only\n");
		}
	} else if (!(adv_reg & adv_sym) && (adv_reg & adv_asm) &&
		   (lp_reg & lp_sym) && (lp_reg & lp_asm)) {
		hw->fc.current_mode = rnpm_fc_tx_pause;
		hw_dbg(hw, "Flow Control = TX PAUSE frames only.\n");
	} else if ((adv_reg & adv_sym) && (adv_reg & adv_asm) &&
		   !(lp_reg & lp_sym) && (lp_reg & lp_asm)) {
		hw->fc.current_mode = rnpm_fc_rx_pause;
		hw_dbg(hw, "Flow Control = RX PAUSE frames only.\n");
	} else {
		hw->fc.current_mode = rnpm_fc_none;
		hw_dbg(hw, "Flow Control = NONE.\n");
	}
	return 0;
}

/**
 *  rnpm_fc_autoneg_copper - Enable flow control IEEE clause 37
 *  @hw: pointer to hardware structure
 *
 *  Enable flow control according to IEEE clause 37.
 **/
static s32 rnpm_fc_autoneg_copper(struct rnpm_hw *hw)
{
	u16 technology_ability_reg = 0;
	u16 lp_technology_ability_reg = 0;

	hw->phy.ops.read_reg(hw, 4, 0, &technology_ability_reg);
	hw->phy.ops.read_reg(hw, 5, 0, &lp_technology_ability_reg);

	return rnpm_negotiate_fc(hw, (u32)technology_ability_reg,
				 (u32)lp_technology_ability_reg,
				 RNPM_TAF_SYM_PAUSE, RNPM_TAF_ASM_PAUSE,
				 RNPM_TAF_SYM_PAUSE, RNPM_TAF_ASM_PAUSE);
}

/**
 *  rnpm_fc_autoneg - Configure flow control
 *  @hw: pointer to hardware structure
 *
 *  Compares our advertised flow control capabilities to those advertised by
 *  our link partner, and determines the proper flow control mode to use.
 **/
void rnpm_fc_autoneg(struct rnpm_hw *hw)
{
	s32 ret_val = RNPM_ERR_FC_NOT_NEGOTIATED;

	switch (hw->phy.media_type) {
	case rnpm_media_type_fiber:
		break;
	case rnpm_media_type_backplane:
		break;
	case rnpm_media_type_copper:
		if (rnpm_device_supports_autoneg_fc(hw))
			ret_val = rnpm_fc_autoneg_copper(hw);
		break;
	default:
		break;
	}

	if (ret_val == 0) {
		hw->fc.fc_was_autonegged = true;
	} else {
		hw->fc.fc_was_autonegged = false;
		hw->fc.current_mode = hw->fc.requested_mode;
	}
}

/**
 *  rnpm_clear_vmdq_generic - Disassociate a VMDq pool index from a rx address
 *  @hw: pointer to hardware struct
 *  @rar: receive address register index to disassociate
 *  @vmdq: VMDq pool index to remove from the rar
 **/
s32 rnpm_clear_vmdq_generic(struct rnpm_hw *hw, u32 rar, u32 vmdq)
{
	u32 rar_entries = hw->mac.num_rar_entries;

	/* Make sure we are using a valid rar index range */
	if (rar >= rar_entries + hw->ncsi_rar_entries) {
		hw_dbg(hw,
		       "clear_vmdq_generic RAR index %d is out of range.\n",
		       rar);
		return RNPM_ERR_INVALID_ARGUMENT;
	}

	wr32(hw, RNPM_VM_DMAC_MPSAR_RING(rar), 0);

	return 0;
}

/**
 *  rnpm_set_vmdq_generic - Associate a VMDq pool index with a rx address
 *  @hw: pointer to hardware struct
 *  @rar: receive address register index to associate with a VMDq index
 *  @vmdq: VMDq pool index
 **/
s32 rnpm_set_vmdq_generic(struct rnpm_hw *hw, u32 rar, u32 vmdq)
{
	u32 rar_entries = hw->mac.num_rar_entries;

	/* Make sure we are using a valid rar index range */
	if (rar >= rar_entries + hw->ncsi_rar_entries) {
		hw_dbg(hw,
		       "set_vmdq_generic RAR index %d is out of range.\n",
		       rar);
		return RNPM_ERR_INVALID_ARGUMENT;
	}
	wr32(hw, RNPM_VM_DMAC_MPSAR_RING(rar), vmdq);
	return 0;
}

/**
 *  rnpm_init_uta_tables_generic - Initialize the Unicast Table Array
 *  @hw: pointer to hardware structure
 **/
s32 rnpm_init_uta_tables_generic(struct rnpm_hw *hw)
{
	int i;

	for (i = 0; i < hw->mac.num_rar_entries; i++)
		wr32(hw, RNPM_ETH_UTA(i), 0);
	return 0;
}

/**
 *  rnpm_find_vlvf_slot - find the vlanid or the first empty slot
 *  @hw: pointer to hardware structure
 *  @vlan: VLAN id to write to VLAN filter
 *
 *  return the VLVF index where this VLAN id should be placed
 *
 **/
__maybe_unused static s32 rnpm_find_vlvf_slot(struct rnpm_hw *hw, u32 vlan)
{
	u32 bits = 0;
	u32 first_empty_slot = 0;
	s32 regindex = -1;

	/* short cut the special case */
	if (vlan == 0)
		return 0;

	/*
	 * Search for the vlan id in the VLVF entries. Save off the first empty
	 * slot found along the way
	 */
	for (regindex = 1; regindex < RNPM_VLVF_ENTRIES; regindex++) {
		bits = rd32(hw, RNPM_VLVF(regindex));
		if (!bits && !(first_empty_slot))
			first_empty_slot = regindex;
		else if ((bits & 0x0FFF) == vlan)
			break;
	}

	/*
	 * If regindex is less than RNPM_VLVF_ENTRIES, then we found the vlan
	 * in the VLVF. Else use the first empty VLVF register for this
	 * vlan id.
	 */
	if (regindex >= RNPM_VLVF_ENTRIES) {
		if (first_empty_slot)
			regindex = first_empty_slot;
		else {
			hw_dbg(hw, "No space in VLVF.\n");
			regindex = RNPM_ERR_NO_SPACE;
		}
	}
	return regindex;
}

s32 rnpm_set_vfta_mac_generic(struct rnpm_hw *hw, u32 vlan, u32 vind,
			      bool vlan_on)
{
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	u8 port = adapter->port;
	u32 value, vector;
	u16 vid;

	if (vlan > 4095)
		return RNPM_ERR_PARAM;

	value = rd32(hw, RNPM_MAC_VLAN_HASH_TB(port));

	vid = cpu_to_le16(vlan);
	vector = bitrev32(~rnpm_vid_crc32_le(vid));
	vector = vector >> 28;
	value |= (1 << vector);
	wr32(hw, RNPM_MAC_VLAN_HASH_TB(port), value);
	return 0;
}

/**
 *  rnpm_set_vfta_generic - Set VLAN filter table
 *  @hw: pointer to hardware structure
 *  @vlan: VLAN id to write to VLAN filter
 *  @vind: VMDq output index that maps queue to VLAN id in VFVFB
 *  @vlan_on: boolean flag to turn on/off VLAN in VFVF
 *
 *  Turn on/off specified VLAN in the VLAN filter table.
 **/
s32 rnpm_set_vfta_generic(struct rnpm_hw *hw, u32 vlan, u32 vind,
			  bool vlan_on)
{
	bool vfta_changed = false;
	s32 regindex;
	u32 bitindex;
	u32 vfta;
	u32 targetbit;

	if (vlan > 4095)
		return RNPM_ERR_PARAM;

	regindex = (vlan >> 5) & 0x7F;
	bitindex = vlan & 0x1F;
	targetbit = (1 << bitindex);
	vfta = rd32(hw, RNPM_VFTA(regindex));

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
		wr32(hw, RNPM_VFTA(regindex), vfta);
	return 0;
}

/**
 *  rnpm_clear_vfta_generic - Clear VLAN filter table
 *  @hw: pointer to hardware structure
 *
 *  Clears the VLAN filer table, and the VMDq index associated with the filter
 **/
s32 rnpm_clear_vfta_generic(struct rnpm_hw *hw)
{
	u32 offset;

	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	/* clear vlan table not in mutiport mode */
	if (!(adapter->flags & RNPM_FLAG_MUTIPORT_ENABLED)) {
		for (offset = 0; offset < hw->mac.vft_size; offset++)
			wr32(hw, RNPM_VFTA(offset), 0);
		for (offset = 0; offset < RNPM_VLVF_ENTRIES; offset++)
			wr32(hw, RNPM_VLVF(offset), 0);
	}
	return 0;
}

#define RNPM_GET_FIBER_SPEED_CUSTOM_FOR_H3C (1)
#define __GET_PCS_REG_BY_MBX (1)
static u32 rnpm_fiber_get_speed_info_from_pcs(struct rnpm_hw *hw)
{
#if !__GET_PCS_REG_BY_MBX
	struct rnpm_pcs_info *pcs = &hw->pcs;
#endif
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	u32 speed = 0;
	u32 status;
	int ret = 0;

#if !RNPM_GET_FIBER_SPEED_CUSTOM_FOR_H3C
	return 0;
#endif
	if (hw->is_sgmii)
		return 0;

#if __GET_PCS_REG_BY_MBX
	ret = rnpm_fw_get_pcs_reg(hw, adapter->port, RNPM_PCS_LINK_SPEED,
				  &status);
	if (ret < 0)
		return ret;
#else
	status = pcs->ops.read(hw, adapter->port, RNPM_PCS_LINK_SPEED);
#endif
	if (status & RNPM_PCS_1G_OR_10G) {
		switch (status & RNPM_PCS_SPPEED_MASK) {
		case RNPM_PCS_SPPEED_10G:
			speed = RNPM_LINK_SPEED_10GB_FULL;
			break;
		case RNPM_PCS_SPPEED_40G:
			break;
		}
	} else {
		speed = RNPM_LINK_SPEED_1GB_FULL;
	}

	return speed;
}

/**
 *  rnpm_check_mac_link_generic - Determine link and speed status
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @link_up: true when link is up
 *  @link_up_wait_to_complete: bool used to wait for link up or not
 *
 *  Reads the links register to determine if link is up and the current speed
 **/
s32 rnpm_check_mac_link_generic(struct rnpm_hw *hw, rnpm_link_speed *speed,
				bool *link_up,
				bool link_up_wait_to_complete)
{
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	u32 pcs_speed = 0;

	rnpm_logd(LOG_FUNC_ENTER, "enter %s %s\n", __func__,
		  adapter->netdev->name);
	hw->speed = adapter->speed;
	if (hw->speed == SPEED_10)
		*speed = RNPM_LINK_SPEED_10_FULL;
	else if (hw->speed == SPEED_100)
		*speed = RNPM_LINK_SPEED_100_FULL;
	else if (hw->speed == SPEED_1000)
		*speed = RNPM_LINK_SPEED_1GB_FULL;
	else if (hw->speed == SPEED_10000)
		*speed = RNPM_LINK_SPEED_10GB_FULL;
	else if (hw->speed == SPEED_40000)
		*speed = RNPM_LINK_SPEED_40GB_FULL;
	else
		*speed = RNPM_LINK_SPEED_UNKNOWN;

	if (!hw->is_sgmii) {
		if (hw->speed != adapter->pf_adapter->hw.ablity_speed) {
			pcs_speed = rnpm_fiber_get_speed_info_from_pcs(hw);
			if (pcs_speed > 0) {
				if (pcs_speed == RNPM_LINK_SPEED_1GB_FULL)
					hw->speed = SPEED_1000;
				else if (pcs_speed ==
					 RNPM_LINK_SPEED_10GB_FULL)
					hw->speed = SPEED_10000;
				adapter->speed = hw->speed;
				*speed = pcs_speed;
			}
		}
	}
	*link_up = hw->link;
	rnpm_logd(LOG_FUNC_ENTER, "exit %s %s\n", __func__,
		  adapter->netdev->name);
	return 0;
}

/**
 *  rnpm_calculate_checksum - Calculate checksum for buffer
 *  @buffer: pointer to EEPROM
 *  @length: size of EEPROM to calculate a checksum for
 *
 *  Calculates the checksum for some buffer on a specified length.  The
 *  checksum calculated is returned.
 **/
__maybe_unused static u8 rnpm_calculate_checksum(u8 *buffer, u32 length)
{
	u32 i;
	u8 sum = 0;

	if (!buffer)
		return 0;

	for (i = 0; i < length; i++)
		sum += buffer[i];

	return (u8)(0 - sum);
}

/**
 * rnpm_get_thermal_sensor_data_generic - Gathers thermal sensor data
 * @hw: pointer to hardware structure
 *
 * Returns the thermal sensor data structure
 **/
s32 rnpm_get_thermal_sensor_data_generic(struct rnpm_hw *hw)
{
	struct rnpm_thermal_sensor_data *data =
		&hw->mac.thermal_sensor_data;
	int voltage = 0;

	data->sensor[0].temp = rnpm_mbx_get_temp(hw, &voltage);

	return 0;
}

/**
 * rnpm_init_thermal_sensor_thresh_generic - Inits thermal sensor thresholds
 * @hw: pointer to hardware structure
 *
 * Inits the thermal sensor thresholds according to the NVM map
 * and save off the threshold and location values into mac.thermal_sensor_data
 **/
s32 rnpm_init_thermal_sensor_thresh_generic(struct rnpm_hw *hw)
{
	u8 i;
	struct rnpm_thermal_sensor_data *data =
		&hw->mac.thermal_sensor_data;

	for (i = 0; i < RNPM_MAX_SENSORS; i++) {
		data->sensor[i].location = i + 1;
		data->sensor[i].caution_thresh = 100;
		data->sensor[i].max_op_thresh = 115;
	}

	return 0;
}
