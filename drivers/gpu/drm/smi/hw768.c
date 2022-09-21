/*
 * Copyright 2016 SiliconMotion Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License version 2. See the file COPYING in the main
 * directory of this archive for more details.
 *
 */

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
#include <linux/version.h>


extern int lcd_scale;
extern int pwm_ctrl;


struct smi_768_register{
	uint32_t clock_enable, pll_ctrl[3];
	
	uint32_t primary_display_ctrl[10], lvds_ctrl, primary_hwcurs_ctrl[4];
	uint32_t secondary_display_ctrl[10], secondary_hwcurs_ctrl[4];
};


mode_parameter_t convert_drm_mode_to_ddk_mode(struct drm_display_mode mode)
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
		pokeRegisterDWord(0x80020,0x31E30000);		
		pokeRegisterDWord(0x8002C,0x74001200);
	}else{
		unsigned long value = 0;
		pokeRegisterDWord(0x80020, 0x31E3F71D);
		pokeRegisterDWord(0x8002C,0x750FED02);
		value = peekRegisterDWord(DISPLAY_CTRL);
		value = SMI_FIELD_SET(value, DISPLAY_CTRL, LVDS_OUTPUT_FORMAT, CHANNEL0_48BIT);
		value = SMI_FIELD_SET(value, DISPLAY_CTRL, PIXEL_CLOCK_SELECT, HALF);
		value = SMI_FIELD_SET(value, DISPLAY_CTRL, DOUBLE_PIXEL_CLOCK, ENABLE);
		pokeRegisterDWord(DISPLAY_CTRL,value);

		value = peekRegisterDWord(DISPLAY_CTRL + CHANNEL_OFFSET);
		value = SMI_FIELD_SET(value, DISPLAY_CTRL, LVDS_OUTPUT_FORMAT, CHANNEL0_48BIT);
		pokeRegisterDWord(DISPLAY_CTRL + CHANNEL_OFFSET,value);
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

	
 	ulDispCtrlReg = SMI_FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, PLANE, DISABLE)|        
                    SMI_FIELD_SET(0, DISPLAY_CTRL, DATA_PATH, EXTENDED); 

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
	          SMI_FIELD_SET(0, FB_ADDRESS, STATUS, PENDING)
	        | SMI_FIELD_VALUE(0, FB_ADDRESS, ADDRESS, base_addr));

	    /* Pitch value (Hardware people calls it Offset) */
    	pokeRegisterDWord((FB_WIDTH), SMI_FIELD_VALUE(peekRegisterDWord(FB_WIDTH), FB_WIDTH, OFFSET, pitch));

		if(lcd_scale){
				pokeRegisterDWord((VIDEO_FB_WIDTH),
					   SMI_FIELD_VALUE(0, VIDEO_FB_WIDTH, WIDTH, pitch) |
					   SMI_FIELD_VALUE(0, VIDEO_FB_WIDTH, OFFSET, pitch));
		
			   pokeRegisterDWord(VIDEO_FB_ADDRESS,
					   SMI_FIELD_SET(0, VIDEO_FB_ADDRESS, STATUS, PENDING) |
					   SMI_FIELD_VALUE(0, VIDEO_FB_ADDRESS, ADDRESS, base_addr));

		}
	}
	else
	{
		/* Frame buffer base */
	    pokeRegisterDWord((FB_ADDRESS+CHANNEL_OFFSET),
	          SMI_FIELD_SET(0, FB_ADDRESS, STATUS, PENDING)
	        | SMI_FIELD_VALUE(0, FB_ADDRESS, ADDRESS, base_addr));

		
	    /* Pitch value (Hardware people calls it Offset) */	
	    pokeRegisterDWord((FB_WIDTH+CHANNEL_OFFSET),SMI_FIELD_VALUE(peekRegisterDWord(FB_WIDTH+CHANNEL_OFFSET), FB_WIDTH, OFFSET, pitch));

	}
}


void hw768_init_hdmi(void)
{
	HDMI_Init();
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
		SMI_FIELD_SET(0, INT_MASK, CHANNEL1_VSYNC, DISABLE):
		SMI_FIELD_SET(0, INT_MASK, CHANNEL0_VSYNC, DISABLE));
	}
	else
	{
		pokeRegisterDWord(INT_MASK, 	(pipe == CHANNEL1_CTRL) ? 
		SMI_FIELD_SET(0, INT_MASK, CHANNEL1_VSYNC, ENABLE):
		SMI_FIELD_SET(0, INT_MASK, CHANNEL0_VSYNC, ENABLE));
	}
	return 0;
}
#else
int hw768_en_dis_interrupt(int status)
	{
		if(status == 0)
		{
			pokeRegisterDWord(INT_MASK, SMI_FIELD_SET(0, INT_MASK, CHANNEL1_VSYNC, DISABLE));
			pokeRegisterDWord(INT_MASK, SMI_FIELD_SET(0, INT_MASK, CHANNEL0_VSYNC, DISABLE)); 
		}
		else
		{
			pokeRegisterDWord(INT_MASK, SMI_FIELD_SET(0, INT_MASK, CHANNEL1_VSYNC, ENABLE));
			pokeRegisterDWord(INT_MASK, SMI_FIELD_SET(0, INT_MASK, CHANNEL0_VSYNC, ENABLE));  
		}
		return 0;
	}

#endif

void hw768_HDMI_Enable_Output(void)
{
	HDMI_Enable_Output();
}

void hw768_HDMI_Disable_Output(void)
{
	HDMI_Disable_Output();
}


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

	
    if (SMI_FIELD_GET(value, INT_STATUS, I2S) == INT_STATUS_I2S_ACTIVE)
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
	    if ((SMI_FIELD_GET(value1, RAW_INT, CHANNEL0_VSYNC) == RAW_INT_CHANNEL0_VSYNC_ACTIVE)
			&&(SMI_FIELD_GET(value2, INT_MASK, CHANNEL0_VSYNC) == INT_MASK_CHANNEL0_VSYNC_ENABLE))
	    {
			return true;
		}
	}else{
		if ((SMI_FIELD_GET(value1, RAW_INT, CHANNEL1_VSYNC) == RAW_INT_CHANNEL1_VSYNC_ACTIVE)
			&&(SMI_FIELD_GET(value2, INT_MASK, CHANNEL1_VSYNC) == INT_MASK_CHANNEL1_VSYNC_ENABLE))
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
		pokeRegisterDWord(RAW_INT, SMI_FIELD_SET(value, RAW_INT, CHANNEL0_VSYNC, CLEAR));
	}
	else
	{
		pokeRegisterDWord(RAW_INT, SMI_FIELD_SET(value, RAW_INT, CHANNEL1_VSYNC, CLEAR));
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
	intMask = SMI_FIELD_SET(intMask, INT_MASK, HDMI, ENABLE);
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
	intMask = SMI_FIELD_SET(intMask, INT_MASK, HDMI, DISABLE);
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
		ulDispCtrlReg = SMI_FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, PIXEL_CLOCK_SELECT, HALF);
	else
		ulDispCtrlReg = SMI_FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, PIXEL_CLOCK_SELECT, SINGLE);


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
	    value = SMI_FIELD_SET(value, DISPLAY_CTRL, GAMMA, ENABLE);
	else
	    value = SMI_FIELD_SET(value, DISPLAY_CTRL, GAMMA, DISABLE);

	if((lvds_ch == 2) && (dispCtrl == CHANNEL0_CTRL))   //dual channel LVDS and channel 0 gamma setting
		value = SMI_FIELD_SET(value, DISPLAY_CTRL, DOUBLE_PIXEL_CLOCK, ENABLE);
	else{
		value = SMI_FIELD_SET(value, DISPLAY_CTRL, DOUBLE_PIXEL_CLOCK, DISABLE);
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


