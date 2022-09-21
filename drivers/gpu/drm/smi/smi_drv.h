/*
 * Copyright 2016 SiliconMotion Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License version 2. See the file COPYING in the main
 * directory of this archive for more details.
 *
 */

#ifndef __SMI_DRV_H__
#define __SMI_DRV_H__

#include <linux/version.h>

#include <drm/drm_edid.h>
#include <drm/drm_encoder.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem.h>
#include <video/vga.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
#include <drm/drmP.h>
#include <drm/ttm/ttm_bo_api.h>
#include <drm/ttm/ttm_bo_driver.h>
#include <drm/ttm/ttm_memory.h>
#include <drm/ttm/ttm_module.h>
#include <drm/ttm/ttm_placement.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
#include <drm/drm_gem_vram_helper.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <drm/drm_vram_mm_helper.h>
#endif
#endif

#include "smi_priv.h"

#define DRIVER_AUTHOR "SiliconMotion"

#define DRIVER_NAME		"smifb"
#define DRIVER_DESC		"SiliconMotion GPU DRM Driver"
#define DRIVER_DATE		"20220501"

#define DRIVER_MAJOR		2
#define DRIVER_MINOR		1
#define DRIVER_PATCHLEVEL	1

#define SMIFB_CONN_LIMIT 3



#define RELEASE_TYPE "Linux DRM Display Driver Release"
#define SUPPORT_CHIP " SM750, SM768"


#define _version_	"2.1.0.0"

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

#define SMI_MAX_FB_HEIGHT 8192
#define SMI_MAX_FB_WIDTH 8192

#define smi_DPMS_CLEARED (-1)

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

struct smi_750_register;
struct smi_768_register;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
#define DRM_FILE_PAGE_OFFSET (0x100000000ULL >> PAGE_SHIFT)
#endif


struct smi_device {
	struct drm_device *dev;
	struct snd_card 		*card;	
	unsigned long flags;

	resource_size_t rmmio_base;
	resource_size_t rmmio_size;
	void __iomem *rmmio;

	struct smi_mc mc;
	struct smi_mode_info mode_info;

	int num_crtc;
	int fb_mtrr;
	bool need_dma32;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
	struct {
		struct ttm_bo_device bdev;
	} ttm;
#endif
	bool mm_inited;
	union {
		struct smi_750_register *regsave;
		struct smi_768_register *regsave_768;
	};
	struct edid dvi_edid[2];
	struct edid vga_edid[2];
	struct edid hdmi_edid[2];
	struct drm_display_mode *fixed_mode;
	bool is_hdmi;
	bool is_boot_gpu;
};

struct smi_encoder {
	struct drm_encoder base;
	int last_dpms;
};

struct smi_connector {
	struct drm_connector base;
};

#define MAX_CRTC 2
#define MAX_ENCODER 3

/* smi_main.c */
int smi_device_init(struct smi_device *cdev, struct drm_device *ddev, struct pci_dev *pdev,
		    uint32_t flags);
void smi_device_fini(struct smi_device *cdev);
int smi_driver_load(struct drm_device *dev, unsigned long flags);
void smi_driver_unload(struct drm_device *dev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
int smi_dumb_create(struct drm_file *file, struct drm_device *dev,
		    struct drm_mode_create_dumb *args);
int smi_dumb_mmap_offset(struct drm_file *file, struct drm_device *dev, uint32_t handle,
			 uint64_t *offset);
int smi_dumb_destroy(struct drm_file *file, struct drm_device *dev, uint32_t handle);
void smi_gem_free_object(struct drm_gem_object *obj);
#endif

/* smi_plane.c */
struct drm_plane *smi_plane_init(struct smi_device *cdev, unsigned int possible_crtcs,
				 enum drm_plane_type type);

/* smi_mode.c */
int smi_modeset_init(struct smi_device *cdev);
void smi_modeset_fini(struct smi_device *cdev);
int smi_calc_hdmi_ctrl(int m_connector);

#define to_smi_crtc(x) container_of(x, struct smi_crtc, base)
#define to_smi_encoder(x) container_of(x, struct smi_encoder, base)


/* smi_ttm.c */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
struct smi_bo {
	struct ttm_buffer_object bo;
	struct ttm_placement placement;
	struct ttm_bo_kmap_obj kmap;
	struct drm_gem_object gem;
	struct ttm_place placements[3];
	int pin_count;
};
#define gem_to_smi_bo(gobj) container_of((gobj), struct smi_bo, gem)

static inline struct smi_bo *smi_bo(struct ttm_buffer_object *bo)
{
	return container_of(bo, struct smi_bo, bo);
}
int smi_bo_create(struct drm_device *dev, int size, int align, uint32_t flags, struct sg_table *sg,
		  struct reservation_object *resv, struct smi_bo **psmibo);
void smi_ttm_placement(struct smi_bo *bo, int domain);
void smi_bo_ttm_destroy(struct ttm_buffer_object *tbo);
int smi_mmap(struct file *filp, struct vm_area_struct *vma);
int smi_bo_pin(struct smi_bo *bo, u32 pl_flag, u64 *gpu_addr);
int smi_bo_unpin(struct smi_bo *bo);
void *smi_bo_kmap(struct smi_bo *bo, bool map, bool *is_iomem);
void smi_bo_kunmap(struct smi_bo *bo);
#endif
int smi_mm_init(struct smi_device *smi);
void smi_mm_fini(struct smi_device *smi);

/* smi_prime.c */
struct sg_table *smi_gem_prime_get_sg_table(struct drm_gem_object *obj);
struct drm_gem_object *smi_gem_prime_import_sg_table(struct drm_device *dev,
						     struct dma_buf_attachment *attach,
						     struct sg_table *sg);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
void *smi_gem_prime_vmap(struct drm_gem_object *obj);
void smi_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr);
int smi_gem_prime_pin(struct drm_gem_object *obj);
void smi_gem_prime_unpin(struct drm_gem_object *obj);
#endif


int smi_audio_init(struct drm_device *dev);
void smi_audio_remove(struct drm_device *dev);

void smi_audio_suspend(void);
void smi_audio_resume(void);

#ifndef DRM_IRQ_ARGS
#define DRM_IRQ_ARGS int irq, void *arg
#endif

irqreturn_t smi_drm_interrupt(DRM_IRQ_ARGS);



#define smi_LUT_SIZE 256
#define PALETTE_INDEX 0x8
#define PALETTE_DATA 0x9

#define USE_DVI 1
#define USE_VGA (1 << 1)
#define USE_HDMI (1 << 2)
#define USE_DVI_VGA (USE_DVI | USE_VGA)
#define USE_DVI_HDMI (USE_DVI | USE_HDMI)
#define USE_VGA_HDMI (USE_VGA | USE_HDMI)
#define USE_ALL (USE_DVI | USE_VGA | USE_HDMI)

/* please use revision id to distinguish sm750le and sm750*/
#define SPC_SM750 	0
#define SPC_SM712 	1
#define SPC_SM502   2
#define SPC_SM768   3
//#define SPC_SM750LE 8

#define PCI_VENDOR_ID_SMI 	0x126f
#define PCI_DEVID_LYNX_EXP	0x0750
#define PCI_DEVID_SM768		0x0768


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
