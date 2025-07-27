// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.


#ifndef __SMI_DRV_H__
#define __SMI_DRV_H__


#include "smi_ver.h"

#include <drm/drm_edid.h>
#include <drm/drm_encoder.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem.h>
#include <video/vga.h>

//#include <drm/display/drm_dp_helper.h>
#include <drm/drm_gem_vram_helper.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drm_vram_mm_helper.h>
#endif


#include <linux/i2c-algo-bit.h>
#include <linux/i2c.h>

#include "smi_priv.h"

#define DRIVER_AUTHOR "SiliconMotion"

#define DRIVER_NAME		"smifb"
#define DRIVER_DESC		"SiliconMotion GPU DRM Driver"
#define DRIVER_DATE		"20250723"

#define DRIVER_MAJOR		4
#define DRIVER_MINOR		4
#define DRIVER_PATCHLEVEL	1

#define SMIFB_CONN_LIMIT 3



#define RELEASE_TYPE "Linux DRM Display Driver Release"
#define SUPPORT_CHIP " SM750, SM768, SM770"


#define _version_	"4.4.1.0"

#undef  NO_WC

#ifdef CONFIG_CPU_LOONGSON3
#define NO_WC
#endif
#ifdef UNUSED
#elif defined(__GNUC__)
#define UNUSED(x) UNUSED_##x __attribute__((unused))
#elif defined(__LCLINT__)
#define UNUSED(x) /*@unused@*/ x
#elif defined(__cplusplus)
#define UNUSED(x)
#else
#define UNUSED(x) x
#endif

#ifndef CONFIG_LOONGARCH
#define ENABLE_HDMI_IRQ 
#else
#undef  ENABLE_HDMI_IRQ
#endif

#define USE_I2C_ADAPTER 1

#define SMI_MAX_FB_HEIGHT 8192
#define SMI_MAX_FB_WIDTH 8192

#define MAX_CRTC_750 2
#define MAX_CRTC_768 2
#define MAX_CRTC_770 3
#define MAX_CRTC(g_specId) (g_specId == SPC_SM750)? MAX_CRTC_750: (g_specId == SPC_SM768)? MAX_CRTC_768:MAX_CRTC_770

#define MAX_ENCODER_750 2
#define MAX_ENCODER_768 3
#define MAX_ENCODER_770 5
#define MAX_ENCODER(g_specId) (g_specId == SPC_SM750)? MAX_ENCODER_750: (g_specId == SPC_SM768)? MAX_ENCODER_768:MAX_ENCODER_770


#define SM768_MAX_MODE_SIZE (80<<20)
#define SM750_MAX_MODE_SIZE (8<<20)
#define smi_DPMS_CLEARED (-1)

extern unsigned int sm770_max_mode_size;
extern int g_specId;
extern int smi_pat;
extern int smi_bpp;
extern int force_connect;
extern int lvds_channel;
extern int audio_en;
extern int fixed_width;
extern int fixed_height;
extern int hwi2c_en;
extern int swcur_en;
extern int edid_mode;
extern int lcd_scale;
extern int pwm_ctrl;
extern int use_vblank;
extern int use_doublebuffer;

struct smi_750_register;
struct smi_768_register;
struct smi_770_register;

struct smi_plane {
	struct drm_plane base;

	int crtc;
	void __iomem *vaddr;
	void __iomem *vaddr_base;
	u32 vram_size;
	unsigned long size;
	void __iomem *vaddr_front;
	void __iomem *vaddr_back;
	unsigned int current_buffer;  // 0 for front, 1 for back
	int align;
};

static inline struct smi_plane *to_smi_plane(struct drm_plane *plane)
{
	return container_of(plane, struct smi_plane, base);
}

struct smi_device {
	struct drm_device *dev;
	struct snd_card 		*card;	
	unsigned long flags;

	resource_size_t rmmio_base;
	resource_size_t rmmio_size;
	resource_size_t vram_size;
	resource_size_t vram_base;
	void __iomem *rmmio;
	void __iomem *vram;

	int specId;
	
	int m_connector;  
	//bit 0: DVI, bit 1: VGA, bit 2: HDMI, bit 3: HDMI1, bit 4: HDMI2, bit 5: DP, bit 6: DP1

	struct drm_encoder *smi_enc_tab[MAX_ENCODER_770];
	struct drm_connector *smi_conn_tab[MAX_ENCODER_770];
	struct smi_mode_info mode_info;

	int num_crtc;
	int fb_mtrr;
	bool need_dma32;
	bool mm_inited;
	void *vram_save;
	union {
		struct smi_750_register *regsave;
		struct smi_768_register *regsave_768;
		struct smi_770_register *regsave_770;
	};
#ifdef USE_HDMICHIP
	struct edid si9022_edid[2];
#endif
	void *dvi_edid;
	void *vga_edid;
	void *hdmi_edid;

#if USE_I2C_ADAPTER
	struct edid *hdmi0_edid;
	struct edid *hdmi1_edid;
	struct edid *hdmi2_edid;
	struct edid *dp0_edid;
	struct edid *dp1_edid;
#else  //increase edid buffer size
	struct edid hdmi0_edid[4];
	struct edid hdmi1_edid[4];
	struct edid hdmi2_edid[4];
	struct edid dp0_edid[4];
	struct edid dp1_edid[4];
#endif

	struct drm_display_mode *fixed_mode;
	bool is_768hdmi;
	bool is_hdmi[SMIFB_CONN_LIMIT];
	bool is_boot_gpu;

};

struct smi_encoder {
	struct drm_encoder base;
	int last_dpms;
};

struct smi_connector {
	struct drm_connector base;
	struct i2c_adapter adapter;
	struct i2c_adapter dp_adapter;
	struct i2c_algo_bit_data bit_data;
	unsigned char i2c_scl;
	unsigned char i2c_sda;
	unsigned char i2cNumber;
	bool i2c_hw_enabled;
	struct mutex i2c_lock;
	bool i2c_is_segment;
	bool i2c_is_regaddr;
	int i2c_slave_reg;
	int i2c_slave_number;
	//struct drm_dp_aux dp_aux;
};

static inline struct smi_connector *to_smi_connector(struct drm_connector *connector)
{
	return container_of(connector, struct smi_connector, base);
}



/* smi_main.c */
int smi_device_init(struct smi_device *cdev, struct drm_device *ddev, struct pci_dev *pdev,
		    uint32_t flags);
void smi_device_fini(struct smi_device *cdev);
int smi_driver_load(struct drm_device *dev, unsigned long flags);
void smi_driver_unload(struct drm_device *dev);

void smi_gem_free_object(struct drm_gem_object *obj);

/* smi_plane.c */
struct drm_plane *smi_plane_init(struct smi_device *cdev, unsigned int possible_crtcs,
				 enum drm_plane_type type);

/* smi_mode.c */
int smi_modeset_init(struct smi_device *cdev);
void smi_modeset_fini(struct smi_device *cdev);
int smi_calc_hdmi_ctrl(int m_connector);
int smi_encoder_crtc_index_changed(int encoder_index);

#define to_smi_crtc(x) container_of(x, struct smi_crtc, base)
#define to_smi_encoder(x) container_of(x, struct smi_encoder, base)


/* smi_mm.c */
int smi_mm_init(struct smi_device *smi);
void smi_mm_fini(struct smi_device *smi);

/* smi_prime.c */
struct sg_table *smi_gem_prime_get_sg_table(struct drm_gem_object *obj);
struct drm_gem_object *smi_gem_prime_import_sg_table(struct drm_device *dev,
						     struct dma_buf_attachment *attach,
						     struct sg_table *sg);

int smi_audio_init(struct drm_device *dev);
void smi_audio_remove(struct drm_device *dev);

void smi_audio_suspend(struct smi_device *sdev);
void smi_audio_resume(struct smi_device *sdev);

#ifndef DRM_IRQ_ARGS
#define DRM_IRQ_ARGS int irq, void *arg
#endif

irqreturn_t smi_drm_interrupt(DRM_IRQ_ARGS);

irqreturn_t smi_hdmi0_pnp_handler(int irq, void *dev_id);
irqreturn_t smi_hdmi0_hardirq(int irq, void *dev_id);
irqreturn_t smi_hdmi1_pnp_handler(int irq, void *dev_id);
irqreturn_t smi_hdmi1_hardirq(int irq, void *dev_id);
irqreturn_t smi_hdmi2_pnp_handler(int irq, void *dev_id);
irqreturn_t smi_hdmi2_hardirq(int irq, void *dev_id);

#define smi_LUT_SIZE 256
#define CURSOR_WIDTH 64
#define CURSOR_HEIGHT 64

#define PALETTE_INDEX 0x8
#define PALETTE_DATA 0x9

#define USE_DVI 1
#define USE_VGA (1 << 1)
#define USE_HDMI (1 << 2)
#define USE_DVI_VGA (USE_DVI | USE_VGA)
#define USE_DVI_HDMI (USE_DVI | USE_HDMI)
#define USE_VGA_HDMI (USE_VGA | USE_HDMI)
#define USE_ALL (USE_DVI | USE_VGA | USE_HDMI)

/*  for 770  */
#define USE_HDMI0 	(1 << 3)
#define USE_HDMI1 	(1 << 4)
#define USE_HDMI2 	(1 << 5)
#define USE_DP0 	(1 << 6)
#define USE_DP1 	(1 << 7)

#define HDMI_INT_HPD 1
#define HDMI_INT_NOT_HPD 2
/* please use revision id to distinguish sm750le and sm750*/
#define SPC_SM750 	0
#define SPC_SM712 	1
#define SPC_SM502   2
#define SPC_SM768   3
#define SPC_SM770   4
//#define SPC_SM750LE 8

#define PCI_VENDOR_ID_SMI 	0x126f
#define PCI_DEVID_LYNX_EXP	0x0750
#define PCI_DEVID_SM768		0x0768
#define PCI_DEVID_SM770		0x0770


#define BPP32_RED    0x00ff0000
#define BPP32_GREEN  0x0000ff00
#define BPP32_BLUE   0x000000ff
#define BPP32_WHITE  0x00ffffff
#define BPP32_GRAY   0x00808080
#define BPP32_YELLOW 0x00ffff00
#define BPP32_CYAN   0x0000ffff
#define BPP32_PINK   0x00ff00ff
#define BPP32_BLACK  0x00000000


#define BPP16_RED    0x0000f800
#define BPP16_GREEN  0x000007e0
#define BPP16_BLUE   0x0000001f
#define BPP16_WHITE  0x0000ffff
#define BPP16_GRAY   0x00008410
#define BPP16_YELLOW 0x0000ffe0
#define BPP16_CYAN   0x000007ff
#define BPP16_PINK   0x0000f81f
#define BPP16_BLACK  0x00000000

#endif				/* __SMI_DRV_H__ */
