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


typedef union _REG_MM8508	//Command_FIFO_Control_Register
{
    struct
    {
        CBIOS_U32    VSYNC1_INT_EN                     :1;
        CBIOS_U32    ENGINE_BUSY_INT_EN                :1;
        CBIOS_U32    VSYNC2_INT_EN                     :1;
        CBIOS_U32    VSYNC3_INT_EN                     :1;
        CBIOS_U32    VSYNC4_INT_EN                     :1;
        CBIOS_U32    PAGE_FAULT_INT_EN                 :1;
        CBIOS_U32    DIU_BIU_CEC_INT_EN                :1;
        CBIOS_U32    DP1_INT_EN                        :1;
        CBIOS_U32    DP2_INT_EN                        :1;
        CBIOS_U32    INT_TLPMATCH_INT_EN               :1;
        CBIOS_U32    Reserved_1                        :1;
        CBIOS_U32    CORRECTABLE_ERR_INT_EN            :1;
        CBIOS_U32    NON_FATAL_ERR_INT_EN              :1;
        CBIOS_U32    FATAL_ERR_INT_EN                  :1;
        CBIOS_U32    UNSUPPORTED_ERR_INT_EN            :1;
        CBIOS_U32    INVALID_CPURW_INT_EN              :1;
        CBIOS_U32    DP3_INT_EN                        :1;
        CBIOS_U32    DP4_INT_EN                        :1;
        CBIOS_U32    INT_TS_INT_EN                     :1;
        CBIOS_U32    INT_FS0_INT_INT_EN                :1;
        CBIOS_U32    INT_GPIO_EXT_IRQ_X86_INT_EN       :1;
        CBIOS_U32    INT_MB_INT_EN                     :1;
        CBIOS_U32    HDCP_INT_EN                       :1;
        CBIOS_U32    CHIP_IDLE_CNTR_EXPIRE_INT_EN      :1;
        CBIOS_U32    INT_VIP0_EOF_INT_EN               :1;
        CBIOS_U32    VBI_CLEAR_INT_EN                  :1;
        CBIOS_U32    INT_VIP_INT_EN                    :1;
        CBIOS_U32    INT_VIP1_EOF_INT_EN               :1;
        CBIOS_U32    INT_VIP2_EOF_INT_EN               :1;
        CBIOS_U32    HDA_AUDIO_INT_EN                  :1;
        CBIOS_U32    INT_VIP3_EOF_INT_EN               :1;
        CBIOS_U32    INT_FS1_EOF_INT_EN                :1;
    };
    CBIOS_U32    Value;
}REG_MM8508;

typedef union _REG_MM8510	//Transaction_Layer/Data_Link_Layer_Status_0_Register
{
	struct
	{
		CBIOS_U32	Reserved_7to0	:8;
		CBIOS_U32	TLR_MEMW_BUSY	:1;
		CBIOS_U32	FB_FIFO_Full	:1;
		CBIOS_U32	FB_FIFO_Empty	:1;
		CBIOS_U32	BIU_MW_FIFO_Empty	:1;
		CBIOS_U32	TLR_RDFF_EMPTY	:1;
		CBIOS_U32	TLR_PHQ_EMPTY	:1;
		CBIOS_U32	TLR_NPHQ_EMPTY	:1;
		CBIOS_U32	TLR_FMTFF_FULL	:1;
		CBIOS_U32	TLM_BusY	:1;
		CBIOS_U32	Reserved_0	:1;
		CBIOS_U32	TLR_BusY	:1;
		CBIOS_U32	Reserved_1	:1;
		CBIOS_U32	Reserved_2	:1;
		CBIOS_U32	Reserved_3	:1;
		CBIOS_U32	Reserved_4	:1;
		CBIOS_U32	CFAIFX_BusY	:1;
		CBIOS_U32	Reserved_5	:1;
		CBIOS_U32	Reserved_6	:1;
		CBIOS_U32	Reserved_7	:6;
	};
	CBIOS_U32	Value;
}REG_MM8510;


typedef union _REG_MM8514	//Transaction_Layer/Data_Link_Layer_Status_1_Register
{
	struct
	{
		CBIOS_U32	PECPL_OSCNT	:7;
		CBIOS_U32	Reserved_0	:1;
		CBIOS_U32	TRANSACTION_OSCNT	:6;
		CBIOS_U32	Reserved_1	:2;
		CBIOS_U32	Reserved_2	:6;
		CBIOS_U32	Reserved_3	:2;
		CBIOS_U32	Reserved_4	:4;
		CBIOS_U32	Reserved_5	:4;
	};
	CBIOS_U32	Value;
}REG_MM8514;


typedef union _REG_MM8524	//CRAx_BIU_Shadow_Software_RESET_Register
{
	struct
	{
		CBIOS_U32	CRA3_C_Software_Reset_register	:8;
		CBIOS_U32	CRA4_C_Software_Reset_register	:8;
		CBIOS_U32	CRA5_C_Software_Reset_register	:8;
		CBIOS_U32	CRA6_C_Software_Reset_register	:8;
	};
	CBIOS_U32	Value;
}REG_MM8524;


typedef union _REG_MM8528	//GTS_Counter_Low_Register
{
	struct
	{
		CBIOS_U32	GTS_low_32_bits	:32;
	};
	CBIOS_U32	Value;
}REG_MM8528;


typedef union _REG_MM852C	//GTS_Counter_High_Register
{
	struct
	{
		CBIOS_U32	GTS_High_32_bits	:32;
	};
	CBIOS_U32	Value;
}REG_MM852C;


typedef union _REG_MM8530	//GTS_Address_0_Register
{
	struct
	{
		CBIOS_U32	GTS_ADDRESS_low	:32;
	};
	CBIOS_U32	Value;
}REG_MM8530;


typedef union _REG_MM8534	//GTS_Address_1_Register
{
	struct
	{
		CBIOS_U32	GTS_ADDRESS_high	:32;
	};
	CBIOS_U32	Value;
}REG_MM8534;


typedef union _REG_MM8538	//Reserved
{
	struct
	{
		CBIOS_U32	Reserved	:32;
	};
	CBIOS_U32	Value;
}REG_MM8538;


typedef union _REG_MM853C	//Stop/Trap_CSP_Control_Register
{
	struct
	{
		CBIOS_U32	issue_Stop_to_CSP	:1;
		CBIOS_U32	Trap_clear	:1;
		CBIOS_U32	Reserved_31to2	:30;
	};
	CBIOS_U32	Value;
}REG_MM853C;


typedef union _REG_MM8540	//TLM_Control_Register
{
	struct
	{
		CBIOS_U32	Reserved12_0	:13;
		CBIOS_U32	REG_INT_USS_EN	:1;
		CBIOS_U32	REG_MULTI_MSI_EN	:1;
		CBIOS_U32	REG_HDA_HI_PRI_EN	:1;
		CBIOS_U32	Reserved_27_16	:12;
		CBIOS_U32	Reserved	:4;
	};
	CBIOS_U32	Value;
}REG_MM8540;


typedef union _REG_MM8544	//Apeture_Table_(APT)_Base_Address_Register
{
	struct
	{
		CBIOS_U32	Reserved_11to0	:12;
		CBIOS_U32	APT_Base_Addr	:18;
		CBIOS_U32	Reserved_31to30	:2;
	};
	CBIOS_U32	Value;
}REG_MM8544;


typedef union _REG_MM8548	//CSP_Interrupt_Status_Register
{
	struct
	{
		CBIOS_U32	VD0_INT0          :1;
		CBIOS_U32	VD0_INT1          :1;
		CBIOS_U32	VD0_INT2          :1;
		CBIOS_U32	VD0_INT3          :1;
		CBIOS_U32	VD1_INT0          :1;
		CBIOS_U32	VD1_INT1          :1;
		CBIOS_U32	VD1_INT2          :1;
		CBIOS_U32	VD1_INT3          :1;
		CBIOS_U32	Reserved          :8;
		CBIOS_U32	CSP_TIMEOUT_INT   :1;
		CBIOS_U32	SP_TIMEOUT_INT    :1;
		CBIOS_U32	VPP_TIMEOUT_INT   :1;
		CBIOS_U32	PECTL_TIMEOUT_INT :1;
		CBIOS_U32	MSVD_TIMEOUT_INT  :1;
		CBIOS_U32	MXU_TIMEOUT_INT   :1;
		CBIOS_U32	MIU0_TIMEOUT_INT  :1;
		CBIOS_U32	MIU1_TIMEOUT_INT  :1;
		CBIOS_U32	MIU2_TIMEOUT_INT  :1;
		CBIOS_U32	PMP_TIMEOUT_INT   :1;
		CBIOS_U32	VCP_TIMEOUT_INT   :1;
		CBIOS_U32	FENCE_CMD_INT     :1;
		CBIOS_U32	VIRQ              :1;
		CBIOS_U32	Reserved1         :3;
	};
	CBIOS_U32	Value;
}REG_MM8548;


typedef union _REG_MM854C	//CSP_Interrupt_Enable_Register
{
	struct
	{
		CBIOS_U32	VD0_INT0_EN          :1;
		CBIOS_U32	VD0_INT1_EN          :1;
		CBIOS_U32	VD0_INT2_EN          :1;
		CBIOS_U32	VD0_INT3_EN	         :1;
		CBIOS_U32	VD1_INT0_EN          :1;
		CBIOS_U32	VD1_INT1_EN	         :1;
		CBIOS_U32	VD1_INT2_EN	         :1;
		CBIOS_U32	VD1_INT3_EN          :1;
		CBIOS_U32	Reserved             :8;
		CBIOS_U32	CSP_TIMEOUT_INT_EN   :1;
		CBIOS_U32	SP_TIMEOUT_INT_EN    :1;
		CBIOS_U32	VPP_TIMEOUT_INT_EN   :1;
		CBIOS_U32	PECTL_TIMEOUT_INT_EN :1;
		CBIOS_U32	MSVD_TIMEOUT_INT_EN  :1;
		CBIOS_U32	MXU_TIMEOUT_INT_EN   :1;
		CBIOS_U32	MIU0_TIMEOUT_INT_EN	 :1;
		CBIOS_U32	MIU1_TIMEOUT_INT_EN  :1;
		CBIOS_U32	MIU2_TIMEOUT_INT_EN  :1;
		CBIOS_U32	PMP_TIMEOUT_INT_EN   :1;
		CBIOS_U32	VCP_TIMEOUT_INT_EN   :1;
		CBIOS_U32	FENCE_CMD_INT_EN     :1;
		CBIOS_U32	Reserved1            :4;
	};
	CBIOS_U32	Value;
}REG_MM854C;

typedef union _REG_MM8574	//Interrupt_Synchronization_MM8504_Mirror_Register
{
    struct
    {
        CBIOS_U32	VSYNC1_INT                     :1;
        CBIOS_U32	ENGINE_BUSY_INT                :1;
        CBIOS_U32	VSYNC2_INT                     :1;
        CBIOS_U32	VSYNC3_INT                     :1;
        CBIOS_U32	VSYNC4_INT                     :1;
        CBIOS_U32	PAGE_FAULT_INT                 :1;
        CBIOS_U32	DIU_BIU_CEC_INT                :1;
        CBIOS_U32	DP1_INT                        :1;
        CBIOS_U32	DP2_INT                        :1;
        CBIOS_U32	TLPMATCH_INT                   :1;
        CBIOS_U32	MST_ABORT                      :1;
        CBIOS_U32	CORRECTABLE_ERR_INT            :1;
        CBIOS_U32	NON_FATAL_ERR_INT              :1;
        CBIOS_U32	FATAL_ERR_INT                  :1;
        CBIOS_U32	UNSUPPORTED_ERR_INT            :1;
        CBIOS_U32	INVALID_CPURW_INT              :1;
        CBIOS_U32	DP3_INT                        :1;
        CBIOS_U32	DP4_INT                        :1;
        CBIOS_U32	TS_INT                         :1;
        CBIOS_U32	FS0_INT                        :1;
        CBIOS_U32	GPIO_EXT_IRQ_INT               :1;
        CBIOS_U32	MB_INT                         :1;
        CBIOS_U32	HDCP_INT                       :1;
        CBIOS_U32	CHIP_IDLE_CNTR_EXPIRE_INT      :1;
        CBIOS_U32	VIP0_EOF_INT                   :1;
        CBIOS_U32	VBI_CLEAR_INT                  :1;
        CBIOS_U32	HDA_CODEC_INT                  :1;
        CBIOS_U32	VIP1_EOF                       :1;
        CBIOS_U32	VIP2_EOF                       :1;
        CBIOS_U32	HDA_AUDIO_INT                  :1;
        CBIOS_U32	VIP3_EOF                       :1;
        CBIOS_U32	FS1_INT                        :1;
    };
	CBIOS_U32	Value;
}REG_MM8574;


typedef union _REG_MM8578	//Interrupt_Synchronization_MM8548_Mirror_Register
{
	struct
	{
		CBIOS_U32	VD0_INT0          :1;
		CBIOS_U32	VD0_INT1          :1;
		CBIOS_U32	VD0_INT2          :1;
		CBIOS_U32	VD0_INT3          :1;
		CBIOS_U32	VD1_INT0          :1;
		CBIOS_U32	VD1_INT1          :1;
		CBIOS_U32	VD1_INT2          :1;
		CBIOS_U32	VD1_INT3          :1;
		CBIOS_U32	Reserved          :8;
		CBIOS_U32	CSP_TIMEOUT_INT   :1;
		CBIOS_U32	SP_TIMEOUT_INT    :1;
		CBIOS_U32	VPP_TIMEOUT_INT   :1;
		CBIOS_U32	PECTL_TIMEOUT_INT :1;
		CBIOS_U32	MSVD_TIMEOUT_INT  :1;
		CBIOS_U32	MXU_TIMEOUT_INT   :1;
		CBIOS_U32	MIU0_TIMEOUT_INT  :1;
		CBIOS_U32	MIU1_TIMEOUT_INT  :1;
		CBIOS_U32	MIU2_TIMEOUT_INT  :1;
		CBIOS_U32	PMP_TIMEOUT_INT   :1;
		CBIOS_U32	VCP_TIMEOUT_INT   :1;
		CBIOS_U32	FENCE_CMD_INT     :1;
		CBIOS_U32	VIRQ              :1;
		CBIOS_U32	Reserved1         :3;
	};
	CBIOS_U32	Value;
}REG_MM8578;


typedef union _REG_MM85B4	//CRBx_C_Shadow_Software_RESET_Register
{
	struct
	{
		CBIOS_U32	CRB4_C_Software_Reset_register	:8;
		CBIOS_U32	CRB5_C_Software_Reset_register	:8;
		CBIOS_U32	CRB6_C_Software_Reset_register	:8;
		CBIOS_U32	CRB7_C_Software_Reset_register	:8;
	};
	CBIOS_U32	Value;
}REG_MM85B4;


typedef union _REG_MM85D0    //EPLL_control_Register
{
    struct
    {
        CBIOS_U32    EPLL_integer_division_setting    :8;
        CBIOS_U32    EPLL_fractional_division_setting    :20;
        CBIOS_U32    EPLL_Clock1_output_division_ratio    :2;
        CBIOS_U32    EPLL_VCO_frequency_range_setting    :1;
        CBIOS_U32    EPLL_enable_Clock1_output    :1;
    };
    CBIOS_U32    Value;
}REG_MM85D0;


typedef union _REG_MM85D4    //EPLL_control_Register
{
    struct
    {
        CBIOS_U32    EPLL_Power_up_control    :1;
        CBIOS_U32    EPLL_Reset_control    :1;
        CBIOS_U32    Reserved    :30;
    };
    CBIOS_U32    Value;
}REG_MM85D4;


typedef union _REG_MM85D8    //EPLL_control_Register
{
    struct
    {
        CBIOS_U32    EPLL_Charge_Pump_current_selection_setting    :3;
        CBIOS_U32    EPLL_Fractional_function_enable    :1;
        CBIOS_U32    EPLL_Enable_Clock2_output    :1;
        CBIOS_U32    EPLL_Clock2_output_division_ratio    :2;
        CBIOS_U32    Reserved    :25;
    };
    CBIOS_U32    Value;
}REG_MM85D8;


typedef union _REG_MM85E0    //IPLL_control_Register
{
    struct
    {
        CBIOS_U32    IPLL_integer_division_setting    :8;
        CBIOS_U32    IPLL_fractional_division_setting    :20;
        CBIOS_U32    IPLL_Clock1_output_division_ratio    :2;
        CBIOS_U32    IPLL_VCO_frequency_range_setting    :1;
        CBIOS_U32    IPLL_enable_Clock1_output    :1;
    };
    CBIOS_U32    Value;
}REG_MM85E0;


typedef union _REG_MM85E4    //IPLL_control_Register
{
    struct
    {
        CBIOS_U32    IPLL_Power_up_control    :1;
        CBIOS_U32    IPLL_Reset_control    :1;
        CBIOS_U32    Reserved    :30;
    };
    CBIOS_U32    Value;
}REG_MM85E4;


typedef union _REG_MM85E8    //IPLL_control_Register
{
    struct
    {
        CBIOS_U32    IPLL_Charge_Pump_current_selection_setting    :3;
        CBIOS_U32    IPLL_Fractional_function_enable    :1;
        CBIOS_U32    IPLL_Enable_Clock2_output    :1;
        CBIOS_U32    IPLL_Clock2_output_division_ratio    :2;
        CBIOS_U32    Reserved    :25;
    };
    CBIOS_U32    Value;
}REG_MM85E8;


typedef union _REG_MM85F0    //VPLL_control_Register
{
    struct
    {
        CBIOS_U32    VPLL_integer_division_setting    :8;
        CBIOS_U32    VPLL_fractional_division_setting    :20;
        CBIOS_U32    VPLL_Clock1_output_division_ratio    :2;
        CBIOS_U32    VPLL_VCO_frequency_range_setting    :1;
        CBIOS_U32    VPLL_enable_Clock1_output    :1;
    };
    CBIOS_U32    Value;
}REG_MM85F0;


typedef union _REG_MM85F4    //VPLL_control_Register
{
    struct
    {
        CBIOS_U32    VPLL_Power_up_control    :1;
        CBIOS_U32    VPLL_Reset_control    :1;
        CBIOS_U32    Reserved    :30;
    };
    CBIOS_U32    Value;
}REG_MM85F4;


typedef union _REG_MM85F8    //VPLL_control_Register
{
    struct
    {
        CBIOS_U32    VPLL_Charge_Pump_current_selection_setting    :3;
        CBIOS_U32    VPLL_Fractional_function_enable    :1;
        CBIOS_U32    VPLL_Enable_Clock2_output    :1;
        CBIOS_U32    VPLL_Clock2_output_division_ratio    :2;
        CBIOS_U32    Reserved    :25;
    };
    CBIOS_U32    Value;
}REG_MM85F8;


