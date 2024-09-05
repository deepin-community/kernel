
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/string.h>

#include "sxe_filter.h"
#include "sxe.h"
#include "sxe_hw.h"
#include "sxe_regs.h"
#include "sxe_tx_proc.h"
#include "sxe_ethtool.h"

extern struct workqueue_struct *sxe_fnav_workqueue;
extern struct kmem_cache *fnav_cache;

#define SXE_FNAV_BKT_HASH_MASK 0x1FFF
#define SXE_FNAV_HASH_REG_MASK 0xFFFFFFFF

#define SXE_SAMPLE_WORD_BITS				 (16)

s32 sxe_uc_addr_add(struct sxe_hw *hw, struct sxe_uc_addr_table *uc_table,
				const u8 *addr, u16 pool)
{
	struct sxe_adapter *adapter = hw->adapter;
	struct sxe_uc_addr_table *entry;
	s32 ret;
	u32 i;

	if (is_zero_ether_addr(addr)) {
		ret = -EINVAL;
		LOG_ERROR_BDF("mac addr is zero.(err:%d)\n", ret);
		goto l_out;
	}

	for (i = 0; i < SXE_UC_ENTRY_NUM_MAX; i++) {
		entry = &uc_table[i];

		if (!test_and_set_bit(SXE_UC_ADDR_ENTRY_USED, &entry->state)) {
			ether_addr_copy(entry->addr, addr);
			entry->pool = pool;

			hw->filter.mac.ops->uc_addr_add(hw,
						i, entry->addr, entry->pool);
			ret = i;
			LOG_INFO("mac addr:%pM pool:%u add to "
				 "uc_table[%u] success.\n",
				 addr, pool, i);
			goto l_out;
		}
	}

	ret = -ENOMEM;
	LOG_ERROR_BDF("index:%u mac addr:%pM pool:%u add to uc filter fail.\n",
		  i, addr, pool);

l_out:
	return ret;
}

s32 sxe_uc_addr_del(struct sxe_hw *hw, struct sxe_uc_addr_table *uc_table,
				const u8 *addr, u16 pool)
{
	struct sxe_adapter *adapter = hw->adapter;
	struct sxe_uc_addr_table *entry;
	s32 ret = 0;
	u32 i;

	if (is_zero_ether_addr(addr)) {
		ret = -EINVAL;
		LOG_WARN_BDF("mac addr is zero.(err:%d)\n", ret);
		goto l_out;
	}

	for (i = 0; i < SXE_UC_ENTRY_NUM_MAX; i++) {
		entry = &uc_table[i];
		if ((entry->pool == pool) &&
		    (ether_addr_equal(addr, entry->addr))) {
			if ((test_and_clear_bit(SXE_UC_ADDR_ENTRY_USED,
						&entry->state))) {
				hw->filter.mac.ops->uc_addr_del(hw, i);
				LOG_INFO("pool:%u mac addr:%pM uc_filter_addr[%u] "
					 "entry del success.\n",
					 pool, addr, i);
				goto l_out;
			}
		}
	}

	ret = -ENOMEM;

	LOG_ERROR_BDF("index:%u mac addr:%pM pool:%u delete fail due to "
		  "not exsit in uc filter.\n",
		  i, addr, pool);

l_out:
	return ret;
}

s32 sxe_uc_sync(struct net_device *netdev, const u8 *addr)
{
	s32 ret;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_uc_addr_table *uc_table =
					adapter->mac_filter_ctxt.uc_addr_table;

	ret = sxe_uc_addr_add(hw, uc_table, addr, PF_POOL_INDEX(0));

	return min_t(s32, ret, 0);
}

s32 sxe_uc_unsync(struct net_device *netdev, const u8 *addr)
{
	s32 ret;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;

	ret = sxe_uc_addr_del(hw, adapter->mac_filter_ctxt.uc_addr_table,
				addr, PF_POOL_INDEX(0));
	if (ret) {
		LOG_ERROR_BDF("pool idx:%d addr:%pM del fail.\n", PF_POOL_INDEX(0), addr);
	}

	return 0;
}

static void sxe_mc_hash_table_add(struct sxe_adapter *adapter, u8 *mc_addr)
{
	u16 extracted;
	u16 bit_index;
	u16 reg_index;

	adapter->mac_filter_ctxt.mc_hash_table_used++;

	extracted = ((mc_addr[4] >> 4) | (((u16)mc_addr[5]) << 4));

	extracted &= SXE_MC_ADDR_EXTRACT_MASK;
	LOG_DEV_DEBUG(" bit-vector = 0x%03X\n", extracted);

	reg_index = (extracted >> SXE_MC_ADDR_SHIFT) & SXE_MC_ADDR_REG_MASK;

	bit_index = extracted & SXE_MC_ADDR_BIT_MASK;

	adapter->mac_filter_ctxt.mc_hash_table[reg_index] |= BIT(bit_index);

	LOG_INFO("mc_addr:%pM extracted:0x%x reg_index:%u bit_index:%u "
		 "add to mc_hash_table success.\n",
		  mc_addr, extracted, reg_index, bit_index);

	return;
}

#ifdef CONFIG_PCI_IOV
void sxe_vf_mc_addr_restore(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_virtual_context *vt_ctxt = &adapter->vt_ctxt;
	u8 i;
	u8 j;
	u8 reg_idx;
	u8 bit_idx;
	u32 filter_ctl;

	for (i = 0; i < vt_ctxt->num_vfs; i++) {
		struct sxe_vf_info *vf_info = &vt_ctxt->vf_info[i];
		for (j = 0; j < vf_info->mc_hash_used; j++) {
			reg_idx = (vf_info->mc_hash[j] >> SXE_MC_ADDR_SHIFT) &
				  SXE_MC_ADDR_REG_MASK;
			bit_idx = vf_info->mc_hash[j] & SXE_MC_ADDR_BIT_MASK;
			hw->filter.mac.ops->mta_hash_table_update(hw, reg_idx, bit_idx);
			LOG_INFO_BDF("vf_idx:%u mc_cnt:%u mc_hash[%d]:0x%x"
				"reg_idx=%u, bit_idx=%u.\n",
				i, vf_info->mc_hash_used, j, vf_info->mc_hash[j],
				reg_idx, bit_idx);
		}

		filter_ctl = hw->filter.mac.ops->pool_rx_mode_get(hw, i);
		if (vf_info->mc_hash_used) {
			filter_ctl |= SXE_VMOLR_ROMPE;

		} else {
			filter_ctl &= ~SXE_VMOLR_ROMPE;
		}
		hw->filter.mac.ops->pool_rx_mode_set(hw, filter_ctl, i);
	}

	return;
}
#else
void sxe_vf_mc_addr_restore(struct sxe_adapter *adapter)
{
	return;
}
#endif

s32 sxe_mc_addr_add(struct net_device *netdev)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;
	struct netdev_hw_addr *hw_addr;
	struct sxe_mac_filter_context *mac_filter = &adapter->mac_filter_ctxt;
	u8 i;

	 if (!netif_running(netdev)) {
		return 0;
	 }

	LOG_DEV_DEBUG("clearing MTA.\n");
	mac_filter->mc_hash_table_used = 0;
	memset(mac_filter->mc_hash_table, 0,
		sizeof(mac_filter->mc_hash_table));

	netdev_for_each_mc_addr(hw_addr, netdev) {
		LOG_DEV_DEBUG("adding the multicast addresses:\n");
		sxe_mc_hash_table_add(adapter, hw_addr->addr);
	}

	for (i = 0; i < SXE_MTA_ENTRY_NUM_MAX; i++) {
		hw->filter.mac.ops->mta_hash_table_set(hw, i, mac_filter->mc_hash_table[i]);
	}

	if (mac_filter->mc_hash_table_used) {
		hw->filter.mac.ops->mc_filter_enable(hw);
	}

	sxe_vf_mc_addr_restore(adapter);

	LOG_DEV_DEBUG("sxe_mc_addr_add complete.\n");

	return netdev_mc_count(netdev);
}

void sxe_fc_mac_addr_set(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	u8 mac_addr[ETH_ALEN];

	memcpy(mac_addr, adapter->mac_filter_ctxt.cur_mac_addr, ETH_ALEN);
	hw->filter.mac.ops->fc_mac_addr_set(hw, mac_addr);

	return;
}

void sxe_mac_addr_set(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_uc_addr_table *entry =
		&adapter->mac_filter_ctxt.uc_addr_table[SXE_DEFAULT_UC_ADDR_IDX];

	memcpy(&entry->addr, adapter->mac_filter_ctxt.cur_mac_addr, ETH_ALEN);
	entry->pool = PF_POOL_INDEX(0);

	set_bit(SXE_UC_ADDR_ENTRY_USED, &entry->state);

	hw->filter.mac.ops->uc_addr_add(hw, SXE_DEFAULT_UC_ADDR_IDX,
					entry->addr, entry->pool);

	sxe_fc_mac_addr_set(adapter);

	return;
}

static s32 sxe_uc_filter_init(struct sxe_adapter *adapter)
{
	s32 ret = 0;
	struct sxe_mac_filter_context *mac_filter = &adapter->mac_filter_ctxt;

	mac_filter->uc_addr_table = kcalloc(SXE_UC_ENTRY_NUM_MAX,
			 sizeof(struct sxe_uc_addr_table), GFP_KERNEL);
	if (!mac_filter->uc_addr_table) {
		ret = -ENOMEM;
		LOG_ERROR_BDF("rar entry:%d size:%lu mac table kcalloc fail.(err:%d)",
			   SXE_UC_ENTRY_NUM_MAX,
			   sizeof(struct sxe_uc_addr_table), ret);
	}

	return ret;
}

s32 sxe_mac_filter_init(struct sxe_adapter *adapter)
{
	s32 ret;

	ret = sxe_uc_filter_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("uc filter init failed\n");
		goto l_ret;
	}

l_ret:
	return ret;
}

void sxe_mac_filter_reset(struct sxe_adapter *adapter)
{
	struct sxe_uc_addr_table *entry;
	u32 i;

	for (i = 0; i < SXE_UC_ENTRY_NUM_MAX; i++) {
		entry = &adapter->mac_filter_ctxt.uc_addr_table[i];

		clear_bit(SXE_UC_ADDR_ENTRY_USED, &entry->state);
	}

	adapter->mac_filter_ctxt.mc_hash_table_used = 0;
	return;
}

void sxe_mac_filter_destroy(struct sxe_adapter *adapter)
{
	if (adapter->mac_filter_ctxt.uc_addr_table) {
		kfree(adapter->mac_filter_ctxt.uc_addr_table);
	}

	return;
}

void sxe_fnav_rules_restore(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	struct hlist_node *node;
	struct sxe_fnav_rule_node *rule;
	u64 ring_cookie;
	u8 queue;

	spin_lock(&adapter->fnav_ctxt.specific_lock);

	if (!hlist_empty(&adapter->fnav_ctxt.rules_list)) {
		hw->dbu.ops->fnav_specific_rule_mask_set(hw,
				&adapter->fnav_ctxt.rules_mask);

		hlist_for_each_entry_safe(rule, node,
			&adapter->fnav_ctxt.rules_list, node) {
			ring_cookie = rule->ring_cookie;

			sxe_fnav_dest_queue_parse(adapter,
							ring_cookie, &queue);

			hw->dbu.ops->fnav_specific_rule_add(hw,
					&rule->rule_info,
					rule->sw_idx,
					queue);
		}
	}

	spin_unlock(&adapter->fnav_ctxt.specific_lock);

	return;
}

void sxe_fnav_rules_clean(struct sxe_adapter *adapter)
{
	struct hlist_node *container_node;
	struct sxe_fnav_rule_node *rule;

	spin_lock(&adapter->fnav_ctxt.specific_lock);

	hlist_for_each_entry_safe(rule, container_node,
				  &adapter->fnav_ctxt.rules_list,
				  node) {
		hlist_del(&rule->node);
		kfree(rule);
	}
	adapter->fnav_ctxt.rule_cnt = 0;

	spin_unlock(&adapter->fnav_ctxt.specific_lock);

	return ;
}

static bool sxe_fnav_is_sample_protocol_supported(__be16 protocol)
{
	return !((protocol != htons(ETH_P_IP)) &&
		(protocol != htons(ETH_P_IPV6)));
}

static s32 sxe_fnav_sample_header_len_check(struct sk_buff *skb,
					union sxe_sample_data_hdr *hdr)
{
	s32 ret = 0;
	if (unlikely(hdr->network <= skb->data)) {
		ret = -SXE_ERR_PARAM;
		LOG_DEBUG("hdr.network <= skb->data\n");
		goto l_end;
	}

	if (unlikely(skb_tail_pointer(skb) < hdr->network + 40)) {
		ret = -SXE_ERR_PARAM;
		LOG_DEBUG("skb_tail_pointer(skb) < hdr->network + 40\n");
		goto l_end;
	}

l_end:
	return ret;
}

static s32 sxe_fnav_sample_tcp_ip_header_check(
						union sxe_sample_data_hdr *hdr,
						struct sk_buff *skb,
						unsigned int *hlen)
{
	int l4_proto;
	s32 ret = 0;

	switch (hdr->ipv4->version) {
	case SXE_IPV4:
		*hlen = (hdr->network[0] & 0x0F) << 2;
		l4_proto = hdr->ipv4->protocol;
		break;
	case SXE_IPV6:
		*hlen = hdr->network - skb->data;
		l4_proto = ipv6_find_hdr(skb, hlen, IPPROTO_TCP, NULL, NULL);
		*hlen -= hdr->network - skb->data;
		break;
	default:
		ret = -SXE_ERR_PARAM;
		LOG_ERROR("unsupported l3 protocol:%d\n", hdr->ipv4->version);
		goto l_end;;
	}

	if (l4_proto != IPPROTO_TCP) {
		ret = -SXE_ERR_PARAM;
		LOG_INFO("unsupported l4 protocol:%d\n", l4_proto);
		goto l_end;
	}

	if (unlikely(skb_tail_pointer(skb) < hdr->network +
		     *hlen + sizeof(struct tcphdr))) {
		ret = -SXE_ERR_PARAM;
		LOG_ERROR("error on length skb_tail_pointer=0x%p < "
			"(hdr->network + *hlen + sizeof(struct tcphdr))=0x%p\n",
			skb_tail_pointer(skb),
			(hdr->network + *hlen + sizeof(struct tcphdr)));
		goto l_end;
	}

l_end:
	return ret;
}

static void sxe_sample_hash_iter_compute(u8 bit_n,
					u32 *common_hash, u32 *bucket_hash,
					u32 *sig_hash, u32 lo_hash_dword,
					u32 hi_hash_dword)
{
	u32 n = bit_n;

	if (SXE_SAMPLE_COMMON_HASH_KEY & BIT(n)) {
		*common_hash ^= lo_hash_dword >> n;
	} else if (SXE_FNAV_BUCKET_HASH_KEY & BIT(n)) {
		*bucket_hash ^= lo_hash_dword >> n;
	} else if (SXE_FNAV_SAMPLE_HASH_KEY & BIT(n)) {
		*sig_hash ^= lo_hash_dword << (SXE_SAMPLE_WORD_BITS - n);
	}

	if (SXE_SAMPLE_COMMON_HASH_KEY & BIT(n + SXE_SAMPLE_WORD_BITS)) {
		*common_hash ^= hi_hash_dword >> n;
	} else if (SXE_FNAV_BUCKET_HASH_KEY & BIT(n + SXE_SAMPLE_WORD_BITS)) {
		*bucket_hash ^= hi_hash_dword >> n;
	} else if (SXE_FNAV_SAMPLE_HASH_KEY & BIT(n + SXE_SAMPLE_WORD_BITS)) {
		*sig_hash ^= hi_hash_dword << (SXE_SAMPLE_WORD_BITS - n);
	}

	return;
}

static u32 sxe_sample_hash_compute(union sxe_sample_hash_dword input,
					    union sxe_sample_hash_dword common)
{
	u32 hi_hash_dword, lo_hash_dword, flow_vm_vlan;
	u32 sig_hash = 0, bucket_hash = 0, common_hash = 0;
	u8 i;

	flow_vm_vlan = ntohl(input.dword);

	hi_hash_dword = ntohl(common.dword);

	lo_hash_dword = (hi_hash_dword >> SXE_SAMPLE_WORD_BITS) | (hi_hash_dword << SXE_SAMPLE_WORD_BITS);

	hi_hash_dword ^= flow_vm_vlan ^ (flow_vm_vlan >> SXE_SAMPLE_WORD_BITS);

	sxe_sample_hash_iter_compute(0, &common_hash, &bucket_hash, &sig_hash,
					lo_hash_dword, hi_hash_dword);

	lo_hash_dword ^= flow_vm_vlan ^ (flow_vm_vlan << SXE_SAMPLE_WORD_BITS);

	for (i = 1; i < SXE_SAMPLE_WORD_BITS; i++) {
		sxe_sample_hash_iter_compute(i, &common_hash,
					&bucket_hash, &sig_hash,
					lo_hash_dword, hi_hash_dword);

	}

	bucket_hash ^= common_hash;
	bucket_hash &= SXE_SAMPLE_HASH_MASK;

	sig_hash ^= common_hash << SXE_SAMPLE_WORD_BITS;
	sig_hash &= SXE_SAMPLE_HASH_MASK << SXE_SAMPLE_WORD_BITS;

	return sig_hash ^ bucket_hash;
}

static void sxe_fnav_sample_rule_add(struct sxe_adapter *adapter, 
						u64 hash_cmd)
{
	struct sxe_hw * hw = &adapter->hw;
	struct sxe_fnav_sample_filter *input, *filter;
	u32 key;

	input = kzalloc(sizeof(*input), GFP_ATOMIC);
	if (!input) {
		LOG_ERROR_BDF("fnav sample rule add failed, no memory\n");
		hw->dbu.ops->fnav_single_sample_rule_del(hw,
			(u32)(hash_cmd & SXE_FNAV_HASH_REG_MASK));
		goto l_end;
	}

	key = (hash_cmd & SXE_FNAV_BKT_HASH_MASK);
	input->hash = (u32)(hash_cmd & SXE_FNAV_HASH_REG_MASK);
	spin_lock(&adapter->fnav_ctxt.sample_lock);
	hash_for_each_possible(adapter->fnav_ctxt.sample_list, filter, hlist, key) {
		if (filter->hash == input->hash) {
			kfree(input);
			goto l_unlock;
		}
	}


	hash_add(adapter->fnav_ctxt.sample_list, &input->hlist, key);
	adapter->fnav_ctxt.sample_rules_cnt++;

l_unlock:
	spin_unlock(&adapter->fnav_ctxt.sample_lock);
l_end:
	return;
}

static void sxe_fnav_sample_rule_add_task(struct work_struct *work)
{
	struct sxe_fnav_sample_work_info *sample_work = container_of(work,
				struct sxe_fnav_sample_work_info,
				work_st);
	if (!sample_work) {
		goto l_end;
	}

	sxe_fnav_sample_rule_add(sample_work->adapter, sample_work->hash);
	kmem_cache_free(fnav_cache, sample_work);

l_end:
	return;
}

s32 sxe_fnav_sample_rule_get(struct sxe_ring *ring,
		      struct sxe_tx_buffer *tx_buffer)
{
	struct sxe_irq_data *irq_data = ring->irq_data;
	struct sxe_hw *hw = &irq_data->adapter->hw;
	struct sxe_adapter *adapter = hw->adapter;
	union sxe_sample_hash_dword input  = { .dword = 0 };
	union sxe_sample_hash_dword common = { .dword = 0 };
	union sxe_sample_data_hdr hdr;
	struct tcphdr *th;
	unsigned int hlen;
	struct sk_buff *skb;
	__be16 vlan_id;
	bool is_supported;
	s32 ret;
	u32 hash_value;
	u64 hash_cmd;
	struct sxe_fnav_sample_work_info *add_work = NULL;

	LOG_DEBUG_BDF("in sample mode, sample_rate=%u, fnav_sample_count=%u\n",
		 ring->fnav_sample_rate, ring->fnav_sample_count);
	if (!irq_data || !ring->fnav_sample_rate) {
		goto l_end;
	}

	ring->fnav_sample_count++;

	is_supported = sxe_fnav_is_sample_protocol_supported(tx_buffer->protocol);
	if (!is_supported) {
		LOG_DEBUG_BDF("sample protocol=[%d] unsupported\n",tx_buffer->protocol);
		goto l_end;
	}

	skb = tx_buffer->skb;
	hdr.network = skb_network_header(skb);

	ret = sxe_fnav_sample_header_len_check(skb, &hdr);
	if (ret) {
		LOG_ERROR_BDF("sample header len check failed. ret=%d\n", ret);
		goto l_end;
	}

	ret = sxe_fnav_sample_tcp_ip_header_check(&hdr, skb, &hlen);
	if (ret) {
		LOG_INFO("sample tcp ip process err. ret=%d\n", ret);
		goto l_end;
	}

	th = (struct tcphdr *)(hdr.network + hlen);

	LOG_DEBUG_BDF("tcp is fin ? :%s, is syn ? :%s\n" ,
			th->fin ? "yes" : "no", th->syn ? "yes" : "no");

	if ((th->fin) ||
		(!th->syn && (ring->fnav_sample_count < ring->fnav_sample_rate))) {
		goto l_end;
	}

	ring->fnav_sample_count = 0;

	vlan_id = htons(tx_buffer->tx_features >> SXE_TX_FEATURE_VLAN_SHIFT);

	input.formatted.vlan_id = vlan_id;

	if (tx_buffer->tx_features & (SXE_TX_FEATURE_SW_VLAN | SXE_TX_FEATURE_HW_VLAN)) {
		common.port.src ^= th->dest ^ htons(ETH_P_8021Q);
	} else {
		common.port.src ^= th->dest ^ tx_buffer->protocol;
	}

	common.port.dst ^= th->source;

	switch (hdr.ipv4->version) {
	case SXE_IPV4:
		input.formatted.flow_type = SXE_SAMPLE_FLOW_TYPE_TCPV4;
		common.ip ^= hdr.ipv4->saddr ^ hdr.ipv4->daddr;
		break;
	case SXE_IPV6:
		input.formatted.flow_type = SXE_SAMPLE_FLOW_TYPE_TCPV6;
		common.ip ^= hdr.ipv6->saddr.s6_addr32[0] ^
			     hdr.ipv6->saddr.s6_addr32[1] ^
			     hdr.ipv6->saddr.s6_addr32[2] ^
			     hdr.ipv6->saddr.s6_addr32[3] ^
			     hdr.ipv6->daddr.s6_addr32[0] ^
			     hdr.ipv6->daddr.s6_addr32[1] ^
			     hdr.ipv6->daddr.s6_addr32[2] ^
			     hdr.ipv6->daddr.s6_addr32[3];
		break;
	default:
		break;
	}

	LOG_DEBUG_BDF("fnav sample success, start write hw\n");

	hash_value = sxe_sample_hash_compute(input, common);
	hw->dbu.ops->fnav_sample_hash_cmd_get(hw, input.formatted.flow_type,
						hash_value, ring->idx, &hash_cmd);

	if (!adapter->fnav_ctxt.is_sample_table_overflowed && 
		!workqueue_congested(WORK_CPU_UNBOUND, sxe_fnav_workqueue)) {
		add_work = kmem_cache_zalloc(fnav_cache, GFP_ATOMIC);
		if (!add_work) {
			return -ENOMEM;
		}

		INIT_WORK(&add_work->work_st, sxe_fnav_sample_rule_add_task);
		add_work->adapter = adapter;
		add_work->hash    = hash_cmd;
		queue_work(sxe_fnav_workqueue, &add_work->work_st);
		hw->dbu.ops->fnav_sample_hash_set(hw, hash_cmd);
	}

l_end:
	return 0;
}

static void sxe_fnav_sw_specific_rule_add(struct sxe_adapter *adapter,
				    struct sxe_fnav_rule_node *add_rule)
{
	struct sxe_hw *hw = &adapter->hw;
	struct hlist_node *next;
	struct sxe_fnav_rule_node *rule = NULL;
	struct sxe_fnav_rule_node *pre_node = NULL;
	u16 sw_idx = add_rule->sw_idx;
	s32 ret;

	hlist_for_each_entry_safe(rule, next,
				  &adapter->fnav_ctxt.rules_list,
				  node) {
		if (rule->sw_idx >= sw_idx) {
			break;
		}
		pre_node = rule;
	}
	LOG_DEBUG_BDF("add specific fnav rule in sw_idx[%u]\n",sw_idx);

	if (rule && (rule->sw_idx == sw_idx)) {
		LOG_DEBUG_BDF("rule->sw_idx == sw_idx == %u, show bkt_hash."
				"old bkt_hash[0x%x], input new bkt_hash[0x%x]\n",
				sw_idx, rule->rule_info.ntuple.bkt_hash,
				add_rule->rule_info.ntuple.bkt_hash);
		if (rule->rule_info.ntuple.bkt_hash !=
					add_rule->rule_info.ntuple.bkt_hash) {

			ret = hw->dbu.ops->fnav_specific_rule_del(hw,
								&rule->rule_info,
								sw_idx);
			if (ret) {
				LOG_ERROR_BDF("delete fnav rule in sw_idx[%d]"
					"failed\n", sw_idx);
			}
		}

		hlist_del(&rule->node);
		kfree(rule);
		adapter->fnav_ctxt.rule_cnt--;
	}

	INIT_HLIST_NODE(&add_rule->node);

	if (pre_node) {
		hlist_add_behind(&add_rule->node, &pre_node->node);
	} else {
		hlist_add_head(&add_rule->node,
			&adapter->fnav_ctxt.rules_list);
	}

	adapter->fnav_ctxt.rule_cnt++;

	return;
}

static void sxe_fnav_specific_hash_iter_compute(u8 bit_n,
					u32 *bucket_hash,  u32 lo_hash_dword,
					u32 hi_hash_dword)
{
	u32 n = bit_n;
	if (SXE_FNAV_BUCKET_HASH_KEY & BIT(n)) {
		*bucket_hash ^= lo_hash_dword >> n;
	}

	if (SXE_FNAV_BUCKET_HASH_KEY & BIT(n + 16)) {
		*bucket_hash ^= hi_hash_dword >> n;
	}

	return;
}

static void sxe_fnav_specific_hash_compute(
					union sxe_fnav_rule_info *input_rule,
					union sxe_fnav_rule_info *input_mask)
{

	u32 hi_hash_dword, lo_hash_dword, flow_vm_vlan;
	u32 bucket_hash = 0;
	__be32 hi_dword = 0;
	u8 i;

	for (i = 0; i <= 10; i++) {
		input_rule->fast_access[i] &= input_mask->fast_access[i];
	}

	flow_vm_vlan = ntohl(input_rule->fast_access[0]);

	for (i = 1; i <= 10; i++) {
		hi_dword ^= input_rule->fast_access[i];
	}

	hi_hash_dword = ntohl(hi_dword);

	lo_hash_dword = (hi_hash_dword >> 16) | (hi_hash_dword << 16);

	hi_hash_dword ^= flow_vm_vlan ^ (flow_vm_vlan >> 16);

	sxe_fnav_specific_hash_iter_compute(0, &bucket_hash,
					lo_hash_dword, hi_hash_dword);

	lo_hash_dword ^= flow_vm_vlan ^ (flow_vm_vlan << 16);

	for (i = 1; i <= 15; i++) {
		sxe_fnav_specific_hash_iter_compute(i, &bucket_hash,
					lo_hash_dword, hi_hash_dword);
	}

	input_rule->ntuple.bkt_hash = (__force __be16)(bucket_hash & 0x1FFF);
	LOG_DEBUG("fnav bkt_hash=0x%x\n", input_rule->ntuple.bkt_hash);

	return ;
}

s32 sxe_fnav_specific_rule_add_process(
					struct sxe_adapter *adapter,
					struct sxe_fnav_rule_node *input_rule,
					union  sxe_fnav_rule_info *mask,
					u8 queue)
{
	s32 ret = 0;
	struct sxe_hw *hw = &adapter->hw;

	spin_lock(&adapter->fnav_ctxt.specific_lock);

	LOG_DEBUG_BDF("add specific fnav mask---rule_info:vm_pool[%u], flow_type[0x%x]"
		  "vlan_id[%u], dst_ip[%x:%x:%x:%x], src_ip[%x:%x:%x:%x]"
		  "dst_port[%u], src_port[%u], flex_bytes[0x%x], bkt_hash[0x%x]\n",
		mask->ntuple.vm_pool,
		mask->ntuple.flow_type,
		mask->ntuple.vlan_id,
		mask->ntuple.dst_ip[0],
		mask->ntuple.dst_ip[1],
		mask->ntuple.dst_ip[2],
		mask->ntuple.dst_ip[3],
		mask->ntuple.src_ip[0],
		mask->ntuple.src_ip[1],
		mask->ntuple.src_ip[2],
		mask->ntuple.src_ip[3],
		mask->ntuple.dst_port,
		mask->ntuple.src_port,
		mask->ntuple.flex_bytes,
		mask->ntuple.bkt_hash
		);
	if (hlist_empty(&adapter->fnav_ctxt.rules_list)) {
		LOG_DEBUG_BDF("new fnav mask added\n");
		memcpy(&adapter->fnav_ctxt.rules_mask, mask, sizeof(*mask));

		ret = hw->dbu.ops->fnav_specific_rule_mask_set(hw, mask);
		if (ret) {
			LOG_MSG_ERR(drv, "error writing mask\n");
			goto l_err_unlock;
		}
	} else if (memcmp(&adapter->fnav_ctxt.rules_mask, mask, sizeof(*mask))) {
		LOG_MSG_ERR(drv, "only one mask supported per port\n");
		goto l_err_unlock;
	}

	sxe_fnav_specific_hash_compute(&input_rule->rule_info, mask);

	LOG_DEBUG_BDF("add specific fnav rule---filter:vm_pool[%u], flow_type[0x%x]"
		  "vlan_id[%u], dst_ip[%x:%x:%x:%x], src_ip[%x:%x:%x:%x]"
		  "dst_port[%u], src_port[%u], flex_bytes[0x%x], bkt_hash[0x%x]"
		  "sw_idx[%u], ring_cookie[0x%llx]\n",
		input_rule->rule_info.ntuple.vm_pool,
		input_rule->rule_info.ntuple.flow_type,
		input_rule->rule_info.ntuple.vlan_id,
		input_rule->rule_info.ntuple.dst_ip[0],
		input_rule->rule_info.ntuple.dst_ip[1],
		input_rule->rule_info.ntuple.dst_ip[2],
		input_rule->rule_info.ntuple.dst_ip[3],
		input_rule->rule_info.ntuple.src_ip[0],
		input_rule->rule_info.ntuple.src_ip[1],
		input_rule->rule_info.ntuple.src_ip[2],
		input_rule->rule_info.ntuple.src_ip[3],
		input_rule->rule_info.ntuple.dst_port,
		input_rule->rule_info.ntuple.src_port,
		input_rule->rule_info.ntuple.flex_bytes,
		input_rule->rule_info.ntuple.bkt_hash,
		input_rule->sw_idx,
		input_rule->ring_cookie
		);
	ret = hw->dbu.ops->fnav_specific_rule_add(hw,
				&input_rule->rule_info, input_rule->sw_idx, queue);
	if (ret) {
		LOG_ERROR_BDF("set specific rule failed, ret = %d\n", ret);
		goto l_err_unlock;
	}

	sxe_fnav_sw_specific_rule_add(adapter, input_rule);

	spin_unlock(&adapter->fnav_ctxt.specific_lock);
	return 0;

l_err_unlock:
	ret = -EINVAL;
	spin_unlock(&adapter->fnav_ctxt.specific_lock);
	return ret;
}

int sxe_fnav_sw_specific_rule_del(
					struct sxe_adapter *adapter,
					u16 sw_idx)
{
	struct sxe_hw *hw = &adapter->hw;
	struct hlist_node *next;
	struct sxe_fnav_rule_node *rule = NULL;
	int ret = -EINVAL;

	hlist_for_each_entry_safe(rule, next,
				  &adapter->fnav_ctxt.rules_list,
				  node) {
		if (rule->sw_idx >= sw_idx) {
			LOG_INFO("rule->sw_idx = %u; sw_idx = %u\n",
				rule->sw_idx, sw_idx);
			break;
		}
	}

	if (rule && (rule->sw_idx == sw_idx)) {
		LOG_DEBUG_BDF("delete rule in sw_idx[%u]\n", sw_idx);
		ret = hw->dbu.ops->fnav_specific_rule_del(hw,
							&rule->rule_info,
							sw_idx);
		if (ret) {
			LOG_ERROR_BDF("delete fnav rule in sw_idx[%d]"
				"failed\n", sw_idx);
		}

		hlist_del(&rule->node);
		kfree(rule);
		adapter->fnav_ctxt.rule_cnt--;
	} else {
		LOG_ERROR_BDF("fnav rule in sw_idx[%u] not found\n", sw_idx);
	}

	return ret;
}

u64 sxe_fnav_max_rule_num_get(u32 rules_table_size)
{
	return (u64)((SXE_FNAV_RULES_TABLE_SIZE_UNIT <<
			rules_table_size) - 2);
}

struct sxe_fnav_rule_node* sxe_fnav_specific_rule_find(
						struct sxe_adapter *adapter,
						u32 location)
{
	struct sxe_fnav_rule_node *rule = NULL;
	struct hlist_node *next;

	hlist_for_each_entry_safe(rule, next,
				  &adapter->fnav_ctxt.rules_list,
				  node) {
		if (location <= rule->sw_idx) {
			LOG_INFO("location = %u, sw_idx = %u\n",
				location, rule->sw_idx);
			break;
		}
	}

	if (!rule || location != rule->sw_idx) {
		rule = NULL;
	}

	LOG_INFO("loc[%u] rule find finish and %s\n", location,
			rule != NULL ? "found" : "not found");

	return rule;
}
