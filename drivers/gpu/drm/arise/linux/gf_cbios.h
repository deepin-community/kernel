/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */
#ifndef __GF_CBIOS_H__
#define __GF_CBIOS_H__
#include "CBios.h"

enum
{
    FAMILY_CMODEL,
    FAMILY_CLB,
    FAMILY_DST,
    FAMILY_CSR,
    FAMILY_INV,
    FAMILY_EXC,
    FAMILY_ELT,
    FAMILY_LAST,
};

enum
{
    CHIP_CMODEL,
    CHIP_CLB,
    CHIP_DST,
    CHIP_CSR,
    CHIP_INV,
    CHIP_H5,
    CHIP_H5S1,
    CHIP_H6S2,
    CHIP_CMS,
    CHIP_METRO,
    CHIP_MANHATTAN,
    CHIP_MATRIX,
    CHIP_DST2,
    CHIP_DST3,
    CHIP_DUMA,
    CHIP_H6S1,
    CHIP_DST4,
    CHIP_EXC1,      //Excalibur-1
    CHIP_E2UMA,     //E2UMA
    CHIP_ELT,       //Elite
    CHIP_ELT1K,     //Elite1k
    CHIP_ELT2K,     //Elite2k
    CHIP_ELT2K5,    //Elite2500
    CHIP_ZX2000,    //ZX2000
    CHIP_ELT3K,      //ELITE3K
    CHIP_ARISE=CHIP_ELT3K,      //ARISE
    CHIP_ARISE10C0T,      //ARISE10C0T
    CHIP_ARISE1040,      //ARISE1040
    CHIP_ARISE1020,      //ARISE1020
    CHIP_ARISE1010,      //ARISE1010
    CHIP_ARISE2030,      //ARISE2030
    CHIP_ARISE2020,      //ARISE2020
    CHIP_CHX001,    //CHX001
    CHIP_CHX002,    //CHX002
    CHIP_ZX2100,    //ZX2100
    CHIP_LAST,      //Maximum number of chips supported.
};


typedef struct
{
    int crtc_index;
    int *hpos;
    int *vpos;
    int *vblk;
    int *in_vblk;
}gf_get_counter_t;



#define GF_SELECT_MCLK          0x00
#define GF_SELECT_DCLK1         0x01
#define GF_SELECT_DCLK2         0x02
#define GF_SELECT_TVCLK         0x03
#define GF_SELECT_ECLK          0x04
#define GF_SELECT_ICLK          0x05

#define GF_VBIOS_ROM_SIZE         0x10000
#define GF_SHADOW_VBIOS_SIZE      0x20000
#define GF_PMP_SHADOW_IMG_SIZE    0x200
#define GF_PMP_SHADOW_IMG_OFFSET  0x7CE00    // 52K - 512 byte

#define GF_RUN_HDCP_CTS  0

#define EDID_BUF_SIZE 512

#define DUMP_REGISTER_STREAM   0x1

#define  UPDATE_CRTC_MODE_FLAG   0x1
#define  UPDATE_ENCODER_MODE_FLAG  0x2

int         disp_get_output_num(int  outputs);
int         disp_init_cbios(disp_info_t *disp_info);
int         disp_cbios_init_hw(disp_info_t *disp_info);
int         disp_cbios_cleanup(disp_info_t *disp_info);
void        disp_cbios_get_crtc_resource(disp_info_t *disp_info);
void        disp_cbios_get_crtc_caps(disp_info_t *disp_info);
void        disp_cbios_query_vbeinfo(disp_info_t *disp_info);
int         disp_cbios_get_modes_size(disp_info_t *disp_info, int output);
int         disp_cbios_get_modes(disp_info_t *disp_info, int output, void* buffer, int buf_size);
int         disp_cbios_get_adapter_modes_size(disp_info_t *disp_info);
int         disp_cbios_get_adapter_modes(disp_info_t *disp_info, void* buffer, int buf_size);
int         disp_cbios_merge_modes(CBiosModeInfoExt* merge_mode_list, CBiosModeInfoExt * adapter_mode_list, unsigned int const adapter_mode_num,
    CBiosModeInfoExt const * dev_mode_list, unsigned int const dev_mode_num);
CBiosModeInfoExt* disp_cbios_get_preferred_mode(CBiosModeInfoExt *dev_mode_list, unsigned int mode_num);
CBiosModeInfoExt* disp_cbios_get_maxium_mode(CBiosModeInfoExt *dev_mode_list);
int         disp_cbios_cbmode_to_drmmode(disp_info_t *disp_info, int output, void* cbmode, int i, struct drm_display_mode *drm_mode);
int         disp_cbios_3dmode_to_drmmode(disp_info_t *disp_info, int output, void* mode, int i, struct drm_display_mode *drm_mode);
int         disp_cbios_get_3dmode_size(disp_info_t* disp_info, int output);
int         disp_cbios_get_3dmodes(disp_info_t *disp_info, int output, void* buffer, int buf_size);
void*       disp_cbios_get_device_modelist(disp_info_t *disp_info, int output_type, int* mode_num);
int         disp_cbios_get_mode_timing(disp_info_t *disp_info, int output, struct drm_display_mode *drm_mode);
int         disp_cbios_get_monitor_type(disp_info_t *disp_info, int device, int  connected);
void*       disp_cbios_read_edid(disp_info_t *disp_info, int output);
int         disp_cbios_update_output_active(disp_info_t *disp_info, int* outputs);
int         disp_cbios_set_mode(disp_info_t *disp_info, int crtc, struct drm_display_mode* mode, struct drm_display_mode* adjusted_mode, int  update_flag);
int         disp_cbios_set_hdac_connect_status(disp_info_t *disp_info, int device , int bPresent, int bEldValid);
int         disp_cbios_turn_onoff_screen(disp_info_t *disp_info, int iga, int on);
int         disp_cbios_turn_onoff_iga(disp_info_t *disp_info, int iga, int on);
int         disp_cbios_detect_connected_output(disp_info_t *disp_info, int to_detect, int full_detect);
int         disp_cbios_set_dpms(disp_info_t *disp_info, int device, int dpms_mode);
int         disp_cbios_sync_vbios(disp_info_t *disp_info);
int         disp_cbios_get_active_devices(disp_info_t *disp_info, int* devices);
int         disp_cbios_set_gamma(disp_info_t *disp_info, int pipe, void* data);
void        disp_write_port_uchar(unsigned char *port, unsigned char value);
void        disp_delay_micro_seconds(unsigned int usecs);
int         disp_cbios_dbg_level_get(disp_info_t *disp_info);
void        disp_cbios_dbg_level_set(disp_info_t *disp_info, int dbg_level);
int         disp_cbios_get_connector_attrib(disp_info_t *disp_info, gf_connector_t *gf_connector);
int         disp_cbios_get_crtc_mask(disp_info_t *disp_info,  int device);
int         disp_cbios_get_clock(disp_info_t *disp_info, unsigned int type, unsigned int *clock);
int         disp_cbios_set_clock(disp_info_t *disp_info, unsigned int type, unsigned int para);
int         disp_cbios_enable_hdcp(disp_info_t *disp_info, unsigned int enable, unsigned int devices);
int         disp_cbios_get_hdcp_status(disp_info_t *disp_info, gf_hdcp_op_t *dhcp_op, unsigned int devices);
int         disp_cbios_get_interrupt_info(disp_info_t *disp_info, unsigned int *interrupt_mask);
int         disp_cbios_get_dpint_type(disp_info_t *disp_info,unsigned int device);
int         disp_cbios_handle_dp_irq(disp_info_t *disp_info, unsigned int device, int int_type, int* need_detect, int* need_comp_edid);
void        disp_cbios_dump_registers(disp_info_t *disp_info, int type);
int         disp_cbios_set_hda_codec(disp_info_t *disp_info, gf_connector_t*  gf_connector);
int         disp_cbios_hdcp_isr(disp_info_t *disp_info, gf_connector_t*  gf_connector);
int         disp_cbios_get_hdmi_audio_format(disp_info_t *disp_info, unsigned int device_id, gf_hdmi_audio_formats *audio_formats);
void        disp_cbios_reset_hw_block(disp_info_t *disp_info, gf_hw_block hw_block);
int         disp_cbios_get_counter(disp_info_t* disp_info, gf_get_counter_t* get_counter);
int         disp_cbios_crtc_flip(disp_info_t *disp_info, gf_crtc_flip_t *arg);
int         disp_cbios_update_cursor(disp_info_t *disp_info, gf_cursor_update_t *arg);
int         disp_wait_for_vblank(disp_info_t* disp_info, int pipe, int timeout);
int         disp_cbios_get_slice_num(disp_info_t * disp_info);
#ifdef GF_CAPTURE_VIP_INTERFACE
int         disp_cbios_vip_ctl(disp_info_t *disp_info, gf_vip_set_t *v_set);
#endif

#ifdef GF_CAPTURE_WB_INTERFACE
int         disp_cbios_wb_ctl(disp_info_t *disp_info,  gf_wb_set_t *wb_set);
#endif
int disp_cbios_i2c_ctrl(disp_info_t *disp_info, gf_i2c_param_t *i2c_param);

#endif
