/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2012 IBM Corporation
 *
 * Copyright 2023 Loongson Technology, Inc.
 * Yinggang Gu <guyinggang@loongson.cn>
 *
 * Device driver for Loongson SE module.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 *
 */
#ifndef __LOONGSON_SE_H__
#define __LOONGSON_SE_H__

#define SE_MAILBOX_S			0x0
#define SE_MAILBOX_L			0x20
#define SE_S2LINT_STAT			0x88
#define SE_S2LINT_EN			0x8c
#define SE_S2LINT_SET			0x90
#define SE_S2LINT_CL			0x94
#define SE_L2SINT_STAT			0x98
#define SE_L2SINT_EN			0x9c
#define SE_L2SINT_SET			0xa0
#define SE_L2SINT_CL			0xa4

/* INT bit definition */
#define SE_INT_SETUP			BIT(0)
#define SE_INT_SM2				BIT(0)
#define SE_INT_SM3				BIT(0)
#define SE_INT_SM4				BIT(0)
#define SE_INT_RNG				BIT(0)
#define SE_INT_TPM				BIT(5)
#define SE_INT_ALL				0xffffffff

#define SE_CMD_START			0x0
#define SE_CMD_STOP				0x1
#define SE_CMD_GETVER			0x2
#define SE_CMD_SETBUF			0x3
#define SE_CMD_SETMSG			0x4

#define SE_CMD_RNG				0x100

#define SE_CMD_SM2_SIGN			0x200
#define SE_CMD_SM2_VSIGN		0x201

#define SE_CMD_SM3_DIGEST		0x300
#define SE_CMD_SM3_UPDATE		0x301
#define SE_CMD_SM3_FINISH		0x302

#define SE_CMD_SM4_ECB_ENCRY		0x400
#define SE_CMD_SM4_ECB_DECRY		0x401
#define SE_CMD_SM4_CBC_ENCRY		0x402
#define SE_CMD_SM4_CBC_DECRY		0x403
#define SE_CMD_SM4_CTR			0x404

#define SE_CMD_TPM				0x500
#define SE_CMD_ZUC_INIT_READ		0x600
#define SE_CMD_ZUC_READ			0x601

#define SE_CMD_SDF				0x700

#define SE_CH_MAX			32

#define SE_CH_RNG			1
#define SE_CH_SM2			2
#define SE_CH_SM3			3
#define SE_CH_SM4			4
#define SE_CH_TPM			5
#define SE_CH_ZUC			6
#define SE_CH_SDF			7

struct se_msg {
	u32 cmd;
	u32 data_off;
	u32 data_len;
	u32 info[5];
};

struct se_cmd {
	u32 cmd;
	u32 info[7];
};

struct se_res {
	u32 cmd;
	u32 cmd_ret;
	u32 info[6];
};

struct se_mailbox_data {
	u32 int_bit;
	union {
		u32 mailbox[8];
		struct se_cmd gcmd;
		struct se_res res;
	} u;
};

struct lsse_ch {
	u32 id;
	u32 int_bit;
	struct loongson_se *se;
	void *priv;
	spinlock_t ch_lock;
	void *smsg;
	void *rmsg;
	int msg_size;
	void *data_buffer;
	dma_addr_t data_addr;
	int data_size;

	void (*complete)(struct lsse_ch *se_ch);
};

struct loongson_se {
	struct device *dev;
	void __iomem *base;
	u32 version;
	u32 ch_status;
	spinlock_t cmd_lock;
	spinlock_t dev_lock;

	/* Interaction memory */
	void *mem_base;
	dma_addr_t mem_addr;
	unsigned long *mem_map;
	int mem_map_size;
	void *smsg;
	void *rmsg;

	/* Synchronous CMD */
	struct completion cmd_completion;

	/* Virtual Channel */
	struct lsse_ch chs[SE_CH_MAX];
};

struct lsse_ch *se_init_ch(int id, int data_size, int msg_size, void *priv,
		void (*complete)(struct lsse_ch *se_ch));
void se_deinit_ch(struct lsse_ch *ch);
int se_send_ch_requeset(struct lsse_ch *ch);

#endif
