/*******************************************************************************

  WangXun(R) GbE PCI Express Virtual Function Linux Network Driver
  Copyright(c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Software Team <linux.nic@trustnetic.com>
  WangXun Technology, HuaXing Times Square A507, Hangzhou, China.

*******************************************************************************/

#include "ngbe.h"
#include "ngbe_vf.h"

/**
 *  ngbe_init_ops_vf - Initialize the pointers for vf
 *  @hw: pointer to hardware structure
 *
 *  This will assign function pointers, adapter-specific functions can
 *  override the assignment of generic function pointers by assigning
 *  their own adapter-specific function pointers.
 *  Does not touch the hardware.
 **/
s32 ngbe_init_ops_vf(struct ngbevf_hw *hw)
{
	/* MAC */
	hw->mac.ops.init_hw = ngbe_init_hw_vf;
	hw->mac.ops.reset_hw = ngbe_reset_hw_vf;
	hw->mac.ops.start_hw = ngbe_start_hw_vf;
	/* Cannot clear stats on VF */
	hw->mac.ops.clear_hw_cntrs = NULL;
	hw->mac.ops.get_media_type = NULL;
	hw->mac.ops.get_mac_addr = ngbe_get_mac_addr_vf;
	hw->mac.ops.stop_adapter = ngbe_stop_adapter_vf;
	hw->mac.ops.get_bus_info = NULL;

	/* Link */
	hw->mac.ops.setup_link = ngbe_setup_mac_link_vf;
	hw->mac.ops.check_link = ngbe_check_mac_link_vf;
	hw->mac.ops.get_link_capabilities = NULL;

	/* RAR, Multicast, VLAN */
	hw->mac.ops.set_rar = ngbe_set_rar_vf;
	hw->mac.ops.set_uc_addr = ngbe_set_uc_addr_vf;
	hw->mac.ops.init_rx_addrs = NULL;
	hw->mac.ops.update_mc_addr_list = ngbe_update_mc_addr_list_vf;
	hw->mac.ops.update_xcast_mode = ngbe_update_xcast_mode;
	hw->mac.ops.enable_mc = NULL;
	hw->mac.ops.disable_mc = NULL;
	hw->mac.ops.clear_vfta = NULL;
	hw->mac.ops.set_vfta = ngbe_set_vfta_vf;

	hw->mac.max_tx_queues = 1;
	hw->mac.max_rx_queues = 1;

	hw->mbx.ops.init_params = ngbe_init_mbx_params_vf;

	return 0;
}

/* ngbe_virt_clr_reg - Set register to default (power on) state.
 *  @hw: pointer to hardware structure
 */
static void ngbe_virt_clr_reg(struct ngbevf_hw *hw)
{
	u32 vfsrrctl;

	/* VRSRRCTL default values (BSIZEPACKET = 2048, BSIZEHEADER = 256) */
	vfsrrctl = NGBE_VXRXDCTL_HDRSZ(ngbe_hdr_sz(NGBE_RX_HDR_SIZE));
	vfsrrctl |= NGBE_VXRXDCTL_BUFSZ(ngbe_buf_sz(NGBE_RX_BUF_SIZE));

	wr32m(hw, NGBE_VXRXDCTL,
		(NGBE_VXRXDCTL_HDRSZ(~0) | NGBE_VXRXDCTL_BUFSZ(~0)),
		vfsrrctl);

	ngbe_flush(hw);
}

/**
 *  ngbe_start_hw_vf - Prepare hardware for Tx/Rx
 *  @hw: pointer to hardware structure
 *
 *  Starts the hardware by filling the bus info structure and media type, clears
 *  all on chip counters, initializes receive address registers, multicast
 *  table, VLAN filter table, calls routine to set up link and flow control
 *  settings, and leaves transmit and receive units disabled and uninitialized
 **/
s32 ngbe_start_hw_vf(struct ngbevf_hw *hw)
{
	/* Clear adapter stopped flag */
	hw->adapter_stopped = false;

	return 0;
}

/**
 *  ngbe_init_hw_vf - virtual function hardware initialization
 *  @hw: pointer to hardware structure
 *
 *  Initialize the hardware by resetting the hardware and then starting
 *  the hardware
 **/
s32 ngbe_init_hw_vf(struct ngbevf_hw *hw)
{
	s32 status = hw->mac.ops.start_hw(hw);

	TCALL(hw, mac.ops.get_mac_addr, hw->mac.addr);

	return status;
}

/**
 *  ngbe_reset_hw_vf - Performs hardware reset
 *  @hw: pointer to hardware structure
 *
 *  Resets the hardware by reseting the transmit and receive units, masks and
 *  clears all interrupts.
 **/
s32 ngbe_reset_hw_vf(struct ngbevf_hw *hw)
{
	struct ngbe_mbx_info *mbx = &hw->mbx;
	u32 timeout = NGBE_VF_INIT_TIMEOUT;
	s32 err;
	u32 msgbuf[NGBE_VF_PERMADDR_MSG_LEN];
	u8 *addr = (u8 *)(&msgbuf[1]);
	u32 i;

	/* Call adapter stop to disable tx/rx and clear interrupts */
	TCALL(hw, mac.ops.stop_adapter);

	/* reset the api version */
	hw->api_version = ngbe_mbox_api_10;

	hw_dbg(hw, "Issuing a function reset to MAC\n");

	/* backup msix vectors */
	for (i = 0; i < 16; i++)
		hw->b4_buf[i] = ngbe_rd32(hw->b4_addr, i * 4);

	wr32m(hw, NGBE_VXCTRL, NGBE_VXCTRL_RST, NGBE_VXCTRL_RST);
	ngbe_flush(hw);

	msleep(50);

	/* we cannot reset while the RSTI / RSTD bits are asserted */
	while (!mbx->ops.check_for_rst(hw, 0) && timeout) {
		timeout--;
		udelay(5);
	}

	/* restore msix vectors */
	for (i = 0; i < 16; i++)
		ngbe_wr32(hw->b4_addr, i * 4, hw->b4_buf[i]);

	if (!timeout)
		return NGBE_ERR_RESET_FAILED;

	/* Reset VF registers to initial values */
	ngbe_virt_clr_reg(hw);

	/* mailbox timeout can now become active */
	mbx->timeout = NGBE_VF_MBX_INIT_TIMEOUT;

	msgbuf[0] = NGBE_VF_RESET;
	err = mbx->ops.write_posted(hw, msgbuf, 1, 0);
	if (err)
		return err;

	msleep(10);

	/*
	 * set our "perm_addr" based on info provided by PF
	 * also set up the mc_filter_type which is piggy backed
	 * on the mac address in word 3
	 */
	err = mbx->ops.read_posted(hw, msgbuf,
			NGBE_VF_PERMADDR_MSG_LEN, 0);
	if (err)
		return err;

	if (msgbuf[0] != (NGBE_VF_RESET | NGBE_VT_MSGTYPE_ACK) &&
	    msgbuf[0] != (NGBE_VF_RESET | NGBE_VT_MSGTYPE_NACK))
		return NGBE_ERR_INVALID_MAC_ADDR;

	if (msgbuf[0] == (NGBE_VF_RESET | NGBE_VT_MSGTYPE_ACK))
		memcpy(hw->mac.perm_addr, addr, 6);

	hw->mac.mc_filter_type = msgbuf[NGBE_VF_MC_TYPE_WORD];

	return 0;
}

/**
 *  ngbe_stop_adapter_vf - Generic stop Tx/Rx units
 *  @hw: pointer to hardware structure
 *
 *  Sets the adapter_stopped flag within ngbe_hw struct. Clears interrupts,
 *  disables transmit and receive units. The adapter_stopped flag is used by
 *  the shared code and drivers to determine if the adapter is in a stopped
 *  state and should not touch the hardware.
 **/
s32 ngbe_stop_adapter_vf(struct ngbevf_hw *hw)
{
	u32 reg_val;

	/*
	 * Set the adapter_stopped flag so other driver functions stop touching
	 * the hardware
	 */
	hw->adapter_stopped = true;

	/* Clear interrupt mask to stop from interrupts being generated */
	wr32(hw, NGBE_VXIMS, NGBE_VF_IRQ_CLEAR_MASK);

	/* Clear any pending interrupts, flush previous writes */
	wr32(hw, NGBE_VXICR, ~0);

	/* Disable the transmit unit.  Each queue must be disabled. */
	wr32(hw, NGBE_VXTXDCTL, NGBE_VXTXDCTL_FLUSH);

	/* Disable the receive unit by stopping each queue */
	reg_val = rd32(hw, NGBE_VXRXDCTL);
	reg_val &= ~NGBE_VXRXDCTL_ENABLE;
	wr32(hw, NGBE_VXRXDCTL, reg_val);

	/* Clear packet split and pool config */
	wr32(hw, NGBE_VXMRQC, 0);

	/* flush all queues disables */
	ngbe_flush(hw);
	msleep(2);

	return 0;
}

/**
 *  ngbe_mta_vector - Determines bit-vector in multicast table to set
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
static s32 ngbe_mta_vector(struct ngbevf_hw *hw, u8 *mc_addr)
{
	u32 vector = 0;

	switch (hw->mac.mc_filter_type) {
	case 0:   /* use bits [47:36] of the address */
		vector = ((mc_addr[4] >> 4) | (((u16)mc_addr[5]) << 4));
		break;
	case 1:   /* use bits [46:35] of the address */
		vector = ((mc_addr[4] >> 3) | (((u16)mc_addr[5]) << 5));
		break;
	case 2:   /* use bits [45:34] of the address */
		vector = ((mc_addr[4] >> 2) | (((u16)mc_addr[5]) << 6));
		break;
	case 3:   /* use bits [43:32] of the address */
		vector = ((mc_addr[4]) | (((u16)mc_addr[5]) << 8));
		break;
	default:  /* Invalid mc_filter_type */
		hw_dbg(hw, "MC filter type param set incorrectly\n");
		break;
	}

	/* vector can only be 12-bits or boundary will be exceeded */
	vector &= 0xFFF;
	return vector;
}

static s32 ngbe_write_msg_read_ack(struct ngbevf_hw *hw, u32 *msg,
				      u32 *retmsg, u16 size)
{
	struct ngbe_mbx_info *mbx = &hw->mbx;
	s32 retval = mbx->ops.write_posted(hw, msg, size, 0);

	if (retval)
		return retval;

	return mbx->ops.read_posted(hw, retmsg, size, 0);
}

/**
 *  ngbe_set_rar_vf - set device MAC address
 *  @hw: pointer to hardware structure
 *  @index: Receive address register to write
 *  @addr: Address to put into receive address register
 *  @vmdq: VMDq "set" or "pool" index
 *  @enable_addr: set flag that address is active
 **/
s32 ngbe_set_rar_vf(struct ngbevf_hw *hw, u32 index, u8 *addr, u32 vmdq,
		     u32 enable_addr)
{
	struct ngbe_mbx_info *mbx = &hw->mbx;
	u32 msgbuf[3];
	u8 *msg_addr = (u8 *)(&msgbuf[1]);
	s32 err;
	UNREFERENCED_3PARAMETER(vmdq, enable_addr, index);

	memset(msgbuf, 0, 12);
	msgbuf[0] = NGBE_VF_SET_MAC_ADDR;
	memcpy(msg_addr, addr, 6);
	err = mbx->ops.write_posted(hw, msgbuf, 3, 0);

	if (!err)
		err = mbx->ops.read_posted(hw, msgbuf, 3, 0);

	msgbuf[0] &= ~NGBE_VT_MSGTYPE_CTS;

	/* if nacked the address was rejected, use "perm_addr" */
	if (!err &&
	    (msgbuf[0] == (NGBE_VF_SET_MAC_ADDR | NGBE_VT_MSGTYPE_NACK))) {
		ngbe_get_mac_addr_vf(hw, hw->mac.addr);
		return NGBE_ERR_MBX;
	}

	return err;
}

/**
 *  ngbe_update_mc_addr_list_vf - Update Multicast addresses
 *  @hw: pointer to the HW structure
 *  @mc_addr_list: array of multicast addresses to program
 *  @mc_addr_count: number of multicast addresses to program
 *  @next: caller supplied function to return next address in list
 *
 *  Updates the Multicast Table Array.
 **/
s32 ngbe_update_mc_addr_list_vf(struct ngbevf_hw *hw, u8 *mc_addr_list,
				 u32 mc_addr_count, ngbe_mc_addr_itr next,
				 bool clear)
{
	struct ngbe_mbx_info *mbx = &hw->mbx;
	u32 msgbuf[NGBE_VXMAILBOX_SIZE];
	u16 *vector_list = (u16 *)&msgbuf[1];
	u32 vector;
	u32 cnt, i;
	u32 vmdq;

	UNREFERENCED_1PARAMETER(clear);

	/* Each entry in the list uses 1 16 bit word.  We have 30
	 * 16 bit words available in our HW msg buffer (minus 1 for the
	 * msg type).  That's 30 hash values if we pack 'em right.  If
	 * there are more than 30 MC addresses to add then punt the
	 * extras for now and then add code to handle more than 30 later.
	 * It would be unusual for a server to request that many multi-cast
	 * addresses except for in large enterprise network environments.
	 */
	cnt = (mc_addr_count > 30) ? 30 : mc_addr_count;
	msgbuf[0] = NGBE_VF_SET_MULTICAST;
	msgbuf[0] |= cnt << NGBE_VT_MSGINFO_SHIFT;

	for (i = 0; i < cnt; i++) {
		vector = ngbe_mta_vector(hw, next(hw, &mc_addr_list, &vmdq));
		vector_list[i] = (u16)vector;
	}

	return mbx->ops.write_posted(hw, msgbuf, NGBE_VXMAILBOX_SIZE, 0);
}

/**
 *  ngbe_update_xcast_mode - Update Multicast mode
 *  @hw: pointer to the HW structure
 *  @xcast_mode: new multicast mode
 *
 *  Updates the Multicast Mode of VF.
 **/
s32 ngbe_update_xcast_mode(struct ngbevf_hw *hw, int xcast_mode)
{
	struct ngbe_mbx_info *mbx = &hw->mbx;
	u32 msgbuf[2];
	s32 err;

	switch (hw->api_version) {
	case ngbe_mbox_api_12:
		break;
	default:
		return NGBE_ERR_FEATURE_NOT_SUPPORTED;
	}

	msgbuf[0] = NGBE_VF_UPDATE_XCAST_MODE;
	msgbuf[1] = xcast_mode;

	err = mbx->ops.write_posted(hw, msgbuf, 2, 0);
	if (err)
		return err;

	err = mbx->ops.read_posted(hw, msgbuf, 2, 0);
	if (err)
		return err;

	msgbuf[0] &= ~NGBE_VT_MSGTYPE_CTS;
	if (msgbuf[0] == (NGBE_VF_UPDATE_XCAST_MODE | NGBE_VT_MSGTYPE_NACK))
		return NGBE_ERR_FEATURE_NOT_SUPPORTED;
	return 0;
}

/**
 *  ngbe_set_vfta_vf - Set/Unset vlan filter table address
 *  @hw: pointer to the HW structure
 *  @vlan: 12 bit VLAN ID
 *  @vind: unused by VF drivers
 *  @vlan_on: if true then set bit, else clear bit
 *  @vlvf_bypass: boolean flag indicating updating default pool is okay
 *
 *  Turn on/off specified VLAN in the VLAN filter table.
 **/
s32 ngbe_set_vfta_vf(struct ngbevf_hw *hw, u32 vlan, u32 vind,
		      bool vlan_on, bool vlvf_bypass)
{
	struct ngbe_mbx_info *mbx = &hw->mbx;
	u32 msgbuf[2];
	s32 err;
	UNREFERENCED_2PARAMETER(vind, vlvf_bypass);
	UNREFERENCED_2PARAMETER(vlan_on, vlan);

	msgbuf[0] = NGBE_VF_SET_VLAN;
	msgbuf[1] = vlan;
	/* Setting the 8 bit field MSG INFO to TRUE indicates "add" */
	msgbuf[0] |= vlan_on << NGBE_VT_MSGINFO_SHIFT;

	err = mbx->ops.write_posted(hw, msgbuf, 2, 0);
	if (!err)
		err = mbx->ops.read_posted(hw, msgbuf, 1, 0);

	if (!err && (msgbuf[0] & NGBE_VT_MSGTYPE_ACK))
		return 0;

	return err | (msgbuf[0] & NGBE_VT_MSGTYPE_NACK);
}

/**
 *  ngbe_get_num_of_tx_queues_vf - Get number of TX queues
 *  @hw: pointer to hardware structure
 *
 *  Returns the number of transmit queues for the given adapter.
 **/
u32 ngbe_get_num_of_tx_queues_vf(struct ngbevf_hw *hw)
{
	UNREFERENCED_1PARAMETER(hw);
	return NGBE_VF_MAX_TX_QUEUES;
}

/**
 *  ngbe_get_num_of_rx_queues_vf - Get number of RX queues
 *  @hw: pointer to hardware structure
 *
 *  Returns the number of receive queues for the given adapter.
 **/
u32 ngbe_get_num_of_rx_queues_vf(struct ngbevf_hw *hw)
{
	UNREFERENCED_1PARAMETER(hw);
	return NGBE_VF_MAX_RX_QUEUES;
}

/**
 *  ngbe_get_mac_addr_vf - Read device MAC address
 *  @hw: pointer to the HW structure
 **/
s32 ngbe_get_mac_addr_vf(struct ngbevf_hw *hw, u8 *mac_addr)
{
	int i;

	for (i = 0; i < 6; i++)
		mac_addr[i] = hw->mac.perm_addr[i];

	return 0;
}

s32 ngbe_set_uc_addr_vf(struct ngbevf_hw *hw, u32 index, u8 *addr)
{
	struct ngbe_mbx_info *mbx = &hw->mbx;
	u32 msgbuf[3];
	u8 *msg_addr = (u8 *)(&msgbuf[1]);
	s32 err;

	memset(msgbuf, 0, sizeof(msgbuf));
	/*
	 * If index is one then this is the start of a new list and needs
	 * indication to the PF so it can do it's own list management.
	 * If it is zero then that tells the PF to just clear all of
	 * this VF's macvlans and there is no new list.
	 */
	msgbuf[0] |= index << NGBE_VT_MSGINFO_SHIFT;
	msgbuf[0] |= NGBE_VF_SET_MACVLAN;
	if (addr)
		memcpy(msg_addr, addr, 6);
	err = mbx->ops.write_posted(hw, msgbuf, 3, 0);

	if (!err)
		err = mbx->ops.read_posted(hw, msgbuf, 3, 0);

	msgbuf[0] &= ~NGBE_VT_MSGTYPE_CTS;

	if (!err)
		if (msgbuf[0] == (NGBE_VF_SET_MACVLAN | NGBE_VT_MSGTYPE_NACK))
			err = NGBE_ERR_OUT_OF_MEM;

	return err;
}

/**
 *  ngbe_setup_mac_link_vf - Setup MAC link settings
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg: true if autonegotiation enabled
 *  @autoneg_wait_to_complete: true when waiting for completion is needed
 *
 *  Set the link speed in the AUTOC register and restarts link.
 **/
s32 ngbe_setup_mac_link_vf(struct ngbevf_hw *hw, ngbe_link_speed speed,
			    bool autoneg_wait_to_complete)
{
	UNREFERENCED_3PARAMETER(hw, speed, autoneg_wait_to_complete);
	return 0;
}

/**
 *  ngbe_check_mac_link_vf - Get link/speed status
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @link_up: true is link is up, false otherwise
 *  @autoneg_wait_to_complete: true when waiting for completion is needed
 *
 *  Reads the links register to determine if link is up and the current speed
 **/
s32 ngbe_check_mac_link_vf(struct ngbevf_hw *hw, ngbe_link_speed *speed,
			    bool *link_up, bool autoneg_wait_to_complete)
{
	struct ngbe_mbx_info *mbx = &hw->mbx;
	struct ngbe_mac_info *mac = &hw->mac;
	s32 err = 0;
	u32 links_reg;
	u32 in_msg = 0;
	u8 i = 0;

	UNREFERENCED_1PARAMETER(autoneg_wait_to_complete);

	/* If we were hit with a reset drop the link */
	if (!mbx->ops.check_for_rst(hw, 0) || !mbx->timeout)
		mac->get_link_status = true;

	if (!mac->get_link_status)
		goto out;

	/* if link status is down no point in checking to see if pf is up */
	links_reg = rd32(hw, NGBE_VXSTATUS);
	if (NGBE_VXSTATUS_SPEED(links_reg) == 0)
		goto out;
	
	for (i = 0; i < 5; i++) {
		udelay(100);
		links_reg = rd32(hw, NGBE_VXSTATUS);
	
		if (NGBE_VXSTATUS_SPEED(links_reg) == 0)
			goto out;
	}

	if (NGBE_VXSTATUS_SPEED(links_reg) & NGBE_VXSTATUS_SPEED_1G)
		*speed = NGBE_LINK_SPEED_1GB_FULL;
	else if (NGBE_VXSTATUS_SPEED(links_reg) & NGBE_VXSTATUS_SPEED_100M)
		*speed = NGBE_LINK_SPEED_100_FULL;
	else if (NGBE_VXSTATUS_SPEED(links_reg) & NGBE_VXSTATUS_SPEED_10M)
		*speed = NGBE_LINK_SPEED_10_FULL;

	/* if the read failed it could just be a mailbox collision, best wait
	 * until we are called again and don't report an error
	 */
	if (mbx->ops.read(hw, &in_msg, 1, 0))
		goto out;

	if (!(in_msg & NGBE_VT_MSGTYPE_CTS)) {
		/* msg is not CTS and is NACK we must have lost CTS status */
		if (in_msg & NGBE_VT_MSGTYPE_NACK)
			err = -1;
		goto out;
	}

	/* the pf is talking, if we timed out in the past we reinit */
	if (!mbx->timeout) {
		err = -1;
		goto out;
	}

	/* if we passed all the tests above then the link is up and we no
	 * longer need to check for link
	 */
	mac->get_link_status = false;
out:
	*link_up = !mac->get_link_status;
	return err;
}

/**
 *  ngbe_rlpml_set_vf - Set the maximum receive packet length
 *  @hw: pointer to the HW structure
 *  @max_size: value to assign to max frame size
 **/
s32 ngbe_rlpml_set_vf(struct ngbevf_hw *hw, u16 max_size)
{
	u32 msgbuf[2];
	s32 retval;

	msgbuf[0] = NGBE_VF_SET_LPE;
	msgbuf[1] = max_size;

	retval = ngbe_write_msg_read_ack(hw, msgbuf, msgbuf, 2);
	if (retval)
		return retval;
	if ((msgbuf[0] & NGBE_VF_SET_LPE) &&
	    (msgbuf[0] & NGBE_VT_MSGTYPE_NACK))
		return NGBE_ERR_MBX;

	return 0;
}


/**
 *  ngbe_negotiate_api_version - Negotiate supported API version
 *  @hw: pointer to the HW structure
 *  @api: integer containing requested API version
 **/
int ngbe_negotiate_api_version(struct ngbevf_hw *hw, int api)
{
	int err;
	u32 msg[3];

	/* Negotiate the mailbox API version */
	msg[0] = NGBE_VF_API_NEGOTIATE;
	msg[1] = api;
	msg[2] = 0;
	err = TCALL(hw, mbx.ops.write_posted, msg, 3, 0);

	if (!err)
		err = TCALL(hw, mbx.ops.read_posted, msg, 3, 0);

	if (!err) {
		msg[0] &= ~NGBE_VT_MSGTYPE_CTS;

		/* Store value and return 0 on success */
		if (msg[0] == (NGBE_VF_API_NEGOTIATE | NGBE_VT_MSGTYPE_ACK)) {
			hw->api_version = api;
			return 0;
		}

		err = NGBE_ERR_INVALID_ARGUMENT;
	}

	return err;
}

int ngbe_get_queues(struct ngbevf_hw *hw, unsigned int *num_tcs,
		       unsigned int *default_tc)
{
	int err;
	u32 msg[5];

	/* do nothing if API doesn't support ngbe_get_queues */
	switch (hw->api_version) {
	case ngbe_mbox_api_11:
	case ngbe_mbox_api_12:
	case ngbe_mbox_api_13:
		break;
	default:
		return 0;
	}

	/* Fetch queue configuration from the PF */
	msg[0] = NGBE_VF_GET_QUEUES;
	msg[1] = msg[2] = msg[3] = msg[4] = 0;
	err = TCALL(hw, mbx.ops.write_posted, msg, 5, 0);

	if (!err)
		err = TCALL(hw, mbx.ops.read_posted, msg, 5, 0);

	if (!err) {
		msg[0] &= ~NGBE_VT_MSGTYPE_CTS;

		/*
		 * if we we didn't get an ACK there must have been
		 * some sort of mailbox error so we should treat it
		 * as such
		 */
		if (msg[0] != (NGBE_VF_GET_QUEUES | NGBE_VT_MSGTYPE_ACK))
			return NGBE_ERR_MBX;

		/* record and validate values from message */
		hw->mac.max_tx_queues = msg[NGBE_VF_TX_QUEUES];
		if (hw->mac.max_tx_queues == 0 ||
		    hw->mac.max_tx_queues > NGBE_VF_MAX_TX_QUEUES)
			hw->mac.max_tx_queues = NGBE_VF_MAX_TX_QUEUES;

		hw->mac.max_rx_queues = msg[NGBE_VF_RX_QUEUES];
		if (hw->mac.max_rx_queues == 0 ||
		    hw->mac.max_rx_queues > NGBE_VF_MAX_RX_QUEUES)
			hw->mac.max_rx_queues = NGBE_VF_MAX_RX_QUEUES;

		*num_tcs = msg[NGBE_VF_TRANS_VLAN];
		/* in case of unknown state assume we cannot tag frames */
		if (*num_tcs > hw->mac.max_rx_queues)
			*num_tcs = 1;

		*default_tc = msg[NGBE_VF_DEF_QUEUE];
		/* default to queue 0 on out-of-bounds queue number */
		if (*default_tc >= hw->mac.max_tx_queues)
			*default_tc = 0;
	}

	return err;
}
