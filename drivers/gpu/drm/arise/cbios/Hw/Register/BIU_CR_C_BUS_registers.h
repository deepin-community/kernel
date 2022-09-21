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


typedef union _REG_CR00_C	//BIU_Debug_Bus_Selection_Register
{
	struct
	{
		CBIOS_U8	Debug_Bus_Sub_module_Selection	:3;
		CBIOS_U8	Debug_Bus_Selection	:4;
		CBIOS_U8	Reserved	:1;
	};
	CBIOS_U8	Value;
}REG_CR00_C;


typedef union _REG_CR04_CR07_C	//Reserved_1_Dword
{
	struct
	{
		CBIOS_U32	Reserved	:32;
	};
	CBIOS_U32	Value;
}REG_CR04_CR07_C;


typedef union _REG_CR08_CR0B_C	//BIU_TLR_Debug_Register_Dword
{
	struct
	{
		CBIOS_U32	FBFF_full	:1;
		CBIOS_U32	FBFF_empty	:1;
		CBIOS_U32	MWFF_empty	:1;
		CBIOS_U32	TLA_idle	:1;
		CBIOS_U32	Mem_W_request_busy	:1;
		CBIOS_U32	NP_request_idle	:1;
		CBIOS_U32	PH_queue_empty	:1;
		CBIOS_U32	NPH_queue_empty	:1;
		CBIOS_U32	PH_queue_pop_enable	:1;
		CBIOS_U32	NPH_queue_pop_enable	:1;
		CBIOS_U32	Setup_state_of_TLA_pipe	:1;
		CBIOS_U32	Ready_state_of_TLA_pipe	:1;
		CBIOS_U32	Flush_state_of_TLA_pipe	:1;
		CBIOS_U32	Residual_state_of_TLA_pipe	:1;
		CBIOS_U32	Wait_state_of_TLA_pipe	:1;
		CBIOS_U32	Pre_ready_state_of_TLA_pipe	:1;
		CBIOS_U32	Prepare_phase	:1;
		CBIOS_U32	Header_phase	:1;
		CBIOS_U32	Data_phase	:1;
		CBIOS_U32	Pipe_contex_valid	:1;
		CBIOS_U32	FMTFF_full	:1;
		CBIOS_U32	MISC_bus_busy	:1;
		CBIOS_U32	Offset_Fifo_Full	:1;
		CBIOS_U32	Offset_Fifo_Empty	:1;
		CBIOS_U32	CBUCMD_RES	:2;
		CBIOS_U32	Reserved	:6;
	};
	CBIOS_U32	Value;
}REG_CR08_CR0B_C;


typedef union _REG_CR0C_CR0F_C	//Reserved_1_Dword
{
	struct
	{
		CBIOS_U32	Reverved	:32;
	};
	CBIOS_U32	Value;
}REG_CR0C_CR0F_C;


typedef union _REG_CR10_CR13_C	//Reserved_1_Dword
{
	struct
	{
		CBIOS_U32	Reverved	:32;
	};
	CBIOS_U32	Value;
}REG_CR10_CR13_C;


typedef union _REG_CR14_CR17_C	//Reserved_1_Dword
{
	struct
	{
		CBIOS_U32	Reserved	:32;
	};
	CBIOS_U32	Value;
}REG_CR14_CR17_C;


typedef union _REG_CR18_CR1B_C_000	//BIU_TLM_Debug_Queue_Request_Register_DWord
{
	struct
	{
		CBIOS_U32	Queue_read_ptr	:6;
		CBIOS_U32	Request_header_queue_empty	:1;
		CBIOS_U32	Comp_packet_queue_empty	:1;
		CBIOS_U32	Comp_packet_queue_read_ptr	:6;
		CBIOS_U32	Np_Read_Request_or_P_Write_Request	:1;
		CBIOS_U32	rqpcnt_inc	:1;
		CBIOS_U32	Completion_queue_available_space	:6;
		CBIOS_U32	Completion_queue_pass_completion	:1;
		CBIOS_U32	P_write_request	:1;
		CBIOS_U32	Read_request_control_state_machine	:3;
		CBIOS_U32	NP_read_request	:1;
		CBIOS_U32	Write_request_control_state	:2;
		CBIOS_U32	TLR_completion_sop_assert	:1;
		CBIOS_U32	TLR_return_bad_completion	:1;
	};
	CBIOS_U32	Value;
}REG_CR18_CR1B_C_000;


typedef union _REG_CR18_CR1B_C_001	//BIU_TLM_Debug_Queue_Request_Register_DWord
{
	struct
	{
		CBIOS_U32	completion_packet_queue_write_ptr	:6;
		CBIOS_U32	completion_write_control_state_machine	:2;
		CBIOS_U32	Length	:4;
		CBIOS_U32	CPL_DVALID	:1;
		CBIOS_U32	CPL_DISCARD_Delay_from_TLR	:1;
		CBIOS_U32	CPL_EOP	:1;
		CBIOS_U32	CPL_SOP	:1;
		CBIOS_U32	CPL_queue_available_space	:6;
		CBIOS_U32	CPL_queue_full	:1;
		CBIOS_U32	CPL_DISCARD	:1;
		CBIOS_U32	CPL_TAG	:6;
		CBIOS_U32	CPL_Header_Write_enable	:1;
		CBIOS_U32	CPL_Header_queue_full	:1;
	};
	CBIOS_U32	Value;
}REG_CR18_CR1B_C_001;


typedef union _REG_CR18_CR1B_C_010	//BIU_TLM_Debug_Queue_Request__Register_DWord
{
	struct
	{
		CBIOS_U32	CPL_Queue_SM	:2;
		CBIOS_U32	CPL_Header_queue_empty	:1;
		CBIOS_U32	CPL_Header_available	:1;
		CBIOS_U32	Request_FIFO_Control_SM	:3;
		CBIOS_U32	qtag_full_or_cplq_zero	:1;
		CBIOS_U32	Request_queue_write_pointer	:6;
		CBIOS_U32	Request_queue_full	:1;
		CBIOS_U32	Master_write_request	:1;
		CBIOS_U32	Write_data_queue_write_pointer	:6;
		CBIOS_U32	Write_data_queue_full	:1;
		CBIOS_U32	Master_read_request	:1;
		CBIOS_U32	CPL_length	:5;
		CBIOS_U32	Sub_index_rd_extag	:2;
		CBIOS_U32	TAG_queue_write_enable	:1;
	};
	CBIOS_U32	Value;
}REG_CR18_CR1B_C_010;


typedef union _REG_CR18_CR1B_C_011	//BIU_TLM_Debug_Queue_Request_Register_DWord
{
	struct
	{
		CBIOS_U32	Completion_queue_read_ptr	:6;
		CBIOS_U32	Completion_queue_empty	:1;
		CBIOS_U32	Completion_tag_queue_empty	:1;
		CBIOS_U32	TAG_queue_write_pointer	:6;
		CBIOS_U32	Completion_ECLK_write_ptr_0	:2;
		CBIOS_U32	Completion_tag_read_ptr	:6;
		CBIOS_U32	Completion_ECLK_write_ptr_1	:2;
		CBIOS_U32	Completion_ECLK_read_ptr	:6;
		CBIOS_U32	Completion_ECLK_write_ptr_2	:2;
	};
	CBIOS_U32	Value;
}REG_CR18_CR1B_C_011;


typedef union _REG_CR18_CR1B_C_100	//BIU_TLM_Debug_Queue_Request_Register_Dword
{
	struct
	{
		CBIOS_U32	MSI_USS_Control_SM	:3;
		CBIOS_U32	Reserved_4to3	:2;
		CBIOS_U32	CSP_request_pulse	:1;
		CBIOS_U32	MSI_request_pulse	:1;
		CBIOS_U32	Reserved_bit7	:1;
		CBIOS_U32	CR19_C_100_Reserved	:8;
		CBIOS_U32	CR1A_C_100_Reserved	:8;
		CBIOS_U32	CR1B_C_100_Reserved	:8;
	};
	CBIOS_U32	Value;
}REG_CR18_CR1B_C_100;


typedef union _REG_CR18_CR1B_C_101	//BIU_TLM_Debug_Queue_Request_Register_DWord
{
	struct
	{
		CBIOS_U32	CR18_C_101_Reserved	:8;
		CBIOS_U32	CR19_C_101_Reserved	:8;
		CBIOS_U32	CR1A_C_101_Reserved	:8;
		CBIOS_U32	CR1B_C_101_Reserved	:8;
	};
	CBIOS_U32	Value;
}REG_CR18_CR1B_C_101;


typedef union _REG_CR18_CR1B_C_110	//BIU_TLM_EMR_Debug_Queue_Request_Register_DWord
{
	struct
	{
		CBIOS_U32	out_emr_read_request	:1;
		CBIOS_U32	out_emr_write_request	:1;
		CBIOS_U32	in_emr_read_request	:1;
		CBIOS_U32	_1k_master_write_continue	:1;
		CBIOS_U32	master_write_continue	:1;
		CBIOS_U32	in_emr_write_request	:1;
		CBIOS_U32	out_emr_send_request_SM	:4;
		CBIOS_U32	in_emr_read_SM_0	:3;
		CBIOS_U32	out_emr_write_SM	:4;
		CBIOS_U32	in_emr_read_SM_1	:4;
		CBIOS_U32	in_emr_write_SM	:6;
		CBIOS_U32	Reserved_31to27	:5;
	};
	CBIOS_U32	Value;
}REG_CR18_CR1B_C_110;


typedef union _REG_CR18_CR1B_C_111	//BIU_TLM_Debug_Reserved_Register_DWord
{
	struct
	{
		CBIOS_U32	Reserved_31to0	:32;
	};
	CBIOS_U32	Value;
}REG_CR18_CR1B_C_111;


typedef union _REG_CR1C_CR1F_C_000	//BIU_MSG_FSM_State_Debug_Register_DWord
{
	struct
	{
		CBIOS_U32	MSG_FSM_state	:3;
		CBIOS_U32	Reserved_0	:5;
		CBIOS_U32	Reserved_1	:8;
		CBIOS_U32	Reserved_2	:8;
		CBIOS_U32	Reserved_3	:8;
	};
	CBIOS_U32	Value;
}REG_CR1C_CR1F_C_000;


typedef union _REG_CR1C_CR1F_C_100	//BIU_MSG_Header_31:0_Debug_Register_Dword
{
	struct
	{
		CBIOS_U32	message_header_31to0	:32;
	};
	CBIOS_U32	Value;
}REG_CR1C_CR1F_C_100;


typedef union _REG_CR1C_CR1F_C_101	//BIU_MSG_Header_63:32_Debug_Register_Dword
{
	struct
	{
		CBIOS_U32	message_header_63to32	:32;
	};
	CBIOS_U32	Value;
}REG_CR1C_CR1F_C_101;


typedef union _REG_CR1C_CR1F_C_110	//BIU_MSG_Header_95:64_Debug_Register_Dword
{
	struct
	{
		CBIOS_U32	message_header_95to64	:32;
	};
	CBIOS_U32	Value;
}REG_CR1C_CR1F_C_110;


typedef union _REG_CR1C_CR1F_C_111	//BIU_MSG_Header_127:96_Debug_Register_Dword
{
	struct
	{
		CBIOS_U32	message_header_127to96	:32;
	};
	CBIOS_U32	Value;
}REG_CR1C_CR1F_C_111;


typedef union _REG_CR20_C	//BIU_PMC_State_Debug_Register
{
	struct
	{
		CBIOS_U8	PM_state	:4;
		CBIOS_U8	PM_sub_state_of_TX_L0s	:2;
		CBIOS_U8	PM_sub_state_of_RX_L0s	:2;
	};
	CBIOS_U8	Value;
}REG_CR20_C;


typedef union _REG_CR21_C	//BIU_PMC_Command_and_Gating_Clock_Debug_Register
{
	struct
	{
		CBIOS_U8	PM_command	:2;
		CBIOS_U8	block_TLP	:1;
		CBIOS_U8	gating_T_L_clock	:1;
		CBIOS_U8	gating_T_R_clock	:1;
		CBIOS_U8	block_DLLP	:1;
		CBIOS_U8	gating_D_L_TX_clock	:1;
		CBIOS_U8	Reserved	:1;
	};
	CBIOS_U8	Value;
}REG_CR21_C;


typedef union _REG_CR22_C	//BIU_PMC_Gating_D-L_Clock_Debug_Register
{
	struct
	{
		CBIOS_U8	gating_D_L_RX_clock	:1;
		CBIOS_U8	Reserved	:7;
	};
	CBIOS_U8	Value;
}REG_CR22_C;


typedef union _REG_CR23_C	//BIU_PMC_Debug_Reserved_Register
{
	struct
	{
		CBIOS_U8	Reserved	:8;
	};
	CBIOS_U8	Value;
}REG_CR23_C;


typedef union _REG_CR24_CR27_C_000	//Reserved_DWord
{
	struct
	{
		CBIOS_U32	Reserved	:32;
	};
	CBIOS_U32	Value;
}REG_CR24_CR27_C_000;


typedef union _REG_CR24_CR27_C_001	//Reserved_DWord
{
	struct
	{
		CBIOS_U32	Reserved	:32;
	};
	CBIOS_U32	Value;
}REG_CR24_CR27_C_001;


typedef union _REG_CR24_CR27_C_010	//BIU_CIF_CFA_Status_Debug_Register_DWord
{
	struct
	{
		CBIOS_U32	DQEmpty	:1;
		CBIOS_U32	FBFF2_EMPTY	:1;
		CBIOS_U32	FBFF_EMPTY	:1;
		CBIOS_U32	DQFull	:1;
		CBIOS_U32	DQFull_1	:1;
		CBIOS_U32	CFA_CS	:3;
		CBIOS_U32	OFFSET2_EMPTY	:1;
		CBIOS_U32	OFFSET_EMPTY	:1;
		CBIOS_U32	APT_CFA_EMPTY	:1;
		CBIOS_U32	CFAIFX_BUSY	:1;
		CBIOS_U32	VGAMR_W	:1;
		CBIOS_U32	VGAM_REQ	:1;
		CBIOS_U32	INVALID_CPURW	:1;
		CBIOS_U32	BIU_MXU_RW	:1;
		CBIOS_U32	BIU_MXU_REQ	:1;
		CBIOS_U32	vga_idle	:1;
		CBIOS_U32	DQRe	:1;
		CBIOS_U32	PWH_TIMEOUT	:1;
		CBIOS_U32	CR26_C_010_Reserved_23to20	:4;
		CBIOS_U32	CR27_C_010_Reserved	:8;
	};
	CBIOS_U32	Value;
}REG_CR24_CR27_C_010;


typedef union _REG_CR24_CR27_C_011	//BIU_CIF_IO_Debug_Register_DWord
{
	struct
	{
		CBIOS_U32	read_ROM_registers	:1;
		CBIOS_U32	write_ROM_registers	:1;
		CBIOS_U32	RW_LUT_registers	:1;
		CBIOS_U32	RW_BIU_registers	:1;
		CBIOS_U32	Rw_Miu0_Registers	:1;
		CBIOS_U32	Rw_Miu1_registers	:1;
		CBIOS_U32	Rw_Miu2_Registers	:1;
		CBIOS_U32	Rw_Miu3_Registers	:1;
		CBIOS_U32	Rw_Mxu_Registers	:1;
		CBIOS_U32	Rw_Iga_Registers	:1;
		CBIOS_U32	Rw_3c3	:1;
		CBIOS_U32	BIU_DIU_MMIO_MODE	:1;
		CBIOS_U32	DIU_BIU_MIUCRALL	:1;
		CBIOS_U32	DIU_BIU_MIUCRSET	:2;
		CBIOS_U32	DIU_BIU_SELCRD	:2;
		CBIOS_U32	IO_RW_BS	:1;
		CBIOS_U32	IO_REQ_IFX	:1;
		CBIOS_U32	CR26_C_011_Reserved_23to19	:5;
		CBIOS_U32	CR27_C_011_Reserved	:8;
	};
	CBIOS_U32	Value;
}REG_CR24_CR27_C_011;


typedef union _REG_CR24_CR27_C_100	//BIU_CIF_MMIO_Debug_Register_DWord
{
	struct
	{
		CBIOS_U32	RW_error_header_registers	:1;
		CBIOS_U32	RW_Debug_registers	:1;
		CBIOS_U32	RW_TLA_registers	:1;
		CBIOS_U32	RW_CBU_registers	:1;
		CBIOS_U32	RW_BIU_registers	:1;
		CBIOS_U32	RW_CSP_registers	:1;
		CBIOS_U32	RW_SP_registers	:1;
		CBIOS_U32	sel_pcounter	:1;
		CBIOS_U32	sel_sli	:1;
		CBIOS_U32	sel_vpt	:1;
		CBIOS_U32	sel_mxu	:1;
		CBIOS_U32	TLR_MMIO_HDAD	:1;
		CBIOS_U32	MMIO_RW_BS	:1;
		CBIOS_U32	MMIO_REQ_BS	:1;
		CBIOS_U32	Reserved	:18;
	};
	CBIOS_U32	Value;
}REG_CR24_CR27_C_100;


typedef union _REG_CR24_CR27_C_101	//BIU_CIF_VGAIFX_Debug_Register_DWord
{
	struct
	{
		CBIOS_U32	VGAIFX_state_machine_Status	:3;
		CBIOS_U32	VGA_RW_BS	:1;
		CBIOS_U32	VGA_REQ_IFX	:1;
		CBIOS_U32	Reserved_0	:3;
		CBIOS_U32	Reserved_1	:8;
		CBIOS_U32	Reserved_2	:8;
		CBIOS_U32	Reserved_3	:8;
	};
	CBIOS_U32	Value;
}REG_CR24_CR27_C_101;


typedef union _REG_CR24_CR27_C_110	//BIU_CIF_VGAMFX_Debug_Reserved_Register_Dword
{
	struct
	{
		CBIOS_U32	Reserved_0	:8;
		CBIOS_U32	Reserved_1	:8;
		CBIOS_U32	Reserved_2	:8;
		CBIOS_U32	Reserved_3	:8;
	};
	CBIOS_U32	Value;
}REG_CR24_CR27_C_110;


typedef union _REG_CR24_CR27_C_111	//BIU_CIF_CBU2_Debug_Register_Dword
{
	struct
	{
		CBIOS_U32	Hardwared_to_01	:2;
		CBIOS_U32	CBUFF_FULL	:1;
		CBIOS_U32	Hardwared_to_00	:2;
		CBIOS_U32	CBUFF_EMPTY	:1;
		CBIOS_U32	Hardwared_to_000	:3;
		CBIOS_U32	Cbu_Fifo_Upper_Threshold_Arrived	:1;
		CBIOS_U32	Cbu_Fifo_Lower_Threshold_Arrived	:1;
		CBIOS_U32	Hardwared_to_0000	:4;
		CBIOS_U32	vptifx_State_Machine_Status	:1;
		CBIOS_U32	BIU_VPT_MASK	:4;
		CBIOS_U32	VPT_BIU_READ	:1;
		CBIOS_U32	BIU_VPT_REQ	:1;
		CBIOS_U32	CBUCMDFF_AV	:1;
		CBIOS_U32	CBUCMDFF_WE	:1;
		CBIOS_U32	Reserved	:8;
	};
	CBIOS_U32	Value;
}REG_CR24_CR27_C_111;


typedef union _REG_CR28_C	//BIU_TLM_Underrun_Overrun_Debug_Register
{
	struct
	{
		CBIOS_U8	TLT_BF_Underrun	:1;
		CBIOS_U8	TLT_BF_Overrun	:1;
		CBIOS_U8	TLM_CPLQ_Underrun	:1;
		CBIOS_U8	TLM_CPLQ_Overrun	:1;
		CBIOS_U8	TLM_WDQ_Underrun	:1;
		CBIOS_U8	TLM_WDQ_Overrun	:1;
		CBIOS_U8	TLM_RREQQ_Underrun	:1;
		CBIOS_U8	TLM_RREQQ_Overrun	:1;
	};
	CBIOS_U8	Value;
}REG_CR28_C;


typedef union _REG_CR29_C	//BIU_TLR_Debug_Register
{
	struct
	{
		CBIOS_U8	TLR_MEMW_BUSY	:1;
		CBIOS_U8	BIU_Frame_Buffer_FIFO_full	:1;
		CBIOS_U8	BIU_Frame_Buffer_FIFO_empty	:1;
		CBIOS_U8	BIU_MW_FIFO_empty	:1;
		CBIOS_U8	TLR_RDFF_empty	:1;
		CBIOS_U8	TLR_PHQ_empty	:1;
		CBIOS_U8	TLR_NPHQ_empty	:1;
		CBIOS_U8	TLR_FMTFF_full	:1;
	};
	CBIOS_U8	Value;
}REG_CR29_C;


typedef union _REG_CR2A_C	//BIU_Status_Register
{
	struct
	{
		CBIOS_U8	TLM_busy	:1;
		CBIOS_U8	TLT_busy	:1;
		CBIOS_U8	TLR_busy	:1;
		CBIOS_U8	DLR_busy	:1;
		CBIOS_U8	DLT_busy	:1;
		CBIOS_U8	CBU_busy	:1;
		CBIOS_U8	BIU_CBU_Interface_Status	:1;
		CBIOS_U8	CFAIFX_busy	:1;
	};
	CBIOS_U8	Value;
}REG_CR2A_C;


typedef union _REG_CR2B_C	//BIU_TLM_Error_Status_Register
{
	struct
	{
		CBIOS_U8	TLM_CPL_PLEN_ERR	:1;
		CBIOS_U8	TLM_CPL_TAG_ERR	:1;
		CBIOS_U8	TLM_CPL_ELEN_ERR	:1;
		CBIOS_U8	CBU2_BUSY	:1;
		CBIOS_U8	CBU2IFX_BUSY	:1;
		CBIOS_U8	Reserved	:3;
	};
	CBIOS_U8	Value;
}REG_CR2B_C;


typedef union _REG_CR2C_CR2F_C	//BIU_Debug_Bus_Reserved_Register_Dword
{
	struct
	{
		CBIOS_U32	Reserved	:32;
	};
	CBIOS_U32	Value;
}REG_CR2C_CR2F_C;


typedef union _REG_CR40_C	//Reserved
{
	struct
	{
		CBIOS_U8	Reserved	:8;
	};
	CBIOS_U8	Value;
}REG_CR40_C;


typedef union _REG_CR41_C	//BIU_TLR_Event_Trigger_Register
{
	struct
	{
		CBIOS_U8	Generate_CPL_TLPt	:1;
		CBIOS_U8	Generate_CPLD_TLP	:1;
		CBIOS_U8	Reserved	:6;
	};
	CBIOS_U8	Value;
}REG_CR41_C;


typedef union _REG_CR42_C	//Reserved
{
	struct
	{
		CBIOS_U8	Reserved	:8;
	};
	CBIOS_U8	Value;
}REG_CR42_C;


typedef union _REG_CR45_C	//Reserved_1_Byte
{
	struct
	{
		CBIOS_U8	Reserved	:8;
	};
	CBIOS_U8	Value;
}REG_CR45_C;


typedef union _REG_CR46_C	//BIU_MSG_Event_Trigger_Register
{
	struct
	{
		CBIOS_U8	force_send_ASSERT_DEASSERT_msg	:1;
		CBIOS_U8	force_send_correctable_err_message	:1;
		CBIOS_U8	force_send_nonfatal_err_message	:1;
		CBIOS_U8	force_send_fatal_err_message	:1;
		CBIOS_U8	Reserved	:3;
		CBIOS_U8	REG_MSG_TMODE	:1;
	};
	CBIOS_U8	Value;
}REG_CR46_C;


typedef union _REG_CR47_C	//BIU_PMC_Event_Trigger_Register
{
	struct
	{
		CBIOS_U8	force_send_PM_enter_L23_DLLP	:1;
		CBIOS_U8	force_send_PM_enter_L1_DLLP	:1;
		CBIOS_U8	force_send_PM_ASPM_L1_DLLP	:1;
		CBIOS_U8	force_send_PME_TO_ACK_message	:1;
		CBIOS_U8	force_enter_to_TX_L0s	:1;
		CBIOS_U8	force_enter_to_TX_L0_RX_L0	:1;
		CBIOS_U8	REG_PMC_TMODE_0	:1;
		CBIOS_U8	REG_PMC_TMODE_1	:1;
	};
	CBIOS_U8	Value;
}REG_CR47_C;


typedef union _REG_CR48_C	//BIU_TLR_Control_1_Register
{
	struct
	{
		CBIOS_U8	Threshold_for_update_PH_credit	:4;
		CBIOS_U8	Reserved	:3;
		CBIOS_U8	PWHD_Timer_Enable	:1;
	};
	CBIOS_U8	Value;
}REG_CR48_C;


typedef union _REG_CR56_C	//BIU_PCLK_Work_Mode_Select_Register
{
	struct
	{
		CBIOS_U8	Reserved	:3;
		CBIOS_U8	Pclk_Work_Mode_Select	:1;
		CBIOS_U8	RESERVED	:4;
	};
	CBIOS_U8	Value;
}REG_CR56_C;


typedef union _REG_CR5A_C	//BIU_PMC_L1_Timer_7:0__Register
{
	struct
	{
		CBIOS_U8	TIMER_L1	:8;
	};
	CBIOS_U8	Value;
}REG_CR5A_C;


typedef union _REG_CR5B_C	//BIU_PMC_L1_Timer_11:8_Register
{
	struct
	{
		CBIOS_U8	TIMER_L1	:4;
		CBIOS_U8	Reserved	:4;
	};
	CBIOS_U8	Value;
}REG_CR5B_C;


typedef union _REG_CR60_C	//BIU_Correctable_Error_Status_Register
{
	struct
	{
		CBIOS_U8	receiver_err_Status	:1;
		CBIOS_U8	Reserved	:3;
		CBIOS_U8	replay_timer_time_out	:1;
		CBIOS_U8	replay_num_rollover	:1;
		CBIOS_U8	Bad_TLP	:1;
		CBIOS_U8	Bad_DLLP	:1;
	};
	CBIOS_U8	Value;
}REG_CR60_C;


typedef union _REG_CR61_C	//BIU_Correctable_Error_Mask_Register
{
	struct
	{
		CBIOS_U8	Mask_of_receiver_err_Status	:1;
		CBIOS_U8	Reserved	:3;
		CBIOS_U8	Mask_of_timer_time_out	:1;
		CBIOS_U8	Mask_of_replay_num_rollover	:1;
		CBIOS_U8	Mask_of_Bad_TLP	:1;
		CBIOS_U8	Mask_of_Bad_DLLP	:1;
	};
	CBIOS_U8	Value;
}REG_CR61_C;


typedef union _REG_CR62_C	//BIU_UNCorrectable_Error_Status_1_Register
{
	struct
	{
		CBIOS_U8	UNSUPPORTED_Completion	:1;
		CBIOS_U8	Data_Link_Protocol_Error	:1;
		CBIOS_U8	POISIONED_TLP	:1;
		CBIOS_U8	Flow_Control_Protocol_Error	:1;
		CBIOS_U8	Completion_Timeout	:1;
		CBIOS_U8	Completer_Abort	:1;
		CBIOS_U8	Unexpected_Completion	:1;
		CBIOS_U8	Receiver_Overflow_Error	:1;
	};
	CBIOS_U8	Value;
}REG_CR62_C;


typedef union _REG_CR63_C	//BIU_UNCorrectable_Error_Status_2_Register
{
	struct
	{
		CBIOS_U8	unsupported_request_error	:1;
		CBIOS_U8	malformed_TLP	:1;
		CBIOS_U8	Reserved	:6;
	};
	CBIOS_U8	Value;
}REG_CR63_C;


typedef union _REG_CR64_C	//BIU_UNCorrectable_Error_Mask_1_Register
{
	struct
	{
		CBIOS_U8	Reserved_0	:1;
		CBIOS_U8	Data_link_protocol_error	:1;
		CBIOS_U8	poisioned_TLP	:1;
		CBIOS_U8	flow_control_protocol_error	:1;
		CBIOS_U8	completion_timeout	:1;
		CBIOS_U8	Reserved_1	:2;
		CBIOS_U8	receiver_overflow_error	:1;
	};
	CBIOS_U8	Value;
}REG_CR64_C;


typedef union _REG_CR65_C	//BIU_UNCorrectable_Error_Mask_2_Register
{
	struct
	{
		CBIOS_U8	unsupported_request_error	:1;
		CBIOS_U8	malformed_TLP	:1;
		CBIOS_U8	Reserved	:6;
	};
	CBIOS_U8	Value;
}REG_CR65_C;


typedef union _REG_CR69_C	//BIU_Timer_for_Post_Write_Hang_Detection_Register
{
	struct
	{
		CBIOS_U8	Timer	:8;
	};
	CBIOS_U8	Value;
}REG_CR69_C;


typedef union _REG_CR6A_C	//BIU_Timer_for_CSP_MMIO_Write_Hang_Detection_Register
{
	struct
	{
		CBIOS_U8	Timer	:8;
	};
	CBIOS_U8	Value;
}REG_CR6A_C;


typedef union _REG_CR6F_C	//BIU_Boundary_Request_Split_Register
{
	struct
	{
		CBIOS_U8	Reserved	:4;
		CBIOS_U8	Reg_Tlm_Rd_Len_Cfg	:2;
		CBIOS_U8	_0	:1;
		CBIOS_U8	_1	:1;
	};
	CBIOS_U8	Value;
}REG_CR6F_C;


typedef union _REG_CR70_C	//BIU_TLM_Control_1_Register
{
	struct
	{
		CBIOS_U8	Reserved_0	:3;
		CBIOS_U8	REG_TLM_REQ_LIMIT_EN	:1;
		CBIOS_U8	Reserved_1	:1;
		CBIOS_U8	REG_MULTI_MSI_EN	:1;
		CBIOS_U8	REG_TLB_HP	:1;
		CBIOS_U8	Reserved_bit7	:1;
	};
	CBIOS_U8	Value;
}REG_CR70_C;


typedef union _REG_CR74_C	//L0s_Recovery_Debug_Register
{
	struct
	{
		CBIOS_U8	Reserved_0	:3;
		CBIOS_U8	BIU_MXU_Priority_Control	:1;
		CBIOS_U8	Reserved_1	:4;
	};
	CBIOS_U8	Value;
}REG_CR74_C;


typedef union _REG_CR75_C_CR76_C	//Reserved_2_Bytes
{
	struct
	{
		CBIOS_U8	Reserved	:8;
	};
	CBIOS_U8	Value;
}REG_CR75_C_CR76_C;


typedef union _REG_CR77_C	//Reserved_1_Byte
{
	struct
	{
		CBIOS_U8	Reserved	:8;
	};
	CBIOS_U8	Value;
}REG_CR77_C;


typedef union _REG_CR78_C_CR7A_C	//Reserved_3_Bytes
{
	struct
	{
		CBIOS_U8	Reserved	:8;
	};
	CBIOS_U8	Value;
}REG_CR78_C_CR7A_C;


typedef union _REG_CR7D_C	//Dynamic_Clock_Management_Control_Register
{
	struct
	{
		CBIOS_U8	REG_DYN_CLK_EN	:1;
		CBIOS_U8	REG_GFX_BUSY_EN	:1;
		CBIOS_U8	Reserved_3to2	:2;
		CBIOS_U8	Reg_Gfx_Busy	:1;
		CBIOS_U8	Reserved_7to5	:3;
	};
	CBIOS_U8	Value;
}REG_CR7D_C;


typedef union _REG_CR7E_C	//Dynamic_Clock_Management_Timer_7:0_Register
{
	struct
	{
		CBIOS_U8	Reg_Dynamic_Clk_Close_Timer_7to0	:8;
	};
	CBIOS_U8	Value;
}REG_CR7E_C;


typedef union _REG_CR7F_C	//Dynamic_Clock_Management_Timer_15:8_Register
{
	struct
	{
		CBIOS_U8	Reg_Dynamic_Clk_Close_Timer_15to8	:8;
	};
	CBIOS_U8	Value;
}REG_CR7F_C;


typedef union _REG_CR80_C	//Device_ID_High_Register
{
	struct
	{
		CBIOS_U8	Chip_ID_15to8	:8;
	};
	CBIOS_U8	Value;
}REG_CR80_C;


typedef union _REG_CR81_C	//Device_ID_Low_Register
{
	struct
	{
		CBIOS_U8	Chip_ID_4to0	:5;
		CBIOS_U8	Chip_ID_7to5	:3;
	};
	CBIOS_U8	Value;
}REG_CR81_C;


typedef union _REG_CR82_C	//Revision_ID_Register
{
	struct
	{
		CBIOS_U8	Revision_ID	:8;
	};
	CBIOS_U8	Value;
}REG_CR82_C;


typedef union _REG_CR83_C	//Chip_ID/Revision_Register
{
	struct
	{
		CBIOS_U8	REVISION_Status	:4;
		CBIOS_U8	Chip_ID	:4;
	};
	CBIOS_U8	Value;
}REG_CR83_C;


typedef union _REG_CR84_C	//PCI_Subsystem_Vendor_ID_Shadow_Low_Register
{
	struct
	{
		CBIOS_U8	PCI_SUBSYS_Vendor_ID	:8;
	};
	CBIOS_U8	Value;
}REG_CR84_C;


typedef union _REG_CR85_C	//PCI_Subsystem_Vendor_ID_Shadow_High_Register
{
	struct
	{
		CBIOS_U8	PCI_SUBSYS_Vendor_ID	:8;
	};
	CBIOS_U8	Value;
}REG_CR85_C;


typedef union _REG_CR86_C	//PCI_Subsystem_ID_Shadow_Low_Register
{
	struct
	{
		CBIOS_U8	PCI_SUBSYS_ID	:8;
	};
	CBIOS_U8	Value;
}REG_CR86_C;


typedef union _REG_CR87_C	//PCI_Subsystem_ID_Shadow_High_Register
{
	struct
	{
		CBIOS_U8	PCI_SUBSYS_ID	:8;
	};
	CBIOS_U8	Value;
}REG_CR87_C;


typedef union _REG_CR88_C	//Subsystem_ID_Register
{
	struct
	{
		CBIOS_U8	SSID_Func0	:4;
		CBIOS_U8	SSID_Func1	:4;
	};
	CBIOS_U8	Value;
}REG_CR88_C;


typedef union _REG_CR8A_C	//MMIO_Address_Window_Position_Register
{
	struct
	{
		CBIOS_U8	Reserved	:3;
		CBIOS_U8	MMIO_Address_Window_position	:5;
	};
	CBIOS_U8	Value;
}REG_CR8A_C;


typedef union _REG_CR8B_C	//MMIO_Address_Window_Position_Register_2
{
	struct
	{
		CBIOS_U8	MMIO_Address_Window_position	:8;
	};
	CBIOS_U8	Value;
}REG_CR8B_C;


typedef union _REG_CR90_C	//Linear_Address_Window_Position_1_Register
{
	struct
	{
		CBIOS_U8	Reserved	:2;
		CBIOS_U8	La_Window_Position_Bit28to26	:3;
		CBIOS_U8	LA_Window_position_31to29	:3;
	};
	CBIOS_U8	Value;
}REG_CR90_C;


typedef union _REG_CR91_C	//Linear_Address_Window_Position_2_Register
{
	struct
	{
		CBIOS_U8	LA_Window_position	:8;
	};
	CBIOS_U8	Value;
}REG_CR91_C;


typedef union _REG_CR92_C	//Linear_Address_Window_Position_3_Register
{
	struct
	{
		CBIOS_U8	LA_Window_position	:8;
	};
	CBIOS_U8	Value;
}REG_CR92_C;


typedef union _REG_CR93_C	//Linear_Address_Window_Position_4_Register
{
	struct
	{
		CBIOS_U8	LA_Window_position	:8;
	};
	CBIOS_U8	Value;
}REG_CR93_C;


typedef union _REG_CR94_C	//Linear_Address_Window_Position_5_Register
{
	struct
	{
		CBIOS_U8	LA_Window_position	:8;
	};
	CBIOS_U8	Value;
}REG_CR94_C;


typedef union _REG_CR98_C	//PCI_Function1_Subsystem_Vendor_ID_Low_Shadow_Register
{
	struct
	{
		CBIOS_U8	SSVID_Low_Shadow_Func1	:8;
	};
	CBIOS_U8	Value;
}REG_CR98_C;


typedef union _REG_CR99_C	//PCI_Function1_Subsystem_Vendor_ID_High_Shadow_Register
{
	struct
	{
		CBIOS_U8	SSVID_High_Shadow_Func1	:8;
	};
	CBIOS_U8	Value;
}REG_CR99_C;


typedef union _REG_CR9A_C	//PCI_Function1_Subsystem_ID_Low_Shadow_Register
{
	struct
	{
		CBIOS_U8	SSID_Low_Shadow_Func1	:8;
	};
	CBIOS_U8	Value;
}REG_CR9A_C;


typedef union _REG_CR9B_C	//PCI_Function1_Subsystem_ID_High_Shadow_Register
{
	struct
	{
		CBIOS_U8	SSID_High_Shadow_Func1	:8;
	};
	CBIOS_U8	Value;
}REG_CR9B_C;


typedef union _REG_CR9C_C	//Error_Status_Cross_Check_Disable_Register
{
	struct
	{
		CBIOS_U8	ERR_STATUS_CROSS_CHECK_DIS	:1;
		CBIOS_U8	Reserved_bits7to1	:7;
	};
	CBIOS_U8	Value;
}REG_CR9C_C;


typedef union _REG_CR9D_C	//BIU_Internal_Test_Register_1
{
	struct
	{
		CBIOS_U8	PACK_Enable	:1;
		CBIOS_U8	Reserved_0	:2;
		CBIOS_U8	MMIO_read_async_enable	:1;
		CBIOS_U8	Reserved_1	:1;
		CBIOS_U8	Reserved_2	:3;
	};
	CBIOS_U8	Value;
}REG_CR9D_C;


typedef union _REG_CR9E_C	//BIU_Internal_Test_Register_2
{
	struct
	{
		CBIOS_U8	_5_Bit_PACK_timer	:5;
		CBIOS_U8	Reserved	:3;
	};
	CBIOS_U8	Value;
}REG_CR9E_C;


typedef union _REG_CRA0_C	//BIU_Control_1_Register
{
	struct
	{
		CBIOS_U8	Interrupt_Enable	:1;
		CBIOS_U8	MMIO_Enable	:1;
		CBIOS_U8	Old_MMIO_Window	:1;
		CBIOS_U8	VGA_Memory_access_disable	:1;
		CBIOS_U8	Enable_Linear_Addressing	:1;
		CBIOS_U8	Reserved	:3;
	};
	CBIOS_U8	Value;
}REG_CRA0_C;


typedef union _REG_CRA1_C	//BIU_Shadow_Update_Control_Register
{
	struct
	{
		CBIOS_U8	REG_OUTFB_UREN	:1;
		CBIOS_U8	Reserved	:3;
		CBIOS_U8	Shadow_Update	:1;
		CBIOS_U8	REG_FBSIZE_EXIST	:3;
	};
	CBIOS_U8	Value;
}REG_CRA1_C;


typedef union _REG_CRA2_C	//Software_Reset_0_Register
{
	struct
	{
		CBIOS_U8	PCI_interrupt	:1;
		CBIOS_U8	Reserved	:7;
	};
	CBIOS_U8	Value;
}REG_CRA2_C;


typedef union _REG_CRA3_C	//Software_Reset_1_Register
{
	struct
	{
		CBIOS_U8	Reserved_0	:1;
		CBIOS_U8	Reserved_1	:1;
		CBIOS_U8	Reserved_2	:1;
		CBIOS_U8	Reserved_3	:1;
		CBIOS_U8	Reserved_4	:1;
		CBIOS_U8	Reserved_5	:1;
		CBIOS_U8	TV_Unit_Software_Reset	:1;
		CBIOS_U8	Chip_reset_except_PCI_config_space	:1;
	};
	CBIOS_U8	Value;
}REG_CRA3_C;


typedef union _REG_CRA4_C	//Software_Reset_2_Register
{
	struct
	{
		CBIOS_U8	Reserved_0	:5;
		CBIOS_U8	MDI_software_reset	:1;
		CBIOS_U8	Reserved_1	:1;
		CBIOS_U8	_3D_Engine_Software_Reset	:1;
	};
	CBIOS_U8	Value;
}REG_CRA4_C;


typedef union _REG_CRA5_C	//Software_Reset_3_Register
{
	struct
	{
		CBIOS_U8	Reserved_0	:4;
		CBIOS_U8	Reserved_1	:1;
		CBIOS_U8	Reserved_2	:1;
		CBIOS_U8	Reserved_3	:1;
		CBIOS_U8	SLV_Software_Reset	:1;
	};
	CBIOS_U8	Value;
}REG_CRA5_C;


typedef union _REG_CRA6_C	//Software_Reset_4_Register
{
	struct
	{
		CBIOS_U8	TLM_Software_Reset	:1;
		CBIOS_U8	Reserved_0	:1;
		CBIOS_U8	Reserved_1	:1;
		CBIOS_U8	Reserved_2	:1;
		CBIOS_U8	TLR_Software_Reset	:1;
		CBIOS_U8	Reserved_3	:1;
		CBIOS_U8	PMC_Software_Reset	:1;
		CBIOS_U8	MSG_Software_Reset	:1;
	};
	CBIOS_U8	Value;
}REG_CRA6_C;


typedef union _REG_CRA8_C	//BIU_Power_Management_1_Register
{
	struct
	{
		CBIOS_U8	Reserved_0	:1;
		CBIOS_U8	Reserved_1	:1;
		CBIOS_U8	TLR_Power_Management_On	:1;
		CBIOS_U8	BIU_Power_Management_On	:1;
		CBIOS_U8	Reserved_2	:1;
		CBIOS_U8	Wait_until_CPL_back_then_enter_L1_or_L3	:1;
		CBIOS_U8	PCI_Memory_IO_cycle_PM_Disable	:1;
		CBIOS_U8	Chip_Power_Management	:1;
	};
	CBIOS_U8	Value;
}REG_CRA8_C;


typedef union _REG_CRA9_C	//BIU_Power_Management_2_Register
{
	struct
	{
		CBIOS_U8	when_receiving_PME_TURN_OFF	:1;
		CBIOS_U8	REG_PMC_BLKTLP_EN	:1;
		CBIOS_U8	REG_ASPML1_ONLY	:1;
		CBIOS_U8	Reserved	:5;
	};
	CBIOS_U8	Value;
}REG_CRA9_C;


typedef union _REG_CRAA_C	//BIU_Power_Management_3_Register
{
	struct
	{
		CBIOS_U8	receiving_PME_TURN_OFF	:8;
	};
	CBIOS_U8	Value;
}REG_CRAA_C;


typedef union _REG_CRAB_C	//BIU_Reset_Mode_Register
{
	struct
	{
		CBIOS_U8	BIU_RESET_MODE	:1;
		CBIOS_U8    BIU_ECLK_RESET_MODE :1;
		CBIOS_U8	Reserved	:6;
	};
	CBIOS_U8	Value;
}REG_CRAB_C;


typedef union _REG_CRAC_C	//BIU-MXU_Request_Priority_Assertion_Register
{
	struct
	{
		CBIOS_U8	Assert_BIU_MXU_Request	:1;
		CBIOS_U8	Reserved	:7;
	};
	CBIOS_U8	Value;
}REG_CRAC_C;


typedef union _REG_CRAD_C	//BIU_CSP_PW_REGISTER
{
	struct
	{
		CBIOS_U8	BIU_CSP_PW_EN	:8;
	};
	CBIOS_U8	Value;
}REG_CRAD_C;


typedef union _REG_CRAE_C	//BIU_BACKDOOR_ENABLE_REGISTER
{
	struct
	{
		CBIOS_U8	backdoor_enable	:1;
		CBIOS_U8	c4p_enable	:1;
		CBIOS_U8	Reserved	:6;
	};
	CBIOS_U8	Value;
}REG_CRAE_C;


typedef union _REG_CRAF_C	//C4P_timer
{
	struct
	{
		CBIOS_U8	C4P_timer_7_0	:8;
	};
	CBIOS_U8	Value;
}REG_CRAF_C;


typedef union _REG_CRB0_C	//BIU_BACKDOOR__REGISTER1
{
	struct
	{
		CBIOS_U8	Backdoor_Vendor_ID_7_0	:8;
	};
	CBIOS_U8	Value;
}REG_CRB0_C;


typedef union _REG_CRB1_C	//BIU_BACKDOOR__REGISTER2
{
	struct
	{
		CBIOS_U8	Backdoor_Vendor_ID_15_8	:8;
	};
	CBIOS_U8	Value;
}REG_CRB1_C;


typedef union _REG_CRB2_C	//BIU_BACKDOOR__REGISTER3
{
	struct
	{
		CBIOS_U8	Backdoor_Device_ID_7_0	:8;
	};
	CBIOS_U8	Value;
}REG_CRB2_C;


typedef union _REG_CRB3_C	//BIU_BACKDOOR__REGISTER4
{
	struct
	{
		CBIOS_U8	Backdoor_Device_ID_15_8	:8;
	};
	CBIOS_U8	Value;
}REG_CRB3_C;


typedef union _REG_CRB4_C	//Software_Reset_5_Register
{
	struct
	{
		CBIOS_U8	CSP_Software_Reset	:1;
		CBIOS_U8	CNM_software_Reset	:1;
		CBIOS_U8	S3VD_Software_Reset	:1;
		CBIOS_U8	S0TU_Software_Reset	:1;
		CBIOS_U8	S0EUC_Software_Reset	:1;
		CBIOS_U8	MXU_Software_Reset	:1;
		CBIOS_U8	S0FFU_Software_Reset	:1;
		CBIOS_U8	SLRZ1_Software_Reset	:1;
	};
	CBIOS_U8	Value;
}REG_CRB4_C;


typedef union _REG_CRB5_C	//Software_Reset_6_Register
{
	struct
	{
		CBIOS_U8	S0PECTL_Software_Reset	:1;
		CBIOS_U8	S0PEALU_software_Reset	:1;
		CBIOS_U8	Reserved	:6;
	};
	CBIOS_U8	Value;
}REG_CRB5_C;


typedef union _REG_CRC0_C	//Process_Monitor_Counter_[7:0]_Register
{
	struct
	{
		CBIOS_U8	Process_Monitor_Counter	:8;
	};
	CBIOS_U8	Value;
}REG_CRC0_C;


typedef union _REG_CRC1_C	//Process_Monitor_Counter_[15:8]_Register
{
	struct
	{
		CBIOS_U8	Process_Monitor_Counter	:8;
	};
	CBIOS_U8	Value;
}REG_CRC1_C;


typedef union _REG_CRC2_C	//Process_Monitor_Counter_Control_Register
{
	struct
	{
		CBIOS_U8	Reserved_0	:1;
		CBIOS_U8	Process_Monitor_Counter_Enable	:1;
		CBIOS_U8	Sample_clock_Selection	:1;
		CBIOS_U8	clock_Division_Select	:3;
		CBIOS_U8	Reserved_1	:2;
	};
	CBIOS_U8	Value;
}REG_CRC2_C;


typedef union _REG_CRC3_C	//Preset_Engine_Idle_7:4_Register
{
	struct
	{
		CBIOS_U8	Reserved	:4;
		CBIOS_U8	Preset_Engine_Idle_Cntr_7to4	:4;
	};
	CBIOS_U8	Value;
}REG_CRC3_C;


typedef union _REG_CRC4_C	//Preset_Engine_Idle_15:8_Register
{
	struct
	{
		CBIOS_U8	Preset_Engine_Idle_Cntr	:8;
	};
	CBIOS_U8	Value;
}REG_CRC4_C;


typedef union _REG_CRC6_C	//Read_Back_Current_Engine_Idle_Counter_[7:0]_Register
{
	struct
	{
		CBIOS_U8	Current_Engine_Idle_Cntr	:8;
	};
	CBIOS_U8	Value;
}REG_CRC6_C;


typedef union _REG_CRC7_C	//Read_Back_Current_Engine_Idle_Counter_[15:8]_Register
{
	struct
	{
		CBIOS_U8	Current_Engine_Idle_Cntr	:8;
	};
	CBIOS_U8	Value;
}REG_CRC7_C;


typedef union _REG_CRC9_C	//Preset_Chip_Idle_7:4_Register
{
	struct
	{
		CBIOS_U8	Reserved	:4;
		CBIOS_U8	Preset_Chip_Idle_Cntr	:4;
	};
	CBIOS_U8	Value;
}REG_CRC9_C;


typedef union _REG_CRCA_C	//Preset_Chip_Idle_15:8_Register
{
	struct
	{
		CBIOS_U8	Preset_Chip_Idle_Cntr	:8;
	};
	CBIOS_U8	Value;
}REG_CRCA_C;


typedef union _REG_CRD7_C	//Swizzle_Control_Register
{
	struct
	{
		CBIOS_U8	swizzle_disable	:1;
		CBIOS_U8	Reserved_0	:1;
		CBIOS_U8	Reserved_1	:1;
		CBIOS_U8	Reserved_2	:1;
		CBIOS_U8	Reserved_bit_4	:1;
		CBIOS_U8	Reserved_7to5	:3;
	};
	CBIOS_U8	Value;
}REG_CRD7_C;


typedef union _REG_CRDD_C	//Role-Based_Error_Report_Enable_Register
{
	struct
	{
		CBIOS_U8	RBE_EN	:1;
		CBIOS_U8	Reserved	:7;
	};
	CBIOS_U8	Value;
}REG_CRDD_C;


typedef union _REG_CREF_C	//Physical_Layer_Debug_2_Register
{
	struct
	{
		CBIOS_U8	Reserved_0	:2;
		CBIOS_U8	Reserved_1	:5;
		CBIOS_U8	DIU_BIU_PLL_LOCK	:1;
	};
	CBIOS_U8	Value;
}REG_CREF_C;


typedef union _REG_CRF0_C	//Power_On_Strapping_1_Register
{
	struct
	{
		CBIOS_U8	Hdad_Enable	:1;
		CBIOS_U8	Pm_D1d2_Dis	:1;
		CBIOS_U8	Reserved	:6;
	};
	CBIOS_U8	Value;
}REG_CRF0_C;


typedef union _REG_CRF2_C	//BIU_DBG_Software_Reset_Mask_Register
{
	struct
	{
		CBIOS_U8	REG_MASK_RST	:1;
		CBIOS_U8	RESERVED	:7;
	};
	CBIOS_U8	Value;
}REG_CRF2_C;

typedef union _REG_CRF5_C	//BIU_DBG_Software_Reset_Mask_Register
{
	struct
	{
		CBIOS_U8	EPLL_HW_LOAD    :1;
		CBIOS_U8	IPLL_HW_LOAD	:1;
		CBIOS_U8	VPLL_HW_LOAD	:1;
		CBIOS_U8	RESERVED        :5;
	};
	CBIOS_U8	Value;
}REG_CRF5_C;


