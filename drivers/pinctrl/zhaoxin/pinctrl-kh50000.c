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

#define KH50000_SOCKET_PINS(sock)						\
	SOCKET_PINCTRL_PIN(sock, 0, "IOD_CLK27M_G0"),				\
	SOCKET_PINCTRL_PIN(sock, 1, "IOD_CLK27M_G1"),				\
	SOCKET_PINCTRL_PIN(sock, 2, "IOD_CLK27M_G2"),				\
	SOCKET_PINCTRL_PIN(sock, 3, "IOD_CLK27M_G3"),				\
	SOCKET_PINCTRL_PIN(sock, 4, "IOD_CPURST_G0"),				\
	SOCKET_PINCTRL_PIN(sock, 5, "IOD_CPURST_G1"),				\
	SOCKET_PINCTRL_PIN(sock, 6, "IOD_CPURST_G2"),				\
	SOCKET_PINCTRL_PIN(sock, 7, "IOD_CPURST_G3"),				\
	SOCKET_PINCTRL_PIN(sock, 8, "IOD_RSMRST_G0"),				\
	SOCKET_PINCTRL_PIN(sock, 9, "IOD_RSMRST_G1"),				\
	SOCKET_PINCTRL_PIN(sock, 10, "IOD_RSMRST_G2"),				\
	SOCKET_PINCTRL_PIN(sock, 11, "IOD_RSMRST_G3"),				\
	SOCKET_PINCTRL_PIN(sock, 12, "IOD_PWROK_G0"),				\
	SOCKET_PINCTRL_PIN(sock, 13, "IOD_PWROK_G1"),				\
	SOCKET_PINCTRL_PIN(sock, 14, "IOD_PWROK_G2"),				\
	SOCKET_PINCTRL_PIN(sock, 15, "IOD_PWROK_G3"),				\
	SOCKET_PINCTRL_PIN(sock, 16, "IOD_THRMTRIP_G0"),			\
	SOCKET_PINCTRL_PIN(sock, 17, "IOD_THRMTRIP_G1"),			\
	SOCKET_PINCTRL_PIN(sock, 18, "IOD_THRMTRIP_G2"),			\
	SOCKET_PINCTRL_PIN(sock, 19, "IOD_THRMTRIP_G3"),			\
	SOCKET_PINCTRL_PIN(sock, 20, "IOD_CLK50M_G0"),				\
	SOCKET_PINCTRL_PIN(sock, 21, "IOD_CLK50M_G1"),				\
	SOCKET_PINCTRL_PIN(sock, 22, "IOD_CLK50M_G2"),				\
	SOCKET_PINCTRL_PIN(sock, 23, "IOD_CLK50M_G3"),				\
	/*GPIO range 0 */							\
	SOCKET_PINCTRL_PIN(sock, 24, "USBHOC0"),	/*PGPIO0------gpio36*/	\
	SOCKET_PINCTRL_PIN(sock, 25, "USBHOC1"),	/*PGPIO1------gpio37*/	\
	SOCKET_PINCTRL_PIN(sock, 26, "USBHOC2"),	/*PGPIO2------gpio38*/	\
	SOCKET_PINCTRL_PIN(sock, 27, "USBHOC3"),	/*PGPIO3------gpio39*/	\
	SOCKET_PINCTRL_PIN(sock, 28, "I3C0DT"),					\
	SOCKET_PINCTRL_PIN(sock, 29, "I3C0CK"),					\
	SOCKET_PINCTRL_PIN(sock, 30, "I3C1DT"),					\
	SOCKET_PINCTRL_PIN(sock, 31, "I3C1CK"),					\
	SOCKET_PINCTRL_PIN(sock, 32, "I3C2DT"),					\
	SOCKET_PINCTRL_PIN(sock, 33, "I3C2CK"),					\
	SOCKET_PINCTRL_PIN(sock, 34, "I3C3DT"),					\
	SOCKET_PINCTRL_PIN(sock, 35, "I3C3CK"),					\
	SOCKET_PINCTRL_PIN(sock, 36, "SMBDT0"),					\
	/*GPIO range 1*/							\
	SOCKET_PINCTRL_PIN(sock, 37, "SMBCK0"),		/*PGPIO11------gpio47*/	\
	SOCKET_PINCTRL_PIN(sock, 38, "SMBDT1"),		/*PGPIO12------gpio48*/	\
	SOCKET_PINCTRL_PIN(sock, 39, "SMBCK1"),		/*PGPIO13------gpio49*/	\
	SOCKET_PINCTRL_PIN(sock, 40, "SMBDT2"),		/*PGPIO7------gpio43*/	\
	SOCKET_PINCTRL_PIN(sock, 41, "SMBCK2"),		/*PGPIO8------gpio44*/	\
	SOCKET_PINCTRL_PIN(sock, 42, "SMBALRT"),	/*PGPIO14------gpio50*/	\
	SOCKET_PINCTRL_PIN(sock, 43, "SME_I2CDT_S"),				\
	SOCKET_PINCTRL_PIN(sock, 44, "SME_I2CCK_S"),				\
	/*GPIO range 2*/							\
	SOCKET_PINCTRL_PIN(sock, 45, "GPIO0"),		/*GPIO0--------gpio0*/	\
	SOCKET_PINCTRL_PIN(sock, 46, "GPIO1"),		/*GPIO1--------gpio1*/	\
	SOCKET_PINCTRL_PIN(sock, 47, "GPIO2"),		/*GPIO2--------gpio2*/	\
	SOCKET_PINCTRL_PIN(sock, 48, "GPIO3"),		/*GPIO3--------gpio3*/	\
	SOCKET_PINCTRL_PIN(sock, 49, "GPIO4"),		/*GPIO4--------gpio4*/	\
	SOCKET_PINCTRL_PIN(sock, 50, "GPIO5"),		/*GPIO5--------gpio5*/	\
	SOCKET_PINCTRL_PIN(sock, 51, "GPIO6"),		/*GPIO6--------gpio6*/	\
	SOCKET_PINCTRL_PIN(sock, 52, "GPIO7"),		/*GPIO7--------gpio7*/	\
	SOCKET_PINCTRL_PIN(sock, 53, "GPIO8"),		/*GPIO8--------gpio8*/	\
	SOCKET_PINCTRL_PIN(sock, 54, "GPIO9"),		/*GPIO9--------gpio9*/	\
	SOCKET_PINCTRL_PIN(sock, 55, "GPIO10"),		/*GPIO10-------gpio10*/	\
	SOCKET_PINCTRL_PIN(sock, 56, "GPIO11"),		/*GPIO11-------gpio11*/	\
	SOCKET_PINCTRL_PIN(sock, 57, "GPIO12"),		/*GPIO12-------gpio12*/	\
	SOCKET_PINCTRL_PIN(sock, 58, "GPIO13"),		/*GPIO13-------gpio13*/	\
	SOCKET_PINCTRL_PIN(sock, 59, "GPIO14"),		/*GPIO14-------gpio14*/	\
	SOCKET_PINCTRL_PIN(sock, 60, "GPIO15"),		/*GPIO15-------gpio15*/	\
	SOCKET_PINCTRL_PIN(sock, 61, "GPIO16"),		/*GPIO16-------gpio16*/	\
	SOCKET_PINCTRL_PIN(sock, 62, "GPIO17"),		/*GPIO17-------gpio17*/	\
	SOCKET_PINCTRL_PIN(sock, 63, "GPIO18"),		/*GPIO18-------gpio18*/	\
	SOCKET_PINCTRL_PIN(sock, 64, "GPIO19"),		/*GPIO19-------gpio19*/	\
	SOCKET_PINCTRL_PIN(sock, 65, "GPIO20"),		/*GPIO20-------gpio20*/	\
	SOCKET_PINCTRL_PIN(sock, 66, "GPIO21"),		/*GPIO21-------gpio21*/	\
	SOCKET_PINCTRL_PIN(sock, 67, "GPIO22"),		/*GPIO22-------gpio22*/	\
	SOCKET_PINCTRL_PIN(sock, 68, "GPIO23"),		/*GPIO23-------gpio23*/	\
	SOCKET_PINCTRL_PIN(sock, 69, "GPIO24"),		/*GPIO24-------gpio24*/	\
	SOCKET_PINCTRL_PIN(sock, 70, "GPIO25"),		/*GPIO25-------gpio25*/	\
	SOCKET_PINCTRL_PIN(sock, 71, "GPIO26"),		/*GPIO26-------gpio26*/	\
	SOCKET_PINCTRL_PIN(sock, 72, "GPIO27"),		/*GPIO27-------gpio27*/	\
	SOCKET_PINCTRL_PIN(sock, 73, "GPIO28"),		/*GPIO28-------gpio28*/	\
	SOCKET_PINCTRL_PIN(sock, 74, "GPIO29"),		/*GPIO29-------gpio29*/	\
	SOCKET_PINCTRL_PIN(sock, 75, "GPIO30"),		/*GPIO30-------gpio30*/	\
	SOCKET_PINCTRL_PIN(sock, 76, "GPIO31"),		/*GPIO31-------gpio31*/	\
	SOCKET_PINCTRL_PIN(sock, 77, "GPIO32"),		/*GPIO32-------gpio32*/	\
	SOCKET_PINCTRL_PIN(sock, 78, "GPIO33"),		/*GPIO33-------gpio33*/	\
	SOCKET_PINCTRL_PIN(sock, 79, "GPIO34"),		/*GPIO34-------gpio34*/	\
	SOCKET_PINCTRL_PIN(sock, 80, "GPIO35"),		/*GPIO35-------gpio35*/	\
	/*GPIO range 3*/							\
	SOCKET_PINCTRL_PIN(sock, 81, "LPCCLK"),		/*PGPIO16------gpio52*/	\
	SOCKET_PINCTRL_PIN(sock, 82, "LPCDRQ1"),	/*PGPIO17------gpio53*/	\
	SOCKET_PINCTRL_PIN(sock, 83, "LPCDRQ0"),	/*PGPIO18------gpio54*/	\
	SOCKET_PINCTRL_PIN(sock, 84, "LPCFRAME"),	/*PGPIO19------gpio55*/	\
	SOCKET_PINCTRL_PIN(sock, 85, "LPCAD3"),		/*PGPIO20------gpio56*/	\
	SOCKET_PINCTRL_PIN(sock, 86, "LPCAD2"),		/*PGPIO21------gpio57*/	\
	SOCKET_PINCTRL_PIN(sock, 87, "LPCAD1"),		/*PGPIO22------gpio58*/	\
	SOCKET_PINCTRL_PIN(sock, 88, "LPCAD0"),		/*PGPIO23------gpio59*/	\
	SOCKET_PINCTRL_PIN(sock, 89, "SERIRQ"),		/*PGPIO24------gpio60*/	\
	/*GPIO range 4*/							\
	SOCKET_PINCTRL_PIN(sock, 90, "ESPICLK"),	/*PGPIO15------gpio51*/	\
	/*GPIO range 5*/							\
	SOCKET_PINCTRL_PIN(sock, 91, "ESPIRST"),	/*PGPIO29------gpio65*/	\
	SOCKET_PINCTRL_PIN(sock, 92, "ESPICS"),		/*PGPIO30------gpio66*/	\
	SOCKET_PINCTRL_PIN(sock, 93, "ESPIIO3"),	/*PGPIO31------gpio67*/	\
	/*GPIO range 6*/							\
	SOCKET_PINCTRL_PIN(sock, 94, "ESPIIO2"),	/*PGPIO4------gpio40*/	\
	SOCKET_PINCTRL_PIN(sock, 95, "ESPIIO1"),	/*PGPIO5------gpio41*/	\
	SOCKET_PINCTRL_PIN(sock, 96, "ESPIIO0"),	/*PGPIO6------gpio42*/	\
	/* jump */								\
	SOCKET_PINCTRL_PIN(sock, 97, "SPIDI"),					\
	SOCKET_PINCTRL_PIN(sock, 98, "SPIDO"),					\
	SOCKET_PINCTRL_PIN(sock, 99, "SPICLK"),					\
	SOCKET_PINCTRL_PIN(sock, 100, "SPISS"),					\
	SOCKET_PINCTRL_PIN(sock, 101, "TPMRST"),				\
	SOCKET_PINCTRL_PIN(sock, 102, "TPMIRQ"),				\
	SOCKET_PINCTRL_PIN(sock, 103, "MSPIDI"),				\
	SOCKET_PINCTRL_PIN(sock, 104, "MSPIDO"),				\
	SOCKET_PINCTRL_PIN(sock, 105, "MSPIIO2"),				\
	SOCKET_PINCTRL_PIN(sock, 106, "MSPIIO3"),				\
	SOCKET_PINCTRL_PIN(sock, 107, "MSPICLK"),				\
	SOCKET_PINCTRL_PIN(sock, 108, "MSPISS0"),				\
	/*GPIO range 7*/							\
	SOCKET_PINCTRL_PIN(sock, 109, "MSPISS1"),	/*PGPIO9------gpio45*/	\
	/*GPIO range 8 */							\
	SOCKET_PINCTRL_PIN(sock, 110, "MSPISS2"),	/*PGPIO22------gpio58*/	\
	/*GPIO range 9*/							\
	SOCKET_PINCTRL_PIN(sock, 111, "SPIDEVINT"),	/*PGPIO25------gpio61*/	\
	/*jump*/								\
	SOCKET_PINCTRL_PIN(sock, 112, "ZLSDATA_TX_P0"),				\
	SOCKET_PINCTRL_PIN(sock, 113, "ZLSDATA_RX_P0"),				\
	SOCKET_PINCTRL_PIN(sock, 114, "ZLSDATA_TX_P1"),				\
	SOCKET_PINCTRL_PIN(sock, 115, "ZLSDATA_RX_P1"),				\
	SOCKET_PINCTRL_PIN(sock, 116, "ZLSDATA_TX_P2"),				\
	SOCKET_PINCTRL_PIN(sock, 117, "ZLSDATA_RX_P2"),				\
	SOCKET_PINCTRL_PIN(sock, 118, "BOOT_EN"),				\
	SOCKET_PINCTRL_PIN(sock, 119, "BOOT_DONE"),				\
	SOCKET_PINCTRL_PIN(sock, 120, "MST_SKT"),				\
	SOCKET_PINCTRL_PIN(sock, 121, "HRX_BEVO_CLK"),				\
	SOCKET_PINCTRL_PIN(sock, 122, "HRX_BEVO_DATA"),				\
	SOCKET_PINCTRL_PIN(sock, 123, "HTX_BEVO_CLK"),				\
	SOCKET_PINCTRL_PIN(sock, 124, "HTX_BEVO_DATA"),				\
	SOCKET_PINCTRL_PIN(sock, 125, "THRMTRIP_I"),				\
	SOCKET_PINCTRL_PIN(sock, 126, "CLK50M_I"),				\
	SOCKET_PINCTRL_PIN(sock, 127, "CLK50M_O"),				\
	SOCKET_PINCTRL_PIN(sock, 128, "PCIRST_IO"),				\
	SOCKET_PINCTRL_PIN(sock, 129, "RSMRST_IO"),				\
	SOCKET_PINCTRL_PIN(sock, 130, "PWRGD_IO"),				\
	SOCKET_PINCTRL_PIN(sock, 131, "CLK32K_IO"),				\
	SOCKET_PINCTRL_PIN(sock, 132, "BIOSSEL"),				\
	SOCKET_PINCTRL_PIN(sock, 133, "THRMRIP"),				\
	/*GPIO range 10 */							\
	SOCKET_PINCTRL_PIN(sock, 134, "THRM"),		/*PGPIO26------gpio62*/	\
	/*GPIO range 11*/							\
	SOCKET_PINCTRL_PIN(sock, 135, "PEXWAKE"),	/*PGPIO10------gpio46*/	\
	/*jump*/								\
	SOCKET_PINCTRL_PIN(sock, 136, "PWRBTN"),				\
	SOCKET_PINCTRL_PIN(sock, 137, "PCIRST"),				\
	/*GPIO range 12*/							\
	SOCKET_PINCTRL_PIN(sock, 138, "SPKR"),		/*PGPIO27------gpio63*/	\
	SOCKET_PINCTRL_PIN(sock, 139, "PME"),		/*PGPIO28------gpio64*/	\
	SOCKET_PINCTRL_PIN(sock, 140, "SUSA"),					\
	SOCKET_PINCTRL_PIN(sock, 141, "SUSB"),					\
	SOCKET_PINCTRL_PIN(sock, 142, "SUSC"),					\
	SOCKET_PINCTRL_PIN(sock, 143, "SVID0_VREN"),				\
	SOCKET_PINCTRL_PIN(sock, 144, "SVID1_VREN"),

/* kh50000 pin define */
static const struct pinctrl_pin_desc kh50000_pins_0[] = {
	KH50000_SOCKET_PINS(0)
};

static const struct pinctrl_pin_desc kh50000_pins_1[] = {
	KH50000_SOCKET_PINS(1)
};

static const struct pinctrl_pin_desc kh50000_pins_2[] = {
	KH50000_SOCKET_PINS(2)
};

static const struct pinctrl_pin_desc kh50000_pins_3[] = {
	KH50000_SOCKET_PINS(3)
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
	struct resource *res_pmio;
	struct platform_device *pdev = to_platform_device(pctrl->dev);

	res_pmio = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (!res_pmio) {
		dev_err(&pdev->dev, "can't fetch device pmio resource info\n");
		return;
	}

	if (!request_region(res_pmio->start, resource_size(res_pmio), pdev->name)) {
		dev_err(&pdev->dev, "can't request region\n");
		return;
	}

	pctrl->pmio_base = res_pmio->start;
	pctrl->pmio_rx90 = 4;
	pctrl->pmio_rx8c = 0;
	zx_pad_write16(pctrl, 0xF8, 0x7F);
	dev_info(pctrl->dev, "KH50000 private init\n");
}

static const struct zhaoxin_pinctrl_soc_data socket_0_soc_data = {
	.uid = "0",
	.pins = kh50000_pins_0,
	.npins = ARRAY_SIZE(kh50000_pins_0),
	.pin_topologys = kh50000_pin_topologys,
	.gpio_type = kh50000_gpio_type,
	.private_init = kh50000_gpio_init,
	.zhaoxin_pin_maps = kh50000_pinmap_gpps,
	.pin_map_size = ARRAY_SIZE(kh50000_pinmap_gpps),
};
static const struct zhaoxin_pinctrl_soc_data socket_1_soc_data = {
	.uid = "1",
	.pins = kh50000_pins_1,
	.npins = ARRAY_SIZE(kh50000_pins_1),
	.pin_topologys = kh50000_pin_topologys,
	.gpio_type = kh50000_gpio_type,
	.private_init = kh50000_gpio_init,
	.zhaoxin_pin_maps = kh50000_pinmap_gpps,
	.pin_map_size = ARRAY_SIZE(kh50000_pinmap_gpps),
};
static const struct zhaoxin_pinctrl_soc_data socket_2_soc_data = {
	.uid = "2",
	.pins = kh50000_pins_2,
	.npins = ARRAY_SIZE(kh50000_pins_2),
	.pin_topologys = kh50000_pin_topologys,
	.gpio_type = kh50000_gpio_type,
	.private_init = kh50000_gpio_init,
	.zhaoxin_pin_maps = kh50000_pinmap_gpps,
	.pin_map_size = ARRAY_SIZE(kh50000_pinmap_gpps),
};
static const struct zhaoxin_pinctrl_soc_data socket_3_soc_data = {
	.uid = "3",
	.pins = kh50000_pins_3,
	.npins = ARRAY_SIZE(kh50000_pins_3),
	.pin_topologys = kh50000_pin_topologys,
	.gpio_type = kh50000_gpio_type,
	.private_init = kh50000_gpio_init,
	.zhaoxin_pin_maps = kh50000_pinmap_gpps,
	.pin_map_size = ARRAY_SIZE(kh50000_pinmap_gpps),
};

static const struct zhaoxin_pinctrl_soc_data *kh50000_soc_data[] = {
	&socket_0_soc_data,
	&socket_1_soc_data,
	&socket_2_soc_data,
	&socket_3_soc_data,
	NULL,
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
	.probe = zhaoxin_pinctrl_probe_by_uid,
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
