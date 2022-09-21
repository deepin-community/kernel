/*
 * Copyright (C) 2016 SiliconMotion Inc.
 *
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License v2. See the file COPYING in the main directory of this archive for
 * more details.
 */

#include "smi_drv.h"

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/slab.h>

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/timer.h>



#include "smi_snd.h"
#include "ddk768/uda1345.h"
#include "ddk768/wm8978.h"
#include "ddk768/ddk768_reg.h"
#include "ddk768/ddk768_iis.h"
#include "ddk768/ddk768_intr.h"
#include "ddk768/ddk768_power.h"
#include "ddk768/ddk768_chip.h"



//#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>


#include "hw768.h"

#include "smi_dbg.h"

struct sm768chip *chip_irq_id=NULL;/*chip_irq_id is use for request and free irq*/
int use_wm8978 = 0;

static int SM768_AudioInit(unsigned long wordLength, unsigned long sampleRate)
{
	// Set up I2S and GPIO registers to transmit/receive data.
    iisOpen(wordLength, sampleRate);
    //Set I2S to DMA 256 DWord from SRAM starting at location 0 of SRAM
    iisTxDmaSetup(0,SRAM_SECTION_SIZE);


	// Init audio codec
	if(use_wm8978){
		printk("Use WM8978 Codec\n");
		if (WM8978_Init())
		{			
			printk("Init WM8978 Codec failed\n");
			WM8978_DeInit();
			
		}
	}else{
		printk("Use UDA1345 Codec\n");
		if(uda1345_init())
		 {   
		   printk("Init UDA1345 Codec failed\n");
		   uda1345_deinit();
		 }
	}

    return 0;
}


/*
 * This function call iis driver interface iisStart() to start play audio. 
 */
static int SM768_AudioStart(void)
{

	
	if(use_wm8978){
  
	}else{
	    uda1345_setpower(ADCOFF_DACON);
		uda1345_setmute(NO_MUTE);
	}

	HDMI_Audio_Unmute();

	iisStart();
	
    return 0;
}

/*
 * Stop audio. 
 */
static int SM768_AudioStop(void)
{

	if(use_wm8978){
	}else{
		uda1345_setmute(MUTE);
		uda1345_setpower(ADCOFF_DACOFF);
	}

	HDMI_Audio_Mute();
	
	iisStop();
	
    return 0;
}



static int SM768_AudioDeinit(void)
{

	iisStop();
	iisClose();

	if(use_wm8978)
		WM8978_DeInit();
	else
		uda1345_deinit();
	
	sb_IRQMask(SB_IRQ_VAL_I2S);		
	iisClearRawInt();
    ddk768_enableI2S(0); 

	return 0;
}





static u8  VolAuDrvToCodec(u16 audrv)
{
	u8 codecdb,map;
    if(audrv == 0x8000)
        return 0x3b;

    codecdb = (audrv >> 8);

    if(codecdb < 0x80)
        map = (0x1f - (codecdb >> 2));
    else
        map = (0x5f - (codecdb >> 2));
    return map;
}


static int snd_falconi2s_info_hw_volume(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_info *uinfo)
{
	dbg_msg("snd_falconi2s_info_hw_volume\n");
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0x1f;
	return 0;
}

static int snd_falconi2s_get_hw_play_volume(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct sm768chip *chip = kcontrol->private_data;
	dbg_msg("snd_falconi2s_get_hw_volume\n");
	
	ucontrol->value.integer.value[0] = chip->playback_vol;
	
	return 0;
}
static int snd_falconi2s_put_hw_play_volume(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{

	struct sm768chip *chip = kcontrol->private_data;
	int changed = 0;
	u8 vol;
	dbg_msg("snd_falconi2s_put_hw_volume:%ld\n", ucontrol->value.integer.value[0]);

	if (chip->playback_vol!= ucontrol->value.integer.value[0]) {
		vol = chip->playback_vol = ucontrol->value.integer.value[0];

		if(use_wm8978){
			WM8978_HPvol_Set(VolAuDrvToCodec(vol), VolAuDrvToCodec(vol));
			WM8978_SPKvol_Set(VolAuDrvToCodec(vol));
		}else{
			uda1345_setvolume(VolAuDrvToCodec(vol));
		}
		changed = 1;
	}

	return changed;

}


static int snd_falconi2s_get_hw_capture_volume(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct sm768chip *chip = kcontrol->private_data;
	dbg_msg("snd_falconi2s_get_hw_volume\n");
	
	ucontrol->value.integer.value[0] = chip->capture_vol;
	
	return 0;
}
static int snd_falconi2s_put_hw_capture_volume(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct sm768chip *chip = kcontrol->private_data;
	int changed = 0;
	u8 vol;
	dbg_msg("snd_falconi2s_put_hw_volume:%ld\n", ucontrol->value.integer.value[0]);


	if (chip->capture_vol!= ucontrol->value.integer.value[0]) {
		vol = chip->capture_vol = ucontrol->value.integer.value[0];

		if(use_wm8978){
					WM8978_HPvol_Set(VolAuDrvToCodec(vol), VolAuDrvToCodec(vol));
					WM8978_SPKvol_Set(VolAuDrvToCodec(vol));
		}else{
					uda1345_setvolume(VolAuDrvToCodec(vol));
		}

		changed = 1;
	}

	return changed;
	
}


static struct snd_kcontrol_new falconi2s_vol[] = {
 {
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Playback Volume",
		.info = snd_falconi2s_info_hw_volume,
		.get = snd_falconi2s_get_hw_play_volume,
		.put = snd_falconi2s_put_hw_play_volume,
		.private_value = 0,

},
{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "Capture Volume",
		.info = snd_falconi2s_info_hw_volume,
		.get = snd_falconi2s_get_hw_capture_volume,
		.put = snd_falconi2s_put_hw_capture_volume,
		.private_value = 0,
},
};
  
/* hardware definition */
static struct snd_pcm_hardware snd_falconi2s_playback_hw = {
	.info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP_VALID),
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	//this value means both of 44100 and 48000 can work
	.rates = SNDRV_PCM_RATE_48000,
	.rate_min = 48000,
	.rate_max = 48000,
	.channels_min = 2,
	.channels_max = 2,
	//actually total length should less than 4096*1024.
	.buffer_bytes_max = P_PERIOD_BYTE * P_PERIOD_MAX,
	.period_bytes_min = P_PERIOD_BYTE,
	.period_bytes_max = P_PERIOD_BYTE,
	.periods_min =	  P_PERIOD_MIN,
	.periods_max =	  P_PERIOD_MAX,
};

/* hardware definition */
static struct snd_pcm_hardware snd_falconi2s_capture_hw = {
	.info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP_VALID),
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.rates = SNDRV_PCM_RATE_48000,
	.rate_min = 48000,
	.rate_max = 48000,
	.channels_min = 2,
	.channels_max = 2,
	//actually total length should less than 4096*1024.
	.buffer_bytes_max = P_PERIOD_BYTE * P_PERIOD_MAX,
	.period_bytes_min = P_PERIOD_BYTE,
	.period_bytes_max = P_PERIOD_BYTE,
	.periods_min =	  P_PERIOD_MIN,
	.periods_max =	  P_PERIOD_MAX,

};

  /* open callback */
static int snd_falconi2s_playback_open(struct snd_pcm_substream *substream)
{
	struct sm768chip *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	dbg_msg("snd_falconi2s_playback_open\n");

	
	runtime->hw = snd_falconi2s_playback_hw;
	/* set the pointer value of substream field in the chip record at 
	 * open callback to hold the current running substream pointer */
	chip->play_substream = substream;
	
	return 0;
}

  /* close callback */
static int snd_falconi2s_playback_close(struct snd_pcm_substream *substream)
{
	struct sm768chip *chip = snd_pcm_substream_chip(substream);
	/* the hardware-specific codes will be here */
	dbg_msg("snd_falconi2s_playback_close\n");
	/* reset the pointer value of substream field in the chip record at close callback */
	chip->play_substream = NULL;			 
	return 0;

}

  /* open callback */
static int snd_falconi2s_capture_open(struct snd_pcm_substream *substream)
{
	struct sm768chip *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;

	runtime->hw = snd_falconi2s_capture_hw;

	/* set the pointer value of substream field in the chip record at open callback to hold the current running substream pointer */
	chip->capture_substream = substream;

	
	return 0;
}

  /* close callback */
static int snd_falconi2s_capture_close(struct snd_pcm_substream *substream)
{
	struct sm768chip *chip = snd_pcm_substream_chip(substream);
	/* the hardware-specific codes will be here */
	/* reset the pointer value of substream field in the chip record at close callback */
	chip->capture_substream = NULL;
	return 0;

}

  /* hw_params callback */
static int snd_falconi2s_pcm_hw_params(struct snd_pcm_substream *substream,
                               struct snd_pcm_hw_params *hw_params)
{
	dbg_msg("snd_falconi2s_pcm_hw_params,malloc:%d\n",params_buffer_bytes(hw_params));
	   
	return snd_pcm_lib_malloc_pages(substream,
                                     params_buffer_bytes(hw_params));
}

  /* hw_free callback */
static int snd_falconi2s_pcm_hw_free(struct snd_pcm_substream *substream)
{
	dbg_msg("snd_falconi2s_pcm_hw_free\n");
	return snd_pcm_lib_free_pages(substream);
}

  /* prepare callback */
static int snd_falconi2s_pcm_prepare(struct snd_pcm_substream *substream)
{

	struct snd_pcm_runtime *runtime __attribute__((unused)) = substream->runtime;

	struct sm768chip *chip = snd_pcm_substream_chip(substream);
	dbg_msg("snd_falconi2s_pcm_prepare\n");

	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		chip->ppointer = 0;
	else
		chip->cpointer = 0;
	

	dbg_msg("runtime->rate:%d\n",runtime->rate);
	dbg_msg("runtime->buffer_size:%ld\n",runtime->buffer_size);
	dbg_msg("runtime->periods:%d\n",runtime->periods);
	dbg_msg("runtime->period_size:%ld\n",runtime->period_size);
	dbg_msg("runtime->frame_bits:%d\n",runtime->frame_bits);
	dbg_msg("runtime->dma_bytes:%ld\n",runtime->dma_bytes);

	return 0;
}

/* trigger callback */
static int snd_falconi2s_pcm_playback_trigger(struct snd_pcm_substream *substream,
                                    int cmd)
{
	struct sm768chip *chip = snd_pcm_substream_chip(substream);
	dbg_msg("snd_falconi2s_pcm_trigger\n");
	dbg_msg("substream:%p\n",substream);

	
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:			
		dbg_msg("PLAY SNDRV_PCM_TRIGGER_START\n");
		memset_io(chip->pvReg + SRAM_OUTPUT_BASE, 0, SRAM_OUTPUT_SIZE);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		dbg_msg("PLAY SNDRV_PCM_TRIGGER_STOP\n");
		break;
	default:
		return -EINVAL;
	}
	
	
	return 0;
}


 static int snd_falconi2s_pcm_capture_trigger(struct snd_pcm_substream *substream,
									  int cmd)
 {
  	struct sm768chip *chip = snd_pcm_substream_chip(substream);
	dbg_msg("snd_falconi2s_pcm_trigger\n");
	dbg_msg("substream:%p\n",substream);

	

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:		
		dbg_msg("CAPTURE SNDRV_PCM_TRIGGER_START\n");
		memset_io(chip->pvReg + SRAM_INPUT_BASE, 0, SRAM_INPUT_SIZE);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		dbg_msg("CAPTURE SNDRV_PCM_TRIGGER_STOP\n");
		break;
	default:
		return -EINVAL;
	}

	
	return 0;
 }

 static int snd_falconi2s_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
 {
	 
	  if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		  return snd_falconi2s_pcm_playback_trigger(substream, cmd);
	  else
		  return snd_falconi2s_pcm_capture_trigger(substream, cmd);
 }



  /* pointer callback */
  static snd_pcm_uframes_t
snd_falconi2s_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct sm768chip *chip = snd_pcm_substream_chip(substream);
	snd_pcm_uframes_t value = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		value = bytes_to_frames(substream->runtime, chip->ppointer);
	else
		value = bytes_to_frames(substream->runtime, chip->cpointer);
	
	return value;
}

  /* operators */
static struct snd_pcm_ops snd_falconi2s_playback_ops = {
          .open =        snd_falconi2s_playback_open,
          .close =       snd_falconi2s_playback_close,
          .ioctl =       snd_pcm_lib_ioctl,
          .hw_params =   snd_falconi2s_pcm_hw_params,
          .hw_free =     snd_falconi2s_pcm_hw_free,
          .prepare =     snd_falconi2s_pcm_prepare,
          .trigger =     snd_falconi2s_pcm_trigger,
          .pointer =     snd_falconi2s_pcm_pointer,
  };

  /* operators */
static struct snd_pcm_ops snd_falconi2s_capture_ops = {
          .open =        snd_falconi2s_capture_open,
          .close =       snd_falconi2s_capture_close,
          .ioctl =       snd_pcm_lib_ioctl,
          .hw_params =   snd_falconi2s_pcm_hw_params,
          .hw_free =     snd_falconi2s_pcm_hw_free,
          .prepare =     snd_falconi2s_pcm_prepare,
          .trigger =     snd_falconi2s_pcm_trigger,
          .pointer =     snd_falconi2s_pcm_pointer,
  };




static int snd_falconi2s_free(struct sm768chip *chip)
{
	/* will be implemented later... */
	dbg_msg("snd_falconi2s_free!\n");

	return 0;
}

/* component-destructor
* (see "Management of Cards and Components")
*/
static int snd_falconi2s_dev_free(struct snd_device *device)
{
	return snd_falconi2s_free(device->device_data);
}


static int snd_smi_play_copy_data(struct sm768chip *chip,int sramTxSection)
{
		
	struct snd_pcm_runtime *play_runtime;
	struct snd_pcm_substream *play_substream;

	play_substream = chip->play_substream;

	if(play_substream == NULL)
		memset_io(chip->pvReg + SRAM_OUTPUT_BASE + SRAM_SECTION_SIZE * sramTxSection, 0x00, P_PERIOD_BYTE);
	else{
		play_runtime = play_substream->runtime;

		if (play_runtime->dma_area == NULL) 
			return 0;

		memcpy_toio(chip->pvReg + SRAM_OUTPUT_BASE + SRAM_SECTION_SIZE * sramTxSection, play_runtime->dma_area + chip->ppointer, P_PERIOD_BYTE);
		chip->ppointer+= P_PERIOD_BYTE;
		chip->ppointer%= ((play_runtime->periods) * (P_PERIOD_BYTE));
		snd_pcm_period_elapsed(play_substream);
	}
	return 0;
}

static int snd_smi_capture_copy_data(struct sm768chip *chip,int sramTxSection)
{
		
	struct snd_pcm_runtime *capture_runtime;
	struct snd_pcm_substream *capture_substream;

	capture_substream = chip->capture_substream;

	if(capture_substream == NULL)	
		memset_io(chip->pvReg + SRAM_INPUT_BASE + SRAM_SECTION_SIZE * sramTxSection, 0x00,  P_PERIOD_BYTE);
		
	else{
		capture_runtime = capture_substream->runtime;

		if (capture_runtime->dma_area == NULL) 
			return 0;

		memcpy_fromio(capture_runtime->dma_area + chip->cpointer, chip->pvReg + SRAM_INPUT_BASE + SRAM_SECTION_SIZE * sramTxSection,  P_PERIOD_BYTE);
		chip->cpointer+= P_PERIOD_BYTE;
		chip->cpointer%= ((capture_runtime->periods) * (P_PERIOD_BYTE));		
		snd_pcm_period_elapsed(capture_substream);
	}
	return 0;
}


/*
 * interrupt handler
 */
static irqreturn_t snd_smi_interrupt(int irq, void *dev_id)
{
	
	struct sm768chip *chip = dev_id;

	int sramTxSection = 0; 

	if(hw768_check_iis_interrupt())
	{
		unsigned long iParameter = 0;
		iisClearRawInt(); //clear int

		iParameter = iisDmaPointer();
		
		//SRAM is logically divided into 2 portions (1024 byte each)
		//Check I2S DMA pointer to find out which portion is active.
		if(iParameter >= 255)
			//Fill top half SRAM if lower half is active
			sramTxSection = 0;
		else
			//Fill lower half SRAM if top half is active
			sramTxSection = 1;

		snd_smi_play_copy_data(chip,sramTxSection);   
		snd_smi_capture_copy_data(chip,sramTxSection);

		return IRQ_HANDLED;
	}
	else
		return IRQ_NONE;
}

  /* chip-specific constructor
   * (see "Management of Cards and Components")
   */
static int snd_falconi2s_create(struct snd_card *card,
                                         struct drm_device *dev,
                                         struct sm768chip **smichip)
{
	int err;
	int sample_rate = 44100;
	struct pci_dev *pdev;
	struct smi_device *smi_device = dev->dev_private;
	struct sm768chip *chip;
	static struct snd_device_ops ops = {
		.dev_free = snd_falconi2s_dev_free,
	};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	pdev = to_pci_dev(dev->dev);
#else
	pdev = dev->pdev;
#endif

	*smichip = NULL;

	/* allocate a chip-specific data with zero filled */
	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	chip->card = card;

	err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, chip, &ops);
	if (err < 0) {
		snd_falconi2s_free(chip);
		return err;
	}
	
	//map register
	chip->vidreg_start = smi_device->rmmio_base;
	chip->vidreg_size = smi_device->rmmio_size;
	dbg_msg("Audio MMIO phyAddr = 0x%lx\n",chip->vidreg_start);

	chip->pvReg = smi_device->rmmio;
	dbg_msg("Audio MMIO virtual addr = %p\n",chip->pvReg);
	

	//map video memory. 
	chip->vidmem_start = smi_device->mc.vram_base;
	chip->vidmem_size = 0x200000;   // change the video memory temperarily
	dbg_msg("video memory phyAddr = 0x%lx, size = (Dec)%ld bytes\n",
		chip->vidmem_start,chip->vidmem_size);

#ifdef NO_WC
	chip->pvMem = ioremap(chip->vidmem_start,chip->vidmem_size);
#else
	chip->pvMem = ioremap_wc(chip->vidmem_start,chip->vidmem_size);
#endif


	if(!chip->pvMem){
		dev_err(&pdev->dev, "Map memory failed\n");
		snd_falconi2s_free(chip);
		err = -EFAULT;
		return err;
	}else{
		dbg_msg("Audio video memory virtual addr = %p\n",chip->pvMem);
	}
	//above 

	chip->irq = pdev->irq;

	dbg_msg("Audio pci irq :%d\n",chip->irq);
	
	if (ddk768_getCrystalType())
		sample_rate = 48000;
	else
		sample_rate = 44100;

	if(SM768_AudioInit(SAMPLE_BITS, sample_rate)) {
		dev_err(&pdev->dev, "Audio init failed!\n");
		snd_falconi2s_free(chip);
		return -1;
	}
	SM768_AudioStart();

	//clear SRAM
	memset_io(chip->pvReg + SRAM_OUTPUT_BASE, 0, SRAM_TOTAL_SIZE);
	
	chip_irq_id=chip;/*Record chip_irq_id which will use in free_irq*/
	dbg_msg("chip_irq_id=%p\n", chip_irq_id);

	iisClearRawInt();//clear int

	//Setup ISR. The ISR will move more data from DDR to SRAM.
	
	if (request_irq(pdev->irq, snd_smi_interrupt, IRQF_SHARED,
		KBUILD_MODNAME, chip_irq_id)) {
		dev_err(&pdev->dev, "unable to grab IRQ %d\n", pdev->irq);
		snd_falconi2s_free(chip);
		return -EBUSY;
	}
	sb_IRQUnmask(SB_IRQ_VAL_I2S); 

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,18,0)
	snd_card_set_dev(card, &pdev->dev);
#endif

	*smichip = chip;
	return 0;
}



int smi_audio_init(struct drm_device *dev)
{
	int idx, err;
	struct pci_dev *pdev;
	struct smi_device *sdev = dev->dev_private;
	struct snd_pcm *pcm;
	struct snd_card *card;
	struct sm768chip *chip;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	pdev = to_pci_dev(dev->dev);
#else
	pdev = dev->pdev;
#endif

	if(audio_en == 1)
		use_wm8978 = 0;
	else if(audio_en == 2)
		use_wm8978 = 1;
	
	err = snd_card_new(&pdev->dev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1, THIS_MODULE, 0, &card); 


	
	if (err < 0)
		return err;

	err = snd_falconi2s_create(card, dev, &chip);
	if (err < 0) {
		snd_card_free(card);
      		return err;
	}

	strcpy(card->driver, "smi-audio");
	strcpy(card->shortname, "smi-audio");
	strcpy(card->longname, "SiliconMotion Audio");

	snd_pcm_new(card,"smiaudio_pcm",0,1,1,&pcm);
	pcm->private_data = chip;

      
	/* set operators */
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
                          &snd_falconi2s_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,
                          &snd_falconi2s_capture_ops);
	  
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
					      &pdev->dev,
#else
					      snd_dma_pci_data(pdev),
#endif
						P_PERIOD_BYTE*P_PERIOD_MIN, P_PERIOD_BYTE*P_PERIOD_MAX);

	strcpy(card->mixername, "SiliconMotion Audio Mixer Control");
	
	for (idx = 0; idx < ARRAY_SIZE(falconi2s_vol); idx++) {
		if ((err = snd_ctl_add(card,snd_ctl_new1(&falconi2s_vol[idx], chip))) < 0)
		{
			return err;
		}
	}

	
	err = snd_card_register(card);
	if (err < 0) {
		snd_card_free(card);
		return err;
	}

	sdev->card = card;

	return 0;

}

void smi_audio_remove(struct drm_device *dev)
{
	struct pci_dev *pdev;
	struct smi_device *sdev = dev->dev_private;
	struct snd_card *card = sdev->card;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	pdev = to_pci_dev(dev->dev);
#else
	pdev = dev->pdev;
#endif

	
	dbg_msg("perpare to free irq, pci irq:%u, chip_irq_id=0x%p\n", pdev->irq, chip_irq_id);
	if(pdev->irq){				
		free_irq(pdev->irq, chip_irq_id);
		dbg_msg("free irq\n");
	}
	SM768_AudioStop();
	SM768_AudioDeinit();

	snd_card_free(card);
	
	iounmap(chip_irq_id->pvMem);
}

void smi_audio_resume()
{

	int sample_rate = 44100;
	
	if (ddk768_getCrystalType())
			sample_rate = 48000;
		else
			sample_rate = 44100;
	
	SM768_AudioInit(SAMPLE_BITS, sample_rate);
	SM768_AudioStart();

	sb_IRQUnmask(SB_IRQ_VAL_I2S);
}


void smi_audio_suspend()
{
	SM768_AudioStop();
	SM768_AudioDeinit();
}


