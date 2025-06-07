// SPDX-License-Identifier: GPL-2.0
#include <linux/semaphore.h>
#include <linux/moduleparam.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/crypto.h>
#include <crypto/internal/skcipher.h>
#include <linux/acpi.h>
#include <asm/loongson.h>
#include "wst_se_echip_driver.h"

#define LS_CRYPTO_SE_ADDR1 TO_UNCACHE(LOONGSON_REG_BASE + 0x0400)
#define LS_CRYPTO_SE_ADDR2 TO_UNCACHE(0xc0010200000)
#define DRIVER_VERSION "01.10.220111"
DEFINE_SPINLOCK(g_reclistlock);
DEFINE_SPINLOCK(g_sendlistlock);
DEFINE_SPINLOCK(g_getdmabuflock);

unsigned char *g_pCacheInBuf = NULL, *g_pCacheOutBuf = NULL;
static struct class *g_psecclass;
static struct device *g_psecdev;
static struct tagSEdrvctl *g_psechipDrvCtrl;
static int g_isechip_Major = -1;
static struct semaphore g_lowsema, g_lowirqsema;
static atomic_t g_sendtotallen;
static struct tag_Queue_container g_RecQueueContainer;
static struct tag_Queue_container g_SendQueueContainer;
static int g_suspend;
static struct semaphore g_dmabufsem;
struct loongson_sedriver_irq se_irq[] = {
	{
		.pat = "se-irq",
	},
	{
		.pat = "se-lirq",
	},
};
static struct tag_Queue_container g_DmaBQueueContainer;
static struct tag_dma_buf_ctl *g_pdmadatabuf;
static int g_iDmaBufNum;
static struct work_struct g_recwork, g_sendwork;
static struct workqueue_struct *g_worksendqueue, *g_workrecqueue;
static irqreturn_t se_interrupt(int irq, void *p);
static irqreturn_t wst_low_channel_status(int irq, void *p);
static void globalmem_do_send_op(struct work_struct *p);
static void globalmem_do_rec_op(struct work_struct *p);
static int g_iUseIntr = 1;
module_param(g_iUseIntr, int, 0644);

static int se_init_dma_buf(int idatasize, int idatanum)
{
	int i;
	struct tag_dma_buf_ctl *pdmactl;

	wst_InitQueue(&g_DmaBQueueContainer, idatanum);
	g_pdmadatabuf = kmalloc((sizeof(struct tag_dma_buf_ctl) * idatanum),
				GFP_KERNEL);
	if (!g_pdmadatabuf)
		return -1;
	for (i = 0; i < idatanum; i++) {
		pdmactl = &g_pdmadatabuf[i];
		pdmactl->pDmaBuf =
			(unsigned char *)__get_free_page(GFP_KERNEL | GFP_DMA);
		if (!pdmactl->pDmaBuf) {
			g_iDmaBufNum = i;
			return SE_OK;
		}
		wst_Pushback_Que(&g_DmaBQueueContainer, pdmactl);
	}
	g_iDmaBufNum = i;
	sema_init(&g_dmabufsem, 1);
	return SE_OK;
}

static int se_del_dma_buf(void)
{
	int i;
	struct tag_dma_buf_ctl *pdmactl;

	for (i = 0; i < g_iDmaBufNum; i++) {
		pdmactl = &g_pdmadatabuf[i];
		if (pdmactl) {
			free_page((unsigned long)pdmactl->pDmaBuf);
			pdmactl->pDmaBuf = NULL;
		}
	}
	kfree(g_pdmadatabuf);
	g_pdmadatabuf = NULL;
	return 0;
}

struct tag_dma_buf_ctl *se_get_dma_buf(int ikernel)
{
	struct tag_dma_buf_ctl *pbufctl = NULL;
	unsigned long ultimeout = 0;

	ultimeout = jiffies + 20 * HZ;
	while (1) {
		spin_lock(&g_getdmabuflock);
		pbufctl = (struct tag_dma_buf_ctl *)wst_Popfront_Que(
			&g_DmaBQueueContainer);
		spin_unlock(&g_getdmabuflock);
		if (pbufctl)
			return pbufctl;
		if (down_timeout(&g_dmabufsem, ultimeout))
			return NULL;
	}
	return pbufctl;
}

int se_free_dma_buf(struct tag_dma_buf_ctl *pdmabufctl)
{
	spin_lock(&g_getdmabuflock);
	wst_Pushback_Que(&g_DmaBQueueContainer, pdmabufctl);
	spin_unlock(&g_getdmabuflock);
	if (g_dmabufsem.count <= 0)
		up(&g_dmabufsem);

	return 0;
}

static unsigned long bytes_align(struct device *pdev, unsigned long ulVirAddr)
{
	unsigned char diff;
	unsigned long ulPhyAddr = (unsigned long)__pa((void *)ulVirAddr);

	if ((ulPhyAddr & 0x000000000000003f) == 0)
		return ulVirAddr;
	diff = ((long)ulPhyAddr & (~(0x000000000000003f))) + 64 - ulPhyAddr;
	ulVirAddr += diff;

	return ulVirAddr;
}

static unsigned long descri_bytes_align(unsigned long ulVirAddr)
{
	unsigned char diff;
	unsigned long ulPhyAddr = ulVirAddr;

	if ((ulPhyAddr & (~0x00000000ffffffe0)) == 0)
		return ulVirAddr;
	diff = ((long)ulPhyAddr & 0x00000000ffffffe0) + 32 - ulPhyAddr;
	ulVirAddr += diff;
	return ulVirAddr;
}

int se_printk_hex(unsigned char *buff, int length)
{
	unsigned char *string_tmp = buff;
	int i;
	int count = 0;

	for (i = 0; i < length; i++, count++) {
		if (count < 16)
			pr_info("%02x ", string_tmp[i]);
		else {
			count = 0;
			pr_info("\n%02x ", string_tmp[i]);
			continue;
		}
	}
	pr_info("\n");
	return 0;
}

static int se_ChipInit(struct tagSEdrvctl *pDrvCtrl)
{
	dma_addr_t ulBusAddr;
	unsigned long ulVirAddr;
	int i = 0, j = 0;
	unsigned int dmaoldmask;

	for (i = 0; i < SWCHANNELNUM; i++) {
		ulVirAddr = (unsigned long)dma_alloc_coherent(
			pDrvCtrl->pdev, (SE_BDQUEUE_LEN * SE_BD_LENGTH + 32),
			&ulBusAddr, GFP_KERNEL);
		if (ulVirAddr == 0 || ulBusAddr == 0)
			return -EFAULT;
		memset((void *)ulVirAddr, 0, (SE_BDQUEUE_LEN * SE_BD_LENGTH));

		pDrvCtrl->ulBDMemBasePhy[i] = ulBusAddr;
		pDrvCtrl->ulBDMemBase[i] = ulVirAddr;
		pDrvCtrl->ulCurrBdReadPtr[i] = 0;
		pDrvCtrl->ulCurrBdWritePtr[i] = 0;
		pDrvCtrl->ulCurrReadPtr[i] = 0;
		pDrvCtrl->ulCurrWritePtr[i] = 0;
	}
	for (i = 0; i < SE_BDQUEUE_LEN; i++) {
		for (j = 0; j < SWCHANNELNUM; j++)
			(&((struct tagSEBasicBD *)(pDrvCtrl->ulBDMemBase[j]))[i])
				->ucRetCode = 0x0f;
	}
	ulBusAddr = descri_bytes_align(pDrvCtrl->ulBDMemBasePhy[0]);
	HandleWrite32(pDrvCtrl, SE_HREG_BQBA0, HIULONG(ulBusAddr));
	HandleWrite32(pDrvCtrl, SE_LREG_BQBA0, LOULONG(ulBusAddr));
	HandleWrite32(pDrvCtrl, SE_REG_BQS0, SE_BDQUEUE_LEN - 1);
	HandleWrite32(pDrvCtrl, SE_REG_RQRP0, pDrvCtrl->ulCurrBdReadPtr[0]);
	HandleWrite32(pDrvCtrl, SE_REG_BQWP0, pDrvCtrl->ulCurrBdWritePtr[0]);

	ulBusAddr = descri_bytes_align(pDrvCtrl->ulBDMemBasePhy[1]);
	HandleWrite32(pDrvCtrl, SE_HREG_BQBA1, HIULONG(ulBusAddr));
	HandleWrite32(pDrvCtrl, SE_LREG_BQBA1, LOULONG(ulBusAddr));
	HandleWrite32(pDrvCtrl, SE_REG_BQS1, SE_BDQUEUE_LEN - 1);
	HandleWrite32(pDrvCtrl, SE_REG_RQRP1, pDrvCtrl->ulCurrBdReadPtr[1]);
	HandleWrite32(pDrvCtrl, SE_REG_BQWP1, pDrvCtrl->ulCurrBdWritePtr[1]);
	HandleRead32(pDrvCtrl, SE_REG_MSK, &dmaoldmask);
	HandleWrite32(pDrvCtrl, SE_REG_MSK,
		      (dmaoldmask | DMA0_CTRL_CHANNEL_ENABLE |
		       DMA1_CTRL_CHANNEL_ENABLE));
	if (g_iUseIntr != 0)
		HandleWrite32(pDrvCtrl, SE_LOWREG_INQ, 1);
	else
		HandleWrite32(pDrvCtrl, SE_LOWREG_INQ, 0);
	mdelay(1000);

	return SE_OK;
}

static void se_ChipRelease(struct tagSEdrvctl *pDrvCtrl)
{
	int i;

	for (i = 0; i < SWCHANNELNUM; i++) {
		if (pDrvCtrl->ulBDMemBase[i]) {
			dma_free_coherent(pDrvCtrl->pdev,
					  (SE_BDQUEUE_LEN * SE_BD_LENGTH),
					  (void *)pDrvCtrl->ulBDMemBase[i],
					  pDrvCtrl->ulBDMemBasePhy[i]);
			pDrvCtrl->ulBDMemBase[i] = 0;
			pDrvCtrl->ulBDMemBasePhy[i] = 0;
		}
	}
}

static void SE_RESET(struct tagSEdrvctl *pdrvctl)
{
	unsigned int reg;
	unsigned long ulreg64, uladdr = LS_CRYPTO_SE_ADDR1;

	HandleRead32(pdrvctl, SE_REG_RESET, &reg);
	HandleWrite32(pdrvctl, SE_REG_RESET, reg | SE_DMA_CONTROL_RESET);
	mdelay(300);
	HandleWrite32(pdrvctl, SE_REG_RESET,
		      (reg & (~SE_DMA_CONTROL_RESET)) | SE_DMA_CONTROL_SET);
	mdelay(300);
	ulreg64 = readq((void *)uladdr);
	if ((ulreg64 & 0xf0000000000000) != 0xf0000000000000)
		writeq(ulreg64 | 0xf0000000000000, (void *)uladdr);
	HandleWrite32(pdrvctl, SE_INT_CLR, 0xf);
}

static int wst_init(void)
{
	int iRes = SE_OK;
	static u64 wst_dma_mask = DMA_BIT_MASK(64);
	char cName[256];
	struct tagSEdrvctl *pdrvctl = NULL;

	pdrvctl = kmalloc(sizeof(struct tagSEdrvctl), GFP_KERNEL);
	if (pdrvctl == NULL)
		return -ENOMEM;
	memset(pdrvctl, 0, sizeof(struct tagSEdrvctl));
	pdrvctl->ulMemBase = LS_CRYPTO_SE_ADDR2;
	memset(cName, 0, 256);
	sema_init(&(pdrvctl->sema), 0);
	rwlock_init(&(pdrvctl->mr_lock));
	rwlock_init(&(pdrvctl->mr_lowlock));
	g_psechipDrvCtrl = pdrvctl;
	g_psechipDrvCtrl->pdev = g_psecdev;
	g_psechipDrvCtrl->pdev->dma_mask = &wst_dma_mask;
	g_psechipDrvCtrl->pdev->coherent_dma_mask =
		(unsigned long long)&wst_dma_mask;
	wst_InitQueue(&g_RecQueueContainer, 2000);
	wst_InitQueue(&g_SendQueueContainer, 2000);
	SE_RESET(pdrvctl);
	pdrvctl->ilowIrq = 0;
	pdrvctl->iIrq = se_irq[0].irq;
	iRes = request_irq(pdrvctl->iIrq, &se_interrupt, IRQF_SHARED,
			   "wst-se-hirq", pdrvctl);
	if (iRes) {
		pr_err("request_irq err\n");
		pdrvctl->iIrq = 0;
		goto err;
	}
	if (g_iUseIntr == 1) {
		pdrvctl->ilowIrq = se_irq[1].irq;
		iRes = request_irq(pdrvctl->ilowIrq, &wst_low_channel_status,
				   IRQF_SHARED, "wst-se-lirq", pdrvctl);
		if (iRes) {
			pr_err("\nrequest_lowirq err, iRes=0x%x\n", iRes);
			pdrvctl->ilowIrq = 0;
			goto err;
		}
	}
	if (se_ChipInit(pdrvctl) != SE_OK) {
		iRes = -ENODEV;
		goto err;
	}
	return SE_OK;
err:
	if (pdrvctl != NULL) {
		if (pdrvctl->iIrq) {
			free_irq(pdrvctl->iIrq, pdrvctl);
			pdrvctl->iIrq = 0;
		}
		if (pdrvctl->ilowIrq) {
			free_irq(pdrvctl->ilowIrq, pdrvctl);
			pdrvctl->ilowIrq = 0;
		}
		se_ChipRelease(pdrvctl);
		kfree(pdrvctl);
		g_psechipDrvCtrl = NULL;
	}
	return iRes;
}

static void wst_clear(void)
{
	struct tagSEdrvctl *pdrvctl = NULL;

	pdrvctl = g_psechipDrvCtrl;
	if (pdrvctl) {
		if (pdrvctl->iIrq) {
			free_irq(pdrvctl->iIrq, pdrvctl);
			pdrvctl->iIrq = 0;
		}
		if (pdrvctl->ilowIrq) {
			free_irq(pdrvctl->ilowIrq, pdrvctl);
			pdrvctl->ilowIrq = 0;
		}
		se_ChipRelease(pdrvctl);
		kfree(pdrvctl);
		g_psechipDrvCtrl = NULL;
	}
}
static void globalmem_do_send_op(struct work_struct *p)
{
	struct tagSEBasicBD *pCurBD;
	unsigned int ulCurrWritePtr, ulWritePtr;
	unsigned short len = 0;
	unsigned long ulCurrAddrInput = 0, ulCurrAddrOutput = 0;
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
	struct ST_SEND_PACKAGE *psendpackage;
	unsigned long ulflag;
	unsigned long ultimeout;
	int rv = 0;

PROG:
	spin_lock_irq(&g_sendlistlock);
	psendpackage = (struct ST_SEND_PACKAGE *)wst_Popfront_Que(&g_SendQueueContainer);
	if (!psendpackage) {
		spin_unlock_irq(&g_sendlistlock);
		return;
	}
	spin_unlock_irq(&g_sendlistlock);
	pdrvctl = psendpackage->pdrvctl;
	pInPtr = psendpackage->pInPtr;
	usInlen = psendpackage->usInlen;
	pOutPtr = psendpackage->pOutPtr;
	pusOutlen = psendpackage->pusOutlen;
	iChannel = psendpackage->iChannel;
	ucFlag = psendpackage->ucFlag;
	ucOpCode = psendpackage->ucOpCode;
	pucRetCode = psendpackage->pucRetCode;
	pcallback = psendpackage->pcallback;
	pParma = psendpackage->pParma;
	iKernel = psendpackage->iKernel;
	mycomplete = psendpackage->mycomplete;
	ultimeout = psendpackage->ulendtime;
	kfree(psendpackage);

	if (iKernel == 0) {
		while (time_before(jiffies, ultimeout)) {
#ifdef CONFIG_MIPS
			if ((pdrvctl->ulCurrBdReadPtr[iChannel] ==
			     ((pdrvctl->ulCurrBdWritePtr[iChannel] + 1) &
			      (SE_BDQUEUE_LEN - 1))) ||
			    ((atomic_read(&g_sendtotallen) + *pusOutlen +
			      SE_FILL_LEN) > SE_MAX_SEND_LEN))
#else
			if (pdrvctl->ulCurrBdReadPtr[iChannel] ==
			    ((pdrvctl->ulCurrBdWritePtr[iChannel] + 1) &
			     (SE_BDQUEUE_LEN - 1)))
#endif
			{
				if (down_timeout(&(pdrvctl->sema), 1 * HZ))
					pr_info("down timeout\n");
				rv = WST_SE_ERROR_FULL;
			} else {
				rv = 0;
				break;
			}
		}
		if (rv != 0x0) {
			*pucRetCode = WST_SE_ERROR_FULL;
			complete(mycomplete);
			goto PROG;
		}
	} else {
		ultimeout = jiffies + 1 * HZ;
		while (time_before(jiffies, ultimeout)) {
#ifdef CONFIG_MIPS
			if ((pdrvctl->ulCurrBdReadPtr[iChannel] ==
			     ((pdrvctl->ulCurrBdWritePtr[iChannel] + 1) &
			      (SE_BDQUEUE_LEN - 1))) ||
			    ((atomic_read(&g_sendtotallen) + *pusOutlen +
			      SE_FILL_LEN) > SE_MAX_SEND_LEN))
#else
			if (pdrvctl->ulCurrBdReadPtr[iChannel] ==
			    ((pdrvctl->ulCurrBdWritePtr[iChannel] + 1) &
			     (SE_BDQUEUE_LEN - 1)))
#endif
			{
				rv = WST_SE_ERROR_FULL;
			} else {
				rv = 0;
				break;
			}
		}
		if (rv != 0x0) {
			*pucRetCode = WST_SE_ERROR_FULL;
			if (pcallback)
				pcallback(pParma);
			goto PROG;
		}
	}
	ulCurrWritePtr = pdrvctl->ulCurrBdWritePtr[iChannel];
	ulWritePtr = (ulCurrWritePtr + 1) & (SE_BDQUEUE_LEN - 1);

	pCurBD = &((
		struct tagSEBasicBD *)(pdrvctl->ulBDMemBase[iChannel]))[ulCurrWritePtr];
	memset(pCurBD, 0x0, sizeof(struct tagSEBasicBD));
	if (pcallback != NULL) {
		(pdrvctl->pcallback)[iChannel][ulCurrWritePtr] = pcallback;
		pdrvctl->pParma[iChannel][ulCurrWritePtr] = pParma;
	} else {
		(pdrvctl->pcallback)[iChannel][ulCurrWritePtr] = NULL;
		pdrvctl->pParma[iChannel][ulCurrWritePtr] = NULL;
	}

	pdrvctl->ikernel[iChannel][ulCurrWritePtr] = iKernel;
	pdrvctl->stsemphore[iChannel][ulCurrWritePtr] = mycomplete;

	if (pInPtr == pOutPtr) {
		if (pOutPtr) {
			len = usInlen >= *pusOutlen ? usInlen : *pusOutlen;
			if (len) {
				ulCurrAddrOutput =
					dma_map_single(pdrvctl->pdev, pOutPtr,
						       len, DMA_BIDIRECTIONAL);
				if (ulCurrAddrOutput == 0) {
					pr_err(
						"map ulCurrAddrOutput error\n");
					*pucRetCode = WST_SE_FAILURE;
					if (iKernel == 0)
						complete(mycomplete);
					else if (pcallback)
						pcallback(pParma);
					goto PROG;
				}
				pCurBD->ulOutputLPtr =
					LOULONG(ulCurrAddrOutput);
				pCurBD->ulOutputHPtr =
					HIULONG(ulCurrAddrOutput);
				pCurBD->ulInputLPtr = pCurBD->ulOutputLPtr;
				pCurBD->ulInputHPtr = pCurBD->ulOutputHPtr;
			}
		}
	} else {
		if (pOutPtr && (*pusOutlen)) {
			ulCurrAddrOutput = dma_map_single(pdrvctl->pdev,
							  pOutPtr, *pusOutlen,
							  DMA_FROM_DEVICE);
			if (ulCurrAddrOutput == 0) {
				pr_err("map ulCurrAddrOutput error\n");
				*pucRetCode = WST_SE_FAILURE;
				if (iKernel == 0) {
					complete(mycomplete);
				} else {
					*pucRetCode = WST_SE_FAILURE;
					if (pcallback)
						pcallback(pParma);
				}
				goto PROG;
			}
			pCurBD->ulOutputLPtr = LOULONG(ulCurrAddrOutput);
			pCurBD->ulOutputHPtr = HIULONG(ulCurrAddrOutput);
		}
		if (usInlen && pInPtr) {
			ulCurrAddrInput = dma_map_single(
				pdrvctl->pdev, pInPtr, usInlen, DMA_TO_DEVICE);
			if (ulCurrAddrInput == 0) {
				if (ulCurrAddrOutput) {
					dma_unmap_single(pdrvctl->pdev,
							 ulCurrAddrOutput,
							 *pusOutlen,
							 DMA_FROM_DEVICE);
					pCurBD->ulOutputLPtr = 0;
					pCurBD->ulOutputHPtr = 0;
				}
				*pucRetCode = WST_SE_FAILURE;
				if (iKernel == 0) {
					complete(mycomplete);
				} else {
					*pucRetCode = WST_SE_FAILURE;
					if (pcallback)
						pcallback(pParma);
				}
				goto PROG;
			}
			pCurBD->ulInputLPtr = LOULONG(ulCurrAddrInput);
			pCurBD->ulInputHPtr = HIULONG(ulCurrAddrInput);
		}
	}
	pCurBD->ucOpCode = ucOpCode & 0x0f;
	pCurBD->ucFlag = ucFlag & 0x7;
	pCurBD->usInputLength = usInlen;
	if (pusOutlen)
		pCurBD->usOutputLength = *pusOutlen;

	pCurBD->ucRetCode = 0x0f;

	pdrvctl->pusOutlen[iChannel][ulCurrWritePtr] = pusOutlen;
	pdrvctl->usInlen[iChannel][ulCurrWritePtr] = usInlen & 0xffff;
	if (ulCurrAddrOutput)
		pdrvctl->ulOutputPtr[iChannel][ulCurrWritePtr] =
			(unsigned long *)ulCurrAddrOutput;
	else
		pdrvctl->ulOutputPtr[iChannel][ulCurrWritePtr] = 0;
	if (ulCurrAddrInput)
		pdrvctl->ulInputPtr[iChannel][ulCurrWritePtr] =
			(unsigned long *)ulCurrAddrInput;
	else
		pdrvctl->ulInputPtr[iChannel][ulCurrWritePtr] = 0;
	pdrvctl->pucRetCode[iChannel][ulCurrWritePtr] = pucRetCode;

#ifdef CONFIG_MIPS
	atomic_add(
		(*(pdrvctl->pusOutlen[iChannel][ulCurrWritePtr]) + SE_FILL_LEN),
		&g_sendtotallen);
#endif
	write_lock_irqsave(&(pdrvctl->mr_lock), ulflag);
	if (iChannel == 0)
		HandleWrite32(pdrvctl, SE_REG_BQWP0, ulWritePtr);
	else
		HandleWrite32(pdrvctl, SE_REG_BQWP1, ulWritePtr);
	write_unlock_irqrestore(&(pdrvctl->mr_lock), ulflag);
	pdrvctl->ulCurrBdWritePtr[iChannel] = ulWritePtr;
	goto PROG;
}

static int se_hardtrans(struct tagSEdrvctl *pdrvctl, unsigned char *pInPtr,
			unsigned short usInlen, unsigned char *pOutPtr,
			unsigned short *pusOutlen, int iChannel,
			unsigned char ucFlag, unsigned char ucOpCode,
			unsigned char *pucRetCode, PSECallBackfn pcallback,
			void *pParma, int iKernel,
			struct completion *mycomplete)
{
	struct ST_SEND_PACKAGE *psendpackage;
	gfp_t gfp_flag;

	if (in_interrupt())
		gfp_flag = GFP_ATOMIC;
	else
		gfp_flag = GFP_KERNEL;
	if (g_suspend == 1)
		return WST_SE_FAILURE;
	psendpackage = kmalloc(sizeof(struct ST_SEND_PACKAGE), gfp_flag);
	if (psendpackage == NULL)
		return -1;

	psendpackage->pdrvctl = pdrvctl;
	psendpackage->pInPtr = pInPtr;
	psendpackage->usInlen = usInlen;
	psendpackage->pOutPtr = pOutPtr;
	psendpackage->pusOutlen = pusOutlen;
	psendpackage->iChannel = iChannel;
	psendpackage->ucFlag = ucFlag;
	psendpackage->ucOpCode = ucOpCode;
	psendpackage->pucRetCode = pucRetCode;
	psendpackage->pcallback = pcallback;
	psendpackage->pParma = pParma;
	psendpackage->iKernel = iKernel;
	psendpackage->mycomplete = mycomplete;
	psendpackage->ulendtime = jiffies + 30 * HZ;
	spin_lock_irq(&g_sendlistlock);
	if (wst_Pushback_Que(&g_SendQueueContainer, psendpackage) == -1) {
		spin_unlock_irq(&g_sendlistlock);
		kfree(psendpackage);
		return WST_SE_ERROR_FULL;
	}
	spin_unlock_irq(&g_sendlistlock);
	queue_work(g_worksendqueue, &g_sendwork);
	return 0;
}

static irqreturn_t wst_low_channel_status(int irq, void *p)
{
	struct tagSEdrvctl *pdrvctl = p;
	int64_t ulIntStat = 0;
	unsigned long ulflag;

	read_lock_irqsave(&(pdrvctl->mr_lock), ulflag);
	HandleRead64(pdrvctl, SE_LOWREG_STS, &ulIntStat);
	if (ulIntStat == 2) {
		HandleWrite64(pdrvctl, SE_LOWINT_CLEAR, 2);
		up(&g_lowirqsema);
	}
	read_unlock_irqrestore(&(pdrvctl->mr_lock), ulflag);

	return IRQ_HANDLED;
}

static int se_useropen(struct inode *inode, struct file *file)
{
	if (MINOR(inode->i_rdev) != 0)
		return -ENODEV;

	return SE_OK;
}

static ssize_t wst_low_channel_userwrite_op(struct tagSEdrvctl *pdrvctl,
					    struct tagSWCOMMUDATA *UserCommuData,
					    int iskernel)
{
	unsigned long long addr = 0, outaddr = 0;
	int ilen;
	int count = SE_OK;
	unsigned long long ulsendlen;
	unsigned char *m_pCacheInBuf;
	unsigned char *m_pCacheOutBuf;
	unsigned long ulflag;

	if ((g_pCacheInBuf == NULL) || (g_pCacheOutBuf == NULL))
		return -EFAULT;

	m_pCacheInBuf =
		(unsigned char *)bytes_align(0, (unsigned long)g_pCacheInBuf);
	m_pCacheOutBuf =
		(unsigned char *)bytes_align(0, (unsigned long)g_pCacheOutBuf);
	if (iskernel == 0) {
		if (wst_cpyusrbuf((void *)(UserCommuData->pucInbuf),
				  (void *)m_pCacheInBuf,
				  UserCommuData->usInputLen, READUBUF)) {
			pr_err("copy user data error\n");
			return -EFAULT;
		}
	} else

		memcpy((void *)m_pCacheInBuf, (void *)(UserCommuData->pucInbuf),
		       UserCommuData->usInputLen);
	ilen = UserCommuData->usInputLen >= UserCommuData->usOutputLen ?
		       UserCommuData->usInputLen :
		       UserCommuData->usOutputLen;
	addr = dma_map_single(pdrvctl->pdev, m_pCacheInBuf, ilen,
			      DMA_TO_DEVICE);
	if (addr == 0) {
		pr_err("transfer buffer is err\n");
		return -EFAULT;
	}
	outaddr = dma_map_single(pdrvctl->pdev, m_pCacheOutBuf, ilen,
				 DMA_FROM_DEVICE);
	if (outaddr == 0) {
		pr_err("transfer buffer is err\n");
		dma_unmap_single(pdrvctl->pdev, addr, ilen, DMA_TO_DEVICE);
		return -EFAULT;
	}
	ulsendlen = (UserCommuData->usInputLen / 8);
	ulsendlen = (ulsendlen & 0x00000000ffffffff) << 32;
	write_lock_irqsave(&(pdrvctl->mr_lock), ulflag);
	HandleWrite64(pdrvctl, SE_WRITE_REG1, ulsendlen);
	HandleWrite64(pdrvctl, SE_WRITE_REG2, addr);
	HandleWrite64(pdrvctl, SE_WRITE_REG3, outaddr);
	write_unlock_irqrestore(&(pdrvctl->mr_lock), ulflag);
	if (g_iUseIntr != 0) {
		if (down_interruptible(&g_lowirqsema) == -EINTR) {
			count = -EINTR;
			goto EXIT;
		}
	} else {
		unsigned long start_jiffies = 0, end_jiffies = 0;
		int64_t ulIntStat = 0;

		start_jiffies = jiffies;
		end_jiffies = jiffies;
		while (1) {
			write_lock_irqsave(&(pdrvctl->mr_lock), ulflag);
			HandleRead64(pdrvctl, SE_LOWREG_SR, &ulIntStat);
			end_jiffies = jiffies;
			if (ulIntStat == 1) {
				HandleWrite64(pdrvctl, SE_LOWREG_SR, 0);
				write_unlock_irqrestore(&(pdrvctl->mr_lock),
							ulflag);
				break;
			}
			write_unlock_irqrestore(&(pdrvctl->mr_lock), ulflag);
			if (jiffies_to_msecs(end_jiffies - start_jiffies) /
				    1000 >=
			    90) {
				count = -EFAULT;
				goto EXIT;
			}
		}
	}
	dma_unmap_single(pdrvctl->pdev, addr, ilen, DMA_TO_DEVICE);
	dma_unmap_single(pdrvctl->pdev, outaddr, ilen, DMA_FROM_DEVICE);
	if (UserCommuData->usOutputLen) {
		if (iskernel == 0) {
			if (wst_cpyusrbuf(
				    UserCommuData->pucOutbuf, m_pCacheOutBuf,
				    UserCommuData->usOutputLen, WRITEUBUF))
				return -EFAULT;
		} else
			memcpy(UserCommuData->pucOutbuf, m_pCacheOutBuf,
			       UserCommuData->usOutputLen);
	}
	return count;
EXIT:
	dma_unmap_single(pdrvctl->pdev, addr, ilen, DMA_TO_DEVICE);
	dma_unmap_single(pdrvctl->pdev, outaddr, ilen, DMA_FROM_DEVICE);
	return count;
}

static ssize_t se_userwrite(struct file *file, const char *buf, size_t count,
			    loff_t *ppos)
{
	unsigned char *pCacheBuf = NULL, *pCacheOutBuf = NULL,
		      *pCacheBufalign = NULL, *pCacheOutBufalign = NULL;
	struct tagSEdrvctl *pdrvctl = NULL;
	struct tagSWCOMMUDATA *pCommuData = NULL;
	int iCommuDatalen = 0;
	int pucRetCode = 0;
	unsigned short iChannel = 0;
	unsigned char ucFlag = 0, ucOpCode = 0;
	int *ppucRetCode;
	struct completion mycomplete;
	struct tag_dma_buf_ctl *pbufinctl = NULL;
	int iret = 0;

	if (count == 0) {
		pr_err("count=0\n");
		return SE_OK;
	}

	if (MINOR(file->f_path.dentry->d_inode->i_rdev) != 0)
		return -ENODEV;

	iCommuDatalen = sizeof(struct tagSWCOMMUDATA);
	if (count != iCommuDatalen)
		return -EINVAL;

	pdrvctl = g_psechipDrvCtrl;
	pCommuData = kmalloc(iCommuDatalen, GFP_KERNEL);
	if (!pCommuData) {
		pr_err("pCommuData NULL\n");
		return -ENOMEM;
	}
	if (wst_cpyusrbuf((void *)buf, (void *)pCommuData, iCommuDatalen,
			  READUBUF)) {
		pr_err("copy user data error\n");
		count = -EFAULT;
		goto EXIT;
	}
	switch ((pCommuData->usFlags) & 0x000f) {
	case WST_GO_CHANNEL2:
		if ((pCommuData->usInputLen > DMA_BUFSIZE) ||
		    (pCommuData->usOutputLen > DMA_BUFSIZE)) {
			pr_err("len is error\n");
			count = -EINVAL;
			goto EXIT;
		}
		if (down_interruptible(&g_lowsema) == -EINTR) {
			count = -EINTR;
			goto EXIT;
		}
		count = wst_low_channel_userwrite_op(pdrvctl, pCommuData, 0);
		up(&g_lowsema);
		goto EXIT;
	case WST_GO_CHANNEL0:
		if (pCommuData->usInputLen == 0) {
			count = -EINVAL;
			goto EXIT;
		}
		if (pCommuData->usInputLen != 0) {
			if (pCommuData->usInputLen > DMA_BUFSIZE) {
				count = -EINVAL;
				goto EXIT;
			}
			ucFlag = INPUT_VALID;
			if (pCommuData->usOutputLen)
				ucFlag |= OUTPUT_VALID;
		}

		iChannel = 0;
		ucOpCode = 0x0;
		break;
	case WST_GO_CHANNEL1:
		if (pCommuData->usInputLen == 0) {
			count = -EINVAL;
			goto EXIT;
		}
		if (pCommuData->usInputLen != 0) {
			if (pCommuData->usInputLen > DMA_BUFSIZE) {
				count = -EINVAL;
				goto EXIT;
			}
			ucFlag = INPUT_VALID;
			if (pCommuData->usOutputLen)
				ucFlag |= OUTPUT_VALID;
		}
		iChannel = 1;
		ucOpCode = 0x0;
		break;
	default: {
		count = -EINVAL;
		goto EXIT;
	}
	}
	pbufinctl = se_get_dma_buf(0);
	if (pbufinctl == NULL) {
		pr_err("kmalloc pCacheBuf error\n");
		count = -ENOMEM;
		goto EXIT;
	}
	pCacheBuf = pbufinctl->pDmaBuf;
	pCacheBufalign = pCacheBuf;

	if (wst_cpyusrbuf((void *)(pCommuData->pucInbuf),
			  (void *)pCacheBufalign, pCommuData->usInputLen,
			  READUBUF)) {
		pr_err("cpyuserbuf pCacheBufalign error\n");
		count = -ENOMEM;
		goto EXIT;
	}

	pCacheOutBuf = pbufinctl->pDmaBuf;
	pCacheOutBufalign = pCacheOutBuf;

	ppucRetCode = &pucRetCode;

	count = SE_OK;
	init_completion(&mycomplete);
	iret = se_hardtrans(pdrvctl, pCacheBufalign, pCommuData->usInputLen,
			    pCacheOutBufalign, &(pCommuData->usOutputLen),
			    iChannel, ucFlag, ucOpCode,
			    (unsigned char *)ppucRetCode, 0, 0, 0, &mycomplete);
	if (iret == -1) {
		count = -EIO;
		goto EXIT;
	}
	if (!wait_for_completion_timeout(&mycomplete,
					 msecs_to_jiffies(60 * 1000))) {
		count = -EFAULT;
		goto EXIT;
	}
	if (pucRetCode != SE_OK) {
		count = -(SE_BASEERR + pucRetCode);
		goto EXIT;
	}

	if (pCommuData->pucOutbuf) {
		if (wst_cpyusrbuf(pCommuData->pucOutbuf, pCacheOutBufalign,
				  pCommuData->usOutputLen, WRITEUBUF)) {
			count = -EFAULT;
			goto EXIT;
		}
	}
EXIT:
	if (pbufinctl)
		se_free_dma_buf(pbufinctl);
	kfree(pCommuData);
	return count;
}
static void globalmem_do_rec_op(struct work_struct *p)
{
	struct ST_INT_MESSAGE *intmessage;
	unsigned long ulflags1;

	while (1) {
		spin_lock_irqsave(&g_reclistlock, ulflags1);
		intmessage =
			(struct ST_INT_MESSAGE *)wst_Popfront_Que(&g_RecQueueContainer);
		spin_unlock_irqrestore(&g_reclistlock, ulflags1);
		if (!intmessage)
			return;
		intmessage->pcallback(intmessage->pParma);
		kfree(intmessage);
	}
}
static irqreturn_t se_interrupt(int irq, void *p)
{
	struct tagSEdrvctl *pdrvctl;
	struct tagSEBasicBD *pCurBD;
	unsigned int ulCurrReadPtr, ulReadPtr;
	int iChannel;
	int len = 0;
	int i;
	unsigned char ucMyRetCode = 0;
	unsigned long ulIntStat;
	int istatus[2] = { 1, 2 };
	unsigned long ulflags;

	pdrvctl = p;
	if (!pdrvctl)
		return IRQ_HANDLED;

	read_lock_irqsave(&(pdrvctl->mr_lock), ulflags);
	HandleRead32(pdrvctl, SE_REG_STS, &ulIntStat);
	read_unlock_irqrestore(&(pdrvctl->mr_lock), ulflags);
	if ((!(ulIntStat & INT_STAT_DMA_MASK)) || (ulIntStat == 0xffffffff))
		return IRQ_HANDLED;

	i = 0;
loop:
	if (ulIntStat & istatus[i]) {
		if (i == 0) {
			read_lock_irqsave(&(pdrvctl->mr_lock), ulflags);
			HandleWrite32(pdrvctl, SE_INT_CLR, 1);
			HandleRead32(pdrvctl, SE_REG_RQRP0, &ulReadPtr);
			read_unlock_irqrestore(&(pdrvctl->mr_lock), ulflags);
			iChannel = 0;
		} else {
			read_lock_irqsave(&(pdrvctl->mr_lock), ulflags);
			HandleWrite32(pdrvctl, SE_INT_CLR, 2);
			HandleRead32(pdrvctl, SE_REG_RQRP1, &ulReadPtr);
			read_unlock_irqrestore(&(pdrvctl->mr_lock), ulflags);
			iChannel = 1;
		}
	} else {
		if (i == 0) {
			i++;
			goto loop;
		} else
			return IRQ_HANDLED;
	}
	ulCurrReadPtr = pdrvctl->ulCurrReadPtr[iChannel];
loop2:
	if (ulCurrReadPtr != ulReadPtr) {
		pCurBD = &((struct tagSEBasicBD *)(pdrvctl->ulBDMemBase[iChannel]))
				 [ulCurrReadPtr];
		if ((pCurBD->ucRetCode == 0x0f) ||
		    ((pCurBD->ucFlag & 0x8) != 0x8)) {
			goto loop2;
		} else {
			if (pdrvctl->ulInputPtr[iChannel][ulCurrReadPtr] ==
			    pdrvctl->ulOutputPtr[iChannel][ulCurrReadPtr]) {
				if (pdrvctl->ulOutputPtr[iChannel]
							[ulCurrReadPtr]) {
					len = (*(pdrvctl->pusOutlen
							 [iChannel]
							 [ulCurrReadPtr])) >=
							      pdrvctl->usInlen
								      [iChannel]
								      [ulCurrReadPtr] ?
						      (*(pdrvctl->pusOutlen
								 [iChannel]
								 [ulCurrReadPtr])) :
						      pdrvctl->usInlen
							      [iChannel]
							      [ulCurrReadPtr];
					dma_unmap_single(
						pdrvctl->pdev,
						(unsigned long)(pdrvctl->ulOutputPtr
									[iChannel]
									[ulCurrReadPtr]),
						len, DMA_BIDIRECTIONAL);
					pCurBD->ulOutputLPtr = 0;
					pCurBD->ulOutputHPtr = 0;
					pCurBD->ulInputHPtr = 0;
					pCurBD->ulInputLPtr = 0;
					pdrvctl->ulOutputPtr[iChannel]
							    [ulCurrReadPtr] = 0;
				}
			} else {
				if (pdrvctl->ulOutputPtr[iChannel]
							[ulCurrReadPtr]) {
					dma_unmap_single(
						pdrvctl->pdev,
						(unsigned long)(pdrvctl->ulOutputPtr
									[iChannel]
									[ulCurrReadPtr]),
						*(pdrvctl->pusOutlen
							  [iChannel]
							  [ulCurrReadPtr]),
						DMA_FROM_DEVICE);
					/* WMB */
					smp_wmb();
					pdrvctl->ulOutputPtr[iChannel]
							    [ulCurrReadPtr] = 0;
				}
				if (pdrvctl->ulInputPtr[iChannel]
						       [ulCurrReadPtr]) {
					dma_unmap_single(
						pdrvctl->pdev,
						(unsigned long)(pdrvctl->ulInputPtr
									[iChannel]
									[ulCurrReadPtr]),
						pdrvctl->usInlen[iChannel]
								[ulCurrReadPtr],
						DMA_TO_DEVICE);
					pdrvctl->ulInputPtr[iChannel]
							   [ulCurrReadPtr] = 0;
				}
			}
			ucMyRetCode = pCurBD->ucRetCode;
			memcpy(pdrvctl->pucRetCode[iChannel][ulCurrReadPtr],
			       &ucMyRetCode, 1);
			if (pCurBD->ucRetCode != SE_OK)
				pr_info("\nstatus %x\n", pCurBD->ucRetCode);
#ifdef CONFIG_MIPS
			atomic_sub(((*(pdrvctl->pusOutlen[iChannel]
							 [ulCurrReadPtr])) +
				    SE_FILL_LEN),
				   &g_sendtotallen);
#endif
			if ((pdrvctl->ikernel)[iChannel][ulCurrReadPtr] != 0) {
				if (pdrvctl->pcallback[iChannel][ulCurrReadPtr]) {
					struct ST_INT_MESSAGE *intmessage = NULL;
					unsigned long ulflags1;

					intmessage =
						kmalloc(sizeof(struct ST_INT_MESSAGE),
							GFP_ATOMIC);
					if (!intmessage)
						return IRQ_HANDLED;
					intmessage->pcallback =
						pdrvctl->pcallback[iChannel]
								  [ulCurrReadPtr];
					intmessage->pParma =
						pdrvctl->pParma[iChannel]
							       [ulCurrReadPtr];
					spin_lock_irqsave(&g_reclistlock,
							  ulflags1);
					wst_Pushback_Que(&g_RecQueueContainer,
							 intmessage);
					spin_unlock_irqrestore(&g_reclistlock,
							       ulflags1);
					queue_work(g_workrecqueue, &g_recwork);
				}
			} else {
				complete(pdrvctl->stsemphore[iChannel]
							    [ulCurrReadPtr]);
			}
			ulCurrReadPtr =
				((ulCurrReadPtr + 1) & (SE_BDQUEUE_LEN - 1));
			pdrvctl->ulCurrReadPtr[iChannel] = ulCurrReadPtr;
			pdrvctl->ulCurrBdReadPtr[iChannel] = ulCurrReadPtr;
			if (pdrvctl->sema.count <= 0)
				up(&(pdrvctl->sema));
		}
	}
	if (i == 0) {
		i++;
		goto loop;
	} else
		return IRQ_HANDLED;

	return IRQ_HANDLED;
}

static int se_userrelease(struct inode *inode, struct file *file)
{
	return SE_OK;
}

ssize_t se_kernelwrite(unsigned char *pInPtr, unsigned short usInlen,
		       unsigned char *pOutPtr, unsigned short *pusOutlen,
		       unsigned char ucFlag, unsigned char *pucRetCode,
		       PSECallBackfn pcallback, void *pParma)
{
	int iret;
	struct tagSEdrvctl *pdrvctl;
	int iChannel;
	unsigned char ucOpCode;
	struct tagSWCOMMUDATA CommuData;

	pdrvctl = g_psechipDrvCtrl;

	switch (ucFlag) {
	case WST_GO_CHANNEL2: {
		CommuData.pucInbuf = pInPtr;
		CommuData.pucOutbuf = pOutPtr;
		CommuData.usFlags = 0;
		CommuData.usInputLen = usInlen;
		CommuData.usOutputLen = *pusOutlen;
		CommuData.usReserve = 0;
		if (down_interruptible(&g_lowsema) == -EINTR)
			return -EINTR;
		iret = wst_low_channel_userwrite_op(pdrvctl, &CommuData, 1);
		up(&g_lowsema);
		return iret;
	}
	case WST_GO_CHANNEL0:
		if (pcallback == NULL)
			return WST_SE_PARAM_ERROR;
		if (usInlen == 0)
			return -EINVAL;
		ucFlag = 0;
		if (usInlen != 0) {
			if (usInlen > DMA_BUFSIZE)
				return -EINVAL;
			ucFlag = INPUT_VALID;
			if (*pusOutlen)
				ucFlag |= OUTPUT_VALID;
		}
		iChannel = 0;
		ucOpCode = 0x0;
		break;
	case WST_GO_CHANNEL1:
		if (pcallback == NULL)
			return WST_SE_PARAM_ERROR;
		if (usInlen == 0)
			return -EINVAL;
		ucFlag = 0;
		if (usInlen != 0) {
			if (usInlen > DMA_BUFSIZE)
				return -EINVAL;
			ucFlag = INPUT_VALID;
			if (*pusOutlen)
				ucFlag |= OUTPUT_VALID;
		}
		iChannel = 1;
		ucOpCode = 0x0;
		break;
	default:
		return -EINVAL;
	}
	iret = se_hardtrans(pdrvctl, pInPtr, usInlen, pOutPtr, pusOutlen,
			    iChannel, ucFlag, ucOpCode, pucRetCode, pcallback,
			    pParma, 1, NULL);
	if (iret == -1)
		return -EIO;

	return SE_OK;
}
EXPORT_SYMBOL(se_kernelwrite);

static long se_userioctl(struct file *filp, u_int cmd, u_long arg)
{
	long iret = SE_OK;
	struct tagSEdrvctl *pdrvctl = g_psechipDrvCtrl;
	unsigned long ulvalue;

	HandleRead64(pdrvctl, 0x120, &ulvalue);
	pr_info("read reg value is 0x%lx in offset 120\n", ulvalue);
	HandleRead64(pdrvctl, 0x118, &ulvalue);
	pr_info("read reg value is 0x%lx in offset 118\n", ulvalue);

	return iret;
}

static const struct file_operations SE_fops = { .owner = THIS_MODULE,
						.write = se_userwrite,
						.open = se_useropen,
						.release = se_userrelease,
						.unlocked_ioctl = se_userioctl,
						.compat_ioctl = se_userioctl };

int se_chip_load(void)
{
	int iRes = SE_OK;

	if (g_isechip_Major >= 0)
		return WST_SE_HAS_OPEN;
	g_psechipDrvCtrl = NULL;
	iRes = se_init_dma_buf(DMA_BUFSIZE, CTL_DMABUFNUM);
	if (iRes != SE_OK)
		return WST_SE_ERROR_MALLOC;
	iRes = register_chrdev(0, CRYNAME, &SE_fops);
	if (iRes < 0)
		goto EXIT;

	g_isechip_Major = iRes;
	iRes = 0;
	g_psecclass = class_create(CRYNAME);
	if (IS_ERR(g_psecclass)) {
		iRes = PTR_ERR(g_psecclass);
		goto EXIT;
	}
	g_psecdev = device_create(g_psecclass, NULL, MKDEV(g_isechip_Major, 0),
				  NULL, CRYNAME);
	if (IS_ERR(g_psecdev)) {
		iRes = PTR_ERR(g_psecdev);
		goto EXIT;
	}
	iRes = wst_init();
	if (iRes != SE_OK)
		goto EXIT;

	sema_init(&g_lowsema, 1);
	sema_init(&g_lowirqsema, 0);
	atomic_set(&g_sendtotallen, 0);
	g_pCacheInBuf = (unsigned char *)__get_free_page(GFP_DMA);
	if (IS_ERR(g_pCacheInBuf)) {
		iRes = PTR_ERR(g_pCacheInBuf);
		goto EXIT;
	}
	g_pCacheOutBuf = (unsigned char *)__get_free_page(GFP_DMA);
	if (IS_ERR(g_pCacheOutBuf)) {
		iRes = PTR_ERR(g_pCacheOutBuf);
		goto EXIT;
	}

	g_worksendqueue =
		alloc_workqueue("seworksendqueue",
				WQ_MEM_RECLAIM | __WQ_ORDERED | WQ_UNBOUND, 1);
	if (IS_ERR(g_worksendqueue)) {
		iRes = PTR_ERR(g_worksendqueue);
		goto EXIT;
	}
	g_workrecqueue = alloc_workqueue("seworkrecqueue",
					 WQ_MEM_RECLAIM | __WQ_ORDERED, 0);
	if (IS_ERR(g_workrecqueue)) {
		iRes = PTR_ERR(g_workrecqueue);
		goto EXIT;
	}
	INIT_WORK(&g_recwork, globalmem_do_rec_op);
	INIT_WORK(&g_sendwork, globalmem_do_send_op);
	pr_info("this driver version is %s\n", DRIVER_VERSION);

	return SE_OK;
EXIT:
	se_del_dma_buf();
	if (g_pCacheInBuf) {
		free_page((unsigned long)g_pCacheInBuf);
		g_pCacheInBuf = NULL;
	}
	if (g_pCacheOutBuf) {
		free_page((unsigned long)g_pCacheOutBuf);
		g_pCacheOutBuf = NULL;
	}
	if (g_worksendqueue) {
		destroy_workqueue(g_worksendqueue);
		g_worksendqueue = NULL;
	}
	if (g_workrecqueue) {
		destroy_workqueue(g_workrecqueue);
		g_workrecqueue = NULL;
	}
	wst_clear();
	if (g_psecdev) {
		device_unregister(g_psecdev);
		g_psecdev = NULL;
	}
	if (g_psecclass) {
		class_destroy(g_psecclass);
		g_psecclass = NULL;
	}
	if (g_isechip_Major >= 0) {
		unregister_chrdev(g_isechip_Major, CRYNAME);
		g_isechip_Major = -1;
	}
	return iRes;
}

void se_chip_unload(void)
{
	struct tagSEdrvctl *pdrvctl = NULL;

	pdrvctl = g_psechipDrvCtrl;

	up(&pdrvctl->sema);
	if (g_pCacheInBuf) {
		free_page((unsigned long)g_pCacheInBuf);
		g_pCacheInBuf = NULL;
	}
	if (g_pCacheOutBuf) {
		free_page((unsigned long)g_pCacheOutBuf);
		g_pCacheOutBuf = NULL;
	}
	if (g_worksendqueue) {
		cancel_work_sync(&g_sendwork);
		destroy_workqueue(g_worksendqueue);
		g_worksendqueue = NULL;
	}
	if (g_workrecqueue) {
		cancel_work_sync(&g_recwork);
		destroy_workqueue(g_workrecqueue);
		g_workrecqueue = NULL;
	}
	wst_clear();
	if (g_psecdev) {
		device_destroy(g_psecclass, MKDEV(g_isechip_Major, 0));
		g_psecdev = NULL;
	}
	if (g_psecclass) {
		class_destroy(g_psecclass);
		g_psecclass = NULL;
	}
	if (g_isechip_Major >= 0) {
		unregister_chrdev(g_isechip_Major, CRYNAME);
		g_isechip_Major = -1;
	}
	se_del_dma_buf();
}

static int loongson_cryp_get_irq(struct platform_device *pdev, char *pat)
{
	int i;
	struct resource *res = pdev->resource;

	for (i = 0; i < pdev->num_resources; i++) {
		if (strcmp(res[i].name, pat) == 0)
			return res[i].start;
	}
	return -1;
}
static int loongson_cryp_probe(struct platform_device *pdev)
{
	int i;

	if (ACPI_COMPANION(&pdev->dev)) {
		se_irq[0].irq = platform_get_irq(pdev, 1);
		se_irq[1].irq = platform_get_irq(pdev, 0);
	} else {
		for (i = 0; i < pdev->num_resources; i++) {
			se_irq[i].irq =
				loongson_cryp_get_irq(pdev, se_irq[i].pat);
			if (se_irq[i].irq < 0) {
				pr_warn("ERROR:sedriver get irq failed\n");
				return -1;
			}
		}
	}

	return se_chip_load();
}
static int loongson_cryp_remove(struct platform_device *pdev)
{
	se_chip_unload();
	return 0;
}
static int loongson_cryp_suspend(struct platform_device *pdev,
				 pm_message_t state)
{
	g_suspend = 1;
	cancel_work_sync(&g_recwork);
	cancel_work_sync(&g_sendwork);
	flush_work(&g_recwork);
	flush_work(&g_sendwork);
	se_chip_unload();
	return 0;
}

static int loongson_cryp_resume(struct platform_device *pdev)
{
	int i;

	g_suspend = 0;
	g_worksendqueue = NULL;
	g_workrecqueue = NULL;
	g_isechip_Major = -1;
	g_pCacheInBuf = NULL;
	g_pCacheOutBuf = NULL;
	g_iUseIntr = 1;
	spin_lock_init(&g_reclistlock);
	spin_lock_init(&g_sendlistlock);
	spin_lock_init(&g_getdmabuflock);

	for (i = 0; i < pdev->num_resources; i++) {
		se_irq[i].irq = loongson_cryp_get_irq(pdev, se_irq[i].pat);

		if (se_irq[i].irq < 0) {
			pr_warn("ERROR:sedriver get irq failed\n");
			return -1;
		}
	}
	se_chip_load();
	return 0;
}

static const struct acpi_device_id loongson_cryp_acpi_match[] = { { "LOON0003" },
								  {} };
MODULE_DEVICE_TABLE(acpi, loongson_cryp_acpi_match);

static struct platform_driver loongson_cryp_driver = {
	.probe		= loongson_cryp_probe,
	.remove		= loongson_cryp_remove,
	.suspend	= loongson_cryp_suspend,
	.resume		= loongson_cryp_resume,
	.driver		= {
			.name = "loongson3_crypto",
			.acpi_match_table = ACPI_PTR(loongson_cryp_acpi_match),
	},
};

static int __init initmodule(void)
{
	return platform_driver_register(&loongson_cryp_driver);
}

static void __exit exitmodule(void)
{
	platform_driver_unregister(&loongson_cryp_driver);
}

module_init(initmodule);
module_exit(exitmodule);

MODULE_ALIAS("platform:loongson3_crypto");
MODULE_AUTHOR("dcm");
MODULE_DESCRIPTION("se encryption chip driver Co westone");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
