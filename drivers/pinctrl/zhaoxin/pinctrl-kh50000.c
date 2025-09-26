// SPDX-License-Identifier: GPL-2.0
/*
 * zhaoxin KH50000 pinctrl/GPIO driver
 *
 *
 *    Copyright(c) 2021 Shanghai Zhaoxin Corporation. All rights reserved.
 *
 */

#define DRIVER_VERSION "1.0.0"

#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-zhaoxin.h"

/* kh50000 pin define */
static const struct pinctrl_pin_desc kh50000_pins[] = {
	PINCTRL_PIN(0, "IOD_CLK27M_G0"),
	PINCTRL_PIN(1, "IOD_CLK27M_G1"),
	PINCTRL_PIN(2, "IOD_CLK27M_G2"),
	PINCTRL_PIN(3, "IOD_CLK27M_G3"),
	PINCTRL_PIN(4, "IOD_CPURST_G0"),
	PINCTRL_PIN(5, "IOD_CPURST_G1"),
	PINCTRL_PIN(6, "IOD_CPURST_G2"),
	PINCTRL_PIN(7, "IOD_CPURST_G3"),
	PINCTRL_PIN(8, "IOD_RSMRST_G0"),
	PINCTRL_PIN(9, "IOD_RSMRST_G1"),
	PINCTRL_PIN(10, "IOD_RSMRST_G2"),
	PINCTRL_PIN(11, "IOD_RSMRST_G3"),
	PINCTRL_PIN(12, "IOD_PWROK_G0"),
	PINCTRL_PIN(13, "IOD_PWROK_G1"),
	PINCTRL_PIN(14, "IOD_PWROK_G2"),
	PINCTRL_PIN(15, "IOD_PWROK_G3"),
	PINCTRL_PIN(16, "IOD_THRMTRIP_G0"),
	PINCTRL_PIN(17, "IOD_THRMTRIP_G1"),
	PINCTRL_PIN(18, "IOD_THRMTRIP_G2"),
	PINCTRL_PIN(19, "IOD_THRMTRIP_G3"),
	PINCTRL_PIN(20, "IOD_CLK50M_G0"),
	PINCTRL_PIN(21, "IOD_CLK50M_G1"),
	PINCTRL_PIN(22, "IOD_CLK50M_G2"),
	PINCTRL_PIN(23, "IOD_CLK50M_G3"),
	/* GPIO range 0 */
	PINCTRL_PIN(24, "USBHOC0"),
	PINCTRL_PIN(25, "USBHOC1"),
	PINCTRL_PIN(26, "USBHOC2"),
	PINCTRL_PIN(27, "USBHOC3"),
	PINCTRL_PIN(28, "I3C0DT"),
	PINCTRL_PIN(29, "I3C0CK"),
	PINCTRL_PIN(30, "I3C1DT"),
	PINCTRL_PIN(31, "I3C1CK"),
	PINCTRL_PIN(32, "I3C2DT"),
	PINCTRL_PIN(33, "I3C2CK"),
	PINCTRL_PIN(34, "I3C3DT"),
	PINCTRL_PIN(35, "I3C3CK"),
	PINCTRL_PIN(36, "SMBDT0"),
	/* GPIO range 1 */
	PINCTRL_PIN(37, "SMBCK0"),
	PINCTRL_PIN(38, "SMBDT1"),
	PINCTRL_PIN(39, "SMBCK1"),
	PINCTRL_PIN(40, "SMBDT2"),
	PINCTRL_PIN(41, "SMBCK2"),
	PINCTRL_PIN(42, "SMBALRT"),
	PINCTRL_PIN(43, "SME_I2CDT_S"),
	PINCTRL_PIN(44, "SME_I2CCK_S"),
	/* GPIO range 2 */
	PINCTRL_PIN(45, "GPIO0"),
	PINCTRL_PIN(46, "GPIO1"),
	PINCTRL_PIN(47, "GPIO2"),
	PINCTRL_PIN(48, "GPIO3"),
	PINCTRL_PIN(49, "GPIO4"),
	PINCTRL_PIN(50, "GPIO5"),
	PINCTRL_PIN(51, "GPIO6"),
	PINCTRL_PIN(52, "GPIO7"),
	PINCTRL_PIN(53, "GPIO8"),
	PINCTRL_PIN(54, "GPIO9"),
	PINCTRL_PIN(55, "GPIO10"),
	PINCTRL_PIN(56, "GPIO11"),
	PINCTRL_PIN(57, "GPIO12"),
	PINCTRL_PIN(58, "GPIO13"),
	PINCTRL_PIN(59, "GPIO14"),
	PINCTRL_PIN(60, "GPIO15"),
	PINCTRL_PIN(61, "GPIO16"),
	PINCTRL_PIN(62, "GPIO17"),
	PINCTRL_PIN(63, "GPIO18"),
	PINCTRL_PIN(64, "GPIO19"),
	PINCTRL_PIN(65, "GPIO20"),
	PINCTRL_PIN(66, "GPIO21"),
	PINCTRL_PIN(67, "GPIO22"),
	PINCTRL_PIN(68, "GPIO23"),
	PINCTRL_PIN(69, "GPIO24"),
	PINCTRL_PIN(70, "GPIO25"),
	PINCTRL_PIN(71, "GPIO26"),
	PINCTRL_PIN(72, "GPIO27"),
	PINCTRL_PIN(73, "GPIO28"),
	PINCTRL_PIN(74, "GPIO29"),
	PINCTRL_PIN(75, "GPIO30"),
	PINCTRL_PIN(76, "GPIO31"),
	PINCTRL_PIN(77, "GPIO32"),
	PINCTRL_PIN(78, "GPIO33"),
	PINCTRL_PIN(79, "GPIO34"),
	PINCTRL_PIN(80, "GPIO35"),
	/* GPIO range 3 */
	PINCTRL_PIN(81, "LPCCLK"),
	PINCTRL_PIN(82, "LPCDRQ1"),
	PINCTRL_PIN(83, "LPCDRQ0"),
	PINCTRL_PIN(84, "LPCFRAME"),
	PINCTRL_PIN(85, "LPCAD3"),
	PINCTRL_PIN(86, "LPCAD2"),
	PINCTRL_PIN(87, "LPCAD1"),
	PINCTRL_PIN(88, "LPCAD0"),
	PINCTRL_PIN(89, "SERIRQ"),
	/* GPIO range 4 */
	PINCTRL_PIN(90, "ESPICLK"),
	/* GPIO range 5 */
	PINCTRL_PIN(91, "ESPIRST"),
	PINCTRL_PIN(92, "ESPICS"),
	PINCTRL_PIN(93, "ESPIIO3"),
	/* GPIO range 6 */
	PINCTRL_PIN(94, "ESPIIO2"),
	PINCTRL_PIN(95, "ESPIIO1"),
	PINCTRL_PIN(96, "ESPIIO0"),
	PINCTRL_PIN(97, "SPIDI"),
	PINCTRL_PIN(98, "SPIDO"),
	PINCTRL_PIN(99, "SPICLK"),
	PINCTRL_PIN(100, "SPISS"),
	PINCTRL_PIN(101, "TPMRST"),
	PINCTRL_PIN(102, "TPMIRQ"),
	PINCTRL_PIN(103, "MSPIDI"),
	PINCTRL_PIN(104, "MSPIDO"),
	PINCTRL_PIN(105, "MSPIIO2"),
	PINCTRL_PIN(106, "MSPIIO3"),
	PINCTRL_PIN(107, "MSPICLK"),
	PINCTRL_PIN(108, "MSPISS0"),
	/* GPIO range 7 */
	PINCTRL_PIN(109, "MSPISS1"),
	/* GPIO range 8 */
	PINCTRL_PIN(110, "MSPISS2"),
	/* GPIO range 9 */
	PINCTRL_PIN(111, "SPIDEVINT"),
	PINCTRL_PIN(112, "ZLSDATA_TX_P0"),
	PINCTRL_PIN(113, "ZLSDATA_RX_P0"),
	PINCTRL_PIN(114, "ZLSDATA_TX_P1"),
	PINCTRL_PIN(115, "ZLSDATA_RX_P1"),
	PINCTRL_PIN(116, "ZLSDATA_TX_P2"),
	PINCTRL_PIN(117, "ZLSDATA_RX_P2"),
	PINCTRL_PIN(118, "BOOT_EN"),
	PINCTRL_PIN(119, "BOOT_DONE"),
	PINCTRL_PIN(120, "MST_SKT"),
	PINCTRL_PIN(121, "HRX_BEVO_CLK"),
	PINCTRL_PIN(122, "HRX_BEVO_DATA"),
	PINCTRL_PIN(123, "HTX_BEVO_CLK"),
	PINCTRL_PIN(124, "HTX_BEVO_DATA"),
	PINCTRL_PIN(125, "THRMTRIP_I"),
	PINCTRL_PIN(126, "CLK50M_I"),
	PINCTRL_PIN(127, "CLK50M_O"),
	PINCTRL_PIN(128, "PCIRST_IO"),
	PINCTRL_PIN(129, "RSMRST_IO"),
	PINCTRL_PIN(130, "PWRGD_IO"),
	PINCTRL_PIN(131, "CLK32K_IO"),
	PINCTRL_PIN(132, "BIOSSEL"),
	PINCTRL_PIN(133, "THRMRIP"),
	/* GPIO range 10 */
	PINCTRL_PIN(134, "THRM"),
	/* GPIO range 11 */
	PINCTRL_PIN(135, "PEXWAKE"),
	PINCTRL_PIN(136, "PWRBTN"),
	PINCTRL_PIN(137, "PCIRST"),
	/* GPIO range 12 */
	PINCTRL_PIN(138, "SPKR"),
	PINCTRL_PIN(139, "PME"),
	PINCTRL_PIN(140, "SUSA"),
	PINCTRL_PIN(141, "SUSB"),
	PINCTRL_PIN(142, "SUSC"),
	PINCTRL_PIN(143, "SVID0_VREN"),
	PINCTRL_PIN(144, "SVID1_VREN"),
};

#define NOT_DEFINE -30000

static int calibrate_int[] = {
	0,  1,	2,  3,	4,  5,	6,  7,	8,  9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
	50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
	60, 61, 62, 63, 64, 65, 66, 67,
};

static int calibrate_status[] = {
	0,  1,	2,  3,	4,  5,	6,  7,	8,  9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
	50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
	60, 61, 62, 63, 64, 65, 66, 67,
};

static const struct reg_cal_array kh50000_int_cal[] = {
	ZX_CAL_ARRAY((0xCC - 0xCC), 16), /* GPIO0-15 */
	ZX_CAL_ARRAY((0xCE - 0xCC), 16), /* GPIO15-31 */
	ZX_CAL_ARRAY((0xD4 - 0xCC), 4), /* GPIO32-35 */
	ZX_CAL_ARRAY((0xD0 - 0xCC), 16), /* PGPIO0-PGPIO15 */
	ZX_CAL_ARRAY((0xD2 - 0xCC), 16), /* PGPIO16-PGPIO31 */
};

static const struct reg_calibrate int_cal[] = {
	{
		.reg = kh50000_int_cal,
		.reg_cal_size = ARRAY_SIZE(kh50000_int_cal),
		.cal_array = calibrate_int,
		.size = ARRAY_SIZE(calibrate_int),
		.is_pmio = false,
	}
};

static const struct reg_cal_array kh50000_status_cal[] = {
	ZX_CAL_ARRAY((0xE4 - 0xCC), 16),
	ZX_CAL_ARRAY((0xE6 - 0xCC), 16),
	ZX_CAL_ARRAY((0xEC - 0xCC), 4),
	ZX_CAL_ARRAY((0xE8 - 0xCC), 16),
	ZX_CAL_ARRAY((0xEA - 0xCC), 16),
};

static const struct reg_calibrate status_cal[] = { {
	.reg = kh50000_status_cal,
	.reg_cal_size = ARRAY_SIZE(kh50000_status_cal),
	.cal_array = calibrate_status,
	.size = ARRAY_SIZE(calibrate_status),
} };

static const struct reg_cal_array kh50000_mod_sel_cal[] = {
	ZX_CAL_ARRAY((0xD8 - 0xCC), 16),
	ZX_CAL_ARRAY((0xDA - 0xCC), 16),
	ZX_CAL_ARRAY((0xE0 - 0xCC), 4),
	ZX_CAL_ARRAY((0xDC - 0xCC), 16),
	ZX_CAL_ARRAY((0xDE - 0xCC), 16),
};

static const struct reg_calibrate mod_sel_cal[] = {
	{
		.reg = kh50000_mod_sel_cal,
		.reg_cal_size = ARRAY_SIZE(kh50000_mod_sel_cal),
		.cal_array = calibrate_status,
		.size = ARRAY_SIZE(calibrate_status),
	}
};

static const struct index_cal_array kh50000_gpio_in_cal[] = {
	ZX_CAL_INDEX_ARRAY(0xC8, NULL, 68),
};

static const struct index_cal_array kh50000_gpio_out_cal[] = {
	ZX_CAL_INDEX_ARRAY(0xC0, NULL, 68),
};

static int calibrate_trigger[] = {
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
	50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
	60, 61, 62, 63, 64, 65, 66, 67
};

static const struct index_cal_array kh50000_trigger_cal[] = {
	ZX_CAL_INDEX_ARRAY_MASK(0xD0, calibrate_trigger, 68, 3, 0x7),
};

static const struct zhaoxin_pin_topology kh50000_pin_topologys[] = { {
	.int_cal = int_cal,
	.status_cal = status_cal,
	.mod_sel_cal = mod_sel_cal,
	.gpio_in_cal = kh50000_gpio_in_cal,
	.gpio_out_cal = kh50000_gpio_out_cal,
	.trigger_cal = kh50000_trigger_cal,
} };

static const struct zhaoxin_pin_map2_gpio kh50000_pinmap_gpps[] = {
	ZHAOXIN_GPP(0, 23, ZHAOXIN_GPIO_BASE_NOMAP), /* no range */
	ZHAOXIN_GPP(24, 27, 10), /* gpio range 0 */
	ZHAOXIN_GPP(28, 36, ZHAOXIN_GPIO_BASE_NOMAP), /* no range */
	ZHAOXIN_GPP(37, 42, 47), /* gpio range 1 */
	ZHAOXIN_GPP(43, 44, ZHAOXIN_GPIO_BASE_NOMAP), /* no range */
	ZHAOXIN_GPP(45, 80, 0), /* gpio range 2 */
	ZHAOXIN_GPP(81, 89, 52), /* gpio range 3 */
	ZHAOXIN_GPP(90, 90, 51), /* gpio range 4 */
	ZHAOXIN_GPP(91, 93, 65), /* gpio range 5 */
	ZHAOXIN_GPP(94, 96, 40), /* gpio range 6 */
	ZHAOXIN_GPP(97, 108, ZHAOXIN_GPIO_BASE_NOMAP), /* no range */
	ZHAOXIN_GPP(109, 109, 45), /* gpio range 7 */
	ZHAOXIN_GPP(110, 110, 58), /* gpio range 8 */
	ZHAOXIN_GPP(111, 111, 61), /* gpio range 9 */
	ZHAOXIN_GPP(112, 133, ZHAOXIN_GPIO_BASE_NOMAP), /* no range */
	ZHAOXIN_GPP(134, 134, 62), /* gpio range 10 */
	ZHAOXIN_GPP(135, 135, 46), /* gpio range 11 */
	ZHAOXIN_GPP(136, 137, ZHAOXIN_GPIO_BASE_NOMAP), /* no range */
	ZHAOXIN_GPP(138, 139, 63), /* gpio range 12 */
	ZHAOXIN_GPP(140, 144, ZHAOXIN_GPIO_BASE_NOMAP), /* no range */
};

static zx_gpio_type kh50000_gpio_type(struct zhaoxin_pinctrl *pctrl, unsigned int pin)
{
	if (pin >= 24 && pin <= 27)
		return ZX_TYPE_PGPIO;
	else if (pin >= 37 && pin <= 42)
		return ZX_TYPE_PGPIO;
	else if (pin >= 45 && pin <= 80)
		return ZX_TYPE_GPIO;
	else if (pin >= 81 && pin <= 96)
		return ZX_TYPE_PGPIO;
	else if (pin >= 109 && pin <= 111)
		return ZX_TYPE_PGPIO;
	else if (pin >= 134 && pin <= 135)
		return ZX_TYPE_PGPIO;
	else if (pin >= 138 && pin <= 139)
		return ZX_TYPE_PGPIO;
	else
		return ZX_TYPE_ERROR;
}

static void kh50000_gpio_init(struct zhaoxin_pinctrl *pctrl)
{
	pctrl->pmio_base = 0x800;
	pctrl->pmio_rx90 = 0x90;
	pctrl->pmio_rx8c = 0x8c;

	zx_pad_write16(pctrl, 0xF8, 0x7F);
	dev_info(pctrl->dev, "KH50000 private init\n");
}

static const struct zhaoxin_pinctrl_soc_data kh50000_soc_data = {
	.pins = kh50000_pins,
	.npins = ARRAY_SIZE(kh50000_pins),
	.pin_topologys = kh50000_pin_topologys,
	.gpio_type = kh50000_gpio_type,
	.private_init = kh50000_gpio_init,
	.zhaoxin_pin_maps = kh50000_pinmap_gpps,
	.pin_map_size = ARRAY_SIZE(kh50000_pinmap_gpps),
};

static const struct acpi_device_id kh50000_pinctrl_acpi_match[] = {
	{ "KH8344B", (kernel_ulong_t)&kh50000_soc_data },
	{}
};
MODULE_DEVICE_TABLE(acpi, kh50000_pinctrl_acpi_match);

static const struct dev_pm_ops kh50000_pinctrl_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(zhaoxin_pinctrl_suspend_noirq, zhaoxin_pinctrl_resume_noirq)
};

static struct platform_driver kh50000_pinctrl_driver = {
	.probe = zhaoxin_pinctrl_probe_by_hid,
	.driver = {
		.name = "kh50000-pinctrl",
		.acpi_match_table = kh50000_pinctrl_acpi_match,
		.pm = &kh50000_pinctrl_pm_ops,
	},
};

module_platform_driver(kh50000_pinctrl_driver);

MODULE_AUTHOR("www.zhaoxin.com");
MODULE_DESCRIPTION("Shanghai Zhaoxin pinctrl driver");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
