/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_HW_MT_H
#define HINIC3_HW_MT_H

#include "hinic3_lld.h"

struct sm_in_st {
	int node;
	int id;
	int instance;
};

struct sm_out_st {
	u64 val1;
	u64 val2;
};

struct up_log_msg_st {
	u32 rd_len;
	u32 addr;
};

struct csr_write_st {
	u32 rd_len;
	u32 addr;
	u8 *data;
};

int get_func_type(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
		  void *buf_out, u32 *out_size);

int get_func_id(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
		void *buf_out, u32 *out_size);

int get_hw_driver_stats(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			void *buf_out, u32 *out_size);

int clear_hw_driver_stats(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			  void *buf_out, u32 *out_size);

int get_self_test_result(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			 void *buf_out, u32 *out_size);

int get_chip_faults_stats(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			  void *buf_out, u32 *out_size);

#endif
