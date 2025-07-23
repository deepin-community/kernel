#ifndef __DDK770_WM8978_H
#define __DDK770_WM8978_H


unsigned char ddk770_WM8978_Init(void);
void ddk770_WM8978_DeInit(void);
void ddk770_WM8978_HPvol_Set(unsigned char voll, unsigned char volr);
void ddk770_WM8978_SPKvol_Set(unsigned char volx);

extern int hwi2c_en;


#endif
