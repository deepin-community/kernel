#ifndef __WM8978_H
#define __WM8978_H

#define WM8978_ADDR	0x34

unsigned char WM8978_Write_Reg(unsigned char reg, unsigned short val);
unsigned short WM8978_Read_Reg(unsigned char reg);
unsigned char WM8978_Init(void);
void WM8978_DeInit(void);
void WM8978_ADDA_Cfg(unsigned char dacen, unsigned char adcen);
void WM8978_Output_Cfg(unsigned char dacen, unsigned char bpsen);
void WM8978_HPvol_Set(unsigned char voll, unsigned char volr);
void WM8978_SPKvol_Set(unsigned char volx);
void WM8978_I2S_Cfg(unsigned char fmt, unsigned char len);
void WM8978_Input_Cfg(unsigned char micen, unsigned char lineinen, unsigned char auxen);
void WM8978_MIC_Gain(unsigned char gain);
void WM8978_LINEIN_Gain(unsigned char gain);
void WM8978_AUX_Gain(unsigned char gain);

extern int hwi2c_en;


#endif
