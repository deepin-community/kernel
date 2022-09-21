/*
 * JM7200 GPU driver
 *
 * Copyright (c) 2018 ChangSha JingJiaMicro Electronics Co., Ltd.
 *
 * Author:
 *      rfshen <jjwgpu@jingjiamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef MWV206_CONFIG_H
#define MWV206_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif
#include <linux/version.h>
#include "mwv206.h"







#define MWV206_PCIE_RW_DMA

#define MWV206_MEM2D_IDX     0
#define MWV206_MEM3D_IDX     1


#define MWV206_KERNEL_MAPPED_FBSIZE (2ULL << 30)




#if defined(__loongarch__) && (LINUX_VERSION_CODE == KERNEL_VERSION(4, 4, 131))
#define MWV206_MAX_DMA_SIZE                (1024*256)
#else
#define MWV206_MAX_DMA_SIZE                (1024*1024*4)
#define MWV206_RING_USE_DMABUF
#endif
#define MWV206_MIN_DMA_READ_BYTES           (128)
#define MWV206_MEMACCESS_CHECK_SLICE        (128)

#define MWV206_MAX_DMA_LIST_NODE  (4096)
#define MWV206_OUTOFBOUND_VTXBAR_ID   16
#define MWV206_AXI_PCIE_VTX_START_ADDR (0x09000000)
#define MWV206_AXI_PCIE_VTX_END_ADDR   (0x0AFFFFFF)

#define MWV206_OUTOFBOUND_CMDBAR_ID   17





#define MWV206_2D_SENDCMDMODE V206API040

#define MWV206_3D_SENDCMDMODE V206API040

#define MWV206_REFCOLOCK_FREQ 50

#define MWV206_MAX2DCMDLEN 128

#define MWV206_MAXCMDLEN 0x40000

#define MWV206CMDADDRLEN (0x100 / 4)

#define MWV206_MAX_DEVICEBUSNO 256

#define MWV206CMDTIMEOUT ((GLJ_TICK)FUNC206LXDEV098() * 1)

#define MWV206_GET_PCI_BAR_LEN pci_resource_len
#define MWV206_GET_PCI_BAR_STARTADDR pci_resource_start


#define MAX_VIDEO_SCREEN_NUMBER 4

typedef enum {
	V206CONFIG015 = 0,
	V206CONFIG016 = 1,
} e_MWV206DDRConfType;

#ifdef __cplusplus
}
#endif
#endif