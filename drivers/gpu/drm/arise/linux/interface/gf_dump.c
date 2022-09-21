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

#include <stdio.h>
#include <string.h>
#include "gf_bufmgr.h"

#pragma pack(push, 1)

typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int   U32;

typedef struct tagBITMAPFILEHEADER {
    U16 bfType;    
    U32 bfSize;
    U16 bfReserved1;
    U16 bfReserved2;
    U32 bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    U32 biSize;
    U32 biWidth;
    U32 biHeight;
    U16 biPlanes;
    U16 biBitCount;
    U32 biCompression;
    U32 biSizeImage;
    U32 biXPelsPerMeter;
    U32 biYPelsPerMeter;
    U32 biClrUsed;
    U32 biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    U8 rgbBlue;
    U8 rgbGreen;
    U8 rgbRed;
    U8 rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO;

typedef struct tagBITMAP
{
    BITMAPFILEHEADER bfHeader;
    BITMAPINFO biInfo;
}BITMAPFILE;
#pragma pack(pop)

int gf_bo_dump_bmp(gf_bo_t *bo, const char *filename)
{
    int ret = 0;
    int row = 0;
    map_args_t map_arg = {0, };
    FILE *fp = fopen(filename, "wb");
    RGBQUAD *ct = NULL;
    int ctnum = 0;

    if (!fp)
        return 0;

    map_arg.flags.read_only = 1;
    map_arg.flags.acquire_aperture = bo->tiled;
    if (0 != gf_bo_map(bo, &map_arg))
        goto done;

    if (!map_arg.virt_addr)
        goto done;

    switch (bo->bit_cnt)
    {
    case 8:
        ctnum = 256;
        break;
    case 4:
        ctnum = 16;
        break;
    case 1:
        ctnum = 2;
        break;
    }

    if (ctnum > 0)
    {
        int i;
        ct = (RGBQUAD*)malloc(sizeof(*ct) * ctnum);
        for (i = 0; i < ctnum; i++)
        {
            ct[i].rgbBlue = ct[i].rgbGreen = ct[i].rgbRed = 255 * i / (ctnum - 1);
            ct[i].rgbReserved = 0;
        }
    }

    unsigned int bmppitch = ((bo->width * bo->bit_cnt + 31) >> 5) << 2;
    unsigned int filesize = bmppitch * bo->height;

    BITMAPFILE bmpfile = {0, };
    bmpfile.bfHeader.bfType = 0x4D42;
    bmpfile.bfHeader.bfSize = filesize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(*ct) * ctnum;
    bmpfile.bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(*ct) * ctnum;
    bmpfile.biInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpfile.biInfo.bmiHeader.biWidth = bo->width;
    bmpfile.biInfo.bmiHeader.biHeight = bo->height;
    bmpfile.biInfo.bmiHeader.biPlanes = 1;
    bmpfile.biInfo.bmiHeader.biBitCount = bo->bit_cnt;
    bmpfile.biInfo.bmiHeader.biCompression = 0;

    fwrite(&(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER), 1, fp);
    if (ct)
    {
        fwrite(ct, sizeof(*ct), ctnum, fp);
    }

    unsigned char *p = (unsigned char*)map_arg.virt_addr;
    p = p + bo->pitch * (bo->height - 1);
    for (row = 0; row < bo->height; row++)
    {
        fwrite(p, bmppitch, 1, fp);
        p -= bo->pitch;
    }

done:
    if (map_arg.virt_addr)
    {
        gf_bo_unmap(bo);
    }
    if (ct)
    {
        free(ct);
    }
    fclose(fp);
    return ret;
}


