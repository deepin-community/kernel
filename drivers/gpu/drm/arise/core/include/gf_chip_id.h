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
#ifndef __GF_CHIP_ID_H
#define __GF_CHIP_ID_H


#define PCI_ID_EXC2UMA  0x3A01
#define PCI_ID_ELT1K    0x3200
#define PCI_ID_ELT2K    0x330F // Elite2000, TBD for real HW ID
//#define PCI_ID_ZX2000   0x322F // ZaoXin2000, TBD for real HW ID
#define PCI_ID_ZX2000   0x3600 //fpga read value
#define PCI_ID_EXC2CHX001   0x3A03
#define PCI_ID_EXC2CHX002   0x3A04
#define PCI_ID_ELT3K   0x3D00 //Elite3000
#define PCI_ID_ARISE   0x3D00 //ARISE10C0
#define PCI_ID_ARISE10C0T  0x3D06 //ARISE10C0T
#define PCI_ID_ARISE1020   0x3D02 //ARISE1020
#define PCI_ID_ARISE1040   0x3D03 //ARISE1040
#define PCI_ID_ARISE1010   0x3D04 //ARISE1010
#define PCI_ID_ARISE2030   0x3D07 //ARISE2030
#define PCI_ID_ARISE2020   0x3D08 //ARISE2020
#define PCI_ID_ARISE10D0   0x3D0E //ARISE10D0

#define PCI_ID_GENERIC_EXCALIBUR   PCI_ID_EXC2UMA
#define PCI_ID_GENERIC_ELITE       PCI_ID_ELT
#define PCI_ID_GENERIC_ELITE3K     PCI_ID_ELT3K

#define DEVICE_MASK     0xFF00
#define CHIP_MASK       0x00FF
#define DEVICE_ZX2000           (PCI_ID_ZX2000 & DEVICE_MASK)

#define DEVICE_EXC2UMA     (PCI_ID_EXC2UMA & DEVICE_MASK)
#define DEVICE_ELITE       (PCI_ID_ELT  & DEVICE_MASK)
#define DEVICE_ELITE1K     (PCI_ID_ELT1K  & DEVICE_MASK)
#define DEVICE_ELITE2K     (PCI_ID_ELT2K  & DEVICE_MASK)
#define DEVICE_CHX001     (PCI_ID_EXC2CHX001  & DEVICE_MASK)
#define DEVICE_CHX002     (PCI_ID_EXC2CHX002  & DEVICE_MASK)
#define DEVICE_ELITE3K     (PCI_ID_ELT3K  & DEVICE_MASK)

#define CHIP_ELITE        (PCI_ID_ELT   & CHIP_MASK)
#define CHIP_ELITE1K      (PCI_ID_ELT1K & CHIP_MASK)
#define CHIP_ELITE2K      (PCI_ID_ELT2K & CHIP_MASK)
#define CHIP_ZHAOXIN2000  (PCI_ID_ZX2000 & CHIP_MASK)

#define CHIP_EXCALIBUR2_UMA     (PCI_ID_EXC2UMA & CHIP_MASK)
#define CHIP_EXCALIBUR2_CHX001     (PCI_ID_EXC2CHX001 & CHIP_MASK)
#define CHIP_EXCALIBUR2_CHX002     (PCI_ID_EXC2CHX002 & CHIP_MASK)
#define CHIP_ELITE3K     (PCI_ID_ELT3K & CHIP_MASK)
#define CHIP_MASK_ARISE1020     (PCI_ID_ARISE1020 & CHIP_MASK)
#define CHIP_MASK_ARISE1040     (PCI_ID_ARISE1040 & CHIP_MASK)
#define CHIP_MASK_ARISE1010     (PCI_ID_ARISE1010 & CHIP_MASK)
#define CHIP_MASK_ARISE10C0T    (PCI_ID_ARISE10C0T & CHIP_MASK)
#define CHIP_MASK_ARISE2030     (PCI_ID_ARISE2030 & CHIP_MASK)
#define CHIP_MASK_ARISE2020     (PCI_ID_ARISE2020 & CHIP_MASK)
#define CHIP_MASK_ARISE10D0     (PCI_ID_ARISE10D0 & CHIP_MASK)

enum
{
    FAMILY_CMODEL,
    FAMILY_CLB,
    FAMILY_DST,
    FAMILY_CSR,
    FAMILY_INV,
    FAMILY_EXC,
    FAMILY_ELT,
    FAMILY_LAST,
};

enum
{
    CHIP_CMODEL,
    CHIP_CLB,
    CHIP_DST,
    CHIP_CSR,
    CHIP_INV,
    CHIP_H5,
    CHIP_H5S1,
    CHIP_H6S2,
    CHIP_CMS,
    CHIP_METRO,
    CHIP_MANHATTAN,
    CHIP_MATRIX,
    CHIP_DST2,
    CHIP_DST3,
    CHIP_DUMA,
    CHIP_H6S1,
    CHIP_DST4,
    CHIP_EXC1,      //Excalibur-1
    CHIP_E2UMA,     //E2UMA
    CHIP_ELT,       //Elite
    CHIP_ELT1K,     //Elite1k
    CHIP_ELT2K,     //Elite2k
    CHIP_ELT2K5,    //Elite2500
    CHIP_ZX2000,    //ZX2000
    CHIP_ELT3K,      //ELITE3K
    CHIP_ARISE=CHIP_ELT3K, //ARISE10C0
    CHIP_ARISE10C0T, //ARISE10C0T
    CHIP_ARISE1040, //ARISE1040
    CHIP_ARISE1020, //ARISE1020
    CHIP_ARISE1010, //ARISE1010
    CHIP_ARISE2030, //ARISE2030
    CHIP_ARISE2020, //ARISE2020
    CHIP_CHX001,    //CHX001
    CHIP_CHX002,    //CHX002
    CHIP_ZX2100,    //ZX2100
    CHIP_LAST,      //Maximum number of chips supported.
};


#endif /*__GF_CHIP_ID_H*/



