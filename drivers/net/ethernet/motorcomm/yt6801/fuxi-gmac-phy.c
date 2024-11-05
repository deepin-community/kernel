/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2021 Motorcomm Corporation. */

#include "fuxi-gmac.h"
#include "fuxi-gmac-reg.h"

/*
 * When in forced mode, set the speed, duplex, and auto-negotiation of the PHY
 * all at once to avoid the problems caused by individual settings
 * on some machines
 */
int fxgmac_phy_force_mode(struct fxgmac_pdata *pdata)
{
	struct fxgmac_hw_ops    *hw_ops = &pdata->hw_ops;
	u32                     regval = 0;
	unsigned int            high_bit = 0, low_bit = 0;
	int ret = 0;

	switch (pdata->phy_speed) {
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

	hw_ops->read_ephy_reg(pdata, REG_MII_BMCR, &regval);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_AUTOENG_POS, PHY_CR_AUTOENG_LEN, pdata->phy_autoeng);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_SPEED_SEL_H_POS, PHY_CR_SPEED_SEL_H_LEN, high_bit);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_SPEED_SEL_L_POS, PHY_CR_SPEED_SEL_L_LEN, low_bit);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_DUPLEX_POS, PHY_CR_DUPLEX_LEN, pdata->phy_duplex);
	ret = hw_ops->write_ephy_reg(pdata, REG_MII_BMCR, regval);
	return ret;
}

int fxgmac_phy_force_speed(struct fxgmac_pdata *pdata, int speed)
{
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
	u32 regval = 0;
	unsigned int high_bit = 0, low_bit = 0;
	int ret = 0;

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

	hw_ops->read_ephy_reg(pdata, REG_MII_BMCR, &regval);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_SPEED_SEL_H_POS,
				     PHY_CR_SPEED_SEL_H_LEN, high_bit);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_SPEED_SEL_L_POS,
				     PHY_CR_SPEED_SEL_L_LEN, low_bit);
	ret = hw_ops->write_ephy_reg(pdata, REG_MII_BMCR, regval);
	return ret;
}

int fxgmac_phy_force_duplex(struct fxgmac_pdata *pdata, int duplex)
{
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
	u32 regval = 0;
	int ret = 0;

	hw_ops->read_ephy_reg(pdata, REG_MII_BMCR, &regval);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_DUPLEX_POS,
				     PHY_CR_DUPLEX_LEN, (duplex ? 1 : 0));
	ret = hw_ops->write_ephy_reg(pdata, REG_MII_BMCR, regval);

	return ret;
}

int fxgmac_phy_force_autoneg(struct fxgmac_pdata *pdata, int autoneg)
{
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
	u32 regval = 0;
	int ret = 0;

	hw_ops->read_ephy_reg(pdata, REG_MII_BMCR, &regval);
	regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_AUTOENG_POS,
				     PHY_CR_AUTOENG_LEN, (autoneg ? 1 : 0));
	ret = hw_ops->write_ephy_reg(pdata, REG_MII_BMCR, regval);

	return ret;
}

void fxgmac_set_phy_link_ksettings(struct fxgmac_pdata *pdata)
{
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;

	pdata->phy_speed = pdata->expansion.pre_phy_speed;
	pdata->phy_duplex = pdata->expansion.pre_phy_duplex;
	pdata->phy_autoeng = pdata->expansion.pre_phy_autoneg;

	if (pdata->phy_autoeng)
		hw_ops->phy_config(pdata);
	else
		fxgmac_phy_force_mode(pdata);
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
	} while ((ret == 0) && (0 != (val & 0x8000)) && (busy));

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
	*duplex = (val & BIT(FXGMAC_EPHY_DUPLEX_BIT)) >> FXGMAC_EPHY_DUPLEX_BIT;
	speed_mode = (val & FXGMAC_EPHY_SPEED_MODE) >> FXGMAC_EPHY_SPEED_MODE_BIT;
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

	link = val & (BIT(FXGMAC_EPHY_LINK_STATUS_BIT));
	if (link) {
		link_utp = 1;
		fxgmac_ephy_adjust_status(0, val, 1, speed, duplex);
	} else {
		link_utp = 0;
	}

	if (link_utp || link_fiber) {
		/* case of fiber of priority */
		if (link_utp)
			*media = (FXGMAC_EPHY_SMI_SEL_PHY + 1);
		if (link_fiber)
			*media = (FXGMAC_EPHY_SMI_SEL_SDS_SGMII + 1);

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

/*
 * fxgmac_phy_update_link - update the phy link status
 * @adapter: pointer to the device adapter structure
 */
static void fxgmac_phy_update_link(struct net_device *netdev)
{
	struct fxgmac_pdata *pdata = netdev_priv(netdev);
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
	u32 regval, cur_link, cur_speed;

	regval = hw_ops->get_ephy_state(pdata);
	// We should make sure that PHY is done with the reset
	if (regval & MGMT_EPHY_CTRL_STA_EPHY_RESET) {
		pdata->expansion.phy_link = false;
		return;
	}

	cur_link = FXGMAC_GET_REG_BITS(regval,
								MGMT_EPHY_CTRL_STA_EPHY_LINKUP_POS,
								MGMT_EPHY_CTRL_STA_EPHY_LINKUP_LEN);
	if (pdata->expansion.phy_link != cur_link) {
		hw_ops->read_ephy_reg(pdata, REG_MII_INT_STATUS, NULL);
		hw_ops->read_ephy_reg(pdata, REG_MII_INT_STATUS, NULL);

		pdata->expansion.phy_link = cur_link;
		if (pdata->expansion.phy_link) {
			cur_speed = FXGMAC_GET_REG_BITS(regval,
								MGMT_EPHY_CTRL_STA_SPEED_POS,
								MGMT_EPHY_CTRL_STA_SPEED_LEN);
			pdata->phy_speed = (cur_speed == 2) ? SPEED_1000 :
								(cur_speed == 1) ? SPEED_100 : SPEED_10;
			pdata->phy_duplex = FXGMAC_GET_REG_BITS(regval,
								MGMT_EPHY_CTRL_STA_EPHY_DUPLEX_POS,
								MGMT_EPHY_CTRL_STA_EPHY_DUPLEX_LEN);
			hw_ops->config_mac_speed(pdata);

			hw_ops->enable_rx(pdata);
			hw_ops->enable_tx(pdata);
			netif_carrier_on(pdata->netdev);
			if (netif_running(pdata->netdev)) {
				netif_tx_wake_all_queues(pdata->netdev);
				dev_info(pdata->dev, "%s now is link up, mac_speed=%d.\n",
													netdev_name(pdata->netdev),
													pdata->phy_speed);
			}
		} else {
			netif_carrier_off(pdata->netdev);
			netif_tx_stop_all_queues(pdata->netdev);
			pdata->phy_speed = SPEED_UNKNOWN;
			pdata->phy_duplex = DUPLEX_UNKNOWN;
			hw_ops->disable_rx(pdata);
			hw_ops->disable_tx(pdata);
			dev_info(pdata->dev, "%s now is link down\n", netdev_name(pdata->netdev));
		}
	}
}

static void fxgmac_phy_link_poll(struct timer_list *t)
{
	struct fxgmac_pdata *pdata = from_timer(pdata, t, expansion.phy_poll_tm);

	if (NULL == pdata->netdev) {
		DPRINTK("fxgmac_phy_timer polling with NULL netdev %lx\n", (unsigned long)(pdata->netdev));
		return;
	}

	pdata->stats.ephy_poll_timer_cnt++;

#if FXGMAC_PM_FEATURE_ENABLED
	if (!test_bit(FXGMAC_POWER_STATE_DOWN, &pdata->expansion.powerstate))
#endif
	{
		mod_timer(&pdata->expansion.phy_poll_tm, jiffies + HZ / 2);
		fxgmac_phy_update_link(pdata->netdev);
	} else {
		DPRINTK("fxgmac_phy_timer polling, powerstate changed, %ld, netdev=%lx, tm=%lx\n", pdata->expansion.powerstate, (unsigned long)(pdata->netdev), (unsigned long)&pdata->expansion.phy_poll_tm);
	}
}

int fxgmac_phy_timer_init(struct fxgmac_pdata *pdata)
{
	init_timer_key(&pdata->expansion.phy_poll_tm, NULL, 0, "fuxi_phy_link_update_timer", NULL);
	pdata->expansion.phy_poll_tm.expires = jiffies + HZ / 2;
	pdata->expansion.phy_poll_tm.function = (void *)(fxgmac_phy_link_poll);
	add_timer(&pdata->expansion.phy_poll_tm);

	DPRINTK("fxgmac_phy_timer started, %lx\n", jiffies);
	return 0;
}

void fxgmac_phy_timer_destroy(struct fxgmac_pdata *pdata)
{
	del_timer_sync(&pdata->expansion.phy_poll_tm);
	DPRINTK("fxgmac_phy_timer removed\n");
}
