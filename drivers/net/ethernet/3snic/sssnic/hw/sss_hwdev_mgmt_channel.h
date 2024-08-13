/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWDEV_MGMT_CHANNEL_H
#define SSS_HWDEV_MGMT_CHANNEL_H

#include "sss_hwdev.h"

#define SSS_STACK_DATA_LEN 1024
#define SSS_XREGS_NUM 31
#define SSS_MPU_LASTWORD_SIZE 1024

struct sss_watchdog_info {
	struct sss_mgmt_msg_head head;

	u32 cur_time_h;
	u32 cur_time_l;
	u32 task_id;
	u32 rsvd;

	u64 pc;
	u64 elr;
	u64 spsr;
	u64 far;
	u64 esr;
	u64 xzr;
	u64 x30;
	u64 x29;
	u64 x28;
	u64 x27;
	u64 x26;
	u64 x25;
	u64 x24;
	u64 x23;
	u64 x22;
	u64 x21;
	u64 x20;
	u64 x19;
	u64 x18;
	u64 x17;
	u64 x16;
	u64 x15;
	u64 x14;
	u64 x13;
	u64 x12;
	u64 x11;
	u64 x10;
	u64 x09;
	u64 x08;
	u64 x07;
	u64 x06;
	u64 x05;
	u64 x04;
	u64 x03;
	u64 x02;
	u64 x01;
	u64 x00;

	u64 stack_top;
	u64 stack_bottom;
	u64 sp;
	u32 cur_used;
	u32 peak_used;
	u32 is_overflow;

	u32 stack_actlen;
	u8 stack_data[SSS_STACK_DATA_LEN];
};

struct sss_cpu_tick {
	u32 tick_cnt_h; /* The cycle count higher 32 bits  */
	u32 tick_cnt_l; /* The cycle count lower 32 bits */
};

struct sss_ax_exc_reg_info {
	u64 ttbr0;
	u64 ttbr1;
	u64 tcr;
	u64 mair;
	u64 sctlr;
	u64 vbar;
	u64 current_el;
	u64 sp;
	u64 elr;
	u64 spsr;
	u64 far_r;
	u64 esr;
	u64 xzr;
	u64 xregs[SSS_XREGS_NUM]; /* 0~30: x30~x0 */
};

struct sss_exc_info {
	char os_ver[48]; /* OS version */
	char app_ver[64]; /* Product version */
	u32 exc_cause; /* Cause of exception */
	u32 thread_type; /* The thread type before the exception */
	u32 thread_id; /* Thread PID before exception */
	u16 byte_order; /* Byte order */
	u16 cpu_type; /* CPU type */
	u32 cpu_id; /* CPU ID */
	struct sss_cpu_tick cpu_tick; /* CPU Tick */
	u32 nest_cnt; /* The exception nested count */
	u32 fatal_errno; /* Fatal error code */
	u64 uw_sp; /* The stack pointer before the exception */
	u64 stack_bottom; /* Bottom of the stack before the exception */

	/* The in-core register context information,*/
	/* 82\57 must be at 152 bytes; if it has changed, */
	/* the OS_EXC_REGINFO_OFFSET macro in sre_platform.eh must be updated */
	struct sss_ax_exc_reg_info reg_info;
};

struct sss_lastword_info {
	struct sss_mgmt_msg_head head;
	struct sss_exc_info stack_info;

	/* Stack details, Actual stack size(<=1024) */
	u32 stack_actlen;

	/* More than 1024, it will be truncated */
	u8 stack_data[SSS_MPU_LASTWORD_SIZE];
};

int sss_init_mgmt_channel(struct sss_hwdev *hwdev);
void sss_deinit_mgmt_channel(struct sss_hwdev *hwdev);

#endif
