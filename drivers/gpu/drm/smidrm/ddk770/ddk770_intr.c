#include "ddk770_reg.h"
#include "ddk770_chip.h"
#include "ddk770_intr.h"
#include "ddk770_help.h"


/* 
 * Change interrupt mask 
 */
void ddk770_setIntMask(
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


void ddk770_sb_IRQMask(int irq_num)
{
		unsigned int mask;
        mask = peekRegisterDWord(INT_MASK);
        mask &= ~(0x1<<irq_num);
        pokeRegisterDWord(INT_MASK,mask);  
  
}




void ddk770_sb_IRQUnmask(int irq_num)
{
		unsigned int mask;
        mask = peekRegisterDWord(INT_MASK);
        mask = mask | 0x1<<irq_num;
        pokeRegisterDWord(INT_MASK,mask);  
  
}



