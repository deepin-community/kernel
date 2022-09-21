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


/*****************************************************************************
** DESCRIPTION:
** DSI port interface function prototype and parameter definition.
**
** NOTE:
** 
******************************************************************************/

#ifndef _CBIOS_DSI_H_
#define _CBIOS_DSI_H_

#include "../Monitor/DSIPanel/CBiosDSIPanel.h"

#define CBIOS_DSI_VERSION        0x01
#define CBIOS_DSI_PANEL_TAG      0x445349    // ASCII code of 'DSI'
#define CBIOS_DSI_CMD_FIFO_DEPTH 63
#define CBIOS_DSI_POLLING_TIMEOUT 1000   // ms
#define CBIOS_DSI_MAX_RETURN_PACKET_SIZE 16    // byte

#define khz2ps(khz) (1000000000 /(khz))

#define ceil(x, y) (((unsigned long)(x) + (unsigned long)(y) - 1) / (y))

// these cmd types are defined by hw design, not DSI spec
#define DSI_CMD_TYPE_DMA              (1 << 0)    // 1: DMA cmd, 0: non DMA cmd
#define DSI_CMD_TYPE_LAST_GROUP       (1 << 1)    // 1: last cmd of a transaction group, 0: is not
#define DSI_CMD_TYPE_NEED_ACK         (1 << 2)    // 1: the cmd need acknowledge, 0: no need
#define DSI_CMD_TYPE_FOLLOW_INTERV    (1 << 3)    // 1: next cmd should not be transferred until a configured interval has passed
#define DSI_CMD_TYPE_LP               (1 << 4)    // 1: the cmd can be transferred in LP mode, 0: the cmd can only be transferred in HS mode
#define DSI_CMD_TYPE_TE               (1 << 5)    // 1: the cmd is a TE BTA, 0: is not
#define DSI_CMD_TYPE_VIDEO            (1 << 6)    // 1: video cmd, 0: non video cmd

#define DSI_TRIGGER_MSG_ACK           0x21
#define DSI_TRIGGER_MSG_RESET         0x62
#define DSI_TRIGGER_MSG_TE            0x5D

// the interrupt status defined in register MM3438
#define DSI_IRQ_DSI_READBACK          (1 << 2)
#define DSI_IRQ_EPHY_LPP_HFAULT       (1 << 3)
#define DSI_IRQ_EPHY_LPN_HFAULT       (1 << 4)
#define DSI_IRQ_EPHY_LPP_LFAULT       (1 << 5)
#define DSI_IRQ_EPHY_LPN_LFAULT       (1 << 6)
#define DSI_IRQ_LPRX_TIMEOUT          (1 << 7)
#define DSI_IRQ_HSTX_TIMEOUT          (1 << 8)
#define DSI_IRQ_BTA_TIMEOUT           (1 << 9)
#define DSI_IRQ_LPDATA_SYNC_ERR       (1 << 10)
#define DSI_IRQ_INCRRECT_LINE_STATE   (1 << 11)
#define DSI_IRQ_ULPS_TRIGGER          (1 << 12)
#define DSI_IRQ_RESET_TRIGGER         (1 << 13)
#define DSI_IRQ_TE_TRIGGER            (1 << 14)
#define DSI_IRQ_OTHER_TRIGGER         (1 << 15)
#define DSI_IRQ_HOST_BUS_TIMEOUT      (1 << 24)

// the Hsync Vsync signal polarity
#define DSI_HPOSITIVE                 0x02
#define DSI_HNEGATIVE                 0x00
#define DSI_VPOSITIVE                 0x04
#define DSI_VNEGATIVE                 0x00

/* DSI Registers Begin */
typedef union   _REG_MM3294_MM32AC
{
    CBIOS_U32   Value;    
    struct
    {
        CBIOS_U32    IGA_VSync_Beg            :12;
        CBIOS_U32    Reserved_0               :4;
        CBIOS_U32    IGA_VSync_End            :4;
        CBIOS_U32    Reserved_1               :11;
        CBIOS_U32    Vcnt_Reset_Value_Sel     :1; 
    };
} REG_MM3294_MM32AC; 

typedef union _REG_MM3400_MM3530	//DSI_DMA_Time_Out_Register
{
    struct
    {
        CBIOS_U32	DMA_FIFO_Time_Out	: 7;
        CBIOS_U32	Reserved	: 1;
        CBIOS_U32	DMA_FIFO_Threshold	: 16;
        CBIOS_U32	Wait_interval_between_two_command_group	: 8;
    };
    CBIOS_U32	Value;
} REG_MM3400_MM3530;

typedef union _REG_MM3404_MM3534	//DSI_Command_Data_Register
{
    struct
    {
        CBIOS_U32	DSI_Command_Data	: 32;
    };
    CBIOS_U32	Value;
} REG_MM3404_MM3534;

typedef union _REG_MM3408_MM3538	//DSI_General_Control_Register
{
    struct
    {
        CBIOS_U32	DSI_Video_Mode_Software_Reset	: 1;
        CBIOS_U32	DSI_Output_Enable	: 1;
        CBIOS_U32	DSI_Mode	: 1;
        CBIOS_U32	DSI_Video_Mode_Pixel_End_Position	: 1;
        CBIOS_U32	DSI_Virtual_Channel	: 2;
        CBIOS_U32	Enable_Command_EoTp_End_of_Transmission_packet	: 1;
        CBIOS_U32	DSI_Video_Mode_Ouput_Pixel_Format	: 2;
        CBIOS_U32	DSI_Read_Data_Ready_Interrupt_Enable	: 1;
        CBIOS_U32	Reserved	: 4;
        CBIOS_U32	DCS_DSI_16BPP	: 1;
        CBIOS_U32	ENTER_HS	: 1;
        CBIOS_U32	IGA_VSYNC_Previous_Signal_Select	: 2;
        CBIOS_U32	IGA_VSYNC_Previous_Time	: 13;
        CBIOS_U32	DSI_Mode_Bit_Load_Control	: 1;
    };
    CBIOS_U32	Value;
} REG_MM3408_MM3538;

typedef union _REG_MM340C_MM353C	//DSI_Logical_PHY_Mode_Control_Register
{
    struct
    {
        CBIOS_U32	DSI_Data_Lane_Number	: 2;
        CBIOS_U32	Data_Lane_0_Bi_directional_or_Unidirectional	: 1;
        CBIOS_U32	Clock_Lane_Control_Mode	: 1;
        CBIOS_U32	Reserved_0	: 3;
        CBIOS_U32	Enable_Turn_Around_Timeout_Check	: 1;
        CBIOS_U32	Enable_HS_TX_Timeout_Check	: 1;
        CBIOS_U32	Enable_LP_RX_Timeout_Check	: 1;
        CBIOS_U32	Contention_Detected_Sample_Select	: 1;
        CBIOS_U32	Reserved_1	: 1;
        CBIOS_U32	LPP_High_Fault_Contention_Interrupt_Enable	: 1;
        CBIOS_U32	LPN_High_Fault_Contention_Interrupt_Enable	: 1;
        CBIOS_U32	LPP_Low_Fault_Contention_Interrupt_Enable	: 1;
        CBIOS_U32	LPN_Low_Fault_Contention_Interrupt_Enable	: 1;
        CBIOS_U32	LP_RX_Timeout_Interrupt_Enable	: 1;
        CBIOS_U32	Turn_Around_Timeout_Interrupt_Enable	: 1;
        CBIOS_U32	HS_TX_Timeout_Interrupt_Enable	: 1;
        CBIOS_U32	Low_Power_Data_Transmission_Synchronization_Error_Interrupt_Enable	: 1;
        CBIOS_U32	Incorrect_Line_State_Detected_Interrupt_Enable	: 1;
        CBIOS_U32	Receive_Trigger_Message_Interrupt_Enable	: 1;
        CBIOS_U32	Contention_Detected_LPP_TX_State_Delay_Select	: 2;
        CBIOS_U32	DSI_TE_from_Pad_interrupt_Enable	: 1;
        CBIOS_U32	DSI_TE_from_Pad_interrupt_set_select	: 1;
        CBIOS_U32	Reserved_2	: 6;
    };
    CBIOS_U32	Value;
} REG_MM340C_MM353C;

typedef union _REG_MM3410_MM3540	//LPHY_Real-Time_Control_Register
{
    struct
    {
        CBIOS_U32	LPHY_Software_Reset	: 1;
        CBIOS_U32	Data_Lane_Ultra_Low_Power_State_Control	: 1;
        CBIOS_U32	Enable_Clock_lane_high_speed_clock	: 1;
        CBIOS_U32	Clock_Lane_Ultra_Low_Power_State_Control	: 1;
        CBIOS_U32	Reserved_0	: 2;
        CBIOS_U32	Driver_Send_Trigger_Message	: 1;
        CBIOS_U32	Driver_Send_Trigger_Message_in_Process	: 1;
        CBIOS_U32	Driver_Send_Trigger_Message_Data	: 8;
        CBIOS_U32	Reserved_1	: 16;
    };
    CBIOS_U32	Value;
} REG_MM3410_MM3540;

typedef union _REG_MM3414_MM3544	//LPHY_LPX_and_TA-GET_Period_Register
{
    struct
    {
        CBIOS_U32	LPX_Period	: 16;
        CBIOS_U32	TA_GET_Period	: 16;
    };
    CBIOS_U32	Value;
} REG_MM3414_MM3544;


typedef union _REG_MM3418_MM3548	//LPHY_TA-GO_and_TA-SURE_Period_Register
{
    struct
    {
        CBIOS_U32	TA_GO_Period	: 16;
        CBIOS_U32	TA_SURE_Period	: 16;
    };
    CBIOS_U32	Value;
} REG_MM3418_MM3548;


typedef union _REG_MM341C_MM354C	//LPHY_Wake_Up_Period_Register
{
    struct
    {
        CBIOS_U32	Wake_Up_Period	: 32;
    };
    CBIOS_U32	Value;
} REG_MM341C_MM354C;


typedef union _REG_MM3420_MM3550	//LPHY_Data/Clock_Lane_HS_Prepare_Period_Register
{
    struct
    {
        CBIOS_U32	HS_PREPARE_and_HS_ZERO_Period	: 16;
        CBIOS_U32	CLK_PREPARE_and_CLK_ZERO_Period	: 16;
    };
    CBIOS_U32	Value;
} REG_MM3420_MM3550;


typedef union _REG_MM3424_MM3554	//LPHY_Data/Clock_Lane_HS_Trail_Period_Register
{
    struct
    {
        CBIOS_U32	HS_TRAIL_Period	: 8;
        CBIOS_U32	CLK_TRAIL_Period	: 8;
        CBIOS_U32	CLK_POST_Period	: 8;
        CBIOS_U32	CLK_PRE_Period	: 8;
    };
    CBIOS_U32	Value;
} REG_MM3424_MM3554;


typedef union _REG_MM3428_MM3558	//LPHY_HS-EXIT_Period_Register
{
    struct
    {
        CBIOS_U32	HS_EXIT_Period	: 8;
        CBIOS_U32	Reserved	: 24;
    };
    CBIOS_U32	Value;
} REG_MM3428_MM3558;

typedef union _REG_MM342C_MM355C	//DSI_TurnAround_Timeout_Period_Register
{
    struct
    {
        CBIOS_U32	DSI_TurnAround_Timeout_Period	: 32;
    };
    CBIOS_U32	Value;
} REG_MM342C_MM355C;


typedef union _REG_MM3430_MM3560	//DSI_HS-TX_Timeout_Period_Register
{
    struct
    {
        CBIOS_U32	DSI_HS_TX_Timeout_Period	: 32;
    };
    CBIOS_U32	Value;
} REG_MM3430_MM3560;


typedef union _REG_MM3434_MM3564	//DSI_LP-RX_Timeout_Period_Register
{
    struct
    {
        CBIOS_U32	DSI_LP_RX_Timeout_Period	: 32;
    };
    CBIOS_U32	Value;
} REG_MM3434_MM3564;

typedef union _REG_MM3440_MM3570	//DSI Command Group Wait Control register
{
    struct
    {
        CBIOS_U32	DSI_Command_Group_Wait_Time	: 8;
        CBIOS_U32	DSI_Command_Group_Wait_Enable	: 1;
        CBIOS_U32	DIU_HostBus_Timeout_Interrupt_Enable	: 1;
        CBIOS_U32	Reserved_1	: 6;
        CBIOS_U32	DSI_Command_DMA_Request_Threshold	: 8;
        CBIOS_U32	DMA_Pixel_Dataformat_32bpp	: 1;
        CBIOS_U32	DMA_Pixel_Dataformat_16bpp	: 1;
        CBIOS_U32	Switch_R_B_32bpp	: 1;
        CBIOS_U32	Switch_R_B_16bpp	: 1;
        CBIOS_U32	Special_Switch_R_B_16bpp	: 1;
        CBIOS_U32	Reserved_2	: 3;
    };
    CBIOS_U32	Value;
} REG_MM3440_MM3570;

typedef union _REG_MM3444_MM3574	//DSI_Command_Mode_Status_Register
{
    struct
    {
        CBIOS_U32	Reserved_0                  : 6;
        CBIOS_U32	DSI_Command_Busy_Status     : 1;
        CBIOS_U32	DSI_Command_DMA_Busy_Status : 1;
        CBIOS_U32	DSI_Read_Back_Data_Number   : 8;
        CBIOS_U32	DSI_Command_FIFO_Status     :10;
        CBIOS_U32	Reserved_1                  : 6;
    };
    CBIOS_U32	Value;
} REG_MM3444_MM3574;

typedef union _REG_MM3438_MM3568	//LPHY_Interrupt_Status_Register
{
    struct
    {
        CBIOS_U32	IGA1_Vertical_Interrupt_Stauts	: 1;
        CBIOS_U32	IGA2_Vertical_Interrupt_Stauts	: 1;
        CBIOS_U32	DSI_Read_Back_Data_Interrupt_Status	: 1;
        CBIOS_U32	EPHY_LPP_High_Fault_Interrupt_Status	: 1;
        CBIOS_U32	EPHY_LPN_High_Fault_Interrupt_Status	: 1;
        CBIOS_U32	EPHY_LPP_Low_Fault_Interrupt_Status	: 1;
        CBIOS_U32	EPHY_LPN_Low_Fault_Interrupt_Status	: 1;
        CBIOS_U32	LP_RX_Timer_Timeout_Interrupt_Status	: 1;
        CBIOS_U32	HS_TX_Timer_Timeout_Interrupt_Status	: 1;
        CBIOS_U32	Turnaround_Timer_Timeout_Interrupt_Status	: 1;
        CBIOS_U32	Low_Power_Data_Transmission_Synchronization_Error_Interrupt_Status	: 1;
        CBIOS_U32	Incorrect_Line_State_Detected_Interrupt_Status	: 1;
        CBIOS_U32	Receive_ULPS_Trigger_Interrupt_Status	: 1;
        CBIOS_U32	Receive_Reset_Trigger_Interrupt_Status	: 1;
        CBIOS_U32	Receive_TE_Trigger_Interrupt_Status	: 1;
        CBIOS_U32	Receive_Other_Trigger_or_Unrecognized_Stauts	: 1;
        CBIOS_U32	Receive_Trigger_Data	: 8;
        CBIOS_U32	DIU_host_bus_timeout_interrupt_status	: 1;
        CBIOS_U32	DSI_TE_from_PAD_Interrupt_status	: 1;
        CBIOS_U32	Reserved	: 6;
    };
    CBIOS_U32	Value;
} REG_MM3438_MM3568;

typedef union _REG_MM3448_MM3578	//DSI_EPHY_Control_Register
{
    struct
    {
        CBIOS_U32	Power_down_PLL	: 1;
        CBIOS_U32	PLL_charge_pump_current_control	: 3;
        CBIOS_U32	PLL_filter_C_control	: 2;
        CBIOS_U32	PLL_filter_R_control	: 2;
        CBIOS_U32	Clock_Lane_Power_Down	: 1;
        CBIOS_U32	TCLK_PREPARE	: 6;
        CBIOS_U32	Clock_Lane_Boost	: 1;
        CBIOS_U32	Power_Down_Data_Lane_0	: 1;
        CBIOS_U32	Power_Down_Data_Lane_1	: 1;
        CBIOS_U32	Power_Down_Data_Lane_2	: 1;
        CBIOS_U32	Power_Down_Data_Lane_3	: 1;
        CBIOS_U32	THS_PREPARE	: 6;
        CBIOS_U32	Data_Lane_Boost	: 1;
        CBIOS_U32	Power_Down_LP_RX	: 1;
        CBIOS_U32	Reserved	: 4;
    };
    CBIOS_U32	Value;
} REG_MM3448_MM3578;

typedef union _REG_MM344C_MM357C    //DSI_EPHY_Control_Register
{
    struct
    {
        CBIOS_U32 RTNSWX                                    : 1;
        CBIOS_U32 RTNEN                                     : 1;
        CBIOS_U32 RTNSETEN                                  : 1;
        CBIOS_U32 RTNSET                                    : 4;
        CBIOS_U32 RTNCMSET                                  : 4;
        CBIOS_U32 RTNRSTB                                   : 1;
        CBIOS_U32 RTNOUT                                    : 4;
        CBIOS_U32 EPHY_HS_Drive_Amplitude_Select            : 4;
        CBIOS_U32 EPHY_LP_Drivers_Slew_Rate_Control         : 4;
        CBIOS_U32 EPHY_Bandgap_Reference_Voltage_Select     : 3;
        CBIOS_U32 EPHY_Input_Data_Hold_Time_Select          : 4;
        CBIOS_U32 EPHY_Current_Reference_Resistor_Select    : 1;

    };
    CBIOS_U32    Value;
} REG_MM344C_MM357C;

typedef union _REG_MM3450_MM3580    //DSI_EPHY_Control_Register
{
    struct
    {
        CBIOS_U32 EPHY_CHY : 8 ;
        CBIOS_U32 EPHY_CHX :8  ;
        CBIOS_U32 EPHY_REFS : 4 ;
        CBIOS_U32 Reserved  : 12 ;
    };
    CBIOS_U32    Value;
} REG_MM3450_MM3580;


typedef union _REG_MM345C_MM358C	//DSI_Video_Mode_Register_1
{
    struct
    {
        CBIOS_U32	DSI_Video_Mode_EoTp_Packet_Enable	: 1;
        CBIOS_U32	Reserved	: 6;
        CBIOS_U32	HFP_LP	: 1;
        CBIOS_U32	HSA_LP	: 1;
        CBIOS_U32	HBP_LP	: 1;
        CBIOS_U32	HSS_EN	: 1;
        CBIOS_U32	HSE_EN	: 1;
        CBIOS_U32	VSS_EN	: 1;
        CBIOS_U32	VSE_EN	: 1;
        CBIOS_U32	DSI_Video_Mode_Enable	: 1;
        CBIOS_U32	DSI_Video_Mode_Enable_Flip_Control	: 1;
        CBIOS_U32	PXL_WC	: 16;
    };
    CBIOS_U32	Value;
} REG_MM345C_MM358C;


typedef union _REG_MM3460_MM3590	//DSI_Video_Mode_Register_2
{
    struct
    {
        CBIOS_U32	HFP_WC	: 16;
        CBIOS_U32	HSA_WC	: 16;
    };
    CBIOS_U32	Value;
} REG_MM3460_MM3590;


typedef union _REG_MM3464_MM3594	//DSI_Video_Mode_Register_3
{
    struct
    {
        CBIOS_U32	HBP_WC	: 16;
        CBIOS_U32	HSS_LP2HS	: 16;
    };
    CBIOS_U32	Value;
} REG_MM3464_MM3594;


typedef union _REG_MM3468_MM3598	//DSI_Video_Mode_Register_4
{
    struct
    {
        CBIOS_U32	HSE_LP2HS	: 16;
        CBIOS_U32	PXL_LP2HS	: 16;
    };
    CBIOS_U32	Value;
} REG_MM3468_MM3598;

typedef union _REG_MM346C 	//backlight adjustment
{
    struct
    {
        CBIOS_U32	reg_backlight_factor	: 16;
        CBIOS_U32	pwm_frequency_counter	: 16;
    };
    CBIOS_U32	Value;
} REG_MM346C;

typedef union _REG_MM3470 	//backlight adjustment
{
    struct
    {
        CBIOS_U32	pwm_enable	: 1;
        CBIOS_U32	Reserved_0	: 3;
        CBIOS_U32	reg_pwm_control	: 4;
        CBIOS_U32	Reserved_1	: 24;
    };
    CBIOS_U32	Value;
} REG_MM3470;

typedef union _REG_MM3480	//MDI misc registers
{
    struct
    {
        CBIOS_U32	ONE_CHANNEL	: 1;
        CBIOS_U32	LCH_EN	: 1;
        CBIOS_U32	LCH_MODE	: 2;
        CBIOS_U32	SS_DMA1_SEL 	: 1;
        CBIOS_U32	TS_DMA2_SEL 	: 1;
        CBIOS_U32	PS1_FF_DEEPTH 	: 3;
        CBIOS_U32	PS2_FF_DEEPTH 	: 3;
        CBIOS_U32	CHANNEL_SIZE 	: 3;
        CBIOS_U32	MIU_BANK_NUM 	: 1;
        CBIOS_U32	MIU_PAGE_SIZE 	: 2;
        CBIOS_U32	AUD_LCH_EN 	: 1;
        CBIOS_U32	IDLE_ROUND_EN 	: 1;
        CBIOS_U32	RDATA_FF_LEFT 	: 4;
        CBIOS_U32	RDATA_FF_THRESHOLD 	: 8;
    };
    CBIOS_U32	Value;
} REG_MM3480;

typedef union _REG_MM349C	//Pad Control
{
    struct
    {
        CBIOS_U32    GPIO3_REN:       1;
        CBIOS_U32    GPIO4_REN:       1;
        CBIOS_U32    GPIO5_REN:       1;
        CBIOS_U32    GPIO6_REN:       1;
        CBIOS_U32    GPIO7_REN:       1;
        CBIOS_U32    GPIO8_REN:       1;
        CBIOS_U32    GPIO9_REN:       1;
        CBIOS_U32    ENVDD_REN:       1;
        CBIOS_U32    ENVEE_REN:       1;
        CBIOS_U32    DSI_TE_IEN:      1;
        CBIOS_U32    MHLSWT_OEN:      1;
        CBIOS_U32    ENVDD_ENVEE_OEN: 1;
        CBIOS_U32    Reserved: 20;
    };
    CBIOS_U32    Value;
} REG_MM349C;

typedef union _REG_MM34A0
{
    CBIOS_U32    Value;    
    struct
    {
        CBIOS_U32    DSI_Clock_Enable :1;
        CBIOS_U32    MHL_Enable       :1;
        CBIOS_U32    MHL_Mode         :1;   //0: MHL normal mode, 1: MHL packed mode
        CBIOS_U32    Reserved         :29;
    };
} REG_MM34A0;

typedef union _REG_MM34BC_MM35EC 	//
{
    struct
    {
        CBIOS_U32	Y_START	: 12;
        CBIOS_U32	Reserved_0	: 4;
        CBIOS_U32	Y_HEIGHT	: 12;
        CBIOS_U32	Reserved_1	: 3;
        CBIOS_U32	DSI_Partial_Timing_Enable	: 1;
    };
    CBIOS_U32	Value;
} REG_MM34BC_MM35EC;

typedef union _REG_MM34C0_MM34C8	//
{
    struct
    {
        CBIOS_U32	X_START	: 13;
        CBIOS_U32	Y_START	: 12;
        CBIOS_U32	Reserved	: 6;
        CBIOS_U32	DSI_CMD_Mode_Enable	: 1;
    };
    CBIOS_U32	Value;
} REG_MM34C0_MM34C8;

typedef union _REG_MM34C4_MM34CC	//
{
    struct
    {
        CBIOS_U32	Width	: 13;
        CBIOS_U32	Height	: 12;
        CBIOS_U32	Reserved	: 7;
    };
    CBIOS_U32	Value;
} REG_MM34C4_MM34CC;

/* DSI Registers End */

/* MIPI DSI Processor-to-Peripheral transaction packet data types */
typedef enum _CBIOS_DSI_PROCESSOR_DATA_TYPE
{
    DSI_V_SYNC_START                           = 0x01,
    DSI_V_SYNC_END                             = 0x11,
    DSI_H_SYNC_START                           = 0x21,
    DSI_H_SYNC_END                             = 0x31,
    DSI_END_OF_TRANSMISSION                    = 0x08,
    DSI_COLOR_MODE_OFF                         = 0x02,
    DSI_COLOR_MODE_ON                          = 0x12,
    DSI_SHUTDOWN_PERIPHERAL                    = 0x22,
    DSI_TURN_ON_PERIPHERAL                     = 0x32,
    DSI_GENERIC_SHORT_WRITE_0_PARAM            = 0x03,
    DSI_GENERIC_SHORT_WRITE_1_PARAM            = 0x13,
    DSI_GENERIC_SHORT_WRITE_2_PARAM            = 0x23,
    DSI_GENERIC_READ_REQUEST_0_PARAM           = 0x04,
    DSI_GENERIC_READ_REQUEST_1_PARAM           = 0x14,
    DSI_GENERIC_READ_REQUEST_2_PARAM           = 0x24,
    DSI_DCS_SHORT_WRITE                        = 0x05,
    DSI_DCS_SHORT_WRITE_PARAM                  = 0x15,
    DSI_DCS_READ                               = 0x06,
    DSI_SET_MAXIMUM_RETURN_PACKET_SIZE         = 0x37,
    DSI_NULL_PACKET                            = 0x09,
    DSI_BLANKING_PACKET                        = 0x19,
    DSI_GENERIC_LONG_WRITE                     = 0x29,
    DSI_DCS_LONG_WRITE                         = 0x39,
    DSI_LOOSELY_PACKED_PIXEL_STREAM_YCBCR20    = 0x0c,
    DSI_PACKED_PIXEL_STREAM_YCBCR24            = 0x1c,
    DSI_PACKED_PIXEL_STREAM_YCBCR16            = 0x2c,
    DSI_PACKED_PIXEL_STREAM_30                 = 0x0d,
    DSI_PACKED_PIXEL_STREAM_36	               = 0x1d,
    DSI_PACKED_PIXEL_STREAM_YCBCR12            = 0x3d,
    DSI_PACKED_PIXEL_STREAM_16                 = 0x0e,
    DSI_PACKED_PIXEL_STREAM_18                 = 0x1e,
    DSI_PIXEL_STREAM_3BYTE_18                  = 0x2e,
    DSI_PACKED_PIXEL_STREAM_24                 = 0x3e,
} CBIOS_DSI_PROCESSOR_DATA_TYPE;

/* MIPI DSI Peripheral-to-Processor transaction packet data types */
typedef enum _CBIOS_DSI_PERIPHERAL_DATA_TYPE
{
    DSI_RX_ACKNOWLEDGE_AND_ERROR_REPORT         = 0x02,
    DSI_RX_END_OF_TRANSMISSION                  = 0x08,
    DSI_RX_GENERIC_SHORT_READ_RESPONSE_1BYTE    = 0x11,
    DSI_RX_GENERIC_SHORT_READ_RESPONSE_2BYTE    = 0x12,
    DSI_RX_GENERIC_LONG_READ_RESPONSE           = 0x1a,
    DSI_RX_DCS_LONG_READ_RESPONSE               = 0x1c,
    DSI_RX_DCS_SHORT_READ_RESPONSE_1BYTE        = 0x21,
    DSI_RX_DCS_SHORT_READ_RESPONSE_2BYTE        = 0x22,
} CBIOS_DSI_PERIPHERAL_DATA_TYPE;


/* MIPI DCS commands */
typedef enum _CBIOS_DSI_DCS_COMMAND
{
    DSI_DCS_NOP                      = 0x00,
    DSI_DCS_SOFT_RESET               = 0x01,
    DSI_DCS_GET_DISPLAY_ID           = 0x04,
    DSI_DCS_GET_RED_CHANNEL          = 0x06,
    DSI_DCS_GET_GREEN_CHANNEL        = 0x07,
    DSI_DCS_GET_BLUE_CHANNEL         = 0x08,
    DSI_DCS_GET_DISPLAY_STATUS       = 0x09,
    DSI_DCS_GET_POWER_MODE           = 0x0A,
    DSI_DCS_GET_ADDRESS_MODE         = 0x0B,
    DSI_DCS_GET_PIXEL_FORMAT	     = 0x0C,
    DSI_DCS_GET_DISPLAY_MODE	     = 0x0D,
    DSI_DCS_GET_SIGNAL_MODE          = 0x0E,
    DSI_DCS_GET_DIAGNOSTIC_RESULT    = 0x0F,
    DSI_DCS_ENTER_SLEEP_MODE         = 0x10,
    DSI_DCS_EXIT_SLEEP_MODE          = 0x11,
    DSI_DCS_ENTER_PARTIAL_MODE       = 0x12,
    DSI_DCS_ENTER_NORMAL_MODE        = 0x13,
    DSI_DCS_EXIT_INVERT_MODE         = 0x20,
    DSI_DCS_ENTER_INVERT_MODE        = 0x21,
    DSI_DCS_SET_GAMMA_CURVE          = 0x26,
    DSI_DCS_SET_DISPLAY_OFF          = 0x28,
    DSI_DCS_SET_DISPLAY_ON           = 0x29,
    DSI_DCS_SET_COLUMN_ADDRESS       = 0x2A,
    DSI_DCS_SET_PAGE_ADDRESS         = 0x2B,
    DSI_DCS_WRITE_MEMORY_START       = 0x2C,
    DSI_DCS_WRITE_LUT                = 0x2D,
    DSI_DCS_READ_MEMORY_START        = 0x2E,
    DSI_DCS_SET_PARTIAL_ROWS         = 0x30,
    DSI_DCS_SET_PARTIAL_COLUMNS      = 0x31,
    DSI_DCS_SET_SCROLL_AREA          = 0x33,
    DSI_DCS_SET_TEAR_OFF             = 0x34,
    DSI_DCS_SET_TEAR_ON              = 0x35,
    DSI_DCS_SET_ADDRESS_MODE         = 0x36,
    DSI_DCS_SET_SCROLL_START         = 0x37,
    DSI_DCS_EXIT_IDLE_MODE           = 0x38,
    DSI_DCS_ENTER_IDLE_MODE          = 0x39,
    DSI_DCS_SET_PIXEL_FORMAT         = 0x3A,
    DSI_DCS_WRITE_MEMORY_CONTINUE    = 0x3C,
    DSI_DCS_READ_MEMORY_CONTINUE     = 0x3E,
    DSI_DCS_SET_TEAR_SCANLINE        = 0x44,
    DSI_DCS_GET_SCANLINE             = 0x45,
    DSI_DCS_READ_DDB_START           = 0xA1,
    DSI_DCS_READ_DDB_CONTINUE        = 0xA8,
} CBIOS_DSI_DCS_COMMAND;

typedef enum _CBIOS_DSI_PANELID
{
    DSI_PANELID_HX8369A = 0,
    DSI_PANELID_HX8392A = 1,
    DSI_PANELID_R63417  = 2
} CBIOS_DSI_PANELID;

typedef enum _CBIOS_DSI_INDEX
{
    DSI_INDEX0 = 0,
    DSI_INDEX1 = 1
} CBIOS_DSI_INDEX;

typedef enum _CBIOS_DSI_DCS_MODE
{
    CBIOS_DSI_DCS_IDLE_MODE = 0x00,
    CBIOS_DSI_DCS_INVERT_MODE,
    CBIOS_DSI_DCS_NORMAL_MODE,
    CBIOS_DSI_DCS_PARTIAL_MODE,
    CBIOS_DSI_DCS_SLEEP_MODE,
} CBIOS_DSI_DCS_MODE;

typedef struct _CBIOS_DSI_PHY_PARAMS
{
    CBIOS_U32    LPX;
    CBIOS_U32    HS_PREPARE;
    CBIOS_U32    HS_PREPARE_ZERO;
    CBIOS_U32    HS_TRAIL;
    CBIOS_U32    HS_EXIT;
    CBIOS_U32    CLK_PRE;
    CBIOS_U32    CLK_PREPARE;
    CBIOS_U32    CLK_PREPARE_ZERO;
    CBIOS_U32    CLK_POST;
    CBIOS_U32    CLK_TRAIL;
    CBIOS_U32    TA_GET;
    CBIOS_U32    TA_GO;
    CBIOS_U32    TA_SURE;
    CBIOS_U32    WakeUp;

    CBIOS_U32    tLP2HS;
    CBIOS_U32    tHS2LP;
} CBIOS_DSI_PHY_PARAMS, *PCBIOS_DSI_PHY_PARAMS;





typedef struct _CBIOS_DSI_VIDEO_TIMING_REG
{
    CBIOS_U32    PXL_WC;
    CBIOS_U32    HFP_WC;
    CBIOS_U32    HSA_WC;
    CBIOS_U32    HBP_WC;
    CBIOS_U32    HSS_LP2HS;
    CBIOS_U32    HSE_LP2HS;
    CBIOS_U32    PXL_LP2HS;
} CBIOS_DSI_VIDEO_TIMING_REG, *PCBIOS_DSI_VIDEO_TIMING_REG;

// CBIOS_DSI_WRITE_PARA is for CBIOS interface use, and CBIOS_DSI_WRITE_PARA_INTERNAL is for CBIOS internal use
typedef struct _CBIOS_DSI_WRITE_PARA_INTERNAL
{
    CBIOS_U32                 DSIIndex;       // 0: DSI-1, 1:DSI-2
    CBIOS_U8                  VirtualCh;      // refer MIPI-DSI spec 4.2.3
    CBIOS_DSI_PACKET_TYPE     PacketType;     // 0: short packet, 1: long packet
    CBIOS_DSI_CONTENT_TYPE    ContentType;    // 0: DCS write cmd, 1: generic write cmd
    CBIOS_U16                 DataLen;
    PCBIOS_U8                 pDataBuf;
    union
    {
        CBIOS_U32             DSIFlags;
        struct
        {
            CBIOS_U32         bNeedAck        : 1;   // 1: the cmd need acknowledge, 0: no need
            CBIOS_U32         bHSModeOnly     : 1;   // 1: the cmd can be transferred in hs mode only, 0: both LP and HS mode
            CBIOS_U32         Reserved        : 30;
        };
    };
} CBIOS_DSI_WRITE_PARA_INTERNAL, *PCBIOS_DSI_WRITE_PARA_INTERNAL;

// CBIOS_DSI_READ_PARA is for CBIOS interface use, and CBIOS_DSI_READ_PARA_INTERNAL is for CBIOS internal use
typedef struct _CBIOS_DSI_READ_PARA_INTERNAL
{
    CBIOS_U32                 Size;
    CBIOS_U32                 DSIIndex;              // 0: DSI-1, 1:DSI-2
    CBIOS_U8                  VirtualCh;             // refer MIPI-DSI spec 4.2.3
    CBIOS_DSI_CONTENT_TYPE    ContentType;           // 0: DCS read cmd, 1: generic read cmd
    CBIOS_U8                  DataLen;
    PCBIOS_U8                 pDataBuf;
    PCBIOS_U8                 pReceivedPayloadBuf;      // The buffer to store the receive payload
    CBIOS_U16                 ReceivedPayloadLen;       // The length of response payload after the DCS read cmd
    union
    {
        CBIOS_U32              DSIReadFlags;
        struct
        {
            CBIOS_U32          bHSModeOnly     : 1;   // 1: the cmd can be transferred in hs mode only, 0: both LP and HS mode
            CBIOS_U32          Reserved        : 31;
        };
    };
} CBIOS_DSI_READ_PARA_INTERNAL, *PCBIOS_DSI_READ_PARA_INTERNAL;


typedef struct _CBIOS_DSI_PARAMS
{
    CBIOS_DSI_PANEL_DESC          DSIPanelDesc;
    CBIOS_DSI_PHY_PARAMS          PhyParams;
    CBIOS_DSI_UPDATE_PARA         UpdatePara;
    CBIOS_DSI_HOSTUPDATE_PARA     HostUpdatePara;
    CBIOS_TIMING_ATTRIB           TargetTiming;
    PCBIOS_U8                     pPanelDataBuf;
    CBIOS_BOOL                    bInitialized;
} CBIOS_DSI_PARAMS, *PCBIOS_DSI_PARAMS;


CBIOS_STATUS cbDSI_SendWriteCmd(PCBIOS_VOID pvcbe, PCBIOS_DSI_WRITE_PARA_INTERNAL pDSIWriteParams);
CBIOS_STATUS cbDSI_SendReadCmd(PCBIOS_VOID pvcbe, PCBIOS_DSI_READ_PARA_INTERNAL pDSIReadParams);
CBIOS_STATUS cbDSI_SendCmdList(PCBIOS_VOID pvcbe, PCBIOS_DSI_CMD_DESC pCmdList, CBIOS_U32 CmdCount);
CBIOS_STATUS cbDSI_DisplayUpdate(PCBIOS_VOID pvcbe, PCBIOS_DSI_UPDATE_PARA pUpdatePara);
CBIOS_STATUS cbDSI_SetTE_BTA(PCBIOS_VOID pvcbe, CBIOS_DSI_INDEX DSIIndex);

#endif
