// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.
#include <linux/acpi.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/ctype.h>
#include <sound/core.h>
#include <sound/jack.h>
#include <sound/hda_codec.h>
#include <linux/of.h>
#include "hdacodec.h"

#define snd_hda_regmap_sync(codec)	snd_hdac_regmap_sync(&(codec)->core)
#define AMP_OUT_MUTE			0xb080
#define PIN_OUT				(AC_PINCTL_OUT_EN)

/* extra amp-initialization sequence types */
enum {
	ALC_INIT_UNDEFINED,
	ALC_INIT_NONE,
	ALC_INIT_DEFAULT,
};

enum {
	ALC_HEADSET_MODE_UNKNOWN,
	ALC_HEADSET_MODE_UNPLUGGED,
	ALC_HEADSET_MODE_HEADSET,
	ALC_HEADSET_MODE_MIC,
	ALC_HEADSET_MODE_HEADPHONE,
};

enum {
	ALC_HEADSET_TYPE_UNKNOWN,
	ALC_HEADSET_TYPE_CTIA,
	ALC_HEADSET_TYPE_OMTP,
};

enum {
	ALC_KEY_MICMUTE_INDEX,
};

struct alc_spec {
	/* GPIO bits */
	unsigned int gpio_mask;
	unsigned int gpio_dir;
	unsigned int gpio_data;
	bool gpio_write_delay;	/* add a delay before writing gpio_data */

	struct mutex coef_mutex;

	hda_nid_t headset_mic_pin;
	hda_nid_t headphone_mic_pin;
	int current_headset_mode;
	int current_headset_type;

	/* hooks */
	void (*init_hook)(struct hda_codec *codec);
	void (*power_hook)(struct hda_codec *codec);
	void (*shutup)(struct hda_codec *codec);

	int init_amp;
	int codec_variant;	/* flag for other variants */

	unsigned int no_depop_delay:1;
	unsigned int done_hp_init:1;
	unsigned int no_shutup_pins:1;
	unsigned int ultra_low_power:1;
	unsigned int has_hs_key:1;
	unsigned int no_internal_mic_pin:1;
	unsigned int en_3kpull_low:1;

	unsigned int coef0;
	struct input_dev *kb_dev;
};

/*
 * COEF access helper functions
 */

static void coef_mutex_lock(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;

	snd_hda_power_up_pm(codec);
	mutex_lock(&spec->coef_mutex);
}

static void coef_mutex_unlock(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;

	mutex_unlock(&spec->coef_mutex);
	snd_hda_power_down_pm(codec);
}

static int __alc_read_coefex_idx(struct hda_codec *codec, hda_nid_t nid,
	unsigned int coef_idx)
{
	unsigned int val;

	snd_hda_codec_write(codec, nid, 0, AC_VERB_SET_COEF_INDEX, coef_idx);
	val = snd_hda_codec_read(codec, nid, 0, AC_VERB_GET_PROC_COEF, 0);

	return val;
}

static int alc_read_coefex_idx(struct hda_codec *codec, hda_nid_t nid,
	unsigned int coef_idx)
{
	unsigned int val;

	coef_mutex_lock(codec);
	val = __alc_read_coefex_idx(codec, nid, coef_idx);
	coef_mutex_unlock(codec);

	return val;
}

#define alc_read_coef_idx(codec, coef_idx) \
	alc_read_coefex_idx(codec, 0x20, coef_idx)

static void __alc_write_coefex_idx(struct hda_codec *codec, hda_nid_t nid,
	unsigned int coef_idx, unsigned int coef_val)
{
	snd_hda_codec_write(codec, nid, 0, AC_VERB_SET_COEF_INDEX, coef_idx);
	snd_hda_codec_write(codec, nid, 0, AC_VERB_SET_PROC_COEF, coef_val);
}

static void alc_write_coefex_idx(struct hda_codec *codec, hda_nid_t nid,
	unsigned int coef_idx, unsigned int coef_val)
{
	coef_mutex_lock(codec);
	__alc_write_coefex_idx(codec, nid, coef_idx, coef_val);
	coef_mutex_unlock(codec);
}

#define alc_write_coef_idx(codec, coef_idx, coef_val) \
	alc_write_coefex_idx(codec, 0x20, coef_idx, coef_val)

static void __alc_update_coefex_idx(struct hda_codec *codec, hda_nid_t nid,
	unsigned int coef_idx, unsigned int mask, unsigned int bits_set)
{
	unsigned int val = __alc_read_coefex_idx(codec, nid, coef_idx);

	if (val != -1)
		__alc_write_coefex_idx(codec, nid, coef_idx,
				       (val & ~mask) | bits_set);
}

static void alc_update_coefex_idx(struct hda_codec *codec, hda_nid_t nid,
	unsigned int coef_idx, unsigned int mask, unsigned int bits_set)
{
	coef_mutex_lock(codec);
	__alc_update_coefex_idx(codec, nid, coef_idx, mask, bits_set);
	coef_mutex_unlock(codec);
}

#define alc_update_coef_idx(codec, coef_idx, mask, bits_set)	\
	alc_update_coefex_idx(codec, 0x20, coef_idx, mask, bits_set)

static unsigned int alc_get_coef0(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;

	if (!spec->coef0)
		spec->coef0 = alc_read_coef_idx(codec, 0);

	return spec->coef0;
}

/* coef writes/updates batch */
struct coef_fw {
	unsigned char nid;
	unsigned char idx;
	unsigned short mask;
	unsigned short val;
};

#define UPDATE_COEFEX(_nid, _idx, _mask, _val) \
	{ .nid = (_nid), .idx = (_idx), .mask = (_mask), .val = (_val) }
#define WRITE_COEFEX(_nid, _idx, _val) UPDATE_COEFEX(_nid, _idx, -1, _val)
#define WRITE_COEF(_idx, _val) WRITE_COEFEX(0x20, _idx, _val)
#define UPDATE_COEF(_idx, _mask, _val) UPDATE_COEFEX(0x20, _idx, _mask, _val)

static void alc_write_gpio_data(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;

	snd_hda_codec_write(codec, 0x01, 0, AC_VERB_SET_GPIO_DATA,
			    spec->gpio_data);
}

static int alc_init(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;

	if (spec->init_hook)
		spec->init_hook(codec);

	//	TODO: may add verbs config

	return 0;
}

void alc_free(struct hda_codec *codec)
{
}

static inline void alc_shutup(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;

	if (spec && spec->shutup)
		spec->shutup(codec);
}

static int alc_suspend(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;

	alc_shutup(codec);
	if (spec && spec->power_hook)
		spec->power_hook(codec);

	return 0;
}

static int alc_resume(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;

	if (!spec->no_depop_delay)
		msleep(150); /* to avoid pop noise */
	codec->patch_ops.init(codec);
	snd_hda_regmap_sync(codec);
	hda_call_check_power_status(codec, 0x01);

	return 0;
}

static const struct hda_codec_ops alc_patch_ops = {
	.init = alc_init,
	.free = alc_free,
	.resume = alc_resume,
	.suspend = alc_suspend,
};

#define alc_codec_rename(codec, name) snd_hda_codec_set_name(codec, name)

/*
 * Rename codecs appropriately from COEF value or subvendor id
 */
struct alc_codec_rename_table {
	unsigned int vendor_id;
	unsigned short coef_mask;
	unsigned short coef_bits;
	const char *name;
};

static const struct alc_codec_rename_table rename_tbl[] = {
	{ 0x10ec0221, 0xf00f, 0x1003, "ALC231" },
	{ 0x10ec0269, 0xfff0, 0x3010, "ALC277" },
	{ 0x10ec0269, 0xf0f0, 0x2010, "ALC259" },
	{ 0x10ec0269, 0xf0f0, 0x3010, "ALC258" },
	{ 0x10ec0269, 0x00f0, 0x0010, "ALC269VB" },
	{ 0x10ec0269, 0xffff, 0xa023, "ALC259" },
	{ 0x10ec0269, 0xffff, 0x6023, "ALC281X" },
	{ 0x10ec0269, 0x00f0, 0x0020, "ALC269VC" },
	{ 0x10ec0269, 0x00f0, 0x0030, "ALC269VD" },
	{ 0x10ec0662, 0xffff, 0x4020, "ALC656" },
	{ 0x10ec0887, 0x00f0, 0x0030, "ALC887-VD" },
	{ 0x10ec0888, 0x00f0, 0x0030, "ALC888-VD" },
	{ 0x10ec0888, 0xf0f0, 0x3020, "ALC886" },
	{ 0x10ec0899, 0x2000, 0x2000, "ALC899" },
	{ 0x10ec0892, 0xffff, 0x8020, "ALC661" },
	{ 0x10ec0892, 0xffff, 0x8011, "ALC661" },
	{ 0x10ec0892, 0xffff, 0x4011, "ALC656" },
	{ } /* terminator */
};

static int alc_codec_rename_from_preset(struct hda_codec *codec)
{
	const struct alc_codec_rename_table *p;

	for (p = rename_tbl; p->vendor_id; p++) {
		if (p->vendor_id != codec->core.vendor_id)
			continue;
		if ((alc_get_coef0(codec) & p->coef_mask) == p->coef_bits)
			return alc_codec_rename(codec, p->name);
	}

	return 0;
}

/* common preparation job for alc_spec */
static int alc_alloc_spec(struct hda_codec *codec, hda_nid_t mixer_nid)
{
	struct alc_spec *spec = kzalloc(sizeof(*spec), GFP_KERNEL);
	int err;

	if (!spec)
		return -ENOMEM;
	codec->spec = spec;

	codec->single_adc_amp = 1;
	codec->spdif_status_reset = 1;
	codec->forced_resume = 1;
	codec->patch_ops = alc_patch_ops;
	mutex_init(&spec->coef_mutex);

	err = alc_codec_rename_from_preset(codec);
	if (err < 0) {
		kfree(spec);
		return err;
	}

	return 0;
}

/*
 * ALC269
 */

/* different alc269-variants */
enum {
	ALC269_TYPE_ALC269VA,
	ALC269_TYPE_ALC269VB,
	ALC269_TYPE_ALC269VC,
	ALC269_TYPE_ALC269VD,
	ALC269_TYPE_ALC280,
	ALC269_TYPE_ALC282,
	ALC269_TYPE_ALC283,
	ALC269_TYPE_ALC284,
	ALC269_TYPE_ALC293,
	ALC269_TYPE_ALC286,
	ALC269_TYPE_ALC298,
	ALC269_TYPE_ALC255,
	ALC269_TYPE_ALC256,
	ALC269_TYPE_ALC257,
	ALC269_TYPE_ALC215,
	ALC269_TYPE_ALC225,
	ALC269_TYPE_ALC245,
	ALC269_TYPE_ALC287,
	ALC269_TYPE_ALC294,
	ALC269_TYPE_ALC300,
	ALC269_TYPE_ALC623,
	ALC269_TYPE_ALC700,
};

/* get a primary headphone pin if available */
static hda_nid_t alc_get_hp_pin(struct alc_spec *spec)
{
	return 0;
}

static void alc256_init(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;
	hda_nid_t hp_pin = alc_get_hp_pin(spec);
	bool hp_pin_sense;

	if (spec->ultra_low_power) {
		alc_update_coef_idx(codec, 0x03, 1<<1, 1<<1);
		alc_update_coef_idx(codec, 0x08, 3<<2, 3<<2);
		alc_update_coef_idx(codec, 0x08, 7<<4, 0);
		alc_update_coef_idx(codec, 0x3b, 1<<15, 0);
		alc_update_coef_idx(codec, 0x0e, 7<<6, 7<<6);
		msleep(30);
	}

	if (!hp_pin)
		hp_pin = 0x21;

	msleep(30);

	hp_pin_sense = false;

	alc_update_coefex_idx(codec, 0x57, 0x04, 0x0007, 0x1); /* Low power */

	snd_hda_codec_write(codec, hp_pin, 0,
			    AC_VERB_SET_AMP_GAIN_MUTE, AMP_OUT_MUTE);

	if (hp_pin_sense || spec->ultra_low_power)
		msleep(85);

	snd_hda_codec_write(codec, hp_pin, 0,
			    AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_OUT);

	if (hp_pin_sense || spec->ultra_low_power)
		msleep(100);

	alc_update_coef_idx(codec, 0x46, 3 << 12, 0);
	alc_update_coefex_idx(codec, 0x57, 0x04, 0x0007, 0x4); /* Hight power */
	alc_update_coefex_idx(codec, 0x53, 0x02, 0x8000, 1 << 15); /* Clear bit */
	alc_update_coefex_idx(codec, 0x53, 0x02, 0x8000, 0 << 15);
	/*
	 * Expose headphone mic (or possibly Line In on some machines) instead
	 * of PC Beep on 1Ah, and disable 1Ah loopback for all outputs. See
	 * Documentation/sound/hd-audio/realtek-pc-beep.rst for details of
	 * this register.
	 */
	alc_write_coef_idx(codec, 0x36, 0x5757);
}

static void alc256_shutup(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;
	hda_nid_t hp_pin = alc_get_hp_pin(spec);
	bool hp_pin_sense;

	if (!hp_pin)
		hp_pin = 0x21;

	alc_update_coefex_idx(codec, 0x57, 0x04, 0x0007, 0x1); /* Low power */
	hp_pin_sense = false;

	snd_hda_codec_write(codec, hp_pin, 0,
			    AC_VERB_SET_AMP_GAIN_MUTE, AMP_OUT_MUTE);

	if (hp_pin_sense || spec->ultra_low_power)
		msleep(85);

	/* 3k pull low control for Headset jack. */
	/* NOTE: call this before clearing the pin, otherwise codec stalls */
	/* If disable 3k pulldown control for alc257,
	 * the Mic detection will not work correctly
	 * when booting with headset plugged.
	 * So skip setting it for the codec alc257
	 */
	if (spec->en_3kpull_low)
		alc_update_coef_idx(codec, 0x46, 0, 3 << 12);

	if (!spec->no_shutup_pins)
		snd_hda_codec_write(codec, hp_pin, 0,
				    AC_VERB_SET_PIN_WIDGET_CONTROL, 0x0);

	if (hp_pin_sense || spec->ultra_low_power)
		msleep(100);

	if (spec->ultra_low_power) {
		msleep(50);
		alc_update_coef_idx(codec, 0x03, 1<<1, 0);
		alc_update_coef_idx(codec, 0x08, 7<<4, 7<<4);
		alc_update_coef_idx(codec, 0x08, 3<<2, 0);
		alc_update_coef_idx(codec, 0x3b, 1<<15, 1<<15);
		alc_update_coef_idx(codec, 0x0e, 7<<6, 0);
		msleep(30);
	}
}

static int alc269_suspend(struct hda_codec *codec)
{
	return alc_suspend(codec);
}

static int alc269_resume(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;

	codec->patch_ops.init(codec);

	snd_hda_regmap_sync(codec);
	hda_call_check_power_status(codec, 0x01);

	/* on some machine, the BIOS will clear the codec gpio data when enter
	 * suspend, and won't restore the data after resume, so we restore it
	 * in the driver.
	 */
	if (spec->gpio_data)
		alc_write_gpio_data(codec);

	return 0;
}

static void alc269_shutup(struct hda_codec *codec)
{
}

static void alc269_fill_coef(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;
	int val;

	if (spec->codec_variant != ALC269_TYPE_ALC269VB)
		return;

	if ((alc_get_coef0(codec) & 0x00ff) < 0x015) {
		alc_write_coef_idx(codec, 0xf, 0x960b);
		alc_write_coef_idx(codec, 0xe, 0x8817);
	}

	if ((alc_get_coef0(codec) & 0x00ff) == 0x016) {
		alc_write_coef_idx(codec, 0xf, 0x960b);
		alc_write_coef_idx(codec, 0xe, 0x8814);
	}

	if ((alc_get_coef0(codec) & 0x00ff) == 0x017) {
		/* Power up output pin */
		alc_update_coef_idx(codec, 0x04, 0, 1<<11);
	}

	if ((alc_get_coef0(codec) & 0x00ff) == 0x018) {
		val = alc_read_coef_idx(codec, 0xd);
		if (val != -1 && (val & 0x0c00) >> 10 != 0x1) {
			/* Capless ramp up clock control */
			alc_write_coef_idx(codec, 0xd, val | (1<<10));
		}
		val = alc_read_coef_idx(codec, 0x17);
		if (val != -1 && (val & 0x01c0) >> 6 != 0x4) {
			/* Class D power on reset */
			alc_write_coef_idx(codec, 0x17, val | (1<<7));
		}
	}

	/* HP */
	alc_update_coef_idx(codec, 0x4, 0, 1<<11);
}

static int alc256_pb_config(struct hdac_bus *bus)
{
	static unsigned int verb_table[] = {
		0xf0000
		, 0xf0002
		, 0x02050046
		, 0x02040004
		, 0x0205001B
		, 0x02040A4B
		, 0x02050038
		, 0x02046901

		, 0x00220011
		, 0x00270610
		, 0x0023b057
		, 0x0143b000
		, 0x01470740
		, 0x01470C02
		, 0x0213b000
		, 0x021707C0
	};
	int i;

	if (!bus) {
		pr_err("codec alc256 config bus null\n");
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(verb_table); i++)
		bus->ops->command(bus,  verb_table[i]);

	return 0;
}

static int alc256_cp_config(struct hdac_bus *bus)
{
	static unsigned int verb_table[] = {
		 0x02040004
		, 0x0205001B
		, 0x02040A4B
		, 0x02050038
		, 0x02046901

		//check headset status
		, 0x02050046
		, 0x020C0000

		, 0x01970724
		, 0x01937002
		, 0x02337100
		, 0x00837017
		, 0x00820011
		, 0x00870610
	};
	int i;

	if (!bus) {
		pr_err("codec alc256 config bus null\n");
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(verb_table); i++)
		bus->ops->command(bus,  verb_table[i]);

	return 0;
}

static int alc_hw_params(struct hda_codec *codec, int stream)
{
	struct hdac_bus *bus = codec->core.bus;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		alc256_pb_config(bus);
	else
		alc256_cp_config(bus);

	return 0;
}

static void alc_default_init(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;
	hda_nid_t hp_pin = alc_get_hp_pin(spec);
	bool hp_pin_sense;

	if (!hp_pin)
		return;

	msleep(30);

	hp_pin_sense = false;

	snd_hda_codec_write(codec, hp_pin, 0,
			    AC_VERB_SET_AMP_GAIN_MUTE, AMP_OUT_MUTE);

	if (hp_pin_sense)
		msleep(85);

	snd_hda_codec_write(codec, hp_pin, 0,
			    AC_VERB_SET_PIN_WIDGET_CONTROL, PIN_OUT);

	if (hp_pin_sense)
		msleep(100);
}

static void alc_default_shutup(struct hda_codec *codec)
{
	struct alc_spec *spec = codec->spec;
	hda_nid_t hp_pin = alc_get_hp_pin(spec);
	bool hp_pin_sense;

	if (!hp_pin) {
		alc269_shutup(codec);
		return;
	}

	hp_pin_sense = false;

	snd_hda_codec_write(codec, hp_pin, 0,
			    AC_VERB_SET_AMP_GAIN_MUTE, AMP_OUT_MUTE);

	if (hp_pin_sense)
		msleep(85);

	if (!spec->no_shutup_pins)
		snd_hda_codec_write(codec, hp_pin, 0,
				    AC_VERB_SET_PIN_WIDGET_CONTROL, 0x0);

	if (hp_pin_sense)
		msleep(100);
}


/*
 */
static int patch_alc269(struct hda_codec *codec)
{
	struct alc_spec *spec;
	int err;

	err = alc_alloc_spec(codec, 0x0b);
	if (err < 0)
		return err;
	spec = codec->spec;
	codec->power_save_node = 0;

	codec->patch_ops.suspend = alc269_suspend;
	codec->patch_ops.resume = alc269_resume;

	spec->shutup = alc_default_shutup;
	spec->init_hook = alc_default_init;

	switch (codec->core.vendor_id) {
	case 0x10ec0269:
		spec->codec_variant = ALC269_TYPE_ALC269VA;
		switch (alc_get_coef0(codec) & 0x00f0) {
		case 0x0010:
			spec->codec_variant = ALC269_TYPE_ALC269VB;
			break;
		case 0x0020:
			spec->codec_variant = ALC269_TYPE_ALC269VC;
			break;
		case 0x0030:
			spec->codec_variant = ALC269_TYPE_ALC269VD;
			break;
		default:
			break;
		}
		if (err < 0)
			goto error;
		spec->shutup = alc269_shutup;
		spec->init_hook = alc269_fill_coef;
		alc269_fill_coef(codec);
		break;
	case 0x10ec0256:
		spec->codec_variant = ALC269_TYPE_ALC256;
		spec->shutup = alc256_shutup;
		spec->init_hook = alc256_init;

		break;
	case 0x10ec0257:
		spec->codec_variant = ALC269_TYPE_ALC257;
		spec->shutup = alc256_shutup;
		spec->init_hook = alc256_init;

		break;

	}

	codec->patch_ops.init(codec);

	return 0;

 error:
	alc_free(codec);
	return err;
}

/*
 * patch entries
 */
const struct hda_codec_ext_ops alc_ops = {
	.hw_params = alc_hw_params,
};

const struct hda_codec_pdata alc_codec_pdata = {
	.ops = &alc_ops,
	.data = &patch_alc269,
};

const struct of_device_id snd_hda_id_realtek_of_match[] = {
	{ .compatible = "realtek,alc256", .data = &alc_codec_pdata},
	{ .compatible = "realtek,alc257", .data = &alc_codec_pdata},
	{ .compatible = "realtek,alc269", .data = &alc_codec_pdata},
	{}
};
MODULE_DEVICE_TABLE(of, snd_hda_id_realtek_of_match);

static const struct acpi_device_id snd_hda_id_realtek_acpi_match[] = {
	{ "CIXH6030", (kernel_ulong_t)&alc_codec_pdata}, /* alc256 */
	{ "CIXH6031", (kernel_ulong_t)&alc_codec_pdata}, /* alc257 */
	{ "CIXH6032", (kernel_ulong_t)&alc_codec_pdata}, /* alc269 */
	{ },
};
MODULE_DEVICE_TABLE(acpi, snd_hda_id_realtek_acpi_match);

HDA_CODEC_DRIVER_REGISTER(snd_hda_id_realtek_of_match,
			snd_hda_id_realtek_acpi_match);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Realtek HDA codec Driver");
MODULE_AUTHOR("Xing Wang<xing.wang@cixtech.com>");
