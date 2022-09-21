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


typedef union _REG_MM48C00_MM48C78	//Tile_Surface_Start_Range_Registers_0-15
{
	struct
	{
		CBIOS_U32	TS_Height_8	:11;
		CBIOS_U32	TS_Start_Range_28to8	:21;
	};
	CBIOS_U32	Value;
}REG_MM48C00_MM48C78;


typedef union _REG_MM48C04_MM48C7C	//Tile_Surface_End_Range_Registers_0-15
{
	struct
	{
		CBIOS_U32	TS_Width_8	:11;
		CBIOS_U32	End_Range_Value	:21;
	};
	CBIOS_U32	Value;
}REG_MM48C04_MM48C7C;


typedef union _REG_MM48C80_MM48CBC	//Tile_Surface_Description_Registers__0-15
{
	struct
	{
		CBIOS_U32	Format	:4;
		CBIOS_U32	Rotation_Angle	:2;
		CBIOS_U32	Start_Range_Value_29	:1;
		CBIOS_U32	End_Range_Value_29	:1;
		CBIOS_U32	Reserved	:1;
		CBIOS_U32	Reciprical_of_Surface_Width	:14;
		CBIOS_U32	Destination_Surface	:1;
		CBIOS_U32	Destination_Surface_Width	:8;
	};
	CBIOS_U32	Value;
}REG_MM48C80_MM48CBC;


