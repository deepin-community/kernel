/*******************************************************************
* 
*         Copyright (c) 2014 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  This file contains the definitions for the timer functions.
* 
*******************************************************************/
#ifndef _PWM_H_
#define _PWM_H_

/*
 * This function open PWM lines in GPIO Mux
 */
void ddk768_pwmOpen(
    unsigned long pwm           /* which pwm: 0 to 2 */
);

/*
 * This function closes PWM lines in GPIO Mux
 */
void ddk768_pwmClose(
    unsigned long pwm           /* which pwm: 0 to 2 */
);

/*
 * According to input time, this function calculates values
 * for PWM clock divide and PWM counter with 50% duty cycle.
 */
void ddk768_pwmCalcCounter(
    unsigned long mSec,        /* Input time in milli second */
    unsigned long *clkDivider, /* Output clock divider value */
    unsigned long *counter     /* Output counter value for both high and low counter at 50% duty cycle */
);

/*
 * This function starts PWM.
 * When PWM completes a cycle, its raw interrupt pending bit will be set. 
 *
 * Important limitation: Since all three PWM shares one INT status and mask,
 * only one PWM can set up ISR at any time.
 *
 */
void ddk768_pwmStart(
    unsigned long pwm,           /* which pwm: 0 to 2 */
    unsigned long divider,
    unsigned long highCounter,
    unsigned long lowCounter,
    unsigned long isrSupport     /* 1 if user wants to hook an ISR, 0 if use pulling method 
                                    Note that only one PWM can have ISR at any time.
                                  */
);

/*
 * This function checks if a PWM's raw interrupt has been pending.
 * When raw interrupt is detected with pending status, it indicate the
 * a pulse cycle is completed after pwmStart().
 * 
 * Return:
 *        1 = Raw interrupt status is pending.
 *        0 = Raw int is NOT pending.
 */
unsigned long ddk768_pwmRawIntPending(
    unsigned long pwm           /* which pwm: 0 to 2 */
);

/*
 * This function clears the RAW interrupt status of PWM.
 * 
 * When PWM completes a cycle, the raw interrupt bit will be set.
 * It has to be cleared, in order to distinguish between different cycles.
 * 
 */
void ddk768_pwmClearRawInt(
    unsigned long pwm           /* which pwm: 0 to 2 */
);

/*
 * This function stop PWM
 *
 */
void ddk768_pwmStop(
    unsigned long pwm           /* which pwm: 0 to 2 */
);

/* 
 * This funciton uses PWM to wait a specific amount of time.
 *
 * Input: millisecond unit.
 */
void ddk768_pwmWait(
    unsigned long pwm,
    unsigned long milliSeconds
);

/* 
 * This function returns the INT mask for a specific PWM
 *
 */
unsigned long ddk768_pwmIntMask(
    unsigned long pwm           /* which pwm: 0 to 2 */
);

/*
 * This is a reference sample showing how to implement ISR for PWM
 * It works together with libsrc\intr.c
 * 
 * Refer to Apps\timer\tstpwm.c on how to hook up this function with system
 * interrupt under WATCOM DOS extender.
 * 
 */
void ddk768_pwmIsrTemplate(unsigned long status);

#endif /* _PWM_H_ */

