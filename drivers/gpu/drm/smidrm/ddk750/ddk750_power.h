#ifndef _POWER_H_
#define _POWER_H_

#include "../hw_com.h"

/*
 * This function sets the DPMS state 
 */
void setDPMS(DPMS_t state);

/* 
 * This function gets the current power mode 
 */
unsigned long getPowerMode(void);

/* 
 * This function sets the current power mode
 */
void setPowerMode(unsigned long powerMode);

/* 
 * This function sets current gate 
 */
void setCurrentGate(unsigned long gate);

/*
 * This function enable/disable Bus Master
 */
void enableBusMaster(unsigned long enable);

/* 
 *	setPCIMasterBaseAddress
 *		This function set the PCI Master Base Address (used by bus master or DMA).
 *
 *	Input:	
 *		physicalSystemMemAddress	- System physical memory address which PCI
 *									  Master Base Address to be set to.
 *
 *	Output:
 *		The memory address to be set in the register.  
 */
unsigned long setPCIMasterBaseAddress(
	unsigned long physicalSystemMemAddress
);

/*
 * 	This function enable/disable PCI Slave Burst Write provided the CPU supports Write Combine.
 *
 *	Input:
 *			enable		- Enable/Disable the PCI Slave Burst Write (0 = disable, 1 = enable)
 */
void enablePCISlaveBurstWrite(
	unsigned long enable
);

/*
 * 	This function enable/disable PCI Slave Burst Read provided the CPU supports it.
 *
 *	Input:
 *			enable			- Enable/Disable the PCI Slave Burst Read (0 = disable, 1 = enable)
 *			burstReadSize	- Burst Read Size in 32-words (valid values are 1, 2, 4, and 8)
 */
void enablePCISlaveBurstRead(
	unsigned long enable,
	unsigned long burstReadSize
);

/* 
 * This function enable/disable the 2D engine.
 */
void enable2DEngine(unsigned long enable);

/* 
 * This function enable/disable the ZV Port 
 */
void enableZVPort(unsigned long enable);

/* 
 * This function enable/disable the DMA Engine
 */
void enableDMA(unsigned long enable);

/* 
 * This function enable/disable the GPIO Engine
 */
void enableGPIO(unsigned long enable);

/* 
 * This function enable/disable the PWM Engine
 */
void enablePWM(unsigned long enable);

/* 
 * This function enable/disable the I2C Engine
 */
void enableI2C(unsigned long enable);

/* 
 * This function enable/disable the SSP.
 */
void enableSSP(unsigned long enable);

#endif /* _POWER_H_ */
