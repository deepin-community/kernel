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
#include <linux/sched.h>
#include "gljos.h"
#include "mwv206dev.h"
#include "mwv206reg.h"
#include "mwv206kconfig.h"

static int FUNC206HAL272(V206DEV025 *pDev, long timeout)
{
#ifdef __POLLING_4B14__
	FUNC206HAL231(pDev, (((0x200000) + 0x4000) + 0x0b14), 0xFFFFFFFF, 1);
#endif
	return 0;
}

static int FUNC206HAL273(V206DEV025 *pDev, long timeout)
{
#ifdef __POLLING_4B14__
	return FUNC206HAL244(pDev, (((0x200000) + 0x4000) + 0x0b14), 0);
#else
	return 0;
#endif
}

int FUNC206HAL403(V206DEV025 *pDev, long timeout)
{
	if (FUNC206LXDEV105(pDev->V206DEV064[MWV206KINTR_RENDER], timeout) == 0) {
		V206DEV005("wait for render-3D-intr sucessfully!\n");
		return 0;
	} else {
		V206KDEBUG002("wait for render-3D-intr timeout %ld!\n", timeout);
		return -1;
	}
}




static int FUNC206HAL307(V206DEV025 *pDev)
{
	unsigned int V206IOCTLCMD018;
	if (pDev->V206DEV080.V206DEV161 == NULL) {
		return -1;
	}

	V206DEV002((((0x200000) + 0x4000) + 0x0200),
			0x80000000 | pDev->V206DEV036 / 4);
	V206DEV002((((0x200000) + 0x4000) + 0x0214), pDev->V206DEV080.mask);
	V206DEV005("[INFO] CPU ringbuf Mask: 0x%x\n", pDev->V206DEV080.mask);

	V206IOCTLCMD018 = V206DEV001((((0x200000) + 0x4000) + 0x020C));
	V206DEV002((((0x200000) + 0x4000) + 0x0208), V206IOCTLCMD018);

	V206DEV002((((0x200000) + 0x4000) + 0x0210), 1);
	V206DEV005("[INFO] %s finish!\n", __FUNCTION__);

	pDev->V206DEV080.V206IOCTLCMD018 = pDev->V206DEV080.V206IOCTLCMD020 = V206IOCTLCMD018;

	return 0;
}

static int FUNC206HAL303(V206DEV025 *pDev)
{
	unsigned int size = 0;
	unsigned long align = 0x10000;
	unsigned long long V206IOCTLCMD001, V206IOCTLCMD002;
	unsigned long long addr;

	size = pDev->V206DEV037 - pDev->V206DEV036 + 1;

	if (pDev->V206DEV080.V206DEV165 == 1) {
		return 0;
	}
	pDev->V206DEV080.V206DEV165 = 1;

#if defined(__mips__) || defined(__loongarch__)
#else

	{
		unsigned int val;
		pci_read_config_dword(pDev->V206DEV103, 0x78, &val);
		val |= 0x3000;
		pci_write_config_dword(pDev->V206DEV103, 0x78, val);
	}
#endif

	pDev->V206DEV080.V206DEV164 = size + align;

	if (pDev->V206DEV080.V206DEV163 == 0) {
#ifdef MWV206_RING_USE_DMABUF
		addr = (unsigned long long)(uintptr_t)FUNC206LXDEV009
			(&(pDev->V206DEV103->dev), &V206IOCTLCMD001, pDev->V206DEV080.V206DEV164);
#else
		addr = (unsigned long long)(uintptr_t)FUNC206LXDEV010(pDev->V206DEV080.V206DEV164);
		V206IOCTLCMD001 = FUNC206LXDEV011(addr);
#endif
		if (addr == 0) {
			V206KDEBUG002("alloc cpuring buffer failed\n");
			V206KDEBUG002("dma size: 0x%x\n", pDev->V206DEV080.V206DEV164);
			return -1;
		}

		pDev->V206DEV080.V206DEV163 = addr;
		pDev->V206DEV080.V206DEV162 = V206IOCTLCMD001;
	} else {
		addr = pDev->V206DEV080.V206DEV163;
		V206IOCTLCMD001 = pDev->V206DEV080.V206DEV162;
	}


	V206IOCTLCMD002 = V206IOCTLCMD001 + align;
	V206IOCTLCMD002 &= ~(align - 1);

	addr += (V206IOCTLCMD002 - V206IOCTLCMD001);

	FUNC206HAL238(pDev, 17,
			size - 1, pDev->V206DEV036, V206IOCTLCMD002);
	pDev->V206DEV080.V206DEV161 = (unsigned int *)(uintptr_t)addr;
	pDev->V206DEV080.V206IOCTLCMD018 = 0;
	pDev->V206DEV080.V206IOCTLCMD020 = 0;
	pDev->V206DEV080.V206IOCTLCMD019 = size / 4;
	pDev->V206DEV080.mask = pDev->V206DEV080.V206IOCTLCMD019 - 1;
	pDev->V206DEV080.free_words = pDev->V206DEV080.V206IOCTLCMD019 - 1;
	V206DEV005("cpuring buffer vaddr: 0x%llx, bus addr: 0x%llx\n", addr, V206IOCTLCMD001);

	FUNC206HAL307(pDev);
	return 0;
}

static inline void FUNC206HAL309(V206DEV025 *pDev, unsigned int cmd)
{
	pDev->V206DEV080.V206DEV161[pDev->V206DEV080.V206IOCTLCMD020++] = cmd;
	pDev->V206DEV080.V206IOCTLCMD020 &= pDev->V206DEV080.mask;
	pDev->V206DEV080.free_words--;
}

static int FUNC206HAL306(void *dev,
		const unsigned int *buff, int V206IOCTLCMD019)
{
	V206DEV025 *pDev;
	int V206IOCTLCMD020;
	int i;
	GLJ_TICK tick;

	pDev = (V206DEV025 *)dev;
	V206IOCTLCMD020 = pDev->V206DEV080.V206IOCTLCMD020;

	mwv206_timed_loop (tick, pDev->V206DEV080.free_words < V206IOCTLCMD019, MWV206CMDTIMEOUT) {
		int V206IOCTLCMD018 = V206DEV001((((0x200000) + 0x4000) + 0x020C));
		int n = V206IOCTLCMD018 - V206IOCTLCMD020 - 1;

		if (n < 0) {
			V206DEV005("[hdebug]:rptr %d, wptr %d, n %d\n", V206IOCTLCMD018, V206IOCTLCMD020, n);
			n += pDev->V206DEV080.V206IOCTLCMD019;
			V206DEV005("[hdebug]:after n=%d, words=%#x\n", n, pDev->V206DEV080.V206IOCTLCMD019);
		}
		pDev->V206DEV080.free_words = n;
	}
	if (pDev->V206DEV080.free_words < V206IOCTLCMD019) {
		V206KDEBUG002("[ERROR] %s timeout when waiting for free words in cpuring buffer.\n",
				__FUNCTION__);
		return 0;
	}

	for (i = 0; i < V206IOCTLCMD019; i++) {
		FUNC206HAL309(pDev, buff[i]);
	}
	barrier();
	if ((V206IOCTLCMD020 & 0xffffff00) != (pDev->V206DEV080.V206IOCTLCMD020 & 0xffffff00)) {
		V206DEV002((((0x200000) + 0x4000) + 0x0208), pDev->V206DEV080.V206IOCTLCMD020 & 0xffffff00);
	}

	return V206IOCTLCMD019;
}

static int FUNC206HAL304(void *dev, const unsigned int *buff, int V206IOCTLCMD019)
{
	int ret = V206IOCTLCMD019;
	V206DEV005("[INFO] %s(), words %#x", __FUNCTION__, V206IOCTLCMD019);


	FUNC206HAL291(dev, 1);
	ret = FUNC206HAL306(dev, buff, V206IOCTLCMD019);
	if (ret != V206IOCTLCMD019) {
		V206KDEBUG002("[ERROR] %s: error.\n", __FUNCTION__);
	}
	FUNC206HAL291(dev, 0);

	return ret;
}

static int FUNC206HAL305(void *dev, const unsigned int *buff, int V206IOCTLCMD019, unsigned int offset)
{
	V206DEV025 *pDev = (V206DEV025 *)dev;
	int ret                = V206DEV018;

	V206DEV005("[INFO] %s(), words %#x", __FUNCTION__, V206IOCTLCMD019);

	FUNC206HAL291(dev, 1);
	if (offset == pDev->V206DEV096.offset || offset == V206DEV017) {
		ret = FUNC206HAL306(dev, buff, V206IOCTLCMD019);
		if (ret != V206IOCTLCMD019) {
			V206KDEBUG002("[ERROR] %s: error.\n", __FUNCTION__);
		}
	}
	FUNC206HAL291(dev, 0);

	return ret;
}

static int FUNC206HAL302(V206DEV025 *pDev)
{
	int V206IOCTLCMD020;
	static unsigned int V206IOCTLCMD011[256];
	static int init = 1;
	if (init) {
		int i;
		unsigned int nopcmd;

		if (FUNC206LXDEV137()) {
			nopcmd = 0x80000000;
		} else {
			nopcmd = 0x80;
		}

		init = 0;
		for (i = 0; i < 256; i++) {
			V206IOCTLCMD011[i] = nopcmd;
		}
	}
	V206IOCTLCMD020 = pDev->V206DEV080.V206IOCTLCMD020;
	V206DEV005("wptr : %#x\n", V206IOCTLCMD020);
	if (V206IOCTLCMD020 & 0xff) {
		V206DEV005("mwv206_cpuring_sendcmds_nolock : %#x\n", 0x100 - (V206IOCTLCMD020 & 0xff));
		FUNC206HAL306(pDev, V206IOCTLCMD011, 0x100 - (V206IOCTLCMD020 & 0xff));
	}
	return 0;
}

static int FUNC206HAL308(V206DEV025 *pDev)
{
	FUNC206HAL302(pDev);

	return FUNC206HAL231(pDev, (((0x200000) + 0x4000) + 0x020C), 0xFFFFFFFF,
			pDev->V206DEV080.V206IOCTLCMD020);
}

int FUNC206HAL298(void *dev, long arg)
{
	int ret = 0;
	V206DEV025 *pDev;
	pDev = (V206DEV025 *)dev;

	V206DEV005("wait for 2d idle\n");

	ret = FUNC206HAL308(pDev);
	if (ret != 0) {
		V206KDEBUG002("[ERROR] %s sync error.\n", __FUNCTION__);
		return -1;
	}

	ret = FUNC206HAL231(pDev, (((0x200000) + 0x4000) + 0x0100), ((0x00010000) | (0x00800000) | (0x7F007800)), 0x0);
	if (ret != 0) {
		V206KDEBUG002("[ERROR] %s 0x4100:0x%x; 0x4B00:0x%x, 0x4B04:0x%x, 0x4B08:0x%x,\n 0x4B0C:0x%x, 0x4B10:0x%x\n",
				__FUNCTION__,
				V206DEV001(0x204100), V206DEV001(0x204B00), V206DEV001(0x204B04),
				V206DEV001(0x204B08), V206DEV001(0x204B0C), V206DEV001(0x204B10));
		ret = -2;
	}
	V206DEV005("End wait for 2d idle.\n");
	return ret;
}

static int FUNC206HAL301(void *dev)
{
	FUNC206HAL291(dev, 1);
	FUNC206HAL302((V206DEV025 *)dev);
	FUNC206HAL291(dev, 0);

	return 0;
}

int FUNC206HAL297(void *dev, long arg)
{
	int ret;


	FUNC206HAL291(dev, 1);
	ret = FUNC206HAL298(dev, MWV206CMDTIMEOUT);
	FUNC206HAL291(dev, 0);

	return ret;
}

int FUNC206HAL300(void *dev, long arg)
{
	int timeout;
	int ret = 0;
	V206DEV025 *pDev;
	pDev = (V206DEV025 *)dev;

	ret = FUNC206HAL308(pDev);
	if (ret != 0) {
		V206KDEBUG002("[ERROR] %s sync error.\n", __FUNCTION__);
		return -1;
	}

	timeout = MWV206CMDTIMEOUT;
	if (!FUNC206HAL368(pDev, MWV206KINTR_RENDER)) {
		V206DEV005("wait for render 3D idle\n");

		FUNC206HAL272(pDev, timeout);

		ret = FUNC206HAL231(pDev, (((0x200000) + 0x4000) + 0x0100), ((0x00010000) | (0x00800000) | (0x007E0000) | (0x7F007800)), 0x0);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] %s 0x4100:0x%x; 0x4B00:0x%x, 0x4B04:0x%x, 0x4B08:0x%x,\n 0x4B0C:0x%x, 0x4B10:0x%x\n",
					__FUNCTION__,
					V206DEV001(0x204100), V206DEV001(0x204B00), V206DEV001(0x204B04),
					V206DEV001(0x204B08), V206DEV001(0x204B0C), V206DEV001(0x204B10));
			ret = -2;
		}
		FUNC206HAL273(pDev, timeout);
		V206DEV005("End wait for idle.\n");

		return ret;
	} else {
		V206DEV005("wait for render 3D interrupt:\n");
		return FUNC206HAL403(pDev, timeout);
	}
}

int FUNC206HAL299(void *dev, long arg)
{
	int ret;
	FUNC206HAL291(dev, 1);
	ret = FUNC206HAL300(dev, MWV206CMDTIMEOUT);
	FUNC206HAL291(dev, 0);
	return ret;
}




static int FUNC206HAL345(V206DEV025 *pDev, int rbsize)
{
	int oldrbsize;
	unsigned long V206IOCTLCMD013;
	unsigned long V206IOCTLCMD014;

	if (pDev->V206DEV081.V206DEV165 == 1) {
		return 0;
	}
	pDev->V206DEV081.V206DEV165 = 1;



	if (rbsize > 1024) {
		V206KDEBUG002("ring buffer size is too large: %dMBytes,"
				"should be smaller than 1024Mbytes\n", rbsize);
		return -1;
	}


	V206DEV002((((0x200000) + 0x4000) + 0x020C), 0);
	V206DEV002((((0x200000) + 0x4000) + 0x0208), 0);
	V206DEV005("[GPURING_W]:0x%08x, 0\n", (((0x200000) + 0x4000) + 0x020C));
	V206DEV005("[GPURING_W]:0x%08x, 0\n", (((0x200000) + 0x4000) + 0x0208));

	V206DEV005("rbsize = %dMbytes\n", rbsize);

#ifdef __FOR_WRAP_TEST__
	rbsize = (2 << 8);


#else
	rbsize = rbsize << 20;
#endif

	V206KDEBUG002("rbsize = 0x%xbytes\n", rbsize >> 2);
	V206DEV005("ringbuffersize = 0x%xMbytes\n\n", pDev->V206DEV081.size);

	oldrbsize = pDev->V206DEV081.size;

	if (oldrbsize >= (rbsize >> 2)) {
		V206KDEBUG002("gpu-ring-buffer has been allocted previously.");
		goto FUNC206HAL011;
		return 0;
	}


	if (oldrbsize != 0) {
		FUNC206HAL226(pDev, pDev->V206DEV081.startaddr);
		V206DEV005("ring buffer mem free.");
	}

	V206IOCTLCMD013 = FUNC206HAL223(pDev, -rbsize, 4);

	if (V206IOCTLCMD013 == 0) {
		V206KDEBUG002("allocate from framebuffer failure.");
		return -2;
	}

	V206KDEBUG002("mwv206MemAlloc rbaddr = 0x%lx\n", V206IOCTLCMD013);


	V206IOCTLCMD014 = mwv206GetAXIAddr(V206IOCTLCMD013);
	V206KDEBUG002("mwv206GetAXIAddr rbaddr = 0x%lx\n", V206IOCTLCMD014);

	if ((V206IOCTLCMD013 >> 16 >> 16) > 0) {
		V206KDEBUG002("address(0x%lx) is 64bit", V206IOCTLCMD013);

	}


	pDev->V206DEV081.startaddr = V206IOCTLCMD013;
	pDev->V206DEV081.absstartaddr = V206IOCTLCMD014;

	pDev->V206DEV081.size = (rbsize >> 2);
	V206KDEBUG002("changed rbsize = %dMbytes\n\n\n", rbsize >> 20);

FUNC206HAL011:
	V206DEV002((((0x200000) + 0x4000) + 0x0200),
			0x80000000 |
			(((pDev->V206DEV081.absstartaddr) >> 2) & 0x3fffffff));
	V206DEV002((((0x200000) + 0x4000) + 0x0214), (pDev->V206DEV081.size - 1));
	V206DEV002((((0x200000) + 0x4000) + 0x0210), 1);

	V206DEV005("[GPURING_W]:0x%08x, 0x%08x\n", (((0x200000) + 0x4000) + 0x0200),
			0x80000000 | (((pDev->V206DEV081.absstartaddr) >> 2) & 0x3fffffff));
	V206DEV005("[GPURING_W]:0x%08x, 0x%08x\n", (((0x200000) + 0x4000) + 0x0214),
			(pDev->V206DEV081.size - 1));
	V206DEV005("[GPURING_W]:0x%08x, 0x%08x\n", (((0x200000) + 0x4000) + 0x0210), 1);

	return 0;
}

static int FUNC206HAL348(void *dev, const unsigned int *cmd, int cmdcount)
{
	V206DEV025 *pDev;
	GLJ_TICK tick;
	int V206IOCTLCMD015, V206IOCTLCMD023, V206IOCTLCMD003, V206IOCTLCMD021, V206IOCTLCMD007;
	int V206IOCTLCMD017;
	unsigned int V206IOCTLCMD022 = 0, size, V206IOCTLCMD009, *V206IOCTLCMD006;
	int V206IOCTLCMD008;

	pDev = (V206DEV025 *)dev;
	V206DEV005("\n\n%s function !!!!!!!!!!\n", __FUNCTION__);
	V206DEV005("cmdcount = 0x%x\n", cmdcount);





	V206IOCTLCMD008 = 0;



	V206IOCTLCMD017 = pDev->V206DEV081.size;
	V206IOCTLCMD003 = pDev->V206DEV081.startaddr;
	V206DEV005("ringBufferLen4 = 0x%x, cRBStartAddr = 0x%x\n", V206IOCTLCMD017, V206IOCTLCMD003);



	V206IOCTLCMD023 = pDev->V206DEV081.V206IOCTLCMD020;

	V206DEV005("writePtr = 0x%x\n", V206IOCTLCMD023);
	V206DEV005("cmd addr = %p\n", cmd);
	V206DEV005("cmdcount = %d, ringbufferLen4 = %d\n", cmdcount, V206IOCTLCMD017);


	V206IOCTLCMD009 = (V206IOCTLCMD003) + (V206IOCTLCMD023 << 2);
	V206IOCTLCMD006 = (unsigned int *)cmd;
	size = cmdcount;


	V206IOCTLCMD007 = 1;


	mwv206_timed_loop (tick, 1, MWV206CMDTIMEOUT) {

		V206IOCTLCMD015 = pDev->V206DEV081.V206IOCTLCMD018;
		V206DEV005("readPtr = %d\n", V206IOCTLCMD015);


		if (((V206IOCTLCMD023 + V206IOCTLCMD007) % V206IOCTLCMD017)  == V206IOCTLCMD015) {
			V206DEV005("ring buffer is full.\n");
			V206IOCTLCMD015 = V206DEV001((((0x200000) + 0x4000) + 0x020C));
			pDev->V206DEV081.V206IOCTLCMD018 = V206IOCTLCMD015;
			continue;
		}


		if (V206IOCTLCMD023 >= V206IOCTLCMD015) {



			V206IOCTLCMD021 = V206IOCTLCMD017 - (V206IOCTLCMD023 - V206IOCTLCMD015) - V206IOCTLCMD007;

			if (cmdcount > V206IOCTLCMD021) {


				V206IOCTLCMD015 = V206DEV001((((0x200000) + 0x4000) + 0x020C));
				pDev->V206DEV081.V206IOCTLCMD018 = V206IOCTLCMD015;
				continue;
			} else {
				V206DEV005("cap is enough\n");

				if (cmdcount < (V206IOCTLCMD017 - V206IOCTLCMD023)) {
					V206DEV005("copy once[s-r-w]\n");
					V206DEV005("mwv206addr = 0x%x, cpuaddr = %p, size = %d\n", V206IOCTLCMD009, V206IOCTLCMD006, size);
					V206DEV005("pDev->memaddr[%d] = 0x%lx\n", 1, pDev->V206DEV031[1]);
					V206DEV005("[0x%x, 0x%x, 0x%x]\n", V206IOCTLCMD006[0], V206IOCTLCMD006[1], V206IOCTLCMD006[2]);
					FUNC206HAL229(pDev, V206IOCTLCMD009, (unsigned char *)V206IOCTLCMD006, size * 4);
					V206DEV005("copy once done");

				} else {
					V206DEV005("copy twice[s-r-w]\n");

					size = (V206IOCTLCMD017 - V206IOCTLCMD023);
					FUNC206HAL229(pDev, V206IOCTLCMD009, (unsigned char *)cmd, size * 4);


					V206IOCTLCMD009 = V206IOCTLCMD003;
					V206IOCTLCMD006 = (unsigned int *)(cmd + size);
					size = cmdcount - size;
					FUNC206HAL229(pDev, V206IOCTLCMD009, (unsigned char *)V206IOCTLCMD006, size * 4);
				}
				V206IOCTLCMD023 = (V206IOCTLCMD023 + cmdcount) % V206IOCTLCMD017;
				V206DEV005("write OK \n");
				V206IOCTLCMD008 += cmdcount;
				V206IOCTLCMD022 = 1;
				break;
			}
		} else {


			V206IOCTLCMD021 = (V206IOCTLCMD015 - V206IOCTLCMD023 - V206IOCTLCMD007);
			if (cmdcount > V206IOCTLCMD021) {

				V206IOCTLCMD015 = V206DEV001((((0x200000) + 0x4000) + 0x020C));
				pDev->V206DEV081.V206IOCTLCMD018 = V206IOCTLCMD015;
				continue;
			} else {


				FUNC206HAL229(pDev, V206IOCTLCMD009, (unsigned char *)V206IOCTLCMD006, size * 4);
				V206IOCTLCMD008 += cmdcount;
				V206IOCTLCMD023 += cmdcount;
				V206IOCTLCMD022 = 1;
				break;
			}
		}
	}

	if (V206IOCTLCMD022 == 0) {
		V206KDEBUG002("write command timeout(%lu), write failure.\n", MWV206CMDTIMEOUT);
		return V206IOCTLCMD008;
	}

	V206DEV005("write frame buffer done\n");



	FUNC206HAL314(pDev, pDev->V206DEV081.startaddr);

	V206DEV002((((0x200000) + 0x4000) + 0x0208), V206IOCTLCMD023);
	pDev->V206DEV081.V206IOCTLCMD020 = V206IOCTLCMD023;
	return V206IOCTLCMD008;
}

static int FUNC206HAL346(void *dev, const unsigned int *cmd, int cmdcount)
{
	int ret;
	FUNC206HAL291(dev, 1);
	ret = FUNC206HAL348(dev, cmd, cmdcount);
	FUNC206HAL291(dev, 0);
	return ret;
}

int FUNC206HAL341(void *dev, long arg)
{
	int timeout;
	int ret;
	unsigned int V206IOCTLCMD023;
	V206DEV025 *pDev;

	pDev = (V206DEV025 *)dev;


	timeout = MWV206CMDTIMEOUT;
	V206DEV005("polling mode, timeout = %d\n", timeout);
	V206IOCTLCMD023 = pDev->V206DEV081.V206IOCTLCMD020;


	ret = FUNC206HAL231(pDev, (((0x200000) + 0x4000) + 0x020C), 0xFFFFFFFF, V206IOCTLCMD023);
	if (ret != 0) {
		V206KDEBUG002("wait for 'readptr(0x%x) == writeptr(0x%x)' failure.\n",
				V206DEV001((((0x200000) + 0x4000) + 0x020C)), V206IOCTLCMD023);

		return -1;
	}



	ret = FUNC206HAL231(pDev, (((0x200000) + 0x4000) + 0x0100), ((0x00010000) | (0x00800000) | (0x7F007800)), 0);
	if (ret != 0) {
		V206KDEBUG002("wait for 2d-idle timeout, (0x%x: 0x%x).\n",
				(((0x200000) + 0x4000) + 0x0100), V206DEV001((((0x200000) + 0x4000) + 0x0100)));
		ret = -2;
	}

	V206DEV005("End wait for 2d-idle.\n");
	return 0;

}

int FUNC206HAL340(void *dev, long arg)
{
	int ret;
	FUNC206HAL291(dev, 1);
	ret = FUNC206HAL341(dev, MWV206CMDTIMEOUT);
	FUNC206HAL291(dev, 0);
	return ret;
}

int FUNC206HAL344(void *dev)
{

	return 0;
}

static int FUNC206HAL347(void *dev, const unsigned int *cmd, int cmdcount, unsigned int offset)
{
	V206DEV025 *pDev = (V206DEV025 *)dev;
	int               ret  = V206DEV018;

	FUNC206HAL291(dev, 1);
	if (offset == pDev->V206DEV096.offset || offset == V206DEV017) {
		ret = FUNC206HAL348(dev, cmd, cmdcount);
	}
	FUNC206HAL291(dev, 0);

	return ret;
}

int FUNC206HAL343(void *dev, long arg)
{
	int timeout;
	int ret;
	V206DEV025 *pDev;
	pDev = (V206DEV025 *)dev;


	timeout = MWV206CMDTIMEOUT;

	if (!FUNC206HAL368(pDev, MWV206KINTR_RENDER)) {
		unsigned int V206IOCTLCMD023;

		V206DEV005("polling mode, timeout = %d\n", timeout);

		FUNC206HAL272(pDev, timeout);

		V206IOCTLCMD023 = pDev->V206DEV081.V206IOCTLCMD020;

		ret = FUNC206HAL231(pDev, (((0x200000) + 0x4000) + 0x020C), 0xFFFFFFFF, V206IOCTLCMD023);
		if (ret != 0) {
			V206KDEBUG002("wait for 'readptr(0x%x) == writeptr(0x%x)' failure.\n",
					V206DEV001((((0x200000) + 0x4000) + 0x020C)), V206IOCTLCMD023);
			return -1;
		}



		ret = FUNC206HAL231(pDev, (((0x200000) + 0x4000) + 0x0100), ((0x00010000) | (0x00800000) | (0x007E0000) | (0x7F007800)), 0);
		if (ret != 0) {
			V206KDEBUG002("wait for 3d-idle timeout, (0x%x: 0x%x).\n",
					(((0x200000) + 0x4000) + 0x0100), V206DEV001((((0x200000) + 0x4000) + 0x0100)));
			ret = -2;
		}

		FUNC206HAL273(pDev, timeout);

		V206DEV005("End wait for 3d-idle.\n");
		return 0;
	} else {
		V206DEV005("wait for render 3D interrupt:\n");
		ret = FUNC206HAL403(pDev, timeout);
		return ret;
	}
}

int FUNC206HAL342(void *dev, long arg)
{
	int ret;
	FUNC206HAL291(dev, 1);
	ret = FUNC206HAL343(dev, MWV206CMDTIMEOUT);
	FUNC206HAL291(dev, 0);
	return ret;
}

int FUNC206HAL338(void *dev)
{
	V206DEV025 *pDev = (V206DEV025 *)dev;
	return ((V206DEV001((((0x200000) + 0x4000) + 0x0100)) & (0x00800000)) == 0);
}

int FUNC206HAL339(void *dev)
{
	V206DEV025 *pDev = (V206DEV025 *)dev;
	return ((V206DEV001((((0x200000) + 0x4000) + 0x0100)) & (0x007E0000)) == 0);
}





static int FUNC206HAL288(
		V206DEV025 *pDev, const unsigned int *pCmd, int count)
{
	GLJ_TICK tick;
	int V206IOCTLCMD004, V206IOCTLCMD008, ecount, scratch;

	V206IOCTLCMD008 = 0;
	ecount = count;

	mwv206_timed_do (tick, scratch, MWV206CMDTIMEOUT) {
		V206IOCTLCMD004 = V206DEV001((((0x200000) + 0x4000) + 0x0A10));

		if (V206IOCTLCMD004 <= 0) {
			continue;
		}


		V206IOCTLCMD004 -= (64);
		if (V206IOCTLCMD004 < 0) {
			V206IOCTLCMD004 = 0;
		}

		if (V206IOCTLCMD004 > count) {
			V206IOCTLCMD004 = count;
		}
		{
			int i;
			for (i = 0; i < V206IOCTLCMD004; i++) {
				V206DEV002((((0x200000) + 0x4000) + 0x0000), pCmd[i]);
			}


			FUNC206LXDEV012();

			V206IOCTLCMD008 += V206IOCTLCMD004;
		}
		if (count <= V206IOCTLCMD004) {
			return V206IOCTLCMD008;
		}
		pCmd += V206IOCTLCMD004;
		count -= V206IOCTLCMD004;
	}

	V206DEV005("%d commands are written, expected 0x%x.\n", V206IOCTLCMD008, ecount);
	V206KDEBUG002("SendCommand timeout[%lu] once, (0x%x: 0x%08x, 0x%x: 0x%08x)\n", MWV206CMDTIMEOUT,
			(((0x200000) + 0x4000) + 0x0100), V206DEV001((((0x200000) + 0x4000) + 0x0100)),
			(((0x200000) + 0x4000) + 0x0b14), V206DEV001((((0x200000) + 0x4000) + 0x0b14)));
	return V206IOCTLCMD008;
}


int FUNC206HAL404(V206DEV025 *pDev)
{
	V206DEV005("wait for cmdport interrupt:\n");
	if (FUNC206LXDEV105(pDev->V206DEV064[MWV206KINTR_CMDPORT], MWV206CMDTIMEOUT) == 0) {
		V206DEV005("wait for cmd-port-intr successfully!\n");
		return 0;
	} else {
		V206KDEBUG002("wait for cmd-port-intr timeout!\n");
		return -1;
	}
}

static int FUNC206HAL287(
		V206DEV025 *pDev, const unsigned int *pCmd, int count)
{
	int V206IOCTLCMD004;
	int V206IOCTLCMD016 = count;
	int k, ret, V206IOCTLCMD008;
	V206DEV005("Waiting mode: Interrupt!\n");

	V206IOCTLCMD008 = 0;

	V206DEV002((((0x200000) + 0x4000) + 0x0104), 0x200);
	k = V206DEV001((((0x200000) + 0x4000) + 0x0104));
	while (V206IOCTLCMD016 != 0) {
		V206IOCTLCMD004 = V206DEV001((((0x200000) + 0x4000) + 0x0A10));
		V206DEV005("can write number is 0x%x, k = 0x%x\n", V206IOCTLCMD004, k);
		if (V206IOCTLCMD004 < k) {
			ret = FUNC206HAL404(pDev);
			if (ret != 0) {
				V206KDEBUG002("SendCommand timeout[%lu] once, (0x%x: 0x%08x, 0x%x: 0x%08x)\n",
						MWV206CMDTIMEOUT,
						(((0x200000) + 0x4000) + 0x0100), V206DEV001((((0x200000) + 0x4000) + 0x0100)),
						(((0x200000) + 0x4000) + 0x0b14), V206DEV001((((0x200000) + 0x4000) + 0x0b14)));
				return V206IOCTLCMD008;
			}
			V206IOCTLCMD004 = V206DEV001((((0x200000) + 0x4000) + 0x0A10));
		}


		V206IOCTLCMD004 -= (64);
		if (V206IOCTLCMD004 < 0) {
			V206IOCTLCMD004 = 0;
		}

		if (V206IOCTLCMD004 > V206IOCTLCMD016) {
			V206IOCTLCMD004 = V206IOCTLCMD016;
		}

		{
			int i;
			for (i = 0; i < V206IOCTLCMD004; i++) {
				V206DEV002((((0x200000) + 0x4000) + 0x0000), pCmd[i]);
			}


			FUNC206LXDEV012();

			V206IOCTLCMD008 += V206IOCTLCMD004;
			pCmd += V206IOCTLCMD004;
			V206IOCTLCMD016 -= V206IOCTLCMD004;
		}
	}

	return V206IOCTLCMD008;
}

int FUNC206HAL285(void *dev, const unsigned int *pCmd, int count)
{
	V206DEV025 *pDev = (V206DEV025 *)dev;
	int               V206IOCTLCMD019;

	if (count <= 0) {
		return 0;
	}

	FUNC206HAL291(pDev, 1);

	if (FUNC206HAL368(pDev, MWV206KINTR_CMDPORT)
			&& (V206DEV001((0x205200)) & (1 << 1))) {
		V206DEV005("use command-port-intr.\n\n");
		V206IOCTLCMD019 = FUNC206HAL287(pDev, pCmd, count);
	} else {
		V206DEV005("use command-port-polling.\n\n");
		V206IOCTLCMD019 =  FUNC206HAL288(pDev, pCmd, count);
	}

	FUNC206HAL291(pDev, 0);

	return V206IOCTLCMD019;
}

static int FUNC206HAL286(void *dev, const unsigned int *pCmd, int count, unsigned int offset)
{
	V206DEV025 *pDev = (V206DEV025 *)dev;
	int               V206IOCTLCMD019 = V206DEV018;

	if (count <= 0) {
		return 0;
	}

	FUNC206HAL291(pDev, 1);

	if (offset == pDev->V206DEV096.offset || offset == V206DEV017) {
		if (FUNC206HAL368(pDev, MWV206KINTR_CMDPORT)
				&& (V206DEV001((0x205200)) & (1 << 1))) {
			V206DEV005("use command-port-intr.\n\n");
			V206IOCTLCMD019 = FUNC206HAL287(pDev, pCmd, count);
		} else {
			V206DEV005("use command-port-polling.\n\n");
			V206IOCTLCMD019 =  FUNC206HAL288(pDev, pCmd, count);
		}
	}

	FUNC206HAL291(pDev, 0);

	return V206IOCTLCMD019;
}

static int FUNC206HAL290(void *dev, long arg)
{
	int timeout;
	int ret;
	V206DEV025 *pDev;
	pDev = (V206DEV025 *)dev;


	timeout = MWV206CMDTIMEOUT;
	if (!FUNC206HAL368(pDev, MWV206KINTR_RENDER)) {
		V206DEV005("polling mode, timeout = %d\n", timeout);

		V206DEV005("sendcmdmode : send cmd to cmdport");
		FUNC206HAL272(pDev, timeout);
		ret = FUNC206HAL231(pDev, (((0x200000) + 0x4000) + 0x0100), ((0x00010000) | (0x00800000) | (0x007E0000) | (0x000007FF)), 0);
		FUNC206HAL273(pDev, timeout);
		V206DEV005("End wait for idle.\n");
		return ret;
	} else {
		V206DEV005("wait for render 3D interrupt:\n");
		return FUNC206HAL403(pDev, timeout);
	}
}

int FUNC206HAL289(void *dev, long arg)
{
	int ret;
	FUNC206HAL291(dev, 1);
	ret = FUNC206HAL290(dev, MWV206CMDTIMEOUT);
	FUNC206HAL291(dev, 0);
	return ret;
}

int FUNC206HAL283(void *dev)
{

	return 0;
}




static int FUNC206HAL277(void *dev, const unsigned int *pCmd, int count)
{
	int V206IOCTLCMD004, i;
	GLJ_TICK tick;
	V206DEV025 *pDev;
	pDev = (V206DEV025 *)dev;

	V206DEV005("%s start\n\n", __FUNCTION__);


	V206IOCTLCMD004 = 0;
	mwv206_timed_loop (tick, V206IOCTLCMD004 < count, MWV206CMDTIMEOUT) {
		V206IOCTLCMD004 = V206DEV001((((0x200000) + 0x5000) + 0x0A10));
		V206IOCTLCMD004 -= (64);
	}

	if (V206IOCTLCMD004 < count) {
		V206KDEBUG002("!!!!!!!!!!!!!!can write number is only 0x%x(expected 0x%x)\n",
				V206IOCTLCMD004, count);
		return 0;
	}


	for (i = 0; i < count; i++) {
		V206DEV002((((0x200000) + 0x5000) + 0x0000), pCmd[i]);
	}


	FUNC206LXDEV012();

	V206DEV005("%s end\n\n", __FUNCTION__);
	return count;
}

static int FUNC206HAL278(void *dev, const unsigned int *pCmd, int count, unsigned int offset)
{
	V206DEV025 *pDev;
	int V206IOCTLCMD005;

	pDev = (V206DEV025 *)dev;

	if (count > (1000 - (64))) {
		V206KDEBUG002("[MWV206]: 2D command is too long, ignored\n");
		return 0;
	}

	FUNC206HAL282(pDev, 1);

	if (offset != pDev->V206DEV096.offset && offset != V206DEV017) {
		FUNC206HAL282(pDev, 0);
		return V206DEV018;
	}

	V206IOCTLCMD005 = pDev->V206DEV095;
	if (V206IOCTLCMD005 + count > (1000 - (64))) {
		FUNC206HAL277(dev, pDev->V206DEV094, V206IOCTLCMD005);
		V206IOCTLCMD005 = 0;
	}

	memcpy(pDev->V206DEV094 + V206IOCTLCMD005, pCmd, count * 4);
	pDev->V206DEV095 = V206IOCTLCMD005 + count;

	FUNC206HAL282(pDev, 0);

	return count;
}

static int FUNC206HAL279(void *dev)
{
	V206DEV025 *pDev;
	pDev = (V206DEV025 *)dev;

	if (pDev->V206DEV095 > 0) {
		FUNC206HAL277(dev, pDev->V206DEV094, pDev->V206DEV095);
		pDev->V206DEV095 = 0;
	}
	return 0;
}

static int FUNC206HAL281(void *dev, long arg)
{
	int ret = 0;
	V206DEV025 *pDev = (V206DEV025 *)dev;

	V206DEV005("%s\n", __FUNCTION__);

	FUNC206HAL279(pDev);

	ret = FUNC206HAL231(pDev, (((0x200000) + 0x5000) + 0x0100), ((0x00010000) | (0x00800000) | (0x000007FF)), 0);

	V206DEV005("End wait for idle.\n");

	return ret;
}
static int FUNC206HAL275(void *dev)
{
	FUNC206HAL282(dev, 1);
	FUNC206HAL279(dev);
	FUNC206HAL282(dev, 0);

	return 0;
}

int FUNC206HAL280(void *dev, long arg)
{
	int ret;
	FUNC206HAL282(dev, 1);
	ret = FUNC206HAL281(dev, MWV206CMDTIMEOUT);
	FUNC206HAL282(dev, 0);
	return ret;
}

int FUNC206HAL276(void *dev)
{
	V206DEV025 *pDev = (V206DEV025 *)dev;
	return ((V206DEV001((((0x200000) + 0x5000) + 0x0100)) & ((0x00010000) | (0x00800000) | (0x000007FF))) == 0);
}

int FUNC206HAL284(void *dev)
{
	V206DEV025 *pDev = (V206DEV025 *)dev;
	return ((V206DEV001((((0x200000) + 0x4000) + 0x0100)) & ((0x00010000) | (0x00800000) | (0x007E0000) | (0x000007FF))) == 0);
}

int FUNC206HAL232(V206DEV025 *pDev)
{
	unsigned int val, V206IOCTLCMD010;
	int          ret = 0;

	if (!FUNC206HAL257(pDev)) {
		return 0;
	}

	V206KDEBUG002("MWV206: try to recover from error\n");

	FUNC206HAL282(pDev, 1);


	ret = FUNC206HAL281(pDev, MWV206CMDTIMEOUT);
	if (ret) {
		goto out;
	}


	val    = V206DEV001(0x400240);
	V206IOCTLCMD010 = val & 0xFFFFE000;
	ret    = FUNC206HAL244(pDev, 0x400240, V206IOCTLCMD010);
	V206IOCTLCMD010 = val | 0x111F;
	ret   |= FUNC206HAL244(pDev, 0x400240, V206IOCTLCMD010);

	pDev->V206DEV080.V206DEV165 = 0;
	FUNC206HAL303(pDev);

	pDev->V206DEV088 = 0;
out:
	FUNC206HAL282(pDev, 0);
	return ret;
}


int FUNC206HAL242(V206DEV025 *pDev, const char *pBuf, int nBytes)
{
	int ret;

	ret =  pDev->V206DEV097.send((void *)pDev, (const unsigned int *)pBuf, nBytes / 4);
	if (ret != nBytes / 4) {
		FUNC206HAL232(pDev);
	}

	return ret;
}

int FUNC206HAL410(V206DEV025 *pDev, long arg)
{
	V206IOCTL177 *sync = (V206IOCTL177 *)arg;
	int ret;

	if (sync->op == SYNC_WAITFORIDLE) {
		if (pDev->V206DEV096.wait == NULL) {
			return -1;
		}

		if (pDev->vblank_sync) {
			ret = pDev->V206DEV096.wait(pDev, sync->timeout);
			mwv206_shadow_try_sync(pDev);
		} else {
			ret = pDev->V206DEV096.wait(pDev, sync->timeout);
		}
		return ret;
	} else if (sync->op == SYNC_FLUSH) {
		if (pDev->V206DEV096.flush == NULL) {
			return -1;
		}
		return pDev->V206DEV096.flush(pDev);
	} else {
		return -2;
	}
}

int FUNC206HAL411(V206DEV025 *pDev, long arg)
{
	V206IOCTL177 *sync = (V206IOCTL177 *)arg;

	if (sync->op == SYNC_WAITFORIDLE) {
		if (pDev->V206DEV097.wait == NULL) {
			return -1;
		}
		if (pDev->V206DEV097.wait(pDev, sync->timeout)) {
			return FUNC206HAL232(pDev);
		}
	} else if (sync->op == SYNC_FLUSH) {
		if (pDev->V206DEV097.flush == NULL) {
			return -1;
		}
		return pDev->V206DEV097.flush(pDev);
	} else {
		return -2;
	}

	return 0;
}





int FUNC206HAL270(unsigned int mode3d, unsigned int mode2d)
{
	static int first = 1;
	if (first == 1) {
		return 1;
	}

	first = 0;
	if (mode3d == mode2d) {
		return 1;
	}

	if (mode2d == V206API041) {
		return 1;
	}
	return 0;
}


static void FUNC206HAL271(V206DEV025 *pDev, int lock)
{

	if (lock) {
		FUNC206HAL291(pDev, 1);
		FUNC206HAL282(pDev, 1);
	} else {
		FUNC206HAL282(pDev, 0);
		FUNC206HAL291(pDev, 0);
	}
}

int FUNC206HAL398(V206DEV025 *pDev,
		int is3d, unsigned int mode, unsigned int rbsize)
{
	unsigned int V206IOCTLCMD012;
	int ret = 0;

	FUNC206HAL271(pDev, 1);
	if (is3d) {
		V206IOCTLCMD012 = pDev->V206DEV097.mode;
		if (V206IOCTLCMD012 == mode) {
			ret = 0;
			goto out_release_lock;
		}


		if (FUNC206HAL270(mode, pDev->V206DEV096.mode) == 0) {
			V206KDEBUG002("3d-send-cmds-mode(%d) is not match with 2d-send-cmds-mode(%d).\n",
					mode, pDev->V206DEV096.mode);
			ret = -1;
			goto out_release_lock;
		};
		V206KDEBUG003("[INFO] 3D send command mode [%d], rbsize = %d.\n", mode, rbsize);
		if (pDev->V206DEV097.first == 0) {
			switch (V206IOCTLCMD012) {
			case V206API038:
				FUNC206HAL290(pDev, MWV206CMDTIMEOUT);
				break;
			case V206API039:
				FUNC206HAL343(pDev, 1);
				break;
			case V206API040:
				FUNC206HAL300(pDev, 1);
				break;
			default:
				V206KDEBUG002("error old mode %d.\n", V206IOCTLCMD012);
				ret = -2;
				goto  out_release_lock;
			}
			V206DEV005("\n[3D] sync at oldmode %d.\n", V206IOCTLCMD012);
		}


		pDev->V206DEV097.mode = mode;
		switch (mode) {
		case V206API038:
			pDev->V206DEV097.send = FUNC206HAL285;
			pDev->V206DEV097.wait = FUNC206HAL289;
			pDev->V206DEV097.isidle = FUNC206HAL284;
			pDev->V206DEV097.flush = NULL;
			V206DEV002((((0x200000) + 0x4000) + 0x0200), 0x00000000);
			ret = 0;
			break;
		case V206API039:
			pDev->V206DEV097.send = FUNC206HAL346;
			pDev->V206DEV097.wait = FUNC206HAL342;
			pDev->V206DEV097.isidle = FUNC206HAL339;
			pDev->V206DEV097.flush = NULL;
			ret = FUNC206HAL345(pDev, rbsize);
			break;
		case V206API040:
			pDev->V206DEV097.send = FUNC206HAL304;
			pDev->V206DEV097.wait = FUNC206HAL299;
			pDev->V206DEV097.isidle = FUNC206HAL339;
			pDev->V206DEV097.flush = NULL;
			ret = FUNC206HAL303(pDev);
			break;
		default:
			V206KDEBUG002("Invalid 3D sendcmdmode: 0x%x.\n", mode);
			ret = -3;
			break;
		}

		V206DEV005("[3D] switch to newmode %d.\n", mode);
		pDev->V206DEV097.first = 0;
	} else {
		V206IOCTLCMD012 = pDev->V206DEV096.mode;
		if (V206IOCTLCMD012 == mode) {
			ret = 0;
			goto  out_release_lock;
		}

		V206KDEBUG003("[INFO] 2D send command mode [%d], oldmode = %d, rbsize = %d.\n",
				mode, V206IOCTLCMD012, rbsize);


		if (FUNC206HAL270(pDev->V206DEV097.mode, mode) == 0) {
			V206KDEBUG002("2d-send-cmds-mode(%d) is not match with 3d-send-cmds-mode(%d).\n",
					mode, pDev->V206DEV097.mode);
			ret =  -1;
			goto  out_release_lock;
		};

		if (pDev->V206DEV096.first == 0) {
			switch (V206IOCTLCMD012) {
			case V206API041:
				FUNC206HAL281(pDev, MWV206CMDTIMEOUT);
				break;
			case V206API039:
				FUNC206HAL341(pDev, 1);
				break;
			case V206API040:
				FUNC206HAL298(pDev, 1);
				break;
			default:
				V206KDEBUG002("error old mode %d.\n", V206IOCTLCMD012);
				ret =  -2;
				goto  out_release_lock;
			}
			V206DEV005("\n[2D] sync at oldmode %d.\n", V206IOCTLCMD012);
		}


		pDev->V206DEV096.mode = mode;
		switch (mode) {
		case V206API041:
			pDev->V206DEV096.offset = 0x5000;
			pDev->V206DEV096.send = FUNC206HAL278;
			pDev->V206DEV096.wait = FUNC206HAL280;
			pDev->V206DEV096.isidle = FUNC206HAL276;
			pDev->V206DEV096.flush = FUNC206HAL275;
			ret = 0;
			break;
		case V206API038:
			pDev->V206DEV096.offset = 0x0;
			pDev->V206DEV096.send = FUNC206HAL286;
			pDev->V206DEV096.wait = FUNC206HAL289;
			pDev->V206DEV096.isidle = FUNC206HAL284;
			pDev->V206DEV096.flush = FUNC206HAL283;
			ret = 0;
			break;
		case V206API039:
			pDev->V206DEV096.offset = 0;
			pDev->V206DEV096.send = FUNC206HAL347;
			pDev->V206DEV096.wait = FUNC206HAL340;
			pDev->V206DEV096.isidle = FUNC206HAL338;
			pDev->V206DEV096.flush = FUNC206HAL344;
			ret = FUNC206HAL345(pDev, rbsize);
			break;
		case V206API040:
			pDev->V206DEV096.offset = 0;
			pDev->V206DEV096.send = FUNC206HAL305;
			pDev->V206DEV096.wait = FUNC206HAL297;
			pDev->V206DEV096.isidle = FUNC206HAL338;
			pDev->V206DEV096.flush = FUNC206HAL301;
			ret = FUNC206HAL303(pDev);
			break;
		default:
			V206KDEBUG002("Invalid [2D] sendcmdmode: 0x%x.\n", mode);
			ret = -3;
			break;
		}

		V206DEV005("\n[2D] switch to newmode %d.\n", mode);
		pDev->V206DEV096.first = 0;
	}

out_release_lock:
	FUNC206HAL271(pDev, 0);
	return ret;
}

int FUNC206HAL399(V206DEV025 *pDev, long arg)
{
	int is3d = arg >> 30;
	unsigned int mode = ((unsigned long)arg >> 28) & 0x3;
	unsigned int rbsize = arg & 0xfffffff;

	return FUNC206HAL398(pDev, is3d, mode, rbsize);
}

void FUNC206HAL397(V206DEV025 *pDev, unsigned int reg, unsigned int val)
{
	unsigned int cmd[2];
	int V206KG2D003 = 0;
	cmd[V206KG2D003++] = 0x40000000 | reg;
	cmd[V206KG2D003++] = val;
	FUNC206HAL242(pDev, (unsigned char *)cmd, V206KG2D003 * 4);
}




int FUNC206HAL252(V206DEV025 *pDev,  V206IOCTL157 *mbit)
{
	unsigned    cmd[128];
	int         V206KG2D003;
	int         height;

	if (pDev->V206DEV096.send == NULL) {
		return -1;
	}

	height  = mbit->height;

	FUNC206HAL271(pDev, 1);
	FUNC206HAL308(pDev);
	if (!FUNC206HAL257(pDev)
			|| pDev->V206DEV097.mode != V206API040
			|| pDev->V206DEV096.mode != V206API041
			) {
		unsigned int offset;
		int cnt, ret;
		FUNC206HAL271(pDev, 0);
		cnt = 0;
retry:
		offset = pDev->V206DEV096.offset;
		V206KG2D003 = FUNC206HAL247(pDev, mbit, offset, cmd, 0);
		ret    = pDev->V206DEV096.send(pDev, cmd, V206KG2D003, offset);
		if (ret == V206DEV018 && cnt < 10) {
			cnt++;
			goto retry;
		};

		return  ret == V206KG2D003 ? 0 : -1;
	}


	FUNC206HAL281(pDev, MWV206CMDTIMEOUT);


	mbit->height     = (height + 1) / 2;
	V206KG2D003 = FUNC206HAL247(pDev, mbit, pDev->V206DEV096.offset, cmd, 0);


	FUNC206HAL277(pDev, cmd, V206KG2D003);

	mbit->height = height / 2;
	mbit->sy    += (height + 1) / 2;
	mbit->dy    += (height + 1) / 2;
	V206KG2D003       = FUNC206HAL247(pDev, mbit, 0x0, cmd, 1);


	FUNC206HAL306(pDev, cmd, V206KG2D003);


	FUNC206HAL298(pDev, MWV206CMDTIMEOUT);

	FUNC206HAL271(pDev, 0);

	return 0;
}

int FUNC206HAL274(V206DEV025 *pDev, int mode2d, int mode3d)
{
	V206KDEBUG003("[INFO] command mode init.\n");


	pDev->V206DEV096.mode = 0xFFFFFFFF;
	pDev->V206DEV097.mode = 0xFFFFFFFF;

	pDev->V206DEV096.first = 1;
	pDev->V206DEV097.first = 1;
	pDev->V206DEV080.V206DEV165 = 0;

	FUNC206HAL398(pDev, 0, mode2d, 4);
	FUNC206HAL398(pDev, 1, mode3d, 4);

	return 0;
}