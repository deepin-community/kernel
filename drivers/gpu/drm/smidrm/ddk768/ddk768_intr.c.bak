#include "ddk768_reg.h"
#include "ddk768_chip.h"
#include "ddk768_intr.h"
#include "ddk768_help.h"


/* 
 * Change interrupt mask 
 */
void setIntMask(
    unsigned long mask_on, 
    unsigned long mask_off
)
{
    unsigned long mask;

    /* Get current interrupt mask */
    mask = peekRegisterDWord(INT_MASK);

    /* Enable new masks and disable old masks */
    mask = mask | mask_on;
    mask = mask & ~mask_off;

    /* Program new interrupt mask */
    pokeRegisterDWord(INT_MASK, mask);
}


void sb_IRQMask(int irq_num)
{
		unsigned int mask;
        mask = peekRegisterDWord(INT_MASK);
        mask &= ~(0x1<<irq_num);
        pokeRegisterDWord(INT_MASK,mask);  
  
}




void sb_IRQUnmask(int irq_num)
{
		unsigned int mask;
        mask = peekRegisterDWord(INT_MASK);
        mask = mask | 0x1<<irq_num;
        pokeRegisterDWord(INT_MASK,mask);  
  
}


