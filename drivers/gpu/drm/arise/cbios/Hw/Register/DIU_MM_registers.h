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


typedef union _REG_MM8180    //Cursor_3_Contrl_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Cursor_3_Enable    :1;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    Cursor_3_Type    :2;
        CBIOS_U32    Cursor_3_X_Rotation    :1;
        CBIOS_U32    Cursor_3_Y_Rotation    :1;
        CBIOS_U32    Cursor_3_Size    :1;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    Cursor_3_Display_Start_X    :7;
        CBIOS_U32    Reserved_2    :1;
        CBIOS_U32    Cursor_3_Display_Start_Y    :7;
        CBIOS_U32    Reserved_3    :1;
        CBIOS_U32    Cursor_3_csc_format    :3;
        CBIOS_U32    Cursor_3_YCbCr420_Enable    :1;
        CBIOS_U32    Reserved_4    :3;
        CBIOS_U32    Cursor_3_mmio_reg_en    :1;
    };
}REG_MM8180;


typedef union _REG_MM8184    //Secondary_Stream_1_Color/Chroma_8bit_Key_Lower_Bound_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_8bit_B_V_Cr_Key_Lower_Bound    :8;
        CBIOS_U32    SS1_8bit_G_U_Cb_Key_Lower_Bound    :8;
        CBIOS_U32    SS1_8bit_R_Y_Key_Lower_Bound    :8;
        CBIOS_U32    Reserved_8bit    :8;
    };
    struct
    {
        CBIOS_U32    SS1_10bit_B_V_Cr_Key_Lower_Bound    :10;
        CBIOS_U32    SS1_10bit_G_U_Cb_Key_Lower_Bound    :10;
        CBIOS_U32    SS1_10bit_R_Y_Key_Lower_Bound    :10;
        CBIOS_U32    RESERVED_10bit    :2;
    };
}REG_MM8184;


typedef union _REG_MM8188    //Secondary_Stream_2_Color/Chroma_8bit_Key_Lower_Bound_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_8bit_B_V_Cr_Key_Lower_Bound    :8;
        CBIOS_U32    SS2_8bit_G_U_Cb_Key_Lower_Bound    :8;
        CBIOS_U32    SS2_8bit_R_Y_Key_Lower_Bound    :8;
        CBIOS_U32    RESERVED_8bit    :8;
    };
    struct
    {
        CBIOS_U32    SS2_10bit_B_V_Cr_Key_Lower_Bound    :10;
        CBIOS_U32    SS2_10bit_G_U_Cb_Key_Lower_Bound    :10;
        CBIOS_U32    SS2_10bit_R_Y_Key_Lower_Bound    :10;
        CBIOS_U32    RESERVED_10bit    :2;
    };
}REG_MM8188;


typedef union _REG_MM818C    //Secondary_Stream_2_Color/Chroma_Key_Upper_Bound_and_Key_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_V_Cr_Key_Upper_Bound    :8;
        CBIOS_U32    SS2_U_Cb_Key_Upper_Bound    :8;
        CBIOS_U32    SS2_Y_Key_Upper_Bound    :8;
        CBIOS_U32    SS2_Keying_Mode_Select    :4;
        CBIOS_U32    Invert_Alpha2_or_Ka2    :1;
        CBIOS_U32    RESERVED    :3;
    };
}REG_MM818C;

typedef union _REG_MM8190    //Streams_1_&_2_Blend_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ka1_3to0_or_Ks1    :4;
        CBIOS_U32    Ka1_7to4_or_Kp1    :4;
        CBIOS_U32    SS1_Input_Format    :3;
        CBIOS_U32    SS1_YCbCr_order    :1;
        CBIOS_U32    RESERVED_0    :4;
        CBIOS_U32    SS1_Source_Line_Width    :13;
        CBIOS_U32    RESERVED_1    :2;
        CBIOS_U32    RESERVED_2    :1;
    };
}REG_MM8190;


typedef union _REG_MM8194    //Secondary_Stream_1_Chroma_Key_Upper_Bound_and_Key_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_V_Cr_Key_Upper_Bound    :8;
        CBIOS_U32    SS1_U_Cb_Key_Upper_Bound    :8;
        CBIOS_U32    SS1_Y_Key_Upper_Bound    :8;
        CBIOS_U32    Keying_Mode    :4;
        CBIOS_U32    Invert_Alpha1_or_Ka1    :1;
        CBIOS_U32    RESERVED    :3;
    };
}REG_MM8194;


typedef union _REG_MM8198    //Cursor_1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Cursor_1_Enable    :1;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    Cursor_1_Type    :2;
        CBIOS_U32    Cursor_1_X_Rotation    :1;
        CBIOS_U32    Cursor_1_Y_Rotation    :1;
        CBIOS_U32    Cursor_1_Size    :1;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    Cursor_1_Display_Start_X    :7;
        CBIOS_U32    Reserved_2    :1;
        CBIOS_U32    Cursor_1_Display_Start_Y    :7;
        CBIOS_U32    Reserved_3    :1;
        CBIOS_U32    Cursor_1_csc_format    :3;
        CBIOS_U32    Cursor_1_YCbCr420_Enable    :1;
        CBIOS_U32    Reserved_4    :3;
        CBIOS_U32    Cursor_1_mmio_reg_en    :1;
    };
}REG_MM8198;


typedef union _REG_MM819C    //Cursor_1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Cursor_1_X_Origin    :14;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    Cursor_1_Y_Origin    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM819C;


typedef union _REG_MM81A0    //Cursor_1_Base_Address_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :3;
        CBIOS_U32    Cursor_1_Base_Address    :27;
        CBIOS_U32    Cursor_1_Vsync_Off_Page_Flip    :1;
        CBIOS_U32    Cursor_1_Enable_Work_Registers    :1;
    };
}REG_MM81A0;


typedef union _REG_MM81A4    //Cursor_1_Right_Frame_Base_Address_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    Cursor_1_Right_Frame_Base_Address    :27;
        CBIOS_U32    Reserved_1    :2;
    };
}REG_MM81A4;

typedef union _REG_MM81A8    //Secondary_Stream_Source_Height_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_Line_Height    :13;
        CBIOS_U32    SS1_Buffer_Select    :1;
        CBIOS_U32    SS1_Double_Buffering_Select    :1;
        CBIOS_U32    SS1_Triple_Buffering_Enable    :1;
        CBIOS_U32    Reserved    :16;
    };
}REG_MM81A8;


typedef union _REG_MM81AC    //Secondary_Stream_2_Window_Size_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_Window_Height    :13;
        CBIOS_U32    RESERVED_0    :2;
        CBIOS_U32    SS2_First_Line_Swizzle    :1;
        CBIOS_U32    SS2_Window_Width    :13;
        CBIOS_U32    RESERVED_1    :3;
    };
}REG_MM81AC;


typedef union _REG_MM81B0    //Primary_Stream_2_Frame_Buffer_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_Enable_Work_Registers_0    :1;
        CBIOS_U32    SS2_Enable_Work_Registers    :1;
        CBIOS_U32    PS2_Start_Address_0    :28;
        CBIOS_U32    PS2_Vsync_Off_Page_Flip    :1;
        CBIOS_U32    PS2_Enable_Work_Registers_1    :1;
    };
}REG_MM81B0;


typedef union _REG_MM81B4    //Cursor_3_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Cursor_3_X_Origin    :14;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    Cursor_3_Y_Origin    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM81B4;


typedef union _REG_MM81B8    //Primary_Stream_2_Stride_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    reserved_0    :1;
        CBIOS_U32    PS2_ABGR_EN    :1;
        CBIOS_U32    PS2_First_Line_Swizzle    :1;
        CBIOS_U32    PS2_Work_Reg_Enable    :1;
        CBIOS_U32    PS2_Stride_or_Tile_Stride    :12;
        CBIOS_U32    PS2_Start_Address_Pixel_Offset    :5;
        CBIOS_U32    PS2_Tiling_X_Offset    :3;
        CBIOS_U32    PS2_Tiling_Line_Offset    :6;
        CBIOS_U32    reserved_1    :1;
        CBIOS_U32    PS2_Tile_Addressing_Enable    :1;
    };
}REG_MM81B8;


typedef union _REG_MM81BC    //Secondary_Stream_2_Frame_Buffer_Start_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RESERVED    :3;
        CBIOS_U32    SS2_FB_Start_Address_0    :27;
        CBIOS_U32    SS2_VSYNC_Off_Page_Flip    :1;
        CBIOS_U32    SS2_Work_Reg_En    :1;
    };
}REG_MM81BC;


typedef union _REG_MM81C0    //Primary_Stream_1_Frame_Buffer_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_Enable_Work_Registers_0    :1;
        CBIOS_U32    SS1_Enable_Work_Registers      :1;
        CBIOS_U32    Reserved                       :1;
        CBIOS_U32    PS1_Start_Address_0            :27;
        CBIOS_U32    PS1_Vsync_Off_Page_Flip        :1;
        CBIOS_U32    PS1_Enable_Work_Registers_1    :1;
    };
}REG_MM81C0;

typedef union _REG_MM81C4    //Cursor_3_Base_Address_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :3;
        CBIOS_U32    Cursor_3_Base_Address    :27;
        CBIOS_U32    Cursor_3_Vsync_Off_Page_Flip    :1;
        CBIOS_U32    Cursor_3_Enable_Work_Registers    :1;
    };
}REG_MM81C4;


typedef union _REG_MM81C8    //Primary_Stream_1_Stride_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    reserved    :1;
        CBIOS_U32    PS1_ABGR_EN    :1;
        CBIOS_U32    PS1_First_Line_Swizzle    :1;
        CBIOS_U32    PS1_Work_Reg_Enable    :1;
        CBIOS_U32    PS1_Stride_or_Tile_Stride    :12;
        CBIOS_U32    PS1_Start_Address_Pixel_Offset    :5;
        CBIOS_U32    PS1_Tiling_X_Offset    :3;
        CBIOS_U32    PS1_Tiling_Line_Offset    :6;
        CBIOS_U32    RESERVED    :1;
        CBIOS_U32    PS1_Tile_Addressing_Enable    :1;
    };
}REG_MM81C8;

typedef union _REG_MM81CC    //Secondary_Stream_2_Stride_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_Read_length    :2;
        CBIOS_U32    RESERVED    :2;
        CBIOS_U32    SS2_Stride    :12;
        CBIOS_U32    SS2_Start_Address_Pixel_Offset    :6;
        CBIOS_U32    SS2_Tiling_X_Offset    :3;
        CBIOS_U32    SS2_Tiling_Line_Offset    :6;
        CBIOS_U32    SS2_ABGR_EN    :1;
    };
}REG_MM81CC;


typedef union _REG_MM81D0    //Secondary_Stream_1_Frame_Buffer_Start_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RESERVED    :3;
        CBIOS_U32    SS1_FB_Start_Address_0    :27;
        CBIOS_U32    SS1_VSYNC_Off_Page_Flip    :1;
        CBIOS_U32    SS1_Work_Reg_En    :1;
    };
}REG_MM81D0;

typedef union _REG_MM81D4    //Secondary_Stream_1_ROFFSET_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RESERVED_0    :3;
        CBIOS_U32    SS1_Right_Base_Address    :27;
        CBIOS_U32    RESERVED_1    :2;
    };
}REG_MM81D4;

typedef union _REG_MM81D8    //Secondary_Stream_1_Stride_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_Read_Length    :2;
        CBIOS_U32    RESERVED    :2;
        CBIOS_U32    SS1_Stride    :12;
        CBIOS_U32    SS1_Start_Address_Byte_Offset    :6;
        CBIOS_U32    SS1_Tiling_X_Offset    :3;
        CBIOS_U32    SS1_Tiling_Line_Offset    :6;
        CBIOS_U32    SS1_ABGR_EN    :1;
    };
}REG_MM81D8;

typedef union _REG_MM81DC    //Secondary_Stream_Source_Width_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    KA2_3to0_or_KS2    :4;
        CBIOS_U32    KA2_7to4_or_KP2    :4;
        CBIOS_U32    SS2_Input_Format    :3;
        CBIOS_U32    SS2_YCbCr_order    :1;
        CBIOS_U32    RESERVED_0    :4;
        CBIOS_U32    SS2_Source_Line_Width    :13;
        CBIOS_U32    RESERVED_1    :2;
        CBIOS_U32    RESERVED_2    :1;
    };
}REG_MM81DC;


typedef union _REG_MM81E0    //Secondary_Stream_2_ROFFSET_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RESERVED_0    :3;
        CBIOS_U32    SS2_Right_Base_Address    :27;
        CBIOS_U32    RESERVED_1    :2;
    };
}REG_MM81E0;


typedef union _REG_MM81E4    //Cursor_2_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Cursor_2_Enable    :1;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    Cursor_2_Type    :2;
        CBIOS_U32    Cursor_2_X_Rotation    :1;
        CBIOS_U32    Cursor_2_Y_Rotation    :1;
        CBIOS_U32    Cursor_2_Size    :1;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    Cursor_2_Display_Start_X    :7;
        CBIOS_U32    Reserved_2    :1;
        CBIOS_U32    Cursor_2_Display_Start_Y    :7;
        CBIOS_U32    Reserved_3    :1;
        CBIOS_U32    Cursor_2_csc_format    :3;
        CBIOS_U32    Cursor_2_YCbCr420_Enable    :1;
        CBIOS_U32    Reserved_4    :3;
        CBIOS_U32    Cursor_2_mmio_reg_en    :1;
    };
}REG_MM81E4;


typedef union _REG_MM81E8    //
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Cursor_2_X_Origin    :14;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    Cursor_2_Y_Origin    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM81E8;


typedef union _REG_MM81EC    //Secondary_Stream_1_Frame_Buffer_Start_Address_2_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RESERVED_0    :3;
        CBIOS_U32    SS1_FB_Start_Address_2    :27;
        CBIOS_U32    RESERVED_1    :2;
    };
}REG_MM81EC;


typedef union _REG_MM81F0    //Cursor_2_Base_Address_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :3;
        CBIOS_U32    Cursor_2_Base_Address    :27;
        CBIOS_U32    Cursor_2_Vsync_Off_Page_Flip    :1;
        CBIOS_U32    Cursor_2_Enable_Work_Registers    :1;
    };
}REG_MM81F0;


typedef union _REG_MM81F4    //Cursor_2_Right_Frame_Base_Address_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    Cursor_2_Right_Frame_Base_Address    :27;
        CBIOS_U32    Reserved_1    :2;
    };
}REG_MM81F4;


typedef union _REG_MM81F8    //Secondary_Stream_1_Window_Start_Coordinates_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_Y_Start    :13;
        CBIOS_U32    RESERVED_0    :3;
        CBIOS_U32    SS1_X_Start_0    :12;
        CBIOS_U32    SS1_Loading_Enable    :1;
        CBIOS_U32    SS1_X_Start_1    :1;
        CBIOS_U32    RESERVED_1    :2;
    };
}REG_MM81F8;


typedef union _REG_MM81FC    //Secondary_Stream1_Window_Size_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_Window_Height    :13;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    SS1_First_Line_swizzle    :1;
        CBIOS_U32    SS1_Window_Width    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM81FC;

typedef union _REG_MM8200    //Cursor_3_Right_Frame_Base_Address_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RESERVED_0    :3;
        CBIOS_U32    Cursor_3_Right_Frame_Base_Address    :27;
        CBIOS_U32    RESERVED_1    :2;
    };
}REG_MM8200;


typedef union _REG_MM8208    //interlace_right_offset[31:2]
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    reserved    :2;
        CBIOS_U32    PS_INTR_ROFFSET_31_2    :30;
    };
}REG_MM8208;


typedef union _REG_MM820C    //Secondary_Stream_2_Window_Start_Coordinates_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_Y_Start    :13;
        CBIOS_U32    RESERVED_0    :3;
        CBIOS_U32    SS2_X_Start_0    :12;
        CBIOS_U32    SS2_Loading_Enable    :1;
        CBIOS_U32    SS2_X_Start_1    :1;
        CBIOS_U32    RESERVED_1    :2;
    };
}REG_MM820C;


typedef union _REG_MM8210    //DP_NVID_amd_MISC1_Attribute_Data_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    NVID    :24;
        CBIOS_U32    MISC1_Even    :1;
        CBIOS_U32    MISC1_Stereo_Video    :2;
        CBIOS_U32    MISC1_Reserved    :5;
    };
}REG_MM8210;


typedef union _REG_MM8214    //DP_Link_Training_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Start_Link_Training    :1;
        CBIOS_U32    Start_Link_Rate_0    :1;
        CBIOS_U32    Max_V_swing    :2;
        CBIOS_U32    Max_Pre_emphasis    :2;
        CBIOS_U32    SW_Hpd_assert    :1;
        CBIOS_U32    Num_of_Lanes    :3;
        CBIOS_U32    SW_Link_Train_Enable    :1;
        CBIOS_U32    SW_Link_Train_State    :2;
        CBIOS_U32    Software_Bit_Rate    :1;
        CBIOS_U32    SW_Lane0_Swing    :2;
        CBIOS_U32    SW_Lane0_Pre_emphasis    :2;
        CBIOS_U32    SW_Lane1_Swing    :2;
        CBIOS_U32    SW_Lane1_Pre_emphasis    :2;
        CBIOS_U32    SW_Lane2_Swing    :2;
        CBIOS_U32    SW_Lane2_Pre_emphasis    :2;
        CBIOS_U32    SW_Lane3_Swing    :2;
        CBIOS_U32    SW_Lane3_Pre_emphasis    :2;
        CBIOS_U32    SW_Set_Link_Train_Fail    :1;
        CBIOS_U32    HW_Link_Training_Done    :1;
    };
}REG_MM8214;


typedef union _REG_MM8218    //DP_Video_Input_Select_and_General_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Scramble_enable    :1;
        CBIOS_U32    Switch_Idle_mode_to_video    :1;
        CBIOS_U32    Idle_Pattern_Counter    :9;
        CBIOS_U32    AUX_Length    :5;
        CBIOS_U32    Audio_Strm_Select    :1;
        CBIOS_U32    HW_Link_Train_Fail    :1;
        CBIOS_U32    Min_AUX_SYNC_Count    :6;
        CBIOS_U32    DELAY    :6;
        CBIOS_U32    reseved    :2;
    };
}REG_MM8218;


typedef union _REG_MM821C    //DP_Version_and_Extension_Packet_Head_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ext_Pkt_Head    :24;
        CBIOS_U32    Ext_Pkt_ID_value    :4;
        CBIOS_U32    RESEVERD    :2;
        CBIOS_U32    Horizontal_Width_bit12to11    :2;
    };
}REG_MM821C;

typedef union _REG_MM8220    //Primary_Stream_1_Display_Position_Frame_Line_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_Frame_Counter    :16;
        CBIOS_U32    PS1_Line_Counter    :16;
    };
}REG_MM8220;


typedef union _REG_MM8224    //Primary_Stream_1_Display_Position_Line_Pixel_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_Line_Counter    :16;
        CBIOS_U32    PS1_Pixel_Counter    :16;
    };
}REG_MM8224;


typedef union _REG_MM8828    //Primary_Stream_1_Top_Left_Pixel_Line_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_Top_Left_Line    :16;
        CBIOS_U32    PS1_Top_Leftt_Pixel    :16;
    };
}REG_MM8828;


typedef union _REG_MM822C    //Primary_Stream_1_Bottom_Right_Pixel_Line_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_Bottom_Right_Line    :16;
        CBIOS_U32    PS1_Bottom_Right_Pixel    :16;
    };
}REG_MM822C;


typedef union _REG_MM8230    //Primary_Stream_2_Display_Position_Frame_Line_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_Frame_Counter    :16;
        CBIOS_U32    PS2_Line_Counter    :16;
    };
}REG_MM8230;


typedef union _REG_MM8234    //Primary_Stream_2_Display_Position_Line_Pixel_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_Line_Counter    :16;
        CBIOS_U32    PS2_Pixel_Counter    :16;
    };
}REG_MM8234;


typedef union _REG_MM8238    //Primary_Stream_2_VDC_Top_Left_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_Top_Left_Line    :16;
        CBIOS_U32    PS2_Top_Leftt_Pixel    :16;
    };
}REG_MM8238;


typedef union _REG_MM823C    //Primary_Stream_2_VDC_Bottom_Right_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_Bottom_Right_Line    :16;
        CBIOS_U32    PS2_Bottom_Right_Pixel    :16;
    };
}REG_MM823C;


typedef union _REG_MM8240    //DP_Display_Port_Enable_and_InfoFrame_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DP_Enable    :1;
        CBIOS_U32    Field_Invert    :1;
        CBIOS_U32    Enhanced_Framing_Mode    :1;
        CBIOS_U32    Video_Enable    :1;
        CBIOS_U32    InfoFrame_FIFO_1_Ready    :1;
        CBIOS_U32    INFOFRAME_FIFO_2_READY    :1;
        CBIOS_U32    InfoFrame_FIFO_Select    :1;
        CBIOS_U32    InfoFrame_FIFO_1_Start_Address    :4;
        CBIOS_U32    InfoFrame_FIFO_2_Start_Address    :4;
        CBIOS_U32    InfoFrame_FIFO_1_Length    :4;
        CBIOS_U32    InfoFrame_FIFO_2_Length    :4;
        CBIOS_U32    Ext_Packet_Enable    :1;
        CBIOS_U32    Enable_Audio    :1;
        CBIOS_U32    Generate_MVID    :1;
        CBIOS_U32    output_format_is_BIAS_RGB    :1;
        CBIOS_U32    header_of_audio_info_frame_is_from_HDAudio_codec    :1;
        CBIOS_U32    Main_Link_Status    :2;
        CBIOS_U32    Link_Qual_Pattern_Set    :2;
    };
}REG_MM8240;


typedef union _REG_MM8244    //DP_Horizontal_Width_and_TU_Size_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Horiz_Width    :11;
        CBIOS_U32    TU_Size    :6;
        CBIOS_U32    TU_Ratio    :15;
    };
}REG_MM8244;


typedef union _REG_MM8248    //DP_Horizontal_Line_Duration_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Horiz_Line_Duration    :15;
        CBIOS_U32    HBLANK_Duration    :12;
        CBIOS_U32    Ext_Packet_Byte_Num    :4;
        CBIOS_U32    Ext_Packet_Available    :1;
    };
}REG_MM8248;


typedef union _REG_MM824C    //DP_MVID_and_MISC0_Attribute_Data_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    MVID    :24;
        CBIOS_U32    MISC0_Sync_Clk    :1;
        CBIOS_U32    MISC0_Component_Format    :2;
        CBIOS_U32    MISC0_Dynamic_Range    :1;
        CBIOS_U32    MISC0_YCbCr_Colorimetry    :1;
        CBIOS_U32    MISC0_Bit_depth    :3;
    };
}REG_MM824C;


typedef union _REG_MM8250    //DP_HTOTAL_Attribute_Data_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    H_Total    :16;
        CBIOS_U32    V_Total    :16;
    };
}REG_MM8250;


typedef union _REG_MM8254    //DP_Horiz_Vert_Start_Attribute_Data_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    H_Start    :16;
        CBIOS_U32    V_Start    :16;
    };
}REG_MM8254;


typedef union _REG_MM8258    //DP_Polarity/Width_Attribute_Data_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HSYNC_Width    :15;
        CBIOS_U32    HSYNC_Polarity    :1;
        CBIOS_U32    VSYNC_Width    :15;
        CBIOS_U32    VSYNC_Polarity    :1;
    };
}REG_MM8258;


typedef union _REG_MM825C    //DP_Polarity/Width_Attribute_Data_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Acitve_Width    :16;
        CBIOS_U32    Active_Height    :16;
    };
}REG_MM825C;


typedef union _REG_MM8260    //DSCL/Write_back_csc_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_Line_Height    :13;
        CBIOS_U32    SS2_Buffer_Select    :1;
        CBIOS_U32    SS2_Double_Buffering_Select    :1;
        CBIOS_U32    SS2_Triple_Buffering_Enable    :1;
        CBIOS_U32    Reserved    :16;
    };
}REG_MM8260;


typedef union _REG_MM8264    //DSCL/Write_back_csc_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    dscl_write_back_src_sel    :1;
        CBIOS_U32    CSC_DATA_IN_FMT    :3;
        CBIOS_U32    CSC_DATA_OUT_FMT    :3;
        CBIOS_U32    CSC_PROGRAMMBLE    :1;
        CBIOS_U32    CSC_BRIGHT    :9;
        CBIOS_U32    CSC_COEF_F9    :14;
        CBIOS_U32    Reserved_31    :1;
    };
}REG_MM8264;


typedef union _REG_MM8268    //DSCL/Write_back_csc_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CSC_COEF_F3    :14;
        CBIOS_U32    CSC_COEF_F4    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM8268;


typedef union _REG_MM826C    //DSCL/Write_back_csc_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CSC_COEF_F5    :14;
        CBIOS_U32    CSC_COEF_F6    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM826C;


typedef union _REG_MM8270    //Primary_Stream_1_Right_Frame_Offset_Address_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ps1_Right_Frame_Base    :30;
        CBIOS_U32    RESERVED    :2;
    };
}REG_MM8270;


typedef union _REG_MM8274    //Primary_Stream_2_Right_Frame_Offset_Address_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ps2_Right_Frame_Base    :30;
        CBIOS_U32    RESERVED    :2;
    };
}REG_MM8274;


typedef union _REG_MM8278    //Secondary_Stream_1_Horizontal_Scaling_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_HACC    :21;
        CBIOS_U32    SS1_HTAP_SEL    :1;
        CBIOS_U32    RESERVED    :10;
    };
}REG_MM8278;


typedef union _REG_MM827C    //Secondary_Stream_2_Horizontal_Scaling_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_HACC    :21;
        CBIOS_U32    SS2_HTAP_SEL    :1;
        CBIOS_U32    RESERVED    :10;
    };
}REG_MM827C;


typedef union _REG_MM8280    //HDMI1_General_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDMI_Reset    :1;
        CBIOS_U32    HDMI_Enable    :1;
        CBIOS_U32    Deep_Color_Mode    :2;
        CBIOS_U32    reserved    :1;
        CBIOS_U32    Video_Clip    :1;
        CBIOS_U32    DVI_Mode_during_HDMI_Enable    :1;
        CBIOS_U32    TMDS_Video_Pixel_Format_Select    :2;
        CBIOS_U32    Convert_to_YCbCr422_Enable    :1;
        CBIOS_U32    HSYNC_Invert_Enable    :1;
        CBIOS_U32    HDMI_Debug_Bus_Select    :1;
        CBIOS_U32    Reserved    :1;
        CBIOS_U32    VSYNC_Invert_Enable    :1;
        CBIOS_U32    Reserved_15to14    :2;
        CBIOS_U32    Delay_for_HDCP    :7;
        CBIOS_U32    Delay_for_HDCP_SEL    :1;
        CBIOS_U32    Transmit_Between_AE_300_Enable    :1;
        CBIOS_U32    Transmit_Between_385_507_Enable    :1;
        CBIOS_U32    Transmit_After_650_Enable    :1;
        CBIOS_U32    STATUS_OF_HDMI_STATION_MACHINE    :5;
    };
}REG_MM8280;


typedef union _REG_MM8284    //HDMI1__InfoFrame_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    InfoFrame_FIFO_2_Select    :1;
        CBIOS_U32    InfoFrame_FIFO_1_Ready    :1;
        CBIOS_U32    INFOFRAME_FIFO_2_READY    :1;
        CBIOS_U32    INFO_VSYNC_EN    :1;
        CBIOS_U32    InfoFrame_FIFO_1_Start_Address    :4;
        CBIOS_U32    InfoFrame_FIFO_1_Length    :5;
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    InfoFrame_FIFO_2_Start_Address    :4;
        CBIOS_U32    InfoFrame_FIFO_2_Length    :5;
        CBIOS_U32    Reserved_1    :3;
        CBIOS_U32    Horiz_Blank_Max_Packets    :4;
    };
}REG_MM8284;


typedef union _REG_MM8288    //HD_Audio_Codec_Status_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :25;
        CBIOS_U32    Int_Src_Codec1    :1;
        CBIOS_U32    Int_Src_Codec2    :1;
        CBIOS_U32    Reserved_2    :5;
    };
}REG_MM8288;


typedef union _REG_MM828C    //Secondary_Stream_1_Vertical_Scaling_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_VACC    :21;
        CBIOS_U32    SS1_Vscale_Selection    :1;
        CBIOS_U32    SS1_VDUP    :1;
        CBIOS_U32    RESERVED    :8;
        CBIOS_U32    SS1_Deinterlacing    :1;
    };
}REG_MM828C;


typedef union _REG_MM8290    //Secondary_Stream_1_Frame_Buffer_Start_Address_1_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    SS1_FB_Start_Address_1    :27;
        CBIOS_U32    Reserved_1    :2;
    };
}REG_MM8290;


typedef union _REG_MM8294    //HDMI1_Audio_Insert_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Select_HDMI_Audio_Source    :1;
        CBIOS_U32    HDMI_Audio_Enable    :1;
        CBIOS_U32    Set_AVMUTE_Enable    :1;
        CBIOS_U32    Clear_AVMUTE_Enable    :1;
        CBIOS_U32    HDAUDIO_Stream1_Threshold    :6;
        CBIOS_U32    HDAUDIO_Stream2_Threshold    :6;
        CBIOS_U32    Reserved    :1;
        CBIOS_U32    DC_Gen_Cntl_Pkt_EN    :1;
        CBIOS_U32    SW_PP    :4;
        CBIOS_U32    PP_SELECT    :1;
        CBIOS_U32    CD    :4;
        CBIOS_U32    Default_Phase    :1;
        CBIOS_U32    ACR_ratio_select    :1;
        CBIOS_U32    RIRB_WPTR_INC_SEL    :1;
        CBIOS_U32    Reserved_31to30    :2;
    };
}REG_MM8294;


typedef union _REG_MM8298    //HDAUDIO_CODEC1_Audio_Packet_to_Clock_Ratio_Register_1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CODEC1_Audio_Packet_to_DClk_Ratio_31to0    :32;
    };
}REG_MM8298;


typedef union _REG_MM829C    //HDAUDIO_CODEC1_Audio_Packet_to_Clock_Ratio_Register_2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CODEC1_Audio_Packet_to_DClk_Ratio_39to32    :8;
        CBIOS_U32    CODEC1_ACR_ratio    :20;
        CBIOS_U32    CODEC1_ACR_ENABLE    :1;
        CBIOS_U32    CODEC1_MUTE_EN    :1;
        CBIOS_U32    CODEC1_WAKE_FROM_S3    :1;
        CBIOS_U32    CODEC1_SW_RESET    :1;
    };
}REG_MM829C;


typedef union _REG_MM82A0    //HDAUDIO_CODEC1_Audio_Mode_and_Response_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HD_AUDIO_MODE_SELECT    :1;
        CBIOS_U32    ResP_generation    :1;
        CBIOS_U32    Resp_Ready    :1;
        CBIOS_U32    Send_UNSOLRESP    :1;
        CBIOS_U32    HDAUDIO_CODEC1_Enable    :1;
        CBIOS_U32    up_Sample_incoming_audio    :4;
        CBIOS_U32    Driver_ready    :1;
        CBIOS_U32    Ignore_driver_ready    :1;
        CBIOS_U32    Down_sample_incoming_audio_factor    :4;
        CBIOS_U32    Use_SW_Stream_Format    :1;
        CBIOS_U32    SW_Stream_Format    :16;
    };
}REG_MM82A0;


typedef union _REG_MM82A4    //HDAUDIO_CODEC1_Command_Field_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDA_HDMI_CMD_Verb    :20;
        CBIOS_U32    NID    :8;
        CBIOS_U32    Cad    :4;
    };
}REG_MM82A4;


typedef union _REG_MM82A8    //HDAUDIO_CODEC1_Software_Response_Field_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Response    :32;
    };
}REG_MM82A8;


typedef union _REG_MM82AC    //HDAUDIO_CODEC1_Speaker_Allocation_and_Channel_Status_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    ELD_READ_Status    :1;
        CBIOS_U32    DIP_WRITE_Status    :1;
        CBIOS_U32    DIP_READ_Status    :1;
        CBIOS_U32    HDA_Setconvert_Int_Status    :1;
        CBIOS_U32    HDA_Cpcontrol_Int_Status    :1;
        CBIOS_U32    SET_CONVERTER1_DigiConvert3_INT_Status    :1;
        CBIOS_U32    SET_FunGroup_PowerState_INT_status    :1;
        CBIOS_U32    Set_ELD_Default    :1;
        CBIOS_U32    sample_flat    :1;
        CBIOS_U32    Always_Output_Audio    :1;
        CBIOS_U32    multiple_sample    :1;
        CBIOS_U32    faudio_selsect    :1;
        CBIOS_U32    Ratio_CLK_Select    :1;
        CBIOS_U32    Codec_Type    :3;
        CBIOS_U32    Reserved_bits_18to16    :3;
        CBIOS_U32    BCIS_SEL    :1;
        CBIOS_U32    Reserved_bits_23to20    :4;
        CBIOS_U32    ELD_Use_LUT    :1;
        CBIOS_U32    Enable_Transmit_DIP_Packet    :1;
        CBIOS_U32    Enable_HDA_POS_CTR    :1;
        CBIOS_U32    Converter_Stream_Channel    :1;
        CBIOS_U32    CP_Control_CES_State    :1;
        CBIOS_U32    CP_Control_Ready_Status    :1;
        CBIOS_U32    Channel_status_control    :2;
    };
}REG_MM82AC;


typedef union _REG_MM82B0    //HDCP1_Key_Selection_Vector_(KSV)_Register_1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    KSV_39to8    :32;
    };
}REG_MM82B0;


typedef union _REG_MM82B4    //HDCP1_Control_1_and_Key_Selection_Vector_(KSV)_2_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    KSV_7to0    :8;
        CBIOS_U32    Read_Data    :8;
        CBIOS_U32    Write_Data    :8;
        CBIOS_U32    Source_Select    :2;
        CBIOS_U32    Test_Key_Enable    :1;
        CBIOS_U32    CP_EN    :1;
        CBIOS_U32    Mode_Sel    :1;
        CBIOS_U32    AC_EN    :1;
        CBIOS_U32    Verify_Pj_Enable    :1;
        CBIOS_U32    EESS_Signaling_Select    :1;
    };
}REG_MM82B4;


typedef union _REG_MM82B8    //HDCP1_Control_2_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDCP_I2C_Function_Enable    :1;
        CBIOS_U32    SW_Request_I2C_Access    :1;
        CBIOS_U32    HDCP_Test_Mode_Select    :1;
        CBIOS_U32    Write_Data_Available    :1;
        CBIOS_U32    START_Request    :1;
        CBIOS_U32    STOP_REQUEST    :1;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    READ_Finished    :1;
        CBIOS_U32    KSV_Revocation_List_Available    :1;
        CBIOS_U32    KSV_Verification_Done    :1;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    I2C_Status    :1;
        CBIOS_U32    Authentication_Protocol_Status    :2;
        CBIOS_U32    Reserved_2    :2;
        CBIOS_U32    EFUSE_read_Address    :7;
        CBIOS_U32    Reserved_3    :9;
    };
}REG_MM82B8;


typedef union _REG_MM82BC    //Efuse_Read_Data
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Efuse_Read_Data    :32;
    };
}REG_MM82BC;


typedef union _REG_MM82C0    //HDCP1_Slave_receiver_not_ready_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :24;
        CBIOS_U32    Slave_Receiver_Not_Ready    :1;
        CBIOS_U32    EFUSE_read_request    :1;
        CBIOS_U32    Reserved_1    :2;
        CBIOS_U32    efuse_mode    :3;
        CBIOS_U32    Reserved_2    :1;
    };
}REG_MM82C0;


typedef union _REG_MM82C4    //HDCP1_Miscellaneous_1_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Interrupt_Source :4;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    AUTH_FAIL_INT_SEL  :1;
        CBIOS_U32    HDCP_SW_Reset    :1;
        CBIOS_U32    Repeater_Flag    :1; // 0: receiver, 1: repeater
        CBIOS_U32    Device_Count    :7;
        CBIOS_U32    Reserved_1    :3;
        CBIOS_U32    CTL    :4; // 1: Encryption is disabled for this frame, 9: Encryption is enabled for this frame
        CBIOS_U32    I2C_Frequency_Sleect    :3;
        CBIOS_U32    reserved    :1;
        CBIOS_U32    Disable_hamming_decoder    :1;
        CBIOS_U32    Reserved_2    :4;
    };
}REG_MM82C4;


typedef union _REG_MM82C8    //HDCP1_Miscellaneous_Register_2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RI_Verification_Counter    :4;
        CBIOS_U32    OW_Config    :3;
        CBIOS_U32    Re_Auth_Off    :1;
        CBIOS_U32    Auto_Detect_I2C_Off    :1;
        CBIOS_U32    CTL_Select    :1;
        CBIOS_U32    Not_support_0_KSV_repeater    :1;
        CBIOS_U32    No_check_KSV_list_ready    :1;
        CBIOS_U32    Reserved_14to12    :3;
        CBIOS_U32    Read_Out_AKSV    :1;
        CBIOS_U32    HDCP1_Interrupt    :1;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    Reserved_1    :2;
        CBIOS_U32    command_buffer_nfull    :1;
        CBIOS_U32    command_buffer_nempty    :1;
        CBIOS_U32    Reserved_2    :2;
        CBIOS_U32    Reserved_3    :1;
        CBIOS_U32    DRV_EFUSE_RWL    :1;
        CBIOS_U32    DRV_EFUSE_RWL_SEL    :1;
        CBIOS_U32    reserved    :1;
        CBIOS_U32    DRV_EFUSE_RSB    :1;
        CBIOS_U32    Reserved_4    :3;
    };
}REG_MM82C8;

typedef union _REG_MM82CC    //DP1_control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TX0T    :3;
        CBIOS_U32    TX1T    :3;
        CBIOS_U32    TX2T    :3;
        CBIOS_U32    TX3T    :3;
        CBIOS_U32    DIAJ_L0    :3;
        CBIOS_U32    DIAJ_L1    :3;
        CBIOS_U32    DIAJ_L2    :3;
        CBIOS_U32    DIAJ_L3    :3;
        CBIOS_U32    DIU_EPHY1_AUX_DIAJ    :1;
        CBIOS_U32    check_sync_cnt    :1;
        CBIOS_U32    int_mode    :1;
        CBIOS_U32    RST_PISO_EN    :1;
        CBIOS_U32    CR2EQ_WR02_ONLY    :1;
        CBIOS_U32    EN_DEFER_LT8    :1;
        CBIOS_U32    EDP_ASSR    :1;
        CBIOS_U32    DP_Clock_Debug    :1;
    };
}REG_MM82CC;


typedef union _REG_MM82D0    //HDAUDIO_CODEC1_Vendor_ID_Parameter_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Device_ID    :16;
        CBIOS_U32    Vendor_ID    :16;
    };
}REG_MM82D0;


typedef union _REG_MM82D4    //HDAUDIO_CODEC1_Revision_ID_and_Support_Parameters_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Stepping_ID    :8;
        CBIOS_U32    Revision_ID    :8;
        CBIOS_U32    MinRev    :4;
        CBIOS_U32    MajRev    :4;
        CBIOS_U32    PCM_Support    :1;
        CBIOS_U32    PCM_Float32_Only_Support    :1;
        CBIOS_U32    AC3_16_bit_only_Support    :1;
        CBIOS_U32    Reserved    :1;
        CBIOS_U32    SuppPowerState_D0Sup    :1;
        CBIOS_U32    SuppPowerState_D1Sup    :1;
        CBIOS_U32    SuppPowerState_D2Sup    :1;
        CBIOS_U32    SuppPowerState_D3Sup    :1;
    };
}REG_MM82D4;


typedef union _REG_MM82D8    //HDAUDIO_CODEC1_Function_Group_Subordinate_Node_Count_Parameter_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FunGroup_Sub_Node_Count_Total    :8;
        CBIOS_U32    Reserved_15to8    :8;
        CBIOS_U32    FunGroup_Sub_Node_Start_Num    :8;
        CBIOS_U32    Reserved_31to24    :8;
    };
}REG_MM82D8;


typedef union _REG_MM82DC    //HDAUDIO_CODEC1_Audio_Function_Group_Type_and_Capability_Parameter_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AFGC_Output_Delay    :4;
        CBIOS_U32    Reserved_7to4    :4;
        CBIOS_U32    AFGC_Input_Delay    :4;
        CBIOS_U32    Reserved_15to12    :4;
        CBIOS_U32    AFGC_BeepGen    :1;
        CBIOS_U32    Reserved    :2;
        CBIOS_U32    _7_3_4_12_EPSS    :1;
        CBIOS_U32    _7_3_4_12_CLKSTOP    :1;
        CBIOS_U32    _7_3_4_12_S3D3coldSup    :1;
        CBIOS_U32    _7_3_4_12_D3COLDSUP    :1;
        CBIOS_U32    AFGT_NodeType    :8;
        CBIOS_U32    AFGT_UnSol_Capable    :1;
    };
}REG_MM82DC;


typedef union _REG_MM82E0    //HDAUDIO_CODEC1_PCM_Size_and_Sample_Rate_Capability_Parameter_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    R1    :1;
        CBIOS_U32    R2    :1;
        CBIOS_U32    R3    :1;
        CBIOS_U32    R4    :1;
        CBIOS_U32    R5    :1;
        CBIOS_U32    R6    :1;
        CBIOS_U32    R7    :1;
        CBIOS_U32    R8    :1;
        CBIOS_U32    R9    :1;
        CBIOS_U32    R10    :1;
        CBIOS_U32    R11    :1;
        CBIOS_U32    R12    :1;
        CBIOS_U32    Reserved_15to12    :4;
        CBIOS_U32    B8    :1;
        CBIOS_U32    B16    :1;
        CBIOS_U32    B20    :1;
        CBIOS_U32    B24    :1;
        CBIOS_U32    B32    :1;
        CBIOS_U32    Reserved_31to21    :11;
    };
}REG_MM82E0;


typedef union _REG_MM82E4    //HDAUDIO_CODEC1_Output_Amplifier_Capability_Parameter_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    OutputAmpC_Offset    :7;
        CBIOS_U32    Reserved_7    :1;
        CBIOS_U32    OutputAmpC_NumSteps    :7;
        CBIOS_U32    Reserved_15    :1;
        CBIOS_U32    OutputAmpC_StepSize    :7;
        CBIOS_U32    Reserved_30to23    :8;
        CBIOS_U32    OutputAmpC_Mute_Capable    :1;
    };
}REG_MM82E4;


typedef union _REG_MM82E8    //HDAUDIO_CODEC1_Functional_Group_Subsystem_ID_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FunGroup__SubsystemID_SSID    :32;
    };
}REG_MM82E8;


typedef union _REG_MM82EC    //HDAUDIO_CODEC1_Converter1_Audio_Widget_Capatibility_Parameter_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Stereo    :1;
        CBIOS_U32    In_Amp_Present    :1;
        CBIOS_U32    Out_Amp_Present    :1;
        CBIOS_U32    Amp_Param_Override    :1;
        CBIOS_U32    Format_Override    :1;
        CBIOS_U32    Stripe    :1;
        CBIOS_U32    Proc_Widget    :1;
        CBIOS_U32    Unsol_Capable    :1;
        CBIOS_U32    Conn_List    :1;
        CBIOS_U32    Digital    :1;
        CBIOS_U32    Power_Cntrl    :1;
        CBIOS_U32    L_R_Swap    :1;
        CBIOS_U32    Reserved_15to12    :4;
        CBIOS_U32    Delay    :4;
        CBIOS_U32    Type    :4;
        CBIOS_U32    Reserved_31to24    :8;
    };
}REG_MM82EC;


typedef union _REG_MM82F0    //HDAUDIO_CODEC1_PinWidget1_Audio_Widget_Capability_Parameter_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Stereo    :1;
        CBIOS_U32    In_Amp_Present    :1;
        CBIOS_U32    Out_Amp_Present    :1;
        CBIOS_U32    Amp_Param_Override    :1;
        CBIOS_U32    Format_Override    :1;
        CBIOS_U32    Stripe    :1;
        CBIOS_U32    Proc_Widget    :1;
        CBIOS_U32    Unsol_Capable    :1;
        CBIOS_U32    Conn_List    :1;
        CBIOS_U32    Digital    :1;
        CBIOS_U32    Power_Cntrl    :1;
        CBIOS_U32    L_R_Swap    :1;
        CBIOS_U32    Reserved_15to12    :4;
        CBIOS_U32    Delay    :4;
        CBIOS_U32    Type    :4;
        CBIOS_U32    Reserved_31to24    :8;
    };
}REG_MM82F0;


typedef union _REG_MM82F4    //HDAUDIO_CODEC1_PinWidget1_Pin_Capability_Parameter_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Impedance_Sense_Capable    :1;
        CBIOS_U32    Trigger_Reqd    :1;
        CBIOS_U32    Presense_Detect_Capable    :1;
        CBIOS_U32    Headphone_Drive_Capable    :1;
        CBIOS_U32    Output_Capable    :1;
        CBIOS_U32    Input_Capable    :1;
        CBIOS_U32    Balanced_IO_Pins    :1;
        CBIOS_U32    HDMI    :1;
        CBIOS_U32    Vref_Control_0_Hi_Z    :1;
        CBIOS_U32    Vref_Control_1_50_percent    :1;
        CBIOS_U32    Vref_Control_2_Ground    :1;
        CBIOS_U32    Vref_Control_3_Reserved    :1;
        CBIOS_U32    Vref_Control_4_80_percent    :1;
        CBIOS_U32    Vref_Control_5_100_percent    :1;
        CBIOS_U32    Vref_Control_7to6_Reserved    :2;
        CBIOS_U32    EAPD_Capable    :1;
        CBIOS_U32    Reserved_31to17    :15;
    };
}REG_MM82F4;


typedef union _REG_MM82F8    //HDAUDIO_CODEC1_PinWidget1_Configuration_Default_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Config_Default_Sequence    :4;
        CBIOS_U32    Config_Default_Default_Assn    :4;
        CBIOS_U32    Config_Default_Misc    :4;
        CBIOS_U32    Config_Default_Color    :4;
        CBIOS_U32    Config_Default_Connect_Type    :4;
        CBIOS_U32    Config_Default_Default_Device    :4;
        CBIOS_U32    Config_Default_Location_low_bits    :4;
        CBIOS_U32    Config_Default_Location_hi_bits    :2;
        CBIOS_U32    Config_Default_Port_Connectivity    :2;
    };
}REG_MM82F8;


typedef union _REG_MM82FC    //HDAUDIO_CODEC1_Converter1_Stream_Format_and_Channel_Stream_ID_Controls_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Converter_Format_CHAN    :4;
        CBIOS_U32    Converter_Format_BITS    :3;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    Converter_Format_DIV    :3;
        CBIOS_U32    Converter_Format_MULT    :3;
        CBIOS_U32    Converter_Format_BASE    :1;
        CBIOS_U32    Converter_Format_TYPE    :1;
        CBIOS_U32    Reserved_1    :8;
        CBIOS_U32    Converter_Channel    :4;
        CBIOS_U32    Converter_Stream    :4;
    };
}REG_MM82FC;


typedef union _REG_MM8300    //HDAUDIO_CODEC1_Converter1_Digital_Converter_and_PinWidget1_Unsolicited_Response_Controls_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CONVERTER1_DigiConvert_SIC_DigEn    :1;
        CBIOS_U32    CONVERTER1_DigiConvert_SIC_V    :1;
        CBIOS_U32    CONVERTER1_DigiConvert_SIC_VCFG    :1;
        CBIOS_U32    CONVERTER1_DigiConvert_SIC_PRE    :1;
        CBIOS_U32    CONVERTER1_DigiConvert_SIC_COPY    :1;
        CBIOS_U32    CONVERTER1_DigiConvert_SIC_AUDIO    :1;
        CBIOS_U32    CONVERTER1_DigiConvert_SIC_PRO    :1;
        CBIOS_U32    CONVERTER1_DigiConvert_SIC_L    :1;
        CBIOS_U32    CONVERTER1_DigiConvert_SIC_CC_6to0    :7;
        CBIOS_U32    CONVERTER1_DigiConvert_SIC_Reserved    :1;
        CBIOS_U32    IEC_CODING_TYPE    :4;
        CBIOS_U32    disable_non_audio    :1;
        CBIOS_U32    copy_bit_polarity    :1;
        CBIOS_U32    enable_73eh_73fh_verb    :1;
        CBIOS_U32    keep_alive_enable    :1;
        CBIOS_U32    PINWIDGET1_UnsoliResponse_EnableUnsol_Tag    :6;
        CBIOS_U32    PINWIDGET1_UnsoliResponse_EnableUnsol_bit6    :1;
        CBIOS_U32    PINWIDGET1_UnsoliResponse_EnableUnsol_Enable    :1;
    };
}REG_MM8300;


typedef union _REG_MM8304    //HDAUDIO_CODEC1_Converter_1_Stripe_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CONVERTER1_Stripe_Control    :2;
        CBIOS_U32    CONVERTER1_StripeControl_Reserved_7to2    :6;
        CBIOS_U32    Undefined    :12;
        CBIOS_U32    CONVERTER1_StripeControl_Stripe_Capability__bit_0    :1;
        CBIOS_U32    CONVERTER1_StripeControl_Stripe_Capability__bit_1    :1;
        CBIOS_U32    CONVERTER1_StripeControl_Stripe_Capability__bit_2    :1;
        CBIOS_U32    CONVERTER1_StripeControl_Reserved_31to23    :9;
    };
}REG_MM8304;


typedef union _REG_MM8308    //HDAUDIO_CODEC1_PinWidget1_Pin_Sense_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PINWIDGET1_PinSense_RightChnl    :1;
        CBIOS_U32    PINWIDGET1_PinSense_Impedance    :30;
        CBIOS_U32    PINWIDGET1_PinSense_Presense_Detect    :1;
    };
}REG_MM8308;


typedef union _REG_MM830C    //HDAUDIO_CODEC1_PinWidget1_Pin_Widget_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PINWIDGET1_Control__VRefEn    :3;
        CBIOS_U32    PINWIDGET1_Control_Reserved_4to3    :2;
        CBIOS_U32    PINWIDGET1_Control__In_Enable    :1;
        CBIOS_U32    PINWIDGET1_Control__Out_Enable    :1;
        CBIOS_U32    PINWIDGET1_Control__H_Phn_Enable    :1;
        CBIOS_U32    FunGroup_Power_State_PS_Set_PS_Act    :4;
        CBIOS_U32    power_state    :7;
        CBIOS_U32    hda_power_saving_en    :1;
        CBIOS_U32    PS_SettingsReset    :1;
        CBIOS_U32    Reserved_31    :11;
    };
}REG_MM830C;


typedef union _REG_MM8310    //DP_Software_AUX_Write_Data_Bytes_3-0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Write_Bytes_3_0    :32;
    };
}REG_MM8310;


typedef union _REG_MM8314    //DP_Software_AUX_Write_Data_Bytes_7-4_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Write_Bytes_7_4    :32;
    };
}REG_MM8314;


typedef union _REG_MM8318    //DP_Software_AUX_Write_Data_Bytes_11-8_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Write_Bytes_11_8    :32;
    };
}REG_MM8318;


typedef union _REG_MM831C    //DP_Software_AUX_Write_Data_Bytes_15-12_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Write_Bytes_15_12    :32;
    };
}REG_MM831C;


typedef union _REG_MM8320    //DP_Software_AUX_Read_Data_Bytes_3-0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Read_Bytes_3_0    :32;
    };
}REG_MM8320;


typedef union _REG_MM8324    //DP_Software_AUX_Read_Data_Bytes_7-4_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Read_Bytes_7_4    :32;
    };
}REG_MM8324;


typedef union _REG_MM8328    //DP_Software_AUX_Read_Data_Bytes_11-8_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Read_Bytes_11_8    :32;
    };
}REG_MM8328;


typedef union _REG_MM832C    //DP_Software_AUX_Read_Data_Bytes_15-12_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Read_Bytes_15_12    :32;
    };
}REG_MM832C;


typedef union _REG_MM8330    //DP_Software_AUX_Timer_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SW_AUX    :1;
        CBIOS_U32    AUX_Request    :1;
        CBIOS_U32    AUX_DRDY    :1;
        CBIOS_U32    AUX_Timeout    :1;
        CBIOS_U32    SW_Timer_Clear    :1;
        CBIOS_U32    SW_Timer_Enable    :1;
        CBIOS_U32    SW_Timer_Counter    :20;
        CBIOS_U32    AUX_Command    :4;
        CBIOS_U32    HPD_Status    :2;
    };
}REG_MM8330;


typedef union _REG_MM8334    //DP_Software_AUX_Command_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SW_AUX_CMD    :4;
        CBIOS_U32    SW_AUX_Length    :8;
        CBIOS_U32    SW_AUX_Addr    :20;
    };
}REG_MM8334;


typedef union _REG_MM8338    //DP_NAUD_and_Mute_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    NAUD    :24;
        CBIOS_U32    Audio_InfoFrame    :1;
        CBIOS_U32    Audio_Output_Enable    :1;
        CBIOS_U32    Mute    :1;
        CBIOS_U32    Mute_Mode    :1;
        CBIOS_U32    Generate_MAUD    :1;
        CBIOS_U32    Generated_MAUD_Mode    :1;
        CBIOS_U32    AUX_SW_Reset    :1;
        CBIOS_U32    Link_Training_SW_Reset    :1;
    };
}REG_MM8338;


typedef union _REG_MM833C    //DP_MAUD_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    MAUD    :24;
        CBIOS_U32    Secondary_data_Packet_ID    :8;
    };
}REG_MM833C;


typedef union _REG_MM8340    //DP_EPHY_MPLL_Power_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Bandgap_Power_Down    :1;
        CBIOS_U32    MPLL_Reg_Power_Down    :1;
        CBIOS_U32    MPLL_Power_Down    :1;
        CBIOS_U32    MPLL_PTAT_Current    :2;
        CBIOS_U32    MPLL_CP_Current    :3;
        CBIOS_U32    MPLL_N    :8;
        CBIOS_U32    MPLL_R    :1;
        CBIOS_U32    MPLL_P    :2;
        CBIOS_U32    SSC_Enable    :1;
        CBIOS_U32    SSC_Freq_Spread    :1;
        CBIOS_U32    Dither    :1;
        CBIOS_U32    Signal_Profile    :1;
        CBIOS_U32    Spread_Magnitude    :2;
        CBIOS_U32    TPLL_Reg_Power_Down    :1;
        CBIOS_U32    TPLL_Power_Down    :1;
        CBIOS_U32    reserverd    :3;
        CBIOS_U32    TPLL_N_Div    :2;
    };
}REG_MM8340;


typedef union _REG_MM8344    //DP_EPHY_TX_Power_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Resistance_Tuning_PD    :1;
        CBIOS_U32    Resistance_Tuning_Reset    :1;
        CBIOS_U32    Resistance_Tuning_Enable    :1;
        CBIOS_U32    TX_Resistance_Set_Enable    :1;
        CBIOS_U32    TX_Resistance_Value    :4;
        CBIOS_U32    TX_Reg_Power_Down_Lane0    :1;
        CBIOS_U32    TX_Reg_Power_Down_Lane1    :1;
        CBIOS_U32    TX_Reg_Power_Down_Lane2    :1;
        CBIOS_U32    TX_Reg_Power_Down_Lane3    :1;
        CBIOS_U32    Driver_Control_Lane0    :1;
        CBIOS_U32    Driver_Control_Lane1    :1;
        CBIOS_U32    Driver_Control_Lane2    :1;
        CBIOS_U32    Driver_Control_Lane3    :1;
        CBIOS_U32    EPHY1_SR_MAN_L0    :1;
        CBIOS_U32    EPHY1_SR_MAN_L1    :1;
        CBIOS_U32    EPHY1_SR_MAN_L2    :1;
        CBIOS_U32    EPHY1_SR_MAN_L3    :1;
        CBIOS_U32    DIU_EPHY1_AUX_DIAJ    :3;
        CBIOS_U32    EPHY_MPLL_CP    :1;
        CBIOS_U32    TX_Power_Control_Lane0    :2;
        CBIOS_U32    TX_Power_Control_Lane1    :2;
        CBIOS_U32    TX_Power_Control_Lane2    :2;
        CBIOS_U32    TX_Power_Control_Lane3    :2;
    };
}REG_MM8344;


typedef union _REG_MM8348    //DP_EPHY_Miscellaneous_Power_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Driver_Mode    :1;
        CBIOS_U32    Reserved    :1;
        CBIOS_U32    RTNBIST    :2;
        CBIOS_U32    CKHLD    :2;
        CBIOS_U32    M1V    :1;
        CBIOS_U32    MT    :1;
        CBIOS_U32    T1V    :1;
        CBIOS_U32    TT    :1;
        CBIOS_U32    EPHY1_TPLL_CP    :4;
        CBIOS_U32    AUC_Ch_Op_Mode    :2;
        CBIOS_U32    TX_High_Impedance_Lane0    :1;
        CBIOS_U32    TX_High_Impedance_Lane1    :1;
        CBIOS_U32    TX_High_Impedance_Lane2    :1;
        CBIOS_U32    TX_High_Impedance_Lane3    :1;
        CBIOS_U32    HPD_Power_Down    :1;
        CBIOS_U32    TPLL_Reset_Signal    :1;
        CBIOS_U32    MPLL_SSC_Output    :4;
        CBIOS_U32    TPLL_Lock_Indicator    :1;
        CBIOS_U32    MPLL_Lock_Indicator    :1;
        CBIOS_U32    RTN_Results    :4;
    };
}REG_MM8348;

typedef union _REG_MM834C    //HDAUDIO_CODEC1_Channel_Count_and_ELD_DIP_Buffer_Size_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Converter_Channel_Count    :8;
        CBIOS_U32    Byte_Offset_into_ELD_memory    :8;
        CBIOS_U32    ELD_Buffer_Size    :8;
        CBIOS_U32    DIP_packet_buffer_size    :3;
        CBIOS_U32    DIP_PI_base_size    :5;
    };
}REG_MM834C;


typedef union _REG_MM8350    //HDAUDIO_CODEC1_ASP_Channel_Map_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    ASP_Channel_Map_Slot_0    :4;
        CBIOS_U32    ASP_Channel_Map_Slot_1    :4;
        CBIOS_U32    ASP_Channel_Map_Slot_2    :4;
        CBIOS_U32    ASP_Channel_Map_Slot_3    :4;
        CBIOS_U32    ASP_Channel_Map_Slot_4    :4;
        CBIOS_U32    ASP_Channel_Map_Slot_5    :4;
        CBIOS_U32    ASP_Channel_Map_Slot_6    :4;
        CBIOS_U32    ASP_Channel_Map_Slot_7    :4;
    };
}REG_MM8350;


typedef union _REG_MM8354    //HDAUDIO_CODEC1_DIP_Transmit_and_CP_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DIP_XmitCtrl_PI0    :2;
        CBIOS_U32    DIP_XmitCtrl_PI1    :2;
        CBIOS_U32    DIP_XmitCtrl_PI2    :2;
        CBIOS_U32    DIP_XmitCtrl_PI3    :2;
        CBIOS_U32    DIP_XmitCtrl_PI4    :2;
        CBIOS_U32    DIP_XmitCtrl_PI5    :2;
        CBIOS_U32    DIP_XmitCtrl_PI6    :2;
        CBIOS_U32    DIP_XmitCtrl_PI7    :2;
        CBIOS_U32    CP_Control_Requested_State    :2;
        CBIOS_U32    CP_Control_0    :1;
        CBIOS_U32    CP_Control_UR_subtag    :5;
        CBIOS_U32    One_Bit_Audio_En    :1;
        CBIOS_U32    One_Bit_AUdio_Mapping    :1;
        CBIOS_U32    ELD_Rd_Status_Control    :1;
        CBIOS_U32    DIP_Write_Status_Control    :1;
        CBIOS_U32    DIP_Read_Status_Control    :1;
        CBIOS_U32    Reserved    :3;
    };
}REG_MM8354;


typedef union _REG_MM8358    //DP_Extension_Packet_Payload_Bytes_3-0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Extension_Packet_Payload_Bytes_3_0    :32;
    };
}REG_MM8358;


typedef union _REG_MM835C    //DP_Extension_Packet_Payload_Bytes_7-4_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Extension_Packet_Payload_Bytes_7_4    :32;
    };
}REG_MM835C;


typedef union _REG_MM8360    //DP_Extension_Packet_Payload_Bytes_11-8_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Extension_Packet_Payload_Bytes_11_8    :32;
    };
}REG_MM8360;


typedef union _REG_MM8364    //DP_Extension_Packet_Payload_Bytes_15-12_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Extension_Packet_Payload_Bytes_15_12    :32;
    };
}REG_MM8364;

typedef union _REG_MM8368    //DP
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DP1_SW_swing    :6;
        CBIOS_U32    DP1_SW_pp    :5;
        CBIOS_U32    DP1_SW_post_cursor    :4;
        CBIOS_U32    SW_swing_SW_PP_SW_post_cursor_load_index    :6;
        CBIOS_U32    enable_SW_swing_pp    :1;
        CBIOS_U32    PSR_ML_AUTOOFF    :1;
        CBIOS_U32    VER1P2    :1;
        CBIOS_U32    RD_INTVAL    :3;
        CBIOS_U32    PSR_UPDATE    :1;
        CBIOS_U32    PSR_EXIT    :1;
        CBIOS_U32    PSR_ENTER    :1;
        CBIOS_U32    PSR_FORCE_BS    :1;
        CBIOS_U32    bugfix_en    :1;
    };
}REG_MM8368;


typedef union _REG_MM836C    //DP
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    EPHY_lane3_pre_pp    :2;
        CBIOS_U32    EPHY_lane2_pre_pp    :2;
        CBIOS_U32    EPHY_lane1_pre_pp    :2;
        CBIOS_U32    EPHY_lane0_pre_pp    :2;
        CBIOS_U32    EPHY_lane3_swing_level    :2;
        CBIOS_U32    EPHY_lane2_swing_level    :2;
        CBIOS_U32    EPHY_lane1_swing_level    :2;
        CBIOS_U32    EPHY_lane0_swing_level    :2;
        CBIOS_U32    EPHY_bit_rate    :1;
        CBIOS_U32    EPHY_HPD_IN    :1;
        CBIOS_U32    EPHY1_TPLL_ISEL    :2;
        CBIOS_U32    MR    :3;
        CBIOS_U32    MC    :3;
        CBIOS_U32    TR    :3;
        CBIOS_U32    TC    :3;
    };
}REG_MM836C;


typedef union _REG_MM8370    //DP_HDCP_Key_Selection_Vector_(KSV)_1_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    KSV_39to8    :32;
    };
}REG_MM8370;


typedef union _REG_MM8374    //DP_HDCP_Control_1_and_Key_Selection_Vector_(KSV)_2_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    KSV_7to0    :8;
        CBIOS_U32    Key_ECC_Done    :1;
        CBIOS_U32    DP_HDCP_INT    :1;
        CBIOS_U32    Key_Error_detect    :1;
        CBIOS_U32    Key_Error_correct    :1;
        CBIOS_U32    Reserved_14to12    :3;
        CBIOS_U32    DPHDCP_Test_Mode_Select    :1;
        CBIOS_U32    Test_Key_Enable    :1;
        CBIOS_U32    AUX_Fail_Config    :3;
        CBIOS_U32    AUX_Def_Config    :2;
        CBIOS_U32    Notfinish_Fail    :1;
        CBIOS_U32    Disable_AUX    :1;
        CBIOS_U32    Not_support_0_KSV_repeater    :1;
        CBIOS_U32    Reserved_26to25    :2;
        CBIOS_U32    CP_EN    :1;
        CBIOS_U32    Enc_Sel    :1;
        CBIOS_U32    Enc_Con    :1;
        CBIOS_U32    KSV_Rev_List    :1;
        CBIOS_U32    KSV_Verifcation_Done    :1;
    };
}REG_MM8374;


typedef union _REG_MM8378    //DP_HDCP_Control_2_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Authentication_Protocol_Status    :2;
        CBIOS_U32    Repeater_Flag    :1;
        CBIOS_U32    Device_Count    :7;
        CBIOS_U32    Interrupt_Source    :4;
        CBIOS_U32    AUX_Status    :2;
        CBIOS_U32    Reserved    :16;
    };
}REG_MM8378;


typedef union _REG_MM837C    //HDAUDIO_CODEC1_Control_Writes_Default_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Load_Converter1_StreamFormat    :1;
        CBIOS_U32    Load_Converter1_ChanStreamID    :1;
        CBIOS_U32    Load_Converter1_DigiConvert    :1;
        CBIOS_U32    Load_Pinwidget1_UnsoliResponse    :1;
        CBIOS_U32    Load_Converter1_StripeControl    :1;
        CBIOS_U32    Load_Pinwidget1_PinSense    :1;
        CBIOS_U32    Load_Pinwidget1_Control    :1;
        CBIOS_U32    Load_Pinwidget1_ConfigDefault    :1;
        CBIOS_U32    Load_Convert1_Converter_Channel_Count    :1;
        CBIOS_U32    Load_ASP_Channel_Mapping    :1;
        CBIOS_U32    Load_ELD_Offset    :1;
        CBIOS_U32    Load_DIP_XmitCtrl    :1;
        CBIOS_U32    Load_CP_Control    :1;
        CBIOS_U32    Load_One_Bit_audio_Control    :1;
        CBIOS_U32    Reserved    :18;
    };
}REG_MM837C;


typedef union _REG_MM8380    //HDAUDIO_CODEC1_Read_Out_Control_Select_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Load_Function_Group_SubsystemID    :1;
        CBIOS_U32    Load_Function_Group_Power_state    :1;
        CBIOS_U32    SW_Strm1_FIFO_Depth_Select    :1;
        CBIOS_U32    SW_Strm1_FIFO_Depth    :5;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    Reserved_1    :5;
        CBIOS_U32    Strm1_Link_Position_Select    :1;
        CBIOS_U32    Reserved_2    :1;
        CBIOS_U32    HDA_Offset_84_PRS    :1;
        CBIOS_U32    IMM_CMD_Response_V_SEL    :1;
        CBIOS_U32    Reserved_3    :1;
        CBIOS_U32    HDAUDIO_Wall_Clock_Select    :2;
        CBIOS_U32    Wal_Clk_Cnt_Sel    :1;
        CBIOS_U32    Wal_Clk_Cnt_Clock_Sel1    :1;
        CBIOS_U32    Wal_Clk_Cnt_Clock_Sel2    :1;
        CBIOS_U32    Read_Out_Control_Select    :8;
    };
}REG_MM8380;


typedef union _REG_MM8384    //HDAUDIO_CODEC1_Read_Out_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Codec_Read_Out    :32;
    };
}REG_MM8384;


typedef union _REG_MM8388    //HDAUDIO_Wall_Clock_Ratio_Low_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Wall_clock_ratio_low    :32;
    };
}REG_MM8388;


typedef union _REG_MM838C    //HDAUDIO_Wall_Clock_Ratio_Hi_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Wall_clock__ratio__hi    :8;
        CBIOS_U32    Wall_clock_ratio_enable    :1;
        CBIOS_U32    Reserved    :23;
    };
}REG_MM838C;


typedef union _REG_MM8390    //HDAUDIO_CODEC1_Channel_Status_Bits_31:0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    bit_0    :1;
        CBIOS_U32    bit_1    :1;
        CBIOS_U32    Channel_status_31_2    :30;
    };
}REG_MM8390;


typedef union _REG_MM8394    //HDAUDIO_CODEC1_Channel_Status_Bits_63:32_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Channel_status_63_32    :32;
    };
}REG_MM8394;


typedef union _REG_MM8398    //HDAUDIO_CODEC1_Channel_Status_Bits_95:64_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Channel_status_95_64    :32;
    };
}REG_MM8398;


typedef union _REG_MM839C    //HDAUDIO_CODEC1_Channel_Status_Block_Bits_127:96_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Channel_status_127_96    :32;
    };
}REG_MM839C;


typedef union _REG_MM83A0    //HDAUDIO_CODEC1_Channel_Status_Bits_159:128_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Channel_status_159_128    :32;
    };
}REG_MM83A0;


typedef union _REG_MM83A4    //HDAUDIO_CODEC1_Channel_Status_Bits_191:160_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Channel_status_191_160    :32;
    };
}REG_MM83A4;


typedef union _REG_MM83A8    //HDMI1_CODEC1_Audio_Clock_Regeneration_Numerator_(N)_and_CTS_Low_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    N    :20;
        CBIOS_U32    CTS    :12;
    };
}REG_MM83A8;


typedef union _REG_MM83AC    //HDMI_AUDIO_CODEC1_Audio_Clock_Cycle_Time_Stamp_(CTS)__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CTS    :8;
        CBIOS_U32    HW_Generated_CTS    :20;
        CBIOS_U32    Reserved    :2;
        CBIOS_U32    CTS_Select    :1;
        CBIOS_U32    HW_CTS_MODE    :1;
    };
}REG_MM83AC;


typedef union _REG_MM8400    //Primary_Stream_2_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_Blank_Alpha    :8;
        CBIOS_U32    PS2_YCBCR_Mode    :1;
        CBIOS_U32    PS2_YBCR420_Enable    :1;
        CBIOS_U32    PS2_Reserve_Pre_Multiple_Alpha    :1;
        CBIOS_U32    PS2_Reserve_Pre_Multiple_use_Invert_Alpha    :1;
        CBIOS_U32    PS2_fifo_depth_control    :3;
        CBIOS_U32    Ps2_Uyvy422    :1;
        CBIOS_U32    RESERVED    :16;
    };
}REG_MM8400;


typedef union _REG_MM8404    //Primary_Stream_3_Frame_Buffer_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_Enable_Work_Registers_0    :1;
        CBIOS_U32    SS3_Enable_Work_Registers    :1;
        CBIOS_U32    PS3_Start_Address_0    :28;
        CBIOS_U32    PS3_Vsync_Off_Page_Flip    :1;
        CBIOS_U32    PS3_Enable_Work_Registers_1    :1;
    };
}REG_MM8404;


typedef union _REG_MM8408    //Primary_Stream_3_Stride_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RESERVED_0    :1;
        CBIOS_U32    PS3_ABGR_EN    :1;
        CBIOS_U32    PS3_First_Line_Swizzle    :1;
        CBIOS_U32    PS3_Work_Reg_Enable    :1;
        CBIOS_U32    PS3_Stride_or_Tile_Stride    :12;
        CBIOS_U32    PS3_Start_Address_Pixel_Offset    :5;
        CBIOS_U32    PS3_Tiling_X_Offset    :3;
        CBIOS_U32    PS3_Tiling_Line_Offset    :6;
        CBIOS_U32    RESERVED_1    :1;
        CBIOS_U32    PS3_Tile_Addressing_Enable    :1;
    };
}REG_MM8408;


typedef union _REG_MM840C    //interlace_right_offset[31:2]
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    reserved    :2;
        CBIOS_U32    PS2_INTR_ROFFSET_31_2    :30;
    };
}REG_MM840C;


typedef union _REG_MM8410    //Primary_Stream_3_Right_Frame_Offset_Address_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_Right_Frame_Base    :30;
        CBIOS_U32    RESERVED    :2;
    };
}REG_MM8410;


typedef union _REG_MM8414    //Primary_Stream_3_YCbCr420_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_Blank_Alpha    :8;
        CBIOS_U32    PS3_YCBCR_Mode    :1;
        CBIOS_U32    PS3_YBCR420_Enable    :1;
        CBIOS_U32    PS3_Reserve_Pre_Multiple_Alpha    :1;
        CBIOS_U32    PS3_Reserve_Pre_Multiple_use_Invert_Alpha    :1;
        CBIOS_U32    PS3_fifo_depth_control    :3;
        CBIOS_U32    Ps3_Uyvy422    :1;
        CBIOS_U32    RESERVED    :16;
    };
}REG_MM8414;


typedef union _REG_MM8418    //Primary_Stream_1_YCbCr420_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_Blank_Alpha    :8;
        CBIOS_U32    PS1_YCBCR_Mode    :1; //0=Crycb_Mode_When_Ycbcr444 1=Ycbcr_Mode_When_Ycbcr444
        CBIOS_U32    PS1_YBCR420_Enable    :1;
        CBIOS_U32    PS1_Reserve_Pre_Multiple_Alpha    :1;
        CBIOS_U32    PS1_Reserve_Pre_Multiple_use_Invert_Alpha    :1;
        CBIOS_U32    PS1_fifo_depth_control    :3;
        CBIOS_U32    Ps1_Uyvy422    :1;
        CBIOS_U32    RESERVED    :16;
    };
}REG_MM8418;


typedef union _REG_MM841C    //Secondary_Stream_1_control_Regsiter
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_Plane_Alpha    :8;
        CBIOS_U32    SS1_Alpha_Blend_Mode    :1;
        CBIOS_U32    SS1_Sor_Des_Select    :1;
        CBIOS_U32    SS1_YCbCr_Mode    :1;
        CBIOS_U32    SS1_YCbCr420_Enable    :1;
        CBIOS_U32    SS1_fifo_depth_control    :3;
        CBIOS_U32    RESERVED    :15;
        CBIOS_U32    SS1_enable    :1;
        CBIOS_U32    SS1_enable_use_mmio    :1;
    };
}REG_MM841C;


typedef union _REG_MM8420    //Primary_Stream_3_Display_Position_Frame_Line_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_Frame_Counter    :16;
        CBIOS_U32    PS3_Line_Counter    :16;
    };
}REG_MM8420;


typedef union _REG_MM8424    //Primary_Stream_1_Display_Position_Line_Pixel_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_Line_Counter    :16;
        CBIOS_U32    PS3_Pixel_Counter    :16;
    };
}REG_MM8424;


typedef union _REG_MM8428    //Primary_Stream_1_Top_Left_Pixel_Line_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_Top_Left_Line    :16;
        CBIOS_U32    PS3_Top_Leftt_Pixel    :16;
    };
}REG_MM8428;


typedef union _REG_MM842C    //Primary_Stream_1_Bottom_Right_Pixel_Line_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_Bottom_Right_Line    :16;
        CBIOS_U32    PS3_Bottom_Right_Pixel    :16;
    };
}REG_MM842C;


typedef union _REG_MM8430    //Secondary_Stream_3_Color/Chroma_8bit_Key_Lower_Bound_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_8bit_B_V_Cr_Key_Lower_Bound    :8;
        CBIOS_U32    SS3_8bit_G_U_Cb_Key_Lower_Bound    :8;
        CBIOS_U32    SS3_8bit_R_Y_Key_Lower_Bound    :8;
        CBIOS_U32    Reserved_8bit    :8;
    };
    struct
    {
        CBIOS_U32    SS3_10bit_B_V_Cr_Key_Lower_Bound    :10;
        CBIOS_U32    SS3_10bit_G_U_Cb_Key_Lower_Bound    :10;
        CBIOS_U32    SS3_10bit_R_Y_Key_Lower_Bound    :10;
        CBIOS_U32    RESERVED_10bit    :2;
    };
}REG_MM8430;


typedef union _REG_MM8434    //Streams_3_Blend_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ka3_3to0_or_Ks3    :4;
        CBIOS_U32    Ka3_7to4_or_Kp3    :4;
        CBIOS_U32    SS3_Input_Format    :3;
        CBIOS_U32    SS3_YCbCr_order    :1;
        CBIOS_U32    RESERVED    :20;
    };
}REG_MM8434;


typedef union _REG_MM8438    //Secondary_Stream_1_Chroma_Key_Upper_Bound_and_Key_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_V_Cr_Key_Upper_Bound    :8;
        CBIOS_U32    SS3_U_Cb_Key_Upper_Bound    :8;
        CBIOS_U32    SS3_Y_Key_Upper_Bound    :8;
        CBIOS_U32    Keying_Mode    :4;
        CBIOS_U32    Invert_Alpha3_or_Ka3    :1;
        CBIOS_U32    RESERVED    :3;
    };
}REG_MM8438;


typedef union _REG_MM843C    //Secondary_Stream_3_Source_Height_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_Line_Height    :13;
        CBIOS_U32    SS3_Buffer_Select    :1;
        CBIOS_U32    SS3_Double_Buffering_Select    :1;
        CBIOS_U32    SS3_Triple_Buffering_Enable    :1;
        CBIOS_U32    SS3_Source_Line_Width    :13;
        CBIOS_U32    RESERVED    :2;
        CBIOS_U32    RESEREED    :1;
    };
}REG_MM843C;


typedef union _REG_MM8440    //Secondary_Stream_3_Frame_Buffer_Start_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RESERVED    :3;
        CBIOS_U32    SS3_FB_Start_Address_0    :27;
        CBIOS_U32    SS3_VSYNC_Off_Page_Flip    :1;
        CBIOS_U32    SS3_Work_Reg_En    :1;
    };
}REG_MM8440;


typedef union _REG_MM8444    //Secondary_Stream_3_ROFFSET_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RESERVED_0    :3;
        CBIOS_U32    SS3_Right_Base_Address    :27;
        CBIOS_U32    RESERVED_1    :2;
    };
}REG_MM8444;


typedef union _REG_MM8448    //Secondary_Stream_3_Stride_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_Read_Length    :2;
        CBIOS_U32    RESERVED    :2;
        CBIOS_U32    SS3_Stride    :12;
        CBIOS_U32    SS3_Start_Address_Byte_Offset    :6;
        CBIOS_U32    SS3_Tiling_X_Offset    :3;
        CBIOS_U32    SS3_Tiling_Line_Offset    :6;
        CBIOS_U32    SS3_ABGR_EN    :1;
    };
}REG_MM8448;


typedef union _REG_MM844C    //interlace_right_offset[31:2]
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    reserved    :2;
        CBIOS_U32    PS3_INTR_ROFFSET_31_2    :30;
    };
}REG_MM844C;


typedef union _REG_MM8450    //Secondary_Stream_3_Window_Start_Coordinates_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_Y_Start    :13;
        CBIOS_U32    RESERVED_0    :3;
        CBIOS_U32    SS3_X_Start_0    :12;
        CBIOS_U32    SS3_Loading_Enable    :1;
        CBIOS_U32    SS3_X_Start_1    :1;
        CBIOS_U32    RESERVED_1    :2;
    };
}REG_MM8450;


typedef union _REG_MM8454    //Secondary_Stream_3_Window_Size_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_Window_Height    :13;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    SS3_First_Line_swizzle    :1;
        CBIOS_U32    SS3_Window_Width    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM8454;


typedef union _REG_MM8458    //Secondary_Stream_3_Horizontal_Scaling_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_HACC    :21;
        CBIOS_U32    SS3_HTAP_SEL    :1;
        CBIOS_U32    RESERVED    :10;
    };
}REG_MM8458;


typedef union _REG_MM845C    //Secondary_Stream_3_Vertical_Scaling_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_VACC    :21;
        CBIOS_U32    SS3_Vscale_Selection    :1;
        CBIOS_U32    SS3_VDUP    :1;
        CBIOS_U32    RESERVED    :8;
        CBIOS_U32    SS3_Deinterlacing    :1;
    };
}REG_MM845C;


typedef union _REG_MM8460    //Secondary_Stream_3_Frame_Buffer_Start_Address_1_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    SS3_FB_Start_Address_1    :27;
        CBIOS_U32    Reserved_1    :2;
    };
}REG_MM8460;


typedef union _REG_MM8464    //Secondary_Stream_3_Frame_Buffer_Start_Address_2_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RESERVED_0    :3;
        CBIOS_U32    SS3_FB_Start_Address_2    :27;
        CBIOS_U32    RESERVED_1    :2;
    };
}REG_MM8464;


typedef union _REG_MM8468    //Secondary_Stream_3_Control__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_Plane_Alpha    :8;
        CBIOS_U32    SS3_Alpha_Blend_Mode    :1;
        CBIOS_U32    SS3_Sor_Des_Select    :1;
        CBIOS_U32    SS3_YCbCr_Mode    :1;
        CBIOS_U32    SS3_YCbCr420_Enable    :1;
        CBIOS_U32    SS3_fifo_depth_control    :3;
        CBIOS_U32    RESERVED    :15;
        CBIOS_U32    SS3_enable    :1;
        CBIOS_U32    SS3_enable_use_mmio    :1;
    };
}REG_MM8468;


typedef union _REG_MM846C    //Secondary_Stream_2_Control__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_Plane_Alpha    :8;
        CBIOS_U32    SS2_Alpha_Blend_Mode    :1;
        CBIOS_U32    SS2_Sor_Des_Select    :1;
        CBIOS_U32    SS2_YCbCr_Mode    :1;
        CBIOS_U32    SS2_YCbCr420_Enable    :1;
        CBIOS_U32    SS2_fifo_depth_control    :3;
        CBIOS_U32    RESERVED    :15;
        CBIOS_U32    SS2_enable    :1;
        CBIOS_U32    SS2_enable_use_mmio    :1;
    };
}REG_MM846C;


typedef union _REG_MM8470    //Primary_Stream_1_CSC_Format__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_CSC_input_Format    :3;
        CBIOS_U32    reserved    :1;
        CBIOS_U32    PS1_CSC_output_Format    :3;
        CBIOS_U32    Programmable    :1;
        CBIOS_U32    PS1_disable    :1;
        CBIOS_U32    PS1_L1_bit11    :1;
        CBIOS_U32    PS1_3D_Video_Mode    :3;
        CBIOS_U32    PS1_Read_Length    :2;
        CBIOS_U32    Enable_PS1_L1    :1;
        CBIOS_U32    PS1_L1_7to0    :8;
        CBIOS_U32    PS1_L1_10to8    :3;
        CBIOS_U32    DAC1_Color_Mode    :3;
        CBIOS_U32    use_L_R_or_ODD_EVEN_to_do_2_frame_page_flip    :1;
        CBIOS_U32    enable_page_flip_every_2_frame_for_3D_video    :1;
    };
}REG_MM8470;


typedef union _REG_MM8474    //Primary_Stream_1_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_F1    :14;
        CBIOS_U32    PS1_F2    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM8474;


typedef union _REG_MM8478    //Primary_Stream_1_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_F3    :14;
        CBIOS_U32    PS1_F4    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM8478;


typedef union _REG_MM847C    //Primary_Stream_1_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_F5    :14;
        CBIOS_U32    PS1_F6    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM847C;


typedef union _REG_MM8480    //Primary_Stream_1_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_F7    :14;
        CBIOS_U32    PS1_F8    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM8480;


typedef union _REG_MM8484    //Primary_Stream_1_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_F9    :14;
        CBIOS_U32    PS1_Bright    :9;
        CBIOS_U32    Reserved    :9;
    };
}REG_MM8484;


typedef union _REG_MM8488    //Primary_Stream_2_CSC_Format__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_CSC_input_Format    :3;
        CBIOS_U32    RESERVED    :1;
        CBIOS_U32    PS2_CSC_output_Format    :3;
        CBIOS_U32    Programmable    :1;
        CBIOS_U32    PS2_disable    :1;
        CBIOS_U32    PS2_L1_bit11    :1;
        CBIOS_U32    PS2_3D_Video_Mode    :3;
        CBIOS_U32    PS2_Read_Length    :2;
        CBIOS_U32    Enable_PS2_L1    :1;
        CBIOS_U32    PS2_L1_7to0    :8;
        CBIOS_U32    PS2_L1_10to8    :3;
        CBIOS_U32    DAC2_Color_Mode    :3;
        CBIOS_U32    use_L_R_or_ODD_EVEN_to_do_2_frame_page_flip    :1;
        CBIOS_U32    enable_page_flip_every_2_frame_for_3D_video    :1;
    };
}REG_MM8488;


typedef union _REG_MM848C    //Primary_Stream_2_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_F1    :14;
        CBIOS_U32    PS2_F2    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM848C;


typedef union _REG_MM8490    //Primary_Stream_2_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_F3    :14;
        CBIOS_U32    PS2_F4    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM8490;


typedef union _REG_MM8494    //Primary_Stream_2_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_F5    :14;
        CBIOS_U32    PS2_F6    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM8494;


typedef union _REG_MM8498    //Primary_Stream_2_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_F7    :14;
        CBIOS_U32    PS2_F8    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM8498;


typedef union _REG_MM849C    //Primary_Stream_2_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_F9    :14;
        CBIOS_U32    PS2_Bright    :9;
        CBIOS_U32    Reserved    :9;
    };
}REG_MM849C;


typedef union _REG_MM84A0    //Primary_Stream_3_CSC_Format__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_CSC_input_Format    :3;
        CBIOS_U32    RESERVED    :1;
        CBIOS_U32    PS3_CSC_output_Format    :3;
        CBIOS_U32    Programmable    :1;
        CBIOS_U32    PS3_disable    :1;
        CBIOS_U32    PS3_L1_bit11    :1;
        CBIOS_U32    PS3_3D_Video_Mode    :3;
        CBIOS_U32    PS3_Read_Length    :2;
        CBIOS_U32    Enable_PS3_L1    :1;
        CBIOS_U32    PS3_L1_7to0    :8;
        CBIOS_U32    PS3_L1_10to8    :3;
        CBIOS_U32    DAC3_Color_Mode    :3;
        CBIOS_U32    use_L_R_or_ODD_EVEN_to_do_2_frame_page_flip    :1;
        CBIOS_U32    enable_page_flip_every_2_frame_for_3D_video    :1;
    };
}REG_MM84A0;


typedef union _REG_MM84A4    //Primary_Stream_3_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_F1    :14;
        CBIOS_U32    PS3_F2    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84A4;


typedef union _REG_MM84A8    //Primary_Stream_3_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_F3    :14;
        CBIOS_U32    PS3_F4    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84A8;


typedef union _REG_MM84AC    //Primary_Stream_3_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_F5    :14;
        CBIOS_U32    PS3_F6    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84AC;


typedef union _REG_MM84B0    //Primary_Stream_3_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_F7    :14;
        CBIOS_U32    PS3_F8    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84B0;


typedef union _REG_MM84B4    //Primary_Stream_3_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_F9    :14;
        CBIOS_U32    PS3_Bright    :9;
        CBIOS_U32    Reserved    :9;
    };
}REG_MM84B4;


typedef union _REG_MM84B8    //Secondary_Stream_1_CSC_Format__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_CSC_input_Format    :3;
        CBIOS_U32    RESERVED_0    :1;
        CBIOS_U32    SS1_CSC_output_Format    :3;
        CBIOS_U32    Programmable    :1;
        CBIOS_U32    RESERVED_1    :8;
        CBIOS_U32    SS1_Reverse_Pre_Mulitple    :1;
        CBIOS_U32    SS1_Invert_Alpha    :1;
        CBIOS_U32    RESERVED_2    :14;
    };
}REG_MM84B8;


typedef union _REG_MM84BC    //Secondary_Stream_1_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_F1    :14;
        CBIOS_U32    SS1_F2    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84BC;


typedef union _REG_MM84C0    //Secondary_Stream_1_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_F3    :14;
        CBIOS_U32    SS1_F4    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84C0;


typedef union _REG_MM84C4    //Secondary_Stream_1_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_F5    :14;
        CBIOS_U32    SS1_F6    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84C4;


typedef union _REG_MM84C8    //Secondary_Stream_1_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_F7    :14;
        CBIOS_U32    SS1_F8    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84C8;


typedef union _REG_MM84CC    //Secondary_Stream_1_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_F9    :14;
        CBIOS_U32    SS1_Bright    :9;
        CBIOS_U32    Reserved    :9;
    };
}REG_MM84CC;


typedef union _REG_MM84D0    //Secondary_Stream_2_CSC_Format__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_CSC_input_Format    :3;
        CBIOS_U32    RESERVED_0    :1;
        CBIOS_U32    SS2_CSC_output_Format    :3;
        CBIOS_U32    Programmable    :1;
        CBIOS_U32    RESERVED_1    :8;
        CBIOS_U32    SS2_Reverse_Pre_Mulitple    :1;
        CBIOS_U32    SS2_Invert_Alpha    :1;
        CBIOS_U32    RESERVED_2    :14;
    };
}REG_MM84D0;


typedef union _REG_MM84D4    //Secondary_Stream_2_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_F1    :14;
        CBIOS_U32    SS2_F2    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84D4;


typedef union _REG_MM84D8    //Secondary_Stream_2_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_F3    :14;
        CBIOS_U32    SS2_F4    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84D8;


typedef union _REG_MM84DC    //Secondary_Stream_2_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_F5    :14;
        CBIOS_U32    SS2_F6    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84DC;


typedef union _REG_MM84E0    //Secondary_Stream_2_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_F7    :14;
        CBIOS_U32    SS2_F8    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84E0;


typedef union _REG_MM84E4    //Secondary_Stream_2_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS2_F9    :14;
        CBIOS_U32    SS2_Bright    :9;
        CBIOS_U32    Reserved    :9;
    };
}REG_MM84E4;


typedef union _REG_MM84E8    //Secondary_Stream_3_CSC_Format__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_CSC_input_Format    :3;
        CBIOS_U32    RESERVED_0    :1;
        CBIOS_U32    SS3_CSC_output_Format    :3;
        CBIOS_U32    Programmable    :1;
        CBIOS_U32    RESERVED_1    :8;
        CBIOS_U32    SS3_Reverse_Pre_Mulitple    :1;
        CBIOS_U32    SS3_Invert_Alpha    :1;
        CBIOS_U32    RESERVED_2    :14;
    };
}REG_MM84E8;


typedef union _REG_MM84EC    //Secondary_Stream_3_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_F1    :14;
        CBIOS_U32    SS3_F2    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84EC;


typedef union _REG_MM84F0    //Secondary_Stream_3_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_F3    :14;
        CBIOS_U32    SS3_F4    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84F0;


typedef union _REG_MM84F4    //Secondary_Stream_3_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_F5    :14;
        CBIOS_U32    SS3_F6    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84F4;


typedef union _REG_MM84F8    //Secondary_Stream_3_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_F7    :14;
        CBIOS_U32    SS3_F8    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM84F8;


typedef union _REG_MM84FC    //Secondary_Stream_3_CSC_Coefficient_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_F9    :14;
        CBIOS_U32    SS3_Bright    :9;
        CBIOS_U32    Reserved    :9;
    };
}REG_MM84FC;

typedef union _REG_MM334C8    //DP1_control_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LINK_QUAL_SET_EN    :1;
        CBIOS_U32    LINK_QUAL_SET_LANE    :3;
        CBIOS_U32    EQ_USE_TP3    :1;
        CBIOS_U32    Start_LINK_RATE_1    :1;
        CBIOS_U32    SW_bit_rate_1    :1;
        CBIOS_U32    IDLE_FIFO_INFO_SEND_EN    :1;
        CBIOS_U32    DPCD102_B5    :1;
        CBIOS_U32    Aux_time_Reduced    :1;
        CBIOS_U32    DUR_HTOTAL_15    :1;
        CBIOS_U32    DUR_HBLANK_12    :1;
        CBIOS_U32    DP_SUPPORT_POST_CURSOR    :1;
        CBIOS_U32    MAX_POST_EMPHASIS    :2;
        CBIOS_U32    EQ_WRITE_POST    :1;
        CBIOS_U32    HBR2_COMPLIANCE_SCRAMBLER_RESET    :16;
    };
}REG_MM334C8;


typedef union _REG_MM334CC    //DP1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SW_MPLL_M_1    :8;
        CBIOS_U32    SW_MPLL_P_1    :2;
        CBIOS_U32    SW_NX_P    :2;
        CBIOS_U32    SW_NX_P_1    :2;
        CBIOS_U32    SW_NX_P_2    :2;
        CBIOS_U32    SW_MPLL_M_2    :8;
        CBIOS_U32    SW_MPLL_P_2    :2;
        CBIOS_U32    DP_VERSION_VALUE    :6;
    };
}REG_MM334CC;


typedef union _REG_MM334DC    //DP1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Link_Rate_value_1    :8;
        CBIOS_U32    Link_Rate_value_2    :8;
        CBIOS_U32    Link_Rate_value_3    :8;
        CBIOS_U32    VCS_SUPPORT_FIELD_3D    :1;
        CBIOS_U32    VCS_FIELD_INV    :1;
        CBIOS_U32    LINK_bit_rate_status_1_0    :2;
        CBIOS_U32    RESEVERD    :2;
        CBIOS_U32    HBR_M_GEN_MOD    :1;
        CBIOS_U32    Link_Rate_use_DPCP115    :1;
    };
}REG_MM334DC;


typedef union _REG_MM334E0    //DP1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    EPHY1_FBOOST    :3;
        CBIOS_U32    EPHY1_HDCKBY4    :1;
        CBIOS_U32    EPHY1_FBOOST_1    :3;
        CBIOS_U32    EPHY1_FBOOST_2    :3;
        CBIOS_U32    reseverd_0    :6;
        CBIOS_U32    reseverd_1    :4;
        CBIOS_U32    EPHY1_SR_SPD    :2;
        CBIOS_U32    EPHY1_SR_DLY    :2;
        CBIOS_U32    EPHY1_SR_NDLY    :2;
        CBIOS_U32    Reserved_30to26    :5;
        CBIOS_U32    DP1_HPD_INV    :1;
    };
}REG_MM334E0;


typedef union _REG_MM334E4    //DP1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    EPHY1_TXDU_L0    :6;
        CBIOS_U32    EPHY1_TXDU_L1    :6;
        CBIOS_U32    EPHY1_TXDU_L2    :6;
        CBIOS_U32    EPHY1_TXDU_L3    :6;
        CBIOS_U32    EPHY1_TX_VMR    :4;
        CBIOS_U32    EPHY1_TX_VMX    :1;
        CBIOS_U32    EPHY1_TX_H1V2    :1;
        CBIOS_U32    Reserved_31to30    :2;
    };
}REG_MM334E4;


typedef union _REG_MM33680    //HDMI1_HDCP22_control_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    STREAM_ID    :8;
        CBIOS_U32    STREAM_TYPE    :8;
        CBIOS_U32    RNG_OSC_MD    :2;
        CBIOS_U32    RNG_OSC_EN_HDCP    :1;
        CBIOS_U32    RNG_EN_HDCP    :1;
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    HDCP22_AUTH_SEL    :1;
        CBIOS_U32    HDCP22_AUTH_TRIG    :1;
        CBIOS_U32    CSM_TRIGGER    :1;
        CBIOS_U32    AKE_Stored_km_DIS    :1;
        CBIOS_U32    HDCP22_VER_RD_DIS    :1; // disable reading HDCP 2.2 version register, for debug
        CBIOS_U32    TEST_REPEATER    :1;
        CBIOS_U32    TEST_MODE    :1;
        CBIOS_U32    CONT_STREAM_EN    :1;
        CBIOS_U32    HDCP22_CP_EN    :1;
    };
}REG_MM33680;


typedef union _REG_MM33684    //DP1_HDCP22_control_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :23;
        CBIOS_U32    DPHDCP22_AUTH_SEL    :1;
        CBIOS_U32    DPHDCP22_AUTH_TRIG    :1;
        CBIOS_U32    DPHDCP22_CSM_TRIGGER    :1;
        CBIOS_U32    DPHDCP22_AKE_Stored_km_DIS    :1;
        CBIOS_U32    Reserved_1    :2;
        CBIOS_U32    DPHDCP22_CAP_EN    :1;
        CBIOS_U32    DPHDCP22_CONT_STREAM_EN    :1;
        CBIOS_U32    DPHDCP22_CP_EN    :1;
    };
}REG_MM33684;


typedef union _REG_MM33688    //DP1/HDMI1_HDCP22_control_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TxCaps    :24;
        CBIOS_U32    Reserved    :2;
        CBIOS_U32    PGM_OPEN    :1;
        CBIOS_U32    RNG_TEST_FAIL    :1;
        CBIOS_U32    HDCP22_CSM_PASS_DP1    :1;
        CBIOS_U32    HDCP22_AUTH_PASS_DP1    :1;
        CBIOS_U32    HDCP22_CSM_PASS_HDMI1    :1;
        CBIOS_U32    HDCP22_AUTH_PASS_HDMI1    :1;
    };
}REG_MM33688;


typedef union _REG_MM3368C    //DP1/HDMI1_HDCP22_interrupt_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    MSG_ID_ERR_INT    :1;
        CBIOS_U32    RecID_INT    :1;
        CBIOS_U32    KSVFF_INT    :1;
        CBIOS_U32    NOT_HDCP22_INT    :1;
        CBIOS_U32    IS_HDCP22_INT    :1;
        CBIOS_U32    LINK_INT_FAIL    :1;
        CBIOS_U32    CSM_FAIL    :1;
        CBIOS_U32    AUTH_FAIL    :1;
        CBIOS_U32    AUXFAIL_INT_IICFAIL_INT    :1;
        CBIOS_U32    DEV_ZERO    :1;
        CBIOS_U32    CSM_PASS    :1;
        CBIOS_U32    AUTH_PASS    :1;
        CBIOS_U32    SEQ_NUM_M_ROLLOVER    :1;
        CBIOS_U32    M_RETRY_NUMOUT    :1;
        CBIOS_U32    M_TIMEOUT    :1;
        CBIOS_U32    M_FAIL    :1;
        CBIOS_U32    NONZERO_SEQ_NUM_V_INT    :1;
        CBIOS_U32    SEQ_NUM_V_ROLLOVER_INT    :1;
        CBIOS_U32    V_FAIL    :1;
        CBIOS_U32    MAX_CAS    :1;
        CBIOS_U32    MAX_DEVS    :1;
        CBIOS_U32    WAIT_RECID_TIMEOUT    :1;
        CBIOS_U32    REAUTH_REQ    :1;
        CBIOS_U32    L_FAIL    :1;
        CBIOS_U32    LC_RETRY_NUMOUT    :1;
        CBIOS_U32    LC_TIMEOUT    :1;
        CBIOS_U32    Ekh_TIMEOUT    :1;
        CBIOS_U32    H_FAIL    :1;
        CBIOS_U32    H_TIMEOUT    :1;
        CBIOS_U32    CERT_FAIL    :1;
        CBIOS_U32    RECID_INVALID    :1;
        CBIOS_U32    CERT_TIMEOUT    :1;
    };
}REG_MM3368C;


typedef union _REG_MM33694    //hdtv1_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB1_CSC_IN_FMT    :3;
        CBIOS_U32    Reserved_bit4    :1;
        CBIOS_U32    LB1_CSC_OUT_FMT    :3;
        CBIOS_U32    LB1_PROGRAMMBLE    :1;
        CBIOS_U32    LB1_BYPASS    :1;
        CBIOS_U32    Reserved_31to9    :23;
    };
}REG_MM33694;


typedef union _REG_MM336B0    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDMI1_SSCP_UCC_POS    :12;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    HDMI1_SCRAMBLE_ACTIVE_POS_SEL    :1;
        CBIOS_U32    HDMI1_SCRAMBLE_MODE    :1;
        CBIOS_U32    HDMI1_SCRAMBLE_EN    :1;
        CBIOS_U32    HDMI1_YC_420_EN    :1;
        CBIOS_U32    HDMI1_YC_420_MODE    :1;
        CBIOS_U32    HDMI1_CENTRAL_START_MODE    :1;
        CBIOS_U32    HDMI1_CLK_LANE_EN    :1;
        CBIOS_U32    DATA_ISLAND_START_SEL    :1;
        CBIOS_U32    Reserved_1    :2;
        CBIOS_U32    PKTLEG_SEL_DISABLE    :1;
        CBIOS_U32    PKTLEG_VBLANK    :8;
    };
}REG_MM336B0;


typedef union _REG_MM336B4    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RIRB_WPTR_SEL    :1;
        CBIOS_U32    CORB_RPTR_SEL    :1;
        CBIOS_U32    CIE_GIE_RST_CTL    :1;
        CBIOS_U32    HW_CORB_RST_SEL    :1;
        CBIOS_U32    STRM1_FREE_RUN    :1;
        CBIOS_U32    STRM2_FREE_RUN    :1;
        CBIOS_U32    STRM_NUM_SUPPORT    :1;
        CBIOS_U32    ADDR_64OK    :1;
        CBIOS_U32    CORB_VIR    :1;
        CBIOS_U32    STRM1_BDL_VIR    :1;
        CBIOS_U32    STRM1_VIR    :1;
        CBIOS_U32    STRM2_BDL_VIR    :1;
        CBIOS_U32    STRM2_VIR    :1;
        CBIOS_U32    DMAP_DES1_VIR    :1;
        CBIOS_U32    DMAP_DES2_VIR    :1;
        CBIOS_U32    RIRB_VIR    :1;
        CBIOS_U32    CORB_FB    :1;
        CBIOS_U32    STRM1_BDL_FB    :1;
        CBIOS_U32    STRM1_FB    :1;
        CBIOS_U32    STRM2_BDL_FB    :1;
        CBIOS_U32    STRM2_FB    :1;
        CBIOS_U32    DMAP_DES1_FB    :1;
        CBIOS_U32    DMAP_DES2_FB    :1;
        CBIOS_U32    RIRB_FB    :1;
        CBIOS_U32    CORB_SNOOP    :1;
        CBIOS_U32    STRM1_BDL_SNOOP    :1;
        CBIOS_U32    STRM1_SNOOP    :1;
        CBIOS_U32    STRM2_BDL_SNOOP    :1;
        CBIOS_U32    STRM2_SNOOP    :1;
        CBIOS_U32    DMAP_DES1_SNOOP    :1;
        CBIOS_U32    DMAP_DES2_SNOOP    :1;
        CBIOS_U32    RIRB_SNOOP    :1;
    };
}REG_MM336B4;


typedef union _REG_MM336B8    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDMI1_SCDC_WAIT_TIMEOUT    :24;
        CBIOS_U32    HDMI1_SCDC_RR_INT_STATUS    :1;
        CBIOS_U32    Reserved    :3;
        CBIOS_U32    HDMI1_SCDC_START_STOP_ENABLE    :1;
        CBIOS_U32    HDMI1_SCDC_HW_DRV_STOP_ENABLE    :1;
        CBIOS_U32    HDMI1_SCDC_HW_DRV_START_ENABLE    :1;
        CBIOS_U32    HDMI1_SCDC_RR_ENABLE    :1;
    };
}REG_MM336B8;


typedef union _REG_MM336BC    //HDMI1_SCDC_control_2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDMI1_SCDC_WDATA    :8;
        CBIOS_U32    HDMI1_SCDC_WDATA_SEL    :1;
        CBIOS_U32    IIC1_STATIMEOUT    :23;
    };
}REG_MM336BC;


typedef union _REG_MM338B8    //HDTV1_control_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Progressive_Mode_Enable    :1;
        CBIOS_U32    SMPTE_274M_Enable    :1;
        CBIOS_U32    SMPTE_296M_Enable    :1;
        CBIOS_U32    SMPTE_293M_Enable    :1;
        CBIOS_U32    _576P_Enable    :1;
        CBIOS_U32    HDTV_Timing_Enable_Control    :1;
        CBIOS_U32    ITU470_SELECT    :1;
        CBIOS_U32    _576i_480i_enable    :1;
        CBIOS_U32    Trilevel_Sync_Width    :6;
        CBIOS_U32    HDTV_EN_TRIG_PULSE    :2;
        CBIOS_U32    Blank_Level    :10;
        CBIOS_U32    _1x_2x_4x_oversampling_sel    :2;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM338B8;


typedef union _REG_MM338BC    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_CSC_input_data_format    :5;
        CBIOS_U32    HDTV_CSC_coefficients_sel    :1;
        CBIOS_U32    HDTV_old_slope_enable    :1;
        CBIOS_U32    Reserved    :1;
        CBIOS_U32    HDTV_Contrast_tuning_value    :8;
        CBIOS_U32    HDTV_Pb_Saturation_tuning_value    :8;
        CBIOS_U32    HDTV_Pr_Saturation_tuning_value    :8;
    };
}REG_MM338BC;


typedef union _REG_MM338C0    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_y_dac_filter_select_coef    :5;
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    HDMI_SYNC_Delay_7to0    :8;
        CBIOS_U32    HDTV_SYNC_Delay_10to8    :3;
        CBIOS_U32    Reserved_1    :5;
        CBIOS_U32    HDTV_SYNC_Delay_7to0    :8;
    };
}REG_MM338C0;


typedef union _REG_MM338C4    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_CSC_F0_7to0    :8;
        CBIOS_U32    HDTV_CSC_F1_7to0    :8;
        CBIOS_U32    HDTV_CSC_F2_7to0    :8;
        CBIOS_U32    HDTV_CSC_F3_7to0    :8;
    };
}REG_MM338C4;


typedef union _REG_MM338C8    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_CSC_F4_7to0    :8;
        CBIOS_U32    HDTV_CSC_F5_7to0    :8;
        CBIOS_U32    HDTV_CSC_F6_7to0    :8;
        CBIOS_U32    HDTV_CSC_F7_7to0    :8;
    };
}REG_MM338C8;


typedef union _REG_MM338CC    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_CSC_F8_7to0    :8;
        CBIOS_U32    Left_Blank_Pixels_10to8    :3;
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    Left_Blank_Pixels_7to0    :8;
        CBIOS_U32    HDTV_Brightness_Control    :8;
    };
}REG_MM338CC;


typedef union _REG_MM338D0    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Undefined_Bit_0_to_7    :8;
        CBIOS_U32    HDTV_Digital_HSYNC_Width    :8;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    HDTV_WSS_Clock_Ratio_bit_14    :1;
        CBIOS_U32    Reserved_1    :5;
        CBIOS_U32    HDTV_HSYNC_Delay    :3;
        CBIOS_U32    Reserved_2    :5;
    };
}REG_MM338D0;


typedef union _REG_MM338D4    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Iga1_Character_Clock    :3;
        CBIOS_U32    Iga2_Character_Clock    :3;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    HDTV_WSS_Clock_Ratio13to6    :8;
        CBIOS_U32    HDTV_SENSE_Line_Sleect    :5;
        CBIOS_U32    Reserved_1    :3;
        CBIOS_U32    Hdtv_Broad_Pulse_6to0    :7;
        CBIOS_U32    Reserved_2    :1;
    };
}REG_MM338D4;


typedef union _REG_MM338D8    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Hdtv_Broad_Pulse_14to7    :8;
        CBIOS_U32    Hdtv_Half_Sync_10to0    :11;
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    HDTV_CC_Clock_Ratio_7to0    :8;
    };
}REG_MM338D8;


typedef union _REG_MM338DC    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_CC_Clock_Ratio_14to8    :7;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    HDTV_CC_Line_Select    :6;
        CBIOS_U32    HDTV_CC_Field_Select    :2;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    HDTV_CC_End_10to8    :3;
        CBIOS_U32    Reserved_2    :1;
        CBIOS_U32    HDTV_CC_Start_bit_8    :1;
        CBIOS_U32    HDTV_CC_Sermode    :1;
        CBIOS_U32    HDTV_CC_Word_Mode_Select    :1;
        CBIOS_U32    HDTV_CC_Start_7to0    :8;
    };
}REG_MM338DC;


typedef union _REG_MM338E0    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_CC_WSS_End_7to0    :8;
        CBIOS_U32    HDTV_CC_Field1_Word0    :7;
        CBIOS_U32    HDTV_CC_Field1_Data_Write_Status    :1;
        CBIOS_U32    HDTV_CC_Field1_Word1    :7;
        CBIOS_U32    Reserved    :1;
        CBIOS_U32    HDTV_Parm0_Start_HDTV_HSYNC_7to0    :8;
    };
}REG_MM338E0;


typedef union _REG_MM338E4    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_Parm1_End_1st_Equalizing_7to0_0    :8;
        CBIOS_U32    HDTV_Parm1_End_1st_Equalizing_7to0_1    :8;
        CBIOS_U32    HDTV_HDE_10to0    :11;
        CBIOS_U32    Reserved    :5;
    };
}REG_MM338E4;


typedef union _REG_MM338E8    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_Parm3_Start_HDTV_HSYNC_7to0    :8;
        CBIOS_U32    HDTV_Parm4_End_1st_Serration_Start_2nd_Equalization_7to0    :8;
        CBIOS_U32    HDTV_Parm5_End_2nd_Equalization_7to0    :8;
        CBIOS_U32    HDTV_Parm6_End_2nd_Equalization_7to0    :8;
    };
}REG_MM338E8;


typedef union _REG_MM338EC    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_Parm6_Start_2nd_Serration_10to8    :3;
        CBIOS_U32    Reserved    :4;
        CBIOS_U32    VBI_Enable    :1;
        CBIOS_U32    WSS_Word0_or_B_Data_Byte_2_or_WSS_Word_13to8    :8;
        CBIOS_U32    WSS_Word1_or_B_Data_Byte_2_or_WSS_Word_13to8    :8;
        CBIOS_U32    WSS_Word2_or_Packet_A_Data_or_Packet_B_Data_Byte    :8;
    };
}REG_MM338EC;


typedef union _REG_MM338F0    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_CC_Field0_Word0    :7;
        CBIOS_U32    HDTV_CC_Field0_Data_Write_Status    :1;
        CBIOS_U32    HDTV_CC_Field0_Word1_or_HDTV_Packet_A_CRC_Or_Packet_B_Data_Byte_14    :7;
        CBIOS_U32    Reserved_OR__Packet_B_Data_Byte_14    :1;
        CBIOS_U32    WSS_Clock_Ratio_5to0    :6;
        CBIOS_U32    WSS_Mode_Enable    :2;
        CBIOS_U32    WSS_CGMS_A_Line_Select    :6;
        CBIOS_U32    WSS_Field_Select    :2;
    };
}REG_MM338F0;


typedef union _REG_MM338F4    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_CGMSA_Header    :6;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    Reserved_1    :3;
        CBIOS_U32    HDTV_WSS_CGMSA_Start_4to0    :5;
        CBIOS_U32    HDTV_WSS_CGMSA_Start_8to5    :4;
        CBIOS_U32    HDTV_WSS_Sermode    :1;
        CBIOS_U32    Reserved_2    :1;
        CBIOS_U32    HDTV_CGMSA_Mode    :2;
        CBIOS_U32    _576i_initial_programming    :2;
        CBIOS_U32    HDTV__pbpr_dac_filter_select_coef    :5;
        CBIOS_U32    Reserved_3    :1;
    };
}REG_MM338F4;


typedef union _REG_MM338F8    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Undefined_Bit_0_to_7    :8;
        CBIOS_U32    Pr_channel_delay    :2;
        CBIOS_U32    Pb_channel_delay    :2;
        CBIOS_U32    HDTV_Parm1_End_1st_Equalizing_10to8    :3;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    HDTV_Parm2_End_TV_HSYNC_10to8    :3;
        CBIOS_U32    Y_channel_delay    :2;
        CBIOS_U32    Reserved_1    :3;
        CBIOS_U32    HDTV_Parm3_Start_First_Serration_9to8    :2;
        CBIOS_U32    HDTV_Parm4_End_1st_Serration_Start_2nd_Equalization_9to8    :2;
        CBIOS_U32    HDTV_Parm5_End_2nd_Equalization_9to8    :2;
        CBIOS_U32    Reserved_2    :2;
    };
}REG_MM338F8;


typedef union _REG_MM338FC    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Undefined_Bit_0_to_7    :8;
        CBIOS_U32    Reserved_bit0    :1;
        CBIOS_U32    HDTV_CSC_F8_10to8    :3;
        CBIOS_U32    Reserved_bit1    :3;
        CBIOS_U32    HDTV_Enable    :1;
        CBIOS_U32    Undefined_Bit_16_to_31    :16;
    };
}REG_MM338FC;


typedef union _REG_MM33394    //hdtv1_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB1_COEF_F1    :14;
        CBIOS_U32    LB1_COEF_F2    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM33394;


typedef union _REG_MM33398    //hdtv1_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB1_COEF_F3    :14;
        CBIOS_U32    LB1_COEF_F4    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM33398;


typedef union _REG_MM3339C    //hdtv1_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB1_COEF_F5    :14;
        CBIOS_U32    LB1_COEF_F6    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM3339C;


typedef union _REG_MM333A0    //hdtv1_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB1_COEF_F7    :14;
        CBIOS_U32    LB1_COEF_F8    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM333A0;


typedef union _REG_MM333A4    //hdtv1_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB1_COEF_F9    :14;
        CBIOS_U32    LB1_BRIGHT    :9;
        CBIOS_U32    Reserved_31to23    :9;
    };
}REG_MM333A4;


typedef union _REG_MM333AC    //hdtv2_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB2_COEF_F1    :14;
        CBIOS_U32    LB2_COEF_F2    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM333AC;


typedef union _REG_MM333B0    //hdtv2_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB2_COEF_F3    :14;
        CBIOS_U32    LB2_COEF_F4    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM333B0;


typedef union _REG_MM333B4    //hdtv2_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB2_COEF_F5    :14;
        CBIOS_U32    LB2_COEF_F6    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM333B4;


typedef union _REG_MM333B8    //hdtv2_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB2_COEF_F7    :14;
        CBIOS_U32    LB2_COEF_F8    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM333B8;


typedef union _REG_MM333BC    //hdtv2_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB2_COEF_F9    :14;
        CBIOS_U32    LB2_BRIGHT    :9;
        CBIOS_U32    Reserved_31to23    :9;
    };
}REG_MM333BC;


typedef union _REG_MM333C0    //DAC_csc_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DAC_DATA_IN_FMT    :3;
        CBIOS_U32    DAC_DATA_OUT_FMT    :3;
        CBIOS_U32    DAC_PROGRAMMBLE    :1;
        CBIOS_U32    Reserved_31to7    :25;
    };
}REG_MM333C0;


typedef union _REG_MM333C4    //DAC_csc_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DAC_COEF_F1    :14;
        CBIOS_U32    DAC_COEF_F2    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM333C4;


typedef union _REG_MM333C8    //DAC_csc_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DAC_COEF_F3    :14;
        CBIOS_U32    DAC_COEF_F4    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM333C8;


typedef union _REG_MM333CC    //DAC_csc_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DAC_COEF_F5    :14;
        CBIOS_U32    DAC_COEF_F6    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM333CC;


typedef union _REG_MM333D0    //DAC_csc_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DAC_COEF_F7    :14;
        CBIOS_U32    DAC_COEF_F8    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM333D0;


typedef union _REG_MM333D4    //DAC_csc_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DAC_COEF_F9    :14;
        CBIOS_U32    DAC_BRIGHT    :9;
        CBIOS_U32    Reserved_31to23    :9;
    };
}REG_MM333D4;


typedef union _REG_MM333D8    //PWM_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    backlight_value    :16;
        CBIOS_U32    PWM_frequency_counter    :16;
    };
}REG_MM333D8;


typedef union _REG_MM333DC    //PWM_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    REG_PWM_EN    :1;
        CBIOS_U32    REG_PWM_CTRL    :2;
        CBIOS_U32    PWM_frequency_counter    :2;
        CBIOS_U32    DIU_DIO_PWM_oen    :1;
        CBIOS_U32    Reserved_31to6    :26;
    };
}REG_MM333DC;


typedef union _REG_MM333E0    //Primary_Stream_1_Frame_Buffer_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_Enable_Work_Registers    :1;
        CBIOS_U32    SS1_Enable_Work_Registers    :1;
        CBIOS_U32    Cursor_1_Enable_Work_Registers    :1;
        CBIOS_U32    Write_Back1_Enable_Work_Registers    :1;
        CBIOS_U32    Reserved    :27;
        CBIOS_U32    Stream1_Enable_Work_Registers    :1;
    };
}REG_MM333E0;


typedef union _REG_MM333E4    //Primary_Stream_1_Frame_Buffer_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_Enable_Work_Registers    :1;
        CBIOS_U32    SS2_Enable_Work_Registers    :1;
        CBIOS_U32    Cursor_2_Enable_Work_Registers    :1;
        CBIOS_U32    Write_Back2_Enable_Work_Registers    :1;
        CBIOS_U32    Reserved    :27;
        CBIOS_U32    Stream2_Enable_Work_Registers    :1;
    };
}REG_MM333E4;


typedef union _REG_MM333E8    //Primary_Stream_1_Frame_Buffer_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_Enable_Work_Registers    :1;
        CBIOS_U32    SS3_Enable_Work_Registers    :1;
        CBIOS_U32    Cursor_3_Enable_Work_Registers    :1;
        CBIOS_U32    Reserved    :28;
        CBIOS_U32    Stream3_Enable_Work_Registers    :1;
    };
}REG_MM333E8;

