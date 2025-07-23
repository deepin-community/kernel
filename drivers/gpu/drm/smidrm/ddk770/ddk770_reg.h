
#define DP_BASE                 0x00190000

#define DP_OFFSET               0x8000

#define DP1_BASE                DP_BASE+DP_OFFSET

#define DP_PIN                  0x0000
#define DP_PIN_SYS_RSTN         0:0
#define DP_PIN_TEST_SEL         0:0
#define DP_PIN_TEST_EN          0:0
#define DP_PIN_TEST_DONE        0:0
#define DP_PIN_TEST_PASS        0:0

#define DP_TOP18                                          0x0018
#define DP_TOP18_SCRAMBLE                       0:0
#define DP_TOP18_SCRAMBLE_ENABLE                       0
#define DP_TOP18_SCRAMBLE_DISABLE                      1
#define DP_TOP18_ENHANCE_FRAMING                        1:1
#define DP_TOP18_ENHANCE_FRAMING_ENABLE                 1
#define DP_TOP18_ENHANCE_FRAMING_DISABLE                0
#define DP_TOP18_REG_HPD_SCALE                             3:3
#define DP_TOP18_REG_HPD_SCALE_NORMAL                      0
#define DP_TOP18_REG_HPD_SCALE_SCALING                     1

#define DP_TOP1C                                  0x001C
#define DP_TOP1C_REG_RESET_AUDIO                  3:3
#define DP_TOP1C_REG_RESET_AUDIO_TRUE                  1
#define DP_TOP1C_REG_RESET_AUDIO_FALSE                  0
#define DP_TOP1C_REG_RESET_PHY                    1:1
#define DP_TOP1C_REG_RESET_PHY_TRUE                    1
#define DP_TOP1C_REG_RESET_PHY_FALSE                    0

#define DP_TOP80                              0x0080
#define DP_TOP80_STA_AUD_OVERRUN                5:5
#define DP_TOP80_STA_AUD_OVERRUN_TRUE                1
#define DP_TOP80_STA_AUD_OVERRUN_FALSE                0
#define DP_TOP80_STA_AUX_REPLY_EVENT          1:1
#define DP_TOP80_STA_AUX_REPLY_EVENT_TRUE     1
#define DP_TOP80_STA_AUX_REPLY_EVENT_FALSE    0
#define DP_TOP80_STA_HPD_EVENT                0:0
#define DP_TOP80_STA_HPD_EVENT_TRUE                1
#define DP_TOP80_STA_HPD_EVENT_FALSE                0

#define DP_TOP84                              0x0084

#define DP_TOP84_ENABLE_AUD_OVERRUN                5:5
#define DP_TOP84_ENABLE_AUD_OVERRUN_TRUE           1
#define DP_TOP84_ENABLE_AUD_OVERRUN_FALSE          0

#define DP_TOP84_ENABLE_HPD_UNPLUG_EVENT        2:2
#define DP_TOP84_ENABLE_HPD_UNPLUG_EVENT_TRUE        1
#define DP_TOP84_ENABLE_HPD_UNPLUG_EVENT_FALSE       0

#define DP_TOP84_ENABLE_AUX_REPLY_EVENT       1:1
#define DP_TOP84_ENABLE_AUX_REPLY_EVENT_TRUE       1
#define DP_TOP84_ENABLE_AUX_REPLY_EVENT_FALSE       0
#define DP_TOP84_ENABLE_HPD_PLUG_EVENT        0:0
#define DP_TOP84_ENABLE_HPD_PLUG_EVENT_TRUE        1
#define DP_TOP84_ENABLE_HPD_PLUG_EVENT_FALSE       0

#define DP_TOP88        0x0088
#define DP_TOP88_STA_HPD_UNPLUG  2:2
#define DP_TOP88_STA_HPD_UNPLUG_TRUE  1
#define DP_TOP88_STA_HPD_UNPLUG_FALSE  0
#define DP_TOP88_STA_HPD_PLUG  1:1
#define DP_TOP88_STA_HPD_PLUG_TRUE  1
#define DP_TOP88_STA_HPD_PLUG_FALSE  0

#define DP_TOP8C             0x008C
#define DP_TOP8C_HPD_UNPLUG      2:2
#define DP_TOP8C_HPD_UNPLUG_ENABLE      1
#define DP_TOP8C_HPD_UNPLUG_DISABLE     0
#define DP_TOP8C_HPD_PLUG      1:1
#define DP_TOP8C_HPD_PLUG_ENABLE      1
#define DP_TOP8C_HPD_PLUG_DISABLE     0
#define DP_TOP8C_HPD_IRQ      0:0
#define DP_TOP8C_HPD_IRQ_ENABLE      1
#define DP_TOP8C_HPD_IRQ_DISABLE     0

#define DP_PHY100                  0x0100
#define DP_PHY100_DP_PHY_RATE           28:26
#define DP_PHY100_DP_PHY_RATE_216            1
#define DP_PHY100_DP_PHY_RATE_243            2
#define DP_PHY100_DP_PHY_RATE_OTHER          0
#define DP_PHY100_TRANSMIT_ENABLE  11:8
#define DP_PHY100_TRANSMIT_ENABLE_NONE   0
#define DP_PHY100_TRANSMIT_ENABLE_LANE0  1  
#define DP_PHY100_TRANSMIT_ENABLE_LANE1  2
#define DP_PHY100_TRANSMIT_ENABLE_LANE2  4
#define DP_PHY100_TRANSMIT_ENABLE_LANE3  8
#define DP_PHY100_TRANSMIT_ENABLE_ALL    15
#define DP_PHY100_PHY_LANES        7:6
#define DP_PHY100_PHY_LANES_1      0
#define DP_PHY100_PHY_LANES_2      1
#define DP_PHY100_PHY_LANES_4      2
#define DP_PHY100_PHY_RATE            5:4
#define DP_PHY100_PHY_RATE_810            3
#define DP_PHY100_PHY_RATE_540            2
#define DP_PHY100_PHY_RATE_270            1
#define DP_PHY100_PHY_RATE_162            0
#define DP_PHY100_TPS_SEL          3:0
#define DP_PHY100_TPS_SEL_NO          0
#define DP_PHY100_TPS_SEL_TPS1          1
#define DP_PHY100_TPS_SEL_TPS2          2
#define DP_PHY100_TPS_SEL_TPS3          3
#define DP_PHY100_TPS_SEL_TPS4          4

#define DP_PHY108               	        0x108  //DP_MAIN_LINK_CHANNEL_CODING_SET
#define DP_DP_PHY108_SET_ANSI_8B10B		    (1 << 0)
#define DP_DP_PHY108_SET_ANSI_128B132B      (1 << 1)

#define DP_ANALOG180     0x0180
#define DP_ANALOG180_COREPLL_FBDIV 31:24
#define DP_ANALOG180_SSCG 21:21
#define DP_ANALOG180_SSCG_ENABLE 0
#define DP_ANALOG180_SSCG_DISABLE 1
#define DP_ANALOG180_DOWNSPREAD 20:20
#define DP_ANALOG180_DOWNSPREAD_DOWN 1
#define DP_ANALOG180_DOWNSPREAD_CENTER 0
#define DP_ANALOG180_DA_COREPLL_FBDIV 19:16
#define DP_ANALOG180_DA_COREPLL_PREDIV 13:8
#define DP_ANALOG180_DA_COREPLL_LOCK   7:7
#define DP_ANALOG180_DA_COREPLL_LOCK_TRUE   1
#define DP_ANALOG180_DA_COREPLL_LOCK_FALSE   0
#define DP_ANALOG180_DA_COREPLL_FRAC_PD 5:4
#define DP_ANALOG180_COREPLL_PD 0:0
#define DP_ANALOG180_COREPLL_PD_TRUE 1
#define DP_ANALOG180_COREPLL_PD_FALSE 0

#define DP_ANALOG184   0X184
#define DP_ANALOG184_REG_COREPLL_FRAC    23:0

#define DP_ANALOG188     0X188
#define DP_ANALOG188_REG_SPREAD 30:28
#define DP_ANALOG188_REG_DIVVAL 27:24
#define DP_ANALOG188_DA_COREPLL_POSTDIV 3:2

#define DP_ANALOG190    0X190
#define DP_ANALOG190_DA_PIXELPLL_FBDIV1 31:24
#define DP_ANALOG190_DA_PIXELPLL_FBDIV0 19:16
#define DP_ANALOG190_DA_PIXELPLL_PREDIV 13:8
#define DP_ANALOG190_DA_PIXELPLL_LOCK 7:7
#define DP_ANALOG190_DA_PIXELPLL_LOCK_TRUE  1
#define DP_ANALOG190_DA_PIXELPLL_LOCK_FALSE 0
#define DP_ANALOG190_DA_PIXELPLL_FRAC_PD 5:4
#define DP_ANALOG190_REG_PIXELPLL_PD 0:0
#define DP_ANALOG190_REG_PIXELPLL_PD_TRUE 1
#define DP_ANALOG190_REG_PIXELPLL_PD_FALSE 0

#define DP_ANALOG194    0X194
#define DP_ANALOG194_PIXEL_PLL_DIVIDER_C 20:16
#define DP_ANALOG194_PIXEL_PLL_DIVIDER_B 9:8
#define DP_ANALOG194_PIXEL_PLL_DIVIDER_A 4:0

#define DP_ANALOG19C    0X19C
#define DP_ANALOG19C_DA_PIXELPLL_FRAC0  23:16
#define DP_ANALOG19C_DA_PIXELPLL_FRAC1  15:8
#define DP_ANALOG19C_DA_PIXELPLL_FRAC2  7:0

#define DP_ANALOG1A4    0X1a4
#define DP_ANALOG1A4_DA_TX_ISEL_DRV_D3 31:28
#define DP_ANALOG1A4_DA_TX_ISEL_DRV_D2 27:24

#define DP_ANALOG1A8 	0X1a8
#define DP_ANALOG1A8_DA_TX_MAINSEL_D2    28:24
#define DP_ANALOG1A8_DA_TX_MAINSEL_D3    20:16
#define DP_ANALOG1A8_DA_TX_MAINSEL_D1    7:4
#define DP_ANALOG1A8_DA_TX_MAINSEL_D0    3:0

#define DP_ANALOG1AC 	0X1ac
#define DP_ANALOG1AC_DA_TX_POSTSEL_D1  31:28
#define DP_ANALOG1AC_DA_TX_POSTSEL_D0  27:24
#define DP_ANALOG1AC_DA_TX_POSTSEL_D3  23:20
#define DP_ANALOG1AC_DA_TX_POSTSEL_D2  19:16
#define DP_ANALOG1AC_DA_TX_MAINSEL_D0  12:8
#define DP_ANALOG1AC_DA_TX_MAINSEL_D1  4:0

#define DP_ANALOG1B0 	0x1b0
#define DP_ANALOG1B0_DA_TX_PRESEL_D1 14:12
#define DP_ANALOG1B0_DA_TX_PRESEL_D0 10:8
#define DP_ANALOG1B0_DA_TX_PRESEL_D3 6:4
#define DP_ANALOG1B0_DA_TX_PRESEL_D2 2:0

#define DP_ANALOG1C0 	0x1c0
#define DP_ANALOG1C0_REG_BG_RCAL_SEL    26:25
#define DP_ANALOG1C0_REG_RTCAL_FREQDIV0  23:16
#define DP_ANALOG1C0_REG_RTCAL_BYPASS   15:15
#define DP_ANALOG1C0_REG_RTCAL_FREQDIV1  14:8
#define DP_ANALOG1C0_REG_BG_RCAL_VAL    5:0

#define DP_ANALOG1C4    0x1c4
#define DP_ANALOG1C4_REG_TX_RTM_D3 29:24
#define DP_ANALOG1C4_REG_TX_RTM_D2 21:16
#define DP_ANALOG1C4_REG_TX_RTM_D1 13:8
#define DP_ANALOG1C4_REG_TX_RTM_D0 5:0

#define DP_CONTROLLER200 0x0200
#define DP_CONTROLLER200_REG_ALT_SCREAMBLE_RESET    24:24
#define DP_CONTROLLER200_REG_ALT_SCREAMBLE_RESET_TRUE    1
#define DP_CONTROLLER200_REG_ALT_SCREAMBLE_RESET_FALSE   0
#define DP_CONTROLLER200_VIDEO_MAPPING              20:16
#define DP_CONTROLLER200_VIDEO_MAPPING_RGB6             0
#define DP_CONTROLLER200_VIDEO_MAPPING_RGB8             1
#define DP_CONTROLLER200_VIDEO_MAPPING_RGB10            2
#define DP_CONTROLLER200_VIDEO_MAPPING_RGB12            3
#define DP_CONTROLLER200_VIDEO_MAPPING_RGB16            4
#define DP_CONTROLLER200_VIDEO_MAPPING_YCBCR444_8       5 
#define DP_CONTROLLER200_VIDEO_MAPPING_YCBCR444_10            6
#define DP_CONTROLLER200_VIDEO_MAPPING_YCBCR444_12           7
#define DP_CONTROLLER200_VIDEO_MAPPING_YCBCR444_16           8
#define DP_CONTROLLER200_VIDEO_MAPPING_YCBCR422_8       9 
#define DP_CONTROLLER200_VIDEO_MAPPING_YCBCR422_10       10     
#define DP_CONTROLLER200_VIDEO_MAPPING_YCBCR422_12          11
#define DP_CONTROLLER200_VIDEO_MAPPING_YCBCR422_16           12
#define DP_CONTROLLER200_VIDEO_STREAM                  5:5
#define DP_CONTROLLER200_VIDEO_STREAM_ENABLE            1
#define DP_CONTROLLER200_VIDEO_STREAM_DISABLE           0

#define DP_CONTROLLER20C  0x020C
#define DP_CONTROLLER20C_REG_TX_POLARITY
#define DP_CONTROLLER20C_REG_TX_POLARITY_HSYNC 1:1
#define DP_CONTROLLER20C_REG_TX_POLARITY_HSYNC_HIGH 1
#define DP_CONTROLLER20C_REG_TX_POLARITY_HSYNC_LOW 1
#define DP_CONTROLLER20C_REG_TX_POLARITY_VSYNC 0:0
#define DP_CONTROLLER20C_REG_TX_POLARITY_VSYNC_HIGH 1
#define DP_CONTROLLER20C_REG_TX_POLARITY_VSYNC_LOW 1

#define DP_CONTROLLER210  0x0210
#define DP_CONTROLLER210_REG_TX_HACTIVE  31:16
#define DP_CONTROLLER210_REG_TX_HBLANK   15:2

#define DP_CONTROLLER214  0x0214
#define DP_CONTROLLER214_REG_TX_VBLANK  31:16
#define DP_CONTROLLER214_REG_TX_VACTIVE 15:0

#define DP_CONTROLLER218  0x0218
#define DP_CONTROLLER218_REG_TX_HSWIDTH 31:16
#define DP_CONTROLLER218_REG_TX_H_FRONT_PORCH   15:0

#define DP_CONTROLLER21C  0x021C
#define DP_CONTROLLER21C_REG_TX_VSWIDTH 31:16
#define DP_CONTROLLER21C_REG_TX_V_FRONT_PORCH 15:0

#define DP_CONTROLLER220  0x0220
#define DP_CONTROLLER220_REG_AVG_PER_TU_FRAC  19:16
#define DP_CONTROLLER220_REG_LINE_RD_THRES    13:7
#define DP_CONTROLLER220_REG_AVG_PER_TU_INT   6:0

#define DP_CONTROLLER224  0x0224
#define DP_CONTROLLER224_REG_TX_VSTART 31:16
#define DP_CONTROLLER224_REG_TX_HSTART 15:0

#define DP_CONTROLLER228  0x0228
#define DP_CONTROLLER228_REG_TX_MSA_MISC0 31:24

#define DP_CONTROLLER22C  0x022C
#define DP_CONTROLLER22C_REG_TX_MSA_MISC1 31:24


#define DP_CONTROLLER230  0x0230
#define DP_CONTROLLER230_HBLANK_LINK_CYC 15:0

#define DP_CONTROLLER300  0x0300
#define DP_CONTROLLER300_AUDIO_MUTE   15:15
#define DP_CONTROLLER300_AUDIO_MUTE_SET   1
#define DP_CONTROLLER300_AUDIO_MUTE_CLEAR   0
#define DP_CONTROLLER300_AUDIO_CHANNEL_NUM    14:12
#define DP_CONTROLLER300_AUDIO_CHANNEL_NUM_1   0x0
#define DP_CONTROLLER300_AUDIO_CHANNEL_NUM_2   0x1
#define DP_CONTROLLER300_AUDIO_CHANNEL_NUM_8   0x8
#define DP_CONTROLLER300_AUDIO_LEGEND         11:11
#define DP_CONTROLLER300_AUDIO_LEGEND_VALID   0x0       // i2s stream format is legend, only include valid audio data
#define DP_CONTROLLER300_AUDIO_LEGEND_ALL     0x1       // change input i2s stream format, include PR, P, C, U, V��valid audio data
#define DP_CONTROLLER300_AUDIO_DATA_WIDTH     9:5
#define DP_CONTROLLER300_AUDIO_DATA_WIDTH_16     0X10
#define DP_CONTROLLER300_AUDIO_DATA_WIDTH_20     0X14
#define DP_CONTROLLER300_AUDIO_DATA_WIDTH_24     0X18
#define DP_CONTROLLER300_AUDIO_DATA_EN             4:1
#define DP_CONTROLLER300_AUDIO_DATA_EN_0   0x0
#define DP_CONTROLLER300_AUDIO_DATA_EN_12  0x1
#define DP_CONTROLLER300_AUDIO_DATA_EN_34  0x2
#define DP_CONTROLLER300_AUDIO_DATA_EN_56  0x4
#define DP_CONTROLLER300_AUDIO_DATA_EN_78  0x8
#define DP_CONTROLLER300_AUDIO_DATA_EN_ALL 0xf
#define DP_CONTROLLER300_AUDIO_INTERFACE_SEL  0:0
#define DP_CONTROLLER300_AUDIO_INTERFACE_SEL_I2S    0
#define DP_CONTROLLER300_AUDIO_INTERFACE_SEL_SPDIF  1

#define DP_CONTROLLER304  0x0304
#define DP_CONTROLLER304_AUDSET  15:8

#define DP_CONTROLLER308  0x0308
#define DP_CONTROLLER308_AUDSET  15:8

#define DP_CONTROLLER30C  0x030c
#define DP_CONTROLLER30C_AUDSET1 5:0
#define DP_CONTROLLER30C_AUDSET2 11:8


#define DP_CONTROLLER310  0x0310
#define DP_CONTROLLER310_AUDSET  7:0

#define DP_CONTROLLER400  0x0400
#define DP_CONTROLLER400_CFG_PHY_AUX_DATA_CMD 31:28
#define DP_CONTROLLER400_CFG_PHY_AUX_DATA_ADDR 27:8
#define DP_CONTROLLER400_CFG_PHY_AUX_DATA_LENGTH   3:0

#define DP_CONTROLLER404  0x0404
#define DP_CONTROLLER404_AUX_TIMEOUT  17:17
#define DP_CONTROLLER404_AUX_TIMEOUT_TRUE  1
#define DP_CONTROLLER404_AUX_TIMEOUT_FALSE 0
#define DP_CONTROLLER404_AUX_REPLY_RECEIVED  16:16
#define DP_CONTROLLER404_AUX_REPLY_RECEIVED_TRUE  0
#define DP_CONTROLLER404_AUX_REPLY_RECEIVED_FALSE  1
#define DP_CONTROLLER404_AUX_REPLY_CMD  7:4
#define DP_CONTROLLER404_AUX_REPLY_CMD_ACK  0
#define DP_CONTROLLER404_AUX_REPLY_CMD_NOACK 1

#define DP_CONTROLLER408  0x0408

#define DP_CONTROLLER40C  0x040C

#define DP_CONTROLLER410  0x0410

#define DP_CONTROLLER414  0x0414

#define DP_CONTROLLER418  0x0418
#define DP_CONTROLLER418_CFG_PHY_AUX_START 0:0
#define DP_CONTROLLER418_CFG_PHY_AUX_START_TRUE 1
#define DP_CONTROLLER418_CFG_PHY_AUX_START_FALSE 0

#define DP_CONTROLLER500  0x0500
#define DP_CONTROLLER500_SDP_AUDIO_INFO_FRAME  3:3
#define DP_CONTROLLER500_SDP_AUDIO_INFO_FRAME_ENABLE  1
#define DP_CONTROLLER500_SDP_AUDIO_INFO_FRAME_DISABLE 0
#define DP_CONTROLLER500_SDP_AUDIO_STREAM_VERTICAL  1:1
#define DP_CONTROLLER500_SDP_AUDIO_STREAM_VERTICAL_ENABLE  1
#define DP_CONTROLLER500_SDP_AUDIO_STREAM_VERTICAL_DISABLE 0
#define DP_CONTROLLER500_SDP_AUDIO_TIMESTAMP_VERTICAL 0:0
#define DP_CONTROLLER500_SDP_AUDIO_TIMESTAMP_VERTICAL_ENABLE 1
#define DP_CONTROLLER500_SDP_AUDIO_TIMESTAMP_VERTICAL_DISABLE 0

#define DP_CONTROLLER504  0x0504
#define DP_CONTROLLER504_SDP_AUDIO_STREAM_HORIZONTAL 1:1
#define DP_CONTROLLER504_SDP_AUDIO_STREAM_HORIZONTAL_ENABLE 1
#define DP_CONTROLLER504_SDP_AUDIO_STREAM_HORIZONTAL_DISABLE 0
#define DP_CONTROLLER504_SDP_AUDIO_TIMESTAMP_HORIZONTAL 0:0
#define DP_CONTROLLER504_SDP_AUDIO_TIMESTAMP_HORIZONTAL_ENABLE 1
#define DP_CONTROLLER504_SDP_AUDIO_TIMESTAMP_HORIZONTAL_DISABLE 0

/*
`define apbCfg_REGISTER_ADDR_0508                   16'h0508,   5'd31 ,      5'd0
`define apbCfg_SDP_STATUS                           16'h0508,   5'd19,      5'd0
`define apbCfg_REGISTER_ADDR_0510                   16'h0510,   5'd31 ,      5'd0
`define apbCfg_SDP_STATUS_ENABLE                    16'h0510,   5'd19,      5'd0
`define apbCfg_REGISTER_ADDR_050C                   16'h050C,   5'd31 ,      5'd0
`define apbCfg_SDP_MANUAL_CTRL_MANUAL_MODE          16'h050C,   5'd19,      5'd0

`define apbCfg_SDP_STATUS_VSC_HB                    16'h0624,   5'd31,      5'd0
`define apbCfg_SDP_STATUS_VSC_PAYLOAD0              16'h0628,   5'd31,      5'd0
`define apbCfg_SDP_STATUS_VSC_PAYLOAD1              16'h062c,   5'd31,      5'd0
`define apbCfg_SDP_STATUS_VSC_PAYLOAD2              16'h0630,   5'd31,      5'd0
`define apbCfg_SDP_STATUS_VSC_PAYLOAD3              16'h0634,   5'd31,      5'd0
`define apbCfg_SDP_STATUS_VSC_PAYLOAD4              16'h0638,   5'd31,      5'd0
*/

#define DP_CONTROLLER50C 			0x050C
#define DP_CONTROLLER50C_SDP_MANUAL_CTRL_MANUAL_MODE 19:0

#define DP_CONTROLLER624          	0x0624
#define DP_CONTROLLER624_VSC_HB        31:0
#define DP_CONTROLLER628   		0x0628
#define DP_CONTROLLER628_VSC_PAYLOAD0        31:0
#define DP_CONTROLLER62C   		0x062C
#define DP_CONTROLLER62C_VSC_PAYLOAD1        31:0
#define DP_CONTROLLER630   		0x0630
#define DP_CONTROLLER630_VSC_PAYLOAD2        31:0
#define DP_CONTROLLER634   		0x0634
#define DP_CONTROLLER634_VSC_PAYLOAD3        31:0
#define DP_CONTROLLER638   		0x0638
#define DP_CONTROLLER638_VSC_PAYLOAD4        31:0
#define DP_CONTROLLER63C   		0x063c
#define DP_CONTROLLER63C_VSC_PAYLOAD5        31:0
#define DP_CONTROLLER640   		0x0640
#define DP_CONTROLLER640_VSC_PAYLOAD6        31:0
#define DP_CONTROLLER644   		0x0644
#define DP_CONTROLLER644_VSC_PAYLOAD7        31:0

#define SCRATCH_PAD0                                0x000060

#define SCRATCH_PAD1                                0x000064

#ifdef __ICCARM__
#define CLOCK_ENABLE                                (0x000068 + MMIO_BASE)
#else
#define CLOCK_ENABLE                                0x0068
#endif
#define CLOCK_ENABLE_DMA1R                          31:31
#define CLOCK_ENABLE_DMA1R_NORMAL                   0
#define CLOCK_ENABLE_DMA1R_RESET                    1
#define CLOCK_ENABLE_JPU1R                          30:30
#define CLOCK_ENABLE_JPU1R_NORMAL                   0
#define CLOCK_ENABLE_JPU1R_RESET                    1
#define CLOCK_ENABLE_HDMIR                          29:29
#define CLOCK_ENABLE_HDMIR_NORMAL                   0
#define CLOCK_ENABLE_HDMIR_RESET                    1
#define CLOCK_ENABLE_DDRR                          28:28
#define CLOCK_ENABLE_DDRR_NORMAL                   0
#define CLOCK_ENABLE_DDRR_RESET                    1
#define CLOCK_ENABLE_USBSR                          27:27
#define CLOCK_ENABLE_USBSR_NORMAL                   0
#define CLOCK_ENABLE_USBSR_RESET                    1
#define CLOCK_ENABLE_DC2R                           26:26
#define CLOCK_ENABLE_DC2R_NORMAL                    0
#define CLOCK_ENABLE_DC2R_RESET                     1
#define CLOCK_ENABLE_JPUR                           25:25
#define CLOCK_ENABLE_JPUR_NORMAL                    0
#define CLOCK_ENABLE_JPUR_RESET                     1
#define CLOCK_ENABLE_VPUR                           24:24
#define CLOCK_ENABLE_VPUR_NORMAL                    0
#define CLOCK_ENABLE_VPUR_RESET                     1
#define CLOCK_ENABLE_DER                            23:23
#define CLOCK_ENABLE_DER_NORMAL                     0
#define CLOCK_ENABLE_DER_RESET                      1
#define CLOCK_ENABLE_DMAR                           22:22
#define CLOCK_ENABLE_DMAR_NORMAL                    0
#define CLOCK_ENABLE_DMAR_RESET                     1
#define CLOCK_ENABLE_UARTR                          21:21
#define CLOCK_ENABLE_UARTR_NORMAL                   0
#define CLOCK_ENABLE_UARTR_RESET                    1
#define CLOCK_ENABLE_I2SR                           20:20
#define CLOCK_ENABLE_I2SR_NORMAL                    0
#define CLOCK_ENABLE_I2SR_RESET                     1
#define CLOCK_ENABLE_SSPR                           19:19
#define CLOCK_ENABLE_SSPR_NORMAL                    0
#define CLOCK_ENABLE_SSPR_RESET                     1
#define CLOCK_ENABLE_DC1R                           18:18
#define CLOCK_ENABLE_DC1R_NORMAL                    0
#define CLOCK_ENABLE_DC1R_RESET                     1
#define CLOCK_ENABLE_DC0R                           17:17
#define CLOCK_ENABLE_DC0R_NORMAL                    0
#define CLOCK_ENABLE_DC0R_RESET                     1
#define CLOCK_ENABLE_ARMR                           16:16
#define CLOCK_ENABLE_ARMR_NORMAL                    0
#define CLOCK_ENABLE_ARMR_RESET                     1
#define CLOCK_ENABLE_DMA1                           15:15
#define CLOCK_ENABLE_DMA1_OFF                       0
#define CLOCK_ENABLE_DMA1_ON                        1
#define CLOCK_ENABLE_JPU1                           14:14
#define CLOCK_ENABLE_JPU1_OFF                       0
#define CLOCK_ENABLE_JPU1_ON                        1
#define CLOCK_ENABLE_HDMI                           13:13
#define CLOCK_ENABLE_HDMI_OFF                       0
#define CLOCK_ENABLE_HDMI_ON                        1
#define CLOCK_ENABLE_USBS                           11:11
#define CLOCK_ENABLE_USBS_OFF                       0
#define CLOCK_ENABLE_USBS_ON                        1
#define CLOCK_ENABLE_DC2                            10:10
#define CLOCK_ENABLE_DC2_OFF                        0
#define CLOCK_ENABLE_DC2_ON                         1
#define CLOCK_ENABLE_JPU                            9:9
#define CLOCK_ENABLE_JPU_OFF                        0
#define CLOCK_ENABLE_JPU_ON                         1
#define CLOCK_ENABLE_VPU                            8:8
#define CLOCK_ENABLE_VPU_OFF                        0
#define CLOCK_ENABLE_VPU_ON                         1
#define CLOCK_ENABLE_DE                             7:7
#define CLOCK_ENABLE_DE_OFF                         0
#define CLOCK_ENABLE_DE_ON                          1
#define CLOCK_ENABLE_DMA                            6:6
#define CLOCK_ENABLE_DMA_OFF                        0
#define CLOCK_ENABLE_DMA_ON                         1
#define CLOCK_ENABLE_UART                           5:5
#define CLOCK_ENABLE_UART_OFF                       0
#define CLOCK_ENABLE_UART_ON                        1
#define CLOCK_ENABLE_I2S                            4:4
#define CLOCK_ENABLE_I2S_OFF                        0
#define CLOCK_ENABLE_I2S_ON                         1
#define CLOCK_ENABLE_SSP                            3:3
#define CLOCK_ENABLE_SSP_OFF                        0
#define CLOCK_ENABLE_SSP_ON                         1
#define CLOCK_ENABLE_DC1                            2:2
#define CLOCK_ENABLE_DC1_OFF                        0
#define CLOCK_ENABLE_DC1_ON                         1
#define CLOCK_ENABLE_DC0                            1:1
#define CLOCK_ENABLE_DC0_OFF                        0
#define CLOCK_ENABLE_DC0_ON                         1
#define CLOCK_ENABLE_ARM                            0:0
#define CLOCK_ENABLE_ARM_OFF                        0
#define CLOCK_ENABLE_ARM_ON                         1

#define CLOCK1_ENABLE                               0x0140
#define CLOCK1_ENABLE_PCIER                          30:30
#define CLOCK1_ENABLE_PCIER_NORMAL                   0
#define CLOCK1_ENABLE_PCIER_RESET                    1

#define CLOCK1_ENABLE_EFUSER                          27:27
#define CLOCK1_ENABLE_EFUSER_NORMAL                   0
#define CLOCK1_ENABLE_EFUSER_RESET                    1
#define CLOCK1_ENABLE_LVDSPHYR                           26:26
#define CLOCK1_ENABLE_LVDSPHYR_NORMAL                    0
#define CLOCK1_ENABLE_LVDSPHYR_RESET                     1
#define CLOCK1_ENABLE_LPDDR4PHYR							25:25
#define CLOCK1_ENABLE_LPDDR4PHYR_NORMAL                    0
#define CLOCK1_ENABLE_LPDDR4PHYR_RESET                     1
#define CLOCK1_ENABLE_LPDDR4CTLR                           24:24
#define CLOCK1_ENABLE_LPDDR4CTLR_NORMAL                    0
#define CLOCK1_ENABLE_LPDDR4CTLR_RESET                     1
#define CLOCK1_ENABLE_DP1R                           23:23
#define CLOCK1_ENABLE_DP1R_NORMAL                     0
#define CLOCK1_ENABLE_DP1R_RESET                    1
#define CLOCK1_ENABLE_DP0R                           22:22
#define CLOCK1_ENABLE_DP0R_NORMAL                     0
#define CLOCK1_ENABLE_DP0R_RESET                    1
#define CLOCK1_ENABLE_HDCP0R                           21:21
#define CLOCK1_ENABLE_HDCP0R_NORMAL                     0
#define CLOCK1_ENABLE_HDCP0R_RESET                    1

#define CLOCK1_ENABLE_HDMI2R                           20:20
#define CLOCK1_ENABLE_HDMI2R_NORMAL                     0
#define CLOCK1_ENABLE_HDMI2R_RESET                    1
#define CLOCK1_ENABLE_HDMI1R                           19:19
#define CLOCK1_ENABLE_HDMI1R_NORMAL                     0
#define CLOCK1_ENABLE_HDMI1R_RESET                    1
#define CLOCK1_ENABLE_HDMI0R                           18:18
#define CLOCK1_ENABLE_HDMI0R_NORMAL                     0
#define CLOCK1_ENABLE_HDMI0R_RESET                    1

#define CLOCK1_ENABLE_JPU3R                           17:17
#define CLOCK1_ENABLE_JPU3R_NORMAL                     0
#define CLOCK1_ENABLE_JPU3R_RESET                    1
#define CLOCK1_ENABLE_JPU2R                           16:16
#define CLOCK1_ENABLE_JPU2R_NORMAL                     0
#define CLOCK1_ENABLE_JPU2R_RESET                    1

#define CLOCK1_ENABLE_PCIE                            14:14
#define CLOCK1_ENABLE_PCIE_OFF                        0
#define CLOCK1_ENABLE_PCIE_ON                         1

#define CLOCK1_ENABLE_LPDDR4                            8:8
#define CLOCK1_ENABLE_LPDDR4_OFF                        0
#define CLOCK1_ENABLE_LPDDR4_ON                         1
#define CLOCK1_ENABLE_DP1                            7:7
#define CLOCK1_ENABLE_DP1_OFF                        0
#define CLOCK1_ENABLE_DP1_ON                         1
#define CLOCK1_ENABLE_DP0                             6:6
#define CLOCK1_ENABLE_DP0_OFF                         0
#define CLOCK1_ENABLE_DP0_ON                          1

#define CLOCK1_ENABLE_HDCP0                             5:5
#define CLOCK1_ENABLE_HDCP0_OFF                         0
#define CLOCK1_ENABLE_HDCP0_ON                          1


#define CLOCK1_ENABLE_HDMI2                             4:4
#define CLOCK1_ENABLE_HDMI2_OFF                         0
#define CLOCK1_ENABLE_HDMI2_ON                          1
#define CLOCK1_ENABLE_HDMI1                             3:3
#define CLOCK1_ENABLE_HDMI1_OFF                         0
#define CLOCK1_ENABLE_HDMI1_ON                          1
#define CLOCK1_ENABLE_HDMI0                             2:2
#define CLOCK1_ENABLE_HDMI0_OFF                         0
#define CLOCK1_ENABLE_HDMI0_ON                          1

#define CLOCK1_ENABLE_JPU3                            1:1
#define CLOCK1_ENABLE_JPU3_OFF                         0
#define CLOCK1_ENABLE_JPU3_ON                          1
#define CLOCK1_ENABLE_JPU2                             0:0
#define CLOCK1_ENABLE_JPU2_OFF                         0
#define CLOCK1_ENABLE_JPU2_ON                          1



/* No bit fields for VGA PLL since DDK don't use it */
#define VGA25PLL                                    0x00006C
#define VGA28PLL                                    0x000070

/* Master clock for DDR and core */
#define MCLK_PLL                                     0x000074
#define MCLK_PLL_VCO                                23:22
#define MCLK_PLL_INT                                21:16
#define MCLK_PLL_POWER                                0:0
#define MCLK_PLL_POWER_NORMAL                        0
#define MCLK_PLL_POWER_DOWN                            1

#define PLL3_SET                              	0x000230
#define PLL3_SET_PLL3PD                     	10:10
#define PLL3_SET_PLL3PD_OFF						0
#define PLL3_SET_PLL3PD_ON                 		1

#define PLL_SET                              	0x000228
#define PLL_SET_VCLK0PD                     	16:16
#define PLL_SET_VCLK0PD_OFF						0
#define PLL_SET_VCLK0PD_ON                 	1
#define PLL_SET_VCLK1PD                     	28:28
#define PLL_SET_VCLK1PD_OFF						0
#define PLL_SET_VCLK1PD_ON                 	1

#if 0
#define PLL_SET_PLL3PD                     	24:24
#define PLL_SET_PLL3PD_OFF						0
#define PLL_SET_PLL3PD_ON                 		1
#endif

#define PLL_SET_PLL2PD                     	20:20
#define PLL_SET_PLL2PD_OFF						0
#define PLL_SET_PLL2PD_ON                 		1

#define PLL_SET_VCLK0LOCK                     	13:13
#define PLL_SET_VCLK0LOCK_ON					1
#define PLL_SET_VCLK0LOCK_OFF                 	0

#define PLL_SET_PLL2LOCK                     	7:7
#define PLL_SET_PLL2LOCK_ON						1
#define PLL_SET_PLL2LOCK_OFF                 	0

#define PLL_SET_PLL3LOCK                     	4:4
#define PLL_SET_PLL3LOCK_ON						1
#define PLL_SET_PLL3LOCK_OFF                 	0

#define PLL_SET_VCLK1LOCK                     	2:2
#define PLL_SET_VCLK1LOCK_ON					1
#define PLL_SET_VCLK1LOCK_OFF                 	0
#define PLL_SET_VCLK2LOCK                     	0:0
#define PLL_SET_VCLK2LOCK_ON					1
#define PLL_SET_VCLK2LOCK_OFF                 	0

#define PLL_SET1                              0x00022C
#define PLL_SET1_VCLK0CP					  	29:28
#define PLL_SET1_VCLK2PD                     	0:0
#define PLL_SET1_VCLK2PD_OFF					0
#define PLL_SET1_VCLK2PD_ON                 	1

/* Video clock 0  */
#define VCLK_PLL                            0x000230
#define VCLK_PLL_SSCSEL                     30:30
#define VCLK_PLL_SSCSEL_XTAL 				 0
#define VCLK_PLL_SSCSEL_SSCG				 1
#define VCLK_PLL_PRESEL                     27:27
#define VCLK_PLL_PRESEL_250                 0
#define VCLK_PLL_PRESEL_125                 1
#define VCLK_PLL_VCO                  	  	26:24
#define VCLK_PLL_DIVIDER                    23:16

/* Video clock 1. Bit field definiton is same as VCLK0 */
#define VCLK1_PLL                            0x000238
#define VCLK1_PLL_VCLK1CP					 13:12

/* Video clock 1. Bit field definiton is same as VCLK0 */
#define VCLK2_PLL                            0x00023c
#define VCLK2_PLL_VCLK2CP					 13:12

/* PLL3 (system including DDR/CPU/APB) */
#define PLL3_PLL                            0x000234
#define PLL3_PLL_PRESEL                     27:27
#define PLL3_PLL_PRESEL_250                 0
#define PLL3_PLL_PRESEL_125                 1
#define PLL3_PLL_VCO                  	  	26:24
#define PLL3_PLL_DIVIDER                    23:16

/* PLL2 (VPU and JPU) */
#define PLL2_PLL                            0x000234
#define PLL2_PLL_PRESEL                     11:11
#define PLL2_PLL_PRESEL_250                 0
#define PLL2_PLL_PRESEL_125                 1
#define PLL2_PLL_VCO                  	  	10:8
#define PLL2_PLL_DIVIDER                    7:0

#define SSCG_CTL                           0x000354
#define SSCG_CTL_FN 					   27:27
#define SSCG_CTL_FN_DISABLE 			   0
#define SSCG_CTL_FN_ENABLE 			   	   1
#define SSCG_CTL_PORCORE 				   17:17
#define SSCG_CTL_PORCORE_DISABLE 		   0
#define SSCG_CTL_PORCORE_ENABLE 		   1
#define SSCG_CTL_SSCGPD 				   5:5
#define SSCG_CTL_SSCGPD_OFF 		   	   0
#define SSCG_CTL_SSCGPD_ON 		 		   1

#define SSCG_SET                           0x000358
#define SSCG_SET_MODE                      31:30
#define SSCG_SET_MODE_NO				   0
#define SSCG_SET_MODE_DOWN				   1
#define SSCG_SET_MODE_CENTER			   2
#define SSCG_SET_MODE_UP			   	   3
#define SSCG_SET_DIV				   	   14:0

/* Clock Control 3 */
#define CLOCK3_CTL							0x3bc

#define CLOCK3_CTL_LVDS1CLK                 6:6
#define CLOCK3_CTL_LVDS1CLK_HALF			1
#define CLOCK3_CTL_LVDS1CLK_NORMAL			0
#define CLOCK3_CTL_LVDS0CLK                 5:5
#define CLOCK3_CTL_LVDS0CLK_HALF			1
#define CLOCK3_CTL_LVDS0CLK_NORMAL			0

#define CLOCK3_CTL_SYCCLK					2:0
#define CLOCK3_CTL_SYCCLK_PLL3				1
#define CLOCK3_CTL_SYCCLK_24MHZ				2
#define CLOCK3_CTL_SYCCLK_LFOSC				4

#define PROTOCOL_SEMAPHORE0                            0x000080
#define PROTOCOL_SEMAPHORE1                            0x000084

#define VGA_CONFIGURATION                              0x000088
#define VGA_CONFIGURATION_PLL                          2:2
#define VGA_CONFIGURATION_PLL_VGA                      0
#define VGA_CONFIGURATION_PLL_PANEL                    1
#define VGA_CONFIGURATION_MODE                         1:1
#define VGA_CONFIGURATION_MODE_TEXT                    0
#define VGA_CONFIGURATION_MODE_GRAPHIC                 1
#define VGA_CONFIGURATION_PREFETCH                     0:0
#define VGA_CONFIGURATION_PREFETCH_DISABLE             0
#define VGA_CONFIGURATION_PREFETCH_ENABLE              1

/* Lock or unlock PCIE bar 2 to 5 */
#define PCIE_BAR                                    0x00008C
#define PCIE_BAR_LOCK                                0:0
#define PCIE_BAR_LOCK_UNLOCK                        0
#define PCIE_BAR_LOCK_LOCK                            1

#define RAW_INT                                        0x000090
#define RAW_INT_VPU                                    6:6
#define RAW_INT_VPU_INACTIVE                           0
#define RAW_INT_VPU_ACTIVE                             1
#define RAW_INT_VPU_CLEAR                              1
#define RAW_INT_CSC                                    5:5
#define RAW_INT_CSC_INACTIVE                           0
#define RAW_INT_CSC_ACTIVE                             1
#define RAW_INT_CSC_CLEAR                              1
#define RAW_INT_DE                                     4:4
#define RAW_INT_DE_INACTIVE                            0
#define RAW_INT_DE_ACTIVE                              1
#define RAW_INT_DE_CLEAR                               1
#define RAW_INT_CHANNEL2_VSYNC                         3:3
#define RAW_INT_CHANNEL2_VSYNC_INACTIVE                0
#define RAW_INT_CHANNEL2_VSYNC_ACTIVE                  1
#define RAW_INT_CHANNEL2_VSYNC_CLEAR                   1
#define RAW_INT_CHANNEL1_VSYNC                         2:2
#define RAW_INT_CHANNEL1_VSYNC_INACTIVE                0
#define RAW_INT_CHANNEL1_VSYNC_ACTIVE                  1
#define RAW_INT_CHANNEL1_VSYNC_CLEAR                   1
#define RAW_INT_CHANNEL0_VSYNC                         1:1
#define RAW_INT_CHANNEL0_VSYNC_INACTIVE                0
#define RAW_INT_CHANNEL0_VSYNC_ACTIVE                  1
#define RAW_INT_CHANNEL0_VSYNC_CLEAR                   1
#define RAW_INT_VGA_VSYNC                              0:0
#define RAW_INT_VGA_VSYNC_INACTIVE                     0
#define RAW_INT_VGA_VSYNC_ACTIVE                       1
#define RAW_INT_VGA_VSYNC_CLEAR                        1

#ifdef SMI_ARM
#define INT_STATUS                                     0x0000A8
#else
#define INT_STATUS                                     0x000094
#endif
#define INT_STATUS_TIMER3                              31:31
#define INT_STATUS_TIMER3_INACTIVE                     0
#define INT_STATUS_TIMER3_ACTIVE                       1
#define INT_STATUS_TIMER2                              30:30
#define INT_STATUS_TIMER2_INACTIVE                     0
#define INT_STATUS_TIMER2_ACTIVE                       1
#define INT_STATUS_TIMER1                              29:29
#define INT_STATUS_TIMER1_INACTIVE                     0
#define INT_STATUS_TIMER1_ACTIVE                       1
#define INT_STATUS_TIMER0                              28:28
#define INT_STATUS_TIMER0_INACTIVE                     0
#define INT_STATUS_TIMER0_ACTIVE                       1
#define INT_STATUS_VPU                                27:27
#define INT_STATUS_VPU_INACTIVE                        0
#define INT_STATUS_VPU_ACTIVE                        1
#define INT_STATUS_JPU3                                26:26
#define INT_STATUS_JPU3_INACTIVE                        0
#define INT_STATUS_JPU3_ACTIVE                        1
#define INT_STATUS_JPU2                                25:25
#define INT_STATUS_JPU2_INACTIVE                        0
#define INT_STATUS_JPU2_ACTIVE                        1
#define INT_STATUS_JPU1                                24:24
#define INT_STATUS_JPU1_INACTIVE                        0
#define INT_STATUS_JPU1_ACTIVE                        1
#define INT_STATUS_JPU0                                23:23
#define INT_STATUS_JPU0_INACTIVE                        0
#define INT_STATUS_JPU0_ACTIVE                        1
#define INT_STATUS_USBS                                22:22
#define INT_STATUS_USBS_INACTIVE                       0
#define INT_STATUS_USBS_ACTIVE                         1
#define INT_STATUS_I2S                                 21:21
#define INT_STATUS_I2S_INACTIVE                        0
#define INT_STATUS_I2S_ACTIVE                          1
#define INT_STATUS_SSP0                                20:20
#define INT_STATUS_SSP0_INACTIVE                       0
#define INT_STATUS_SSP0_ACTIVE                         1
#define INT_STATUS_I2C0                                18:18
#define INT_STATUS_I2C0_INACTIVE                       0
#define INT_STATUS_I2C0_ACTIVE                         1
#define INT_STATUS_HDCP                               17:17
#define INT_STATUS_HDCP_INACTIVE                      0
#define INT_STATUS_HDCP_ACTIVE                        1
#define INT_STATUS_HDMI2WK                                16:16
#define INT_STATUS_HDMI2WK_INACTIVE                    0
#define INT_STATUS_HDMI2WK_ACTIVE                        1
#define INT_STATUS_HDMI2                                15:15
#define INT_STATUS_HDMI2_INACTIVE                    0
#define INT_STATUS_HDMI2_ACTIVE                        1
#define INT_STATUS_HDMI1WK                                14:14
#define INT_STATUS_HDMI1WK_INACTIVE                    0
#define INT_STATUS_HDMI1WK_ACTIVE                        1
#define INT_STATUS_HDMI1                                13:13
#define INT_STATUS_HDMI1_INACTIVE                    0
#define INT_STATUS_HDMI1_ACTIVE                        1
#define INT_STATUS_HDMI0WK                                12:12
#define INT_STATUS_HDMI0WK_INACTIVE                    0
#define INT_STATUS_HDMI0WK_ACTIVE                        1
#define INT_STATUS_HDMI0                                11:11
#define INT_STATUS_HDMI0_INACTIVE                    0
#define INT_STATUS_HDMI0_ACTIVE                        1
#define INT_STATUS_HDMI0_CLEAR                        1
#define INT_STATUS_DP1                                10:10
#define INT_STATUS_DP1_INACTIVE                      0
#define INT_STATUS_DP1_ACTIVE                        1
#define INT_STATUS_DP0                               9:9
#define INT_STATUS_DP0_INACTIVE                    	 0
#define INT_STATUS_DP0_ACTIVE                        1
#define INT_STATUS_DMA                                8:8
#define INT_STATUS_DMA_INACTIVE                        0
#define INT_STATUS_DMA_ACTIVE                          1
#define INT_STATUS_CPU                                 7:7
#define INT_STATUS_CPU_INACTIVE                        0
#define INT_STATUS_CPU_ACTIVE                          1
#define INT_STATUS_DE                                  6:6
#define INT_STATUS_DE_INACTIVE                         0
#define INT_STATUS_DE_ACTIVE                           1
#define INT_STATUS_CHANNEL2_VSYNC                      5:5
#define INT_STATUS_CHANNEL2_VSYNC_INACTIVE             0
#define INT_STATUS_CHANNEL2_VSYNC_ACTIVE               1
#define INT_STATUS_CHANNEL1_VSYNC                      4:4
#define INT_STATUS_CHANNEL1_VSYNC_INACTIVE             0
#define INT_STATUS_CHANNEL1_VSYNC_ACTIVE               1
#define INT_STATUS_CHANNEL0_VSYNC                      3:3
#define INT_STATUS_CHANNEL0_VSYNC_INACTIVE             0
#define INT_STATUS_CHANNEL0_VSYNC_ACTIVE               1
#define INT_STATUS_PCIE_ST0                           0:0
#define INT_STATUS_PCIE_ST0_INACTIVE                  0
#define INT_STATUS_PCIE_ST0_ACTIVE                    1

#ifdef SMI_ARM
#define INT_MASK                                       0x00009C
#else
#define INT_MASK                                       0x000098
#endif

#define INT_MASK_TIMER3                                31:31
#define INT_MASK_TIMER3_DISABLE                        0
#define INT_MASK_TIMER3_ENABLE                         1
#define INT_MASK_TIMER2                                30:30
#define INT_MASK_TIMER2_DISABLE                        0
#define INT_MASK_TIMER2_ENABLE                         1
#define INT_MASK_TIMER1                                29:29
#define INT_MASK_TIMER1_DISABLE                        0
#define INT_MASK_TIMER1_ENABLE                         1
#define INT_MASK_TIMER0                                28:28
#define INT_MASK_TIMER0_DISABLE                        0
#define INT_MASK_TIMER0_ENABLE                         1
#define INT_MASK_VPU                                27:27
#define INT_MASK_VPU_DISABLE                        0
#define INT_MASK_VPU_ENABLE                            1
#define INT_MASK_JPU3                                26:26
#define INT_MASK_JPU3_DISABLE                        0
#define INT_MASK_JPU3_ENABLE                            1
#define INT_MASK_JPU2                                25:25
#define INT_MASK_JPU2_DISABLE                        0
#define INT_MASK_JPU2_ENABLE                            1
#define INT_MASK_JPU1                                24:24
#define INT_MASK_JPU1_DISABLE                        0
#define INT_MASK_JPU1_ENABLE                            1
#define INT_MASK_JPU0                                23:23
#define INT_MASK_JPU0_DISABLE                        0
#define INT_MASK_JPU0_ENABLE                            1
#define INT_MASK_USBS                                 22:22
#define INT_MASK_USBS_DISABLE                         0
#define INT_MASK_USBS_ENABLE                          1
#define INT_MASK_I2S                                   21:21
#define INT_MASK_I2S_DISABLE                           0
#define INT_MASK_I2S_ENABLE                            1
#define INT_MASK_SSP0                                  20:20
#define INT_MASK_SSP0_DISABLE                          0
#define INT_MASK_SSP0_ENABLE                           1
#define INT_MASK_I2C1                                  19:19
#define INT_MASK_I2C1_DISABLE                          0
#define INT_MASK_I2C1_ENABLE                           1
#define INT_MASK_I2C0                                  18:18
#define INT_MASK_I2C0_DISABLE                          0
#define INT_MASK_I2C0_ENABLE                           1
#define INT_MASK_HDCP                               17:17
#define INT_MASK_HDCP_DISABLE                      0
#define INT_MASK_HDCP_ENABLE                        1
#define INT_MASK_HDMI2WK                                16:16
#define INT_MASK_HDMI2WK_DISABLE                    0
#define INT_MASK_HDMI2WK_ENABLE                        1
#define INT_MASK_HDMI2                                15:15
#define INT_MASK_HDMI2_DISABLE                        0
#define INT_MASK_HDMI2_ENABLE                        1
#define INT_MASK_HDMI1WK                                14:14
#define INT_MASK_HDMI1WK_DISABLE                    0
#define INT_MASK_HDMI1WK_ENABLE                        1
#define INT_MASK_HDMI1                                13:13
#define INT_MASK_HDMI1_DISABLE                        0
#define INT_MASK_HDMI1_ENABLE                        1
#define INT_MASK_HDMI0WK                                12:12
#define INT_MASK_HDMI0WK_DISABLE                    0
#define INT_MASK_HDMI0WK_ENABLE                        1
#define INT_MASK_HDMI0                                11:11
#define INT_MASK_HDMI0_DISABLE                        0
#define INT_MASK_HDMI0_ENABLE                        1
#define INT_MASK_DP1                                10:10
#define INT_MASK_DP1_DISABLE                      0
#define INT_MASK_DP1_ENABLE                        1
#define INT_MASK_DP0                               9:9
#define INT_MASK_DP0_DISABLE                    	 0
#define INT_MASK_DP0_ENABLE                        1
#define INT_MASK_DMA                                   8:8
#define INT_MASK_DMA_DISABLE                           0
#define INT_MASK_DMA_ENABLE                            1
#define INT_MASK_CPU                                   7:7
#define INT_MASK_CPU_DISABLE                           0
#define INT_MASK_CPU_ENABLE                            1
#define INT_MASK_DE                                    6:6
#define INT_MASK_DE_DISABLE                            0
#define INT_MASK_DE_ENABLE                             1
#define INT_MASK_CHANNEL2_VSYNC                        5:5
#define INT_MASK_CHANNEL2_VSYNC_DISABLE                0
#define INT_MASK_CHANNEL2_VSYNC_ENABLE                 1
#define INT_MASK_CHANNEL1_VSYNC                        4:4
#define INT_MASK_CHANNEL1_VSYNC_DISABLE                0
#define INT_MASK_CHANNEL1_VSYNC_ENABLE                 1
#define INT_MASK_CHANNEL0_VSYNC                        3:3
#define INT_MASK_CHANNEL0_VSYNC_DISABLE                0
#define INT_MASK_CHANNEL0_VSYNC_ENABLE                 1
#define INT_MASK_STATUS_PCIE_ST0                            0:0
#define INT_MASK_STATUS_PCIE_ST0_DISABLE                     0
#define INT_MASK_STATUS_PCIE_ST0_ENABLE                      1

#define INT_STATUS1                                       0x0000B4

#define INT_STATUS1_PWM                                 11:11
#define INT_STATUS1_PWM_INACTIVE                        0
#define INT_STATUS1_PWM_ACTIVE                          1
#define INT_STATUS1_TRNG                                 10:10
#define INT_STATUS1_TRNG_INACTIVE                        0
#define INT_STATUS1_TRNG_ACTIVE                          1
#define INT_STATUS1_UART1                                 9:9
#define INT_STATUS1_UART1_INACTIVE                        0
#define INT_STATUS1_UART1_ACTIVE                          1
#define INT_STATUS1_UART0                                 8:8
#define INT_STATUS1_UART0_INACTIVE                        0
#define INT_STATUS1_UART0_ACTIVE                          1
#define INT_STATUS1_GPIO44                               7:7
#define INT_STATUS1_GPIO44_INACTIVE                      0
#define INT_STATUS1_GPIO44_ACTIVE                        1
#define INT_STATUS1_GPIO43                               6:6
#define INT_STATUS1_GPIO43_INACTIVE                      0
#define INT_STATUS1_GPIO43_ACTIVE                        1
#define INT_STATUS1_GPIO57                               5:5
#define INT_STATUS1_GPIO57_INACTIVE                      0
#define INT_STATUS1_GPIO57_ACTIVE                        1
#define INT_STATUS1_GPIO56                               4:4
#define INT_STATUS1_GPIO56_INACTIVE                      0
#define INT_STATUS1_GPIO56_ACTIVE                        1
#define INT_STATUS1_GPIO55                               3:3
#define INT_STATUS1_GPIO55_INACTIVE                      0
#define INT_STATUS1_GPIO55_ACTIVE                        1
#define INT_STATUS1_GPIO54                               2:2
#define INT_STATUS1_GPIO54_INACTIVE                      0
#define INT_STATUS1_GPIO54_ACTIVE                        1
#define INT_STATUS1_GPIO53                               1:1
#define INT_STATUS1_GPIO53_INACTIVE                      0
#define INT_STATUS1_GPIO53_ACTIVE                        1
#define INT_STATUS1_CSC                                 0:0
#define INT_STATUS1_CSC_INACTIVE                        0
#define INT_STATUS1_CSC_ACTIVE                          1

#define INT_MASK1                                       0x0000B8

#define INT_MASK1_PWM                                   11:11
#define INT_MASK1_PWM_DISABLE                           0
#define INT_MASK1_PWM_ENABLE                            1
#define INT_MASK1_TRNG                                   10:10
#define INT_MASK1_TRNG_DISABLE                           0
#define INT_MASK1_TRNG_ENABLE                            1
#define INT_MASK1_UART1                                 9:9
#define INT_MASK1_UART1_DISABLE                         0
#define INT_MASK1_UART1_ENABLE                          1
#define INT_MASK1_UART0                                8:8
#define INT_MASK1_UART0_DISABLE                         0
#define INT_MASK1_UART0_ENABLE                          1
#define INT_MASK1_GPIO44                                 7:7
#define INT_MASK1_GPIO44_DISABLE                         0
#define INT_MASK1_GPIO44_ENABLE                          1
#define INT_MASK1_GPIO43                                 6:6
#define INT_MASK1_GPIO43_DISABLE                         0
#define INT_MASK1_GPIO43_ENABLE                          1
#define INT_MASK1_GPIO57                                 5:5
#define INT_MASK1_GPIO57_DISABLE                         0
#define INT_MASK1_GPIO57_ENABLE                          1
#define INT_MASK1_GPIO56                                 4:4
#define INT_MASK1_GPIO56_DISABLE                         0
#define INT_MASK1_GPIO56_ENABLE                          1
#define INT_MASK1_GPIO55                                 3:3
#define INT_MASK1_GPIO55_DISABLE                         0
#define INT_MASK1_GPIO55_ENABLE                          1
#define INT_MASK1_GPIO54                                 2:2
#define INT_MASK1_GPIO54_DISABLE                         0
#define INT_MASK1_GPIO54_ENABLE                          1
#define INT_MASK1_GPIO53                                 1:1
#define INT_MASK1_GPIO53_DISABLE                         0
#define INT_MASK1_GPIO53_ENABLE                          1
#define INT_MASK1_CSC                                   0:0
#define INT_MASK1_CSC_DISABLE                           0
#define INT_MASK1_CSC_ENABLE                            1

#define ARM_PROTOCOL_INT                               0x0000A0
#define ARM_PROTOCOL_INT_TOKEN                        31:1
#define ARM_PROTOCOL_INT_ENABLE                        0:0
#define ARM_PROTOCOL_INT_ENABLE_CLEAR                0
#define ARM_PROTOCOL_INT_ENABLE_ENABLE                1

#define PCIE_PROTOCOL_INT                              0x0000A4
#define PCIE_PROTOCOL_INT_TOKEN                        31:1
#define PCIE_PROTOCOL_INT_ENABLE                    0:0
#define PCIE_PROTOCOL_INT_ENABLE_CLEAR                0
#define PCIE_PROTOCOL_INT_ENABLE_ENABLE                1

#define ARM_STARTUP_CONFIG                            0x000100
#define ARM_STARTUP_CONFIG_USBH                        30:30
#define ARM_STARTUP_CONFIG_USBH_NORMAL                0
#define ARM_STARTUP_CONFIG_USBH_RESET                1
#define ARM_STARTUP_CONFIG_USBHPHY                    29:29
#define ARM_STARTUP_CONFIG_USBHPHY_NORMAL            0
#define ARM_STARTUP_CONFIG_USBHPHY_RESET            1
#define ARM_STARTUP_CONFIG_USBSPHY                    28:28
#define ARM_STARTUP_CONFIG_USBSPHY_NORMAL            0
#define ARM_STARTUP_CONFIG_USBSPHY_RESET            1
#define ARM_STARTUP_CONFIG_USBS                        27:27
#define ARM_STARTUP_CONFIG_USBS_NORMAL                0
#define ARM_STARTUP_CONFIG_USBS_RESET                1
#define ARM_STARTUP_CONFIG_ARM                        0:0
#define ARM_STARTUP_CONFIG_ARM_STOP                    0
#define ARM_STARTUP_CONFIG_ARM_START                1

#define ARM_CONTROL                                  0x000110
#define ARM_CONTROL_RESET                            0:0
#define ARM_CONTROL_RESET_RESET                        0
#define ARM_CONTROL_RESET_NORMAL                    1

#define STRAP_PINS1                                 0x0010000
#define STRAP_PINS1_BOOTDMA                         30:30
#define STRAP_PINS1_BOOTDMA_USB                      0    
#define STRAP_PINS1_BOOTDMA_PCIE					 1 
#define STRAP_PINS1_MEM_SIZE                         31:31
#define STRAP_PINS1_MEM_SIZE_256M                    0
#define STRAP_PINS1_MEM_SIZE_512M                    1

#define STRAP_PINS2                                 0x0010018
#define STRAP_PINS2_MEM_SIZE                        0:0
#define STRAP_PINS2_MEM_SIZE_X1                    	0    
#define STRAP_PINS2_MEM_SIZE_X4					 	1 
#define STRAP_PINS2_DDRRATE                         1:1
#define STRAP_PINS2_DDRRATE_3200                    0    
#define STRAP_PINS2_DDRRATE_1600					1 
#define STRAP_PINS2_DDRWIDTH                        2:2
#define STRAP_PINS2_DDRWIDTH_HALF                   0
#define STRAP_PINS2_DDRWIDTH_FULL                   1
#define STRAP_PINS2_DDRTYPE                         3:3
#define STRAP_PINS2_DDRTYPE_DDR4                    0
#define STRAP_PINS2_DDRTYPE_LPDDR4                  1

#define JPU_PERFORMANCE_MODE                         0x000134
#define JPU_PERFORMANCE_MODE_JPU1                    3:2
#define JPU_PERFORMANCE_MODE_JPU1_DISABLE            0
#define JPU_PERFORMANCE_MODE_JPU1_HD                 1
#define JPU_PERFORMANCE_MODE_JPU1_UHD                2
#define JPU_PERFORMANCE_MODE_JPU0                    1:0
#define JPU_PERFORMANCE_MODE_JPU0_DISABLE	           0
#define JPU_PERFORMANCE_MODE_JPU0_HD                 1
#define JPU_PERFORMANCE_MODE_JPU0_UHD                2

#define PMODE_2K 2048
#define PMODE_4K 4096

#define DDR_PRIORITY1                                0x000138

#define DDR_PRIORITY2                                0x00013C

#define THERMAL_SENSOR								0x214
#define THERMAL_SENSOR_STATUS						10:10
#define THERMAL_SENSOR_STATUS_ENABLE				1
#define THERMAL_SENSOR_STATUS_DISABLE				0
#define THERMAL_SENSOR_VALUE_BIT8					9:9
#define THERMAL_SENSOR_PD							8:8
#define THERMAL_SENSOR_PD_ENABLE					1
#define THERMAL_SENSOR_PD_DISABLE					0
#define THERMAL_SENSOR_VALUE_LOWBIT					7:0

#define CHIP_REV 									0x8B0
#define CHIP_REV_AA									0
#define CHIP_REV_AB									1

#define GPIO_DATA                                       0x010000
#define GPIO_DATA_31                                    31:31
#define GPIO_DATA_30                                    30:30
#define GPIO_DATA_29                                    29:29
#define GPIO_DATA_28                                    28:28
#define GPIO_DATA_27                                    27:27
#define GPIO_DATA_26                                    26:26
#define GPIO_DATA_25                                    25:25
#define GPIO_DATA_24                                    24:24
#define GPIO_DATA_23                                    23:23
#define GPIO_DATA_22                                    22:22
#define GPIO_DATA_21                                    21:21
#define GPIO_DATA_20                                    20:20
#define GPIO_DATA_19                                    19:19
#define GPIO_DATA_18                                    18:18
#define GPIO_DATA_17                                    17:17
#define GPIO_DATA_16                                    16:16
#define GPIO_DATA_15                                    15:15
#define GPIO_DATA_14                                    14:14
#define GPIO_DATA_13                                    13:13
#define GPIO_DATA_12                                    12:12
#define GPIO_DATA_11                                    11:11
#define GPIO_DATA_10                                    10:10
#define GPIO_DATA_9                                     9:9
#define GPIO_DATA_8                                     8:8
#define GPIO_DATA_7                                     7:7
#define GPIO_DATA_6                                     6:6
#define GPIO_DATA_5                                     5:5
#define GPIO_DATA_4                                     4:4
#define GPIO_DATA_3                                     3:3
#define GPIO_DATA_2                                     2:2
#define GPIO_DATA_1                                     1:1
#define GPIO_DATA_0                                     0:0

#define GPIO_DATA2                                      0x010018
#define GPIO_DATA2_60                                    28:28
#define GPIO_DATA2_59                                    27:27
#define GPIO_DATA2_58                                    26:26
#define GPIO_DATA2_57                                    25:25
#define GPIO_DATA2_56                                    24:24
#define GPIO_DATA2_55                                    23:23
#define GPIO_DATA2_54                                    22:22
#define GPIO_DATA2_53                                    21:21
#define GPIO_DATA2_52                                    20:20
#define GPIO_DATA2_51                                    19:19
#define GPIO_DATA2_50                                    18:18
#define GPIO_DATA2_49                                    17:17
#define GPIO_DATA2_48                                    16:16
#define GPIO_DATA2_47                                    15:15
#define GPIO_DATA2_46                                    14:14
#define GPIO_DATA2_45                                    13:13
#define GPIO_DATA2_44                                    12:12
#define GPIO_DATA2_43                                    11:11
#define GPIO_DATA2_42                                    10:10
#define GPIO_DATA2_41                                     9:9
#define GPIO_DATA2_40                                    8:8
#define GPIO_DATA2_39                                     7:7
#define GPIO_DATA2_38                                     6:6
#define GPIO_DATA2_37                                     5:5
#define GPIO_DATA2_36                                     4:4
#define GPIO_DATA2_35                                     3:3
#define GPIO_DATA2_34                                     2:2
#define GPIO_DATA2_33                                     1:1
#define GPIO_DATA2_32                                     0:0

#define GPIO0_PAD_CONTROL                                0x10080
#define GPIO0_PAD_CONTROL_INPUT							24:24     //input signal from core side
#define GPIO0_PAD_CONTROL_MSC							17:17	  //Mode selector
#define GPIO0_PAD_CONTROL_ST							16:16	 //Schmitt trigger enable
#define GPIO0_PAD_CONTROL_ST_ENABLE						1
#define GPIO0_PAD_CONTROL_ST_DISABLE					0
#define GPIO0_PAD_CONTROL_SL							15:15	  //Slew-rate-control enable
#define GPIO0_PAD_CONTROL_DS2							14:14      //Driving selctor
#define GPIO0_PAD_CONTROL_DS1							13:13		//Driving selctor
#define GPIO0_PAD_CONTROL_DS0							12:12		//Driving selctor

#define GPIO0_PAD_CONTROL_PD							11:11		//Pull down enable
#define GPIO0_PAD_CONTROL_PD_ENABLE						1
#define GPIO0_PAD_CONTROL_PD_DISABLE					0

#define GPIO0_PAD_CONTROL_PU							10:10		//Pull up enable
#define GPIO0_PAD_CONTROL_PU_ENABLE						1
#define GPIO0_PAD_CONTROL_PU_DISABLE					0

#define GPIO0_PAD_CONTROL_OEN							8:8			//Output enable
#define GPIO0_PAD_CONTROL_OEN_INPUT						1
#define GPIO0_PAD_CONTROL_OEN_OUTPUT						0

#define GPIO0_PAD_CONTROL_DATA							0:0			//Data bit reflect the valeue on the GPIO pin

#define GPIO_PAD_DP0_HPD                                0x10110
#define GPIO_PAD_DP0_HPD_PD                             11:11
#define GPIO_PAD_DP0_HPD_PD_ENABLE                      1
#define GPIO_PAD_DP0_HPD_PD_DISABLE                     0


#define GPIO_INTERRUPT_SETUP                            0x010008
#define GPIO_INTERRUPT_SETUP_TRIGGER_44                  22:22
#define GPIO_INTERRUPT_SETUP_TRIGGER_44_EDGE             0
#define GPIO_INTERRUPT_SETUP_TRIGGER_44_LEVEL            1
#define GPIO_INTERRUPT_SETUP_TRIGGER_43                  21:21
#define GPIO_INTERRUPT_SETUP_TRIGGER_43_EDGE             0
#define GPIO_INTERRUPT_SETUP_TRIGGER_43_LEVEL            1
#define GPIO_INTERRUPT_SETUP_TRIGGER_57                  20:20
#define GPIO_INTERRUPT_SETUP_TRIGGER_57_EDGE             0
#define GPIO_INTERRUPT_SETUP_TRIGGER_57_LEVEL            1
#define GPIO_INTERRUPT_SETUP_TRIGGER_56                  19:19
#define GPIO_INTERRUPT_SETUP_TRIGGER_56_EDGE             0
#define GPIO_INTERRUPT_SETUP_TRIGGER_56_LEVEL            1
#define GPIO_INTERRUPT_SETUP_TRIGGER_55                  18:18
#define GPIO_INTERRUPT_SETUP_TRIGGER_55_EDGE             0
#define GPIO_INTERRUPT_SETUP_TRIGGER_55_LEVEL            1
#define GPIO_INTERRUPT_SETUP_TRIGGER_54                  17:17
#define GPIO_INTERRUPT_SETUP_TRIGGER_54_EDGE             0
#define GPIO_INTERRUPT_SETUP_TRIGGER_54_LEVEL            1
#define GPIO_INTERRUPT_SETUP_TRIGGER_53                  16:16
#define GPIO_INTERRUPT_SETUP_TRIGGER_53_EDGE             0
#define GPIO_INTERRUPT_SETUP_TRIGGER_53_LEVEL            1
#define GPIO_INTERRUPT_SETUP_ACTIVE_44                   14:14
#define GPIO_INTERRUPT_SETUP_ACTIVE_44_LOW               0
#define GPIO_INTERRUPT_SETUP_ACTIVE_44_HIGH              1
#define GPIO_INTERRUPT_SETUP_ACTIVE_43                   13:13
#define GPIO_INTERRUPT_SETUP_ACTIVE_43_LOW               0
#define GPIO_INTERRUPT_SETUP_ACTIVE_43_HIGH              1
#define GPIO_INTERRUPT_SETUP_ACTIVE_57                   12:12
#define GPIO_INTERRUPT_SETUP_ACTIVE_57_LOW               0
#define GPIO_INTERRUPT_SETUP_ACTIVE_57_HIGH              1
#define GPIO_INTERRUPT_SETUP_ACTIVE_56                   11:11
#define GPIO_INTERRUPT_SETUP_ACTIVE_56_LOW               0
#define GPIO_INTERRUPT_SETUP_ACTIVE_56_HIGH              1
#define GPIO_INTERRUPT_SETUP_ACTIVE_55                   10:10
#define GPIO_INTERRUPT_SETUP_ACTIVE_55_LOW               0
#define GPIO_INTERRUPT_SETUP_ACTIVE_55_HIGH              1
#define GPIO_INTERRUPT_SETUP_ACTIVE_54                   9:9
#define GPIO_INTERRUPT_SETUP_ACTIVE_54_LOW               0
#define GPIO_INTERRUPT_SETUP_ACTIVE_54_HIGH              1
#define GPIO_INTERRUPT_SETUP_ACTIVE_53                   8:8
#define GPIO_INTERRUPT_SETUP_ACTIVE_53_LOW               0
#define GPIO_INTERRUPT_SETUP_ACTIVE_53_HIGH              1
#define GPIO_INTERRUPT_SETUP_ENABLE_44                   6:6
#define GPIO_INTERRUPT_SETUP_ENABLE_44_GPIO              0
#define GPIO_INTERRUPT_SETUP_ENABLE_44_INTERRUPT         1
#define GPIO_INTERRUPT_SETUP_ENABLE_43                   5:5
#define GPIO_INTERRUPT_SETUP_ENABLE_43_GPIO              0
#define GPIO_INTERRUPT_SETUP_ENABLE_43_INTERRUPT         1
#define GPIO_INTERRUPT_SETUP_ENABLE_57                   4:4
#define GPIO_INTERRUPT_SETUP_ENABLE_57_GPIO              0
#define GPIO_INTERRUPT_SETUP_ENABLE_57_INTERRUPT         1
#define GPIO_INTERRUPT_SETUP_ENABLE_56                   3:3
#define GPIO_INTERRUPT_SETUP_ENABLE_56_GPIO              0
#define GPIO_INTERRUPT_SETUP_ENABLE_56_INTERRUPT         1
#define GPIO_INTERRUPT_SETUP_ENABLE_55                   2:2
#define GPIO_INTERRUPT_SETUP_ENABLE_55_GPIO              0
#define GPIO_INTERRUPT_SETUP_ENABLE_55_INTERRUPT         1
#define GPIO_INTERRUPT_SETUP_ENABLE_54                   1:1
#define GPIO_INTERRUPT_SETUP_ENABLE_54_GPIO              0
#define GPIO_INTERRUPT_SETUP_ENABLE_54_INTERRUPT         1
#define GPIO_INTERRUPT_SETUP_ENABLE_53                   0:0
#define GPIO_INTERRUPT_SETUP_ENABLE_53_GPIO              0
#define GPIO_INTERRUPT_SETUP_ENABLE_53_INTERRUPT         1

#define GPIO_INTERRUPT_STATUS                           0x01000C
#define GPIO_INTERRUPT_STATUS_44                         6:6
#define GPIO_INTERRUPT_STATUS_44_INACTIVE                0
#define GPIO_INTERRUPT_STATUS_44_ACTIVE                  1
#define GPIO_INTERRUPT_STATUS_44_RESET                   1
#define GPIO_INTERRUPT_STATUS_43                         5:5
#define GPIO_INTERRUPT_STATUS_43_INACTIVE                0
#define GPIO_INTERRUPT_STATUS_43_ACTIVE                  1
#define GPIO_INTERRUPT_STATUS_43_RESET                   1
#define GPIO_INTERRUPT_STATUS_57                         4:4
#define GPIO_INTERRUPT_STATUS_57_INACTIVE                0
#define GPIO_INTERRUPT_STATUS_57_ACTIVE                  1
#define GPIO_INTERRUPT_STATUS_57_RESET                   1
#define GPIO_INTERRUPT_STATUS_56                         3:3
#define GPIO_INTERRUPT_STATUS_56_INACTIVE                0
#define GPIO_INTERRUPT_STATUS_56_ACTIVE                  1
#define GPIO_INTERRUPT_STATUS_56_RESET                   1
#define GPIO_INTERRUPT_STATUS_55                         2:2
#define GPIO_INTERRUPT_STATUS_55_INACTIVE                0
#define GPIO_INTERRUPT_STATUS_55_ACTIVE                  1
#define GPIO_INTERRUPT_STATUS_55_RESET                   1
#define GPIO_INTERRUPT_STATUS_54                         1:1
#define GPIO_INTERRUPT_STATUS_54_INACTIVE                0
#define GPIO_INTERRUPT_STATUS_54_ACTIVE                  1
#define GPIO_INTERRUPT_STATUS_54_RESET                   1
#define GPIO_INTERRUPT_STATUS_53                         0:0
#define GPIO_INTERRUPT_STATUS_53_INACTIVE                0
#define GPIO_INTERRUPT_STATUS_53_ACTIVE                  1
#define GPIO_INTERRUPT_STATUS_53_RESET                   1

#define GPIO_MUX                                      0x010010

//I2C4 is for Slave mode
#if 0
#define GPIO_MUX_I2C4                                 27:26
#define GPIO_MUX_I2C4_DISABLE                         0
#define GPIO_MUX_I2C4_ENABLE                          3
#endif

//I2C3 is for General IIC Master
#define GPIO_MUX_I2C3                                 25:24
#define GPIO_MUX_I2C3_DISABLE                         0
#define GPIO_MUX_I2C3_ENABLE                          3

//I2C0-2 is for HDMI0-HDMI2 DDC
#define GPIO_MUX_I2C2                                 5:4
#define GPIO_MUX_I2C2_DISABLE                         0
#define GPIO_MUX_I2C2_ENABLE                          3

#define GPIO_MUX_I2C1                                 3:2
#define GPIO_MUX_I2C1_DISABLE                         0
#define GPIO_MUX_I2C1_ENABLE                          3

#define GPIO_MUX_I2C0                                 1:0
#define GPIO_MUX_I2C0_DISABLE                         0
#define GPIO_MUX_I2C0_ENABLE                          3

#define GPIO_MUX_UART0                                20:18
#define GPIO_MUX_UART0_DISABLE                        0
#define GPIO_MUX_UART0_ENABLE                         7

#define GPIO_MUX_UART1                                19:17
#define GPIO_MUX_UART1_DISABLE                        0
#define GPIO_MUX_UART1_ENABLE                         7

#define GPIO_MUX_25                                   25:25
#define GPIO_MUX_25_GPIO                              0
#define GPIO_MUX_25_I2C3                              1
#define GPIO_MUX_24                                   24:24
#define GPIO_MUX_24_GPIO                              0
#define GPIO_MUX_24_I2C3                              1
#define GPIO_MUX_23                                   23:23
#define GPIO_MUX_23_GPIO                              0
#define GPIO_MUX_23_PWM2                              1
#define GPIO_MUX_22                                   22:22
#define GPIO_MUX_22_GPIO                              0
#define GPIO_MUX_22_PWM1                              1
#define GPIO_MUX_21                                   21:21
#define GPIO_MUX_21_GPIO                              0
#define GPIO_MUX_21_PWM0                              1
#define GPIO_MUX_20                                   20:20
#define GPIO_MUX_20_GPIO                              0
#define GPIO_MUX_20_UART0                              1
#define GPIO_MUX_19                                   19:19
#define GPIO_MUX_19_GPIO                              0
#define GPIO_MUX_19_UART1                              1
#define GPIO_MUX_18                                   18:18
#define GPIO_MUX_18_GPIO                              0
#define GPIO_MUX_18_UART0                              1
#define GPIO_MUX_17                                   17:17
#define GPIO_MUX_17_GPIO                              0
#define GPIO_MUX_17_UART1                              1
#define GPIO_MUX_16                                   16:16
#define GPIO_MUX_16_GPIO                              0
#define GPIO_MUX_16_RESERVE                            1
#define GPIO_MUX_15                                   15:15
#define GPIO_MUX_15_GPIO                              0
#define GPIO_MUX_15_USBPNP                             1
#define GPIO_MUX_14                                   14:14
#define GPIO_MUX_14_GPIO		                      0
#define GPIO_MUX_14_SSP0	                          1
#define GPIO_MUX_13                                   13:13
#define GPIO_MUX_13_GPIO		                      0
#define GPIO_MUX_13_SSP0                             1
#define GPIO_MUX_12                                   12:12
#define GPIO_MUX_12_GPIO		                      0
#define GPIO_MUX_12_SSP0                             1
#define GPIO_MUX_11                                   11:11
#define GPIO_MUX_11_GPIO                              0
#define GPIO_MUX_11_SSP0                             1
#define GPIO_MUX_10                                   10:10
#define GPIO_MUX_10_GPIO		                      0
#define GPIO_MUX_10_SSP0	                          1
#define GPIO_MUX_9                                    9:9
#define GPIO_MUX_9_GPIO			                      0
#define GPIO_MUX_9_I2S_RX                              1
#define GPIO_MUX_8                                    8:8
#define GPIO_MUX_8_GPIO			                      0
#define GPIO_MUX_8_I2S_TX                              1
#define GPIO_MUX_7                                    7:7
#define GPIO_MUX_7_GPIO			                      0
#define GPIO_MUX_7_I2S_WS                               1
#define GPIO_MUX_6                                    6:6
#define GPIO_MUX_6_GPIO			                      0
#define GPIO_MUX_6_I2S_CK                               1
#define GPIO_MUX_5                                    5:5
#define GPIO_MUX_5_GPIO			                      0
#define GPIO_MUX_5_I2C2                            1
#define GPIO_MUX_4                                    4:4
#define GPIO_MUX_4_GPIO		                      0
#define GPIO_MUX_4_I2C2                            1
#define GPIO_MUX_3                                    3:3
#define GPIO_MUX_3_GPIO		                      0
#define GPIO_MUX_3_I2C1                             1
#define GPIO_MUX_2                                    2:2
#define GPIO_MUX_2_GPIO		                      0
#define GPIO_MUX_2_I2C1                             1
#define GPIO_MUX_1                                    1:1
#define GPIO_MUX_1_GPIO                      0
#define GPIO_MUX_1_I2C0                      1
#define GPIO_MUX_0                                    0:0
#define GPIO_MUX_0_GPIO                      0
#define GPIO_MUX_0_I2C0                      1

#define GPIO_MUX1                                      0x010014

#define GPIO_MUX1_52		                             20:20
#define GPIO_MUX1_52_GPIO52			                       0
#define GPIO_MUX1_52_HDMI2CEC							   1

#define GPIO_MUX1_51		                             19:19
#define GPIO_MUX1_51_GPIO51			                       0
#define GPIO_MUX1_51_HDMI1CEC							   1

#define GPIO_MUX1_50		                             18:18
#define GPIO_MUX1_50_GPIO50			                       0
#define GPIO_MUX1_50_HDMI0CEC							   1

/* There are 50 GPIO pads in the system with the same definition,
   but different MMIO address as below

	0x10080 to 0x10148

   We only define the MMIO for GPIO0, the MMIO for other
   GPIO pad control can be work out like this:
   0x10080 + (4 x GPIO number)

*/
#define GPIO_CONTROL		 0x010080
                                   

#define GPIO_CONTROL_DATA  0:0

/* There are 3 PWM in the system with the same definition,
   but different MMIO address as below
  
   PWM 0  0x010020
   PWM 1  0x010024
   PWM 2  0x010028

   We only define the MMIO for PWM 0, the MMIO for other
   PWM can be work out like this:
   0x10020 + (4 x PWM number)
*/
#define PWM_CONTROL                                               0x010020
#define PWM_CONTROL_HIGH_COUNTER                                  31:20
#define PWM_CONTROL_LOW_COUNTER                                   19:8
#define PWM_CONTROL_CLOCK_DIVIDE                                  7:4
#define PWM_CONTROL_CLOCK_DIVIDE_1                                0
#define PWM_CONTROL_CLOCK_DIVIDE_2                                1
#define PWM_CONTROL_CLOCK_DIVIDE_4                                2
#define PWM_CONTROL_CLOCK_DIVIDE_8                                3
#define PWM_CONTROL_CLOCK_DIVIDE_16                               4
#define PWM_CONTROL_CLOCK_DIVIDE_32                               5
#define PWM_CONTROL_CLOCK_DIVIDE_64                               6
#define PWM_CONTROL_CLOCK_DIVIDE_128                              7
#define PWM_CONTROL_CLOCK_DIVIDE_256                              8
#define PWM_CONTROL_CLOCK_DIVIDE_512                              9
#define PWM_CONTROL_CLOCK_DIVIDE_1024                             10
#define PWM_CONTROL_CLOCK_DIVIDE_2048                             11
#define PWM_CONTROL_CLOCK_DIVIDE_4096                             12
#define PWM_CONTROL_CLOCK_DIVIDE_8192                             13
#define PWM_CONTROL_CLOCK_DIVIDE_16384                            14
#define PWM_CONTROL_CLOCK_DIVIDE_32768                            15
#define PWM_CONTROL_INTERRUPT_STATUS                              3:3
#define PWM_CONTROL_INTERRUPT_STATUS_NOT_PENDING                  0
#define PWM_CONTROL_INTERRUPT_STATUS_PENDING                      1
#define PWM_CONTROL_INTERRUPT_STATUS_CLEAR                        1
#define PWM_CONTROL_INTERRUPT                                     2:2
#define PWM_CONTROL_INTERRUPT_DISABLE                             0
#define PWM_CONTROL_INTERRUPT_ENABLE                              1
#define PWM_CONTROL_STATUS                                        0:0
#define PWM_CONTROL_STATUS_DISABLE                                0
#define PWM_CONTROL_STATUS_ENABLE                                 1

/* There are 2 Synchronous Serial Port (SSP) in SM768 with identical definitions,
   but different MMIO address as below
  
   SSP 0 0x20000
   SSP 1 0x20100

   We only define the MMIO for SSP 0, the MMIO for
   SSP 1 can be work out like this:
   0x20000 + SSP0_SSP1_GAP, where SSP0_SSP1_GAP is defined as 0x100.
*/
#define SSP_CONTROL_0                                 0x020000
#define SSP_CONTROL_0_CLOCK_RATE                      15:8
#define SSP_CONTROL_0_SCLKOUT_PHASE                   7:7
#define SSP_CONTROL_0_SCLKOUT_PHASE_0                 0
#define SSP_CONTROL_0_SCLKOUT_PHASE_1                 1
#define SSP_CONTROL_0_SCLKOUT_POLARITY                6:6
#define SSP_CONTROL_0_SCLKOUT_POLARITY_RISING         0
#define SSP_CONTROL_0_SCLKOUT_POLARITY_FALLING        1
#define SSP_CONTROL_0_FRAME_FORMAT                    5:4
#define SSP_CONTROL_0_FRAME_FORMAT_MOTOROLA           0
#define SSP_CONTROL_0_FRAME_FORMAT_TI                 1
#define SSP_CONTROL_0_FRAME_FORMAT_NATIONAL           2
#define SSP_CONTROL_0_DATA_SIZE                       3:0
#define SSP_CONTROL_0_DATA_SIZE_4                     3
#define SSP_CONTROL_0_DATA_SIZE_5                     4
#define SSP_CONTROL_0_DATA_SIZE_6                     5
#define SSP_CONTROL_0_DATA_SIZE_7                     6
#define SSP_CONTROL_0_DATA_SIZE_8                     7
#define SSP_CONTROL_0_DATA_SIZE_9                     8
#define SSP_CONTROL_0_DATA_SIZE_10                    9
#define SSP_CONTROL_0_DATA_SIZE_11                    10
#define SSP_CONTROL_0_DATA_SIZE_12                    11
#define SSP_CONTROL_0_DATA_SIZE_13                    12
#define SSP_CONTROL_0_DATA_SIZE_14                    13
#define SSP_CONTROL_0_DATA_SIZE_15                    14
#define SSP_CONTROL_0_DATA_SIZE_16                    15

#define SSP_CONTROL_1                                 0x020004
#define SSP_CONTROL_1_SLAVE_OUTPUT                    6:6
#define SSP_CONTROL_1_SLAVE_OUTPUT_ENABLE             0
#define SSP_CONTROL_1_SLAVE_OUTPUT_DISABLE            1
#define SSP_CONTROL_1_MODE_SELECT                     5:5
#define SSP_CONTROL_1_MODE_SELECT_MASTER              0
#define SSP_CONTROL_1_MODE_SELECT_SLAVE               1
#define SSP_CONTROL_1_STATUS                          4:4
#define SSP_CONTROL_1_STATUS_DISABLE                  0
#define SSP_CONTROL_1_STATUS_ENABLE                   1
#define SSP_CONTROL_1_LOOP_BACK                       3:3
#define SSP_CONTROL_1_LOOP_BACK_DISABLE               0
#define SSP_CONTROL_1_LOOP_BACK_ENABLE                1
#define SSP_CONTROL_1_OVERRUN_INTERRUPT               2:2
#define SSP_CONTROL_1_OVERRUN_INTERRUPT_DISABLE       0
#define SSP_CONTROL_1_OVERRUN_INTERRUPT_ENABLE        1
#define SSP_CONTROL_1_TRANSMIT_INTERRUPT              1:1
#define SSP_CONTROL_1_TRANSMIT_INTERRUPT_DISABLE      0
#define SSP_CONTROL_1_TRANSMIT_INTERRUPT_ENABLE       1
#define SSP_CONTROL_1_RECEIVE_INTERRUPT               0:0
#define SSP_CONTROL_1_RECEIVE_INTERRUPT_DISABLE       0
#define SSP_CONTROL_1_RECEIVE_INTERRUPT_ENABLE        1

#define SSP_DATA                                      0x020008
#define SSP_DATA_DATA                                 15:0

#define SSP_STATUS                                    0x02000C
#define SSP_STATUS_STATUS                             4:4
#define SSP_STATUS_STATUS_IDLE                        0
#define SSP_STATUS_STATUS_BUSY                        1
#define SSP_STATUS_RECEIVE_FIFO                       3:2
#define SSP_STATUS_RECEIVE_FIFO_EMPTY                 0
#define SSP_STATUS_RECEIVE_FIFO_NOT_EMPTY             1
#define SSP_STATUS_RECEIVE_FIFO_FULL                  3
#define SSP_STATUS_TRANSMIT_FIFO                      1:0
#define SSP_STATUS_TRANSMIT_FIFO_FULL                 0
#define SSP_STATUS_TRANSMIT_FIFO_NOT_FULL             2
#define SSP_STATUS_TRANSMIT_FIFO_EMPTY                3

#define SSP_CLOCK_PRESCALE                            0x020010
#define SSP_CLOCK_PRESCALE_DIVISOR                    7:0

#define SSP_INTERRUPT_STATUS                          0x020014
#define SSP_INTERRUPT_STATUS_OVERRUN                  2:2
#define SSP_INTERRUPT_STATUS_OVERRUN_NOT_ACTIVE       0
#define SSP_INTERRUPT_STATUS_OVERRUN_ACTIVE           1
#define SSP_INTERRUPT_STATUS_OVERRUN_CLEAR            1
#define SSP_INTERRUPT_STATUS_TRANSMIT                 1:1
#define SSP_INTERRUPT_STATUS_TRANSMIT_NOT_ACTIVE      0
#define SSP_INTERRUPT_STATUS_TRANSMIT_ACTIVE          1
#define SSP_INTERRUPT_STATUS_RECEIVE                  0:0
#define SSP_INTERRUPT_STATUS_RECEIVE_NOT_ACTIVE       0
#define SSP_INTERRUPT_STATUS_RECEIVE_ACTIVE           1

/* Different register offset location between the first and the second SSP */
#define SSP0_SSP1_GAP              0x100
#define SSP1_OFFSET 0x100

/* There are 2 display channels in the system with the same definition,
   but different MMIO address as below
  
   Display Channel 0 0x080000
   Display Channel 1 0x088000
   Display Channel 2 0x098000

   We only define the MMIO for Display Channel 0, the MMIO for
   Display Channel 1 can be work out like this:
   0x080000 + CHANNEL_OFFSET, where CHANNEL_OFFSET is defined as 0x8000.
*/
#define LVDS_SET						  0x3ff0
#define LVDS_SET_LVDS1_ENABLE		  3:3
#define LVDS_SET_LVDS1_ENABLE_ON	   1
#define LVDS_SET_LVDS1_ENABLE_OFF	   0
#define LVDS_SET_LVDS1_PD			  2:2
#define LVDS_SET_LVDS1_PD_ON		 1
#define LVDS_SET_LVDS1_PD_OFF		 0
#define LVDS_SET_LVDS0_ENABLE		  1:1
#define LVDS_SET_LVDS0_ENABLE_ON	   1
#define LVDS_SET_LVDS0_ENABLE_OFF	   0
#define LVDS_SET_LVDS0_PD			  0:0
#define LVDS_SET_LVDS0_PD_ON		 1
#define LVDS_SET_LVDS0_PD_OFF		 0

#define DC_MUX								0x00003ffc
#define DC_MUX_HDMI0						3:0
#define DC_MUX_HDMI0_DC0					1
#define DC_MUX_HDMI0_DC1					2
#define DC_MUX_HDMI0_DC2					4

#define DC_MUX_HDMI1						7:4
#define DC_MUX_HDMI1_DC0					1
#define DC_MUX_HDMI1_DC1					2
#define DC_MUX_HDMI1_DC2					4

#define DC_MUX_HDMI2						11:8
#define DC_MUX_HDMI2_DC0					1
#define DC_MUX_HDMI2_DC1					2
#define DC_MUX_HDMI2_DC2					4

#define DC_MUX_DP0							15:12
#define DC_MUX_DP0_DC0					1
#define DC_MUX_DP0_DC1					2
#define DC_MUX_DP0_DC2					4

#define DC_MUX_DP1							19:16
#define DC_MUX_DP1_DC0					1
#define DC_MUX_DP1_DC1					2
#define DC_MUX_DP1_DC2					4

#define DC_MUX_LVDS0						23:20
#define DC_MUX_LVDS0_DC0					1
#define DC_MUX_LVDS0_DC1					2
#define DC_MUX_LVDS0_DC2					4

#define DC_MUX_LVDS1						27:24
#define DC_MUX_LVDS1_DC0					1
#define DC_MUX_LVDS1_DC1					2
#define DC_MUX_LVDS1_DC2					4

#define DISPLAY_CTRL                            0x080000
#define DISPLAY_CTRL_DPMS                         31:30
#define DISPLAY_CTRL_DPMS_VPHP                       0
#define DISPLAY_CTRL_DPMS_VPHN                       1
#define DISPLAY_CTRL_DPMS_VNHP                       2
#define DISPLAY_CTRL_DPMS_VNHN                       3
#define DISPLAY_CTRL_DATA_PATH                 29:29
#define DISPLAY_CTRL_DATA_PATH_VGA    	        0
#define DISPLAY_CTRL_DATA_PATH_EXTENDED         1
#define DISPLAY_CTRL_FPEN                       27:27
#define DISPLAY_CTRL_FPEN_LOW                   0
#define DISPLAY_CTRL_FPEN_HIGH                  1
#define DISPLAY_CTRL_VBIASEN                    26:26
#define DISPLAY_CTRL_VBIASEN_LOW                0
#define DISPLAY_CTRL_VBIASEN_HIGH               1
#define DISPLAY_CTRL_DATA                       25:25
#define DISPLAY_CTRL_DATA_DISABLE               0
#define DISPLAY_CTRL_DATA_ENABLE                1
#define DISPLAY_CTRL_FPVDDEN                    24:24
#define DISPLAY_CTRL_FPVDDEN_LOW                0
#define DISPLAY_CTRL_FPVDDEN_HIGH               1
#define DISPLAY_CTRL_LOOP_BACK_SELECT              			22:22
#define DISPLAY_CTRL_LOOP_BACK_SELECT_CHANNEL0        			0
#define DISPLAY_CTRL_LOOP_BACK_SELECT_CHANNEL1        			1
#define DISPLAY_CTRL_CLOCK_PHASE                14:14
#define DISPLAY_CTRL_CLOCK_PHASE_ACTIVE_HIGH    0
#define DISPLAY_CTRL_CLOCK_PHASE_ACTIVE_LOW     1
#define DISPLAY_CTRL_VSYNC_PHASE                13:13
#define DISPLAY_CTRL_VSYNC_PHASE_ACTIVE_HIGH    0
#define DISPLAY_CTRL_VSYNC_PHASE_ACTIVE_LOW     1
#define DISPLAY_CTRL_HSYNC_PHASE                12:12
#define DISPLAY_CTRL_HSYNC_PHASE_ACTIVE_HIGH    0
#define DISPLAY_CTRL_HSYNC_PHASE_ACTIVE_LOW     1
#define DISPLAY_CTRL_DOUBLE_PIXEL_CLOCK         11:11
#define DISPLAY_CTRL_DOUBLE_PIXEL_CLOCK_DISABLE 0
#define DISPLAY_CTRL_DOUBLE_PIXEL_CLOCK_ENABLE  1
#define DISPLAY_CTRL_COLOR_KEY                  9:9
#define DISPLAY_CTRL_COLOR_KEY_DISABLE          0
#define DISPLAY_CTRL_COLOR_KEY_ENABLE           1
#define DISPLAY_CTRL_TIMING                     8:8
#define DISPLAY_CTRL_TIMING_DISABLE             0
#define DISPLAY_CTRL_TIMING_ENABLE              1
#define DISPLAY_CTRL_PIXEL                      7:4
#define DISPLAY_CTRL_GAMMA                      3:3
#define DISPLAY_CTRL_GAMMA_DISABLE              0
#define DISPLAY_CTRL_GAMMA_ENABLE               1
#define DISPLAY_CTRL_PLANE                      2:2
#define DISPLAY_CTRL_PLANE_DISABLE              0
#define DISPLAY_CTRL_PLANE_ENABLE               1
#define DISPLAY_CTRL_FORMAT                     1:0
#define DISPLAY_CTRL_FORMAT_8                   0
#define DISPLAY_CTRL_FORMAT_16                  1
#define DISPLAY_CTRL_FORMAT_32                  2
#define DISPLAY_CTRL_FORMAT_YUV422              3

#define FB_ADDRESS                              0x080004
#define FB_ADDRESS_STATUS                       31:31
#define FB_ADDRESS_STATUS_CURRENT               0
#define FB_ADDRESS_STATUS_PENDING               1
#define FB_ADDRESS_ADDRESS                      30:0

#define FB_WIDTH                                0x080008
#define FB_WIDTH_WIDTH                          31:16
#define FB_WIDTH_OFFSET                         15:0

#define HORIZONTAL_TOTAL                        0x08000C
#define HORIZONTAL_TOTAL_PIPESEL				31:31
#define HORIZONTAL_TOTAL_DATASEQ				30:30
#define HORIZONTAL_TOTAL_DATASEQ_DISABLE		0
#define HORIZONTAL_TOTAL_DATASEQ_ENABLE			1
#define HORIZONTAL_TOTAL_VGA					29:29
#define HORIZONTAL_TOTAL_VGA_DISABLE			0
#define HORIZONTAL_TOTAL_VGA_ENABLE				1					
#define HORIZONTAL_TOTAL_TOTAL                  28:16
#define HORIZONTAL_TOTAL_DISPLAY_END            12:0

#define HORIZONTAL_SYNC                         0x080010
#define HORIZONTAL_SYNC_WIDTH                   25:16
#define HORIZONTAL_SYNC_START                   12:0

#define VERTICAL_TOTAL                          0x080014
#define VERTICAL_TOTAL_TOTAL                    27:16
#define VERTICAL_TOTAL_DISPLAY_END              11:0

#define VERTICAL_SYNC                           0x080018
#define VERTICAL_SYNC_DPMODE                    31:31
#define VERTICAL_SYNC_VSYNC                   	30:30
#define VERTICAL_SYNC_VSYNC_INACTIVE			0
#define VERTICAL_SYNC_VSYNC_ACTIVE				1
#define VERTICAL_SYNC_HEIGHT                    22:16
#define VERTICAL_SYNC_START                     11:0

#define CURRENT_LINE                            0x080020
#define CURRENT_LINE_LINE                       11:0

#define CRT_DETECT                     0x080024
#define CRT_DETECT_LVDS_CLK            26:26
#define CRT_DETECT_LVDS_CLK_POS		   0
#define CRT_DETECT_LVDS_CLK_NEG		   1
#define CRT_DETECT_CRT                 25:25
#define CRT_DETECT_CRT_ABSENT          0
#define CRT_DETECT_CRT_PRESENT         1
#define CRT_DETECT_ENABLE              24:24
#define CRT_DETECT_ENABLE_DISABLE      0
#define CRT_DETECT_ENABLE_ENABLE       1
#define CRT_DETECT_DATA_RED            23:16
#define CRT_DETECT_DATA_GREEN          15:8
#define CRT_DETECT_DATA_BLUE           7:0

#define COLOR_KEY                               0x080028
#define COLOR_KEY_MASK                          31:16
#define COLOR_KEY_VALUE                         15:0

/* Cursor Control */
#define HWC_CONTROL                             0x080030
#define HWC_CONTROL_MODE                        31:30
#define HWC_CONTROL_MODE_DISABLE                0
#define HWC_CONTROL_MODE_MASK                   1
#define HWC_CONTROL_MODE_MONO                   2
#define HWC_CONTROL_MODE_ALPHA                  3
#define HWC_CONTROL_ADDRESS                     29:0

#define HWC_LOCATION                            0x080034
#define HWC_LOCATION_TOP                        31:31
#define HWC_LOCATION_TOP_INSIDE                 0
#define HWC_LOCATION_TOP_OUTSIDE                1
#define HWC_LOCATION_SIZE                       30:30
#define HWC_LOCATION_SIZE_64                 	0
#define HWC_LOCATION_SIZE_128                	1
#define HWC_LOCATION_PREFETCH                   29:29
#define HWC_LOCATION_PREFETCH_DISABLE           0
#define HWC_LOCATION_PREFETCH_ENABLE            1
#define HWC_LOCATION_LASTPIXEL                  28:28
#define HWC_LOCATION_LASTPIXEL_DISABLE          0
#define HWC_LOCATION_LASTPIXEL_ENABLE           1


#define HWC_LOCATION_Y                          27:16
#define HWC_LOCATION_LEFT                       15:15
#define HWC_LOCATION_LEFT_INSIDE                0
#define HWC_LOCATION_LEFT_OUTSIDE               1
#define HWC_LOCATION_X                          11:0

#define HWC_COLOR0                              0x080038
#define HWC_COLOR0_RGB888                       23:0

#define HWC_COLOR1                              0x08003C
#define HWC_COLOR1_RGB888                       23:0

/* Video Control */
#define VIDEO_DISPLAY_CTRL                              0x080040
#define VIDEO_DISPLAY_CTRL_JPUP                         15:14
#define VIDEO_DISPLAY_CTRL_JPUP_DISABLE                 0
#define VIDEO_DISPLAY_CTRL_JPUP_HD                      1
#define VIDEO_DISPLAY_CTRL_JPUP_UHD                     2
#define VIDEO_DISPLAY_CTRL_BYTE_SWAP                    12:12
#define VIDEO_DISPLAY_CTRL_BYTE_SWAP_DISABLE            0
#define VIDEO_DISPLAY_CTRL_BYTE_SWAP_ENABLE             1
#define VIDEO_DISPLAY_CTRL_YUV444FMT                    10:10
#define VIDEO_DISPLAY_CTRL_YUV444FMT_DISABLE           	0
#define VIDEO_DISPLAY_CTRL_YUV444FMT_ENABLE            	1
#define VIDEO_DISPLAY_CTRL_VERTICAL_MODE                9:9
#define VIDEO_DISPLAY_CTRL_VERTICAL_MODE_REPLICATE      0
#define VIDEO_DISPLAY_CTRL_VERTICAL_MODE_INTERPOLATE    1
#define VIDEO_DISPLAY_CTRL_HORIZONTAL_MODE              8:8
#define VIDEO_DISPLAY_CTRL_HORIZONTAL_MODE_REPLICATE    0
#define VIDEO_DISPLAY_CTRL_HORIZONTAL_MODE_INTERPOLATE  1
#define VIDEO_DISPLAY_CTRL_PIXEL                        7:4
#define VIDEO_DISPLAY_CTRL_GAMMA                        3:3
#define VIDEO_DISPLAY_CTRL_GAMMA_DISABLE                0
#define VIDEO_DISPLAY_CTRL_GAMMA_ENABLE                 1
#define VIDEO_DISPLAY_CTRL_PLANE                        2:2
#define VIDEO_DISPLAY_CTRL_PLANE_DISABLE                0
#define VIDEO_DISPLAY_CTRL_PLANE_ENABLE                 1
#define VIDEO_DISPLAY_CTRL_FORMAT                       1:0
#define VIDEO_DISPLAY_CTRL_FORMAT_16                    0
#define VIDEO_DISPLAY_CTRL_FORMAT_32                    1
#define VIDEO_DISPLAY_CTRL_FORMAT_YUV422                2
#define VIDEO_DISPLAY_CTRL_FORMAT_YUV420                3

#define VIDEO_FB_ADDRESS                              0x080044
#define VIDEO_FB_ADDRESS_STATUS                       31:31
#define VIDEO_FB_ADDRESS_STATUS_CURRENT               0
#define VIDEO_FB_ADDRESS_STATUS_PENDING               1
#define VIDEO_FB_ADDRESS_ADDRESS                      30:0

#define VIDEO_FB_WIDTH                                0x080048
#define VIDEO_FB_WIDTH_WIDTH                          29:16
#define VIDEO_FB_WIDTH_OFFSET                         15:0

#define VIDEO_FB_ADDRESS_Y                            0x080044
#define VIDEO_FB_ADDRESS_Y_ADDRESS                    29:0

#define VIDEO_FB_WIDTH_Y                              0x080048
#define VIDEO_FB_WIDTH_Y_WIDTH                        29:16
#define VIDEO_FB_WIDTH_Y_OFFSET                       15:0

#define VIDEO_FB_ADDRESS_U                            0x08004C
#define VIDEO_FB_ADDRESS_U_ADDRESS                    30:0

#define VIDEO_FB_WIDTH_U                              0x080050
#define VIDEO_FB_WIDTH_U_WIDTH                        29:16
#define VIDEO_FB_WIDTH_U_OFFSET                       15:0

#define VIDEO_FB_ADDRESS_V                            0x080054
#define VIDEO_FB_ADDRESS_V_ADDRESS                    30:0

#define VIDEO_FB_WIDTH_V                              0x080058
#define VIDEO_FB_WIDTH_V_WIDTH                        29:16
#define VIDEO_FB_WIDTH_V_OFFSET                       15:0

#define VIDEO_YUV_CONSTANTS                           0x08005C
#define VIDEO_YUV_CONSTANTS_Y                         31:24
#define VIDEO_YUV_CONSTANTS_R                         23:16
#define VIDEO_YUV_CONSTANTS_G                         15:8
#define VIDEO_YUV_CONSTANTS_B                         7:0

#define VIDEO_PLANE_TL                                  0x080060
#define VIDEO_PLANE_TL_TOP                              27:16
#define VIDEO_PLANE_TL_LEFT                             12:0

#define VIDEO_PLANE_BR                                  0x080064
#define VIDEO_PLANE_BR_BOTTOM                           27:16
#define VIDEO_PLANE_BR_RIGHT                            12:0

#define VIDEO_SCALE                                     0x080068
#define VIDEO_SCALE_VERTICAL_SCALE                      27:16
#define VIDEO_SCALE_HORIZONTAL_SCALE                    11:0

#define VIDEO_INITIAL_SCALE                             0x08006C
#define VIDEO_INITIAL_SCALE_VERTICAL                    27:16
#define VIDEO_INITIAL_SCALE_HORIZONTAL                  11:0

#define VIDEO_SOURCE_SIZE                               0x080070
#define VIDEO_SOURCE_SIZE_4KPATCH                       31:31
#define VIDEO_SOURCE_SIZE_4KPATCH_DISABLE               0
#define VIDEO_SOURCE_SIZE_4KPATCH_ENABLE				1
#define VIDEO_SOURCE_SIZE_YUVPREFETCH                   30:30
#define VIDEO_SOURCE_SIZE_YUVPREFETCH_DISABLE           0
#define VIDEO_SOURCE_SIZE_YUVPREFETCH_ENABLE			1
#define VIDEO_SOURCE_SIZE_YUVARLENINC                   29:29
#define VIDEO_SOURCE_SIZE_YUVARLENINC_DISABLE           0
#define VIDEO_SOURCE_SIZE_YUVARLENINC_ENABLE			1
#define VIDEO_SOURCE_SIZE_YUVUVREDUCE                   28:28
#define VIDEO_SOURCE_SIZE_YUVUVREDUCE_DISABLE           0
#define VIDEO_SOURCE_SIZE_YUVUVREDUCE_ENABLE			1
#define VIDEO_SOURCE_SIZE_LASTLINE                      27:16
#define VIDEO_SOURCE_SIZE_LASTPIXEL                     11:0

/* Alpha Control */
#define ALPHA_DISPLAY_CTRL                              0x080080
#define ALPHA_DISPLAY_CTRL_SELECT                       28:28
#define ALPHA_DISPLAY_CTRL_SELECT_PER_PIXEL             0
#define ALPHA_DISPLAY_CTRL_SELECT_ALPHA                 1
#define ALPHA_DISPLAY_CTRL_ALPHA                        27:24
#define ALPHA_DISPLAY_CTRL_FIFO                         17:16
#define ALPHA_DISPLAY_CTRL_FIFO_1                       0
#define ALPHA_DISPLAY_CTRL_FIFO_3                       1
#define ALPHA_DISPLAY_CTRL_FIFO_7                       2
#define ALPHA_DISPLAY_CTRL_FIFO_11                      3
#define ALPHA_DISPLAY_CTRL_PIXEL                        7:4
#define ALPHA_DISPLAY_CTRL_CHROMA_KEY                   3:3
#define ALPHA_DISPLAY_CTRL_CHROMA_KEY_DISABLE           0
#define ALPHA_DISPLAY_CTRL_CHROMA_KEY_ENABLE            1
#define ALPHA_DISPLAY_CTRL_PLANE                        2:2
#define ALPHA_DISPLAY_CTRL_PLANE_DISABLE                0
#define ALPHA_DISPLAY_CTRL_PLANE_ENABLE                 1
#define ALPHA_DISPLAY_CTRL_FORMAT                       1:0
#define ALPHA_DISPLAY_CTRL_FORMAT_8               		0
#define ALPHA_DISPLAY_CTRL_FORMAT_16                    1
#define ALPHA_DISPLAY_CTRL_FORMAT_ALPHA_4_4             2
#define ALPHA_DISPLAY_CTRL_FORMAT_ALPHA_4_4_4_4         3

#define ALPHA_FB_ADDRESS                                0x080084
#define ALPHA_FB_ADDRESS_STATUS                         31:31
#define ALPHA_FB_ADDRESS_STATUS_CURRENT                 0
#define ALPHA_FB_ADDRESS_STATUS_PENDING                 1
#define ALPHA_FB_ADDRESS_ADDRESS                        29:0

#define ALPHA_FB_WIDTH                                  0x080088
#define ALPHA_FB_WIDTH_WIDTH                            29:16
#define ALPHA_FB_WIDTH_OFFSET                           13:0

#define ALPHA_PLANE_TL                                  0x08008C
#define ALPHA_PLANE_TL_TOP                              27:16
#define ALPHA_PLANE_TL_LEFT                             11:0

#define ALPHA_PLANE_BR                                  0x080090
#define ALPHA_PLANE_BR_BOTTOM                           27:16
#define ALPHA_PLANE_BR_RIGHT                            11:0

#define ALPHA_CHROMA_KEY                                0x080094
#define ALPHA_CHROMA_KEY_MASK                           31:16
#define ALPHA_CHROMA_KEY_VALUE                          15:0

#define ALPHA_COLOR_LOOKUP_01                           0x080098
#define ALPHA_COLOR_LOOKUP_01_1                         31:16
#define ALPHA_COLOR_LOOKUP_01_1_RED                     31:27
#define ALPHA_COLOR_LOOKUP_01_1_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_01_1_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_01_0                         15:0
#define ALPHA_COLOR_LOOKUP_01_0_RED                     15:11
#define ALPHA_COLOR_LOOKUP_01_0_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_01_0_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_23                           0x08009C
#define ALPHA_COLOR_LOOKUP_23_3                         31:16
#define ALPHA_COLOR_LOOKUP_23_3_RED                     31:27
#define ALPHA_COLOR_LOOKUP_23_3_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_23_3_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_23_2                         15:0
#define ALPHA_COLOR_LOOKUP_23_2_RED                     15:11
#define ALPHA_COLOR_LOOKUP_23_2_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_23_2_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_45                           0x0800a0
#define ALPHA_COLOR_LOOKUP_45_5                         31:16
#define ALPHA_COLOR_LOOKUP_45_5_RED                     31:27
#define ALPHA_COLOR_LOOKUP_45_5_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_45_5_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_45_4                         15:0
#define ALPHA_COLOR_LOOKUP_45_4_RED                     15:11
#define ALPHA_COLOR_LOOKUP_45_4_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_45_4_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_67                           0x0800a4
#define ALPHA_COLOR_LOOKUP_67_7                         31:16
#define ALPHA_COLOR_LOOKUP_67_7_RED                     31:27
#define ALPHA_COLOR_LOOKUP_67_7_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_67_7_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_67_6                         15:0
#define ALPHA_COLOR_LOOKUP_67_6_RED                     15:11
#define ALPHA_COLOR_LOOKUP_67_6_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_67_6_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_89                           0x0800a8
#define ALPHA_COLOR_LOOKUP_89_9                         31:16
#define ALPHA_COLOR_LOOKUP_89_9_RED                     31:27
#define ALPHA_COLOR_LOOKUP_89_9_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_89_9_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_89_8                         15:0
#define ALPHA_COLOR_LOOKUP_89_8_RED                     15:11
#define ALPHA_COLOR_LOOKUP_89_8_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_89_8_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_AB                           0x0800AC
#define ALPHA_COLOR_LOOKUP_AB_B                         31:16
#define ALPHA_COLOR_LOOKUP_AB_B_RED                     31:27
#define ALPHA_COLOR_LOOKUP_AB_B_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_AB_B_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_AB_A                         15:0
#define ALPHA_COLOR_LOOKUP_AB_A_RED                     15:11
#define ALPHA_COLOR_LOOKUP_AB_A_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_AB_A_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_CD                           0x0800B0
#define ALPHA_COLOR_LOOKUP_CD_D                         31:16
#define ALPHA_COLOR_LOOKUP_CD_D_RED                     31:27
#define ALPHA_COLOR_LOOKUP_CD_D_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_CD_D_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_CD_C                         15:0
#define ALPHA_COLOR_LOOKUP_CD_C_RED                     15:11
#define ALPHA_COLOR_LOOKUP_CD_C_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_CD_C_BLUE                    4:0

#define ALPHA_COLOR_LOOKUP_EF                           0x0800B4
#define ALPHA_COLOR_LOOKUP_EF_F                         31:16
#define ALPHA_COLOR_LOOKUP_EF_F_RED                     31:27
#define ALPHA_COLOR_LOOKUP_EF_F_GREEN                   26:21
#define ALPHA_COLOR_LOOKUP_EF_F_BLUE                    20:16
#define ALPHA_COLOR_LOOKUP_EF_E                         15:0
#define ALPHA_COLOR_LOOKUP_EF_E_RED                     15:11
#define ALPHA_COLOR_LOOKUP_EF_E_GREEN                   10:5
#define ALPHA_COLOR_LOOKUP_EF_E_BLUE                    4:0

#define HDMI_CONTROL									0x0800C0
#define HDMI_CONTROL_MODE_SELECT						7:4
#define HDMI_CONTROL_MODE_SELECT_A						1
#define HDMI_CONTROL_MODE_SELECT_B						2
#define HDMI_CONTROL_MODE_SELECT_D						4
#define HDMI_CONTROL_MODE_SELECT_E						8
#define HDMI_CONTROL_PLLB								3:3
#define HDMI_CONTROL_PLLB_NORMAL						0
#define HDMI_CONTROL_PLLB_RESET							1
#define HDMI_CONTROL_PLLA								2:2
#define HDMI_CONTROL_PLLA_NORMAL						0
#define HDMI_CONTROL_PLLA_RESET							1
#define HDMI_CONTROL_INTMODE							1:1
#define HDMI_CONTROL_INT_MODE_OPEN						0
#define HDMI_CONTROL_INT_MODE_PULL						1
#define HDMI_CONTROL_INT_POLARITY						0:0
#define HDMI_CONTROL_INT_POLARITY_LOW					0
#define HDMI_CONTROL_INT_POLARITY_HIGH					1

#define HDMI_CONFIG										0x0800C0
#define HDMI_CONFIG_READ								17:17
#define HDMI_CONFIG_READ_LATCH							0
#define HDMI_CONFIG_READ_ENABLE							1
#define HDMI_CONFIG_WRITE								16:16
#define HDMI_CONFIG_WRITE_LATCH							0
#define HDMI_CONFIG_WRITE_ENABLE						1
#define HDMI_CONFIG_DATA								15:8
#define HDMI_CONFIG_ADDRESS								7:0

#define DISPLAY_YUV_CONVERSION_1						0x0800D4  //Default value:0x726f5d12
#define DISPLAY_YUV_CONVERSION_1_UB                     31:24
#define DISPLAY_YUV_CONVERSION_1_VR                     23:16
#define DISPLAY_YUV_CONVERSION_1_VG 					15:8
#define DISPLAY_YUV_CONVERSION_1_VB 					7:0


#define DISPLAY_YUV_CONVERSION_2						0x0800D8 //Default value:0x80182645
#define DISPLAY_YUV_CONVERSION_2_YG                     31:24
#define DISPLAY_YUV_CONVERSION_2_YB                     23:16
#define DISPLAY_YUV_CONVERSION_2_UR 					15:8
#define DISPLAY_YUV_CONVERSION_2_UG 					7:0

#define DISPLAY_YUV_CONVERSION_3						0x0800DC  //Default value:0x40
#define DISPLAY_YUV_CONVERSION_3_R2YCTRL                10:8
#define DISPLAY_YUV_CONVERSION_3_R2YCTRL_RGB888         0
#define DISPLAY_YUV_CONVERSION_3_R2YCTRL_YUV422         1
#define DISPLAY_YUV_CONVERSION_3_R2YCTRL_RGB666			2
#define DISPLAY_YUV_CONVERSION_3_YR						7:0

#define DISPLAY_YUV										0x0800E0  //Default value:0x1000
#define DISPLAY_YUV_BRIGHTNESS	                		19:0

/* Palette RAM */
#define PALETTE_RAM                             0x080C00

/* Distance between channel 1 and channel 2 display control */
#define CHANNEL_OFFSET 0x8000
#define CHANNEL_OFFSET2 0x18000
// #define CHANNEL_OFFSET2 0x8000
#define DC_OFFSET 0x8000
#define DC_OFFSET2 0x18000
// #define DC_OFFSET2 0x8000

#define DMA0_DESTINATION                                0x0D0000
#define DMA0_DESTINATION_DIRECTION						31:31   								
#define DMA0_DESTINATION_DIRECTION_TODDR				0
#define DMA0_DESTINATION_DIRECTION_TOTCM				1
#define DMA0_DESTINATION_TCM							25:24
#define DMA0_DESTINATION_TCM_CPU0						1
#define DMA0_DESTINATION_TCM_CPU1						3
#define DMA0_DESTINATION_ADDRESS                        29:0

#define DMA0_SOURCE                                     0x0D0004
#define DMA0_SOURCE_ADDRESS                             23:0

#define DMA0_CONTROL                                    0x0D0008
#define DMA0_CONTROL_STATUS                             31:31
#define DMA0_CONTROL_STATUS_IDLE                        0
#define DMA0_CONTROL_STATUS_ENABLE                      1
#define DMA0_CONTROL_SIZE                               23:0

/* DMA 2 can transfer from:
   DDR to SRAM.
   SRAM to DDR.
*/
#define DMA2_SOURCE                                     0x0D8010
#define DMA2_SOURCE_SEL                                31:31 
#define DMA2_SOURCE_SEL_DDR                          	0
#define DMA2_SOURCE_SEL_SRAM                         	1  
#define DMA2_SOURCE_ADDRESS                            30:0

#define DMA2_SOURCE_SIZE                               0x0D8014
#define DMA2_SOURCE_SIZE_SIZE                          23:0

#define DMA2_DESTINATION                                0x0D8018
#define DMA2_DESTINATION_SEL                            31:31
#define DMA2_DESTINATION_SEL_DDR                      	0
#define DMA2_DESTINATION_SEL_SRAM                     	1 
#define DMA2_DESTINATION_ADDRESS                        29:0

#define DMA2_CONTROL                                    0x0D801C
#define DMA2_CONTROL_STATUS                             31:31
#define DMA2_CONTROL_STATUS_IDLE                        0 
#define DMA2_CONTROL_STATUS_ENABLE                      1 
#define DMA2_CONTROL_TRI_STREAM                         30:30
#define DMA2_CONTROL_TRI_STREAM_DISABLE                 0
#define DMA2_CONTROL_TRI_STREAM_ENABLE                  1
#define DMA2_CONTROL_FORMAT                             29:28
#define DMA2_CONTROL_FORMAT_8BPP                        0
#define DMA2_CONTROL_FORMAT_16BPP                       1
#define DMA2_CONTROL_FORMAT_32BPP                       2
#define DMA2_CONTROL_TILE_HEIGHT                        27:16
#define DMA2_CONTROL_TILE_WIDTH                         12:0

#define DMA2_DESTINATION_PITCH                          0x0D8020
#define DMA2_DESTINATION_PITCH_PITCH                    14:0

#define DMA2_INTCTL                                     0x0D8024
#define DMA2_INTCTL_DMA2_STATUS                         1:1
#define DMA2_INTCTL_DMA2_STATUS_NORMAL                  0
#define DMA2_INTCTL_DMA2_STATUS_ABORT                   1
#define DMA2_INTCTL_DMA2_RAWINT                         0:0
#define DMA2_INTCTL_DMA2_RAWINT_CLEAR                   0
#define DMA2_INTCTL_DMA2_RAWINT_SET                     1

/* DMA 1 can transfer from:
   System to local DDR.
   local DDR to system.
   local DDR to local DDR.

   The source is alwasy linear and the destination is always tiled.
*/
#define DMA1_SOURCE0                                    0x0D0010
#define DMA1_SOURCE0_SEL                                31:31 
#define DMA1_SOURCE0_SEL_LOCAL                          0
#define DMA1_SOURCE0_SEL_SYSTEM                         1  
#define DMA1_SOURCE0_DECODE                             30:30
#define DMA1_SOURCE0_DECODE_DISABLE                     0
#define DMA1_SOURCE0_DECODE_ENABLE                      1
#define DMA1_SOURCE0_ADDRESS                            29:0

#define DMA1_SOURCE0_SIZE                               0x0D0014
#define DMA1_SOURCE0_SIZE_NSC_CSS                       27:27
#define DMA1_SOURCE0_SIZE_NSC_CSS_DISABLE               0
#define DMA1_SOURCE0_SIZE_NSC_CSS_ENABLE                1
#define DMA1_SOURCE0_SIZE_NSC_CLL                       26:24
#define DMA1_SOURCE0_SIZE_SIZE                          23:0

#define DMA1_DESTINATION                                0x0D0018
#define DMA1_DESTINATION_SEL                            31:31
#define DMA1_DESTINATION_SEL_LOCAL                      0
#define DMA1_DESTINATION_SEL_SYSTEM                     1 
#define DMA1_DESTINATION_ADDRESS                        29:0

#define DMA1_CONTROL                                    0x0D001C
#define DMA1_CONTROL_STATUS                             31:31
#define DMA1_CONTROL_STATUS_IDLE                        0 
#define DMA1_CONTROL_STATUS_ENABLE                      1 
#define DMA1_CONTROL_TRI_STREAM                         30:30
#define DMA1_CONTROL_TRI_STREAM_DISABLE                 0
#define DMA1_CONTROL_TRI_STREAM_ENABLE                  1
#define DMA1_CONTROL_FORMAT                             29:28
#define DMA1_CONTROL_FORMAT_8BPP                        0
#define DMA1_CONTROL_FORMAT_16BPP                       1
#define DMA1_CONTROL_FORMAT_32BPP                       2
#define DMA1_CONTROL_TILE_HEIGHT                        27:16
#define DMA1_CONTROL_TILE_WIDTH                         12:0

#define DMA1_DESTINATION_PITCH                          0x0D0020
#define DMA1_DESTINATION_PITCH_PITCH                    14:0

#define DMA_CONTROL                                     0x0D0024
#define DMA_CONTROL_NSC_RB_SWAP                         11:11
#define DMA_CONTROL_NSC_RB_SWAP_DISABLE                 0
#define DMA_CONTROL_NSC_RB_SWAP_ENABLE                  1
#define DMA_CONTROL_CSC                                 10:10
#define DMA_CONTROL_CSC_DISABLE                         0
#define DMA_CONTROL_CSC_ENABLE                          1
#define DMA_CONTROL_DECOMP                              9:8
#define DMA_CONTROL_DECOMP_TEXT                         0
#define DMA_CONTROL_DECOMP_NSC                          1
#define DMA_CONTROL_DECOMP_GOLOMB                       2
#define DMA_CONTROL_DMA1_STATUS                         5:5
#define DMA_CONTROL_DMA1_STATUS_NORMAL                  0
#define DMA_CONTROL_DMA1_STATUS_ABORT                   1
#define DMA_CONTROL_DMA0_STATUS                         4:4
#define DMA_CONTROL_DMA0_STATUS_NORMAL                  0
#define DMA_CONTROL_DMA0_STATUS_ABORT                   1
#define DMA_CONTROL_DMA1_RAWINT                         1:1
#define DMA_CONTROL_DMA1_RAWINT_CLEAR                   0
#define DMA_CONTROL_DMA1_RAWINT_SET                     1
#define DMA_CONTROL_DMA0_RAWINT                         0:0
#define DMA_CONTROL_DMA0_RAWINT_CLEAR                   0
#define DMA_CONTROL_DMA0_RAWINT_SET                     1

/* DMA 1 source 1 and source 2 are only used when RGB is in 3 separate planes.*/
#define DMA1_SOURCE1                                    0x0D0028
#define DMA1_SOURCE1_ADDRESS                            29:0

#define DMA1_SOURCE1_SIZE                               0x0D002c
#define DMA1_SOURCE1_SIZE_NSC_CSS                       27:27
#define DMA1_SOURCE1_SIZE_NSC_CSS_DISABLE               0
#define DMA1_SOURCE1_SIZE_NSC_CSS_ENABLE                1
#define DMA1_SOURCE1_SIZE_NSC_CLL                       26:24
#define DMA1_SOURCE1_SIZE_SIZE                          23:0

#define DMA1_SOURCE2                                    0x0D0030
#define DMA1_SOURCE2_ADDRESS                            29:0

#define DMA1_SOURCE2_SIZE                               0x0D0034
#define DMA1_SOURCE2_SIZE_NSC_CSS                       27:27
#define DMA1_SOURCE2_SIZE_NSC_CSS_DISABLE               0
#define DMA1_SOURCE2_SIZE_NSC_CSS_ENABLE                1
#define DMA1_SOURCE2_SIZE_NSC_CLL                       26:24
#define DMA1_SOURCE2_SIZE_SIZE                          23:0


/* 2D registers. */

#define DE_SOURCE                                       0x100000
#define DE_SOURCE_WRAP                                  31:31
#define DE_SOURCE_WRAP_DISABLE                          0
#define DE_SOURCE_WRAP_ENABLE                           1

/* 
 * The following definitions are used in different setting 
 */

/* Use these definitions in XY addressing mode or linear addressing mode. */
#define DE_SOURCE_X_K1                                  27:16
#define DE_SOURCE_Y_K2                                  11:0

/* Use this definition in host write mode for mono. The Y_K2 is not used
   in host write mode. */
#define DE_SOURCE_X_K1_MONO                             20:16

/* Use these definitions in Bresenham line drawing mode. */
#define DE_SOURCE_X_K1_LINE                             29:16
#define DE_SOURCE_Y_K2_LINE                             13:0

#define DE_DESTINATION                                  0x100004
#define DE_DESTINATION_WRAP                             31:31
#define DE_DESTINATION_WRAP_DISABLE                     0
#define DE_DESTINATION_WRAP_ENABLE                      1
#if 1
    #define DE_DESTINATION_X                            27:16
    #define DE_DESTINATION_Y                            11:0
#else
    #define DE_DESTINATION_X                            28:16
    #define DE_DESTINATION_Y                            15:0
#endif

#define DE_DIMENSION                                    0x100008
#define DE_DIMENSION_X                                  28:16
#define DE_DIMENSION_Y_ET                               15:0

#define DE_CONTROL                                      0x10000C
#define DE_CONTROL_STATUS                               31:31
#define DE_CONTROL_STATUS_STOP                          0
#define DE_CONTROL_STATUS_START                         1
#define DE_CONTROL_PATTERN                              30:30
#define DE_CONTROL_PATTERN_MONO                         0
#define DE_CONTROL_PATTERN_COLOR                        1
#define DE_CONTROL_UPDATE_DESTINATION_X                 29:29
#define DE_CONTROL_UPDATE_DESTINATION_X_DISABLE         0
#define DE_CONTROL_UPDATE_DESTINATION_X_ENABLE          1
#define DE_CONTROL_QUICK_START                          28:28
#define DE_CONTROL_QUICK_START_DISABLE                  0
#define DE_CONTROL_QUICK_START_ENABLE                   1
#define DE_CONTROL_DIRECTION                            27:27
#define DE_CONTROL_DIRECTION_LEFT_TO_RIGHT              0
#define DE_CONTROL_DIRECTION_RIGHT_TO_LEFT              1
#define DE_CONTROL_MAJOR                                26:26
#define DE_CONTROL_MAJOR_X                              0
#define DE_CONTROL_MAJOR_Y                              1
#define DE_CONTROL_STEP_X                               25:25
#define DE_CONTROL_STEP_X_POSITIVE                      0
#define DE_CONTROL_STEP_X_NEGATIVE                      1
#define DE_CONTROL_STEP_Y                               24:24
#define DE_CONTROL_STEP_Y_POSITIVE                      0
#define DE_CONTROL_STEP_Y_NEGATIVE                      1
#define DE_CONTROL_STRETCH                              23:23
#define DE_CONTROL_STRETCH_DISABLE                      0
#define DE_CONTROL_STRETCH_ENABLE                       1
#define DE_CONTROL_HOST                                 22:22
#define DE_CONTROL_HOST_COLOR                           0
#define DE_CONTROL_HOST_MONO                            1
#define DE_CONTROL_LAST_PIXEL                           21:21
#define DE_CONTROL_LAST_PIXEL_OFF                       0
#define DE_CONTROL_LAST_PIXEL_ON                        1
#define DE_CONTROL_COMMAND                              20:16
#define DE_CONTROL_COMMAND_BITBLT                       0
#define DE_CONTROL_COMMAND_RECTANGLE_FILL               1
#define DE_CONTROL_COMMAND_DE_TILE                      2
#define DE_CONTROL_COMMAND_TRAPEZOID_FILL               3
#define DE_CONTROL_COMMAND_ALPHA_BLEND                  4
#define DE_CONTROL_COMMAND_RLE_STRIP                    5
#define DE_CONTROL_COMMAND_SHORT_STROKE                 6
#define DE_CONTROL_COMMAND_LINE_DRAW                    7
#define DE_CONTROL_COMMAND_HOST_WRITE                   8
#define DE_CONTROL_COMMAND_HOST_READ                    9
#define DE_CONTROL_COMMAND_HOST_WRITE_BOTTOM_UP         10
#define DE_CONTROL_COMMAND_ROTATE                       11
#define DE_CONTROL_COMMAND_FONT                         12
#define DE_CONTROL_COMMAND_TEXTURE_LOAD                 15
#define DE_CONTROL_ROP_SELECT                           15:15
#define DE_CONTROL_ROP_SELECT_ROP3                      0
#define DE_CONTROL_ROP_SELECT_ROP2                      1
#define DE_CONTROL_ROP2_SOURCE                          14:14
#define DE_CONTROL_ROP2_SOURCE_BITMAP                   0
#define DE_CONTROL_ROP2_SOURCE_PATTERN                  1
#define DE_CONTROL_MONO_DATA                            13:12
#define DE_CONTROL_MONO_DATA_NOT_PACKED                 0
#define DE_CONTROL_MONO_DATA_8_PACKED                   1
#define DE_CONTROL_MONO_DATA_16_PACKED                  2
#define DE_CONTROL_MONO_DATA_32_PACKED                  3
#define DE_CONTROL_REPEAT_ROTATE                        11:11
#define DE_CONTROL_REPEAT_ROTATE_DISABLE                0
#define DE_CONTROL_REPEAT_ROTATE_ENABLE                 1
#define DE_CONTROL_TRANSPARENCY_MATCH                   10:10
#define DE_CONTROL_TRANSPARENCY_MATCH_OPAQUE            0
#define DE_CONTROL_TRANSPARENCY_MATCH_TRANSPARENT       1
#define DE_CONTROL_TRANSPARENCY_SELECT                  9:9
#define DE_CONTROL_TRANSPARENCY_SELECT_SOURCE           0
#define DE_CONTROL_TRANSPARENCY_SELECT_DESTINATION      1
#define DE_CONTROL_TRANSPARENCY                         8:8
#define DE_CONTROL_TRANSPARENCY_DISABLE                 0
#define DE_CONTROL_TRANSPARENCY_ENABLE                  1
#define DE_CONTROL_ROP                                  7:0

/* Pseudo fields. */

#define DE_CONTROL_SHORT_STROKE_DIR                     27:24
#define DE_CONTROL_SHORT_STROKE_DIR_225                 0
#define DE_CONTROL_SHORT_STROKE_DIR_135                 1
#define DE_CONTROL_SHORT_STROKE_DIR_315                 2
#define DE_CONTROL_SHORT_STROKE_DIR_45                  3
#define DE_CONTROL_SHORT_STROKE_DIR_270                 4
#define DE_CONTROL_SHORT_STROKE_DIR_90                  5
#define DE_CONTROL_SHORT_STROKE_DIR_180                 8
#define DE_CONTROL_SHORT_STROKE_DIR_0                   10
#define DE_CONTROL_ROTATION                             25:24
#define DE_CONTROL_ROTATION_0                           0
#define DE_CONTROL_ROTATION_270                         1
#define DE_CONTROL_ROTATION_90                          2
#define DE_CONTROL_ROTATION_180                         3

#define DE_PITCH                                        0x100010
#define DE_PITCH_DESTINATION                            28:16
#define DE_PITCH_SOURCE                                 12:0

#define DE_FOREGROUND                                   0x100014
#define DE_FOREGROUND_COLOR                             31:0

#define DE_BACKGROUND                                   0x100018
#define DE_BACKGROUND_COLOR                             31:0

#define DE_STRETCH_FORMAT                               0x10001C
#define DE_STRETCH_FORMAT_PATTERN_XY                    30:30
#define DE_STRETCH_FORMAT_PATTERN_XY_NORMAL             0
#define DE_STRETCH_FORMAT_PATTERN_XY_OVERWRITE          1
#define DE_STRETCH_FORMAT_PATTERN_Y                     29:27
#define DE_STRETCH_FORMAT_PATTERN_X                     25:23
#define DE_STRETCH_FORMAT_PIXEL_FORMAT                  21:20
#define DE_STRETCH_FORMAT_PIXEL_FORMAT_8                0
#define DE_STRETCH_FORMAT_PIXEL_FORMAT_16               1
#define DE_STRETCH_FORMAT_PIXEL_FORMAT_32               2
#define DE_STRETCH_FORMAT_ADDRESSING                    19:16
#define DE_STRETCH_FORMAT_ADDRESSING_XY                 0
#define DE_STRETCH_FORMAT_ADDRESSING_LINEAR             15
#define DE_STRETCH_FORMAT_SOURCE_HEIGHT                 11:0

#define DE_COLOR_COMPARE                                0x100020
#define DE_COLOR_COMPARE_COLOR                          23:0

#define DE_COLOR_COMPARE_MASK                           0x100024
#define DE_COLOR_COMPARE_MASK_MASKS                     23:0

#define DE_MASKS                                        0x100028
#define DE_MASKS_BYTE_MASK                              31:16
#define DE_MASKS_BIT_MASK                               15:0

#define DE_CLIP_TL                                      0x10002C
#define DE_CLIP_TL_TOP                                  31:16
#define DE_CLIP_TL_STATUS                               13:13
#define DE_CLIP_TL_STATUS_DISABLE                       0
#define DE_CLIP_TL_STATUS_ENABLE                        1
#define DE_CLIP_TL_INHIBIT                              12:12
#define DE_CLIP_TL_INHIBIT_OUTSIDE                      0
#define DE_CLIP_TL_INHIBIT_INSIDE                       1
#define DE_CLIP_TL_LEFT                                 11:0

#define DE_CLIP_BR                                      0x100030
#define DE_CLIP_BR_BOTTOM                               31:16
#define DE_CLIP_BR_RIGHT                                12:0

#define DE_MONO_PATTERN_LOW                             0x100034
#define DE_MONO_PATTERN_LOW_PATTERN                     31:0

#define DE_MONO_PATTERN_HIGH                            0x100038
#define DE_MONO_PATTERN_HIGH_PATTERN                    31:0

#define DE_WINDOW_WIDTH                                 0x10003C
#define DE_WINDOW_WIDTH_DESTINATION                     28:16
#define DE_WINDOW_WIDTH_SOURCE                          12:0

#define DE_WINDOW_SOURCE_BASE                           0x100040
#define DE_WINDOW_SOURCE_BASE_ADDRESS                   29:0

#define DE_WINDOW_DESTINATION_BASE                      0x100044
#define DE_WINDOW_DESTINATION_BASE_ADDRESS              29:0

#define DE_ALPHA                                        0x100048
#define DE_ALPHA_VALUE                                  7:0

#define DE_WRAP                                         0x10004C
#define DE_WRAP_X                                       31:16
#define DE_WRAP_Y                                       15:0

#define DE_STATE2                                        0x100054
#define DE_STATE2_DE_FIFO                                3:3
#define DE_STATE2_DE_FIFO_NOTEMPTY                       1
#define DE_STATE2_DE_FIFO_EMPTY                          0
#define DE_STATE2_DE_STATUS                              2:2
#define DE_STATE2_DE_STATUS_IDLE                         0
#define DE_STATE2_DE_STATUS_BUSY                         1
#define DE_STATE2_DE_MEM_FIFO                            1:1
#define DE_STATE2_DE_MEM_FIFO_NOTEMPTY                   0
#define DE_STATE2_DE_MEM_FIFO_EMPTY                      1
#define DE_STATE2_DE_ABORT                               0:0
#define DE_STATE2_DE_ABORT_OFF                           0
#define DE_STATE2_DE_ABORT_ON                            1

/* Color Space Conversion registers. */

#define CSC_Y_SOURCE_BASE                               0x1000C8
#define CSC_Y_SOURCE_BASE_ADDRESS                       29:0

#define CSC_CONSTANTS                                   0x1000CC
#define CSC_CONSTANTS_Y                                 31:24
#define CSC_CONSTANTS_R                                 23:16
#define CSC_CONSTANTS_G                                 15:8
#define CSC_CONSTANTS_B                                 7:0

#define CSC_Y_SOURCE_X                                  0x1000D0
#define CSC_Y_SOURCE_X_INTEGER                          27:16
#define CSC_Y_SOURCE_X_FRACTION                         15:3

#define CSC_Y_SOURCE_Y                                  0x1000D4
#define CSC_Y_SOURCE_Y_INTEGER                          27:16
#define CSC_Y_SOURCE_Y_FRACTION                         15:3

#define CSC_U_SOURCE_BASE                               0x1000D8
#define CSC_U_SOURCE_BASE_ADDRESS                       29:0

#define CSC_V_SOURCE_BASE                               0x1000DC
#define CSC_V_SOURCE_BASE_ADDRESS                       29:0

#define CSC_SOURCE_DIMENSION                            0x1000E0
#define CSC_SOURCE_DIMENSION_X                          31:16
#define CSC_SOURCE_DIMENSION_Y                          15:0

#define CSC_SOURCE_PITCH                                0x1000E4
#define CSC_SOURCE_PITCH_Y                              31:16
#define CSC_SOURCE_PITCH_UV                             15:0

#define CSC_DESTINATION                                 0x1000E8
#define CSC_DESTINATION_WRAP                            31:31
#define CSC_DESTINATION_WRAP_DISABLE                    0
#define CSC_DESTINATION_WRAP_ENABLE                     1
#define CSC_DESTINATION_X                               27:16
#define CSC_DESTINATION_Y                               11:0

#define CSC_DESTINATION_DIMENSION                       0x1000EC
#define CSC_DESTINATION_DIMENSION_X                     31:16
#define CSC_DESTINATION_DIMENSION_Y                     15:0

#define CSC_DESTINATION_PITCH                           0x1000F0
#define CSC_DESTINATION_PITCH_X                         31:16
#define CSC_DESTINATION_PITCH_Y                         15:0

#define CSC_SCALE_FACTOR                                0x1000F4
#define CSC_SCALE_FACTOR_HORIZONTAL                     31:16
#define CSC_SCALE_FACTOR_VERTICAL                       15:0

#define CSC_DESTINATION_BASE                            0x1000F8
#define CSC_DESTINATION_BASE_ADDRESS                    29:0

#define CSC_CONTROL                                     0x1000FC
#define CSC_CONTROL_STATUS                              31:31
#define CSC_CONTROL_STATUS_STOP                         0
#define CSC_CONTROL_STATUS_START                        1
#define CSC_CONTROL_SOURCE_FORMAT                       30:28
#define CSC_CONTROL_SOURCE_FORMAT_YUV422                0
#define CSC_CONTROL_SOURCE_FORMAT_YUV420I               1
#define CSC_CONTROL_SOURCE_FORMAT_YUV420                2
#define CSC_CONTROL_SOURCE_FORMAT_YVU9                  3
#define CSC_CONTROL_SOURCE_FORMAT_IYU1                  4
#define CSC_CONTROL_SOURCE_FORMAT_IYU2                  5
#define CSC_CONTROL_SOURCE_FORMAT_RGB565                6
#define CSC_CONTROL_SOURCE_FORMAT_RGB8888               7
#define CSC_CONTROL_DESTINATION_FORMAT                  27:26
#define CSC_CONTROL_DESTINATION_FORMAT_RGB565           0
#define CSC_CONTROL_DESTINATION_FORMAT_RGB8888          1
#define CSC_CONTROL_HORIZONTAL_FILTER                   25:25
#define CSC_CONTROL_HORIZONTAL_FILTER_DISABLE           0
#define CSC_CONTROL_HORIZONTAL_FILTER_ENABLE            1
#define CSC_CONTROL_VERTICAL_FILTER                     24:24
#define CSC_CONTROL_VERTICAL_FILTER_DISABLE             0
#define CSC_CONTROL_VERTICAL_FILTER_ENABLE              1
#define CSC_CONTROL_BYTE_ORDER                          23:23
#define CSC_CONTROL_BYTE_ORDER_YUYV                     0
#define CSC_CONTROL_BYTE_ORDER_UYVY                     1

#define DE_DATA_PORT                                    0x110000


#define ZV0_CAPTURE_CTRL                                0x090000
#define ZV0_CAPTURE_CTRL_FIELD_INPUT                    27:27
#define ZV0_CAPTURE_CTRL_FIELD_INPUT_EVEN_FIELD         0
#define ZV0_CAPTURE_CTRL_FIELD_INPUT_ODD_FIELD          1
#define ZV0_CAPTURE_CTRL_SCAN                           26:26
#define ZV0_CAPTURE_CTRL_SCAN_PROGRESSIVE               0
#define ZV0_CAPTURE_CTRL_SCAN_INTERLACE                 1
#define ZV0_CAPTURE_CTRL_CURRENT_BUFFER                 25:25
#define ZV0_CAPTURE_CTRL_CURRENT_BUFFER_0               0
#define ZV0_CAPTURE_CTRL_CURRENT_BUFFER_1               1
#define ZV0_CAPTURE_CTRL_VERTICAL_SYNC                  24:24
#define ZV0_CAPTURE_CTRL_VERTICAL_SYNC_INACTIVE         0
#define ZV0_CAPTURE_CTRL_VERTICAL_SYNC_ACTIVE           1
#define ZV0_CAPTURE_CTRL_OUTPUT_FORMAT                  22:22
#define ZV0_CAPTURE_CTRL_OUTPUT_FORMAT_16               0
#define ZV0_CAPTURE_CTRL_OUTPUT_FORMAT_32               1
#define ZV0_CAPTURE_CTRL_INCOME_DATA                  	21:21
#define ZV0_CAPTURE_CTRL_INCOME_DATA_16                 0
#define ZV0_CAPTURE_CTRL_INCOME_DATA_32	                1
#define ZV0_CAPTURE_CTRL_ADJ                            19:19
#define ZV0_CAPTURE_CTRL_ADJ_NORMAL                     0
#define ZV0_CAPTURE_CTRL_ADJ_DELAY                      1
#define ZV0_CAPTURE_CTRL_HA                             18:18
#define ZV0_CAPTURE_CTRL_HA_DISABLE                     0
#define ZV0_CAPTURE_CTRL_HA_ENABLE                      1
#define ZV0_CAPTURE_CTRL_VSK                            17:17
#define ZV0_CAPTURE_CTRL_VSK_DISABLE                    0
#define ZV0_CAPTURE_CTRL_VSK_ENABLE                     1
#define ZV0_CAPTURE_CTRL_HSK                            16:16
#define ZV0_CAPTURE_CTRL_HSK_DISABLE                    0
#define ZV0_CAPTURE_CTRL_HSK_ENABLE                     1
#define ZV0_CAPTURE_CTRL_FD                             15:15
#define ZV0_CAPTURE_CTRL_FD_RISING                      0
#define ZV0_CAPTURE_CTRL_FD_FALLING                     1
#define ZV0_CAPTURE_CTRL_VP                             14:14
#define ZV0_CAPTURE_CTRL_VP_HIGH                        0
#define ZV0_CAPTURE_CTRL_VP_LOW                         1
#define ZV0_CAPTURE_CTRL_HP                             13:13
#define ZV0_CAPTURE_CTRL_HP_HIGH                        0
#define ZV0_CAPTURE_CTRL_HP_LOW                         1
#define ZV0_CAPTURE_CTRL_CP                             12:12
#define ZV0_CAPTURE_CTRL_CP_HIGH                        0
#define ZV0_CAPTURE_CTRL_CP_LOW                         1
#define ZV0_CAPTURE_CTRL_UVS                            11:11
#define ZV0_CAPTURE_CTRL_UVS_DISABLE                    0
#define ZV0_CAPTURE_CTRL_UVS_ENABLE                     1
#define ZV0_CAPTURE_CTRL_BS                             10:10
#define ZV0_CAPTURE_CTRL_BS_DISABLE                     0
#define ZV0_CAPTURE_CTRL_BS_ENABLE                      1
#define ZV0_CAPTURE_CTRL_CS                             9:9
#define ZV0_CAPTURE_CTRL_CS_16                          0
#define ZV0_CAPTURE_CTRL_CS_8                           1
#define ZV0_CAPTURE_CTRL_CF                             8:8
#define ZV0_CAPTURE_CTRL_CF_YUV                         0
#define ZV0_CAPTURE_CTRL_CF_RGB                         1
#define ZV0_CAPTURE_CTRL_FS                             7:7
#define ZV0_CAPTURE_CTRL_FS_DISABLE                     0
#define ZV0_CAPTURE_CTRL_FS_ENABLE                      1
#define ZV0_CAPTURE_CTRL_WEAVE                          6:6
#define ZV0_CAPTURE_CTRL_WEAVE_DISABLE                  0
#define ZV0_CAPTURE_CTRL_WEAVE_ENABLE                   1
#define ZV0_CAPTURE_CTRL_BOB                            5:5
#define ZV0_CAPTURE_CTRL_BOB_DISABLE                    0
#define ZV0_CAPTURE_CTRL_BOB_ENABLE                     1
#define ZV0_CAPTURE_CTRL_DB                             4:4
#define ZV0_CAPTURE_CTRL_DB_DISABLE                     0
#define ZV0_CAPTURE_CTRL_DB_ENABLE                      1
#define ZV0_CAPTURE_CTRL_CC                             3:3
#define ZV0_CAPTURE_CTRL_CC_CONTINUE                    0
#define ZV0_CAPTURE_CTRL_CC_CONDITION                   1
#define ZV0_CAPTURE_CTRL_RGB                            2:2
#define ZV0_CAPTURE_CTRL_RGB_DISABLE                    0
#define ZV0_CAPTURE_CTRL_RGB_ENABLE                     1
#define ZV0_CAPTURE_CTRL_656                            1:1
#define ZV0_CAPTURE_CTRL_656_DISABLE                    0
#define ZV0_CAPTURE_CTRL_656_ENABLE                     1
#define ZV0_CAPTURE_CTRL_CAP                            0:0
#define ZV0_CAPTURE_CTRL_CAP_DISABLE                    0
#define ZV0_CAPTURE_CTRL_CAP_ENABLE                     1

#define ZV0_CAPTURE_CLIP                                0x090004
#define ZV0_CAPTURE_CLIP_YCLIP_EVEN_FIELD                25:16
#define ZV0_CAPTURE_CLIP_YCLIP                          25:16
#define ZV0_CAPTURE_CLIP_XCLIP                          9:0

#define ZV0_CAPTURE_SIZE                                0x090008
#define ZV0_CAPTURE_SIZE_HEIGHT                         26:16
#define ZV0_CAPTURE_SIZE_WIDTH                          10:0   

#define ZV0_CAPTURE_BUF0_ADDRESS                        0x09000C
#define ZV0_CAPTURE_BUF0_ADDRESS_STATUS                 31:31
#define ZV0_CAPTURE_BUF0_ADDRESS_STATUS_CURRENT         0
#define ZV0_CAPTURE_BUF0_ADDRESS_STATUS_PENDING         1
#define ZV0_CAPTURE_BUF0_ADDRESS_ADDRESS                29:0

#define ZV0_CAPTURE_BUF1_ADDRESS                        0x090010
#define ZV0_CAPTURE_BUF1_ADDRESS_STATUS                 31:31
#define ZV0_CAPTURE_BUF1_ADDRESS_STATUS_CURRENT         0
#define ZV0_CAPTURE_BUF1_ADDRESS_STATUS_PENDING         1
#define ZV0_CAPTURE_BUF1_ADDRESS_ADDRESS                29:0

#define ZV0_CAPTURE_BUF_OFFSET                          0x090014
#define ZV0_CAPTURE_BUF_OFFSET_YCLIP_ODD_FIELD          25:16
#define ZV0_CAPTURE_BUF_OFFSET_OFFSET                   15:0

#define ZV0_CAPTURE_FIFO_CTRL                           0x090018
#define ZV0_CAPTURE_FIFO_CTRL_FIFO                      2:0
#define ZV0_CAPTURE_FIFO_CTRL_FIFO_0                    0
#define ZV0_CAPTURE_FIFO_CTRL_FIFO_1                    1
#define ZV0_CAPTURE_FIFO_CTRL_FIFO_2                    2
#define ZV0_CAPTURE_FIFO_CTRL_FIFO_3                    3
#define ZV0_CAPTURE_FIFO_CTRL_FIFO_4                    4
#define ZV0_CAPTURE_FIFO_CTRL_FIFO_5                    5
#define ZV0_CAPTURE_FIFO_CTRL_FIFO_6                    6
#define ZV0_CAPTURE_FIFO_CTRL_FIFO_7                    7

#define ZV0_CAPTURE_YRGB_CONST                          0x09001C
#define ZV0_CAPTURE_YRGB_CONST_Y                        31:24
#define ZV0_CAPTURE_YRGB_CONST_R                        23:16
#define ZV0_CAPTURE_YRGB_CONST_G                        15:8
#define ZV0_CAPTURE_YRGB_CONST_B                        7:0

#define ZV0_CAPTURE_LINE_COMP                           0x090020
#define ZV0_CAPTURE_LINE_COMP_LC                        10:0



#define I2C_BYTE_COUNT                                  0x010040
#define I2C_BYTE_COUNT_COUNT                            3:0

#define I2C_CTRL                                        0x010041
#define I2C_CTRL_REPEAT									6:6
#define I2C_CTRL_REPEAT_DISABLE							0
#define I2C_CTRL_REPEAT_ENABLE							1
#define I2C_CTRL_INTACK									5:5
#define I2C_CTRL_INTACK_DISABLE							0
#define I2C_CTRL_INTACK_ENABLE							1
#define I2C_CTRL_INT                                    4:4
#define I2C_CTRL_INT_DISABLE                            0
#define I2C_CTRL_INT_ENABLE                             1
#define I2C_CTRL_CTRL                                   2:2
#define I2C_CTRL_CTRL_STOP                              0
#define I2C_CTRL_CTRL_START                             1
#define I2C_CTRL_MODE                                   1:1
#define I2C_CTRL_MODE_STANDARD                          0
#define I2C_CTRL_MODE_FAST                              1
#define I2C_CTRL_EN                                     0:0
#define I2C_CTRL_EN_DISABLE                             0
#define I2C_CTRL_EN_ENABLE                              1

#define I2C_STATUS                                      0x010042
#define I2C_STATUS_TX                                   3:3
#define I2C_STATUS_TX_PROGRESS                          0
#define I2C_STATUS_TX_COMPLETED                         1
#define I2C_STATUS_ERR                                  2:2
#define I2C_STATUS_ERR_NORMAL                           0
#define I2C_STATUS_ERR_ERROR                            1
#define I2C_STATUS_ACK                                  1:1
#define I2C_STATUS_ACK_RECEIVED                         1
#define I2C_STATUS_ACK_NOT                              0
#define I2C_STATUS_BUS                                  0:0
#define I2C_STATUS_BUS_IDLE                             0
#define I2C_STATUS_BUS_BUSY                             1

#define I2C_RESET                                       0x010042
#define I2C_RESET_TX									3:3
#define I2C_RESET_TX_CLEAR								0
#define I2C_RESET_ERR                           		2:2
#define I2C_RESET_ERR_CLEAR                       		0
#define I2C_RESET_ACK									1:1
#define I2C_RESET_ACK_CLEAR								0
#define I2C_RESET_BUS									0:0
#define I2C_RESET_BUS_CLEAR								0

#define I2C_SLAVE_ADDRESS                               0x010043
#define I2C_SLAVE_ADDRESS_ADDRESS                       7:1
#define I2C_SLAVE_ADDRESS_RW                            0:0
#define I2C_SLAVE_ADDRESS_RW_W                          0
#define I2C_SLAVE_ADDRESS_RW_R                          1

#define I2C_DATA0                                       0x010044
#define I2C_DATA1                                       0x010045
#define I2C_DATA2                                       0x010046
#define I2C_DATA3                                       0x010047
#define I2C_DATA4                                       0x010048
#define I2C_DATA5                                       0x010049
#define I2C_DATA6                                       0x01004A
#define I2C_DATA7                                       0x01004B
#define I2C_DATA8                                       0x01004C
#define I2C_DATA9                                       0x01004D
#define I2C_DATA10                                      0x01004E
#define I2C_DATA11                                      0x01004F
#define I2C_DATA12                                      0x010050
#define I2C_DATA13                                      0x010051
#define I2C_DATA14                                      0x010052
#define I2C_DATA15                                      0x010053

/* MMIO offset between I2C0 and I2C1 */
#define I2C_OFFSET 0x20


#define TIMER_CONTROL                                   0x010030
#define TIMER_CONTROL_COUNTER                           31:4
#define TIMER_CONTROL_RAWINT_STATUS                     3:3
#define TIMER_CONTROL_RAWINT_STATUS_CLEAR               0
#define TIMER_CONTROL_RAWINT_STATUS_PENDING             1
#define TIMER_CONTROL_RAWINT_STATUS_RESET               1
#define TIMER_CONTROL_RAWINT_ENABLE                     2:2  
#define TIMER_CONTROL_RAWINT_ENABLE_DISABLE             0
#define TIMER_CONTROL_RAWINT_ENABLE_ENABLE              1
#define TIMER_CONTROL_DIV16                             1:1
#define TIMER_CONTROL_DIV16_DISABLE                     0
#define TIMER_CONTROL_DIV16_ENABLE                      1
#define TIMER_CONTROL_ENABLE                            0:0
#define TIMER_CONTROL_ENABLE_DISABLE                    0
#define TIMER_CONTROL_ENABLE_ENABLE                     1

#define TIMER_OFFSET									4


#define UART_RX                                     0x030000
#define UART_RX_DATA								7:0

#define UART_TX                                     0x030000
#define UART_TX_DATA								7:0

#define UART_DLL                                    0x030000
#define UART_DLL_DATA								7:0

#define UART_DLM                                    0x030004
#define UART_DLM_DATA								7:0

#define UART_INT									0x030004
#define UART_INT_RX_LINE		 					2:2
#define UART_INT_RX_LINE_DISABLE 					0
#define UART_INT_RX_LINE_ENABLE	 					1
#define UART_INT_TX_BUF_EMTPTY						1:1
#define UART_INT_TX_BUF_EMPTY_DISABLE				0
#define UART_INT_TX_BUF_EMPTY_ENABLE				1
#define UART_INT_RX_BUF_FULL						0:0
#define UART_INT_RX_BUF_FULL_DISABLE				0
#define UART_INT_RX_BUF_FULL_ENABLE					1

#define UART_STATUS									0x030008
#define UART_STATUS_FIFO							7:6
#define UART_STATUS_FIFO_DISABLE					0
#define UART_STATUS_FIFO_ENABLE						3
#define UART_STATUS_INT_ID							5:1
#define UART_STATUS_INT_ID_RXLINE					3
#define UART_STATUS_INT_ID_RXDATA					2
#define UART_STATUS_INT_ID_RXFIFO					6
#define UART_STATUS_INT_ID_TXEMPTY					1
#define UART_STATUS_INT_ID_XOFF						8
#define UART_STATUS_INT_ID_EDGE						16
#define UART_STATUS_NINT							0:0
#define UART_STATUS_NINT_PENDING					0
#define UART_STATUS_NINT_NOPENDING					1

#define UART_CONTROL								0x030008
#define UART_CONTROL_RX_FIFO						7:6
#define UART_CONTROL_RX_FIFO_1						0
#define UART_CONTROL_RX_FIFO_16						1
#define UART_CONTROL_RX_FIFO_32						2
#define UART_CONTROL_RX_FIFO_62						3
#define UART_CONTROL_TX_FIFO						5:4
#define UART_CONTROL_TX_FIFO_1						0
#define UART_CONTROL_TX_FIFO_16						1
#define UART_CONTROL_TX_FIFO_32						2
#define UART_CONTROL_TX_FIFO_62						3
#define UART_CONTROL_TX_FIFOCLR						2:2
#define UART_CONTROL_TX_FIFOCLR_CLR					1
#define UART_CONTROL_RX_FOFOCLR						1:1
#define UART_CONTROL_RX_FIFOCLR_CLR					1
#define UART_CONTROL_FIFO							0:0
#define UART_CONTROL_FIFO_DISABLE					0
#define UART_CONTROL_FIFO_ENABLE					1

#define UART_LINECONTROL                            0x03000C
#define UART_LINECONTROL_DLAB                       7:7
#define UART_LINECONTROL_DLAB_NORMAL                0
#define UART_LINECONTROL_DLAB_CONFIG                1
#define UART_LINECONTROL_SETBREAK                   6:6
#define UART_LINECONTROL_SETBREAK_DISABLE           0
#define UART_LINECONTROL_SETBREAK_ENABLE            1
#define UART_LINECONTROL_STICKPARITY                5:5
#define UART_LINECONTROL_STICKPARITY_DISABLE        0
#define UART_LINECONTROL_STICKPARITY_ENABLE         1
#define UART_LINECONTROL_PARITYSELECT               4:4
#define UART_LINECONTROL_PARITYSELECT_ODD           0
#define UART_LINECONTROL_PARITYSELECT_EVEN          1
#define UART_LINECONTROL_PARITY                     3:3
#define UART_LINECONTROL_PARITY_DISABLE             0
#define UART_LINECONTROL_PARITY_ENABLE              1
#define UART_LINECONTROL_STOPBIT                    2:2
#define UART_LINECONTROL_STOPBIT_ONE				0
#define UART_LINECONTROL_STOPBIT_TWO                1
#define UART_LINECONTROL_LENGTH                     1:0
#define UART_LINECONTROL_LENGTH_5BIT                0
#define UART_LINECONTROL_LENGTH_6BIT                1
#define UART_LINECONTROL_LENGTH_7BIT                2
#define UART_LINECONTROL_LENGTH_8BIT                3

#define UART_LINESTATUS								0x030014
#define UART_LINESTATUS_FIFO_ERR					7:7
#define UART_LINESTATUS_TX_EMPTY					6:6
#define UART_LINESTATUS_TX_HOLDEMPTY				5:5
#define UART_LINESTATUS_BI							4:4
#define UART_LINESTATUS_FRAME_ERR					3:3
#define UART_LINESTATUS_PARITY_ERR					2:2
#define UART_LINESTATUS_OVERRUN_ERR					1:1
#define UART_LINESTATUS_DATA_READY					0:0

#define UART_OFFSET 0x20

#define UART_SCRATCH                                0x03001C



#define I2S_TX_DATA_L                              0x0A0200
#define I2S_TX_DATA_L_DATA						   15:0

#define I2S_TX_DATA_R                              0x0A0204
#define I2S_TX_DATA_R_DATA                         15:0

#define I2S_RX_DATA_L                              0x0A0208
#define I2S_RX_DATA_L_DATA						   15:0

#define I2S_RX_DATA_R                              0x0A020C
#define I2S_RX_DATA_R_DATA						   15:0

#define I2S_STATUS                                 0x0A0210
#define I2S_STATUS_R                               11:11
#define I2S_STATUS_R_NO_ERR                        0
#define I2S_STATUS_R_OVERFLOW                      1
#define I2S_STATUS_T                               10:10
#define I2S_STATUS_T_NO_ERR                        0
#define I2S_STATUS_T_UNDERFLOW                     1
#define I2S_STATUS_TX                              2:2
#define I2S_STATUS_TX_DISABLE                      0
#define I2S_STATUS_TX_ENABLE                       1

#define I2S_CTRL                                    0x0A0214
#define I2S_CTRL_MODE                               7:7
#define I2S_CTRL_MODE_SLAVE                         0
#define I2S_CTRL_MODE_MASTER                        1
#define I2S_CTRL_CS                                 6:5
#define I2S_CTRL_CS_16                              0
#define I2S_CTRL_CS_24                              1
#define I2S_CTRL_CS_32                              2
#define I2S_CTRL_CDIV                               4:0

#define I2S_SRAM_DMA                                0x0A0218
#define I2S_SRAM_DMA_STATE                          31:31    
#define I2S_SRAM_DMA_STATE_DISABLE                  0            
#define I2S_SRAM_DMA_STATE_ENABLE                   1
#define I2S_SRAM_DMA_SIZE                           23:16     
#define I2S_SRAM_DMA_ADDRESS						8:0                               
           
#define I2S_SRAM_DMA_STATUS                         0x0A021C
#define I2S_SRAM_DMA_STATUS_TC                      0:0    
#define I2S_SRAM_DMA_STATUS_TC_COMPLETE             1            
#define I2S_SRAM_DMA_STATUS_TC_CLEAR                0
           
#define SRAM_OUTPUT_BASE							0x8000
#define SRAM_INPUT_BASE								0x8800
#define SRAM_SIZE									0x0800


#define SM768_PHY_MAX_LANE 4

#define DDRPHYREG(A) (A * 4) + 0x4000
#define RIDR    DDRPHYREG(0x00)
#define PIR     DDRPHYREG(0x01)
#define ACIOCR  DDRPHYREG(0x09)
#define BISTMSKR0 DDRPHYREG(0x41)
#define BISTMSKR1 DDRPHYREG(0x42)
#define BISTLSR DDRPHYREG(0x44)
#define BISTUDPR DDRPHYREG(0x48)
#define BISTBER0 DDRPHYREG(0x4B)
#define BISTBER1 DDRPHYREG(0x4C)
#define BISTBER2 DDRPHYREG(0x4D)
#define BISTWCSR DDRPHYREG(0x4E)
#define BISTFWR0 DDRPHYREG(0x4F)
#define BISTFWR1 DDRPHYREG(0x50)
#define DX0DLLCR DDRPHYREG(0x73)
#define DX0DQSTR DDRPHYREG(0x75)
#define DX1DLLCR DDRPHYREG(0x83)
#define DX1DQSTR DDRPHYREG(0x85)
#define DX2DLLCR DDRPHYREG(0x93)
#define DX2DQSTR DDRPHYREG(0x95)
#define DX3DLLCR DDRPHYREG(0xA3)
#define DX3DQSTR DDRPHYREG(0xA5)

#define PGCR										DDRPHYREG(0x02)
#define PGCR_LBMODE									31:31
#define PGCR_LBGDQS									30:30
#define PGCR_LBDQSS									29:29
#define PGCR_RFSHDT									28:25
#define PGCR_PDDISDX								24:24
#define PGCR_ZCKSEL									23:22
#define PGCR_RANKEN									21:18
#define PGCR_IODDRM									17:16
#define PGCR_IOLB									15:15
#define PGCR_IOLB_AFTERPAD							0
#define PGCR_IOLB_BEFOREPAD							1
#define PGCR_CKINV									14:14
#define PGCR_CKDV									13:12
#define PGCR_CKEN									11:9
#define PGCR_DTOSEL									8:5
#define PGCR_DFTLMT									4:3
#define PGCR_DFTCMP									2:2
#define PGCR_DQSCFG									1:1
#define PGCR_ITMDMD									0:0
#define PGCR_ITMDMD_DQSDQS							0
#define PGCR_ITMDMD_DQS								1

#define PGSR										DDRPHYREG(0x03)
#define PGSR_TQ										31:31
#define PGSR_RESERVED								30:10
#define PGSR_RVEIRR									9:9
#define PGSR_RVERR									8:8
#define PGSR_DFTERR									7:7
#define PGSR_DTIERR									6:6
#define PGSR_DTERR									5:5
#define PGSR_DTDONE									4:4
#define PGSR_DIDONE									3:3
#define PGSR_ZCDONE									2:2
#define PGSR_DLDONE									1:1
#define PGSR_IDONE									0:0
#define PGSR_IDONE_NO								0
#define PGSR_IDONE_YES								1

#define BISTRR  									DDRPHYREG(0x40) //BIST Run Register
#define	BISTRR_RESERVED								31:26
#define BISTRR_BCKSEL								25:23
#define BISTRR_BDXSEL								22:19
#define BISTRR_BDPAT								18:17
#define BISTRR_BDPAT_WALKING0						0
#define BISTRR_BDPAT_WALKING1						1
#define BISTRR_BDPAT_RANDOM							2
#define BISTRR_BDPAT_PROGRAM						3
#define BISTRR_BDMEN								16:16
#define BISTRR_BDMEM_DISABLE						0
#define BISTRR_BDMEN_ENABLE							1
#define BISTRR_BACEN								15:15
#define BISTRR_BACEN_DISABLE						0
#define BISTRR_BACEN_ENABLE							1
#define BISTRR_BDXEN								14:14
#define BISTRR_BDXEN_DISABLE						0
#define BISTRR_BDXEN_ENABLE							1
#define BISTRR_BSONF								13:13
#define BISTRR_NFAIL								12:5
#define BISTRR_BINF									4:4
#define BISTRR_BINF_STOPONDONE						0
#define BISTRR_BINF_NONSTOP							1
#define BISTRR_BMODE								3:3
#define BISTRR_BMODE_LOOPBACK						0
#define BISTRR_BMODE_DRAM							1
#define BISTRR_BINST 								2:0
#define BISTRR_BINST_NOP							0
#define BISTRR_BINST_RUN							1
#define BISTRR_BINST_STOP							2
#define BISTRR_BINST_RESET							3

#define BISTWCR										DDRPHYREG(0x43)
#define BISTWCR_RESERVED							31:16
#define BISTWCR_BWCNT								15:0

#define BISTAR0 									DDRPHYREG(0x45)
#define BISTAR0_RESERVED							31:31
#define BISTAR0_BBANK								30:28
#define BISTAR0_BROW								27:12
#define BISTAR0_BCOL								11:0

#define BISTAR1 									DDRPHYREG(0x46)
#define BISTAR1_RESERVED							31:16
#define BISTAR1_BAINC								15:4
#define BISTAR1_BMRANK								3:2
#define BISTAR1_BRANK								1:0

#define BISTAR2										DDRPHYREG(0x47)
#define BISTAR2_RESERVED							31:31
#define BISTAR2_BMBANK								30:28
#define BISTAR2_BMROW								27:12
#define BISTAR2_BMCOL								11:0

#define BISTGSR 									DDRPHYREG(0x49)
#define BISTGSR_CASBER								31:30
#define BISTGSR_RASBER								29:28
#define BISTGSR_DMBER								27:24
#define BISTGSR_TDPBER								23:22
#define BISTGSR_PARBER								21:20
#define BISTGSR_RESERVED							19:3
#define BISTGSR_BDXERR								2:2
#define BISTGSR_BDXERR_NO							0
#define BISTGSR_BDXERR_YES							1
#define BISTGSR_BACERR								1:1
#define BISTGSR_BACERR_NO							0
#define BISTGSR_BACERR_YES							1
#define BISTGSR_BDONE								0:0
#define BISTGSR_BDONE_NO							0
#define BISTGSR_BDONE_YES							1

#define BISTWER										DDRPHYREG(0x4A)
#define BISTWER_DXWER								31:16
#define BISTWER_ACWER								15:0






//PHY Interface Interrupt Mute Control Register
#define IH_MUTE_PHY_STAT0  0x00000184
#define IH_MUTE_PHY_STAT0_HPD_MASK  0x00000001 //When set to 1, mutes ih_phy_stat0[0]
#define IH_MUTE_PHY_STAT0_TX_PHY_LOCK_MASK  0x00000002 //When set to 1, mutes ih_phy_stat0[1]
#define IH_MUTE_PHY_STAT0_RX_SENSE_0_MASK  0x00000004 //When set to 1, mutes ih_phy_stat0[2]
#define IH_MUTE_PHY_STAT0_RX_SENSE_1_MASK  0x00000008 //When set to 1, mutes ih_phy_stat0[3]
#define IH_MUTE_PHY_STAT0_RX_SENSE_2_MASK  0x00000010 //When set to 1, mutes ih_phy_stat0[4]
#define IH_MUTE_PHY_STAT0_RX_SENSE_3_MASK  0x00000020 //When set to 1, mutes ih_phy_stat0[5]

//Frame Composer video/audio Force Enable Register This register allows to force the controller to output audio and video data the values configured in the FC_DBGAUD and FC_DBGTMDS registers
#define FC_DBGFORCE  0x00001200
#define FC_DBGFORCE_FORCEVIDEO_MASK  0x00000001 //Force fixed video output with FC_DBGTMDSx register contents
#define FC_DBGFORCE_FORCEAUDIO_MASK  0x00000010 //Force fixed audio output with FC_DBGAUDxCHx register contents

#define FC_DBGTMDS0  0x1219
#define FC_DBGTMDS1  0x121A
#define FC_DBGTMDS2  0x121B

//Frame Composer Scrambler Control
#define FC_SCRAMBLER_CTRL  0x000010E1
#define FC_SCRAMBLER_CTRL_SCRAMBLER_ON_MASK  0x00000001 //When set (1'b1), this field activates the HDMI 2
#define FC_SCRAMBLER_CTRL_SCRAMBLER_UCP_LINE_MASK  0x00000010 //Debug register

//Main Controller Software Reset Register Main controller software reset request per clock domain
#define MC_SWRSTZREQ  0x0004002
#define MC_SWRSTZREQ_PIXELSWRST_REQ_MASK  0x00000001 //Pixel software reset request
#define MC_SWRSTZREQ_TMDSSWRST_REQ_MASK  0x00000002 //TMDS software reset request
#define MC_SWRSTZREQ_PREPSWRST_REQ_MASK  0x00000004 //Pixel Repetition software reset request
#define MC_SWRSTZREQ_II2SSWRST_REQ_MASK  0x00000008 //I2S audio software reset request
#define MC_SWRSTZREQ_ISPDIFSWRST_REQ_MASK  0x00000010 //SPDIF audio software reset request
#define MC_SWRSTZREQ_CECSWRST_REQ_MASK  0x00000040 //CEC software reset request
#define MC_SWRSTZREQ_IGPASWRST_REQ_MASK  0x00000080 //GPAUD interface soft reset request

/* SCDC Registers */
#define SCDC_SLAVE_ADDRESS 	0x54

#define SCDC_SINK_VER  		    0x01	/* sink version                                   */
#define SCDC_SOURCE_VER  	    0x02	/* source version                                 */
#define SCDC_UPDATE_0  		    0x10	/* Update_0                                       */
#define SCDC_UPDATE_0_STATUS  	0x1	    /* Status update flag                             */
#define SCDC_UPDATE_0_CED  		0x2	    /* Character error update flag                   */
#define SCDC_UPDATE_0_RR_TEST  	0x4	    /* Read request test                              */

#define SCDC_UPDATE_1 		    0x11	/* Update_1                                       */
#define SCDC_UPDATE_RESERVED  	0x12	/* 0x12-0x1f - Reserved for Update Related Uses   */
#define SCDC_TMDS_CONFIG  	    0x20	/* TMDS_Config                                    */
#define SCDC_SCRAMBLER_STAT     0x21	/* Scrambler_Status                               */
#define SCDC_CONFIG_0  		    0x30	/* Config_0                                       */
#define SCDC_CONFIG_RESERVED  	0x31	/* 0x31-0x3f - Reserved for configuration         */
#define SCDC_STATUS_FLAG_0  	0x40	/* Status_Flag_0                                  */
#define SCDC_STATUS_FLAG_1  	0x41	/* Status_Flag_1                                  */
#define SCDC_STATUS_RESERVED 	0x42	/* 0x42-0x4f - Reserved for Status Related Uses   */
#define SCDC_ERR_DET_0_L  	    0x50	/* Err_Det_0_L                                    */
#define SCDC_ERR_DET_0_H  	    0x51	/* Err_Det_0_H                                    */
#define SCDC_ERR_DET_1_L  	    0x52	/* Err_Det_1_L                                    */
#define SCDC_ERR_DET_1_H  	    0x53	/* Err_Det_1_H                                    */
#define SCDC_ERR_DET_2_L  	    0x54	/* Err_Det_2_L                                    */
#define SCDC_ERR_DET_2_H  	    0x55	/* Err_Det_2_H                                    */
#define SCDC_ERR_DET_CHKSUM  	0x56	/* Err_Det_Checksum                               */
#define SCDC_TEST_CFG_0  	    0xc0	/* Test_config_0                                  */
#define SCDC_TEST_RESERVED  	0xc1	/* 0xc1-0xcf - Reserved for test features         */
#define SCDC_MAN_OUI_3RD  	    0xd0	/* Manufacturer IEEE OUI, Third Octet             */
#define SCDC_MAN_OUI_2ND  	    0xd1	/* Manufacturer IEEE OUI, Second Octet            */
#define SCDC_MAN_OUI_1ST  	    0xd2	/* Manufacturer IEEE OUI, First Octet             */
#define SCDC_DEVICE_ID  	    0xd3	/* 0xd3-0xdd - Device ID                          */
#define SCDC_MAN_SPECIFIC  	    0xde	/* 0xde-0xff - ManufacturerSpecific               */

/*****************************************************************************
 *                                                                           *
 *                         Main Controller Registers                         *
 *                                                                           *
 *****************************************************************************/

//Main Controller Synchronous Clock Domain Disable Register
#define MC_CLKDIS  0x0004001
#define MC_CLKDIS_PIXELCLK_DISABLE_MASK  0x00000001 //Pixel clock synchronous disable signal
#define MC_CLKDIS_TMDSCLK_DISABLE_MASK  0x00000002 //TMDS clock synchronous disable signal
#define MC_CLKDIS_PREPCLK_DISABLE_MASK  0x00000004 //Pixel Repetition clock synchronous disable signal
#define MC_CLKDIS_AUDCLK_DISABLE_MASK  0x00000008 //Audio Sampler clock synchronous disable signal
#define MC_CLKDIS_CSCCLK_DISABLE_MASK  0x00000010 //Color Space Converter clock synchronous disable signal
#define MC_CLKDIS_CECCLK_DISABLE_MASK  0x00000020 //CEC Engine clock synchronous disable signal
#define MC_CLKDIS_HDCPCLK_DISABLE_MASK  0x00000040 //HDCP clock synchronous disable signal

//Main Controller Software Reset Register Main controller software reset request per clock domain
#define MC_SWRSTZREQ  0x0004002
#define MC_SWRSTZREQ_PIXELSWRST_REQ_MASK  0x00000001 //Pixel software reset request
#define MC_SWRSTZREQ_TMDSSWRST_REQ_MASK  0x00000002 //TMDS software reset request
#define MC_SWRSTZREQ_PREPSWRST_REQ_MASK  0x00000004 //Pixel Repetition software reset request
#define MC_SWRSTZREQ_II2SSWRST_REQ_MASK  0x00000008 //I2S audio software reset request
#define MC_SWRSTZREQ_ISPDIFSWRST_REQ_MASK  0x00000010 //SPDIF audio software reset request
#define MC_SWRSTZREQ_CECSWRST_REQ_MASK  0x00000040 //CEC software reset request
#define MC_SWRSTZREQ_IGPASWRST_REQ_MASK  0x00000080 //GPAUD interface soft reset request

//Main Controller HDCP Bypass Control Register
#define MC_OPCTRL  0x0004003
#define MC_OPCTRL_HDCP_BLOCK_BYP_MASK  0x00000001 //Block HDCP bypass mechanism - 1'b0: This is the default value

//Main Controller Feed Through Control Register
#define MC_FLOWCTRL  0x0004004
#define MC_FLOWCTRL_FEED_THROUGH_OFF_MASK  0x00000001 //Video path Feed Through enable bit: - 1b: Color Space Converter is in the video data path

//Main Controller PHY Reset Register
#define MC_PHYRSTZ  0x0004005
#define MC_PHYRSTZ_PHYRSTZ_MASK  0x00000001 //HDMI Source PHY active low reset control for PHY GEN1, active high reset control for PHY GEN2

//Main Controller Clock Present Register
#define MC_LOCKONCLOCK  0x0004006
#define MC_LOCKONCLOCK_CECCLK_MASK  0x00000001 //CEC clock status
#define MC_LOCKONCLOCK_AUDIOSPDIFCLK_MASK  0x00000004 //SPDIF clock status
#define MC_LOCKONCLOCK_I2SCLK_MASK  0x00000008 //I2S clock status
#define MC_LOCKONCLOCK_PREPCLK_MASK  0x00000010 //Pixel Repetition clock status
#define MC_LOCKONCLOCK_TCLK_MASK  0x00000020 //TMDS clock status
#define MC_LOCKONCLOCK_PCLK_MASK  0x00000040 //Pixel clock status
#define MC_LOCKONCLOCK_IGPACLK_MASK  0x00000080 //GPAUD interface clock status

/*****************************************************************************
 *                                                                           *
 *                            Interrupt Registers                            *
 *                                                                           *
 *****************************************************************************/

//Frame Composer Interrupt Status Register 0 (Packet Interrupts)
#define IH_FC_STAT0  0x00000100
#define IH_FC_STAT0_NULL_MASK  0x00000001 //Active after successful transmission of an Null packet
#define IH_FC_STAT0_ACR_MASK  0x00000002 //Active after successful transmission of an Audio Clock Regeneration (N/CTS transmission) packet
#define IH_FC_STAT0_AUDS_MASK  0x00000004 //Active after successful transmission of an Audio Sample packet
#define IH_FC_STAT0_NVBI_MASK  0x00000008 //Active after successful transmission of an NTSC VBI packet
#define IH_FC_STAT0_MAS_MASK  0x00000010 //Active after successful transmission of an MultiStream Audio packet
#define IH_FC_STAT0_HBR_MASK  0x00000020 //Active after successful transmission of an Audio HBR packet
#define IH_FC_STAT0_ACP_MASK  0x00000040 //Active after successful transmission of an Audio Content Protection packet
#define IH_FC_STAT0_AUDI_MASK  0x00000080 //Active after successful transmission of an Audio InfoFrame packet

//Frame Composer Interrupt Status Register 1 (Packet Interrupts)
#define IH_FC_STAT1  0x00000101
#define IH_FC_STAT1_GCP_MASK  0x00000001 //Active after successful transmission of an General Control Packet
#define IH_FC_STAT1_AVI_MASK  0x00000002 //Active after successful transmission of an AVI InfoFrame packet
#define IH_FC_STAT1_AMP_MASK  0x00000004 //Active after successful transmission of an Audio Metadata packet
#define IH_FC_STAT1_SPD_MASK  0x00000008 //Active after successful transmission of an Source Product Descriptor InfoFrame packet
#define IH_FC_STAT1_VSD_MASK  0x00000010 //Active after successful transmission of an Vendor Specific Data InfoFrame packet
#define IH_FC_STAT1_ISCR2_MASK  0x00000020 //Active after successful transmission of an International Standard Recording Code 2 packet
#define IH_FC_STAT1_ISCR1_MASK  0x00000040 //Active after successful transmission of an International Standard Recording Code 1 packet
#define IH_FC_STAT1_GMD_MASK  0x00000080 //Active after successful transmission of an Gamut metadata packet

//Frame Composer Interrupt Status Register 2 (Packet Queue Overflow Interrupts)
#define IH_FC_STAT2  0x00000102
#define IH_FC_STAT2_HIGHPRIORITY_OVERFLOW_MASK  0x00000001 //Frame Composer high priority packet queue descriptor overflow indication
#define IH_FC_STAT2_LOWPRIORITY_OVERFLOW_MASK  0x00000002 //Frame Composer low priority packet queue descriptor overflow indication

//Audio Sampler Interrupt Status Register (FIFO Threshold, Underflow and Overflow Interrupts)
#define IH_AS_STAT0  0x00000103
#define IH_AS_STAT0_AUD_FIFO_OVERFLOW_MASK  0x00000001 //Audio Sampler audio FIFO full indication
#define IH_AS_STAT0_AUD_FIFO_UNDERFLOW_MASK  0x00000002 //Audio Sampler audio FIFO empty indication
#define IH_AS_STAT0_AUD_FIFO_UNDERFLOW_THR_MASK  0x00000004 //Audio Sampler audio FIFO empty threshold (four samples) indication for the legacy HBR audio interface
#define IH_AS_STAT0_FIFO_OVERRUN_MASK  0x00000008 //Indicates an overrun on the audio FIFO

//PHY Interface Interrupt Status Register (RXSENSE, PLL Lock and HPD Interrupts)
#define IH_PHY_STAT0  0x00000104
#define IH_PHY_STAT0_HPD_MASK  0x00000001 //HDMI Hot Plug Detect indication
#define IH_PHY_STAT0_TX_PHY_LOCK_MASK  0x00000002 //TX PHY PLL lock indication
#define IH_PHY_STAT0_RX_SENSE_0_MASK  0x00000004 //TX PHY RX_SENSE indication for driver 0
#define IH_PHY_STAT0_RX_SENSE_1_MASK  0x00000008 //TX PHY RX_SENSE indication for driver 1
#define IH_PHY_STAT0_RX_SENSE_2_MASK  0x00000010 //TX PHY RX_SENSE indication for driver 2
#define IH_PHY_STAT0_RX_SENSE_3_MASK  0x00000020 //TX PHY RX_SENSE indication for driver 3

//E-DDC I2C Master Interrupt Status Register (Done and Error Interrupts)
#define IH_I2CM_STAT0  0x00000105
#define IH_I2CM_STAT0_I2CMASTERERROR_MASK  0x00000001 //I2C Master error indication
#define IH_I2CM_STAT0_I2CMASTERDONE_MASK  0x00000002 //I2C Master done indication
#define IH_I2CM_STAT0_SCDC_READREQ_MASK  0x00000004 //I2C Master SCDC read request indication

//CEC Interrupt Status Register (Functional Operation Interrupts)
#define IH_CEC_STAT0  0x00000106
#define IH_CEC_STAT0_DONE_MASK  0x00000001 //CEC Done Indication
#define IH_CEC_STAT0_EOM_MASK  0x00000002 //CEC End of Message Indication
#define IH_CEC_STAT0_NACK_MASK  0x00000004 //CEC Not Acknowledge indication
#define IH_CEC_STAT0_ARB_LOST_MASK  0x00000008 //CEC Arbitration Lost indication
#define IH_CEC_STAT0_ERROR_INITIATOR_MASK  0x00000010 //CEC Error Initiator indication
#define IH_CEC_STAT0_ERROR_FOLLOW_MASK  0x00000020 //CEC Error Follow indication
#define IH_CEC_STAT0_WAKEUP_MASK  0x00000040 //CEC Wake-up indication

//Video Packetizer Interrupt Status Register (FIFO Full and Empty Interrupts)
#define IH_VP_STAT0  0x00000107
#define IH_VP_STAT0_FIFOEMPTYBYP_MASK  0x00000001 //Video Packetizer 8 bit bypass FIFO empty interrupt
#define IH_VP_STAT0_FIFOFULLBYP_MASK  0x00000002 //Video Packetizer 8 bit bypass FIFO full interrupt
#define IH_VP_STAT0_FIFOEMPTYREMAP_MASK  0x00000004 //Video Packetizer pixel YCC 422 re-mapper FIFO empty interrupt
#define IH_VP_STAT0_FIFOFULLREMAP_MASK  0x00000008 //Video Packetizer pixel YCC 422 re-mapper FIFO full interrupt
#define IH_VP_STAT0_FIFOEMPTYPP_MASK  0x00000010 //Video Packetizer pixel packing FIFO empty interrupt
#define IH_VP_STAT0_FIFOFULLPP_MASK  0x00000020 //Video Packetizer pixel packing FIFO full interrupt
#define IH_VP_STAT0_FIFOEMPTYREPET_MASK  0x00000040 //Video Packetizer pixel repeater FIFO empty interrupt
#define IH_VP_STAT0_FIFOFULLREPET_MASK  0x00000080 //Video Packetizer pixel repeater FIFO full interrupt

//Interruption Handler Decode Assist Register
#define IH_DECODE  0x00000170
#define IH_DECODE_IH_AHBDMAAUD_STAT0_MASK  0x00000001 //Interruption active at the ih_ahbdmaaud_stat0 register
#define IH_DECODE_IH_CEC_STAT0_MASK  0x00000002 //Interruption active at the ih_cec_stat0 register
#define IH_DECODE_IH_I2CM_STAT0_MASK  0x00000004 //Interruption active at the ih_i2cm_stat0 register
#define IH_DECODE_IH_PHY_MASK  0x00000008 //Interruption active at the ih_phy_stat0 or ih_i2cmphy_stat0 register
#define IH_DECODE_IH_AS_STAT0_MASK  0x00000010 //Interruption active at the ih_as_stat0 register
#define IH_DECODE_IH_FC_STAT2_VP_MASK  0x00000020 //Interruption active at the ih_fc_stat2 or ih_vp_stat0 register
#define IH_DECODE_IH_FC_STAT1_MASK  0x00000040 //Interruption active at the ih_fc_stat1 register
#define IH_DECODE_IH_FC_STAT0_MASK  0x00000080 //Interruption active at the ih_fc_stat0 register

//Frame Composer Interrupt Mute Control Register 0
#define IH_MUTE_FC_STAT0  0x00000180
#define IH_MUTE_FC_STAT0_NULL_MASK  0x00000001 //When set to 1, mutes ih_fc_stat0[0]
#define IH_MUTE_FC_STAT0_ACR_MASK  0x00000002 //When set to 1, mutes ih_fc_stat0[1]
#define IH_MUTE_FC_STAT0_AUDS_MASK  0x00000004 //When set to 1, mutes ih_fc_stat0[2]
#define IH_MUTE_FC_STAT0_NVBI_MASK  0x00000008 //When set to 1, mutes ih_fc_stat0[3]
#define IH_MUTE_FC_STAT0_MAS_MASK  0x00000010 //When set to 1, mutes ih_fc_stat0[4]
#define IH_MUTE_FC_STAT0_HBR_MASK  0x00000020 //When set to 1, mutes ih_fc_stat0[5]
#define IH_MUTE_FC_STAT0_ACP_MASK  0x00000040 //When set to 1, mutes ih_fc_stat0[6]
#define IH_MUTE_FC_STAT0_AUDI_MASK  0x00000080 //When set to 1, mutes ih_fc_stat0[7]

//Frame Composer Interrupt Mute Control Register 1
#define IH_MUTE_FC_STAT1  0x00000181
#define IH_MUTE_FC_STAT1_GCP_MASK  0x00000001 //When set to 1, mutes ih_fc_stat1[0]
#define IH_MUTE_FC_STAT1_AVI_MASK  0x00000002 //When set to 1, mutes ih_fc_stat1[1]
#define IH_MUTE_FC_STAT1_AMP_MASK  0x00000004 //When set to 1, mutes ih_fc_stat1[2]
#define IH_MUTE_FC_STAT1_SPD_MASK  0x00000008 //When set to 1, mutes ih_fc_stat1[3]
#define IH_MUTE_FC_STAT1_VSD_MASK  0x00000010 //When set to 1, mutes ih_fc_stat1[4]
#define IH_MUTE_FC_STAT1_ISCR2_MASK  0x00000020 //When set to 1, mutes ih_fc_stat1[5]
#define IH_MUTE_FC_STAT1_ISCR1_MASK  0x00000040 //When set to 1, mutes ih_fc_stat1[6]
#define IH_MUTE_FC_STAT1_GMD_MASK  0x00000080 //When set to 1, mutes ih_fc_stat1[7]

//Frame Composer Interrupt Mute Control Register 2
#define IH_MUTE_FC_STAT2  0x00000182
#define IH_MUTE_FC_STAT2_HIGHPRIORITY_OVERFLOW_MASK  0x00000001 //When set to 1, mutes ih_fc_stat2[0]
#define IH_MUTE_FC_STAT2_LOWPRIORITY_OVERFLOW_MASK  0x00000002 //When set to 1, mutes ih_fc_stat2[1]

//Audio Sampler Interrupt Mute Control Register
#define IH_MUTE_AS_STAT0  0x00000183
#define IH_MUTE_AS_STAT0_AUD_FIFO_OVERFLOW_MASK  0x00000001 //When set to 1, mutes ih_as_stat0[0]
#define IH_MUTE_AS_STAT0_AUD_FIFO_UNDERFLOW_MASK  0x00000002 //When set to 1, mutes ih_as_stat0[1]
#define IH_MUTE_AS_STAT0_AUD_FIFO_UNDERFLOW_THR_MASK  0x00000004 //When set to 1, mutes ih_as_stat0[2]
#define IH_MUTE_AS_STAT0_FIFO_OVERRUN_MASK  0x00000008 //When set to 1, mutes ih_as_stat0[3]

//PHY Interface Interrupt Mute Control Register
#define IH_MUTE_PHY_STAT0  0x00000184
#define IH_MUTE_PHY_STAT0_HPD_MASK  0x00000001 //When set to 1, mutes ih_phy_stat0[0]
#define IH_MUTE_PHY_STAT0_TX_PHY_LOCK_MASK  0x00000002 //When set to 1, mutes ih_phy_stat0[1]
#define IH_MUTE_PHY_STAT0_RX_SENSE_0_MASK  0x00000004 //When set to 1, mutes ih_phy_stat0[2]
#define IH_MUTE_PHY_STAT0_RX_SENSE_1_MASK  0x00000008 //When set to 1, mutes ih_phy_stat0[3]
#define IH_MUTE_PHY_STAT0_RX_SENSE_2_MASK  0x00000010 //When set to 1, mutes ih_phy_stat0[4]
#define IH_MUTE_PHY_STAT0_RX_SENSE_3_MASK  0x00000020 //When set to 1, mutes ih_phy_stat0[5]

//E-DDC I2C Master Interrupt Mute Control Register
#define IH_MUTE_I2CM_STAT0  0x00000185
#define IH_MUTE_I2CM_STAT0_I2CMASTERERROR_MASK  0x00000001 //When set to 1, mutes ih_i2cm_stat0[0]
#define IH_MUTE_I2CM_STAT0_I2CMASTERDONE_MASK  0x00000002 //When set to 1, mutes ih_i2cm_stat0[1]
#define IH_MUTE_I2CM_STAT0_SCDC_READREQ_MASK  0x00000004 //When set to 1, mutes ih_i2cm_stat0[2]

//CEC Interrupt Mute Control Register
#define IH_MUTE_CEC_STAT0  0x00000186
#define IH_MUTE_CEC_STAT0_DONE_MASK  0x00000001 //When set to 1, mutes ih_cec_stat0[0]
#define IH_MUTE_CEC_STAT0_EOM_MASK  0x00000002 //When set to 1, mutes ih_cec_stat0[1]
#define IH_MUTE_CEC_STAT0_NACK_MASK  0x00000004 //When set to 1, mutes ih_cec_stat0[2]
#define IH_MUTE_CEC_STAT0_ARB_LOST_MASK  0x00000008 //When set to 1, mutes ih_cec_stat0[3]
#define IH_MUTE_CEC_STAT0_ERROR_INITIATOR_MASK  0x00000010 //When set to 1, mutes ih_cec_stat0[4]
#define IH_MUTE_CEC_STAT0_ERROR_FOLLOW_MASK  0x00000020 //When set to 1, mutes ih_cec_stat0[5]
#define IH_MUTE_CEC_STAT0_WAKEUP_MASK  0x00000040 //When set to 1, mutes ih_cec_stat0[6]

//Video Packetizer Interrupt Mute Control Register
#define IH_MUTE_VP_STAT0  0x00000187
#define IH_MUTE_VP_STAT0_FIFOEMPTYBYP_MASK  0x00000001 //When set to 1, mutes ih_vp_stat0[0]
#define IH_MUTE_VP_STAT0_FIFOFULLBYP_MASK  0x00000002 //When set to 1, mutes ih_vp_stat0[1]
#define IH_MUTE_VP_STAT0_FIFOEMPTYREMAP_MASK  0x00000004 //When set to 1, mutes ih_vp_stat0[2]
#define IH_MUTE_VP_STAT0_FIFOFULLREMAP_MASK  0x00000008 //When set to 1, mutes ih_vp_stat0[3]
#define IH_MUTE_VP_STAT0_FIFOEMPTYPP_MASK  0x00000010 //When set to 1, mutes ih_vp_stat0[4]
#define IH_MUTE_VP_STAT0_FIFOFULLPP_MASK  0x00000020 //When set to 1, mutes ih_vp_stat0[5]
#define IH_MUTE_VP_STAT0_FIFOEMPTYREPET_MASK  0x00000040 //When set to 1, mutes ih_vp_stat0[6]
#define IH_MUTE_VP_STAT0_FIFOFULLREPET_MASK  0x00000080 //When set to 1, mutes ih_vp_stat0[7]

//PHY GEN2 I2C Master Interrupt Mute Control Register
#define IH_MUTE_I2CMPHY_STAT0  0x00000188
#define IH_MUTE_I2CMPHY_STAT0_I2CMPHYERROR_MASK  0x00000001 //When set to 1, mutes ih_i2cmphy_stat0[0]
#define IH_MUTE_I2CMPHY_STAT0_I2CMPHYDONE_MASK  0x00000002 //When set to 1, mutes ih_i2cmphy_stat0[1]

//AHB Audio DMA Interrupt Mute Control Register
#define IH_MUTE_AHBDMAAUD_STAT0  0x00000189
#define IH_MUTE_AHBDMAAUD_STAT0_INTBUFFEMPTY_MASK  0x00000001 //When set to 1, mutes ih_ahbdmaaud_stat0[0]
#define IH_MUTE_AHBDMAAUD_STAT0_INTBUFFULL_MASK  0x00000002 //en set to 1, mutes ih_ahbdmaaud_stat0[1]
#define IH_MUTE_AHBDMAAUD_STAT0_INTDONE_MASK  0x00000004 //When set to 1, mutes ih_ahbdmaaud_stat0[2]
#define IH_MUTE_AHBDMAAUD_STAT0_INTINTERTRYSPLIT_MASK  0x00000008 //When set to 1, mutes ih_ahbdmaaud_stat0[3]
#define IH_MUTE_AHBDMAAUD_STAT0_INTLOSTOWNERSHIP_MASK  0x00000010 //When set to 1, mutes ih_ahbdmaaud_stat0[4]
#define IH_MUTE_AHBDMAAUD_STAT0_INTERROR_MASK  0x00000020 //When set to 1, mutes ih_ahbdmaaud_stat0[5]
#define IH_MUTE_AHBDMAAUD_STAT0_INTBUFFOVERRUN_MASK  0x00000040 //When set to 1, mutes ih_ahbdmaaud_stat0[6]

/*****************************************************************************
 *                                                                           *
 *                        PHY Configuration Registers                        *
 *                                                                           *
 *****************************************************************************/
	//PHY Configuration Register This register holds the power down, data enable polarity, and interface control of the HDMI Source PHY control
#define PHY_CONF0  0x00003000
#define PHY_CONF0_SELDIPIF_MASK  0x00000001 //Select interface control
#define PHY_CONF0_SELDATAENPOL_MASK  0x00000002 //Select data enable polarity
#define PHY_CONF0_ENHPDRXSENSE_MASK  0x00000004 //PHY ENHPDRXSENSE signal
#define PHY_CONF0_TXPWRON_MASK  0x00000008 //PHY TXPWRON signal
#define PHY_CONF0_PDDQ_MASK  0x00000010 //PHY PDDQ signal
#define PHY_CONF0_SVSRET_MASK  0x00000020 //Reserved as "spare" register with no associated functionality
#define PHY_CONF0_SPARES_1_MASK  0x00000040 //Reserved as "spare" register with no associated functionality
#define PHY_CONF0_SPARES_2_MASK  0x00000080 //Reserved as "spare" register with no associated functionality

//PHY RXSENSE, PLL Lock, and HPD Status Register This register contains the following active high packet sent status indications
#define PHY_STAT0  0x3004
#define PHY_STAT0_TX_PHY_LOCK_MASK  0x00000001 //Status bit
#define PHY_STAT0_HPD_MASK  0x00000002 //Status bit
#define PHY_STAT0_RX_SENSE_0_MASK  0x00000010 //Status bit
#define PHY_STAT0_RX_SENSE_1_MASK  0x00000020 //Status bit
#define PHY_STAT0_RX_SENSE_2_MASK  0x00000040 //Status bit
#define PHY_STAT0_RX_SENSE_3_MASK  0x00000080 //Status bit

//PHY RXSENSE, PLL Lock, and HPD Interrupt Register This register contains the interrupt indication of the PHY_STAT0 status interrupts
#define PHY_INT0  0x3005
#define PHY_INT0_TX_PHY_LOCK_MASK  0x00000001 //Interrupt indication bit
#define PHY_INT0_HPD_MASK  0x00000002 //Interrupt indication bit
#define PHY_INT0_RX_SENSE_0_MASK  0x00000010 //Interrupt indication bit
#define PHY_INT0_RX_SENSE_1_MASK  0x00000020 //Interrupt indication bit
#define PHY_INT0_RX_SENSE_2_MASK  0x00000040 //Interrupt indication bit
#define PHY_INT0_RX_SENSE_3_MASK  0x00000080 //Interrupt indication bit

//PHY RXSENSE, PLL Lock, and HPD Mask Register Mask register for generation of PHY_INT0 interrupts
#define PHY_MASK0  0x3006
#define PHY_MASK0_TX_PHY_LOCK_MASK  0x00000001 //Mask bit for PHY_INT0
#define PHY_MASK0_HPD_MASK  0x00000002 //Mask bit for PHY_INT0
#define PHY_MASK0_RX_SENSE_0_MASK  0x00000010 //Mask bit for PHY_INT0
#define PHY_MASK0_RX_SENSE_1_MASK  0x00000020 //Mask bit for PHY_INT0
#define PHY_MASK0_RX_SENSE_2_MASK  0x00000040 //Mask bit for PHY_INT0
#define PHY_MASK0_RX_SENSE_3_MASK  0x00000080 //Mask bit for PHY_INT0

//PHY RXSENSE, PLL Lock, and HPD Polarity Register Polarity register for generation of PHY_INT0 interrupts
#define PHY_POL0  0x3007
#define PHY_POL0_TX_PHY_LOCK_MASK  0x00000001 //Polarity bit for PHY_INT0
#define PHY_POL0_HPD_MASK  0x00000002 //Polarity bit for PHY_INT0
#define PHY_POL0_RX_SENSE_0_MASK  0x00000010 //Polarity bit for PHY_INT0
#define PHY_POL0_RX_SENSE_1_MASK  0x00000020 //Polarity bit for PHY_INT0
#define PHY_POL0_RX_SENSE_2_MASK  0x00000040 //Polarity bit for PHY_INT0
#define PHY_POL0_RX_SENSE_3_MASK  0x00000080 //Polarity bit for PHY_INT0

//PHY I2C Slave Address Configuration Register
#define PHY_I2CM_SLAVE  0x00003020
#define PHY_I2CM_SLAVE_SLAVEADDR_MASK  0x0000007F //Slave address to be sent during read and write operations

//PHY I2C Address Configuration Register This register writes the address for read and write operations
#define PHY_I2CM_ADDRESS  0x00003021
#define PHY_I2CM_ADDRESS_ADDRESS_MASK  0x000000FF //Register address for read and write operations

//PHY I2C Data Write Register 1
#define PHY_I2CM_DATAO_1  0x00003022
#define PHY_I2CM_DATAO_1_DATAO_MASK  0x000000FF //Data MSB (datao[15:8]) to be written on register pointed by phy_i2cm_address [7:0]

//PHY I2C Data Write Register 0
#define PHY_I2CM_DATAO_0  0x00003023
#define PHY_I2CM_DATAO_0_DATAO_MASK  0x000000FF //Data LSB (datao[7:0]) to be written on register pointed by phy_i2cm_address [7:0]

//PHY I2C Data Read Register 1
#define PHY_I2CM_DATAI_1  0x00003024
#define PHY_I2CM_DATAI_1_DATAI_MASK  0x000000FF //Data MSB (datai[15:8]) read from register pointed by phy_i2cm_address[7:0]

//PHY I2C Data Read Register 0
#define PHY_I2CM_DATAI_0  0x00003025
#define PHY_I2CM_DATAI_0_DATAI_MASK  0x000000FF //Data LSB (datai[7:0]) read from register pointed by phy_i2cm_address[7:0]

//PHY I2C RD/RD_EXT/WR Operation Register This register requests read and write operations from the I2C Master PHY
#define PHY_I2CM_OPERATION  0x00003026
#define PHY_I2CM_OPERATION_RD_MASK  0x00000001 //Read operation request
#define PHY_I2CM_OPERATION_WR_MASK  0x00000010 //Write operation request

//PHY I2C Done Interrupt Register This register contains and configures I2C master PHY done interrupt
#define PHY_I2CM_INT  0x00003027
#define PHY_I2CM_INT_DONE_STATUS_MASK  0x00000001 //Operation done status bit
#define PHY_I2CM_INT_DONE_INTERRUPT_MASK  0x00000002 //Operation done interrupt bit
#define PHY_I2CM_INT_DONE_MASK_MASK  0x00000004 //Done interrupt mask signal
#define PHY_I2CM_INT_DONE_POL_MASK  0x00000008 //Done interrupt polarity configuration

//PHY I2C error Interrupt Register This register contains and configures the I2C master PHY error interrupts
#define PHY_I2CM_CTLINT  0x00003028
#define PHY_I2CM_CTLINT_ARBITRATION_STATUS_MASK  0x00000001 //Arbitration error status bit
#define PHY_I2CM_CTLINT_ARBITRATION_INTERRUPT_MASK  0x00000002 //Arbitration error interrupt bit {arbitration_interrupt = (arbitration_mask==0b) && (arbitration_status==arbitration_pol)} Note: This bit field is read by the sticky bits present on the ih_i2cmphy_stat0 register
#define PHY_I2CM_CTLINT_ARBITRATION_MASK_MASK  0x00000004 //Arbitration error interrupt mask signal
#define PHY_I2CM_CTLINT_ARBITRATION_POL_MASK  0x00000008 //Arbitration error interrupt polarity configuration
#define PHY_I2CM_CTLINT_NACK_STATUS_MASK  0x00000010 //Not acknowledge error status bit
#define PHY_I2CM_CTLINT_NACK_INTERRUPT_MASK  0x00000020 //Not acknowledge error interrupt bit
#define PHY_I2CM_CTLINT_NACK_MASK_MASK  0x00000040 //Not acknowledge error interrupt mask signal
#define PHY_I2CM_CTLINT_NACK_POL_MASK  0x00000080 //Not acknowledge error interrupt polarity configuration

//PHY I2C Speed control Register This register wets the I2C Master PHY to work in either Fast or Standard mode
#define PHY_I2CM_DIV  0x00003029
#define PHY_I2CM_DIV_SPARE_MASK  0x00000007 //Reserved as "spare" register with no associated functionality
#define PHY_I2CM_DIV_FAST_STD_MODE_MASK  0x00000008 //Sets the I2C Master to work in Fast Mode or Standard Mode: 1: Fast Mode 0: Standard Mode

//PHY I2C SW reset control register This register sets the I2C Master PHY software reset
#define PHY_I2CM_SOFTRSTZ  0x0000302A
#define PHY_I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK  0x00000001 //I2C Master Software Reset

//PHY GEN2 I2C Master Interrupt Status Register (Done and Error Interrupts)
#define IH_I2CMPHY_STAT0  0x00000108
#define IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK  0x00000001 //I2C Master PHY error indication
#define IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK  0x00000002 //I2C Master PHY done indication

//Main Controller PHY Reset Register
#define MC_PHYRSTZ  0x0004005
#define MC_PHYRSTZ_PHYRSTZ_MASK  0x00000001 //HDMI Source PHY active low reset control for PHY GEN1, active high reset control for PHY GEN2

//PHY I2C/JTAG I/O Configuration Control Register
#define JTAG_PHY_CONFIG  0x00003034
#define JTAG_PHY_CONFIG_JTAG_TRST_N_MASK  0x00000001 //Configures the JTAG PHY interface output pin JTAG_TRST_N when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_trst_n when PHY_EXTERNAL=1
#define JTAG_PHY_CONFIG_I2C_JTAGZ_MASK  0x00000010 //Configures the JTAG PHY interface output pin I2C_JTAGZ to select the PHY configuration interface when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_i2c_jtagz when PHY_EXTERNAL=1

/*****************************************************************************
 *                                                                           *
 *                           Audio Sample Registers                          *
 *                                                                           *
 *****************************************************************************/

//Audio I2S Software FIFO Reset, Select, and Enable Control Register 0 This register configures the I2S input enable that indicates which input I2S channels have valid data
#define AUD_CONF0  0x00003100
#define AUD_CONF0_I2S_IN_EN_MASK  0x0000000F //Action I2S_in_en[0] - I2Sdata[0] enable I2S_in_en[1] - I2Sdata[1] enable I2S_in_en[2] - I2Sdata[2] enable I2S_in_en[3] - I2Sdata[3] enable
#define AUD_CONF0_SPARE_1_MASK  0x00000010 //This field is a "spare" bit with no associated functionality
#define AUD_CONF0_I2S_SELECT_MASK  0x00000020 //1b: Selects I2S Audio Interface 0b: Selects the second (SPDIF/GPA) interface, in configurations with more that one audio interface (DOUBLE/GDOUBLE)
#define AUD_CONF0_SPARE_2_MASK  0x00000040 //This field is a "spare" bit with no associated functionality
#define AUD_CONF0_SW_AUDIO_FIFO_RST_MASK  0x00000080 //Audio FIFOs software reset - Writing 0b: no action taken - Writing 1b: Resets all audio FIFOs Reading from this register always returns 0b

//Audio I2S Width and Mode Configuration Register 1 This register configures the I2S mode and data width of the input data
#define AUD_CONF1  0x00003101
#define AUD_CONF1_I2S_WIDTH_MASK  0x0000001F //I2S input data width I2S_width[4:0] | Action 00000b-01111b | Not used 10000b | 16 bit data samples at input 10001b | 17 bit data samples at input 10010b | 18 bit data samples at input 10011b | 19 bit data samples at input 10100b | 20 bit data samples at input 10101b | 21 bit data samples at input 10110b | 22 bit data samples at input 10111b | 23 bit data samples at input 11000b | 24 bit data samples at input 11001b-11111b | Not Used
//I2S input data mode I2S_mode[4:0] | Action
//	000b | Standard I2S mode
//	001b | Right-justified I2S mode
//	010b | Left-justified I2S mode
//	011b | Burst 1 mode
//	100b | Burst 2 mode
#define AUD_CONF1_I2S_MODE_MASK  0x000000E0
//I2S FIFO status and interrupts
#define AUD_INT  0x00003102
#define AUD_INT_FIFO_FULL_MASK_MASK  0x00000004 //FIFO full mask
#define AUD_INT_FIFO_EMPTY_MASK_MASK  0x00000008 //FIFO empty mask

//Audio I2S NLPCM and HBR configuration Register 2 This register configures the I2S Audio Data mapping
#define AUD_CONF2  0x00003103
#define AUD_CONF2_HBR_MASK  0x00000001 //I2S HBR Mode Enable
#define AUD_CONF2_NLPCM_MASK  0x00000002 //I2S NLPCM Mode Enable

//I2S Mask Interrupt Register This register masks the interrupts present in the I2S module
#define AUD_INT1  0x00003104
#define AUD_INT1_FIFO_OVERRUN_MASK_MASK  0x00000010 //FIFO overrun mask

//Audio SPDIF FIFO Empty/Full Mask Register
#define AUD_SPDIFINT  0x00003302
#define AUD_SPDIFINT_SPDIF_FIFO_FULL_MASK_MASK  0x00000004 //SPDIF FIFO full mask
#define AUD_SPDIFINT_SPDIF_FIFO_EMPTY_MASK_MASK  0x00000008 //SPDIF FIFO empty mask

/*****************************************************************************
 *                                                                           *
 *                          Frame Composer Registers                         *
 *                                                                           *
 *****************************************************************************/

//Frame Composer Input Video Configuration and HDCP Keepout Register
#define FC_INVIDCONF  0x00001000
#define FC_INVIDCONF_IN_I_P_MASK  0x00000001 //Input video mode: 1b: Interlaced 0b: Progressive
#define FC_INVIDCONF_R_V_BLANK_IN_OSC_MASK  0x00000002 //Used for CEA861-D modes with fractional Vblank (for example, modes 5, 6, 7, 10, 11, 20, 21, and 22)
#define FC_INVIDCONF_DVI_MODEZ_MASK  0x00000008 //Active low 0b: DVI mode selected 1b: HDMI mode selected
#define FC_INVIDCONF_DE_IN_POLARITY_MASK  0x00000010 //Data enable input polarity 1b: Active high 0b: Active low
#define FC_INVIDCONF_HSYNC_IN_POLARITY_MASK  0x00000020 //Hsync input polarity 1b: Active high 0b: Active low
#define FC_INVIDCONF_VSYNC_IN_POLARITY_MASK  0x00000040 //Vsync input polarity 1b: Active high 0b: Active low
#define FC_INVIDCONF_HDCP_KEEPOUT_MASK  0x00000080 //Start/stop HDCP keepout window generation 1b: Active

//Frame Composer Input Video HActive Pixels Register 0
#define FC_INHACTIV0  0x00001001
#define FC_INHACTIV0_H_IN_ACTIV_MASK  0x000000FF //Input video Horizontal active pixel region width

//Frame Composer Input Video HActive Pixels Register 1
#define FC_INHACTIV1  0x00001002
#define FC_INHACTIV1_H_IN_ACTIV_MASK  0x0000000F //Input video Horizontal active pixel region width
#define FC_INHACTIV1_H_IN_ACTIV_12_MASK  0x00000010 //Input video Horizontal active pixel region width (0
#define FC_INHACTIV1_H_IN_ACTIV_13_MASK  0x00000020 //Input video Horizontal active pixel region width (0

//Frame Composer Input Video HBlank Pixels Register 0
#define FC_INHBLANK0  0x00001003
#define FC_INHBLANK0_H_IN_BLANK_MASK  0x000000FF //Input video Horizontal blanking pixel region width

//Frame Composer Input Video HBlank Pixels Register 1
#define FC_INHBLANK1  0x00001004
#define FC_INHBLANK1_H_IN_BLANK_MASK  0x00000003 //Input video Horizontal blanking pixel region width this bit field holds bits 9:8 of number of Horizontal blanking pixels
#define FC_INHBLANK1_H_IN_BLANK_12_MASK  0x0000001C //Input video Horizontal blanking pixel region width If configuration parameter DWC_HDMI_TX_14 = True (1), this bit field holds bit 12:10 of number of horizontal blanking pixels

//Frame Composer Input Video VActive Pixels Register 0
#define FC_INVACTIV0  0x00001005
#define FC_INVACTIV0_V_IN_ACTIV_MASK  0x000000FF //Input video Vertical active pixel region width

//Frame Composer Input Video VActive Pixels Register 1
#define FC_INVACTIV1  0x00001006
#define FC_INVACTIV1_V_IN_ACTIV_MASK  0x00000007 //Input video Vertical active pixel region width
#define FC_INVACTIV1_V_IN_ACTIV_12_11_MASK  0x00000018 //Input video Vertical active pixel region width

//Frame Composer Input Video VBlank Pixels Register
#define FC_INVBLANK  0x00001007
#define FC_INVBLANK_V_IN_BLANK_MASK  0x000000FF //Input video Vertical blanking pixel region width

//Frame Composer Input Video HSync Front Porch Register 0
#define FC_HSYNCINDELAY0  0x00001008
#define FC_HSYNCINDELAY0_H_IN_DELAY_MASK  0x000000FF //Input video Hsync active edge delay

//Frame Composer Input Video HSync Front Porch Register 1
#define FC_HSYNCINDELAY1  0x00001009
#define FC_HSYNCINDELAY1_H_IN_DELAY_MASK  0x00000007 //Input video Horizontal active edge delay
#define FC_HSYNCINDELAY1_H_IN_DELAY_12_MASK  0x00000018 //Input video Horizontal active edge delay

//Frame Composer Input Video HSync Width Register 0
#define FC_HSYNCINWIDTH0  0x0000100a
#define FC_HSYNCINWIDTH0_H_IN_WIDTH_MASK  0x000000FF //Input video Hsync active pulse width

//Frame Composer Input Video HSync Width Register 1
#define FC_HSYNCINWIDTH1  0x0000100b
#define FC_HSYNCINWIDTH1_H_IN_WIDTH_MASK  0x00000001 //Input video Hsync active pulse width
#define FC_HSYNCINWIDTH1_H_IN_WIDTH_9_MASK  0x00000002 //Input video Hsync active pulse width

//Frame Composer Input Video VSync Front Porch Register
#define FC_VSYNCINDELAY  0x0000100c
#define FC_VSYNCINDELAY_V_IN_DELAY_MASK  0x000000FF //Input video Vsync active edge delay

//Frame Composer Input Video VSync Width Register
#define FC_VSYNCINWIDTH  0x0000100d
#define FC_VSYNCINWIDTH_V_IN_WIDTH_MASK  0x0000003F //Input video Vsync active pulse width

//Frame Composer Input Video Refresh Rate Register 0
#define FC_INFREQ0  0x0000100e
#define FC_INFREQ0_INFREQ_MASK  0x000000FF //Video refresh rate in Hz*1E3 format

//Frame Composer Input Video Refresh Rate Register 1
#define FC_INFREQ1  0x0000100f
#define FC_INFREQ1_INFREQ_MASK  0x000000FF //Video refresh rate in Hz*1E3 format

//Frame Composer Input Video Refresh Rate Register 2
#define FC_INFREQ2  0x00001010
#define FC_INFREQ2_INFREQ_MASK  0x0000000F //Video refresh rate in Hz*1E3 format

//Frame Composer Control Period Duration Register
#define FC_CTRLDUR  0x00001011
#define FC_CTRLDUR_CTRLPERIODDURATION_MASK  0x000000FF //Configuration of the control period minimum duration (minimum of 12 pixel clock cycles; refer to HDMI 1

//Frame Composer Extended Control Period Duration Register
#define FC_EXCTRLDUR  0x00001012
#define FC_EXCTRLDUR_EXCTRLPERIODDURATION_MASK  0x000000FF //Configuration of the extended control period minimum duration (minimum of 32 pixel clock cycles; refer to HDMI 1

//Frame Composer Extended Control Period Maximum Spacing Register
#define FC_EXCTRLSPAC  0x00001013
#define FC_EXCTRLSPAC_EXCTRLPERIODSPACING_MASK  0x000000FF //Configuration of the maximum spacing between consecutive extended control periods (maximum of 50ms; refer to the applicable HDMI specification)

//Frame Composer Channel 0 Non-Preamble Data Register
#define FC_CH0PREAM  0x00001014
#define FC_CH0PREAM_CH0_PREAMBLE_FILTER_MASK  0x000000FF //When in control mode, configures 8 bits that fill the channel 0 data lines not used to transmit the preamble (for more clarification, refer to the HDMI 1

//Frame Composer Channel 1 Non-Preamble Data Register
#define FC_CH1PREAM  0x00001015
#define FC_CH1PREAM_CH1_PREAMBLE_FILTER_MASK  0x0000003F //When in control mode, configures 6 bits that fill the channel 1 data lines not used to transmit the preamble (for more clarification, refer to the HDMI 1

//Frame Composer Channel 2 Non-Preamble Data Register
#define FC_CH2PREAM  0x00001016
#define FC_CH2PREAM_CH2_PREAMBLE_FILTER_MASK  0x0000003F //When in control mode, configures 6 bits that fill the channel 2 data lines not used to transmit the preamble (for more clarification, refer to the HDMI 1

//Frame Composer AVI Packet Configuration Register 3
#define FC_AVICONF3  0x00001017
#define FC_AVICONF3_CN_MASK  0x00000003 //IT content type according to CEA the specification
#define FC_AVICONF3_YQ_MASK  0x0000000C //YCC Quantization range according to the CEA specification

//Frame Composer GCP Packet Configuration Register
#define FC_GCP  0x00001018
#define FC_GCP_CLEAR_AVMUTE_MASK  0x00000001 //Value of "clear_avmute" in the GCP packet
#define FC_GCP_SET_AVMUTE_MASK  0x00000002 //Value of "set_avmute" in the GCP packet Once the AVmute is set, the frame composer schedules the GCP packet with AVmute set in the packet scheduler to be sent once (may only be transmitted between the active edge of VSYNC and 384 pixels following this edge)
#define FC_GCP_DEFAULT_PHASE_MASK  0x00000004 //Value of "default_phase" in the GCP packet

//Frame Composer AVI Packet Configuration Register 0
#define FC_AVICONF0  0x00001019
#define FC_AVICONF0_RGC_YCC_INDICATION_MASK  0x00000003 //Y1,Y0 RGB or YCC indicator
#define FC_AVICONF0_BAR_INFORMATION_MASK  0x0000000C //Bar information data valid
#define FC_AVICONF0_SCAN_INFORMATION_MASK  0x00000030 //Scan information
#define FC_AVICONF0_ACTIVE_FORMAT_PRESENT_MASK  0x00000040 //Active format present
#define FC_AVICONF0_RGC_YCC_INDICATION_2_MASK  0x00000080 //Y2, Bit 2 of rgc_ycc_indication

//Frame Composer AVI Packet Configuration Register 1
#define FC_AVICONF1  0x0000101a
#define FC_AVICONF1_ACTIVE_ASPECT_RATIO_MASK  0x0000000F //Active aspect ratio
#define FC_AVICONF1_PICTURE_ASPECT_RATIO_MASK  0x00000030 //Picture aspect ratio
#define FC_AVICONF1_COLORIMETRY_MASK  0x000000C0 //Colorimetry

//Frame Composer AVI Packet Configuration Register 2
#define FC_AVICONF2  0x0000101b
#define FC_AVICONF2_NON_UNIFORM_PICTURE_SCALING_MASK  0x00000003 //Non-uniform picture scaling
#define FC_AVICONF2_QUANTIZATION_RANGE_MASK  0x0000000C //Quantization range
#define FC_AVICONF2_EXTENDED_COLORIMETRY_MASK  0x00000070 //Extended colorimetry
#define FC_AVICONF2_IT_CONTENT_MASK  0x00000080 //IT content

//Frame Composer AVI Packet VIC Register
#define FC_AVIVID  0x0000101c
#define FC_AVIVID_FC_AVIVID_MASK  0x0000007F //Configures the AVI InfoFrame Video Identification code
#define FC_AVIVID_FC_AVIVID_7_MASK  0x00000080 //Bit 7 of fc_avivid register

#define FC_AVIETB0  0x101d
#define FC_AVIETB1  0x101e
#define FC_AVISBB0  0x101f
#define FC_AVISBB1  0x1020
#define FC_AVIELB0  0x1021
#define FC_AVIELB1  0x1022
#define FC_AVISRB0  0x1023
#define FC_AVISRB1  0x1024

//Frame Composer AUD Packet Configuration Register 0
#define FC_AUDICONF0  0x00001025
#define FC_AUDICONF0_CT_MASK  0x0000000F //Coding Type
#define FC_AUDICONF0_CC_MASK  0x00000070 //Channel count

//Frame Composer AUD Packet Configuration Register 1
#define FC_AUDICONF1  0x00001026
#define FC_AUDICONF1_SF_MASK  0x00000007 //Sampling frequency
#define FC_AUDICONF1_SS_MASK  0x00000030 //Sampling size

//Frame Composer AUD Packet Configuration Register 2
#define FC_AUDICONF2  0x00001027
#define FC_AUDICONF2_CA_MASK  0x000000FF //Channel allocation

//Frame Composer AUD Packet Configuration Register 0
#define FC_AUDICONF3  0x00001028
#define FC_AUDICONF3_LSV_MASK  0x0000000F //Level shift value (for down mixing)
#define FC_AUDICONF3_DM_INH_MASK  0x00000010 //Down mix enable
#define FC_AUDICONF3_LFEPBL_MASK  0x00000060 //LFE playback information LFEPBL1, LFEPBL0 LFE playback level as compared to the other channels

//Frame Composer Audio Sample Flat and Layout Configuration Register
#define FC_AUDSCONF  0x00001063
#define FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK  0x00000001 //Set the audio packet layout to be sent in the packet: 1b: layout 1 0b: layout 0 If DWC_HDMI_TX_20 is defined and register field fc_multistream_ctrl
#define FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK  0x000000F0 //Set the audio packet sample flat value to be sent on the packet

//Frame Composer Audio Sample Channel Status Configuration Register 0
#define FC_AUDSCHNL0  0x00001067
#define FC_AUDSCHNL0_OIEC_COPYRIGHT_MASK  0x00000001 //IEC Copyright indication
#define FC_AUDSCHNL0_OIEC_CGMSA_MASK  0x00000030 //CGMS-A

//Frame Composer Audio Sample Channel Status Configuration Register 1
#define FC_AUDSCHNL1  0x00001068
#define FC_AUDSCHNL1_OIEC_CATEGORYCODE_MASK  0x000000FF //Category code

//Frame Composer Audio Sample Channel Status Configuration Register 2
#define FC_AUDSCHNL2  0x00001069
#define FC_AUDSCHNL2_OIEC_SOURCENUMBER_MASK  0x0000000F //Source number
#define FC_AUDSCHNL2_OIEC_PCMAUDIOMODE_MASK  0x00000070 //PCM audio mode

//Frame Composer Audio Sample Channel Status Configuration Register 3
#define FC_AUDSCHNL3  0x0000106a
#define FC_AUDSCHNL3_OIEC_CHANNELNUMCR0_MASK  0x0000000F //Channel number for first right sample
#define FC_AUDSCHNL3_OIEC_CHANNELNUMCR1_MASK  0x000000F0 //Channel number for second right sample

//Frame Composer Audio Sample Channel Status Configuration Register 4
#define FC_AUDSCHNL4  0x0000106b
#define FC_AUDSCHNL4_OIEC_CHANNELNUMCR2_MASK  0x0000000F //Channel number for third right sample
#define FC_AUDSCHNL4_OIEC_CHANNELNUMCR3_MASK  0x000000F0 //Channel number for fourth right sample

//Frame Composer Audio Sample Channel Status Configuration Register 5
#define FC_AUDSCHNL5  0x0000106c
#define FC_AUDSCHNL5_OIEC_CHANNELNUMCL0_MASK  0x0000000F //Channel number for first left sample
#define FC_AUDSCHNL5_OIEC_CHANNELNUMCL1_MASK  0x000000F0 //Channel number for second left sample

//Frame Composer Audio Sample Channel Status Configuration Register 6
#define FC_AUDSCHNL6  0x0000106d
#define FC_AUDSCHNL6_OIEC_CHANNELNUMCL2_MASK  0x0000000F //Channel number for third left sample
#define FC_AUDSCHNL6_OIEC_CHANNELNUMCL3_MASK  0x000000F0 //Channel number for fourth left sample

//Frame Composer Audio Sample Channel Status Configuration Register 7
#define FC_AUDSCHNL7  0x0000106e
#define FC_AUDSCHNL7_OIEC_SAMPFREQ_MASK  0x0000000F //Sampling frequency
#define FC_AUDSCHNL7_OIEC_CLKACCURACY_MASK  0x00000030 //Clock accuracy
#define FC_AUDSCHNL7_OIEC_SAMPFREQ_EXT_MASK  0x000000C0 //Sampling frequency (channel status bits 31 and 30)

//Frame Composer Audio Sample Channel Status Configuration Register 8
#define FC_AUDSCHNL8  0x0000106f
#define FC_AUDSCHNL8_OIEC_WORDLENGTH_MASK  0x0000000F //Word length configuration
#define FC_AUDSCHNL8_OIEC_ORIGSAMPFREQ_MASK  0x000000F0 //Original sampling frequency

/*****************************************************************************
 *                                                                           *
 *                         Audio Packetizer Registers                        *
 *                                                                           *
 *****************************************************************************/

//Audio Clock Regenerator N Value Register 1 For N expected values, refer to the HDMI 1
#define AUD_N1  0x00003200
#define AUD_N1_AUDN_MASK  0x000000FF //HDMI Audio Clock Regenerator N value

//Audio Clock Regenerator N Value Register 2 For N expected values, refer to the HDMI 1
#define AUD_N2  0x00003201
#define AUD_N2_AUDN_MASK  0x000000FF //HDMI Audio Clock Regenerator N value

//Audio Clock Regenerator N Value Register 3 For N expected values, refer to the HDMI 1
#define AUD_N3  0x00003202
#define AUD_N3_AUDN_MASK  0x0000000F //HDMI Audio Clock Regenerator N value
#define AUD_N3_NCTS_ATOMIC_WRITE_MASK  0x00000080 //When set, the new N and CTS values are only used when aud_n1 register is written

//Audio Clock Regenerator CTS Value Register 1 For CTS expected values, refer to the HDMI 1
#define AUD_CTS1  0x00003203
#define AUD_CTS1_AUDCTS_MASK  0x000000FF //HDMI Audio Clock Regenerator CTS calculated value

//Audio Clock Regenerator CTS Register 2 For CTS expected values, refer to the HDMI 1
#define AUD_CTS2  0x00003204
#define AUD_CTS2_AUDCTS_MASK  0x000000FF //HDMI Audio Clock Regenerator CTS calculated value

//Audio Clock Regenerator CTS value Register 3
#define AUD_CTS3  0x00003205
#define AUD_CTS3_AUDCTS_MASK  0x0000000F //HDMI Audio Clock Regenerator CTS calculated value
#define AUD_CTS3_CTS_MANUAL_MASK  0x00000010 //If the CTS_manual bit equals 0b, this registers contains audCTS[19:0] generated by the Cycle time counter according to the specified timing
#define AUD_CTS3_N_SHIFT_MASK  0x000000E0 //N_shift factor configuration: N_shift | Shift Factor | Action 0 | 1 | This is the N shift factor used for the case that N' ="audN[19:0]"

//Audio Input Clock FS Factor Register
#define AUD_INPUTCLKFS  0x00003206
#define AUD_INPUTCLKFS_IFSFACTOR_MASK  0x00000007 //Fs factor configuration: ifsfactor[2:0] | Audio Clock | Action 0 | 128xFs | If you select the Bypass SPDIF DRU unit in coreConsultant, the input audio clock (either I2S or SPDIF according to configuration) is used at the audio packetizer to calculate the CTS value and ACR packet insertion rate

/*****************************************************************************
 *                                                                           *
 *                              E-DDC Registers                              *
 *                                                                           *
 *****************************************************************************/

//I2C DDC Slave address Configuration Register
#define I2CM_SLAVE  0x7E00
#define I2CM_SLAVE_SLAVEADDR_MASK  0x0000007F //Slave address to be sent during read and write normal operations

//I2C DDC Address Configuration Register
#define I2CM_ADDRESS  0x7E01
#define I2CM_ADDRESS_ADDRESS_MASK  0x000000FF //Register address for read and write operations

//I2C DDC Data Write Register
#define I2CM_DATAO  0x7E02
#define I2CM_DATAO_DATAO_MASK  0x000000FF //Data to be written on register pointed by address[7:0]

//I2C DDC Data read Register
#define I2CM_DATAI  0x7E03
#define I2CM_DATAI_DATAI_MASK  0x000000FF //Data read from register pointed by address[7:0]

//I2C DDC RD/RD_EXT/WR Operation Register Read and write operation request
#define I2CM_OPERATION  0x7E04
#define I2CM_OPERATION_RD_MASK  0x00000001 //Single byte read operation request
#define I2CM_OPERATION_RD_EXT_MASK  0x00000002 //After writing 1'b1 to rd_ext bit a extended data read operation is started (E-DDC read operation)
#define I2CM_OPERATION_RD8_MASK  0x00000004 //Sequential read operation request
#define I2CM_OPERATION_RD8_EXT_MASK  0x00000008 //Extended sequential read operation request
#define I2CM_OPERATION_WR_MASK  0x00000010 //Single byte write operation request
#define I2CM_OPERATION_BUSCLEAR_MASK  0x00000020 //Single byte write operation request

//I2C DDC Done Interrupt Register This register configures the I2C master interrupts
#define I2CM_INT  0x7E05
#define I2CM_INT_DONE_MASK  0x00000004 //Done interrupt mask signal
#define I2CM_INT_READ_REQ_MASK  0x00000040 //Read request interruption mask signal

//I2C DDC error Interrupt Register This register configures the I2C master arbitration lost and not acknowledge error interrupts
#define I2CM_CTLINT  0x7E06
#define I2CM_CTLINT_ARBITRATION_MASK  0x00000004 //Arbitration error interrupt mask signal
#define I2CM_CTLINT_NACK_MASK  0x00000040 //Not acknowledge error interrupt mask signal

//I2C DDC Speed Control Register This register configures the division relation between master and scl clock
#define I2CM_DIV  0x7E07
#define I2CM_DIV_SPARE_MASK  0x00000007 //This bit is a spare register with no associated functionality
#define I2CM_DIV_FAST_STD_MODE_MASK  0x00000008 //Sets the I2C Master to work in Fast Mode or Standard Mode: 1: Fast Mode 0: Standard Mode

//I2C DDC Segment Address Configuration Register This register configures the segment address for extended R/W destination and is used for EDID reading operations, particularly for the Extended Data Read Operation for Enhanced DDC
#define I2CM_SEGADDR  0x7E08
#define I2CM_SEGADDR_SEG_ADDR_MASK  0x0000007F //I2C DDC Segment Address Configuration Register

//I2C DDC Software Reset Control Register This register resets the I2C master
#define I2CM_SOFTRSTZ  0x7E09
#define I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK  0x00000001 //I2C Master Software Reset

//I2C DDC Segment Pointer Register This register configures the segment pointer for extended RD/WR request
#define I2CM_SEGPTR  0x7E0A
#define I2CM_SEGPTR_SEGPTR_MASK  0x000000FF //I2C DDC Segment Pointer Register

//I2C DDC Slow Speed SCL High Level Control Register 1
#define I2CM_SS_SCL_HCNT_1_ADDR  0x7E0B
#define I2CM_SS_SCL_HCNT_1_ADDR_I2CMP_SS_SCL_HCNT1_MASK  0x000000FF //I2C DDC Slow Speed SCL High Level Control Register 1

//I2C DDC Slow Speed SCL High Level Control Register 0
#define I2CM_SS_SCL_HCNT_0_ADDR  0x7E0C
#define I2CM_SS_SCL_HCNT_0_ADDR_I2CMP_SS_SCL_HCNT0_MASK  0x000000FF //I2C DDC Slow Speed SCL High Level Control Register 0

//I2C DDC Slow Speed SCL Low Level Control Register 1
#define I2CM_SS_SCL_LCNT_1_ADDR  0x7E0D
#define I2CM_SS_SCL_LCNT_1_ADDR_I2CMP_SS_SCL_LCNT1_MASK  0x000000FF //I2C DDC Slow Speed SCL Low Level Control Register 1

//I2C DDC Slow Speed SCL Low Level Control Register 0
#define I2CM_SS_SCL_LCNT_0_ADDR  0x7E0E
#define I2CM_SS_SCL_LCNT_0_ADDR_I2CMP_SS_SCL_LCNT0_MASK  0x000000FF //I2C DDC Slow Speed SCL Low Level Control Register 0

//I2C DDC Fast Speed SCL High Level Control Register 1
#define I2CM_FS_SCL_HCNT_1_ADDR  0x7E0F
#define I2CM_FS_SCL_HCNT_1_ADDR_I2CMP_FS_SCL_HCNT1_MASK  0x000000FF //I2C DDC Fast Speed SCL High Level Control Register 1

//I2C DDC Fast Speed SCL High Level Control Register 0
#define I2CM_FS_SCL_HCNT_0_ADDR  0x7E10
#define I2CM_FS_SCL_HCNT_0_ADDR_I2CMP_FS_SCL_HCNT0_MASK  0x000000FF //I2C DDC Fast Speed SCL High Level Control Register 0

//I2C DDC Fast Speed SCL Low Level Control Register 1
#define I2CM_FS_SCL_LCNT_1_ADDR  0x7E11
#define I2CM_FS_SCL_LCNT_1_ADDR_I2CMP_FS_SCL_LCNT1_MASK  0x000000FF //I2C DDC Fast Speed SCL Low Level Control Register 1

//I2C DDC Fast Speed SCL Low Level Control Register 0
#define I2CM_FS_SCL_LCNT_0_ADDR  0x7E12
#define I2CM_FS_SCL_LCNT_0_ADDR_I2CMP_FS_SCL_LCNT0_MASK  0x000000FF //I2C DDC Fast Speed SCL Low Level Control Register 0

//I2C DDC SDA Hold Register
#define I2CM_SDA_HOLD  0x7E13
#define I2CM_SDA_HOLD_OSDA_HOLD_MASK  0x000000FF //Defines the number of SFR clock cycles to meet tHD;DAT (300 ns) osda_hold = round_to_high_integer (300 ns / (1 / isfrclk_frequency))

//SCDC Control Register This register configures the SCDC update status read through the I2C master interface
#define I2CM_SCDC_READ_UPDATE  0x7E14
#define I2CM_SCDC_READ_UPDATE_READ_UPDATE_MASK  0x00000001 //When set to 1'b1, a SCDC Update Read is performed and the read data loaded into registers i2cm_scdc_update0 and i2cm_scdc_update1
#define I2CM_SCDC_READ_UPDATE_READ_REQUEST_EN_MASK  0x00000010 //Read request enabled
#define I2CM_SCDC_READ_UPDATE_UPDTRD_VSYNCPOLL_EN_MASK  0x00000020 //Update read polling enabled

//I2C Master Sequential Read Buffer Register 0
#define I2CM_READ_BUFF0  0x7E20
#define I2CM_READ_BUFF0_I2CM_READ_BUFF0_MASK  0x000000FF //Byte 0 of a I2C read buffer sequential read (from address i2cm_address)

//I2C Master Sequential Read Buffer Register 1
#define I2CM_READ_BUFF1  0x7E21
#define I2CM_READ_BUFF1_I2CM_READ_BUFF1_MASK  0x000000FF //Byte 1 of a I2C read buffer sequential read (from address i2cm_address+1)

//I2C Master Sequential Read Buffer Register 2
#define I2CM_READ_BUFF2  0x7E22
#define I2CM_READ_BUFF2_I2CM_READ_BUFF2_MASK  0x000000FF //Byte 2 of a I2C read buffer sequential read (from address i2cm_address+2)

//I2C Master Sequential Read Buffer Register 3
#define I2CM_READ_BUFF3  0x7E23
#define I2CM_READ_BUFF3_I2CM_READ_BUFF3_MASK  0x000000FF //Byte 3 of a I2C read buffer sequential read (from address i2cm_address+3)

//I2C Master Sequential Read Buffer Register 4
#define I2CM_READ_BUFF4  0x7E24
#define I2CM_READ_BUFF4_I2CM_READ_BUFF4_MASK  0x000000FF //Byte 4 of a I2C read buffer sequential read (from address i2cm_address+4)

//I2C Master Sequential Read Buffer Register 5
#define I2CM_READ_BUFF5  0x7E25
#define I2CM_READ_BUFF5_I2CM_READ_BUFF5_MASK  0x000000FF //Byte 5 of a I2C read buffer sequential read (from address i2cm_address+5)

//I2C Master Sequential Read Buffer Register 6
#define I2CM_READ_BUFF6  0x7E26
#define I2CM_READ_BUFF6_I2CM_READ_BUFF6_MASK  0x000000FF //Byte 6 of a I2C read buffer sequential read (from address i2cm_address+6)

//I2C Master Sequential Read Buffer Register 7
#define I2CM_READ_BUFF7  0x7E27
#define I2CM_READ_BUFF7_I2CM_READ_BUFF7_MASK  0x000000FF //Byte 7 of a I2C read buffer sequential read (from address i2cm_address+7)

//I2C SCDC Read Update Register 0
#define I2CM_SCDC_UPDATE0  0x7E30
#define I2CM_SCDC_UPDATE0_I2CM_SCDC_UPDATE0_MASK  0x000000FF //Byte 0 of a SCDC I2C update sequential read

//I2C SCDC Read Update Register 1
#define I2CM_SCDC_UPDATE1  0x7E31
#define I2CM_SCDC_UPDATE1_I2CM_SCDC_UPDATE1_MASK  0x000000FF //Byte 1 of a SCDC I2C update sequential read

//Global Interrupt Mute Control Register
#define IH_MUTE  0x000001FF
#define IH_MUTE_MUTE_ALL_INTERRUPT_MASK  0x00000001 //When set to 1, mutes the main interrupt line (where all interrupts are ORed)
#define IH_MUTE_MUTE_WAKEUP_INTERRUPT_MASK  0x00000002 //When set to 1, mutes the main interrupt output port

/*****************************************************************************
 *                                                                           *
 *                               HDCP Registers                              *
 *                                                                           *
 *****************************************************************************/

//HDCP Enable and Functional Control Configuration Register 0
#define A_HDCPCFG0  0x0005000
#define A_HDCPCFG0_HDMIDVI_MASK  0x00000001 //Configures the transmitter to operate with a HDMI capable device or with a DVI device
#define A_HDCPCFG0_EN11FEATURE_MASK  0x00000002 //Enable the use of features 1
#define A_HDCPCFG0_RXDETECT_MASK  0x00000004 //Information that a sink device was detected connected to the HDMI port
#define A_HDCPCFG0_AVMUTE_MASK  0x00000008 //This register holds the current AVMUTE state of the DWC_hdmi_tx controller, as expected to be perceived by the connected HDMI/HDCP sink device
#define A_HDCPCFG0_SYNCRICHECK_MASK  0x00000010 //Configures if the Ri check should be done at every 2s even or synchronously to every 128 encrypted frame
#define A_HDCPCFG0_BYPENCRYPTION_MASK  0x00000020 //Bypasses all the data encryption stages
#define A_HDCPCFG0_I2CFASTMODE_MASK  0x00000040 //Enable the I2C fast mode option from the transmitter's side
#define A_HDCPCFG0_ELVENA_MASK  0x00000080 //Enables the Enhanced Link Verification from the transmitter's side

/* Internal macros */
#define _F_START(f)             (0 ? f)
#define _F_END(f)               (1 ? f)
#define _F_SIZE(f)              (1 + _F_END(f) - _F_START(f))
//#define _F_MASK(f)              (((1 << _F_SIZE(f)) - 1) << _F_START(f))
#define _F_MASK(f)              (((1ULL << _F_SIZE(f)) - 1) << _F_START(f))
#define _F_NORMALIZE(v, f)      (((v) & _F_MASK(f)) >> _F_START(f))
#define _F_DENORMALIZE(v, f)    (((v) << _F_START(f)) & _F_MASK(f))

/* Global macros */
#define FIELD_VAL_GET(x, reg, field) \
( \
    _F_NORMALIZE((x), reg ## _ ## field) \
)

#define FIELD_SET(x, reg, field, value) \
( \
    (x & ~_F_MASK(reg ## _ ## field)) \
    | _F_DENORMALIZE(reg ## _ ## field ## _ ## value, reg ## _ ## field) \
)

#define FIELD_VALUE(x, reg, field, value) \
( \
    (x & ~_F_MASK(reg ## _ ## field)) \
    | _F_DENORMALIZE(value, reg ## _ ## field) \
)

#define FIELD_CLEAR(reg, field) \
( \
    ~ _F_MASK(reg ## _ ## field) \
)

/* FIELD MACROS */
#define FIELD_START(field)              (0 ? field)
#define FIELD_END(field)                (1 ? field)
#define FIELD_SIZE(field)               (1 + FIELD_END(field) - FIELD_START(field))
#define FIELD_MASK(field)               (((1 << (FIELD_SIZE(field)-1)) | ((1 << (FIELD_SIZE(field)-1)) - 1)) << FIELD_START(field))
#define FIELD_NORMALIZE(reg, field)     (((reg) & FIELD_MASK(field)) >> FIELD_START(field))
#define FIELD_DENORMALIZE(field, value) (((value) << FIELD_START(field)) & FIELD_MASK(field))
#define FIELD_INIT(reg, field, value)   FIELD_DENORMALIZE(reg ## _ ## field, \
                                                          reg ## _ ## field ## _ ## value)
#define FIELD_INIT_VAL(reg, field, value) \
                                        (FIELD_DENORMALIZE(reg ## _ ## field, value))
#define FIELD_VAL_SET(x, r, f, v)       x = x & ~FIELD_MASK(r ## _ ## f) \
                                              | FIELD_DENORMALIZE(r ## _ ## f, r ## _ ## f ## _ ## v)

#define RGB(r, g, b) ( (unsigned long) (((r) << 16) | ((g) << 8) | (b)) )

#define RGB16(r, g, b) ( (unsigned short) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3)))