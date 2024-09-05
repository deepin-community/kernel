#include <linux/highmem.h>

#include "sxe_ethtool.h"
#include "sxe_hw.h"
#include "sxe_log.h"
#include "sxe_version.h"
#include "sxe_tx_proc.h"
#include "sxe_netdev.h"
#include "sxe_rx_proc.h"
#include "sxe_filter.h"
#include "sxe_irq.h"
#include "sxe_host_hdc.h"
#include "sxe_phy.h"
#include "sxe_cli.h"

enum sxe_diag_test_case{
	SXE_DIAG_REGS_TEST     = 0,
	SXE_DIAG_EEPROM_TEST   = 1,
	SXE_DIAG_IRQ_TEST      = 2,
	SXE_DIAG_LOOPBACK_TEST = 3,
	SXE_DIAG_LINK_TEST     = 4,
	SXE_DIAG_TEST_MAX,
};

#define SXE_STAT(m)		SXE_STATS, \
				sizeof(((struct sxe_adapter *)0)->m), \
				offsetof(struct sxe_adapter, m)
#define SXE_NETDEV_STAT(m)	NETDEV_STATS, \
				sizeof(((struct rtnl_link_stats64 *)0)->m), \
				offsetof(struct rtnl_link_stats64, m)
#define SXE_MAC_STATS_OFFSET(m)	offsetof(struct sxe_mac_stats, m)

#define SXE_TEST_SLEEP_TIME		(2)

static const char sxe_gstrings_test[][ETH_GSTRING_LEN] = {
	"Register test  (offline)", "Eeprom test    (offline)",
	"Interrupt test (offline)", "Loopback test  (offline)",
	"Link test   (on/offline)"
};

static const struct sxe_ethtool_stats sxe_gstrings_stats[] = {
	{"rx_packets", SXE_NETDEV_STAT(rx_packets)}, 
	{"tx_packets", SXE_NETDEV_STAT(tx_packets)}, 
	{"rx_bytes", SXE_NETDEV_STAT(rx_bytes)},     
	{"tx_bytes", SXE_NETDEV_STAT(tx_bytes)},     
	{"rx_errors", SXE_NETDEV_STAT(rx_errors)},   
	{"tx_errors", SXE_NETDEV_STAT(tx_errors)},   
	{"rx_dropped", SXE_NETDEV_STAT(rx_dropped)}, 
	{"tx_dropped", SXE_NETDEV_STAT(tx_dropped)}, 
	{"multicast", SXE_NETDEV_STAT(multicast)},   
	{"collisions", SXE_NETDEV_STAT(collisions)}, 
	{"rx_over_errors", SXE_NETDEV_STAT(rx_over_errors)},   
	{"rx_crc_errors", SXE_NETDEV_STAT(rx_crc_errors)},     
	{"rx_frame_errors", SXE_NETDEV_STAT(rx_frame_errors)}, 
	{"rx_fifo_errors", SXE_NETDEV_STAT(rx_fifo_errors)},   
	{"rx_missed_errors", SXE_NETDEV_STAT(rx_missed_errors)},   
	{"tx_aborted_errors", SXE_NETDEV_STAT(tx_aborted_errors)}, 
	{"tx_carrier_errors", SXE_NETDEV_STAT(tx_carrier_errors)}, 
	{"tx_fifo_errors", SXE_NETDEV_STAT(tx_fifo_errors)},       
	{"tx_heartbeat_errors", SXE_NETDEV_STAT(tx_heartbeat_errors)}, 
	{"rx_pkts_nic", SXE_STAT(stats.hw.gprc)},
	{"tx_pkts_nic", SXE_STAT(stats.hw.gptc)},
	{"rx_bytes_nic", SXE_STAT(stats.hw.gorc)},
	{"tx_bytes_nic", SXE_STAT(stats.hw.gotc)},
	{"link_state_change_cnt", SXE_STAT(stats.sw.link_state_change_cnt)},
	{"tx_busy", SXE_STAT(stats.sw.tx_busy)},
	{"non_eop_descs", SXE_STAT(stats.sw.non_eop_descs)},
	{"broadcast", SXE_STAT(stats.hw.bprc)},
	{"hw_lro_aggregated", SXE_STAT(stats.sw.lro_total_count)},
	{"hw_lro_flushed", SXE_STAT(stats.sw.lro_total_flush)},
	{"fnav_match", SXE_STAT(stats.hw.fnavmatch)},
	{"fnav_miss", SXE_STAT(stats.hw.fnavmiss)},
	{"fnav_overflow", SXE_STAT(stats.sw.fnav_overflow)},
	{"reset_work_trigger", SXE_STAT(stats.sw.reset_work_trigger_cnt)},
	{"tx_restart_queue", SXE_STAT(stats.sw.restart_queue)},
	{"rx_length_errors", SXE_STAT(stats.hw.rlec)},
	{"rx_long_length_errors", SXE_STAT(stats.hw.roc)},
	{"rx_short_length_errors", SXE_STAT(stats.hw.ruc)},
	{"rx_csum_offload_errors", SXE_STAT(stats.sw.hw_csum_rx_error)},
	{"alloc_rx_page", SXE_STAT(stats.sw.alloc_rx_page)},
	{"alloc_rx_page_failed", SXE_STAT(stats.sw.alloc_rx_page_failed)},
	{"alloc_rx_buff_failed", SXE_STAT(stats.sw.alloc_rx_buff_failed)},
	{"rx_no_dma_resources", SXE_STAT(stats.hw.hw_rx_no_dma_resources)},
	{"tx_hwtstamp_timeouts", SXE_STAT(stats.sw.tx_hwtstamp_timeouts)},
	{"tx_hwtstamp_skipped", SXE_STAT(stats.sw.tx_hwtstamp_skipped)},
	{"rx_hwtstamp_cleared", SXE_STAT(stats.sw.rx_hwtstamp_cleared)},
	{"tx_ipsec", SXE_STAT(stats.sw.tx_ipsec)},
	{"rx_ipsec", SXE_STAT(stats.sw.rx_ipsec)},
};

static const struct sxe_ethtool_stats sxe_gstrings_dma_stats[] = {
	{"dma_good_rx_pkts", SXE_STAT(stats.hw.rxdgpc)},   
	{"dma_good_rx_bytes", SXE_STAT(stats.hw.rxdgbc)},  
	{"dma_good_tx_pkts", SXE_STAT(stats.hw.txdgpc)},   
	{"dma_good_tx_bytes", SXE_STAT(stats.hw.txdgbc)},  
	{"dma_dup_good_rx_pkts", SXE_STAT(stats.hw.rxddpc)},
	{"dma_dup_good_rx_bytes", SXE_STAT(stats.hw.rxddbc)},
	{"dma_vm_to_host_rx_pkts", SXE_STAT(stats.hw.rxlpbkpc)}, 
	{"dma_vm_to_host_rx_bytes", SXE_STAT(stats.hw.rxlpbkbc)},
	{"dma_dup_vm_to_host_pkts", SXE_STAT(stats.hw.rxdlpbkpc)},
	{"dma_dup_vm_to_host_bytes", SXE_STAT(stats.hw.rxdlpbkbc)},
	{"dbu_to_dma_rx_pkts", SXE_STAT(stats.hw.rxtpcing)},
	{"dma_to_host_rx_pkts", SXE_STAT(stats.hw.rxtpceng)},
	{"dma_rx_drop", SXE_STAT(stats.hw.prddc)},
	{"pcie_err_tx_pkts", SXE_STAT(stats.hw.txswerr)},
	{"vm_to_vm_tx_pkts", SXE_STAT(stats.hw.txswitch)},
	{"vm_to_vm_tx_dropped", SXE_STAT(stats.hw.txrepeat)},
	{"dma_tx_desc_err", SXE_STAT(stats.hw.txdescerr)},
};

static const char sxe_priv_flags_strings[][ETH_GSTRING_LEN] = {
#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
	"legacy-rx",
#endif
#ifdef SXE_IPSEC_CONFIGURE
	"vf-ipsec",
#endif
};

static const struct sxe_mac_stats_info mac_stats[] = {
	{"crcerrs", SXE_MAC_STATS_OFFSET(crcerrs)},
	{"errbc", SXE_MAC_STATS_OFFSET(errbc)},
	{"rlec", SXE_MAC_STATS_OFFSET(rlec)},
	{"prc64", SXE_MAC_STATS_OFFSET(prc64)},
	{"prc127", SXE_MAC_STATS_OFFSET(prc127)},
	{"prc255", SXE_MAC_STATS_OFFSET(prc255)},
	{"prc511", SXE_MAC_STATS_OFFSET(prc511)},
	{"prc1023", SXE_MAC_STATS_OFFSET(prc1023)},
	{"prc1522", SXE_MAC_STATS_OFFSET(prc1522)},
	{"bprc", SXE_MAC_STATS_OFFSET(bprc)},
	{"mprc", SXE_MAC_STATS_OFFSET(mprc)},
	{"gprc", SXE_MAC_STATS_OFFSET(gprc)},
	{"gorc", SXE_MAC_STATS_OFFSET(gorc)},
	{"ruc", SXE_MAC_STATS_OFFSET(ruc)},
	{"rfc", SXE_MAC_STATS_OFFSET(rfc)},
	{"roc", SXE_MAC_STATS_OFFSET(roc)},
	{"rjc", SXE_MAC_STATS_OFFSET(rjc)},
	{"tor", SXE_MAC_STATS_OFFSET(tor)},
	{"tpr", SXE_MAC_STATS_OFFSET(tpr)},
	{"gptc", SXE_MAC_STATS_OFFSET(gptc)},
	{"gotc", SXE_MAC_STATS_OFFSET(gotc)},
	{"tpt", SXE_MAC_STATS_OFFSET(tpt)},
	{"ptc64", SXE_MAC_STATS_OFFSET(ptc64)},
	{"ptc127", SXE_MAC_STATS_OFFSET(ptc127)},
	{"ptc255", SXE_MAC_STATS_OFFSET(ptc255)},
	{"ptc511", SXE_MAC_STATS_OFFSET(ptc511)},
	{"ptc1023", SXE_MAC_STATS_OFFSET(ptc1023)},
	{"ptc1522", SXE_MAC_STATS_OFFSET(ptc1522)},
	{"mptc", SXE_MAC_STATS_OFFSET(mptc)},
	{"bptc", SXE_MAC_STATS_OFFSET(bptc)},
	{"prcpf[0]", SXE_MAC_STATS_OFFSET(prcpf[0])},
	{"prcpf[1]", SXE_MAC_STATS_OFFSET(prcpf[1])},
	{"prcpf[2]", SXE_MAC_STATS_OFFSET(prcpf[2])},
	{"prcpf[3]", SXE_MAC_STATS_OFFSET(prcpf[3])},
	{"prcpf[4]", SXE_MAC_STATS_OFFSET(prcpf[4])},
	{"prcpf[5]", SXE_MAC_STATS_OFFSET(prcpf[5])},
	{"prcpf[6]", SXE_MAC_STATS_OFFSET(prcpf[6])},
	{"prcpf[7]", SXE_MAC_STATS_OFFSET(prcpf[7])},
	{"pfct[0]", SXE_MAC_STATS_OFFSET(pfct[0])},
	{"pfct[1]", SXE_MAC_STATS_OFFSET(pfct[1])},
	{"pfct[2]", SXE_MAC_STATS_OFFSET(pfct[2])},
	{"pfct[3]", SXE_MAC_STATS_OFFSET(pfct[3])},
	{"pfct[4]", SXE_MAC_STATS_OFFSET(pfct[4])},
	{"pfct[5]", SXE_MAC_STATS_OFFSET(pfct[5])},
	{"pfct[6]", SXE_MAC_STATS_OFFSET(pfct[6])},
	{"pfct[7]", SXE_MAC_STATS_OFFSET(pfct[7])},
};

u32 sxe_mac_stats_regs_num_get(void)
{
	return ARRAY_SIZE(mac_stats);
}

u32 sxe_self_test_suite_num_get(void)
{
	return sizeof(sxe_gstrings_test) / ETH_GSTRING_LEN;
}

u32 sxe_stats_num_get(void)
{
	return ARRAY_SIZE(sxe_gstrings_stats);
}

u32 sxe_dma_stats_num_get(void)
{
	return ARRAY_SIZE(sxe_gstrings_dma_stats);
}

u32 sxe_priv_flags_num_get(void)
{
	return ARRAY_SIZE(sxe_priv_flags_strings);
}

static void sxe_get_drvinfo(struct net_device *netdev,
			      struct ethtool_drvinfo *drvinfo)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	strlcpy(drvinfo->driver, SXE_DRV_NAME, sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, SXE_VERSION,
		sizeof(drvinfo->version));

	sxe_fw_version_get(adapter);
	strlcpy(drvinfo->fw_version, (s8 *)adapter->fw_info.fw_version,
		sizeof(drvinfo->fw_version));

	strlcpy(drvinfo->bus_info, pci_name(adapter->pdev),
		sizeof(drvinfo->bus_info));

	drvinfo->n_priv_flags = SXE_PRIV_FLAGS_STR_LEN;

	return;
}

static int sxe_get_sset_count(struct net_device *netdev, int sset)
{
	int ret;

	switch (sset) {
	case ETH_SS_TEST:
		ret = SXE_TEST_GSTRING_ARRAY_SIZE;
		break;
	case ETH_SS_STATS:
		ret = SXE_STATS_LEN;
		break;
	case ETH_SS_PRIV_FLAGS:
		ret = SXE_PRIV_FLAGS_STR_LEN;
		break;
	default:
		ret = -EOPNOTSUPP;
	}

	LOG_DEBUG("type cmd=%d, string len=%d\n", sset, ret);

	return ret;
}

static void sxe_get_ethtool_stats(struct net_device *netdev,
				    struct ethtool_stats *stats, u64 *data)
{
	s8 *p;
	u32 i, j, start;
	struct sxe_ring *ring;
	struct rtnl_link_stats64 temp;
	const struct rtnl_link_stats64 *net_stats;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	stats_lock(adapter);
	sxe_stats_update(adapter);
	net_stats = dev_get_stats(netdev, &temp);
	for (i = 0; i < SXE_STATS_ARRAY_SIZE; i++) {
		switch (sxe_gstrings_stats[i].type) {
		case NETDEV_STATS:
			p = (s8 *) net_stats +
					sxe_gstrings_stats[i].stat_offset;
			break;
		case SXE_STATS:
			p = (s8 *) adapter +
					sxe_gstrings_stats[i].stat_offset;
			break;
		default:
			data[i] = 0;
			continue;
		}

		data[i] = (sxe_gstrings_stats[i].sizeof_stat ==
			   sizeof(u64)) ? *(u64 *)p : *(u32 *)p;
	}

	for (j = 0; j < netdev->num_tx_queues; j++) {
		ring = adapter->tx_ring_ctxt.ring[j];
		if (!ring) {
			data[i] = 0;
			data[i+1] = 0;
			i += 2;
			continue;
		}

		do {
			start = u64_stats_fetch_begin_irq(&ring->syncp);
			data[i]   = ring->stats.packets;
			data[i+1] = ring->stats.bytes;
		} while (u64_stats_fetch_retry_irq(&ring->syncp, start));
		i += 2;
	}

	for (j = 0; j < SXE_RX_RING_NUM; j++) {
		ring = adapter->rx_ring_ctxt.ring[j];
		if (!ring) {
			data[i] = 0;
			data[i+1] = 0;
			i += 2;
			continue;
		}

		do {
			start = u64_stats_fetch_begin_irq(&ring->syncp);
			data[i]   = ring->stats.packets;
			data[i+1] = ring->stats.bytes;
		} while (u64_stats_fetch_retry_irq(&ring->syncp, start));
		i += 2;
	}

	for (j = 0; j < SXE_PKG_BUF_NUM_MAX; j++) {
		data[i++] = adapter->stats.hw.dburxtcin[j];
		data[i++] = adapter->stats.hw.dburxtcout[j];
		data[i++] = adapter->stats.hw.dburxdrofpcnt[j];
		data[i++] = adapter->stats.hw.dburxgdreecnt[j];
	}

	for (j = 0; j < SXE_PKG_BUF_NUM_MAX; j++) {
		data[i++] = adapter->stats.hw.dbutxtcin[j];
		data[i++] = adapter->stats.hw.dbutxtcout[j];
	}

	for (j = 0; j < SXE_DCB_8_TC; j++) {
		data[i++] = adapter->stats.hw.qptc[j];
		data[i++] = adapter->stats.hw.qprc[j];
		data[i++] = adapter->stats.hw.qbtc[j];
		data[i++] = adapter->stats.hw.qbrc[j];
		data[i++] = adapter->stats.hw.qprdc[j];
	}

	for (j = 0; j < sxe_dma_stats_num_get(); j++) {
		p = (s8 *) adapter + sxe_gstrings_dma_stats[j].stat_offset;
		data[i++] = *(u64 *)p;
	}

	for (j = 0; j < SXE_PKG_BUF_NUM_MAX; j++) {
		data[i++] = adapter->stats.hw.prcpf[j];
		data[i++] = adapter->stats.hw.pfct[j];
	}
	stats_unlock(adapter);

	return;
}

static void sxe_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
	u32 i;
	char *string = (char *)data;

	switch (stringset) {
	case ETH_SS_TEST:
		for (i = 0; i < SXE_TEST_GSTRING_ARRAY_SIZE; i++) {
			memcpy(string, sxe_gstrings_test[i], ETH_GSTRING_LEN);
			string += ETH_GSTRING_LEN;
		}
		break;
	case ETH_SS_STATS:
		for (i = 0; i < SXE_STATS_ARRAY_SIZE; i++) {
			memcpy(string, sxe_gstrings_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			string += ETH_GSTRING_LEN;
		}

		for (i = 0; i < netdev->num_tx_queues; i++) {
			sprintf(string, "tx_ring_%u_packets", i);
			string += ETH_GSTRING_LEN;
			sprintf(string, "tx_ring_%u_bytes", i);
			string += ETH_GSTRING_LEN;
		}
		for (i = 0; i < SXE_RX_RING_NUM; i++) {
			sprintf(string, "rx_ring_%u_packets", i);
			string += ETH_GSTRING_LEN;
			sprintf(string, "rx_ring_%u_bytes", i);
			string += ETH_GSTRING_LEN;
		}

		for (i = 0; i < SXE_PKG_BUF_NUM_MAX; i++) {
			sprintf(string, "rx_pkt_buf_%u_in_packets", i);
			string += ETH_GSTRING_LEN;
			sprintf(string, "rx_pkt_buf_%u_out_packets", i);
			string += ETH_GSTRING_LEN;
			sprintf(string, "rx_pkt_buf_%u_overflow_packets", i);
			string += ETH_GSTRING_LEN;
			sprintf(string, "rx_pkt_buf_%u_ecc_errors", i);
			string += ETH_GSTRING_LEN;
		}

		for (i = 0; i < SXE_PKG_BUF_NUM_MAX; i++) {
			sprintf(string, "tx_pkt_buf_%u_in_packets", i);
			string += ETH_GSTRING_LEN;
			sprintf(string, "tx_pkt_buf_%u_out_packets", i);
			string += ETH_GSTRING_LEN;
		}

		for (i = 0; i < SXE_DCB_8_TC; i++) {
			sprintf(string, "tc_%u_tx_ring_packets", i);
			string += ETH_GSTRING_LEN;
			sprintf(string, "tc_%u_rx_ring_packets", i);
			string += ETH_GSTRING_LEN;
			sprintf(string, "tc_%u_tx_ring_bytes", i);
			string += ETH_GSTRING_LEN;
			sprintf(string, "tc_%u_rx_ring_bytes", i);
			string += ETH_GSTRING_LEN;
			sprintf(string, "tc_%u_rx_ring_dropped", i);
			string += ETH_GSTRING_LEN;
		}

		for (i = 0; i < sxe_dma_stats_num_get(); i++) {
			memcpy(string, sxe_gstrings_dma_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			string += ETH_GSTRING_LEN;
		}

		for (i = 0; i < SXE_PKG_BUF_NUM_MAX; i++) {
			sprintf(string, "up_%u_pause_recv", i);
			string += ETH_GSTRING_LEN;
			sprintf(string, "up_%u_pause_send", i);
			string += ETH_GSTRING_LEN;
		}

		break;
	case ETH_SS_PRIV_FLAGS:
		memcpy(string, sxe_priv_flags_strings,
		       SXE_PRIV_FLAGS_STR_LEN * ETH_GSTRING_LEN);
		break;
	default:
		break;
	}

	return ;
}

static u32 sxe_get_priv_flags(struct net_device *netdev)
{
	u32 priv_flags = 0;
#if !defined HAVE_NO_SWIOTLB_SKIP_CPU_SYNC || \
	defined SXE_IPSEC_CONFIGURE
	struct sxe_adapter *adapter = netdev_priv(netdev);
#endif

#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
	if (adapter->cap & SXE_RX_LEGACY) {
		priv_flags |= SXE_PRIV_FLAGS_LEGACY_RX;
	}
#endif

#ifdef SXE_IPSEC_CONFIGURE
	if (adapter->cap & SXE_VF_IPSEC_ENABLED) {
		priv_flags |= SXE_PRIV_FLAGS_VF_IPSEC_EN;
	}
#endif
	return priv_flags;
}

static int sxe_set_priv_flags(struct net_device *netdev, u32 priv_flags)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	u32 cap = adapter->cap;

#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
	cap &= ~SXE_RX_LEGACY;
	if (priv_flags & SXE_PRIV_FLAGS_LEGACY_RX) {
		cap |= SXE_RX_LEGACY;
	}
#endif

#ifdef SXE_IPSEC_CONFIGURE
	cap &= ~SXE_VF_IPSEC_ENABLED;
	if (priv_flags & SXE_PRIV_FLAGS_VF_IPSEC_EN) {
		cap |= SXE_VF_IPSEC_ENABLED;
	}
#endif

	if (cap != adapter->cap) {
		adapter->cap = cap;

		if (netif_running(netdev)) {
			LOG_DEBUG_BDF("set priv flags reinit\n");
			sxe_hw_reinit(adapter);
		}
	}

	LOG_DEBUG_BDF("priv_flags=%u\n", priv_flags);

	return 0;
}

s32 sxe_fnav_dest_queue_parse(
					struct sxe_adapter *adapter,
					u64 ring_cookie,
					u8 *queue)
{
	s32 ret = 0;

	LOG_DEBUG_BDF("source ring_cookie = 0x%llx\n", ring_cookie);
	if (ring_cookie == RX_CLS_FLOW_DISC) {
		*queue = SXE_FNAV_DROP_QUEUE;
	} else {
		u32 ring = ethtool_get_flow_spec_ring(ring_cookie);
		u8 vf = ethtool_get_flow_spec_ring_vf(ring_cookie);
		LOG_DEBUG_BDF("input ring = %u, vf = %u \n", ring, vf);

		if (!vf && (ring >= adapter->rx_ring_ctxt.num)) {
			LOG_ERROR_BDF("input ring[%u] exceed max rx ring num[%u]\n",
					ring, adapter->rx_ring_ctxt.num);
			ret = -EINVAL;
			goto l_ret;
		} else if (vf &&
			 ((vf > adapter->vt_ctxt.num_vfs) ||
			   ring >= adapter->ring_f.ring_per_pool)) {
			LOG_ERROR_BDF("input vf[%u] exceed max vf num[%u] or "
				"ring[%u] exceed max rx ring num[%u] in pool\n",
				vf, adapter->vt_ctxt.num_vfs,
				ring, adapter->ring_f.ring_per_pool);
			ret = -EINVAL;
			goto l_ret;
		}

		if (!vf) {
			*queue = adapter->rx_ring_ctxt.ring[ring]->reg_idx;
		} else {
			*queue = ((vf - 1) *
				adapter->ring_f.ring_per_pool ) + ring;
		}
	}

	LOG_DEBUG_BDF("parse fnav dest queue end, ring_cookie = 0x%llx, queue = %u\n",
			ring_cookie, *queue);
l_ret:
	return ret;
}

static s32 sxe_fnav_rule_parse_to_flow(
					struct sxe_adapter *adapter,
					struct sxe_fnav_rule_node *rule,
					struct ethtool_rx_flow_spec *flow)
{
	s32 ret = 0;
	union sxe_fnav_rule_info *mask = &adapter->fnav_ctxt.rules_mask;

	switch (rule->rule_info.ntuple.flow_type) {
	case SXE_SAMPLE_FLOW_TYPE_TCPV4:
		flow->flow_type = TCP_V4_FLOW;
		break;
	case SXE_SAMPLE_FLOW_TYPE_UDPV4:
		flow->flow_type = UDP_V4_FLOW;
		break;
	case SXE_SAMPLE_FLOW_TYPE_SCTPV4:
		flow->flow_type = SCTP_V4_FLOW;
		break;
	case SXE_SAMPLE_FLOW_TYPE_IPV4:
		flow->flow_type = IP_USER_FLOW;
		flow->h_u.usr_ip4_spec.ip_ver = ETH_RX_NFC_IP4;
		flow->h_u.usr_ip4_spec.proto = 0;
		flow->m_u.usr_ip4_spec.proto = 0;
		break;
	default:
		LOG_ERROR_BDF("unknow flow type[%u]\n",
				rule->rule_info.ntuple.flow_type);
		ret = -EINVAL;
		goto l_ret;
	}
	flow->flow_type |= FLOW_EXT;
	LOG_DEBUG_BDF("flow_type=0x%x\n", flow->flow_type);

	flow->h_u.tcp_ip4_spec.psrc = rule->rule_info.ntuple.src_port;
	flow->h_u.tcp_ip4_spec.pdst = rule->rule_info.ntuple.dst_port;
	flow->h_u.tcp_ip4_spec.ip4src = rule->rule_info.ntuple.src_ip[0];
	flow->h_u.tcp_ip4_spec.ip4dst = rule->rule_info.ntuple.dst_ip[0];
	flow->h_ext.vlan_tci = rule->rule_info.ntuple.vlan_id;
	flow->h_ext.vlan_etype = rule->rule_info.ntuple.flex_bytes;
	flow->h_ext.data[1] = htonl(rule->rule_info.ntuple.vm_pool);
	LOG_DEBUG_BDF("parse rule to user src_port[%u], dst_port[%u], src_ip[0x%x], "
		"dst_ip[0x%x] vlan_id[%u], flex_bytes[0x%x], vm_pool[%u]\n",
		flow->h_u.tcp_ip4_spec.psrc,
		flow->h_u.tcp_ip4_spec.pdst,
		flow->h_u.tcp_ip4_spec.ip4src,
		flow->h_u.tcp_ip4_spec.ip4dst,
		flow->h_ext.vlan_tci,
		flow->h_ext.vlan_etype,
		flow->h_ext.data[1]);

	flow->m_u.tcp_ip4_spec.psrc = mask->ntuple.src_port;
	flow->m_u.tcp_ip4_spec.pdst = mask->ntuple.dst_port;
	flow->m_u.tcp_ip4_spec.ip4src = mask->ntuple.src_ip[0];
	flow->m_u.tcp_ip4_spec.ip4dst = mask->ntuple.dst_ip[0];
	flow->m_ext.vlan_tci = mask->ntuple.vlan_id;
	flow->m_ext.vlan_etype = mask->ntuple.flex_bytes;
	flow->m_ext.data[1] = htonl(mask->ntuple.vm_pool);
	LOG_DEBUG_BDF("parse rule mask to user src_port[%u], dst_port[%u], src_ip[0x%x],"
		" dst_ip[0x%x] vlan_id[%u], flex_bytes[0x%x], vm_pool[%u]\n",
		flow->m_u.tcp_ip4_spec.psrc,
		flow->m_u.tcp_ip4_spec.pdst,
		flow->m_u.tcp_ip4_spec.ip4src,
		flow->m_u.tcp_ip4_spec.ip4dst,
		flow->m_ext.vlan_tci,
		flow->m_ext.vlan_etype,
		flow->m_ext.data[1]);

	flow->ring_cookie = rule->ring_cookie;
	LOG_DEBUG_BDF("parse ring_cookie[%llu]\n", flow->ring_cookie);

l_ret:
	return ret;
}

static int sxe_fnav_rule_get(struct sxe_adapter *adapter,
					struct ethtool_rxnfc *cmd)
{
	int ret;
	struct ethtool_rx_flow_spec *flow =
		(struct ethtool_rx_flow_spec *)&cmd->fs;
	struct sxe_fnav_rule_node *rule = NULL;

	cmd->data = sxe_fnav_max_rule_num_get(
				adapter->fnav_ctxt.rules_table_size);
	LOG_DEBUG_BDF("fnav table size = %llu\n", cmd->data);

	LOG_DEBUG_BDF("find rule in loc[%u]\n", flow->location);
	rule = sxe_fnav_specific_rule_find(adapter, flow->location);
	if (rule == NULL) {
		LOG_ERROR_BDF("find in loc[%u] fnav rule failed\n",
				flow->location);
		ret = -EINVAL;
		goto l_end;
	}

	ret = sxe_fnav_rule_parse_to_flow(adapter, rule, flow);
	if (ret) {
		LOG_ERROR_BDF("parse fnav rule[%p] to user failed\n", rule);
		goto l_end;
	}

l_end:
	return ret;
}

static int sxe_fnav_rule_locs_get(
					struct sxe_adapter *adapter,
					struct ethtool_rxnfc *cmd,
					u32 *rule_locs)
{
	struct hlist_node *next;
	struct sxe_fnav_rule_node *rule;
	int cnt = 0;

	cmd->data = sxe_fnav_max_rule_num_get(
				adapter->fnav_ctxt.rules_table_size);

	hlist_for_each_entry_safe(rule, next,
				  &adapter->fnav_ctxt.rules_list,
				  node) {
		if (cnt == cmd->rule_cnt) {
			LOG_WARN_BDF("no fnav rules found, max_rule_cnt=%u\n", cnt);
			return -EMSGSIZE;
		}
		rule_locs[cnt] = rule->sw_idx;
		cnt++;
	}

	cmd->rule_cnt = cnt;

	LOG_DEBUG_BDF("get fnav rule count=%u, table size=%llu\n",
			cmd->rule_cnt, cmd->data);
	return 0;
}

static int sxe_rss_hash_srcs_get(struct sxe_adapter *adapter,
				   struct ethtool_rxnfc *cmd)
{
	cmd->data = 0;

	switch (cmd->flow_type) {
	case TCP_V4_FLOW:
	case TCP_V6_FLOW:
		cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		fallthrough;
	case UDP_V4_FLOW:
		if (adapter->cap & SXE_RSS_FIELD_IPV4_UDP) {
			cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		}
		fallthrough;
	case SCTP_V4_FLOW:
	case AH_ESP_V4_FLOW:
	case AH_V4_FLOW:
	case ESP_V4_FLOW:
	case IPV4_FLOW:

	case SCTP_V6_FLOW:
	case AH_ESP_V6_FLOW:
	case AH_V6_FLOW:
	case ESP_V6_FLOW:
	case IPV6_FLOW:
		cmd->data |= RXH_IP_SRC | RXH_IP_DST;
		break;
	case UDP_V6_FLOW:
		if (adapter->cap & SXE_RSS_FIELD_IPV6_UDP) {
			cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		}
		cmd->data |= RXH_IP_SRC | RXH_IP_DST;
		break;
	default:
		LOG_ERROR_BDF("unkown cmd->flow_type=0x%x\n", cmd->flow_type);
		return -EINVAL;
	}

	LOG_DEBUG_BDF("cmd->flow_type[0x%x] get data[0x%llx]\n",
			cmd->flow_type, cmd->data);
	return 0;
}

static int sxe_get_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *cmd,
			   u32 *rule_locs)
{
	int ret = 0;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
		cmd->data = adapter->rx_ring_ctxt.num;
		break;
	case ETHTOOL_GRXCLSRLCNT:
		cmd->rule_cnt = adapter->fnav_ctxt.rule_cnt;
		break;
	case ETHTOOL_GRXCLSRULE:
		ret = sxe_fnav_rule_get(adapter, cmd);
		break;
	case ETHTOOL_GRXCLSRLALL:
		ret = sxe_fnav_rule_locs_get(adapter, cmd, rule_locs);
		break;
	case ETHTOOL_GRXFH:
		ret = sxe_rss_hash_srcs_get(adapter, cmd);
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	return ret;
}

static s32 sxe_fnav_flow_param_check(struct sxe_adapter *adapter,
					struct ethtool_rx_flow_spec *flow)
{
	s32 ret = 0;
	u64 max_rule_num;

	if (!(adapter->cap & SXE_FNAV_SPECIFIC_ENABLE)) {
		ret = -EOPNOTSUPP;
		LOG_ERROR_BDF("sxe not in specific fnav mode\n");
		goto l_ret;
	}

	max_rule_num = sxe_fnav_max_rule_num_get(
		adapter->fnav_ctxt.rules_table_size);

	if (flow->location >= max_rule_num) {
		LOG_MSG_ERR(drv, "location[%u] out of range[%llu]\n",
			flow->location, max_rule_num);
		ret = -EINVAL;
	}

l_ret:
	return ret;
}

STATIC int sxe_fnav_flow_type_parse(
					struct ethtool_rx_flow_spec *flow,
					u8 *flow_type)
{
	s32 ret = 0;

	switch (flow->flow_type & ~FLOW_EXT) {
	case TCP_V4_FLOW:
		*flow_type = SXE_SAMPLE_FLOW_TYPE_TCPV4;
		break;
	case UDP_V4_FLOW:
		*flow_type = SXE_SAMPLE_FLOW_TYPE_UDPV4;
		break;
	case SCTP_V4_FLOW:
		*flow_type = SXE_SAMPLE_FLOW_TYPE_SCTPV4;
		break;
	case IP_USER_FLOW:
		switch (flow->h_u.usr_ip4_spec.proto) {
		case IPPROTO_TCP:
			*flow_type = SXE_SAMPLE_FLOW_TYPE_TCPV4;
			break;
		case IPPROTO_UDP:
			*flow_type = SXE_SAMPLE_FLOW_TYPE_UDPV4;
			break;
		case IPPROTO_SCTP:
			*flow_type = SXE_SAMPLE_FLOW_TYPE_SCTPV4;
			break;
		case 0:
			if (!flow->m_u.usr_ip4_spec.proto) {
				*flow_type = SXE_SAMPLE_FLOW_TYPE_IPV4;
				break;
			}
			LOG_WARN("pass throuth to default in proto[%u]\n",
				flow->m_u.usr_ip4_spec.proto);
			fallthrough;
		default:
			LOG_WARN("unknow IP_USER_FLOW proto[%u]\n",
				flow->h_u.usr_ip4_spec.proto);
			ret = -EINVAL;
		}
		break;
	default:
		LOG_WARN("unknow flow type[0x%x]\n",
				flow->flow_type & ~FLOW_EXT);
		ret = -EINVAL;
	}

	LOG_DEBUG("parse flow type = 0x%x\n", *flow_type);
	return ret;
}

static s32 sxe_fnav_rule_parse_from_flow(
					struct sxe_adapter *adapter,
					struct sxe_fnav_rule_node *rule,
					union  sxe_fnav_rule_info *mask,
					struct ethtool_rx_flow_spec *flow)
{
	s32 ret;

	rule->sw_idx = flow->location;

	ret = sxe_fnav_flow_type_parse(flow,
					 &rule->rule_info.ntuple.flow_type);
	if (ret) {
		LOG_MSG_ERR(drv, "unrecognized flow type:0x%x\n",
			rule->rule_info.ntuple.flow_type);
		ret = -EINVAL;
		goto l_ret;
	}

	mask->ntuple.flow_type = SXE_SAMPLE_L4TYPE_IPV6_MASK |
				   SXE_SAMPLE_L4TYPE_MASK;

	if (rule->rule_info.ntuple.flow_type == SXE_SAMPLE_FLOW_TYPE_IPV4) {
		LOG_DEBUG_BDF("note: entry SXE_SAMPLE_FLOW_TYPE_IPV4\n");
		mask->ntuple.flow_type &= SXE_SAMPLE_L4TYPE_IPV6_MASK;
	}
	LOG_DEBUG_BDF("mask's flow_type=0x%x\n", mask->ntuple.flow_type);

	rule->rule_info.ntuple.src_ip[0] = flow->h_u.tcp_ip4_spec.ip4src;
	rule->rule_info.ntuple.dst_ip[0] = flow->h_u.tcp_ip4_spec.ip4dst;
	rule->rule_info.ntuple.src_port = flow->h_u.tcp_ip4_spec.psrc;
	rule->rule_info.ntuple.dst_port = flow->h_u.tcp_ip4_spec.pdst;

	mask->ntuple.src_ip[0] = flow->m_u.tcp_ip4_spec.ip4src;
	mask->ntuple.dst_ip[0] = flow->m_u.tcp_ip4_spec.ip4dst;
	mask->ntuple.src_port = flow->m_u.tcp_ip4_spec.psrc;
	mask->ntuple.dst_port = flow->m_u.tcp_ip4_spec.pdst;

	if (flow->flow_type & FLOW_EXT) {
		rule->rule_info.ntuple.vm_pool =
				(unsigned char)ntohl(flow->h_ext.data[1]);
		rule->rule_info.ntuple.vlan_id = flow->h_ext.vlan_tci;
		rule->rule_info.ntuple.flex_bytes =
						flow->h_ext.vlan_etype;

		mask->ntuple.vm_pool =
				(unsigned char)ntohl(flow->m_ext.data[1]);
		mask->ntuple.vlan_id = flow->m_ext.vlan_tci;
		mask->ntuple.flex_bytes = flow->m_ext.vlan_etype;
	}

	rule->ring_cookie = flow->ring_cookie;

l_ret:
	return ret;
}

static int sxe_fnav_specific_rule_add(
					struct sxe_adapter *adapter,
					struct ethtool_rxnfc *cmd)
{
	struct ethtool_rx_flow_spec *flow =
		(struct ethtool_rx_flow_spec *)&cmd->fs;
	struct sxe_fnav_rule_node *rule;
	union  sxe_fnav_rule_info mask;
	u8 queue;
	s32 ret;

	ret = sxe_fnav_flow_param_check(adapter, flow);
	if (ret) {
		LOG_ERROR_BDF("fnav param check failed, ret=%d\n", ret);
		goto l_err_ret;
	}

	ret = sxe_fnav_dest_queue_parse(adapter, flow->ring_cookie, &queue);
	if (ret) {
		LOG_ERROR_BDF("get fnav destination queue failed, ret=%d\n", ret);
		goto l_err_ret;
	}

	rule = kzalloc(sizeof(*rule), GFP_ATOMIC);
	if (!rule) {
		LOG_ERROR_BDF("malloc rule mem failed\n");
		ret = -ENOMEM;
		goto l_err_ret;
	}

	memset(&mask, 0, sizeof(union sxe_fnav_rule_info));

	ret = sxe_fnav_rule_parse_from_flow(adapter, rule, &mask, flow);
	if (ret) {
		LOG_ERROR_BDF("get fnav rule info failed, ret=%d\n", ret);
		goto l_err_free;
	}

	ret = sxe_fnav_specific_rule_add_process(adapter, rule, &mask, queue);
	if (ret) {
		ret = -EINVAL;
		LOG_ERROR_BDF("add fnav rule failed, ret=%d\n", ret);
		goto l_err_free;
	}

	return ret;

l_err_free:
	kfree(rule);
	rule = NULL;
l_err_ret:
	return ret;
}

static int sxe_fnav_specific_rule_del(
					struct sxe_adapter *adapter,
					struct ethtool_rxnfc *cmd)
{
	struct ethtool_rx_flow_spec *user_flow =
		(struct ethtool_rx_flow_spec *)&cmd->fs;
	int ret;

	spin_lock(&adapter->fnav_ctxt.specific_lock);
	ret = sxe_fnav_sw_specific_rule_del(adapter, user_flow->location);
	spin_unlock(&adapter->fnav_ctxt.specific_lock);

	return ret;
}

static int sxe_rss_hash_srcs_update(struct ethtool_rxnfc *nfc, u32 *cap)
{
	int ret = -EINVAL;

	LOG_INFO("user input nfc->data = 0x%llx， nfc->flow_type=0x%x \n",
		nfc->data, nfc->flow_type);

	if (nfc->data & ~(RXH_IP_SRC | RXH_IP_DST |
			  RXH_L4_B_0_1 | RXH_L4_B_2_3)) {
		LOG_ERROR("data[0x%llx] exist unsupport hash srcs\n", nfc->data);
		goto l_err_ret;
	}

	switch (nfc->flow_type) {
	case TCP_V4_FLOW:
	case TCP_V6_FLOW:
		if (!(nfc->data & RXH_IP_SRC) ||
		    !(nfc->data & RXH_IP_DST) ||
		    !(nfc->data & RXH_L4_B_0_1) ||
		    !(nfc->data & RXH_L4_B_2_3)) {
			LOG_ERROR("data[0x%llx] in flow_type[0x%x] does not"
				"exist support hash srcs\n",
				nfc->data, nfc->flow_type);
			goto l_err_ret;
		}

		break;
	case UDP_V4_FLOW:
		if (!(nfc->data & RXH_IP_SRC) ||
		    !(nfc->data & RXH_IP_DST)) {
			LOG_ERROR("data[0x%llx] in flow_type[UDP_V4_FLOW] does not"
				"exist support hash srcs\n",
				nfc->data);
			goto l_err_ret;
		}

		switch (nfc->data & (RXH_L4_B_0_1 | RXH_L4_B_2_3)) {
		case 0:
			*cap &= ~SXE_RSS_FIELD_IPV4_UDP;
			break;
		case (RXH_L4_B_0_1 | RXH_L4_B_2_3):
			*cap |= SXE_RSS_FIELD_IPV4_UDP;
			break;
		default:
			goto l_err_ret;
		}
		break;
	case UDP_V6_FLOW:
		if (!(nfc->data & RXH_IP_SRC) ||
		    !(nfc->data & RXH_IP_DST)) {

			LOG_ERROR("data[0x%llx] in flow_type[UDP_V6_FLOW] does not"
				"exist support hash srcs\n",
				nfc->data);
			goto l_err_ret;
		}

		switch (nfc->data & (RXH_L4_B_0_1 | RXH_L4_B_2_3)) {
		case 0:
			*cap &= ~SXE_RSS_FIELD_IPV6_UDP;
			break;
		case (RXH_L4_B_0_1 | RXH_L4_B_2_3):
			*cap |= SXE_RSS_FIELD_IPV6_UDP;
			break;
		default:
			goto l_err_ret;
		}
		break;
	case AH_ESP_V4_FLOW:
	case AH_V4_FLOW:
	case ESP_V4_FLOW:
	case SCTP_V4_FLOW:
	case AH_ESP_V6_FLOW:
	case AH_V6_FLOW:
	case ESP_V6_FLOW:
	case SCTP_V6_FLOW:
		if (!(nfc->data & RXH_IP_SRC) ||
		    !(nfc->data & RXH_IP_DST) ||
		    (nfc->data & RXH_L4_B_0_1) ||
		    (nfc->data & RXH_L4_B_2_3)) {
			goto l_err_ret;
		}

		break;
	default:
		goto l_err_ret;
	}
	LOG_ERROR("cap=0x%x\n", *cap);
	return 0;

l_err_ret:
	return ret;
}

static int sxe_rss_hash_srcs_set(struct sxe_adapter *adapter,
				  struct ethtool_rxnfc *nfc)
{
	u32 cap = adapter->cap;
	u32 version = 0;
	int ret;

	ret = sxe_rss_hash_srcs_update(nfc, &cap);
	if (ret) {
		LOG_ERROR_BDF("rss hash srcs update failed\n");
		goto l_ret;
	}

	if (cap != adapter->cap) {
		struct sxe_hw *hw = &adapter->hw;
		if ((cap & UDP_RSS_FLAGS) &&
		    !(adapter->cap & UDP_RSS_FLAGS)) {
			LOG_MSG_WARN(drv, "enabling udp rss: fragmented packets may"
				"arrive out of order to the stack above\n");
		}

		adapter->cap = cap;
		if (cap & SXE_RSS_FIELD_IPV4_UDP) {
			version = SXE_RSS_IP_VER_4;
		} else if(cap & SXE_RSS_FIELD_IPV6_UDP) {
			version = SXE_RSS_IP_VER_6;
		}
		LOG_DEBUG_BDF("cap=[0x%x], version=%u\n", cap, version);
		hw->dbu.ops->rss_hash_pkt_type_update(hw, version);
	}

l_ret:
	return ret;
}

static int sxe_set_rxnfc(struct net_device *netdev,
				struct ethtool_rxnfc *cmd)
{
	int ret = -EOPNOTSUPP;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	switch (cmd->cmd) {
	case ETHTOOL_SRXCLSRLINS:
		ret = sxe_fnav_specific_rule_add(adapter, cmd);
		break;
	case ETHTOOL_SRXCLSRLDEL:
		ret = sxe_fnav_specific_rule_del(adapter, cmd);
		break;
	case ETHTOOL_SRXFH:
		ret = sxe_rss_hash_srcs_set(adapter, cmd);
		break;
	default:
		break;
	}

	return ret;
}

static u32 sxe_get_rxfh_key_size(struct net_device *netdev)
{
	return SXE_RSS_KEY_SIZE;
}

static u32 sxe_rss_indir_size(struct net_device *netdev)
{
	return sxe_rss_redir_tbl_size_get();
}

static void sxe_rss_redir_tbl_get(struct sxe_adapter *adapter, u32 *indir)
{
	u32 i;
	u32 tbl_size = sxe_rss_redir_tbl_size_get();
	u16 rss_m = sxe_rss_mask_get(adapter);

	if (adapter->cap & SXE_SRIOV_ENABLE) {
		rss_m = sxe_rss_num_get(adapter) - 1;
	}

	for (i = 0; i < tbl_size; i++) {
		indir[i] = adapter->rss_indir_tbl[i] & rss_m;
	}

	return ;
}

static int sxe_get_rxfh(struct net_device *netdev,
				u32 *indir, u8 *key, u8 *hfunc)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	if (hfunc) {
		*hfunc = ETH_RSS_HASH_TOP;
	}

	if (indir) {
		sxe_rss_redir_tbl_get(adapter, indir);
	}

	if (key) {
		memcpy(key, adapter->rss_key, sxe_get_rxfh_key_size(netdev));
	}

	return 0;
}

static int sxe_set_rxfh(struct net_device *netdev, const u32 *redir,
			  const u8 *key, const u8 hfunc)
{
	u16 i, max_queues;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	u16 rss = sxe_rss_num_get(adapter);
	u32 tbl_entries = sxe_rss_redir_tbl_size_get();
	struct sxe_hw *hw = &adapter->hw;

	LOG_DEBUG_BDF("rss=%u, tbl_entries=%u\n", rss, tbl_entries);
	if (hfunc) {
		LOG_ERROR_BDF("sxe unsupport hfunc[%d]\n", hfunc);
		return -EINVAL;
	}

	if (redir) {
		max_queues = min_t(int,
				adapter->rx_ring_ctxt.num, rss);

		if ((adapter->cap & SXE_SRIOV_ENABLE) && (max_queues < 2)) {
			max_queues = 2;
		}

		for (i = 0; i < tbl_entries; i++) {
			if (redir[i] >= max_queues) {
				LOG_ERROR_BDF("indir[%u]=%u > max_que=%u\n",
						i, redir[i], max_queues);
				return -EINVAL;
			}
		}

		for (i = 0; i < tbl_entries; i++) {
			adapter->rss_indir_tbl[i] = redir[i];
		}

		hw->dbu.ops->rss_redir_tbl_set_all(hw, adapter->rss_indir_tbl);
	}

	if (key) {
		memcpy(adapter->rss_key, key, sxe_get_rxfh_key_size(netdev));
		hw->dbu.ops->rss_key_set_all(hw, adapter->rss_key);
	}

	return 0;
}

#ifdef HAVE_ETHTOOL_EXTENDED_RINGPARAMS
static void
sxe_get_ringparam(struct net_device *netdev,
		    struct ethtool_ringparam *ring,
		    struct kernel_ethtool_ringparam __always_unused *kernel_ring,
		    struct netlink_ext_ack __always_unused *extack)
#else
static void sxe_get_ringparam(struct net_device *netdev,
				struct ethtool_ringparam *ring)
#endif
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	ring->rx_max_pending = SXE_DESC_CNT_MAX;
	ring->tx_max_pending = SXE_DESC_CNT_MAX;
	ring->rx_pending = adapter->rx_ring_ctxt.ring[0]->depth;
	ring->tx_pending = adapter->tx_ring_ctxt.ring[0]->depth;

	return;
}

inline static bool sxe_ringparam_changed(struct sxe_adapter *adapter,
				struct ethtool_ringparam *ring,
				u32 *tx_cnt, u32 *rx_cnt)
{
	bool changed = true;

	*tx_cnt = clamp_t(u32, ring->tx_pending,
			       SXE_DESC_CNT_MIN, SXE_DESC_CNT_MAX);
	*tx_cnt = ALIGN(*tx_cnt, SXE_REQ_DESCRIPTOR_MULTIPLE);

	*rx_cnt = clamp_t(u32, ring->rx_pending,
			       SXE_DESC_CNT_MIN, SXE_DESC_CNT_MAX);
	*rx_cnt = ALIGN(*rx_cnt, SXE_REQ_DESCRIPTOR_MULTIPLE);

	if ((*tx_cnt == adapter->tx_ring_ctxt.depth) &&
	    (*rx_cnt == adapter->rx_ring_ctxt.depth)) {
		changed = false;
	}

	return changed;
}

inline static void sxe_ring_depth_set(struct sxe_adapter *adapter,
				u32 tx_cnt, u32 rx_cnt)
{
	u32 i;
	struct sxe_ring **tx_ring   = adapter->tx_ring_ctxt.ring;
	struct sxe_ring **rx_ring   = adapter->rx_ring_ctxt.ring;
	struct sxe_ring **xdp_ring  = adapter->xdp_ring_ctxt.ring;

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
sxe_set_ringparam(struct net_device *netdev,
		struct ethtool_ringparam *user_param,
		struct kernel_ethtool_ringparam __always_unused *kernel_ring,
		struct netlink_ext_ack __always_unused *extack)
#else
static int sxe_set_ringparam(struct net_device *netdev,
		struct ethtool_ringparam *user_param)
#endif
{
	int ret = 0;
	u32 new_rx_count, new_tx_count;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	if ((user_param->rx_mini_pending) || (user_param->rx_jumbo_pending)) {
		LOG_ERROR_BDF("dont support set rx_mini_pending=%u or rx_jumbo_pending=%u\n",
			user_param->rx_mini_pending, user_param->rx_jumbo_pending);
		ret = -EINVAL;
		goto l_end;
	}

	if (!sxe_ringparam_changed(adapter, user_param,
			&new_tx_count, &new_rx_count)) {
		LOG_DEBUG_BDF("ring depth dont change, tx_depth=%u, rx_depth=%u\n",
			new_tx_count, new_rx_count);
		goto l_end;
	}

	while (test_and_set_bit(SXE_RESETTING, &adapter->state)) {
		usleep_range(SXE_NIC_RESET_WAIT_MIN, SXE_NIC_RESET_WAIT_MAX);
	}

	if (!netif_running(adapter->netdev)) {
		sxe_ring_depth_set(adapter, new_tx_count, new_rx_count);
		goto l_clear;
	}

	sxe_down(adapter);

	if (new_tx_count != adapter->tx_ring_ctxt.depth) {
		ret = sxe_tx_ring_depth_reset(adapter, new_tx_count);
		if (ret < 0) {
			goto l_up;
		}
	}

	if (new_rx_count != adapter->rx_ring_ctxt.depth) {
		ret = sxe_rx_ring_depth_reset(adapter, new_rx_count);
	}
l_up:
	sxe_up(adapter);
l_clear:
	clear_bit(SXE_RESETTING, &adapter->state);
l_end:
	return ret;
}

static int sxe_nway_reset(struct net_device *netdev)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev)) {
		LOG_DEBUG_BDF("ethtool reset\n");
		sxe_hw_reinit(adapter);
	}

	return 0;
}

static int sxe_get_ts_info(struct net_device *dev,
			     struct ethtool_ts_info *info)
{
	struct sxe_adapter *adapter = netdev_priv(dev);

	info->rx_filters = BIT(HWTSTAMP_FILTER_NONE) |
		BIT(HWTSTAMP_FILTER_PTP_V1_L4_SYNC) |
		BIT(HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ) |
		BIT(HWTSTAMP_FILTER_PTP_V2_EVENT);

	info->so_timestamping =
		SOF_TIMESTAMPING_TX_SOFTWARE |
		SOF_TIMESTAMPING_RX_SOFTWARE |
		SOF_TIMESTAMPING_SOFTWARE |
		SOF_TIMESTAMPING_TX_HARDWARE |
		SOF_TIMESTAMPING_RX_HARDWARE |
		SOF_TIMESTAMPING_RAW_HARDWARE;

	if (adapter->ptp_ctxt.ptp_clock) {
		info->phc_index = ptp_clock_index(adapter->ptp_ctxt.ptp_clock);
	} else {
		info->phc_index = -1;
	}

	info->tx_types =
		BIT(HWTSTAMP_TX_OFF) |
		BIT(HWTSTAMP_TX_ON);

	return 0;
}

static u32 sxe_max_channels(struct sxe_adapter *adapter)
{
	u32 max_combined;
	u8 tcs = sxe_dcb_tc_get(adapter);

	if (!(adapter->cap & SXE_MSIX_ENABLED)) {
		max_combined = 1;
	} else if (adapter->cap & SXE_SRIOV_ENABLE) {
		max_combined = sxe_rss_mask_get(adapter) + 1;
	} else if (tcs > SXE_DCB_1_TC) { 
		if (tcs > SXE_DCB_4_TC) {
			max_combined = SXE_8_RING_PER_TC;
		} else {
			max_combined = SXE_16_RING_PER_TC;
		}
	} else if (adapter->fnav_ctxt.sample_rate) {
		max_combined = SXE_FNAV_RING_NUM_MAX;
	} else {
		max_combined = SXE_RSS_RING_NUM_MAX;
	}

	return min_t(int, max_combined, num_online_cpus());
}

static void sxe_get_channels(struct net_device *netdev,
			       struct ethtool_channels *ch)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	ch->max_combined = sxe_max_channels(adapter);

	if (adapter->cap & SXE_MSIX_ENABLED) {
		ch->max_other = SXE_EVENT_IRQ_NUM;
		ch->other_count = SXE_EVENT_IRQ_NUM;
	}

	ch->combined_count = sxe_rss_num_get(adapter);

	if ((1 == ch->combined_count) ||
	    (adapter->cap & SXE_SRIOV_ENABLE) ||
	    (sxe_dcb_tc_get(adapter) > 1)) {
		LOG_WARN_BDF("current combined count=%u, adapter cap=%x, "
			"tcs=%u, sample_rate=%u, dont support fnav\n",
			ch->combined_count, adapter->cap,
			sxe_dcb_tc_get(adapter),
			adapter->fnav_ctxt.sample_rate);
		goto l_end;
	}

	ch->combined_count = adapter->ring_f.fnav_num;

l_end:
	return;
}

static int sxe_set_channels(struct net_device *netdev,
			      struct ethtool_channels *ch)
{
	s32 ret;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	u32 count = ch->combined_count;

	LOG_DEBUG_BDF("user param: cmd=%u, combined=%u, max_combined=%u, "
		"max_other=%u, max_rx=%u, max_tx=%u, other_cnt=%u, "
		"rx_cnt=%u, tx_cnt=%u\n",
		ch->cmd, ch->combined_count, ch->max_combined,
		ch->max_other, ch->max_rx, ch->max_tx, ch->other_count,
		ch->rx_count, ch->tx_count);

	if (!count || ch->rx_count || ch->tx_count ||
	    (ch->other_count != SXE_EVENT_IRQ_NUM) ||
	    (count > sxe_max_channels(adapter))) {
		LOG_ERROR_BDF("ethtool set channel failed, combined count=%u, "
			  "rx_count=%u, tx_count=%u, other_count=%u, max_ch=%u\n",
			count, ch->rx_count, ch->tx_count, ch->other_count,
			sxe_max_channels(adapter));
		ret = -EINVAL;
		goto l_err;
	}

	adapter->ring_f.fnav_limit = count;

	adapter->ring_f.rss_limit = (count > SXE_RSS_RING_NUM_MAX) ? \
						SXE_RSS_RING_NUM_MAX : count;

	ret = sxe_ring_reassign(adapter, sxe_dcb_tc_get(adapter));
	if (ret) {
		LOG_ERROR_BDF("sxe_ring_reassign failed, err=%d, combined count=%u, "
			  "rx_count=%u, tx_count=%u, other_count=%u, max_ch=%u"
			  "tc=%u\n",
			ret, count, ch->rx_count, ch->tx_count, ch->other_count,
			sxe_max_channels(adapter), sxe_dcb_tc_get(adapter));
	}

l_err:
	return ret;
}

static int sxe_get_link_ksettings_proto(struct net_device *netdev,
					  struct ethtool_link_ksettings *cmd)
{
	u32 supported;   
	u32 advertising; 
	u32 speed_supported;
	bool autoneg_supported;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;

	ethtool_convert_link_mode_to_legacy_u32(&supported,
					cmd->link_modes.supported);

	adapter->phy_ctxt.ops->get_link_capabilities(adapter,
					&speed_supported, &autoneg_supported);

	if (speed_supported & SXE_LINK_SPEED_10GB_FULL) {
		supported |= SUPPORTED_10000baseKR_Full;
	}
	if (speed_supported & SXE_LINK_SPEED_1GB_FULL) {
		supported |= SUPPORTED_1000baseKX_Full;
	}

	if (adapter->phy_ctxt.autoneg_advertised) {
		advertising = 0;
		if (adapter->phy_ctxt.autoneg_advertised & SXE_LINK_SPEED_10GB_FULL) {
			advertising |= SUPPORTED_10000baseKR_Full;
		}
		if (adapter->phy_ctxt.autoneg_advertised & SXE_LINK_SPEED_1GB_FULL) {
			advertising |= SUPPORTED_1000baseKX_Full;
		}
	} else {
		advertising = supported;
	}

	if (autoneg_supported) {
		supported |= SUPPORTED_Autoneg;
		advertising |= ADVERTISED_Autoneg;
		cmd->base.autoneg = AUTONEG_ENABLE;
	} else {
		cmd->base.autoneg = AUTONEG_DISABLE;
	}

	if (adapter->phy_ctxt.is_sfp) {
		switch(adapter->phy_ctxt.sfp_info.type) {
		case SXE_SFP_TYPE_DA_CU:
			supported |= SUPPORTED_FIBRE;
			advertising |= ADVERTISED_FIBRE;
			cmd->base.port = PORT_DA;
			break;
		case SXE_SFP_TYPE_SRLR:
		case SXE_SFP_TYPE_1G_SXLX:
			supported |= SUPPORTED_FIBRE;
			advertising |= ADVERTISED_FIBRE;
			cmd->base.port = PORT_FIBRE;
			break;
		case SXE_SFP_TYPE_1G_CU:
			supported |= SUPPORTED_TP;
			advertising |= ADVERTISED_TP;
			cmd->base.port = PORT_TP;
			break;
		case SXE_SFP_TYPE_NOT_PRESENT:
			supported |= SUPPORTED_FIBRE;
			advertising |= ADVERTISED_FIBRE;
			cmd->base.port = PORT_NONE;
			break;
		default:
			supported |= SUPPORTED_FIBRE;
			advertising |= ADVERTISED_FIBRE;
			cmd->base.port = PORT_OTHER;
			break;
		}
	} else {
		supported |= SUPPORTED_TP;
		advertising |= ADVERTISED_TP;
		cmd->base.port = PORT_TP;
	}

	supported |= SUPPORTED_Pause;

	switch (hw->fc.requested_mode) {
	case SXE_FC_FULL:
		advertising |= ADVERTISED_Pause;
		break;
	case SXE_FC_RX_PAUSE:
		advertising |= ADVERTISED_Pause |
				     ADVERTISED_Asym_Pause;
		break;
	case SXE_FC_TX_PAUSE:
		advertising |= ADVERTISED_Asym_Pause;
		break;
	default:
		advertising &= ~(ADVERTISED_Pause |
				       ADVERTISED_Asym_Pause);
	}

	if (netif_carrier_ok(netdev)) {
		switch (adapter->link.speed) {
		case SXE_LINK_SPEED_10GB_FULL:
			cmd->base.speed  = SPEED_10000;
			cmd->base.duplex = DUPLEX_FULL;
			break;
		case SXE_LINK_SPEED_1GB_FULL:
			cmd->base.speed  = SPEED_1000;
			cmd->base.duplex = DUPLEX_FULL;
			break;
		case SXE_LINK_SPEED_100_FULL:
			cmd->base.speed  = SPEED_100;
			cmd->base.duplex = DUPLEX_FULL;
			break;
		case SXE_LINK_SPEED_10_FULL:
			cmd->base.speed  = SPEED_10;
			cmd->base.duplex = DUPLEX_FULL;
			break;
		default:
			cmd->base.speed  = SPEED_UNKNOWN;
			cmd->base.duplex = DUPLEX_UNKNOWN;
			break;
		}
	} else {
		cmd->base.speed  = SPEED_UNKNOWN;
		cmd->base.duplex = DUPLEX_UNKNOWN;
	}

	LOG_DEBUG_BDF("ethtool get link, speed=%x, is_up=%d, base.speed=%u\n",
			adapter->link.speed, adapter->link.is_up, cmd->base.speed);

	ethtool_convert_legacy_u32_to_link_mode(cmd->link_modes.supported,
						supported);
	ethtool_convert_legacy_u32_to_link_mode(cmd->link_modes.advertising,
						advertising);

	return 0;
}

static int sxe_set_link_ksettings_proto(struct net_device *netdev,
					const struct ethtool_link_ksettings *cmd)
{
	int ret = 0;
	u32 advertised, old;
	u32 supported, advertising;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_phy_context *phy_ctxt = &adapter->phy_ctxt;

	ethtool_convert_link_mode_to_legacy_u32(&supported,
						cmd->link_modes.supported);
	ethtool_convert_link_mode_to_legacy_u32(&advertising,
						cmd->link_modes.advertising);

	LOG_DEBUG_BDF("multispeed sfp=%d, advertising=%x, supported=%x, "
				"cmd autoneg=%x\n",
				phy_ctxt->sfp_info.multispeed_fiber,
				advertising,
				supported,
				cmd->base.autoneg);

	if (phy_ctxt->sfp_info.multispeed_fiber) {
		if (advertising & ~supported) {
			LOG_ERROR_BDF("(advertising & ~supported) > 0 failed,"
				"advertising=%x, supported=%x\n", advertising, supported);
			ret = -EINVAL;
			goto l_end;
		}

		if (!cmd->base.autoneg && phy_ctxt->sfp_info.multispeed_fiber) {
			if (advertising == (ADVERTISED_10000baseKR_Full |
				ADVERTISED_1000baseKX_Full)) {
				ret = -EINVAL;
				goto l_end;
			}
		}

		old = phy_ctxt->autoneg_advertised;
		advertised = 0;
		if (advertising & ADVERTISED_10000baseKR_Full) {
			advertised |= SXE_LINK_SPEED_10GB_FULL;
		}

		if (advertising & ADVERTISED_1000baseKX_Full) {
			advertised |= SXE_LINK_SPEED_1GB_FULL;
		}

		if (old == advertised) {
			ret = 0;
			goto l_end;
		}

		set_bit(SXE_SFP_MULTI_SPEED_SETTING, &adapter->state);
		adapter->link.sfp_multispeed_time = jiffies;
		while (test_and_set_bit(SXE_IN_SFP_INIT, &adapter->state)) {
			usleep_range(SXE_SFP_INIT_WAIT_ITR_MIN, SXE_SFP_INIT_WAIT_ITR_MAX);
		}

		smp_wmb();
		clear_bit(SXE_LINK_NEED_CONFIG, &(adapter->monitor_ctxt.state));
		set_bit(SXE_LINK_SPEED_CHANGE, &(adapter->monitor_ctxt.state));
		adapter->hw.mac.auto_restart = true;
		LOG_INFO_BDF("set auto_restart true.\n");

		ret = sxe_link_configure(adapter, advertised);
		if (ret) {
			LOG_MSG_INFO(probe, "setup link failed, ret = %d, advertised=%d\n",
							ret, advertised);
			sxe_link_configure(adapter, old);
		}
		clear_bit(SXE_IN_SFP_INIT, &adapter->state);

	} else {
		return -EPERM;
	}

l_end:
	return ret;
}

static void sxe_get_pauseparam(struct net_device *netdev,
				 struct ethtool_pauseparam *pause)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;

	u32 current_mode = hw->mac.ops->fc_requested_mode_get(hw);

	if (sxe_device_supports_autoneg_fc(hw) &&
	    !hw->mac.ops->is_fc_autoneg_disabled(hw)) {
		pause->autoneg = 1;
	} else {
		pause->autoneg = 0;
	}

	if (current_mode == SXE_FC_RX_PAUSE) {
		pause->rx_pause = 1;
	} else if (current_mode == SXE_FC_TX_PAUSE) {
		pause->tx_pause = 1;
	} else if (current_mode == SXE_FC_FULL) {
		pause->rx_pause = 1;
		pause->tx_pause = 1;
	}

	LOG_DEBUG_BDF("flow control current mode, "
		"autoneg = %u, rx_pause = %u, tx_pause = %u",
		pause->autoneg, pause->rx_pause, pause->tx_pause);

	return;
}

static int sxe_set_pauseparam(struct net_device *netdev,
				struct ethtool_pauseparam *pause)
{
	int ret = 0;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;

	bool old_autoneg_status = hw->mac.ops->is_fc_autoneg_disabled(hw);
	bool new_autoneg_status;
	enum sxe_fc_mode old_requested_mode = hw->mac.ops->fc_requested_mode_get(hw);
	enum sxe_fc_mode new_requested_mode;

	if ((pause->autoneg == AUTONEG_ENABLE) &&
	    !sxe_device_supports_autoneg_fc(hw)) {
		LOG_ERROR_BDF("netdev[%p] does not support autoneg\n", netdev);
		ret = -EINVAL;
		goto l_ret;
	}

	new_autoneg_status = (pause->autoneg != AUTONEG_ENABLE);

	if ((pause->rx_pause && pause->tx_pause) || pause->autoneg) {
		new_requested_mode = SXE_FC_FULL;
	} else if (pause->rx_pause && !pause->tx_pause) {
		new_requested_mode = SXE_FC_RX_PAUSE;
	} else if (!pause->rx_pause && pause->tx_pause) {
		new_requested_mode = SXE_FC_TX_PAUSE;
	} else {
		new_requested_mode = SXE_FC_NONE;
	}

	LOG_ERROR_BDF("netdev[%p] user set new_disable_fc_autoneg = %s, "
		"new_requested_mode = %u, old_disable_fc_autoneg = %s,"
		"old_requested_mode = %u\n", netdev,
		new_autoneg_status ? "yes" : "no", new_requested_mode,
		old_autoneg_status ? "yes" : "no", old_requested_mode);

	if (old_autoneg_status != new_autoneg_status ||
		old_requested_mode != new_requested_mode) {
		hw->mac.ops->fc_autoneg_disable_set(hw, new_autoneg_status);
		hw->mac.ops->fc_requested_mode_set(hw, new_requested_mode);

		if (netif_running(netdev)) {
			sxe_hw_reinit(adapter);
		} else {
			sxe_reset(adapter);
		}
	}

l_ret:
	return ret;
}

#ifdef SXE_WOL_CONFIGURE

bool sxe_is_wol_supported(struct sxe_adapter *adapter)
{
	return true;
}

static s32 sxe_wol_cap_check(struct sxe_adapter *adapter,
			       struct ethtool_wolinfo *wol)
{
	s32 ret = 0;

	if (!sxe_is_wol_supported(adapter)) {
		ret = -SXE_ERR_DEVICE_NOT_SUPPORTED;
		wol->supported = 0;
	}

	return ret;
}

#endif

static void sxe_get_wol(struct net_device *netdev,
			  struct ethtool_wolinfo *wol)
{
#ifdef SXE_WOL_CONFIGURE

	struct sxe_adapter *adapter = netdev_priv(netdev);

	wol->supported = WAKE_UCAST | WAKE_MCAST |
			 WAKE_BCAST | WAKE_MAGIC | WAKE_PHY;
	wol->wolopts = 0;

	if (sxe_wol_cap_check(adapter, wol) ||
	    !device_can_wakeup(&adapter->pdev->dev)) {
		goto l_ret;
	}

	if (adapter->wol & SXE_WUFC_EX) {
		wol->wolopts |= WAKE_UCAST;
	}

	if (adapter->wol & SXE_WUFC_MC) {
		wol->wolopts |= WAKE_MCAST;
	}

	if (adapter->wol & SXE_WUFC_BC) {
		wol->wolopts |= WAKE_BCAST;
	}

	if (adapter->wol & SXE_WUFC_MAG) {
		wol->wolopts |= WAKE_MAGIC;
	}

	if (adapter->wol & SXE_WUFC_LNKC) {
		wol->wolopts |= WAKE_PHY;
	}

l_ret:
	return;
#else
	wol->supported = 0;
	wol->wolopts = 0;
	return;

#endif
}

#ifdef SXE_WOL_CONFIGURE

s32 sxe_fw_wol_set(struct sxe_adapter *adapter, u32 enable)
{
	return 0;
}

#endif

static int sxe_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wol)
{
#ifdef SXE_WOL_CONFIGURE
	struct sxe_adapter *adapter = netdev_priv(netdev);
	u32 wol_old;
	int ret = 0;

	if (wol->wolopts & (WAKE_ARP | WAKE_MAGICSECURE | WAKE_FILTER)) {
		LOG_ERROR_BDF("sxe's not support wol mode[%u]\n", wol->wolopts);
		ret = -EOPNOTSUPP;
		goto l_ret;
	}

	if (sxe_wol_cap_check(adapter, wol)) {
		ret = wol->wolopts ? -EOPNOTSUPP : 0;
		goto l_ret;
	}

	wol_old = adapter->wol;
	adapter->wol = 0;

	if (wol->wolopts & WAKE_PHY) {
		adapter->wol |= SXE_WUFC_LNKC;
	}

	if (wol->wolopts & WAKE_UCAST) {
		adapter->wol |= SXE_WUFC_EX;
	}

	if (wol->wolopts & WAKE_MCAST) {
		adapter->wol |= SXE_WUFC_MC;
	}

	if (wol->wolopts & WAKE_BCAST) {
		adapter->wol |= SXE_WUFC_BC;
	}

	if (wol->wolopts & WAKE_MAGIC) {
		adapter->wol |= SXE_WUFC_MAG;
	}

	LOG_DEBUG_BDF("old wol config:0x%x, new wol config:0x%x\n",
			wol_old, adapter->wol);
	if (adapter->wol) {
		if (!wol_old) {
			sxe_fw_wol_set(adapter, 1);
		}
	} else {
		if (wol_old) {
			sxe_fw_wol_set(adapter, 0);
		}
	}

	device_set_wakeup_enable(&adapter->pdev->dev, adapter->wol);

l_ret:
	return ret;
#else
	return -EOPNOTSUPP;

#endif
}

static u32 sxe_get_msglevel(struct net_device *netdev)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	return adapter->msg_enable;
}

static void sxe_set_msglevel(struct net_device *netdev, u32 data)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	adapter->msg_enable = data;

	return;
}

STATIC irqreturn_t sxe_irq_test_handler(int irq, void *data)
{
	struct net_device *netdev = (struct net_device *) data;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;

	adapter->test_ctxt.icr |= hw->irq.ops->pending_irq_read_clear(&adapter->hw);
	LOG_INFO_BDF("irq test : in irq handler eicr=%x\n", adapter->test_ctxt.icr);

	return IRQ_HANDLED;
}

static s32 sxe_irq_test(struct sxe_adapter *adapter)
{
	bool shared_int = true;
	s32 ret = SXE_DIAG_TEST_PASSED;
	struct sxe_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	u32 irq = adapter->pdev->irq;

	if (adapter->irq_ctxt.msix_entries) {
		goto l_end;
	} else if (adapter->cap & SXE_MSI_ENABLED) {
		shared_int = false;
		LOG_INFO_BDF("test irq: msix mode\n");
		if (request_irq(irq, sxe_irq_test_handler, 0,
			netdev->name, netdev)) {
			ret = -SXE_DIAG_TEST_BLOCKED;
			goto l_end;
		}
	} else if (!request_irq(irq, sxe_irq_test_handler, IRQF_PROBE_SHARED,
				netdev->name, netdev)) {
		shared_int = false;
		LOG_INFO_BDF("test irq: intx mode, type:probe shared\n");
	} else if (request_irq(irq, sxe_irq_test_handler, IRQF_SHARED,
			       netdev->name, netdev)) {
		ret = -SXE_DIAG_TEST_BLOCKED;
		goto l_end;
	}
	LOG_MSG_INFO(hw, "testing %s interrupt\n", shared_int ? "shared" : "unshared");

	ret = hw->irq.ops->irq_test(hw, &adapter->test_ctxt.icr, shared_int);
	if (ret) {
		LOG_ERROR_BDF("testing unshared irq failed\n");
	}

	free_irq(irq, netdev);

l_end:
	return ret;
}

int sxe_reg_test(struct sxe_adapter *adapter)
{
	s32 ret;
	struct sxe_hw *hw = &adapter->hw;

	if (sxe_is_hw_fault(hw)) {
		LOG_MSG_ERR(drv, "nic hw fault - register test blocked\n");
		ret = -SXE_DIAG_TEST_BLOCKED;
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

STATIC s32 sxe_link_test(struct sxe_adapter *adapter)
{
	s32 ret;
	struct sxe_hw *hw = &adapter->hw;
	bool link_up;
	u32 link_speed;

	if (sxe_is_hw_fault(hw)) {
		ret = -SXE_DIAG_TEST_BLOCKED;
		goto l_end;
	}

	sxe_link_info_get(adapter, &link_speed, &link_up);
	if (!link_up) {
		ret = -SXE_DIAG_TEST_BLOCKED;
	} else {
		ret = SXE_DIAG_TEST_PASSED;
	}

l_end:
	return ret;
}

static void sxe_rx_buffer_clean (struct sxe_ring *ring, \
				struct sxe_rx_buffer *rx_buffer)
{
	struct sk_buff *skb;
#ifdef HAVE_DMA_ATTRS_STRUCT
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif
	if (!rx_buffer) {
		return;
	}

	if (rx_buffer->skb) {
		skb = rx_buffer->skb;
		if (SXE_CTRL_BUFFER(skb)->page_released) {
			dma_unmap_page_attrs(ring->dev,
					SXE_CTRL_BUFFER(skb)->dma,
					sxe_rx_pg_size(ring),
					DMA_FROM_DEVICE,
#ifdef HAVE_DMA_ATTRS_STRUCT
					&attrs);
#else
					SXE_RX_DMA_ATTR);
#endif
		}
		dev_kfree_skb(skb);
	}

	if (rx_buffer->page) {
		dma_sync_single_range_for_cpu(ring->dev,
						rx_buffer->dma,
						rx_buffer->page_offset,
						sxe_rx_bufsz(ring),
						DMA_FROM_DEVICE);

		dma_unmap_page_attrs(ring->dev, rx_buffer->dma,
					sxe_rx_pg_size(ring),
					DMA_FROM_DEVICE,
#ifdef HAVE_DMA_ATTRS_STRUCT
					&attrs);
#else
					SXE_RX_DMA_ATTR);
#endif
		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
	}
}

static void sxe_test_ring_free(struct sxe_adapter *adapter)
{
	struct sxe_ring *tx_ring;
	struct sxe_ring *rx_ring;
	struct sxe_rx_buffer *rx_buffer;
	struct sxe_rx_buffer *rx_buffer_info;
	u16 nta;
	sxe_hw_rx_disable(adapter);
	sxe_hw_tx_disable(adapter);

	sxe_reset(adapter);

	tx_ring = &adapter->test_ctxt.tx_ring;
	rx_ring = &adapter->test_ctxt.rx_ring;
	sxe_tx_ring_free(tx_ring);
	rx_buffer_info = rx_ring->rx_buffer_info;
	if (rx_buffer_info) {
		nta = rx_ring->next_to_alloc;
		rx_buffer = &rx_buffer_info[nta];
		sxe_rx_buffer_clean(rx_ring, rx_buffer);
	}
	sxe_rx_ring_free(rx_ring);

	return;
}

STATIC s32 sxe_test_ring_configure(struct sxe_adapter *adapter)
{
	s32 ret;
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_ring *tx_ring = &adapter->test_ctxt.tx_ring;
	struct sxe_ring *rx_ring = &adapter->test_ctxt.rx_ring;

	ret = sxe_test_tx_configure(adapter, tx_ring);
	if (ret) {
		ret = -SXE_DIAG_TX_RING_CONFIGURE_ERR;
		LOG_ERROR_BDF("test tx ring config failed, ret=%d\n", ret);
		goto l_end;
	}

	ret = sxe_test_rx_configure(adapter, rx_ring);
	if (ret) {
		ret = -SXE_DIAG_RX_RING_CONFIGURE_ERR;
		LOG_ERROR_BDF("test rx ring config failed, ret=%d\n", ret);
		goto err_nomem;
	}

	hw->mac.ops->txrx_enable(hw);

	return 0;

err_nomem:
	sxe_test_ring_free(adapter);
l_end:
	return ret;
}

#define SXE_DEFAULT_MTU 1500

static s32 sxe_loopback_pcs_init(struct sxe_adapter *adapter, sxe_pcs_mode_e mode,
			u32 max_frame)
{
	s32 ret;
	sxe_pcs_cfg_s pcs_cfg;
	struct sxe_driver_cmd cmd;
	struct sxe_hw *hw = &adapter->hw;

	pcs_cfg.mode = mode;
	pcs_cfg.mtu  = max_frame;

	cmd.req      = &pcs_cfg;
	cmd.req_len  = sizeof(pcs_cfg);
	cmd.resp     = NULL;
	cmd.resp_len = 0;
	cmd.trace_id = 0;
	cmd.opcode   = SXE_CMD_PCS_SDS_INIT;
	cmd.is_interruptible = true;
	ret = sxe_driver_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("hdc trans failed ret=%d, cmd:pcs init\n", ret);
		goto l_end;
	}

	sxe_fc_mac_addr_set(adapter);

	LOG_INFO_BDF("mode:%u loopback pcs init done.\n", mode);

l_end:
	return ret;
}

static void sxe_loopback_test_setup(struct sxe_adapter *adapter)
{
	s32 ret;
	u32 max_frme = SXE_DEFAULT_MTU + ETH_FRAME_LEN + ETH_FCS_LEN;

	(void)sxe_sfp_tx_laser_disable(adapter);

	ret = sxe_loopback_pcs_init(adapter, SXE_PCS_MODE_10GBASE_KR_WO, max_frme);
	if (ret) {
		LOG_ERROR_BDF("pcs sds init failed, mode=%d, ret=%d\n",
					SXE_PCS_MODE_10GBASE_KR_WO, ret);
	}

	ret = sxe_loopback_pcs_init(adapter, SXE_PCS_MODE_LPBK_PHY_TX2RX,
							max_frme);
	if (ret) {
		LOG_ERROR_BDF("pcs sds init failed, mode=%d, ret=%d\n",
					SXE_PCS_MODE_LPBK_PHY_TX2RX, ret);
	}

	usleep_range(SXE_LPBK_TX_DISB_WAIT_MIN, SXE_LPBK_TX_DISB_WAIT_MAX);

	return;
}

static void sxe_loopback_frame_create(struct sk_buff *skb,
					unsigned int frame_size)
{
	memset(skb->data, 0xFF, frame_size);
	frame_size >>= 1;
	memset(&skb->data[frame_size], 0xAA, frame_size / 2 - 1);
	skb->data[frame_size + 10] = 0xBE;
	skb->data[frame_size + 12] = 0xAF;
	return;
}

STATIC bool sxe_loopback_frame_check(struct sxe_rx_buffer *rx_buffer,
					unsigned int frame_size)
{
	u8 *data;
	bool match = true;

	data = kmap(rx_buffer->page) + rx_buffer->page_offset;

	frame_size >>= 1;
	if (data[3] != 0xFF ||
	    data[frame_size + 10] != 0xBE ||
	    data[frame_size + 12] != 0xAF) {
		match = false;
	}

	kunmap(rx_buffer->page);

	return match;
}

static u16 sxe_test_ring_clean(struct sxe_ring *rx_ring,
				struct sxe_ring *tx_ring,
				u32 size)
{
	u16 rx_ntc, tx_ntc, count = 0;
	union sxe_tx_data_desc *tx_desc;
	union sxe_rx_data_desc *rx_desc;
	struct sxe_tx_buffer *tx_buffer;
	struct sxe_rx_buffer *rx_buffer;
	struct sxe_adapter *adapter = netdev_priv(rx_ring->netdev);

	rx_ntc = rx_ring->next_to_clean;
	tx_ntc = tx_ring->next_to_clean;
	rx_desc = SXE_RX_DESC(rx_ring, rx_ntc);

	while (tx_ntc != tx_ring->next_to_use) {
		tx_desc = SXE_TX_DESC(tx_ring, tx_ntc);

		if (!(tx_desc->wb.status & cpu_to_le32(SXE_TX_DESC_STAT_DD))) {
			LOG_ERROR_BDF("xmit dont completed, next_to_use=%u, count=%u\n",
					tx_ring->next_to_use, count);
			return count;
		}

		tx_buffer = &tx_ring->tx_buffer_info[tx_ntc];

		dev_kfree_skb_any(tx_buffer->skb);

		dma_unmap_single(tx_ring->dev,
				 dma_unmap_addr(tx_buffer, dma),
				 dma_unmap_len(tx_buffer, len),
				 DMA_TO_DEVICE);
		dma_unmap_len_set(tx_buffer, len, 0);

		tx_ntc++;
		if (tx_ntc == tx_ring->depth) {
			tx_ntc = 0;
		}

		count++;
	}

	count = 0;
	while (rx_desc->wb.upper.length) {
		rx_buffer = &rx_ring->rx_buffer_info[rx_ntc];

		dma_sync_single_for_cpu(rx_ring->dev,
					rx_buffer->dma,
					sxe_rx_bufsz(rx_ring),
					DMA_FROM_DEVICE);

		if (sxe_loopback_frame_check(rx_buffer, size)) {
			count++;
		} else {
			break;
		}

		dma_sync_single_for_device(rx_ring->dev,
					   rx_buffer->dma,
					   sxe_rx_bufsz(rx_ring),
					   DMA_FROM_DEVICE);

		rx_ntc++;
		if (rx_ntc == rx_ring->depth) {
			rx_ntc = 0;
		}

		rx_desc = SXE_RX_DESC(rx_ring, rx_ntc);
	}
	LOG_DEBUG_BDF("revice pkg num=%u\n", count);

	netdev_tx_reset_queue(netdev_get_tx_queue(tx_ring->netdev, tx_ring->idx));

	sxe_rx_ring_buffers_alloc(rx_ring, count);
	rx_ring->next_to_clean = rx_ntc;
	tx_ring->next_to_clean = tx_ntc;

	return count;
}

STATIC int sxe_loopback_test_run(struct sxe_adapter *adapter)
{
	s32 ret = SXE_DIAG_TEST_PASSED;
	u32 size = SXE_LOOPBACK_TEST_FRAME_SIZE;     
	struct sk_buff *skb;
	u32 i, j, lc, good_cnt;
	netdev_tx_t tx_ret_val;
	u32 cap_orig = adapter->cap;
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_ring *tx_ring = &adapter->test_ctxt.tx_ring;
	struct sxe_ring *rx_ring = &adapter->test_ctxt.rx_ring;
	struct sxe_mac_stats *hw_stats = &adapter->stats.hw;

	adapter->cap &= ~SXE_DCB_ENABLE;

	skb = alloc_skb(size, GFP_KERNEL);
	if (!skb) {
		ret = -SXE_DIAG_ALLOC_SKB_ERR;
		goto l_end;
	}

	sxe_loopback_frame_create(skb, size);
	skb_put(skb, size);

	if (rx_ring->depth <= tx_ring->depth) {
		lc = ((tx_ring->depth / SXE_LOOPBACK_TEST_DESC_COUNT) * SXE_LOOPBACK_TEST_LOOP) + 1;
	} else {
		lc = ((rx_ring->depth / SXE_LOOPBACK_TEST_DESC_COUNT) * SXE_LOOPBACK_TEST_LOOP) + 1;
	}

	for (j = 0; j <= lc; j++) {

		hw->stat.ops->stats_get(hw, hw_stats);
		LOG_DEBUG_BDF("max_loop_num=%u, cnt=%u original gptc:%llu gprc:%llu "
			      "qptc:%llu dbutxtcin:%llu dbutxtcout:%llu "
			      "qprc:%llu dburxtcin:%llu dburxtcout:%llu "
			      "crcerrs:%llu rfc:%llu\n",
			      lc, j, hw_stats->gptc,
			      hw_stats->gprc,
			      hw_stats->qptc[0],
			      hw_stats->dbutxtcin[0],
			      hw_stats->dbutxtcout[0],
			      hw_stats->qprc[0],
			      hw_stats->dburxtcin[0],
			      hw_stats->dburxtcout[0],
			      hw_stats->crcerrs, hw_stats->rfc);

		good_cnt = 0;
		for (i = 0; i < 64; i++) {
			skb_get(skb);
			tx_ret_val = sxe_ring_xmit(skb, adapter->netdev, tx_ring);
			if (tx_ret_val == NETDEV_TX_OK) {
				good_cnt++;
			}
		}

		hw->stat.ops->stats_get(hw, hw_stats);
		LOG_DEBUG_BDF("=====j:%u tx done==== gptc:%llu gprc:%llu "
			      "qptc:%llu dbutxtcin:%llu dbutxtcout:%llu"
			      "qprc:%llu dburxtcin:%llu dburxtcout:%llu"
			      "crcerrs:%llu rfc:%llu\n",
			      j, hw_stats->gptc, hw_stats->gprc,
			      hw_stats->qptc[0],
			      hw_stats->dbutxtcin[0],
			      hw_stats->dbutxtcout[0],
			      hw_stats->qprc[0],
			      hw_stats->dburxtcin[0],
			      hw_stats->dburxtcout[0],
			      hw_stats->crcerrs, hw_stats->rfc);

		if (good_cnt != 64) {
			LOG_ERROR_BDF("xmit pkg num=%u, !=64\n", good_cnt);
			ret = -SXE_DIAG_LOOPBACK_SEND_TEST_ERR;
			break;
		}

		msleep(200);

		good_cnt = sxe_test_ring_clean(rx_ring, tx_ring, size);

		hw->stat.ops->stats_get(hw, hw_stats);
		LOG_DEBUG_BDF("====j:%u rx done===== gptc:%llu gprc:%llu "
			      "qptc:%llu dbutxtcin:%llu dbutxtcout:%llu"
			      "qprc:%llu dburxtcin:%llu dburxtcout:%llu"
			      "crcerrs:%llu rfc:%llu\n",
			      j, hw_stats->gptc, hw_stats->gprc,
			      hw_stats->qptc[0],
			      hw_stats->dbutxtcin[0],
			      hw_stats->dbutxtcout[0],
			      hw_stats->qprc[0],
			      hw_stats->dburxtcin[0],
			      hw_stats->dburxtcout[0],
			      hw_stats->crcerrs, hw_stats->rfc);

		if (good_cnt != 64) {
			LOG_ERROR_BDF("recive pkg num=%u, !=64\n", good_cnt);
			ret = -SXE_DIAG_LOOPBACK_RECV_TEST_ERR;
			break;
		}
	}

	kfree_skb(skb);
	adapter->cap = cap_orig;
l_end:
	return ret;
}

STATIC s32 sxe_loopback_test(struct sxe_adapter *adapter)
{
	s32 ret;
	LOG_DEBUG_BDF("loopback test start\n");

	ret = sxe_test_ring_configure(adapter);
	if (ret) {
		goto l_end;
	}

	LOG_DEBUG_BDF("test_ring_configure end\n");

	sxe_loopback_test_setup(adapter);

	LOG_DEBUG_BDF("loopback_test_setup end\n");

	ret = sxe_loopback_test_run(adapter);

	LOG_DEBUG_BDF("sxe_loopback_test_run end\n");

	sxe_test_ring_free(adapter);
l_end:
	LOG_INFO_BDF("loopback test end, ret = %d\n", ret);
	return ret;
}

static inline void sxe_all_test_result_set(u64 *res, s32 value)
{
	res[SXE_DIAG_REGS_TEST]     = value;
	res[SXE_DIAG_EEPROM_TEST]   = value;
	res[SXE_DIAG_IRQ_TEST]      = value;
	res[SXE_DIAG_LOOPBACK_TEST] = value;
	res[SXE_DIAG_LINK_TEST]     = value;
}

static void sxe_diag_test(struct net_device *netdev,
			struct ethtool_test *eth_test, u64 *result)
{
	u32 i;
	s32 ret;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	bool if_running = netif_running(netdev);
	struct sxe_hw *hw = &adapter->hw;

	LOG_INFO_BDF("ethtool -t start\n");

	if (sxe_is_hw_fault(hw)) {
		eth_test->flags |= ETH_TEST_FL_FAILED;
		sxe_all_test_result_set(result, SXE_DIAG_TEST_BLOCKED);
		LOG_MSG_ERR(hw, "nic hw fault - test blocked\n");
		goto l_end;
	}

	set_bit(SXE_TESTING, &adapter->state);

	if (eth_test->flags != ETH_TEST_FL_OFFLINE) {
		LOG_MSG_INFO(hw, "online testing starting\n");

		ret = sxe_link_test(adapter);
		if (ret) {
			eth_test->flags |= ETH_TEST_FL_FAILED;
		}
		result[SXE_DIAG_LINK_TEST] = -ret;

		result[SXE_DIAG_REGS_TEST]     = SXE_DIAG_TEST_PASSED;
		result[SXE_DIAG_EEPROM_TEST]   = SXE_DIAG_TEST_PASSED;
		result[SXE_DIAG_IRQ_TEST]      = SXE_DIAG_TEST_PASSED;
		result[SXE_DIAG_LOOPBACK_TEST] = SXE_DIAG_TEST_PASSED;

		clear_bit(SXE_TESTING, &adapter->state);
		goto skip_ol_tests;
	}

	if (adapter->cap & SXE_SRIOV_ENABLE) {
		for (i = 0; i < adapter->vt_ctxt.num_vfs; i++) {
			if (adapter->vt_ctxt.vf_info[i].is_ready) {
				LOG_DEV_WARN("offline diagnostic is not "
					"supported when VFs are present\n");
				sxe_all_test_result_set(result, SXE_DIAG_TEST_BLOCKED);
				eth_test->flags |= ETH_TEST_FL_FAILED;
				clear_bit(SXE_TESTING, &adapter->state);
				goto skip_ol_tests;
			}
		}
	}

	LOG_MSG_INFO(hw, "offline testing starting\n");

	msleep_interruptible(SXE_TEST_SLEEP_TIME * SXE_HZ_TRANSTO_MS);
	ret = sxe_link_test(adapter);
	if (ret) {
		eth_test->flags |= ETH_TEST_FL_FAILED;
	}
	result[SXE_DIAG_LINK_TEST] = -ret;

	if (if_running) {
		sxe_close(netdev);
	} else {
		sxe_reset(adapter);
	}

	LOG_MSG_INFO(hw, "register testing starting\n");
	ret = sxe_reg_test(adapter);
	if (ret) {
		eth_test->flags |= ETH_TEST_FL_FAILED;
	}
	result[SXE_DIAG_REGS_TEST] = -ret;

	sxe_reset(adapter);

	result[SXE_DIAG_EEPROM_TEST] = SXE_DIAG_TEST_PASSED;

	LOG_MSG_INFO(hw, "interrupt testing starting\n");
	ret = sxe_irq_test(adapter);
	if (ret) {
		eth_test->flags |= ETH_TEST_FL_FAILED;
	}
	result[SXE_DIAG_IRQ_TEST] = -ret;

	sxe_reset(adapter);

	if (adapter->cap & (SXE_SRIOV_ENABLE | SXE_MACVLAN_ENABLE)) {
		LOG_MSG_INFO(hw, "skip mac loopback diagnostic in vt mode\n");
		result[SXE_DIAG_LOOPBACK_TEST] = 0;
		goto skip_loopback;
	}

	LOG_MSG_INFO(hw, "loopback testing starting\n");
	ret = sxe_loopback_test(adapter);
	if (ret) {
		eth_test->flags |= ETH_TEST_FL_FAILED;
	}
	result[SXE_DIAG_LOOPBACK_TEST] = -ret;

skip_loopback:
	sxe_reset(adapter);

	clear_bit(SXE_TESTING, &adapter->state);

	if (if_running) {
		sxe_open(netdev);
	} else if (adapter->phy_ctxt.ops->sfp_tx_laser_disable) {
		adapter->phy_ctxt.ops->sfp_tx_laser_disable(adapter);
	}

skip_ol_tests:
	msleep_interruptible(SXE_TEST_SLEEP_TIME * SXE_HZ_TRANSTO_MS);
l_end:
	LOG_INFO_BDF("ethtool -t end\n");
	return;
}

static int sxe_regs_len_get(struct net_device *netdev)
{
	return SXE_ETHTOOL_DUMP_REGS_LEN;
}

static void sxe_regs_get(struct net_device *netdev,
		struct ethtool_regs *regs, void *data)
{
	u32 i;
	u64 *p;
	u8 dump_regs_num;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;

	memset(data, 0, SXE_ETHTOOL_DUMP_REGS_LEN);

	regs->version = 0;

	stats_lock(adapter);
	dump_regs_num = hw->stat.ops->mac_stats_dump(hw, data,
						SXE_MAC_REGS_VAL_LEN);

	LOG_DEBUG_BDF("mac stats:\n");
	p = (u64 *)(((u8 *)data) + SXE_MAC_REGS_VAL_LEN);
	for (i = 0; i < SXE_MAC_STATS_REGS_NUM; i++) {
		p[i] = *(u64 *)(((s8 *)&adapter->stats.hw) +
					mac_stats[i].stat_offset);
		LOG_DEBUG_BDF("%s:%llu\n", mac_stats[i].stat_string, p[i]);
	}

	dump_regs_num += SXE_MAC_STATS_REGS_NUM;

	if (dump_regs_num != SXE_ETHTOOL_DUMP_REGS_NUM) {
		LOG_WARN_BDF("dump_regs_num=%u, regs_num_max=%u\n",
				dump_regs_num, (u32)SXE_ETHTOOL_DUMP_REGS_NUM);
	}
	stats_unlock(adapter);

	return;
}

static s32 sxe_identify_led_ctrl(struct sxe_adapter *adapter, bool is_blink)
{
	s32 ret;
	s32 resp;
	struct sxe_led_ctrl ctrl;
	struct sxe_driver_cmd cmd;
	struct sxe_hw *hw = &adapter->hw;

	ctrl.mode = (true == is_blink) ? SXE_IDENTIFY_LED_BLINK_ON : \
					SXE_IDENTIFY_LED_BLINK_OFF;
	ctrl.duration = 0;

	cmd.req      = &ctrl;
	cmd.req_len  = sizeof(ctrl);
	cmd.resp     = &resp;
	cmd.resp_len = sizeof(resp);
	cmd.trace_id = 0;
	cmd.opcode   = SXE_CMD_LED_CTRL;
	cmd.is_interruptible = false;
	ret = sxe_driver_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("hdc trans failed ret=%d, cmd:led ctrl\n", ret);
		ret = -EIO;
	}

	return ret;
}

int sxe_phys_id_set(struct net_device *netdev, enum ethtool_phys_id_state state)
{
	int ret = 0;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	switch (state) {
	case ETHTOOL_ID_ACTIVE:
		ret = sxe_identify_led_ctrl(adapter, true);
		if (ret) {
			LOG_ERROR_BDF("led active failed, ret=%d\n", ret);
		}
		break;

	case ETHTOOL_ID_INACTIVE:
		ret = sxe_identify_led_ctrl(adapter, false);
		if (ret) {
			LOG_ERROR_BDF("led inactive failed, ret=%d\n", ret);
		}
		break;
	default:
		LOG_ERROR_BDF("identify led dont support ON/OFF, state=%d\n", state);
		ret = -EOPNOTSUPP;
	}

	return ret;
}

static int sxe_get_module_info(struct net_device *netdev,
				   struct ethtool_modinfo *info)
{
	s32 ret;
	bool page_swap = false;
	u8 sff8472_rev, addr_mode;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	ret = sxe_sfp_eeprom_read(adapter, SXE_SFF_8472_COMPLIANCE,
					sizeof(sff8472_rev), &sff8472_rev);
	if (ret) {
		ret = -EIO;
		goto l_end;
	}

	ret = sxe_sfp_eeprom_read(adapter, SXE_SFF_8472_DIAG_MONITOR_TYPE,
					sizeof(addr_mode), &addr_mode);
	if (ret) {
		ret = -EIO;
		goto l_end;
	}

	if (addr_mode & SXE_SFF_ADDRESSING_MODE) {
		LOG_MSG_ERR(drv, "address change required to access page 0xA2, "
			"but not supported. please report the module "
			"type to the driver maintainers.\n");
		page_swap = true;
	}

	if ((sff8472_rev == SXE_SFF_8472_UNSUP) || page_swap || \
			!(addr_mode & SXE_SFF_DDM_IMPLEMENTED)) {
		info->type = ETH_MODULE_SFF_8079;
		info->eeprom_len = ETH_MODULE_SFF_8079_LEN;
	} else {
		info->type = ETH_MODULE_SFF_8472;
		info->eeprom_len = ETH_MODULE_SFF_8472_LEN;
	}

	LOG_INFO("sfp support management is %x, eeprom addr mode=%x "
			"eeprom type=%x, eeprom len=%d\n",
		sff8472_rev, addr_mode, info->type, info->eeprom_len);

l_end:
	return ret;
}

static int sxe_get_module_eeprom(struct net_device *netdev,
				struct ethtool_eeprom *eep, u8 *data)
{
	s32 ret;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	if (eep->len == 0) {
		ret = -EINVAL;
		goto l_end;
	}

	if (test_bit(SXE_IN_SFP_INIT, &adapter->state)) {
		ret = -EBUSY;
		goto l_end;
	}

	ret = sxe_sfp_eeprom_read(adapter, eep->offset, eep->len, data);
	if (ret) {
		LOG_ERROR("read sfp failed\n");
	}

l_end:
	return ret;
}

static int sxe_get_coalesce(struct net_device *netdev,
#ifdef HAVE_ETHTOOL_COALESCE_EXTACK
				struct ethtool_coalesce *ec,
				struct kernel_ethtool_coalesce *kernel_coal,
				struct netlink_ext_ack *extack)
#else
				struct ethtool_coalesce *ec)
#endif
{
	return sxe_irq_coalesce_get(netdev, ec);
}

static int sxe_set_coalesce(struct net_device *netdev,
#ifdef HAVE_ETHTOOL_COALESCE_EXTACK
				struct ethtool_coalesce *ec,
				struct kernel_ethtool_coalesce *kernel_coal,
				struct netlink_ext_ack *extack)
#else
				struct ethtool_coalesce *ec)
#endif
{
	return sxe_irq_coalesce_set(netdev, ec);
}

static const struct ethtool_ops sxe_ethtool_ops = {
#ifdef ETHTOOL_COALESCE_USECS
	.supported_coalesce_params = ETHTOOL_COALESCE_USECS,
#endif
	.get_drvinfo            = sxe_get_drvinfo,
	.nway_reset             = sxe_nway_reset,
	.get_link               = ethtool_op_get_link,
	.get_ringparam          = sxe_get_ringparam,
	.set_ringparam          = sxe_set_ringparam,
	.get_channels           = sxe_get_channels,
	.set_channels           = sxe_set_channels,
	.get_strings            = sxe_get_strings,
	.get_sset_count         = sxe_get_sset_count,
	.get_ethtool_stats      = sxe_get_ethtool_stats,
	.get_rxnfc		= sxe_get_rxnfc,
	.set_rxnfc		= sxe_set_rxnfc,
	.get_rxfh_indir_size	= sxe_rss_indir_size,
	.get_rxfh_key_size	= sxe_get_rxfh_key_size,
	.get_rxfh		= sxe_get_rxfh,
	.set_rxfh		= sxe_set_rxfh,
	.get_priv_flags		= sxe_get_priv_flags,
	.set_priv_flags		= sxe_set_priv_flags,
	.get_ts_info		= sxe_get_ts_info,
	.set_phys_id		= sxe_phys_id_set,
	.set_link_ksettings	= sxe_set_link_ksettings_proto,
	.get_link_ksettings     = sxe_get_link_ksettings_proto,
	.self_test              = sxe_diag_test, 
	.get_pauseparam         = sxe_get_pauseparam,
	.set_pauseparam         = sxe_set_pauseparam,
	.set_coalesce           = sxe_set_coalesce,
	.get_coalesce           = sxe_get_coalesce,
	.get_wol                = sxe_get_wol,
	.set_wol                = sxe_set_wol,
	.get_msglevel           = sxe_get_msglevel,
	.set_msglevel           = sxe_set_msglevel,
	.get_regs_len           = sxe_regs_len_get,
	.get_regs               = sxe_regs_get,
	.get_module_info	= sxe_get_module_info,
	.get_module_eeprom	= sxe_get_module_eeprom,
};

void sxe_ethtool_ops_set(struct net_device *netdev)
{
	netdev->ethtool_ops = &sxe_ethtool_ops;

	return;
}
