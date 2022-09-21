/*
 * JM7200 GPU driver
 *
 * Copyright (c) 2018 ChangSha JingJiaMicro Electronics Co., Ltd.
 *
 * Author:
 *      rfshen <jjwgpu@jingjiamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef _MWV206IOCTL_H_
#define _MWV206IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mwv206kdevconfig.h"

#define MWV206PACKED __attribute__((packed))
#define MWV206_MAXVERSTRLEN     (128)

typedef unsigned int jjuint32;
typedef int jjint32;

#ifdef __KERNEL_BITWIDTH_DIFFFROM_USER__
typedef unsigned long long jjuintptr;
#else
typedef unsigned long jjuintptr;
#endif

typedef enum {
	MWV206KPARAM_RESMEMSIZE = 0,
	MWV206KPARAM_CORECLOCK,
	MWV206KPARAM_MEMCLOCK,
	MWV206KPARAM_MEMFREE,
	MWV206KPARAM_IRQ,
	MWV206KPARAM_REFCLOCKFREQ,
	MWV206KPARAM_TILEADDRESS,
	MWV206KPARAM_SENDCMDMODE3D,
	MWV206KPARAM_VERTEXORDER,
	MWV206KPARAM_VERTEXLOCATION,
	MWV206KPARAM_MEMBAR_BASEADDR,
	MWV206KPARAM_MEMBLOCKSIZE,
	MWV206KPARAM_DDRCNT,
	MWV206KPARAM_BARNO,
	MWV206KPARAM_BUSNO,
	MWV206KPARAM_GPURBSIZE,
	MWV206KPARAM_CPURBSIZE,
	MWV206KPARAM_DDR0USERCHECKADDR,
	MWV206KPARAM_DDR1USERCHECKADDR,
	MWV206KPARAM_FRAMERATELIMIT,
	MWV206KPARAM_SENDCMDMODE2D,
	MWV206KPARAM_DDR0SIZE,
	MWV206KPARAM_DDR1SIZE,
	MWV206KPARAM_DEVID,
	MWV206KPARAM_MAXCORECLOCK,
	MWV206KPARAM_MEMSHOW,
} e_mwv206_param_t;

typedef enum {
	MWV206KINTR_SD = 0,
	MWV206KINTR_HD_DECODE,
	MWV206KINTR_RENDER,
	MWV206KINTR_CMDPORT,
	MWV206KINTR_CMDSTATUS,
	MWV206KINTR_VS,
	MWV206KINTR_PS0,
	MWV206KINTR_PS1,
	MWV206KINTR_PS2,
	MWV206KINTR_PS3,
	MWV206KINTR_RASTER0,
	MWV206KINTR_RASTER1,
	MWV206KINTR_RASTER2,
	MWV206KINTR_RASTER3,
	MWV206KINTR_DISPLAY0,
	MWV206KINTR_DISPLAY1,
	MWV206KINTR_DISPLAY2,
	MWV206KINTR_DISPLAY3,
	MWV206KINTR_GRAPH_ABNORMAL,
	MWV206KINTR_PCIEX16_DMA = 28,
	MWV206KINTR_PCIEX4_1_DMA = 33,
	MWV206KINTR_PCIEX8_DMA = 38,
	MWV206KINTR_PCIEX4_0_DMA = 43,
	MWV206KINTR_HDMI_0 = 56,
	MWV206KINTR_HDMI_1 = 57,
	MWV206KINTR_HDMI_2 = 58,
	MWV206KINTR_HDMI_3 = 59,
	MWV206KINTR_JJWLINK_SLAVE = 68,
	MWV206KINTR_JJWLINK_MASTER = 69,
	MWV206KINTR_COUNT = 96,
} e_mwv206_intr_t;

typedef enum {
	MWV206K_DPMODE_FIRST = -1,
	MWV206K_DPMODE_640x480x60HZ = 0,
	MWV206K_DPMODE_800x600x60HZ,
	MWV206K_DPMODE_1024x768x60HZ,
	MWV206K_DPMODE_1280x720x60HZ,
	MWV206K_DPMODE_1280x768x60HZ,
	MWV206K_DPMODE_1280x800x60HZ,
	MWV206K_DPMODE_1280x960x60HZ,
	MWV206K_DPMODE_1280x1024x60HZ,
	MWV206K_DPMODE_1360x768x60HZ,
	MWV206K_DPMODE_1366x768x60HZ,
	MWV206K_DPMODE_1400x1050x60HZ,
	MWV206K_DPMODE_1440x900x60HZ,
	MWV206K_DPMODE_1600x1200x60HZ,
	MWV206K_DPMODE_1680x1050x60HZ,
	MWV206K_DPMODE_1792x1344x60HZ,
	MWV206K_DPMODE_1856x1392x60HZ,
	MWV206K_DPMODE_1920x1080x60HZ,
	MWV206K_DPMODE_1920x1200x60HZ,
	MWV206K_DPMODE_1920x1440x60HZ,
	MWV206K_DPMODE_2048x1152x60HZ,
	MWV206K_DPMODE_2560x1600x60HZ,
	MWV206K_DPMODE_3840x2160x30HZ,
	MWV206K_DPMODE_4096x2160x30HZ,
	MWV206K_DPMODE_1920x1440x75HZ,
	MWV206K_DPMODE_USER,
	MWV206K_DPMODE_LAST
} e_display_mode_t;

#define MWV206KINTR_CMDPORT_2D         (MWV206KINTR_COUNT + 8)
#define MWV206KINTR_GROUP               3

typedef enum {
	MWV206K_DP_LVDS_MODE = 0,
} e_output_port_param;

typedef enum lvds_mode_param {
	MWV206K_LVDS_MODE_18BIT_LDI,
	MWV206K_LVDS_MODE_24BIT_LDI,
	MWV206K_LVDS_MODE_18BIT_LDI_SHAKE,
	MWV206K_LVDS_MODE_24BIT_VESA,
} e_lvds_mode_param_t;

#define V206IOCTL001                0
#define V206IOCTL002                 1
#define V206IOCTL003        2
#define V206IOCTL004           3

typedef struct V206IOCTL130 {
	int reg;
	int action;
	unsigned int setvalue;
	unsigned int getvalue;
	unsigned int setmask;
} V206IOCTL167;


typedef struct V206IOCTL124 {
	int V206FB011;
	int mode;

	int htotal, hactive, hfrontporch, hsync, hpol;
	int vtotal, vactive, vfrontporch, vsync, vpol;
	int framerate;
	int V206DEV079;
} V206IOCTL161;


#define MWV206_PORT_IS_HDMI(flags)                ((flags) & 0x1)
#define MWV206_PORT_IS_DVI(flags)                 ((flags) & 0x2)
#define MWV206_PORT_AUDIO_PRESENT(flags)          ((flags) & 0x4)
#define MWV206_PORT_IS_INTERLACED(flags)          ((flags) & 0x8)

#define MWV206_PORT_SET_HDMI(flags)               ((flags) = ((flags) | 0x1) & ~0x2)
#define MWV206_PORT_SET_DVI(flags)                ((flags) = ((flags) | 0x2) & ~0x5)
#define MWV206_PORT_SET_AUDIO(flags)              ((flags) = ((flags) | 0x5) & ~0x2)
#define MWV206_PORT_SET_INTERLACED(flags)         ((flags) |= 0x8)
#define MWV206_PORT_CLEAR_INTERLACED(flags)       ((flags) &= ~0x8)

typedef struct V206IOCTL131 {
	int V206FB011;
	int V206HDMIAUDIO027;
	int mode;
	int htotal, hactive, hfrontporch, hsync, hpol;
	int vtotal, vactive, vfrontporch, vsync, vpol;
	int framerate;
	int dualpixel;
	int flags;
} V206IOCTL168;

typedef struct V206IOCTL137 {
	int  V206FB008;
	int  dualpixel;
	int  V206FB011;
	int  enable;
} V206IOCTL172;

typedef struct V206IOCTL138 {
	int V206FB008;
	int type;
	int value;
} V206IOCTL173;

#define V206IOCTL016    1
#define V206IOCTL017     2
#define V206IOCTL018       4
#define V206IOCTL019     8

typedef struct V206IOCTL123 {
	int options;
	int V206FB011;
	int enable;
	int alpha;
	int x, y;
	int hotx, hoty;
	unsigned int shape;
	unsigned int shapeHi;
} V206IOCTL160;

typedef struct V206IOCTL125 {
	int V206FB011;
	int paletteid;
	unsigned char palette[768];
} V206IOCTL162;

typedef struct V206IOCTL122 {
	int V206FB011;
	unsigned int addr;
	int           format;
	int           V206KG2D033;
	int           width;
	int           height;
	int           vsync;
} V206IOCTL159;

typedef int (*MWV206INTRFUNC)(int intrsrc, void *pDev);

typedef struct V206IOCTL129 {
	unsigned int param;
	unsigned int retval;
} V206IOCTL166;

typedef struct V206IOCTL132 {
	int             dev;
	unsigned int    param;
	int             intrsrc;
	int             inttype;
	MWV206INTRFUNC  V206DEV070;
} V206IOCTL169;

#define MWV206MEM_ALLOC     0
#define MWV206MEM_REFERENCE 1
typedef struct V206IOCTL135 {
	jjuint32 addr;
	int alignment;
	int size;
	int op;
} V206IOCTL170;

typedef struct V206IOCTL136 {
	jjuint32 addr;
} V206IOCTL171;

typedef struct V206IOCTL127 {
	unsigned long long vaddr;
	unsigned long long dma_handle;
	int size;
} V206IOCTL164;

#define V206IOCTL020            0
#define V206IOCTL021           1
#define V206IOCTL022             2
#define V206IOCTL023          3
#define V206IOCTL024         4

typedef struct V206IOCTL134 {
	jjuintptr V206DEV031;
	jjuint32  V206IOCTLCMD009;

	int vsize;
	int size;
	int memstride;
	int mwv206stride;
	unsigned int value;
	int op;
} MWV206PACKED V206IOCTL146;

typedef struct V206IOCTL126 {
	unsigned int V206DEV031;
	unsigned int memaddrHi;
	jjuint32     V206IOCTLCMD009;
	int chan;

	int vsize;
	int size;
	int memstride;
	int mwv206stride;
	int op;
} V206IOCTL163;

#define SYNC_WAITFORIDLE  0
#define SYNC_FLUSH        1
typedef struct V206IOCTL141 {
	int op;
	int timeout;
} V206IOCTL177;

#define	V206IOCTL025      0
#define	V206IOCTL026     1
#define	V206IOCTL027  2
#define	V206IOCTL028 3
typedef struct V206IOCTL133 {
	jjuintptr userctx;
	unsigned int op;
} MWV206PACKED V206IOCTL145 ;

typedef struct V206IOCTL140 {
	char str[MWV206_MAXVERSTRLEN];
} V206IOCTL176;

typedef struct V206IOCTL128 {
	jjuint32 addr;
	int V206KG2D033;
	int V206KG2D001;
	int x, y;
	int width, height;
	unsigned int color, mask;
	int rrop;
} V206IOCTL165;

typedef struct V206IOCTL120 {
	jjuint32 V206KG2D023, V206KG2D004;
	int V206KG2D032, V206KG2D011;
	int V206KG2D001;
	int sx, sy;
	int dx, dy;
	int width, height;
	unsigned int mask, rrop;
} V206IOCTL157;

typedef struct V206IOCTL139 {
	jjuint32 V206KG2D026;
	unsigned int V206KG2D030;
	unsigned int V206KG2D036;
	unsigned int V206KG2D038;
	unsigned int V206KG2D031;
	unsigned int V206KG2D028;
	jjuint32 V206KG2D008;
	unsigned int V206KG2D013;
	unsigned int V206KG2D035;
	unsigned int V206KG2D037;
	unsigned int V206KG2D014;
	unsigned int V206KG2D010;
} V206IOCTL174;


typedef struct V206IOCTL121 {
	int buffen;
	jjuint32 buffaddr, V206KG2D023, V206KG2D004;
	int buffstride, V206KG2D032, V206KG2D011;
	int sfmt, dfmt;
	int V206KG2D024, V206KG2D005;
	int sfactor, dfactor;
	int sx, sy;
	int dx, dy;
	int width, height;
} V206IOCTL158;


typedef struct V206IOCTL142 {
	int buffen;
	jjuint32 buffaddr, V206KG2D023, V206KG2D004;
	int buffstride, V206KG2D032, V206KG2D011;
	int mode;
	int sfmt, dfmt;
	int V206KG2D024, V206KG2D005;
	int sfactor, dfactor;
	int globalalpha;
	int sx, sy;
	int dx, dy;
	int V206KG2D034, V206KG2D025;
	int V206KG2D015, V206KG2D007;
} V206IOCTL178;

#define V206IOCTL034       0
#define V206IOCTL035       1
#define V206IOCTL036      2
#define V206IOCTL037  3
#define V206IOCTL038  4
#define V206IOCTL039 5
#define V206IOCTL040 6
typedef struct V206IOCTL113 {
	void         *mpData;
	int           op;
	int           mOpResult;
	unsigned char mSlaveAddr;
	unsigned char mDestAddr;
	unsigned char mDirect;
	unsigned char mTRLen;
	unsigned char mIsSetStartBit;
	unsigned char mIsSetStopBit;
} V206IOCTL143, *V206IOCTL144;


typedef struct V206IOCTL117 {
	int             spi;
	unsigned int    slave;
	char           *cmdContent;
	unsigned int    cmdLength;
	char           *pTxBuffer;
	unsigned int   *pSendLen;
	char           *pRxBuffer;
	unsigned int   *pRecvLen;
	unsigned int    startRcv;
	unsigned int    transBytes;
	int             sclkPhase;
	int             sclkPolarity;
	int             lsbFirst;
	int             recvSampMode;
} V206IOCTL151, *V206IOCTL152;

typedef struct V206IOCTL119 {
	int             spi;
	char           *cmdContent;
	unsigned int    cmdLength;
	char           *pTxBuffer;
	unsigned int   *pSendLen;
	char           *pRxBuffer;
	unsigned int   *pRecvLen;
	unsigned int    startRcv;
	unsigned int    transBytes;
} V206IOCTL155, *V206IOCTL156;

typedef struct V206IOCTL118 {
	int             spi;
	unsigned int    slave;
	int             sclkPhase;
	int             sclkPolarity;
	int             lsbFirst;
	int             recvSampMode;
} V206IOCTL153, *V206IOCTL154;

typedef struct V206IOCTL115 {
	int           spi;
	unsigned int  freq;
	unsigned int *freq_out;
} V206IOCTL149, *V206IOCTL150;

#define V206IOCTL041    0
#define V206IOCTL042    1
#define V206IOCTL043    2
#define V206IOCTL044     3
#define V206IOCTL045  4
typedef struct V206IOCTL116 {
	union _tag_u {
		V206IOCTL151           trans;
		V206IOCTL155        trans_ex;
		V206IOCTL153   trans_context;
		V206IOCTL149       commufreq;
	} u;
	int op;
} V206IOCTL175;

typedef struct V206IOCTL114 {
	int         mUserPid;
	int         mIrq;
	int         mIndex;
	int         mReserved;
} V206IOCTL147, *V206IOCTL148;

typedef struct V206IOCTL112 {
	int plltype;
	int configfreq;
	int enable;
} V206IOCTL180, *V206IOCTL181;

typedef struct V206IOCTL111 {
	int plltype;
	int freq;
	int maxfreq;
	int minfreq;
} V206IOCTL179;

typedef struct V206IOCTL110 {
	unsigned char edid[8][V206CONFIG010];
	int connect_status[8];
	int changed[8];
	int isfake;
} EDID;

#include <linux/ioctl.h>

#define MWV206_IOC_MAGIC    'j'
#define V206IOCTL063           _IO(MWV206_IOC_MAGIC, 1)
#define V206IOCTL064             _IOWR(MWV206_IOC_MAGIC, 2, V206IOCTL170)
#define V206IOCTL065            _IOW(MWV206_IOC_MAGIC, 3, V206IOCTL146)
#define V206IOCTL066              _IOWR(MWV206_IOC_MAGIC, 4, V206IOCTL171)
#define V206IOCTL067         _IOW(MWV206_IOC_MAGIC, 5, V206IOCTL163)
#define V206IOCTL068    _IOW(MWV206_IOC_MAGIC, 6, V206IOCTL163)
#define V206IOCTL069             _IO(MWV206_IOC_MAGIC, 7)
#define V206IOCTL070            _IOWR(MWV206_IOC_MAGIC, 8, V206IOCTL167)
#define V206IOCTL071             _IOWR(MWV206_IOC_MAGIC, 9, V206IOCTL166)
#define V206IOCTL072           _IO(MWV206_IOC_MAGIC, 10)
#define V206IOCTL073         _IO(MWV206_IOC_MAGIC, 11)
#define V206IOCTL074           _IOW(MWV206_IOC_MAGIC, 12, V206IOCTL165)
#define V206IOCTL075              _IOW(MWV206_IOC_MAGIC, 13, V206IOCTL158)
#define V206IOCTL076           _IOW(MWV206_IOC_MAGIC, 14, V206IOCTL157)
#define V206IOCTL077          _IOW(MWV206_IOC_MAGIC, 15, V206IOCTL178)
#define V206IOCTL078             _IOW(MWV206_IOC_MAGIC, 16, V206IOCTL174)
#define V206IOCTL079       _IOW(MWV206_IOC_MAGIC, 17, V206IOCTL177)
#define V206IOCTL080          _IOW(MWV206_IOC_MAGIC, 18, V206IOCTL177)
#define V206IOCTL081          _IOWR(MWV206_IOC_MAGIC, 19, V206IOCTL145)
#define V206IOCTL082         _IO(MWV206_IOC_MAGIC, 20)
#define V206IOCTL083       _IO(MWV206_IOC_MAGIC, 21)
#define V206IOCTL084       _IO(MWV206_IOC_MAGIC, 22)
#define V206IOCTL085    _IO(MWV206_IOC_MAGIC, 23)
#define V206IOCTL086           _IO(MWV206_IOC_MAGIC, 24)
#define V206IOCTL087          _IOW(MWV206_IOC_MAGIC, 25, V206IOCTL168)
#define V206IOCTL088       _IOW(MWV206_IOC_MAGIC, 26, V206IOCTL159)
#define V206IOCTL089       _IOW(MWV206_IOC_MAGIC, 27, V206IOCTL161)
#define V206IOCTL090       _IOW(MWV206_IOC_MAGIC, 28, V206IOCTL172)
#define V206IOCTL091     _IOW(MWV206_IOC_MAGIC, 29, V206IOCTL160)
#define V206IOCTL092    _IOW(MWV206_IOC_MAGIC, 30, V206IOCTL162)
#define V206IOCTL093  _IOW(MWV206_IOC_MAGIC, 31, V206IOCTL173)
#define V206IOCTL094        _IO(MWV206_IOC_MAGIC, 32)
#define V206IOCTL095    _IO(MWV206_IOC_MAGIC, 33)
#define V206IOCTL096          _IOWR(MWV206_IOC_MAGIC, 34, int)
#define V206IOCTL097         _IOWR(MWV206_IOC_MAGIC, 35, V206IOCTL180)
#define V206IOCTL098          _IOWR(MWV206_IOC_MAGIC, 36, V206IOCTL179)
#define V206IOCTL099         _IO(MWV206_IOC_MAGIC, 37)
#define V206IOCTL100              _IO(MWV206_IOC_MAGIC, 38)
#define V206IOCTL101               _IO(MWV206_IOC_MAGIC, 39)
#define V206IOCTL102              _IO(MWV206_IOC_MAGIC, 40)
#define V206IOCTL103    _IO(MWV206_IOC_MAGIC, 41)
#define V206IOCTL104    _IO(MWV206_IOC_MAGIC, 42)
#define V206IOCTL105             _IO(MWV206_IOC_MAGIC, 43)
#define V206IOCTL106                  _IOWR(MWV206_IOC_MAGIC, 44, V206IOCTL143)
#define V206IOCTL107                  _IOWR(MWV206_IOC_MAGIC, 45, V206IOCTL175)
#define IOCTL_MWV206_SETHDMIMODE_NEW          _IOW(MWV206_IOC_MAGIC, 46, V206IOCTL168)
#define V206IOCTL108  46

#ifdef __cplusplus
}
#endif

#endif