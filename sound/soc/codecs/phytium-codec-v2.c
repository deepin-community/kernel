// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium CODEC ALSA SoC Audio driver
 *
 * Copyright (C) 2023-2024, Phytium Technology Co., Ltd.
 *
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>
#include <linux/clocksource.h>
#include <linux/random.h>
#include <linux/timecounter.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/acpi.h>
#include <sound/tlv.h>
#include <linux/i2c.h>

#include "phytium-codec-v2.h"

#define PHYT_CODEC_V2_VERSION "1.1.1"
#define PHYTIUM_RATES (SNDRV_PCM_RATE_192000 | \
		SNDRV_PCM_RATE_96000 | \
		SNDRV_PCM_RATE_88200 | \
		SNDRV_PCM_RATE_8000_48000)
#define PHYTIUM_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
		SNDRV_PCM_FMTBIT_S18_3LE | \
		SNDRV_PCM_FMTBIT_S20_3LE | \
		SNDRV_PCM_FMTBIT_S24_LE | \
		SNDRV_PCM_FMTBIT_S32_LE)

static const struct snd_kcontrol_new phyt_snd_controls[] = {
	SOC_SINGLE("PCM Volume", PHYTIUM_CODEC_PLAYBACKE_VOL, 0, 0xc0, 0),
	SOC_SINGLE("Playback Volume1", PHYTIUM_CODEC_PLAYBACKE_OUT1_VOL, 0, 0x24, 0),
	SOC_SINGLE("Playback Volume2", PHYTIUM_CODEC_PLAYBACKE_OUT2_VOL, 0, 0x24, 0),

	SOC_SINGLE("Capture Digital Volume", PHYTIUM_CODEC_CAPTURE_VOL, 0, 0xc0, 0),
	SOC_SINGLE("Mic PGA Volume", PHYTIUM_CODEC_CAPTURE_IN1_VOL, 0, 8, 0),
};

/*
 * DAPM Controls
 */

/* Input Mux */
static const char * const phyt_mux_sel[] = {
	"Line 1", "Line 2"};

static const struct soc_enum phyt_mux_enum =
	SOC_ENUM_SINGLE(PHYTIUM_CODEC_INMUX_SEL, 0,
			ARRAY_SIZE(phyt_mux_sel),
			phyt_mux_sel);

static const struct snd_kcontrol_new phyt_pga_controls =
	SOC_DAPM_ENUM("Route", phyt_mux_enum);

/* dapm widgets */
static const struct snd_soc_dapm_widget phyt_dapm_widgets[] = {
	/* Input Signal */
	SND_SOC_DAPM_INPUT("INPUT1"),
	SND_SOC_DAPM_INPUT("INPUT2"),

	/* Input Mux */
	SND_SOC_DAPM_MUX("Input Mux", PHYTIUM_CODEC_INMUX_ENABLE, 0, 0, &phyt_pga_controls),

	/* Input ADC */
	SND_SOC_DAPM_ADC("Input ADC", "Capture", PHYTIUM_CODEC_ADC_ENABLE, 0, 0),

	/* Output DAC */
	SND_SOC_DAPM_DAC("Output DAC", "Playback", PHYTIUM_CODEC_DAC_ENABLE, 0, 0),

	/* Output Signal */
	SND_SOC_DAPM_OUTPUT("OUTPUT1"),
	SND_SOC_DAPM_OUTPUT("OUTPUT2"),
};

static const struct snd_soc_dapm_route phyt_dapm_routes[] = {
	{ "Input Mux", "Line 1", "INPUT1"},
	{ "Input Mux", "Line 2", "INPUT2"},

	{ "Input ADC", NULL, "Input Mux"},

	{ "OUTPUT1", NULL, "Output DAC"},
	{ "OUTPUT2", NULL, "Output DAC"},
};

static void phyt_codec_show_status(uint8_t status)
{
	switch (status) {
	case ERR_CODEC_SUCCESS:
		pr_err("success\n");
		break;
	case ERR_CODEC_DEV_BUSY:
		pr_err("device busy\n");
		break;
	case ERR_CODEC_RW_ERROR:
		pr_err("read/write error\n");
		break;
	case ERR_CODEC_NODEV:
		pr_err("no hw device\n");
		break;
	case ERR_CODEC_NO_INIT:
		pr_err("no init\n");
		break;
	default:
		pr_err("unknown error: %d\n", status);
		break;
	}
}

int phyt_codec_msg_set_cmd(struct phytium_codec *priv)
{
	struct phytcodec_cmd *ans_msg;
	int timeout = 5000, ret = 0;

	phyt_writel_reg(priv->regfile_base, PHYTIUM_CODEC_AP2RV_INT_STATE, SEND_INTR);

	ans_msg = priv->sharemem_base;

	while (timeout && (ans_msg->complete == PHYTCODEC_COMPLETE_NOT_READY
			|| ans_msg->complete == PHYTCODEC_COMPLETE_GOING)) {
		udelay(200);
		timeout--;
	}

	if (timeout == 0) {
		dev_err(priv->dev, "failed to receive msg, timeout\n");
		return -EINVAL;
	} else if (ans_msg->complete == PHYTCODEC_COMPLETE_SUCCESS) {
		dev_dbg(priv->dev, "receive msg successfully\n");
		if (ans_msg->status != 0) {
			phyt_codec_show_status(ans_msg->status);
			dev_err(priv->dev, "controller status error code:%d\n",
					ans_msg->status);
			return -EINVAL;
		}
	} else if (ans_msg->complete != PHYTCODEC_COMPLETE_SUCCESS) {
		phyt_codec_show_status(ans_msg->status);
		dev_err(priv->dev, "receive msg; error code:%d\n",
					ans_msg->complete);
		ret = -EINVAL;
	} else {
		dev_err(priv->dev, "unknown error");
		ret = -EINVAL;
	}

	return ret;
}

static int phyt_set_cmd(struct phytium_codec *priv,
				unsigned int cmd)
{
	struct phytcodec_cmd *msg = priv->sharemem_base;
	int ret = 0;

	msg->reserved = 0;
	msg->seq = 0;
	msg->cmd_id = PHYTCODEC_MSG_CMD_SET;
	msg->cmd_subid = cmd;
	msg->complete = 0;
	ret = phyt_codec_msg_set_cmd(priv);
	if (ret < 0) {
		dev_err(priv->dev, "set cmd_subid 0x%x failed\n", cmd);
		ret = -EINVAL;
		goto error;
	}
error:
	return ret;
}

static int phyt_pm_cmd(struct phytium_codec *priv,
				unsigned int cmd)
{
	struct phytcodec_cmd *msg = priv->sharemem_base;
	uint16_t total_regs_len;
	uint8_t *regs;
	int ret = 0, i = 0, cnt = 1;

	memset(msg, 0, sizeof(struct phytcodec_cmd));

	msg->reserved = 0;
	msg->seq = 0;
	msg->cmd_id = PHYTCODEC_MSG_CMD_SET;
	msg->cmd_subid = cmd;
	msg->complete = 0;
	msg->cmd_para.phytcodec_reg.cnt = 0;

	if (cmd == PHYTCODEC_MSG_CMD_SET_RESUME) {
		memcpy(msg->cmd_para.phytcodec_reg.regs, priv->regs, REG_SH_LEN);
		phyt_writel_reg(priv->regfile_base, PHYTIUM_CODEC_INT_MASK, 0x0);
		phyt_writel_reg(priv->regfile_base, PHYTIUM_CODEC_INT_ENABLE, 0x1);
	}
	ret = phyt_codec_msg_set_cmd(priv);
	if (ret < 0) {
		dev_err(priv->dev, "set cmd_subid 0x%x failed\n", cmd);
		return -EINVAL;
	}
	total_regs_len = msg->cmd_para.phytcodec_reg.total_regs_len;
	if (total_regs_len % REG_SH_LEN == 0)
		cnt = total_regs_len / REG_SH_LEN;
	else
		cnt = total_regs_len / REG_SH_LEN + 1;

	if (cmd == PHYTCODEC_MSG_CMD_SET_SUSPEND) {
		regs = kmalloc(total_regs_len, GFP_KERNEL);
		priv->regs = regs;

		for (i = 1; i < cnt; i++) {
			if (msg->cmd_para.phytcodec_reg.cnt != i) {
				dev_err(priv->dev, "error phytcodec_reg.cnt\n");
				ret = -EINVAL;
				goto error;
			}
			memcpy(regs, msg->cmd_para.phytcodec_reg.regs, REG_SH_LEN);
			regs += REG_SH_LEN;
			msg->complete = 0;
			ret = phyt_codec_msg_set_cmd(priv);
			if (ret < 0) {
				dev_err(priv->dev, "set cmd_subid 0x%x failed\n", cmd);
				ret = -EINVAL;
				goto error;
			}
		}
		memcpy(regs, msg->cmd_para.phytcodec_reg.regs,
			total_regs_len - REG_SH_LEN * (msg->cmd_para.phytcodec_reg.cnt - 1));
	} else if (cmd == PHYTCODEC_MSG_CMD_SET_RESUME) {
		regs = priv->regs;
		for (i = 1; i < cnt; i++) {
			if (msg->cmd_para.phytcodec_reg.cnt != i) {
				dev_err(priv->dev, "error phytcodec_reg.cnt\n");
				ret = -EINVAL;
				goto error;
			}
			regs += REG_SH_LEN;
			memcpy(msg->cmd_para.phytcodec_reg.regs, regs, REG_SH_LEN);
			msg->complete = 0;
			ret = phyt_codec_msg_set_cmd(priv);
			if (ret < 0) {
				dev_err(priv->dev, "set cmd_subid 0x%x failed\n", cmd);
				ret = -EINVAL;
				goto error;
			}
		}
		kfree(priv->regs);
		priv->regs = NULL;
	}
	return ret;

error:
	kfree(priv->regs);
	priv->regs = NULL;
	return ret;
}

static int phyt_get_cmd(struct phytium_codec *priv, unsigned int cmd)
{
	struct phytcodec_cmd *msg = priv->sharemem_base;
	int ret = 0, i = 0, cnt = 1;
	uint16_t total_regs_len;
	uint8_t *regs;

	msg->reserved = 0;
	msg->seq = 0;
	msg->cmd_id = PHYTCODEC_MSG_CMD_GET;
	msg->cmd_subid = cmd;
	msg->complete = 0;
	if (cmd == PHYTCODEC_MSG_CMD_GET_ALL_REGS)
		msg->cmd_para.phytcodec_reg.cnt = 0;
	ret = phyt_codec_msg_set_cmd(priv);
	if (ret < 0) {
		dev_err(priv->dev, "get cmd_subid 0x%x failed\n", cmd);
		return -EINVAL;
	}

	total_regs_len = msg->cmd_para.phytcodec_reg.total_regs_len;
	if (cmd == PHYTCODEC_MSG_CMD_GET_ALL_REGS) {
		if (total_regs_len % REG_SH_LEN == 0)
			cnt = total_regs_len / REG_SH_LEN;
		else
			cnt = total_regs_len / REG_SH_LEN + 1;
		regs = kmalloc(total_regs_len, GFP_KERNEL);
		priv->regs = regs;
		for (i = 1; i < cnt; i++) {
			if (msg->cmd_para.phytcodec_reg.cnt != i) {
				dev_err(priv->dev, "error phytcodec_reg.cnt\n");
				ret = -EINVAL;
				goto error;
			}
			memcpy(regs, msg->cmd_para.phytcodec_reg.regs, REG_SH_LEN);
			regs += REG_SH_LEN;
			msg->complete = 0;
			ret = phyt_codec_msg_set_cmd(priv);
			if (ret < 0) {
				dev_err(priv->dev, "set cmd_subid 0x%x failed\n", cmd);
				ret = -EINVAL;
				goto error;
			}
		}
		memcpy(regs, msg->cmd_para.phytcodec_reg.regs,
			total_regs_len - REG_SH_LEN * (msg->cmd_para.phytcodec_reg.cnt - 1));
		for (i = 0; i < total_regs_len; i++)
			dev_info(priv->dev, "0x%02x-0x%02x\n", i, priv->regs[i]);

		kfree(priv->regs);
		priv->regs = NULL;
	}
	return ret;
error:
	kfree(priv->regs);
	priv->regs = NULL;
	return -EINVAL;
}

static int phyt_probe(struct snd_soc_component *component)
{
	struct phytium_codec *priv = snd_soc_component_get_drvdata(component);
	struct phytcodec_cmd *msg = priv->sharemem_base;

	memset(msg, 0, sizeof(struct phytcodec_cmd));

	return phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_PROBE);
}

static void phyt_remove(struct snd_soc_component *component)
{
	struct phytium_codec *priv = snd_soc_component_get_drvdata(component);
	struct phytcodec_cmd *msg = priv->sharemem_base;

	memset(msg, 0, sizeof(struct phytcodec_cmd));

	phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_REMOVE);
}

static int phyt_suspend(struct snd_soc_component *component)
{
	struct phytium_codec *priv = snd_soc_component_get_drvdata(component);
	struct phytcodec_cmd *msg = priv->sharemem_base;

	memset(msg, 0, sizeof(struct phytcodec_cmd));

	return phyt_pm_cmd(priv, PHYTCODEC_MSG_CMD_SET_SUSPEND);
}

static int phyt_resume(struct snd_soc_component *component)
{
	struct phytium_codec *priv = snd_soc_component_get_drvdata(component);
	struct phytcodec_cmd *msg = priv->sharemem_base;

	memset(msg, 0, sizeof(struct phytcodec_cmd));

	return phyt_pm_cmd(priv, PHYTCODEC_MSG_CMD_SET_RESUME);
}

static int phyt_set_bias_level(struct snd_soc_component *component,
				 enum snd_soc_bias_level level)
{
	int ret;
	struct phytium_codec *priv = snd_soc_component_get_drvdata(component);
	struct phytcodec_cmd *msg = priv->sharemem_base;

	memset(msg, 0, sizeof(struct phytcodec_cmd));
	msg->cmd_para.para[0] = priv->channels/2;

	switch (level) {
	case SND_SOC_BIAS_ON:
		ret = phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_BIAS_ON);
		break;

	case SND_SOC_BIAS_PREPARE:
		ret = phyt_set_cmd(priv,  PHYTCODEC_MSG_CMD_SET_BIAS_PREPARE);
		break;

	case SND_SOC_BIAS_STANDBY:
		if (snd_soc_component_get_bias_level(component) == SND_SOC_BIAS_OFF)
			ret = phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_BIAS_STANDBY);
		else
			ret = phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_BIAS_STANDBY);
		break;

	case SND_SOC_BIAS_OFF:
		ret = phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_BIAS_OFF);
		break;
	}

	return ret;
}

static const struct snd_soc_component_driver phyt_component_driver = {
	.probe			= phyt_probe,
	.remove			= phyt_remove,
	.suspend		= phyt_suspend,
	.resume			= phyt_resume,
	.set_bias_level		= phyt_set_bias_level,
	.controls		= phyt_snd_controls,
	.num_controls		= ARRAY_SIZE(phyt_snd_controls),
	.dapm_widgets		= phyt_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(phyt_dapm_widgets),
	.dapm_routes		= phyt_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(phyt_dapm_routes),
	.suspend_bias_off	= 1,
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
};

static int phyt_mute(struct snd_soc_dai *dai, int mute, int direction)
{
	int ret;
	struct snd_soc_component *component = dai->component;
	struct phytium_codec *priv = snd_soc_component_get_drvdata(component);
	struct phytcodec_cmd *msg = priv->sharemem_base;

	memset(msg, 0, sizeof(struct phytcodec_cmd));
	msg->cmd_para.para[0] = (uint8_t)direction;
	if (mute)
		ret = phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_MUTE);
	else
		ret = phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_UNMUTE);

	return ret;
}

static int phyt_startup(struct snd_pcm_substream *substream,
			  struct snd_soc_dai *dai)
{
	int ret;
	struct snd_soc_component *component = dai->component;
	struct phytium_codec *priv = snd_soc_component_get_drvdata(component);
	struct phytcodec_cmd *msg = priv->sharemem_base;

	memset(msg, 0, sizeof(struct phytcodec_cmd));

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_STARTUP);
	else
		ret = phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_STARTUP_RC);

	return ret;
}

static void phyt_shutdown(struct snd_pcm_substream *substream,
			  struct snd_soc_dai *dai)
{
	int ret;
	struct snd_soc_component *component = dai->component;
	struct phytium_codec *priv = snd_soc_component_get_drvdata(component);
	struct phytcodec_cmd *msg = priv->sharemem_base;

	memset(msg, 0, sizeof(struct phytcodec_cmd));

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_SHUTDOWN);
	else
		ret = phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_SHUTDOWN_RC);
}

static int phyt_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	int wl, ret = 0;
	struct snd_soc_component *component = dai->component;
	struct phytium_codec *priv = snd_soc_component_get_drvdata(component);

	priv->channels = params_channels(params);
	switch (params_width(params)) {
	case PHYTCODEC_FORMAT_S16:
		wl = 3;
		break;
	case PHYTCODEC_FORMAT_S18:
		wl = 2;
		break;
	case PHYTCODEC_FORMAT_S20:
		wl = 1;
		break;
	case PHYTCODEC_FORMAT_S24:
		wl = 0;
		break;
	case PHYTCODEC_FORMAT_S32:
		wl = 4;
		break;
	default:
		ret = -EINVAL;
		goto error;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_component_write(component, PHYTIUM_CODEC_HW_PARAM, wl);
	else
		snd_soc_component_write(component, PHYTIUM_CODEC_HW_PARAM_RC, wl);

error:
	return ret;
}

static int phyt_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	int ret;
	struct snd_soc_component *component = codec_dai->component;
	struct phytium_codec *priv = snd_soc_component_get_drvdata(component);
	struct phytcodec_cmd *msg = priv->sharemem_base;

	memset(msg, 0, sizeof(struct phytcodec_cmd));

	if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) != SND_SOC_DAIFMT_CBS_CFS)
		return -EINVAL;

	if ((fmt & SND_SOC_DAIFMT_FORMAT_MASK) != SND_SOC_DAIFMT_I2S)
		return -EINVAL;

	if ((fmt & SND_SOC_DAIFMT_INV_MASK) != SND_SOC_DAIFMT_NB_NF)
		return -EINVAL;

	ret = phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_DAI_FMT);

	return ret;
}

static const struct snd_soc_dai_ops phyt_dai_ops = {
	.startup	 = phyt_startup,
	.shutdown	 = phyt_shutdown,
	.hw_params	 = phyt_hw_params,
	.mute_stream	= phyt_mute,
	.set_fmt	 = phyt_set_dai_fmt,
};

static struct snd_soc_dai_driver phyt_dai = {
	.name = "phytium-hifi-v2",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = PHYTIUM_RATES,
		.formats = PHYTIUM_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = PHYTIUM_RATES,
		.formats = PHYTIUM_FORMATS,
	},
	.ops = &phyt_dai_ops,
	.symmetric_rate = 1,
};

static const struct regmap_config phyt_codec_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = REG_MAX,
	.cache_type = REGCACHE_NONE,
};

void phyt_enable_debug(struct phytium_codec *priv)
{
	u32 reg;

	reg = phyt_readl_reg(priv->regfile_base, PHYTIUM_CODEC_DEBUG);
	phyt_writel_reg(priv->regfile_base,
		PHYTIUM_CODEC_DEBUG, reg | PHYTIUM_CODEC_DEBUG_ENABLE);
}

void phyt_disable_debug(struct phytium_codec *priv)
{
	u32 reg;

	reg = phyt_readl_reg(priv->regfile_base, PHYTIUM_CODEC_DEBUG);
	reg &= ~PHYTIUM_CODEC_DEBUG_ENABLE;
	phyt_writel_reg(priv->regfile_base, PHYTIUM_CODEC_DEBUG, reg);
}

void phyt_enable_alive(struct phytium_codec *priv)
{
	u32 reg;

	reg = phyt_readl_reg(priv->regfile_base, PHYTIUM_CODEC_DEBUG);
	phyt_writel_reg(priv->regfile_base,
		PHYTIUM_CODEC_DEBUG, reg | PHYTIUM_CODEC_ALIVE_ENABLE);
}

void phyt_disable_alive(struct phytium_codec *priv)
{
	u32 reg;

	reg = phyt_readl_reg(priv->regfile_base, PHYTIUM_CODEC_DEBUG);
	reg &= ~PHYTIUM_CODEC_ALIVE_ENABLE;
	phyt_writel_reg(priv->regfile_base, PHYTIUM_CODEC_DEBUG, reg);
}

void phyt_heartbeat(struct phytium_codec *priv)
{
	u32 reg;

	reg = phyt_readl_reg(priv->regfile_base, PHYTIUM_CODEC_DEBUG);
	phyt_writel_reg(priv->regfile_base,
		PHYTIUM_CODEC_DEBUG, reg | PHYTIUM_CODEC_HEARTBIT_VAL);
}

static void phyt_timer_handle(struct timer_list *t)
{
	struct phytium_codec *priv = from_timer(priv, t, timer);

	if (priv->alive_enabled && priv->heartbeat)
		priv->heartbeat(priv);

	mod_timer(&priv->timer, jiffies + msecs_to_jiffies(2000));
}

static int phyt_get_one_reg(struct phytium_codec *priv, uint8_t arg1, uint8_t arg2)
{
	struct phytcodec_cmd *msg = priv->sharemem_base;
	int ret = 0;

	memset(msg, 0, sizeof(struct phytcodec_cmd));
	msg->cmd_para.rw_data.addr = arg1;
	msg->cmd_para.rw_data.reg = arg2;
	ret = phyt_get_cmd(priv, PHYTCODEC_MSG_CMD_GET_ONE_REG);
	dev_info(priv->dev, "val: 0x%x\n", msg->cmd_para.rw_data.val);

	return ret;
}

static int phyt_set_one_reg(struct phytium_codec *priv, uint8_t arg1, uint8_t arg2, uint16_t arg3)
{
	struct phytcodec_cmd *msg = priv->sharemem_base;
	int ret = 0;

	memset(msg, 0, sizeof(struct phytcodec_cmd));
	msg->cmd_para.rw_data.addr = arg1;
	msg->cmd_para.rw_data.reg = arg2;
	msg->cmd_para.rw_data.val = arg3;
	ret = phyt_set_cmd(priv, PHYTCODEC_MSG_CMD_SET_ONE_REG);

	return ret;
}

static ssize_t debug_show(struct device *dev, struct device_attribute *da, char *buf)
{
	struct phytium_codec *priv = dev_get_drvdata(dev);
	ssize_t ret;
	u32 reg;
	dev_info(dev, "Usage: echo <command> [args...] > debug\n");
	dev_info(dev, "Usage: echo help 1 > debug for more details");

	reg = phyt_readl_reg(priv->regfile_base, PHYTIUM_CODEC_DEBUG);
	ret = sprintf(buf, "%x\n", reg);

	return ret;
}

static ssize_t debug_store(struct device *dev, struct device_attribute *da,
					const char *buf, size_t size)
{
	struct phytium_codec *priv = dev_get_drvdata(dev);
	char *arg1_str = NULL, *arg2_str = NULL, *arg3_str = NULL;
	uint8_t arg1 = 0, arg2 = 0;
	uint16_t arg3 = 0;
	char *cmd_buffer, *cmd;
	long value;
	u32 reg;
	int status;

	cmd_buffer = kmalloc(size + 1, GFP_KERNEL);
	if (!cmd_buffer)
		goto error;
	strscpy(cmd_buffer, buf, size + 1);

	cmd = strsep(&cmd_buffer, " ");
	if (!cmd) {
		dev_err(dev, "Invalid command argument\n");
		goto error;
	}

	arg1_str = strsep(&cmd_buffer, " ");
	if (arg1_str) {
		status = kstrtoul(arg1_str, 0, &value);
		if (status) {
			dev_err(dev, "Invalid value for arg1: %s\n", arg1_str);
			goto error;
		}
		arg1 = (uint8_t)value;
	}

	arg2_str = strsep(&cmd_buffer, " ");
	if (arg2_str) {
		status = kstrtoul(arg2_str, 0, &value);
		if (status) {
			dev_err(dev, "Invalid value for arg2: %s\n", arg2_str);
			goto error;
		}
		arg2 = (uint8_t)value;
	}

	arg3_str = strsep(&cmd_buffer, " ");
	if (arg3_str) {
		status = kstrtou16(arg3_str, 0, &arg3);
		if (status) {
			dev_err(dev, "Invalid value for arg3: %s\n", arg3_str);
			goto error;
		}
	}

	if (strcmp(cmd, "dbg") == 0) {
		if (!arg1_str || !arg2_str) {
			dev_err(dev, "debug command requires two arguments\n");
			goto error;
		}
		reg = phyt_readl_reg(priv->regfile_base, PHYTIUM_CODEC_DEBUG);
		if (arg1 == 1) {
			if (arg2 == 1) {
				priv->alive_enabled = true;
				reg |= BIT(arg1);
			} else if (arg2 == 0) {
				priv->alive_enabled = false;
				reg &= ~BIT(arg1);
			} else {
				dev_err(dev, "arg2 should be 0 or 1 for dbg command\n");
				goto error;
			}
		} else if (arg1 == 0) {
			if (arg2 == 1) {
				priv->debug_enabled = true;
				reg |= BIT(arg1);
			} else if (arg2 == 0) {
				priv->debug_enabled = false;
				reg &= ~BIT(arg1);
			} else {
				dev_err(dev, "arg2 should be 0 or 1 for dbg command\n");
				goto error;
			}
		} else {
			dev_err(dev, "arg1 should be 0 or 1 for dbg command\n");
			goto error;
		}
		phyt_writel_reg(priv->regfile_base, PHYTIUM_CODEC_DEBUG, reg);
	} else if (strcmp(cmd, "get") == 0) {
		if (!arg1_str || !arg2_str) {
			dev_err(dev, "get command requires two arguments\n");
			goto error;
		}
		phyt_get_one_reg(priv, arg1, arg2);
	} else if (strcmp(cmd, "set") == 0) {
		if (!arg1_str || !arg2_str || !arg3_str) {
			dev_err(dev, "set command requires three arguments\n");
			goto error;
		}
		phyt_set_one_reg(priv, arg1, arg2, arg3);
	} else if (strcmp(cmd, "dump") == 0) {
		if (!arg1_str) {
			dev_err(dev, "dump command requires one argument\n");
			goto error;
		}
		memset(priv->sharemem_base, 0, sizeof(struct phytcodec_cmd));
		phyt_get_cmd(priv, PHYTCODEC_MSG_CMD_GET_ALL_REGS);
	} else if (strcmp(cmd, "help") == 0) {
		dev_info(dev, "Available commands:\n"
			"dump all regs: echo \"dump\" > debug\n"
			"dbg: echo \"dbg 0 1\" > debug\n"
			"heartbeat: echo \"dbg 1 1\" > debug\n"
			"read a reg: echo \"get [addr] [reg]\" > debug\n"
			"write a reg: echo \"set [addr] [reg] [val]\" > debug\n");
	} else {
		dev_err(dev, "Unknown command: %s\n", cmd);
		goto error;
	}

	kfree(cmd_buffer);
	return size;

error:
	kfree(cmd_buffer);
	return -EINVAL;
}

static DEVICE_ATTR_RW(debug);

static struct attribute *phyt_codec_device_attr[] = {
	&dev_attr_debug.attr,
	NULL,
};

static const struct attribute_group phyt_codec_device_group = {
	.attrs = phyt_codec_device_attr,
};

static int phyt_get_channels(struct phytium_codec *priv)
{
	struct phytcodec_cmd *msg = priv->sharemem_base;
	int ret = 0;
	uint8_t channels;

	memset(msg, 0, sizeof(struct phytcodec_cmd));
	ret = phyt_get_cmd(priv, PHYTCODEC_MSG_CMD_GET_CHANNELS);
	channels = msg->cmd_para.para[0] * 2;

	return channels;
}

static void phyt_codec_init(struct phytium_codec *priv)
{
	phyt_disable_debug(priv);
	phyt_disable_alive(priv);
	priv->debug_enabled = false;
	priv->alive_enabled = false;
	priv->heartbeat = phyt_heartbeat;
	priv->timer.expires = jiffies + msecs_to_jiffies(10000);
	timer_setup(&priv->timer, phyt_timer_handle, 0);
	add_timer(&priv->timer);

	if (sysfs_create_group(&priv->dev->kobj, &phyt_codec_device_group))
		dev_warn(priv->dev, "failed to create sysfs\n");

	priv->channels = phyt_get_channels(priv);
	phyt_dai.playback.channels_max = priv->channels;
	phyt_dai.capture.channels_max = priv->channels;

	phyt_writel_reg(priv->regfile_base, PHYTIUM_CODEC_INT_MASK, 0x0);
	phyt_writel_reg(priv->regfile_base, PHYTIUM_CODEC_INT_ENABLE, 0x1);
}

static int phyt_codec_probe(struct platform_device *pdev)
{
	struct phytium_codec *priv;
	struct resource *res;
	int ret;
	struct device *dev;

	dev = &pdev->dev;
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(dev, "failed to alloc struct phytium_codec\n");
		ret = -ENOMEM;
		goto failed_alloc_phytium_codec;
	}

	dev_set_drvdata(dev, priv);
	priv->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->regfile_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->regfile_base)) {
		dev_err(&pdev->dev, "failed to ioremap resource0\n");
		ret = PTR_ERR(priv->regfile_base);
		goto failed_ioremap_res0;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	priv->sharemem_base = devm_ioremap_wc(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(priv->sharemem_base)) {
		dev_err(&pdev->dev, "failed to ioremap resource1\n");
		ret = PTR_ERR(priv->sharemem_base);
		goto failed_ioremap_res1;
	}

	priv->regmap = devm_regmap_init_mmio(dev, priv->regfile_base,
			     &phyt_codec_regmap_config);
	if (IS_ERR(priv->regmap)) {
		dev_err(dev, "failed to init regmap\n");
		ret = PTR_ERR(priv->regmap);
		goto failed_regmap_init;
	}

	phyt_codec_init(priv);

	ret = devm_snd_soc_register_component(dev, &phyt_component_driver,
					      &phyt_dai, 1);
	if (ret != 0) {
		dev_err(dev, "not able to register codec dai\n");
		goto failed_register_com;
	}

	return 0;
failed_register_com:
failed_regmap_init:
failed_ioremap_res1:
failed_ioremap_res0:
failed_alloc_phytium_codec:
	return ret;
}

static int phyt_codec_remove(struct platform_device *pdev)
{
	struct phytium_codec *priv = dev_get_drvdata(&pdev->dev);

	sysfs_remove_group(&pdev->dev.kobj, &phyt_codec_device_group);
	del_timer(&priv->timer);
	return 0;
}

static const struct of_device_id phyt_codec_of_match[] = {
	{ .compatible = "phytium,codec-2.0", },
	{ },
};
MODULE_DEVICE_TABLE(of, phyt_codec_of_match);

static const struct acpi_device_id phyt_codec_acpi_match[] = {
	{ "PHYT1002", 0},
	{ },
};
MODULE_DEVICE_TABLE(acpi, phyt_codec_acpi_match);

static struct platform_driver phyt_codec_driver = {
	.probe	= phyt_codec_probe,
	.remove = phyt_codec_remove,
	.driver	= {
		.name = "phytium-codec-v2",
		.of_match_table = of_match_ptr(phyt_codec_of_match),
		.acpi_match_table = phyt_codec_acpi_match,
	},
};

module_platform_driver(phyt_codec_driver);
MODULE_DESCRIPTION("Phytium CODEC V2 Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yang Xun <yangxun@phytium.com.cn>");
MODULE_VERSION(PHYT_CODEC_V2_VERSION);
