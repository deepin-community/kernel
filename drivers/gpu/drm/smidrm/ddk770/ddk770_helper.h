/*******************************************************************
* 
*         Copyright (c) 2009 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  helper.h --- SMI DDK 
*  This file contains the helper functions those are used throughout
*  the library
* 
*******************************************************************/
#ifndef _DDK770_HELPER_H_
#define _DDK770_HELPER_H_

/* Functions to manipulate a test counter. */
unsigned long ddk770_getTestCounter(void);

void ddk770_setTestCounter(unsigned long value);

void ddk770_incTestCounter(void);


/* Perform a rounded division with signed number. 
 * For example, if the result is 4.5, this function returns 5.
 * If the result is 4.4, this function returns 4.
 */
long ddk770_lRoundedDiv(long num, long denom);

/* Perform a rounded division. 
 * For example, if the result is 4.5, this function returns 5.
 * If the result is 4.4, this function returns 4.
 */
unsigned long ddk770_roundedDiv(unsigned long num, unsigned long denom);

/* Absolute differece between two numbers */
unsigned long ddk770_absDiff(unsigned long a, unsigned long b);

/* This function calculates 2 to the power of x 
   Input is the power number.
 */
unsigned long ddk770_twoToPowerOfx(unsigned long x);

#endif  /* _HELPER_H_ */
