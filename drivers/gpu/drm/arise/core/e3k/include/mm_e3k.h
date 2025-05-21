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
#ifndef __MM_E3K_H
#define __MM_E3K_H

#define MAX_WIDTH_2DBLT                  (16 * 1024)  //16K
#define MAX_HEIGHT_2DBLT                 (16 * 1024)  //16K

typedef struct HWM_TILE_INFO_E3K
{
    int                        bTileSupported;    // Display & Render target tiled support
    int                        bTileTexSupported; // Texture tiled support
    int                        bTileZWSupported;  // Z write tiled support
    unsigned int       tileSizeInBytes;
    unsigned int       dwTileSizeX[7];
    unsigned int       dwTileSizeY[7];
} HWM_TILE_INFO_E3K;

//[TODO: need refine
//should be as same as hwm_caps_e3k.cpp
static HWM_TILE_INFO_E3K hwmTileInfo_e3k =
{
    TRUE,       // bTileSupported
    TRUE,       // bTileTexSupported
    TRUE,       // bTileZWSupported
    1024*64,    // tileSizeInBytes = 16k

    // dwTileSizeX
    {
            256,        // 0 8bit or less
            256,        // 1 16bit
            128,         // 2 32bit
            128,         // 3 64bit
            64,         // 4 128bit
            64,         // 5 256bit
            32,         // 6 512bit
    },

    // dwTileSizeY
    {
            256,        // 0 8bit or less
            128,         // 1 16bit
            128,         // 2 32bit
            64,         // 3 64bit
            64,         // 4 128bit
            32,         // 5 256bit
            32,         // 6 512bit
        },
};

#define UNIT_SIZE_E3K 8

static inline unsigned int cacl_log2(unsigned int s)
{
    unsigned int   iter = (unsigned int)-1;
    switch (s)
    {
    case 1:
        iter = 0;
        break;
    case 2:
        iter = 1;
        break;
    case 4:
        iter = 2;
        break;
    case 8:
        iter = 3;
        break;
    case 16:
        iter = 4;
        break;
    case 32:
        iter = 5;
        break;
    case 64:
        iter = 6;
        break;
    case 128:
        iter = 7;
        break;
    case 256:
        iter = 8;
        break;
    case 512:
        iter = 9;
        break;
    case 1024:
        iter = 10;
        break;
    default:
        {
            unsigned int   d = 1;
            do {
                d *= 2;
                iter++;
            } while (d < s);
            iter += ((s << 1) != d);
        }
    }
    return iter;
}

static inline unsigned int getTileSizeInBytes(void)
{
    return hwmTileInfo_e3k.tileSizeInBytes;
}

static inline unsigned int calcTileWidth_e3k (unsigned int bit_cnt)
{
    unsigned int bpp = cacl_log2(bit_cnt);
    unsigned int idx = (bit_cnt > 8) ? (bpp - 3) : 0;

    return hwmTileInfo_e3k.dwTileSizeX[idx];
}

static inline unsigned int calcTileHeight_e3k (unsigned int bit_cnt)
{
    unsigned int bpp = cacl_log2(bit_cnt);
    unsigned int idx = (bit_cnt > 8) ? (bpp - 3) : 0;

    return hwmTileInfo_e3k.dwTileSizeY[idx];
}

static inline unsigned int calcWidthInTiles_e3k (unsigned int width, unsigned int bit_cnt)
{
    unsigned int tsize = calcTileWidth_e3k(bit_cnt);

    return (width + tsize - 1) / tsize;
}

static inline unsigned int calcHeightInTiles_e3k (unsigned int height, unsigned int bit_cnt)
{
    unsigned int tsize = calcTileHeight_e3k(bit_cnt);

    return (height + tsize - 1) / tsize;
}

static inline unsigned int  calcUnitWidth_e3k (unsigned int bit_cnt)
{
    unsigned int bpp = cacl_log2(bit_cnt);
    unsigned int idx = (bit_cnt > 8) ? (bpp - 3) : 0;

    return (1 << ((UNIT_SIZE_E3K - idx + 1) / 2));
}

static inline unsigned int calcUnitHeight_e3k (unsigned int bit_cnt)
{
    unsigned int bpp = cacl_log2(bit_cnt);
    unsigned int idx = (bit_cnt > 8) ? (bpp - 3) : 0;

    return (1 << ((UNIT_SIZE_E3K - idx) / 2));
}

typedef enum MM_ALLOCATION_TYPE_E3K
{
    MM_AT_GENERIC_E3K = 0,
    MM_AT_VERTEXBUFFER_E3K,
    MM_AT_INDEXBUFFER_E3K,
    MM_AT_CSHARP_E3K,
    MM_AT_TEXTURE_E3K,
    MM_AT_TSHARP_E3K,
    MM_AT_SSHARP_E3K,
    MM_AT_BCSHARP_E3K,
    MM_AT_RT_TEXTURE_E3K,
    MM_AT_DS_TEXTURE_E3K,
    MM_AT_RENDERTARGET_E3K,
    MM_AT_RENDERTARGET_SSB_E3K,
    MM_AT_DEPTHBUFFER_E3K,
    MM_AT_DEPTHBUFFER_SSB_E3K,
    MM_AT_STENCILBUFFER_E3K,
    MM_AT_STENCILBUFFER_SSB_E3K,
    MM_AT_SO_BUFFER_E3K,
    MM_AT_SO_DESC_E3K,
    MM_AT_EUPFSHADER_E3K,
    MM_AT_EUPBSHADER_E3K,
    MM_AT_THREADSPACE_E3K,
    MM_AT_BACKBUFFER_E3K,
    MM_AT_CONTEXT_E3K,
    MM_AT_FENCE_E3K,
    MM_AT_FLAGBUFFER_E3K,
    MM_AT_2D_E3K,
    MM_AT_VIDEO_RESERVEDMEM_E3K,
    MM_AT_VIDEO_DECODE_RT_E3K,
    MM_AT_VIDEO_DECODE_COMPBUFF_E3K,
    MM_AT_VIDEO_VIDEOPROCESS_RT_E3K,
    MM_AT_ISP_BACKLIGHT_CTRL_BUFF_E3K,
    MM_AT_HEAP,
    MM_AT_SECOND_BUFFER_ADDRESS,
    MM_AT_DESCRIPTOR_HEAP,
    MM_AT_QUERYPOOL_E3K,
    MM_AT_TBR_SIGNATURE_BUFFER_ADDRESS_E3K,
    MM_AT_MIUCOUNTER_E3K,
    MM_AT_SIGNATURE_PER_DRAW_E3K,
    MM_AT_SVM_POINTER_E3K,
    MM_AT_OCL_SPECIAL_BUFFER_E3K,
    MM_AT_LAST_E3K,
} MM_ALLOCATION_TYPE_E3K;


typedef enum MM_DRIVERID_E3K
{
    MM_DRIVERID_INVALID = 0,
    // 2D
    MM_DRIVERID_2D_SOURCE,
    MM_DRIVERID_2D_DESTINATION,
    // 3D
    // IA
    MM_DRIVERID_VERTEXBUFFER_ADDRESS,//  32
    MM_DRIVERID_INDEXBUFFER_ADDRESS,
    // STO
    MM_DRIVERID_SO_BUFFER_ADDRESS,
    MM_DRIVERID_SO_BUFFER_BOUND,
    // VS
    MM_DRIVERID_VS_SHADER,
    MM_DRIVERID_VS_TEXTURE,          // 128
    MM_DRIVERID_VS_TSHARP,           //   8
    MM_DRIVERID_VS_SSHARP,           //   8
    MM_DRIVERID_VS_BCSHARP,

    // HS
    MM_DRIVERID_HS_SHADER,
    MM_DRIVERID_HS_TEXTURE,          // 128
    MM_DRIVERID_HS_TSHARP,           //   8
    MM_DRIVERID_HS_SSHARP,           //   8
    MM_DRIVERID_HS_BCSHARP,

    // DS
    MM_DRIVERID_DS_SHADER,
    MM_DRIVERID_DS_TEXTURE,          // 128
    MM_DRIVERID_DS_TSHARP,           //   8
    MM_DRIVERID_DS_SSHARP,           //   8
    MM_DRIVERID_DS_BCSHARP,

    // GS
    MM_DRIVERID_GS_SHADER,
    MM_DRIVERID_GS_TEXTURE,          // 128
    MM_DRIVERID_GS_TSHARP,           //   8
    MM_DRIVERID_GS_SSHARP,           //   8
    MM_DRIVERID_GS_BCSHARP,

    // PS
    MM_DRIVERID_PS_SHADER,
    MM_DRIVERID_PS_TEXTURE,          // 128
    MM_DRIVERID_PS_TSHARP,           //   8
    MM_DRIVERID_PS_SSHARP,           //   8
    MM_DRIVERID_PS_BCSHARP,

    // OM
    MM_DRIVERID_RENDERTARGET,        //   8
    MM_DRIVERID_DEPTHBUFFER,
    MM_DRIVERID_STENCILBUFFER,

    // EU ThreadSpace
    MM_DRIVERID_THREADSPACE,
    MM_DRIVERID_CSHARP,

    MM_DRIVERID_RANGE_FLAGBUFFER,    // 32
    MM_DRIVERID_RANGE,               // 32
    MM_DRIVERID_RANGE_BOUND,         // 32

                                     // CS
    MM_DRIVERID_CS_SHADER,
    MM_DRIVERID_CS_TEXTURE,          // 128
    MM_DRIVERID_CS_TSHARP,           // 8
    MM_DRIVERID_CS_SSHARP,           // 8
    MM_DRIVERID_CSUNORDEREDACCESS,   // 8

    MM_DRIVERID_CSL1,
    MM_DRIVERID_CSL2,
    MM_DRIVERID_CSL3,

    MM_DRIVERID_3D_LAST,             //  All above are state related

    // RM 3DBLT,Clear,MASS,AutoGen
    MM_DRIVERID_RM_DEPTHBUFFER,
    MM_DRIVERID_RM_STENCILBUFFER,
    MM_DRIVERID_RM_PS_TEXTURE,
    MM_DRIVERID_RM_RENDERTARGET,
    MM_DRIVERID_RM_VERTEXBUFFER,


    // MISC
    MM_DRIVERID_CONTEXT,
    MM_DRIVERID_FENCE,
    MM_DRIVERID_SIGNATURE,
    MM_DRIVERID_SIGNATURE_PER_DRAW,
    MM_DRIVERID_DIP_PARAM_BUFFER_ADDRESS,
    MM_DRIVERID_DIP_INDIRECT_BUFFER_ADDRESS,
    MM_DRIVERID_DISPATCH_INDIRECT_BUFFER_ADDRESS,
    MM_DRIVERID_SO_BUFFER_FILL_SIZE_POOL,
    MM_DRIVERID_UA_APPEND_FILLED_SIZE_POOL,
    MM_DRIVERID_FLAGBUFFER,
    MM_DRIVERID_CMDBUFFER,
    MM_DRIVERID_SVM_POINTER,
    MM_DRIVERID_MIUCOUNTER,
    // Video
    MM_DRIVERID_VIDEO_RESERVEDMEM,          // 16
    MM_DRIVERID_VIDEO_SIGNATURE,            // 8
    MM_DRIVERID_VIDEO_FENCE,                // 1
    MM_DRIVERID_VIDEO_PATCH,                // 8
    MM_DRIVERID_VIDEO_DECODE_COMPBUFF,      // 1
    MM_DRIVERID_VIDEO_DECODE_MSVD,          // 20
    MM_DRIVERID_VIDEO_DECODE_CMD,           // 1
    MM_DRIVERID_VIDEO_DECODE_INS,           // 1
    MM_DRIVERID_VIDEO_DECODE_VCPDMA,
    MM_DRIVERID_VIDEO_DECODE_VCPSBDMA,
    MM_DRIVERID_VIDEO_DECODE_QTMDMA,
    MM_DRIVERID_VIDEO_DECODE_DRVDMA,
    MM_DRIVERID_VIDEO_DRAWID,

    // DIU
    MM_DRIVERID_VIDEO_PS1_ADDR0,
    MM_DRIVERID_VIDEO_PS2_ADDR0,
    MM_DRIVERID_VIDEO_SS1_ADDR0,
    MM_DRIVERID_VIDEO_SS2_ADDR0,
    MM_DRIVERID_VIDEO_SS1_MMIOUPDATE_ADDR0,
    MM_DRIVERID_VIDEO_SS2_MMIOUPDATE_ADDR0,
    MM_DRIVERID_VIDEO_SS1_MMIOFLIP_ADDR0,
    MM_DRIVERID_VIDEO_SS2_MMIOFLIP_ADDR0,
    MM_DRIVERID_VIDEO_BACKLIGHT_CTRL_VALUE,

    //ISP
    MM_DRIVERID_ISP_VPP_SRC,
    MM_DRIVERID_ISP_VPP_DST,
    MM_DRIVERID_ISP_CAPTURE,
    MM_DRIVERID_ISP_BACKLIGHT_CTRL_BUFF,

    // Cache invalidate
    MM_DRIVERID_INVALID_STREAMCACHE_VS_ADDRESS,
    MM_DRIVERID_INVALID_STREAMCACHE_HS_ADDRESS,
    MM_DRIVERID_INVALID_STREAMCACHE_DS_ADDRESS,
    MM_DRIVERID_INVALID_STREAMCACHE_GS_ADDRESS,
    MM_DRIVERID_INVALID_STREAMCACHE_PS_ADDRESS,
    MM_DRIVERID_INVALID_STREAMCACHE_CS_ADDRESS,
    MM_DRIVERID_INVALID_L2_CACHE_ADDRESS,
    MM_DRIVERID_INVALID_L2_CACHE_MASK,

    // split 32 bits address to two 16 bits address in ppmode
    MM_DRIVERID_PPMODE_SPLIT_ADDR,

    // only for model extension SetBufferAddrToCB
    MM_DRIVERID_VERTEXBUFFER_40BIT_ADDRESS,

    MM_DRIVERID_HEAP,
    MM_DRIVERID_SECOND_BUFFER_ADDRESS,
    MM_DRIVERID_DESCRIPTOR_HEAP,
    MM_DRIVERID_QUERYPOOL,
    MM_DRIVERID_TBR_SIGNATURE_BUFFER_ADDRESS,
    MM_DRIVERID_LAST,

} MM_DRIVERID_E3K;


typedef struct MM_ALLOCATION_DESC MM_ALLOCATION_DESC_E3K;


typedef struct
{
    int          bTileSupport;
    unsigned int tileSizeInBytes;
    unsigned int dwTileSizeX[10];
    unsigned int dwTileSizeY[10];
}KMDTILEINFO_E3K;

extern const KMDTILEINFO_E3K hwKMDTileInfo_e3k;

#define HW_KMD_TILE_INFO_E3K \
{\
        TRUE,                    /* bTileSupported */ \
        0x10000,                 /* tileSizeInBytes = 64KB */ \
        /* dwTileSizeX */ \
        0x200,                   /* dwTileSizeX -   1bit, E1 not support */ \
        0xFFFFFFFF,              /*dummy item for Nearestlog2() */ \
        0x0,                     /* dwTileSizeX -   4bit, E1 not support */ \
        0x100,                    /*  dwTileSizeX -     8bit,*/ \
        0x100,                    /* dwTileSizeX -  16bit */ \
        0x80,                    /* dwTileSizeX -  32bit */ \
        0x80,                    /* dwTileSizeX -  64bit,*/ \
        0x40,                    /* dwTileSizeX - 128bit,*/ \
        0x40,                    /* dwTileSizeX - 256bit,*/ \
        0x20,                    /* dwTileSizeX - 512bit,*/ \
        /* dwTileSizeY */ \
        0x100,                   /* dwTileSizeY -   1bit, E1 not support */ \
        0xFFFFFFFF,              /* dummy item for Nearestlog2() */ \
        0x0,                     /* dwTileSizeY -   4bit, E1 not support */ \
        0x100,                   /* dwTileSizeY -       8bit, */ \
        0x80,                    /* dwTileSizeY -  16bit */ \
        0x80,                    /* dwTileSizeY -  32bit */ \
        0x40,                    /* dwTileSizeY -  64bit, */ \
        0x40,                    /* dwTileSizeX - 128bit, */ \
        0x20,                    /* dwTileSizeX - 256bit, */ \
        0x20,                    /* dwTileSizeX - 512bit, */ \
}

static inline int IsPow2(unsigned int value)
{
    return (!(value & (value-1)));
}

/*---------------------------------------------------------------------------*/
/* NearestLog2()                                                             */
/*                                                                           */
/* Finds the closest integer base-2 log of a value.                          */
/* (0 will return 0xFFFFFFFF)                                                */
/*---------------------------------------------------------------------------*/
static inline unsigned int NearestLog2(unsigned int  val)
{
    unsigned int result = 0xFFFFFFFF;
    unsigned int input  = val;

    while (val & 0xFFFFFFF0)
    {
        val >>= 4;
        result += 4;
    }

    while (val)
    {
        val >>= 1;
        result++;
    }

    //
    // if val is not a pow2, increment by one, since
    // this function floors
    //

    if (IsPow2(input) == 0)
    {
        result++;
    }

    return(result);
}
#define ALIGNED_2KBits(Value)        (((Value) + 0xFF) & ~0xFF)
#define ALIGNED_128KBYTE(Value)      (((Value) + 0x1FFFF) & ~0x1FFFF)


#define TILE_SIZE_IN_BYTES (hwKMDTileInfo_e3k.tileSizeInBytes)
#define TILE_SIZE_X(bpp) (hwKMDTileInfo_e3k.dwTileSizeX[NearestLog2(bpp)])
#define TILE_SIZE_Y(bpp) (hwKMDTileInfo_e3k.dwTileSizeY[NearestLog2(bpp)])
#define ALIGN_TO_X_TILE_SIZE(value,bpp) (((value) + TILE_SIZE_X(bpp) - 1) & ~(TILE_SIZE_X(bpp) - 1))
#define ALIGN_TO_Y_TILE_SIZE(value,bpp) (((value) + TILE_SIZE_Y(bpp) - 1) & ~(TILE_SIZE_Y(bpp) - 1))
#define NUM_OF_X_TILES(value,bpp) (ALIGN_TO_X_TILE_SIZE((value),(bpp)) / TILE_SIZE_X(bpp))
#define NUM_OF_Y_TILES(value,bpp) (ALIGN_TO_Y_TILE_SIZE((value),(bpp)) / TILE_SIZE_Y(bpp))


/*
 *  definition segment assocated
 */
typedef enum _SEGMENT_ID_E3K
{
    SEGMENT_ID_INVALID_E3K                  = 0x0,  //invalid
    SEGMENT_ID_LOCAL_E3K                    = 0x1,  //local low /local cpu visible/pcie bar memory range
    SEGMENT_ID_GART_UNSNOOPABLE_E3K         = 0x2,  //pcie unsnoop
    SEGMENT_ID_GART_SNOOPABLE_E3K           = 0x3,  //pcie snoonp
    SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K     = 0x4,  //local video/local cpu unvisible and inside 4G
    SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1   = 0x5,  //local high/local cpu unvisible and outside 4G
    SEGMENT_ID_SECURE_RANGE_E3K             = 0x6,
    SEGMENT_ID_SECURE_RANGE_E3K_1           = 0x7,
    SEGMENT_ID_MAX_E3K                      = 0x8,
} SEGMENT_ID_E3K;

#define SMALL_HEAP_SIZE_E3K                   (4*1024*1024)       //4M
#define SMALL_HEAP_MAX_ALLOCATE_SIZE_E3K      (32*1024)           //32K
#define SMALL_HEAP_MAX_ALLOCATE_SIZE_GART_E3K (64*1024)           //64K

// hang dump define
#define E3K_RINGBUFFER_FOR_HANG_SIZE        (0x4000)          // include DH_COMMON_INFO
#define E3K_DMABUFFER_FOR_HANG_SIZE         (0x400000)
#define E3K_CONTEXTBUFFER_FOR_HANG_SIZE     (0x160000)
#define E3K_TRANSFERBUFFER_FOR_HANG_SIZE    (0x1000000) //16M
#define E3K_RANGE_FORMAT_BUFFER_SIZE        (0x10000)

#define E3K_LOCAL_MEMORY_MAX_ADDR_FOR_HANG_DUMP (252*1024*1024)

#define GPU_PAGE_SIZE_E3K                  (4*1024)             //4K
// should use ULL instead of U as suffix, since 2G * 3 > 4G, the maximum of 32bit
#define GART_MEMORY_SIZE_E3K               (8*1024*1024*1024ULL)     //8G, actually this is pcie memory
#define PAGING_SEGMENT_SIZE_E3K            (1*1024*1024*1024ULL)     //1G
#define SNOOPABLE_SEGMENT_RATION_E3K(size) ((adapter->hw_caps.snoop_only)?(size):(((size)*1/2)&~15))

//secure range
#define SECURE_RANGE_NUM 8
#define SECURE_RANGE_BUFFER_SIZE             (32*1024*1024)
#define SECURE_RANGE_ENCRYPTEN_ADDRESS       (0xD3F0)
#define SECURE_RANGE_SCRAMBLE_PATTEN_ADDRESS (0xD300)

#define MIU0_RANGE_BASE_ADDRESS              (0xD390)
#define MIU1_RANGE_BASE_ADDRESS              (0xD790)

//Burst length buffer

#define BL_BUFFER_SIZE         0x2000000   //32M
#define BL_BUFFER_ALIGN        0x10000     //64K
//from spec, current bl index bit using 18bit plan.
#define BL_INDEX_BITS          18
//from spec, 1byte slot map to 512byte allocation
#define BL_MAPPING_RATIO       512

#define BL_SLOT_SIZE           (BL_BUFFER_SIZE >> BL_INDEX_BITS)
#define BL_SLICE_ALIGNMENT     (BL_BUFFER_SIZE >> BL_INDEX_BITS)

#endif

