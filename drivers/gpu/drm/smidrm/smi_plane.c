// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.

#include "smi_drv.h"

#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_atomic_state_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_damage_helper.h>
#include <drm/drm_format_helper.h>
#include <drm/drm_gem_shmem_helper.h>



#include <drm/drm_simple_kms_helper.h>

//#include <linux/dma-buf-map.h>


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
#include <drm/drm_gem_atomic_helper.h> //Need check
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
#include <linux/iosys-map.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
#include <linux/dma-buf-map.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
#include <drm/drm_framebuffer.h>
#endif

#include "smi_dbg.h"

#include "hw750.h"
#include "hw768.h"
#include "hw770.h"


__attribute__((unused)) static void colorcur2monocur(void *data);
__attribute__((unused)) static spinlock_t buffer_lock;

static const uint32_t smi_cursor_plane_formats[] = { DRM_FORMAT_RGB565, DRM_FORMAT_BGR565,
						     DRM_FORMAT_ARGB8888 };

static const uint32_t smi_formats[] = { DRM_FORMAT_RGB565,   DRM_FORMAT_BGR565,
					DRM_FORMAT_RGB888,
					DRM_FORMAT_XRGB8888,
					DRM_FORMAT_RGBA8888,
					DRM_FORMAT_ARGB8888};


static int smi_cursor_atomic_check(struct drm_plane *plane, 
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)  
				struct drm_plane_state *state
#else
				struct drm_atomic_state *atom_state
#endif
)
{
#if KERNEL_VERSION(5, 13, 0) <= LINUX_VERSION_CODE
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atom_state, plane);
#endif
	struct drm_crtc *crtc = state->crtc;
	int src_w, src_h;

	if (!crtc || !state->fb)
		return 0;

	src_w = (state->src_w >> 16);
	src_h = (state->src_h >> 16);
	if (src_w > CURSOR_WIDTH || src_h > CURSOR_HEIGHT) {
		DRM_ERROR("Invalid cursor size (%dx%d)\n", src_w, src_h);
		return -EINVAL;
	}

	dbg_msg("(%dx%d)@(%dx%d)\n", state->crtc_w, state->crtc_h, state->crtc_x, state->crtc_y);
	return 0;
}

static void smi_cursor_add_zero_padding(void __iomem * addr, unsigned int size)
{
	unsigned int i;
	for (i = 0; i < size; i += 4)
	{
		iowrite32(0,(void __iomem *) (addr + i));
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
static void smi_cursor_atomic_update(struct drm_plane *plane, struct drm_atomic_state *state)
#else
static void smi_cursor_atomic_update(struct drm_plane *plane, struct drm_plane_state *plane_old_state)
#endif
{
	
	u8 __iomem *dst;
	int x, y;
	int width;
	disp_control_t disp_ctrl;
	int i, ctrl_index = 0, max_enc = 0;	
	struct smi_device *sdev = plane->dev->dev_private;
	struct smi_plane *smi_plane = to_smi_plane(plane);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
	struct drm_plane_state* plane_state = plane->state;
#else
	struct drm_plane_state *plane_state = drm_atomic_get_new_plane_state(state, plane);
#endif
	struct drm_crtc* crtc = plane_state->crtc;
	struct drm_framebuffer *fb = plane_state->fb;
	u32 dst_off = 0;

#if	LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 61)
	struct dma_buf_map map;
	struct drm_gem_shmem_object *shem;
	int ret;
	shem = to_drm_gem_shmem_obj(fb->obj[0]);	
	ret = drm_gem_shmem_vmap(shem, &map);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
    struct dma_buf_map map;
	int ret;
	ret = drm_gem_shmem_vmap(fb->obj[0],&map);
#else
	const u8 *src = drm_gem_shmem_vmap(fb->obj[0]);
#endif
#else
	struct drm_shadow_plane_state *shadow_plane_state = to_drm_shadow_plane_state(plane_state);
	struct iosys_map src_map = shadow_plane_state->data[0];
	const u8 *src = src_map.vaddr;
#endif

	max_enc = MAX_ENCODER(sdev->specId);
	for(i = 0;i < max_enc; i++)
	{
		if(crtc == sdev->smi_enc_tab[i]->crtc)
		{
			ctrl_index = i;
			break;
		}
	}
	

	disp_ctrl = (ctrl_index == CHANNEL1_CTRL)?CHANNEL1_CTRL:CHANNEL0_CTRL;
	
	if(sdev->specId == SPC_SM768){
		if(ctrl_index >= MAX_CRTC_768)  //calc which path should we use for HDMI.
	{
		disp_ctrl = (disp_control_t)smi_calc_hdmi_ctrl(sdev->m_connector);
	}
	}else if(sdev->specId == SPC_SM770){
		disp_ctrl = (disp_control_t)smi_encoder_crtc_index_changed(ctrl_index);
	}
	/* cursor offset */
	if (sdev->specId == SPC_SM750)
		dst_off = (SM750_MAX_MODE_SIZE * (disp_ctrl + 1)) - (4 * CURSOR_WIDTH * CURSOR_HEIGHT);
	else if (sdev->specId == SPC_SM768)
		dst_off = (SM768_MAX_MODE_SIZE * (disp_ctrl + 1)) - (4 * CURSOR_WIDTH * CURSOR_HEIGHT);
	else if (sdev->specId == SPC_SM770)
		dst_off = (sdev->vram_size - ((disp_ctrl + 1) * (2 << 20)));
	dst = (smi_plane->vaddr_base + dst_off);
	//printk("smi_cursor_atomic_update() disp_ctrl %d, fb->width %d, fb->height %d cpp %d\n", disp_ctrl, fb->width, fb->height, fb->format->cpp[0]);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
	memcpy_toio(dst, src, fb->width * fb->height * fb->format->cpp[0]);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5,15,0)
	memcpy_toio(dst, map.vaddr, fb->width * fb->height * fb->format->cpp[0]);
	drm_gem_shmem_vunmap(shem,&map);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5,11,0)
	memcpy_toio(dst, map.vaddr, fb->width * fb->height * fb->format->cpp[0]);
	drm_gem_shmem_vunmap(fb->obj[0],&map);
#else
	memcpy_toio(dst, src, fb->width * fb->height * fb->format->cpp[0]);
	drm_gem_shmem_vunmap(fb->obj[0],src);
#endif
	if (sdev->specId == SPC_SM750) {
			ddk750_initCursor(disp_ctrl, (u32)dst_off, BPP16_BLACK,
				BPP16_WHITE, BPP16_BLUE);
			colorcur2monocur(dst);
			ddk750_enableCursor(disp_ctrl, 1);
		} else if (sdev->specId == SPC_SM768) {
			ddk768_initCursor(disp_ctrl, (u32)dst_off, BPP32_BLACK, BPP32_WHITE,
					  BPP32_BLUE);
			ddk768_enableCursor(disp_ctrl, 3);
		} else if (sdev->specId == SPC_SM770) {
			smi_cursor_add_zero_padding((void __iomem *)(dst + (4 * CURSOR_WIDTH * CURSOR_HEIGHT)), (4 * CURSOR_WIDTH * 4));
			ddk770_initCursor(disp_ctrl, (u32)dst_off, BPP32_BLACK, BPP32_WHITE,
			 		  BPP32_BLUE);
			ddk770_enableCursor(disp_ctrl, 3);
		}
	

	x = plane_state->crtc_x;
	y = plane_state->crtc_y;


	/* set cursor location */
	if (sdev->specId == SPC_SM750) {
		ddk750_setCursorPosition(disp_ctrl, x < 0 ? -x : x, y < 0 ? -y : y, y < 0 ? 1 : 0,
					 x < 0 ? 1 : 0);
	} else if (sdev->specId == SPC_SM768) {
		ddk768_setCursorPosition(disp_ctrl, x < 0 ? -x : x, y < 0 ? -y : y, y < 0 ? 1 : 0,
					 x < 0 ? 1 : 0);
	} else if (sdev->specId == SPC_SM770) {
		width = hw770_get_current_mode_width(disp_ctrl);
		if((x + CURSOR_WIDTH) >= width)
			ddk770_setCursorPosition(disp_ctrl, x < 0 ? -x : x, y < 0 ? -y : y, y < 0 ? 1 : 0,
					 x < 0 ? 1 : 0, 0);
		else
			ddk770_setCursorPosition(disp_ctrl, x < 0 ? -x : x, y < 0 ? -y : y, y < 0 ? 1 : 0,
					 x < 0 ? 1 : 0, 1);
		//printk("current cursor position dc:%d x:%d  y:%d  width:%d\n",disp_ctrl,x,y,width);
	}

}

static void smi_cursor_atomic_disable(struct drm_plane *plane, 
#if KERNEL_VERSION(5, 13, 0) >  LINUX_VERSION_CODE
				struct drm_plane_state *old_state
#else
				struct drm_atomic_state *atom_state
#endif
)
{
#if KERNEL_VERSION(5, 13, 0) <= LINUX_VERSION_CODE
	struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(atom_state, plane);
#endif
	disp_control_t disp_ctrl;
	struct smi_device *sdev = plane->dev->dev_private;	
	int i, ctrl_index = 0, max_enc = MAX_ENCODER(sdev->specId);

	
	if (!old_state || !old_state->crtc) {
		dbg_msg("drm plane:%d not enabled\n", plane->base.id);
		return;
	}

	for(i = 0;i < max_enc; i++)
	{
		if(old_state->crtc == sdev->smi_enc_tab[i]->crtc)
		{
			ctrl_index = i;
			break;
		}
	}
	disp_ctrl = (ctrl_index == CHANNEL1_CTRL)?CHANNEL1_CTRL:CHANNEL0_CTRL;
	
	if(sdev->specId == SPC_SM768){
		if(ctrl_index >= MAX_CRTC_768)  //calc which path should we use for HDMI.
	{
		disp_ctrl = (disp_control_t)smi_calc_hdmi_ctrl(sdev->m_connector);
	}
	}else if(sdev->specId == SPC_SM770){
		disp_ctrl = (disp_control_t)smi_encoder_crtc_index_changed(ctrl_index);
	}

		
	if (sdev->specId == SPC_SM750) {
		ddk750_enableCursor(disp_ctrl, 0);
	} else if(sdev->specId == SPC_SM768) {
		ddk768_enableCursor(disp_ctrl, 0);
	} else if(sdev->specId == SPC_SM770) {
	    ddk770_enableCursor(disp_ctrl, 0);
	}
}


static const struct drm_plane_helper_funcs smi_cursor_helper_funcs = {
#if LINUX_VERSION_CODE > KERNEL_VERSION(5,18,0)
	DRM_GEM_SHADOW_PLANE_HELPER_FUNCS,
#endif
	.atomic_check = smi_cursor_atomic_check,
	.atomic_update = smi_cursor_atomic_update,
	.atomic_disable = smi_cursor_atomic_disable,
};



#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
static void smi_handle_damage(struct smi_plane *smi_plane, struct iosys_map *src,
			      struct drm_framebuffer *fb,
			      struct drm_rect *clip)
#else
static void smi_handle_damage(struct smi_plane *smi_plane, 
			      struct drm_framebuffer *fb,
			      struct drm_rect *clip)
#endif
{
	void *back_buffer;
	if(use_doublebuffer)
		back_buffer = ((smi_plane->current_buffer == 1) ? smi_plane->vaddr_back : smi_plane->vaddr_front) + smi_plane->align;
	else
		back_buffer = smi_plane->vaddr;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
	struct iosys_map dst;

	iosys_map_set_vaddr_iomem(&dst, back_buffer);
	//printk("smi_handle_damage(): dst.vaddr_iomem: %p, src->vaddr:%p, clip_offset %x\n", dst.vaddr_iomem, src->vaddr, drm_fb_clip_offset(fb->pitches[0], fb->format, clip));
	iosys_map_incr(&dst, drm_fb_clip_offset(fb->pitches[0], fb->format, clip));
	//printk("smi_handle_damage(): clip x strat:%d  y start:%d  x end:%d  y end:%d\n",clip->x1, clip->y1, clip->x2, clip->y2);
	drm_fb_memcpy(&dst, fb->pitches, src, fb, clip);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)	
	void *dst = back_buffer;
	struct iosys_map map;
	drm_gem_shmem_vmap(to_drm_gem_shmem_obj(fb->obj[0]), &map);
	dst += drm_fb_clip_offset(fb->pitches[0], fb->format, clip);
	drm_fb_memcpy_toio(dst, fb->pitches[0], map.vaddr, fb, clip);
	drm_gem_shmem_vunmap(to_drm_gem_shmem_obj(fb->obj[0]), &map);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 61)
	struct dma_buf_map map;
	struct drm_gem_shmem_object* shem;
	int ret;
	shem = to_drm_gem_shmem_obj(fb->obj[0]);
	ret = drm_gem_shmem_vmap(shem,&map);
	drm_fb_memcpy_dstclip(back_buffer, fb->pitches[0],map.vaddr, fb, clip);
	drm_gem_shmem_vunmap(shem, &map);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
    struct dma_buf_map map;
	int ret;
	ret = drm_gem_shmem_vmap(fb->obj[0],&map);
#if	LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
    drm_fb_memcpy_dstclip(back_buffer, vmap, fb, clip);
#else
	drm_fb_memcpy_dstclip(back_buffer, fb->pitches[0],map.vaddr, fb, clip);
#endif
	drm_gem_shmem_vunmap(fb->obj[0], &map);

#else
	void *vmap;

	vmap = drm_gem_shmem_vmap(fb->obj[0]);
	if (!vmap) {
		pr_err("vmap NULL\n");
		return; /* BUG: SHMEM BO should always be vmapped */
	}

	drm_fb_memcpy_dstclip(back_buffer, vmap, fb, clip);
	drm_gem_shmem_vunmap(fb->obj[0], vmap);
	
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
static void smi_primary_plane_atomic_update(struct drm_plane *plane, struct drm_atomic_state *state)
#else
static void smi_primary_plane_atomic_update(struct drm_plane *plane, struct drm_plane_state *old_plane_state)
#endif
{

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 13, 0)
    struct drm_plane_state *plane_state = drm_atomic_get_new_plane_state(state, plane);
	struct drm_framebuffer *fb = plane_state->fb;
    struct drm_plane_state *old_plane_state = drm_atomic_get_old_plane_state(state, plane);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
    struct drm_shadow_plane_state *shadow_plane_state = to_drm_shadow_plane_state(plane_state);
#endif
#else
    struct drm_plane_state *plane_state = plane->state;
	struct drm_framebuffer *fb = plane_state->fb;
#endif
	struct drm_atomic_helper_damage_iter iter;
	struct smi_plane *smi_plane = to_smi_plane(plane);
	struct drm_rect damage;
	int dst_off, offset, x, y;
	unsigned int buffer_size =0;
	int i, ctrl_index = 0, max_enc = 0;
	disp_control_t disp_ctrl;
	struct smi_device *sdev = plane->dev->dev_private;	

	if (!plane_state->crtc || !plane_state->fb)
		return;

	if (!plane_state->visible)
		return;

	max_enc = MAX_ENCODER(sdev->specId);

	for(i = 0;i < max_enc; i++)
	{
		if(plane_state->crtc == sdev->smi_enc_tab[i]->crtc)
		{
			ctrl_index = i;
			break;
		}
	}
	disp_ctrl = (ctrl_index == CHANNEL1_CTRL)?CHANNEL1_CTRL:CHANNEL0_CTRL;

	if(sdev->specId == SPC_SM768){
		if(ctrl_index >= MAX_CRTC_768)  //calc which path should we use for HDMI.
		{
		disp_ctrl = (disp_control_t)smi_calc_hdmi_ctrl(sdev->m_connector);
		}
	}else if(sdev->specId == SPC_SM770){
		disp_ctrl = (disp_control_t)smi_encoder_crtc_index_changed(ctrl_index);
	}

	x = (plane_state->src_x >> 16);
	y = (plane_state->src_y >> 16);
	//printk("before smi_handle_damage dc%d x:%d  y:%d\n",disp_ctrl,x,y);
	/* primary plane offset */
	if(disp_ctrl == 0) 
		dst_off = 0;  /* with shmem, the primary plane is always at offset 0 */
	if(disp_ctrl == 1) {
		if (sdev->specId == SPC_SM768) 
				dst_off = SM768_MAX_MODE_SIZE; //the second DC is at offset 32MB
		else if (sdev->specId == SPC_SM750) 
				dst_off = SM750_MAX_MODE_SIZE; //the second DC is at offset 8MB	
		else if (sdev->specId == SPC_SM770) 
				dst_off = sm770_max_mode_size; 
	}if(disp_ctrl == 2)
	{
		if (sdev->specId == SPC_SM770) 
		dst_off = sm770_max_mode_size<<1;         //the third DC is at offset 64MB
	}

	if (sdev->specId == SPC_SM770 && (x % 0x100))
		smi_plane->align = alignLineOffset(x * fb->format->cpp[0]) - x * fb->format->cpp[0];
	else 
		smi_plane->align = 0;
	
	if (use_doublebuffer)
	{

		if (sdev->specId == SPC_SM768)
			buffer_size = SM768_MAX_MODE_SIZE / 2; // the second DC is at offset 32MB
		else if (sdev->specId == SPC_SM750)
			buffer_size = SM750_MAX_MODE_SIZE / 2; // the second DC is at offset 8MB
		else if (sdev->specId == SPC_SM770)
			buffer_size = sm770_max_mode_size / 2;

	}
	else
		smi_plane->vaddr = (smi_plane->vaddr_base + dst_off + smi_plane->align);
	//printk("smi_primary_plane_atomic_update(): disp_ctrl %d,  vram_size %x, dst_off %x  pitch %d  smi_plane->vaddr_base:%p\n", disp_ctrl,  smi_plane->vram_size, dst_off,fb->pitches[0],smi_plane->vaddr_base);
	
	drm_atomic_helper_damage_iter_init(&iter, old_plane_state, plane_state);
	drm_atomic_for_each_plane_damage(&iter, &damage) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
		smi_handle_damage(smi_plane, shadow_plane_state->data, fb, &damage);
#else
		smi_handle_damage(smi_plane, fb, &damage);		
#endif
	}


	fb->pitches[0] = (fb->pitches[0] + 15) & ~15;
	
	x = (plane_state->src_x >> 16);
	y = (plane_state->src_y >> 16);

	if (use_doublebuffer)
		offset = dst_off +smi_plane->current_buffer * buffer_size+ y * fb->pitches[0] + x * fb->format->cpp[0] + smi_plane->align;
	else
		offset = dst_off + y * fb->pitches[0] + x * fb->format->cpp[0] + smi_plane->align;

	//printk("DC%d set_base: offset %x, distoffset %x, pitch %d, x %d, y %d  align %d\n", disp_ctrl,offset,dst_off, fb->pitches[0], x, y,smi_plane->align);
	if (sdev->specId == SPC_SM750) {
		hw750_set_base(disp_ctrl, fb->pitches[0], offset);
	} else if (sdev->specId == SPC_SM768) {
		hw768_set_base(disp_ctrl, fb->pitches[0], offset);
	} else if (sdev->specId == SPC_SM770) {
		hw770_set_base(disp_ctrl, fb->pitches[0], offset);
	}

	if (use_doublebuffer)
	{
	    // Swap buffers with synchronization
		spin_lock(&buffer_lock);
		smi_plane->current_buffer = 1 - smi_plane->current_buffer;
		spin_unlock(&buffer_lock);
	}
	return;
}

static int smi_primary_plane_atomic_check(struct drm_plane *plane, 
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	struct drm_plane_state *state
#else
	struct drm_atomic_state *atom_state
#endif
)
{
	//printk("smi_primary_plane_atomic_check plane addr:%p\n",plane);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atom_state, plane);
#endif
	struct drm_crtc *crtc = state->crtc;
	struct drm_crtc_state *crtc_state;

	ENTER();

	if (!crtc)
		LEAVE(0);
	//printk("smi_primary_plane_atomic_check plane addr:%p  crtc indrx %d\n",plane,crtc->index);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	crtc_state = drm_atomic_get_crtc_state(state->state, crtc);
#else
	crtc_state = drm_atomic_get_crtc_state(atom_state, crtc);
#endif	
	if (IS_ERR(crtc_state))
		LEAVE(PTR_ERR(crtc_state));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	LEAVE(drm_atomic_helper_check_plane_state(state, crtc_state, DRM_PLANE_NO_SCALING,
						  DRM_PLANE_NO_SCALING, false, true));
#else
	LEAVE(drm_atomic_helper_check_plane_state(state, crtc_state, DRM_PLANE_HELPER_NO_SCALING,
						  DRM_PLANE_HELPER_NO_SCALING, false, true));
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
static void smi_primary_plane_helper_atomic_disable(struct drm_plane *plane, struct drm_atomic_state *state)
#else
static void smi_primary_plane_helper_atomic_disable(struct drm_plane *plane, struct drm_plane_state *old_plane_state)
#endif
{
	//Add disable plane.
	dbg_msg("smi_primary_plane_helper_atomic_disable():\n");
}
static const struct drm_plane_helper_funcs smi_primary_plane_helper_funcs = {
#if LINUX_VERSION_CODE > KERNEL_VERSION(5,18,0)
	DRM_GEM_SHADOW_PLANE_HELPER_FUNCS,
#endif
	.atomic_check = smi_primary_plane_atomic_check,
	.atomic_update = smi_primary_plane_atomic_update,
	.atomic_disable = smi_primary_plane_helper_atomic_disable,
};

static const struct drm_plane_funcs smi_plane_funcs = {
	.update_plane	= drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_plane_destroy_state,
#if LINUX_VERSION_CODE > KERNEL_VERSION(5,18,0)
 	DRM_GEM_SHADOW_PLANE_FUNCS,
#endif
};

struct drm_plane *smi_plane_init(struct smi_device *cdev, unsigned int possible_crtcs,
				 enum drm_plane_type type)
{
	int err;
	int num_formats;
	const uint32_t *formats;
	struct drm_plane *plane;
	struct smi_plane *smi_plane;
	const struct drm_plane_funcs *funcs;
	const struct drm_plane_helper_funcs *helper_funcs;
	unsigned int buffer_offset = 0;
	unsigned int buffer_size = 0;

	switch (type) {
	case DRM_PLANE_TYPE_PRIMARY:
		funcs = &smi_plane_funcs;
		formats = smi_formats;
		num_formats = ARRAY_SIZE(smi_formats);
		helper_funcs = &smi_primary_plane_helper_funcs;
		break;
	case DRM_PLANE_TYPE_CURSOR:
		funcs = &smi_plane_funcs;
		formats = smi_cursor_plane_formats;
		num_formats = ARRAY_SIZE(smi_cursor_plane_formats);
		helper_funcs = &smi_cursor_helper_funcs;
		break;
	default:
		return ERR_PTR(-EINVAL);
	}

	smi_plane = kzalloc(sizeof(*smi_plane), GFP_KERNEL);
	if (!smi_plane)
		return ERR_PTR(-ENOMEM);

	if (use_doublebuffer)
	{
		if (cdev->specId == SPC_SM750){
			buffer_offset = (possible_crtcs / 2) * SM750_MAX_MODE_SIZE;
			buffer_size = SM750_MAX_MODE_SIZE / 2;
		}else if (cdev->specId == SPC_SM768){
			buffer_offset = (possible_crtcs / 2) * SM768_MAX_MODE_SIZE;
			buffer_size = SM768_MAX_MODE_SIZE / 2;
		}else if (cdev->specId == SPC_SM770){
			buffer_offset = (possible_crtcs / 2) * sm770_max_mode_size;
			buffer_size = sm770_max_mode_size / 2;
		}
		smi_plane->vaddr_front = cdev->vram + buffer_offset;
		smi_plane->vaddr_back = smi_plane->vaddr_front + buffer_size;
		smi_plane->current_buffer = 0;
	}

	smi_plane->vaddr_base = cdev->vram;
	smi_plane->vram_size = cdev->vram_size;
	plane = &smi_plane->base;
	err = drm_universal_plane_init(cdev->dev, plane, possible_crtcs, funcs, formats,
				       num_formats, NULL, type, NULL);

	if (err)
		goto free_plane;

	drm_plane_helper_add(plane, helper_funcs);
	drm_plane_enable_fb_damage_clips(plane);

	return plane;

free_plane:
	kfree(plane);
	return ERR_PTR(-EINVAL);
}

__attribute__((unused)) static void colorcur2monocur(void *data)
{
	unsigned int *col = (unsigned int *)data;
	unsigned char *mono = (unsigned char *)data;
	unsigned char pixel = 0;
	char bit_values;

	int i;
	for (i = 0; i < 64 * 64; i++) {
		if (*col >> 24 < 0xe0) {
			bit_values = 0;
		} else {
			int val = *col & 0xff;

			if (val < 0x80) {
				bit_values = 1;
			} else {
				bit_values = 2;
			}
		}
		col++;
		/* Copy bits into cursor byte */
		switch (i & 3) {
		case 0:
			pixel = bit_values;
			break;

		case 1:
			pixel |= bit_values << 2;
			break;

		case 2:
			pixel |= bit_values << 4;
			break;

		case 3:
			pixel |= bit_values << 6;
			*mono = pixel;
			mono++;
			pixel = 0;
			break;
		}
	}
}
