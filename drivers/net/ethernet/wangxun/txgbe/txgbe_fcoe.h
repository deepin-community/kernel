/*
 * WangXun 10 Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * based on ixgbe_fcoe.h, Copyright(c) 1999 - 2017 Intel Corporation.
 * Contact Information:
 * Linux NICS <linux.nics@intel.com>
 * e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 */

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
