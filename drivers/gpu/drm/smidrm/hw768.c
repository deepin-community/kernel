// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.


#include <drm/drm_modes.h>

#include "ddk768/ddk768_mode.h"
#include "ddk768/ddk768_help.h"
#include "ddk768/ddk768_reg.h"	
#include "ddk768/ddk768_display.h"
#include "ddk768/ddk768_2d.h"
#include "ddk768/ddk768_power.h"
#include "ddk768/ddk768_cursor.h"
#include "ddk768/ddk768_video.h"
#include "ddk768/ddk768_hdmi.h"
#include "ddk768/ddk768_pwm.h"
#include "ddk768/ddk768_swi2c.h"
#include "ddk768/ddk768_hwi2c.h"


#include "smi_ver.h"
#include "hw768.h"

extern int lcd_scale;
extern int pwm_ctrl;


struct smi_768_register{
	uint32_t clock_enable, pll_ctrl[3];
	
	uint32_t primary_display_ctrl[10], lvds_ctrl, primary_hwcurs_ctrl[4];
	uint32_t secondary_display_ctrl[10], secondary_hwcurs_ctrl[4];
};


static mode_parameter_t convert_drm_mode_to_ddk_mode(struct drm_display_mode mode)
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
    modeP.clock_phase_polarity = POS;

	return modeP;
}



void hw768_enable_lvds(int channels)
{

	if(channels == 1){	
		setSingleLVDS(0);

	}else{
		set48bitLVDS(0);
	}
	
	
	if(pwm_ctrl)
	{
		unsigned long pwm, divider, highCounter, lowCounter;

	     	pwm = pwm_ctrl & 0xf;
		divider = (pwm_ctrl & 0xf0) >> 4;
	    	highCounter = (pwm_ctrl & 0xfff00) >> 8;
		lowCounter = (pwm_ctrl & 0xfff00000) >> 20;

		ddk768_pwmOpen(pwm);
		ddk768_pwmStart(pwm, divider, highCounter, lowCounter, 0);
	}

	
}	



void ddk768_setDisplayPlaneDisableOnly(
   disp_control_t dispControl /* Channel 0 or Channel 1) */
)
{
    unsigned long ulDispCtrlAddr;
    unsigned long ulDispCtrlReg;

    ulDispCtrlAddr = (dispControl == CHANNEL0_CTRL)? DISPLAY_CTRL : (DISPLAY_CTRL+CHANNEL_OFFSET);
    ulDispCtrlReg = peekRegisterDWord(ulDispCtrlAddr);

	
 	ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, PLANE, DISABLE)|        
                    FIELD_SET(0, DISPLAY_CTRL, DATA_PATH, EXTENDED); 

	 pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
   
}




void hw768_suspend(struct smi_768_register * pSave)
{

	int i;

	pSave->clock_enable = peekRegisterDWord(CLOCK_ENABLE);
	for (i = 0; i < 3; i++)
		pSave->pll_ctrl[i] = peekRegisterDWord(MCLK_PLL + i * 4);

	for (i = 0; i < 10; i++)
		pSave->primary_display_ctrl[i] = peekRegisterDWord(DISPLAY_CTRL + i * 4);
	pSave->lvds_ctrl = peekRegisterDWord(LVDS_CTRL2);
	for (i = 0; i < 4; i++)
		pSave->primary_hwcurs_ctrl[i] = peekRegisterDWord(HWC_CONTROL + i * 4);

	for (i = 0; i < 10; i++)
		pSave->secondary_display_ctrl[i] = peekRegisterDWord(0x8000 + DISPLAY_CTRL + i * 4);
	for (i = 0; i < 4; i++)
		pSave->secondary_hwcurs_ctrl[i] = peekRegisterDWord(0x8000 + HWC_CONTROL + i * 4);


}

void hw768_resume(struct smi_768_register * pSave)
{

	int i;

	pokeRegisterDWord(CLOCK_ENABLE, pSave->clock_enable);
	for (i = 0; i < 3; i++)
		pokeRegisterDWord(MCLK_PLL + i * 4, pSave->pll_ctrl[i]);

	for (i = 0; i < 10; i++)
		pokeRegisterDWord(DISPLAY_CTRL + i * 4, pSave->primary_display_ctrl[i]);
	pokeRegisterDWord(LVDS_CTRL2, pSave->lvds_ctrl);
	for (i = 0; i < 4; i++)
		pokeRegisterDWord(HWC_CONTROL + i * 4, pSave->primary_hwcurs_ctrl[i]);

	for (i = 0; i < 10; i++)
		pokeRegisterDWord(0x8000 + DISPLAY_CTRL + i * 4, pSave->secondary_display_ctrl[i]);
	for (i = 0; i < 4; i++)
		pokeRegisterDWord(0x8000 + HWC_CONTROL + i * 4, pSave->secondary_hwcurs_ctrl[i]);

}
void hw768_set_base(int display,int pitch,int base_addr)
{	

	if(display == 0)
	{
		/* Frame buffer base */
	    pokeRegisterDWord((FB_ADDRESS),
	          FIELD_SET(0, FB_ADDRESS, STATUS, PENDING)
	        | FIELD_VALUE(0, FB_ADDRESS, ADDRESS, base_addr));

	    /* Pitch value (Hardware people calls it Offset) */
    	pokeRegisterDWord((FB_WIDTH), FIELD_VALUE(peekRegisterDWord(FB_WIDTH), FB_WIDTH, OFFSET, pitch));

		if(lcd_scale){
				pokeRegisterDWord((VIDEO_FB_WIDTH),
					   FIELD_VALUE(0, VIDEO_FB_WIDTH, WIDTH, pitch) |
					   FIELD_VALUE(0, VIDEO_FB_WIDTH, OFFSET, pitch));
		
			   pokeRegisterDWord(VIDEO_FB_ADDRESS,
					   FIELD_SET(0, VIDEO_FB_ADDRESS, STATUS, PENDING) |
					   FIELD_VALUE(0, VIDEO_FB_ADDRESS, ADDRESS, base_addr));

		}
	}
	else
	{
		/* Frame buffer base */
	    pokeRegisterDWord((FB_ADDRESS+CHANNEL_OFFSET),
	          FIELD_SET(0, FB_ADDRESS, STATUS, PENDING)
	        | FIELD_VALUE(0, FB_ADDRESS, ADDRESS, base_addr));

		
	    /* Pitch value (Hardware people calls it Offset) */	
	    pokeRegisterDWord((FB_WIDTH+CHANNEL_OFFSET),FIELD_VALUE(peekRegisterDWord(FB_WIDTH+CHANNEL_OFFSET), FB_WIDTH, OFFSET, pitch));

	}
}

int hw768_set_hdmi_mode(logicalMode_t *pLogicalMode, struct drm_display_mode mode, bool isHDMI)
{
	int ret = 1;
	mode_parameter_t modeParam;
	
	// set HDMI parameters
	HDMI_Disable_Output();

	if(pLogicalMode->valid_edid)
		modeParam = convert_drm_mode_to_ddk_mode(mode);
	ret = HDMI_Set_Mode(pLogicalMode,&modeParam,isHDMI);
	return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
int hw768_en_dis_interrupt(int status, int pipe)
{
	if(status == 0)
	{
		pokeRegisterDWord(INT_MASK, 	(pipe == CHANNEL0_CTRL) ? 
		FIELD_SET(0, INT_MASK, CHANNEL1_VSYNC, DISABLE):
		FIELD_SET(0, INT_MASK, CHANNEL0_VSYNC, DISABLE));
	}
	else
	{
		pokeRegisterDWord(INT_MASK, 	(pipe == CHANNEL1_CTRL) ? 
		FIELD_SET(0, INT_MASK, CHANNEL1_VSYNC, ENABLE):
		FIELD_SET(0, INT_MASK, CHANNEL0_VSYNC, ENABLE));
	}
	return 0;
}
#else
int hw768_en_dis_interrupt(int status)
	{
		if(status == 0)
		{
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, CHANNEL1_VSYNC, DISABLE));
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, CHANNEL0_VSYNC, DISABLE)); 
		}
		else
		{
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, CHANNEL1_VSYNC, ENABLE));
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, CHANNEL0_VSYNC, ENABLE));  
		}
		return 0;
	}

#endif

int hw768_get_hdmi_edid(unsigned char *pEDIDBuffer)
{
    int ret;
    enableHdmI2C(1);
    ret = HDMI_Read_Edid(pEDIDBuffer, 128);
    enableHdmI2C(0);

    return ret;
}

int hw768_check_iis_interrupt(void)
{

	unsigned long value;
		
	value = peekRegisterDWord(INT_STATUS);

	
    if (FIELD_VAL_GET(value, INT_STATUS, I2S) == INT_STATUS_I2S_ACTIVE)
		return true;
	else	
		return false;
}


int hw768_check_vsync_interrupt(int path)
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
	}else{
		if ((FIELD_VAL_GET(value1, RAW_INT, CHANNEL1_VSYNC) == RAW_INT_CHANNEL1_VSYNC_ACTIVE)
			&&(FIELD_VAL_GET(value2, INT_MASK, CHANNEL1_VSYNC) == INT_MASK_CHANNEL1_VSYNC_ENABLE))
		{
			return true;
		}
	}
	
	return false;
}


void hw768_clear_vsync_interrupt(int path)
{
	
	unsigned long value;
	
	value = peekRegisterDWord(RAW_INT);

	if (path == CHANNEL0_CTRL)
	{
		pokeRegisterDWord(RAW_INT, FIELD_SET(value, RAW_INT, CHANNEL0_VSYNC, CLEAR));
	}
	else
	{
		pokeRegisterDWord(RAW_INT, FIELD_SET(value, RAW_INT, CHANNEL1_VSYNC, CLEAR));
	}
}

long hw768_setMode(logicalMode_t *pLogicalMode, struct drm_display_mode mode)
{
	
	if (!pLogicalMode->valid_edid)
		return ddk768_setMode(pLogicalMode);
	else
	{	
		mode_parameter_t ModeParam;
		ModeParam = convert_drm_mode_to_ddk_mode(mode);
		return ddk768_setCustomMode(pLogicalMode, &ModeParam);
	}
	
}


int hdmi_int_status = 0;

inline int hdmi_hotplug_detect(void)
{
	int ret = 0;
	unsigned int intMask = peekRegisterDWord(INT_MASK);
	intMask = FIELD_SET(intMask, INT_MASK, HDMI, ENABLE);
	pokeRegisterDWord(INT_MASK, intMask);

	ret = hdmi_detect();

	if (ret == 1)
	{
		hdmi_int_status = 1;
	}
	else if (ret == 0)
	{
		hdmi_int_status = 0;
	}
	else
	{
		hdmi_int_status = hdmi_int_status & ret;
	}

	intMask = peekRegisterDWord(INT_MASK);
	intMask = FIELD_SET(intMask, INT_MASK, HDMI, DISABLE);
	pokeRegisterDWord(INT_MASK, intMask);

	return hdmi_int_status;
}

void ddk768_disable_IntMask(void)
{
	
    pokeRegisterDWord(INT_MASK, 0);
}

void hw768_SetPixelClockFormat(disp_control_t dispControl,unsigned int is_half)
{
    unsigned long ulDispCtrlAddr;
    unsigned long ulDispCtrlReg;

    if (dispControl == CHANNEL0_CTRL)
    {
        ulDispCtrlAddr = DISPLAY_CTRL;
    }
    else
        return;
    
    ulDispCtrlReg = peekRegisterDWord(ulDispCtrlAddr);
    
	if(is_half)
		ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, PIXEL_CLOCK_SELECT, HALF);
	else
		ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, PIXEL_CLOCK_SELECT, SINGLE);


    pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
}

void hw768_setgamma(disp_control_t dispCtrl, unsigned long enable, unsigned long lvds_ch)
{
	unsigned long value;
	unsigned long regCtrl;

	regCtrl = (dispCtrl == CHANNEL0_CTRL)? DISPLAY_CTRL 
				: (DISPLAY_CTRL+CHANNEL_OFFSET);

	value = peekRegisterDWord(regCtrl);
	
	if (enable)
	    value = FIELD_SET(value, DISPLAY_CTRL, GAMMA, ENABLE);
	else
	    value = FIELD_SET(value, DISPLAY_CTRL, GAMMA, DISABLE);

	if((lvds_ch == 2) && (dispCtrl == CHANNEL0_CTRL))   //dual channel LVDS and channel 0 gamma setting
		value = FIELD_SET(value, DISPLAY_CTRL, DOUBLE_PIXEL_CLOCK, ENABLE);
	else{
		value = FIELD_SET(value, DISPLAY_CTRL, DOUBLE_PIXEL_CLOCK, DISABLE);
	}
	   	
	pokeRegisterDWord(regCtrl, value);    
}

void hw768_load_lut(disp_control_t dispCtrl, int size, u8 lut_r[], u8 lut_g[], u8 lut_b[])
{
	unsigned int i, v;
	unsigned long regCtrl;

	regCtrl = (dispCtrl == CHANNEL0_CTRL)? PALETTE_RAM 
				: (PALETTE_RAM+CHANNEL_OFFSET);

	for (i = 0; i < size; i++) {
		v = (lut_r[i] << 16);
		v |= (lut_g[i] << 8);
		v |= lut_b[i];
		pokeRegisterDWord(regCtrl + (i * 4), v);
	}
}



long hw768_AdaptI2CInit(struct smi_connector *smi_connector)
{
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
        return ddk768_AdaptHWI2CInit(smi_connector);
    }
    else
    {
        return ddk768_AdaptSWI2CInit(smi_connector); 
    }
#endif
	struct drm_connector *connector = &smi_connector->base;
    switch(connector->connector_type){
	case DRM_MODE_CONNECTOR_DVII:
	case DRM_MODE_CONNECTOR_VGA:
	    if(hwi2c_en){
			smi_connector->i2c_hw_enabled = 1;
			return ddk768_AdaptHWI2CInit(smi_connector);
		}
		break;
	case DRM_MODE_CONNECTOR_HDMIA:
	default:
		break;

   }
	smi_connector->i2c_hw_enabled = 0; 
	
	return ddk768_AdaptSWI2CInit(smi_connector); 

}


long hw768_AdaptI2CCleanBus(struct drm_connector *connector)
{
        struct smi_connector *smi_connector = to_smi_connector(connector);
    
    if(smi_connector->i2c_hw_enabled)
    {
        return ddk768_AdaptHWI2CCleanBus(smi_connector);
    }
    else
    {
        return ddk768_AdaptSWI2CCleanBus(smi_connector);
    }
}



