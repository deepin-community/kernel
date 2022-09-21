/*
 * WangXun Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 */


#ifndef _NGBE_TYPE_H_
#define _NGBE_TYPE_H_

/*
 * The following is a brief description of the error categories used by the
 * ERROR_REPORT* macros.
 *
 * - NGBE_ERROR_INVALID_STATE
 * This category is for errors which represent a serious failure state that is
 * unexpected, and could be potentially harmful to device operation. It should
 * not be used for errors relating to issues that can be worked around or
 * ignored.
 *
 * - NGBE_ERROR_POLLING
 * This category is for errors related to polling/timeout issues and should be
 * used in any case where the timeout occured, or a failure to obtain a lock, or
 * failure to receive data within the time limit.
 *
 * - NGBE_ERROR_CAUTION
 * This category should be used for reporting issues that may be the cause of
 * other errors, such as temperature warnings. It should indicate an event which
 * could be serious, but hasn't necessarily caused problems yet.
 *
 * - NGBE_ERROR_SOFTWARE
 * This category is intended for errors due to software state preventing
 * something. The category is not intended for errors due to bad arguments, or
 * due to unsupported features. It should be used when a state occurs which
 * prevents action but is not a serious issue.
 *
 * - NGBE_ERROR_ARGUMENT
 * This category is for when a bad or invalid argument is passed. It should be
 * used whenever a function is called and error checking has detected the
 * argument is wrong or incorrect.
 *
 * - NGBE_ERROR_UNSUPPORTED
 * This category is for errors which are due to unsupported circumstances or
 * configuration issues. It should not be used when the issue is due to an
 * invalid argument, but for when something has occurred that is unsupported
 * (Ex: Flow control autonegotiation or an unsupported SFP+ module.)
 */

#include "ngbe_osdep.h"

#define NGBE_NO_LRO


/* Override this by setting IOMEM in your ngbe_osdep.h header */
#ifndef IOMEM
#define IOMEM
#endif

/* Little Endian defines */
#ifndef __le16
#define __le16  u16
#endif
#ifndef __le32
#define __le32  u32
#endif
#ifndef __le64
#define __le64  u64

#endif
#ifndef __be16
/* Big Endian defines */
#define __be16  u16
#define __be32  u32
#define __be64  u64

#endif

/************ ngbe_register.h ************/
/* Vendor ID */
#ifndef PCI_VENDOR_ID_TRUSTNETIC
#define PCI_VENDOR_ID_TRUSTNETIC                0x8088
#endif

/* Device IDs */
/* copper */
#define NGBE_DEV_ID_EM_TEST                   0x0000
#define NGBE_DEV_ID_EM_WX1860AL_W             0x0100
#define NGBE_DEV_ID_EM_WX1860A2               0x0101
#define NGBE_DEV_ID_EM_WX1860A2S              0x0102
#define NGBE_DEV_ID_EM_WX1860A4               0x0103
#define NGBE_DEV_ID_EM_WX1860A4S              0x0104
#define NGBE_DEV_ID_EM_WX1860AL2              0x0105
#define NGBE_DEV_ID_EM_WX1860AL2S             0x0106
#define NGBE_DEV_ID_EM_WX1860AL4              0x0107
#define NGBE_DEV_ID_EM_WX1860AL4S             0x0108
#define NGBE_DEV_ID_EM_WX1860NCSI             0x0109
#define NGBE_DEV_ID_EM_WX1860A1               0x010a
#define NGBE_DEV_ID_EM_WX1860A1L              0x010b




/* transfer units */
#define NGBE_KB_TO_B                          1024

/* Revision ID */
#define NGBE_SP_MPW  1

/* Subsystem ID */
#define NGBE_WX1860AL_INTERNAL                0x0410
#define NGBE_WX1860AL_ZTE5201_RJ45            0x0100
#define NGBE_WX1860AL_M88E1512_RJ45           0x0200
#define NGBE_WX1860AL_M88E1512_SFP            0x0403
#define NGBE_WX1860AL_YT8521S_SFP             0x0460

#define NGBE_SUBSYSTEM_ID_EM_SF100F_LP        0x0103
#define NGBE_SUBSYSTEM_ID_EM_SF100HF_LP       0x0103
#define NGBE_SUBSYSTEM_ID_EM_SF200T           0x0201
#define NGBE_SUBSYSTEM_ID_EM_SF200T_S         0x0210
#define NGBE_SUBSYSTEM_ID_EM_SF400T           0x0401
#define NGBE_SUBSYSTEM_ID_EM_SF400T_S         0x0410
#define NGBE_SUBSYSTEM_ID_EM_SF200HT          0x0202
#define NGBE_SUBSYSTEM_ID_EM_SF200HT_S        0x0220
#define NGBE_SUBSYSTEM_ID_EM_SF400HT          0x0402
#define NGBE_SUBSYSTEM_ID_EM_SF400HT_S        0x0420
#define NGBE_SUBSYSTEM_ID_EM_SF200HXT         0x0230
#define NGBE_SUBSYSTEM_ID_EM_SF400HXT         0x0430
#define NGBE_SUBSYSTEM_ID_EM_SF400_OCP        0x0440
#define NGBE_SUBSYSTEM_ID_EM_SF400_LY         0x0450
#define NGBE_SUBSYSTEM_ID_EM_SF400_LY_YT      0x0470

#define M88E1512_SFP                          0x0003
#define OCP_CARD                              0x0040
#define LY_M88E1512_SFP                       0x0050
#define M88E1512_RJ45                         0x0051
#define M88E1512_MIX                          0x0052
#define YT8521S_SFP                           0x0060
#define LY_YT8521S_SFP                        0x0070
#define INTERNAL_YT8521S_SFP                  0x0061
#define YT8521S_SFP_GPIO                      0x0062
#define INTERNAL_YT8521S_SFP_GPIO             0x0064
#define RGMII_FPGA                            0x0080

#define OEM_MASK                              0x00FF
#define INTERNAL_SFP_MASK                     0x00FF

#define NCSI_SUP                              0x8000
#define NCSI_SUP_MASK                         0x8000

#define WOL_SUP                               0x4000
#define WOL_SUP_MASK                          0x4000


/* MDIO Manageable Devices (MMDs). */
#define NGBE_MDIO_PMA_PMD_DEV_TYPE            0x1 /* PMA and PMD */
#define NGBE_MDIO_PCS_DEV_TYPE                0x3 /* Physical Coding Sublayer*/
#define NGBE_MDIO_PHY_XS_DEV_TYPE             0x4 /* PHY Extender Sublayer */
#define NGBE_MDIO_AUTO_NEG_DEV_TYPE           0x7 /* Auto-Negotiation */
#define NGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE  0x1E /* Vendor specific 1 */

/* phy register definitions */
/* VENDOR_SPECIFIC_1_DEV regs */
#define NGBE_MDIO_VENDOR_SPECIFIC_1_STATUS             0x1 /* VS1 Status Reg */
#define NGBE_MDIO_VENDOR_SPECIFIC_1_LINK_STATUS        0x0008 /* 1 = Link Up */
#define NGBE_MDIO_VENDOR_SPECIFIC_1_SPEED_STATUS       0x0010 /* 0-10G, 1-1G */

/* AUTO_NEG_DEV regs */
#define NGBE_MDIO_AUTO_NEG_CONTROL              0x0  /* AUTO_NEG Control Reg */
#define NGBE_MDIO_AUTO_NEG_ADVT                 0x10 /* AUTO_NEG Advt Reg */
#define NGBE_MDIO_AUTO_NEG_LP                   0x13 /* AUTO_NEG LP Reg */
#define NGBE_MDIO_AUTO_NEG_LP_STATUS            0xE820 /* AUTO NEG RX LP Status
														 * Reg */
#define NGBE_MII_AUTONEG_VENDOR_PROVISION_1_REG 0xC400 /* 1G Provisioning 1 */
#define NGBE_MII_AUTONEG_XNP_TX_REG             0x17   /* 1G XNP Transmit */
#define NGBE_MII_AUTONEG_ADVERTISE_REG          0x10   /* 100M Advertisement */


#define NGBE_MDIO_AUTO_NEG_1000BASE_EEE_ADVT   0x4
#define NGBE_MDIO_AUTO_NEG_100BASE_EEE_ADVT    0x2
#define NGBE_MDIO_AUTO_NEG_LP_1000BASE_CAP     0x8000

#define NGBE_MII_1GBASE_T_ADVERTISE_XNP_TX     0x4000 /* full duplex, bit:14*/
#define NGBE_MII_1GBASE_T_ADVERTISE            0x8000 /* full duplex, bit:15*/
#define NGBE_MII_100BASE_T_ADVERTISE           0x0100 /* full duplex, bit:8 */
#define NGBE_MII_100BASE_T_ADVERTISE_HALF      0x0080 /* half duplex, bit:7 */
#define NGBE_MII_RESTART                       0x200
#define NGBE_MII_AUTONEG_COMPLETE              0x20
#define NGBE_MII_AUTONEG_LINK_UP               0x04
#define NGBE_MII_AUTONEG_REG                   0x0

/* PHY_XS_DEV regs */
#define NGBE_MDIO_PHY_XS_CONTROL               0x0    /* PHY_XS Control Reg */
#define NGBE_MDIO_PHY_XS_RESET                 0x8000 /* PHY_XS Reset */

/* Media-dependent registers. */
#define NGBE_MDIO_PHY_ID_HIGH                  0x2 /* PHY ID High Reg*/
#define NGBE_MDIO_PHY_ID_LOW                   0x3 /* PHY ID Low Reg*/
#define NGBE_MDIO_PHY_SPEED_ABILITY            0x4 /* Speed Ability Reg */
#define NGBE_MDIO_PHY_EXT_ABILITY              0xB /* Ext Ability Reg */

#define NGBE_MDIO_PHY_SPEED_1G                 0x0010 /* 1G capable */
#define NGBE_MDIO_PHY_SPEED_100M               0x0020 /* 100M capable */
#define NGBE_MDIO_PHY_SPEED_10M                0x0040 /* 10M capable */

#define NGBE_MDIO_PHY_1000BASET_ABILITY        0x0020 /* 1000BaseT capable */
#define NGBE_MDIO_PHY_100BASETX_ABILITY        0x0080 /* 100BaseTX capable */

#define NGBE_PHY_REVISION_MASK                 0xFFFFFFF0U
#define NGBE_MAX_PHY_ADDR                      32

#define NGBE_MDIO_CLAUSE_SELECT                0x11220

/* INTERNAL PHY CONTROL */
#define NGBE_INTERNAL_PHY_PAGE_SELECT_OFFSET   31
#define NGBE_INTERNAL_PHY_OFFSET_MAX           32
#define NGBE_INTERNAL_PHY_ID                   0x000732

#define NGBE_INTPHY_LED0                       0x0010
#define NGBE_INTPHY_LED1                       0x0040
#define NGBE_INTPHY_LED2                       0x2000

#define NGBE_INTPHY_INT_LSC                    0x0010
#define NGBE_INTPHY_INT_ANC                    0x0008




/* PHY MDI STANDARD CONFIG */
#define NGBE_MDI_PHY_ID1_OFFSET                2
#define NGBE_MDI_PHY_ID2_OFFSET                3
#define NGBE_MDI_PHY_ID_MASK                   0xFFFFFC00U
#define NGBE_MDI_PHY_SPEED_SELECT1             0x0040
#define NGBE_MDI_PHY_DUPLEX                    0x0100
#define NGBE_MDI_PHY_RESTART_AN                0x0200
#define NGBE_MDI_PHY_ANE                       0x1000
#define NGBE_MDI_PHY_SPEED_SELECT0             0x2000
#define NGBE_MDI_PHY_RESET                     0x8000

#define NGBE_PHY_RST_WAIT_PERIOD               50

#define NGBE_M88E1512_PHY_ID                   0x005043
/* reg 18_0 */
#define NGBE_M88E1512_INT_LSC                  0x0400
#define NGBE_M88E1512_INT_ANC                  0x0800
/* reg 18_3 */
#define NGBE_M88E1512_INT_EN                   0x0080
#define NGBE_M88E1512_INT_POL                  0x0800

/* reg 21_2 */
#define NGBE_M88E1512_RGM_TTC                  0x0010
#define NGBE_M88E1512_RGM_RTC                  0x0020

/* LED control */
#define NGBE_M88E1512_LED1_CONF                0x6
#define NGBE_M88E1512_LED0_CONF                0x1

/* LED polarity */
#define NGBE_M88E1512_LED1_POL                0x1
#define NGBE_M88E1512_LED0_POL                0x1

/* reg 4_0 ADV REG*/
#define NGBE_M88E1512_10BASET_HALF             0x0020
#define NGBE_M88E1512_10BASET_FULL             0x0040
#define NGBE_M88E1512_100BASET_HALF            0x0080
#define NGBE_M88E1512_100BASET_FULL            0x0100

/* reg 9_0 ADV REG*/
#define NGBE_M88E1512_1000BASET_HALF           0x0100
#define NGBE_M88E1512_1000BASET_FULL           0x0200

/* reg 19_0 INT status*/
#define NGBE_M88E1512_ANC                      0x0800
#define NGBE_M88E1512_LSC                      0x0400

/* yt8521s reg */
#define NGBE_YT8521S_PHY_ID                    0x011a

#define NGBE_YT8521S_SDS_LINK_UP               0x4
#define NGBE_YT8521S_SDS_LINK_DOWN             0x8
#define NGBE_YT8521S_UTP_LINK_UP               0x400
#define NGBE_YT8521S_UTP_LINK_DOWN             0x800


/* PHY IDs*/
#define TN1010_PHY_ID                          0x00A19410U
#define QT2022_PHY_ID                          0x0043A400U
#define ATH_PHY_ID                             0x03429050U
/* PHY FW revision */
#define TNX_FW_REV                             0xB
#define AQ_FW_REV                              0x20

/* ETH PHY Registers */
#define NGBE_SR_XS_PCS_MMD_STATUS1             0x30001
#define NGBE_SR_PCS_CTL2                       0x30007
#define NGBE_SR_PMA_MMD_CTL1                   0x10000
#define NGBE_SR_MII_MMD_CTL                    0x1F0000
#define NGBE_SR_MII_MMD_DIGI_CTL               0x1F8000
#define NGBE_SR_MII_MMD_AN_CTL                 0x1F8001
#define NGBE_SR_MII_MMD_AN_ADV                 0x1F0004
#define NGBE_SR_MII_MMD_AN_ADV_PAUSE(_v)       ((0x3 & (_v)) << 7)
#define NGBE_SR_MII_MMD_LP_BABL                0x1F0005
#define NGBE_SR_AN_MMD_CTL                     0x70000
#define NGBE_SR_AN_MMD_ADV_REG1                0x70010
#define NGBE_SR_AN_MMD_ADV_REG1_PAUSE(_v)      ((0x3 & (_v)) << 10)
#define NGBE_SR_AN_MMD_ADV_REG1_PAUSE_SYM      0x400
#define NGBE_SR_AN_MMD_ADV_REG1_PAUSE_ASM      0x800
#define NGBE_SR_AN_MMD_ADV_REG2                0x70011
#define NGBE_SR_AN_MMD_LP_ABL1                 0x70013
#define NGBE_VR_AN_KR_MODE_CL                  0x78003
#define NGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1        0x38000
#define NGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS      0x38010

#define NGBE_PHY_MPLLA_CTL0                    0x18071
#define NGBE_PHY_MPLLA_CTL3                    0x18077
#define NGBE_PHY_MISC_CTL0                     0x18090
#define NGBE_PHY_VCO_CAL_LD0                   0x18092
#define NGBE_PHY_VCO_CAL_LD1                   0x18093
#define NGBE_PHY_VCO_CAL_LD2                   0x18094
#define NGBE_PHY_VCO_CAL_LD3                   0x18095
#define NGBE_PHY_VCO_CAL_REF0                  0x18096
#define NGBE_PHY_VCO_CAL_REF1                  0x18097
#define NGBE_PHY_RX_AD_ACK                     0x18098
#define NGBE_PHY_AFE_DFE_ENABLE                0x1805D
#define NGBE_PHY_DFE_TAP_CTL0                  0x1805E
#define NGBE_PHY_RX_EQ_ATT_LVL0                0x18057
#define NGBE_PHY_RX_EQ_CTL0                    0x18058
#define NGBE_PHY_RX_EQ_CTL                     0x1805C
#define NGBE_PHY_TX_EQ_CTL0                    0x18036
#define NGBE_PHY_TX_EQ_CTL1                    0x18037
#define NGBE_PHY_TX_RATE_CTL                   0x18034
#define NGBE_PHY_RX_RATE_CTL                   0x18054
#define NGBE_PHY_TX_GEN_CTL2                   0x18032
#define NGBE_PHY_RX_GEN_CTL2                   0x18052
#define NGBE_PHY_RX_GEN_CTL3                   0x18053
#define NGBE_PHY_MPLLA_CTL2                    0x18073
#define NGBE_PHY_RX_POWER_ST_CTL               0x18055
#define NGBE_PHY_TX_POWER_ST_CTL               0x18035
#define NGBE_PHY_TX_GENCTRL1                   0x18031

#define NGBE_SR_PCS_CTL2_PCS_TYPE_SEL_R        0x0
#define NGBE_SR_PCS_CTL2_PCS_TYPE_SEL_X        0x1
#define NGBE_SR_PCS_CTL2_PCS_TYPE_SEL_MASK     0x3
#define NGBE_SR_PMA_MMD_CTL1_SPEED_SEL_1G      0x0
#define NGBE_SR_PMA_MMD_CTL1_SPEED_SEL_MASK    0x2000
#define NGBE_SR_PMA_MMD_CTL1_LB_EN             0x1
#define NGBE_SR_MII_MMD_CTL_AN_EN              0x1000
#define NGBE_SR_MII_MMD_CTL_RESTART_AN         0x0200
#define NGBE_SR_AN_MMD_CTL_RESTART_AN          0x0200
#define NGBE_SR_AN_MMD_CTL_ENABLE              0x1000
#define NGBE_SR_AN_MMD_ADV_REG2_BP_TYPE_KX4    0x40
#define NGBE_SR_AN_MMD_ADV_REG2_BP_TYPE_KX     0x20
#define NGBE_SR_AN_MMD_ADV_REG2_BP_TYPE_KR     0x80
#define NGBE_SR_AN_MMD_ADV_REG2_BP_TYPE_MASK   0xFFFF
#define NGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1_ENABLE 0x1000
#define NGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1_VR_RST 0x8000
#define NGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS_PSEQ_MASK            0x1C
#define NGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS_PSEQ_POWER_GOOD      0x10

#define NGBE_PHY_MPLLA_CTL0_MULTIPLIER_1GBASEX_KX              32
#define NGBE_PHY_MPLLA_CTL0_MULTIPLIER_OTHER                   40
#define NGBE_PHY_MPLLA_CTL0_MULTIPLIER_MASK                    0xFF
#define NGBE_PHY_MPLLA_CTL3_MULTIPLIER_BW_1GBASEX_KX           0x46
#define NGBE_PHY_MPLLA_CTL3_MULTIPLIER_BW_OTHER                0x56
#define NGBE_PHY_MPLLA_CTL3_MULTIPLIER_BW_MASK                 0x7FF
#define NGBE_PHY_MISC_CTL0_TX2RX_LB_EN_0                       0x1
#define NGBE_PHY_MISC_CTL0_TX2RX_LB_EN_3_1                     0xE
#define NGBE_PHY_MISC_CTL0_RX_VREF_CTRL                        0x1F00
#define NGBE_PHY_VCO_CAL_LD0_1GBASEX_KX                        1344
#define NGBE_PHY_VCO_CAL_LD0_OTHER                             1360
#define NGBE_PHY_VCO_CAL_LD0_MASK                              0x1000
#define NGBE_PHY_VCO_CAL_REF0_LD0_1GBASEX_KX                   42
#define NGBE_PHY_VCO_CAL_REF0_LD0_OTHER                        34
#define NGBE_PHY_VCO_CAL_REF0_LD0_MASK                         0x3F
#define NGBE_PHY_AFE_DFE_ENABLE_DFE_EN0                        0x10
#define NGBE_PHY_AFE_DFE_ENABLE_AFE_EN0                        0x1
#define NGBE_PHY_AFE_DFE_ENABLE_MASK                           0xFF
#define NGBE_PHY_RX_EQ_CTL_CONT_ADAPT0                         0x1
#define NGBE_PHY_RX_EQ_CTL_CONT_ADAPT_MASK                     0xF
#define NGBE_PHY_TX_RATE_CTL_TX0_RATE_RXAUI                    0x1
#define NGBE_PHY_TX_RATE_CTL_TX0_RATE_1GBASEX_KX               0x3
#define NGBE_PHY_TX_RATE_CTL_TX0_RATE_OTHER                    0x2
#define NGBE_PHY_TX_RATE_CTL_TX1_RATE_OTHER                    0x20
#define NGBE_PHY_TX_RATE_CTL_TX2_RATE_OTHER                    0x200
#define NGBE_PHY_TX_RATE_CTL_TX3_RATE_OTHER                    0x2000
#define NGBE_PHY_TX_RATE_CTL_TX0_RATE_MASK                     0x7
#define NGBE_PHY_TX_RATE_CTL_TX1_RATE_MASK                     0x70
#define NGBE_PHY_TX_RATE_CTL_TX2_RATE_MASK                     0x700
#define NGBE_PHY_TX_RATE_CTL_TX3_RATE_MASK                     0x7000
#define NGBE_PHY_RX_RATE_CTL_RX0_RATE_RXAUI                    0x1
#define NGBE_PHY_RX_RATE_CTL_RX0_RATE_1GBASEX_KX               0x3
#define NGBE_PHY_RX_RATE_CTL_RX0_RATE_OTHER                    0x2
#define NGBE_PHY_RX_RATE_CTL_RX1_RATE_OTHER                    0x20
#define NGBE_PHY_RX_RATE_CTL_RX2_RATE_OTHER                    0x200
#define NGBE_PHY_RX_RATE_CTL_RX3_RATE_OTHER                    0x2000
#define NGBE_PHY_RX_RATE_CTL_RX0_RATE_MASK                     0x7
#define NGBE_PHY_RX_RATE_CTL_RX1_RATE_MASK                     0x70
#define NGBE_PHY_RX_RATE_CTL_RX2_RATE_MASK                     0x700
#define NGBE_PHY_RX_RATE_CTL_RX3_RATE_MASK                     0x7000
#define NGBE_PHY_TX_GEN_CTL2_TX0_WIDTH_OTHER                   0x100
#define NGBE_PHY_TX_GEN_CTL2_TX0_WIDTH_MASK                    0x300
#define NGBE_PHY_TX_GEN_CTL2_TX1_WIDTH_OTHER                   0x400
#define NGBE_PHY_TX_GEN_CTL2_TX1_WIDTH_MASK                    0xC00
#define NGBE_PHY_TX_GEN_CTL2_TX2_WIDTH_OTHER                   0x1000
#define NGBE_PHY_TX_GEN_CTL2_TX2_WIDTH_MASK                    0x3000
#define NGBE_PHY_TX_GEN_CTL2_TX3_WIDTH_OTHER                   0x4000
#define NGBE_PHY_TX_GEN_CTL2_TX3_WIDTH_MASK                    0xC000
#define NGBE_PHY_RX_GEN_CTL2_RX0_WIDTH_OTHER                   0x100
#define NGBE_PHY_RX_GEN_CTL2_RX0_WIDTH_MASK                    0x300
#define NGBE_PHY_RX_GEN_CTL2_RX1_WIDTH_OTHER                   0x400
#define NGBE_PHY_RX_GEN_CTL2_RX1_WIDTH_MASK                    0xC00
#define NGBE_PHY_RX_GEN_CTL2_RX2_WIDTH_OTHER                   0x1000
#define NGBE_PHY_RX_GEN_CTL2_RX2_WIDTH_MASK                    0x3000
#define NGBE_PHY_RX_GEN_CTL2_RX3_WIDTH_OTHER                   0x4000
#define NGBE_PHY_RX_GEN_CTL2_RX3_WIDTH_MASK                    0xC000

#define NGBE_PHY_MPLLA_CTL2_DIV_CLK_EN_8                       0x100
#define NGBE_PHY_MPLLA_CTL2_DIV_CLK_EN_10                      0x200
#define NGBE_PHY_MPLLA_CTL2_DIV_CLK_EN_16P5                    0x400
#define NGBE_PHY_MPLLA_CTL2_DIV_CLK_EN_MASK                    0x700

#define NGBE_XPCS_POWER_GOOD_MAX_POLLING_TIME  100
#define NGBE_PHY_INIT_DONE_POLLING_TIME        100

/**************** Global Registers ****************************/
/* chip control Registers */
#define NGBE_MIS_RST                   0x1000C
#define NGBE_MIS_PWR                   0x10000
#define NGBE_MIS_CTL                   0x10004
#define NGBE_MIS_PF_SM                 0x10008
#define NGBE_MIS_ST                    0x10028
#define NGBE_MIS_SWSM                  0x1002C
#define NGBE_MIS_RST_ST                0x10030

#define NGBE_MIS_RST_SW_RST            0x00000001U
#define NGBE_MIS_RST_LAN0_RST           0x00000002U
#define NGBE_MIS_RST_LAN1_RST           0x00000004U
#define NGBE_MIS_RST_LAN2_RST           0x00000008U
#define NGBE_MIS_RST_LAN3_RST           0x00000010U
#define NGBE_MIS_RST_FW_RST             0x00000020U

#define NGBE_MIS_RST_LAN0_CHG_ETH_MODE 0x20000000U
#define NGBE_MIS_RST_LAN1_CHG_ETH_MODE 0x40000000U
#define NGBE_MIS_RST_GLOBAL_RST        0x80000000U

#define NGBE_MIS_PWR_LAN_ID(_r)        ((0xF0000000U & (_r)) >> 28)
#define NGBE_MIS_PWR_LAN_ID_0          (1)
#define NGBE_MIS_PWR_LAN_ID_1          (2)
#define NGBE_MIS_PWR_LAN_ID_2          (3)
#define NGBE_MIS_PWR_LAN_ID_3          (4)

#define NGBE_MIS_ST_MNG_INIT_DN        0x00000001U
#define NGBE_MIS_ST_MNG_VETO           0x00000100U
#define NGBE_MIS_ST_LAN0_ECC           0x00010000U
#define NGBE_MIS_ST_LAN1_ECC           0x00020000U
#define NGBE_MIS_ST_LAN2_ECC           0x00040000U
#define NGBE_MIS_ST_LAN3_ECC           0x00080000U
#define NGBE_MIS_ST_MNG_ECC            0x00100000U
#define NGBE_MIS_ST_PCORE_ECC          0x00200000U
#define NGBE_MIS_ST_PCIWRP_ECC         0x00400000U
#define NGBE_MIS_ST_PCIEPHY_ECC        0x00800000U
#define NGBE_MIS_ST_FMGR_ECC           0x01000000U
#define NGBE_MIS_ST_GPHY_IN_RST(_r)    (0x00000200U << (_r))


#define NGBE_MIS_SWSM_SMBI             1
#define NGBE_MIS_RST_ST_DEV_RST_ST_DONE        0x00000000U
#define NGBE_MIS_RST_ST_DEV_RST_ST_REQ         0x00080000U
#define NGBE_MIS_RST_ST_DEV_RST_ST_INPROGRESS  0x00100000U
#define NGBE_MIS_RST_ST_DEV_RST_ST_MASK        0x00180000U
#define NGBE_MIS_RST_ST_DEV_RST_TYPE_MASK      0x00070000U
#define NGBE_MIS_RST_ST_DEV_RST_TYPE_SHIFT     16
#define NGBE_MIS_RST_ST_DEV_RST_TYPE_SW_RST    0x3
#define NGBE_MIS_RST_ST_DEV_RST_TYPE_GLOBAL_RST 0x5
#define NGBE_MIS_RST_ST_RST_INIT       0x0000FF00U
#define NGBE_MIS_RST_ST_RST_INI_SHIFT  8
#define NGBE_MIS_RST_ST_RST_TIM        0x000000FFU
#define NGBE_MIS_PF_SM_SM              1

/* Sensors for PVT(Process Voltage Temperature) */
#define NGBE_TS_CTL                    0x10300
#define NGBE_TS_EN                     0x10304
#define NGBE_TS_ST                     0x10308
#define NGBE_TS_ALARM_THRE             0x1030C
#define NGBE_TS_DALARM_THRE            0x10310
#define NGBE_TS_INT_EN                 0x10314
#define NGBE_TS_ALARM_ST               0x10318
#define NGBE_TS_ALARM_ST_DALARM        0x00000002U
#define NGBE_TS_ALARM_ST_ALARM         0x00000001U

#define NGBE_EFUSE_WDATA0              0x10320
#define NGBE_EFUSE_WDATA1              0x10324
#define NGBE_EFUSE_RDATA0              0x10328
#define NGBE_EFUSE_RDATA1              0x1032C
#define NGBE_EFUSE_STATUS              0x10330


#define NGBE_TS_CTL_CALI_DONE          0x80000000U
#define NGBE_TS_EN_ENA                 0x00000001U
#define NGBE_TS_ST_DATA_OUT_MASK       0x000003FFU
#define NGBE_TS_ALARM_THRE_MASK        0x000003FFU
#define NGBE_TS_DALARM_THRE_MASK       0x000003FFU
#define NGBE_TS_INT_EN_DALARM_INT_EN   0x00000002U
#define NGBE_TS_INT_EN_ALARM_INT_EN    0x00000001U

struct ngbe_thermal_diode_data {
	s16 temp;
	s16 alarm_thresh;
	s16 dalarm_thresh;
};

struct ngbe_thermal_sensor_data {
	struct ngbe_thermal_diode_data sensor;
};


/* FMGR Registers */
#define NGBE_SPI_ILDR_STATUS            0x10120
#define NGBE_SPI_ILDR_STATUS_PERST      0x00000001U /* PCIE_PERST is done */
#define NGBE_SPI_ILDR_STATUS_PWRRST     0x00000002U /* Power on reset done */
#define NGBE_SPI_ILDR_STATUS_SW_RESET    0x00000800U /* software reset done */
#define NGBE_SPI_ILDR_STATUS_LAN0_SW_RST 0x00002000U /* lan0 soft reset done */
#define NGBE_SPI_ILDR_STATUS_LAN1_SW_RST 0x00004000U /* lan1 soft reset done */
#define NGBE_SPI_ILDR_STATUS_LAN2_SW_RST 0x00008000U /* lan2 soft reset done */
#define NGBE_SPI_ILDR_STATUS_LAN3_SW_RST 0x00010000U /* lan3 soft reset done */


#define NGBE_MAX_FLASH_LOAD_POLL_TIME  10

#define NGBE_SPI_CMD                   0x10104
#define NGBE_SPI_CMD_CMD(_v)           (((_v) & 0x7) << 28)
#define NGBE_SPI_CMD_CLK(_v)           (((_v) & 0x7) << 25)
#define NGBE_SPI_CMD_ADDR(_v)          (((_v) & 0x7FFFFF))

#define NGBE_SPI_DATA                  0x10108
#define NGBE_SPI_DATA_BYPASS           ((0x1) << 31)
#define NGBE_SPI_DATA_STATUS(_v)       (((_v) & 0xFF) << 16)
#define NGBE_SPI_DATA_OP_DONE          ((0x1))

#define NGBE_SPI_STATUS                0x1010C
#define NGBE_SPI_STATUS_OPDONE         ((0x1))
#define NGBE_SPI_STATUS_FLASH_BYPASS   ((0x1) << 31)

#define NGBE_SPI_USR_CMD               0x10110
#define NGBE_SPI_CMDCFG0               0x10114
#define NGBE_SPI_CMDCFG1               0x10118
#define NGBE_SPI_ILDR_SWPTR            0x10124

/************************* Port Registers ************************************/

/* port cfg Registers */
#define NGBE_CFG_PORT_CTL              0x14400
#define NGBE_CFG_PORT_ST               0x14404
#define NGBE_CFG_EX_VTYPE              0x14408
#define NGBE_CFG_LED_CTL               0x14424

/* internal phy reg_offset [0,31] */
#define NGBE_PHY_CONFIG(reg_offset)    (0x14000 + ((reg_offset) * 4))

#define NGBE_CFG_TCP_TIME              0x14420
#define NGBE_CFG_TAG_TPID(_i)          (0x14430 + ((_i) * 4)) /* [0,3] */
#define NGBE_CFG_LAN_SPEED             0x14440



/* port cfg bit */
#define NGBE_CFG_PORT_CTL_PFRSTD       0x00004000U /* Phy Function Reset Done */
#define NGBE_CFG_PORT_CTL_D_VLAN       0x00000001U /* double vlan*/
#define NGBE_CFG_PORT_CTL_ETAG_ETYPE_VLD 0x00000002U
#define NGBE_CFG_PORT_CTL_QINQ         0x00000004U
#define NGBE_CFG_PORT_CTL_DRV_LOAD     0x00000008U
#define NGBE_CFG_PORT_CTL_NUM_VT_MASK  0x00001000U /* number of TVs */
#define NGBE_CFG_PORT_CTL_NUM_VT_NONE  0x00000000U
#define NGBE_CFG_PORT_CTL_NUM_VT_8    0x00001000U
/* Status Bit */
#define NGBE_CFG_PORT_ST_LINK_1000M    0x00000002U
#define NGBE_CFG_PORT_ST_LINK_100M     0x00000004U
#define NGBE_CFG_PORT_ST_LINK_10M      0x00000008U
#define NGBE_CFG_PORT_ST_LAN_ID(_r)    ((0x00000300U & (_r)) >> 8)
#define NGBE_LINK_UP_TIME              90

/* LED CTL Bit */

#define NGBE_CFG_LED_CTL_LINK_10M_SEL    0x00000008U
#define NGBE_CFG_LED_CTL_LINK_100M_SEL   0x00000004U
#define NGBE_CFG_LED_CTL_LINK_1G_SEL     0x00000002U
#define NGBE_CFG_LED_CTL_LINK_OD_SHIFT   16
/* LED modes */
#define NGBE_LED_LINK_10M              NGBE_CFG_LED_CTL_LINK_10M_SEL
#define NGBE_LED_LINK_1G               NGBE_CFG_LED_CTL_LINK_1G_SEL
#define NGBE_LED_LINK_100M             NGBE_CFG_LED_CTL_LINK_100M_SEL

/* GPIO Registers */
#define NGBE_GPIO_DR                   0x14800
#define NGBE_GPIO_DDR                  0x14804
#define NGBE_GPIO_CTL                  0x14808
#define NGBE_GPIO_INTEN                0x14830
#define NGBE_GPIO_INTMASK              0x14834
#define NGBE_GPIO_INTTYPE_LEVEL        0x14838
#define NGBE_GPIO_POLARITY             0x1483C
#define NGBE_GPIO_INTSTATUS            0x14840
#define NGBE_GPIO_EOI                  0x1484C
/*GPIO bit */
#define NGBE_GPIO_DR_0         0x00000001U /* SDP0 Data Value */
#define NGBE_GPIO_DR_1         0x00000002U /* SDP1 Data Value */
#define NGBE_GPIO_DDR_0        0x00000001U /* SDP0 IO direction */
#define NGBE_GPIO_DDR_1        0x00000002U /* SDP1 IO direction */
#define NGBE_GPIO_CTL_SW_MODE  0x00000000U /* SDP software mode */
#define NGBE_GPIO_INTEN_1      0x00000002U /* SDP1 interrupt enable */
#define NGBE_GPIO_INTEN_2      0x00000004U /* SDP2 interrupt enable */
#define NGBE_GPIO_INTEN_3      0x00000008U /* SDP3 interrupt enable */
#define NGBE_GPIO_INTEN_5      0x00000020U /* SDP5 interrupt enable */
#define NGBE_GPIO_INTEN_6      0x00000040U /* SDP6 interrupt enable */
#define NGBE_GPIO_INTTYPE_LEVEL_2 0x00000004U /* SDP2 interrupt type level */
#define NGBE_GPIO_INTTYPE_LEVEL_3 0x00000008U /* SDP3 interrupt type level */
#define NGBE_GPIO_INTTYPE_LEVEL_5 0x00000020U /* SDP5 interrupt type level */
#define NGBE_GPIO_INTTYPE_LEVEL_6 0x00000040U /* SDP6 interrupt type level */
#define NGBE_GPIO_INTSTATUS_1  0x00000002U /* SDP1 interrupt status */
#define NGBE_GPIO_INTSTATUS_2  0x00000004U /* SDP2 interrupt status */
#define NGBE_GPIO_INTSTATUS_3  0x00000008U /* SDP3 interrupt status */
#define NGBE_GPIO_INTSTATUS_5  0x00000020U /* SDP5 interrupt status */
#define NGBE_GPIO_INTSTATUS_6  0x00000040U /* SDP6 interrupt status */
#define NGBE_GPIO_EOI_2        0x00000004U /* SDP2 interrupt clear */
#define NGBE_GPIO_EOI_3        0x00000008U /* SDP3 interrupt clear */
#define NGBE_GPIO_EOI_5        0x00000020U /* SDP5 interrupt clear */
#define NGBE_GPIO_EOI_6        0x00000040U /* SDP6 interrupt clear */

/* TPH registers */
#define NGBE_CFG_TPH_TDESC     0x14F00 /* TPH conf for Tx desc write back */
#define NGBE_CFG_TPH_RDESC     0x14F04 /* TPH conf for Rx desc write back */
#define NGBE_CFG_TPH_RHDR      0x14F08 /* TPH conf for writing Rx pkt header */
#define NGBE_CFG_TPH_RPL       0x14F0C /* TPH conf for payload write access */
/* TPH bit */
#define NGBE_CFG_TPH_TDESC_EN  0x80000000U
#define NGBE_CFG_TPH_TDESC_PH_SHIFT 29
#define NGBE_CFG_TPH_TDESC_ST_SHIFT 16
#define NGBE_CFG_TPH_RDESC_EN  0x80000000U
#define NGBE_CFG_TPH_RDESC_PH_SHIFT 29
#define NGBE_CFG_TPH_RDESC_ST_SHIFT 16
#define NGBE_CFG_TPH_RHDR_EN   0x00008000U
#define NGBE_CFG_TPH_RHDR_PH_SHIFT 13
#define NGBE_CFG_TPH_RHDR_ST_SHIFT 0
#define NGBE_CFG_TPH_RPL_EN    0x80000000U
#define NGBE_CFG_TPH_RPL_PH_SHIFT 29
#define NGBE_CFG_TPH_RPL_ST_SHIFT 16

/*********************** Transmit DMA registers **************************/
/* transmit global control */
#define NGBE_TDM_CTL           0x18000
#define NGBE_TDM_POOL_TE         0x18004
#define NGBE_TDM_PB_THRE       0x18020



#define NGBE_TDM_LLQ           0x18040
#define NGBE_TDM_ETYPE_LB_L    0x18050

#define NGBE_TDM_ETYPE_AS_L    0x18058
#define NGBE_TDM_MAC_AS_L      0x18060

#define NGBE_TDM_VLAN_AS_L     0x18070

#define NGBE_TDM_TCP_FLG_L     0x18078
#define NGBE_TDM_TCP_FLG_H     0x1807C
#define NGBE_TDM_VLAN_INS(_i)  (0x18100 + ((_i) * 4)) /* 8 of these 0 - 7 */
/* TDM CTL BIT */
#define NGBE_TDM_CTL_TE        0x1 /* Transmit Enable */
#define NGBE_TDM_CTL_PADDING   0x2 /* Padding byte number for ipsec ESP */
#define NGBE_TDM_CTL_VT_SHIFT  16  /* VLAN EtherType */
/* Per VF Port VLAN insertion rules */
#define NGBE_TDM_VLAN_INS_VLANA_DEFAULT 0x40000000U /*Always use default VLAN*/
#define NGBE_TDM_VLAN_INS_VLANA_NEVER   0x80000000U /* Never insert VLAN tag */

#define NGBE_TDM_RP_CTL_RST    ((0x1) << 0)
#define NGBE_TDM_RP_CTL_RPEN   ((0x1) << 2)
#define NGBE_TDM_RP_CTL_RLEN   ((0x1) << 3)
#define NGBE_TDM_RP_RATE_MIN(v) ((0x3FFF & (v)))
#define NGBE_TDM_RP_RATE_MAX(v) ((0x3FFF & (v)) << 16)

/* qos */
#define NGBE_TDM_PBWARB_CTL    0x18200
#define NGBE_TDM_VM_CREDIT_VAL(v) (0x3FF & (v))

/* etag */
#define NGBE_TDM_ETAG_INS(_i)  (0x18700 + ((_i) * 4)) /* 8 of these 0 - 7 */
/* statistic */
#define NGBE_TDM_DRP_CNT       0x18300
#define NGBE_TDM_SEC_DRP       0x18304
#define NGBE_TDM_PKT_CNT       0x18308
#define NGBE_TDM_BYTE_CNT_L    0x1830C
#define NGBE_TDM_BYTE_CNT_H    0x18310
#define NGBE_TDM_OS2BMC_CNT    0x18314

/**************************** Receive DMA registers **************************/
/* receive control */
#define NGBE_RDM_ARB_CTL       0x12000
#define NGBE_RDM_POOL_RE         0x12004

#define NGBE_RDM_PF_QDE        0x12080 
#define NGBE_RDM_PF_HIDE       0x12090
/* VFRE bitmask */
#define NGBE_RDM_POOL_RE_ENABLE_ALL  0xFFFFFFFFU

/* statistic */
#define NGBE_RDM_DRP_PKT           0x12500
#define NGBE_RDM_PKT_CNT           0x12504
#define NGBE_RDM_BYTE_CNT_L        0x12508
#define NGBE_RDM_BYTE_CNT_H        0x1250C
#define NGBE_RDM_BMC2OS_CNT        0x12510

/***************************** RDB registers *********************************/
/* Flow Control Registers */
#define NGBE_RDB_RFCV              0x19200
#define NGBE_RDB_RFCL              0x19220
#define NGBE_RDB_RFCH              0x19260
#define NGBE_RDB_RFCRT             0x192A0
#define NGBE_RDB_RFCC              0x192A4
/* receive packet buffer */
#define NGBE_RDB_PB_WRAP           0x19004
#define NGBE_RDB_PB_SZ             0x19020

#define NGBE_RDB_PB_CTL            0x19000
#define NGBE_RDB_PB_SZ_SHIFT       10
#define NGBE_RDB_PB_SZ_MASK        0x000FFC00U
/* lli interrupt */
#define NGBE_RDB_LLI_THRE          0x19080
#define NGBE_RDB_LLI_THRE_SZ(_v)   ((0xFFF & (_v)))
#define NGBE_RDB_LLI_THRE_UP(_v)   ((0x7 & (_v)) << 16)
#define NGBE_RDB_LLI_THRE_UP_SHIFT 16

/* ring assignment */
#define NGBE_RDB_PL_CFG(_i)    (0x19300 + ((_i) * 4)) /* [0,7] */
#define NGBE_RDB_RSSTBL(_i)    (0x19400 + ((_i) * 4)) /* [0,31] */
#define NGBE_RDB_RSSRK(_i)     (0x19480 + ((_i) * 4)) /* [0,9] */
#define NGBE_RDB_RA_CTL         0x194F4
#define NGBE_RDB_5T_SDP(_i)    (0x19A00 + ((_i) * 4)) /*Src Dst Addr Q Filter*/
#define NGBE_RDB_5T_CTL0(_i)   (0x19C00 + ((_i) * 4)) /* Five Tuple Q Filter */
#define NGBE_RDB_ETYPE_CLS(_i) (0x19100 + ((_i) * 4)) /* EType Q Select */
#define NGBE_RDB_SYN_CLS       0x19130
#define NGBE_RDB_5T_CTL1(_i)   (0x19E00 + ((_i) * 4)) /*8 of these (0-7)*/
/* VM RSS */
#define NGBE_RDB_VMRSSRK(_i, _p)   (0x1A000 + ((_i) * 4) + ((_p) * 0x40))
#define NGBE_RDB_VMRSSTBL(_i, _p)  (0x1B000 + ((_i) * 4) + ((_p) * 0x40))
/* statistic */
#define NGBE_RDB_MPCNT             0x19040
#define NGBE_RDB_PKT_CNT           0x19060
#define NGBE_RDB_REPLI_CNT         0x19064
#define NGBE_RDB_DRP_CNT           0x19068
#define NGBE_RDB_LXONTXC           0x1921C
#define NGBE_RDB_LXOFFTXC          0x19218
#define NGBE_RDB_PFCMACDAL         0x19210
#define NGBE_RDB_PFCMACDAH         0x19214
#define NGBE_RDB_TXSWERR           0x1906C
#define NGBE_RDB_TXSWERR_TB_FREE   0x3FF
/* rdb_pl_cfg reg mask */
#define NGBE_RDB_PL_CFG_L4HDR          0x2
#define NGBE_RDB_PL_CFG_L3HDR          0x4
#define NGBE_RDB_PL_CFG_L2HDR          0x8
#define NGBE_RDB_PL_CFG_TUN_OUTER_L2HDR 0x20
#define NGBE_RDB_PL_CFG_TUN_TUNHDR     0x10
/* RQTC Bit Masks and Shifts */
#define NGBE_RDB_RSS_TC_SHIFT_TC(_i)   ((_i) * 4)
#define NGBE_RDB_RSS_TC_TC0_MASK       (0x7 << 0)
#define NGBE_RDB_RSS_TC_TC1_MASK       (0x7 << 4)
#define NGBE_RDB_RSS_TC_TC2_MASK       (0x7 << 8)
#define NGBE_RDB_RSS_TC_TC3_MASK       (0x7 << 12)
#define NGBE_RDB_RSS_TC_TC4_MASK       (0x7 << 16)
#define NGBE_RDB_RSS_TC_TC5_MASK       (0x7 << 20)
#define NGBE_RDB_RSS_TC_TC6_MASK       (0x7 << 24)
#define NGBE_RDB_RSS_TC_TC7_MASK       (0x7 << 28)
/* Packet Buffer Initialization */
#define NGBE_MAX_PACKET_BUFFERS        8
#define NGBE_RDB_PB_SZ_48KB    0x00000030U /* 48KB Packet Buffer */
#define NGBE_RDB_PB_SZ_64KB    0x00000040U /* 64KB Packet Buffer */
#define NGBE_RDB_PB_SZ_80KB    0x00000050U /* 80KB Packet Buffer */
#define NGBE_RDB_PB_SZ_128KB   0x00000080U /* 128KB Packet Buffer */
#define NGBE_RDB_PB_SZ_MAX     0x00000200U /* 512KB Packet Buffer */


/* Packet buffer allocation strategies */
enum {
	PBA_STRATEGY_EQUAL      = 0, /* Distribute PB space equally */
#define PBA_STRATEGY_EQUAL      PBA_STRATEGY_EQUAL
	PBA_STRATEGY_WEIGHTED   = 1, /* Weight front half of TCs */
#define PBA_STRATEGY_WEIGHTED   PBA_STRATEGY_WEIGHTED
};


/* FCRTL Bit Masks */
#define NGBE_RDB_RFCL_XONE             0x80000000U /* XON enable */
#define NGBE_RDB_RFCH_XOFFE            0x80000000U /* Packet buffer fc enable */
/* FCCFG Bit Masks */
#define NGBE_RDB_RFCC_RFCE_802_3X      0x00000008U /* Tx link FC enable */

/* Immediate Interrupt Rx (A.K.A. Low Latency Interrupt) */
#define NGBE_RDB_5T_CTL1_SIZE_BP       0x00001000U /* Packet size bypass */
#define NGBE_RDB_5T_CTL1_LLI           0x00100000U /* Enables low latency Int */
#define NGBE_RDB_LLI_THRE_PRIORITY_MASK 0x00070000U /* VLAN priority mask */
#define NGBE_RDB_LLI_THRE_PRIORITY_EN  0x00080000U /* VLAN priority enable */

#define NGBE_MAX_RDB_5T_CTL0_FILTERS           128
#define NGBE_RDB_5T_CTL0_PROTOCOL_MASK         0x00000003U
#define NGBE_RDB_5T_CTL0_PROTOCOL_TCP          0x00000000U
#define NGBE_RDB_5T_CTL0_PROTOCOL_UDP          0x00000001U
#define NGBE_RDB_5T_CTL0_PROTOCOL_SCTP         2
#define NGBE_RDB_5T_CTL0_PRIORITY_MASK         0x00000007U
#define NGBE_RDB_5T_CTL0_PRIORITY_SHIFT        2
#define NGBE_RDB_5T_CTL0_POOL_MASK             0x0000003FU
#define NGBE_RDB_5T_CTL0_POOL_SHIFT            8
#define NGBE_RDB_5T_CTL0_5TUPLE_MASK_MASK      0x00000007U
#define NGBE_RDB_5T_CTL0_5TUPLE_MASK_SHIFT     27
#define NGBE_RDB_5T_CTL0_SOURCE_PORT_MASK      0x1B
#define NGBE_RDB_5T_CTL0_DEST_PORT_MASK        0x05
#define NGBE_RDB_5T_CTL0_PROTOCOL_COMP_MASK    0x0F
#define NGBE_RDB_5T_CTL0_POOL_MASK_EN          0x40000000U
#define NGBE_RDB_5T_CTL0_QUEUE_ENABLE          0x80000000U

#define NGBE_RDB_ETYPE_CLS_RX_QUEUE            0x007F0000U /* bits 22:16 */
#define NGBE_RDB_ETYPE_CLS_RX_QUEUE_SHIFT      16
#define NGBE_RDB_ETYPE_CLS_LLI                 0x20000000U /* bit 29 */
#define NGBE_RDB_ETYPE_CLS_QUEUE_EN            0x80000000U /* bit 31 */

/* Receive Config masks */
#define NGBE_RDB_PB_CTL_PBEN           (0x80000000) /* Enable Receiver */
#define NGBE_RDB_PB_CTL_DISABLED       0x1

#define NGBE_RDB_RA_CTL_RSS_EN         0x00000004U /* RSS Enable */
#define NGBE_RDB_RA_CTL_RSS_MASK       0xFFFF0000U
#define NGBE_RDB_RA_CTL_RSS_IPV4_TCP   0x00010000U
#define NGBE_RDB_RA_CTL_RSS_IPV4       0x00020000U
#define NGBE_RDB_RA_CTL_RSS_IPV6       0x00100000U
#define NGBE_RDB_RA_CTL_RSS_IPV6_TCP   0x00200000U
#define NGBE_RDB_RA_CTL_RSS_IPV4_UDP   0x00400000U
#define NGBE_RDB_RA_CTL_RSS_IPV6_UDP   0x00800000U

/******************************* PSR Registers *******************************/
/* psr control */
#define NGBE_PSR_CTL                   0x15000
#define NGBE_PSR_VLAN_CTL              0x15088
#define NGBE_PSR_VM_CTL                0x151B0
#define NGBE_PSR_PKT_CNT               0x151B8
#define NGBE_PSR_MNG_PKT_CNT           0x151BC
#define NGBE_PSR_DBG_DOP_CNT           0x151C0
#define NGBE_PSR_MNG_DOP_CNT           0x151C4
#define NGBE_PSR_VM_FLP_L              0x151C8

/* Header split receive */
#define NGBE_PSR_CTL_SW_EN             0x00040000U
#define NGBE_PSR_CTL_PCSD              0x00002000U
#define NGBE_PSR_CTL_IPPCSE            0x00001000U
#define NGBE_PSR_CTL_BAM               0x00000400U
#define NGBE_PSR_CTL_UPE               0x00000200U
#define NGBE_PSR_CTL_MPE               0x00000100U
#define NGBE_PSR_CTL_MFE               0x00000080U
#define NGBE_PSR_CTL_MO                0x00000060U
#define NGBE_PSR_CTL_TPE               0x00000010U
#define NGBE_PSR_CTL_MO_SHIFT          5
/* VT_CTL bitmasks */
#define NGBE_PSR_VM_CTL_DIS_DEFPL      0x20000000U /* disable default pool */
#define NGBE_PSR_VM_CTL_REPLEN         0x40000000U /* replication enabled */
#define NGBE_PSR_VM_CTL_POOL_SHIFT     7
#define NGBE_PSR_VM_CTL_POOL_MASK      (0x7 << NGBE_PSR_VM_CTL_POOL_SHIFT)
/* VLAN Control Bit Masks */
#define NGBE_PSR_VLAN_CTL_VET          0x0000FFFFU  /* bits 0-15 */
#define NGBE_PSR_VLAN_CTL_CFI          0x10000000U  /* bit 28 */
#define NGBE_PSR_VLAN_CTL_CFIEN        0x20000000U  /* bit 29 */
#define NGBE_PSR_VLAN_CTL_VFE          0x40000000U  /* bit 30 */

/* vm L2 contorl */
#define NGBE_PSR_VM_L2CTL(_i)          (0x15600 + ((_i) * 4))
/* VMOLR bitmasks */
#define NGBE_PSR_VM_L2CTL_LBDIS        0x00000002U /* disable loopback */
#define NGBE_PSR_VM_L2CTL_LLB          0x00000004U /* local pool loopback */
#define NGBE_PSR_VM_L2CTL_UPE          0x00000010U /* unicast promiscuous */
#define NGBE_PSR_VM_L2CTL_TPE          0x00000020U /* ETAG promiscuous */
#define NGBE_PSR_VM_L2CTL_VACC         0x00000040U /* accept nomatched vlan */
#define NGBE_PSR_VM_L2CTL_VPE          0x00000080U /* vlan promiscuous mode */
#define NGBE_PSR_VM_L2CTL_AUPE         0x00000100U /* accept untagged packets */
#define NGBE_PSR_VM_L2CTL_ROMPE        0x00000200U /*accept packets in MTA tbl*/
#define NGBE_PSR_VM_L2CTL_ROPE         0x00000400U /* accept packets in UC tbl*/
#define NGBE_PSR_VM_L2CTL_BAM          0x00000800U /* accept broadcast packets*/
#define NGBE_PSR_VM_L2CTL_MPE          0x00001000U /* multicast promiscuous */

/* etype switcher 1st stage */
#define NGBE_PSR_ETYPE_SWC(_i) (0x15128 + ((_i) * 4)) /* EType Queue Filter */
/* ETYPE Queue Filter/Select Bit Masks */
#define NGBE_MAX_PSR_ETYPE_SWC_FILTERS         8
#define NGBE_PSR_ETYPE_SWC_FCOE                0x08000000U /* bit 27 */
#define NGBE_PSR_ETYPE_SWC_TX_ANTISPOOF        0x20000000U /* bit 29 */
#define NGBE_PSR_ETYPE_SWC_1588                0x40000000U /* bit 30 */
#define NGBE_PSR_ETYPE_SWC_FILTER_EN           0x80000000U /* bit 31 */
#define NGBE_PSR_ETYPE_SWC_POOL_ENABLE         (1 << 26) /* bit 26 */
#define NGBE_PSR_ETYPE_SWC_POOL_SHIFT          20
/*
 * ETQF filter list: one static filter per filter consumer. This is
 *                 to avoid filter collisions later. Add new filters
 *                 here!!
 *
 * Current filters:
 *      EAPOL 802.1x (0x888e): Filter 0
 *      FCoE (0x8906):   Filter 2
 *      1588 (0x88f7):   Filter 3
 *      FIP  (0x8914):   Filter 4
 *      LLDP (0x88CC):   Filter 5
 *      LACP (0x8809):   Filter 6
 *      FC   (0x8808):   Filter 7
 */
#define NGBE_PSR_ETYPE_SWC_FILTER_EAPOL        0
#define NGBE_PSR_ETYPE_SWC_FILTER_FCOE         2
#define NGBE_PSR_ETYPE_SWC_FILTER_1588         3
#define NGBE_PSR_ETYPE_SWC_FILTER_FIP          4
#define NGBE_PSR_ETYPE_SWC_FILTER_LLDP         5
#define NGBE_PSR_ETYPE_SWC_FILTER_LACP         6
#define NGBE_PSR_ETYPE_SWC_FILTER_FC           7

/* mcasst/ucast overflow tbl */
#define NGBE_PSR_MC_TBL(_i)    (0x15200  + ((_i) * 4))
#define NGBE_PSR_UC_TBL(_i)    (0x15400 + ((_i) * 4))

/* vlan tbl */
#define NGBE_PSR_VLAN_TBL(_i)  (0x16000 + ((_i) * 4))

/* mac switcher */
#define NGBE_PSR_MAC_SWC_AD_L  0x16200
#define NGBE_PSR_MAC_SWC_AD_H  0x16204
#define NGBE_PSR_MAC_SWC_VM    0x16208
#define NGBE_PSR_MAC_SWC_IDX   0x16210
/* RAH */
#define NGBE_PSR_MAC_SWC_AD_H_AD(v)       (((v) & 0xFFFF))
#define NGBE_PSR_MAC_SWC_AD_H_ADTYPE(v)   (((v) & 0x1) << 30)
#define NGBE_PSR_MAC_SWC_AD_H_AV       0x80000000U
#define NGBE_CLEAR_VMDQ_ALL            0xFFFFFFFFU

/* vlan switch */
#define NGBE_PSR_VLAN_SWC      0x16220
#define NGBE_PSR_VLAN_SWC_VM_L 0x16224
#define NGBE_PSR_VLAN_SWC_IDX  0x16230         /* 32 vlan entries */
/* VLAN pool filtering masks */
#define NGBE_PSR_VLAN_SWC_VIEN         0x80000000U  /* filter is valid */
#define NGBE_PSR_VLAN_SWC_ENTRIES      32
#define NGBE_PSR_VLAN_SWC_VLANID_MASK  0x00000FFFU
#define NGBE_ETHERNET_IEEE_VLAN_TYPE   0x8100  /* 802.1q protocol */

/* Manangbeent */
#define NGBE_PSR_MNG_FIT_CTL           0x15820
/* Manangbeent Bit Fields and Masks */
#define NGBE_PSR_MNG_FIT_CTL_MPROXYE    0x40000000U /* Manangbeent Proxy Enable*/
#define NGBE_PSR_MNG_FIT_CTL_RCV_TCO_EN 0x00020000U /* Rcv TCO packet enable */
#define NGBE_PSR_MNG_FIT_CTL_EN_BMC2OS  0x10000000U /* Ena BMC2OS and OS2BMC
													  *traffic */
#define NGBE_PSR_MNG_FIT_CTL_EN_BMC2OS_SHIFT   28

#define NGBE_PSR_MNG_FLEX_SEL  0x1582C
#define NGBE_PSR_MNG_FLEX_DW_L(_i) (0x15A00 + ((_i) * 16)) /* [0,15] */
#define NGBE_PSR_MNG_FLEX_DW_H(_i) (0x15A04 + ((_i) * 16))
#define NGBE_PSR_MNG_FLEX_MSK(_i)  (0x15A08 + ((_i) * 16))

/* mirror */
#define NGBE_PSR_MR_CTL(_i)    (0x15B00 + ((_i) * 4)) /* [0,3] */
#define NGBE_PSR_MR_VLAN_L(_i) (0x15B10 + ((_i) * 8))
#define NGBE_PSR_MR_VM_L(_i)   (0x15B30 + ((_i) * 8))

/* 1588 */
#define NGBE_PSR_1588_CTL      0x15188 /* Rx Time Sync Control register - RW */
#define NGBE_PSR_1588_STMPL    0x151E8 /* Rx timestamp Low - RO */
#define NGBE_PSR_1588_STMPH    0x151A4 /* Rx timestamp High - RO */
#define NGBE_PSR_1588_ATTRL    0x151A0 /* Rx timestamp attribute low - RO */
#define NGBE_PSR_1588_ATTRH    0x151A8 /* Rx timestamp attribute high - RO */
#define NGBE_PSR_1588_MSGTYPE  0x15120 /* RX message type register low - RW */
/* 1588 CTL Bit */
#define NGBE_PSR_1588_CTL_VALID            0x00000001U /* Rx timestamp valid */
#define NGBE_PSR_1588_CTL_TYPE_MASK        0x0000000EU /* Rx type mask */
#define NGBE_PSR_1588_CTL_TYPE_L2_V2       0x00
#define NGBE_PSR_1588_CTL_TYPE_L4_V1       0x02
#define NGBE_PSR_1588_CTL_TYPE_L2_L4_V2    0x04
#define NGBE_PSR_1588_CTL_TYPE_EVENT_V2    0x0A
#define NGBE_PSR_1588_CTL_ENABLED          0x00000010U /* Rx Timestamp enabled*/
/* 1588 msg type bit */
#define NGBE_PSR_1588_MSGTYPE_V1_CTRLT_MASK            0x000000FFU
#define NGBE_PSR_1588_MSGTYPE_V1_SYNC_MSG              0x00
#define NGBE_PSR_1588_MSGTYPE_V1_DELAY_REQ_MSG         0x01
#define NGBE_PSR_1588_MSGTYPE_V1_FOLLOWUP_MSG          0x02
#define NGBE_PSR_1588_MSGTYPE_V1_DELAY_RESP_MSG        0x03
#define NGBE_PSR_1588_MSGTYPE_V1_MGMT_MSG              0x04
#define NGBE_PSR_1588_MSGTYPE_V2_MSGID_MASK            0x0000FF00U
#define NGBE_PSR_1588_MSGTYPE_V2_SYNC_MSG              0x0000
#define NGBE_PSR_1588_MSGTYPE_V2_DELAY_REQ_MSG         0x0100
#define NGBE_PSR_1588_MSGTYPE_V2_PDELAY_REQ_MSG        0x0200
#define NGBE_PSR_1588_MSGTYPE_V2_PDELAY_RESP_MSG       0x0300
#define NGBE_PSR_1588_MSGTYPE_V2_FOLLOWUP_MSG          0x0800
#define NGBE_PSR_1588_MSGTYPE_V2_DELAY_RESP_MSG        0x0900
#define NGBE_PSR_1588_MSGTYPE_V2_PDELAY_FOLLOWUP_MSG   0x0A00
#define NGBE_PSR_1588_MSGTYPE_V2_ANNOUNCE_MSG          0x0B00
#define NGBE_PSR_1588_MSGTYPE_V2_SIGNALLING_MSG        0x0C00
#define NGBE_PSR_1588_MSGTYPE_V2_MGMT_MSG              0x0D00

/* Wake up registers */
#define NGBE_PSR_WKUP_CTL      0x15B80
#define NGBE_PSR_WKUP_IPV      0x15B84
#define NGBE_PSR_LAN_FLEX_SEL  0x15B8C
#define NGBE_PSR_WKUP_IP4TBL(_i)       (0x15BC0 + ((_i) * 4)) /* [0,3] */
#define NGBE_PSR_WKUP_IP6TBL(_i)       (0x15BE0 + ((_i) * 4))
#define NGBE_PSR_LAN_FLEX_DW_L(_i)     (0x15C00 + ((_i) * 16)) /* [0,15] */
#define NGBE_PSR_LAN_FLEX_DW_H(_i)     (0x15C04 + ((_i) * 16))
#define NGBE_PSR_LAN_FLEX_MSK(_i)      (0x15C08 + ((_i) * 16))
#define NGBE_PSR_LAN_FLEX_CTL  0x15CFC
/* Wake Up Filter Control Bit */
#define NGBE_PSR_WKUP_CTL_LNKC 0x00000001U /* Link Status Change Wakeup Enable*/
#define NGBE_PSR_WKUP_CTL_MAG  0x00000002U /* Magic Packet Wakeup Enable */
#define NGBE_PSR_WKUP_CTL_EX   0x00000004U /* Directed Exact Wakeup Enable */
#define NGBE_PSR_WKUP_CTL_MC   0x00000008U /* Directed Multicast Wakeup Enable*/
#define NGBE_PSR_WKUP_CTL_BC   0x00000010U /* Broadcast Wakeup Enable */
#define NGBE_PSR_WKUP_CTL_ARP  0x00000020U /* ARP Request Packet Wakeup Enable*/
#define NGBE_PSR_WKUP_CTL_IPV4 0x00000040U /* Directed IPv4 Pkt Wakeup Enable */
#define NGBE_PSR_WKUP_CTL_IPV6 0x00000080U /* Directed IPv6 Pkt Wakeup Enable */
#define NGBE_PSR_WKUP_CTL_IGNORE_TCO   0x00008000U /* Ignore WakeOn TCO pkts */
#define NGBE_PSR_WKUP_CTL_FLX0         0x00010000U /* Flexible Filter 0 Ena */
#define NGBE_PSR_WKUP_CTL_FLX1         0x00020000U /* Flexible Filter 1 Ena */
#define NGBE_PSR_WKUP_CTL_FLX2         0x00040000U /* Flexible Filter 2 Ena */
#define NGBE_PSR_WKUP_CTL_FLX3         0x00080000U /* Flexible Filter 3 Ena */
#define NGBE_PSR_WKUP_CTL_FLX4         0x00100000U /* Flexible Filter 4 Ena */
#define NGBE_PSR_WKUP_CTL_FLX5         0x00200000U /* Flexible Filter 5 Ena */
#define NGBE_PSR_WKUP_CTL_FLX_FILTERS  0x000F0000U /* Mask for 4 flex filters */
#define NGBE_PSR_WKUP_CTL_FLX_FILTERS_6 0x003F0000U /* Mask for 6 flex filters*/
#define NGBE_PSR_WKUP_CTL_FLX_FILTERS_8 0x00FF0000U /* Mask for 8 flex filters*/
#define NGBE_PSR_WKUP_CTL_FW_RST_WK    0x80000000U /* Ena wake on FW reset
													 * assertion */
/* Mask for Ext. flex filters */
#define NGBE_PSR_WKUP_CTL_EXT_FLX_FILTERS  0x00300000U
#define NGBE_PSR_WKUP_CTL_ALL_FILTERS   0x000F00FFU /* Mask all 4 flex filters*/
#define NGBE_PSR_WKUP_CTL_ALL_FILTERS_6 0x003F00FFU /* Mask all 6 flex filters*/
#define NGBE_PSR_WKUP_CTL_ALL_FILTERS_8 0x00FF00FFU /* Mask all 8 flex filters*/
#define NGBE_PSR_WKUP_CTL_FLX_OFFSET    16 /* Offset to the Flex Filters bits*/

#define NGBE_PSR_MAX_SZ                0x15020

/****************************** TDB ******************************************/
#define NGBE_TDB_RFCS                  0x1CE00
#define NGBE_TDB_PB_SZ                  0x1CC00

#define NGBE_TDB_PRB_CTL               0x17010
#define NGBE_TDB_PBRARB_CTL            0x1CD00

#define NGBE_TDB_PB_SZ_MAX     0x00005000U /* 20KB Packet Buffer */
#define NGBE_TXPKT_SIZE_MAX    0xA /* Max Tx Packet size */
#define NGBE_MAX_PB            8
/* statistic */
#define NGBE_TDB_OUT_PKT_CNT           0x1CF00
#define NGBE_TDB_MNG_PKT_CNT           0x1CF04
#define NGBE_TDB_LB_PKT_CNT            0x1CF08
#define NGBE_TDB_MNG_LARGE_DOP_CNT     0x1CF0C

/****************************** TSEC *****************************************/
/* Security Control Registers */
#define NGBE_TSEC_CTL                   0x1D000
#define NGBE_TSEC_ST                    0x1D004
#define NGBE_TSEC_BUF_AF                0x1D008
#define NGBE_TSEC_BUF_AE                0x1D00C
#define NGBE_TSEC_MIN_IFG               0x1D020

/* 1588 */
#define NGBE_TSEC_1588_CTL              0x11F00 /* Tx Time Sync Control reg */
#define NGBE_TSEC_1588_STMPL            0x11F04 /* Tx timestamp value Low */
#define NGBE_TSEC_1588_STMPH            0x11F08 /* Tx timestamp value High */
#define NGBE_TSEC_1588_SYSTIML          0x11F0C /* System time register Low */
#define NGBE_TSEC_1588_SYSTIMH          0x11F10 /* System time register High */
#define NGBE_TSEC_1588_INC              0x11F14 /* Increment attributes reg */
#define NGBE_TSEC_1588_INC_IV(v)        ((v) & 0x7FFFFFF)

#define NGBE_TSEC_1588_ADJL             0x11F18 /* Time Adjustment Offset reg Low */
#define NGBE_TSEC_1588_ADJH             0x11F1C /* Time Adjustment Offset reg High*/

#define NGBE_TSEC_1588_INT_ST           0x11F20
#define NGBE_TSEC_1588_INT_EN           0x11F24

/* 1588 fields */
#define NGBE_TSEC_1588_CTL_VALID    0x00000001U /* Tx timestamp valid */
#define NGBE_TSEC_1588_CTL_ENABLED  0x00000010U /* Tx timestamping enabled */

#define NGBE_TSEC_1588_AUX_CTL          0x11F28
#define NGBE_TSEC_1588_TRGT_L(i)        (0x11F2C + ((i) * 8)) /* [0,1] */
#define NGBE_TSEC_1588_TRGT_H(i)        (0x11F30 + ((i) * 8)) /* [0,1] */
#define NGBE_TSEC_1588_FREQ_CLK_L(i)    (0x11F3C + ((i) * 8)) /* [0,1] */
#define NGBE_TSEC_1588_FREQ_CLK_H(i)    (0x11F40 + ((i) * 8)) /* [0,1] */
#define NGBE_TSEC_1588_AUX_STMP_L(i)    (0x11F4C + ((i) * 8)) /* [0,1] */
#define NGBE_TSEC_1588_AUX_STMP_H(i)    (0x11F50 + ((i) * 8)) /* [0,1] */
#define NGBE_TSEC_1588_SDP(n)           (0x11F5C + ((n) * 4)) /* [0,3] */



/********************************* RSEC **************************************/
/* general rsec */
#define NGBE_RSEC_CTL                   0x17000
#define NGBE_RSEC_ST                    0x17004
/* general rsec fields */
#define NGBE_RSEC_CTL_SECRX_DIS         0x00000001U
#define NGBE_RSEC_CTL_RX_DIS            0x00000002U
#define NGBE_RSEC_CTL_CRC_STRIP         0x00000004U
#define NGBE_RSEC_CTL_SAVE_MAC_ERR      0x00000040U
#define NGBE_RSEC_ST_RSEC_RDY           0x00000001U
#define NGBE_RSEC_ST_RSEC_OFLD_DIS      0x00000002U
#define NGBE_RSEC_ST_ECC_RXERR          0x00000004U

/* link sec */
#define NGBE_RSEC_LSEC_CAP              0x17200
#define NGBE_RSEC_LSEC_CTL              0x17204
#define NGBE_RSEC_LSEC_SCI_L            0x17208
#define NGBE_RSEC_LSEC_SCI_H            0x1720C
#define NGBE_RSEC_LSEC_SA0              0x17210
#define NGBE_RSEC_LSEC_SA1              0x17214
#define NGBE_RSEC_LSEC_PKNUM0           0x17218
#define NGBE_RSEC_LSEC_PKNUM1           0x1721C
#define NGBE_RSEC_LSEC_KEY0(_n)         0x17220
#define NGBE_RSEC_LSEC_KEY1(_n)         0x17230
#define NGBE_RSEC_LSEC_UNTAG_PKT        0x17240
#define NGBE_RSEC_LSEC_DEC_OCTET        0x17244
#define NGBE_RSEC_LSEC_VLD_OCTET        0x17248
#define NGBE_RSEC_LSEC_BAD_PKT          0x1724C
#define NGBE_RSEC_LSEC_NOSCI_PKT        0x17250
#define NGBE_RSEC_LSEC_UNSCI_PKT        0x17254
#define NGBE_RSEC_LSEC_UNCHK_PKT        0x17258
#define NGBE_RSEC_LSEC_DLY_PKT          0x1725C
#define NGBE_RSEC_LSEC_LATE_PKT         0x17260
#define NGBE_RSEC_LSEC_OK_PKT(_n)       0x17264
#define NGBE_RSEC_LSEC_INV_PKT(_n)      0x17274
#define NGBE_RSEC_LSEC_BADSA_PKT        0x1727C
#define NGBE_RSEC_LSEC_INVSA_PKT        0x17280

/* ipsec */
#define NGBE_RSEC_IPS_IDX               0x17100
#define NGBE_RSEC_IPS_IDX_WT        0x80000000U
#define NGBE_RSEC_IPS_IDX_RD        0x40000000U
#define NGBE_RSEC_IPS_IDX_TB_IDX    0x0U /* */
#define NGBE_RSEC_IPS_IDX_TB_IP     0x00000002U
#define NGBE_RSEC_IPS_IDX_TB_SPI    0x00000004U
#define NGBE_RSEC_IPS_IDX_TB_KEY    0x00000006U
#define NGBE_RSEC_IPS_IDX_EN        0x00000001U
#define NGBE_RSEC_IPS_IP(i)             (0x17104 + ((i) * 4))
#define NGBE_RSEC_IPS_SPI               0x17114
#define NGBE_RSEC_IPS_IP_IDX            0x17118
#define NGBE_RSEC_IPS_KEY(i)            (0x1711C + ((i) * 4))
#define NGBE_RSEC_IPS_SALT              0x1712C
#define NGBE_RSEC_IPS_MODE              0x17130
#define NGBE_RSEC_IPS_MODE_IPV6         0x00000010
#define NGBE_RSEC_IPS_MODE_DEC          0x00000008
#define NGBE_RSEC_IPS_MODE_ESP          0x00000004
#define NGBE_RSEC_IPS_MODE_AH           0x00000002
#define NGBE_RSEC_IPS_MODE_VALID        0x00000001

/************************************** ETH PHY ******************************/
#define NGBE_XPCS_IDA_ADDR    0x13000
#define NGBE_XPCS_IDA_DATA    0x13004
#define NGBE_ETHPHY_IDA_ADDR  0x13008
#define NGBE_ETHPHY_IDA_DATA  0x1300C

/************************************** MNG ********************************/
#define NGBE_MNG_FW_SM         0x1E000
#define NGBE_MNG_SW_SM         0x1E004
#define NGBE_MNG_SWFW_SYNC     0x1E008
#define NGBE_MNG_MBOX          0x1E100
#define NGBE_MNG_MBOX_CTL      0x1E044


#define NGBE_MNG_OS2BMC_CNT    0x1E094
#define NGBE_MNG_BMC2OS_CNT    0x1E090

/* Firmware Semaphore Register */
#define NGBE_MNG_FW_SM_MODE_MASK       0xE
#define NGBE_MNG_FW_SM_TS_ENABLED      0x1
/* SW Semaphore Register bitmasks */
#define NGBE_MNG_SW_SM_SM              0x00000001U /* software Semaphore */

/* SW_FW_SYNC definitions */
#define NGBE_MNG_SWFW_SYNC_SW_PHY      0x0001
#define NGBE_MNG_SWFW_SYNC_SW_FLASH    0x0008
#define NGBE_MNG_SWFW_SYNC_SW_MB       0x0004

#define NGBE_MNG_MBOX_CTL_SWRDY        0x1
#define NGBE_MNG_MBOX_CTL_SWACK        0x2
#define NGBE_MNG_MBOX_CTL_FWRDY        0x4
#define NGBE_MNG_MBOX_CTL_FWACK        0x8

/************************************* ETH MAC *****************************/
#define NGBE_MAC_TX_CFG                0x11000
#define NGBE_MAC_RX_CFG                0x11004
#define NGBE_MAC_PKT_FLT               0x11008
#define NGBE_MAC_PKT_FLT_PR            (0x1) /* promiscuous mode */
#define NGBE_MAC_PKT_FLT_RA            (0x80000000) /* receive all */
#define NGBE_MAC_WDG_TIMEOUT           0x1100C
#define NGBE_MAC_TX_FLOW_CTRL          0x11070
#define NGBE_MAC_RX_FLOW_CTRL          0x11090
#define NGBE_MAC_INT_ST                0x110B0
#define NGBE_MAC_INT_EN                0x110B4
#define NGBE_MAC_ADDRESS0_HIGH         0x11300
#define NGBE_MAC_ADDRESS0_LOW          0x11304

#define NGBE_MAC_TX_CFG_TE             0x00000001U
#define NGBE_MAC_TX_CFG_SPEED_MASK     0x60000000U
#define NGBE_MAC_TX_CFG_SPEED_1G       0x60000000U
#define NGBE_MAC_RX_CFG_RE             0x00000001U
#define NGBE_MAC_RX_CFG_JE             0x00000100U
#define NGBE_MAC_RX_CFG_LM             0x00000400U
#define NGBE_MAC_WDG_TIMEOUT_PWE       0x00000100U
#define NGBE_MAC_WDG_TIMEOUT_WTO_MASK  0x0000000FU
#define NGBE_MAC_WDG_TIMEOUT_WTO_DELTA 2

#define NGBE_MAC_RX_FLOW_CTRL_RFE      0x00000001U /* receive fc enable */

#define NGBE_MSCA                      0x11200
#define NGBE_MSCA_RA(v)                ((0xFFFF & (v)))
#define NGBE_MSCA_PA(v)                ((0x1F & (v)) << 16)
#define NGBE_MSCA_DA(v)                ((0x1F & (v)) << 21)
#define NGBE_MSCC                      0x11204
#define NGBE_MSCC_DATA(v)              ((0xFFFF & (v)))
#define NGBE_MSCC_CMD(v)               ((0x3 & (v)) << 16)
enum NGBE_MSCA_CMD_value {
	NGBE_MSCA_CMD_RSV = 0,
	NGBE_MSCA_CMD_WRITE,
	NGBE_MSCA_CMD_POST_READ,
	NGBE_MSCA_CMD_READ,
};
#define NGBE_MSCC_SADDR                ((0x1U) << 18)
#define NGBE_MSCC_CR(v)                ((0x8U & (v)) << 19)
#define NGBE_MSCC_BUSY                 ((0x1U) << 22)
#define NGBE_MDIO_CLK(v)               ((0x7 & (v)) << 19)


/* EEE registers */

/* statistic */
#define NGBE_MAC_LXOFFRXC              0x11988
#define NGBE_MAC_PXOFFRXC              0x119DC
#define NGBE_RX_BC_FRAMES_GOOD_LOW     0x11918
#define NGBE_RX_CRC_ERROR_FRAMES_LOW   0x11928
#define NGBE_RX_LEN_ERROR_FRAMES_LOW   0x11978
#define NGBE_RX_UNDERSIZE_FRAMES_GOOD  0x11938
#define NGBE_RX_OVERSIZE_FRAMES_GOOD   0x1193C
#define NGBE_RX_FRAME_CNT_GOOD_BAD_LOW 0x11900
#define NGBE_TX_FRAME_CNT_GOOD_BAD_LOW 0x1181C
#define NGBE_TX_MC_FRAMES_GOOD_LOW     0x1182C
#define NGBE_TX_BC_FRAMES_GOOD_LOW     0x11824
#define NGBE_MMC_CONTROL               0x11800
#define NGBE_MMC_CONTROL_RSTONRD       0x4 /* reset on read */
#define NGBE_MMC_CONTROL_UP            0x700


/********************************* BAR registers ***************************/
/* Interrupt Registers */
#define NGBE_BME_CTL                           0x12020
#define NGBE_PX_MISC_IC                        0x100
#define NGBE_PX_MISC_ICS                       0x104
#define NGBE_PX_MISC_IEN                       0x108
#define NGBE_PX_MISC_IVAR                      0x4FC
#define NGBE_PX_GPIE                           0x118
#define NGBE_PX_ISB_ADDR_L                     0x160
#define NGBE_PX_ISB_ADDR_H                     0x164
#define NGBE_PX_TCP_TIMER                      0x170
#define NGBE_PX_ITRSEL                         0x180
#define NGBE_PX_IC                             0x120
#define NGBE_PX_ICS                            0x130
#define NGBE_PX_IMS                            0x140
#define NGBE_PX_IMC                            0x150
#define NGBE_PX_IVAR(_i)                       (0x500 + (_i) * 4) /* [0,3] */
#define NGBE_PX_ITR(_i)                        (0x200 + (_i) * 4) /* [0,8] */
#define NGBE_PX_TRANSACTION_PENDING            0x168
#define NGBE_PX_INTA                           0x110

/* Interrupt register bitmasks */
/* Extended Interrupt Cause Read */
#define NGBE_PX_MISC_IC_DEV_RST        0x00000400U /* device reset event */
#define NGBE_PX_MISC_IC_TIMESYNC       0x00000800U /* time sync */
#define NGBE_PX_MISC_IC_STALL          0x00001000U /* trans or recv path is
													 * stalled */
#define NGBE_PX_MISC_IC_LINKSEC        0x00002000U /* Tx LinkSec require key
													 * exchange */
#define NGBE_PX_MISC_IC_RX_MISS        0x00004000U /* Packet Buffer Overrun */
#define NGBE_PX_MISC_IC_I2C            0x00010000U /* I2C interrupt */
#define NGBE_PX_MISC_IC_ETH_EVENT      0x00020000U /* err reported by MAC except
													 * eth link down */
#define NGBE_PX_MISC_IC_PHY            0x00040000U /* link up */
#define NGBE_PX_MISC_IC_INT_ERR        0x00100000U /* integrity error */
#define NGBE_PX_MISC_IC_SPI            0x00200000U /* SPI interface */
#define NGBE_PX_MISC_IC_VF_MBOX        0x00800000U /* VF-PF message box */
#define NGBE_PX_MISC_IC_GPIO           0x04000000U /* GPIO interrupt */
#define NGBE_PX_MISC_IC_PCIE_REQ_ERR   0x08000000U /* pcie request error int */
#define NGBE_PX_MISC_IC_OVER_HEAT      0x10000000U /* overheat detection */
#define NGBE_PX_MISC_IC_PROBE_MATCH    0x20000000U /* probe match */
#define NGBE_PX_MISC_IC_MNG_HOST_MBOX  0x40000000U /* mng mailbox */
#define NGBE_PX_MISC_IC_TIMER          0x80000000U /* tcp timer */

/* Extended Interrupt Cause Set */
#define NGBE_PX_MISC_ICS_ETH_LKDN      0x00000100U
#define NGBE_PX_MISC_ICS_DEV_RST       0x00000400U
#define NGBE_PX_MISC_ICS_TIMESYNC      0x00000800U
#define NGBE_PX_MISC_ICS_STALL         0x00001000U
#define NGBE_PX_MISC_ICS_LINKSEC       0x00002000U
#define NGBE_PX_MISC_ICS_RX_MISS       0x00004000U
#define NGBE_PX_MISC_ICS_FLOW_DIR      0x00008000U
#define NGBE_PX_MISC_ICS_I2C           0x00010000U
#define NGBE_PX_MISC_ICS_ETH_EVENT     0x00020000U
#define NGBE_PX_MISC_ICS_ETH_LK        0x00040000U
#define NGBE_PX_MISC_ICS_ETH_AN        0x00080000U
#define NGBE_PX_MISC_ICS_INT_ERR       0x00100000U
#define NGBE_PX_MISC_ICS_SPI           0x00200000U
#define NGBE_PX_MISC_ICS_VF_MBOX       0x00800000U
#define NGBE_PX_MISC_ICS_GPIO          0x04000000U
#define NGBE_PX_MISC_ICS_PCIE_REQ_ERR  0x08000000U
#define NGBE_PX_MISC_ICS_OVER_HEAT     0x10000000U
#define NGBE_PX_MISC_ICS_PROBE_MATCH   0x20000000U
#define NGBE_PX_MISC_ICS_MNG_HOST_MBOX 0x40000000U
#define NGBE_PX_MISC_ICS_TIMER         0x80000000U

/* Extended Interrupt Enable Set */
#define NGBE_PX_MISC_IEN_ETH_LKDN      0x00000100U
#define NGBE_PX_MISC_IEN_DEV_RST       0x00000400U
#define NGBE_PX_MISC_IEN_TIMESYNC      0x00000800U
#define NGBE_PX_MISC_IEN_STALL         0x00001000U
#define NGBE_PX_MISC_IEN_LINKSEC       0x00002000U
#define NGBE_PX_MISC_IEN_RX_MISS       0x00004000U
#define NGBE_PX_MISC_IEN_I2C           0x00010000U
#define NGBE_PX_MISC_IEN_ETH_EVENT     0x00020000U
#define NGBE_PX_MISC_IEN_ETH_LK        0x00040000U
#define NGBE_PX_MISC_IEN_ETH_AN        0x00080000U
#define NGBE_PX_MISC_IEN_INT_ERR       0x00100000U
#define NGBE_PX_MISC_IEN_SPI           0x00200000U
#define NGBE_PX_MISC_IEN_VF_MBOX       0x00800000U
#define NGBE_PX_MISC_IEN_GPIO          0x04000000U
#define NGBE_PX_MISC_IEN_PCIE_REQ_ERR  0x08000000U
#define NGBE_PX_MISC_IEN_OVER_HEAT     0x10000000U
#define NGBE_PX_MISC_IEN_PROBE_MATCH   0x20000000U
#define NGBE_PX_MISC_IEN_MNG_HOST_MBOX 0x40000000U
#define NGBE_PX_MISC_IEN_TIMER         0x80000000U

#define NGBE_PX_MISC_IEN_MASK ( \
				NGBE_PX_MISC_IEN_ETH_LKDN| \
				NGBE_PX_MISC_IEN_DEV_RST | \
				NGBE_PX_MISC_IEN_ETH_EVENT | \
				NGBE_PX_MISC_IEN_ETH_LK | \
				NGBE_PX_MISC_IEN_ETH_AN | \
				NGBE_PX_MISC_IEN_INT_ERR | \
				NGBE_PX_MISC_IEN_VF_MBOX | \
				NGBE_PX_MISC_IEN_GPIO | \
				NGBE_PX_MISC_IEN_MNG_HOST_MBOX | \
				NGBE_PX_MISC_IEN_STALL | \
				NGBE_PX_MISC_IEN_PCIE_REQ_ERR | \
				NGBE_PX_MISC_IEN_TIMER)

/* General purpose Interrupt Enable */
#define NGBE_PX_GPIE_MODEL             0x00000001U
#define NGBE_PX_GPIE_IMEN              0x00000002U
#define NGBE_PX_GPIE_LL_INTERVAL       0x000000F0U

/* Interrupt Vector Allocation Registers */
#define NGBE_PX_IVAR_REG_NUM              64
#define NGBE_PX_IVAR_ALLOC_VAL            0x80 /* Interrupt Allocation valid */

#define NGBE_MAX_INT_RATE              500000
#define NGBE_MIN_INT_RATE              980
#define NGBE_MAX_EITR                  0x00007FFCU
#define NGBE_MIN_EITR                  4
#define NGBE_PX_ITR_ITR_INT_MASK       0x00000FF8U
#define NGBE_PX_ITR_LLI_CREDIT         0x001f0000U
#define NGBE_PX_ITR_LLI_MOD            0x00008000U
#define NGBE_PX_ITR_CNT_WDIS           0x80000000U
#define NGBE_PX_ITR_ITR_CNT            0x0FE00000U

/* transmit DMA Registers */
#define NGBE_PX_TR_BAL(_i)     (0x03000 + ((_i) * 0x40)) /* [0, 7] */
#define NGBE_PX_TR_BAH(_i)     (0x03004 + ((_i) * 0x40))
#define NGBE_PX_TR_WP(_i)      (0x03008 + ((_i) * 0x40))
#define NGBE_PX_TR_RP(_i)      (0x0300C + ((_i) * 0x40))
#define NGBE_PX_TR_CFG(_i)     (0x03010 + ((_i) * 0x40))
/* Transmit Config masks */
#define NGBE_PX_TR_CFG_ENABLE          (1) /* Ena specific Tx Queue */
#define NGBE_PX_TR_CFG_TR_SIZE_SHIFT   1 /* tx desc number per ring */
#define NGBE_PX_TR_CFG_SWFLSH          (1 << 26) /* Tx Desc. wr-bk flushing */
#define NGBE_PX_TR_CFG_WTHRESH_SHIFT   16 /* shift to WTHRESH bits */
#define NGBE_PX_TR_CFG_THRE_SHIFT      8

#define NGBE_PX_TR_RPn(q_per_pool, vf_number, vf_q_index) \
		(NGBE_PX_TR_RP((q_per_pool)*(vf_number) + (vf_q_index)))

#define NGBE_PX_TR_WPn(q_per_pool, vf_number, vf_q_index) \
		(NGBE_PX_TR_WP((q_per_pool)*(vf_number) + (vf_q_index)))

/* Receive DMA Registers */
#define NGBE_PX_RR_BAL(_i)             (0x01000 + ((_i) * 0x40)) /* [0, 7] */
#define NGBE_PX_RR_BAH(_i)             (0x01004 + ((_i) * 0x40))
#define NGBE_PX_RR_WP(_i)              (0x01008 + ((_i) * 0x40))
#define NGBE_PX_RR_RP(_i)              (0x0100C + ((_i) * 0x40))
#define NGBE_PX_RR_CFG(_i)             (0x01010 + ((_i) * 0x40))
/* PX_RR_CFG bit definitions */
#define NGBE_PX_RR_CFG_RR_SIZE_SHIFT           1
#define NGBE_PX_RR_CFG_BSIZEPKT_SHIFT          2 /* so many KBs */
#define NGBE_PX_RR_CFG_BSIZEHDRSIZE_SHIFT      6 /* 64byte resolution (>> 6)
						* + at bit 8 offset (<< 12)
						*  = (<< 6)
						*/
#define NGBE_PX_RR_CFG_DROP_EN         0x40000000U
#define NGBE_PX_RR_CFG_VLAN            0x80000000U
#define NGBE_PX_RR_CFG_RSC             0x20000000U
#define NGBE_PX_RR_CFG_CNTAG           0x10000000U
#define NGBE_PX_RR_CFG_RSC_CNT_MD      0x08000000U
#define NGBE_PX_RR_CFG_SPLIT_MODE      0x04000000U
#define NGBE_PX_RR_CFG_STALL           0x02000000U
#define NGBE_PX_RR_CFG_MAX_RSCBUF_1    0x00000000U
#define NGBE_PX_RR_CFG_MAX_RSCBUF_4    0x00800000U
#define NGBE_PX_RR_CFG_MAX_RSCBUF_8    0x01000000U
#define NGBE_PX_RR_CFG_MAX_RSCBUF_16   0x01800000U
#define NGBE_PX_RR_CFG_RR_THER         0x00070000U
#define NGBE_PX_RR_CFG_RR_THER_SHIFT   16

#define NGBE_PX_RR_CFG_RR_HDR_SZ       0x0000F000U
#define NGBE_PX_RR_CFG_RR_BUF_SZ       0x00000F00U
#define NGBE_PX_RR_CFG_RR_SZ           0x0000007EU
#define NGBE_PX_RR_CFG_RR_EN           0x00000001U

/* statistic */
#define NGBE_PX_MPRC(_i)               (0x1020 + ((_i) * 64)) /* [0,7] */
#define NGBE_PX_BPRC(_i)               (0x1024 + ((_i) * 64))


#define NGBE_PX_MPTC(_i)               (0x3020 + ((_i) * 64)) /* [0,7] */
#define NGBE_PX_BPTC(_i)               (0x3024 + ((_i) * 64))

#define NGBE_VX_GPRC                   0x01014
#define NGBE_VX_GORC_LSB               0x01018
#define NGBE_VX_GORC_MSB               0x0101C
#define NGBE_VX_MPRC                   0x01020
#define NGBE_VX_BPRC                   0x01024

#define NGBE_VX_GPTC                   0x03014
#define NGBE_VX_GOTC_LSB               0x03018
#define NGBE_VX_GOTC_MSB               0x0301C
#define NGBE_VX_MPTC                   0x03020
#define NGBE_VX_BPTC                   0x03024



#define NGBE_PX_GPRC                   0x12504

#define NGBE_PX_GPTC                   0x18308

#define NGBE_PX_GORC_LSB               0x12508
#define NGBE_PX_GORC_MSB               0x1250C

#define NGBE_PX_GOTC_LSB               0x1830C
#define NGBE_PX_GOTC_MSB               0x18310

/*************************** Flash region definition *************************/
/* EEC Register */
#define NGBE_EEC_SK            0x00000001U /* EEPROM Clock */
#define NGBE_EEC_CS            0x00000002U /* EEPROM Chip Select */
#define NGBE_EEC_DI            0x00000004U /* EEPROM Data In */
#define NGBE_EEC_DO            0x00000008U /* EEPROM Data Out */
#define NGBE_EEC_FWE_MASK      0x00000030U /* FLASH Write Enable */
#define NGBE_EEC_FWE_DIS       0x00000010U /* Disable FLASH writes */
#define NGBE_EEC_FWE_EN        0x00000020U /* Enable FLASH writes */
#define NGBE_EEC_FWE_SHIFT     4
#define NGBE_EEC_REQ           0x00000040U /* EEPROM Access Request */
#define NGBE_EEC_GNT           0x00000080U /* EEPROM Access Grant */
#define NGBE_EEC_PRES          0x00000100U /* EEPROM Present */
#define NGBE_EEC_ARD           0x00000200U /* EEPROM Auto Read Done */
#define NGBE_EEC_FLUP          0x00800000U /* Flash update command */
#define NGBE_EEC_SEC1VAL       0x02000000U /* Sector 1 Valid */
#define NGBE_EEC_FLUDONE       0x04000000U /* Flash update done */
/* EEPROM Addressing bits based on type (0-small, 1-large) */
#define NGBE_EEC_ADDR_SIZE     0x00000400U
#define NGBE_EEC_SIZE          0x00007800U /* EEPROM Size */
#define NGBE_EERD_MAX_ADDR     0x00003FFFU /* EERD alows 14 bits for addr. */

#define NGBE_EEC_SIZE_SHIFT            11
#define NGBE_EEPROM_WORD_SIZE_SHIFT    6
#define NGBE_EEPROM_OPCODE_BITS        8

/* FLA Register */
#define NGBE_FLA_LOCKED        0x00000040U

/* Part Number String Length */
#define NGBE_PBANUM_LENGTH     32

/* Checksum and EEPROM pointers */
#define NGBE_PBANUM_PTR_GUARD          0xFAFA
#define NGBE_CHECKSUM_CAP_ST_PASS      0x80658383
#define NGBE_CHECKSUM_CAP_ST_FAIL      0x70657376
#define NGBE_ERR_ST                    0xffffffff
#define NGBE_EEPROM_CHECKSUM           0x2F
#define NGBE_EEPROM_SUM                0xBABA
#define NGBE_OPTION_ROM_PTR            0x05
#define NGBE_SHADOW_RAM_SIZE           0x4000
#define NGBE_PCIE_CONFIG_SIZE          0x08
#define NGBE_EEPROM_LAST_WORD          0x800
#define NGBE_FW_PTR                    0x0F
#define NGBE_SW_REGION_PTR             0x28

#define NGBE_CALSUM_COMMAND            0xE9
#define NGBE_CALSUM_CAP_STATUS         0x10224
#define NGBE_EEPROM_VERSION_STORE_REG  0x1022C
#define NGBE_SAN_MAC_ADDR_PTR          0x18
#define NGBE_DEVICE_CAPS               0x1C
#define NGBE_EEPROM_VERSION_L          0x1D
#define NGBE_EEPROM_VERSION_H          0x1E

#define NGBE_MAX_MSIX_VECTORS_EMERALD  0x09

/* MSI-X capability fields masks */
#define NGBE_PCIE_MSIX_TBL_SZ_MASK     0x7FF

/* EEPROM Commands - SPI */
#define NGBE_EEPROM_MAX_RETRY_SPI      5000 /* Max wait 5ms for RDY signal */
#define NGBE_EEPROM_STATUS_RDY_SPI     0x01
#define NGBE_EEPROM_READ_OPCODE_SPI    0x03  /* EEPROM read opcode */
#define NGBE_EEPROM_WRITE_OPCODE_SPI   0x02  /* EEPROM write opcode */
#define NGBE_EEPROM_A8_OPCODE_SPI      0x08  /* opcode bit-3 = addr bit-8 */
#define NGBE_EEPROM_WREN_OPCODE_SPI    0x06  /* EEPROM set Write Ena latch */
/* EEPROM reset Write Enable latch */
#define NGBE_EEPROM_WRDI_OPCODE_SPI        0x04
#define NGBE_EEPROM_RDSR_OPCODE_SPI        0x05  /* EEPROM read Status reg */
#define NGBE_EEPROM_WRSR_OPCODE_SPI        0x01  /* EEPROM write Status reg */
#define NGBE_EEPROM_ERASE4K_OPCODE_SPI     0x20  /* EEPROM ERASE 4KB */
#define NGBE_EEPROM_ERASE64K_OPCODE_SPI    0xD8  /* EEPROM ERASE 64KB */
#define NGBE_EEPROM_ERASE256_OPCODE_SPI    0xDB  /* EEPROM ERASE 256B */

/* EEPROM Read Register */
#define NGBE_EEPROM_RW_REG_DATA        16 /* data offset in EEPROM read reg */
#define NGBE_EEPROM_RW_REG_DONE        2 /* Offset to READ done bit */
#define NGBE_EEPROM_RW_REG_START       1 /* First bit to start operation */
#define NGBE_EEPROM_RW_ADDR_SHIFT      2 /* Shift to the address bits */
#define NGBE_NVM_POLL_WRITE            1 /* Flag for polling for wr complete */
#define NGBE_NVM_POLL_READ             0 /* Flag for polling for rd complete */

#define NVM_INIT_CTRL_3                 0x38
#define NVM_INIT_CTRL_3_LPLU            0x8

#define NGBE_ETH_LENGTH_OF_ADDRESS     6

#define NGBE_EEPROM_PAGE_SIZE_MAX      128
#define NGBE_EEPROM_RD_BUFFER_MAX_COUNT        256 /* words rd in burst */
#define NGBE_EEPROM_WR_BUFFER_MAX_COUNT        256 /* words wr in burst */
#define NGBE_EEPROM_CTRL_2             1 /* EEPROM CTRL word 2 */
#define NGBE_EEPROM_CCD_BIT            2

#ifndef NGBE_EEPROM_GRANT_ATTEMPTS
#define NGBE_EEPROM_GRANT_ATTEMPTS     1000 /* EEPROM attempts to gain grant */
#endif

#ifndef NGBE_EERD_EEWR_ATTEMPTS
/* Number of 5 microseconds we wait for EERD read and
 * EERW write to complete */
#define NGBE_EERD_EEWR_ATTEMPTS        100000
#endif

#ifndef NGBE_FLUDONE_ATTEMPTS
/* # attempts we wait for flush update to complete */
#define NGBE_FLUDONE_ATTEMPTS          20000
#endif

#define NGBE_PCIE_CTRL2                0x5   /* PCIe Control 2 Offset */
#define NGBE_PCIE_CTRL2_DUMMY_ENABLE   0x8   /* Dummy Function Enable */
#define NGBE_PCIE_CTRL2_LAN_DISABLE    0x2   /* LAN PCI Disable */
#define NGBE_PCIE_CTRL2_DISABLE_SELECT 0x1   /* LAN Disable Select */

#define NGBE_SAN_MAC_ADDR_PORT0_OFFSET         0x0
#define NGBE_SAN_MAC_ADDR_PORT1_OFFSET         0x3
#define NGBE_DEVICE_CAPS_ALLOW_ANY_SFP         0x1
#define NGBE_DEVICE_CAPS_FCOE_OFFLOADS         0x2
#define NGBE_FW_LESM_PARAMETERS_PTR            0x2
#define NGBE_FW_LESM_STATE_1                   0x1
#define NGBE_FW_LESM_STATE_ENABLED             0x8000 /* LESM Enable bit */
#define NGBE_FW_PASSTHROUGH_PATCH_CONFIG_PTR   0x4
#define NGBE_FW_PATCH_VERSION_4                0x7
#define NGBE_FCOE_IBA_CAPS_BLK_PTR             0x33 /* iSCSI/FCOE block */
#define NGBE_FCOE_IBA_CAPS_FCOE                0x20 /* FCOE flags */
#define NGBE_ISCSI_FCOE_BLK_PTR                0x17 /* iSCSI/FCOE block */
#define NGBE_ISCSI_FCOE_FLAGS_OFFSET           0x0 /* FCOE flags */
#define NGBE_ISCSI_FCOE_FLAGS_ENABLE           0x1 /* FCOE flags enable bit */
#define NGBE_ALT_SAN_MAC_ADDR_BLK_PTR          0x17 /* Alt. SAN MAC block */
#define NGBE_ALT_SAN_MAC_ADDR_CAPS_OFFSET      0x0 /* Alt SAN MAC capability */
#define NGBE_ALT_SAN_MAC_ADDR_PORT0_OFFSET     0x1 /* Alt SAN MAC 0 offset */
#define NGBE_ALT_SAN_MAC_ADDR_PORT1_OFFSET     0x4 /* Alt SAN MAC 1 offset */
#define NGBE_ALT_SAN_MAC_ADDR_WWNN_OFFSET      0x7 /* Alt WWNN prefix offset */
#define NGBE_ALT_SAN_MAC_ADDR_WWPN_OFFSET      0x8 /* Alt WWPN prefix offset */
#define NGBE_ALT_SAN_MAC_ADDR_CAPS_SANMAC      0x0 /* Alt SAN MAC exists */
#define NGBE_ALT_SAN_MAC_ADDR_CAPS_ALTWWN      0x1 /* Alt WWN base exists */
#define NGBE_DEVICE_CAPS_WOL_PORT0_1   0x4 /* WoL supported on ports 0 & 1 */
#define NGBE_DEVICE_CAPS_WOL_PORT0     0x8 /* WoL supported on port 0 */
#define NGBE_DEVICE_CAPS_WOL_MASK      0xC /* Mask for WoL capabilities */

/******************************** PCI Bus Info *******************************/
#define NGBE_PCI_DEVICE_STATUS         0xAA
#define NGBE_PCI_DEVICE_STATUS_TRANSACTION_PENDING     0x0020
#define NGBE_PCI_LINK_STATUS           0xB2
#define NGBE_PCI_DEVICE_CONTROL2       0xC8
#define NGBE_PCI_LINK_WIDTH            0x3F0
#define NGBE_PCI_LINK_WIDTH_1          0x10
#define NGBE_PCI_LINK_WIDTH_2          0x20
#define NGBE_PCI_LINK_WIDTH_4          0x40
#define NGBE_PCI_LINK_WIDTH_8          0x80
#define NGBE_PCI_LINK_SPEED            0xF
#define NGBE_PCI_LINK_SPEED_2500       0x1
#define NGBE_PCI_LINK_SPEED_5000       0x2
#define NGBE_PCI_LINK_SPEED_8000       0x3
#define NGBE_PCI_HEADER_TYPE_REGISTER  0x0E
#define NGBE_PCI_HEADER_TYPE_MULTIFUNC 0x80
#define NGBE_PCI_DEVICE_CONTROL2_16ms  0x0005

#define NGBE_PCIDEVCTRL2_RELAX_ORDER_OFFSET    4
#define NGBE_PCIDEVCTRL2_RELAX_ORDER_MASK      \
				(0x0001 << NGBE_PCIDEVCTRL2_RELAX_ORDER_OFFSET)
#define NGBE_PCIDEVCTRL2_RELAX_ORDER_ENABLE    \
				(0x01 << NGBE_PCIDEVCTRL2_RELAX_ORDER_OFFSET)

#define NGBE_PCIDEVCTRL2_TIMEO_MASK    0xf
#define NGBE_PCIDEVCTRL2_16_32ms_def   0x0
#define NGBE_PCIDEVCTRL2_50_100us      0x1
#define NGBE_PCIDEVCTRL2_1_2ms         0x2
#define NGBE_PCIDEVCTRL2_16_32ms       0x5
#define NGBE_PCIDEVCTRL2_65_130ms      0x6
#define NGBE_PCIDEVCTRL2_260_520ms     0x9
#define NGBE_PCIDEVCTRL2_1_2s          0xa
#define NGBE_PCIDEVCTRL2_4_8s          0xd
#define NGBE_PCIDEVCTRL2_17_34s        0xe


/******************* Receive Descriptor bit definitions **********************/
#define NGBE_RXD_IPSEC_STATUS_SECP             0x00020000U
#define NGBE_RXD_IPSEC_ERROR_INVALID_PROTOCOL  0x08000000U
#define NGBE_RXD_IPSEC_ERROR_INVALID_LENGTH    0x10000000U
#define NGBE_RXD_IPSEC_ERROR_AUTH_FAILED       0x18000000U
#define NGBE_RXD_IPSEC_ERROR_BIT_MASK          0x18000000U

#define NGBE_RXD_NEXTP_MASK            0x000FFFF0U /* Next Descriptor Index */
#define NGBE_RXD_NEXTP_SHIFT           0x00000004U
#define NGBE_RXD_STAT_MASK             0x000fffffU /* Stat/NEXTP: bit 0-19 */
#define NGBE_RXD_STAT_DD               0x00000001U /* Done */
#define NGBE_RXD_STAT_EOP              0x00000002U /* End of Packet */
#define NGBE_RXD_STAT_CLASS_ID_MASK    0x0000001CU
#define NGBE_RXD_STAT_CLASS_ID_TC_RSS  0x00000000U
#define NGBE_RXD_STAT_CLASS_ID_SYN     0x00000008U
#define NGBE_RXD_STAT_CLASS_ID_5_TUPLE 0x0000000CU
#define NGBE_RXD_STAT_CLASS_ID_L2_ETYPE 0x00000010U
#define NGBE_RXD_STAT_VP               0x00000020U /* IEEE VLAN Pkt */
#define NGBE_RXD_STAT_UDPCS            0x00000040U /* UDP xsum calculated */
#define NGBE_RXD_STAT_L4CS             0x00000080U /* L4 xsum calculated */
#define NGBE_RXD_STAT_IPCS             0x00000100U /* IP xsum calculated */
#define NGBE_RXD_STAT_PIF              0x00000200U /* passed in-exact filter */
#define NGBE_RXD_STAT_OUTERIPCS        0x00000400U /* Cloud IP xsum calculated*/
#define NGBE_RXD_STAT_VEXT             0x00000800U /* 1st VLAN found */
#define NGBE_RXD_STAT_LLINT            0x00002000U /* Pkt caused Low Latency
													 * Int */
#define NGBE_RXD_STAT_TS               0x00004000U /* IEEE1588 Time Stamp */
#define NGBE_RXD_STAT_SECP             0x00008000U /* Security Processing */
#define NGBE_RXD_STAT_LB               0x00010000U /* Loopback Status */
#define NGBE_RXD_STAT_FCEOFS           0x00020000U /* FCoE EOF/SOF Stat */
#define NGBE_RXD_STAT_FCSTAT           0x000C0000U /* FCoE Pkt Stat */
#define NGBE_RXD_STAT_FCSTAT_NOMTCH    0x00000000U /* 00: No Ctxt Match */
#define NGBE_RXD_STAT_FCSTAT_NODDP     0x00040000U /* 01: Ctxt w/o DDP */
#define NGBE_RXD_STAT_FCSTAT_FCPRSP    0x00080000U /* 10: Recv. FCP_RSP */
#define NGBE_RXD_STAT_FCSTAT_DDP       0x000C0000U /* 11: Ctxt w/ DDP */

#define NGBE_RXD_ERR_MASK              0xfff00000U /* RDESC.ERRORS mask */
#define NGBE_RXD_ERR_SHIFT             20         /* RDESC.ERRORS shift */
#define NGBE_RXD_ERR_FCEOFE            0x80000000U /* FCEOFe/IPE */
#define NGBE_RXD_ERR_HBO               0x00800000U /*Header Buffer Overflow */
#define NGBE_RXD_ERR_OUTERIPER         0x04000000U /* CRC IP Header error */
#define NGBE_RXD_ERR_SECERR_MASK       0x18000000U
#define NGBE_RXD_ERR_RXE               0x20000000U /* Any MAC Error */
#define NGBE_RXD_ERR_TCPE              0x40000000U /* TCP/UDP Checksum Error */
#define NGBE_RXD_ERR_IPE               0x80000000U /* IP Checksum Error */

#define NGBE_RXDPS_HDRSTAT_HDRSP       0x00008000U
#define NGBE_RXDPS_HDRSTAT_HDRLEN_MASK 0x000003FFU

#define NGBE_RXD_RSSTYPE_MASK          0x0000000FU
#define NGBE_RXD_TPID_MASK             0x000001C0U
#define NGBE_RXD_TPID_SHIFT            6
#define NGBE_RXD_HDRBUFLEN_MASK        0x00007FE0U
#define NGBE_RXD_RSCCNT_MASK           0x001E0000U
#define NGBE_RXD_RSCCNT_SHIFT          17
#define NGBE_RXD_HDRBUFLEN_SHIFT       5
#define NGBE_RXD_SPLITHEADER_EN        0x00001000U
#define NGBE_RXD_SPH                   0x8000

/* RSS Hash results */
#define NGBE_RXD_RSSTYPE_NONE          0x00000000U
#define NGBE_RXD_RSSTYPE_IPV4_TCP      0x00000001U
#define NGBE_RXD_RSSTYPE_IPV4          0x00000002U
#define NGBE_RXD_RSSTYPE_IPV6_TCP      0x00000003U
#define NGBE_RXD_RSSTYPE_IPV4_SCTP     0x00000004U
#define NGBE_RXD_RSSTYPE_IPV6          0x00000005U
#define NGBE_RXD_RSSTYPE_IPV6_SCTP     0x00000006U
#define NGBE_RXD_RSSTYPE_IPV4_UDP      0x00000007U
#define NGBE_RXD_RSSTYPE_IPV6_UDP      0x00000008U

/**
 * receive packet type
 * PTYPE:8 = TUN:2 + PKT:2 + TYP:4
 **/
/* TUN */
#define NGBE_PTYPE_TUN_IPV4            (0x80)
#define NGBE_PTYPE_TUN_IPV6            (0xC0)

/* PKT for TUN */
#define NGBE_PTYPE_PKT_IPIP            (0x00) /* IP+IP */
#define NGBE_PTYPE_PKT_IG              (0x10) /* IP+GRE */
#define NGBE_PTYPE_PKT_IGM             (0x20) /* IP+GRE+MAC */
#define NGBE_PTYPE_PKT_IGMV            (0x30) /* IP+GRE+MAC+VLAN */
/* PKT for !TUN */
#define NGBE_PTYPE_PKT_MAC             (0x10)
#define NGBE_PTYPE_PKT_IP              (0x20)
#define NGBE_PTYPE_PKT_FCOE            (0x30)

/* TYP for PKT=mac */
#define NGBE_PTYPE_TYP_MAC             (0x01)
#define NGBE_PTYPE_TYP_TS              (0x02) /* time sync */
#define NGBE_PTYPE_TYP_FIP             (0x03)
#define NGBE_PTYPE_TYP_LLDP            (0x04)
#define NGBE_PTYPE_TYP_CNM             (0x05)
#define NGBE_PTYPE_TYP_EAPOL           (0x06)
#define NGBE_PTYPE_TYP_ARP             (0x07)
/* TYP for PKT=ip */
#define NGBE_PTYPE_PKT_IPV6            (0x08)
#define NGBE_PTYPE_TYP_IPFRAG          (0x01)
#define NGBE_PTYPE_TYP_IP              (0x02)
#define NGBE_PTYPE_TYP_UDP             (0x03)
#define NGBE_PTYPE_TYP_TCP             (0x04)
#define NGBE_PTYPE_TYP_SCTP            (0x05)
/* TYP for PKT=fcoe */
#define NGBE_PTYPE_PKT_VFT             (0x08)
#define NGBE_PTYPE_TYP_FCOE            (0x00)
#define NGBE_PTYPE_TYP_FCDATA          (0x01)
#define NGBE_PTYPE_TYP_FCRDY           (0x02)
#define NGBE_PTYPE_TYP_FCRSP           (0x03)
#define NGBE_PTYPE_TYP_FCOTHER         (0x04)

/* Packet type non-ip values */
enum ngbe_l2_ptypes {
	NGBE_PTYPE_L2_ABORTED = (NGBE_PTYPE_PKT_MAC),
	NGBE_PTYPE_L2_MAC = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_MAC),
	NGBE_PTYPE_L2_TS = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_TS),
	NGBE_PTYPE_L2_FIP = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_FIP),
	NGBE_PTYPE_L2_LLDP = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_LLDP),
	NGBE_PTYPE_L2_CNM = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_CNM),
	NGBE_PTYPE_L2_EAPOL = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_EAPOL),
	NGBE_PTYPE_L2_ARP = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_ARP),

	NGBE_PTYPE_L2_IPV4_FRAG = (NGBE_PTYPE_PKT_IP |
				    NGBE_PTYPE_TYP_IPFRAG),
	NGBE_PTYPE_L2_IPV4 = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_TYP_IP),
	NGBE_PTYPE_L2_IPV4_UDP = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_TYP_UDP),
	NGBE_PTYPE_L2_IPV4_TCP = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_TYP_TCP),
	NGBE_PTYPE_L2_IPV4_SCTP = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_TYP_SCTP),
	NGBE_PTYPE_L2_IPV6_FRAG = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_PKT_IPV6 |
				    NGBE_PTYPE_TYP_IPFRAG),
	NGBE_PTYPE_L2_IPV6 = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_PKT_IPV6 |
			       NGBE_PTYPE_TYP_IP),
	NGBE_PTYPE_L2_IPV6_UDP = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_PKT_IPV6 |
				   NGBE_PTYPE_TYP_UDP),
	NGBE_PTYPE_L2_IPV6_TCP = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_PKT_IPV6 |
				   NGBE_PTYPE_TYP_TCP),
	NGBE_PTYPE_L2_IPV6_SCTP = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_PKT_IPV6 |
				    NGBE_PTYPE_TYP_SCTP),

	NGBE_PTYPE_L2_FCOE = (NGBE_PTYPE_PKT_FCOE | NGBE_PTYPE_TYP_FCOE),
	NGBE_PTYPE_L2_FCOE_FCDATA = (NGBE_PTYPE_PKT_FCOE |
				      NGBE_PTYPE_TYP_FCDATA),
	NGBE_PTYPE_L2_FCOE_FCRDY = (NGBE_PTYPE_PKT_FCOE |
				     NGBE_PTYPE_TYP_FCRDY),
	NGBE_PTYPE_L2_FCOE_FCRSP = (NGBE_PTYPE_PKT_FCOE |
				     NGBE_PTYPE_TYP_FCRSP),
	NGBE_PTYPE_L2_FCOE_FCOTHER = (NGBE_PTYPE_PKT_FCOE |
				       NGBE_PTYPE_TYP_FCOTHER),
	NGBE_PTYPE_L2_FCOE_VFT = (NGBE_PTYPE_PKT_FCOE | NGBE_PTYPE_PKT_VFT),
	NGBE_PTYPE_L2_FCOE_VFT_FCDATA = (NGBE_PTYPE_PKT_FCOE |
				NGBE_PTYPE_PKT_VFT | NGBE_PTYPE_TYP_FCDATA),
	NGBE_PTYPE_L2_FCOE_VFT_FCRDY = (NGBE_PTYPE_PKT_FCOE |
				NGBE_PTYPE_PKT_VFT | NGBE_PTYPE_TYP_FCRDY),
	NGBE_PTYPE_L2_FCOE_VFT_FCRSP = (NGBE_PTYPE_PKT_FCOE |
				NGBE_PTYPE_PKT_VFT | NGBE_PTYPE_TYP_FCRSP),
	NGBE_PTYPE_L2_FCOE_VFT_FCOTHER = (NGBE_PTYPE_PKT_FCOE |
				NGBE_PTYPE_PKT_VFT | NGBE_PTYPE_TYP_FCOTHER),

	NGBE_PTYPE_L2_TUN4_MAC = (NGBE_PTYPE_TUN_IPV4 | NGBE_PTYPE_PKT_IGM),
	NGBE_PTYPE_L2_TUN6_MAC = (NGBE_PTYPE_TUN_IPV6 | NGBE_PTYPE_PKT_IGM),
};

#define NGBE_RXD_PKTTYPE(_rxd) \
	((le32_to_cpu((_rxd)->wb.lower.lo_dword.data) >> 9) & 0xFF)
#define NGBE_PTYPE_TUN(_pt) ((_pt) & 0xC0)
#define NGBE_PTYPE_PKT(_pt) ((_pt) & 0x30)
#define NGBE_PTYPE_TYP(_pt) ((_pt) & 0x0F)
#define NGBE_PTYPE_TYPL4(_pt) ((_pt) & 0x07)

#define NGBE_RXD_IPV6EX(_rxd) \
	((le32_to_cpu((_rxd)->wb.lower.lo_dword.data) >> 6) & 0x1)

/* Security Processing bit Indication */
#define NGBE_RXD_LNKSEC_STATUS_SECP            0x00020000U
#define NGBE_RXD_LNKSEC_ERROR_NO_SA_MATCH      0x08000000U
#define NGBE_RXD_LNKSEC_ERROR_REPLAY_ERROR     0x10000000U
#define NGBE_RXD_LNKSEC_ERROR_BIT_MASK         0x18000000U
#define NGBE_RXD_LNKSEC_ERROR_BAD_SIG          0x18000000U

/* Masks to determine if packets should be dropped due to frame errors */
#define NGBE_RXD_ERR_FRAME_ERR_MASK    NGBE_RXD_ERR_RXE

/*********************** Adv Transmit Descriptor Config Masks ****************/
#define NGBE_TXD_DTALEN_MASK           0x0000FFFFU /* Data buf length(bytes) */
#define NGBE_TXD_MAC_LINKSEC           0x00040000U /* Insert LinkSec */
#define NGBE_TXD_MAC_TSTAMP            0x00080000U /* IEEE1588 time stamp */
#define NGBE_TXD_IPSEC_SA_INDEX_MASK   0x000003FFU /* IPSec SA index */
#define NGBE_TXD_IPSEC_ESP_LEN_MASK    0x000001FFU /* IPSec ESP length */
#define NGBE_TXD_DTYP_MASK             0x00F00000U /* DTYP mask */
#define NGBE_TXD_DTYP_CTXT             0x00100000U /* Adv Context Desc */
#define NGBE_TXD_DTYP_DATA             0x00000000U /* Adv Data Descriptor */
#define NGBE_TXD_EOP                   0x01000000U  /* End of Packet */
#define NGBE_TXD_IFCS                  0x02000000U /* Insert FCS */
#define NGBE_TXD_LINKSEC               0x04000000U /* enable linksec */
#define NGBE_TXD_RS                    0x08000000U /* Report Status */
#define NGBE_TXD_ECU                   0x10000000U /* DDP hdr type or iSCSI */
#define NGBE_TXD_QCN                   0x20000000U /* cntag insertion enable */
#define NGBE_TXD_VLE                   0x40000000U /* VLAN pkt enable */
#define NGBE_TXD_TSE                   0x80000000U /* TCP Seg enable */
#define NGBE_TXD_STAT_DD               0x00000001U /* Descriptor Done */
#define NGBE_TXD_IDX_SHIFT             4 /* Adv desc Index shift */
#define NGBE_TXD_CC                    0x00000080U /* Check Context */
#define NGBE_TXD_IPSEC                 0x00000100U /* enable ipsec esp */
#define NGBE_TXD_IIPCS                 0x00000400U
#define NGBE_TXD_EIPCS                 0x00000800U
#define NGBE_TXD_L4CS                  0x00000200U
#define NGBE_TXD_PAYLEN_SHIFT          13 /* Adv desc PAYLEN shift */
#define NGBE_TXD_MACLEN_SHIFT          9  /* Adv ctxt desc mac len shift */
#define NGBE_TXD_VLAN_SHIFT            16  /* Adv ctxt vlan tag shift */
#define NGBE_TXD_TAG_TPID_SEL_SHIFT    11
#define NGBE_TXD_IPSEC_TYPE_SHIFT      14
#define NGBE_TXD_ENC_SHIFT             15

#define NGBE_TXD_TUCMD_IPSEC_TYPE_ESP  0x00004000U /* IPSec Type ESP */
#define NGBE_TXD_TUCMD_IPSEC_ENCRYPT_EN 0x00008000/* ESP Encrypt Enable */
#define NGBE_TXD_TUCMD_FCOE            0x00010000U /* FCoE Frame Type */
#define NGBE_TXD_FCOEF_EOF_MASK        (0x3 << 10) /* FC EOF index */
#define NGBE_TXD_FCOEF_SOF             ((1 << 2) << 10) /* FC SOF index */
#define NGBE_TXD_FCOEF_PARINC          ((1 << 3) << 10) /* Rel_Off in F_CTL */
#define NGBE_TXD_FCOEF_ORIE            ((1 << 4) << 10) /* Orientation End */
#define NGBE_TXD_FCOEF_ORIS            ((1 << 5) << 10) /* Orientation Start */
#define NGBE_TXD_FCOEF_EOF_N           (0x0 << 10) /* 00: EOFn */
#define NGBE_TXD_FCOEF_EOF_T           (0x1 << 10) /* 01: EOFt */
#define NGBE_TXD_FCOEF_EOF_NI          (0x2 << 10) /* 10: EOFni */
#define NGBE_TXD_FCOEF_EOF_A           (0x3 << 10) /* 11: EOFa */
#define NGBE_TXD_L4LEN_SHIFT           8  /* Adv ctxt L4LEN shift */
#define NGBE_TXD_MSS_SHIFT             16  /* Adv ctxt MSS shift */

#define NGBE_TXD_OUTER_IPLEN_SHIFT     12 /* Adv ctxt OUTERIPLEN shift */
#define NGBE_TXD_TUNNEL_LEN_SHIFT      21 /* Adv ctxt TUNNELLEN shift */
#define NGBE_TXD_TUNNEL_TYPE_SHIFT     11 /* Adv Tx Desc Tunnel Type shift */
#define NGBE_TXD_TUNNEL_DECTTL_SHIFT   27 /* Adv ctxt DECTTL shift */
#define NGBE_TXD_TUNNEL_UDP            (0x0ULL << NGBE_TXD_TUNNEL_TYPE_SHIFT)
#define NGBE_TXD_TUNNEL_GRE            (0x1ULL << NGBE_TXD_TUNNEL_TYPE_SHIFT)


/************ ngbe_type.h ************/
/* Number of Transmit and Receive Descriptors must be a multiple of 8 */
#define NGBE_REQ_TX_DESCRIPTOR_MULTIPLE        8
#define NGBE_REQ_RX_DESCRIPTOR_MULTIPLE        8
#define NGBE_REQ_TX_BUFFER_GRANULARITY         1024

/* Vlan-specific macros */
#define NGBE_RX_DESC_SPECIAL_VLAN_MASK 0x0FFF /* VLAN ID in lower 12 bits */
#define NGBE_RX_DESC_SPECIAL_PRI_MASK  0xE000 /* Priority in upper 3 bits */
#define NGBE_RX_DESC_SPECIAL_PRI_SHIFT 0x000D /* Priority in upper 3 of 16 */
#define NGBE_TX_DESC_SPECIAL_PRI_SHIFT NGBE_RX_DESC_SPECIAL_PRI_SHIFT

/* Transmit Descriptor */
union ngbe_tx_desc {
	struct {
		__le64 buffer_addr; /* Address of descriptor's data buf */
		__le32 cmd_type_len;
		__le32 olinfo_status;
	} read;
	struct {
		__le64 rsvd; /* Reserved */
		__le32 nxtseq_seed;
		__le32 status;
	} wb;
};

/* Receive Descriptor */
union ngbe_rx_desc {
	struct {
		__le64 pkt_addr; /* Packet buffer address */
		__le64 hdr_addr; /* Header buffer address */
	} read;
	struct {
		struct {
			union {
				__le32 data;
				struct {
					__le16 pkt_info; /* RSS, Pkt type */
					__le16 hdr_info; /* Splithdr, hdrlen */
				} hs_rss;
			} lo_dword;
			union {
				__le32 rss; /* RSS Hash */
				struct {
					__le16 ip_id; /* IP id */
					__le16 csum; /* Packet Checksum */
				} csum_ip;
			} hi_dword;
		} lower;
		struct {
			__le32 status_error; /* ext status/error */
			__le16 length; /* Packet length */
			__le16 vlan; /* VLAN tag */
		} upper;
	} wb;  /* writeback */
};

/* Context descriptors */
struct ngbe_tx_context_desc {
	__le32 vlan_macip_lens;
	__le32 seqnum_seed;
	__le32 type_tucmd_mlhl;
	__le32 mss_l4len_idx;
};

/************************* Flow Directory HASH *******************************/
/* Software ATR hash keys */
#define NGBE_ATR_BUCKET_HASH_KEY       0x3DAD14E2
#define NGBE_ATR_SIGNATURE_HASH_KEY    0x174D3614

/* Software ATR input stream values and masks */
#define NGBE_ATR_HASH_MASK             0x7fff
#define NGBE_ATR_L4TYPE_MASK           0x3
#define NGBE_ATR_L4TYPE_UDP            0x1
#define NGBE_ATR_L4TYPE_TCP            0x2
#define NGBE_ATR_L4TYPE_SCTP           0x3
#define NGBE_ATR_L4TYPE_IPV6_MASK      0x4
#define NGBE_ATR_L4TYPE_TUNNEL_MASK    0x10
enum ngbe_atr_flow_type {
	NGBE_ATR_FLOW_TYPE_IPV4        = 0x0,
	NGBE_ATR_FLOW_TYPE_UDPV4       = 0x1,
	NGBE_ATR_FLOW_TYPE_TCPV4       = 0x2,
	NGBE_ATR_FLOW_TYPE_SCTPV4      = 0x3,
	NGBE_ATR_FLOW_TYPE_IPV6        = 0x4,
	NGBE_ATR_FLOW_TYPE_UDPV6       = 0x5,
	NGBE_ATR_FLOW_TYPE_TCPV6       = 0x6,
	NGBE_ATR_FLOW_TYPE_SCTPV6      = 0x7,
	NGBE_ATR_FLOW_TYPE_TUNNELED_IPV4       = 0x10,
	NGBE_ATR_FLOW_TYPE_TUNNELED_UDPV4      = 0x11,
	NGBE_ATR_FLOW_TYPE_TUNNELED_TCPV4      = 0x12,
	NGBE_ATR_FLOW_TYPE_TUNNELED_SCTPV4     = 0x13,
	NGBE_ATR_FLOW_TYPE_TUNNELED_IPV6       = 0x14,
	NGBE_ATR_FLOW_TYPE_TUNNELED_UDPV6      = 0x15,
	NGBE_ATR_FLOW_TYPE_TUNNELED_TCPV6      = 0x16,
	NGBE_ATR_FLOW_TYPE_TUNNELED_SCTPV6     = 0x17,
};

/* Flow Director ATR input struct. */
union ngbe_atr_input {
	/*
	 * Byte layout in order, all values with MSB first:
	 *
	 * vm_pool      - 1 byte
	 * flow_type    - 1 byte
	 * vlan_id      - 2 bytes
	 * src_ip       - 16 bytes
	 * inner_mac    - 6 bytes
	 * cloud_mode   - 2 bytes
	 * tni_vni      - 4 bytes
	 * dst_ip       - 16 bytes
	 * src_port     - 2 bytes
	 * dst_port     - 2 bytes
	 * flex_bytes   - 2 bytes
	 * bkt_hash     - 2 bytes
	 */
	struct {
		u8 vm_pool;
		u8 flow_type;
		__be16 vlan_id;
		__be32 dst_ip[4];
		__be32 src_ip[4];
		__be16 src_port;
		__be16 dst_port;
		__be16 flex_bytes;
		__be16 bkt_hash;
	} formatted;
	__be32 dword_stream[11];
};

/* Flow Director compressed ATR hash input struct */
union ngbe_atr_hash_dword {
	struct {
		u8 vm_pool;
		u8 flow_type;
		__be16 vlan_id;
	} formatted;
	__be32 ip;
	struct {
		__be16 src;
		__be16 dst;
	} port;
	__be16 flex_bytes;
	__be32 dword;
};


/****************** Manageablility Host Interface defines ********************/
#define NGBE_HI_MAX_BLOCK_BYTE_LENGTH  256 /* Num of bytes in range */
#define NGBE_HI_MAX_BLOCK_DWORD_LENGTH 64 /* Num of dwords in range */
#define NGBE_HI_COMMAND_TIMEOUT        5000 /* Process HI command limit */
#define NGBE_HI_FLASH_ERASE_TIMEOUT    5000 /* Process Erase command limit */
#define NGBE_HI_FLASH_UPDATE_TIMEOUT   5000 /* Process Update command limit */
#define NGBE_HI_FLASH_VERIFY_TIMEOUT   60000 /* Process Apply command limit */
#define NGBE_HI_PHY_MGMT_REQ_TIMEOUT   2000 /* Wait up to 2 seconds */

/* CEM Support */
#define FW_CEM_HDR_LEN                  0x4
#define FW_CEM_CMD_DRIVER_INFO          0xDD
#define FW_CEM_CMD_DRIVER_INFO_LEN      0x5
#define FW_CEM_CMD_RESERVED             0X0
#define FW_CEM_UNUSED_VER               0x0
#define FW_CEM_MAX_RETRIES              3
#define FW_CEM_RESP_STATUS_SUCCESS      0x1
#define FW_READ_SHADOW_RAM_CMD          0x31
#define FW_READ_SHADOW_RAM_LEN          0x6
#define FW_WRITE_SHADOW_RAM_CMD         0x33
#define FW_WRITE_SHADOW_RAM_LEN         0xA /* 8 plus 1 WORD to write */
#define FW_SHADOW_RAM_DUMP_CMD          0x36
#define FW_SHADOW_RAM_DUMP_LEN          0
#define FW_DEFAULT_CHECKSUM             0xFF /* checksum always 0xFF */
#define FW_NVM_DATA_OFFSET              3
#define FW_MAX_READ_BUFFER_SIZE         244
#define FW_DISABLE_RXEN_CMD             0xDE
#define FW_DISABLE_RXEN_LEN             0x1
#define FW_PHY_MGMT_REQ_CMD             0x20
#define FW_RESET_CMD                    0xDF
#define FW_RESET_LEN                    0x2
#define FW_SETUP_MAC_LINK_CMD           0xE0
#define FW_SETUP_MAC_LINK_LEN           0x2
#define FW_FLASH_UPGRADE_START_CMD      0xE3
#define FW_FLASH_UPGRADE_START_LEN      0x1
#define FW_FLASH_UPGRADE_WRITE_CMD      0xE4
#define FW_FLASH_UPGRADE_VERIFY_CMD     0xE5
#define FW_FLASH_UPGRADE_VERIFY_LEN     0x4
#define FW_EEPROM_CHECK_STATUS 			0xE9
#define FW_PHY_LED_CONF			0xF1
#define FW_PHY_SIGNAL					0xF0


/* Host Interface Command Structures */
struct ngbe_hic_hdr {
	u8 cmd;
	u8 buf_len;
	union {
		u8 cmd_resv;
		u8 ret_status;
	} cmd_or_resp;
	u8 checksum;
};

struct ngbe_hic_hdr2_req {
	u8 cmd;
	u8 buf_lenh;
	u8 buf_lenl;
	u8 checksum;
};

struct ngbe_hic_hdr2_rsp {
	u8 cmd;
	u8 buf_lenl;
	u8 buf_lenh_status;     /* 7-5: high bits of buf_len, 4-0: status */
	u8 checksum;
};

union ngbe_hic_hdr2 {
	struct ngbe_hic_hdr2_req req;
	struct ngbe_hic_hdr2_rsp rsp;
};

struct ngbe_hic_drv_info {
	struct ngbe_hic_hdr hdr;
	u8 port_num;
	u8 ver_sub;
	u8 ver_build;
	u8 ver_min;
	u8 ver_maj;
	u8 pad; /* end spacing to ensure length is mult. of dword */
	u16 pad2; /* end spacing to ensure length is mult. of dword2 */
};

/* These need to be dword aligned */
struct ngbe_hic_read_shadow_ram {
	union ngbe_hic_hdr2 hdr;
	u32 address;
	u16 length;
	u16 pad2;
	u16 data;
	u16 pad3;
};

struct ngbe_hic_write_shadow_ram {
	union ngbe_hic_hdr2 hdr;
	u32 address;
	u16 length;
	u16 pad2;
	u16 data;
	u16 pad3;
};

struct ngbe_hic_disable_rxen {
	struct ngbe_hic_hdr hdr;
	u8  port_number;
	u8  pad2;
	u16 pad3;
};

struct ngbe_hic_reset {
	struct ngbe_hic_hdr hdr;
	u16 lan_id;
	u16 reset_type;
};

struct ngbe_hic_phy_cfg {
	struct ngbe_hic_hdr hdr;
	u8 lan_id;
	u8 phy_mode;
	u16 phy_speed;
};

enum ngbe_module_id {
	NGBE_MODULE_EEPROM = 0,
	NGBE_MODULE_FIRMWARE,
	NGBE_MODULE_HARDWARE,
	NGBE_MODULE_PCIE
};

struct ngbe_hic_upg_start {
	struct ngbe_hic_hdr hdr;
	u8 module_id;
	u8  pad2;
	u16 pad3;
};

struct ngbe_hic_upg_write {
	struct ngbe_hic_hdr hdr;
	u8 data_len;
	u8 eof_flag;
	u16 check_sum;
	u32 data[62];
};

enum ngbe_upg_flag {
	NGBE_RESET_NONE = 0,
	NGBE_RESET_FIRMWARE,
	NGBE_RELOAD_EEPROM,
	NGBE_RESET_LAN
};

struct ngbe_hic_upg_verify {
	struct ngbe_hic_hdr hdr;
	u32 action_flag;
};

/* Number of 100 microseconds we wait for PCI Express master disable */
#define NGBE_PCI_MASTER_DISABLE_TIMEOUT        800

/* Check whether address is multicast. This is little-endian specific check.*/
#define NGBE_IS_MULTICAST(Address) \
		(bool)(((u8 *)(Address))[0] & ((u8)0x01))

/* Check whether an address is broadcast. */
#define NGBE_IS_BROADCAST(Address) \
		((((u8 *)(Address))[0] == ((u8)0xff)) && \
		(((u8 *)(Address))[1] == ((u8)0xff)))

/* DCB registers */
#define NGBE_DCB_MAX_TRAFFIC_CLASS     8

/* Power Manangbeent */
/* DMA Coalescing configuration */
struct ngbe_dmac_config {
	u16     watchdog_timer; /* usec units */
	bool    fcoe_en;
	u32     link_speed;
	u8      fcoe_tc;
	u8      num_tcs;
};


/* Autonegotiation advertised speeds */
typedef u32 ngbe_autoneg_advertised;
/* Link speed */
#define NGBE_LINK_SPEED_UNKNOWN        0
#define NGBE_LINK_SPEED_100_FULL       1
#define NGBE_LINK_SPEED_1GB_FULL       2
#define NGBE_LINK_SPEED_10_FULL        8
#define NGBE_LINK_SPEED_AUTONEG  (NGBE_LINK_SPEED_100_FULL | \
				   NGBE_LINK_SPEED_1GB_FULL | \
				   NGBE_LINK_SPEED_10_FULL)

/* Physical layer type */
typedef u32 ngbe_physical_layer;
#define NGBE_PHYSICAL_LAYER_UNKNOWN            0
#define NGBE_PHYSICAL_LAYER_1000BASE_T         0x0002
#define NGBE_PHYSICAL_LAYER_100BASE_TX         0x0004
#define NGBE_PHYSICAL_LAYER_SFP_PLUS_CU        0x0008
#define NGBE_PHYSICAL_LAYER_1000BASE_KX        0x0200
#define NGBE_PHYSICAL_LAYER_1000BASE_BX        0x0400
#define NGBE_PHYSICAL_LAYER_SFP_ACTIVE_DA      0x2000
#define NGBE_PHYSICAL_LAYER_1000BASE_SX        0x4000


/* Special PHY Init Routine */
#define NGBE_PHY_INIT_OFFSET_NL        0x002B
#define NGBE_PHY_INIT_END_NL           0xFFFF
#define NGBE_CONTROL_MASK_NL           0xF000
#define NGBE_DATA_MASK_NL              0x0FFF
#define NGBE_CONTROL_SHIFT_NL          12
#define NGBE_DELAY_NL                  0
#define NGBE_DATA_NL                   1
#define NGBE_CONTROL_NL                0x000F
#define NGBE_CONTROL_EOL_NL            0x0FFF
#define NGBE_CONTROL_SOL_NL            0x0000

/* ethtool */
#define SPEED_10        10
#define SPEED_100       100
#define SPEED_1000      1000


/* Flow Control Data Sheet defined values
 * Calculation and defines taken from 802.1bb Annex O
 */

/* BitTimes (BT) conversion */
#define NGBE_BT2KB(BT)         ((BT + (8 * 1024 - 1)) / (8 * 1024))
#define NGBE_B2BT(BT)          (BT * 8)

/* Calculate Delay to respond to PFC */
#define NGBE_PFC_D     672

/* Calculate Cable Delay */
#define NGBE_CABLE_DC  5556 /* Delay Copper */
#define NGBE_CABLE_DO  5000 /* Delay Optical */

/* Calculate Interface Delay X540 */
#define NGBE_PHY_DC    25600 /* Delay 10G BASET */
#define NGBE_MAC_DC    8192  /* Delay Copper XAUI interface */
#define NGBE_XAUI_DC   (2 * 2048) /* Delay Copper Phy */

#define NGBE_ID_X540   (NGBE_MAC_DC + NGBE_XAUI_DC + NGBE_PHY_DC)

/* Calculate Interface Delay */
#define NGBE_PHY_D     12800
#define NGBE_MAC_D     4096
#define NGBE_XAUI_D    (2 * 1024)

#define NGBE_ID        (NGBE_MAC_D + NGBE_XAUI_D + NGBE_PHY_D)

/* Calculate Delay incurred from higher layer */
#define NGBE_HD        6144

/* Calculate PCI Bus delay for low thresholds */
#define NGBE_PCI_DELAY 10000

/* Calculate X540 delay value in bit times */
#define NGBE_DV_X540(_max_frame_link, _max_frame_tc) \
			((36 * \
			  (NGBE_B2BT(_max_frame_link) + \
			   NGBE_PFC_D + \
			   (2 * NGBE_CABLE_DC) + \
			   (2 * NGBE_ID_X540) + \
			   NGBE_HD) / 25 + 1) + \
			 2 * NGBE_B2BT(_max_frame_tc))


/* Calculate delay value in bit times */
#define NGBE_DV(_max_frame_link, _max_frame_tc) \
			((36 * \
			  (NGBE_B2BT(_max_frame_link) + \
			   NGBE_PFC_D + \
			   (2 * NGBE_CABLE_DC) + \
			   (2 * NGBE_ID) + \
			   NGBE_HD) / 25 + 1) + \
			 2 * NGBE_B2BT(_max_frame_tc))

/* Calculate low threshold delay values */
#define NGBE_LOW_DV_X540(_max_frame_tc) \
			(2 * NGBE_B2BT(_max_frame_tc) + \
			(36 * NGBE_PCI_DELAY / 25) + 1)

#define NGBE_LOW_DV(_max_frame_tc) \
			(2 * NGBE_LOW_DV_X540(_max_frame_tc))


/*
 * Unavailable: The FCoE Boot Option ROM is not present in the flash.
 * Disabled: Present; boot order is not set for any targets on the port.
 * Enabled: Present; boot order is set for at least one target on the port.
 */
enum ngbe_fcoe_boot_status {
	ngbe_fcoe_bootstatus_disabled = 0,
	ngbe_fcoe_bootstatus_enabled = 1,
	ngbe_fcoe_bootstatus_unavailable = 0xFFFF
};

enum ngbe_eeprom_type {
	ngbe_eeprom_uninitialized = 0,
	ngbe_eeprom_spi,
	ngbe_flash,
	ngbe_eeprom_none /* No NVM support */
};

enum ngbe_phy_type {
	ngbe_phy_unknown = 0,
	ngbe_phy_none,
	ngbe_phy_internal,
	ngbe_phy_m88e1512,
	ngbe_phy_m88e1512_sfi,
	ngbe_phy_m88e1512_unknown,
	ngbe_phy_yt8521s,
	ngbe_phy_yt8521s_sfi,
	ngbe_phy_zte,
	ngbe_phy_sfp_passive_tyco,
	ngbe_phy_sfp_passive_unknown,
	ngbe_phy_sfp_active_unknown,
	ngbe_phy_sfp_avago,
	ngbe_phy_sfp_ftl,
	ngbe_phy_sfp_ftl_active,
	ngbe_phy_sfp_unknown,
	ngbe_phy_sfp_intel,
	ngbe_phy_sfp_unsupported, /*Enforce bit set with unsupported module*/
	ngbe_phy_internal_yt8521s_sfi,
	ngbe_phy_generic
};

/*
 * SFP+ module type IDs:
 *
 * ID   Module Type
 * =============
 * 0    SFP_DA_CU
 * 1    SFP_SR
 * 2    SFP_LR
 * 3    SFP_DA_CU_CORE0
 * 4    SFP_DA_CU_CORE1
 * 5    SFP_SR/LR_CORE0
 * 6    SFP_SR/LR_CORE1
 */
enum ngbe_sfp_type {
	ngbe_sfp_type_da_cu = 0,
	ngbe_sfp_type_sr = 1,
	ngbe_sfp_type_lr = 2,
	ngbe_sfp_type_da_cu_core0 = 3,
	ngbe_sfp_type_da_cu_core1 = 4,
	ngbe_sfp_type_srlr_core0 = 5,
	ngbe_sfp_type_srlr_core1 = 6,
	ngbe_sfp_type_da_act_lmt_core0 = 7,
	ngbe_sfp_type_da_act_lmt_core1 = 8,
	ngbe_sfp_type_1g_cu_core0 = 9,
	ngbe_sfp_type_1g_cu_core1 = 10,
	ngbe_sfp_type_1g_sx_core0 = 11,
	ngbe_sfp_type_1g_sx_core1 = 12,
	ngbe_sfp_type_1g_lx_core0 = 13,
	ngbe_sfp_type_1g_lx_core1 = 14,
	ngbe_sfp_type_not_present = 0xFFFE,
	ngbe_sfp_type_unknown = 0xFFFF
};

enum ngbe_media_type {
	ngbe_media_type_unknown = 0,
	ngbe_media_type_fiber,
	ngbe_media_type_copper,
	ngbe_media_type_backplane,
	ngbe_media_type_virtual
};

/* Flow Control Settings */
enum ngbe_fc_mode {
	ngbe_fc_none = 0,
	ngbe_fc_rx_pause,
	ngbe_fc_tx_pause,
	ngbe_fc_full,
	ngbe_fc_default
};

/* Smart Speed Settings */
#define NGBE_SMARTSPEED_MAX_RETRIES    3
enum ngbe_smart_speed {
	ngbe_smart_speed_auto = 0,
	ngbe_smart_speed_on,
	ngbe_smart_speed_off
};

/* PCI bus types */
enum ngbe_bus_type {
	ngbe_bus_type_unknown = 0,
	ngbe_bus_type_pci,
	ngbe_bus_type_pcix,
	ngbe_bus_type_pci_express,
	ngbe_bus_type_internal,
	ngbe_bus_type_reserved
};

/* PCI bus speeds */
enum ngbe_bus_speed {
	ngbe_bus_speed_unknown	= 0,
	ngbe_bus_speed_33	= 33,
	ngbe_bus_speed_66	= 66,
	ngbe_bus_speed_100	= 100,
	ngbe_bus_speed_120	= 120,
	ngbe_bus_speed_133	= 133,
	ngbe_bus_speed_2500	= 2500,
	ngbe_bus_speed_5000	= 5000,
	ngbe_bus_speed_8000	= 8000,
	ngbe_bus_speed_reserved
};

/* PCI bus widths */
enum ngbe_bus_width {
	ngbe_bus_width_unknown	= 0,
	ngbe_bus_width_pcie_x1	= 1,
	ngbe_bus_width_pcie_x2	= 2,
	ngbe_bus_width_pcie_x4	= 4,
	ngbe_bus_width_pcie_x8	= 8,
	ngbe_bus_width_32	= 32,
	ngbe_bus_width_64	= 64,
	ngbe_bus_width_reserved
};

struct ngbe_addr_filter_info {
	u32 num_mc_addrs;
	u32 rar_used_count;
	u32 mta_in_use;
	u32 overflow_promisc;
	bool user_set_promisc;
};

/* Bus parameters */
struct ngbe_bus_info {
	enum pci_bus_speed speed;
	enum pcie_link_width width;
	enum ngbe_bus_type type;

	u16 func;
	u16 lan_id;
};

/* Flow control parameters */
struct ngbe_fc_info {
	u32 high_water; /* Flow Ctrl High-water */
	u32 low_water; /* Flow Ctrl Low-water */
	u16 pause_time; /* Flow Control Pause timer */
	bool send_xon; /* Flow control send XON */
	bool strict_ieee; /* Strict IEEE mode */
	bool disable_fc_autoneg; /* Do not autonegotiate FC */
	bool fc_was_autonegged; /* Is current_mode the result of autonegging? */
	enum ngbe_fc_mode current_mode; /* FC mode in effect */
	enum ngbe_fc_mode requested_mode; /* FC mode requested by caller */
};

/* Statistics counters collected by the MAC */
struct ngbe_hw_stats {
	u64 crcerrs;
	u64 illerrc;
	u64 errbc;
	u64 mspdc;
	u64 mpctotal;
	u64 mpc[8];
	u64 mlfc;
	u64 mrfc;
	u64 rlec;
	u64 lxontxc;
	u64 lxonrxc;
	u64 lxofftxc;
	u64 lxoffrxc;
	u64 pxontxc[8];
	u64 pxonrxc[8];
	u64 pxofftxc[8];
	u64 pxoffrxc[8];
	u64 prc64;
	u64 prc127;
	u64 prc255;
	u64 prc511;
	u64 prc1023;
	u64 prc1522;
	u64 gprc;
	u64 bprc;
	u64 mprc;
	u64 gptc;
	u64 gorc;
	u64 gotc;
	u64 rnbc[8];
	u64 ruc;
	u64 rfc;
	u64 roc;
	u64 rjc;
	u64 mngprc;
	u64 mngpdc;
	u64 mngptc;
	u64 tor;
	u64 tpr;
	u64 tpt;
	u64 ptc64;
	u64 ptc127;
	u64 ptc255;
	u64 ptc511;
	u64 ptc1023;
	u64 ptc1522;
	u64 mptc;
	u64 bptc;
	u64 xec;
	u64 qprc[16];
	u64 qptc[16];
	u64 qbrc[16];
	u64 qbtc[16];
	u64 qprdc[16];
	u64 pxon2offc[8];
	u64 fccrc;
	u64 fclast;
	u64 fcoerpdc;
	u64 fcoeprc;
	u64 fcoeptc;
	u64 fcoedwrc;
	u64 fcoedwtc;
	u64 fcoe_noddp;
	u64 fcoe_noddp_ext_buff;
	u64 ldpcec;
	u64 pcrc8ec;
	u64 b2ospc;
	u64 b2ogprc;
	u64 o2bgptc;
	u64 o2bspc;
};

/* forward declaration */
struct ngbe_hw;

/* iterator type for walking multicast address lists */
typedef u8* (*ngbe_mc_addr_itr) (struct ngbe_hw *hw, u8 **mc_addr_ptr,
				  u32 *vmdq);

/* Function pointer table */
struct ngbe_eeprom_operations {
	s32 (*init_params)(struct ngbe_hw *);
	s32 (*read)(struct ngbe_hw *, u16, u16 *);
	s32 (*read_buffer)(struct ngbe_hw *, u16, u16, u16 *);
	s32 (*read32)(struct ngbe_hw *, u16, u32 *);
	s32 (*write)(struct ngbe_hw *, u16, u16);
	s32 (*write_buffer)(struct ngbe_hw *, u16, u16, u16 *);
	s32 (*validate_checksum)(struct ngbe_hw *, u16 *);
	s32 (*update_checksum)(struct ngbe_hw *);
	s32 (*calc_checksum)(struct ngbe_hw *);
	s32 (*eeprom_chksum_cap_st)(struct ngbe_hw *, u16, u32 *);
	s32 (*phy_led_oem_chk)(struct ngbe_hw *, u32 *);
	s32 (*phy_signal_set)(struct ngbe_hw *);
};

struct ngbe_flash_operations {
	s32 (*init_params)(struct ngbe_hw *);
	s32 (*read_buffer)(struct ngbe_hw *, u32, u32, u32 *);
	s32 (*write_buffer)(struct ngbe_hw *, u32, u32, u32 *);
	s32 (*check_led_oem)(struct ngbe_hw *);
};

struct ngbe_mac_operations {
	s32 (*init_hw)(struct ngbe_hw *);
	s32 (*reset_hw)(struct ngbe_hw *);
	s32 (*start_hw)(struct ngbe_hw *);
	s32 (*clear_hw_cntrs)(struct ngbe_hw *);
	enum ngbe_media_type (*get_media_type)(struct ngbe_hw *);
	s32 (*get_mac_addr)(struct ngbe_hw *, u8 *);
	s32 (*get_device_caps)(struct ngbe_hw *, u16 *);
	s32 (*stop_adapter)(struct ngbe_hw *);
	s32 (*get_bus_info)(struct ngbe_hw *);
	void (*set_lan_id)(struct ngbe_hw *);
	s32 (*enable_rx_dma)(struct ngbe_hw *, u32);
	s32 (*disable_sec_rx_path)(struct ngbe_hw *);
	s32 (*enable_sec_rx_path)(struct ngbe_hw *);
	s32 (*acquire_swfw_sync)(struct ngbe_hw *, u32);
	void (*release_swfw_sync)(struct ngbe_hw *, u32);

	/* Link */
	void (*disable_tx_laser)(struct ngbe_hw *);
	void (*enable_tx_laser)(struct ngbe_hw *);
	void (*flap_tx_laser)(struct ngbe_hw *);
	s32 (*setup_link)(struct ngbe_hw *, u32, bool);
	s32 (*setup_mac_link)(struct ngbe_hw *, u32, bool);
	s32 (*check_link)(struct ngbe_hw *, u32 *, bool *, bool);
	s32 (*get_link_capabilities)(struct ngbe_hw *, u32 *,
				     bool *);
	void (*set_rate_select_speed)(struct ngbe_hw *, u32);

	/* Packet Buffer manipulation */
	void (*setup_rxpba)(struct ngbe_hw *, int, u32, int);

	/* LED */
	s32 (*led_on)(struct ngbe_hw *, u32);
	s32 (*led_off)(struct ngbe_hw *, u32);

	/* RAR, Multicast, VLAN */
	s32 (*set_rar)(struct ngbe_hw *, u32, u8 *, u64, u32);
	s32 (*clear_rar)(struct ngbe_hw *, u32);
	s32 (*insert_mac_addr)(struct ngbe_hw *, u8 *, u32);
	s32 (*set_vmdq)(struct ngbe_hw *, u32, u32);
	s32 (*set_vmdq_san_mac)(struct ngbe_hw *, u32);
	s32 (*clear_vmdq)(struct ngbe_hw *, u32, u32);
	s32 (*init_rx_addrs)(struct ngbe_hw *);
	s32 (*update_uc_addr_list)(struct ngbe_hw *, u8 *, u32,
				   ngbe_mc_addr_itr);
	s32 (*update_mc_addr_list)(struct ngbe_hw *, u8 *, u32,
				   ngbe_mc_addr_itr, bool clear);
	s32 (*enable_mc)(struct ngbe_hw *);
	s32 (*disable_mc)(struct ngbe_hw *);
	s32 (*clear_vfta)(struct ngbe_hw *);
	s32 (*set_vfta)(struct ngbe_hw *, u32, u32, bool);
	s32 (*set_vlvf)(struct ngbe_hw *, u32, u32, bool, bool *);
	s32 (*init_uta_tables)(struct ngbe_hw *);
	void (*set_mac_anti_spoofing)(struct ngbe_hw *, bool, int);
	void (*set_vlan_anti_spoofing)(struct ngbe_hw *, bool, int);

	/* Flow Control */
	s32 (*fc_enable)(struct ngbe_hw *);
	s32 (*setup_fc)(struct ngbe_hw *);

	/* Manageability interface */
	s32 (*set_fw_drv_ver)(struct ngbe_hw *, u8, u8, u8, u8);
	s32 (*get_thermal_sensor_data)(struct ngbe_hw *);
	s32 (*init_thermal_sensor_thresh)(struct ngbe_hw *hw);
	void (*get_rtrup2tc)(struct ngbe_hw *hw, u8 *map);
	void (*disable_rx)(struct ngbe_hw *hw);
	void (*enable_rx)(struct ngbe_hw *hw);
	void (*set_source_address_pruning)(struct ngbe_hw *, bool,
					   unsigned int);
	void (*set_ethertype_anti_spoofing)(struct ngbe_hw *, bool, int);
	s32 (*dmac_config)(struct ngbe_hw *hw);
	s32 (*setup_eee)(struct ngbe_hw *hw, bool enable_eee);
};

struct ngbe_phy_operations {
	s32 (*identify)(struct ngbe_hw *);
	s32 (*identify_sfp)(struct ngbe_hw *);
	s32 (*init)(struct ngbe_hw *);
	s32 (*reset)(struct ngbe_hw *);
	s32 (*read_reg)(struct ngbe_hw *, u32, u32, u16 *);
	s32 (*write_reg)(struct ngbe_hw *, u32, u32, u16);
	s32 (*read_reg_mdi)(struct ngbe_hw *, u32, u32, u16 *);
	s32 (*write_reg_mdi)(struct ngbe_hw *, u32, u32, u16);
	u32 (*setup_link)(struct ngbe_hw *, u32, bool);
	u32 (*phy_led_ctrl)(struct ngbe_hw *);
	s32 (*setup_internal_link)(struct ngbe_hw *);
	u32 (*setup_link_speed)(struct ngbe_hw *, u32, bool);
	s32 (*check_link)(struct ngbe_hw *, u32 *, bool *);
	s32 (*check_overtemp)(struct ngbe_hw *);
	s32 (*check_event)(struct ngbe_hw *);
	s32 (*get_adv_pause)(struct ngbe_hw *, u8 *);
	s32 (*get_lp_adv_pause)(struct ngbe_hw *, u8 *);
	s32 (*set_adv_pause)(struct ngbe_hw *, u16);
	s32 (*setup_once)(struct ngbe_hw *);
};

struct ngbe_eeprom_info {
	struct ngbe_eeprom_operations ops;
	enum ngbe_eeprom_type type;
	u32 semaphore_delay;
	u16 word_size;
	u16 address_bits;
	u16 word_page_size;
	u16 ctrl_word_3;
	u16 sw_region_offset;
};

struct ngbe_flash_info {
	struct ngbe_flash_operations ops;
	u32 semaphore_delay;
	u32 dword_size;
	u16 address_bits;
};

#define NGBE_FLAGS_DOUBLE_RESET_REQUIRED       0x01
struct ngbe_mac_info {
	struct ngbe_mac_operations ops;
	u8 addr[NGBE_ETH_LENGTH_OF_ADDRESS];
	u8 perm_addr[NGBE_ETH_LENGTH_OF_ADDRESS];
	u8 san_addr[NGBE_ETH_LENGTH_OF_ADDRESS];
	/* prefix for World Wide Node Name (WWNN) */
	u16 wwnn_prefix;
	/* prefix for World Wide Port Name (WWPN) */
	u16 wwpn_prefix;
#define NGBE_MAX_MTA                   128
#define NGBE_MAX_VFTA_ENTRIES          128
	u32 mta_shadow[NGBE_MAX_MTA];
	s32 mc_filter_type;
	u32 mcft_size;
	u32 vft_shadow[NGBE_MAX_VFTA_ENTRIES];
	u32 vft_size;
	u32 num_rar_entries;
	u32 rar_highwater;
	u32 rx_pb_size;
	u32 max_tx_queues;
	u32 max_rx_queues;
	u32 orig_sr_pcs_ctl2;
	u32 orig_sr_pma_mmd_ctl1;
	u32 orig_sr_an_mmd_ctl;
	u32 orig_sr_an_mmd_adv_reg2;
	u32 orig_vr_xs_or_pcs_mmd_digi_ctl1;
	u8  san_mac_rar_index;
	bool get_link_status;
	u16 max_msix_vectors;
	bool arc_subsystem_valid;
	bool orig_link_settings_stored;
	bool autotry_restart;
	u8 flags;
	struct ngbe_thermal_sensor_data  thermal_sensor_data;
	bool thermal_sensor_enabled;
	struct ngbe_dmac_config dmac_config;
	bool set_lben;
	bool autoneg;
};

struct ngbe_phy_info {
	struct ngbe_phy_operations ops;
	enum ngbe_phy_type type;
	u32 addr;
	u32 id;
	enum ngbe_sfp_type sfp_type;
	bool sfp_setup_needed;
	u32 revision;
	enum ngbe_media_type media_type;
	u32 phy_semaphore_mask;
	u8 lan_id; /* to be delete */
	ngbe_autoneg_advertised autoneg_advertised;
	enum ngbe_smart_speed smart_speed;
	bool smart_speed_active;
	bool multispeed_fiber;
	bool reset_if_overtemp;
	ngbe_physical_layer link_mode;
	u32 force_speed;
};

#include "ngbe_mbx.h"

struct ngbe_mbx_operations {
	void (*init_params)(struct ngbe_hw *hw);
	s32  (*read)(struct ngbe_hw *, u32 *, u16,  u16);
	s32  (*write)(struct ngbe_hw *, u32 *, u16, u16);
	s32  (*read_posted)(struct ngbe_hw *, u32 *, u16,  u16);
	s32  (*write_posted)(struct ngbe_hw *, u32 *, u16, u16);
	s32  (*check_for_msg)(struct ngbe_hw *, u16);
	s32  (*check_for_ack)(struct ngbe_hw *, u16);
	s32  (*check_for_rst)(struct ngbe_hw *, u16);
};

struct ngbe_mbx_stats {
	u32 msgs_tx;
	u32 msgs_rx;

	u32 acks;
	u32 reqs;
	u32 rsts;
};

struct ngbe_mbx_info {
	struct ngbe_mbx_operations ops;
	struct ngbe_mbx_stats stats;
	u32 timeout;
	u32 udelay;
	u32 v2p_mailbox;
	u16 size;
};

enum ngbe_reset_type {
	NGBE_LAN_RESET = 0,
	NGBE_SW_RESET,
	NGBE_GLOBAL_RESET
};

enum ngbe_link_status {
	NGBE_LINK_STATUS_NONE = 0,
	NGBE_LINK_STATUS_KX,
	NGBE_LINK_STATUS_KX4
};

struct ngbe_hw {
	u8 IOMEM *hw_addr;
	void *back;
	struct ngbe_mac_info mac;
	struct ngbe_addr_filter_info addr_ctrl;
	struct ngbe_fc_info fc;
	struct ngbe_phy_info phy;
	struct ngbe_eeprom_info eeprom;
	struct ngbe_flash_info flash;
	struct ngbe_bus_info bus;
	struct ngbe_mbx_info mbx;
	u16 device_id;
	u16 vendor_id;
	u16 subsystem_device_id;
	u16 subsystem_vendor_id;
	u8 revision_id;
	bool adapter_stopped;
	int api_version;
	enum ngbe_reset_type reset_type;
	bool force_full_reset;
	bool allow_unsupported_sfp;
	bool wol_enabled;
	enum ngbe_link_status link_status;
	u16 tpid[8];
	bool gpio_ctl;
};

#define TCALL(hw, func, args...) (((hw)->func != NULL) \
		? (hw)->func((hw), ##args) : NGBE_NOT_IMPLEMENTED)

/* Error Codes */
#define NGBE_OK                                 0
#define NGBE_ERR                                100
#define NGBE_NOT_IMPLEMENTED                    0x7FFFFFFF
/* (-NGBE_ERR, NGBE_ERR): reserved for non-ngbe defined error code */
#define NGBE_ERR_NOSUPP                        -(NGBE_ERR+0)
#define NGBE_ERR_EEPROM                        -(NGBE_ERR+1)
#define NGBE_ERR_EEPROM_CHECKSUM               -(NGBE_ERR+2)
#define NGBE_ERR_PHY                           -(NGBE_ERR+3)
#define NGBE_ERR_CONFIG                        -(NGBE_ERR+4)
#define NGBE_ERR_PARAM                         -(NGBE_ERR+5)
#define NGBE_ERR_MAC_TYPE                      -(NGBE_ERR+6)
#define NGBE_ERR_UNKNOWN_PHY                   -(NGBE_ERR+7)
#define NGBE_ERR_LINK_SETUP                    -(NGBE_ERR+8)
#define NGBE_ERR_ADAPTER_STOPPED               -(NGBE_ERR+09)
#define NGBE_ERR_INVALID_MAC_ADDR              -(NGBE_ERR+10)
#define NGBE_ERR_DEVICE_NOT_SUPPORTED          -(NGBE_ERR+11)
#define NGBE_ERR_MASTER_REQUESTS_PENDING       -(NGBE_ERR+12)
#define NGBE_ERR_INVALID_LINK_SETTINGS         -(NGBE_ERR+13)
#define NGBE_ERR_AUTONEG_NOT_COMPLETE          -(NGBE_ERR+14)
#define NGBE_ERR_RESET_FAILED                  -(NGBE_ERR+15)
#define NGBE_ERR_SWFW_SYNC                     -(NGBE_ERR+16)
#define NGBE_ERR_PHY_ADDR_INVALID              -(NGBE_ERR+17)
#define NGBE_ERR_I2C                           -(NGBE_ERR+18)
#define NGBE_ERR_SFP_NOT_SUPPORTED             -(NGBE_ERR+19)
#define NGBE_ERR_SFP_NOT_PRESENT               -(NGBE_ERR+20)
#define NGBE_ERR_SFP_NO_INIT_SEQ_PRESENT       -(NGBE_ERR+21)
#define NGBE_ERR_NO_SAN_ADDR_PTR               -(NGBE_ERR+22)
#define NGBE_ERR_FDIR_REINIT_FAILED            -(NGBE_ERR+23)
#define NGBE_ERR_EEPROM_VERSION                -(NGBE_ERR+24)
#define NGBE_ERR_NO_SPACE                      -(NGBE_ERR+25)
#define NGBE_ERR_OVERTEMP                      -(NGBE_ERR+26)
#define NGBE_ERR_UNDERTEMP                     -(NGBE_ERR+27)
#define NGBE_ERR_FC_NOT_NEGOTIATED             -(NGBE_ERR+28)
#define NGBE_ERR_FC_NOT_SUPPORTED              -(NGBE_ERR+29)
#define NGBE_ERR_SFP_SETUP_NOT_COMPLETE        -(NGBE_ERR+30)
#define NGBE_ERR_PBA_SECTION                   -(NGBE_ERR+31)
#define NGBE_ERR_INVALID_ARGUMENT              -(NGBE_ERR+32)
#define NGBE_ERR_HOST_INTERFACE_COMMAND        -(NGBE_ERR+33)
#define NGBE_ERR_OUT_OF_MEM                    -(NGBE_ERR+34)
#define NGBE_ERR_FEATURE_NOT_SUPPORTED         -(NGBE_ERR+36)
#define NGBE_ERR_EEPROM_PROTECTED_REGION       -(NGBE_ERR+37)
#define NGBE_ERR_FDIR_CMD_INCOMPLETE           -(NGBE_ERR+38)
#define NGBE_ERR_FLASH_LOADING_FAILED          -(NGBE_ERR+39)
#define NGBE_ERR_XPCS_POWER_UP_FAILED          -(NGBE_ERR+40)
#define NGBE_ERR_FW_RESP_INVALID               -(NGBE_ERR+41)
#define NGBE_ERR_PHY_INIT_NOT_DONE             -(NGBE_ERR+42)
#define NGBE_ERR_TIMEOUT                       -(NGBE_ERR+43)
#define NGBE_ERR_TOKEN_RETRY                   -(NGBE_ERR+44)
#define NGBE_ERR_REGISTER                      -(NGBE_ERR+45)
#define NGBE_ERR_MBX                           -(NGBE_ERR+46)
#define NGBE_ERR_MNG_ACCESS_FAILED             -(NGBE_ERR+47)
#define NGBE_ERR_PHY_TYPE                      -(NGBE_ERR+48)
#define NGBE_ERR_PHY_TIMEOUT                   -(NGBE_ERR+49)

/**
 * register operations
 **/
/* read register */
#define NGBE_DEAD_READ_RETRIES     10
#define NGBE_DEAD_READ_REG         0xdeadbeefU
#define NGBE_DEAD_READ_REG64       0xdeadbeefdeadbeefULL

#define NGBE_FAILED_READ_REG       0xffffffffU
#define NGBE_FAILED_READ_REG64     0xffffffffffffffffULL

static inline bool NGBE_REMOVED(void __iomem *addr)
{
	return unlikely(!addr);
}

static inline u32
ngbe_rd32(u8 __iomem *base)
{
	return readl(base);
}

static inline u32
rd32(struct ngbe_hw *hw, u32 reg)
{
	u8 __iomem *base = READ_ONCE(hw->hw_addr);
	u32 val = NGBE_FAILED_READ_REG;

	if (unlikely(!base))
		return val;

	val = ngbe_rd32(base + reg);

	return val;
}
#define rd32a(a, reg, offset) ( \
	rd32((a), (reg) + ((offset) << 2)))

static inline u32
rd32m(struct ngbe_hw *hw, u32 reg, u32 mask)
{
	u8 __iomem *base = READ_ONCE(hw->hw_addr);
	u32 val = NGBE_FAILED_READ_REG;

	if (unlikely(!base))
		return val;

	val = ngbe_rd32(base + reg);
	if (unlikely(val == NGBE_FAILED_READ_REG))
		return val;

	return val & mask;
}

/* write register */
static inline void
ngbe_wr32(u8 __iomem *base, u32 val)
{
	writel(val, base);
}

static inline void
wr32(struct ngbe_hw *hw, u32 reg, u32 val)
{
	u8 __iomem *base = READ_ONCE(hw->hw_addr);

	if (unlikely(!base))
		return;

	ngbe_wr32(base + reg, val);
}
#define wr32a(a, reg, off, val) \
	wr32((a), (reg) + ((off) << 2), (val))

static inline void
wr32m(struct ngbe_hw *hw, u32 reg, u32 mask, u32 field)
{
	u8 __iomem *base = READ_ONCE(hw->hw_addr);
	u32 val;

	if (unlikely(!base))
		return;

	val = ngbe_rd32(base + reg);
	if (unlikely(val == NGBE_FAILED_READ_REG))
		return;

	val = ((val & ~mask) | (field & mask));
	ngbe_wr32(base + reg, val);
}

/* poll register */
#define NGBE_MDIO_TIMEOUT 1000
#define NGBE_I2C_TIMEOUT  1000
#define NGBE_SPI_TIMEOUT  1000
static inline s32
po32m(struct ngbe_hw *hw, u32 reg,
		u32 mask, u32 field, int usecs, int count)
{
	int loop;

	loop = (count ? count : (usecs + 9) / 10);
	usecs = (loop ? (usecs + loop - 1) / loop : 0);

	count = loop;
	do {
		u32 value = rd32(hw, reg);
		if ((value & mask) == (field & mask)) {
			break;
		}

		if (loop-- <= 0)
			break;

		usec_delay(usecs);
	} while (true);

	return (count - loop <= count ? 0 : NGBE_ERR_TIMEOUT);
}

#define NGBE_WRITE_FLUSH(H) rd32(H, NGBE_MIS_PWR)

#endif /* _NGBE_TYPE_H_ */
