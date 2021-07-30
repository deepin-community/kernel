/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HYGON Platform Security Processor (PSP) driver interface
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 */

#ifndef __PSP_HYGON_H__
#define __PSP_HYGON_H__

#include <linux/types.h>
#include <linux/fs.h>

/*****************************************************************************/
/***************************** CSV interface *********************************/
/*****************************************************************************/

#define CSV_FW_MAX_SIZE		0x80000	/* 512KB */

/**
 * Guest/platform management commands for CSV
 */
enum csv_cmd {
	CSV_CMD_RING_BUFFER		= 0x00F,
	CSV_CMD_HGSC_CERT_IMPORT        = 0x300,
	CSV_CMD_MAX,
};

/**
 * CSV communication state
 */
enum csv_comm_state {
	CSV_COMM_MAILBOX_ON		= 0x0,
	CSV_COMM_RINGBUFFER_ON		= 0x1,

	CSV_COMM_MAX
};

/**
 * Ring Buffer Mode regions:
 *   There are 4 regions and every region is a 4K area that must be 4K aligned.
 *   To accomplish this allocate an amount that is the size of area and the
 *   required alignment.
 *   The aligned address will be calculated from the returned address.
 */
#define CSV_RING_BUFFER_SIZE		(32 * 1024)
#define CSV_RING_BUFFER_ALIGN		(4 * 1024)
#define CSV_RING_BUFFER_LEN		(CSV_RING_BUFFER_SIZE + CSV_RING_BUFFER_ALIGN)
#define CSV_RING_BUFFER_ESIZE		16

/**
 * struct csv_data_hgsc_cert_import - HGSC_CERT_IMPORT command parameters
 *
 * @hgscsk_cert_address: HGSCSK certificate chain
 * @hgscsk_cert_len: len of HGSCSK certificate
 * @hgsc_cert_address: HGSC certificate chain
 * @hgsc_cert_len: len of HGSC certificate
 */
struct csv_data_hgsc_cert_import {
	u64 hgscsk_cert_address;        /* In */
	u32 hgscsk_cert_len;            /* In */
	u32 reserved;                   /* In */
	u64 hgsc_cert_address;          /* In */
	u32 hgsc_cert_len;              /* In */
} __packed;

#define CSV_COMMAND_PRIORITY_HIGH	0
#define CSV_COMMAND_PRIORITY_LOW	1
#define CSV_COMMAND_PRIORITY_NUM	2

struct csv_cmdptr_entry {
	u16 cmd_id;
	u16 cmd_flags;
	u32 sw_data;
	u64 cmd_buf_ptr;
} __packed;

struct csv_statval_entry {
	u16 status;
	u16 reserved0;
	u32 reserved1;
	u64 reserved2;
} __packed;

struct csv_queue {
	u32 head;
	u32 tail;
	u32 mask; /* mask = (size - 1), inicates the elements max count */
	u32 esize; /* size of an element */
	u64 data;
	u64 data_align;
} __packed;

struct csv_ringbuffer_queue {
	struct csv_queue cmd_ptr;
	struct csv_queue stat_val;
} __packed;

/**
 * struct csv_data_ring_buffer - RING_BUFFER command parameters
 *
 * @queue_lo_cmdptr_address: physical address of the region to be used for
 *                           low priority queue's CmdPtr ring buffer
 * @queue_lo_statval_address: physical address of the region to be used for
 *                            low priority queue's StatVal ring buffer
 * @queue_hi_cmdptr_address: physical address of the region to be used for
 *                           high priority queue's CmdPtr ring buffer
 * @queue_hi_statval_address: physical address of the region to be used for
 *                            high priority queue's StatVal ring buffer
 * @queue_lo_size: size of the low priority queue in 4K pages. Must be 1
 * @queue_hi_size: size of the high priority queue in 4K pages. Must be 1
 * @queue_lo_threshold: queue(low) size, below which an interrupt may be generated
 * @queue_hi_threshold: queue(high) size, below which an interrupt may be generated
 * @int_on_empty: unconditionally interrupt when both queues are found empty
 */
struct csv_data_ring_buffer {
	u64 queue_lo_cmdptr_address;	/* In */
	u64 queue_lo_statval_address;	/* In */
	u64 queue_hi_cmdptr_address;	/* In */
	u64 queue_hi_statval_address;	/* In */
	u8 queue_lo_size;		/* In */
	u8 queue_hi_size;		/* In */
	u16 queue_lo_threshold;		/* In */
	u16 queue_hi_threshold;		/* In */
	u16 int_on_empty;		/* In */
} __packed;

#ifdef CONFIG_CRYPTO_DEV_SP_PSP

int csv_ring_buffer_queue_init(void);
int csv_ring_buffer_queue_free(void);
int csv_fill_cmd_queue(int prio, int cmd, void *data, uint16_t flags);
int csv_check_stat_queue_status(int *psp_ret);

/**
 * csv_issue_ringbuf_cmds_external_user - issue CSV commands into a ring
 * buffer.
 */
int csv_issue_ringbuf_cmds_external_user(struct file *filep, int *psp_ret);

#else	/* !CONFIG_CRYPTO_DEV_SP_PSP */

static inline int csv_ring_buffer_queue_init(void) { return -ENODEV; }
static inline int csv_ring_buffer_queue_free(void) { return -ENODEV; }
static inline
int csv_fill_cmd_queue(int prio, int cmd, void *data, uint16_t flags) { return -ENODEV; }
static inline int csv_check_stat_queue_status(int *psp_ret) { return -ENODEV; }
static inline int
csv_issue_ringbuf_cmds_external_user(struct file *filep, int *psp_ret) { return -ENODEV; }

#endif	/* CONFIG_CRYPTO_DEV_SP_PSP */

#endif	/* __PSP_HYGON_H__ */
