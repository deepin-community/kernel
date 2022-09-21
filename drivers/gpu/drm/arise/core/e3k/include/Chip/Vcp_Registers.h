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

#ifndef _VCP_REGISTERS_H
#define _VCP_REGISTERS_H

#ifndef        VCP_BLOCKBASE_INF
    #define    VCP_BLOCKBASE_INF
    #define    BLOCK_VCP_VERSION 1
    #define    BLOCK_VCP_TIMESTAMP "2019/1/16 11:01:18"
    #define    VCP_BLOCK                                                  0xC
    #define    VCP_REG_LIMIT                                              0x10
#endif


#define        Reg_Vcp_Mmio_Regs_Offset                                   0x0
#define        Reg_Fe_Ila_Counter_Video_Offset                            0x12
#define        Reg_Fe_Signature_Video_Offset                              0x16
#define        Reg_Fe_Framelvl_Memcount_Offset                            0x1A
#define        Reg_Fe_Framelvl_Perf_Offset                                0x1F
#define        Reg_Fe_Framelvl_Cmp_Memcount_Offset                        0x28
#define        Reg_Fe_Qeury_Reg_End_Offset                                0x2A
#define        Reg_Be_Ila_Counter_Video_Offset                            0x12
#define        Reg_Be_Signature_Video_Offset                              0x19
#define        Reg_Be_S3vd_Enc_Avg_Dist_Offset                            0x2D
#define        Reg_Be_Framelvl_Memcount_Offset                            0x2E
#define        Reg_Be_Framelvl_Perf_Offset                                0x33
#define        Reg_Be_Framelvl_Cmp_Memcount_Offset                        0x5B
#define        Reg_Be_Framelvl_Perf_New_Added_Offset                      0x5D
#define        Reg_Be_Qeury_Reg_End_Offset                                0x7C
#define        Reg_Vcp_Set_Regs_Offset                                    0x0







typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Risc_Act_Cnt_Fe;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Risc_Idle_Cnt_Fe;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Risc_First_Cmd_Fe;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Risc_Act_Cnt_Be;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Risc_Idle_Cnt_Be;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Risc_First_Cmd_Be;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Risc_Debug_Bus_Be;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Misc_Init;

typedef union
{
    struct
    {
        unsigned int     Wroutofrange          : 1;
        unsigned int     Dont_Kickoff_Be_Flag  : 1;

        unsigned int     Reserved              : 30;
    } reg;
    unsigned int uint;
} Reg_Mmio_S3vd_Status;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Me_Gpuw_Cpur_Cnt;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Iqtdbk_Cpuw_Gpur_Cnt;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Vp8enc_Mxu_En;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Vle_Debug_Bus_Be_0t13;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Jpg_Debug_Bus_Fe_0t1;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Reserved_Space_Frm43;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Dcp_Debug_Bus_0t5;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Reserved_Space_Frm56;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Dbg_Intr_Pc;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Vcp_Sig_Cnt_En;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Vld_Debug_Bus_Fe_8t13;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Jpg_Debug_Bus_Be_0t1;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Prd_Debug_Bus_18t26;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Avs2_Tu_Debug_Bus_0t1;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Avs2_Qtree_Debug_Bus;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Avs2_Cunit_Debug_Bus_0t1;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Enc_Debug_Bus_19t26;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Reserved_Space_Frm286;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Cal_Debug_Bus_0t2;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Vcp_Signature_Out;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Vcp_Debug_Bus_Stall;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Vcptop_Debug_Bus;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Enc_Debug_Bus_0t18;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Prd_Debug_Bus_0t17;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Risc_Debug_Bus_Fe_0t18;

typedef union
{
    struct
    {
        unsigned int     Errrecvoerflag        : 1;
        unsigned int     Errframeidx           : 31;
    } reg;
    unsigned int uint;
} Reg_Mmio_Err_Interrupt_Fe_Frame;

typedef union
{
    struct
    {
        unsigned int     Errblkstart           : 16;
        unsigned int     Errblknum             : 16;
    } reg;
    unsigned int uint;
} Reg_Mmio_Err_Interrupt_Fe_Blk_Addr;

typedef union
{
    struct
    {
        unsigned int     Errtag                : 8;
        unsigned int     Errlvl                : 3;
        unsigned int     Bfirsterrfe           : 1;
        unsigned int     Reservedfe            : 20;
    } reg;
    unsigned int uint;
} Reg_Mmio_Err_Interrupt_Fe_Info;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Err_Interrupt_Fe_Rsv;

typedef union
{
    struct
    {
        unsigned int     Reserved              : 1;
        unsigned int     Errframeidx           : 31;
    } reg;
    unsigned int uint;
} Reg_Mmio_Err_Interrupt_Be_Frame;

typedef union
{
    struct
    {
        unsigned int     Errblkstart           : 16;
        unsigned int     Reserved              : 16;
    } reg;
    unsigned int uint;
} Reg_Mmio_Err_Interrupt_Be_Blk_Addr;

typedef union
{
    struct
    {
        unsigned int     Errtag                : 8;
        unsigned int     Errlvl                : 3;
        unsigned int     Bfirsterrbe           : 1;
        unsigned int     Reservedbe            : 20;
    } reg;
    unsigned int uint;
} Reg_Mmio_Err_Interrupt_Be_Info;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Err_Interrupt_Be_Rsv;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Reserved_Space_Frm503;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Vld_Debug_Bus_Fe_0t7;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Mmio_Aes_Debug_Bus_0t1;

typedef struct _Group_Vcp_Mmio_Regs
{
    Reg_Mmio_Risc_Act_Cnt_Fe         reg_Mmio_Risc_Act_Cnt_Fe;
    Reg_Mmio_Risc_Idle_Cnt_Fe        reg_Mmio_Risc_Idle_Cnt_Fe;
    Reg_Mmio_Risc_First_Cmd_Fe       reg_Mmio_Risc_First_Cmd_Fe;
    Reg_Mmio_Risc_Act_Cnt_Be         reg_Mmio_Risc_Act_Cnt_Be;
    Reg_Mmio_Risc_Idle_Cnt_Be        reg_Mmio_Risc_Idle_Cnt_Be;
    Reg_Mmio_Risc_First_Cmd_Be       reg_Mmio_Risc_First_Cmd_Be;
    Reg_Mmio_Risc_Debug_Bus_Be       reg_Mmio_Risc_Debug_Bus_Be[18];
    Reg_Mmio_Misc_Init               reg_Mmio_Misc_Init;
    Reg_Mmio_S3vd_Status             reg_Mmio_S3vd_Status;
    Reg_Mmio_Me_Gpuw_Cpur_Cnt        reg_Mmio_Me_Gpuw_Cpur_Cnt;
    Reg_Mmio_Iqtdbk_Cpuw_Gpur_Cnt    reg_Mmio_Iqtdbk_Cpuw_Gpur_Cnt;
    Reg_Mmio_Vp8enc_Mxu_En           reg_Mmio_Vp8enc_Mxu_En;
    Reg_Mmio_Vle_Debug_Bus_Be_0t13   reg_Mmio_Vle_Debug_Bus_Be_0t13[11];
    Reg_Mmio_Jpg_Debug_Bus_Fe_0t1    reg_Mmio_Jpg_Debug_Bus_Fe_0t1[2];
    Reg_Mmio_Reserved_Space_Frm43    reg_Mmio_Reserved_Space_Frm43[8];
    Reg_Mmio_Dcp_Debug_Bus_0t5       reg_Mmio_Dcp_Debug_Bus_0t5[6];
    Reg_Mmio_Reserved_Space_Frm56    reg_Mmio_Reserved_Space_Frm56[200];
    Reg_Mmio_Dbg_Intr_Pc             reg_Mmio_Dbg_Intr_Pc;
    Reg_Mmio_Vcp_Sig_Cnt_En          reg_Mmio_Vcp_Sig_Cnt_En;
    Reg_Mmio_Vld_Debug_Bus_Fe_8t13   reg_Mmio_Vld_Debug_Bus_Fe_8t13[5];
    Reg_Mmio_Jpg_Debug_Bus_Be_0t1    reg_Mmio_Jpg_Debug_Bus_Be_0t1[2];
    Reg_Mmio_Prd_Debug_Bus_18t26     reg_Mmio_Prd_Debug_Bus_18t26[9];
    Reg_Mmio_Avs2_Tu_Debug_Bus_0t1   reg_Mmio_Avs2_Tu_Debug_Bus_0t1[2];
    Reg_Mmio_Avs2_Qtree_Debug_Bus    reg_Mmio_Avs2_Qtree_Debug_Bus;
    Reg_Mmio_Avs2_Cunit_Debug_Bus_0t1
                                     reg_Mmio_Avs2_Cunit_Debug_Bus_0t1[2];
    Reg_Mmio_Enc_Debug_Bus_19t26     reg_Mmio_Enc_Debug_Bus_19t26[7];
    Reg_Mmio_Reserved_Space_Frm286   reg_Mmio_Reserved_Space_Frm286[150];
    Reg_Mmio_Cal_Debug_Bus_0t2       reg_Mmio_Cal_Debug_Bus_0t2[3];
    Reg_Mmio_Vcp_Signature_Out       reg_Mmio_Vcp_Signature_Out[2];
    Reg_Mmio_Vcp_Debug_Bus_Stall     reg_Mmio_Vcp_Debug_Bus_Stall;
    Reg_Mmio_Vcptop_Debug_Bus        reg_Mmio_Vcptop_Debug_Bus;
    Reg_Mmio_Enc_Debug_Bus_0t18      reg_Mmio_Enc_Debug_Bus_0t18[17];
    Reg_Mmio_Prd_Debug_Bus_0t17      reg_Mmio_Prd_Debug_Bus_0t17[17];
    Reg_Mmio_Risc_Debug_Bus_Fe_0t18  reg_Mmio_Risc_Debug_Bus_Fe_0t18[18];
    Reg_Mmio_Err_Interrupt_Fe_Frame  reg_Mmio_Err_Interrupt_Fe_Frame;
    Reg_Mmio_Err_Interrupt_Fe_Blk_Addr
                                     reg_Mmio_Err_Interrupt_Fe_Blk_Addr;
    Reg_Mmio_Err_Interrupt_Fe_Info   reg_Mmio_Err_Interrupt_Fe_Info;
    Reg_Mmio_Err_Interrupt_Fe_Rsv    reg_Mmio_Err_Interrupt_Fe_Rsv;
    Reg_Mmio_Err_Interrupt_Be_Frame  reg_Mmio_Err_Interrupt_Be_Frame;
    Reg_Mmio_Err_Interrupt_Be_Blk_Addr
                                     reg_Mmio_Err_Interrupt_Be_Blk_Addr;
    Reg_Mmio_Err_Interrupt_Be_Info   reg_Mmio_Err_Interrupt_Be_Info;
    Reg_Mmio_Err_Interrupt_Be_Rsv    reg_Mmio_Err_Interrupt_Be_Rsv;
    Reg_Mmio_Reserved_Space_Frm503   reg_Mmio_Reserved_Space_Frm503;
    Reg_Mmio_Vld_Debug_Bus_Fe_0t7    reg_Mmio_Vld_Debug_Bus_Fe_0t7[6];
    Reg_Mmio_Aes_Debug_Bus_0t1       reg_Mmio_Aes_Debug_Bus_0t1[2];
} Reg_Vcp_Mmio_Regs_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Vcp_Fe_Total_Busy_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Risc_Fe_Total_Stall_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Fe_Write_Backpress_Bymemory_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Vcp_Fe_Total_Backpress_Cycle;

typedef struct _Group_Fe_Ila_Counter_Video
{
    Reg_S3vd_Vcp_Fe_Total_Busy_Cycle reg_S3vd_Vcp_Fe_Total_Busy_Cycle;
    Reg_S3vd_Risc_Fe_Total_Stall_Cycle
                                     reg_S3vd_Risc_Fe_Total_Stall_Cycle;
    Reg_S3vd_Fe_Write_Backpress_Bymemory_Cycle
                                     reg_S3vd_Fe_Write_Backpress_Bymemory_Cycle;
    Reg_S3vd_Vcp_Fe_Total_Backpress_Cycle
                                     reg_S3vd_Vcp_Fe_Total_Backpress_Cycle;
} Reg_Fe_Ila_Counter_Video_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Aesin_Signature_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Aesin_Signature_High;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Fe_Dma_Write_Signature_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Fe_Dma_Write_Signature_High;

typedef struct _Group_Fe_Signature_Video
{
    Reg_S3vd_Bci_Aesin_Signature_Low reg_S3vd_Bci_Aesin_Signature_Low;
    Reg_S3vd_Bci_Aesin_Signature_High
                                     reg_S3vd_Bci_Aesin_Signature_High;
    Reg_S3vd_Bci_Fe_Dma_Write_Signature_Low
                                     reg_S3vd_Bci_Fe_Dma_Write_Signature_Low;
    Reg_S3vd_Bci_Fe_Dma_Write_Signature_High
                                     reg_S3vd_Bci_Fe_Dma_Write_Signature_High;
} Reg_Fe_Signature_Video_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Fe_Memory_Read_Latency_Count_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Fe_Memory_Read_Latency_Count_High;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Fe_Total_Memory_Read_Count;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Fe_Total_Memory_Write_Count;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Fe_Dma_Write_Out_Count;

typedef struct _Group_Fe_Framelvl_Memcount
{
    Reg_S3vd_Fe_Memory_Read_Latency_Count_Low
                                     reg_S3vd_Fe_Memory_Read_Latency_Count_Low;
    Reg_S3vd_Fe_Memory_Read_Latency_Count_High
                                     reg_S3vd_Fe_Memory_Read_Latency_Count_High;
    Reg_S3vd_Fe_Total_Memory_Read_Count
                                     reg_S3vd_Fe_Total_Memory_Read_Count;
    Reg_S3vd_Fe_Total_Memory_Write_Count
                                     reg_S3vd_Fe_Total_Memory_Write_Count;
    Reg_S3vd_Fe_Dma_Write_Out_Count  reg_S3vd_Fe_Dma_Write_Out_Count;
} Reg_Fe_Framelvl_Memcount_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Risc_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Vld_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Vld_Total_Useful_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Vld_Total_Backpressure_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Vld_Total_Waitmemory_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Nonvldrisc_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Nonvldrisc_Total_Useful_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Nonvldrisc_Total_Backpressure_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Nonvldrisc_Total_Waitmemory_Cycle;

typedef struct _Group_Fe_Framelvl_Perf
{
    Reg_S3vd_Risc_Total_Cycle        reg_S3vd_Risc_Total_Cycle;
    Reg_S3vd_Vld_Total_Cycle         reg_S3vd_Vld_Total_Cycle;
    Reg_S3vd_Vld_Total_Useful_Cycle  reg_S3vd_Vld_Total_Useful_Cycle;
    Reg_S3vd_Vld_Total_Backpressure_Cycle
                                     reg_S3vd_Vld_Total_Backpressure_Cycle;
    Reg_S3vd_Vld_Total_Waitmemory_Cycle
                                     reg_S3vd_Vld_Total_Waitmemory_Cycle;
    Reg_S3vd_Nonvldrisc_Total_Cycle  reg_S3vd_Nonvldrisc_Total_Cycle;
    Reg_S3vd_Nonvldrisc_Total_Useful_Cycle
                                     reg_S3vd_Nonvldrisc_Total_Useful_Cycle;
    Reg_S3vd_Nonvldrisc_Total_Backpressure_Cycle
                                     reg_S3vd_Nonvldrisc_Total_Backpressure_Cycle;
    Reg_S3vd_Nonvldrisc_Total_Waitmemory_Cycle
                                     reg_S3vd_Nonvldrisc_Total_Waitmemory_Cycle;
} Reg_Fe_Framelvl_Perf_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Fe_Memory_Read_Cmp_Count;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Fe_Memory_Write_Cmp_Count;

typedef struct _Group_Fe_Framelvl_Cmp_Memcount
{
    Reg_S3vd_Fe_Memory_Read_Cmp_Count
                                     reg_S3vd_Fe_Memory_Read_Cmp_Count;
    Reg_S3vd_Fe_Memory_Write_Cmp_Count
                                     reg_S3vd_Fe_Memory_Write_Cmp_Count;
} Reg_Fe_Framelvl_Cmp_Memcount_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Fe_Qeury_Reserved;

typedef struct _Group_Fe_Qeury_Reg_End
{
    Reg_Fe_Qeury_Reserved            reg_Fe_Qeury_Reserved;
} Reg_Fe_Qeury_Reg_End_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Vcp_Be_Total_Busy_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Rsic_Be_Total_Stall_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Write_Backpress_Bymemory_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Vcp_Be_Total_Backpress_Bytb_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Total_Busy_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Wait_Data_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Mxu_Back_Qtm_Cycle;

typedef struct _Group_Be_Ila_Counter_Video
{
    Reg_S3vd_Vcp_Be_Total_Busy_Cycle reg_S3vd_Vcp_Be_Total_Busy_Cycle;
    Reg_S3vd_Rsic_Be_Total_Stall_Cycle
                                     reg_S3vd_Rsic_Be_Total_Stall_Cycle;
    Reg_S3vd_Be_Write_Backpress_Bymemory_Cycle
                                     reg_S3vd_Be_Write_Backpress_Bymemory_Cycle;
    Reg_S3vd_Vcp_Be_Total_Backpress_Bytb_Cycle
                                     reg_S3vd_Vcp_Be_Total_Backpress_Bytb_Cycle;
    Reg_S3vd_Qtm_Total_Busy_Cycle    reg_S3vd_Qtm_Total_Busy_Cycle;
    Reg_S3vd_Qtm_Wait_Data_Cycle     reg_S3vd_Qtm_Wait_Data_Cycle;
    Reg_S3vd_Mxu_Back_Qtm_Cycle      reg_S3vd_Mxu_Back_Qtm_Cycle;
} Reg_Be_Ila_Counter_Video_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Dec_Signature_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Dec_Signature_High;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Enc_Signature_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Enc_Signature_High;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Cmg0_Signature_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Cmg0_Signature_High;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Cmg1_Signature_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Cmg1_Signature_High;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Tbich0_Signature_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Tbich0_Signature_High;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Tbich1_Signature_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Tbich1_Signature_High;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Sadin_Signature_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Sadin_Signature_High;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Mcfin_Signature_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Mcfin_Signature_High;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Srcimg_Signature_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Srcimg_Signature_High;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Be_Dma_Read_Signature_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Bci_Be_Dma_Read_Signature_High;

typedef struct _Group_Be_Signature_Video
{
    Reg_S3vd_Bci_Dec_Signature_Low   reg_S3vd_Bci_Dec_Signature_Low;
    Reg_S3vd_Bci_Dec_Signature_High  reg_S3vd_Bci_Dec_Signature_High;
    Reg_S3vd_Bci_Enc_Signature_Low   reg_S3vd_Bci_Enc_Signature_Low;
    Reg_S3vd_Bci_Enc_Signature_High  reg_S3vd_Bci_Enc_Signature_High;
    Reg_S3vd_Bci_Cmg0_Signature_Low  reg_S3vd_Bci_Cmg0_Signature_Low;
    Reg_S3vd_Bci_Cmg0_Signature_High reg_S3vd_Bci_Cmg0_Signature_High;
    Reg_S3vd_Bci_Cmg1_Signature_Low  reg_S3vd_Bci_Cmg1_Signature_Low;
    Reg_S3vd_Bci_Cmg1_Signature_High reg_S3vd_Bci_Cmg1_Signature_High;
    Reg_S3vd_Bci_Tbich0_Signature_Low
                                     reg_S3vd_Bci_Tbich0_Signature_Low;
    Reg_S3vd_Bci_Tbich0_Signature_High
                                     reg_S3vd_Bci_Tbich0_Signature_High;
    Reg_S3vd_Bci_Tbich1_Signature_Low
                                     reg_S3vd_Bci_Tbich1_Signature_Low;
    Reg_S3vd_Bci_Tbich1_Signature_High
                                     reg_S3vd_Bci_Tbich1_Signature_High;
    Reg_S3vd_Bci_Sadin_Signature_Low reg_S3vd_Bci_Sadin_Signature_Low;
    Reg_S3vd_Bci_Sadin_Signature_High
                                     reg_S3vd_Bci_Sadin_Signature_High;
    Reg_S3vd_Bci_Mcfin_Signature_Low reg_S3vd_Bci_Mcfin_Signature_Low;
    Reg_S3vd_Bci_Mcfin_Signature_High
                                     reg_S3vd_Bci_Mcfin_Signature_High;
    Reg_S3vd_Bci_Srcimg_Signature_Low
                                     reg_S3vd_Bci_Srcimg_Signature_Low;
    Reg_S3vd_Bci_Srcimg_Signature_High
                                     reg_S3vd_Bci_Srcimg_Signature_High;
    Reg_S3vd_Bci_Be_Dma_Read_Signature_Low
                                     reg_S3vd_Bci_Be_Dma_Read_Signature_Low;
    Reg_S3vd_Bci_Be_Dma_Read_Signature_High
                                     reg_S3vd_Bci_Be_Dma_Read_Signature_High;
} Reg_Be_Signature_Video_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Enc_Avg_Dist;

typedef struct _Group_Be_S3vd_Enc_Avg_Dist
{
    Reg_S3vd_Enc_Avg_Dist            reg_S3vd_Enc_Avg_Dist;
} Reg_Be_S3vd_Enc_Avg_Dist_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Memory_Latency_Count_Low;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Memory_Latency_Count_High;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Total_Memory_Read_Count;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Total_Memory_Write_Count;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Dma_Read_Count;

typedef struct _Group_Be_Framelvl_Memcount
{
    Reg_S3vd_Be_Memory_Latency_Count_Low
                                     reg_S3vd_Be_Memory_Latency_Count_Low;
    Reg_S3vd_Be_Memory_Latency_Count_High
                                     reg_S3vd_Be_Memory_Latency_Count_High;
    Reg_S3vd_Be_Total_Memory_Read_Count
                                     reg_S3vd_Be_Total_Memory_Read_Count;
    Reg_S3vd_Be_Total_Memory_Write_Count
                                     reg_S3vd_Be_Total_Memory_Write_Count;
    Reg_S3vd_Be_Dma_Read_Count       reg_S3vd_Be_Dma_Read_Count;
} Reg_Be_Framelvl_Memcount_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Mc_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Mc_Total_Useful_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Mc_Total_Backpressure_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Mc_Total_Waitmemory_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Mc_Total_Waitcmd_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Mcidct_Total_Backpressure_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Mcidct_Total_Waitcmd_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Idct_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Idct_Total_Useful_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Idct_Total_Backpressure_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Idct_Total_Waitmemory_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Idct_Total_Waitcmd_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Dbk_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Dbk_Total_Useful_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Dbk_Total_Backpressure_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Dbk_Total_Waitmemory_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Dbk_Total_Waitcmd_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Dbk_Total_Waitidctmcdata_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Dbk_Total_Waitidctdata_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Qtm_Dbk_Total_Waitmcdata_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Cmg_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Cmg_Total_Useful_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Cmg_Total_Backpressure_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Cmg_Total_Waitcmd_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Cmg_Total_Ch0waitch1_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Cmg_Total_Waitbufferfull_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Prd_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Prd_Total_Useful_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Prd_Vcp_Ch0_Total_Backpressure_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Prd_Vcp_Ch1_Total_Backpressure_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Prd_Total_Waitmemory_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Prd_Total_Waitcmd_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Risc_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Risc_Vle_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Risc_Vle_Useful_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Risc_Blkbymem_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Risc_Vle_Waitformee_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Risc_Nonvle_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Risc_Nonvle_Useful_Cycle;

typedef struct _Group_Be_Framelvl_Perf
{
    Reg_S3vd_Qtm_Mc_Total_Cycle      reg_S3vd_Qtm_Mc_Total_Cycle;
    Reg_S3vd_Qtm_Mc_Total_Useful_Cycle
                                     reg_S3vd_Qtm_Mc_Total_Useful_Cycle;
    Reg_S3vd_Qtm_Mc_Total_Backpressure_Cycle
                                     reg_S3vd_Qtm_Mc_Total_Backpressure_Cycle;
    Reg_S3vd_Qtm_Mc_Total_Waitmemory_Cycle
                                     reg_S3vd_Qtm_Mc_Total_Waitmemory_Cycle;
    Reg_S3vd_Qtm_Mc_Total_Waitcmd_Cycle
                                     reg_S3vd_Qtm_Mc_Total_Waitcmd_Cycle;
    Reg_S3vd_Qtm_Total_Cycle         reg_S3vd_Qtm_Total_Cycle;
    Reg_S3vd_Qtm_Mcidct_Total_Backpressure_Cycle
                                     reg_S3vd_Qtm_Mcidct_Total_Backpressure_Cycle;
    Reg_S3vd_Qtm_Mcidct_Total_Waitcmd_Cycle
                                     reg_S3vd_Qtm_Mcidct_Total_Waitcmd_Cycle;
    Reg_S3vd_Qtm_Idct_Total_Cycle    reg_S3vd_Qtm_Idct_Total_Cycle;
    Reg_S3vd_Qtm_Idct_Total_Useful_Cycle
                                     reg_S3vd_Qtm_Idct_Total_Useful_Cycle;
    Reg_S3vd_Qtm_Idct_Total_Backpressure_Cycle
                                     reg_S3vd_Qtm_Idct_Total_Backpressure_Cycle;
    Reg_S3vd_Qtm_Idct_Total_Waitmemory_Cycle
                                     reg_S3vd_Qtm_Idct_Total_Waitmemory_Cycle;
    Reg_S3vd_Qtm_Idct_Total_Waitcmd_Cycle
                                     reg_S3vd_Qtm_Idct_Total_Waitcmd_Cycle;
    Reg_S3vd_Qtm_Dbk_Total_Cycle     reg_S3vd_Qtm_Dbk_Total_Cycle;
    Reg_S3vd_Qtm_Dbk_Total_Useful_Cycle
                                     reg_S3vd_Qtm_Dbk_Total_Useful_Cycle;
    Reg_S3vd_Qtm_Dbk_Total_Backpressure_Cycle
                                     reg_S3vd_Qtm_Dbk_Total_Backpressure_Cycle;
    Reg_S3vd_Qtm_Dbk_Total_Waitmemory_Cycle
                                     reg_S3vd_Qtm_Dbk_Total_Waitmemory_Cycle;
    Reg_S3vd_Qtm_Dbk_Total_Waitcmd_Cycle
                                     reg_S3vd_Qtm_Dbk_Total_Waitcmd_Cycle;
    Reg_S3vd_Qtm_Dbk_Total_Waitidctmcdata_Cycle
                                     reg_S3vd_Qtm_Dbk_Total_Waitidctmcdata_Cycle;
    Reg_S3vd_Qtm_Dbk_Total_Waitidctdata_Cycle
                                     reg_S3vd_Qtm_Dbk_Total_Waitidctdata_Cycle;
    Reg_S3vd_Qtm_Dbk_Total_Waitmcdata_Cycle
                                     reg_S3vd_Qtm_Dbk_Total_Waitmcdata_Cycle;
    Reg_S3vd_Cmg_Total_Cycle         reg_S3vd_Cmg_Total_Cycle;
    Reg_S3vd_Cmg_Total_Useful_Cycle  reg_S3vd_Cmg_Total_Useful_Cycle;
    Reg_S3vd_Cmg_Total_Backpressure_Cycle
                                     reg_S3vd_Cmg_Total_Backpressure_Cycle;
    Reg_S3vd_Cmg_Total_Waitcmd_Cycle reg_S3vd_Cmg_Total_Waitcmd_Cycle;
    Reg_S3vd_Cmg_Total_Ch0waitch1_Cycle
                                     reg_S3vd_Cmg_Total_Ch0waitch1_Cycle;
    Reg_S3vd_Cmg_Total_Waitbufferfull_Cycle
                                     reg_S3vd_Cmg_Total_Waitbufferfull_Cycle;
    Reg_S3vd_Prd_Total_Cycle         reg_S3vd_Prd_Total_Cycle;
    Reg_S3vd_Prd_Total_Useful_Cycle  reg_S3vd_Prd_Total_Useful_Cycle;
    Reg_S3vd_Prd_Vcp_Ch0_Total_Backpressure_Cycle
                                     reg_S3vd_Prd_Vcp_Ch0_Total_Backpressure_Cycle;
    Reg_S3vd_Prd_Vcp_Ch1_Total_Backpressure_Cycle
                                     reg_S3vd_Prd_Vcp_Ch1_Total_Backpressure_Cycle;
    Reg_S3vd_Prd_Total_Waitmemory_Cycle
                                     reg_S3vd_Prd_Total_Waitmemory_Cycle;
    Reg_S3vd_Prd_Total_Waitcmd_Cycle reg_S3vd_Prd_Total_Waitcmd_Cycle;
    Reg_S3vd_Be_Risc_Total_Cycle     reg_S3vd_Be_Risc_Total_Cycle;
    Reg_S3vd_Be_Risc_Vle_Total_Cycle reg_S3vd_Be_Risc_Vle_Total_Cycle;
    Reg_S3vd_Be_Risc_Vle_Useful_Cycle
                                     reg_S3vd_Be_Risc_Vle_Useful_Cycle;
    Reg_S3vd_Be_Risc_Blkbymem_Cycle  reg_S3vd_Be_Risc_Blkbymem_Cycle;
    Reg_S3vd_Be_Risc_Vle_Waitformee_Cycle
                                     reg_S3vd_Be_Risc_Vle_Waitformee_Cycle;
    Reg_S3vd_Be_Risc_Nonvle_Total_Cycle
                                     reg_S3vd_Be_Risc_Nonvle_Total_Cycle;
    Reg_S3vd_Be_Risc_Nonvle_Useful_Cycle
                                     reg_S3vd_Be_Risc_Nonvle_Useful_Cycle;
} Reg_Be_Framelvl_Perf_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Memory_Read_Cmp_Count;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Memory_Write_Cmp_Count;

typedef struct _Group_Be_Framelvl_Cmp_Memcount
{
    Reg_S3vd_Be_Memory_Read_Cmp_Count
                                     reg_S3vd_Be_Memory_Read_Cmp_Count;
    Reg_S3vd_Be_Memory_Write_Cmp_Count
                                     reg_S3vd_Be_Memory_Write_Cmp_Count;
} Reg_Be_Framelvl_Cmp_Memcount_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Useful_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Idle_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Backpressbycmg_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Backpressbysadin_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Backpressbymcfin_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Intrapredsearch_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Inp_Idle_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Inp_Waitfordata_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Getsearchcenter_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Skiptest_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Coarsesearch_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Finesearch_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Interintrasel_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Frcsearch_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Updatemv_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Encoder_Mc_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Encoder_Mc_Waitfordata_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Encoder_Ftqiqt_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Encoder_Ftqiqt_Waitfordata_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Encoder_Backpressbyvlesram_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Qtm_Sad_Total_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S4vd_Be_Qtm_Sad_Useful_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S5vd_Be_Qtm_Sad_Backpress_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S6vd_Be_Qtm_Sad_Waitformem_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S7vd_Be_Qtm_Sad_Waitcmd_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S8vd_Be_Qtm_Sao_Compensation_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S9vd_Be_Qtm_Sdh_Delta_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Cusplit_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Getmergemv_Cycle;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_S3vd_Be_Mee_Dealwithresi_Cycle;

typedef struct _Group_Be_Framelvl_Perf_New_Added
{
    Reg_S3vd_Be_Mee_Total_Cycle      reg_S3vd_Be_Mee_Total_Cycle;
    Reg_S3vd_Be_Mee_Useful_Cycle     reg_S3vd_Be_Mee_Useful_Cycle;
    Reg_S3vd_Be_Mee_Idle_Cycle       reg_S3vd_Be_Mee_Idle_Cycle;
    Reg_S3vd_Be_Mee_Backpressbycmg_Cycle
                                     reg_S3vd_Be_Mee_Backpressbycmg_Cycle;
    Reg_S3vd_Be_Mee_Backpressbysadin_Cycle
                                     reg_S3vd_Be_Mee_Backpressbysadin_Cycle;
    Reg_S3vd_Be_Mee_Backpressbymcfin_Cycle
                                     reg_S3vd_Be_Mee_Backpressbymcfin_Cycle;
    Reg_S3vd_Be_Mee_Intrapredsearch_Cycle
                                     reg_S3vd_Be_Mee_Intrapredsearch_Cycle;
    Reg_S3vd_Be_Mee_Inp_Idle_Cycle   reg_S3vd_Be_Mee_Inp_Idle_Cycle;
    Reg_S3vd_Be_Mee_Inp_Waitfordata_Cycle
                                     reg_S3vd_Be_Mee_Inp_Waitfordata_Cycle;
    Reg_S3vd_Be_Mee_Getsearchcenter_Cycle
                                     reg_S3vd_Be_Mee_Getsearchcenter_Cycle;
    Reg_S3vd_Be_Mee_Skiptest_Cycle   reg_S3vd_Be_Mee_Skiptest_Cycle;
    Reg_S3vd_Be_Mee_Coarsesearch_Cycle
                                     reg_S3vd_Be_Mee_Coarsesearch_Cycle;
    Reg_S3vd_Be_Mee_Finesearch_Cycle reg_S3vd_Be_Mee_Finesearch_Cycle;
    Reg_S3vd_Be_Mee_Interintrasel_Cycle
                                     reg_S3vd_Be_Mee_Interintrasel_Cycle;
    Reg_S3vd_Be_Mee_Frcsearch_Cycle  reg_S3vd_Be_Mee_Frcsearch_Cycle;
    Reg_S3vd_Be_Mee_Updatemv_Cycle   reg_S3vd_Be_Mee_Updatemv_Cycle;
    Reg_S3vd_Be_Encoder_Mc_Total_Cycle
                                     reg_S3vd_Be_Encoder_Mc_Total_Cycle;
    Reg_S3vd_Be_Encoder_Mc_Waitfordata_Cycle
                                     reg_S3vd_Be_Encoder_Mc_Waitfordata_Cycle;
    Reg_S3vd_Be_Encoder_Ftqiqt_Total_Cycle
                                     reg_S3vd_Be_Encoder_Ftqiqt_Total_Cycle;
    Reg_S3vd_Be_Encoder_Ftqiqt_Waitfordata_Cycle
                                     reg_S3vd_Be_Encoder_Ftqiqt_Waitfordata_Cycle;
    Reg_S3vd_Be_Encoder_Backpressbyvlesram_Cycle
                                     reg_S3vd_Be_Encoder_Backpressbyvlesram_Cycle;
    Reg_S3vd_Be_Qtm_Sad_Total_Cycle  reg_S3vd_Be_Qtm_Sad_Total_Cycle;
    Reg_S4vd_Be_Qtm_Sad_Useful_Cycle reg_S4vd_Be_Qtm_Sad_Useful_Cycle;
    Reg_S5vd_Be_Qtm_Sad_Backpress_Cycle
                                     reg_S5vd_Be_Qtm_Sad_Backpress_Cycle;
    Reg_S6vd_Be_Qtm_Sad_Waitformem_Cycle
                                     reg_S6vd_Be_Qtm_Sad_Waitformem_Cycle;
    Reg_S7vd_Be_Qtm_Sad_Waitcmd_Cycle
                                     reg_S7vd_Be_Qtm_Sad_Waitcmd_Cycle;
    Reg_S8vd_Be_Qtm_Sao_Compensation_Cycle
                                     reg_S8vd_Be_Qtm_Sao_Compensation_Cycle;
    Reg_S9vd_Be_Qtm_Sdh_Delta_Cycle  reg_S9vd_Be_Qtm_Sdh_Delta_Cycle;
    Reg_S3vd_Be_Mee_Cusplit_Cycle    reg_S3vd_Be_Mee_Cusplit_Cycle;
    Reg_S3vd_Be_Mee_Getmergemv_Cycle reg_S3vd_Be_Mee_Getmergemv_Cycle;
    Reg_S3vd_Be_Mee_Dealwithresi_Cycle
                                     reg_S3vd_Be_Mee_Dealwithresi_Cycle;
} Reg_Be_Framelvl_Perf_New_Added_Group;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Be_Qeury_Reserved;

typedef struct _Group_Be_Qeury_Reg_End
{
    Reg_Be_Qeury_Reserved            reg_Be_Qeury_Reserved;
} Reg_Be_Qeury_Reg_End_Group;

typedef union
{
    struct
    {
        unsigned int     Hang_Wait_Cnt         : 24;
        unsigned int     Error_Detect_Enable   : 1;
        unsigned int     Power_Gating_Enable   : 1;
        unsigned int     Reserved              : 2;
        unsigned int     Swhalt_Enable         : 1;
        unsigned int     Hang_Notran_Reset_En  : 1;
        unsigned int     Sig_Clear_Disable     : 1;
        unsigned int     Hang_Auto_Reset_En    : 1;
    } reg;
    unsigned int uint;
} Reg_Set_Reg_0;

typedef union
{
    struct
    {
        unsigned int     Sig_Sel               : 4;
        unsigned int     Sig_Mode              : 1;
        unsigned int     Sig_Lock_Dec          : 1;
        unsigned int     Sig_Lock_Enc          : 1;
        unsigned int     Mem_Check_Enable      : 1;
        unsigned int     Sig_Lock_Cnt          : 24;
    } reg;
    unsigned int uint;
} Reg_Set_Reg_1;

typedef union
{
    struct
    {
        unsigned int     Dec_Sig_Fail_Bk       : 1;
        unsigned int     Enc_Sig_Fail_Bk       : 1;
        unsigned int     Mmio_Nout             : 1;
        unsigned int     Mmio_Qlp              : 1;
        unsigned int     Tile_Swizzle_En       : 1;
        unsigned int     Vcp_Clear_En          : 1;
        unsigned int     Vcp_Int_En            : 1;
        unsigned int     Tb_Int_En             : 1;
        unsigned int     Qtm_Int_En            : 1;
        unsigned int     Hang_Rst_Int_En       : 1;
        unsigned int     Sw_Rst_Int_En         : 1;
        unsigned int     Sw_Rst_Idle_Cnt_En    : 1;
        unsigned int     S3vd_Enc_Clk_Disable  : 1;
        unsigned int     Vcp_Always_Busy       : 1;
        unsigned int     Vp9_Partitionprob_Select
                                               : 1;
        unsigned int     Reserved1             : 17;
    } reg;
    unsigned int uint;
} Reg_Set_Reg_2;

typedef union
{
    struct
    {
        unsigned int     Codec_Type            : 8;
        unsigned int     Reserved              : 24;
    } reg;
    unsigned int uint;
} Reg_Set_Reg_3;

typedef union
{
    struct
    {
        unsigned int     Baseaddr_L            : 32;
    } reg;
    unsigned int uint;
} Reg_Set_Reg_4;

typedef union
{
    struct
    {
        unsigned int     Baseaddr_H            : 8;
        unsigned int     Bl_Slot_Idx           : 18;
        unsigned int     Reserved1             : 6;
    } reg;
    unsigned int uint;
} Reg_Set_Reg_5;

typedef union
{
    struct
    {
        unsigned int     Rangesize             : 32;
    } reg;
    unsigned int uint;
} Reg_Set_Reg_6;

typedef union
{
    struct
    {
        unsigned int     Bnewframestart        : 1;
        unsigned int     Udrawid_L             : 2;
        unsigned int     Reserved0             : 4;
        unsigned int     Udrawid_H             : 4;
        unsigned int     Reserved1             : 21;
    } reg;
    unsigned int uint;
} Reg_Set_Reg_7;

typedef union
{
    struct
    {
        unsigned int     Error_Level           : 4;
        unsigned int     Syntax_Max_Value      : 28;
    } reg;
    unsigned int uint;
} Reg_Syntax_Max_Value;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Constant_Key;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Wrap_Ctr;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Content_Key;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Content_Ctr;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Frame_Distortion;

typedef union
{
    struct
    {
        unsigned int     Count_Mb_Perf_Flag    : 1;
        unsigned int     Reserved              : 5;
        unsigned int     Vld_Mb_Perf_Base_L    : 26;
    } reg;
    unsigned int uint;
} Reg_Set_Mb_Perf_Base_Addr_L;

typedef union
{
    struct
    {
        unsigned int     Vld_Mb_Perf_Base_H    : 8;
        unsigned int     Bl_Slot_Idx           : 18;
        unsigned int     Reserved1             : 6;
    } reg;
    unsigned int uint;
} Reg_Set_Mb_Perf_Base_Addr_H;

typedef union
{
    struct
    {
        unsigned int     Count_Mb_Perf_Flag    : 1;
        unsigned int     Reserved              : 31;
    } reg;
    unsigned int uint;
} Reg_Set_Mb_Perf_Control_Flag;

typedef union
{
    struct
    {
        unsigned int     Count_Mb_Perf_Flag    : 1;
        unsigned int     Reserved              : 5;
        unsigned int     Vle_Mb_Perf_Base_L    : 26;
    } reg;
    unsigned int uint;
} Reg_Set_Vle_Mb_Perf_Base_Addr_L;

typedef union
{
    struct
    {
        unsigned int     Vle_Mb_Perf_Base_H    : 8;
        unsigned int     Bl_Slot_Idx           : 18;
        unsigned int     Reserved1             : 6;
    } reg;
    unsigned int uint;
} Reg_Set_Vle_Mb_Perf_Base_Addr_H;

typedef union
{
    struct
    {
        unsigned int     Count_Mb_Perf_Flag    : 1;
        unsigned int     Reserved              : 5;
        unsigned int     Encoder_Mb_Perf_Base_L
                                               : 26;
    } reg;
    unsigned int uint;
} Reg_Set_Encoder_Mb_Perf_Base_Addr_L;

typedef union
{
    struct
    {
        unsigned int     Encoder_Mb_Perf_Base_H
                                               : 8;

        unsigned int     Bl_Slot_Idx           : 18;

        unsigned int     Reserved1             : 6;
    } reg;
    unsigned int uint;
} Reg_Set_Encoder_Mb_Perf_Base_Addr_H;

typedef union
{
    struct
    {
        unsigned int     Count_Mb_Perf_Flag    : 1;
        unsigned int     Reserved              : 5;
        unsigned int     Ftq_Mb_Perf_Base_L    : 26;
    } reg;
    unsigned int uint;
} Reg_Set_Ftq_Mb_Perf_Base_Addr_L;

typedef union
{
    struct
    {
        unsigned int     Ftq_Mb_Perf_Base_H    : 8;

        unsigned int     Bl_Slot_Idx           : 18;

        unsigned int     Reserved1             : 6;
    } reg;
    unsigned int uint;
} Reg_Set_Ftq_Mb_Perf_Base_Addr_H;

typedef union
{
    struct
    {
        unsigned int     Ftq_Mb_Perf_Slice_Offset
                                               : 32;

    } reg;
    unsigned int uint;
} Reg_Set_Ftq_Mb_Perf_Slice_Offset;

typedef union
{
    struct
    {
        unsigned int     Enable_Bypass_1cycperbit
                                               : 1;

        unsigned int     Enabe_Nonzerolvl_Puttb2cyc
                                               : 1;

        unsigned int     Enable_Residual_Coding_Change
                                               : 1;

        unsigned int     Enable_Cunit_Intra_Change
                                               : 1;

        unsigned int     Enable_Sao_Change     : 1;

        unsigned int     Enable_Cunit_Change   : 1;

        unsigned int     Enable_Codingquadtree_Change
                                               : 1;

        unsigned int     Reserved              : 25;
    } reg;
    unsigned int uint;
} Reg_Set_Hevc_Perf_Count_Mode;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Set_Regreserv1;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Lambda_Table_Data_Intra;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Lambda_Table_Data_Inter;

typedef union
{
    struct
    {
        unsigned int     Insert_Dma_Error      : 1;
        unsigned int     Reserved              : 7;
        unsigned int     Error_Interval        : 24;
    } reg;
    unsigned int uint;
} Reg_Dma_Cmd_Fuzzy_Test;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Set_Regreserv2;

typedef union
{
    struct
    {
        unsigned int     Reserved              : 5;
        unsigned int     Base_Addr_L           : 27;
    } reg;
    unsigned int uint;
} Reg_Be_Bci_Mem_Base_Addr_L;

typedef union
{
    struct
    {
        unsigned int     Base_Addr_H           : 8;
        unsigned int     Bl_Slot_Idx           : 18;
        unsigned int     Reserved              : 6;
    } reg;
    unsigned int uint;
} Reg_Be_Bci_Mem_Base_Addr_H;

typedef union
{
    struct
    {
        unsigned int     uint;
    } reg;
    unsigned int uint;
} Reg_Be_Bci_Mem_Size;

typedef struct _Group_Vcp_Set_Regs
{
    Reg_Set_Reg_0                    reg_Set_Reg_0;
    Reg_Set_Reg_1                    reg_Set_Reg_1;
    Reg_Set_Reg_2                    reg_Set_Reg_2;
    Reg_Set_Reg_3                    reg_Set_Reg_3;
    Reg_Set_Reg_4                    reg_Set_Reg_4;
    Reg_Set_Reg_5                    reg_Set_Reg_5;
    Reg_Set_Reg_6                    reg_Set_Reg_6;
    Reg_Set_Reg_7                    reg_Set_Reg_7;
    Reg_Syntax_Max_Value             reg_Syntax_Max_Value[25];
    Reg_Constant_Key                 reg_Constant_Key[4];
    Reg_Wrap_Ctr                     reg_Wrap_Ctr[4];
    Reg_Content_Key                  reg_Content_Key[4];
    Reg_Content_Ctr                  reg_Content_Ctr[4];
    Reg_Frame_Distortion             reg_Frame_Distortion;
    Reg_Set_Mb_Perf_Base_Addr_L      reg_Set_Mb_Perf_Base_Addr_L;
    Reg_Set_Mb_Perf_Base_Addr_H      reg_Set_Mb_Perf_Base_Addr_H;
    Reg_Set_Mb_Perf_Control_Flag     reg_Set_Mb_Perf_Control_Flag;
    Reg_Set_Vle_Mb_Perf_Base_Addr_L  reg_Set_Vle_Mb_Perf_Base_Addr_L;
    Reg_Set_Vle_Mb_Perf_Base_Addr_H  reg_Set_Vle_Mb_Perf_Base_Addr_H;
    Reg_Set_Encoder_Mb_Perf_Base_Addr_L
                                     reg_Set_Encoder_Mb_Perf_Base_Addr_L;
    Reg_Set_Encoder_Mb_Perf_Base_Addr_H
                                     reg_Set_Encoder_Mb_Perf_Base_Addr_H;
    Reg_Set_Ftq_Mb_Perf_Base_Addr_L  reg_Set_Ftq_Mb_Perf_Base_Addr_L;
    Reg_Set_Ftq_Mb_Perf_Base_Addr_H  reg_Set_Ftq_Mb_Perf_Base_Addr_H;
    Reg_Set_Ftq_Mb_Perf_Slice_Offset reg_Set_Ftq_Mb_Perf_Slice_Offset;
    Reg_Set_Hevc_Perf_Count_Mode     reg_Set_Hevc_Perf_Count_Mode;
    Reg_Set_Regreserv1               reg_Set_Regreserv1[5];
    Reg_Lambda_Table_Data_Intra      reg_Lambda_Table_Data_Intra[13];
    Reg_Lambda_Table_Data_Inter      reg_Lambda_Table_Data_Inter[13];
    Reg_Dma_Cmd_Fuzzy_Test           reg_Dma_Cmd_Fuzzy_Test;
    Reg_Set_Regreserv2               reg_Set_Regreserv2[160];
    Reg_Be_Bci_Mem_Base_Addr_L       reg_Be_Bci_Mem_Base_Addr_L;
    Reg_Be_Bci_Mem_Base_Addr_H       reg_Be_Bci_Mem_Base_Addr_H;
    Reg_Be_Bci_Mem_Size              reg_Be_Bci_Mem_Size;
} Reg_Vcp_Set_Regs_Group;

typedef struct _Vcp_regs
{
    Reg_Vcp_Mmio_Regs_Group          reg_Vcp_Mmio_Regs;
    Reg_Fe_Ila_Counter_Video_Group   reg_Fe_Ila_Counter_Video;
    Reg_Fe_Signature_Video_Group     reg_Fe_Signature_Video;
    Reg_Fe_Framelvl_Memcount_Group   reg_Fe_Framelvl_Memcount;
    Reg_Fe_Framelvl_Perf_Group       reg_Fe_Framelvl_Perf;
    Reg_Fe_Framelvl_Cmp_Memcount_Group
                                     reg_Fe_Framelvl_Cmp_Memcount;
    Reg_Fe_Qeury_Reg_End_Group       reg_Fe_Qeury_Reg_End;
    Reg_Be_Ila_Counter_Video_Group   reg_Be_Ila_Counter_Video;
    Reg_Be_Signature_Video_Group     reg_Be_Signature_Video;
    Reg_Be_S3vd_Enc_Avg_Dist_Group   reg_Be_S3vd_Enc_Avg_Dist;
    Reg_Be_Framelvl_Memcount_Group   reg_Be_Framelvl_Memcount;
    Reg_Be_Framelvl_Perf_Group       reg_Be_Framelvl_Perf;
    Reg_Be_Framelvl_Cmp_Memcount_Group
                                     reg_Be_Framelvl_Cmp_Memcount;
    Reg_Be_Framelvl_Perf_New_Added_Group
                                     reg_Be_Framelvl_Perf_New_Added;
    Reg_Be_Qeury_Reg_End_Group       reg_Be_Qeury_Reg_End;
    Reg_Vcp_Set_Regs_Group           reg_Vcp_Set_Regs;
} Vcp_regs;

#endif
