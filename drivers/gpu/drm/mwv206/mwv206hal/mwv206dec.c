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

int mwv206dec_init(void **decoder, unsigned long reg_base, void *dev)
{
	int ret;
	struct mwv206decoder *dec = NULL;

	if (!decoder || !dev || (reg_base == 0))
		return -1;

	dec = (struct mwv206decoder *)FUNC206LXDEV010(sizeof(struct mwv206decoder));
	if (!dec)
		return -1;

	memset(dec, 0, sizeof(struct mwv206decoder));
	*decoder = dec;

	dec->reg_base = reg_base;
	dec->cfg.hwreg = V206DECH023;
	dec->cfg.iosize = MWV206_DEC_IO_SIZE;
	dec->cfg.irq = 10;

	ret = mwv206dec_reserve_hw(&dec->cfg, dev);
	if (ret < 0)
		goto err_return;

	dec->dec_core_sem = FUNC206HAL088();
	if (!dec->dec_core_sem)
		goto err_return;

	dec->pp_core_sem = FUNC206HAL088();
	if (!dec->pp_core_sem)
		goto err_return;

	mwv206dec_read_config(dec, dev);
	mwv206dec_reset_irq(&dec->cfg, dev);

	dec->owner_lock = FUNC206HAL092();
	if (!dec->owner_lock)
		goto err_return;

	dec->dec_wait_queue = FUNC206HAL097();
	if (!dec->dec_wait_queue)
		goto err_return;

	dec->pp_wait_queue = FUNC206HAL097();
	if (!dec->pp_wait_queue)
		goto err_return;

	dec->hw_queue = FUNC206HAL097();
	if (!dec->hw_queue)
		goto err_return;

	dec->irq_rx = FUNC206HAL072();
	if (!dec->irq_rx)
		goto err_return;

	dec->irq_tx = FUNC206HAL072();
	if (!dec->irq_tx)
		goto err_return;

	V206KDEBUG003("decoder init ok.\n");
	return 0;

err_return:
	mwv206dec_free_resources(dec);
	*decoder = NULL;
	return -1;
}

void mwv206dec_reset(void *decoder, void *dev)
{
	struct mwv206decoder *dec = NULL;

	if (!decoder || !dev)
		return;

	dec = (struct mwv206decoder *)decoder;

	mwv206dec_reset_irq(&dec->cfg, dev);
	V206KDEBUG003("decoder reset ok.\n");
}

void mwv206dec_uninit(void *decoder, void *dev)
{
	if (!decoder || !dev) {
		return;
	}

	mwv206dec_reset(decoder, dev);
	mwv206dec_free_resources(decoder);
	V206KDEBUG003("decoder uninit ok.\n");
}

int mwv206dec_decoder_isr(void *decoder, void *dev)
{
	unsigned int irq;
	unsigned int handled = 0;
	unsigned long flags;
	struct mwv206decoder *dec = NULL;

	if (!decoder || !dev)
		return -1;

	dec = (struct mwv206decoder *)decoder;

	irq = decoder_irq_read(&dec->cfg, dev);
	if (irq & V206DECC031) {
		V206KDEBUG003("decoder irq received!\n");
		irq &= (~V206DECC031);
		decoder_irq_write(&dec->cfg, dev, irq);

		flags = FUNC206HAL094(dec->owner_lock);
		dec->dec_irq |= 1;
		FUNC206HAL095(dec->owner_lock, flags);

		FUNC206HAL075(dec->irq_rx);
		FUNC206HAL099(dec->dec_wait_queue);
		handled++;
	}

	irq = post_processor_irq_read(&dec->cfg, dev);
	if (irq & V206DECC032) {
		V206KDEBUG003("post-processor irq received!\n");
		irq &= (~V206DECC032);
		post_processor_irq_write(&dec->cfg, dev, irq);

		flags = FUNC206HAL094(dec->owner_lock);
		dec->pp_irq |= 1;
		FUNC206HAL095(dec->owner_lock, flags);

		FUNC206HAL075(dec->irq_rx);
		FUNC206HAL099(dec->pp_wait_queue);
		handled++;
	}

	if (!handled)
		V206KDEBUG002("irq received, but not mwv206's!\n");

	return IRQ_RETVAL(handled);
}

int mwv206dec_release(void *inode, void *filp, void *decoder, void *dev)
{
	struct mwv206decoder *dec = NULL;

	if (!filp || !decoder || !dev)
		return -EFAULT;

	dec = (struct mwv206decoder *)decoder;
	if (dec->dec_owner == filp)
		mwv206dec_release_decoder(dec, dev);
	if (dec->pp_owner == filp)
		mwv206dec_release_post_processor(dec, dev);

	return 0;
}

long mwv206dec_ioctl(void *filp, unsigned int cmd, unsigned long arg, void *decoder, void *dev)
{
	long ret;
	struct mwv206decoder *dec = NULL;

	if (!filp || !decoder || !dev)
		return -EFAULT;

	dec = (struct mwv206decoder *)decoder;

	switch (cmd) {
	case V206DECH005:
		FUNC206HAL079(dec->cfg.irq);
		break;
	case V206DECH006:
		FUNC206HAL080(dec->cfg.irq);
		break;
	case V206DECH003:
		FUNC206HAL087(dec->reg_base, (unsigned long *)arg);
		break;
	case V206DECH004:
		FUNC206HAL086(dec->cfg.iosize, (unsigned int *)arg);
		break;
	case V206DECH007: {
		FUNC206HAL087(dec->reg_base, (unsigned long *) arg);
		break;
	}
	case V206DECH008:
		FUNC206HAL086(1, (unsigned int *) arg);
		break;
	case V206DECH009: {
		struct core_desc core;
		if (FUNC206HAL077(&core, (void *)arg, sizeof(struct core_desc)))
			return -EFAULT;
		return mwv206dec_flush_dec_regs(dec, &core, dev);
	}
	case V206DECH010: {
		struct core_desc core;
		if (FUNC206HAL077(&core, (void *)arg, sizeof(struct core_desc)))
			return -EFAULT;
		return mwv206dec_flush_pp_regs(dec, &core, dev);
	}
	case V206DECH017: {
		struct core_desc core;
		if (FUNC206HAL077(&core, (void *)arg, sizeof(struct core_desc)))
			return -EFAULT;
		return mwv206dec_refresh_dec_regs(dec, &core, dev);
	}
	case V206DECH018: {
		struct core_desc core;
		if (FUNC206HAL077(&core, (void *)arg, sizeof(struct core_desc)))
			return -EFAULT;
		return mwv206dec_refresh_pp_regs(dec, &core, dev);
	}
	case V206DECH011:
		return mwv206dec_reserve_decoder(dec, filp, arg);
	case V206DECH012:
		if (dec->dec_owner != filp)
			return -EFAULT;
		mwv206dec_release_decoder(dec, dev);
		break;
	case V206DECH013:
		return mwv206dec_reserve_post_processor(dec, filp);
	case V206DECH014:
		if (dec->pp_owner != filp)
			return -EFAULT;
		mwv206dec_release_post_processor(dec, dev);
		break;
	case V206DECH015: {
		struct core_desc core;
		if (FUNC206HAL077(&core, (void *)arg, sizeof(struct core_desc)))
			return -EFAULT;
		return mwv206dec_dec_wait_ready_and_refresh_regs(dec, &core, dev);
	}
	case V206DECH016: {
		struct core_desc core;
		if (FUNC206HAL077(&core, (void *)arg, sizeof(struct core_desc)))
			return -EFAULT;
		return mwv206dec_pp_wait_ready_and_refresh_regs(dec, &core, dev);
	}
	case V206DECH019:
		ret = mwv206dec_wait_ready(dec, filp);
		FUNC206HAL085(0, (int *) arg);
		return ret;
	case V206DECH020: {
		unsigned int id = FUNC206HAL082((unsigned int *)arg);
		if (id >= 1)
			return -EFAULT;
		id = V206DEV007(dev, dec->cfg.hwreg);
		FUNC206HAL086(id, (unsigned int *)arg);
		break;
	}
	case V206DECH021:
		V206KDEBUG002("decoder: dec->dec_irq     = 0x%08x \n", dec->dec_irq);
		V206KDEBUG002("decoder: dec->pp_irq      = 0x%08x \n", dec->pp_irq);
		V206KDEBUG002("decoder: irqs recv/send   = %d / %d \n",
				FUNC206HAL076(dec->irq_rx), FUNC206HAL076(dec->irq_tx));
		V206KDEBUG002("decoder: dec_core %s\n", dec->dec_owner == NULL ? "FREE" : "RESERVED");
		V206KDEBUG002("decoder: pp_core %s\n", dec->pp_owner == NULL ? "FREE" : "RESERVED");
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}