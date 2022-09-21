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
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include "gljos.h"
#include "mwv206memmgr.h"
#include "mwv206kdebug.h"
#include "mwv206dev.h"

#define MAC206HAL141(_s) kzalloc(_s, GFP_KERNEL)
#define MAC206HAL039(_p) kfree(_p)

#undef DEBUG_REFCNT

typedef struct tagSaveMemBlock {
	unsigned int  size;
	unsigned int  V206IOCTLCMD009;
	void *V206IOCTLCMD006;
} saveMemBlock;

typedef struct tagSaveMemMrg {
	unsigned int blockcnt;
	saveMemBlock pBlock[1];
} saveMemmgr;

typedef struct tagUserTable {
	struct tagUserTable *next;
	void         *userdata[8];
	int           userflag;
} UserTable;

#define MAC206HAL211(utable)  (((utable)->userflag & 0xff) == 0)
#define MAC206HAL212(utable)   (((utable)->userflag & 0xff) == 0xff)
#define MAC206HAL209(ut, userdata)  \
	do { \
		int i; \
		for (i = 0; i < 8; i++) { \
			if ((ut)->userdata[i] == 0) { \
				(ut)->userdata[i] = (userdata); \
				(ut)->userflag |= (1 << i); \
				break; \
			} \
		} \
	} while (0)
#define MAC206HAL210(ut, userdata)  \
	({ \
	 int i; \
	 for (i = 0; i < 8; i++) { \
	 if ((ut)->userdata[i] == (userdata)) { \
	 (ut)->userdata[i] = 0; \
	 (ut)->userflag &= ~(1 << i); \
	 break; \
	 } \
	 } \
	 (i < 8);\
	 })

typedef struct tagMemBlock {
	struct tagMemBlock     *next;
	struct tagMemBlock     *prev;
	unsigned int  size;
	unsigned int  addr;
	unsigned int  refcnt;
	UserTable     utable;
} MemBlock;

#define MAC206HAL145  0x5400ec44

#define MAC206HAL229(mgr_) (((mgr_) != 0) && (((mgrtype *)(mgr_))->objectid == MAC206HAL145))

typedef struct memmgr_struct {
	unsigned int objectid;
	int maxblockcount;
	int validhead;
	GLJ_LOCK     access;
	MemBlock head[1];
} mgrtype;

static void FUNC206HAL205(MemBlock *pBlock, int addr, int size);
static void FUNC206HAL209(MemBlock *pBlock);
static void FUNC206HAL208(MemBlock *pBlock);
static void FUNC206HAL210(MemBlock *pBlock);

static MemMgr FUNC206HAL204(int maxblockcount)
{
	mgrtype *handle;
	if (maxblockcount < 1) {
		maxblockcount = 1;
	}
	handle = (mgrtype *)MAC206HAL141(sizeof(mgrtype) + (maxblockcount - 1) * sizeof(MemBlock));
	memset(handle, 0, sizeof(mgrtype) + (maxblockcount - 1) * sizeof(MemBlock));
	handle->maxblockcount = maxblockcount;
	handle->objectid = MAC206HAL145;
	handle->access = FUNC206LXDEV118();
	return (MemMgr)handle;
}


static void FUNC206HAL199(MemMgr mgr)
{
	int i;
	mgrtype *pmgr = (mgrtype *)mgr;
	MemBlock *pBlock;
	if (!MAC206HAL229(mgr)) {
		return ;
	}
	FUNC206LXDEV116(pmgr->access);
	for (i = 0; i < pmgr->validhead; i++) {
		pBlock = pmgr->head[i].next;
		while (pBlock != NULL) {
			MemBlock *pTemp;
			pTemp = pBlock->next;
			FUNC206HAL210(pBlock);
			MAC206HAL039(pBlock);
			pBlock = pTemp;
		}
	}
	FUNC206LXDEV119(pmgr->access);
	MAC206HAL039(pmgr);
}

static int FUNC206HAL197(MemMgr mgr, unsigned int addr, int size)
{
	mgrtype *pmgr = (mgrtype *)mgr;
	if (!MAC206HAL229(mgr)) {
		return -2;
	}
	if (pmgr->validhead < pmgr->maxblockcount) {
		pmgr->head[pmgr->validhead].size   = size;
		pmgr->head[pmgr->validhead].addr   = addr;
		pmgr->head[pmgr->validhead].refcnt = 0;
		memset(&pmgr->head[pmgr->validhead].utable, 0, sizeof(UserTable));
		pmgr->validhead++;
		return 0;
	}
	return -1;
}

static void FUNC206HAL210(MemBlock *pBlock)
{
	UserTable *ut, *next;
	for (ut = pBlock->utable.next; ut; ut = next) {
		next = ut->next;
		FUNC206LXDEV120(ut);
	}
	memset(&pBlock->utable, 0, sizeof(UserTable));
}

static int FUNC206HAL198(MemBlock *pBlock, void *userdata)
{
	UserTable *ut = &pBlock->utable;

	if (likely(!MAC206HAL212(ut))) {
		goto FUNC206HAL033;
	}


	while (ut->next && MAC206HAL212(ut->next)) {
		ut = ut->next;
	}


	if (ut->next) {
		ut = ut->next;
		goto FUNC206HAL033;
	}


	ut = (UserTable *) FUNC206LXDEV010(sizeof(UserTable));
	if (!ut) {
		goto FUNC206HAL057;
	}

	memset(ut, 0, sizeof(UserTable));
	ut->next = pBlock->utable.next;
	pBlock->utable.next = ut;

FUNC206HAL033:
	MAC206HAL209(ut, userdata);
	pBlock->refcnt++;
	return 0;
FUNC206HAL057:
	return -1;
}

static int FUNC206HAL200(MemBlock *pBlock, void *userdata)
{
	UserTable *ut;

	for (ut = &pBlock->utable; ut != NULL; ut = ut->next) {
		if (!MAC206HAL211(ut) && MAC206HAL210(ut, userdata)) {
			pBlock->refcnt--;
			if (pBlock->refcnt <= 0) {
				FUNC206HAL210(pBlock);
				pBlock->size  &= 0x7fffffff;
				pBlock->refcnt = 0;
				FUNC206HAL208(pBlock);
				FUNC206HAL209(pBlock);
			}
			return 0;
		}
	}

	return -1;
}


static unsigned int FUNC206HAL206(MemMgr mgr, int size, int aligned, void *userdata)
{
	int i;
	mgrtype *hmgr;
	MemBlock *pBlock;
	if (!MAC206HAL229(mgr)) {
		return 0;
	}
	if (size == 0) {
		return 0;
	}
	if (aligned == 0 || ((aligned - 1) & aligned)) {
		return 0;
	}
	hmgr = (mgrtype *)mgr;
	FUNC206LXDEV116(hmgr->access);
	size = (size + 3) & (~3);
	for (i = 0; i < hmgr->validhead; i++) {
		pBlock = &hmgr->head[i];
		while (pBlock != NULL) {
			int needsize;
			needsize = size;
			if ((pBlock->addr & (aligned - 1)) != 0) {
				needsize += aligned - (pBlock->addr & (aligned - 1));
			}
			if (((pBlock->size & 0x80000000) == 0) &&
					((int)pBlock->size >= needsize)) {
				if (needsize > size) {
					FUNC206HAL205(pBlock, pBlock->addr + needsize - size, pBlock->size - (needsize - size));
					pBlock->size = needsize - size;
					pBlock = pBlock->next;
				}
				FUNC206HAL205(pBlock, pBlock->addr + size, pBlock->size - size);
				if (FUNC206HAL198(pBlock, userdata) == 0) {
					pBlock->size = size | 0x80000000;
					FUNC206LXDEV136(hmgr->access);
					return pBlock->addr;
				}
			}
			pBlock = pBlock->next;
		}
	}
	FUNC206LXDEV136(hmgr->access);
	return 0;
}


static int FUNC206HAL201(MemMgr mgr, unsigned int ptr, void *userdata)
{
	int i;
	mgrtype *hmgr;
	unsigned int addr;
	MemBlock *pBlock;
	if (!MAC206HAL229(mgr)) {
		return -1;
	}
	hmgr = (mgrtype *)mgr;
	addr = ptr;
	FUNC206LXDEV116(hmgr->access);
	for (i = 0; i < hmgr->validhead; i++) {
		for (pBlock = &hmgr->head[i]; pBlock; pBlock = pBlock->next) {
			if (pBlock->addr == addr) {
				FUNC206HAL200(pBlock, userdata);
				FUNC206LXDEV136(hmgr->access);
				return 0;
			}
		}
	}
	FUNC206LXDEV136(hmgr->access);
	return -3;
}

static unsigned int FUNC206HAL213(MemMgr mgr)
{
	int i;
	unsigned int total;
	mgrtype *hmgr;
	MemBlock *pBlock;
	if (!MAC206HAL229(mgr)) {
		return 0;
	}
	hmgr = mgr;
	total = 0;
	FUNC206LXDEV116(hmgr->access);
	for (i = 0; i < hmgr->validhead; i++) {
		pBlock = &hmgr->head[i];
		while (pBlock)  {
			if ((pBlock->size & 0x80000000) == 0) {
				total += pBlock->size;
			}
#ifdef DEBUG_REFCNT
			else {
				UserTable *ut = &pBlock->utable;
				int        users, j;
				for (users = 0; ut; ut = ut->next) {
					for (j = 0; j < 8; j++) {
						if (ut->userdata[j]) {
							users++;
							if ((ut->userflag & (1 << j)) == 0) {
								V206KDEBUG003("MWV206: userflag mismatch with user, cleared\n");
							}
						} else {
							if ((ut->userflag & (1 << j)) != 0) {
								V206KDEBUG003("MWV206: userflag mismatch with user, set\n");
							}
						}
					}
				}
				if (users != pBlock->refcnt) {
					V206KDEBUG003("MWV206: refcnt don't match user number, refcnt=%d, usercnt=%d\n",
							pBlock->refcnt, users);
				}
			}
#endif
			pBlock = pBlock->next;
		}
	}
	FUNC206LXDEV136(hmgr->access);
	return total;
}

static unsigned int FUNC206HAL207(MemMgr mgr, unsigned int *addr)
{
	int i;
	unsigned int maxlen;
	mgrtype *hmgr;
	MemBlock *pBlock;

	*addr = 0;

	if (!MAC206HAL229(mgr)) {
		return 0;
	}

	hmgr = mgr;
	maxlen = 0;

	FUNC206LXDEV116(hmgr->access);
	for (i = 0; i < hmgr->validhead; i++) {
		pBlock = &hmgr->head[i];
		while (pBlock) {
			if ((pBlock->size & 0x80000000) == 0)
				if (pBlock->size > maxlen) {
					maxlen = pBlock->size;
					*addr = pBlock->addr;
				}
			pBlock = pBlock->next;
		}
	}
	FUNC206LXDEV136(hmgr->access);

	return maxlen;
}

static void FUNC206HAL205(MemBlock *pBlock, int addr, int size)
{
	MemBlock *pB;
	if (size <= 0) {
		return ;
	}
	pB = (MemBlock *)MAC206HAL141(sizeof(MemBlock));
	if (pB == NULL) {
		return ;
	}
	memset(&pB->utable, 0, sizeof(UserTable));
	pB->addr   = addr;
	pB->size   = size;
	pB->refcnt = 0;
	pB->prev = pBlock;
	pB->next = pBlock->next;
	if (pB->next != NULL) {
		pB->next->prev = pB;
	}
	pBlock->next = pB;
}

static void FUNC206HAL209(MemBlock *pBlock)
{
	MemBlock *pPrev;
	pPrev = pBlock->prev;
	if ((pPrev != NULL) && ((pPrev->size & 0x80000000) == 0)) {
		pPrev->size += pBlock->size;
		pPrev->next = pBlock->next;
		if (pBlock->next != NULL) {
			pBlock->next->prev = pPrev;
		}
		FUNC206HAL210(pBlock);
		MAC206HAL039(pBlock);
	}
}

static void FUNC206HAL208(MemBlock *pBlock)
{
	MemBlock *pNext;
	pNext = pBlock->next;
	if ((pNext != NULL) && ((pNext->size & 0x80000000) == 0)) {
		pBlock->size += pNext->size;
		pBlock->next = pNext->next;
		if (pBlock->next != NULL) {
			pBlock->next->prev = pBlock;
		}
		FUNC206HAL210(pNext);
		MAC206HAL039(pNext);
	}
}

static void FUNC206HAL212(MemMgr mgr)
{
	int i;
	unsigned int mb, addr;
	mgrtype *hmgr;
	MemBlock *pBlock;
	if (!MAC206HAL229(mgr)) {
		V206DEV005("Invalid memmgr object %p\n", mgr);
		return ;
	}
	hmgr = mgr;
	FUNC206LXDEV116(hmgr->access);
	for (i = 0; i < hmgr->validhead; i++) {
		pBlock = &hmgr->head[i];
		while (pBlock != NULL) {
			V206KDEBUG002("[%08X]: %d, %s\n", pBlock->addr, pBlock->size & 0x7fffffff,
					(pBlock->size & 0x80000000) ? " " : "Free");
			pBlock = pBlock->next;
		}
	}
	FUNC206LXDEV136(hmgr->access);

	mb = FUNC206HAL207(mgr, &addr);
	V206KDEBUG002("!!![L%d]Free:%d, MaxBlockSize:%d\n", __LINE__, FUNC206HAL213(mgr), mb);
}

void FUNC206HAL202(MemMgr mgr, void *userdata)
{
	int i;
	mgrtype *hmgr;
	MemBlock *pBlock;
	if (!MAC206HAL229(mgr)) {
		return ;
	}
	hmgr = mgr;
	FUNC206LXDEV116(hmgr->access);
	for (i = 0; i < hmgr->validhead; i++) {
		int hasmore;
		hasmore = 1;
		while (hasmore) {
			hasmore = 0;
			pBlock = &hmgr->head[i];
			while (pBlock != NULL) {
				if (FUNC206HAL200(pBlock, userdata) == 0) {
					hasmore = 1;
					break;
				} else {
					pBlock = pBlock->next;
				}
			}
		}
	}
	FUNC206LXDEV136(hmgr->access);
}

GLJ_LOCK FUNC206HAL203(MemMgr mgr)
{
	mgrtype *hmgr;
	if (!MAC206HAL229(mgr)) {
		return 0;
	}
	hmgr = mgr;

	return hmgr->access;
}

static int FUNC206HAL211(MemMgr mgr, unsigned int addr, void *userdata)
{
	mgrtype  *hmgr;
	MemBlock *pBlock;
	int       i;

	if (!MAC206HAL229(mgr)) {
		return -1;
	}
	hmgr = (mgrtype *)mgr;

	FUNC206LXDEV116(hmgr->access);
	for (i = 0; i < hmgr->validhead; i++) {
		for (pBlock = &hmgr->head[i]; pBlock != NULL; pBlock = pBlock->next) {
			if (pBlock->addr == addr && (pBlock->size & 0x80000000) && pBlock->refcnt > 0) {
				int ret = FUNC206HAL198(pBlock, userdata);
				FUNC206LXDEV136(hmgr->access);
				return ret;
			}
		}
	}
	FUNC206LXDEV136(hmgr->access);

	return -2;
}

const memmgr_t FUNC206HAL418 = {
	.FUNC206HAL204            = FUNC206HAL204,
	.FUNC206HAL199          = FUNC206HAL199,
	.FUNC206HAL197        = FUNC206HAL197,
	.FUNC206HAL206          = FUNC206HAL206,
	.FUNC206HAL201            = FUNC206HAL201,
	.FUNC206HAL213       = FUNC206HAL213,
	.FUNC206HAL207     = FUNC206HAL207,
	.FUNC206HAL212            = FUNC206HAL212,
	.FUNC206HAL202  = FUNC206HAL202,
	.FUNC206HAL203   = FUNC206HAL203,
	.FUNC206HAL211       = FUNC206HAL211,
};


#define MAC206HAL156(...)

static int FUNC206HAL421(V206DEV025 *pDev)
{
	mgrtype *pmgr;
	int blockcnt, i, j;
	MemBlock *pBlock;
	saveMemmgr *pSaveMgr;
	unsigned int size;

	blockcnt = 0;
	for (i = 0; i < 4; i++) {
		pmgr = (mgrtype *)pDev->V206DEV068[i];
		if (!MAC206HAL229(pmgr)) {
			continue;
		}

		for (j = 0; j < pmgr->validhead; j++) {
			pBlock = &pmgr->head[j];
			while (pBlock) {
				if (pBlock->size & 0x80000000) {
					blockcnt++;
				}
				pBlock = pBlock->next;
			}
		}
	}

	size = (blockcnt) * sizeof(saveMemBlock) + sizeof(saveMemmgr);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
	pSaveMgr = (saveMemmgr *)__vmalloc(size, GFP_ATOMIC | __GFP_HIGHMEM);
#else
	pSaveMgr = (saveMemmgr *)__vmalloc(size, GFP_ATOMIC | __GFP_HIGHMEM, PAGE_KERNEL);
#endif
	if (pSaveMgr == NULL) {
		return -1;
	}
	memset(pSaveMgr, 0, size);

	pSaveMgr->blockcnt = blockcnt;
	pDev->pm.V206DEV120 = pSaveMgr;
	return 0;
}

int FUNC206HAL420(void *V206DEV103)
{
	MemBlock *pBlock;
	saveMemBlock *saveBlock;
	mgrtype *pmgr;
	int i, j, cnt;
	unsigned int size, V206IOCTLCMD009;
	void *V206IOCTLCMD006;
	int ret = 0;
	unsigned int totalsize = 0;
	V206DEV025 *pDev = (V206DEV025 *)V206DEV103;

	ret = FUNC206HAL421(pDev);
	if (ret != 0) {
		return -1;
	}

	cnt = 0;
	saveBlock = ((saveMemmgr *)(pDev->pm.V206DEV120))->pBlock;
	for (i = 0; i < 4; i++) {
		pmgr = (mgrtype *)pDev->V206DEV068[i];
		if (!MAC206HAL229(pmgr)) {
			continue;
		}

		V206DEV005("[INFO] pmgr->validhead %d \n", pmgr->validhead);
		for (j = 0; j < pmgr->validhead; j++) {
			pBlock = &pmgr->head[j];
			while (pBlock) {
				if (pBlock->size & 0x80000000) {

					size = pBlock->size;
					size = size & 0x7fffffff;
					V206IOCTLCMD009 = pBlock->addr;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
					V206IOCTLCMD006 = __vmalloc(size, GFP_ATOMIC | __GFP_HIGHMEM);
#else
					V206IOCTLCMD006 = __vmalloc(size, GFP_ATOMIC | __GFP_HIGHMEM, PAGE_KERNEL);
#endif
					V206DEV005("[INFO] pBlock[%d]: addr = 0x%08x, size = 0x%x. cpuaddr = %p\n", j, V206IOCTLCMD009, size, V206IOCTLCMD006);
					if (V206IOCTLCMD006 == 0) {
						V206KDEBUG002("[Error] %s: malloc cpu error.\n", __FUNCTION__);
						goto FUNC206HAL022;
					}

					saveBlock[cnt].size = size;
					saveBlock[cnt].V206IOCTLCMD009 = V206IOCTLCMD009;
					saveBlock[cnt].V206IOCTLCMD006 = V206IOCTLCMD006;
					totalsize += size;

#ifdef MWV206_PCIE_RW_DMA
					ret = FUNC206HAL327(pDev, V206IOCTL020, 0, V206IOCTLCMD009, V206IOCTLCMD006, size);
					if (ret != 0) {
						V206KDEBUG002("[Error] %s: dma read mwv206addr(0x%x) error, size = %d.\n",
								__FUNCTION__, V206IOCTLCMD009, size);
						saveBlock[cnt].V206IOCTLCMD006 = 0;
					}
#else
					FUNC206HAL316(pDev, 1);
					pmwv206addr = (unsigned char *)FUNC206HAL334(pDev, 1, V206IOCTLCMD009);
					FUNC206HAL377(V206IOCTLCMD006, pmwv206addr, size);
					FUNC206HAL316(pDev, 0);
#endif

					cnt++;

				}
				pBlock = pBlock->next;
			}
		}
	}
	MAC206HAL156("[INFO] %s: totalsize: %dbytes(%d+M).\n\n", __FUNCTION__, totalsize, totalsize >> 20);
	return 0;

FUNC206HAL022:
	for (i = 0; i < cnt; i++) {
		if (saveBlock[i].V206IOCTLCMD006) {
			vfree(saveBlock[i].V206IOCTLCMD006);
			saveBlock[i].V206IOCTLCMD006 = NULL;
		}
	}

	if (pDev->pm.V206DEV120) {
		vfree(pDev->pm.V206DEV120);
		pDev->pm.V206DEV120 = NULL;
	}

	return -1;
}

int FUNC206HAL419(void *V206DEV103)
{
	saveMemBlock *psaveBlock = NULL;
	int i, blockcnt = 0;
	unsigned int size, V206IOCTLCMD009;
	void *V206IOCTLCMD006;
	unsigned reloadsize = 0;
	V206DEV025 *pDev = (V206DEV025 *)V206DEV103;

	int ret = 0;
	if (pDev->pm.V206DEV120) {
		blockcnt = ((saveMemmgr *)(pDev->pm.V206DEV120))->blockcnt;
		psaveBlock = ((saveMemmgr *)(pDev->pm.V206DEV120))->pBlock;
	} else {
		V206KDEBUG002("[ERROR]%s: savemgr is NULL\n", __FUNCTION__);
		return -1;
	}

	V206DEV005("[INFO]%s: psaveBlock blockcnt %p %d \n", __FUNCTION__, psaveBlock, blockcnt);

	for (i = 0; i < blockcnt; i++) {
		size = psaveBlock[i].size;
		V206IOCTLCMD009 = psaveBlock[i].V206IOCTLCMD009;
		V206IOCTLCMD006 = psaveBlock[i].V206IOCTLCMD006;

		if (V206IOCTLCMD006 == 0) {
			V206KDEBUG002("[Error] %s: cpuaddr is null.\n", __FUNCTION__);
			continue;
		}

#ifdef MWV206_PCIE_RW_DMA
		ret = FUNC206HAL327(pDev, V206IOCTL021, 0, V206IOCTLCMD009, V206IOCTLCMD006, size);

		if (ret != 0) {
			V206KDEBUG002("[Error] %s: dma write mwv206addr 0x%x error, size = %d.\n", __FUNCTION__, V206IOCTLCMD009, size);
		}
#else
		FUNC206HAL316(pDev, 1);
		pmwv206addr = (unsigned char *)FUNC206HAL334(pDev, 1, V206IOCTLCMD009);
		FUNC206HAL377(pmwv206addr, V206IOCTLCMD006, size);
		FUNC206HAL316(pDev, 0);
#endif
		vfree(V206IOCTLCMD006);
		reloadsize += size;
	}

	if (pDev->pm.V206DEV120) {
		vfree(pDev->pm.V206DEV120);
	}

	MAC206HAL156("[INFO] %s: reload_size: %dbytes(%d+M).\n\n", __FUNCTION__, reloadsize, reloadsize >> 20);
	return 0;
}