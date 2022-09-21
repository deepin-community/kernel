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


typedef union _REG_CR22_Pair    //CRT_Test_DAC_Data_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Test_Reset    :1;
        CBIOS_U8    RESERVED_0    :3;
        CBIOS_U8    Test_Sync    :1;
        CBIOS_U8    RESERVED_1    :2;
        CBIOS_U8    Test_DAC_Data    :1;
    };
}REG_CR22_Pair;


typedef union _REG_CR23_Pair    //Test_Data_High_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Test_Data_10to3    :8;
    };
}REG_CR23_Pair;


typedef union _REG_CR26_Pair    //Test_Data_Load_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Load_Test_Data    :8;
    };
}REG_CR26_Pair;


typedef union _REG_CR29_Pair    //Test_Data_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Test_Data_2to0    :3;
        CBIOS_U8    Test_Data_bit11    :1;
        CBIOS_U8    RESERVED    :4;
    };
}REG_CR29_Pair;


typedef union _REG_CR31_Pair    //Memory_Configuration_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    CPU_Base_Address_Offset_Bits_Enable    :1;
        CBIOS_U8    Two_Page_Screen_Enable    :1;
        CBIOS_U8    VGA_16bit_Bus_Width    :1;
        CBIOS_U8    Enhanced_Mode_Map    :1;
        CBIOS_U8    Display_Start_Address_19to18    :2;
        CBIOS_U8    High_Speed_Text    :1;
        CBIOS_U8    RESERVED    :1;
    };
}REG_CR31_Pair;


typedef union _REG_CR32_Pair    //Backward_Compatibility_1_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS_Read_Length    :2;
        CBIOS_U8    Force_Char_Clock_High    :1;
        CBIOS_U8    RESERVED_0    :1;
        CBIOS_U8    vretrace_inter_EN    :1;
        CBIOS_U8    MISC_bit_7_EN    :1;
        CBIOS_U8    VGA_Memory_Wrap    :1;
        CBIOS_U8    RESERVED_1    :1;
    };
}REG_CR32_Pair;


typedef union _REG_CR33_Pair    //Backward_Compatibility_2_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Display_Mode_Inactive    :1;
        CBIOS_U8    VDE_Protection_Disable    :1;
        CBIOS_U8    Vertical_Sync_Active    :1;
        CBIOS_U8    VCLK_Is_Inverted_DCLK    :1;
        CBIOS_U8    Lock_DAC_Writes    :1;
        CBIOS_U8    BLANK_Active_Time_Select    :1;
        CBIOS_U8    Palette_Border_Color_Lock    :1;
        CBIOS_U8    Flicker_Filter_Odd_Even_Status    :1;
    };
}REG_CR33_Pair;


typedef union _REG_CR34_Pair    //Backward_Compatibility_3_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PCI_DAC_Snoop_Handling    :1;
        CBIOS_U8    PCI_Master_Abort_Handling    :1;
        CBIOS_U8    PCI_Retry_Handling    :1;
        CBIOS_U8    Vertical_Blank_Active    :1;
        CBIOS_U8    Reserved_bit4    :1;
        CBIOS_U8    SR01_0_Lock    :1;
        CBIOS_U8    VGA_Register_Lock    :1;
        CBIOS_U8    VGA_Clock_Frequency_Lock    :1;
    };
}REG_CR34_Pair;


typedef union _REG_CR35_Pair    //CRT_Register_Lock_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    For_VGA_mem_offset    :4;
        CBIOS_U8    Vertical_Timing_Registers_Lock    :1;
        CBIOS_U8    Horizontal_Timing_Registers_Lock    :1;
        CBIOS_U8    CR01_3C2h_6_Lock    :1;
        CBIOS_U8    CR12_3C2h_7_Lock    :1;
    };
}REG_CR35_Pair;


typedef union _REG_CR36    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :8;
    };
}REG_CR36;


typedef union _REG_CR37    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :8;
    };
}REG_CR37;


typedef union _REG_CR38    //Register_CR2D-3F_Lock_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    CRTC_Register_Lock_1    :8;
    };
}REG_CR38;


typedef union _REG_CR39    //Register_CR40-FF_Lock_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    CRTC_Register_Lock_2    :8;
    };
}REG_CR39;


typedef union _REG_CR3A_Pair    //Miscellaneous_1_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    C1_Vertical_Counter_2to0_Read    :3;
        CBIOS_U8    Top_of_Memory_Access    :1;
        CBIOS_U8    Enhanced_Mode_Enable    :1;
        CBIOS_U8    High_Speed_Text    :1;
        CBIOS_U8    C1_Vertical_Counter_bit_11_Read    :1;
        CBIOS_U8    RESERVED    :1;
    };
}REG_CR3A_Pair;


typedef union _REG_CR3B_Pair    //Start_Display_FIFO_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Start_Display_FIFO_Fetch_7to0    :8;
    };
}REG_CR3B_Pair;


typedef union _REG_CR3C_Pair    //Interlace_Retrace_Start_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Interlace_Retrace_Start_Position    :8;
    };
}REG_CR3C_Pair;


typedef union _REG_CR3E_Pair    //Vertical_Counter_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Vertical_Counter_Bits_10to3    :8;
    };
}REG_CR3E_Pair;


typedef union _REG_CR3F    //LED_panel_Power_ON/OFF_Sequence_for_PWM_signal
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Timer_step_setting    :7;
        CBIOS_U8    Timer_status_    :1;
    };
}REG_CR3F;


typedef union _REG_CR41    //Extended_BIOS_Flag_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    BIOS_Flag_1    :8;
    };
}REG_CR41;


typedef union _REG_CR42_Pair    //Mode_Control_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    CPU_Base_Address_11to8    :4;
        CBIOS_U8    Reduced_Blanking_Enable    :1;
        CBIOS_U8    Interlaced_Mode    :1;
        CBIOS_U8    Controller2_HSYNC_VSYNC    :2;
    };
}REG_CR42_Pair;


typedef union _REG_CR43_Pair    //Extended_Mode_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    RESERVED_0    :1;
        CBIOS_U8    Controller3_HSYNC_VSYNC    :2;
        CBIOS_U8    Character_Blink    :1;
        CBIOS_U8    PS1_VR_FRAME    :1;
        CBIOS_U8    Cursor_Blink    :2;
        CBIOS_U8    PS2_VR_FRAME    :1;
    };
}REG_CR43_Pair;


typedef union _REG_CR44_Pair    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    _3D_Video_Mode    :4;
        CBIOS_U8    PS_FIFO_share    :1;
        CBIOS_U8    PS_X_Rotation_Enable    :1;
        CBIOS_U8    PS_Y_Rotation_Enable    :1;
        CBIOS_U8    enable_page_flip_every_2_frame_for_3D_video    :1;
    };
}REG_CR44_Pair;


typedef union _REG_CR45_Pair    //Hardware_Graphics_Cursor_Mode_Register__Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Enable    :1;
        CBIOS_U8    HW_Cursor_Position    :1;
        CBIOS_U8    HW_Cursor_Type    :2;
        CBIOS_U8    HW_Cursor_X_Rotation    :1;
        CBIOS_U8    HW_Cursor_Y_Rotation    :1;
        CBIOS_U8    HW_Cursor_Size    :1;
        CBIOS_U8    Reserved    :1;
    };
}REG_CR45_Pair;


typedef union _REG_CR46_Pair    //Hardware_Graphics_Cursor_Origin-X_High_Register__Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_X_Origin_12to8    :6;
        CBIOS_U8    RESERVED    :2;
    };
}REG_CR46_Pair;


typedef union _REG_CR47_Pair    //Hardware_Graphics_Cursor_Origin-X_Low_Register__Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_X_Origin_7to0    :8;
    };
}REG_CR47_Pair;


typedef union _REG_CR48_Pair    //Hardware_Graphics_Cursor_Origin-Y_HIgh_Register__Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Y_Origin_11to8    :5;
        CBIOS_U8    RESERVED    :3;
    };
}REG_CR48_Pair;


typedef union _REG_CR49_Pair    //Hardware_Graphics_Cursor_Origin-Y_Low_Register__Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Y_Origin_7to0    :8;
    };
}REG_CR49_Pair;


typedef union _REG_CR4A_Pair    //Hardware_Graphics_Cursor_Foreground_Color_Stack_Register__Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Color_Foreground_Stack    :8;
    };
}REG_CR4A_Pair;


typedef union _REG_CR4B_Pair    //Hardware_Graphics_Cursor_Background_Color_Stack_Register__Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Color_Background_Stack    :8;
    };
}REG_CR4B_Pair;


typedef union _REG_CR4C_Pair    //Hardware_Graphics_Cursor_Start_Address_15to8_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Start_Address_15to8    :8;
    };
}REG_CR4C_Pair;


typedef union _REG_CR4D_Pair    //Hardware_Graphics_Cursor_Start_Address_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Start_Address_7to0    :8;
    };
}REG_CR4D_Pair;


typedef union _REG_CR4E_Pair    //Hardware_Graphics_Cursor_Display_Start_X_Pixel_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Display_Start_X    :7;
        CBIOS_U8    RESERVED    :1;
    };
}REG_CR4E_Pair;


typedef union _REG_CR4F_Pair    //Hardware_Graphics_Cursor_Display_Start_Y_Pixel_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Display_Start_Y    :7;
        CBIOS_U8    RESERVED    :1;
    };
}REG_CR4F_Pair;


typedef union _REG_CR50    //Extended_System_Control_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Screen_Width_bit2    :1;
        CBIOS_U8    RESERVED    :3;
        CBIOS_U8    Pixel_Length_Select    :2;
        CBIOS_U8    Screen_Width_1to0    :2;
    };
}REG_CR50;


typedef union _REG_CR51_Pair    //Extended_System_Control_2_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Controller2_VSYNC_Lock    :1;
        CBIOS_U8    Controller2_HSYNC_Lock    :1;
        CBIOS_U8    Logical_Screen_Width_13to8    :6;
    };
}REG_CR51_Pair;


typedef union _REG_CR52    //Extended_BIOS_Flag_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    BIOS_Flag_2    :8;
    };
}REG_CR52;


typedef union _REG_CR53    //Extended_Memory_Control_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    output_to_BIU    :7;
        CBIOS_U8    VGA_Access_Disable    :1;
    };
}REG_CR53;


typedef union _REG_CR54_Pair    //Hardware_Graphics_Cursor_Start_Address_3_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Start_Address_23to16    :8;
    };
}REG_CR54_Pair;


typedef union _REG_CR55    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SS3_Load_Control    :1;
        CBIOS_U8    PS3_Load_Control    :1;
        CBIOS_U8    Cursor_1_Load_Control    :1;
        CBIOS_U8    Cursor_2_Load_Control    :1;
        CBIOS_U8    Cursor_3_Load_Control    :1;
        CBIOS_U8    Reserved    :3;
    };
}REG_CR55;


typedef union _REG_CR56    //NB_Product_SKU_and_HDCP_Allow_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Pdmod0    :1;
        CBIOS_U8    Pdmod1    :1;
        CBIOS_U8    Hdcp_Bopt    :1;
        CBIOS_U8    RESERVED    :5;
    };
}REG_CR56;


typedef union _REG_CR57_Pair    //Hardware_Graphics_Cursor_Start_Address_4_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Start_Address_27to24    :3;
        CBIOS_U8    HW_Cursor_Right_Frame_Start_Address__27to24    :3;
        CBIOS_U8    RESERVED    :2;
    };
}REG_CR57_Pair;


typedef union _REG_CR58_Pair    //Adjust_Horizontal_Display_Enable_Control
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Remainder_Of_Pixel_for_One_Line_of_Active_Display_Divided_by_8    :3;
        CBIOS_U8    Enable_HDE_adjusting_function    :1;
        CBIOS_U8    output_to_BIU    :4;
    };
}REG_CR58_Pair;


typedef union _REG_CR59_Pair    //Horizontal_Display_Shift_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDES_7_to_0    :8;
    };
}REG_CR59_Pair;


typedef union _REG_CR5A_Pair    //Vertical_Display_Shift_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    VDES_7to0    :8;
    };
}REG_CR5A_Pair;


typedef union _REG_CR5B_Pair    //Display_Shift_Overflow_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HDES_8    :1;
        CBIOS_U8    RESERVED    :2;
        CBIOS_U8    Enable_Shift_On    :1;
        CBIOS_U8    VDES    :4;
    };
}REG_CR5B_Pair;


typedef union _REG_CR5C    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :8;
    };
}REG_CR5C;


typedef union _REG_CR5D_Pair    //Extended_Horizontal_Overflow_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Horizontal_total_bit_8    :1;
        CBIOS_U8    Horizontal_Display_End_bit_8    :1;
        CBIOS_U8    Start_Horizontal_Blank_bit_8    :1;
        CBIOS_U8    End_Horizontal_Blank_bit_6    :1;
        CBIOS_U8    Start_Horizontal_Sync_Position_bit_8    :1;
        CBIOS_U8    End_Horizontal_Sync_Period_bit_5    :1;
        CBIOS_U8    Start_FIFO_Fetch_bit_8    :1;
        CBIOS_U8    Start_TV_Hsync_Position_8    :1;
    };
}REG_CR5D_Pair;


typedef union _REG_CR5E_Pair    //Extended_Vertical_Overflow_Register__Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Vdisplay_end_bit_10    :2;
        CBIOS_U8    Start_Vertical_Blank_11to10    :2;
        CBIOS_U8    Vertical_Retrace_Start_11to10    :2;
        CBIOS_U8    Line_Compare_11to10    :2;
    };
}REG_CR5E_Pair;


typedef union _REG_CR5F_Pair    //Extended_Horizontal_Overflow_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Horizontal_total_bit_9    :1;
        CBIOS_U8    Horizontal_Display_End_bit_9    :1;
        CBIOS_U8    Start_Horizontal_Blank_bit_9    :1;
        CBIOS_U8    Start_TV_Horizontal_Sync_Position_bit_9    :1;
        CBIOS_U8    Start_FIFO_Fetch_bit_9    :1;
        CBIOS_U8    Start_TV_Hsync_Position_bit_9    :1;
        CBIOS_U8    RESERVED    :2;
    };
}REG_CR5F_Pair;


typedef union _REG_CR60_Pair    //Hardware_Graphics_Cursor_Right_Frame_Start_Address_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Right_Frame_Start_Address_7to0    :8;
    };
}REG_CR60_Pair;


typedef union _REG_CR61_Pair    //Hardware_Graphics_Cursor_Right_Frame_Start_Address_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Right_Frame_Start__15to8    :8;
    };
}REG_CR61_Pair;


typedef union _REG_CR62    //Hardware_Graphics_Cursor_Right_Frame_Start_Address_Low_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HW_Cursor_Right_Frame_Start__23to16    :8;
    };
}REG_CR62;


typedef union _REG_CR63_Pair    //Start_FIFO_Fetch_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HSYNC_Timing    :1;
        CBIOS_U8    PS_Request_Disable_Pos    :1;
        CBIOS_U8    Start_FIFO_Fetch_Pos    :1;
        CBIOS_U8    Start_FIFO_Fetch    :2;
        CBIOS_U8    RESERVED    :1;
        CBIOS_U8    Vertical_Display_End_bit_11    :2;
    };
}REG_CR63_Pair;


typedef union _REG_CR64_Pair    //Display_Start_Address_27:24_Register__Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Display_Start_Address_29to24    :6;
        CBIOS_U8    PS_shadow_bit_select    :1;
        CBIOS_U8    RESERVED    :1;
    };
}REG_CR64_Pair;


typedef union _REG_CR65_Pair    //Extended_Miscellaneous_Control_0_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS2_Load_Control    :1;
        CBIOS_U8    PS1_Load_Control    :1;
        CBIOS_U8    PS_LUT_before_Blender    :1;
        CBIOS_U8    BLANK_Delay_in_DCLK    :3;
        CBIOS_U8    SS2_Load_Control    :1;
        CBIOS_U8    SS1_Load_Control    :1;
    };
}REG_CR65_Pair;


typedef union _REG_CR66_Pair    //Extended_Miscellaneous_Control_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Enhanced_Mode    :1;
        CBIOS_U8    RESERVED_0    :1;
        CBIOS_U8    Enhanced_Pixel_Length    :1;
        CBIOS_U8    RESERVED_1    :5;
    };
}REG_CR66_Pair;


typedef union _REG_CR67_Pair    //Extended_Miscellaneous_Control_2_Register__Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SS_Blender_Disable    :1;
        CBIOS_U8    Primary_Stream_Disable    :1;
        CBIOS_U8    Secondary_Stream_Enable    :1;
        CBIOS_U8    Primary_Streams_Control    :1;
        CBIOS_U8    DAC1_Color_Mode    :3;
        CBIOS_U8    reserved    :1;
    };
}REG_CR67_Pair;


typedef union _REG_CR68    //Configuration_3_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    EOCLK_Source    :1;
        CBIOS_U8    DCLK1_CLK_Source    :1;
        CBIOS_U8    DCLK2_CLK_Source    :1;
        CBIOS_U8    EICLK_Source    :1;
        CBIOS_U8    reserved    :3;
        CBIOS_U8    HDCP_eFuse_Blow_Enable    :1;
    };
}REG_CR68;


typedef union _REG_CR69_Pair    //Display_Start_Address_23:16_Register__Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Display_Start_Address_23to16    :8;
    };
}REG_CR69_Pair;


typedef union _REG_CR6A    //CPU_Base_Address_7:0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    CPU_Base_Address_7to0    :8;
    };
}REG_CR6A;


typedef union _REG_CR6B    //Extended_BIOS_Flag_3_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    BIOS_Flag_3    :8;
    };
}REG_CR6B;


typedef union _REG_CR6C    //Extended_BIOS_Flag_4_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    BIOS_Flag_4    :8;
    };
}REG_CR6C;


typedef union _REG_CR6D    //Extended_BIOS_Flag_5_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    BIOS_Flag_5    :8;
    };
}REG_CR6D;


typedef union _REG_CR6E    //DAC_Signature_Test_Data_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DAC_SIG_TEST_DATA_STATUS    :8;
    };
}REG_CR6E;


typedef union _REG_CR6F    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :8;
    };
}REG_CR6F;


typedef union _REG_CR70_Pair    //Start_TV_Horizontal_Sync_Position_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Start_TV_Hsync_Position_7_to_0    :8;
    };
}REG_CR70_Pair;


typedef union _REG_CR71_Pair    //Dual_Image_Control_Register__Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    RESERVED    :1;
        CBIOS_U8    Screen_Off_Control_Select    :1;
        CBIOS_U8    DLYSEL    :1;
        CBIOS_U8    SENSEL    :2;
        CBIOS_U8    Screen_Off    :1;
        CBIOS_U8    SENWIDTH    :2;
    };
}REG_CR71_Pair;


typedef union _REG_CR72_Pair    //Adjusting_Timing_Control_1
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Adjust_H_Bottom_Right_Border_H_Front_Porch_period    :3;
        CBIOS_U8    Adjust_HSYNC_period    :3;
        CBIOS_U8    RESERVED    :2;
    };
}REG_CR72_Pair;


typedef union _REG_CR73_Pair    //Adjusting_Timing_Control_2
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Adjust_H_Back_Porch_H_Top_Left_Border_period    :3;
        CBIOS_U8    RESERVED    :5;
    };
}REG_CR73_Pair;


typedef union _REG_CR74    //high_bit__for_timing_control_register1_pair
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
}REG_CR74;


typedef union _REG_CR75    //high_bit__for_timing_control_register2_pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Start_FIFO_Fetch_bit_10    :1;
        CBIOS_U8    HDES_9    :1;
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    Start_TV_Hsync_Position_bit_10    :1;
        CBIOS_U8    reserved    :4;
    };
}REG_CR75;


typedef union _REG_CR76    //high_bit__for_timing_control_register3_pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Interlace_mode_enable    :1;
        CBIOS_U8    s_vblnk_end_off    :3;
        CBIOS_U8    s_vsync_off    :3;
        CBIOS_U8    s_totol_off_bit2    :1;
    };
}REG_CR76;


typedef union _REG_CR77    //high_bit__for_timing_control_register4_pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    s_totol_off_bit1_0    :2;
        CBIOS_U8    reserved_0    :1;
        CBIOS_U8    double_FrameBuffer_mode    :1;
        CBIOS_U8    enable_4_frame_page_flip_for_FRC    :1;
        CBIOS_U8    use_L_R_or_ODD_EVEN_to_do_2_frame_page_flip    :1;
        CBIOS_U8    reserved_1    :2;
    };
}REG_CR77;


typedef union _REG_CR79    //high_bit__for_timing_control_register5_pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Vertical_Retrace_Start_bit_13    :1;
        CBIOS_U8    reserved    :1;
        CBIOS_U8    Vertical_Total_bit_13    :1;
        CBIOS_U8    Vertical_Display_End_bit_13    :1;
        CBIOS_U8    Start_Vertical_Blank_bit_13    :1;
        CBIOS_U8    End_Vertical_Blank_bit9    :1;
        CBIOS_U8    Vertical_Retrace_End__bit6    :1;
        CBIOS_U8    VDES_bit13    :1;
    };
}REG_CR79;


typedef union _REG_CR7A    //high_bit__for_timing_control_register6_pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Line_Compare_bit_13    :1;
        CBIOS_U8    reserved    :7;
    };
}REG_CR7A;


typedef union _REG_CR7B    //pt_request_control_pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS1_pt_request_conotrl    :1;
        CBIOS_U8    RESERVED    :7;
    };
}REG_CR7B;


typedef union _REG_CR85    //
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DIU_MEM_IDLE_mode    :2;
        CBIOS_U8    DIU_MEM_IDLE    :1;
        CBIOS_U8    HDA_PWSEN    :1;
        CBIOS_U8    IGA3_PWSEN    :1;
        CBIOS_U8    TS_PWSEN    :1;
        CBIOS_U8    IGA1_PWSEN    :1;
        CBIOS_U8    IGA2_PWSEN    :1;
    };
}REG_CR85;


typedef union _REG_CR86    //reerved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :8;
    };
}REG_CR86;


typedef union _REG_CR87    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :8;
    };
}REG_CR87;


typedef union _REG_CR88    //Primary_Stream_1_Timeout_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS1_Timeout    :8;
    };
}REG_CR88;


typedef union _REG_CR89    //Secondary_Stream_1_Timeout_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :6;
        CBIOS_U8    Memory_Bank_Info    :1;
        CBIOS_U8    Display_Requests_to_MIU    :1;
    };
}REG_CR89;


typedef union _REG_CR8D    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :8;
    };
}REG_CR8D;


typedef union _REG_CR90_Pair    //Primary_Stream_FIFO_Fetch_Control_1_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS_L1_10to8    :3;
        CBIOS_U8    RESERVED_0    :2;
        CBIOS_U8    EOF_Signal_to_BIU_Select    :1;
        CBIOS_U8    RESERVED_1    :1;
        CBIOS_U8    Enable_PS_L1    :1;
    };
}REG_CR90_Pair;


typedef union _REG_CR91_Pair    //Primary_Stream_FIFO_Fetch_Control_2_Register_Pair
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS_L1_7to0    :8;
    };
}REG_CR91_Pair;


typedef union _REG_CR92    //TIMEOUT_CONTROL
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS3_Timeout    :1;
        CBIOS_U8    SS3_Timeout    :1;
        CBIOS_U8    AUDIO_Timeout    :1;
        CBIOS_U8    TVW_Timeout    :1;
        CBIOS_U8    RESERVED_7to6    :4;
    };
}REG_CR92;


typedef union _REG_CR93    //
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS3_request_timeout_reg_bit_7_0    :8;
    };
}REG_CR93;


typedef union _REG_CR94    //
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :7;
        CBIOS_U8    Disable_PS3_odd_or_even_line_check    :1;
    };
}REG_CR94;


typedef union _REG_CR95    //
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SS3_request_timeout_reg_bit_7_0    :8;
    };
}REG_CR95;


typedef union _REG_CR96    //
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PT_request_timeout_reg_bit_7_0    :8;
    };
}REG_CR96;


typedef union _REG_CR9A    //MDI_latency_check_control
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    mdi_read_latency_check_eanble    :1;
        CBIOS_U8    reset_latency_recorder_counter    :1;
        CBIOS_U8    ps_ss_requeset_method    :1;
        CBIOS_U8    reserved    :5;
    };
}REG_CR9A;


typedef union _REG_CRA0    //Serial_Port_1_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SPCLK1_Write    :1;
        CBIOS_U8    SPDAT1_Write    :1;
        CBIOS_U8    SPCLK1_Read    :1;
        CBIOS_U8    SPDAT1_Read    :1;
        CBIOS_U8    Serial_Port_1_Enable    :1;
        CBIOS_U8    Reserved    :2;
        CBIOS_U8    signature_reset    :1;
    };
}REG_CRA0;


typedef union _REG_CRA1    //LVDS_Test_Pattern_Sequence_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRA1;


typedef union _REG_CRA2    //LVDS_Test_Pattern_Sequence_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRA2;


typedef union _REG_CRA3    //LVDS_Test_Pattern_Sequence_3_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRA3;


typedef union _REG_CRA4    //LVDS_Test_Pattern_Sequence_4_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRA4;


typedef union _REG_CRA5    //pt_request_control0
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PT_enable    :1;
        CBIOS_U8    pt_requset_threshold_value    :5;
        CBIOS_U8    mdi_round_robbin_in_idle_st_control    :1;
        CBIOS_U8    reserved    :1;
    };
}REG_CRA5;


typedef union _REG_CRA7    //
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    tvw_timeout_control_bit_7_0    :8;
    };
}REG_CRA7;


typedef union _REG_CRA8    //
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    hdaudio_timeout_control_bit_7_0    :8;
    };
}REG_CRA8;


typedef union _REG_CRAA    //Serial_Port_2_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SPCLK2_Write    :1;
        CBIOS_U8    SPDAT2_Write    :1;
        CBIOS_U8    SPCLK2_Read    :1;
        CBIOS_U8    SPDAT2_Read    :1;
        CBIOS_U8    Serial_Port_2_Enable    :1;
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    HDCP1_I2C_port_selection    :2;
    };
}REG_CRAA;


typedef union _REG_CRAB    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SP3_LUT_Interpolation_Enable    :1;
        CBIOS_U8    SP3_LUT_Split    :1;
        CBIOS_U8    reserved_0    :1;
        CBIOS_U8    SP3_CRT_Gamma    :2;
        CBIOS_U8    reserved_1    :1;
        CBIOS_U8    SP3_TV_Gamma    :2;
    };
}REG_CRAB;


typedef union _REG_CRAC    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    CLUT3_Select :1;
        CBIOS_U8    Signature_Test_Data_Source  :1;
        CBIOS_U8    reserved    :3;
        CBIOS_U8    CLUT3_Configuration :2;
        CBIOS_U8    Controller3_DAC_Blank_Power_Down    :1;
    };
}REG_CRAC;


typedef union _REG_CRB0    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :8;
    };
}REG_CRB0;


typedef union _REG_CRB1    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRB1;


typedef union _REG_CRB4    //PLL_DCLK3_parameter_setting
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PLL_DCLK3_R    :2;
        CBIOS_U8    Reserved_7to2    :6;
    };
}REG_CRB4;


typedef union _REG_CRB5    //PLL_DCLK3_parameter_setting
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PLL_DCLK3_integer_M    :8;
    };
}REG_CRB5;


typedef union _REG_CRB6    //PLL_DCLK3_parameter_setting
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PLL_DCLK3_fractional_7to0_bits    :8;
    };
}REG_CRB6;


typedef union _REG_CRB7    //PLL_DCLK3_parameter_setting
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PLL_DCLK3_fractional_15to8_bits    :8;
    };
}REG_CRB7;


typedef union _REG_CRB8    //PLL_DCLK3_parameter_setting
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PLL_DCLK3_fractional_19to16_bits    :8;
    };
}REG_CRB8;


typedef union _REG_CRB9    //PLL_DCLK1_parameter_setting
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    VGA2_fractional_11to4_bits    :8;
    };
}REG_CRB9;


typedef union _REG_CRBA    //PLL_DCLK1_parameter_setting
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    VGA1_PLL_fractional_bits_3to0    :4;
        CBIOS_U8    VGA2_fractional_3to0_bits    :4;
    };
}REG_CRBA;


typedef union _REG_CRBB    //PLL_DCLK1_parameter_setting
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    VGA1_fractional_11to4_bits    :8;
    };
}REG_CRBB;


typedef union _REG_CRBC    //Primary_Stream_2_Timeout_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS2_Timeout    :8;
    };
}REG_CRBC;


typedef union _REG_CRBD    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reseerved    :8;
    };
}REG_CRBD;


typedef union _REG_CRBE    //Secondary_Stream_2_Timeout_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SS2_Timeout_7_0    :8;
    };
}REG_CRBE;


typedef union _REG_CRC0_B    //Pad_Pull_Up/Down_Driving_Setting_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DISP1_DQ_PS    :3;
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    DISP1_DQ_NS    :3;
        CBIOS_U8    Reserved_1    :1;
    };
}REG_CRC0_B;


typedef union _REG_CRC1_B    //Pad_Pull_Up/Down_Driving_Setting_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DISP1CLK_DQ_PS    :3;
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    DISP1CLK_DQ_NS    :3;
        CBIOS_U8    Reserved_1    :1;
    };
}REG_CRC1_B;


typedef union _REG_CRC2_B    //Frame_Buffer_Base_Address_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    FB_Base_Addr_Source    :1;
        CBIOS_U8    Reserved    :3;
        CBIOS_U8    FB_Base_Addr_A23_A20    :4;
    };
}REG_CRC2_B;


typedef union _REG_CRC3_B    //Frame_Buffer_Base_Address_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    FB_Base_Addr_A31_A24    :8;
    };
}REG_CRC3_B;


typedef union _REG_CRC4_B    //Pad_Pull_Up/Down_Driving_Setting_5_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    GP_DQ_PS    :3;
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    GP_DQ_NS    :3;
        CBIOS_U8    Reserved_1    :1;
    };
}REG_CRC4_B;


typedef union _REG_CRC5_B    //Serial_Port_3_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SPCLK3_Write    :1;
        CBIOS_U8    SPDAT3_Write    :1;
        CBIOS_U8    SPCLK3_Read    :1;
        CBIOS_U8    SPDAT3_Read    :1;
        CBIOS_U8    Serial_Port_3_Enable    :1;
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    HDCP4_I2C_port_selection    :2;
    };
}REG_CRC5_B;


typedef union _REG_CRC6_B    //Serial_Port_4_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SPCLK4    :1;
        CBIOS_U8    SPDAT4_Data_Write    :1;
        CBIOS_U8    SPCLK4_Read    :1;
        CBIOS_U8    SPDAT4_Read    :1;
        CBIOS_U8    Serial_Port_4_Enable    :1;
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    HDCP2_I2C_port_selection    :2;
    };
}REG_CRC6_B;


typedef union _REG_CRCE    //LVDS_TESTMODE[16:9]_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    LOCKSEL    :1;
        CBIOS_U8    WINSEL    :1;
        CBIOS_U8    Testmode_4to3    :2;
        CBIOS_U8    BYPASS    :1;
        CBIOS_U8    Testmode_7to5    :3;
    };
}REG_CRCE;


typedef union _REG_CRCF    //PS_odd/even_line_check_control
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :4;
        CBIOS_U8    Hreq_Force    :1;
        CBIOS_U8    Reserved_bit5    :1;
        CBIOS_U8    Disable_PS1_odd_or_even_line_check    :1;
        CBIOS_U8    Disable_PS2_odd_or_even_line_check    :1;
    };
}REG_CRCF;


typedef union _REG_CRD0_B    //PLL_Parameter_and_DCLK2_Charge_Pump_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK1_fractional_function_en    :1;
        CBIOS_U8    DCLK1_Band    :1;
        CBIOS_U8    DCLK1_PLL_clk_output_enable    :1;
        CBIOS_U8    DCLK1_CP_Current    :3;
        CBIOS_U8    DCLK1_DIV_SEL    :2;
    };
}REG_CRD0_B;


typedef union _REG_CRD1_B    //PLL_DCLK2_Parameter_and_Charge_Pump_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK2_fractional_function_en    :1;
        CBIOS_U8    DCLK2_Band    :1;
        CBIOS_U8    DCLK2_PLL_CLK_output_enable    :1;
        CBIOS_U8    DCLK2_CP_Current    :3;
        CBIOS_U8    DCLK2_DIV_SEL    :2;
    };
}REG_CRD1_B;


typedef union _REG_CRD2_B    //PLL_DCLK3_Parameter_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK3_fractional_function_en    :1;
        CBIOS_U8    DCLK3_Band    :1;
        CBIOS_U8    DCLK3_PLL_CLK_output_enable    :1;
        CBIOS_U8    DCLK3_CP_Current    :3;
        CBIOS_U8    DCLK3_DIV_SEL    :2;
    };
}REG_CRD2_B;


typedef union _REG_CRD6    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRD6;


typedef union _REG_CRD7    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRD7;


typedef union _REG_CRD8    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRD8;


typedef union _REG_CRD9    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRD9;


typedef union _REG_CRDA    //
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    VPLL_Band_Sel    :1;
        CBIOS_U8    VPLL_CKOUT1_DIVN    :2;
        CBIOS_U8    VPLL_PU_Down    :1;
        CBIOS_U8    reserved_1    :4;
    };
}REG_CRDA;


typedef union _REG_CRDA_B    //GPIO2_and_THERM_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    THERM_Output_Data    :1;
        CBIOS_U8    THERM_Output_Enable    :1;
        CBIOS_U8    THERM_Input_Enable    :1;
        CBIOS_U8    THERM_Input_Data    :1;
        CBIOS_U8    GPIO2_Ouptut_Data    :1;
        CBIOS_U8    GPIO2_Output_Enable    :1;
        CBIOS_U8    GPIO2_Input_Enable    :1;
        CBIOS_U8    GPIO2_Input_Data    :1;
    };
}REG_CRDA_B;


typedef union _REG_CRDB    //
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved_1    :3;
        CBIOS_U8    VPLL_Fraction_EN    :1;
        CBIOS_U8    reserved_2    :4;
    };
}REG_CRDB;


typedef union _REG_CRDD    //
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    EPLL_Band_Sel    :1;
        CBIOS_U8    EPLL_CKOUT1_DIVN    :2;
        CBIOS_U8    EPLL_PU_Down    :1;
        CBIOS_U8    reserved_1    :4;
    };
}REG_CRDD;


typedef union _REG_CRDE    //
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved_1    :3;
        CBIOS_U8    EPLL_Fraction_EN    :1;
        CBIOS_U8    reserved_2    :4;
    };
}REG_CRDE;


typedef union _REG_CRE0    //VPLL
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    VPLL_SW_CTRL    :1;
        CBIOS_U8    VPLL_SW_LOAD      :1;
        CBIOS_U8    VPLL_RST_DOWN    :1;
        CBIOS_U8    Reserved    :5;
    };
}REG_CRE0;


typedef union _REG_CRE0_B    //DAC_Output_Current_Select_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    MTEST2    :1;
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    M_PLL_Select    :1;
        CBIOS_U8    DAC1_Output_Select    :1;
        CBIOS_U8    Reserved_4    :3;
        CBIOS_U8    Reserved_7    :1;
    };
}REG_CRE0_B;


typedef union _REG_CRE1    //Reserved_3_Bytes
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRE1;


typedef union _REG_CRE2    //EPLL
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    EPLL_SW_CTRL    :1;
        CBIOS_U8    EPLL_SW_LOAD      :1;
        CBIOS_U8    EPLL_RST_DOWN    :1;
        CBIOS_U8    LOAD_ECLK    :1;
        CBIOS_U8    EPLL_DISABLE   :1;
        CBIOS_U8    LUT_MMIO_EN   :1;
        CBIOS_U8    Reserved    :2;
    };
}REG_CRE2;


typedef union _REG_CRE3_B    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    RESERVED    :8;
    };
}REG_CRE3_B;


typedef union _REG_CRE4    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    LUT1_BIT_width  :2;
        CBIOS_U8    Reserved        :6;
    };
}REG_CRE4;


typedef union _REG_CRE4_B    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :8;
    };
}REG_CRE4_B;


typedef union _REG_CRE5    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRE5;


typedef union _REG_CRE5_B    //EOCLK_PLL_R_and_PLL_Load_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PLL_DCLK2_fractional_19to16_bit    :4;
        CBIOS_U8    RESERVED_7to4    :4;
    };
}REG_CRE5_B;


typedef union _REG_CRE6    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRE6;


typedef union _REG_CRE7    //FIFO_Underflow_Indicators_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS1FF_Underflow    :1;
        CBIOS_U8    PS2FF_Underflow    :1;
        CBIOS_U8    SS1FF_Underflow    :1;
        CBIOS_U8    SS2FF_Underflow    :1;
        CBIOS_U8    HWC1FF_Underflow    :1;
        CBIOS_U8    HWC2FF_Underflow    :1;
        CBIOS_U8    PS3FF_Underflow    :1;
        CBIOS_U8    SS3FF_Underflow    :1;
    };
}REG_CRE7;


typedef union _REG_CRE8    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    HWC3FF_Underflow    :1;
        CBIOS_U8    TVW_ACK_fake_flag    :1;
        CBIOS_U8    Reserved    :6;
    };
}REG_CRE8;


typedef union _REG_CRE8_B    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    reserved    :8;
    };
}REG_CRE8_B;


typedef union _REG_CRE9    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRE9;


typedef union _REG_CREA_CREB    //Reserved_2_Bytes
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CREA_CREB;


typedef union _REG_CREC_B    //DCLK1_PLL_Fractional_M_bit_9_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK_1_M_Fractional_bit15to8    :8;
    };
}REG_CREC_B;


typedef union _REG_CRED_CREF    //Reserved_3_Bytes
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRED_CREF;


typedef union _REG_CRF1    //IGA1/IGA2_Pixel_Repetition_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved_0    :1;
        CBIOS_U8    reserved_0    :1;
        CBIOS_U8    reserved_1    :1;
        CBIOS_U8    Reserved_1    :1;
        CBIOS_U8    IGA2_Pixel_Repetition    :2;
        CBIOS_U8    IGA1_Pixel_Repetition    :2;
    };
}REG_CRF1;


typedef union _REG_CRF2_B    //PLL_DCLK2_Parameter_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PLL_DCLK2_fractional_value    :8;
    };
}REG_CRF2_B;


typedef union _REG_CRF3_CRF4    //Reserved_2_Bytes
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRF3_CRF4;


typedef union _REG_CRF5    //Time_Out_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PS1_Timeout    :1;
        CBIOS_U8    PS2_Timeout    :1;
        CBIOS_U8    SS1_Timeout    :1;
        CBIOS_U8    SS2_Timeout    :1;
        CBIOS_U8    Disable_High_Request    :1;
        CBIOS_U8    reserved    :1;
        CBIOS_U8    RESERVED_7to6    :2;
    };
}REG_CRF5;


typedef union _REG_CRF5_B    //Spread_Spectrum_Control_0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SSC_Frequency_Control    :5;
        CBIOS_U8    SSC_Enable    :1;
        CBIOS_U8    DCLK1_SSC_Enable    :1;
        CBIOS_U8    DCLK2_SSC_Enable    :1;
    };
}REG_CRF5_B;


typedef union _REG_CRF6    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRF6;


typedef union _REG_CRF6_B    //Spread_Spectrum_Control_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Address_Generation_Control    :5;
        CBIOS_U8    SSC_LUT_Reset_Enable    :1;
        CBIOS_U8    External_Spread_Spectrum_Control    :1;
        CBIOS_U8    Clock_Output    :1;
    };
}REG_CRF6_B;


typedef union _REG_CRF7    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRF7;


typedef union _REG_CRF7_B    //Spread_Spectrum_Control_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SSC_Mode    :1;
        CBIOS_U8    Reserved_0    :4;
        CBIOS_U8    SSC_Frequency_Contro    :2;
        CBIOS_U8    Reserved_1    :1;
    };
}REG_CRF7_B;


typedef union _REG_CRF8    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRF8;


typedef union _REG_CRF8_B    //PWM_output_enable
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SPCLK5    :1;
        CBIOS_U8    SPDAT5_Data_Write    :1;
        CBIOS_U8    SPCLK5_Read    :1;
        CBIOS_U8    SPDAT5_Read    :1;
        CBIOS_U8    Serial_Port_5_Enable    :1;
        CBIOS_U8    Reserved    :1;
        CBIOS_U8    HDCP3_I2C_port_selection    :2;
    };
}REG_CRF8_B;


typedef union _REG_CRF9_Pair    //Reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRF9_Pair;


typedef union _REG_CRFA    //Flat_Panel_Power_Sequence_Control_Time_Factor_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    RESERVED    :7;
        CBIOS_U8    Time_Factor    :1;
    };
}REG_CRFA;


typedef union _REG_CRFB    //reserved
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    resreved    :8;
    };
}REG_CRFB;


typedef union _REG_CRFB_B    //Test_Data_Source_Select_3:0_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    S3D_DEBUG_SEL_7_0    :8;
    };
}REG_CRFB_B;


typedef union _REG_CRFC    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    SS1_Timeout_bit_7_0    :8;
    };
}REG_CRFC;


typedef union _REG_CRFC_B    //PLL_Power_Down_and_Lock_Control_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    PLL_DCLK1_POWER_DOWN    :1;
        CBIOS_U8    PLL_DCLK3_Power_Down    :1;
        CBIOS_U8    PLL_EICLK_Power_Down    :1;
        CBIOS_U8    RESERVED    :1;
        CBIOS_U8    DPLL_Lock    :1;
        CBIOS_U8    D2PLL_Lock    :1;
        CBIOS_U8    D3PLL_Lock    :1;
        CBIOS_U8    EOPLL_LOCK    :1;
    };
}REG_CRFC_B;


typedef union _REG_CRFD    //Debug_Bus_Status_1_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Debug_Bus_Status_23to16    :8;
    };
}REG_CRFD;


typedef union _REG_CRFD_B
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    DCLK4_PLL_PWDN    :1;
        CBIOS_U8    RESERVED0    :2;
        CBIOS_U8    SOFT_LOAD_D4CLK    :1;
        CBIOS_U8    LOAD_DCLK4    :1;
        CBIOS_U8    RESERVED1    :3;
    };
}REG_CRFD_B;


typedef union _REG_CRFE    //Debug_Bus_Status_2_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Debug_Bus_Status_15to8    :8;
    };
}REG_CRFE;


typedef union _REG_CRFE_B    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRFE_B;


typedef union _REG_CRFF    //Debug_Bus_Status_3_Register
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Debug_Bus_Status_7to0    :8;
    };
}REG_CRFF;


typedef union _REG_CRFF_B    //Reserved_1_Byte
{
    CBIOS_U8    Value;
    struct
    {
        CBIOS_U8    Reserved    :8;
    };
}REG_CRFF_B;


