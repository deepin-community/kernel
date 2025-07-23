// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.


#include "smi_drv.h"


#include <drm/drm_fourcc.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
#include <drm/drm_pci.h>
#else
#include <linux/pci.h>
#endif
#include <drm/drm_vblank.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_atomic_state_helper.h>
#include <drm/drm_gem.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_probe_helper.h>


#include "hw750.h"
#include "hw768.h"
#include "ddk768/ddk768_video.h"
#include "ddk768/ddk768_chip.h"
#include "hw770.h"
#include "smi_dbg.h"
#include "ddk770/ddk770_hdmi.h"

#define MAX_COLOR_LUT_ENTRIES 256

#if 0

/*
------------------------------------------------------
		DP0		DP1		HDMI0		HDMI1		HDMI2
1.		CRTC0	CRTC1	CRTC2
2.		CRTC0	CRTC1				CRTC2
3.		CRTC0	CRTC1							CRTC2
4. 		CRTC0			CRTC1		CRTC2
5.  	CRTC0			CRTC1					CRTC2
6.		CRTC0						CRTC1		CRTC2
7.				CRTC0	CRTC1		CRTC2
8.				CRTC0	CRTC1					CRTC2	
9.				CRTC0				CRTC1		CRTC2
10.						CRTC0		CRTC1		CRTC2
------------------------------------------------------
CRTC.	0		0/1		0/1/2		1/2			2
------------------------------------------------------
SO:
DP0 	-> CRTC0 (default)
DP1 	-> CRTC0 (default)   ->CRTC1 (USE_DP0)
HDMI0   -> CRTC0 (default)   ->CRTC1 (USE_DP0)  	->CRTC2 (USE_DP0 && USE_DP1)
HDMI1  	-> CRTC2 (default)   ->CRTC1 (USE_HDMI2)  
HDMI2 	-> CRTC2 (default)   
*/
int smi_encoder_crtc_index_changed(int encoder_index, int m_connector){  //For SM770
	int crtc_index = 0;
	switch (encoder_index) {
		case 0:
			crtc_index = CHANNEL0_CTRL;
			break;
		case 1:
			crtc_index = (m_connector & USE_DP0) ? CHANNEL1_CTRL : CHANNEL0_CTRL;
			break;
		case 2:
			crtc_index = ((m_connector & USE_DP0) && (m_connector & USE_DP1)) ? CHANNEL2_CTRL : 
					(m_connector & USE_DP0) ? CHANNEL1_CTRL : CHANNEL0_CTRL;
			break;
		case 3:
			crtc_index = (m_connector & USE_HDMI2) ? CHANNEL1_CTRL : CHANNEL2_CTRL;
			break;
		case 4:
			crtc_index = CHANNEL2_CTRL;
			break;
	}
	return crtc_index;
}
#else
/*
DP0->Encode0-> CRTC0 
DP1->Encode1-> CRTC1 (default) 
HDMI0->Encode2  -> CRTC0 (default) 
HDMI1->Encode3  -> CRTC1 (default) 
HDMI2->Encode4 	-> CRTC2 (default) 

So only support 
Single HDMI0/1/2, DP0/DP1
DP0+DP1
HDMI0+HDMI1/HDMI2
DP0+HDMI1/HDMI2
DP0+HDMI1+HDMI2
DP0+DP1+HDMI2
HDMI0+HDMI1+HDMI2

Priority:
HDMI0>DP0
HDMI1>DP1

*/


int smi_encoder_crtc_index_changed(int encoder_index){  //For SM770
	int crtc_index = 0;
	switch (encoder_index) {
		case 0: case 2:
			crtc_index = CHANNEL0_CTRL;
			break;
		case 1: case 3:
			crtc_index = CHANNEL1_CTRL;
			break;
		case 4:
			crtc_index = CHANNEL2_CTRL;
			break;
	}
	return crtc_index;
}


#endif

int smi_calc_hdmi_ctrl(int m_connector)
{
		int smi_ctrl = 0;

		if(m_connector==USE_DVI_HDMI) // //vga is empty, dvi is occupied , HDMI use ctrl 1;
			smi_ctrl = 1;
		else
			smi_ctrl = 0;
			
		return smi_ctrl;

}

static int count_set_bits(unsigned int value) {
    int count = 0;
    while (value) {
        count += value & 1;
        value >>= 1;
    }
    return count;
}


static int smi_crtc_set_gamma(struct drm_crtc *crtc, const struct drm_format_info *format,
			       struct drm_color_lut *lut)

{

	struct smi_crtc *smi_crtc = to_smi_crtc(crtc); 
	int i , ctrl_index, dst_ctrl;
	struct smi_device *sdev = crtc->dev->dev_private;
	int max_enc, max_ctrl;

	ctrl_index = 0;
	dst_ctrl = 0;

	max_enc = MAX_ENCODER(sdev->specId);
	max_ctrl = MAX_CRTC(sdev->specId);	

	for(i = 0;i < max_enc; i++)
	{
		if(crtc == sdev->smi_enc_tab[i]->crtc)
		{
			ctrl_index = i;
			break;
		}
	}
	
	dst_ctrl = (ctrl_index == CHANNEL1_CTRL)?CHANNEL1_CTRL:CHANNEL0_CTRL;



	if(sdev->specId == SPC_SM768){
		if(ctrl_index >= MAX_CRTC_768)  //calc which path should we use for HDMI.
	{
			dst_ctrl = smi_calc_hdmi_ctrl(sdev->m_connector);
	}
	}else if(sdev->specId == SPC_SM770){
		dst_ctrl = (disp_control_t)smi_encoder_crtc_index_changed(ctrl_index);
	}
	
	
	if(!lut) {
	for (i = 0; i < crtc->gamma_size; i++)
	{
		smi_crtc->lut_r[i] = i;
		smi_crtc->lut_g[i] = i;
		smi_crtc->lut_b[i] = i;
	}
	} else {



	for (i = 0; i < crtc->gamma_size; i++)
	{
		smi_crtc->lut_r[i] = lut[i].red >> 8;
		smi_crtc->lut_g[i] = lut[i].green >> 8;
		smi_crtc->lut_b[i] = lut[i].blue >> 8;
       	}
       	}

	
	if(sdev->specId == SPC_SM750) {
		hw750_setgamma(dst_ctrl, true);
		hw750_load_lut(dst_ctrl, crtc->gamma_size, smi_crtc->lut_r, smi_crtc->lut_g, smi_crtc->lut_b);
	}else if (sdev->specId == SPC_SM768) {
		if(sdev->m_connector & USE_DVI)
			hw768_setgamma(dst_ctrl, true , lvds_channel);
		else
			hw768_setgamma(dst_ctrl, true , 0);
			
		hw768_load_lut(dst_ctrl, crtc->gamma_size, smi_crtc->lut_r, smi_crtc->lut_g, smi_crtc->lut_b);
	}else if (sdev->specId == SPC_SM770) {
		hw770_setgamma(dst_ctrl, true);
		hw770_load_lut(dst_ctrl, crtc->gamma_size, smi_crtc->lut_r, smi_crtc->lut_g, smi_crtc->lut_b);
	}
	
	return 0;
}


static void smi_dp_set_mode(struct smi_device *sdev, dp_index index)
{
	struct drm_display_mode *mode;

	struct drm_device *dev;
	logicalMode_t logicalMode;
	unsigned long refresh_rate;
	struct drm_crtc *crtc;
	struct smi_770_fb_info fb_info = {0};

	int ret = 0;

	dev = sdev->dev;

	crtc = sdev->smi_enc_tab[index]->crtc;
	if (crtc) {
		dbg_msg("CRTC%d found: %p\n", index,crtc);
	} else {
		dbg_msg("CRTC%d not found\n",index);
		return;
	}

	mode = &crtc->state->adjusted_mode;

	if(!mode)
		return;

	refresh_rate = drm_mode_vrefresh(mode);
	hw770_get_current_fb_info((disp_control_t)index,&fb_info);

	logicalMode.valid_edid = false;

	if(index == INDEX_DP0){
		if (sdev->dp0_edid && drm_edid_header_is_valid((u8 *)sdev->dp0_edid) == 8)
			logicalMode.valid_edid = true;
	}else if(index == INDEX_DP1){
		if (sdev->dp1_edid && drm_edid_header_is_valid((u8 *)sdev->dp1_edid) == 8)
			logicalMode.valid_edid = true;
	}
	logicalMode.x = mode->hdisplay;
	logicalMode.y = mode->vdisplay;
	logicalMode.bpp = smi_bpp;
	logicalMode.hz = refresh_rate;
	logicalMode.pitch = 0;
	logicalMode.dispCtrl = (disp_control_t)index;


	hw770_setMode(&logicalMode, *mode);
	dbg_msg("Starting init SM770 DP %d! Use Channel [%d]\n", index,index);

	ret = hw770_set_dp_mode(&logicalMode, *mode, index);

	if (ret != 0)
	{
		dbg_msg("DP Mode not supported!\n");
		return;
	}
	hw770_set_current_pitch((disp_control_t)index,&fb_info);
}
/*
 * The DRM core requires DPMS functions, but they make little sense in our
 * case and so are just stubs
 */

static void smi_crtc_mode_set_nofb(struct drm_crtc *crtc)
{
	
	struct drm_display_mode *mode;
	struct smi_device *sdev = crtc->dev->dev_private;
	logicalMode_t logicalMode;
	unsigned long refresh_rate;
	unsigned int need_to_scale = 0;
	YUV_BUF_ADDR SrcAddr;
	BLIT_BLK src;
	BLIT_BLK dest;

	int ret = 0;
	

	ENTER();
	
	logicalMode.valid_edid = false;

	if (WARN_ON(!crtc->state))
		LEAVE();

	sdev = crtc->dev->dev_private;
	mode = &crtc->state->adjusted_mode;
	refresh_rate = drm_mode_vrefresh(mode);

	dbg_msg("***crtc addr:%p\n", crtc);

	dbg_msg("encode->crtc:[%p, %p, %p] \n", sdev->smi_enc_tab[0]->crtc, sdev->smi_enc_tab[1]->crtc,
		sdev->smi_enc_tab[2] == NULL ? NULL : sdev->smi_enc_tab[2]->crtc);
	dbg_msg("m_connector = %d,  DVI [%d], VGA[%d], HDMI[%d] \n", sdev->m_connector,
		sdev->m_connector & 0x1, sdev->m_connector & 0x2, sdev->m_connector & 0x4);

	dbg_msg("wxh:%dx%d@%ldHz\n", mode->hdisplay, mode->vdisplay, refresh_rate);

	if (sdev->specId == SPC_SM750) {
			if(crtc == sdev->smi_enc_tab[0]->crtc)
		{
			logicalMode.baseAddress = 0;
			logicalMode.x = mode->hdisplay;
			logicalMode.y = mode->vdisplay;
			logicalMode.bpp = smi_bpp;
			logicalMode.dispCtrl = CHANNEL0_CTRL;
			logicalMode.hz = refresh_rate;
			logicalMode.pitch = 0;

			setMode(&logicalMode);

			setDisplayControl(CHANNEL0_CTRL, DISP_ON);           /* Turn on Primary Control */
			setPath(SMI0_PATH, CHANNEL0_CTRL, DISP_ON);     /* Turn on Panel Path and use Primary data */
			
		}
		if(crtc == sdev->smi_enc_tab[1]->crtc)
		{
			logicalMode.baseAddress = 0;
			logicalMode.x = mode->hdisplay;
			logicalMode.y = mode->vdisplay;
			logicalMode.bpp = smi_bpp;
			logicalMode.dispCtrl = CHANNEL1_CTRL;
			logicalMode.hz = refresh_rate;
			logicalMode.pitch = 0;
			setMode(&logicalMode);

			setDisplayControl(CHANNEL1_CTRL, DISP_ON);         /* Turn on Secondary control */
			setPath(SMI1_PATH, CHANNEL1_CTRL, DISP_ON);     /* Turn on CRT Path and use Secondary data */
			
		}
		
		swPanelPowerSequence(DISP_ON, 4);                   /* Turn on Panel */
	    setDAC(DISP_ON);                                    /* Turn on DAC */

#ifdef USE_HDMICHIP
		printk("SI9022 HDMI set mode\n");
		sii9022xSetMode(5);
#endif

	}
	else if(sdev->specId == SPC_SM768) {  //SPC_SM768
		int i, ctrl_index, dst_ctrl;
		ctrl_index = 0;
		dst_ctrl = 0;
		for(i = 0;i < MAX_ENCODER(sdev->specId); i++)
		{
			if(crtc == sdev->smi_enc_tab[i]->crtc)
			{
				ctrl_index = i;
				break;
			}
		}

		dst_ctrl = (ctrl_index == CHANNEL1_CTRL)?CHANNEL1_CTRL:CHANNEL0_CTRL;
		
		if(ctrl_index >= MAX_CRTC_768)
		{
			dst_ctrl=smi_calc_hdmi_ctrl(sdev->m_connector);
			dbg_msg("hdmi use channel %d\n",dst_ctrl);
	
		}
		
		logicalMode.baseAddress = 0;
		logicalMode.x = mode->hdisplay;
		logicalMode.y = mode->vdisplay;
		logicalMode.bpp = smi_bpp;
		logicalMode.hz = refresh_rate;
		logicalMode.pitch = 0;
		logicalMode.dispCtrl = dst_ctrl;

		switch (ctrl_index) // 0:DVI, 1:VGA, 2:HDMI
		{
			case 0:
				if (sdev->dvi_edid && drm_edid_header_is_valid((u8 *)sdev->dvi_edid) == 8)
					logicalMode.valid_edid = true;
				break;
			case 1:
				if (sdev->vga_edid && drm_edid_header_is_valid((u8 *)sdev->vga_edid) == 8)
					logicalMode.valid_edid = true;
				break;
			case 2:
				if (sdev->hdmi_edid && drm_edid_header_is_valid((u8 *)sdev->hdmi_edid) == 8)
					logicalMode.valid_edid = true;
				break;
			default:
				break;
		}
		if(edid_mode == 0){
			printk("Use Driver build-in mode timing\n");
			logicalMode.valid_edid = false;
		}

		if(lcd_scale && (dst_ctrl == 0) && (mode->hdisplay != fixed_width) && (mode->vdisplay != fixed_height) ){
			need_to_scale = 1;
		}
		
		if(need_to_scale){
				 logicalMode.x = fixed_width;
				 logicalMode.y = fixed_height;
				 logicalMode.valid_edid = false;
		 }
		
		hw768_setMode(&logicalMode, *mode);
		DisableDoublePixel(0);
		DisableDoublePixel(1);

		if(need_to_scale)
		{
			file_format srcFormat;
			if(logicalMode.bpp==16)
				srcFormat = FFT_RGB565;
			if(logicalMode.bpp==32)
				srcFormat = FFT_RGBx888;
		
			src.Width = mode->hdisplay;
			src.Height = mode->vdisplay;
			src.Pitch = logicalMode.pitch;
			dest.Width = logicalMode.x;  
			dest.Height = logicalMode.y; 
			dest.Pitch = logicalMode.pitch;   
			dest.x = 0;
			dest.y = 0;
		
			SrcAddr.bufYAddr = logicalMode.baseAddress;
			SrcAddr.bufCbAddr = 0;
			SrcAddr.bufCrAddr = 0;		
			SM768_setOverlay(logicalMode.dispCtrl, &src, &SrcAddr, &dest, srcFormat);
		}else{
			stopVideo(dst_ctrl);
		}
		setSingleViewOn(dst_ctrl);

		initDisplay();		

		if(need_to_scale)
			ddk768_setDisplayPlaneDisableOnly(dst_ctrl);
#ifdef USE_EP952
		hw768_SetPixelClockFormat(dst_ctrl,1);
#else
		if (ddk768_getPixelType())
			hw768_SetPixelClockFormat(dst_ctrl, 1);
		else
			hw768_SetPixelClockFormat(dst_ctrl,0);
#endif
		if((sdev->m_connector & USE_HDMI)&&(ctrl_index > CHANNEL1_CTRL))
		{
			int ret = 0;
			printk("Starting init SM768 HDMI! Use Channel [%d]\n", dst_ctrl);
			if(dst_ctrl == 0)
				hw768_SetPixelClockFormat(dst_ctrl,0);
			ret=hw768_set_hdmi_mode(&logicalMode, *mode, sdev->is_768hdmi);
			if (ret != 0)
			{
				printk("HDMI Mode not supported!\n");
			}
		}
		
		if((sdev->m_connector & USE_DVI) && dst_ctrl == 0){  //LVDS and TTL always use CH0
			if(lvds_channel == 1){
				printk("Use Single Channel LVDS\n");
				hw768_enable_lvds(1);
				DisableDoublePixel(0);
			}
			else if(lvds_channel == 2){
				printk("Use Dual Channel LVDS\n");
				hw768_enable_lvds(2);
				EnableDoublePixel(0);
			}	
		}
		
		
		
		
			
	}else if(sdev->specId ==SPC_SM770){
		int encoder_index = 0, dst_ctrl = 0, i;
	
		for(i = 0;i < MAX_ENCODER(sdev->specId); i++)
		{
			if(crtc == sdev->smi_enc_tab[i]->crtc)
			{
				encoder_index = i;
				break;
			}
		}

		dst_ctrl = smi_encoder_crtc_index_changed(encoder_index);

		logicalMode.baseAddress = 0;
		logicalMode.x = mode->hdisplay;
		logicalMode.y = mode->vdisplay;
		logicalMode.bpp = smi_bpp;
		logicalMode.hz = refresh_rate;
		logicalMode.pitch = 0;
		logicalMode.dispCtrl = dst_ctrl;

		switch (encoder_index)
		{
			case 0:
				if (sdev->dp0_edid && drm_edid_header_is_valid((u8 *)sdev->dp0_edid) == 8)
					logicalMode.valid_edid = true;
				break;
			case 1:
				if (sdev->dp1_edid && drm_edid_header_is_valid((u8 *)sdev->dp1_edid) == 8)
					logicalMode.valid_edid = true;
				break;
			case 2:
				if (sdev->hdmi0_edid && drm_edid_header_is_valid((u8 *)sdev->hdmi0_edid) == 8)
					logicalMode.valid_edid = true;
				break;
			case 3:
				if (sdev->hdmi1_edid && drm_edid_header_is_valid((u8 *)sdev->hdmi1_edid) == 8)
					logicalMode.valid_edid = true;
				break;
			case 4:
				if (sdev->hdmi2_edid && drm_edid_header_is_valid((u8 *)sdev->hdmi2_edid) == 8)
					logicalMode.valid_edid = true;
				break;
			default:
				break;
		}

		if(edid_mode == 0){
			printk("Use Driver build-in mode timing\n");
			logicalMode.valid_edid = false;
		}
		if(!mode->clock)
			return;
		
		hw770_setMode(&logicalMode, *mode);
		
		if((sdev->m_connector & USE_HDMI0 || sdev->m_connector & USE_HDMI1 || sdev->m_connector & USE_HDMI2) && (encoder_index > 1)) //hdmi start from encoder 2
		{
		
			int hdmi_index;

			hdmi_index = encoder_index - 2; 
			
			printk("Starting init SM770 HDMI %d! Use Channel [%d]\n", hdmi_index,dst_ctrl);

		
            ret = hw770_set_hdmi_mode(&logicalMode, *mode, sdev->is_hdmi[hdmi_index],hdmi_index);
			
			hw770_hdmi_interrupt_enable(hdmi_index,1);
			
			if (ret != 0)
			{
				printk("HDMI Mode not supported!\n");
			}
		}

		if((sdev->m_connector & USE_DP0 || sdev->m_connector & USE_DP1) && (encoder_index < 2)) //dp start from encoder 0 to encoder 1
		{
			int dp_index;

			dp_index = encoder_index;

			printk("Starting init SM770 DP %d! Use Channel [%d]\n", dp_index,dst_ctrl);

			ret = hw770_set_dp_mode(&logicalMode, *mode, dp_index);

			if (ret != 0)
			{
				printk("DP Mode not supported!\n");
			}
	

		}	
	
	}

	LEAVE();
}

/* Simple cleanup function */
static void smi_crtc_destroy(struct drm_crtc *crtc)
{
	struct smi_crtc *smi_crtc = to_smi_crtc(crtc);

	drm_crtc_cleanup(crtc);
	kfree(smi_crtc);
}


static void smi_crtc_atomic_flush(struct drm_crtc *crtc, 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
	struct drm_atomic_state *state)
#else
	struct drm_crtc_state *old_state)
#endif
{
	
	unsigned long flags;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
	struct drm_crtc_state *crtc_state = drm_atomic_get_new_crtc_state(state,crtc);
#else
	struct drm_crtc_state *crtc_state = old_state;
#endif

	ENTER();
	/*
	 * The gamma LUT has to be reloaded after changing the primary
	 * plane's color format.
	 */
	if (crtc_state->enable && crtc_state->color_mgmt_changed) {
		if (crtc_state->gamma_lut)
			smi_crtc_set_gamma(crtc,
					   NULL,
					   crtc_state->gamma_lut->data);
		else
			smi_crtc_set_gamma(crtc, NULL, NULL);
	}

	spin_lock_irqsave(&crtc->dev->event_lock, flags);
	if (crtc->state->event)
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
	crtc->state->event = NULL;
	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
	LEAVE();
}

static void smi_crtc_atomic_enable(struct drm_crtc *crtc, 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
		struct drm_atomic_state *state)
#else
		struct drm_crtc_state *old_state)
#endif
{
	
	struct smi_device *sdev = crtc->dev->dev_private;

	if (crtc->state->gamma_lut)
		smi_crtc_set_gamma(crtc, NULL, crtc->state->gamma_lut->data);
	else
		smi_crtc_set_gamma(crtc, NULL, NULL);


	if (sdev->specId == SPC_SM770){

		int encoder_index = 0, dst_ctrl = 0, i;
		
			for(i = 0;i < MAX_ENCODER(sdev->specId); i++)
			{
				if(crtc == sdev->smi_enc_tab[i]->crtc)
				{
					encoder_index = i;
					break;
				}
			}

			dst_ctrl = smi_encoder_crtc_index_changed(encoder_index);
		
			ddk770_EnableChannelTiming(dst_ctrl,DISP_ON);

	}

}

static void smi_crtc_atomic_disable(struct drm_crtc *crtc, 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
		struct drm_atomic_state *state)
#else
		struct drm_crtc_state *old_state)
#endif
{
	struct smi_device *sdev = crtc->dev->dev_private;
			
	if (sdev->specId == SPC_SM770){

		int encoder_index = 0, dst_ctrl = 0, i;
		
			for(i = 0;i < MAX_ENCODER(sdev->specId); i++)
			{
				if(crtc == sdev->smi_enc_tab[i]->crtc)
				{
					encoder_index = i;
					break;
				}
			}

			dst_ctrl = smi_encoder_crtc_index_changed(encoder_index);
			
			ddk770_EnableChannelTiming(dst_ctrl,DISP_OFF);

	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
static int smi_enable_vblank(struct drm_crtc *crtc)
{
	struct smi_device *sdev = crtc->dev->dev_private;

	if (sdev->specId == SPC_SM750) {
		hw750_en_dis_interrupt(1);
	} else if (sdev->specId == SPC_SM768) {
		hw768_en_dis_interrupt(1);
	} else if (sdev->specId == SPC_SM770) {
		hw770_en_dis_interrupt(1);
	}
	return 0;
}

static void smi_disable_vblank(struct drm_crtc *crtc)
{
	struct smi_device *sdev = crtc->dev->dev_private;
	
	if (sdev->specId == SPC_SM750) {
		hw750_en_dis_interrupt(0);
	} else if (sdev->specId == SPC_SM768) {
		hw768_en_dis_interrupt(0);
	} else if (sdev->specId == SPC_SM770) {
		hw770_en_dis_interrupt(0);
	}
}
#endif




/* These provide the minimum set of functions required to handle a CRTC */
static const struct drm_crtc_funcs smi_crtc_funcs = {
	.page_flip = drm_atomic_helper_page_flip,
	.reset = drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
	.set_config = drm_atomic_helper_set_config,
	.destroy = smi_crtc_destroy,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
	.enable_vblank = smi_enable_vblank,
	.disable_vblank = smi_disable_vblank,
#endif
};

static const struct drm_crtc_helper_funcs smi_crtc_helper_funcs = {
	.mode_set_nofb = smi_crtc_mode_set_nofb,
	.atomic_flush = smi_crtc_atomic_flush,
	.atomic_enable = smi_crtc_atomic_enable,
	.atomic_disable = smi_crtc_atomic_disable
};


/* CRTC setup */
static struct smi_crtc *smi_crtc_init(struct drm_device *dev, int crtc_id)
{
	struct smi_device *cdev = dev->dev_private;
	struct smi_crtc *smi_crtc;
	struct drm_plane *primary = NULL, *cursor = NULL;
	int r, i;

	smi_crtc = kzalloc(sizeof(struct smi_crtc) + sizeof(struct drm_connector *), GFP_KERNEL);
	if (smi_crtc == NULL)
		return NULL;

	primary = smi_plane_init(cdev, 1 << crtc_id, DRM_PLANE_TYPE_PRIMARY);

	if (IS_ERR(primary)) {
		r = -ENOMEM;
		goto free_mem;
	}

	if(swcur_en)
		cursor = NULL;
	else{
		cursor = smi_plane_init(cdev, 1 << crtc_id, DRM_PLANE_TYPE_CURSOR);
		if (IS_ERR(cursor)) {
			r = -ENOMEM;
			goto clean_primary;
		}
	}
	smi_crtc->CursorOffset = 0;

	r = drm_crtc_init_with_planes(dev, &smi_crtc->base, primary, cursor, &smi_crtc_funcs, NULL);

	if (r) {
		goto clean_cursor;
	}

	drm_mode_crtc_set_gamma_size(&smi_crtc->base, MAX_COLOR_LUT_ENTRIES);
	
	for (i = 0; i < smi_crtc->base.gamma_size; i++)
	{
		smi_crtc->lut_r[i] = smi_crtc->lut_g[i] = smi_crtc->lut_b[i] = i;
	}
	drm_crtc_enable_color_mgmt(&smi_crtc->base, 0, false, MAX_COLOR_LUT_ENTRIES);
	drm_mode_crtc_set_gamma_size(&smi_crtc->base, MAX_COLOR_LUT_ENTRIES);
	
	drm_crtc_helper_add(&smi_crtc->base, &smi_crtc_helper_funcs);
	return smi_crtc;


clean_cursor:
	if (cursor) {
		drm_plane_cleanup(cursor);
		kfree(cursor);
	}

clean_primary:
	drm_plane_cleanup(primary);
	kfree(primary);
free_mem:
	kfree(smi_crtc);
	return NULL;
}



static void smi_encoder_mode_set(struct drm_encoder *encoder,
				struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
}

static void smi_encoder_dpms(struct drm_encoder *encoder, int mode)
{
	int index =0, i;
	struct smi_device *sdev = encoder->dev->dev_private;

	ENTER();
	if (sdev->specId == SPC_SM750 || sdev->specId == SPC_SM768)
	{
		if (encoder->encoder_type == DRM_MODE_ENCODER_LVDS)
			index = 0;
		else if (encoder->encoder_type == DRM_MODE_ENCODER_DAC)
			index = 1;
		else if (encoder->encoder_type == DRM_MODE_ENCODER_TMDS)
			index = 2;
	}
	else if (sdev->specId == SPC_SM770)
	{
		for (i = 0; i < MAX_ENCODER(sdev->specId); i++)
		{
			if (encoder == sdev->smi_enc_tab[i])
			{
				index = i;
				break;
			}
		}
	}

	dbg_msg("The current connect group = [%d], we deal with con=[%d], mode=[%s]\n", sdev->m_connector,index, (mode == DRM_MODE_DPMS_OFF)?"Off":"ON");
	if(sdev->specId == SPC_SM750)
	{	
		if (mode == DRM_MODE_DPMS_OFF) {
			dbg_msg("disable dpms, index=%d\n",index);
			setDisplayControl(index, DISP_OFF); 
		}else
		{
			setDisplayControl(index, DISP_ON); 
			swPanelPowerSequence(DISP_ON, 4); 
			dbg_msg("enable dpms ,index=%d\n",index);
		}
	}else if(sdev->specId == SPC_SM768)
	{

		if(encoder->encoder_type  == DRM_MODE_ENCODER_LVDS) 
		{
			if(sdev->m_connector == USE_VGA_HDMI||sdev->m_connector==USE_HDMI)
			{
				dbg_msg("DVI connector off\n");
				LEAVE();
			}
			dbg_msg("DVI connector: index=%d\n",index);
	
		}
		else if(encoder->encoder_type  == DRM_MODE_ENCODER_DAC)
		{
			if(sdev->m_connector == USE_DVI_HDMI)
			{
				dbg_msg("VGA connector off\n");
				LEAVE();
			}
			dbg_msg("VGA connector: index=%d\n",index);
		}
		else if(encoder->encoder_type  == DRM_MODE_ENCODER_TMDS)
		{	
			if (mode == DRM_MODE_DPMS_OFF)	
				HDMI_Disable_Output();
			else
				HDMI_Enable_Output();
			if(sdev->m_connector == USE_DVI_HDMI){
				index = CHANNEL1_CTRL;
			 	dbg_msg("HDMI connector: index=%d\n",index);
			}
			else if(sdev->m_connector == USE_VGA_HDMI || sdev->m_connector == USE_HDMI){
				index = CHANNEL0_CTRL;
			 	dbg_msg("HDMI connector: index=%d\n",index);
			}else{
				dbg_msg("HDMI connector not set dpms\n");
				LEAVE();
			}
		}
		
		if (mode == DRM_MODE_DPMS_OFF){
			setDisplayDPMS(index, DISP_DPMS_OFF);
			ddk768_swPanelPowerSequence(index, 0, 4);
		}else{
			setDisplayDPMS(index, DISP_DPMS_ON);
			ddk768_swPanelPowerSequence(index, 1, 4);
		}
		
		if(lvds_channel == 2 && (sdev->m_connector & USE_DVI) )
			EnableDoublePixel(0);
		else
		    DisableDoublePixel(0);

	}else if(sdev->specId == SPC_SM770)
	{
		
		index = smi_encoder_crtc_index_changed(index);

		if (mode == DRM_MODE_DPMS_OFF){
			dbg_msg("DC %d 770 dpms off\n", index);
			ddk770_setDisplayDPMS(index, DISP_DPMS_OFF);
			ddk770_swPanelPowerSequence(index, 0, index, 4);  
		}else{
			dbg_msg("DC %d 770 dpms on\n", index);
			ddk770_setDisplayDPMS(index, DISP_DPMS_ON);
			ddk770_swPanelPowerSequence(index, 1, index, 4);
		}
		
	}
	LEAVE();
}

static void smi_encoder_prepare(struct drm_encoder *encoder)
{
	smi_encoder_dpms(encoder, DRM_MODE_DPMS_OFF);

}

static void smi_encoder_commit(struct drm_encoder *encoder)
{
	smi_encoder_dpms(encoder, DRM_MODE_DPMS_ON);

}

static void smi_encoder_destroy(struct drm_encoder *encoder)
{
	struct smi_encoder *smi_encoder = to_smi_encoder(encoder);
	drm_encoder_cleanup(encoder);
	kfree(smi_encoder);
}

static const struct drm_encoder_helper_funcs smi_encoder_helper_funcs = {
	.dpms = smi_encoder_dpms,
	.mode_set = smi_encoder_mode_set,
	.prepare = smi_encoder_prepare,
	.commit = smi_encoder_commit,
};



static const struct drm_encoder_funcs smi_encoder_encoder_funcs = {
	.destroy = smi_encoder_destroy,
};

static struct drm_encoder *smi_encoder_init(struct drm_device *dev, int index)
{
	struct drm_encoder *encoder;
	struct smi_encoder *smi_encoder;
	struct smi_device *sdev = dev->dev_private;

	smi_encoder = kzalloc(sizeof(struct smi_encoder), GFP_KERNEL);
	if (!smi_encoder)
		return NULL;

	encoder = &smi_encoder->base;
	encoder->possible_crtcs = (1 << index);

	if (sdev->specId == SPC_SM750 || sdev->specId == SPC_SM768){
	switch (index)
	{
		case 0:
			//DVI,LVDS
			drm_encoder_init(dev, encoder, &smi_encoder_encoder_funcs, DRM_MODE_ENCODER_LVDS, NULL);
			break;
		case 1:
			//VGA
			drm_encoder_init(dev, encoder, &smi_encoder_encoder_funcs, DRM_MODE_ENCODER_DAC, NULL);
			break;
		case 2:
			//HDMI
            encoder->possible_crtcs = 0x3;
			drm_encoder_init(dev, encoder, &smi_encoder_encoder_funcs, DRM_MODE_ENCODER_TMDS, NULL);
			break;
		default:
			printk(KERN_ERR "Wrong connector index\n");
	}
	} else if(sdev->specId == SPC_SM770){
		
	//	drm_encoder_init(dev, encoder, &smi_encoder_encoder_funcs, DRM_MODE_ENCODER_TMDS, NULL);
		switch (index)
		{
			case 0: case 1:
				//DP
				drm_encoder_init(dev, encoder, &smi_encoder_encoder_funcs, DRM_MODE_ENCODER_DPMST, NULL);
				break;
			case 2: case 3:
				//HDMI
				encoder->possible_crtcs = (index - 1);
				drm_encoder_init(dev, encoder, &smi_encoder_encoder_funcs, DRM_MODE_ENCODER_TMDS, NULL);
				break;
			case 4:
				//HDMI
				encoder->possible_crtcs = 0x4;
				drm_encoder_init(dev, encoder, &smi_encoder_encoder_funcs, DRM_MODE_ENCODER_TMDS, NULL);
				break;		
			default:
				printk(KERN_ERR "Wrong connector index\n");
		}
	}

	drm_encoder_helper_add(encoder, &smi_encoder_helper_funcs);
 	return encoder;
}

static int hdmi_get_edid_property(struct drm_connector *connector,
				  struct edid *hdmi_edid, int use_flag,
				  hdmi_index index, int retry)
{
	int count = 0;
	struct smi_device *sdev = connector->dev->dev_private;
	struct smi_connector *smi_connector = to_smi_connector(connector);

	ENTER();

#if USE_I2C_ADAPTER
read_again0:
	hdmi_edid = drm_get_edid(connector, &smi_connector->adapter);

	if ((sdev->m_connector & use_flag) && !hdmi_edid && retry) {
		retry--;
		dbg_msg("HDMI_%d iic reset\n\n", index);
		hw770_i2c_reset_busclear(index);
		goto read_again0;
	}
	if (hdmi_edid)
#else
	int ret = hw770_get_hdmi_edid(index, (unsigned char *)hdmi_edid);
	if (ret)
#endif
	{
		dbg_msg("HDMI_%d get edid success.\n", index);
		drm_connector_update_edid_property(connector, hdmi_edid);
		count = drm_add_edid_modes(connector, hdmi_edid);
		sdev->is_hdmi[index] = drm_detect_hdmi_monitor(hdmi_edid);
		ddk770_HDMI_set_SCDC(index, (u8 *)hdmi_edid);
		dbg_msg("SM770 HDMI_%d connector is %s\n", index,
			(sdev->is_hdmi[index] ? "HDMI monitor" :
						"DVI monitor"));
	}
	if (hdmi_edid == NULL || count == 0) {
		drm_connector_update_edid_property(connector, NULL);
		count = drm_add_modes_noedid(connector, 3840, 2160);
		drm_set_preferred_mode(connector, fixed_width, fixed_height);
		sdev->is_hdmi[index] = true;
	}

	LEAVE(count);
}

static int smi_connector_get_modes(struct drm_connector *connector)
{

	int ret __attribute__((unused))= 0;
	void *edid_buf __attribute__((unused)) = NULL;
	int count = 0;
	unsigned int retry = 3;
	struct smi_device *sdev = connector->dev->dev_private;
	struct smi_connector *smi_connector = to_smi_connector(connector);

	ENTER();
	dbg_msg("print connector type: [%d], DVI=%d, VGA=%d, HDMI=%d\n",
			connector->connector_type, DRM_MODE_CONNECTOR_DVII, DRM_MODE_CONNECTOR_VGA, DRM_MODE_CONNECTOR_HDMIA);

	if(!fixed_width || !fixed_height){
		fixed_width = 1024;
		fixed_height = 768;
	}
	
	
	if(sdev->specId == SPC_SM750)
	{
		if(connector->connector_type == DRM_MODE_CONNECTOR_DVII)
		{
#ifdef USE_HDMICHIP

			edid_buf = sdev->si9022_edid;
			if(ddk750_GetDDC_9022Access())
				ret = ddk750_edidReadMonitorEx(CHANNEL0_CTRL, edid_buf, 256, 0, 30, 31);
			ddk750_Release9022DDC();
			if(ret){
              	drm_connector_update_edid_property(connector, sdev->si9022_edid);
                count = drm_add_edid_modes(connector, sdev->si9022_edid);
	
            }
                if (ret == 0 || count == 0)
                {
	
                        drm_connector_update_edid_property(connector, NULL);
                        count = drm_add_modes_noedid(connector, 1920, 1080);
                        drm_set_preferred_mode(connector, fixed_width, fixed_height);
                }


#else
		

				sdev->dvi_edid = drm_get_edid(connector, &smi_connector->adapter);


			if(sdev->dvi_edid)
			{
				dbg_msg("DVI get edid success.\n");

				drm_connector_update_edid_property(connector, sdev->dvi_edid);
				count = drm_add_edid_modes(connector, sdev->dvi_edid);
			}
			if (sdev->dvi_edid == NULL || count == 0)
			{
				drm_connector_update_edid_property(connector, NULL);
				count = drm_add_modes_noedid(connector, 1920, 1080);
				drm_set_preferred_mode(connector, fixed_width, fixed_height);
			}
				
#endif
		}
		if(connector->connector_type == DRM_MODE_CONNECTOR_VGA)
		{
		
			sdev->vga_edid = drm_get_edid(connector, &smi_connector->adapter);
			
			if(sdev->vga_edid){
		
			    dbg_msg("VGA get edid success.\n");
			
				drm_connector_update_edid_property(connector, sdev->vga_edid); 

				count = drm_add_edid_modes(connector, sdev->vga_edid);
			}
			if (sdev->vga_edid == NULL || count == 0)
			{

				drm_connector_update_edid_property(connector, NULL);

				count = drm_add_modes_noedid(connector, 1920, 1080);
				drm_set_preferred_mode(connector, fixed_width, fixed_height);
			}
				
		}
	}
	else if(sdev->specId == SPC_SM768)
	{ //SM768 Part

		if(connector->connector_type == DRM_MODE_CONNECTOR_DVII)
		{
			if(lvds_channel){

				drm_connector_update_edid_property(connector, NULL);

				count = drm_add_modes_noedid(connector, fixed_width, fixed_height);
				drm_set_preferred_mode(connector, fixed_width, fixed_height);
			}
			else
			{

				sdev->dvi_edid = drm_get_edid(connector, &smi_connector->adapter);

				if(sdev->dvi_edid)
				{
					dbg_msg("DVI get edid success.\n");

					drm_connector_update_edid_property(connector, sdev->dvi_edid); 

					count = drm_add_edid_modes(connector, sdev->dvi_edid);
				}
		
				if (sdev->dvi_edid == NULL || count == 0)
				{

					drm_connector_update_edid_property(connector, NULL);

					count = drm_add_modes_noedid(connector, 1920, 1080);
					drm_set_preferred_mode(connector, fixed_width, fixed_height);
				}
			}

		}
		if(connector->connector_type == DRM_MODE_CONNECTOR_VGA)
		{

   			   sdev->vga_edid = drm_get_edid(connector, &smi_connector->adapter);
			
			if(sdev->vga_edid)
			{
				dbg_msg("VGA get edid success.\n");

				drm_connector_update_edid_property(connector, sdev->vga_edid);
				count = drm_add_edid_modes(connector, sdev->vga_edid);
			}

			if (sdev->vga_edid == NULL || count == 0)
			{

				drm_connector_update_edid_property(connector, NULL);
				count = drm_add_modes_noedid(connector, 1920, 1080);
				drm_set_preferred_mode(connector, fixed_width, fixed_height);
			}

		}
		if(connector->connector_type == DRM_MODE_CONNECTOR_HDMIA)
		{

			sdev->hdmi_edid = drm_get_edid(connector, &smi_connector->adapter);
            //hw768_get_hdmi_edid(tmpedid);   // Too Slow..
			if (sdev->hdmi_edid)
			{
				dbg_msg("HDMIA get edid success.\n");
				drm_connector_update_edid_property(connector, sdev->hdmi_edid);		
				count = drm_add_edid_modes(connector, sdev->hdmi_edid);
				sdev->is_768hdmi = drm_detect_hdmi_monitor(sdev->hdmi_edid);
                dbg_msg("HDMI connector is %s\n",(sdev->is_768hdmi ? "HDMI monitor" : "DVI monitor"));
			}
			if (sdev->hdmi_edid == NULL || count == 0)
			{
				drm_connector_update_edid_property(connector, NULL);
				count = drm_add_modes_noedid(connector, 1920, 1080);
				drm_set_preferred_mode(connector, fixed_width, fixed_height);
				sdev->is_768hdmi = true;
			}

		}

	}
	else if(sdev->specId == SPC_SM770){ //SM770 Part
		if(connector->connector_type == DRM_MODE_CONNECTOR_DisplayPort)
		{
#if USE_I2C_ADAPTER
			sdev->dp0_edid = drm_get_edid(
				connector, &smi_connector->dp_adapter);
			if (sdev->dp0_edid)
#else
			ret = hw770_get_dp_edid(
				0, (unsigned char *)sdev->dp0_edid);
			if (ret)
#endif
			{
				dbg_msg("DP0 get edid success.\n");
				drm_connector_update_edid_property(connector, sdev->dp0_edid);
				count = drm_add_edid_modes(connector, sdev->dp0_edid);
			}

			if (sdev->dp0_edid == NULL || count == 0)
			{
				drm_connector_update_edid_property(connector, NULL);
				count = drm_add_modes_noedid(connector, 3840, 2160);
				drm_set_preferred_mode(connector, fixed_width, fixed_height);
			}

		}
		if(connector->connector_type == DRM_MODE_CONNECTOR_eDP)
		{
#if USE_I2C_ADAPTER
			sdev->dp1_edid = drm_get_edid(
				connector, &smi_connector->dp_adapter);
			if (sdev->dp1_edid)
#else
			ret = hw770_get_dp_edid(
				1, (unsigned char *)sdev->dp1_edid);
			if (ret)
#endif
			{
				dbg_msg("DP1 get edid success.\n");
				drm_connector_update_edid_property(connector, sdev->dp1_edid);
				count = drm_add_edid_modes(connector, sdev->dp1_edid);
			}

			if (sdev->dp1_edid == NULL || count == 0)
			{
				pr_err("[fail] DP1 get edid fail.\n");
				drm_connector_update_edid_property(connector, NULL);
				count = drm_add_modes_noedid(connector, 3840, 2160);
				drm_set_preferred_mode(connector, fixed_width, fixed_height);
			}

		}
		if (connector->connector_type == DRM_MODE_CONNECTOR_HDMIA) {
			count = hdmi_get_edid_property(connector,
						       sdev->hdmi0_edid,
						       USE_HDMI0, INDEX_HDMI0,
						       retry);
		}
		if (connector->connector_type == DRM_MODE_CONNECTOR_HDMIB) {
			count = hdmi_get_edid_property(connector,
						       sdev->hdmi1_edid,
						       USE_HDMI1, INDEX_HDMI1,
						       retry);
		}
		if (connector->connector_type == DRM_MODE_CONNECTOR_DVID) {
			count = hdmi_get_edid_property(connector,
						       sdev->hdmi2_edid,
						       USE_HDMI2, INDEX_HDMI2,
						       retry);
		}
	}
	LEAVE(count);
}

static enum drm_mode_status smi_connector_mode_valid(struct drm_connector *connector,
													 struct drm_display_mode *mode)
{
	struct smi_device *sdev = connector->dev->dev_private;
	
	u32 vrefresh = drm_mode_vrefresh(mode);	
	
	if ((vrefresh < 29) || (vrefresh > 61) || (vrefresh > 31 && vrefresh < 59)){  
		if(!edid_mode)
			return MODE_NOCLOCK;
	}

	if(sdev->specId != SPC_SM770){

	if ((mode->hdisplay > 3840) || (mode->vdisplay > 2160) || (mode->clock > 297000))
		 return MODE_NOMODE;
	
	
	if(mode->hdisplay > 1920) {
		if ((sdev->m_connector == USE_DVI_HDMI) || (sdev->m_connector == USE_VGA_HDMI)||(sdev->specId == SPC_SM750))
			return MODE_NOMODE;
	}

	if((sdev->specId == SPC_SM750) && (mode->clock >= 80000) && (sdev->m_connector == USE_DVI_VGA))
		return MODE_NOMODE;

	if(connector->connector_type == DRM_MODE_CONNECTOR_DVII){
		if(mode->clock >= 200000)
				return MODE_NOCLOCK;
	}



	if(lvds_channel && (!lcd_scale)){
		if (connector->connector_type == DRM_MODE_CONNECTOR_DVII) {              
				if ((mode->hdisplay == fixed_width) && (mode->vdisplay == fixed_height))                
					return MODE_OK;    
				else                
					return MODE_NOMODE;
		}
	}

	}else{
			if((mode->hdisplay > 3840) || (mode->vdisplay > 2160) || (mode->clock > 600000))
				return MODE_NOMODE;
			if(mode->clock >=300000 && (count_set_bits(sdev->m_connector) > 2))  //SM770 can't support triple 4k@60hz
				return MODE_NOCLOCK;
			//For Xorg, if sram is 256Mb, can not support triple 4k@30hz
			if((sdev->vram_size == MB(256)) &&  (mode->clock >= 250000) && (count_set_bits(sdev->m_connector) > 2))
				return MODE_NOMODE;
	}
		

	return MODE_OK;

}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
struct drm_encoder *smi_connector_best_encoder(struct drm_connector *connector)
{
	int enc_id = connector->encoder_ids[0];
	if (enc_id)
		return drm_encoder_find(connector->dev, NULL, enc_id);

	return NULL;
}
#endif


static enum drm_connector_status smi_connector_detect(struct drm_connector
														  *connector,
													  bool force)
{
	struct smi_connector *smi_connector = to_smi_connector(connector);
	struct smi_device *sdev = connector->dev->dev_private;
	int ret = 0;	
#ifdef USE_HDMICHIP

	void *edid_buf;
#endif

	if (sdev->specId == SPC_SM750)
	{
		if (connector->connector_type == DRM_MODE_CONNECTOR_DVII)
		{

			if (force_connect & USE_DVI){
					sdev->m_connector = sdev->m_connector | USE_DVI;
					return connector_status_connected;
			}else if(force_connect){
					sdev->m_connector = sdev->m_connector & (~USE_DVI);
					return connector_status_disconnected;
			}
#ifdef USE_HDMICHIP	
			if (ddk750_GetDDC_9022Access())
				ret = ddk750_edidReadMonitorEx(CHANNEL0_CTRL, edid_buf, 128, 0, 30, 31);
			ddk750_Release9022DDC();
			if (ret)
			{
				dbg_msg("detect sii9022 HDMI connected.\n");
				return connector_status_connected;
			}
			else
			{
				dbg_msg("detect sii9022 HDMI disconnected.\n");
				return connector_status_disconnected;
			}
#endif
			if (hwi2c_en)

                ret = ddk750_edidHeaderReadMonitorExHwI2C();

            else
				ret = drm_probe_ddc(&smi_connector->adapter);

			if (ret)
			{
				sdev->m_connector = sdev->m_connector | USE_DVI;
				dbg_msg("detect DVI/Panel connected.\n");
				return connector_status_connected;
			}
			else
			{
				sdev->m_connector = sdev->m_connector & (~USE_DVI);
				dbg_msg("detect DVI/Panel DO NOT connected.\n");
				return connector_status_disconnected;
			}
			
		}
		else if (connector->connector_type == DRM_MODE_CONNECTOR_VGA)
		{
			if (force_connect & USE_VGA){
					sdev->m_connector = sdev->m_connector | USE_VGA;
					return connector_status_connected;
			}else if(force_connect){
					sdev->m_connector = sdev->m_connector & (~USE_VGA);
					return connector_status_disconnected;
			}

			if(!drm_probe_ddc(&smi_connector->adapter))
			{
				
				sdev->m_connector = sdev->m_connector & (~USE_VGA);
				dbg_msg("detect CRT DO NOT connected.\n");
				return connector_status_disconnected;
			}
			else
			{
				sdev->m_connector = sdev->m_connector | USE_VGA;
				dbg_msg("detect CRT connected.\n");
				return connector_status_connected;
			}
			
		}
		else
			return connector_status_unknown;
	}
	else if(sdev->specId == SPC_SM768)  //SM768 Part
	{
		if(connector->connector_type == DRM_MODE_CONNECTOR_DVII)
		{
			if ((force_connect & USE_DVI) || (lvds_channel)){
				sdev->m_connector = sdev->m_connector | USE_DVI;
				return connector_status_connected;
			}else if(force_connect){
					sdev->m_connector = sdev->m_connector & (~USE_DVI);
					return connector_status_disconnected;
			}

			if (hwi2c_en)
                ret = ddk768_edidHeaderReadMonitorExHwI2C(0);
            else
				ret = drm_probe_ddc(&smi_connector->adapter);

			if (ret)
			{
				dbg_msg("detect DVI connected(GPIO30,31)\n");
				sdev->m_connector =sdev->m_connector |USE_DVI;
				return connector_status_connected;
			}
			else
			{
				dbg_msg("detect DVI DO NOT connected. \n");
				sdev->m_connector =sdev->m_connector & (~USE_DVI);
				return connector_status_disconnected;
			}

		}
		else if (connector->connector_type == DRM_MODE_CONNECTOR_VGA)
		{
			if (force_connect & USE_VGA){
				sdev->m_connector = sdev->m_connector | USE_VGA;
				return connector_status_connected;
			}else if(force_connect){
					sdev->m_connector = sdev->m_connector & (~USE_VGA);
					return connector_status_disconnected;
			}

			if (hwi2c_en)
			    ret = ddk768_edidHeaderReadMonitorExHwI2C(1);
			else
				ret = drm_probe_ddc(&smi_connector->adapter);	

			if (ret)
			{
				dbg_msg("detect CRT connected(GPIO 6, 7)\n");
				sdev->m_connector = sdev->m_connector|USE_VGA;
				return connector_status_connected;
			}
			else
			{
				dbg_msg("detect CRT DO NOT connected. \n");
				sdev->m_connector = sdev->m_connector&(~USE_VGA);
				return connector_status_disconnected;
			}
		
		}
		else if (connector->connector_type == DRM_MODE_CONNECTOR_HDMIA)
		{

			if ((sdev->m_connector == USE_DVI_VGA) || (sdev->m_connector == USE_ALL))
			{
				HDMI_Disable_Output();
				dbg_msg("set HDMI connector_status_disconnected because of VGA+DVI\n");
				sdev->m_connector = sdev->m_connector&(~USE_HDMI);
				return connector_status_disconnected;  //If VGA and DVI are both connected, disable HDMI
			}

			if (force_connect & USE_HDMI){
				sdev->m_connector = sdev->m_connector | USE_HDMI;
				return connector_status_connected;
			}else if(force_connect){
					sdev->m_connector = sdev->m_connector & (~USE_HDMI);
					return connector_status_disconnected;
			}

#if 0//ndef AUDIO_EN
			if (hdmi_hotplug_detect())
#else
			if (drm_probe_ddc(&smi_connector->adapter))
#endif
			{
				dbg_msg("detect HDMI connected(GPIO 8,9) \n");
				sdev->m_connector = sdev->m_connector|USE_HDMI;
				return connector_status_connected; 
			}
			else if(HDMI_connector_detect())
			{
			    dbg_msg("detect HDMI connected(HPG pull high) \n");
				sdev->m_connector = sdev->m_connector|USE_HDMI;
				return connector_status_connected; 
			}
			else
			{
				dbg_msg("detect HDMI DO NOT connected. \n");
				sdev->m_connector = sdev->m_connector&(~USE_HDMI);
				return connector_status_disconnected;
			}
		
		}
		else
			return connector_status_unknown;
	}
	else /*if(sdev->specId == SPC_SM770)  //SM770 Part   */
	{
		if (connector->connector_type == DRM_MODE_CONNECTOR_DisplayPort)
		{
			if (sdev->m_connector & USE_HDMI0)
			{
				hw770_DP_Disable_Output(0);
				hw770_DP_Clear_Channel(0);
				dbg_msg("set DP0 connector_status_disconnected because HDMI0 connect\n");
				sdev->m_connector = sdev->m_connector&(~USE_DP0);
				return connector_status_disconnected;  
			}

			if (force_connect & USE_DP0){
				sdev->m_connector = sdev->m_connector | USE_DP0;
				return connector_status_connected;
			}else if(force_connect){
				sdev->m_connector = sdev->m_connector & (~USE_DP0);
				return connector_status_disconnected;
			}

			if (DP_HPD_Detect(0))
			{
			
				dbg_msg("detect DP0 connected\n");
				sdev->m_connector = sdev->m_connector|USE_DP0;
				ret = hw770_dp_check_sink_status(0);
				if(ret)
					smi_dp_set_mode(sdev, 0);

				return connector_status_connected;
			}
			else
			{
				dbg_msg("detect DP0 DO NOT connected. \n");
				hw770_DP_Clear_Channel(0);
				sdev->m_connector =sdev->m_connector&(~USE_DP0);
				return connector_status_disconnected;

			}
		}
		else if (connector->connector_type == DRM_MODE_CONNECTOR_eDP)
		{

			if (sdev->m_connector & USE_HDMI1)
			{
				hw770_DP_Disable_Output(1);
				hw770_DP_Clear_Channel(1);
				dbg_msg("set DP1 connector_status_disconnected because HDMI1 connect\n");
				sdev->m_connector = sdev->m_connector&(~USE_DP1);
				return connector_status_disconnected;  
			}


			if (force_connect & USE_DP1){
				sdev->m_connector = sdev->m_connector | USE_DP1;
				return connector_status_connected;
			}else if(force_connect){
				sdev->m_connector = sdev->m_connector & (~USE_DP1);
				return connector_status_disconnected;
			}

			if (DP_HPD_Detect(1))
			{
				dbg_msg("detect DP1 connected\n");
				sdev->m_connector = sdev->m_connector|USE_DP1;
				ret = hw770_dp_check_sink_status(1);
				if(ret)
					smi_dp_set_mode(sdev, 1);

				return connector_status_connected;
				
				
			}
			else
			{
				dbg_msg("detect DP1 DO NOT connected. \n");
				hw770_DP_Clear_Channel(1);
				sdev->m_connector =sdev->m_connector&(~USE_DP1);
				return connector_status_disconnected;

			}
		}
		else if (connector->connector_type == DRM_MODE_CONNECTOR_HDMIA)
		{

			if (force_connect & USE_HDMI0){
				sdev->m_connector = sdev->m_connector | USE_HDMI0;
				return connector_status_connected;
			}else if(force_connect){
				hw770_HDMI_Disable_Output(0);
				sdev->m_connector = sdev->m_connector & (~USE_HDMI0);
				return connector_status_disconnected;
			}

			if(ddk770_HDMI_HPD_Detect(0))
			{
			    dbg_msg("detect HDMI0 connected \n");
				//hw770_hdmi_vga_mode(0);
				sdev->m_connector = sdev->m_connector | USE_HDMI0;
				return connector_status_connected; 
			}
			else
			{
				dbg_msg("detect HDMI0 DO NOT connected. \n");
				hw770_HDMI_Disable_Output(0);
				sdev->m_connector = sdev->m_connector&(~USE_HDMI0);
				return connector_status_disconnected;
			}
		
		}
		else if (connector->connector_type == DRM_MODE_CONNECTOR_HDMIB)
		{

			if (force_connect & USE_HDMI1){
				sdev->m_connector = sdev->m_connector | USE_HDMI1;
				return connector_status_connected;
			}else if(force_connect){
				hw770_HDMI_Disable_Output(1);
				sdev->m_connector = sdev->m_connector & (~USE_HDMI1);
				return connector_status_disconnected;
			}

			if(ddk770_HDMI_HPD_Detect(1))
			{
			    dbg_msg("detect HDMI1 connected \n");
				//hw770_hdmi_vga_mode(1);
				sdev->m_connector = sdev->m_connector | USE_HDMI1;
				return connector_status_connected; 
			}
			else
			{
				dbg_msg("detect HDMI1 DO NOT connected. \n");
				hw770_HDMI_Disable_Output(1);
				sdev->m_connector = sdev->m_connector&(~USE_HDMI1);
				return connector_status_disconnected;
			}
		
		}
		else if (connector->connector_type == DRM_MODE_CONNECTOR_DVID)
		{

			if (force_connect & USE_HDMI2){
				sdev->m_connector = sdev->m_connector | USE_HDMI2;
				return connector_status_connected;
			}else if(force_connect){
				hw770_HDMI_Disable_Output(2);
				sdev->m_connector = sdev->m_connector & (~USE_HDMI2);
				return connector_status_disconnected;
			}

			if(ddk770_HDMI_HPD_Detect(2))
			{
			    dbg_msg("detect HDMI2 connected \n");
				//hw770_hdmi_vga_mode(2);
				sdev->m_connector = sdev->m_connector | USE_HDMI2;
				return connector_status_connected; 
			}
			else
			{
				dbg_msg("detect HDMI2 DO NOT connected. \n");
				hw770_HDMI_Disable_Output(2);
				sdev->m_connector = sdev->m_connector&(~USE_HDMI2);
				return connector_status_disconnected;
			}
		
		}

		else
			return connector_status_unknown;
	}
}

static void smi_connector_destroy(struct drm_connector *connector)
{
	struct smi_device *sdev = connector->dev->dev_private;
	if (connector->connector_type == DRM_MODE_CONNECTOR_HDMIA && sdev->hdmi_edid)
	{
		kfree(sdev->hdmi_edid);
		sdev->hdmi_edid = NULL;
	}
	else if (connector->connector_type == DRM_MODE_CONNECTOR_DVII && sdev->dvi_edid)
	{
		kfree(sdev->dvi_edid);
		sdev->dvi_edid = NULL;
	}
	else if (connector->connector_type == DRM_MODE_CONNECTOR_VGA && sdev->vga_edid)
	{
		kfree(sdev->vga_edid);
		sdev->vga_edid = NULL;
	}


	if(sdev->specId == SPC_SM750)
	{
		hw750_AdaptI2CCleanBus(connector);
	} else if(sdev->specId == SPC_SM768)
	{
		hw768_AdaptI2CCleanBus(connector);
	} else if (sdev->specId == SPC_SM770)
	{
		//hw770_AdaptI2CCleanBus(connector);
	}

	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
	kfree(connector);
}


static const struct drm_connector_helper_funcs smi_vga_connector_helper_funcs = {
	.get_modes = smi_connector_get_modes,
	.mode_valid = smi_connector_mode_valid,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
	.best_encoder = smi_connector_best_encoder,
#endif
};

static const struct drm_connector_funcs smi_vga_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = smi_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = smi_connector_destroy,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static struct drm_connector *smi_connector_init(struct drm_device *dev, int index)
{
	struct drm_connector *connector;
	struct smi_connector *smi_connector;
	struct smi_device *sdev = dev->dev_private;

	smi_connector = kzalloc(sizeof(struct smi_connector), GFP_KERNEL);
	if (!smi_connector)
		return NULL;

	connector = &smi_connector->base;
	smi_connector->i2c_hw_enabled = false;

	if (sdev->specId == SPC_SM750 || sdev->specId == SPC_SM768) {
	switch (index)
	{
		case 0:
			drm_connector_init(dev, connector, &smi_vga_connector_funcs, DRM_MODE_CONNECTOR_DVII);
			break;
		case 1:
			drm_connector_init(dev, connector, &smi_vga_connector_funcs, DRM_MODE_CONNECTOR_VGA);
			break;
		case 2:
			drm_connector_init(dev, connector, &smi_vga_connector_funcs, DRM_MODE_CONNECTOR_HDMIA);
			break;
		default:
			dbg_msg("error index of Connector\n");
	}
	}else if(sdev->specId == SPC_SM770){
		switch (index)
		{
			case 0:
				drm_connector_init(dev, connector, &smi_vga_connector_funcs, DRM_MODE_CONNECTOR_DisplayPort);
				break;
			case 1:
				drm_connector_init(dev, connector, &smi_vga_connector_funcs, DRM_MODE_CONNECTOR_eDP);
				break;
			case 2:
				drm_connector_init(dev, connector, &smi_vga_connector_funcs, DRM_MODE_CONNECTOR_HDMIA);
				break;
			case 3:
				drm_connector_init(dev, connector, &smi_vga_connector_funcs, DRM_MODE_CONNECTOR_HDMIB);
				break;
			case 4:
				drm_connector_init(dev, connector, &smi_vga_connector_funcs, DRM_MODE_CONNECTOR_DVID);
				break;
			default:
				dbg_msg("error index of Connector\n");
		}
	}


	if (sdev->specId == SPC_SM750) 
	{
		hw750_AdaptI2CInit(smi_connector);
	}
	else if(sdev->specId == SPC_SM768)
	{
		hw768_AdaptI2CInit(smi_connector);
	}
	else if(sdev->specId == SPC_SM770)
	{
		hw770_AdaptI2CInit(smi_connector);
	}

	drm_connector_helper_add(connector, &smi_vga_connector_helper_funcs);

	if (sdev->specId == SPC_SM770)
		connector->polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT | DRM_CONNECTOR_POLL_HPD;	
	else
		connector->polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;

	drm_connector_register(connector);
	
	return connector;
}


int smi_modeset_init(struct smi_device *cdev)
{
	int index, max_enc = MAX_ENCODER(cdev->specId), max_ctrl = MAX_CRTC(cdev->specId);
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct smi_crtc *smi_crtc;
	
	if(!lvds_channel)
		lcd_scale = 0;

	if(lcd_scale)
		swcur_en = 1;

	if(smi_bpp >= 24)
		smi_bpp = 32;

	if(cdev->specId == SPC_SM750)
		smi_bpp = 32;

#ifdef PRIME
	smi_bpp = 32;
#endif

	drm_mode_config_init(cdev->dev);
	cdev->mode_info.mode_config_initialized = true;

	cdev->dev->mode_config.min_width = 0;
	cdev->dev->mode_config.min_height = 0;


	if (cdev->specId == SPC_SM768) { 
	cdev->dev->mode_config.max_width = SMI_MAX_FB_WIDTH;
	cdev->dev->mode_config.max_height = SMI_MAX_FB_HEIGHT;
	}else if (cdev->specId == SPC_SM750) { 	
		cdev->dev->mode_config.max_width = 3840;
		cdev->dev->mode_config.max_height = 2160;
	}else if  (cdev->specId == SPC_SM770) { 	
		cdev->dev->mode_config.max_width = 16384;
		cdev->dev->mode_config.max_height = 16384;
	}
	cdev->dev->mode_config.cursor_width = CURSOR_WIDTH;
	cdev->dev->mode_config.cursor_height = CURSOR_HEIGHT;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
	cdev->dev->mode_config.fb_base = cdev->vram_base;
#endif
	cdev->dev->mode_config.preferred_depth = smi_bpp;
	cdev->dev->mode_config.prefer_shadow = 1;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
	cdev->dev->mode_config.prefer_shadow_fbdev = 1;
#endif

	for(index = 0; index < max_ctrl ; index ++)
	{
		smi_crtc = smi_crtc_init(cdev->dev, index);
		smi_crtc->crtc_index = index;
	}
	
	for(index = 0; index < max_enc ; index ++)
	{
		encoder = smi_encoder_init(cdev->dev, index);
		if (!encoder) {
			DRM_ERROR("smi_encoder_tmds_init failed\n");
			return -1;
		}
		cdev->smi_enc_tab[index] = encoder;

		connector = smi_connector_init(cdev->dev, index);
		if (!connector) {
			DRM_ERROR("smi_%s_init failed\n", index?"VGA":"DVI");
			return -1;
		}
		cdev->smi_conn_tab[index] = connector;
		drm_connector_attach_encoder(connector, encoder);

	}

	drm_mode_config_reset(cdev->dev);


#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
		if ((cdev->specId == SPC_SM750 && (cdev->dev->pdev->resource[PCI_ROM_RESOURCE].flags & IORESOURCE_ROM_SHADOW)) || cdev->specId == SPC_SM768 || cdev->specId == SPC_SM770) {
		int ret = 0;
		ret = drm_fbdev_generic_setup(cdev->dev, cdev->dev->mode_config.preferred_depth);
		if (ret) {
			DRM_ERROR("smi_fbdev_init failed\n");
			return ret;
		}
	}
#endif
	return 0;
}

void smi_modeset_fini(struct smi_device *cdev)
{

	if (cdev->mode_info.mode_config_initialized) {
		drm_atomic_helper_shutdown(cdev->dev);
		drm_mode_config_cleanup(cdev->dev);
		cdev->mode_info.mode_config_initialized = false;
	}
}


