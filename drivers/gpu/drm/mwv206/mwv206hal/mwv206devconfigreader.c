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
#include <asm/byteorder.h>
#include "jmspi.h"
#include "mwv206kdevconfig.h"
#include "mwv206devconfigbin.h"
#include "mwv206ioctl.h"

#define V206DEVCONPARSER006(x, y) \
	do {\
		uint8_t tmp = x;\
		x = y;\
		y = tmp;\
	} while (0)

static int FUNC206LXDEV016(void)
{
	static int32_t x = 0x01000000;
	return *((volatile char *) (&x)) == 0x01 ;
}

static uint32_t FUNC206HAL045(uint32_t cpu32)
{
	return cpu_to_le32(cpu32);
}

static void FUNC206HAL124(struct mwv206_chip_config *pchip)
{
	pchip->corefreq    = 1000;
	pchip->memfreq     = 400;
	pchip->V206DEV038    = 1024;
	pchip->V206DEV039    = 1024;
	pchip->V206DEV072  = 512;

	pchip->subenable   = 0;
	pchip->pcictrl     = 1;
	pchip->pcilane     = 8;
	pchip->pcibar      = 0;
	pchip->pcirom      = 0;
	pchip->blctrl      = 0;
	pchip->warningtemp = 0;
	pchip->warninggpio = 0;
	pchip->flags       = 0;
	CFG_DDR_SET_PROBE(pchip->flags, 1);
}

void FUNC206HAL130(struct mwv206_port_config *pport)
{
	uint32_t i = 0;

	pport->flags = 0;

	V206DEVCONFIG031(pport->flags, 0);
	V206DEVCONFIG032(pport->flags, 0);
	V206DEVCONFIG033(pport->flags, 0);
	V206DEVCONFIG034(pport->flags, 0);
	V206DEVCONFIG041(pport->flags, V206DEVCONFIG011);

	pport->i2cchan = V206DEVCONFIG017;
	pport->sameas = V206DEVCONFIG018;
	pport->bitmode = MWV206K_LVDS_MODE_24BIT_VESA;
	pport->stdedid = V206DEVCONFIG020;

	for (i = 0; i < sizeof(pport->edid); i++) {
		pport->edid[i] = 0;
	}
}

static void FUNC206HAL127(struct mwv206_fb_config *pcfg)
{
	pcfg->mode         = MWV206K_DPMODE_1920x1080x60HZ;
	pcfg->V206DEV079  = 0;
	pcfg->rfsrate      = 0;
	pcfg->resv         = 0;
	pcfg->htotal       = 0;
	pcfg->hactive      = 0;
	pcfg->hfrontporch  = 0;
	pcfg->hsync        = 0;
	pcfg->hpol         = 0;
	pcfg->vtotal       = 0;
	pcfg->vactive      = 0;
	pcfg->vfrontporch  = 0;
	pcfg->vsync        = 0;
	pcfg->vpol         = 0;
}



static void FUNC206HAL126(struct mwv206_dev_config *pcfg)
{
	int port;

	FUNC206HAL124(&pcfg->chip);
	FUNC206HAL127(&pcfg->fb);


	for (port = 0; port < V206CONFIG006; port++) {
		FUNC206HAL130(&pcfg->hdmi[port]);
		pcfg->hdmi[port].i2cchan = port + 2;
		V206DEVCONFIG031(pcfg->hdmi[port].flags, 1);
	}


	for (port = 0; port < 1; port++) {
		FUNC206HAL130(&pcfg->vga[port]);
		pcfg->vga[port].i2cchan = port + 6;
		V206DEVCONFIG031(pcfg->vga[port].flags, 1);
	}

	V206DEVCONFIG031(pcfg->vga[1].flags, 0);


	FUNC206HAL130(&pcfg->lvds[0]);
	pcfg->lvds[0].i2cchan = 0;
	V206DEVCONFIG031(pcfg->lvds[0].flags, 1);


	FUNC206HAL130(&pcfg->dvo[0]);
	pcfg->dvo[0].i2cchan = 1;
	V206DEVCONFIG031(pcfg->dvo[0].flags, 1);
}




static unsigned char FUNC206HAL194[128] = {
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x0C, 0xF4, 0x0C, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x01, 0x0F, 0x01, 0x03, 0x80, 0x00, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xAF, 0xCF, 0x00, 0xA9, 0x40, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x48, 0x3F, 0x40, 0x30, 0x62, 0xB0, 0x32, 0x40, 0x40, 0xC0,
	0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x10, 0x00, 0x32, 0x78, 0x3C,
	0x4B, 0x0B, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x63,
	0x6F, 0x6E, 0x67, 0x61, 0x74, 0x65, 0x63, 0x20, 0x41, 0x47, 0x0A, 0x20, 0x00, 0x00, 0x00, 0x0E,
	0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x0C,
};


static unsigned char FUNC206HAL195[128] = {
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x0C, 0xF4, 0x0C, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x01, 0x0F, 0x01, 0x03, 0x80, 0x00, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xAF, 0xCF, 0x00, 0xd1, 0xc0, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x48, 0x3F, 0x80, 0x30, 0x72, 0x38, 0x32, 0x40, 0x40, 0xC0,
	0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x10, 0x00, 0x32, 0x78, 0x3C,
	0x4B, 0x0B, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x63,
	0x6F, 0x6E, 0x67, 0x61, 0x74, 0x65, 0x63, 0x20, 0x41, 0x47, 0x0A, 0x20, 0x00, 0x00, 0x00, 0x0E,
	0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x8C,
};


static void FUNC206HAL128(struct mwv206_dev_config *pcfg)
{
	int port;

	FUNC206HAL124(&pcfg->chip);
	FUNC206HAL127(&pcfg->fb);
	pcfg->fb.mode = MWV206K_DPMODE_1600x1200x60HZ;

	for (port = 0; port < 4; port++) {
		FUNC206HAL130(&pcfg->hdmi[port]);
		pcfg->hdmi[port].i2cchan = port + 2;
	}

	V206DEVCONFIG031(pcfg->hdmi[0].flags, 1);
	V206DEVCONFIG041(pcfg->hdmi[0].flags, V206DEVCONFIG011);


	V206DEVCONFIG031(pcfg->hdmi[1].flags, 1);
	V206DEVCONFIG041(pcfg->hdmi[1].flags, V206DEVCONFIG011);


	V206DEVCONFIG031(pcfg->hdmi[2].flags, 1);
	V206DEVCONFIG041(pcfg->hdmi[2].flags, V206DEVCONFIG011);


	V206DEVCONFIG031(pcfg->hdmi[3].flags, 1);
	V206DEVCONFIG041(pcfg->hdmi[3].flags, V206DEVCONFIG011);

	for (port = 0; port < 2; port++) {
		FUNC206HAL130(&pcfg->vga[port]);
		pcfg->vga[port].i2cchan = port + 6;
	}

	V206DEVCONFIG031(pcfg->vga[0].flags, 1);
	V206DEVCONFIG041(pcfg->vga[0].flags, V206DEVCONFIG011);

	pcfg->vga[0].stdedid = MWV206K_DPMODE_1600x1200x60HZ;


	V206DEVCONFIG031(pcfg->vga[1].flags, 1);
	V206DEVCONFIG041(pcfg->vga[1].flags, V206DEVCONFIG011);
	pcfg->vga[0].stdedid = MWV206K_DPMODE_1600x1200x60HZ;


	for (port = 0; port < 2; port++) {
		FUNC206HAL130(&pcfg->lvds[port]);
		pcfg->lvds[port].i2cchan = port + 0;
	}


	V206DEVCONFIG031(pcfg->lvds[0].flags, 1);
	V206DEVCONFIG041(pcfg->lvds[0].flags, V206DEVCONFIG013);
	pcfg->lvds[0].stdedid = MWV206K_DPMODE_1600x1200x60HZ;
	V206DEVCONFIG032(pcfg->lvds[0].flags, 1);
	V206DEVCONFIG033(pcfg->lvds[0].flags, 0);
	V206DEVCONFIG035(pcfg->lvds[0].flags, 1);

	pcfg->lvds[0].bitmode = MWV206K_LVDS_MODE_18BIT_LDI_SHAKE;

	{
		int i;
		for (i = 0; i < (int)sizeof(FUNC206HAL194); i++) {
			pcfg->lvds[0].edid[i] = FUNC206HAL194[i];
		}
	}

	V206DEVCONFIG031(pcfg->lvds[1].flags, 0);

	for (port = 0; port < 2; port++) {
		FUNC206HAL130(&pcfg->dvo[port]);
		pcfg->dvo[port].i2cchan = V206DEVCONFIG017;
	}
}

static void FUNC206HAL129(struct mwv206_dev_config *pcfg)
{
	int port;

	FUNC206HAL124(&pcfg->chip);
	FUNC206HAL127(&pcfg->fb);
	pcfg->fb.mode = MWV206K_DPMODE_1920x1080x60HZ;

	for (port = 0; port < 4; port++) {
		FUNC206HAL130(&pcfg->hdmi[port]);
		pcfg->hdmi[port].i2cchan = port + 2;
	}

	V206DEVCONFIG031(pcfg->hdmi[0].flags, 1);
	V206DEVCONFIG041(pcfg->hdmi[0].flags, V206DEVCONFIG011);


	V206DEVCONFIG031(pcfg->hdmi[1].flags, 1);
	V206DEVCONFIG041(pcfg->hdmi[1].flags, V206DEVCONFIG011);


	V206DEVCONFIG031(pcfg->hdmi[2].flags, 1);
	V206DEVCONFIG041(pcfg->hdmi[2].flags, V206DEVCONFIG011);


	V206DEVCONFIG031(pcfg->hdmi[3].flags, 1);
	V206DEVCONFIG041(pcfg->hdmi[3].flags, V206DEVCONFIG011);

	for (port = 0; port < 2; port++) {
		FUNC206HAL130(&pcfg->vga[port]);
		pcfg->vga[port].i2cchan = port + 6;
	}

	V206DEVCONFIG031(pcfg->vga[0].flags, 1);
	V206DEVCONFIG041(pcfg->vga[0].flags, V206DEVCONFIG011);

	pcfg->vga[0].stdedid = MWV206K_DPMODE_1600x1200x60HZ;


	V206DEVCONFIG031(pcfg->vga[1].flags, 1);
	V206DEVCONFIG041(pcfg->vga[1].flags, V206DEVCONFIG011);
	pcfg->vga[0].stdedid = MWV206K_DPMODE_1600x1200x60HZ;


	for (port = 0; port < 2; port++) {
		FUNC206HAL130(&pcfg->lvds[port]);
		pcfg->lvds[port].i2cchan = port + 0;
	}


	V206DEVCONFIG031(pcfg->lvds[0].flags, 1);
	V206DEVCONFIG041(pcfg->lvds[0].flags, V206DEVCONFIG013);
	pcfg->lvds[0].stdedid = MWV206K_DPMODE_1920x1080x60HZ;
	V206DEVCONFIG032(pcfg->lvds[0].flags, 1);
	V206DEVCONFIG033(pcfg->lvds[0].flags, 0);
	V206DEVCONFIG035(pcfg->lvds[0].flags, 1);

	pcfg->lvds[0].bitmode = MWV206K_LVDS_MODE_24BIT_VESA;

	{
		int i;
		for (i = 0; i <  (int)sizeof(FUNC206HAL195); i++) {
			pcfg->lvds[0].edid[i] = FUNC206HAL195[i];
		}
	}

	V206DEVCONFIG031(pcfg->lvds[1].flags, 0);

	for (port = 0; port < 2; port++) {
		FUNC206HAL130(&pcfg->dvo[port]);
		pcfg->dvo[port].i2cchan = V206DEVCONFIG017;
	}
}

static void FUNC206HAL125(struct mwv206_dev_config_bin *pbin, struct mwv206_dev_config *pcfg)
{
	pbin->magic[0] = 'j';
	pbin->magic[1] = 'c';
	pbin->magic[2] = 'f';
	pbin->magic[3] = 'g';

	pbin->version[0] = MAJOR_VER;
	pbin->version[1] = MINOR_VER;
	pbin->version[2] = RELEASE_VER;

	pbin->resved = 0;

	pbin->crc32 = FUNC206HAL107(pcfg, sizeof(*pcfg));
	pbin->crc32 = FUNC206HAL045(pbin->crc32);
}

int32_t FUNC206HAL108(int32_t offset, void *buf, uint32_t len)
{
	static struct mwv206_dev_config cfg;
	static struct mwv206_dev_config_bin bin;
	static int isCfgInited;
	char   *psrcbuf;
	char   *pdstbuf;

	if (buf == NULL) {
		return -1;
	}
	if (len == 0) {
		return 0;
	}

	if (!isCfgInited) {
		FUNC206HAL126(&cfg);


		cfg.chip.memfreq            = FUNC206HAL045(cfg.chip.memfreq);
		cfg.chip.corefreq           = FUNC206HAL045(cfg.chip.corefreq);
		cfg.chip.V206DEV038           = FUNC206HAL045(cfg.chip.V206DEV038);
		cfg.chip.V206DEV039           = FUNC206HAL045(cfg.chip.V206DEV039);
		cfg.chip.subenable          = FUNC206HAL045(cfg.chip.subenable);
		cfg.chip.V206DEV072         = FUNC206HAL045(cfg.chip.V206DEV072);

		cfg.fb.htotal               = FUNC206HAL045(cfg.fb.htotal);
		cfg.fb.hactive              = FUNC206HAL045(cfg.fb.hactive);
		cfg.fb.hfrontporch          = FUNC206HAL045(cfg.fb.hfrontporch);
		cfg.fb.hsync                = FUNC206HAL045(cfg.fb.hsync);
		cfg.fb.hpol                 = FUNC206HAL045(cfg.fb.hpol);
		cfg.fb.vtotal               = FUNC206HAL045(cfg.fb.vtotal);
		cfg.fb.vactive              = FUNC206HAL045(cfg.fb.vactive);
		cfg.fb.vfrontporch          = FUNC206HAL045(cfg.fb.vfrontporch);
		cfg.fb.vsync                = FUNC206HAL045(cfg.fb.vsync);
		cfg.fb.vpol                 = FUNC206HAL045(cfg.fb.vpol);

		FUNC206HAL125(&bin, &cfg);

		isCfgInited = 1;
	}

	pdstbuf = (char *) buf;


	if (offset >= 0 && (uint32_t)offset < sizeof(bin)) {
		psrcbuf = (char *)&bin;
		pdstbuf[0] = psrcbuf[offset];
		return 1;
	}

	if ((uint32_t)offset < sizeof(bin) + sizeof(cfg)) {
		psrcbuf = (char *)&cfg;
		pdstbuf[0] = psrcbuf[offset - sizeof(bin)];
		return 1;
	}

	return -1;
}



static  JMSPI    FUNC206HAL058;

int32_t FUNC206HAL112(void  *handle)
{

	FUNC206HAL058 = (JMSPI) handle;

	return 0;
}

#define MAC206HAL195  0
#define MAC206HAL186    0x3
#define MAC206HAL190 0x60000
#define MAC206HAL191 0x80000
#define SPI_FLASH_VBIOSCFGVER_BASE 0x5FFB0
#define MAC206HAL192 0x10000
#define MAC206HAL182   1
#define MAC206HAL183   0
#define MAC206HAL193   0
#define MAC206HAL194   0

int32_t jmCfgSpiFlashReadEx(uint32_t faddr, int32_t offset, void *buf, uint32_t len)
{
	unsigned int  sendlen;
	unsigned int  recvlen;
	char          readcmd[4];
	uint32_t      spiaddr;
	int           result;


	if (offset < 0 || offset + len > MAC206HAL192 || FUNC206HAL058 == NULL) {
		return -1;
	}

	spiaddr = offset + faddr;

	readcmd[0] = MAC206HAL186;
	readcmd[1] = (spiaddr & 0xffffff) >> 16;
	readcmd[2] = (spiaddr & 0xffff) >> 8;
	readcmd[3] = (spiaddr & 0xff);

	sendlen = 0;
	recvlen = len;
	if (recvlen > 32) {
		recvlen = 32;
	}

	result = FUNC206HAL172(FUNC206HAL058,
			MAC206HAL195,
			readcmd,
			4,
			NULL,
			&sendlen,
			buf,
			&recvlen,
			4,
			len + 4,
			MAC206HAL182,
			MAC206HAL183,
			MAC206HAL193,
			MAC206HAL194);

	if (result != 0) {
		return -1;
	}

	return recvlen;
}

int32_t FUNC206HAL113(int32_t offset, void *buf, uint32_t len)
{
	return jmCfgSpiFlashReadEx(MAC206HAL191, offset, buf, len);
}

int32_t FUNC206HAL111(int32_t offset, void *buf, uint32_t len)
{
	return jmCfgSpiFlashReadEx(MAC206HAL190, offset, buf, len);
}

int32_t jmVbiosCfgVerSpiFlashReader(int32_t offset, void *buf, uint32_t len)
{
	return jmCfgSpiFlashReadEx(SPI_FLASH_VBIOSCFGVER_BASE, offset, buf, len);
}