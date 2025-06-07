/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _WST_SE_ECHIP_DRIVER_H
#define _WST_SE_ECHIP_DRIVER_H
#include "wst_se_ktrans.h"

#define CRYNAME "wst-se"

#define SWBUFNUM 512
#define SWCHANNELNUM 2
#define CTL_DMABUFNUM 512

#define DMA_MASK 0xffffffff
#define SE_DMA_CONTROL_RESET 0x80000000
#define SE_DMA_CONTROL_SET 0x00000000

#define SE_BASEERR 1000

#define SE_MAX_SEND_LEN (3 * 2048)
#define SE_FILL_LEN 48
#define SE_BD_LENGTH sizeof(struct tagSEBasicBD)
#define SE_BDQUEUE_LEN SWBUFNUM
#define DMA0_CTRL_CHANNEL_ENABLE 1
#define DMA0_CTRL_CHANNEL_DISABLE 0xfffffffe
#define DMA1_CTRL_CHANNEL_ENABLE 2
#define DMA1_CTRL_CHANNEL_DISABLE 0xfffffffd

#define SE_REG_RESET 0
#define SE_REG_STS 0x08
#define SE_INT_CTRL 0x10
#define SE_REG_MSK 0x18
#define SE_INT_CLR 0x20

#define SE_LREG_BQBA0 0x100
#define SE_HREG_BQBA0 0x108
#define SE_REG_BQS0 0x110
#define SE_REG_BQWP0 0x118
#define SE_REG_RQRP0 0x120

#define SE_LREG_BQBA1 0x128
#define SE_HREG_BQBA1 0x130
#define SE_REG_BQS1 0x138
#define SE_REG_BQWP1 0x140
#define SE_REG_RQRP1 0x148

#define SE_WRITE_REG1 0x200
#define SE_WRITE_REG2 0x208
#define SE_WRITE_REG3 0x210
#define SE_LOWREG_STS 0x240
#define SE_LOWINT_CLEAR 0x248
#define SE_LOWREG_INQ 0x1600
#define SE_LOWREG_SR 0x1608
#define INPUT_VALID 0x4
#define OUTPUT_VALID 0x8
#define SE_LOWINT_CLR 0x228
#define INT_STAT_DMA0_PACK_DONE 1
#define INT_STAT_DMA1_PACK_DONE 2
#define INT_STAT_DMA_MASK (INT_STAT_DMA0_PACK_DONE | INT_STAT_DMA1_PACK_DONE)

struct tagSEdrvctl {
	unsigned long ulMemBase;
	struct device *pdev;
	struct device dev;
	unsigned int ulCurrBdReadPtr[SWCHANNELNUM];
	unsigned int ulCurrBdWritePtr[SWCHANNELNUM];
	unsigned int ulCurrReadPtr[SWCHANNELNUM];
	unsigned int ulCurrWritePtr[SWCHANNELNUM];
	PSECallBackfn pcallback[SWCHANNELNUM][SWBUFNUM];
	void *pParma[SWCHANNELNUM][SWBUFNUM];
	struct completion *stsemphore[SWCHANNELNUM][SWBUFNUM];
	int ikernel[SWCHANNELNUM][SWBUFNUM];
	unsigned long ulBDMemBasePhy[SWCHANNELNUM];
	unsigned long ulBDMemBase[SWCHANNELNUM];
	unsigned short *pusOutlen[SWCHANNELNUM][SWBUFNUM];
	unsigned short usInlen[SWCHANNELNUM][SWBUFNUM];
	unsigned long *ulOutputPtr[SWCHANNELNUM][SWBUFNUM];
	unsigned long *ulInputPtr[SWCHANNELNUM][SWBUFNUM];
	unsigned char *pucRetCode[SWCHANNELNUM][SWBUFNUM];
	rwlock_t mr_lock;
	rwlock_t mr_lowlock;
	spinlock_t readlock;
	struct semaphore sema;
	int iIrq;
	int ilowIrq;
};

struct tagSEBasicBD {
	unsigned int ucOpCode : 4, ucFlag : 4, ucRetCode : 8, ucInCtxLength : 8,
		ucOutCtxLength : 8;
	unsigned int usInputLength : 16, usOutputLength : 16;
	unsigned int ulCtxLPtr;
	unsigned int ulCtxHPtr;
	unsigned int ulInputLPtr;
	unsigned int ulInputHPtr;
	unsigned int ulOutputLPtr;
	unsigned int ulOutputHPtr;

};

#define SW_CONT_BUF_SIZE 0x100
#define DMA_BUFSIZE 0x1000

#define PAGE_NUM (DMA_BUFSIZE / PAGE_SIZE + 1)

struct _SW_GET_STAT {
	unsigned long TXD;
	unsigned long RXD;
};

DEFINE_SPINLOCK(g_writelock);

#define HandleRead32(handle, addr, pData)                             \
	do {    /* MB */                                              \
		smp_mb();                                             \
		*(pData) = readl((void *)(handle->ulMemBase + addr)); \
	} while (0)

#define HandleWrite32(handle, addr, value)                         \
	do {                                                       \
		writel(value, (void *)(handle->ulMemBase + addr)); \
		/* MB */                                           \
		smp_mb();                                          \
	} while (0)

#define HandleRead64(handle, addr, pData)                             \
	do {    /* MB */                                              \
		smp_mb();                                             \
		*(pData) = readq((void *)(handle->ulMemBase + addr)); \
	} while (0)

#define HandleWrite64(handle, addr, value)                         \
	do {                                                       \
		writeq(value, (void *)(handle->ulMemBase + addr)); \
		/* MB */                                           \
		smp_mb();                                          \
	} while (0)

#define SPRINTF sprintf

#define HIULONG(w) \
	((unsigned int)((((unsigned long long)w) >> 32) & 0x00000000ffffffff))
#define LOULONG(w) ((unsigned int)((unsigned long long)w) & 0x00000000ffffffff)

#define HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))
#define LOBYTE(w) ((unsigned char)(w))

struct ST_SEND_PACKAGE {
	struct list_head list;
	struct tagSEdrvctl *pdrvctl;
	unsigned char *pInPtr;
	unsigned short usInlen;
	unsigned char *pOutPtr;
	unsigned short *pusOutlen;
	int iChannel;
	unsigned char ucFlag;
	unsigned char ucOpCode;
	unsigned char *pucRetCode;
	PSECallBackfn pcallback;
	void *pParma;
	int iKernel;
	struct completion *mycomplete;
	unsigned long ulendtime;
};

struct ST_INT_MESSAGE {
	struct list_head list;
	PSECallBackfn pcallback;
	void *pParma;
};
#endif
