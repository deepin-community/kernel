// SPDX-License-Identifier: GPL-2.0
/* UltraRisc DP1000 pinctrl header
 *
 * Copyright(C) 2025 UltraRisc Technology Co., Ltd.
 *
 *  Author:  wangjia <wangjia@ultrarisc.com>
 */

#ifndef __UR_DP1000_PINCTRL_H__
#define __UR_DP1000_PINCTRL_H__

#define UR_DP1000_IOMUX_A		0x0
#define UR_DP1000_IOMUX_B		0x1
#define UR_DP1000_IOMUX_C		0x2
#define UR_DP1000_IOMUX_D		0x3
#define UR_DP1000_IOMUX_LPC		0x4

#define UR_FUNC_DEF				0
#define UR_FUNC0			1
#define UR_FUNC1			0x10000

/**
 * port: 'A' 'B' 'C'
 *     Pin in the port
 * pin:
 *     PA: 0 - 15
 *     PB-PD: 0 - 7
 * func:
 *     UR_FUNC_DEF: default
 *     UR_FUNC0: func0
 *     UR_FUNC1: func1
 */
#define UR_DP1000_IOPAD(port, pin, func)	(port) (pin) (func)

/**
 * Configure pull up/down resistor of the IO pin
 * UR_PULL_DIS: disable pull-up and pull-down
 * UR_PULL_UP: enable pull-up
 * UR_PULL_DOWN: enable pull-down
 */
#define UR_PULL_DIS		0
#define UR_PULL_UP		1
#define UR_PULL_DOWN		2
/**
 * Configure drive strength of the IO pin
 * UR_DRIVE_DEF: default value, reset value is 2
 * UR_DRIVE_0: 20mA
 * UR_DRIVE_1: 27mA
 * UR_DIRVE_2: 33mA
 * UR_DRIVE_3: 40mA
 */
#define UR_DRIVE_DEF		2
#define UR_DRIVE_0		0
#define UR_DRIVE_1		1
#define UR_DRIVE_2		2
#define UR_DRIVE_3		3

/**
 * Combine the pull-up/down resistor and drive strength
 * pull: UR_PULL_DIS, UR_PULL_UP, UR_PULL_DOWN
 * drive: UR_DRIVE_DEF, UR_DRIVE_0, UR_DRIVE_1, UR_DRIVE_2, UR_DRIVE_3
 */
#define UR_DP1000_BIAS(pull, drive)		(((pull)<<2) + (drive))

#endif
