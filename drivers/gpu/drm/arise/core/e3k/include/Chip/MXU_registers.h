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

#ifndef _MXU_REGISTERS_H
#define _MXU_REGISTERS_H


#ifndef        MXU_BLOCKBASE_INF
    #define    MXU_BLOCKBASE_INF
    #define    BLOCK_MXU_VERSION 1
    #define    BLOCK_MXU_TIMESTAMP "2019/1/25 14:57:16"
    #define    MXU_BLOCK                                                  0xC
    #define    MXU_REG_START                                              0x0
    #define    MXU_REG_END                                                0x1FFF
    #define    MXU_REG_LIMIT                                              0x1FFF
#endif


#define        Reg_Fb_Ctrl_Offset                                         0x0
#define        Reg_Mmu_Mode_Offset                                        0x1
#define        Reg_Rserved_0_Offset                                       0x2
#define        Reg_Bl_Size_Offset                                         0x3
#define        Reg_Gart_Base_Offset                                       0x4
#define        Reg_Proc_Sec_Offset                                        0x5
#define        Reg_Pt_Inv_Addr_Offset                                     0x6
#define        Reg_Pt_Inv_Mask_Offset                                     0x7
#define        Reg_Pt_Inv_Trig_Offset                                     0x8
#define        Reg_Mxu_Channel_Control_Offset                             0x9
#define        Reg_Mxu_Dec_Ctrl_Offset                                    0xA
#define        Reg_Bus_Id_Mode_Offset                                     0xB
#define        Reg_Mxu_Ctrl_0_Offset                                      0xC
#define        Reg_Mxu_Ila_Reg_Offset                                     0xD
#define        Reg_Mxu_Int_Status_Offset                                  0xE
#define        Reg_Mxu_Int_Mask_Offset                                    0xF
#define        Reg_Mxu_Page_Fault_Status0_Offset                          0x10
#define        Reg_Mxu_Page_Fault_Status1_Offset                          0x11
#define        Reg_Mem_Addr_For_Page_Fault_Offset                         0x12
#define        Reg_Level_1_Fault_Page_Addr_Offset                         0x13
#define        Reg_Level_2_Fault_Page_Addr_Offset                         0x14
#define        Reg_Rb0_Fl2_Offset                                         0x15
#define        Reg_Rb1_Fl2_Offset                                         0x16
#define        Reg_Rb2_Fl2_Offset                                         0x17
#define        Reg_Argument_Fl2_Offset                                    0x18
#define        Reg_L2_Performance_Redundancy_Offset                       0x19
#define        Reg_Page_Fault_Level_Offset                                0x1A
#define        Reg_Pending_Buf_Ctrl_0_Offset                              0x1B
#define        Reg_Pending_Buf_Ctrl_1_Offset                              0x1C
#define        Reg_Rdata_Buf_Ctrl_0_Offset                                0x1D
#define        Reg_Diu_Reserve_Ctrl_Offset                                0x1E
#define        Reg_Bium_Hdr_Num_Offset                                    0x1F
#define        Reg_Mxu_Reserved_Offset                                    0x20
#define        Reg_Mxu_Resvered_For_Sila_Offset                           0x7FF


typedef enum
{
    BL_SIZE_BUFFER_SIZE_8MB                  = 0,
    BL_SIZE_BUFFER_SIZE_16MB                 = 1,
    BL_SIZE_BUFFER_SIZE_32MB                 = 2,
    BL_SIZE_BUFFER_SIZE_64MB                 = 3,
    BL_SIZE_BUFFER_SIZE_128MB                = 4,
    BL_SIZE_BUFFER_SIZE_256MB                = 5,
    BL_SIZE_BUFFER_SIZE_RESERVED1            = 6,
    BL_SIZE_BUFFER_SIZE_RESERVED2            = 7,
} BL_SIZE_BUFFER_SIZE;
typedef enum
{
    PT_INV_TRIG_TARGET_OFF                   = 0,
    PT_INV_TRIG_TARGET_UTLB                  = 1,
    PT_INV_TRIG_TARGET_DTLB                  = 2,
    PT_INV_TRIG_TARGET_RESERVED              = 3,
} PT_INV_TRIG_TARGET;
typedef enum
{
    MXU_CHANNEL_CONTROL_CH_SIZE_256B         = 0,
    MXU_CHANNEL_CONTROL_CH_SIZE_512B         = 1,
    MXU_CHANNEL_CONTROL_CH_SIZE_1KB          = 2,
    MXU_CHANNEL_CONTROL_CH_SIZE_RESERVED0    = 3,
    MXU_CHANNEL_CONTROL_CH_SIZE_RESERVED1    = 4,
    MXU_CHANNEL_CONTROL_CH_SIZE_RESERVED2    = 5,
    MXU_CHANNEL_CONTROL_CH_SIZE_RESERVED3    = 6,
    MXU_CHANNEL_CONTROL_CH_SIZE_RESERVED4    = 7,
} MXU_CHANNEL_CONTROL_CH_SIZE;





typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Fb_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Vmen                      : 1;

        unsigned int Reserved0                 : 3;
        unsigned int Bius_Va_En                : 1;



        unsigned int Reserved1                 : 3;
        unsigned int Video_Size                : 8;



        unsigned int Legacy_Size               : 8;


        unsigned int Legacy_Offset             : 8;



    } reg;
} Reg_Mmu_Mode;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Rserved_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Buffer_Size               : 3;
        unsigned int Reserved                  : 1;
        unsigned int Buffer_Offset             : 19;



        unsigned int Reserved0                 : 1;
        unsigned int Invalid_Bl_To_Rf_En       : 1;
        unsigned int Reserved1                 : 7;
    } reg;
} Reg_Bl_Size;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 8;
        unsigned int Addr                      : 24;

    } reg;
} Reg_Gart_Base;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Sec_Status                : 16;
        unsigned int Reserved                  : 16;
    } reg;
} Reg_Proc_Sec;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Address                   : 28;

        unsigned int Proc_Id                   : 4;

    } reg;
} Reg_Pt_Inv_Addr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Mask                      : 28;

        unsigned int Proc_Id                   : 4;

    } reg;
} Reg_Pt_Inv_Mask;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Proc_Id                   : 4;

        unsigned int Addr_Valid                : 1;


        unsigned int Target                    : 2;
        unsigned int Reserved                  : 25;
    } reg;
} Reg_Pt_Inv_Trig;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Miu_Channel0_Disable      : 1;
        unsigned int Miu_Channel1_Disable      : 1;
        unsigned int Miu_Channel2_Disable      : 1;
        unsigned int Ch_Size                   : 3;

        unsigned int Reserved                  : 26;
    } reg;
} Reg_Mxu_Channel_Control;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ddec_Antilock_Time_Threshold : 3;





        unsigned int Ddec_Antilock             : 1;



        unsigned int Reserved                  : 28;
    } reg;
} Reg_Mxu_Dec_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Biu_Axi_Id_Mode           : 1;

        unsigned int Reserved0                 : 31;

    } reg;
} Reg_Bus_Id_Mode;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved0                 : 1;
        unsigned int Reseved1                  : 2;



        unsigned int Reserved3                 : 2;



        unsigned int Reserved2                 : 27;
    } reg;
} Reg_Mxu_Ctrl_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Latency                   : 10;

        unsigned int Reserve                   : 22;
    } reg;
} Reg_Mxu_Ila_Reg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Page_Fault                : 30;
        unsigned int Reserved1                 : 1;
        unsigned int Reserved0                 : 1;
    } reg;
} Reg_Mxu_Int_Status;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved0                 : 30;
        unsigned int Reserved1                 : 1;
        unsigned int Reserved2                 : 1;
    } reg;
} Reg_Mxu_Int_Mask;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 18;
        unsigned int Fault_Level               : 2;




        unsigned int Proc_Id                   : 4;
        unsigned int Va                        : 8;

    } reg;
} Reg_Mxu_Page_Fault_Status0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Va                        : 32;
    } reg;
} Reg_Mxu_Page_Fault_Status1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 2;
        unsigned int Va                        : 30;

    } reg;
} Reg_Mem_Addr_For_Page_Fault;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 2;
        unsigned int Va                        : 30;
    } reg;
} Reg_Level_1_Fault_Page_Addr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 2;
        unsigned int Va                        : 30;
    } reg;
} Reg_Level_2_Fault_Page_Addr;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Addr                 : 20;
        unsigned int Size                      : 12;
    } reg;
} Reg_Rb0_Fl2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Addr                 : 20;
        unsigned int Size                      : 12;
    } reg;
} Reg_Rb1_Fl2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Addr                 : 20;
        unsigned int Size                      : 12;
    } reg;
} Reg_Rb2_Fl2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Base_Addr                 : 20;
        unsigned int Size                      : 12;
    } reg;
} Reg_Argument_Fl2;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 22;
    } reg;
} Reg_L2_Performance_Redundancy;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Mmu_L1_Invalid            : 2;
        unsigned int Mmu_L2_Invalid            : 30;
        unsigned int Reserved                  : 30;
    } reg;
} Reg_Page_Fault_Level;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ctrl_0                    : 32;
    } reg;
} Reg_Pending_Buf_Ctrl_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Ctrl_1                    : 32;
    } reg;
} Reg_Pending_Buf_Ctrl_1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Rdata_Ctrl_0              : 32;
    } reg;
} Reg_Rdata_Buf_Ctrl_0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Wr_Entry_Reserve_For_Diu : 2;
        unsigned int Diu_Rdata_Buf_Cnt_Ctrl_En : 3;
        unsigned int Rdata_Ctrl_0              : 31;

    } reg;
} Reg_Diu_Reserve_Ctrl;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserve0                  : 15;
        unsigned int Read_Out_Of_Order_Output : 1;
        unsigned int Reserve1                  : 15;
    } reg;
} Reg_Bium_Hdr_Num;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Mxu_Reserved;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Mxu_Resvered_For_Sila;

typedef struct _Mxu_regs
{
    Reg_Fb_Ctrl                      reg_Fb_Ctrl;
    Reg_Mmu_Mode                     reg_Mmu_Mode;
    Reg_Rserved_0                    reg_Rserved_0;
    Reg_Bl_Size                      reg_Bl_Size;
    Reg_Gart_Base                    reg_Gart_Base;
    Reg_Proc_Sec                     reg_Proc_Sec;
    Reg_Pt_Inv_Addr                  reg_Pt_Inv_Addr;
    Reg_Pt_Inv_Mask                  reg_Pt_Inv_Mask;
    Reg_Pt_Inv_Trig                  reg_Pt_Inv_Trig;
    Reg_Mxu_Channel_Control          reg_Mxu_Channel_Control;
    Reg_Mxu_Dec_Ctrl                 reg_Mxu_Dec_Ctrl;
    Reg_Bus_Id_Mode                  reg_Bus_Id_Mode;
    Reg_Mxu_Ctrl_0                   reg_Mxu_Ctrl_0;
    Reg_Mxu_Ila_Reg                  reg_Mxu_Ila_Reg;
    Reg_Mxu_Int_Status               reg_Mxu_Int_Status;
    Reg_Mxu_Int_Mask                 reg_Mxu_Int_Mask;
    Reg_Mxu_Page_Fault_Status0       reg_Mxu_Page_Fault_Status0;
    Reg_Mxu_Page_Fault_Status1       reg_Mxu_Page_Fault_Status1;
    Reg_Mem_Addr_For_Page_Fault      reg_Mem_Addr_For_Page_Fault;
    Reg_Level_1_Fault_Page_Addr      reg_Level_1_Fault_Page_Addr;
    Reg_Level_2_Fault_Page_Addr      reg_Level_2_Fault_Page_Addr;
    Reg_Rb0_Fl2                      reg_Rb0_Fl2;
    Reg_Rb1_Fl2                      reg_Rb1_Fl2;
    Reg_Rb2_Fl2                      reg_Rb2_Fl2;
    Reg_Argument_Fl2                 reg_Argument_Fl2;
    Reg_L2_Performance_Redundancy    reg_L2_Performance;
    Reg_Page_Fault_Level             reg_Page_Fault_Level;
    Reg_Pending_Buf_Ctrl_0           reg_Pending_Buf_Ctrl_0;
    Reg_Pending_Buf_Ctrl_1           reg_Pending_Buf_Ctrl_1;
    Reg_Rdata_Buf_Ctrl_0             reg_Rdata_Buf_Ctrl_0;
    Reg_Diu_Reserve_Ctrl             reg_Diu_Reserve_Ctrl;
    Reg_Bium_Hdr_Num                 reg_Bium_Hdr_Num;
    Reg_Mxu_Reserved                 reg_Mxu_Reserved[2015];
    Reg_Mxu_Resvered_For_Sila        reg_Mxu_Resvered_For_Sila[6144];
} Mxu_regs;

#endif
