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

#ifndef __VIDSCH_3DBLT_E3K_H
#define __VIDSCH_3DBLT_E3K_H

#include "vidmm.h"
#include "context.h"
#include "chip_include_e3k.h"
#include "util.h"

typedef struct _BITBLT_REGSETTING_E3K
{
    DWORD                   SetDstRTAddrCtrl;      // set Dst RT Addr Ctrl Register 3 DWORD
    Reg_Rt_Addr             DstAddr;
    Reg_Rt_Depth            DstDepth;
    Reg_Rt_View_Ctrl        DstViewCtrl;
    DWORD                   SetDstRTCtrl;         // set Dst RT Ctrl Register 3 DWORD
    Reg_Rt_Fmt              DstFormat;
    Reg_Rt_Size             DstSize;
    Reg_Rt_Misc             DstMisc;              // might need disable D_Read_En, disable alphablend, need to disable Rop

    DWORD                   SetSrcRTAddrCtrl;      // set Dst RT Addr Ctrl Register 3 DWORD
    Reg_Rt_Addr             SrcAddr;
    Reg_Rt_Depth            SrcDepth;
    Reg_Rt_View_Ctrl        SrcViewCtrl;
    DWORD                   SetSrcRTCtrl;       // set Dst RT Ctrl Register 3 DWORD
    Reg_Rt_Fmt              SrcFormat;
    Reg_Rt_Size             SrcSize;
    Reg_Rt_Misc             SrcMisc;

    DWORD                   Set_Rast_Ctrl;
    Reg_Rast_Ctrl           Rast_Ctrl;

    DWORD                   Set_Iu_Ctrl_Ex;
    Reg_Iu_Ctrl_Ex          reg_Iu_Ctrl_Ex;

    DWORD                   Set_Tasfe_Ctrl;
    Reg_Tasfe_Ctrl          reg_Tasfe_Ctrl;

    DWORD                   Set_Resolution_Ctrl;
    Reg_Resolution_Ctrl     reg_Resolution_Ctrl;

    DWORD                   Set_Dzs_Ctrl;
    Reg_Dzs_Ctrl            Dzs_Ctrl;

    DWORD                   Set_Zs_Req_Ctrl;
    Reg_Zs_Req_Ctrl            Zs_Req_Ctrl;
    
    DWORD                   Set_Blend_Ctrl;
    Reg_Blend_Ctl_Group            reg_Blend_Ctl;

    Cmd_Block_Command_Sg                   BitBltCmd;
    Cmd_Block_Command_Sg_Dword0            RectX;
    Cmd_Block_Command_Sg_Dword1            RectY;
    Cmd_Block_Command_Sg_Blt_Dword2        SrcDxDy;

    Cmd_Block_Command_Flush D_Flush;
    Cmd_Block_Command_Flush D_Invalidate;

    Csp_Opcodes_cmd         FenceCmd;
    DWORD                   FenceValue;
    Csp_Opcodes_cmd         WaitCmd;
    Csp_Opcodes_cmd         WaitMainCmd;
} BITBLT_REGSETTING_E3K;

typedef struct _IMMBLT_REGSETTING_E3K
{
    DWORD                   SetDstRTAddrCtrl;      // set Dst RT Addr Ctrl Register 3 DWORD
    Reg_Rt_Addr             DstAddr;
    Reg_Rt_Depth            DstDepth;
    Reg_Rt_View_Ctrl        DstViewCtrl;
    DWORD                   SetDstRTCtrl;         // set Dst RT Ctrl Register 3 DWORD
    Reg_Rt_Fmt              DstFormat;
    Reg_Rt_Size             DstSize;
    Reg_Rt_Misc             DstMisc;              // might need disable D_Read_En, disable alphablend, need to disable Rop

    DWORD                   Set_Zs_Req_Ctrl;
    Reg_Zs_Req_Ctrl         Zs_Req_Ctrl;        // might need disable Zl2_End_Pipe_En

    DWORD                   Set_Rast_Ctrl;
    Reg_Rast_Ctrl           Rast_Ctrl;

    DWORD                   Set_Tasfe_Ctrl;
    Reg_Tasfe_Ctrl          reg_Tasfe_Ctrl;

    DWORD                   Set_Resolution_Ctrl;
    Reg_Resolution_Ctrl     reg_Resolution_Ctrl;

    DWORD                   Set_Dzs_Ctrl;
    Reg_Dzs_Ctrl            Dzs_Ctrl;

    DWORD                   Set_Ff_Glb_Ctrl;
    Reg_Ff_Glb_Ctrl         Ff_Glb_Ctrl;

    Cmd_Block_Command_Img_Trn                   IMMBltCmd;
    Cmd_Block_Command_Img_Trn_Dword0            RectX;
    Cmd_Block_Command_Img_Trn_Dword1            RectY;
} IMMBLT_REGSETTING_E3K;

typedef struct _FASTCLEAR_REGSETTING_E3K
{
    DWORD                   SetDstRTAddrCtrl;      // set Dst RT Addr Ctrl Register 3 DWORD
    Reg_Rt_Addr             DstAddr;
    Reg_Rt_Depth            DstDepth;
    Reg_Rt_View_Ctrl        DstViewCtrl;
    DWORD                   SetDstRTCtrl;         // set Dst RT Ctrl Register 3 DWORD
    Reg_Rt_Fmt              DstFormat;
    Reg_Rt_Size             DstSize;
    Reg_Rt_Misc             DstMisc;              // might need disable D_Read_En, disable alphablend, need to disable Rop

    DWORD                   SetOtherRTDisable[14];

    DWORD                   Set_Zs_Req_Ctrl;
    Reg_Zs_Req_Ctrl         Zs_Req_Ctrl;        // might need disable Zl2_End_Pipe_En

    DWORD                   Set_Rast_Ctrl;
    Reg_Rast_Ctrl           Rast_Ctrl;
    
    DWORD                   Set_Iu_Ctrl_Ex;
    Reg_Iu_Ctrl_Ex          reg_Iu_Ctrl_Ex;

    DWORD                   Set_Tasfe_Ctrl;
    Reg_Tasfe_Ctrl          reg_Tasfe_Ctrl;

    DWORD                   Set_Resolution_Ctrl;
    Reg_Resolution_Ctrl     reg_Resolution_Ctrl;

    DWORD                   Set_Dzs_Ctrl;
    Reg_Dzs_Ctrl            Dzs_Ctrl;

    DWORD                   Set_Ff_Glb_Ctrl;
    Reg_Ff_Glb_Ctrl         Ff_Glb_Ctrl;

    DWORD                   Set_Fast_Clear_Color;
    Reg_Fast_Clear_Color    Fast_Clear_Color0;
    Reg_Fast_Clear_Color    Fast_Clear_Color1;
    Reg_Fast_Clear_Color    Fast_Clear_Color2;
    Reg_Fast_Clear_Color    Fast_Clear_Color3;

    Cmd_Block_Command_Sg                   FastClearCmd;
    Cmd_Block_Command_Sg_Dword0            RectX;
    Cmd_Block_Command_Sg_Dword1            RectY;
} FASTCLEAR_REGSETTING_E3K;

#endif

