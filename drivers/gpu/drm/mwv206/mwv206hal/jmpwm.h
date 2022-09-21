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
#ifndef _JMPWM_H_
#define _JMPWM_H_

#include "mwv206dev.h"


#define JM_PWM_REG_OFFSET               0x10


#define JM_PWM_HIGH_CYCLE_NUMBER        0x00


#define JM_TOTAL_LOW_CYCLE_NUMBER       0x04


#define JM_FORWARD_TIME_REG             0x08



#define JM_PWM_OUTPUT_ENABLE_REG        0x40

#define JM_PWM_DRIVER_VER       "jm_pwm-1.0.0-20160701.1600"

#define JM_PWM_REG_SET(reg, value)  V206DEV002(0x400000 + reg, value)
#define JM_PWM_REG_GET(reg)  V206DEV001(0x400000 + reg)

typedef enum _enum_pwm_channel {
	JM_PWM_CH0 = 0,
	JM_PWM_CH1,
	JM_PWM_CH2,
	JM_PWM_CH3,
	JM_PWM_CH_NUMBER
} JM_PWM_CH_INDEX;

#ifdef __cplusplus
extern "C" {
#endif


int FUNC206HAL163(V206DEV025 * pDev,
		int chIndex,
		int highLevelTime,
		int totalTime,
		int forwareTime);


int FUNC206HAL165(V206DEV025 *pDev,
		int group,
		int chMask);


int FUNC206HAL164(V206DEV025 *pDev,
		int chMask);

#ifdef __cplusplus
}
#endif

#endif