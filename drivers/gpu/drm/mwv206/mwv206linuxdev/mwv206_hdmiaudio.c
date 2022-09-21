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
#include <linux/kthread.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <sound/core.h>

#include <sound/pcm.h>
#include <sound/info.h>
#include <sound/control.h>
#include <sound/initval.h>
#include <sound/asoundef.h>
#include <sound/pcm_iec958.h>
#include <asm/irq.h>
#include <linux/version.h>
#include "mwv206hal/mwv206dev.h"
#include "mwv206reg.h"
#include "sethdmi_const_new.h"
#include "mwv206_hdmiaudio.h"
#include "mwv206dev.h"

#if defined(SUPPORT_SND_PCM)

static ktime_t timer_period;

static int disable_audio;
module_param(disable_audio, int, 0644);
MODULE_PARM_DESC(disable_audio, "Disable hdmi audio, enable immediately, disable after hdmi re-hotplug");


struct dw_hdmi_channel_conf {
	u8 conf1;
	u8 ca;
};

static struct dw_hdmi_channel_conf default_hdmi_channel_config[7] = {
	{ 0x03, 0x00 },
	{ 0x0b, 0x02 },
	{ 0x33, 0x08 },
	{ 0x37, 0x09 },
	{ 0x3f, 0x0b },
	{ 0x7f, 0x0f },
	{ 0xff, 0x13 },
};



static void mwv206_audio_sample_convert(int rate, int *pi_sf, int *pi_oiecsf, int *pi_oiecosf)
{
	int sf = 0, osf = 0, oosf = 0;

	switch (rate) {
	case 32000:
		sf = 0x01;
		osf = 0x03;
		oosf = 0xC0;
		break;
	case 44100:
		sf = 0x02;
		osf = 0x00;
		oosf = 0xF0;
		break;
	case 48000:
		sf = 0x03;
		osf = 0x02;
		oosf = 0xD0;
		break;
	case 88200:
		sf = 0x04;
		osf = 0x08;
		oosf = 0x70;
		break;
	case 96000:
		sf = 0x05;
		osf = 0x0a;
		oosf = 0x50;
		break;
	default:
		sf = 0x00;
		osf = 0x00;
		oosf = 0x00;
		break;
	}

	if (pi_sf) {
		*pi_sf = sf;
	}

	if (pi_oiecsf) {
		*pi_oiecsf = osf;
	}

	if (pi_oiecosf) {
		*pi_oiecosf = oosf;
	}
}

int FUNC206LXDEV140(V206DEV025 *pDev, int arg)
{
	if (arg) {
		return FUNC206LXDEV116(pDev->V206DEV142);
	} else {
		return FUNC206LXDEV136(pDev->V206DEV142);
	}
}

static void FUNC206LXDEV017(V206DEV025 *pDev, int V206HDMIAUDIO027)
{
	if (V206HDMIAUDIO027 < 0 || V206HDMIAUDIO027 > 2) {
		return;
	}

	V206DEV002(fc_acp0(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp1(V206HDMIAUDIO027), 0x01);
	V206DEV002(fc_acp2(V206HDMIAUDIO027), 0x9e);
	V206DEV002(fc_acp3(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp4(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp5(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp6(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp7(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp8(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp9(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp10(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp11(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp12(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp13(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp14(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp15(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_acp16(V206HDMIAUDIO027), 0x00);
}

static void FUNC206LXDEV020(V206DEV025 *pDev, int V206HDMIAUDIO027)
{
	int sf = 0, osf = 0, oosf = 0;
	struct V206DEV139 *mwv = NULL;

	if (V206HDMIAUDIO027 < 0 || V206HDMIAUDIO027 > 2) {
		return;
	}

	mwv = pDev->audio[V206HDMIAUDIO027];

	V206DEV002(fc_audsconf(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_audsv(V206HDMIAUDIO027), 0x33);
	V206DEV002(fc_audsu(V206HDMIAUDIO027), 0x33);

	V206DEV002(aud_int_ctl(V206HDMIAUDIO027), 0x7);

	V206DEV002(fc_audschnls0(V206HDMIAUDIO027), 0x20);
	V206DEV002(fc_audschnls1(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_audschnls2(V206HDMIAUDIO027), 0x12);
	V206DEV002(fc_audschnls3(V206HDMIAUDIO027), 0x01);
	V206DEV002(fc_audschnls4(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_audschnls5(V206HDMIAUDIO027), 0x01);
	V206DEV002(fc_audschnls6(V206HDMIAUDIO027), 0x00);

	mdelay(1);

	if (mwv && mwv->rate) {
		mwv206_audio_sample_convert(mwv->rate, &sf, &osf, &oosf);

		V206DEV002(fc_audiconf1(V206HDMIAUDIO027), sf);

		V206DEV002(fc_audschnls7(V206HDMIAUDIO027), osf & 0xff);
		V206DEV002(fc_audschnls8(V206HDMIAUDIO027), oosf & 0xff);
	} else {

		V206DEV002(fc_audiconf1(V206HDMIAUDIO027), 0x0);
		V206DEV002(fc_audschnls7(V206HDMIAUDIO027), 0xc0);
		V206DEV002(fc_audschnls8(V206HDMIAUDIO027), 0xf5);
	}
}

static void FUNC206LXDEV018(V206DEV025 *pDev, int V206HDMIAUDIO027)
{
	if (V206HDMIAUDIO027 < 0 || V206HDMIAUDIO027 > 2) {
		return;
	}

	V206DEV002(fc_rdrb0(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_rdrb1(V206HDMIAUDIO027), 0x10);
	V206DEV002(fc_rdrb2(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_rdrb3(V206HDMIAUDIO027), 0x1b);
	V206DEV002(fc_rdrb4(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_rdrb5(V206HDMIAUDIO027), 0x10);
	V206DEV002(fc_rdrb6(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_rdrb7(V206HDMIAUDIO027), 0x1e);
}

static void FUNC206LXDEV019(V206DEV025 *pDev, int V206HDMIAUDIO027)
{
	if (V206HDMIAUDIO027 < 0 || V206HDMIAUDIO027 > 2) {
		return;
	}


	V206DEV002(fc_audiconf0(V206HDMIAUDIO027), 0x11);
	V206DEV002(fc_audiconf2(V206HDMIAUDIO027), 0x0);
	V206DEV002(fc_audiconf3(V206HDMIAUDIO027), 0x0);
}

static void FUNC206LXDEV030(V206DEV025 *pDev, int V206HDMIAUDIO027, unsigned int N, unsigned int C)
{
	if (V206HDMIAUDIO027 < 0 || V206HDMIAUDIO027 > 2) {
		return;
	}

	V206DEV002(aud_n3(V206HDMIAUDIO027), (N >> 16) | (0x1 << 7));

	V206DEV002(aud_cts3(V206HDMIAUDIO027), (C >> 16) | (0x1 << 4));
	V206DEV002(aud_cts2(V206HDMIAUDIO027), C >> 8);
	V206DEV002(aud_cts1(V206HDMIAUDIO027), C);

	V206DEV002(aud_n3(V206HDMIAUDIO027), (N >> 16) | (0x1 << 7));
	V206DEV002(aud_n2(V206HDMIAUDIO027), N >> 8);
	V206DEV002(aud_n1(V206HDMIAUDIO027), N);
}

int FUNC206LXDEV138(V206DEV025 *pDev, int V206HDMIAUDIO027)
{
	if (V206HDMIAUDIO027 < 0 || V206HDMIAUDIO027 > 2) {
		return -1;
	}

	if (pDev->audio[V206HDMIAUDIO027] == NULL) {
		return -1;
	}

	V206DEV002(aud_gp_conf0(V206HDMIAUDIO027), 0x01);
	mdelay(1);

	V206DEV002(aud_gp_conf1(V206HDMIAUDIO027), 0x03);
	V206DEV002(aud_gp_conf2(V206HDMIAUDIO027), 0x02);
	V206DEV002(fc_audsconf(V206HDMIAUDIO027), 0x00);
	V206DEV002(fc_audsv(V206HDMIAUDIO027), 0x33);
	V206DEV002(fc_audsu(V206HDMIAUDIO027), 0x33);


	FUNC206LXDEV020(pDev, V206HDMIAUDIO027);


	FUNC206LXDEV021(pDev, V206HDMIAUDIO027, 0);

	FUNC206LXDEV019(pDev, V206HDMIAUDIO027);
	FUNC206LXDEV018(pDev, V206HDMIAUDIO027);
	FUNC206LXDEV017(pDev, V206HDMIAUDIO027);

	return 0;
}

static unsigned int mwv206_audio_compute_n(unsigned int freq, unsigned long pixel_clk)
{
	unsigned int n = (128 * freq) / 1000;
	unsigned int mult = 1;

	while (freq > 48000) {
		mult *= 2;
		freq /= 2;
	}

	switch (freq) {
	case 32000:
		if (pixel_clk == 25175000) {
			n = 4576;
		} else if (pixel_clk == 27027000) {
			n = 4096;
		} else if (pixel_clk == 74176000 || pixel_clk == 148352000) {
			n = 11648;
		} else {
			n = 4096;
		}
		n *= mult;
		break;
	case 44100:
		if (pixel_clk == 25175000) {
			n = 7007;
		} else if (pixel_clk == 74176000) {
			n = 17836;
		} else if (pixel_clk == 148352000) {
			n = 8918;
		} else {
			n = 6272;
		}
		n *= mult;
		break;
	case 48000:
		if (pixel_clk == 25175000) {
			n = 6864;
		} else if (pixel_clk == 27027000) {
			n = 6144;
		} else if (pixel_clk == 74176000) {
			n = 11648;
		} else if (pixel_clk == 148352000) {
			n = 5824;
		} else {
			n = 6144;
		}
		n *= mult;
		break;
	default:
		break;
	}

	return n;
}

void FUNC206LXDEV021(V206DEV025 *pDev, int V206HDMIAUDIO027, unsigned int rate)
{
	static int srate[4] = { 44100, 44100, 44100, 44100 };
	u64 tmp = 0;
	unsigned long pixelclock = 0;
	unsigned long ftdms = 0;
	unsigned int cts = 0, n = 0;
	int sf = 0, osf = 0, oosf = 0;

	if (V206HDMIAUDIO027 < 0 || V206HDMIAUDIO027 > 2) {
		return;
	}

	if (!pDev->V206DEV141[V206HDMIAUDIO027]) {
		return;
	}

	if (rate == 0) {
		rate = srate[V206HDMIAUDIO027];
	} else {
		srate[V206HDMIAUDIO027] = rate;
	}

	mwv206_audio_sample_convert(rate, &sf, &osf, &oosf);
	V206DEV002(aud_int_ctl(V206HDMIAUDIO027), 0x7);
	mdelay(1);

	pixelclock = pDev->pixelclock[V206HDMIAUDIO027];

	ftdms = pixelclock;
	n = mwv206_audio_compute_n(rate, pixelclock);

	tmp = (u64)ftdms * n;
	do_div(tmp, 128 * rate);
	cts = tmp;

	FUNC206LXDEV030(pDev, V206HDMIAUDIO027, n, cts);

	mdelay(1);

	if (rate != 0) {
		FUNC206LXDEV020(pDev, V206HDMIAUDIO027);
	}
}

static inline unsigned int mwv206_audio_sample16_to_s16(unsigned int n)
{
	return (n << 8);
}

static void FUNC206LXDEV025(struct V206DEV139 *mwv)
{

	unsigned int period = 0;
	unsigned int  start = 0;
	unsigned int hard_fifo_len = 0, hard_fifo_avail = 0;
	unsigned int soft_fifo_len = 0;
	unsigned int bytes_sec = 0, bytes_hrtime = 0;
	int cnt = 0;
	int V206HDMIAUDIO027 =  mwv->V206HDMIAUDIO027;
	V206DEV025 *pDev = mwv->pdata;
	ktime_t new_hrtime;
	long hrtime_diff_usec;

	hard_fifo_len = V206DEV001(0x403434 + (V206HDMIAUDIO027 << 8));
	hard_fifo_len = (hard_fifo_len >> 16);
	hard_fifo_avail = (1000 - hard_fifo_len);
	hard_fifo_avail = (hard_fifo_avail << 1);


	bytes_sec = (mwv->V206HDMIAUDIO003->runtime->rate * 4);


	new_hrtime = ktime_get();

	hrtime_diff_usec = ktime_to_us(ktime_sub(new_hrtime, mwv->hrtime));
	if (hrtime_diff_usec > 10 * 1000 * 1000) {
		hrtime_diff_usec = 10 * 1000 * 1000;
	} else if (hrtime_diff_usec < 0) {
		hrtime_diff_usec = 0;
	}

	bytes_hrtime = (bytes_sec * hrtime_diff_usec) / 1000000;
	bytes_hrtime += 16;


	period = hard_fifo_avail > bytes_hrtime ? bytes_hrtime : hard_fifo_avail;
	if (hard_fifo_len < 100) {

		period = hard_fifo_avail;
	}
	soft_fifo_len = kfifo_len(&mwv->fifo);

	if (hard_fifo_avail < 16) {
		V206DEV005("!!! [%s] : hfifo len=%d, hfifo avail=%d, bytesec=%d, sfifo len = %d, period = %d\n",
				__FUNCTION__, hard_fifo_len, hard_fifo_avail, bytes_sec, soft_fifo_len, period);


		mwv->hrtime = new_hrtime;
		mwv->V206HDMIAUDIO006 += (mwv->V206HDMIAUDIO003->runtime->rate*4/HZ);
		return;
	}

	while (soft_fifo_len >= 4 && period >= 4) {
		cnt = kfifo_out(&mwv->fifo, &start, 2);
		start = mwv206_audio_sample16_to_s16(start);
		V206DEV002(Audio_FIFOAddr(V206HDMIAUDIO027), start);

		cnt = kfifo_out(&mwv->fifo, &start, 2);
		start = mwv206_audio_sample16_to_s16(start);
		V206DEV002(Audio_FIFOAddr(V206HDMIAUDIO027), start);

		soft_fifo_len -= 4;
		period -= 4;
		mwv->V206HDMIAUDIO006 += 4;

	}

	mwv->hrtime = new_hrtime;
}

static enum hrtimer_restart mwv206_audio_timer_task(struct hrtimer *hrtimer)
{
	unsigned long flags;
	struct V206DEV139 *mwv = container_of(hrtimer, struct V206DEV139, timer);

	spin_lock_irqsave(&mwv->lock, flags);
	if (mwv->V206HDMIAUDIO003 != NULL) {
		struct snd_pcm_substream *V206HDMIAUDIO003;
		FUNC206LXDEV025(mwv);
		V206HDMIAUDIO003 = mwv->V206HDMIAUDIO003;
		spin_unlock_irqrestore(&mwv->lock, flags);


		snd_pcm_period_elapsed(V206HDMIAUDIO003);
	} else {
		spin_unlock_irqrestore(&mwv->lock, flags);
	}

	hrtimer_forward_now(hrtimer, timer_period);
	return HRTIMER_RESTART;
}

static int FUNC206LXDEV036(struct snd_pcm_substream *substream,
	int cmd)
{
	struct V206DEV139 *mwv = snd_pcm_substream_chip(substream);
	V206DEV025 *pDev = mwv->pdata;
	unsigned long flags;
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:

		spin_lock_irqsave(&mwv->lock, flags);
		mwv->V206HDMIAUDIO003 = substream;
		mwv->V206HDMIAUDIO006 = 0;
		substream->runtime->delay = substream->runtime->period_size;
		spin_unlock_irqrestore(&mwv->lock, flags);
		break;

	case SNDRV_PCM_TRIGGER_STOP:

		spin_lock_irqsave(&mwv->lock, flags);
		mwv->V206HDMIAUDIO003 = NULL;
		kfifo_reset(&mwv->fifo);
		V206DEV002(aud_int_ctl(mwv->V206HDMIAUDIO027), 0x7);
		spin_unlock_irqrestore(&mwv->lock, flags);
		break;

	case SNDRV_PCM_TRIGGER_SUSPEND:
		break;

	case SNDRV_PCM_TRIGGER_RESUME:
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int FUNC206LXDEV035(struct snd_pcm_substream *substream)
{
	struct V206DEV139 *mwv = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned long flags;
	V206DEV025 *pDev = mwv->pdata;
	u8 layout, ca;
	int V206HDMIAUDIO027 = 0;

	spin_lock_irqsave(&mwv->lock, flags);
	kfifo_reset(&mwv->fifo);
	if (runtime->rate > 0) {
		mwv->rate = runtime->rate;
		FUNC206LXDEV021(pDev, mwv->V206HDMIAUDIO027, runtime->rate);
	} else {
		FUNC206LXDEV021(pDev, mwv->V206HDMIAUDIO027, 0);
	}

	runtime->hw.fifo_size = 2 * 1024;


	ca = default_hdmi_channel_config[runtime->channels - 2].ca;
	if (runtime->channels > 2) {
		layout = 1 << 0;
	} else {
		layout = 0 << 0;
	}

	V206HDMIAUDIO027 = mwv->V206HDMIAUDIO027;
	V206DEV002(fc_audsconf(V206HDMIAUDIO027), layout);
	V206DEV002(fc_audiconf2(V206HDMIAUDIO027), ca);

	spin_unlock_irqrestore(&mwv->lock, flags);

	return 0;
}

static snd_pcm_uframes_t FUNC206LXDEV034(struct snd_pcm_substream *substream)
{
	struct V206DEV139 *mwv = snd_pcm_substream_chip(substream);
	return bytes_to_frames(substream->runtime, mwv->V206HDMIAUDIO006 % snd_pcm_lib_buffer_bytes(substream));
}

static struct snd_pcm_hardware mwv206_audio_pcm_hw = {

	.info               = 	SNDRV_PCM_INFO_INTERLEAVED |
				SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.formats            = SNDRV_PCM_FMTBIT_S16,
	.rates              = 	SNDRV_PCM_RATE_32000 |
				SNDRV_PCM_RATE_44100 |
				SNDRV_PCM_RATE_48000 |
				SNDRV_PCM_RATE_96000,
	.rate_min           = 32000,
	.rate_max           = 96000,
	.channels_min       = 2,
	.channels_max       = 2,
	.buffer_bytes_max   = MAC206LXDEV010,
	.period_bytes_min   = MAC206LXDEV028,
	.period_bytes_max   = MAC206LXDEV027,
	.periods_min        = MAC206LXDEV026,
	.periods_max        = MAC206LXDEV025,
};

static int FUNC206LXDEV033(struct snd_pcm_substream *substream)
{
	struct V206DEV139 *mwv = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	V206DEV025 *pDev = mwv->pdata;
	int V206HDMIAUDIO027 = mwv->V206HDMIAUDIO027, ret;
	unsigned long flags;

	spin_lock_irqsave(&mwv->lock, flags);



	ret = snd_pcm_limit_hw_rates(runtime);
	if (ret < 0) {
		spin_unlock_irqrestore(&mwv->lock, flags);
		return ret;
	}

	ret = snd_pcm_hw_constraint_integer(runtime,
			SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		spin_unlock_irqrestore(&mwv->lock, flags);
		return ret;
	}


	ret = snd_pcm_hw_constraint_minmax(runtime,
			SNDRV_PCM_HW_PARAM_BUFFER_SIZE,
			0, substream->dma_buffer.bytes);
	if (ret < 0) {
		spin_unlock_irqrestore(&mwv->lock, flags);
		return ret;
	}

	FUNC206LXDEV138(pDev, V206HDMIAUDIO027);

	mwv->V206HDMIAUDIO003 = substream;
	runtime->hw = mwv206_audio_pcm_hw;
	snd_pcm_set_sync(substream);
	spin_unlock_irqrestore(&mwv->lock, flags);

	return 0;
}

static int FUNC206LXDEV032(struct snd_pcm_substream *substream)
{
	struct V206DEV139 *mwv = snd_pcm_substream_chip(substream);
	unsigned long flags;

	spin_lock_irqsave(&mwv->lock, flags);
	mwv->V206HDMIAUDIO003 = NULL;
	spin_unlock_irqrestore(&mwv->lock, flags);

	return 0;
}

static int FUNC206LXDEV028(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *hw_params)
{
	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
}

static int FUNC206LXDEV027(struct snd_pcm_substream *substream)
{
	return snd_pcm_lib_free_pages(substream);
}

int FUNC206LXDEV139(struct snd_pcm_substream *substream, int channel,
	snd_pcm_uframes_t pos, void __user *buf, snd_pcm_uframes_t count)
{
	unsigned int real_bytes;
	int ret, avail, len;
	unsigned long flags;
	struct V206DEV139 *mwv = snd_pcm_substream_chip(substream);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 9, 58)
	struct snd_pcm_runtime *runtime = substream->runtime;
	real_bytes = frames_to_bytes(runtime, count);
#else
	real_bytes = count ;
#endif

	ret = copy_from_user(mwv->kbuf, buf, real_bytes);
	if (ret) {
		return ret;
	}
	spin_lock_irqsave(&mwv->lock, flags);
	avail = kfifo_avail(&mwv->fifo);
	len = avail > real_bytes ? real_bytes : avail;
	kfifo_in(&mwv->fifo, mwv->kbuf, len);
	spin_unlock_irqrestore(&mwv->lock, flags);

	return 0;
}

static struct snd_pcm_ops mwv206_audio_playback_ops = {
	.open         =   FUNC206LXDEV033,
	.close        =   FUNC206LXDEV032,
	.ioctl        =   snd_pcm_lib_ioctl,
	.hw_params    =   FUNC206LXDEV028,
	.hw_free      =   FUNC206LXDEV027,
	.prepare      =   FUNC206LXDEV035,
	.trigger      =   FUNC206LXDEV036,
	.pointer      =   FUNC206LXDEV034,

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 9, 158)
	.copy         =   FUNC206LXDEV139,
#else
	.copy_user    =   FUNC206LXDEV139,
#endif
};

static int FUNC206LXDEV031(struct V206DEV139 *mwv)
{
	struct snd_pcm *pcm;
	int err;


	err = snd_pcm_new(mwv->card, mwv->card->shortname, 0, 1, 0, &pcm);
	if (err < 0) {
		return err;
	}

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &mwv206_audio_playback_ops);

	pcm->private_data = mwv;
	pcm->info_flags = 0;
	strcpy(pcm->name, mwv->card->driver);
	mwv->pcm = pcm;

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
			snd_dma_continuous_data(GFP_KERNEL),
			128 * 1024, 1024 * 1024);

	return 0;
}

static int FUNC206LXDEV079(struct device *dev, V206DEV025 *pri, int V206HDMIAUDIO027)
{
	struct V206DEV139 *mwv;
	struct snd_card *card;
	int err;

	err = snd_card_new(dev, -1, NULL, THIS_MODULE, sizeof(struct V206DEV139) + (MAC206LXDEV010 << 2), &card);
	if (err < 0) {
		V206KDEBUG002("[ERROR] HDA-MWV206 new card failed(%d).\n", err);
		return err;
	}

	sprintf(card->driver, "MWV206 HDMI_%d", V206HDMIAUDIO027);
	sprintf(card->shortname, "HDMI_%d ", V206HDMIAUDIO027);
	sprintf(card->longname, "%s at 0x%02lx:0x%x, irq %d",
			card->shortname,
			0 & 0xffL,
			0,
			0);

	mwv = card->private_data;
	spin_lock_init(&mwv->lock);
	mwv->card  = card;
	mwv->pdata = pri;

	err = FUNC206LXDEV031(mwv);
	if (err < 0) {
		V206KDEBUG002("[ERROR] HDA-MWV206 pcm failed(%d).\n", err);
		goto out_err;
	}

	kfifo_init(&mwv->fifo, card->private_data + sizeof(struct V206DEV139), MAC206LXDEV010 << 2);

	pri->audio[V206HDMIAUDIO027] = mwv;
	mwv->V206HDMIAUDIO027 = V206HDMIAUDIO027;
	mwv->rate = 44100;

	err = snd_card_register(card);
	if (err < 0) {
		V206KDEBUG002("[ERROR] HDA-MWV206 registration failed(%d).\n", err);
		goto out_err;
	}

	return 0;

out_err:
	V206KDEBUG002("[ERROR] HDA-MWV206 sbus probe failed.\n");
	snd_card_free(card);
	return err;

}


int FUNC206LXDEV141(V206DEV025 *pDev, long args)
{
	static int period_inited;

	if (disable_audio) {
		V206KDEBUG003("[INFO] HDMI audio is disabled.\n");
		return 0;
	}

	if (!pDev->V206DEV155) {
		V206KDEBUG003("[INFO] 16M register bar is detected, HDMI audio is disabled.\n");
		return 0;
	}

	if (args < 0 || args > 3) {
		V206KDEBUG003("[INFO] HDMI audio get Invalid args.\n");
		return 0;
	}

	if (args == 3) {
		V206KDEBUG003("[INFO] HDMI3 audio is disabled for the moment.\n");
		return 0;
	}

	if (!pDev->V206DEV142) {
		return 0;
	}

	if (!period_inited) {
		timer_period = ktime_set(0, 4000000);
		period_inited = 1;
	}

	FUNC206LXDEV140(pDev, 1);

	if (pDev->V206DEV141[args] == 0 && pDev->pixelclock[args]) {
		struct V206DEV139 *mwv;
		int ret = 0;

		ret = FUNC206LXDEV079(&pDev->V206DEV103->dev, pDev, args);
		if (ret) {
			V206KDEBUG002("[ERROR] HDM-MWV206 hdmi probe failed(%d).\n", ret);
			FUNC206LXDEV140(pDev, 0);
			return -1;
		}

		mwv = pDev->audio[args];
		mwv->hrtime = ktime_get();
		hrtimer_init(&mwv->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		mwv->timer.function = mwv206_audio_timer_task;
		hrtimer_start(&mwv->timer, timer_period, HRTIMER_MODE_REL);

		pDev->V206DEV141[args] = 1;
		FUNC206LXDEV140(pDev, 0);
		return 0;
	}
	FUNC206LXDEV140(pDev, 0);
	return 0;
}

int FUNC206LXDEV142(V206DEV025 *pDev, long args)
{
	if (!pDev->V206DEV155) {
		return 0;
	}

	if (args < 0 || args >= 3) {
		V206KDEBUG003("[INFO] HDMI audio get Invalid args.\n");
		return 0;
	}

	FUNC206LXDEV140(pDev, 1);
	if (pDev->V206DEV141[args]) {
		struct V206DEV139 *mwv = pDev->audio[args];
		hrtimer_cancel(&mwv->timer);
		snd_card_disconnect(mwv->card);
		snd_card_free_when_closed(mwv->card);

		pDev->V206DEV141[args] = 0;
	}
	FUNC206LXDEV140(pDev, 0);
	return 0;
}

int mwv206_hdmi_suspend(V206DEV025 *pDev)
{
	int i;

	if (pDev == NULL) {
		return 0;
	}

	FUNC206LXDEV140(pDev, 1);
	for (i = 0; i < 4; i++) {
		if (pDev->V206DEV141[i]) {
			struct V206DEV139 *mwv = pDev->audio[i];

			hrtimer_cancel(&mwv->timer);
			snd_power_change_state(mwv->card, SNDRV_CTL_POWER_D3hot);
			snd_pcm_suspend_all(mwv->pcm);
		}
	}
	FUNC206LXDEV140(pDev, 0);

	return 0;
}


int mwv206_hdmi_resume(V206DEV025 *pDev)
{
	int i;

	if (pDev == NULL) {
		return 0;
	}

	FUNC206LXDEV140(pDev, 1);
	for (i = 0; i < 4; i++) {
		if (pDev->V206DEV141[i]) {
			struct V206DEV139 *mwv = pDev->audio[i];

			snd_power_change_state(mwv->card, SNDRV_CTL_POWER_D0);
			hrtimer_start(&mwv->timer, timer_period, HRTIMER_MODE_REL);
		}
	}
	FUNC206LXDEV140(pDev, 0);

	return 0;
}

void FUNC206LXDEV024(struct pci_dev *V206DEV103)
{
	V206DEV025 *pDev = pci_get_drvdata(V206DEV103);
	int i;

	FUNC206LXDEV140(pDev, 1);
	for (i = 0; i < 4; i++) {
		if (pDev->V206DEV141[i]) {
			struct V206DEV139 *mwv = pDev->audio[i];
			hrtimer_cancel(&mwv->timer);
			snd_card_free(mwv->card);
			pDev->V206DEV141[i] = 0;
		}
	}
	FUNC206LXDEV140(pDev, 0);
}
#endif