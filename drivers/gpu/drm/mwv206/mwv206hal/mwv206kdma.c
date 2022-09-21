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
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include "gljos.h"
#include "mwv206reg.h"
#include "mwv206kdebug.h"
#include "mwv206dev.h"
#include "mwv206kconfig.h"
#include "mwv206ioctl.h"

#define __PRINT_TRANSE_SIZE_REG__   { \
	int val; \
	PCI_READ_U32(dev, 0xA78, &val); \
	V206KDEBUG002("a78 = 0x%x\n", val); \
}

int FUNC206HAL320(V206DEV025 *pDev, int arg)
{
	if (arg) {
		return FUNC206LXDEV116(pDev->V206DEV090.V206DEV166);
	} else {
		return FUNC206LXDEV136(pDev->V206DEV090.V206DEV166);
	}
}

int FUNC206HAL319(V206DEV025 *pDev, int op, int chan, int arg)
{
	if (arg) {
		return FUNC206LXDEV116(pDev->V206DEV090.V206DEV167[chan][op]);
	} else {
		return FUNC206LXDEV136(pDev->V206DEV090.V206DEV167[chan][op]);
	}
}

jjuint32 FUNC206HAL323(V206DEV025 *pDev, jjuint32 startaddr)
{
	int i, j;
	int listsize;
	int chancnt;
	jjuint32 endaddr;

#ifdef MWV206_DMA_USE_MULTI_CHAN
	chancnt = 4;
#else
	chancnt = 1;
#endif

	listsize = ((4096) + 1) * 6 * 4;
	listsize = (listsize + 31) & 0xFFFFFFE0;
	endaddr = (startaddr + 31) & 0xFFFFFFE0;

	for (i = 0; i < chancnt; i++) {
		for (j = 0; j < 2; j++) {
			pDev->V206DEV090.V206DEV170[i][j] = endaddr;
			endaddr += listsize;
		}
	}
	return endaddr;
}



int FUNC206HAL321(V206DEV025 *pDev)
{
	int i, j;
	int chancnt;

#ifdef MWV206_DMA_USE_MULTI_CHAN
	chancnt = 4;
#else
	chancnt = 1;
#endif

	pDev->V206DEV090.V206DEV166 = FUNC206LXDEV118();
	for (i = 0; i < chancnt; i++) {
		for (j = 0; j < 2; j++) {
			pDev->V206DEV090.V206DEV167[i][j] = FUNC206LXDEV118();
			pDev->V206DEV090.V206DEV168[i][j] = FUNC206LXDEV009(&(pDev->V206DEV103->dev),
					&pDev->V206DEV090.V206DEV169[i][j], MWV206_MAX_DMA_SIZE);
			if (pDev->V206DEV090.V206DEV168[i][j] == NULL) {
				V206KDEBUG002("[ERROR](i = %d, j = %d): malloc dma address failure.\n", i, j);
				FUNC206LXDEV128(MWV206CMDTIMEOUT);
				return -1;
			}
		}
	}
	return 0;
}

void FUNC206HAL324(V206DEV025 *pDev)
{
	int i, j;
	int chancnt;
	void *vaddr;

#ifdef MWV206_DMA_USE_MULTI_CHAN
	chancnt = 4;
#else
	chancnt = 1;
#endif

	for (i = 0; i < chancnt; i++) {
		for (j = 0; j < 2; j++) {
			vaddr = pDev->V206DEV090.V206DEV168[i][j];
			if (vaddr) {
				FUNC206LXDEV100(&(pDev->V206DEV103->dev), MWV206_MAX_DMA_SIZE,
						vaddr, pDev->V206DEV090.V206DEV169[i][j]);
			}
		}
	}
}

static void FUNC206HAL322(struct pci_dev *dev)
{

	PCI_WRITE_U32(dev, 0x99C, 0);
	PCI_WRITE_U32(dev, 0x99C, 1);

	PCI_WRITE_U32(dev, 0x97C, 0);
	PCI_WRITE_U32(dev, 0x97C, 1);
}

static int FUNC206HAL409(V206DEV025 *pDev, int rw, int chan)
{
	GLJ_TICK tick;
	int timeout, scratch;
	int intren, done;
	int intID;
	int statusReg, clearReg, status;
	const char *str;
	struct pci_dev *dev;

	dev = pDev->V206DEV103;

	intID = MWV206KINTR_PCIEX16_DMA;
	V206DEV005("intID = 0x%x\n", (int)intID);

	timeout = MWV206CMDTIMEOUT;

	str = rw ? "write" : "read";
	V206DEV005("\nwaiting for dma %d transfer(%s) done\n", chan, str);

	intren = FUNC206HAL368(pDev, intID);
	if (rw == V206IOCTL020) {
		statusReg = 0x9BC;
		clearReg = 0x9C8;
	} else {
		statusReg = 0xA10;
		clearReg = 0xA1C;
	}

	if (intren) {
		int ret, eventID;
		V206DEV005("dma transfer interrupt mode.\n");

		if (rw == 0) {
			eventID = chan;
		} else {
			eventID = chan + 4;
		}
		V206DEV005("eventID = %d\n", eventID);
		ret = FUNC206LXDEV105(pDev->V206DEV064[MWV206KINTR_COUNT + eventID], timeout);
		if (ret != 0) {
			V206KDEBUG002("wait ford dma tranfer interrupt timeout (pending 0x%x: 0x%08x, status 0x%x: 0x%08x)\n",
					((0x406000) + 0x0034), V206DEV001(((0x406000) + 0x0034)),
					((0x406000) + 0x0024), V206DEV001(((0x406000) + 0x0024)));

			PCI_READ_U32(dev, statusReg, &status);
			V206KDEBUG002("wait ford dma tranfer interrupt timeout (status 0x%x: 0x%08x)\n",
					statusReg, status);
			FUNC206HAL322(dev);
			return -1;
		}
	} else {

		V206DEV005("dma transfer polling mode.\n");

		mwv206_timed_do (tick, scratch, timeout) {

			PCI_READ_U32(dev, statusReg, &status);

			done = (status & (1 << chan)) && !(status & (1 << (16 + chan)));
			if (done) {
				break;
			}
		}

		if (!done) {
			V206KDEBUG002("wait for dma chan %d transfer(%s) done timeout(0x%x: 0x%08x)\n",
					chan, str, statusReg, status);
			FUNC206HAL322(dev);
			return -1;
		} else {
			V206DEV005("wait for dma chan %d transfer(%s) done successfully.", chan, str);

			PCI_WRITE_U32(dev, clearReg, (1 << chan) | (1 << (16 + chan)));
		}
	}

	return 0;
}

int FUNC206HAL359(int intrsrc, V206DEV025 *pDev)
{
	GLJ_TICK tick;
	int pending = 0, flag = 0, val;
	int status, status1;
	int i, tmp, tmp2, scratch;
	struct pci_dev *dev;

	V206DEV005("in interrupt service program.");

	dev = pDev->V206DEV103;

	val = 1 << MWV206KINTR_PCIEX16_DMA;

	status = V206DEV001(0xC000B8);
	V206DEV005("status = 0x%x\n\n", status);


	V206DEV002(0xC000B8, 0xFF);
	V206DEV002(((0x406000) + 0x0044), val);

	mwv206_timed_do (tick, scratch, MWV206CMDTIMEOUT) {
		pending = V206DEV001(((0x406000) + 0x0034));
		if (((pending & val) == 0)) {
			flag = 1;
			break;
		} else {
			V206DEV002(0xC000B8, 0xFF);
			V206DEV002(((0x406000) + 0x0044), val);
		}
	}


	if (flag == 0) {
		V206KDEBUG002("\n\nMwv206Interrupt: clear interrupt error!!!\n"
				"pending register = [0x%x]! \n\n\n\n", pending);
		return 0;
	}


	PCI_READ_U32(dev, 0x9BC, &status1);
	status = V206DEV001(0xC000B8);
	V206DEV005("status = 0x%x, status1 = 0x%x\n\n", status, status1);
	for (i = 0; i < 4; i++) {
		tmp = 1 << i;
		 {
			if (status1 & tmp) {
				V206DEV005("DMA read channel %d finished.\n", i);
				PCI_WRITE_U32(dev, 0x9C8, tmp);
				FUNC206LXDEV103(pDev->V206DEV064[MWV206KINTR_COUNT + i]);
			}
			if (status1 & (1 << (16 + i))) {
				PCI_WRITE_U32(dev, 0x97C, 0);
				PCI_WRITE_U32(dev, 0x97C, 1);
				V206KDEBUG002("DMA Write Failure, DMA Write channels are reset.\n\n");
			}
		}
	}


	PCI_READ_U32(dev, 0xA10, &status1);
	V206DEV005("status = 0x%x, status1 = 0x%x\n\n", status, status1);
	for (i = 4; i < 8; i++) {
		tmp = 1 << i;
		tmp2 = 1 << (i - 4);
		{
			if (status1 & tmp2) {
				V206DEV005("DMA write channel %d finished.\n", i);
				PCI_WRITE_U32(dev, 0xA1C, tmp2);
				FUNC206LXDEV103(pDev->V206DEV064[MWV206KINTR_COUNT + i]);
			}
			if (status1 & (1 << (16 + (i - 4)))) {
				PCI_WRITE_U32(dev, 0x99C, 0);
				PCI_WRITE_U32(dev, 0x99C, 1);
				V206KDEBUG002("DMA Write Failure, DMA Write channels are reset.\n\n");
			}
		}
	}
	return IRQ_HANDLED;
}


static int FUNC206HAL295(V206DEV025 *pDev,
		int chan, unsigned long paddr, unsigned int V206IOCTLCMD009, int size)
{
	struct pci_dev *dev;
	int ret = 0;

	dev = pDev->V206DEV103;

	FUNC206HAL320(pDev, 1);


	PCI_WRITE_U32(dev, 0x99C, 1);


	PCI_WRITE_U32(dev, 0xA18, 0);


	PCI_WRITE_U32(dev, 0xA6C, (1 << 31) | chan);



	PCI_WRITE_U32(dev, 0xA70, 0x04000008);


	PCI_WRITE_U32(dev, 0xA78, size);


	PCI_WRITE_U32(dev, 0xA7C, paddr);
	PCI_WRITE_U32(dev, 0xA80, 0);


	PCI_WRITE_U32(dev, 0xA84, V206IOCTLCMD009);
	PCI_WRITE_U32(dev, 0xA88, 0);



	PCI_WRITE_U32(dev, 0x9A0, chan);

	FUNC206HAL320(pDev, 0);

	return ret;
}

static int FUNC206HAL294(V206DEV025 *pDev,
		int chan, unsigned long paddr, unsigned int V206IOCTLCMD009, int size)
{
	struct pci_dev *dev;
	int ret = 0;

	dev = pDev->V206DEV103;

	FUNC206HAL320(pDev, 1);


	PCI_WRITE_U32(dev, 0x97C, 1);


	PCI_WRITE_U32(dev, 0x9C4, 0);


	PCI_WRITE_U32(dev, 0xA6C, chan);



	PCI_WRITE_U32(dev, 0xA70, 0x04000008);


	PCI_WRITE_U32(dev, 0xA78, size);


	PCI_WRITE_U32(dev, 0xA7C, V206IOCTLCMD009);
	PCI_WRITE_U32(dev, 0xA80, 0);


	PCI_WRITE_U32(dev, 0xA84, (unsigned int)paddr);
	PCI_WRITE_U32(dev, 0xA88, 0);



	PCI_WRITE_U32(dev, 0x980, chan);

	FUNC206HAL320(pDev, 0);
	return ret;
}

int FUNC206HAL325(V206DEV025 *pDev, int op,
		int chan, unsigned int V206IOCTLCMD009, unsigned char *V206DEV031, int size)
{
	int ret;
	unsigned int mapaddr;
	unsigned long paddr;
	void *vaddr = NULL;

	if (pDev->V206DEV090.dma_is_err) {
		ret = -3;
		goto __ERROR2;
	}


	mapaddr = mwv206GetAXIAddr(V206IOCTLCMD009);


	vaddr = pDev->V206DEV090.V206DEV168[chan][op];
	paddr = pDev->V206DEV090.V206DEV169[chan][op];
	V206DEV005("mwv206addr = 0x%x, vaddr = %p, dma_handle = 0x%lx, size = %d\n",
			V206IOCTLCMD009, vaddr, dma_handle, size);

	FUNC206HAL319(pDev, op, chan, 1);

	switch (op) {
	case V206IOCTL020:



		if (FUNC206HAL294(pDev, chan, paddr, mapaddr, size) != 0) {
			ret = -2;
			goto FUNC206HAL020;
		}


		V206DEV005("waiting for dma read finished\n");
		if (FUNC206HAL409(pDev, V206IOCTL020, chan) != 0) {
			ret = -3;
			goto FUNC206HAL020;

		}


		if (FUNC206HAL314(pDev, V206IOCTLCMD009) != 0) {
			ret = -4;
			goto FUNC206HAL020;
		}


		if (copy_to_user((void __user *)V206DEV031, (void *)vaddr, size) != 0) {
			V206KDEBUG002("###ERR: copy_to_user error.\n");
			ret = -5;
			goto FUNC206HAL020;
		}
		V206DEV005("DMA read finished\n");
		break;
	case V206IOCTL021:

		ret = copy_from_user((void *)vaddr, (void __user *)V206DEV031, size);
		if (ret != 0) {
			V206KDEBUG002("###ERR: copy_from_user error, ret = %d.\n", ret);
			ret = -5;
			goto FUNC206HAL020;
		}


		if (FUNC206HAL295(pDev, chan, paddr, mapaddr, size) != 0) {
			ret = -2;
			goto FUNC206HAL020;
		}


		V206DEV005("waiting for dma write finished\n");
		if (FUNC206HAL409(pDev, V206IOCTL021, chan) != 0) {
			ret = -3;
			goto FUNC206HAL020;
		}


		if (FUNC206HAL314(pDev, V206IOCTLCMD009) != 0) {
			ret = -4;
			goto FUNC206HAL020;
		}

		V206DEV005("DMA write finished\n");

		break;
	default: {
			 ret = -101;
		 } break;
	}

	V206DEV005("[DONE]mwv206_dmamem_access 0x%x.\n", op);
	FUNC206HAL319(pDev, op, chan, 0);
	return 0;

FUNC206HAL020:
	FUNC206HAL319(pDev, op, chan, 0);
__ERROR2:
	if ((ret == -2 || ret == -3) && (FUNC206HAL314(pDev, V206IOCTLCMD009) == 0)) {
		if (!pDev->V206DEV090.dma_is_err) {
			pDev->V206DEV090.dma_is_err = 1;
			V206KDEBUG003("[INFO] mwv206 dma switch to mem_rw\n");
		}
		ret = mwv206_mem_rw(pDev, op, (u8 *)V206DEV031, (u32)V206IOCTLCMD009, size);
	}
	return ret;
}

int V206IOCTL126(V206DEV025 *pDev, long arg)
{
	V206IOCTL163 *memaccess;
	int ret;
	int size, chan, onesize;
	unsigned char *V206DEV031;
	unsigned int V206IOCTLCMD009;
	int op, i;

	ret = 0;
	memaccess = (V206IOCTL163 *)arg;

	size = memaccess->size;


#ifdef MWV206_DMA_USE_MULTI_CHAN
	chan = memaccess->chan;
#else
	chan = 0;
#endif


	V206DEV031 = (unsigned char *)FUNC206HAL071(memaccess->V206DEV031, memaccess->memaddrHi);
	V206DEV005("memaddr = 0x%lx(0x%x 0x%x)\n", (unsigned long)V206DEV031,
			memaccess->V206DEV031, memaccess->memaddrHi);


	V206IOCTLCMD009 = memaccess->V206IOCTLCMD009;

	if (!addr_in_fb_range(pDev, V206IOCTLCMD009) || !size_in_fb_range(pDev, V206IOCTLCMD009, size)) {
		V206KDEBUG002("[INFO] invalid dmamem_access, ignored:\n");
		V206KDEBUG002("[INFO] pid:%d mwv206addr:0x%x size:%d\n",
				current->pid, V206IOCTLCMD009, size);
		return -1;
	}


	op = memaccess->op;
	onesize = MWV206_MAX_DMA_SIZE;
	if (unlikely(op == V206IOCTL020)) {
		int readbytes;
		readbytes = pDev->V206DEV090.V206DEV171;
		onesize = onesize > readbytes ? readbytes : onesize;
	}

	for (i = 0; i < size / onesize; i++) {
		ret = FUNC206HAL325(pDev, op, chan, V206IOCTLCMD009, V206DEV031, onesize);
		if (unlikely(ret != 0)) {
			return -2;
		}
		V206IOCTLCMD009 += onesize;
		V206DEV031 += onesize;
	}

	size = size % onesize;

	if (size) {
		ret = FUNC206HAL325(pDev, op, chan, V206IOCTLCMD009, V206DEV031, size);
		if (unlikely(ret != 0)) {
			return -2;
		}
	}
	return 0;
}

int FUNC206HAL326(V206DEV025 *pDev, int op,
		int chan, unsigned int V206IOCTLCMD009, unsigned char *V206DEV031, int size)
{
	int ret;
	unsigned int mapaddr;
	unsigned long paddr;
	void *vaddr = NULL;

	if (pDev->V206DEV090.dma_is_err) {
		ret = -3;
		goto __ERROR2;
	}


	mapaddr = mwv206GetAXIAddr(V206IOCTLCMD009);


	vaddr = pDev->V206DEV090.V206DEV168[chan][op];
	paddr = pDev->V206DEV090.V206DEV169[chan][op];
	V206DEV005("mwv206addr = 0x%x, vaddr = %p, dma_handle = 0x%lx, size = %d\n",
			V206IOCTLCMD009, vaddr, dma_handle, size);

	FUNC206HAL319(pDev, op, chan, 1);

	switch (op) {
	case V206IOCTL020:



		if (FUNC206HAL294(pDev, chan, paddr, mapaddr, size) != 0) {
			ret = -2;
			goto FUNC206HAL020;
		}


		V206DEV005("waiting for dma read finished\n");
		if (FUNC206HAL409(pDev, V206IOCTL020, chan) != 0) {
			ret = -3;
			goto FUNC206HAL020;
		}

		if (FUNC206HAL314(pDev, V206IOCTLCMD009) != 0) {
			ret = -4;
			goto FUNC206HAL020;
		}


		memcpy(V206DEV031, (void *)vaddr, size);
		V206DEV005("DMA read finished\n");
		break;
	case V206IOCTL021:
		memcpy((void *)vaddr, V206DEV031, size);

		if (FUNC206HAL295(pDev, chan, paddr, mapaddr, size) != 0) {
			ret = -2;
			goto FUNC206HAL020;
		}


		V206DEV005("waiting for dma write finished\n");
		if (FUNC206HAL409(pDev, V206IOCTL021, chan) != 0) {
			ret = -3;
			goto FUNC206HAL020;
		}


		if (FUNC206HAL314(pDev, V206IOCTLCMD009) != 0) {
			ret = -4;
			goto FUNC206HAL020;
		}

		V206DEV005("DMA write finished\n");

		break;
	default:
		ret = -101;
		break;
	}

	V206DEV005("[DONE]%s 0x%x.\n", __FUNCTION__, op);
	FUNC206HAL319(pDev, op, chan, 0);
	return 0;

FUNC206HAL020:
	FUNC206HAL319(pDev, op, chan, 0);
__ERROR2:
	if (ret == -2 || ret == -3) {
		if (!pDev->V206DEV090.dma_is_err) {
			pDev->V206DEV090.dma_is_err = 1;
			V206KDEBUG003("[INFO] mwv206 dma_kern switch to memcpy\n");
		}
		ret = mwv206_memcpy_rw(pDev, op, V206DEV031, V206IOCTLCMD009, size);
	}
	return ret;
}

int FUNC206HAL327(V206DEV025 *pDev, int op,
		int chan, unsigned int V206IOCTLCMD009, unsigned char *V206DEV031, int size)
{
	int ret;
	int onesize;
	int i;

	ret = 0;

	onesize = MWV206_MAX_DMA_SIZE;
	if (unlikely(op == V206IOCTL020)) {
		int readbytes;
		readbytes = pDev->V206DEV090.V206DEV171;
		onesize = onesize > readbytes ? readbytes : onesize;
	}

	for (i = 0; i < size / onesize; i++) {
		ret = FUNC206HAL326(pDev, op, chan, V206IOCTLCMD009, V206DEV031, onesize);
		if (unlikely(ret != 0)) {
			return -2;
		}
		V206IOCTLCMD009 += onesize;
		V206DEV031 += onesize;
	}

	size = size % onesize;

	if (size) {
		ret = FUNC206HAL326(pDev, op, chan, V206IOCTLCMD009, V206DEV031, size);
		if (unlikely(ret != 0)) {
			return -2;
		}
	}
	return 0;
}

static int FUNC206HAL310(V206DEV025 *pDev,
		unsigned long srcphyaddr, unsigned int V206KG2D030,
		unsigned long dstphyaddr, unsigned int V206KG2D013,
		int width, int height, unsigned int linkaddr, unsigned int linkmapaddr)
{
	unsigned int linksize;
	unsigned int *linkcpuaddr, *p;
	int i;


	linksize = (height + 1) * 6 * 4;
	linkcpuaddr = (unsigned int *)FUNC206LXDEV010(linksize);
	if (linkcpuaddr == 0) {
		V206KDEBUG002("malloc linkcpu addr error.\n");
		return -1;
	}
	p = linkcpuaddr;


	for (i = 0; i < height - 1; i++) {
		*p++ = 1;
		*p++ = width;
		*p++ = ((unsigned long)srcphyaddr & 0xffffffff);
		*p++ = ((unsigned long)srcphyaddr >> 16 >> 16);
		*p++ = ((unsigned long)dstphyaddr & 0xffffffff);
		*p++ = ((unsigned long)dstphyaddr >> 16 >> 16);
		srcphyaddr += V206KG2D030;
		dstphyaddr += V206KG2D013;
	}


	*p++ = 9;
	*p++ = width;
	*p++ = ((unsigned long)srcphyaddr & 0xffffffff);
	*p++ = ((unsigned long)srcphyaddr >> 16 >> 16);
	*p++ = ((unsigned long)dstphyaddr & 0xffffffff);
	*p++ = ((unsigned long)dstphyaddr >> 16 >> 16);
	srcphyaddr += V206KG2D030;
	dstphyaddr += V206KG2D013;


	*p++ = 6;
	*p++ = 0;
	*p++ = linkmapaddr;
	*p++ = 0;

	V206DEV005("create linker from cpu done.\n");
	FUNC206HAL230(pDev, linkaddr, (unsigned char *)linkcpuaddr, linksize);
	V206DEV005("write linker from cpu to gpu 0x%x.\n", linkaddr);

	if (linkcpuaddr != 0) {
		FUNC206LXDEV120(linkcpuaddr);
	}
	return 0;
}

static int FUNC206HAL293(V206DEV025 *pDev,
		int chan, unsigned long linkphyaddr)
{
	struct pci_dev *dev;
	int ret = 0;

	dev = pDev->V206DEV103;

	FUNC206HAL320(pDev, 1);


	PCI_WRITE_U32(dev, 0x99C, 1);


	PCI_WRITE_U32(dev, 0xA18, 0);


	PCI_WRITE_U32(dev, 0xA6C, (1 << 31) | chan);



	PCI_WRITE_U32(dev, 0xA70, 0x04000308);


	PCI_WRITE_U32(dev, 0xA8C, linkphyaddr);
	PCI_WRITE_U32(dev, 0xA90, 0);



	PCI_WRITE_U32(dev, 0x9A0, chan);

	FUNC206HAL320(pDev, 0);
	return ret;
}

static int FUNC206HAL292(V206DEV025 *pDev,
		int chan, unsigned long linkphyaddr)
{
	struct pci_dev *dev;
	int ret = 0;

	dev = pDev->V206DEV103;

	FUNC206HAL320(pDev, 1);


	PCI_WRITE_U32(dev, 0x97C, 1);


	PCI_WRITE_U32(dev, 0x9C4, 0);


	PCI_WRITE_U32(dev, 0xA6C, chan);



	PCI_WRITE_U32(dev, 0xA70, 0x04000308);


	PCI_WRITE_U32(dev, 0xA8C, linkphyaddr);
	PCI_WRITE_U32(dev, 0xA90, 0);



	PCI_WRITE_U32(dev, 0x980, chan);

	FUNC206HAL320(pDev, 0);
	return ret;
}

static int FUNC206HAL329(V206DEV025 *pDev,
		int op, int chan,
		unsigned int V206IOCTLCMD009, int mwv206stride,
		unsigned char *V206DEV031, int cpustride, int width, int height)
{
	int ret;
	unsigned int mapaddr;
	unsigned long paddr;
	void *vaddr = NULL;

	unsigned int cpusize;
	unsigned int linkaddr = 0, linkmapaddr;

	if (pDev->V206DEV090.dma_is_err) {
		goto __ERROR2;
	}

	cpusize = width * height;

	mapaddr = mwv206GetAXIAddr(V206IOCTLCMD009);

	V206DEV005("\twidth= %d, height = %d, mwv206addr = 0x%x, mwv206stride=0x%x,\n"
			"cpuaddr = %p, cpustride = 0x%x.\n",
			width, height, V206IOCTLCMD009, mwv206stride, V206DEV031, cpustride);


	vaddr = pDev->V206DEV090.V206DEV168[chan][op];
	paddr = pDev->V206DEV090.V206DEV169[chan][op];
	V206DEV005("mwv206addr = 0x%x, vaddr = %p, dma_handle = 0x%lx\n",
			V206IOCTLCMD009, vaddr, paddr);

	FUNC206HAL319(pDev, op, chan, 1);

	switch (op) {
	case V206IOCTL020:

		linkaddr = pDev->V206DEV090.V206DEV170[chan][op];
		linkmapaddr = mwv206GetAXIAddr(linkaddr);

		ret = FUNC206HAL310(pDev, mapaddr, mwv206stride, paddr, width,
				width, height, linkaddr, linkmapaddr);
		if (ret != 0) {
			ret = -3;
			goto FUNC206HAL020;
		}


		if (FUNC206HAL314(pDev, linkaddr) != 0) {
			ret = -8;
			goto FUNC206HAL020;
		}

		if (FUNC206HAL292(pDev, chan, linkmapaddr) != 0) {
			ret = -4;
			goto FUNC206HAL020;
		}


		V206DEV005("waiting for dma read block finished\n");
		if (FUNC206HAL409(pDev, V206IOCTL020, chan) != 0) {
			ret = -5;
			goto FUNC206HAL020;
		}

		V206DEV005("!!!!vaddr(%p: 0x%lx) = 0x%x\n", vaddr, paddr, ((int *)vaddr)[0]);
		if (FUNC206HAL314(pDev, V206IOCTLCMD009) != 0) {
			ret = -6;
			goto FUNC206HAL020;
		}


		{
			int i;
			void *pvaddr;
			pvaddr = vaddr;
			for (i = 0; i < height; i++) {
				if (copy_to_user((void __user *)V206DEV031, pvaddr, width) != 0) {
					V206KDEBUG002("###ERR: copy_to_user (size = %d) error.\n", width);
					ret = -7;
					goto FUNC206HAL020;
				}
				V206DEV031 += cpustride;
				pvaddr += width;
			}
		}
		break;
	case V206IOCTL021:

		{
			int i;
			void *pvaddr;
			unsigned char *pmemaddr;

			pvaddr = vaddr;
			pmemaddr = V206DEV031;
			for (i = 0; i < height; i++) {
				ret = copy_from_user(pvaddr, (void __user *)pmemaddr, width);
				if (ret != 0) {
					V206KDEBUG002("###ERR: copy_from_user (ret = %d)(size = %d) error.\n", ret, width);
					ret = -3;
					goto FUNC206HAL020;
				}
				pmemaddr += cpustride;
				pvaddr += width;
			}
		}


		linkaddr = pDev->V206DEV090.V206DEV170[chan][op];
		linkmapaddr = mwv206GetAXIAddr(linkaddr);

		ret = FUNC206HAL310(pDev, paddr, width, mapaddr, mwv206stride,
				width, height, linkaddr, linkmapaddr);
		if (ret != 0) {
			ret = -3;
			goto FUNC206HAL020;
		}


		if (FUNC206HAL314(pDev, linkaddr) != 0) {
			ret = -8;
			goto FUNC206HAL020;
		}

		if (FUNC206HAL293(pDev, chan, linkmapaddr) != 0) {
			ret = -4;
			goto FUNC206HAL020;
		}


		V206DEV005("waiting for dma write block finished\n");
		if (FUNC206HAL409(pDev, V206IOCTL021, chan) != 0) {
			ret = -5;
			goto FUNC206HAL020;
		}

		if (FUNC206HAL314(pDev, V206IOCTLCMD009) != 0) {
			ret = -6;
			goto FUNC206HAL020;
		}


		break;
	default:
		ret = -101;
		break;
	}

	V206DEV005("[DONE]mwv206_dmamemblock_access 0x%x.\n", memaccess->op);
	FUNC206HAL319(pDev, op, chan, 0);
	return 0;

FUNC206HAL020:
	FUNC206HAL319(pDev, op, chan, 0);
	if (ret == -4 || ret == -5) {
		pDev->V206DEV090.dma_is_err = 1;
		V206KDEBUG003("[INFO] mwv206 dmablock switch to mem_rw\n");
		ret = mwv206_mem_rw_block(pDev, op, V206DEV031, cpustride, V206IOCTLCMD009, mwv206stride, width, height);
	}
	return ret;

__ERROR2:
	if (FUNC206HAL314(pDev, linkaddr) != 0) {
		return -8;
	}
	ret = mwv206_mem_rw_block(pDev, op, V206DEV031, cpustride, V206IOCTLCMD009, mwv206stride, width, height);
	return ret;
}

int FUNC206HAL328(V206DEV025 *pDev, long arg)
{
	V206IOCTL163 *memaccess;
	int ret;
	int op;
	int width, height, chan, w, h, w1, h1, wcnt, hcnt;
	unsigned char *V206DEV031, *memaddr_t;
	unsigned int V206IOCTLCMD009, mwv206stride, cpustride, mwv206addr_t;
	int i, j;

	ret = 0;
	memaccess = (V206IOCTL163 *)arg;


#ifdef MWV206_DMA_USE_MULTI_CHAN
	chan = memaccess->chan;
#else
	chan = 0;
#endif


	op = memaccess->op;


	width = memaccess->size;
	height = memaccess->vsize;
	V206DEV005("[INFO] DMA %s block start: width = %d, height = %d.",
			op ? "write" : "read", width, height);


	V206DEV031 = (unsigned char *)FUNC206HAL071(memaccess->V206DEV031, memaccess->memaddrHi);
	cpustride = memaccess->memstride;


	V206IOCTLCMD009 = memaccess->V206IOCTLCMD009;
	mwv206stride = memaccess->mwv206stride;
	if (!size_in_fb_range(pDev, V206IOCTLCMD009, mwv206stride * height - (mwv206stride - width))
		|| !addr_in_fb_range(pDev, V206IOCTLCMD009)) {
		V206KDEBUG002("[INFO] invalid dmamemblock_access, ignored:\n");
		V206KDEBUG002("[INFO] pid:%d mwv206addr:0x%x mwv206stride:0x%x, width:%d, height:%d\n",
				current->pid, V206IOCTLCMD009, mwv206stride, width, height);
		return -1;
	}


	w = width > MWV206_MAX_DMA_SIZE ? MWV206_MAX_DMA_SIZE : width;
	h = MWV206_MAX_DMA_SIZE / w;

	if (op == V206IOCTL020) {
		int oncebytes;
		oncebytes = pDev->V206DEV090.V206DEV171;
		w = w > oncebytes ? oncebytes : w;
		h1 = oncebytes / w;
		h = h > h1 ? h1 : h;
	}

	h = h > (4096) ? (4096) : h;

	V206DEV005("[INFO][%d] DMA %s block start: w = %d, h = %d.",
			__LINE__, op ? "write" : "read", w, h);

	w1 = width % w;
	wcnt = width / w;
	h1 = height % h;
	hcnt = height / h;
	V206DEV005("[INFO][%d] w1 = %d, wcnt = %d, h1 = %d, hcnt = %d.",
			__LINE__, w1, wcnt, h1, hcnt);

	for (i = 0; i < hcnt; i++) {
		memaddr_t = V206DEV031;
		mwv206addr_t = V206IOCTLCMD009;
		for (j = 0; j < wcnt; j++) {
			ret = FUNC206HAL329(pDev, op, chan, mwv206addr_t, mwv206stride,
					memaddr_t, cpustride, w, h);
			if (ret != 0) {
				V206KDEBUG002("dma block %s error: w = %d, h = %d.", op ? "write" : "read", w, h);
				return -2;
			}
			memaddr_t += w;
			mwv206addr_t += w;
		}
		if (w1) {
			ret = FUNC206HAL329(pDev, op, chan, mwv206addr_t, mwv206stride,
					memaddr_t, cpustride, w1, h);
			if (ret != 0) {
				V206KDEBUG002("dma block %s error: w1 = %d, h = %d.", op ? "write" : "read", w1, h);
				return -2;
			}
		}
		V206IOCTLCMD009 += h * mwv206stride;
		V206DEV031 += h * cpustride;
	}

	if (h1) {
		for (j = 0; j < wcnt; j++) {
			ret = FUNC206HAL329(pDev, op, chan, V206IOCTLCMD009, mwv206stride,
					V206DEV031, cpustride, w, h1);
			if (ret != 0) {
				V206KDEBUG002("dma block %s error: w = %d, h1 = %d.\n", op ? "write" : "read", w, h1);
				return -2;
			}
			V206DEV031 += w;
			V206IOCTLCMD009 += w;
		}
		if (w1) {
			ret = FUNC206HAL329(pDev, op, chan, V206IOCTLCMD009, mwv206stride,
					V206DEV031, cpustride, w1, h1);
			if (ret != 0) {
				V206KDEBUG002("dma block %s error: w1 = %d, h1 = %d.", op ? "write" : "read", w1, h1);
				return -2;
			}
		}
	}
	return 0;
}