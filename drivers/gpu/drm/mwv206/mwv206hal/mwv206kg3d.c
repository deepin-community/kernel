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
#include "mwv206dev.h"
#include "mwv206kdebug.h"
#include "gljos.h"
#include "asm/string.h"

static unsigned int FUNC206HAL193 = 1;


int FUNC206HAL260(V206DEV025 *pDev, unsigned long arg)
{
	unsigned long key = (unsigned long)FUNC206HAL193++;
	V206IOCTL145 *pkctx;
	int ret;


	if (unlikely(key == 0)) {
		key = (unsigned long)FUNC206HAL193++;
	}

	pkctx = (V206IOCTL145 *)arg;

	ret = mwv206_addrlist_insert(pDev->V206DEV087, &key, 1, 1);
	if (ret < 0) {
		V206KDEBUG002("kcontext create failed.");
		return -1;
	} else {
		pkctx->userctx = (jjuintptr)key;
		V206DEV005("kcontext create success.");
		return 0;
	}
}


int FUNC206HAL261(V206DEV025 *pDev, unsigned long userctx)
{
	int ret;

	ret = mwv206_addrlist_delete(pDev->V206DEV087, &userctx);
	if (ret == 0) {
		V206DEV005("kcontext destroy success.");
		return 0;
	} else {
		V206KDEBUG002("kcontext destroy failed.");
		return -1;
	}
}


int FUNC206HAL262(V206DEV025 *pDev, unsigned long userctx)
{
	int i;

	i = mwv206_addrlist_find(pDev->V206DEV087, &userctx);

	if (i != -1) {
		pDev->V206DEV088 = userctx;
		return 0;
	} else {
		V206KDEBUG002("[ERROR] the kcx is not found for userctx(0x%lx).\n", userctx);
		return -1;
	}
}


int FUNC206HAL259(V206DEV025 *pDev, unsigned long userctx)
{
	int ret;
	int mapctx = -1;

	V206DEV005("[INFO] %s: for (userctx-0x%lx).\n", __FUNCTION__, userctx);
	if (userctx == 0) {
		return 0;
	}

	mapctx = mwv206_addrlist_find(pDev->V206DEV087, &userctx);

	if (mapctx == -1) {
		V206KDEBUG002("[ERROR] the kcx is not found for userctx(0x%lx).\n", userctx);
		return -1;
	} else {
		if ((int)userctx == pDev->V206DEV088) {
			V206DEV005("[INFO] %s(kctx-0x%x: userctx-0x%lx) successfully.\n", __FUNCTION__, mapctx, userctx);
			ret = 1;
		} else {
			V206DEV005("[INFO] %s(kctx-0x%x: userctx-0x%lx) failure.\n", __FUNCTION__, mapctx, userctx);
			ret = 0;
		}
	}
	V206DEV005("[INFO] %s: set currentkctx to be 0x%x.\n", __FUNCTION__, mapctx);
	pDev->V206DEV088 = userctx;
	return ret;

}

int FUNC206HAL258(V206DEV025 *pDev, unsigned long arg)
{
	V206IOCTL145 *pkctx;
	unsigned long userctx;
	int op, ret;

	pkctx = (V206IOCTL145 *)arg;
	op = pkctx->op;
	userctx = pkctx->userctx;

	switch (op) {
	case V206IOCTL025:
		ret = FUNC206HAL260(pDev, arg);
		break;
	case V206IOCTL027:
		ret = FUNC206HAL259(pDev, userctx);
		break;
	case V206IOCTL026:
		ret = FUNC206HAL261(pDev, userctx);
		break;
	case V206IOCTL028:
		ret = FUNC206HAL262(pDev, userctx);
		break;
	default:
		V206KDEBUG002("error mwv206 kcontext access op %d.\n", op);
		ret = -1;
		break;
	}
	return ret;
}

int FUNC206HAL264(V206DEV025 *pDev, unsigned long userdata)
{
	V206DEV005("save owner.");
	return mwv206_addrlist_insert(pDev->V206DEV086, &userdata, 1, 1);
}

static int FUNC206HAL254(V206DEV025 *pDev, struct file *owner)
{
	int ret = 0;
	unsigned long listitem;
	listitem = (unsigned long)owner;

	V206DEV005("[INFO] owner = {0x%lx, 0x%lx, 0x%lx, 0x%lx}\n",
			pDev->V206DEV086->list[0], pDev->V206DEV086->list[1],
			pDev->V206DEV086->list[2], pDev->V206DEV086->list[3]);
	ret = mwv206_addrlist_delete(pDev->V206DEV086, &listitem);

	if (ret == 0) {
		V206DEV005("[INFO] %s: 3d-driver-owner(0x%lx) is free\n", __FUNCTION__, listitem);
	} else {
		V206DEV005("[INFO] %s: owner(0x%lx) is not 3d-driver\n", __FUNCTION__, listitem);
	}
	return ret;
}

int FUNC206HAL257(V206DEV025 *pDev)
{
	int ret;
	ret = mwv206_addrlist_isempty(pDev->V206DEV086);
	if (ret == 1) {
		V206DEV005("[INFO] 3d is not running.\n");
	} else {
		V206DEV005("[INFO] 3d is running.\n");
	}

	return !ret;
}

void FUNC206HAL265(V206DEV025 *pDev)
{

	mwv206_addrlist_destroy(pDev->V206DEV087);

	mwv206_addrlist_destroy(pDev->V206DEV086);
	V206DEV005("[INFO] mwv206_KContextUnInit successfully.\n");
	return;
}

int FUNC206HAL256(V206DEV025 *pDev)
{
	pDev->V206DEV086 = mwv206_addrlist_create(64);
	if (pDev->V206DEV086 == NULL) {
		return -1;
	}
	pDev->V206DEV087 = mwv206_addrlist_create(64);
	if (pDev->V206DEV087 == NULL) {
		return -2;
	}
	pDev->V206DEV088 = -1;
	return 0;
}

#define TEX_ID_REG_COUNT 1024
#define TEX_WRITE_ONCE_SIZE 128
static int mwv206TextureIDInit(V206DEV025 *pDev)
{
	unsigned int buf[TEX_WRITE_ONCE_SIZE];
	unsigned int reg, cmd;
	int i, cnt, total_cnt;
	int loop;

	if (pDev == NULL) {
		return -1;
	}

	cnt = TEX_WRITE_ONCE_SIZE;
	total_cnt = TEX_ID_REG_COUNT;

	if (cnt <= 0 || cnt > 1024 || total_cnt <= 0 || total_cnt > 1024 || cnt > total_cnt) {
		V206KDEBUG002("[ERR] texture size is wrong\n");
		return -1;
	}

	if (total_cnt % cnt == 0) {
		loop = total_cnt / cnt;
	} else {
		loop = total_cnt / cnt + 1;
	}




	for (i = 0; i < cnt; i++) {
		buf[i] = 0x100000;
	}


	reg = (0x8000);
	for (i = 0; i < loop; i++) {
		if (total_cnt < cnt) {
			cnt = total_cnt;
		}
		cmd = MWV206SETREGCMD(reg, cnt);
		FUNC206HAL242(pDev, (unsigned char *)&cmd,  4);
		FUNC206HAL242(pDev, (unsigned char *)&buf[0], cnt * 4);
		total_cnt -= cnt;
		reg += TEX_WRITE_ONCE_SIZE * 4;
	}

	return 0;
}


#include "mwv206romdata/texture_env.h"
int FUNC206HAL241(V206DEV025 *pDev)
{
	int i;
	int regCnt, V206KG2D003 = 0;
	unsigned int cmds[64];
	V206IOCTL177 sync;


	mwv206TextureIDInit(pDev);

	regCnt = sizeof(g_textureEnv_low) / sizeof(unsigned int);


	cmds[V206KG2D003++] = MWV206SETREGCMD((0xE000), 30);

	for (i = 0; i < regCnt; i++) {
		cmds[V206KG2D003++] = g_textureEnv_low[i];

	}


	cmds[V206KG2D003++] = MWV206SETREGCMD((0xF000), 30);

	for (i = 0; i < regCnt; i++) {
		cmds[V206KG2D003++] = g_textureEnv_high[i];

	}


	if (V206KG2D003 > 64) {
		V206KDEBUG002("cmd length is beyond 64.");
		return -1;
	}

	FUNC206HAL242(pDev, (unsigned char *)cmds, V206KG2D003 * 4);
	sync.op      = SYNC_WAITFORIDLE;
	sync.timeout = 0;
	FUNC206HAL411(pDev, (long)&sync);

	return 0;
}

#include "mwv206romdata/1_reciprom.h"
#include "mwv206romdata/2_sqrtreciprom.h"
#include "mwv206romdata/3_sqrtrom.h"
#include "mwv206romdata/4_expromb.h"
#include "mwv206romdata/5_exproms.h"
#include "mwv206romdata/7_lnxrom.h"
#include "mwv206romdata/8_lnxreciprom.h"

int FUNC206HAL233(V206DEV025 *pDev)
{
	unsigned int i;



	for (i = 0; i < sizeof(recip_rom_data) / sizeof(unsigned long); i++) {
		V206DEV002(((0x200000) + 0x10000) + i * 4, recip_rom_data[i]);
	}


	for (i = 0; i < sizeof(sqrt_recip_rom_data) / sizeof(unsigned long); i++) {
		V206DEV002(((0x200000) + 0x20000) + i * 4, sqrt_recip_rom_data[i]);
	}


	for (i = 0; i < sizeof(sqrt_rom_data) / sizeof(unsigned long); i++) {
		V206DEV002(((0x200000) + 0x30000) + i * 4, sqrt_rom_data[i]);
	}


	for (i = 0; i < sizeof(exp_rom_b_data) / sizeof(unsigned long); i++) {
		V206DEV002(((0x200000) + 0x40000) + i * 4, exp_rom_b_data[i]);
	}


	for (i = 0; i < sizeof(exp_rom_s_data) / sizeof(unsigned long); i++) {
		V206DEV002(((0x200000) + 0x50000) + i * 4, exp_rom_s_data[i]);
	}


	for (i = 0; i < sizeof(lnx_rom_data) / sizeof(unsigned long); i++) {
		V206DEV002(((0x200000) + 0x70000) + i * 4, lnx_rom_data[i]);
	}


	for (i = 0; i < sizeof(lnx_recip_rom_data) / sizeof(unsigned long); i++) {
		V206DEV002(((0x200000) + 0x80000) + i * 4, lnx_recip_rom_data[i]);
	}

	return 0;
}

int FUNC206HAL255(V206DEV025 *pDev)
{
	int ret;

	V206KDEBUG003("[INFO] 3d-hw init.\n");


	V206DEV002(((0x200000) + 0x4f00), 0);


	ret = FUNC206HAL244(pDev, (((0x200000) + 0x4000) + 0x0b14), 0);
	if (0 != ret) {
		V206KDEBUG002("[ERROR] mwv206WriteRegWithCheck failed(%d)!\n", ret);
		return -4;
	} else {
		V206DEV005("[INFO] mwv206WriteRegWithCheck successed\n");
	}


	V206DEV002((((0x200000) + 0x4000) + 0x0100),
			FUNC206LXDEV137() ? ((0) << 31)
			: ((1) << 31));


	FUNC206HAL233(pDev);


	FUNC206HAL241(pDev);
	V206KDEBUG003("[INFO] mwv206TexenvRegInit done.\n");


	pDev->V206DEV091 = V206API044;


	pDev->V206DEV092 = V206API048;

	return 0;
}

void FUNC206HAL263(V206DEV025 *pDev, struct file *filp)
{
	int        ret;
	static int eof[] = {
		MWV206SETREGCMD(((0x7000) + 0x0150), 1),
		1,
		0x85000000,
		0x81000000,
		0x81010000,
		MWV206SETREGCMD(((0x7000) + 0x0154), 1),
		1
	};

	V206DEV005("3d release, file %p.", filp);
	if (!FUNC206HAL257(pDev)) {
		return;
	}

	ret = FUNC206HAL254(pDev, filp);
	if ((ret == 0) && !FUNC206HAL257(pDev)) {
		pDev->V206DEV097.send(pDev, eof, sizeof(eof) / 4);
		pDev->V206DEV097.wait(pDev, MWV206CMDTIMEOUT);
		V206DEV005("send command 2d to %d.\n", MWV206_2D_SENDCMDMODE);
		FUNC206HAL398(pDev, 0, MWV206_2D_SENDCMDMODE, 4);
		FUNC206HAL004(pDev);
		return;
	}
}