#ifndef __WM8978_H
#define __WM8978_H

#define WM8978_ADDR	0x34

unsigned char WM8978_Init(void);
void WM8978_DeInit(void);
void WM8978_HPvol_Set(unsigned char voll, unsigned char volr);
void WM8978_SPKvol_Set(unsigned char volx);

extern int hwi2c_en;


#endif
