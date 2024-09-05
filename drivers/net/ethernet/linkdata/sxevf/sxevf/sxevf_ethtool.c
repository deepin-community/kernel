#include "sxe_version.h"
#include "sxevf_ethtool.h"
#include "sxevf_hw.h"
#include "sxe_log.h"
#include "sxevf_tx_proc.h"
#include "sxevf_rx_proc.h"
#include "sxevf_netdev.h"
#include "sxevf_msg.h"
#include "sxevf_irq.h"

#define SXEVF_DIAG_REGS_TEST 0
#define SXEVF_DIAG_LINK_TEST 1

#define SXEVF_TEST_SLEEP_TIME 4

#define SXEVF_ETHTOOL_DUMP_REGS_NUM (sxevf_reg_dump_num_get())
#define SXEVF_ETHTOOL_DUMP_REGS_LEN (SXEVF_ETHTOOL_DUMP_REGS_NUM * sizeof(u32))

#define SXEVF_STAT(m)		SXEVF_STATS, \
				sizeof(((struct sxevf_adapter *)0)->m), \
				offsetof(struct sxevf_adapter, m)

#define SXEVF_NETDEV_STAT(m)	NETDEV_STATS, \
				sizeof(((struct net_device_stats *)0)->m), \
				offsetof(struct net_device_stats, m)

static const char sxevf_gstrings_test[][ETH_GSTRING_LEN] = {
	"Register test  (offline)",
	"Link test   (on/offline)"
};

static const struct sxevf_ethtool_stats sxevf_gstrings_stats[] = {
	{"rx_packets", SXEVF_NETDEV_STAT(rx_packets)},
	{"tx_packets", SXEVF_NETDEV_STAT(tx_packets)},
	{"rx_bytes", SXEVF_NETDEV_STAT(rx_bytes)},
	{"tx_bytes", SXEVF_NETDEV_STAT(tx_bytes)},
	{"multicast", SXEVF_NETDEV_STAT(multicast)},
	{"tx_busy", SXEVF_STAT(stats.sw.tx_busy)},
	{"tx_restart_queue", SXEVF_STAT(stats.sw.restart_queue)},
	{"tx_timeout_count", SXEVF_STAT(stats.sw.tx_timeout_count)},
	{"rx_csum_offload_errors", SXEVF_STAT(stats.sw.hw_csum_rx_error)},
	{"alloc_rx_page", SXEVF_STAT(stats.sw.alloc_rx_page)},
	{"alloc_rx_page_failed", SXEVF_STAT(stats.sw.alloc_rx_page_failed)},
	{"alloc_rx_buff_failed", SXEVF_STAT(stats.sw.alloc_rx_buff_failed)},
};

static const char sxevf_priv_flags_strings[][ETH_GSTRING_LEN] = {
#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
	"legacy-rx",
#endif
};

u32 sxevf_self_test_suite_num_get(void)
{
	return sizeof(sxevf_gstrings_test) / ETH_GSTRING_LEN;
}

u32 sxevf_stats_num_get(void)
{
	return ARRAY_SIZE(sxevf_gstrings_stats);
}

u32 sxevf_priv_flags_num_get(void)
{
	return ARRAY_SIZE(sxevf_priv_flags_strings);
}

static void sxevf_get_drvinfo(struct net_device *netdev,
				struct ethtool_drvinfo *drvinfo)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	strlcpy(drvinfo->driver, SXEVF_DRV_NAME, sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, SXE_VERSION,
		sizeof(drvinfo->version));

	strlcpy(drvinfo->bus_info, pci_name(adapter->pdev),
		sizeof(drvinfo->bus_info));

	drvinfo->n_priv_flags = SXEVF_PRIV_FLAGS_STR_LEN;

	return;
}

#ifdef HAVE_ETHTOOL_EXTENDED_RINGPARAMS
static void
sxevf_get_ringparam(struct net_device *netdev,
		    struct ethtool_ringparam *ring,
		    struct kernel_ethtool_ringparam __always_unused *kernel_ring,
		    struct netlink_ext_ack __always_unused *extack)
#else
static void sxevf_get_ringparam(struct net_device *netdev,
				struct ethtool_ringparam *ring)
#endif
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	ring->rx_max_pending = SXEVF_DESC_CNT_MAX;
	ring->tx_max_pending = SXEVF_DESC_CNT_MAX;
	ring->rx_pending = adapter->rx_ring_ctxt.ring[0]->depth;
	ring->tx_pending = adapter->tx_ring_ctxt.ring[0]->depth;

	return;
}

inline static bool sxevf_ringparam_changed(struct sxevf_adapter *adapter,
				struct ethtool_ringparam *ring,
				u32 *tx_cnt, u32 *rx_cnt)
{
	bool changed = true;

	*tx_cnt = clamp_t(u32, ring->tx_pending,
			       SXEVF_DESC_CNT_MIN, SXEVF_DESC_CNT_MAX);
	*tx_cnt = ALIGN(*tx_cnt, SXEVF_REQ_DESCRIPTOR_MULTIPLE);

	*rx_cnt = clamp_t(u32, ring->rx_pending,
			       SXEVF_DESC_CNT_MIN, SXEVF_DESC_CNT_MAX);
	*rx_cnt = ALIGN(*rx_cnt, SXEVF_REQ_DESCRIPTOR_MULTIPLE);

	if ((*tx_cnt == adapter->tx_ring_ctxt.depth) &&
	    (*rx_cnt == adapter->rx_ring_ctxt.depth)) {
		changed = false;
	}

	return changed;
}

inline static void sxevf_ring_depth_set(struct sxevf_adapter *adapter,
				u32 tx_cnt, u32 rx_cnt)
{
	u32 i;
	struct sxevf_ring **tx_ring   = adapter->tx_ring_ctxt.ring;
	struct sxevf_ring **rx_ring   = adapter->rx_ring_ctxt.ring;
	struct sxevf_ring **xdp_ring  = adapter->xdp_ring_ctxt.ring;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		tx_ring[i]->depth = tx_cnt;
	}
	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		xdp_ring[i]->depth = tx_cnt;
	}
	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		rx_ring[i]->depth = rx_cnt;
	}
	adapter->tx_ring_ctxt.depth  = tx_cnt;
	adapter->xdp_ring_ctxt.depth = tx_cnt;
	adapter->rx_ring_ctxt.depth  = rx_cnt;

	return;
}

#ifdef HAVE_ETHTOOL_EXTENDED_RINGPARAMS
static int
sxevf_set_ringparam(struct net_device *netdev,
		struct ethtool_ringparam *user_param,
		struct kernel_ethtool_ringparam __always_unused *kernel_ring,
		struct netlink_ext_ack __always_unused *extack)
#else
static int sxevf_set_ringparam(struct net_device *netdev,
		struct ethtool_ringparam *user_param)
#endif
{
	int ret = 0;
	u32 new_rx_count, new_tx_count;
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	if ((user_param->rx_mini_pending) || (user_param->rx_jumbo_pending)) {
		LOG_ERROR_BDF("dont support set rx_mini_pending=%u or rx_jumbo_pending=%u\n",
			user_param->rx_mini_pending, user_param->rx_jumbo_pending);
		ret = -EINVAL;
		goto l_end;
	}

	if (!sxevf_ringparam_changed(adapter, user_param,
			&new_tx_count, &new_rx_count)) {
		LOG_DEBUG_BDF("ring depth dont change, tx_depth=%u, rx_depth=%u\n",
			new_tx_count, new_rx_count);
		goto l_end;
	}

	while (test_and_set_bit(SXEVF_RESETTING, &adapter->state)) {
		usleep_range(1000, 2000);
	}

	if (!netif_running(adapter->netdev)) {
		sxevf_ring_depth_set(adapter, new_tx_count, new_rx_count);
		goto l_clear;
	}

	sxevf_down(adapter);

	if (new_tx_count != adapter->tx_ring_ctxt.depth) {
		ret = sxevf_tx_ring_depth_reset(adapter, new_tx_count);
		if (ret < 0) {
			goto l_up;
		}
	}

	if (new_rx_count != adapter->rx_ring_ctxt.depth) {
		ret = sxevf_rx_ring_depth_reset(adapter, new_rx_count);
	}
l_up:
	sxevf_up(adapter);
l_clear:
	clear_bit(SXEVF_RESETTING, &adapter->state);
l_end:
	return ret;
}

static void sxevf_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
	u32 i;
	char *p = (char *)data;
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	switch (stringset) {
	case ETH_SS_TEST:
		memcpy(data, *sxevf_gstrings_test,
			SXEVF_TEST_GSTRING_ARRAY_SIZE * ETH_GSTRING_LEN);
		break;
	case ETH_SS_STATS:
		for (i = 0; i < SXEVF_STATS_ARRAY_SIZE; i++) {
			memcpy(p, sxevf_gstrings_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
			sprintf(p, "tx_ring_%u_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "tx_ring_%u_bytes", i);
			p += ETH_GSTRING_LEN;
		}
		for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
			sprintf(p, "xdp_ring_%u_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "xdp_ring_%u_bytes", i);
			p += ETH_GSTRING_LEN;
		}
		for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
			sprintf(p, "rx_ring_%u_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "rx_ring_%u_bytes", i);
			p += ETH_GSTRING_LEN;
		}
		break;
	case ETH_SS_PRIV_FLAGS:
		memcpy(data, sxevf_priv_flags_strings,
		       SXEVF_PRIV_FLAGS_STR_LEN * ETH_GSTRING_LEN);
		break;
	default:
		break;
	}

	return ;
}

static int sxevf_get_sset_count(struct net_device *netdev, int sset)
{
	int ret;

	switch (sset) {
	case ETH_SS_TEST:
		ret = SXEVF_TEST_GSTRING_ARRAY_SIZE;
		break;
	case ETH_SS_STATS:
		ret = SXEVF_STATS_LEN;
		break;
	case ETH_SS_PRIV_FLAGS:
		ret = SXEVF_PRIV_FLAGS_STR_LEN;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static void sxevf_get_ethtool_stats(struct net_device *netdev,
				    struct ethtool_stats *stats, u64 *data)
{
	s8 *p;
	u32 i, j, start;
	struct sxevf_ring *ring;
	struct rtnl_link_stats64 temp;
	const struct rtnl_link_stats64 *net_stats;
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	sxevf_update_stats(adapter);
	net_stats = dev_get_stats(netdev, &temp);
	for (i = 0; i < SXEVF_STATS_ARRAY_SIZE; i++) {
		switch (sxevf_gstrings_stats[i].type) {
		case NETDEV_STATS:
			p = (char *) net_stats +
					sxevf_gstrings_stats[i].stat_offset;
			break;
		case SXEVF_STATS:
			p = (char *) adapter +
					sxevf_gstrings_stats[i].stat_offset;
			break;
		default:
			data[i] = 0;
			continue;
		}

		data[i] = (sxevf_gstrings_stats[i].sizeof_stat ==
			   sizeof(u64)) ? *(u64 *)p : *(u32 *)p;
	}

	for (j = 0; j < adapter->tx_ring_ctxt.num; j++) {
		ring = adapter->tx_ring_ctxt.ring[j];
		if (!ring) {
			data[i++] = 0;
			data[i++] = 0;
			continue;
		}

		do {
			start = u64_stats_fetch_begin_irq(&ring->syncp);
			data[i]   = ring->stats.packets;
			data[i+1] = ring->stats.bytes;
		} while (u64_stats_fetch_retry_irq(&ring->syncp, start));
		i += 2;
	}

	for (j = 0; j < adapter->xdp_ring_ctxt.num; j++) {
		ring = adapter->xdp_ring_ctxt.ring[j];
		if (!ring) {
			data[i++] = 0;
			data[i++] = 0;
			continue;
		}

		do {
			start = u64_stats_fetch_begin_irq(&ring->syncp);
			data[i] = ring->stats.packets;
			data[i + 1] = ring->stats.bytes;
		} while (u64_stats_fetch_retry_irq(&ring->syncp, start));
		i += 2;
	}

	for (j = 0; j < adapter->rx_ring_ctxt.num; j++) {
		ring = adapter->rx_ring_ctxt.ring[j];
		if (!ring) {
			data[i++] = 0;
			data[i++] = 0;
			continue;
		}

		do {
			start = u64_stats_fetch_begin_irq(&ring->syncp);
			data[i]   = ring->stats.packets;
			data[i+1] = ring->stats.bytes;
		} while (u64_stats_fetch_retry_irq(&ring->syncp, start));
		i += 2;
	}

	return;
}

static int sxevf_get_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *cmd,
			   u32 *rule_locs)
{
	int ret = -EOPNOTSUPP;
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
		cmd->data = adapter->rx_ring_ctxt.num;
		ret = 0;
		break;
	default:
		LOG_DEBUG_BDF("command parameters not supported\n, cmd=%u", cmd->cmd);
		break;
	}

	return ret;
}

static u32 sxevf_get_priv_flags(struct net_device *netdev)
{
	u32 priv_flags = 0;
#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	if (adapter->cap & SXEVF_RX_LEGACY_ENABLE)
		priv_flags |= SXEVF_PRIV_FLAGS_LEGACY_RX;
#endif

	return priv_flags;
}

static int sxevf_set_priv_flags(struct net_device *netdev, u32 priv_flags)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	u32 cap = adapter->cap;

#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
	cap &= ~SXEVF_RX_LEGACY_ENABLE;
	if (priv_flags & SXEVF_PRIV_FLAGS_LEGACY_RX) {
		cap |= SXEVF_RX_LEGACY_ENABLE;
	}
#endif

	if (cap != adapter->cap) {
		adapter->cap = cap;

		if (netif_running(netdev)) {
			sxevf_hw_reinit(adapter);
		}
	}

	LOG_DEBUG_BDF("priv_flags=%u\n", priv_flags);

	return 0;
}

static int sxevf_nway_reset(struct net_device *netdev)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev)) {
		sxevf_hw_reinit(adapter);
		LOG_DEBUG_BDF("ethtool reset\n");
	}

	return 0;
}

static int sxevf_get_link_ksettings(struct net_device *netdev,
					  struct ethtool_link_ksettings *cmd)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	ethtool_link_ksettings_zero_link_mode(cmd, supported);
	cmd->base.autoneg = AUTONEG_DISABLE;
	cmd->base.port = -1;

	if (adapter->link.is_up) {
		switch (adapter->link.speed) {
		case SXEVF_LINK_SPEED_10GB_FULL:
			ethtool_link_ksettings_add_link_mode(cmd, supported,
							10000baseKR_Full);
			cmd->base.speed  = SPEED_10000;
			break;
		case SXEVF_LINK_SPEED_1GB_FULL:
			cmd->base.speed  = SPEED_1000;
			ethtool_link_ksettings_add_link_mode(cmd, supported,
							1000baseKX_Full);
			break;
		default:
			cmd->base.speed  = SPEED_10000;
			ethtool_link_ksettings_add_link_mode(cmd, supported,
							10000baseKR_Full);
			break;
		}
		cmd->base.duplex = DUPLEX_FULL;
	} else {
		cmd->base.speed  = SPEED_UNKNOWN;
		cmd->base.duplex = DUPLEX_UNKNOWN;
	}

	return 0;
}

static u32 sxevf_get_rss_redir_tbl_size(struct net_device *netdev)
{
	return SXEVF_MAX_RETA_ENTRIES;
}

static u32 sxevf_get_rss_hash_key_size(struct net_device *netdev)
{
	return SXEVF_RSS_HASH_KEY_SIZE;
}

STATIC int sxevf_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key,
			    u8 *hfunc)
{
	int err = 0;
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	if (hfunc) {
		*hfunc = ETH_RSS_HASH_TOP;
	}

	if (!indir && !key) {
		LOG_DEBUG_BDF("param err, indir=%p, key=%p\n", indir, key);
		return 0;
	}

	spin_lock_bh(&adapter->mbx_lock);
	if (indir) {
		err = sxevf_redir_tbl_get(&adapter->hw, adapter->rx_ring_ctxt.num, indir);
	}

	if (!err && key) {
		err = sxevf_rss_hash_key_get(&adapter->hw, key);
	}

	spin_unlock_bh(&adapter->mbx_lock);

	return err;
}

static int sxevf_get_regs_len(struct net_device *netdev)
{
	return SXEVF_ETHTOOL_DUMP_REGS_LEN;
}

static void sxevf_get_regs(struct net_device *netdev,
		struct ethtool_regs *regs, void *data)
{
	u8 dump_regs_num;
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	struct sxevf_hw *hw = &adapter->hw;

	memset(data, 0, SXEVF_ETHTOOL_DUMP_REGS_LEN);

	regs->version = 0;

	dump_regs_num = hw->setup.ops->regs_dump(hw, data, SXEVF_ETHTOOL_DUMP_REGS_LEN);

	if (dump_regs_num != SXEVF_ETHTOOL_DUMP_REGS_NUM) {
		LOG_WARN_BDF("dump_regs_num=%u, regs_num_max=%u\n", dump_regs_num, SXEVF_ETHTOOL_DUMP_REGS_NUM);
	}

	return;
}

static u32 sxevf_get_msglevel(struct net_device *netdev)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	return adapter->msg_enable;
}

static void sxevf_set_msglevel(struct net_device *netdev, u32 data)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	adapter->msg_enable = data;

	return;
}

static s32 sxevf_link_test(struct sxevf_adapter *adapter)
{
	s32 ret;
	u32 link_up;
	struct sxevf_hw *hw = &adapter->hw;

	link_up = hw->setup.ops->link_state_get(hw);
	if (!(link_up & SXE_VFLINKS_UP)) {
		ret = -SXEVF_DIAG_TEST_BLOCKED;
	} else {
		ret = SXEVF_DIAG_TEST_PASSED;
	}

	return ret;
}

STATIC int sxevf_reg_test(struct sxevf_adapter *adapter)
{
	s32 ret;
	struct sxevf_hw *hw = &adapter->hw;

	if (sxevf_is_hw_fault(hw)) {
		LOG_DEV_ERR("nic hw fault - register test blocked\n");
		ret = -SXEVF_DIAG_TEST_BLOCKED;
		goto l_end;
	}

	ret = hw->setup.ops->regs_test(hw);
	if (ret) {
		LOG_ERROR_BDF("register test  failed\n");
		goto l_end;
	}

l_end:
	return ret;
}

static void sxevf_diag_test(struct net_device *netdev,
			      struct ethtool_test *eth_test, u64 *result)
{
	s32 ret;
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	bool if_running = netif_running(netdev);
	struct sxevf_hw *hw = &adapter->hw;

	if (sxevf_is_hw_fault(hw)) {
		result[SXEVF_DIAG_REGS_TEST] = SXEVF_DIAG_TEST_BLOCKED;
		result[SXEVF_DIAG_LINK_TEST] = SXEVF_DIAG_TEST_BLOCKED;
		eth_test->flags |= ETH_TEST_FL_FAILED;
		LOG_DEV_ERR("nic hw fault - test blocked\n");
		return;
	}

	set_bit(SXEVF_TESTING, &adapter->state);
	if (eth_test->flags == ETH_TEST_FL_OFFLINE) {

		LOG_DEV_DEBUG("offline testing starting\n");

		ret = sxevf_link_test(adapter);
		if (ret) {
			eth_test->flags |= ETH_TEST_FL_FAILED;
		}
		result[SXEVF_DIAG_LINK_TEST] = -ret;

		if (if_running) {
			sxevf_close(netdev);
		} else {
			sxevf_reset(adapter);
		}

		LOG_DEV_DEBUG("register testing starting\n");

		ret = sxevf_reg_test(adapter);
		if (ret) {
			eth_test->flags |= ETH_TEST_FL_FAILED;
		}
		result[SXEVF_DIAG_REGS_TEST] = -ret;

		sxevf_reset(adapter);

		clear_bit(SXEVF_TESTING, &adapter->state);
		if (if_running) {
			sxevf_open(netdev);
		}
	} else {
		LOG_DEV_DEBUG("online testing starting\n");

		ret = sxevf_link_test(adapter);
		if (ret) {
			eth_test->flags |= ETH_TEST_FL_FAILED;
		}
		result[SXEVF_DIAG_LINK_TEST] = -ret;

		result[SXEVF_DIAG_REGS_TEST] = SXEVF_DIAG_TEST_PASSED;

		clear_bit(SXEVF_TESTING, &adapter->state);
	}

	msleep_interruptible(SXEVF_TEST_SLEEP_TIME * SXEVF_HZ_TRANSTO_MS);

	return;
}

static int sxevf_get_coalesce(struct net_device *netdev,
#ifdef HAVE_ETHTOOL_COALESCE_EXTACK
			      struct ethtool_coalesce *ec,
			      struct kernel_ethtool_coalesce *kernel_coal,
			      struct netlink_ext_ack *extack)
#else
			      struct ethtool_coalesce *ec)
#endif
{
	return sxevf_irq_coalesce_get(netdev, ec);
}

static int sxevf_set_coalesce(struct net_device *netdev,
#ifdef HAVE_ETHTOOL_COALESCE_EXTACK
			      struct ethtool_coalesce *ec,
			      struct kernel_ethtool_coalesce *kernel_coal,
			      struct netlink_ext_ack *extack)
#else
			      struct ethtool_coalesce *ec)
#endif
{
	return sxevf_irq_coalesce_set(netdev, ec);
}

static const struct ethtool_ops sxevf_ethtool_ops = {
#ifdef ETHTOOL_COALESCE_USECS
	.supported_coalesce_params = ETHTOOL_COALESCE_USECS,
#endif
	.get_drvinfo		= sxevf_get_drvinfo,
	.nway_reset		= sxevf_nway_reset,
	.get_link		= ethtool_op_get_link,
	.get_ringparam		= sxevf_get_ringparam,
	.set_ringparam		= sxevf_set_ringparam,
	.get_strings		= sxevf_get_strings,
	.get_sset_count		= sxevf_get_sset_count,
	.get_ethtool_stats	= sxevf_get_ethtool_stats,
	.get_rxnfc		= sxevf_get_rxnfc,
	.get_rxfh_indir_size	= sxevf_get_rss_redir_tbl_size,
	.get_rxfh_key_size	= sxevf_get_rss_hash_key_size,
	.get_rxfh		= sxevf_get_rxfh,
	.get_link_ksettings	= sxevf_get_link_ksettings,
	.get_priv_flags		= sxevf_get_priv_flags,
	.set_priv_flags		= sxevf_set_priv_flags,
	.get_regs_len		= sxevf_get_regs_len,
	.get_regs		= sxevf_get_regs,
	.get_msglevel		= sxevf_get_msglevel,
	.set_msglevel		= sxevf_set_msglevel,
	.self_test		= sxevf_diag_test,

	.set_coalesce           = sxevf_set_coalesce,
	.get_coalesce           = sxevf_get_coalesce,
};

void sxevf_ethtool_ops_set(struct net_device *netdev)
{
	netdev->ethtool_ops = &sxevf_ethtool_ops;

	return;
}
