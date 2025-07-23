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
#include "ddk770_reg.h"
#include "ddk770_helper.h"
#include "ddk770_timer.h"
#include "ddk770_help.h"

/* 
 * A global varible to store the counter values set in the timer.
 * It is needed because the counter value cannot be read back from the timer.
 * A read to the timer counter only gets the latest value being decremented.
 */
static unsigned long gTimerCounter[4] = {0, 0, 0, 0};

/*
 * Calculate a value for timer counter according to input time in micro-second.
 * Calculation is based on 168MHz master clock, and the counter decrements at every 16 ticks.
 */
unsigned long ddk770_calcTimerCounter(
    unsigned long microSeconds
)
{
    return( microSeconds * 168 / 16);
}

/*
 * This function start the timer with raw interrupt enabled.
 * When the timer decrements to 0, timer raw interrupt will be generated.
 *
 * Raw interrupt of the timers can be used in one of 2 ways:
 * 1. In pulling mode, detection of raw interrupt pending means timer is decremented to 0.
 * 2. In interrupt mode, unlock the timer interrupt mask will generate a interrput to system.
 *
 */
void ddk770_timerStart(
    timer_number_t timer,         /* which timer: 0 to 3 */
    unsigned long timerCounter,   /* Timer counter */
    unsigned long div16Enable     /* Enable the 16 divisor, time out will be increased by 16 */
)
{  
    unsigned long ulTimerAddr, ulTimerValue;

    if (timerCounter == 0) return; /* Nothing to set */

    /* Work out the timer's MMIO address 
       Timer 0 address + ( timer number x 4 )
    */
    ulTimerAddr = TIMER_CONTROL + (timer << 2);

    ulTimerValue =
          FIELD_VALUE(0, TIMER_CONTROL, COUNTER, timerCounter)
        | FIELD_SET(0, TIMER_CONTROL, RAWINT_STATUS, RESET)    /* Reset the raw interrupt */
        | FIELD_SET(0, TIMER_CONTROL, RAWINT_ENABLE, ENABLE)   /* Enable raw interrupt to happen when time out */
        | FIELD_VALUE(0, TIMER_CONTROL, DIV16, div16Enable)
        | FIELD_SET(0, TIMER_CONTROL, ENABLE, ENABLE);           /* Start the timer */

    pokeRegisterDWord(ulTimerAddr, ulTimerValue);

    gTimerCounter[timer] = timerCounter;
}

/*
 * This function checks if a timer's raw interrupt has been pending.
 * When raw interrupt is detected with pending status, it indicate the
 * countdown of ddk770_timerStart() has been completed.
 * 
 * Return:
 *        1 = Raw interrupt status is pending.
 *        0 = Raw int is NOT pending.
 */
unsigned long ddk770_timerRawIntPending(
    timer_number_t timer         /* which timer: 0 to 3 */
)
{  
    unsigned long ulTimerAddr, rawIntStatus;

    /* Work out the timer's MMIO address 
       Timer 0 address + ( timer number x 4 )
    */
    ulTimerAddr = TIMER_CONTROL + (timer << 2);
    rawIntStatus = FIELD_VAL_GET(peekRegisterDWord(ulTimerAddr), TIMER_CONTROL, RAWINT_STATUS);

    return(rawIntStatus);
}

/*
 * This function clears the RAW interrupt status of the timer.
 * 
 * When a timer completes countdown, the raw interrupt bit will be set.
 * It has to be cleared, in order to distinguish between different sessions of countdown.
 * 
 */
void ddk770_timerClearRawInt(
    timer_number_t timer         /* which timer: 0 to 3 */
)
{  
    unsigned long ulTimerAddr, ulTimerValue;

    /* Work out the timer's MMIO address 
       Timer 0 address + ( timer number x 4 )
    */
    ulTimerAddr = TIMER_CONTROL + (timer << 2);
    ulTimerValue = peekRegisterDWord(ulTimerAddr)
                 & FIELD_CLEAR(TIMER_CONTROL, COUNTER); /* We don't want the current counter value */

    pokeRegisterDWord(ulTimerAddr,
            ulTimerValue
          | FIELD_VALUE(0, TIMER_CONTROL, COUNTER, gTimerCounter[timer]) /* When clearing raw int, we don't want to erase the original counter value */
          | FIELD_SET(0, TIMER_CONTROL, RAWINT_STATUS, RESET));    /* Reset the raw interrupt */
}

/*
 * This function stop the timer.
 *
 */
void ddk770_timerStop(
    timer_number_t timer         /* which timer: 0 to 3 */
)
{  
    unsigned long ulTimerAddr, ulTimerValue;

    /* Work out the timer's MMIO address 
       Timer 0 address + ( timer number x 4 )
    */
    ulTimerAddr = TIMER_CONTROL + (timer << 2);

    ulTimerValue =
          FIELD_SET(0, TIMER_CONTROL, RAWINT_STATUS, RESET)    /* Reset the raw interrupt */
        | FIELD_SET(0, TIMER_CONTROL, ENABLE, DISABLE);        /* Stop the timer */

    pokeRegisterDWord(ulTimerAddr, ulTimerValue);

    gTimerCounter[timer] = 0;
}

/*
 * This function read the current value in the timer counter.
 *
 * Note: When timer is disable, always read back 0.
 */
unsigned long ddk770_timerGetCounter(
    timer_number_t timer         /* which timer: 0 to 3 */
)
{  
    unsigned long ulTimerAddr, ulCounter;

    /* Work out the timer's MMIO address 
       Timer 0 address + ( timer number x 4 )
    */
    ulTimerAddr = TIMER_CONTROL + (timer << 2);
    ulCounter = FIELD_VAL_GET(peekRegisterDWord(ulTimerAddr), TIMER_CONTROL, COUNTER);

    return(ulCounter);
}

/*
 * This function gets the countdown setting stored in timer.
 * Function ddk770_timerGetCounter() can only get the current counter value.
 * It cannot get the original countdown setting of timer.
 *
 * Note: When timer is disable, always read back 0.
 */
unsigned long ddk770_timerGetCounterSetting(
    timer_number_t timer         /* which timer: 0 to 3 */
)
{  
    /* The counter value put in timer cannot be read back from the register.
       It has to keep in global variable.
    */
    return(gTimerCounter[timer]);
}

/* 
 * This funciton uses the timer to wait a specific amount of time
 * in micro-second.
 */
void ddk770_timerWait(
    timer_number_t timer,
    unsigned long microSeconds
)
{
    unsigned long ticks;

    // Limit  max delay to 10 seconds 
    if (microSeconds > 10000000)
        microSeconds = 10000000;

    ticks = ddk770_calcTimerCounter(microSeconds);

    //Tick count is based on enabling DIV 16.
    //Third parameter to ddk770_timerStart is 1.
    ddk770_timerStart(timer, ticks, 1);

    while (!ddk770_timerRawIntPending(timer));

    ddk770_timerStop(timer);
}

/* 
 * This funciton uses the timer to wait a specific ticks of master clock
 *
 */
void ddk770_timerWaitTicks(
    timer_number_t timer, /* Use timer 0, 1, 2 or 3 */
    unsigned long ticks
)
{
    //Counter is 28 bits only.
    ticks &= 0xFFFFFFF;

    ddk770_timerStart(timer, ticks, 0);

    while (!ddk770_timerRawIntPending(timer));

    ddk770_timerStop(timer);
}

/* 
 * This function returns the INT mask for a specific timer.
 *
 */
unsigned long ddk770_timerIntMask(
    timer_number_t timer        /* Which timer: 0 to 3 */
)
{
    unsigned long mask;

    mask = 0;
    switch (timer)
    {
        case 0:
            mask |= FIELD_SET(0, INT_MASK, TIMER0, ENABLE);
            break;
        case 1:
            mask |= FIELD_SET(0, INT_MASK, TIMER1, ENABLE);
            break;
        case 2:
            mask |= FIELD_SET(0, INT_MASK, TIMER2, ENABLE);
            break;
        case 3:
            mask |= FIELD_SET(0, INT_MASK, TIMER3, ENABLE);
            break;
        default:
            break;
    }

    return mask;
}

/*
 * This is a reference sample showing how to implement ISR for timers.
 * It works together with libsrc\intr.c module.
 * 
 * Refer to Apps\timer\tstimer.c on how to hook up this function with system
 * interrupt under WATCOM DOS extender.
 * 
 */
void ddk770_timerIsrTemplate(unsigned long status)
{
    if (FIELD_VAL_GET(status, INT_STATUS, TIMER0) == INT_STATUS_TIMER0_ACTIVE)
    {
        /* Perform ISR action for timer 0 here */
        ddk770_incTestCounter();

        ddk770_timerClearRawInt(0);
    }            

    if (FIELD_VAL_GET(status, INT_STATUS, TIMER1) == INT_STATUS_TIMER1_ACTIVE)
    {
        /* Perform ISR action for timer 1 here */
        ddk770_incTestCounter();

        ddk770_timerClearRawInt(1);
    }            

    if (FIELD_VAL_GET(status, INT_STATUS, TIMER2) == INT_STATUS_TIMER2_ACTIVE)
    {
        /* Perform ISR action for timer 2 here */
        ddk770_incTestCounter();

        ddk770_timerClearRawInt(2);
    }            

    if (FIELD_VAL_GET(status, INT_STATUS, TIMER3) == INT_STATUS_TIMER3_ACTIVE)
    {
        /* Perform ISR action for timer 3 here */
        ddk770_incTestCounter();

        ddk770_timerClearRawInt(3);
    }            
}

