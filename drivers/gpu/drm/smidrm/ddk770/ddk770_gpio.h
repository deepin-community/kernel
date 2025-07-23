#ifndef _DDK770_GPIO_H_
#define _DDK770_GPIO_H_


void ddk770_gpioMode(int pin, int mode);
void ddk770_gpioWrite(int pin, int value);
unsigned char ddk770_gpioRead(int pin);
void ddk770_gpioRENMode(int pin,int mode);


#endif
