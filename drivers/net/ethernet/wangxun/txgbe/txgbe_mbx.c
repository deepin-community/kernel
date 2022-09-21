/*
 * WangXun 10 Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * based on ixgbe_mbx.c, Copyright(c) 1999 - 2017 Intel Corporation.
 * Contact Information:
 * Linux NICS <linux.nics@intel.com>
 * e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 */

#include "txgbe_type.h"
#include "txgbe.h"
#include "txgbe_mbx.h"


/**
 *  txgbe_read_mbx - Reads a message from the mailbox
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @mbx_id: id of mailbox to read
 *
 *  returns SUCCESS if it successfuly read message from buffer
 **/
int txgbe_read_mbx(struct txgbe_hw *hw, u32 *msg, u16 size, u16 mbx_id)
{
	struct txgbe_mbx_info *mbx = &hw->mbx;
	int err = TXGBE_ERR_MBX;

	/* limit read to size of mailbox */
	if (size > mbx->size)
		size = mbx->size;

	err = TCALL(hw, mbx.ops.read, msg, size, mbx_id);

	return err;
}

/**
 *  txgbe_write_mbx - Write a message to the mailbox
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @mbx_id: id of mailbox to write
 *
 *  returns SUCCESS if it successfully copied message into the buffer
 **/
int txgbe_write_mbx(struct txgbe_hw *hw, u32 *msg, u16 size, u16 mbx_id)
{
	struct txgbe_mbx_info *mbx = &hw->mbx;
	int err = 0;

	if (size > mbx->size) {
		err = TXGBE_ERR_MBX;
		ERROR_REPORT2(TXGBE_ERROR_ARGUMENT,
			     "Invalid mailbox message size %d", size);
	} else
		err = TCALL(hw, mbx.ops.write, msg, size, mbx_id);

	return err;
}

/**
 *  txgbe_check_for_msg - checks to see if someone sent us mail
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to check
 *
 *  returns SUCCESS if the Status bit was found or else ERR_MBX
 **/
int txgbe_check_for_msg(struct txgbe_hw *hw, u16 mbx_id)
{
	int err = TXGBE_ERR_MBX;

	err = TCALL(hw, mbx.ops.check_for_msg, mbx_id);

	return err;
}

/**
 *  txgbe_check_for_ack - checks to see if someone sent us ACK
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to check
 *
 *  returns SUCCESS if the Status bit was found or else ERR_MBX
 **/
int txgbe_check_for_ack(struct txgbe_hw *hw, u16 mbx_id)
{
	int err = TXGBE_ERR_MBX;

	err = TCALL(hw, mbx.ops.check_for_ack, mbx_id);

	return err;
}

/**
 *  txgbe_check_for_rst - checks to see if other side has reset
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to check
 *
 *  returns SUCCESS if the Status bit was found or else ERR_MBX
 **/
int txgbe_check_for_rst(struct txgbe_hw *hw, u16 mbx_id)
{
	struct txgbe_mbx_info *mbx = &hw->mbx;
	int err = TXGBE_ERR_MBX;

	if (mbx->ops.check_for_rst)
		err = mbx->ops.check_for_rst(hw, mbx_id);

	return err;
}

/**
 *  txgbe_poll_for_msg - Wait for message notification
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to write
 *
 *  returns SUCCESS if it successfully received a message notification
 **/
int txgbe_poll_for_msg(struct txgbe_hw *hw, u16 mbx_id)
{
	struct txgbe_mbx_info *mbx = &hw->mbx;
	int countdown = mbx->timeout;

	if (!countdown || !mbx->ops.check_for_msg)
		goto out;

	while (countdown && TCALL(hw, mbx.ops.check_for_msg, mbx_id)) {
		countdown--;
		if (!countdown)
			break;
		udelay(mbx->udelay);
	}

	if (countdown == 0)
		ERROR_REPORT2(TXGBE_ERROR_POLLING,
			   "Polling for VF%d mailbox message timedout", mbx_id);

out:
	return countdown ? 0 : TXGBE_ERR_MBX;
}

/**
 *  txgbe_poll_for_ack - Wait for message acknowledgement
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to write
 *
 *  returns SUCCESS if it successfully received a message acknowledgement
 **/
int txgbe_poll_for_ack(struct txgbe_hw *hw, u16 mbx_id)
{
	struct txgbe_mbx_info *mbx = &hw->mbx;
	int countdown = mbx->timeout;

	if (!countdown || !mbx->ops.check_for_ack)
		goto out;

	while (countdown && TCALL(hw, mbx.ops.check_for_ack, mbx_id)) {
		countdown--;
		if (!countdown)
			break;
		udelay(mbx->udelay);
	}

	if (countdown == 0)
		ERROR_REPORT2(TXGBE_ERROR_POLLING,
			     "Polling for VF%d mailbox ack timedout", mbx_id);

out:
	return countdown ? 0 : TXGBE_ERR_MBX;
}

/**
 *  txgbe_read_posted_mbx - Wait for message notification and receive message
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @mbx_id: id of mailbox to write
 *
 *  returns SUCCESS if it successfully received a message notification and
 *  copied it into the receive buffer.
 **/
int txgbe_read_posted_mbx(struct txgbe_hw *hw, u32 *msg, u16 size, u16 mbx_id)
{
	struct txgbe_mbx_info *mbx = &hw->mbx;
	int err = TXGBE_ERR_MBX;

	if (!mbx->ops.read)
		goto out;

	err = txgbe_poll_for_msg(hw, mbx_id);

	/* if ack received read message, otherwise we timed out */
	if (!err)
		err = TCALL(hw, mbx.ops.read, msg, size, mbx_id);
out:
	return err;
}

/**
 *  txgbe_write_posted_mbx - Write a message to the mailbox, wait for ack
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @mbx_id: id of mailbox to write
 *
 *  returns SUCCESS if it successfully copied message into the buffer and
 *  received an ack to that message within delay * timeout period
 **/
int txgbe_write_posted_mbx(struct txgbe_hw *hw, u32 *msg, u16 size,
			   u16 mbx_id)
{
	struct txgbe_mbx_info *mbx = &hw->mbx;
	int err;

	/* exit if either we can't write or there isn't a defined timeout */
	if (!mbx->timeout)
		return TXGBE_ERR_MBX;

	/* send msg */
	err = TCALL(hw, mbx.ops.write, msg, size, mbx_id);

	/* if msg sent wait until we receive an ack */
	if (!err)
		err = txgbe_poll_for_ack(hw, mbx_id);

	return err;
}

/**
 *  txgbe_init_mbx_ops - Initialize MB function pointers
 *  @hw: pointer to the HW structure
 *
 *  Setups up the mailbox read and write message function pointers
 **/
void txgbe_init_mbx_ops(struct txgbe_hw *hw)
{
	struct txgbe_mbx_info *mbx = &hw->mbx;

	mbx->ops.read_posted = txgbe_read_posted_mbx;
	mbx->ops.write_posted = txgbe_write_posted_mbx;
}

/**
 *  txgbe_read_v2p_mailbox - read v2p mailbox
 *  @hw: pointer to the HW structure
 *
 *  This function is used to read the v2p mailbox without losing the read to
 *  clear status bits.
 **/
u32 txgbe_read_v2p_mailbox(struct txgbe_hw *hw)
{
	u32 v2p_mailbox = rd32(hw, TXGBE_VXMAILBOX);

	v2p_mailbox |= hw->mbx.v2p_mailbox;
	/* read and clear mirrored mailbox flags */
	v2p_mailbox |= rd32a(hw, TXGBE_VXMBMEM, TXGBE_VXMAILBOX_SIZE);
	wr32a(hw, TXGBE_VXMBMEM, TXGBE_VXMAILBOX_SIZE, 0);
	hw->mbx.v2p_mailbox |= v2p_mailbox & TXGBE_VXMAILBOX_R2C_BITS;

	return v2p_mailbox;
}

/**
 *  txgbe_check_for_bit_vf - Determine if a status bit was set
 *  @hw: pointer to the HW structure
 *  @mask: bitmask for bits to be tested and cleared
 *
 *  This function is used to check for the read to clear bits within
 *  the V2P mailbox.
 **/
int txgbe_check_for_bit_vf(struct txgbe_hw *hw, u32 mask)
{
	u32 mailbox = txgbe_read_v2p_mailbox(hw);

	hw->mbx.v2p_mailbox &= ~mask;

	return (mailbox & mask ? 0 : TXGBE_ERR_MBX);
}

/**
 *  txgbe_check_for_msg_vf - checks to see if the PF has sent mail
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to check
 *
 *  returns SUCCESS if the PF has set the Status bit or else ERR_MBX
 **/
int txgbe_check_for_msg_vf(struct txgbe_hw *hw, u16 mbx_id)
{
	int err = TXGBE_ERR_MBX;

	UNREFERENCED_PARAMETER(mbx_id);

	/* read clear the pf sts bit */
	if (!txgbe_check_for_bit_vf(hw, TXGBE_VXMAILBOX_PFSTS)) {
		err = 0;
		hw->mbx.stats.reqs++;
	}

	return err;
}

/**
 *  txgbe_check_for_ack_vf - checks to see if the PF has ACK'd
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to check
 *
 *  returns SUCCESS if the PF has set the ACK bit or else ERR_MBX
 **/
int txgbe_check_for_ack_vf(struct txgbe_hw *hw, u16 mbx_id)
{
	int err = TXGBE_ERR_MBX;

	UNREFERENCED_PARAMETER(mbx_id);

	/* read clear the pf ack bit */
	if (!txgbe_check_for_bit_vf(hw, TXGBE_VXMAILBOX_PFACK)) {
		err = 0;
		hw->mbx.stats.acks++;
	}

	return err;
}

/**
 *  txgbe_check_for_rst_vf - checks to see if the PF has reset
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to check
 *
 *  returns true if the PF has set the reset done bit or else false
 **/
int txgbe_check_for_rst_vf(struct txgbe_hw *hw, u16 mbx_id)
{
	int err = TXGBE_ERR_MBX;

	UNREFERENCED_PARAMETER(mbx_id);
	if (!txgbe_check_for_bit_vf(hw, (TXGBE_VXMAILBOX_RSTD |
	    TXGBE_VXMAILBOX_RSTI))) {
		err = 0;
		hw->mbx.stats.rsts++;
	}

	return err;
}

/**
 *  txgbe_obtain_mbx_lock_vf - obtain mailbox lock
 *  @hw: pointer to the HW structure
 *
 *  return SUCCESS if we obtained the mailbox lock
 **/
int txgbe_obtain_mbx_lock_vf(struct txgbe_hw *hw)
{
	int err = TXGBE_ERR_MBX;
	u32 mailbox;

	/* Take ownership of the buffer */
	wr32(hw, TXGBE_VXMAILBOX, TXGBE_VXMAILBOX_VFU);

	/* reserve mailbox for vf use */
	mailbox = txgbe_read_v2p_mailbox(hw);
	if (mailbox & TXGBE_VXMAILBOX_VFU)
		err = 0;
	else
		ERROR_REPORT2(TXGBE_ERROR_POLLING,
			   "Failed to obtain mailbox lock for VF");

	return err;
}

/**
 *  txgbe_write_mbx_vf - Write a message to the mailbox
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @mbx_id: id of mailbox to write
 *
 *  returns SUCCESS if it successfully copied message into the buffer
 **/
int txgbe_write_mbx_vf(struct txgbe_hw *hw, u32 *msg, u16 size,
			      u16 mbx_id)
{
	int err;
	u16 i;

	UNREFERENCED_PARAMETER(mbx_id);

	/* lock the mailbox to prevent pf/vf race condition */
	err = txgbe_obtain_mbx_lock_vf(hw);
	if (err)
		goto out_no_write;

	/* flush msg and acks as we are overwriting the message buffer */
	txgbe_check_for_msg_vf(hw, 0);
	txgbe_check_for_ack_vf(hw, 0);

	/* copy the caller specified message to the mailbox memory buffer */
	for (i = 0; i < size; i++)
		wr32a(hw, TXGBE_VXMBMEM, i, msg[i]);

	/* update stats */
	hw->mbx.stats.msgs_tx++;

	/* Drop VFU and interrupt the PF to tell it a message has been sent */
	wr32(hw, TXGBE_VXMAILBOX, TXGBE_VXMAILBOX_REQ);

out_no_write:
	return err;
}

/**
 *  txgbe_read_mbx_vf - Reads a message from the inbox intended for vf
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @mbx_id: id of mailbox to read
 *
 *  returns SUCCESS if it successfuly read message from buffer
 **/
int txgbe_read_mbx_vf(struct txgbe_hw *hw, u32 *msg, u16 size,
			     u16 mbx_id)
{
	int err = 0;
	u16 i;
	UNREFERENCED_PARAMETER(mbx_id);

	/* lock the mailbox to prevent pf/vf race condition */
	err = txgbe_obtain_mbx_lock_vf(hw);
	if (err)
		goto out_no_read;

	/* copy the message from the mailbox memory buffer */
	for (i = 0; i < size; i++)
		msg[i] = rd32a(hw, TXGBE_VXMBMEM, i);

	/* Acknowledge receipt and release mailbox, then we're done */
	wr32(hw, TXGBE_VXMAILBOX, TXGBE_VXMAILBOX_ACK);

	/* update stats */
	hw->mbx.stats.msgs_rx++;

out_no_read:
	return err;
}

/**
 *  txgbe_init_mbx_params_vf - set initial values for vf mailbox
 *  @hw: pointer to the HW structure
 *
 *  Initializes the hw->mbx struct to correct values for vf mailbox
 */
void txgbe_init_mbx_params_vf(struct txgbe_hw *hw)
{
	struct txgbe_mbx_info *mbx = &hw->mbx;

	/* start mailbox as timed out and let the reset_hw call set the timeout
	 * value to begin communications */
	mbx->timeout = 0;
	mbx->udelay = TXGBE_VF_MBX_INIT_DELAY;

	mbx->size = TXGBE_VXMAILBOX_SIZE;

	mbx->ops.read = txgbe_read_mbx_vf;
	mbx->ops.write = txgbe_write_mbx_vf;
	mbx->ops.read_posted = txgbe_read_posted_mbx;
	mbx->ops.write_posted = txgbe_write_posted_mbx;
	mbx->ops.check_for_msg = txgbe_check_for_msg_vf;
	mbx->ops.check_for_ack = txgbe_check_for_ack_vf;
	mbx->ops.check_for_rst = txgbe_check_for_rst_vf;

	mbx->stats.msgs_tx = 0;
	mbx->stats.msgs_rx = 0;
	mbx->stats.reqs = 0;
	mbx->stats.acks = 0;
	mbx->stats.rsts = 0;
}

int txgbe_check_for_bit_pf(struct txgbe_hw *hw, u32 mask, int index)
{
	u32 mbvficr = rd32(hw, TXGBE_MBVFICR(index));
	int err = TXGBE_ERR_MBX;

	if (mbvficr & mask) {
		err = 0;
		wr32(hw, TXGBE_MBVFICR(index), mask);
	}

	return err;
}

/**
 *  txgbe_check_for_msg_pf - checks to see if the VF has sent mail
 *  @hw: pointer to the HW structure
 *  @vf: the VF index
 *
 *  returns SUCCESS if the VF has set the Status bit or else ERR_MBX
 **/
int txgbe_check_for_msg_pf(struct txgbe_hw *hw, u16 vf)
{
	int err = TXGBE_ERR_MBX;
	int index = TXGBE_MBVFICR_INDEX(vf);
	u32 vf_bit = vf % 16;

	if (!txgbe_check_for_bit_pf(hw, TXGBE_MBVFICR_VFREQ_VF1 << vf_bit,
				    index)) {
		err = 0;
		hw->mbx.stats.reqs++;
	}

	return err;
}

/**
 *  txgbe_check_for_ack_pf - checks to see if the VF has ACKed
 *  @hw: pointer to the HW structure
 *  @vf: the VF index
 *
 *  returns SUCCESS if the VF has set the Status bit or else ERR_MBX
 **/
int txgbe_check_for_ack_pf(struct txgbe_hw *hw, u16 vf)
{
	int err = TXGBE_ERR_MBX;
	int index = TXGBE_MBVFICR_INDEX(vf);
	u32 vf_bit = vf % 16;

	if (!txgbe_check_for_bit_pf(hw, TXGBE_MBVFICR_VFACK_VF1 << vf_bit,
				    index)) {
		err = 0;
		hw->mbx.stats.acks++;
	}

	return err;
}

/**
 *  txgbe_check_for_rst_pf - checks to see if the VF has reset
 *  @hw: pointer to the HW structure
 *  @vf: the VF index
 *
 *  returns SUCCESS if the VF has set the Status bit or else ERR_MBX
 **/
int txgbe_check_for_rst_pf(struct txgbe_hw *hw, u16 vf)
{
	u32 reg_offset = (vf < 32) ? 0 : 1;
	u32 vf_shift = vf % 32;
	u32 vflre = 0;
	int err = TXGBE_ERR_MBX;

	vflre = rd32(hw, TXGBE_VFLRE(reg_offset));

	if (vflre & (1 << vf_shift)) {
		err = 0;
		wr32(hw, TXGBE_VFLREC(reg_offset), (1 << vf_shift));
		hw->mbx.stats.rsts++;
	}

	return err;
}

/**
 *  txgbe_obtain_mbx_lock_pf - obtain mailbox lock
 *  @hw: pointer to the HW structure
 *  @vf: the VF index
 *
 *  return SUCCESS if we obtained the mailbox lock
 **/
int txgbe_obtain_mbx_lock_pf(struct txgbe_hw *hw, u16 vf)
{
	int err = TXGBE_ERR_MBX;
	u32 mailbox;

	/* Take ownership of the buffer */
	wr32(hw, TXGBE_PXMAILBOX(vf), TXGBE_PXMAILBOX_PFU);

	/* reserve mailbox for vf use */
	mailbox = rd32(hw, TXGBE_PXMAILBOX(vf));
	if (mailbox & TXGBE_PXMAILBOX_PFU)
		err = 0;
	else
		ERROR_REPORT2(TXGBE_ERROR_POLLING,
			   "Failed to obtain mailbox lock for PF%d", vf);


	return err;
}

/**
 *  txgbe_write_mbx_pf - Places a message in the mailbox
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @vf: the VF index
 *
 *  returns SUCCESS if it successfully copied message into the buffer
 **/
int txgbe_write_mbx_pf(struct txgbe_hw *hw, u32 *msg, u16 size,
			      u16 vf)
{
	int err;
	u16 i;

	/* lock the mailbox to prevent pf/vf race condition */
	err = txgbe_obtain_mbx_lock_pf(hw, vf);
	if (err)
		goto out_no_write;

	/* flush msg and acks as we are overwriting the message buffer */
	txgbe_check_for_msg_pf(hw, vf);
	txgbe_check_for_ack_pf(hw, vf);

	/* copy the caller specified message to the mailbox memory buffer */
	for (i = 0; i < size; i++)
		wr32a(hw, TXGBE_PXMBMEM(vf), i, msg[i]);

	/* Interrupt VF to tell it a message has been sent and release buffer*/
	/* set mirrored mailbox flags */
	wr32a(hw, TXGBE_PXMBMEM(vf), TXGBE_VXMAILBOX_SIZE, TXGBE_PXMAILBOX_STS);
	wr32(hw, TXGBE_PXMAILBOX(vf), TXGBE_PXMAILBOX_STS);

	/* update stats */
	hw->mbx.stats.msgs_tx++;

out_no_write:
	return err;

}

/**
 *  txgbe_read_mbx_pf - Read a message from the mailbox
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @vf: the VF index
 *
 *  This function copies a message from the mailbox buffer to the caller's
 *  memory buffer.  The presumption is that the caller knows that there was
 *  a message due to a VF request so no polling for message is needed.
 **/
int txgbe_read_mbx_pf(struct txgbe_hw *hw, u32 *msg, u16 size,
			     u16 vf)
{
	int err;
	u16 i;

	/* lock the mailbox to prevent pf/vf race condition */
	err = txgbe_obtain_mbx_lock_pf(hw, vf);
	if (err)
		goto out_no_read;

	/* copy the message to the mailbox memory buffer */
	for (i = 0; i < size; i++)
		msg[i] = rd32a(hw, TXGBE_PXMBMEM(vf), i);

	/* Acknowledge the message and release buffer */
	/* set mirrored mailbox flags */
	wr32a(hw, TXGBE_PXMBMEM(vf), TXGBE_VXMAILBOX_SIZE, TXGBE_PXMAILBOX_ACK);
	wr32(hw, TXGBE_PXMAILBOX(vf), TXGBE_PXMAILBOX_ACK);

	/* update stats */
	hw->mbx.stats.msgs_rx++;

out_no_read:
	return err;
}

/**
 *  txgbe_init_mbx_params_pf - set initial values for pf mailbox
 *  @hw: pointer to the HW structure
 *
 *  Initializes the hw->mbx struct to correct values for pf mailbox
 */
void txgbe_init_mbx_params_pf(struct txgbe_hw *hw)
{
	struct txgbe_mbx_info *mbx = &hw->mbx;

	mbx->timeout = 0;
	mbx->udelay = 0;

	mbx->size = TXGBE_VXMAILBOX_SIZE;

	mbx->ops.read = txgbe_read_mbx_pf;
	mbx->ops.write = txgbe_write_mbx_pf;
	mbx->ops.read_posted = txgbe_read_posted_mbx;
	mbx->ops.write_posted = txgbe_write_posted_mbx;
	mbx->ops.check_for_msg = txgbe_check_for_msg_pf;
	mbx->ops.check_for_ack = txgbe_check_for_ack_pf;
	mbx->ops.check_for_rst = txgbe_check_for_rst_pf;

	mbx->stats.msgs_tx = 0;
	mbx->stats.msgs_rx = 0;
	mbx->stats.reqs = 0;
	mbx->stats.acks = 0;
	mbx->stats.rsts = 0;
}
