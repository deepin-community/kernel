// SPDX-License-Identifier: GPL-2.0
/*
 * zhaoxin KX7000 pinctrl/GPIO driver
 *
 * Copyright(c) 2023 Shanghai Zhaoxin Corporation. All rights reserved.
 *
 */

#define DRIVER_VERSION "1.0.0"

#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-zhaoxin.h"

#define ZX_CAL_ARRAY(a, b) \
{ \
	.pmio_offset = (a), \
	.size = (b), \
}

#define PMIO_RX90 100
#define PMIO_RX8C 200

#define ZX_CAL_INDEX_ARRAY(a, b, c) \
{ \
	.reg_port_base = (PMIO_RX90), \
	.reg_data_base = (PMIO_RX8C), \
	.index = (a), \
	.cal_array = (b), \
	.size = (c), \
}

/* kx7000 pin define */
static const struct pinctrl_pin_desc kx7000_pins[] = {

	PINCTRL_PIN(0, "IOD_CPUTCK"),
	PINCTRL_PIN(1, "IOD_CPUTMS"),
	PINCTRL_PIN(2, "IOD_CPUTRST"),
	PINCTRL_PIN(3, "IOD_CPUTDO"),
	PINCTRL_PIN(4, "IOD_CPUTDI"),
	PINCTRL_PIN(5, "IOD_ZLSCLK0"),
	PINCTRL_PIN(6, "IOD_ZLDATA0"),
	PINCTRL_PIN(7, "IOD_ZLSCLK1"),
	PINCTRL_PIN(8, "IOD_ZLDATA1"),
	PINCTRL_PIN(9, "IOD_CLK27M"),
	PINCTRL_PIN(10, "IOD_CPURST"),
	PINCTRL_PIN(11, "IOD_PWORK"),
	PINCTRL_PIN(12, "IOD_RSMRST"),
	PINCTRL_PIN(13, "IOD_THRMTRIP"),
	//GPIO range 0
	PINCTRL_PIN(14, "USBHOC0"),
	PINCTRL_PIN(15, "USBHOC1"),
	PINCTRL_PIN(16, "USBHOC2"),
	PINCTRL_PIN(17, "USBHOC3"),
	PINCTRL_PIN(18, "USBHOC4"),
	PINCTRL_PIN(19, "USBHOC5"),
	PINCTRL_PIN(20, "USBHOC6"),
	PINCTRL_PIN(21, "USBHOC7"),
	//gpio range 1
	PINCTRL_PIN(22, "USB4SBTX0"),
	PINCTRL_PIN(23, "USB4SBRX0"),
	PINCTRL_PIN(24, "USB4SBTX1"),
	PINCTRL_PIN(25, "USB4SBRX1"),
	//gpio range 2
	PINCTRL_PIN(26, "I2C1DT"),
	PINCTRL_PIN(27, "I2C1CK"),
	PINCTRL_PIN(28, "I2C1INT"),
	//gpio range 3
	PINCTRL_PIN(29, "I2C2DT"),
	PINCTRL_PIN(30, "I2C2CK"),
	//gpio range 4
	PINCTRL_PIN(31, "I2C2INT"),
	//gpio range 5
	PINCTRL_PIN(32, "SMBDT1"),
	PINCTRL_PIN(33, "SMBCK1"),
	PINCTRL_PIN(34, "SMBDT2"),
	PINCTRL_PIN(35, "SMBCK2"),
	PINCTRL_PIN(36, "SMBALRT"),
	//gpio range 6
	PINCTRL_PIN(37, "SME_I2CDT"),
	PINCTRL_PIN(38, "SME_I2CCK"),
	//gpio range 7
	PINCTRL_PIN(39, "PWM"),
	PINCTRL_PIN(40, "TACH"),
	//gpio range 8
	PINCTRL_PIN(41, "GPIO0"),
	PINCTRL_PIN(42, "GPIO1"),
	PINCTRL_PIN(43, "GPIO2"),
	PINCTRL_PIN(44, "GPIO3"),
	PINCTRL_PIN(45, "GPIO4"),
	PINCTRL_PIN(46, "GPIO5"),
	PINCTRL_PIN(47, "GPIO6"),
	PINCTRL_PIN(48, "GPIO7"),
	PINCTRL_PIN(49, "GPIO8"),
	PINCTRL_PIN(50, "GPIO9"),
	PINCTRL_PIN(51, "LPCCLK"),
	PINCTRL_PIN(52, "LPCDRQ1"),
	//gpio range 9
	PINCTRL_PIN(53, "LPCDRQ0"),
	PINCTRL_PIN(54, "LPCFRAME"),
	PINCTRL_PIN(55, "LPCAD3"),
	PINCTRL_PIN(56, "LPCAD2"),
	PINCTRL_PIN(57, "LPCAD1"),
	PINCTRL_PIN(58, "LPCAD0"),
	//gpio range 10
	PINCTRL_PIN(59, "SERIRQ"),
	PINCTRL_PIN(60, "AZRST"),
	PINCTRL_PIN(61, "AZBITCLK"),
	PINCTRL_PIN(62, "AZSDIN0"),
	PINCTRL_PIN(63, "AZSDIN1"),
	PINCTRL_PIN(64, "AZSDOUT"),
	PINCTRL_PIN(65, "AZSYNC"),
	//gpio range 11
	PINCTRL_PIN(66, "I2S1_SCLK"),
	PINCTRL_PIN(67, "I2S1_TXD"),
	PINCTRL_PIN(68, "I2S1_WS"),
	PINCTRL_PIN(69, "I2S1_MCLK"),
	//gpio range 12
	PINCTRL_PIN(70, "I2S1_RXD"),
	//gpio range 13
	PINCTRL_PIN(71, "I2S1_INT"),
	PINCTRL_PIN(72, "MSPIDI"),
	PINCTRL_PIN(73, "MSPIDO"),
	PINCTRL_PIN(74, "MSPIIO2"),
	PINCTRL_PIN(75, "MSPIIO3"),
	PINCTRL_PIN(76, "MSPICLK"),
	PINCTRL_PIN(77, "MSPISS0"),
	//gpio range 14
	PINCTRL_PIN(78, "MSPISS1"),
	PINCTRL_PIN(79, "MSPISS2"),
	//gpio range 15
	PINCTRL_PIN(80, "SPIDEVINT"),
	PINCTRL_PIN(81, "BIOSSEL"),
	//gpio range 16
	PINCTRL_PIN(82, "THRM"),
	PINCTRL_PIN(83, "PEXWAKE"),
	PINCTRL_PIN(84, "PWRBTN"),
	//gpio range 17
	PINCTRL_PIN(85, "SPKR"),
	PINCTRL_PIN(86, "PME"),
	//gpio range 18
	PINCTRL_PIN(87, "BATLOW"),
	PINCTRL_PIN(88, "EXTSMI"),
	PINCTRL_PIN(89, "SUSA"),
	PINCTRL_PIN(90, "SUSB"),
	PINCTRL_PIN(91, "SUSC"),
	PINCTRL_PIN(92, "GPWAKE"),
	PINCTRL_PIN(93, "RING"),
	PINCTRL_PIN(94, "LID"),
	PINCTRL_PIN(95, "SLPS0"),
	PINCTRL_PIN(96, "PCIRST"),
	PINCTRL_PIN(97, "SVID_VREN"),
	//gpio range 19
	PINCTRL_PIN(98, "INTRUDER"),
	//gpio range 20
	PINCTRL_PIN(99, "GFX_I2CCLK0"),
	PINCTRL_PIN(100, "GFX_I2CDAT0"),
	PINCTRL_PIN(101, "GFX_I2CCLK1"),
	PINCTRL_PIN(102, "GFX_I2CDAT1"),
	PINCTRL_PIN(103, "GFX_I2CCLK2"),
	PINCTRL_PIN(104, "GFX_I2CDAT2"),
	PINCTRL_PIN(105, "GFX_I2CCLK3"),
	PINCTRL_PIN(106, "GFX_I2CDAT3"),
	PINCTRL_PIN(107, "GFX_GPIO0"),
	PINCTRL_PIN(108, "GFX_GPIO1"),
	PINCTRL_PIN(109, "GFX_GPIO2"),
	PINCTRL_PIN(110, "GFX_GPIO3"),
	PINCTRL_PIN(111, "CRTHSYNC"),
	PINCTRL_PIN(112, "CRTVSYNC"),
};

#define NOT_DEFINE	-30000

static int calibrate_int[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	63, 64, 65, 66, 67, 68,
	69, 70,
	18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
	34, 35, 36, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62
};

static int calibrate_sattus[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	63, 64, 65, 66, 67, 68,
	69, 70,
	18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
	34, 35, 36, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62
};

static const struct reg_cal_array kx7000_int_cal[] = {
	ZX_CAL_ARRAY(0x58, 16),
	ZX_CAL_ARRAY(0x5A, 2),
	ZX_CAL_ARRAY(0xDA, 16),
	ZX_CAL_ARRAY(0xDE, 16),
};

static const struct reg_calibrate int_cal[] = {
	{
		.reg = kx7000_int_cal,
		.reg_cal_size = ARRAY_SIZE(kx7000_int_cal),
		.cal_array = calibrate_int,
		.size = ARRAY_SIZE(calibrate_int),
	}
};

static const struct reg_cal_array kx7000_status_cal[] = {
	ZX_CAL_ARRAY((0x8), 16),
	ZX_CAL_ARRAY((0xE), 2),
	ZX_CAL_ARRAY((0xA), 16),
	ZX_CAL_ARRAY((0xC), 16),
};

static const struct reg_calibrate status_cal[] = {
	{
		.reg = kx7000_status_cal,
		.reg_cal_size = ARRAY_SIZE(kx7000_status_cal),
		.cal_array = calibrate_sattus,
		.size = ARRAY_SIZE(calibrate_sattus),
	}
};

static const struct reg_cal_array kx7000_mod_sel_cal[] = {
	ZX_CAL_ARRAY((0x0), 16),
	ZX_CAL_ARRAY((0x6), 2),
	ZX_CAL_ARRAY((0x2), 16),
	ZX_CAL_ARRAY((0x4), 16),
};

static const struct reg_calibrate mod_sel_cal[] = {
	{
		.reg = kx7000_mod_sel_cal,
		.reg_cal_size = ARRAY_SIZE(kx7000_mod_sel_cal),
		.cal_array = calibrate_sattus,
		.size = ARRAY_SIZE(calibrate_sattus),
	}
};

static const struct index_cal_array kx7000_gpio_in_cal[] = {
	ZX_CAL_INDEX_ARRAY(0x98, NULL, 71),
};

static const struct index_cal_array kx7000_gpio_out_cal[] = {
	ZX_CAL_INDEX_ARRAY(0x90, NULL, 71),
};

static int calibrate_trigger[] = {
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 18, 19,
	20, 21, 22, 23,
	24, 25, 26, 27,
	28, 29, 30, 31,
	32, 33, 34, 35,
	36, 50, 51, 52,
	53, 54, 55, 56,
	57, 58, 59, 60,
	61, 62, 63, 64,
	65, 66, 67, 68,
	69, 70
};

static const struct index_cal_array kx7000_trigger_cal[] = {
	ZX_CAL_INDEX_ARRAY(0xA0, calibrate_trigger, 50),
};

static const struct zhaoxin_pin_topology kx7000_pin_topologys[] = {
	{
		.int_cal = int_cal,
		.status_cal = status_cal,
		.mod_sel_cal = mod_sel_cal,
		.gpio_in_cal = kx7000_gpio_in_cal,
		.gpio_out_cal = kx7000_gpio_out_cal,
		.trigger_cal = kx7000_trigger_cal,
	}
};

#define KX7000_GPP(s, e, g) \
{ \
	.zhaoxin_range_pin_base = (s), \
	.zhaoxin_range_pin_size = ((e) - (s) + 1), \
	.zhaoxin_range_gpio_base = (g), \
}

static const struct zhaoxin_pin_map2_gpio kx7000_pinmap_gpps[] = {
	KX7000_GPP(0, 13, ZHAOXIN_GPIO_BASE_NOMAP),
	KX7000_GPP(14, 19, 10),
	KX7000_GPP(20, 21, ZHAOXIN_GPIO_BASE_NOMAP),
	KX7000_GPP(22, 25, 65),
	KX7000_GPP(26, 28, 43),
	KX7000_GPP(29, 30, 41),
	KX7000_GPP(31, 31, 49),
	KX7000_GPP(32, 36, 16),
	KX7000_GPP(37, 38, 69),
	KX7000_GPP(39, 40, 67),
	KX7000_GPP(41, 50, 0),
	KX7000_GPP(51, 52, ZHAOXIN_GPIO_BASE_NOMAP),
	KX7000_GPP(53, 53, 39),
	KX7000_GPP(54, 58, ZHAOXIN_GPIO_BASE_NOMAP),
	KX7000_GPP(59, 59, 40),
	KX7000_GPP(60, 65, ZHAOXIN_GPIO_BASE_NOMAP),
	KX7000_GPP(66, 69, 35),
	KX7000_GPP(70, 70, 46),
	KX7000_GPP(71, 71, 64),
	KX7000_GPP(72, 77, ZHAOXIN_GPIO_BASE_NOMAP),
	KX7000_GPP(78, 78, 50),
	KX7000_GPP(79, 79, ZHAOXIN_GPIO_BASE_NOMAP),
	KX7000_GPP(80, 80, 51),
	KX7000_GPP(81, 81, ZHAOXIN_GPIO_BASE_NOMAP),
	KX7000_GPP(82, 82, 52),
	KX7000_GPP(83, 84, ZHAOXIN_GPIO_BASE_NOMAP),
	KX7000_GPP(85, 85, 53),
	KX7000_GPP(86, 86, ZHAOXIN_GPIO_BASE_NOMAP),
	KX7000_GPP(87, 95, 54),
	KX7000_GPP(96, 97, ZHAOXIN_GPIO_BASE_NOMAP),
	KX7000_GPP(98, 98, 63),
	KX7000_GPP(99, 112, 21),
};

static const struct zhaoxin_pinctrl_soc_data kx7000_soc_data = {
	.pins = kx7000_pins,
	.npins = ARRAY_SIZE(kx7000_pins),
	.pin_topologys = kx7000_pin_topologys,
	.zhaoxin_pin_maps = kx7000_pinmap_gpps,
	.pin_map_size = ARRAY_SIZE(kx7000_pinmap_gpps),
};

static const struct acpi_device_id kx7000_pinctrl_acpi_match[] = {
	{ "KX8344B", (kernel_ulong_t)&kx7000_soc_data },
	{ }
};
MODULE_DEVICE_TABLE(acpi, kx7000_pinctrl_acpi_match);

static const struct dev_pm_ops kx7000_pinctrl_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(zhaoxin_pinctrl_suspend_noirq, zhaoxin_pinctrl_resume_noirq)
};

static struct platform_driver kx7000_pinctrl_driver = {
	.probe = zhaoxin_pinctrl_probe_by_hid,
	.driver = {
		.name = "kx7000-pinctrl",
		.acpi_match_table = kx7000_pinctrl_acpi_match,
		.pm = &kx7000_pinctrl_pm_ops,
	},
};

module_platform_driver(kx7000_pinctrl_driver);

MODULE_AUTHOR("www.zhaoxin.com");
MODULE_DESCRIPTION("Shanghai Zhaoxin pinctrl driver");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");
