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
#include <linux/sort.h>
#include <linux/bsearch.h>
#include <linux/string.h>
#include <asm/byteorder.h>
#include "mwv206kdevconfig.h"
#include "mwv206devconfigbin.h"
#include "mwv206dev.h"

#define V206DEVCONPARSER006(x, y)   \
	do {\
		uint8_t tmp = x;\
		x = y;\
		y = tmp;\
	} while (0)

#define MAC206HAL227(str, args...)
#define MAC206HAL228(str, args...)
#define V206DEVCONPARSER005  (2716)
#define V206DEVCONPARSER002 (2000)

static int FUNC206LXDEV016(void)
{
	static int32_t x = 0x01000000;
	return *((volatile char *) (&x)) == 0x01 ;
}

static uint32_t FUNC206HAL034(uint32_t le)
{
	return le32_to_cpu(le);
}


static int FUNC206HAL106(struct mwv206_dev_config_bin *pbinaddr)
{
	uint8_t    *ver = (uint8_t *)pbinaddr + 4;
	char       *magic = (char *)pbinaddr;

	if (pbinaddr == NULL) {
		MAC206HAL227("null pointer to binary data\n");
		return 0;
	}

	if (magic[0] != 'j' || magic[1] != 'c'
			|| magic[2] != 'f' || magic[3] != 'g') {
		MAC206HAL227("wrong magic!\n");
		return 0;
	}

	if (ver[0] != MAJOR_VER || ver[1] != MINOR_VER
			|| ver[2] != RELEASE_VER) {
		MAC206HAL228("version mismatch, proceed with faith\n");
	}

	return 1;
}

static int FUNC206HAL035(uint32_t startaddr, void *buf, uint32_t len, FileReader_t fileReader)
{
	int32_t       remain;
	int32_t       rdcnt;
	int32_t       offset;
	char          *pbuf;
	uint32_t      trycnt;

	for (remain = (int32_t)len, offset = startaddr, pbuf = (char *)buf, trycnt = 0;
			(remain > 0) && (trycnt < len + 1);
			trycnt++) {
		rdcnt = fileReader(offset, pbuf, remain);
		if (rdcnt < 0) {
			static int failcnt;

			++failcnt;
			if (failcnt > 10) {
				return -1;
			}

			continue;
		}
		remain -= rdcnt;
		offset += rdcnt;
		pbuf   += rdcnt;
	}

	if (remain > 0) {
		return -1;
	}

	return 0;
}

struct mwv206_board_config FUNC206HAL066 = {0};

sKEYVALUEITEM FUNC206HAL063[] = {
	{"board", "bl_maxlevel", V206DEVCONFIG046, (char *) &FUNC206HAL066.bl_maxlevel},
	{"board", "bl_maxDutyRatio", V206DEVCONFIG046, (char *) &FUNC206HAL066.bl_maxdutyratio},
	{"board", "bl_minDutyRatio", V206DEVCONFIG046, (char *) &FUNC206HAL066.bl_mindutyratio},
	{"board", "bl_pwmgpio", V206DEVCONFIG046, (char *) &FUNC206HAL066.bl_pwmgpio},
	{"board", "usrInfo", V206DEVCONFIG050, (char *) &FUNC206HAL066.usrinfo}
};

int32_t FUNC206HAL009(const void *ptr1, const void *ptr2)
{
	int32_t comp;
	sKEYVALUEITEM *p1 = (sKEYVALUEITEM *)ptr1;
	sKEYVALUEITEM *p2 = (sKEYVALUEITEM *)ptr2;
	comp = strncmp(p1->secName, p2->secName, strlen(p2->secName));
	if (comp < 0) {
		return -1;
	} else if (comp > 0) {
		return 1;
	}
	comp = strncmp(p1->keyName, p2->keyName, strlen(p2->keyName));
	if (comp < 0) {
		return -1;
	} else if (comp > 0) {
		return 1;
	}
	return 0;
}

static int32_t FUNC206HAL008(const char ch)
{
	if (ch >= '0' && ch <= '9') {
		return ch - '0';
	}
	if (ch >= 'a' && ch <= 'f') {
		return ch - 'a' + 10;
	}
	if (ch >= 'A' && ch <= 'F') {
		return ch - 'A' + 10;
	}
	return -1;
}

void FUNC206HAL007(const char *pSection, const char *pKey, const char *pValue)
{
	static sKEYVALUEITEM TempItem = {0};
	sKEYVALUEITEM   *pItem;
	int32_t hexstrlen;
	int32_t strsize;
	int32_t i;

	if (NULL == pSection || NULL == pKey) {
		return ;
	}
	TempItem.secName = pSection;
	TempItem.keyName = pKey;

	pItem = (sKEYVALUEITEM *)bsearch(&TempItem, FUNC206HAL063,
			sizeof(FUNC206HAL063) / sizeof(FUNC206HAL063[0]),
			sizeof(sKEYVALUEITEM), FUNC206HAL009);
	if (NULL == pItem) {
		return;
	}

	switch (pItem->valuetype) {
	case V206DEVCONFIG045:
	case V206DEVCONFIG046:
		*(uint8_t *)pItem->value = simple_strtol(pValue, NULL, 10);
		break;
	case V206DEVCONFIG047:
		*(uint32_t *)pItem->value = simple_strtol(pValue, NULL, 10);
		break;
	case V206DEVCONFIG048:
		*(int32_t *)pItem->value = simple_strtol(pValue, NULL, 10);
		break;
	case V206DEVCONFIG049:
		hexstrlen = strlen(pValue);
		if (hexstrlen < 2) {
			return;
		}
		for (i = 1; i < hexstrlen / 2; i++) {
			*(uint8_t *)(&pItem->value[i - 1]) = (FUNC206HAL008(*(pValue + i * 2)) << 4) + FUNC206HAL008(*(pValue + i * 2 + 1));
		}
		break;
	case V206DEVCONFIG050:
		strsize = strlen(pValue);
		for (i = 0; i < strsize; i++) {
			*(uint8_t *)(&pItem->value[i]) = *(pValue + i);
		}
		break;
	default:
		break;
	}
}

int FUNC206HAL162(char *pbuf)
{
	int32_t status, nextstatus;
	int32_t keycnt, i;
	const char *pszSection, *pszKey, *pszValue;
	char *p;

	status = STATE_NA;
	nextstatus = STATE_NA;
	pszSection = NULL;
	pszKey = NULL;
	pszValue = NULL;
	keycnt = 0;

	sort(FUNC206HAL063, sizeof(FUNC206HAL063) / sizeof(FUNC206HAL063[0]),
			sizeof(sKEYVALUEITEM), FUNC206HAL009, NULL);

	p = pbuf;
	for (i = 0; i < V206DEVCONPARSER002; i++, p++) {
		if ((uint8_t)(*p) == (uint8_t)0xff) {
			break;
		}
		if (*p == '\n' || i == V206DEVCONPARSER002 - 1) {
			*p = 0;
			if (STATE_VALUE == status) {
				FUNC206HAL007(pszSection, pszKey, pszValue);
				keycnt++;
			}
			if ('[' == p[1]) {
				status = STATE_SECTION;
				pszSection = NULL;
				pszKey = NULL;
				pszValue = NULL;
			} else {
				status = STATE_NA;
				pszKey = NULL;
				pszValue = NULL;
			}
			continue;
		}

		nextstatus = status;
		switch (status) {
		case STATE_NA:
			pszKey = p;
			nextstatus = STATE_KEY;
			break;
		case STATE_SECTION:
			if (*p == '[') {
				pszSection = p + 1;
			} else if (*p == ']') {
				*p = 0;
			}
			break;
		case STATE_KEY:
			if (*p == '=') {
				*p = 0;
				pszValue = p + 1;
				nextstatus = STATE_VALUE;
			}
			break;
		case STATE_VALUE:
			break;
		default:
			break;
		}
		status = nextstatus;
	}







	return 0;
}


int FUNC206HAL110(void *dev, FileReader_t fileReader, int bdefaultRearder)
{
	struct mwv206_dev_config_bin bhdr;
	struct mwv206_dev_config    *pcfg;
	struct mwv206_board_config  *pboardcfg;
	V206DEV025   *V206DEV103;
	uint32_t   crc32;
	uint8_t    *ver;
	char       *pbuf = NULL;
	uint32_t   cfgverval = 0;

	if (dev == NULL || fileReader == NULL) {
		return -1;
	}

	V206DEV103 = (V206DEV025 *)dev;
	if (V206DEV103->V206DEV107) {
		return -1;
	}

	pcfg = &V206DEV103->V206DEV105;
	pboardcfg = &V206DEV103->V206DEV106;
	memset(&bhdr, 0, sizeof(bhdr));
	memset(pcfg, 0, sizeof(*pcfg));
	memset(pboardcfg, 0, sizeof(*pboardcfg));


	if (FUNC206HAL035(0, &bhdr, sizeof(bhdr), fileReader) < 0) {
		return -1;
	}
	if (!FUNC206HAL106(&bhdr)) {
		return -1;
	}


	if (FUNC206HAL035(sizeof(bhdr), pcfg, V206DEVCONPARSER005, fileReader) < 0) {
		return -1;
	}


	bhdr.crc32 = FUNC206HAL034(bhdr.crc32);
	crc32 = FUNC206HAL107(pcfg, V206DEVCONPARSER005);
	if (crc32 != bhdr.crc32) {
		return -1;
	}

	{
		pcfg->chip.memfreq              = FUNC206HAL034(pcfg->chip.memfreq);
		pcfg->chip.corefreq             = FUNC206HAL034(pcfg->chip.corefreq);
		pcfg->chip.V206DEV038             = FUNC206HAL034(pcfg->chip.V206DEV038);
		pcfg->chip.V206DEV039             = FUNC206HAL034(pcfg->chip.V206DEV039);
		pcfg->chip.subenable            = FUNC206HAL034(pcfg->chip.subenable);
		pcfg->chip.V206DEV072           = FUNC206HAL034(pcfg->chip.V206DEV072);

		pcfg->fb.htotal                 = FUNC206HAL034(pcfg->fb.htotal);
		pcfg->fb.hactive                = FUNC206HAL034(pcfg->fb.hactive);
		pcfg->fb.hfrontporch            = FUNC206HAL034(pcfg->fb.hfrontporch);
		pcfg->fb.hsync                  = FUNC206HAL034(pcfg->fb.hsync);
		pcfg->fb.hpol                   = FUNC206HAL034(pcfg->fb.hpol);
		pcfg->fb.vtotal                 = FUNC206HAL034(pcfg->fb.vtotal);
		pcfg->fb.vactive                = FUNC206HAL034(pcfg->fb.vactive);
		pcfg->fb.vfrontporch            = FUNC206HAL034(pcfg->fb.vfrontporch);
		pcfg->fb.vsync                  = FUNC206HAL034(pcfg->fb.vsync);
		pcfg->fb.vpol                   = FUNC206HAL034(pcfg->fb.vpol);
	}
	ver = ((uint8_t *)&bhdr) + 4;
	cfgverval = ver[0] * 0x10000 + ver[1] * 0x100 + ver[2];
	if (cfgverval < 0x010001) {
		pcfg->chip.blctrl = 0;
	}

	if (cfgverval < 0x010002) {
		pcfg->chip.warningtemp = 0;
		pcfg->chip.warninggpio = 0;
	}
	if (cfgverval < 0x010003) {
		pcfg->chip.flags = 0;
	}

#if 1

	if (bdefaultRearder == 0) {
		pbuf = (char *)kzalloc(V206DEVCONPARSER002, GFP_KERNEL);
		memset(pbuf, 0, V206DEVCONPARSER002);
		if (FUNC206HAL035(sizeof(bhdr) + V206DEVCONPARSER005, pbuf, V206DEVCONPARSER002, fileReader) < 0) {
			kfree(pbuf);
			return -1;
		}
		if (FUNC206HAL162(pbuf) != 0) {
			kfree(pbuf);
			return -1;
		}


		memcpy(pboardcfg, &FUNC206HAL066, sizeof(struct mwv206_board_config));
		kfree(pbuf);
	}
#endif

#if 0
	V206KDEBUG003("[INFO] chip-memfreq = %d\n", (unsigned int)pcfg->chip.memfreq);
	V206KDEBUG003("[INFO] chip-corfreq = %d\n", (unsigned int)pcfg->chip.corefreq);
	V206KDEBUG003("[INFO] chip-ddr0size = %d\n", (unsigned int)pcfg->chip.V206DEV038);
	V206KDEBUG003("[INFO] chip-ddr1size = %d\n", (unsigned int)pcfg->chip.V206DEV039);
	V206KDEBUG003("[INFO] chip-subenable = %d\n", (unsigned int)pcfg->chip.subenable);
	V206KDEBUG003("[INFO] board-bl_pwmgpio = %d\n", (unsigned int)pboardcfg->bl_pwmgpio);
	V206KDEBUG003("[INFO] board-bl_maxlevel = %d\n", (unsigned int)pboardcfg->bl_maxlevel);
	V206KDEBUG003("[INFO] board-bl_maxDutyRatio = %d\n", (unsigned int)pboardcfg->bl_maxdutyratio);
	V206KDEBUG003("[INFO] board-bl_minDutyRatio = %d\n", (unsigned int)pboardcfg->bl_mindutyratio);
	V206KDEBUG003("[INFO] board-usrinfo = %s\n", pboardcfg->usrinfo);
#endif


	V206DEV103->V206DEV107 = 1;

	return 0;
}

int jmVbiosCfgVerGet(void *dev, FileReader_t fileReader)
{
	V206DEV025   *V206DEV103;
	unsigned char *pbuf = NULL;
	int i;

	if (dev == NULL || fileReader == NULL) {
		return -1;
	}

	V206DEV103 = (V206DEV025 *)dev;

	pbuf = (char *)kzalloc(sizeof(V206DEV103->vbioscfgver), GFP_KERNEL);

	if (FUNC206HAL035(0, pbuf, sizeof(V206DEV103->vbioscfgver) - 1, fileReader) < 0) {
		kfree(pbuf);
		return -1;
	}

	for (i = 0; i < sizeof(V206DEV103->vbioscfgver); i++) {
		if ((pbuf[i] == 0xFF)) {
			pbuf[i] = 0;
		}
	}

	memcpy(&V206DEV103->vbioscfgver, pbuf, sizeof(V206DEV103->vbioscfgver));

	kfree(pbuf);

	return 0;
}


static int mwv206_ddr_wrap(V206DEV025 *pDev, int ddrno, int addr)
{
	uint8_t  addr0_data[2] = {0, 0};
	uint8_t  addr1_data[2] = {'J', 'M'};
	uint32_t V206IOCTLCMD009 = 0;

	V206IOCTLCMD009 = addr * 1024 * 1024;

	FUNC206HAL229(pDev, 0, addr0_data, 2);
	FUNC206HAL229(pDev, V206IOCTLCMD009, addr1_data, 2);

	FUNC206HAL228(pDev, addr0_data, 0, 2);

	if (addr0_data[0] == 0) {
		return 0;
	} else {
		return 1;
	}
}

int mwv206_ddrsize_probe(V206DEV025 *pDev,  int ddrno)
{
	if (!mwv206_ddr_wrap(pDev, ddrno, 1024)) {
		return 2048;
	}
	if (!mwv206_ddr_wrap(pDev, ddrno, 512)) {
		return 1024;
	}
	if (!mwv206_ddr_wrap(pDev, ddrno, 256)) {
		return 512;
	}
	V206KDEBUG003("[INFO] use default mininmum ddr%d size 256M\n", ddrno);
	return 256;
}

static void mwv206_fix_memfreq(V206DEV025 *pDev)
{
	uint32_t chiptype = pDev->chiptype;

	if (chiptype == 0x0A || chiptype == 0x05
			|| chiptype == 0x08 || chiptype == 0x10
			|| chiptype == 0x09 || chiptype == 0x0F) {
		pDev->V206DEV105.chip.memfreq = 400;
	} else if (chiptype == 0x03 || chiptype == 0x06) {
		pDev->V206DEV105.chip.memfreq = 465;
	} else {
		pDev->V206DEV105.chip.memfreq = 533;
	}
}

static void mwv206_validate_port(V206DEV025 *pDev)
{
	uint32_t chiptype = pDev->chiptype;
	uint8_t  is7201chip;

	is7201chip = (chiptype == 0x05)
		|| (chiptype == 0x08)
		|| (chiptype == 0x0A);

	if (is7201chip) {
		V206KDEBUG003("[INFO] Find MWG6439!");

		V206DEVCONFIG031(pDev->V206DEV105.hdmi[2].flags, 0);
		V206DEVCONFIG031(pDev->V206DEV105.hdmi[3].flags, 0);


		V206DEVCONFIG031(pDev->V206DEV105.dvo[0].flags, 0);
		V206DEVCONFIG031(pDev->V206DEV105.dvo[1].flags, 0);
	}
}



struct mwv206_dev_config *FUNC206HAL109(void *dev)
{
	V206DEV025 *V206DEV103 = (V206DEV025 *) dev;

	if (V206DEV103 == NULL) {
		return NULL;
	}

	return  &V206DEV103->V206DEV105;
}


static const uint32_t FUNC206HAL047[256] = {
	0x00000000L, 0xF26B8303L, 0xE13B70F7L, 0x1350F3F4L,
	0xC79A971FL, 0x35F1141CL, 0x26A1E7E8L, 0xD4CA64EBL,
	0x8AD958CFL, 0x78B2DBCCL, 0x6BE22838L, 0x9989AB3BL,
	0x4D43CFD0L, 0xBF284CD3L, 0xAC78BF27L, 0x5E133C24L,
	0x105EC76FL, 0xE235446CL, 0xF165B798L, 0x030E349BL,
	0xD7C45070L, 0x25AFD373L, 0x36FF2087L, 0xC494A384L,
	0x9A879FA0L, 0x68EC1CA3L, 0x7BBCEF57L, 0x89D76C54L,
	0x5D1D08BFL, 0xAF768BBCL, 0xBC267848L, 0x4E4DFB4BL,
	0x20BD8EDEL, 0xD2D60DDDL, 0xC186FE29L, 0x33ED7D2AL,
	0xE72719C1L, 0x154C9AC2L, 0x061C6936L, 0xF477EA35L,
	0xAA64D611L, 0x580F5512L, 0x4B5FA6E6L, 0xB93425E5L,
	0x6DFE410EL, 0x9F95C20DL, 0x8CC531F9L, 0x7EAEB2FAL,
	0x30E349B1L, 0xC288CAB2L, 0xD1D83946L, 0x23B3BA45L,
	0xF779DEAEL, 0x05125DADL, 0x1642AE59L, 0xE4292D5AL,
	0xBA3A117EL, 0x4851927DL, 0x5B016189L, 0xA96AE28AL,
	0x7DA08661L, 0x8FCB0562L, 0x9C9BF696L, 0x6EF07595L,
	0x417B1DBCL, 0xB3109EBFL, 0xA0406D4BL, 0x522BEE48L,
	0x86E18AA3L, 0x748A09A0L, 0x67DAFA54L, 0x95B17957L,
	0xCBA24573L, 0x39C9C670L, 0x2A993584L, 0xD8F2B687L,
	0x0C38D26CL, 0xFE53516FL, 0xED03A29BL, 0x1F682198L,
	0x5125DAD3L, 0xA34E59D0L, 0xB01EAA24L, 0x42752927L,
	0x96BF4DCCL, 0x64D4CECFL, 0x77843D3BL, 0x85EFBE38L,
	0xDBFC821CL, 0x2997011FL, 0x3AC7F2EBL, 0xC8AC71E8L,
	0x1C661503L, 0xEE0D9600L, 0xFD5D65F4L, 0x0F36E6F7L,
	0x61C69362L, 0x93AD1061L, 0x80FDE395L, 0x72966096L,
	0xA65C047DL, 0x5437877EL, 0x4767748AL, 0xB50CF789L,
	0xEB1FCBADL, 0x197448AEL, 0x0A24BB5AL, 0xF84F3859L,
	0x2C855CB2L, 0xDEEEDFB1L, 0xCDBE2C45L, 0x3FD5AF46L,
	0x7198540DL, 0x83F3D70EL, 0x90A324FAL, 0x62C8A7F9L,
	0xB602C312L, 0x44694011L, 0x5739B3E5L, 0xA55230E6L,
	0xFB410CC2L, 0x092A8FC1L, 0x1A7A7C35L, 0xE811FF36L,
	0x3CDB9BDDL, 0xCEB018DEL, 0xDDE0EB2AL, 0x2F8B6829L,
	0x82F63B78L, 0x709DB87BL, 0x63CD4B8FL, 0x91A6C88CL,
	0x456CAC67L, 0xB7072F64L, 0xA457DC90L, 0x563C5F93L,
	0x082F63B7L, 0xFA44E0B4L, 0xE9141340L, 0x1B7F9043L,
	0xCFB5F4A8L, 0x3DDE77ABL, 0x2E8E845FL, 0xDCE5075CL,
	0x92A8FC17L, 0x60C37F14L, 0x73938CE0L, 0x81F80FE3L,
	0x55326B08L, 0xA759E80BL, 0xB4091BFFL, 0x466298FCL,
	0x1871A4D8L, 0xEA1A27DBL, 0xF94AD42FL, 0x0B21572CL,
	0xDFEB33C7L, 0x2D80B0C4L, 0x3ED04330L, 0xCCBBC033L,
	0xA24BB5A6L, 0x502036A5L, 0x4370C551L, 0xB11B4652L,
	0x65D122B9L, 0x97BAA1BAL, 0x84EA524EL, 0x7681D14DL,
	0x2892ED69L, 0xDAF96E6AL, 0xC9A99D9EL, 0x3BC21E9DL,
	0xEF087A76L, 0x1D63F975L, 0x0E330A81L, 0xFC588982L,
	0xB21572C9L, 0x407EF1CAL, 0x532E023EL, 0xA145813DL,
	0x758FE5D6L, 0x87E466D5L, 0x94B49521L, 0x66DF1622L,
	0x38CC2A06L, 0xCAA7A905L, 0xD9F75AF1L, 0x2B9CD9F2L,
	0xFF56BD19L, 0x0D3D3E1AL, 0x1E6DCDEEL, 0xEC064EEDL,
	0xC38D26C4L, 0x31E6A5C7L, 0x22B65633L, 0xD0DDD530L,
	0x0417B1DBL, 0xF67C32D8L, 0xE52CC12CL, 0x1747422FL,
	0x49547E0BL, 0xBB3FFD08L, 0xA86F0EFCL, 0x5A048DFFL,
	0x8ECEE914L, 0x7CA56A17L, 0x6FF599E3L, 0x9D9E1AE0L,
	0xD3D3E1ABL, 0x21B862A8L, 0x32E8915CL, 0xC083125FL,
	0x144976B4L, 0xE622F5B7L, 0xF5720643L, 0x07198540L,
	0x590AB964L, 0xAB613A67L, 0xB831C993L, 0x4A5A4A90L,
	0x9E902E7BL, 0x6CFBAD78L, 0x7FAB5E8CL, 0x8DC0DD8FL,
	0xE330A81AL, 0x115B2B19L, 0x020BD8EDL, 0xF0605BEEL,
	0x24AA3F05L, 0xD6C1BC06L, 0xC5914FF2L, 0x37FACCF1L,
	0x69E9F0D5L, 0x9B8273D6L, 0x88D28022L, 0x7AB90321L,
	0xAE7367CAL, 0x5C18E4C9L, 0x4F48173DL, 0xBD23943EL,
	0xF36E6F75L, 0x0105EC76L, 0x12551F82L, 0xE03E9C81L,
	0x34F4F86AL, 0xC69F7B69L, 0xD5CF889DL, 0x27A40B9EL,
	0x79B737BAL, 0x8BDCB4B9L, 0x988C474DL, 0x6AE7C44EL,
	0xBE2DA0A5L, 0x4C4623A6L, 0x5F16D052L, 0xAD7D5351L
};


uint32_t FUNC206HAL107(void *buf, uint32_t len)
{
#define V206DEVCONPARSER001  0xffffffffL
#define V206DEVCONPARSER007 0xffffffffL
	uint32_t crc32;
	uint8_t *pbuf;

	crc32 = V206DEVCONPARSER001;
	pbuf = (uint8_t *) buf;

	while (len) {
		len--;
		crc32 = FUNC206HAL047[(crc32 ^ *pbuf++) & 0xFFL] ^ (crc32 >> 8);
	}

	return crc32 ^ V206DEVCONPARSER007;
}

jme_chip_grade mwv206_get_chipgrade(V206DEV025 *pDev)
{
	unsigned int chiptype = pDev->chiptype;
	jme_chip_grade grade;

	switch (chiptype) {

	case 9:
	case 0xf:
	case 0xa:
	case 0x10:
		grade = JMV_CHIP_COMMERCIAL;
		break;

	case 2:
	case 0xc:
	case 6:
	case 7:
	case 0xe:
	case 8:
		grade = JMV_CHIP_INDUSTRIAL;
		break;

	case 1:
	case 0xb:
	case 3:
	case 4:
	case 0xd:
	case 5:
		grade = JMV_CHIP_MILITARY;
		break;
	default:
		V206KDEBUG003("[INFO] chiptype = 0, default to MILI\n");
		grade = JMV_CHIP_MILITARY;
		break;
	}
	return grade;
}

void mwv206_validate_cfg(V206DEV025 *pDev)
{
	struct mwv206_board_config *board = &pDev->V206DEV106;
	struct mwv206_chip_config *chip = &pDev->V206DEV105.chip;
	struct mwv206_port_config *vga1 = &pDev->V206DEV105.vga[1];
	int V206DEV038, V206DEV039;


	if (pDev->isdevcfgdefault || chip->memfreq == 533) {
		mwv206_fix_memfreq(pDev);
	}

	mwv206_validate_port(pDev);


	if (chip->V206DEV039 == 0) {
		pDev->V206DEV041 = 1;
	} else {
		pDev->V206DEV041 = 2;
	}
	V206KDEBUG003("[INFO] ddrcnt = %d.\n", pDev->V206DEV041);

	V206DEV038 = mwv206_ddrsize_probe(pDev, 0);
	V206DEV039 = pDev->V206DEV041 < 2 ? 0 : mwv206_ddrsize_probe(pDev, 1);

	if (CFG_DDR_IS_PROBE(chip->flags)) {
		chip->V206DEV038 = V206DEV038;
		chip->V206DEV039 = V206DEV039;
	} else {
		if (chip->V206DEV038 > V206DEV038) {
			V206KDEBUG003("[INFO] ddr0size(%dMB) is larger than probed size(%dMB)\n",
				       chip->V206DEV038, V206DEV038);
			V206KDEBUG003("[INFO] clamp ddr0size to probed size\n");
			chip->V206DEV038 = V206DEV038;
		}
		if (chip->V206DEV039 > V206DEV039) {
			V206KDEBUG003("[INFO] ddr1size(%dMB) is larger than probed size(%dMB)\n",
				       chip->V206DEV039, V206DEV039);
			V206KDEBUG003("[INFO] clamp ddr1size to probed size\n");
			chip->V206DEV039 = V206DEV039;
		}

	}

	if (chip->V206DEV072 > (chip->V206DEV038 + chip->V206DEV039) / 4) {
		chip->V206DEV072 = chip->V206DEV038 / 4;
	}
	if (chip->V206DEV072 < 128) {
		chip->V206DEV072 = 128;
	}

	if (board->bl_maxlevel == 0) {
		V206KDEBUG003("[INFO] illegal backlight param, bl_maxlevel = 0, use default!\n");
		board->bl_maxlevel = 19;
		board->bl_maxdutyratio = 100;
		board->bl_mindutyratio = 5;
	}


	switch (mwv206_get_chipgrade(pDev)) {
	case JMV_CHIP_INDUSTRIAL:
	case JMV_CHIP_MILITARY:
		if (pDev->isdevcfgdefault) {
			FUNC206HAL130(vga1);
			V206DEVCONFIG031(vga1->flags, 1);
			vga1->i2cchan = 7;
			V206KDEBUG003("[INFO] vga1 is enabled\n");
		}
		break;
	default:
		break;
	}
}