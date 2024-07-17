/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2021 Motorcomm Corporation. */

#include <linux/timer.h>
#include <linux/module.h>

#include "fuxi-gmac.h"
#include "fuxi-gmac-reg.h"

void fxgmac_phy_force_speed(struct fxgmac_pdata *pdata, int speed)
{
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
	u32 regval = 0;
	unsigned int high_bit = 0, low_bit = 0;

	switch (speed) {
	case SPEED_1000:
		high_bit = 1, low_bit = 0;
		break;
	case SPEED_100:
		high_bit = 0, low_bit = 1;
		break;
	case SPEED_10:
		high_bit = 0, low_bit = 0;
		break;
	default:
		break;
	}

	/* disable autoneg */
	hw_ops->read_ephy_reg(pdata, REG_MII_BMCR, &regval);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_AUTOENG_POS,
				     PHY_CR_AUTOENG_LEN, 0);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_SPEED_SEL_H_POS,
				     PHY_CR_SPEED_SEL_H_LEN, high_bit);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_SPEED_SEL_L_POS,
				     PHY_CR_SPEED_SEL_L_LEN, low_bit);
	hw_ops->write_ephy_reg(pdata, REG_MII_BMCR, regval);
}

void fxgmac_phy_force_duplex(struct fxgmac_pdata *pdata, int duplex)
{
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
	u32 regval = 0;
	hw_ops->read_ephy_reg(pdata, REG_MII_BMCR, &regval);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_DUPLEX_POS,
				     PHY_CR_DUPLEX_LEN, (duplex ? 1 : 0));
	hw_ops->write_ephy_reg(pdata, REG_MII_BMCR, regval);
}

void fxgmac_phy_force_autoneg(struct fxgmac_pdata *pdata, int autoneg)
{
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
	u32 regval = 0;
	hw_ops->read_ephy_reg(pdata, REG_MII_BMCR, &regval);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_AUTOENG_POS,
				     PHY_CR_AUTOENG_LEN, (autoneg ? 1 : 0));
	hw_ops->write_ephy_reg(pdata, REG_MII_BMCR, regval);
}

/*
 * input: lport
 * output:
 *	cap_mask, bit definitions:
 *		pause capbility and 100/10 capbilitys follow the definition of mii reg4.
 *		for 1000M capability, bit0=1000M half; bit1=1000M full, refer to mii reg9.[9:8].
 */
int fxgmac_ephy_autoneg_ability_get(struct fxgmac_pdata *pdata,
				    unsigned int *cap_mask)
{
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
	unsigned int val;
	unsigned int reg;

	if ((!hw_ops->read_ephy_reg) || (!hw_ops->write_ephy_reg))
		return -1;

	reg = REG_MII_ADVERTISE;
	if (hw_ops->read_ephy_reg(pdata, reg, &val) < 0)
		goto busy_exit;

	if (FXGMAC_ADVERTISE_10HALF & val) {
		*cap_mask |= FXGMAC_ADVERTISE_10HALF;
	} else {
		*cap_mask &= ~FXGMAC_ADVERTISE_10HALF;
	}

	if (FXGMAC_ADVERTISE_10FULL & val) {
		*cap_mask |= FXGMAC_ADVERTISE_10FULL;
	} else {
		*cap_mask &= ~FXGMAC_ADVERTISE_10FULL;
	}

	if (FXGMAC_ADVERTISE_100HALF & val) {
		*cap_mask |= FXGMAC_ADVERTISE_100HALF;
	} else {
		*cap_mask &= ~FXGMAC_ADVERTISE_100HALF;
	}

	if (FXGMAC_ADVERTISE_100FULL & val) {
		*cap_mask |= FXGMAC_ADVERTISE_100FULL;
	} else {
		*cap_mask &= ~FXGMAC_ADVERTISE_100FULL;
	}

	if (FXGMAC_ADVERTISE_PAUSE_CAP & val) {
		*cap_mask |= FXGMAC_ADVERTISE_PAUSE_CAP;
	} else {
		*cap_mask &= ~FXGMAC_ADVERTISE_PAUSE_CAP;
	}

	if (FXGMAC_ADVERTISE_PAUSE_ASYM & val) {
		*cap_mask |= FXGMAC_ADVERTISE_PAUSE_ASYM;
	} else {
		*cap_mask &= ~FXGMAC_ADVERTISE_PAUSE_ASYM;
	}

	reg = REG_MII_CTRL1000;
	if (hw_ops->read_ephy_reg(pdata, reg, &val) < 0)
		goto busy_exit;

	if (REG_BIT_ADVERTISE_1000HALF & val) {
		*cap_mask |= FXGMAC_ADVERTISE_1000HALF;
	} else {
		*cap_mask &= ~FXGMAC_ADVERTISE_1000HALF;
	}

	if (REG_BIT_ADVERTISE_1000FULL & val) {
		*cap_mask |= FXGMAC_ADVERTISE_1000FULL;
	} else {
		*cap_mask &= ~FXGMAC_ADVERTISE_1000FULL;
	}

	return 0;

busy_exit:
	DPRINTK("fxgmac_ephy_autoneg_ability_get exit due to ephy reg access fail.\n");

	return -1;
}

int fxgmac_ephy_soft_reset(struct fxgmac_pdata *pdata)
{
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
	int ret;
	volatile unsigned int val;
	int busy = 15;

	ret = hw_ops->read_ephy_reg(pdata, REG_MII_BMCR, (unsigned int *)&val);
	if (0 > ret)
		goto busy_exit;

	ret = hw_ops->write_ephy_reg(pdata, REG_MII_BMCR, (val | 0x8000));
	if (0 > ret)
		goto busy_exit;

	do {
		ret = hw_ops->read_ephy_reg(pdata, REG_MII_BMCR,
					    (unsigned int *)&val);
		busy--;
	} while ((ret >= 0) && (0 != (val & 0x8000)) && (busy));

	if (0 == (val & 0x8000))
		return 0;

	DPRINTK("fxgmac_ephy_soft_reset, timeout, busy=%d.\n", busy);
	return -EBUSY;

busy_exit:
	DPRINTK("fxgmac_ephy_soft_reset exit due to ephy reg access fail.\n");

	return ret;
}

/* this function used to double check the speed. for fiber, to correct there is no 10M */
static int fxgmac_ephy_adjust_status(u32 lport, int val, int is_utp, int *speed,
				     int *duplex)
{
	int speed_mode;

	*speed = -1;
	*duplex = (val & BIT(FUXI_EPHY_DUPLEX_BIT)) >> FUXI_EPHY_DUPLEX_BIT;
	speed_mode = (val & FUXI_EPHY_SPEED_MODE) >> FUXI_EPHY_SPEED_MODE_BIT;
	switch (speed_mode) {
	case 0:
		if (is_utp)
			*speed = SPEED_10M;
		break;
	case 1:
		*speed = SPEED_100M;
		break;
	case 2:
		*speed = SPEED_1000M;
		break;
	case 3:
		break;
	default:
		break;
	}

	return 0;
}

/*
 * this function for polling to get status of ephy link.
 * output:
 * 		speed: SPEED_10M, SPEED_100M, SPEED_1000M or -1;
 *		duplex: 0 or 1, see reg 0x11, bit YT8614_DUPLEX_BIT.
 *		ret_link: 0 or 1, link down or up.
 *		media: only valid when ret_link=1, (YT8614_SMI_SEL_SDS_SGMII + 1) for fiber; (YT8614_SMI_SEL_PHY + 1) for utp. -1 for link down.
 */
int fxgmac_ephy_status_get(struct fxgmac_pdata *pdata, int *speed, int *duplex,
			   int *ret_link, int *media)
{
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
	int ret;
	u16 reg;
	volatile unsigned int val;
	volatile int link;
	int link_utp = 0, link_fiber = 0;

	reg = REG_MII_SPEC_STATUS;
	ret = hw_ops->read_ephy_reg(pdata, reg, (unsigned int *)&val);
	if (0 > ret)
		goto busy_exit;

	link = val & (BIT(FUXI_EPHY_LINK_STATUS_BIT));
	if (link) {
		link_utp = 1;
		fxgmac_ephy_adjust_status(0, val, 1, speed, duplex);
	} else {
		link_utp = 0;
	}

	if (link_utp || link_fiber) {
		/* case of fiber of priority */
		if (link_utp)
			*media = (FUXI_EPHY_SMI_SEL_PHY + 1);
		if (link_fiber)
			*media = (FUXI_EPHY_SMI_SEL_SDS_SGMII + 1);

		*ret_link = 1;
	} else {
		*ret_link = 0;
		*media = -1;
		*speed = -1;
		*duplex = -1;
	}

	return 0;

busy_exit:
	DPRINTK("fxgmac_ephy_status_get exit due to ephy reg access fail.\n");

	return ret;
}
