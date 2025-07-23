/*******************************************************************
* 
*         Copyright (c) 2014 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  This file contains the definitions for the PWM functions.
* 
*******************************************************************/
#include "ddk770_reg.h"
#include "ddk770_helper.h"
#include "ddk770_pwm.h"
#include "ddk770_help.h"

static unsigned long gPwm = 0;

/*
 * This function open PWM lines in GPIO Mux
 */
void ddk770_pwmOpen(
    unsigned long pwm           /* which pwm: 0 to 2 */
)
{
    unsigned long value;

    /* Enable GPIO mux (0x10010) 19, 18 or 17 for PWM output */
    value = peekRegisterDWord(GPIO_MUX);

    switch(pwm)
    {
        case 0:
            value = FIELD_SET(value, GPIO_MUX, 21, PWM0);
            break;

        case 1:
            value = FIELD_SET(value, GPIO_MUX, 22, PWM1);
            break;

        default:
            value = FIELD_SET(value, GPIO_MUX, 23, PWM2);
            break;
    }

    pokeRegisterDWord(GPIO_MUX, value);
}

/*
 * This function closes PWM lines in GPIO Mux
 */
void ddk770_pwmClose(
    unsigned long pwm           /* which pwm: 0 to 2 */
)
{
    unsigned long value;

    /* Enable GPIO mux (0x10010) 19, 18 or 17 for PWM output */
    value = peekRegisterDWord(GPIO_MUX);

    switch(pwm)
    {
        case 0:
            value = FIELD_SET(value, GPIO_MUX, 21, GPIO);
            break;

        case 1:
            value = FIELD_SET(value, GPIO_MUX, 22, GPIO);
            break;

        default:
            value = FIELD_SET(value, GPIO_MUX, 23, GPIO);
            break;
    }

    pokeRegisterDWord(GPIO_MUX, value);
}

/*
 * According to input time, this function calculates values
 * for PWM clock divide and PWM counter with 50% duty cycle.
 */
void ddk770_pwmCalcCounter(
    unsigned long mSec,        /* Input time in milli second */
    unsigned long *clkDivider, /* Output clock divider value */
    unsigned long *counter     /* Output counter value for both high and low counter at 50% duty cycle */
)
{
    /* The value of counter depends on the clock frequency the counter is running.
       For FPGA, it is 100MHz. For ASIC, it is 180 to 200 MHz.
     */

    /* For FPGA testing purpose, just return any number for now */
    *clkDivider = 8;  /* Divide by 256 */
    *counter = 0xfff; /* Max 12 bit value */ 
}

/*
 * This function starts PWM.
 * When PWM completes a cycle, its raw interrupt pending bit will be set. 
 *
 * Important limitation: Since all three PWM shares one INT status and mask,
 * only one PWM can set up ISR at any time.
 *
 */
void ddk770_pwmStart(
    unsigned long pwm,           /* which pwm: 0 to 2 */
    unsigned long divider,
    unsigned long highCounter,
    unsigned long lowCounter,
    unsigned long isrSupport     /* 1 if user wants to hook an ISR, 0 if use pulling method 
                                    Note that only one PWM can have ISR at any time.
                                  */
)
{  
    unsigned long pwmAddr, pwmValue;

    if (pwm > 2) return; /* Invalid PWM number */

    if (highCounter == 0 || lowCounter == 0) return; /* Nothing to set */

    /* Work out the PWM's MMIO address 
       PWM 0 address + ( PWM x 4 )
    */
    pwmAddr = PWM_CONTROL + (pwm << 2);

    pwmValue =
          FIELD_VALUE(0, PWM_CONTROL, HIGH_COUNTER, highCounter)
        | FIELD_VALUE(0, PWM_CONTROL, LOW_COUNTER, lowCounter)
        | FIELD_VALUE(0, PWM_CONTROL, CLOCK_DIVIDE, divider)
        | FIELD_SET(0, PWM_CONTROL, INTERRUPT_STATUS, CLEAR)   /* Reset the raw interrupt */
        | FIELD_SET(0, PWM_CONTROL, INTERRUPT, ENABLE)         /* Enable raw interrupt to happen */
        | FIELD_SET(0, PWM_CONTROL, STATUS, ENABLE);           /* Start PWM */

    pokeRegisterDWord(pwmAddr, pwmValue);

    if (isrSupport)
        gPwm = pwm;
}

/*
 * This function checks if a PWM's raw interrupt has been pending.
 * When raw interrupt is detected with pending status, it indicate the
 * a pulse cycle is completed after ddk770_pwmStart().
 * 
 * Return:
 *        1 = Raw interrupt status is pending.
 *        0 = Raw int is NOT pending.
 */
unsigned long ddk770_pwmRawIntPending(
    unsigned long pwm           /* which pwm: 0 to 2 */
)
{  
    unsigned long pwmAddr, rawIntStatus;

    if (pwm > 2) return 0; /* Invalid PWM number */

    /* Work out the PWM's MMIO address 
       PWM 0 address + ( PWM x 4 )
    */
    pwmAddr = PWM_CONTROL + (pwm << 2);

    rawIntStatus = FIELD_VAL_GET(peekRegisterDWord(pwmAddr), PWM_CONTROL, INTERRUPT_STATUS);

    return(rawIntStatus);
}

/*
 * This function clears the RAW interrupt status of PWM.
 * 
 * When PWM completes a cycle, the raw interrupt bit will be set.
 * It has to be cleared, in order to distinguish between different cycles.
 * 
 */
void ddk770_pwmClearRawInt(
    unsigned long pwm           /* which pwm: 0 to 2 */
)
{  
    unsigned long pwmAddr, pwmValue;

    if (pwm > 2) return; /* Invalid PWM number */

    /* Work out the PWM's MMIO address 
       PWM 0 address + ( PWM x 4 )
    */
    pwmAddr = PWM_CONTROL + (pwm << 2);

    pwmValue = peekRegisterDWord(pwmAddr);

    pokeRegisterDWord(pwmAddr,
            pwmValue
          | FIELD_SET(0, PWM_CONTROL, INTERRUPT_STATUS, CLEAR));    /* Reset the raw interrupt */
}

/*
 * This function stop PWM
 *
 */
void ddk770_pwmStop(
    unsigned long pwm           /* which pwm: 0 to 2 */
)
{  
    unsigned long pwmAddr, pwmValue;

    if (pwm > 2) return; /* Invalid PWM number */

    /* Work out the PWM's MMIO address 
       PWM 0 address + ( PWM x 4 )
    */
    pwmAddr = PWM_CONTROL + (pwm << 2);

    pwmValue =
          FIELD_SET(0, PWM_CONTROL, INTERRUPT_STATUS, CLEAR)    /* Reset the raw interrupt */
        | FIELD_SET(0, PWM_CONTROL, STATUS, DISABLE);        /* Stop the PWM */

    pokeRegisterDWord(pwmAddr, pwmValue);
}

/* 
 * This funciton uses PWM to wait a specific amount of time.
 *
 * Input: millisecond unit.
 */
void ddk770_pwmWait(
    unsigned long pwm,
    unsigned long milliSeconds
)
{
    unsigned long divider, highCounter, lowCounter;

    if (pwm > 2) return; /* Invalid PWM number */

    /* Calculate how many ticks are needed for the amount of time.*/
    ddk770_pwmCalcCounter(milliSeconds, &divider, &highCounter);
    lowCounter = highCounter;

    ddk770_pwmStart(pwm, divider, highCounter, lowCounter, 0);

    while (!ddk770_pwmRawIntPending(pwm)); /* Danger, put a timeout here later */

    ddk770_pwmStop(pwm);
}

/* 
 * This function returns the INT mask for a specific PWM
 *
 */
unsigned long ddk770_pwmIntMask(
    unsigned long pwm           /* which pwm: 0 to 2 */
)
{
    unsigned long mask = 0;

    /* Note: In falcon, all PWM shares one INT mask */
    mask |= FIELD_SET(0, INT_MASK1, PWM, ENABLE);

    return mask;
}

/*
 * This is a reference sample showing how to implement ISR for PWM
 * It works together with libsrc\intr.c
 * 
 * Refer to Apps\timer\tstpwm.c on how to hook up this function with system
 * interrupt under WATCOM DOS extender.
 * 
 */
void ddk770_pwmIsrTemplate(unsigned long status)
{
    /* Check if it's PWM interrupt. */

    if (FIELD_VAL_GET(status, INT_STATUS1, PWM))
    {
        /* Perform ISR action for timer 0 here */
        ddk770_incTestCounter();    /* Dummy example action */

        /* In Falcon, sicne all 3 PWM shares one PWM mask,
           There is no way to tell which PWM generates the interrupt.
           Just Clear the one marked down by ddk770_pwmStart.
         */
        ddk770_pwmClearRawInt(gPwm);
    }            
}

