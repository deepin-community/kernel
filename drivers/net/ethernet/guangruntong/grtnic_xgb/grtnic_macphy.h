/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2019 - 2026 Beijing GuangRunTong Corporation. */

#ifndef _GRTNICMACPHY_H_
#define _GRTNICMACPHY_H_

struct sfp_info {
	u8	wr_cmd;
	u8	count;
	u8	dev_addr;
	u8	reg_addr;
};


void grtnic_SetSpeed(struct net_device *netdev, int speed);
void grtnic_SetFc(struct net_device *netdev, int onoff);
int  grtnic_ResetTx(struct net_device *netdev);
int  grtnic_ResetRx(struct net_device *netdev);
void grtnic_SetTx(struct net_device *netdev, int onoff);
void grtnic_SetRx(struct net_device *netdev, int onoff);
void grtnic_GetRx(struct net_device *netdev, u32 *status);

void grtnic_SetMaxFrameLen(struct net_device *netdev, int len);
void grtnic_SetJumbo(struct net_device *netdev, int onoff);

void grtnic_SetAdrsFilter(struct net_device *netdev, int filter);
int  grtnic_GetAdrsFilter(struct net_device *netdev);
u32  grtnic_GetSFP_Reg(struct net_device *netdev, struct sfp_info *sfc_info);

void grtnic_SetMacAddress(struct net_device *netdev, const u8 *AddressPtr);
void grtnic_GetMacAddress(struct net_device *netdev, void *AddressPtr);
void grtnic_PhySetMdioDivisor(struct net_device *netdev, u8 Divisor);
int  grtnic_Get_phy_status(struct net_device *netdev, int *linkup);

void grtnic_SetPhyAddr(struct net_device *netdev, u32 Prtad, u32 Devad, u32 RegisterNum); //only for 10G phy
void grtnic_PhyRead(struct net_device *netdev, u32 PhyAddress, u32 RegisterNum, u16 *PhyDataPtr);
void grtnic_PhyWrite(struct net_device *netdev, u32 PhyAddress, u32 RegisterNum, u16 PhyData);

void grtnic_SetPause(struct net_device *netdev, u8 flowctl);
void grtnic_set_fc_watermarks(struct net_device *netdev);
void grtnic_SetMacPauseAddress(struct net_device *netdev, const u8 *AddressPtr);
void grtnic_GetMacPauseAddress(struct net_device *netdev, void *AddressPtr);
u64 grtnic_get_statistics_cnt(struct grtnic_adapter *adapter, u32 reg, u32 old_cnt);

s32 grtnic_read_i2c_eeprom(struct grtnic_hw *hw, u8 byte_offset, u8 *eeprom_data);
s32 grtnic_read_i2c_sff8472(struct grtnic_hw *hw, u8 byte_offset, u8 *sff8472_data);


#define XXGE_PORT_ADDRBASE	0x00008000

#define XXGE_RCW0_OFFSET	0x00000400 /**< Rx Configuration Word 0 */
#define XXGE_RCW1_OFFSET	0x00000404 /**< Rx Configuration Word 1 */
#define XXGE_TC_OFFSET		0x00000408 /**< Tx Configuration */
#define XXGE_FCC_OFFSET		0x0000040C /**< Flow Control Configuration */

#define XXGE_SPEED_OFFSET	0x00000410 /**< MAC Speed Configuration */

#define XXGE_RMTU_OFFSET	0x00000414 /**< Receiver MTU Configuration Word ~chng..  */
#define XXGE_TMTU_OFFSET	0x00000418 /**< Transmitter MTU Configuration Word ~chng.. */

#define XXGE_MDIO_REGISTER_ADDRESS  32         /* Register to read for getting phy status */
#define XXGE_MDIO_CFG0_OFFSET       0x00000500 /**< MDIO Configuration word 0 */
#define XXGE_MDIO_CFG1_OFFSET	    0x00000504 /**< MDIO Configuration word 1 */
#define XXGE_MDIO_TX_DATA_OFFSET    0x00000508 /**< MDIO TX Data */
#define XXGE_MDIO_RX_DATA_OFFSET	0x0000050C /**< MDIO RX Data (Read-only) */

#define XXGE_TC_TXCONTROLBIT            0x1C /* Set this bit to 0 to disable transmission */

/** @name Flow Control Configuration (FCC) Register Bit definitions
 *  @{
 */
#define XXGE_FCC_FCRX_MASK	0x20000000   /**< Rx flow control enable */
#define XXGE_FCC_FCTX_MASK	0x40000000   /**< Tx flow control enable */


/** @name Receive Configuration Word 1 (RCW1) Register bit definitions
 *  @{
 */
#define XXGE_RCW1_RST_MASK		0x80000000 /**< Reset */
#define XXGE_RCW1_JUM_MASK		0x40000000 /**< Jumbo frame enable */
#define XXGE_RCW1_FCS_MASK		0x20000000 /**< In-Band FCS enable (FCS not stripped) */
#define XXGE_RCW1_RX_MASK			0x10000000 /**< Receiver enable */
#define XXGE_RCW1_VLAN_MASK		0x08000000 /**< VLAN frame enable */

/**< Receiver Preserve Preamble Enable !!chng... change HD<->PP */
#define XXGE_RCW1_HD_MASK			0x04000000
#define XXGE_RCW1_LT_DIS_MASK	0x02000000 /**< Length/type field valid check
					     *  disable
					     */
#define XXGE_RCW1_CL_DIS_MASK	0x01000000 /**< Control frame Length check
					     *  disable
					     */
#define XXGE_RCW1_PAUSEADDR_MASK 0x0000FFFF	/*< Pause frame source
						 *  address bits [47:32].Bits
						 *	[31:0] are stored in register
						 *  RCW0
						 */
/** @name Transmitter Configuration (TC) Register bit definitions
 *  @{
 */
#define XXGE_TC_RST_MASK		0x80000000 /**< Reset */
#define XXGE_TC_JUM_MASK		0x40000000 /**< Jumbo frame enable */
#define XXGE_TC_FCS_MASK		0x20000000 /*< In-Band FCS enable
						    *  (FCS not generated)
						    */
#define XXGE_TC_TX_MASK		0x10000000 /**< Transmitter enable */
#define XXGE_TC_VLAN_MASK	0x08000000 /**< VLAN frame enable */
#define XXGE_TC_HD_MASK		0x04000000 /**< WAN Mode Enable !!chng...bit-26 we may NOT use*/
#define XXGE_TC_IFG_MASK	0x02000000 /**< Inter-frame gap adjustment enable */
#define XXGE_TC_DIC_MASK	0x01000000 /**< Deficit Idle Count Enable */


/** @name MDIO Management Configuration (MC) Register bit definitions
 * @{
 */
#define XXGE_MDIO_CFG0_MDIOEN_MASK		0x00000040  /**< MII management enable*/
#define XXGE_MDIO_CFG0_CLOCK_DIVIDE_MAX	0x3F        /**< Maximum MDIO divisor */
#define XXGE_MDIO_PHY_LINK_UP_MASK      0x1000 /* Checking for 12th bit  */


#define XXGE_MDIO_MC_MDIOPRTAD_MASK		0x1F000000  /**< PRTAD ...b28:24*/
#define XXGE_MDIO_MC_CLOCK_DEVAD_MAX	0x001F0000  /**< DEVAD ...b20:16*/
#define XXGE_MDIO_MC_MDIO_TXOP_MASK		0x0000C000  /**< TX OP ...b15:14*/
#define XXGE_MDIO_CFG1_INITIATE_MASK	0x00000800  /**< Initiate ...b11 */
#define XXGE_MDIO_CFG1_READY_MASK       0x00000080  /**< MDIO Ready ...b7*/
#define XXGE_MDIO_CFG1_OP_SETADDR_MASK	0x00000000  /**< Opcode Set Addr Mask */
#define XXGE_MDIO_CFG1_OP_READ_MASK     0x00008000  /**< Opcode Read Mask */
#define XXGE_MDIO_CFG1_OP_WRITE_MASK	0x00004000  /**< Opcode Write Mask */

#define XXGE_MDIO_CFG1_OP_READ_MASK_10G     0x0000C000  /**< Opcode Read Mask for 10G*/

/*@}*/


/** @name MDIO TX Data (MTX) Register bit definitions
 * @{
 */
#define XXGE_MDIO_TX_DATA_MASK		0x0000FFFF /**< MDIO TX Data ...b15:0 */

/** @name MDIO TX Data (MTX) Register bit definitions
 * @{
 */
#define XXGE_MDIO_RX_DATA_MASK		0x0000FFFF /**< MDIO RX Data ...b15:0 */


//user define
#define XXGE_RST_DELAY_LOOPCNT_VAL	4	/**< Timeout in ticks used
						  *  while checking if the core
						  *  had come out of reset. The
						  *  exact tick time is defined
						  *  in each case/loop where it
						  *  will be used
						  */
#define XXGE_MDIO_RDY_LOOPCNT_VAL	100	// Timeout in ticks used



/* PHY Control Register */
#define PHY_SPEED_SELECT_MSB	0x0040  /* bits 6,13: 10=1000, 01=100, 00=10 */
#define PHY_COLL_TEST_ENABLE	0x0080  /* Collision test enable */
#define PHY_FULL_DUPLEX	0x0100  /* FDX =1, half duplex =0 */
#define PHY_RESTART_AUTO_NEG	0x0200  /* Restart auto negotiation */
#define PHY_ISOLATE		0x0400  /* Isolate PHY from MII */
#define PHY_POWER_DOWN	0x0800  /* Power down */
#define PHY_AUTO_NEG_EN	0x1000  /* Auto Neg Enable */
#define PHY_SPEED_SELECT_LSB	0x2000  /* bits 6,13: 10=1000, 01=100, 00=10 */
#define PHY_LOOPBACK		0x4000  /* 0 = normal, 1 = loopback */
#define PHY_RESET		0x8000  /* 0 = normal, 1 = PHY reset */
#define PHY_SPEED_1000	0x0040
#define PHY_SPEED_100	0x2000
#define PHY_SPEED_10		0x0000

/* PHY Basic Control Register 0x00*/
#define PHY_BMCR_RESET			BIT(15)  /* bits 15, 1: PHY Software Reset.*/
#define PHY_BMCR_LOOPBACK		BIT(14)  /* bits 14, Internal loopback control*/
#define PHY_BMCR_AUTO_NEG		BIT(12)  /* bits 12, 1: to enable auto-negotiation*/
#define PHY_BMCR_POWER_DOWN	BIT(11)  /* bits 11, 1: Power down */
#define PHY_BMCR_ISOLATE		BIT(10)  /* bits 10, 1: Isolate mode */
#define PHY_BMCR_RE_AUTONEG	BIT(9)  /* bits 9, 1: Restart Auto-Negotiation Process  */
#define PHY_BMCR_DUPLEX			BIT(8)  /* bits 8, 1: Full Duplex  */
#define PHY_BMCR_SPEED_MASK	(BIT(6) | BIT(13))  /* bits 6,13 Speed_Selection  */
#define PHY_BMCR_SPEED_LSB	BIT(13)  /* bits 6,13 Speed_Selection  */
#define PHY_BMCR_SPEED_MSB	BIT(6)  /* bits 6,13 Speed_Selection  */

/* PHY Auto-Negotiation advertisement Register 0x04*/
/* bits 15, 1: Advertise Next_page*/
#define PHY_NEXT_Page				BIT(15)
/* bits 12, 1: Extended nEXT page enable control bit*/
#define PHY_XNP							BIT(12)
/* bits 8, 1: Advertise support of 100Base-TX full-duplex mode*/
#define PHY_100Base_TX_Full	BIT(8)
/* bits 7, 1: Advertise support of 100Base-TX half-duplex mode*/
#define PHY_100Base_TX_Half	BIT(7)
/* bits 6, 1: Advertise support of 10Base-TX full-duplex mode*/
#define PHY_10Base_TX_Full	BIT(6)
/* bits 5, 1: Advertise support of 10Base-TX half-duplex mode*/
#define PHY_10Base_TX_Half	BIT(5)

/* PHY Auto-Negotiation link partner ability Register 0x05*/
/* bits 14, 1: Link partner has received link code word */
#define PHY_LP_LPACK						BIT(14)
/* bits 11, 1: Link partner requests asymmetric pause*/
#define PHY_LP_ASYM_PAUSE				BIT(11)
/* bits 10, 1: Link partner supports pause operation*/
#define PHY_LP_PAUSE						BIT(10)
/* bits 8, 1: Link Partner support of 100Base-TX full-duplex mode*/
#define PHY_LP_100Base_TX_Full	BIT(8)
/* bits 7, 1: Link Partner support of 100Base-TX half-duplex mode*/
#define PHY_LP_100Base_TX_Half	BIT(7)
/* bits 6, 1: Link Partner support of 10Base-TX full-duplex mode*/
#define PHY_LP_10Base_TX_Full		BIT(6)
/* bits 5, 1: Link Partner support of 10Base-TX half-duplex mode*/
#define PHY_LP_10Base_TX_Half		BIT(5)

/* PHY Auto-Negotiation NEXT Page register (0x07)*/
/* bits 15, 1: The page is not the last page  */
#define PHY_NP_NEXT_Page		BIT(15)
/* bits 13, 1: Message Page  */
#define PHY_NP_MessagePage	BIT(13)
/* bits 12, 1: Comply with message  */
#define PHY_NP_Ack2					BIT(12)
/* bits 11, 1: This bit in the previously exchanged Code Word is logic 0  */
#define PHY_NP_Toggle				BIT(11)
/* bits 10:0, These bits are encoded as Message Code Field when bit[13] is set to 1,
 *or as Unformatted Code Field when bit[13] is set to 0.
 */
#define PHY_NP_MESSAGE_MASK	0x07FF

/* PHY MASTER-SLAVE control register 0x09*/
/* bits 9, 1: Advertise support of 1000Base-T full-duplex mode*/
#define PHY_1000Base_T_Full		BIT(9)
/* bits 8, 1: Advertise support of 1000Base-T full-duplex mode*/
#define PHY_1000Base_T_Half		BIT(8)

/* PHY MASTER-SLAVE status register 0x0A*/
/* bits 11, 1: Link Partner supports 1000BASE-T Full duplex*/
#define PHY_LP_1000Base_T_Full	BIT(11)
/* bits 10, 1: Link Partner supports 1000BASE-T half duplex*/
#define PHY_LP_1000Base_T_Half	BIT(10)

/* PHY Multi-GBASE-T AN control 1 register (MMD7,0x20)*/
/* bits 7, 1: Advertise PHY as 2.5G BASE-T */
#define PHY_2500Base_T_Full			BIT(7)

/* PHY Multi-GBASE-T AN status 1 register (MMD7,0x21)*/
/* bits 5, 1: link partner is able to operate as 2.5G BASE-T*/
#define PHY_LP_2500Base_T_Full	BIT(5)

/* SFP I2C */
#define GRTNIC_I2C_CLOCK_STRETCHING_TIMEOUT	500
#define GRTNIC_ERR_PHY_ADDR_INVALID		-17
#define GRTNIC_ERR_I2C								-18

#define GRTNIC_I2C_EEPROM_DEV_ADDR	0xA0
#define GRTNIC_I2C_EEPROM_DEV_ADDR2	0xA2
#define GRTNIC_I2C_EEPROM_BANK_LEN	0xFF

/* EEPROM byte offsets */
#define GRTNIC_SFF_IDENTIFIER				0x0
#define GRTNIC_SFF_IDENTIFIER_SFP		0x3
#define GRTNIC_SFF_VENDOR_OUI_BYTE0	0x25
#define GRTNIC_SFF_VENDOR_OUI_BYTE1	0x26
#define GRTNIC_SFF_VENDOR_OUI_BYTE2	0x27
#define GRTNIC_SFF_1GBE_COMP_CODES	0x6
#define GRTNIC_SFF_10GBE_COMP_CODES	0x3
#define GRTNIC_SFF_CABLE_TECHNOLOGY	0x8
#define GRTNIC_SFF_CABLE_SPEC_COMP	0x3C
#define GRTNIC_SFF_SFF_8472_SWAP		0x5C
#define GRTNIC_SFF_SFF_8472_COMP		0x5E
#define GRTNIC_SFF_SFF_8472_OSCB		0x6E
#define GRTNIC_SFF_SFF_8472_ESCB		0x76
#define GRTNIC_SFF_IDENTIFIER_QSFP_PLUS	0xD
#define GRTNIC_SFF_QSFP_VENDOR_OUI_BYTE0	0xA5
#define GRTNIC_SFF_QSFP_VENDOR_OUI_BYTE1	0xA6
#define GRTNIC_SFF_QSFP_VENDOR_OUI_BYTE2	0xA7
#define GRTNIC_SFF_QSFP_CONNECTOR		0x82
#define GRTNIC_SFF_QSFP_10GBE_COMP	0x83
#define GRTNIC_SFF_QSFP_1GBE_COMP		0x86
#define GRTNIC_SFF_QSFP_CABLE_LENGTH	0x92
#define GRTNIC_SFF_QSFP_DEVICE_TECH	0x93

/* Bitmasks */
#define GRTNIC_SFF_DA_PASSIVE_CABLE	0x4
#define GRTNIC_SFF_DA_ACTIVE_CABLE	0x8
#define GRTNIC_SFF_DA_SPEC_ACTIVE_LIMITING	0x4
#define GRTNIC_SFF_1GBASESX_CAPABLE	0x1
#define GRTNIC_SFF_1GBASELX_CAPABLE	0x2
#define GRTNIC_SFF_1GBASET_CAPABLE	0x8
#define GRTNIC_SFF_10GBASESR_CAPABLE	0x10
#define GRTNIC_SFF_10GBASELR_CAPABLE	0x20
#define GRTNIC_SFF_SOFT_RS_SELECT_MASK	0x8
#define GRTNIC_SFF_SOFT_RS_SELECT_10G	0x8
#define GRTNIC_SFF_SOFT_RS_SELECT_1G	0x0
#define GRTNIC_SFF_ADDRESSING_MODE		0x4
#define GRTNIC_SFF_QSFP_DA_ACTIVE_CABLE		0x1
#define GRTNIC_SFF_QSFP_DA_PASSIVE_CABLE	0x8
#define GRTNIC_SFF_QSFP_CONNECTOR_NOT_SEPARABLE	0x23
#define GRTNIC_SFF_QSFP_TRANSMITER_850NM_VCSEL	0x0
#define GRTNIC_I2C_EEPROM_READ_MASK		0x100
#define GRTNIC_I2C_EEPROM_STATUS_MASK	0x3
#define GRTNIC_I2C_EEPROM_STATUS_NO_OPERATION	0x0
#define GRTNIC_I2C_EEPROM_STATUS_PASS	0x1
#define GRTNIC_I2C_EEPROM_STATUS_FAIL	0x2
#define GRTNIC_I2C_EEPROM_STATUS_IN_PROGRESS	0x3

#define GRTNIC_TN_LASI_STATUS_REG	0x9005
#define GRTNIC_TN_LASI_STATUS_TEMP_ALARM	0x0008

/* SFP+ SFF-8472 Compliance */
#define GRTNIC_SFF_SFF_8472_UNSUP	0x00


/* I2C SDA and SCL timing parameters for standard mode */
#define GRTNIC_I2C_T_HD_STA  4
#define GRTNIC_I2C_T_LOW   5
#define GRTNIC_I2C_T_HIGH  4
#define GRTNIC_I2C_T_SU_STA  5
#define GRTNIC_I2C_T_HD_DATA 5
#define GRTNIC_I2C_T_SU_DATA 1
#define GRTNIC_I2C_T_RISE  1
#define GRTNIC_I2C_T_FALL  1
#define GRTNIC_I2C_T_SU_STO  4
#define GRTNIC_I2C_T_BUF   5

#define GRTNIC_I2C_CLK_IN    0x00000001
#define GRTNIC_I2C_CLK_OUT   0x00000002
#define GRTNIC_I2C_DATA_IN   0x00000004
#define GRTNIC_I2C_DATA_OUT  0x00000008

#define msec_delay(_x) msleep(_x)
#define usec_delay(_x) udelay(_x)

#define DEBUGFUNC(S)		do {} while (0)

#endif /* _XDMANET_H_ */
