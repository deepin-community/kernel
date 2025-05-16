// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/module.h>

#include "ossl_knl.h"
#include "hinic3_common.h"
#include "hinic3_sm_lt.h"
#include "hinic3_hw.h"
#include "hinic3_hwdev.h"
#include "hinic3_api_cmd.h"
#include "hinic3_mgmt.h"

#define ACK 1
#define NOACK 0

#define LT_LOAD16_API_SIZE (16 + 4)
#define LT_STORE16_API_SIZE (32 + 4)

#ifndef HTONL
#define HTONL(x) \
	((((x) & 0x000000ff) << 24) \
	| (((x) & 0x0000ff00) << 8) \
	| (((x) & 0x00ff0000) >> 8) \
	| (((x) & 0xff000000) >> 24))
#endif

static inline void sm_lt_build_head(union sml_lt_req_head *head,
				    u8 instance_id,
				    u8 op_id, u8 ack,
				    u8 offset, u8 num)
{
	head->value = 0;
	head->bs.instance = instance_id;
	head->bs.op_id = op_id;
	head->bs.ack = ack;
	head->bs.num = num;
	head->bs.abuf_flg = 0;
	head->bs.bc = 1;
	head->bs.offset = offset;
	head->value = HTONL((head->value));
}

static inline void sm_lt_load_build_req(struct sml_lt_load_req *req,
					u8 instance_id,
					u8 op_id, u8 ack,
					u32 lt_index,
					u8 offset, u8 num)
{
	sm_lt_build_head(&req->head, instance_id, op_id, ack, offset, num);
	req->extra = 0;
	req->index = lt_index;
	req->index = HTONL(req->index);
	req->pad0 = 0;
	req->pad1 = 0;
}

static void sml_lt_store_data(u32 *dst, const u32 *src, u8 num)
{
	switch (num) {
	case SM_LT_NUM_2:
		*(dst + SM_LT_OFFSET_11) = *(src + SM_LT_OFFSET_11);
		*(dst + SM_LT_OFFSET_10) = *(src + SM_LT_OFFSET_10);
		*(dst + SM_LT_OFFSET_9)  = *(src + SM_LT_OFFSET_9);
		*(dst + SM_LT_OFFSET_8)  = *(src + SM_LT_OFFSET_8);
		fallthrough;
	case SM_LT_NUM_1:
		*(dst + SM_LT_OFFSET_7) = *(src + SM_LT_OFFSET_7);
		*(dst + SM_LT_OFFSET_6) = *(src + SM_LT_OFFSET_6);
		*(dst + SM_LT_OFFSET_5) = *(src + SM_LT_OFFSET_5);
		*(dst + SM_LT_OFFSET_4) = *(src + SM_LT_OFFSET_4);
		fallthrough;
	case SM_LT_NUM_0:
		*(dst + SM_LT_OFFSET_3) = *(src + SM_LT_OFFSET_3);
		*(dst + SM_LT_OFFSET_2) = *(src + SM_LT_OFFSET_2);
		*(dst + SM_LT_OFFSET_1) = *(src + SM_LT_OFFSET_1);
		*dst = *src;
		break;
	default:
		break;
	}
}

static inline void sm_lt_store_build_req(struct sml_lt_store_req *req,
					 u8 instance_id,
					 u8 op_id, u8 ack,
					 u32 lt_index,
					 u8 offset,
					 u8 num,
					 u16 byte_enb3,
					 u16 byte_enb2,
					 u16 byte_enb1,
					 u8 *data)
{
	sm_lt_build_head(&req->head, instance_id, op_id, ack, offset, num);
	req->index     = lt_index;
	req->index     = HTONL(req->index);
	req->extra = 0;
	req->byte_enb[0] = (u32)(byte_enb3);
	req->byte_enb[0] = HTONL(req->byte_enb[0]);
	req->byte_enb[1] = HTONL((((u32)byte_enb2) << 16) | byte_enb1);
	sml_lt_store_data((u32 *)req->write_data, (u32 *)(void *)data, num);
}

int hinic3_dbg_lt_rd_16byte(void *hwdev, u8 dest, u8 instance,
			    u32 lt_index, u8 *data)
{
	struct sml_lt_load_req req;
	int ret;

	if (!hwdev)
		return -EFAULT;

	if (!COMM_SUPPORT_API_CHAIN((struct hinic3_hwdev *)hwdev))
		return -EPERM;

	sm_lt_load_build_req(&req, instance, SM_LT_LOAD, ACK, lt_index, 0, 0);

	ret = hinic3_api_cmd_read_ack(hwdev, dest, (u8 *)(&req),
				      LT_LOAD16_API_SIZE, (void *)data, 0x10);
	if (ret) {
		sdk_err(((struct hinic3_hwdev *)hwdev)->dev_hdl,
			"Read linear table 16byte fail, err: %d\n", ret);
		return -EFAULT;
	}

	return 0;
}

int hinic3_dbg_lt_wr_16byte_mask(void *hwdev, u8 dest, u8 instance,
				 u32 lt_index, u8 *data, u16 mask)
{
	struct sml_lt_store_req req;
	int ret;

	if (!hwdev || !data)
		return -EFAULT;

	if (!COMM_SUPPORT_API_CHAIN((struct hinic3_hwdev *)hwdev))
		return -EPERM;

	sm_lt_store_build_req(&req, instance, SM_LT_STORE, NOACK, lt_index,
			      0, 0, 0, 0, mask, data);

	ret = hinic3_api_cmd_write_nack(hwdev, dest, &req, LT_STORE16_API_SIZE);
	if (ret) {
		sdk_err(((struct hinic3_hwdev *)hwdev)->dev_hdl,
			"Write linear table 16byte fail, err: %d\n", ret);
		return -EFAULT;
	}

	return 0;
}

