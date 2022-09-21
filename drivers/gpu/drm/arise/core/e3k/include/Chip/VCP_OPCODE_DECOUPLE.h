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

#ifndef _VCP_OPCODE_DECOUPLE_H
#define _VCP_OPCODE_DECOUPLE_H

#ifndef        COMMAND_OPCODES_BLOCKBASE_INF
    #define    COMMAND_OPCODES_BLOCKBASE_INF
    #define    BLOCK_COMMAND_OPCODES_VERSION 1
    #define    BLOCK_COMMAND_OPCODES_TIMESTAMP "2019/2/18 17:08:36"
#endif


#define  VCP_OPCODE_Vcp_Skip                0x0
#define  VCP_OPCODE_Vcp_Set_Data_Buf        0x1
#define  VCP_OPCODE_Vcp_Internal_Wait       0x2
#define  VCP_OPCODE_Vcp_Query_Dump          0x3
#define  VCP_OPCODE_Vcp_Set_Register        0x4
#define  VCP_OPCODE_Vcp_Set_Register_Short  0x5
#define  VCP_OPCODE_Vcp_Kickoff             0x7
#define  VCP_OPCODE_Vcp_Fence               0x8
#define  VCP_OPCODE_Vcp_Indicator           0x9
#define  VCP_OPCODE_Vcp_Autoclear           0xC
#define  VCP_OPCODE_Vcp_Init                0xD
#define  VCP_OPCODE_Vcp_Aes                 0xE


typedef enum
{
    VCP_SET_DATA_BUF_DATA_EXCH_TYPE_MEM      = 0,
    VCP_SET_DATA_BUF_DATA_EXCH_TYPE_FIFO     = 1,
} VCP_SET_DATA_BUF_DATA_EXCH_TYPE;
typedef enum
{
    VCP_INTERNAL_WAIT_WAIT_MODE_NORMAL_WAIT  = 0,
    VCP_INTERNAL_WAIT_WAIT_MODE_KKK_WAIT     = 1,
    VCP_INTERNAL_WAIT_WAIT_MODE_WAIT_CHIP_IDLE= 2,
    VCP_INTERNAL_WAIT_WAIT_MODE_EXTERNAL_WAIT= 3,
} VCP_INTERNAL_WAIT_WAIT_MODE;
typedef enum
{
    VCP_INTERNAL_WAIT_METHOD_BIGEQUAL        = 0,
    VCP_INTERNAL_WAIT_METHOD_EQUAL           = 1,
} VCP_INTERNAL_WAIT_METHOD;
typedef enum
{
    VCP_INTERNAL_WAIT_STATION_ID_MAIN_PARSER= 0,
    VCP_INTERNAL_WAIT_STATION_ID_PRE_PARSER  = 1,

} VCP_INTERNAL_WAIT_STATION_ID;
typedef enum
{
    VCP_INTERNAL_WAIT_SLOT_ID_TO_WAIT_VCP0_BE= 8,
    VCP_INTERNAL_WAIT_SLOT_ID_TO_WAIT_VCP0_FE= 9,
    VCP_INTERNAL_WAIT_SLOT_ID_TO_WAIT_VCP1_BE= 11,
    VCP_INTERNAL_WAIT_SLOT_ID_TO_WAIT_VCP1_FE= 12,
} VCP_INTERNAL_WAIT_SLOT_ID;
typedef enum
{
    VCP_INTERNAL_WAIT_FE_BE_FLAG_VCP_FE      = 0,
    VCP_INTERNAL_WAIT_FE_BE_FLAG_VCP_BE      = 1,
} VCP_INTERNAL_WAIT_FE_BE_FLAG;
typedef enum
{
    VCP_QUERY_DUMP_IRQ_NOP                   = 0,
    VCP_QUERY_DUMP_IRQ_INTERRUPT             = 1,
} VCP_QUERY_DUMP_IRQ;
typedef enum
{
    VCP_QUERY_DUMP_DATA_TYPE_SIGNATURE       = 0,
    VCP_QUERY_DUMP_DATA_TYPE_COUNTER         = 1,
    VCP_QUERY_DUMP_DATA_TYPE_OTHERS          = 2,
} VCP_QUERY_DUMP_DATA_TYPE;
typedef enum
{
    VCP_QUERY_DUMP_BLOCK_ID_VCP_FE           = 0,
    VCP_QUERY_DUMP_BLOCK_ID_VCP_BE           = 1,
} VCP_QUERY_DUMP_BLOCK_ID;
typedef enum
{
    VCP_SET_REGISTER_SHORT_SET_WHOLE_BYTE_SET_NIBBLE_MASK= 0,

    VCP_SET_REGISTER_SHORT_SET_WHOLE_BYTE_SET_WHOLE_BYTE= 1,

} VCP_SET_REGISTER_SHORT_SET_WHOLE_BYTE;
typedef enum
{
    VCP_KICKOFF_KICKOFF_MODE_START           = 0,

    VCP_KICKOFF_KICKOFF_MODE_EXECUTE         = 1,
} VCP_KICKOFF_KICKOFF_MODE;
typedef enum
{
    VCP_FENCE_IRQ_NOP                        = 0,
    VCP_FENCE_IRQ_INTERRUPT                  = 1,
    VCP_FENCE_IRQ_INTERRUPT_GPU              = 2,




} VCP_FENCE_IRQ;
typedef enum
{
    VCP_FENCE_FENCE_TYPE_INTERNAL            = 0,
    VCP_FENCE_FENCE_TYPE_EXTERNAL32          = 2,
    VCP_FENCE_FENCE_TYPE_EXTERNAL64          = 3,


} VCP_FENCE_FENCE_TYPE;
typedef enum
{
    VCP_FENCE_SLOT_ID_INTERNAL_FENCE_VCP0_BE= 8,
    VCP_FENCE_SLOT_ID_INTERNAL_FENCE_VCP0_FE= 9,
    VCP_FENCE_SLOT_ID_INTERNAL_FENCE_VCP1_BE= 11,
    VCP_FENCE_SLOT_ID_INTERNAL_FENCE_VCP1_FE= 12,
} VCP_FENCE_SLOT_ID;
typedef enum
{
    VCP_FENCE_FENCE_UPDATE_MODE_COPY         = 0,
    VCP_FENCE_FENCE_UPDATE_MODE_OR           = 1,
} VCP_FENCE_FENCE_UPDATE_MODE;
typedef enum
{
    VCP_FENCE_RB_TYPE_3DFE                   = 0,
    VCP_FENCE_RB_TYPE_3DBE                   = 1,
    VCP_FENCE_RB_TYPE_CSL                    = 2,
    VCP_FENCE_RB_TYPE_CSH                    = 3,
} VCP_FENCE_RB_TYPE;
typedef enum
{
    VCP_FENCE_BLOCK_ID_VCP_FE                = 0,
    VCP_FENCE_BLOCK_ID_VCP_BE                = 1,
} VCP_FENCE_BLOCK_ID;
typedef enum
{
    VCP_AES_MODE_NONE                        = 0,
    VCP_AES_MODE_XOR                         = 1,
    VCP_AES_MODE_AES_CTR                     = 2,
    VCP_AES_MODE_CIPHER                      = 3,
} VCP_AES_MODE;





typedef struct _Cmd_Vcp_Skip
{
    unsigned int     Dwc                   : 22;
    unsigned int     Reserved              : 2;
    unsigned int     Block_Id              : 4;
    unsigned int     Major_Opcode          : 4;
} Cmd_Vcp_Skip;

typedef struct _Cmd_Vcp_Set_Data_Buf
{
    unsigned int     Dwc                   : 4;
    unsigned int     Reset_Buf_Base_En     : 1;
    unsigned int     Reserved1             : 1;
    unsigned int     Bandwidth_Limit_En    : 1;
    unsigned int     Data_Exch_Type        : 1;
    unsigned int     Reserved2             : 16;
    unsigned int     Block_Id              : 4;
    unsigned int     Major_Opcode          : 4;
} Cmd_Vcp_Set_Data_Buf;


       typedef struct _Cmd_Vcp_Buf_Base_Addr_L32
       {
            unsigned int     Reserved              : 5;
            unsigned int     Buf_Base_L            : 27;
       } Cmd_Vcp_Buf_Base_Addr_L32;
       typedef struct _Cmd_Vcp_Buf_Base_Addr_H32
       {
            unsigned int     Address_H8            : 8;
            unsigned int     Bl_Slot_Index         : 18;
            unsigned int     Reserved              : 6;
       } Cmd_Vcp_Buf_Base_Addr_H32;
       typedef struct _Cmd_Vcp_Buf_Size
       {
            unsigned int     Reserved              : 5;
            unsigned int     Buf_Size              : 27;
       } Cmd_Vcp_Buf_Size;
       typedef struct _Cmd_Vcp_Bandwidth_Size
       {
            unsigned int     Reserved              : 5;
            unsigned int     Bandwdith_Size        : 27;
       } Cmd_Vcp_Bandwidth_Size;




typedef struct _Cmd_Vcp_Internal_Wait
{
    unsigned int     Ref_Value             : 16;
    unsigned int     Wait_Mode             : 2;



    unsigned int     Method                : 1;
    unsigned int     Station_Id            : 3;
    unsigned int     Slot_Id               : 5;

    unsigned int     Fe_Be_Flag            : 1;
    unsigned int     Major_Opcode          : 4;
} Cmd_Vcp_Internal_Wait;


       typedef struct _Cmd_Vcp_Internal_Wait_Dword1
       {
            unsigned int     Event_Flag_Idx1       : 16;



            unsigned int     Event_Flag_Idx2       : 16;
       } Cmd_Vcp_Internal_Wait_Dword1;




typedef struct _Cmd_Vcp_Query_Dump
{
    unsigned int     Dwc                   : 13;
    unsigned int     Irq                   : 1;
    unsigned int     Data_Type             : 2;
    unsigned int     Reg_Offset            : 8;
    unsigned int     Block_Id              : 4;
    unsigned int     Major_Opcode          : 4;

} Cmd_Vcp_Query_Dump;


       typedef struct _Cmd_Vcp_Query_Dump_Address_Dword_L32

       {
            unsigned int     Reserved              : 2;
            unsigned int     Address_L             : 30;
       } Cmd_Vcp_Query_Dump_Address_Dword_L32;
       typedef struct _Cmd_Vcp_Query_Dump_Address_Dword_H32

       {
            unsigned int     Address_H8            : 8;
            unsigned int     Bl_Slot_Index         : 18;
            unsigned int     Reserved              : 6;
       } Cmd_Vcp_Query_Dump_Address_Dword_H32;




typedef struct _Cmd_Vcp_Set_Register
{
    unsigned int     Dwc                   : 7;

    unsigned int     Reserved              : 9;
    unsigned int     Start_Offset          : 8;
    unsigned int     Block_Id              : 4;
    unsigned int     Major_Opcode          : 4;
} Cmd_Vcp_Set_Register;


       typedef struct _Cmd_Vcp_Set_Register_Value_Dword

       {
            unsigned int     Reg_Val               : 32;
       } Cmd_Vcp_Set_Register_Value_Dword;




typedef struct _Cmd_Vcp_Set_Register_Short
{
    unsigned int     Register_Value        : 8;



    unsigned int     Nibble_Mask           : 4;
    unsigned int     Set_Whole_Byte        : 1;
    unsigned int     Nibble_Offset         : 3;
    unsigned int     Reserved              : 8;
    unsigned int     Block_Id              : 4;
    unsigned int     Major_Opcode          : 4;
} Cmd_Vcp_Set_Register_Short;


       typedef struct _Cmd_Vcp_Set_Register_Short_Offset_Dword

       {
            unsigned int     Register_Offset       : 13;
            unsigned int     Reserved              : 19;
       } Cmd_Vcp_Set_Register_Short_Offset_Dword;




typedef struct _Cmd_Vcp_Kickoff
{
    unsigned int     Dwc                   : 4;
    unsigned int     Kickoff_Mode          : 1;
    unsigned int     Set_Dma_Flag          : 1;
    unsigned int     Sub_Frame_End         : 1;
    unsigned int     Secure_Video          : 1;
    unsigned int     Slice_Group_Idx       : 2;
    unsigned int     Frame_Idx             : 6;
    unsigned int     Sequence_Idx          : 7;
    unsigned int     Stop_Reset_Enable     : 1;
    unsigned int     Block_Id              : 4;
    unsigned int     Major_Opcode          : 4;
} Cmd_Vcp_Kickoff;


       typedef struct _Cmd_Vcp_Data_Address_L32
       {
            unsigned int     Address_L             : 32;
       } Cmd_Vcp_Data_Address_L32;
       typedef struct _Cmd_Vcp_Data_Address_H32
       {
            unsigned int     Address_H8            : 8;
            unsigned int     Bl_Slot_Index         : 18;
            unsigned int     Reserved              : 6;
       } Cmd_Vcp_Data_Address_H32;
       typedef struct _Cmd_Vcp_Output_Address_L32
       {
            unsigned int     Address_L             : 32;

       } Cmd_Vcp_Output_Address_L32;
       typedef struct _Cmd_Vcp_Output_Address_H32
       {
            unsigned int     Address_H8            : 8;
            unsigned int     Bl_Slot_Index         : 18;
            unsigned int     Reserved              : 6;
       } Cmd_Vcp_Output_Address_H32;
       typedef struct _Cmd_Vcp_Pc_L32
       {
            unsigned int     Pc_Value              : 24;

            unsigned int     Reserved              : 8;
       } Cmd_Vcp_Pc_L32;
       typedef struct _Cmd_Vcp_Pc_H32
       {
            unsigned int     Address_H8            : 8;
            unsigned int     Bl_Slot_Index         : 18;
            unsigned int     Reserved              : 6;
       } Cmd_Vcp_Pc_H32;
       typedef struct _Cmd_Vcp_Top_Mvd_Surf_L32
       {
            unsigned int     Address_L             : 32;

       } Cmd_Vcp_Top_Mvd_Surf_L32;
       typedef struct _Cmd_Vcp_Top_Mvd_Surf_H32
       {
            unsigned int     Address_H8            : 8;
            unsigned int     Bl_Slot_Index         : 18;
            unsigned int     Reserved              : 6;
       } Cmd_Vcp_Top_Mvd_Surf_H32;




typedef struct _Cmd_Vcp_Fence
{
    unsigned int     Dwc                   : 3;

    unsigned int     Irq                   : 2;
    unsigned int     Fence_Type            : 2;
    unsigned int     Slot_Id               : 5;
    unsigned int     Reserved              : 1;
    unsigned int     Fence_Update_Mode     : 1;
    unsigned int     Reserved1             : 8;
    unsigned int     Rb_Type               : 2;

    unsigned int     Block_Id              : 4;

    unsigned int     Major_Opcode          : 4;

} Cmd_Vcp_Fence;


       typedef struct _Cmd_Vcp_Fence_Internal_Dword1
       {
            unsigned int     Update_Value          : 16;
            unsigned int     Slice_Mask            : 16;



       } Cmd_Vcp_Fence_Internal_Dword1;
       typedef struct _Cmd_Vcp_External_Fence_Command_Dword0


       {
            unsigned int     External_Addr_Low32   : 32;
       } Cmd_Vcp_External_Fence_Command_Dword0;
       typedef struct _Cmd_Vcp_External_Fence_Command_Dword1


       {
            unsigned int     External_Addr_High8   : 8;
            unsigned int     Reserved              : 23;
            unsigned int     Fence_Update_Timing   : 1;

       } Cmd_Vcp_External_Fence_Command_Dword1;
       typedef struct _Cmd_Vcp_External_Fence_Command_Dword2


       {
            unsigned int     External_Data1        : 32;
       } Cmd_Vcp_External_Fence_Command_Dword2;
       typedef struct _Cmd_Vcp_External_Fence_Command_Dword3


       {
            unsigned int     External_Data2        : 32;
       } Cmd_Vcp_External_Fence_Command_Dword3;




typedef struct _Cmd_Vcp_Indicator
{
    unsigned int     Dwc                   : 3;
    unsigned int     Reserved              : 10;
    unsigned int     Process_Id            : 4;
    unsigned int     Reserved1             : 7;
    unsigned int     Block_Id              : 4;
    unsigned int     Major_Opcode          : 4;
} Cmd_Vcp_Indicator;

typedef struct _Cmd_Vcp_Autoclear
{
    unsigned int     Dwc                   : 3;
    unsigned int     Counter               : 17;

    unsigned int     Reserved              : 4;
    unsigned int     Block_Id              : 4;
    unsigned int     Major_Opcode          : 4;
} Cmd_Vcp_Autoclear;


       typedef struct _Cmd_Vcp_Autoclear_Dword0
       {
            unsigned int     Reserved              : 5;
            unsigned int     Start_Address_L       : 27;

       } Cmd_Vcp_Autoclear_Dword0;
       typedef struct _Cmd_Vcp_Autoclear_Dword1
       {
            unsigned int     Address_H8            : 8;
            unsigned int     Bl_Slot_Index         : 18;

            unsigned int     Reserved              : 6;
       } Cmd_Vcp_Autoclear_Dword1;
       typedef struct _Cmd_Vcp_Autoclear_Dword2
       {
            unsigned int     Reserved              : 24;
            unsigned int     Clear_Value1          : 4;
            unsigned int     Clear_Value0          : 4;
       } Cmd_Vcp_Autoclear_Dword2;
       typedef struct _Cmd_Vcp_Autoclear_Dword3
       {
            unsigned int     Clear_Mask0           : 32;





       } Cmd_Vcp_Autoclear_Dword3;
       typedef struct _Cmd_Vcp_Autoclear_Dword4
       {
            unsigned int     Clear_Mask1           : 32;





       } Cmd_Vcp_Autoclear_Dword4;
       typedef struct _Cmd_Vcp_Autoclear_Dword5
       {
            unsigned int     Clear_Mask2           : 32;





       } Cmd_Vcp_Autoclear_Dword5;
       typedef struct _Cmd_Vcp_Autoclear_Dword6
       {
            unsigned int     Clear_Mask3           : 32;





       } Cmd_Vcp_Autoclear_Dword6;




typedef struct _Cmd_Vcp_Init
{
    unsigned int     Dbg_Shader_Load_En    : 1;
    unsigned int     Cmodel_Mem_Check_En   : 1;

    unsigned int     Hw_Cross_Mem_Check_En : 1;
    unsigned int     Codec_Type            : 8;
    unsigned int     Reserved              : 2;
    unsigned int     Flush_Blc             : 1;
    unsigned int     Invalidate_Blc        : 1;
    unsigned int     Timeout_Reset         : 1;
    unsigned int     Vcp_Reset             : 1;
    unsigned int     Flush                 : 1;
    unsigned int     Invalidate_Ic         : 1;
    unsigned int     Invalidate_Dc         : 1;
    unsigned int     Dwc                   : 4;
    unsigned int     Block_Id              : 4;
    unsigned int     Major_Opcode          : 4;
} Cmd_Vcp_Init;


       typedef struct _Cmd_Vcp_Blc_Surface_Addr_L32
       {
            unsigned int     Surf_Addr_L           : 32;
       } Cmd_Vcp_Blc_Surface_Addr_L32;
       typedef struct _Cmd_Vcp_Blc_Surface_Addr_H32
       {
            unsigned int     Address_H8            : 8;
            unsigned int     Bl_Slot_Index         : 18;
            unsigned int     Reserved              : 6;
       } Cmd_Vcp_Blc_Surface_Addr_H32;
       typedef struct _Cmd_Vcp_Blc_Surface_Size
       {
            unsigned int     Byte_Size             : 32;
       } Cmd_Vcp_Blc_Surface_Size;
       typedef struct _Cmd_Vcp_Timeout_Reset_Value
       {
            unsigned int     Cycle_Count           : 32;
       } Cmd_Vcp_Timeout_Reset_Value;




typedef struct _Cmd_Vcp_Aes
{
    unsigned int     Mode                  : 2;
    unsigned int     Remove003_En          : 1;
    unsigned int     Remove002_En          : 1;

    unsigned int     To_Memory             : 1;
    unsigned int     Secure_Video          : 1;
    unsigned int     Reserved              : 15;

    unsigned int     Dwc                   : 3;

    unsigned int     Block_Id              : 4;
    unsigned int     Major_Opcode          : 4;
} Cmd_Vcp_Aes;


       typedef struct _Cmd_Vcp_Aes_Source_Addr_L32
       {
            unsigned int     Start_Byte            : 5;
            unsigned int     Aes_Source_Addr_L     : 27;
       } Cmd_Vcp_Aes_Source_Addr_L32;
       typedef struct _Cmd_Vcp_Aes_Source_Addr_H32
       {
            unsigned int     Address_H8            : 8;
            unsigned int     Bl_Slot_Index         : 18;
            unsigned int     Reserved              : 6;
       } Cmd_Vcp_Aes_Source_Addr_H32;
       typedef struct _Cmd_Vcp_Aes_Size
       {
            unsigned int     Byte_Size             : 32;
       } Cmd_Vcp_Aes_Size;
       typedef struct _Cmd_Vcp_Aes_Dst_Addr_L32
       {
            unsigned int     Start_Byte            : 5;
            unsigned int     Aes_Dst_Addr_L        : 27;
       } Cmd_Vcp_Aes_Dst_Addr_L32;
       typedef struct _Cmd_Vcp_Aes_Dst_Addr_H32
       {
            unsigned int     Address_H8            : 8;
            unsigned int     Bl_Slot_Index         : 18;
            unsigned int     Reserved              : 6;
       } Cmd_Vcp_Aes_Dst_Addr_H32;

typedef union Vcp_Decouple_Opcodes_cmds
{
    unsigned int                    uint ;
    Cmd_Vcp_Skip                    cmd_Vcp_Skip;
    Cmd_Vcp_Set_Data_Buf            cmd_Vcp_Set_Data_Buf;
    Cmd_Vcp_Internal_Wait           cmd_Vcp_Internal_Wait;
    Cmd_Vcp_Query_Dump              cmd_Vcp_Query_Dump;
    Cmd_Vcp_Set_Register            cmd_Vcp_Set_Register;
    Cmd_Vcp_Set_Register_Short      cmd_Vcp_Set_Register_Short;
    Cmd_Vcp_Kickoff                 cmd_Vcp_Kickoff;
    Cmd_Vcp_Fence                   cmd_Vcp_Fence;
    Cmd_Vcp_Indicator               cmd_Vcp_Indicator;
    Cmd_Vcp_Autoclear               cmd_Vcp_Autoclear;
    Cmd_Vcp_Init                    cmd_Vcp_Init;
    Cmd_Vcp_Aes                     cmd_Vcp_Aes;
}Vcp_Decouple_Opcodes_cmd;

#endif
