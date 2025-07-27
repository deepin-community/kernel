#ifndef _DDK770_HELP_H__
#define _DDK770_HELP_H__
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/uaccess.h>



#define PEEK32(addr) readl((addr)+mmio770)
#define POKE32(addr,data) writel((data),(addr)+mmio770)

#if 0
unsigned int peekRegisterDWord(unsigned int offset);
void pokeRegisterDWord(unsigned int offset, unsigned int value);


unsigned char peekRegisterByte(unsigned int offset);

void pokeRegisterByte(unsigned int offset, unsigned char value);



#else


#define peekRegisterDWord(addr) readl((addr)+mmio770)
#define pokeRegisterDWord(addr,data) writel((data),(addr)+mmio770)

#define peekRegisterByte(addr) readb((addr)+mmio770)
#define pokeRegisterByte(addr,data) writeb((data),(addr)+mmio770)

#endif

/* Size of SM770 MMIO and memory */
#define SM770_PCI_ALLOC_MMIO_SIZE       (2*1024*1024)
#define SM770_PCI_ALLOC_MEMORY_SIZE     (256*1024*1024)


void ddk770_set_mmio(volatile unsigned char * addr,unsigned short devId,char revId);

extern volatile unsigned  char __iomem * mmio770;


#endif
