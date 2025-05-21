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
#ifndef __gf_def_h__
#define __gf_def_h__

#include "gf_perf.h"

#define MAX_SCREENS         4

//Fix-me:
// need set MAX_CORE_CRTCS as 3 in CHX001, just keep it as 2 in e2uma validation.
// currently cannot set 3, because there is no vsync interrupt for the 3rd IGA, this may lead KMD pending in some cases
//
#define MAX_CORE_CRTCS      4

#define OUTPUT_NAME_LEN     16

#define MAX_ENGINE_COUNT   10
#define ALL_ENGINE_MASK    ((1 << MAX_ENGINE_COUNT) - 1)

typedef union
{
#ifndef __BIG_ENDIAN__
 struct
 {
    unsigned int low32;
    unsigned int  high32;
 };
 #else
 struct
 {
    unsigned int  high32;
    unsigned int  low32;
 };
 #endif
    unsigned long long quad64;
}large_inter;

#define ALIGN8 __attribute__ ((aligned(8)))

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

typedef unsigned long long gf_ptr64_t;

typedef struct gf_clip_rect
{
     short x1;
     short y1;
     short x2;
     short y2;
}gf_clip_rect_t;

typedef enum gf_miu_reg_type
{
    GF_MIU_MMIO = 1,
    GF_MIU_PMC = 2,
    GF_MIU_PMU = 3,
}gf_miu_reg_type_t;


typedef enum
{
    GF_STREAM_PS = 0,
    GF_STREAM_SS,
    GF_STREAM_TS,
    GF_STREAM_4S,
    GF_STREAM_5S,
    GF_STREAM_6S,
    GF_STREAM_MAX,
}GF_STREAM_TYPE;

typedef enum
{
    GF_PLANE_PS = 0,
    GF_PLANE_SS,
    GF_PLANE_TS,
    GF_PLANE_FS,
    GF_MAX_PLANE,
}GF_PLANE_TYPE;


typedef enum
{
    GF_AUDIO_FORMAT_REFER_TO_STREAM_HEADER,
    GF_AUDIO_FORMAT_LPCM,
    GF_AUDIO_FORMAT_AC_3,
    GF_AUDIO_FORMAT_MPEG_1,
    GF_AUDIO_FORMAT_MP3,
    GF_AUDIO_FORMAT_MPEG_2,
    GF_AUDIO_FORMAT_AAC_LC,
    GF_AUDIO_FORMAT_DTS,
    GF_AUDIO_FORMAT_ATRAC,
    GF_AUDIO_FORMAT_DSD,
    GF_AUDIO_FORMAT_E_AC_3,
    GF_AUDIO_FORMAT_DTS_HD,
    GF_AUDIO_FORMAT_MLP,
    GF_AUDIO_FORMAT_DST,
    GF_AUDIO_FORMAT_WMA_PRO,
    GF_AUDIO_FORMAT_HE_AAC,
    GF_AUDIO_FORMAT_HE_AAC_V2,
    GF_AUDIO_FORMAT_MPEG_SURROUND
}GF_HDMI_AUDIO_FORMAT_TYPE;

typedef struct
{
    GF_HDMI_AUDIO_FORMAT_TYPE   format;
    unsigned int                 max_channel_num;
    union
    {
        struct
        {
            unsigned int         SR_32kHz             :1; /* Bit0 = 1, support sample rate of 32kHz */
            unsigned int         SR_44_1kHz           :1; /* Bit1 = 1, support sample rate of 44.1kHz */
            unsigned int         SR_48kHz             :1; /* Bit2 = 1, support sample rate of 48kHz */
            unsigned int         SR_88_2kHz           :1; /* Bit3 = 1, support sample rate of 88.2kHz */
            unsigned int         SR_96kHz             :1; /* Bit4 = 1, support sample rate of 96kHz */
            unsigned int         SR_176_4kHz          :1; /* Bit5 = 1, support sample rate of 176.4kHz */
            unsigned int         SR_192kHz            :1; /* Bit6 = 1, support sample rate of 192kHz */
            unsigned int         reserved             :25;
        }sample_rate;

        unsigned int             sample_rate_unit;
    };

    union
    {
        unsigned int             unit;

        // for audio format: LPCM
        struct
        {
            unsigned int         BD_16bit             :1; /* Bit0 = 1, support bit depth of 16 bits */
            unsigned int         BD_20bit             :1; /* Bit1 = 1, support bit depth of 20 bits */
            unsigned int         BD_24bit             :1; /* Bit2 = 1, support bit depth of 24 bits */
            unsigned int         reserved             :29;
        }bit_depth;

        // for audio format: AC-3, MPEG-1, MP3, MPED-2, AAC LC, DTS, ATRAC
        unsigned int             max_bit_Rate; // unit: kHz

        // for audio format: DSD, E-AC-3, DTS-HD, MLP, DST
        unsigned int             audio_format_depend_value; /* for these audio formats, this value is defined in
                                                            it's corresponding format-specific documents*/

        // for audio format: WMA Pro
        struct
        {
            unsigned int         value                :3;
            unsigned int         reserved             :29;
        }profile;
    };
}gf_hdmi_audio_format_t;

typedef struct
{
    unsigned int            num_formats;
    gf_hdmi_audio_format_t audio_formats[16];
}gf_hdmi_audio_formats;

typedef struct
{
    unsigned int engine_mask;  /* in */
    unsigned int  hAllocation;  /* in */
}gf_wait_allocation_idle_t;

typedef struct
{
    int input_size;    /* in */
    int output_size;   /* in */

    gf_ptr64_t input  ALIGN8;/* in */
    gf_ptr64_t output ALIGN8;/* output */
}gf_escape_call_t;

typedef enum
{
    GF_QUERY_VBIOS_VERSION,
    GF_QUERY_TOTAL_VRAM_SIZE,
    GF_QUERY_CPU_VISIBLE_VRAM_SIZE,
    GF_QUERY_RESERV_VRAM_SIZE,
    GF_QUERY_CHIP_ID,
    GF_QUERY_CRTC_OUTPUT,
    GF_QUERY_VSYNC_CNT,
    GF_QUERY_HEIGHT_ALIGN,
    GF_QUERY_LOCAL_VRAM_TYPE,
    GF_QUERY_ENGINE_CLOCK,
    GF_QUERY_VCLK,
    GF_QUERY_MCLK,
    GF_QUERY_VMEM_CLOCK,
    GF_QUERY_VRAM_USED,
    GF_QUERY_GART_USED,
    GF_QUERY_REGISTER_U32,
    GF_QUERY_REGISTER_U64,
    GF_QUERY_INFO_MAX,
    GF_QUERY_I_CLOCK,
    GF_QUERY_CPU_FREQUENCE,
    GF_SET_I_CLOCK,
    GF_QUERY_PMU_REGISTER_U32,
    GF_SET_CPU_FREQUENCE,
    GF_QUERY_VENDOR_ID,
    GF_QUERY_DEVICE_ID,
    GF_QUERY_REVISION_ID,
    GF_QUERY_SEGMENT_FREE_SIZE,
    GF_QUERY_HW_HANG,
    GF_QUERY_SECURED_ON,
    GF_QUERY_PAGE_SWIZZLE_SUPPORT,
    GF_QUERY_PENDING_FRAME_NUM,
    GF_QUERY_PMC_REGISTER_U32,
    GF_QUERY_ALLOCATION_INFO,
    GF_QUERY_GPU_TEMPERATURE,
    GF_QUERY_GPU_TIME_STAMP,
    GF_QUERY_LOCAL_ALLOCATION_MAX_SIZE,
    GF_SET_PERF_SWAP_HINT,
    GF_GET_PERF_SWAP_HINT,
    GF_QUERY_VCP_INDEX,
    GF_QUERY_RET_VCP_INDEX,
    GF_QUERY_VCP_INFO,
    GF_QUERY_VIDEO_SEQ_INDEX,
    GF_QUERY_RET_VIDEO_SEQ_INDEX,
    GF_QUERY_ADAPTER_INFO,

    GF_QUERY_PCIE_LINK_WIDTH,
    GF_QUERY_ACTIVE_ENGINE_COUNT,

    GF_QUERY_GET_VIDEO_BRIDGE_BUFFER,
    GF_QUERY_CHIP_SLICE_MASK,
    GF_SET_MIU_REGISTER_U32,
    GF_QUERY_MIU_REGISTER_U32,
    GF_QUERY_DIAGS,
    GF_SET_REGISTER_U32,

    GF_QUERY_ALLOCATION_INFO_KMD,
    GF_QUERY_SEGMENT_MEM_INFO,
    GF_QUERY_ENGINE_USAGE_STATUS,
    GF_QUERY_PROCESS_INFO,
    GF_QUERY_ENGINE_USAGE_STATUS_EXT,
    GF_QUERY_CORE_CLOCK,
}GF_QUERY_INFO_TYPE;

typedef struct
{
    unsigned int type;    /* in */
    unsigned int argu;    /* in */
    unsigned int buf_len; /*in */
    unsigned int pad;
    union
    {
        int          signed_value;           /* out */
        unsigned int value;                  /* out */
        unsigned long long value64;          /* out */
        void         *buf;                   /* out */
    };
    struct{
        unsigned int device_id;
        unsigned int vendor_id;
        unsigned int pci_link_width;
        unsigned int pci_link_speed;
        unsigned int engine_clk;
        unsigned int memory_clk;
        unsigned int video_clk;
        unsigned int pad;
        unsigned long long total_vram_size;
        unsigned long long cpu_visible_size;
    }diags;
}gf_query_info_t;

typedef gf_query_info_t gf_config_info_t;

/*
if umd need poll events,  the definition need consistent between interrupt.h and gf_def.h
*/


typedef struct
{
    unsigned short  Vendor_ID;
    unsigned char   Device_ID;
    unsigned char   Revision_ID;
}gf_chip_id_t;

#define GF_HDCP_ENABLED            0x0
#define GF_HDCP_FAILED             0x1
#define GF_HDCP_NO_SUPPORT_DEVICE  0x2

typedef struct
{
    unsigned int    enable;                  /* in, 1 enable, 0 disable */
    unsigned int    output_type;             /* in, if 0, all HDMIs will be enable/disable */
    unsigned int    result;                  /* out, see GF_HDCP_XXX */
}gf_hdcp_op_t;

typedef struct
{
    unsigned int  hAllocation;
    unsigned int  engine_mask;
    unsigned int  state;
}gf_get_allocation_state_t;

typedef enum
{
    GF_HW_NONE = 0x00,
    GF_HW_IGA  = 0x01,
}gf_hw_block;

#define STREAM_MASK_UPDATE  0x01
#define STREAM_MASK_FLIP    0x02

#define GF_NAME "gf"

#define     UT_OUTPUT_TYPE_NONE      0x00
#define     UT_OUTPUT_TYPE_CRT       0x01
#define     UT_OUTPUT_TYPE_TV        0x02
#define     UT_OUTPUT_TYPE_HDTV      0x04
#define     UT_OUTPUT_TYPE_PANEL     0x08
#define     UT_OUTPUT_TYPE_DVI       0x10
#define     UT_OUTPUT_TYPE_HDMI      0x20
#define     UT_OUTPUT_TYPE_DP        0x40
#define     UT_OUTPUT_TYPE_MHL       0x80

static inline gf_ptr64_t ptr_to_ptr64(void *ptr)
{
    return (gf_ptr64_t)(unsigned long)ptr;
}

static inline void *ptr64_to_ptr(gf_ptr64_t ptr64)
{
    return (void*)(unsigned long)ptr64;
}

typedef struct {
    unsigned int         pid;
    unsigned int         enable;
    unsigned int         dwDecodeLevel;
    unsigned int         dwDecodeType;
    unsigned int         dwFrameType;
    int                  DecodeWidth;
    int                  DecodeHeight;
    unsigned int         DecodeRTNum;
    unsigned int         SliceNum;
    unsigned int         TotalRenderFrameNum;
    unsigned int         TotalFrameNum;
    unsigned int         TotalIFrameNum;
    unsigned int         TotalPFrameNum;
    unsigned int         TotalBFrameNum;
    unsigned int         TotalBltFrameNum;
    unsigned int         TotalBitstreamSize;
    unsigned int         DecodeApi;
    unsigned int         PresentApi;
    unsigned int         TotalDecodetime; //ms
    unsigned int         TotalDecodeFrameNum;
}gf_vcp_info;

typedef struct
{
    unsigned short  VendorID;                   // (ro)
    unsigned short  DeviceID;                   // (ro)
    unsigned short  Command;                    // Device control
    unsigned short  Status;
    unsigned char   RevisionID;                 // (ro)
    unsigned char   ProgIf;                     // (ro)
    unsigned char   SubClass;                   // (ro)
    unsigned char   BaseClass;                  // (ro)
    unsigned char   CacheLineSize;              // (ro+)
    unsigned char   LatencyTimer;               // (ro+)
    unsigned char   HeaderType;                 // (ro)
    unsigned char   BIST;                       // Built in self test
    unsigned int    ulBaseAddresses[6]; // 6 == PCI_TYPE0_ADDRESSES
    unsigned int    CardBus;
    unsigned short  SubsystemVendorID;
    unsigned short  SubSystemID;
    unsigned int    PCIConfig_Unused[4];
    unsigned int    AGPCapability;
    unsigned int    AGPStatus;
    unsigned int    AGPCommand;
    unsigned int    BusNum;
    unsigned int    SlotNum;

    // for PCI Express support
    unsigned int    LinkCaps;
    unsigned short  LinkStatus;

    int             bSwizzled;
    unsigned int    memoryType;      // 0: 128bits bus; 1: 64bits bus
    unsigned char   pageSelect;      // For memory (2K, 4K, 8K page sizes)
    unsigned char   bankSelect;      // Which address bits to use for bank select
    unsigned char   bankSelectShift; // Which address bits to use for bank select
    unsigned char   bankSelectMask;  // BitMask for bank select bits
    unsigned long long   memorySize;
} gf_pci_config_t;

typedef struct {
    unsigned int VidmemSize;
    unsigned int cjGARTTable;
    unsigned int GARTTableSize;
    unsigned int cjVidmemPageTable;
    unsigned int VidmemPageTableSize;
    unsigned int cjPageKeyBuffer;
    unsigned int PageKeyBufferSize;
    unsigned int cjFBRingBuffer;
    unsigned int FBRingBufferSize;
    int    bNullHW;
    int    bNullBridge;
    int pad;                        // for 32bit alignment compatible
    unsigned long long UserCModelVidMemBase;
    unsigned long long pfnCModelFlushEngine;
    unsigned long long pfnGetCModelInfo;
    unsigned long long pfnGetRegistryFromCModelIni;
} gf_model2_cil_info_t;

typedef struct
{
    gf_pci_config_t            bus_config;
    unsigned int               chipslicemask;
    unsigned int               gpucount;
    unsigned int               osversion;
    int                        bVideoOnly;
    int                        bCTEDumpEnable;
    int                        segmentMapTable[5];
    gf_model2_cil_info_t       model2cilinfo;
    int                        non_simul_chip;
    int                        pad;
} gf_adapter_info_t;

//query segment usage for UMA
typedef struct
{
    //segment mem size in kbytes
    unsigned int               segment_total_size;
    unsigned int               segment_used_size;
    unsigned int               segment_alloc_num;
    unsigned int               segment_id;
} gf_segment_mem_t;
//query process info for UMA
#define KERNAL_BASE_ARRAY_MAX 16
#define KERNAL_NORMAT_ARRAY_MAX 32
#define KERNAL_STRING_ARRAY_MAX 64
typedef struct
{
    unsigned int               handle;
    unsigned int               segment_id;
    unsigned int               format;
    unsigned int               compress_format;
    unsigned int               width;
    unsigned int               height;
    unsigned int               pitch;
    unsigned int               alignment;
    unsigned int               size;
    unsigned int               bit_count;
    unsigned int               physical_addr;
    unsigned int               at_type;
    unsigned int               priority;
    unsigned int               flag;
    unsigned int               status;
    unsigned int               perference;
} gf_process_allocation_t;
typedef struct
{
    unsigned int               device_handle;
    unsigned int               context_handle[KERNAL_NORMAT_ARRAY_MAX];
    unsigned int               context_count;
    unsigned int               pid;
    unsigned char              name[KERNAL_STRING_ARRAY_MAX];
    gf_process_allocation_t    alloc_info[KERNAL_NORMAT_ARRAY_MAX];
    unsigned int               alloc_num;
} gf_process_base_t;
#endif
