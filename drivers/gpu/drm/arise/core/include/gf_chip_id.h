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
#define PCI_ID_ARISE   0x3D00 //ARISE
#define PCI_ID_ARISE1020   0x3D02 //ARISE
#define PCI_ID_716     0x0012

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
#define DEVICE_716         PCI_ID_716

#define CHIP_ELITE        (PCI_ID_ELT   & CHIP_MASK)
#define CHIP_ELITE1K      (PCI_ID_ELT1K & CHIP_MASK)
#define CHIP_ELITE2K      (PCI_ID_ELT2K & CHIP_MASK)
#define CHIP_ZHAOXIN2000  (PCI_ID_ZX2000 & CHIP_MASK)

#define CHIP_EXCALIBUR2_UMA     (PCI_ID_EXC2UMA & CHIP_MASK)
#define CHIP_EXCALIBUR2_CHX001     (PCI_ID_EXC2CHX001 & CHIP_MASK)
#define CHIP_EXCALIBUR2_CHX002     (PCI_ID_EXC2CHX002 & CHIP_MASK)
#define CHIP_ELITE3K     (PCI_ID_ELT3K & CHIP_MASK)
#define CHIP_MASK_ARISE1020     (PCI_ID_ARISE1020 & CHIP_MASK)

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
    CHIP_ARISE=CHIP_ELT3K, //ARISE
    CHIP_ARISE1020, //ARISE1020
    CHIP_CHX001,    //CHX001
    CHIP_CHX002,    //CHX002
    CHIP_ZX2100,    //ZX2100
    CHIP_LAST,      //Maximum number of chips supported.
};


#endif /*__GF_CHIP_ID_H*/



