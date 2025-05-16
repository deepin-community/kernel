/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_API_CMD_H
#define HINIC3_API_CMD_H

#include <linux/semaphore.h>

#include "hinic3_eqs.h"
#include "hinic3_hwif.h"

/* api_cmd_cell.ctrl structure */
#define HINIC3_API_CMD_CELL_CTRL_CELL_LEN_SHIFT			0
#define HINIC3_API_CMD_CELL_CTRL_RD_DMA_ATTR_OFF_SHIFT		16
#define HINIC3_API_CMD_CELL_CTRL_WR_DMA_ATTR_OFF_SHIFT		24
#define HINIC3_API_CMD_CELL_CTRL_XOR_CHKSUM_SHIFT		56

#define HINIC3_API_CMD_CELL_CTRL_CELL_LEN_MASK			0x3FU
#define HINIC3_API_CMD_CELL_CTRL_RD_DMA_ATTR_OFF_MASK		0x3FU
#define HINIC3_API_CMD_CELL_CTRL_WR_DMA_ATTR_OFF_MASK		0x3FU
#define HINIC3_API_CMD_CELL_CTRL_XOR_CHKSUM_MASK			0xFFU

#define HINIC3_API_CMD_CELL_CTRL_SET(val, member)		\
		((((u64)(val)) & HINIC3_API_CMD_CELL_CTRL_##member##_MASK) << \
		HINIC3_API_CMD_CELL_CTRL_##member##_SHIFT)

/* api_cmd_cell.desc structure */
#define HINIC3_API_CMD_DESC_API_TYPE_SHIFT			0
#define HINIC3_API_CMD_DESC_RD_WR_SHIFT				1
#define HINIC3_API_CMD_DESC_MGMT_BYPASS_SHIFT			2
#define HINIC3_API_CMD_DESC_RESP_AEQE_EN_SHIFT			3
#define HINIC3_API_CMD_DESC_APICHN_RSVD_SHIFT			4
#define HINIC3_API_CMD_DESC_APICHN_CODE_SHIFT			6
#define HINIC3_API_CMD_DESC_PRIV_DATA_SHIFT			8
#define HINIC3_API_CMD_DESC_DEST_SHIFT				32
#define HINIC3_API_CMD_DESC_SIZE_SHIFT				40
#define HINIC3_API_CMD_DESC_XOR_CHKSUM_SHIFT			56

#define HINIC3_API_CMD_DESC_API_TYPE_MASK			0x1U
#define HINIC3_API_CMD_DESC_RD_WR_MASK				0x1U
#define HINIC3_API_CMD_DESC_MGMT_BYPASS_MASK			0x1U
#define HINIC3_API_CMD_DESC_RESP_AEQE_EN_MASK			0x1U
#define HINIC3_API_CMD_DESC_APICHN_RSVD_MASK			0x3U
#define HINIC3_API_CMD_DESC_APICHN_CODE_MASK			0x3U
#define HINIC3_API_CMD_DESC_PRIV_DATA_MASK			0xFFFFFFU
#define HINIC3_API_CMD_DESC_DEST_MASK				0x1FU
#define HINIC3_API_CMD_DESC_SIZE_MASK				0x7FFU
#define HINIC3_API_CMD_DESC_XOR_CHKSUM_MASK			0xFFU

#define HINIC3_API_CMD_DESC_SET(val, member)			\
		((((u64)(val)) & HINIC3_API_CMD_DESC_##member##_MASK) << \
		HINIC3_API_CMD_DESC_##member##_SHIFT)

/* api_cmd_status header */
#define HINIC3_API_CMD_STATUS_HEADER_VALID_SHIFT		0
#define HINIC3_API_CMD_STATUS_HEADER_CHAIN_ID_SHIFT		16

#define HINIC3_API_CMD_STATUS_HEADER_VALID_MASK			0xFFU
#define HINIC3_API_CMD_STATUS_HEADER_CHAIN_ID_MASK		0xFFU

#define HINIC3_API_CMD_STATUS_HEADER_GET(val, member)		\
	      (((val) >> HINIC3_API_CMD_STATUS_HEADER_##member##_SHIFT) & \
	      HINIC3_API_CMD_STATUS_HEADER_##member##_MASK)

/* API_CHAIN_REQ CSR: 0x0020+api_idx*0x080 */
#define HINIC3_API_CMD_CHAIN_REQ_RESTART_SHIFT			1
#define HINIC3_API_CMD_CHAIN_REQ_WB_TRIGGER_SHIFT		2

#define HINIC3_API_CMD_CHAIN_REQ_RESTART_MASK			0x1U
#define HINIC3_API_CMD_CHAIN_REQ_WB_TRIGGER_MASK			0x1U

#define HINIC3_API_CMD_CHAIN_REQ_SET(val, member)		\
	       (((val) & HINIC3_API_CMD_CHAIN_REQ_##member##_MASK) << \
	       HINIC3_API_CMD_CHAIN_REQ_##member##_SHIFT)

#define HINIC3_API_CMD_CHAIN_REQ_GET(val, member)		\
	      (((val) >> HINIC3_API_CMD_CHAIN_REQ_##member##_SHIFT) & \
	      HINIC3_API_CMD_CHAIN_REQ_##member##_MASK)

#define HINIC3_API_CMD_CHAIN_REQ_CLEAR(val, member)		\
	((val) & (~(HINIC3_API_CMD_CHAIN_REQ_##member##_MASK	\
		<< HINIC3_API_CMD_CHAIN_REQ_##member##_SHIFT)))

/* API_CHAIN_CTL CSR: 0x0014+api_idx*0x080 */
#define HINIC3_API_CMD_CHAIN_CTRL_RESTART_EN_SHIFT		1
#define HINIC3_API_CMD_CHAIN_CTRL_XOR_ERR_SHIFT			2
#define HINIC3_API_CMD_CHAIN_CTRL_AEQE_EN_SHIFT			4
#define HINIC3_API_CMD_CHAIN_CTRL_AEQ_ID_SHIFT			8
#define HINIC3_API_CMD_CHAIN_CTRL_XOR_CHK_EN_SHIFT		28
#define HINIC3_API_CMD_CHAIN_CTRL_CELL_SIZE_SHIFT		30

#define HINIC3_API_CMD_CHAIN_CTRL_RESTART_EN_MASK		0x1U
#define HINIC3_API_CMD_CHAIN_CTRL_XOR_ERR_MASK			0x1U
#define HINIC3_API_CMD_CHAIN_CTRL_AEQE_EN_MASK			0x1U
#define HINIC3_API_CMD_CHAIN_CTRL_AEQ_ID_MASK			0x3U
#define HINIC3_API_CMD_CHAIN_CTRL_XOR_CHK_EN_MASK		0x3U
#define HINIC3_API_CMD_CHAIN_CTRL_CELL_SIZE_MASK			0x3U

#define HINIC3_API_CMD_CHAIN_CTRL_SET(val, member)		\
	(((val) & HINIC3_API_CMD_CHAIN_CTRL_##member##_MASK) << \
	HINIC3_API_CMD_CHAIN_CTRL_##member##_SHIFT)

#define HINIC3_API_CMD_CHAIN_CTRL_CLEAR(val, member)		\
	((val) & (~(HINIC3_API_CMD_CHAIN_CTRL_##member##_MASK	\
		<< HINIC3_API_CMD_CHAIN_CTRL_##member##_SHIFT)))

/* api_cmd rsp header */
#define HINIC3_API_CMD_RESP_HEAD_VALID_SHIFT		0
#define HINIC3_API_CMD_RESP_HEAD_STATUS_SHIFT		8
#define HINIC3_API_CMD_RESP_HEAD_CHAIN_ID_SHIFT		16
#define HINIC3_API_CMD_RESP_HEAD_RESP_LEN_SHIFT		24
#define HINIC3_API_CMD_RESP_HEAD_DRIVER_PRIV_SHIFT	40

#define HINIC3_API_CMD_RESP_HEAD_VALID_MASK		0xFF
#define HINIC3_API_CMD_RESP_HEAD_STATUS_MASK		0xFFU
#define HINIC3_API_CMD_RESP_HEAD_CHAIN_ID_MASK		0xFFU
#define HINIC3_API_CMD_RESP_HEAD_RESP_LEN_MASK		0x1FFU
#define HINIC3_API_CMD_RESP_HEAD_DRIVER_PRIV_MASK	0xFFFFFFU

#define HINIC3_API_CMD_RESP_HEAD_VALID_CODE		0xFF

#define HINIC3_API_CMD_RESP_HEADER_VALID(val)	\
		(((val) & HINIC3_API_CMD_RESP_HEAD_VALID_MASK) == \
		HINIC3_API_CMD_RESP_HEAD_VALID_CODE)

#define HINIC3_API_CMD_RESP_HEAD_GET(val, member) \
		(((val) >> HINIC3_API_CMD_RESP_HEAD_##member##_SHIFT) & \
		HINIC3_API_CMD_RESP_HEAD_##member##_MASK)

#define HINIC3_API_CMD_RESP_HEAD_CHAIN_ID(val)	\
		(((val) >> HINIC3_API_CMD_RESP_HEAD_CHAIN_ID_SHIFT) & \
		HINIC3_API_CMD_RESP_HEAD_CHAIN_ID_MASK)

#define HINIC3_API_CMD_RESP_HEAD_DRIVER_PRIV(val)	\
		((u16)(((val) >> HINIC3_API_CMD_RESP_HEAD_DRIVER_PRIV_SHIFT) & \
		HINIC3_API_CMD_RESP_HEAD_DRIVER_PRIV_MASK))
/* API_STATUS_0 CSR: 0x0030+api_idx*0x080 */
#define HINIC3_API_CMD_STATUS_CONS_IDX_MASK		0xFFFFFFU
#define HINIC3_API_CMD_STATUS_CONS_IDX_SHIFT		0

#define HINIC3_API_CMD_STATUS_FSM_MASK			0xFU
#define HINIC3_API_CMD_STATUS_FSM_SHIFT			24

#define HINIC3_API_CMD_STATUS_CHKSUM_ERR_MASK		0x3U
#define HINIC3_API_CMD_STATUS_CHKSUM_ERR_SHIFT		28

#define HINIC3_API_CMD_STATUS_CPLD_ERR_MASK		0x1U
#define HINIC3_API_CMD_STATUS_CPLD_ERR_SHIFT		30

#define HINIC3_API_CMD_STATUS_CONS_IDX(val) \
		((val) & HINIC3_API_CMD_STATUS_CONS_IDX_MASK)

#define HINIC3_API_CMD_STATUS_CHKSUM_ERR(val) \
		(((val) >> HINIC3_API_CMD_STATUS_CHKSUM_ERR_SHIFT) & \
		HINIC3_API_CMD_STATUS_CHKSUM_ERR_MASK)

#define HINIC3_API_CMD_STATUS_GET(val, member)			\
		(((val) >> HINIC3_API_CMD_STATUS_##member##_SHIFT) & \
		HINIC3_API_CMD_STATUS_##member##_MASK)

enum hinic3_api_cmd_chain_type {
	/* write to mgmt cpu command with completion  */
	HINIC3_API_CMD_WRITE_TO_MGMT_CPU	= 2,
	/* multi read command with completion notification - not used */
	HINIC3_API_CMD_MULTI_READ		= 3,
	/* write command without completion notification */
	HINIC3_API_CMD_POLL_WRITE		= 4,
	/* read command without completion notification */
	HINIC3_API_CMD_POLL_READ		= 5,
	/* read from mgmt cpu command with completion */
	HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU	= 6,
	HINIC3_API_CMD_MAX,
};

struct hinic3_api_cmd_status {
	u64 header;
	u32 buf_desc;
	u32 cell_addr_hi;
	u32 cell_addr_lo;
	u32 rsvd0;
	u64 rsvd1;
};

/* HW struct */
struct hinic3_api_cmd_cell {
	u64 ctrl;

	/* address is 64 bit in HW struct */
	u64 next_cell_paddr;

	u64 desc;

	/* HW struct */
	union {
		struct {
			u64 hw_cmd_paddr;
		} write;

		struct {
			u64 hw_wb_resp_paddr;
			u64 hw_cmd_paddr;
		} read;
	};
};

struct hinic3_api_cmd_resp_fmt {
	u64				header;
	u64				resp_data;
};

struct hinic3_api_cmd_cell_ctxt {
	struct hinic3_api_cmd_cell	*cell_vaddr;

	void				*api_cmd_vaddr;

	struct hinic3_api_cmd_resp_fmt	*resp;

	struct completion		done;
	int				status;

	u32				saved_prod_idx;
	struct hinic3_hwdev		*hwdev;
};

struct hinic3_api_cmd_chain_attr {
	struct hinic3_hwdev		*hwdev;
	enum hinic3_api_cmd_chain_type	chain_type;

	u32				num_cells;
	u16				rsp_size;
	u16				cell_size;
};

struct hinic3_api_cmd_chain {
	struct hinic3_hwdev		*hwdev;
	enum hinic3_api_cmd_chain_type	chain_type;

	u32				num_cells;
	u16				cell_size;
	u16				rsp_size;
	u32				rsvd1;

	/* HW members is 24 bit format */
	u32				prod_idx;
	u32				cons_idx;

	struct semaphore		sem;
	/* Async cmd can not be scheduling */
	spinlock_t			async_lock;

	dma_addr_t			wb_status_paddr;
	struct hinic3_api_cmd_status	*wb_status;

	dma_addr_t			head_cell_paddr;
	struct hinic3_api_cmd_cell	*head_node;

	struct hinic3_api_cmd_cell_ctxt	*cell_ctxt;
	struct hinic3_api_cmd_cell	*curr_node;

	struct hinic3_dma_addr_align	cells_addr;

	u8				*cell_vaddr_base;
	u64				cell_paddr_base;
	u8				*rsp_vaddr_base;
	u64				rsp_paddr_base;
	u8				*buf_vaddr_base;
	u64				buf_paddr_base;
	u64				cell_size_align;
	u64				rsp_size_align;
	u64				buf_size_align;

	u64				rsvd2;
};

int hinic3_api_cmd_write(struct hinic3_api_cmd_chain *chain, u8 node_id,
			 const void *cmd, u16 size);

int hinic3_api_cmd_read(struct hinic3_api_cmd_chain *chain, u8 node_id,
			const void *cmd, u16 size, void *ack, u16 ack_size);

int hinic3_api_cmd_init(struct hinic3_hwdev *hwdev,
			struct hinic3_api_cmd_chain **chain);

void hinic3_api_cmd_free(const struct hinic3_hwdev *hwdev, struct hinic3_api_cmd_chain **chain);

#endif
