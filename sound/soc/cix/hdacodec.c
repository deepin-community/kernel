// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.
#include "hdac.h"
#include "hdacodec.h"

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/export.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <sound/core.h>
#include <sound/hda_codec.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <linux/acpi.h>
#include <linux/of_platform.h>

#include <linux/mod_devicetable.h>

#define DEV_NAME_LEN 32

static struct hda_codec *hda_codec_device_init(struct hdac_bus *bus, int addr);

int cix_snd_hda_codec_set_name(struct hda_codec *codec, const char *name)
{
	int err;

	if (!name)
		return 0;
	err = snd_hdac_device_set_chip_name(&codec->core, name);
	if (err < 0)
		return err;

	return 0;
}

static int hda_codec_config(struct hda_codec_priv *hcp, int stream,
			    struct snd_pcm_hw_params *params)
{
	struct hda_codec *codec = hcp->codec;
	struct hda_codec_pdata *codec_pdata = hcp->codec_pdata;
	struct hdac_device *hdev = &codec->core;
	unsigned int format_val, i;
	hda_nid_t nid;

	// TODO: update verbs by params
	codec_pdata->ops->hw_params(codec, stream);

	format_val = snd_hdac_calc_stream_format(params_rate(params),
						 params_channels(params),
						 params_format(params),
						 32,
						 0);

	nid = hdev->start_nid;
	for (i = 0; i < hdev->num_nodes; i++, nid++) {
		unsigned int type;

		type = get_wcaps_type(get_wcaps(hdev, nid));
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (type == AC_WID_AUD_OUT) {
				snd_hda_codec_write(codec, nid, 0,
						    AC_VERB_SET_STREAM_FORMAT,
						    format_val);
				continue;
			}
		} else {
			if (type == AC_WID_AUD_IN) {
				snd_hda_codec_write(codec, nid, 0,
						    AC_VERB_SET_STREAM_FORMAT,
						    format_val);
				continue;
			}
		}
	}

	return 0;
}

static int hda_codec_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct hda_codec_priv *hcp = snd_soc_component_get_drvdata(component);
	struct hda_codec *codec = hcp->codec;

	if (!codec) {
		dev_err(component->dev, "no codec setups yet");
		return -EINVAL;
	}

	dev_info(component->dev, "bitdepth:%d, channels:%d, rate:%d\n",
		 params_width(params),
		 params_channels(params),
		 params_rate(params));

	hcp->codec_params[substream->stream] =
		devm_kzalloc(hcp->dev, sizeof(struct snd_pcm_hw_params),
			     GFP_KERNEL);
	if (!hcp->codec_params[substream->stream])
		return -ENOMEM;

	memcpy(hcp->codec_params[substream->stream],
	       params, sizeof(struct snd_pcm_hw_params));

	return hda_codec_config(hcp, substream->stream, params);
}

static int hda_codec_hw_free(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct hda_codec_priv *hcp = snd_soc_component_get_drvdata(component);

	if (hcp->codec_params[substream->stream]) {
		devm_kfree(hcp->dev, hcp->codec_params[substream->stream]);
		hcp->codec_params[substream->stream] = NULL;
	}

	return 0;
}

static const struct snd_soc_dai_ops hda_codec_dai_ops = {
	.hw_params = hda_codec_hw_params,
	.hw_free   = hda_codec_hw_free,
};

static const struct snd_soc_component_driver hda_codec_dev = {
	.use_pmdown_time        = 1,
	.endianness             = 1,
};

static struct snd_soc_dai_driver hda_codec_dai[] = {
	{
		.name = HDA_CODEC_DRV_NAME,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 2,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats =  HDA_CODEC_FMT,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 2,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats =  HDA_CODEC_FMT,
		},
		.ops = &hda_codec_dai_ops,
	}
};

static void snd_hda_codec_dev_release(struct device *dev)
{
	struct hda_codec *codec = dev_to_hda_codec(dev);

	snd_hdac_device_exit(&codec->core);
	kfree(codec->modelname);
	kfree(codec->wcaps);
	kfree(codec);
}


struct hda_codec *cix_snd_hda_codec_device_init(struct hda_bus *bus,
	unsigned int codec_addr,
	const char *fmt, ...)
{
	va_list vargs;
	char name[DEV_NAME_LEN];
	struct hda_codec *codec;
	int err;

	if (snd_BUG_ON(!bus))
		return ERR_PTR(-EINVAL);
	if (snd_BUG_ON(codec_addr > HDA_MAX_CODEC_ADDRESS))
		return ERR_PTR(-EINVAL);

	codec = devm_kzalloc(bus->core.dev, sizeof(*codec), GFP_KERNEL);
	if (!codec)
		return ERR_PTR(-ENOMEM);

	va_start(vargs, fmt);
	vsprintf(name, fmt, vargs);
	va_end(vargs);

	err = snd_hdac_device_init(&codec->core, &bus->core, name, codec_addr);
	if (err < 0) {
		devm_kfree(bus->core.dev, codec);
		return ERR_PTR(err);
	}

	codec->bus = bus;
	codec->depop_delay = -1;
	codec->core.dev.release = snd_hda_codec_dev_release;
	codec->core.type = HDA_DEV_LEGACY;

	return codec;
}

static struct hda_codec *hda_codec_device_init(struct hdac_bus *bus, int addr)
{
	struct hda_codec *codec;
	int ret;

	codec = cix_snd_hda_codec_device_init(to_hda_bus(bus), addr,
					  "hdaudio%dD%d", bus->idx, addr);
	if (IS_ERR(codec)) {
		dev_err(bus->dev,
			"device init failed for hdac device\n");
		return codec;
	}

	codec->core.type = HDA_DEV_ASOC;

	ret = snd_hdac_device_register(&codec->core);
	if (ret) {
		dev_err(bus->dev,
			"failed to register hdac device\n");
		put_device(&codec->core.dev);
		return ERR_PTR(ret);
	}

	return codec;
}

extern const struct of_device_id snd_hda_id_realtek_of_match[];
extern struct hdac_bus *get_ipb_hda_bus(void);

int hda_codec_probe(struct platform_device *pdev)
{
	struct hda_codec_bus_pdata *hcbd = pdev->dev.platform_data;
	struct device *dev = &pdev->dev;
	hda_codec_patch_t patch;
	struct hdac_bus *bus;
	struct hda_codec_priv *hcp;
	struct hda_codec_pdata *codec_pdata;
	int ret;

	codec_pdata = (struct hda_codec_pdata *)device_get_match_data(dev);
	if (codec_pdata)
		patch = codec_pdata->data;

	hcp = devm_kzalloc(dev, sizeof(*hcp), GFP_KERNEL);
	if (!hcp)
		return -ENOMEM;

	if (hcbd) {
		hcp->hcbdata = *hcbd;
		bus = (struct hdac_bus *)(hcbd->bus);
	} else {
		dev_warn(dev,
			 "no platform data, get bus from controller data\n");

		bus = get_ipb_hda_bus();
		if (!bus)
			return -EPROBE_DEFER;
		hcp->codec_pdata = codec_pdata;
	}
	hcp->dev = dev;
	hcp->bus = bus;

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	platform_set_drvdata(pdev, hcp);

	/* ASoC specific initialization */
	ret = devm_snd_soc_register_component(dev, &hda_codec_dev,
					      hda_codec_dai,
					      ARRAY_SIZE(hda_codec_dai));

	if (ret < 0) {
		dev_err(dev, "failed to register hda codec component");
		pm_runtime_disable(dev);
		return ret;
	}

	dev_info(dev, "%s, %s probed\n", __func__, HDA_CODEC_DRV_NAME);

	return ret;
}

int hda_codec_remove(struct platform_device *pdev)
{
	struct hda_codec_priv *hcp = dev_get_drvdata(&pdev->dev);

	pm_runtime_disable(hcp->dev);

	return 0;
}

int __maybe_unused hda_codec_runtime_suspend(struct device *dev)
{
	struct hda_codec_priv *hcp = dev_get_drvdata(dev);
	struct hda_codec *codec = hcp->codec;

	if (codec) {
		if (codec->patch_ops.suspend)
			codec->patch_ops.suspend(codec);

		snd_hdac_device_unregister(&codec->core);
	}

	return 0;
}

int __maybe_unused hda_codec_runtime_resume(struct device *dev)
{
	struct hda_codec_priv *hcp = dev_get_drvdata(dev);
	struct hda_codec_pdata *codec_pdata = hcp->codec_pdata;
	hda_codec_patch_t patch = codec_pdata->data;
	struct hdac_bus *bus = hcp->bus;
	struct hda_codec *codec;
	int addr = 0;
	int ret;
	int i;

	// TODO: the given codec address
	codec = hda_codec_device_init(bus, addr);
	if (IS_ERR(codec)) {
		dev_err(dev,
			"failed to create hdac codec, error:0x%x\n",
			PTR_ERR_OR_ZERO(codec));
		return -EINVAL;
	}
	hcp->codec = codec;

	cix_snd_hda_codec_set_name(codec, dev_name(dev));

	if (codec && patch) {
		ret = patch(codec);
		if (ret < 0)
			goto error_exit;
	}

	for (i = 0; i < SNDRV_PCM_STREAM_LAST + 1; i++) {
		if (hcp->codec_params[i])
			hda_codec_config(hcp, i, hcp->codec_params[i]);
	}

	return ret;

 error_exit:
	snd_hdac_device_unregister(&codec->core);
	if (codec->patch_ops.free)
		codec->patch_ops.free(codec);

	return ret;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HDA codec Driver");
MODULE_AUTHOR("Xing Wang<xing.wang@cixtech.com>");
