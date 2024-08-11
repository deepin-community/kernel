#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>
#include <linux/netdevice.h>
#include <asm/uaccess.h>
//#include <linux/version.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/nmi.h>

#include "grtnic.h"
#include "grtnic_nvm.h"
#include "grtnic_macphy.h"

#define be32(x) ((x<<24 & 0xff000000) | (x<<8 & 0x00ff0000) | (x>>8 & 0x0000ff00) | (x>>24 & 0x000000ff))
#define be16(x) ((x<<8 & 0xff00) | (x>>8 & 0x00ff))

#ifndef ETH_GSTRING_LEN
#define ETH_GSTRING_LEN 32
#endif


#ifdef ETHTOOL_GSTATS
struct grtnic_stats {
	char stat_string[ETH_GSTRING_LEN];
	int sizeof_stat;
	int stat_offset;
};

#define GRTNIC_NETDEV_STAT(_net_stat) { \
	.stat_string = #_net_stat, \
	.sizeof_stat = sizeof_field(struct net_device_stats, _net_stat), \
	.stat_offset = offsetof(struct net_device_stats, _net_stat) \
}

static const struct grtnic_stats grtnic_gstrings_net_stats[] = {
	GRTNIC_NETDEV_STAT(rx_errors),
	GRTNIC_NETDEV_STAT(tx_errors),
	GRTNIC_NETDEV_STAT(tx_dropped),
	GRTNIC_NETDEV_STAT(rx_length_errors),
	GRTNIC_NETDEV_STAT(rx_over_errors),
	GRTNIC_NETDEV_STAT(rx_frame_errors),
	GRTNIC_NETDEV_STAT(rx_fifo_errors),
	GRTNIC_NETDEV_STAT(tx_fifo_errors),
	GRTNIC_NETDEV_STAT(tx_heartbeat_errors)
};

#define GRTNIC_STAT(_name, _stat) { \
	.stat_string = _name, \
	.sizeof_stat = sizeof_field(struct grtnic_adapter, _stat), \
	.stat_offset = offsetof(struct grtnic_adapter, _stat) \
}

static const struct grtnic_stats grtnic_gstrings_stats[] = {
	GRTNIC_STAT("rx_packets", stats.gprc),
	GRTNIC_STAT("tx_packets", stats.gptc),
	GRTNIC_STAT("rx_bytes", stats.gorc),
	GRTNIC_STAT("tx_bytes", stats.gotc),

	GRTNIC_STAT("lsc_int", lsc_int),
	GRTNIC_STAT("tx_busy", tx_busy),
	GRTNIC_STAT("non_eop_descs", non_eop_descs),
//	GRTNIC_STAT("tx_timeout_count", tx_timeout_count),
	GRTNIC_STAT("tx_restart_queue", restart_queue),
	GRTNIC_STAT("rx_csum_offload_errors", hw_csum_rx_error),
	GRTNIC_STAT("alloc_rx_page", alloc_rx_page),
	GRTNIC_STAT("alloc_rx_page_failed", alloc_rx_page_failed),
	GRTNIC_STAT("alloc_rx_buff_failed", alloc_rx_buff_failed),

	GRTNIC_STAT("rx_broadcast", stats.bprc),
	GRTNIC_STAT("tx_broadcast", stats.bptc),
	GRTNIC_STAT("rx_multicast", stats.mprc),
	GRTNIC_STAT("tx_multicast", stats.mptc),
	GRTNIC_STAT("multicast", stats.mprc),
	GRTNIC_STAT("rx_pause", stats.rxpause),
	GRTNIC_STAT("tx_pause", stats.txpause),
	GRTNIC_STAT("tx_underrun", stats.tx_underrun),
	GRTNIC_STAT("rx_crc_errors", stats.crcerrs),
	GRTNIC_STAT("rx_missed_errors", stats.mpc),
	GRTNIC_STAT("tx_aborted_errors", stats.ecol),
	GRTNIC_STAT("tx_window_errors", stats.latecol),
	GRTNIC_STAT("tx_abort_late_coll", stats.latecol),
	GRTNIC_STAT("tx_deferred_ok", stats.dc),
	GRTNIC_STAT("tx_single_coll_ok", stats.scc),
	GRTNIC_STAT("tx_multi_coll_ok", stats.mcc),
	GRTNIC_STAT("rx_long_length_errors", stats.roc),
	GRTNIC_STAT("rx_short_length_errors", stats.ruc),
	GRTNIC_STAT("rx_align_errors", stats.algnerrc),
	GRTNIC_STAT("rx_long_byte_count", stats.gorc)
};

/* grtnic allocates num_tx_queues and num_rx_queues symmetrically so
 * we set the num_rx_queues to evaluate to num_tx_queues. This is
 * used because we do not have a good way to get the max number of
 * rx queues with CONFIG_RPS disabled.
 */
#ifdef HAVE_TX_MQ
#ifdef HAVE_NETDEV_SELECT_QUEUE
#define GRTNIC_NUM_RX_QUEUES netdev->num_tx_queues
#define GRTNIC_NUM_TX_QUEUES netdev->num_tx_queues
#else
#define GRTNIC_NUM_RX_QUEUES adapter->num_tx_queues
#define GRTNIC_NUM_TX_QUEUES adapter->num_tx_queues
#endif /* HAVE_NETDEV_SELECT_QUEUE */
#else /* HAVE_TX_MQ */
#define GRTNIC_NUM_TX_QUEUES 1
#define GRTNIC_NUM_RX_QUEUES ( \
		((struct grtnic_adapter *)netdev_priv(netdev))->num_rx_queues)
#endif /* HAVE_TX_MQ */

#define GRTNIC_QUEUE_STATS_LEN ( \
		(GRTNIC_NUM_TX_QUEUES + GRTNIC_NUM_RX_QUEUES) * \
		(sizeof(struct grtnic_queue_stats) / sizeof(u64)))

#define GRTNIC_GLOBAL_STATS_LEN ARRAY_SIZE(grtnic_gstrings_stats)
#define GRTNIC_NETDEV_STATS_LEN ARRAY_SIZE(grtnic_gstrings_net_stats)
#define GRTNIC_STATS_LEN \
	(GRTNIC_GLOBAL_STATS_LEN + GRTNIC_NETDEV_STATS_LEN + GRTNIC_QUEUE_STATS_LEN)

#endif /* ETHTOOL_GSTATS */
#ifdef ETHTOOL_TEST
static const char grtnic_gstrings_test[][ETH_GSTRING_LEN] = {
	"Register test  (offline)", "Eeprom test    (offline)",
	"Interrupt test (offline)", "Loopback test  (offline)",
	"Link test   (on/offline)"
};
#define GRTNIC_TEST_LEN	(sizeof(grtnic_gstrings_test) / ETH_GSTRING_LEN)
#endif /* ETHTOOL_TEST */


#ifdef ETHTOOL_GLINKSETTINGS
static int grtnic_get_link_ksettings(struct net_device *netdev, struct ethtool_link_ksettings *cmd)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	int max_port_speed = adapter->speed;
	int port_speed = adapter->link_speed;
	u32 fiber_speed = SPEED_1000;

	ethtool_link_ksettings_zero_link_mode(cmd, supported);
	ethtool_link_ksettings_zero_link_mode(cmd, advertising);

	if(adapter->type==1) //copper
	{
		ethtool_link_ksettings_add_link_mode(cmd, supported, 10baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, supported, 100baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, supported, 1000baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, supported, Autoneg);
		ethtool_link_ksettings_add_link_mode(cmd, supported, TP);
		ethtool_link_ksettings_add_link_mode(cmd, supported, Pause);
		if(max_port_speed)
			ethtool_link_ksettings_add_link_mode(cmd, supported, 10000baseT_Full);

		ethtool_link_ksettings_add_link_mode(cmd, advertising, 10baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, 100baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, 1000baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, Autoneg);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, TP);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, Pause);
		if(max_port_speed)
			ethtool_link_ksettings_add_link_mode(cmd, advertising, 10000baseT_Full);

		cmd->base.port = PORT_TP;
	}

	else //fiber
	{
		ethtool_link_ksettings_add_link_mode(cmd, supported, Autoneg);
		ethtool_link_ksettings_add_link_mode(cmd, supported, FIBRE);
		ethtool_link_ksettings_add_link_mode(cmd, supported, Pause);

		if(max_port_speed)
			ethtool_link_ksettings_add_link_mode(cmd, supported, 10000baseT_Full);
		else
			ethtool_link_ksettings_add_link_mode(cmd, supported, 1000baseT_Full);

		ethtool_link_ksettings_add_link_mode(cmd, advertising, Autoneg);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, FIBRE);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, Pause);
		if(max_port_speed)
			ethtool_link_ksettings_add_link_mode(cmd, advertising, 10000baseT_Full);
		else
			ethtool_link_ksettings_add_link_mode(cmd, advertising, 1000baseT_Full);

		cmd->base.port = PORT_FIBRE;
		fiber_speed = max_port_speed ? SPEED_10000 : SPEED_1000;
	}

	cmd->base.speed = SPEED_UNKNOWN;
	cmd->base.duplex = DUPLEX_UNKNOWN;

	if (netif_running(netdev)) {
		if (netif_carrier_ok(netdev)) {
			cmd->base.speed = (adapter->type==0) ? fiber_speed : (port_speed==0x03) ? SPEED_10000 : (port_speed==0x02) ? SPEED_1000 : (port_speed==0x01) ? SPEED_100 : SPEED_10;
			cmd->base.duplex = DUPLEX_FULL;
		}
	} 

	cmd->base.phy_address = adapter->func;
	cmd->base.autoneg = AUTONEG_ENABLE;

	return 0;
}

static int grtnic_set_link_ksettings(struct net_device *netdev, const struct ethtool_link_ksettings *cmd)
{
	return 0;
}


#else /* !ETHTOOL_GLINKSETTINGS */
static int grtnic_nic_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	int max_port_speed = adapter->speed;
	int port_speed = adapter->link_speed;
	u32 fiber_speed = SPEED_1000;

	if(adapter->type==1) //copper
	{
		ecmd->supported = ( SUPPORTED_100baseT_Full |
				   SUPPORTED_1000baseT_Full|
					(max_port_speed ? SUPPORTED_10000baseT_Full : 0)|
				   SUPPORTED_Autoneg |
				   SUPPORTED_TP |
				   SUPPORTED_Pause);

		ecmd->advertising = ecmd->supported | ADVERTISED_TP | ADVERTISED_Autoneg | ADVERTISED_Pause;
		ecmd->port = PORT_TP;
		ecmd->transceiver = XCVR_INTERNAL;
	}

	else //fiber
	{
		ecmd->supported 	= (max_port_speed ? SUPPORTED_10000baseT_Full : SUPPORTED_1000baseT_Full) | SUPPORTED_FIBRE | SUPPORTED_Autoneg | SUPPORTED_Pause;
		ecmd->advertising = ecmd->supported | ADVERTISED_FIBRE | ADVERTISED_Autoneg | ADVERTISED_Pause;
		ecmd->port = PORT_FIBRE;
		ecmd->transceiver = XCVR_EXTERNAL;
		fiber_speed = max_port_speed ? SPEED_10000 : SPEED_1000;
	}

	ecmd->speed = SPEED_UNKNOWN;
	ecmd->duplex = DUPLEX_UNKNOWN;

	if (netif_running(netdev)) {
		if (netif_carrier_ok(netdev)) {
			ecmd->speed = (adapter->type==0) ? fiber_speed : (port_speed==0x03) ? SPEED_10000 : (port_speed==0x02) ? SPEED_1000 : (port_speed==0x01) ? SPEED_100 : SPEED_10;
			ecmd->duplex = DUPLEX_FULL;
		}
	} 

	ecmd->autoneg = AUTONEG_ENABLE;
	ecmd->phy_address = adapter->func;

	return 0;
}

static int grtnic_nic_set_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
//	struct grtnic_port *grtnic_port = netdev_priv(netdev);
//	struct grtnic_adapter *adapter = grtnic_port->adapter;
//	u32 phy_mode_control_val32 = 0;

	//printk("netdev_no=%d, autoneg=%d, speed=%d duplex=%d\n", port_adapter->netdev_no, ecmd->autoneg, ecmd->speed, ecmd->duplex);

	return 0;
#if 0
	phy_mode_control_val32 = phy_read(adapter, port_adapter->phyid, PHY_MODE_CONTRL_REG);

	if (ecmd->autoneg == AUTONEG_ENABLE) {
		phy_mode_control_val32 |= BIT(12);
		phy_mode_control_val32 |= BIT(6);	/* forced speed selection bit 6,13 */
	} else {
		phy_mode_control_val32 &= ~BIT(12);

		if (ecmd->speed == SPEED_1000) { /* 10 */
			phy_mode_control_val32 |= BIT(6);			
			phy_mode_control_val32 &= ~BIT(13);
		} else if (ecmd->speed == SPEED_100 && port_adapter->support_100M) { /* 01 */
			phy_mode_control_val32 &= ~BIT(6);
			phy_mode_control_val32 |= BIT(13);
		} else
			return -EINVAL;
		
		if (ecmd->duplex == DUPLEX_FULL)
			phy_mode_control_val32 |= BIT(8);	/* full duplex bit 8 */
		else
			phy_mode_control_val32 &= ~BIT(8);
	}

	port_adapter->phy_mode_control_val = phy_mode_control_val32;

	chip_rx_disable(adapter);
	phy_write(adapter, port_adapter->phyid, PHY_MODE_CONTRL_REG, phy_mode_control_val32);
	chip_rx_enable(adapter);

	return 0;
#endif	
}

#endif //* !HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE */

//////////////////////////////////////////////////////////////////////////////////////////

static void grtnic_nic_get_drvinfo(struct net_device *netdev, struct ethtool_drvinfo *drvinfo)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	struct grtnic_hw *hw 						 = &adapter->hw;
	char firmware_version[32];
	char ipxe_version[32];
	u32 hw_version[2];
	u8 offset = adapter->speed ? 1 : 0;
//	u32 sn_h[2];
//	u8 sn_h_l;
//	char sn_s[64];
//	u32 chip_temp;

	strncpy(drvinfo->driver, DRIVER_NAME, 32);
	strncpy(drvinfo->version, DRIVER_VERSION, 32);

//	chip_temp = read_register(adapter->user_bar + TEMP_STATUS);
//	printk("temp = %d\n", (chip_temp*504)/4096-273);
//
//	sn_h[0] = read_register(adapter->user_bar + SERIAL_NO);
//	sn_h[1] = read_register(adapter->user_bar + SERIAL_NO);
//	sn_h_l = read_register(adapter->user_bar + SERIAL_NO) & 0xff;
//	sprintf(sn_s, "%08x%08x%02x", sn_h[0],sn_h[1],sn_h_l);
//	printk("sn = %s\n", sn_s);

	hw_version[0] = GRTNIC_READ_REG(hw, DESIGN_STATUS, 0);
	hw_version[1] = GRTNIC_READ_REG(hw, IPXE_STATUS, 0);

	if(hw_version[0] < 0x200) //maybe old firmware //0x101
	{
		read_flash_buffer(adapter, (VPD_OFFSET - (offset * 0x100000)) + VERSION_OFFSET, 2, (u32 *)&hw_version);
		sprintf(firmware_version, "%08d", hw_version[0] ^ 0xFFFFFFFF);
		sprintf(ipxe_version, "%08d", hw_version[1] ^ 0xFFFFFFFF);
	}
	else
	{
		sprintf(firmware_version, "%08x", hw_version[0]);
		sprintf(ipxe_version, "%08x", hw_version[1]);
	}

	strncpy(drvinfo->fw_version, firmware_version, 32);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
	strncpy(drvinfo->erom_version, ipxe_version, 32);
#endif
	strncpy(drvinfo->bus_info, pci_name(adapter->pdev), 32);
	drvinfo->n_stats = GRTNIC_STATS_LEN;
	drvinfo->testinfo_len = 0;
	drvinfo->regdump_len = 0;
	drvinfo->eedump_len = 0;
}

static void grtnic_get_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *pause)
{
	struct grtnic_adapter *adapter 	 = netdev_priv(netdev);
	struct grtnic_hw *hw 						 = &adapter->hw;
	struct grtnic_mac_info *mac      = &hw->mac;

	pause->autoneg = (mac->fc.fc_autoneg ? AUTONEG_ENABLE : AUTONEG_DISABLE);

	if (mac->fc.current_mode == fc_rx_pause) {
		pause->rx_pause = 1;
	} else if (mac->fc.current_mode == fc_tx_pause) {
		pause->tx_pause = 1;
	} else if (mac->fc.current_mode == fc_full) {
		pause->rx_pause = 1;
		pause->tx_pause = 1;
	}
}

#ifdef HAVE_ETHTOOL_EXTENDED_RINGPARAMS
static void
grtnic_nic_get_ringparam(struct net_device *netdev,
		    struct ethtool_ringparam *ring,
		    struct kernel_ethtool_ringparam __always_unused *ker,
		    struct netlink_ext_ack __always_unused *extack)
#else
static void grtnic_nic_get_ringparam(struct net_device *netdev,
				struct ethtool_ringparam *ring)
#endif /* HAVE_ETHTOOL_EXTENDED_RINGPARAMS */
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);

	ring->rx_max_pending = GRTNIC_MAX_NUM_DESCRIPTORS;
	ring->tx_max_pending = GRTNIC_MAX_NUM_DESCRIPTORS;
	ring->rx_mini_max_pending = 0;
	ring->rx_jumbo_max_pending = 0;
	ring->rx_pending = adapter->rx_ring_count;
	ring->tx_pending = adapter->tx_ring_count;
	ring->rx_mini_pending = 0;
	ring->rx_jumbo_pending = 0;
}

#ifdef HAVE_ETHTOOL_EXTENDED_RINGPARAMS
static int
grtnic_nic_set_ringparam(struct net_device *netdev,
		    struct ethtool_ringparam *ring,
		    struct kernel_ethtool_ringparam __always_unused *ker,
		    struct netlink_ext_ack __always_unused *extack)
#else
static int grtnic_nic_set_ringparam(struct net_device *netdev,
			       struct ethtool_ringparam *ring)
#endif /* HAVE_ETHTOOL_EXTENDED_RINGPARAMS */
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	struct grtnic_ring *temp_ring;
	int i, err = 0;
	u32 new_rx_count, new_tx_count;

	if ((ring->rx_mini_pending) || (ring->rx_jumbo_pending))
		return -EINVAL;

	if (ring->tx_pending > GRTNIC_MAX_NUM_DESCRIPTORS ||
	    ring->tx_pending < GRTNIC_MIN_NUM_DESCRIPTORS ||
	    ring->rx_pending > GRTNIC_MAX_NUM_DESCRIPTORS ||
	    ring->rx_pending < GRTNIC_MIN_NUM_DESCRIPTORS) {
		netdev_info(netdev,
			    "Descriptors requested (Tx: %d / Rx: %d) out of range [%d-%d]\n",
			    ring->tx_pending, ring->rx_pending,
			    GRTNIC_MIN_NUM_DESCRIPTORS,
			    GRTNIC_MAX_NUM_DESCRIPTORS);
		return -EINVAL;
	}

	new_tx_count = ALIGN(ring->tx_pending,
			     GRTNIC_REQ_TX_DESCRIPTOR_MULTIPLE);
	new_rx_count = ALIGN(ring->rx_pending,
			     GRTNIC_REQ_RX_DESCRIPTOR_MULTIPLE);

	if ((new_tx_count == adapter->tx_ring_count) &&
	    (new_rx_count == adapter->rx_ring_count)) {
		/* nothing to do */
		return 0;
	}

	while (test_and_set_bit(__GRTNIC_RESETTING, &adapter->state))
		usleep_range(1000, 2000);

	if (!netif_running(adapter->netdev)) {
		for (i = 0; i < adapter->num_tx_queues; i++)
			adapter->tx_ring[i]->count = new_tx_count;
		for (i = 0; i < adapter->num_rx_queues; i++)
			adapter->rx_ring[i]->count = new_rx_count;
		adapter->tx_ring_count = new_tx_count;
		adapter->rx_ring_count = new_rx_count;
		goto clear_reset;
	}

	/* allocate temporary buffer to store rings in */
	i = max_t(int, adapter->num_tx_queues, adapter->num_rx_queues);
	temp_ring = vmalloc(i * sizeof(struct grtnic_ring));

	if (!temp_ring) {
		err = -ENOMEM;
		goto clear_reset;
	}

	grtnic_down(adapter);

	/*
	 * Setup new Tx resources and free the old Tx resources in that order.
	 * We can then assign the new resources to the rings via a memcpy.
	 * The advantage to this approach is that we are guaranteed to still
	 * have resources even in the case of an allocation failure.
	 */
	if (new_tx_count != adapter->tx_ring_count) {
		for (i = 0; i < adapter->num_tx_queues; i++) {
			memcpy(&temp_ring[i], adapter->tx_ring[i],
			       sizeof(struct grtnic_ring));

			temp_ring[i].count = new_tx_count;
			err = grtnic_setup_tx_resources(&temp_ring[i]);
			if (err) {
				while (i) {
					i--;
					grtnic_free_tx_resources(&temp_ring[i]);
				}
				goto err_setup;
			}
		}

		for (i = 0; i < adapter->num_tx_queues; i++) {
			grtnic_free_tx_resources(adapter->tx_ring[i]);

			memcpy(adapter->tx_ring[i], &temp_ring[i],
			       sizeof(struct grtnic_ring));
		}

		adapter->tx_ring_count = new_tx_count;
	}

	/* Repeat the process for the Rx rings if needed */
	if (new_rx_count != adapter->rx_ring_count) {
		for (i = 0; i < adapter->num_rx_queues; i++) {
			memcpy(&temp_ring[i], adapter->rx_ring[i],
			       sizeof(struct grtnic_ring));

			temp_ring[i].count = new_rx_count;
			err = grtnic_setup_rx_resources(&temp_ring[i]);
			if (err) {
				while (i) {
					i--;
					grtnic_free_rx_resources(&temp_ring[i]);
				}
				goto err_setup;
			}
		}

		for (i = 0; i < adapter->num_rx_queues; i++) {
			grtnic_free_rx_resources(adapter->rx_ring[i]);

			memcpy(adapter->rx_ring[i], &temp_ring[i],
			       sizeof(struct grtnic_ring));
		}

		adapter->rx_ring_count = new_rx_count;
	}

err_setup:
	grtnic_up(adapter);
	vfree(temp_ring);
clear_reset:
	clear_bit(__GRTNIC_RESETTING, &adapter->state);
	return err;
}




static int grtnic_set_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *pause)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;
	struct grtnic_mac_info *mac = &hw->mac;
	u8 flowctl = 0;
	int retval = 0;

	mac->fc.fc_autoneg = pause->autoneg;

	if (mac->fc.fc_autoneg == AUTONEG_ENABLE) {
		mac->fc.requested_mode = fc_full;
	} else {
		if (pause->rx_pause && pause->tx_pause)
			mac->fc.requested_mode = fc_full;
		else if (pause->rx_pause && !pause->tx_pause)
			mac->fc.requested_mode = fc_rx_pause;
		else if (!pause->rx_pause && pause->tx_pause)
			mac->fc.requested_mode = fc_tx_pause;
		else if (!pause->rx_pause && !pause->tx_pause)
			mac->fc.requested_mode = fc_none;

		mac->fc.current_mode = mac->fc.requested_mode;

	}

	if(mac->fc.requested_mode == fc_full) 			    flowctl = 3;
	else if(mac->fc.requested_mode == fc_tx_pause)	flowctl = 2;
	else if(mac->fc.requested_mode == fc_rx_pause)	flowctl = 1;
	else 																						flowctl = 0;

	if(pause->tx_pause)
		grtnic_set_fc_watermarks(netdev);

	GRTNIC_WRITE_REG(hw, ETH_TX_PAUSE, pause->tx_pause, 0);

	grtnic_SetPause(netdev, flowctl);

	return retval;
}


#ifndef HAVE_ETHTOOL_GET_SSET_COUNT
static int grtnic_get_stats_count(struct net_device *netdev)
{
	return GRTNIC_STATS_LEN;
}

static int grtnic_diag_test_count(struct net_device *netdev)
{
	return GRTNIC_TEST_LEN;
}

#else /* HAVE_ETHTOOL_GET_SSET_COUNT */

static int grtnic_get_sset_count(struct net_device *netdev, int sset)
{
#ifdef HAVE_TX_MQ
#ifndef HAVE_NETDEV_SELECT_QUEUE
	struct grtnic_adapter *adapter = netdev_priv(netdev);
#endif
#endif

	switch (sset) {
	case ETH_SS_STATS:
		return GRTNIC_STATS_LEN;
	case ETH_SS_TEST:
		return GRTNIC_TEST_LEN;
	case ETH_SS_PRIV_FLAGS:
//		return IXGBE_PRIV_FLAGS_STR_LEN;
	default:
		return -EOPNOTSUPP;
	}
}

#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */

static void grtnic_get_ethtool_stats(struct net_device *netdev,
				    struct ethtool_stats __always_unused *stats, u64 *data)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);

#ifdef HAVE_NDO_GET_STATS64
	const struct rtnl_link_stats64 *net_stats;
	struct rtnl_link_stats64 temp;
	unsigned int start;
#else
#ifdef HAVE_NETDEV_STATS_IN_NETDEV
	struct net_device_stats *net_stats = &netdev->stats;
#else
	struct net_device_stats *net_stats = &adapter->net_stats;
#endif
#endif
	struct grtnic_ring *ring;
	int i, j;
	char *p;

	grtnic_update_stats(adapter);
#ifdef HAVE_NDO_GET_STATS64
	net_stats = dev_get_stats(netdev, &temp);
#endif

	for (i = 0; i < GRTNIC_NETDEV_STATS_LEN; i++) {
		p = (char *)net_stats + grtnic_gstrings_net_stats[i].stat_offset;
		data[i] = (grtnic_gstrings_net_stats[i].sizeof_stat == sizeof(u64)) ? *(u64 *)p : *(u32 *)p;
	}
	for (j = 0; j < GRTNIC_GLOBAL_STATS_LEN; j++, i++) {
		p = (char *)adapter + grtnic_gstrings_stats[j].stat_offset;
		data[i] = (grtnic_gstrings_stats[j].sizeof_stat == sizeof(u64)) ? *(u64 *)p : *(u32 *)p;
	}

	for (j = 0; j < GRTNIC_NUM_TX_QUEUES; j++) {
		ring = adapter->tx_ring[j];
		if (!ring) {
			data[i++] = 0;
			data[i++] = 0;
#ifdef BP_EXTENDED_STATS
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
#endif
			continue;
		}

#ifdef HAVE_NDO_GET_STATS64
		do {
			start = u64_stats_fetch_begin(&ring->syncp);
#endif
			data[i]   = ring->stats.packets;
			data[i+1] = ring->stats.bytes;
#ifdef HAVE_NDO_GET_STATS64
		} while (u64_stats_fetch_retry(&ring->syncp, start));
#endif
		i += 2;
#ifdef BP_EXTENDED_STATS
		data[i] = ring->stats.yields;
		data[i+1] = ring->stats.misses;
		data[i+2] = ring->stats.cleaned;
		i += 3;
#endif
	}
	for (j = 0; j < GRTNIC_NUM_RX_QUEUES; j++) {
		ring = adapter->rx_ring[j];
		if (!ring) {
			data[i++] = 0;
			data[i++] = 0;
#ifdef BP_EXTENDED_STATS
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
#endif
			continue;
		}

#ifdef HAVE_NDO_GET_STATS64
		do {
			start = u64_stats_fetch_begin(&ring->syncp);
#endif
			data[i]   = ring->stats.packets;
			data[i+1] = ring->stats.bytes;
#ifdef HAVE_NDO_GET_STATS64
		} while (u64_stats_fetch_retry(&ring->syncp, start));
#endif
		i += 2;
#ifdef BP_EXTENDED_STATS
		data[i] = ring->stats.yields;
		data[i+1] = ring->stats.misses;
		data[i+2] = ring->stats.cleaned;
		i += 3;
#endif
	}
}

static void grtnic_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
	char *p = (char *)data;
	unsigned int i;

	switch (stringset) {
	case ETH_SS_TEST:
		memcpy(data, *grtnic_gstrings_test,
		       GRTNIC_TEST_LEN * ETH_GSTRING_LEN);
		break;
	case ETH_SS_STATS:
		for (i = 0; i < GRTNIC_NETDEV_STATS_LEN; i++) {
			memcpy(p, grtnic_gstrings_net_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		for (i = 0; i < GRTNIC_GLOBAL_STATS_LEN; i++) {
			memcpy(p, grtnic_gstrings_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		for (i = 0; i < GRTNIC_NUM_TX_QUEUES; i++) {
			snprintf(p, ETH_GSTRING_LEN,
				 "tx_queue_%u_packets", i);
			p += ETH_GSTRING_LEN;
			snprintf(p, ETH_GSTRING_LEN,
				 "tx_queue_%u_bytes", i);
			p += ETH_GSTRING_LEN;
#ifdef BP_EXTENDED_STATS
			snprintf(p, ETH_GSTRING_LEN,
				 "tx_queue_%u_bp_napi_yield", i);
			p += ETH_GSTRING_LEN;
			snprintf(p, ETH_GSTRING_LEN,
				 "tx_queue_%u_bp_misses", i);
			p += ETH_GSTRING_LEN;
			snprintf(p, ETH_GSTRING_LEN,
				 "tx_queue_%u_bp_cleaned", i);
			p += ETH_GSTRING_LEN;
#endif /* BP_EXTENDED_STATS */
		}
		for (i = 0; i < GRTNIC_NUM_RX_QUEUES; i++) {
			snprintf(p, ETH_GSTRING_LEN,
				 "rx_queue_%u_packets", i);
			p += ETH_GSTRING_LEN;
			snprintf(p, ETH_GSTRING_LEN,
				 "rx_queue_%u_bytes", i);
			p += ETH_GSTRING_LEN;
#ifdef BP_EXTENDED_STATS
			snprintf(p, ETH_GSTRING_LEN,
				 "rx_queue_%u_bp_poll_yield", i);
			p += ETH_GSTRING_LEN;
			snprintf(p, ETH_GSTRING_LEN,
				 "rx_queue_%u_bp_misses", i);
			p += ETH_GSTRING_LEN;
			snprintf(p, ETH_GSTRING_LEN,
				 "rx_queue_%u_bp_cleaned", i);
			p += ETH_GSTRING_LEN;
#endif /* BP_EXTENDED_STATS */
		}
		/* BUG_ON(p - data != IXGBE_STATS_LEN * ETH_GSTRING_LEN); */
		break;
#ifdef HAVE_ETHTOOL_GET_SSET_COUNT
	case ETH_SS_PRIV_FLAGS:
//		memcpy(data, ixgbe_priv_flags_strings,
//		       IXGBE_PRIV_FLAGS_STR_LEN * ETH_GSTRING_LEN);
		break;
#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */
	}
}

#ifdef HAVE_ETHTOOL_SET_PHYS_ID
static int grtnic_nic_set_phys_id(struct net_device *netdev, enum ethtool_phys_id_state state)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;
	u8 led_cmd, led_on;

	led_cmd = 1<<5;
	led_on 	= 1<<4;

	switch (state) {
	case ETHTOOL_ID_ACTIVE:
//		grtnic_port->led_reg = read_register(adapter->user_bar + MAC_LED_CTL);
		return 2;

	case ETHTOOL_ID_ON:
		GRTNIC_WRITE_REG(hw, MAC_LED_CTL, (led_cmd|led_on), 0); //led_start+led_on
		break;

	case ETHTOOL_ID_OFF:
		GRTNIC_WRITE_REG(hw, MAC_LED_CTL, led_cmd, 0); //led_start + led_off
		break;

	case ETHTOOL_ID_INACTIVE:
		/* Restore LED settings */
		GRTNIC_WRITE_REG(hw, MAC_LED_CTL, 0, 0); //led_stop and led_off
		break;
	}

	return 0;
}
#else
static int grtnic_nic_phys_id(struct net_device *netdev, u32 data)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;
	u32 i;
	u8 led_cmd, led_on;

	led_cmd = 1<<5;
	led_on 	= 1<<4;

	if (!data || data > 300)
		data = 300;

	for (i = 0; i < (data * 1000); i += 400) {
		GRTNIC_WRITE_REG(hw, MAC_LED_CTL, (led_cmd|led_on), 0); //led_start+led_on
		msleep_interruptible(200);
		GRTNIC_WRITE_REG(hw, MAC_LED_CTL, led_cmd, 0); //led_start + led_off
		msleep_interruptible(200);
	}

	/* Restore LED settings */

	GRTNIC_WRITE_REG(hw, MAC_LED_CTL, 0, 0); //led_stop and led_off

	return 0;
}
#endif /* HAVE_ETHTOOL_SET_PHYS_ID */

int firmware_is_old(struct grtnic_adapter *adapter)
{
	int old_firmware;
	struct grtnic_hw *hw = &adapter->hw;

	GRTNIC_WRITE_REG(hw, FIRMWARE_CMD, 1, 0);
	old_firmware = !GRTNIC_READ_REG(hw, FIRMWARE_CMD, 0);
	GRTNIC_WRITE_REG(hw, FIRMWARE_CMD, 0, 0);
	return old_firmware;
}

static int grtnic_flash_device(struct net_device *netdev, struct ethtool_flash *flash)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);

	int i = 0;

	u16 temp = 0;
	u16 vid = 0;
	u16 pid = 0;
	int image_type = 0;

	char version_s[64];
	u32 version = 0;
	long version_h;
	long result = 0;

	int pxe_size = 0;
	char ipxe_ver_s[9] = {0};

	const struct firmware *fw;
	const char *filename = flash->data;
	int rc = 0;

	u32 offset = 0;
	u32 copied = 0;

	u32 read_filesize;
 	u32 once_size;

	int firmware_offset;

	int cycle, remainder, schedule;
	u32 offset_int;
  u32 *segment;

  int old_firmware = firmware_is_old(adapter);

	rc = request_firmware(&fw, filename, &netdev->dev);
	if (rc != 0) {
		netdev_err(netdev, "PKG error %d requesting file: %s\n", rc, filename);
		printk("You MUST copy image file to /lib/firmware directory!!!");
		return rc;
	}

	read_filesize = fw->size;
	cycle = read_filesize / FLASH_SECTOR_SIZE;
	remainder = read_filesize % FLASH_SECTOR_SIZE;
	cycle = cycle + (remainder ? 1 : 0);


	firmware_offset = adapter->speed; //10G,link_speed = 1
	temp = *(u16 *) (fw->data);

	if(temp==0xAA55) //maybe pxe image
	{
		image_type = 1;
		vid = *(u16 *) (fw->data+ 0x20);
		pid = *(u16 *) (fw->data+ 0x22);

		pxe_size = read_filesize - 8; //last 8 is ver
		memcpy(ipxe_ver_s, (fw->data + pxe_size), 8);
//		version = strtol(ipxe_ver_s,NULL,0); 

		if(kstrtol(ipxe_ver_s,0,&result))
			result = 0; 
		version = result^0xFFFFFFFF;

		offset = PXE_OFFSET - (firmware_offset * 0x100000);
	}

	else if( (temp&0xff)==0x82 && read_filesize==0x100) //maybe vpd image
	{
		image_type = 2;
		offset = VPD_OFFSET - (firmware_offset * 0x100000);
	}

	else //firmware
	{
		version = *(u32 *)(fw->data);
		pid = *(u16 *) (fw->data+ 0x04); //exchange vid & pid pos
		vid = *(u16 *) (fw->data+ 0x06);
		vid ^=0xFFFF;
		pid ^=0xFFFF;

		offset = old_firmware ? 0 : 0x200000;
	}

	if(image_type!=2) //vpd no vid&pid
	{
		if(vid != adapter->pdev->vendor || pid != adapter->pdev->device)
		{
			printk("Wrong image!\n\n");
			return 0;
		}
	}

	printk("Found %s image File!!! ", (image_type==1) ? "pxe" : (image_type==2) ? "vpd" : "firmware");
	if(image_type==0 || image_type==1)
		printk("and version = %08d", version^0xFFFFFFFF);

	printk("\n\n");


	if(image_type==2) //vpd image no needed vpd & pid
	{
	  segment = vmalloc(FLASH_SUBSECTOR_SIZE);
	  memset(segment, 0x00, FLASH_SUBSECTOR_SIZE);

		read_flash_buffer(adapter, offset, FLASH_SUBSECTOR_SIZE>>2, segment);
		erase_subsector_flash(adapter, offset);

		memcpy(segment, fw->data, read_filesize);

		write_flash_buffer(adapter, offset, FLASH_SUBSECTOR_SIZE>>2, segment);
    vfree(segment);
	}

	else //firmware or pxe image
	{
		GRTNIC_WRITE_REG(&adapter->hw, FIRMWARE_CMD, 1, 0);

		while (read_filesize>0)
		{
			erase_sector_flash(adapter, offset);
			
			if(read_filesize >= FLASH_SECTOR_SIZE)	
				once_size = FLASH_SECTOR_SIZE;
			else
				once_size = read_filesize;

			if(once_size & 0x04)
				write_flash_buffer(adapter, offset, (once_size>>2)+1, (u32 *)(fw->data + copied));
			else
				write_flash_buffer(adapter, offset, once_size>>2, (u32 *)(fw->data + copied));

			schedule = (i+1)*100 / cycle;

			if(i< cycle-1)
				printk("\rUpgrading--->[%d%%]",schedule);
			else
				printk("\rUpgrading--->[%s]\n\n","Done");

			touch_softlockup_watchdog();

			read_filesize = read_filesize - once_size;
			offset += once_size;
			copied += once_size;
			i++;
		}

		GRTNIC_WRITE_REG(&adapter->hw, FIRMWARE_CMD, 0, 0);

		//next write version to flash
		offset_int = VPD_OFFSET - (firmware_offset * 0x100000);
	  segment = vmalloc(FLASH_SUBSECTOR_SIZE);
	  memset(segment, 0x00, FLASH_SUBSECTOR_SIZE);

		read_flash_buffer(adapter, offset_int, FLASH_SUBSECTOR_SIZE>>2, segment);
		erase_subsector_flash(adapter, offset_int);

		sprintf(version_s, "%08d", version ^ 0xFFFFFFFF);
		if(kstrtol(version_s,16,&version_h))
			version_h = 0;
 //save 16jinzhi version for easy asic get version cmd

		segment[(VERSION_OFFSET>>2) + image_type] =  version;
		segment[(VERSION_OFFSET>>2) + 4 + image_type] =  version_h ^ 0xFFFFFFFF;

		write_flash_buffer(adapter, offset_int, FLASH_SUBSECTOR_SIZE>>2, segment);
    vfree(segment);
	}

	release_firmware(fw);

	printk("firmware Update Complete\n");
//  printk("Triggering IPROG to reload ASIC...\n");
//	write_register(0xFEE1DEAD, adapter->user_bar + 0x0054);
  printk("YOU MUST REBOOT COMPUTER TO LET NEW FIRMWARE BEGIN WORKS!\n");
	return rc;
}

#ifdef ETHTOOL_GRXRINGS

static int grtnic_get_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd,
#ifdef HAVE_ETHTOOL_GET_RXNFC_VOID_RULE_LOCS
			   void *rule_locs)
#else
			   u32 *rule_locs)
#endif
{
	struct grtnic_adapter *adapter = netdev_priv(dev);
	int ret = -EOPNOTSUPP;

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
		cmd->data = adapter->num_rx_queues;
		ret = 0;
		break;
//	case ETHTOOL_GRXCLSRLCNT:
//		cmd->rule_cnt = adapter->fdir_filter_count;
//		ret = 0;
//		break;
//	case ETHTOOL_GRXCLSRULE:
//		ret = grtnic_get_ethtool_fdir_entry(adapter, cmd);
//		break;
//	case ETHTOOL_GRXCLSRLALL:
//		ret = grtnic_get_ethtool_fdir_all(adapter, cmd,
//						 (u32 *)rule_locs);
//		break;
//	case ETHTOOL_GRXFH:
//		ret = grtnic_get_rss_hash_opts(adapter, cmd);
//		break;
	default:
		break;
	}

	return ret;
}

#endif /* ETHTOOL_GRXRINGS */

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
static int grtnic_rss_indir_tbl_max(struct grtnic_adapter *adapter)
{
	return 16;
}

static u32 grtnic_get_rxfh_key_size(struct net_device *netdev)
{
	return GRTNIC_RSS_KEY_SIZE;
}

static u32 grtnic_rss_indir_size(struct net_device *netdev)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);

	return grtnic_rss_indir_tbl_entries(adapter);
}

static void grtnic_get_reta(struct grtnic_adapter *adapter, u32 *indir)
{
	int i, reta_size = grtnic_rss_indir_tbl_entries(adapter);
	for (i = 0; i < reta_size; i++)
		indir[i] = adapter->rss_indir_tbl[i];
}

#ifdef HAVE_RXFH_HASHFUNC
static int grtnic_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key, u8 *hfunc)
#else
static int grtnic_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key)
#endif
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc)
		*hfunc = ETH_RSS_HASH_TOP;
#endif

	if (indir)
		grtnic_get_reta(adapter, indir);

	if (key)
		memcpy(key, adapter->rss_key, grtnic_get_rxfh_key_size(netdev));

	return 0;
}

#ifdef HAVE_RXFH_HASHFUNC
static int grtnic_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key, const u8 hfunc)
#else
#ifdef HAVE_RXFH_NONCONST
static int grtnic_set_rxfh(struct net_device *netdev, u32 *indir, u8 *key)
#else
static int grtnic_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key)
#endif /* HAVE_RXFH_NONCONST */
#endif /* HAVE_RXFH_HASHFUNC */
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	int i;
	u32 reta_entries = grtnic_rss_indir_tbl_entries(adapter);

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc)
		return -EINVAL;
#endif

	/* Fill out the redirection table */
	if (indir) {
		int max_queues = min_t(int, adapter->num_rx_queues, grtnic_rss_indir_tbl_max(adapter));

		/* Verify user input. */
		for (i = 0; i < reta_entries; i++)
			if (indir[i] >= max_queues)
				return -EINVAL;

		for (i = 0; i < reta_entries; i++)
			adapter->rss_indir_tbl[i] = indir[i];

		grtnic_store_reta(adapter);
	}

	/* Fill out the rss hash key */
	if (key) {
		memcpy(adapter->rss_key, key, grtnic_get_rxfh_key_size(netdev));
		grtnic_store_key(adapter);
	}

	return 0;
}
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */
//////////////////////////////////////////////////////////////////////////////////////////////
static irqreturn_t grtnic_test_intr(int __always_unused irq, void *data)
{
	struct net_device *netdev = (struct net_device *) data;
	struct grtnic_adapter *adapter = netdev_priv(netdev);

	adapter->test_icr = GRTNIC_READ_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_VECTOR*4), 1);

	return IRQ_HANDLED;
}

static int grtnic_intr_test(struct grtnic_adapter *adapter, u64 *data)
{
	struct net_device *netdev = adapter->netdev;
	u32 mask, i = 0, shared_int = true;
	u32 irq = adapter->pdev->irq;

	if (GRTNIC_REMOVED(adapter->hw.dma_bar)) {
		*data = 1;
		return -1;
	}
	*data = 0;

	/* Hook up test interrupt handler just for this test */
	if (adapter->msix_entries) {
		/* NOTE: we don't test MSI-X interrupts here, yet */
		return 0;
	} else if (adapter->flags & GRTNIC_FLAG_MSI_ENABLED) {
		shared_int = false;
		if (request_irq(irq, &grtnic_test_intr, 0, netdev->name,
				netdev)) {
			*data = 1;
			return -1;
		}
	} else if (!request_irq(irq, &grtnic_test_intr, IRQF_PROBE_SHARED,
				netdev->name, netdev)) {
		shared_int = false;
	} else if (request_irq(irq, &grtnic_test_intr, IRQF_SHARED,
			       netdev->name, netdev)) {
		*data = 1;
		return -1;
	}
	e_info(hw, "testing %s interrupt\n",
	       (shared_int ? "shared" : "unshared"));

	/* Disable all the interrupts */
	GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_IMC*4), 0xFFFFFFFF, 1);
	GRTNIC_WRITE_FLUSH(&adapter->hw);
	usleep_range(10000, 20000);

	/* Test each interrupt */
	for (; i < 2; i++) {
		/* Interrupt to test */
		mask = 1 << i;

		if (!shared_int) {
			/*
			 * Disable the interrupts to be reported in
			 * the cause register and then force the same
			 * interrupt and see if one gets posted.  If
			 * an interrupt was posted to the bus, the
			 * test failed.
			 */
			adapter->test_icr = 0;
			GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_IMC*4), mask, 1);
			GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_ICS*4), mask, 1); //trigger interrupt
			GRTNIC_WRITE_FLUSH(&adapter->hw);
			usleep_range(10000, 20000);

			if (adapter->test_icr & mask) {
				*data = 3;
				break;
			}
		}

		/*
		 * Enable the interrupt to be reported in the cause
		 * register and then force the same interrupt and see
		 * if one gets posted.  If an interrupt was not posted
		 * to the bus, the test failed.
		 */
		adapter->test_icr = 0;
		GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_IMS*4), mask, 1);
		GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_ICS*4), mask, 1); //trigger interrupt
		GRTNIC_WRITE_FLUSH(&adapter->hw);
		usleep_range(10000, 20000);

		if (!(adapter->test_icr & mask)) {
			*data = 4;
			break;
		}

		if (!shared_int) {
			/*
			 * Disable the other interrupts to be reported in
			 * the cause register and then force the other
			 * interrupts and see if any get posted.  If
			 * an interrupt was posted to the bus, the
			 * test failed.
			 */
			adapter->test_icr = 0;
			GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_IMC*4), ~mask & 0x03, 1);
			GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_ICS*4), ~mask & 0x03, 1); //trigger interrupt
			GRTNIC_WRITE_FLUSH(&adapter->hw);
			usleep_range(10000, 20000);

			if (adapter->test_icr) {
				*data = 5;
				break;
			}
		}
	}

	/* Disable all the interrupts */
	GRTNIC_WRITE_REG(&adapter->hw, ((TARGET_IRQ<<12) + ADDR_INTR_IMC*4), 0xFFFFFFFF, 1);
	GRTNIC_WRITE_FLUSH(&adapter->hw);
	usleep_range(10000, 20000);

	/* Unhook test interrupt handler */
	free_irq(irq, netdev);

	return *data;
}


static void grtnic_free_desc_rings(struct grtnic_adapter *adapter)
{
	/* Shut down the DMA engines now so they can be reinitialized later,
	 * since the test rings and normally used rings should overlap on
	 * queue 0 we can just use the standard disable Rx/Tx calls and they
	 * will take care of disabling the test rings for us.
	 */

	/* first Rx */
	grtnic_disable_rx_queue(adapter);

	/* now Tx */
	grtnic_disable_tx_queue(adapter);

	grtnic_reset(adapter);

	grtnic_free_tx_resources(&adapter->test_tx_ring);
	grtnic_free_rx_resources(&adapter->test_rx_ring);
}



static int grtnic_setup_desc_rings(struct grtnic_adapter *adapter)
{
	struct grtnic_ring *tx_ring = &adapter->test_tx_ring;
	struct grtnic_ring *rx_ring = &adapter->test_rx_ring;
	int ret_val;
	int err;

	/* Setup Tx descriptor ring and Tx buffers */
	tx_ring->count = GRTNIC_DEFAULT_TXD;
	tx_ring->queue_index = 0;
	tx_ring->dev = pci_dev_to_dev(adapter->pdev);
	tx_ring->netdev = adapter->netdev;
	tx_ring->reg_idx = adapter->tx_ring[0]->reg_idx;

	err = grtnic_setup_tx_resources(tx_ring);
	if (err)
		return 1;

	grtnic_configure_tx_ring(adapter, tx_ring);

	/* Setup Rx Descriptor ring and Rx buffers */
	rx_ring->count = GRTNIC_DEFAULT_RXD;
	rx_ring->queue_index = 0;
	rx_ring->dev = pci_dev_to_dev(adapter->pdev);
	rx_ring->netdev = adapter->netdev;
	rx_ring->reg_idx = adapter->rx_ring[0]->reg_idx;
	rx_ring->rx_buffer_len = GRTNIC_RXBUFFER_2K;

	err = grtnic_setup_rx_resources(rx_ring);
	if (err) {
		ret_val = 4;
		goto err_nomem;
	}

	grtnic_SetRx(adapter->netdev, 0); 	//stop rx
	GRTNIC_WRITE_REG(&adapter->hw, ASIC_RX_FIFO_RST, 0xff, 0); //reset all channel rx fifo data

	grtnic_configure_rx_ring(adapter, rx_ring);

	grtnic_SetRx(adapter->netdev, 1); 	//start rx

	return 0;

err_nomem:
	grtnic_free_desc_rings(adapter);
	return ret_val;
}

static int grtnic_setup_loopback_test(struct grtnic_adapter *adapter)
{
	struct grtnic_hw *hw = &adapter->hw;
	u32 phy_addr = hw->phy_addr;
	u16 reg_data;
  u8 promisc_mode = 1;

	GRTNIC_WRITE_REG(hw, PHY_TX_DISABLE, 0x00, 0); //enable laser; only for led blink

	if(adapter->ei->type == board_1002E_GRT_FF || adapter->ei->type == board_1005E_GRT_FX)
	{
		//enable loopback	
		grtnic_SetPhyAddr(adapter->netdev, phy_addr, 0x01, 0x00); //prtad_devad_reg //mdio reg:1.0
	  grtnic_PhyRead(adapter->netdev, phy_addr, 0x01, &reg_data);
		reg_data |= 0x01; //loopback 1.0.0
		grtnic_PhyWrite(adapter->netdev, phy_addr, 0x01, reg_data);
	}

	else
	{
	  /* Setup PHY loopback */
	  grtnic_PhyRead(adapter->netdev, phy_addr, 0x00, &reg_data);

		reg_data |= PHY_LOOPBACK;
		reg_data &= ~PHY_ISOLATE;
		reg_data &= ~PHY_AUTO_NEG_EN;

		if(adapter->ei->type == board_902T_GRT_FF)
			reg_data &= ~PHY_POWER_DOWN;

	  grtnic_PhyWrite(adapter->netdev, phy_addr, 0x00, reg_data);

		if(adapter->ei->type == board_902T_GRT_FF)
		{
		  /* Setup mac speed */
			grtnic_ResetRx(adapter->netdev);
		  grtnic_SetSpeed(adapter->netdev, 0x02); //speed 1000
		}
	}

  /*muliticast mode*/
	reg_data = grtnic_GetAdrsFilter(adapter->netdev);
	reg_data |= promisc_mode; //promisc
	grtnic_SetAdrsFilter(adapter->netdev, reg_data);

	GRTNIC_WRITE_REG(hw, CSUM_ENABLE, 0, 0); ////tx rx checksum off

	usleep_range(10000, 20000);
	
	return 0;
}

static void grtnic_loopback_cleanup(struct grtnic_adapter *adapter)
{
	struct grtnic_hw *hw = &adapter->hw;
	u32 phy_addr = hw->phy_addr;
	u16 reg_data;
  u8 promisc_mode = 1;
	u8 csum_tx_mode = 0, csum_rx_mode = 0;

	if(adapter->flags & GRTNIC_FLAG_TXCSUM_CAPABLE) csum_tx_mode = 1;
	if(adapter->flags & GRTNIC_FLAG_RXCSUM_CAPABLE) csum_rx_mode = 1;
	GRTNIC_WRITE_REG(hw, CSUM_ENABLE, (csum_rx_mode << 1 | csum_tx_mode), 0); //告诉asic, tx checksum offload

	if(adapter->ei->type == board_1002E_GRT_FF || adapter->ei->type == board_1005E_GRT_FX)
	{
		//disable loopback	
		grtnic_SetPhyAddr(adapter->netdev, phy_addr, 0x01, 0x00); //prtad_devad_reg //mdio reg:1.0
	  grtnic_PhyRead(adapter->netdev, phy_addr, 0x01, &reg_data);
		reg_data &= ~0x01; //clear loopback 1.0.0
		grtnic_PhyWrite(adapter->netdev, phy_addr, 0x01, reg_data);
	}

	else
	{
		/* Clear PHY loopback */
	  grtnic_PhyRead(adapter->netdev, phy_addr, 0x00, &reg_data);
		reg_data &= ~PHY_LOOPBACK;
		reg_data |= PHY_AUTO_NEG_EN;

		if(adapter->ei->type == board_902T_GRT_FF)
			reg_data |= PHY_POWER_DOWN;
	  grtnic_PhyWrite(adapter->netdev, phy_addr, 0x00, reg_data);
	}

  /*Clear muliticast mode*/
	reg_data = grtnic_GetAdrsFilter(adapter->netdev);
	reg_data &= ~promisc_mode; //promisc
	grtnic_SetAdrsFilter(adapter->netdev, reg_data);
}

static void grtnic_create_lbtest_frame(struct sk_buff *skb, unsigned int frame_size)
{
	memset(skb->data, 0xFF, frame_size);
	frame_size >>= 1;
	memset(&skb->data[frame_size], 0xAA, frame_size / 2 - 1);
	memset(&skb->data[frame_size + 10], 0xBE, 1);
	memset(&skb->data[frame_size + 12], 0xAF, 1);
}

static bool grtnic_check_lbtest_frame(struct grtnic_rx_buffer *rx_buffer, unsigned int frame_size)
{
	unsigned char *data;
	bool match = true;

	frame_size >>= 1;

#ifdef CONFIG_DISABLE_PACKET_SPLIT
	data = rx_buffer->skb->data;
#else
	data = kmap(rx_buffer->page) + rx_buffer->page_offset;
#endif

	if (data[3] != 0xFF ||
	    data[frame_size + 10] != 0xBE ||
	    data[frame_size + 12] != 0xAF)
		match = false;

#ifndef CONFIG_DISABLE_PACKET_SPLIT
	kunmap(rx_buffer->page);

#endif
	return match;
}

static u16 grtnic_clean_test_rings(struct grtnic_ring *rx_ring, struct grtnic_ring *tx_ring, unsigned int size)
{
  union grtnic_rx_desc *rx_desc;
#ifdef CONFIG_DISABLE_PACKET_SPLIT
	const int bufsz = rx_ring->rx_buffer_len;
#else
	const int bufsz = grtnic_rx_bufsz(rx_ring);
#endif
	u16 rx_ntc, tx_ntc, count = 0;

	/* initialize next to clean and descriptor values */
	rx_ntc = rx_ring->next_to_clean;
	tx_ntc = tx_ring->next_to_clean;
	rx_desc = GRTNIC_RX_DESC(*rx_ring, rx_ntc);

	while (tx_ntc != tx_ring->next_to_use) {
		union grtnic_tx_desc *tx_desc;
		struct grtnic_tx_buffer *tx_buffer;

		tx_desc = GRTNIC_TX_DESC(*tx_ring, tx_ntc);

		/* if DD is not set transmit has not completed */
    if (!tx_desc->wb.len_ctl.cmp)
			return count;

		/* unmap buffer on Tx side */
		tx_buffer = &tx_ring->tx_buffer_info[tx_ntc];

		/* Free all the Tx ring sk_buffs */
		dev_kfree_skb_any(tx_buffer->skb);

		/* unmap skb header data */
		dma_unmap_single(tx_ring->dev,
				 dma_unmap_addr(tx_buffer, dma),
				 dma_unmap_len(tx_buffer, len),
				 DMA_TO_DEVICE);
		dma_unmap_len_set(tx_buffer, len, 0);

		/* increment Tx next to clean counter */
		tx_ntc++;
		if (tx_ntc == tx_ring->count)
			tx_ntc = 0;
	}

	while (rx_desc->wb.upper.len_ctl.cmp) {
		struct grtnic_rx_buffer *rx_buffer;

		/* check Rx buffer */
		rx_buffer = &rx_ring->rx_buffer_info[rx_ntc];

		/* sync Rx buffer for CPU read */
		dma_sync_single_for_cpu(rx_ring->dev,
					rx_buffer->dma,
					bufsz,
					DMA_FROM_DEVICE);

		/* verify contents of skb */
		if (grtnic_check_lbtest_frame(rx_buffer, size))
			count++;
		else
			break;

		/* sync Rx buffer for device write */
		dma_sync_single_for_device(rx_ring->dev,
					   rx_buffer->dma,
					   bufsz,
					   DMA_FROM_DEVICE);

		/* increment Rx next to clean counter */
		rx_ntc++;
		if (rx_ntc == rx_ring->count)
			rx_ntc = 0;

		/* fetch next descriptor */
		rx_desc = GRTNIC_RX_DESC(*rx_ring, rx_ntc);
	}

	/* re-map buffers to ring, store next to clean values */
	grtnic_alloc_rx_buffers(rx_ring, count);
	rx_ring->next_to_clean = rx_ntc;
	tx_ring->next_to_clean = tx_ntc;

	return count;
}

#define DESC_PER_LOOP 64

static int grtnic_run_loopback_test(struct grtnic_adapter *adapter)
{
	struct grtnic_ring *tx_ring = &adapter->test_tx_ring;
	struct grtnic_ring *rx_ring = &adapter->test_rx_ring;
	int i, j, lc, ret_val = 0;
	unsigned int size = 1024;
	netdev_tx_t tx_ret_val;
	struct sk_buff *skb;

	/* allocate test skb */
	skb = alloc_skb(size, GFP_KERNEL);
	if (!skb)
		return 11;

	/* place data into test skb */
	grtnic_create_lbtest_frame(skb, size);
	skb_put(skb, size);

	/*
	 * Calculate the loop count based on the largest descriptor ring
	 * The idea is to wrap the largest ring a number of times using 64
	 * send/receive pairs during each loop
	 */

	if (rx_ring->count <= tx_ring->count)
		lc = ((tx_ring->count / DESC_PER_LOOP) * 2) + 1;
	else
		lc = ((rx_ring->count / DESC_PER_LOOP) * 2) + 1;

	for (j = 0; j <= lc; j++) {
		unsigned int good_cnt;

		/* reset count of good packets */
		good_cnt = 0;

		/* place 64 packets on the transmit queue*/
		for (i = 0; i < DESC_PER_LOOP; i++) {
			skb_get(skb);
			tx_ret_val = grtnic_xmit_frame_ring(skb,
							   adapter,
							   tx_ring);
			if (tx_ret_val == NETDEV_TX_OK)
				good_cnt++;
		}

		if (good_cnt != DESC_PER_LOOP) {
			ret_val = 12;
			break;
		}

		/* allow 200 milliseconds for packets to go from Tx to Rx */
		msleep(200);

		good_cnt = grtnic_clean_test_rings(rx_ring, tx_ring, size);
		if (good_cnt != DESC_PER_LOOP) {
			ret_val = 13;
			break;
		}

	}

	/* free the original skb */
	kfree_skb(skb);

	return ret_val;
}


static int grtnic_loopback_test(struct grtnic_adapter *adapter, u64 *data)
{
	*data = grtnic_setup_desc_rings(adapter);
	if (*data)
		goto out;

	*data = grtnic_setup_loopback_test(adapter);
	if (*data)
		goto err_loopback;

	*data = grtnic_run_loopback_test(adapter);
	grtnic_loopback_cleanup(adapter);

err_loopback:
	grtnic_free_desc_rings(adapter);
out:
	return *data;
}

static bool grtnic_eeprom_test(struct grtnic_adapter *adapter, u64 *data)
{
	int firmware_offset = adapter->speed; //10G,link_speed = 1
	int offset_int = 0xF00000 - (firmware_offset * 0x100000);
	u32 ident_id;

	read_flash_buffer(adapter, offset_int, 1, &ident_id);

	if (ident_id != 0x665599AA) {
		*data = 1;
		return true;
	} else {
		*data = 0;
		return false;
	}
}


/* ethtool register test data */
struct grtnic_reg_test {
	u16 reg;
	u8  array_len;
	u8  test_type;
	u32 mask;
	u32 write;
};

#define PATTERN_TEST	1
#define SET_READ_TEST	2
#define WRITE_NO_TEST	3
#define TABLE32_TEST	4
#define TABLE64_TEST_LO	5
#define TABLE64_TEST_HI	6

static struct grtnic_reg_test reg_test[] = {
	{ MAX_LED_PKT_NUM, 1, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
//	{ MAC_ADRS_LOW, 1, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ MAC_ADRS_HIGH, 1, PATTERN_TEST, 0x0000FFFF, 0x0000FFFF },
	{ FC_WATERMARK, 1, PATTERN_TEST, 0x3FFFFFFF, 0x3FFFFFFF },
	{ MAC_ADRS_FILTER, 1, SET_READ_TEST, 0x00000007, 0x00000007 },
	{ CSUM_ENABLE, 1, SET_READ_TEST, 0x00000003, 0x00000003 },
	{ .reg = 0 }
};

static bool reg_set_and_check(struct grtnic_adapter *adapter, u64 *data, int reg,
			      u32 mask, u32 write)
{
	u32 val, before;

	before = GRTNIC_READ_REG(&adapter->hw, reg, 0);
	GRTNIC_WRITE_REG(&adapter->hw, reg, write & mask, 0);
	usleep_range(10, 20); //wait for data stable
	val = GRTNIC_READ_REG(&adapter->hw, reg, 0);

	if ((write & mask) != (val & mask)) {
		e_err(drv,
		      "set/check reg %04X test failed: got 0x%08X expected 0x%08X\n",
		      reg, (val & mask), (write & mask));
		*data = reg;
		GRTNIC_WRITE_REG(&adapter->hw, reg, before, 0);
		return true;
	}
	GRTNIC_WRITE_REG(&adapter->hw, reg, before, 0);
	return false;
}


static bool reg_pattern_test(struct grtnic_adapter *adapter, u64 *data, int reg,
			     u32 mask, u32 write)
{
	u32 pat, val, before;
	static const u32 test_pattern[] = {
		0x5A5A5A5A, 0xA5A5A5A5, 0x00000000, 0xFFFFFFFF
	};

	for (pat = 0; pat < ARRAY_SIZE(test_pattern); pat++) {
		before = GRTNIC_READ_REG(&adapter->hw, reg, 0);
		GRTNIC_WRITE_REG(&adapter->hw, reg, test_pattern[pat] & write, 0);
		usleep_range(10, 20); //wait for data stable
		val = GRTNIC_READ_REG(&adapter->hw, reg, 0);
		if (val != (test_pattern[pat] & write & mask)) {
			e_err(drv,
			      "pattern test reg %04X failed: got 0x%08X expected 0x%08X\n",
			      reg, val, test_pattern[pat] & write & mask);
			*data = reg;
			GRTNIC_WRITE_REG(&adapter->hw, reg, before, 0);
			return true;
		}
		GRTNIC_WRITE_REG(&adapter->hw, reg, before, 0);
	}
	return false;
}

static bool grtnic_reg_test(struct grtnic_adapter *adapter, u64 *data)
{
	struct grtnic_reg_test *test;
	struct grtnic_hw *hw = &adapter->hw;
	u32 i;

	if (GRTNIC_REMOVED(hw->user_bar)) {
		e_err(drv, "Adapter removed - register test blocked\n");
		*data = 1;
		return true;
	}

	test = reg_test;

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
						      test->mask,
						      test->write);
				break;
			case SET_READ_TEST:
				b = reg_set_and_check(adapter, data,
						       test->reg + (i * 0x40),
						       test->mask,
						       test->write);
				break;
			case WRITE_NO_TEST:
				GRTNIC_WRITE_REG(hw, test->reg + (i * 0x40),
						test->write, 0);
				break;
			case TABLE32_TEST:
				b = reg_pattern_test(adapter, data,
						      test->reg + (i * 4),
						      test->mask,
						      test->write);
				break;
			case TABLE64_TEST_LO:
				b = reg_pattern_test(adapter, data,
						      test->reg + (i * 8),
						      test->mask,
						      test->write);
				break;
			case TABLE64_TEST_HI:
				b = reg_pattern_test(adapter, data,
						      (test->reg + 4) + (i * 8),
						      test->mask,
						      test->write);
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


static void grtnic_diag_test(struct net_device *netdev,
			    struct ethtool_test *eth_test, u64 *data)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	bool if_running = netif_running(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	if (GRTNIC_REMOVED(hw->user_bar)) {
		e_err(hw, "Adapter removed - test blocked\n");
		data[0] = 1;
		data[1] = 1;
		data[2] = 1;
		data[3] = 1;
		data[4] = 1;
		eth_test->flags |= ETH_TEST_FL_FAILED;
		return;
	}
	set_bit(__GRTNIC_TESTING, &adapter->state);
	if (eth_test->flags == ETH_TEST_FL_OFFLINE) {

		/* Offline tests */
		e_info(hw, "offline testing starting\n");

		/* Link test performed before hardware reset so autoneg doesn't
		 * interfere with test result */
	  if(GRTNIC_READ_REG(hw, XPHY_STATUS, 0) & 0x01) //link up
			data[4] = 0;
		else {
			data[4] = 1;
			eth_test->flags |= ETH_TEST_FL_FAILED;
	  }

		if (if_running)
			/* indicate we're in test mode */
			grtnic_close(netdev);
		else
			grtnic_reset(adapter);

		e_info(hw, "register testing starting\n");
		if (grtnic_reg_test(adapter, &data[0]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		grtnic_reset(adapter);
		e_info(hw, "eeprom testing starting\n");
		if (grtnic_eeprom_test(adapter, &data[1]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		grtnic_reset(adapter);
		e_info(hw, "interrupt testing starting\n");
		if (grtnic_intr_test(adapter, &data[2]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		grtnic_reset(adapter);

		e_info(hw, "loopback testing starting\n");
		if (grtnic_loopback_test(adapter, &data[3]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		grtnic_reset(adapter);

		/* clear testing bit and return adapter to previous state */
		clear_bit(__GRTNIC_TESTING, &adapter->state);

		if (if_running)
			grtnic_open(netdev);
		else
			GRTNIC_WRITE_REG(hw, PHY_TX_DISABLE, 0x01, 0); //disable laser;
	} else {
		e_info(hw, "online testing starting\n");

		/* Online tests */
	  if(GRTNIC_READ_REG(hw, XPHY_STATUS, 0) & 0x01) //link up
			data[4] = 0;
		else {
			data[4] = 1;
			eth_test->flags |= ETH_TEST_FL_FAILED;
	  }

		/* Offline tests aren't run; pass by default */
		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;

		clear_bit(__GRTNIC_TESTING, &adapter->state);
	}

	msleep_interruptible(4 * 1000);
}

#ifdef ETHTOOL_GMODULEINFO
static int grtnic_get_module_info(struct net_device *dev,
				       struct ethtool_modinfo *modinfo)
{
	struct grtnic_adapter *adapter = netdev_priv(dev);
	struct grtnic_hw *hw = &adapter->hw;
	u32 status;
	u8 sff8472_rev, addr_mode;
	bool page_swap = false;

	if(adapter->type!=0) //not fiber
		return 0;

	/* Check whether we support SFF-8472 or not */
	status = grtnic_read_i2c_eeprom(hw, GRTNIC_SFF_SFF_8472_COMP, &sff8472_rev);
	if (status != 0)
		return -EIO;

	/* addressing mode is not supported */
	status = grtnic_read_i2c_eeprom(hw, GRTNIC_SFF_SFF_8472_SWAP, &addr_mode);
	if (status != 0)
		return -EIO;

	if (addr_mode & GRTNIC_SFF_ADDRESSING_MODE) {
		e_err(drv, "Address change required to access page 0xA2, but not supported. Please report the module type to the driver maintainers.\n");
		page_swap = true;
	}

	if (sff8472_rev == GRTNIC_SFF_SFF_8472_UNSUP || page_swap) {
		/* We have a SFP, but it does not support SFF-8472 */
		modinfo->type = ETH_MODULE_SFF_8079;
		modinfo->eeprom_len = ETH_MODULE_SFF_8079_LEN;
	} else {
		/* We have a SFP which supports a revision of SFF-8472. */
		modinfo->type = ETH_MODULE_SFF_8472;
		modinfo->eeprom_len = ETH_MODULE_SFF_8472_LEN;
	}

	return 0;
}

static int grtnic_get_module_eeprom(struct net_device *dev, struct ethtool_eeprom *ee, u8 *data)
{
	struct grtnic_adapter *adapter = netdev_priv(dev);
	struct grtnic_hw *hw = &adapter->hw;
	u32 status = GRTNIC_ERR_PHY_ADDR_INVALID;
	u8 databyte = 0xFF;
	int i = 0;

	if (ee->len == 0)
		return -EINVAL;

	for (i = ee->offset; i < ee->offset + ee->len; i++) {
		/* I2C reads can take long time */
		if (test_bit(__GRTNIC_IN_SFP_INIT, &adapter->state))
			return -EBUSY;

		if (i < ETH_MODULE_SFF_8079_LEN)
			status = grtnic_read_i2c_eeprom(hw, i, &databyte);
		else
			status = grtnic_read_i2c_sff8472(hw, i, &databyte);

		if (status != 0)
			return -EIO;

		data[i - ee->offset] = databyte;
	}

	return 0;
}
#endif /* ETHTOOL_GMODULEINFO */

#ifndef HAVE_NDO_SET_FEATURES
static u32 grtnic_get_rx_csum(struct net_device *netdev)
{
	return !!(netdev->features & NETIF_F_RXCSUM);
}

static int grtnic_set_rx_csum(struct net_device *netdev, u32 data)
{

	if (data)
		netdev->features |= NETIF_F_RXCSUM;
	else
		netdev->features &= ~NETIF_F_RXCSUM;

	return 0;
}

static int grtnic_set_tx_csum(struct net_device *netdev, u32 data)
{

	if (data)
		netdev->features |= NETIF_F_HW_CSUM;
	else
		netdev->features &= ~NETIF_F_HW_CSUM;

	return 0;
}
#endif /* HAVE_NDO_SET_FEATURES */

static int grtnic_get_coalesce(struct net_device *netdev,
#ifdef HAVE_ETHTOOL_COALESCE_EXTACK
			      struct ethtool_coalesce *ec,
			      struct kernel_ethtool_coalesce *kernel_coal,
			      struct netlink_ext_ack *extack)
#else
			      struct ethtool_coalesce *ec)
#endif
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);

	ec->tx_max_coalesced_frames_irq = adapter->tx_work_limit;
	/* only valid if in constant ITR mode */
	if (adapter->rx_itr_setting <= 1)
		ec->rx_coalesce_usecs = adapter->rx_itr_setting;
	else
		ec->rx_coalesce_usecs = adapter->rx_itr_setting >> 2;

	/* if in mixed tx/rx queues per vector mode, report only rx settings */
	if (adapter->q_vector[0]->tx.count && adapter->q_vector[0]->rx.count)
		return 0;

	/* only valid if in constant ITR mode */
	if (adapter->tx_itr_setting <= 1)
		ec->tx_coalesce_usecs = adapter->tx_itr_setting;
	else
		ec->tx_coalesce_usecs = adapter->tx_itr_setting >> 2;

	return 0;
}

static int grtnic_set_coalesce(struct net_device *netdev,
#ifdef HAVE_ETHTOOL_COALESCE_EXTACK
			      struct ethtool_coalesce *ec,
			      struct kernel_ethtool_coalesce *kernel_coal,
			      struct netlink_ext_ack *extack)
#else
			      struct ethtool_coalesce *ec)
#endif
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	int i;
	u16 tx_itr_param, rx_itr_param;
	u16  tx_itr_prev;
	bool need_reset = false;

	if (adapter->q_vector[0]->tx.count && adapter->q_vector[0]->rx.count) {
		/* reject Tx specific changes in case of mixed RxTx vectors */
		if (ec->tx_coalesce_usecs)
			return -EINVAL;
		tx_itr_prev = adapter->rx_itr_setting;
	} else {
		tx_itr_prev = adapter->tx_itr_setting;
	}

	if (ec->tx_max_coalesced_frames_irq)
		adapter->tx_work_limit = ec->tx_max_coalesced_frames_irq;

	if ((ec->rx_coalesce_usecs > (MAX_EITR >> 2)) ||
	    (ec->tx_coalesce_usecs > (MAX_EITR >> 2)))
		return -EINVAL;

	if (ec->rx_coalesce_usecs > 1)
		adapter->rx_itr_setting = ec->rx_coalesce_usecs << 2;
	else
		adapter->rx_itr_setting = ec->rx_coalesce_usecs;

	if (adapter->rx_itr_setting == 1)
		rx_itr_param = GRTNIC_20K_ITR;
	else
		rx_itr_param = adapter->rx_itr_setting;

	if (ec->tx_coalesce_usecs > 1)
		adapter->tx_itr_setting = ec->tx_coalesce_usecs << 2;
	else
		adapter->tx_itr_setting = ec->tx_coalesce_usecs;

	if (adapter->tx_itr_setting == 1)
		tx_itr_param = GRTNIC_12K_ITR;
	else
		tx_itr_param = adapter->tx_itr_setting;

	/* mixed Rx/Tx */
	if (adapter->q_vector[0]->tx.count && adapter->q_vector[0]->rx.count)
		adapter->tx_itr_setting = adapter->rx_itr_setting;

	/* detect ITR changes that require update of TXDCTL.WTHRESH */
	if ((adapter->tx_itr_setting != 1) &&
	    (adapter->tx_itr_setting < GRTNIC_100K_ITR)) {
		if ((tx_itr_prev == 1) ||
		    (tx_itr_prev >= GRTNIC_100K_ITR))
			need_reset = true;
	} else {
		if ((tx_itr_prev != 1) &&
		    (tx_itr_prev < GRTNIC_100K_ITR))
			need_reset = true;
	}

	/* check the old value and enable RSC if necessary */
//	need_reset |= grtnic_update_rsc(adapter);

//	if (adapter->hw.mac.dmac_config.watchdog_timer &&
//	    (!adapter->rx_itr_setting && !adapter->tx_itr_setting)) {
//		e_info(probe,
//		       "Disabling DMA coalescing because interrupt throttling is disabled\n");
//		adapter->hw.mac.dmac_config.watchdog_timer = 0;
//		ixgbe_dmac_config(&adapter->hw);
//	}

	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct grtnic_q_vector *q_vector = adapter->q_vector[i];

		q_vector->tx.work_limit = adapter->tx_work_limit;
		if (q_vector->tx.count && !q_vector->rx.count)
			/* tx only */
			q_vector->itr = tx_itr_param;
		else
			/* rx only or mixed */
			q_vector->itr = rx_itr_param;
		grtnic_write_itr(q_vector);
	}

	/*
	 * do reset here at the end to make sure EITR==0 case is handled
	 * correctly w.r.t stopping tx, and changing TXDCTL.WTHRESH settings
	 * also locks in RSC enable/disable which requires reset
	 */
	if (need_reset)
		grtnic_do_reset(netdev);

	return 0;
}

static u32 grtnic_get_msglevel(struct net_device *netdev)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	return adapter->msg_enable;
}

static void grtnic_set_msglevel(struct net_device *netdev, u32 data)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	adapter->msg_enable = data;
}

/////////////////////////////////////////////////////////////////////////////////////////////
static struct ethtool_ops grtnic_nic_ethtool_ops = {
#ifdef ETHTOOL_GLINKSETTINGS
	.get_link_ksettings	= grtnic_get_link_ksettings,
	.set_link_ksettings	= grtnic_set_link_ksettings,
#else
	.get_settings			= grtnic_nic_get_settings,
	.set_settings			= grtnic_nic_set_settings,
#endif
	.get_drvinfo			= grtnic_nic_get_drvinfo,
	.get_link					= ethtool_op_get_link,
	.get_ringparam		= grtnic_nic_get_ringparam,
	.set_ringparam		= grtnic_nic_set_ringparam,
	.get_pauseparam		= grtnic_get_pauseparam,
	.set_pauseparam		= grtnic_set_pauseparam,
	.get_msglevel			= grtnic_get_msglevel,
	.set_msglevel			= grtnic_set_msglevel,
#ifndef HAVE_ETHTOOL_GET_SSET_COUNT
	.self_test_count	= grtnic_diag_test_count,
#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */
	.self_test				= grtnic_diag_test,
	.get_strings			= grtnic_get_strings,

#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
#ifdef HAVE_ETHTOOL_SET_PHYS_ID
	.set_phys_id			= grtnic_nic_set_phys_id,
#else
	.phys_id					= grtnic_nic_phys_id,
#endif /* HAVE_ETHTOOL_SET_PHYS_ID */
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
#ifndef HAVE_ETHTOOL_GET_SSET_COUNT
	.get_stats_count	= grtnic_get_stats_count,
#else /* HAVE_ETHTOOL_GET_SSET_COUNT */
	.get_sset_count		= grtnic_get_sset_count,
#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */
	.get_ethtool_stats      = grtnic_get_ethtool_stats,
#ifdef HAVE_ETHTOOL_GET_PERM_ADDR
	.get_perm_addr		= ethtool_op_get_perm_addr,
#endif
	.get_coalesce		= grtnic_get_coalesce,
	.set_coalesce		= grtnic_set_coalesce,
#ifdef ETHTOOL_COALESCE_USECS
	.supported_coalesce_params = ETHTOOL_COALESCE_USECS,
#endif
#ifndef HAVE_NDO_SET_FEATURES
	.get_rx_csum		= grtnic_get_rx_csum,
	.set_rx_csum		= grtnic_set_rx_csum,
	.get_tx_csum		= ethtool_op_get_tx_csum,
	.set_tx_csum		= grtnic_set_tx_csum,
	.get_sg					= ethtool_op_get_sg,
	.set_sg					= ethtool_op_set_sg,
#endif /* HAVE_NDO_SET_FEATURES */

#ifdef ETHTOOL_GRXRINGS
	.get_rxnfc		= grtnic_get_rxnfc,
//	.set_rxnfc		= ixgbe_set_rxnfc,
//#ifdef ETHTOOL_SRXNTUPLE
//	.set_rx_ntuple		= ixgbe_set_rx_ntuple,
//#endif
#endif /* ETHTOOL_GRXRINGS */

#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
#ifdef ETHTOOL_GMODULEINFO
	.get_module_info	= grtnic_get_module_info,
	.get_module_eeprom	= grtnic_get_module_eeprom,
#endif
#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
	.get_rxfh_indir_size	= grtnic_rss_indir_size,
	.get_rxfh_key_size		= grtnic_get_rxfh_key_size,
	.get_rxfh		= grtnic_get_rxfh,
	.set_rxfh		= grtnic_set_rxfh,
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */

	.flash_device = grtnic_flash_device,
};

#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
static const struct ethtool_ops_ext grtnic_ethtool_ops_ext = {
	.size			= sizeof(struct ethtool_ops_ext),
	.set_phys_id		= grtnic_nic_set_phys_id,

#ifdef ETHTOOL_GMODULEINFO
	.get_module_info	= grtnic_get_module_info,
	.get_module_eeprom	= grtnic_get_module_eeprom,
#endif

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
	.get_rxfh_indir_size	= grtnic_rss_indir_size,
	.get_rxfh_key_size		= grtnic_get_rxfh_key_size,
	.get_rxfh		= grtnic_get_rxfh,
	.set_rxfh		= grtnic_set_rxfh,
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */
};

#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */


void grtnic_set_ethtool_ops(struct net_device *netdev)
{
#ifndef ETHTOOL_OPS_COMPAT
	netdev->ethtool_ops = &grtnic_nic_ethtool_ops;
#else
	SET_ETHTOOL_OPS(netdev, &grtnic_nic_ethtool_ops);
#endif

#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
	set_ethtool_ops_ext(netdev, &grtnic_ethtool_ops_ext);
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
}