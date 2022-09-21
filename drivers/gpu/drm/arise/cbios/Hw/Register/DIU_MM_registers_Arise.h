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


typedef union _REG_MM8180_Arise    //backlight_adjustment_&_FRC
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Backlight_Adj_Pwm_Factor    :8;
        CBIOS_U32    Backlight_Adj_Factor    :9;
        CBIOS_U32    PS1_Backlight_Adjust    :1;
        CBIOS_U32    Reserved_0    :11;
        CBIOS_U32    PS1_Ffc_Distrib_Dither    :1;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    PS1_Ffc_Enable    :1;
    };
}REG_MM8180_Arise;


typedef union _REG_MM8184_Arise    //Secondary_Stream_1_Color/Chroma_8bit_Key_Lower_Bound_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_8bit_B__Cb_Key_Lower_Bound    :8;
        CBIOS_U32    SS_8bit_G_Y_Key_Lower_Bound    :8;
        CBIOS_U32    SS_8bit_R_Cr_Key_Lower_Bound    :8;
        CBIOS_U32    Reserved    :8;
    };
    struct
    {
        CBIOS_U32    SS_10bit_B_Cb_Key_Lower_Bound    :10;
        CBIOS_U32    SS_10bit_G_Y_Key_Lower_Bound    :10;
        CBIOS_U32    SS_10bit_R_Cr_Key_Lower_Bound    :10;
        CBIOS_U32    Reserved_0    :2;
    };
}REG_MM8184_Arise;

typedef union _REG_MM8188_Arise    //PS_KEYH_for_chorma_key
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_KeyH    :24;
        CBIOS_U32    Reserved    :8;
    };
}REG_MM8188_Arise;


typedef union _REG_MM8190_Arise    //Streams_1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :8;
        CBIOS_U32    SS_Input_Format    :3;
        CBIOS_U32    SS_Uyvy422    :1;
        CBIOS_U32    NV12_Tile_Enable    :1;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    SS_YCbCr_Mode    :1;
        CBIOS_U32    Reserved_2    :1;
        CBIOS_U32    SS_Src_Line_Width    :13;
        CBIOS_U32    Reserved_3    :3;
    };
}REG_MM8190_Arise;


typedef union _REG_MM8194_Arise    //OVL2 register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ka_3to0_Or_Ks    :4;
        CBIOS_U32    Ka_7to4_or_Kp    :4;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    Ovl1_Input_Stream    :2;
        CBIOS_U32    Color_Key_Sel    :2;
        CBIOS_U32    Alpha_Select    :2;
        CBIOS_U32    Alpha_Range    :1;
        CBIOS_U32    Alpha_Round    :1;
        CBIOS_U32    Reserved    :6;
        CBIOS_U32    Key_Mode    :4;
        CBIOS_U32    Invert_Alpha_Or_Ka    :1;
        CBIOS_U32    Ovl1_One_Shot    :1;
        CBIOS_U32    Ovl1_Vsync_Off_Flip    :1;
        CBIOS_U32    Ovl1_Enable_Work    :1;
    };
}REG_MM8194_Arise;


typedef union _REG_MM8198_Arise    //Secondary_Stream_1_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_Compress_Type    :6;
        CBIOS_U32    SS_Comp_Mode_Enable    :1;
        CBIOS_U32    Reserved_0    :17;
        CBIOS_U32    SS_Fifo_Depth_Control    :3;
        CBIOS_U32    SS_Share_Scaler_Fifo    :1;
        CBIOS_U32    Reserved_3    :4;
    };
}REG_MM8198_Arise;


typedef union _REG_MM819C_Arise    //SS1_destination_size_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_Dest_Height    :12;
        CBIOS_U32    Reserved_0    :4;
        CBIOS_U32    SS_Dest_Width    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM819C_Arise;


typedef union _REG_MM81A0_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Lut_Read_Data    :32;
    };
}REG_MM81A0_Arise;


typedef union _REG_MM81A4_Arise    //Primary_Stream_1_Register_1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_Compress_Type    :6;
        CBIOS_U32    PS_Dis_Oddeven    :1;
        CBIOS_U32    Reserved_0    :17;
        CBIOS_U32    PS_Fifo_Depth    :3;
        CBIOS_U32    PS_Share_Scl_Fifo    :1;
        CBIOS_U32    Reserved_3    :4;
    };
}REG_MM81A4_Arsie;


typedef union _REG_MM81A8_Arise    //Secondary_Stream_Source_Height_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_Line_Height    :12;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    SS_Buffer_Select    :1;
        CBIOS_U32    SS_Double_Buf_Select    :1;
        CBIOS_U32    SS_Triple_Buf_Enable    :1;
        CBIOS_U32    Reserved_1    :16;
    };
}REG_MM81A8_Arise;


typedef union _REG_MM81AC_Arise    //PS1_key_reg
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_B_Cb_Low_Or_Alpha_Key    :8;
        CBIOS_U32    PS_G_Y_Low_Key    :8;
        CBIOS_U32    PS_R_Cr_Low_Key    :8;
        CBIOS_U32    Reserved    :8;
    };
}REG_MM81AC_Arise;


typedef union _REG_MM81C0_Arise    //Primary_Stream_1_Frame_Buffer_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Corrupt_Frame_Dscl_    :1;
        CBIOS_U32    Reserved    :30;
        CBIOS_U32    PS_Enable_Work    :1;
    };
}REG_MM81C0_Arise;


typedef union _REG_MM81C4_Arise    //Primary_Stream_1_ROFFSET_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_Right_Frame_Base    :30;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    VsyncOff    :1;
    };
}REG_MM81C4_Arise;


typedef union _REG_MM81C8_Arise    //Primary_Stream_1_Stride_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    PS_Abgr_En    :1;
        CBIOS_U32    Reserved_1    :2;
        CBIOS_U32    PS_Stride    :12;
        CBIOS_U32    PS_Pixel_Offset    :4;
        CBIOS_U32    PS_Blank_Alpha    :8;
        CBIOS_U32    PS_Ycbcr_Mode    :1;
        CBIOS_U32    Eanble_422_To_420    :1;
        CBIOS_U32    PS_Pixel_Offset_Bit4    :1;
        CBIOS_U32    PS_Uyvy422    :1;
    };
}REG_MM81C8_Arise;


typedef union _REG_MM81D0_Arise    //Secondary_Stream_1_Frame_Buffer_Start_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_Start_Address0    :31;
        CBIOS_U32    SS_Work_Reg_En    :1;
    };
}REG_MM81D0_Arise;


typedef union _REG_MM81D4_Arise    //Secondary_Stream_1_Frame_Buffer_Start_Address_1__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_Roffset    :31;
        CBIOS_U32    SS_Vsync_Off_Flip    :1;
    };
}REG_MM81D4_Arise;


typedef union _REG_MM81D8_Arise    //Secondary_Stream_1_Stride_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_Read_Length    :2;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    SS_Stride    :12;
        CBIOS_U32    SS_Pixel_Offset    :4;
        CBIOS_U32    Reserved_1    :11;
        CBIOS_U32    SS_Abgr_En    :1;
    };
}REG_MM81D8_Arise;

typedef union _REG_MM81DC_Arise
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0         :1;
        CBIOS_U32    PS1_Start_Address   :31;
    };
}REG_MM81DC_Arise;

typedef union _REG_MM81E0_Arise
{
    CBIOS_U32 Value;
    struct
    {
        CBIOS_U32 PS_Right_Frame_Base    :32;
    };
}REG_MM81E0_Arise;

typedef union _REG_MM81E4_Arise    //Cursor1_control_reg
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Cursor1_One_Shot    :1;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    Cursor_420_Input    :1;
        CBIOS_U32    Reserved_1    :5;
        CBIOS_U32    Cursor1_Csc_output_Fmt    :3;
        CBIOS_U32    Reserved_2    :21;
    };
}REG_MM81E4_Arise;


typedef union _REG_MM81EC_Arise    //Secondary_Stream_1_Frame_Buffer_Start_Address_2_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_Start_Address2    :31;
        CBIOS_U32    Reserved    :1;
    };
}REG_MM81EC_Arise;


typedef union _REG_MM81F4_Arise    //NV12C_offset
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    Nv12y_Offset_X    :4;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    Nv12y_Offset_Y    :7;
        CBIOS_U32    Reserved_2    :4;
        CBIOS_U32    Nv12c_Offset_X    :4;
        CBIOS_U32    Reserved_3    :1;
        CBIOS_U32    Nv12c_Offset_Y    :7;
        CBIOS_U32    Reserved_4    :1;
    };
}REG_MM81F4_Arise;


typedef union _REG_MM81F8_Arise    //Secondary_Stream_1_Window_Start_Coordinates_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_Y_Start    :12;
        CBIOS_U32    Reserved_0    :4;
        CBIOS_U32    SS_X_Start    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM81F8_Arise;


typedef union _REG_MM81FC_Arise    //Primary_Stream1_Shadow_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_L1_Bit11    :1;
        CBIOS_U32    PS_3D_Video_Mode    :3;
        CBIOS_U32    PS_Read_Length    :2;
        CBIOS_U32    Enable_PS_L1    :1;
        CBIOS_U32    PS_L1_10to0    :11;
        CBIOS_U32    DAC_Color_Mode    :3;
        CBIOS_U32    Use_2_Frame_Page_Flip    :1;
        CBIOS_U32    Page_Flip_Every_2_Frame    :1;
        CBIOS_U32    PS_Disable    :1;
        CBIOS_U32    PS_Compress_Enable    :1;
        CBIOS_U32    Reserved    :7;
    };
}REG_MM81FC_Arise;


typedef union _REG_MM8200_Arise    //Stream_1_one_shot_control
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_Enable_Work    :1;
        CBIOS_U32    SS_Enable_Work    :1;
        CBIOS_U32    TS_Enable_Work    :1;
        CBIOS_U32    QS_Enable_Work    :1;
        CBIOS_U32    Write_Back_Enable_Work    :1;
        CBIOS_U32    Ovl1_Enable_Work    :1;
        CBIOS_U32    Ovl2_Enable_Work    :1;
        CBIOS_U32    Ovl3_Enable_Work    :1;
        CBIOS_U32    Hwc_Enable_Work    :1;
        CBIOS_U32    TS_win2_Enable_Work    :1;
        CBIOS_U32    QS_win2_Enable_Work    :1;
        CBIOS_U32    Ovl0_Enable_Work    :1;
        CBIOS_U32    Reserved    :19;
        CBIOS_U32    Iga_Enable_Work    :1;
    };
}REG_MM8200_Arise;


typedef union _REG_MM8210_Arise    //DP_NVID_amd_MISC1_Attribute_Data_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    NVID    :24;
        CBIOS_U32    MISC1_Even    :1;
        CBIOS_U32    MISC1_Stereo_Video    :2;
        CBIOS_U32    MISC1_Reserved    :5;
    };
}REG_MM8210_Arise;


typedef union _REG_MM8214_Arise    //DP_Link_Training_Control_Register
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
}REG_MM8214_Arise;


typedef union _REG_MM8218_Arise    //DP_Video_Input_Select_and_General_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Scramble_Enable    :1;
        CBIOS_U32    Switch_Idle_Mode_To_Video    :1;
        CBIOS_U32    Idle_Pattern_Counter    :9;
        CBIOS_U32    Aux_Length    :5;
        CBIOS_U32    Audio_Strm_Select    :1;
        CBIOS_U32    Hw_Link_Train_Fail    :1;
        CBIOS_U32    Min_Aux_Sync_Count    :6;
        CBIOS_U32    Delay    :6;
        CBIOS_U32    Reseved    :2;
    };
}REG_MM8218_Arise;


typedef union _REG_MM821C_Arise    //DP_Version_and_Extension_Packet_Head_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ext_Pkt_Head    :24;
        CBIOS_U32    Ext_Pkt_ID_Value    :4;
        CBIOS_U32    Reserved    :2;
        CBIOS_U32    Horizontal_Width_Bit12to11    :2;
    };
}REG_MM821C_Arise;


typedef union _REG_MM8220_Arise    //Primary_Stream_1_Display_Position_Frame_Line_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_Frame_Counter    :16;
        CBIOS_U32    PS1_Line_Counter    :16;
    };
}REG_MM8220_Arise;


typedef union _REG_MM8224_Arise    //Primary_Stream_1_Display_Position_Line_Pixel_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_Line_Counter    :16;
        CBIOS_U32    PS1_Pixel_Counter    :16;
    };
}REG_MM8224_Arise;


typedef union _REG_MM8228_Arise    //Primary_Stream_1_Top_Left_Pixel_Line_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_Top_Left_Line    :16;
        CBIOS_U32    PS1_Top_Leftt_Pixel    :16;
    };
}REG_MM8228_Arise;


typedef union _REG_MM822C_Arise    //Primary_Stream_1_Bottom_Right_Pixel_Line_Count_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_Bottom_Right_Line    :16;
        CBIOS_U32    PS1_Bottom_Right_Pixel    :16;
    };
}REG_MM822C_Arise;


typedef union _REG_MM8240_Arise    //DP_Display_Port_Enable_and_InfoFrame_Control_Register
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
}REG_MM8240_Arise;


typedef union _REG_MM8244_Arise    //DP_Horizontal_Width_and_TU_Size_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Horiz_Width    :11;
        CBIOS_U32    TU_Size    :6;
        CBIOS_U32    TU_Ratio    :15;
    };
}REG_MM8244_Arise;


typedef union _REG_MM8248_Arise    //DP_Horizontal_Line_Duration_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Horiz_Line_Duration    :15;
        CBIOS_U32    HBLANK_Duration    :12;
        CBIOS_U32    Ext_Packet_Byte_Num    :4;
        CBIOS_U32    Ext_Packet_Available    :1;
    };
}REG_MM8248_Arise;


typedef union _REG_MM824C_Arise    //DP_MVID_and_MISC0_Attribute_Data_Register
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
}REG_MM824C_Arise;


typedef union _REG_MM8250_Arise    //DP_HTOTAL_Attribute_Data_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    H_Total    :16;
        CBIOS_U32    V_Total    :16;
    };
}REG_MM8250_Arise;


typedef union _REG_MM8254_Arise    //DP_Horiz_Vert_Start_Attribute_Data_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    H_Start    :16;
        CBIOS_U32    V_Start    :16;
    };
}REG_MM8254_Arise;


typedef union _REG_MM8258_Arise    //DP_Polarity/Width_Attribute_Data_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HSYNC_Width    :15;
        CBIOS_U32    HSYNC_Polarity    :1;
        CBIOS_U32    VSYNC_Width    :15;
        CBIOS_U32    VSYNC_Polarity    :1;
    };
}REG_MM8258_Arise;


typedef union _REG_MM825C_Arise    //DP_Polarity/Width_Attribute_Data_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Acitve_Width    :16;
        CBIOS_U32    Active_Height    :16;
    };
}REG_MM825C_Arise;


typedef union _REG_MM8264_Arise    //down_Scaling1_write_back_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Down_Scale_Src_Select    :1;
        CBIOS_U32    Csc_In_Format    :3;
        CBIOS_U32    Csc_Out_Format    :3;
        CBIOS_U32    Csc_Coef_In_Dscl_Mux    :1;
        CBIOS_U32    Program_Bright_255_255_In_Dscl_Csc    :9;
        CBIOS_U32    Reserved_0    :15;
    };
}REG_MM8264_Arise;


typedef union _REG_MM8268_Arise    //Dscl1_CSC_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Program_Coef_1_In_Dscl_Csc    :14;
        CBIOS_U32    Program_Coef_2_In_Dscl_Csc    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM8268_Arise;


typedef union _REG_MM826C_Arise    //Dscl1_CSC_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Program_Coef_3_In_Dscl_Csc    :14;
        CBIOS_U32    Program_Coef_4_In_Dscl_Csc    :14;
        CBIOS_U32    Reserved_0    :4;
    };
}REG_MM826C_Arise;


typedef union _REG_MM8270_Arise    //Dscl1_CSC_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Program_Coef_5_In_Dscl_Csc    :14;
        CBIOS_U32    Program_Coef_6_In_Dscl_Csc    :14;
        CBIOS_U32    Reserved_0    :4;
    };
}REG_MM8270_Arise;


typedef union _REG_MM8274_Arise    //Dscl1_CSC_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Program_Coef_7_In_Dscl_Csc    :14;
        CBIOS_U32    Program_Coef_8_In_Dscl_Csc    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM8274_Arise;


typedef union _REG_MM8278_Arise    //Dscl1_CSC_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Program_Coef_9_In_Dscl_Csc    :14;
        CBIOS_U32    Reserved_0    :18;
    };
}REG_MM8278_Arise;


typedef union _REG_MM8280_Arise    //HDMI1_General_Control_Register
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
}REG_MM8280_Arise;


typedef union _REG_MM8284_Arise    //HDMI1__InfoFrame_Control_Register
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
}REG_MM8284_Arise;


typedef union _REG_MM8288_Arise    //HD_Audio_Codec_Status_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :25;
        CBIOS_U32    Int_Src_Codec1    :1;
        CBIOS_U32    Reserved_1    :6;
    };
}REG_MM8288_Arise;


typedef union _REG_MM8294_Arise    //HDMI1_Audio_Insert_Control_Register
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
}REG_MM8294_Arise;


typedef union _REG_MM8298_Arise    //HDAUDIO_CODEC1_Audio_Packet_to_Clock_Ratio_Register_1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CODEC1_Audio_Packet_to_DClk_Ratio_31to0    :32;
    };
}REG_MM8298_Arise;


typedef union _REG_MM829C_Arise    //HDAUDIO_CODEC1_Audio_Packet_to_Clock_Ratio_Register_2
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
}REG_MM829C_Arise;


typedef union _REG_MM82A0_Arise    //HDAUDIO_CODEC1_Audio_Mode_and_Response_Register
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
}REG_MM82A0_Arise;


typedef union _REG_MM82A4_Arise    //HDAUDIO_CODEC1_Command_Field_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDA_HDMI_CMD_Verb    :20;
        CBIOS_U32    NID    :8;
        CBIOS_U32    Cad    :4;
    };
}REG_MM82A4_Arise;


typedef union _REG_MM82A8_Arise    //HDAUDIO_CODEC1_Software_Response_Field_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Response    :32;
    };
}REG_MM82A8_Arise;


typedef union _REG_MM82AC_Arise    //HDAUDIO_CODEC1_Speaker_Allocation_and_Channel_Status_Control_Register
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
        CBIOS_U32    Codec_Type    :2;
        CBIOS_U32    Reserved_bits_18to15    :4;
        CBIOS_U32    BCIS_SEL    :1;
        CBIOS_U32    Reserved_bits_23to21    :4;
        CBIOS_U32    ELD_Use_LUT    :1;
        CBIOS_U32    Enable_Transmit_DIP_Packet    :1;
        CBIOS_U32    Enable_HDA_POS_CTR    :1;
        CBIOS_U32    Converter_Stream_Channel    :1;
        CBIOS_U32    CP_Control_CES_State    :1;
        CBIOS_U32    CP_Control_Ready_Status    :1;
        CBIOS_U32    Channel_status_control    :2;
    };
}REG_MM82AC_Arise;


typedef union _REG_MM82B0_Arise    //HDCP1_Key_Selection_Vector_(KSV)_Register_1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    KSV_39to8    :32;
    };
}REG_MM82B0_Arise;


typedef union _REG_MM82B4_Arise    //HDCP1_Control_1_and_Key_Selection_Vector_(KSV)_2_Register
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
}REG_MM82B4_Arsise;


typedef union _REG_MM82B8_Arise    //HDCP1_Control_2_Register
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
}REG_MM82B8_Arise;


typedef union _REG_MM82BC_Arise    //Efuse_Read_Data
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Efuse_Read_Data    :32;
    };
}REG_MM82BC_Arise;


typedef union _REG_MM82C0_Arise    //HDCP1_Slave_receiver_not_ready_register
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
}REG_MM82C0_Arise;


typedef union _REG_MM82C4_Arise    //HDCP1_Miscellaneous_1_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :7;
        CBIOS_U32    HDCP_SW_Reset    :1;
        CBIOS_U32    Repeater_Flag    :1;
        CBIOS_U32    Device_Count    :7;
        CBIOS_U32    Interrupt_Source    :3;
        CBIOS_U32    CTL    :4;
        CBIOS_U32    I2C_Frequency_Sleect    :3;
        CBIOS_U32    reserved    :1;
        CBIOS_U32    Disable_hamming_decoder    :1;
        CBIOS_U32    Reserved_1    :4;
    };
}REG_MM82C4_Arise;


typedef union _REG_MM82C8_Arise    //HDCP1_Miscellaneous_Register_2
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
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    command_buffer_nfull    :1;
        CBIOS_U32    command_buffer_nempty    :1;
        CBIOS_U32    Reserved_2    :3;
        CBIOS_U32    DRV_EFUSE_RWL    :1;
        CBIOS_U32    DRV_EFUSE_RWL_SEL    :1;
        CBIOS_U32    reserved    :1;
        CBIOS_U32    DRV_EFUSE_RSB    :1;
        CBIOS_U32    Reserved_4    :3;
    };
}REG_MM82C8_Arise;


typedef union _REG_MM82CC_Arise    //DP1_control_Register
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
}REG_MM82CC_Arise;


typedef union _REG_MM82D0_Arise    //HDAUDIO_CODEC1_Vendor_ID_Parameter_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Device_ID    :16;
        CBIOS_U32    Vendor_ID    :16;
    };
}REG_MM82D0_Arise;


typedef union _REG_MM82D4_Arise    //HDAUDIO_CODEC1_Revision_ID_and_Support_Parameters_Register
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
}REG_MM82D4_Arise;


typedef union _REG_MM82D8_Arise    //HDAUDIO_CODEC1_Function_Group_Subordinate_Node_Count_Parameter_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FunGroup_Sub_Node_Count_Total    :8;
        CBIOS_U32    Reserved_15to8    :8;
        CBIOS_U32    FunGroup_Sub_Node_Start_Num    :8;
        CBIOS_U32    Reserved_31to24    :8;
    };
}REG_MM82D8_Arise;


typedef union _REG_MM82DC_Arise    //HDAUDIO_CODEC1_Audio_Function_Group_Type_and_Capability_Parameter_Register
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
}REG_MM82DC_Arise;


typedef union _REG_MM82E0_Arise   //HDAUDIO_CODEC1_PCM_Size_and_Sample_Rate_Capability_Parameter_Register
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
}REG_MM82E0_Arise;


typedef union _REG_MM82E4_Arise    //HDAUDIO_CODEC1_Output_Amplifier_Capability_Parameter_Register
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
}REG_MM82E4_Arise;


typedef union _REG_MM82E8_Arise    //HDAUDIO_CODEC1_Functional_Group_Subsystem_ID_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FunGroup__SubsystemID_SSID    :32;
    };
}REG_MM82E8_Arise;


typedef union _REG_MM82EC_Arise    //HDAUDIO_CODEC1_Converter1_Audio_Widget_Capatibility_Parameter_Register
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
}REG_MM82EC_Arise;


typedef union _REG_MM82F0_Arise    //HDAUDIO_CODEC1_PinWidget1_Audio_Widget_Capability_Parameter_Register
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
}REG_MM82F0_Arise;


typedef union _REG_MM82F4_Arise    //HDAUDIO_CODEC1_PinWidget1_Pin_Capability_Parameter_Register
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
}REG_MM82F4_Arise;


typedef union _REG_MM82F8_Arise    //HDAUDIO_CODEC1_PinWidget1_Configuration_Default_Control_Register
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
}REG_MM82F8_Arise;


typedef union _REG_MM82FC_Arise  //HDAUDIO_CODEC1_Converter1_Stream_Format_and_Channel_Stream_ID_Controls_Register
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
}REG_MM82FC_Arise;


typedef union _REG_MM8300_Arise    //HDAUDIO_CODEC1_Converter1_Digital_Converter_and_PinWidget1_Unsolicited_Response_Controls_Register
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
}REG_MM8300_Arise;


typedef union _REG_MM8304_Arise    //HDAUDIO_CODEC1_Converter_1_Stripe_Control_Register
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
}REG_MM8304_Arise;


typedef union _REG_MM8308_Arise    //HDAUDIO_CODEC1_PinWidget1_Pin_Sense_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PINWIDGET1_PinSense_RightChnl    :1;
        CBIOS_U32    PINWIDGET1_PinSense_Impedance    :30;
        CBIOS_U32    PINWIDGET1_PinSense_Presense_Detect    :1;
    };
}REG_MM8308_Arise;


typedef union _REG_MM830C_Arise    //HDAUDIO_CODEC1_PinWidget1_Pin_Widget_Control_Register
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
}REG_MM830C_Arise;


typedef union _REG_MM8310_Arise    //DP_Software_AUX_Write_Data_Bytes_3-0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Write_Bytes_3_0    :32;
    };
}REG_MM8310_Arise;


typedef union _REG_MM8314_Arise    //DP_Software_AUX_Write_Data_Bytes_7-4_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Write_Bytes_7_4    :32;
    };
}REG_MM8314_Arise;


typedef union _REG_MM8318_Arise    //DP_Software_AUX_Write_Data_Bytes_11-8_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Write_Bytes_11_8    :32;
    };
}REG_MM8318_Arsie;


typedef union _REG_MM831C_Arise    //DP_Software_AUX_Write_Data_Bytes_15-12_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Write_Bytes_15_12    :32;
    };
}REG_MM831C_Arise;


typedef union _REG_MM8320_Arise    //DP_Software_AUX_Read_Data_Bytes_3-0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Read_Bytes_3_0    :32;
    };
}REG_MM8320_Arise;


typedef union _REG_MM8324_Arise    //DP_Software_AUX_Read_Data_Bytes_7-4_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Read_Bytes_7_4    :32;
    };
}REG_MM8324_Arise;


typedef union _REG_MM8328_Arise    //DP_Software_AUX_Read_Data_Bytes_11-8_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Read_Bytes_11_8    :32;
    };
}REG_MM8328_Arise;


typedef union _REG_MM832C_Arise    //DP_Software_AUX_Read_Data_Bytes_15-12_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUX_Read_Bytes_15_12    :32;
    };
}REG_MM832C_Arise;


typedef union _REG_MM8330_Arise    //DP_Software_AUX_Timer_Register
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
}REG_MM8330_Arise;


typedef union _REG_MM8334_Arise    //DP_Software_AUX_Command_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SW_AUX_CMD    :4;
        CBIOS_U32    SW_AUX_Length    :8;
        CBIOS_U32    SW_AUX_Addr    :20;
    };
}REG_MM8334_Arise;


typedef union _REG_MM8338_Arise    //DP_NAUD_and_Mute_Control_Register
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
}REG_MM8338_Arise;


typedef union _REG_MM833C_Arise    //DP_MAUD_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    MAUD    :24;
        CBIOS_U32    Secondary_data_Packet_ID    :8;
    };
}REG_MM833C_Arise;


typedef union _REG_MM8340_Arise    //DP_EPHY_MPLL_Power_Control_Register
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
}REG_MM8340_Arise;


typedef union _REG_MM8344_Arise    //DP_EPHY_TX_Power_Control_Register
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
}REG_MM8344_Arise;


typedef union _REG_MM8348_Arise    //DP_EPHY_Miscellaneous_Power_Control_Register
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
}REG_MM8348_Arise;


typedef union _REG_MM834C_Arise    //HDAUDIO_CODEC1_Channel_Count_and_ELD_DIP_Buffer_Size_Register
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
}REG_MM834C_Arise;


typedef union _REG_MM8350_Arise    //HDAUDIO_CODEC1_ASP_Channel_Map_Register
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
}REG_MM8350_Arise;


typedef union _REG_MM8354_Arise    //HDAUDIO_CODEC1_DIP_Transmit_and_CP_Control_Register
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
}REG_MM8354_Arise;


typedef union _REG_MM8358_Arise    //DP_Extension_Packet_Payload_Bytes_3-0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Extension_Packet_Payload_Bytes_3_0    :32;
    };
}REG_MM8358_Arise;


typedef union _REG_MM835C_Arise    //DP_Extension_Packet_Payload_Bytes_7-4_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Extension_Packet_Payload_Bytes_7_4    :32;
    };
}REG_MM835C_Arise;


typedef union _REG_MM8360_Arise    //DP_Extension_Packet_Payload_Bytes_11-8_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Extension_Packet_Payload_Bytes_11_8    :32;
    };
}REG_MM8360_Arise;


typedef union _REG_MM8364_Arise    //DP_Extension_Packet_Payload_Bytes_15-12_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Extension_Packet_Payload_Bytes_15_12    :32;
    };
}REG_MM8364_Arise;


typedef union _REG_MM8368_Arise    //DP
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
}REG_MM8368_Arise;


typedef union _REG_MM836C_Arise    //DP
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
}REG_MM836C_Arise;


typedef union _REG_MM8370_Arise    //DP_HDCP_Key_Selection_Vector_(KSV)_1_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    KSV_39to8    :32;
    };
}REG_MM8370_Arise;


typedef union _REG_MM8374_Arise    //DP_HDCP_Control_1_and_Key_Selection_Vector_(KSV)_2_Register
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
}REG_MM8374_Arise;


typedef union _REG_MM8378_Arise    //DP_HDCP_Control_2_Register
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
}REG_MM8378_Arise;


typedef union _REG_MM837C_Arise    //HDAUDIO_CODEC1_Control_Writes_Default_Register
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
}REG_MM837C_Arise;


typedef union _REG_MM8380_Arise    //HDAUDIO_CODEC1_Read_Out_Control_Select_Register
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
}REG_MM8380_Arise;


typedef union _REG_MM8384_Arise    //HDAUDIO_CODEC1_Read_Out_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Codec_Read_Out    :32;
    };
}REG_MM8384_Arise;


typedef union _REG_MM8388_Arise    //HDAUDIO_Wall_Clock_Ratio_Low_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Wall_clock_ratio_low    :32;
    };
}REG_MM8388_Arise;


typedef union _REG_MM838C_Arise    //HDAUDIO_Wall_Clock_Ratio_Hi_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Wall_clock__ratio__hi    :8;
        CBIOS_U32    Wall_clock_ratio_enable    :1;
        CBIOS_U32    Reserved    :23;
    };
}REG_MM838C_Arise;


typedef union _REG_MM8390_Arise    //HDAUDIO_CODEC1_Channel_Status_Bits_31:0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    bit_0    :1;
        CBIOS_U32    bit_1    :1;
        CBIOS_U32    Channel_status_31_2    :30;
    };
}REG_MM8390_Arise;


typedef union _REG_MM8394_Arise    //HDAUDIO_CODEC1_Channel_Status_Bits_63:32_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Channel_status_63_32    :32;
    };
}REG_MM8394_Arise;


typedef union _REG_MM8398_Arise    //HDAUDIO_CODEC1_Channel_Status_Bits_95:64_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Channel_status_95_64    :32;
    };
}REG_MM8398_Arise;


typedef union _REG_MM839C_Arise    //HDAUDIO_CODEC1_Channel_Status_Block_Bits_127:96_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Channel_status_127_96    :32;
    };
}REG_MM839C_Arise;


typedef union _REG_MM83A0_Arise    //HDAUDIO_CODEC1_Channel_Status_Bits_159:128_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Channel_status_159_128    :32;
    };
}REG_MM83A0_Arise;


typedef union _REG_MM83A4_Arise    //HDAUDIO_CODEC1_Channel_Status_Bits_191:160_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Channel_status_191_160    :32;
    };
}REG_MM83A4_Arise;


typedef union _REG_MM83A8_Arise    //HDMI1_CODEC1_Audio_Clock_Regeneration_Numerator_(N)_and_CTS_Low_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    N    :20;
        CBIOS_U32    CTS    :12;
    };
}REG_MM83A8_Arise;


typedef union _REG_MM83AC_Arise    //HDMI_AUDIO_CODEC1_Audio_Clock_Cycle_Time_Stamp_(CTS)__Register
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
}REG_MM83AC_Arise;


typedef union _REG_MM8400_Arise    //Thrid_Stream_1_Color/Chroma_8bit_Key_Lower_Bound_Register_(windows1)
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS1_8bit_B__Cb_Key_Lower_Bound    :8;
        CBIOS_U32    TS1_8bit_G_Y_Key_Lower_Bound    :8;
        CBIOS_U32    TS1_8bit_R_Cr_Key_Lower_Bound    :8;
        CBIOS_U32    Reserved0    :8;
    };
    struct
    {
        CBIOS_U32    TS1_10bit_B__Cb_Key_Lower_Bound    :10;
        CBIOS_U32    TS1_10bit_G_Y_Key_Lower_Bound    :10;
        CBIOS_U32    TS1_10bit_R_Cr_Key_Lower_Bound    :10;
        CBIOS_U32    Reserved1    :2;
    };
}REG_MM8400_Arise;

typedef union _REG_MM8410_Arise    //Third_OVL
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ka_3to0_Or_Ks    :4;
        CBIOS_U32    Ka_7to4_oOr_Kp    :4;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    Ovl2_Input_Stream    :2;
        CBIOS_U32    Ovl2_Color_Key_sel    :2;
        CBIOS_U32    Ovl2_Alpha_Select    :2;
        CBIOS_U32    Alpha_Rang    :1;
        CBIOS_U32    Alpha_Round    :1;
        CBIOS_U32    Reserved_1    :6;
        CBIOS_U32    Ovl2_Key_Mode    :4;
        CBIOS_U32    Invert_Alpha_or_Ka    :1;
        CBIOS_U32    Ovl2_One_Shot    :1;
        CBIOS_U32    Ovl2_Vsync_Off_Flip    :1;
        CBIOS_U32    Ovl2_Enable_Work    :1;
    };
}REG_MM8410_Arise;


typedef union _REG_MM8414_Arise    //Third_Stream_1_windows1__Register_2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Comp_Type    :6;
        CBIOS_U32    TS_Comp_Enable    :1;
        CBIOS_U32    Reserved_0    :17;
        CBIOS_U32    TS_Fifo_Depth    :3;
        CBIOS_U32    TS_Share_Scaler_Fifo    :1;
        CBIOS_U32    Reserved_1    :4;
    };
}REG_MM8414_Arise;


typedef union _REG_MM8430_Arise    //Thrid_Stream_windows_1_Frame_Buffer_Start_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_FB_Start_Address_0    :31;
        CBIOS_U32    TS_Work_Reg_En    :1;
    };
}REG_MM8430_Arise;


typedef union _REG_MM8438_Arise    //ThirdStream_1_Stride_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Read_Length    :2;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    TS_Stride    :12;
        CBIOS_U32    TS_Start_Address_Byte_Offset    :4;
        CBIOS_U32    Reserved_1    :11;
        CBIOS_U32    TS_Abgr_En    :1;
    };
}REG_MM8438_Arise;


typedef union _REG_MM843C_Arise    //Third_Stream_1__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win1_KeyH    :24;
        CBIOS_U32    Reserved    :8;
    };
}REG_MM843C_Arise;


typedef union _REG_MM8444_Arise    //Third_Stream__Window_1_Start_Coordinates_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Y_Start    :12;
        CBIOS_U32    Reserved_0    :4;
        CBIOS_U32    TS_X_Start    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM8444_Arise;


typedef union _REG_MM8448_Arise    //Third_Stream__Window_1_Start_Coordinates_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win1_Src_H    :12;
        CBIOS_U32    Reserved_0    :4;
        CBIOS_U32    TS_Win1_Src_W    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM8448_Arise;


typedef union _REG_MM844C_Arise    //Third_Stream_window_1_Frame_Buffer_Start_Address_1__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win1_Roffset    :31;
        CBIOS_U32    TS_Win1_Vsync_Off_Flip    :1;
    };
}REG_MM844C_Arise;


typedef union _REG_MM8450_Arise    //Third_Stream__windows1__Frame_Buffer_display_timing
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win1_Dst_H    :12;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    TS_Win1_Buf_Sel    :1;
        CBIOS_U32    TS_Win1_Dbl_Buf_Mode    :1;
        CBIOS_U32    TS_Win1_Triple_Buf_En    :1;
        CBIOS_U32    TS_Win1_Dst_W    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM8450_Arise;


typedef union _REG_MM8454_Arise    //PS1/SS1_threshold
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_low_thrld    :8;
        CBIOS_U32    PS1_high_thrld    :8;
        CBIOS_U32    SS1_low_thrld    :8;
        CBIOS_U32    SS1_high_thrld    :8;
    };
}REG_MM8454_Arise;


typedef union _REG_MM8458_Arise    //TS_QS_two_window_enable
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_En    :1;
        CBIOS_U32    TS_Win2_En    :1;
        CBIOS_U32    Reserved    :30;
    };
}REG_MM8458_Arise;


typedef union _REG_MM8460_Arise    //MDI_read_latency_debug_register_1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_CHECKTAG    :9;
        CBIOS_U32    PS2_CHECKTAG    :9;
        CBIOS_U32    PS3_CHECKTAG    :9;
        CBIOS_U32    Reserved    :5;
    };
}REG_MM8460_Arise;


typedef union _REG_MM8464_Arise    //MDI_read_latency_debug_register_2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS4_CHECKTAG    :9;
        CBIOS_U32    SS1_CHECKTAG    :9;
        CBIOS_U32    SS2_CHECKTAG    :9;
        CBIOS_U32    Reserved    :5;
    };
}REG_MM8464_Arise;


typedef union _REG_MM8468_Arise    //MDI_read_latency_debug_register_3
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS3_CHECKTAG    :9;
        CBIOS_U32    SS4_CHECKTAG    :9;
        CBIOS_U32    TS1_CHECKTAG    :9;
        CBIOS_U32    Reserved    :5;
    };
}REG_MM8468_Arise;


typedef union _REG_MM846C_Arise    //MDI_read_latency_debug_register_4
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS2_CHECKTAG    :9;
        CBIOS_U32    TS3_CHECKTAG    :9;
        CBIOS_U32    TS4_CHECKTAG    :9;
        CBIOS_U32    RESERVED    :5;
    };
}REG_MM846C_Arise;


typedef union _REG_MM8470_Arise    //MDI_read_latency_debug_register_5
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS1_CHECKTAG    :9;
        CBIOS_U32    QS2_CHECKTAG    :9;
        CBIOS_U32    QS3_CHECKTAG    :9;
        CBIOS_U32    RESERVED    :5;
    };
}REG_MM8470_Arise;


typedef union _REG_MM8474_Arise    //MDI_read_latency_debug_register_6
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS4_CHECKTAG    :9;
        CBIOS_U32    BURST_CHECKTAG    :6;
        CBIOS_U32    RESERVED_0    :1;
        CBIOS_U32    Check_en    :1;
        CBIOS_U32    Check_rst    :1;
        CBIOS_U32    RESERVED_1    :14;
    };
}REG_MM8474_Arise;


typedef union _REG_MM8478_Arise    //MDI_read_latency_debug_register_7
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_CTR_MAX    :13;
        CBIOS_U32    RESERVED_0    :3;
        CBIOS_U32    SS1_CTR_MAX    :13;
        CBIOS_U32    RESERVED_1    :3;
    };
}REG_MM8478_Arise;


typedef union _REG_MM847C_Arise    //MDI_read_latency_debug_register_8
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS1_CTR_MAX    :13;
        CBIOS_U32    RESERVED_0    :3;
        CBIOS_U32    QS1_CTR_MAX    :13;
        CBIOS_U32    RESERVED_1    :3;
    };
}REG_MM847C_Arise;


typedef union _REG_MM8480_Arise    //Thrid_Stream_windows_2_Frame_Buffer_Start_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_FB_Start_Address_0    :31;
        CBIOS_U32    TS_Win2_Work_Reg_En    :1;
    };
}REG_MM8480_Arise;


typedef union _REG_MM8484_Arise    //Third_Stream_window_2_Frame_Buffer_Start_Address_1__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_Roffset    :31;
        CBIOS_U32    TS_Win2_Vsync_Off_Flip    :1;
    };
}REG_MM8484_Arise;


typedef union _REG_MM8488_Arise    //ThirdStream_windows2_Stride_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_Read_Length    :2;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    TS_Win2_Stride    :12;
        CBIOS_U32    TS_Win2_Start_Address_Byte_Offset    :4;
        CBIOS_U32    Reserved_1    :11;
        CBIOS_U32    TS_Win2_Abgr_En    :1;
    };
}REG_MM8488_Arise;


typedef union _REG_MM848C_Arise    //Third_Stream__Windows2_Start_Coordinates_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_Y_Start    :12;
        CBIOS_U32    Reserved_0    :4;
        CBIOS_U32    TS_Win2_X_Start    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM848C_Arise;


typedef union _REG_MM8490_Arise    //Third_Stream__Windows2_Start_Coordinates_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_Src_H    :12;
        CBIOS_U32    Reserved_0    :4;
        CBIOS_U32    TS_Win2_Src_W    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM8490_Arise;


typedef union _REG_MM8494_Arise    //Third_Stream__windows2__Frame_Buffer_display_timing
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_Dst_H    :12;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    TS_Win2_Buf_Sel    :1;
        CBIOS_U32    TS_Win2_Dbl_Buf_Mode    :1;
        CBIOS_U32    TS_Win2_Triple_Buf_En    :1;
        CBIOS_U32    TS_Win2_Dst_W    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM8494_Arise;


typedef union _REG_MM8498_Arise    //Third_stream_windows1_surface_address_offset
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    TS_Win1_Base_Offset    :27;
    };
}REG_MM8498_Arise;


typedef union _REG_MM849C_Arise    //Third_stream_windows2_surface_address_offset
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    TS_Win2_Base_Offset    :27;
    };
}REG_MM849C_Arise;


typedef union _REG_MM84A0_Arise    //Third_Stream_1_windows2__Register_2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_Comp_Type    :6;
        CBIOS_U32    TS_Win2_Comp_Enable    :1;
        CBIOS_U32    Reserved_0    :17;
        CBIOS_U32    TS_Win2_Fifo_Depth    :3;
        CBIOS_U32    TS_Win2_Share_Scaler_Fifo    :1;
        CBIOS_U32    Reserved_1    :4;
    };
}REG_MM84A0_Arise;


typedef union _REG_MM84A4_Arise    //Third_stream_windows1_upsacler_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win1_Hacc    :21;
        CBIOS_U32    TS_Win1_Htap_Sel    :1;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    TS_Win1_Alpha_Scale    :1;
        CBIOS_U32    Reserved_1    :7;
    };
}REG_MM84A4_Arise;


typedef union _REG_MM84A8_Arise    //Third_stream_windows1_upsacler_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win1_Vacc    :21;
        CBIOS_U32    TS_Win1_Vtap_Sel    :1;
        CBIOS_U32    TS_Win1_Vdup_En    :1;
        CBIOS_U32    Reserved    :8;
        CBIOS_U32    TS_Win1_Deint_En    :1;
    };
}REG_MM84A8_Arise;


typedef union _REG_MM84B0_Arise    //Third_stream_windows2_chroma_keyH
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_KeyH    :24;
        CBIOS_U32    RESERVED    :8;
    };
}REG_MM84B0_Arise;


typedef union _REG_MM84B4_Arise    //Third_stream_windows2_upsacler_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_Hacc    :21;
        CBIOS_U32    Ts_Win2_Htap_Sel    :1;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    TS_Win2_Alpha_Scale    :1;
        CBIOS_U32    Reserved_1    :7;
    };
}REG_MM84B4_Arise;


typedef union _REG_MM84B8_Arise    //Third_stream_windows2_upsacler_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_Vacc    :21;
        CBIOS_U32    TS_Win2_Vtap_Sel    :1;
        CBIOS_U32    TS_Win2_Vdup_En    :1;
        CBIOS_U32    Reserved    :8;
        CBIOS_U32    TS_Win2_Deint_En    :1;
    };
}REG_MM84B8_Arise;


typedef union _REG_MM33000_Arise    //ChipID_read_request_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :7;
        CBIOS_U32    CHIP_ID_read_request    :1;
        CBIOS_U32    Reserved_1    :24;
    };
}REG_MM33000_Arise;


typedef union _REG_MM33100_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDMIIN_EN    :1;
        CBIOS_U32    BCH_BYPASS    :1;
        CBIOS_U32    Channel_status_sel    :1;
        CBIOS_U32    audio_Mute_value_sel    :1;
        CBIOS_U32    Ctrl_period_length    :4;
        CBIOS_U32    audio_Mute_vale_SW    :1;
        CBIOS_U32    ISRC_int_mode    :1;
        CBIOS_U32    DST_mode    :1;
        CBIOS_U32    one_base_address    :1;
        CBIOS_U32    sw_flush    :1;
        CBIOS_U32    sw_reset    :1;
        CBIOS_U32    counter_en    :1;
        CBIOS_U32    Reserved    :1;
        CBIOS_U32    Line_number    :12;
        CBIOS_U32    video_Mute_value_sel    :1;
        CBIOS_U32    video_Mute_value_SW    :1;
        CBIOS_U32    stable_clear_contidon_1_enable    :1;
        CBIOS_U32    stable_clear_contidon_2_enable    :1;
    };
}REG_MM33100_Arise;


typedef union _REG_MM33104_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PR_SW    :4;
        CBIOS_U32    STRUC_3D_SW    :4;
        CBIOS_U32    CD_SW    :4;
        CBIOS_U32    Y_SW    :2;
        CBIOS_U32    default_phase_SW    :1;
        CBIOS_U32    Interlace_SW    :1;
        CBIOS_U32    HDMI_FORMAT_SW    :3;
        CBIOS_U32    video_software_setting_enable    :1;
        CBIOS_U32    XCLK_NU    :11;
        CBIOS_U32    stable_output_enable    :1;
    };
}REG_MM33104_Arise;


typedef union _REG_MM33108_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VDEN_SW    :12;
        CBIOS_U32    HDEN_SW    :12;
        CBIOS_U32    VBLK_SW    :7;
        CBIOS_U32    stable_clear_contidon_3_enable    :1;
    };
}REG_MM33108_Arise;


typedef union _REG_MM3310C_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    audio_int_size    :27;
    };
}REG_MM3310C_Arise;


typedef union _REG_MM33110_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    BASADR1    :27;
    };
}REG_MM33110_Arise;


typedef union _REG_MM33114_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    LENGTH1    :27;
    };
}REG_MM33114_Arise;


typedef union _REG_MM33118_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    BASADR2    :27;
    };
}REG_MM33118_Arise;


typedef union _REG_MM3311C_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    LENGTH2    :27;
    };
}REG_MM3311C_Arise;


typedef union _REG_MM33120_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    WHDEN    :12;
        CBIOS_U32    WHBACK    :9;
        CBIOS_U32    WVSYNC    :4;
        CBIOS_U32    WVBLK    :7;
    };
}REG_MM33120_Arise;


typedef union _REG_MM33124_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    WVDEN    :12;
        CBIOS_U32    WHFRONT    :12;
        CBIOS_U32    WVBACK    :7;
        CBIOS_U32    INTERLACE    :1;
    };
}REG_MM33124_Arise;


typedef union _REG_MM33128_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    WHSYNC    :9;
        CBIOS_U32    WVFRONT    :5;
        CBIOS_U32    HSYNC_INV    :1;
        CBIOS_U32    VSYNC_INV    :1;
        CBIOS_U32    INT_source_0_enable    :1;
        CBIOS_U32    INT_source_1_enable    :1;
        CBIOS_U32    INT_source_2_enable    :1;
        CBIOS_U32    INT_source_3_enable    :1;
        CBIOS_U32    INT_source_4_enable    :1;
        CBIOS_U32    INT_source_5_enable    :1;
        CBIOS_U32    INT_source_6_enable    :1;
        CBIOS_U32    INT_source_7_enable    :1;
        CBIOS_U32    INT_source_8_enable    :1;
        CBIOS_U32    INT_source_9_enable    :1;
        CBIOS_U32    INT_source_10_enable    :1;
        CBIOS_U32    INT_source_11_enable    :1;
        CBIOS_U32    INT_source_12_enable    :1;
        CBIOS_U32    INT_source_13_enable    :1;
        CBIOS_U32    INT_source_14_enable    :1;
        CBIOS_U32    INT_source_15_enable    :1;
    };
}REG_MM33128_Arise;


typedef union _REG_MM3312C_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUDIO_INT1    :1;
        CBIOS_U32    AUDIO_INT2    :1;
        CBIOS_U32    AVL_GAMUT    :1;
        CBIOS_U32    AVL_ISRC1    :1;
        CBIOS_U32    AVL_ISRC2    :1;
        CBIOS_U32    AVL_ACP    :1;
        CBIOS_U32    ACP_TIMEOUT    :1;
        CBIOS_U32    AVL_GCP    :1;
        CBIOS_U32    FEDGE_EPHY_DATA_OK    :1;
        CBIOS_U32    AVL_INFO_MPEG    :1;
        CBIOS_U32    AVL_INFO_SPD    :1;
        CBIOS_U32    AVL_INFO_VENDOR    :1;
        CBIOS_U32    AVL_INFO_AUDIO    :1;
        CBIOS_U32    AVL_INFO_AVI    :1;
        CBIOS_U32    ERR_INFO_CHECKSUM    :1;
        CBIOS_U32    AVL_ONEBIT    :1;
        CBIOS_U32    AVL_HBR    :1;
        CBIOS_U32    AVL_AUDIO_CSTATUS    :1;
        CBIOS_U32    ERR_AUDIO_PARITY    :1;
        CBIOS_U32    AVL_AUDIO_CK_REG    :1;
        CBIOS_U32    BCH_ERRH    :1;
        CBIOS_U32    BCH_ERR0    :1;
        CBIOS_U32    BCH_ERR1    :1;
        CBIOS_U32    BCH_ERR2    :1;
        CBIOS_U32    BCH_ERR3    :1;
        CBIOS_U32    BCH_CORECTH    :1;
        CBIOS_U32    BCH_CORECT0    :1;
        CBIOS_U32    BCH_CORECT1    :1;
        CBIOS_U32    BCH_CORECT2    :1;
        CBIOS_U32    BCH_CORECT3    :1;
        CBIOS_U32    FEDGE_EPHY_INCLK_OK    :1;
        CBIOS_U32    FEDGE_EPHY_INCLK_LOCK    :1;
    };
}REG_MM3312C_Arise;


typedef union _REG_MM33130_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    LINK_position    :27;
    };
}REG_MM33130_Arise;


typedef union _REG_MM33134_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Recived_Gamut_packet_PB_Bytes_3_0    :32;
    };
}REG_MM33134_Arise;


typedef union _REG_MM33138_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Recived_Gamut_packet_PB_Bytes_7_4    :32;
    };
}REG_MM33138_Arise;


typedef union _REG_MM3313C_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Recived_Gamut_packet_PB_Bytes_11_8    :32;
    };
}REG_MM3313C_Arise;


typedef union _REG_MM33140_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Recived_Gamut_packet_PB_Bytes_15_12    :32;
    };
}REG_MM33140_Arise;


typedef union _REG_MM33144_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Recived_Gamut_packet_PB_Bytes_19_16    :32;
    };
}REG_MM33144_Arise;


typedef union _REG_MM33148_Arise    //CEC1_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CEC_Initiator_Retry_Cnt    :3;
        CBIOS_U32    CEC_Device_Addr    :4;
        CBIOS_U32    CEC_Initiator_Dest_Addr    :4;
        CBIOS_U32    CEC_Initiator_Cmd_Len    :5;
        CBIOS_U32    CEC_Initiator_Broadcast    :1;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    CEC_Enable    :1;
        CBIOS_U32    CEC_Initiator_Cmd_Available    :1;
        CBIOS_U32    cec20_enable    :1;
        CBIOS_U32    pulse_sel    :1;
        CBIOS_U32    error_detect_sel    :1;
        CBIOS_U32    fall_to_rise_error_en    :1;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    cec_start_total_l_para_reg    :7;
    };
}REG_MM33148_Arise;


typedef union _REG_MM3314C_Arise    //CEC1_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CEC_Initiator_Command    :32;
    };
}REG_MM3314C_Arise;


typedef union _REG_MM33150_Arise    //CEC1_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Initiator_Transmit_Finished    :1;
        CBIOS_U32    Initiator_Transmit_Succeed    :1;
        CBIOS_U32    Reserved    :1;
        CBIOS_U32    Follower_Received_Source_Addr    :4;
        CBIOS_U32    Follower_Received_Broadcast    :1;
        CBIOS_U32    Follower_Received_Cmd_Len    :5;
        CBIOS_U32    Follower_Received_Ready    :1;
        CBIOS_U32    Debug_use    :18;
    };
}REG_MM33150_Arise;


typedef union _REG_MM33154_Arise    //CEC1_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Follower_Recvd_Command    :32;
    };
}REG_MM33154_Arise;


typedef union _REG_MM33158_Arise    //CEC1_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    cec_start_para_reg    :7;
        CBIOS_U32    cec_logic0_para_reg    :7;
        CBIOS_U32    cec_logic1_para_reg    :7;
        CBIOS_U32    cec_error_para_reg    :7;
        CBIOS_U32    cec_f2r_error_para4_reg    :2;
        CBIOS_U32    reserved    :1;
        CBIOS_U32    cec_timing_para_sel    :1;
    };
}REG_MM33158_Arise;


typedef union _REG_MM3315C_Arise    //CEC1_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    cec_ack0_para_reg    :7;
        CBIOS_U32    cec_ack1_para_reg    :7;
        CBIOS_U32    cec_start_rise_l_para_reg    :7;
        CBIOS_U32    cec_start_rise_h_para_reg    :7;
        CBIOS_U32    cec_f2r_error_para3_reg    :4;
    };
}REG_MM3315C_Arise;


typedef union _REG_MM33160_Arise    //CEC1_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    cec_start_total_h_para_reg    :7;
        CBIOS_U32    cec_ack_total_para_reg    :7;
        CBIOS_U32    cec_error_total_para_reg    :7;
        CBIOS_U32    cec_data_strobe_para_reg    :7;
        CBIOS_U32    cec_f2r_error_para4_reg    :4;
    };
}REG_MM33160_Arise;


typedef union _REG_MM33164_Arise    //CEC1_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    cec_wr_para_reg    :7;
        CBIOS_U32    cec_f2f_error_para_reg    :7;
        CBIOS_U32    cec_f2r_error_para1_reg    :7;
        CBIOS_U32    cec_f2r_error_para2_reg    :7;
        CBIOS_U32    cec_f2r_error_para3_reg    :3;
        CBIOS_U32    cec_f2r_error_para4_reg    :1;
    };
}REG_MM33164_Arise;


typedef union _REG_MM33168_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_ISRC2_packet_PB_Bytes_11_8    :32;
    };
}REG_MM33168_Arise;


typedef union _REG_MM3316C_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_ISRC2_packet_PB_Bytes_15_12    :32;
    };
}REG_MM3316C_Arise;


typedef union _REG_MM33170_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_ACP_packet_PB_Byte_0_3    :32;
    };
}REG_MM33170_Arise;


typedef union _REG_MM33174_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_ACP_packet_PB_Byte_7_4    :32;
    };
}REG_MM33174_Arise;


typedef union _REG_MM33178_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_ACP_packet_PB_Byte_11_8    :32;
    };
}REG_MM33178_Arise;


typedef union _REG_MM3317C_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_ACP_packet_PB_Byte_15_12    :32;
    };
}REG_MM3317C_Arise;


typedef union _REG_MM33180_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_MPEG_info_packet_PB_Byte_0_3    :32;
    };
}REG_MM33180_Arise;


typedef union _REG_MM33184_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_MPEG_info_packet_PB_Byte_7_4    :32;
    };
}REG_MM33184_Arise;


typedef union _REG_MM33188_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_MPEG_info_packet_PB_Byte_9_8    :16;
        CBIOS_U32    Received_MPEG_info_packet_version    :8;
        CBIOS_U32    Received_SPD_info_packet_version    :8;
    };
}REG_MM33188_Arise;


typedef union _REG_MM3318C_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_Gamut_packet_HB_value    :16;
        CBIOS_U32    Received_ISRC1_packet_HB    :8;
        CBIOS_U32    Received_ACP_packet_type    :8;
    };
}REG_MM3318C_Arise;


typedef union _REG_MM33190_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_SPD_packet_information_Byte_0_3    :32;
    };
}REG_MM33190_Arise;


typedef union _REG_MM33194_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_SPD_packet_information_Byte_7_4    :32;
    };
}REG_MM33194_Arise;


typedef union _REG_MM33198_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_SPD_packet_information_Byte_11_8    :32;
    };
}REG_MM33198_Arise;


typedef union _REG_MM3319C_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_SPD_packet_information_Byte_15_12    :32;
    };
}REG_MM3319C_Arise;


typedef union _REG_MM331A0_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_SPD_packet_information_Byte_19_16    :32;
    };
}REG_MM331A0_Arise;


typedef union _REG_MM331A4_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_SPD_packet_information_Byte_23_20    :32;
    };
}REG_MM331A4_Arise;


typedef union _REG_MM331A8_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_Audio_info_packet_PB_byte9_8    :16;
        CBIOS_U32    Received_Audio_info_packet_version    :8;
        CBIOS_U32    Received_SPD_info_packet_PB_byte_24    :8;
    };
}REG_MM331A8_Arise;


typedef union _REG_MM331AC_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_VENDOR_info_packet_PB_byte3_0    :32;
    };
}REG_MM331AC_Arise;


typedef union _REG_MM331B0_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_VENDOR_info_packet_PB_byte7_4    :32;
    };
}REG_MM331B0_Arise;


typedef union _REG_MM331B4_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_VENDOR_info_packet_PB_byte11_8    :32;
    };
}REG_MM331B4_Arise;


typedef union _REG_MM331B8_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_VENDOR_info_packet_PB_byte15_12    :32;
    };
}REG_MM331B8_Arise;


typedef union _REG_MM331BC_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_VENDOR_info_packet_PB_byte19_16    :32;
    };
}REG_MM331BC_Arise;


typedef union _REG_MM331C0_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_VENDOR_info_packet_PB_byte23_20    :32;
    };
}REG_MM331C0_Arise;


typedef union _REG_MM331C4_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_VENDOR_info_packet_PB_byte26_24    :24;
        CBIOS_U32    Received_VENDOR_info_packet_version    :8;
    };
}REG_MM331C4_Arise;


typedef union _REG_MM331C8_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_Audio_info_packet_PB_byte3_0    :32;
    };
}REG_MM331C8_Arise;


typedef union _REG_MM331CC_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_Audio_info_packet_PB_7_4    :32;
    };
}REG_MM331CC_Arise;


typedef union _REG_MM331D0_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_AVI_info_packet_PB_byte3_0    :32;
    };
}REG_MM331D0_Arise;


typedef union _REG_MM331D4_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_AVI_info_packet_PB_byte7_4    :32;
    };
}REG_MM331D4_Arise;


typedef union _REG_MM331D8_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_AVI_info_packet_PB_byte11_8    :32;
    };
}REG_MM331D8_Arise;


typedef union _REG_MM331DC_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_AVI_info_packet_PB_byte_12    :8;
        CBIOS_U32    Received_AVI_info_packet_version_value    :8;
        CBIOS_U32    INT_source_16_enable    :1;
        CBIOS_U32    INT_source_17_enable    :1;
        CBIOS_U32    INT_source_18_enable    :1;
        CBIOS_U32    INT_source_19_enable    :1;
        CBIOS_U32    INT_source_20_enable    :1;
        CBIOS_U32    INT_source_21_enable    :1;
        CBIOS_U32    INT_source_22_enable    :1;
        CBIOS_U32    INT_source_23_enable    :1;
        CBIOS_U32    INT_source_24_enable    :1;
        CBIOS_U32    SEL_SUB_CSTATUS    :2;
        CBIOS_U32    VSYNC_NU    :1;
        CBIOS_U32    range    :4;
    };
}REG_MM331DC_Arise;


typedef union _REG_MM331E0_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_Channel_status_byte_3_0    :32;
    };
}REG_MM331E0_Arise;


typedef union _REG_MM331E4_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_Channel_status_byte_7_4    :32;
    };
}REG_MM331E4_Arise;


typedef union _REG_MM331E8_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_Channel_status_byte_11_8    :32;
    };
}REG_MM331E8_Arise;


typedef union _REG_MM331EC_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_Channel_status_byte_15_12    :32;
    };
}REG_MM331EC_Arise;


typedef union _REG_MM331F0_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_Channel_status_byte_19_16    :32;
    };
}REG_MM331F0_Arise;


typedef union _REG_MM331F4_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_Channel_status_byte_23_20    :32;
    };
}REG_MM331F4_Arise;


typedef union _REG_MM331F8_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_user_data_value_byte_3_0    :32;
    };
}REG_MM331F8_Arise;


typedef union _REG_MM331FC_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_user_data_value_byte_7_4    :32;
    };
}REG_MM331FC_Arise;


typedef union _REG_MM33200_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_user_data_value_byte_11_8    :32;
    };
}REG_MM33200_Arise;


typedef union _REG_MM33204_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_user_data_value_byte_15_12    :32;
    };
}REG_MM33204_Arise;


typedef union _REG_MM33208_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_user_data_value_byte_19_16    :32;
    };
}REG_MM33208_Arise;


typedef union _REG_MM3320C_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Received_user_data_value_byte_23_20    :32;
    };
}REG_MM3320C_Arise;


typedef union _REG_MM33210_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUDIO_CK_CTS    :20;
        CBIOS_U32    GCP_PP    :4;
        CBIOS_U32    GCP_CD    :4;
        CBIOS_U32    GCP_default_PHASE    :1;
        CBIOS_U32    GCP_AVMUTE    :1;
        CBIOS_U32    STABLE_OUT    :1;
        CBIOS_U32    LAYOUT_REG    :1;
    };
}REG_MM33210_Arise;


typedef union _REG_MM33214_Arise    //HDMI_IN_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    AUDIO_CK_N    :20;
        CBIOS_U32    DEEPC_CDOUT    :4;
        CBIOS_U32    DET_PASS0    :1;
        CBIOS_U32    DET_PASS1    :1;
        CBIOS_U32    DET_PASS2    :1;
        CBIOS_U32    FLATDONTW    :1;
        CBIOS_U32    LSBZERO    :1;
        CBIOS_U32    FLAT2ZERO    :1;
        CBIOS_U32    HUNSAME    :1;
        CBIOS_U32    AVL_DST    :1;
    };
}REG_MM33214_Arise;


typedef union _REG_MM33218_Arise    //HDMI_IN_Ephy_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FHH_EPHYMSTEN    :1;
        CBIOS_U32    FHH_BG_OW    :1;
        CBIOS_U32    FHH_BG_TRIM    :2;
        CBIOS_U32    FHP_MPLL0_CP    :3;
        CBIOS_U32    FHH_MPLL_LOCK_EN    :1;
        CBIOS_U32    FHHS0_RTNRXSET    :4;
        CBIOS_U32    FHH_RTNBIST    :2;
        CBIOS_U32    FHHS0_RTNRXSETEN    :1;
        CBIOS_U32    FHH00_DATA_OW    :1;
        CBIOS_U32    FHH01_DATA_OW    :1;
        CBIOS_U32    FHH02_DATA_OW    :1;
        CBIOS_U32    FHH_INCLK_LOCK_EN    :1;
        CBIOS_U32    FHH_INCLK_OK_EN    :1;
        CBIOS_U32    FHH_DATADETVTH    :3;
        CBIOS_U32    FHH_CLKDETVTH    :3;
        CBIOS_U32    FHH00_RXPWRSET    :2;
        CBIOS_U32    FHH01_RXPWRSET    :2;
        CBIOS_U32    FHH02_RXPWRSET    :2;
    };
}REG_MM33218_Arise;


typedef union _REG_MM3321C_Arise    //HDMI_IN_Ephy_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FHH00_RXPWRSETEN    :1;
        CBIOS_U32    FHH01_RXPWRSETEN    :1;
        CBIOS_U32    FHH02_RXPWRSETEN    :1;
        CBIOS_U32    FHH_RXCLKSEL    :3;
        CBIOS_U32    FHH_VCOPDSEL    :1;
        CBIOS_U32    FHH_RCVPDSEL    :1;
        CBIOS_U32    FHH_IVCPSEL    :3;
        CBIOS_U32    FHH00_FTXSWNSETEN    :1;
        CBIOS_U32    FHH01_FTXSWNSETEN    :1;
        CBIOS_U32    FHH02_FTXSWNSETEN    :1;
        CBIOS_U32    FHH00_FTXSWNSET    :2;
        CBIOS_U32    FHH01_FTXSWNSET    :2;
        CBIOS_U32    FHH02_FTXSWNSET    :2;
        CBIOS_U32    FHH00_DFC_OWEN    :1;
        CBIOS_U32    FHH01_DFC_OWEN    :1;
        CBIOS_U32    FHH02_DFC_OWEN    :1;
        CBIOS_U32    FHH_CDR_UGB_HBW    :1;
        CBIOS_U32    FHH_RTN_VTH    :2;
        CBIOS_U32    FHHc_RXPWRSETEN    :1;
        CBIOS_U32    FHHS_RCVVCM_CLKLANE    :1;
        CBIOS_U32    FHHc_RXPWRSET    :2;
        CBIOS_U32    FHH_DFC_VTH    :2;
    };
}REG_MM3321C_Arise;


typedef union _REG_MM33220_Arise    //HDMI_IN_Ephy_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FHH_EQTNMODE    :4;
        CBIOS_U32    FHH_Eqtndcbw    :4;
        CBIOS_U32    FHH_Eqtnhfbw    :4;
        CBIOS_U32    FHH_Eqtnosbw    :4;
        CBIOS_U32    FHH_Eqtnwtbw    :4;
        CBIOS_U32    FHHs0_Eqtndfe    :2;
        CBIOS_U32    FHH_Eqtndiven    :1;
        CBIOS_U32    Eqtnen_Lf    :1;
        CBIOS_U32    Eqtnalwy    :1;
        CBIOS_U32    FHH0_Eqtnspdswen    :1;
        CBIOS_U32    FHHS0_EQTNDFETRN    :1;
        CBIOS_U32    FHHS0_DCSETEN    :1;
        CBIOS_U32    FHHS0_EQTNVTH    :3;
        CBIOS_U32    FHHS0_HFSETEN    :1;
    };
}REG_MM33220_Arise;


typedef union _REG_MM33224_Arise    //HDMI_IN_Ephy_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FHHS0_DCSET    :5;
        CBIOS_U32    FHHS0_HFSET    :5;
        CBIOS_U32    FHHS0_OSSET    :5;
        CBIOS_U32    FHHS0_W1SET    :5;
        CBIOS_U32    FHHS0_W2SET    :5;
        CBIOS_U32    FHHS0_OSSETEN    :1;
        CBIOS_U32    FHHS0_W1SETEN    :1;
        CBIOS_U32    FHHS0_W2SETEN    :1;
        CBIOS_U32    FHH_EQTNBISTEVT    :3;
        CBIOS_U32    FHH_EQTNBISTEN    :1;
    };
}REG_MM33224_Arise;


typedef union _REG_MM33228_Arise    //HDMI_IN_Ephy_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FHHS0_FDRATIO    :3;
        CBIOS_U32    FHHS0_FTSRATIO    :3;
        CBIOS_U32    FHHS0_P1EXT    :3;
        CBIOS_U32    FHHS0_P0SEXT    :3;
        CBIOS_U32    FHHS0_NIPRSEL    :2;
        CBIOS_U32    FHHS0_KIPRSEL    :2;
        CBIOS_U32    FHHS0_NIPCSEL    :2;
        CBIOS_U32    FHHS0_KIPCSEL    :2;
        CBIOS_U32    FHHS0_NLFSEL    :2;
        CBIOS_U32    FHHS0_KLFSEL    :2;
        CBIOS_U32    FHHS0_NTSEL    :1;
        CBIOS_U32    FHHS0_KTSEL    :1;
        CBIOS_U32    FHHS0_PWUP    :2;
        CBIOS_U32    FHHS0_DIVTMSETEN    :1;
        CBIOS_U32    FHS0_DIVICPSETEN    :1;
        CBIOS_U32    FHS0_DFC_VCOST    :2;
    };
}REG_MM33228_Arise;


typedef union _REG_MM3322C_Arise    //HDMI_IN_Ephy_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    EPH00_DCOUT    :5;
        CBIOS_U32    EPH01_DCOUT    :5;
        CBIOS_U32    EPH02_DCOUT    :5;
        CBIOS_U32    Reserved    :1;
        CBIOS_U32    FHHSc_KTSEL    :1;
        CBIOS_U32    FHHSc_KLFSEL    :2;
        CBIOS_U32    RAWSPIDF_SW    :1;
        CBIOS_U32    NPCM_BIT_SW    :1;
        CBIOS_U32    AUDIO_BIT_SW    :1;
        CBIOS_U32    EN_SW_AUDIO    :1;
        CBIOS_U32    NPCM_mode_SW    :1;
        CBIOS_U32    CA_SW    :8;
    };
}REG_MM3322C_Arise;


typedef union _REG_MM33230_Arise    //HDMI_IN_Ephy_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    EPH_BG_OK    :1;
        CBIOS_U32    EPH_MPLL_OK    :1;
        CBIOS_U32    EPHS0_RTNRXOUT    :4;
        CBIOS_U32    EPH_INCLK_LOCKED    :1;
        CBIOS_U32    EPH_INCLK_OK    :1;
        CBIOS_U32    EPH00_DIV    :2;
        CBIOS_U32    EPH01_DIV    :2;
        CBIOS_U32    EPH02_DIV    :2;
        CBIOS_U32    FHHc_VCO_BAND    :1;
        CBIOS_U32    FHH02_VCO_BAND    :1;
        CBIOS_U32    FHH01_VCO_BAND    :1;
        CBIOS_U32    FHH00_VCO_BAND    :1;
        CBIOS_U32    EPHc_DIV    :2;
        CBIOS_U32    EPHc_DFC_WORK    :1;
        CBIOS_U32    EPH02_DFC_WORK    :1;
        CBIOS_U32    EPH01_DFC_WORK    :1;
        CBIOS_U32    EPH00_DFC_WORK    :1;
        CBIOS_U32    EPH02_RCAD_OK    :1;
        CBIOS_U32    EPH01_RCAD_OK    :1;
        CBIOS_U32    EPH00_RCAD_OK    :1;
        CBIOS_U32    TRUE_RCAD_OK    :1;
        CBIOS_U32    MASTER_OK    :1;
        CBIOS_U32    EPH00_DATA_OK    :1;
        CBIOS_U32    EPH01_DATA_OK    :1;
        CBIOS_U32    EPH02_DATA_OK    :1;
    };
}REG_MM33230_Arise;


typedef union _REG_MM33234_Arise    //HDMI_IN_Ephy_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    EPH00_HFOUT    :5;
        CBIOS_U32    EPH01_HFOUT    :5;
        CBIOS_U32    EPH02_HFOUT    :5;
        CBIOS_U32    EPH00_OSOUT    :5;
        CBIOS_U32    EPH01_OSOUT    :5;
        CBIOS_U32    EPH02_OSOUT    :5;
        CBIOS_U32    EPHS0_DFCRSTB    :1;
        CBIOS_U32    EPHSc_DFCRSTB    :1;
    };
}REG_MM33234_Arise;


typedef union _REG_MM33238_Arise    //HDMI_IN_Ephy_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    EPH00_W1OUT    :5;
        CBIOS_U32    EPH01_W1OUT    :5;
        CBIOS_U32    EPH02_W1OUT    :5;
        CBIOS_U32    EPH00_W2OUT    :5;
        CBIOS_U32    EPH01_W2OUT    :5;
        CBIOS_U32    EPH02_W2OUT    :5;
        CBIOS_U32    LPH00_EQTNEN    :1;
        CBIOS_U32    LPH00_EQTNRSTB    :1;
    };
}REG_MM33238_Arise;


typedef union _REG_MM3323C_Arise    //HDMI_IN_Ephy_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LPH_MPLLPDB    :1;
        CBIOS_U32    LPHS0_RTNPDB    :1;
        CBIOS_U32    LPHS0_RTNRSTB    :1;
        CBIOS_U32    LPHS0_RTNEN    :1;
        CBIOS_U32    LPH00_DATADETPDB    :1;
        CBIOS_U32    LPH01_DATADETPDB    :1;
        CBIOS_U32    LPH02_DATADETPDB    :1;
        CBIOS_U32    LPH_CLKDETPDB    :1;
        CBIOS_U32    LPH00_RXPWR    :2;
        CBIOS_U32    LPH01_RXPWR    :2;
        CBIOS_U32    LPH02_RXPWR    :2;
        CBIOS_U32    LPHc_RXPWR    :2;
        CBIOS_U32    LPH_BG_PDB    :1;
        CBIOS_U32    EQTNEN0_SW    :1;
        CBIOS_U32    EQTNEN1_SW    :1;
        CBIOS_U32    EQTNEN2_SW    :1;
        CBIOS_U32    EQTNRSTB0_SW    :1;
        CBIOS_U32    EQTNRSTB1_SW    :1;
        CBIOS_U32    EQTNRSTB2_SW    :1;
        CBIOS_U32    LPH00_FTXSWN    :2;
        CBIOS_U32    LPH01_FTXSWN    :2;
        CBIOS_U32    LPH02_FTXSWN    :2;
        CBIOS_U32    LPHc_FTXSWN    :2;
        CBIOS_U32    EQ_SoftWare_Setting_enable    :1;
    };
}REG_MM3323C_Arise;


typedef union _REG_MM33240_Arise    //HDMI_IN__register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    WFRAME    :32;
    };
}REG_MM33240_Arise;


typedef union _REG_MM33244_Arise    //HDMI_IN_Ephy_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FHH00_DFC_OW    :3;
        CBIOS_U32    FHH01_DFC_OW    :3;
        CBIOS_U32    FHH02_DFC_OW    :3;
        CBIOS_U32    FHH_PFDDBW    :2;
        CBIOS_U32    FHHc_FTXSWNSETEN    :1;
        CBIOS_U32    FHHc_FTXSWNSET    :2;
        CBIOS_U32    FHHc_DFC_OW    :3;
        CBIOS_U32    FHHc_DFC_OWEN    :1;
        CBIOS_U32    FHHS_PFD_ICP    :2;
        CBIOS_U32    FHHS_PFD_ICP_EN    :1;
        CBIOS_U32    FHHSc_NIPRSEL    :2;
        CBIOS_U32    FHHSc_KIPRSEL    :2;
        CBIOS_U32    FHHSc_NIPCSEL    :2;
        CBIOS_U32    FHHSc_KIPCSEL    :2;
        CBIOS_U32    FHHSc_NTSEL    :1;
        CBIOS_U32    FHHSc_NLFSEL    :2;
    };
}REG_MM33244_Arise;


typedef union _REG_MM33248_Arise    //STREAM_Process_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FRC_REG0    :32;
    };
}REG_MM33248_Arise;


typedef union _REG_MM3324C_Arise    //STREAM_Process_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FRC_REG1    :32;
    };
}REG_MM3324C_Arise;


typedef union _REG_MM33250_Arise    //STREAM_Process_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FRC_REG2    :32;
    };
}REG_MM33250_Arise;


typedef union _REG_MM33254_Arise    //STREAM_Process_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FRC_REG3    :32;
    };
}REG_MM33254_Arise;


typedef union _REG_MM33258_Arise    //STREAM_Process_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FRC_REG4    :32;
    };
}REG_MM33258_Arise;


typedef union _REG_MM3325C_Arise   //STREAM_Process_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FRC_REG5    :32;
    };
}REG_MM3325C_Arise;


typedef union _REG_MM33260_Arise    //STREAM_Process_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FRC_REG6    :32;
    };
}REG_MM33260_Arise;


typedef union _REG_MM33264_Arise    //STREAM_Process_register_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FRC_REG7    :32;
    };
}REG_MM33264_Arise;


typedef union _REG_MM33268_Arise    //HDMI_IN__Registers_Group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    page_flip_mode    :1;
        CBIOS_U32    Allign_enable    :1;
        CBIOS_U32    TERC4_Error_0    :1;
        CBIOS_U32    TERC4_Error_1    :1;
        CBIOS_U32    TERC4_Error_2    :1;
        CBIOS_U32    TMDS_state_error    :1;
        CBIOS_U32    Data_island_timeout    :1;
        CBIOS_U32    Data_island_length_error    :1;
        CBIOS_U32    Allign_error    :1;
        CBIOS_U32    CK_COUNT_Enable    :1;
        CBIOS_U32    EDID_R_W_enable    :1;
        CBIOS_U32    IIC_EDID_RW_EN    :1;
        CBIOS_U32    DRV_FORCE_WEN    :1;
        CBIOS_U32    Reserved    :4;
        CBIOS_U32    EDID_Status    :4;
        CBIOS_U32    HDMIIN_TestPass_Status    :3;
        CBIOS_U32    HDMIIN_SEL0    :2;
        CBIOS_U32    HDMIIN_SEL1    :2;
        CBIOS_U32    HDMIIN_SEL2    :2;
        CBIOS_U32    HDMIIN_FIRST_ALLIGN    :1;
        CBIOS_U32    HDMIIN_ALLIGN    :1;
    };
}REG_MM33268_Arise;


typedef union _REG_MM3326C_Arise
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    ctl_byte     :16;
        CBIOS_U32    Reserved_0    :5;
        CBIOS_U32    reverse_Hsync_signal    :1;
        CBIOS_U32    reverse_Vsync_signal    :1;
        CBIOS_U32    _3d_mode_enable    :1;
        CBIOS_U32    _3d_frame_identification    :1;
        CBIOS_U32    phy_switch    :1;
        CBIOS_U32    pixel_source    :2;
        CBIOS_U32    lane_mode    :2;
        CBIOS_U32    byte_mode    :1;
        CBIOS_U32    Reserved_1    :1;
    };
}REG_MM3326C_Arise;


typedef union _REG_MM33270_Arise
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    padding_counter    :30;
        CBIOS_U32    padding__type    :2;
    };
}REG_MM33270_Arise;


typedef union _REG_MM33274_Arise
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :28;
        CBIOS_U32    vx1_state    :2;
        CBIOS_U32    manual_boot_up    :1;
        CBIOS_U32    en_vx1    :1;
    };
}REG_MM33274_Arise;


typedef union _REG_MM33278_Arise    //diu_downscaler_register_group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Enable_Work_Register    :1;
        CBIOS_U32    Vsync_Off_Flip    :1;
        CBIOS_U32    Reserved    :30;
    };
}REG_MM33278_Arise;


typedef union _REG_MM3327C_Arise    //diu_downscaler_register_group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DOWNSCALING_EN    :1;
        CBIOS_U32    _3D_LEFT_RIGHT    :1;
        CBIOS_U32    REG_HOR_INC    :1;
        CBIOS_U32    REG_VER_MODULAR    :1;
        CBIOS_U32    VER_INC    :4;
        CBIOS_U32    HOR_INC    :4;
        CBIOS_U32    VER_MODULAR    :4;
        CBIOS_U32    HOR_MODULAR    :5;
        CBIOS_U32    DST_FORMAT    :1;
        CBIOS_U32    SCALING_RCP    :10;
    };
}REG_MM3327C_Arise;


typedef union _REG_MM33280_Arise    //diu_downscaler_register_group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    clip_left_11    :1;
        CBIOS_U32    REG_HOR_MODULAR_5    :1;
        CBIOS_U32    clip_right_12    :1;
        CBIOS_U32    REG_DAT_FORMAT    :1;
        CBIOS_U32    DOUBLE_FB    :1;
        CBIOS_U32    SCALING_SW_RESET    :1;
        CBIOS_U32    FIFO_RST_EN    :1;
        CBIOS_U32    REG_SCL_MODE_0    :1;
        CBIOS_U32    REG_SCL_MODE_1    :1;          //write back enable bit
        CBIOS_U32    clip_left_10to0    :11;
        CBIOS_U32    clip_right_11to0    :12;
    };
}REG_MM33280_Arise;


typedef union _REG_MM33284_Arise    //diu_downscaler_register_group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HIGH_THRESHOLD_7_6    :2;
        CBIOS_U32    LOW_THRESHOLD_7_6    :2;
        CBIOS_U32    REG_YCBCR422_MAP    :1;
        CBIOS_U32    REG_DROP_FRAME    :1;
        CBIOS_U32    reserved_0    :1;
        CBIOS_U32    BASE_ADDR_29_26    :4;
        CBIOS_U32    reserved_1    :9;
        CBIOS_U32    HIGH_THRESHOLD    :6;
        CBIOS_U32    LOW_THRESHOLD    :6;
    };
}REG_MM33284_Arise;


typedef union _REG_MM33288_Arise    //diu_downscaler_register_group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :3;
        CBIOS_U32    STRIDE    :9;
        CBIOS_U32    OFFSET    :20;
    };
}REG_MM33288_Arise;


typedef union _REG_MM3328C_Arise    //MDI_time_out_reg
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_time_out    :8;
        CBIOS_U32    SS1_time_out    :8;
        CBIOS_U32    TS1_time_out    :8;
        CBIOS_U32    QS1_time_out    :8;
    };
}REG_MM3328C_Arise;


typedef union _REG_MM33290_Arise    //iga1_centre_mode_register, PS dst window
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    WriteBackBaseAddress   :30;
        CBIOS_U32    Reserved    :2;
    };
}REG_MM33290_Arise;


typedef union _REG_MM33294_Arise    //iga1_centre_mode_register, PS dst window
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Center_Vde_Begin    :13;
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    Center_Vde_End    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM33294_Arise;


typedef union _REG_MM332A0_Arise    //ss1_plane_alpha
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ovl1_pPlane_Alpha    :8;
        CBIOS_U32    Reserved_0    :6;
        CBIOS_U32    Ovl1_Alpha_Blend_Mode    :1;
        CBIOS_U32    Reserved_1    :17;
    };
}REG_MM332A0_Arise;


typedef union _REG_MM332A4_Arise    //ts1_plane_alpha
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ovl2_Plane_Alpha    :8;
        CBIOS_U32    Reserved_0    :6;
        CBIOS_U32    Ovl2_Alpha_Blend_Mode    :1;
        CBIOS_U32    Reserved_1    :17;
    };
}REG_MM332A4_Arise;


typedef union _REG_MM332A8_Arise    //GPIO_REN_and_DAC_mode_en
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FPVEE_FPVDD_pinmux_oen    :1;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    DAC_mode_enable    :1;
        CBIOS_U32    Reserved_1    :29;
    };
}REG_MM332A8_Arise;


typedef union _REG_MM332AC_Arise    //DAC_SENSE_Read_and_LUT_read_pointer_control
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LUT_SSC_HDMI_INFO_DP_INFO_write_current_index    :8;
        CBIOS_U32    LUT_SSC_HDMI_INFO_DP_INFO_Read_current_index    :8;
        CBIOS_U32    reserved    :8;
        CBIOS_U32    Reserved    :4;
        CBIOS_U32    DAC1_SENSE_C    :1;
        CBIOS_U32    DAC1_SENSE_R    :1;
        CBIOS_U32    DAC1_SENSE_G    :1;
        CBIOS_U32    DAC1_SENSE_B    :1;
    };
}REG_MM332AC_Arise;


typedef union _REG_MM332B0_Arise    //MDI_time_out_reg_2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_time_out    :8;
        CBIOS_U32    SS2_time_out    :8;
        CBIOS_U32    TS2_time_out    :8;
        CBIOS_U32    QS2_time_out    :8;
    };
}REG_MM332B0_Arise;


typedef union _REG_MM332B4_Arise    //MDI_time_out_reg_3
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP0_time_out    :8;
        CBIOS_U32    Reserved_0    :8;
        CBIOS_U32    Reserved_1    :8;
        CBIOS_U32    VIP1_time_out    :8;
    };
}REG_MM332B4_Arise;


typedef union _REG_MM332B8_Arise    //MDI_time_out_reg_4
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS4_timtout    :8;
        CBIOS_U32    SS4_timeout    :8;
        CBIOS_U32    TS4_timeout    :8;
        CBIOS_U32    QS4_timeout    :8;
    };
}REG_MM332B8_Arise;


typedef union _REG_MM332BC_Arise    //QS_overlay_register_1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ka_3to0_Or_Ks    :4;
        CBIOS_U32    Ka_7to4_Or_Kp    :4;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    Ovl3_Input_Stream    :2;
        CBIOS_U32    Reserved_1    :12;
        CBIOS_U32    Ovl3_Key_Mode_Sel    :4;
        CBIOS_U32    Invert_Alpha_Or_Ka    :1;
        CBIOS_U32    Ovl3_One_Shot    :1;
        CBIOS_U32    Ovl3_Vsync_Off_Flip    :1;
        CBIOS_U32    Ovl3_Enable_Work    :1;
    };
}REG_MM332BC_Arise;


typedef union _REG_MM332C0_Arise    //QS1_overlay_register_2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ovl3_Plane_Alpha    :8;
        CBIOS_U32    Reserved_0    :6;
        CBIOS_U32    Ovl3_Alpha_Blend_Mode    :1;
        CBIOS_U32    Reserved_1    :17;
    };
}REG_MM332C0_Arise;


typedef union _REG_MM332C4_Arise    //NV12C_base_address
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    BASE_ADDR_C    :27;
    };
}REG_MM332C4_Arise;


typedef union _REG_MM332C8_Arise    //EFUSE_redundancy_data_0_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DRV_EFUSE_R_DATA0    :32;
    };
}REG_MM332C8_Arise;


typedef union _REG_MM332CC_Arise    //EFUSE_redundancy_data_1_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DRV_EFUSE_R_DATA1    :32;
    };
}REG_MM332CC_Arise;


typedef union _REG_MM332D0_Arise    //EFUSE_redundancy_data_2_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DRV_EFUSE_R_DATA2    :32;
    };
}REG_MM332D0_Arise;


typedef union _REG_MM332D4_Arise    //EFUSE_redundancy_data_3_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DRV_EFUSE_R_DATA3    :32;
    };
}REG_MM332D4_Arise;


typedef union _REG_MM332D8_Arise   //EFUSE_redundancy_flag_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DRV_EFUSE_RF    :8;
        CBIOS_U32    DRV_EFUSE_R_READ_ST    :2;
        CBIOS_U32    Reserved    :22;
    };
}REG_MM332D8_Arise;


typedef union _REG_MM332DC_Arise    //HDCP_EFUSE_driver_programming_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PGM_EFUSE_ADDR    :12;
        CBIOS_U32    Reserved_0    :4;
        CBIOS_U32    PGM_EFUSE_TIME    :9;
        CBIOS_U32    Reserved_1    :4;
        CBIOS_U32    PGM_EFUSE_TIME_SEL    :1;
        CBIOS_U32    PGM_EFUSE_FINISH    :1;
        CBIOS_U32    PGM_EFUSE_REQ    :1;
    };
}REG_MM332DC_Arise;


typedef union _REG_MM332E0_Arise    //SS1_SCL_H_Reg
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_Hacc    :21;
        CBIOS_U32    SS_Htap_Sel    :1;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    SS_Alpha_Ups_En    :1;
        CBIOS_U32    Reserved_1    :7;
    };
}REG_MM332E0_Arise;


typedef union _REG_MM332E4_Arise    //SS1_SCL_V_Reg
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_Vacc    :21;
        CBIOS_U32    SS_Vtap_Sel    :1;
        CBIOS_U32    SS_Vdup_En    :1;
        CBIOS_U32    Reserved    :8;
        CBIOS_U32    SS_Vint_En    :1;
    };
}REG_MM332E4_Arise;


typedef union _REG_MM332E8_Arise    //SS_KEYH_for_chroma_key
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_KeyH    :24;
        CBIOS_U32    Reserved    :8;
    };
}REG_MM332E8_Arise;


typedef union _REG_MM332F0_Arise    //MDI_timeout_reg_5
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TVW0_timeout    :8;
        CBIOS_U32    TVW1_timeout    :8;
        CBIOS_U32    TVW2_timeout    :8;
        CBIOS_U32    TVW3_timeout    :8;
    };
}REG_MM332F0_Arise;


typedef union _REG_MM332F4_Arise    //MDI_timeout_reg_6
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP0_time_out    :8;
        CBIOS_U32    VIP1_time_out    :8;
        CBIOS_U32    VIP2_time_out    :8;
        CBIOS_U32    VIP3_time_out    :8;
    };
}REG_MM332F4_Arise;


typedef union _REG_MM332F8_Arise    //MDI_timeout_reg_7
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDA_timeout    :8;
        CBIOS_U32    Reserved    :24;
    };
}REG_MM332F8_Arise;


typedef union _REG_MM332FC_Arise    //MDI_timeout_disable_reg
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    read_clients_timeout_disable    :16;
        CBIOS_U32    write_clients_timeout_disable    :8;
        CBIOS_U32    HDA_timeout_disable    :1;
        CBIOS_U32    Reserved    :7;
    };
}REG_MM332FC_Arise;


typedef union _REG_MM33300_Arise    //
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    s_vtotal    :13;
        CBIOS_U32    reserved_0    :3;
        CBIOS_U32    s_vdisp_end    :13;
        CBIOS_U32    TIM_USE_MMIO    :1;
        CBIOS_U32    reserved_1    :2;
    };
}REG_MM33300_Arise;


typedef union _REG_MM33304_Arise    //
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    s_vblnk_beg    :13;
        CBIOS_U32    reserved_0    :3;
        CBIOS_U32    s_vblnk_end    :9;
        CBIOS_U32    reserved_1    :7;
    };
}REG_MM33304_Arise;


typedef union _REG_MM33318_Arise    //SCL_SD_Width_Ratio_Reg
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_Scl_Width_Ratio    :21;
        CBIOS_U32    Reserved    :9;
        CBIOS_U32    PS_Htap_Sel    :1;
        CBIOS_U32    PS_Alpha_Upen    :1;
    };
}REG_MM33318_Arise;


typedef union _REG_MM3331C_Arise    //SCL_SD_Height_Ratio_Reg
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_Scl_Height_Ratio    :21;
        CBIOS_U32    Reserved_0    :8;
        CBIOS_U32    PS_Vdup_En    :1;
        CBIOS_U32    PS_Vtap_Sel    :1;
        CBIOS_U32    PS1_Deint_En    :1;
    };
}REG_MM3331C_Arise;


typedef union _REG_MM33320_Arise    //SCL_Src_Size_Reg
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_Scl_Src_Width    :13;
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    PS_Scl_Src_Height    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM33320_Arise;


typedef union _REG_MM333D8_Arise    //PWM_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    backlight_value    :16;
        CBIOS_U32    PWM_frequency_counter    :16;
    };
}REG_MM333D8_Arise;


typedef union _REG_MM333DC_Arise    //PWM_setting
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
}REG_MM333DC_Arise;


typedef union _REG_MM333E0_Arise    //PWM1_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    backlight_value    :16;
        CBIOS_U32    PWM_frequency_counter    :16;
    };
}REG_MM333E0_Arise;


typedef union _REG_MM333E4_Arise    //PWM1_setting
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
}REG_MM333E4_Arise;


typedef union _REG_MM333E8_Arise    //PWM2_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    backlight_value    :16;
        CBIOS_U32    PWM_frequency_counter    :16;
    };
}REG_MM333E8_Arise;


typedef union _REG_MM333EC_Arise    //PWM2_setting
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
}REG_MM333EC_Arise;


typedef union _REG_MM333F0_Arise    //PWM3_setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    backlight_value    :16;
        CBIOS_U32    PWM_frequency_counter    :16;
    };
}REG_MM333F0_Arise;


typedef union _REG_MM333F4_Arise    //PWM3_setting
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
}REG_MM333F4_Arise;


typedef union _REG_MM33400_Arise    //QS_compress/threshold_register(windows1)
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_Comp_Type    :6;
        CBIOS_U32    QS_Win1_Comp_Enable    :1;
        CBIOS_U32    Reserved_0    :17;
        CBIOS_U32    QS_Win1_Fifo_Depth_Control    :3;
        CBIOS_U32    QS_Win1_Share_Scaler_Fifo    :1;
        CBIOS_U32    Reserved_1    :4;
    };
}REG_MM33400_Arise;


typedef union _REG_MM33404_Arise    //Quad__windows_1_Frame_Buffer_Start_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_FB_Start_Address_0    :31;
        CBIOS_U32    QS_Win1_Work_Reg_En    :1;
    };
}REG_MM33404_Arise;


typedef union _REG_MM33408_Arise    //Quad_Stream_window_1_Frame_Buffer_Start_Address_1__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_Roffset    :31;
        CBIOS_U32    QS_Win1_Vsync_Off_Flip    :1;
    };
}REG_MM33408_Arise;


typedef union _REG_MM3340C_Arise    //Quad_Stream_1_Stride_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_Read_Length    :2;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    QS_Win1_Stride    :12;
        CBIOS_U32    QS_Win1_Start_Address_Byte_Offset    :4;
        CBIOS_U32    Reserved_1    :11;
        CBIOS_U32    QS_Win1_Abgr_En    :1;
    };
}REG_MM3340C_Arise;


typedef union _REG_MM33410_Arise    //Quad_stream_windows1_surface_address_offset
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    Suf_Badr_Off    :27;
    };
}REG_MM33410_Arise;


typedef union _REG_MM33414_Arise    //Quad_Stream__Window_1_Start_Coordinates_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Q1_Dat_H    :12;
        CBIOS_U32    Reserved_0    :4;
        CBIOS_U32    Q1_Dat_W    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM33414_Arise;


typedef union _REG_MM33418_Arise    //Quad_Stream__Window_1_Start_Coordinates_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_Y_Start    :12;
        CBIOS_U32    Reserved_0    :4;
        CBIOS_U32    QS_Win1_X_Start    :13;
        CBIOS_U32    RESERVED_1    :3;
    };
}REG_MM33418_Arise;


typedef union _REG_MM3341C_Arise    //Quad_Stream__windows1__Frame_Buffer_display_timing
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_Crt_H    :12;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    QS_Win1_Buf_SEL    :1;
        CBIOS_U32    QS_Win1_Dbl_Buf_Mode    :1;
        CBIOS_U32    QS_Win1_Triple_Buf_En    :1;
        CBIOS_U32    QS_Win1_Crt_W    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM3341C_Arise;


typedef union _REG_MM33420_Arise    //Quad_stream_windows1_upsacler_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_Hacc    :21;
        CBIOS_U32    Qs_Win1_Htap_Sel    :1;
        CBIOS_U32    Reserved    :10;
    };
}REG_MM33420_Arise;


typedef union _REG_MM33424_Arise    //Quad_stream_windows1_upsacler_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_Vacc    :21;
        CBIOS_U32    QS_Win1_Vtap_Sel    :1;
        CBIOS_U32    QS_Win1_Vdup_En    :1;
        CBIOS_U32    Reserved    :8;
        CBIOS_U32    QS_Win1_Deint_En    :1;
    };
}REG_MM33424_Arise;


typedef union _REG_MM33428_Arise    //QS1_win1/win2_threshold
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_Low_Thrld    :8;
        CBIOS_U32    QS_Win1_High_Thrld    :8;
        CBIOS_U32    QS_Win2_Low_Thrld    :8;
        CBIOS_U32    QS_Win2_High_Thrld    :8;
    };
}REG_MM33428_Arise;


typedef union _REG_MM3342C_Arise    //Quad_Stream_1_Color/Chroma_8bit_Key_Lower_Bound_Register_(windows1_)
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_8bit_B__Cb_Key_Lower_Bound    :8;
        CBIOS_U32    QS_Win1_8bit_G_Y_Key_Lower_Bound    :8;
        CBIOS_U32    QS_Win1_8bit_R_Cr_Key_Lower_Bound    :8;
        CBIOS_U32    Reserved_0    :7;
        CBIOS_U32    QS_Win1_oneshot    :1;
    };
    struct
    {
        CBIOS_U32    QS_Win1_10bit_B__Cb_Key_Lower_Bound    :10;
        CBIOS_U32    QS_Win1_10bit_G_Y_Key_Lower_Bound    :10;
        CBIOS_U32    QS_Win1_10bit_R_Cr_Key_Lower_Bound    :10;
        CBIOS_U32    Reserved    :1;
        CBIOS_U32    QS_Win1_Oneshot_Bit    :1;
    };
}REG_MM3342C_Arise;


typedef union _REG_MM33430_Arise    //qs_windows2__csc_control_register_(qs_windows1_register_MM3480)
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :11;
        CBIOS_U32    QS_Win2_YCbCr_Order    :1;
        CBIOS_U32    QS_Win2_Format    :3;
        CBIOS_U32    Reserved_2    :3;
        CBIOS_U32    QS_Win2_Alpha_Upscaler    :1;
        CBIOS_U32    QS_Win2_444_YCbCr_Order    :1;
        CBIOS_U32    QS_Win2_Enable    :1;
        CBIOS_U32    Reserved_3    :11;
    };
}REG_MM33430_Arise;


typedef union _REG_MM33434_Arise    //qs_windows2_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win2_F1    :14;
        CBIOS_U32    QS_Win2_F2    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33434_Arise;


typedef union _REG_MM33438_Arise    //qs_windows2_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win2_F3    :14;
        CBIOS_U32    QS_Win2_F4    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33438_Arise;


typedef union _REG_MM3343C_Arise    //qs_windows2_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win2_F5    :14;
        CBIOS_U32    QS_Win2_F6    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM3343C_Arise;


typedef union _REG_MM33440_Arise    //qs_windows2_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win2_F7    :14;
        CBIOS_U32    QS_Win2_F8    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33440_Arise;


typedef union _REG_MM33444_Arise    //qs_windows2_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win2_F9    :14;
        CBIOS_U32    QS_Win2_Bright    :9;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    QS_Win2_In_Format    :3;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    QS_Win2_Out_Format    :3;
        CBIOS_U32    QS_Win2_Program    :1;
    };
}REG_MM33444_Arise;


typedef union _REG_MM33448_Arise    //Quad_Stream_1_Color/Chroma_8bit_Key_Lower_Bound_Register_(windows2_)
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win2_8bit_B__Cb_Key_Lower_Bound    :8;
        CBIOS_U32    QS_Win2_8bit_G_Y_Key_Lower_Bound    :8;
        CBIOS_U32    QS_Win2_8bit_R_Cr_Key_Lower_Bound    :8;
        CBIOS_U32    Reserved_0    :7;
        CBIOS_U32    QS_Win2_Oneshot    :1;
    };
    struct
    {
        CBIOS_U32    QS_Win2_10bit_B__Cb_Key_Lower_Bound    :10;
        CBIOS_U32    QS_Win2_10bit_G_Y_Key_Lower_Bound    :10;
        CBIOS_U32    QS_Win2_10bit_R_Cr_Key_Lower_Bound    :10;
        CBIOS_U32    Reserved    :1;
        CBIOS_U32    QS_Win2_Oneshot_Bit    :1;
    };
}REG_MM33448_Arise;


typedef union _REG_MM3344C_Arise    //qs_windows2_chroma_keyh
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win2_KeyH    :24;
        CBIOS_U32    Reserved    :8;
    };
}REG_MM3344C_Arise;


typedef union _REG_MM33450_Arise    //QS_compress/threshold_register(windows2)
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win2_Comp_Type    :6;
        CBIOS_U32    QS_Win2_Comp_Enable    :1;
        CBIOS_U32    Reserved_0    :17;
        CBIOS_U32    QS_Win2_Fifo_Depth_Control    :3;
        CBIOS_U32    QS_Win2_Share_Scaler_Fifo    :1;
        CBIOS_U32    Reserved_1    :4;
    };
}REG_MM33450_Arise;


typedef union _REG_MM33454_Arise    //Quad__windows_1_Frame_Buffer_Start_Address_0_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win2_FB_Start_Address_0    :31;
        CBIOS_U32    QS_Win2_Work_Reg_En    :1;
    };
}REG_MM33454_Arise;


typedef union _REG_MM33458_Arise    //Quad_Stream_window2_Frame_Buffer_Start_Address_1__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_win2_ROFFSET    :31;
        CBIOS_U32    QS_win2_VSYNC_OFF_page_flip    :1;
    };
}REG_MM33458_Arise;


typedef union _REG_MM3345C_Arise    //Quad_Stream_1_Stride_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_win2_Read_Length    :2;
        CBIOS_U32    RESERVED_0    :2;
        CBIOS_U32    QS_win2_Stride    :12;
        CBIOS_U32    QS_win2__Start_Address_Byte_Offset    :4;
        CBIOS_U32    RESERVED_1    :11;
        CBIOS_U32    QS_win2_ABGR_EN    :1;
    };
}REG_MM3345C_Arise;


typedef union _REG_MM33460_Arise    //Quad_stream_window2_surface_address_offset
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    RESERVED    :5;
        CBIOS_U32    SUF_BADR_OFF    :27;
    };
}REG_MM33460_Arise;


typedef union _REG_MM33464_Arise    //Quad_Stream__Window2_Start_Coordinates_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Q1_DAT_H    :12;
        CBIOS_U32    Reserved_0    :4;
        CBIOS_U32    Q1_DAT_W    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM33464_Arise;


typedef union _REG_MM33468_Arise    //Quad_Stream__Window2_Start_Coordinates_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS1_Y_Start    :12;
        CBIOS_U32    Reserved_0    :4;
        CBIOS_U32    QS1_X_Start    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM33468_Arise;


typedef union _REG_MM3346C_Arise    //Third_Stream_Window1_and_Window2_threshold__Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win1_Lthrhld    :8;
        CBIOS_U32    TS_Win1_Hthrhld    :8;
        CBIOS_U32    TS_Win2_Lthrhld    :8;
        CBIOS_U32    TS_Win2_Hthrhld    :8;
    };
}REG_MM3346C_Arise;


typedef union _REG_MM33470_Arise    //Quad_stream_windows2_upsacler_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win2_Hacc    :21;
        CBIOS_U32    Qs_Win2_Htap_Sel    :1;
        CBIOS_U32    Reserved    :10;
    };
}REG_MM33470_Arise;


typedef union _REG_MM33474_Arise    //Quad_stream_windows2_upsacler_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win2_Vacc    :21;
        CBIOS_U32    QS_Win2_Vtap_Sel    :1;
        CBIOS_U32    QS_Win2_Vdup_En    :1;
        CBIOS_U32    Reserved    :8;
        CBIOS_U32    QS_Win2_Deint_En    :1;
    };
}REG_MM33474_Arise;


typedef union _REG_MM33478_Arise    //Quad_Stream__windows2__Frame_Buffer_display_timing
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Q1_CRT_H    :12;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    Q1_BUF_SEL    :1;
        CBIOS_U32    Q1_DBLBUF_MODE    :1;
        CBIOS_U32    Q1_TRIPLEBUF_EN    :1;
        CBIOS_U32    Q1_CRT_W    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM33478_Arise;


typedef union _REG_MM33480_Arise    //qs_windows1__csc_control_register_(qs_windows1_register_MM34A0)
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :11;
        CBIOS_U32    QS_Win1_YCbCr_Order    :1;
        CBIOS_U32    QS_Win2_Format    :3;
        CBIOS_U32    Reserved_2    :3;
        CBIOS_U32    QS_Win1_Alpha_Upscaler    :1;
        CBIOS_U32    QS_Win1_444_YCbCr_Order    :1;
        CBIOS_U32    QS_Win2_Enable    :1;
        CBIOS_U32    Reserved_3    :11;
    };
}REG_MM33480_Arise;


typedef union _REG_MM33484_Arise    //qs_windows1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_F1    :14;
        CBIOS_U32    QS_Win1_F2    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33484_Arise;


typedef union _REG_MM33488_Arise    //qs_windows1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_F3    :14;
        CBIOS_U32    QS_Win1_F4    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33488_Arise;


typedef union _REG_MM3348C_Arise    //qs_windows1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_F5    :14;
        CBIOS_U32    QS_Win1_F6    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM3348C_Arise;


typedef union _REG_MM33490_Arise    //qs_windows1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_F7    :14;
        CBIOS_U32    QS_Win1_F8    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33490_Arise;


typedef union _REG_MM33494_Arise    //qs_windows1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_F9    :14;
        CBIOS_U32    QS_Win1_Bright    :9;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    QS_Win1_In_Format    :3;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    QS_Win1_Out_Format    :3;
        CBIOS_U32    QS_Win1_Program    :1;
    };
}REG_MM33494_Arise;


typedef union _REG_MM33498_Arise    //qs_windows1_chroma_keyh
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_Win1_KeyH    :24;
        CBIOS_U32    Reserved    :8;
    };
}REG_MM33498_Arise;


typedef union _REG_MM334A0_Arise    //ts_windows2__csc_control_register_(ts_windows2_register_MM34A0)
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :11;
        CBIOS_U32    TS_Win2_YCbCr_Order    :1;
        CBIOS_U32    TS_Win2_Format    :3;
        CBIOS_U32    Reserved_2    :3;
        CBIOS_U32    TS_Win1_Alpha_Upscaler    :1;
        CBIOS_U32    TS_Win2_444_YCbCr_Order    :1;
        CBIOS_U32    TS_Win2_Enable    :1;
        CBIOS_U32    Reserved_3    :11;
    };
}REG_MM334A0_Arise;


typedef union _REG_MM334A4_Arise    //ts_windows2_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_F1    :14;
        CBIOS_U32    TS_Win2_F2    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM334A4_Arise;


typedef union _REG_MM334A8_Arise    //ts_windows2_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_F3    :14;
        CBIOS_U32    TS_Win2_F4    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM334A8_Arise;


typedef union _REG_MM334AC_Arise    //ts_windows2_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_F5    :14;
        CBIOS_U32    TS_Win2_F6    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM334AC_Arise;


typedef union _REG_MM334B0_Arise    //ts_windows2_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_F7    :14;
        CBIOS_U32    TS_Win2_F8    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM334B0_Arise;


typedef union _REG_MM334B4_Arise    //ts_windows2_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_F9    :14;
        CBIOS_U32    TS_Win2_Bright    :9;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    TS_Win2_In_Format    :3;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    TS_Win2_Out_Format    :3;
        CBIOS_U32    TS_Win2_Program    :1;
    };
}REG_MM334B4_Arise;


typedef union _REG_MM334B8_Arise    //Thrid_Stream_1_Color/Chroma_8bit_Key_Lower_Bound_Register_(windows2_)
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_Win2_8bit_B__Cb_Key_Lower_Bound    :8;
        CBIOS_U32    TS_Win2_8bit_G_Y_Key_Lower_Bound    :8;
        CBIOS_U32    TS_Win2_8bit_R_Cr_Key_Lower_Bound    :8;
        CBIOS_U32    Reserved_0    :7;
        CBIOS_U32    TS_Win2_Oneshot    :1;
    };
    struct
    {
        CBIOS_U32    TS_Win2_10bit_B__Cb_Key_Lower_Bound    :10;
        CBIOS_U32    TS_Win2_10bit_G_Y_Key_Lower_Bound    :10;
        CBIOS_U32    TS_Win2_10bit_R_Cr_Key_Lower_Bound    :10;
        CBIOS_U32    Reserved    :1;
        CBIOS_U32    TS_Win2_Oneshot_Bit    :1;
    };
}REG_MM334B8_Arise;


typedef union _REG_MM334BC_Arise    //eDP1_PSR_control
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VSC_SW_CRC_47_32    :16;
        CBIOS_U32    PSR_EN    :1;
        CBIOS_U32    CRC_ENREG    :1;
        CBIOS_U32    PSR_BURST_UPDATE    :1;
        CBIOS_U32    PSR_SLEEP    :1;
        CBIOS_U32    PSR_ENTER_MODE    :1;
        CBIOS_U32    PSR_UPDATE_MODE    :1;
        CBIOS_U32    PSR_CRC_MODE    :1;
        CBIOS_U32    SEL_SW_CRC    :1;
        CBIOS_U32    PSR_MAINLINK_ON    :1;
        CBIOS_U32    Reserved    :7;
    };
}REG_MM334BC_Arise;


typedef union _REG_MM334C0_Arise    //eDP1_PSR_control
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VSC_SW_CRC_31_0    :32;
    };
}REG_MM334C0_Arise;


typedef union _REG_MM334C4_Arise    //eDP1_PSR_control
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VSC_HEAD    :8;
        CBIOS_U32    VSC_DB0    :8;
        CBIOS_U32    VSC_COUNT    :12;
        CBIOS_U32    PSR_ST    :3;
        CBIOS_U32    MAINLINK_TURNOFF    :1;
    };
}REG_MM334C4_Arise;


typedef union _REG_MM334C8_Arise    //DP1_control_register
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
}REG_MM334C8_Arise;


typedef union _REG_MM334CC_Arise    //DP1_Control_Register
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
}REG_MM334CC_Arise;


typedef union _REG_MM334D0_Arise    //DP1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CUSTOM_80BIT_PATTERN_31_0    :32;
    };
}REG_MM334D0_Arise;


typedef union _REG_MM334D4_Arise    //DP1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CUSTOM_80BIT_PATTERN_63_32    :32;
    };
}REG_MM334D4_Arise;


typedef union _REG_MM334D8_Arise    //DP1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    CUSTOM_80BIT_PATTERN_79_64    :16;
        CBIOS_U32    SW_LANE0_post_pp    :2;
        CBIOS_U32    SW_LANE1_post_pp    :2;
        CBIOS_U32    SW_LANE2_post_pp    :2;
        CBIOS_U32    SW_LANE3_post_pp    :2;
        CBIOS_U32    Lane0_POST_CURSOR_level_status    :2;
        CBIOS_U32    Lane1_POST_CURSOR_level_status    :2;
        CBIOS_U32    Lane2_POST_CURSOR_level_status    :2;
        CBIOS_U32    Lane3_POST_CURSOR_level_status    :2;
    };
}REG_MM334D8_Arise;


typedef union _REG_MM334DC_Arise    //DP1_Control_Register
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
}REG_MM334DC_Arise;


typedef union _REG_MM334E0_Arise    //DP1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    EPHY1_FBOOST    :3;
        CBIOS_U32    EPHY1_HDCKBY4    :1;
        CBIOS_U32    EPHY1_FBOOST_1    :3;
        CBIOS_U32    EPHY1_FBOOST_2    :3;
        CBIOS_U32    reseverd_0    :10;
        CBIOS_U32    EPHY1_SR_SPD    :2;
        CBIOS_U32    EPHY1_SR_DLY    :2;
        CBIOS_U32    EPHY1_SR_NDLY    :2;
        CBIOS_U32    Reserved_30to26    :5;
        CBIOS_U32    DP1_HPD_INV    :1;
    };
}REG_MM334E0_Arise;


typedef union _REG_MM334E4_Arise    //DP1_Control_Register
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
}REG_MM334E4_Arise;


typedef union _REG_MM33500_Arise    //mdi_strobe_read_back_data_register_0
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    MDI_DSP_RDAT_31_0    :32;
    };
}REG_MM33500_Arise;


typedef union _REG_MM33504_Arise    //mdi_strobe_read_back_data_register_1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    MDI_DSP_RDAT_63_32    :32;
    };
}REG_MM33504_Arise;


typedef union _REG_MM33508_Arise    //mdi_strobe_read_back_data_register_2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    MDI_DSP_RDAT_95_64    :32;
    };
}REG_MM33508_Arise;


typedef union _REG_MM3350C_Arise    //mdi_strobe_read_back_data_register_3
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    MDI_DSP_RDAT_127_96    :32;
    };
}REG_MM3350C_Arise;


typedef union _REG_MM33510_Arise    //mdi_strobe_read_back_data_register_4
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    MDI_DSP_RDAT_159_128    :32;
    };
}REG_MM33510_Arise;


typedef union _REG_MM33514_Arise    //mdi_strobe_read_back_data_register_5
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    MDI_DSP_RDAT_191_160    :32;
    };
}REG_MM33514_Arise;


typedef union _REG_MM33518_Arise    //mdi_strobe_read_back_data_register_6
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    MDI_DSP_RDAT_223_192    :32;
    };
}REG_MM33518_Arise;


typedef union _REG_MM3351C_Arise    //mdi_strobe_read_back_data_register_7
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    MDI_DSP_RDAT_255_224    :32;
    };
}REG_MM3351C_Arise;


typedef union _REG_MM33520_Arise    //mdi_strobe_control_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    strobe_enable    :1;
        CBIOS_U32    strobe_reset    :1;
        CBIOS_U32    data_type    :1;
        CBIOS_U32    engine_ID    :5;
        CBIOS_U32    group_number    :2;
        CBIOS_U32    fifo_write_address    :9;
        CBIOS_U32    strobe_status    :1;
        CBIOS_U32    reserved    :12;
    };
}REG_MM33520_Arise;


typedef union _REG_MM33524_Arise    //security_flag
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    security_r    :20;
        CBIOS_U32    security_w    :8;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33524_Arise;


typedef union _REG_MM33528_Arise    //MDI_read_latency_debug_register_9
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_CTR_MAX    :13;
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    SS2_CTR_MAX    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM33528_Arise;


typedef union _REG_MM3352C_Arise    //MDI_read_latency_debug_register_10
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS2_CTR_MAX    :13;
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    QS2_CTR_MAX    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM3352C_Arise;


typedef union _REG_MM33530_Arise    //MDI_read_latency_debug_register_11
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS3_CTR_MAX    :13;
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    SS3_CTR_MAX    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM33530_Arise;


typedef union _REG_MM33534_Arise    //MDI_read_latency_debug_register_12
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS3_CTR_MAX    :13;
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    QS3_CTR_MAX    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM33534_Arise;


typedef union _REG_MM33538_Arise    //MDI_read_latency_debug_register_13
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS4_CTR_MAX    :13;
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    SS4_CTR_MAX    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM33538_Arise;


typedef union _REG_MM3353C_Arise    //MDI_read_latency_debug_register_14
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS4_CTR_MAX    :13;
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    QS4_CTR_MAX    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM3353C_Arise;


typedef union _REG_MM33540_Arise    //MDI_read_latency_debug_register_15
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    BURST_CTR_MAX    :13;
        CBIOS_U32    Reserved    :19;
    };
}REG_MM33540_Arise;


typedef union _REG_MM33600_Arise    //ts_windows1__csc_control_register_(ts_windows2_register_MM34A0)
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :11;
        CBIOS_U32    TS_YCbCr_Order    :1;
        CBIOS_U32    TS_Win1_Format    :3;
        CBIOS_U32    Reserved_2    :3;
        CBIOS_U32    TS_Win1_Alpha_Upscaler    :1;
        CBIOS_U32    TS_444_YCbCr_Order    :1;
        CBIOS_U32    Reserved_3    :12;
    };
}REG_MM33600_Arise;


typedef union _REG_MM33604_Arise    //ts_windows1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_F1    :14;
        CBIOS_U32    TS_F2    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33604_Arise;


typedef union _REG_MM33608_Arise    //ts_windows1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_F3    :14;
        CBIOS_U32    TS_F4    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33608_Arise;


typedef union _REG_MM3360C_Arise    //ts_windows1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_F5    :14;
        CBIOS_U32    TS_F6    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM3360C_Arise;


typedef union _REG_MM33610_Arise    //ts_windows1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_F7    :14;
        CBIOS_U32    TS_F8    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33610_Arise;


typedef union _REG_MM33614_Arise    //ts_windows1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_F9    :14;
        CBIOS_U32    TS_Bright    :9;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    TS_In_Format    :3;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    TS_Out_Format    :3;
        CBIOS_U32    TS_Program    :1;
    };
}REG_MM33614_Arise;

typedef union _REG_MM3361C_Arise    //ss1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_F1    :14;
        CBIOS_U32    SS_F2    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM3361C_Arise;


typedef union _REG_MM33620_Arise    //ss1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_F3    :14;
        CBIOS_U32    SS_F4    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33620_Arise;


typedef union _REG_MM33624_Arise    //ss1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_F5    :14;
        CBIOS_U32    SS_F6    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33624_Arise;


typedef union _REG_MM33628_Arise    //ss1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_F7    :14;
        CBIOS_U32    SS_F8    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33628_Arise;


typedef union _REG_MM3362C_Arise    //ss1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_F9    :14;
        CBIOS_U32    SS_Bright    :9;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    SS_Data_In_Format    :3;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    SS_Data_Out_Format    :3;
        CBIOS_U32    SS_Program    :1;
    };
}REG_MM3362C_Arise;


typedef union _REG_MM33634_Arise    //ps1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_F1    :14;
        CBIOS_U32    PS_F2    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33634_Arise;


typedef union _REG_MM33638_Arise    //ps1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_F3    :14;
        CBIOS_U32    PS_F4    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33638_Arise;


typedef union _REG_MM3363C_Arise    //ps1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_F5    :14;
        CBIOS_U32    PS_F6    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM3363C_Arise;


typedef union _REG_MM33640_Arise    //ps1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_F7    :14;
        CBIOS_U32    PS_F8    :14;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM33640_Arise;


typedef union _REG_MM33644_Arise    //ps1_csc_coefficient
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS_F9    :14;
        CBIOS_U32    PS_Bright    :9;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    PS_Data_In_Fmt    :3;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    PS_Data_Out_Fmt    :3;
        CBIOS_U32    PS_Program    :1;
    };
}REG_MM33644_Arise;


typedef union _REG_MM33648_Arise    //tv_csc_control
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :4;
        CBIOS_U32    TV_Csc_Coef_Sel    :1;
        CBIOS_U32    TV_Csc_Coef_Tune    :1;
        CBIOS_U32    Reserved_1    :2;
        CBIOS_U32    TV_Contrast_Tune_Value    :8;
        CBIOS_U32    TV_U_Satu_Tune_Value    :8;
        CBIOS_U32    TV_V_Satu_Tune_Value    :8;
    };
}REG_MM33648_Arise;


typedef union _REG_MM33658_Arise    //tv_csc_factor_register_4
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TV_Color_Space_Conv_Factor_0_F6    :11;
        CBIOS_U32    Reserved_0    :5;
        CBIOS_U32    TV_Color_Space_Conv_Factor_1_F7    :11;
        CBIOS_U32    Reserved_1    :5;
    };
}REG_MM33658_Arise;


typedef union _REG_MM33660_Arise    //iga1_interlace_mode_right_base
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved    :2;
        CBIOS_U32    IGA1_Interlace_Right_Base    :30;
    };
}REG_MM33660_Arise;


typedef union _REG_MM33664_Arise    //iga2_interlace_mode_right_base
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    reserved    :2;
        CBIOS_U32    IGA2_Interlace_Right_Base    :30;
    };
}REG_MM33664_Arise;


typedef union _REG_MM33668_Arise    //PS_1_dynamic_sync_control
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Dynamic_Sync_Enable    :1;
        CBIOS_U32    Select_PS_Load_Or_SS_Load    :1;
        CBIOS_U32    Max_Line_Number    :13;
        CBIOS_U32    Edp_Frc_Enable    :1;
        CBIOS_U32    Reserved    :16;
    };
}REG_MM33668_Arise;


typedef union _REG_MM33670_Arise    //Color_value_outside_of_3rd_overlay_window
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Backgnd_Color_Value    :30;
        CBIOS_U32    Backgnd_Color_Ycbcr    :1;
        CBIOS_U32    reserved    :1;
    };
}REG_MM33670_Arise;


typedef union _REG_MM33680_Arise    //HDMI1_HDCP22_control_register
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
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    TEST_REPEATER    :1;
        CBIOS_U32    TEST_MODE    :1;
        CBIOS_U32    CONT_STREAM_EN    :1;
        CBIOS_U32    HDCP22_CP_EN    :1;
    };
}REG_MM33680_Arise;


typedef union _REG_MM33684_Arise    //DP1_HDCP22_control_register
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
}REG_MM33684_Arise;


typedef union _REG_MM33688_Arise    //DP1/HDMI1_HDCP22_control_register
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
}REG_MM33688_Arise;


typedef union _REG_MM3368C_Arise    //DP1/HDMI1_HDCP22_interrupt_register
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
}REG_MM3368C_Arise;


typedef union _REG_MM33690_Arise    //DP1/HDMI1_HDCP22_interrupt_mask_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDCP22_INT_MASK    :32;
    };
}REG_MM33690_Arise;


typedef union _REG_MM33694_Arise    //hdtv1_line_buffer__ps_csc__setting
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
}REG_MM33694_Arise;


typedef union _REG_MM33698_Arise    //hdtv1_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB1_COEF_F1    :14;
        CBIOS_U32    LB1_COEF_F2    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM33698_Arise;


typedef union _REG_MM3369C_Arise    //hdtv1_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB1_COEF_F3    :14;
        CBIOS_U32    LB1_COEF_F4    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM3369C_Arise;


typedef union _REG_MM336A0_Arise    //hdtv1_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB1_COEF_F5    :14;
        CBIOS_U32    LB1_COEF_F6    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM336A0_Arise;


typedef union _REG_MM336A4_Arise    //hdtv1_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB1_COEF_F7    :14;
        CBIOS_U32    LB1_COEF_F8    :14;
        CBIOS_U32    Reserved_31to28    :4;
    };
}REG_MM336A4_Arise;


typedef union _REG_MM336A8_Arise    //hdtv1_line_buffer__ps_csc__setting
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    LB1_COEF_F9    :14;
        CBIOS_U32    LB1_BRIGHT    :9;
        CBIOS_U32    Reserved_31to23    :9;
    };
}REG_MM336A8_Arise;


typedef union _REG_MM336B0_Arise    // 
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
}REG_MM336B0_Arise;


typedef union _REG_MM336B4_Arise    // 
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
}REG_MM336B4_Arise;


typedef union _REG_MM336B8_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDMI1_SCDC_RR_START   :1;
        CBIOS_U32    HDMI1_SCDC_WAIT_TIMEOUT    :23;
        CBIOS_U32    HDMI1_SCDC_RR_INT_STATUS    :1;
        CBIOS_U32    Reserved    :3;
        CBIOS_U32    HDMI1_SCDC_START_STOP_ENABLE    :1;
        CBIOS_U32    HDMI1_SCDC_HW_DRV_STOP_ENABLE    :1;
        CBIOS_U32    HDMI1_SCDC_HW_DRV_START_ENABLE    :1;
        CBIOS_U32    HDMI1_SCDC_RR_ENABLE    :1;
    };
}REG_MM336B8_Arise;


typedef union _REG_MM336BC_Arise    //HDMI1_SCDC_control_2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDMI1_SCDC_WDATA    :8;
        CBIOS_U32    HDMI1_SCDC_WDATA_SEL    :1;
        CBIOS_U32    IIC1_STATIMEOUT    :23;
    };
}REG_MM336BC_Arise;


typedef union _REG_MM33700_Arise    //PS1_base_offset
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    Base_Offset    :27;
        CBIOS_U32    Reserved_1    :2;
    };
}REG_MM33700_Arise;


typedef union _REG_MM33704_Arise    //PS2_base_offset
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    Reserved_1    :25;
        CBIOS_U32    Reserved_2    :4;
    };
}REG_MM33704_Arise;


typedef union _REG_MM33708_Arise    //SS1_base_offset/NV12Y_base_offset
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :5;
        CBIOS_U32    SS_Base_Offset    :25;
        CBIOS_U32    Reserved_1    :2;
    };
}REG_MM33708_Arise;


typedef union _REG_MM3370C_Arise    //SS1_NV12C_base_offset
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :5;
        CBIOS_U32    offset_value    :25;
        CBIOS_U32    Reserved_1    :2;
    };
}REG_MM3370C_Arise;


typedef union _REG_MM33710_Arise    //PS1_right_frame_base_offset
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    offset_value    :27;
        CBIOS_U32    Reserved_1    :2;
    };
}REG_MM33710_Arise;


typedef union _REG_MM33714_Arise    //PS2_right_frame_base_offset
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :3;
        CBIOS_U32    Reserved_1    :25;
        CBIOS_U32    Reserved_2    :4;
    };
}REG_MM33714_Arise;


typedef union _REG_MM33718_Arise    //Cursor_1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Cursor_1_Enable    :1;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    Cursor_1_Type    :2;
        CBIOS_U32    Cursor_1_X_Rotation    :1;
        CBIOS_U32    Cursor_1_Y_Rotation    :1;
        CBIOS_U32    Cursor_1_Size    :2;
        CBIOS_U32    Cursor_1_Display_Start_X    :7;
        CBIOS_U32    Reserved_2    :1;
        CBIOS_U32    Cursor_1_Display_Start_Y    :7;
        CBIOS_U32    Cursor_1_reverse_alpha_enable    :1;
        CBIOS_U32    Cursor_1_inverse_alpha    :1;
        CBIOS_U32    Reserved_3    :3;
        CBIOS_U32    Reserved_4    :3;
        CBIOS_U32    Cursor_1_mmio_reg_en    :1;
    };
}REG_MM33718_Arise;


typedef union _REG_MM3371C_Arise    //Cursor_1_Control_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Cursor_1_X_Origin    :14;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    Cursor_1_Y_Origin    :13;
        CBIOS_U32    Reserved_1    :3;
    };
}REG_MM3371C_Arise;


typedef union _REG_MM33720_Arise    //Cursor_1_Base_Address_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Cursor_1_Base_Address    :31;
        CBIOS_U32    Cursor_1_Enable_Work_Registers    :1;
    };
}REG_MM33720_Arise;


typedef union _REG_MM33724_Arise    //Cursor_1_Right_Frame_Base_Address_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Cursor_1_Right_Frame_Base_Address    :31;
        CBIOS_U32    Cursor_1_Vsync_Off_Page_Flip    :1;
    };
}REG_MM33724_Arise;


typedef union _REG_MM33728_Arise    //Cursor_1_end
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Cursor_1_X_end    :7;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    Cursor_1_Y_end    :7;
        CBIOS_U32    Reserved_1    :17;
    };
}REG_MM33728_Arise;


typedef union _REG_MM3372C_Arise    //SS1_NV12T_block_total_Register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    NV12Y_request_block_total    :8;
        CBIOS_U32    NV12C_request_block_total    :8;
        CBIOS_U32    Reserved    :16;
    };
}REG_MM3372C_Arise;


typedef union _REG_MM33730_Arise    //VIP0_REG0
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_vcount    :14;
        CBIOS_U32    VIP_hcount    :14;
        CBIOS_U32    reserved    :4;
    };
}REG_MM33730_Arise;


typedef union _REG_MM33734_Arise    //VIP1_REG0
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_vcount    :14;
        CBIOS_U32    VIP_hcount    :14;
        CBIOS_U32    reserved    :4;
    };
}REG_MM33734_Arise;


typedef union _REG_MM33738_Arise    //VIP2_REG0
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_vcount    :14;
        CBIOS_U32    VIP_hcount    :14;
        CBIOS_U32    reserved    :4;
    };
}REG_MM33738_Arise;


typedef union _REG_MM3373C_Arise    //VIP3_REG0
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_vcount    :14;
        CBIOS_U32    VIP_hcount    :14;
        CBIOS_U32    reserved    :4;
    };
}REG_MM3373C_Arise;


typedef union _REG_MM33740_Arise    //VIP0_REG1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_reset    :1;
        CBIOS_U32    VIP_enable    :1;
        CBIOS_U32    video_mode    :7;
        CBIOS_U32    VIP_hsync_polarity_invert    :1;
        CBIOS_U32    VIP_vsync_polarity_invert    :1;
        CBIOS_U32    SCE    :1;
        CBIOS_U32    header_select    :1;
        CBIOS_U32    SMB    :2;
        CBIOS_U32    VBI_enable    :1;
        CBIOS_U32    RAW_VBI_enable    :1;
        CBIOS_U32    VIP_video_data_delay    :2;
        CBIOS_U32    VBI_header_type    :1;
        CBIOS_U32    VBI_double_mode    :1;
        CBIOS_U32    header_type    :1;
        CBIOS_U32    _10bits_mode    :1;
        CBIOS_U32    one_more_SVBI    :2;
        CBIOS_U32    DROP_frame_mode    :2;
        CBIOS_U32    Drop_VBI    :1;
        CBIOS_U32    drop_frame    :1;
        CBIOS_U32    drop_odd_even    :1;
        CBIOS_U32    VIP_load_work_register    :1;
        CBIOS_U32    Load_at_VIP_vsync_enable    :1;
    };
}REG_MM33740_Arise;


typedef union _REG_MM33744_Arise    //VIP0_REG2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    REG_auto_gen    :1;
        CBIOS_U32    control_word_0_select    :2;
        CBIOS_U32    control_word_1_select    :2;
        CBIOS_U32    control_word_2_select    :2;
        CBIOS_U32    control_word_3_select    :2;
        CBIOS_U32    VIP_hde_enable    :1;
        CBIOS_U32    VIP_sync_enable    :1;
        CBIOS_U32    enable_data_0    :1;
        CBIOS_U32    data_type    :1;
        CBIOS_U32    data_word_0_select    :2;
        CBIOS_U32    data_word_1_select    :2;
        CBIOS_U32    data_word_2_select    :2;
        CBIOS_U32    data_word_3_select    :2;
        CBIOS_U32    video_data_out_format    :1;
        CBIOS_U32    SVBI_data_out_format    :1;
        CBIOS_U32    VBI_control_word0_sel    :2;
        CBIOS_U32    VBI_control_word1_sel    :2;
        CBIOS_U32    VBI_control_word2_sel    :2;
        CBIOS_U32    VBI_control_word3_sel    :2;
        CBIOS_U32    Undefined_Bit_31_to_31    :1;
    };
}REG_MM33744_Arise;


typedef union _REG_MM33748_Arise    //VIP0_REG3
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_ver_start    :14;
        CBIOS_U32    reserved    :2;
        CBIOS_U32    VIP_ver_window_length    :14;
        CBIOS_U32    enable_error_detect    :1;
        CBIOS_U32    crop_window_enable    :1;
    };
}REG_MM33748_Arise;


typedef union _REG_MM3374C_Arise    //VIP0_REG4
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_hor_start    :14;
        CBIOS_U32    reserved_0    :2;
        CBIOS_U32    VIP_hor_length    :14;
        CBIOS_U32    reserved_1    :2;
    };
}REG_MM3374C_Arise;


typedef union _REG_MM33750_Arise    //VIP0_REG5
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    fifo_threshold    :7;
        CBIOS_U32    reserved_0    :1;
        CBIOS_U32    fifo_high_threshold    :7;
        CBIOS_U32    reserved_1    :1;
        CBIOS_U32    VIP_input_clock_inv    :1;
        CBIOS_U32    VIP_input_clock_delay_select    :4;
        CBIOS_U32    VIP_input_clock_xor_enable    :1;
        CBIOS_U32    VIP_input_data_select    :8;
        CBIOS_U32    reserved_2    :1;
        CBIOS_U32    VIP_DVO_enable    :1;
    };
}REG_MM33750_Arise;


typedef union _REG_MM33754_Arise    //VIP0_REG6
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FB_mode    :2;
        CBIOS_U32    FB_base0    :30;
    };
}REG_MM33754_Arise;


typedef union _REG_MM33758_Arise    //VIP0_REG7
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_overflow    :1;
        CBIOS_U32    XYZ_error_flg    :1;
        CBIOS_U32    FB_base1    :30;
    };
}REG_MM33758_Arise;


typedef union _REG_MM3375C_Arise    //VIP0_REG8
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_clk_detect    :1;
        CBIOS_U32    Line_cnt_wr_en    :1;
        CBIOS_U32    FB_base2    :30;
    };
}REG_MM3375C_Arise;


typedef union _REG_MM33760_Arise    //VIP0_REG9
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    interleave_enable    :1;
        CBIOS_U32    frame_by_frame_odd_on_top    :1;
        CBIOS_U32    flush_enable    :1;
        CBIOS_U32    frame_end_reset    :1;
        CBIOS_U32    fifo_overfloe_clr    :1;
        CBIOS_U32    XYZ_error_clr    :1;
        CBIOS_U32    FB_stride    :10;
        CBIOS_U32    reserved    :5;
        CBIOS_U32    FB_VBI_size    :11;
    };
}REG_MM33760_Arise;


typedef union _REG_MM33764_Arise    //VIP0_REG10
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    cap_full_frame    :1;
        CBIOS_U32    VIP_data_type    :1;
        CBIOS_U32    FB_max_size    :30;
    };
}REG_MM33764_Arise;


typedef union _REG_MM33768_Arise    //VIP0_REG11
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_VBI_count0    :16;
        CBIOS_U32    VIP_VBI_count1    :16;
    };
}REG_MM33768_Arise;


typedef union _REG_MM3376C_Arise    //VIP0_REG12
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_which_BAR    :2;
        CBIOS_U32    VIP_interlace_detect    :1;
        CBIOS_U32    VREF    :1;
        CBIOS_U32    HREF    :1;
        CBIOS_U32    FIELD_ID    :1;
        CBIOS_U32    frame_odd_even    :1;
        CBIOS_U32    reserved_0    :1;
        CBIOS_U32    EOF_INT_enable    :1;
        CBIOS_U32    HPD_INT_enable    :1;
        CBIOS_U32    EOF_INT    :1;
        CBIOS_U32    HPD_INT    :1;
        CBIOS_U32    VIP_HPD_INT_status    :2;
        CBIOS_U32    reserved_1    :18;
    };
}REG_MM3376C_Arise;


typedef union _REG_MM33770_Arise    //VIP1_REG1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_reset    :1;
        CBIOS_U32    VIP_enable    :1;
        CBIOS_U32    video_mode    :7;
        CBIOS_U32    VIP_hsync_polarity_invert    :1;
        CBIOS_U32    VIP_vsync_polarity_invert    :1;
        CBIOS_U32    SCE    :1;
        CBIOS_U32    header_select    :1;
        CBIOS_U32    SMB    :2;
        CBIOS_U32    VBI_enable    :1;
        CBIOS_U32    RAW_VBI_enable    :1;
        CBIOS_U32    VIP_video_data_delay    :2;
        CBIOS_U32    VBI_header_type    :1;
        CBIOS_U32    VBI_double_mode    :1;
        CBIOS_U32    header_type    :1;
        CBIOS_U32    _10bits_mode    :1;
        CBIOS_U32    one_more_SVBI    :2;
        CBIOS_U32    DROP_frame_mode    :2;
        CBIOS_U32    Drop_VBI    :1;
        CBIOS_U32    drop_frame    :1;
        CBIOS_U32    drop_odd_even    :1;
        CBIOS_U32    VIP_load_work_register    :1;
        CBIOS_U32    Load_at_VIP_vsync_enable    :1;
    };
}REG_MM33770_Arise;


typedef union _REG_MM33774_Arise    //VIP1_REG2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    REG_auto_gen    :1;
        CBIOS_U32    control_word_0_select    :2;
        CBIOS_U32    control_word_1_select    :2;
        CBIOS_U32    control_word_2_select    :2;
        CBIOS_U32    control_word_3_select    :2;
        CBIOS_U32    VIP_hde_enable    :1;
        CBIOS_U32    VIP_sync_enable    :1;
        CBIOS_U32    enable_data_0    :1;
        CBIOS_U32    data_type    :1;
        CBIOS_U32    data_word_0_select    :2;
        CBIOS_U32    data_word_1_select    :2;
        CBIOS_U32    data_word_2_select    :2;
        CBIOS_U32    data_word_3_select    :2;
        CBIOS_U32    video_data_out_format    :1;
        CBIOS_U32    SVBI_data_out_format    :1;
        CBIOS_U32    VBI_control_word0_sel    :2;
        CBIOS_U32    VBI_control_word1_sel    :2;
        CBIOS_U32    VBI_control_word2_sel    :2;
        CBIOS_U32    VBI_control_word3_sel    :2;
        CBIOS_U32    Undefined_Bit_31_to_31    :1;
    };
}REG_MM33774_Arise;


typedef union _REG_MM33778_Arise    //VIP1_REG3
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_ver_start    :14;
        CBIOS_U32    reserved    :2;
        CBIOS_U32    VIP_ver_window_length    :14;
        CBIOS_U32    enable_error_detect    :1;
        CBIOS_U32    crop_window_enable    :1;
    };
}REG_MM33778_Arise;


typedef union _REG_MM3377C_Arise    //VIP1_REG4
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_hor_start    :14;
        CBIOS_U32    reserved_0    :2;
        CBIOS_U32    VIP_hor_length    :14;
        CBIOS_U32    reserved_1    :2;
    };
}REG_MM3377C_Arise;


typedef union _REG_MM33780_Arise    //VIP1_REG5
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    fifo_threshold    :7;
        CBIOS_U32    reserved_0    :1;
        CBIOS_U32    fifo_high_threshold    :7;
        CBIOS_U32    reserved_1    :1;
        CBIOS_U32    VIP_input_clock_inv    :1;
        CBIOS_U32    VIP_input_clock_delay_select    :4;
        CBIOS_U32    VIP_input_clock_xor_enable    :1;
        CBIOS_U32    VIP_input_data_select    :8;
        CBIOS_U32    reserved_2    :1;
        CBIOS_U32    VIP_DVO_enable    :1;
    };
}REG_MM33780_Arise;


typedef union _REG_MM33784_Arise    //VIP1_REG6
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FB_mode    :2;
        CBIOS_U32    FB_base0    :30;
    };
}REG_MM33784_Arise;


typedef union _REG_MM33788_Arise    //VIP1_REG7
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_overflow    :1;
        CBIOS_U32    XYZ_error_flg    :1;
        CBIOS_U32    FB_base1    :30;
    };
}REG_MM33788_Arise;


typedef union _REG_MM3378C_Arise    //VIP1_REG8
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_clk_detect    :1;
        CBIOS_U32    Line_cnt_wr_en    :1;
        CBIOS_U32    FB_base2    :30;
    };
}REG_MM3378C_Arise;


typedef union _REG_MM33790_Arise    //VIP1_REG9
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    interleave_enable    :1;
        CBIOS_U32    frame_by_frame_odd_on_top    :1;
        CBIOS_U32    flush_enable    :1;
        CBIOS_U32    frame_end_reset    :1;
        CBIOS_U32    fifo_overfloe_clr    :1;
        CBIOS_U32    XYZ_error_clr    :1;
        CBIOS_U32    FB_stride    :10;
        CBIOS_U32    reserved    :5;
        CBIOS_U32    FB_VBI_size    :11;
    };
}REG_MM33790_Arise;


typedef union _REG_MM33794_Arise    //VIP1_REG10
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    cap_full_frame    :1;
        CBIOS_U32    VIP_data_type    :1;
        CBIOS_U32    FB_max_size    :30;
    };
}REG_MM33794_Arise;


typedef union _REG_MM33798_Arise    //VIP1_REG11
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_VBI_count0    :16;
        CBIOS_U32    VIP_VBI_count1    :16;
    };
}REG_MM33798_Arise;


typedef union _REG_MM3379C_Arise    //VIP1_REG12
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_which_BAR    :2;
        CBIOS_U32    VIP_interlace_detect    :1;
        CBIOS_U32    VREF    :1;
        CBIOS_U32    HREF    :1;
        CBIOS_U32    FIELD_ID    :1;
        CBIOS_U32    frame_odd_even    :1;
        CBIOS_U32    reserved_0    :1;
        CBIOS_U32    EOF_INT_enable    :1;
        CBIOS_U32    HPD_INT_enable    :1;
        CBIOS_U32    EOF_INT    :1;
        CBIOS_U32    HPD_INT    :1;
        CBIOS_U32    VIP_HPD_INT_status    :2;
        CBIOS_U32    reserved_1    :18;
    };
}REG_MM3379C_Arise;


typedef union _REG_MM337A0_Arise    //VIP2_REG1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_reset    :1;
        CBIOS_U32    VIP_enable    :1;
        CBIOS_U32    video_mode    :7;
        CBIOS_U32    VIP_hsync_polarity_invert    :1;
        CBIOS_U32    VIP_vsync_polarity_invert    :1;
        CBIOS_U32    SCE    :1;
        CBIOS_U32    header_select    :1;
        CBIOS_U32    SMB    :2;
        CBIOS_U32    VBI_enable    :1;
        CBIOS_U32    RAW_VBI_enable    :1;
        CBIOS_U32    VIP_video_data_delay    :2;
        CBIOS_U32    VBI_header_type    :1;
        CBIOS_U32    VBI_double_mode    :1;
        CBIOS_U32    header_type    :1;
        CBIOS_U32    _10bits_mode    :1;
        CBIOS_U32    one_more_SVBI    :2;
        CBIOS_U32    DROP_frame_mode    :2;
        CBIOS_U32    Drop_VBI    :1;
        CBIOS_U32    drop_frame    :1;
        CBIOS_U32    drop_odd_even    :1;
        CBIOS_U32    VIP_load_work_register    :1;
        CBIOS_U32    Load_at_VIP_vsync_enable    :1;
    };
}REG_MM337A0_Arise;


typedef union _REG_MM337A4_Arise    //VIP2_REG2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    REG_auto_gen    :1;
        CBIOS_U32    control_word_0_select    :2;
        CBIOS_U32    control_word_1_select    :2;
        CBIOS_U32    control_word_2_select    :2;
        CBIOS_U32    control_word_3_select    :2;
        CBIOS_U32    VIP_hde_enable    :1;
        CBIOS_U32    VIP_sync_enable    :1;
        CBIOS_U32    enable_data_0    :1;
        CBIOS_U32    data_type    :1;
        CBIOS_U32    data_word_0_select    :2;
        CBIOS_U32    data_word_1_select    :2;
        CBIOS_U32    data_word_2_select    :2;
        CBIOS_U32    data_word_3_select    :2;
        CBIOS_U32    video_data_out_format    :1;
        CBIOS_U32    SVBI_data_out_format    :1;
        CBIOS_U32    VBI_control_word0_sel    :2;
        CBIOS_U32    VBI_control_word1_sel    :2;
        CBIOS_U32    VBI_control_word2_sel    :2;
        CBIOS_U32    VBI_control_word3_sel    :2;
        CBIOS_U32    Undefined_Bit_31_to_31    :1;
    };
}REG_MM337A4_Arise;


typedef union _REG_MM337A8_Arise    //VIP2_REG3
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_ver_start    :14;
        CBIOS_U32    reserved    :2;
        CBIOS_U32    VIP_ver_window_length    :14;
        CBIOS_U32    enable_error_detect    :1;
        CBIOS_U32    crop_window_enable    :1;
    };
}REG_MM337A8_Arise;


typedef union _REG_MM337AC_Arise    //VIP2_REG4
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_hor_start    :14;
        CBIOS_U32    reserved_0    :2;
        CBIOS_U32    VIP_hor_length    :14;
        CBIOS_U32    reserved_1    :2;
    };
}REG_MM337AC_Arise;


typedef union _REG_MM337B0_Arise    //VIP2_REG5
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    fifo_threshold    :7;
        CBIOS_U32    reserved_0    :1;
        CBIOS_U32    fifo_high_threshold    :7;
        CBIOS_U32    reserved_1    :1;
        CBIOS_U32    VIP_input_clock_inv    :1;
        CBIOS_U32    VIP_input_clock_delay_select    :4;
        CBIOS_U32    VIP_input_clock_xor_enable    :1;
        CBIOS_U32    VIP_input_data_select    :8;
        CBIOS_U32    reserved_2    :1;
        CBIOS_U32    VIP_DVO_enable    :1;
    };
}REG_MM337B0_Arise;


typedef union _REG_MM337B4_Arise    //VIP2_REG6
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FB_mode    :2;
        CBIOS_U32    FB_base0    :30;
    };
}REG_MM337B4_Arise;


typedef union _REG_MM337B8_Arise    //VIP2_REG7
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_overflow    :1;
        CBIOS_U32    XYZ_error_flg    :1;
        CBIOS_U32    FB_base1    :30;
    };
}REG_MM337B8_Arise;


typedef union _REG_MM337BC_Arise    //VIP2_REG8
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_clk_detect    :1;
        CBIOS_U32    Line_cnt_wr_en    :1;
        CBIOS_U32    FB_base2    :30;
    };
}REG_MM337BC_Arise;


typedef union _REG_MM337C0_Arise    //VIP2_REG9
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    interleave_enable    :1;
        CBIOS_U32    frame_by_frame_odd_on_top    :1;
        CBIOS_U32    flush_enable    :1;
        CBIOS_U32    frame_end_reset    :1;
        CBIOS_U32    fifo_overfloe_clr    :1;
        CBIOS_U32    XYZ_error_clr    :1;
        CBIOS_U32    FB_stride    :10;
        CBIOS_U32    reserved    :5;
        CBIOS_U32    FB_VBI_size    :11;
    };
}REG_MM337C0_Arise;


typedef union _REG_MM337C4_Arise   //VIP2_REG10
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    cap_full_frame    :1;
        CBIOS_U32    VIP_data_type    :1;
        CBIOS_U32    FB_max_size    :30;
    };
}REG_MM337C4_Arise;


typedef union _REG_MM337C8_Arise    //VIP2_REG11
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_VBI_count0    :16;
        CBIOS_U32    VIP_VBI_count1    :16;
    };
}REG_MM337C8_Arise;


typedef union _REG_MM337CC_Arise    //VIP2_REG12
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_which_BAR    :2;
        CBIOS_U32    VIP_interlace_detect    :1;
        CBIOS_U32    VREF    :1;
        CBIOS_U32    HREF    :1;
        CBIOS_U32    FIELD_ID    :1;
        CBIOS_U32    frame_odd_even    :1;
        CBIOS_U32    reserved_0    :1;
        CBIOS_U32    EOF_INT_enable    :1;
        CBIOS_U32    HPD_INT_enable    :1;
        CBIOS_U32    EOF_INT    :1;
        CBIOS_U32    HPD_INT    :1;
        CBIOS_U32    VIP_HPD_INT_status    :2;
        CBIOS_U32    reserved_1    :18;
    };
}REG_MM337CC_Arise;


typedef union _REG_MM337D0_Arise    //VIP3_REG1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_reset    :1;
        CBIOS_U32    VIP_enable    :1;
        CBIOS_U32    video_mode    :7;
        CBIOS_U32    VIP_hsync_polarity_invert    :1;
        CBIOS_U32    VIP_vsync_polarity_invert    :1;
        CBIOS_U32    SCE    :1;
        CBIOS_U32    header_select    :1;
        CBIOS_U32    SMB    :2;
        CBIOS_U32    VBI_enable    :1;
        CBIOS_U32    RAW_VBI_enable    :1;
        CBIOS_U32    VIP_video_data_delay    :2;
        CBIOS_U32    VBI_header_type    :1;
        CBIOS_U32    VBI_double_mode    :1;
        CBIOS_U32    header_type    :1;
        CBIOS_U32    _10bits_mode    :1;
        CBIOS_U32    one_more_SVBI    :2;
        CBIOS_U32    DROP_frame_mode    :2;
        CBIOS_U32    Drop_VBI    :1;
        CBIOS_U32    drop_frame    :1;
        CBIOS_U32    drop_odd_even    :1;
        CBIOS_U32    VIP_load_work_register    :1;
        CBIOS_U32    Load_at_VIP_vsync_enable    :1;
    };
}REG_MM337D0_Arise;


typedef union _REG_MM337D4_Arise    //VIP2_REG2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    REG_auto_gen    :1;
        CBIOS_U32    control_word_0_select    :2;
        CBIOS_U32    control_word_1_select    :2;
        CBIOS_U32    control_word_2_select    :2;
        CBIOS_U32    control_word_3_select    :2;
        CBIOS_U32    VIP_hde_enable    :1;
        CBIOS_U32    VIP_sync_enable    :1;
        CBIOS_U32    enable_data_0    :1;
        CBIOS_U32    data_type    :1;
        CBIOS_U32    data_word_0_select    :2;
        CBIOS_U32    data_word_1_select    :2;
        CBIOS_U32    data_word_2_select    :2;
        CBIOS_U32    data_word_3_select    :2;
        CBIOS_U32    video_data_out_format    :1;
        CBIOS_U32    SVBI_data_out_format    :1;
        CBIOS_U32    VBI_control_word0_sel    :2;
        CBIOS_U32    VBI_control_word1_sel    :2;
        CBIOS_U32    VBI_control_word2_sel    :2;
        CBIOS_U32    VBI_control_word3_sel    :2;
        CBIOS_U32    Undefined_Bit_31_to_31    :1;
    };
}REG_MM337D4_Arise;


typedef union _REG_MM337D8_Arise    //VIP3_REG3
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_ver_start    :14;
        CBIOS_U32    reserved    :2;
        CBIOS_U32    VIP_ver_window_length    :14;
        CBIOS_U32    enable_error_detect    :1;
        CBIOS_U32    crop_window_enable    :1;
    };
}REG_MM337D8_Arise;


typedef union _REG_MM337DC_Arise    //VIP3_REG4
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_hor_start    :14;
        CBIOS_U32    reserved_0    :2;
        CBIOS_U32    VIP_hor_length    :14;
        CBIOS_U32    reserved_1    :2;
    };
}REG_MM337DC_Arise;


typedef union _REG_MM337E0_Arise    //VIP3_REG5
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    fifo_threshold    :7;
        CBIOS_U32    reserved_0    :1;
        CBIOS_U32    fifo_high_threshold    :7;
        CBIOS_U32    reserved_1    :1;
        CBIOS_U32    VIP_input_clock_inv    :1;
        CBIOS_U32    VIP_input_clock_delay_select    :4;
        CBIOS_U32    VIP_input_clock_xor_enable    :1;
        CBIOS_U32    VIP_input_data_select    :8;
        CBIOS_U32    reserved_2    :1;
        CBIOS_U32    VIP_DVO_enable    :1;
    };
}REG_MM337E0_Arise;


typedef union _REG_MM337E4_Arise    //VIP3_REG6
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    FB_mode    :2;
        CBIOS_U32    FB_base0    :30;
    };
}REG_MM337E4_Arise;


typedef union _REG_MM337E8_Arise    //VIP3_REG7
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_overflow    :1;
        CBIOS_U32    XYZ_error_flg    :1;
        CBIOS_U32    FB_base1    :30;
    };
}REG_MM337E8_Arise;


typedef union _REG_MM337EC_Arise    //VIP3_REG8
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_clk_detect    :1;
        CBIOS_U32    Line_cnt_wr_en    :1;
        CBIOS_U32    FB_base2    :30;
    };
}REG_MM337EC_Arise;


typedef union _REG_MM337F0_Arise    //VIP3_REG9
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    interleave_enable    :1;
        CBIOS_U32    frame_by_frame_odd_on_top    :1;
        CBIOS_U32    flush_enable    :1;
        CBIOS_U32    frame_end_reset    :1;
        CBIOS_U32    fifo_overfloe_clr    :1;
        CBIOS_U32    XYZ_error_clr    :1;
        CBIOS_U32    FB_stride    :10;
        CBIOS_U32    reserved    :5;
        CBIOS_U32    FB_VBI_size    :11;
    };
}REG_MM337F0_Arise;


typedef union _REG_MM337F4_Arise    //VIP3_REG10
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    cap_full_frame    :1;
        CBIOS_U32    VIP_data_type    :1;
        CBIOS_U32    FB_max_size    :30;
    };
}REG_MM337F4_Arise;


typedef union _REG_MM337F8_Arise    //VIP3_REG11
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_VBI_count0    :16;
        CBIOS_U32    VIP_VBI_count1    :16;
    };
}REG_MM337F8_Arise;


typedef union _REG_MM337FC_Arise    //VIP3_REG12
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    VIP_which_BAR    :2;
        CBIOS_U32    VIP_interlace_detect    :1;
        CBIOS_U32    VREF    :1;
        CBIOS_U32    HREF    :1;
        CBIOS_U32    FIELD_ID    :1;
        CBIOS_U32    frame_odd_even    :1;
        CBIOS_U32    reserved_0    :1;
        CBIOS_U32    EOF_INT_enable    :1;
        CBIOS_U32    HPD_INT_enable    :1;
        CBIOS_U32    EOF_INT    :1;
        CBIOS_U32    HPD_INT    :1;
        CBIOS_U32    VIP_HPD_INT_status    :2;
        CBIOS_U32    reserved_1    :18;
    };
}REG_MM337FC_Arise;


typedef union _REG_MM33800_Arise    //dp
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DP1_EQ_USE_TP4    :1;
        CBIOS_U32    DP1_SW_FBOOST_3    :3;
        CBIOS_U32    DP1_SW_NX_P_3    :2;
        CBIOS_U32    DP1_SW_MPLL_P_3    :2;
        CBIOS_U32    DP1_SW_MPLL_M_3    :8;
        CBIOS_U32    DP1_Link_Rate_value_4    :8;
        CBIOS_U32    DP1_SWX    :1;
        CBIOS_U32    DP1_AUX_SWX    :1;
        CBIOS_U32    DP1_PH1REG_PDB    :1;
        CBIOS_U32    DP1_PH1REG_1V2    :2;
        CBIOS_U32    DP1_PH2REG_PDB    :1;
        CBIOS_U32    DP1_PH2REG_1V2    :2;
    };
}REG_MM33800_Arise;


typedef union _REG_MM33804_Arise    //dp
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DP1__CR_DONE_CLR    :1;
        CBIOS_U32    Reserved    :31;
    };
}REG_MM33804_Arise;


typedef union _REG_MM33810_Arise    //kevinq
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DP2_Clock_Debug    :1;
        CBIOS_U32    DP3_Clock_Debug    :1;
        CBIOS_U32    DP4_Clock_Debug    :1;
        CBIOS_U32    reserved_0    :3;
        CBIOS_U32    reserved_1    :1;
        CBIOS_U32    reserved_2    :1;
        CBIOS_U32    reserved_3    :1;
        CBIOS_U32    reserved_4    :1;
        CBIOS_U32    VIPCLK_INV    :1;
        CBIOS_U32    VIP1CLK_INV    :1;
        CBIOS_U32    VIP2CLK_INV    :1;
        CBIOS_U32    VIP3CLK_INV    :1;
        CBIOS_U32    PS1_BL_IDX    :18;
    };
}REG_MM33810_Arise;


typedef union _REG_MM33814_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_BL_IDX_R    :18;
        CBIOS_U32    PS1_BL_IDX2    :14;
    };
}REG_MM33814_Arise;


typedef union _REG_MM33818_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    ps1_base_address    :4;
        CBIOS_U32    PS1_BASE_R    :4;
        CBIOS_U32    PS1_INTR_ROFFSET    :4;
        CBIOS_U32    reserved    :20;
    };
}REG_MM33818_Arise;


typedef union _REG_MM3381C_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS1_BL_IDX2    :4;
        CBIOS_U32    PS1_BL_IDX_R2    :18;
        CBIOS_U32    reserved    :10;
    };
}REG_MM3381C_Arise;


typedef union _REG_MM33824_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    s_vsync_beg_vga_pre    :13;
        CBIOS_U32    reserved_0    :3;
        CBIOS_U32    s_vsync_end_vga_pre    :6;
        CBIOS_U32    reserved_1    :10;
    };
}REG_MM33824_Arise;


typedef union _REG_MM33828_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    s_htotal    :11;
        CBIOS_U32    reserved_0    :5;
        CBIOS_U32    s_hdisp_end    :11;
        CBIOS_U32    reserved_1    :5;
    };
}REG_MM33828_Arise;


typedef union _REG_MM3382C_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    s_hblnk_beg    :11;
        CBIOS_U32    reserved_0    :5;
        CBIOS_U32    s_hblnk_end    :10;
        CBIOS_U32    reserved_1    :6;
    };
}REG_MM3382C_Arise;


typedef union _REG_MM33830_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    s_hsync_beg    :11;
        CBIOS_U32    reserved_0    :5;
        CBIOS_U32    s_hsync_end    :6;
        CBIOS_U32    reserved_1    :10;
    };
}REG_MM33830_Arise;


typedef union _REG_MM33834_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS_Enable    :1;
        CBIOS_U32    SS_Use_Mmio_En    :1;
        CBIOS_U32    Reserved    :30;
    };
}REG_MM33834_Arise;


typedef union _REG_MM33840_Arise    //background_overlay_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ka_3to0_Or_Ks    :4;
        CBIOS_U32    Ka_7to4_Or_Kp    :4;
        CBIOS_U32    Reserved_0    :2;
        CBIOS_U32    Ovl0_Input_Stream    :2;
        CBIOS_U32    Color_Key_Sel    :2;
        CBIOS_U32    Alpha_Select     :2;
        CBIOS_U32    Alpha_Rang    :1;
        CBIOS_U32    Alpha_Round    :1;
        CBIOS_U32    Reserved_1    :6;
        CBIOS_U32    Key_Mode    :4;
        CBIOS_U32    Invert_Alpha_Or_Ka    :1;
        CBIOS_U32    Ovl0_One_Shot    :1;
        CBIOS_U32    Ovl0_Vsync_Off_Flip    :1;
        CBIOS_U32    Ovl0_Enable_Work    :1;
    };
}REG_MM33840_Arise;


typedef union _REG_MM33844_Arise   //background_overlay_plane_alpha
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Ovl0_Plane_Alpha    :8;
        CBIOS_U32    Reserved_0    :6;
        CBIOS_U32    Ovl0_Alpha_Blend_Mode    :1;
        CBIOS_U32    Reserved_1    :17;
    };
}REG_MM33844_Arise;


typedef union _REG_MM33854_Arise    //SS1_burst_index
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    SS1_burst_index    :18;
        CBIOS_U32    Reserved    :14;
    };
}REG_MM33854_Arise;


typedef union _REG_MM33858_Arise    //TS1_win1_burst_index
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_win1_burst_index    :18;
        CBIOS_U32    Reserved    :14;
    };
}REG_MM33858_Arise;


typedef union _REG_MM3385C_Arise    //TS1_win2_burst_index
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    TS_win2_burst_index    :18;
        CBIOS_U32    Reserved    :14;
    };
}REG_MM3385C_Arise;


typedef union _REG_MM33860_Arise    //QS_win1_burst_index
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_win1_burst_index    :18;
        CBIOS_U32    Reserved    :14;
    };
}REG_MM33860_Arise;


typedef union _REG_MM33864_Arise    //QS_win2_burst_index
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    QS_win2_burst_index    :18;
        CBIOS_U32    Reserved    :14;
    };
}REG_MM33864_Arise;


typedef union _REG_MM33868_Arise    //DAC_CSC_REG1
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Dac_Csc_In_Format    :3;
        CBIOS_U32    Dac_Csc_Out_Format    :3;
        CBIOS_U32    Dac_Csc_Program    :1;
        CBIOS_U32    Reserved    :25;
    };
}REG_MM33868_Arise;


typedef union _REG_MM3386C_Arise    //DAC_CSC_REG2
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DAC_CSC_COEF1    :14;
        CBIOS_U32    DAC_CSC_COEF2    :14;
        CBIOS_U32    reserved    :4;
    };
}REG_MM3386C_Arise;


typedef union _REG_MM33870_Arise    //DAC_CSC_REG3
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DAC_CSC_COEF3    :14;
        CBIOS_U32    DAC_CSC_COEF4    :14;
        CBIOS_U32    reserved    :4;
    };
}REG_MM33870_Arise;


typedef union _REG_MM33874_Arise    //DAC_CSC_REG4
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DAC_CSC_COEF5    :14;
        CBIOS_U32    DAC_CSC_COEF6    :14;
        CBIOS_U32    reserved    :4;
    };
}REG_MM33874_Arise;


typedef union _REG_MM33878_Arise    //DAC_CSC_REG5
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DAC_CSC_COEF7    :14;
        CBIOS_U32    DAC_CSC_COEF8    :14;
        CBIOS_U32    reserved    :4;
    };
}REG_MM33878_Arise;


typedef union _REG_MM3387C_Arise    //DAC_CSC_REG6
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DAC_CSC_COEF9    :14;
        CBIOS_U32    DAC_CSC_bright    :9;
        CBIOS_U32    reserved    :9;
    };
}REG_MM3387C_Arise;


typedef union _REG_MM33880_Arise    //DSCL1_register_group
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    REG_STRIDE_11_9    :3;
        CBIOS_U32    REG_OFFSET    :10;
        CBIOS_U32    reserved    :17;
        CBIOS_U32    DROPPED_FRAME_STATUS    :1;
        CBIOS_U32    DOUBLE_BUFF_STATUS    :1;
    };
}REG_MM33880_Arise;


typedef union _REG_MM33884_Arise    //HDTV1_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    HDTV_CSC_F1_10to8    :3;
        CBIOS_U32    Reserved_1    :1;
        CBIOS_U32    HDTV_CSC_F0_10to8    :3;
        CBIOS_U32    Reserved_2    :1;
        CBIOS_U32    HDTV_CSC_F3_10to8    :3;
        CBIOS_U32    Reserved_3    :1;
        CBIOS_U32    HDTV_CSC_F2_10to8    :3;
        CBIOS_U32    Reserved_4    :1;
        CBIOS_U32    HDTV_CSC_F5_10to8    :3;
        CBIOS_U32    Reserved_5    :1;
        CBIOS_U32    HDTV_CSC_F4_10to8    :3;
        CBIOS_U32    Reserved_6    :1;
        CBIOS_U32    HDTV_CSC_F7_10to8    :3;
        CBIOS_U32    Reserved_7    :1;
        CBIOS_U32    HDTV_CSC_F6_10to8    :3;
    };
}REG_MM33884_Arise;


typedef union _REG_MM33888_Arise    //VIP0_IIC_R/W_data_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DRV2HW_WDATA    :8;
        CBIOS_U32    Reserved_0    :8;
        CBIOS_U32    HW2DRV_RDATA    :8;
        CBIOS_U32    Reserved_1    :8;
    };
}REG_MM33888_Arise;


typedef union _REG_MM3388C_Arise    //VIP0_IIC_Control_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    START_Request    :1;
        CBIOS_U32    STOP_REQUEST    :1;
        CBIOS_U32    Write_Data_Available    :1;
        CBIOS_U32    READ_Finished    :1;
        CBIOS_U32    VIP_I2C_Function_Enable    :1;
        CBIOS_U32    I2C_Frequency_Sleect    :3;
        CBIOS_U32    Reserved_0    :8;
        CBIOS_U32    Slave_Receiver_Not_Ready    :1;
        CBIOS_U32    I2C_Status    :1;
        CBIOS_U32    command_buffer_nempty    :1;
        CBIOS_U32    command_buffer_nfull    :1;
        CBIOS_U32    Reserved_1    :12;
    };
}REG_MM3388C_Arise;


typedef union _REG_MM33890_Arise    //VIP1_IIC_R/W_data_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DRV2HW_WDATA    :8;
        CBIOS_U32    Reserved_0    :8;
        CBIOS_U32    HW2DRV_RDATA    :8;
        CBIOS_U32    Reserved_1    :8;
    };
}REG_MM33890_Arise;


typedef union _REG_MM33894_Arise    //VIP1_IIC_Control_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    START_Request    :1;
        CBIOS_U32    STOP_REQUEST    :1;
        CBIOS_U32    Write_Data_Available    :1;
        CBIOS_U32    READ_Finished    :1;
        CBIOS_U32    VIP_I2C_Function_Enable    :1;
        CBIOS_U32    I2C_Frequency_Sleect    :3;
        CBIOS_U32    Reserved_0    :8;
        CBIOS_U32    Slave_Receiver_Not_Ready    :1;
        CBIOS_U32    I2C_Status    :1;
        CBIOS_U32    command_buffer_nempty    :1;
        CBIOS_U32    command_buffer_nfull    :1;
        CBIOS_U32    Reserved_1    :12;
    };
}REG_MM33894_Arise;


typedef union _REG_MM33898_Arise    //VIP2_IIC_R/W_data_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DRV2HW_WDATA    :8;
        CBIOS_U32    Reserved_0    :8;
        CBIOS_U32    HW2DRV_RDATA    :8;
        CBIOS_U32    Reserved_1    :8;
    };
}REG_MM33898_Arise;


typedef union _REG_MM3389C_Arise    //VIP2_IIC_Control_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    START_Request    :1;
        CBIOS_U32    STOP_REQUEST    :1;
        CBIOS_U32    Write_Data_Available    :1;
        CBIOS_U32    READ_Finished    :1;
        CBIOS_U32    VIP_I2C_Function_Enable    :1;
        CBIOS_U32    I2C_Frequency_Sleect    :3;
        CBIOS_U32    Reserved_0    :8;
        CBIOS_U32    Slave_Receiver_Not_Ready    :1;
        CBIOS_U32    I2C_Status    :1;
        CBIOS_U32    command_buffer_nempty    :1;
        CBIOS_U32    command_buffer_nfull    :1;
        CBIOS_U32    Reserved_1    :12;
    };
}REG_MM3389C_Arise;


typedef union _REG_MM338A0_Arise    //VIP3_IIC_R/W_data_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    DRV2HW_WDATA    :8;
        CBIOS_U32    Reserved_0    :8;
        CBIOS_U32    HW2DRV_RDATA    :8;
        CBIOS_U32    Reserved_1    :8;
    };
}REG_MM338A0_Arise;


typedef union _REG_MM338A4_Arise    //VIP3_IIC_Control_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    START_Request    :1;
        CBIOS_U32    STOP_REQUEST    :1;
        CBIOS_U32    Write_Data_Available    :1;
        CBIOS_U32    READ_Finished    :1;
        CBIOS_U32    VIP_I2C_Function_Enable    :1;
        CBIOS_U32    I2C_Frequency_Sleect    :3;
        CBIOS_U32    Reserved_0    :8;
        CBIOS_U32    Slave_Receiver_Not_Ready    :1;
        CBIOS_U32    I2C_Status    :1;
        CBIOS_U32    command_buffer_nempty    :1;
        CBIOS_U32    command_buffer_nfull    :1;
        CBIOS_U32    Reserved_1    :12;
    };
}REG_MM338A4_Arise;


typedef union _REG_MM338A8_Arise    //VIP0_IIC_TIMEOUT_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    IIC_STATIMEOUT    :23;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    IIC_TIMEOUT_HIT    :1;
        CBIOS_U32    Reserved_1    :7;
    };
}REG_MM338A8_Arise;


typedef union _REG_MM338AC_Arise    //VIP1_IIC_TIMEOUT_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    IIC_STATIMEOUT    :23;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    IIC_TIMEOUT_HIT    :1;
        CBIOS_U32    Reserved_1    :7;
    };
}REG_MM338AC_Arise;


typedef union _REG_MM338B0_Arise    //VIP2_IIC_TIMEOUT_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    IIC_STATIMEOUT    :23;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    IIC_TIMEOUT_HIT    :1;
        CBIOS_U32    Reserved_1    :7;
    };
}REG_MM338B0_Arise;


typedef union _REG_MM338B4_Arise    //VIP3_IIC_TIMEOUT_register
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    IIC_STATIMEOUT    :23;
        CBIOS_U32    Reserved_0    :1;
        CBIOS_U32    IIC_TIMEOUT_HIT    :1;
        CBIOS_U32    Reserved_1    :7;
    };
}REG_MM338B4_Arise;


typedef union _REG_MM338B8_Arise    //HDTV1_control_register
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
        CBIOS_U32    Blank_Level_7to0    :8;
        CBIOS_U32    Blank_Level_9to8    :2;
        CBIOS_U32    _1x_2x_4x_oversampling_sel    :2;
        CBIOS_U32    Reserved    :4;
    };
}REG_MM338B8_Arise;


typedef union _REG_MM338BC_Arise    // 
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
}REG_MM338BC_Arise;


typedef union _REG_MM338C0_Arise    // 
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
}REG_MM338C0_Arise;


typedef union _REG_MM338C4_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_CSC_F0_7to0    :8;
        CBIOS_U32    HDTV_CSC_F1_7to0    :8;
        CBIOS_U32    HDTV_CSC_F2_7to0    :8;
        CBIOS_U32    HDTV_CSC_F3_7to0    :8;
    };
}REG_MM338C4_Arise;


typedef union _REG_MM338C8_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_CSC_F4_7to0    :8;
        CBIOS_U32    HDTV_CSC_F5_7to0    :8;
        CBIOS_U32    HDTV_CSC_F6_7to0    :8;
        CBIOS_U32    HDTV_CSC_F7_7to0    :8;
    };
}REG_MM338C8_Arise;


typedef union _REG_MM338CC_Arise    // 
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
}REG_MM338CC_Arise;


typedef union _REG_MM338D0_Arise    // 
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
}REG_MM338D0_Arise;


typedef union _REG_MM338D4_Arise    // 
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
}REG_MM338D4_Arise;


typedef union _REG_MM338D8_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    Hdtv_Broad_Pulse_14to7    :8;
        CBIOS_U32    Hdtv_Half_Sync_7to0    :8;
        CBIOS_U32    Hdtv_Half_Sync_10to8    :3;
        CBIOS_U32    Reserved    :5;
        CBIOS_U32    HDTV_CC_Clock_Ratio_7to0    :8;
    };
}REG_MM338D8_Arise;


typedef union _REG_MM338DC_Arise    // 
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
}REG_MM338DC_Arise;


typedef union _REG_MM338E0_Arise    // 
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
}REG_MM338E0_Arise;


typedef union _REG_MM338E4_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_Parm1_End_1st_Equalizing_7to0_0    :8;
        CBIOS_U32    HDTV_Parm1_End_1st_Equalizing_7to0_1    :8;
        CBIOS_U32    HDTV_HDE_7to0    :8;
        CBIOS_U32    HDTV_HDE_10to8    :3;
        CBIOS_U32    Reserved    :5;
    };
}REG_MM338E4_Arise;


typedef union _REG_MM338E8_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    HDTV_Parm3_Start_HDTV_HSYNC_7to0    :8;
        CBIOS_U32    HDTV_Parm4_End_1st_Serration_Start_2nd_Equalization_7to0    :8;
        CBIOS_U32    HDTV_Parm5_End_2nd_Equalization_7to0    :8;
        CBIOS_U32    HDTV_Parm6_End_2nd_Equalization_7to0    :8;
    };
}REG_MM338E8_Arise;


typedef union _REG_MM338EC_Arise    // 
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
}REG_MM338EC_Arise;


typedef union _REG_MM338F0_Arise    // 
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
}REG_MM338F0_Arise;


typedef union _REG_MM338F4_Arise    // 
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
}REG_MM338F4_Arise;


typedef union _REG_MM338F8_Arise    // 
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
}REG_MM338F8_Arise;


typedef union _REG_MM338FC_Arise    // 
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
}REG_MM338FC_Arise;

typedef union _REG_MM33E6C_Arise    // 
{
    CBIOS_U32    Value;
    struct
    {
        CBIOS_U32    PS2_BL_IDX                 :18;
        CBIOS_U32    Reserved0                  :8;
        CBIOS_U32    PS2_INTR_ROFFSET_B35_32    :4;
        CBIOS_U32    Reserved1                  :2;
    };
}REG_MM33E6C_Arise;
