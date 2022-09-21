/*
 * drivers/net/phy/mars.c
 *
 * Driver for Centec PHYs
 *
 * Author: liuht
 *
 * Copyright 2002-2018, Centec Networks (Suzhou) Co., Ltd.
 *
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/of.h>

#include <linux/io.h>
#include <asm/irq.h>
#include <linux/uaccess.h>

 /* Mask used for ID comparisons */
#define CTC_PHY_ID_MASK		0xffffffff

/* Known PHY IDs */
#define CTC_PHY_ID_MARS1S_V1        0x00782013
#define CTC_PHY_ID_MARS1S           0x01E04013
#define CTC_PHY_ID_MARS1P_V1	    0x00782011
#define CTC_PHY_ID_MARS1P 	    0x01E04011
#define CTC_PHY_IMASK                     0x12
#define CTC_PHY_IEVENT                   0x13

#define CTC_PHY_IMASK_INIT		    0x6c00
#define CTC_PHY_IMASK_CLEAR         0x0000

#define CTC_PHY_REG_SPACE             0
#define CTC_SDS_REG_SPACE             1

/* Mars page register */
#define CTC_MARS_PAGE_REG            0xa000

#define CTC_PHY_GLB_DISABLE          0
#define CTC_PHY_GLB_ENABLE           1

/* Mars specific status register */
#define CTC_MARS_SSREG		           0x11

/* Interrupt Enable Register */
#define CTC_MARS_INTR_REG             0x12
/* WOL Event Interrupt Enable */
#define CTC_MARS_WOL_INTR            BIT(6)
/* Magic Packet MAC address registers */
#define CTC_MARS_MAGIC_PACKET_MAC_ADDR2                 0xa007
#define CTC_MARS_MAGIC_PACKET_MAC_ADDR1                 0xa008
#define CTC_MARS_MAGIC_PACKET_MAC_ADDR0                 0xa009
/* Mars wol config register */
#define CTC_MARS_WOL_CFG_REG     0xa00a
/* WOL TYPE */
#define CTC_MARS_WOL_TYPE            BIT(0)
/* WOL Enable */
#define CTC_MARS_WOL_EN                BIT(3)
/* WOL Event Interrupt Enable */
#define CTC_MARS_WOL_INTR_SEL     BIT(6)
/* WOL Pulse Width */
#define CTC_MARS_WOL_WIDTH1        BIT(1)
#define CTC_MARS_WOL_WIDTH2        BIT(2)

enum mars_wol_type_e
{
    MARS_WOL_TYPE_LEVEL,
    MARS_WOL_TYPE_PULSE,
    MARS_WOL_TYPE_MAX
};
typedef enum mars_wol_type_e mars_wol_type_t;

enum mars_wol_width_e
{
    MARS_WOL_WIDTH_84MS,
    MARS_WOL_WIDTH_168MS,
    MARS_WOL_WIDTH_336MS,
    MARS_WOL_WIDTH_672MS,
    MARS_WOL_WIDTH_MAX
};
typedef enum mars_wol_width_e mars_wol_width_t;

struct mars_wol_cfg_s
{
    int enable;
    int type;
    int width;
};
typedef struct mars_wol_cfg_s mars_wol_cfg_t;

static int mars_ext_read(struct phy_device *phydev, u32 regnum)
{
	int ret;
		
	ret = phy_write(phydev, 0x1e, regnum);
	if(ret<0)
		return ret;

	return phy_read(phydev, 0x1f);
}

static int mars_ext_write(struct phy_device *phydev, u32 regnum, u16 val)
{
	int ret;
		
	ret = phy_write(phydev, 0x1e, regnum);
	if(ret<0)
		return ret;
	
	return phy_write(phydev, 0x1f, val);
}

static int mars_select_reg_space(struct phy_device *phydev, int space)
{
	int ret;
	
	if (space == CTC_PHY_REG_SPACE){
		ret = mars_ext_write(phydev, 0xa000, 0x0);
	}else{
		ret = mars_ext_write(phydev, 0xa000, 0x2);
	}
	
	return ret;
}

static int mars_config_advert(struct phy_device *phydev)
{
	int oldadv, adv, bmsr;
	int err, changed = 0;

	/* Only allow advertising what this PHY supports */
	linkmode_and(phydev->advertising, phydev->advertising, phydev->supported);

	/* Setup standard advertisement */
	adv = phy_read(phydev, MII_ADVERTISE);
	if (adv < 0)
		return adv;

	oldadv = adv;
	adv &= ~(ADVERTISE_ALL | ADVERTISE_100BASE4 | ADVERTISE_PAUSE_CAP |
		 ADVERTISE_PAUSE_ASYM);
	adv |= linkmode_adv_to_mii_adv_t(phydev->advertising);

	if (adv != oldadv) {
		err = phy_write(phydev, MII_ADVERTISE, adv);

		if (err < 0)
			return err;
		changed = 1;
	}

	bmsr = phy_read(phydev, MII_BMSR);
	if (bmsr < 0)
		return bmsr;

	/* Per 802.3-2008, Section 22.2.4.2.16 Extended status all
	 * 1000Mbits/sec capable PHYs shall have the BMSR_ESTATEN bit set to a
	 * logical 1.
	 */
	if (!(bmsr & BMSR_ESTATEN))
		return changed;

	/* Configure gigabit if it's supported */
	adv = phy_read(phydev, MII_CTRL1000);
	if (adv < 0)
		return adv;

	oldadv = adv;
	adv &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);

	if (linkmode_test_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT, phydev->supported) || linkmode_test_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT, phydev->supported)) {
		adv |= linkmode_adv_to_mii_ctrl1000_t(phydev->advertising);
	}

	if (adv != oldadv)
		changed = 1;

	err = phy_write(phydev, MII_CTRL1000, adv);
	if (err < 0)
		return err;

	return changed;
}

int mars1s_config_aneg(struct phy_device *phydev)
{
	int err, changed=0;

	if (AUTONEG_ENABLE != phydev->autoneg)
		return genphy_setup_forced(phydev);

	err = mars_config_advert(phydev);
	if (err < 0) /* error */
		return err;

	changed |= err;

	if (changed == 0) {
		/* Advertisement hasn't changed, but maybe aneg was never on to
		 * begin with?  Or maybe phy was isolated?
		 */
		int ctl = phy_read(phydev, MII_BMCR);

		if (ctl < 0)
			return ctl;

		if (!(ctl & BMCR_ANENABLE) || (ctl & BMCR_ISOLATE))
			changed = 1; /* do restart aneg */
	}

	/* Only restart aneg if we are advertising something different
	 * than we were before.
	 */
	if (changed > 0)
		return genphy_restart_aneg(phydev);

	return 0;
}

static int mars_ack_interrupt(struct phy_device *phydev)
{
	int err;

#if 1
	/* Clear the interrupts by reading the reg */
	err = phy_read(phydev, CTC_PHY_IEVENT);
#else
	err = mars_ext_read(phydev, 0xa011);
#endif
	if (err < 0)
		return err;

	return 0;
}

static int mars_config_intr(struct phy_device *phydev)
{
	int err;

#if 1
	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_write(phydev, CTC_PHY_IMASK, CTC_PHY_IMASK_INIT);
	else
		err = phy_write(phydev, CTC_PHY_IMASK, CTC_PHY_IMASK_CLEAR);
#else
	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = mars_ext_write(phydev, 0xa010, 0xffff);
	else
		err = mars_ext_write(phydev, 0xa010, 0x0000);
#endif
	return err;
}

#if 0
static int mars_set_link_timer_6_3ms(struct phy_device *phydev)
{
	int ret=0;
	
	ret = mars_select_reg_space(phydev, CTC_SDS_REG_SPACE);
	if(!ret)
		mars_ext_write(phydev, 0xa5, 0xc);
	mars_select_reg_space(phydev, CTC_PHY_REG_SPACE);

	return 0;
}
#endif

static int mars_read_status(struct phy_device *phydev)
{
	int val = 0;
	int err = 0;
	int lpa;

	/* Update the link, but return if there was an error */
	err = genphy_update_link(phydev);
	if (err)
		return err;

	phydev->speed = SPEED_10;
	phydev->duplex = DUPLEX_HALF;
	phydev->pause = 0;
	phydev->asym_pause = 0;

	val = phy_read(phydev, CTC_MARS_SSREG);
	if (val < 0)
		return val;

	lpa = phy_read(phydev, MII_LPA);
	if (lpa < 0)
		return lpa;

	if (val & 0x8000) {
		phydev->speed = SPEED_1000;
		phydev->duplex = DUPLEX_FULL;
	} else if (val & 0x4000) {
		phydev->speed = SPEED_100;
		if (val & 0x2000) 
			phydev->duplex = DUPLEX_FULL;
	} else if (val & 0x2000) {
		phydev->duplex = DUPLEX_FULL;
	}

	if (phydev->duplex == DUPLEX_FULL) {
		phydev->pause = lpa & LPA_PAUSE_CAP ? 1 : 0;
		phydev->asym_pause = lpa & LPA_PAUSE_ASYM ? 1 : 0;
	}

	return 0;
}

static int mars_set_link_timer_2_6ms(struct phy_device *phydev)
{
	int ret=0;
	int data=0;

	data = mars_ext_read(phydev,0xA003);
	data |= 0x1000;
	mars_ext_write(phydev,0xA003,data);
	mars_ext_write(phydev,0xA00C,0x07);
	mars_ext_write(phydev,0xA00D,0x40);
	mars_ext_write(phydev,0xA00E,0x20);

	ret = mars_select_reg_space(phydev, CTC_SDS_REG_SPACE);
	if(!ret)
		mars_ext_write(phydev, 0xa5, 0x5);
	mars_select_reg_space(phydev, CTC_PHY_REG_SPACE);

	return 0;
}

static int mars_wol_en_cfg(struct phy_device *phydev, mars_wol_cfg_t wol_cfg)
{
	int ret=0;
	int val=0;

	val = mars_ext_read(phydev, CTC_MARS_WOL_CFG_REG);
	if (val < 0)
		return val;

	if(wol_cfg.enable) {
		val |= CTC_MARS_WOL_EN;

		if(wol_cfg.type == MARS_WOL_TYPE_LEVEL) {
			val &= ~CTC_MARS_WOL_TYPE;
			val &= ~CTC_MARS_WOL_INTR_SEL;
		} else if(wol_cfg.type == MARS_WOL_TYPE_PULSE) {
			val |= CTC_MARS_WOL_TYPE;
			val |= CTC_MARS_WOL_INTR_SEL;

			if(wol_cfg.width == MARS_WOL_WIDTH_84MS) {
				val &= ~CTC_MARS_WOL_WIDTH1;
				val &= ~CTC_MARS_WOL_WIDTH2;
			} else if(wol_cfg.width == MARS_WOL_WIDTH_168MS) {
				val |= CTC_MARS_WOL_WIDTH1;
				val &= ~CTC_MARS_WOL_WIDTH2;
			} else if(wol_cfg.width == MARS_WOL_WIDTH_336MS) {
				val &= ~CTC_MARS_WOL_WIDTH1;
				val |= CTC_MARS_WOL_WIDTH2;
			} else if(wol_cfg.width == MARS_WOL_WIDTH_672MS) {
				val |= CTC_MARS_WOL_WIDTH1;
				val |= CTC_MARS_WOL_WIDTH2;
			}
		}
	} else {
		val &= ~CTC_MARS_WOL_EN;
		val &= ~CTC_MARS_WOL_INTR_SEL;
	}

	ret = mars_ext_write(phydev, CTC_MARS_WOL_CFG_REG, val);
	if (ret < 0)
		return ret;

	return 0;
}

static void mars_get_wol(struct phy_device *phydev, struct ethtool_wolinfo *wol)
{
	int val = 0;

	wol->supported = WAKE_MAGIC;
	wol->wolopts = 0;

	val = mars_ext_read(phydev, CTC_MARS_WOL_CFG_REG);
	if (val < 0)
		return val;

	if (val & CTC_MARS_WOL_EN)
		wol->wolopts |= WAKE_MAGIC;

	return;
}

static int mars_set_wol(struct phy_device *phydev, struct ethtool_wolinfo *wol)
{
	int ret, oldpage, val;
	mars_wol_cfg_t wol_cfg;

	memset(&wol_cfg,0,sizeof(mars_wol_cfg_t));
	oldpage = mars_ext_read(phydev, CTC_MARS_PAGE_REG);
	if (oldpage < 0)
		return oldpage;

	/* Switch to phy page */
	ret = mars_select_reg_space(phydev, CTC_PHY_REG_SPACE);
	if (ret < 0)
		return ret;

	if (wol->wolopts & WAKE_MAGIC) {
		/* Enable the WOL interrupt */
		val = phy_read(phydev, CTC_MARS_INTR_REG);
		val |= CTC_MARS_WOL_INTR;
		ret = phy_write(phydev, CTC_MARS_INTR_REG, val);
		if (ret < 0)
			return ret;

		/* Set the WOL config */
		wol_cfg.enable = CTC_PHY_GLB_ENABLE;
		wol_cfg.type= MARS_WOL_TYPE_PULSE;
		wol_cfg.width= MARS_WOL_WIDTH_672MS;
		ret = mars_wol_en_cfg(phydev, wol_cfg);
		if (ret < 0)
			return ret;

		/* Store the device address for the magic packet */
		ret = mars_ext_write(phydev, CTC_MARS_MAGIC_PACKET_MAC_ADDR2,
				((phydev->attached_dev->dev_addr[0] << 8) |
				 phydev->attached_dev->dev_addr[1]));
		if (ret < 0)
			return ret;
		ret = mars_ext_write(phydev, CTC_MARS_MAGIC_PACKET_MAC_ADDR1,
				((phydev->attached_dev->dev_addr[2] << 8) |
				 phydev->attached_dev->dev_addr[3]));
		if (ret < 0)
			return ret;
		ret = mars_ext_write(phydev, CTC_MARS_MAGIC_PACKET_MAC_ADDR0,
				((phydev->attached_dev->dev_addr[4] << 8) |
				 phydev->attached_dev->dev_addr[5]));
		if (ret < 0)
			return ret;
	} else {
		wol_cfg.enable = CTC_PHY_GLB_DISABLE;
		wol_cfg.type= MARS_WOL_TYPE_MAX;
		wol_cfg.width= MARS_WOL_WIDTH_MAX;
		ret = mars_wol_en_cfg(phydev, wol_cfg);
		if (ret < 0)
			return ret;
	}

	/* Recover to old page */
	ret = mars_select_reg_space(phydev, (oldpage & 0x1));
	if (ret < 0)
		return ret;

	return 0;
}

int mars_config_init(struct phy_device *phydev)
{
	int val;
	__ETHTOOL_DECLARE_LINK_MODE_MASK(features);
	struct ethtool_wolinfo wol;

	mars_set_link_timer_2_6ms(phydev);

	linkmode_zero(features);
	linkmode_set_bit(ETHTOOL_LINK_MODE_TP_BIT, features);
	linkmode_set_bit(ETHTOOL_LINK_MODE_MII_BIT, features);
	linkmode_set_bit(ETHTOOL_LINK_MODE_AUI_BIT, features);
	linkmode_set_bit(ETHTOOL_LINK_MODE_FIBRE_BIT, features);
	linkmode_set_bit(ETHTOOL_LINK_MODE_BNC_BIT, features);
	linkmode_set_bit(ETHTOOL_LINK_MODE_Pause_BIT, features);
	linkmode_set_bit(ETHTOOL_LINK_MODE_Asym_Pause_BIT, features);

	/* Do we support autonegotiation? */
	val = phy_read(phydev, MII_BMSR);
	if (val < 0)
		return val;

	if (val & BMSR_ANEGCAPABLE)
		linkmode_set_bit(ETHTOOL_LINK_MODE_Autoneg_BIT, features);

	if (val & BMSR_100FULL)
		linkmode_set_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT, features);
	if (val & BMSR_100HALF)
		linkmode_set_bit(ETHTOOL_LINK_MODE_100baseT_Half_BIT, features);
	if (val & BMSR_10FULL)
		linkmode_set_bit(ETHTOOL_LINK_MODE_10baseT_Full_BIT, features);
	if (val & BMSR_10HALF)
		linkmode_set_bit(ETHTOOL_LINK_MODE_10baseT_Half_BIT, features);

	if (val & BMSR_ESTATEN) {
		val = phy_read(phydev, MII_ESTATUS);
		if (val < 0)
			return val;

		if (val & ESTATUS_1000_TFULL)
			linkmode_set_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT, features);
		if (val & ESTATUS_1000_THALF)
			linkmode_set_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT, features);
	}

	linkmode_and(phydev->supported, phydev->supported, features);
	linkmode_and(phydev->advertising, phydev->advertising, features);


	wol.wolopts = 0;
	wol.supported = WAKE_MAGIC;
	wol.wolopts |= WAKE_MAGIC;
	mars_set_wol(phydev, &wol);

	return 0;
}
int mars1p_config_init(struct phy_device *phydev)
{
	
	/*RGMII clock 2.5M when link down, bit12:1->0*/
	mars_ext_write(phydev, 0xc, 0x8051);
	/*Disable sleep mode, bit15:1->0*/
	mars_ext_write(phydev, 0x27, 0x2029);
	/* disable PHY to respond to MDIO access with PHYAD0 */
    /* MMD7 8001h: bit6: 0, change value: 0x7f --> 0x3f */
	phy_write(phydev, 0xd, 0x7);
	phy_write(phydev, 0xe, 0x8001);
	phy_write(phydev, 0xd, 0x4007);
	phy_write(phydev, 0xe, 0x3f);
	
	return mars_config_init(phydev);
}

static struct phy_driver ctc_drivers[] = {
	{
		.phy_id		= CTC_PHY_ID_MARS1S,
		.phy_id_mask	= CTC_PHY_ID_MASK,
		.name		= "CTC MARS1S",
		.config_init	= mars_config_init,
		.features	       = PHY_GBIT_FEATURES,
		.config_aneg	= mars1s_config_aneg,
		.ack_interrupt = &mars_ack_interrupt,
		.config_intr     = &mars_config_intr,
		.read_status	= &mars_read_status,
		//.suspend	       = genphy_suspend,
		//.resume		= genphy_resume,
		.get_wol         = &mars_get_wol,
		.set_wol         = &mars_set_wol,
	},
	{
		.phy_id		= CTC_PHY_ID_MARS1S_V1,
		.phy_id_mask	= CTC_PHY_ID_MASK,
		.name		= "CTC MARS1S_V1",
		.config_init	= mars_config_init,
		.features	       = PHY_GBIT_FEATURES,
		.config_aneg	= mars1s_config_aneg,
		.ack_interrupt = &mars_ack_interrupt,
		.config_intr     = &mars_config_intr,
		.read_status	= &mars_read_status,
		//.suspend	       = genphy_suspend,
		//.resume		= genphy_resume,
		.get_wol         = &mars_get_wol,
		.set_wol         = &mars_set_wol,
	},	
	{
		.phy_id		= CTC_PHY_ID_MARS1P,
		.phy_id_mask	= CTC_PHY_ID_MASK,
		.name		= "CTC MARS1P",
		.config_init	= mars1p_config_init,
		.features	       = PHY_GBIT_FEATURES,
		.config_aneg	= mars1s_config_aneg,
		.ack_interrupt = &mars_ack_interrupt,
		.config_intr     = &mars_config_intr,
		.read_status	= genphy_read_status,
		//.suspend	       = genphy_suspend,
		//.resume		= genphy_resume,
	},
	{
		.phy_id		= CTC_PHY_ID_MARS1P_V1,
		.phy_id_mask	= CTC_PHY_ID_MASK,
		.name		= "CTC MARS1P_V1",
		.config_init	= mars1p_config_init,
		.features	       = PHY_GBIT_FEATURES,
		.config_aneg	= mars1s_config_aneg,
		.ack_interrupt = &mars_ack_interrupt,
		.config_intr     = &mars_config_intr,
		.read_status	= genphy_read_status,
		//.suspend	       = genphy_suspend,
		//.resume		= genphy_resume,
	},
};
module_phy_driver(ctc_drivers);

static struct mdio_device_id __maybe_unused mars_tbl[] = {
	{ CTC_PHY_ID_MARS1S, CTC_PHY_ID_MASK },
	{ CTC_PHY_ID_MARS1S_V1, CTC_PHY_ID_MASK },
	{ CTC_PHY_ID_MARS1P, CTC_PHY_ID_MASK },
	{ CTC_PHY_ID_MARS1P_V1, CTC_PHY_ID_MASK },
	{ }
};

MODULE_DEVICE_TABLE(mdio, mars_tbl);

