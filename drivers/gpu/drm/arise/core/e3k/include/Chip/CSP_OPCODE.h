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

#ifndef _CSP_OPCODE_H
#define _CSP_OPCODE_H

#ifndef        COMMAND_OPCODES_BLOCKBASE_INF
    #define    COMMAND_OPCODES_BLOCKBASE_INF
    #define    BLOCK_COMMAND_OPCODES_VERSION 1
    #define    BLOCK_COMMAND_OPCODES_TIMESTAMP "3/5/2019 6:26:53 PM"
#endif


#define  CSP_OPCODE_Skip                    0x0
#define  CSP_OPCODE_Dma                     0x1
#define  CSP_OPCODE_Wait                    0x2
#define  CSP_OPCODE_Query_Dump              0x3

#define  CSP_OPCODE_Set_Register            0x4

#define  CSP_OPCODE_Set_Object              0x6
#define  CSP_OPCODE_Dip                     0x7
#define  CSP_OPCODE_Fence                   0x8




#define  CSP_OPCODE_Block_Command_Template  0x9


#define  CSP_OPCODE_Block_Command_Tas       0x9
#define  CSP_OPCODE_Block_Command_Sg        0x9
#define  CSP_OPCODE_Block_Command_Img_Trn   0x9
#define  CSP_OPCODE_Block_Command_Flush     0x9
#define  CSP_OPCODE_Block_Command_Eu        0x9
#define  CSP_OPCODE_Block_Command_Tu        0x9
#define  CSP_OPCODE_Block_Command_L2        0x9
#define  CSP_OPCODE_Block_Command_Mxu       0x9
#define  CSP_OPCODE_Block_Command_Csp       0x9
#define  CSP_OPCODE_Blk_Cmd_Csp_Blc         0x9
#define  CSP_OPCODE_Blk_Cmd_Csp_Save_Rsto   0x9
#define  CSP_OPCODE_Blk_Cmd_Csp_Indicator   0x9
#define  CSP_OPCODE_Blk_Cmd_Csp_Ld_Des      0x9
#define  CSP_OPCODE_Block_Command_Video     0x9
#define  CSP_OPCODE_Gp                      0xB

#define  CSP_OPCODE_Tbr_Indicator           0xF
#define  CSP_OPCODE_Vpp                     0xC



typedef enum
{
    DMA_SPECIAL_DMA_TYPE_SAVE_REST_CMD       = 0,
    DMA_SPECIAL_DMA_TYPE_QUERY_SETREG_CMD    = 1,
} DMA_SPECIAL_DMA_TYPE;
typedef enum
{
    DMA_MODE_NORMAL                          = 0,
    DMA_MODE_SAVE                            = 1,
    DMA_MODE_RESTORE                         = 2,
    DMA_MODE_FLUSH                           = 3,
} DMA_MODE;
typedef enum
{
    WAIT_WAIT_MODE_NORMAL_WAIT               = 0,
    WAIT_WAIT_MODE_KKK_WAIT                  = 1,
    WAIT_WAIT_MODE_WAIT_CHIP_IDLE            = 2,
    WAIT_WAIT_MODE_EXTERNAL_WAIT             = 3,
} WAIT_WAIT_MODE;
typedef enum
{
    WAIT_METHOD_BIGEQUAL                     = 0,
    WAIT_METHOD_EQUAL                        = 1,
} WAIT_METHOD;
typedef enum
{
    WAIT_STATION_ID_MAIN_PARSER              = 0,
    WAIT_STATION_ID_PRE_PARSER               = 1,

} WAIT_STATION_ID;
typedef enum
{
    QUERY_DUMP_ADDRESS_MODE_ADDRESS          = 0,

    QUERY_DUMP_ADDRESS_MODE_OFFSET           = 1,


} QUERY_DUMP_ADDRESS_MODE;
typedef enum
{
    QUERY_DUMP_CMD_TYPE_NORMAL_QUERY         = 0,
    QUERY_DUMP_CMD_TYPE_CPY_QRY_RES          = 1,
} QUERY_DUMP_CMD_TYPE;
typedef enum
{
    QUERY_DUMP_BLOCK_ID_CSP                  = 0,
    QUERY_DUMP_BLOCK_ID_GPCPFE               = 1,
    QUERY_DUMP_BLOCK_ID_SPIN                 = 2,
    QUERY_DUMP_BLOCK_ID_EU_FS                = 3,
    QUERY_DUMP_BLOCK_ID_TAS_FE               = 4,
    QUERY_DUMP_BLOCK_ID_TAS_BE               = 5,
    QUERY_DUMP_BLOCK_ID_FF                   = 6,
    QUERY_DUMP_BLOCK_ID_IU                   = 7,
    QUERY_DUMP_BLOCK_ID_WLS_FE               = 8,
    QUERY_DUMP_BLOCK_ID_EU_PS                = 9,
    QUERY_DUMP_BLOCK_ID_TUFE                 = 10,
    QUERY_DUMP_BLOCK_ID_L2                   = 11,
    QUERY_DUMP_BLOCK_ID_MXU                  = 12,
    QUERY_DUMP_BLOCK_ID_MCE                  = 13,
    QUERY_DUMP_BLOCK_ID_EU_CS                = 14,
    QUERY_DUMP_BLOCK_ID_VCP                  = 15,
    QUERY_DUMP_BLOCK_ID_VPP                  = 16,
    QUERY_DUMP_BLOCK_ID_ISP                  = 17,
    QUERY_DUMP_BLOCK_ID_MMU                  = 18,
    QUERY_DUMP_BLOCK_ID_WLS_BE               = 19,
    QUERY_DUMP_BLOCK_ID_TUBE                 = 20,
    QUERY_DUMP_BLOCK_ID_GPCPBE               = 21,
    QUERY_DUMP_BLOCK_ID_SPOUT                = 22,
    QUERY_DUMP_BLOCK_ID_MIU                  = 23,
    QUERY_DUMP_BLOCK_ID_GPCTOP               = 24,
} QUERY_DUMP_BLOCK_ID;
typedef enum
{
    SET_REGISTER_ADDRESS_MODE_ADDRESS        = 0,

    SET_REGISTER_ADDRESS_MODE_OFFSET         = 1,


} SET_REGISTER_ADDRESS_MODE;
typedef enum
{
    SET_OBJECT_OBJECT_TYPE_OCCLUSION_PREDICATE= 0,



    SET_OBJECT_OBJECT_TYPE_SO_OVERFLOW_PREDICATE_STREAMI= 1,




    SET_OBJECT_OBJECT_TYPE_SO_OVERFLOW_PREDICATE= 2,


} SET_OBJECT_OBJECT_TYPE;
typedef enum
{
    DIP_MODE_DRAW_IMM                        = 0,

    DIP_MODE_IB_8                            = 1,


    DIP_MODE_IB_16                           = 2,
    DIP_MODE_IB_32                           = 3,
    DIP_MODE_DRAW_AUTO                       = 4,
} DIP_MODE;
typedef enum
{
    DIP_INSTANCE_EN_DISABLED                 = 0,
    DIP_INSTANCE_EN_ENABLED                  = 1,
} DIP_INSTANCE_EN;
typedef enum
{
    DIP_P_TYPE_POINTLIST                     = 0,
    DIP_P_TYPE_LINELIST                      = 1,
    DIP_P_TYPE_LINESTRIP                     = 2,
    DIP_P_TYPE_TRIANGLELIST                  = 3,
    DIP_P_TYPE_TRIANGLESTRIP                 = 4,
    DIP_P_TYPE_LINELIST_ADJ                  = 5,
    DIP_P_TYPE_LINESTRIP_ADJ                 = 6,
    DIP_P_TYPE_TRIANGLELIST_ADJ              = 7,
    DIP_P_TYPE_TRIANGLESTRIP_ADJ             = 8,
    DIP_P_TYPE_LINELOOP                      = 9,
    DIP_P_TYPE_TRIANGLEFAN_OGL               = 10,
    DIP_P_TYPE_TRIANGLEFAN_D3D_VULKAN        = 11,
    DIP_P_TYPE_PATCHLIST                     = 12,
    DIP_P_TYPE_QUADLIST_OGL                  = 13,
    DIP_P_TYPE_QUADSTRIP_OGL                 = 14,
    DIP_P_TYPE_POLYGON_OGL                   = 15,
    DIP_P_TYPE_AXIS_ALIGNED_RECTANGLELIST    = 16,
} DIP_P_TYPE;
typedef enum
{
    FENCE_IRQ_NOP                            = 0,
    FENCE_IRQ_INTERRUPT_CPU                  = 1,
    FENCE_IRQ_INTERRUPT_GPU                  = 2,




} FENCE_IRQ;
typedef enum
{
    FENCE_FENCE_TYPE_INTERNAL                = 0,
    FENCE_FENCE_TYPE_EXTERNAL32              = 2,
    FENCE_FENCE_TYPE_EXTERNAL64              = 3,


} FENCE_FENCE_TYPE;
typedef enum
{
    FENCE_FENCE_UPDATE_MODE_COPY             = 0,
    FENCE_FENCE_UPDATE_MODE_OR               = 1,
} FENCE_FENCE_UPDATE_MODE;
typedef enum
{
    FENCE_RB_TYPE_3DFE                       = 0,
    FENCE_RB_TYPE_3DBE                       = 1,
    FENCE_RB_TYPE_CSL                        = 2,
    FENCE_RB_TYPE_CSH                        = 3,
} FENCE_RB_TYPE;
typedef enum
{
    FENCE_ROUTE_ID_CSP_FENCE                 = 0,

    FENCE_ROUTE_ID_Z_FENCE                   = 1,






    FENCE_ROUTE_ID_D_FENCE                   = 2,



    FENCE_ROUTE_ID_UAVFE_FENCE               = 3,






    FENCE_ROUTE_ID_UAVBE_FENCE               = 4,






    FENCE_ROUTE_ID_FS_STO_FENCE              = 5,


    FENCE_ROUTE_ID_MCE_FENCE                 = 6,

    FENCE_ROUTE_ID_L2_FENCE                  = 7,

    FENCE_ROUTE_ID_VCP_FENCE                 = 8,
    FENCE_ROUTE_ID_VPP_FENCE                 = 9,
    FENCE_ROUTE_ID_ISP_FENCE                 = 10,
} FENCE_ROUTE_ID;
typedef enum
{
    BLOCK_COMMAND_TEMPLATE_TYPE_FLUSH        = 0,
    BLOCK_COMMAND_TEMPLATE_TYPE_INVALIDATE_CACHE= 1,
    BLOCK_COMMAND_TEMPLATE_TYPE_SG           = 2,
    BLOCK_COMMAND_TEMPLATE_TYPE_IMAGE_TRANSFER= 3,
} BLOCK_COMMAND_TEMPLATE_TYPE;
typedef enum
{
    BLOCK_COMMAND_TAS_TYPE_COLOR_EXCLUDED    = 0,

    BLOCK_COMMAND_TAS_TYPE_COLOR_INCLUDED    = 1,


} BLOCK_COMMAND_TAS_TYPE;
typedef enum
{
    BLOCK_COMMAND_TAS_COMMAND_SPECIFIC_FIELD_DP_LINE= 0,

} BLOCK_COMMAND_TAS_COMMAND_SPECIFIC_FIELD;
typedef enum
{
    BLOCK_COMMAND_SG_AREA_TARGET_Z           = 0,
    BLOCK_COMMAND_SG_AREA_TARGET_S           = 1,
    BLOCK_COMMAND_SG_AREA_TARGET_D           = 2,
    BLOCK_COMMAND_SG_AREA_TARGET_U           = 3,
} BLOCK_COMMAND_SG_AREA_TARGET;
typedef enum
{
    BLOCK_COMMAND_SG_ACTION_TYPE_FAST_CLEAR_TILE= 0,
    BLOCK_COMMAND_SG_ACTION_TYPE_FAST_CLEAR_LINEAR= 1,


    BLOCK_COMMAND_SG_ACTION_TYPE_BIT_BLT     = 2,
    BLOCK_COMMAND_SG_ACTION_TYPE_GRADIENT_FILL= 3,
    BLOCK_COMMAND_SG_ACTION_TYPE_ROTATION    = 4,
    BLOCK_COMMAND_SG_ACTION_TYPE_RESERVED    = 7,
} BLOCK_COMMAND_SG_ACTION_TYPE;
typedef enum
{
    BLOCK_COMMAND_SG_TBR_LOOP_STATUS_PERTBRTILE= 0,
    BLOCK_COMMAND_SG_TBR_LOOP_STATUS_ONCE    = 1,



} BLOCK_COMMAND_SG_TBR_LOOP_STATUS;
typedef enum
{
    BLOCK_COMMAND_SG_BLT_OVERLAP_NON_OVERLAP= 0,

    BLOCK_COMMAND_SG_BLT_OVERLAP_OVERLAP     = 1,
} BLOCK_COMMAND_SG_BLT_OVERLAP;
typedef enum
{
    BLOCK_COMMAND_SG_ROT_DWORD0_CW_CW        = 0,
    BLOCK_COMMAND_SG_ROT_DWORD0_CW_CCW       = 1,
} BLOCK_COMMAND_SG_ROT_DWORD0_CW;
typedef enum
{
    BLOCK_COMMAND_IMG_TRN_DATA_FMT_1BPP      = 0,


    BLOCK_COMMAND_IMG_TRN_DATA_FMT_4BPP      = 1,



    BLOCK_COMMAND_IMG_TRN_DATA_FMT_8BPP      = 2,
    BLOCK_COMMAND_IMG_TRN_DATA_FMT_16BPP     = 3,
    BLOCK_COMMAND_IMG_TRN_DATA_FMT_32BPP     = 4,
    BLOCK_COMMAND_IMG_TRN_DATA_FMT_64BPP     = 5,
    BLOCK_COMMAND_IMG_TRN_DATA_FMT_128BPP    = 6,
} BLOCK_COMMAND_IMG_TRN_DATA_FMT;
typedef enum
{
    BLOCK_COMMAND_IMG_TRN_TBR_LOOP_STATUS_PERTBRTILE= 0,

    BLOCK_COMMAND_IMG_TRN_TBR_LOOP_STATUS_ONCE= 1,



} BLOCK_COMMAND_IMG_TRN_TBR_LOOP_STATUS;
typedef enum
{
    BLOCK_COMMAND_FLUSH_TYPE_FLUSH           = 0,
    BLOCK_COMMAND_FLUSH_TYPE_INVALIDATE_CACHE= 1,
    BLOCK_COMMAND_FLUSH_TYPE_SG              = 2,
    BLOCK_COMMAND_FLUSH_TYPE_IMAGE_TRANSFER  = 3,
} BLOCK_COMMAND_FLUSH_TYPE;
typedef enum
{
    BLOCK_COMMAND_FLUSH_TARGET_ALL_C         = 0,

    BLOCK_COMMAND_FLUSH_TARGET_Z_C           = 1,


    BLOCK_COMMAND_FLUSH_TARGET_S_C           = 2,


    BLOCK_COMMAND_FLUSH_TARGET_D_C           = 3,


    BLOCK_COMMAND_FLUSH_TARGET_UAV_C         = 4,

    BLOCK_COMMAND_FLUSH_TARGET_USHARP_DESC   = 5,

    BLOCK_COMMAND_FLUSH_TARGET_2D_ONLY       = 6,



    BLOCK_COMMAND_FLUSH_TARGET_D_SIG_BUF     = 7,



} BLOCK_COMMAND_FLUSH_TARGET;
typedef enum
{
    BLOCK_COMMAND_FLUSH_UAV_TYPE_3DFE        = 0,


    BLOCK_COMMAND_FLUSH_UAV_TYPE_3DBE        = 1,


    BLOCK_COMMAND_FLUSH_UAV_TYPE_CSL         = 2,


    BLOCK_COMMAND_FLUSH_UAV_TYPE_CSH         = 3,


} BLOCK_COMMAND_FLUSH_UAV_TYPE;
typedef enum
{
    BLOCK_COMMAND_EU_TYPE_INVALIDATE_L1I     = 0,
    BLOCK_COMMAND_EU_TYPE_DRAIN_EU           = 1,






} BLOCK_COMMAND_EU_TYPE;
typedef enum
{
    BLOCK_COMMAND_TU_TYPE_INVALIDATE_L1      = 0,


















} BLOCK_COMMAND_TU_TYPE;
typedef enum
{
    BLOCK_COMMAND_L2_TYPE_FLUSH_L2           = 0,
    BLOCK_COMMAND_L2_TYPE_INVALIDATE_L2      = 1,

    BLOCK_COMMAND_L2_TYPE_FLUSH_L1           = 4,

} BLOCK_COMMAND_L2_TYPE;
typedef enum
{
    BLOCK_COMMAND_L2_USAGE_ALL               = 0,
    BLOCK_COMMAND_L2_USAGE_TU                = 1,
    BLOCK_COMMAND_L2_USAGE_UAV               = 2,
    BLOCK_COMMAND_L2_USAGE_DZ                = 3,
    BLOCK_COMMAND_L2_USAGE_IC                = 4,
    BLOCK_COMMAND_L2_USAGE_CSP               = 5,
    BLOCK_COMMAND_L2_USAGE_DESC              = 6,
} BLOCK_COMMAND_L2_USAGE;
typedef enum
{
    BLOCK_COMMAND_MXU_CMD_TYPE_FLUSH_CACHE   = 0,
    BLOCK_COMMAND_MXU_CMD_TYPE_INVALIDATE_CACHE= 1,
    BLOCK_COMMAND_MXU_CMD_TYPE_INVALIDATE_UTLB_CACHE= 2,

    BLOCK_COMMAND_MXU_CMD_TYPE_INVALIDATE_DTLB_CACHE= 3,

} BLOCK_COMMAND_MXU_CMD_TYPE;
typedef enum
{
    BLOCK_COMMAND_MXU_BLC_DWORD1_USAGE_ALL   = 0,
    BLOCK_COMMAND_MXU_BLC_DWORD1_USAGE_TU    = 1,
    BLOCK_COMMAND_MXU_BLC_DWORD1_USAGE_UAV   = 2,
    BLOCK_COMMAND_MXU_BLC_DWORD1_USAGE_DZ    = 3,
    BLOCK_COMMAND_MXU_BLC_DWORD1_USAGE_IC    = 4,
    BLOCK_COMMAND_MXU_BLC_DWORD1_USAGE_DMA   = 5,
    BLOCK_COMMAND_MXU_BLC_DWORD1_USAGE_DESC  = 6,
} BLOCK_COMMAND_MXU_BLC_DWORD1_USAGE;
typedef enum
{
    BLOCK_COMMAND_CSP_TYPE_INDICATOR         = 0,

    BLOCK_COMMAND_CSP_TYPE_CLEAR_MXU_BLC     = 1,

    BLOCK_COMMAND_CSP_TYPE_SAVE              = 2,

    BLOCK_COMMAND_CSP_TYPE_RESTORE           = 3,

    BLOCK_COMMAND_CSP_TYPE_LOAD_DESCRIPTOR   = 4,

} BLOCK_COMMAND_CSP_TYPE;
typedef enum
{
    BLK_CMD_CSP_SAVE_RSTO_ADDRESS_MODE_ADDRESS= 0,

    BLK_CMD_CSP_SAVE_RSTO_ADDRESS_MODE_OFFSET= 1,


} BLK_CMD_CSP_SAVE_RSTO_ADDRESS_MODE;
typedef enum
{
    BLK_CMD_CSP_INDICATOR_INFO_OFF_MODE      = 0,
    BLK_CMD_CSP_INDICATOR_INFO_MCE_MODE      = 1,


    BLK_CMD_CSP_INDICATOR_INFO_CS_MODE       = 2,
    BLK_CMD_CSP_INDICATOR_INFO_3D_MODE       = 3,
    BLK_CMD_CSP_INDICATOR_INFO_VCP_MODE      = 4,
} BLK_CMD_CSP_INDICATOR_INFO;
typedef enum
{
    BLK_CMD_CSP_LD_DES_ADDRESS_MODE_ADDRESS  = 0,
    BLK_CMD_CSP_LD_DES_ADDRESS_MODE_OFFSET   = 1,

} BLK_CMD_CSP_LD_DES_ADDRESS_MODE;
typedef enum
{
    BLK_CMD_CSP_LD_DES_DESC_TYPE_T           = 0,

    BLK_CMD_CSP_LD_DES_DESC_TYPE_S           = 1,

    BLK_CMD_CSP_LD_DES_DESC_TYPE_U           = 2,

    BLK_CMD_CSP_LD_DES_DESC_TYPE_S_T         = 3,

} BLK_CMD_CSP_LD_DES_DESC_TYPE;
typedef enum
{
    BLOCK_COMMAND_VIDEO_IRQ_NOP              = 0,
    BLOCK_COMMAND_VIDEO_IRQ_INTERRUPT        = 1,
} BLOCK_COMMAND_VIDEO_IRQ;
typedef enum
{
    BLOCK_COMMAND_VIDEO_EXTERNAL_FENCE_NOP   = 0,
    BLOCK_COMMAND_VIDEO_EXTERNAL_FENCE_EXTERNAL_FENCE= 1,


} BLOCK_COMMAND_VIDEO_EXTERNAL_FENCE;
typedef enum
{
    GP_DW_TYPE_GROUP_NUMBER                  = 0,
    GP_DW_TYPE_GLOBAL_SIZE                   = 1,
} GP_DW_TYPE;
typedef enum
{
    GP_MINOR_OPCODE_NORMAL_GP                = 0,
    GP_MINOR_OPCODE_CS_FULL_CTX_END          = 1,



} GP_MINOR_OPCODE;
typedef enum
{
    TBR_INDICATOR_INDICATOR_INFO_BEGIN       = 0,
    TBR_INDICATOR_INDICATOR_INFO_FORCE_KICKOFF= 1,



    TBR_INDICATOR_INDICATOR_INFO_END         = 2,
} TBR_INDICATOR_INDICATOR_INFO;





typedef struct _Cmd_Skip
{
    unsigned int     Dwc                   : 16;

    unsigned int     Reserved              : 12;
    unsigned int     Major_Opcode          : 4;
} Cmd_Skip;

typedef struct _Cmd_Dma
{
    unsigned int     Dw_Num                : 16;
    unsigned int     Reserved              : 8;
    unsigned int     Special_Dma_Type      : 1;





    unsigned int     Reset_Dma             : 1;

    unsigned int     Mode                  : 2;

    unsigned int     Major_Opcode          : 4;
} Cmd_Dma;


       typedef struct _Cmd_Dma_Address_Dword1
       {
            unsigned int     Address_Low32         : 32;
       } Cmd_Dma_Address_Dword1;
       typedef struct _Cmd_Dma_Address_Dword2
       {
            unsigned int     Address_High8         : 8;
            unsigned int     L2_Cachable           : 1;

            unsigned int     Reserved              : 23;
       } Cmd_Dma_Address_Dword2;
       typedef struct _Cmd_Ctx_Address_Dword3
       {
            unsigned int     Ctx_Base_Addr         : 29;
            unsigned int     Reserved              : 3;
       } Cmd_Ctx_Address_Dword3;




typedef struct _Cmd_Wait
{
    unsigned int     Dwc                   : 16;



    unsigned int     Wait_Mode             : 2;
    unsigned int     Method                : 1;
    unsigned int     Station_Id            : 3;
    unsigned int     Slot_Id               : 5;

    unsigned int     Reserved              : 1;
    unsigned int     Major_Opcode          : 4;
} Cmd_Wait;


       typedef struct _Cmd_Wait_Kkk_Dword1
       {
            unsigned int     Event_Flag_Idx1       : 16;



            unsigned int     Event_Flag_Idx2       : 16;
       } Cmd_Wait_Kkk_Dword1;
       typedef struct _Cmd_Wait_External_Dword1
       {
            unsigned int     Address_Low32         : 16;

       } Cmd_Wait_External_Dword1;
       typedef struct _Cmd_Wait_External_Dword2
       {
            unsigned int     Address_High8         : 8;
            unsigned int     Reserved              : 24;
       } Cmd_Wait_External_Dword2;
       typedef struct _Cmd_Wait_External_Dword3
       {
            unsigned int     Event_Cnt_Ref_Low32   : 32;
       } Cmd_Wait_External_Dword3;
       typedef struct _Cmd_Wait_External_Dword4
       {
            unsigned int     Event_Cnt_Ref_High32  : 32;
       } Cmd_Wait_External_Dword4;




typedef struct _Cmd_Query_Dump

{
    unsigned int     Dwc                   : 3;


    unsigned int     Reserved              : 1;
    unsigned int     Query_Ready_En        : 1;


    unsigned int     Channle_Id            : 2;
    unsigned int     Dwc_To_Dump           : 13;

    unsigned int     Address_Mode          : 1;
    unsigned int     Timestamp_En          : 1;
    unsigned int     Cmd_Type              : 1;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;

} Cmd_Query_Dump;


       typedef struct _Cmd_Query_Dump_Address_Dword1
       {
            unsigned int     Address_Low32         : 32;


       } Cmd_Query_Dump_Address_Dword1;
       typedef struct _Cmd_Query_Dump_Address_Dword2
       {
            unsigned int     Address_High8         : 8;
            unsigned int     Reserved              : 11;
            unsigned int     Reg_Offset            : 13;
       } Cmd_Query_Dump_Address_Dword2;
       typedef struct _Cmd_Query_Dump_Offset_Dword1
       {
            unsigned int     Offset_Low32          : 32;
       } Cmd_Query_Dump_Offset_Dword1;
       typedef struct _Cmd_Query_Dump_Offset_Dword2
       {
            unsigned int     Offset_High8          : 8;
            unsigned int     Reserved              : 11;
            unsigned int     Reg_Offset            : 13;
       } Cmd_Query_Dump_Offset_Dword2;
       typedef struct _Cmd_Cpy_Qry_Res_Dword1
       {
            unsigned int     Begin_Query_Addr_Low32
                                                   : 32;

       } Cmd_Cpy_Qry_Res_Dword1;
       typedef struct _Cmd_Cpy_Qry_Res_Dword2
       {
            unsigned int     Begin_Query_Addr_High5
                                                   : 5;

            unsigned int     L2_Cachable           : 1;

            unsigned int     Bit64_En              : 1;
            unsigned int     Avaliability_Bit      : 1;
            unsigned int     Partial_Bit           : 1;
            unsigned int     Slice_Num             : 4;

            unsigned int     Reserved              : 3;
            unsigned int     End_Query_Ddw_Offs    : 8;

            unsigned int     End_Query_Ava_Ddw_Offs
                                                   : 8;


       } Cmd_Cpy_Qry_Res_Dword2;
       typedef struct _Cmd_Cpy_Qry_Res_Dword3
       {
            unsigned int     Dest_Addr_Low32       : 32;

       } Cmd_Cpy_Qry_Res_Dword3;
       typedef struct _Cmd_Cpy_Qry_Res_Dword4
       {
            unsigned int     Dest_Addr_High8       : 8;

            unsigned int     Reserved              : 24;
       } Cmd_Cpy_Qry_Res_Dword4;




typedef struct _Cmd_Set_Register

{
    unsigned int     Dwc                   : 7;





    unsigned int     Address_Mode          : 1;
    unsigned int     Addr_En               : 1;



    unsigned int     Mask_En               : 1;


    unsigned int     Start_Offset          : 13;
    unsigned int     Block_Id              : 5;

    unsigned int     Major_Opcode          : 4;
} Cmd_Set_Register;


       typedef struct _Cmd_Set_Register_Addr_Dword1
       {
            unsigned int     Address_Low32         : 32;
       } Cmd_Set_Register_Addr_Dword1;
       typedef struct _Cmd_Set_Register_Addr_Dword2
       {
            unsigned int     Address_High8         : 8;
            unsigned int     L2_Cachable           : 1;
            unsigned int     If_Physical           : 1;
            unsigned int     Reserved              : 9;
            unsigned int     Reg_Cnt               : 13;
       } Cmd_Set_Register_Addr_Dword2;
       typedef struct _Cmd_Set_Register_Offset_Dword1


       {
            unsigned int     Offset_Low32          : 32;
       } Cmd_Set_Register_Offset_Dword1;
       typedef struct _Cmd_Set_Register_Offset_Dword2


       {
            unsigned int     Offset_High8          : 8;
            unsigned int     Reserved              : 11;
            unsigned int     Reg_Cnt               : 13;
       } Cmd_Set_Register_Offset_Dword2;




typedef struct _Cmd_Set_Object
{
    unsigned int     Dwc                   : 2;
    unsigned int     Object_Type           : 2;
    unsigned int     Reserved              : 22;
    unsigned int     Predicate_Not         : 1;
    unsigned int     Hint                  : 1;
    unsigned int     Major_Opcode          : 4;
} Cmd_Set_Object;


       typedef struct _Cmd_Set_Object_Dword1
       {
            unsigned int     Address_Low32         : 32;
       } Cmd_Set_Object_Dword1;
       typedef struct _Cmd_Set_Object_Dword2
       {
            unsigned int     Address_High8         : 8;
            unsigned int     Reserved              : 24;
       } Cmd_Set_Object_Dword2;




typedef struct _Cmd_Dip
{
    unsigned int     Predicate_En          : 1;
    unsigned int     Indirect              : 1;

    unsigned int     Mode                  : 3;
    unsigned int     Instance_En           : 1;
    unsigned int     Start_Index_Valid     : 1;






    unsigned int     Start_Vertex_Valid    : 1;







    unsigned int     Start_Instance_Valid  : 1;






    unsigned int     P_Type                : 5;
    unsigned int     Indirect_Drawcnt_En   : 1;





    unsigned int     Reserved1             : 1;
    unsigned int     Patch_Vertex_Count    : 6;
    unsigned int     Reserved              : 6;
    unsigned int     Major_Opcode          : 4;
} Cmd_Dip;


       typedef struct _Cmd_Dip_Dword1
       {
            unsigned int     Draw_Count            : 32;




       } Cmd_Dip_Dword1;
       typedef struct _Cmd_Dip_Dword2
       {
            unsigned int     Drawcount_Addr_Low32  : 32;




       } Cmd_Dip_Dword2;
       typedef struct _Cmd_Dip_Dword3
       {
            unsigned int     Drawcount_Addr_High8  : 8;




            unsigned int     L2_Cachable           : 1;
            unsigned int     Reserved              : 23;
       } Cmd_Dip_Dword3;
       typedef struct _Cmd_Dip_Dword4
       {
            unsigned int     Start_Instance        : 32;




       } Cmd_Dip_Dword4;
       typedef struct _Cmd_Dip_Dword5
       {
            unsigned int     Instance_Count        : 32;



       } Cmd_Dip_Dword5;
       typedef struct _Cmd_Dip_Dword6
       {
            unsigned int     Index_Count           : 32;



       } Cmd_Dip_Dword6;
       typedef struct _Cmd_Dip_Dword7
       {
            unsigned int     Ib_Base_Addr_Low32    : 32;




       } Cmd_Dip_Dword7;
       typedef struct _Cmd_Dip_Dword8
       {
            unsigned int     Ib_Base_Addr_High8    : 8;




            unsigned int     L2_Cachable           : 1;

            unsigned int     Outofbound_Mode       : 1;

            unsigned int     Reserved              : 22;
       } Cmd_Dip_Dword8;
       typedef struct _Cmd_Dip_Dword9
       {
            unsigned int     Ib_Offset             : 32;




       } Cmd_Dip_Dword9;
       typedef struct _Cmd_Dip_Dword10
       {
            unsigned int     Ib_Byte_Size          : 32;




       } Cmd_Dip_Dword10;
       typedef struct _Cmd_Dip_Dword11
       {
            unsigned int     Start_Vertex          : 32;




       } Cmd_Dip_Dword11;
       typedef struct _Cmd_Dip_Dword12
       {
            unsigned int     Start_Index           : 32;




       } Cmd_Dip_Dword12;
       typedef struct _Cmd_Dip_Dword13
       {
            unsigned int     Para_Base_Addr_Low32  : 32;


       } Cmd_Dip_Dword13;
       typedef struct _Cmd_Dip_Dword14
       {
            unsigned int     Para_Base_Addr_High8  : 8;


            unsigned int     L2_Cachable           : 1;

            unsigned int     Reserved              : 23;
       } Cmd_Dip_Dword14;
       typedef struct _Cmd_Dip_Dword15
       {
            unsigned int     Para_Stride           : 32;








       } Cmd_Dip_Dword15;
       typedef struct _Cmd_Dip_Insdrawauto_Dword1
       {
            unsigned int     Instance_Count        : 32;




       } Cmd_Dip_Insdrawauto_Dword1;




typedef struct _Cmd_Fence




{
    unsigned int     Dwc                   : 3;

    unsigned int     Irq                   : 2;
    unsigned int     Fence_Type            : 2;
    unsigned int     Slot_Id               : 5;



    unsigned int     Reserved              : 1;
    unsigned int     Fence_Update_Mode     : 1;
    unsigned int     Reserved1             : 8;
    unsigned int     Rb_Type               : 2;

    unsigned int     Route_Id              : 4;


    unsigned int     Major_Opcode          : 4;

} Cmd_Fence;


       typedef struct _Cmd_Fence_Internal_Dword1
       {
            unsigned int     Update_Value          : 16;
            unsigned int     Slice_Mask            : 16;



       } Cmd_Fence_Internal_Dword1;
       typedef struct _Cmd_Fence_External_Addr_Dword1


       {
            unsigned int     External_Addr_Low32   : 32;
       } Cmd_Fence_External_Addr_Dword1;
       typedef struct _Cmd_Fence_External_Addr_Dword2


       {
            unsigned int     External_Addr_High8   : 8;
            unsigned int     Reserved              : 23;
            unsigned int     Fence_Update_Timing   : 1;

       } Cmd_Fence_External_Addr_Dword2;
       typedef struct _Cmd_Fence_External_Data_Dword3


       {
            unsigned int     External_Data1        : 32;
       } Cmd_Fence_External_Data_Dword3;
       typedef struct _Cmd_Fence_External_Data_Dword4


       {
            unsigned int     External_Data2        : 32;
       } Cmd_Fence_External_Data_Dword4;




typedef struct _Cmd_Block_Command_Template


{
    unsigned int     Dwc                   : 12;



    unsigned int     Type                  : 2;
    unsigned int     Command_Specific_Field
                                           : 7;
    unsigned int     Reserved0             : 2;
    unsigned int     Block_Id              : 5;

    unsigned int     Major_Opcode          : 4;

} Cmd_Block_Command_Template;

typedef struct _Cmd_Block_Command_Tas
{
    unsigned int     Dwc                   : 12;
    unsigned int     Type                  : 2;
    unsigned int     Command_Specific_Field
                                           : 6;
    unsigned int     Reserved0             : 3;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;

} Cmd_Block_Command_Tas;

typedef struct _Cmd_Block_Command_Sg
{
    unsigned int     Dwc                   : 12;
    unsigned int     Type                  : 2;
    unsigned int     Area_Target           : 2;
    unsigned int     Action_Type           : 3;
    unsigned int     Tbr_Loop_Status       : 1;




    unsigned int     Reserved0             : 1;
    unsigned int     Blt_Overlap           : 1;

    unsigned int     Predicate_En          : 1;
    unsigned int     Block_Id              : 5;





    unsigned int     Major_Opcode          : 4;

} Cmd_Block_Command_Sg;


       typedef struct _Cmd_Block_Command_Sg_Dword0

       {
            unsigned int     Xmin                  : 16;
            unsigned int     Xmax                  : 16;
       } Cmd_Block_Command_Sg_Dword0;
       typedef struct _Cmd_Block_Command_Sg_Rot_Dword0

       {
            unsigned int     Cw                    : 1;
            unsigned int     Reserved0             : 31;
       } Cmd_Block_Command_Sg_Rot_Dword0;
       typedef struct _Cmd_Block_Command_Sg_Dword1

       {
            unsigned int     Ymin                  : 16;
            unsigned int     Ymax                  : 16;
       } Cmd_Block_Command_Sg_Dword1;
       typedef struct _Cmd_Block_Command_Sg_Blt_Dword2

       {
            unsigned int     Dx                    : 15;
            unsigned int     Reserved0             : 1;
            unsigned int     Dy                    : 15;
            unsigned int     Reserved1             : 1;
       } Cmd_Block_Command_Sg_Blt_Dword2;
       typedef struct _Cmd_Block_Command_Sg_Gradient_Fill_Dword

       {
            unsigned int     Color                 : 28;
            unsigned int     Reserved              : 4;
       } Cmd_Block_Command_Sg_Gradient_Fill_Dword;




typedef struct _Cmd_Block_Command_Img_Trn
{
    unsigned int     Dwc                   : 12;
    unsigned int     Type                  : 2;
    unsigned int     Is_Dword_Aligned      : 1;





    unsigned int     Data_Fmt              : 3;
    unsigned int     Tbr_Loop_Status       : 1;



    unsigned int     Reserved              : 3;
    unsigned int     Predicate_En          : 1;
    unsigned int     Block_Id              : 5;


    unsigned int     Major_Opcode          : 4;

} Cmd_Block_Command_Img_Trn;


       typedef struct _Cmd_Block_Command_Img_Trn_Dword0

       {
            unsigned int     Xmin                  : 16;
            unsigned int     Xmax                  : 16;
       } Cmd_Block_Command_Img_Trn_Dword0;
       typedef struct _Cmd_Block_Command_Img_Trn_Dword1

       {
            unsigned int     Ymin                  : 16;
            unsigned int     Ymax                  : 16;
       } Cmd_Block_Command_Img_Trn_Dword1;




typedef struct _Cmd_Block_Command_Flush
{
    unsigned int     Dwc                   : 12;

    unsigned int     Type                  : 2;

    unsigned int     Target                : 5;

    unsigned int     Uav_Type              : 2;



    unsigned int     Reserved              : 2;
    unsigned int     Block_Id              : 5;


    unsigned int     Major_Opcode          : 4;

} Cmd_Block_Command_Flush;

typedef struct _Cmd_Block_Command_Eu
{
    unsigned int     Dwc                   : 12;
    unsigned int     Type                  : 2;
    unsigned int     Command_Specific_Field
                                           : 7;







    unsigned int     Reserved              : 2;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;

} Cmd_Block_Command_Eu;

typedef struct _Cmd_Block_Command_Tu
{
    unsigned int     Dwc                   : 12;
    unsigned int     Type                  : 2;
    unsigned int     Command_Specific_Field
                                           : 7;







    unsigned int     Reserved              : 2;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;

} Cmd_Block_Command_Tu;

typedef struct _Cmd_Block_Command_L2
{
    unsigned int     Dwc                   : 12;

    unsigned int     Type                  : 3;
    unsigned int     Usage                 : 3;
    unsigned int     Reserved              : 5;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;

} Cmd_Block_Command_L2;


       typedef struct _Cmd_Block_Command_L2_Dword0
       {
            unsigned int     Start_Address_Low32   : 32;
       } Cmd_Block_Command_L2_Dword0;
       typedef struct _Cmd_Block_Command_L2_Dword1
       {
            unsigned int     Start_Address_High8   : 8;

            unsigned int     Reserved              : 24;
       } Cmd_Block_Command_L2_Dword1;
       typedef struct _Cmd_Block_Command_L2_Dword2
       {
            unsigned int     Address_Byte_Mask_Low32
                                                   : 32;
       } Cmd_Block_Command_L2_Dword2;
       typedef struct _Cmd_Block_Command_L2_Dword3
       {
            unsigned int     Address_Byte_Mask_High8
                                                   : 8;


            unsigned int     Reserved              : 24;
       } Cmd_Block_Command_L2_Dword3;




typedef struct _Cmd_Block_Command_Mxu
{
    unsigned int     Dwc                   : 12;







    unsigned int     Cmd_Type              : 2;
    unsigned int     Reserved              : 4;
    unsigned int     Proc_Id               : 4;
    unsigned int     Reserved1             : 1;

    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;

} Cmd_Block_Command_Mxu;


       typedef struct _Cmd_Block_Command_Mxu_Blc_Dword0

       {
            unsigned int     Start_Address_Low32   : 32;
       } Cmd_Block_Command_Mxu_Blc_Dword0;
       typedef struct _Cmd_Block_Command_Mxu_Blc_Dword1

       {
            unsigned int     Start_Address_High8   : 8;

            unsigned int     Usage                 : 3;
            unsigned int     Reserved              : 21;
       } Cmd_Block_Command_Mxu_Blc_Dword1;
       typedef struct _Cmd_Block_Command_Mxu_Blc_Dword2

       {
            unsigned int     Address_Byte_Mask_Low32
                                                   : 32;
       } Cmd_Block_Command_Mxu_Blc_Dword2;
       typedef struct _Cmd_Block_Command_Mxu_Blc_Dword3

       {
            unsigned int     Address_Byte_Mask_High8
                                                   : 8;

            unsigned int     Reserved              : 24;
       } Cmd_Block_Command_Mxu_Blc_Dword3;
       typedef struct _Cmd_Block_Command_Mxu_Ptc_Dword0

       {
            unsigned int     Reserved              : 12;
            unsigned int     Start_Address_L       : 20;
       } Cmd_Block_Command_Mxu_Ptc_Dword0;
       typedef struct _Cmd_Block_Command_Mxu_Ptc_Dword1

       {
            unsigned int     Start_Address_High8   : 8;

            unsigned int     Reserved              : 24;
       } Cmd_Block_Command_Mxu_Ptc_Dword1;
       typedef struct _Cmd_Block_Command_Mxu_Ptc_Dword2

       {
            unsigned int     Reserved              : 12;
            unsigned int     Address_Byte_Mask_L   : 20;
       } Cmd_Block_Command_Mxu_Ptc_Dword2;
       typedef struct _Cmd_Block_Command_Mxu_Ptc_Dword3

       {
            unsigned int     Address_Byte_Mask_High8
                                                   : 8;

            unsigned int     Reserved              : 24;
       } Cmd_Block_Command_Mxu_Ptc_Dword3;
       typedef struct _Cmd_Block_Command_Mxu_L2_Dword0

       {
            unsigned int     Address_Low32         : 32;
       } Cmd_Block_Command_Mxu_L2_Dword0;
       typedef struct _Cmd_Block_Command_Mxu_L2_Dword1

       {
            unsigned int     Address_High8         : 8;

            unsigned int     Reserved              : 24;
       } Cmd_Block_Command_Mxu_L2_Dword1;
       typedef struct _Cmd_Block_Command_Mxu_L2_Dword2

       {
            unsigned int     Mask_Low32            : 32;
       } Cmd_Block_Command_Mxu_L2_Dword2;
       typedef struct _Cmd_Block_Command_Mxu_L2_Dword3

       {
            unsigned int     Mask_High8            : 8;


            unsigned int     Reserved              : 24;
       } Cmd_Block_Command_Mxu_L2_Dword3;




typedef struct _Cmd_Block_Command_Csp
{
    unsigned int     Dwc                   : 3;

    unsigned int     Header                : 17;
    unsigned int     Type                  : 3;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;

} Cmd_Block_Command_Csp;

typedef struct _Cmd_Blk_Cmd_Csp_Blc
{
    unsigned int     Dwc                   : 3;
    unsigned int     Counter               : 17;



    unsigned int     Type                  : 3;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;

} Cmd_Blk_Cmd_Csp_Blc;


       typedef struct _Cmd_Blk_Cmd_Csp_Blc_Dword1
       {
            unsigned int     Start_Address_Low32   : 32;
       } Cmd_Blk_Cmd_Csp_Blc_Dword1;
       typedef struct _Cmd_Blk_Cmd_Csp_Blc_Dword2
       {
            unsigned int     Start_Address_High8   : 8;

            unsigned int     Reserved              : 16;
            unsigned int     Clear_Value1          : 4;


            unsigned int     Clear_Value0          : 4;


       } Cmd_Blk_Cmd_Csp_Blc_Dword2;
       typedef struct _Cmd_Blk_Cmd_Csp_Blc_Dword3
       {
            unsigned int     Blc_Mask              : 32;

       } Cmd_Blk_Cmd_Csp_Blc_Dword3;




typedef struct _Cmd_Blk_Cmd_Csp_Save_Rsto
{
    unsigned int     Dwc                   : 3;

    unsigned int     Reserved              : 16;
    unsigned int     Address_Mode          : 1;
    unsigned int     Type                  : 3;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;

} Cmd_Blk_Cmd_Csp_Save_Rsto;


       typedef struct _Cmd_Blk_Cmd_Csp_Save_Rsto_Addr_Dword1


       {
            unsigned int     Start_Address_Low32   : 32;
       } Cmd_Blk_Cmd_Csp_Save_Rsto_Addr_Dword1;
       typedef struct _Cmd_Blk_Cmd_Csp_Save_Rsto_Addr_Dword2


       {
            unsigned int     Start_Address_High8   : 8;
            unsigned int     L2_Cachable           : 1;

            unsigned int     Reserved              : 23;
       } Cmd_Blk_Cmd_Csp_Save_Rsto_Addr_Dword2;
       typedef struct _Cmd_Blk_Cmd_Csp_Save_Rsto_Offset_Dword1


       {
            unsigned int     Offset_Low32          : 32;
       } Cmd_Blk_Cmd_Csp_Save_Rsto_Offset_Dword1;
       typedef struct _Cmd_Blk_Cmd_Csp_Save_Rsto_Offset_Dword2


       {
            unsigned int     Offset_High8          : 8;
            unsigned int     Reserved              : 24;
       } Cmd_Blk_Cmd_Csp_Save_Rsto_Offset_Dword2;




typedef struct _Cmd_Blk_Cmd_Csp_Indicator
{
    unsigned int     Dwc                   : 3;

    unsigned int     Reserved              : 10;
    unsigned int     Process_Id            : 4;
    unsigned int     Info                  : 3;
    unsigned int     Type                  : 3;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;

} Cmd_Blk_Cmd_Csp_Indicator;


       typedef struct _Cmd_Blk_Cmd_Csp_Indicator_Dword1

       {
            unsigned int     Slice_Mask            : 12;
            unsigned int     Reserved              : 20;
       } Cmd_Blk_Cmd_Csp_Indicator_Dword1;




typedef struct _Cmd_Blk_Cmd_Csp_Ld_Des
{
    unsigned int     Dwc                   : 3;
    unsigned int     Des_Block_Id          : 5;
    unsigned int     Reserved1             : 1;
    unsigned int     Pack_En               : 1;



    unsigned int     Heap_Idx              : 3;

    unsigned int     Address_Mode          : 1;
    unsigned int     Reserved              : 2;
    unsigned int     Desc_Buffer_En        : 1;



    unsigned int     Desc_Type             : 3;
    unsigned int     Type                  : 3;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;

} Cmd_Blk_Cmd_Csp_Ld_Des;


       typedef struct _Cmd_Blk_Cmd_Csp_Ld_Des_T_Dword1


       {
            unsigned int     Desc_Base_Addr_Low32  : 32;
       } Cmd_Blk_Cmd_Csp_Ld_Des_T_Dword1;
       typedef struct _Cmd_Blk_Cmd_Csp_Ld_Des_T_Dword2


       {
            unsigned int     Desc_Base_Addr_High8  : 8;
            unsigned int     L2_Cachable           : 1;

            unsigned int     Reserved              : 16;
            unsigned int     Desc_Num              : 7;


       } Cmd_Blk_Cmd_Csp_Ld_Des_T_Dword2;
       typedef struct _Cmd_Blk_Cmd_Csp_Ld_Des_T_Dword3


       {
            unsigned int     Buf_Full_Range        : 28;


            unsigned int     Reserved              : 4;
       } Cmd_Blk_Cmd_Csp_Ld_Des_T_Dword3;
       typedef struct _Cmd_Blk_Cmd_Csp_Ld_Des_T_Dword4


       {
            unsigned int     Desc_Origin_Offset    : 28;

            unsigned int     Reserved              : 4;
       } Cmd_Blk_Cmd_Csp_Ld_Des_T_Dword4;
       typedef struct _Cmd_Blk_Cmd_Csp_Ld_Des_T_Dword5


       {
            unsigned int     Desc_Dynamic_Offset   : 28;

            unsigned int     Reserved              : 4;
       } Cmd_Blk_Cmd_Csp_Ld_Des_T_Dword5;
       typedef struct _Cmd_Blk_Cmd_Csp_Ld_Des_T_Dword6


       {
            unsigned int     Reg_Offset1           : 13;

            unsigned int     Reg_Offset2           : 13;
            unsigned int     Reserved              : 6;
       } Cmd_Blk_Cmd_Csp_Ld_Des_T_Dword6;




typedef struct _Cmd_Block_Command_Video
{
    unsigned int     Dwc                   : 15;
    unsigned int     Irq                   : 1;
    unsigned int     External_Fence        : 1;
    unsigned int     Reserved_1            : 2;
    unsigned int     Fence_Update_Timing   : 1;

    unsigned int     Reserved_2            : 3;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;

} Cmd_Block_Command_Video;


       typedef struct _Cmd_Block_Command_Video_Dword0

       {
            unsigned int     Address_Low32         : 32;
       } Cmd_Block_Command_Video_Dword0;
       typedef struct _Cmd_Block_Command_Video_Dword1

       {
            unsigned int     Address_High8         : 8;

            unsigned int     Reserved              : 24;
       } Cmd_Block_Command_Video_Dword1;
       typedef struct _Cmd_Block_Command_Video_Dword2

       {
            unsigned int     Value                 : 32;
       } Cmd_Block_Command_Video_Dword2;
       typedef struct _Cmd_Block_Command_Video_Dword3

       {
            unsigned int     Value                 : 32;
       } Cmd_Block_Command_Video_Dword3;




typedef struct _Cmd_Gp

{
    unsigned int     Dwc                   : 4;

    unsigned int     Wait_Done_En          : 1;

    unsigned int     Predicate_En          : 1;
    unsigned int     Reserved              : 14;
    unsigned int     Indirect_En           : 1;
    unsigned int     Dw_Type               : 1;
    unsigned int     Minor_Opcode          : 1;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;
} Cmd_Gp;


       typedef struct _Cmd_Gp_Dword1
       {
            unsigned int     Global_Size_X         : 32;

       } Cmd_Gp_Dword1;
       typedef struct _Cmd_Gp_Dword2
       {
            unsigned int     Global_Size_Y         : 32;
       } Cmd_Gp_Dword2;
       typedef struct _Cmd_Gp_Dword3
       {
            unsigned int     Global_Size_Z         : 32;
       } Cmd_Gp_Dword3;
       typedef struct _Cmd_Gp_Dword4
       {
            unsigned int     Work_Group_Size_X     : 32;
       } Cmd_Gp_Dword4;
       typedef struct _Cmd_Gp_Dword5
       {
            unsigned int     Work_Group_Size_Y     : 32;
       } Cmd_Gp_Dword5;
       typedef struct _Cmd_Gp_Dword6
       {
            unsigned int     Work_Group_Size_Z     : 32;
       } Cmd_Gp_Dword6;
       typedef struct _Cmd_Gp_Dword7
       {
            unsigned int     Global_Id_Offset_X    : 32;
       } Cmd_Gp_Dword7;
       typedef struct _Cmd_Gp_Dword8
       {
            unsigned int     Global_Id_Offset_Y    : 32;
       } Cmd_Gp_Dword8;
       typedef struct _Cmd_Gp_Dword9
       {
            unsigned int     Global_Id_Offset_Z    : 32;
       } Cmd_Gp_Dword9;
       typedef struct _Cmd_Gp_Dword10
       {
            unsigned int     Shader_Offset         : 32;
       } Cmd_Gp_Dword10;
       typedef struct _Cmd_Gp_Dword11
       {
            unsigned int     Shader_Range          : 32;
       } Cmd_Gp_Dword11;
       typedef struct _Cmd_Gp_Dword12
       {
            unsigned int     Argumentaddr_X        : 32;
       } Cmd_Gp_Dword12;
       typedef struct _Cmd_Gp_Dword13
       {
            unsigned int     Argumentaddr_Y        : 32;
       } Cmd_Gp_Dword13;
       typedef struct _Cmd_Indirect_Gp_Dword1
       {
            unsigned int     Address_Low32         : 32;
       } Cmd_Indirect_Gp_Dword1;
       typedef struct _Cmd_Indirect_Gp_Dword2
       {
            unsigned int     Address_High8         : 8;
            unsigned int     L2_Cachable           : 1;

            unsigned int     Reserved              : 23;
       } Cmd_Indirect_Gp_Dword2;




typedef struct _Cmd_Tbr_Indicator
{
    unsigned int     Reserved              : 20;
    unsigned int     Skip_En               : 1;


    unsigned int     Indicator_Info        : 2;
    unsigned int     Block_Id              : 5;
    unsigned int     Major_Opcode          : 4;
} Cmd_Tbr_Indicator;

typedef struct _Cmd_Vpp

{
    unsigned int     Dwf                   : 3;
    unsigned int     Reserved              : 7;
    unsigned int     Clear_Mode            : 3;
    unsigned int     Color_Mode            : 1;
    unsigned int     Counter               : 11;
    unsigned int     Auto_Clear            : 1;
    unsigned int     Flush_Blc             : 1;
    unsigned int     Invalidate_Blc        : 1;
    unsigned int     Major_Opcode          : 4;
} Cmd_Vpp;

typedef union Csp_Opcodes_cmds
{
    Cmd_Skip                        cmd_Skip;
    Cmd_Dma                         cmd_Dma;
    Cmd_Wait                        cmd_Wait;
    Cmd_Query_Dump                  cmd_Query_Dump;
    Cmd_Set_Register                cmd_Set_Register;
    Cmd_Set_Object                  cmd_Set_Object;
    Cmd_Dip                         cmd_Dip;
    Cmd_Fence                       cmd_Fence;
    Cmd_Block_Command_Template      cmd_Block_Command_Template;
    Cmd_Block_Command_Tas           cmd_Block_Command_Tas;
    Cmd_Block_Command_Sg            cmd_Block_Command_Sg;
    Cmd_Block_Command_Img_Trn       cmd_Block_Command_Img_Trn;
    Cmd_Block_Command_Flush         cmd_Block_Command_Flush;
    Cmd_Block_Command_Eu            cmd_Block_Command_Eu;
    Cmd_Block_Command_Tu            cmd_Block_Command_Tu;
    Cmd_Block_Command_L2            cmd_Block_Command_L2;
    Cmd_Block_Command_Mxu           cmd_Block_Command_Mxu;
    Cmd_Block_Command_Csp           cmd_Block_Command_Csp;
    Cmd_Blk_Cmd_Csp_Blc             cmd_Blk_Cmd_Csp_Blc;
    Cmd_Blk_Cmd_Csp_Save_Rsto       cmd_Blk_Cmd_Csp_Save_Rsto;
    Cmd_Blk_Cmd_Csp_Indicator       cmd_Blk_Cmd_Csp_Indicator;
    Cmd_Blk_Cmd_Csp_Ld_Des          cmd_Blk_Cmd_Csp_Ld_Des;
    Cmd_Block_Command_Video         cmd_Block_Command_Video;
    Cmd_Gp                          cmd_Gp;
    Cmd_Tbr_Indicator               cmd_Tbr_Indicator;
    Cmd_Vpp                         cmd_Vpp;
    unsigned int                    uint ;
}Csp_Opcodes_cmd;

#endif
