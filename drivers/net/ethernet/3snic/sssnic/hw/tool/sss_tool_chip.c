// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */
#define pr_fmt(fmt) KBUILD_MODNAME ": [TOOL]" fmt

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

#include "sss_kernel.h"
#include "sss_hwdev.h"
#include "sss_common.h"
#include "sss_pci_sriov.h"
#include "sss_adapter_mgmt.h"
#include "sss_hwif_adm.h"
#include "sss_hwif_adm_common.h"
#include "sss_hwif_mgmt_common.h"
#include "sss_hwif_ctrlq.h"
#include "sss_hwif_api.h"
#include "sss_hw_common.h"
#include "sss_mgmt_channel.h"
#include "sss_linux_kernel.h"
#include "sss_csr.h"
#include "sss_hw.h"
#include "sss_adapter.h"
#include "sss_tool.h"

#define SSS_TOOL_DW_WIDTH 4

/* completion timeout interval, unit is millisecond */
#define SSS_TOOL_UPDATE_MSG_TIMEOUT	50000U

#define SSS_TOOL_CLP_REG_GAP		0x20
#define SSS_TOOL_CLP_INPUT_BUF_LEN	4096UL
#define SSS_TOOL_CLP_DATA_UNIT		4UL
#define SSS_TOOL_CLP_MAX_DATA_SIZE	(SSS_TOOL_CLP_INPUT_BUF_LEN / SSS_TOOL_CLP_DATA_UNIT)

#define SSS_TOOL_CLP_REQ_SIZE_OFFSET	0
#define SSS_TOOL_CLP_RSP_SIZE_OFFSET	16
#define SSS_TOOL_CLP_BASE_OFFSET	0
#define SSS_TOOL_CLP_LEN_OFFSET		0
#define SSS_TOOL_CLP_START_OFFSET	31
#define SSS_TOOL_CLP_READY_OFFSET	31
#define SSS_TOOL_CLP_OFFSET(member)	(SSS_TOOL_CLP_##member##_OFFSET)

#define SSS_TOOL_CLP_SIZE_MASK		0x7ffUL
#define SSS_TOOL_CLP_BASE_MASK		0x7ffffffUL
#define SSS_TOOL_CLP_LEN_MASK		0x7ffUL
#define SSS_TOOL_CLP_START_MASK		0x1UL
#define SSS_TOOL_CLP_READY_MASK		0x1UL
#define SSS_TOOL_CLP_MASK(member)	(SSS_TOOL_CLP_##member##_MASK)

#define SSS_TOOL_CLP_DELAY_CNT_MAX		200UL
#define SSS_TOOL_CLP_SRAM_SIZE_REG_MAX		0x3ff
#define SSS_TOOL_CLP_SRAM_BASE_REG_MAX		0x7ffffff
#define SSS_TOOL_CLP_LEN_REG_MAX		0x3ff
#define SSS_TOOL_CLP_START_OR_READY_REG_MAX	0x1

#define SSS_TOOL_CLP_DATA_REAL_SIZE(in_size, header) \
		(((in_size) + (u16)sizeof(header) + \
		  (((in_size) % SSS_TOOL_CLP_DATA_UNIT) ? SSS_TOOL_CLP_DATA_UNIT : 0)) / \
		 SSS_TOOL_CLP_DATA_UNIT)

#define SSS_TOOL_CLP_REG_VALUE(value, offset, mask) \
		(((value) >> SSS_TOOL_CLP_OFFSET(offset)) & SSS_TOOL_CLP_MASK(mask))

enum sss_tool_clp_data_type {
	SSS_TOOL_CLP_REQ = 0,
	SSS_TOOL_CLP_RSP = 1
};

enum sss_tool_clp_reg_type {
	SSS_TOOL_CLP_BASE = 0,
	SSS_TOOL_CLP_SIZE = 1,
	SSS_TOOL_CLP_LEN = 2,
	SSS_TOOL_CLP_START_REQ = 3,
	SSS_TOOL_CLP_READY_RSP = 4
};

enum SSS_TOOL_ADM_CSR_DATA_OPERATION {
	SSS_TOOL_ADM_CSR_WRITE = 0x1E,
	SSS_TOOL_ADM_CSR_READ = 0x1F
};

enum SSS_TOOL_ADM_CSR_NEED_RESP_DATA {
	SSS_TOOL_ADM_CSR_NO_RESP_DATA = 0,
	SSS_TOOL_ADM_CSR_NEED_RESP_DATA = 1
};

enum SSS_TOOL_ADM_CSR_DATA_SIZE {
	SSS_TOOL_ADM_CSR_DATA_SZ_32 = 0,
	SSS_TOOL_ADM_CSR_DATA_SZ_64 = 1
};

struct sss_tool_csr_request_adm_data {
	u32 dw0;

	union {
		struct {
			u32 reserved1:13;
			/* this field indicates the write/read data size:
			 * 2'b00: 32 bits
			 * 2'b01: 64 bits
			 * 2'b10~2'b11:reserved
			 */
			u32 data_size:2;
			/* this field indicates that requestor expect receive a
			 * response data or not.
			 * 1'b0: expect not to receive a response data.
			 * 1'b1: expect to receive a response data.
			 */
			u32 need_response:1;
			/* this field indicates the operation that the requestor
			 *  expected.
			 * 5'b1_1110: write value to csr space.
			 * 5'b1_1111: read register from csr space.
			 */
			u32 operation_id:5;
			u32 reserved2:6;
			/* this field specifies the Src node ID for this API
			 * request message.
			 */
			u32 src_node_id:5;
		} bits;

		u32 val32;
	} dw1;

	union {
		struct {
			/* it specifies the CSR address. */
			u32 csr_addr:26;
			u32 reserved3:6;
		} bits;

		u32 val32;
	} dw2;

	/* if data_size=2'b01, it is high 32 bits of write data. else, it is
	 * 32'hFFFF_FFFF.
	 */
	u32 csr_write_data_h;
	/* the low 32 bits of write data. */
	u32 csr_write_data_l;
};

struct sss_tool_csr_read {
	u32 rd_len;
	u32 addr;
};

struct sss_tool_csr_write {
	u32 rd_len;
	u32 addr;
	u8 *data;
};

static u32 sss_tool_get_timeout_val(enum sss_mod_type mod, u16 cmd)
{
	if (mod == SSS_MOD_TYPE_COMM &&
	    (cmd == SSS_COMM_MGMT_CMD_UPDATE_FW ||
	    cmd == SSS_COMM_MGMT_CMD_UPDATE_BIOS ||
	    cmd == SSS_COMM_MGMT_CMD_ACTIVE_FW ||
	    cmd == SSS_COMM_MGMT_CMD_SWITCH_CFG ||
	    cmd == SSS_COMM_MGMT_CMD_HOT_ACTIVE_FW))
		return SSS_TOOL_UPDATE_MSG_TIMEOUT;

	return 0; /* use default mbox/adm timeout time */
}

static int sss_tool_get_clp_reg(void *hwdev, enum sss_tool_clp_data_type data_type,
				enum sss_tool_clp_reg_type type, u32 *addr)
{
	switch (type) {
	case SSS_TOOL_CLP_BASE:
		*addr = (data_type == SSS_TOOL_CLP_REQ) ?
			 SSS_CLP_REG(REQBASE) : SSS_CLP_REG(RSPBASE);
		break;

	case SSS_TOOL_CLP_SIZE:
		*addr = SSS_CLP_REG(SIZE);
		break;

	case SSS_TOOL_CLP_LEN:
		*addr = (data_type == SSS_TOOL_CLP_REQ) ?
			 SSS_CLP_REG(REQ) : SSS_CLP_REG(RSP);
		break;

	case SSS_TOOL_CLP_START_REQ:
		*addr = SSS_CLP_REG(REQ);
		break;

	case SSS_TOOL_CLP_READY_RSP:
		*addr = SSS_CLP_REG(RSP);
		break;

	default:
		*addr = 0;
		break;
	}

	return (*addr == 0) ? -EINVAL : 0;
}

static inline int sss_tool_clp_param_valid(enum sss_tool_clp_data_type data_type,
					   enum sss_tool_clp_reg_type reg_type)
{
	if (data_type == SSS_TOOL_CLP_REQ && reg_type == SSS_TOOL_CLP_READY_RSP)
		return -EINVAL;

	if (data_type == SSS_TOOL_CLP_RSP && reg_type == SSS_TOOL_CLP_START_REQ)
		return -EINVAL;

	return 0;
}

static u32 sss_tool_get_clp_reg_value(struct sss_hwdev *hwdev,
				      enum sss_tool_clp_data_type data_type,
				      enum sss_tool_clp_reg_type reg_type, u32 reg_addr)
{
	u32 value;

	value = sss_chip_read_reg(hwdev->hwif, reg_addr);

	switch (reg_type) {
	case SSS_TOOL_CLP_BASE:
		value = SSS_TOOL_CLP_REG_VALUE(value, BASE, BASE);
		break;

	case SSS_TOOL_CLP_SIZE:
		if (data_type == SSS_TOOL_CLP_REQ)
			value = SSS_TOOL_CLP_REG_VALUE(value, REQ_SIZE, SIZE);
		else
			value = SSS_TOOL_CLP_REG_VALUE(value, RSP_SIZE, SIZE);
		break;

	case SSS_TOOL_CLP_LEN:
		value = SSS_TOOL_CLP_REG_VALUE(value, LEN, LEN);
		break;

	case SSS_TOOL_CLP_START_REQ:
		value = SSS_TOOL_CLP_REG_VALUE(value, START, START);
		break;

	case SSS_TOOL_CLP_READY_RSP:
		value = SSS_TOOL_CLP_REG_VALUE(value, READY, READY);
		break;

	default:
		break;
	}

	return value;
}

static int sss_tool_read_clp_reg(struct sss_hwdev *hwdev,
				 enum sss_tool_clp_data_type data_type,
				 enum sss_tool_clp_reg_type reg_type, u32 *read_value)
{
	u32 reg_addr;
	int ret;

	ret = sss_tool_clp_param_valid(data_type, reg_type);
	if (ret)
		return ret;

	ret = sss_tool_get_clp_reg(hwdev, data_type, reg_type, &reg_addr);
	if (ret)
		return ret;

	*read_value = sss_tool_get_clp_reg_value(hwdev, data_type, reg_type, reg_addr);

	return 0;
}

static int sss_tool_check_reg_value(enum sss_tool_clp_reg_type reg_type, u32 value)
{
	if (reg_type == SSS_TOOL_CLP_BASE &&
	    value > SSS_TOOL_CLP_SRAM_BASE_REG_MAX)
		return -EINVAL;

	if (reg_type == SSS_TOOL_CLP_SIZE &&
	    value > SSS_TOOL_CLP_SRAM_SIZE_REG_MAX)
		return -EINVAL;

	if (reg_type == SSS_TOOL_CLP_LEN &&
	    value > SSS_TOOL_CLP_LEN_REG_MAX)
		return -EINVAL;

	if ((reg_type == SSS_TOOL_CLP_START_REQ ||
	     reg_type == SSS_TOOL_CLP_READY_RSP) &&
	    value > SSS_TOOL_CLP_START_OR_READY_REG_MAX)
		return -EINVAL;

	return 0;
}

static int sss_tool_check_clp_init_status(struct sss_hwdev *hwdev)
{
	int ret;
	u32 reg_value = 0;

	ret = sss_tool_read_clp_reg(hwdev, SSS_TOOL_CLP_REQ,
				    SSS_TOOL_CLP_BASE, &reg_value);
	if (ret || !reg_value) {
		tool_err("Fail to read clp reg: 0x%x\n", reg_value);
		return -EINVAL;
	}

	ret = sss_tool_read_clp_reg(hwdev, SSS_TOOL_CLP_RSP,
				    SSS_TOOL_CLP_BASE, &reg_value);
	if (ret || !reg_value) {
		tool_err("Fail to read rsp ba value: 0x%x\n", reg_value);
		return -EINVAL;
	}

	ret = sss_tool_read_clp_reg(hwdev, SSS_TOOL_CLP_REQ,
				    SSS_TOOL_CLP_SIZE, &reg_value);
	if (ret || !reg_value) {
		tool_err("Fail to read req size\n");
		return -EINVAL;
	}

	ret = sss_tool_read_clp_reg(hwdev, SSS_TOOL_CLP_RSP,
				    SSS_TOOL_CLP_SIZE, &reg_value);
	if (ret || !reg_value) {
		tool_err("Fail to read rsp size\n");
		return -EINVAL;
	}

	return 0;
}

static void sss_tool_write_clp_reg(struct sss_hwdev *hwdev,
				   enum sss_tool_clp_data_type data_type,
				   enum sss_tool_clp_reg_type reg_type, u32 value)
{
	u32 reg_addr, reg_value;

	if (sss_tool_clp_param_valid(data_type, reg_type))
		return;

	if (sss_tool_check_reg_value(reg_type, value))
		return;

	if (sss_tool_get_clp_reg(hwdev, data_type, reg_type, &reg_addr))
		return;

	reg_value = sss_chip_read_reg(hwdev->hwif, reg_addr);

	switch (reg_type) {
	case SSS_TOOL_CLP_LEN:
		reg_value &= (~(SSS_TOOL_CLP_MASK(LEN) << SSS_TOOL_CLP_OFFSET(LEN)));
		reg_value |= (value << SSS_TOOL_CLP_OFFSET(LEN));
		break;

	case SSS_TOOL_CLP_START_REQ:
		reg_value &= (~(SSS_TOOL_CLP_MASK(START) << SSS_TOOL_CLP_OFFSET(START)));
		reg_value |= (value << SSS_TOOL_CLP_OFFSET(START));
		break;

	case SSS_TOOL_CLP_READY_RSP:
		reg_value &= (~(SSS_TOOL_CLP_MASK(READY) << SSS_TOOL_CLP_OFFSET(READY)));
		reg_value |= (value << SSS_TOOL_CLP_OFFSET(READY));
		break;

	default:
		return;
	}

	sss_chip_write_reg(hwdev->hwif, reg_addr, reg_value);
}

static int sss_tool_read_clp_data(struct sss_hwdev *hwdev, void *buf_out, u16 *out_size)
{
	int err;
	u32 reg = SSS_CLP_DATA(RSP);
	u32 ready, delay_cnt;
	u32 *ptr = (u32 *)buf_out;
	u32 temp_out_size = 0;

	err = sss_tool_read_clp_reg(hwdev, SSS_TOOL_CLP_RSP,
				    SSS_TOOL_CLP_READY_RSP, &ready);
	if (err)
		return err;

	delay_cnt = 0;
	while (ready == 0) {
		usleep_range(9000, 10000); /* sleep 9000 us ~ 10000 us */
		delay_cnt++;
		err = sss_tool_read_clp_reg(hwdev, SSS_TOOL_CLP_RSP,
					    SSS_TOOL_CLP_READY_RSP, &ready);
		if (err || delay_cnt > SSS_TOOL_CLP_DELAY_CNT_MAX) {
			tool_err("Fail to read clp delay rsp, timeout delay_cnt: %u\n",
				 delay_cnt);
			return -EINVAL;
		}
	}

	err = sss_tool_read_clp_reg(hwdev, SSS_TOOL_CLP_RSP,
				    SSS_TOOL_CLP_LEN, &temp_out_size);
	if (err)
		return err;

	if (temp_out_size > SSS_TOOL_CLP_SRAM_SIZE_REG_MAX || !temp_out_size) {
		tool_err("Invalid temp out size: %u\n", temp_out_size);
		return -EINVAL;
	}

	*out_size = (u16)temp_out_size;
	for (; temp_out_size > 0; temp_out_size--) {
		*ptr = sss_chip_read_reg(hwdev->hwif, reg);
		ptr++;
		/* read 4 bytes every time */
		reg = reg + 4;
	}

	sss_tool_write_clp_reg(hwdev, SSS_TOOL_CLP_RSP,
			       SSS_TOOL_CLP_READY_RSP, (u32)0x0);
	sss_tool_write_clp_reg(hwdev, SSS_TOOL_CLP_RSP, SSS_TOOL_CLP_LEN, (u32)0x0);

	return 0;
}

static int sss_tool_write_clp_data(struct sss_hwdev *hwdev, void *buf_in, u16 in_size)
{
	int ret;
	u32 reg = SSS_CLP_DATA(REQ);
	u32 start = 1;
	u32 delay_cnt = 0;
	u32 *ptr = (u32 *)buf_in;
	u16 size_in = in_size;

	ret = sss_tool_read_clp_reg(hwdev, SSS_TOOL_CLP_REQ,
				    SSS_TOOL_CLP_START_REQ, &start);
	if (ret != 0)
		return ret;

	while (start == 1) {
		usleep_range(9000, 10000); /* sleep 9000 us ~ 10000 us */
		delay_cnt++;
		ret = sss_tool_read_clp_reg(hwdev, SSS_TOOL_CLP_REQ,
					    SSS_TOOL_CLP_START_REQ, &start);
		if (ret || delay_cnt > SSS_TOOL_CLP_DELAY_CNT_MAX)
			return -EINVAL;
	}

	sss_tool_write_clp_reg(hwdev, SSS_TOOL_CLP_REQ, SSS_TOOL_CLP_LEN, size_in);
	sss_tool_write_clp_reg(hwdev, SSS_TOOL_CLP_REQ, SSS_TOOL_CLP_START_REQ, (u32)0x1);

	for (; size_in > 0; size_in--) {
		sss_chip_write_reg(hwdev->hwif, reg, *ptr);
		ptr++;
		reg = reg + sizeof(u32);
	}

	return 0;
}

static void sss_tool_clear_clp_data(struct sss_hwdev *hwdev,
				    enum sss_tool_clp_data_type data_type)
{
	u32 reg = (data_type == SSS_TOOL_CLP_REQ) ?
		   SSS_CLP_DATA(REQ) : SSS_CLP_DATA(RSP);
	u32 count = SSS_TOOL_CLP_MAX_DATA_SIZE;

	for (; count > 0; count--) {
		sss_chip_write_reg(hwdev->hwif, reg, 0x0);
		reg = reg + sizeof(u32);
	}
}

static void sss_tool_clp_prepare_header(struct sss_hwdev *hwdev, u64 *header,
					u16 msg_len, u8 mod, enum sss_mgmt_cmd cmd)
{
	struct sss_hwif *hwif = hwdev->hwif;

	*header = SSS_SET_MSG_HEADER(msg_len, MSG_LEN) |
		  SSS_SET_MSG_HEADER(mod, MODULE) |
		  SSS_SET_MSG_HEADER(msg_len, SEG_LEN) |
		  SSS_SET_MSG_HEADER(0, NO_ACK) |
		  SSS_SET_MSG_HEADER(SSS_INLINE_DATA, DATA_TYPE) |
		  SSS_SET_MSG_HEADER(0, SEQID) |
		  SSS_SET_MSG_HEADER(SSS_ADM_MSG_AEQ_ID, AEQ_ID) |
		  SSS_SET_MSG_HEADER(SSS_LAST_SEG, LAST) |
		  SSS_SET_MSG_HEADER(0, DIRECTION) |
		  SSS_SET_MSG_HEADER(cmd, CMD) |
		  SSS_SET_MSG_HEADER(hwif->attr.func_id, SRC_GLB_FUNC_ID) |
		  SSS_SET_MSG_HEADER(0, MSG_ID);
}

int sss_tool_send_clp_msg(struct sss_hwdev *hwdev, u8 mod, u16 cmd, const void *buf_in,
			  u16 in_size, void *buf_out, u16 *out_size)

{
	struct sss_clp_pf_to_mgmt *clp_msg;
	u64 header;
	u16 size;
	u8 *msg_buf;
	int ret;

	if (!hwdev || SSS_GET_FUNC_TYPE(hwdev) == SSS_FUNC_TYPE_VF)
		return -EINVAL;

	if (!hwdev->chip_present_flag || !SSS_SUPPORT_CLP(hwdev))
		return -EPERM;

	clp_msg = hwdev->clp_pf_to_mgmt;
	if (!clp_msg)
		return -EPERM;

	msg_buf = clp_msg->clp_msg_buf;

	/* 4 bytes alignment */
	size = SSS_TOOL_CLP_DATA_REAL_SIZE(in_size, header);

	if (size > SSS_TOOL_CLP_MAX_DATA_SIZE) {
		tool_err("Invalid data size: %u\n", size);
		return -EINVAL;
	}
	down(&clp_msg->clp_msg_lock);

	ret = sss_tool_check_clp_init_status(hwdev);
	if (ret) {
		tool_err("Fail to check clp init status\n");
		up(&clp_msg->clp_msg_lock);
		return ret;
	}

	sss_tool_clear_clp_data(hwdev, SSS_TOOL_CLP_RSP);
	sss_tool_write_clp_reg(hwdev, SSS_TOOL_CLP_RSP,
			       SSS_TOOL_CLP_READY_RSP, 0x0);

	/* Send request */
	memset(msg_buf, 0x0, SSS_TOOL_CLP_INPUT_BUF_LEN);
	sss_tool_clp_prepare_header(hwdev, &header, in_size, mod, cmd);

	memcpy(msg_buf, &header, sizeof(header));
	msg_buf += sizeof(header);
	memcpy(msg_buf, buf_in, in_size);

	msg_buf = clp_msg->clp_msg_buf;

	sss_tool_clear_clp_data(hwdev, SSS_TOOL_CLP_REQ);
	ret = sss_tool_write_clp_data(hwdev, clp_msg->clp_msg_buf, size);
	if (ret) {
		tool_err("Fail to send clp request\n");
		up(&clp_msg->clp_msg_lock);
		return -EINVAL;
	}

	/* Get response */
	msg_buf = clp_msg->clp_msg_buf;
	memset(msg_buf, 0x0, SSS_TOOL_CLP_INPUT_BUF_LEN);
	ret = sss_tool_read_clp_data(hwdev, msg_buf, &size);
	sss_tool_clear_clp_data(hwdev, SSS_TOOL_CLP_RSP);
	if (ret) {
		tool_err("Fail to read clp response\n");
		up(&clp_msg->clp_msg_lock);
		return -EINVAL;
	}

	size = (u16)((size * SSS_TOOL_CLP_DATA_UNIT) & 0xffff);
	if (size <= sizeof(header) || size > SSS_TOOL_CLP_INPUT_BUF_LEN) {
		tool_err("Invalid response size: %u", size);
		up(&clp_msg->clp_msg_lock);
		return -EINVAL;
	}

	if (size != *out_size + sizeof(header)) {
		tool_err("Invalid size:%u, out_size: %u\n", size, *out_size);
		up(&clp_msg->clp_msg_lock);
		return -EINVAL;
	}

	memcpy(buf_out, (msg_buf + sizeof(header)), size);
	up(&clp_msg->clp_msg_lock);

	return 0;
}

int sss_tool_adm_csr_rd32(struct sss_hwdev *hwdev, u8 dest, u32 addr, u32 *val)
{
	int ret;
	u32 csr_val = 0;
	struct sss_tool_csr_request_adm_data adm_data = {0};

	if (!hwdev || !val)
		return -EFAULT;

	if (!SSS_SUPPORT_ADM_MSG(hwdev))
		return -EPERM;

	adm_data.dw0 = 0;
	adm_data.dw1.bits.operation_id = SSS_TOOL_ADM_CSR_READ;
	adm_data.dw1.bits.need_response = SSS_TOOL_ADM_CSR_NEED_RESP_DATA;
	adm_data.dw1.bits.data_size = SSS_TOOL_ADM_CSR_DATA_SZ_32;
	adm_data.dw1.val32 = cpu_to_be32(adm_data.dw1.val32);
	adm_data.dw2.bits.csr_addr = addr;
	adm_data.dw2.val32 = cpu_to_be32(adm_data.dw2.val32);

	ret = sss_adm_msg_read_ack(hwdev, dest, (u8 *)(&adm_data),
				   sizeof(adm_data), &csr_val, 0x4);
	if (ret) {
		tool_err("Fail to read 32 bit csr, dest %u addr 0x%x, ret: 0x%x\n",
			 dest, addr, ret);
		return ret;
	}

	*val = csr_val;

	return 0;
}

int sss_tool_adm_csr_wr32(struct sss_hwdev *hwdev, u8 dest, u32 addr, u32 val)
{
	int ret;
	struct sss_tool_csr_request_adm_data adm_data = {0};

	if (!hwdev)
		return -EFAULT;

	if (!SSS_SUPPORT_ADM_MSG(hwdev))
		return -EPERM;

	adm_data.dw1.bits.operation_id = SSS_TOOL_ADM_CSR_WRITE;
	adm_data.dw1.bits.need_response = SSS_TOOL_ADM_CSR_NO_RESP_DATA;
	adm_data.dw1.bits.data_size = SSS_TOOL_ADM_CSR_DATA_SZ_32;
	adm_data.dw1.val32 = cpu_to_be32(adm_data.dw1.val32);
	adm_data.dw2.bits.csr_addr = addr;
	adm_data.dw2.val32 = cpu_to_be32(adm_data.dw2.val32);
	adm_data.csr_write_data_h = 0xffffffff;
	adm_data.csr_write_data_l = val;

	ret = sss_adm_msg_write_nack(hwdev, dest, (u8 *)(&adm_data), sizeof(adm_data));
	if (ret) {
		tool_err("Fail to write 32 bit csr! dest %u addr 0x%x val 0x%x\n",
			 dest, addr, val);
		return ret;
	}

	return 0;
}

static int sss_tool_adm_csr_read(void *hwdev, struct sss_tool_msg *tool_msg,
				 void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
	int ret = 0;
	u32 cnt = 0;
	u32 offset = 0;
	u32 i;
	struct sss_tool_csr_read *rd_msg = (struct sss_tool_csr_read *)buf_in;
	u8 node_id = (u8)tool_msg->mpu_cmd.mod;
	u32 rd_len = rd_msg->rd_len;
	u32 rd_addr = rd_msg->addr;

	if (!buf_in || !buf_out || in_size != sizeof(*rd_msg) ||
	    *out_size != rd_len || rd_len % SSS_TOOL_DW_WIDTH != 0)
		return -EINVAL;

	cnt = rd_len / SSS_TOOL_DW_WIDTH;
	for (i = 0; i < cnt; i++) {
		ret = sss_tool_adm_csr_rd32(hwdev, node_id, rd_addr + offset,
					    (u32 *)(((u8 *)buf_out) + offset));
		if (ret) {
			tool_err("Fail to read csr, err: %d, node_id: %u, csr addr: 0x%08x\n",
				 ret, node_id, rd_addr + offset);
			return ret;
		}
		offset += SSS_TOOL_DW_WIDTH;
	}
	*out_size = rd_len;

	return ret;
}

static int sss_tool_adm_csr_write(void *hwdev, struct sss_tool_msg *tool_msg,
				  void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
	int ret = 0;
	u32 cnt = 0;
	u32 offset = 0;
	u32 i;
	struct sss_tool_csr_write *wr_msg = (struct sss_tool_csr_write *)buf_in;
	u8 node_id = (u8)tool_msg->mpu_cmd.mod;
	u32 rd_len = wr_msg->rd_len;
	u32 rd_addr = wr_msg->addr;
	u8 *data = NULL;

	if (!buf_in || in_size != sizeof(*wr_msg) ||
	    wr_msg->rd_len % SSS_TOOL_DW_WIDTH != 0)
		return -EINVAL;

	data = kzalloc(rd_len, GFP_KERNEL);
	if (!data)
		return -EFAULT;

	if (copy_from_user(data, (void *)wr_msg->data, rd_len)) {
		tool_err("Fail to copy information from user\n");
		kfree(data);
		return -EFAULT;
	}

	cnt = rd_len / SSS_TOOL_DW_WIDTH;
	for (i = 0; i < cnt; i++) {
		ret = sss_tool_adm_csr_wr32(hwdev, node_id, rd_addr + offset,
					    *((u32 *)(data + offset)));
		if (ret) {
			tool_err("Fail to write csr, ret: %d, node_id: %u, csr addr: 0x%08x\n",
				 ret, rd_addr + offset, node_id);
			kfree(data);
			return ret;
		}
		offset += SSS_TOOL_DW_WIDTH;
	}

	*out_size = 0;
	kfree(data);
	return ret;
}

int sss_tool_msg_to_mpu(struct sss_hal_dev *hal_dev, struct sss_tool_msg *tool_msg,
			void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
	int ret = 0;
	u16 cmd = tool_msg->mpu_cmd.cmd;
	enum sss_mod_type mod = (enum sss_mod_type)tool_msg->mpu_cmd.mod;
	u32 timeout = sss_tool_get_timeout_val(mod, cmd);
	void *hwdev = hal_dev->hwdev;

	if (tool_msg->mpu_cmd.channel == SSS_TOOL_CHANNEL_MBOX ||
	    tool_msg->mpu_cmd.channel == SSS_TOOL_CHANNEL_CLP) {
		if (tool_msg->mpu_cmd.channel == SSS_TOOL_CHANNEL_MBOX) {
			ret = sss_sync_mbx_send_msg(hwdev, mod, cmd, buf_in, (u16)in_size,
						    buf_out, (u16 *)out_size, timeout,
						    SSS_CHANNEL_DEFAULT);
		} else {
			ret = sss_tool_send_clp_msg(hwdev, mod, cmd, buf_in, (u16)in_size,
						    buf_out, (u16 *)out_size);
		}

		if (ret) {
			tool_err("Fail to send msg to mgmt cpu, mod: %d, cmd: %u\n", mod, cmd);
			return ret;
		}

	} else if (tool_msg->mpu_cmd.channel == SSS_TOOL_CHANNEL_ADM_MSG_BYPASS) {
		if (tool_msg->mpu_cmd.cmd == SSS_TOOL_ADM_MSG_WRITE)
			return sss_tool_adm_csr_write(hwdev, tool_msg, buf_in, in_size,
						      buf_out, out_size);

		ret = sss_tool_adm_csr_read(hwdev, tool_msg, buf_in, in_size, buf_out, out_size);
	} else if (tool_msg->mpu_cmd.channel == SSS_TOOL_CHANNEL_ADM_MSG_TO_MPU) {
		if (SSS_GET_HWIF_PCI_INTF_ID(SSS_TO_HWIF(hwdev)) != SSS_SPU_HOST_ID)
			ret = sss_sync_send_adm_msg(hwdev, mod, cmd, buf_in, (u16)in_size,
						    buf_out, (u16 *)out_size, timeout);
		else
			ret = sss_sync_mbx_send_msg(hwdev, mod, cmd, buf_in, (u16)in_size,
						    buf_out, (u16 *)out_size, timeout,
						    SSS_CHANNEL_DEFAULT);

		if (ret) {
			tool_err("Fail to send adm msg to mgmt cpu, mod: %d, cmd: %u\n",
				 mod, cmd);
			return ret;
		}

	} else {
		tool_err("Invalid channel %d\n", tool_msg->mpu_cmd.channel);
		return -EINVAL;
	}

	return ret;
}

int sss_tool_msg_to_npu(struct sss_hal_dev *hal_dev, struct sss_tool_msg *tool_msg,
			void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
	int ret = 0;
	u8 cmd = tool_msg->npu_cmd.cmd;
	enum sss_mod_type mod = (enum sss_mod_type)tool_msg->npu_cmd.mod;

	if (tool_msg->npu_cmd.direct_resp) {
		ret = sss_ctrlq_direct_reply(hal_dev->hwdev, mod, cmd, buf_in,
					     buf_out, 0, SSS_CHANNEL_DEFAULT);
		if (ret)
			tool_err("Fail to send direct ctrlq, ret: %d\n", ret);
	} else {
		ret = sss_ctrlq_sync_cmd_detail_reply(hal_dev->hwdev, mod, cmd, buf_in, buf_out,
						      NULL, 0, SSS_CHANNEL_DEFAULT);
		if (ret)
			tool_err("Fail to send detail ctrlq, ret: %d\n", ret);
	}

	return ret;
}
