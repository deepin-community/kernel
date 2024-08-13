/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_ADM_INFO_H
#define SSS_ADM_INFO_H

#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/completion.h>

#include "sss_hw_common.h"

enum sss_adm_msg_type {
	/* write to mgmt cpu command with completion  */
	SSS_ADM_MSG_WRITE_TO_MGMT_MODULE		= 2,

	/* multi read command with completion notification */
	SSS_ADM_MSG_MULTI_READ				= 3,

	/* write command without completion notification */
	SSS_ADM_MSG_POLL_WRITE				= 4,

	/* read command without completion notification */
	SSS_ADM_MSG_POLL_READ				= 5,

	/* read from mgmt cpu command with completion */
	SSS_ADM_MSG_WRITE_ASYNC_TO_MGMT_MODULE	= 6,

	SSS_ADM_MSG_MAX,
};

struct sss_adm_msg_state {
	u64 head;
	u32 desc_buf;
	u32 elem_hi;
	u32 elem_lo;
	u32 rsvd0;
	u64 rsvd1;
};

/* HW struct */
struct sss_adm_msg_elem {
	u64 control;

	u64 next_elem_paddr;

	u64 desc;

	/* HW struct */
	union {
		struct {
			u64 hw_msg_paddr;
		} write;

		struct {
			u64 hw_wb_reply_paddr;
			u64 hw_msg_paddr;
		} read;
	};
};

struct sss_adm_msg_reply_fmt {
	u64		head;
	u64		reply;
};

struct sss_adm_msg_elem_ctx {
	struct sss_adm_msg_elem	*elem_vaddr;

	void				*adm_msg_vaddr;

	struct sss_adm_msg_reply_fmt	*reply_fmt;

	struct completion	done;
	int					state;

	u32		 store_pi;
	void	*hwdev;
};

struct sss_adm_msg {
	void	*hwdev;

	enum sss_adm_msg_type		msg_type;

	u32				elem_num;

	u16				elem_size;
	u16				reply_size;

	u32				pi;
	u32				ci;

	struct semaphore				sem;
	spinlock_t			async_lock; /* protect adm msg async and sync */
	dma_addr_t						wb_state_paddr;

	dma_addr_t						head_elem_paddr;

	struct sss_adm_msg_state		*wb_state;

	struct sss_adm_msg_elem		*head_node;

	struct sss_adm_msg_elem_ctx	*elem_ctx;
	struct sss_adm_msg_elem		*now_node;

	struct sss_dma_addr_align		elem_addr;

	u8				*elem_vaddr_base;
	u8				*reply_vaddr_base;
	u8				*buf_vaddr_base;

	u64				elem_paddr_base;
	u64				reply_paddr_base;
	u64				buf_paddr_base;
	u64				elem_size_align;
	u64				reply_size_align;
	u64				buf_size_align;
};
#endif
