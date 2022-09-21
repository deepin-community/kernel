/**
*******************************************************************************
* JM7200 GPU driver
* Copyright (C), Changsha Jingjiamicro Electronics Co.,Ltd.
*******************************************************************************
* mwv206reg.h
*
*/
#ifndef __MWV206REG_H
#define __MWV206REG_H

#define MWV206REG_HDMI_REGBAR_ADDR  0x01000000
#define MWV206REG_HDMI_REGBAR_MASK  0x0f000000
#define MWV206REG_HDMI_OFFSET_MASK  0x00ffffff
#define MWV206_HDMI_REG(hdmiid, offset)    ((((0x10000 * (hdmiid)) + (offset)) << 2) + MWV206REG_HDMI_REGBAR_ADDR)

#define MWV206SETREGCMD(reg, count) (\
	(0x40000000) \
	| (1 << (16)) \
	| (((count)-1) << (20)) \
	| ((reg) << (0)) \
	)

#define MWV206COUNTOF(_cmd) ((((_cmd)>>(20)) & 0xff) + 1)
#define MWV206REGOF(_cmd) (((_cmd)>>(0)) & 0xffff)

#define MWV206SURFACEBUFFER(addr_, stride_) ((((addr_) & 0xffffffff) >> 16) | (((stride_) / 16) << 16))
#define MWV2062DBUFFER(addr_, stride_) ((((addr_) & 0xffffffff) >> 16) | (((stride_) / 16) << 16))
#define MWV2063DBUFFER(addr_, stride_) ((((addr_) & 0xffffffff) >> 16) | (((stride_) / 16) << 16))
#define MWV206WAITCMD(waitfor_) ((0x81000000) | (waitfor_))

enum {
	MWV2062D_BLENDMODE_ADD,
	MWV2062D_BLENDMODE_MAXV,
	MWV2062D_BLENDMODE_MINV,
	MWV2062D_BLENDMODE_SUBREV,
	MWV2062D_BLENDMODE_SUB,
	MWV2062D_BLENDMODE_MAX,
};

enum {
	  MWV2062D_BLENDFAC_ZERO,
	  MWV2062D_BLENDFAC_ONE,
	  MWV2062D_BLENDFAC_SRCCLR,
	  MWV2062D_BLENDFAC_INVSRCCLR,
	  MWV2062D_BLENDFAC_SRCALPHA,
	  MWV2062D_BLENDFAC_INVSRCALPHA,
	  MWV2062D_BLENDFAC_DSTALPHA,
	  MWV2062D_BLENDFAC_INVDSTALPHA,
	  MWV2062D_BLENDFAC_INVDSTCLR,
	  MWV2062D_BLENDFAC_DSTCLR,
	 MWV2062D_BLENDFAC_SRCALPHASAT,
	 MWV2062D_BLENDFAC_CONST = 13,
	 MWV2062D_BLENDFAC_INVCONST = 14,
	 MWV2062D_BLENDFAC_MAX,
};


#define MWV206_FB1_BASEADDR 0x80000000
#define MWV206_AXIMAP_FB1_BASEADDR 0xA0000000
#define MWV206_FB0_BASEADDR 0x00000000
#define MWV206_AXIMAP_FB0_BASEADDR 0x40000000

static inline unsigned int mwv206GetAXIAddr(unsigned int mwv206addr)
{
	unsigned int axiAddr;
	if (mwv206addr >= MWV206_FB1_BASEADDR) {
		axiAddr = mwv206addr - MWV206_FB1_BASEADDR + MWV206_AXIMAP_FB1_BASEADDR;
	} else {
		axiAddr = mwv206addr - MWV206_FB0_BASEADDR + MWV206_AXIMAP_FB0_BASEADDR;
	}
	return axiAddr;
}

#endif
