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
#ifndef _MWV206K_DEV_CONFIG_H_
#define  _MWV206K_DEV_CONFIG_H_

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#include "mwv206.h"

#define EDID_ITEM_LEN                    (256)
#define V206CONFIG010                    (512)

#ifdef __cplusplus
extern "C" {
#endif


#define V206DEVCONFIG001(flag, bitpos, bit)           ((flag) = ((flag) & ~(1<<(bitpos))) | (((bit)&0x1)<<(bitpos)))
#define V206DEVCONFIG002(flag, bitpos)                (((flag) >> (bitpos))&0x1)
#define V206DEVCONFIG003(flag, bitmask, shift, dat)  ((flag) = ((flag) & (bitmask))|(((dat)<<(shift))&~(bitmask)))
#define V206DEVCONFIG004(flag, bitmask, shift)       (((flag)&~(bitmask))>>(shift))

#define CFG_DDR_IS_PROBE(flags)                  (V206DEVCONFIG002(flags, 0))
#define CFG_DDR_SET_PROBE(flags, bit)            (V206DEVCONFIG001(flags, 0, bit))

struct mwv206_chip_config{

	uint32_t memfreq;
	uint32_t corefreq;
	uint32_t V206DEV038;
	uint32_t V206DEV039;

	uint32_t subenable;

	uint8_t  pcictrl;
	uint8_t  pcilane;
	uint8_t  pcibar;
	uint8_t  pcirom;
	uint32_t V206DEV072;

	uint8_t  flags;
	uint8_t  blctrl;
	uint8_t  warningtemp;
	uint8_t  warninggpio;
};

struct mwv206_fb_config{

	uint8_t    mode;
#define CFG_FB_MODE_USER            (0xff)


	uint8_t    V206DEV079;
	uint8_t    rfsrate;
	uint8_t    resv;
	uint32_t   htotal;
	uint32_t   hactive;
	uint32_t   hfrontporch;
	uint32_t   hsync;
	uint32_t   hpol;
	uint32_t   vtotal;
	uint32_t   vactive;
	uint32_t   vfrontporch;
	uint32_t   vsync;
	uint32_t   vpol;
};

struct mwv206_port_config{

	uint8_t flags;
#define V206DEVCONFIG005               (7)
#define V206DEVCONFIG006            (6)
#define V206DEVCONFIG007            (5)
#define V206DEVCONFIG008                 (4)
#define V206DEVCONFIG009            (0xf3)
#define V206DEVCONFIG010           (0x02)
#define V206DEVCONFIG011            (0x00)
#define V206DEVCONFIG012             (0x01)
#define V206DEVCONFIG013            (0x02)
#define V206DEVCONFIG014              (0x03)
#define V206DEVCONFIG015           (1)


	uint8_t i2cchan;
	uint8_t sameas;
	uint8_t bitmode;
	uint8_t stdedid;
#define V206DEVCONFIG016                 (0xff)
#define V206DEVCONFIG017              (0xff)
#define V206DEVCONFIG018               (0xff)
#define V206DEVCONFIG019               (0xff)
#define V206DEVCONFIG020              (0x7f)

#define CFG_PORT_ISDVI_BIT               (7)

	uint8_t mainScreen;
	uint8_t resv0;
	uint8_t resv1;
#define EDID_ITEM_LEN                    (256)
	uint8_t edid[EDID_ITEM_LEN];

};

#define V206DEVCONFIG021            (0)
#define V206DEVCONFIG022            (1)
#define V206DEVCONFIG023            (2)
#define V206DEVCONFIG024            (3)
#define V206DEVCONFIG025             (4)
#define V206DEVCONFIG026             (5)
#define V206DEVCONFIG027            (6)
#define V206DEVCONFIG028            (7)
#define V206DEVCONFIG029             (8)
#define V206DEVCONFIG030             (9)


#define V206DEVCONFIG031(flags, enable)          V206DEVCONFIG001(flags, V206DEVCONFIG005, !!(enable))
#define V206DEVCONFIG032(flags, dualpixel)    V206DEVCONFIG001(flags, V206DEVCONFIG006, !!(dualpixel))
#define V206DEVCONFIG033(flags, pixelchan)    V206DEVCONFIG001(flags, V206DEVCONFIG007, !!(pixelchan))
#define V206DEVCONFIG034(flags, enable)            V206DEVCONFIG001(flags, V206DEVCONFIG008, !!(enable))
#define V206DEVCONFIG035(flags, enable)            V206DEVCONFIG001(flags, V206DEVCONFIG015, !!(enable))

#define V206DEVCONFIG036(flags)                  V206DEVCONFIG002(flags, V206DEVCONFIG005)
#define V206DEVCONFIG037(flags)               V206DEVCONFIG002(flags, V206DEVCONFIG006)
#define V206DEVCONFIG038(flags)               V206DEVCONFIG002(flags, V206DEVCONFIG007)
#define V206DEVCONFIG039(flags)                    V206DEVCONFIG002(flags, V206DEVCONFIG008)
#define V206DEVCONFIG040(flags)                    V206DEVCONFIG002(flags, V206DEVCONFIG015)


#define V206DEVCONFIG041(flags, mode)          V206DEVCONFIG003(flags, V206DEVCONFIG009, V206DEVCONFIG010, mode)
#define V206DEVCONFIG042(flags)                V206DEVCONFIG004(flags, V206DEVCONFIG009, V206DEVCONFIG010)


#define V206DEVCONFIG043(flags, enable)            V206DEVCONFIG001(flags, CFG_PORT_ISDVI_BIT, !!(enable))
#define V206DEVCONFIG044(flags)                    V206DEVCONFIG002(flags, CFG_PORT_ISDVI_BIT)


struct mwv206_dev_config{
	struct mwv206_chip_config chip;
	struct mwv206_fb_config   fb;


	struct mwv206_port_config hdmi[4];
	struct mwv206_port_config vga[2];
	struct mwv206_port_config lvds[2];
	struct mwv206_port_config dvo[2];
};

#define port_for_each(V206DEV105, port) for ((port)  = &(V206DEV105)->hdmi[0]; \
					(port) <= &(V206DEV105)->dvo[1]; \
					(port)++)

#define port_is_hdmi(V206DEV105, port)  ((port) >= &(V206DEV105)->hdmi[0] \
					&& (port) <= &(V206DEV105)->hdmi[3])
#define port_is_vga(V206DEV105, port)  ((port) >= &(V206DEV105)->vga[0] \
					&& (port) <= &(V206DEV105)->vga[1])
#define port_is_lvds(V206DEV105, port)  ((port) >= &(V206DEV105)->lvds[0] \
					&& (port) <= &(V206DEV105)->lvds[1])
#define port_is_dvo(V206DEV105, port)  ((port) >= &(V206DEV105)->dvo[0] \
					&& (port) <= &(V206DEV105)->dvo[1])

struct mwv206_board_config{
	uint8_t bl_maxlevel;
	uint8_t bl_maxdutyratio;
	uint8_t bl_mindutyratio;
	uint8_t bl_pwmgpio;
	char usrinfo[200];
};

struct mwv206_dev_config_all{
	struct mwv206_dev_config V206DEV105;
	struct mwv206_board_config board;
};

#define MWV206DEV_CONFIG_SIZE   (sizeof(struct mwv206_dev_config))
#define MWV206DEV_CONFIG_MAXCFG (8)

#ifndef NULL
#define NULL (0)
#endif

typedef enum {
	V206DEVCONFIG045 = 0,
	V206DEVCONFIG046,
	V206DEVCONFIG047,
	V206DEVCONFIG048,
	V206DEVCONFIG049,
	V206DEVCONFIG050,
} e_MWV206KeyType;

typedef struct tagKEYVALUEITEM {
	const char *secName;
	const char *keyName;
	int valuetype;
	char *value;
} sKEYVALUEITEM;
#define STATE_NA            (0)
#define STATE_SECTION       (1)
#define STATE_KEY           (2)
#define STATE_VALUE         (3)


typedef int32_t (*FileReader_t)(int32_t offset, void *buf, uint32_t len);


int32_t FUNC206HAL108(int32_t offset, void *buf, uint32_t len);


int32_t FUNC206HAL113(int32_t offset, void *buf, uint32_t len);


int32_t FUNC206HAL111(int32_t offset, void *buf, uint32_t len);


int32_t jmVbiosCfgVerSpiFlashReader(int32_t offset, void *buf, uint32_t len);


int32_t jmCfgSpiFlashReadEx(uint32_t faddr, int32_t offset, void *buf, uint32_t len);


int32_t  FUNC206HAL112(void   *handle);


int FUNC206HAL110(void *dev, FileReader_t fileReader, int bdefaultReader);


int jmVbiosCfgVerGet(void *dev, FileReader_t fileReader);


struct mwv206_dev_config *FUNC206HAL109(void *dev);


uint32_t FUNC206HAL107(void *buf, uint32_t len);


static inline int mwv206_get_portid(struct mwv206_dev_config *V206DEV105, struct mwv206_port_config *port)
{
	if (port_is_vga(V206DEV105, port)) {
		return MWV206_DP_DAC_0 + (port - &V206DEV105->vga[0]);
	}

	if (port_is_hdmi(V206DEV105, port)) {
		return MWV206_DP_HDMI_0 + (port - &V206DEV105->hdmi[0]);
	}

	if (port_is_lvds(V206DEV105, port)) {
		return MWV206_DP_LVDS_0 + (port - &V206DEV105->lvds[0]);
	}

	if (port_is_dvo(V206DEV105, port)) {
		return MWV206_DP_DVO_0 + (port - &V206DEV105->dvo[0]);
	}

	return -1;
}

#ifdef __cplusplus
}
#endif

#endif