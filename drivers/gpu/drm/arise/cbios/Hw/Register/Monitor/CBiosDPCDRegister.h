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
** DPCD(DP Configuration Data) register definition.
**   For detailed register definition, please refer to DP1.2 spec in sector 2.9.3.1
******************************************************************************/

#ifndef _CBIOS_DPCD_REGISTER_H_
#define _CBIOS_DPCD_REGISTER_H_

typedef union _DPCD_REG_00000
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    Minor_Rev_Num   :4;
        CBIOS_U8    Major_Rev_Num   :4;
    };
}DPCD_REG_00000;

typedef union _DPCD_REG_00001
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    MAX_LINK_RATE   :8; // 0x06: 1.62Gbps per lane
                                        // 0x0A: 2.7 Gbps per lane
                                        // 0x14: 5.4 Gbps per lane
    };
}DPCD_REG_00001;

typedef union _DPCD_REG_00002
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    MAX_LANE_COUNT      :5;
        CBIOS_U8    Reserved0           :1;
        CBIOS_U8    TPS3_SUPPORTED      :1; // For DPCD Rev.1.2
        CBIOS_U8    ENHANCED_FRAME_CAP  :1; // For DPCD Rev.1.1
    };
}DPCD_REG_00002;

typedef union _DPCD_REG_00003
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    MAX_DOWNSPREAD                  :1;
        CBIOS_U8    Reserved0                       :5;
        CBIOS_U8    NO_AUX_HANDSHAKE_LINK_TRAINING  :1; // For DPCD Rev.1.1
        CBIOS_U8    Reserved1                       :1;
    };
}DPCD_REG_00003;

typedef union _DPCD_REG_00004
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    NORP            :1; // Number of Receiver Ports = Value + 1 
        CBIOS_U8    Reserved0       :4;
        CBIOS_U8    DP_PWR_CAP_5V   :1; // For DPCD Rev 1.2
        CBIOS_U8    DP_PWR_CAP_12V  :1; // For DPCD Rev 1.2
        CBIOS_U8    DP_PWR_CAP_18V  :1; // For DPCD Rev 1.2
    };
}DPCD_REG_00004;

typedef union _DPCD_REG_00005
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    DWN_STRM_PORT_PRESENT       :1;
        CBIOS_U8    DWN_STRM_PORT_TYPE          :2; // 0 = DisplayPort
                                                    // 1 = Analog VGA or analog video over DVI-I
                                                    // 2 = DVI, HDMI or DP++
                                                    // 3 = Others (This Downstream port type will have no EDID in the Sink device)
        CBIOS_U8    FORMAT_CONVERSION           :1; // For DPCD Rev 1.1
        CBIOS_U8    DETAILED_CAP_INFO_AVAILABLE :1; // For DPCD Rev 1.1
        CBIOS_U8    Reserved0                   :3; // For DPCD Rev 1.1
    };
}DPCD_REG_00005;

typedef union _DPCD_REG_00007
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    DWN_STRM_PORT_COUNT         :4; // For DPCD Rev 1.1
        CBIOS_U8    Reserved0                   :2; // For DPCD Rev 1.1
        CBIOS_U8    MSA_TIMING_PAR_IGNORED      :1; // For DPCD Rev 1.1
        CBIOS_U8    OUI_Support                 :1; // For DPCD Rev 1.1
    };
}DPCD_REG_00007;

typedef union _DPCD_REG_0000D // eDP_CONFIGURATION_CAP
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    ALTERNATE_SCRAMBLER_RESET_CAPABLE   :1; // A setting of 1 indicates that this is an eDP device that can use 
                                                            // the eDP alternate scrambler reset value of FFFEh.  
        CBIOS_U8    FRAMING_CHANGE_CAPABLE              :1; // A setting of 1 indicates that this is an eDP device that uses only 
                                                            // Enhanced Framing, independently of the setting by the source of 
                                                            // ENHANCED_FRAME_EN 
        CBIOS_U8    Reserved0                           :6;
    };
}DPCD_REG_0000D;

typedef union _DPCD_REG_0000E  //From DP1.3, bit 7 is used to identify extended capability present
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TRAINING_AUX_RD_INTERVAL    :7; // 0x00: 100us
                                                    // 0x01: 4  ms
                                                    // 0x02: 8  ms
                                                    // 0x03: 12 ms
                                                    // 0x04: 16 ms
        CBIOS_U8    EXTENDED_RECEIVER_CAPABILITY_FIELD_PRESENT  :1;
                                                    //0 = Not present. 1 = Present at DPCD Addresses 02200h through 022FFh.
                                                    //A DPRX with DPCD Rev. 1.4 (or higher) must have an Extended Receiver Capability field.
    };
}DPCD_REG_0000E;

typedef union _DPCD_REG_00100
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    LINK_BW_SET     :8;
    };
}DPCD_REG_00100;

typedef union _DPCD_REG_00101
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    LANE_COUNT_SET      :5;
        CBIOS_U8    Reserved0           :2;
        CBIOS_U8    ENHANCED_FRAME_EN   :1;
    };
}DPCD_REG_00101;

typedef union _DPCD_REG_00102
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TRAINING_PATTERN_SELECT :2; // 0: Training not in progress (or disabled)
                                                // 1: Training Pattern 1
                                                // 2: Training Pattern 2
                                                // 3: Training Pattern 3
        CBIOS_U8    LINK_QUAL_PATTERN_SET   :2; // 0: Link quality test pattern not transmitted
                                                // 1: D10.2 test pattern (unscrambled) transmitted (same as Training Pattern 1)
                                                // 2: Symbol Error Rate measurement pattern transmitted
                                                // 3: PRBS7 transmitted
        CBIOS_U8    RECOVERED_CLOCK_OUT_EN  :1;
        CBIOS_U8    SCRAMBLING_DISABLE      :1;
        CBIOS_U8    SYMBOL_ERROR_COUNT_SEL  :2; // 0: Disparity error and Illegal Symbol error 
                                                // 1: Disparity error 
                                                // 2: Illegal symbol error 
                                                // 3: RESERVED 
    };
}DPCD_REG_00102;

// 00103, 00104, 00105, 00106
typedef union _DPCD_REG_00103
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    VOLTAGE_SWING_SET           :2; // 0: Voltage swing level 0
                                                    // 1: Voltage swing level 1 
                                                    // 2: Voltage swing level 2 
                                                    // 3: Voltage swing level 3 
        CBIOS_U8    MAX_SWING_REACHED           :1;
        CBIOS_U8    PRE_EMPHASIS_SET            :2; // 0: Pre-emphasis level 0
                                                    // 1: Pre-emphasis level 1
                                                    // 2: Pre-emphasis level 2
                                                    // 3: Pre-emphasis level 3
        CBIOS_U8    MAX_PRE_EMPHASIS_REACHED    :1;
        CBIOS_U8    Reserved0                   :2;
    };
}DPCD_REG_00103;

typedef union _DPCD_REG_0010A
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    ALTERNATE_SCRAMBER_RESET_ENABLE :1;
        CBIOS_U8    FRAMING_CHANGE_ENABLE           :1;
        CBIOS_U8    Reserved0                       :5;
        CBIOS_U8    PANEL_SELF_TEST_ENABLE          :1;
    };
}DPCD_REG_0010A;

// 0010B, 0010C, 0010D, 0010E
typedef union _DPCD_REG_0010B
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    LINK_QUAL_PATTERN_SET       :3; // 0: Link quality test pattern not transmitted 
                                                    // 1: D10.2 test pattern (unscrambled) transmitted (same as Training Pattern 1)
                                                    // 2: Symbol Error Rate measurement pattern transmitted
                                                    // 3: PRBS7 transmitted
                                                    // 4: 80 bit custom pattern transmitted 
                                                    // 5: HBR2 Compliance EYE pattern transmitted 
        CBIOS_U8    Reserved0                   :5;
    };
}DPCD_REG_0010B;

// 0010F, 00110
typedef union _DPCD_REG_0010F
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    LANE0_POST_CURSOR2_SET          :2; // 0: Training Pattern 2 or 3 with post cursor2 level 0
                                                        // 1: Training Pattern 2 or 3 with post cursor2 level 1
                                                        // 2: Training Pattern 2 or 3 with post cursor2 level 2
                                                        // 3: Training Pattern 2 or 3 with post cursor2 level 3
        CBIOS_U8    Lane0_MAX_POST_CURSOR2_REACHED  :1;
        CBIOS_U8    Reserved0                       :1;
        CBIOS_U8    LANE1_POST_CURSOR2_SET          :2; // 0: Training Pattern 2 or 3 with post cursor2 level 0
                                                        // 1: Training Pattern 2 or 3 with post cursor2 level 1
                                                        // 2: Training Pattern 2 or 3 with post cursor2 level 2
                                                        // 3: Training Pattern 2 or 3 with post cursor2 level 3
        CBIOS_U8    Lane1_MAX_POST_CURSOR2_REACHED  :1;
        CBIOS_U8    Reserved1                       :1;
    };
}DPCD_REG_0010F;

typedef union _DPCD_REG_00200
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    SINK_COUNT_5_0      :6; // Bits 5:0
        CBIOS_U8    CP_READY            :1;
        CBIOS_U8    SINK_COUNT_6        :1; // Bits 6
    };
}DPCD_REG_00200;

typedef union _DPCD_REG_00201
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    REMOTE_CONTROL_COMMAND_PENDING  :1;
        CBIOS_U8    AUTOMATED_TEST_REQUEST          :1;
        CBIOS_U8    CP_IRQ                          :1;
        CBIOS_U8    MCCS_IRQ                        :1;
        CBIOS_U8    DOWN_REP_MSG_RDY                :1;
        CBIOS_U8    UP_REQ_MSG_RDY                  :1;
        CBIOS_U8    SINK_SPECIFIC_IRQ               :1;
        CBIOS_U8    Reserved0                       :1;
    };
}DPCD_REG_00201;

typedef union _DPCD_REG_00202
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    LANE0_CR_DONE                   :1;
        CBIOS_U8    LANE0_CHANNEL_EQ_DONE           :1;
        CBIOS_U8    LANE0_SYMBOL_LOCKED             :1;
        CBIOS_U8    Reserved0                       :1;
        CBIOS_U8    LANE1_CR_DONE                   :1;
        CBIOS_U8    LANE1_CHANNEL_EQ_DONE           :1;
        CBIOS_U8    LANE1_SYMBOL_LOCKED             :1;
        CBIOS_U8    Reserved1                       :1;
    };
}DPCD_REG_00202;

typedef union _DPCD_REG_00203
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    LANE2_CR_DONE                   :1;
        CBIOS_U8    LANE2_CHANNEL_EQ_DONE           :1;
        CBIOS_U8    LANE2_SYMBOL_LOCKED             :1;
        CBIOS_U8    Reserved0                       :1;
        CBIOS_U8    LANE3_CR_DONE                   :1;
        CBIOS_U8    LANE3_CHANNEL_EQ_DONE           :1;
        CBIOS_U8    LANE3_SYMBOL_LOCKED             :1;
        CBIOS_U8    Reserved1                       :1;
    };
}DPCD_REG_00203;

typedef union _DPCD_REG_00204
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    INTERLANE_ALIGN_DONE            :1;
        CBIOS_U8    Reserved0                       :5;
        CBIOS_U8    DOWNSTREAM_PORT_STATUS_CHANGED  :1;
        CBIOS_U8    LINK_STATUS_UPDATED             :1;
    };
}DPCD_REG_00204;

typedef union _DPCD_REG_00205
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    RECEIVE_PORT_0_STATUS           :1; // 0: SINK out of synchronization
                                                        // 1: SINK in synchronization
        CBIOS_U8    RECEIVE_PORT_1_STATUS           :1;
        CBIOS_U8    Reserved0                       :6;
    };
}DPCD_REG_00205;

typedef union _DPCD_REG_00206 // ADJUST_REQUEST_LANE0_1 
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    VOLTAGE_SWING_LANE0             :2;
        CBIOS_U8    PRE_EMPHASIS_LANE0              :2;
        CBIOS_U8    VOLTAGE_SWING_LANE1             :2;
        CBIOS_U8    PRE_EMPHASIS_LANE1              :2;
    };
}DPCD_REG_00206;

typedef union _DPCD_REG_00207 // ADJUST_REQUEST_LANE2_3 
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    VOLTAGE_SWING_LANE2             :2;
        CBIOS_U8    PRE_EMPHASIS_LANE2              :2;
        CBIOS_U8    VOLTAGE_SWING_LANE3             :2;
        CBIOS_U8    PRE_EMPHASIS_LANE3              :2;
    };
}DPCD_REG_00207;

typedef union _DPCD_REG_0020C // ADJUST_REQUEST_POST_CURSOR2 
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    POST_CURSOR2_LANE0              :2;
        CBIOS_U8    POST_CURSOR2_LANE1              :2;
        CBIOS_U8    POST_CURSOR2_LANE2              :2;
        CBIOS_U8    POST_CURSOR2_LANE3              :2;
    };
}DPCD_REG_0020C;

typedef union _DPCD_REG_00210
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    Error_Count_Bits_7_0            :8;
    };
}DPCD_REG_00210;

typedef union _DPCD_REG_00211
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    Error_Count_Bits_14_8           :7;
        CBIOS_U8    Error_Count_Valid               :1;
    };
}DPCD_REG_00211;

typedef union _DPCD_REG_00218
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_LINK_TRAINING              :1;
        CBIOS_U8    TEST_PATTERN                    :1; 
        CBIOS_U8    TEST_EDID_READ                  :1;
        CBIOS_U8    PHY_TEST_PATTERN                :1; // For DPCD version 1.1
        CBIOS_U8    FAUX_TEST_PATTERN               :1; // For DPCD Version 1.2
        CBIOS_U8    Reserved0                       :3;
    };
}DPCD_REG_00218;

typedef union _DPCD_REG_00219
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_LINK_RATE                  :8;
    };
}DPCD_REG_00219;

typedef union _DPCD_REG_00220
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_LANE_COUNT                 :5;
        CBIOS_U8    Reserved0                       :3;
    };
}DPCD_REG_00220;

typedef union _DPCD_REG_00221
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_PATTERN                :8; // 00h = No test pattern transmitted
                                                    // 01h = color ramps
                                                    // 02h = black and white vertical lines
                                                    // 03h = color square
    };
}DPCD_REG_00221;

typedef union _DPCD_REG_00222
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_H_TOTAL_15_8           :8;
    };
}DPCD_REG_00222;

typedef union _DPCD_REG_00223
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_H_TOTAL_7_0            :8;
    };
}DPCD_REG_00223;

typedef union _DPCD_REG_00224
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_V_TOTAL_15_8           :8;
    };
}DPCD_REG_00224;

typedef union _DPCD_REG_00225
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_V_TOTAL_7_0            :8;
    };
}DPCD_REG_00225;

typedef union _DPCD_REG_00226
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_H_START_15_8           :8;
    };
}DPCD_REG_00226;

typedef union _DPCD_REG_00227
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_H_START_7_0            :8;
    };
}DPCD_REG_00227;

typedef union _DPCD_REG_00228
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_V_START_15_8           :8;
    };
}DPCD_REG_00228;

typedef union _DPCD_REG_00229
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_V_START_7_0            :8;
    };
}DPCD_REG_00229;

typedef union _DPCD_REG_0022A
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_HSYNC_WIDTH_14_8       :7;
        CBIOS_U8    TEST_HSYNC_POLARITY         :1;
    };
}DPCD_REG_0022A;

typedef union _DPCD_REG_0022B
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_HSYNC_WIDTH_7_0        :8;
    };
}DPCD_REG_0022B;

typedef union _DPCD_REG_0022C
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_VSYNC_WIDTH_14_8       :7;
        CBIOS_U8    TEST_VSYNC_POLARITY         :1;
    };
}DPCD_REG_0022C;

typedef union _DPCD_REG_0022D
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_VSYNC_WIDTH_7_0        :8;
    };
}DPCD_REG_0022D;

typedef union _DPCD_REG_0022E
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_H_WIDTH_15_8           :8;
    };
}DPCD_REG_0022E;

typedef union _DPCD_REG_0022F
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_H_WIDTH_7_0            :8;
    };
}DPCD_REG_0022F;

typedef union _DPCD_REG_00230
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_V_WIDTH_15_8           :8;
    };
}DPCD_REG_00230;

typedef union _DPCD_REG_00231
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_V_WIDTH_7_0            :8;
    };
}DPCD_REG_00231;

typedef union _DPCD_REG_00232
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_SYNCHRONOUS_CLOCK      :1; // 0 = Link clock and stream clock asynchronous
                                                    // 1 = Link clock and stream clock synchronous 
        CBIOS_U8    TEST_COLOR_FORMAT           :2; // 00 = RGB 
                                                    // 01 = YCbCr 4:2:2 
                                                    // 10 = YCbCr 4:4:4 
        CBIOS_U8    TEST_DYNAMIC_RANGE          :1; // 0 = VESA range (from 0 to the maximum) 
                                                    // 1 = CEA range (as defined in CEA-861C Section 5) 
        CBIOS_U8    TEST_YCBCR_COEFFICIENTS     :1;
        CBIOS_U8    TEST_BIT_DEPTH              :3; // 000 = 6 bits 
                                                    // 001 = 8 bits
                                                    // 010 = 10 bits 
                                                    // 011 = 12 bits 
                                                    // 100 = 16 bits 
    };
}DPCD_REG_00232;

typedef union _DPCD_REG_00233
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_REFRESH_DENOMINATOR    :1;
        CBIOS_U8    TEST_INTERLACED             :1; 
        CBIOS_U8    Reserved0                   :6;
    };
}DPCD_REG_00233;

typedef union _DPCD_REG_00234
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_REFRESH_RATE_NUMERATOR :8;
    };
}DPCD_REG_00234;

typedef union _DPCD_REG_00260
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_ACK                    :1;
        CBIOS_U8    TEST_NACK                   :1; 
        CBIOS_U8    TEST_EDID_CHECKSUM_WRITE    :1;
        CBIOS_U8    Reserved0                   :5;
    };
}DPCD_REG_00260;

typedef union _DPCD_REG_00261
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    TEST_EDID_CHECKSUM          :8;
    };
}DPCD_REG_00261;

typedef union _DPCD_REG_00600
{
    CBIOS_U8        Value;
    struct
    {
        CBIOS_U8    SET_POWER                   :3; // 1: Set local sink and all downstream sinks to D0 (normal operation mode)
                                                    // 2: Set local sink and all downstream sinks to D3 (power down mode)
                                                    // 5: Set main link for local sink and all downstream sinks to D3 (power down mode),
                                                    //    keep AUX block fully powered, ready to reply within a Response Time-Out period 
                                                    //    of 300µs to Manchester transactions and FAUX Power State 1 or 2 (if FAUX supported) 
        CBIOS_U8    Reserved0                   :2; 
        CBIOS_U8    SET_DN_DEVICE_DP_PWR_5V     :1;
        CBIOS_U8    SET_DN_DEVICE_DP_PWR_12V    :1;
        CBIOS_U8    SET_DN_DEVICE_DP_PWR_18V    :1;
    };
}DPCD_REG_00600;

#endif
