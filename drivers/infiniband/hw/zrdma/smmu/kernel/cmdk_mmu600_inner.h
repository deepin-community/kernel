/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef _CMDK_MMU600_INNER_H_
#define _CMDK_MMU600_INNER_H_

#include "cmdk.h"
/**************************************************************************
 *                        Macro                                           *
 **************************************************************************/
/* TTFmt value */
#define PAGE_FORMAT_V8 (1)
#define PAGE_FORMAT_V7_LPAE (0)

/* udEndian value */
#define SMMU_TT_BIGENDIAN (1)
#define SMMU_TT_LITTLEENDIAN (0)

// TG
#define SMMU_CD_TG0_4K (0x0)
#define SMMU_CD_TG0_16K (0x2)
#define SMMU_CD_TG0_64K (0x1)
// ips
#define SMMU_CD_IPS_32 (0x0)
#define SMMU_CD_IPS_36 (0x1)
#define SMMU_CD_IPS_40 (0x2)
#define SMMU_CD_IPS_42 (0x3)
#define SMMU_CD_IPS_44 (0x4)
#define SMMU_CD_IPS_48 (0x5)

// manage L2/L3 pte
struct t_Map_Manage {
	u64 uddMaskedVa; /* L2 mask 2M, L3 mask 4k */
	u64 uddTTBaseAddr; /* Translation table base address */
	u32 udMapValid; /* map udValid */
	u32 udSteamIndex; /* belongs to stream */
};
#endif
