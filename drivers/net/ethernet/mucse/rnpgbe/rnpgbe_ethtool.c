// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/firmware.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>
#include <linux/uaccess.h>
#include <linux/mdio.h>

#include "rnpgbe.h"
#include "rnpgbe_phy.h"
#include "rnpgbe_sriov.h"
#include "rnpgbe_mbx_fw.h"
#include "rnpgbe_ethtool.h"

int rnpgbe_wol_exclusion(struct rnpgbe_adapter *adapter,
			 struct ethtool_wolinfo *wol)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	int retval = 0;

	if (!hw->wol_en) {
		retval = 1;
		wol->supported = 0;
	}

	/* WOL not supported for all devices */
	if (!rnpgbe_wol_supported(adapter, hw->device_id)) {
		retval = 1;
		wol->supported = 0;
	}

	return retval;
}

void rnpgbe_get_wol(struct net_device *netdev, struct ethtool_wolinfo *wol)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;

	// we only support magic wol
	wol->supported = hw->wol_supported;
	wol->wolopts = 0;

	/* we now can't wol */
	if (rnpgbe_wol_exclusion(adapter, wol) ||
	    !device_can_wakeup(&adapter->pdev->dev))
		return;

	if (adapter->wol & RNP_WUFC_EX)
		wol->wolopts |= WAKE_UCAST;
	if (adapter->wol & RNP_WUFC_MC)
		wol->wolopts |= WAKE_MCAST;
	if (adapter->wol & RNP_WUFC_BC)
		wol->wolopts |= WAKE_BCAST;
	if (adapter->wol & RNP_WUFC_MAG)
		wol->wolopts |= WAKE_MAGIC;
}

int rnpgbe_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wol)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;
	int ret = 0;
	u32 new_wol = 0;

	if (wol->wolopts & (~hw->wol_supported))
		return -EOPNOTSUPP;

	if (wol->wolopts & WAKE_UCAST)
		new_wol |= RNP_WUFC_EX;
	if (wol->wolopts & WAKE_MCAST)
		new_wol |= RNP_WUFC_MC;
	if (wol->wolopts & WAKE_BCAST)
		new_wol |= RNP_WUFC_BC;
	if (wol->wolopts & WAKE_MAGIC)
		new_wol |= RNP_WUFC_MAG;

	ret = rnpgbe_mbx_wol_set(hw, new_wol);
	if (ret != 0)
		return -EOPNOTSUPP;

	adapter->wol = new_wol;
	// setup mbx
	device_set_wakeup_enable(&adapter->pdev->dev, !!adapter->wol);

	return 0;
}

/* ethtool register test data */
struct rnpgbe_reg_test {
	u16 reg;
	u8 array_len;
	u8 test_type;
	u32 mask;
	u32 write;
};

/* In the hardware, registers are laid out either singly, in arrays
 * spaced 0x40 bytes apart, or in contiguous tables.  We assume
 * most tests take place on arrays or single registers (handled
 * as a single-element array) and special-case the tables.
 * Table tests are always pattern tests.
 *
 * We also make provision for some required setup steps by specifying
 * registers to be written without any read-back testing.
 */

#define PATTERN_TEST 1
#define SET_READ_TEST 2
#define WRITE_NO_TEST 3
#define TABLE32_TEST 4
#define TABLE64_TEST_LO 5
#define TABLE64_TEST_HI 6

/* default n10 register test */
static struct rnpgbe_reg_test reg_test_n10[] = {
	//{RNP_DMA_CONFIG, 1, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF},
	/*
	 * { RNP_FCRTL_n10(0), 1, PATTERN_TEST, 0x8007FFF0, 0x8007FFF0 },
	 * { RNP_FCRTH_n10(0), 1, PATTERN_TEST, 0x8007FFF0, 0x8007FFF0 },
	 * { RNP_PFCTOP, 1, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	 * { RNP_VLNCTRL, 1, PATTERN_TEST, 0x00000000, 0x00000000 },
	 * { RNP_RDBAL(0), 4, PATTERN_TEST, 0xFFFFFF80, 0xFFFFFF80 },
	 * { RNP_RDBAH(0), 4, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	 * { RNP_RDLEN(0), 4, PATTERN_TEST, 0x000FFF80, 0x000FFFFF },
	 * { RNP_RXDCTL(0), 4, WRITE_NO_TEST, 0, RNPGBE_RXDCTL_ENABLE },
	 * { RNP_RDT(0), 4, PATTERN_TEST, 0x0000FFFF, 0x0000FFFF },
	 * { RNP_RXDCTL(0), 4, WRITE_NO_TEST, 0, 0 },
	 * { RNP_FCRTH(0), 1, PATTERN_TEST, 0x8007FFF0, 0x8007FFF0 },
	 * { RNP_FCTTV(0), 1, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	 * { RNP_TDBAL(0), 4, PATTERN_TEST, 0xFFFFFF80, 0xFFFFFFFF },
	 * { RNP_TDBAH(0), 4, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	 * { RNP_TDLEN(0), 4, PATTERN_TEST, 0x000FFF80, 0x000FFF80 },
	 * { RNP_RXCTRL, 1, SET_READ_TEST, 0x00000001, 0x00000001 },
	 * { RNP_RAL(0), 16, TABLE64_TEST_LO, 0xFFFFFFFF, 0xFFFFFFFF },
	 * { RNP_RAL(0), 16, TABLE64_TEST_HI, 0x8001FFFF, 0x800CFFFF },
	 * { RNP_MTA(0), 128, TABLE32_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	 */
	{ .reg = 0 },
};

/* write and read check */
static bool reg_pattern_test(struct rnpgbe_adapter *adapter, u64 *data, int reg,
			     u32 mask, u32 write)
{
	u32 pat, val, before;
	static const u32 test_pattern[] = { 0x5A5A5A5A, 0xA5A5A5A5, 0x00000000,
					    0xFFFFFFFF };

	for (pat = 0; pat < ARRAY_SIZE(test_pattern); pat++) {
		before = readl(adapter->hw.hw_addr + reg);
		printk(KERN_DEBUG "before reg %x is %x\n", reg, before);
		writel((test_pattern[pat] & write),
		       (adapter->hw.hw_addr + reg));
		val = readl(adapter->hw.hw_addr + reg);
		if (val != (test_pattern[pat] & write & mask)) {
			e_err(drv,
			      "pattern test reg %04X failed: got 0x%08X expected 0x%08X\n",
			      reg, val, (test_pattern[pat] & write & mask));
			*data = reg;
			writel(before, adapter->hw.hw_addr + reg);
			return 1;
		}
		writel(before, adapter->hw.hw_addr + reg);
	}
	return 0;
}

static bool reg_set_and_check(struct rnpgbe_adapter *adapter, u64 *data,
			      int reg, u32 mask, u32 write)
{
	u32 val, before;

	before = readl(adapter->hw.hw_addr + reg);
	writel((write & mask), (adapter->hw.hw_addr + reg));
	val = readl(adapter->hw.hw_addr + reg);
	if ((write & mask) != (val & mask)) {
		e_err(drv,
		      "set/check reg %04X test failed: got 0x%08X expected 0x%08X\n",
		      reg, (val & mask), (write & mask));
		*data = reg;
		writel(before, (adapter->hw.hw_addr + reg));
		return 1;
	}
	writel(before, (adapter->hw.hw_addr + reg));
	return 0;
}

static bool rnpgbe_reg_test(struct rnpgbe_adapter *adapter, u64 *data)
{
	struct rnpgbe_reg_test *test;
	struct rnpgbe_hw *hw = &adapter->hw;
	u32 i;

	if (RNPGBE_REMOVED(hw->hw_addr)) {
		e_err(drv, "Adapter removed - register test blocked\n");
		*data = 1;
		return true;
	}

	test = reg_test_n10;
	/*
	 * Perform the remainder of the register test, looping through
	 * the test table until we either fail or reach the null entry.
	 */
	while (test->reg) {
		for (i = 0; i < test->array_len; i++) {
			bool b = false;

			switch (test->test_type) {
			case PATTERN_TEST:
				b = reg_pattern_test(adapter, data,
						     test->reg + (i * 0x40),
						     test->mask, test->write);
				break;
			case SET_READ_TEST:
				b = reg_set_and_check(adapter, data,
						      test->reg + (i * 0x40),
						      test->mask, test->write);
				break;
			case WRITE_NO_TEST:
				wr32(hw, test->reg + (i * 0x40), test->write);
				break;
			case TABLE32_TEST:
				b = reg_pattern_test(adapter, data,
						     test->reg + (i * 4),
						     test->mask, test->write);
				break;
			case TABLE64_TEST_LO:
				b = reg_pattern_test(adapter, data,
						     test->reg + (i * 8),
						     test->mask, test->write);
				break;
			case TABLE64_TEST_HI:
				b = reg_pattern_test(adapter, data,
						     (test->reg + 4) + (i * 8),
						     test->mask, test->write);
				break;
			}
			if (b)
				return true;
		}
		test++;
	}

	*data = 0;
	return false;
}

static int rnpgbe_link_test(struct rnpgbe_adapter *adapter, u64 *data)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	bool link_up;
	u32 link_speed = 0;
	bool duplex;
	*data = 0;

	hw->ops.check_link(hw, &link_speed, &link_up, &duplex, true);
	if (!link_up)
		*data = 1;
	return *data;
}

void rnpgbe_diag_test(struct net_device *netdev, struct ethtool_test *eth_test,
		      u64 *data)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;
	bool if_running = netif_running(netdev);

	set_bit(__RNP_TESTING, &adapter->state);
	if (eth_test->flags == ETH_TEST_FL_OFFLINE) {
		if (adapter->flags & RNP_FLAG_SRIOV_ENABLED) {
			int i;

			for (i = 0; i < adapter->num_vfs; i++) {
				if (adapter->vfinfo[i].clear_to_send) {
					netdev_warn(
						netdev, "%s",
						"offline diagnostic is not supported when VFs "
						"are present, just show pass\n");
					data[0] = 0;
					data[1] = 0;
					data[2] = 0;
					data[3] = 0;
					if (rnpgbe_link_test(adapter, &data[4]))
						eth_test->flags |= ETH_TEST_FL_FAILED;
					//eth_test->flags |= ETH_TEST_FL_FAILED;
					clear_bit(__RNP_TESTING,
						  &adapter->state);
					goto skip_ol_tests;
				}
			}
		}

		/* Offline tests */
		e_info(hw, "offline testing starting\n");

		/* bringing adapter down disables SFP+ optics */
		if (hw->ops.enable_tx_laser)
			hw->ops.enable_tx_laser(hw);

		/* Link test performed before hardware reset so autoneg doesn't
		 * interfere with test result
		 */
		if (rnpgbe_link_test(adapter, &data[4]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		e_info(hw, "register testing starting\n");
		if (rnpgbe_reg_test(adapter, &data[0]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		data[1] = 0;
		data[2] = 0;
		/*
		 * rnpgbe_reset(adapter);
		 * e_info(hw, "eeprom testing starting\n");
		 * if (rnpgbe_eeprom_test(adapter, &data[1]))
		 * eth_test->flags |= ETH_TEST_FL_FAILED;
		 * rnpgbe_reset(adapter);
		 * e_info(hw, "interrupt testing starting\n");
		 * if (rnpgbe_intr_test(adapter, &data[2]))
		 * eth_test->flags |= ETH_TEST_FL_FAILED;
		 */
		/* If SRIOV or VMDq is enabled then skip MAC
		 * loopback diagnostic.
		 */
		if (adapter->flags &
		    (RNP_FLAG_SRIOV_ENABLED | RNP_FLAG_VMDQ_ENABLED)) {
			e_info(hw, "Skip MAC loopback diagnostic in VT mode\n");
			data[3] = 0;
			goto skip_loopback;
		}

		data[3] = 0;
	skip_loopback:
		/* clear testing bit and return adapter to previous state */
		clear_bit(__RNP_TESTING, &adapter->state);
	} else {
		e_info(hw, "online testing starting\n");

		/* if adapter is down, SFP+ optics will be disabled */
		if (!if_running && hw->ops.enable_tx_laser)
			hw->ops.enable_tx_laser(hw);

		/* Online tests */
		if (rnpgbe_link_test(adapter, &data[4]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		/* Offline tests aren't run; pass by default */
		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;

		clear_bit(__RNP_TESTING, &adapter->state);
	}

	/* if adapter was down, ensure SFP+ optics are disabled again */
	if (!if_running && hw->ops.disable_tx_laser)
		hw->ops.disable_tx_laser(hw);
skip_ol_tests:
	msleep_interruptible(4 * 1000);
}

u32 rnpgbe_get_msglevel(struct net_device *netdev)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	return adapter->msg_enable;
}

void rnpgbe_set_msglevel(struct net_device *netdev, u32 data)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	adapter->msg_enable = data;
}

int rnpgbe_set_phys_id(struct net_device *netdev,
		       enum ethtool_phys_id_state state)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;

	switch (state) {
	case ETHTOOL_ID_ACTIVE:
		rnpgbe_mbx_led_set(hw, 1);
		return 2;

	case ETHTOOL_ID_ON:
		rnpgbe_mbx_led_set(hw, 2);
		break;

	case ETHTOOL_ID_OFF:
		rnpgbe_mbx_led_set(hw, 3);
		break;

	case ETHTOOL_ID_INACTIVE:
		rnpgbe_mbx_led_set(hw, 0);
		break;
	}
	return 0;
}

int rnpgbe_get_ts_info(struct net_device *dev, struct ethtool_ts_info *info)
{
	struct rnpgbe_adapter *adapter = netdev_priv(dev);

	/*For we juse set it as pf0 */
	if (!(adapter->flags2 & RNP_FLAG2_PTP_ENABLED))
		return ethtool_op_get_ts_info(dev, info);

	if (adapter->ptp_clock)
		info->phc_index = ptp_clock_index(adapter->ptp_clock);
	else
		info->phc_index = -1;

	dbg("phc_index is %d\n", info->phc_index);
	info->so_timestamping =
		SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RX_HARDWARE |
		SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_TX_SOFTWARE |
		SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_RAW_HARDWARE;

	info->tx_types = (1 << HWTSTAMP_TX_OFF) | (1 << HWTSTAMP_TX_ON);

	info->rx_filters = BIT(HWTSTAMP_FILTER_NONE) |
			   BIT(HWTSTAMP_FILTER_PTP_V1_L4_SYNC) |
			   BIT(HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ) |
			   BIT(HWTSTAMP_FILTER_PTP_V1_L4_EVENT) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_L4_SYNC) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_L4_EVENT) |
			   /* 802.AS1 */
			   BIT(HWTSTAMP_FILTER_PTP_V2_L2_EVENT) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_L2_SYNC) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ) |
			   BIT(HWTSTAMP_FILTER_ALL);

	return 0;
}

static unsigned int rnpgbe_max_channels(struct rnpgbe_adapter *adapter)
{
	unsigned int max_combined;
	struct rnpgbe_hw *hw = &adapter->hw;

	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED) {
		/* SR-IOV currently only allows 2 queue on the PF */
		max_combined = hw->sriov_ring_limit;
	} else if (adapter->flags & RNP_FLAG_DCB_ENABLED) {
		/* dcb on max support 32 */
		max_combined = 32;
	} else {
		/* support up to 16 queues with RSS */
		max_combined = adapter->max_ring_pair_counts;
		/* should not large than q_vectors ? */
	}

	return max_combined;
}

int rnpgbe_get_keee(struct net_device *netdev, struct ethtool_keee *edata)
{

	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;

	if (!(hw->feature_flags & RNP_HW_FEATURE_EEE))
		return -EOPNOTSUPP;

	if (!(hw->eee_capability))
		return -EOPNOTSUPP;

	edata->supported_u32 = 0;

	if (hw->eee_capability & EEE_1000BT)
		edata->supported_u32 |= SUPPORTED_1000baseT_Full;
	if (hw->eee_capability & EEE_100BT)
		edata->supported_u32 |= SUPPORTED_100baseT_Full;

	if (adapter->eee_enabled)
		edata->eee_enabled = true;

	edata->lp_advertised_u32 =
		mmd_eee_adv_to_ethtool_adv_t(adapter->partner_eee);
	edata->advertised_u32 = mmd_eee_adv_to_ethtool_adv_t(adapter->local_eee);

	/* @eee_active: Result of the eee auto negotiation. */
	if ((adapter->eee_enabled) &&
	    (adapter->local_eee & adapter->partner_eee))
		edata->eee_active = true;
	/* @tx_lpi_enabled: Whether the interface should assert its tx lpi */
	edata->tx_lpi_enabled = adapter->tx_path_in_lpi_mode;
	edata->tx_lpi_timer = adapter->tx_lpi_timer;

	/* if in half duplex fixme */
	if (!hw->duplex) {
		edata->eee_enabled = false;
		edata->eee_active = false;
		edata->tx_lpi_enabled = false;
		edata->advertised_u32 &= ~edata->advertised_u32;
	}
	return 0;

}

void eee_to_keee(struct ethtool_keee *keee,
                 const struct ethtool_eee *eee)
{
        memset(keee, 0, sizeof(*keee));

        keee->supported_u32 = eee->supported;
        keee->advertised_u32 = eee->advertised;
        keee->lp_advertised_u32 = eee->lp_advertised;
        keee->eee_active = eee->eee_active;
        keee->eee_enabled = eee->eee_enabled;
        keee->tx_lpi_enabled = eee->tx_lpi_enabled;
        keee->tx_lpi_timer = eee->tx_lpi_timer;

        ethtool_convert_legacy_u32_to_link_mode(keee->supported,
                                                eee->supported);
        ethtool_convert_legacy_u32_to_link_mode(keee->advertised,
                                                eee->advertised);
        ethtool_convert_legacy_u32_to_link_mode(keee->lp_advertised,
                                                eee->lp_advertised);
}

bool ethtool_eee_use_linkmodes(const struct ethtool_keee *eee)
{
        return false;
}


void keee_to_eee(struct ethtool_eee *eee,
                 const struct ethtool_keee *keee)
{
        memset(eee, 0, sizeof(*eee));

        eee->eee_active = keee->eee_active;
        eee->eee_enabled = keee->eee_enabled;
        eee->tx_lpi_enabled = keee->tx_lpi_enabled;
        eee->tx_lpi_timer = keee->tx_lpi_timer;

        if (ethtool_eee_use_linkmodes(keee)) {
                bool overflow;

                overflow = !ethtool_convert_link_mode_to_legacy_u32(&eee->supported,
                                                                    keee->supported);
                ethtool_convert_link_mode_to_legacy_u32(&eee->advertised,
                                                        keee->advertised);
                ethtool_convert_link_mode_to_legacy_u32(&eee->lp_advertised,
                                                        keee->lp_advertised);
                if (overflow)
                        pr_warn("Ethtool ioctl interface doesn't support passing EEE linkmodes beyond bit 32\n");
        } else {
                eee->supported = keee->supported_u32;
                eee->advertised = keee->advertised_u32;
                eee->lp_advertised = keee->lp_advertised_u32;
        }
}


int rnpgbe_get_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
	struct ethtool_keee kedata;
	int ret;

	eee_to_keee(&kedata, edata);
	ret = rnpgbe_get_keee(netdev, &kedata);
	keee_to_eee(edata, &kedata);

	return ret;
}

int rnpgbe_set_keee(struct net_device *netdev, struct ethtool_keee *edata)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;
	struct ethtool_keee eee_curr;
	s32 ret_val;

	if (!(hw->feature_flags & RNP_HW_FEATURE_EEE))
		return -EOPNOTSUPP;

	memset(&eee_curr, 0, sizeof(struct ethtool_eee));

	ret_val = rnpgbe_get_keee(netdev, &eee_curr);

	if (ret_val)
		return ret_val;

	if ((adapter->flags & RNP_FLAG_SRIOV_ENABLED) && (edata->eee_enabled)) {
		dev_err(pci_dev_to_dev(adapter->pdev),
			"not supported enable eee with sriov on\n");

		return -EINVAL;
	}

	if (edata->eee_enabled) {
		if (eee_curr.tx_lpi_enabled != edata->tx_lpi_enabled) {
			dev_err(pci_dev_to_dev(adapter->pdev),
				"Setting EEE tx-lpi is not supported\n");
			return -EINVAL;
		}

		if (!edata->advertised_u32 ||
		    (edata->advertised_u32 &
		     ~(ADVERTISE_100_FULL | ADVERTISE_1000_FULL))) {
			dev_err(pci_dev_to_dev(adapter->pdev),
				"EEE Advertisement supports 100Base-Tx Full Duplex(0x08) 1000Base-T Full Duplex(0x20) or both(0x28)\n");
			return -EINVAL;
		}
		adapter->local_eee = 0;
		if (edata->advertised_u32 & ADVERTISE_100_FULL)
			adapter->local_eee |= EEE_100BT;
		if (edata->advertised_u32 & SUPPORTED_1000baseT_Full)
			adapter->local_eee |= EEE_1000BT;

	} else if (!edata->eee_enabled) {
		/* we set local eee to control eee */
		adapter->local_eee = 0;
	}

	if (edata->eee_enabled)
		adapter->eee_enabled = 1;
	else
		adapter->eee_enabled = 0;

	adapter->tx_lpi_timer = edata->tx_lpi_timer;

	/* link will down is re-auto */
	if (hw->ops.setup_eee)
		hw->ops.setup_eee(hw, RNP_DEFAULT_LIT_LS, adapter->tx_lpi_timer,
				  adapter->local_eee);

	return 0;
}

int rnpgbe_set_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
	struct ethtool_keee kedata;
	int ret;

	eee_to_keee(&kedata, edata);
	ret = rnpgbe_set_keee(netdev, &kedata);
	keee_to_eee(edata, &kedata);

	return ret;
}

void rnpgbe_get_channels(struct net_device *dev, struct ethtool_channels *ch)
{
	struct rnpgbe_adapter *adapter = netdev_priv(dev);

	/* report maximum channels */
	ch->max_combined = rnpgbe_max_channels(adapter);

	/* report info for other vector */
	ch->max_other = NON_Q_VECTORS;
	ch->other_count = NON_Q_VECTORS;

	/* record RSS queues */
	ch->combined_count = adapter->ring_feature[RING_F_RSS].indices;

	/* nothing else to report if RSS is disabled */
	if (ch->combined_count == 1)
		return;

	/* we do not support ATR queueing if SR-IOV is enabled */
	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED)
		return;

	/* same thing goes for being DCB enabled */
	if (netdev_get_num_tc(dev) > 1)
		return;
}

int rnpgbe_set_channels(struct net_device *dev, struct ethtool_channels *ch)
{
	struct rnpgbe_adapter *adapter = netdev_priv(dev);
	unsigned int count = ch->combined_count;

	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED)
		return -EINVAL;

	/* verify they are not requesting separate vectors */
	if (!count || ch->rx_count || ch->tx_count)
		return -EINVAL;

	/* verify other_count has not changed */
	if (ch->other_count != NON_Q_VECTORS)
		return -EINVAL;

	dbg("call set channels %d %d %d\n", count, ch->rx_count, ch->tx_count);
	dbg("max channels %d\n", rnpgbe_max_channels(adapter));
	/* verify the number of channels does not exceed hardware limits */
	if (count > rnpgbe_max_channels(adapter))
		return -EINVAL;

	/* update feature limits from largest to smallest supported values */
	adapter->ring_feature[RING_F_FDIR].limit = count;

	/* cap RSS limit at 16 */
	if (count > adapter->max_ring_pair_counts)
		count = adapter->max_ring_pair_counts;
	adapter->ring_feature[RING_F_RSS].limit = count;

	/* use setup TC to update any traffic class queue mapping */
	return rnpgbe_setup_tc(dev, netdev_get_num_tc(dev));
}

int rnpgbe_get_module_info(struct net_device *dev,
			   struct ethtool_modinfo *modinfo)
{
	struct rnpgbe_adapter *adapter = netdev_priv(dev);
	struct rnpgbe_hw *hw = &adapter->hw;
	u8 module_id, diag_supported;
	int rc;

	rnpgbe_mbx_get_lane_stat(hw);

	if (hw->is_sgmii)
		return -EIO;

	rc = rnpgbe_mbx_sfp_module_eeprom_info(hw, 0xA0, SFF_MODULE_ID_OFFSET,
					       1, &module_id);
	if (rc || module_id == 0xff)
		return -EIO;
	rc = rnpgbe_mbx_sfp_module_eeprom_info(
		hw, 0xA0, SFF_DIAG_SUPPORT_OFFSET, 1, &diag_supported);
	if (!rc) {
		switch (module_id) {
		case SFF_MODULE_ID_SFP:
			modinfo->type = ETH_MODULE_SFF_8472;
			modinfo->eeprom_len = ETH_MODULE_SFF_8472_LEN;
			if (!diag_supported)
				modinfo->eeprom_len = ETH_MODULE_SFF_8436_LEN;
			break;
		case SFF_MODULE_ID_QSFP:
		case SFF_MODULE_ID_QSFP_PLUS:
			modinfo->type = ETH_MODULE_SFF_8436;
			modinfo->eeprom_len = ETH_MODULE_SFF_8436_LEN;
			break;
		case SFF_MODULE_ID_QSFP28:
			modinfo->type = ETH_MODULE_SFF_8636;
			modinfo->eeprom_len = ETH_MODULE_SFF_8636_LEN;
			break;
		default:
			printk(KERN_DEBUG
			       "%s: module_id:0x%x diag_supported:0x%x\n",
			       __func__, module_id, diag_supported);
			rc = -EOPNOTSUPP;
			break;
		}
	}

	return rc;
}

int rnpgbe_get_module_eeprom(struct net_device *dev,
			     struct ethtool_eeprom *eeprom, u8 *data)
{
	struct rnpgbe_adapter *adapter = netdev_priv(dev);
	struct rnpgbe_hw *hw = &adapter->hw;
	u16 start = eeprom->offset, length = eeprom->len;
	int rc = 0;

	rnpgbe_mbx_get_lane_stat(hw);

	if (hw->is_sgmii)
		return -EIO;

	memset(data, 0, eeprom->len);

	/* Read A0 portion of the EEPROM */
	if (start < ETH_MODULE_SFF_8436_LEN) {
		if (start + eeprom->len > ETH_MODULE_SFF_8436_LEN)
			length = ETH_MODULE_SFF_8436_LEN - start;
		rc = rnpgbe_mbx_sfp_module_eeprom_info(hw, 0xA0, start, length,
						       data);
		if (rc)
			return rc;
		start += length;
		data += length;
		length = eeprom->len - length;
	}

	/* Read A2 portion of the EEPROM */
	if (length) {
		start -= ETH_MODULE_SFF_8436_LEN;
		rc = rnpgbe_mbx_sfp_module_eeprom_info(hw, 0xA2, start, length,
						       data);
	}

	return rc;
}

void rnpgbe_get_ringparam(struct net_device *netdev,
			  struct ethtool_ringparam *ring,
			  struct kernel_ethtool_ringparam __always_unused *ker,
			  struct netlink_ext_ack __always_unused *extack)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	/* all ring share the same status*/

	ring->rx_max_pending = RNP_MAX_RXD;
	ring->tx_max_pending = RNP_MAX_TXD;
	ring->rx_mini_max_pending = 0;
	ring->rx_jumbo_max_pending = 0;
	ring->rx_pending = adapter->rx_ring_item_count;
	ring->tx_pending = adapter->tx_ring_item_count;
	ring->rx_mini_pending = 0;
	ring->rx_jumbo_pending = 0;
}

int rnpgbe_set_ringparam(struct net_device *netdev,
			 struct ethtool_ringparam *ring,
			 struct kernel_ethtool_ringparam __always_unused *ker,
			 struct netlink_ext_ack __always_unused *extack)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_ring *temp_ring;
	struct rnpgbe_hw *hw = &adapter->hw;
	int i, err = 0;
	u32 new_rx_count, new_tx_count;

	if ((ring->rx_mini_pending) || (ring->rx_jumbo_pending))
		return -EINVAL;

	if ((ring->tx_pending < RNP_MIN_TXD) ||
	    (ring->tx_pending > RNP_MAX_TXD) ||
	    (ring->rx_pending < RNP_MIN_RXD) ||
	    (ring->rx_pending > RNP_MAX_RXD)) {
		netdev_info(
			netdev,
			"Descriptors requested (Tx: %d / Rx: %d) out of range [%d-%d]\n",
			ring->tx_pending, ring->rx_pending, RNP_MIN_TXD,
			RNP_MAX_TXD);
		return -EINVAL;
	}

	new_tx_count = clamp_t(u32, ring->tx_pending, RNP_MIN_TXD, RNP_MAX_TXD);
	new_tx_count = ALIGN(new_tx_count, RNP_REQ_TX_DESCRIPTOR_MULTIPLE);

	new_rx_count = clamp_t(u32, ring->rx_pending, RNP_MIN_RXD, RNP_MAX_RXD);
	new_rx_count = ALIGN(new_rx_count, RNP_REQ_RX_DESCRIPTOR_MULTIPLE);

	if ((new_tx_count == adapter->tx_ring_item_count) &&
	    (new_rx_count == adapter->rx_ring_item_count)) {
		/* nothing to do */
		return 0;
	}

	while (test_and_set_bit(__RNP_RESETTING, &adapter->state))
		usleep_range(1000, 2000);

	if (!netif_running(adapter->netdev)) {
		for (i = 0; i < adapter->num_tx_queues; i++)
			adapter->tx_ring[i]->count = new_tx_count;
		for (i = 0; i < adapter->num_rx_queues; i++)
			adapter->rx_ring[i]->count = new_rx_count;
		adapter->tx_ring_item_count = new_tx_count;
		adapter->rx_ring_item_count = new_rx_count;
		goto clear_reset;
	}

	/* allocate temporary buffer to store rings in */
	i = max_t(int, adapter->num_tx_queues, adapter->num_rx_queues);
	temp_ring = vmalloc(i * sizeof(struct rnpgbe_ring));
	if (!temp_ring) {
		err = -ENOMEM;
		goto clear_reset;
	}
	memset(temp_ring, 0x00, i * sizeof(struct rnpgbe_ring));

	if (new_rx_count != adapter->rx_ring_item_count) {
		for (i = 0; i < adapter->num_rx_queues; i++) {
			adapter->rx_ring[i]->reset_count = new_rx_count;
			if (!(adapter->rx_ring[i]->ring_flags &
			      RNP_RING_SIZE_CHANGE_FIX))
				adapter->rx_ring[i]->ring_flags |=
					RNP_RING_FLAG_CHANGE_RX_LEN;
		}
	}

	/* if now we are in force mode, never need force, if not force it */
	if (!(adapter->priv_flags & RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE)) {
		hw->ops.set_mac_rx(hw, false);
		if (hw->ops.driver_status)
			hw->ops.driver_status(hw, true,
					      rnpgbe_driver_force_control_phy);
	}

	rnpgbe_down(adapter);
	/*
	 * Setup new Tx resources and free the old Tx resources in that order.
	 * We can then assign the new resources to the rings via a memcpy.
	 * The advantage to this approach is that we are guaranteed to still
	 * have resources even in the case of an allocation failure.
	 */
	if (new_tx_count != adapter->tx_ring_item_count) {
		for (i = 0; i < adapter->num_tx_queues; i++) {
			memcpy(&temp_ring[i], adapter->tx_ring[i],
			       sizeof(struct rnpgbe_ring));

			temp_ring[i].count = new_tx_count;
			err = rnpgbe_setup_tx_resources(&temp_ring[i], adapter);
			if (err) {
				while (i) {
					i--;
					rnpgbe_free_tx_resources(&temp_ring[i]);
				}
				goto err_setup;
			}
		}

		for (i = 0; i < adapter->num_tx_queues; i++) {
			rnpgbe_free_tx_resources(adapter->tx_ring[i]);
			memcpy(adapter->tx_ring[i], &temp_ring[i],
			       sizeof(struct rnpgbe_ring));
		}

		adapter->tx_ring_item_count = new_tx_count;
	}

	/* Repeat the process for the Rx rings if needed */
	if (new_rx_count != adapter->rx_ring_item_count) {
		for (i = 0; i < adapter->num_rx_queues; i++) {
			memcpy(&temp_ring[i], adapter->rx_ring[i],
			       sizeof(struct rnpgbe_ring));
			/* setup ring count */
			if (!(adapter->rx_ring[i]->ring_flags &
			      RNP_RING_FLAG_DELAY_SETUP_RX_LEN)) {
				temp_ring[i].count = new_rx_count;
			} else {
				/* setup temp count */
				temp_ring[i].count = temp_ring[i].temp_count;
				adapter->rx_ring[i]->reset_count = new_rx_count;
			}
			err = rnpgbe_setup_rx_resources(&temp_ring[i], adapter);
			if (err) {
				while (i) {
					i--;
					rnpgbe_free_rx_resources(&temp_ring[i]);
				}
				goto err_setup;
			}
		}

		for (i = 0; i < adapter->num_rx_queues; i++) {
			rnpgbe_free_rx_resources(adapter->rx_ring[i]);
			memcpy(adapter->rx_ring[i], &temp_ring[i],
			       sizeof(struct rnpgbe_ring));
		}
		adapter->rx_ring_item_count = new_rx_count;
	}

err_setup:
	rnpgbe_up(adapter);
	vfree(temp_ring);
	if (!(adapter->priv_flags & RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE)) {
		if (hw->ops.driver_status)
			hw->ops.driver_status(hw, false,
					      rnpgbe_driver_force_control_phy);
	}
clear_reset:
	clear_bit(__RNP_RESETTING, &adapter->state);
	return err;
}

int rnpgbe_get_dump_flag(struct net_device *netdev, struct ethtool_dump *dump)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	rnpgbe_mbx_get_dump_flags(&adapter->hw);

	dump->flag = adapter->hw.dump.flag;
	dump->len = adapter->hw.dump.len;
	dump->version = adapter->hw.dump.version;

	return 0;
}

int rnpgbe_get_dump_data(struct net_device *netdev, struct ethtool_dump *dump,
			 void *buffer)
{
	int err;
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	err = rnpgbe_mbx_get_dump(&adapter->hw, dump->flag, buffer, dump->len);
	if (err)
		return err;

	dump->flag = adapter->hw.dump.flag;
	dump->len = adapter->hw.dump.len;
	dump->version = adapter->hw.dump.version;

	return 0;
}

int rnpgbe_set_dump(struct net_device *netdev, struct ethtool_dump *dump)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	rnpgbe_mbx_set_dump(&adapter->hw, dump->flag);

	return 0;
}

int rnpgbe_get_coalesce(struct net_device *netdev,
			struct ethtool_coalesce *coal,
			struct kernel_ethtool_coalesce *kernel_coal,
			struct netlink_ext_ack *extack)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	if (adapter->priv_flags & RNP_PRIV_FLAG_TX_COALESCE) {
		coal->use_adaptive_tx_coalesce = 0;
		coal->tx_coalesce_usecs = adapter->tx_usecs;
		coal->tx_coalesce_usecs_irq = 0;
		coal->tx_max_coalesced_frames = adapter->tx_frames;
		coal->tx_max_coalesced_frames_irq = adapter->tx_work_limit;
		coal->tx_coalesce_usecs_low = 0;
		coal->tx_max_coalesced_frames_low = 0;
		coal->tx_coalesce_usecs_high = 0;
		coal->tx_max_coalesced_frames_high = 0;
	} else {
		coal->use_adaptive_tx_coalesce = 1;
		coal->tx_coalesce_usecs = adapter->tx_usecs;
		coal->tx_coalesce_usecs_irq = 0;
		coal->tx_max_coalesced_frames = adapter->tx_frames;
		coal->tx_max_coalesced_frames_irq = adapter->tx_work_limit;
		coal->tx_coalesce_usecs_low = 0;
		coal->tx_max_coalesced_frames_low = 0;
		coal->tx_coalesce_usecs_high = 0;
		coal->tx_max_coalesced_frames_high = 0;
	}

	if (adapter->priv_flags & RNP_PRIV_FLAG_RX_COALESCE) {
		coal->use_adaptive_rx_coalesce = 0;
		coal->rx_coalesce_usecs_irq = 0;
		coal->rx_coalesce_usecs = adapter->rx_usecs;
		coal->rx_max_coalesced_frames = adapter->rx_frames;
		coal->rx_max_coalesced_frames_irq = adapter->napi_budge;
		coal->rx_coalesce_usecs_low = 0;
		coal->rx_max_coalesced_frames_low = 0;
		coal->rx_coalesce_usecs_high = 0;
		coal->rx_max_coalesced_frames_high = 0;
	} else {
		coal->use_adaptive_rx_coalesce = 1;
		coal->rx_coalesce_usecs_irq = 0;
		coal->rx_coalesce_usecs = adapter->rx_usecs;
		coal->rx_max_coalesced_frames = adapter->rx_frames;
		coal->rx_max_coalesced_frames_irq = adapter->napi_budge;
		coal->rx_coalesce_usecs_low = 0;
		coal->rx_max_coalesced_frames_low = 0;
		coal->rx_coalesce_usecs_high = 0;
		coal->rx_max_coalesced_frames_high = 0;
	}

	/* this is not support */
	coal->pkt_rate_low = 0;
	coal->pkt_rate_high = 0;
	coal->rate_sample_interval = 0;

	return 0;
}

int rnpgbe_set_coalesce(struct net_device *netdev,
			struct ethtool_coalesce *ec,
			struct kernel_ethtool_coalesce *kernel_coal,
			struct netlink_ext_ack *extack)
{
	int reset = 0;
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	u32 value;
	if (!ec->use_adaptive_tx_coalesce)
		adapter->priv_flags |= RNP_PRIV_FLAG_TX_COALESCE;
	else
		adapter->priv_flags &= ~RNP_PRIV_FLAG_TX_COALESCE;

	if (!ec->use_adaptive_rx_coalesce)
		adapter->priv_flags |= RNP_PRIV_FLAG_RX_COALESCE;
	else
		adapter->priv_flags &= ~RNP_PRIV_FLAG_RX_COALESCE;

	if ((ec->tx_max_coalesced_frames_irq < RNP_MIN_TX_WORK) ||
	    (ec->tx_max_coalesced_frames_irq > RNP_MAX_TX_WORK))
		return -EINVAL;
	/* check coalesce frame irq */
	value = clamp_t(u32, ec->tx_max_coalesced_frames_irq, RNP_MIN_TX_WORK,
			RNP_MAX_TX_WORK);
	value = ALIGN(value, RNP_WORK_ALIGN);

	if (adapter->tx_work_limit != value) {
		reset = 1;
		adapter->tx_work_limit = value;
	}
	/* check vlaue */
	if ((ec->tx_max_coalesced_frames < RNP_MIN_TX_FRAME) ||
	    (ec->tx_max_coalesced_frames > RNP_MAX_TX_FRAME))
		return -EINVAL;

	value = clamp_t(u32, ec->tx_max_coalesced_frames, RNP_MIN_TX_FRAME,
			RNP_MAX_TX_FRAME);
	if (adapter->tx_frames != value) {
		reset = 1;
		adapter->tx_frames = value;
	}

	if ((ec->tx_coalesce_usecs < RNP_MIN_TX_USEC) ||
	    (ec->tx_coalesce_usecs > RNP_MAX_TX_USEC))
		return -EINVAL;
	/* check vlaue */
	value = clamp_t(u32, ec->tx_coalesce_usecs, RNP_MIN_TX_USEC,
			RNP_MAX_TX_USEC);
	if (adapter->tx_usecs != value) {
		reset = 1;
		adapter->tx_usecs = value;
	}

	if ((ec->rx_max_coalesced_frames_irq < RNP_MIN_RX_WORK) ||
	    (ec->rx_max_coalesced_frames_irq > RNP_MAX_RX_WORK))
		return -EINVAL;

	value = clamp_t(u32, ec->rx_max_coalesced_frames_irq, RNP_MIN_RX_WORK,
			RNP_MAX_RX_WORK);
	value = ALIGN(value, RNP_WORK_ALIGN);

	if (adapter->napi_budge != value) {
		reset = 1;
		adapter->napi_budge = value;
	}

	if ((ec->rx_max_coalesced_frames < RNP_MIN_RX_FRAME) ||
	    (ec->rx_max_coalesced_frames > RNP_MAX_RX_FRAME))
		return -EINVAL;

	value = clamp_t(u32, ec->rx_max_coalesced_frames, RNP_MIN_RX_FRAME,
			RNP_MAX_RX_FRAME);
	if (adapter->rx_frames != value) {
		reset = 1;
		adapter->rx_frames = value;
	}

	if ((ec->rx_coalesce_usecs < RNP_MIN_RX_USEC) ||
	    (ec->rx_coalesce_usecs > RNP_MAX_RX_USEC))
		return -EINVAL;
	/* check vlaue */
	value = clamp_t(u32, ec->rx_coalesce_usecs, RNP_MIN_RX_USEC,
			RNP_MAX_RX_USEC);

	if (adapter->rx_usecs != value) {
		reset = 1;
		adapter->rx_usecs = value;
	}
	/* other setup is not supported */
	if ((ec->pkt_rate_low) || (ec->pkt_rate_high) ||
	    (ec->rx_coalesce_usecs_low) || (ec->rx_max_coalesced_frames_low) ||
	    (ec->tx_coalesce_usecs_low) || (ec->tx_max_coalesced_frames_low) ||
	    (ec->rx_coalesce_usecs_high) ||
	    (ec->rx_max_coalesced_frames_high) ||
	    (ec->tx_coalesce_usecs_high) ||
	    (ec->tx_max_coalesced_frames_high) || (ec->rate_sample_interval) ||
	    (ec->tx_coalesce_usecs_irq) || (ec->rx_coalesce_usecs_irq))
		return -EINVAL;

	if (reset)
		return rnpgbe_setup_tc(netdev, netdev_get_num_tc(netdev));

	return 0;
}

static int rnpgbe_get_rss_hash_opts(struct rnpgbe_adapter *adapter,
				    struct ethtool_rxnfc *cmd)
{
	cmd->data = 0;

	/* Report default options for RSS on rnp */
	switch (cmd->flow_type) {
	case TCP_V4_FLOW:
		cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		/* fall through */
		fallthrough;
	case UDP_V4_FLOW:
	case SCTP_V4_FLOW:
		cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		/* fall through */
		fallthrough;
	case AH_ESP_V4_FLOW:
	case AH_V4_FLOW:
	case ESP_V4_FLOW:
	case IPV4_FLOW:
		cmd->data |= RXH_IP_SRC | RXH_IP_DST;
		break;
	case TCP_V6_FLOW:
		cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		/* fall through */
		fallthrough;
	case UDP_V6_FLOW:
	case SCTP_V6_FLOW:
		cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		/* fall through */
		fallthrough;
	case AH_ESP_V6_FLOW:
	case AH_V6_FLOW:
	case ESP_V6_FLOW:
	case IPV6_FLOW:
		cmd->data |= RXH_IP_SRC | RXH_IP_DST;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

void dump_fsp(struct ethtool_rx_flow_spec *fsp)
{
	int i;

	dbg(" fsp cookie is %llx\n", fsp->ring_cookie);
	switch (fsp->flow_type & ~FLOW_EXT) {
	case ETHER_FLOW:
		for (i = 0; i < ETH_ALEN; i++)
			dbg("src 0x%02x\n", fsp->h_u.ether_spec.h_source[i]);
		for (i = 0; i < ETH_ALEN; i++)
			dbg("dst 0x%02x\n", fsp->h_u.ether_spec.h_dest[i]);
		for (i = 0; i < ETH_ALEN; i++)
			dbg("src mask 0x%02x\n",
			    fsp->m_u.ether_spec.h_source[i]);
		for (i = 0; i < ETH_ALEN; i++)
			dbg("dst mask 0x%02x\n", fsp->m_u.ether_spec.h_dest[i]);

		dbg("proto type is %x\n", fsp->h_u.ether_spec.h_proto);

		break;

	default:
		dbg("flow type is %x\n", fsp->flow_type);
		dbg("l2 prot is %x\n", fsp->h_u.ether_spec.h_proto);
		dbg("ip4 src ip is %x\n", fsp->h_u.tcp_ip4_spec.ip4src);
		dbg("ip4 src ip mask is %x\n", fsp->m_u.tcp_ip4_spec.ip4src);

		dbg("ip4 dst ip is %x\n", fsp->h_u.tcp_ip4_spec.ip4dst);
		dbg("ip4 dst ip mask is %x\n", fsp->m_u.tcp_ip4_spec.ip4dst);

		dbg("ip4 src port is %x\n", fsp->h_u.tcp_ip4_spec.psrc);
		dbg("ip4 src port mask is %x\n", fsp->m_u.tcp_ip4_spec.psrc);

		dbg("ip4 dst port is %x\n", fsp->h_u.tcp_ip4_spec.pdst);
		dbg("ip4 dst port mask is %x\n", fsp->m_u.tcp_ip4_spec.pdst);

		dbg("proto is %x\n", fsp->h_u.usr_ip4_spec.proto);
		break;
	}
}

static int rnpgbe_get_ethtool_fdir_entry(struct rnpgbe_adapter *adapter,
					 struct ethtool_rxnfc *cmd)
{
	struct ethtool_rx_flow_spec *fsp =
		(struct ethtool_rx_flow_spec *)&cmd->fs;
	struct hlist_node *node2;
	struct rnpgbe_fdir_filter *rule = NULL;

	/* report total rule count */
	cmd->data = adapter->fdir_pballoc;

	hlist_for_each_entry_safe (rule, node2, &adapter->fdir_filter_list,
				   fdir_node)
		if (fsp->location <= rule->sw_idx)
			break;

	if (!rule || fsp->location != rule->sw_idx)
		return -EINVAL;

	/* fill out the flow spec entry */

	/* set flow type field */
	switch (rule->filter.formatted.flow_type) {
	case RNP_ATR_FLOW_TYPE_TCPV4:
		fsp->flow_type = TCP_V4_FLOW;
		break;
	case RNP_ATR_FLOW_TYPE_UDPV4:
		fsp->flow_type = UDP_V4_FLOW;
		break;
	case RNP_ATR_FLOW_TYPE_SCTPV4:
		fsp->flow_type = SCTP_V4_FLOW;
		break;
	case RNP_ATR_FLOW_TYPE_IPV4:
		fsp->flow_type = IP_USER_FLOW;
		fsp->h_u.usr_ip4_spec.ip_ver = ETH_RX_NFC_IP4;
		if (adapter->fdir_mode == fdir_mode_tuple5) {
			fsp->h_u.usr_ip4_spec.proto =
				rule->filter.formatted.inner_mac[0];
			fsp->m_u.usr_ip4_spec.proto = 0xff;
		} else {
			fsp->h_u.usr_ip4_spec.proto =
				rule->filter.formatted.inner_mac[0] &
				rule->filter.formatted.inner_mac_mask[0];
			fsp->m_u.usr_ip4_spec.proto =
				rule->filter.formatted.inner_mac_mask[0];
		}
		break;
	case RNP_ATR_FLOW_TYPE_ETHER:
		fsp->flow_type = ETHER_FLOW;
		/* support proto and mask only in this mode */
		fsp->h_u.ether_spec.h_proto = rule->filter.layer2_formate.proto;
		fsp->m_u.ether_spec.h_proto = 0xffff;
		break;
	default:
		return -EINVAL;
	}
	if (rule->filter.formatted.flow_type != RNP_ATR_FLOW_TYPE_ETHER) {
		/* not support mask in tuple 5 mode */
		if (adapter->fdir_mode == fdir_mode_tuple5) {
			fsp->h_u.tcp_ip4_spec.psrc =
				rule->filter.formatted.src_port;
			fsp->h_u.tcp_ip4_spec.pdst =
				rule->filter.formatted.dst_port;
			fsp->h_u.tcp_ip4_spec.ip4src =
				rule->filter.formatted.src_ip[0];
			fsp->h_u.tcp_ip4_spec.ip4dst =
				rule->filter.formatted.dst_ip[0];
			fsp->m_u.tcp_ip4_spec.psrc = 0xffff;
			fsp->m_u.tcp_ip4_spec.pdst = 0xffff;
			fsp->m_u.tcp_ip4_spec.ip4src = 0xffffffff;
			fsp->m_u.tcp_ip4_spec.ip4dst = 0xffffffff;
		} else {
			fsp->h_u.tcp_ip4_spec.psrc =
				rule->filter.formatted.src_port &
				rule->filter.formatted.src_port_mask;
			fsp->m_u.tcp_ip4_spec.psrc =
				rule->filter.formatted.src_port_mask;
			fsp->h_u.tcp_ip4_spec.pdst =
				rule->filter.formatted.dst_port &
				rule->filter.formatted.dst_port_mask;
			fsp->m_u.tcp_ip4_spec.pdst =
				rule->filter.formatted.dst_port_mask;

			fsp->h_u.tcp_ip4_spec.ip4src =
				rule->filter.formatted.src_ip[0] &
				rule->filter.formatted.src_ip_mask[0];
			fsp->m_u.tcp_ip4_spec.ip4src =
				rule->filter.formatted.src_ip_mask[0];

			fsp->h_u.tcp_ip4_spec.ip4dst =
				rule->filter.formatted.dst_ip[0] &
				rule->filter.formatted.dst_ip_mask[0];
			fsp->m_u.tcp_ip4_spec.ip4dst =
				rule->filter.formatted.dst_ip_mask[0];
		}
	}

	/* record action */
	if (rule->action == RNP_FDIR_DROP_QUEUE)
		fsp->ring_cookie = RX_CLS_FLOW_DISC;
	else {
		if (rule->vf_num != 0) {
			fsp->ring_cookie =
				((u64)rule->vf_num << 32) | (rule->action);
		} else {
			fsp->ring_cookie = rule->action;
		}
	}

	return 0;
}

static int rnpgbe_get_ethtool_fdir_all(struct rnpgbe_adapter *adapter,
				       struct ethtool_rxnfc *cmd,
				       u32 *rule_locs)
{
	struct hlist_node *node2;
	struct rnpgbe_fdir_filter *rule;
	int cnt = 0;

	/* report total rule count */
	cmd->data = adapter->fdir_pballoc;

	hlist_for_each_entry_safe (rule, node2, &adapter->fdir_filter_list,
				   fdir_node) {
		if (cnt == cmd->rule_cnt)
			return -EMSGSIZE;
		rule_locs[cnt] = rule->sw_idx;
		cnt++;
	}

	cmd->rule_cnt = cnt;

	return 0;
}

int rnpgbe_get_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd,
		     u32 *rule_locs)
{
	struct rnpgbe_adapter *adapter = netdev_priv(dev);
	int ret = -EOPNOTSUPP;
	struct rnpgbe_hw *hw = &adapter->hw;

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
		if (adapter->flags & RNP_FLAG_SRIOV_ENABLED) {
			/* we fix 2 when srio on */
			cmd->data = hw->sriov_ring_limit;
		} else {
			cmd->data = adapter->num_rx_queues;
		}
		ret = 0;
		break;
	case ETHTOOL_GRXCLSRLCNT:
		cmd->rule_cnt = adapter->fdir_filter_count;
		ret = 0;
		break;
	case ETHTOOL_GRXCLSRULE:
		ret = rnpgbe_get_ethtool_fdir_entry(adapter, cmd);
		break;
	case ETHTOOL_GRXCLSRLALL:
		ret = rnpgbe_get_ethtool_fdir_all(adapter, cmd,
						  (u32 *)rule_locs);
		break;
	case ETHTOOL_GRXFH:
		ret = rnpgbe_get_rss_hash_opts(adapter, cmd);
		break;
	default:
		break;
	}

	return ret;
}
#define UDP_RSS_FLAGS                                                          \
	(RNP_FLAG2_RSS_FIELD_IPV4_UDP | RNP_FLAG2_RSS_FIELD_IPV6_UDP)
static int rnpgbe_set_rss_hash_opt(struct rnpgbe_adapter *adapter,
				   struct ethtool_rxnfc *nfc)
{
	/*
	 * RSS does not support anything other than hashing
	 * to queues on src and dst IPs and ports
	 */
	if (nfc->data &
	    ~(RXH_IP_SRC | RXH_IP_DST | RXH_L4_B_0_1 | RXH_L4_B_2_3))
		return -EINVAL;

	switch (nfc->flow_type) {
	case TCP_V4_FLOW:
	case TCP_V6_FLOW:
	case UDP_V4_FLOW:
	case UDP_V6_FLOW:
		if (!(nfc->data & RXH_IP_SRC) || !(nfc->data & RXH_IP_DST) ||
		    !(nfc->data & RXH_L4_B_0_1) || !(nfc->data & RXH_L4_B_2_3))
			return -EINVAL;
		break;
	case AH_ESP_V4_FLOW:
	case AH_V4_FLOW:
	case ESP_V4_FLOW:
	case SCTP_V4_FLOW:
	case AH_ESP_V6_FLOW:
	case AH_V6_FLOW:
	case ESP_V6_FLOW:
	case SCTP_V6_FLOW:
		if (!(nfc->data & RXH_IP_SRC) || !(nfc->data & RXH_IP_DST) ||
		    (nfc->data & RXH_L4_B_0_1) || (nfc->data & RXH_L4_B_2_3))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int rnpgbe_flowspec_to_flow_type(struct rnpgbe_adapter *adapter,
					struct ethtool_rx_flow_spec *fsp,
					uint8_t *flow_type,
					struct rnpgbe_fdir_filter *input)
{
	int i;
	int ret = 1;
	/* not support flow_ext */
	if (fsp->flow_type & FLOW_EXT)
		return 0;

	switch (fsp->flow_type & ~FLOW_EXT) {
	/* todo ipv6 is not considered*/
	case TCP_V4_FLOW:
		*flow_type = RNP_ATR_FLOW_TYPE_TCPV4;
		break;
	case UDP_V4_FLOW:
		*flow_type = RNP_ATR_FLOW_TYPE_UDPV4;
		break;
	case SCTP_V4_FLOW:
		*flow_type = RNP_ATR_FLOW_TYPE_SCTPV4;
		break;
	case ETHER_FLOW:
		/* layer 2 flow */
		*flow_type = RNP_ATR_FLOW_TYPE_ETHER;
		input->filter.layer2_formate.proto =
			fsp->h_u.ether_spec.h_proto;
		break;
	case IP_USER_FLOW:
		switch (fsp->h_u.usr_ip4_spec.proto) {
		case IPPROTO_TCP:
			*flow_type = RNP_ATR_FLOW_TYPE_TCPV4;
			break;
		case IPPROTO_UDP:
			*flow_type = RNP_ATR_FLOW_TYPE_UDPV4;
			break;
		case IPPROTO_SCTP:
			*flow_type = RNP_ATR_FLOW_TYPE_SCTPV4;
			break;
		case 0:
			/* if only ip4 no src no dst*/
			if (!(fsp->h_u.tcp_ip4_spec.ip4src) &&
			    (!(fsp->h_u.tcp_ip4_spec.ip4dst))) {
				/* if have no l4 proto, use layer2 */
				*flow_type = RNP_ATR_FLOW_TYPE_ETHER;
				input->filter.layer2_formate.proto =
					htons(0x0800);
			} else {
				/* may only src or dst input */
				*flow_type = RNP_ATR_FLOW_TYPE_IPV4;
			}
			break;
		default:
			/* other unknown l4 proto ip */
			*flow_type = RNP_ATR_FLOW_TYPE_IPV4;
		}
		break;
	default:
		return 0;
	}
	/* layer2 flow */
	if (*flow_type == RNP_ATR_FLOW_TYPE_ETHER) {
		if (adapter->layer2_count < 0) {
			e_err(drv, "layer2 count full\n");
			ret = 0;
		}
		/* should check dst mac filter */
		/* should check src dst all zeros */
		for (i = 0; i < ETH_ALEN; i++) {
			if (fsp->h_u.ether_spec.h_source[i] != 0)
				ret = 0;

			if (fsp->h_u.ether_spec.h_dest[i] != 0)
				ret = 0;

			if (fsp->m_u.ether_spec.h_source[i] != 0)
				ret = 0;

			if (fsp->m_u.ether_spec.h_dest[i] != 0)
				ret = 0;
		}
		// we not support setup vlan type
		if (input->filter.layer2_formate.proto == htons(ETH_P_8021Q))
			ret = 0;
		if (input->filter.layer2_formate.proto == htons(0x88a8))
			ret = 0;
		if (input->filter.layer2_formate.proto == htons(0x9100))
			ret = 0;
		if (input->filter.layer2_formate.proto == htons(0x9200))
			ret = 0;

	} else if (*flow_type == RNP_ATR_FLOW_TYPE_IPV4) {
		if (adapter->fdir_mode == fdir_mode_tuple5) {
			if (adapter->tuple_5_count < 0) {
				e_err(drv, "tuple 5 count full\n");
				ret = 0;
			}
			if ((fsp->h_u.usr_ip4_spec.ip4src != 0) &&
			    (fsp->m_u.usr_ip4_spec.ip4src != 0xffffffff)) {
				e_err(drv, "ip src mask error\n");
				ret = 0;
			}
			if ((fsp->h_u.usr_ip4_spec.ip4dst != 0) &&
			    (fsp->m_u.usr_ip4_spec.ip4dst != 0xffffffff)) {
				e_err(drv, "ip dst mask error\n");
				ret = 0;
			}
			if ((fsp->h_u.usr_ip4_spec.proto != 0) &&
			    (fsp->m_u.usr_ip4_spec.proto != 0xff)) {
				e_err(drv, "ip l4 proto mask error\n");
				ret = 0;
			}
		} else {
			if (adapter->tuple_5_count < 0) {
				e_err(drv, "tcam count full\n");
				ret = 0;
			}
			/* tcam mode can support mask */
		}
		/* not support l4_4_bytes */
		if ((fsp->h_u.usr_ip4_spec.l4_4_bytes != 0)) {
			e_err(drv, "ip l4_4_bytes error\n");
			ret = 0;
		}
	} else {
		if (adapter->fdir_mode == fdir_mode_tuple5) {
			/* should check mask all ff */
			if (adapter->tuple_5_count < 0) {
				e_err(drv, "tuple 5 count full\n");
				ret = 0;
			}
			if ((fsp->h_u.tcp_ip4_spec.ip4src != 0) &&
			    (fsp->m_u.tcp_ip4_spec.ip4src != 0xffffffff)) {
				e_err(drv, "src mask error\n");
				ret = 0;
			}
			if ((fsp->h_u.tcp_ip4_spec.ip4dst != 0) &&
			    (fsp->m_u.tcp_ip4_spec.ip4dst != 0xffffffff)) {
				e_err(drv, "dst mask error\n");
				ret = 0;
			}
			if ((fsp->h_u.tcp_ip4_spec.psrc != 0) &&
			    (fsp->m_u.tcp_ip4_spec.psrc != 0xffff)) {
				e_err(drv, "src port mask error\n");
				ret = 0;
			}
			if ((fsp->h_u.tcp_ip4_spec.pdst != 0) &&
			    (fsp->m_u.tcp_ip4_spec.pdst != 0xffff)) {
				e_err(drv, "src port mask error\n");
				ret = 0;
			}
		} else {
			if (adapter->tuple_5_count < 0) {
				e_err(drv, "tcam count full\n");
				ret = 0;
			}
		}
		/* l4 tos is not supported */
		if (fsp->h_u.tcp_ip4_spec.tos != 0) {
			e_err(drv, "tos error\n");
			ret = 0;
		}
	}

	return ret;
}

int rnpgbe_update_ethtool_fdir_entry(struct rnpgbe_adapter *adapter,
				     struct rnpgbe_fdir_filter *input,
				     u16 sw_idx)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct hlist_node *node2;
	struct rnpgbe_fdir_filter *rule, *parent;
	bool deleted = false;
	u16 hw_idx_layer2 = 0;
	u16 hw_idx_tuple5 = 0;

	s32 err;

	parent = NULL;
	rule = NULL;

	hlist_for_each_entry_safe (rule, node2, &adapter->fdir_filter_list,
				   fdir_node) {
		/* hash found, or no matching entry */
		if (rule->sw_idx >= sw_idx)
			break;

		parent = rule;
	}

	/* if there is an old rule occupying our place remove it */
	if (rule && (rule->sw_idx == sw_idx)) {
		/* only clear hw enable bits */
		/* hardware filters are only configured when interface is up,
		 * and we should not issue filter commands while the interface
		 * is down
		 */
		if (netif_running(adapter->netdev) && (!input)) {
			err = rnpgbe_fdir_erase_perfect_filter(
				adapter->fdir_mode, hw, &rule->filter,
				rule->hw_idx);
			if (err)
				return -EINVAL;
		}

		adapter->fdir_filter_count--;
		if (rule->filter.formatted.flow_type == RNP_ATR_FLOW_TYPE_ETHER)
			adapter->layer2_count++;
		else
			adapter->tuple_5_count++;

		hlist_del(&rule->fdir_node);
		kfree(rule);
		deleted = true;
	}

	/* If we weren't given an input, then this was a request to delete a
	 * filter. We should return -EINVAL if the filter wasn't found, but
	 * return 0 if the rule was successfully deleted.
	 */
	if (!input)
		return deleted ? 0 : -EINVAL;

	/* initialize node and set software index */
	INIT_HLIST_NODE(&input->fdir_node);

	/* add filter to the list */
	if (parent)
		hlist_add_behind(&input->fdir_node, &parent->fdir_node);
	else
		hlist_add_head(&input->fdir_node, &adapter->fdir_filter_list);

	/* we must setup all */
	/* should first earase all tcam and l2 rule */

	if (adapter->fdir_mode != fdir_mode_tcam)
		hw->ops.clr_all_layer2_remapping(hw);
	else
		hw->ops.clr_all_tuple5_remapping(hw);

	/* setup hw */
	hlist_for_each_entry_safe(rule, node2, &adapter->fdir_filter_list,
				  fdir_node) {
		if (netif_running(adapter->netdev)) {
			/* hw_idx */
			if (rule->filter.formatted.flow_type ==
			    RNP_ATR_FLOW_TYPE_ETHER)
				rule->hw_idx = hw_idx_layer2++;
			else
				rule->hw_idx = hw_idx_tuple5++;

			if ((!rule->vf_num) &&
			    (rule->action != ACTION_TO_MPE)) {
				int idx = rule->action;

				err = rnpgbe_fdir_write_perfect_filter(
					adapter->fdir_mode, hw, &rule->filter,
					rule->hw_idx,
					(rule->action == RNP_FDIR_DROP_QUEUE) ?
						RNP_FDIR_DROP_QUEUE :
						adapter->rx_ring[idx]
							->rnpgbe_queue_idx,
					(adapter->priv_flags &
					 RNP_PRIV_FLAG_REMAP_PRIO) ?
						true :
						false);
			} else {
				err = rnpgbe_fdir_write_perfect_filter(
					adapter->fdir_mode, hw, &rule->filter,
					rule->hw_idx,
					(rule->action == RNP_FDIR_DROP_QUEUE) ?
						RNP_FDIR_DROP_QUEUE :
						rule->action,
					(adapter->priv_flags &
					 RNP_PRIV_FLAG_REMAP_PRIO) ?
						true :
						false);
			}
			if (err)
				return -EINVAL;
		}
	}

	/* update counts */
	adapter->fdir_filter_count++;
	if (input->filter.formatted.flow_type == RNP_ATR_FLOW_TYPE_ETHER) {
		/* used to determine hw reg offset */
		adapter->layer2_count--;
	} else {
		adapter->tuple_5_count--;
	}
	return 0;
}

/* used to dbg flo_spec info */
static void print_fsp(struct ethtool_rx_flow_spec *fsp)
{
	int i;

	switch (fsp->flow_type & ~FLOW_EXT) {
	case ETHER_FLOW:
		for (i = 0; i < ETH_ALEN; i++)
			dbg("src 0x%02x\n", fsp->h_u.ether_spec.h_source[i]);
		for (i = 0; i < ETH_ALEN; i++)
			dbg("dst 0x%02x\n", fsp->h_u.ether_spec.h_dest[i]);
		for (i = 0; i < ETH_ALEN; i++)
			dbg("src mask 0x%02x\n",
			    fsp->m_u.ether_spec.h_source[i]);
		for (i = 0; i < ETH_ALEN; i++)
			dbg("dst mask 0x%02x\n", fsp->m_u.ether_spec.h_dest[i]);

		dbg("proto type is %x\n", fsp->h_u.ether_spec.h_proto);

		break;

	default:
		dbg("flow type is %x\n", fsp->flow_type);

		dbg("ip4 src ip is %x\n", fsp->h_u.tcp_ip4_spec.ip4src);
		dbg("ip4 src ip mask is %x\n", fsp->m_u.tcp_ip4_spec.ip4src);

		dbg("ip4 dst ip is %x\n", fsp->h_u.tcp_ip4_spec.ip4dst);
		dbg("ip4 dst ip mask is %x\n", fsp->m_u.tcp_ip4_spec.ip4dst);

		dbg("ip4 src port is %x\n", fsp->h_u.tcp_ip4_spec.psrc);
		dbg("ip4 src port mask is %x\n", fsp->m_u.tcp_ip4_spec.psrc);

		dbg("ip4 dst port is %x\n", fsp->h_u.tcp_ip4_spec.pdst);
		dbg("ip4 dst port mask is %x\n", fsp->m_u.tcp_ip4_spec.pdst);

		dbg("l4 proto type is %x\n", fsp->h_u.usr_ip4_spec.proto);
		break;
	}
}

static int rnpgbe_add_ethtool_fdir_entry(struct rnpgbe_adapter *adapter,
					 struct ethtool_rxnfc *cmd)
{
	struct ethtool_rx_flow_spec *fsp =
		(struct ethtool_rx_flow_spec *)&cmd->fs;
	struct rnpgbe_fdir_filter *input;
	struct rnpgbe_hw *hw = &adapter->hw;
	/* we don't support mask */
	int err;

	u32 ring_cookie_high = fsp->ring_cookie >> 32;

	if (!(adapter->flags & RNP_FLAG_FDIR_PERFECT_CAPABLE))
		return -EOPNOTSUPP;

	/*
	 * Don't allow programming if the action is a queue greater than
	 * the number of online Rx queues.
	 */
	/* is sriov is on, allow vf and queue */
	/* vf should smaller than num_vfs */
	print_fsp(fsp);
	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED) {
		if ((fsp->ring_cookie != RX_CLS_FLOW_DISC) &&
		    (((ring_cookie_high & 0xff) > adapter->num_vfs) ||
		     ((fsp->ring_cookie & (u64)0xffffffff) >=
		      hw->sriov_ring_limit)))
			return -EINVAL;

	} else {
		if ((fsp->ring_cookie != RX_CLS_FLOW_DISC) &&
		    (fsp->ring_cookie >= adapter->num_rx_queues)) {
			/* ACTION_TO_MPE to mpe special */
			if (fsp->ring_cookie != ACTION_TO_MPE)
				return -EINVAL;
		}
	}

	/* Don't allow indexes to exist outside of available space */
	if (fsp->location >= (adapter->fdir_pballoc)) {
		e_err(drv, "Location out of range\n");
		return -EINVAL;
	}

	input = kzalloc(sizeof(*input), GFP_ATOMIC);
	if (!input)
		return -ENOMEM;

	/* set SW index */
	input->sw_idx = fsp->location;

	/* record flow type */
	if (!rnpgbe_flowspec_to_flow_type(
		    adapter, fsp, &input->filter.formatted.flow_type, input)) {
		e_err(drv, "Unrecognized flow type\n");
		goto err_out;
	}

	if (input->filter.formatted.flow_type == RNP_ATR_FLOW_TYPE_ETHER) {
		/* used to determine hw reg offset */
	} else if (input->filter.formatted.flow_type ==
		   RNP_ATR_FLOW_TYPE_IPV4) {
		/* Copy input into formatted structures */
		input->filter.formatted.src_ip[0] =
			fsp->h_u.usr_ip4_spec.ip4src;
		input->filter.formatted.src_ip_mask[0] =
			fsp->m_u.usr_ip4_spec.ip4src;
		input->filter.formatted.dst_ip[0] =
			fsp->h_u.usr_ip4_spec.ip4dst;
		input->filter.formatted.dst_ip_mask[0] =
			fsp->m_u.usr_ip4_spec.ip4dst;
		input->filter.formatted.src_port = 0;
		input->filter.formatted.src_port_mask = 0xffff;
		input->filter.formatted.dst_port = 0;
		input->filter.formatted.dst_port_mask = 0xffff;
		input->filter.formatted.inner_mac[0] =
			fsp->h_u.usr_ip4_spec.proto;
		input->filter.formatted.inner_mac_mask[0] =
			fsp->m_u.usr_ip4_spec.proto;
	} else { /* tcp or udp or sctp*/
		/* Copy input into formatted structures */
		input->filter.formatted.src_ip[0] =
			fsp->h_u.tcp_ip4_spec.ip4src;
		input->filter.formatted.src_ip_mask[0] =
			fsp->m_u.usr_ip4_spec.ip4src;
		input->filter.formatted.dst_ip[0] =
			fsp->h_u.tcp_ip4_spec.ip4dst;
		input->filter.formatted.dst_ip_mask[0] =
			fsp->m_u.usr_ip4_spec.ip4dst;
		input->filter.formatted.src_port = fsp->h_u.tcp_ip4_spec.psrc;
		input->filter.formatted.src_port_mask =
			fsp->m_u.tcp_ip4_spec.psrc;
		input->filter.formatted.dst_port = fsp->h_u.tcp_ip4_spec.pdst;
		input->filter.formatted.dst_port_mask =
			fsp->m_u.tcp_ip4_spec.pdst;
	}

	/* determine if we need to drop or route the packet */
	if (fsp->ring_cookie == RX_CLS_FLOW_DISC)
		input->action = RNP_FDIR_DROP_QUEUE;
	else {
		input->vf_num = (fsp->ring_cookie >> 32) & 0xff;
		if (input->vf_num) {
			/* in vf mode input->action is the real queue nums */
			input->action =
				hw->sriov_ring_limit *
					(((fsp->ring_cookie >> 32) & 0xff) -
					 1) +
				(fsp->ring_cookie & 0xffffffff);
		} else
			input->action = fsp->ring_cookie;
	}

	spin_lock(&adapter->fdir_perfect_lock);
	err = rnpgbe_update_ethtool_fdir_entry(adapter, input, input->sw_idx);
	spin_unlock(&adapter->fdir_perfect_lock);

	return err;
err_out:
	kfree(input);
	return -EINVAL;
}

static int rnpgbe_del_ethtool_fdir_entry(struct rnpgbe_adapter *adapter,
					 struct ethtool_rxnfc *cmd)
{
	struct ethtool_rx_flow_spec *fsp =
		(struct ethtool_rx_flow_spec *)&cmd->fs;
	int err;

	spin_lock(&adapter->fdir_perfect_lock);
	err = rnpgbe_update_ethtool_fdir_entry(adapter, NULL, fsp->location);
	spin_unlock(&adapter->fdir_perfect_lock);

	return err;
}

int rnpgbe_set_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd)
{
	struct rnpgbe_adapter *adapter = netdev_priv(dev);
	int ret = -EOPNOTSUPP;

	switch (cmd->cmd) {
	case ETHTOOL_SRXCLSRLINS:
		ret = rnpgbe_add_ethtool_fdir_entry(adapter, cmd);
		break;
	case ETHTOOL_SRXCLSRLDEL:
		ret = rnpgbe_del_ethtool_fdir_entry(adapter, cmd);
		break;
	case ETHTOOL_SRXFH:
		ret = rnpgbe_set_rss_hash_opt(adapter, cmd);
		break;
	default:
		break;
	}

	return ret;
}

u32 rnpgbe_rss_indir_size(struct net_device *netdev)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	return rnpgbe_rss_indir_tbl_entries(adapter);
}

u32 rnpgbe_get_rxfh_key_size(struct net_device *netdev)
{
	return RNP_RSS_KEY_SIZE;
}

void rnpgbe_get_reta(struct rnpgbe_adapter *adapter, u32 *indir)
{
	int i, reta_size = rnpgbe_rss_indir_tbl_entries(adapter);
	u16 rss_m = adapter->ring_feature[RING_F_RSS].mask;

	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED)
		rss_m = adapter->ring_feature[RING_F_RSS].indices - 1;

	for (i = 0; i < reta_size; i++)
		indir[i] = adapter->rss_indir_tbl[i] & rss_m;
}

int rnpgbe_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key, u8 *hfunc)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	u8 *seed = NULL;

	if (hfunc) {
		switch (adapter->rss_func_mode) {
		case rss_func_top:
			*hfunc = ETH_RSS_HASH_TOP;
			break;
		case rss_func_xor:
			*hfunc = ETH_RSS_HASH_XOR;
			break;
		case rss_func_order:
			*hfunc = ETH_RSS_HASH_TOP;
			break;
		}
	}

	if (indir)
		rnpgbe_get_reta(adapter, indir);

	if (key)
		seed = key;
	if (seed)
		memcpy(seed, adapter->rss_key, rnpgbe_get_rxfh_key_size(netdev));

	return 0;
}

static int check_fw_type(struct rnpgbe_hw *hw, const u8 *data)
{
	u32 device_id;
	int ret = 0;

	device_id = *((u16 *)data + 30); 

	/* if no device_id no check */
	if ((device_id == 0) || (device_id == 0xffff))
		return 0;

	switch (hw->hw_type) {
	case rnpgbe_hw_n500:
		if (device_id != 0x8308)
			ret = 1;
	break;
	case rnpgbe_hw_n210:
		if (device_id != 0x8208)
			ret = 1;
	break;
	default:
		ret = 1;
	}

	return ret;
}

static int rnpgbe_flash_firmware(struct rnpgbe_adapter *adapter, int region,
				 const u8 *data, int bytes)
{
	struct rnpgbe_hw *hw = &adapter->hw;

	if ((hw->hw_type == rnpgbe_hw_n500) ||
	    (hw->hw_type == rnpgbe_hw_n210)) {
		switch (region) {
		case PART_FW:
			if ((*((u32 *)(data)) != 0xa55aa55a) || 
			    check_fw_type(hw, data))
				return -EINVAL;
			break;
		case PART_CFG:
			if (*((u32 *)(data)) != 0x00010cf9)
				return -EINVAL;
			break;
		case PART_MACSN:
			break;
		case PART_PCSPHY:
			if (*((u16 *)(data)) != 0x081d)
				return -EINVAL;
			break;
		case PART_PXE:
			if (*((u16 *)(data)) != 0xaa55)
				return -EINVAL;
			break;
		default:
			return -EINVAL;
		}
		return rnp500_fw_update(hw, region, data, bytes);

	} else {
		switch (region) {
		case PART_FW:
			if (*((u32 *)(data + 28)) != 0xA51BBEAF)
				return -EINVAL;
			break;
		case PART_CFG:
			if (*((u32 *)(data)) != 0x00010cf9)
				return -EINVAL;
			break;
		case PART_MACSN:
			break;
		case PART_PCSPHY:
			if (*((u16 *)(data)) != 0x081d)
				return -EINVAL;
			break;
		case PART_PXE:
			if (*((u16 *)(data)) != 0xaa55)
				return -EINVAL;
			break;
		default:
			return -EINVAL;
		}
		return rnpgbe_fw_update(hw, region, data, bytes);
	}
}

static int rnpgbe_flash_firmware_from_file(struct net_device *dev,
					   struct rnpgbe_adapter *adapter,
					   int region, const char *filename)
{
	const struct firmware *fw;
	int rc;

	rc = request_firmware(&fw, filename, &dev->dev);
	if (rc != 0) {
		netdev_err(dev, "Error %d requesting firmware file: %s\n", rc,
			   filename);
		return rc;
	}

	rc = rnpgbe_flash_firmware(adapter, region, fw->data, fw->size);
	release_firmware(fw);
	return rc;
}

int rnpgbe_flash_device(struct net_device *dev, struct ethtool_flash *flash)
{
	struct rnpgbe_adapter *adapter = netdev_priv(dev);

	if (IS_VF(adapter->hw.pfvfnum)) {
		netdev_err(dev,
			   "flashdev not supported from a virtual function\n");
		return -EINVAL;
	}

	return rnpgbe_flash_firmware_from_file(dev, adapter, flash->region,
					       flash->data);
}
static int rnpgbe_rss_indir_tbl_max(struct rnpgbe_adapter *adapter)
{
	if (adapter->hw.rss_type == rnpgbe_rss_uv3p)
		return 8;
	else if (adapter->hw.rss_type == rnpgbe_rss_uv440)
		return 128;
	else if (adapter->hw.rss_type == rnpgbe_rss_n10)
		return 128;
	else if (adapter->hw.rss_type == rnpgbe_rss_n500)
		return 128;
	else
		return 128;
}

int rnpgbe_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key,
		    const u8 hfunc)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;
	int i;
	u32 reta_entries = rnpgbe_rss_indir_tbl_entries(adapter);

	if (hfunc) {
		if (hw->ops.set_rss_hfunc) {
			if (hw->ops.set_rss_hfunc(hw, hfunc))
				return -EINVAL;
		} else
			return -EINVAL;

	} else {
		if (hw->ops.set_rss_hfunc)
			hw->ops.set_rss_hfunc(hw, hfunc);
	}

	if ((indir) && (adapter->flags & RNP_FLAG_SRIOV_ENABLED))
		return -EINVAL;
	/* Fill out the redirection table */
	if (indir) {
		int max_queues = min_t(int, adapter->num_rx_queues,
				       rnpgbe_rss_indir_tbl_max(adapter));

		/*Allow max 2 queues w/ SR-IOV.*/
		if ((adapter->flags & RNP_FLAG_SRIOV_ENABLED) &&
		    (max_queues > 1))
			max_queues = 1;

		/* Verify user input. */
		for (i = 0; i < reta_entries; i++) {
			if (indir[i] >= max_queues)
				return -EINVAL;
		}
		/* store rss tbl */
		for (i = 0; i < reta_entries; i++)
			adapter->rss_indir_tbl[i] = indir[i];

		rnpgbe_store_reta(adapter);
	}

	/* Fill out the rss hash key */
	if (key) {
		memcpy(adapter->rss_key, key, rnpgbe_get_rxfh_key_size(netdev));
		rnpgbe_store_key(adapter);
	}
	return 0;
}
