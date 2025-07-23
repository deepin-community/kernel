#include <linux/kernel.h>
#include "ddk770_os.h"
#include "ddk770_reg.h"
#include "ddk770_power.h"
#include "ddk770_ddkdebug.h"
#include "ddk770_help.h"
#include "ddk770_gpio.h"


void ddk770_gpioMode(int pin , int mode) //0 =output 1 = intput
{
	unsigned int regval, pinoffset = 0;

	pinoffset = pin << 2;

	regval = peekRegisterDWord(GPIO0_PAD_CONTROL+pinoffset);
	
	if(mode == 0) {	
		pokeRegisterDWord(GPIO0_PAD_CONTROL+pinoffset, 
			FIELD_SET(regval, GPIO0_PAD_CONTROL, OEN, OUTPUT));

	}
	else if(mode == 1) {
			pokeRegisterDWord(GPIO0_PAD_CONTROL+pinoffset, 
			FIELD_SET(regval, GPIO0_PAD_CONTROL, OEN, INPUT));
	}
	return ;
}


/*
This used for disable or enable GPIO Resistor. 

mode: 1: Without Resistor(No PU or PD) 0: With Resistor

*/
void ddk770_gpioRENMode(int pin,int mode)
{
	unsigned int regval, pinoffset = 0;
	
	pinoffset = pin << 2;
	pokeRegisterDWord(GPIO0_PAD_CONTROL+pinoffset,0x300);

	regval = peekRegisterDWord(GPIO0_PAD_CONTROL+pinoffset);
	pokeRegisterDWord(GPIO0_PAD_CONTROL+pinoffset, 
			FIELD_VALUE(regval, GPIO0_PAD_CONTROL, MSC, mode));
}




void ddk770_gpioWrite(int pin, int value)
{
	unsigned int regval, pinoffset = 0;

	pinoffset = pin << 2;

	regval = peekRegisterDWord(GPIO0_PAD_CONTROL + pinoffset);
	pokeRegisterDWord(GPIO0_PAD_CONTROL + pinoffset, 
		FIELD_VALUE(regval, GPIO0_PAD_CONTROL, DATA, value));

	return ;
}

unsigned char ddk770_gpioRead(int pin)
{
	unsigned int regval;
	//unsigned int val;

	if(pin < 32){
		regval = peekRegisterDWord(GPIO_DATA);
		if(regval & 1<<pin)
			return 1;
		else
			return 0;
	}else{
		regval = peekRegisterDWord(GPIO_DATA2);
		if(regval & 1<< (pin - 32))
			return 1;
		else
			return 0;	
	}

}





