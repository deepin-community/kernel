// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.

#include "ddk750/ddk750_mode.h"
#include "ddk750/ddk750_help.h"
#include "ddk750/ddk750_regdc.h"	
#include "ddk750/ddk750_defs.h"
#include "ddk750/ddk750_display.h"
#include "ddk750/ddk750_2d.h"
#include "ddk750/ddk750_power.h"
#include "ddk750/ddk750_edid.h"
#include "ddk750/ddk750_cursor.h"
#include "ddk750/ddk750_swi2c.h"
#include "ddk750/ddk750_hwi2c.h"


#ifdef USE_HDMICHIP
#include "ddk750/ddk750_sii9022.h"
#endif

#include "smi_ver.h"
#include "hw750.h"


struct smi_750_register{
	/* registers for save */
	uint32_t system_ctrl, misc_ctrl,gpio_mux, localmem_arbitration;
	uint32_t pcimem_arbitration, raw_int, int_status, int_mask;
	uint32_t current_gate, mode0_gate, mode1_gate, power_mode_ctrl;
	uint32_t pci_master_base, primary_pll_ctrl,	secondary_pll_ctrl,	vga_pll0_ctrl;
	uint32_t vga_pll1_ctrl,	mxclk_pll_ctrl,	vga_configuration;
	
	uint32_t de_stretch_format, de_masks, de_window_width, de_control;

	uint32_t primary_display_ctrl,primary_pan_ctrl,primary_color_key,primary_fb_address;
	uint32_t primary_fb_width, primary_window_width,primary_window_height, primary_plane_tl;
	uint32_t primary_plane_br, primary_horizontal_total, primary_horizontal_sync, primary_vertical_total;
	uint32_t primary_vertical_sync, primary_current_line;
	
	uint32_t secondary_display_ctrl, secondary_fb_address, secondary_fb_width;
	uint32_t secondary_horizontal_total, secondary_horizontal_sync;
	uint32_t secondary_vertical_total, secondary_vertical_sync;
	uint32_t secondary_auto_centering_tl, secondary_auto_centering_br;
	uint32_t secondary_scale, secondary_hwc_address, secondary_hwc_location;
	uint32_t secondary_hwc_color_12, secondary_hwc_color_3;
};



void hw750_suspend(struct smi_750_register * pSave)
{

		/* save mmio registers */

		pSave->system_ctrl = PEEK32(SYSTEM_CTRL);
		pSave->misc_ctrl = PEEK32(MISC_CTRL);
		pSave->gpio_mux = PEEK32(GPIO_MUX);
		pSave->localmem_arbitration = PEEK32(LOCALMEM_ARBITRATION);
		pSave->pcimem_arbitration = PEEK32(PCIMEM_ARBITRATION);
		pSave->raw_int = PEEK32(RAW_INT);
		pSave->int_status = PEEK32(INT_STATUS);
		pSave->int_mask = PEEK32(INT_MASK);
		pSave->current_gate = PEEK32(CURRENT_GATE);
		pSave->mode0_gate = PEEK32(MODE0_GATE);
		pSave->mode1_gate = PEEK32(MODE1_GATE);
		pSave->power_mode_ctrl = PEEK32(POWER_MODE_CTRL);
		pSave->pci_master_base = PEEK32(PCI_MASTER_BASE);
		pSave->primary_pll_ctrl = PEEK32(PRIMARY_PLL_CTRL);
		pSave->secondary_pll_ctrl = PEEK32(SECONDARY_PLL_CTRL);
		pSave->vga_pll0_ctrl = PEEK32(VGA_PLL0_CTRL);
		pSave->vga_pll1_ctrl = PEEK32(VGA_PLL1_CTRL);
		pSave->mxclk_pll_ctrl = PEEK32(MXCLK_PLL_CTRL);
		pSave->vga_configuration = PEEK32(VGA_CONFIGURATION);

#if 0// for 2d, JUST HOLD IT.
		pSave->de_stretch_format = PEEK32(DE_STRETCH_FORMAT);
		pSave->de_masks = PEEK32(DE_MASKS);
		pSave->de_window_width = PEEK32(DE_WINDOW_WIDTH);
		pSave->de_control = PEEK32(DE_CONTROL);
#endif	
		pSave->primary_display_ctrl = PEEK32(PRIMARY_DISPLAY_CTRL);
		pSave->primary_pan_ctrl = PEEK32(PRIMARY_PAN_CTRL);
		pSave->primary_color_key = PEEK32(PRIMARY_COLOR_KEY);
		pSave->primary_fb_address = PEEK32(PRIMARY_FB_ADDRESS);
		pSave->primary_fb_width = PEEK32(PRIMARY_FB_WIDTH);
		pSave->primary_window_width= PEEK32(PRIMARY_WINDOW_WIDTH);
		pSave->primary_window_height= PEEK32(PRIMARY_WINDOW_HEIGHT);
		pSave->primary_plane_tl= PEEK32(PRIMARY_PLANE_TL);
		pSave->primary_plane_br= PEEK32(PRIMARY_PLANE_BR);
		pSave->primary_horizontal_total = PEEK32(PRIMARY_HORIZONTAL_TOTAL);
		pSave->primary_horizontal_sync = PEEK32(PRIMARY_HORIZONTAL_SYNC);
		pSave->primary_vertical_total = PEEK32(PRIMARY_VERTICAL_TOTAL);
		pSave->primary_vertical_sync = PEEK32(PRIMARY_VERTICAL_SYNC);
		pSave->primary_current_line = PEEK32(PRIMARY_CURRENT_LINE);
#if 0	//for hw cursor, JUST HOLD IT.
		pSave->primary_hwc_address = PEEK32(PRIMARY_HWC_ADDRESS);
		pSave->primary_hwc_location = PEEK32(PRIMARY_HWC_LOCATION);
		pSave->primary_hwc_color_12 = PEEK32(PRIMARY_HWC_COLOR_12);
		pSave->primary_hwc_color_3 = PEEK32(PRIMARY_HWC_COLOR_3);
#endif	
	
		pSave->secondary_display_ctrl = PEEK32(SECONDARY_DISPLAY_CTRL);
		pSave->secondary_fb_address = PEEK32(SECONDARY_FB_ADDRESS);
		pSave->secondary_fb_width = PEEK32(SECONDARY_FB_WIDTH);
		pSave->secondary_horizontal_total = PEEK32(SECONDARY_HORIZONTAL_TOTAL);
		pSave->secondary_horizontal_sync = PEEK32(SECONDARY_HORIZONTAL_SYNC);
		pSave->secondary_vertical_total = PEEK32(SECONDARY_VERTICAL_TOTAL);
		pSave->secondary_vertical_sync = PEEK32(SECONDARY_VERTICAL_SYNC);
		pSave->secondary_scale = PEEK32(SECONDARY_SCALE);
		pSave->secondary_hwc_address = PEEK32(SECONDARY_HWC_ADDRESS);
		pSave->secondary_hwc_location = PEEK32(SECONDARY_HWC_LOCATION);
		pSave->secondary_hwc_color_12 = PEEK32(SECONDARY_HWC_COLOR_12);
		pSave->secondary_hwc_color_3 = PEEK32(SECONDARY_HWC_COLOR_3);
		pSave->secondary_auto_centering_tl = PEEK32(SECONDARY_AUTO_CENTERING_TL);
		pSave->secondary_auto_centering_br = PEEK32(SECONDARY_AUTO_CENTERING_BR);

}

void hw750_resume(struct smi_750_register * pSave)
{
	/* restore mmio registers */
	POKE32(SYSTEM_CTRL, pSave->system_ctrl);
	POKE32(MISC_CTRL, pSave->misc_ctrl);
	POKE32(GPIO_MUX, pSave->gpio_mux);
	POKE32(LOCALMEM_ARBITRATION, pSave->localmem_arbitration);
	POKE32(PCIMEM_ARBITRATION, pSave->pcimem_arbitration);
	POKE32(RAW_INT, pSave->raw_int);
	POKE32(INT_STATUS, pSave->int_status);
	POKE32(INT_MASK, pSave->int_mask);
	POKE32(CURRENT_GATE, pSave->current_gate);

	POKE32(MODE0_GATE, pSave->mode0_gate);
	POKE32(MODE1_GATE, pSave->mode1_gate);
	POKE32(POWER_MODE_CTRL, pSave->power_mode_ctrl);
	POKE32(PCI_MASTER_BASE, pSave->pci_master_base);
	POKE32(PRIMARY_PLL_CTRL, pSave->primary_pll_ctrl);
	POKE32(SECONDARY_PLL_CTRL, pSave->secondary_pll_ctrl);
	POKE32(VGA_PLL0_CTRL, pSave->vga_pll0_ctrl);
	POKE32(VGA_PLL1_CTRL, pSave->vga_pll1_ctrl);
	POKE32(MXCLK_PLL_CTRL, pSave->mxclk_pll_ctrl);
	POKE32(VGA_CONFIGURATION, pSave->vga_configuration);
	
    POKE32(PRIMARY_DISPLAY_CTRL, pSave->primary_display_ctrl);
	POKE32(PRIMARY_PAN_CTRL, pSave->primary_pan_ctrl);
	POKE32(PRIMARY_COLOR_KEY, pSave->primary_color_key );
    POKE32(PRIMARY_FB_ADDRESS, pSave->primary_fb_address);
    POKE32(PRIMARY_FB_WIDTH, pSave->primary_fb_width);
	POKE32(PRIMARY_WINDOW_WIDTH, pSave->primary_window_width);
	POKE32(PRIMARY_WINDOW_HEIGHT, pSave->primary_window_height);
	POKE32(PRIMARY_PLANE_TL, pSave->primary_plane_tl);
	POKE32(PRIMARY_PLANE_BR, pSave->primary_plane_br);
    POKE32(PRIMARY_HORIZONTAL_TOTAL, pSave->primary_horizontal_total);
    POKE32(PRIMARY_HORIZONTAL_SYNC, pSave->primary_horizontal_sync);
    POKE32(PRIMARY_VERTICAL_TOTAL, pSave->primary_vertical_total);
    POKE32(PRIMARY_VERTICAL_SYNC, pSave->primary_vertical_sync);
    POKE32(PRIMARY_CURRENT_LINE, pSave->primary_current_line);

    POKE32(SECONDARY_DISPLAY_CTRL, pSave->secondary_display_ctrl);	
    POKE32(SECONDARY_FB_ADDRESS,  pSave->secondary_fb_address);
    POKE32(SECONDARY_FB_WIDTH, pSave->secondary_fb_width);
    POKE32(SECONDARY_HORIZONTAL_TOTAL, pSave->secondary_horizontal_total);

    POKE32(SECONDARY_HORIZONTAL_SYNC,    pSave->secondary_horizontal_sync);
    POKE32(SECONDARY_VERTICAL_TOTAL, pSave->secondary_vertical_total);
    POKE32(SECONDARY_VERTICAL_SYNC, pSave->secondary_vertical_sync);
    POKE32(SECONDARY_SCALE, pSave->secondary_scale);

#if 0
	POKE32(SECONDARY_HWC_ADDRESS, pSave->secondary_hwc_address);
    POKE32(SECONDARY_HWC_LOCATION, pSave->secondary_hwc_location);
    POKE32(SECONDARY_HWC_COLOR_12, pSave->secondary_hwc_color_12);
    POKE32(SECONDARY_HWC_COLOR_3, pSave->secondary_hwc_color_3);
#endif
    POKE32(SECONDARY_AUTO_CENTERING_TL, pSave->secondary_auto_centering_tl);
    POKE32(SECONDARY_AUTO_CENTERING_BR, pSave->secondary_auto_centering_br);


#if 0	//HOLD FOR 2D
    POKE32(DE_STRETCH_FORMAT, pSave->de_stretch_format);
    POKE32(DE_MASKS, pSave->de_masks);
    POKE32(DE_WINDOW_WIDTH, pSave->de_window_width);
    POKE32(DE_CONTROL, pSave->de_control);
#endif

}

void hw750_set_base(int display,int pitch,int base_addr)
{	

	if(display == 0)
	{
		pokeRegisterDWord(PRIMARY_FB_WIDTH,
		          FIELD_VALUE(0, PRIMARY_FB_WIDTH, WIDTH, pitch)
		        | FIELD_VALUE(0, PRIMARY_FB_WIDTH, OFFSET, pitch));
		setDisplayBaseAddress(CHANNEL0_CTRL, base_addr);
	}
	else
	{
		pokeRegisterDWord(SECONDARY_FB_WIDTH,
		          FIELD_VALUE(0, SECONDARY_FB_WIDTH, WIDTH, pitch)
		        | FIELD_VALUE(0, SECONDARY_FB_WIDTH, OFFSET, pitch));
		setDisplayBaseAddress(CHANNEL1_CTRL, base_addr);
	}
}

void hw750_set_dpms(int display,int state)
{
	if(display == 0)
	{
		setDisplayControl(CHANNEL0_CTRL, state);           /* Turn on Primary Control */
		setPath(SMI0_PATH, CHANNEL0_CTRL, state); 
	}
	else
	{
		setDisplayControl(CHANNEL1_CTRL, state);         /* Turn on Secondary control */
		setPath(SMI1_PATH, CHANNEL1_CTRL, state);
	}
}

 
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
int hw750_en_dis_interrupt(int status, int pipe)
{
	if(status == 0)
	{
		pokeRegisterDWord(INT_MASK, 	(pipe == CHANNEL1_CTRL) ? 
		FIELD_SET(0, INT_MASK, SECONDARY_VSYNC, DISABLE):
		FIELD_SET(0, INT_MASK, PRIMARY_VSYNC, DISABLE));
	}
	else
	{
		pokeRegisterDWord(INT_MASK, 	(pipe == CHANNEL1_CTRL) ? 
		FIELD_SET(0, INT_MASK, SECONDARY_VSYNC, ENABLE):
		FIELD_SET(0, INT_MASK, PRIMARY_VSYNC, ENABLE));
	}
	return 0;

}
#else
int hw750_en_dis_interrupt(int status)
	{
		if(status == 0)
		{
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, SECONDARY_VSYNC, DISABLE));
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, PRIMARY_VSYNC, DISABLE)); 
		}
		else
		{
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, SECONDARY_VSYNC, ENABLE));
			pokeRegisterDWord(INT_MASK, FIELD_SET(0, INT_MASK, PRIMARY_VSYNC, ENABLE));  
		}
		return 0;
	}

#endif


int hw750_check_vsync_interrupt(int path)
{

	unsigned long value1,value2;
	
	value1 = peekRegisterDWord(RAW_INT);
	value2 = peekRegisterDWord(INT_MASK);
	
	if(path == CHANNEL0_CTRL)
	{
	    if ((FIELD_VAL_GET(value1, RAW_INT, PRIMARY_VSYNC) == RAW_INT_PRIMARY_VSYNC_ACTIVE)
			&&(FIELD_VAL_GET(value2, INT_MASK, PRIMARY_VSYNC) == INT_MASK_PRIMARY_VSYNC_ENABLE))
	    {
			return true;
		}
	}else{
		if ((FIELD_VAL_GET(value1, RAW_INT, SECONDARY_VSYNC) == RAW_INT_SECONDARY_VSYNC_ACTIVE)
			&&(FIELD_VAL_GET(value2, INT_MASK, SECONDARY_VSYNC) == INT_MASK_SECONDARY_VSYNC_ENABLE))
		{
			return true;
		}
	}
	return false;
}



void hw750_clear_vsync_interrupt(int path)
{

	unsigned long value;
		
	value = peekRegisterDWord(RAW_INT);
	
	if(path == CHANNEL0_CTRL)
	{
	    
		pokeRegisterDWord(RAW_INT, FIELD_SET(value, RAW_INT, PRIMARY_VSYNC, CLEAR));

	}else{
		
		pokeRegisterDWord(RAW_INT, FIELD_SET(value, RAW_INT, SECONDARY_VSYNC, CLEAR));	
		
	}

}

void ddk750_disable_IntMask(void)
{
	
    pokeRegisterDWord(INT_MASK, 0);
}


void hw750_setgamma(disp_control_t dispCtrl, unsigned long enable)
{
	unsigned long value;
	unsigned long regCtrl;



	if(dispCtrl == CHANNEL0_CTRL)
		regCtrl = PRIMARY_DISPLAY_CTRL;
	else
		regCtrl = SECONDARY_DISPLAY_CTRL;


	value = peekRegisterDWord(regCtrl);
	
	if (enable)
	    value = FIELD_SET(value, PRIMARY_DISPLAY_CTRL, GAMMA, ENABLE);
	else
	    value = FIELD_SET(value, PRIMARY_DISPLAY_CTRL, GAMMA, DISABLE);
	    
	pokeRegisterDWord(regCtrl, value);    
}

void hw750_load_lut(disp_control_t dispCtrl, int size, u8 lut_r[], u8 lut_g[], u8 lut_b[])
{
	unsigned int i, v;
	unsigned long regCtrl;

	if(dispCtrl == CHANNEL0_CTRL)
		regCtrl = PRIMARY_PALETTE_RAM;
	else
		regCtrl = SECONDARY_PALETTE_RAM;

	for (i = 0; i < size; i++) {
		v = (lut_r[i] << 16);
		v |= (lut_g[i] << 8);
		v |= lut_b[i];
		pokeRegisterDWord(regCtrl + (i * 4), v);
	}
}


long hw750_AdaptI2CInit(struct smi_connector *smi_connector)
{
    if(hwi2c_en && (smi_connector->base.connector_type == DRM_MODE_CONNECTOR_DVII))
    {
        smi_connector->i2c_hw_enabled = 1;
    }
    else
    {
        smi_connector->i2c_hw_enabled = 0;      
    }

    if(smi_connector->i2c_hw_enabled)
    {
        return ddk750_AdaptHWI2CInit(smi_connector);
    }
    else
    {
        return ddk750_AdaptSWI2CInit(smi_connector); 
    }
}


long hw750_AdaptI2CCleanBus(struct drm_connector *connector)
{
    struct smi_connector *smi_connector = to_smi_connector(connector);
    
    if(smi_connector->i2c_hw_enabled)
    {
        return ddk750_AdaptHWI2CCleanBus(smi_connector);
    }
    else
    {
        return ddk750_AdapSWI2CCleanBus(smi_connector); 
    }

    return 0;
}




