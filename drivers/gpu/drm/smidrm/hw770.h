// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.


#ifndef LYNX_HW770_H__
#define LYNX_HW770_H__

#include "hw_com.h"


struct smi_770_register{
	uint32_t clock_enable, pll_ctrl[3];
	
	uint32_t primary_display_ctrl[10], lvds_ctrl, primary_hwcurs_ctrl[4];
	uint32_t secondary_display_ctrl[10], secondary_hwcurs_ctrl[4];
};

struct smi_770_fb_info{
	unsigned int fb_addr;
	unsigned long fb_pitch;

};

void hw770_setDisplayPlaneDisableOnly(
   disp_control_t dispControl /* Channel 0 or Channel 1) */
);


void hw770_enable_lvds(int channels);

void ddk770_set_mmio(volatile unsigned char * addr,unsigned short devId,char revId);
unsigned long ddk770_getFrameBufSize(void);
long ddk770_initChip(void);


void ddk770_swPanelPowerSequence(
    disp_control_t dispControl, 
    disp_state_t dispState, 
    disp_control_t dataPath,
    unsigned long vSyncDelay
);


int hw770_get_hdmi_edid(hdmi_index index, unsigned char *pEDIDBuffer);
int hw770_get_dp_edid(dp_index index, unsigned char *pEDIDBuffer);


int hw770_set_dp_mode(logicalMode_t *pLogicalMode, struct drm_display_mode mode, int dp_index);



void DP_Enable_Output(dp_index index);

void hw770_DP_Disable_Output(dp_index index);


/*
 * Disable double pixel clock. 
 * This is a teporary function, used to patch for the random fuzzy font problem. 
 */
void hw770_SetPixelClockFormat(disp_control_t dispControl,unsigned int is_half);

void ddk770_EnableChannelTiming(
   disp_control_t dispControl, /* Channel 0 or Channel 1) */
   unsigned short enable
);

/*
 * This function initializes the cursor attributes.
 */
void ddk770_initCursor(
    disp_control_t dispControl,     /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long base,             /* Base Address */ 
    unsigned long color1,           /* Cursor color 1 in RGB 5:6:5 format */
    unsigned long color2,          /* Cursor color 2 in RGB 5:6:5 format */
    unsigned long color3          /* Cursor color 3 in RGB 5:6:5 format */
);

/*
 * This function sets the cursor position.
 */
void ddk770_setCursorPosition(
    disp_control_t dispControl,     /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long dx,               /* X Coordinate of the cursor */
    unsigned long dy,               /* Y Coordinate of the cursor */
    unsigned char topOutside,       /* Top Boundary Select: either partially outside (= 1) 
                                       or within the screen top boundary (= 0) */
    unsigned char leftOutside,       /* Left Boundary Select: either partially outside (= 1) 
                                       or within the screen left boundary (= 0) */
    unsigned int enable
);
 
void hw770_set_base(disp_control_t dispControl,int pitch,int base_addr);
 
/*
 * This function enables/disables the cursor.
 */
void ddk770_enableCursor(
    disp_control_t dispControl,     /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long enable
);

void ddk770_HDMI_Enable_Output(hdmi_index index);
void hw770_HDMI_Disable_Output(hdmi_index index);
void ddk770_HDMI_Set_Channel(hdmi_index index, disp_control_t dc);

void hw770_HDMI_Clear_Channel(hdmi_index index);

void hw770_DP_Clear_Channel(dp_index index);

int hw770_hdmi_vga_mode(hdmi_index index);


void DP_Set_Channel(dp_index index, disp_control_t dc);
	

void ddk770_setDisplayDPMS(
   disp_control_t dispControl, /* Channel 0 or Channel 1) */
   DISP_DPMS_t state /* DPMS state */
   );

void hw770_init_hdmi(void);
int hw770_set_hdmi_mode(logicalMode_t *pLogicalMode, struct drm_display_mode mode, bool isHDMI, int hdmi_index);


int hw770_check_iis_interrupt(void);

int hw770_check_vsync_interrupt(int path);
void hw770_clear_vsync_interrupt(int path);

long hw770_setMode(logicalMode_t *pLogicalMode, struct drm_display_mode mode);


#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
int hw770_en_dis_interrupt(int status, int pipe);
#else
int hw770_en_dis_interrupt(int status);
#endif

long ddk770_HDMI_HPD_Detect(hdmi_index index);

int DP_HPD_Detect(dp_index dp_index);

void ddk770_disable_IntMask(void);

void hw770_suspend(struct smi_770_register * pSave);
void hw770_resume(struct smi_770_register * pSave);

void hw770_setgamma(disp_control_t dispCtrl, unsigned long enable);
void hw770_load_lut(disp_control_t dispCtrl, int size, u8 lut_r[], u8 lut_g[], u8 lut_b[]);

long hw770_AdaptI2CCleanBus(
    struct drm_connector *connector
);
long hw770_AdaptI2CInit(struct smi_connector *smi_connector);

void hw770_AudioInit(unsigned long wordLength, unsigned long sampleRate);
void hw770_AudioStart(void);
void hw770_AudioStop(void);
void hw770_AudioDeinit(void);

void hw770_init_dp(void);
void ddk770_iis_Init(void);
int hw770_dp_detect(dp_index dp_index);
void ddk770_iisClearRawInt(void);
unsigned long ddk770_iisDmaPointer(void);

void ddk770_sb_IRQUnmask(int irq_num);

unsigned char ddk770_WM8978_Init(void);
void ddk770_WM8978_DeInit(void);
void ddk770_WM8978_HPvol_Set(unsigned char voll, unsigned char volr);
void ddk770_WM8978_SPKvol_Set(unsigned char volx);

unsigned short alignLineOffset(unsigned short lineOffset);

int hw770_hdmi_detect(hdmi_index hdmi_index);
int hw770_check_pnp_interrupt(hdmi_index index);
void hw770_hdmi_interrupt_enable(hdmi_index index,int enable);

void  hw770_get_current_fb_info(disp_control_t index, struct smi_770_fb_info *fb_info);
void hw770_set_current_pitch(disp_control_t index, struct smi_770_fb_info *fb_info);
void hw770_i2c_reset_busclear(hdmi_index index);
int hw770_dp_check_sink_status(dp_index index);

int hw770_get_current_mode_width(disp_control_t index);

void SetCursorPrefetch(disp_control_t dispControl, unsigned int enable);
#endif
