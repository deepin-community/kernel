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
#include "gf_irq.h"
#include "gf_crtc.h"
#include "gf_capture_drv.h"
#include "gf_disp.h"
#include "gf_kms.h"
#include "gf_splice.h"
#include "gf_trace.h"
#include "gf_pm.h"

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 13, 0)
/* get_scanout_position() return flags */
#define DRM_SCANOUTPOS_VALID        (1 << 0)
#define DRM_SCANOUTPOS_IN_VBLANK    (1 << 1)
#define DRM_SCANOUTPOS_ACCURATE     (1 << 2)
#endif

#define DEFAULT_PRINT_COUNT 3
#define VIDEO_ERROR_INFO_NUM  8

#define RESET_TIME_MINUTE_interval  10

static int video_irq_info_count[VIDEO_ERROR_INFO_NUM] = {0};
static ktime_t video_irq_info_time[VIDEO_ERROR_INFO_NUM] = {0};
static int video_irq_mask[VIDEO_ERROR_INFO_NUM] = {INT_FE_HANG_VD0, INT_BE_HANG_VD0, INT_FE_HANG_VD1, INT_BE_HANG_VD1,
    INT_FE_ERROR_VD0, INT_BE_ERROR_VD0, INT_FE_ERROR_VD1, INT_BE_ERROR_VD1};
static char* video_irq_name[VIDEO_ERROR_INFO_NUM] = {"CORE0_FE_HANG", "CORE0_BE_HANG", "CORE1_FE_HANG", "CORE1_BE_HANG",
    "CORE0_FE_ERROR", "CORE0_BE_ERROR", "CORE1_FE_ERROR", "CORE1_BE_ERROR"};
static int video_reg_offset[VIDEO_ERROR_INFO_NUM] = {0x4C81C, 0x4C81C, 0x4A81C, 0x4A81C, 0x4C81C, 0x4C81C, 0x4A81C, 0x4A81C};

static struct drm_crtc* gf_get_crtc_by_pipe(struct drm_device *dev, pipe_t pipe)
{
    struct drm_crtc *crtc = NULL;

    list_for_each_entry(crtc, &(dev->mode_config.crtc_list), head)
    {
        if (drm_get_crtc_index(crtc) == pipe)
        {
            return crtc;
        }
    }

    return NULL;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
u32 gf_get_vblank_counter(struct drm_crtc *crtc)
#else
u32 gf_get_vblank_counter(struct drm_device *dev, pipe_t pipe)
#endif
{
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    struct drm_device *dev = crtc->dev;
    unsigned int pipe = crtc->index;
#endif
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    gf_get_counter_t  gf_counter;
    int  vblank_cnt = 0;

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
    if (is_splice_target_crtc(dev, pipe))
    {
        return gf_splice_get_vblank_counter(dev, pipe);
    }
#endif

    gf_memset(&gf_counter, 0, sizeof(gf_get_counter_t));
    gf_counter.crtc_index = pipe;
    gf_counter.vblk = &vblank_cnt;
    disp_cbios_get_counter(disp_info, &gf_counter);

    return  (u32)vblank_cnt;
}


#if DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 13, 0)
int gf_get_vblank_timestamp(struct drm_device *dev, pipe_t pipe,
             int *max_error, struct timeval *time, unsigned flags)
#else
bool gf_get_vblank_timestamp(struct drm_device *dev,
                             unsigned int pipe,
                             int *max_error,
                         #if DRM_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
                             ktime_t *vblank_time,
                         #else
                             struct timeval *vblank_time,
                         #endif
                             bool in_vblank_irq)
#endif
{

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 13, 0)
    struct drm_crtc *crtc = gf_get_crtc_by_pipe(dev, pipe);
    struct drm_display_mode *mode;

    if (!crtc)
        return -EINVAL;

    if (is_splice_target_crtc(dev, pipe))
    {
        return gf_splice_get_vblank_timestamp(dev, pipe, max_error, time, flags);
    }

    mode = &crtc->hwmode;
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
    if (dev->mode_config.funcs->atomic_commit)
    {
        mode = &crtc->state->adjusted_mode;
    }

    return drm_calc_vbltimestamp_from_scanoutpos(dev,
            pipe, max_error, time, flags, mode);
#else
    return drm_calc_vbltimestamp_from_scanoutpos(dev,
            pipe, max_error, time, flags, crtc, mode);
#endif

#else

    if (is_splice_target_crtc(dev, pipe))
    {
        return gf_splice_get_vblank_timestamp(dev, pipe, max_error, vblank_time, in_vblank_irq);
    }

    return  drm_calc_vbltimestamp_from_scanoutpos(dev, pipe, max_error,
                                                  vblank_time, in_vblank_irq);
#endif
}
#endif

int gf_get_crtc_scanoutpos(struct drm_device *dev, unsigned int pipe,
             unsigned int flags, int *vpos, int *hpos,
             ktime_t *stime, ktime_t *etime,
             const struct drm_display_mode *mode)
{
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    gf_get_counter_t  gf_counter;
    int  in_vblank = 0, ret = 0, status = 0;

    gf_memset(&gf_counter, 0, sizeof(gf_get_counter_t));
    gf_counter.crtc_index = pipe;
    gf_counter.hpos = hpos;
    gf_counter.vpos = vpos;
    gf_counter.in_vblk = &in_vblank;

    if (stime)
    {
        *stime = ktime_get();
    }

    status = disp_cbios_get_counter(disp_info, &gf_counter);

    if (etime)
    {
        *etime = ktime_get();
    }

    if(status == DISP_OK)
    {
        ret = DRM_SCANOUTPOS_VALID | DRM_SCANOUTPOS_ACCURATE;
        if(in_vblank)
        {
            *vpos -= mode->crtc_vblank_end;
            if(*hpos)
            {
                *hpos -= mode->crtc_htotal;
                *vpos += 1;
            }
            ret |= DRM_SCANOUTPOS_IN_VBLANK;
        }
    }

    return  ret;
}

int gf_legacy_get_crtc_scanoutpos(struct drm_device *dev, int pipe, unsigned int flags,
                     int *vpos, int *hpos, ktime_t *stime, ktime_t *etime)
{
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    gf_get_counter_t  gf_counter;
    struct drm_crtc* crtc = NULL;
    struct drm_display_mode* mode = NULL;
    int  in_vblank = 0, ret = 0, status = 0;

    gf_memset(&gf_counter, 0, sizeof(gf_get_counter_t));
    gf_counter.crtc_index = pipe;
    gf_counter.hpos = hpos;
    gf_counter.vpos = vpos;
    gf_counter.in_vblk = &in_vblank;

    list_for_each_entry(crtc, &dev->mode_config.crtc_list, head)
    {
        if(crtc && to_gf_crtc(crtc)->pipe == pipe)
        {
            mode = &crtc->hwmode;
            break;
        }
    }
    if (stime)
    {
        *stime = ktime_get();
    }
    status = disp_cbios_get_counter(disp_info, &gf_counter);

    if (etime)
    {
        *etime = ktime_get();
    }
    if(status == DISP_OK)
    {
        ret = DRM_SCANOUTPOS_VALID | DRM_SCANOUTPOS_ACCURATE;
        if(in_vblank)
        {
            if(mode)
            {
                *vpos -= mode->crtc_vblank_end;
                if(*hpos)
                {
                    *hpos -= mode->crtc_htotal;
                    *vpos += 1;
                }
            }
            ret |= DRM_SCANOUTPOS_IN_VBLANK;
        }
    }

    return  ret;
}

bool gf_get_crtc_scanoutpos_kernel_4_10(struct drm_device *dev, unsigned int pipe,
                bool in_vblank_irq, int *vpos, int *hpos,
                ktime_t *stime, ktime_t *etime,
                const struct drm_display_mode *mode)
{
    return gf_get_crtc_scanoutpos(dev, pipe, 0, vpos, hpos, stime, etime, mode);
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
bool gf_crtc_get_scanout_position(struct drm_crtc *crtc,
                                  bool in_vblank_irq, int *vpos, int *hpos,
                                  ktime_t *stime, ktime_t *etime,
                                  const struct drm_display_mode *mode)
{
    struct drm_device *dev = crtc->dev;
    unsigned int pipe = crtc->index;

    return gf_get_crtc_scanoutpos(dev, pipe, 0, vpos, hpos, stime, etime, mode);
}
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
int gf_enable_vblank(struct drm_crtc *crtc)
#else
int gf_enable_vblank(struct drm_device *dev, pipe_t pipe)
#endif
{
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    struct drm_device *dev = crtc->dev;
    unsigned int pipe = crtc->index;
#endif

    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    irq_chip_funcs_t* chip_func = (irq_chip_funcs_t*)disp_info->irq_chip_func;
    int  intrrpt = 0, intr_en = 0;
    unsigned long flags = 0;

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
    if (is_splice_target_crtc(dev, pipe))
    {
        return gf_splice_enable_vblank(dev, pipe);
    }
#endif

    if(!chip_func || !chip_func->get_intr_enable_mask || !chip_func->set_intr_enable_mask)
    {
        return 0;
    }

    if(pipe == IGA1)
    {
        intrrpt = INT_VSYNC1;
    }
    else if(pipe == IGA2)
    {
        intrrpt = INT_VSYNC2;
    }
    else if(pipe == IGA3)
    {
        intrrpt = INT_VSYNC3;
    }
    else if(pipe == IGA4)
    {
        intrrpt = INT_VSYNC4;
    }

    flags = gf_spin_lock_irqsave(disp_info->intr_lock);

    if(disp_info->irq_enabled)
    {
        intr_en = chip_func->get_intr_enable_mask(disp_info);
        intr_en |= intrrpt;
        chip_func->set_intr_enable_mask(disp_info, intr_en);
    }
    else
    {
        disp_info->intr_en_bits |= intrrpt;
    }

    gf_spin_unlock_irqrestore(disp_info->intr_lock, flags);

    trace_gfx_vblank_onoff(gf_card->index << 16 | pipe, 1);

    return  0;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
void  gf_disable_vblank(struct drm_crtc *crtc)
#else
void  gf_disable_vblank(struct drm_device *dev, pipe_t pipe)
#endif
{
#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    struct drm_device *dev = crtc->dev;
    unsigned int pipe = crtc->index;
#else
    struct drm_crtc *crtc = gf_get_crtc_by_pipe(dev, pipe);
#endif
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    irq_chip_funcs_t* chip_func = (irq_chip_funcs_t*)disp_info->irq_chip_func;
    int  intrrpt = 0, intr_en = 0;
    unsigned long flags = 0;

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
    if (is_splice_target_crtc(dev, pipe))
    {
        return gf_splice_disable_vblank(dev, pipe);
    }
#endif

    if (!(is_crtc_work_in_splice_mode(crtc) &&
          is_splice_target_active_in_drm(dev)))
    {

        if(!chip_func || !chip_func->get_intr_enable_mask || !chip_func->set_intr_enable_mask)
        {
            return;
        }

        if(pipe == IGA1)
        {
            intrrpt = INT_VSYNC1;
        }
        else if(pipe == IGA2)
        {
            intrrpt = INT_VSYNC2;
        }
        else if(pipe == IGA3)
        {
            intrrpt = INT_VSYNC3;
        }
        else if(pipe == IGA4)
        {
            intrrpt = INT_VSYNC4;
        }

        flags = gf_spin_lock_irqsave(disp_info->intr_lock);

        if(disp_info->irq_enabled)
        {
            intr_en = chip_func->get_intr_enable_mask(disp_info);
            intr_en &= ~intrrpt;
            chip_func->set_intr_enable_mask(disp_info, intr_en);
        }
        else
        {
            disp_info->intr_en_bits &= ~intrrpt;
        }

        gf_spin_unlock_irqrestore(disp_info->intr_lock, flags);

    }

    trace_gfx_vblank_onoff(gf_card->index << 16 | pipe, 0);
}


void gf_disp_enable_interrupt(disp_info_t*  disp_info, int irq_inst_uninst)
{
    gf_card_t* gf_card = disp_info->gf_card;
    irq_chip_funcs_t* chip_func = (irq_chip_funcs_t*)disp_info->irq_chip_func;
    int intr_en, fence_int_lost = 0, new_int = 0;
    unsigned long flags = 0;
#ifdef _DEBUG_
    adapter_info_t* adapter = disp_info->adp_info;
    unsigned char mm8aa0 = 0;
#endif

    if(!chip_func || !chip_func->enable_interrupt)
    {
        return;
    }

    if(!irq_inst_uninst && disp_info->irq_enabled)
    {
        return;
    }

#ifdef _DEBUG_
    gf_info("To enable GFX interrupt.\n");
#endif

    flags = gf_spin_lock_irqsave(disp_info->intr_lock);

    if(irq_inst_uninst)
    {
        intr_en = DEF_INTR | disp_info->intr_en_bits;
    }
    else
    {
        intr_en = disp_info->intr_en_bits;  //intr_en_bits is saved by disable_interrupt func
    }

    if((intr_en & INT_FENCE) == 0)
    {
        intr_en |= INT_FENCE;
        fence_int_lost = 1;
    }

    chip_func->enable_interrupt(disp_info, intr_en);

    if(gf_card->pdev->msi_enabled && chip_func->enable_msi)
    {
        chip_func->enable_msi(disp_info);
    }

    new_int = chip_func->get_intr_enable_mask(disp_info);

    if(!irq_inst_uninst)
    {
        disp_info->irq_enabled = 1;
    }

    gf_spin_unlock_irqrestore(disp_info->intr_lock, flags);

#ifdef _DEBUG_
    mm8aa0 = gf_read8(adapter->mmio + 0x8AA0);
    gf_info("After enable interrupt, mm8AA0=0x%x.\n", mm8aa0);
#endif

    if(intr_en == new_int && new_int)
    {
#ifdef _DEBUG_
        gf_info("Enabled int is 0x%x.\n", new_int);
#endif
    }
    else
    {
        gf_error("#### Something error, old int:0x%x, new int:0x%x. ####\n", intr_en, new_int);
    }

    if(fence_int_lost)
    {
        gf_error("#### Fence INT cleared by sw! inter_en %x ####\n", (intr_en & ~INT_FENCE));
#ifdef _DEBUG_
        gf_dump_stack();
#endif
    }
}

void gf_disp_disable_interrupt(disp_info_t*  disp_info, int irq_inst_uninst)
{
    gf_card_t* gf_card = disp_info->gf_card;
    irq_chip_funcs_t* chip_func = (irq_chip_funcs_t*)disp_info->irq_chip_func;
    int intr_en;
    unsigned long flags = 0;

    if(!chip_func || !chip_func->disable_interrupt)
    {
        return;
    }

    if(!irq_inst_uninst && !disp_info->irq_enabled)
    {
        return;
    }

#ifdef _DEBUG_
    gf_info("To disable GFX interrupt.\n");
#endif

    flags = gf_spin_lock_irqsave(disp_info->intr_lock);

    intr_en = chip_func->disable_interrupt(disp_info);

    if(gf_card->pdev->msi_enabled && chip_func->disable_msi)
    {
        chip_func->disable_msi(disp_info);
    }

    if(!irq_inst_uninst)
    {
        disp_info->irq_enabled = 0;
        disp_info->intr_en_bits = intr_en;
        if((intr_en & INT_FENCE) == 0)
        {
            //patch for fence INT lost issue
            disp_info->intr_en_bits |= INT_FENCE;
        }
    }

    gf_spin_unlock_irqrestore(disp_info->intr_lock, flags);

    if(((intr_en & INT_FENCE) == 0) && (!irq_inst_uninst))
    {
        gf_error("#### Found hw reg value exception! inter_en %x ####\n", intr_en);
#ifdef _DEBUG_
        gf_dump_stack();
#endif
    }
}

void  gf_irq_preinstall(struct drm_device *dev)
{
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;

    //disable all interrupt
    gf_disp_disable_interrupt(disp_info, 1);
}

int  gf_irq_postinstall(struct drm_device *dev)
{
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;

    gf_disp_enable_interrupt(disp_info, 1);

    return 0;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)

int gf_irq_install(struct drm_device *drm_dev)
{
    int irq = to_pci_dev(drm_dev->dev)->irq;
    int ret = 0;

    gf_irq_preinstall(drm_dev);

    ret = request_irq(irq, gf_irq_handle, IRQF_SHARED, STR(DRIVER_NAME), drm_dev);

    if (ret < 0)
    {
        gf_error("request irq failed\n");
        return ret;
    }

    gf_irq_postinstall(drm_dev);

    return ret;
}

#endif

static void  gf_vblank_intrr_handle(struct drm_device* dev, unsigned int intrr)
{
    gf_card_t *gf = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf->disp_info;
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;
    gf_splice_target_t *target = NULL;
    gf_splice_source_t *source = NULL;
    struct drm_crtc *splice_source_crtc = NULL, *splice_target_crtc = NULL;
    struct drm_crtc *crtc = NULL;
    gf_crtc_t *gf_crtc = NULL;
    unsigned int crtc_idx = 0;

    if (splice_manager != NULL)
    {
        target = &splice_manager->target;
        splice_target_crtc = target->crtc;
        if (to_gf_crtc(splice_target_crtc)->enabled)
        {
            source = &(splice_manager->sources[target->config.vblank_source]);

            splice_source_crtc = gf_splice_get_crtc_by_source(dev, source);
        }
    }

    list_for_each_entry(crtc, &(dev->mode_config.crtc_list), head)
    {
        gf_crtc = to_gf_crtc(crtc);

        if (intrr & gf_crtc->vsync_int)
        {
            gf_perf_event_t perf_event = {0, };
            gf_get_counter_t get_cnt = {0, };
            unsigned int vblcnt = 0;
            unsigned long long timestamp;

            if (splice_source_crtc == crtc)
            {
                drm_crtc_handle_vblank(splice_target_crtc);
            }

            if (gf_crtc->enabled)
            {
                drm_crtc_handle_vblank(crtc);
            }

            crtc_idx = drm_get_crtc_index(crtc);

            get_cnt.crtc_index = crtc_idx;
            get_cnt.vblk = &vblcnt;
            disp_cbios_get_counter(disp_info, &get_cnt);

            trace_gfx_vblank_intrr(gf->index << 16 | crtc_idx, vblcnt);

            gf_get_nsecs(&timestamp);
            perf_event.header.timestamp_high = timestamp >> 32;
            perf_event.header.timestamp_low = timestamp & 0xffffffff;
            perf_event.header.size = sizeof(gf_perf_event_vsync_t);
            perf_event.header.type = GF_PERF_EVENT_VSYNC;
            perf_event.vsync_event.iga_idx = crtc_idx + 1;
            perf_event.vsync_event.vsync_cnt_low = vblcnt;
            perf_event.vsync_event.vsync_cnt_high = 0;

            gf_core_interface->perf_event_add_isr_event(gf->adapter, &perf_event);
            //gf_core_interface->hwq_process_vsync_event(gf->adapter, timestamp);
        }
    }
}

#define MAX_DP_QUEUE_DEPTH    (MAX_DP_EVENT_NUM -1)
#define DP_QUEUE_DEPTH(head, tail)        ((head <= tail)? (tail-head) : (MAX_DP_EVENT_NUM -head + tail))
#define DP_QUEUE_FULL(head, tail)         (DP_QUEUE_DEPTH(head, tail) >= MAX_DP_QUEUE_DEPTH)
#define DP_QUEUE_EMPTY(head, tail)        (DP_QUEUE_DEPTH(head, tail) == 0)
#define DP_ADVANCE_QUEUE_POS(pos)  {pos = (pos < MAX_DP_QUEUE_DEPTH)? (pos + 1) : 0; }


static void  gf_hpd_handle(struct drm_device* dev, unsigned int hpd)
{
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    struct drm_connector* connector = NULL;
    gf_connector_t* gf_connector = NULL;
    int  hpd_happen = 0, dp_int = 0, queue_irq_work = 0;
    unsigned long flags;

    if(!hpd)
    {
        return;
    }

    flags = gf_spin_lock_irqsave(disp_info->hpd_lock);

    list_for_each_entry(connector, &dev->mode_config.connector_list, head)
    {
        gf_connector = to_gf_connector(connector);

        if((connector->polled == DRM_CONNECTOR_POLL_HPD) &&
            (gf_connector->hpd_int_bit & hpd) && (gf_connector->hpd_enable))
        {
            if(connector->connector_type == DRM_MODE_CONNECTOR_DisplayPort ||
               connector->connector_type == DRM_MODE_CONNECTOR_HDMIA)
            {
                dp_int = disp_cbios_get_dpint_type(disp_info, gf_connector->output_type);
                if(dp_int == DP_HPD_HDMI_OUT || dp_int == DP_HPD_DP_OUT)
                {
                    hpd_happen = 1;
                    disp_info->hpd_outputs |= gf_connector->output_type;
#if GF_RUN_HDCP_CTS
                    disp_cbios_enable_hdcp(disp_info, FALSE, gf_connector->output_type);
                    gf_connector->hdcp_enable = 0;
                    gf_connector->hpd_out = 1;
#endif
                }
                else if(dp_int == DP_HPD_IRQ || dp_int == DP_HPD_IN)
                {
                    if(DP_QUEUE_EMPTY(disp_info->head, disp_info->tail))
                    {
                        queue_irq_work = 1;
                    }

                    if(!DP_QUEUE_FULL(disp_info->head, disp_info->tail))
                    {
                        disp_info->event[disp_info->tail].device = gf_connector->output_type;
                        disp_info->event[disp_info->tail].int_type = dp_int;
                        DP_ADVANCE_QUEUE_POS(disp_info->tail);
                    }
                }
            }
            else
            {
                hpd_happen = 1;
                disp_info->hpd_outputs |= gf_connector->output_type;
            }
        }
    }

    gf_spin_unlock_irqrestore(disp_info->hpd_lock, flags);

    if(queue_irq_work)
    {
        schedule_work(&disp_info->dp_irq_work);
    }

    if(hpd_happen)
    {
        schedule_work(&disp_info->hotplug_work);
    }
}

#define  HDAC_INT_REG1   0x8288
#define  HDAC_INT_REG2   0x33D9C
#define  HDAC_INT_BITS  (1 << 25)

static void  gf_hdaudio_handle(struct drm_device* dev)
{
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t*)gf_card->disp_info;
    adapter_info_t* adapter = disp_info->adp_info;
    struct drm_connector* connector = NULL;
    gf_connector_t* gf_connector = NULL;
    unsigned long flags;
    unsigned int hdac_int_value = 0;
    unsigned int hda_codec_index = 0;

    hdac_int_value = gf_read32(adapter->mmio + HDAC_INT_REG1);
    gf_write32(adapter->mmio + HDAC_INT_REG1, (hdac_int_value & ~HDAC_INT_BITS));
    if(hdac_int_value & HDAC_INT_BITS)
    {
        hda_codec_index |= (1 << 0);
    }

    hdac_int_value = gf_read32(adapter->mmio + HDAC_INT_REG2);
    gf_write32(adapter->mmio + HDAC_INT_REG2, (hdac_int_value & ~HDAC_INT_BITS));
    if(hdac_int_value & HDAC_INT_BITS)
    {
        hda_codec_index |= (1 << 1);
    }

    if(!hda_codec_index)
    {
        return;
    }

    flags = gf_spin_lock_irqsave(disp_info->hda_lock);

    list_for_each_entry(connector, &dev->mode_config.connector_list, head)
    {
        gf_connector = to_gf_connector(connector);

        if(gf_connector->hda_codec_index & hda_codec_index)
        {
            disp_info->hda_intr_outputs |= gf_connector->output_type;
        }
    }

    gf_spin_unlock_irqrestore(disp_info->hda_lock, flags);

    if(disp_info->hda_intr_outputs)
    {
        schedule_work(&disp_info->hda_work);
    }
}


#define  HDCP_INT_REG1   0x82C8
#define  HDCP_INT_REG2   0x33C70
#define  HDCP_INT_REG3   0x34370
#define  HDCP_INT_REG4   0x34A70
#define  HDCP_INT_BITS  (1 << 16)

#define HDCP_ISR_REG1 0x3368C
#define HDCP_ISR_REG2 0x33C88
#define HDCP_ISR_REG3 0x34388
#define HDCP_ISR_REG4 0x34A88

static void  gf_hdcp_handle(struct drm_device* dev)
{
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t*)gf_card->disp_info;
    adapter_info_t* adapter = disp_info->adp_info;
    struct drm_connector* connector = NULL;
    gf_connector_t* gf_connector = NULL;
    unsigned long flags;
    unsigned int hdcp_int_value = 0;
    unsigned int hdcp_index = 0;
    unsigned int hdcp_isr_value = 0;

    hdcp_int_value = gf_read32(adapter->mmio + HDCP_INT_REG1);
    gf_write32(adapter->mmio + HDCP_INT_REG1, (hdcp_int_value & ~HDCP_INT_BITS));
    if(hdcp_int_value & HDCP_INT_BITS)
    {
        hdcp_index |= (1 << 0);
    }

    hdcp_int_value = gf_read32(adapter->mmio + HDCP_INT_REG2);
    gf_write32(adapter->mmio + HDCP_INT_REG2, (hdcp_int_value & ~HDCP_INT_BITS));
    if(hdcp_int_value & HDCP_INT_BITS)
    {
        hdcp_index |= (1 << 1);
    }

    hdcp_int_value = gf_read32(adapter->mmio + HDCP_INT_REG3);
    gf_write32(adapter->mmio + HDCP_INT_REG3, (hdcp_int_value & ~HDCP_INT_BITS));
    if(hdcp_int_value & HDCP_INT_BITS)
    {
        hdcp_index |= (1 << 2);
    }

    hdcp_int_value = gf_read32(adapter->mmio + HDCP_INT_REG4);
    gf_write32(adapter->mmio + HDCP_INT_REG4, (hdcp_int_value & ~HDCP_INT_BITS));
    if(hdcp_int_value & HDCP_INT_BITS)
    {
        hdcp_index |= (1 << 3);
    }

    hdcp_isr_value = gf_read32(adapter->mmio + HDCP_ISR_REG1);
    if(hdcp_isr_value != 0)
    {
        hdcp_index |= (1 << 0);
    }

    hdcp_isr_value = gf_read32(adapter->mmio + HDCP_ISR_REG2);
    if(hdcp_isr_value != 0)
    {
        hdcp_index |= (1 << 1);
    }

    hdcp_isr_value = gf_read32(adapter->mmio + HDCP_ISR_REG3);
    if(hdcp_isr_value != 0)
    {
        hdcp_index |= (1 << 2);
    }

    hdcp_isr_value = gf_read32(adapter->mmio + HDCP_ISR_REG4);
    if(hdcp_isr_value != 0)
    {
        hdcp_index |= (1 << 3);
    }

    if(!hdcp_index)
    {
        return;
    }

    flags = gf_spin_lock_irqsave(disp_info->hdcp_lock);

    list_for_each_entry(connector, &dev->mode_config.connector_list, head)
    {
        gf_connector = to_gf_connector(connector);
        if(gf_connector->hdcp_index & hdcp_index)
        {
            disp_info->hdcp_intr_outputs |= gf_connector->output_type;
        }
    }

    gf_spin_unlock_irqrestore(disp_info->hdcp_lock, flags);

    if(disp_info->hdcp_intr_outputs)
    {
        schedule_work(&disp_info->hdcp_work);
    }
}

irqreturn_t gf_irq_handle(int irq, void *arg)
{
    struct drm_device* dev = arg;
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;
    irq_chip_funcs_t* chip_func = (irq_chip_funcs_t*)disp_info->irq_chip_func;
    unsigned short  command;
    unsigned int  intrr = 0;

    if(!disp_info->irq_enabled)
    {
        return  IRQ_NONE;
    }

    if(atomic_xchg(&disp_info->atomic_irq_lock, 1) == 1)
    {
        return IRQ_NONE;
    }

    if(!chip_func || !chip_func->get_interrupt_mask)
    {
        return IRQ_NONE;
    }

#if 0
    gf_get_command_status16(gf_card->pdev, &command);

    if(!(command & PCI_EN_MEM_SPACE))
    {
        gf_write_command_status16(gf_card->pdev, (command | PCI_EN_MEM_SPACE));
    }
#endif

    intrr = chip_func->get_interrupt_mask(disp_info);

    if(intrr & INT_VSYNCS)
    {
        gf_vblank_intrr_handle(dev, intrr & INT_VSYNCS);

        gf_capture_interrupt_handle(disp_info, intrr & INT_VSYNCS);
    }

    if(intrr & INT_HDCODEC)
    {
        gf_hdaudio_handle(dev);
    }

    if (intrr & INT_HOTPLUG)
    {
        gf_hpd_handle(dev, intrr & INT_HOTPLUG);
    }

    if(intrr & INT_HDCP)
    {
        gf_hdcp_handle(dev);
    }

    if(intrr & INT_VIPS)
    {
        gf_capture_interrupt_handle(disp_info, intrr & INT_VIPS);
    }

    if(intrr & INT_VIDEO_EVENTS)
    {
        gf_video_interrupt_handle(disp_info, intrr & INT_VIDEO_EVENTS, gf_card->video_irq_info_all);
    }

    if (!gf_card->adapter_info.init_render &&
        gf_card->adapter_info.patch_fence_intr_lost)
    {
        intrr |= INT_FENCE;
    }

    if(intrr & INT_FENCE)
    {
        tasklet_schedule(&gf_card->fence_notify);

        gf_card->adapter_info.init_render = 0;
    }

    atomic_set(&disp_info->atomic_irq_lock, 0);

    gf_rpm_mark_last_busy(dev->dev);

    return  IRQ_HANDLED;
}

static void gf_hot_plug_intr_ctrl(disp_info_t* disp_info, unsigned int intr, int enable)
{
    irq_chip_funcs_t* chip_func = (irq_chip_funcs_t*)disp_info->irq_chip_func;
    unsigned long  flags = 0;
    unsigned int  intr_en = 0;

    if (!chip_func || !chip_func->get_intr_enable_mask || !chip_func->set_intr_enable_mask)
    {
        return;
    }

    intr &= INT_HOTPLUG;

    flags = gf_spin_lock_irqsave(disp_info->intr_lock);

    if(disp_info->irq_enabled)
    {
        intr_en = chip_func->get_intr_enable_mask(disp_info);
    }
    else
    {
        intr_en = disp_info->intr_en_bits;
    }

    if(enable)
    {
        intr_en |= intr;
    }
    else
    {
        intr_en &= ~intr;
    }

    if(disp_info->irq_enabled)
    {
        chip_func->set_intr_enable_mask(disp_info, intr_en);
    }
    else
    {
        disp_info->intr_en_bits = intr_en;
    }


    gf_spin_unlock_irqrestore(disp_info->intr_lock, flags);
}

void gf_hot_plug_intr_onoff(disp_info_t* disp_info, int on)
{
    gf_card_t* gf_card = disp_info->gf_card;
    struct drm_device* drm = gf_card->drm_dev;
    struct drm_connector* connector = NULL;
    gf_connector_t* gf_connector = NULL;
    unsigned int  hpd_int_bits = 0;
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    struct drm_connector_list_iter conn_iter;
#endif

    mutex_lock(&drm->mode_config.mutex);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_begin(drm, &conn_iter);
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    drm_for_each_connector_iter(connector, &conn_iter)
#else

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    drm_for_each_connector(connector, drm)
#else
    list_for_each_entry(connector, &drm->mode_config.connector_list, head)
#endif

#endif
    {
        //mark status to enable for all outputs that support hot plug
        gf_connector = to_gf_connector(connector);

        if ((connector->polled == DRM_CONNECTOR_POLL_HPD) && gf_connector->hpd_int_bit)
        {
            gf_connector->hpd_enable = on;
            hpd_int_bits |= gf_connector->hpd_int_bit;
        }
    }
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_end(&conn_iter);
#endif

    mutex_unlock(&drm->mode_config.mutex);

    if (hpd_int_bits)
    {
        gf_hot_plug_intr_ctrl(disp_info, hpd_int_bits, on);
    }
}

void gf_dp_irq_work_func(struct work_struct *work)
{
    disp_info_t  *disp_info = container_of(work, disp_info_t, dp_irq_work);
    gf_connector_t  *gf_connector = NULL;
    unsigned long irq = 0;
    int device = 0, int_type = 0, detect_devices = 0, comp_edid_devs = 0;
    int  empty = 0, need_detect = 0, need_comp_edid = 0;

    while(1)
    {
        irq = gf_spin_lock_irqsave(disp_info->hpd_lock);

        if(DP_QUEUE_EMPTY(disp_info->head, disp_info->tail))
        {
            empty = 1;
        }
        else
        {
            device = disp_info->event[disp_info->head].device;
            int_type = disp_info->event[disp_info->head].int_type;
            DP_ADVANCE_QUEUE_POS(disp_info->head);
        }

        gf_spin_unlock_irqrestore(disp_info->hpd_lock, irq);

        if(empty)
        {
            break;
        }

        gf_connector = gf_get_connector_by_device_id(disp_info, device);
        if(!gf_connector)
        {
            continue;
        }
        need_detect = need_comp_edid = 0;

        gf_mutex_lock(gf_connector->conn_mutex);
        disp_cbios_handle_dp_irq(disp_info, device, int_type, &need_detect, &need_comp_edid);
        gf_mutex_unlock(gf_connector->conn_mutex);

        if(need_detect)
        {
            detect_devices |= device;

            if(need_comp_edid)
            {
                comp_edid_devs |= device;
            }
        }
    }

    if(detect_devices)
    {
        irq = gf_spin_lock_irqsave(disp_info->hpd_lock);

        disp_info->hpd_outputs |= detect_devices;
        disp_info->compare_edid_outputs |= comp_edid_devs;

        gf_spin_unlock_irqrestore(disp_info->hpd_lock, irq);

#if GF_RUN_HDCP_CTS
        //here we add some delay to make sure plug out is report to OS
        gf_msleep(2000);
#endif
        schedule_work(&disp_info->hotplug_work);
    }
}

static void gf_poll_enable_locked(struct drm_device *dev)
{
    int poll = 0;
    struct drm_connector *connector = NULL;
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    struct drm_connector_list_iter conn_iter;
#endif

    WARN_ON(!mutex_is_locked(&dev->mode_config.mutex));

    if (!dev->mode_config.poll_enabled)
    {
        return;
    }

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_begin(dev, &conn_iter);
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    drm_for_each_connector_iter(connector, &conn_iter)
#else

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    drm_for_each_connector(connector, dev)
#else
    list_for_each_entry(connector, &dev->mode_config.connector_list, head)
#endif

#endif
    {
        if (connector->polled & (DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT))
        {
            poll = true;
            break;
        }
    }
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_end(&conn_iter);
#endif

    if (poll)
    {
        schedule_delayed_work(&dev->mode_config.output_poll_work, OUTPUT_POLL_PERIOD);
    }
}

#define INIT_POLLING_TIME 10

void gf_hotplug_work_func(struct work_struct *work)
{
    disp_info_t*  disp_info = container_of(work, disp_info_t, hotplug_work);
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device*  drm = gf_card->drm_dev;
    struct drm_mode_config *mode_config = &drm->mode_config;
    struct  drm_connector* connector = NULL;
    gf_connector_t*  gf_connector = NULL;
    unsigned long irq = 0;
    unsigned int hpd_outputs = 0, changed = 0, comp_edid_outputs = 0, need_poll = 0;
    unsigned int plug_out = 0, plug_in = 0, cur_output = 0;
    enum drm_connector_status old_status;
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    struct drm_connector_list_iter conn_iter;
#endif
#if DRM_VERSION_CODE >=  KERNEL_VERSION(4, 8, 0)
    struct drm_modeset_acquire_ctx ctx;
    int ret = 0;
#endif
    gf_capture_id_t cf_id = GF_CAPTURE_INVALID;
    gf_capture_event_t cf_event = GF_CAPTURE_EVENT_NONE;

    irq = gf_spin_lock_irqsave(disp_info->hpd_lock);

    hpd_outputs = disp_info->hpd_outputs;
    disp_info->hpd_outputs = 0;

    comp_edid_outputs = disp_info->compare_edid_outputs;
    disp_info->compare_edid_outputs = 0;

    gf_spin_unlock_irqrestore(disp_info->hpd_lock, irq);

    if(!hpd_outputs)
    {
        return;
    }

    mutex_lock(&mode_config->mutex);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_begin(drm, &conn_iter);
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    drm_for_each_connector_iter(connector, &conn_iter)
#else

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    drm_for_each_connector(connector, drm)
#else
    list_for_each_entry(connector, &mode_config->connector_list, head)
#endif

#endif
    {
        gf_connector =  to_gf_connector(connector);
        if ((gf_connector->output_type & hpd_outputs) && (gf_connector->monitor_type != UT_OUTPUT_TYPE_PANEL))
        {
            if (is_connector_work_in_splice_mode(connector))
            {
                changed = gf_splice_handle_source_status(connector);
                continue;
            }

            old_status = connector->status;
            gf_connector->compare_edid = (comp_edid_outputs & gf_connector->output_type)? 1 : 0;
            connector->status = gf_connector_detect_internal(connector, 0, 1);

            if(connector_status_connected == old_status && connector_status_disconnected == connector->status)
            {
                plug_out |= gf_connector->output_type;
            }
            else if(connector_status_disconnected == old_status && connector_status_connected == connector->status)
            {
                plug_in |= gf_connector->output_type;
            }

            if(old_status != connector->status)
            {
                changed = 1;
                gf_connector->polling_time = 0;
            }
            else if(gf_connector->compare_edid && gf_connector->edid_changed)
            {
                changed = 1;
                gf_connector->polling_time = 0;
            }
            else if(old_status == connector_status_connected && UT_OUTPUT_TYPE_HDMI == gf_connector->monitor_type)
            {
                //HDMI plug out INT happen, but no change can be detected, use polling
                gf_info("Polling work will detect connector 0x%x for %d times.\n", gf_connector->output_type, INIT_POLLING_TIME);
                gf_connector->polling_time = INIT_POLLING_TIME;
                if(!disp_info->poll_running)
                {
                    need_poll = 1;
                }
            }
            gf_connector->compare_edid = 0;
        }


#if DRM_VERSION_CODE >=  KERNEL_VERSION(4, 8, 0)
        if (plug_in & gf_connector->output_type)
        {
            drm_modeset_acquire_init(&ctx, 0);
            while (1)
            {
                ret = drm_modeset_lock_all_ctx(drm, &ctx);
                if (ret != -EDEADLK)
                    break;

                drm_modeset_backoff(&ctx);
            }

            if (!ret)
            {
                gf_restore_drm_connector_state(drm, connector, &ctx);
            }

            drm_modeset_drop_locks(&ctx);
            drm_modeset_acquire_fini(&ctx);
        }
#endif
    }
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_end(&conn_iter);
#endif

    if(need_poll)
    {
        gf_poll_enable_locked(drm);
    }

    mutex_unlock(&mode_config->mutex);

    if(changed)
    {
        drm_kms_helper_hotplug_event(drm);
        if(plug_out || plug_in)
        {
            gf_info("**** Hot plug detected: plug_out : 0x%x, plug_in : 0x%x.****\n", plug_out, plug_in);
        }

        cur_output = plug_out;
        cf_event = GF_CAPTURE_EVENT_PLUG_OUT;
        if (plug_in)
        {
            cur_output = plug_in;
            cf_event = GF_CAPTURE_EVENT_PLUG_IN;
        }

        if (DISP_OUTPUT_DP1 == cur_output)
        {
            cf_id = GF_CAPTURE_WB1;
        }
        else if (DISP_OUTPUT_DP2 == cur_output)
        {
            cf_id = GF_CAPTURE_WB2;
        }
        else if (DISP_OUTPUT_DP3 == cur_output)
        {
            cf_id = GF_CAPTURE_WB3;
        }
        else if (DISP_OUTPUT_DP4 == cur_output)
        {
            cf_id = GF_CAPTURE_WB4;
        }

        gf_capture_handle_event(disp_info, cf_id, cf_event);
    }
}

void gf_hda_work_func(struct work_struct* work)
{
    disp_info_t*  disp_info = container_of(work, disp_info_t, hda_work);
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device*  drm = gf_card->drm_dev;
    struct drm_mode_config *mode_config = &drm->mode_config;
    struct  drm_connector* connector = NULL;
    gf_connector_t*  gf_connector = NULL;
    unsigned long irq = 0;
    unsigned int hda_outputs = 0;
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    struct drm_connector_list_iter conn_iter;
#endif

    irq = gf_spin_lock_irqsave(disp_info->hda_lock);

    hda_outputs = disp_info->hda_intr_outputs;
    disp_info->hda_intr_outputs = 0;

    gf_spin_unlock_irqrestore(disp_info->hda_lock, irq);

    if(!hda_outputs)
    {
        return;
    }

    mutex_lock(&mode_config->mutex);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_begin(drm, &conn_iter);
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    drm_for_each_connector_iter(connector, &conn_iter)
#else

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    drm_for_each_connector(connector, drm)
#else
    list_for_each_entry(connector, &mode_config->connector_list, head)
#endif

#endif
    {
        gf_connector =  to_gf_connector(connector);
        if(gf_connector->output_type & hda_outputs && gf_connector->support_audio)
        {
            disp_cbios_set_hda_codec(disp_info, gf_connector);
        }
    }
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_end(&conn_iter);
#endif

    mutex_unlock(&mode_config->mutex);
}

void gf_hdcp_work_func(struct work_struct* work)
{
    disp_info_t*  disp_info = container_of(work, disp_info_t, hdcp_work);
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device*  drm = gf_card->drm_dev;
    struct drm_mode_config *mode_config = &drm->mode_config;
    struct  drm_connector* connector = NULL;
    gf_connector_t*  gf_connector = NULL;
    unsigned long irq = 0;
    unsigned int hdcp_outputs = 0;
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    struct drm_connector_list_iter conn_iter;
#endif

    irq = gf_spin_lock_irqsave(disp_info->hdcp_lock);

    hdcp_outputs = disp_info->hdcp_intr_outputs;
    disp_info->hdcp_intr_outputs = 0;

    gf_spin_unlock_irqrestore(disp_info->hdcp_lock, irq);

    if(!hdcp_outputs)
    {
        return;
    }

    mutex_lock(&mode_config->mutex);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_begin(drm, &conn_iter);
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    drm_for_each_connector_iter(connector, &conn_iter)
#else

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    drm_for_each_connector(connector, drm)
#else
    list_for_each_entry(connector, &mode_config->connector_list, head)
#endif

#endif
    {
        gf_connector =  to_gf_connector(connector);
        if(gf_connector->output_type & hdcp_outputs)
        {
            disp_cbios_hdcp_isr(disp_info, gf_connector);
        }
    }
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_end(&conn_iter);
#endif

    mutex_unlock(&mode_config->mutex);
}

void gf_irq_uninstall (struct drm_device *dev)
{
    gf_card_t*  gf_card = dev->dev_private;
    disp_info_t*  disp_info = (disp_info_t *)gf_card->disp_info;

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
    int irq = to_pci_dev(dev->dev)->irq;
    free_irq(irq, dev);
#endif

    //disable all interrupt
    gf_disp_disable_interrupt(disp_info, 1);
}

void gf_poll_enable(disp_info_t* disp_info)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device* drm = gf_card->drm_dev;

    mutex_lock(&drm->mode_config.mutex);
    gf_poll_enable_locked(drm);
    mutex_unlock(&drm->mode_config.mutex);
}

void gf_poll_disable(disp_info_t *disp_info)
{
    gf_card_t*  gf_card = disp_info->gf_card;
    struct drm_device* drm = gf_card->drm_dev;

    if (!drm->mode_config.poll_enabled)
        return;
    cancel_delayed_work_sync(&drm->mode_config.output_poll_work);
}

void gf_output_poll_work_func(struct work_struct *work)
{
    struct delayed_work *delayed_work = to_delayed_work(work);
    struct drm_device *dev = container_of(delayed_work, struct drm_device, mode_config.output_poll_work);
    struct drm_connector *connector = NULL;
    enum drm_connector_status old_status;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_connector_t *gf_connector;
    int repoll = 0, changed = 0;
    unsigned char *p_edid = NULL;
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    struct drm_connector_list_iter conn_iter;
#endif

    if (!mutex_trylock(&dev->mode_config.mutex))
    {
        repoll = true;
        goto out;
    }

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_begin(dev, &conn_iter);
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    drm_for_each_connector_iter(connector, &conn_iter)
#else

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    drm_for_each_connector(connector, dev)
#else
    list_for_each_entry(connector, &dev->mode_config.connector_list, head)
#endif

#endif
    {
        gf_connector = to_gf_connector(connector);

        /* Ignore forced connectors. */
        if ((connector->force) || gf_connector->output_type == DISP_OUTPUT_SPLICE ||
            (!connector->polled || (connector->polled == DRM_CONNECTOR_POLL_HPD && !gf_connector->polling_time)))
        {
            continue;
        }

        old_status = connector->status;
        /* if we are connected and don't want to poll for disconnect skip it */
        if (old_status == connector_status_connected &&
         connector->polled != DRM_CONNECTOR_POLL_HPD &&
        !(connector->polled & DRM_CONNECTOR_POLL_DISCONNECT))
        {
            continue;
        }

        repoll = 1;

        if (is_connector_work_in_splice_mode(connector))
        {
            changed = gf_splice_handle_source_status(connector);
            continue;
        }

        if (connector->funcs->detect)
        {
            p_edid = disp_cbios_read_edid(disp_info, gf_connector->output_type);
            if (!p_edid)
            {
                gf_connector->compare_edid = 1;
            }
            connector->status = gf_connector_detect_internal(connector, 0, 0);
            gf_connector->compare_edid = 0;

            if (p_edid)
            {
                gf_free(p_edid);
            }
        }
        else
        {
            connector->status = connector_status_connected;
        }

        if (old_status != connector->status)
        {
            if (connector->status == connector_status_unknown)
            {
                connector->status = old_status;
                continue;
            }

            changed = 1;
            gf_connector->polling_time = 0;
        }
        else if(gf_connector->polling_time > 0)
        {
            if (gf_connector->edid_changed)
            {
                changed = 1;
                gf_connector->polling_time = 0;
            }
            else
            {
                gf_connector->polling_time--;
            }
        }
    }
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    drm_connector_list_iter_end(&conn_iter);
#endif

    disp_info->poll_running = 0;
    if(repoll)
    {
        disp_info->poll_running = 1;
    }
    mutex_unlock(&dev->mode_config.mutex);

out:
    if (changed)
    {
        drm_kms_helper_hotplug_event(dev);
    }

    if (repoll)
    {
        schedule_delayed_work(delayed_work, OUTPUT_POLL_PERIOD);
    }
}

void gf_video_interrupt_handle(disp_info_t*  disp_info, unsigned int video_int_mask, int output_all)
{
    adapter_info_t*  adapter = disp_info->adp_info;
    unsigned int  video0_info0 = 0, video0_info1 = 0, video0_info2 = 0;
    unsigned int  video1_info0 = 0, video1_info1 = 0, video1_info2 = 0;
    int i = 0;

    for (i = 0; i < VIDEO_ERROR_INFO_NUM; i++)
    {
        if (video_int_mask & video_irq_mask[i])
        {
            ktime_t ctime = ktime_get();
            if (output_all || video_irq_info_count[i] < DEFAULT_PRINT_COUNT)
            {
                if (!output_all)
                {
                    /* Ignore default be error info print. */
                    if (5 == i || 7 == i)
                    {
                        continue;
                    }
                }
                gf_info("*******************%s!*******************\n", video_irq_name[i]);
                if (i < 4)
                {
                    video0_info0 = gf_read32(adapter->mmio + video_reg_offset[i]);
                    gf_info("info0: 0x%x\n", video0_info0);
                }
                else
                {
                    video0_info0 = gf_read32(adapter->mmio + video_reg_offset[i]);
                    video0_info1 = gf_read32(adapter->mmio + video_reg_offset[i] + 4);
                    video0_info2 = gf_read32(adapter->mmio + video_reg_offset[i] + 8);
                    if (5 == i || 7 == i)
                    {
                        if(!((video0_info2 << 4) & 0x32))
                            gf_info("info0: 0x%x info1: 0x%x info2: 0x%x\n", video0_info0, video0_info1, video0_info2);
                    }
                    else
                    {
                        gf_info("info0: 0x%x info1: 0x%x info2: 0x%x\n", video0_info0, video0_info1, video0_info2);
                    }
                }
                video_irq_info_time[i] = ctime;
                video_irq_info_count[i]++;
            }

            if (ktime_ms_delta(ctime, video_irq_info_time[i]) > (RESET_TIME_MINUTE_interval * 60 * 1000L))
                video_irq_info_count[i] = 0;
        }
    }
}

