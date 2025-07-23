/*******************************************************************
* 
*         Copyright (c) 2008 by Silicon Motion, Inc. (SMI)
* 
* OS.H --- VGX family DDK
*
*******************************************************************/
#ifndef _DDK770_OS_H_
#define _DDK770_OS_H_

/* The following definitioin for interrupt service function is platform dependent */
#ifdef WDOSE
#define ISR_FUNC interrupt far
#else
#define ISR_FUNC
#endif

/* Falcon ARM has no OS and PCIe interface.
   We don't need those functions for embeddeded implementation with ARM.
 */
#ifndef SMI_ARM

/*
 * Important:
 * How to implement the functions here is OS, bus and CPU dependent.
 * Please refer to OS.C as an implementation example for WATCOM DOS extender.
 *
 * By keeping the same interface here, rest of the codes in this DDK are
 * pretty much portable for different compilers.
 */

/*
 * This function reads a DWord value from the PCI configuration space
 * of a specific PCI device.
 *
 * Inputs are the Vendor and device ID of the device in question.
 * Return: a DWord value.
 */
unsigned long readPCIDword(
    unsigned short vendorId, /* PCI vendor ID */
    unsigned short deviceId, /* PCI device ID */
    unsigned short deviceNum, /* If more than one device in system, device number are ordered as 0, 1, 2,... */
    unsigned short offset     /* Offset in configuration space to be read */
);

/*
 * This function writes a dword value to the PCI configuration space
 * of a specific device.
 *
 * Inputs are the Vendor and device ID of the device in question.
 * Return:  0 = Success.
 *         -1 = Fail.
 */
long writePCIDword(
    unsigned short vendorId, /* PCI vendor ID */
    unsigned short deviceId, /* PCI device ID */
    unsigned short deviceNum, /* If more than one device in system, device number are ordered as 0, 1, 2,... */
    unsigned short offset,   /* Offset in configuration space to be written */
    unsigned long value     /* To be written BYTE value */
);

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
);

/*
 * This function writes a word value to the PCI configuration space
 * of a specific device.
 *
 * Inputs are the Vendor and device ID of the device in question.
 * Return:  0 = Success.
 *         -1 = Fail.
 */
long writePCIWord(
    unsigned short vendorId, /* PCI vendor ID */
    unsigned short deviceId, /* PCI device ID */
    unsigned short deviceNum, /* If more than one device in system, device number are ordered as 0, 1, 2,... */
    unsigned short offset,   /* Offset in configuration space to be written */
    unsigned short value     /* To be written BYTE value */
);

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
);

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
);

/*
 * This function maps a physical address into logical address.
 * Return: NULL address pointer if fail
 *         A Logical address pointer if success.
 */
void *mapPhysicalAddress(
    void *phyAddr,            /* 32 bit physical address */
    unsigned long size        /* Memory size to be mapped */
);

/*
 * This function unmaps a linear logical address obtained by mapPhysicalAddress.
 * Return:
 *      0   - Success
 *     -1   - Fail
 */
long unmapPhysicalAddress(
    void *linearAddr          /* 32 bit linear address */
);
#endif /* #ifndef SMI_ARM */

/*******************************************************************
 * Interrupt implementation support from the OS
 * 
 * This implementation is used for handling the interrupt.
 *******************************************************************/

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
    void (ISR_FUNC *pfnHandler)(void)
);

/* 
 * Unregister an interrupt handler from the interrupt vector table
 *
 * Input:
 *          irqNumber   - IRQ Number
 */
short unregisterInterrupt(
    unsigned char irqNumber
);

/* 
 * Signal the End Of Interrupt to the system and chain the interrupt
 * if necessary.
 *
 * Input:
 *          irqNumber   - IRQ Number
 */
void interruptEOI(
    unsigned char irqNumber
);

/* 
    We didn't port the following low level functions to ARM yet.
 */
#ifndef SMI_ARM
/*******************************************************************
 * COM Port implementation
 * 
 * This implementation is used by Debug module to send any 
 * debugging messages to other system through serial port.
 *******************************************************************/
typedef enum _baud_rate_t
{
    COM_2400 = 0,
    COM_4800,
    COM_9600,
    COM_19200,
    COM_38400,
    COM_57600,
    COM_115200
} 
baud_rate_t;

typedef enum _data_size_t
{
    DATA_SIZE_8,
    DATA_SIZE_7
}
data_size_t;

// typedef enum _parity_t
// {
//     PARITY_NONE = 0,
//     PARITY_ODD,
//     PARITY_EVEN,
//     PARITY_SPACE
// }
// parity_t;

typedef enum _stop_bit_t
{
    STOP_BIT_1,
    STOP_BIT_2
}
stop_bit_t;

typedef enum _flow_ctrl_t
{
    FLOW_CONTROL_NONE = 0,
    FLOW_CONTROL_HW,
    FLOW_CONTROL_SW
}
flow_ctrl_t;

/* 
 * Initialize the serial port.
 *
 * Parameters:
 *      comPortIndex    - Serial Port Index
 *      baudRate        - The communication speed to be set
 *      dataSize        - Number of bits per characters
 *      parity          - Error checking
 *      stopBit         - Number of bits used as the end of each character
 *      flowCtrl        - Serial port Flow Control (handshake)
 *
 * Returns:
 *       0  - Success
 *      -1  - Fail
 */
// long comInit(
//     unsigned char comPortIndex,
//     baud_rate_t baudRate,
//     data_size_t dataSize,
//     parity_t parity,
//     stop_bit_t stopBit,
//     flow_ctrl_t flowCtrl
// );

/* 
 * Send data out to through the serial port.
 *
 * Parameters:
 *      pszPrintBuffer  - Pointer to a buffer which data to be sent out
 *      length          - Number of characters to be sent out.
 *
 * Returns:
 *      Number of characters that are actually sent out.
 */
unsigned long comWrite(
    char* pszPrintBuffer,
    unsigned long length
);

/* 
 * Close the serial communication port.
 */
void comClose(void);

/* 
 * Call BIOS to set mode
 */
void setModeBIOS(unsigned short mode);

/*
 * Read the IO Port.
 */
unsigned char readIO(
    unsigned short ioPort       /* IO Port */
);

/*
 * Write to the IO Port.
 */
void writeIO(
    unsigned short ioPort,      /* IO Port */
    unsigned char value         /* Value to be written to the port */
);

/*******************************************************************
 * Time related functions
 * 
 * This implementation can be used for performance analysis or delay
 * function.
 *******************************************************************/
 
/* Tick count will be reset after midnight 24 hours. Therefore, when the tick count counter
   reset, it means that it has passed the 24 hours boundary. 
   A Timer tick occurs every 1,193,180 / 65536 (= ~18.20648) times per second. 
   The maximum tick count is calculated as follows: (24*3600*1000/54.9254) = ~1573040
 */
#define MAX_TICKCOUNT                       1573040
unsigned long getSystemTickCount(void);
 
/* Get current time in milliseconds. */
#define MAX_TIME_VALUE                      (24*3600*1000)
unsigned long getCurrentTime(void);

#endif /* #ifndef SMI_ARM */

#endif /* _OS_H_ */
