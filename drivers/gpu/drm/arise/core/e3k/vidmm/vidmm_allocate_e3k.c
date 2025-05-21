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
#include "gf_adapter.h"
#include "global.h"
#include "vidmm.h"
#include "vidmmi.h"
#include "mm_e3k.h"
#include "chip_include_e3k.h"
#include "vidmmi_e3k.h"

unsigned int __BitCountTable_e3k[] = BIT_COUNT_TABLE;
const HWFORMAT_TABLE_ENTRY_E3K g_HwFormatTable_e3k[]= HWFORMAT_TABLE;
const HWFORMAT_TABLE_ENTRY_E3K *g_pHwFormatTable_e3k = g_HwFormatTable_e3k;
const KMDTILEINFO_E3K hwKMDTileInfo_e3k = HW_KMD_TILE_INFO_E3K;

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define E3K_VRAM_LOCAL_USAGES       (GF_USAGE_DISPLAY_SURFACE | \
                                     GF_USAGE_OVERLAY_SURFACE | \
                                     GF_USAGE_CURSOR_SURFACE | \
                                     GF_USAGE_SHADOW_SURFACE | \
                                     GF_USAGE_RENDER_TARGET   | \
                                     GF_USAGE_TEXTURE_BUFFER  | \
                                     GF_USAGE_SHADER_BUFFER)

#define E3K_VRAM_NONLOCAL_USAGES   (GF_USAGE_DATA_BUFFER | \
                                     GF_USAGE_TEMP_BUFFER)

/* if such usage set, this allocation must use for display */
#define E3K_DIU_DISPLAY_USAGES      (GF_USAGE_DISPLAY_SURFACE | \
                                     GF_USAGE_OVERLAY_SURFACE | \
                                     GF_USAGE_CURSOR_SURFACE)

static int vidmmi_update_segment_id_e3k(adapter_t *adapter, vidmm_allocation_t *allocation, int segment_id)
{
    int segment_id_new = segment_id;

    //valid secure range segment, now id 4~10 stand for secure range1~7 in rm.
    //kmd only reserved 2 secure range mem.
    if(segment_id >= 4 && segment_id <= 10 && allocation->flag.secured)
    {
        segment_id_new = adapter->hw_caps.secure_range_enable ?
             ((segment_id == 4)?SEGMENT_ID_SECURE_RANGE_E3K: SEGMENT_ID_SECURE_RANGE_E3K_1): SEGMENT_ID_LOCAL_E3K;
    }

//validate here for both (escapse,ioctl) umd should not know the details of memory of different platform
    switch(segment_id)
    {
        case 1:
            //for local, if cpu visible + no-compress + linear, must be in local. but aarm64 has alignment
            //requirement for local memory.(by aarch64, device memory).
            if(allocation->flag.cpu_visible  && allocation->compress_format == 0 && allocation->flag.swizzled == 0)
            {
               segment_id_new = SEGMENT_ID_LOCAL_E3K;
            }
            else
            {
                segment_id_new = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1;
            }

            break;
        case 2:
                segment_id_new = adapter->hw_caps.snoop_only ? 0 : SEGMENT_ID_GART_UNSNOOPABLE_E3K;
                break;
        case 3:
            segment_id_new = adapter->hw_caps.support_snooping ? SEGMENT_ID_GART_SNOOPABLE_E3K: SEGMENT_ID_GART_UNSNOOPABLE_E3K;
            break;

        default:
            //escape mm alloc call does not set any prefer.
            if(allocation->flag.cpu_visible  && allocation->compress_format == 0 && allocation->flag.swizzled == 0)
            {
                segment_id_new = SEGMENT_ID_LOCAL_E3K;
            }
            else
            {
                segment_id_new = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1;
            }

            break;
    }


    if (segment_id_new == SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1)
    {
        vidmm_segment_t *segment = adapter->mm_mgr->segment + SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1;
        if (segment->gpu_vm_size == 0)
        {
            segment_id_new = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K;
        }
    }

    return segment_id_new;
}


#define vaildate_segment_id(id)   do {                                                \
    segment_id = allocation->preferred_segment_raw.segment_id_##id;                   \
    if(segment_id <= SEGMENT_ID_MAX_E3K)                                                               \
    {                                                                                 \
        segment_id = vidmmi_update_segment_id_e3k(adapter, allocation, segment_id); \
        if(segment_id > 0)                                                            \
        {                                                                             \
            segment_bit = 1 << segment_id;                                            \
            if(!(segment_mask & segment_bit))                                         \
            {                                                                         \
                segments[num]   = segment_id;                                         \
                segment_mask   |= segment_bit;                                        \
                num++;                                                                \
            }                                                                         \
        }                                                                             \
    }                                                                                 \
    }while(0);

#define preferred_segment_id(id)         allocation->preferred_segment.segment_id_##id
#define preferred_segment_id_raw(id)     allocation->preferred_segment_raw.segment_id_##id


static void vidmmi_validate_allocation_info_e3k(vidmm_mgr_t *mm_mgr, vidmm_allocation_t *allocation)
{
    adapter_t *adapter = mm_mgr->adapter;

    int segments[MAX_SEGMENT_ID]   = {0};
    int segment_id  = 0;
    int segment_bit = 0;
    int segment_mask= 0;
    int num         = 0;

    //only using prefer 0/1 from upper. update prefer sement id by prefer raw's pool type
    vaildate_segment_id(0);
    segments[0] = segment_id;
    vaildate_segment_id(1);
    segments[1] = segment_id;

    if (!allocation->flag.cpu_visible)
    {
       segments[2] = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K;
    }
    else
    {
        segments[2] = SEGMENT_ID_LOCAL_E3K;
    }

    //cannot be PCIE memory when allocation is compressed or allocation is used for display
    if(allocation->compress_format != 0 || allocation->flag.primary == 1)
    {
        segments[3] = SEGMENT_ID_LOCAL_E3K;
    }
    else
    {
#ifdef __mips64__
        segments[3] = SEGMENT_ID_GART_UNSNOOPABLE_E3K;
#else
        segments[3] = SEGMENT_ID_GART_SNOOPABLE_E3K;
#endif
    }
    segments[4] = SEGMENT_ID_LOCAL_E3K;

    //following code is used as force segemnt id case: as hw limitation, debug and etc.
    //local only must case
    if(allocation->flag.primary || allocation->flag.bVideoInternal||
        adapter->hw_caps.local_only
#ifdef __aarch64__
       || allocation->flag.bHwctxBuffer
#endif
      )
    {
         if(allocation->flag.cpu_visible)
        {
            segments[0] = SEGMENT_ID_LOCAL_E3K;
            segments[1] = 0;
        }
        else//cpu unvisible
        {
            if(allocation->flag.bVideoInternal)//video must <4G
            {
                segments[0] = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K;
                segments[1] = SEGMENT_ID_LOCAL_E3K;
            }
            else//3d prefer >4G first, than <4G
            {
                if (mm_mgr->segment[SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1].gpu_vm_size > 0)
                {
                    segments[0] = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1;
                    segments[1] = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K;
                    segments[2] = SEGMENT_ID_LOCAL_E3K;
                }
                else
                {
                    segments[0] = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K;
                    segments[1] = SEGMENT_ID_LOCAL_E3K;
                }
            }
        }
    }

    preferred_segment_id(0) = segments[0];
    preferred_segment_id(1) = segments[1];
    preferred_segment_id(2) = segments[2];
    preferred_segment_id(3) = segments[3];
    preferred_segment_id(4) = segments[4];

    if (allocation->flag.primary)
    {
        allocation->alignment = max(0x8000, allocation->alignment);
    }
}

static void vidmmi_check_allocation_flag_e3k(adapter_t *adapter, vidmm_allocation_t  *allocation, gf_create_allocation_t *hint, unsigned char compress)
{
    unsigned int     usage_mask = hint->usage_mask;
    gf_access_hint access_hint  = hint->access_hint;

    allocation->flag.swizzled   = hint->tiled;

    if(usage_mask & (E3K_DIU_DISPLAY_USAGES))
    {
       allocation->flag.primary  = 1;
       if(hint->tiled)
       {
           gf_warning("%s(): display surface must be linear. but setting as tiled.\n", util_remove_name_suffix(__func__));
       }
    }

    allocation->flag.cpu_visible = (access_hint != GF_ACCESS_GPU_ONLY);
    allocation->flag.unpagable = (hint->unpagable || !adapter->ctl_flags.paging_enable);

    if(allocation->flag.secured)
    {
        allocation->flag.unpagable = 1;
    }

    if (hint->usage_mask & (GF_USAGE_DISPLAY_SURFACE | GF_USAGE_RENDER_TARGET))
    {
        allocation->flag.force_clear = TRUE;
    }
#ifdef __aarch64__
    if (hint->usage_mask & GF_USAGE_HWCTX_BUFFER)
    {
        allocation->flag.bHwctxBuffer = 1;
    }
#endif
}

static inline unsigned int vidmmi_get_allocation_hw_format_e3k(gf_format format)
{
    Hw_Surf_Format hw_format = 0;

    switch(format)
    {
    case GF_FORMAT_A8_UNORM:
        hw_format = HSF_A8_UNORM;
        break;

    case GF_FORMAT_B5G6R5_UNORM:
        hw_format = HSF_B5G6R5_UNORM;
        break;

    case GF_FORMAT_B8G8R8A8_UNORM:
        hw_format = HSF_B8G8R8A8_UNORM;
        break;

    case GF_FORMAT_B8G8R8X8_UNORM:
        hw_format = HSF_B8G8R8X8_UNORM;
        break;

    case GF_FORMAT_R8G8B8A8_UNORM:
        hw_format = HSF_R8G8B8A8_UNORM;
         break;

    case GF_FORMAT_A8R8G8B8_UNORM:
        hw_format = HSF_A8R8G8B8_UNORM;
        break;

    case GF_FORMAT_R8G8B8X8_UNORM:
        hw_format = HSF_R8G8B8X8_UNORM;
        break;

    case GF_FORMAT_YUY2:
        hw_format = HSF_YUY2;
        break;

    case GF_FORMAT_NV12_LINEAR:
        hw_format = HSF_NV12;
        break;

    case GF_FORMAT_NV12_TILED:
        hw_format = HSF_NV12;
        break;

    case GF_FORMAT_NV21_LINEAR:
        hw_format = HSF_NV12;
        break;

    case GF_FORMAT_YV12:
        hw_format = HSF_YV12;
        break;

    case GF_FORMAT_FLOAT32:
        hw_format = HSF_D32_FLOAT;
        break;

    case GF_FORMAT_UINT32:
        hw_format = HSF_R32_UINT;
        break;

    case GF_FORMAT_INT32:
        hw_format = HSF_R32_SINT;
        break;
    case GF_FORMAT_R8G8_UNORM:
        hw_format = HSF_R8G8_UNORM;
        break;
    case GF_FORMAT_R8_UNORM:
        hw_format = HSF_R8_UNORM;
        break;

    case GF_FORMAT_B10G10R10A2_UNORM:
        hw_format = HSF_A2B10G10R10_UNORM;
        break;

   case GF_FORMAT_P010:
        hw_format = HSF_P010;
        break;

   case GF_FORMAT_L8_UNORM:
        hw_format = HSF_L8_UNORM;
        break;

   case GF_FORMAT_L8A8_UNORM:
        hw_format = HSF_L8A8_UNORM;
        break;

   case GF_FORMAT_R8G8B8_UNORM:
        hw_format = HSF_R8G8B8_UNORM;
        break;

   case GF_FORMAT_R4G4B4A4_UNORM:
        hw_format = HSF_A4B4G4R4_UNORM;
        break;

   case GF_FORMAT_R5G5B5A1_UNORM:
        hw_format = HSF_A1B5G5R5_UNORM;
        break;

   case GF_FORMAT_D16_UNORM:
        hw_format = HSF_D16_UNORM;
        break;

   case GF_FORMAT_S8_UINT:
        hw_format = HSF_R8_UINT;
        break;

    default:
        gf_warning("unknow gfx format :%d.\n", format);
        //gf_assert(0, "vidmmi_get_allocation_hw_format unknown gfx format ");
        break;
    }

    return hw_format;
}

static void vidmmi_set_allocation_compress_format_e3k(adapter_t *adapter, vidmm_allocation_t *allocation, gf_create_allocation_t *create_hint)
{
    COMPRESS_MXUFORMAT compress_format = CP_OFF;
    int can_compress = FALSE;

    if(create_hint->compressed)
    {
        switch (allocation->at_type)
        {
            case MM_AT_TEXTURE_E3K:
            case MM_AT_RENDERTARGET_E3K:
                can_compress = TRUE;
                break;

            default:
                break;
        }
    }

    if(can_compress)
    {
        compress_format = g_pHwFormatTable_e3k[allocation->hw_format].CompressFmt;
    }

    switch(compress_format)
    {
    case CP_R8G8B8A8_L:
        if(allocation->flag.swizzled)
        {
            compress_format = CP_R8G8B8A8_T;
        }
        break;

    case CP_A8R8G8B8_L:
        if(allocation->flag.swizzled)
        {
            compress_format = CP_A8R8G8B8_T;
            if (allocation->hw_format == HSF_B8G8R8A8_UNORM)
            {
                compress_format = CP_R8G8B8A8_T;
            }
        }
        break;

    case CP_TOTAL_RANGE:
       compress_format = CP_OFF;
       break;

    default:
        break;
    }

    allocation->compress_format = compress_format;
}


#define E3K_32K_ALIGNMENT_USAGES    (GF_USAGE_DISPLAY_SURFACE | \
                                     GF_USAGE_SHADOW_SURFACE | \
                                     GF_USAGE_OVERLAY_SURFACE | \
                                     GF_USAGE_CURSOR_SURFACE  | \
                                     GF_USAGE_RENDER_TARGET)

#define E3K_4K_ALIGNMENT_USAGES     (GF_USAGE_DATA_BUFFER | \
                                     GF_USAGE_SHADER_BUFFER)

//port from mmGetAllocationAlingment_e3k
static unsigned int vidmmi_get_allocation_alignment_e3k(adapter_t *adapter, vidmm_allocation_t *allocation, unsigned int at_type)
{
    unsigned int alignment = 0;

    switch (at_type)
    {
    case MM_AT_TEXTURE_E3K:
    case MM_AT_RENDERTARGET_E3K:
        if(allocation->compress_format)
        {
            alignment = 0x8000;
        }
        else if(allocation->flag.swizzled)
        {
            alignment= 0x10000;
        }
        else
        {
            alignment = 0x100;
        }
        break;
    case MM_AT_VERTEXBUFFER_E3K:
    case MM_AT_EUPFSHADER_E3K:
        alignment = 0x0100;
        break;
    case MM_AT_GENERIC_E3K:
        alignment = 0x8000;
        break;
    case MM_AT_MIUCOUNTER_E3K:
        alignment = 0x10000;
        break;
    default:
        alignment = 0x8000;
        gf_info("unknown at_type. set default aligned to 32K.\n");
        break;
    }

    return alignment;
}


static inline unsigned int vidmmi_get_allocation_pool_type_e3k(adapter_t *adapter, unsigned int usage_mask, gf_access_hint access_hint, int try_secure)
{
    vidmm_segment_preference_t segment = {0};

    //to same as cil2 mm logic
    //1 means to allocate in local video memory
    //2 means to allocate in non-local non-snoop video memory
    //3 means to allocate in non-local snoop video memory.

    if(try_secure && adapter->hw_caps.secure_range_enable)
    {
        segment.segment_id_0 = SEGMENT_ID_SECURE_RANGE_E3K;
    }
    else if(usage_mask & GF_USAGE_FORCE_PCIE)
    {
        segment.segment_id_0 = SEGMENT_ID_GART_SNOOPABLE_E3K; // snoop for cpu fast access
    }
    //local video memory case
    else if(usage_mask & E3K_VRAM_LOCAL_USAGES)
    {
        segment.segment_id_0 = SEGMENT_ID_LOCAL_E3K;
    }
    //non local video memory case
    else if(usage_mask & E3K_VRAM_NONLOCAL_USAGES)
    {
        if(access_hint == GF_ACCESS_CPU_ALMOST)
        {
            segment.segment_id_0 = SEGMENT_ID_GART_SNOOPABLE_E3K;
        }
        else
        {
            segment.segment_id_0 = adapter->hw_caps.snoop_only ? SEGMENT_ID_GART_SNOOPABLE_E3K : SEGMENT_ID_GART_UNSNOOPABLE_E3K;
        }
    }
    //default: not care memory location case
    else
    {
        //2d not considering cacheable case, only set to non local.
        segment.segment_id_0 = SEGMENT_ID_GART_SNOOPABLE_E3K;
        if (!adapter->hw_caps.snoop_only)
        {
            segment.segment_id_1 = SEGMENT_ID_GART_UNSNOOPABLE_E3K;
        }
    }

    return segment.value;
}

//port from rmiCalcResourcePitch_e3k() and  rmiCalcHwLinearPittch_e3k()
static void vidmmi_calc_allocation_pitch_e3k(vidmm_allocation_t *allocation)
{
    unsigned int  BitCount;
    unsigned int  Width0, Height0;
    unsigned int  TileWidth, TileHeight;
    unsigned int  UnitWidth, UnitHeight;
    unsigned int  P2Width0, P2Height0;
    unsigned int  Pitch;
    unsigned int  WidthAligned;
    unsigned int  HeightAligned;

    BitCount   = allocation->bit_count;
    TileWidth  = calcTileWidth_e3k(BitCount);
    TileHeight = calcTileHeight_e3k(BitCount);
    UnitWidth  = calcUnitWidth_e3k(BitCount);
    UnitHeight = calcUnitHeight_e3k(BitCount);

    Width0     = allocation->width;
    Height0    = allocation->height;

    // overflow check
    if ( Width0            >= (0xFFFFFFFF / Height0) ||
        (Width0 * Height0) >= (0xFFFFFFFF / BitCount))
    {
        gf_error("invlad argument, width or height too big. width:%d, height:%d.\n", Width0, Height0);
        return;
    }

    if (allocation->flag.swizzled)
    {
        P2Width0  = (Width0 & (Width0 - 1)) ?
                util_get_msb_position(Width0) + 1:
                util_get_msb_position(Width0);
        P2Width0  = 1 << P2Width0;

        P2Height0 = (Height0&(Height0-1))?
                    util_get_msb_position(Height0) + 1:
                    util_get_msb_position(Height0);
        P2Height0 = 1 << P2Height0;

        P2Width0  = max(P2Width0, UnitWidth);
        P2Height0 = max(P2Height0, UnitHeight);

        //Folding is disabled for NV12 tiled surface, we need make NV12 surface tiled aligned.
        if (allocation->hw_format == HSF_NV12 || allocation->hw_format == HSF_P010)
        {
            P2Width0  = P2Width0 >= TileWidth ? P2Width0 : TileWidth;
            P2Height0 = P2Height0 >= TileHeight ? P2Height0 : TileHeight;
        }

        if ((P2Width0 >= TileWidth) && (P2Height0 >= TileHeight))
        {
            //pResource->Flags.TileAligned = 1;

            WidthAligned  = calcWidthInTiles_e3k(Width0, BitCount) * TileWidth;
            HeightAligned = calcHeightInTiles_e3k(Height0, BitCount) * TileHeight;
        }
        // P2 Aligned
        else
        {
            //pResource->Flags.TileAligned = 0;
            WidthAligned   = P2Width0;
            HeightAligned  = P2Height0;
        }
        Pitch =((WidthAligned * BitCount +7)/8);
    }
    else
    {
        //port rmiCalcHwLinearPitch_e3k()
        // TODO: how to calc the pitch for linear source
        WidthAligned  = (Width0) ? Width0 : 1;
        Pitch = ALIGNED_2KBits((WidthAligned * BitCount  + 7)/8);

        WidthAligned   = Pitch * 8 / BitCount;
        HeightAligned  = allocation->height;
    }

    //Get Pitch Size
    allocation->tiled_width    = TileWidth;
    allocation->tiled_height   = TileHeight;
    allocation->aligned_width  = WidthAligned;
    allocation->aligned_height = HeightAligned;

    allocation->orig_size      = Pitch * HeightAligned;

    //allocation->orig_size = ALIGNED_2KBits( allocation->orig_size);
    allocation->pitch          = Pitch;
}

static unsigned int vidmmi_get_allocation_type_e3k(unsigned int usage_mask)
{
    unsigned int at_type = MM_AT_GENERIC_E3K;

    if(usage_mask & GF_USAGE_MIU_COUNTER)
    {
        at_type = MM_AT_MIUCOUNTER_E3K;
    }
    else if(usage_mask & (GF_USAGE_DISPLAY_SURFACE |
                     GF_USAGE_OVERLAY_SURFACE |
                     GF_USAGE_SHADOW_SURFACE  |
                     GF_USAGE_CURSOR_SURFACE  |
                     GF_USAGE_RENDER_TARGET))
    {
        at_type = MM_AT_RENDERTARGET_E3K;
    }
    else if(usage_mask & GF_USAGE_TEXTURE_BUFFER)
    {
        at_type = MM_AT_TEXTURE_E3K;
    }
    else if(usage_mask & GF_USAGE_SHADER_BUFFER)
    {
        at_type = MM_AT_EUPFSHADER_E3K;
    }
    else if(usage_mask & GF_USAGE_DATA_BUFFER)
    {
        at_type = MM_AT_VERTEXBUFFER_E3K;
    }
    else if(usage_mask & GF_USAGE_TEMP_BUFFER)
    {
        at_type = MM_AT_GENERIC_E3K;
    }

    return at_type;
}

int vidmm_describe_allocation_e3k(adapter_t *adapter, vidmm_describe_allocation_t *desc_info)
{
    vidmm_allocation_t *allocation = desc_info->allocation;

    int y_tile = 0;
    int status = S_OK;

    if(desc_info->create_info != NULL)
    {
        gf_create_alloc_info_t *desc = desc_info->create_info;

        y_tile = TILE_SIZE_Y(allocation->bit_count);
        allocation->aligned_width  = (allocation->pitch * 8)/allocation->bit_count;
        allocation->tiled_width    = NUM_OF_X_TILES(allocation->aligned_width, allocation->bit_count);//tile num of x
        allocation->aligned_height = (allocation->height + y_tile -1) & ~(y_tile - 1);
        allocation->tiled_height   = NUM_OF_Y_TILES(allocation->height, allocation->bit_count);
        allocation->compress_format= desc->HWCompressFormat;
        allocation->flag.secured   = desc->bSecurity;
        allocation->flag.bVideoInternal = desc->bVideoInternal;

        vidmmi_validate_allocation_info_e3k(adapter->mm_mgr, allocation);

        if(allocation->at_type == MM_AT_RT_TEXTURE_E3K    ||
            allocation->at_type == MM_AT_RENDERTARGET_E3K  ||
            allocation->at_type == MM_AT_STENCILBUFFER_E3K ||
            allocation->at_type == MM_AT_DEPTHBUFFER_E3K ||
            allocation->at_type == MM_AT_OCL_SPECIAL_BUFFER_E3K)
        {
            allocation->flag.force_clear = 1;
        }

        vidmm_trace("create_allocation: allocation:%p, handle: %x, width:%x, height:%x,pitch: %x. y_tile:%x, size:%x\n",
            allocation, allocation->handle, allocation->width, allocation->height, allocation->pitch, y_tile, allocation->orig_size);
    }
    else if(desc_info->create_gf != NULL)
    {
        gf_create_allocation_t *create_hint = desc_info->create_gf->create_data;

        allocation->width           = create_hint->width;
        allocation->height           = create_hint->height;
        allocation->priority         = PMAX;
        allocation->at_type         = vidmmi_get_allocation_type_e3k(create_hint->usage_mask);
        allocation->hw_format    = vidmmi_get_allocation_hw_format_e3k(create_hint->format);
        allocation->alignment     = vidmmi_get_allocation_alignment_e3k(adapter, allocation, allocation->at_type);
        allocation->bit_count      = __BitCountTable_e3k[allocation->hw_format];

        /******************************************************************
        * 1. modify allocation flags according usage, such as cpu visible *
        * 2. check confict about usage and create flags.                  *
        ******************************************************************/
        vidmmi_check_allocation_flag_e3k(adapter, allocation, create_hint, allocation->compress_format);

        //calc pitch according to tile or linear.
        vidmmi_calc_allocation_pitch_e3k(allocation);

        //if compress, set compress format, e3k only setting rgba_compress now
        vidmmi_set_allocation_compress_format_e3k(adapter, allocation, create_hint);//set compress format

        /***********************************************************************
        * get allocation's pool type according to usage,access hint and hw caps*
        * pool type 1 means local video memory                                 *
        * pool type 2 means non-local non-snoop video memory                   *
        * pool type 3 means non-local snoop video memory.                      *
        ***********************************************************************/
        allocation->preferred_segment_raw.value =
            vidmmi_get_allocation_pool_type_e3k(adapter, create_hint->usage_mask, create_hint->access_hint, create_hint->secured);


        /*********************************************************************
        * 1. update segment id accoding to pool type, no usage/hint will     *
             be used, only considering allocation flags and hw caps to update*
        * 2. validate segment id according by hw limitation and etc          *
        *********************************************************************/
        vidmmi_validate_allocation_info_e3k(adapter->mm_mgr, allocation);

        vidmm_trace("gf_create_allocation: allocation:%p, handle: %x, width:%x, height:%x,pitch: %x. y_tile:%x, ori size:%x\n",
            allocation, allocation->handle, allocation->width, allocation->height, allocation->pitch, y_tile, allocation->orig_size);
    }
    else
    {
        status = E_INVALIDARG;
    }

    return status;
}

int vidmm_get_allocation_info_e3k(adapter_t *adapter, vidmm_get_allocation_info_t *info)
{
    vidmm_allocation_t    *allocation = info->allocation;

    info->ForceLocal = (allocation->preferred_segment.segment_id_0 == SEGMENT_ID_LOCAL_E3K) &&
        (allocation->preferred_segment.segment_id_1 == 0 || allocation->preferred_segment.segment_id_1 == SEGMENT_ID_LOCAL_E3K) &&
        (allocation->preferred_segment.segment_id_2 == 0 || allocation->preferred_segment.segment_id_2 == SEGMENT_ID_LOCAL_E3K) &&
        (allocation->preferred_segment.segment_id_3 == 0 || allocation->preferred_segment.segment_id_3 == SEGMENT_ID_LOCAL_E3K) &&
        (allocation->preferred_segment.segment_id_4 == 0 || allocation->preferred_segment.segment_id_4 == SEGMENT_ID_LOCAL_E3K);
    info->snoop = (allocation->segment_id == SEGMENT_ID_GART_SNOOPABLE_E3K) ? 1 : 0;
    return S_OK;
}

