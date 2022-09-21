/*
 *  pmdk_dp.c
 *
 *  Copyright (c) 2021 Phytium Technology Co. Ltd
 *  Author: Yiqun Zhang <zhangyiqun@phytium.com.cn>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/jack.h>

struct pmdk_dp_private {
	struct snd_soc_jack jack0;
	struct snd_soc_jack jack1;
	struct snd_soc_jack jack2;
};

/* PMDK widgets */
static const struct snd_soc_dapm_widget pmdk_dp_dapm_widgets[] = {
	SND_SOC_DAPM_LINE("DP", NULL),
};

/* PMDK control */
static const struct snd_kcontrol_new pmdk_controls[] = {
	SOC_DAPM_PIN_SWITCH("DP"),
};

/* PMDK connections */
static const struct snd_soc_dapm_route pmdk_dp_audio_map[] = {
	{"DP", NULL, "TX"},
};

static struct snd_soc_jack_pin dp0_pins[] = {
	{
		.pin	= "DP/HDMI 0",
		.mask	= SND_JACK_LINEOUT,
	},
};

static struct snd_soc_jack_pin dp1_pins[] = {
	{
		.pin	= "DP/HDMI 1",
		.mask	= SND_JACK_LINEOUT,
	},
};

static struct snd_soc_jack_pin dp2_pins[] = {
	{
		.pin	= "DP/HDMI 2",
		.mask	= SND_JACK_LINEOUT,
	},
};

#define SMDK_DAI_FMT (SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | \
	SND_SOC_DAIFMT_CBS_CFS)

static int pmdk_dp0_init(struct snd_soc_pcm_runtime *runtime)
{
	struct snd_soc_card *card = runtime->card;
	struct pmdk_dp_private *priv = snd_soc_card_get_drvdata(card);
	struct snd_soc_component *component = runtime->codec_dai->component;
	int ret;

	ret = snd_soc_card_jack_new(card, "DP/HDMI 0",
				    SND_JACK_LINEOUT,
				    &priv->jack0, dp0_pins,
				    ARRAY_SIZE(dp0_pins));
	if (ret) {
		dev_err(card->dev, "Jack creation failed %d\n", ret);
		return ret;
	}
	snd_soc_component_set_jack(component, &priv->jack0, NULL);
	return ret;
}

static int pmdk_dp1_init(struct snd_soc_pcm_runtime *runtime)
{
	struct snd_soc_card *card = runtime->card;
	struct pmdk_dp_private *priv = snd_soc_card_get_drvdata(card);
	struct snd_soc_component *component = runtime->codec_dai->component;
	int ret;

	ret = snd_soc_card_jack_new(card, "DP/HDMI 1",
				    SND_JACK_LINEOUT,
				    &priv->jack1, dp1_pins,
				    ARRAY_SIZE(dp1_pins));
	if (ret) {
		dev_err(card->dev, "Jack creation failed %d\n", ret);
		return ret;
	}
	snd_soc_component_set_jack(component, &priv->jack1, NULL);
	return ret;
}

static int pmdk_dp2_init(struct snd_soc_pcm_runtime *runtime)
{
	struct snd_soc_card *card = runtime->card;
	struct pmdk_dp_private *priv = snd_soc_card_get_drvdata(card);
	struct snd_soc_component *component = runtime->codec_dai->component;
	int ret;

	ret = snd_soc_card_jack_new(card, "DP/HDMI 2",
				    SND_JACK_LINEOUT,
				    &priv->jack2, dp2_pins,
				    ARRAY_SIZE(dp2_pins));
	if (ret) {
		dev_err(card->dev, "Jack creation failed %d\n", ret);
		return ret;
	}
	snd_soc_component_set_jack(component, &priv->jack2, NULL);
	return ret;
}

static struct snd_soc_dai_link pmdk_dai0 = {
	.name = "Phytium dp0-audio",
	.stream_name = "Playback",
	.cpu_dai_name = "phytium-i2s-dp0",
	.codec_dai_name = "i2s-hifi",
	.platform_name = "snd-soc-dummy",
	.codec_name = "hdmi-audio-codec.0.auto",
	.dai_fmt = SMDK_DAI_FMT,
	.init = pmdk_dp0_init,
};

static struct snd_soc_dai_link pmdk_dai1 = {
	.name = "Phytium dp1-audio",
	.stream_name = "Playback",
	.cpu_dai_name = "phytium-i2s-dp1",
	.codec_dai_name = "i2s-hifi",
	.platform_name = "snd-soc-dummy",
	.codec_name = "hdmi-audio-codec.1.auto",
	.dai_fmt = SMDK_DAI_FMT,
	.init = pmdk_dp1_init,
};

static struct snd_soc_dai_link pmdk_dai2 = {
	.name = "Phytium dp2-audio",
	.stream_name = "Playback",
	.cpu_dai_name = "phytium-i2s-dp2",
	.codec_dai_name = "i2s-hifi",
	.platform_name = "snd-soc-dummy",
	.codec_name = "hdmi-audio-codec.2.auto",
	.dai_fmt = SMDK_DAI_FMT,
	.init = pmdk_dp2_init,
};

static struct snd_soc_card pmdk = {
	.name = "PMDK-I2S",
	.owner = THIS_MODULE,

	.dapm_widgets = pmdk_dp_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(pmdk_dp_dapm_widgets),
	.controls = pmdk_controls,
	.num_controls = ARRAY_SIZE(pmdk_controls),
	.dapm_routes = pmdk_dp_audio_map,
	.num_dapm_routes = ARRAY_SIZE(pmdk_dp_audio_map),
};

static int pmdk_sound_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &pmdk;
	struct pmdk_dp_private *priv;
	struct snd_soc_dai_link *pmdk_dai;
	int num_dp = 2;
	card->dev = &pdev->dev;
	device_property_read_u32(&pdev->dev, "num-dp", &num_dp);
	pmdk_dai = devm_kzalloc(&pdev->dev, num_dp * sizeof(*pmdk_dai), GFP_KERNEL);
	if (!pmdk_dai)
		return -ENOMEM;

	switch (num_dp) {
	case 1:
		pmdk_dai[0] = pmdk_dai0;
		break;
	case 2:
		pmdk_dai[0] = pmdk_dai0;
		pmdk_dai[1] = pmdk_dai1;
		break;
	case 3:
		pmdk_dai[0] = pmdk_dai0;
		pmdk_dai[1] = pmdk_dai1;
		pmdk_dai[2] = pmdk_dai2;
		break;
	default:
		return -EINVAL;
	}

	card->dai_link = pmdk_dai;
	card->num_links = num_dp;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	snd_soc_card_set_drvdata(card, priv);

	return devm_snd_soc_register_card(&pdev->dev, card);
}

static const struct acpi_device_id pmdk_sound_acpi_match[] = {
	{ "PHYT8006", 0},
	{ }
};
MODULE_DEVICE_TABLE(acpi, pmdk_sound_acpi_match);

static struct platform_driver pmdk_sound_driver = {
	.probe = pmdk_sound_probe,
	.driver = {
		.name = "pmdk_dp",
		.acpi_match_table = pmdk_sound_acpi_match,
#ifdef CONFIG_PM
		.pm = &snd_soc_pm_ops,
#endif
	},
};

module_platform_driver(pmdk_sound_driver);

MODULE_AUTHOR("Zhang Yiqun<zhangyiqun@phytium.com.cn>");
MODULE_DESCRIPTION("ALSA SoC PMDK DP");
MODULE_LICENSE("GPL");
