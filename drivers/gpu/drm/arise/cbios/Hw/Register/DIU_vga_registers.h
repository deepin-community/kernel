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


typedef union _REG_ARX	//Attribute_Controller_Index_Register
{
	struct
	{
		CBIOS_U8	Attribute_Address	:5;
		CBIOS_U8	Enable_Video_Display_via_Palette	:1;
		CBIOS_U8	RESERVED	:2;
	};
	CBIOS_U8	Value;
}REG_ARX;


typedef union _REG_ARD	//Attribute_Controller_Data_Read_Register
{
	struct
	{
		CBIOS_U8	Attribute_Data	:8;
	};
	CBIOS_U8	Value;
}REG_ARD;


typedef union _REG_AR00_AR0F	//Palette_Registers_0-15
{
	struct
	{
		CBIOS_U8	Palette_Color_Primary_RGB	:3;
		CBIOS_U8	Palette_Color__SR__SGI_SBV	:3;
		CBIOS_U8	RESERVED	:2;
	};
	CBIOS_U8	Value;
}REG_AR00_AR0F;


typedef union _REG_AR10	//Atrribute_Mode_Control_Register
{
	struct
	{
		CBIOS_U8	Attribute_Control_Mode	:1;
		CBIOS_U8	Monochrome_Attributes	:1;
		CBIOS_U8	Enable_Line_Graphics	:1;
		CBIOS_U8	Enable_Blinking	:1;
		CBIOS_U8	RESERVED	:1;
		CBIOS_U8	Enable_Top_Panning	:1;
		CBIOS_U8	Select_256_Color	:1;
		CBIOS_U8	Select_Pixel_Padding	:1;
	};
	CBIOS_U8	Value;
}REG_AR10;


typedef union _REG_AR11	//Border_Color_Register
{
	struct
	{
		CBIOS_U8	Border_Color	:8;
	};
	CBIOS_U8	Value;
}REG_AR11;


typedef union _REG_AR12	//Color_Plane_Enable_Register
{
	struct
	{
		CBIOS_U8	Display_Plane_Enable	:4;
		CBIOS_U8	Video_Test_Select	:2;
		CBIOS_U8	RESERVED	:2;
	};
	CBIOS_U8	Value;
}REG_AR12;


typedef union _REG_AR13	//Horizontal_Pixel_Panning_Register
{
	struct
	{
		CBIOS_U8	Number_of_Pixel_Pan_Shift_Left	:4;
		CBIOS_U8	RESERVED	:4;
	};
	CBIOS_U8	Value;
}REG_AR13;


typedef union _REG_AR14	//Pixel_Padding_Register
{
	struct
	{
		CBIOS_U8	Pixel_Padding_V5_V4	:2;
		CBIOS_U8	Pixel_Padding_V7_V6	:2;
		CBIOS_U8	RESERVED	:4;
	};
	CBIOS_U8	Value;
}REG_AR14;


typedef union _REG_VGA_STATUS_0	//Input_Status_0_Register_(Read_Only)
{
	struct
	{
		CBIOS_U8	RESERVED	:4;
		CBIOS_U8	DAC1_B_Sense	:1;
		CBIOS_U8	DAC1_G_Sense	:1;
		CBIOS_U8	DAC1_R_Sense	:1;
		CBIOS_U8	CRT_Vert_Retrace_Interrupt_Status	:1;
	};
	CBIOS_U8	Value;
}REG_VGA_STATUS_0;


typedef union _REG_VGA_ENABLE	//VGA_Display_Enable_Register
{
	struct
	{
		CBIOS_U8	Chip_Enable	:1;
		CBIOS_U8	RESERVED	:7;
	};
	CBIOS_U8	Value;
}REG_VGA_ENABLE;


typedef union _REG_SRX	//Sequencer_Index_Register
{
	struct
	{
		CBIOS_U8	Sequencer_Address_Index	:8;
	};
	CBIOS_U8	Value;
}REG_SRX;


typedef union _REG_SRD	//Sequencer_Data_Register
{
	struct
	{
		CBIOS_U8	Sequencer_Register_Data	:8;
	};
	CBIOS_U8	Value;
}REG_SRD;


typedef union _REG_SR00	//Reset_Register
{
	struct
	{
		CBIOS_U8	RESERVED_0	:1;
		CBIOS_U8	RESERVED_1	:1;
		CBIOS_U8	RESERVED_2	:6;
	};
	CBIOS_U8	Value;
}REG_SR00;


typedef union _REG_SR01	//Clocking_Mode_Register
{
	struct
	{
		CBIOS_U8	RESERVED_0	:1;
		CBIOS_U8	RESERVED_1	:1;
		CBIOS_U8	RESERVED_2	:1;
		CBIOS_U8	RESERVED_3	:1;
		CBIOS_U8	RESERVED_4	:1;
		CBIOS_U8	Screen_Off	:1;
		CBIOS_U8	RESERVED_5	:2;
	};
	CBIOS_U8	Value;
}REG_SR01;


typedef union _REG_SR02	//Enable_Write_Plane_Register
{
	struct
	{
		CBIOS_U8	Enable_Write_to_Plane	:4;
		CBIOS_U8	RESERVED	:4;
	};
	CBIOS_U8	Value;
}REG_SR02;


typedef union _REG_SR03	//Character_Font_Select_Register
{
	struct
	{
		CBIOS_U8	Reserved_0	:2;
		CBIOS_U8	Reserved_1	:2;
		CBIOS_U8	Reserved_2	:1;
		CBIOS_U8	Reserved_3	:1;
		CBIOS_U8	RESERVED	:2;
	};
	CBIOS_U8	Value;
}REG_SR03;


typedef union _REG_SR04	//Memory_Mode_Control_Register
{
	struct
	{
		CBIOS_U8	RESERVED_0	:1;
		CBIOS_U8	Extended_Memory_Access	:1;
		CBIOS_U8	Sequential_Addressing_Mode	:1;
		CBIOS_U8	Chain_4_Mode_Select	:1;
		CBIOS_U8	RESERVED_1	:4;
	};
	CBIOS_U8	Value;
}REG_SR04;


typedef union _REG_DAC_Mask	//DAC_Mask_Register
{
	struct
	{
		CBIOS_U8	DAC_Address_Mask	:8;
	};
	CBIOS_U8	Value;
}REG_DAC_Mask;


typedef union _REG_DAC_Read_Index	//DAC_Read_Index_Register
{
	struct
	{
		CBIOS_U8	DAC_Read_Address	:8;
	};
	CBIOS_U8	Value;
}REG_DAC_Read_Index;


typedef union _REG_DAC_Status	//DAC_Read_Status_Register
{
	struct
	{
		CBIOS_U8	DAC_Status	:2;
		CBIOS_U8	RESERVED	:6;
	};
	CBIOS_U8	Value;
}REG_DAC_Status;


typedef union _REG_DAC_Write_Index	//DAC_Write_Index_Register
{
	struct
	{
		CBIOS_U8	DAC_Write_Address	:8;
	};
	CBIOS_U8	Value;
}REG_DAC_Write_Index;


typedef union _REG_DAC_Data	//RAMDAC_Data_Register
{
	struct
	{
		CBIOS_U8	DAC_Read_Write_Data	:8;
	};
	CBIOS_U8	Value;
}REG_DAC_Data;


typedef union _REG_VGA_FCR_AD	//Feature_Control_Read_Register
{
	struct
	{
		CBIOS_U8	RESERVED_0	:3;
		CBIOS_U8	Vertical_Sync_Type	:1;
		CBIOS_U8	RESERVED_1	:4;
	};
	CBIOS_U8	Value;
}REG_VGA_FCR_AD;


typedef union _REG_VGA_MISC_Read	//Miscellaneous_Output_Register_(Read_Only)
{
	struct
	{
		CBIOS_U8	IO_Address_Select	:1;
		CBIOS_U8	Enable_RAM	:1;
		CBIOS_U8	Clock_Select	:2;
		CBIOS_U8	RESERVED	:1;
		CBIOS_U8	Page_Select	:1;
		CBIOS_U8	Sync_Polarity_Vertical_Size	:2;
	};
	CBIOS_U8	Value;
}REG_VGA_MISC_Read;


typedef union _REG_GRX	//Graphics_Controller_Index_Register
{
	struct
	{
		CBIOS_U8	Graphics_Controller_Address_Index	:4;
		CBIOS_U8	RESERVED	:4;
	};
	CBIOS_U8	Value;
}REG_GRX;


typedef union _REG_GRD	//Graphics_Controller_Data_Register
{
	struct
	{
		CBIOS_U8	Graphics_Controller_Data	:8;
	};
	CBIOS_U8	Value;
}REG_GRD;


typedef union _REG_GR00	//Graphics_Controller_Set/Reset_Data_Register
{
	struct
	{
		CBIOS_U8	Set_Reset_Data	:4;
		CBIOS_U8	RESERVED	:4;
	};
	CBIOS_U8	Value;
}REG_GR00;


typedef union _REG_GR01	//Graphics_Controller_Enable_Set/Reset_Data_Register
{
	struct
	{
		CBIOS_U8	Enable_Set_Reset_Data	:4;
		CBIOS_U8	RESERVED	:4;
	};
	CBIOS_U8	Value;
}REG_GR01;


typedef union _REG_GR02	//Color_Compare_Register
{
	struct
	{
		CBIOS_U8	Color_Compare_Data	:4;
		CBIOS_U8	RESERVED	:4;
	};
	CBIOS_U8	Value;
}REG_GR02;


typedef union _REG_GR03	//Raster_Operation/Rotate_Count_Register
{
	struct
	{
		CBIOS_U8	Rotate_Count	:3;
		CBIOS_U8	Raster_Operation_Select	:2;
		CBIOS_U8	RESERVED	:3;
	};
	CBIOS_U8	Value;
}REG_GR03;


typedef union _REG_GR04	//Read_Plane_Select_Register
{
	struct
	{
		CBIOS_U8	Read_Plane_Select	:2;
		CBIOS_U8	RESERVED	:6;
	};
	CBIOS_U8	Value;
}REG_GR04;


typedef union _REG_GR06	//Memory_Map_Mode_Control_Register
{
	struct
	{
		CBIOS_U8	Select_Addressing_Mode	:1;
		CBIOS_U8	Chain_Odd_Even	:1;
		CBIOS_U8	Memory_Map_Mode	:2;
		CBIOS_U8	RESERVED	:4;
	};
	CBIOS_U8	Value;
}REG_GR06;


typedef union _REG_GR07	//Color_Don't_Care_Register
{
	struct
	{
		CBIOS_U8	Compare_Plane_Select	:4;
		CBIOS_U8	RESERVED	:4;
	};
	CBIOS_U8	Value;
}REG_GR07;


typedef union _REG_GR08	//Bit_Mask_Register
{
	struct
	{
		CBIOS_U8	Bit_Mask	:8;
	};
	CBIOS_U8	Value;
}REG_GR08;


typedef union _REG_CRX	//CRT_Controller_Index_Register
{
	struct
	{
		CBIOS_U8	CRTC_Address_Index	:8;
	};
	CBIOS_U8	Value;
}REG_CRX;


typedef union _REG_CRD	//CRT_Controller_Data_Register
{
	struct
	{
		CBIOS_U8	CRTC_Data	:8;
	};
	CBIOS_U8	Value;
}REG_CRD;


typedef union _REG_CR00_Pair	//Horizontal_Total_Register_Pair
{
	struct
	{
		CBIOS_U8	Horizontal_Total_7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR00_Pair;


typedef union _REG_CR01_Pair	//Horizontal_Display_End_Register_Pair
{
	struct
	{
		CBIOS_U8	Horizontal_Display_End_7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR01_Pair;


typedef union _REG_CR02_Pair	//Start_Horizontal_Blank_Register_Pair
{
	struct
	{
		CBIOS_U8	Start_Horizontal_Blank_7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR02_Pair;


typedef union _REG_CR03_Pair	//End_Horizontal_Blank_Register_Pair
{
	struct
	{
		CBIOS_U8	End_Horizontal_Blank_9to5	:5;
		CBIOS_U8	Display_Enable_Skew	:2;
		CBIOS_U8	RESERVED	:1;
	};
	CBIOS_U8	Value;
}REG_CR03_Pair;


typedef union _REG_CR04_Pair	//Start_Horizontal_Sync_Position_Register_Pair
{
	struct
	{
		CBIOS_U8	Start_Horizontal_Sync_Position_7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR04_Pair;


typedef union _REG_CR05_Pair	//End_Horizontal_Sync_Position_Register_Pair
{
	struct
	{
		CBIOS_U8	End_Horizontal_Sync_Position_4to0	:5;
		CBIOS_U8	HSYNC_Signal_Delay_Skew	:2;
		CBIOS_U8	End_Horizontal_Blank_bit_10	:1;
	};
	CBIOS_U8	Value;
}REG_CR05_Pair;


typedef union _REG_CR06_Pair	//Vertical_Total_Register_Pair
{
	struct
	{
		CBIOS_U8	Vertical_Total_7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR06_Pair;


typedef union _REG_CR07_Pair	//CRTC_Overflow_Register_Pair
{
	struct
	{
		CBIOS_U8	Vertical_Total_bit_8	:1;
		CBIOS_U8	Vertical_Display_End_bit_8	:1;
		CBIOS_U8	Vertical_Retrace_Start_bit_8	:1;
		CBIOS_U8	Start_Vertical_Blank_bit_8	:1;
		CBIOS_U8	Line_Compare_bit_8	:1;
		CBIOS_U8	Vertical_Total_bit_9	:1;
		CBIOS_U8	Vertical_Display_End_bit_9	:1;
		CBIOS_U8	Vertical_Retrace_Start_bit_9	:1;
	};
	CBIOS_U8	Value;
}REG_CR07_Pair;


typedef union _REG_CR08_Pair	//Preset_Row_Scan_Register_Pair
{
	struct
	{
		CBIOS_U8	Preset_Row_Scan_Count	:5;
		CBIOS_U8	Byte_Pan	:2;
		CBIOS_U8	RESERVED	:1;
	};
	CBIOS_U8	Value;
}REG_CR08_Pair;


typedef union _REG_CR09_Pair	//Maximum_Scan_Line_Register_Pair
{
	struct
	{
		CBIOS_U8	Reserved	:5;
		CBIOS_U8	Start_Vertical_Blank_bit_9	:1;
		CBIOS_U8	Line_Compare_bit_9	:1;
		CBIOS_U8	Double_Scan	:1;
	};
	CBIOS_U8	Value;
}REG_CR09_Pair;


typedef union _REG_CR0A_Pair	//Text_Cursor_Start_Scan_Line_Register_Pair
{
	struct
	{
		CBIOS_U8	Reserved	:5;
		CBIOS_U8	Text_Cursor_Off	:1;
		CBIOS_U8	RESERVED	:2;
	};
	CBIOS_U8	Value;
}REG_CR0A_Pair;


typedef union _REG_CR0B_Pair	//Text_Cursor_End_Scan_Line_Register_Pair
{
	struct
	{
		CBIOS_U8	Reserved	:5;
		CBIOS_U8	Text_Cursor_Signal_Delay_Skew	:2;
		CBIOS_U8	RESERVED	:1;
	};
	CBIOS_U8	Value;
}REG_CR0B_Pair;


typedef union _REG_CR0C_Pair	//Start_Address_High_Register_Pair
{
	struct
	{
		CBIOS_U8	Display_Start_Address_15to8	:8;
	};
	CBIOS_U8	Value;
}REG_CR0C_Pair;


typedef union _REG_CR0D_Pair	//Start_Address_Low_Register_Pair
{
	struct
	{
		CBIOS_U8	Display_Start_Address__7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR0D_Pair;


typedef union _REG_CR0E_Pair	//Cursor_Location_Address_High_Register_Pair
{
	struct
	{
		CBIOS_U8	Reserved	:8;
	};
	CBIOS_U8	Value;
}REG_CR0E_Pair;


typedef union _REG_CR0F_Pair	//Cursor_Location_Address_Low_Register_Pair
{
	struct
	{
		CBIOS_U8	Reserved	:8;
	};
	CBIOS_U8	Value;
}REG_CR0F_Pair;


typedef union _REG_CR10_Pair	//Vertical_Retrace_Start_Register_Pair
{
	struct
	{
		CBIOS_U8	Vertical_Retrace_Start_7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR10_Pair;


typedef union _REG_CR11_Pair	//Vertical_Retrace_End_Register_Pair
{
	struct
	{
		CBIOS_U8	Vertical_Retrace_End_3to0	:4;
		CBIOS_U8	Clear_Vertical_Retrace_Interrupt	:1;
		CBIOS_U8	Disable_Vertical_Interrupt	:1;
		CBIOS_U8	Reserved	:1;
		CBIOS_U8	Lock_Writes_to_CR00_to_CR07	:1;
	};
	CBIOS_U8	Value;
}REG_CR11_Pair;


typedef union _REG_CR12_Pair	//Vertical_Display_End_Register_Pair
{
	struct
	{
		CBIOS_U8	Vertical_Display_End_7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR12_Pair;


typedef union _REG_CR13_Pair	//Screen_Offset_Register_Pair
{
	struct
	{
		CBIOS_U8	Logical_Screen_Width_7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR13_Pair;


typedef union _REG_CR14_Pair	//Underline_Location_Register_Pair
{
	struct
	{
		CBIOS_U8	Reserved	:5;
		CBIOS_U8	Count_by_4_Mode	:1;
		CBIOS_U8	DoubleWord_Addressing	:1;
		CBIOS_U8	RESERVED	:1;
	};
	CBIOS_U8	Value;
}REG_CR14_Pair;


typedef union _REG_CR15_Pair	//Start_Vertical_Blank_Register_Pair
{
	struct
	{
		CBIOS_U8	Start_Vertical_Blank_7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR15_Pair;


typedef union _REG_CR16_Pair	//End_Vertical_Blank_Register_Pair
{
	struct
	{
		CBIOS_U8	End_Vertical_Blank_7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR16_Pair;


typedef union _REG_CR17_Pair	//CRTC_Mode_Control_Register_Pair
{
	struct
	{
		CBIOS_U8	Reserved_0	:1;
		CBIOS_U8	Reserved_1	:1;
		CBIOS_U8	Vertical_Total_Double_Mode	:1;
		CBIOS_U8	Reserved_2	:1;
		CBIOS_U8	RESERVED	:1;
		CBIOS_U8	Reserved_3	:1;
		CBIOS_U8	Reserved_4	:1;
		CBIOS_U8	Hardware_Reset	:1;
	};
	CBIOS_U8	Value;
}REG_CR17_Pair;


typedef union _REG_CR18_Pair	//Line_Compare_Register_Pair
{
	struct
	{
		CBIOS_U8	Line_Compare_7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR18_Pair;


typedef union _REG_CR19	//Attribute_Controller_Alternate_Register
{
	struct
	{
		CBIOS_U8	I_O_Address_Select	:1;
		CBIOS_U8	Enable_Display_Memory_Access_from_CPU	:1;
		CBIOS_U8	Clock_Select	:1;
		CBIOS_U8	Reserved	:1;
		CBIOS_U8	Vertical_Sync_Type	:1;
		CBIOS_U8	Page_Select	:1;
		CBIOS_U8	Sync_Polarity_and_Vertical_Size	:2;
	};
	CBIOS_U8	Value;
}REG_CR19;


typedef union _REG_CR1A	//Attribute_Controller_Alternate_Register
{
	struct
	{
		CBIOS_U8	Display_Mode_Inactive	:1;
		CBIOS_U8	Reserved_0	:2;
		CBIOS_U8	Vertical_Sync_Active	:1;
		CBIOS_U8	Reserved_1	:3;
		CBIOS_U8	CRT_Vertical_Retrace_Interrupt_Status	:1;
	};
	CBIOS_U8	Value;
}REG_CR1A;


typedef union _REG_CR26	//Attribute_Controller_Alternate_Register
{
	struct
	{
		CBIOS_U8	RESERVED	:8;
	};
	CBIOS_U8	Value;
}REG_CR26;


typedef union _REG_VGA_STATUS_1	//Input_Status_1_Read_Register
{
	struct
	{
		CBIOS_U8	Display_Mode_Inactive	:1;
		CBIOS_U8	RESERVED_0	:1;
		CBIOS_U8	RESERVED_1	:1;
		CBIOS_U8	Vert_Sync_Active	:1;
		CBIOS_U8	Video_Signal_Test_bits_1to0	:2;
		CBIOS_U8	RESERVED_2	:2;
	};
	CBIOS_U8	Value;
}REG_VGA_STATUS_1;


