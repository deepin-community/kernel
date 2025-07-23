/*******************************************************************
* 
*         Copyright (c) 2008 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  intr.h --- SM750/SM718 DDK 
*  This file contains the source code for the interrupt mechanism 
* 
*******************************************************************/
#ifndef _DDK770_INTR_H_
#define _DDK770_INTR_H_

#include "ddk770_os.h"

/*******************************************************************
 * Interrupt implementation
 * 
 * This implementation is used for handling the interrupt.
 *******************************************************************/

typedef void (*PFNINTRHANDLER)(unsigned long);

/*
 * Register an interrupt handler function to an interrupt status
 */ 
short registerHandler(
    void (*handler)(unsigned long status), 
    unsigned long mask
);

/*
 * Un-register a registered interrupt handler
 */
short unregisterHandler(
    void (*handler)(unsigned long status)
);

void ddk770_setIntMask(
    unsigned long mask_on, 
    unsigned long mask_off
);


void ddk770_sb_IRQMask(int irq_num);
void ddk770_sb_IRQUnmask(int irq_num);


#define SM770_SB_IRQ_VAL_I2S	21


#endif /* _INTR_H_ */

