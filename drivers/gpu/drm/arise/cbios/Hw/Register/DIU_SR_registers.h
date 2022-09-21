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


typedef union _REG_SR08    //Unlock_Extended_Sequencer_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Unlock_Extended_Sequencer    :8;
    };
}REG_SR08;


typedef union _REG_SR09    //Extended_Sequencer_9_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :3;
        CBIOS_U8    Virtual_DCLK1_Standby_Enable    :1;
        CBIOS_U8    Virtual_DCLK1_Suspend_Enable    :1;
        CBIOS_U8    True_DCLK1_Standby_Enable    :1;
        CBIOS_U8    True_DCLK1_Suspend_Enable    :1;
        CBIOS_U8    MMIO_Access_Only    :1;
    };
}REG_SR09;


typedef union _REG_SR0A    //iga_blank_adjust
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    adjust_iga1__blank    :1;
        CBIOS_U8    adjust_iga2__blank    :1;
        CBIOS_U8    adjust_iga3__blank    :1;
        CBIOS_U8    reserved    :5;
    };
}REG_SR0A;


typedef union _REG_SR0B    //DCLK2_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK2_Power_Down    :1;
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    PLL_MTEST1    :1;
        CBIOS_U8    True_DCLK3_Standby_Enable    :1;
        CBIOS_U8    True_DCLK3_Suspend_Enable    :1;
        CBIOS_U8    Reserved_2    :2;
    };
}REG_SR0B;


typedef union _REG_SR0D    //Extended_Sequencer_D_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    DCLK1_Invert_Enable    :1;
        CBIOS_U8    DCLK2_Invert_Enable    :1;
        CBIOS_U8    DCLK3_Invert_Enable    :1;
        CBIOS_U8    CRT1_HSYNC    :2;
        CBIOS_U8    CRT1_VSYNC    :2;
    };
}REG_SR0D;


typedef union _REG_SR0E    //DCLK2_PLL_Integer_M_and_R_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK2_M_Integer    :8;
    };
}REG_SR0E;


typedef union _REG_SR0F    //DCLK2_PLL_Fractional_M_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK2_M_Fractional_7to0    :8;
    };
}REG_SR0F;


typedef union _REG_SR10    //VGA_Clock_PLL_R_Divider_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    VGA_R_25_175_MHz    :2;
        CBIOS_U8    PLL1_R    :2;
        CBIOS_U8    VGA1_M    :2;
        CBIOS_U8    VGA_R_28_322_MHz    :2;
    };
}REG_SR10;


typedef union _REG_SR12    //DCLK1_PLL_Integer_M_and_R_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK1_M_Integer    :8;
    };
}REG_SR12;


typedef union _REG_SR13    //DCLK1_PLL_Fractional_M_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK1_M_Fract_7to0    :8;
    };
}REG_SR13;


typedef union _REG_SR14    //PLL_Clock_Synthesizer_Control_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK1_Power_Down_Test    :1;
        CBIOS_U8    Counter_Timer_Enable    :1;
        CBIOS_U8    Enable_Counters    :1;
        CBIOS_U8    MTEST0    :1;
        CBIOS_U8    Clear_Counter    :1;
        CBIOS_U8    Reserved    :3;
    };
}REG_SR14;


typedef union _REG_SR15    //PLL_Clock_Synthesizer_Control_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK3_M_R_Load    :1;
        CBIOS_U8    DCLK1_PLL_LOAD    :1;
        CBIOS_U8    DCLK1_M_R_Load    :1;
        CBIOS_U8    DCLK2_PLL_LOAD    :1;
        CBIOS_U8    DCLK3_PLL_LOAD    :1;
        CBIOS_U8    DCLK2_M_R_Load    :1;
        CBIOS_U8    Auto_Load_Reset_Enable    :1;
        CBIOS_U8    For_VGA_mem_access    :1;
    };
}REG_SR15;


typedef union _REG_SR16    //CLKSYN_Test_High_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Clock_Test_Results_High_Byte    :8;
    };
}REG_SR16;


typedef union _REG_SR17    //CLKSYN_Test_Low_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Clock_Test_Results_Low_Byte    :8;
    };
}REG_SR17;


typedef union _REG_SR18    //DAC/CLKSYN_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reset_Signature_Test    :1;
        CBIOS_U8    Signature_Data_to_Read    :3;
        CBIOS_U8    Signature_Test_Data_Source    :1;
        CBIOS_U8    DAC1_Power_Up    :1;
        CBIOS_U8    Reserved    :2;
    };
}REG_SR18;


typedef union _REG_SR19    //SP1_Gamma_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :2;
        CBIOS_U8    SP1_LUT_Interpolation_Enable    :1;
        CBIOS_U8    Dac1_Test_Mode    :2;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    SP1_Gamma    :2;
    };
}REG_SR19;


typedef union _REG_SR1A    //Extended_Sequencer_1A_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Debug_sel_11_8    :4;
        CBIOS_U8    Software_Debug_Disable    :1;
        CBIOS_U8    CLUT2_Configuration    :2;
        CBIOS_U8    SP1_LUT_Split    :1;
    };
}REG_SR1A;


typedef union _REG_SR1B    //Extended_Sequencer_1B_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :3;
        CBIOS_U8    SP1_CRT_Gamma    :2;
        CBIOS_U8    CLUT1_Configuration    :2;
        CBIOS_U8    Controller1_DCLK_Parm_Source    :1;
    };
}REG_SR1B;


typedef union _REG_SR1C    //Extended_Sequencer_1C_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SP2_LUT_Interpolation_Enable    :1;
        CBIOS_U8    SP2_LUT_Split    :1;
        CBIOS_U8    TV_Encoder_Clock_Power_Down    :1;
        CBIOS_U8    TV_Encoder_Clock_Standby_Power_Down    :1;
        CBIOS_U8    TV_Encoder_Clock_Suspend_Power_Down    :1;
        CBIOS_U8    Reserved    :3;
    };
}REG_SR1C;


typedef union _REG_SR1D    //Extended_Sequencer_ID_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PCI_PAD_PD_Enable    :1;
        CBIOS_U8    Reserved_0    :2;
        CBIOS_U8    SP1_DCLK_PD_Enable    :1;
        CBIOS_U8    SP1_DCLK_Standby_PD_Enable    :1;
        CBIOS_U8    SP1_DCLK_Suspend_PD_Enable    :1;
        CBIOS_U8    SP3_DCLK_Suspend_PD_Enable    :1;
        CBIOS_U8    Reserved_1    :1;
    };
}REG_SR1D;


typedef union _REG_SR1E    //Power_Management_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Controller_1_DCLK_PD_Enable    :1;
        CBIOS_U8    PLL_Suspend_Power_Down_Enable    :1;
        CBIOS_U8    Controller_2_DCLK_Power_Down    :1;
        CBIOS_U8    Controller_3_DCLK_Power_Down    :1;
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    Controller_2_Standby_Power_Down    :1;
        CBIOS_U8    Controller_2_Suspend_Power_Down    :1;
        CBIOS_U8    Reserved_1    :1;
    };
}REG_SR1E;


typedef union _REG_SR1F    //Extended_Sequencer_1F_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Engine_Clock_Suspend_Enable    :1;
        CBIOS_U8    Reserved    :2;
        CBIOS_U8    PCIE0_PM_Enable    :1;
        CBIOS_U8    Reserved_7to4    :4;
    };
}REG_SR1F;


typedef union _REG_SR20    //Extended_Sequencer_20_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK2_PLL_Suspend_Enable    :1;
        CBIOS_U8    DAC1_Standby_Enable    :1;
        CBIOS_U8    DAC1_Suspend_Enable    :1;
        CBIOS_U8    DCLK1_PLL_Suspend_Enable    :1;
        CBIOS_U8    MCLK_PLL_Suspend_Enable    :1;
        CBIOS_U8    Reserved    :2;
        CBIOS_U8    Pads_Suspend_Enable    :1;
    };
}REG_SR20;


typedef union _REG_SR21    //Extended_Sequencer_21_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    CLUT1_Disable    :1;
        CBIOS_U8    DAC1_SENSE_Power_Down_Enable    :1;
        CBIOS_U8    CLUT1_Standby_Enable    :1;
        CBIOS_U8    CLUT1_Suspend_Enable    :1;
        CBIOS_U8    CLUT2_Disable    :1;
        CBIOS_U8    SEN_SEL    :1;
        CBIOS_U8    CLUT2_Standby_Enable    :1;
        CBIOS_U8    CLUT2_Suspend_Enable    :1;
    };
}REG_SR21;


typedef union _REG_SR22    //VGA_Clock_1_Low_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    VGA1_PLL_Integral_M_Divider    :6;
        CBIOS_U8    External_D1CLK    :1;
        CBIOS_U8    External_D2CLK    :1;
    };
}REG_SR22;


typedef union _REG_SR23    //VGA_Clock_1_High_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    VGA1_PLL_Fractional_M_Divider_19to12    :8;
    };
}REG_SR23;


typedef union _REG_SR24    //VGA_Clock2_M_Integer_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    VGA2_PLL_Integral_M_Dividier    :8;
    };
}REG_SR24;


typedef union _REG_SR25    //VGA_Clock2_M_Fractional_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    VGA2_PLL_Fractional_M_Divider    :8;
    };
}REG_SR25;


typedef union _REG_SR26    //Paired_Register_Read/Write_Select_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Controller_Reads_select    :2;
        CBIOS_U8    Controller1_write_Disable    :1;
        CBIOS_U8    Controller2_Writes_Enable    :1;
        CBIOS_U8    Controller3_Writes_Enable    :1;
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    Vertical_Interrupt_Source    :2;
    };
}REG_SR26;


typedef union _REG_SR27    //DAC_Current_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DAC1_Gain_Adjust    :3;
        CBIOS_U8    DAC1_BLANK_Pedestal_Enable    :1;
        CBIOS_U8    RD0_DAC1    :1;
        CBIOS_U8    RD1_DAC1    :1;
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    Controller1_DAC_Blank_Power_Down    :1;
    };
}REG_SR27;


typedef union _REG_SR28    //DCLK_PLL_IREF_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :2;
        CBIOS_U8    DCLK1_IREF    :2;
        CBIOS_U8    DCLK2_IREF    :2;
        CBIOS_U8    Reserved_1    :2;
    };
}REG_SR28;


typedef union _REG_SR29    //DCLK_PLL_M_and_R_Value_Overflow_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK1_Fractional_M_bit_19to16    :4;
        CBIOS_U8    DCLK2_R    :2;
        CBIOS_U8    Reserved_7to6    :2;
    };
}REG_SR29;


typedef union _REG_SR2A    //Paired_/_Alternate_Register_Select_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Paired_SR_Registers_Select_bit0    :1;
        CBIOS_U8    Alternate_D_Registers_Select    :2;
        CBIOS_U8    Paired_CR_Registers_Select_bit0    :1;
        CBIOS_U8    reserved    :1;
        CBIOS_U8    Paired_SR_Registers_Select_bit1    :1;
        CBIOS_U8    MIUCRALL    :1;
        CBIOS_U8    Paired_CR_Registers_Select_bit1    :1;
    };
}REG_SR2A;


typedef union _REG_SR2B    //DVO_Data_Source_Select_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DVO1_Source_Select    :2;
        CBIOS_U8    Reserved_7to2    :6;
    };
}REG_SR2B;


typedef union _REG_SR2C    //LVDS_Power_Up_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    LVDS_Power_Up_0    :1;
        CBIOS_U8    LVDS_Power_Up_1    :1;
        CBIOS_U8    LVDS_Power_Down_Standby_Enable    :1;
        CBIOS_U8    LVDS_Power_Down_Suspend_Enable    :1;
        CBIOS_U8    LVDS_Power_Down_FP_Inactive_Enable    :1;
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    PDCH3_L    :1;
        CBIOS_U8    Reserved_1    :1;
    };
}REG_SR2C;


typedef union _REG_SR2D    //DVO-1_Port_Control_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :4;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    Reserved_2    :1;
        CBIOS_U8    Invert_DVO1_HSYNC    :1;
        CBIOS_U8    Invert_DVO1_VSYNC    :1;
    };
}REG_SR2D;


typedef union _REG_SR2E    //high_bit__for_timing_control_register1_when_fp_is_enalbed__pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Horizontal_total_bit_10    :1;
        CBIOS_U8    Horizontal_Display_End_bit_10    :1;
        CBIOS_U8    Start_Horizontal_Blank_bit_10    :1;
        CBIOS_U8    Horizontal_Blank_end_bit_9_7    :3;
        CBIOS_U8    Start_Horizontal_Sync_Position_bit_10    :1;
        CBIOS_U8    reserved    :1;
    };
}REG_SR2E;


typedef union _REG_SR2F    //high_bit__for_timing_control_register2_when_fp_is_enabled_pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Vertical_Total_bit_12    :1;
        CBIOS_U8    Vertical_Display_End_bit_12    :1;
        CBIOS_U8    Start_Vertical_Blank_bit_12    :1;
        CBIOS_U8    End_Vertical_Blank_bit8    :1;
        CBIOS_U8    Vertical_Retrace_Start_bit_12    :1;
        CBIOS_U8    Vertical_Retrace_End_bit9_8    :2;
        CBIOS_U8    reserved    :1;
    };
}REG_SR2F;


typedef union _REG_SR30_Pair    //Architecture_Configuration_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS2_VGA_Enable    :1;
        CBIOS_U8    Reset_CLK_DIV    :1;
        CBIOS_U8    reserved    :1;
        CBIOS_U8    Controller1_DCLK    :1;
        CBIOS_U8    Controller2_DCLK    :1;
        CBIOS_U8    VGA3_enable    :1;
        CBIOS_U8    Controller3_DCLK    :1;
        CBIOS_U8    SS2_FIFO_share    :1;
    };
}REG_SR30_Pair;


typedef union _REG_SR30_B    //Architecture_Configuration_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved_0    :2;
        CBIOS_U8    HDTV1_Data_Source_Select    :1;
        CBIOS_U8    reserved_1    :5;
    };
}REG_SR30_B;


typedef union _REG_SR30_T    //Architecture_Configuration_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved_0    :2;
        CBIOS_U8    HDTV2_Data_Source_Select    :1;
        CBIOS_U8    reserved_1    :5;
    };
}REG_SR30_T;


typedef union _REG_SR31_Pair    //Flat_Panel_Display_Mode_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    F1C    :1;
        CBIOS_U8    reserved    :1;
        CBIOS_U8    DAC1_Source_Select    :2;
        CBIOS_U8    Controller1_is_Flat_Panel_Source    :1;
        CBIOS_U8    Power_On_Sequence    :1;
        CBIOS_U8    DAC1_SENSE_Source    :1;
        CBIOS_U8    DAC_SENSE_INT    :1;
    };
}REG_SR31_Pair;


typedef union _REG_SR31_B_Pair    //Flat_Panel_Display_Mode_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved_0    :1;
        CBIOS_U8    reserved_1    :1;
        CBIOS_U8    reserved_2    :2;
        CBIOS_U8    Controller2_is_Flat_Panel_Source    :1;
        CBIOS_U8    reserved_3    :3;
    };
}REG_SR31_B_Pair;


typedef union _REG_SR31_T_Pair    //Flat_Panel_Display_Mode_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved_0    :1;
        CBIOS_U8    reserved_1    :1;
        CBIOS_U8    reserved_2    :2;
        CBIOS_U8    Controller3_is_Flat_Panel_Source    :1;
        CBIOS_U8    reserved_3    :3;
    };
}REG_SR31_T_Pair;


typedef union _REG_SR32    //DVO_Port_Output_Enable_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DVO_1_DAT_Pad_Out    :1;
        CBIOS_U8    DVO_1_HSYNC_VSYNC_Pad_Out    :1;
        CBIOS_U8    DVO_1_DEN_CLK_Pad_Out    :1;
        CBIOS_U8    DVO_1_CLKB_Pad_Out    :1;
        CBIOS_U8    DIU_BIU_DVO_INT_ien    :1;
        CBIOS_U8    reserved    :1;
        CBIOS_U8    DAC_mode_en    :1;
        CBIOS_U8    CRT_H_VSYNC_output_en    :1;
    };
}REG_SR32;


typedef union _REG_SR33    //Flat_Panel_Function_Control_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    LVDS_PLL_Lock_Status    :1;
        CBIOS_U8    DAC_test_mode    :1;
        CBIOS_U8    GPIO0_Input_Data    :1;
        CBIOS_U8    LVDS_TEST_Enable    :1;
        CBIOS_U8    DAC_TEST_Enable    :1;
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    GPIO1_Input_Data    :1;
        CBIOS_U8    consider_MXU_DIU_BUSY_to_generate_G2N_VB_C4P    :1;
    };
}REG_SR33;


typedef union _REG_SR34    //GFX_MISC_PAD_PD/PU_select
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    GFX_MISC_PAD_PD_sel    :1;
        CBIOS_U8    GFX_MISC_PAD_PU_sel    :1;
        CBIOS_U8    GFX_MISC_PAD_PU0_sel    :1;
        CBIOS_U8    GFX_MISC_PAD_PU1_sel    :1;
        CBIOS_U8    DVO_PAD_PD_sel    :1;
        CBIOS_U8    DVO_PAD_PAD_PU_sel    :1;
        CBIOS_U8    Reserved    :2;
    };
}REG_SR34;


typedef union _REG_SR35_Pair    //Flat_Panel_1_Function_Control_2_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    GPIO0_Select    :2;
        CBIOS_U8    TMDS_Y_Z_Sense_or_DC_GOP0_Status    :1;
        CBIOS_U8    GPIO0_Data    :1;
        CBIOS_U8    TMDS_YZ_Sense_or_DC_GPOUT1_Status    :1;
        CBIOS_U8    FP_Test_Mode_1    :1;
        CBIOS_U8    FP_Test_Mode_2    :1;
        CBIOS_U8    General_Test_Mode_2    :1;
    };
}REG_SR35_Pair;


typedef union _REG_SR35_B_Pair    //Flat_Panel_2_Function_Control_2_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    GPIO1_Select    :2;
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    GPIO1_Data    :1;
        CBIOS_U8    Reserved_1    :4;
    };
}REG_SR35_B_Pair;


typedef union _REG_SR36    //Flat_Panel_Dither_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Dither_Pattern_Select :1; // 0:dither 2x2 1:dither 4x4
        CBIOS_U8    Reserved_1 :2;
        CBIOS_U8    Dither_Bit_Select  :3;
        CBIOS_U8    Dither_EN :1;
        CBIOS_U8    Reserved_2 :1;
    };
}REG_SR36;


typedef union _REG_SR36_B    //GPIO6_and_GPIO7_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Dither_Pattern_Select :1; // 0:dither 2x2 1:dither 4x4
        CBIOS_U8    Reserved_1 :2;
        CBIOS_U8    Dither_Bit_Select  :3;
        CBIOS_U8    Dither_EN :1;
        CBIOS_U8    Reserved_2 :1;
    };
}REG_SR36_B;


typedef union _REG_SR37    //Flat_Panel_Pulse_Width_Modulation_(PWM)_Enable_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PWM0_Enable    :1;
        CBIOS_U8    PWM_FQ_CNTL_1    :3;
        CBIOS_U8    PWM_FQ_CNTL_2    :3;
        CBIOS_U8    PWM1_Enable    :1;
    };
}REG_SR37;


typedef union _REG_SR38    //Flat_Panel_PWM0__Duty_Cycle_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PWM0_Duty_Cycle_LSB    :8;
    };
}REG_SR38;


typedef union _REG_SR39    //DVO-1_Port_Control_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DVO_mapping_mode    :1;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    DVO_mode_select    :1;
        CBIOS_U8    DVO1_Delta_Delay    :4;
        CBIOS_U8    Invert_DVO1_CLK    :1;
    };
}REG_SR39;


typedef union _REG_SR3A_Pair    //DP_PHY_Source_Select_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DP_PHY_Test_Mode_Select    :3;
        CBIOS_U8    DP_PHY_Source_Sel    :3;
        CBIOS_U8    FP_DP_HYSNC_POL    :1;
        CBIOS_U8    FP_DP_VYSNC_POL    :1;
    };
}REG_SR3A_Pair;


typedef union _REG_SR3B_Pair    //DP_PHY_Source_Select_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Sel_DP_TMDS    :1;
        CBIOS_U8    CLK_Delay    :4;
        CBIOS_U8    Reserved    :2;
        CBIOS_U8    DP_test_enable    :1;
    };
}REG_SR3B_Pair;


typedef union _REG_SR3C_Pair    //LVDS_Dither_Enable_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    FPVDD_high_to_FP_signals_active    :1;
        CBIOS_U8    FP_Active_to_FPVEE_High_0    :1;
        CBIOS_U8    FPVEE_Low_to_Flat_panel_signals_inactive    :1;
        CBIOS_U8    ENVEE    :1;
        CBIOS_U8    ENVDD    :1;
        CBIOS_U8    LVDS_RGB_Distributed_Dither    :1;
        CBIOS_U8    FP_Active_to_FPVEE_High_1    :2;
    };
}REG_SR3C_Pair;


typedef union _REG_SR3C_B    //LVDS_Dither_Enable_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved_0    :5;
        CBIOS_U8    LVDS_RGB_Distributed_Dither    :1;
        CBIOS_U8    reserved_1    :2;
    };
}REG_SR3C_B;


typedef union _REG_SR3D    //LVDS_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    ENVDD_oen_en    :1;
        CBIOS_U8    ENVEE_oen_en    :1;
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    Color_Mapping_Mode_Select    :1;
        CBIOS_U8    LVDS_Test_Mode    :1;
        CBIOS_U8    Flat_Panel_Source_Select    :3;
    };
}REG_SR3D;


typedef union _REG_SR3E    //Flat_Panel_PWM1_Pulse_Width_Modulation_Duty_Cycle_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PWM1_Duty_Cycle_LSB    :8;
    };
}REG_SR3E;


typedef union _REG_SR3F    //DAC_SENSE_LSB_and_Mode_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    B_Sense_1to0    :2;
        CBIOS_U8    G_Sense_1to0    :2;
        CBIOS_U8    R_Sense_1to0    :2;
        CBIOS_U8    SENSE_Mode    :1;
        CBIOS_U8    Reserved    :1;
    };
}REG_SR3F;


typedef union _REG_SR40    //LVDS_Configuration_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    Reserved_1    :2;
        CBIOS_U8    Reserved_2    :1;
        CBIOS_U8    PWM_Disable    :1;
        CBIOS_U8    LVDS_Test_Mode    :3;
    };
}REG_SR40;


typedef union _REG_SR41    //Flat_Panel_Power_Sequence_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    FPVEE_Low    :2;
        CBIOS_U8    FPVDD_Low    :2;
        CBIOS_U8    FPVDD_High    :2;
        CBIOS_U8    Standby_Timer_Resolution    :2;
    };
}REG_SR41;


typedef union _REG_SR42    //Flat_Panel_Power_Management_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Sync_Outputs    :1;
        CBIOS_U8    SW_Suspend_Enable    :1;
        CBIOS_U8    FPVSYNC_Tri_State    :1;
        CBIOS_U8    FPHSYNC_Tri_State    :1;
        CBIOS_U8    Hardware_Standby    :1;
        CBIOS_U8    Software_Standby    :1;
        CBIOS_U8    Suspend_Debounce_Timer    :2;
    };
}REG_SR42;


typedef union _REG_SR43    //Flat_Panel_Standby_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Standby_Timeout    :6;
        CBIOS_U8    Activity_Enable    :1;
        CBIOS_U8    FP_signals_inactive_to_FPVDD_low    :1;
    };
}REG_SR43;


typedef union _REG_SR44    //Flat_Panel_Power_Management_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :4;
        CBIOS_U8    STANDBY_Pin    :1;
        CBIOS_U8    Reserved_1    :2;
        CBIOS_U8    DAC_Power_Down    :1;
    };
}REG_SR44;


typedef union _REG_SR45    //Flat_Panel_PLL_Power_Management_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PLL_Wait    :2;
        CBIOS_U8    Hot_Plug_Select    :1;
        CBIOS_U8    PWM1_Control    :1;
        CBIOS_U8    PWM1_Function_Select    :1;
        CBIOS_U8    Reserved    :2;
        CBIOS_U8    FP_Hot_Plug_Function_Enable    :1;
    };
}REG_SR45;


typedef union _REG_SR46    //Flat_Panel_Power_Management_Status_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PM_Current_State    :5;
        CBIOS_U8    Standby_Status    :1;
        CBIOS_U8    Idle_Power_Down_State    :1;
        CBIOS_U8    Idle_Power_Up_State    :1;
    };
}REG_SR46;


typedef union _REG_SR47    //CLUT_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    CLUT_Select     :4;
        CBIOS_U8    SP_Gamma_Enable :1;
        CBIOS_U8    SP_LUT_Interpolation_Enable    :1;
        CBIOS_U8    Reserved    :2;
    };
}REG_SR47;


typedef union _REG_SR48    //GPIO3_and_GPIO4_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    GPIO3_Ouptut_Data    :1;
        CBIOS_U8    GPIO3_Output_Enable    :1;
        CBIOS_U8    GPIO3_Input_Enable    :1;
        CBIOS_U8    GPIO3_Input_Data    :1;
        CBIOS_U8    GPIO4_Ouptut_Data    :1;
        CBIOS_U8    GPIO4_Output_Enable    :1;
        CBIOS_U8    GPIO4_Input_Enable    :1;
        CBIOS_U8    GPIO4_Input_Data    :1;
    };
}REG_SR48;


typedef union _REG_SR49    //Primary_Stream_1_Scalar_Destination_Width_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS1_Scalar_Dest_Width_bit12    :1;
        CBIOS_U8    Reserved    :7;
    };
}REG_SR49;


typedef union _REG_SR4A    //GPIO5_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    GPIO5_Ouptut_Data    :1;
        CBIOS_U8    GPIO5_Output_Enable    :1;
        CBIOS_U8    GPIO5_Input_Enable    :1;
        CBIOS_U8    GPIO5_Input_Data    :1;
        CBIOS_U8    Reserved    :4;
    };
}REG_SR4A;


typedef union _REG_SR4B    //R_SENSE_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    R_SENSE    :8;
    };
}REG_SR4B;


typedef union _REG_SR4C    //G_SENSE_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    G_SENSE    :8;
    };
}REG_SR4C;


typedef union _REG_SR4D    //B_SENSE_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    B_SENSE    :8;
    };
}REG_SR4D;


typedef union _REG_SR4E    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR4E;


typedef union _REG_SR4E_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR4E_B;


typedef union _REG_SR4F    //Primary_Stream_1_Upscaler_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS1_Upscaler_Enable    :1;
        CBIOS_U8    PS1_Horizontal_upscaling_Enable    :1;
        CBIOS_U8    PS1_Vertical_upscaling_Enable    :1;
        CBIOS_U8    PS1_Upscaler_Auto_Ratio    :1;
        CBIOS_U8    PS1_Upscaler_Cosine_Interpolation_Enable    :1;
        CBIOS_U8    Reserved    :2;
        CBIOS_U8    PS1_Upscaler_RATIO_MUTE_1    :1;
    };
}REG_SR4F;


typedef union _REG_SR4F_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR4F_B;


typedef union _REG_SR50    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR50;


typedef union _REG_SR50_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR50_B;


typedef union _REG_SR51    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR51;


typedef union _REG_SR51_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR51_B;


typedef union _REG_SR52    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR52;


typedef union _REG_SR52_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR52_B;


typedef union _REG_SR53    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR53;


typedef union _REG_SR53_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR53_B;


typedef union _REG_SR54    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR54;


typedef union _REG_SR54_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR54_B;


typedef union _REG_SR55    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR55;


typedef union _REG_SR55_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR55_B;


typedef union _REG_SR56    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR56;


typedef union _REG_SR56_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR56_B;


typedef union _REG_SR57    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR57;


typedef union _REG_SR57_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR57_B;


typedef union _REG_SR58    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR58;


typedef union _REG_SR58_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR58_B;


typedef union _REG_SR59    //Primary_Stream_1_Upscaler_Destination_Width_7:0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS1_Upscaler_Dest_Width_7_0    :8;
    };
}REG_SR59;


typedef union _REG_SR59_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR59_B;


typedef union _REG_SR5A    //Primary_Stream_1_Upscaler_Destination_Overflow_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS1_Upscaler_Dest_width    :4;
        CBIOS_U8    PS1_Upscaler_Dest_Height   :4;
    };
}REG_SR5A;


typedef union _REG_SR5A_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR5A_B;


typedef union _REG_SR5B    //Primary_Stream_1_Upscaler_Destination_Height_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS1_Upscaler_Dest_Height_7_0    :8;
    };
}REG_SR5B;


typedef union _REG_SR5B_B    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_SR5B_B;


typedef union _REG_SR5C_Pair    //Flat_Panel_Enable_Position_Control_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    FP_Display_Enable_Control_1    :4;
        CBIOS_U8    FP_Display_Enable_Control_2    :4;
    };
}REG_SR5C_Pair;


typedef union _REG_SR5D_Pair    //Flat_Panel_/_CRT_Sync_Position_Control_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    FP_CRT_Sync_Control_1    :4;
        CBIOS_U8    FP_CRT_Sync_Control_2    :4;
    };
}REG_SR5D_Pair;


typedef union _REG_SR5E_Pair    //Flat_Panel_BIOS_Scratch_1_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    BIOS_Scratch    :8;
    };
}REG_SR5E_Pair;


typedef union _REG_SR5F_Pair    //Flat_Panel_BIOS_Scratch_2_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    BIOS_Scratch    :8;
    };
}REG_SR5F_Pair;


typedef union _REG_SR60_Pair    //Flat_Panel_Horizontal_Total_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Horizontal_Total_7to0    :8;
    };
}REG_SR60_Pair;


typedef union _REG_SR61_Pair    //Flat_Panel_Horizontal_Panel_Size_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Horizontal_Panel_Size_7to0    :8;
    };
}REG_SR61_Pair;


typedef union _REG_SR62_Pair    //Flat_Panel_Horizontal_Blank_Start_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Horizontal_Blank_Start_7to0    :8;
    };
}REG_SR62_Pair;


typedef union _REG_SR63_Pair    //Flat_Panel_Horizontal_Blank_End_Low_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Horizontal_Blank_End_6to0    :7;
        CBIOS_U8    Reserved    :1;
    };
}REG_SR63_Pair;


typedef union _REG_SR64_Pair    //Flat_Panel_Horizontal_Sync_Start_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Horizontal_Sync_Start_7to0    :8;
    };
}REG_SR64_Pair;


typedef union _REG_SR65_Pair    //Flat_Panel_Horizontal_Sync_End_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Horizontal_Sync_End_5to0    :6;
        CBIOS_U8    Reserved    :2;
    };
}REG_SR65_Pair;


typedef union _REG_SR66_Pair    //Flat_Panel_Horizontal_Overflow_Register_Pair_1
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Horizontal_Total_bit_8    :1;
        CBIOS_U8    Horizontal_Panel_Size_bit_8    :1;
        CBIOS_U8    Horizontal_Blank_Start_bit_8    :1;
        CBIOS_U8    Horizontal_Sync_Start_bit_8    :1;
        CBIOS_U8    Reserved    :4;
    };
}REG_SR66_Pair;


typedef union _REG_SR67    //Flat_Panel_Horizontal_Overflow_Register_Pair_2
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Horizontal_Total_bit_9    :1;
        CBIOS_U8    Horizontal_Panel_Size_bit_9    :1;
        CBIOS_U8    Horizontal_Blank_Start_bit_9    :1;
        CBIOS_U8    Horizontal_Sync_Start_bit_9    :1;
        CBIOS_U8    Reserved    :4;
    };
}REG_SR67;


typedef union _REG_SR68_Pair    //Flat_Panel_Vertical_Total_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Vertical_Total_7to0    :8;
    };
}REG_SR68_Pair;


typedef union _REG_SR69_Pair    //Flat_Panel_Vertical_Panel_Size_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Vertical_Panel_Size_7to0    :8;
    };
}REG_SR69_Pair;


typedef union _REG_SR6A_Pair    //Flat_Panel_Vertical_Blank_Start_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Vertical_Blank_Start_7to0    :8;
    };
}REG_SR6A_Pair;


typedef union _REG_SR6B_Pair    //Flat_Panel_Vertical_Blank_End_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Vertical_Blank_End_7to0    :8;
    };
}REG_SR6B_Pair;


typedef union _REG_SR6C_Pair    //Flat_Panel_Vertical_Sync_Start_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Vertical_Sync_Start_7to0    :8;
    };
}REG_SR6C_Pair;


typedef union _REG_SR6D_Pair    //Flat_Panel_Vertical_Sync_End_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Vertical_Sync_End_3to0    :4;
        CBIOS_U8    Reserved    :4;
    };
}REG_SR6D_Pair;


typedef union _REG_SR6E_Pair    //Flat_Panel_Vertical_Overflow_1_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Vertical_Total_11to8    :4;
        CBIOS_U8    Vertical_Panel_Size_11to8    :4;
    };
}REG_SR6E_Pair;


typedef union _REG_SR6F_Pair    //Flat_Panel_Vertical_Overflow_2_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Vertical_Blank_Start_11to8    :4;
        CBIOS_U8    Vertical_Sync_Start_11to8    :4;
    };
}REG_SR6F_Pair;


typedef union _REG_SR70_B    //HDTV1_Mode_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Progressive_Mode_Enable    :1;
        CBIOS_U8    SMPTE_274M_Enable    :1;
        CBIOS_U8    SMPTE_296M_Enable    :1;
        CBIOS_U8    SMPTE_293M_Enable    :1;
        CBIOS_U8    _576P_Enable    :1;
        CBIOS_U8    HDTV_Timing_Enable_Control    :1;
        CBIOS_U8    ITU470_SELECT    :1;
        CBIOS_U8    _576i_480i_enable    :1;
    };
}REG_SR70_B;


typedef union _REG_SR70_T    //HDTV2_Mode_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Progressive_Mode_Enable    :1;
        CBIOS_U8    SMPTE_274M_Enable    :1;
        CBIOS_U8    SMPTE_296M_Enable    :1;
        CBIOS_U8    SMPTE_293M_Enable    :1;
        CBIOS_U8    _576P_Enable    :1;
        CBIOS_U8    HDTV_Timing_Enable_Control    :1;
        CBIOS_U8    ITU470_SELECT    :1;
        CBIOS_U8    _576i_480i_enable    :1;
    };
}REG_SR70_T;


typedef union _REG_SR71_B    //HDTV1_Tri-Level_SYNC_Width
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Trilevel_Sync_Width    :6;
        CBIOS_U8    HDTV_EN_TRIG_PULSE    :2;
    };
}REG_SR71_B;


typedef union _REG_SR71_T    //HDTV2_Tri-Level_SYNC_Width
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Trilevel_Sync_Width    :6;
        CBIOS_U8    HDTV_EN_TRIG_PULSE    :2;
    };
}REG_SR71_T;


typedef union _REG_SR72_B    //HDTV1_BLANK_Level_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Blank_Level_7to0    :8;
    };
}REG_SR72_B;


typedef union _REG_SR72_T    //HDTV2_BLANK_Level_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Blank_Level_7to0    :8;
    };
}REG_SR72_T;


typedef union _REG_SR73_B    //HDTV1_BLANK_Level_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Blank_Level_9to8    :2;
        CBIOS_U8    _1x_2x_4x_oversampling_sel    :2;
        CBIOS_U8    Reserved    :4;
    };
}REG_SR73_B;


typedef union _REG_SR73_T    //HDTV2_BLANK_Level_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Blank_Level_9to8    :2;
        CBIOS_U8    _1x_2x_4x_oversampling_sel    :2;
        CBIOS_U8    Reserved    :4;
    };
}REG_SR73_T;


typedef union _REG_SR74_B    //HDTV1_CSC_input_data_format
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_input_data_format    :5;
        CBIOS_U8    HDTV_CSC_coefficients_sel    :1;
        CBIOS_U8    HDTV_old_slope_enable    :1;
        CBIOS_U8    Reserved    :1;
    };
}REG_SR74_B;


typedef union _REG_SR74_T    //HDTV2_CSC_input_data_format
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_input_data_format    :5;
        CBIOS_U8    HDTV_CSC_coefficients_sel    :1;
        CBIOS_U8    HDTV_old_slope_enable    :1;
        CBIOS_U8    Reserved    :1;
    };
}REG_SR74_T;


typedef union _REG_SR75_B    //HDTV_1_contrast_tuning_register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Contrast_tuning_value    :8;
    };
}REG_SR75_B;


typedef union _REG_SR75_T    //HDTV2_contrast_tuning_register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Contrast_tuning_value    :8;
    };
}REG_SR75_T;


typedef union _REG_SR76_B    //HDTV1_Pb_saturation_tuning
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Pb_Saturation_tuning_value    :8;
    };
}REG_SR76_B;


typedef union _REG_SR76_T    //HDTV2_Pb_saturation_tuning
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Pb_Saturation_tuning_value    :8;
    };
}REG_SR76_T;


typedef union _REG_SR77_B    //HDTV1_Pr_saturation_tuning
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Pr_Saturation_tuning_value    :8;
    };
}REG_SR77_B;


typedef union _REG_SR77_T    //HDTV2_Pr_saturation_tuning
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Pr_Saturation_tuning_value    :8;
    };
}REG_SR77_T;


typedef union _REG_SR78_B    //HDTV1_576i_initial_programming_data_enable
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    _576i_initial_programming    :2;
        CBIOS_U8    HDTV__pbpr_dac_filter_select_coef    :5;
        CBIOS_U8    Reserved    :1;
    };
}REG_SR78_B;


typedef union _REG_SR78_T    //HDTV2_576i_initial_programming_data_enable
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    _576i_initial_programming    :2;
        CBIOS_U8    HDTV__pbpr_dac_filter_select_coef    :5;
        CBIOS_U8    Reserved    :1;
    };
}REG_SR78_T;


typedef union _REG_SR79_B    //HDTV1_y_dac_filter_select_coef
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_y_dac_filter_select_coef    :5;
        CBIOS_U8    HDMI_SYNC_Delay_9to8    :2;
        CBIOS_U8    Reserved    :1;
    };
}REG_SR79_B;


typedef union _REG_SR79_T    //HDTV2__dac_filter_select_coef
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_y_dac_filter_select_coef    :5;
        CBIOS_U8    HDMI_SYNC_Delay_9to8    :2;
        CBIOS_U8    Reserved    :1;
    };
}REG_SR79_T;


typedef union _REG_SR7A_B    //HDTV1_HDMI_SYNC_Delay_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDMI_SYNC_Delay_7to0    :8;
    };
}REG_SR7A_B;


typedef union _REG_SR7A_T    //HDTV2_HDMI_SYNC_Delay_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDMI_SYNC_Delay_7to0    :8;
    };
}REG_SR7A_T;


typedef union _REG_SR7D_B    //HDTV1_SYNC_Delay_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_SYNC_Delay_10to8    :3;
        CBIOS_U8    Reserved    :5;
    };
}REG_SR7D_B;


typedef union _REG_SR7D_T    //HDTV2_SYNC_Delay_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_SYNC_Delay_10to8    :3;
        CBIOS_U8    Reserved    :5;
    };
}REG_SR7D_T;


typedef union _REG_SR7E_B    //HDTV1_SYNC_Delay_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_SYNC_Delay_7to0    :8;
    };
}REG_SR7E_B;


typedef union _REG_SR7E_T    //HDTV2_SYNC_Delay_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_SYNC_Delay_7to0    :8;
    };
}REG_SR7E_T;


typedef union _REG_SR80_B    //HDTV1_Color_Space_Converter_Factor_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F0_7to0    :8;
    };
}REG_SR80_B;


typedef union _REG_SR80_T    //HDTV2_Color_Space_Converter_Factor_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F0_7to0    :8;
    };
}REG_SR80_T;


typedef union _REG_SR81_B    //HDTV1_Color_Space_Converter_Factor_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F1_7to0    :8;
    };
}REG_SR81_B;


typedef union _REG_SR81_T    //HDTV2_Color_Space_Converter_Factor_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F1_7to0    :8;
    };
}REG_SR81_T;


typedef union _REG_SR82_B    //HDTV1_Color_Space_Converter_Factor_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F2_7to0    :8;
    };
}REG_SR82_B;


typedef union _REG_SR82_T    //HDTV2_Color_Space_Converter_Factor_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F2_7to0    :8;
    };
}REG_SR82_T;


typedef union _REG_SR83_B    //HDTV1_Color_Space_Converter_Factor_3_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F3_7to0    :8;
    };
}REG_SR83_B;


typedef union _REG_SR83_T    //HDTV2_Color_Space_Converter_Factor_3_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F3_7to0    :8;
    };
}REG_SR83_T;


typedef union _REG_SR84_B    //HDTV1_Color_Space_Converter_Factor_4_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F4_7to0    :8;
    };
}REG_SR84_B;


typedef union _REG_SR84_T    //HDTV2_Color_Space_Converter_Factor_4_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F4_7to0    :8;
    };
}REG_SR84_T;


typedef union _REG_SR85_B    //HDTV1_Color_Space_Converter_Factor_5_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F5_7to0    :8;
    };
}REG_SR85_B;


typedef union _REG_SR85_T    //HDTV2_Color_Space_Converter_Factor_5_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F5_7to0    :8;
    };
}REG_SR85_T;


typedef union _REG_SR86_B    //HDTV1_Color_Space_Converter_Factor_6_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F6_7to0    :8;
    };
}REG_SR86_B;


typedef union _REG_SR86_T    //HDTV2_Color_Space_Converter_Factor_6_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F6_7to0    :8;
    };
}REG_SR86_T;


typedef union _REG_SR87_B    //HDTV1_Color_Space_Converter_Factor_7_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F7_7to0    :8;
    };
}REG_SR87_B;


typedef union _REG_SR87_T    //HDTV2_Color_Space_Converter_Factor_7_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F7_7to0    :8;
    };
}REG_SR87_T;


typedef union _REG_SR88_B    //HDTV1_Color_Space_Converter_Factor_8_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F8_7to0    :8;
    };
}REG_SR88_B;


typedef union _REG_SR88_T    //HDTV2_Color_Space_Converter_Factor_8_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CSC_F8_7to0    :8;
    };
}REG_SR88_T;


typedef union _REG_SR89_B    //HDTV1_Left_Blank_Pixels_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Left_Blank_Pixels_10to8    :3;
        CBIOS_U8    Reserved    :5;
    };
}REG_SR89_B;


typedef union _REG_SR89_T    //HDTV2_Left_Blank_Pixels_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Left_Blank_Pixels_10to8    :3;
        CBIOS_U8    Reserved    :5;
    };
}REG_SR89_T;


typedef union _REG_SR8A_B    //HDTV1_Left_Blank_Pixels_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Left_Blank_Pixels_7to0    :8;
    };
}REG_SR8A_B;


typedef union _REG_SR8A_T    //HDTV2_Left_Blank_Pixels_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Left_Blank_Pixels_7to0    :8;
    };
}REG_SR8A_T;


typedef union _REG_SR8E_B    //HDTV1_Brightness_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Brightness_Control    :8;
    };
}REG_SR8E_B;


typedef union _REG_SR8E_T    //HDTV2_Brightness_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Brightness_Control    :8;
    };
}REG_SR8E_T;


typedef union _REG_SR8F_B    //HDTV1_Enable_and_CSC_F8_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_bit0    :1;
        CBIOS_U8    HDTV_CSC_F8_10to8    :3;
        CBIOS_U8    Reserved_bit1    :3;
        CBIOS_U8    HDTV_Enable    :1;
    };
}REG_SR8F_B;


typedef union _REG_SR8F_T    //HDTV2_Enable_and_CSC_F8_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_bit0    :1;
        CBIOS_U8    HDTV_CSC_F8_10to8    :3;
        CBIOS_U8    Reserved_bit1    :3;
        CBIOS_U8    HDTV_Enable    :1;
    };
}REG_SR8F_T;


typedef union _REG_SRA3_B    //HDTV1_Digital_HSYNC_Width_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Digital_HSYNC_Width    :8;
    };
}REG_SRA3_B;


typedef union _REG_SRA3_T    //HDTV2_Digital_HSYNC_Width_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Digital_HSYNC_Width    :8;
    };
}REG_SRA3_T;


typedef union _REG_SRA4_B    //HDTV1_Color_Space_Converter_Factor_0_and_1_Upper_Bits_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    HDTV_CSC_F1_10to8    :3;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    HDTV_CSC_F0_10to8    :3;
    };
}REG_SRA4_B;


typedef union _REG_SRA4_T    //HDTV2_Color_Space_Converter_Factor_0_and_1_Upper_Bits_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    HDTV_CSC_F1_10to8    :3;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    HDTV_CSC_F0_10to8    :3;
    };
}REG_SRA4_T;


typedef union _REG_SRA5_B    //HDTV1_Color_Space_Converter_Factor_2_and_3_Upper_Bits_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    HDTV_CSC_F3_10to8    :3;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    HDTV_CSC_F2_10to8    :3;
    };
}REG_SRA5_B;


typedef union _REG_SRA5_T    //HDTV2_Color_Space_Converter_Factor_2_and_3_Upper_Bits_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    HDTV_CSC_F3_10to8    :3;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    HDTV_CSC_F2_10to8    :3;
    };
}REG_SRA5_T;


typedef union _REG_SRA6_B    //HDTV1_Color_Space_Converter_Factor_4_and_5_Upper_Bits_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    HDTV_CSC_F5_10to8    :3;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    HDTV_CSC_F4_10to8    :3;
    };
}REG_SRA6_B;


typedef union _REG_SRA6_T    //HDTV2_Color_Space_Converter_Factor_4_and_5_Upper_Bits_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    HDTV_CSC_F5_10to8    :3;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    HDTV_CSC_F4_10to8    :3;
    };
}REG_SRA6_T;


typedef union _REG_SRA7_B    //HDTV1_Color_Space_Converter_Factor_6_and_7_Upper_Bits_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    HDTV_CSC_F7_10to8    :3;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    HDTV_CSC_F6_10to8    :3;
    };
}REG_SRA7_B;


typedef union _REG_SRA7_T    //HDTV2_Color_Space_Converter_Factor_6_and_7_Upper_Bits_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    HDTV_CSC_F7_10to8    :3;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    HDTV_CSC_F6_10to8    :3;
    };
}REG_SRA7_T;


typedef union _REG_SRA8_B    //HDTV1_Miscellaneous_Overflow_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :2;
        CBIOS_U8    HDTV_WSS_Clock_Ratio_bit_14    :1;
        CBIOS_U8    Reserved_1    :5;
    };
}REG_SRA8_B;


typedef union _REG_SRA8_T    //HDTV2_Miscellaneous_Overflow_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :2;
        CBIOS_U8    HDTV_WSS_Clock_Ratio_bit_14    :1;
        CBIOS_U8    Reserved_1    :5;
    };
}REG_SRA8_T;


typedef union _REG_SRAA_B    //HDTV1_HSYNC_Delay_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_HSYNC_Delay    :3;
        CBIOS_U8    Reserved    :5;
    };
}REG_SRAA_B;


typedef union _REG_SRAA_T    //HDTV2_HSYNC_Delay_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_HSYNC_Delay    :3;
        CBIOS_U8    Reserved    :5;
    };
}REG_SRAA_T;


typedef union _REG_SRAB_B    //HDTV1_Character_Clock_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Iga1_Character_Clock    :3;
        CBIOS_U8    Iga2_Character_Clock    :3;
        CBIOS_U8    Reserved    :2;
    };
}REG_SRAB_B;


typedef union _REG_SRAB_T    //HDTV2_Character_Clock_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Iga1_Character_Clock    :3;
        CBIOS_U8    Iga2_Character_Clock    :3;
        CBIOS_U8    Reserved    :2;
    };
}REG_SRAB_T;


typedef union _REG_SRAD_B    //HDTV1_Wide_Screen_Signal_(WSS)_Clock_Ratio_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_WSS_Clock_Ratio13to6    :8;
    };
}REG_SRAD_B;


typedef union _REG_SRAD_T    //HDTV2_Wide_Screen_Signal_(WSS)_Clock_Ratio_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_WSS_Clock_Ratio13to6    :8;
    };
}REG_SRAD_T;


typedef union _REG_SRBF_B    //HDTV1_SENSE_Line_Select_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_SENSE_Line_Sleect    :5;
        CBIOS_U8    Reserved    :3;
    };
}REG_SRBF_B;


typedef union _REG_SRBF_T    //HDTV2_SENSE_Line_Select_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_SENSE_Line_Sleect    :5;
        CBIOS_U8    Reserved    :3;
    };
}REG_SRBF_T;


typedef union _REG_SRD0_B    //HDTV1_Broad_Pulse_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Hdtv_Broad_Pulse_6to0    :7;
        CBIOS_U8    Reserved    :1;
    };
}REG_SRD0_B;


typedef union _REG_SRD0_T    //HDTV2_Broad_Pulse_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Hdtv_Broad_Pulse_6to0    :7;
        CBIOS_U8    Reserved    :1;
    };
}REG_SRD0_T;


typedef union _REG_SRD1_B    //HDTV1_Broad_Pulse_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Hdtv_Broad_Pulse_14to7    :8;
    };
}REG_SRD1_B;


typedef union _REG_SRD1_T    //HDTV2_Broad_Pulse_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Hdtv_Broad_Pulse_14to7    :8;
    };
}REG_SRD1_T;


typedef union _REG_SRD2_B    //HDTV1_Half_SYNC_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Hdtv_Half_Sync_7to0    :8;
    };
}REG_SRD2_B;


typedef union _REG_SRD2_T    //HDTV2_Half_SYNC_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Hdtv_Half_Sync_7to0    :8;
    };
}REG_SRD2_T;


typedef union _REG_SRD3_B    //HDTV1_Half_SYNC_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Hdtv_Half_Sync_10to8    :3;
        CBIOS_U8    Reserved    :5;
    };
}REG_SRD3_B;


typedef union _REG_SRD3_T    //HDTV2_Half_SYNC_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Hdtv_Half_Sync_10to8    :3;
        CBIOS_U8    Reserved    :5;
    };
}REG_SRD3_T;


typedef union _REG_SRD6_B    //HDTV1_576i/580i_Closed_Caption_(CC)_Clock_Ratio_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Clock_Ratio_7to0    :8;
    };
}REG_SRD6_B;


typedef union _REG_SRD6_T    //HDTV2_576i/580i_Closed_Caption_(CC)_Clock_Ratio_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Clock_Ratio_7to0    :8;
    };
}REG_SRD6_T;


typedef union _REG_SRD7_B    //HDTV1_576i/480i_Closed_Caption_(CC)_Clock_Ratio_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Clock_Ratio_14to8    :7;
        CBIOS_U8    Reserved    :1;
    };
}REG_SRD7_B;


typedef union _REG_SRD7_T    //HDTV2_576i/480i_Closed_Caption_(CC)_Clock_Ratio_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Clock_Ratio_14to8    :7;
        CBIOS_U8    Reserved    :1;
    };
}REG_SRD7_T;


typedef union _REG_SRD8_B    //HDTV1_576i/480i_Closed_Caption_(CC)_Line_and_Field_Select_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Line_Select    :6;
        CBIOS_U8    HDTV_CC_Field_Select    :2;
    };
}REG_SRD8_B;


typedef union _REG_SRD8_T    //HDTV2_576i/480i_Closed_Caption_(CC)_Line_and_Field_Select_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Line_Select    :6;
        CBIOS_U8    HDTV_CC_Field_Select    :2;
    };
}REG_SRD8_T;


typedef union _REG_SRD9_B    //HDTV1_576i/480i_Closed_Caption_(CC)/WSS_Range_Overflow_and_Word_Mode_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    HDTV_CC_End_10to8    :3;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    HDTV_CC_Start_bit_8    :1;
        CBIOS_U8    HDTV_CC_Sermode    :1;
        CBIOS_U8    HDTV_CC_Word_Mode_Select    :1;
    };
}REG_SRD9_B;


typedef union _REG_SRD9_T    //HDTV2_576i/480i_Closed_Caption_(CC)/WSS_Range_Overflow_and_Word_Mode_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    HDTV_CC_End_10to8    :3;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    HDTV_CC_Start_bit_8    :1;
        CBIOS_U8    HDTV_CC_Sermode    :1;
        CBIOS_U8    HDTV_CC_Word_Mode_Select    :1;
    };
}REG_SRD9_T;


typedef union _REG_SRDA_B    //HDTV1_576i/480i_Closed_Caption_(CC)_Start_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Start_7to0    :8;
    };
}REG_SRDA_B;


typedef union _REG_SRDA_T    //HDTV2_576i/480i_Closed_Caption_(CC)_Start_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Start_7to0    :8;
    };
}REG_SRDA_T;


typedef union _REG_SRDB_B    //HDTV1_576i/480i_Closed_Caption_(CC)/WSS_End_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_WSS_End_7to0    :8;
    };
}REG_SRDB_B;


typedef union _REG_SRDB_T    //HDTV2_576i/480i_Closed_Caption_(CC)/WSS_End_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_WSS_End_7to0    :8;
    };
}REG_SRDB_T;


typedef union _REG_SRDC_B    //HDTV1_576i/480i_Closed_Caption_(CC)_Field_1_Word_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Field1_Word0    :7;
        CBIOS_U8    HDTV_CC_Field1_Data_Write_Status    :1;
    };
}REG_SRDC_B;


typedef union _REG_SRDC_T    //HDTV2_576i/480i_Closed_Caption_(CC)_Field_1_Word_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Field1_Word0    :7;
        CBIOS_U8    HDTV_CC_Field1_Data_Write_Status    :1;
    };
}REG_SRDC_T;


typedef union _REG_SRDD_B    //HDTV1_576i/480i_Closed_Caption_(CC)_Field_1_Word_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Field1_Word1    :7;
        CBIOS_U8    Reserved    :1;
    };
}REG_SRDD_B;


typedef union _REG_SRDD_T    //HDTV2_576i/480i_Closed_Caption_(CC)_Field_1_Word_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Field1_Word1    :7;
        CBIOS_U8    Reserved    :1;
    };
}REG_SRDD_T;


typedef union _REG_SRE0_B    //HDTV1_576i/480i_Parameter_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm0_Start_HDTV_HSYNC_7to0    :8;
    };
}REG_SRE0_B;


typedef union _REG_SRE0_T    //HDTV2_576i/480i_Parameter_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm0_Start_HDTV_HSYNC_7to0    :8;
    };
}REG_SRE0_T;


typedef union _REG_SRE1_B    //HDTV1_576i/480i_Parameter_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm1_End_1st_Equalizing_7to0    :8;
    };
}REG_SRE1_B;


typedef union _REG_SRE1_T    //HDTV2_576i/480i_Parameter_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm1_End_1st_Equalizing_7to0    :8;
    };
}REG_SRE1_T;


typedef union _REG_SRE2_B    //HDTV1_576i/480i_Parameter_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm1_End_1st_Equalizing_7to0    :8;
    };
}REG_SRE2_B;


typedef union _REG_SRE2_T    //HDTV2_576i/480i_Parameter_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm1_End_1st_Equalizing_7to0    :8;
    };
}REG_SRE2_T;


typedef union _REG_SRE4_B    //HDTV1_HDE_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_HDE_7to0    :8;
    };
}REG_SRE4_B;


typedef union _REG_SRE4_T    //HDTV2_HDE_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_HDE_7to0    :8;
    };
}REG_SRE4_T;


typedef union _REG_SRE5_B    //HDTV1_HDE_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_HDE_10to8    :3;
        CBIOS_U8    Reserved    :5;
    };
}REG_SRE5_B;


typedef union _REG_SRE5_T    //HDTV2_HDE_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_HDE_10to8    :3;
        CBIOS_U8    Reserved    :5;
    };
}REG_SRE5_T;


typedef union _REG_SRE6_B    //HDTV1_576i/480i_Parameter_3_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm3_Start_HDTV_HSYNC_7to0    :8;
    };
}REG_SRE6_B;


typedef union _REG_SRE6_T    //HDTV2_576i/480i_Parameter_3_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm3_Start_HDTV_HSYNC_7to0    :8;
    };
}REG_SRE6_T;


typedef union _REG_SRE7_B    //HDTV1_576i/480i_Parameter_4_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm4_End_1st_Serration_Start_2nd_Equalization_7to0    :8;
    };
}REG_SRE7_B;


typedef union _REG_SRE7_T    //HDTV2_576i/480i_Parameter_4_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm4_End_1st_Serration_Start_2nd_Equalization_7to0    :8;
    };
}REG_SRE7_T;


typedef union _REG_SRE8_B    //HDTV1_576i/480i_Parameter_5_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm5_End_2nd_Equalization_7to0    :8;
    };
}REG_SRE8_B;


typedef union _REG_SRE8_T    //HDTV2_576i/480i_Parameter_5_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm5_End_2nd_Equalization_7to0    :8;
    };
}REG_SRE8_T;


typedef union _REG_SRE9_B    //HDTV1_576i/480i_Parameter_6_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm6_End_2nd_Equalization_7to0    :8;
    };
}REG_SRE9_B;


typedef union _REG_SRE9_T    //HDTV2_576i/480i_Parameter_6_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm6_End_2nd_Equalization_7to0    :8;
    };
}REG_SRE9_T;


typedef union _REG_SREB_B    //HDTV1(576i/480i)_Parameter_Overflow_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Pr_channel_delay    :2;
        CBIOS_U8    Pb_channel_delay    :2;
        CBIOS_U8    HDTV_Parm1_End_1st_Equalizing_10to8    :3;
        CBIOS_U8    Reserved    :1;
    };
}REG_SREB_B;


typedef union _REG_SREB_T    //HDTV2(576i/480i)_Parameter_Overflow_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Pr_channel_delay    :2;
        CBIOS_U8    Pb_channel_delay    :2;
        CBIOS_U8    HDTV_Parm1_End_1st_Equalizing_10to8    :3;
        CBIOS_U8    Reserved    :1;
    };
}REG_SREB_T;


typedef union _REG_SREC_B    //HDTV1(576i/480i)_Parameter_Overflow_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm2_End_TV_HSYNC_10to8    :3;
        CBIOS_U8    Y_channel_delay    :2;
        CBIOS_U8    Reserved    :3;
    };
}REG_SREC_B;


typedef union _REG_SREC_T    //HDTV2(576i/480i)_Parameter_Overflow_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm2_End_TV_HSYNC_10to8    :3;
        CBIOS_U8    Y_channel_delay    :2;
        CBIOS_U8    Reserved    :3;
    };
}REG_SREC_T;


typedef union _REG_SREE_B    //HDTV1(576i/480i)_Parameter_Overflow_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm3_Start_First_Serration_9to8    :2;
        CBIOS_U8    HDTV_Parm4_End_1st_Serration_Start_2nd_Equalization_9to8    :2;
        CBIOS_U8    HDTV_Parm5_End_2nd_Equalization_9to8    :2;
        CBIOS_U8    Reserved    :2;
    };
}REG_SREE_B;


typedef union _REG_SREE_T    //HDTV2(576i/480i)_Parameter_Overflow_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm3_Start_First_Serration_9to8    :2;
        CBIOS_U8    HDTV_Parm4_End_1st_Serration_Start_2nd_Equalization_9to8    :2;
        CBIOS_U8    HDTV_Parm5_End_2nd_Equalization_9to8    :2;
        CBIOS_U8    Reserved    :2;
    };
}REG_SREE_T;


typedef union _REG_SREF_B    //HDTV1(576i/480i)_Parameter_Overflow_3_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm6_Start_2nd_Serration_10to8    :3;
        CBIOS_U8    Reserved    :4;
        CBIOS_U8    VBI_Enable    :1;
    };
}REG_SREF_B;


typedef union _REG_SREF_T    //HDTV2(576i/480i)_Parameter_Overflow_3_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_Parm6_Start_2nd_Serration_10to8    :3;
        CBIOS_U8    Reserved    :4;
        CBIOS_U8    VBI_Enable    :1;
    };
}REG_SREF_T;


typedef union _REG_SRF0_B    //HDTV1_Wide_Screen_Signal_(WSS)_Word_or_Packet_B_Data_Byte_1Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    WSS_Word0_or_B_Data_Byte_2_or_WSS_Word_13to8    :8;
    };
}REG_SRF0_B;


typedef union _REG_SRF0_T    //HDTV2_Wide_Screen_Signal_(WSS)_Word_or_Packet_B_Data_Byte_1Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    WSS_Word0_or_B_Data_Byte_2_or_WSS_Word_13to8    :8;
    };
}REG_SRF0_T;


typedef union _REG_SRF1_B    //HDTV1_Wide_Screen_Signal_(WSS)_Word_or_Packet_B_Data_Byte_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    WSS_Word1_or_B_Data_Byte_2_or_WSS_Word_13to8    :8;
    };
}REG_SRF1_B;


typedef union _REG_SRF1_T    //HDTV2_Wide_Screen_Signal_(WSS)_Word_or_Packet_B_Data_Byte_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    WSS_Word1_or_B_Data_Byte_2_or_WSS_Word_13to8    :8;
    };
}REG_SRF1_T;


typedef union _REG_SRF2_B    //HDTV1_WSS_or_Packet_B_Data_Byte_3_or_Packet_A_Data_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    WSS_Word2_or_Packet_A_Data_or_Packet_B_Data_Byte    :8;
    };
}REG_SRF2_B;


typedef union _REG_SRF2_T    //HDTV2_WSS_or_Packet_B_Data_Byte_3_or_Packet_A_Data_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    WSS_Word2_or_Packet_A_Data_or_Packet_B_Data_Byte    :8;
    };
}REG_SRF2_T;


typedef union _REG_SRF3_B    //HDTV1(576i/480i)_Closed_Caption_(CC)_Field0_Word0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Field0_Word0    :7;
        CBIOS_U8    HDTV_CC_Field0_Data_Write_Status    :1;
    };
}REG_SRF3_B;


typedef union _REG_SRF3_T    //HDTV2(576i/480i)_Closed_Caption_(CC)_Field0_Word0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Field0_Word0    :7;
        CBIOS_U8    HDTV_CC_Field0_Data_Write_Status    :1;
    };
}REG_SRF3_T;


typedef union _REG_SRF4_B    //HDTV1(576i/480i)_Closed_Caption_(CC)_Field0_Word1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Field0_Word1_or_HDTV_Packet_A_CRC_Or_Packet_B_Data_Byte_14    :7;
        CBIOS_U8    Reserved_OR__Packet_B_Data_Byte_14    :1;
    };
}REG_SRF4_B;


typedef union _REG_SRF4_T    //HDTV2(576i/480i)_Closed_Caption_(CC)_Field0_Word1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CC_Field0_Word1_or_HDTV_Packet_A_CRC_Or_Packet_B_Data_Byte_14    :7;
        CBIOS_U8    Reserved_OR__Packet_B_Data_Byte_14    :1;
    };
}REG_SRF4_T;


typedef union _REG_SRF5_B    //HDTV1_Wide_Screen_Signal_(WSS)_Clock_Ratio_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    WSS_Clock_Ratio_5to0    :6;
        CBIOS_U8    WSS_Mode_Enable    :2;
    };
}REG_SRF5_B;


typedef union _REG_SRF5_T    //HDTV2_Wide_Screen_Signal_(WSS)_Clock_Ratio_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    WSS_Clock_Ratio_5to0    :6;
        CBIOS_U8    WSS_Mode_Enable    :2;
    };
}REG_SRF5_T;


typedef union _REG_SRF6_B    //HDTV1_Wide_Screen_Signal_(WSS)/CGMS-A_Line_and_Field_Select_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    WSS_CGMS_A_Line_Select    :6;
        CBIOS_U8    WSS_Field_Select    :2;
    };
}REG_SRF6_B;


typedef union _REG_SRF6_T    //HDTV2_Wide_Screen_Signal_(WSS)/CGMS-A_Line_and_Field_Select_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    WSS_CGMS_A_Line_Select    :6;
        CBIOS_U8    WSS_Field_Select    :2;
    };
}REG_SRF6_T;


typedef union _REG_SRF7_B    //HDTV1_CGMSA_Header_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CGMSA_Header    :6;
        CBIOS_U8    Reserved    :2;
    };
}REG_SRF7_B;


typedef union _REG_SRF7_T    //HDTV2_CGMSA_Header_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_CGMSA_Header    :6;
        CBIOS_U8    Reserved    :2;
    };
}REG_SRF7_T;


typedef union _REG_SRF8_B    //HDTV1_WSS/CGMSA_Start_LSB_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :3;
        CBIOS_U8    HDTV_WSS_CGMSA_Start_4to0    :5;
    };
}REG_SRF8_B;


typedef union _REG_SRF8_T    //HDTV2_WSS/CGMSA_Start_LSB_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :3;
        CBIOS_U8    HDTV_WSS_CGMSA_Start_4to0    :5;
    };
}REG_SRF8_T;


typedef union _REG_SRF9_B    //HDTV1_WSS/CGMSA_Enable_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_WSS_CGMSA_Start_8to5    :4;
        CBIOS_U8    HDTV_WSS_Sermode    :1;
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    HDTV_CGMSA_Mode    :2;
    };
}REG_SRF9_B;


typedef union _REG_SRF9_T    //HDTV2_WSS/CGMSA_Enable_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDTV_WSS_CGMSA_Start_8to5    :4;
        CBIOS_U8    HDTV_WSS_Sermode    :1;
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    HDTV_CGMSA_Mode    :2;
    };
}REG_SRF9_T;


