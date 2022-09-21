//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************

#ifndef  _GF_DISP_H
#define  _GF_DISP_H

#include "gf_kms.h"
#include "gf_driver.h"

/* same to  CBIOS_REGISTER_TYPE */
enum
{
    /* CR_SET = 0,     //CR registers to be set, used in s3postreg */
    /* SR_SET,         //SR registers to be set */
    /* GR_SET,         //GR registers to be set */
    CR=0,                /*CR registers           */
    SR,                /*SR registers           */
    AR,
    GR,
    MISC,
    CR_X,
    SR_X,
    CR_B,               /* B-set of CR */
    CR_C,              /* C-set of CR */
    CR_D,               /* D-set of CR */
    CR_T,               /* CR on IGA3  */
    SR_T,               /* SR on IGA3  */
    SR_B,
    CR_D_0,
    CR_D_1,
    CR_D_2,
    CR_D_3,
    RESERVED=0xFF
};

enum
{
    DP_HPD_NONE = 0,
    DP_HPD_IN,
    DP_HPD_HDMI_OUT,
    DP_HPD_DP_OUT,
    DP_HPD_IRQ,
};

#define GET_LAST_BIT(N) ((~((N)-1))&(N))

//interrupt bit used by SW, it's irrelevant to HW register define, DO NOT set them to register directly
#define INT_VSYNC1  (1 << 0)
#define INT_VSYNC2  (1 << 1)
#define INT_VSYNC3  (1 << 2)
#define INT_VSYNC4  (1 << 3)
#define INT_VSYNCS  (INT_VSYNC1 | INT_VSYNC2 | INT_VSYNC3 | INT_VSYNC4)

#define INT_DP1     (1 << 4)
#define INT_DP2     (1 << 5)
#define INT_DP3     (1 << 6)
#define INT_DP4     (1 << 7)
#define INT_HOTPLUG  (INT_DP1 | INT_DP2 | INT_DP3 | INT_DP4)

#define INT_VIP1    (1 << 8)
#define INT_VIP2    (1 << 9)
#define INT_VIP3    (1 << 10)
#define INT_VIP4    (1 << 11)
#define INT_VIPS  (INT_VIP1 | INT_VIP2 | INT_VIP3 | INT_VIP4)

#define INT_HDCODEC  (1 << 12)

#define INT_FENCE           (1 << 13)
#define INT_FE_HANG_VD0     (1 << 14)
#define INT_BE_HANG_VD0     (1 << 15)
#define INT_FE_ERROR_VD0    (1 << 16)
#define INT_BE_ERROR_VD0    (1 << 17)
#define INT_FE_HANG_VD1     (1 << 18)
#define INT_BE_HANG_VD1     (1 << 19)
#define INT_FE_ERROR_VD1    (1 << 20)
#define INT_BE_ERROR_VD1    (1 << 21)
#define INT_VIDEO_EVENTS      (INT_FE_HANG_VD0 | INT_BE_HANG_VD0 | INT_FE_ERROR_VD0 | INT_BE_ERROR_VD0 | INT_FE_HANG_VD1 | INT_BE_HANG_VD1 | INT_FE_ERROR_VD1 | INT_BE_ERROR_VD1)    

#define INT_HDCP  (1 << 22)

//default interrupt to be enabled at irq install, vsync and hotplug intr is controlled by independent module
#define DEF_INTR  (INT_VIDEO_EVENTS | INT_FENCE | INT_HDCODEC | INT_VIPS | INT_HDCP)

#define AR_INIT_REG     0x83DA
#define AR_INDEX        0x83C0
#define AR_DATA         0x83C1
#define CR_INDEX        0x83D4
#define CR_DATA         0x83D5
#define SR_INDEX        0x83C4
#define SR_DATA         0x83C5

//mmioOffset of registers
#define MMIO_OFFSET_SR_GROUP_A_EXC  0x8600
#define MMIO_OFFSET_SR_GROUP_B_EXC  0x8700
#define MMIO_OFFSET_CR_GROUP_A_EXC  0x8800
#define MMIO_OFFSET_CR_GROUP_B_EXC  0x8900
#define MMIO_OFFSET_CR_GROUP_C_EXC  0x8A00
#define MMIO_OFFSET_CR_GROUP_D_EXC  0x8B00  // for 4-channel MIU CR_D registers
#define MMIO_OFFSET_CR_GROUP_D0_EXC 0x8C00  // MIU channel 0 CR_D registers
#define MMIO_OFFSET_CR_GROUP_D1_EXC 0x8D00  // MIU channel 1 CR_D registers
#define MMIO_OFFSET_CR_GROUP_D2_EXC 0x8E00  // MIU channel 2 CR_D registers
#define MMIO_OFFSET_CR_GROUP_D3_EXC 0x8F00  // MIU channel 3 CR_D registers
#define MMIO_OFFSET_SR_GROUP_T_EXC   0x9400
#define MMIO_OFFSET_CR_GROUP_T_EXC   0x9500

// H/V sync Polarity
#define HOR_NEGATIVE         0x40
#define HOR_POSITIVE         0x00
#define VER_NEGATIVE         0x80
#define VER_POSITIVE         0x00


#ifndef  S_OK
#define  S_OK  0
#endif

#ifndef  E_FAIL
#define  E_FAIL  (-1)
#endif

#ifndef  gf_mb
#if defined(__i386__) || defined(__x86_64__)
#define gf_mb()       asm volatile("mfence":::"memory")
#define gf_rmb()      asm volatile("lfence":::"memory")
#define gf_wmb()      asm volatile("sfence":::"memory")
#define gf_flush_wc() gf_wmb()
#else
#define gf_mb()
#define gf_rmb()
#define gf_wmb()
#define gf_flush_wc()
#endif
#endif

#define  GF_DPMS_OFF 4
#define  GF_DPMS_ON  0

#define ENABLE_HDMI4_VGA_ON_IGA4 1

typedef enum
{
    DISP_OUTPUT_NONE = 0x00,
    DISP_OUTPUT_CRT  = 0X01,
    DISP_OUTPUT_DP1  = 0X8000,
    DISP_OUTPUT_DP2  = 0X10000,
    DISP_OUTPUT_DP3  = 0x20000,
    DISP_OUTPUT_DP4  = 0x40000,
}disp_output_type;


#define DISP_OUTPUT_DP_TYPES (DISP_OUTPUT_DP1| DISP_OUTPUT_DP2|DISP_OUTPUT_DP3|DISP_OUTPUT_DP4)


typedef  struct
{
    int device;
    int int_type;
}DP_EVENT, *PDP_EVENT;

#define  MAX_DP_EVENT_NUM 64
#define MAX_I2CBUS          5

typedef struct 
{
    void            *rom_image;
    void            *cbios_ext;
    struct os_spinlock *cbios_inner_spin_lock;
    struct os_mutex *cbios_aux_mutex;
    struct os_mutex *cbios_i2c_mutex[MAX_I2CBUS];
    struct os_mutex *cbios_lock; //protect cbios reentry.
    void*      gf_card;
    adapter_info_t*  adp_info;

    unsigned int     num_crtc;
    unsigned int     num_output;
    unsigned int     num_plane[MAX_CORE_CRTCS];
    unsigned int     scale_support;  //support panel up scale
    unsigned int     up_scale_plane_mask[MAX_CORE_CRTCS]; //stream mask for each crtc
    unsigned int     down_scale_plane_mask[MAX_CORE_CRTCS]; 

    unsigned int     support_output;
    unsigned int     active_output[MAX_CORE_CRTCS];
    
    void*            irq_chip_func;
    unsigned int     intr_en_bits;
    unsigned int     force_output;       //outputs that have no hpd intr and can't be detected, like hdtv, tv
    unsigned int     supp_polling_outputs;  //outputs that have no hpd intr, but still can be detected, such as CRT
    unsigned int     supp_hpd_outputs;
    unsigned int     hpd_outputs;
    unsigned int     compare_edid_outputs;
    unsigned int     hda_intr_outputs;
    unsigned int     hdcp_intr_outputs;
#ifdef ENABLE_HDMI4_VGA_ON_IGA4
        disp_output_type   conflict_high;
        disp_output_type   conflict_low;
#endif
    struct os_spinlock *intr_lock;
    struct os_spinlock *hpd_lock;
    struct os_spinlock *hda_lock;
    struct os_spinlock *hdcp_lock;
    int              irq_enabled;
    atomic_t         atomic_irq_lock;
    struct work_struct hotplug_work;
    struct work_struct dp_irq_work;
    struct work_struct hda_work;
    struct work_struct hdcp_work;
    struct 
    {
        unsigned int head;
        unsigned int tail;
        DP_EVENT  event[MAX_DP_EVENT_NUM];
    };

    void            *modeset_restore_state;

    int              vbios_version;
    int              vbios_revision;
    unsigned char    pmp_version[64];     // PMP info,include version and build time

    struct gf_capture_type  *captures;
}disp_info_t;

static __inline__ unsigned char read_reg_exc(unsigned char *mmio, int type, unsigned char index)
{
    unsigned int  offset = 0;
    unsigned char temp   = 0;

    switch(type)
    {
    case CR:
        offset = MMIO_OFFSET_CR_GROUP_A_EXC + index;
        break;
    case CR_B:
        offset = MMIO_OFFSET_CR_GROUP_B_EXC + index;
        break;
    case CR_C:
        offset = MMIO_OFFSET_CR_GROUP_C_EXC + index;
        break;
    case SR:
        offset = MMIO_OFFSET_SR_GROUP_A_EXC + index;
        break;
    case SR_B:
        offset = MMIO_OFFSET_SR_GROUP_B_EXC + index;
        break;
    case AR:
        gf_read8(mmio + AR_INIT_REG);
        gf_write8(mmio + AR_INDEX, index);
        temp = gf_read8(mmio + AR_DATA);
        return temp;
    default:
        gf_assert(0, GF_FUNC_NAME(__func__));
        break;
    }

    temp = gf_read8(mmio + offset);

    return temp;
}

static __inline__ void write_reg_exc(unsigned char *mmio, int type, unsigned char index, unsigned char value, unsigned char mask)
{
    unsigned int  offset = 0;
    unsigned char temp   = 0;

    switch(type)
    {
    case CR:
        offset = MMIO_OFFSET_CR_GROUP_A_EXC + index;
        break;
    case CR_B:
        offset = MMIO_OFFSET_CR_GROUP_B_EXC + index;
        break;
    case CR_C:
        offset = MMIO_OFFSET_CR_GROUP_C_EXC + index;
        break;
    case SR:
        offset = MMIO_OFFSET_SR_GROUP_A_EXC + index;
        break;
    case SR_B:
        offset = MMIO_OFFSET_SR_GROUP_B_EXC + index;
        break;
    case SR_T:
        offset = MMIO_OFFSET_SR_GROUP_T_EXC + index;
        break;
    case CR_T:
        offset = MMIO_OFFSET_CR_GROUP_T_EXC + index;
        break;
    default:
        gf_assert(0, GF_FUNC_NAME(__func__));
        break;
    }

    temp   = gf_read8(mmio + offset);
    temp   = (temp & mask) | (value & ~mask);

    gf_write8(mmio + offset, temp);
}

void  disp_irq_init(disp_info_t* disp_info);
void  disp_irq_deinit(disp_info_t* disp_info);
void disp_irq_install(disp_info_t* disp_info);
void disp_irq_uninstall(disp_info_t* disp_info);
struct drm_connector* disp_connector_init(disp_info_t* disp_info, disp_output_type output);
struct drm_encoder* disp_encoder_init(disp_info_t* disp_info, disp_output_type output);
int disp_get_pipe_from_crtc(gf_file_t *priv, gf_kms_get_pipe_from_crtc_t *get);
int disp_suspend(struct drm_device *dev);

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
void disp_vblank_save(struct drm_device* dev);
void disp_vblank_restore(struct drm_device* dev);
void  gf_disp_suspend_helper(struct drm_device *dev);
#endif

void disp_pre_resume(struct drm_device *dev);
void disp_post_resume(struct drm_device *dev);
void gf_encoder_disable(struct drm_encoder *encoder);
void gf_encoder_enable(struct drm_encoder *encoder);

int         disp_cbios_get_clock(disp_info_t *disp_info, unsigned int type, unsigned int *output);

enum drm_connector_status
gf_connector_detect_internal(struct drm_connector *connector, bool force, int full_detect);

#endif
