// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.

#include <drm/drm_modes.h>
#include "ddk770/ddk770_reg.h"
#include "ddk770/ddk770_help.h"
#include "ddk770/ddk770_mode.h"
#include "ddk770/ddk770_hardware.h"
#include "ddk770/ddk770_display.h"
#include "ddk770/ddk770_power.h"
#include "ddk770/ddk770_cursor.h"
#include "ddk770/ddk770_video.h"
#include "ddk770/ddk770_hdmi.h"
#include "ddk770/ddk770_chip.h"
#include "ddk770/ddk770_pwm.h"
#include "ddk770/ddk770_swi2c.h"
#include "ddk770/ddk770_hwi2c.h"
#include "ddk770/ddk770_iis.h"
#include "ddk770/ddk770_dp.h"
#include "ddk770/ddk770_intr.h"
#include "ddk770/ddk770_wm8978.h"
#include "hw770.h"
#include "smi_snd.h"
#include "smi_ver.h"

extern int lcd_scale;
extern int pwm_ctrl;

static mode_parameter_t hw770_convert_drm_mode_to_ddk_mode(struct drm_display_mode mode)
{
	mode_parameter_t modeP;

    modeP.horizontal_total = mode.htotal;
    modeP.horizontal_display_end = mode.hdisplay;
    modeP.horizontal_sync_start = mode.hsync_start;
    modeP.horizontal_sync_width = mode.hsync_end - mode.hsync_start;
    modeP.horizontal_sync_polarity = mode.flags & DRM_MODE_FLAG_PHSYNC ? POS : NEG;

    /* Vertical timing. */
    modeP.vertical_total = mode.vtotal;
    modeP.vertical_display_end = mode.vdisplay;
    modeP.vertical_sync_start = mode.vsync_start;
    modeP.vertical_sync_height = mode.vsync_end - mode.vsync_start;
    modeP.vertical_sync_polarity = mode.flags & DRM_MODE_FLAG_PVSYNC ? POS : NEG;

    /* Refresh timing. */
    modeP.pixel_clock = mode.clock * 1000;
    modeP.horizontal_frequency = 0;
    modeP.vertical_frequency = 0;
    
    /* Clock Phase. This clock phase only applies to Panel. */
    modeP.clock_phase_polarity = NEG;

	return modeP;
}


void hw770_enable_lvds(int channels)
{
	
}	



void hw770_setDisplayPlaneDisableOnly(
   disp_control_t dispControl /* Channel 0 or Channel 1) */
)
{
    unsigned long ulDispCtrlAddr;
    unsigned long ulDispCtrlReg;

	ulDispCtrlAddr = DISPLAY_CTRL + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
    ulDispCtrlReg = peekRegisterDWord(ulDispCtrlAddr);

	
 	ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, PLANE, DISABLE)|        
                    FIELD_SET(0, DISPLAY_CTRL, DATA_PATH, EXTENDED); 

	 pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
   
}




void hw770_suspend(struct smi_770_register * pSave)
{
	;
	// int i;

	// pSave->clock_enable = peekRegisterDWord(CLOCK_ENABLE);
	// for (i = 0; i < 3; i++)
	// 	pSave->pll_ctrl[i] = peekRegisterDWord(MCLK_PLL + i * 4);

	// for (i = 0; i < 10; i++)
	// 	pSave->primary_display_ctrl[i] = peekRegisterDWord(DISPLAY_CTRL + i * 4);
	// pSave->lvds_ctrl = peekRegisterDWord(LVDS_CTRL2);
	// for (i = 0; i < 4; i++)
	// 	pSave->primary_hwcurs_ctrl[i] = peekRegisterDWord(HWC_CONTROL + i * 4);

	// for (i = 0; i < 10; i++)
	// 	pSave->secondary_display_ctrl[i] = peekRegisterDWord(0x8000 + DISPLAY_CTRL + i * 4);
	// for (i = 0; i < 4; i++)
	// 	pSave->secondary_hwcurs_ctrl[i] = peekRegisterDWord(0x8000 + HWC_CONTROL + i * 4);


}

void hw770_resume(struct smi_770_register * pSave)
{
	if (ddk770_GetChipRev() == CHIP_REV_AB) {
		pokeRegisterDWord(0x130, 0x78000000);
	}

	ddk770_iis_Init();
	hw770_init_hdmi();
	hw770_init_dp();

	// int i;

	// pokeRegisterDWord(CLOCK_ENABLE, pSave->clock_enable);
	// for (i = 0; i < 3; i++)
	// 	pokeRegisterDWord(MCLK_PLL + i * 4, pSave->pll_ctrl[i]);

	// for (i = 0; i < 10; i++)
	// 	pokeRegisterDWord(DISPLAY_CTRL + i * 4, pSave->primary_display_ctrl[i]);
	// pokeRegisterDWord(LVDS_CTRL2, pSave->lvds_ctrl);
	// for (i = 0; i < 4; i++)
	// 	pokeRegisterDWord(HWC_CONTROL + i * 4, pSave->primary_hwcurs_ctrl[i]);

	// for (i = 0; i < 10; i++)
	// 	pokeRegisterDWord(0x8000 + DISPLAY_CTRL + i * 4, pSave->secondary_display_ctrl[i]);
	// for (i = 0; i < 4; i++)
	// 	pokeRegisterDWord(0x8000 + HWC_CONTROL + i * 4, pSave->secondary_hwcurs_ctrl[i]);

}
void hw770_set_base(disp_control_t dispControl,int pitch,int base_addr)
{	

	unsigned int ulFBBaseAddr, ulFBPitchAddr, ulFBPitchReg,LineOffset;

	LineOffset = alignLineOffset(pitch);

	ulFBBaseAddr = FB_ADDRESS + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);

	/* Frame buffer base */
	pokeRegisterDWord(ulFBBaseAddr,
	      FIELD_SET(0, FB_ADDRESS, STATUS, PENDING)
	    | FIELD_VALUE(0, FB_ADDRESS, ADDRESS, base_addr));


	
	/* Pitch value (Hardware people calls it Offset) */

	ulFBPitchAddr = FB_WIDTH + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
	
	ulFBPitchReg = peekRegisterDWord(ulFBPitchAddr);

	pokeRegisterDWord(ulFBPitchAddr, FIELD_VALUE(ulFBPitchReg, FB_WIDTH, OFFSET, LineOffset));	



}



void hw770_init_hdmi(void)
{
	ddk770_HDMI_Init(0);
	ddk770_HDMI_Init(1);
	ddk770_HDMI_Init(2);
}

void hw770_init_dp(void)
{
	DP_Init(0);
	DP_Init(1);
}

int hw770_set_hdmi_mode(logicalMode_t *pLogicalMode, struct drm_display_mode mode, bool isHDMI,int hdmi_index)
{
	int ret = 1;
	mode_parameter_t modeParam;

	ddk770_HDMI_Set_Channel(hdmi_index,pLogicalMode->dispCtrl);
	ddk770_setSingleViewOn(pLogicalMode->dispCtrl);

	printk("hdmi_index= %x, DC=%x, 0x3ffc value=%x\n",hdmi_index,pLogicalMode->dispCtrl,peekRegisterDWord(0x3ffc));
	
	if(pLogicalMode->valid_edid)
		modeParam = hw770_convert_drm_mode_to_ddk_mode(mode);
	ret = ddk770_HDMI_Set_Mode(hdmi_index, pLogicalMode,&modeParam, isHDMI);
	return ret;
}

int hw770_hdmi_vga_mode(hdmi_index index)
{
	int ret = 1;
	mode_parameter_t modeParam;
	logicalMode_t logicalMode;

	logicalMode.valid_edid = false;

	logicalMode.baseAddress = 0;
	logicalMode.x = 640;
	logicalMode.y = 480;
	logicalMode.bpp = 32;
	logicalMode.dispCtrl = (disp_control_t)index;
	logicalMode.hz = 60;
	logicalMode.pitch = 0;;

	
	ddk770_setMode(&logicalMode);

	ddk770_HDMI_Set_Channel(index,logicalMode.dispCtrl);
	ddk770_setSingleViewOn(logicalMode.dispCtrl);

	ret = ddk770_HDMI_Set_Mode(index, &logicalMode,&modeParam, 1);

	return ret;
	

}

int hw770_set_dp_mode(logicalMode_t *pLogicalMode, struct drm_display_mode mode, int dp_index)
{
	int ret = 1;
	mode_parameter_t modeParam;

	DP_Set_Channel(dp_index,pLogicalMode->dispCtrl);

	ddk770_setSingleViewOn(pLogicalMode->dispCtrl);
	
	printk("dp_index= %x, DC=%x, 0x3ffc value=%x\n",dp_index,pLogicalMode->dispCtrl,peekRegisterDWord(0x3ffc));
	
	if(pLogicalMode->valid_edid)
		modeParam = hw770_convert_drm_mode_to_ddk_mode(mode);
	ret = DP_Setmode(dp_index, pLogicalMode,&modeParam);
	return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
int hw770_en_dis_interrupt(int status, int pipe)
{
    int channel_mask = 0;

    switch (pipe) {
        case CHANNEL0_CTRL:
            channel_mask = (status == 0) ? FIELD_SET(0, INT_MASK, CHANNEL0_VSYNC, DISABLE) :
                                            FIELD_SET(0, INT_MASK, CHANNEL0_VSYNC, ENABLE);
            break;
        case CHANNEL1_CTRL:
            channel_mask = (status == 0) ? FIELD_SET(0, INT_MASK, CHANNEL1_VSYNC, DISABLE) :
                                            FIELD_SET(0, INT_MASK, CHANNEL1_VSYNC, ENABLE);
            break;
        case CHANNEL2_CTRL:
            channel_mask = (status == 0) ? FIELD_SET(0, INT_MASK, CHANNEL2_VSYNC, DISABLE) :
                                            FIELD_SET(0, INT_MASK, CHANNEL2_VSYNC, ENABLE);
            break;
        default:
            return -1;
    }
    pokeRegisterDWord(INT_MASK, channel_mask);

    return 0;
}
#else
int hw770_en_dis_interrupt(int status)
	{
		if(status == 0)
		{
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, CHANNEL2_VSYNC, DISABLE));
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, CHANNEL1_VSYNC, DISABLE));
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, CHANNEL0_VSYNC, DISABLE)); 
		}
		else
		{
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, CHANNEL2_VSYNC, ENABLE));
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, CHANNEL1_VSYNC, ENABLE));
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, CHANNEL0_VSYNC, ENABLE));  
		}
		return 0;
	}

#endif

void hw770_HDMI_Disable_Output(hdmi_index index)
{
	
	ddk770_HDMI_Disable_Output(index);
	ddk770_HDMI_Clear_Channel(index);
	ddk770_HDMI_Standby(index);
	hw770_hdmi_interrupt_enable(index,0);
}

void hw770_DP_Disable_Output(dp_index index)
{
	DP_Clear_Channel(index);
	DP_Disable_Output(index);
}

void hw770_HDMI_Clear_Channel(hdmi_index index)
{
    if(ddk770_HDMI_Get_Channel(index) != 0)  //DC will be 0x1 , 0x2 or 0x4
    {
		ddk770_HDMI_Disable_Output(index);
		ddk770_HDMI_Clear_Channel(index);
		ddk770_HDMI_Standby(index);

    }

}

int hw770_get_hdmi_edid(hdmi_index index, unsigned char *pEDIDBuffer)
{
	u16 edidSize = 0;
	
	ddk770_HDMI_Read_EDID(index, pEDIDBuffer, &edidSize);
	
	return edidSize;

}

int hw770_get_dp_edid(dp_index index, unsigned char *pEDIDBuffer)
{
	u16 edidSize = 0;
	
	DP_Read_EDID(index, pEDIDBuffer, &edidSize);
	
	return edidSize;

}

void hw770_DP_Clear_Channel(dp_index index)
{
	 if(DP_Get_Channel(index) != 0)
	        DP_Clear_Channel(index);
}

int hw770_check_iis_interrupt(void)
{

	 unsigned long value;
		
	value = peekRegisterDWord(INT_STATUS);

	
   if (FIELD_VAL_GET(value, INT_STATUS, I2S) == INT_STATUS_I2S_ACTIVE)
	 return true;
   else	
	 return false;
}

int hw770_check_vsync_interrupt(int path)
{

	unsigned long value1,value2;
		
	value1 = peekRegisterDWord(RAW_INT);
	value2 = peekRegisterDWord(INT_MASK);

	if(path == CHANNEL0_CTRL)
	{
	   if ((FIELD_VAL_GET(value1, RAW_INT, CHANNEL0_VSYNC) == RAW_INT_CHANNEL0_VSYNC_ACTIVE)
			&&(FIELD_VAL_GET(value2, INT_MASK, CHANNEL0_VSYNC) == INT_MASK_CHANNEL0_VSYNC_ENABLE))
	    {
		return true;
		}
	 }else if (path == CHANNEL1_CTRL){
		if ((FIELD_VAL_GET(value1, RAW_INT, CHANNEL1_VSYNC) == RAW_INT_CHANNEL1_VSYNC_ACTIVE)
			&&(FIELD_VAL_GET(value2, INT_MASK, CHANNEL1_VSYNC) == INT_MASK_CHANNEL1_VSYNC_ENABLE))
	    {
		return true;
		}
	}else{
		if ((FIELD_VAL_GET(value1, RAW_INT, CHANNEL2_VSYNC) == RAW_INT_CHANNEL2_VSYNC_ACTIVE)
			&&(FIELD_VAL_GET(value2, INT_MASK, CHANNEL2_VSYNC) == INT_MASK_CHANNEL2_VSYNC_ENABLE))
		{
		return true;
		}

	}

	return false;
}

void hw770_clear_vsync_interrupt(int path)
{
	
	unsigned long value;
	
	value = peekRegisterDWord(RAW_INT);

	if (path == CHANNEL0_CTRL)
	{
		pokeRegisterDWord(RAW_INT, FIELD_SET(value, RAW_INT, CHANNEL0_VSYNC, CLEAR));
	}
	else if (path == CHANNEL1_CTRL)
	{
		pokeRegisterDWord(RAW_INT, FIELD_SET(value, RAW_INT, CHANNEL1_VSYNC, CLEAR));
	}
	else
	{
		pokeRegisterDWord(RAW_INT, FIELD_SET(value, RAW_INT, CHANNEL2_VSYNC, CLEAR));
	}
}

long hw770_setMode(logicalMode_t *pLogicalMode, struct drm_display_mode mode)
{
	if (!pLogicalMode->valid_edid)
		return ddk770_setMode(pLogicalMode);
	else
	{	
		mode_parameter_t ModeParam;
		ModeParam = hw770_convert_drm_mode_to_ddk_mode(mode);
		return ddk770_setCustomMode(pLogicalMode, &ModeParam);
	}
	
}

int hw770_hdmi_detect(hdmi_index hdmi_index)
{

	return ddk770_HDMI_HPD_Detect(hdmi_index);
	
}

int hw770_dp_detect(dp_index dp_index)
{

	return DP_HPD_Detect(dp_index);	

}

void ddk770_disable_IntMask(void)
{
    pokeRegisterDWord(INT_MASK, 0);
}

void hw770_SetPixelClockFormat(disp_control_t dispControl,unsigned int is_half)
{
    
}

void hw770_setgamma(disp_control_t dispCtrl, unsigned long enable)
{
	unsigned long value;
	unsigned long regCtrl;

	regCtrl = DISPLAY_CTRL + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

	value = peekRegisterDWord(regCtrl);
	
	if (enable)
	    value = FIELD_SET(value, DISPLAY_CTRL, GAMMA, ENABLE);
	else
	    value = FIELD_SET(value, DISPLAY_CTRL, GAMMA, DISABLE);

	   	
	pokeRegisterDWord(regCtrl, value);    
}

void hw770_load_lut(disp_control_t dispCtrl, int size, u8 lut_r[], u8 lut_g[], u8 lut_b[])
{
	unsigned int i, v;
	unsigned long regCtrl;


	regCtrl = PALETTE_RAM + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

	for (i = 0; i < size; i++) {
		v = (lut_r[i] << 16);
		v |= (lut_g[i] << 8);
		v |= lut_b[i];
		pokeRegisterDWord(regCtrl + (i * 4), v);
	}
}



long hw770_AdaptI2CInit(struct smi_connector *smi_connector)
{
	if (smi_connector->base.connector_type == DRM_MODE_CONNECTOR_DVID ||
	    smi_connector->base.connector_type == DRM_MODE_CONNECTOR_HDMIB ||
	    smi_connector->base.connector_type == DRM_MODE_CONNECTOR_HDMIA) {
#if USE_I2C_ADAPTER
		return ddk770_HDMI_AdaptHWI2CInit(smi_connector);
#endif
	} else if (smi_connector->base.connector_type ==
			   DRM_MODE_CONNECTOR_DisplayPort ||
		   smi_connector->base.connector_type ==
			   DRM_MODE_CONNECTOR_eDP) {
#if USE_I2C_ADAPTER
		return ddk770_DP_AdaptHWI2CInit(smi_connector);
#endif
	}
	return 0;
#if 0
    if(hwi2c_en)
    {
        smi_connector->i2c_hw_enabled = 1;
    }
    else
    {
        smi_connector->i2c_hw_enabled = 0;      
    }

    if(smi_connector->i2c_hw_enabled)
    {
		return 0;
       
    }
    else
    {
		return 0;
    
    }
#endif
}


long hw770_AdaptI2CCleanBus(struct drm_connector *connector)
{
	return 0;
    
}

void hw770_AudioInit(unsigned long wordLength, unsigned long sampleRate)
{
    
	// Set up I2S and GPIO registers to transmit/receive data.
    ddk770_iisOpen(wordLength, sampleRate);
	//Set I2S to DMA 256 DWord from SRAM starting at location 0 of SRAM
	ddk770_iisTxDmaSetup(0,SRAM_SECTION_SIZE);
}

void hw770_AudioStart(void)
{
	int i = 0;
	
	for (i = 0 ; i <3 ;i++) 
	{
    	ddk770_HDMI_Audio_Unmute(i);
	}

	for (i = 0 ; i <2 ;i++) 
	{
		DP_Audio_UnMute(i);

	}

	ddk770_iisStart();
}

void hw770_AudioStop(void)
{
	int i;
	for (i = 0 ; i <3 ;i++) 
	{
		ddk770_HDMI_Audio_Mute(i);

	}

	for (i = 0 ; i <2 ;i++) 
	{
		DP_Audio_Mute(i);

	}
		ddk770_iisStop();

}

void hw770_AudioDeinit(void)
{
	ddk770_iisStop();

	ddk770_iisClose();

	ddk770_sb_IRQMask(SM770_SB_IRQ_VAL_I2S);	
	
	ddk770_iisClearRawInt();

    ddk770_enableI2S(0); 
}


int hw770_check_pnp_interrupt(hdmi_index index)
{
	int ret = 0;
	ret = hdmiISR(index);
	return ret;
}

void hw770_hdmi_interrupt_enable(hdmi_index index,int enable)
{
	ddk770_HDMI_interrupt_enable(index,enable);
}

void hw770_get_current_fb_info(disp_control_t index, struct smi_770_fb_info *fb_info)
{
	unsigned int baseAddr = FB_ADDRESS + (index > 1 ? CHANNEL_OFFSET2 : index * CHANNEL_OFFSET);
	fb_info->fb_addr = peekRegisterDWord(baseAddr);

	baseAddr = FB_WIDTH + (index > 1 ? CHANNEL_OFFSET2 : index * CHANNEL_OFFSET);
	fb_info->fb_pitch = peekRegisterDWord(baseAddr);
}

void hw770_set_current_pitch(disp_control_t index, struct smi_770_fb_info *fb_info)
{
	unsigned int baseAddr = FB_ADDRESS + (index > 1 ? CHANNEL_OFFSET2 : index * CHANNEL_OFFSET);
	pokeRegisterDWord(baseAddr, fb_info->fb_addr);

	baseAddr = FB_WIDTH + (index > 1 ? CHANNEL_OFFSET2 : index * CHANNEL_OFFSET);
	pokeRegisterDWord(baseAddr, fb_info->fb_pitch);
}

void hw770_i2c_reset_busclear(hdmi_index index)
{
	ddk770_i2c_reset_busclear(index);

}

int hw770_dp_check_sink_status(dp_index index)
{
	int ret;
	ret = DP_Check_Sink_Status(index);

	return ret;
}

int hw770_get_current_mode_width(disp_control_t index)
{
	int width;
	unsigned int baseAddr = HORIZONTAL_TOTAL + (index > 1 ? CHANNEL_OFFSET2 : index * CHANNEL_OFFSET);
	width = (peekRegisterDWord(baseAddr)&0xfff) + 1;

	return width;
}
