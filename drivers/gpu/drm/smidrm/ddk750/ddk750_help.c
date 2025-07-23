#include "ddk750_help.h"

volatile unsigned char __iomem * mmio750 = NULL;


/* after driver mapped io registers, use this function first */
void ddk750_set_mmio(volatile unsigned char * addr,unsigned short devId,char revId)
{
	mmio750 = addr;
	printk("Found SM750 Chip\n");
}

