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
#include <linux/irq.h>
#include "mwv206dec.h"
#include "mwv206dec_priv.h"
#include "gljos.h"
#include "gljos_kernel.h"

void mwv206dec_read_config(struct mwv206decoder *dec, void *dev)
{
	unsigned int cfg;
	unsigned int state;

	cfg = decoder_cfg_read(&dec->cfg, dev);
	dec->supports |= ((cfg >> 24) & 0x03U) ? (0x1 << 1) : 0;
	dec->supports |= ((cfg >> 26) & 0x03U) ? (0x1 << 2) : 0;
	dec->supports |= ((cfg >> 28) & 0x01U) ? (0x1 << 3) : 0;
	dec->supports |= ((cfg >> 29) & 0x03U) ? (0x1 << 5) : 0;
	dec->supports |= ((cfg >> 31) & 0x01U) ? (0x1 << 6) : 0;
	dec->supports |= ((cfg >> 24) & 0x01U) ? (0x1 << 7) : 0;

	cfg = decoder_cfg2_read(&dec->cfg, dev);
	dec->supports |= ((cfg >> 22) & 0x01U) ? (0x1 << 8) : 0;
	dec->supports |= ((cfg >> 26) & 0x03U) ? (0x1 << 9) : 0;
	state =	((cfg >> 23) | (cfg >> 19) | (cfg >> 24)) & 0x1U;
	dec->supports |= state ? (0x1 << 10) : 0;

	cfg = post_processor_cfg_read(&dec->cfg, dev);
	dec->supports |= ((cfg >> 16) & 0x01) ? (0x1 << 4) : 0;
}

#define IS_FORMAT_SUPPORTED(cfg, format) (((cfg) & (1 << (format))) ? 1 : 0)

static int mwv206dec_get_decoder(struct mwv206decoder *dec, void *filp, unsigned long format)
{
	unsigned long flags;

	if (!IS_FORMAT_SUPPORTED(dec->supports, format)) {
		return 0;
	}

	if (dec->dec_owner != NULL) {
		return 0;
	}

	flags = FUNC206HAL094(dec->owner_lock);
	dec->dec_owner = filp;
	FUNC206HAL095(dec->owner_lock, flags);

	return 1;
}

long mwv206dec_reserve_decoder(struct mwv206decoder *dec, void *filp, unsigned long format)
{
	if (FUNC206HAL090(dec->dec_core_sem))
		return -ERESTARTSYS;

	if (FUNC206HAL096(dec->hw_queue,
		mwv206dec_get_decoder(dec, filp, format) != 0)) {
		FUNC206HAL091(dec->dec_core_sem);
		return -ERESTARTSYS;
	}

	return 0;
}

static void mwv206dec_wait_dec_finish(struct mwv206dec_config *cfg, void *dev)
{
	unsigned int irq;
	unsigned int count = 0;

	irq = decoder_irq_read(cfg, dev);
	while (irq & V206DECC026) {
		irq = decoder_irq_read(cfg, dev);
		if (++count > 500000) {
			V206KDEBUG002("decoder kill timeout! (0x%x)\n", irq);
			break;
		}
	}
}

void mwv206dec_release_decoder(struct mwv206decoder *dec, void *dev)
{
	unsigned int irq;
	unsigned long flags;

	irq = decoder_irq_read(&dec->cfg, dev);
	if (irq & V206DECC026) {
		V206KDEBUG003("decoder still alived -> killing ...\n");
		mwv206dec_wait_dec_finish(&dec->cfg, dev);
		decoder_irq_write(&dec->cfg, dev, 0);
	}

	flags = FUNC206HAL094(dec->owner_lock);
	dec->dec_owner = NULL;
	FUNC206HAL095(dec->owner_lock, flags);

	FUNC206HAL091(dec->dec_core_sem);
	FUNC206HAL099(dec->hw_queue);
}

static void mwv206dec_wait_pp_finish(struct mwv206dec_config *cfg, void *dev)
{
	unsigned int irq;
	unsigned int count = 0;

	irq = post_processor_irq_read(cfg, dev);
	while (irq & V206DECC027) {
		irq = post_processor_irq_read(cfg, dev);
		if (++count > 500000) {
			V206KDEBUG002("post-processor kill timeout! (0x%x)\n", irq);
			break;
		}
	}
}

void mwv206dec_release_post_processor(struct mwv206decoder *dec, void *dev)
{
	unsigned int irq;
	unsigned long flags;

	irq = post_processor_irq_read(&dec->cfg, dev);
	if (irq & V206DECC027) {
		V206KDEBUG003("post-processor still alived -> killing ...\n");
		mwv206dec_wait_pp_finish(&dec->cfg, dev);
		post_processor_irq_write(&dec->cfg, dev, 0x10);
	}

	flags = FUNC206HAL094(dec->owner_lock);
	dec->pp_owner = NULL;
	FUNC206HAL095(dec->owner_lock, flags);

	FUNC206HAL091(dec->pp_core_sem);
}

long mwv206dec_reserve_post_processor(struct mwv206decoder *dec, void *filp)
{
	unsigned long flags;

	if (FUNC206HAL090(dec->pp_core_sem))
		return -ERESTARTSYS;

	flags = FUNC206HAL094(dec->owner_lock);
	dec->pp_owner = filp;
	FUNC206HAL095(dec->owner_lock, flags);

	return 0;
}

long mwv206dec_flush_dec_regs(struct mwv206decoder *dec, struct core_desc *core, void *dev)
{
	long ret;
	unsigned long i;

	ret = FUNC206HAL077(dec->regs, core->regs, V206DECC005 * 4);
	if (ret) {
		V206KDEBUG002("gljosCopyFromUser failed, returned %li\n", ret);
		return -EFAULT;
	}

	for (i = 2; i <= V206DECC013; i++)
		V206DEV006(dev, dec->cfg.hwreg + i * 4, dec->regs[i]);

	V206DEV006(dev, dec->cfg.hwreg + 4, dec->regs[1]);

	return 0;
}

long mwv206dec_refresh_dec_regs(struct mwv206decoder *dec, struct core_desc *core, void *dev)
{
	long ret;
	unsigned int i;

	if (core->size != (V206DECC005 * 4))
		return -EFAULT;

	for (i = 0; i <= V206DECC013; i++)
		dec->regs[i] = V206DEV007(dev, dec->cfg.hwreg + i * 4);

	ret = FUNC206HAL078(core->regs, dec->regs, V206DECC005 * 4);
	if (ret) {
		V206KDEBUG002("gljosCopyToUser failed!\n");
		return -EFAULT;
	}

	return 0;
}

static int mwv206dec_check_dec_irq(struct mwv206decoder *dec)
{
	int rdy = 0;
	unsigned int irq_mask = 0x1;
	unsigned long flags;

	if (dec->dec_irq & irq_mask) {
		flags = FUNC206HAL094(dec->owner_lock);
		dec->dec_irq &= ~irq_mask;
		FUNC206HAL095(dec->owner_lock, flags);
		rdy = 1;
	}

	return rdy;
}

long mwv206dec_dec_wait_ready_and_refresh_regs(struct mwv206decoder *dec, struct core_desc *core, void *dev)
{
	if (FUNC206HAL096(dec->dec_wait_queue, mwv206dec_check_dec_irq(dec))) {
		V206KDEBUG002("decoder wait_event_interruptible interrupted\n");
		return -ERESTARTSYS;
	}

	FUNC206HAL075(dec->irq_tx);

	return mwv206dec_refresh_dec_regs(dec, core, dev);
}

long mwv206dec_flush_pp_regs(struct mwv206decoder *dec, struct core_desc *core, void *dev)
{
	long ret;
	unsigned int i;

	ret = FUNC206HAL077(dec->regs + V206DECC016,
			core->regs + V206DECC016,
			V206DECC006 * 4);
	if (ret) {
		V206KDEBUG002("decoder gljosCopyFromUser failed!\n");
		return -EFAULT;
	}

	mwv206dec_wait_pp_finish(&dec->cfg, dev);

	for (i = V206DECC016 + 1; i <= V206DECC017; i++)
		V206DEV006(dev, dec->cfg.hwreg + i * 4, dec->regs[i]);

	V206DEV006(dev, dec->cfg.hwreg + V206DECC016 * 4,
			dec->regs[V206DECC016]);

	return 0;
}

long mwv206dec_refresh_pp_regs(struct mwv206decoder *dec, struct core_desc *core, void *dev)
{
	long ret;
	unsigned int i;

	if (core->size != (V206DECC006 * 4))
		return -EFAULT;

	for (i = V206DECC016; i <= V206DECC017; i++)
		dec->regs[i] = V206DEV007(dev, dec->cfg.hwreg + i * 4);

	ret = FUNC206HAL078(core->regs + V206DECC016,
			dec->regs + V206DECC016,
			V206DECC006 * 4);
	if (ret) {
		V206KDEBUG002("decoder gljosCopyToUser failed!\n");
		return -EFAULT;
	}

	return 0;
}

static int mwv206dec_check_pp_irq(struct mwv206decoder *dec)
{
	int rdy = 0;
	unsigned long flags;
	unsigned int irq_mask = 0x01;

	flags = FUNC206HAL094(dec->owner_lock);
	if (dec->pp_irq & irq_mask) {
		dec->pp_irq &= ~irq_mask;
		rdy = 1;
	}
	FUNC206HAL095(dec->owner_lock, flags);

	return rdy;
}

long mwv206dec_pp_wait_ready_and_refresh_regs(struct mwv206decoder *dec, struct core_desc *core, void *dev)
{
	if (FUNC206HAL096(dec->pp_wait_queue, mwv206dec_check_pp_irq(dec))) {
		V206KDEBUG002("decoder wait_event_interruptible interrupted\n");
		return -ERESTARTSYS;
	}

	FUNC206HAL075(dec->irq_tx);

	return mwv206dec_refresh_pp_regs(dec, core, dev);
}

static int mwv206dec_check_irq(struct mwv206decoder *dec, const void *filp)
{
	int rdy = 0;
	unsigned long flags;
	unsigned int irq_mask = 0x1;

	flags = FUNC206HAL094(dec->owner_lock);
	if (dec->dec_irq & irq_mask) {
		if (dec->dec_owner == filp)
			rdy = 1;
		dec->dec_irq &= ~irq_mask;
	}
	FUNC206HAL095(dec->owner_lock, flags);
	return rdy;
}

long mwv206dec_wait_ready(struct mwv206decoder *dec, const void *filp)
{
	if (FUNC206HAL096(dec->dec_wait_queue, mwv206dec_check_irq(dec, filp)))
		return -ERESTARTSYS;

	FUNC206HAL075(dec->irq_tx);

	return 0;
}

void mwv206dec_free_resources(struct mwv206decoder *dec)
{
	if (dec->irq_tx) {
		FUNC206HAL074(dec->irq_tx);
		dec->irq_tx = 0;
	}

	if (dec->irq_rx) {
		FUNC206HAL074(dec->irq_rx);
		dec->irq_rx = 0;
	}

	if (dec->hw_queue) {
		FUNC206HAL098(dec->hw_queue);
		dec->hw_queue = 0;
	}

	if (dec->pp_wait_queue) {
		FUNC206HAL098(dec->pp_wait_queue);
		dec->pp_wait_queue = 0;
	}

	if (dec->dec_wait_queue) {
		FUNC206HAL098(dec->dec_wait_queue);
		dec->dec_wait_queue = 0;
	}

	if (dec->owner_lock) {
		FUNC206HAL093(dec->owner_lock);
		dec->owner_lock = 0;
	}

	if (dec->pp_core_sem) {
		FUNC206HAL089(dec->pp_core_sem);
		dec->pp_core_sem = 0;
	}

	if (dec->dec_core_sem) {
		FUNC206HAL089(dec->dec_core_sem);
		dec->dec_core_sem = 0;
	}

	(void)FUNC206LXDEV120(dec);
	dec = NULL;
}

int mwv206dec_reserve_hw(struct mwv206dec_config *cfg, void *dev)
{
	int i;
	unsigned int id;
	unsigned int id_num;
	unsigned int decoder_ids[] = {
		0x8190, 0x8170,	0x9170,	0x9190,	0x6731
	};

	id = V206DEV007(dev, cfg->hwreg);
	id = (id >> 16) & 0xFFFF;

	id_num = sizeof(decoder_ids) / sizeof(decoder_ids[0]);

	for (i = 0; i < id_num; i++) {
		if (id == decoder_ids[i]) {
			return 0;
		}
	}

	V206KDEBUG003("decoder hw isn't supported!\n");
	return -EBUSY;
}

void mwv206dec_reset_irq(struct mwv206dec_config *cfg, void *dev)
{
	unsigned long i;
	unsigned int irq;

	irq = decoder_irq_read(cfg, dev);
	if (irq & V206DECC026) {
		irq = V206DECC028 | V206DECC029;
		decoder_irq_write(cfg, dev, irq);
	}
	post_processor_irq_write(cfg, dev, 0);

	for (i = 4; i < cfg->iosize; i += 4)
		V206DEV006(dev, cfg->hwreg + i, 0);
}