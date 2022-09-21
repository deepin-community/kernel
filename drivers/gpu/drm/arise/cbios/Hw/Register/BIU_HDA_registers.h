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


typedef union _REG_Global_Caps	//HDA_Global_Capabilities_Register
{
	struct
	{
		CBIOS_U32	_64OK	:1;
		CBIOS_U32	NSS	:2;
		CBIOS_U32	BSS	:5;
		CBIOS_U32	ISS	:4;
		CBIOS_U32	OSS	:4;
		CBIOS_U32	Minor_Version	:8;
		CBIOS_U32	Major_Version	:8;
	};
	CBIOS_U32	Value;
}REG_Global_Caps;


typedef union _REG_Total_Payload_Capacity	//HDA_Total_Payload_Capacity_Register
{
	struct
	{
		CBIOS_U32	Out_Pay	:16;
		CBIOS_U32	In_Pay	:16;
	};
	CBIOS_U32	Value;
}REG_Total_Payload_Capacity;


typedef union _REG_GLOBAL_CONTROL	//HDA__Global_Control_Register
{
	struct
	{
		CBIOS_U32	CRST	:1;
		CBIOS_U32	FCNTRL	:1;
		CBIOS_U32	Reserved_0	:6;
		CBIOS_U32	UNSOL	:1;
		CBIOS_U32	Reserved_1	:23;
	};
	CBIOS_U32	Value;
}REG_GLOBAL_CONTROL;


typedef union _REG_WAKEN	//HDA_Wake_Enable_&_State_Change_Status_Register
{
	struct
	{
		CBIOS_U32	Reserved_0	:1;
		CBIOS_U32	SDIWEN_1	:1;
		CBIOS_U32	SDIWEN_2	:1;
		CBIOS_U32	Reserved_1	:14;
		CBIOS_U32	STATESTS_1	:1;
		CBIOS_U32	STATESTS_2	:1;
		CBIOS_U32	Reserved_3	:12;
		CBIOS_U32	Undefined_Bit_31_to_31	:1;
	};
	CBIOS_U32	Value;
}REG_WAKEN;


typedef union _REG_GLOBAL_STS	//HDA_Global_Status_Register
{
	struct
	{
		CBIOS_U32	Reserved_0	:1;
		CBIOS_U32	FSTS	:1;
		CBIOS_U32	Reserved_1	:30;
	};
	CBIOS_U32	Value;
}REG_GLOBAL_STS;


typedef union _REG_Stream_Payload_Capacity	//HDA_Stream_Payload_Capacity_Register
{
	struct
	{
		CBIOS_U32	Out_Strm_Pay	:16;
		CBIOS_U32	In_Strm_Pay	:16;
	};
	CBIOS_U32	Value;
}REG_Stream_Payload_Capacity;


typedef union _REG_INTERRUPT_CONTROL	//HDA_Interrupt_Control_Register
{
	struct
	{
		CBIOS_U32	SIE_0	:1;
		CBIOS_U32	SIE_1	:1;
		CBIOS_U32	Reserved_0	:28;
		CBIOS_U32	CIE	:1;
		CBIOS_U32	GIE	:1;
	};
	CBIOS_U32	Value;
}REG_INTERRUPT_CONTROL;


typedef union _REG_INTERRUPT_STATUS	//HDA_Interrupt_Status_Register
{
	struct
	{
		CBIOS_U32	SIS_0	:1;
		CBIOS_U32	SIS_1	:1;
		CBIOS_U32	Reserved_0	:28;
		CBIOS_U32	CIS	:1;
		CBIOS_U32	GIS	:1;
	};
	CBIOS_U32	Value;
}REG_INTERRUPT_STATUS;


typedef union _REG_WALL_CLK_CTR	//HDA_Wall_Clock_Counter_Register
{
	struct
	{
		CBIOS_U32	Counter	:32;
	};
	CBIOS_U32	Value;
}REG_WALL_CLK_CTR;


typedef union _REG_STREAM_SYNC	//HDA_Stream_Synchronization_Register
{
	struct
	{
		CBIOS_U32	S0_SYNC	:1;
		CBIOS_U32	S1_SYNC	:1;
		CBIOS_U32	Reserved	:30;
	};
	CBIOS_U32	Value;
}REG_STREAM_SYNC;


typedef union _REG_CORB_LOW_ADDR	//HDA_CORB_Base_Address_Low_Register
{
	struct
	{
		CBIOS_U32	CORB_LBASE_6to0	:7;
		CBIOS_U32	CORB_LBASE_31to7	:25;
	};
	CBIOS_U32	Value;
}REG_CORB_LOW_ADDR;


typedef union _REG_CORB_UPPER_ADDR	//HDA_CORB_Base_Address_High_Register
{
	struct
	{
		CBIOS_U32	CORB_UPPER_ADDR_39to32	:8;
		CBIOS_U32	RESERVED	:24;
	};
	CBIOS_U32	Value;
}REG_CORB_UPPER_ADDR;


typedef union _REG_CORB_RW_POINTER	//HDA_CORB_Read_Write_Pointer_Register
{
	struct
	{
		CBIOS_U32	CORBWP	:8;
		CBIOS_U32	RESERVED_0	:8;
		CBIOS_U32	CORBRP	:8;
		CBIOS_U32	RESERVED_1	:7;
		CBIOS_U32	CORBRPRST	:1;
	};
	CBIOS_U32	Value;
}REG_CORB_RW_POINTER;


typedef union _REG_CORB_CTRL	//HDA_CORB_Control_Register
{
	struct
	{
		CBIOS_U32	CMEIE	:1;
		CBIOS_U32	CORBRUN	:1;
		CBIOS_U32	RESERVED_0	:6;
		CBIOS_U32	CMEI	:1;
		CBIOS_U32	RESERVED_1	:7;
		CBIOS_U32	CORBSIZE	:2;
		CBIOS_U32	RESERVED_2	:2;
		CBIOS_U32	CORBSZCAP	:4;
		CBIOS_U32	RESERVED_3	:8;
	};
	CBIOS_U32	Value;
}REG_CORB_CTRL;


typedef union _REG_RIRB_LOW_ADDR	//HDA_RIRB_Base_Address_Low_Register
{
	struct
	{
		CBIOS_U32	RIRB_LBASE_6to0	:7;
		CBIOS_U32	RIRB_LBASE_31to7	:25;
	};
	CBIOS_U32	Value;
}REG_RIRB_LOW_ADDR;


typedef union _REG_RIRB_UPPER_ADDR	//HDA_RIRB_Base_Address_High_Register
{
	struct
	{
		CBIOS_U32	RIRB_UPPER_ADDR_39to32	:8;
		CBIOS_U32	RESERVED	:24;
	};
	CBIOS_U32	Value;
}REG_RIRB_UPPER_ADDR;


typedef union _REG_RIRB_WP_RESP_INT	//HDA_RIRB_Write_Pointer_and_Response_Interrupt_Register
{
	struct
	{
		CBIOS_U32	RIRBWP	:8;
		CBIOS_U32	RESERVED_0	:7;
		CBIOS_U32	RIRBWPRST	:1;
		CBIOS_U32	RINTCNT	:8;
		CBIOS_U32	RESERVED_1	:7;
		CBIOS_U32	Undefined_Bit_31_to_31	:1;
	};
	CBIOS_U32	Value;
}REG_RIRB_WP_RESP_INT;


typedef union _REG_RIRB_CTRL	//HDA_RIRB_Control_Register
{
	struct
	{
		CBIOS_U32	RINTCTL	:1;
		CBIOS_U32	RIRBDMAEN	:1;
		CBIOS_U32	RIRBOIC	:1;
		CBIOS_U32	RESERVED_0	:6;
		CBIOS_U32	RINTFL	:1;
		CBIOS_U32	RESERVED_1	:1;
		CBIOS_U32	RINTOIS	:1;
		CBIOS_U32	RESERVED_2	:4;
		CBIOS_U32	RIRBSIZE	:2;
		CBIOS_U32	RESERVED_3	:2;
		CBIOS_U32	RIRBSZCAP	:4;
		CBIOS_U32	RESERVED_4	:8;
	};
	CBIOS_U32	Value;
}REG_RIRB_CTRL;


typedef union _REG_IM_CMD_OUT_INTERFACE	//HDA_Immediate_Command_Output_Interface_Register
{
	struct
	{
		CBIOS_U32	ICW	:32;
	};
	CBIOS_U32	Value;
}REG_IM_CMD_OUT_INTERFACE;


typedef union _REG_IM_RESP_INPUT_INTERFACE	//HDA_Immediate_Response_Input_Interface_Register
{
	struct
	{
		CBIOS_U32	IRR	:32;
	};
	CBIOS_U32	Value;
}REG_IM_RESP_INPUT_INTERFACE;


typedef union _REG_IM_CMD_STATUS	//HDA_Immediate_Command_Status_Register
{
	struct
	{
		CBIOS_U32	ICB	:1;
		CBIOS_U32	IRV	:1;
		CBIOS_U32	RESERVED_0	:2;
		CBIOS_U32	IRRADD	:4;
		CBIOS_U32	RESERVED_1	:24;
	};
	CBIOS_U32	Value;
}REG_IM_CMD_STATUS;


typedef union _REG_OS0_DESC_CTRL	//HDA_Output_Stream_0_Descriptor_Control_Register
{
	struct
	{
		CBIOS_U32	SRST	:1;
		CBIOS_U32	STRM_RUN	:1;
		CBIOS_U32	IOCE	:1;
		CBIOS_U32	FEIE	:1;
		CBIOS_U32	DEIE	:1;
		CBIOS_U32	RESERVED	:11;
		CBIOS_U32	STRIPE	:2;
		CBIOS_U32	TP	:1;
		CBIOS_U32	DIR	:1;
		CBIOS_U32	STRM_NUM	:4;
		CBIOS_U32	RESERVED_2	:2;
		CBIOS_U32	BCIS	:1;
		CBIOS_U32	FIFOE	:1;
		CBIOS_U32	DESE	:1;
		CBIOS_U32	FIFORDY	:1;
		CBIOS_U32	RESERVED_31to30	:2;
	};
	CBIOS_U32	Value;
}REG_OS0_DESC_CTRL;


typedef union _REG_OS0_DESC_LINK_POS	//HDA_Output_Stream_0_Descriptor_Link_Position_Register
{
	struct
	{
		CBIOS_U32	LPIB	:32;
	};
	CBIOS_U32	Value;
}REG_OS0_DESC_LINK_POS;


typedef union _REG_OS0_DESC_CBL	//HDA_Output_Stream_0_Descriptor_Cycle_Buffer_Length_Register
{
	struct
	{
		CBIOS_U32	CBL	:32;
	};
	CBIOS_U32	Value;
}REG_OS0_DESC_CBL;


typedef union _REG_OS0_DESC_LVI	//HDA_Output_Stream_0_Descriptor_Last_Valid_Index_Register
{
	struct
	{
		CBIOS_U32	LVI	:8;
		CBIOS_U32	RESERVED	:24;
	};
	CBIOS_U32	Value;
}REG_OS0_DESC_LVI;


typedef union _REG_OS0_DESC_FIFO_DESC	//HDA_Output_Stream_0_Descriptor_FIFO_Description_Register
{
	struct
	{
		CBIOS_U32	FIFO_SIZE	:8;
		CBIOS_U32	RESERVED	:8;
		CBIOS_U32	CHAN	:4;
		CBIOS_U32	BITS	:3;
		CBIOS_U32	RESERVED_23	:1;
		CBIOS_U32	DIV	:3;
		CBIOS_U32	MULT	:3;
		CBIOS_U32	BASE	:1;
		CBIOS_U32	RESERVED_31to30	:1;
	};
	CBIOS_U32	Value;
}REG_OS0_DESC_FIFO_DESC;


typedef union _REG_OS0_BDL_LOW_ADDR	//HDA_Stream_0_BDL_Base_Address_Low_Register
{
	struct
	{
		CBIOS_U32	BDL_LBASE_6to0	:7;
		CBIOS_U32	BDL_LBASE_31to7	:25;
	};
	CBIOS_U32	Value;
}REG_OS0_BDL_LOW_ADDR;


typedef union _REG_OS0_BDL_UPPER_ADDR	//HDA_Stream_0_BDL_Base_Address_High_Register
{
	struct
	{
		CBIOS_U32	BDL_UPPER_ADDR_39to32	:8;
		CBIOS_U32	RESERVED	:24;
	};
	CBIOS_U32	Value;
}REG_OS0_BDL_UPPER_ADDR;


typedef union _REG_OS1_DESC_CTRL	//HDA_Output_Stream_1_Descriptor_Control_Register
{
	struct
	{
		CBIOS_U32	SRST	:1;
		CBIOS_U32	STRM_RUN	:1;
		CBIOS_U32	IOCE	:1;
		CBIOS_U32	FEIE	:1;
		CBIOS_U32	DEIE	:1;
		CBIOS_U32	RESERVED	:11;
		CBIOS_U32	STRIPE	:2;
		CBIOS_U32	TP	:1;
		CBIOS_U32	DIR	:1;
		CBIOS_U32	STRM_NUM	:4;
		CBIOS_U32	RESERVED_2	:2;
		CBIOS_U32	BCIS	:1;
		CBIOS_U32	FIFOE	:1;
		CBIOS_U32	DESE	:1;
		CBIOS_U32	FIFORDY	:1;
		CBIOS_U32	RESERVED_31to30	:2;
	};
	CBIOS_U32	Value;
}REG_OS1_DESC_CTRL;


typedef union _REG_OS1_DESC_LINK_POS	//HDA_Output_Stream_1_Descriptor_Link_Position_Register
{
	struct
	{
		CBIOS_U32	LPIB	:32;
	};
	CBIOS_U32	Value;
}REG_OS1_DESC_LINK_POS;


typedef union _REG_OS1_DESC_CBL	//HDA_Output_Stream_1_Descriptor_Cycle_Buffer_Length_Register
{
	struct
	{
		CBIOS_U32	CBL	:32;
	};
	CBIOS_U32	Value;
}REG_OS1_DESC_CBL;


typedef union _REG_OS1_DESC_LVI	//HDA_Output_Stream_1_Descriptor_Last_Valid_Index_Register
{
	struct
	{
		CBIOS_U32	LVI	:8;
		CBIOS_U32	RESERVED	:24;
	};
	CBIOS_U32	Value;
}REG_OS1_DESC_LVI;


typedef union _REG_OS1_DESC_FIFO_DESC	//HDA_Output_Stream_1_Descriptor_FIFO_Description_Register
{
	struct
	{
		CBIOS_U32	FIFO_SIZE	:8;
		CBIOS_U32	RESERVED	:8;
		CBIOS_U32	CHAN	:4;
		CBIOS_U32	BITS	:3;
		CBIOS_U32	RESERVED_23	:1;
		CBIOS_U32	DIV	:3;
		CBIOS_U32	MULT	:3;
		CBIOS_U32	BASE	:1;
		CBIOS_U32	RESERVED_31to30	:1;
	};
	CBIOS_U32	Value;
}REG_OS1_DESC_FIFO_DESC;


typedef union _REG_OS1_BDL_LOW_ADDR	//HDA_Stream_1_BDL_Base_Address_Low_Register
{
	struct
	{
		CBIOS_U32	BDL_LBASE_6to0	:7;
		CBIOS_U32	BDL_LBASE_31to7	:25;
	};
	CBIOS_U32	Value;
}REG_OS1_BDL_LOW_ADDR;


typedef union _REG_OS1_BDL_UPPER_ADDR	//HDA_Stream_1_BDL_Base_Address_High_Register
{
	struct
	{
		CBIOS_U32	BDL_UPPER_ADDR_39to32	:8;
		CBIOS_U32	RESERVED	:24;
	};
	CBIOS_U32	Value;
}REG_OS1_BDL_UPPER_ADDR;


typedef union _REG_WALL_CLK_CTR_ALIAS	//HDA_Wall_Clock_Counter_Alias_Register
{
	struct
	{
		CBIOS_U32	Counter	:32;
	};
	CBIOS_U32	Value;
}REG_WALL_CLK_CTR_ALIAS;


typedef union _REG_OS0_LINK_POS_ALIAS	//HDA_Output_Stream_0_Descriptor_Link_Position_Alias_Register
{
	struct
	{
		CBIOS_U32	LPIB	:32;
	};
	CBIOS_U32	Value;
}REG_OS0_LINK_POS_ALIAS;


