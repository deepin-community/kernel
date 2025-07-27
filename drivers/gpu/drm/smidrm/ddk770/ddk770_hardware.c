/*******************************************************************
* 
*         Copyright (c) 2007 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  Hardware.c --- Voyager GX SDK 
*  This file contains the source code for the hardware.
* 
*******************************************************************/
#include <linux/kernel.h>
#include "ddk770_reg.h"
#include "ddk770_os.h"
#include "ddk770_help.h"
#include "ddk770_hardware.h"
#include "ddk770_ddkdebug.h"



// static device_object_t deviceObject;
static device_object_t *pDeviceObject = (device_object_t *)0;

/* Total number of devices with the same ID in the system.
   Use 0 as default.
*/
static unsigned short gwNumOfDev = 0;

/* Current device in use.
   If there is only ONE device, it's always device 0.
   If gwNumDev > 1, user need to set device 0, device 1, device 2, etc..
   The purpose is using it control multiple devices of the same ID, if any.
*/
static unsigned short gwCurDev = 0;



/* Functions to return property of device Object */

/*
 *  Function to return vendor ID property of device object.
 */
unsigned short getVendorId(void)
{
    if (pDeviceObject == (device_object_t *)0)
        return 0;

    return pDeviceObject->vendorId;
}

/*
 *  Function to return device ID property of device object.
 */
unsigned short getDeviceId(void)
{
    if (pDeviceObject == (device_object_t *)0)
        return 0;

    return pDeviceObject->deviceId;
}

/*
 *  Function to return revision ID property of device object.
 */
unsigned char getDeviceRev(void)
{
    if (pDeviceObject == (device_object_t *)0)
        return 0;

    return pDeviceObject->deviceRev;
}

/*
 * Get the IRQ number of the current device.
 */
unsigned char getIRQ(void)
{
    if (pDeviceObject == (device_object_t *)0)
        return 0;
    else
        return(pDeviceObject->irq);
}

/*
 * Get the frame buffer size of the current device.
 */
unsigned long getFrameBufSize(void)
{
    if (pDeviceObject == (device_object_t *)0)
        return 0;
    else
        return(pDeviceObject->frameBufSize);
}

/*
 * Get a pointer to the physical base of Memory Mapped IO space.
 *
 * Return: A pointer to physical base of MMIO.
 *         NULL pointer if fail.
 */
void *getMmioPhysicalBase(void)
{
    if (pDeviceObject == (device_object_t *)0)
        return (void *)0;
    else
        return(pDeviceObject->mmioPhysicalBase[gwCurDev]);
}

/*
 * Get a pointer to the logical base of Memory Mapped IO space.
 * Software need this base address to access MMIO (not the mmioPhysicalBase).
 *
 * Return: A pointer to base of MMIO.
 *         NULL pointer if fail.
 */
void *getMmioBase(void)
{
    if (pDeviceObject == (device_object_t *)0)
        return (void *)0;
    else
        return(pDeviceObject->mmioBase[gwCurDev]);
}

/*
 * Get the logical address of an offset location in MMIO space.
 * Return: A valid address if success.
 *         NULL address if fail.
 */
void *getMmioAddress(unsigned long offset /*Offset from base of MMIO*/)
/* Note: When offset is equal to 0, this funciton is same as getMmioBase() */
{
    if (pDeviceObject == (device_object_t *)0)
        return (void *)0;
    else
        return((void *)((unsigned long)pDeviceObject->mmioBase[gwCurDev] + offset));
}

/* 
 * Get a pointer to the physical base video memory. This can be used for DMA transfer.
 * In DOS, the physical and the logical address most likely are the same.
 * Return: A pointer to base of physical video memory.
 *         NULL pointer if fail.
 */
void *getFrameBufPhysicalBase(void)
{
    if (pDeviceObject == (device_object_t *)0)
        return (void *)0;
    else
        return(pDeviceObject->frameBufPhysicalBase[gwCurDev]);
}

/*
 * Get the physical address of an offset location in frame buffer.
 * In DOS, the physical and the logical address most likely are the same.
 * Return: A valid address if success.
 *         NULL address if fail.
 */
void *getFrameBufPhysicalAddress(unsigned long offset /* Offset from base of physical frame buffer */)
/* Note: When offset is equal to 0, this function is same as getFrameBufPhysicalBase() */
{
    if (pDeviceObject == (device_object_t *)0)
        return (void *)0;
    else
        return((void *)((unsigned long)pDeviceObject->frameBufPhysicalBase[gwCurDev]+offset));
}

/* 
 * Get a pointer to the logical base address of video memory (or frame buffer).
 * Software can only work with logical address.
 *
 * Return: A pointer to base of video memory.
 *         NULL pointer if fail.
 */
void *getFrameBufBase(void)
{
    if (pDeviceObject == (device_object_t *)0)
        return (void *)0;
    else
        return(pDeviceObject->frameBufBase[gwCurDev]);
}

/*
 * Get the logical address of an offset location in frame buffer.
 * Return: A valid address if success.
 *         NULL address if fail.
 */
void *getFrameBufAddress(unsigned long offset /*Offset from base of frame buffer*/)
/* Note: When offset is equal to 0, this function is same as getFrameBufBase() */
{
    if (pDeviceObject == (device_object_t *)0)
        return (void *)0;
    else
        return((void *)((unsigned long)pDeviceObject->frameBufBase[gwCurDev]+offset));
}

/*
 * How many devices of the same ID are there.
 */
unsigned short getNumOfDevices()
{
    return gwNumOfDev;
}
 
/*
 * This function sets up the current accessible device, if more
 * than one of the same ID exist in the system.
 *
 * Note:
 * Single device application don't need to call this function.
 * This function is to control multiple devices.
 */
long setCurrentDevice(unsigned short dev)
{
    /* Error check */
    if ( dev >= gwNumOfDev)
        return -1;

    gwCurDev = dev;
    return 0;
}

/*
 * This function gets the current accessible device index.
 */
unsigned short getCurrentDevice()
{
    return gwCurDev;
}


/*
 * This function uses strap pin to detect DDR size, and
 * set up DDR controller accordingly.
 */
unsigned long setupMemController()
{
    unsigned long strapPin; 
	unsigned long rValue;
    unsigned long regValue;
    unsigned int g_ddr_size;
    regValue = peekRegisterDWord(0x6bc);
    regValue &=0xfffffff1;
    strapPin = ( FIELD_VAL_GET(peekRegisterDWord(STRAP_PINS2), STRAP_PINS2, MEM_SIZE) << 1 ) | FIELD_VAL_GET(peekRegisterDWord(STRAP_PINS1), STRAP_PINS1, MEM_SIZE) ;
	
    switch(strapPin)
    {
        case 0x02:
        rValue = MB(512);
        regValue |= 0x6;
        g_ddr_size = 1;
        break;
        case 0x01:
        rValue = MB(1024);
        regValue |= 0x4;
        g_ddr_size = 2;
        break;
        case 0x00:
        rValue = MB(2048);
        regValue |= 0x2;
        g_ddr_size = 3;
        break;
		case 0x03:
        default: /* default size of 512MB. Don't need to do anything */
        rValue = MB(256);
        regValue |= 0x8;
        g_ddr_size = 0;
        break;
    }
    printk("%s strapPin:%lx  rValue:%lx \n", __FUNCTION__,strapPin,rValue);
	//rValue = MB(256); //Just use 1024MB for mammping
	pokeRegisterDWord(0x6bc, regValue);

    return(rValue);
}

