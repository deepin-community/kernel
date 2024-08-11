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

#ifndef _SURFACE_FORMAT_H
#define _SURFACE_FORMAT_H


#define SURFACE_FORMAT_TIMESTAMP  "2015-09-16 16:40:28"
typedef enum
{
    HSF_UNKNOWN                   = 0,
    HSF_R64G64B64A64_FLOAT        = 1,
    HSF_R64G64B64_FLOAT           = 2,
    HSF_R32G32B32A32_TYPELESS     = 3,
    HSF_R32G32B32A32_FLOAT        = 4,       //128bpp RGBA
    HSF_R32G32B32A32_UINT         = 5,
    HSF_R32G32B32A32_SINT         = 6,
    HSF_R32G32B32A32_SNORM        = 7,
    HSF_R32G32B32A32_SCALE        = 8,
    HSF_R32G32B32A32_UNORM        = 9,
    HSF_R32G32B32A32_USCALE       = 10,
    HSF_R32G32B32A32_FIX          = 11,
    HSF_R64G64_FLOAT              = 12,
    HSF_R32G32B32_TYPELESS        = 13,
    HSF_R32G32B32_FLOAT           = 14,     //128bpp RGB
    HSF_R32G32B32_UINT            = 15,
    HSF_R32G32B32_SINT            = 16,
    HSF_R32G32B32_UNORM           = 17,
    HSF_R32G32B32_USCALE          = 18,
    HSF_R32G32B32_SNORM           = 19,
    HSF_R32G32B32_SCALE           = 20,
    HSF_R32G32B32_FIX             = 21,
    HSF_R16G16B16A16_TYPELESS     = 22,     //64bpp RGBA
    HSF_R16G16B16A16_FLOAT        = 23,
    HSF_R16G16B16A16_UNORM        = 24,
    HSF_R16G16B16A16_UINT         = 25,
    HSF_R16G16B16A16_SNORM        = 26,
    HSF_R16G16B16A16_SINT         = 27,
    HSF_R16G16B16A16_SCALE        = 28,
    HSF_R16G16B16A16_USCALE       = 29,
    HSF_R16G16B16A16_SSCALE       = 30,
    HSF_R12G12B12A12_UNORM        = 31,
    HSF_R16G16B16_FLOAT           = 32,     //64bpp RGB
    HSF_R16G16B16_SINT            = 33,
    HSF_R16G16B16_SNORM           = 34,
    HSF_R16G16B16_SCALE           = 35,
    HSF_R16G16B16_UINT            = 36,
    HSF_R16G16B16_UNORM           = 37,
    HSF_R16G16B16_USCALE          = 38,
    HSF_R32G32_TYPELESS           = 39,
    HSF_R32G32_FLOAT              = 40,     //64bpp RG
    HSF_R32G32_UINT               = 41,
    HSF_R32G32_SINT               = 42,
    HSF_R32G32_SNORM              = 43,
    HSF_R32G32_SCALE              = 44,
    HSF_R32G32_UNORM              = 45,
    HSF_R32G32_USCALE             = 46,
    HSF_R32G32_FIX                = 47,
    HSF_R32G8X24_TYPELESS         = 48,
    HSF_D32_FLOAT_S8X24_UINT      = 49,
    HSF_R32_FLOAT_X8X24_TYPELESS  = 50,
    HSF_X32_TYPELESS_G8X24_UINT   = 51,
    HSF_R64_FLOAT                 = 52,
    HSF_R12G12B12_UNORM           = 53,
    HSF_R10G10B10A2_TYPELESS      = 54,
    HSF_R10G10B10A2_UNORM         = 55,     //32bpp RGBA
    HSF_R10G10B10A2_UINT          = 56,
    HSF_R10G10B10X2_UINT          = 57,     //32bpp RGBA
    HSF_R10G10B10X2_USCALE        = 58,
    HSF_R10G10B10A2_SNORM         = 59,
    HSF_R10G10B10A2_SINT          = 60,
    HSF_R10G10B10X2_SNORM         = 61,
    HSF_B10G10R10A2_SNORM         = 62,
    HSF_B10G10R10A2_UNORM         = 63,
    HSF_B10G10R10X2_UNORM         = 64,
    HSF_A2B10G10R10_UNORM         = 65,
    HSF_A2B10G10R10_SNORM         = 66,
    HSF_A2B10G10R10_USCALE        = 67,
    HSF_A2B10G10R10_SSCALE        = 68,
    HSF_R10G10B10_SINT            = 69,
    HSF_R10G10B10_SNORM           = 70,
    HSF_R10G10B10_SCALE           = 71,
    HSF_R10G10B10_UNIT            = 72,
    HSF_R10G10B10_UNORM           = 73,
    HSF_R10G10B10_USCALE          = 74,
    HSF_R10G10_SINT               = 75,
    HSF_R10G10_SNORM              = 76,
    HSF_R10G10_SCALE              = 77,
    HSF_R10G10_UINT               = 78,
    HSF_R10G10_UNORM              = 79,
    HSF_R10G10_USCALE             = 80,
    HSF_R10G10B10_FLOAT_A2_UNORM  = 81,
    HSF_R11G11B10_FLOAT           = 82,     //32bpp RGB
    HSF_B10G11R11_FLOAT           = 83,
    HSF_R11G11_FLOAT              = 84,
    HSF_R8G8B8A8_TYPELESS         = 85,
    HSF_R8G8B8A8_UNORM            = 86,     //32bpp RGBA
    HSF_R8G8B8A8_UNORM_SRGB       = 87,
    HSF_R8G8B8X8_UNORM            = 88,     //32bpp RGBA
    HSF_B8G8R8A8_UNORM            = 89,
    HSF_B8G8R8A8_UNORM_SRGB       = 90,
    HSF_B8G8R8X8_UNORM            = 91,
    HSF_B8G8R8X8_UNORM_SRGB       = 92,
    HSF_X8R8G8B8_UNORM            = 93,
    HSF_X8B8G8R8_UNORM            = 94,
    HSF_A8R8G8B8_UNORM            = 95,
    HSF_A8B8G8R8_UNORM            = 96,
    HSF_R8G8B8A8_UINT             = 97,
    HSF_R8G8B8A8_SNORM            = 98,
    HSF_R8G8B8A8_SINT             = 99,
    HSF_B8G8R8A8_XNORM            = 100,
    HSF_R8G8B8A8_SSCALE           = 101,
    HSF_R8G8B8A8_USCALE           = 102,
    HSF_R8G8B8_SINT               = 103,
    HSF_R8G8B8_SNORM              = 104,
    HSF_R8G8B8_SCALE              = 105,
    HSF_R8G8B8_UINT               = 106,
    HSF_R8G8B8_UNORM              = 107,
    HSF_R8G8B8_USCALE             = 108,
    HSF_R16G16_TYPELESS           = 109,
    HSF_R16G16_FLOAT              = 110,     //32bpp RG
    HSF_R16G16_UNORM              = 111,
    HSF_R16G16_UINT               = 112,
    HSF_R16G16_SNORM              = 113,
    HSF_R16G16_SINT               = 114,
    HSF_R16G16_SCALE              = 115,
    HSF_R16G16_USCALE             = 116,
    HSF_R16G16_SSCALE             = 117,
    HSF_R32_TYPELESS              = 118,
    HSF_D32_FLOAT                 = 119,
    HSF_D32_UNORM                 = 120,     //32bpp R
    HSF_R32_FLOAT                 = 121,
    HSF_R32_UINT                  = 122,
    HSF_R32_SINT                  = 123,
    HSF_R32_UNORM                 = 124,
    HSF_R32_USCALE                = 125,
    HSF_R32_SNORM                 = 126,
    HSF_R32_SCALE                 = 127,
    HSF_R32_FIX                   = 128,
    HSF_R24G8_TYPELESS            = 129,
    HSF_D32_FLOAT_S8_UINT         = 130,

    HSF_R24_UNORM_X8_TYPELESS     = 132,
    HSF_X24_TYPELESS_G8_UINT      = 133,
    HSF_D24_UNORM                 = 134,
    HSF_R24_FLOAT                 = 135,
    HSF_R8G8_TYPELESS             = 136,
    HSF_R8G8_UNORM                = 137,
    HSF_R8G8_UINT                 = 138,
    HSF_R8G8_SNORM                = 139,
    HSF_R8G8_SINT                 = 140,
    HSF_R8G8_USCALE               = 141,
    HSF_R8G8_SSCALE               = 142,
    HSF_R16_TYPELESS              = 143,
    HSF_R16_FLOAT                 = 144,     //16bpp R
    HSF_D16_UNORM                 = 145,
    HSF_R16_UNORM                 = 146,
    HSF_R16_UINT                  = 147,
    HSF_R16_SNORM                 = 148,
    HSF_R16_SINT                  = 149,
    HSF_R16_USCALE                = 150,
    HSF_R16_SSCALE                = 151,
    HSF_A16_UNORM                 = 152,
    HSF_B5G6R5_UNORM              = 153,
    HSF_R5G6B5_UNORM              = 154,
    HSF_B5G5R5X1_UNORM            = 155,     //16bpp RGBA
    HSF_B5G5R5A1_UNORM            = 156,     //16bpp RGBA
    HSF_R5G5B5A1_UNORM            = 157,
    HSF_A1B5G5R5_UNORM            = 158,
    HSF_A1R5G5B5_UNORM            = 159,
    HSF_B4G4R4A4_UNORM            = 160,
    HSF_B4G4R4X4_UNORM            = 161,     //16bpp BGRA
    HSF_R4G4B4A4_UNORM            = 162,     //16bpp RGBA
    HSF_A4B4G4R4_UNORM            = 163,
    HSF_A4R4G4B4_UNORM            = 164,
    HSF_R4G4B4_UNORM              = 165,
    HSF_R5G5B5_UNORM              = 166,
    HSF_U5V5L6_XNORM              = 167,
    HSF_L16_UNORM                 = 168,     //16bpp
    HSF_L8_A8_UNORM               = 169,
    HSF_R10_SINT                  = 170,
    HSF_R10_SNORM                 = 171,
    HSF_R10_SCALE                 = 172,
    HSF_R10_UINT                  = 173,
    HSF_R10_UNORM                 = 174,
    HSF_R10_USCALE                = 175,
    HSF_R2G2B2A2_UNORM            = 176,
    HSF_R8_TYPELESS               = 177,
    HSF_R8_UNORM                  = 178,
    HSF_R8_UINT                   = 179,
    HSF_R8_SNORM                  = 180,
    HSF_R8_SINT                   = 181,
    HSF_R8_USCALE                 = 182,
    HSF_R8_SSCALE                 = 183,
    HSF_A8_UNORM                  = 184,
    HSF_L8_UNORM                  = 185,
    HSF_A12_UNORM                 = 186,
    HSF_A4_UNORM                  = 187,
    HSF_R1_UNORM                  = 188,
    HSF_R3G3B2_UNORM              = 189,
    HSF_L4A4_UNORM                = 190,     //8bpp RA
    HSF_A4L4_UNORM                = 191,
    HSF_L4A4_VIDEO                = 192,
    HSF_AYUV_VIDEO                = 193,     //32bpp
    HSF_YUYV                      = 194,     //16bpp
    HSF_NV12                      = 195,     //16bpp
    HSF_P010                      = 196,     //32bpp
    HSF_P016                      = 197,     //32bpp
    HSF_R9G9B9E5_SHAREDEXP        = 198,
    HSF_R8G8_B8G8_UNORM           = 199,
    HSF_G8R8_G8B8_UNORM           = 200,
    HSF_BC1_UNORM                 = 201,
    HSF_BC1_UNORM_SRGB            = 202,
    HSF_BC2_UNORM                 = 203,
    HSF_BC2_UNORM_SRGB            = 204,
    HSF_BC3_UNORM                 = 205,
    HSF_BC3_UNORM_SRGB            = 206,
    HSF_BC4_UNORM                 = 207,
    HSF_BC4_SNORM                 = 208,
    HSF_BC4_UNORM_L               = 209,
    HSF_BC4_SNORM_L               = 210,
    HSF_BC5_UNORM                 = 211,
    HSF_BC5_SNORM                 = 212,
    HSF_BC5_UNORM_LA              = 213,
    HSF_BC5_SNORM_LA              = 214,
    HSF_BC6H_UF16                 = 215,
    HSF_BC6H_SF16                 = 216,
    HSF_BC7_UNORM                 = 217,
    HSF_BC7_UNORM_SRGB            = 218,
    HSF_G8_UINT                   = 219,
    HSF_3DC_UNORM                 = 220,
    HSF_YUY2                      = 221,     //16bpp
    HSF_Y216_VIDEO                = 222,
    HSF_Y210_VIDEO                = 223,
    HSF_A4I4_VIDEO                = 224,
    HSF_A8L8_UNORM                = 225,
    HSF_L8A8_UNORM                = 226,
    HSF_ZL1                       = 227,
    HSF_R16G16B16A16_UNORM_SRGB   = 228,  // only used for TU border color format, internal use, added by peng@2018.01.04
    HSF_YCRCB_MB_8_422            = 229,
    HSF_YCRCB_MB_8_420            = 230,
    HSF_YCRCB_MB_16_422           = 231,

    HSF_UYVA1010102_VIDEO         = 233,
    HSF_UYVY                      = 234,
    HSF_BAYER_12                  = 235,
    HSF_BAYER                     = 236,
    HSF_L8_SNORM                  = 237, // SLUMINANCE; channel_mask =0x1; bpt =8

    HSF_YV12                      = 238,
    HSF_L8A8_SNORM                = 239, // SLUMINANCE_ALPHA8; channel_mask =0x3; bpt =16

    HSF_S1_UINT                   = 240,
    HSF_S16_UINT                  = 241,
    HSF_S4_UINT                   = 242,
    HSF_S8_UINT                   = 243,
    HSF_I12_UNORM                 = 244,
    HSF_I16_UNORM                 = 245,
    HSF_I4_UNORM                  = 246,
    HSF_I8_UNORM                  = 247,
    HSF_L12_UNORM                 = 248,
    HSF_L12_A12_UNORM             = 249,
    HSF_L12_A4_UNORM              = 250,
    HSF_L4_UNORM                  = 252,
    HSF_L4_A4_UNORM               = 253,
    HSF_L6_A2_UNORM               = 254,
    HSF_RGB8_ETC2                            = 255,
    HSF_SRGB8_ETC2                           = 256,
    HSF_RGBA8_ETC2_EAC                       = 257,
    HSF_SRGB8_ALPHA8_ETC2_EAC                = 258,
    HSF_R11_EAC                              = 259,
    HSF_RG11_EAC                             = 260,
    HSF_SIGNED_R11_EAC                       = 261,
    HSF_SIGNED_RG11_EAC                      = 262,
    HSF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2        = 263,
    HSF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2       = 264,
    HSF_COMPRESSED_ALPHA                     = 265,
    HSF_COMPRESSED_INTENSITY                 = 266,
    HSF_COMPRESSED_LUMINANCE                 = 267,
    HSF_COMPRESSED_RED                       = 268,
    HSF_COMPRESSED_RED_RGTC1                 = 269,
    HSF_COMPRESSED_RG                        = 270,
    HSF_COMPRESSED_RG_RGTC2                  = 271,
    HSF_COMPRESSED_RGB                       = 272,
    HSF_COMPRESSED_RGB_BPTC_SIGNED_FLOAT     = 273,
    HSF_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT   = 274,
    HSF_COMPRESSED_RGBA                      = 275,
    HSF_COMPRESSED_RGBA_BPTC_UNORM           = 276,
    HSF_COMPRESSED_SIGNED_RED_RGTC1          = 277,
    HSF_COMPRESSED_SIGNED_RG_RGTC2           = 278,
    HSF_COMPRESSED_SLUMINANCE                = 279,
    HSF_COMPRESSED_SLUMINANCE_ALPHA          = 280,
    HSF_COMPRESSED_SRGB                      = 281,
    HSF_COMPRESSED_SRGB_ALPHA                = 282,
    HSF_COMPRESSED_SRGB_ALPHA_BPTC_UNORM     = 283,
    HSF_R10G10B10A2_USCALE                   = 284,
    HSF_R10G10B10A2_SSCALE                   = 285,
    HSF_R32G32B32A32_SSCALE                  = 286,
    HSF_R32G32B32_SSCALE                     = 287,
    HSF_R32G32_SSCALE                        = 288,
    HSF_R16G16B16_SSCALE                     = 289,
    HSF_R32_SSCALE                           = 290,
    HSF_R8G8B8_SSCALE                        = 291,


    HSF_RGBA_ASTC_4X4                        = 321,
    HSF_RGBA_ASTC_5X4                        = 322,
    HSF_RGBA_ASTC_5X5                        = 323,
    HSF_RGBA_ASTC_6X5                        = 324,
    HSF_RGBA_ASTC_6X6                        = 325,
    HSF_RGBA_ASTC_8X5                        = 326,
    HSF_RGBA_ASTC_8X6                        = 327,
    HSF_RGBA_ASTC_8X8                        = 328,
    HSF_RGBA_ASTC_10X5                       = 329,
    HSF_RGBA_ASTC_10X6                       = 330,
    HSF_RGBA_ASTC_10X8                       = 331,
    HSF_RGBA_ASTC_10X10                      = 332,
    HSF_RGBA_ASTC_12X10                      = 333,
    HSF_RGBA_ASTC_12X12                      = 334,
    HSF_ASTC_RESERVED0                       = 335,
    HSF_ASTC_RESERVED1                       = 336,
    HSF_RGBA_ASTC_4X4_SRGB                   = 337,
    HSF_RGBA_ASTC_5X4_SRGB                   = 338,
    HSF_RGBA_ASTC_5X5_SRGB                   = 339,
    HSF_RGBA_ASTC_6X5_SRGB                   = 340,
    HSF_RGBA_ASTC_6X6_SRGB                   = 341,
    HSF_RGBA_ASTC_8X5_SRGB                   = 342,
    HSF_RGBA_ASTC_8X6_SRGB                   = 343,
    HSF_RGBA_ASTC_8X8_SRGB                   = 344,
    HSF_RGBA_ASTC_10X5_SRGB                  = 345,
    HSF_RGBA_ASTC_10X6_SRGB                  = 346,
    HSF_RGBA_ASTC_10X8_SRGB                  = 347,
    HSF_RGBA_ASTC_10X10_SRGB                 = 348,
    HSF_RGBA_ASTC_12X10_SRGB                 = 349,
    HSF_RGBA_ASTC_12X12_SRGB                 = 350,
    HSF_R8G8B8_SRGB                          = 351, //only used for TU border color format, internal use, added by Jenna@20180326
    HSF_ASTC_RESERVED3                       = 352,
    HSF_RGBA_ASTC_3x3x3                      = 353,
    HSF_RGBA_ASTC_4x3x3                      = 354,
    HSF_RGBA_ASTC_4x4x3                      = 355,
    HSF_RGBA_ASTC_4x4x4                      = 356,
    HSF_RGBA_ASTC_5x4x4                      = 357,
    HSF_RGBA_ASTC_5x5x4                      = 358,
    HSF_RGBA_ASTC_5x5x5                      = 359,
    HSF_RGBA_ASTC_6x5x5                      = 360,
    HSF_RGBA_ASTC_6x6x5                      = 361,
    HSF_RGBA_ASTC_6x6x6                      = 362,
    HSF_L16A16_UNORM                         = 363, // ; channel_mask =0x3; bpt =32
    HSF_L16A16_SNORM                         = 364, // ; channel_mask =0x3; bpt =32
    HSF_ASTC_RESERVED6                       = 365,
    HSF_ASTC_RESERVED7                       = 366,
    HSF_ASTC_RESERVED8                       = 367,
    HSF_ASTC_RESERVED9                       = 368,
    HSF_RGBA_ASTC_3x3x3_SRGB                 = 369,
    HSF_RGBA_ASTC_4x3x3_SRGB                 = 370,
    HSF_RGBA_ASTC_4x4x3_SRGB                 = 371,
    HSF_RGBA_ASTC_4x4x4_SRGB                 = 372,
    HSF_RGBA_ASTC_5x4x4_SRGB                 = 373,
    HSF_RGBA_ASTC_5x5x4_SRGB                 = 374,
    HSF_RGBA_ASTC_5x5x5_SRGB                 = 375,
    HSF_RGBA_ASTC_6x5x5_SRGB                 = 376,
    HSF_RGBA_ASTC_6x6x5_SRGB                 = 377,
    HSF_RGBA_ASTC_6x6x6_SRGB                 = 378,
    HSF_R64_UINT                             = 379,
    HSF_R64G64_UINT                          = 380,
    HSF_R64G64B64_UINT                       = 381,
    HSF_R64G64B64A64_UINT                    = 382,
    HSF_R64_SINT                             = 383,
    HSF_R64G64_SINT                          = 384,
    HSF_R64G64B64_SINT                       = 385,
    HSF_R64G64B64A64_SINT                    = 386,
    HSF_R4G4B4X4_UNORM                       = 387,
    HSF_R5G5B5X1_UNORM                       = 388,
    HSF_R8G8B8X8_SNORM                       = 389,
    HSF_R8G8B8X8_UINT                        = 390,
    HSF_R8G8B8X8_SINT                        = 391,
    HSF_R8G8B8X8_UNORM_SRGB                  = 392,
    HSF_R10G10B10X2_UNORM                    = 393,
    HSF_R16G16B16X16_UNORM                   = 394,
    HSF_R16G16B16X16_SNORM                   = 395,
    HSF_R16G16B16X16_FLOAT                   = 396,
    HSF_R16G16B16X16_UINT                    = 397,
    HSF_R16G16B16X16_SINT                    = 398,
    HSF_R32G32B32X32_FLOAT                   = 399,
    HSF_R32G32B32X32_UINT                    = 400,
    HSF_R32G32B32X32_SINT                    = 401,

    HSF_S8_UINT_D24_UNORM                    = 402,
    HSF_D24_UNORM_S8_UINT                    = 403,
    HSF_S8_UINT_D32_FLOAT                    = 404,

    HSF_R24_UNORM                            = 405,
    HSF_R24G24_UNORM                         = 406,
    HSF_R24G24B24A24_UNORM                   = 407,
    HSF_R10G10B10A10_UNORM                   = 408,
    HSF_BC1_TYPELESS                         = 409,

    HSF_B8G8R8A8_TYPELESS                    = 410,
    HSF_B8G8R8X8_TYPELESS                    = 411,
    HSF_BC2_TYPELESS                         = 412,
    HSF_BC3_TYPELESS                         = 413,
    HSF_BC4_TYPELESS                         = 414,
    HSF_BC5_TYPELESS                         = 415,
    HSF_BC6H_TYPELESS                        = 416

} Hw_Surf_Format;

typedef enum COMPRESS_MXUFORMAT
{
    CP_OFF                  = 0,
    CP_R8G8B8A8_T           = 1,
    CP_R8G8B8A8_L           = 2,
    CP_A8R8G8B8_T           = 3,
    CP_A8R8G8B8_L           = 4,
    CP_R10G10B10A2          = 5,
    CP_A2B10G10R10          = 6,
    CP_R11G11B10            = 7,
    CP_R5G6B5               = 8,
    CP_NV12                 = 9,
    CP_YUYV                 = 10,
    CP_UYVY                 = 11,
    CP_R10G10B10A2_T        = 12,
    CP_A2B10G10R10_T        = 13,
    CP_NV12_10              = 14,
    CP_Z32                  = 15,
    CP_R16G16B16A16_T       = 16,
    CP_Z16                  = 17,
    CP_Z24                  = 18,
    CP_R11G11B10_T          = 19,
    CP_RGBAX8888_TILE_4X    = 20,
    CP_XARGB8888_TILE_4X    = 21,
    CP_Z16_4X               = 22,
    CP_Z24_4X               = 23,
    CP_Z32_4X               = 24,
    CP_RGBAX8888_TILE_2X    = 25,
    CP_XARGB8888_TILE_2X    = 26,
    CP_Z16_2X               = 27,
    CP_Z24_2X               = 28,
    CP_Z32_2X               = 29,
    CP_RGBAX8888_TILE_8X    = 30,
    CP_XARGB8888_TILE_8X    = 31,
    CP_Z16_8X               = 32,
    CP_Z24_8X               = 33,
    CP_Z32_8X               = 34,
    CP_BAYER                = 35,
    CP_R32G32B32A32_TILE    = 36,
    CP_RGBAX8888_TILE_16X   = 37,
    CP_XARGB8888_TILE_16X   = 38,
    CP_Z16_16X              = 39,
    CP_Z24_16X              = 40,
    CP_Z32_16X              = 41,
    CP_TOTAL_RANGE          = 42,
}COMPRESS_MXUFORMAT;

#define BIT_COUNT_TABLE \
{\
    8,      /*HSF_UNKNOWN                   = 0*/\
    256,    /*HSF_R64G64B64A64_FLOAT        = 1*/\
    192,    /*HSF_R64G64B64_FLOAT           = 2*/\
    128,    /*HSF_R32G32B32A32_TYPELESS     = 3*/\
    128,    /*HSF_R32G32B32A32_FLOAT        = 4*/\
    128,    /*HSF_R32G32B32A32_UINT         = 5*/\
    128,    /*HSF_R32G32B32A32_SINT         = 6*/\
    128,    /*HSF_R32G32B32A32_SNORM        = 7*/\
    128,    /*HSF_R32G32B32A32_SCALE        = 8*/\
    128,    /*HSF_R32G32B32A32_UNORM        = 9*/\
    128,    /*HSF_R32G32B32A32_USCALE       = 10*/\
    128,    /*HSF_R32G32B32A32_FIX          = 11*/\
    128,    /*HSF_R64G64_FLOAT              = 12*/\
    128,    /*HSF_R32G32B32_TYPELESS        = 13*/\
    128,    /*HSF_R32G32B32_FLOAT           = 14*/\
    128,    /*HSF_R32G32B32_UINT            = 15*/\
    128,    /*HSF_R32G32B32_SINT            = 16*/\
    128,    /*HSF_R32G32B32_UNORM           = 17*/\
    128,    /*HSF_R32G32B32_USCALE          = 18*/\
    128,    /*HSF_R32G32B32_SNORM           = 19*/\
    128,    /*HSF_R32G32B32_SCALE           = 20*/\
    128,     /*HSF_R32G32B32_FIX             = 21*/\
    64,     /*HSF_R16G16B16A16_TYPELESS     = 22*/\
    64,     /*HSF_R16G16B16A16_FLOAT        = 23*/\
    64,     /*HSF_R16G16B16A16_UNORM        = 24*/\
    64,     /*HSF_R16G16B16A16_UINT         = 25*/\
    64,     /*HSF_R16G16B16A16_SNORM        = 26*/\
    64,     /*HSF_R16G16B16A16_SINT         = 27*/\
    64,     /*HSF_R16G16B16A16_SCALE        = 28*/\
    64,     /*HSF_R16G16B16A16_USCALE       = 29*/\
    64,     /*HSF_R16G16B16A16_SSCALE       = 30*/\
    48,     /*HSF_R12G12B12A12_UNORM        = 31*/\
    48,     /*HSF_R16G16B16_FLOAT           = 32*/\
    48,     /*HSF_R16G16B16_SINT            = 33*/\
    48,     /*HSF_R16G16B16_SNORM           = 34*/\
    48,     /*HSF_R16G16B16_SCALE           = 35*/\
    48,     /*HSF_R16G16B16_UINT            = 36*/\
    48,     /*HSF_R16G16B16_UNORM           = 37*/\
    48,     /*HSF_R16G16B16_USCALE          = 38*/\
    64,     /*HSF_R32G32_TYPELESS           = 39*/\
    64,     /*HSF_R32G32_FLOAT              = 40*/\
    64,     /*HSF_R32G32_UINT               = 41*/\
    64,     /*HSF_R32G32_SINT               = 42*/\
    64,     /*HSF_R32G32_SNORM              = 43*/\
    64,     /*HSF_R32G32_SCALE              = 44*/\
    64,     /*HSF_R32G32_UNORM              = 45*/\
    64,     /*HSF_R32G32_USCALE             = 46*/\
    64,     /*HSF_R32G32_FIX                = 47*/\
    64,     /*HSF_R32G8X24_TYPELESS         = 48*/\
    64,     /*HSF_D32_FLOAT_S8X24_UINT      = 49*/\
    64,     /*HSF_R32_FLOAT_X8X24_TYPELESS  = 50*/\
    64,     /*HSF_X32_TYPELESS_G8X24_UINT   = 51*/\
    64,     /*HSF_R64_FLOAT                 = 52*/\
    36,     /*HSF_R12G12B12_UNORM           = 53*/\
    32,     /*HSF_R10G10B10A2_TYPELESS      = 54*/\
    32,     /*HSF_R10G10B10A2_UNORM         = 55*/\
    32,     /*HSF_R10G10B10A2_UINT          = 56*/\
    32,     /*HSF_R10G10B10X2_UINT          = 57*/\
    32,     /*HSF_R10G10B10X2_USCALE        = 58*/\
    32,     /*HSF_R10G10B10A2_SNORM         = 59*/\
    32,     /*HSF_R10G10B10A2_SINT          = 60*/\
    32,     /*HSF_R10G10B10X2_SNORM         = 61*/\
    32,     /*HSF_B10G10R10A2_SNORM         = 62*/\
    32,     /*HSF_B10G10R10A2_UNORM         = 63*/\
    32,     /*HSF_B10G10R10X2_UNORM         = 64*/\
    32,     /*HSF_A2B10G10R10_UNORM         = 65*/\
    32,     /*HSF_A2B10G10R10_SNORM         = 66*/\
    32,     /*HSF_A2B10G10R10_USCALE        = 67*/\
    32,     /*HSF_A2B10G10R10_SSCALE        = 68*/\
    30,     /*HSF_R10G10B10_SINT            = 69*/\
    30,     /*HSF_R10G10B10_SNORM           = 70*/\
    30,     /*HSF_R10G10B10_SCALE           = 71*/\
    30,     /*HSF_R10G10B10_UNIT            = 72*/\
    30,     /*HSF_R10G10B10_UNORM           = 73*/\
    30,     /*HSF_R10G10B10_USCALE          = 74*/\
    20,     /*HSF_R10G10_SINT               = 75*/\
    20,     /*HSF_R10G10_SNORM              = 76*/\
    20,     /*HSF_R10G10_SCALE              = 77*/\
    20,     /*HSF_R10G10_UINT               = 78*/\
    20,     /*HSF_R10G10_UNORM              = 79*/\
    20,     /*HSF_R10G10_USCALE             = 80*/\
    32,     /*HSF_R10G10B10_FLOAT_A2_UNORM  = 81*/\
    32,     /*HSF_R11G11B10_FLOAT           = 82*/\
    32,     /*HSF_B10G11R11_FLOAT           = 83*/\
    22,     /*HSF_R11G11_FLOAT              = 84*/\
    32,     /*HSF_R8G8B8A8_TYPELESS         = 85*/\
    32,     /*HSF_R8G8B8A8_UNORM            = 86*/\
    32,     /*HSF_R8G8B8A8_UNORM_SRGB       = 87*/\
    32,     /*HSF_R8G8B8X8_UNORM            = 88*/\
    32,     /*HSF_B8G8R8A8_UNORM            = 89*/\
    32,     /*HSF_B8G8R8A8_UNORM_SRGB       = 90*/\
    32,     /*HSF_B8G8R8X8_UNORM            = 91*/\
    32,     /*HSF_B8G8R8X8_UNORM_SRGB       = 92*/\
    32,     /*HSF_X8R8G8B8_UNORM            = 93*/\
    32,     /*HSF_X8B8G8R8_UNORM            = 94*/\
    32,     /*HSF_A8R8G8B8_UNORM            = 95*/\
    32,     /*HSF_A8B8G8R8_UNORM            = 96*/\
    32,     /*HSF_R8G8B8A8_UINT             = 97*/\
    32,     /*HSF_R8G8B8A8_SNORM            = 98*/\
    32,     /*HSF_R8G8B8A8_SINT             = 99*/\
    32,     /*HSF_B8G8R8A8_XNORM            = 100*/\
    32,     /*HSF_R8G8B8A8_SSCALE           = 101*/\
    32,     /*HSF_R8G8B8A8_USCALE           = 102*/\
    24,     /*HSF_R8G8B8_SINT               = 103*/\
    24,     /*HSF_R8G8B8_SNORM              = 104*/\
    24,     /*HSF_R8G8B8_SCALE              = 105*/\
    24,     /*HSF_R8G8B8_UINT               = 106*/\
    24,     /*HSF_R8G8B8_UNORM              = 107*/\
    24,     /*HSF_R8G8B8_USCALE             = 108*/\
    32,     /*HSF_R16G16_TYPELESS           = 109*/\
    32,     /*HSF_R16G16_FLOAT              = 110*/\
    32,     /*HSF_R16G16_UNORM              = 111*/\
    32,     /*HSF_R16G16_UINT               = 112*/\
    32,     /*HSF_R16G16_SNORM              = 113*/\
    32,     /*HSF_R16G16_SINT               = 114*/\
    32,     /*HSF_R16G16_SCALE              = 115*/\
    32,     /*HSF_R16G16_USCALE             = 116*/\
    32,     /*HSF_R16G16_SSCALE             = 117*/\
    32,     /*HSF_R32_TYPELESS              = 118*/\
    32,     /*HSF_D32_FLOAT                 = 119*/\
    32,     /*HSF_D32_UNORM                 = 120*/\
    32,     /*HSF_R32_FLOAT                 = 121*/\
    32,     /*HSF_R32_UINT                  = 122*/\
    32,     /*HSF_R32_SINT                  = 123*/\
    32,     /*HSF_R32_UNORM                 = 124*/\
    32,     /*HSF_R32_USCALE                = 125*/\
    32,     /*HSF_R32_SNORM                 = 126*/\
    32,     /*HSF_R32_SCALE                 = 127*/\
    32,     /*HSF_R32_FIX                   = 128*/\
    32,     /*HSF_R24G8_TYPELESS            = 129*/\
    40,     /*HSF_D32_FLOAT_S8_UINT         = 130*/\
    32,     /*HSF_D24_UNORM_S8_UINT         = 131*/\
    32,     /*HSF_R24_UNORM_X8_TYPELESS     = 132*/\
    32,     /*HSF_X24_TYPELESS_G8_UINT      = 133*/\
    32,     /*HSF_D24_UNORM                 = 134*/\
    32,     /*HSF_R24_FLOAT                 = 135*/\
    16,     /*HSF_R8G8_TYPELESS             = 136*/\
    16,     /*HSF_R8G8_UNORM                = 137*/\
    16,     /*HSF_R8G8_UINT                 = 138*/\
    16,     /*HSF_R8G8_SNORM                = 139*/\
    16,     /*HSF_R8G8_SINT                 = 140*/\
    16,     /*HSF_R8G8_USCALE               = 141*/\
    16,     /*HSF_R8G8_SSCALE               = 142*/\
    16,     /*HSF_R16_TYPELESS              = 143*/\
    16,     /*HSF_R16_FLOAT                 = 144*/\
    16,     /*HSF_D16_UNORM                 = 145*/\
    16,     /*HSF_R16_UNORM                 = 146*/\
    16,     /*HSF_R16_UINT                  = 147*/\
    16,     /*HSF_R16_SNORM                 = 148*/\
    16,     /*HSF_R16_SINT                  = 149*/\
    16,     /*HSF_R16_USCALE                = 150*/\
    16,     /*HSF_R16_SSCALE                = 151*/\
    16,     /*HSF_A16_UNORM                 = 152*/\
    16,     /*HSF_B5G6R5_UNORM              = 153*/\
    16,     /*HSF_R5G6B5_UNORM              = 154*/\
    16,     /*HSF_B5G5R5X1_UNORM            = 155*/\
    16,     /*HSF_B5G5R5A1_UNORM            = 156*/\
    16,     /*HSF_R5G5B5A1_UNORM            = 157*/\
    16,     /*HSF_A1B5G5R5_UNORM            = 158*/\
    16,     /*HSF_A1R5G5B5_UNORM            = 159*/\
    16,     /*HSF_B4G4R4A4_UNORM            = 160*/\
    16,     /*HSF_B4G4R4X4_UNORM            = 161*/\
    16,     /*HSF_R4G4B4A4_UNORM            = 162*/\
    16,     /*HSF_A4B4G4R4_UNORM            = 163*/\
    16,     /*HSF_A4R4G4B4_UNORM            = 164*/\
    12,     /*HSF_R4G4B4_UNORM              = 165*/\
    15,     /*HSF_R5G5B5_UNORM              = 166*/\
    16,     /*HSF_U5V5L6_XNORM              = 167*/\
    16,     /*HSF_L16_UNORM                 = 168*/\
    16,     /*HSF_L8_A8_UNORM               = 169*/\
    10,     /*HSF_R10_SINT                  = 170*/\
    10,     /*HSF_R10_SNORM                 = 171*/\
    10,     /*HSF_R10_SCALE                 = 172*/\
    10,     /*HSF_R10_UINT                  = 173*/\
    10,     /*HSF_R10_UNORM                 = 174*/\
    10,     /*HSF_R10_USCALE                = 175*/\
    8,      /*HSF_R2G2B2A2_UNORM            = 176*/\
    8,      /*HSF_R8_TYPELESS               = 177*/\
    8,      /*HSF_R8_UNORM                  = 178*/\
    8,      /*HSF_R8_UINT                   = 179*/\
    8,      /*HSF_R8_SNORM                  = 180*/\
    8,      /*HSF_R8_SINT                   = 181*/\
    8,      /*HSF_R8_USCALE                 = 182*/\
    8,      /*HSF_R8_SSCALE                 = 183*/\
    8,      /*HSF_A8_UNORM                  = 184*/\
    8,      /*HSF_L8_UNORM                  = 185*/\
    12,      /*HSF_A12_UNORM                 = 186*/\
    4,      /*HSF_A4_UNORM                  = 187*/\
    1,      /*HSF_R1_UNORM                  = 188*/\
    8,      /*HSF_R3G3B2_UNORM              = 189*/\
    8,      /*HSF_L4A4_UNORM                = 190*/\
    8,      /*HSF_A4L4_UNORM                = 191*/\
    8,      /*HSF_L4A4_VIDEO                = 192*/\
    32,     /*HSF_AYUV_VIDEO                = 193*/\
    16,     /*HSF_YUYV                      = 194*/\
    8,      /*HSF_NV12                      = 195*/\
    16,      /*HSF_P010                      = 196*/\
    16,      /*HSF_P016                      = 197*/\
    32,     /*HSF_R9G9B9E5_SHAREDEXP        = 198*/\
    16,     /*HSF_R8G8_B8G8_UNORM/ treate as UYVY           = 199*/\
    16,     /*HSF_G8R8_G8B8_UNORM/ treate as YUYV           = 200*/\
    64,     /*HSF_BC1_UNORM                 = 201*/\
    64,     /*HSF_BC1_UNORM_SRGB            = 202*/\
    128,    /*HSF_BC2_UNORM                 = 203*/\
    128,    /*HSF_BC2_UNORM_SRGB            = 204*/\
    128,    /*HSF_BC3_UNORM                 = 205*/\
    128,    /*HSF_BC3_UNORM_SRGB            = 206*/\
    64,     /*HSF_BC4_UNORM                 = 207*/\
    64,     /*HSF_BC4_SNORM                 = 208*/\
    64,     /*HSF_BC4_UNORM_L               = 209*/\
    64,     /*HSF_BC4_SNORM_L               = 210*/\
    128,    /*HSF_BC5_UNORM                 = 211*/\
    128,    /*HSF_BC5_SNORM                 = 212*/\
    128,    /*HSF_BC5_UNORM_LA              = 213*/\
    128,    /*HSF_BC5_SNORM_LA              = 214*/\
    128,    /*HSF_BC6H_UF16                 = 215*/\
    128,    /*HSF_BC6H_SF16                 = 216*/\
    128,    /*HSF_BC7_UNORM                 = 217*/\
    128,    /*HSF_BC7_UNORM_SRGB            = 218*/\
    8,      /*HSF_G8_UINT                   = 219*/\
    128,    /*HSF_3DC_UNORM                 = 220*/\
    16,      /*HSF_YUY2                      = 221*/\
    32,     /*HSF_Y216_VIDEO                = 222*/\
    32,     /*HSF_Y210_VIDEO                = 223*/\
    8,      /*HSF_A4I4_VIDEO                = 224*/\
    16,     /*HSF_A8L8_UNORM                = 225*/\
    16,     /*HSF_L8A8_UNORM                = 226*/\
    0,      /*HSF_ZL1                       = 227*/\
    64,     /*HSF_R16G16B16A16_UNORM_SRGB   = 228*/\
    8,      /*HSF_YCRCB_MB_8_422            = 229*/\
    8,      /*HSF_YCRCB_MB_8_420            = 230*/\
    0,      /*HSF_YCRCB_MB_16_422           = 231*/\
    16,     /*HSF_P016T                     = 232*/\
    32,     /*HSF_UYVA1010102_VIDEO         = 233*/\
    16,     /*HSF_UYVY                      = 234*/\
    16,     /*HSF_BAYER_12                  = 235*/\
    16,     /*HSF_BAYER                     = 236*/\
    16,     /*HSF_P016L                     = 237*/\
    8,      /*HSF_YV12                      = 238*/\
    16,     /*HSF_L8A8_SNORM                = 239*/\
    1,      /*HSF_S1_UINT                   = 240*/\
    16,      /*HSF_S16_UINT                  = 241*/\
    4,      /*HSF_S4_UINT                   = 242*/\
    8,      /*HSF_S8_UINT                   = 243*/\
    0,      /*HSF_I12_UNORM                 = 244*/\
    16,      /*HSF_I16_UNORM                 = 245*/\
    0,      /*HSF_I4_UNORM                  = 246*/\
    8,      /*HSF_I8_UNORM                  = 247*/\
    0,      /*HSF_L12_UNORM                 = 248*/\
    0,      /*HSF_L12_A12_UNORM             = 249*/\
    0,      /*HSF_L12_A4_UNORM              = 250*/\
    32,      /*HSF_L16_A16_UNORM             = 251*/\
    0,      /*HSF_L4_UNORM                  = 252*/\
    0,      /*HSF_L4_A4_UNORM               = 253*/\
    0,      /*HSF_L6_A2_UNORM               = 254*/\
    64,     /*HSF_RGB8_ETC2                           = 255*/\
    64,     /*HSF_SRGB8_ETC2                           = 256*/\
    128,    /*HSF_RGBA8_ETC2_EAC                       = 257*/\
    128,    /*HSF_SRGB8_ALPHA8_ETC2_EAC                = 258*/\
    64,     /*HSF_R11_EAC                              = 259*/\
    128,    /*HSF_RG11_EAC                             = 260*/\
    64,     /*HSF_SIGNED_R11_EAC                       = 261*/\
    128,    /*HSF_SIGNED_RG11_EAC                      = 262*/\
    64,     /*HSF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2        = 263*/\
    64,     /*HSF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2       = 264*/\
    0,      /*HSF_COMPRESSED_ALPHA                     = 265*/\
    0,      /*HSF_COMPRESSED_INTENSITY                 = 266*/\
    0,      /*HSF_COMPRESSED_LUMINANCE                 = 267*/\
    0,      /*HSF_COMPRESSED_RED                       = 268*/\
    64,      /*HSF_COMPRESSED_RED_RGTC1                 = 269*/\
    0,      /*HSF_COMPRESSED_RG                        = 270*/\
    128,      /*HSF_COMPRESSED_RG_RGTC2                  = 271*/\
    0,      /*HSF_COMPRESSED_RGB                       = 272*/\
    128,      /*HSF_COMPRESSED_RGB_BPTC_SIGNED_FLOAT     = 273*/\
    128,      /*HSF_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT   = 274*/\
    0,      /*HSF_COMPRESSED_RGBA                      = 275*/\
    128,      /*HSF_COMPRESSED_RGBA_BPTC_UNORM           = 276*/\
    64,      /*HSF_COMPRESSED_SIGNED_RED_RGTC1          = 277*/\
    128,      /*HSF_COMPRESSED_SIGNED_RG_RGTC2           = 27*/\
    0,      /*HSF_COMPRESSED_SLUMINANCE                = 279*/\
    0,      /*HSF_COMPRESSED_SLUMINANCE_ALPHA          = 280*/\
    0,      /*HSF_COMPRESSED_SRGB                      = 281*/\
    0,      /*HSF_COMPRESSED_SRGB_ALPHA                = 282*/\
    128,      /*HSF_COMPRESSED_SRGB_ALPHA_BPTC_UNORM     = 283*/\
    32,     /*HSF_R10G10B10A2_USCALE                   = 284*/\
    32,     /*HSF_R10G10B10A2_SSCALE                   = 285*/\
    128,    /*HSF_R32G32B32A32_SSCALE                  = 286*/\
    128,     /*HSF_R32G32B32_SSCALE                     = 287*/\
    64,     /*HSF_R32G32_SSCALE                        = 288*/\
    48,     /*HSF_R16G16B16_SSCALE                     = 289*/\
    32,     /*HSF_R32_SSCALE                           = 290*/\
    24,     /*HSF_R8G8B8_SSCALE                        = 291*/\
    0,      /*HSF_UNKOWN                               = 292*/\
    0,      /*HSF_UNKOWN                               = 293*/\
    0,      /*HSF_UNKOWN                               = 294*/\
    0,      /*HSF_UNKOWN                               = 295*/\
    0,      /*HSF_UNKOWN                               = 296*/\
    0,      /*HSF_UNKOWN                               = 297*/\
    0,      /*HSF_UNKOWN                               = 298*/\
    0,      /*HSF_UNKOWN                               = 299*/\
    0,      /*HSF_UNKOWN                               = 300*/\
    0,      /*HSF_UNKOWN                               = 301*/\
    0,      /*HSF_UNKOWN                               = 302*/\
    0,      /*HSF_UNKOWN                               = 303*/\
    0,      /*HSF_UNKOWN                               = 304*/\
    0,      /*HSF_UNKOWN                               = 305*/\
    0,      /*HSF_UNKOWN                               = 306*/\
    0,      /*HSF_UNKOWN                               = 307*/\
    0,      /*HSF_UNKOWN                               = 308*/\
    0,      /*HSF_UNKOWN                               = 309*/\
    0,      /*HSF_UNKOWN                               = 310*/\
    0,      /*HSF_UNKOWN                               = 311*/\
    0,      /*HSF_UNKOWN                               = 312*/\
    0,      /*HSF_UNKOWN                               = 313*/\
    0,      /*HSF_UNKOWN                               = 314*/\
    0,      /*HSF_UNKOWN                               = 315*/\
    0,      /*HSF_UNKOWN                               = 316*/\
    0,      /*HSF_UNKOWN                               = 317*/\
    0,      /*HSF_UNKOWN                               = 318*/\
    0,      /*HSF_UNKOWN                               = 319*/\
    0,      /*HSF_UNKOWN                               = 320*/\
    128,    /*HSF_RGBA_ASTC_4X4                        = 321*/\
    128,    /*HSF_RGBA_ASTC_5X4                        = 322*/\
    128,    /*HSF_RGBA_ASTC_5X5                        = 323*/\
    128,    /*HSF_RGBA_ASTC_6X5                        = 324*/\
    128,    /*HSF_RGBA_ASTC_6X6                        = 325*/\
    128,    /*HSF_RGBA_ASTC_8X5                        = 326*/\
    128,    /*HSF_RGBA_ASTC_8X6                        = 327*/\
    128,    /*HSF_RGBA_ASTC_8X8                        = 328*/\
    128,    /*HSF_RGBA_ASTC_10X5                       = 329*/\
    128,    /*HSF_RGBA_ASTC_10X6                       = 330*/\
    128,    /*HSF_RGBA_ASTC_10X8                       = 331*/\
    128,    /*HSF_RGBA_ASTC_10X10                      = 332*/\
    128,    /*HSF_RGBA_ASTC_12X10                      = 333*/\
    128,    /*HSF_RGBA_ASTC_12X12                      = 334*/\
    0,      /*HSF_RESERVED0                            = 335*/\
    0,      /*HSF_RESERVED1                            = 336*/\
    128,    /*HSF_RGBA_ASTC_4X4_SRGB                   = 337*/\
    128,    /*HSF_RGBA_ASTC_5X4_SRGB                   = 338*/\
    128,    /*HSF_RGBA_ASTC_5X5_SRGB                   = 339*/\
    128,    /*HSF_RGBA_ASTC_6X5_SRGB                   = 340*/\
    128,    /*HSF_RGBA_ASTC_6X6_SRGB                   = 341*/\
    128,    /*HSF_RGBA_ASTC_8X5_SRGB                   = 342*/\
    128,    /*HSF_RGBA_ASTC_8X6_SRGB                   = 343*/\
    128,    /*HSF_RGBA_ASTC_8X8_SRGB                   = 344*/\
    128,    /*HSF_RGBA_ASTC_10X5_SRGB                  = 345*/\
    128,    /*HSF_RGBA_ASTC_10X6_SRGB                  = 346*/\
    128,    /*HSF_RGBA_ASTC_10X8_SRGB                  = 347*/\
    128,    /*HSF_RGBA_ASTC_10X10_SRGB                 = 348*/\
    128,    /*HSF_RGBA_ASTC_12X10_SRGB                 = 349*/\
    128,    /*HSF_RGBA_ASTC_12X12_SRGB                 = 350*/\
    0,      /*HSF_RESERVED2                            = 351*/\
    0,      /*HSF_RESERVED3                            = 352*/\
    128,    /*HSF_RGBA_ASTC_3x3x3                      = 353,*/\
    128,    /*HSF_RGBA_ASTC_4x3x3                      = 354,*/\
    128,    /*HSF_RGBA_ASTC_4x4x3                      = 355,*/\
    128,    /*HSF_RGBA_ASTC_4x4x4                      = 356,*/\
    128,    /*HSF_RGBA_ASTC_5x4x4                      = 357,*/\
    128,    /*HSF_RGBA_ASTC_5x5x4                      = 358,*/\
    128,    /*HSF_RGBA_ASTC_5x5x5                      = 359,*/\
    128,    /*HSF_RGBA_ASTC_6x5x5                      = 360,*/\
    128,    /*HSF_RGBA_ASTC_6x6x5                      = 361,*/\
    128,    /*HSF_RGBA_ASTC_6x6x6                      = 362,*/\
    32,      /*HSF_L16A16_UNORM                            = 363,*/\
    32,      /*HSF_L16A16_SNORM                            = 364,*/\
    0,      /*HSF_ASTC_RESERVED6                            = 365,*/\
    0,      /*HSF_ASTC_RESERVED7                            = 366,*/\
    0,      /*HSF_ASTC_RESERVED8                            = 367,*/\
    0,      /*HSF_ASTC_RESERVED9                            = 368,*/\
    128,    /*HSF_RGBA_ASTC_3x3x3_SRGB                 = 369, */\
    128,    /*HSF_RGBA_ASTC_4x3x3_SRGB                 = 370, */\
    128,    /*HSF_RGBA_ASTC_4x4x3_SRGB                 = 371, */\
    128,    /*HSF_RGBA_ASTC_4x4x4_SRGB                 = 372, */\
    128,    /*HSF_RGBA_ASTC_5x4x4_SRGB                 = 373, */\
    128,    /*HSF_RGBA_ASTC_5x5x4_SRGB                 = 374,*/\
    128,    /*HSF_RGBA_ASTC_5x5x5_SRGB                 = 375,  */\
    128,    /*HSF_RGBA_ASTC_6x5x5_SRGB                 = 376, */\
    128,    /*HSF_RGBA_ASTC_6x6x5_SRGB                 = 377, */\
    128,    /*HSF_RGBA_ASTC_6x6x6_SRGB                 = 378,*/\
    64,     /*HSF_R64_UINT                             = 379,*/\
    128,    /*HSF_R64G64_UINT                          = 380,*/\
    192,    /*HSF_R64G64B64_UINT                       = 381,*/\
    256,    /*HSF_R64G64B64A64_UINT                    = 382,*/\
    64,     /*HSF_R64_SINT                             = 383,*/\
    128,    /*HSF_R64G64_SINT                          = 384,*/\
    192,    /*HSF_R64G64B64_SINT                       = 385,*/\
    256,    /*HSF_R64G64B64A64_SINT                    = 386,*/\
    16,     /*HSF_R4G4B4X4_UNORM                       = 387,*/\
    16,     /*HSF_R5G5B5X1_UNORM                       = 388,*/\
    32,     /*HSF_R8G8B8X8_SNORM                       = 389,*/\
    32,     /*HSF_R8G8B8X8_UINT                        = 390,*/\
    32,     /*HSF_R8G8B8X8_SINT                        = 391,*/\
    32,     /*HSF_R8G8B8X8_UNORM_SRGB                  = 392,*/\
    32,     /*HSF_R10G10B10X2_UNORM                    = 393,*/\
    64,     /*HSF_R16G16B16X16_UNORM                   = 394,*/\
    64,     /*HSF_R16G16B16X16_SNORM                   = 395,*/\
    64,     /*HSF_R16G16B16X16_FLOAT                   = 396,*/\
    64,     /*HSF_R16G16B16X16_UINT                    = 397,*/\
    64,     /*HSF_R16G16B16X16_SINT                    = 398,*/\
    128,    /*HSF_R32G32B32X32_FLOAT                   = 399,*/\
    128,    /*HSF_R32G32B32X32_UINT                    = 400,*/\
    128,    /*HSF_R32G32B32X32_SINT                    = 401,*/\
    32,     /*HSF_S8_UINT_D24_UNORM                    = 402,*/\
    32,     /*HSF_D24_UNORM_S8_UINT                    = 403,*/\
    64,     /*HSF_S8_UINT_D32_FLOAT                    = 404,*/\
    32,     /*HSF_R24_UNORM                            = 405,*/\
    64,     /*HSF_R24G24_UNORM                         = 406,*/\
    128,    /*HSF_R24G24B24A24_UNORM                   = 407,*/\
    64,     /*HSF_R10G10B10A10_UNORM                   = 408,*/\
    64,     /*HSF_BC1_TYPELESS                         = 409,*/\
    32,     /*HSF_B8G8R8A8_TYPELESS                    = 410,*/\
    32,     /*HSF_B8G8R8X8_TYPELESS                    = 411,*/\
    128,    /*HSF_BC2_TYPELESS                         = 412,*/\
    128,    /*HSF_BC3_TYPELESS                         = 413,*/\
    64,     /*HSF_BC4_TYPELESS                         = 414,*/\
    128,    /*HSF_BC5_TYPELESS                         = 415,*/\
    128,    /*HSF_BC6H_TYPELESS                        = 416,*/\
    }\




#define SURFACE_FORMAT_MODE \
   {\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_UNKNOWN                   = 0,   */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R64G64B64A64_FLOAT        = 1,   */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R64G64B64_FLOAT           = 2,   */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32A32_TYPELESS     = 3,   */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32A32_FLOAT        = 4,   */\
   PS_OUT_FMT_O0_DATA_FMT_UINT32, /* HSF_R32G32B32A32_UINT         = 5,   */\
   PS_OUT_FMT_O0_DATA_FMT_SINT32, /* HSF_R32G32B32A32_SINT         = 6,   */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32A32_SNORM        = 7,   */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32A32_SCALE        = 8,   */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32A32_UNORM        = 9,   */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32A32_USCALE       = 10,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32A32_FIX          = 11,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R64G64_FLOAT              = 12,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32_TYPELESS        = 13,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32_FLOAT           = 14,  */\
   PS_OUT_FMT_O0_DATA_FMT_UINT32, /* HSF_R32G32B32_UINT            = 15,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32_SINT            = 16,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32_UNORM           = 17,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32_USCALE          = 18,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32_SNORM           = 19,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32_SCALE           = 20,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32B32_FIX             = 21,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP16, /* HSF_R16G16B16A16_TYPELESS     = 22,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP16, /* HSF_R16G16B16A16_FLOAT        = 23,  */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM16, /* HSF_R16G16B16A16_UNORM        = 24,  */\
   PS_OUT_FMT_O0_DATA_FMT_UINT16, /* HSF_R16G16B16A16_UINT         = 25,  */\
   PS_OUT_FMT_O0_DATA_FMT_SNORM16, /* HSF_R16G16B16A16_SNORM        = 26,  */\
   PS_OUT_FMT_O0_DATA_FMT_SINT16, /* HSF_R16G16B16A16_SINT         = 27,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R16G16B16A16_SCALE        = 28,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R16G16B16A16_USCALE       = 29,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R16G16B16A16_SSCALE       = 30,  */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM16, /* HSF_R12G12B12A12_UNORM        = 31,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP16, /* HSF_R16G16B16_FLOAT           = 32,  */\
   PS_OUT_FMT_O0_DATA_FMT_SINT16, /* HSF_R16G16B16_SINT            = 33,  */\
   PS_OUT_FMT_O0_DATA_FMT_SNORM16, /* HSF_R16G16B16_SNORM           = 34,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R16G16B16_SCALE           = 35,  */\
   PS_OUT_FMT_O0_DATA_FMT_UINT16, /* HSF_R16G16B16_UINT            = 36,  */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM16, /* HSF_R16G16B16_UNORM           = 37,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R16G16B16_USCALE          = 38,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32_TYPELESS           = 39,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32_FLOAT              = 40,  */\
   PS_OUT_FMT_O0_DATA_FMT_UINT32, /* HSF_R32G32_UINT               = 41,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32_SINT               = 42,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32_SNORM              = 43,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32_SCALE              = 44,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32_UNORM              = 45,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32_USCALE             = 46,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G32_FIX                = 47,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /* HSF_R32G8X24_TYPELESS         = 48,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_D32_FLOAT_S8X24_UINT      = 49, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_R32_FLOAT_X8X24_TYPELESS  = 50, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_X32_TYPELESS_G8X24_UINT   = 51, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_R64_FLOAT                 = 52, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_R12G12B12_UNORM           = 53, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10B10A2_TYPELESS      = 54, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10B10A2_UNORM         = 55, */\
   PS_OUT_FMT_O0_DATA_FMT_UINT16, /*  HSF_R10G10B10A2_UINT          = 56, */\
   PS_OUT_FMT_O0_DATA_FMT_UINT16, /*  HSF_R10G10B10X2_UINT          = 57, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10B10X2_USCALE        = 58, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10B10A2_SNORM         = 59, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10B10A2_SINT          = 60, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10B10X2_SNORM         = 61, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_B10G10R10A2_SNORM         = 62, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_B10G10R10A2_UNORM         = 63, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_B10G10R10X2_UNORM         = 64, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_A2B10G10R10_UNORM         = 65, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_A2B10G10R10_SNORM         = 66, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_A2B10G10R10_USCALE        = 67, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_A2B10G10R10_SSCALE        = 68, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10B10_SINT            = 69, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10B10_SNORM           = 70, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_R10G10B10_SCALE           = 71, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10B10_UNIT            = 72, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10B10_UNORM           = 73, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_R10G10B10_USCALE          = 74, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10_SINT               = 75, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10_SNORM              = 76, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_R10G10_SCALE              = 77, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10_UINT               = 78, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10, /*  HSF_R10G10_UNORM              = 79, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_R10G10_USCALE             = 80, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_R10G10B10_FLOAT_A2_UNORM  = 81, */\
   PS_OUT_FMT_O0_DATA_FMT_FP16, /*  HSF_R11G11B10_FLOAT           = 82, */\
   PS_OUT_FMT_O0_DATA_FMT_FP32, /*  HSF_B10G11R11_FLOAT           = 83, */\
   PS_OUT_FMT_O0_DATA_FMT_FP16, /*  HSF_R11G11_FLOAT              = 84, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8, /*  HSF_R8G8B8A8_TYPELESS         = 85, */\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8, /*  HSF_R8G8B8A8_UNORM            = 86, */\
   PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_R8G8B8A8_UNORM_SRGB       = 87*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_R8G8B8X8_UNORM            = 88*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_B8G8R8A8_UNORM            = 89*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_B8G8R8A8_UNORM_SRGB       = 90*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_B8G8R8X8_UNORM            = 91*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_B8G8R8X8_UNORM_SRGB       = 92*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_X8R8G8B8_UNORM            = 93*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_X8B8G8R8_UNORM            = 94*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_A8R8G8B8_UNORM            = 95*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_A8B8G8R8_UNORM            = 96*/\
   PS_OUT_FMT_O0_DATA_FMT_UINT8,     /*HSF_R8G8B8A8_UINT             = 97*/\
   PS_OUT_FMT_O0_DATA_FMT_SNORM8,     /*HSF_R8G8B8A8_SNORM            = 98*/\
   PS_OUT_FMT_O0_DATA_FMT_SINT8,     /*HSF_R8G8B8A8_SINT             = 99*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_B8G8R8A8_XNORM          = 100?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R8G8B8A8_SSCALE           = 101?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R8G8B8A8_USCALE           = 102*/\
   PS_OUT_FMT_O0_DATA_FMT_SINT8,     /*HSF_R8G8B8_SINT               = 103*/\
   PS_OUT_FMT_O0_DATA_FMT_SNORM8,     /*HSF_R8G8B8_SNORM              = 104*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R8G8B8_SCALE              = 105?*/\
   PS_OUT_FMT_O0_DATA_FMT_UINT8,     /*HSF_R8G8B8_UINT               = 106*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_R8G8B8_UNORM              = 107*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R8G8B8_USCALE             = 108?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_R16G16_TYPELESS           = 109?0*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_R16G16_FLOAT              = 110*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM16,     /*HSF_R16G16_UNORM              = 111*/\
   PS_OUT_FMT_O0_DATA_FMT_UINT16,     /*HSF_R16G16_UINT               = 112*/\
   PS_OUT_FMT_O0_DATA_FMT_SNORM16,     /*HSF_R16G16_SNORM              = 113*/\
   PS_OUT_FMT_O0_DATA_FMT_SINT16,     /*HSF_R16G16_SINT               = 114*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R16G16_SCALE              = 115?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R16G16_USCALE             = 116?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R16G16_SSCALE             = 117?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32_TYPELESS              = 118?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_D32_FLOAT                 = 119*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_D32_UNORM                 = 120?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32_FLOAT                 = 121*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32_UINT                  = 122?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32_SINT                  = 123?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32_UNORM                 = 124?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32_USCALE                = 125*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32_SNORM                 = 126*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32_SCALE                 = 127*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32_FIX                   = 128*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R24G8_TYPELESS            = 129*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_D32_FLOAT_S8_UINT         = 130*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_D24_UNORM_S8_UINT         = 131*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R24_UNORM_X8_TYPELESS     = 132*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_X24_TYPELESS_G8_UINT      = 133*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM24,     /*HSF_D24_UNORM                 = 134*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R24_FLOAT                 = 135*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_R8G8_TYPELESS             = 136?0*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_R8G8_UNORM                = 137*/\
   PS_OUT_FMT_O0_DATA_FMT_UINT8,     /*HSF_R8G8_UINT                 = 138*/\
   PS_OUT_FMT_O0_DATA_FMT_SNORM8,     /*HSF_R8G8_SNORM                = 139*/\
   PS_OUT_FMT_O0_DATA_FMT_SINT8,     /*HSF_R8G8_SINT                 = 140*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R8G8_USCALE               = 141*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R8G8_SSCALE               = 142*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R16_TYPELESS              = 143*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_R16_FLOAT                 = 144*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM16,     /*HSF_D16_UNORM                 = 145*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM16,     /*HSF_R16_UNORM                 = 146*/\
   PS_OUT_FMT_O0_DATA_FMT_UINT16,     /*HSF_R16_UINT                  = 147*/\
   PS_OUT_FMT_O0_DATA_FMT_SNORM16,     /*HSF_R16_SNORM                 = 148*/\
   PS_OUT_FMT_O0_DATA_FMT_SINT16,     /*HSF_R16_SINT                  = 149*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R16_USCALE                = 150?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R16_SSCALE                = 151?*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM16,     /*HSF_A16_UNORM                 = 152*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_B5G6R5_UNORM              = 153*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_R5G6B5_UNORM              = 154*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_B5G5R5X1_UNORM            = 155*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_B5G5R5A1_UNORM            = 156*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_R5G5B5A1_UNORM            = 157*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_A1B5G5R5_UNORM            = 158*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_A1R5G5B5_UNORM            = 159*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_B4G4R4A4_UNORM            = 160*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_B4G4R4X4_UNORM            = 161*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_R4G4B4A4_UNORM            = 162*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_A4B4G4R4_UNORM            = 163*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_A4R4G4B4_UNORM            = 164*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_R4G4B4_UNORM              = 165*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_R5G5B5_UNORM              = 166*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_U5V5L6_XNORM              = 167?0*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM16,     /*HSF_L16_UNORM                 = 168*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_L8_A8_UNORM               = 169*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R10_SINT                  = 170?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R10_SNORM                 = 171?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R10_SCALE                 = 172?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R10_UINT                  = 173?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R10_UNORM                 = 174?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R10_USCALE                = 175?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_R2G2B2A2_UNORM            = 176?*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,      /*HSF_R8_TYPELESS               = 177?0*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,      /*HSF_R8_UNORM                  = 178*/\
   PS_OUT_FMT_O0_DATA_FMT_UINT8,      /*HSF_R8_UINT                   = 179*/\
   PS_OUT_FMT_O0_DATA_FMT_SNORM8,      /*HSF_R8_SNORM                  = 180*/\
   PS_OUT_FMT_O0_DATA_FMT_SINT8,      /*HSF_R8_SINT                   = 181*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_R8_USCALE                 = 182?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_R8_SSCALE                 = 183?*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,      /*HSF_A8_UNORM                  = 184*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,      /*HSF_L8_UNORM                  = 185*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_A12_UNORM                 = 186*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_A4_UNORM                  = 187*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_R1_UNORM                  = 188*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_R3G3B2_UNORM              = 189*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_L4A4_UNORM                = 190*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_A4L4_UNORM                = 191*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_L4A4_VIDEO                = 192*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10,     /*HSF_AYUV_VIDEO                = 193*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_YUYV                      = 194*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_NV12                      = 195?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_P010                      = 196?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_P016                      = 197?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R9G9B9E5_SHAREDEXP        = 198?UINT16*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R8G8_B8G8_UNORM           = 199*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_G8R8_G8B8_UNORM           = 200*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_BC1_UNORM                 = 201*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_BC1_UNORM_SRGB            = 202*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_BC2_UNORM                 = 203*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_BC2_UNORM_SRGB            = 204*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_BC3_UNORM                 = 205*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_BC3_UNORM_SRGB            = 206*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_BC4_UNORM                 = 207*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_BC4_SNORM                 = 208*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_BC4_UNORM_L               = 209*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_BC4_SNORM_L               = 210*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_BC5_UNORM                 = 211*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_BC5_SNORM                 = 212*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_BC5_UNORM_LA              = 213*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_BC5_SNORM_LA              = 214*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_BC6H_UF16                 = 215*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_BC6H_SF16                 = 216*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_BC7_UNORM                 = 217*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_BC7_UNORM_SRGB            = 218*/\
   PS_OUT_FMT_O0_DATA_FMT_UINT8,      /*HSF_G8_UINT                   = 219?0?UNORM8*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_3DC_UNORM                 = 220*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_YUY2                      = 221*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_Y216_VIDEO                = 222*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_Y210_VIDEO                = 223*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,      /*HSF_A4I4_VIDEO                = 224?0*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_A8L8_UNORM                = 225*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_L8A8_UNORM                = 226*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_ZL1                       = 227*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_R16G16B16A16_UNORM_SRGB   = 228*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_YCRCB_MB_8_422            = 229*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_YCRCB_MB_8_420            = 230*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_YCRCB_MB_16_422           = 231*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_P016T                     = 232?*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM10,     /*HSF_UYVA1010102_VIDEO         = 233?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_UYVY                      = 234?PS_OUT_FMT_O0_DATA_FMT_UNORM10*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM16,     /*HSF_BAYER_12                  = 235?0*/\
   PS_OUT_FMT_O0_DATA_FMT_UNORM16,     /*HSF_BAYER                     = 236?0*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_P016L                     = 237?*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_YV12                      = 238*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_NV12T                     = 239*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_S1_UINT                   = 240*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_S16_UINT                  = 241*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_S4_UINT                   = 242*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_S8_UINT                   = 243*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_I12_UNORM                 = 244*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_I16_UNORM                 = 245*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_I4_UNORM                  = 246*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_I8_UNORM                  = 247*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_L12_UNORM                 = 248*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_L12_A12_UNORM             = 249*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_L12_A4_UNORM              = 250*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_L16_A16_UNORM             = 251*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_L4_UNORM                  = 252*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_L4_A4_UNORM               = 253*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_L6_A2_UNORM               = 254*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_RGB8_ETC2                  = 255?0*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_SRGB8_ETC2                 = 256?0*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA8_ETC2_EAC                       = 257*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_SRGB8_ALPHA8_ETC2_EAC                = 258*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_R11_EAC                              = 259?0*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RG11_EAC                             = 260*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_SIGNED_R11_EAC                       = 261*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_SIGNED_RG11_EAC                      = 262*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2        = 263*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2       = 264*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_ALPHA                     = 265*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_INTENSITY                 = 266*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_LUMINANCE                 = 267*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_RED                       = 268*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_RED_RGTC1                 = 269*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_RG                        = 270*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_RG_RGTC2                  = 271*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_RGB                       = 272*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_RGB_BPTC_SIGNED_FLOAT     = 273*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT   = 274*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_RGBA                      = 275*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_RGBA_BPTC_UNORM           = 276*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_SIGNED_RED_RGTC1          = 277*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_COMPRESSED_SIGNED_RG_RGTC2           = 27*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_COMPRESSED_SLUMINANCE                = 279*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_COMPRESSED_SLUMINANCE_ALPHA          = 280*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_COMPRESSED_SRGB                      = 281*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_COMPRESSED_SRGB_ALPHA                = 282*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_COMPRESSED_SRGB_ALPHA_BPTC_UNORM     = 283*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R10G10B10A2_USCALE                   = 284?0*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R10G10B10A2_SSCALE                   = 285*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32G32B32A32_SSCALE                  = 286,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32G32B32_SSCALE                     = 287,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32G32_SSCALE                        = 288,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R16G16B16_SSCALE                     = 289,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32_SSCALE                           = 290,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R8G8B8_SSCALE                        = 291,*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED, /*HSF_UNKOWN                               = 292*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 293*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 294*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 295*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 296*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 297*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 298*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 299*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 300*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 301*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 302*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 303*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 304*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 305*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 306*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 307*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 308*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 309*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 310*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 311*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 312*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 313*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 314*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 315*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 316*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 317*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 318*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 319*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 320*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_4X4                        = 321*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_5X4                        = 322*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_5X5                        = 323*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_6X5                        = 324*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_6X6                        = 325*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_8X5                        = 326*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_8X6                        = 327*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_8X8                        = 328*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_10X5                       = 329*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_10X6                       = 330*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_10X8                       = 331*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_10X10                      = 332*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_12X10                      = 333*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,    /*HSF_RGBA_ASTC_12X12                      = 334*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED, /*HSF_RESERVED0                            = 335*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED, /*HSF_RESERVED1                            = 336*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_4X4_SRGB                   = 337*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_5X4_SRGB                   = 338*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_5X5_SRGB                   = 339*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_6X5_SRGB                   = 340*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_6X6_SRGB                   = 341*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_8X5_SRGB                   = 342*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_8X6_SRGB                   = 343*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_8X8_SRGB                   = 344*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_10X5_SRGB                  = 345*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_10X6_SRGB                  = 346*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_10X8_SRGB                  = 347*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_10X10_SRGB                 = 348*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_12X10_SRGB                 = 349*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,    /*HSF_RGBA_ASTC_12X12_SRGB                 = 350*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,  /*HSF_RESERVED2                            = 351*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,  /*HSF_RESERVED3                            = 352*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_RGBA_ASTC_3x3x3                      = 353,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_RGBA_ASTC_4x3x3                      = 354,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_RGBA_ASTC_4x4x3                      = 355,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_RGBA_ASTC_4x4x4                      = 356,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_RGBA_ASTC_5x4x4                      = 357,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_RGBA_ASTC_5x5x4                      = 358,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_RGBA_ASTC_5x5x5                      = 359,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_RGBA_ASTC_6x5x5                      = 360,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_RGBA_ASTC_6x6x5                      = 361,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP32,      /*HSF_RGBA_ASTC_6x6x6                      = 362,*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,  /*HSF_ASTC_RESERVED4                            = 363,*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,  /*HSF_ASTC_RESERVED5                            = 364,*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,  /*HSF_ASTC_RESERVED6                            = 365,*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,  /*HSF_ASTC_RESERVED7                            = 366,*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,  /*HSF_ASTC_RESERVED8                            = 367,*/\
   PS_OUT_FMT_O0_DATA_FMT_RESERVED,  /*HSF_ASTC_RESERVED9                            = 368,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_RGBA_ASTC_3x3x3_SRGB                 = 369, */\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_RGBA_ASTC_4x3x3_SRGB                 = 370, */\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_RGBA_ASTC_4x4x3_SRGB                 = 371, */\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_RGBA_ASTC_4x4x4_SRGB                 = 372, */\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_RGBA_ASTC_5x4x4_SRGB                 = 373, */\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_RGBA_ASTC_5x5x4_SRGB                 = 374,*/\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_RGBA_ASTC_5x5x5_SRGB                 = 375,  */\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_RGBA_ASTC_6x5x5_SRGB                 = 376, */\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_RGBA_ASTC_6x6x5_SRGB                 = 377, */\
   PS_OUT_FMT_O0_DATA_FMT_FP16,      /*HSF_RGBA_ASTC_6x6x6_SRGB                 = 378,*/\
    PS_OUT_FMT_O0_DATA_FMT_UINT32,     /*HSF_R64_UINT                             = 379,*/\
    PS_OUT_FMT_O0_DATA_FMT_UINT32,     /*HSF_R64G64_UINT                          = 380,*/\
    PS_OUT_FMT_O0_DATA_FMT_UINT32,     /*HSF_R64G64B64_UINT                       = 381,*/\
    PS_OUT_FMT_O0_DATA_FMT_UINT32,     /*HSF_R64G64B64A64_UINT                    = 382,*/\
    PS_OUT_FMT_O0_DATA_FMT_UINT32,      /*HSF_R64_SINT                             = 383,*/\
    PS_OUT_FMT_O0_DATA_FMT_UINT32,     /*HSF_R64G64_SINT                          = 384,*/\
    PS_OUT_FMT_O0_DATA_FMT_UINT32,     /*HSF_R64G64B64_SINT                       = 385,*/\
    PS_OUT_FMT_O0_DATA_FMT_UINT32,     /*HSF_R64G64B64A64_SINT                    = 386,*/\
    PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_R4G4B4X4_UNORM                       = 387,*/\
    PS_OUT_FMT_O0_DATA_FMT_UNORM8,      /*HSF_R5G5B5X1_UNORM                       = 388,*/\
    PS_OUT_FMT_O0_DATA_FMT_SNORM8,     /*HSF_R8G8B8X8_SNORM                       = 389,*/\
    PS_OUT_FMT_O0_DATA_FMT_UINT8,     /*HSF_R8G8B8X8_UINT                        = 390,*/\
    PS_OUT_FMT_O0_DATA_FMT_SINT8,      /*HSF_R8G8B8X8_SINT                        = 391,*/\
    PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_R8G8B8X8_UNORM_SRGB                  = 392,*/\
    PS_OUT_FMT_O0_DATA_FMT_UNORM8,     /*HSF_R10G10B10X2_UNORM                    = 393,*/\
    PS_OUT_FMT_O0_DATA_FMT_UNORM16,     /*HSF_R16G16B16X16_UNORM                   = 394,*/\
    PS_OUT_FMT_O0_DATA_FMT_SNORM16,     /*HSF_R16G16B16X16_SNORM                   = 395,*/\
    PS_OUT_FMT_O0_DATA_FMT_FP16,     /*HSF_R16G16B16X16_FLOAT                   = 396,*/\
    PS_OUT_FMT_O0_DATA_FMT_UINT16,     /*HSF_R16G16B16X16_UINT                    = 397,*/\
    PS_OUT_FMT_O0_DATA_FMT_SINT16,     /*HSF_R16G16B16X16_SINT                    = 398,*/\
    PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_R32G32B32X32_FLOAT                   = 399,*/\
    PS_OUT_FMT_O0_DATA_FMT_UINT32,     /*HSF_R32G32B32X32_UINT                    = 400,*/\
    PS_OUT_FMT_O0_DATA_FMT_SINT32,     /*HSF_R32G32B32X32_SINT                    = 401,*/\
    PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_S8_UINT_D24_UNORM                    = 402,*/\
    PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_D24_UNORM_S8_UINT                    = 403,*/\
    PS_OUT_FMT_O0_DATA_FMT_FP32,     /*HSF_S8_UINT_D32_FLOAT                    = 404,*/\
    PS_OUT_FMT_O0_DATA_FMT_UNORM24,      /*HSF_R24_UNORM                            = 405,*/\
    PS_OUT_FMT_O0_DATA_FMT_UNORM24,      /*HSF_R24G24_UNORM                         = 406,*/\
    PS_OUT_FMT_O0_DATA_FMT_UNORM24,      /*HSF_R24G24B24A24_UNORM                   = 407,*/\
    PS_OUT_FMT_O0_DATA_FMT_UNORM10,      /*HSF_R10G10B10A10_UNORM                   = 408,*/\
    PS_OUT_FMT_O0_DATA_FMT_FP16,         /*HSF_BC1_TYPELESS                         = 409,*/\
    PS_OUT_FMT_O0_DATA_FMT_UNORM8,        /*HSF_B8G8R8A8_TYPELESS                   = 410,*/\
    PS_OUT_FMT_O0_DATA_FMT_UNORM8,        /*HSF_B8G8R8X8_TYPELESS                   = 411,*/\
    PS_OUT_FMT_O0_DATA_FMT_FP16,          /*HSF_BC2_TYPELESS                        = 412,*/\
    PS_OUT_FMT_O0_DATA_FMT_FP16,          /*HSF_BC3_TYPELESS                        = 413,*/\
    PS_OUT_FMT_O0_DATA_FMT_FP16,          /*HSF_BC4_TYPELESS                        = 414,*/\
    PS_OUT_FMT_O0_DATA_FMT_FP32,          /*HSF_BC5_TYPELESS                        = 415,*/\
    PS_OUT_FMT_O0_DATA_FMT_FP32,          /*DXGI_FORMAT_BC6H_TYPELESS               = 416,*/\
   }


#define UAV_FORMAT \
{\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_UNKNOWN                   = 0,   */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R64G64B64A64_FLOAT        = 1,   */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R64G64B64_FLOAT           = 2,   */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32A32_TYPELESS     = 3,   */\
    CS_U_FMT_U0_DATA_FMT_FP32, /* HSF_R32G32B32A32_FLOAT        = 4,   */\
    CS_U_FMT_U0_DATA_FMT_UINT32, /* HSF_R32G32B32A32_UINT         = 5,   */\
    CS_U_FMT_U0_DATA_FMT_SINT32, /* HSF_R32G32B32A32_SINT         = 6,   */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32A32_SNORM        = 7,   */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32A32_SCALE        = 8,   */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32A32_UNORM        = 9,   */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32A32_USCALE       = 10,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32A32_FIX          = 11,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R64G64_FLOAT              = 12,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32_TYPELESS        = 13,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32_FLOAT           = 14,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32_UINT            = 15,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32_SINT            = 16,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32_UNORM           = 17,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32_USCALE          = 18,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32_SNORM           = 19,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32_SCALE           = 20,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32B32_FIX             = 21,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R16G16B16A16_TYPELESS     = 22,  */\
    CS_U_FMT_U0_DATA_FMT_FP16, /* HSF_R16G16B16A16_FLOAT        = 23,  */\
    CS_U_FMT_U0_DATA_FMT_UNORM16, /* HSF_R16G16B16A16_UNORM        = 24,  */\
    CS_U_FMT_U0_DATA_FMT_UINT16, /* HSF_R16G16B16A16_UINT         = 25,  */\
    CS_U_FMT_U0_DATA_FMT_SNORM16, /* HSF_R16G16B16A16_SNORM        = 26,  */\
    CS_U_FMT_U0_DATA_FMT_SINT16, /* HSF_R16G16B16A16_SINT         = 27,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R16G16B16A16_SCALE        = 28,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R16G16B16A16_USCALE       = 29,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R16G16B16A16_SSCALE       = 30,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R12G12B12A12_UNORM        = 31,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R16G16B16_FLOAT           = 32,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R16G16B16_SINT            = 33,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R16G16B16_SNORM           = 34,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R16G16B16_SCALE           = 35,  */\
    CS_U_FMT_U0_DATA_FMT_UINT16, /* HSF_R16G16B16_UINT            = 36,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R16G16B16_UNORM           = 37,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R16G16B16_USCALE          = 38,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32_TYPELESS           = 39,  */\
    CS_U_FMT_U0_DATA_FMT_FP32, /* HSF_R32G32_FLOAT              = 40,  */\
    CS_U_FMT_U0_DATA_FMT_UINT32, /* HSF_R32G32_UINT               = 41,  */\
    CS_U_FMT_U0_DATA_FMT_SINT32, /* HSF_R32G32_SINT               = 42,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32_SNORM              = 43,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32_SCALE              = 44,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32_UNORM              = 45,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32_USCALE             = 46,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G32_FIX                = 47,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /* HSF_R32G8X24_TYPELESS         = 48,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_D32_FLOAT_S8X24_UINT      = 49, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R32_FLOAT_X8X24_TYPELESS  = 50, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_X32_TYPELESS_G8X24_UINT   = 51, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R64_FLOAT                 = 52, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R12G12B12_UNORM           = 53, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10A2_TYPELESS      = 54, */\
    CS_U_FMT_U0_DATA_FMT_UNORM10, /*  HSF_R10G10B10A2_UNORM         = 55, */\
    CS_U_FMT_U0_DATA_FMT_UINT16, /*  HSF_R10G10B10A2_UINT          = 56, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10X2_UINT          = 57, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10X2_USCALE        = 58, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10A2_SNORM         = 59, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10A2_SINT          = 60, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10X2_SNORM         = 61, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_B10G10R10A2_SNORM         = 62, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_B10G10R10A2_UNORM         = 63, */\
    CS_U_FMT_U0_DATA_FMT_UNORM10, /*  HSF_B10G10R10X2_UNORM         = 64, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_A2B10G10R10_UNORM         = 65, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_A2B10G10R10_SNORM         = 66, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_A2B10G10R10_USCALE        = 67, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_A2B10G10R10_SSCALE        = 68, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10_SINT            = 69, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10_SNORM           = 70, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10_SCALE           = 71, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10_UNIT            = 72, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10_UNORM           = 73, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10_USCALE          = 74, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10_SINT               = 75, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10_SNORM              = 76, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10_SCALE              = 77, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10_UINT               = 78, */\
    CS_U_FMT_U0_DATA_FMT_UNORM10, /*  HSF_R10G10_UNORM              = 79, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10_USCALE             = 80, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R10G10B10_FLOAT_A2_UNORM  = 81, */\
    CS_U_FMT_U0_DATA_FMT_FP16,      /*  HSF_R11G11B10_FLOAT           = 82, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_B10G11R11_FLOAT           = 83, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R11G11_FLOAT              = 84, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*  HSF_R8G8B8A8_TYPELESS         = 85, */\
    CS_U_FMT_U0_DATA_FMT_UNORM8, /*  HSF_R8G8B8A8_UNORM            = 86, */\
    CS_U_FMT_U0_DATA_FMT_UNORM8,     /*HSF_R8G8B8A8_UNORM_SRGB       = 87*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8X8_UNORM            = 88*/\
    CS_U_FMT_U0_DATA_FMT_UNORM8,     /*HSF_B8G8R8A8_UNORM            = 89*/\
    CS_U_FMT_U0_DATA_FMT_UNORM8,     /*HSF_B8G8R8A8_UNORM_SRGB       = 90*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_B8G8R8X8_UNORM            = 91*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_B8G8R8X8_UNORM_SRGB       = 92*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_X8R8G8B8_UNORM            = 93*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_X8B8G8R8_UNORM            = 94*/\
    CS_U_FMT_U0_DATA_FMT_UNORM8,     /*HSF_A8R8G8B8_UNORM            = 95*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_A8B8G8R8_UNORM            = 96*/\
    CS_U_FMT_U0_DATA_FMT_UINT8,     /*HSF_R8G8B8A8_UINT             = 97*/\
    CS_U_FMT_U0_DATA_FMT_SNORM8,     /*HSF_R8G8B8A8_SNORM            = 98*/\
    CS_U_FMT_U0_DATA_FMT_SINT8,     /*HSF_R8G8B8A8_SINT             = 99*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_B8G8R8A8_XNORM          = 100?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8A8_SSCALE           = 101?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8A8_USCALE           = 102*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8_SINT               = 103*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8_SNORM              = 104*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8_SCALE              = 105?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8_UINT               = 106*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8_UNORM              = 107*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8_USCALE             = 108?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16G16_TYPELESS           = 109?0*/\
    CS_U_FMT_U0_DATA_FMT_FP16,     /*HSF_R16G16_FLOAT              = 110*/\
    CS_U_FMT_U0_DATA_FMT_UNORM16,     /*HSF_R16G16_UNORM              = 111*/\
    CS_U_FMT_U0_DATA_FMT_UINT16,     /*HSF_R16G16_UINT               = 112*/\
    CS_U_FMT_U0_DATA_FMT_SNORM16,     /*HSF_R16G16_SNORM              = 113*/\
    CS_U_FMT_U0_DATA_FMT_SINT16,     /*HSF_R16G16_SINT               = 114*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16G16_SCALE              = 115?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16G16_USCALE             = 116?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16G16_SSCALE             = 117?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32_TYPELESS              = 118?*/\
    CS_U_FMT_U0_DATA_FMT_FP32,     /*HSF_D32_FLOAT                 = 119*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_D32_UNORM                 = 120?*/\
    CS_U_FMT_U0_DATA_FMT_FP32,     /*HSF_R32_FLOAT                 = 121*/\
    CS_U_FMT_U0_DATA_FMT_UINT32,     /*HSF_R32_UINT                  = 122?*/\
    CS_U_FMT_U0_DATA_FMT_SINT32,     /*HSF_R32_SINT                  = 123?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32_UNORM                 = 124?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32_USCALE                = 125*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32_SNORM                 = 126*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32_SCALE                 = 127*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32_FIX                   = 128*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R24G8_TYPELESS            = 129*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_D32_FLOAT_S8_UINT         = 130*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_D24_UNORM_S8_UINT         = 131*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R24_UNORM_X8_TYPELESS     = 132*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_X24_TYPELESS_G8_UINT      = 133*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_D24_UNORM                 = 134*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R24_FLOAT                 = 135*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8_TYPELESS             = 136?0*/\
    CS_U_FMT_U0_DATA_FMT_UNORM8,     /*HSF_R8G8_UNORM                = 137*/\
    CS_U_FMT_U0_DATA_FMT_UINT8,     /*HSF_R8G8_UINT                 = 138*/\
    CS_U_FMT_U0_DATA_FMT_SNORM8,     /*HSF_R8G8_SNORM                = 139*/\
    CS_U_FMT_U0_DATA_FMT_SINT8,     /*HSF_R8G8_SINT                 = 140*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8_USCALE               = 141*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8_SSCALE               = 142*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16_TYPELESS              = 143*/\
    CS_U_FMT_U0_DATA_FMT_FP16,     /*HSF_R16_FLOAT                 = 144*/\
    CS_U_FMT_U0_DATA_FMT_UNORM16,     /*HSF_D16_UNORM                 = 145*/\
    CS_U_FMT_U0_DATA_FMT_UNORM16,     /*HSF_R16_UNORM                 = 146*/\
    CS_U_FMT_U0_DATA_FMT_UINT16,     /*HSF_R16_UINT                  = 147*/\
    CS_U_FMT_U0_DATA_FMT_SNORM16,     /*HSF_R16_SNORM                 = 148*/\
    CS_U_FMT_U0_DATA_FMT_SINT16,     /*HSF_R16_SINT                  = 149*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16_USCALE                = 150?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16_SSCALE                = 151?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_A16_UNORM                 = 152*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_B5G6R5_UNORM              = 153*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R5G6B5_UNORM              = 154*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_B5G5R5X1_UNORM            = 155*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_B5G5R5A1_UNORM            = 156*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R5G5B5A1_UNORM            = 157*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_A1B5G5R5_UNORM            = 158*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_A1R5G5B5_UNORM            = 159*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_B4G4R4A4_UNORM            = 160*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_B4G4R4X4_UNORM            = 161*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R4G4B4A4_UNORM            = 162*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_A4B4G4R4_UNORM            = 163*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_A4R4G4B4_UNORM            = 164*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R4G4B4_UNORM              = 165*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R5G5B5_UNORM              = 166*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_U5V5L6_XNORM              = 167?0*/\
    CS_U_FMT_U0_DATA_FMT_UNORM16,     /*HSF_L16_UNORM                 = 168*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_L8_A8_UNORM               = 169*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R10_SINT                  = 170?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R10_SNORM                 = 171?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R10_SCALE                 = 172?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R10_UINT                  = 173?*/\
    CS_U_FMT_U0_DATA_FMT_UNORM10,     /*HSF_R10_UNORM                 = 174?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R10_USCALE                = 175?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_R2G2B2A2_UNORM            = 176?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_R8_TYPELESS               = 177?0*/\
    CS_U_FMT_U0_DATA_FMT_UNORM8,      /*HSF_R8_UNORM                  = 178*/\
    CS_U_FMT_U0_DATA_FMT_UINT8,      /*HSF_R8_UINT                   = 179*/\
    CS_U_FMT_U0_DATA_FMT_SNORM8,      /*HSF_R8_SNORM                  = 180*/\
    CS_U_FMT_U0_DATA_FMT_SINT8,      /*HSF_R8_SINT                   = 181*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_R8_USCALE                 = 182?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_R8_SSCALE                 = 183?*/\
    CS_U_FMT_U0_DATA_FMT_UNORM8,      /*HSF_A8_UNORM                  = 184*/\
    CS_U_FMT_U0_DATA_FMT_UNORM8,      /*HSF_L8_UNORM                  = 185*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_A12_UNORM                 = 186*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_A4_UNORM                  = 187*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_R1_UNORM                  = 188*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_R3G3B2_UNORM              = 189*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_L4A4_UNORM                = 190*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_A4L4_UNORM                = 191*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_L4A4_VIDEO                = 192*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_AYUV_VIDEO                = 193*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_YUYV                      = 194*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_NV12                      = 195?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_P010                      = 196?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_P016                      = 197?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R9G9B9E5_SHAREDEXP        = 198?UINT16*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8_B8G8_UNORM           = 199*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_G8R8_G8B8_UNORM           = 200*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_BC1_UNORM                 = 201*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_BC1_UNORM_SRGB            = 202*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_BC2_UNORM                 = 203*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_BC2_UNORM_SRGB            = 204*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_BC3_UNORM                 = 205*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_BC3_UNORM_SRGB            = 206*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC4_UNORM                 = 207*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC4_SNORM                 = 208*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC4_UNORM_L               = 209*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC4_SNORM_L               = 210*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC5_UNORM                 = 211*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC5_SNORM                 = 212*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC5_UNORM_LA              = 213*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC5_SNORM_LA              = 214*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC6H_UF16                 = 215*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC6H_SF16                 = 216*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC7_UNORM                 = 217*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC7_UNORM_SRGB            = 218*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_G8_UINT                   = 219?0?UNORM8*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_3DC_UNORM                 = 220*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_YUY2                      = 221*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_Y216_VIDEO                = 222*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_Y210_VIDEO                = 223*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_A4I4_VIDEO                = 224?0*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_A8L8_UNORM                = 225*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_L8A8_UNORM                = 226*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_ZL1                       = 227*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_R16G16B16A16_UNORM_SRGB   = 228*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_YCRCB_MB_8_422            = 229*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_YCRCB_MB_8_420            = 230*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_YCRCB_MB_16_422           = 231*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_P016T                     = 232?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_UYVA1010102_VIDEO         = 233?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_UYVY                      = 234?CS_U_FMT_U0_DATA_FMT_RESERVED*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_BAYER_12                  = 235?0*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_BAYER                     = 236?0*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_P016L                     = 237?*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_YV12                      = 238*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_NV12T                     = 239*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_S1_UINT                   = 240*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_S16_UINT                  = 241*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_S4_UINT                   = 242*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_S8_UINT                   = 243*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_I12_UNORM                 = 244*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_I16_UNORM                 = 245*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_I4_UNORM                  = 246*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_I8_UNORM                  = 247*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_L12_UNORM                 = 248*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_L12_A12_UNORM             = 249*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_L12_A4_UNORM              = 250*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_L16_A16_UNORM             = 251*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_L4_UNORM                  = 252*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_L4_A4_UNORM               = 253*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_L6_A2_UNORM               = 254*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_RGB8_ETC2                  = 255?0*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_SRGB8_ETC2                 = 256?0*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA8_ETC2_EAC                       = 257*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_SRGB8_ALPHA8_ETC2_EAC                = 258*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_R11_EAC                              = 259?0*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RG11_EAC                             = 260*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_SIGNED_R11_EAC                       = 261*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_SIGNED_RG11_EAC                      = 262*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2        = 263*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2       = 264*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_ALPHA                     = 265*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_INTENSITY                 = 266*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_LUMINANCE                 = 267*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_RED                       = 268*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_RED_RGTC1                 = 269*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_RG                        = 270*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_RG_RGTC2                  = 271*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_RGB                       = 272*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_RGB_BPTC_SIGNED_FLOAT     = 273*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT   = 274*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_RGBA                      = 275*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_RGBA_BPTC_UNORM           = 276*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_SIGNED_RED_RGTC1          = 277*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_COMPRESSED_SIGNED_RG_RGTC2           = 27*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_COMPRESSED_SLUMINANCE                = 279*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_COMPRESSED_SLUMINANCE_ALPHA          = 280*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_COMPRESSED_SRGB                      = 281*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_COMPRESSED_SRGB_ALPHA                = 282*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_COMPRESSED_SRGB_ALPHA_BPTC_UNORM     = 283*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R10G10B10A2_USCALE                   = 284?0*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R10G10B10A2_SSCALE                   = 285*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32G32B32A32_SSCALE                  = 286,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32G32B32_SSCALE                     = 287,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32G32_SSCALE                        = 288,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16G16B16_SSCALE                     = 289,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32_SSCALE                           = 290,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8_SSCALE                        = 291,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*HSF_UNKOWN                               = 292*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 293*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 294*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 295*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 296*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 297*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 298*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 299*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 300*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 301*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 302*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 303*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 304*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 305*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 306*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 307*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 308*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 309*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 310*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 311*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 312*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 313*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 314*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 315*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 316*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 317*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 318*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 319*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_UNKOWN                               = 320*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_4X4                        = 321*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_5X4                        = 322*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_5X5                        = 323*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_6X5                        = 324*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_6X6                        = 325*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_8X5                        = 326*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_8X6                        = 327*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_8X8                        = 328*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_10X5                       = 329*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_10X6                       = 330*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_10X8                       = 331*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_10X10                      = 332*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_12X10                      = 333*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_12X12                      = 334*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*HSF_RESERVED0                            = 335*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED, /*HSF_RESERVED1                            = 336*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_4X4_SRGB                   = 337*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_5X4_SRGB                   = 338*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_5X5_SRGB                   = 339*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_6X5_SRGB                   = 340*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_6X6_SRGB                   = 341*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_8X5_SRGB                   = 342*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_8X6_SRGB                   = 343*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_8X8_SRGB                   = 344*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_10X5_SRGB                  = 345*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_10X6_SRGB                  = 346*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_10X8_SRGB                  = 347*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_10X10_SRGB                 = 348*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_12X10_SRGB                 = 349*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,    /*HSF_RGBA_ASTC_12X12_SRGB                 = 350*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,  /*HSF_RESERVED2                            = 351*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,  /*HSF_RESERVED3                            = 352*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_3x3x3                      = 353,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_4x3x3                      = 354,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_4x4x3                      = 355,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_4x4x4                      = 356,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_5x4x4                      = 357,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_5x5x4                      = 358,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_5x5x5                      = 359,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_6x5x5                      = 360,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_6x6x5                      = 361,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_6x6x6                      = 362,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,  /*HSF_ASTC_RESERVED4                            = 363,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,  /*HSF_ASTC_RESERVED5                            = 364,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,  /*HSF_ASTC_RESERVED6                            = 365,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,  /*HSF_ASTC_RESERVED7                            = 366,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,  /*HSF_ASTC_RESERVED8                            = 367,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,  /*HSF_ASTC_RESERVED9                            = 368,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_3x3x3_SRGB                 = 369, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_4x3x3_SRGB                 = 370, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_4x4x3_SRGB                 = 371, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_4x4x4_SRGB                 = 372, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_5x4x4_SRGB                 = 373, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_5x5x4_SRGB                 = 374,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_5x5x5_SRGB                 = 375,  */\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_6x5x5_SRGB                 = 376, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_6x6x5_SRGB                 = 377, */\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_RGBA_ASTC_6x6x6_SRGB                 = 378,*/\
    CS_U_FMT_U0_DATA_FMT_UINT64,     /*HSF_R64_UINT                             = 379,*/\
    CS_U_FMT_U0_DATA_FMT_UINT64,     /*HSF_R64G64_UINT                          = 380,*/\
    CS_U_FMT_U0_DATA_FMT_UINT64,     /*HSF_R64G64B64_UINT                       = 381,*/\
    CS_U_FMT_U0_DATA_FMT_UINT64,     /*HSF_R64G64B64A64_UINT                    = 382,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_R64_SINT                             = 383,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R64G64_SINT                          = 384,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R64G64B64_SINT                       = 385,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R64G64B64A64_SINT                    = 386,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R4G4B4X4_UNORM                       = 387,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_R5G5B5X1_UNORM                       = 388,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8X8_SNORM                       = 389,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8X8_UINT                        = 390,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_R8G8B8X8_SINT                        = 391,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R8G8B8X8_UNORM_SRGB                  = 392,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R10G10B10X2_UNORM                    = 393,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16G16B16X16_UNORM                   = 394,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16G16B16X16_SNORM                   = 395,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16G16B16X16_FLOAT                   = 396,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16G16B16X16_UINT                    = 397,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R16G16B16X16_SINT                    = 398,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32G32B32X32_FLOAT                   = 399,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32G32B32X32_UINT                    = 400,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_R32G32B32X32_SINT                    = 401,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_S8_UINT_D24_UNORM                    = 402,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_D24_UNORM_S8_UINT                    = 403,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_S8_UINT_D32_FLOAT                    = 404,*/\
    CS_U_FMT_U0_DATA_FMT_UNORM24,      /*HSF_R24_UNORM                            = 405,*/\
    CS_U_FMT_U0_DATA_FMT_UNORM24,      /*HSF_R24G24_UNORM                         = 406,*/\
    CS_U_FMT_U0_DATA_FMT_UNORM24,      /*HSF_R24G24B24A24_UNORM                   = 407,*/\
    CS_U_FMT_U0_DATA_FMT_UNORM10,      /*HSF_R10G10B10A10_UNORM                   = 408,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,     /*HSF_BC1_TYPELESS                         = 409,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_B8G8R8A8_TYPELESS                   = 410,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_B8G8R8X8_TYPELESS                   = 411,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC2_TYPELESS                        = 412,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC3_TYPELESS                        = 413,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC4_TYPELESS                        = 414,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*HSF_BC5_TYPELESS                        = 415,*/\
    CS_U_FMT_U0_DATA_FMT_RESERVED,      /*DXGI_FORMAT_BC6H_TYPELESS               = 416,*/\
}


typedef enum
{
    DS_NONE                = 0,
    S8_UINT                = 0x1,
    D16_UNORM              = 0x2,
    D24_UNORM              = 0x4,
    D32_FLOAT              = 0x8,
    D24_UNORM_S8_UINT      = D24_UNORM | S8_UINT,
    D32_FLOAT_S8_UINT      = D32_FLOAT | S8_UINT,
} DEPTH_STENCIL_FORMAT;

#define CHANNEL_MASK_R              1
#define CHANNEL_MASK_G              2
#define CHANNEL_MASK_B              4
#define CHANNEL_MASK_A              8
#define CHANNEL_MASK_X              16

typedef enum CHANNEL_MASK
{
    C_NONE                 = 0,
    C_R                    = CHANNEL_MASK_R,
    C_A                    = CHANNEL_MASK_A,
    C_GR                   = CHANNEL_MASK_G | CHANNEL_MASK_R,
    C_BGR                  = CHANNEL_MASK_B | CHANNEL_MASK_G | CHANNEL_MASK_R,
    C_BGRA                 = CHANNEL_MASK_B | CHANNEL_MASK_G | CHANNEL_MASK_R | CHANNEL_MASK_A,
    C_RGBX                 = CHANNEL_MASK_R | CHANNEL_MASK_G | CHANNEL_MASK_B | CHANNEL_MASK_X,
} CHANNEL_MASK;

typedef struct HWFORMAT_TABLE_ENTRY_E3K
{
    Hw_Surf_Format        MappedDstFormat;
    Hw_Surf_Format        SuppressAlphaFormat;
    DEPTH_STENCIL_FORMAT  CompatibleDsFmt;
    COMPRESS_MXUFORMAT    CompressFmt;
    int                   ChannelMask;
    int                   bRtSupport;
    int                   bBlockCompressed;
    int                   bYUY2;
    int                   b96bpp;
    int                   bUnorm;
    int                   bSnorm;
    int                   bSrgb;
    int                   bTypeless;
    int                   bFP32Mode;
    int                   bASTC;
    int                   BlockWidth;
    int                   BlockHeight;
    int                   BlockDepth;
    int                   ClrLineNum;
} HWFORMAT_TABLE_ENTRY_E3K;

extern const HWFORMAT_TABLE_ENTRY_E3K g_HwFormatTable[];

#define HWFORMAT_TABLE \
{\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*HSF_UNKNOWN              = 0,  */\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R64G64B64A64_FLOAT    = 1,  */\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R64G64B64_FLOAT       = 2,  */\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_TYPELESS = 3,  */\
    HSF_R32G32B32A32_FLOAT, HSF_R32G32B32X32_FLOAT,DS_NONE,       CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32A32_FLOAT   = 4,  */\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32A32_UINT    = 5,  */\
    HSF_R32G32B32A32_SINT,  HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32A32_SINT    = 6,  */\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_SNORM    = 7,  */\
    HSF_R32G32B32A32_SCALE, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_SCALE    = 8,  */\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_UNORM    = 9,  */\
    HSF_R32G32B32A32_USCALE,HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_USCALE   = 10, */\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_FIX      = 11, */\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R64G64_FLOAT          = 12, */\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32_TYPELESS    = 13, */\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  1,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32_FLOAT      = 14, */\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  1,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32_UINT       = 15, */\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  1,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32_SINT       = 16, */\
    HSF_R32G32B32_UNORM,    HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  1,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32_UNORM       = 17, */\
    HSF_R32G32B32_USCALE,   HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32_USCALE      = 18, */\
    HSF_R32G32B32_SNORM,    HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  1,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32_SNORM       = 19, */\
    HSF_R32G32B32_SCALE,    HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32_SCALE       = 20, */\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32_FIX         = 21, */\
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGRA, 0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16A16_TYPELESS = 22, */\
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16A16_FLOAT    = 23, */\
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16A16_UNORM    = 24, */\
    HSF_R16G16B16A16_UINT,  HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16A16_UINT     = 25, */\
    HSF_R16G16B16A16_SNORM, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGRA, 1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16A16_SNORM    = 26, */\
    HSF_R16G16B16A16_SINT,  HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16A16_SINT     = 27, */\
    HSF_R16G16B16A16_SCALE, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16A16_SCALE    = 28, */\
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16A16_USCALE   = 29, */\
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16A16_SSCALE   = 30, */\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R12G12B12A12_UNORM    = 31, */\
    HSF_R16G16B16_FLOAT,    HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_FLOAT       = 32, */\
    HSF_R16G16B16_SINT,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_SINT        = 33, */\
    HSF_R16G16B16_SNORM,    HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_SNORM       = 34, */\
    HSF_R16G16B16_SCALE,    HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_SCALE       = 35, */\
    HSF_R16G16B16_UINT,     HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_UINT        = 36, */\
    HSF_R16G16B16_UNORM,    HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGR,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_UNORM       = 37, */\
    HSF_R16G16B16_USCALE,   HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_USCALE      = 38, */\
    HSF_R32G32_FLOAT,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32_TYPELESS       = 39, */\
    HSF_R32G32_FLOAT,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32_FLOAT         = 40, */\
    HSF_R32G32_UINT,        HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32_UINT          = 41, */\
    HSF_R32G32_SINT,        HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32_SINT          = 42, */\
    HSF_R32G32_SINT,        HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32_SNORM          = 43, */\
    HSF_R32G32_SCALE,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32_SCALE          = 44, */\
    HSF_R32G32_UINT,        HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   0,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32_UNORM          = 45, */\
    HSF_R32G32_USCALE,      HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32_USCALE         = 46, */\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32_FIX            = 47, */\
    HSF_R32_FLOAT,          HSF_UNKNOWN,    D32_FLOAT_S8_UINT,    CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32G8X24_TYPELESS     = 48, */\
    HSF_R32_FLOAT,          HSF_UNKNOWN,    D32_FLOAT_S8_UINT,    CP_Z32,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_D32_FLOAT_S8X24_UINT  = 49, */\
    HSF_R32G32_FLOAT,       HSF_UNKNOWN,    D32_FLOAT_S8_UINT,    CP_Z32,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32_FLOAT_X8X24_TYPELESS = 50, */\
    HSF_UNKNOWN,            HSF_UNKNOWN,    D32_FLOAT_S8_UINT,    CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_X32_TYPELESS_G8X24_UINT  = 51, */\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R64_FLOAT             = 52, */\
    HSF_R16G16B16X16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_RGBX, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R12G12B12_UNORM       = 53, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10A2_TYPELESS  = 54, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R10G10B10A2_UNORM     = 55, */\
    HSF_R10G10B10A2_UINT,   HSF_R10G10B10X2_UINT, DS_NONE,        CP_R10G10B10A2, C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R10G10B10A2_UINT      = 56, */\
    HSF_R10G10B10A2_UINT,   HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10X2_UINT      = 57, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10X2_USCALE    = 58, */\
    HSF_R10G10B10A2_UNORM,  HSF_R10G10B10X2_SNORM,DS_NONE,        CP_R10G10B10A2, C_BGRA, 0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10A2_SNORM     = 59, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10A2_SINT      = 60, */\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10X2_SNORM     = 61, */\
    HSF_B10G10R10A2_SNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_B10G10R10A2_SNORM     = 62, */\
    HSF_B10G10R10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_B10G10R10A2_UNORM     = 63, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_B10G10R10X2_UNORM     = 64, */\
    HSF_A2B10G10R10_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_A2B10G10R10, C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A2B10G10R10_UNORM     = 65, */\
    HSF_A2B10G10R10_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_A2B10G10R10, C_BGRA, 0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A2B10G10R10_SNORM     = 66, */\
    HSF_A2B10G10R10_USCALE, HSF_UNKNOWN,          DS_NONE,        CP_A2B10G10R10, C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A2B10G10R10_USCALE    = 67, */\
    HSF_A2B10G10R10_SSCALE, HSF_UNKNOWN,          DS_NONE,        CP_A2B10G10R10, C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A2B10G10R10_SSCALE    = 68, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_SINT        = 69, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_SNORM       = 70, */\
    HSF_R10G10B10_SCALE,    HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_SCALE       = 71, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_UNIT        = 72, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_UNORM       = 73, */\
    HSF_R10G10B10_USCALE,   HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_USCALE      = 74, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10_SINT           = 75, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10_SNORM          = 76, */\
    HSF_R10G10_SCALE,       HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10_SCALE          = 77, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10_UINT           = 78, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10_UNORM          = 79, */\
    HSF_R10G10_SCALE,       HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10_USCALE         = 80, */\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_FLOAT_A2_UNORM = 81, */\
    HSF_R11G11B10_FLOAT,    HSF_UNKNOWN,          DS_NONE,        CP_R11G11B10,   C_BGR,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R11G11B10_FLOAT       = 82, */\
    HSF_B10G11R11_FLOAT,    HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_B10G11R11_FLOAT       = 83, */\
    HSF_R11G11B10_FLOAT,    HSF_UNKNOWN,          DS_NONE,        CP_R11G11B10,   C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R11G11_FLOAT          = 84, */\
    HSF_R8G8B8A8_UNORM,     HSF_R8G8B8X8_UNORM,   DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8A8_TYPELESS     = 85, */\
    HSF_R8G8B8A8_UNORM,     HSF_R8G8B8X8_UNORM,   DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8A8_UNORM        = 86, */\
    HSF_R8G8B8A8_UNORM_SRGB,HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8A8_UNORM_SRGB   = 87, */\
    HSF_R8G8B8X8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8X8_UNORM        = 88, */\
    HSF_B8G8R8A8_UNORM,     HSF_B8G8R8X8_UNORM,   DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_B8G8R8A8_UNORM        = 89, */\
    HSF_B8G8R8A8_UNORM_SRGB,HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  4,/*HSF_B8G8R8A8_UNORM_SRGB   = 90, */\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_B8G8R8X8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_B8G8R8X8_UNORM        = 91, */\
    HSF_B8G8R8X8_UNORM_SRGB,HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  4,/*HSF_B8G8R8X8_UNORM_SRGB   = 92, */\
    HSF_X8R8G8B8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_A8R8G8B8_L,  C_RGBX, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_X8R8G8B8_UNORM        = 93, */\
    HSF_X8B8G8R8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_A8R8G8B8_L,  C_RGBX, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_X8B8G8R8_UNORM        = 94, */\
    HSF_R8G8B8A8_UNORM,     HSF_X8R8G8B8_UNORM,   DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A8R8G8B8_UNORM        = 95, */\
    HSF_A8B8G8R8_UNORM,     HSF_X8B8G8R8_UNORM,   DS_NONE,        CP_A8R8G8B8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_A8B8G8R8_UNORM        = 96, */\
    HSF_R8G8B8A8_UINT,      HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8A8_UINT         = 97, */\
    HSF_R8G8B8A8_SNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8A8_SNORM        = 98, */\
    HSF_R8G8B8A8_SINT,      HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8A8_SINT         = 99, */\
    HSF_R8G8B8A8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_B8G8R8A8_XNORM        = 100,*/\
    HSF_R8G8B8A8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8A8_SSCALE       = 101,*/\
    HSF_R8G8B8A8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8A8_USCALE       = 102,*/\
    HSF_R8G8B8_SINT,        HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_SINT           = 103,*/\
    HSF_R8G8B8_SNORM,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_SNORM          = 104,*/\
    HSF_R8G8B8_SCALE,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_SCALE          = 105,*/\
    HSF_R8G8B8_UINT,        HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_UINT           = 106,*/\
    HSF_R8G8B8_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_UNORM          = 107,*/\
    HSF_R8G8B8_USCALE,      HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_USCALE         = 108,*/\
    HSF_R16G16_FLOAT,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16_TYPELESS       = 109,*/\
    HSF_R16G16_FLOAT,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16_FLOAT          = 110,*/\
    HSF_R16G16_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16_UNORM          = 111,*/\
    HSF_R16G16_UINT,        HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16_UINT           = 112,*/\
    HSF_R16G16_SNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16_SNORM          = 113,*/\
    HSF_R16G16_SINT,        HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16_SINT           = 114,*/\
    HSF_R16G16_SCALE,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16_SCALE          = 115,*/\
    HSF_R16G16_FLOAT,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16_USCALE         = 116,*/\
    HSF_R16G16_FLOAT,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16_SSCALE         = 117,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,          D32_FLOAT,      CP_Z32,         C_R,    0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,/*HSF_R32_TYPELESS          = 118,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,          D32_FLOAT,      CP_Z32,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_D32_FLOAT             = 119,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,          D32_FLOAT,      CP_Z32,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_D32_UNORM             = 120,*/\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_R32_FLOAT,          HSF_UNKNOWN,          D32_FLOAT,      CP_Z32,         C_R,    1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32_FLOAT            = 121,*/\
    HSF_R32_UINT,           HSF_UNKNOWN,          DS_NONE,        CP_Z32,         C_R,    1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32_UINT             = 122,*/\
    HSF_R32_SINT,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32_SINT             = 123,*/\
    HSF_R32_UINT,           HSF_UNKNOWN,          DS_NONE,        CP_Z32,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32_UNORM             = 124,*/\
    HSF_R32_USCALE,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32_USCALE            = 125,*/\
    HSF_R32_SINT,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32_SNORM             = 126,*/\
    HSF_R32_SCALE,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32_SCALE             = 127,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_R,    0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32_FIX               = 128,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,     D24_UNORM_S8_UINT,   CP_Z24,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R24G8_TYPELESS        = 129,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,     D32_FLOAT_S8_UINT,   CP_Z32,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_D32_FLOAT_S8_UINT     = 130,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,     D24_UNORM_S8_UINT,   CP_Z24,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_D24_UNORM_S8_UINT     = 131,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_Z24,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R24_UNORM_X8_TYPELESS = 132,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_X24_TYPELESS_G8_UINT  = 133,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,          D24_UNORM,      CP_Z24,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_D24_UNORM             = 134,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_Z24,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R24_FLOAT             = 135,*/\
    HSF_R8G8_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8_TYPELESS         = 136,*/\
    HSF_R8G8_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8_UNORM            = 137,*/\
    HSF_R8G8_UINT,          HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8_UINT             = 138,*/\
    HSF_R8G8_SNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8_SNORM            = 139,*/\
    HSF_R8G8_SINT,          HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8_SINT             = 140,*/\
    HSF_R8G8_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8_USCALE           = 141,*/\
    HSF_R8G8_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8_SSCALE           = 142,*/\
    HSF_R16_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_Z16,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16_TYPELESS          = 143,*/\
    HSF_R16_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_Z16,         C_R,    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16_FLOAT             = 144,*/\
    HSF_R16_UNORM,          HSF_UNKNOWN,          D16_UNORM,      CP_Z16,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_D16_UNORM             = 145,*/\
    HSF_R16_UNORM,          HSF_UNKNOWN,          D16_UNORM,      CP_Z16,         C_R,    1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16_UNORM             = 146,*/\
    HSF_R16_UINT,           HSF_UNKNOWN,          DS_NONE,        CP_Z16,         C_R,    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16_UINT              = 147,*/\
    HSF_R16_SNORM,          HSF_UNKNOWN,          DS_NONE,        CP_Z16,         C_R,    1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16_SNORM             = 148,*/\
    HSF_R16_SINT,           HSF_UNKNOWN,          DS_NONE,        CP_Z16,         C_R,    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16_SINT              = 149,*/\
    HSF_R16_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16_USCALE            = 150,*/\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_R16_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16_SSCALE            = 151,*/\
    HSF_A16_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_A,    1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A16_UNORM             = 152,*/\
    HSF_B5G6R5_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGR,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_B5G6R5_UNORM          = 153,*/\
    HSF_R5G6B5_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGR,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R5G6B5_UNORM          = 154,*/\
    HSF_B5G5R5A1_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGR,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_B5G5R5X1_UNORM        = 155,*/\
    HSF_B5G5R5A1_UNORM,     HSF_B5G5R5X1_UNORM,   DS_NONE,        CP_R5G6B5,      C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_B5G5R5A1_UNORM        = 156,*/\
    HSF_R5G5B5A1_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R5G5B5A1_UNORM        = 157,*/\
    HSF_A1B5G5R5_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_A1B5G5R5_UNORM        = 158,*/\
    HSF_A1R5G5B5_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_A1R5G5B5_UNORM        = 159,*/\
    HSF_B4G4R4A4_UNORM,     HSF_B4G4R4X4_UNORM,   DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_B4G4R4A4_UNORM        = 160,*/\
    HSF_B4G4R4A4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_B4G4R4X4_UNORM        = 161,*/\
    HSF_R4G4B4A4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R4G4B4A4_UNORM        = 162,*/\
    HSF_A4B4G4R4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_A8R8G8B8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_A4B4G4R4_UNORM        = 163,*/\
    HSF_R4G4B4A4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A4R4G4B4_UNORM        = 164,*/\
    HSF_R4G4B4A4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R4G4B4_UNORM          = 165,*/\
    HSF_R5G5B5_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGR,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R5G5B5_UNORM          = 166,*/\
    HSF_B5G6R5_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_U5V5L6_XNORM          = 167,*/\
    HSF_R16_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_Z16,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L16_UNORM             = 168,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L8_A8_UNORM           = 169,*/\
    HSF_R10_SINT,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10_SINT              = 170,*/\
    HSF_R10_SNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10_SNORM             = 171,*/\
    HSF_R10_SCALE,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10_SCALE             = 172,*/\
    HSF_R10_UINT,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10_UINT              = 173,*/\
    HSF_R10_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10_UNORM             = 174,*/\
    HSF_R10_USCALE,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10_USCALE            = 175,*/\
    HSF_R4G4B4A4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R2G2B2A2_UNORM        = 176,*/\
    HSF_R8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8_TYPELESS           = 177,*/\
    HSF_R8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_R,    1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8_UNORM              = 178,*/\
    HSF_R8_UINT,            HSF_UNKNOWN,          S8_UINT,        CP_R8G8B8A8_L,  C_R,    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8_UINT               = 179,*/\
    HSF_R8_SNORM,           HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_R,    1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8_SNORM              = 180,*/\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_R8_SINT,            HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_R,    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8_SINT               = 181,*/\
    HSF_R8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8_USCALE             = 182,*/\
    HSF_R8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8_SSCALE             = 183,*/\
    HSF_A8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_A,    1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_A8_UNORM              = 184,*/\
    HSF_R8_UNORM,           HSF_UNKNOWN,          S8_UINT,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L8_UNORM              = 185,*/\
    HSF_A16_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A12_UNORM             = 186,*/\
    HSF_A8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A4_UNORM              = 187,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R1_UNORM              = 188,*/\
    HSF_R4G4B4X4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R3G3B2_UNORM          = 189,*/\
    HSF_L4A4_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_L4A4_UNORM            = 190,*/\
    HSF_L4A4_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A4L4_UNORM            = 191,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L4A4_VIDEO            = 192,*/\
    HSF_R8G8B8A8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_AYUV_VIDEO            = 193,*/\
    HSF_YUYV,               HSF_UNKNOWN,          DS_NONE,        CP_YUYV,        C_BGRA, 1,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_YUYV                  = 194,*/\
    HSF_NV12,               HSF_UNKNOWN,          DS_NONE,        CP_NV12,        C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_NV12                  = 195,*/\
    HSF_P010,               HSF_UNKNOWN,          DS_NONE,        CP_NV12_10,     C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_P010                  = 196,*/\
    HSF_P016,               HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_P016                  = 197,*/\
    HSF_R8G8B8A8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R9G9B9E5_SHAREDEXP    = 198,*/\
    HSF_UYVY,               HSF_UNKNOWN,          DS_NONE,        CP_UYVY,        C_BGRA, 1,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8_B8G8_UNORM       = 199,*/ /*!< we treat these two(199/200) as UYVY/YUYV*/\
    HSF_YUYV,               HSF_UNKNOWN,          DS_NONE,        CP_YUYV,        C_BGRA, 1,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_G8R8_G8B8_UNORM       = 200,*/\
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC1_UNORM             = 201,*/\
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC1_UNORM_SRGB        = 202,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC2_UNORM             = 203,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC2_UNORM_SRGB        = 204,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC3_UNORM             = 205,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC3_UNORM_SRGB        = 206,*/\
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC4_UNORM             = 207,*/\
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC4_SNORM             = 208,*/\
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC4_UNORM_L           = 209,*/\
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC4_SNORM_L           = 210,*/\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC5_UNORM             = 211,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC5_SNORM             = 212,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC5_UNORM_LA          = 213,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC5_SNORM_LA          = 214,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC6H_UF16             = 215,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC6H_SF16             = 216,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC7_UNORM             = 217,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC7_UNORM_SRGB        = 218,*/\
    HSF_R8_UNORM,           HSF_UNKNOWN,          S8_UINT,        CP_R8G8B8A8_L,  C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_G8_UINT               = 219,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_3DC_UNORM             = 220,*/\
    HSF_YUYV,               HSF_UNKNOWN,          DS_NONE,        CP_YUYV,        C_NONE, 0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_YUY2                  = 221,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_Y216_VIDEO            = 222,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_Y210_VIDEO            = 223,*/\
    HSF_A4I4_VIDEO,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A4I4_VIDEO            = 224,*/\
    HSF_A8L8_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A8L8_UNORM            = 225,*/\
    HSF_L8A8_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L8A8_UNORM            = 226,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_ZL1                   = 227,*/\
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_NONE, 0,  0,  0,  0,  1,  0,  1,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16A16_UNORM_SRGB = 228*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_YCRCB_MB_8_422        = 229,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_YCRCB_MB_16_420       = 230,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_YCRCB_MB_16_422       = 231,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_P016T                 = 232,*/\
    HSF_UYVA1010102_VIDEO,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_UYVA1010102_VIDEO     = 233,*/\
    HSF_UYVY,               HSF_UNKNOWN,          DS_NONE,        CP_UYVY,        C_BGRA, 1,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_UYVY                  = 234,*/\
    HSF_R16_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_BAYER,       C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_BAYER_12              = 235,*/\
    HSF_R16_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_BAYER,       C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_BAYER                 = 236,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_P016L                 = 237,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_YUYV,        C_NONE, 0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_YV12                  = 238,*/\
    HSF_L8A8_SNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_NONE, 1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_L8A8_SNORM            = 239,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_S1_UINT               = 240,*/\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_S16_UINT              = 241,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_S4_UINT               = 242,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_S8_UINT               = 243,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_I12_UNORM             = 244,*/\
    HSF_R16_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_I16_UNORM             = 245,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_I4_UNORM              = 246,*/\
    HSF_R8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_I8_UNORM              = 247,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L12_UNORM             = 248,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L12_A12_UNORM         = 249,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L12_A4_UNORM          = 250,*/\
    HSF_R16G16_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L16_A16_UNORM         = 251,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L4_UNORM              = 252,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L4_A4_UNORM           = 253,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L6_A2_UNORM           = 254,*/\
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_RGB8_ETC2             = 255,*/\
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_SRGB8_ETC2            = 256,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_RGBA8_ETC2_EAC        = 257,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_SRGB8_ALPHA8_ETC2_EAC = 258,*/\
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_R11_EAC               = 259,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_RG11_EAC              = 260,*/\
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_SIGNED_R11_EAC        = 261,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_SIGNED_RG11_EAC       = 262,*/\
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2  = 263,*/\
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 264,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_ALPHA      = 265,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_INTENSITY  = 266,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_LUMINANCE  = 267,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RED        = 268,*/\
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_COMPRESSED_RED_RGTC1  = 269,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RG         = 270,*/\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_COMPRESSED_RG_RGTC2                = 271,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RGB                     = 272,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RGB_BPTC_SIGNED_FLOAT   = 273,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = 274,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RGBA                    = 275,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RGBA_BPTC_UNORM         = 276,*/\
    HSF_R16G16B16A16_SINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_COMPRESSED_SIGNED_RED_RGTC1        = 277,*/\
    HSF_R32G32B32A32_SINT , HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_COMPRESSED_SIGNED_RG_RGTC2         = 278,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_SLUMINANCE              = 279,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_SLUMINANCE_ALPHA        = 280,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_SRGB                    = 281,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_SRGB_ALPHA              = 282,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_SRGB_ALPHA_BPTC_UNORM   = 283,*/\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10A2_USCALE                 = 284,*/\
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10A2_SSCALE                 = 285,*/\
    HSF_R32G32B32A32_SSCALE,HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_SSCALE                = 286,*/\
    HSF_R32G32B32_SSCALE,   HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32G32B32_SSCALE                   = 287,*/\
    HSF_R32G32_SSCALE,      HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32G32_SSCALE                      = 288,*/\
    HSF_R16G16B16_SSCALE,   HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_SSCALE                   = 289,*/\
    HSF_R32_SSCALE,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32_SSCALE                         = 290,*/\
    HSF_R8G8B8_SSCALE,      HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_SSCALE                      = 291,*/\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 292,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 293,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 294,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 295,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 296,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 297,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 298,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 299,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 300,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 301,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 302,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 303,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 304,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 305,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 306,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 307,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 308,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 309,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 310,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 311,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 312,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 313,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 314,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 315,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 316,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 317,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 318,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 319,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place                = 320,*/\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  4,  4,  1,  0,/*HSF_RGBA_ASTC_4X4         = 321,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  5,  4,  1,  0,/*HSF_RGBA_ASTC_5X4         = 322,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  5,  5,  1,  0,/*HSF_RGBA_ASTC_5X5         = 323,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  6,  5,  1,  0,/*HSF_RGBA_ASTC_6X5         = 324,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  6,  6,  1,  0,/*HSF_RGBA_ASTC_6X6         = 325,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  8,  5,  1,  0,/*HSF_RGBA_ASTC_8X5         = 326,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  8,  6,  1,  0,/*HSF_RGBA_ASTC_8X6         = 327,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  8,  8,  1,  0,/*HSF_RGBA_ASTC_8X8         = 328,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  10, 5,  1,  0,/*HSF_RGBA_ASTC_10X5        = 329,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  10, 6,  1,  0,/*HSF_RGBA_ASTC_10X6        = 330,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  10, 8,  1,  0,/*HSF_RGBA_ASTC_10X8        = 331,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  10, 10, 1,  0,/*HSF_RGBA_ASTC_10X10       = 332,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  12, 10, 1,  0,/*HSF_RGBA_ASTC_12X10       = 333,*/\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  12, 12,  1,  0,/*HSF_RGBA_ASTC_12X12      = 334,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,/*HSF_RESERVED2            = 335,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,/*HSF_RESERVED3            = 336,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  4,  4,  1,   0,/*HSF_RGBA_ASTC_4X4_SRGB   = 337,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  5,  4,  1,   0,/*HSF_RGBA_ASTC_5X4_SRGB   = 338,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  5,  5,  1,   0,/*HSF_RGBA_ASTC_5X5_SRGB   = 339,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  6,  5,  1,   0,/*HSF_RGBA_ASTC_6X5_SRGB   = 340,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  6,  6,  1,   0,/*HSF_RGBA_ASTC_6X6_SRGB   = 341,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  8,  5,  1,   0,/*HSF_RGBA_ASTC_8X5_SRGB   = 342,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  8,  6,  1,   0,/*HSF_RGBA_ASTC_8X6_SRGB   = 343,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  8,  8,  1,   0,/*HSF_RGBA_ASTC_8X8_SRGB   = 344,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  10, 5,  1,   0,/*HSF_RGBA_ASTC_10X5_SRGB  = 345,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  10, 6,  1,   0,/*HSF_RGBA_ASTC_10X6_SRGB  = 346,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  10, 8,  1,   0,/*HSF_RGBA_ASTC_10X8_SRGB  = 347,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  10, 10, 1,   0,/*HSF_RGBA_ASTC_10X10_SRGB = 348,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  12, 10, 1,   0,/*HSF_RGBA_ASTC_12X10_SRGB = 349,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  12, 12, 1,   0,/*HSF_RGBA_ASTC_12X12_SRGB = 350,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,/*HSF_RESERVED4            = 351,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,/*HSF_RESERVED5            = 352,*/\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    /*the following new added by Martina ASTC 3D*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  3,  3,  3,  0,/*HSF_RGBA_ASTC_3x3x3       = 353,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  4,  3,  3,  0,/*HSF_RGBA_ASTC_4x3x3       = 354,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  4,  4,  3,  0,/*HSF_RGBA_ASTC_4x4x3       = 355,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  4,  4,  4,  0,/*HSF_RGBA_ASTC_4x4x4       = 356,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  5,  4,  4,  0,/*HSF_RGBA_ASTC_5x4x4       = 357,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  5,  5,  4,  0,/*HSF_RGBA_ASTC_5x5x4       = 358,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  5,  5,  5,  0,/*HSF_RGBA_ASTC_5x5x5       = 359,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  6,  5,  5,  0,/*HSF_RGBA_ASTC_6x5x5       = 360,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  6,  6,  5,  0,/*HSF_RGBA_ASTC_6x6x5       = 361,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  6,  6,  6,  0,/*HSF_RGBA_ASTC_6x6x6       = 362,*/\
    HSF_R16G16_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L16A16_UNORM          = 363,*/\
    HSF_R16G16_SNORM,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L16A16_SNORM          = 364,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_ASTC_RESERVED6        = 365,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_ASTC_RESERVED7        = 366,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_ASTC_RESERVED8        = 367,*/\
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_ASTC_RESERVED9        = 368,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  3,  3,  3,  0,/*HSF_RGBA_ASTC_3x3x3_SRGB  = 369,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  4,  3,  3,  0,/*HSF_RGBA_ASTC_4x3x3_SRGB  = 370,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  4,  4,  3,  0,/*HSF_RGBA_ASTC_4x4x3_SRGB  = 371,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  4,  4,  4,  0,/*HSF_RGBA_ASTC_4x4x4_SRGB  = 372,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  5,  4,  4,  0,/*HSF_RGBA_ASTC_5x4x4_SRGB  = 373,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  5,  5,  4,  0,/*HSF_RGBA_ASTC_5x5x4_SRGB  = 374,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  5,  5,  5,  0,/*HSF_RGBA_ASTC_5x5x5_SRGB  = 375,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  6,  5,  5,  0,/*HSF_RGBA_ASTC_6x5x5_SRGB  = 376,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  6,  6,  5,  0,/*HSF_RGBA_ASTC_6x6x5_SRGB  = 377,*/\
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  6,  6,  6,  0,/*HSF_RGBA_ASTC_6x6x6_SRGB  = 378,*/\
    HSF_R64_UINT,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64_UINT              = 379,*/\
    HSF_R64G64_UINT,        HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64G64_UINT           = 380,*/\
    /*                                                                                ChannelMask                                                                                 */\
    /*                                                                                |   bRtSupport                                                                              */\
    /*                                                                                |   |   bBlockCompressed                                                                    */\
    /*                                                                                |   |   |   bYUY2                                                                           */\
    /*                                                                                |   |   |   |   b96bpp                                                                      */\
    /*                                                                                |   |   |   |   |   bUnorm                                                                  */\
    /*                                                                                |   |   |   |   |   |   bSnorm                                                              */\
    /*                                                                                |   |   |   |   |   |   |   bSrgb                                                           */\
    /*                                                                                |   |   |   |   |   |   |   |   bTypeless                                                   */\
    /*  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                                               */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                                               */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth                                     */\
    /*..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight                                */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth                             */\
    /*  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum                         */\
    HSF_R64G64B64_UINT,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64G64B64_UINT        = 381,*/\
    HSF_R64G64B64A64_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64G64B64A64_UINT     = 382,*/\
    HSF_R64_SINT,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64_SINT              = 383,*/\
    HSF_R64G64_SINT,        HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64G64_SINT           = 384,*/\
    HSF_R64G64B64_SINT,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64G64B64_SINT        = 385,*/\
    HSF_R64G64B64A64_SINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64G64B64A64_SINT     = 386,*/\
    HSF_R4G4B4X4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R4G4B4X4_UNORM        = 387,*/\
    HSF_R5G5B5X1_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_RGBX, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R5G5B5X1_UNORM        = 388,*/\
    HSF_R8G8B8X8_SNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8X8_SNORM        = 389,*/\
    HSF_R8G8B8X8_UINT,      HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8X8_UINT         = 390,*/\
    HSF_R8G8B8X8_SINT,      HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8X8_SINT         = 391,*/\
    HSF_R8G8B8X8_UNORM_SRGB,HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8X8_UNORM_SRGB   = 392,*/\
    HSF_R10G10B10X2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_RGBX, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R10G10B10X2_UNORM     = 393,*/\
    HSF_R16G16B16X16_UNORM, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_RGBX, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16X16_UNORM    = 394,*/\
    HSF_R16G16B16X16_SNORM, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_RGBX, 1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16X16_SNORM    = 395,*/\
    HSF_R16G16B16X16_FLOAT, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_RGBX, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16X16_FLOAT    = 396,*/\
    HSF_R16G16B16X16_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16X16_UINT     = 397,*/\
    HSF_R16G16B16X16_SINT,  HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16X16_SINT     = 398,*/\
    HSF_R32G32B32X32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32X32_FLOAT   = 399,*/\
    HSF_R32G32B32X32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32X32_UINT    = 400,*/\
    HSF_R32G32B32X32_SINT,  HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32X32_SINT    = 401,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,     D24_UNORM_S8_UINT,   CP_Z24,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_S8_UINT_D24_UNORM     = 402,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,     D24_UNORM_S8_UINT,   CP_Z24,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_D24_UNORM_S8_UINT     = 403,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,     D32_FLOAT_S8_UINT,   CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_S8_UINT_D32_FLOAT     = 404,*/\
    HSF_R24_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R24_UNORM             = 405,*/\
    HSF_R24G24_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R24G24_UNORM          = 406,*/\
    HSF_R24G24B24A24_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R24G24B24A24_UNORM    = 407,*/\
    HSF_R10G10B10A10_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10A10_UNORM    = 408,*/\
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*HSF_BC1_TYPELESS          = 409,*/\
    HSF_B8G8R8A8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_NONE, 0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,/*HSF_B8G8R8A8_TYPELESS     = 410,*/\
    HSF_B8G8R8X8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,/*HSF_B8G8R8X8_TYPELESS     = 411,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*HSF_BC2_TYPELESS          = 412,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*HSF_BC3_TYPELESS          = 413,*/\
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*HSF_BC4_TYPELESS          = 414,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*HSF_BC5_TYPELESS          = 415,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*DXGI_FORMAT_BC6H_TYPELESS = 416,*/\
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*HSF_BC7_TYPELESS          = 417,*/\
}

#endif

