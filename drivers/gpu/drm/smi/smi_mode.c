/*
 * Copyright 2016 SiliconMotion Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License version 2. See the file COPYING in the main
 * directory of this archive for more details.
 *
 */

#include "smi_drv.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
#include <drm/drm_fourcc.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
#include <drm/drm_pci.h>
#else
#include <linux/pci.h>
#endif
#include <drm/drm_vblank.h>
#else
#include <drm/drmP.h>
#endif
#include <drm/drm_atomic_helper.h>
#include <drm/drm_atomic_state_helper.h>
#include <drm/drm_gem.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_crtc_helper.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
#include <drm/drm_probe_helper.h>
#endif


#include "hw750.h"
#include "hw768.h"
#include "ddk768/ddk768_video.h"
#include "ddk768/ddk768_chip.h"
#include "smi_dbg.h"


struct smi_crtc * smi_crtc_tab[MAX_CRTC];
struct drm_encoder * smi_enc_tab[MAX_ENCODER];

//bit 0: DVI, bit 1: VGA, bit 2: HDMI.
int g_m_connector = 0;

int smi_calc_hdmi_ctrl(int m_connector)
{
		int smi_ctrl = 0;

		if(m_connector==USE_DVI_HDMI) // //vga is empty, dvi is occupied , HDMI use ctrl 1;
			smi_ctrl = 1;
		else
			smi_ctrl = 0;
			
		return smi_ctrl;

}

static int smi_crtc_gamma_set(struct drm_crtc *crtc, u16 *r, u16 *g,
				u16 *b, uint32_t size,
				struct drm_modeset_acquire_ctx *ctx)
{
	
	struct smi_crtc *smi_crtc = to_smi_crtc(crtc); 
	int i , max_index = 2, ctrl_index, dst_ctrl;
	ctrl_index = 0;
	dst_ctrl = 0;

	if(g_specId == SPC_SM768)
		max_index = MAX_ENCODER;

		
	for(i = 0;i < max_index; i++)
	{
		if(crtc == smi_enc_tab[i]->crtc)
		{
			ctrl_index = i;
			break;
		}
	}
	
	dst_ctrl = (ctrl_index == SMI1_CTRL)?SMI1_CTRL:SMI0_CTRL;

	if(ctrl_index > SMI1_CTRL)
	{
		dst_ctrl= smi_calc_hdmi_ctrl(g_m_connector);
	}


	for (i = 0; i < crtc->gamma_size; i++)
	{
		smi_crtc->lut_r[i] = r[i] >> 8;
		smi_crtc->lut_g[i] = g[i] >> 8;
		smi_crtc->lut_b[i] = b[i] >> 8;
	}


	if (g_specId == SPC_SM768) {
		hw768_setgamma(dst_ctrl, true , lvds_channel);
		hw768_load_lut(dst_ctrl, crtc->gamma_size, smi_crtc->lut_r, smi_crtc->lut_g, smi_crtc->lut_b);
	
	}else if(g_specId == SPC_SM750) {
		hw750_setgamma(dst_ctrl, true);
		hw750_load_lut(dst_ctrl, crtc->gamma_size, smi_crtc->lut_r, smi_crtc->lut_g, smi_crtc->lut_b);
	}
	
	return 0;
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

	ENTER();
	
	logicalMode.valid_edid = false;

	if (WARN_ON(!crtc->state))
		LEAVE();

	mode = &crtc->state->adjusted_mode;
	refresh_rate = drm_mode_vrefresh(mode);

	dbg_msg("***crtc addr:%p\n", crtc);

	dbg_msg("encode->crtc:[%p, %p, %p] \n", smi_enc_tab[0]->crtc, smi_enc_tab[1]->crtc,
		smi_enc_tab[2] == NULL ? NULL : smi_enc_tab[2]->crtc);
	dbg_msg("g_m_connector = %d,  DVI [%d], VGA[%d], HDMI[%d] \n", g_m_connector,
		g_m_connector & 0x1, g_m_connector & 0x2, g_m_connector & 0x4);

	dbg_msg("wxh:%dx%d@%ldHz\n", mode->hdisplay, mode->vdisplay, refresh_rate);

	if (g_specId == SPC_SM750) {
			if(crtc == smi_enc_tab[0]->crtc)
		{
			logicalMode.baseAddress = 0;
			logicalMode.x = mode->hdisplay;
			logicalMode.y = mode->vdisplay;
			logicalMode.bpp = smi_bpp;
			logicalMode.dispCtrl = SMI0_CTRL;
			logicalMode.hz = refresh_rate;
			logicalMode.pitch = 0;

			setMode(&logicalMode);

			setDisplayControl(SMI0_CTRL, DISP_ON);           /* Turn on Primary Control */
			setPath(SMI0_PATH, SMI0_CTRL, DISP_ON);     /* Turn on Panel Path and use Primary data */
			
		}
		if(crtc == smi_enc_tab[1]->crtc)
		{
			logicalMode.baseAddress = 0;
			logicalMode.x = mode->hdisplay;
			logicalMode.y = mode->vdisplay;
			logicalMode.bpp = smi_bpp;
			logicalMode.dispCtrl = SMI1_CTRL;
			logicalMode.hz = refresh_rate;
			logicalMode.pitch = 0;
			setMode(&logicalMode);

			setDisplayControl(SMI1_CTRL, DISP_ON);         /* Turn on Secondary control */
			setPath(SMI1_PATH, SMI1_CTRL, DISP_ON);     /* Turn on CRT Path and use Secondary data */
			
		}
		
		swPanelPowerSequence(DISP_ON, 4);                   /* Turn on Panel */
	    setDAC(DISP_ON);                                    /* Turn on DAC */

#ifdef USE_HDMICHIP
		printk("HDMI set mode\n");
		sii9022xSetMode(5);
#endif

	}
	else {
		int i, ctrl_index, dst_ctrl;
		ctrl_index = 0;
		dst_ctrl = 0;
		for(i = 0;i < MAX_ENCODER; i++)
		{
			if(crtc == smi_enc_tab[i]->crtc)
			{
				ctrl_index = i;
				break;
			}
		}

		dst_ctrl = (ctrl_index == SMI1_CTRL)?SMI1_CTRL:SMI0_CTRL;
		
		if(ctrl_index > SMI1_CTRL)
		{
			dst_ctrl=smi_calc_hdmi_ctrl(g_m_connector);
			dbg_msg("hdmi use channel %d\n",dst_ctrl);
	
		}
		
		logicalMode.baseAddress = 0;
		logicalMode.x = mode->hdisplay;
		logicalMode.y = mode->vdisplay;
		logicalMode.bpp = smi_bpp;
		logicalMode.hz = refresh_rate;
		logicalMode.pitch = 0;
		logicalMode.dispCtrl = dst_ctrl;

		switch (ctrl_index)
		{
			case 0:
				if (drm_edid_header_is_valid((u8 *)sdev->dvi_edid) == 8)
					logicalMode.valid_edid = true;
				break;
			case 1:
				if (drm_edid_header_is_valid((u8 *)sdev->vga_edid) == 8)
					logicalMode.valid_edid = true;
				break;
			case 2:
				if (drm_edid_header_is_valid((u8 *)sdev->hdmi_edid) == 8)
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
		if((g_m_connector & USE_HDMI)&&(ctrl_index > SMI1_CTRL))
		{
			int ret = 0;
			printk("Starting init SM768 HDMI! Use Channel [%d]\n", dst_ctrl);
			if(dst_ctrl == 0)
				hw768_SetPixelClockFormat(dst_ctrl,0);
			ret=hw768_set_hdmi_mode(&logicalMode, *mode, sdev->is_hdmi);
			if (ret != 0)
			{
				printk("HDMI Mode not supported!\n");
			}
		}
		

		if(lvds_channel == 1){
			printk("Use Single Channel LVDS\n");
			hw768_enable_lvds(1);
		}
		else if(lvds_channel == 2){
			printk("Use Dual Channel LVDS\n");
			hw768_enable_lvds(2);
			EnableDoublePixel(0);
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
	ENTER();
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
	ENTER();
	LEAVE();
}

static void smi_crtc_atomic_disable(struct drm_crtc *crtc, 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
		struct drm_atomic_state *state)
#else
		struct drm_crtc_state *old_state)
#endif
{
	ENTER();
	LEAVE();
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
static int smi_enable_vblank(struct drm_crtc *crtc)
{
	if (g_specId == SPC_SM750) {
		hw750_en_dis_interrupt(1);
	} else if (g_specId == SPC_SM768) {
		hw768_en_dis_interrupt(1);
	}
	return 0;
}

static void smi_disable_vblank(struct drm_crtc *crtc)
{
	if (g_specId == SPC_SM750) {
		hw750_en_dis_interrupt(0);
	} else if (g_specId == SPC_SM768) {
		hw768_en_dis_interrupt(0);
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
	.gamma_set = smi_crtc_gamma_set,
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

	drm_mode_crtc_set_gamma_size(&smi_crtc->base, 256);
	
	for (i = 0; i < smi_crtc->base.gamma_size; i++)
	{
		smi_crtc->lut_r[i] = smi_crtc->lut_g[i] = smi_crtc->lut_b[i] = i;
	}
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
	int index =0;
	ENTER();
	if (encoder->encoder_type  == DRM_MODE_ENCODER_LVDS)
		index =  0;
	else if (encoder->encoder_type  == DRM_MODE_ENCODER_DAC)
		index =  1;
	else if (encoder->encoder_type  == DRM_MODE_ENCODER_TMDS)
		index = 2;
	

	dbg_msg("The current connect group = [%d], we deal with con=[%d], mode=[%s]\n", g_m_connector,index, (mode == DRM_MODE_DPMS_OFF)?"Off":"ON");
	if(g_specId == SPC_SM750)
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
	}else if(g_specId == SPC_SM768)
	{

		if(encoder->encoder_type  == DRM_MODE_ENCODER_LVDS) 
		{
			if(g_m_connector == USE_VGA_HDMI||g_m_connector==USE_HDMI)
			{
				dbg_msg("DVI connector off\n");
				LEAVE();
			}
			dbg_msg("DVI connector: index=%d\n",index);
	
		}
		else if(encoder->encoder_type  == DRM_MODE_ENCODER_DAC)
		{
			if(g_m_connector == USE_DVI_HDMI)
			{
				dbg_msg("VGA connector off\n");
				LEAVE();
			}
			dbg_msg("VGA connector: index=%d\n",index);
		}
		else if(encoder->encoder_type  == DRM_MODE_ENCODER_TMDS)
		{	
			if (mode == DRM_MODE_DPMS_OFF)	
				hw768_HDMI_Disable_Output();
			else
				hw768_HDMI_Enable_Output();
			if(g_m_connector == USE_DVI_HDMI){
				index = SMI1_CTRL;
			 	dbg_msg("HDMI connector: index=%d\n",index);
			}
			else if(g_m_connector == USE_VGA_HDMI || g_m_connector==USE_HDMI){
				index = SMI0_CTRL;
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
		
		if(lvds_channel == 2)
			EnableDoublePixel(0);

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

void smi_encoder_destroy(struct drm_encoder *encoder)
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

	smi_encoder = kzalloc(sizeof(struct smi_encoder), GFP_KERNEL);
	if (!smi_encoder)
		return NULL;

	encoder = &smi_encoder->base;
	encoder->possible_crtcs = (1 << index);

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

	drm_encoder_helper_add(encoder, &smi_encoder_helper_funcs);
 	return encoder;
}


int smi_connector_get_modes(struct drm_connector *connector)
{
	int ret = 0, count = 0;
	void *edid_buf;
	struct smi_device *sdev = connector->dev->dev_private;

	ENTER();
	dbg_msg("print connector type: [%d], DVI=%d, VGA=%d, HDMI=%d\n",
			connector->connector_type, DRM_MODE_CONNECTOR_DVII, DRM_MODE_CONNECTOR_VGA, DRM_MODE_CONNECTOR_HDMIA);

	if(!fixed_width || !fixed_height){
		fixed_width = 1024;
		fixed_height = 768;
	}
	
	
	if(g_specId == SPC_SM750)
	{
		if(connector->connector_type == DRM_MODE_CONNECTOR_DVII)
		{
#ifdef USE_HDMICHIP

			edid_buf = sdev->dvi_edid;
			if(ddk750_GetDDC_9022Access())
				ret = ddk750_edidReadMonitorEx(SMI0_CTRL, edid_buf, 256, 0, 30, 31);
			ddk750_Release9022DDC();
			if(ret){
              	drm_connector_update_edid_property(connector, sdev->dvi_edid);
                count = drm_add_edid_modes(connector, sdev->dvi_edid);
	
            }
                if (ret == 0 || count == 0)
                {
	
                        drm_connector_update_edid_property(connector, NULL);
                        count = drm_add_modes_noedid(connector, 1920, 1080);
                        drm_set_preferred_mode(connector, fixed_width, fixed_height);
                }


#else
			edid_buf = sdev->dvi_edid;

			if(hwi2c_en)
				ret = ddk750_edidReadMonitorEx_HW(SMI0_CTRL, edid_buf, 256, 0);
			else
				ret = ddk750_edidReadMonitorEx(SMI0_CTRL, edid_buf, 256, 0, 30, 31);
	
			dbg_msg("DVI edid size= %d\n",ret);
			if(ret){
				drm_connector_update_edid_property(connector, sdev->dvi_edid); 
				count = drm_add_edid_modes(connector, sdev->dvi_edid);
			}
			if (ret == 0 || count == 0)
			{
				drm_connector_update_edid_property(connector, NULL);
				count = drm_add_modes_noedid(connector, 1920, 1080);
				drm_set_preferred_mode(connector, fixed_width, fixed_height);
			}
				
#endif
		}
		if(connector->connector_type == DRM_MODE_CONNECTOR_VGA)
		{
			edid_buf = sdev->vga_edid;
			
			ret = ddk750_edidReadMonitorEx(SMI1_CTRL, edid_buf, 256, 0, 17, 18);

			if(ret){

				drm_connector_update_edid_property(connector, sdev->vga_edid); 

				count = drm_add_edid_modes(connector, sdev->vga_edid);;
			}
			if (ret == 0 || count == 0)
			{

				drm_connector_update_edid_property(connector, NULL);

				count = drm_add_modes_noedid(connector, 1920, 1080);
				drm_set_preferred_mode(connector, fixed_width, fixed_height);
			}
				
		}
	}
	else 
	{ //SM768 Part

		if(connector->connector_type == DRM_MODE_CONNECTOR_DVII)
		{
			if(lvds_channel){

				drm_connector_update_edid_property(connector, NULL);

				count = drm_add_modes_noedid(connector, fixed_width, fixed_height);
				drm_set_preferred_mode(connector, fixed_width, fixed_height);
			}else{
			
				edid_buf = sdev->dvi_edid;

				if(hwi2c_en)
					ret = ddk768_edidReadMonitorExHwI2C(edid_buf,256,0,0);
				else					
			    	ret = ddk768_edidReadMonitorEx(edid_buf, 256, 0, 30, 31 );//GPIO 30,31 for DVI,HW I2C0   

				dbg_msg("DVI edid size= %d\n",ret);

				if(ret){

					drm_connector_update_edid_property(connector, sdev->dvi_edid); 

					count = drm_add_edid_modes(connector, sdev->dvi_edid);
				}
				if (ret == 0 || count == 0)
				{

					drm_connector_update_edid_property(connector, NULL);

					count = drm_add_modes_noedid(connector, 1920, 1080);
					drm_set_preferred_mode(connector, fixed_width, fixed_height);
				}
			}

		}
		if(connector->connector_type == DRM_MODE_CONNECTOR_VGA)
		{
			edid_buf = sdev->vga_edid;

			if(hwi2c_en)		
				ret = ddk768_edidReadMonitorExHwI2C(edid_buf,256,0,1);
			else
				ret = ddk768_edidReadMonitorEx(edid_buf, 256, 0, 6, 7);//GPIO 6,7 for VGA,HW I2C1 

			dbg_msg("VGA edid size= %d\n",ret);		
		       
			if(ret){
				drm_connector_update_edid_property(connector, sdev->vga_edid);
				count = drm_add_edid_modes(connector, sdev->vga_edid);
			}
			if (ret == 0 || count == 0)
			{

				drm_connector_update_edid_property(connector, NULL);
				count = drm_add_modes_noedid(connector, 1920, 1080);
				drm_set_preferred_mode(connector, fixed_width, fixed_height);
			}

		}
		if(connector->connector_type == DRM_MODE_CONNECTOR_HDMIA)
		{
			edid_buf = sdev->hdmi_edid;

			ret = ddk768_edidReadMonitorEx(edid_buf, 256, 0, 8,9 ); //use GPIO8/9 for HDMI,
			dbg_msg("HDMI edid size= %d\n",ret);
            //hw768_get_hdmi_edid(tmpedid);   // Too Slow..
			if(ret){

				drm_connector_update_edid_property(connector, sdev->hdmi_edid);		
				count = drm_add_edid_modes(connector, sdev->hdmi_edid);
				sdev->is_hdmi = drm_detect_hdmi_monitor(sdev->hdmi_edid);
                dbg_msg("HDMI connector is %s\n",(sdev->is_hdmi ? "HDMI monitor" : "DVI monitor"));
			}
			if (ret == 0 || count == 0)
			{
				drm_connector_update_edid_property(connector, NULL);
				count = drm_add_modes_noedid(connector, 1920, 1080);
				drm_set_preferred_mode(connector, fixed_width, fixed_height);
				sdev->is_hdmi = true;
			}

		}

	}
	
	LEAVE(count);
}

static enum drm_mode_status smi_connector_mode_valid(struct drm_connector *connector,
				 struct drm_display_mode *mode)
{
	u32 vrefresh = drm_mode_vrefresh(mode);	
	
	if ((vrefresh < 29) || (vrefresh > 61) || (vrefresh > 31 && vrefresh < 59)){  
		if(!edid_mode)
			return MODE_NOCLOCK;
	}

	if ((mode->hdisplay > 3840) || (mode->vdisplay > 2160) || (mode->clock > 297000))
		 return MODE_NOMODE;
	
	
	if(mode->hdisplay > 1920) {
		if ((g_m_connector == USE_DVI_HDMI) || (g_m_connector == USE_VGA_HDMI)||(g_specId == SPC_SM750))
			return MODE_NOMODE;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0))	
	if ((mode->hdisplay == 1366) && (mode->vdisplay == 768))
		return MODE_NOMODE;
#endif

	if(lvds_channel && (!lcd_scale)){
		if (connector->connector_type == DRM_MODE_CONNECTOR_DVII) {              
				if ((mode->hdisplay == fixed_width) && (mode->vdisplay == fixed_height))                
					return MODE_OK;    
				else                
					return MODE_NOMODE;
		}
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
	long ret = 0;

	if (g_specId == SPC_SM750)
	{
		if (connector->connector_type == DRM_MODE_CONNECTOR_DVII)
		{

			if (force_connect & USE_DVI){
					g_m_connector = g_m_connector | USE_DVI;
					return connector_status_connected;
			}else if(force_connect){
					g_m_connector = g_m_connector & (~USE_DVI);
					return connector_status_disconnected;
			}
#ifdef USE_HDMICHIP
			void *edid_buf;
			int ret = 0;
			if (ddk750_GetDDC_9022Access())
				ret = ddk750_edidReadMonitorEx(SMI0_CTRL, edid_buf, 128, 0, 30, 31);
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


			if(hwi2c_en)
				ret = ddk750_edidHeaderReadMonitorExHwI2C();
			else
				ret = ddk750_edidHeaderReadMonitorEx(30,31);

			if (ret < 0)
			{
				dbg_msg("detect DVI/Panel DO NOT connected.\n");
				return connector_status_disconnected;
			}
			else
			{
				dbg_msg("detect DVI/Panel connected.\n");
				return connector_status_connected;
			}
		}
		else if (connector->connector_type == DRM_MODE_CONNECTOR_VGA)
		{
			if (force_connect & USE_VGA){
					g_m_connector = g_m_connector | USE_VGA;
					return connector_status_connected;
			}else if(force_connect){
					g_m_connector = g_m_connector & (~USE_VGA);
					return connector_status_disconnected;
			}

#if 0
			if(ddk750_detectCRTMonitor(0, 0, 0) == 0)
			{	
				dbg_msg("detect CRT connected.\n");
				return connector_status_connected;
			}
			else
			{	
				dbg_msg("detect CRT DO NOT connected.\n");
				return connector_status_disconnected;
			}

#else

			if(ddk750_edidHeaderReadMonitorEx(17,18) < 0)	
			{
				dbg_msg("detect CRT DO NOT connected.\n");
				return connector_status_disconnected;
			}
			else
			{
				dbg_msg("detect CRT connected.\n");
				return connector_status_connected;
			}
#endif	
			
		}
		else
			return connector_status_unknown;
	}
	else  //SM768 Part
	{
		if(connector->connector_type == DRM_MODE_CONNECTOR_DVII)
		{
			if ((force_connect & USE_DVI) || (lvds_channel)){
				g_m_connector = g_m_connector | USE_DVI;
				return connector_status_connected;
			}else if(force_connect){
					g_m_connector = g_m_connector & (~USE_DVI);
					return connector_status_disconnected;
			}


			if(hwi2c_en)
				ret = ddk768_edidHeaderReadMonitorExHwI2C(0);
			else
				ret = ddk768_edidHeaderReadMonitorEx(30,31);

			if(ret)	
			{				
				dbg_msg("detect DVI DO NOT connected. \n");
				g_m_connector = g_m_connector & (~USE_DVI);
				return connector_status_disconnected; 
			}
			else
			{
				dbg_msg("detect DVI connected(GPIO30,31)\n");
				g_m_connector =g_m_connector |USE_DVI;
				return connector_status_connected;
			}

		}
		else if (connector->connector_type == DRM_MODE_CONNECTOR_VGA)
		{
			if (force_connect & USE_VGA){
				g_m_connector = g_m_connector | USE_VGA;
				return connector_status_connected;
			}else if(force_connect){
					g_m_connector = g_m_connector & (~USE_VGA);
					return connector_status_disconnected;
			}
		

			if(hwi2c_en)
				ret = ddk768_edidHeaderReadMonitorExHwI2C(1);
			else
				ret = ddk768_edidHeaderReadMonitorEx(6,7);
	
			if(ret)						
			{				
				dbg_msg("detect CRT DO NOT connected. \n");
				g_m_connector =g_m_connector&(~USE_VGA);
				return connector_status_disconnected;
			}
			else
			{
				dbg_msg("detect CRT connected(GPIO 6, 7)\n");
				g_m_connector = g_m_connector|USE_VGA;
				return connector_status_connected;
			}
		
		}
		else if (connector->connector_type == DRM_MODE_CONNECTOR_HDMIA)
		{

			if ((g_m_connector == USE_DVI_VGA) || (g_m_connector == USE_ALL))
			{
				hw768_HDMI_Disable_Output();
				dbg_msg("set HDMI connector_status_disconnected because of VGA+DVI\n");
				g_m_connector = g_m_connector&(~USE_HDMI);
				return connector_status_disconnected;  //If VGA and DVI are both connected, disable HDMI
			}

			if (force_connect & USE_HDMI){
				g_m_connector = g_m_connector | USE_HDMI;
				return connector_status_connected;
			}else if(force_connect){
					g_m_connector = g_m_connector & (~USE_HDMI);
					return connector_status_disconnected;
			}

#if 0//ndef AUDIO_EN
			if (hdmi_hotplug_detect())
#else
			if(ddk768_edidHeaderReadMonitorEx(8,9)==0)
#endif
			{
				dbg_msg("detect HDMI connected(GPIO 8,9) \n");
				g_m_connector = g_m_connector|USE_HDMI;
				return connector_status_connected; 
			}
			else if(HDMI_connector_detect())
			{
			    dbg_msg("detect HDMI connected(HPG pull high) \n");
				g_m_connector = g_m_connector|USE_HDMI;
				return connector_status_connected; 
			}
			else
			{
				dbg_msg("detect HDMI DO NOT connected. \n");
				g_m_connector = g_m_connector&(~USE_HDMI);
				return connector_status_disconnected;
			}
		
		}
		else
			return connector_status_unknown;
	}
}

static void smi_connector_destroy(struct drm_connector *connector)
{

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

	smi_connector = kzalloc(sizeof(struct smi_connector), GFP_KERNEL);
	if (!smi_connector)
		return NULL;

	connector = &smi_connector->base;

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
			printk("error index of Connector\n");
	}
	drm_connector_helper_add(connector, &smi_vga_connector_helper_funcs);
	connector->polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;

	drm_connector_register(connector);
	
	return connector;
}


int smi_modeset_init(struct smi_device *cdev)
{
	int index, max_index = 2;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct smi_crtc *smi_crtc;
	
	if(!lvds_channel)
		lcd_scale = 0;

	if(lcd_scale)
		swcur_en = 1;

	if(smi_bpp >= 24)
		smi_bpp = 32;

	if(g_specId == SPC_SM750)
		smi_bpp = 16;

	drm_mode_config_init(cdev->dev);
	cdev->mode_info.mode_config_initialized = true;

	cdev->dev->mode_config.min_width = 0;
	cdev->dev->mode_config.min_height = 0;
	cdev->dev->mode_config.max_width = SMI_MAX_FB_WIDTH;
	cdev->dev->mode_config.max_height = SMI_MAX_FB_HEIGHT;

	if (g_specId == SPC_SM750) { // limit to maximum size supported+		
		cdev->dev->mode_config.max_width = 3840;
		cdev->dev->mode_config.max_height = 2160;
	}
	cdev->dev->mode_config.cursor_width = 64;
	cdev->dev->mode_config.cursor_height = 64;

	cdev->dev->mode_config.fb_base = cdev->mc.vram_base;
	cdev->dev->mode_config.preferred_depth = smi_bpp;
	cdev->dev->mode_config.prefer_shadow = 1;
	cdev->dev->mode_config.prefer_shadow_fbdev = 1;


	for(index = 0; index < MAX_CRTC ; index ++)
	{
		smi_crtc = smi_crtc_init(cdev->dev, index);
		smi_crtc->crtc_index = index;
		smi_crtc_tab[index] = smi_crtc;
		dbg_msg("******smi_crtc_tab[%d]:%p******\n",index, smi_crtc_tab[index]);

	}
	
	if(g_specId == SPC_SM768)
		max_index = MAX_ENCODER;
	
	for(index = 0; index < max_index ; index ++)
	{
		encoder = smi_encoder_init(cdev->dev, index);
		if (!encoder) {
			DRM_ERROR("smi_encoder_tmds_init failed\n");
			return -1;
		}
		smi_enc_tab[index] = encoder;

		connector = smi_connector_init(cdev->dev, index);
		if (!connector) {
			DRM_ERROR("smi_%s_init failed\n", index?"VGA":"DVI");
			return -1;
		}
		
		drm_connector_attach_encoder(connector, encoder);

	}

	drm_mode_config_reset(cdev->dev);


#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
	if ((g_specId == SPC_SM750 && (cdev->dev->pdev->resource[PCI_ROM_RESOURCE].flags & IORESOURCE_ROM_SHADOW)) || g_specId == SPC_SM768) {
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


