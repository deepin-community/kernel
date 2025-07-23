// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.

#ifndef __SMI_SOUND_H__
#define __SMI_SOUND_H__

#include "smi_dbg.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <sound/control.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>

/* definition of the chip-specific record */
struct sm768chip {
	struct snd_card *card;
	int irq;

    int chipId;

	struct snd_pcm_substream *play_substream;
	struct snd_pcm_substream *capture_substream;
	unsigned long ppointer;
	unsigned long cpointer;
	
	void __iomem *pvReg;
	void __iomem *pvMem;

	unsigned long vidreg_start;
	unsigned long vidreg_size;	
	unsigned long vidmem_start;
	unsigned long vidmem_size;

	spinlock_t lock;

	//master playback volume and capture volume
	u8 playback_vol;//only record one channel, right=left
	u8 capture_vol;
	u8 playback_switch;//only record one channel, right=left
	u8 capture_switch;
	
};

#if 0
struct sm768_runtime_data_father{
	struct sm768_runtime_data * play_data;
	struct sm768_runtime_data * capture_data;
};

struct sm768_runtime_data {
	spinlock_t lock;
	struct sm768chip *params;
	struct snd_pcm_substream *psubstream;
	unsigned long ppointer; /* playback pointer in bytes */
	struct snd_pcm_substream *csubstream;
	unsigned long cpointer; /* capture buffer pointer in bytes */
};
#endif

/* For playback hw parameter*/
#define P_PERIOD_BYTE 		  1024
#define P_PERIOD_MIN 		  16
#define P_PERIOD_MAX 		  256

#define FEATURES	          2/* 1:only output; 2:output and input */
#define SRAM_TOTAL_SIZE	  	  0x1000
#define SAMPLE_BITS		  	  32
#define SECTIONS_NUM	  	  2
#define STEREO			  	  2
#define MONO				  1

#define SRAM_OUTPUT_SIZE	  (SRAM_TOTAL_SIZE/FEATURES)
#define SRAM_INPUT_SIZE	  	  (SRAM_TOTAL_SIZE/FEATURES)
#define SRAM_SECTION_SIZE     (SRAM_OUTPUT_SIZE/OUTPUT_SRAM_SECTIONS_NUM)
#define SRAM_SECTION_SIZE_DWORDS  ((SRAM_SECTION_SIZE/4)-1)
#define OUTPUT_SRAM_SECTIONS_NUM  SECTIONS_NUM


#endif				/* __SMI_DRV_H__ */
