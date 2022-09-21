/*
 * WangXun 10 Gigabit PCI Express Linux driver
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
 */


#ifndef _TXGBE_MTD_H_
#define _TXGBE_MTD_H_

#define C_LINKAGE 1 /* set to 1 if C compile/linkage on C files is desired with C++ */

#if C_LINKAGE
#if defined __cplusplus
	extern "C" {
#endif
#endif

/* general */

#undef IN
#define IN
#undef OUT
#define OUT
#undef INOUT
#define INOUT

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef void	  MTD_VOID;
typedef char	  MTD_8;
typedef short	 MTD_16;
typedef long	  MTD_32;
typedef long long MTD_64;

typedef unsigned char  MTD_U8;
typedef unsigned short MTD_U16;
typedef unsigned long  MTD_U32;
typedef unsigned int   MTD_UINT;
typedef int			MTD_INT;
typedef signed short   MTD_S16;

typedef unsigned long long  MTD_U64;

typedef enum {
	MTD_FALSE = 0,
	MTD_TRUE  = 1
} MTD_BOOL;

#define MTD_CONVERT_BOOL_TO_UINT(boolVar, uintVar)  \
					{(boolVar) ? (uintVar = 1) : (uintVar = 0); }
#define MTD_CONVERT_UINT_TO_BOOL(uintVar, boolVar)  \
					{(uintVar) ? (boolVar = MTD_TRUE) : (boolVar = MTD_FALSE); }
#define MTD_GET_BOOL_AS_BIT(boolVar) ((boolVar) ? 1 : 0)
#define MTD_GET_BIT_AS_BOOL(uintVar) ((uintVar) ? MTD_TRUE : MTD_FALSE)

typedef void	 (*MTD_VOIDFUNCPTR) (void); /* ptr to function returning void */
typedef MTD_U32  (*MTD_INTFUNCPTR)  (void); /* ptr to function returning int  */

typedef MTD_U32 MTD_STATUS;

/* Defines for semaphore support */
typedef MTD_U32 MTD_SEM;

typedef enum {
	MTD_SEM_EMPTY,
	MTD_SEM_FULL
} MTD_SEM_BEGIN_STATE;

typedef MTD_SEM (*FMTD_SEM_CREATE)(MTD_SEM_BEGIN_STATE state);
typedef MTD_STATUS (*FMTD_SEM_DELETE)(MTD_SEM semId);
typedef MTD_STATUS (*FMTD_SEM_TAKE)(MTD_SEM semId, MTD_U32 timOut);
typedef MTD_STATUS (*FMTD_SEM_GIVE)(MTD_SEM semId);

/* Defines for mtdLoadDriver() mtdUnloadDriver() and all API functions which need MTD_DEV */
typedef struct _MTD_DEV MTD_DEV;
typedef MTD_DEV * MTD_DEV_PTR;

typedef MTD_STATUS (*FMTD_READ_MDIO)(
						MTD_DEV * dev,
						MTD_U16 port,
						MTD_U16 mmd,
						MTD_U16 reg,
						MTD_U16 * value);
typedef MTD_STATUS (*FMTD_WRITE_MDIO)(
						MTD_DEV * dev,
						MTD_U16 port,
						MTD_U16 mmd,
						MTD_U16 reg,
						MTD_U16 value);




/* MTD_DEVICE_ID format:  */
/* Bits 15:13 reserved */
/* Bit 12: 1-> E20X0 device with max speed of 5G and no fiber interface */
/* Bit 11: 1-> Macsec Capable (Macsec/PTP module included */
/* Bit  10: 1-> Copper Capable (T unit interface included) */
/* Bits 9:4 0x18 -> X32X0 base, 0x1A 0x33X0 base */
/* Bits 3:0 revision/number of ports indication, see list */
/* Following defines are for building MTD_DEVICE_ID */
#define MTD_E20X0_DEVICE (1<<12)   /* whether this is an E20X0 device group */
#define MTD_MACSEC_CAPABLE (1<<11) /* whether the device has a Macsec/PTP module */
#define MTD_COPPER_CAPABLE (1<<10) /* whether the device has a copper (T unit) module */
#define MTD_X32X0_BASE (0x18<<4)   /* whether the device uses X32X0 firmware base */
#define MTD_X33X0_BASE (0x1A<<4)   /* whether the device uses X33X0 firmware base */

/* Following macros are to test MTD_DEVICE_ID for various features */
#define MTD_IS_E20X0_DEVICE(mTdrevId) ((MTD_BOOL)(mTdrevId & MTD_E20X0_DEVICE))
#define MTD_IS_MACSEC_CAPABLE(mTdrevId) ((MTD_BOOL)(mTdrevId & MTD_MACSEC_CAPABLE))
#define MTD_IS_COPPER_CAPABLE(mTdrevId) ((MTD_BOOL)(mTdrevId & MTD_COPPER_CAPABLE))
#define MTD_IS_X32X0_BASE(mTdrevId) ((MTD_BOOL)((mTdrevId & (0x3F<<4)) == MTD_X32X0_BASE))
#define MTD_IS_X33X0_BASE(mTdrevId) ((MTD_BOOL)((mTdrevId & (0x3F<<4)) == MTD_X33X0_BASE))

#define MTD_X33X0BASE_SINGLE_PORTA0 0xA
#define MTD_X33X0BASE_DUAL_PORTA0   0x6
#define MTD_X33X0BASE_QUAD_PORTA0   0x2

/* WARNING: If you add/modify this list, you must also modify mtdIsPhyRevisionValid() */
typedef enum {
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

	MTD_REV_3310P_Z1 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x8), /* 88X33X0 Z1 not supported starting with version 1.2 of API */
	MTD_REV_3320P_Z1 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x4),
	MTD_REV_3340P_Z1 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x0),
	MTD_REV_3310_Z1 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x8),
	MTD_REV_3320_Z1 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x4),
	MTD_REV_3340_Z1 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x0),

	MTD_REV_3310P_Z2 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x9),  /* 88X33X0 Z2 not supported starting with version 1.2 of API */
	MTD_REV_3320P_Z2 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x5),
	MTD_REV_3340P_Z2 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x1),
	MTD_REV_3310_Z2 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x9),
	MTD_REV_3320_Z2 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x5),
	MTD_REV_3340_Z2 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x1),

	MTD_REV_E2010P_Z2 = (MTD_E20X0_DEVICE | MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x9), /* E20X0 Z2 not supported starting with version 1.2 of API */
	MTD_REV_E2020P_Z2 = (MTD_E20X0_DEVICE | MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x5),
	MTD_REV_E2040P_Z2 = (MTD_E20X0_DEVICE | MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x1),
	MTD_REV_E2010_Z2 = (MTD_E20X0_DEVICE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x9),
	MTD_REV_E2020_Z2 = (MTD_E20X0_DEVICE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x5),
	MTD_REV_E2040_Z2 = (MTD_E20X0_DEVICE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | 0x1),


	MTD_REV_3310P_A0 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_SINGLE_PORTA0),
	MTD_REV_3320P_A0 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_DUAL_PORTA0),
	MTD_REV_3340P_A0 = (MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_QUAD_PORTA0),
	MTD_REV_3310_A0 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_SINGLE_PORTA0),
	MTD_REV_3320_A0 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_DUAL_PORTA0),
	MTD_REV_3340_A0 = (MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_QUAD_PORTA0),

	MTD_REV_E2010P_A0 = (MTD_E20X0_DEVICE | MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_SINGLE_PORTA0),
	MTD_REV_E2020P_A0 = (MTD_E20X0_DEVICE | MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_DUAL_PORTA0),
	MTD_REV_E2040P_A0 = (MTD_E20X0_DEVICE | MTD_MACSEC_CAPABLE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_QUAD_PORTA0),
	MTD_REV_E2010_A0 = (MTD_E20X0_DEVICE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_SINGLE_PORTA0),
	MTD_REV_E2020_A0 = (MTD_E20X0_DEVICE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_DUAL_PORTA0),
	MTD_REV_E2040_A0 = (MTD_E20X0_DEVICE | MTD_COPPER_CAPABLE | MTD_X33X0_BASE | MTD_X33X0BASE_QUAD_PORTA0),

	MTD_REV_2340P_A1 = (MTD_MACSEC_CAPABLE | MTD_X32X0_BASE | 0x3),
	MTD_REV_2320P_A0 = (MTD_MACSEC_CAPABLE | MTD_X32X0_BASE | 0x5),
	MTD_REV_2340_A1 = (MTD_X32X0_BASE | 0x3),
	MTD_REV_2320_A0 = (MTD_X32X0_BASE | 0x5)
} MTD_DEVICE_ID;

typedef enum {
	MTD_MSEC_REV_Z0A,
	MTD_MSEC_REV_Y0A,
	MTD_MSEC_REV_A0B,
	MTD_MSEC_REV_FPGA,
	MTD_MSEC_REV_UNKNOWN = -1
} MTD_MSEC_REV;

/* compatible for USB test */
typedef struct  _MTD_MSEC_CTRL {
	MTD_32 dev_num;	  /* indicates the device number (0 if only one) when multiple devices are present on SVB.*/
	MTD_32 port_num;	 /* Indicates which port (0 to 4) is requesting CPU */
	MTD_U16 prev_addr;   /* < Prev write address */
	MTD_U16 prev_dataL;  /* < Prev dataL value */
	MTD_MSEC_REV msec_rev;  /* revision */
} MTD_MSEC_CTRL;

struct _MTD_DEV {
	MTD_DEVICE_ID   deviceId;	  /* type of device and capabilities */
	MTD_BOOL		devEnabled;	/* whether mtdLoadDriver() called successfully */
	MTD_U8		  numPorts;	  /* number of ports per device */
	MTD_U8		  thisPort;	  /* relative port number on this device starting with 0 (not MDIO address) */
	MTD_SEM		 multiAddrSem;

	FMTD_READ_MDIO  fmtdReadMdio;
	FMTD_WRITE_MDIO fmtdWriteMdio;

	FMTD_SEM_CREATE semCreate;  /* create semapore */
	FMTD_SEM_DELETE semDelete;  /* delete the semapore */
	FMTD_SEM_TAKE   semTake;	/* try to get a semapore */
	FMTD_SEM_GIVE   semGive;	/* return semaphore */

	MTD_U8		  macsecIndirectAccess; /* if MTD_TRUE use internal processor to access Macsec */
	MTD_MSEC_CTRL   msec_ctrl;  /* structure use for internal verification */

	void *appData; /* application specific data, anything the host wants to pass to the low layer */
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

/* This macro is handy for calling a function when you want to test the
   return value and return MTD_FAIL, if the function returned MTD_FAIL,
   otherwise continue */
#define ATTEMPT(xFuncToTry) do { if (xFuncToTry == MTD_FAIL) {return MTD_FAIL; }} while (0)

/* These defines are used for some registers which represent the copper
   speed as a 2-bit binary number */
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

/* this value is returned for the speed when the link status is checked and the speed has been */
/* forced to one speed but the link is up at a different speed. it indicates an error. */
#define MTD_SPEED_MISMATCH	   0x8000 /* Speed is forced to one speed, but status indicates another */


/* for macType */
#define MTD_MAC_TYPE_RXAUI_SGMII_AN_EN  (0x0) /* X32X0/X33x0, but not E20x0 */
#define MTD_MAC_TYPE_RXAUI_SGMII_AN_DIS (0x1) /* X32x0/X3340/X3320, but not X3310/E20x0 */
#define MTD_MAC_TYPE_XAUI_RATE_ADAPT	(0x1) /* X3310,E2010 only */
#define MTD_MAC_TYPE_RXAUI_RATE_ADAPT   (0x2)
#define MTD_MAC_TYPE_XAUI			   (0x3) /* X3310,E2010 only */
#define MTD_MAC_TYPE_XFI_SGMII_AN_EN	(0x4) /* XFI at 10G, X33x0/E20x0 also use 5GBASE-R/2500BASE-X */
#define MTD_MAC_TYPE_XFI_SGMII_AN_DIS   (0x5) /* XFI at 10G, X33x0/E20x0 also use 5GBASE-R/2500BASE-X */
#define MTD_MAC_TYPE_XFI_RATE_ADAPT	 (0x6)
#define MTD_MAC_TYPE_USXGMII			(0x7) /* X33x0 only */
#define MTD_MAC_LEAVE_UNCHANGED		 (0x8) /* use this option to not touch these bits */

/* for macIfSnoopSel */
#define MTD_MAC_SNOOP_FROM_NETWORK	  (0x2)
#define MTD_MAC_SNOOP_FROM_HOST		 (0x3)
#define MTD_MAC_SNOOP_OFF			   (0x0)
#define MTD_MAC_SNOOP_LEAVE_UNCHANGED   (0x4) /* use this option to not touch these bits */
/* for macLinkDownSpeed */
#define MTD_MAC_SPEED_10_MBPS			MTD_CU_SPEED_10_MBPS
#define MTD_MAC_SPEED_100_MBPS		   MTD_CU_SPEED_100_MBPS
#define MTD_MAC_SPEED_1000_MBPS		  MTD_CU_SPEED_1000_MBPS
#define MTD_MAC_SPEED_10_GBPS			MTD_CU_SPEED_10_GBPS
#define MTD_MAC_SPEED_LEAVE_UNCHANGED	(0x4)
/* X33X0/E20X0 devices only for macMaxIfSpeed */
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
#define MTD_TUNIT_IEEE_PMA_CTRL1	   MTD_T_UNIT_PMA_PMD, 0x0000 /* do not enclose in parentheses */
#define MTD_TUNIT_IEEE_PMA_DEVID2	  MTD_T_UNIT_PMA_PMD, 0x0003 /* do not enclose in parentheses */
#define MTD_TUNIT_PHY_EXT_CTRL_1	   MTD_T_UNIT_PMA_PMD, 0xC000 /* do not enclose in parentheses */
#define MTD_TUNIT_XG_EXT_STATUS		MTD_T_UNIT_PMA_PMD, 0xC001 /* do not enclose in parentheses */
#define MTD_TUNIT_BIST_STATUS_REG	  MTD_T_UNIT_PMA_PMD, 0xC00C /* do not enclose in parentheses */
#define MTD_TUNIT_PHY_REV_INFO_REG	 MTD_T_UNIT_PMA_PMD, 0xC04E /* do not enclose in parentheses */
#define MTD_BOOT_STATUS_REG			MTD_T_UNIT_PMA_PMD, 0xC050 /* do not enclose in parentheses */

#define MTD_TUNIT_IEEE_PCS_CTRL1	   MTD_T_UNIT_PCS_CU, 0x0000 /* do not enclose in parentheses */
/* control/status for serdes initialization */
#define MTD_SERDES_CTRL_STATUS MTD_T_UNIT_AN, 0x800F /* do not enclose in parentheses */
/* 88X3240/3220 C Unit Registers MMD 31 */
#define MTD_CUNIT_MODE_CONFIG		  MTD_C_UNIT_GENERAL, 0xF000 /* do not enclose in parentheses */
#define MTD_CUNIT_PORT_CTRL			MTD_C_UNIT_GENERAL, 0xF001 /* do not enclose in parentheses */

#define MTD_API_FAIL_SEM_CREATE			(0x18<<24) /*semCreate Failed. */
#define MTD_API_FAIL_SEM_DELETE			(0x19<<24) /*semDelete Failed. */
#define MTD_API_FAIL_READ_REG			(0x16<<16) /*Reading from phy reg failed. */
#define MTD_API_ERR_DEV					(0x3c<<16) /*driver struture is NULL. */
#define MTD_API_ERR_DEV_ALREADY_EXIST	(0x3e<<16) /*Device Driver already loaded. */


#define MTD_CLEAR_PAUSE	 0 /*  clears both pause bits */
#define MTD_SYM_PAUSE	   1 /*  for symmetric pause only */
#define MTD_ASYM_PAUSE	  2 /*  for asymmetric pause only */
#define MTD_SYM_ASYM_PAUSE  3 /*  for both */


/*******************************************************************************
  mtdLoadDriver

  DESCRIPTION:
		Marvell X32X0  Driver Initialization Routine.
		This is the first routine that needs be called by system software.
		It takes parameters from system software, and retures a pointer (*dev)
		to a data structure which includes infomation related to this Marvell Phy
		device. This pointer (*dev) is then used for all the API functions.
		The following is the job performed by this routine:
			1. store MDIO read/write function into the given MTD_DEV structure
			2. run any device specific initialization routine
			3. create semaphore if required
			4. Initialize the deviceId


  INPUTS:
	readMdio - pointer to host's function to do MDIO read
	writeMdio - point to host's function to do MDIO write
	macsecIndirectAccess - MTD_TRUE to access MacSec through T-unit processor
						   MTD_FALSE to do direct register access
	semCreate - pointer to host's function to create a semaphore, NULL
				if not used
	semDelete - pointer to host's function to create a semaphore, NULL
				if not used
	semTake - pointer to host's function to take a semaphore, NULL
			  if not used
	semGive - pointer to host's function to give a semaphore, NULL
			  if not used
	anyPort - port address of any port for this device

  OUTPUTS:
		dev  - pointer to holds device information to be used for each API call.

  RETURNS:
		MTD_OK			   - on success
		MTD_FAIL			 - on error

  COMMENTS:
		mtdUnloadDriver is also provided to do driver cleanup.

		An MTD_DEV is required for each type of X32X0 device in the system. For
		example, if there are 16 ports of X3240 and 4 ports of X3220,
		two MTD_DEV are required, and one call to mtdLoadDriver() must
		be made with one of the X3240 ports, and one with one of the X3220
		ports.
*******************************************************************************/
MTD_STATUS mtdLoadDriver
(
	IN FMTD_READ_MDIO	 readMdio,
	IN FMTD_WRITE_MDIO	writeMdio,
	IN MTD_BOOL		   macsecIndirectAccess,
	IN FMTD_SEM_CREATE	semCreate,
	IN FMTD_SEM_DELETE	semDelete,
	IN FMTD_SEM_TAKE	  semTake,
	IN FMTD_SEM_GIVE	  semGive,
	IN MTD_U16			anyPort,
	OUT MTD_DEV		  * dev
);

/*******************************************************************************
* mtdUnloadDriver
*
* DESCRIPTION:
*	   This function frees semaphore created by Marvell X2*** Driver,
*	   disables Device interrupt, and clears MTD_DEV structure.
*
* INPUTS:
*	   None.
*
* OUTPUTS:
*	   None.
*
* RETURNS:
*	   MTD_OK		   - on success
*	   MTD_FAIL		 - on error
*
* COMMENTS:
*	   1.  This function should be called only after successful execution of
*		   mtdLoadDriver().
*
*******************************************************************************/
MTD_STATUS mtdUnloadDriver
(
	IN MTD_DEV * dev
);

/******************************************************************************
MTD_STATUS mtdHwXmdioWrite
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 dev,
	IN MTD_U16 reg,
	IN MTD_U16 value
);

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31
	dev - MMD device address, 0-31
	reg - MMD register address
	value - data to write

 Outputs:
	None

 Returns:
	MTD_OK - wrote successfully
	MTD_FAIL - an error occurred

 Description:
	Writes a 16-bit word to the MDIO
	Address is in format X.Y.Z, where X selects the MDIO port (0-31), Y selects
	the MMD/Device (0-31), and Z selects the register.

 Side effects:
	None

 Notes/Warnings:
	None

******************************************************************************/
MTD_STATUS mtdHwXmdioWrite
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 dev,
	IN MTD_U16 reg,
	IN MTD_U16 value
);

/******************************************************************************
 MTD_STATUS mtdHwXmdioRead
 (
	 IN MTD_DEV_PTR devPtr,
	 IN MTD_U16 port,
	 IN MTD_U16 dev,
	 IN MTD_U16 reg,
	 OUT MTD_U16 *data
 );

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31
	dev - MMD device address, 0-31
	reg - MMD register address

 Outputs:
	data - Returns 16 bit word from the MDIO

 Returns:
	MTD_OK - read successful
	MTD_FAIL - read was unsuccessful

 Description:
	Reads a 16-bit word from the MDIO
	Address is in format X.Y.Z, where X selects the MDIO port (0-31), Y selects the
	MMD/Device (0-31), and Z selects the register.

 Side effects:
	None

 Notes/Warnings:
	None

******************************************************************************/
MTD_STATUS mtdHwXmdioRead
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 dev,
	IN MTD_U16 reg,
	OUT MTD_U16 *data
);


/*******************************************************************************
  MTD_STATUS mtdHwGetPhyRegField
  (
	  IN  MTD_DEV_PTR devPtr,
	  IN  MTD_U16	  port,
	  IN  MTD_U16	  dev,
	  IN  MTD_U16	  regAddr,
	  IN  MTD_U8	   fieldOffset,
	  IN  MTD_U8	   fieldLength,
	  OUT MTD_U16	  *data
  );

  Inputs:
		devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
		port	   - The port number, 0-31
		dev		- The MMD device, 0-31
		regAddr	- The register's address
		fieldOffset - The field start bit index. (0 - 15)
		fieldLength - Number of bits to read

  Outputs:
		data		- The read register field

  Returns:
		MTD_OK on success, or
		MTD_FAIL  - on error

  Description:
		This function reads a specified field from a port's phy register.
		It first reads the register, then returns the specified bit
		field from what was read.

  Side effects:
	 None

  Notes/Warnings:
		The sum of fieldOffset & fieldLength parameters must be smaller-
		equal to 16

		Reading a register with latched bits may clear the latched bits.
		Use with caution for registers with latched bits.

		To operate on several bits within a register which has latched bits
		before reading the register again, first read the register with
		mtdHwXmdioRead() to get the register value, then operate on the
		register data repeatedly using mtdHwGetRegFieldFromWord() to
		take apart the bit fields without re-reading the register again.

		This approach should also be used to reduce IO to the PHY when reading
		multiple bit fields (do a single read, then grab different fields
		from the register by using mtdHwGetRegFieldFromWord() repeatedly).

*******************************************************************************/
MTD_STATUS mtdHwGetPhyRegField
(
	IN  MTD_DEV_PTR devPtr,
	IN  MTD_U16	  port,
	IN  MTD_U16	  dev,
	IN  MTD_U16	  regAddr,
	IN  MTD_U8	   fieldOffset,
	IN  MTD_U8	   fieldLength,
	OUT MTD_U16	  *data
);

/*******************************************************************************
  MTD_STATUS mtdHwSetPhyRegField
  (
	  IN MTD_DEV_PTR devPtr,
	  IN MTD_U16	  port,
	  IN MTD_U16	  dev,
	  IN MTD_U16	  regAddr,
	  IN MTD_U8	   fieldOffset,
	  IN MTD_U8	   fieldLength,
	  IN MTD_U16	  data
  );

  Inputs:
		devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
		port	   - The port number, 0-31
		dev		- The MMD device, 0-31
		regAddr	-  The register's address
		fieldOffset - The field start bit index. (0 - 15)
		fieldLength - Number of bits to write
		data		- Data to be written.

  Outputs:
		None.

  Returns:
		MTD_OK on success, or
		MTD_FAIL  - on error

  Description:
		This function writes to specified field in a port's phy register.

  Side effects:
	 None

  Notes/Warnings:
		The sum of fieldOffset & fieldLength parameters must be smaller-
		equal to 16.

*******************************************************************************/
MTD_STATUS mtdHwSetPhyRegField
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16	  port,
	IN MTD_U16	  dev,
	IN MTD_U16	  regAddr,
	IN MTD_U8	   fieldOffset,
	IN MTD_U8	   fieldLength,
	IN MTD_U16	  data
);

/*******************************************************************************
  MTD_STATUS mtdHwGetRegFieldFromWord
  (
	  IN  MTD_U16	  regData,
	  IN  MTD_U8	   fieldOffset,
	  IN  MTD_U8	   fieldLength,
	  OUT MTD_U16	  *data
  );

  Inputs:
		regData	- The data previously read from the register
		fieldOffset - The field start bit index. (0 - 15)
		fieldLength - Number of bits to read

  Outputs:
		data		- The data from the associated bit field

  Returns:
		MTD_OK always

  Description:
		This function grabs a value from a bitfield within a word. It could
		be used to get the value of a bitfield within a word which was previously
		read from the PHY.

  Side effects:
	 None

  Notes/Warnings:
		The sum of fieldOffset & fieldLength parameters must be smaller-
		equal to 16

		This register acts on data passed in. It does no hardware access.

		This function is useful if you want to do 1 register access and then
		get different bit fields without doing another register access either
		because there are latched bits in the register to avoid another read,
		or to keep hardware IO down to improve performance/throughput.

		Example:

		MTD_U16 aword, nibble1, nibble2;

		mtdHwXmdioRead(devPtr,0,MTD_TUNIT_IEEE_PCS_CTRL1,&aword); // Read 3.0 from port 0
		mtdHwGetRegFieldFromWord(aword,0,4,&nibble1); // grab first nibble
		mtdHwGetRegFieldFromWord(aword,4,4,&nibble2); // grab second nibble

*******************************************************************************/
MTD_STATUS mtdHwGetRegFieldFromWord
(
	IN  MTD_U16	  regData,
	IN  MTD_U8	   fieldOffset,
	IN  MTD_U8	   fieldLength,
	OUT MTD_U16	  *data
);

/*******************************************************************************
  MTD_STATUS mtdHwSetRegFieldToWord
  (
	  IN  MTD_U16	  regData,
	  IN  MTD_U16	  bitFieldData,
	  IN  MTD_U8	   fieldOffset,
	  IN  MTD_U8	   fieldLength,
	  OUT MTD_U16	  *data
  );

  Inputs:
		regData - original word to modify
		bitFieldData   - The data to set the register field to
					 (must be <= largest value for that bit field,
					  no range checking is done by this function)
		fieldOffset - The field start bit index. (0 - 15)
		fieldLength - Number of bits to write to regData

  Outputs:
		This function grabs a value from a bitfield within a word. It could
		be used to get the value of a bitfield within a word which was previously
		read from the PHY.

  Side effects:
	 None

  Notes/Warnings:
		The sum of fieldOffset & fieldLength parameters must be smaller-
		equal to 16

		This register acts on data passed in. It does no hardware access.

		This function is useful if you want to do 1 register access and then
		get different bit fields without doing another register access either
		because there are latched bits in the register to avoid another read,
		or to keep hardware IO down to improve performance/throughput.

		Example:

		MTD_U16 aword, nibble1, nibble2;

		mtdHwXmdioRead(devPtr,0,MTD_TUNIT_IEEE_PCS_CTRL1,&aword); // Read 3.0 from port 0
		mtdHwGetRegFieldFromWord(aword,0,4,&nibble1); // grab first nibble
		mtdHwGetRegFieldFromWord(aword,4,4,&nibble2); // grab second nibble

*******************************************************************************/
MTD_STATUS mtdHwGetRegFieldFromWord
(
	IN  MTD_U16	  regData,
	IN  MTD_U8	   fieldOffset,
	IN  MTD_U8	   fieldLength,
	OUT MTD_U16	  *data
);

/*******************************************************************************
  MTD_STATUS mtdHwSetRegFieldToWord
  (
	  IN  MTD_U16	  regData,
	  IN  MTD_U16	  bitFieldData,
	  IN  MTD_U8	   fieldOffset,
	  IN  MTD_U8	   fieldLength,
	  OUT MTD_U16	  *data
  );

  Inputs:
		regData - original word to modify
		bitFieldData   - The data to set the register field to
					 (must be <= largest value for that bit field,
					  no range checking is done by this function)
		fieldOffset - The field start bit index. (0 - 15)
		fieldLength - Number of bits to write to regData

  Outputs:
		data		- The new/modified regData with the bitfield changed

  Returns:
		MTD_OK always

  Description:
		This function write a value to a bitfield within a word.

  Side effects:
	 None

  Notes/Warnings:
		The sum of fieldOffset & fieldLength parameters must be smaller-
		equal to 16

		This register acts on data passed in. It does no hardware access.

		This function is useful to reduce IO if several bit fields of a register
		that has been read is to be changed before writing it back.

		MTD_U16 aword;

		mtdHwXmdioRead(devPtr,0,MTD_TUNIT_IEEE_PCS_CTRL1,&aword); // Read 3.0 from port 0
		mtdHwSetRegFieldToWord(aword,2,0,4,&aword); // Change first nibble to 2
		mtdHwSetRegFieldToWord(aword,3,4,4,&aword); // Change second nibble to 3

*******************************************************************************/
MTD_STATUS mtdHwSetRegFieldToWord
(
	IN  MTD_U16	  regData,
	IN  MTD_U16	  bitFieldData,
	IN  MTD_U8	   fieldOffset,
	IN  MTD_U8	   fieldLength,
	OUT MTD_U16	  *data
);


/******************************************************************************
MTD_STATUS mtdWait
(
	IN MTD_DEV_PTR devPtr,
	IN unsigned x
);

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	x - number of milliseconds to wait

 Outputs:
	None

 Returns:
	MTD_OK if wait was successful, MTD_FAIL otherwise

 Description:
	Waits X milliseconds

 Side effects:
	None

 Notes/Warnings:
	None

******************************************************************************/
MTD_STATUS mtdWait
(
	IN MTD_UINT x
);

/******************************************************************************
MTD_STATUS mtdSoftwareReset
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 timeoutMs
);

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31
	timeoutMs - 0 will not wait for reset to complete, otherwise
				waits 'timeout' milliseconds for reset to complete

 Outputs:
	None

 Returns:
	MTD_OK or MTD_FAIL if IO error or timed out

 Description:
	Issues a software reset (1.0.15 <= 1) command. Resets firmware and
	hardware state machines and returns non-retain bits to their hardware
	reset values and retain bits keep their values through the reset.

	If timeoutMs is 0, returns immediately. If timeoutMs is non-zero,
	waits up to 'timeoutMs' milliseconds looking for the reset to complete
	before returning. Returns MTD_FAIL if times out.

 Side effects:
	All "retain" bits keep their values through this reset. Non-"retain"-type
	bits are returned to their hardware reset values following this reset.
	See the Datasheet for a list of retain bits.

 Notes/Warnings:
	Use mtdIsPhyReadyAfterReset() to see if the software reset is complete
	before issuing any other MDIO commands following this reset or pass
	in non-zero timeoutMs to have this function do it for you.

	This is a T unit software reset only. It may only be issued if the T
	unit is ready (1.0.15 is 0) and the T unit is not in low power mode.

******************************************************************************/
MTD_STATUS mtdSoftwareReset
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 timeoutMs
);

MTD_STATUS mtdHardwareReset
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 timeoutMs
);

/******************************************************************************
 MTD_STATUS mtdSetMacInterfaceControl
 (
	 IN MTD_DEV_PTR devPtr,
	 IN MTD_U16 port,
	 IN MTD_U16 macType,
	 IN MTD_BOOL macIfPowerDown,
	 IN MTD_U16 macIfSnoopSel,
	 IN MTD_U16 macIfActiveLaneSelect,
	 IN MTD_U16 macLinkDownSpeed,
	 IN MTD_U16 macMaxIfSpeed,  - 33X0/E20X0 devices only -
	 IN MTD_BOOL doSwReset,
	 IN MTD_BOOL rerunSerdesInitialization
 );

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - port number, 0-31
	macType - the type of MAC interface being used (the hardware interface). One of the following:
		MTD_MAC_TYPE_RXAUI_SGMII_AN_EN - selects RXAUI with SGMII AN enabled
		MTD_MAC_TYPE_RXAUI_SGMII_AN_DIS - selects RXAUI with SGMII AN disabled (not valid on X3310)
		MTD_MAC_TYPE_XAUI_RATE_ADAPT - selects XAUI with rate matching (only valid on X3310)
		MTD_MAC_TYPE_RXAUI_RATE_ADAPT  - selects RXAUI with rate matching
		MTD_MAC_TYPE_XAUI - selects XAUI (only valid on X3310)
		MTD_MAC_TYPE_XFI_SGMII_AN_EN - selects XFI with SGMII AN enabled
		MTD_MAC_TYPE_XFI_SGMII_AN_DIS - selects XFI with SGMII AN disabled
		MTD_MAC_TYPE_XFI_RATE_ADAPT  - selects XFI with rate matching
		MTD_MAC_TYPE_USXGMII - selects USXGMII
		MTD_MAC_LEAVE_UNCHANGED - option to leave this parameter unchanged/as it is
	macIfPowerDown - MTD_TRUE if the host interface is always to be powered up
					 MTD_FALSE if the host interface can be powered down under
						 certain circumstances (see datasheet)
	macIfSnoopSel - If snooping is requested on the other lane, selects the source
		MTD_MAC_SNOOP_FROM_NETWORK - source of snooped data is to come from the network
		MTD_MAC_SNOOP_FROM_HOST - source of snooped data is to come from the host
		MTD_MAC_SNOOP_OFF - snooping is to be turned off
		MTD_MAC_SNOOP_LEAVE_UNCHANGED - option to leave this parameter unchanged/as it is
	macIfActiveLaneSelect - For redundant host mode, this selects the active lane. 0 or 1
		only. 0 selects 0 as the active lane and 1 as the standby. 1 selects the other way.
	macLinkDownSpeed - The speed the mac interface should run when the media side is
		link down. One of the following:
			MTD_MAC_SPEED_10_MBPS
			MTD_MAC_SPEED_100_MBPS
			MTD_MAC_SPEED_1000_MBPS
			MTD_MAC_SPEED_10_GBPS
			MTD_MAC_SPEED_LEAVE_UNCHANGED
	macMaxIfSpeed - For X33X0/E20X0 devices only. Can be used to limit the Mac interface speed
			MTD_MAX_MAC_SPEED_10G
			MTD_MAX_MAC_SPEED_5G
			MTD_MAX_MAC_SPEED_2P5G
			MTD_MAX_MAC_SPEED_LEAVE_UNCHANGED
			MTD_MAX_MAC_SPEED_NOT_APPLICABLE (for 32X0 devices pass this)
	doSwReset - MTD_TRUE if a software reset (31.F001.15) should be done after these changes
		have been made, or MTD_FALSE otherwise. See note below.
	rerunSerdesInitialization - MTD_TRUE if any parameter that is likely to change the speed
		of the serdes interface was performed like macLinkDownSpeed or macType will attempt
		to reset the H unit serdes (this needs to be done AFTER the soft reset, so if doSwReset
		is passed as MTD_FALSE, host must later call
		mtdRerunSerdesAutoInitializationUseAutoMode() eventually to re-init the serdes).


 Outputs:
	None

 Returns:
	MTD_OK or MTD_FAIL if a bad parameter was passed, or an IO error occurs.

 Description:
	Changes the above parameters as indicated in 31.F000 and 31.F001 and
	optionally does a software reset afterwards for those bits which require a
	software reset to take effect.

 Side effects:
	None

 Notes/Warnings:
	These bits are actually in the C unit, but pertain to the host interface
	control so the API called was placed here.

	Changes to the MAC type (31.F001.2:0) do not take effect until a software
	reset is performed on the port.

	Changes to macLinkDownSpeed (31.F001.7:6) require 2 software resets to
	take effect. This function will do 2 resets if doSwReset is MTD_TRUE
	and macLinkDownSpeed is being changed.

	IMPORTANT: the readback reads back the last written value following
	a software reset. Writes followed by reads without an intervening
	software reset will read back the old bit value for all those bits
	requiring a software.

	Because of this, read-modify-writes to different bitfields must have an
	intervening software reset to pick up the latest value before doing
	another read-modify-write to the register, otherwise the bitfield
	may lose the value.

	Suggest always setting doSwReset to MTD_TRUE to avoid problems of
	possibly losing changes.

******************************************************************************/
MTD_STATUS mtdSetMacInterfaceControl
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 macType,
	IN MTD_BOOL macIfPowerDown,
	IN MTD_U16 macIfSnoopSel,
	IN MTD_U16 macIfActiveLaneSelect,
	IN MTD_U16 macLinkDownSpeed,
	IN MTD_U16 macMaxIfSpeed, /* 33X0/E20X0 devices only */
	IN MTD_BOOL doSwReset,
	IN MTD_BOOL rerunSerdesInitialization
);

/******************************************************************************
 MTD_STATUS mtdEnableSpeeds
 (
	 IN MTD_DEV_PTR devPtr,
	 IN MTD_U16 port,
	 IN MTD_U16 speed_bits,
	 IN MTD_BOOL anRestart
 );

 Inputs: 2
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31
	speed_bits - speeds to be advertised during auto-negotiation. One or more
				 of the following (bits logically OR together):
				MTD_ADV_NONE (no bits set)
				MTD_SPEED_10M_HD
				MTD_SPEED_10M_FD
				MTD_SPEED_100M_HD
				MTD_SPEED_100M_FD
				MTD_SPEED_1GIG_HD
				MTD_SPEED_1GIG_FD
				MTD_SPEED_10GIG_FD
				MTD_SPEED_2P5GIG_FD (88X33X0/88E20X0 family only)
				MTD_SPEED_5GIG_FD (88X33X0/88E20X0 family only)
				MTD_SPEED_ALL
				MTD_SPEED_ALL_33X0 (88X33X0/88E20X0 family only)

	anRestart - this takes the value of MTD_TRUE or MTD_FALSE and indicates
				if auto-negotiation should be restarted following the speed
				enable change. If this is MTD_FALSE, the change will not
				take effect until AN is restarted in some other way (link
				drop, toggle low power, toggle AN enable, toggle soft reset).

				If this is MTD_TRUE and AN has been disabled, it will be
				enabled before being restarted.

 Outputs:
	None

 Returns:
	MTD_OK if action was successfully taken, MTD_FAIL if not. Also returns
	MTD_FAIL if try to force the speed or try to advertise a speed not supported
	on this PHY.

 Description:
	This function allows the user to select the speeds to be advertised to the
	link partner during auto-negotiation.

	First, this function enables auto-negotiation and XNPs by calling
	mtdUndoForcedSpeed().

	The function takes in a 16 bit value and sets the appropriate bits in MMD
	7 to have those speeds advertised.

	The function also checks if the input parameter is MTD_ADV_NONE, in which case
	all speeds are disabled effectively disabling the phy from training
	(but not disabling auto-negotiation).

	If anRestart is MTD_TRUE, an auto-negotiation restart is issued making the change
	immediate. If anRestart is MTD_FALSE, the change will not take effect until the
	next time auto-negotiation restarts.

 Side effects:
	Setting speed in 1.0 to 10GBASE-T has the effect of enabling XNPs in 7.0 and
	enabling auto-negotiation in 7.0.

 Notes/Warnings:

	Example:
	To train the highest speed matching the far end among
	either 1000BASE-T Full-duplex or 10GBASE-T:
	mtdEnableSpeeds(devPtr,port,MTD_SPEED_1GIG_FD | MTD_SPEED_10GIG_FD, MTD_TRUE);

	To allow only 10GBASE-T to train:
	mtdEnableSpeeds(devPtr,port,MTD_SPEED_10GIG_FD, MTD_TRUE);

	To disable all speeds (but AN will still be running, just advertising no
	speeds)
	mtdEnableSpeeds(devPtr,port,MTD_ADV_NONE, MTD_TRUE);

	This function is not to be used to disable autonegotiation and force the speed
	to 10BASE-T or 100BASE-TX. Use mtdForceSpeed() for this.

	88X33X0 Z1/Z2 and E20X0 Z2 are not supported starting with API version 1.2.
	Version 1.2 and later require A0 revision of these devices.

******************************************************************************/
MTD_STATUS mtdEnableSpeeds
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 speed_bits,
	IN MTD_BOOL anRestart
);

MTD_STATUS mtdGetAutonegSpeedDuplexResolution
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_U16 *speedResolution
);

MTD_STATUS mtdAutonegIsSpeedDuplexResolutionDone
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_BOOL *anSpeedResolutionDone
);

/******************************************************************************
MTD_STATUS mtdIsBaseTUp(
	IN MTD_DEV_PTR devPtr,
	MTD_U16 port,
	MTD_U16 *speed,
	MTD_BOOL *linkUp
);


 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31

 Outputs:
	speed - which speed was resolved or will be resolved by auto-negotiation,
			one of following
			 MTD_ADV_NONE - autonegotiation is in progress or disabled
			 MTD_SPEED_10M_HD - speed is resolved to 10BT half-duplex
			 MTD_SPEED_10M_FD - speed is resolved to 10BT full-duplex
			 MTD_SPEED_100M_HD - speed is resolved to 100BASE-TX half-duplex
			 MTD_SPEED_100M_FD - speed is resolved to 100BASE-TX full-duplex
			 MTD_SPEED_1GIG_HD - speed is resolved to 1000BASE-T half-duplex
			 MTD_SPEED_1GIG_FD - speed is resolved to 1000BASE-T full-duplex
			 MTD_SPEED_10GIG_FD - speed is resolved to 10GBASE-T
			 MTD_SPEED_2P5GIG_FD - speed is resolved to 2.5GBASE-T (88X33X0/88E20X0 family only)
			 MTD_SPEED_5GIG_FD - speed is resolved to 5GBASE-T (88X33X0/88E20X0 family only)

			 MTD_SPEED_10M_HD_AN_DIS - speed forced to 10BT half-duplex
			 MTD_SPEED_10M_FD_AN_DIS - speed forced to 10BT full-duplex
			 MTD_SPEED_100M_HD_AN_DIS - speed forced to 100BT half-duplex
			 MTD_SPEED_100M_FD_AN_DIS - speed forced to 100BT full-duplex
			 MTD_SPEED_MISMATCH - speed is forced to one of above speeds, but
								  indicated up at a different speed (this is an error)

	linkUp - MTD_TRUE if the copper link is up at the speed above
			 MTD_FALSE if the link is down or autonegotiation is in progress or
			 there's an error

 Returns:
	MTD_OK, if query was successful, MTD_FAIL if query was unsuccessful or if there
	was an error such as MTD_SPEED_MISMATCH. MTD_OK indicates speed and link
	status are valid. MTD_FAIL indicates speed and link status are invalid.

 Description:

	First checks if the speed is forced to 10BT or 100BTX, and if it is returns
	one of the following combinations

	Speed Forced Value		   Link Status	Status
	------------------		   -----------	------
	MTD_SPEED_10M_HD_AN_DIS	  MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid
	MTD_SPEED_10M_FD_AN_DIS	  MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid
	MTD_SPEED_100M_HD_AN_DIS	 MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid
	MTD_SPEED_100M_FD_AN_DIS	 MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid
	any of the above with		MTD_FALSE	  MTD_FAIL - query failed

	MTD_SPEED_MISMATCH		   MTD_FALSE	  MTD_FAIL - link at different speed than forced value
												This is an error. Link may be up at wrong speed
												or training at wrong speed.

	If speed is not forced, then checks if autonegotation is resolved or in-progress and
	returns one of the following combinations:

	Autonegotation Result		Link Status	Status
	---------------------		-----------	------
	MTD_ADV_NONE				 MTD_FALSE	  MTD_OK - autonegotiation is in progress (link is down)
	MTD_SPEED_10M_HD			 MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid
	MTD_SPEED_10M_FD			 MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid
	MTD_SPEED_100M_HD			MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid
	MTD_SPEED_100M_FD			MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid
	MTD_SPEED_1GIG_HD			MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid
	MTD_SPEED_1GIG_FD			MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid
	MTD_SPEED_10GIG_FD		   MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid
	MTD_SPEED_2P5GIG_FD		  MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid
	MTD_SPEED_5GIG_FD			MTD_TRUE/FALSE MTD_OK - query successful, link status/speed valid

	any of the above with		MTD_FALSE	  MTD_FAIL - query failed

 Side effects:
	None

 Notes/Warnings:
	Gives current real-time status of the copper interface.
******************************************************************************/
MTD_STATUS mtdIsBaseTUp(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_U16 *speed,
	OUT MTD_BOOL *linkUp
);



/****************************************************************************/
/*******************************************************************
  Firmware Version
 *******************************************************************/
/****************************************************************************/

/******************************************************************************
MTD_STATUS mtdGetFirmwareVersion
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_U8 *major,
	OUT MTD_U8 *minor,
	OUT MTD_U8 *inc,
	OUT MTD_U8 *test
);

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31

 Outputs:
	major - major version, X.Y.Z.W, the X
	minor - minor version, X.Y.Z.W, the Y
	inc   - incremental version, X.Y.Z.W, the Z
	test  - test version, X.Y.Z.W, the W, should be 0 for released code,
			non-zero indicates this is a non-released code

 Returns:
	MTD_FAIL if version can't be queried or firmware is in download mode
	(meaning all version numbers are 0), MTD_OK otherwise

 Description:
	This function reads the firmware version number and stores it in the
	pointers passed in by the user.

 Side effects:
	None

 Notes/Warnings:
	This function returns all 0's if the phy is in download mode. The phy
	application code must have started and be ready before issuing this
	command.

******************************************************************************/
MTD_STATUS mtdGetFirmwareVersion
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_U8 *major,
	OUT MTD_U8 *minor,
	OUT MTD_U8 *inc,
	OUT MTD_U8 *test
);

/******************************************************************************
MTD_STATUS mtdSetPauseAdvertisement
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U8 pauseType,
	IN MTD_BOOL anRestart
);

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31
	pauseType - one of the following:
				MTD_SYM_PAUSE,
				MTD_ASYM_PAUSE,
				MTD_SYM_ASYM_PAUSE or
				MTD_CLEAR_PAUSE.
	anRestart - this takes the value of MTD_TRUE or MTD_FALSE and indicates
				if auto-negotiation should be restarted following the speed
				enable change. If this is MTD_FALSE, the change will not
				take effect until AN is restarted in some other way (link
				drop, toggle low power, toggle AN enable, toggle soft reset).

				If this is MTD_TRUE and AN has been disabled, it will be
				enabled before being restarted.

 Outputs:
	None

 Returns:
	MTD_OK or MTD_FAIL, if action was successful or failed

 Description:
	This function sets the asymmetric and symmetric pause bits in the technology
	ability field in the AN Advertisement register and optionally restarts
	auto-negotiation to use the new values. This selects what type of pause
	is to be advertised to the far end MAC during auto-negotiation. If
	auto-negotiation is restarted, it is enabled first.

	Sets entire 2-bit field to the value passed in pauseType.

	To clear both bits, pass in MTD_CLEAR_PAUSE.

 Side effects:
	None

 Notes/Warnings:
	This function will not take effect unless the auto-negotiation is restarted.

******************************************************************************/
MTD_STATUS mtdSetPauseAdvertisement
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U32 pauseType,
	IN MTD_BOOL anRestart
);


/******************************************************************************
MTD_STATUS mtdGetLPAdvertisedPause
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_U8 *pauseBits
);

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31

 Outputs:
	pauseBits - setting of link partner's pause bits based on bit definitions above in
				mtdmtdSetPauseAdvertisement()

 Returns:
	MTD_OK or MTD_FAIL, based on whether the query succeeded or failed. Returns
	MTD_FAIL and MTD_CLEAR_PAUSE if AN is not complete.

 Description:
	This function reads 7.19 (LP Base page ability) and returns the advertised
	pause setting that was received from the link partner.

 Side effects:
	None

 Notes/Warnings:
	The user must make sure auto-negotiation has completed by calling
	mtdAutonegIsCompleted() prior to calling this function.

******************************************************************************/
MTD_STATUS mtdGetLPAdvertisedPause
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_U8 *pauseBits
);



/******************************************************************************
MTD_STATUS mtdGetPhyRevision
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_DEVICE_ID *phyRev,
	OUT MTD_U8 *numPorts,
	OUT MTD_U8 *thisPort
);

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31

 Outputs:
	phyRev - revision of this chip, see MTD_DEVICE_ID definition for
			 a list of chip revisions with different options
	numPorts - number of ports on this chip (see note below)
	thisPort - this port number 0-1, or 0-4

 Returns:
	MTD_OK if query was successful, MTD_FAIL if not.

	Will return MTD_FAIL on an unsupported PHY (but will attempt to
	return correct version). See below for a list of unsupported PHYs.

 Description:
	Determines the PHY revision and returns the value in phyRev.
	See definition of MTD_DEVICE_ID for a list of available
	devices and capabilities.

 Side effects:
	None.

 Notes/Warnings:
	The phyRev can be used to determine number PHY revision,
	number of ports, which port this is from PHY perspective
	(0-based indexing 0...3 or 0..2) and what capabilities
	the PHY has.

	If phyRev is MTD_REV_UNKNOWN, numPorts and thisPort will be returned
	as 0 and the function will return MTD_FAIL.

	If T-unit is in download mode, thisPort will be returned as 0.

	88X33X0 Z1/Z2 is not supported starting with version 1.2 of API.
	E20X0 Z2 is not supported starting with version 1.2 of API.

******************************************************************************/
MTD_STATUS mtdGetPhyRevision
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_DEVICE_ID *phyRev,
	OUT MTD_U8 *numPorts,
	OUT MTD_U8 *thisPort
);



/*****************************************************************************
MTD_STATUS mtdGetForcedSpeed
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_BOOL *speedIsForced,
	OUT MTD_U16 *forcedSpeed
);

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31

 Outputs:
	speedIsForced - MTD_TRUE if an is disabled (1.0.12 == 0) AND
		the speed in 1.0.13/6 is set to 10BT or 100BT (speeds which do
		not require an to train).
	forcedSpeed - one of the following if speedIsForced is MTD_TRUE
		MTD_SPEED_10M_HD_AN_DIS  - speed forced to 10BT half-duplex
		MTD_SPEED_10M_FD_AN_DIS  - speed forced to 10BT full-duplex
		MTD_SPEED_100M_HD_AN_DIS - speed forced to 100BT half-duplex
		MTD_SPEED_100M_FD_AN_DIS - speed forced to 100BT full-duplex

 Returns:
	MTD_OK if the query was successful, or MTD_FAIL if not

 Description:
	Checks if AN is disabled (7.0.12=0) and if the speed select in
	register 1.0.13 and 1.0.6 is set to either 10BT or 100BT speeds. If
	all of this is true, returns MTD_TRUE in speedIsForced along with
	the speed/duplex setting in forcedSpeedBits. If any of this is
	false (AN is enabled, or the speed is set to 1000BT or 10GBT), then
	speedIsForced is returned MTD_FALSE and the forcedSpeedBit value
	is invalid.

 Notes/Warnings:
	None.

******************************************************************************/
MTD_STATUS mtdGetForcedSpeed
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_BOOL *speedIsForced,
	OUT MTD_U16 *forcedSpeed
);


/*****************************************************************************
MTD_STATUS mtdUndoForcedSpeed
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_BOOL anRestart
);

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31
	anRestart - this takes the value of MTD_TRUE or MTD_FALSE and indicates
				if auto-negotiation should be restarted following the speed
				enable change. If this is MTD_FALSE, the change will not
				take effect until AN is restarted in some other way (link
				drop, toggle low power, toggle AN enable, toggle soft reset).

				If this is MTD_TRUE and AN has been disabled, it will be
				enabled before being restarted.

 Outputs:
	None

 Returns:
	MTD_OK if the change was successful, or MTD_FAIL if not

 Description:
	Sets the speed bits in 1.0 back to the power-on default of 11b
	(10GBASE-T). Enables auto-negotiation.

	Does a software reset of the T unit and wait until it is complete before
	enabling AN and returning.

 Notes/Warnings:
	None.

******************************************************************************/
MTD_STATUS mtdUndoForcedSpeed
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_BOOL anRestart
);


/******************************************************************************
 MTD_STATUS mtdAutonegEnable
 (
	 IN MTD_DEV_PTR devPtr,
	 IN MTD_U16 port
 );

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31

 Outputs:
	None

 Returns:
	MTD_OK or MTD_FAIL, if action was successful or not

 Description:
	Re-enables auto-negotiation.

 Side effects:

 Notes/Warnings:
	Restart autonegation will not take effect if AN is disabled.

******************************************************************************/
MTD_STATUS mtdAutonegEnable
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port
);



/******************************************************************************
 MTD_STATUS mtdAutonegRestart
 (
	 IN MTD_DEV_PTR devPtr,
	 IN MTD_U16 port
 );

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31

 Outputs:
	None

 Returns:
	MTD_OK or MTD_FAIL, depending on if action was successful

 Description:
	Restarts auto-negotiation. The bit is self-clearing. If the link is up,
	the link will drop and auto-negotiation will start again.

 Side effects:
	None.

 Notes/Warnings:
	Restarting auto-negotiation will have no effect if auto-negotiation is
	disabled.

	This function is important as it is necessary to restart auto-negotiation
	after changing many auto-negotiation settings before the changes will take
	effect.

******************************************************************************/
MTD_STATUS mtdAutonegRestart
(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port
);



/******************************************************************************
MTD_STATUS mtdIsPhyRevisionValid
(
	IN MTD_DEVICE_ID phyRev
);


 Inputs:
	phyRev - a revision id to be checked against MTD_DEVICE_ID type

 Outputs:
	None

 Returns:
	MTD_OK if phyRev is a valid revision, MTD_FAIL otherwise

 Description:
	Takes phyRev and returns MTD_OK if it is one of the MTD_DEVICE_ID
	type, otherwise returns MTD_FAIL.

 Side effects:
	None.

 Notes/Warnings:
	None

******************************************************************************/
MTD_STATUS mtdIsPhyRevisionValid
(
	IN MTD_DEVICE_ID phyRev
);





#if C_LINKAGE
#if defined __cplusplus
}
#endif
#endif

#endif /* _TXGBE_MTD_H_ */

