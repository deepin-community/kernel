/*******************************************************************
*
*         Copyright (c) 2007 by Silicon Motion, Inc. (SMI)
*
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
*
*  HARDWARE.h --- Voyager GX SDK
*  This file contains the definitions for the physical hardware.
*
*******************************************************************/
#ifndef _DDK770_HARDWARE_H_
#define _DDK770_HARDWARE_H_

/* ARM related definitions*/
#ifdef __ICCARM__
#include <stdint.h>

#define MMIO_BASE                       0x80200000U

#define REG32(reg)                      (*(volatile uint32_t*)((uint32_t)reg))
#define REG16(reg)                      (*(volatile uint16_t*)((uint32_t)reg))
#define REG8(reg)                       (*(volatile uint8_t*)((uint32_t)reg))
#endif



/* Silicon Motion PCI vendor ID */
#define SMI_PCI_VENDOR_ID               0x126F

/* Silicon Motion Device ID that is supported by this library. */
#define SMI_DEVICE_ID_SM770             0x0770

/* Maximum number of devices with the same ID supported by this library. */
#define MAX_SMI_DEVICE                  4

//#define MB(x) (x<<20) /* Macro for Mega Bytes */
#define MB(val)	((long long)(val) * 1024 * 1024)

/* Size of SM768 MMIO and memory */
#define SMI_MMIO_SIZE_SM770       MB(2)    /* 2M of MMIO space */
#define SMI_MEMORY_SIZE_SM770     MB(256)  /* Falcon ASIC shuttle 2 has 256M of DDR */
#define DEFAULT_MEM_SIZE MB(256)


#if 1
#define SMI_RELEASE 3
#define PRINT_MAX_NUM 512
//#define REC_SPEED


#define DEBUG_FILE_PATH "/root/tstdma.log"
//#define MB(val)	(val * 1024 * 1024)
#define FILE_MAX_SIZE MB(1)  
#define LOG_BUF_SIZE 128

#if (SMI_RELEASE != 2)
#define  SMI_PRINT(...)         \
do{                         \
char buf[PRINT_MAX_NUM];    \
sprintf(buf, __VA_ARGS__);  \
smi_Print(buf);             \
}while(0)
#else
#define  SMI_PRINT(...)  printf( __VA_ARGS__)
#endif


int smi_Print(char * string);
char transChar(char cin);
#endif
/* This object holds essential informaiton to access the device */
typedef struct _device_object_t
{
    unsigned short vendorId;
    unsigned short deviceId;
    unsigned short deviceNum;
    unsigned short deviceRev;
    unsigned short irq;
    void *mmioPhysicalBase[MAX_SMI_DEVICE];
    void *mmioBase[MAX_SMI_DEVICE];
    void *frameBufPhysicalBase[MAX_SMI_DEVICE];
    void *frameBufBase[MAX_SMI_DEVICE];
    unsigned long mmioSize;
    unsigned long frameBufSize;
}
device_object_t;

/*
 * This function detects if SMI chips is in the system,
 * and create a device object.
 *
 * Input:
 *         regInterval: register delay.
 *         memInterval: memory delay.
 *
 * Return: A non-zero device ID if SMI chip is detected.
 *         Zero means NO SMI chip is detected.
 */
// void * detectDevices(unsigned long regInterval, unsigned long memInterval);

/* Functions to return property of device Object */

/*
 *  Function to return vendor ID property of device object.
 */
unsigned short getVendorId(void);

/*
 *  Function to return device ID property of device object.
 */
unsigned short getDeviceId(void);

/*
 *  Function to return revision ID property of device object.
 */
unsigned char getDeviceRev(void);

/*
 * Get the IRQ number of the current device.
 */
unsigned char getIRQ(void);

/*
 * Get the frame buffer size of the current device.
 */
unsigned long getFrameBufSize(void);

/*
 * Get a pointer to the physical base of Memory Mapped IO space.
 *
 * Return: A pointer to physical base of MMIO.
 *         NULL pointer if fail.
 */
void *getMmioPhysicalBase(void);

/*
 * Get a pointer to the logical base of Memory Mapped IO space.
 * Software need this base address to access MMIO (not the mmioPhysicalBase).
 *
 * Return: A pointer to base of MMIO.
 *         NULL pointer if fail.
 */
void *getMmioBase(void);

/*
 * Get the logical address of an offset location in MMIO space.
 * Return: A valid address if success.
 *         NULL address if fail.
 */
void *getMmioAddress(unsigned long offset /*Offset from base of MMIO*/);

/*
 * Get a pointer to the physical base video memory. This can be used for DMA transfer.
 * In DOS, the physical and the logical address most likely are the same.
 * Return: A pointer to base of physical video memory.
 *         NULL pointer if fail.
 */
void *getFrameBufPhysicalBase(void);

/*
 * Get the physical address of an offset location in frame buffer.
 * In DOS, the physical and the logical address most likely are the same.
 * Return: A valid address if success.
 *         NULL address if fail.
 */
void *getFrameBufPhysicalAddress(unsigned long offset /* Offset from base of physical frame buffer */);

/*
 * Get a pointer to the logical base address of video memory (or frame buffer).
 * Software can only work with logical address.
 *
 * Return: A pointer to base of video memory.
 *         NULL pointer if fail.
 */
void *getFrameBufBase(void);

/*
 * Get the logical address of an offset location in frame buffer.
 * Return: A valid address if success.
 *         NULL address if fail.
 */
void *getFrameBufAddress(unsigned long offset /*Offset from base of frame buffer*/);

unsigned short getNumOfDevices(void);
long setCurrentDevice(unsigned short dev);
unsigned short getCurrentDevice(void);

/* Video Memory read/write functions */
unsigned char peekByte(unsigned long offset);
unsigned short peekWord(unsigned long offset);
unsigned int peekDWord(unsigned long offset);
void poke_4_Byte(unsigned long offset, unsigned char *buf);
void pokeByte(unsigned long offset, unsigned char value);
void pokeWord(unsigned long offset, unsigned short value);
void pokeDWord(unsigned long offset, unsigned long value);


/*
 * This function uses strap pin to detect DDR size, and
 * set up DDR controller accordingly.
 */
unsigned long setupMemController(void);

#endif /* _HARDWARE_H_ */
