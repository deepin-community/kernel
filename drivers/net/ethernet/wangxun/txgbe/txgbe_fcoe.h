/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2015 - 2022 Beijing WangXun Technology Co., Ltd. */

#ifndef _TXGBE_FCOE_H_
#define _TXGBE_FCOE_H_

#if IS_ENABLED(CONFIG_FCOE)

#include <scsi/fc/fc_fs.h>
#include <scsi/fc/fc_fcoe.h>

/* shift bits within STAT fo FCSTAT */
#define TXGBE_RXD_FCSTAT_SHIFT  4

/* ddp user buffer */
#define TXGBE_BUFFCNT_MAX       256     /* 8 bits bufcnt */
#define TXGBE_FCPTR_ALIGN       16
#define TXGBE_FCPTR_MAX         (TXGBE_BUFFCNT_MAX * sizeof(dma_addr_t))
#define TXGBE_FCBUFF_4KB        0x0
#define TXGBE_FCBUFF_8KB        0x1
#define TXGBE_FCBUFF_16KB       0x2
#define TXGBE_FCBUFF_64KB       0x3
#define TXGBE_FCBUFF_MAX        65536   /* 64KB max */
#define TXGBE_FCBUFF_MIN        4096    /* 4KB min */
#define TXGBE_FCOE_DDP_MAX      512     /* 9 bits xid */

/* Default user priority to use for FCoE */
#define TXGBE_FCOE_DEFUP        3

/* fcerr */
#define TXGBE_FCERR_BADCRC      0x00100000
#define TXGBE_FCERR_EOFSOF      0x00200000
#define TXGBE_FCERR_NOFIRST     0x00300000
#define TXGBE_FCERR_OOOSEQ      0x00400000
#define TXGBE_FCERR_NODMA       0x00500000
#define TXGBE_FCERR_PKTLOST     0x00600000

/* FCoE DDP for target mode */
#define __TXGBE_FCOE_TARGET     1

struct txgbe_fcoe_ddp {
	int len;
	u32 err;
	unsigned int sgc;
	struct scatterlist *sgl;
	dma_addr_t udp;
	u64 *udl;
	struct dma_pool *pool;
};

/* per cpu variables */
struct txgbe_fcoe_ddp_pool {
	struct dma_pool *pool;
	u64 noddp;
	u64 noddp_ext_buff;
};

struct txgbe_fcoe {
	struct txgbe_fcoe_ddp_pool __percpu *ddp_pool;
	atomic_t refcnt;
	/* spinlock for fcoe */
	spinlock_t lock;
	struct txgbe_fcoe_ddp ddp[TXGBE_FCOE_DDP_MAX];
	void *extra_ddp_buffer;
	dma_addr_t extra_ddp_buffer_dma;
	unsigned long mode;
	u8 up;
	u8 up_set;
};
#endif /* CONFIG_FCOE */

#endif /* _TXGBE_FCOE_H */
