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
#ifndef _STM_CONTEXT_E3K_H
#define _STM_CONTEXT_E3K_H

#ifndef __linux__
#define CACHE_ALIGN __declspec(align(64))
#else
#define CACHE_ALIGN __attribute__((aligned(64)))
#endif

#include "./Chip/registerDef.h"

typedef struct _Mxu_Context_regs
{
    Reg_Fb_Ctrl                      reg_Fb_Ctrl;
    Reg_Mmu_Mode                     reg_Mmu_Mode;
//    Reg_Vbl_Base                     reg_Vbl_Base;
//    Reg_Pbl_Base                     reg_Pbl_Base;
    Reg_Gart_Base                    reg_Gart_Base;
    Reg_Proc_Sec                     reg_Proc_Sec;
    Reg_Pt_Inv_Addr                  reg_Pt_Inv_Addr;
    Reg_Pt_Inv_Mask                  reg_Pt_Inv_Mask;
    Reg_Pt_Inv_Trig                  reg_Pt_Inv_Trig;
    Reg_Mxu_Channel_Control          reg_Mxu_Channel_Control;
    Reg_Mxu_Dec_Ctrl                 reg_Mxu_Dec_Ctrl;
    Reg_Bus_Id_Mode                  reg_Bus_Id_Mode;
    Reg_Mxu_Ctrl_0                   reg_Mxu_Ctrl_0;
} Mxu_Context_regs;

typedef struct CONTEXT_REGISTER_BUFFER_E3K
{
    int todo;
} CONTEXT_REGISTER_BUFFER_E3K;

#define MAX_HW_SLICE_NUM                8
#define SLICE_ILA_COUNTERS_ALIGN_SIZE   32
#define FENCE_COUNTER_NUM               8

#define HW_MAX_GPC_NUM_E3K               3

typedef struct _CSP_CONTEXTBUFFER_REGS
{
    Reg_Csp_Ms_Total_Gpu_Timestamp   reg_Csp_Ms_Total_Gpu_Timestamp[2];
    Reg_Csp_Ms_Total_Busy_Time       reg_Csp_Ms_Total_Busy_Time[2];
    Reg_Csp_Ms_Query_Occlusion       reg_Csp_Ms_Query_Occlusion[2];
    Reg_Ia_Vertices_Cnt              reg_Ia_Vertices_Cnt[2];
    Reg_Ia_Primitives_Cnt            reg_Ia_Primitives_Cnt[2];
    Reg_Ia_Ogl_Cut_Flag_32           reg_Ia_Ogl_Cut_Flag_32;
    Reg_Ia_Vb0_Offset                reg_Ia_Vb0_Offset;
    Reg_Ia_Vb0_Stride                reg_Ia_Vb0_Stride;
    Reg_Ia_Bufferfilledsize          reg_Ia_Bufferfilledsize;
    Reg_Ia_Batch_Cfg                 reg_Ia_Batch_Cfg;
    Reg_Predicate_Status             reg_Predicate_Status;
    Reg_Csp_Misc_Control             reg_Csp_Misc_Control;
    Reg_Tbr_Render_Mode_Ctrl         reg_Tbr_Render_Mode_Ctrl;
    Reg_Tbr_Frametimestamp_Lth       reg_Tbr_Frametimestamp_Lth;
    Reg_Tbr_Frametimestamp_Hth       reg_Tbr_Frametimestamp_Hth;
    Reg_Tbr_Traffic_Th               reg_Tbr_Traffic_Th;
    Reg_Tbr_Event_Count_Ref          reg_Tbr_Event_Count_Ref;
    Reg_Descriptor_Base_Addr_Group   reg_Descriptor_Base_Addr[8];
    Reg_Csp_Fence_Counter_Group      reg_Csp_Fence_Counter[FENCE_COUNTER_NUM];
} CSP_CONTEXTBUFFER_REGS;
/**********************************here is the definition for all the state registers**************************************/
// Gpcpfe_Shadow_Reg + GPCPFE_CONTEXTBUFFER_REGS = full Gpcpfe_regs
typedef struct _Gpcpfe_Shadow_Reg
{
    Reg_Gpcpfe_Instance_Group        reg_Gpcpfe_Instance[32];
    Reg_Gpcpfe_Inst_Mask             reg_Gpcpfe_Inst_Mask;
    Reg_Gpcpfe_Reserved              reg_Gpcpfe_Reserved[7];
} Gpcpfe_Shadow_Reg;

typedef struct _GPCPFE_CONTEXTBUFFER_REGS
{
    Reg_Gpcpfe_Iidcnt_Group          reg_Gpcpfe_Iidcnt[32];
    Reg_Gpcpfe_Iid                   reg_Gpcpfe_Iid;
    Reg_Gpcpfe_Pid                   reg_Gpcpfe_Pid;
    Reg_Gpcpfe_Gp_Global_Group       reg_Gpcpfe_Gp_Global;
    Reg_Gpcpfe_Gp_Group_Group        reg_Gpcpfe_Gp_Group;
}GPCPFE_CONTEXTBUFFER_REGS;

typedef struct _EUFS_State_Reg
{
    Reg_Eu_Full_Glb                  reg_Eu_Full_Glb;
    Reg_Eu_3d_Glb                    reg_Eu_3d_Glb;
    Reg_Eu_Fe_Glb                    reg_Eu_Fe_Glb;
    Reg_Mv_Cfg                       reg_Mv_Cfg;
    Reg_Vc_Size_Cfg0                 reg_Vc_Size_Cfg0;
    Reg_Vc_Size_Cfg1                 reg_Vc_Size_Cfg1;
    Reg_Vc_Base_Cfg0                 reg_Vc_Base_Cfg0;
    Reg_Vc_Base_Cfg1                 reg_Vc_Base_Cfg1;
    Reg_Fs_Cfg                       reg_Fs_Cfg;
    Reg_Max_Threads_Cfg              reg_Max_Threads_Cfg;
    Reg_Antilock_Cfg                 reg_Antilock_Cfg;
    Reg_Vs_Ctrl                      reg_Vs_Ctrl;
    Reg_Hs_Ctrl                      reg_Hs_Ctrl;
    Reg_Hs_In_Cfg                    reg_Hs_In_Cfg;
    Reg_Hs_Out_Cfg                   reg_Hs_Out_Cfg;
    Reg_Ds_Ctrl                      reg_Ds_Ctrl;
    Reg_Ds_In_Cfg                    reg_Ds_In_Cfg;
    Reg_Gs_Ctrl                      reg_Gs_Ctrl;
    Reg_Gs_In_Cfg                    reg_Gs_In_Cfg;
    Reg_Gs_Out_Cfg                   reg_Gs_Out_Cfg;
    Reg_Ts_Ctrl                      reg_Ts_Ctrl;
    Reg_Ts_Input_Mapping0            reg_Ts_Input_Mapping0;
    Reg_Ts_Input_Mapping1            reg_Ts_Input_Mapping1;
    Reg_Ts_Factors_Const             reg_Ts_Factors_Const[6];
    Reg_Fs_Cs_Ctrl                   reg_Fs_Cs_Ctrl;
    Reg_Fs_Cs_Sm_Cfg                 reg_Fs_Cs_Sm_Cfg;
    Reg_Fs_Out_Loc0                  reg_Fs_Out_Loc0;
    Reg_Fs_Out_Loc1                  reg_Fs_Out_Loc1;
    Reg_Fs_Out_Mapping               reg_Fs_Out_Mapping[12];
    Reg_Fs_Out_Comp                  reg_Fs_Out_Comp[3];
    Reg_Fs_Out_Mask                  reg_Fs_Out_Mask[6];
    Reg_Fs_Pm_Cfg                    reg_Fs_Pm_Cfg;
    Reg_Fs_Pm_Id                     reg_Fs_Pm_Id;
    Reg_Vgs_Pm_Range                 reg_Vgs_Pm_Range;
    Reg_Hds_Pm_Range                 reg_Hds_Pm_Range;
    Reg_Fs_U_Enable                  reg_Fs_U_Enable[4];
    Reg_Fs_U_Fmt                     reg_Fs_U_Fmt[16];
    Reg_Fs_U_Layout                  reg_Fs_U_Layout[4];
    Reg_Vcs_U_Cfg                    reg_Vcs_U_Cfg;
    Reg_Hs_U_Cfg                     reg_Hs_U_Cfg;
    Reg_Ds_U_Cfg                     reg_Ds_U_Cfg;
    Reg_Gs_U_Cfg                     reg_Gs_U_Cfg;
    Reg_Vcs_Instr0                   reg_Vcs_Instr0;
    Reg_Vcs_Instr1                   reg_Vcs_Instr1;
    Reg_Vcs_Instr_Range              reg_Vcs_Instr_Range;
    Reg_Hs_Instr0                    reg_Hs_Instr0;
    Reg_Hs_Instr1                    reg_Hs_Instr1;
    Reg_Hs_Instr_Range               reg_Hs_Instr_Range;
    Reg_Ds_Instr0                    reg_Ds_Instr0;
    Reg_Ds_Instr1                    reg_Ds_Instr1;
    Reg_Ds_Instr_Range               reg_Ds_Instr_Range;
    Reg_Gs_Instr0                    reg_Gs_Instr0;
    Reg_Gs_Instr1                    reg_Gs_Instr1;
    Reg_Gs_Instr_Range               reg_Gs_Instr_Range;
    Reg_Vcs_Cb_Cfg                   reg_Vcs_Cb_Cfg;
    Reg_Hs_Cb_Cfg                    reg_Hs_Cb_Cfg;
    Reg_Ds_Cb_Cfg                    reg_Ds_Cb_Cfg;
    Reg_Gs_Cb_Cfg                    reg_Gs_Cb_Cfg;
    Reg_Fs_Cb_Cfg                    reg_Fs_Cb_Cfg;
    Reg_Fs_Rev_8aligned              reg_Fs_Rev_8aligned;

} EUFS_State_Reg;

typedef struct _EU_CB_Reg
{
    Reg_Fs_Cb_Data                   reg_Fs_Cb_Data[2048];
} EU_CB_Reg;

typedef struct _EUPS_State_Reg
{
    Reg_Ps_Cfg                       reg_Ps_Cfg;
    Reg_Ps_Ctrl                      reg_Ps_Ctrl;
    Reg_Ps_Out_Cfg                   reg_Ps_Out_Cfg;
    Reg_Ps_Doutbuf_Cfg               reg_Ps_Doutbuf_Cfg;
    Reg_Ps_Pm_Cfg                    reg_Ps_Pm_Cfg;
    Reg_Ps_Pm_Id                     reg_Ps_Pm_Id;
    Reg_Ps_Pm_Range                  reg_Ps_Pm_Range;
    Reg_Ps_Out_Mapping               reg_Ps_Out_Mapping;
    Reg_Ps_Out_Fmt                   reg_Ps_Out_Fmt;
    Reg_Ps_U_Enable                  reg_Ps_U_Enable[2];
    Reg_Ps_U_Fmt                     reg_Ps_U_Fmt[8];
    Reg_Ps_U_Layout                  reg_Ps_U_Layout[2];
    Reg_Ps_U_Cfg                     reg_Ps_U_Cfg;
    Reg_Ps_Instr0                    reg_Ps_Instr0;
    Reg_Ps_Instr1                    reg_Ps_Instr1;
    Reg_Ps_Instr_Range               reg_Ps_Instr_Range;
    Reg_Ps_Cb_Cfg                    reg_Ps_Cb_Cfg;
    Reg_Ps_Rev_8aligned              reg_Ps_Rev_8aligned[6];
    //add reserve reg to algin each each block's shadow to 8dword per line
} EUPS_State_Reg;

typedef struct _WLS_Usharp_Reg
{
    Reg_Uav_Group_Group              reg_Uav_Group[128];

} WLS_Usharp_Reg;

typedef struct _WLS_State_Reg
{
    Reg_Ffc_Ubuf_Golbalconfig        reg_Ffc_Ubuf_Golbalconfig;
    Reg_Ffc_Ubuf_3dconfig            reg_Ffc_Ubuf_3dconfig;
    Reg_Descriptor_Heap_Base         reg_Descriptor_Heap_Base[8];
    Reg_Wls_Reserved0                reg_Wls_Reserved0;
    Reg_Wls_Reserved1                reg_Wls_Reserved1;
    Reg_Wls_Reserved2                reg_Wls_Reserved2;
    Reg_Wls_Reserved3                reg_Wls_Reserved3;
    Reg_Wls_Reserved4                reg_Wls_Reserved4;
    Reg_Wls_Reserved5                reg_Wls_Reserved5;

} WLS_State_Reg;

typedef struct _TU_State_Reg
{
    Reg_Tu_Tssharp_Ctrl              reg_Tu_Tssharp_Ctrl;
    Reg_Tu_Tssharp_3d_Ctrl           reg_Tu_Tssharp_3d_Ctrl;
    Reg_Tu_Tssharp_Heap_Base0        reg_Tu_Tssharp_Heap_Base0;
    Reg_Tu_Tssharp_Heap_Base1        reg_Tu_Tssharp_Heap_Base1;
    Reg_Tu_Tssharp_Heap_Base2        reg_Tu_Tssharp_Heap_Base2;
    Reg_Tu_Tssharp_Heap_Base3        reg_Tu_Tssharp_Heap_Base3;
    Reg_Tu_Tssharp_Heap_Base4        reg_Tu_Tssharp_Heap_Base4;
    Reg_Tu_Tssharp_Heap_Base5        reg_Tu_Tssharp_Heap_Base5;
    Reg_Tu_Tssharp_Heap_Base6        reg_Tu_Tssharp_Heap_Base6;
    Reg_Tu_Tssharp_Heap_Base7        reg_Tu_Tssharp_Heap_Base7;
    Reg_Tu_Reserved1                 reg_Tu_Reserved1[6];
    Reg_Tu_Control_Fe                reg_Tu_Control_Fe;
    Reg_Tu_Si_Ctl                    reg_Tu_Si_Ctl;
    Reg_Tu_Vs_Tsharp_Ctrl            reg_Tu_Vs_Tsharp_Ctrl;
    Reg_Tu_Vs_Ssharp_Ctrl            reg_Tu_Vs_Ssharp_Ctrl;
    Reg_Tu_Hs_Tsharp_Ctrl            reg_Tu_Hs_Tsharp_Ctrl;
    Reg_Tu_Hs_Ssharp_Ctrl            reg_Tu_Hs_Ssharp_Ctrl;
    Reg_Tu_Ds_Tsharp_Ctrl            reg_Tu_Ds_Tsharp_Ctrl;
    Reg_Tu_Ds_Ssharp_Ctrl            reg_Tu_Ds_Ssharp_Ctrl;
    Reg_Tu_Gs_Tsharp_Ctrl            reg_Tu_Gs_Tsharp_Ctrl;
    Reg_Tu_Gs_Ssharp_Ctrl            reg_Tu_Gs_Ssharp_Ctrl;
    Reg_Tu_Reserved2                 reg_Tu_Reserved2[6];
    Reg_Tu_Vb_Group                  reg_Tu_Vb[32];
    Reg_Tu_Ve_Group                  reg_Tu_Ve[32];
    Reg_Tu_Instance_Group            reg_Tu_Instance[32];
    Reg_Tu_Control_Be                reg_Tu_Control_Be;
    Reg_Tu_Ps_Tsharp_Ctrl            reg_Tu_Ps_Tsharp_Ctrl;
    Reg_Tu_Ps_Ssharp_Ctrl            reg_Tu_Ps_Ssharp_Ctrl;
    Reg_Tu_Reserved3                 reg_Tu_Reserved3[5];


} TU_State_Reg;

typedef struct _Tsharp_Reg
{
    Reg_Tu_Ts_Sharp_Group            reg_Tu_Ts_Sharp[192];

} Tsharp_Reg;

typedef struct CSP_REG_FLAG
{
    DWORD dirty_flag        : 1;
}CSP_REG_FLAG;

/*****************************************here is the definition for flag buffer*******************************************/
typedef struct _Shadow_FE_BE_Flag
{
    CSP_REG_FLAG    ff_regs_flag[FF_REG_END];
    CSP_REG_FLAG    gpcpfe_regs_flag[GPCPFE_REG_END];
    CSP_REG_FLAG    spin_regs_flag[SPIN_REG_END];
    CSP_REG_FLAG    eu_fs_regs_flag[Reg_Fs_Rev_Cb_Offset];
    CSP_REG_FLAG    eu_ps_regs_flag[Reg_Ps_Rev_Cb_Offset];
    CSP_REG_FLAG    eu_cb_regs_flag[Reg_Dbg_Cfg_Offset - Reg_Fs_Cb_Data_Offset];
    CSP_REG_FLAG    tasfe_regs_flag[TASFE_REG_END];
    CSP_REG_FLAG    iu_regs_flag[IU_REG_END];
    CSP_REG_FLAG    wls_regs_flag[Reg_Ffc_Ubuf_Csconfig_Offset];
    CSP_REG_FLAG    wls_usharp_flag[WLS_FE_REG_END - Reg_Uav_Group_Offset];
    CSP_REG_FLAG    tu_regs_flag[Reg_Tu_Tssharp_Cs_Ctrl_Offset];
    CSP_REG_FLAG    tu_tsharp_flag[TUFE_REG_END - Reg_Tu_Ts_Sharp_Offset];
    //CSP_REG_FLAG    gpcpbe_regs_flag[GPCPBE_REG_END];
    CSP_REG_FLAG    spout_regs_flag[SPOUT_REG_END];
    CSP_REG_FLAG    l2_regs_flag[L2_REG_END];
} Shadow_FE_BE_Flag;

typedef struct _Shadow_FE_BE_Reg
{
    Ff_regs             ff_regs;
    Gpcpfe_Shadow_Reg   gpcpfe_regs;
    Spin_regs           spin_regs;
    EUFS_State_Reg      eufs_regs;
    EUPS_State_Reg      eups_regs;
    EU_CB_Reg           eucb_regs;
    Tasfe_regs          tasfe_regs;
    Iu_regs             iu_regs;
    WLS_State_Reg       wls_regs;
    WLS_Usharp_Reg      usharp_regs;
    TU_State_Reg        tu_regs;
    Tsharp_Reg          tsharp_regs;
    //Gpcpbe_regs       gpcpbe_regs;
    Spout_regs          spout_regs;
    L2_regs             l2_regs;
} Shadow_FE_BE_Reg;

typedef struct _CSP_3D_ShadowBuf
{
    Shadow_FE_BE_Reg    fe_be_regs;
} CSP_3D_ShadowBuf;

typedef struct _CSP_3D_FlagBuf// for 3D shadow buffer, when update, should compare with the original value to see whether to set the dirty into true
{
    Shadow_FE_BE_Flag   fe_be_flag;
} CSP_3D_FlagBuf;

typedef struct _CSP_3D_ContextBuf
{
    CSP_3D_ShadowBuf    shadow_buf_3D;
    CSP_3D_FlagBuf      flag_buf_3D;

}CSP_3D_ContextBuf;

typedef CACHE_ALIGN struct _CONTEXT_BUFFER_E3K
{
    CACHE_ALIGN CSP_3D_ContextBuf         HWContextbuffer;
    /*CACHE_ALIGN*/ CSP_CONTEXTBUFFER_REGS    CSPRegs;
    /*CACHE_ALIGN*/ GPCPFE_CONTEXTBUFFER_REGS GpcpFeRegs;
    /*CACHE_ALIGN*/ Gpcpbe_regs               GpcpBeRegs;
    DWORD GpcTopRegs[HW_MAX_GPC_NUM_E3K][40];
}CONTEXT_BUFFER_E3K;

#undef CACHE_ALIGN

#define CONTEXT_BUFFER_ALIGN  (0x4000)  // Align to 16K for HW requirement
#define CONTEXT_BUFFER_SIZE    \
        ((sizeof(CONTEXT_BUFFER_E3K) + (CONTEXT_BUFFER_ALIGN -1)) & (~(CONTEXT_BUFFER_ALIGN -1)))

#define REGISTER_BUFFER_SIZE sizeof(CONTEXT_REGISTER_BUFFER_E3K)

#define GPCPBE_QUERY_REGISTERS  (sizeof(Gpcpbe_regs) % 0x40 ? (sizeof(Gpcpbe_regs) / 0x40) + 1 :(sizeof(Gpcpbe_regs) / 0x40))

#define SAVE_RESTORE_DMA_SIZE (sizeof(RB_PREDEFINE_DMA) + (0x1000 - 1) & (~(0x1000 -1)))

typedef struct _CONTEXT_RESTORE_DMA_E3K
{
    Cmd_Blk_Cmd_Csp_Save_Rsto   RestoreContext;
    DWORD                       ContextBufferOffset_L;
    DWORD                       ContextBufferOffset_H;

    Csp_Opcodes_cmd             RestoreEuFullGlb;
    DWORD                       EuFullGlbOffset_L;
    DWORD                       EuFullGlbOffset_H;

    Csp_Opcodes_cmd             RestoreTsSharpCtrl;
    DWORD                       TsSharpCtrlOffset_L;
    DWORD                       TsSharpCtrlOffset_H;

    Csp_Opcodes_cmd             RestoreFFCUbufConfig;
    DWORD                       FFCUbufConfigOffset_L;
    DWORD                       FFCUbufConfigOffset_H;

    Csp_Opcodes_cmd             RestoreCsp;
    DWORD                       CSPOffset_L;
    DWORD                       CSPOffset_H;

    Csp_Opcodes_cmd             RestoreGpcpFe;
    DWORD                       GpcpFeOffset_L;
    DWORD                       GpcpFeOffset_H;

    Csp_Opcodes_cmd             RestoreGpcpBe;
    DWORD                       GpcpBeOffset_L;
    DWORD                       GpcpBeOffset_H;

    struct
    {
        Csp_Opcodes_cmd             RestoreGpcTopCmd;
        DWORD                       GpcTopOffset_L;
        DWORD                       GpcTopOffset_H;
    }RestoreGpcTop[HW_MAX_GPC_NUM_E3K];

    Csp_Opcodes_cmd             EUFSL1Invalidate;
    Csp_Opcodes_cmd             EUPSL1Invalidate;

    //Add csp fence to make sure csp registers restore finish before csp pre-parser.
    Csp_Opcodes_cmd             CspFenceCmd;
    DWORD                       CspFenceValue;
    Csp_Opcodes_cmd             CspWaitCmd;
    Csp_Opcodes_cmd             CspWaitMainCmd;
} CONTEXT_RESTORE_DMA_E3K;

typedef struct _GPCPBE_SAVE{
    Csp_Opcodes_cmd             SaveGpcpbe;
    DWORD                       GpcpbeOffset_L;
    DWORD                       GpcpbeOffset_H;
}GPCPBE_SAVE;

typedef struct _CONTEXT_SAVE_DMA_E3K
{
    Cmd_Blk_Cmd_Csp_Save_Rsto   SaveContext;
    DWORD                       ContextBufferOffset_L;
    DWORD                       ContextBufferOffset_H;

    Csp_Opcodes_cmd             SaveCSPRegister;
    DWORD                       CSPOffset_L;
    DWORD                       CSPOffset_H;

    Csp_Opcodes_cmd             SaveGpcpFeRegister;
    DWORD                       GpcpFeOffset_L;
    DWORD                       GpcpFeOffset_H;

    Csp_Opcodes_cmd             SaveGpcpBeRegister;
    DWORD                       GpcpBeOffset_L;
    DWORD                       GpcpBeOffset_H;

    struct
    {
        Csp_Opcodes_cmd             SaveGpcTopRegister;
        DWORD                       GpcTopOffset_L;
        DWORD                       GpcTopOffset_H;
    }SaveGpcTop[HW_MAX_GPC_NUM_E3K];

    Csp_Opcodes_cmd             CspFenceCmd;
    DWORD                       CspFenceValue;
    Csp_Opcodes_cmd             CspWaitCmd;
    Csp_Opcodes_cmd             CspWaitMainCmd;

    Csp_Opcodes_cmd             GpcpBeFenceCmd;
    DWORD                       GpcpBeFenceValue;
    Csp_Opcodes_cmd             GpcpBeWaitCmd;
    Csp_Opcodes_cmd             GpcpBeWaitMainCmd;
}CONTEXT_SAVE_DMA_E3K;

typedef struct _CACHEFLUSH_DMA_E3K
{
    //Fence/Wait to make sure FFC finish;
    Csp_Opcodes_cmd         PreUAVFEFenceCmd;  //CS only need this
    DWORD                   PreUAVFEFenceValue;
    Csp_Opcodes_cmd         PreUAVFEWaitCmd;
    Csp_Opcodes_cmd         PreUAVFEWaitMainCmd;

    Csp_Opcodes_cmd         PreUAVBEFenceCmd;
    DWORD                   PreUAVBEFenceValue;
    Csp_Opcodes_cmd         PreUAVBEWaitCmd;
    Csp_Opcodes_cmd         PreUAVBEWaitMainCmd;

    Csp_Opcodes_cmd         PreDFenceCmd;
    DWORD                   PreDFenceValue;
    Csp_Opcodes_cmd         PreDWaitCmd;
    Csp_Opcodes_cmd         PreDWaitMainCmd;

    Csp_Opcodes_cmd         PreZFenceCmd;
    DWORD                   PreZFenceValue;
    Csp_Opcodes_cmd         PreZWaitCmd;
    Csp_Opcodes_cmd         PreZWaitMainCmd;

    Cmd_Block_Command_Flush DZS_Flush;
    Cmd_Block_Command_Flush DZS_Invalidate;
    Csp_Opcodes_cmd         UAVFE_Flush;
    Csp_Opcodes_cmd         UAVFE_Invalidate;
    Csp_Opcodes_cmd         UAVBE_Flush;
    Csp_Opcodes_cmd         UAVBE_Invalidate;

    //used to make sure all FFC and BLC flush/inv and  above have been all finished.
    Csp_Opcodes_cmd         PostUAVFEFenceCmd;  //CS only need this
    DWORD                   PostUAVFEFenceValue;
    Csp_Opcodes_cmd         PostUAVFEWaitCmd;
    Csp_Opcodes_cmd         PostUAVFEWaitMainCmd;

    Csp_Opcodes_cmd         PostUAVBEFenceCmd;
    DWORD                   PostUAVBEFenceValue;
    Csp_Opcodes_cmd         PostUAVBEWaitCmd;
    Csp_Opcodes_cmd         PostUAVBEWaitMainCmd;

    Csp_Opcodes_cmd         PostDFenceCmd;
    DWORD                   PostDFenceValue;
    Csp_Opcodes_cmd         PostDWaitCmd;
    Csp_Opcodes_cmd         PostDWaitMainCmd;

    Csp_Opcodes_cmd         PostZFenceCmd;
    DWORD                   PostZFenceValue;
    Csp_Opcodes_cmd         PostZWaitCmd;
    Csp_Opcodes_cmd         PostZWaitMainCmd;

    Csp_Opcodes_cmd         StoFenceCmd;
    DWORD                   StoFenceValue;
    Csp_Opcodes_cmd         StoWaitCmd;
    Csp_Opcodes_cmd         StoWaitMainCmd;

    //Fence/Wait to make sure L2 path finish.

    Cmd_Block_Command_L2    L2Flush;
    Cmd_Block_Command_L2    L2Invalidate;
    Csp_Opcodes_cmd         L2FenceCmd;
    DWORD                   L2FenceValue;
    Csp_Opcodes_cmd         L2WaitCmd;
    Csp_Opcodes_cmd         L2WaitMainCmd;

    //Invalidate TU,EU L1 cache
    Csp_Opcodes_cmd         TUFEL1Invalidate; //TUL1 doesn't need fence, hw will handle by itself.
    Csp_Opcodes_cmd         TUBEL1Invalidate;
    Csp_Opcodes_cmd         EUFSL1Invalidate;
    Csp_Opcodes_cmd         EUPSL1Invalidate;

    //Fence/Wait to make sure BLC path finish
    Csp_Opcodes_cmd         PreCspFenceCmd;
    DWORD                   PreCspFenceValue;
    Csp_Opcodes_cmd         PreCspWaitCmd;
    Csp_Opcodes_cmd         PreCspWaitMainCmd;
    Cmd_Block_Command_Mxu   BLC_Flush;
    DWORD                   FlushAddr_L;
    DWORD                   FlushAddr_H;
    DWORD                   FlushMask_L;
    DWORD                   FlushMask_H;
    Csp_Opcodes_cmd         PostCspFenceCmd;
    DWORD                   PostCspFenceValue;
    Csp_Opcodes_cmd         PostCspWaitCmd;
    Csp_Opcodes_cmd         PostCspWaitMainCmd;
}CACHEFLUSH_DMA_E3K;

typedef struct _RB_PREDEFINE_DMA
{
    CONTEXT_RESTORE_DMA_E3K    RestoreDMA;
    CACHEFLUSH_DMA_E3K         FlushDMA3DL;
    CONTEXT_SAVE_DMA_E3K       SaveDMA;
    CONTEXT_RESTORE_DMA_E3K    RestoreShadowDMA;
}RB_PREDEFINE_DMA;

#endif //
