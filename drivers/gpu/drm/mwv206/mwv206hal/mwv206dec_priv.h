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
#ifndef _MWV206_MWV206DEC_PRIV_H_
#define _MWV206_MWV206DEC_PRIV_H_

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/fs.h>
#include "gljos.h"
#include "gljos_kernel.h"

#include "mwv206kdebug.h"

#define V206DECC005             60
#define V206DECC006              41

#define V206DECC007             27
#define V206DECC008              9

#define V206DECC009           (V206DECC005 + V206DECC007)
#define V206DECC010            (V206DECC006 + V206DECC008)
#define V206DECC011               155

#define V206DECC012        0
#define V206DECC013         59
#define V206DECC014        119
#define V206DECC015         145

#define V206DECC016         60
#define V206DECC017          100
#define V206DECC018         146
#define V206DECC019          154

#define MWV206_PP_SYNTH_CFG             100
#define MWV206_PP_SYNTH_CFG_OFF         (MWV206_PP_SYNTH_CFG * 4)
#define MWV206_DEC_SYNTH_CFG            50
#define MWV206_DEC_SYNTH_CFG_OFF        (MWV206_DEC_SYNTH_CFG * 4)
#define MWV206_DEC_SYNTH_CFG_2          54
#define MWV206_DEC_SYNTH_CFG_2_OFF      (MWV206_DEC_SYNTH_CFG_2 * 4)

#define V206DECC026                    0x01
#define V206DECC027                     0x01
#define V206DECC028                0x20
#define V206DECC029          0x10
#define V206DECC030           0x10
#define V206DECC031                  0x100
#define V206DECC032                   0x100

#define V206DECC035             1
#define V206DECC036         (V206DECC035 * 4)
#define V206DECC037              60
#define V206DECC038          (V206DECC037 * 4)

#define MWV206_DEC_IO_SIZE              ((V206DECC005 + V206DECC006) * 4)

struct mwv206dec_config {
	char *buffer;
	unsigned int iosize;
	unsigned long hwreg;
	int irq;
};

struct mwv206decoder {
	unsigned long reg_base;
	unsigned int regs[MWV206_DEC_IO_SIZE / 4];

	GLJOS_SPINLOCK owner_lock;
	GLJOS_WAIT_QUEUE_HEAD dec_wait_queue;
	GLJOS_WAIT_QUEUE_HEAD pp_wait_queue;
	GLJOS_WAIT_QUEUE_HEAD hw_queue;

	struct mwv206dec_config cfg;
	void *dec_owner;
	void *pp_owner;
	GLJOS_SEMAPHORE dec_core_sem;
	GLJOS_SEMAPHORE pp_core_sem;

	int dec_irq;
	int pp_irq;

	unsigned int supports;

	GLJOS_ATOMIC irq_rx;
	GLJOS_ATOMIC irq_tx;
};

extern void V206DEV006(void *devInfo, unsigned int regAddr, unsigned int value);
extern unsigned int V206DEV007(void *devInfo, unsigned int regAddr);

static inline unsigned int decoder_irq_read(struct mwv206dec_config *cfg, void *dev)
{
	return V206DEV007(dev, cfg->hwreg + V206DECC036);
}

static inline void decoder_irq_write(struct mwv206dec_config *cfg, void *dev, unsigned int value)
{
	V206DEV006(dev, cfg->hwreg +  V206DECC036, value);
}

static inline unsigned int decoder_cfg_read(struct mwv206dec_config *cfg, void *dev)
{
	return V206DEV007(dev, cfg->hwreg + MWV206_DEC_SYNTH_CFG * 4);
}

static inline unsigned int decoder_cfg2_read(struct mwv206dec_config *cfg, void *dev)
{
	return V206DEV007(dev, cfg->hwreg + MWV206_DEC_SYNTH_CFG_2 * 4);
}

static inline unsigned int post_processor_cfg_read(struct mwv206dec_config *cfg, void *dev)
{
	return V206DEV007(dev, cfg->hwreg + MWV206_PP_SYNTH_CFG * 4);
}

static inline unsigned int post_processor_irq_read(struct mwv206dec_config *cfg, void *dev)
{
	return V206DEV007(dev, cfg->hwreg + V206DECC038);
}

static inline void post_processor_irq_write(struct mwv206dec_config *cfg, void *dev, unsigned int value)
{
	V206DEV006(dev, cfg->hwreg + V206DECC038, value);
}

void read_core_config(struct mwv206decoder *dec, void *dev);
long mwv206dec_flush_dec_regs(struct mwv206decoder *dec, struct core_desc *core, void *dev);
long mwv206dec_refresh_dec_regs(struct mwv206decoder *dec, struct core_desc *core, void *dev);
long mwv206dec_flush_pp_regs(struct mwv206decoder *dec, struct core_desc *core, void *dev);
void mwv206dec_reset_hw(struct mwv206dec_config *cfg, void *dev);
void mwv206dec_free_resources(struct mwv206decoder *dec);
long mwv206dec_reserve_decoder(struct mwv206decoder *dec, void *filp, unsigned long format);
void mwv206dec_release_decoder(struct mwv206decoder *dec, void *dev);
long mwv206dec_reserve_post_processor(struct mwv206decoder *dec, void *filp);
void mwv206dec_release_post_processor(struct mwv206decoder *dec, void *dev);
long mwv206dec_dec_wait_ready_and_refresh_regs(struct mwv206decoder *dec, struct core_desc *core, void *dev);
long mwv206dec_pp_wait_ready_and_refresh_regs(struct mwv206decoder *dec, struct core_desc *core, void *dev);
long mwv206dec_wait_ready(struct mwv206decoder *dec, const void *filp);
int mwv206dec_reserve_hw(struct mwv206dec_config *cfg, void *dev);
void mwv206dec_read_config(struct mwv206decoder *dec, void *dev);
void mwv206dec_reset_irq(struct mwv206dec_config *cfg, void *dev);
long mwv206dec_refresh_pp_regs(struct mwv206decoder *dec, struct core_desc *core, void *dev);

#endif