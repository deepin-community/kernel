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

#ifndef _REGISTER_DEF_H
#define _REGISTER_DEF_H

#include     "BlockID.h"
#include      "CSP_GLOBAL_Register.h"
#include     "CSP_OPCODE.h"
#include      "TASFE_reg.h"
#include      "TASBE_reg.h"
#include      "EU_FS_reg.h"
#include      "EU_CS_reg.h"
#include      "IU_reg.h"
#include      "SPIN_register.h"
#include      "SPOUT_register.h"
#include      "EU_PS_reg.h"
#include      "FF_registers.h"
#include      "TU_Reg.h"
#include      "GPCPBE_register.h"
#include      "GPCPFE_register.h"
#include      "MMU_registers.h"
#include      "MXU_registers.h"
#include      "MIU_reg.h"
#include      "Reg_BusyStatus_elt.h"
#include      "Vcp_Registers.h"
#include	  "VPP_MMIO_registers.h"
#include      "WLS_Registers.h"
#include      "L2_Register.h"
#include      "MCE_reg.h"
#include     "VCP_OPCODE_DECOUPLE.h"



typedef struct _EliteRegs
{
    Csp_Global_regs					 Regs_CSP;
    Eu_Fs_regs                       Regs_EU_VS;
    Eu_Ps_regs                       Regs_EU_PS;
    Tasfe_regs                         Regs_TASFE;
    Iu_regs                          Regs_IU;
    Tu_regs                          Regs_TU;
    Ff_regs                          Regs_FF;
    Mxu_regs                         Regs_MXU;
    Vpp_Mmio_regs					 Regs_VPP;
} EliteRegs;

#define MMIO_FLAG 0x000FF000
#define MMIO_MIU_START_ADDRESS                  0x00008000
#define MMIO_MIU_DYNAMIC_FB_START_ADDRESS       0x00008B00
#define MMIO_VPP_START_ADDRESS					0x0000B000
#define MMIO_CSP_START_ADDRESS                  0x00030000
#define MMIO_MMU_START_ADDRESS                  0x00050000
#define MMIO_MXU_START_ADDRESS                  0x00049000
#define MMIO_VCP0_START_ADDRESS			        0x0004A000
#define MMIO_VCP1_START_ADDRESS			        0x0004B000


#define MMIO_EU_START_ADDRESS                   (MMIO_CSP_START_ADDRESS|(Reg_Eu_Dbg_Cfg_Offset<<2))
#define MMIO_EU_END_ADDRESS                     (MMIO_CSP_START_ADDRESS|(CSP_GLOBAL_REG_LIMIT<<2))

#define MMIO_PMU_START_ADDRESS                  (0x00080000)
#define PMU_SW_C3D_PVAL_ADDR_OFFSET             (0x50)
#define PMU_SW_C3D_VAL_ADDR_OFFSET              (0x34)
#define PMU_SW_PLL_LOCK_ADDR                    (0x48)

#define MMIO_MXU_RANGETBL_MASK  0xC00
#define MMIO_MXU_RANGETBL_VALUE 0x400

#define MMIO_MXU_PAGEKEY_START 0x400
#define MMIO_MXU_PAGEKEY_MASK  0xC00

#define MIU_DYNAMIC_FB_ENTRY_SIZE 0x200000


#define FF_ZCLIPMIN_OFFSET 0
#define FF_ZCLIPMAX_OFFSET 1

#define FF_SFRONTFACE_OFFSET 0
#define FF_SBACKFACE_OFFSET  1

#define FF_RT_ADDR_OFFSET 0
#define FF_RT_DEPTH_OFFSET 1
#define FF_RT_VIEW_CTRL_OFFSET 3

#define FF_RT_DESC_OFFSET 0
#define FF_RT_FMT_OFFSET  0
#define FF_RT_SIZE_OFFSET 1
#define FF_RT_MISC_OFFSET 2


#define FF_BS_BLEND_CTL_OFFSET 6

#define FF_RT_CTRL_REG_SIZE (sizeof(Reg_Rt_Ctrl_Group)>>2)
#define FF_RT_ADDR_CTRL_REG_SIZE (sizeof(Reg_Rt_Addr_Ctrl_Group)>>2)
#define FF_RT_DST 0
#define FF_RT_SRC 1

#define FF_16FL_COLOR_OFFSET     0
#define FF_16FH_COLOR_OFFSET     1
#define FF_8888_COLOR_OFFSET     2
#define FF_2101010L_COLOR_OFFSET 3
#define FF_2101010H_COLOR_OFFSET 4

#define FF_FG_COLOR_OFFSET 0
#define FF_BG_COLOR_OFFSET 1

#endif
