#include <linux/string.h>
#include "ddk770_reg.h"
#include "ddk770_help.h"
#include "ddk770_swi2c.h"
#include "ddk770_hwi2c.h"
#include "ddk770_wm8978.h"

#define WM8978_ADDR	0x34

static unsigned short WM8978_REGVAL[58]=
{
	0X0000,0X0000,0X0000,0X0000,0X0050,0X0000,0X0140,0X0000,
	0X0000,0X0000,0X0000,0X00FF,0X00FF,0X0000,0X0100,0X00FF,
	0X00FF,0X0000,0X012C,0X002C,0X002C,0X002C,0X002C,0X0000,
	0X0032,0X0000,0X0000,0X0000,0X0000,0X0000,0X0000,0X0000,
	0X0038,0X000B,0X0032,0X0000,0X0008,0X000C,0X0093,0X00E9,
	0X0000,0X0000,0X0000,0X0000,0X0003,0X0010,0X0010,0X0100,
	0X0100,0X0002,0X0001,0X0001,0X0039,0X0039,0X0039,0X0039,
	0X0001,0X0001
}; 

static unsigned char WM8978_Write_Reg(unsigned char reg, unsigned short val)
{
	unsigned char res;
	unsigned char RegAddr;
	unsigned char RegValue;
	RegAddr = (reg<<1)|((unsigned char)((val>>8)&0x01));
	RegValue = (unsigned char)val;

	res = ddk770_hwI2CWriteReg(0, WM8978_ADDR, RegAddr, RegValue);
	
	if(res == 0)
		WM8978_REGVAL[reg]=val;
	return res;
}

static unsigned short WM8978_Read_Reg(unsigned char reg)
{  
	return WM8978_REGVAL[reg];	
} 

static void WM8978_MIC_Gain(unsigned char gain)
{
	gain &= 0x3F;
	WM8978_Write_Reg(45, gain);		
	WM8978_Write_Reg(46, gain|1<<8);	
}

static void WM8978_LINEIN_Gain(unsigned char gain)
{
	unsigned short regval;
	gain &= 0x07;
	regval = WM8978_Read_Reg(47);	
	regval &= ~(7<<4);						
 	WM8978_Write_Reg(47, regval|gain<<4);	
	regval = WM8978_Read_Reg(48);	
	regval &= ~(7<<4);						
 	WM8978_Write_Reg(48,regval|gain<<4);	
}

static void WM8978_AUX_Gain(unsigned char gain)
{
	unsigned short regval;
	gain &= 0x07;
	regval = WM8978_Read_Reg(47);	
	regval &= ~(7<<0);					
 	WM8978_Write_Reg(47, regval|gain<<0);	
	regval = WM8978_Read_Reg(48);	
	regval &= ~(7<<0);						
 	WM8978_Write_Reg(48, regval|gain<<0);	
}  


static void WM8978_Output_Cfg(unsigned char dacen, unsigned char bpsen)
{
	unsigned short regval = 0;
	if(dacen)
		regval |= 1<<0; 
	if(bpsen)
	{
		regval |= 1<<1; 
		regval |= 5<<2;
	} 
	WM8978_Write_Reg(50,regval);
	WM8978_Write_Reg(51,regval);
}

static void WM8978_ADDA_Cfg(unsigned char dacen, unsigned char adcen)
{
	unsigned short regval;
	regval = WM8978_Read_Reg(3);	
	if(dacen)
		regval |= 3<<0;			
	else 
		regval &= ~(3<<0);			
	WM8978_Write_Reg(3, regval);
	regval = WM8978_Read_Reg(2);	
	if(adcen)
		regval |= 3<<0;			
	else 
		regval &= ~(3<<0);		
	WM8978_Write_Reg(2, regval);	
}

static void WM8978_Input_Cfg(unsigned char micen, unsigned char lineinen, unsigned char auxen)
{
	unsigned short regval;  
	regval = WM8978_Read_Reg(2);
	if(micen)
		regval |= 3<<2;		
	else 
		regval &= ~(3<<2);			
 	WM8978_Write_Reg(2, regval);	
	regval = WM8978_Read_Reg(44);
	if(micen)
		regval |= 3<<4|3<<0;	
	else 
		regval &= ~(3<<4|3<<0);
	WM8978_Write_Reg(44, regval);
	if(lineinen)
		WM8978_LINEIN_Gain(5);
	else 
		WM8978_LINEIN_Gain(0);	
	if(auxen)
		WM8978_AUX_Gain(7);
	else 
		WM8978_AUX_Gain(0);
}

static void WM8978_I2S_Cfg(unsigned char fmt, unsigned char len)
{
	fmt &= 0x03;
	len &= 0x03; 
	WM8978_Write_Reg(4, (fmt<<3)|(len<<5));	
}


unsigned char ddk770_WM8978_Init(void)
{
	unsigned char Res;

	ddk770_hwI2CInit(0);

	Res = WM8978_Write_Reg(0, 0);	
	if(Res)
		return 1;				    
	/* Set volume to 0 can improve the noise when init codec */
	ddk770_WM8978_HPvol_Set(0, 0);  
	ddk770_WM8978_SPKvol_Set(0);      
	WM8978_Write_Reg(1, 0x1B); 
	WM8978_Write_Reg(2, 0x1B0); 
	WM8978_Write_Reg(3, 0x6C);  
	WM8978_Write_Reg(6, 0);	    
	WM8978_Write_Reg(43, 1<<4);
	WM8978_Write_Reg(47, 1<<8);
	WM8978_Write_Reg(48, 1<<8);
	WM8978_Write_Reg(49, 1<<1);
	WM8978_Write_Reg(10, 1<<3);
	WM8978_Write_Reg(14, 1<<3);
	
	/* Playback and record setup */

	WM8978_I2S_Cfg(2, 0);	   
	WM8978_ADDA_Cfg(1, 1);	   
	WM8978_Input_Cfg(1, 1, 1);  
	WM8978_MIC_Gain(63);			//R45&R46, MIC gain (record gain, 0 ~ 63)
	WM8978_Output_Cfg(1, 0);	

	return 0;
}

void ddk770_WM8978_DeInit(void)
{
	ddk770_hwI2CClose(0);
	/* To Do: Here should be read device register not globle array.*/
	WM8978_Write_Reg(0, 0);
}

void ddk770_WM8978_HPvol_Set(unsigned char voll, unsigned char volr)
{
	voll &= 0x3F;
	volr &= 0x3F;				
	if(voll == 0)voll |= 1<<6;	
	if(volr == 0)volr |= 1<<6;	
	WM8978_Write_Reg(52, voll);
	WM8978_Write_Reg(53, volr|(1<<8));
}


void ddk770_WM8978_SPKvol_Set(unsigned char volx)
{
	volx &= 0x3F;
	if(volx == 0)volx |= 1<<6;		
 	WM8978_Write_Reg(54, volx);	
	WM8978_Write_Reg(55, volx|(1<<8)); 
}
