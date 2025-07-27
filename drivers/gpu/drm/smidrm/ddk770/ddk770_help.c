#include "ddk770_help.h"

volatile unsigned char __iomem * mmio770 = NULL;

#if 0
unsigned int peekRegisterDWord(unsigned int offset)
{

   return *(volatile unsigned int *)(mmio770 + offset);
}


void pokeRegisterDWord(unsigned int offset, unsigned int value)
{
 
    *(volatile unsigned int *)(mmio770 + offset) = value;
}


unsigned char peekRegisterByte(unsigned int offset)
{


    return *(volatile unsigned char *)(mmio770 + offset);
}

void pokeRegisterByte(unsigned int offset, unsigned char value)
{

    *(volatile unsigned char *)(mmio770 + offset) = value;

}


#endif

/* after driver mapped io registers, use this function first */
void ddk770_set_mmio(volatile unsigned char * addr,unsigned short devId,char revId)
{
	mmio770 = addr;
	printk("Found SM770 SOC Chip\n");
}


