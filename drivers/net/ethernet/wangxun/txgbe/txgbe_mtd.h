/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2015 - 2022 Beijing WangXun Technology Co., Ltd. */

#ifndef _TXGBE_MTD_H_
#define _TXGBE_MTD_H_

#define MTD_CONVERT_BOOL_TO_UINT(bool_var, uint_var)  \
					{(bool_var) ? (uint_var = 1) : (uint_var = 0); }
#define MTD_CONVERT_UINT_TO_BOOL(uint_var, bool_var)  \
					{(uint_var) ? (bool_var = true) : (bool_var = false); }
#define MTD_GET_BOOL_AS_BIT(bool_var) ((bool_var) ? 1 : 0)
#define MTD_GET_BIT_AS_BOOL(uint_var) ((uint_var) ? true : false)

typedef void (*MTD_VOIDFUNCPTR) (void); /* ptr to function returning void */
typedef u32  (*MTD_INTFUNCPTR)  (void); /* ptr to function returning int  */

enum begin_state {
	EMPTY,
	FULL
};

typedef u32 (*f_create)(enum begin_state state);
typedef u32 (*f_delete)(u32 sem_id);
typedef u32 (*f_take)(u32 sem_id, u32 timeout);
typedef u32 (*f_give)(u32 sem_id);

struct mtd_dev;

typedef u32 (*fmtd_read_mdio)(struct mtd_dev *dev,
								u16 port,
								u16 mmd,
								u16 reg,
								u16 *value);
typedef u32 (*fmtd_write_mdio)(struct mtd_dev *dev,
								u16 port,
								u16 mmd,
								u16 reg,
								u16 value);

/* enum txgbe_mtd_dev_id format:  */
/* Bits 15:13 reserved */
/* Bit 12: 1-> E20X0 device with max speed of 5G and no fiber interface */
/* Bit 11: 1-> Macsec Capable (Macsec/PTP module included */
/* Bit  10: 1-> Copper Capable (T unit interface included) */
/* Bits 9:4 0x18 -> X32X0 base, 0x1A 0x33X0 base */
/* Bits 3:0 revision/number of ports indication, see list */
/* Following defines are for building enum txgbe_mtd_dev_id */
#define MTD_E20X0_DEVICE BIT(12)   /* whether this is an E20X0 device group */
#define MTD_MACSEC_CAPABLE BIT(11) /* whether the device has a Macsec/PTP module */
#define MTD_COPPER_CAPABLE BIT(10) /* whether the device has a copper (T unit) module */
#define MTD_X32X0_BASE (0x18 << 4)   /* whether the device uses X32X0 firmware base */
#define MTD_X33X0_BASE (0x1A << 4)   /* whether the device uses X33X0 firmware base */

/* Following macros are to test enum txgbe_mtd_dev_id for various features */
#define MTD_IS_E20X0_DEVICE(mtd_rev_id) ((bool)((mtd_rev_id) & MTD_E20X0_DEVICE))
#define MTD_IS_MACSEC_CAPABLE(mtd_rev_id) ((bool)((mtd_rev_id) & MTD_MACSEC_CAPABLE))
#define MTD_IS_COPPER_CAPABLE(mtd_rev_id) ((bool)((mtd_rev_id) & MTD_COPPER_CAPABLE))
#define MTD_IS_X32X0_BASE(mtd_rev_id) ((bool)(((mtd_rev_id) & (0x3F << 4)) == MTD_X32X0_BASE))
#define MTD_IS_X33X0_BASE(mtd_rev_id) ((bool)(((mtd_rev_id) & (0x3F << 4)) == MTD_X33X0_BASE))

#define MTD_X33X0BASE_SINGLE_PORTA0 0xA
#define MTD_X33X0BASE_DUAL_PORTA0   0x6
#define MTD_X33X0BASE_QUAD_PORTA0   0x2

/* internal device registers */
#define MTD_REG_CCCR9 0xF05E	/* do not enclose in parentheses */
#define MTD_REG_SCR   0xF0F0	/* do not enclose in parentheses */
#define MTD_REG_ECSR  0xF0F5	/* do not enclose in parentheses */

/* WARNING: If you add/modify this list, you must also modify txgbe_mtd_phy_rev_vaild() */
enum txgbe_mtd_dev_id {
	MTD_REV_UNKNOWN = 0,
	MTD_REV_3240P_Z2 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X32X0_BASE | 0x1),
	MTD_REV_3240P_A0 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X32X0_BASE | 0x2),
	MTD_REV_3240P_A1 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X32X0_BASE | 0x3),
	MTD_REV_3220P_Z2 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X32X0_BASE | 0x4),
	MTD_REV_3220P_A0 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X32X0_BASE | 0x5),
	MTD_REV_3240_Z2 = (MTD_COPPER_CAPABLE | MTD_X32X0_BASE | 0x1),
	MTD_REV_3240_A0 = (MTD_COPPER_CAPABLE | MTD_X32X0_BASE | 0x2),
	MTD_REV_3240_A1 = (MTD_COPPER_CAPABLE | MTD_X32X0_BASE | 0x3),
	MTD_REV_3220_Z2 = (MTD_COPPER_CAPABLE | MTD_X32X0_BASE | 0x4),
	MTD_REV_3220_A0 = (MTD_COPPER_CAPABLE | MTD_X32X0_BASE | 0x5),

	MTD_REV_3310P_Z1 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x8),
	MTD_REV_3320P_Z1 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x4),
	MTD_REV_3340P_Z1 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x0),
	MTD_REV_3310_Z1 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x8),
	MTD_REV_3320_Z1 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x4),
	MTD_REV_3340_Z1 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x0),

	MTD_REV_3310P_Z2 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x9),
	MTD_REV_3320P_Z2 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x5),
	MTD_REV_3340P_Z2 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x1),
	MTD_REV_3310_Z2 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x9),
	MTD_REV_3320_Z2 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x5),
	MTD_REV_3340_Z2 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x1),

	MTD_REV_E2010P_Z2 = (MTD_E20X0_DEVICE | MTD_MACSEC_CAPABLE |
		MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x9),
	MTD_REV_E2020P_Z2 = (MTD_E20X0_DEVICE | MTD_MACSEC_CAPABLE |
		MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x5),
	MTD_REV_E2040P_Z2 = (MTD_E20X0_DEVICE | MTD_MACSEC_CAPABLE |
		MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x1),
	MTD_REV_E2010_Z2 = (MTD_E20X0_DEVICE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x9),
	MTD_REV_E2020_Z2 = (MTD_E20X0_DEVICE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x5),
	MTD_REV_E2040_Z2 = (MTD_E20X0_DEVICE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x1),

	MTD_REV_3310P_A0 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE |
		MTD_X33X0_BASE | MTD_X33X0BASE_SINGLE_PORTA0),
	MTD_REV_3320P_A0 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE |
		MTD_X33X0_BASE | MTD_X33X0BASE_DUAL_PORTA0),
	MTD_REV_3340P_A0 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE |
		MTD_X33X0_BASE | MTD_X33X0BASE_QUAD_PORTA0),
	MTD_REV_3310_A0 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_SINGLE_PORTA0),
	MTD_REV_3320_A0 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_DUAL_PORTA0),
	MTD_REV_3340_A0 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_QUAD_PORTA0),

	MTD_REV_E2010P_A0 = (MTD_E20X0_DEVICE | MTD_MACSEC_CAPABLE |
		MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_SINGLE_PORTA0),
	MTD_REV_E2020P_A0 = (MTD_E20X0_DEVICE | MTD_MACSEC_CAPABLE |
		MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_DUAL_PORTA0),
	MTD_REV_E2040P_A0 = (MTD_E20X0_DEVICE | MTD_MACSEC_CAPABLE |
		MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_QUAD_PORTA0),
	MTD_REV_E2010_A0 = (MTD_E20X0_DEVICE | MTD_COPPER_CAPABLE |
		MTD_X33X0_BASE | MTD_X33X0BASE_SINGLE_PORTA0),
	MTD_REV_E2020_A0 = (MTD_E20X0_DEVICE | MTD_COPPER_CAPABLE |
		MTD_X33X0_BASE | MTD_X33X0BASE_DUAL_PORTA0),
	MTD_REV_E2040_A0 = (MTD_E20X0_DEVICE | MTD_COPPER_CAPABLE |
		MTD_X33X0_BASE | MTD_X33X0BASE_QUAD_PORTA0),

	MTD_REV_2340P_A1 = (MTD_MACSEC_CAPABLE | MTD_X32X0_BASE | 0x3),
	MTD_REV_2320P_A0 = (MTD_MACSEC_CAPABLE | MTD_X32X0_BASE | 0x5),
	MTD_REV_2340_A1 = (MTD_X32X0_BASE | 0x3),
	MTD_REV_2320_A0 = (MTD_X32X0_BASE | 0x5)
};

enum mtd_msec_rev {
	MTD_MSEC_REV_Z0A,
	MTD_MSEC_REV_Y0A,
	MTD_MSEC_REV_A0B,
	MTD_MSEC_REV_FPGA,
	MTD_MSEC_REV_UNKNOWN = -1
};

/* compatible for USB test */
struct mtd_sec_ctrl {
	s32 dev_num;
	s32 port_num;
	u16 prev_addr;
	u16 prev_dataL;
	enum mtd_msec_rev msec_rev;
};

struct mtd_dev {
	enum txgbe_mtd_dev_id device_id;
	bool dev_enabled;
	u8 num_ports;
	u8 this_port;
	u32 multi_addr_sem;

	fmtd_read_mdio fmtd_read_mdio;
	fmtd_write_mdio mtd_write_mdio;

	f_create sem_create;  /* create semapore */
	f_delete sem_delete;  /* delete the semapore */
	f_take sem_take;	/* try to get a semapore */
	f_give sem_give;	/* return semaphore */

	u8 macsec_indirect_access; /* if true use internal processor to access Macsec */
	struct mtd_sec_ctrl   msec_ctrl;  /* structure use for internal verification */

	void *app_data;
};

#define MTD_OK			0	/* Operation succeeded */
#define MTD_FAIL		1	/* Operation failed	*/
#define MTD_PENDING		2	/* Pending  */

/* bit definition */
#define MTD_BIT_0	   0x0001
#define MTD_BIT_1	   0x0002
#define MTD_BIT_2	   0x0004
#define MTD_BIT_3	   0x0008
#define MTD_BIT_4	   0x0010
#define MTD_BIT_5	   0x0020
#define MTD_BIT_6	   0x0040
#define MTD_BIT_7	   0x0080
#define MTD_BIT_8	   0x0100
#define MTD_BIT_9	   0x0200
#define MTD_BIT_10	  0x0400
#define MTD_BIT_11	  0x0800
#define MTD_BIT_12	  0x1000
#define MTD_BIT_13	  0x2000
#define MTD_BIT_14	  0x4000
#define MTD_BIT_15	  0x8000

#define MTD_DBG_ERROR(...)
#define MTD_DBG_INFO(...)
#define MTD_DBG_CRITIC_INFO(...)

#define MTD_API_MAJOR_VERSION 2
#define MTD_API_MINOR_VERSION 0

static inline int txgbe_attempt(int result)
{
	if (result == MTD_FAIL)
		return MTD_FAIL;
	return MTD_OK;
}

#define MTD_7_0010_SPEED_BIT_LENGTH 4
#define MTD_7_0010_SPEED_BIT_POS	5
#define MTD_7_8000_SPEED_BIT_LENGTH 2
#define MTD_7_8000_SPEED_BIT_POS	8
#define MTD_7_0020_SPEED_BIT_LENGTH 1
#define MTD_7_0020_SPEED_BIT_POS	12
#define MTD_7_0020_SPEED_BIT_LENGTH2 2
#define MTD_7_0020_SPEED_BIT_POS2	7

/* Bit defines for speed bits */
#define MTD_FORCED_SPEEDS_BIT_MASK  (MTD_SPEED_10M_HD_AN_DIS | MTD_SPEED_10M_FD_AN_DIS | \
									 MTD_SPEED_100M_HD_AN_DIS | MTD_SPEED_100M_FD_AN_DIS)
#define MTD_LOWER_BITS_MASK			0x000F
#define MTD_GIG_SPEED_POS			4
#define MTD_XGIG_SPEED_POS			6
#define MTD_2P5G_SPEED_POS			11
#define MTD_5G_SPEED_POS			12
#define MTD_GET_1000BT_BITS(__speed_bits) (((__speed_bits) & (MTD_SPEED_1GIG_HD | \
										MTD_SPEED_1GIG_FD)) \
										>> MTD_GIG_SPEED_POS)
#define MTD_GET_10GBT_BIT(__speed_bits) (((__speed_bits) & MTD_SPEED_10GIG_FD) \
										>> MTD_XGIG_SPEED_POS)
#define MTD_GET_2P5GBT_BIT(__speed_bits) (((__speed_bits) & MTD_SPEED_2P5GIG_FD) \
										>> MTD_2P5G_SPEED_POS)
#define MTD_GET_5GBT_BIT(__speed_bits) (((__speed_bits) & MTD_SPEED_5GIG_FD) \
										>> MTD_5G_SPEED_POS)

#define MTD_CU_SPEED_10_MBPS	0 /* copper is 10BASE-T */
#define MTD_CU_SPEED_100_MBPS   1 /* copper is 100BASE-TX */
#define MTD_CU_SPEED_1000_MBPS  2 /* copper is 1000BASE-T */
#define MTD_CU_SPEED_10_GBPS	3 /* copper is 10GBASE-T */

/* for 88X33X0 family: */
#define MTD_CU_SPEED_NBT		3 /* copper is NBASE-T */
#define MTD_CU_SPEED_NBT_10G	0 /* copper is 10GBASE-T */
#define MTD_CU_SPEED_NBT_5G	 2 /* copper is 5GBASE-T */
#define MTD_CU_SPEED_NBT_2P5G   1 /* copper is 2.5GBASE-T */

#define MTD_ADV_NONE		   0x0000 /* No speeds to be advertised */
#define MTD_SPEED_10M_HD	   0x0001 /* 10BT half-duplex */
#define MTD_SPEED_10M_FD	   0x0002 /* 10BT full-duplex */
#define MTD_SPEED_100M_HD	   0x0004 /* 100BASE-TX half-duplex */
#define MTD_SPEED_100M_FD	   0x0008 /* 100BASE-TX full-duplex */
#define MTD_SPEED_1GIG_HD	   0x0010 /* 1000BASE-T half-duplex */
#define MTD_SPEED_1GIG_FD	   0x0020 /* 1000BASE-T full-duplex */
#define MTD_SPEED_10GIG_FD	   0x0040 /* 10GBASE-T full-duplex */
#define MTD_SPEED_2P5GIG_FD	   0x0800 /* 2.5GBASE-T full-duplex, 88X33X0/88E20X0 family only */
#define MTD_SPEED_5GIG_FD	   0x1000 /* 5GBASE-T full-duplex, 88X33X0/88E20X0 family only */
#define MTD_SPEED_ALL		   (MTD_SPEED_10M_HD | \
								MTD_SPEED_10M_FD | \
								MTD_SPEED_100M_HD | \
								MTD_SPEED_100M_FD | \
								MTD_SPEED_1GIG_HD | \
								MTD_SPEED_1GIG_FD | \
								MTD_SPEED_10GIG_FD)
#define MTD_SPEED_ALL_33X0	   (MTD_SPEED_10M_HD | \
								MTD_SPEED_10M_FD | \
								MTD_SPEED_100M_HD | \
								MTD_SPEED_100M_FD | \
								MTD_SPEED_1GIG_HD | \
								MTD_SPEED_1GIG_FD | \
								MTD_SPEED_10GIG_FD | \
								MTD_SPEED_2P5GIG_FD |\
								MTD_SPEED_5GIG_FD)

/* these bits are for forcing the speed and disabling autonegotiation */
#define MTD_SPEED_10M_HD_AN_DIS  0x0080 /* Speed forced to 10BT half-duplex */
#define MTD_SPEED_10M_FD_AN_DIS  0x0100 /* Speed forced to 10BT full-duplex */
#define MTD_SPEED_100M_HD_AN_DIS 0x0200 /* Speed forced to 100BT half-duplex */
#define MTD_SPEED_100M_FD_AN_DIS 0x0400 /* Speed forced to 100BT full-duplex */

#define MTD_SPEED_MISMATCH	   0x8000

/* for mac_type */
#define MTD_MAC_TYPE_RXAUI_SGMII_AN_EN  (0x0) /* X32X0/X33x0, but not E20x0 */
#define MTD_MAC_TYPE_RXAUI_SGMII_AN_DIS (0x1) /* X32x0/X3340/X3320, but not X3310/E20x0 */
#define MTD_MAC_TYPE_XAUI_RATE_ADAPT	(0x1) /* X3310,E2010 only */
#define MTD_MAC_TYPE_RXAUI_RATE_ADAPT   (0x2)
#define MTD_MAC_TYPE_XAUI			    (0x3) /* X3310,E2010 only */
#define MTD_MAC_TYPE_XFI_SGMII_AN_EN	(0x4)
#define MTD_MAC_TYPE_XFI_SGMII_AN_DIS   (0x5)
#define MTD_MAC_TYPE_XFI_RATE_ADAPT	    (0x6)
#define MTD_MAC_TYPE_USXGMII			(0x7) /* X33x0 only */
#define MTD_MAC_LEAVE_UNCHANGED			(0x8) /* use this option to not touch these bits */

/* for mac_snoop_sel */
#define MTD_MAC_SNOOP_FROM_NETWORK		(0x2)
#define MTD_MAC_SNOOP_FROM_HOST			(0x3)
#define MTD_MAC_SNOOP_OFF				(0x0)
#define MTD_MAC_SNOOP_LEAVE_UNCHANGED	(0x4) /* use this option to not touch these bits */
/* for mac_link_down_speed */
#define MTD_MAC_SPEED_10_MBPS			MTD_CU_SPEED_10_MBPS
#define MTD_MAC_SPEED_100_MBPS			MTD_CU_SPEED_100_MBPS
#define MTD_MAC_SPEED_1000_MBPS			MTD_CU_SPEED_1000_MBPS
#define MTD_MAC_SPEED_10_GBPS			MTD_CU_SPEED_10_GBPS
#define MTD_MAC_SPEED_LEAVE_UNCHANGED	(0x4)
/* X33X0/E20X0 devices only for mac_max_speed */
#define MTD_MAX_MAC_SPEED_10G  (0)
#define MTD_MAX_MAC_SPEED_5G   (2)
#define MTD_MAX_MAC_SPEED_2P5G (3)
#define MTD_MAX_MAC_SPEED_LEAVE_UNCHANGED (4)
#define MTD_MAX_MAC_SPEED_NOT_APPLICABLE  (4) /* 32X0 devices can pass this */

/* 88X3240/3220 Device Number Definitions */
#define MTD_T_UNIT_PMA_PMD  1
#define MTD_T_UNIT_PCS_CU   3
#define MTD_X_UNIT		  3
#define MTD_H_UNIT		  4
#define MTD_T_UNIT_AN	   7
#define MTD_XFI_DSP		 30
#define MTD_C_UNIT_GENERAL  31
#define MTD_M_UNIT		  31

/* 88X3240/3220 Device Number Definitions Host Redundant Mode */
#define MTD_BASER_LANE_0  MTD_H_UNIT
#define MTD_BASER_LANE_1  MTD_X_UNIT

/* 88X3240/3220 T Unit Registers MMD 1 */
#define MTD_TUNIT_IEEE_PMA_CTRL1        0x0000 /* do not enclose in parentheses */
#define MTD_TUNIT_IEEE_PMA_DEVID2       0x0003 /* do not enclose in parentheses */
#define MTD_TUNIT_PHY_EXT_CTRL_1        0xC000 /* do not enclose in parentheses */
#define MTD_TUNIT_XG_EXT_STATUS         0xC001 /* do not enclose in parentheses */
#define MTD_TUNIT_BIST_STATUS_REG       0xC00C /* do not enclose in parentheses */
#define MTD_TUNIT_PHY_REV_INFO_REG      0xC04E /* do not enclose in parentheses */
#define MTD_BOOT_STATUS_REG             0xC050 /* do not enclose in parentheses */

#define MTD_TUNIT_IEEE_PCS_CTRL1        0x0000 /* do not enclose in parentheses */
/* control/status for serdes initialization */
#define MTD_SERDES_CTRL_STATUS          0x800F /* do not enclose in parentheses */
/* 88X3240/3220 C Unit Registers MMD 31 */
#define MTD_CUNIT_MODE_CONFIG           0xF000 /* do not enclose in parentheses */
#define MTD_CUNIT_PORT_CTRL             0xF001 /* do not enclose in parentheses */

#define MTD_API_FAIL_SEM_CREATE			(0x18 << 24) /*sem_create Failed. */
#define MTD_API_FAIL_SEM_DELETE			(0x19 << 24) /*sem_delete Failed. */
#define MTD_API_FAIL_READ_REG			(0x16 << 16) /*Reading from phy reg failed. */
#define MTD_API_ERR_DEV					(0x3c << 16) /*driver struture is NULL. */
#define MTD_API_ERR_DEV_ALREADY_EXIST	(0x3e << 16) /*Device Driver already loaded. */

#define MTD_CLEAR_PAUSE	 0 /*  clears both pause bits */
#define MTD_SYM_PAUSE	   1 /*  for symmetric pause only */
#define MTD_ASYM_PAUSE	  2 /*  for asymmetric pause only */
#define MTD_SYM_ASYM_PAUSE  3 /*  for both */

u32 txgbe_mtd_load_driver(fmtd_read_mdio read_mdio,
			  fmtd_write_mdio write_mdio,
							bool macsec_indirect_access,
							f_create sem_create,
							f_delete sem_delete,
							f_take sem_take,
							f_give sem_give,
							u16 any_port,
							struct mtd_dev *dev);
u32 txgbe_mtd_unload_driver(struct mtd_dev *dev);
u32 txgbe_mtd_xmdio_wr(struct mtd_dev *dev_ptr,
		       u16 port,
						u16 dev,
						u16 reg,
						u16 value);
u32 txgbe_mtd_hw_xmdio_rd(struct mtd_dev *dev_ptr,
			  u16 port,
							u16 dev,
							u16 reg,
							u16 *data);

u32 txgbe_mtd_get_phy_reg_filed(struct mtd_dev *dev_ptr,
				u16 port,
								u16 dev,
								u16 reg_addr,
								u8 field_offset,
								u8 field_length,
								u16 *data);
u32 txgbe_mtd_set_phy_feild(struct mtd_dev *dev_ptr,
			    u16 port,
									u16 dev,
									u16 reg_addr,
									u8 field_offset,
									u8 field_length,
									u16 data);
u32 txgbe_mtd_get_reg_filed(u16 reg_data,
			    u8 field_offset,
							u8 field_length,
							u16 *data);
u32 txgbe_mtd_get_reg_filed(u16 reg_data,
			    u8 field_offset,
							u8 field_length,
							u16 *data);
u32 txgbe_mtd_set_reg_field_word(u16 reg_data,
				 u16 bit_field_data,
									u8 field_offset,
									u8 field_length,
									u16 *data);
u32 txgbe_mtd_wait(u32 x);
u32 txgbe_mtd_sw_rst(struct mtd_dev *dev_ptr,
		     u16 port,
						u16 timeout);

u32 txgbe_mtd_hw_rst(struct mtd_dev *dev_ptr,
		     u16 port,
						u16 timeout);
u32 txgbe_mtd_set_mac_intf_ctrl(struct mtd_dev *dev_ptr,
				u16 port,
									u16 mac_type,
									bool mac_powerdown,
									u16 mac_snoop_sel,
									u16 mac_active_lane_sel,
									u16 mac_link_down_speed,
									u16 mac_max_speed,
									bool do_sw_rst,
									bool rerun_serdes_init);

u32 txgbe_mtd_enable_speeds(struct mtd_dev *dev_ptr,
			    u16 port,
								u16 speed_bits,
								bool an_restart);

u32 txgbe_mtd_get_autoneg_res(struct mtd_dev *dev_ptr, u16 port, u16 *speed_resolution);
u32 txgbe_mtd_autoneg_done(struct mtd_dev *dev_ptr, u16 port, bool *an_speed_res_done);
u32 txgbe_mtd_is_baset_up(struct mtd_dev *dev_ptr,
			  u16 port,
							u16 *speed,
							bool *link_up);
u32 txgbe_mtd_get_firmver(struct mtd_dev *dev_ptr,
			  u16 port,
							u8 *major,
							u8 *minor,
							u8 *inc,
							u8 *test);
u32 txgbe_mtd_set_pause_adver(struct mtd_dev *dev_ptr,
			      u16 port,
								u32 pause_type,
								bool an_restart);

u32 txgbe_mtd_get_lp_adver_pause(struct mtd_dev *dev_ptr,
				 u16 port,
									u8 *pause_bits);

u32 txgbe_mtd_get_phy_revision(struct mtd_dev *dev_ptr,
			       u16 port,
								enum txgbe_mtd_dev_id *phy_rev,
								u8 *num_ports,
								u8 *this_port);
u32 txgbe_mtd_get_forced_speed(struct mtd_dev *dev_ptr,
			       u16 port,
									bool *speed_is_forced,
									u16 *force_speed);
u32 txgbe_mtd_undo_forced_speed(struct mtd_dev *dev_ptr,
				u16 port,
									bool an_restart);

u32 txgbe_mtd_autoneg_enable(struct mtd_dev *dev_ptr, u16 port);
u32 txgbe_mtd_autoneg_restart(struct mtd_dev *dev_ptr, u16 port);
u32 txgbe_mtd_phy_rev_vaild(enum txgbe_mtd_dev_id phy_rev);
#endif /* _TXGBE_MTD_H_ */
