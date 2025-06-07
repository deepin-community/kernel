/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _WST_SE_COMMON_TYPE_H
#define _WST_SE_COMMON_TYPE_H

#include <linux/kernel.h> /* We're doing kernel work */
#include <linux/module.h> /* Specifically, a module */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/version.h>
#include <linux/ioport.h>
#include <linux/time.h>
#include <linux/pagemap.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <asm/mman.h>
#include <asm/irq.h>
#include <asm/dma.h>

#define WST_GO_CHANNEL0 0x0002
#define WST_GO_CHANNEL1 0x0000
#define WST_GO_CHANNEL2 0x0001

#define SE_OK 0

struct loongson_sedriver_irq {
	char *pat;
	int irq;
};

struct tag_dma_buf_ctl {
	struct list_head list;
	unsigned char *pDmaBuf;
};

struct tag_Queue_container {
	struct list_head m_Head;
	unsigned int qlen;
	unsigned int max_qlen;
};

static inline int wst_InitQueue(struct tag_Queue_container *pQueue, int count)
{
	INIT_LIST_HEAD(&pQueue->m_Head);
	pQueue->qlen = 0;
	pQueue->max_qlen = count;
	return 0;
}

static inline struct list_head *
wst_Popfront_Que(struct tag_Queue_container *pQueue)
{
	struct list_head *pNode = NULL;

	if (list_empty(&pQueue->m_Head))
		return NULL;

	pQueue->qlen--;
	pNode = pQueue->m_Head.next;
	list_del(pNode);
	return pNode;
}

static inline int wst_Pushback_Que(struct tag_Queue_container *pQueue,
				   void *pNode)
{
	if (unlikely(pQueue->qlen >= pQueue->max_qlen))
		return -1;

	pQueue->qlen++;
	list_add_tail((struct list_head *)(pNode), &pQueue->m_Head);
	return 0;
}

#define READUBUF 0
#define WRITEUBUF 1

static inline int wst_cpyusrbuf(unsigned char *puserbuf, unsigned char *pkbuf,
				size_t num, int orient)
{
	if (orient == READUBUF)
		return copy_from_user(pkbuf, puserbuf, num);
	else
		return copy_to_user(puserbuf, pkbuf, num);
}

#endif
