/**
*******************************************************************************
videoadd_reg* JM7200 GPU driver
* Copyright (C), Changsha Jingjiamicro Electronics Co.,Ltd.
*******************************************************************************
* videoadd_reg.h
*
*/
#ifndef _VIDEOADD_REG_H_
#define _VIDEOADD_REG_H_
#define LEVEL_LINK_REG_OFFSET(index)	(index<<6)
#define OUTPUT_REG_OFFSET(index)	(((((index&0x2)<<2)|(index&0x1))<<8) + 0x400000)
#define VINPUT_REG_OFFSET(index)	((((index&0x2)<<2)|(index&0x1))<<8)
#define WINDOW_REG_OFFSET(index)	(((0xE*(index>>1))|(index&0x1))<<8)
#endif
