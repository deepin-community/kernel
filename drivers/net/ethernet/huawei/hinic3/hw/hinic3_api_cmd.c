// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [COMM]" fmt

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/completion.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

#include "ossl_knl.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_common.h"
#include "hinic3_hwdev.h"
#include "hinic3_csr.h"
#include "hinic3_hwif.h"
#include "hinic3_api_cmd.h"

#define API_CMD_CHAIN_CELL_SIZE_SHIFT   6U

#define API_CMD_CELL_DESC_SIZE          8
#define API_CMD_CELL_DATA_ADDR_SIZE     8

#define API_CHAIN_NUM_CELLS             32
#define API_CHAIN_CELL_SIZE             128
#define API_CHAIN_RSP_DATA_SIZE		128

#define API_CMD_CELL_WB_ADDR_SIZE	8

#define API_CHAIN_CELL_ALIGNMENT        8

#define API_CMD_TIMEOUT			10000
#define API_CMD_STATUS_TIMEOUT		10000

#define API_CMD_BUF_SIZE		2048ULL

#define API_CMD_NODE_ALIGN_SIZE		512ULL
#define API_PAYLOAD_ALIGN_SIZE		64ULL

#define API_CHAIN_RESP_ALIGNMENT	128ULL

#define COMPLETION_TIMEOUT_DEFAULT		1000UL
#define POLLING_COMPLETION_TIMEOUT_DEFAULT	1000U

#define API_CMD_RESPONSE_DATA_PADDR(val)	be64_to_cpu(*((u64 *)(val)))

#define READ_API_CMD_PRIV_DATA(id, token)	((((u32)(id)) << 16) + (token))
#define WRITE_API_CMD_PRIV_DATA(id)		(((u8)(id)) << 16)

#define MASKED_IDX(chain, idx)		((idx) & ((chain)->num_cells - 1))

#define SIZE_4BYTES(size)		(ALIGN((u32)(size), 4U) >> 2)
#define SIZE_8BYTES(size)		(ALIGN((u32)(size), 8U) >> 3)

enum api_cmd_data_format {
	SGL_DATA     = 1,
};

enum api_cmd_type {
	API_CMD_WRITE_TYPE = 0,
	API_CMD_READ_TYPE = 1,
};

enum api_cmd_bypass {
	NOT_BYPASS = 0,
	BYPASS = 1,
};

enum api_cmd_resp_aeq {
	NOT_TRIGGER = 0,
	TRIGGER     = 1,
};

enum api_cmd_chn_code {
	APICHN_0 = 0,
};

enum api_cmd_chn_rsvd {
	APICHN_VALID = 0,
	APICHN_INVALID = 1,
};

#define API_DESC_LEN (7)

static u8 xor_chksum_set(void *data)
{
	int idx;
	u8 checksum = 0;
	u8 *val = data;

	for (idx = 0; idx < API_DESC_LEN; idx++)
		checksum ^= val[idx];

	return checksum;
}

static void set_prod_idx(struct hinic3_api_cmd_chain *chain)
{
	enum hinic3_api_cmd_chain_type chain_type = chain->chain_type;
	struct hinic3_hwif *hwif = chain->hwdev->hwif;
	u32 hw_prod_idx_addr = HINIC3_CSR_API_CMD_CHAIN_PI_ADDR(chain_type);
	u32 prod_idx = chain->prod_idx;

	hinic3_hwif_write_reg(hwif, hw_prod_idx_addr, prod_idx);
}

static u32 get_hw_cons_idx(struct hinic3_api_cmd_chain *chain)
{
	u32 addr, val;

	addr = HINIC3_CSR_API_CMD_STATUS_0_ADDR(chain->chain_type);
	val  = hinic3_hwif_read_reg(chain->hwdev->hwif, addr);

	return HINIC3_API_CMD_STATUS_GET(val, CONS_IDX);
}

static void dump_api_chain_reg(struct hinic3_api_cmd_chain *chain)
{
	void *dev = chain->hwdev->dev_hdl;
	u32 addr, val;
	u16 pci_cmd = 0;

	addr = HINIC3_CSR_API_CMD_STATUS_0_ADDR(chain->chain_type);
	val  = hinic3_hwif_read_reg(chain->hwdev->hwif, addr);

	sdk_err(dev, "Chain type: 0x%x, cpld error: 0x%x, check error: 0x%x,  current fsm: 0x%x\n",
		chain->chain_type, HINIC3_API_CMD_STATUS_GET(val, CPLD_ERR),
		HINIC3_API_CMD_STATUS_GET(val, CHKSUM_ERR),
		HINIC3_API_CMD_STATUS_GET(val, FSM));

	sdk_err(dev, "Chain hw current ci: 0x%x\n",
		HINIC3_API_CMD_STATUS_GET(val, CONS_IDX));

	addr = HINIC3_CSR_API_CMD_CHAIN_PI_ADDR(chain->chain_type);
	val  = hinic3_hwif_read_reg(chain->hwdev->hwif, addr);
	sdk_err(dev, "Chain hw current pi: 0x%x\n", val);
	pci_read_config_word(chain->hwdev->pcidev_hdl, PCI_COMMAND, &pci_cmd);
	sdk_err(dev, "PCI command reg: 0x%x\n", pci_cmd);
}

/**
 * chain_busy - check if the chain is still processing last requests
 * @chain: chain to check
 **/
static int chain_busy(struct hinic3_api_cmd_chain *chain)
{
	void *dev = chain->hwdev->dev_hdl;
	struct hinic3_api_cmd_cell_ctxt *ctxt;
	u64 resp_header;

	ctxt = &chain->cell_ctxt[chain->prod_idx];

	switch (chain->chain_type) {
	case HINIC3_API_CMD_MULTI_READ:
	case HINIC3_API_CMD_POLL_READ:
		resp_header = be64_to_cpu(ctxt->resp->header);
		if (ctxt->status &&
		    !HINIC3_API_CMD_RESP_HEADER_VALID(resp_header)) {
			sdk_err(dev, "Context(0x%x) busy!, pi: %u, resp_header: 0x%08x%08x\n",
				ctxt->status, chain->prod_idx,
				upper_32_bits(resp_header),
				lower_32_bits(resp_header));
			dump_api_chain_reg(chain);
			return -EBUSY;
		}
		break;
	case HINIC3_API_CMD_POLL_WRITE:
	case HINIC3_API_CMD_WRITE_TO_MGMT_CPU:
	case HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU:
		chain->cons_idx = get_hw_cons_idx(chain);

		if (chain->cons_idx == MASKED_IDX(chain, chain->prod_idx + 1)) {
			sdk_err(dev, "API CMD chain %d is busy, cons_idx = %u, prod_idx = %u\n",
				chain->chain_type, chain->cons_idx,
				chain->prod_idx);
			dump_api_chain_reg(chain);
			return -EBUSY;
		}
		break;
	default:
		sdk_err(dev, "Unknown Chain type %d\n", chain->chain_type);
		return -EINVAL;
	}

	return 0;
}

/**
 * get_cell_data_size - get the data size of specific cell type
 * @type: chain type
 **/
static u16 get_cell_data_size(enum hinic3_api_cmd_chain_type type)
{
	u16 cell_data_size = 0;

	switch (type) {
	case HINIC3_API_CMD_POLL_READ:
		cell_data_size = ALIGN(API_CMD_CELL_DESC_SIZE +
				    API_CMD_CELL_WB_ADDR_SIZE +
				    API_CMD_CELL_DATA_ADDR_SIZE,
				    API_CHAIN_CELL_ALIGNMENT);
		break;

	case HINIC3_API_CMD_WRITE_TO_MGMT_CPU:
	case HINIC3_API_CMD_POLL_WRITE:
	case HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU:
		cell_data_size = ALIGN(API_CMD_CELL_DESC_SIZE +
				       API_CMD_CELL_DATA_ADDR_SIZE,
				       API_CHAIN_CELL_ALIGNMENT);
		break;
	default:
		break;
	}

	return cell_data_size;
}

/**
 * prepare_cell_ctrl - prepare the ctrl of the cell for the command
 * @cell_ctrl: the control of the cell to set the control into it
 * @cell_len: the size of the cell
 **/
static void prepare_cell_ctrl(u64 *cell_ctrl, u16 cell_len)
{
	u64 ctrl;
	u8 chksum;

	ctrl = HINIC3_API_CMD_CELL_CTRL_SET(SIZE_8BYTES(cell_len), CELL_LEN) |
	       HINIC3_API_CMD_CELL_CTRL_SET(0ULL, RD_DMA_ATTR_OFF) |
	       HINIC3_API_CMD_CELL_CTRL_SET(0ULL, WR_DMA_ATTR_OFF);

	chksum = xor_chksum_set(&ctrl);

	ctrl |= HINIC3_API_CMD_CELL_CTRL_SET(chksum, XOR_CHKSUM);

	/* The data in the HW should be in Big Endian Format */
	*cell_ctrl = cpu_to_be64(ctrl);
}

/**
 * prepare_api_cmd - prepare API CMD command
 * @chain: chain for the command
 * @cell: the cell of the command
 * @node_id: destination node on the card that will receive the command
 * @cmd: command data
 * @cmd_size: the command size
 **/
static void prepare_api_cmd(struct hinic3_api_cmd_chain *chain,
			    struct hinic3_api_cmd_cell *cell, u8 node_id,
			    const void *cmd, u16 cmd_size)
{
	struct hinic3_api_cmd_cell_ctxt	*cell_ctxt;
	u32 priv;

	cell_ctxt = &chain->cell_ctxt[chain->prod_idx];

	switch (chain->chain_type) {
	case HINIC3_API_CMD_POLL_READ:
		priv = READ_API_CMD_PRIV_DATA(chain->chain_type,
					      cell_ctxt->saved_prod_idx);
		cell->desc = HINIC3_API_CMD_DESC_SET(SGL_DATA, API_TYPE) |
			     HINIC3_API_CMD_DESC_SET(API_CMD_READ_TYPE, RD_WR) |
			     HINIC3_API_CMD_DESC_SET(BYPASS, MGMT_BYPASS) |
			     HINIC3_API_CMD_DESC_SET(NOT_TRIGGER,
						     RESP_AEQE_EN) |
			     HINIC3_API_CMD_DESC_SET(priv, PRIV_DATA);
		break;
	case HINIC3_API_CMD_POLL_WRITE:
		priv =  WRITE_API_CMD_PRIV_DATA(chain->chain_type);
		cell->desc = HINIC3_API_CMD_DESC_SET(SGL_DATA, API_TYPE) |
			     HINIC3_API_CMD_DESC_SET(API_CMD_WRITE_TYPE,
						     RD_WR) |
			     HINIC3_API_CMD_DESC_SET(BYPASS, MGMT_BYPASS) |
			     HINIC3_API_CMD_DESC_SET(NOT_TRIGGER,
						     RESP_AEQE_EN) |
			     HINIC3_API_CMD_DESC_SET(priv, PRIV_DATA);
		break;
	case HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU:
	case HINIC3_API_CMD_WRITE_TO_MGMT_CPU:
		priv =  WRITE_API_CMD_PRIV_DATA(chain->chain_type);
		cell->desc = HINIC3_API_CMD_DESC_SET(SGL_DATA, API_TYPE) |
			     HINIC3_API_CMD_DESC_SET(API_CMD_WRITE_TYPE,
						     RD_WR) |
			     HINIC3_API_CMD_DESC_SET(NOT_BYPASS, MGMT_BYPASS) |
			     HINIC3_API_CMD_DESC_SET(TRIGGER, RESP_AEQE_EN) |
			     HINIC3_API_CMD_DESC_SET(priv, PRIV_DATA);
		break;
	default:
		sdk_err(chain->hwdev->dev_hdl, "Unknown Chain type: %d\n",
			chain->chain_type);
		return;
	}

	cell->desc |= HINIC3_API_CMD_DESC_SET(APICHN_0, APICHN_CODE) |
		      HINIC3_API_CMD_DESC_SET(APICHN_VALID, APICHN_RSVD);

	cell->desc |= HINIC3_API_CMD_DESC_SET(node_id, DEST) |
		      HINIC3_API_CMD_DESC_SET(SIZE_4BYTES(cmd_size), SIZE);

	cell->desc |= HINIC3_API_CMD_DESC_SET(xor_chksum_set(&cell->desc),
						XOR_CHKSUM);

	/* The data in the HW should be in Big Endian Format */
	cell->desc = cpu_to_be64(cell->desc);

	memcpy(cell_ctxt->api_cmd_vaddr, cmd, cmd_size);
}

/**
 * prepare_cell - prepare cell ctrl and cmd in the current producer cell
 * @chain: chain for the command
 * @node_id: destination node on the card that will receive the command
 * @cmd: command data
 * @cmd_size: the command size
 * Return: 0 - success, negative - failure
 **/
static void prepare_cell(struct hinic3_api_cmd_chain *chain, u8 node_id,
			 const void *cmd, u16 cmd_size)
{
	struct hinic3_api_cmd_cell *curr_node;
	u16 cell_size;

	curr_node = chain->curr_node;

	cell_size = get_cell_data_size(chain->chain_type);

	prepare_cell_ctrl(&curr_node->ctrl, cell_size);
	prepare_api_cmd(chain, curr_node, node_id, cmd, cmd_size);
}

static inline void cmd_chain_prod_idx_inc(struct hinic3_api_cmd_chain *chain)
{
	chain->prod_idx = MASKED_IDX(chain, chain->prod_idx + 1);
}

static void issue_api_cmd(struct hinic3_api_cmd_chain *chain)
{
	set_prod_idx(chain);
}

/**
 * api_cmd_status_update - update the status of the chain
 * @chain: chain to update
 **/
static void api_cmd_status_update(struct hinic3_api_cmd_chain *chain)
{
	struct hinic3_api_cmd_status *wb_status;
	enum hinic3_api_cmd_chain_type chain_type;
	u64	status_header;
	u32	buf_desc;

	wb_status = chain->wb_status;

	buf_desc = be32_to_cpu(wb_status->buf_desc);
	if (HINIC3_API_CMD_STATUS_GET(buf_desc, CHKSUM_ERR))
		return;

	status_header = be64_to_cpu(wb_status->header);
	chain_type = HINIC3_API_CMD_STATUS_HEADER_GET(status_header, CHAIN_ID);
	if (chain_type >= HINIC3_API_CMD_MAX)
		return;

	if (chain_type != chain->chain_type)
		return;

	chain->cons_idx = HINIC3_API_CMD_STATUS_GET(buf_desc, CONS_IDX);
}

static enum hinic3_wait_return wait_for_status_poll_handler(void *priv_data)
{
	struct hinic3_api_cmd_chain *chain = priv_data;

	if (!chain->hwdev->chip_present_flag)
		return WAIT_PROCESS_ERR;

	api_cmd_status_update(chain);
	/* SYNC API CMD cmd should start after prev cmd finished */
	if (chain->cons_idx == chain->prod_idx)
		return WAIT_PROCESS_CPL;

	return WAIT_PROCESS_WAITING;
}

/**
 * wait_for_status_poll - wait for write to mgmt command to complete
 * @chain: the chain of the command
 * Return: 0 - success, negative - failure
 **/
static int wait_for_status_poll(struct hinic3_api_cmd_chain *chain)
{
	return hinic3_wait_for_timeout(chain,
				      wait_for_status_poll_handler,
				      API_CMD_STATUS_TIMEOUT, 100); /* wait 100 us once */
}

static void copy_resp_data(struct hinic3_api_cmd_chain *chain,
			   struct hinic3_api_cmd_cell_ctxt *ctxt, void *ack,
			   u16 ack_size)
{
	struct hinic3_api_cmd_resp_fmt *resp = ctxt->resp;
	int rsp_size_align = chain->rsp_size_align - 0x8;
	int rsp_size = (ack_size > rsp_size_align) ? rsp_size_align : ack_size;

	memcpy(ack, &resp->resp_data, rsp_size);
	ctxt->status = 0;
}

static enum hinic3_wait_return check_cmd_resp_handler(void *priv_data)
{
	struct hinic3_api_cmd_cell_ctxt *ctxt = priv_data;
	u64 resp_header;
	u8 resp_status;

	if (!ctxt->hwdev->chip_present_flag)
		return WAIT_PROCESS_ERR;

	resp_header = be64_to_cpu(ctxt->resp->header);
	rmb(); /* read the latest header */

	if (HINIC3_API_CMD_RESP_HEADER_VALID(resp_header)) {
		resp_status = HINIC3_API_CMD_RESP_HEAD_GET(resp_header, STATUS);
		if (resp_status) {
			pr_err("Api chain response data err, status: %u\n",
			       resp_status);
			return WAIT_PROCESS_ERR;
		}

		return WAIT_PROCESS_CPL;
	}

	return WAIT_PROCESS_WAITING;
}

/**
 * prepare_cell - polling for respense data of the read api-command
 * @chain: pointer to api cmd chain
 *
 * Return: 0 - success, negative - failure
 **/
static int wait_for_resp_polling(struct hinic3_api_cmd_cell_ctxt *ctxt)
{
	return hinic3_wait_for_timeout(ctxt, check_cmd_resp_handler,
				       POLLING_COMPLETION_TIMEOUT_DEFAULT,
				       USEC_PER_MSEC);
}

/**
 * wait_for_api_cmd_completion - wait for command to complete
 * @chain: chain for the command
 * Return: 0 - success, negative - failure
 **/
static int wait_for_api_cmd_completion(struct hinic3_api_cmd_chain *chain,
				       struct hinic3_api_cmd_cell_ctxt *ctxt,
				       void *ack, u16 ack_size)
{
	void *dev = chain->hwdev->dev_hdl;
	int err = 0;

	switch (chain->chain_type) {
	case HINIC3_API_CMD_POLL_READ:
		err = wait_for_resp_polling(ctxt);
		if (err == 0)
			copy_resp_data(chain, ctxt, ack, ack_size);
		else
			sdk_err(dev, "API CMD poll response timeout\n");
		break;
	case HINIC3_API_CMD_POLL_WRITE:
	case HINIC3_API_CMD_WRITE_TO_MGMT_CPU:
		err = wait_for_status_poll(chain);
		if (err != 0) {
			sdk_err(dev, "API CMD Poll status timeout, chain type: %d\n",
				chain->chain_type);
			break;
		}
		break;
	case HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU:
		/* No need to wait */
		break;
	default:
		sdk_err(dev, "Unknown API CMD Chain type: %d\n",
			chain->chain_type);
		err = -EINVAL;
		break;
	}

	if (err != 0)
		dump_api_chain_reg(chain);

	return err;
}

static inline void update_api_cmd_ctxt(struct hinic3_api_cmd_chain *chain,
				       struct hinic3_api_cmd_cell_ctxt *ctxt)
{
	ctxt->status = 1;
	ctxt->saved_prod_idx = chain->prod_idx;
	if (ctxt->resp) {
		ctxt->resp->header = 0;

		/* make sure "header" was cleared */
		wmb();
	}
}

/**
 * api_cmd - API CMD command
 * @chain: chain for the command
 * @node_id: destination node on the card that will receive the command
 * @cmd: command data
 * @size: the command size
 * Return: 0 - success, negative - failure
 **/
static int api_cmd(struct hinic3_api_cmd_chain *chain, u8 node_id,
		   const void *cmd, u16 cmd_size, void *ack, u16 ack_size)
{
	struct hinic3_api_cmd_cell_ctxt *ctxt = NULL;

	if (chain->chain_type == HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU)
		spin_lock(&chain->async_lock);
	else
		down(&chain->sem);
	ctxt = &chain->cell_ctxt[chain->prod_idx];
	if (chain_busy(chain)) {
		if (chain->chain_type == HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU)
			spin_unlock(&chain->async_lock);
		else
			up(&chain->sem);
		return -EBUSY;
	}
	update_api_cmd_ctxt(chain, ctxt);

	prepare_cell(chain, node_id, cmd, cmd_size);

	cmd_chain_prod_idx_inc(chain);

	wmb(); /* issue the command */

	issue_api_cmd(chain);

	/* incremented prod idx, update ctxt */

	chain->curr_node = chain->cell_ctxt[chain->prod_idx].cell_vaddr;
	if (chain->chain_type == HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU)
		spin_unlock(&chain->async_lock);
	else
		up(&chain->sem);

	return wait_for_api_cmd_completion(chain, ctxt, ack, ack_size);
}

/**
 * hinic3_api_cmd_write - Write API CMD command
 * @chain: chain for write command
 * @node_id: destination node on the card that will receive the command
 * @cmd: command data
 * @size: the command size
 * Return: 0 - success, negative - failure
 **/
int hinic3_api_cmd_write(struct hinic3_api_cmd_chain *chain, u8 node_id,
			 const void *cmd, u16 size)
{
	/* Verify the chain type */
	return api_cmd(chain, node_id, cmd, size, NULL, 0);
}

/**
 * hinic3_api_cmd_read - Read API CMD command
 * @chain: chain for read command
 * @node_id: destination node on the card that will receive the command
 * @cmd: command data
 * @size: the command size
 * Return: 0 - success, negative - failure
 **/
int hinic3_api_cmd_read(struct hinic3_api_cmd_chain *chain, u8 node_id,
			const void *cmd, u16 size, void *ack, u16 ack_size)
{
	return api_cmd(chain, node_id, cmd, size, ack, ack_size);
}

static enum hinic3_wait_return check_chain_restart_handler(void *priv_data)
{
	struct hinic3_api_cmd_chain *cmd_chain = priv_data;
	u32 reg_addr, val;

	if (!cmd_chain->hwdev->chip_present_flag)
		return WAIT_PROCESS_ERR;

	reg_addr = HINIC3_CSR_API_CMD_CHAIN_REQ_ADDR(cmd_chain->chain_type);
	val = hinic3_hwif_read_reg(cmd_chain->hwdev->hwif, reg_addr);
	if (!HINIC3_API_CMD_CHAIN_REQ_GET(val, RESTART))
		return WAIT_PROCESS_CPL;

	return WAIT_PROCESS_WAITING;
}

/**
 * api_cmd_hw_restart - restart the chain in the HW
 * @chain: the API CMD specific chain to restart
 **/
static int api_cmd_hw_restart(struct hinic3_api_cmd_chain *cmd_chain)
{
	struct hinic3_hwif *hwif = cmd_chain->hwdev->hwif;
	u32 reg_addr, val;

	/* Read Modify Write */
	reg_addr = HINIC3_CSR_API_CMD_CHAIN_REQ_ADDR(cmd_chain->chain_type);
	val = hinic3_hwif_read_reg(hwif, reg_addr);

	val = HINIC3_API_CMD_CHAIN_REQ_CLEAR(val, RESTART);
	val |= HINIC3_API_CMD_CHAIN_REQ_SET(1, RESTART);

	hinic3_hwif_write_reg(hwif, reg_addr, val);

	return hinic3_wait_for_timeout(cmd_chain, check_chain_restart_handler,
				       API_CMD_TIMEOUT, USEC_PER_MSEC);
}

/**
 * api_cmd_ctrl_init - set the control register of a chain
 * @chain: the API CMD specific chain to set control register for
 **/
static void api_cmd_ctrl_init(struct hinic3_api_cmd_chain *chain)
{
	struct hinic3_hwif *hwif = chain->hwdev->hwif;
	u32 reg_addr, ctrl;
	u32 size;

	/* Read Modify Write */
	reg_addr = HINIC3_CSR_API_CMD_CHAIN_CTRL_ADDR(chain->chain_type);

	size = (u32)ilog2(chain->cell_size >> API_CMD_CHAIN_CELL_SIZE_SHIFT);

	ctrl = hinic3_hwif_read_reg(hwif, reg_addr);

	ctrl = HINIC3_API_CMD_CHAIN_CTRL_CLEAR(ctrl, AEQE_EN) &
		HINIC3_API_CMD_CHAIN_CTRL_CLEAR(ctrl, CELL_SIZE);

	ctrl |= HINIC3_API_CMD_CHAIN_CTRL_SET(0, AEQE_EN) |
		HINIC3_API_CMD_CHAIN_CTRL_SET(size, CELL_SIZE);

	hinic3_hwif_write_reg(hwif, reg_addr, ctrl);
}

/**
 * api_cmd_set_status_addr - set the status address of a chain in the HW
 * @chain: the API CMD specific chain to set status address for
 **/
static void api_cmd_set_status_addr(struct hinic3_api_cmd_chain *chain)
{
	struct hinic3_hwif *hwif = chain->hwdev->hwif;
	u32 addr, val;

	addr = HINIC3_CSR_API_CMD_STATUS_HI_ADDR(chain->chain_type);
	val = upper_32_bits(chain->wb_status_paddr);
	hinic3_hwif_write_reg(hwif, addr, val);

	addr = HINIC3_CSR_API_CMD_STATUS_LO_ADDR(chain->chain_type);
	val = lower_32_bits(chain->wb_status_paddr);
	hinic3_hwif_write_reg(hwif, addr, val);
}

/**
 * api_cmd_set_num_cells - set the number cells of a chain in the HW
 * @chain: the API CMD specific chain to set the number of cells for
 **/
static void api_cmd_set_num_cells(struct hinic3_api_cmd_chain *chain)
{
	struct hinic3_hwif *hwif = chain->hwdev->hwif;
	u32 addr, val;

	addr = HINIC3_CSR_API_CMD_CHAIN_NUM_CELLS_ADDR(chain->chain_type);
	val  = chain->num_cells;
	hinic3_hwif_write_reg(hwif, addr, val);
}

/**
 * api_cmd_head_init - set the head cell of a chain in the HW
 * @chain: the API CMD specific chain to set the head for
 **/
static void api_cmd_head_init(struct hinic3_api_cmd_chain *chain)
{
	struct hinic3_hwif *hwif = chain->hwdev->hwif;
	u32 addr, val;

	addr = HINIC3_CSR_API_CMD_CHAIN_HEAD_HI_ADDR(chain->chain_type);
	val = upper_32_bits(chain->head_cell_paddr);
	hinic3_hwif_write_reg(hwif, addr, val);

	addr = HINIC3_CSR_API_CMD_CHAIN_HEAD_LO_ADDR(chain->chain_type);
	val = lower_32_bits(chain->head_cell_paddr);
	hinic3_hwif_write_reg(hwif, addr, val);
}

static enum hinic3_wait_return check_chain_ready_handler(void *priv_data)
{
	struct hinic3_api_cmd_chain *chain = priv_data;
	u32 addr, val;
	u32 hw_cons_idx;

	if (!chain->hwdev->chip_present_flag)
		return WAIT_PROCESS_ERR;

	addr = HINIC3_CSR_API_CMD_STATUS_0_ADDR(chain->chain_type);
	val = hinic3_hwif_read_reg(chain->hwdev->hwif, addr);
	hw_cons_idx = HINIC3_API_CMD_STATUS_GET(val, CONS_IDX);
	/* wait for HW cons idx to be updated */
	if (hw_cons_idx == chain->cons_idx)
		return WAIT_PROCESS_CPL;
	return WAIT_PROCESS_WAITING;
}

/**
 * wait_for_ready_chain - wait for the chain to be ready
 * @chain: the API CMD specific chain to wait for
 * Return: 0 - success, negative - failure
 **/
static int wait_for_ready_chain(struct hinic3_api_cmd_chain *chain)
{
	return hinic3_wait_for_timeout(chain, check_chain_ready_handler,
				       API_CMD_TIMEOUT, USEC_PER_MSEC);
}

/**
 * api_cmd_chain_hw_clean - clean the HW
 * @chain: the API CMD specific chain
 **/
static void api_cmd_chain_hw_clean(struct hinic3_api_cmd_chain *chain)
{
	struct hinic3_hwif *hwif = chain->hwdev->hwif;
	u32 addr, ctrl;

	addr = HINIC3_CSR_API_CMD_CHAIN_CTRL_ADDR(chain->chain_type);

	ctrl = hinic3_hwif_read_reg(hwif, addr);
	ctrl = HINIC3_API_CMD_CHAIN_CTRL_CLEAR(ctrl, RESTART_EN) &
	       HINIC3_API_CMD_CHAIN_CTRL_CLEAR(ctrl, XOR_ERR)    &
	       HINIC3_API_CMD_CHAIN_CTRL_CLEAR(ctrl, AEQE_EN)    &
	       HINIC3_API_CMD_CHAIN_CTRL_CLEAR(ctrl, XOR_CHK_EN) &
	       HINIC3_API_CMD_CHAIN_CTRL_CLEAR(ctrl, CELL_SIZE);

	hinic3_hwif_write_reg(hwif, addr, ctrl);
}

/**
 * api_cmd_chain_hw_init - initialize the chain in the HW
 * @chain: the API CMD specific chain to initialize in HW
 * Return: 0 - success, negative - failure
 **/
static int api_cmd_chain_hw_init(struct hinic3_api_cmd_chain *chain)
{
	api_cmd_chain_hw_clean(chain);

	api_cmd_set_status_addr(chain);

	if (api_cmd_hw_restart(chain)) {
		sdk_err(chain->hwdev->dev_hdl, "Failed to restart api_cmd_hw\n");
		return -EBUSY;
	}

	api_cmd_ctrl_init(chain);
	api_cmd_set_num_cells(chain);
	api_cmd_head_init(chain);

	return wait_for_ready_chain(chain);
}

/**
 * alloc_cmd_buf - allocate a dma buffer for API CMD command
 * @chain: the API CMD specific chain for the cmd
 * @cell: the cell in the HW for the cmd
 * @cell_idx: the index of the cell
 * Return: 0 - success, negative - failure
 **/
static int alloc_cmd_buf(struct hinic3_api_cmd_chain *chain,
			 struct hinic3_api_cmd_cell *cell, u32 cell_idx)
{
	struct hinic3_api_cmd_cell_ctxt *cell_ctxt;
	void *dev = chain->hwdev->dev_hdl;
	void *buf_vaddr;
	u64 buf_paddr;
	int err = 0;

	buf_vaddr = (u8 *)((u64)chain->buf_vaddr_base +
		chain->buf_size_align * cell_idx);
	buf_paddr = chain->buf_paddr_base +
		chain->buf_size_align * cell_idx;

	cell_ctxt = &chain->cell_ctxt[cell_idx];

	cell_ctxt->api_cmd_vaddr = buf_vaddr;

	/* set the cmd DMA address in the cell */
	switch (chain->chain_type) {
	case HINIC3_API_CMD_POLL_READ:
		cell->read.hw_cmd_paddr = cpu_to_be64(buf_paddr);
		break;
	case HINIC3_API_CMD_WRITE_TO_MGMT_CPU:
	case HINIC3_API_CMD_POLL_WRITE:
	case HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU:
		/* The data in the HW should be in Big Endian Format */
		cell->write.hw_cmd_paddr = cpu_to_be64(buf_paddr);
		break;
	default:
		sdk_err(dev, "Unknown API CMD Chain type: %d\n",
			chain->chain_type);
		err = -EINVAL;
		break;
	}

	return err;
}

/**
 * alloc_cmd_buf - allocate a resp buffer for API CMD command
 * @chain: the API CMD specific chain for the cmd
 * @cell: the cell in the HW for the cmd
 * @cell_idx: the index of the cell
 **/
static void alloc_resp_buf(struct hinic3_api_cmd_chain *chain,
			   struct hinic3_api_cmd_cell *cell, u32 cell_idx)
{
	struct hinic3_api_cmd_cell_ctxt *cell_ctxt;
	void *resp_vaddr;
	u64 resp_paddr;

	resp_vaddr = (u8 *)((u64)chain->rsp_vaddr_base +
		chain->rsp_size_align * cell_idx);
	resp_paddr = chain->rsp_paddr_base +
		chain->rsp_size_align * cell_idx;

	cell_ctxt = &chain->cell_ctxt[cell_idx];

	cell_ctxt->resp = resp_vaddr;
	cell->read.hw_wb_resp_paddr = cpu_to_be64(resp_paddr);
}

static int hinic3_alloc_api_cmd_cell_buf(struct hinic3_api_cmd_chain *chain,
					 u32 cell_idx,
					 struct hinic3_api_cmd_cell *node)
{
	void *dev = chain->hwdev->dev_hdl;
	int err;

	/* For read chain, we should allocate buffer for the response data */
	if (chain->chain_type == HINIC3_API_CMD_MULTI_READ ||
	    chain->chain_type == HINIC3_API_CMD_POLL_READ)
		alloc_resp_buf(chain, node, cell_idx);

	switch (chain->chain_type) {
	case HINIC3_API_CMD_WRITE_TO_MGMT_CPU:
	case HINIC3_API_CMD_POLL_WRITE:
	case HINIC3_API_CMD_POLL_READ:
	case HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU:
		err = alloc_cmd_buf(chain, node, cell_idx);
		if (err) {
			sdk_err(dev, "Failed to allocate cmd buffer\n");
			goto alloc_cmd_buf_err;
		}
		break;
	/* For api command write and api command read, the data section
	 * is directly inserted in the cell, so no need to allocate.
	 */
	case HINIC3_API_CMD_MULTI_READ:
		chain->cell_ctxt[cell_idx].api_cmd_vaddr =
			&node->read.hw_cmd_paddr;
		break;
	default:
		sdk_err(dev, "Unsupported API CMD chain type\n");
		err = -EINVAL;
		goto alloc_cmd_buf_err;
	}

	return 0;

alloc_cmd_buf_err:

	return err;
}

/**
 * api_cmd_create_cell - create API CMD cell of specific chain
 * @chain: the API CMD specific chain to create its cell
 * @cell_idx: the cell index to create
 * @pre_node: previous cell
 * @node_vaddr: the virt addr of the cell
 * Return: 0 - success, negative - failure
 **/
static int api_cmd_create_cell(struct hinic3_api_cmd_chain *chain, u32 cell_idx,
			       struct hinic3_api_cmd_cell *pre_node,
			       struct hinic3_api_cmd_cell **node_vaddr)
{
	struct hinic3_api_cmd_cell_ctxt *cell_ctxt;
	struct hinic3_api_cmd_cell *node;
	void *cell_vaddr;
	u64 cell_paddr;
	int err;

	cell_vaddr = (void *)((u64)chain->cell_vaddr_base +
		chain->cell_size_align * cell_idx);
	cell_paddr = chain->cell_paddr_base +
		chain->cell_size_align * cell_idx;

	cell_ctxt = &chain->cell_ctxt[cell_idx];
	cell_ctxt->cell_vaddr = cell_vaddr;
	cell_ctxt->hwdev = chain->hwdev;
	node = cell_ctxt->cell_vaddr;

	if (!pre_node) {
		chain->head_node = cell_vaddr;
		chain->head_cell_paddr = (dma_addr_t)cell_paddr;
	} else {
		/* The data in the HW should be in Big Endian Format */
		pre_node->next_cell_paddr = cpu_to_be64(cell_paddr);
	}

	/* Driver software should make sure that there is an empty API
	 * command cell at the end the chain
	 */
	node->next_cell_paddr = 0;

	err = hinic3_alloc_api_cmd_cell_buf(chain, cell_idx, node);
	if (err)
		return err;

	*node_vaddr = node;

	return 0;
}

/**
 * api_cmd_create_cells - create API CMD cells for specific chain
 * @chain: the API CMD specific chain
 * Return: 0 - success, negative - failure
 **/
static int api_cmd_create_cells(struct hinic3_api_cmd_chain *chain)
{
	struct hinic3_api_cmd_cell *node = NULL, *pre_node = NULL;
	void *dev = chain->hwdev->dev_hdl;
	u32 cell_idx;
	int err;

	for (cell_idx = 0; cell_idx < chain->num_cells; cell_idx++) {
		err = api_cmd_create_cell(chain, cell_idx, pre_node, &node);
		if (err) {
			sdk_err(dev, "Failed to create API CMD cell\n");
			return err;
		}

		pre_node = node;
	}

	if (!node)
		return -EFAULT;

	/* set the Final node to point on the start */
	node->next_cell_paddr = cpu_to_be64(chain->head_cell_paddr);

	/* set the current node to be the head */
	chain->curr_node = chain->head_node;
	return 0;
}

/**
 * api_chain_init - initialize API CMD specific chain
 * @chain: the API CMD specific chain to initialize
 * @attr: attributes to set in the chain
 * Return: 0 - success, negative - failure
 **/
static int api_chain_init(struct hinic3_api_cmd_chain *chain,
			  struct hinic3_api_cmd_chain_attr *attr)
{
	void *dev = chain->hwdev->dev_hdl;
	size_t cell_ctxt_size;
	size_t cells_buf_size;
	int err;

	chain->chain_type  = attr->chain_type;
	chain->num_cells = attr->num_cells;
	chain->cell_size = attr->cell_size;
	chain->rsp_size = attr->rsp_size;

	chain->prod_idx  = 0;
	chain->cons_idx  = 0;

	if (chain->chain_type == HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU)
		spin_lock_init(&chain->async_lock);
	else
		sema_init(&chain->sem, 1);

	cell_ctxt_size = chain->num_cells * sizeof(*chain->cell_ctxt);
	if (!cell_ctxt_size) {
		sdk_err(dev, "Api chain cell size cannot be zero\n");
		err = -EINVAL;
		goto alloc_cell_ctxt_err;
	}

	chain->cell_ctxt = kzalloc(cell_ctxt_size, GFP_KERNEL);
	if (!chain->cell_ctxt) {
		err = -ENOMEM;
		goto alloc_cell_ctxt_err;
	}

	chain->wb_status = dma_zalloc_coherent(dev,
					       sizeof(*chain->wb_status),
					       &chain->wb_status_paddr,
					       GFP_KERNEL);
	if (!chain->wb_status) {
		sdk_err(dev, "Failed to allocate DMA wb status\n");
		err = -ENOMEM;
		goto alloc_wb_status_err;
	}

	chain->cell_size_align = ALIGN((u64)chain->cell_size,
				       API_CMD_NODE_ALIGN_SIZE);
	chain->rsp_size_align = ALIGN((u64)chain->rsp_size,
				      API_CHAIN_RESP_ALIGNMENT);
	chain->buf_size_align = ALIGN(API_CMD_BUF_SIZE, API_PAYLOAD_ALIGN_SIZE);

	cells_buf_size = (chain->cell_size_align + chain->rsp_size_align +
			  chain->buf_size_align) * chain->num_cells;

	err = hinic3_dma_zalloc_coherent_align(dev, cells_buf_size,
					       API_CMD_NODE_ALIGN_SIZE,
					       GFP_KERNEL,
					       &chain->cells_addr);
	if (err) {
		sdk_err(dev, "Failed to allocate API CMD cells buffer\n");
		goto alloc_cells_buf_err;
	}

	chain->cell_vaddr_base = chain->cells_addr.align_vaddr;
	chain->cell_paddr_base = chain->cells_addr.align_paddr;

	chain->rsp_vaddr_base = (u8 *)((u64)chain->cell_vaddr_base +
		chain->cell_size_align * chain->num_cells);
	chain->rsp_paddr_base = chain->cell_paddr_base +
		chain->cell_size_align * chain->num_cells;

	chain->buf_vaddr_base = (u8 *)((u64)chain->rsp_vaddr_base +
		chain->rsp_size_align * chain->num_cells);
	chain->buf_paddr_base = chain->rsp_paddr_base +
		chain->rsp_size_align * chain->num_cells;

	return 0;

alloc_cells_buf_err:
	dma_free_coherent(dev, sizeof(*chain->wb_status),
			  chain->wb_status, chain->wb_status_paddr);

alloc_wb_status_err:
	kfree(chain->cell_ctxt);

alloc_cell_ctxt_err:
	if (chain->chain_type == HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU)
		spin_lock_deinit(&chain->async_lock);
	else
		sema_deinit(&chain->sem);
	return err;
}

/**
 * api_chain_free - free API CMD specific chain
 * @chain: the API CMD specific chain to free
 **/
static void api_chain_free(struct hinic3_api_cmd_chain *chain)
{
	void *dev = chain->hwdev->dev_hdl;

	hinic3_dma_free_coherent_align(dev, &chain->cells_addr);

	dma_free_coherent(dev, sizeof(*chain->wb_status),
			  chain->wb_status, chain->wb_status_paddr);
	kfree(chain->cell_ctxt);

	if (chain->chain_type == HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU)
		spin_lock_deinit(&chain->async_lock);
	else
		sema_deinit(&chain->sem);
}

/**
 * api_cmd_create_chain - create API CMD specific chain
 * @chain: the API CMD specific chain to create
 * @attr: attributes to set in the chain
 * Return: 0 - success, negative - failure
 **/
static int api_cmd_create_chain(struct hinic3_api_cmd_chain **cmd_chain,
				struct hinic3_api_cmd_chain_attr *attr)
{
	struct hinic3_hwdev *hwdev = attr->hwdev;
	struct hinic3_api_cmd_chain *chain = NULL;
	int err;

	if (attr->num_cells & (attr->num_cells - 1)) {
		sdk_err(hwdev->dev_hdl, "Invalid number of cells, must be power of 2\n");
		return -EINVAL;
	}

	chain = kzalloc(sizeof(*chain), GFP_KERNEL);
	if (!chain)
		return -ENOMEM;

	chain->hwdev = hwdev;

	err = api_chain_init(chain, attr);
	if (err) {
		sdk_err(hwdev->dev_hdl, "Failed to initialize chain\n");
		goto chain_init_err;
	}

	err = api_cmd_create_cells(chain);
	if (err) {
		sdk_err(hwdev->dev_hdl, "Failed to create cells for API CMD chain\n");
		goto create_cells_err;
	}

	err = api_cmd_chain_hw_init(chain);
	if (err) {
		sdk_err(hwdev->dev_hdl, "Failed to initialize chain HW\n");
		goto chain_hw_init_err;
	}

	*cmd_chain = chain;
	return 0;

chain_hw_init_err:
create_cells_err:
	api_chain_free(chain);

chain_init_err:
	kfree(chain);
	return err;
}

/**
 * api_cmd_destroy_chain - destroy API CMD specific chain
 * @chain: the API CMD specific chain to destroy
 **/
static void api_cmd_destroy_chain(struct hinic3_api_cmd_chain *chain)
{
	api_chain_free(chain);
	kfree(chain);
}

/**
 * hinic3_api_cmd_init - Initialize all the API CMD chains
 * @hwif: the hardware interface of a pci function device
 * @chain: the API CMD chains that will be initialized
 * Return: 0 - success, negative - failure
 **/
int hinic3_api_cmd_init(struct hinic3_hwdev *hwdev,
			struct hinic3_api_cmd_chain **chain)
{
	void *dev = hwdev->dev_hdl;
	struct hinic3_api_cmd_chain_attr attr;
	u8 chain_type, i;
	int err;

	if (COMM_SUPPORT_API_CHAIN(hwdev) == 0)
		return 0;

	attr.hwdev = hwdev;
	attr.num_cells  = API_CHAIN_NUM_CELLS;
	attr.cell_size  = API_CHAIN_CELL_SIZE;
	attr.rsp_size	= API_CHAIN_RSP_DATA_SIZE;

	chain_type = HINIC3_API_CMD_WRITE_TO_MGMT_CPU;
	for (; chain_type < HINIC3_API_CMD_MAX; chain_type++) {
		attr.chain_type = chain_type;

		err = api_cmd_create_chain(&chain[chain_type], &attr);
		if (err) {
			sdk_err(dev, "Failed to create chain %d\n", chain_type);
			goto create_chain_err;
		}
	}

	return 0;

create_chain_err:
	i = HINIC3_API_CMD_WRITE_TO_MGMT_CPU;
	for (; i < chain_type; i++)
		api_cmd_destroy_chain(chain[i]);

	return err;
}

/**
 * hinic3_api_cmd_free - free the API CMD chains
 * @chain: the API CMD chains that will be freed
 **/
void hinic3_api_cmd_free(const struct hinic3_hwdev *hwdev, struct hinic3_api_cmd_chain **chain)
{
	u8 chain_type;

	if (COMM_SUPPORT_API_CHAIN(hwdev) == 0)
		return;

	chain_type = HINIC3_API_CMD_WRITE_TO_MGMT_CPU;

	for (; chain_type < HINIC3_API_CMD_MAX; chain_type++)
		api_cmd_destroy_chain(chain[chain_type]);
}

