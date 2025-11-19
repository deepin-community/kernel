/*******************************************************************
* 
*         Copyright (c) 2008 by Silicon Motion, Inc. (SMI)
* 
*  LINUX.C --- VGX family DDK
*
*  This file contains LINUX dependent codes, which works only 
*  with PCI boards in x86 systems.
*
*  Don't use this file for other OS, non-x86 system, and non-PCI.
* 
*******************************************************************/
#ifdef LINUX /* This file is for Linux only */
#include <linux/fcntl.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/signal.h>
#include <linux/string.h>
#include <linux/mman.h>

/* Global variable to save all the SMI devices */
static struct pci_dev *g_pCurrentDevice = (struct pci_dev *)0;
static struct pci_access *g_pAccess;
static struct pci_filter g_filter;

/*
 * This function maps a physical address into logical address.
 * Return: NULL address pointer if fail
 *         A Logical address pointer if success.
 */
void *mapPhysicalAddress(
    void *phyAddr,            /* 32 bit physical address */
    unsigned long size        /* Memory size to be mapped */
)
{
    unsigned long address;
    int fileDescriptor;
        
    fileDescriptor = open("/dev/mem", O_RDWR);
    if (fileDescriptor == -1)
        return ((void *)0);
        
    address = (unsigned long) mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, (unsigned long)phyAddr);

    if ((void *) address == MAP_FAILED)
        return ((void *)0);
    
    return ((void *) address);
}

/* Initialize the PCI */
long initPCI(
    unsigned short vendorId, 
    unsigned short deviceId, 
    unsigned short deviceNum
)
{
    unsigned short deviceIndex;
    
    /* Get the pci_access structure */
    g_pAccess = pci_alloc();

    /* Initialize the PCI library */
    pci_init(g_pAccess);

    /* Set all options you want -- here we stick with the defaults */    
    pci_filter_init(g_pAccess, &g_filter);
  	g_filter.vendor = vendorId;
  	g_filter.device = deviceId;
  	
    /* Get the list of devices */
    pci_scan_bus(g_pAccess);
    for(g_pCurrentDevice = g_pAccess->devices, deviceIndex = 0; 
        g_pCurrentDevice; 
        g_pCurrentDevice = g_pCurrentDevice->next)
    {
        if (pci_filter_match(&g_filter, g_pCurrentDevice))
        {
            if (deviceIndex == deviceNum)
            {
                pci_fill_info(g_pCurrentDevice, PCI_FILL_IDENT | PCI_FILL_IRQ | PCI_FILL_BASES | PCI_FILL_ROM_BASE | PCI_FILL_SIZES);
                return 0;
            }
            
            /* Increment the device index */
            deviceIndex++;
        }
    }

    return (-1);
}

/*
 * This function reads a DWord value from the PCI configuration space
 * of a specific PCI device.
 *
 * Inputs are the Vendor and device ID of the device in question.
 * Return: a DWord value.
 */
unsigned long readPCIDword(
    unsigned short vendorId,    /* PCI vendor ID */
    unsigned short deviceId,    /* PCI device ID */
    unsigned short deviceNum,   /* If more than one device in system, device number are ordered as 0, 1, 2,... */
    unsigned short offset       /* Offset in configuration space to be read */
)
{
    if (initPCI(vendorId, deviceId, deviceNum) == 0)
    {
        return ((unsigned long) pci_read_long(g_pCurrentDevice, offset));
    }
    
    return 0;
}

unsigned long readPCIBar0Size(
    unsigned short vendorId,    /* PCI vendor ID */
    unsigned short deviceId,    /* PCI device ID */
    unsigned short deviceNum,   /* If more than one device in system, device number are ordered as 0, 1, 2,... */
    unsigned short offset       /* Offset in configuration space to be read */
)
{
    if (initPCI(vendorId, deviceId, deviceNum) == 0)
    {
        return g_pCurrentDevice->size[0];
    }
    
    return 0;
}

/*
 * This function reads a Word value from the PCI configuration space
 * of a specific device.
 *
 * Inputs are the Vendor and device ID of the device in question.
 * Return: a WORD value.
 */
unsigned short readPCIWord(
    unsigned short vendorId, /* PCI vendor ID */
    unsigned short deviceId, /* PCI device ID */
    unsigned short deviceNum, /* If more than one device in system, device number are ordered as 0, 1, 2,... */
    unsigned short offset     /* Offset in configuration space to be read */
)
{
    if (initPCI(vendorId, deviceId, deviceNum) == 0)
    {
        return ((unsigned short) pci_read_word(g_pCurrentDevice, offset));
    }
    
    return 0;
}

/*
 * This function reads a byte value from the PCI configuration space
 * of a specific device.
 *
 * Inputs are the Vendor and device ID of the device in question.
 * Return: a BYTE value.
 */
unsigned char readPCIByte(
    unsigned short vendorId, /* PCI Vendor ID */
    unsigned short deviceId, /* PCI device ID */
    unsigned short deviceNum, /* If more than one device in system, device number are ordered as 0, 1, 2,... */
    unsigned short offset     /* Offset in configuration space to be read */
)
{
    if (initPCI(vendorId, deviceId, deviceNum) == 0)
    {
        return ((unsigned char) pci_read_byte(g_pCurrentDevice, offset));
    }
    
    return 0;
}

/*
 * This function writes a byte value to the PCI configuration space
 * of a specific device.
 *
 * Inputs are the Vendor and device ID of the device in question.
 * Return:  0 = Success.
 *         -1 = Fail.
 */
long writePCIByte(
    unsigned short vendorId, /* PCI vendor ID */
    unsigned short deviceId, /* PCI device ID */
    unsigned short deviceNum, /* If more than one device in system, device number are ordered as 0, 1, 2,... */
    unsigned short offset,      /* Offset in configuration space to be written */
    unsigned char value       /* To be written BYTE value */
)
{
    if (initPCI(vendorId, deviceId, deviceNum) == 0)
        return (pci_write_byte(g_pCurrentDevice, offset, value));
    
    return (-1);
}

/*
 *	This function enable the write combine (burst)
 */
long registerWCMemoryRange(
	void *phyAddr,            /* 32 bit physical address */
    unsigned long size
)
{
	return (-1);		//unsupport yet
}

/*
 *	This function disable the write combine (burst)
 */
long unregisterWCMemoryRange(
	void *phyAddr,            /* 32 bit physical address */
    unsigned long size
)
{

	long returnValue = (-1);

	return returnValue;
}

/*******************************************************************
 * Interrupt implementation support from the OS
 * 
 * This implementation is used for handling the SM50x interrupt.
 *******************************************************************/
#if 1 //CheokTest(2014/10/01): No use at the moment.
extern void irqHandler();
#endif

short enableTimerInterrupt(
    unsigned char enable
)
{
#if 1 //CheokTest(2014/10/01): No use at the moment.
    struct itimerval tick;
        
    /* Initialize struct */
    memset((void *)&tick, 0, sizeof(tick));

    if (enable != 0)
    {
        signal(SIGALRM, irqHandler);
        
        /* Timeout to run function first time */
        tick.it_value.tv_sec = 0;               /* sec */
        tick.it_value.tv_usec = 5880/*16666*/;           /* micro sec. */

        /* Interval time to run function */
        tick.it_interval.tv_sec = 0;
        tick.it_interval.tv_usec = 5880/*16666*/;
    }

    /* Set timer, ITIMER_REAL : real-time to decrease timer, send SIGALRM when timeout */
    if (setitimer(ITIMER_REAL, &tick, NULL)) {
        printk("setitimer error %d\n", errno);
        return (-1);
    }
#endif
        
    return (0);        
}

/* 
 * Register an interrupt handler (ISR) to the interrupt vector table associated
 * with the given irq number.
 *
 * Input:
 *          irqNumber   - IRQ Number
 *          pfnHandler  - Pointer to the ISR function
 *
 * Output:
 *           0  - Success
 *          -1  - Fail
 */
short registerInterrupt(
    unsigned char irqNumber, 
    void (*pfnHandler)()
)
{
    return enableTimerInterrupt(1);
}

/* 
 * Unregister an interrupt handler from the interrupt vector table
 *
 * Input:
 *          irqNumber   - IRQ Number
 */
short unregisterInterrupt(
    unsigned char irqNumber
)
{
    return enableTimerInterrupt(0);
}

/* 
 * Signal the End Of Interrupt to the system and chain the interrupt
 * if necessary.
 *
 * Input:
 *          irqNumber   - IRQ Number
 */
void interruptEOI(
    unsigned char irqNumber
)
{
}


/*******************************************************************
 * Timer Interrupt implementation
 *******************************************************************/

/* Interrupt structure */
typedef struct _timer_interrupt_t
{
    struct _timer_interrupt_t *next;
    unsigned long totalTicks;
    unsigned long ticksCount;
    void (*handler)(void);
}
timer_interrupt_t;

#define TIMER_INTERRUPT         0x08

static timer_interrupt_t *timer_int_handlers = (timer_interrupt_t *)0;
static void (*pfnOldTimerInterrupt)() = (void (*))0;

static void timerHandler(void)
{
    timer_interrupt_t *p;

    /* Walk all registered handlers for handlers */
    for (p = timer_int_handlers; p != ((timer_interrupt_t *)0); p = p->next)
    {
        /* Increment the tick count for each handlers */
        p->ticksCount++;
        
        /* Call the handler and reset the tickCounts when the function has waited
           for totalTicks number of tick count */
        if (p->ticksCount == p->totalTicks)
        {
            /* Call the function */
            p->handler();
            
            /* Reset the tick count */
            p->ticksCount = 0;
        }
    }

    /* Send EOI to PIC 1 */
    //outp(0x20, 0x20);

#if 0
    /* Chain to previous handler. Somehow, the chain does not seem to work.
       Need further investigation if necessary to chain the interrupt.
       It might not be needed in DOS since we only have one process. */
    if (pfnOldTimerInterrupt)
        _chain_intr (pfnOldTimerInterrupt);
#endif
}

static struct sigaction tact;
static struct itimerval value;
static void init_sigaction(void (*handler)(void))
{
	tact.sa_handler = handler;
	tact.sa_flags = 0;

	sigemptyset(&tact.sa_mask);
	sigaction(SIGALRM, &tact, NULL);
}


static void init_time(struct itimerval *value)
{

	setitimer(ITIMER_REAL, value, NULL);
}

/* 
 * Register interrupt handler
 *
 * Output:
 *      0   - Success
 *      -1  - Out of memory
 *
 * Note:
 *      The interrupt is happens every 55ms, or about 18.2 ticks per second. 
 */
short registerTimerHandler(
    void (*handler)(void),
    unsigned long totalTicks        /* Total number of ticks to wait */
) 
{
    timer_interrupt_t *p;

    /* Allocate a new interrupt structure */
    p = (timer_interrupt_t *) malloc(sizeof(timer_interrupt_t));
    if (p == ((timer_interrupt_t *)0))
    {
        /* No more memory */
        return(-1);
    }

    /* If this is the first interrupt handler, intercept the correct IRQ */
    if (timer_int_handlers == ((timer_interrupt_t *)0))
    {
    #if 0	// dos
        /* Get the previous ISR for the timer interrupt */
	    pfnOldTimerInterrupt = _dos_getvect(TIMER_INTERRUPT);

	    /* Install the new timer interrupt handler */
	    _dos_setvect (TIMER_INTERRUPT, timerHandler);

        /* Enable the interrupt in Interrupt Mask register */
        outp(0x21, inp(0x21) & ~0x01);
    #else	// linux
	   init_sigaction(timerHandler);
        value.it_value.tv_sec = 0;
        value.it_value.tv_usec = 50000;	// 50ms
        value.it_interval = value.it_value;
	   init_time(&value);
    #endif
    }


    /* Fill interrupt structure */
    p->next = timer_int_handlers;
    p->totalTicks = totalTicks;
    p->ticksCount = 0;
    p->handler = handler;
    timer_int_handlers = p;

    return 0;
}

/* 
 * Unregister a registered interrupt handler
 *
 * Output:
 *      0   - Success
 *      -1  - Handler is not found 
 */
short unregisterTimerHandler(
    void (*handler)(void)       /* Interrupt function to be unregistered from the SM50x interrupt */
)
{
    timer_interrupt_t *p, *prev;

    /* Find the requested handle to unregister */
    for (p = timer_int_handlers, prev = ((timer_interrupt_t *)0); p != ((timer_interrupt_t *)0); prev = p, p = p->next)
    {
        if (p->handler == handler)
        {
            /* Remove the interrupt handler */
            if (prev == ((timer_interrupt_t *)0))
            {
                timer_int_handlers = p->next;
            }
            else
            {
                prev->next = p->next;
            }
            free(p);

            /* If this was the last interrupt handler, remove the IRQ handler */
            if (timer_int_handlers == ((timer_interrupt_t *)0))
            {
            	#if 0 //dos
                /* Restore the actual protected mode ISR of the original timer interrupt */
                _dos_setvect(TIMER_INTERRUPT, pfnOldTimerInterrupt);
                #else
			   
		        value.it_value.tv_sec = 0;
		        value.it_value.tv_usec = 0;	// stop the timer
		        value.it_interval = value.it_value;
			   init_time(&value);
			   //init_sigaction(NULL);
                #endif
            }

            /* Success */
            return 0;
        }
    }

    /* Oops, handler is not registered */
    return -1;
}


#include <stdio.h>   
#include <termios.h>   
#include <unistd.h> 
#include <time.h>
int kbhit(void)   
{   
  struct termios oldt, newt;   
  int ch;   
  int oldf;   
  tcgetattr(STDIN_FILENO, &oldt);   
  newt = oldt;   
  newt.c_lflag &= ~(ICANON | ECHO);   
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);   
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);   
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);   
  ch = getchar();   
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);   
  fcntl(STDIN_FILENO, F_SETFL, oldf);   
  if(ch != EOF)   
  {   
    ungetc(ch, stdin);   
    return 1;   
  }   
  return 0;   
}  

/* Get current time in milliseconds. */
unsigned long getCurrentTime(void)
{
    time_t   now; 
    struct   tm     *timenow;
    unsigned long milliseconds;

    /* Get the tick count. */
    time(&now);
    timenow   =   localtime(&now);
    
    milliseconds = (((unsigned long)timenow->tm_hour * 3600 + 
                     (unsigned long)timenow->tm_min * 60 +
                     (unsigned long)timenow->tm_sec) * 1000);
    
    return milliseconds;
}

unsigned char getKeyInput(void)
{
	unsigned char key;
	key = getSingleChar();
	return key;
}

#endif /* LINUX */
