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
#include "mwv206dev.h"
#include "mwv206_fb_interpret_edid.h"

#define FBMON_FIX_HEADER  1
#define FBMON_FIX_INPUT   2
#define FBMON_FIX_TIMINGS 3

struct edid_version {
    int version;
    int revision;
};


#define EDID1_LEN 128
#define HEADER 6

#define STD_TIMINGS 8
#define DET_TIMINGS 4

#define  CVT_STANDARD 0x01
#define  CVT_REDUCED  0x02

#define ID_MANUFACTURER_NAME			0x08

#define EDID_STRUCT_VERSION			0x12
#define EDID_STRUCT_REVISION			0x13

#define EDID_STRUCT_DISPLAY                     0x14
#define EDID_INPUT_DIGITAL         (1 << 7)
#define EDID_DETAIL_MONITOR_RANGE 0xfd

#define DPMS_FLAGS				0x18
#define ESTABLISHED_TIMING_1			0x23
#define ESTABLISHED_TIMING_2			0x24
#define MANUFACTURERS_TIMINGS			0x25


#define STD_TIMING                              8
#define STD_TIMING_DESCRIPTION_SIZE             2
#define STD_TIMING_DESCRIPTIONS_START           0x26

#define DETAILED_TIMING_DESCRIPTIONS_START	0x36
#define DETAILED_TIMING_DESCRIPTION_SIZE	18
#define NO_DETAILED_TIMING_DESCRIPTIONS		4

#define DETAILED_TIMING_DESCRIPTION_1		0x36
#define DETAILED_TIMING_DESCRIPTION_2		0x48
#define DETAILED_TIMING_DESCRIPTION_3		0x5a
#define DETAILED_TIMING_DESCRIPTION_4		0x6c

#define _HAVE_CVT(x) (x[10] == 0x04)
#define HAVE_CVT _HAVE_CVT(c)

#define _SUPPORTED_BLANKING(x) ((x[15] & 0x18) >> 3)
#define SUPPORTED_BLANKING _SUPPORTED_BLANKING(c)

#define _NEXT_DT_MD_SECTION(x) (x = (x + DET_TIMING_INFO_LEN))
#define NEXT_DT_MD_SECTION _NEXT_DT_MD_SECTION(c)

#define _VALID_TIMING(x) !(((x[0] == 0x01) && (x[1] == 0x01)) \
						|| ((x[0] == 0x00) && (x[1] == 0x00)) \
						|| ((x[0] == 0x20) && (x[1] == 0x20)))
#define VALID_TIMING _VALID_TIMING(c)
#define _HSIZE1(x) ((x[0] + 31) * 8)
#define HSIZE1 _HSIZE1(c)
#define RATIO(x) ((x[1] & 0xC0) >> 6)
#define RATIO1_1 0

#define RATIO16_10 RATIO1_1
#define RATIO4_3 1
#define RATIO5_4 2
#define RATIO16_9 3
#define _VSIZE1(x, y, r) \
		do { \
			switch (RATIO(x)) { \
			case RATIO1_1: \
				y = ((s->version > 1 || s->revision > 2) \
				       ? (_HSIZE1(x) * 10) / 16 : _HSIZE1(x)); break; \
			case RATIO4_3: \
				y = _HSIZE1(x) * 3 / 4; break; \
			case RATIO5_4: \
				y = _HSIZE1(x) * 4 / 5; break; \
			case RATIO16_9: \
				y = _HSIZE1(x) * 9 / 16; break;\
			} \
		} while (0)
#define VSIZE1(x) _VSIZE1(c, x, s)
#define _REFRESH_R(x) ((x[1] & 0x3F) + 60)
#define REFRESH_R  _REFRESH_R(c)
#define _ID_LOW(x) x[0]
#define ID_LOW _ID_LOW(c)
#define _ID_HIGH(x) (x[1] << 8)
#define ID_HIGH _ID_HIGH(c)
#define STD_TIMING_ID (ID_LOW | ID_HIGH)

#define UPPER_NIBBLE(x) \
		(((128|64|32|16) & (x)) >> 4)

#define LOWER_NIBBLE(x) \
		((1|2|4|8) & (x))

#define COMBINE_HI_8LO(hi, lo) \
		((((unsigned)hi) << 8) | (unsigned)lo)

#define COMBINE_HI_4LO(hi, lo) \
		((((unsigned)hi) << 4) | (unsigned)lo)

#define PIXEL_CLOCK_LO     ((unsigned)block[0])
#define PIXEL_CLOCK_HI     ((unsigned)block[1])
#define PIXEL_CLOCK	   (COMBINE_HI_8LO(PIXEL_CLOCK_HI, PIXEL_CLOCK_LO)*10000)
#define H_ACTIVE_LO        ((unsigned)block[2])
#define H_BLANKING_LO      ((unsigned)block[3])
#define H_ACTIVE_HI        UPPER_NIBBLE((unsigned)block[4])
#define V206DISPMODE018           COMBINE_HI_8LO(H_ACTIVE_HI, H_ACTIVE_LO)
#define H_BLANKING_HI      LOWER_NIBBLE((unsigned)block[4])
#define H_BLANKING         COMBINE_HI_8LO(H_BLANKING_HI, H_BLANKING_LO)

#define V_ACTIVE_LO        ((unsigned)block[5])
#define V_BLANKING_LO      ((unsigned)block[6])
#define V_ACTIVE_HI        UPPER_NIBBLE((unsigned)block[7])
#define V206DISPMODE023           COMBINE_HI_8LO(V_ACTIVE_HI, V_ACTIVE_LO)
#define V_BLANKING_HI      LOWER_NIBBLE((unsigned)block[7])
#define V_BLANKING         COMBINE_HI_8LO(V_BLANKING_HI, V_BLANKING_LO)

#define H_SYNC_OFFSET_LO   ((unsigned)block[8])
#define H_SYNC_WIDTH_LO    ((unsigned)block[9])

#define V_SYNC_OFFSET_LO   UPPER_NIBBLE((unsigned)block[10])
#define V_SYNC_WIDTH_LO    LOWER_NIBBLE((unsigned)block[10])

#define V_SYNC_WIDTH_HI    ((unsigned)block[11] & (1|2))
#define V_SYNC_OFFSET_HI   (((unsigned)block[11] & (4|8)) >> 2)

#define H_SYNC_WIDTH_HI    (((unsigned)block[11] & (16|32)) >> 4)
#define H_SYNC_OFFSET_HI   (((unsigned)block[11] & (64|128)) >> 6)

#define V_SYNC_WIDTH       COMBINE_HI_4LO(V_SYNC_WIDTH_HI, V_SYNC_WIDTH_LO)
#define V_SYNC_OFFSET      COMBINE_HI_4LO(V_SYNC_OFFSET_HI, V_SYNC_OFFSET_LO)

#define H_SYNC_WIDTH       COMBINE_HI_8LO(H_SYNC_WIDTH_HI, H_SYNC_WIDTH_LO)
#define H_SYNC_OFFSET      COMBINE_HI_8LO(H_SYNC_OFFSET_HI, H_SYNC_OFFSET_LO)

#define H_SIZE_LO          ((unsigned)block[12])
#define V_SIZE_LO          ((unsigned)block[13])

#define H_SIZE_HI          UPPER_NIBBLE((unsigned)block[14])
#define V_SIZE_HI          LOWER_NIBBLE((unsigned)block[14])

#define H_SIZE             COMBINE_HI_8LO(H_SIZE_HI, H_SIZE_LO)
#define V_SIZE             COMBINE_HI_8LO(V_SIZE_HI, V_SIZE_LO)

#define H_BORDER           ((unsigned)block[15])
#define V_BORDER           ((unsigned)block[16])

#define FLAGS              ((unsigned)block[17])

#define V_MIN_RATE              block[5]
#define V_MAX_RATE              block[6]
#define H_MIN_RATE              block[7]
#define H_MAX_RATE              block[8]
#define MAX_PIXEL_CLOCK         (((int)block[9]) * 10)
#define GTF_SUPPORT        block[10]

#define INTERLACED         (FLAGS&128)
#define SYNC_TYPE          (FLAGS&3<<3)
#define SYNC_SEPARATE      (3<<3)
#define HSYNC_POSITIVE     (FLAGS & 4)
#define VSYNC_POSITIVE     (FLAGS & 2)

#define EXT_TAG 0
#define EXT_REV 1
#define CEA_EXT   0x02
#define VTB_EXT   0x10
#define DI_EXT    0x40
#define LS_EXT    0x50
#define MI_EXT    0x60

#define CEA_EXT_MIN_DATA_OFFSET 4
#define CEA_EXT_MAX_DATA_OFFSET 127
#define CEA_EXT_DET_TIMING_NUM 6

#define DET_TIMING_INFO_LEN 18

#ifndef M_T_DRIVER
#define M_T_DRIVER	0x40
#endif

struct cea_video_block {
	unsigned char video_code;
};

struct cea_audio_block_descriptor {
	unsigned char audio_code[3];
};

struct cea_audio_block {
	struct cea_audio_block_descriptor descriptor[10];
};

struct cea_vendor_block_hdmi {
	unsigned char portB:4;
	unsigned char portA:4;
	unsigned char portD:4;
	unsigned char portC:4;
	unsigned char support_flags;
	unsigned char max_tmds_clock;
	unsigned char latency_present;
	unsigned char video_latency;
	unsigned char audio_latency;
	unsigned char interlaced_video_latency;
	unsigned char interlaced_audio_latency;
};

struct cea_vendor_block {
	unsigned char ieee_id[3];
	union {
		struct cea_vendor_block_hdmi hdmi;

	};
};

struct cea_speaker_block {
	unsigned char FLR:1;
	unsigned char LFE:1;
	unsigned char FC:1;
	unsigned char RLR:1;
	unsigned char RC:1;
	unsigned char FLRC:1;
	unsigned char RLRC:1;
	unsigned char FLRW:1;
	unsigned char FLRH:1;
	unsigned char TC:1;
	unsigned char FCH:1;
	unsigned char Resv:5;
	unsigned char ResvByte;
};

struct cea_data_block {
	unsigned char len:5;
	unsigned char tag:3;
	union {
		struct cea_video_block video;
		struct cea_audio_block audio;
		struct cea_vendor_block vendor;
		struct cea_speaker_block speaker;
	} u;
};

struct cea_ext_body {
	unsigned char tag;
	unsigned char rev;
	unsigned char dt_offset;
	unsigned char flags;
	struct cea_data_block data_collection;
};

typedef void (*handle_detailed_fn) (unsigned char *, void *, void *);

typedef struct _DisplayModeRec {
	struct _DisplayModeRec *prev;
	struct _DisplayModeRec *next;
	const char *name;
	int status;
	int type;


	int Clock;
	int HDisplay;
	int HSyncStart;
	int HSyncEnd;
	int HTotal;
	int HSkew;
	int VDisplay;
	int VSyncStart;
	int VSyncEnd;
	int VTotal;
	int VScan;
	int Flags;


	int ClockIndex;
	int SynthClock;
	int CrtcHDisplay;
	int CrtcHBlankStart;
	int CrtcHSyncStart;
	int CrtcHSyncEnd;
	int CrtcHBlankEnd;
	int CrtcHTotal;
	int CrtcHSkew;
	int CrtcVDisplay;
	int CrtcVBlankStart;
	int CrtcVSyncStart;
	int CrtcVSyncEnd;
	int CrtcVBlankEnd;
	int CrtcVTotal;
	bool CrtcHAdjusted;
	bool CrtcVAdjusted;
	int PrivSize;
	int32_t *Private;
	int PrivFlags;

	int HSync, VRefresh;
} DisplayModeRec, *DisplayModePtr;



typedef enum {
	V_PHSYNC = 0x0001,
	V_NHSYNC = 0x0002,
	V_PVSYNC = 0x0004,
	V_NVSYNC = 0x0008,
	V_INTERLACE = 0x0010,
	V_DBLSCAN = 0x0020,
	V_CSYNC = 0x0040,
	V_PCSYNC = 0x0080,
	V_NCSYNC = 0x0100,
	V_HSKEW = 0x0200,
	V_BCAST = 0x0400,
	V_PIXMUX = 0x1000,
	V_DBLCLK = 0x2000,
	V_CLKDIV2 = 0x4000
} ModeFlags;

#define M_T_DEFAULT 0x10


static const DisplayModeRec DMTModes[] = {
	{NULL, NULL, NULL, 0, M_T_DRIVER, 31500, 640, 672, 736, 832, 0, 350, 382, 385, 445, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 31500, 640, 672, 736, 832, 0, 400, 401, 404, 445, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 35500, 720, 756, 828, 936, 0, 400, 401, 404, 446, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 25175, 640, 656, 752, 800, 0, 480, 490, 492, 525, 0, V_NHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 31500, 640, 664, 704, 832, 0, 480, 489, 492, 520, 0, V_NHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 31500, 640, 656, 720, 840, 0, 480, 481, 484, 500, 0, V_NHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 36000, 640, 696, 752, 832, 0, 480, 481, 484, 509, 0, V_NHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 36000, 800, 824, 896, 1024, 0, 600, 601, 603, 625, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 40000, 800, 840, 968, 1056, 0, 600, 601, 605, 628, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 50000, 800, 856, 976, 1040, 0, 600, 637, 643, 666, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 49500, 800, 816, 896, 1056, 0, 600, 601, 604, 625, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 56250, 800, 832, 896, 1048, 0, 600, 601, 604, 631, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 73250, 800, 848, 880, 960, 0, 600, 603, 607, 636, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 33750, 848, 864, 976, 1088, 0, 480, 486, 494, 517, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 44900, 1024, 1032, 1208, 1264, 0, 768, 768, 772, 817, 0, V_PHSYNC | V_PVSYNC | V_INTERLACE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 65000, 1024, 1048, 1184, 1344, 0, 768, 771, 777, 806, 0, V_NHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 75000, 1024, 1048, 1184, 1328, 0, 768, 771, 777, 806, 0, V_NHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 78750, 1024, 1040, 1136, 1312, 0, 768, 769, 772, 800, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 94500, 1024, 1072, 1168, 1376, 0, 768, 769, 772, 808, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 115500, 1024, 1072, 1104, 1184, 0, 768, 771, 775, 813, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 108000, 1152, 1216, 1344, 1600, 0, 864, 865, 868, 900, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 68250, 1280, 1328, 1360, 1440, 0, 768, 771, 778, 790, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 79500, 1280, 1344, 1472, 1664, 0, 768, 771, 778, 798, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 102250, 1280, 1360, 1488, 1696, 0, 768, 771, 778, 805, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 117500, 1280, 1360, 1496, 1712, 0, 768, 771, 778, 809, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 140250, 1280, 1328, 1360, 1440, 0, 768, 771, 778, 813, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 71000, 1280, 1328, 1360, 1440, 0, 800, 803, 809, 823, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 83500, 1280, 1352, 1480, 1680, 0, 800, 803, 809, 831, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 106500, 1280, 1360, 1488, 1696, 0, 800, 803, 809, 838, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 122500, 1280, 1360, 1496, 1712, 0, 800, 803, 809, 843, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 146250, 1280, 1328, 1360, 1440, 0, 800, 803, 809, 847, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 108000, 1280, 1376, 1488, 1800, 0, 960, 961, 964, 1000, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 148500, 1280, 1344, 1504, 1728, 0, 960, 961, 964, 1011, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 175500, 1280, 1328, 1360, 1440, 0, 960, 963, 967, 1017, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 108000, 1280, 1328, 1440, 1688, 0, 1024, 1025, 1028, 1066, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 135000, 1280, 1296, 1440, 1688, 0, 1024, 1025, 1028, 1066, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 157500, 1280, 1344, 1504, 1728, 0, 1024, 1025, 1028, 1072, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 187250, 1280, 1328, 1360, 1440, 0, 1024, 1027, 1034, 1084, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 85500, 1360, 1424, 1536, 1792, 0, 768, 771, 777, 795, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 148250, 1360, 1408, 1440, 1520, 0, 768, 771, 776, 813, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 101000, 1400, 1448, 1480, 1560, 0, 1050, 1053, 1057, 1080, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 121750, 1400, 1488, 1632, 1864, 0, 1050, 1053, 1057, 1089, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 156000, 1400, 1504, 1648, 1896, 0, 1050, 1053, 1057, 1099, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 179500, 1400, 1504, 1656, 1912, 0, 1050, 1053, 1057, 1105, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 208000, 1400, 1448, 1480, 1560, 0, 1050, 1053, 1057, 1112, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 88750, 1440, 1488, 1520, 1600, 0, 900, 903, 909, 926, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 106500, 1440, 1520, 1672, 1904, 0, 900, 903, 909, 934, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 136750, 1440, 1536, 1688, 1936, 0, 900, 903, 909, 942, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 157000, 1440, 1544, 1696, 1952, 0, 900, 903, 909, 948, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 182750, 1440, 1488, 1520, 1600, 0, 900, 903, 909, 953, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 162000, 1600, 1664, 1856, 2160, 0, 1200, 1201, 1204, 1250, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 175500, 1600, 1664, 1856, 2160, 0, 1200, 1201, 1204, 1250, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 189000, 1600, 1664, 1856, 2160, 0, 1200, 1201, 1204, 1250, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 202500, 1600, 1664, 1856, 2160, 0, 1200, 1201, 1204, 1250, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 229500, 1600, 1664, 1856, 2160, 0, 1200, 1201, 1204, 1250, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 268250, 1600, 1648, 1680, 1760, 0, 1200, 1203, 1207, 1271, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 119000, 1680, 1728, 1760, 1840, 0, 1050, 1053, 1059, 1080, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 146250, 1680, 1784, 1960, 2240, 0, 1050, 1053, 1059, 1089, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 187000, 1680, 1800, 1976, 2272, 0, 1050, 1053, 1059, 1099, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 214750, 1680, 1808, 1984, 2288, 0, 1050, 1053, 1059, 1105, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 245500, 1680, 1728, 1760, 1840, 0, 1050, 1053, 1059, 1112, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 204750, 1792, 1920, 2120, 2448, 0, 1344, 1345, 1348, 1394, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 261000, 1792, 1888, 2104, 2456, 0, 1344, 1345, 1348, 1417, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 333250, 1792, 1840, 1872, 1952, 0, 1344, 1347, 1351, 1423, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 218250, 1856, 1952, 2176, 2528, 0, 1392, 1393, 1396, 1439, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 288000, 1856, 1984, 2208, 2560, 0, 1392, 1393, 1396, 1500, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 356500, 1856, 1904, 1936, 2016, 0, 1392, 1395, 1399, 1474, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 148500, 1920, 2008, 2052, 2200, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 154000, 1920, 1968, 2000, 2080, 0, 1200, 1203, 1209, 1235, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 193250, 1920, 2056, 2256, 2592, 0, 1200, 1203, 1209, 1245, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 245250, 1920, 2056, 2264, 2608, 0, 1200, 1203, 1209, 1255, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 281250, 1920, 2064, 2272, 2624, 0, 1200, 1203, 1209, 1262, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 317000, 1920, 1968, 2000, 2080, 0, 1200, 1203, 1209, 1271, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 234000, 1920, 2048, 2256, 2600, 0, 1440, 1441, 1444, 1500, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 297000, 1920, 2064, 2288, 2640, 0, 1440, 1441, 1444, 1500, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 380500, 1920, 1968, 2000, 2080, 0, 1440, 1443, 1447, 1525, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 268500, 2560, 2608, 2640, 2720, 0, 1600, 1603, 1609, 1646, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 348500, 2560, 2752, 3032, 3504, 0, 1600, 1603, 1609, 1658, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 443250, 2560, 2768, 3048, 3536, 0, 1600, 1603, 1609, 1672, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 505250, 2560, 2768, 3048, 3536, 0, 1600, 1603, 1609, 1682, 0, V_NHSYNC | V_PVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
	{NULL, NULL, NULL, 0, M_T_DRIVER, 552750, 2560, 2608, 2640, 2720, 0, 1600, 1603, 1609, 1694, 0, V_PHSYNC | V_NVSYNC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, NULL, 0, 0, 0},
};
static void mwv206fb_get_detailed_timing(unsigned char *block, struct fb_videomode *mode)
{
	mode->xres = V206DISPMODE018;
	mode->yres = V206DISPMODE023;
	mode->pixclock = PIXEL_CLOCK;
	mode->pixclock /= 1000;
	mode->pixclock = KHZ2PICOS(mode->pixclock);
	mode->right_margin = H_SYNC_OFFSET;
	mode->left_margin = (V206DISPMODE018 + H_BLANKING) -
						(V206DISPMODE018 + H_SYNC_OFFSET + H_SYNC_WIDTH);
	mode->upper_margin = V_BLANKING - V_SYNC_OFFSET -
						 V_SYNC_WIDTH;
	mode->lower_margin = V_SYNC_OFFSET;
	mode->hsync_len = H_SYNC_WIDTH;
	mode->vsync_len = V_SYNC_WIDTH;
	if (HSYNC_POSITIVE) {
		mode->sync |= FB_SYNC_HOR_HIGH_ACT;
	}
	if (VSYNC_POSITIVE) {
		mode->sync |= FB_SYNC_VERT_HIGH_ACT;
	}
	mode->refresh = PIXEL_CLOCK/((V206DISPMODE018 + H_BLANKING) *
					(V206DISPMODE023 + V_BLANKING));
	if (INTERLACED) {
		mode->yres *= 2;
		mode->upper_margin *= 2;
		mode->lower_margin *= 2;
		mode->vsync_len *= 2;
		mode->vmode |= FB_VMODE_INTERLACED;
	}
	mode->flag = FB_MODE_IS_DETAILED;
	V206DEV005("      %d MHz ",  PIXEL_CLOCK/1000000);
	V206DEV005("%d %d %d %d ", V206DISPMODE018, V206DISPMODE018 + H_SYNC_OFFSET,
	       V206DISPMODE018 + H_SYNC_OFFSET + H_SYNC_WIDTH, V206DISPMODE018 + H_BLANKING);
	V206DEV005("%d %d %d %d ", V206DISPMODE023, V206DISPMODE023 + V_SYNC_OFFSET,
	       V206DISPMODE023 + V_SYNC_OFFSET + V_SYNC_WIDTH, V206DISPMODE023 + V_BLANKING);
	V206DEV005("%sHSync %sVSync\n\n", (HSYNC_POSITIVE) ? "+" : "-",
	       (VSYNC_POSITIVE) ? "+" : "-");
}

static void mwv206fb_calc_mode_timings(int xres, int yres, int refresh,
			      struct fb_videomode *mode)
{
	struct fb_var_screeninfo *var;

	var = kzalloc(sizeof(struct fb_var_screeninfo), GFP_KERNEL);

	if (var) {
		var->xres = xres;
		var->yres = yres;
		fb_get_mode(FB_VSYNCTIMINGS | FB_IGNOREMON,
			    refresh, var, NULL);
		mode->xres = xres;
		mode->yres = yres;
		mode->pixclock = var->pixclock;
		mode->refresh = refresh;
		mode->left_margin = var->left_margin;
		mode->right_margin = var->right_margin;
		mode->upper_margin = var->upper_margin;
		mode->lower_margin = var->lower_margin;
		mode->hsync_len = var->hsync_len;
		mode->vsync_len = var->vsync_len;
		mode->vmode = 0;
		mode->sync = 0;
		kfree(var);
	}
}


static int mwv206fb_get_est_timing(unsigned char *block, struct fb_videomode *mode)
{
	int num = 0;
	unsigned char c;

	c = block[0];
	if (c&0x80) {
		mwv206fb_calc_mode_timings(720, 400, 70, &mode[num]);
		mode[num++].flag = FB_MODE_IS_CALCULATED;
		V206DEV005("      720x400@70Hz\n");
	}
	if (c&0x40) {
		mwv206fb_calc_mode_timings(720, 400, 88, &mode[num]);
		mode[num++].flag = FB_MODE_IS_CALCULATED;
		V206DEV005("      720x400@88Hz\n");
	}
	if (c&0x20) {
		mode[num++] = vesa_modes[3];
		V206DEV005("      640x480@60Hz\n");
	}
	if (c&0x10) {
		mwv206fb_calc_mode_timings(640, 480, 67, &mode[num]);
		mode[num++].flag = FB_MODE_IS_CALCULATED;
		V206DEV005("      640x480@67Hz\n");
	}
	if (c&0x08) {
		mode[num++] = vesa_modes[4];
		V206DEV005("      640x480@72Hz\n");
	}
	if (c&0x04) {
		mode[num++] = vesa_modes[5];
		V206DEV005("      640x480@75Hz\n");
	}
	if (c&0x02) {
		mode[num++] = vesa_modes[7];
		V206DEV005("      800x600@56Hz\n");
	}
	if (c&0x01) {
		mode[num++] = vesa_modes[8];
		V206DEV005("      800x600@60Hz\n");
	}

	c = block[1];
	if (c&0x80) {
		mode[num++] = vesa_modes[9];
		V206DEV005("      800x600@72Hz\n");
	}
	if (c&0x40) {
		mode[num++] = vesa_modes[10];
		V206DEV005("      800x600@75Hz\n");
	}
	if (c&0x20) {
		mwv206fb_calc_mode_timings(832, 624, 75, &mode[num]);
		mode[num++].flag = FB_MODE_IS_CALCULATED;
		V206DEV005("      832x624@75Hz\n");
	}
	if (c&0x10) {
		mode[num++] = vesa_modes[12];
		V206DEV005("      1024x768@87Hz Interlaced\n");
	}
	if (c&0x08) {
		mode[num++] = vesa_modes[13];
		V206DEV005("      1024x768@60Hz\n");
	}
	if (c&0x04) {
		mode[num++] = vesa_modes[14];
		V206DEV005("      1024x768@70Hz\n");
	}
	if (c&0x02) {
		mode[num++] = vesa_modes[15];
		V206DEV005("      1024x768@75Hz\n");
	}
	if (c&0x01) {
		mode[num++] = vesa_modes[21];
		V206DEV005("      1280x1024@75Hz\n");
	}
	c = block[2];
	if (c&0x80) {
		mode[num++] = vesa_modes[17];
		V206DEV005("      1152x870@75Hz\n");
	}
	V206DEV005("      Manufacturer's mask: %x\n", c&0x7F);
	return num;
}

static void handle_detailed_rblank(unsigned char *c, void *data, void *mode)
{
	if (c[3] == EDID_DETAIL_MONITOR_RANGE) {
		if (HAVE_CVT && (SUPPORTED_BLANKING & CVT_REDUCED)) {
			*(bool *) data = TRUE;
		}
	}
}

static void handle_detailed_modes(unsigned char *c, void *data, void *mode)
{
	int sum = 0;
	unsigned char *block = c;
	if (!(block[0] == 0x00 && block[1] == 0x00)) {
		V206DEV005("mwv206dbg: parse detailed_timing! num = %d\n", *(int *)data);
		mwv206fb_get_detailed_timing(block, (struct fb_videomode *)mode);
		sum++;
	}
	*(int *) data += sum;
}

static void mwv206fb_for_each_dt_block(unsigned char *raw_edid, void *mode, handle_detailed_fn fn, void *data)
{
	int i;
	unsigned char *edid = raw_edid;
	unsigned char *ext;
	int dt_offset, dt_num = 0;
	struct fb_videomode *modes = (struct fb_videomode *)mode;
	if (edid == NULL) {
		return;
	}
	for (i = 0; i < DET_TIMINGS; i++) {
		fn(raw_edid + DETAILED_TIMING_DESCRIPTIONS_START + DETAILED_TIMING_DESCRIPTION_SIZE * i, data, modes + *(int *) data);
	}
	for (i = 0; i < raw_edid[0x7e]; i++) {
		ext = raw_edid + EDID1_LEN * (i + 1);
		switch (ext[EXT_TAG]) {
		case CEA_EXT:
			dt_offset = ((struct cea_ext_body *) ext)->dt_offset;
			V206DEV005("mwv206:dt_offset = %d", dt_offset);
			for (; dt_offset < (CEA_EXT_MAX_DATA_OFFSET - DET_TIMING_INFO_LEN) &&
				 dt_num < CEA_EXT_DET_TIMING_NUM; _NEXT_DT_MD_SECTION(dt_offset)) {
				fn(ext + dt_offset, data, modes + *(int *) data);
				dt_num = dt_num + 1;
			}
			break;
		default:
			break;
		}
	}
}


static bool mwv206fb_mon_supports_rb(struct fb_monspecs *specs, unsigned char *edid)
{
	if (specs->revision >= 4 && specs->version >= 1) {
		bool ret = FALSE;

		mwv206fb_for_each_dt_block(edid, NULL, handle_detailed_rblank, &ret);
		return ret;
	}

	return ((edid[EDID_STRUCT_DISPLAY] & EDID_INPUT_DIGITAL) != 0);
}

bool mwv206fb_mode_is_reduced(const DisplayModeRec *mode)
{
	if ((((mode->HDisplay * 5 / 4) & ~0x07) > mode->HTotal) &&
		((mode->HTotal - mode->HDisplay) == 160) &&
		((mode->HSyncEnd - mode->HDisplay) == 80) &&
		((mode->HSyncEnd - mode->HSyncStart) == 32) &&
		((mode->VSyncStart - mode->VDisplay) == 3)) {
		return TRUE;
	}
	return FALSE;
}

static int mwv206fb_get_refresh(const DisplayModeRec *mode)
{
	int refresh = 0;

	if (mode->HTotal > 0 && mode->VTotal > 0) {
		refresh = (mode->Clock * 10000 / mode->HTotal / mode->VTotal + 5)/10;
		if (mode->Flags & V_INTERLACE) {
			refresh *= 2;
		}
		if (mode->Flags & V_DBLSCAN) {
			refresh /= 2;
		}
		if (mode->VScan > 1) {
			refresh /= (mode->VScan);
		}
	}
	return refresh;

}

static void mwv206fb_dismode_to_videomode(const DisplayModeRec *ret, struct fb_videomode *mode)
{
	mode->refresh = (ret->Clock * 10000 / ret->HTotal / ret->VTotal + 5)/10;
	mode->xres = ret->HDisplay;
	mode->yres = ret->VDisplay;
	mode->pixclock = KHZ2PICOS(ret->Clock);
	mode->left_margin = ret->HTotal - ret->HSyncEnd;
	mode->right_margin = ret->HSyncStart - ret->HDisplay;
	mode->upper_margin = ret->VTotal - ret->VSyncEnd;
	mode->lower_margin = ret->VSyncStart - ret->VDisplay;
	mode->hsync_len = ret->HSyncEnd - ret->HSyncStart;
	mode->vsync_len = ret->VSyncEnd - ret->VSyncStart;
	mode->sync = (ret->Flags & V_PHSYNC) | ((ret->Flags & V_NHSYNC) >> 1) | ((ret->Flags & V_PVSYNC) >> 1) | ((ret->Flags & V_NVSYNC) >> 2);
	mode->vmode = (ret->Flags & V_INTERLACE) >> 4;
	mode->flag |= FB_MODE_IS_STANDARD;
	V206DEV005("mwv206: m->refresh = %d m->xres = %d m->yres = %d m->left_margin = %d "\
		"m->right_margin = %d m->upper_margin =%d m->lower_margin = %d m->hsync_len = %d"\
		"m->vsync_len = %d m->sync = %d m->vmode = %d m->flag = 0x%x\n",
		mode->refresh, mode->xres, mode->yres, mode->left_margin, mode->right_margin, mode->upper_margin, mode->lower_margin,
		mode->hsync_len, mode->vsync_len, mode->sync, mode->vmode, mode->flag);
}

static void mwv206fb_find_dmt_mode(int hsize, int vsize, int refresh, bool rb, struct fb_videomode *mode, int *found)
{
	int i;
	const DisplayModeRec *ret;
	for (i = 1; i < sizeof(DMTModes) / sizeof(DisplayModeRec); i++) {
		ret = &DMTModes[i];

		if (!rb && mwv206fb_mode_is_reduced(ret)) {
			continue;
		}

		if (ret->HDisplay == hsize &&
			ret->VDisplay == vsize && refresh == mwv206fb_get_refresh(ret)) {
			*found = 1;
			break;
		}
	}
	V206DEV005("mwv206: mwv206fb_find_dmt_mode found = %d\n", *found);
	if (*found == 1) {
		mwv206fb_dismode_to_videomode(ret, mode);
	}
}


static int mwv206fb_get_std_timing(unsigned char *c, struct fb_videomode *mode,
			const struct fb_monspecs *s, bool rb)
{
	int hsize = 0, vsize = 0, refresh = 0, id = 0;
	int found = 0;
	if (VALID_TIMING) {
		hsize = HSIZE1;
		VSIZE1(vsize);
		refresh = REFRESH_R;
		id = STD_TIMING_ID;
	} else {
		hsize = vsize = refresh = id = 0;
		V206DEV005("mwv206: edid is invalid!\n");
		return 0;
	}
	V206DEV005("mwv206: hsize = %d vsize = %d refresh = %d", hsize, vsize, refresh);
	if (hsize && vsize && refresh) {
		mwv206fb_find_dmt_mode(hsize, vsize, refresh, rb, mode, &found);
	}

	if (!found) {
		V206DEV005("mwv206: can not find in dmt!\n");
		return 0;
	}

	if (s && s->dclkmax
	    && PICOS2KHZ(mode->pixclock) * 1000 > s->dclkmax) {
		V206KDEBUG003("mwv206: mode exceed max DCLK\n");
		return 0;
	}
	return 1;
}

static int mwv206fb_get_dt_modes(struct fb_monspecs *specs, unsigned char *edid, struct fb_videomode *mode)
{
	int sum = 0;

	mwv206fb_for_each_dt_block(edid, mode, handle_detailed_modes, &sum);

	return sum;
}

struct broken_edid {
	u8  manufacturer[4];
	u32 model;
	u32 fix;
};

static const struct broken_edid brokendb[] = {

	{
		.manufacturer = "DEC",
		.model        = 0x073a,
		.fix          = FBMON_FIX_HEADER,
	},

	{
		.manufacturer = "VSC",
		.model        = 0x5a44,
		.fix          = FBMON_FIX_INPUT,
	},

	{
		.manufacturer = "SHP",
		.model        = 0x138e,
		.fix          = FBMON_FIX_TIMINGS,
	},
};

static const unsigned char edid_v1_header[] = { 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x00
};

static int edid_is_serial_block(unsigned char *block)
{
	if ((block[0] == 0x00) && (block[1] == 0x00) &&
		(block[2] == 0x00) && (block[3] == 0xff) &&
		(block[4] == 0x00)) {
		return 1;
	} else {
		return 0;
	}
}

static int edid_is_ascii_block(unsigned char *block)
{
	if ((block[0] == 0x00) && (block[1] == 0x00) &&
		(block[2] == 0x00) && (block[3] == 0xfe) &&
		(block[4] == 0x00)) {
		return 1;
	} else {
		return 0;
	}
}

static int edid_is_limits_block(unsigned char *block)
{
	if ((block[0] == 0x00) && (block[1] == 0x00) &&
		(block[2] == 0x00) && (block[3] == 0xfd) &&
		(block[4] == 0x00)) {
		return 1;
	} else {
		return 0;
	}
}

static int edid_is_monitor_block(unsigned char *block)
{
	if ((block[0] == 0x00) && (block[1] == 0x00) &&
		(block[2] == 0x00) && (block[3] == 0xfc) &&
		(block[4] == 0x00)) {
		return 1;
	} else {
		return 0;
	}
}

static int edid_is_timing_block(unsigned char *block)
{
	if ((block[0] != 0x00) || (block[1] != 0x00) ||
		(block[2] != 0x00) || (block[4] != 0x00)) {
		return 1;
	} else {
		return 0;
	}
}

static int check_edid(unsigned char *edid)
{
	unsigned char *block = edid + ID_MANUFACTURER_NAME, manufacturer[4];
	unsigned char *b;
	u32 model;
	int i, fix = 0, ret = 0;

	manufacturer[0] = ((block[0] & 0x7c) >> 2) + '@';
	manufacturer[1] = ((block[0] & 0x03) << 3) +
		((block[1] & 0xe0) >> 5) + '@';
	manufacturer[2] = (block[1] & 0x1f) + '@';
	manufacturer[3] = 0;
	model = block[2] + (block[3] << 8);

	for (i = 0; i < ARRAY_SIZE(brokendb); i++) {
		if (!strncmp(manufacturer, brokendb[i].manufacturer, 4) &&
			brokendb[i].model == model) {
			fix = brokendb[i].fix;
			break;
		}
	}

	switch (fix) {
	case FBMON_FIX_HEADER:
		for (i = 0; i < 8; i++) {
			if (edid[i] != edid_v1_header[i]) {
				ret = fix;
				break;
			}
		}
		break;
	case FBMON_FIX_INPUT:
		b = edid + EDID_STRUCT_DISPLAY;

		if (b[4] & 0x01 && b[0] & 0x80) {
			ret = fix;
		}
		break;
	case FBMON_FIX_TIMINGS:
		b = edid + DETAILED_TIMING_DESCRIPTIONS_START;
		ret = fix;

		for (i = 0; i < 4; i++) {
			if (edid_is_limits_block(b)) {
				ret = 0;
				break;
			}

			b += DETAILED_TIMING_DESCRIPTION_SIZE;
		}

		break;
	}

	if (ret)
		V206KDEBUG003("mwv206: The EDID Block of "
		       "Manufacturer: %s Model: 0x%x is known to "
		       "be broken,\n",  manufacturer, model);

	return ret;
}

static void fix_edid(unsigned char *edid, int fix)
{
	int i;
	unsigned char *b, csum = 0;

	switch (fix) {
	case FBMON_FIX_HEADER:
		V206DEV005("fbmon: trying a header reconstruct\n");
		memcpy(edid, edid_v1_header, 8);
		break;
	case FBMON_FIX_INPUT:
		V206DEV005("fbmon: trying to fix input type\n");
		b = edid + EDID_STRUCT_DISPLAY;
		b[0] &= ~0x80;
		edid[127] += 0x80;
		break;
	case FBMON_FIX_TIMINGS:
		V206DEV005("fbmon: trying to fix monitor timings\n");
		b = edid + DETAILED_TIMING_DESCRIPTIONS_START;
		for (i = 0; i < 4; i++) {
			if (!(edid_is_serial_block(b) ||
				edid_is_ascii_block(b) ||
				edid_is_monitor_block(b) ||
				edid_is_timing_block(b))) {
				b[0] = 0x00;
				b[1] = 0x00;
				b[2] = 0x00;
				b[3] = 0xfd;
				b[4] = 0x00;
				b[5] = 60;
				b[6] = 60;
				b[7] = 30;
				b[8] = 75;
				b[9] = 17;
				b[10] = 0;
				break;
			}

			b += DETAILED_TIMING_DESCRIPTION_SIZE;
		}

		for (i = 0; i < EDID1_LEN - 1; i++)
			csum += edid[i];

		edid[127] = 256 - csum;
		break;
	}
}

static int edid_checksum(unsigned char *edid)
{
	unsigned char csum = 0, all_null = 0;
	int i, err = 0, fix = check_edid(edid);

	if (fix) {
		fix_edid(edid, fix);
	}
	for (i = 0; i < EDID1_LEN; i++) {
		csum += edid[i];
		all_null |= edid[i];
	}

	if (csum == 0x00 && all_null) {

		err = 1;
	}

	return err;
}

static int edid_check_header(unsigned char *edid)
{
	int i, err = 1, fix = check_edid(edid);

	if (fix) {
		fix_edid(edid, fix);
	}
	for (i = 0; i < 8; i++) {
		if (edid[i] != edid_v1_header[i]) {
			err = 0;
		}
	}

	return err;
}

 struct fb_videomode *mwv206fb_interpret_edid(unsigned char *edid, int *dbsize, struct fb_monspecs *specs)
{
	struct fb_videomode *mode, *m;
	unsigned char *block;
	int num = 0, i;
	bool rb;
	mode = kcalloc(50, sizeof(struct fb_videomode), GFP_KERNEL);
	if (!mode) {
		V206KDEBUG003("mwv206: mode = NULL!\n");
		return NULL;
	}

	*dbsize = 0;
	rb = mwv206fb_mon_supports_rb(specs, edid);

	num += mwv206fb_get_dt_modes(specs, edid, &mode[num]);
	block = edid + ESTABLISHED_TIMING_1;
	V206DEV005("mwv206dbg: parse est_timing! num = %d\n", num);
	num += mwv206fb_get_est_timing(block, &mode[num]);

	block = edid + STD_TIMING_DESCRIPTIONS_START;
	for (i = 0; i < STD_TIMING; i++, block += STD_TIMING_DESCRIPTION_SIZE) {
		V206DEV005("mwv206dbg: parse std_timing! num = %d\n", num);
		num += mwv206fb_get_std_timing(block, &mode[num], specs, rb);
	}

	if (!num) {
		kfree(mode);
		return NULL;
	}
	V206DEV005("mwv206dbg: parse edid end! num = %d\n", num);
	*dbsize = num;

	m = kmalloc_array(num, sizeof(struct fb_videomode), GFP_KERNEL);
	if (!m) {
		return mode;
	}
	memmove(m, mode, num * sizeof(struct fb_videomode));
	kfree(mode);

	return m;
}


void mwv206fb_destroy_modedb(struct fb_videomode *modedb)
{
	kfree(modedb);
}

static int mwv206fb_get_monitor_limits(unsigned char *edid, struct fb_monspecs *specs)
{
	int i, retval = 1;
	unsigned char *block;

	block = edid + DETAILED_TIMING_DESCRIPTIONS_START;

	V206DEV005(" 	 Monitor Operating Limits: ");

	for (i = 0; i < 4; i++, block += DETAILED_TIMING_DESCRIPTION_SIZE) {
		if (edid_is_limits_block(block)) {
			specs->hfmin = H_MIN_RATE * 1000;
			specs->hfmax = H_MAX_RATE * 1000;
			specs->vfmin = V_MIN_RATE;
			specs->vfmax = V_MAX_RATE;
			specs->dclkmax = MAX_PIXEL_CLOCK * 1000000;
			specs->gtf = (GTF_SUPPORT) ? 1 : 0;
			retval = 0;
			V206DEV005("From EDID\n");
			break;
		}
	}

	V206DEV005(" 		  H: %d-%dKHz V: %d-%dHz DCLK: %dMHz\n",
	specs->hfmin/1000, specs->hfmax/1000, specs->vfmin,
	specs->vfmax, specs->dclkmax/1000000);
	return retval;
}

static void mwv206fb_get_monspecs(unsigned char *edid, struct fb_monspecs *specs)
{
	memset(specs, 0, sizeof(struct fb_monspecs));

	specs->version = edid[EDID_STRUCT_VERSION];
	specs->revision = edid[EDID_STRUCT_REVISION];

	mwv206fb_get_monitor_limits(edid, specs);
}

void mwv206fb_edid_to_monspecs(unsigned char *edid, struct fb_monspecs *specs)
{
	if (!edid) {
		V206KDEBUG003("mwv206: edid = NULL!\n");
		return;
	}

	if (!(edid_checksum(edid))) {
		return;
	}
	if (!(edid_check_header(edid))) {
		return;
	}

	mwv206fb_get_monspecs(edid, specs);
	specs->modedb = mwv206fb_interpret_edid(edid, &specs->modedb_len, specs);
}