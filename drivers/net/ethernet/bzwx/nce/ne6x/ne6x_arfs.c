// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include <linux/cpu_rmap.h>

#include "ne6x.h"
#include "ne6x_reg.h"
#include "ne6x_portmap.h"
#include "ne6x_dev.h"
#include "ne6x_txrx.h"
#include "ne6x_arfs.h"

#ifdef CONFIG_RFS_ACCEL

static void
ne6x_arfs_update_active_fltr_cntrs(struct ne6x_adapter *adpt,
				   struct ne6x_arfs_entry *entry, bool add);

int ne6x_dev_add_fster_rules(struct ne6x_adapter *adpt, struct ne6x_fster_fltr *input, bool is_tun)
{
	u32 table_id = 0xffffffff;
	struct ne6x_fster_table fster;
	struct ne6x_fster_search_result result;
	u32 *fster_data = (u32 *)&fster;
	int ret = 0, index;
	struct device *dev;

	dev = ne6x_pf_to_dev(adpt->back);
	dev_info(dev, "add: vport: %d %x %x %x %x %d %d rxq_id：%d\n", adpt->vport,
		 input->ip.v4.dst_ip, input->ip.v4.src_ip, input->ip.v4.dst_port,
		 input->ip.v4.src_port, input->ip.v4.pi, input->ip.v4.proto, input->q_index);

	memset(&fster, 0x00, sizeof(struct ne6x_fster_table));
	/* hash key */
	memcpy(&fster.ip, &input->ip, sizeof(fster.ip));
	/* hash data */
	memcpy(&fster.data, &input->data, sizeof(fster.data));

	/* flow steer info */
	for (index = 0; index < 24; index++)
		fster_data[index] = cpu_to_be32(fster_data[index]);

	ret = ne6x_reg_table_search(adpt->back, NE6X_REG_ARFS_TABLE, (u32 *)fster_data,
				    sizeof(fster.ip), (u32 *)&result, 32);

	if (ret == -ENOENT) {
		ret = ne6x_reg_table_insert(adpt->back, NE6X_REG_ARFS_TABLE, (u32 *)fster_data,
					    sizeof(fster), &table_id);
		if (ret)
			dev_err(ne6x_pf_to_dev(adpt->back), "insert flow steer table fail %02x\n",
				ADPT_LPORT(adpt));
	} else {
		ret = ne6x_reg_table_update(adpt->back, NE6X_REG_ARFS_TABLE, result.key_index + 8,
					    (u32 *)&fster.data, sizeof(fster.data));
		if (ret)
			dev_err(ne6x_pf_to_dev(adpt->back), "update flow steer table fail ret:%d\n",
				ret);
	}

	return 0;
}

int ne6x_dev_del_fster_rules(struct ne6x_adapter *adpt, struct ne6x_fster_fltr *input, bool is_tun)
{
	struct ne6x_fster_table fster;
	struct ne6x_fster_search_result result;
	u32 *fster_data = (u32 *)&fster;
	int ret = 0, index;
	struct device *dev;

	dev = ne6x_pf_to_dev(adpt->back);
	dev_info(dev, "del: vport: %d %x %x %x %x %d %d rxq_id：%d\n",
		 adpt->vport, input->ip.v4.dst_ip, input->ip.v4.src_ip, input->ip.v4.dst_port,
		 input->ip.v4.src_port, input->ip.v4.pi, input->ip.v4.proto, input->q_index);

	memset(&fster, 0x00, sizeof(struct ne6x_fster_table));
	/* hash key */
	memcpy(&fster.ip, &input->ip, sizeof(fster.ip));

	/* flow steer info */
	for (index = 0; index < 16; index++)
		fster_data[index] = cpu_to_be32(fster_data[index]);

	ret = ne6x_reg_table_search(adpt->back, NE6X_REG_ARFS_TABLE, (u32 *)fster_data,
				    sizeof(fster.ip), (u32 *)&result, 32);
	if (!ret) {
		ret = ne6x_reg_table_delete(adpt->back, NE6X_REG_ARFS_TABLE,
					    (u32 *)&fster.ip, sizeof(fster.ip));
		if (ret)
			dev_err(ne6x_pf_to_dev(adpt->back), "delete flow steer table fail ret:%d\n",
				ret);
	} else {
		dev_err(ne6x_pf_to_dev(adpt->back), "search flow steer table fail ret:%d\n", ret);
	}
	return 0;
}

static bool ne6x_is_arfs_active(struct ne6x_adapter *adpt)
{
	return !!adpt->arfs_fltr_list;
}

static bool
ne6x_arfs_is_flow_expired(struct ne6x_adapter *adpt, struct ne6x_arfs_entry *arfs_entry)
{
#define NE6X_ARFS_TIME_DELTA_EXPIRATION	msecs_to_jiffies(5000)
	if (rps_may_expire_flow(adpt->netdev, arfs_entry->fltr_info.q_index,
				arfs_entry->flow_id,
				arfs_entry->fltr_info.fltr_id))
		return true;

	/* expiration timer only used for UDP filters */
	if (arfs_entry->fltr_info.flow_type != NE6X_FLTR_PTYPE_NONF_IPV4_UDP &&
	    arfs_entry->fltr_info.flow_type != NE6X_FLTR_PTYPE_NONF_IPV6_UDP)
		return false;

	return time_in_range64(arfs_entry->time_activated +
			       NE6X_ARFS_TIME_DELTA_EXPIRATION,
			       arfs_entry->time_activated, get_jiffies_64());
}

static void
ne6x_arfs_update_flow_rules(struct ne6x_adapter *adpt, u16 idx,
			    struct hlist_head *add_list,
			    struct hlist_head *del_list)
{
	struct ne6x_arfs_entry *e;
	struct hlist_node *n;
	struct device *dev;

	dev = ne6x_pf_to_dev(adpt->back);

	/* go through the aRFS hlist at this idx and check for needed updates */
	hlist_for_each_entry_safe(e, n, &adpt->arfs_fltr_list[idx], list_entry) {
		/* check if filter needs to be added to HW */
		if (e->fltr_state == NE6X_ARFS_INACTIVE) {
			enum ne6x_fltr_ptype flow_type = e->fltr_info.flow_type;
			struct ne6x_arfs_entry_ptr *ep =
				devm_kzalloc(dev, sizeof(*ep), GFP_ATOMIC);

			if (!ep)
				continue;
			INIT_HLIST_NODE(&ep->list_entry);
			/* reference aRFS entry to add HW filter */
			ep->arfs_entry = e;
			hlist_add_head(&ep->list_entry, add_list);
			e->fltr_state = NE6X_ARFS_ACTIVE;
			/* expiration timer only used for UDP flows */
			if (flow_type == NE6X_FLTR_PTYPE_NONF_IPV4_UDP ||
			    flow_type == NE6X_FLTR_PTYPE_NONF_IPV6_UDP)
				e->time_activated = get_jiffies_64();
		} else if (e->fltr_state == NE6X_ARFS_ACTIVE) {
			/* check if filter needs to be removed from HW */
			if (ne6x_arfs_is_flow_expired(adpt, e)) {
				/* remove aRFS entry from hash table for delete
				 * and to prevent referencing it the next time
				 * through this hlist index
				 */
				hlist_del(&e->list_entry);
				e->fltr_state = NE6X_ARFS_TODEL;
				/* save reference to aRFS entry for delete */
				hlist_add_head(&e->list_entry, del_list);
			}
		}
	}
}

int ne6x_arfs_add_flow_rules(struct ne6x_adapter *adpt, struct hlist_head *add_list_head)
{
	struct ne6x_arfs_entry_ptr *ep;
	struct hlist_node *n;
	struct device *dev;

	dev = ne6x_pf_to_dev(adpt->back);

	hlist_for_each_entry_safe(ep, n, add_list_head, list_entry) {
		int result;

		result = ne6x_dev_add_fster_rules(adpt, &ep->arfs_entry->fltr_info, false);
		if (!result)
			ne6x_arfs_update_active_fltr_cntrs(adpt, ep->arfs_entry, true);
		else
			dev_dbg(dev, "Unable to add aRFS entry, err %d fltr_state %d fltr_id %d flow_id %d Q %d\n",
				result, ep->arfs_entry->fltr_state,
				ep->arfs_entry->fltr_info.fltr_id,
				ep->arfs_entry->flow_id,
				ep->arfs_entry->fltr_info.q_index);

		hlist_del(&ep->list_entry);
		devm_kfree(dev, ep);
	}

	return 0;
}

int ne6x_arfs_del_flow_rules(struct ne6x_adapter *adpt,  struct hlist_head *del_list_head)
{
	struct ne6x_arfs_entry *e;
	struct hlist_node *n;
	struct device *dev;

	dev = ne6x_pf_to_dev(adpt->back);

	hlist_for_each_entry_safe(e, n, del_list_head, list_entry) {
		int result;

		result = ne6x_dev_del_fster_rules(adpt, &e->fltr_info, false);
		if (!result)
			ne6x_arfs_update_active_fltr_cntrs(adpt, e, false);
		else
			dev_dbg(dev, "Unable to delete aRFS entry, err %d fltr_state %d fltr_id %d flow_id %d Q %d\n",
				result, e->fltr_state, e->fltr_info.fltr_id,
				e->flow_id, e->fltr_info.q_index);

		/* The aRFS hash table is no longer referencing this entry */
		hlist_del(&e->list_entry);
		devm_kfree(dev, e);
	}

	return 0;
}

void ne6x_sync_arfs_fltrs(struct ne6x_pf *pf)
{
	struct ne6x_adapter *pf_adpt;
	unsigned int i;
	u8 idx = 0;

	ne6x_for_each_pf(pf, idx) {
		HLIST_HEAD(tmp_del_list);
		HLIST_HEAD(tmp_add_list);

		pf_adpt = pf->adpt[idx];

		if (!pf_adpt)
			continue;

		if (unlikely(!(pf_adpt->netdev->features & NETIF_F_NTUPLE)))
			continue;

		if (!ne6x_is_arfs_active(pf_adpt))
			continue;

		spin_lock_bh(&pf_adpt->arfs_lock);
		/* Once we process aRFS for the PF ADPT get out */
		for (i = 0; i < NE6X_MAX_ARFS_LIST; i++)
			ne6x_arfs_update_flow_rules(pf_adpt, i, &tmp_add_list,
						    &tmp_del_list);
		spin_unlock_bh(&pf_adpt->arfs_lock);

		/* use list of ne6x_arfs_entry(s) for delete */
		ne6x_arfs_del_flow_rules(pf_adpt, &tmp_del_list);

		/* use list of ne6x_arfs_entry(s) for add */
		ne6x_arfs_add_flow_rules(pf_adpt, &tmp_add_list);
	}
}

static void
ne6x_arfs_update_active_fltr_cntrs(struct ne6x_adapter *adpt,
				   struct ne6x_arfs_entry *entry, bool add)
{
	struct ne6x_arfs_active_fltr_cntrs *fltr_cntrs = adpt->arfs_fltr_cntrs;

	switch (entry->fltr_info.flow_type) {
	case NE6X_FLTR_PTYPE_NONF_IPV4_TCP:
		if (add)
			atomic_inc(&fltr_cntrs->active_tcpv4_cnt);
		else
			atomic_dec(&fltr_cntrs->active_tcpv4_cnt);
		break;
	case NE6X_FLTR_PTYPE_NONF_IPV6_TCP:
		if (add)
			atomic_inc(&fltr_cntrs->active_tcpv6_cnt);
		else
			atomic_dec(&fltr_cntrs->active_tcpv6_cnt);
		break;
	case NE6X_FLTR_PTYPE_NONF_IPV4_UDP:
		if (add)
			atomic_inc(&fltr_cntrs->active_udpv4_cnt);
		else
			atomic_dec(&fltr_cntrs->active_udpv4_cnt);
		break;
	case NE6X_FLTR_PTYPE_NONF_IPV6_UDP:
		if (add)
			atomic_inc(&fltr_cntrs->active_udpv6_cnt);
		else
			atomic_dec(&fltr_cntrs->active_udpv6_cnt);
		break;
	default:
		dev_err(ne6x_pf_to_dev(adpt->back), "aRFS: Failed to update filter counters, invalid filter type %d\n",
			entry->fltr_info.flow_type);
	}
}

static bool
ne6x_arfs_cmp(struct ne6x_fster_fltr *fltr_info, const struct flow_keys *fk)
{
	bool is_v4;

	if (!fltr_info || !fk)
		return false;

	is_v4 = (fltr_info->flow_type == NE6X_FLTR_PTYPE_NONF_IPV4_UDP ||
		fltr_info->flow_type == NE6X_FLTR_PTYPE_NONF_IPV4_TCP);

	if (fk->basic.n_proto == htons(ETH_P_IP) && is_v4)
		return (fltr_info->ip.v4.proto == fk->basic.ip_proto &&
			fltr_info->ip.v4.src_port == fk->ports.src &&
			fltr_info->ip.v4.dst_port == fk->ports.dst &&
			fltr_info->ip.v4.src_ip == fk->addrs.v4addrs.src &&
			fltr_info->ip.v4.dst_ip == fk->addrs.v4addrs.dst);

	else if (fk->basic.n_proto == htons(ETH_P_IPV6) && !is_v4)
		return (fltr_info->ip.v6.proto == fk->basic.ip_proto &&
			fltr_info->ip.v6.src_port == fk->ports.src &&
			fltr_info->ip.v6.dst_port == fk->ports.dst &&
			!memcmp(&fltr_info->ip.v6.src_ip,
				&fk->addrs.v6addrs.src,
				sizeof(struct in6_addr)) &&
			!memcmp(&fltr_info->ip.v6.dst_ip,
				&fk->addrs.v6addrs.dst,
				sizeof(struct in6_addr)));

	return false;
}

static struct ne6x_arfs_entry *
ne6x_arfs_build_entry(struct ne6x_adapter *adpt, const struct flow_keys *fk,
		      u32 hash, u16 rxq_idx, u32 flow_id)
{
	struct ne6x_arfs_entry *arfs_entry;
	struct ne6x_fster_fltr *fltr_info;
	u8 ip_proto;

	arfs_entry = devm_kzalloc(ne6x_pf_to_dev(adpt->back),
				  sizeof(*arfs_entry),
				  GFP_ATOMIC | __GFP_NOWARN);
	if (!arfs_entry)
		return NULL;

	fltr_info = &arfs_entry->fltr_info;
	fltr_info->q_index = rxq_idx;
	fltr_info->dest_adpt = adpt->idx;
	ip_proto = fk->basic.ip_proto;

	if (fk->basic.n_proto == htons(ETH_P_IP)) {
		fltr_info->ip.v4.proto = ip_proto;
		fltr_info->flow_type = (ip_proto == IPPROTO_TCP) ?
			NE6X_FLTR_PTYPE_NONF_IPV4_TCP :
			NE6X_FLTR_PTYPE_NONF_IPV4_UDP;
		fltr_info->ip.v4.src_ip = fk->addrs.v4addrs.src;
		fltr_info->ip.v4.dst_ip = fk->addrs.v4addrs.dst;
		fltr_info->ip.v4.src_port = fk->ports.src;
		fltr_info->ip.v4.dst_port = fk->ports.dst;
		fltr_info->ip.v4.proto = fk->basic.ip_proto;
		fltr_info->ip.v4.pi = ADPT_LPORT(adpt);
	} else { /* ETH_P_IPV6 */
		fltr_info->ip.v6.proto = ip_proto;
		fltr_info->flow_type = (ip_proto == IPPROTO_TCP) ?
			NE6X_FLTR_PTYPE_NONF_IPV6_TCP :
			NE6X_FLTR_PTYPE_NONF_IPV6_UDP;
		memcpy(&fltr_info->ip.v6.src_ip, &fk->addrs.v6addrs.src,
		       sizeof(struct in6_addr));
		memcpy(&fltr_info->ip.v6.dst_ip, &fk->addrs.v6addrs.dst,
		       sizeof(struct in6_addr));
		fltr_info->ip.v6.src_port = fk->ports.src;
		fltr_info->ip.v6.dst_port = fk->ports.dst;
		fltr_info->ip.v6.proto = fk->basic.ip_proto;
		fltr_info->ip.v6.pi = ADPT_LPORT(adpt);
	}
	fltr_info->data.tab_id = 5;
	fltr_info->data.port = ADPT_VPORT(adpt);
	fltr_info->data.cos = cpu_to_be16(rxq_idx);
	fltr_info->data.hash = hash;

	arfs_entry->flow_id = flow_id;
	fltr_info->fltr_id =
		atomic_inc_return(adpt->arfs_last_fltr_id) % RPS_NO_FILTER;

	return arfs_entry;
}

void ne6x_free_cpu_rx_rmap(struct ne6x_adapter *adpt)
{
	struct net_device *netdev;

	if (!adpt)
		return;

	netdev = adpt->netdev;
	if (!netdev || !netdev->rx_cpu_rmap)
		return;

	free_irq_cpu_rmap(netdev->rx_cpu_rmap);
	netdev->rx_cpu_rmap = NULL;
}

int ne6x_get_irq_num(struct ne6x_pf *pf, int idx)
{
	if (!pf->msix_entries)
		return -EINVAL;

	return pf->msix_entries[idx].vector;
}

int ne6x_set_cpu_rx_rmap(struct ne6x_adapter *adpt)
{
	struct net_device *netdev;
	struct ne6x_pf *pf;
	int base_idx, i;

	pf = adpt->back;

	netdev = adpt->netdev;
	if (!pf || !netdev || !adpt->num_q_vectors)
		return -EINVAL;

	netdev_dbg(netdev, "Setup CPU RMAP: adpt type 0x%x, ifname %s, q_vectors %d\n",
		   adpt->type, netdev->name, adpt->num_q_vectors);

	netdev->rx_cpu_rmap = alloc_irq_cpu_rmap(adpt->num_q_vectors);
	if (unlikely(!netdev->rx_cpu_rmap))
		return -EINVAL;

	base_idx = adpt->base_vector;
	for (i = 0; i < adpt->num_q_vectors; i++) {
		if (irq_cpu_rmap_add(netdev->rx_cpu_rmap, ne6x_get_irq_num(pf, base_idx + i))) {
			ne6x_free_cpu_rx_rmap(adpt);
			return -EINVAL;
		}
	}

	return 0;
}

int ne6x_rx_flow_steer(struct net_device *netdev, const struct sk_buff *skb,
		       u16 rxq_idx, u32 flow_id)
{
	struct ne6x_netdev_priv *np = netdev_priv(netdev);
	struct ne6x_arfs_entry *arfs_entry;
	struct ne6x_adapter *adpt = np->adpt;
	struct flow_keys fk;
	struct ne6x_pf *pf;
	__be16 n_proto;
	u8 ip_proto;
	u16 idx;
	u32 hash;
	int ret;

	if (unlikely(!(netdev->features & NETIF_F_NTUPLE)))
		return -ENODEV;

	/* failed to allocate memory for aRFS so don't crash */
	if (unlikely(!adpt->arfs_fltr_list))
		return -ENODEV;

	pf = adpt->back;

	/* aRFS only supported on Rx queues belonging to PF ADPT */
	if (rxq_idx >= adpt->num_queue)
		return -EOPNOTSUPP;

	if (skb->encapsulation)
		return -EPROTONOSUPPORT;

	if (!skb_flow_dissect_flow_keys(skb, &fk, 0))
		return -EPROTONOSUPPORT;

	n_proto = fk.basic.n_proto;
	/* Support only IPV4 and IPV6 */
	if ((n_proto == htons(ETH_P_IP) && !ip_is_fragment(ip_hdr(skb))) ||
	    n_proto == htons(ETH_P_IPV6))
		ip_proto = fk.basic.ip_proto;
	else
		return -EPROTONOSUPPORT;

	/* Support only TCP and UDP */
	if (ip_proto != IPPROTO_TCP && ip_proto != IPPROTO_UDP)
		return -EPROTONOSUPPORT;

	/* choose the aRFS list bucket based on skb hash */
	hash = skb_get_hash_raw(skb);
	idx = skb_get_hash_raw(skb) & NE6X_ARFS_LST_MASK;
	/* search for entry in the bucket */
	spin_lock_bh(&adpt->arfs_lock);
	hlist_for_each_entry(arfs_entry, &adpt->arfs_fltr_list[idx],
			     list_entry) {
		struct ne6x_fster_fltr *fltr_info = &arfs_entry->fltr_info;

		/* keep searching for the already existing arfs_entry flow */
		if (!ne6x_arfs_cmp(fltr_info, &fk))
			continue;

		ret = fltr_info->fltr_id;

		if (fltr_info->q_index == rxq_idx ||
		    arfs_entry->fltr_state != NE6X_ARFS_ACTIVE)
			goto out;

		/* update the queue to forward to on an already existing flow */
		fltr_info->q_index = rxq_idx;
		fltr_info->data.cos = cpu_to_be16(rxq_idx);
		arfs_entry->fltr_state = NE6X_ARFS_INACTIVE;
		ne6x_arfs_update_active_fltr_cntrs(adpt, arfs_entry, false);
		goto out_schedule_service_task;
	}

	arfs_entry = ne6x_arfs_build_entry(adpt, &fk, hash, rxq_idx, flow_id);
	if (!arfs_entry) {
		ret = -ENOMEM;
		goto out;
	}

	ret = arfs_entry->fltr_info.fltr_id;
	INIT_HLIST_NODE(&arfs_entry->list_entry);
	hlist_add_head(&arfs_entry->list_entry, &adpt->arfs_fltr_list[idx]);
out_schedule_service_task:
	ne6x_service_event_schedule(pf);
out:
	spin_unlock_bh(&adpt->arfs_lock);
	return ret;
}

static int ne6x_init_arfs_cntrs(struct ne6x_adapter *adpt)
{
	if (!adpt)
		return -EINVAL;

	adpt->arfs_fltr_cntrs = kzalloc(sizeof(*adpt->arfs_fltr_cntrs),
					GFP_KERNEL);
	if (!adpt->arfs_fltr_cntrs)
		return -ENOMEM;

	adpt->arfs_last_fltr_id = kzalloc(sizeof(*adpt->arfs_last_fltr_id),
					  GFP_KERNEL);
	if (!adpt->arfs_last_fltr_id) {
		kfree(adpt->arfs_fltr_cntrs);
		adpt->arfs_fltr_cntrs = NULL;
		return -ENOMEM;
	}

	return 0;
}

void ne6x_init_arfs(struct ne6x_adapter *adpt)
{
	struct hlist_head *arfs_fltr_list;
	unsigned int i;

	if (!adpt)
		return;

	arfs_fltr_list = kcalloc(NE6X_MAX_ARFS_LIST, sizeof(*arfs_fltr_list),
				 GFP_KERNEL);
	if (!arfs_fltr_list)
		return;

	if (ne6x_init_arfs_cntrs(adpt))
		goto free_arfs_fltr_list;

	for (i = 0; i < NE6X_MAX_ARFS_LIST; i++)
		INIT_HLIST_HEAD(&arfs_fltr_list[i]);

	spin_lock_init(&adpt->arfs_lock);

	adpt->arfs_fltr_list = arfs_fltr_list;

	return;

free_arfs_fltr_list:
	kfree(arfs_fltr_list);
}

void ne6x_clear_arfs(struct ne6x_adapter *adpt)
{
	struct device *dev;
	unsigned int i;
	struct ne6x_arfs_entry *r;
	struct hlist_node *n;
	HLIST_HEAD(tmp_del_list);

	if (!adpt || !adpt->back || !adpt->arfs_fltr_list)
		return;

	dev = ne6x_pf_to_dev(adpt->back);

	for (i = 0; i < NE6X_MAX_ARFS_LIST; i++) {
		spin_lock_bh(&adpt->arfs_lock);
		hlist_for_each_entry_safe(r, n, &adpt->arfs_fltr_list[i],
					  list_entry) {
			if (r->fltr_state == NE6X_ARFS_ACTIVE || r->fltr_state == NE6X_ARFS_TODEL) {
				hlist_del(&r->list_entry);
				hlist_add_head(&r->list_entry, &tmp_del_list);
			}
		}
		spin_unlock_bh(&adpt->arfs_lock);
	}

	hlist_for_each_entry_safe(r, n, &tmp_del_list, list_entry) {
		ne6x_dev_del_fster_rules(adpt, &r->fltr_info, false);
			hlist_del(&r->list_entry);
			devm_kfree(dev, r);
	}

	for (i = 0; i < NE6X_MAX_ARFS_LIST; i++) {
		struct ne6x_arfs_entry *r;
		struct hlist_node *n;

		spin_lock_bh(&adpt->arfs_lock);
		hlist_for_each_entry_safe(r, n, &adpt->arfs_fltr_list[i],
					  list_entry) {
			hlist_del(&r->list_entry);
			devm_kfree(dev, r);
		}
		spin_unlock_bh(&adpt->arfs_lock);
	}

	kfree(adpt->arfs_fltr_list);
	adpt->arfs_fltr_list = NULL;
	kfree(adpt->arfs_last_fltr_id);
	adpt->arfs_last_fltr_id = NULL;
	kfree(adpt->arfs_fltr_cntrs);
	adpt->arfs_fltr_cntrs = NULL;
}

void ne6x_remove_arfs(struct ne6x_adapter *adpt)
{
	if (!adpt)
		return;

	ne6x_clear_arfs(adpt);
}

#endif
