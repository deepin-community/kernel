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


typedef union _REG_MM49200	//BIU_VCP_Active_Counter_Register
{
	struct
	{
		CBIOS_U32	RISC_ACT_CNT	:32;
	};
	CBIOS_U32	Value;
}REG_MM49200;


typedef union _REG_MM49204	//BIU_VCP_Operation_Counter_Register
{
	struct
	{
		CBIOS_U32	RISC_OP_CNT	:32;
	};
	CBIOS_U32	Value;
}REG_MM49204;


