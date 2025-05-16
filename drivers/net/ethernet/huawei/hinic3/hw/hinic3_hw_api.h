/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_HW_API_H
#define HINIC3_HW_API_H

#include <linux/types.h>

#define CHIPIF_ACK 1
#define CHIPIF_NOACK 0

#define CHIPIF_SM_CTR_OP_READ 0x2
#define CHIPIF_SM_CTR_OP_READ_CLEAR 0x6

#define BIT_32 32

/* request head */
union chipif_sml_ctr_req_head {
	struct {
		u32  pad:15;
		u32  ack:1;
		u32  op_id:5;
		u32  instance:6;
		u32  src:5;
	} bs;

	u32 value;
};

/* counter read request struct */
struct chipif_sml_ctr_rd_req {
	u32 extra;
	union chipif_sml_ctr_req_head head;
	u32 ctr_id;
	u32 initial;
	u32 pad;
};

struct hinic3_csr_request_api_data {
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

/* counter read response union */
union ctr_rd_rsp {
	struct {
		u32 value1:16;
		u32 pad0:16;
		u32 pad1[3];
	} bs_ss16_rsp;

	struct {
		u32 value1;
		u32 pad[3];
	} bs_ss32_rsp;

	struct {
		u32 value1:20;
		u32 pad0:12;
		u32 value2:12;
		u32 pad1:20;
		u32 pad2[2];
	} bs_sp_rsp;

	struct {
		u32 value1;
		u32 value2;
		u32 pad[2];
	} bs_bs64_rsp;

	struct {
		u32 val1_h;
		u32 val1_l;
		u32 val2_h;
		u32 val2_l;
	} bs_bp64_rsp;
};

enum HINIC3_CSR_API_DATA_OPERATION_ID {
	HINIC3_CSR_OPERATION_WRITE_CSR = 0x1E,
	HINIC3_CSR_OPERATION_READ_CSR = 0x1F
};

enum HINIC3_CSR_API_DATA_NEED_RESPONSE_DATA {
	HINIC3_CSR_NO_RESP_DATA = 0,
	HINIC3_CSR_NEED_RESP_DATA = 1
};

enum HINIC3_CSR_API_DATA_DATA_SIZE {
	HINIC3_CSR_DATA_SZ_32 = 0,
	HINIC3_CSR_DATA_SZ_64 = 1
};

#endif
