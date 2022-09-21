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

#ifndef _CSP_GLOBAL_REGISTER_H
#define _CSP_GLOBAL_REGISTER_H


#ifndef        CSP_GLOBAL_BLOCKBASE_INF
    #define    CSP_GLOBAL_BLOCKBASE_INF
    #define    BLOCK_CSP_GLOBAL_VERSION 1
    #define    BLOCK_CSP_GLOBAL_TIMESTAMP "11/1/2018 1:59:10 PM"
    #define    CSP_GLOBAL_BLOCK                                           0x0
    #define    CSP_GLOBAL_REG_START                                       0x0
    #define    CSP_GLOBAL_REG_END                                         0x380
    #define    CSP_GLOBAL_REG_LIMIT                                       0x380
#endif


#define        Reg_Block_Busy_Bits_Top_Offset                             0x0
#define        Reg_Block_Busy_Bits_Gpc0_0_Offset                          0x1
#define        Reg_Block_Busy_Bits_Gpc0_1_Offset                          0x2
#define        Reg_Block_Busy_Bits_Gpc1_0_Offset                          0x3
#define        Reg_Block_Busy_Bits_Gpc1_1_Offset                          0x4
#define        Reg_Block_Busy_Bits_Gpc2_0_Offset                          0x5
#define        Reg_Block_Busy_Bits_Gpc2_1_Offset                          0x6
#define        Reg_Ring_Buf_Offset                                        0x7
#define        Reg_Preempt_Cmd_Process_Offset                             0x23
#define        Reg_Cpu_Set_Offset                                         0x24
#define        Reg_Fence_Mask_Offset                                      0x25
#define        Reg_Reserved_Dw_0_Offset                                   0x26
#define        Reg_Pfb_Partition_3d_Cfg_Offset                            0x27
#define        Reg_Pfb_Partition_Cs_Cfg_Offset                            0x28
#define        Reg_Csp_Ms_Total_Gpu_Timestamp_Offset                      0x29
#define        Reg_Csp_Ms_Total_Busy_Time_Offset                          0x2B
#define        Reg_Csp_Ms_Query_Occlusion_Offset                          0x2D
#define        Reg_Ia_Vertices_Cnt_Offset                                 0x2F
#define        Reg_Ia_Primitives_Cnt_Offset                               0x31
#define        Reg_Ia_Ogl_Cut_Flag_32_Offset                              0x33
#define        Reg_Ia_Vb0_Offset_Offset                                   0x34
#define        Reg_Ia_Vb0_Stride_Offset                                   0x35
#define        Reg_Ia_Bufferfilledsize_Offset                             0x36
#define        Reg_Ia_Batch_Cfg_Offset                                    0x37
#define        Reg_Predicate_Status_Offset                                0x38
#define        Reg_Csp_Misc_Control_Offset                                0x39
#define        Reg_Tbr_Render_Mode_Ctrl_Offset                            0x3A
#define        Reg_Tbr_Frametimestamp_Lth_Offset                          0x3B
#define        Reg_Tbr_Frametimestamp_Hth_Offset                          0x3C
#define        Reg_Tbr_Traffic_Th_Offset                                  0x3D
#define        Reg_Tbr_Event_Count_Ref_Offset                             0x3E
#define        Reg_Descriptor_Base_Addr_Offset                            0x3F
#define        Reg_Csp_Fence_Counter_Offset                               0x47
#define        Reg_Ila_Counters_Offset                                    0x57
#define        Reg_Gpc_Signature_Offset                                   0x7F
#define        Reg_Sto_Signature_Offset                                   0xE5
#define        Reg_Csp_Slice_Status_Offset                                0xED
#define        Reg_Cur_L1_Dma_Cmd_Offset                                  0xEE
#define        Reg_Cur_L2_Dma_Cmd_Offset                                  0xEF
#define        Reg_Cur_3d_Rbuf_Cmd_Offset                                 0xF0
#define        Reg_Cur_3d_Hrbuf_Cmd_Offset                                0xF1
#define        Reg_Cur_Csh_Rbuf_Cmd_Offset                                0xF2
#define        Reg_Cur_Cs0_Rbuf_Cmd_Offset                                0xF3
#define        Reg_Cur_Cs1_Rbuf_Cmd_Offset                                0xF4
#define        Reg_Cur_Cs2_Rbuf_Cmd_Offset                                0xF5
#define        Reg_Cur_Cs3_Rbuf_Cmd_Offset                                0xF6
#define        Reg_C3d_Eng_Workload_Offset                                0xF7
#define        Reg_C3d_Eng_Alu_Workload_Offset                            0xF8
#define        Reg_C3d_Mem_Workload_Offset                                0xF9
#define        Reg_Vpp_Workload_Offset                                    0xFA
#define        Reg_Vcp0_Workload_Offset                                   0xFB
#define        Reg_Vcp1_Workload_Offset                                   0xFC
#define        Reg_Csp_Skip0_Offset                                       0xFD
#define        Reg_Vcp_Ring_Buf_Offset                                    0x180
#define        Reg_Vpp_Ring_Buf_Offset                                    0x188
#define        Reg_Vcp_Vpp_Block_Busy_Bits_Offset                         0x18C
#define        Reg_Csp_Skip1_Offset                                       0x18D
#define        Reg_Cmodel_Hl_Switch_Offset                                0x1FF
#define        Reg_Eu_Dbg_Reg_Offset                                      0x200


typedef enum
{
    CSP_MISC_CONTROL_PROVOKEMODE_FIRST_VTX   = 0,
    CSP_MISC_CONTROL_PROVOKEMODE_LAST_VTX    = 1,
} CSP_MISC_CONTROL_PROVOKEMODE;
typedef enum
{
    CMODEL_HL_SWITCH_CMD_TYPE_NO_SWITCH      = 0,
    CMODEL_HL_SWITCH_CMD_TYPE_DIP            = 1,
    CMODEL_HL_SWITCH_CMD_TYPE_GP             = 2,
    CMODEL_HL_SWITCH_CMD_TYPE_FAST_CLEAR     = 3,
    CMODEL_HL_SWITCH_CMD_TYPE_BIT_BLT        = 4,
    CMODEL_HL_SWITCH_CMD_TYPE_IMAGE_TRANSFER= 5,
    CMODEL_HL_SWITCH_CMD_TYPE_INDICATOR_OFF  = 6,
} CMODEL_HL_SWITCH_CMD_TYPE;
typedef enum
{
    EU_DBG_CFG_EXEC_EN_DISABLED              = 0,


    EU_DBG_CFG_EXEC_EN_ENABLED               = 1,

} EU_DBG_CFG_EXEC_EN;
typedef enum
{
    EU_DBG_CFG_TH_MODE_SINGLE                = 0,

    EU_DBG_CFG_TH_MODE_ALL                   = 1,
} EU_DBG_CFG_TH_MODE;
typedef enum
{
    EU_DBG_CFG_INT_EN_DISABLED               = 0,

    EU_DBG_CFG_INT_EN_ENABLED                = 1,


} EU_DBG_CFG_INT_EN;
typedef enum
{
    EU_DBG_CFG_RES_EN_DISABLED               = 0,

    EU_DBG_CFG_RES_EN_ENABLED                = 1,


} EU_DBG_CFG_RES_EN;
typedef enum
{
    EU_DBG_BP_STAT_BP_HIT_DISABLED           = 0,

    EU_DBG_BP_STAT_BP_HIT_ENABLED            = 1,


} EU_DBG_BP_STAT_BP_HIT;
typedef enum
{
    EU_DBG_BP_PC_BP_VALID_DISABLED           = 0,
    EU_DBG_BP_PC_BP_VALID_ENABLED            = 1,
} EU_DBG_BP_PC_BP_VALID;
typedef enum
{
    EU_DBG_BP_PC_SHADER_VS                   = 0,
    EU_DBG_BP_PC_SHADER_HS                   = 1,
    EU_DBG_BP_PC_SHADER_DS                   = 2,
    EU_DBG_BP_PC_SHADER_GS                   = 3,
    EU_DBG_BP_PC_SHADER_PS                   = 4,
    EU_DBG_BP_PC_SHADER_CS                   = 5,
} EU_DBG_BP_PC_SHADER;





typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Csp_Busy                  : 1;
        unsigned int Mxua_Busy                 : 1;
        unsigned int Mxub_Busy                 : 1;
        unsigned int L2_Busy                   : 1;
        unsigned int Gpcpfe_Busy               : 1;
        unsigned int Gpcpbe_Busy               : 1;
        unsigned int Sg_Busy                   : 3;
        unsigned int Tasfe_Busy                : 3;
        unsigned int Tasbe_Busy                : 3;
        unsigned int Hub_Busy                  : 3;
        unsigned int Reserved                  : 14;
    } reg;
} Reg_Block_Busy_Bits_Top;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Tgz_Busy                  : 4;
        unsigned int Iu_Busy                   : 4;
        unsigned int Wbu_Busy                  : 4;
        unsigned int Wls_Busy                  : 4;
        unsigned int Ffc_Busy                  : 4;
        unsigned int Tu_Busy                   : 4;
        unsigned int Reserved                  : 8;
    } reg;
} Reg_Block_Busy_Bits_Gpc0_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Eu_Constructor_Busy       : 4;
        unsigned int Euvs_Busy                 : 4;
        unsigned int Euhs_Busy                 : 4;
        unsigned int Eufe_Busy                 : 4;
        unsigned int Euds_Busy                 : 4;
        unsigned int Eugs_Busy                 : 4;
        unsigned int Eups_Busy                 : 4;
        unsigned int Eucs_Busy                 : 4;
    } reg;
} Reg_Block_Busy_Bits_Gpc0_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Tgz_Busy                  : 4;
        unsigned int Iu_Busy                   : 4;
        unsigned int Wbu_Busy                  : 4;
        unsigned int Wls_Busy                  : 4;
        unsigned int Ffc_Busy                  : 4;
        unsigned int Tu_Busy                   : 4;
        unsigned int Reserved                  : 8;
    } reg;
} Reg_Block_Busy_Bits_Gpc1_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Eu_Constructor_Busy       : 4;
        unsigned int Euvs_Busy                 : 4;
        unsigned int Euhs_Busy                 : 4;
        unsigned int Eufe_Busy                 : 4;
        unsigned int Euds_Busy                 : 4;
        unsigned int Eugs_Busy                 : 4;
        unsigned int Eups_Busy                 : 4;
        unsigned int Eucs_Busy                 : 4;
    } reg;
} Reg_Block_Busy_Bits_Gpc1_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Tgz_Busy                  : 4;
        unsigned int Iu_Busy                   : 4;
        unsigned int Wbu_Busy                  : 4;
        unsigned int Wls_Busy                  : 4;
        unsigned int Ffc_Busy                  : 4;
        unsigned int Tu_Busy                   : 4;
        unsigned int Reserved                  : 8;
    } reg;
} Reg_Block_Busy_Bits_Gpc2_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Eu_Constructor_Busy       : 4;
        unsigned int Euvs_Busy                 : 4;
        unsigned int Euhs_Busy                 : 4;
        unsigned int Eufe_Busy                 : 4;
        unsigned int Euds_Busy                 : 4;
        unsigned int Eugs_Busy                 : 4;
        unsigned int Eups_Busy                 : 4;
        unsigned int Eucs_Busy                 : 4;
    } reg;
} Reg_Block_Busy_Bits_Gpc2_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Addr                      : 28;

        unsigned int L2_Cachable               : 1;




        unsigned int Reserved                  : 2;
        unsigned int Kickoff                   : 1;
    } reg;
} Reg_Run_List_Ctx_Addr1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rb_Size                   : 32;
    } reg;
} Reg_Ring_Buf_Size;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rb_Head                   : 32;
    } reg;
} Reg_Ring_Buf_Head;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rb_Tail                   : 32;
    } reg;
} Reg_Ring_Buf_Tail;

typedef struct _Group_Ring_Buf
{
    Reg_Run_List_Ctx_Addr1           reg_Run_List_Ctx_Addr1;
    Reg_Ring_Buf_Size                reg_Ring_Buf_Size;
    Reg_Ring_Buf_Head                reg_Ring_Buf_Head;
    Reg_Ring_Buf_Tail                reg_Ring_Buf_Tail;
} Reg_Ring_Buf_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Preempt                   : 1;

        unsigned int Reserved                  : 31;
    } reg;
} Reg_Preempt_Cmd_Process;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Cpu_Set                   : 1;
        unsigned int Reserved                  : 31;
    } reg;
} Reg_Cpu_Set;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Mask                      : 32;

    } reg;
} Reg_Fence_Mask;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reesrved                  : 32;
    } reg;
} Reg_Reserved_Dw_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Lrb_Size                  : 4;

        unsigned int L1dma_Size                : 4;
        unsigned int L2dma_Size                : 4;
        unsigned int L1buf_Size                : 4;
        unsigned int Lib_Size                  : 4;
        unsigned int Hrb_Size                  : 4;
        unsigned int Hl1buf_Size               : 4;
        unsigned int Hib_Size                  : 4;
    } reg;
} Reg_Pfb_Partition_3d_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Csh_Rb_Size               : 4;
        unsigned int Csl0_Rb_Size              : 4;
        unsigned int Csl1_Rb_Size              : 4;
        unsigned int Csl2_Rb_Size              : 4;
        unsigned int Csl3_Rb_Size              : 4;
        unsigned int L1dma_Size                : 4;
        unsigned int L2dma_Size                : 4;
        unsigned int Csl_L1buf_Size            : 4;
    } reg;
} Reg_Pfb_Partition_Cs_Cfg;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Csp_Ms_Total_Gpu_Timestamp;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Csp_Ms_Total_Busy_Time;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Csp_Ms_Query_Occlusion;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Ia_Vertices_Cnt;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Ia_Primitives_Cnt;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ogl_Cut_Flag_32           : 32;

    } reg;
} Reg_Ia_Ogl_Cut_Flag_32;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Offset                    : 32;
    } reg;
} Reg_Ia_Vb0_Offset;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Stride                    : 32;
    } reg;
} Reg_Ia_Vb0_Stride;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Bufferfilledsize          : 32;
    } reg;
} Reg_Ia_Bufferfilledsize;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Batchprimnum              : 32;


    } reg;
} Reg_Ia_Batch_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Predicate_En              : 1;
        unsigned int Reserved                  : 31;
    } reg;
} Reg_Predicate_Status;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Triangle_Cut_Index_Enable : 1;

        unsigned int Gs_On                     : 1;
        unsigned int Ila_Mode                  : 4;
        unsigned int Fe_Cnt_Disable            : 1;
        unsigned int Ila_Gpc_Sel               : 2;

        unsigned int Provokemode               : 1;

        unsigned int Dump_3d_Signature_Zero    : 1;


        unsigned int Flat_Quad                 : 1;

        unsigned int Reserved1                 : 20;
    } reg;
} Reg_Csp_Misc_Control;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Render_Mode_Switch_En     : 1;
        unsigned int Render_Mode               : 1;

        unsigned int Render_Irq_Mode           : 1;




        unsigned int Event_Counter_Reset       : 1;

        unsigned int Frame_Begin               : 1;
        unsigned int Frame_End                 : 1;
        unsigned int Reserved1                 : 10;
        unsigned int Frame_Check_Window        : 16;

    } reg;
} Reg_Tbr_Render_Mode_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Frametimestamp_Low_Threshold : 32;



    } reg;
} Reg_Tbr_Frametimestamp_Lth;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Frametimestamp_High_Threshold : 32;




    } reg;
} Reg_Tbr_Frametimestamp_Hth;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Traffic_Threshold         : 32;



    } reg;
} Reg_Tbr_Traffic_Th;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Tbr_Irq_Ref               : 16;

        unsigned int Imr_Irq_Ref               : 16;

    } reg;
} Reg_Tbr_Event_Count_Ref;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Base_Addr;

typedef struct _Group_Descriptor_Base_Addr
{
    Reg_Base_Addr                    reg_Base_Addr;
} Reg_Descriptor_Base_Addr_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Counter0                  : 16;
        unsigned int Counter1                  : 16;
    } reg;
} Reg_Fence_Counter;

typedef struct _Group_Csp_Fence_Counter
{
    Reg_Fence_Counter                reg_Fence_Counter;
} Reg_Csp_Fence_Counter_Group;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Ila_Counters;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Sg_Sig_Low;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Sg_Sig_High;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_D_Sig_Low_S0;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_D_Sig_High_S0;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_D_Sig_Low_S1;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_D_Sig_High_S1;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_D_Sig_Low_S2;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_D_Sig_High_S2;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_D_Sig_Low_S3;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_D_Sig_High_S3;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Z_Sig_Low_S0;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Z_Sig_High_S0;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Z_Sig_Low_S1;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Z_Sig_High_S1;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Z_Sig_Low_S2;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Z_Sig_High_S2;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Z_Sig_Low_S3;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Z_Sig_High_S3;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_U_Sig_Low_S0;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_U_Sig_High_S0;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_U_Sig_Low_S1;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_U_Sig_High_S1;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_U_Sig_Low_S2;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_U_Sig_High_S2;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_U_Sig_Low_S3;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_U_Sig_High_S3;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Eu_Sig_Low_S0;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Eu_Sig_High_S0;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Eu_Sig_Low_S1;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Eu_Sig_High_S1;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Eu_Sig_Low_S2;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Eu_Sig_High_S2;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Eu_Sig_Low_S3;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Eu_Sig_High_S3;

typedef struct _Group_Gpc_Signature
{
    Reg_Sg_Sig_Low                   reg_Sg_Sig_Low;
    Reg_Sg_Sig_High                  reg_Sg_Sig_High;
    Reg_D_Sig_Low_S0                 reg_D_Sig_Low_S0;
    Reg_D_Sig_High_S0                reg_D_Sig_High_S0;
    Reg_D_Sig_Low_S1                 reg_D_Sig_Low_S1;
    Reg_D_Sig_High_S1                reg_D_Sig_High_S1;
    Reg_D_Sig_Low_S2                 reg_D_Sig_Low_S2;
    Reg_D_Sig_High_S2                reg_D_Sig_High_S2;
    Reg_D_Sig_Low_S3                 reg_D_Sig_Low_S3;
    Reg_D_Sig_High_S3                reg_D_Sig_High_S3;
    Reg_Z_Sig_Low_S0                 reg_Z_Sig_Low_S0;
    Reg_Z_Sig_High_S0                reg_Z_Sig_High_S0;
    Reg_Z_Sig_Low_S1                 reg_Z_Sig_Low_S1;
    Reg_Z_Sig_High_S1                reg_Z_Sig_High_S1;
    Reg_Z_Sig_Low_S2                 reg_Z_Sig_Low_S2;
    Reg_Z_Sig_High_S2                reg_Z_Sig_High_S2;
    Reg_Z_Sig_Low_S3                 reg_Z_Sig_Low_S3;
    Reg_Z_Sig_High_S3                reg_Z_Sig_High_S3;
    Reg_U_Sig_Low_S0                 reg_U_Sig_Low_S0;
    Reg_U_Sig_High_S0                reg_U_Sig_High_S0;
    Reg_U_Sig_Low_S1                 reg_U_Sig_Low_S1;
    Reg_U_Sig_High_S1                reg_U_Sig_High_S1;
    Reg_U_Sig_Low_S2                 reg_U_Sig_Low_S2;
    Reg_U_Sig_High_S2                reg_U_Sig_High_S2;
    Reg_U_Sig_Low_S3                 reg_U_Sig_Low_S3;
    Reg_U_Sig_High_S3                reg_U_Sig_High_S3;
    Reg_Eu_Sig_Low_S0                reg_Eu_Sig_Low_S0;
    Reg_Eu_Sig_High_S0               reg_Eu_Sig_High_S0;
    Reg_Eu_Sig_Low_S1                reg_Eu_Sig_Low_S1;
    Reg_Eu_Sig_High_S1               reg_Eu_Sig_High_S1;
    Reg_Eu_Sig_Low_S2                reg_Eu_Sig_Low_S2;
    Reg_Eu_Sig_High_S2               reg_Eu_Sig_High_S2;
    Reg_Eu_Sig_Low_S3                reg_Eu_Sig_Low_S3;
    Reg_Eu_Sig_High_S3               reg_Eu_Sig_High_S3;
} Reg_Gpc_Signature_Group;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Sto_Sig_Low;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Sto_Sig_High;

typedef struct _Group_Sto_Signature
{
    Reg_Sto_Sig_Low                  reg_Sto_Sig_Low;
    Reg_Sto_Sig_High                 reg_Sto_Sig_High;
} Reg_Sto_Signature_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Slice_Mask                : 16;
        unsigned int Reserved1                 : 16;
    } reg;
} Reg_Csp_Slice_Status;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Cur_L1_Dma_Cmd;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Cur_L2_Dma_Cmd;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Cur_3d_Rbuf_Cmd;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Cur_3d_Hrbuf_Cmd;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Cur_Csh_Rbuf_Cmd;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Cur_Cs0_Rbuf_Cmd;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Cur_Cs1_Rbuf_Cmd;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Cur_Cs2_Rbuf_Cmd;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Cur_Cs3_Rbuf_Cmd;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_C3d_Eng_Workload;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_C3d_Eng_Alu_Workload;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_C3d_Mem_Workload;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Vpp_Workload;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Vcp0_Workload;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Vcp1_Workload;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Csp_Skip0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Addr                      : 28;

        unsigned int Reserved                  : 3;
        unsigned int Kickoff                   : 1;
    } reg;
} Reg_Vcp_Run_List_Ctx_Addr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rb_Size                   : 32;
    } reg;
} Reg_Vcp_Ring_Buf_Size;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rb_Head                   : 32;
    } reg;
} Reg_Vcp_Ring_Buf_Head;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rb_Tail                   : 32;
    } reg;
} Reg_Vcp_Ring_Buf_Tail;

typedef struct _Group_Vcp_Ring_Buf
{
    Reg_Vcp_Run_List_Ctx_Addr        reg_Vcp_Run_List_Ctx_Addr;
    Reg_Vcp_Ring_Buf_Size            reg_Vcp_Ring_Buf_Size;
    Reg_Vcp_Ring_Buf_Head            reg_Vcp_Ring_Buf_Head;
    Reg_Vcp_Ring_Buf_Tail            reg_Vcp_Ring_Buf_Tail;
} Reg_Vcp_Ring_Buf_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Addr                      : 28;

        unsigned int Reserved                  : 3;
        unsigned int Kickoff                   : 1;
    } reg;
} Reg_Vpp_Run_List_Ctx_Addr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rb_Size                   : 32;
    } reg;
} Reg_Vpp_Ring_Buf_Size;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rb_Head                   : 32;
    } reg;
} Reg_Vpp_Ring_Buf_Head;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rb_Tail                   : 32;
    } reg;
} Reg_Vpp_Ring_Buf_Tail;

typedef struct _Group_Vpp_Ring_Buf
{
    Reg_Vpp_Run_List_Ctx_Addr        reg_Vpp_Run_List_Ctx_Addr;
    Reg_Vpp_Ring_Buf_Size            reg_Vpp_Ring_Buf_Size;
    Reg_Vpp_Ring_Buf_Head            reg_Vpp_Ring_Buf_Head;
    Reg_Vpp_Ring_Buf_Tail            reg_Vpp_Ring_Buf_Tail;
} Reg_Vpp_Ring_Buf_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vcp0_Busy                 : 1;
        unsigned int Vcp1_Busy                 : 1;
        unsigned int Vpp_Busy                  : 1;
        unsigned int Reserved                  : 29;
    } reg;
} Reg_Vcp_Vpp_Block_Busy_Bits;


typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int     uint;
    } reg;
} Reg_Csp_Skip1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Cmd_Type                  : 3;
        unsigned int Cmd_Cnt                   : 5;
        unsigned int Draw_Cnt                  : 5;
        unsigned int Reserved                  : 19;
    } reg;
} Reg_Cmodel_Hl_Switch;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Exec_En                   : 1;
        unsigned int Th_Mode                   : 1;
        unsigned int Int_En                    : 1;
        unsigned int Res_En                    : 1;
        unsigned int Dbg_Th_Id                 : 5;


        unsigned int Reserved                  : 23;
    } reg;
} Reg_Eu_Dbg_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Bp_Hit                    : 1;

        unsigned int Bp_Id                     : 3;
        unsigned int Hit_Th_Id                 : 5;

        unsigned int Reserved                  : 23;
    } reg;
} Reg_Eu_Dbg_Bp_Stat;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Bp_Valid                  : 1;
        unsigned int Shader                    : 3;
        unsigned int Offset                    : 16;


        unsigned int Reserved                  : 12;
    } reg;
} Reg_Eu_Dbg_Bp_Pc;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Offset                    : 16;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Eu_Dbg_Int_Instr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Offset                    : 16;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Eu_Dbg_Res_Instr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Value                     : 32;

    } reg;
} Reg_Eu_Dbg_Time_Stamp;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Eu_Dbg_Reversed;

typedef struct _Group_Eu_Dbg_Reg
{
    Reg_Eu_Dbg_Cfg                   reg_Eu_Dbg_Cfg;
    Reg_Eu_Dbg_Bp_Stat               reg_Eu_Dbg_Bp_Stat;
    Reg_Eu_Dbg_Bp_Pc                 reg_Eu_Dbg_Bp_Pc[8];
    Reg_Eu_Dbg_Int_Instr             reg_Eu_Dbg_Int_Instr;
    Reg_Eu_Dbg_Res_Instr             reg_Eu_Dbg_Res_Instr;
    Reg_Eu_Dbg_Time_Stamp            reg_Eu_Dbg_Time_Stamp[2];
    Reg_Eu_Dbg_Reversed              reg_Eu_Dbg_Reversed[2];
} Reg_Eu_Dbg_Reg_Group;

typedef struct _Csp_Global_regs
{
    Reg_Block_Busy_Bits_Top          reg_Block_Busy_Bits_Top;
    Reg_Block_Busy_Bits_Gpc0_0       reg_Block_Busy_Bits_Gpc0_0;
    Reg_Block_Busy_Bits_Gpc0_1       reg_Block_Busy_Bits_Gpc0_1;
    Reg_Block_Busy_Bits_Gpc1_0       reg_Block_Busy_Bits_Gpc1_0;
    Reg_Block_Busy_Bits_Gpc1_1       reg_Block_Busy_Bits_Gpc1_1;
    Reg_Block_Busy_Bits_Gpc2_0       reg_Block_Busy_Bits_Gpc2_0;
    Reg_Block_Busy_Bits_Gpc2_1       reg_Block_Busy_Bits_Gpc2_1;
    Reg_Ring_Buf_Group               reg_Ring_Buf[7];
    Reg_Preempt_Cmd_Process          reg_Preempt_Cmd_Process;
    Reg_Cpu_Set                      reg_Cpu_Set;
    Reg_Fence_Mask                   reg_Fence_Mask;
    Reg_Reserved_Dw_0                reg_Reserved_Dw_0;
    Reg_Pfb_Partition_3d_Cfg         reg_Pfb_Partition_3d_Cfg;
    Reg_Pfb_Partition_Cs_Cfg         reg_Pfb_Partition_Cs_Cfg;
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
    Reg_Csp_Fence_Counter_Group      reg_Csp_Fence_Counter[16];
    Reg_Ila_Counters                 reg_Ila_Counters[40];
    Reg_Gpc_Signature_Group          reg_Gpc_Signature[3];
    Reg_Sto_Signature_Group          reg_Sto_Signature[4];
    Reg_Csp_Slice_Status             reg_Csp_Slice_Status;
    Reg_Cur_L1_Dma_Cmd               reg_Cur_L1_Dma_Cmd;
    Reg_Cur_L2_Dma_Cmd               reg_Cur_L2_Dma_Cmd;
    Reg_Cur_3d_Rbuf_Cmd              reg_Cur_3d_Rbuf_Cmd;
    Reg_Cur_3d_Hrbuf_Cmd             reg_Cur_3d_Hrbuf_Cmd;
    Reg_Cur_Csh_Rbuf_Cmd             reg_Cur_Csh_Rbuf_Cmd;
    Reg_Cur_Cs0_Rbuf_Cmd             reg_Cur_Cs0_Rbuf_Cmd;
    Reg_Cur_Cs1_Rbuf_Cmd             reg_Cur_Cs1_Rbuf_Cmd;
    Reg_Cur_Cs2_Rbuf_Cmd             reg_Cur_Cs2_Rbuf_Cmd;
    Reg_Cur_Cs3_Rbuf_Cmd             reg_Cur_Cs3_Rbuf_Cmd;
    Reg_C3d_Eng_Workload             reg_C3d_Eng_Workload;
    Reg_C3d_Eng_Alu_Workload         reg_C3d_Eng_Alu_Workload;
    Reg_C3d_Mem_Workload             reg_C3d_Mem_Workload;
    Reg_Vpp_Workload                 reg_Vpp_Workload;
    Reg_Vcp0_Workload                reg_Vcp0_Workload;
    Reg_Vcp1_Workload                reg_Vcp1_Workload;
    Reg_Csp_Skip0                    reg_Csp_Skip0[131];
    Reg_Vcp_Ring_Buf_Group           reg_Vcp_Ring_Buf[2];
    Reg_Vpp_Ring_Buf_Group           reg_Vpp_Ring_Buf;
    Reg_Vcp_Vpp_Block_Busy_Bits      reg_Vcp_Vpp_Block_Busy_Bits;
    Reg_Csp_Skip1                    reg_Csp_Skip1[114];
    Reg_Cmodel_Hl_Switch             reg_Cmodel_Hl_Switch;
    Reg_Eu_Dbg_Reg_Group             reg_Eu_Dbg_Reg[24];
} Csp_Global_regs;

#endif
