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


#ifndef _PMU_REGISTER_H_
#define _PMU_REGISTER_H_


typedef union _REG_PMU_C6C0
{
    struct
    {
        CBIOS_U32 DIU_BIU_DVO_INT   :1;
        CBIOS_U32 DIU_BIU_HDMIIN5V_INT  :1;
        CBIOS_U32 DP1_INT   :1;
        CBIOS_U32 DP2_INT   :1;
        CBIOS_U32 CEC1_INT  :1;
        CBIOS_U32 CEC2_INT  :1;
        CBIOS_U32 RESERVED  :26;
    };
    CBIOS_U32 Value;
}REG_PMU_C6C0;

typedef union _REG_PMU_C6C4
{
    struct
    {
        CBIOS_U32 DIU_BIU_DVO_INT_CLR   :1;
        CBIOS_U32 DIU_BIU_HDMIIN5V_INT_CLR  :1;
        CBIOS_U32 DP1_INT_CLR   :1;
        CBIOS_U32 DP2_INT_CLR   :1;
        CBIOS_U32 CEC1_INT_CLR  :1;
        CBIOS_U32 CEC2_INT_CLR  :1;
        CBIOS_U32 RESERVED  :26;
    };
    CBIOS_U32 Value;
}REG_PMU_C6C4;

typedef union _REG_PMU_C6C8
{
    struct
    {
        CBIOS_U32   DP_INT_MODE :1;
        CBIOS_U32   Reserved    :31;
    };
    CBIOS_U32 Value;
}REG_PMU_C6C8;

typedef union _REG_PMU_C6CC
{
    struct
    {
        CBIOS_U32   DIU_BIU_DVO_INT_FE  :1;
        CBIOS_U32   DIU_BIU_DVO_INT_RE  :1;
        CBIOS_U32   DIU_BIU_HDMIIN5V_INT_FE :1;
        CBIOS_U32   DIU_BIU_HDMIIN5V_INT_RE :1;
        CBIOS_U32   DP2_INT_SC  :1;
        CBIOS_U32   DP2_INT_POUT    :1;
        CBIOS_U32   DP2_INT_PIN :1;
        CBIOS_U32   DP1_INT_SC  :1;
        CBIOS_U32   DP1_INT_POUT    :1;
        CBIOS_U32   DP1_INT_PIN :1;
        CBIOS_U32   CEC1_INT    :1;
        CBIOS_U32   CEC2_INT    :1;
        CBIOS_U32   GPIO5_INT_FE    :1;
        CBIOS_U32   GPIO5_INT_RE    :1;
        CBIOS_U32   GPIO4_INT_FE    :1;
        CBIOS_U32   GPIO4_INT_RE    :1;
        CBIOS_U32   PWRBTN_SHORT_PRESS  :1;
        CBIOS_U32   PWRBTN_LONG_PRESS   :1;
        CBIOS_U32   Reserved    :14;
    };
    CBIOS_U32 Value;
}REG_PMU_C6CC;

typedef union _REG_PMU_3200
{
    struct
    {
        CBIOS_U32   Gpio1_Ouptut_Data  :1;
        CBIOS_U32   Gpio1_Output_Enable  :1;
        CBIOS_U32   Gpio1_Input_Enable :1;
        CBIOS_U32   Gpio1_Input_Data :1;
        CBIOS_U32   GPIO_1_PU0  :1;
        CBIOS_U32   GPIO_1_PU1  :1;
        CBIOS_U32   GPIO_1_PD0  :1;
        CBIOS_U32   GPIO_1_PD1  :1;
        CBIOS_U32   Reserved    :24;
    };
    CBIOS_U32 Value;
}REG_PMU_3200;

typedef union _REG_PMU_320C
{
    struct
    {
        CBIOS_U32   Gpio4_Ouptut_Data  :1;
        CBIOS_U32   Gpio4_Output_Enable  :1;
        CBIOS_U32   Gpio4_Input_Enable :1;
        CBIOS_U32   Gpio4_Input_Data :1;
        CBIOS_U32   Reserved    :24;
    };
    CBIOS_U32 Value;
}REG_PMU_320C;

typedef union _REG_PMU_3210
{
    struct
    {
        CBIOS_U32   Gpio5_Ouptut_Data  :1;
        CBIOS_U32   Gpio5_Output_Enable  :1;
        CBIOS_U32   Gpio5_Input_Enable :1;
        CBIOS_U32   Gpio5_Input_Data :1;
        CBIOS_U32   Reserved    :24;
    };
    CBIOS_U32 Value;
}REG_PMU_3210;

typedef union _REG_PMU_D2E0
{
    struct 
    {
        CBIOS_U32  Mpll_Int        :7;
        CBIOS_U32  Mpll_Fn         :10;
        CBIOS_U32  Mpll_R          :3;
        CBIOS_U32  Mpll_Cp         :3;
        CBIOS_U32  Mpll_Lpf        :2;
        CBIOS_U32  Mpll_Order      :1;
        CBIOS_U32  Mpll_Sscg_Sel   :1;
        CBIOS_U32  Mpll_Sscg_Flag1 :1;
        CBIOS_U32  Mpll_Testb      :1;
        CBIOS_U32  Sw_Mpll_Reset   :1;
        CBIOS_U32  Sw_Mpll_Load    :1;
        CBIOS_U32  Sw_Mpll_Pd      :1;
    };
    CBIOS_U32 Value;
}REG_PMU_D2E0;

typedef union _REG_PMU_C6F4
{
    struct 
    {
        CBIOS_U32  s3vd_clk_sel             :1;
        CBIOS_U32  s3vd_clk_en              :1;
        CBIOS_U32  s3vd_eipll_clk_div       :2;
        CBIOS_U32  s3vd_peripll_clk_div     :2;
        CBIOS_U32  Reserved                 :26;
    };
    CBIOS_U32 Value;
}REG_PMU_C6F4;

typedef union _REG_PMU_C340
{
    struct 
    {
        CBIOS_U32  CPUPLL_FN            :10;
        CBIOS_U32  CPUPLL_INT           :7;
        CBIOS_U32  Reserved_1           :3;
        CBIOS_U32  CPUPLL_R             :3;
        CBIOS_U32  Reserved_2           :4;
        CBIOS_U32  manual_cfg_en        :1;
        CBIOS_U32  CPUPLL_LOAD          :1;
        CBIOS_U32  CPUPLL_PD            :1;
        CBIOS_U32  Reserved_3           :2;
    };
    CBIOS_U32 Value;
}REG_PMU_C340;

typedef union _REG_PMU_C348 //CPU_PLL_STATUS
{
    struct 
    {
        CBIOS_U32  CPUPLL_FN            :10;
        CBIOS_U32  CPUPLL_INT           :7;
        CBIOS_U32  Reserved_1           :3;
        CBIOS_U32  CPUPLL_R             :3;
        CBIOS_U32  Reserved_2           :5;
        CBIOS_U32  CPUPLL_LOAD          :1;
        CBIOS_U32  CPUPLL_PD            :1;
        CBIOS_U32  Reserved_3           :1;
        CBIOS_U32  CPU_PLL_LOCK         :1;
    };
    CBIOS_U32 Value;
}REG_PMU_C348;

typedef union _REG_PMU_E080 //DEBUG_INFO_CTRL
{
    struct 
    {
        CBIOS_U32  SPNIDEN_PL310        :1;
        CBIOS_U32  Reserved_1           :17;
        CBIOS_U32  CPU_CLK_MUX_SEL      :1;
        CBIOS_U32  Reserved_2           :13;
    };
    CBIOS_U32 Value;
}REG_PMU_E080;


typedef union _REG_PMC_0250
{
    struct
    {
        CBIOS_U32   I2C1_Clock_Enable          :1;
        CBIOS_U32   UART0_Clock_Enable         :1;
        CBIOS_U32   UART1_Clock_Enable         :1;
        CBIOS_U32   UART2_Clock_Enable         :1;
        CBIOS_U32   UART3_Clock_Enable         :1;
        CBIOS_U32   I2C0_Clock_Enable          :1;
        CBIOS_U32   Reserved_0                 :1;
        CBIOS_U32   RTC_Clock_Enable           :1;
        CBIOS_U32   TZPC_Clock_Enable          :1;
        CBIOS_U32   KPAD_Clock_Enable          :1;
        CBIOS_U32   PWM_Clock_Enable           :1;
        CBIOS_U32   GPIO_Clock_Enable          :1;
        CBIOS_U32   SPI0_Clock_Enable          :1;
        CBIOS_U32   SPI1_Clock_Enable          :1;
        CBIOS_U32   UART4_Clock_Enable         :1;
        CBIOS_U32   Reserved_1                 :1;
        CBIOS_U32   SD1_Clock_Enable           :1;
        CBIOS_U32   CIR_Timers_Clock_Enable    :1;
        CBIOS_U32   Random_Num_Clock_Enable    :1;
        CBIOS_U32   Reserved_2                 :1;
        CBIOS_U32   PCM                        :1;
        CBIOS_U32   SCC_Clock_Enable           :1;
        CBIOS_U32   Reserved_3                 :3;
        CBIOS_U32   SD2_Clock_Enable           :1;
        CBIOS_U32   SD3_Clock_Enable           :1;
        CBIOS_U32   Reserved_4                 :1;
        CBIOS_U32   PCIE0_Clock_Enable         :1;
        CBIOS_U32   Reserved_5                 :2;
        CBIOS_U32   GFX_BIU_Clock_Enable       :1;
    };
    CBIOS_U32 Value;
}REG_PMC_0250;



#endif
