#ifdef SXE_IPSEC_CONFIGURE
#include <net/xfrm.h>
#include <crypto/aead.h>
#include <linux/if_bridge.h>

#include "sxevf_ipsec.h"
#include "sxevf_tx_proc.h"
#include "sxevf_ring.h"
#include "sxevf_msg.h"

static const char ipsec_aes_name[] = "rfc4106(gcm(aes))";

s32 sxevf_ipsec_sa_add(struct sxevf_adapter *adapter,
			struct xfrm_state *xs, u32 *pf_sa_idx)
{
	struct sxevf_ipsec_add_msg msg = {};
	struct sxevf_hw *hw = &adapter->hw;
	s32 ret;

	msg.msg_type = SXEVF_IPSEC_ADD;

	msg.flags = xs->xso.flags;
	msg.spi = xs->id.spi;
	msg.proto = xs->id.proto;
	msg.family = xs->props.family;

	if (xs->props.family == AF_INET6)
		memcpy(msg.addr, &xs->id.daddr.a6, sizeof(xs->id.daddr.a6));
	else
		memcpy(msg.addr, &xs->id.daddr.a4, sizeof(xs->id.daddr.a4));

	memcpy(msg.key, xs->aead->alg_key, sizeof(msg.key));

	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg,
				     SXEVF_MSG_NUM(sizeof(msg)));
	if (!ret && (msg.msg_type == (SXEVF_IPSEC_ADD | SXEVF_MSGTYPE_ACK))) {
		if (pf_sa_idx) {
			*pf_sa_idx = msg.pf_sa_idx;
		}
		LOG_INFO("xfrm state flags:0x%x spi:0x%x proto:0x%x "
			  "family:0x%x add to pf_sa_idx:%u\n",
			  xs->xso.flags, xs->id.spi,
			  xs->id.proto, xs->props.family,
			  msg.pf_sa_idx);
	} else {
		LOG_ERROR("xfrm state flags:0x%x spi:0x%x proto:0x%x "
			  "family:0x%x add to pf fail.(err:%d)\n",
			  xs->xso.flags, xs->id.spi,
			  xs->id.proto, xs->props.family, ret);
	}

	return ret;
}

s32 sxevf_ipsec_sa_del(struct sxevf_adapter *adapter, u32 pf_sa_idx)
{
	struct sxevf_ipsec_del_msg msg = {};
	struct sxevf_hw *hw = &adapter->hw;
	s32 ret;

	msg.msg_type = SXEVF_IPSEC_DEL;
	msg.sa_idx = pf_sa_idx;

	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg,
				      SXEVF_MSG_NUM(sizeof(msg)));
	if (ret) {
		LOG_ERROR("del pf sa:%d fail.(err:%d)\n", pf_sa_idx, ret);
		goto l_end;
	}

l_end:
	return ret;
}

STATIC inline bool sxevf_need_tx_ipsec_offload(struct sk_buff *skb)
{
	struct sec_path *sp = skb->sp;
	bool ret = true;

	if (!sp || !sp->olen || (sp->len != sp->olen)) {
		ret = false;
	}

	return ret;
}

STATIC struct xfrm_state  *sxevf_ipsec_rx_sa_match(struct sxevf_ipsec_context *ipsec,
				 __be32 spi, u8 proto, __be32 *daddr, u8 daddr_len)
{
	struct sxevf_rx_sa *sa = NULL;
	struct xfrm_state  *xs = NULL;

	rcu_read_lock();
	hash_for_each_possible_rcu(ipsec->rx_table_list, sa, hlist,
					(__force u32) spi) {
		if (spi == sa->xs->id.spi &&
		    proto == sa->xs->id.proto &&
		    !memcmp(daddr, &(sa->xs->id.daddr), daddr_len)) {
			xs = sa->xs;
			xfrm_state_hold(xs); 
			break;
		}
	}
	rcu_read_unlock();

	return xs;
}

static s32 sxevf_ipsec_tx_offload_param_valid(struct sk_buff *skb,
				struct sxevf_tx_sa *sa,
				u32 *vf_sa_idx, struct xfrm_state **xfrm_state)
{
	s32 ret = -SXEVF_ERR_ARGUMENT_INVALID;
	u32 idx;
	struct sec_path *path;
	struct xfrm_state *xs;

	path = skb_sec_path(skb);
	if (unlikely(!path->len)) {
		LOG_DEV_ERR("security path len:0 invalid.\n");
		goto l_out;
	}

	xs = xfrm_input_state(skb);
	if (unlikely(!xs)) {
		LOG_DEV_ERR("security input xs NULL.\n");
		goto l_out;
	}

	*xfrm_state = xs;
	idx = xs->xso.offload_handle - SXEVF_IPSEC_TX_INDEX_BASE;
	if (idx >= SXEVF_IPSEC_SA_CNT_MAX) {
		LOG_DEV_ERR("invalid offload_handle:%lu idx:%d.\n",
			   xs->xso.offload_handle, idx);
		goto l_out;
	}

	if (!test_bit(SXEVF_IPSEC_SA_ENTRY_USED, &sa[idx].status)) {
		LOG_DEV_ERR("tx_table[%d] not used.\n", idx);
		goto l_out;
	}

	*vf_sa_idx = idx;

	LOG_INFO("vf_sa_idx:%u tx ipsec offload valid passed\n",
		  *vf_sa_idx);
	ret = 0;

l_out:
	return ret;
}

s32 sxevf_tx_ipsec_offload(struct sxevf_ring *tx_ring,
			  struct sxevf_tx_buffer *first,
			  struct sxevf_tx_context_desc *ctxt_desc)
{
	u32 vf_sa_idx;
	s32 ret = 0;
	struct sxevf_adapter *adapter = netdev_priv(tx_ring->netdev);
	struct sxevf_ipsec_context *ipsec = &adapter->ipsec_ctxt;
	struct sxevf_tx_sa *sa = ipsec->tx_table;
	struct xfrm_state *xfrm_state = NULL;
	u32 tucmd_ipsec = 0;

	if (!sxevf_need_tx_ipsec_offload(first->skb)) {
		LOG_DEBUG("ring[%u] no need offload IPsec.\n", tx_ring->idx);
		goto l_out;
	}

	ret = sxevf_ipsec_tx_offload_param_valid(first->skb, sa, &vf_sa_idx, &xfrm_state);
	if (ret) {
		LOG_ERROR("ring[%d ]tx ipsec valid failed.\n", tx_ring->idx);
		goto l_out;
	}

	first->tx_features |= SXEVF_TX_FEATURE_IPSEC | SXEVF_TX_FEATURE_CSUM;

	if (xfrm_state->id.proto == IPPROTO_ESP) {
		tucmd_ipsec = SXEVF_TX_CTXTD_TUCMD_IPSEC_TYPE_ESP |
				SXEVF_TX_CTXTD_TUCMD_L4T_TCP;
		if (first->protocol == htons(ETH_P_IP)) {
			tucmd_ipsec |= SXEVF_TX_CTXTD_TUCMD_IPV4;
		}

		if (!skb_is_gso(first->skb)) {
			const u32 auth_len = SXEVF_IPSEC_AUTH_BIT_LEN / CHAR_BITS;
			u8 pad_len;

			ret = skb_copy_bits(first->skb, first->skb->len - SXEVF_IPSEC_PADLEN_OFFSET,
						&pad_len, SXEVF_IPSEC_PADLEN_BYTE);
			if (unlikely(ret)) {
				LOG_ERROR("auth_len:%d offset:%d copy skb "
					  "failed.(err:%d)\n",
					   auth_len,
					   first->skb->len - SXEVF_IPSEC_PADLEN_OFFSET,
					   ret);
				goto l_out;
			}
			tucmd_ipsec |= (SXEVF_IPSEC_PADLEN_OFFSET + pad_len);
		}
	}

	if (sa[vf_sa_idx].encrypt) {
		tucmd_ipsec |= SXEVF_TX_CTXTD_TUCMD_IPSEC_ENCRYPT_EN;
	}

	sxevf_ctxt_desc_sa_idx_set(ctxt_desc, vf_sa_idx);

	sxevf_ctxt_desc_tucmd_set(ctxt_desc, tucmd_ipsec);

l_out:
	return ret;
}

void sxevf_rx_ipsec_proc(struct sxevf_ring *tx_ring,
			  union sxevf_rx_data_desc *desc,
			  struct sk_buff *skb)
{
	s32 ret = 0;
	struct sxevf_adapter *adapter = netdev_priv(tx_ring->netdev);
	struct sxevf_ipsec_context *ipsec = &adapter->ipsec_ctxt;
	__le16 pkt_info = desc->wb.lower.lo_dword.hs_rss.pkt_info;
	struct iphdr *ip4_hdr = NULL;
	struct ipv6hdr *ip6_hdr = NULL;
	void *daddr = NULL; 
	unsigned long daddr_len;
	u8 *sec_hdr = NULL; 
	struct xfrm_state *xs = NULL;
	struct xfrm_offload *offload = NULL;
	__be32 spi;
	u8 proto;

	if (!sxevf_status_err_check(desc, SXEVF_RXD_STAT_SECP)) {
		LOG_DEBUG("not security packet, no need parse "
			  "security header.\n");
		goto l_out;
	}

	if (pkt_info & cpu_to_le16(SXEVF_RXDADV_PKTTYPE_IPV4)) {
		ip4_hdr = (struct iphdr *)(skb->data + ETH_HLEN);
		daddr = &ip4_hdr->daddr;
		daddr_len = sizeof(ip4_hdr->daddr);
		sec_hdr = (u8 *)ip4_hdr + ip4_hdr->ihl * SXEVF_IP_HEAD_LEN_UNIT;
	} else if (pkt_info & cpu_to_le16(SXEVF_RXDADV_PKTTYPE_IPV6)) {
		ip6_hdr = (struct ipv6hdr *)(skb->data + ETH_HLEN);
		daddr = &ip6_hdr->daddr;
		daddr_len = sizeof(ip6_hdr->daddr);
		sec_hdr = (u8 *)ip6_hdr + sizeof(struct ipv6hdr);
	} else {
		ret = -SXEVF_ERR_DEVICE_NOT_SUPPORTED;
		LOG_ERROR("sxe security not support L3 protocol:0x%x.(err:%d)\n",
			  desc->wb.lower.lo_dword.hs_rss.pkt_info, ret);
		goto l_out;
	};

	if (pkt_info & cpu_to_le16(SXEVF_RXDADV_PKTTYPE_IPSEC_ESP)) {
		spi = ((struct ip_esp_hdr *)sec_hdr)->spi;
		proto = IPPROTO_ESP;
	} else if (pkt_info & cpu_to_le16(SXEVF_RXDADV_PKTTYPE_IPSEC_AH)) {
		spi = ((struct ip_auth_hdr *)sec_hdr)->spi;
		proto = IPPROTO_AH;
	} else {
		ret = -SXEVF_ERR_DEVICE_NOT_SUPPORTED;
		LOG_ERROR("sxe security not support security protocol:0x%x.(err:%d)\n",
			  desc->wb.lower.lo_dword.hs_rss.pkt_info, ret);
		goto l_out;
	}

	xs = sxevf_ipsec_rx_sa_match(ipsec, spi, proto, daddr, *(u8 *)&daddr_len);
	if (!xs) {
		ret = -SXEVF_ERR_IPSEC_SA_STATE_NOT_EXSIT;
		LOG_ERROR("spi:0x%x, proto:0x%x daddr:%pI6 daddr_len:%lu"
			"not matched sw rx sa entry.(err:%d)",
			spi, proto, daddr, daddr_len, ret);
		goto l_out;
	}

	skb->sp = secpath_dup(skb->sp);
	if (unlikely(!skb->sp)) {
		LOG_INFO("skb security path null.\n");
		goto l_out;
	}

	skb->sp->xvec[skb->sp->len++] = xs;
	skb->sp->olen++;

	offload = xfrm_offload(skb);
	offload->flags = CRYPTO_DONE;
	offload->status = CRYPTO_SUCCESS;

	ipsec->rx_ipsec++;

l_out:
	return;
}

static bool sxevf_ipsec_offload_ok(struct sk_buff *skb, struct xfrm_state *xs)
{
	bool ret = true;

	if (xs->props.family == AF_INET) {
		if (ip_hdr(skb)->ihl != 5) {
			LOG_ERROR("sxe ipsec offload unsupport ipv4 "
				  "header with option, hdr len:%d.\n",
				   ip_hdr(skb)->ihl);
		        ret = false;
		}
	} else {
		if (ipv6_ext_hdr(ipv6_hdr(skb)->nexthdr)) {
			LOG_ERROR("sxe ipsec offload unsupport ipv6 "
				  "header with ext hdr\n");
				  ret = false;
		}
	}

        return ret;
}

static s32 sxevf_ipsec_param_valid(struct xfrm_state *xs)
{
	s32 ret = -EINVAL;

	if ((xs->id.proto != IPPROTO_ESP) &&
	   (xs->id.proto != IPPROTO_AH)) {
		LOG_DEV_ERR("flags:%u offload:0x%lx unsupport "
			 "security protol:0x%x.\n",
			  xs->xso.flags,
			  xs->xso.offload_handle,
			  xs->id.proto);
		goto l_out;
	}

	if (xs->xso.flags & XFRM_OFFLOAD_INBOUND) {
		if (xs->calg) {
			LOG_DEV_ERR("proto:%u flags:%u offload:0x%lx unsupport "
				 "compression offload\n",
				  xs->id.proto,
				  xs->xso.flags,
				  xs->xso.offload_handle);
			goto l_out;
		}
	}

	ret = 0;

	LOG_INFO("proto:%u flags:%u offload:0x%lx ipsec param valid pass\n",
		  xs->id.proto,
		  xs->xso.flags,
		  xs->xso.offload_handle);

l_out:
	return ret;
}

static s32 sxevf_ipsec_sa_idx_get(struct sxevf_ipsec_context *ipsec, bool is_rx)
{
	s32 ret = -ENOSPC;
	u16 i;

	if (is_rx) {
		if (ipsec->rx_sa_cnt == SXEVF_IPSEC_SA_CNT_MAX) {
			LOG_ERROR("ipsec rx sa cnt reach limit:%u.\n",
				   SXEVF_IPSEC_SA_CNT_MAX);
			goto l_out;
		}

		for (i = 0; i < SXEVF_IPSEC_SA_CNT_MAX; i++) {
			if (!test_and_set_bit(SXEVF_IPSEC_SA_ENTRY_USED,
				&ipsec->rx_table[i].status)) {
				ret = i;
				break;
			}
		}
	} else {
		if (ipsec->tx_sa_cnt == SXEVF_IPSEC_SA_CNT_MAX) {
			LOG_ERROR("ipsec tx sa cnt reach limit:%u.\n",
				   SXEVF_IPSEC_SA_CNT_MAX);
			goto l_out;
		}

		for (i = 0; i < SXEVF_IPSEC_SA_CNT_MAX; i++) {
			if (!test_and_set_bit(SXEVF_IPSEC_SA_ENTRY_USED,
				&ipsec->tx_table[i].status)) {
				ret = i;
				break;
			}
		}
	}

l_out:
	return ret;
}

static s32 sxevf_ipsec_key_salt_parse(struct xfrm_state *xs,
						u32 *key, u32 *salt)
{
	s32 ret = 0;
	s8 *xs_key;
	unsigned long len;

	if (!xs->aead) {
		ret = -EINVAL;
		LOG_DEV_ERR("ipsec offload algorithm unsupport.(err:%d)\n", ret);
		goto l_out;
	}

	if (xs->aead->alg_icv_len != SXEVF_IPSEC_AUTH_BIT_LEN) {
		ret = -EINVAL;
		LOG_DEV_ERR("ipsec offload icv len:%u "
			"unsupport.(err:%d)\n",
			xs->aead->alg_icv_len, ret);
		goto l_out;
	}

	if (strcmp(xs->aead->alg_name, ipsec_aes_name)) {
		ret = -EINVAL;
		LOG_DEV_ERR("unsupport alg name:%s, just support alg:%s.(err:%d)\n",
			xs->aead->alg_name, ipsec_aes_name, ret);
		goto l_out;
	}

	xs_key = xs->aead->alg_key;
	len = xs->aead->alg_key_len;

	if (len == SXEVF_IPSEC_KEY_SALT_BIT_LEN) {
		*salt = *(u32 *)(xs_key + SXEVF_IPSEC_KEY_BYTE_LEN);
	} else if (len == SXEVF_IPSEC_KEY_BIT_LEN) {
		*salt = 0;
	} else {
		ret = -EINVAL;
		LOG_DEV_ERR("unsupport key_salt len:%lu.(err:%d)\n", len, ret);
		goto l_out;
	}

	memcpy(key, xs_key, sizeof(u32) * SXEVF_IPSEC_KEY_LEN);

	LOG_INFO("ipsec offload flag:0x%x key_salt len:%lu "
		 "salt:%u key:0x%x%x%x%x.\n",
		 xs->xso.flags ,len,
		 *salt, key[0], key[1], key[2], key[3]);

l_out:
	return ret;
}

static s32 sxevf_ipsec_rx_sa_entry_fill(struct xfrm_state *xs,
					struct sxevf_rx_sa *sa_entry)
{
	s32 ret;

	memset(sa_entry, 0, sizeof(*sa_entry));

	sa_entry->xs = xs;

	if (xs->id.proto & IPPROTO_ESP) {
		sa_entry->decrypt = !!((xs->ealg) || (xs->aead));
	}

	ret = sxevf_ipsec_key_salt_parse(xs, sa_entry->key, &sa_entry->salt);
	if (ret) {
		LOG_DEV_ERR("ipsec offload key salt param parse fail.(err:%d)\n",
			   ret);
		goto l_out;
	}

	if (xs->props.family == AF_INET6) {
		memcpy(sa_entry->ip_addr, &xs->id.daddr.a6, SXEVF_IPV6_ADDR_SIZE);
	} else {
		memcpy(&sa_entry->ip_addr[SXEVF_IPV4_ADDR_SIZE - 1],
			&xs->id.daddr.a4,
			SXEVF_IPV4_ADDR_SIZE);
	}

	sa_entry->mode = SXEVF_IPSEC_RXMOD_VALID;
	if (sa_entry->xs->id.proto & IPPROTO_ESP) {
		sa_entry->mode |= SXEVF_IPSEC_RXMOD_PROTO_ESP;
	}

	if (sa_entry->decrypt) {
		sa_entry->mode |= SXEVF_IPSEC_RXMOD_DECRYPT;
	}

	if (sa_entry->xs->props.family == AF_INET6) {
		sa_entry->mode |= SXEVF_IPSEC_RXMOD_IPV6;
	}

l_out:
	return ret;
}

static s32 sxevf_ipsec_sa_add_to_pf(struct sxevf_adapter *adapter,
				struct xfrm_state *xs, u32 *pf_sa_idx)
{
	s32 ret;

	spin_lock_bh(&adapter->mbx_lock);
	ret = sxevf_ipsec_sa_add(adapter, xs, pf_sa_idx);
	spin_unlock_bh(&adapter->mbx_lock);
	if (ret) {
		LOG_ERROR("xfrm state flags:0x%x spi:0x%x proto:0x%x "
			  "family:0x%x add to pf fail.(err:%d)\n",
			  xs->xso.flags, xs->id.spi,
			  xs->id.proto, xs->props.family, ret);
	}

	return ret;
}

static s32 sxevf_ipsec_rx_xs_add(struct sxevf_adapter *adapter,
					     struct xfrm_state *xs)
{
	struct sxevf_rx_sa sa_entry;
	struct sxevf_ipsec_context *ipsec = &adapter->ipsec_ctxt;
	u32 vf_sa_idx; 
	s32 ret;
	u32 pf_sa_idx;

	ret = sxevf_ipsec_sa_idx_get(ipsec, true);
	if (ret < 0) {
		LOG_DEV_ERR("rx_sa_cnt:%d rx sa table no space.(err:%d)\n",
			   ipsec->rx_sa_cnt, ret);
		goto l_out;
	}

	vf_sa_idx = (u32)ret;

	sa_entry.status = ipsec->rx_table[vf_sa_idx].status;

	ret = sxevf_ipsec_rx_sa_entry_fill(xs, &sa_entry);
	if (ret) {
		LOG_ERROR("ipsec offload  param parse fail.(err:%d)\n", ret);
		goto clear_used_xs;
	}

	ret = sxevf_ipsec_sa_add_to_pf(adapter, xs, &pf_sa_idx);
	if (ret) {
		LOG_ERROR("xfrm state flags:0x%x spi:0x%x proto:0x%x "
			  "family:0x%x add to pf fail.(err:%d)\n",
			  xs->xso.flags, xs->id.spi,
			  xs->id.proto, xs->props.family, ret);
		goto clear_used_xs;
	}

	memcpy(&ipsec->rx_table[vf_sa_idx], &sa_entry, sizeof(sa_entry));
	sa_entry.pf_sa_idx = pf_sa_idx;
	xs->xso.offload_handle = vf_sa_idx + SXEVF_IPSEC_RX_INDEX_BASE;

	ipsec->rx_sa_cnt++;

	LOG_INFO("tx_sa_table[%u] add done pf_sa_idx:%u rx_sa_cnt:%u.\n",
		   vf_sa_idx, pf_sa_idx, ipsec->rx_sa_cnt);

l_out:
	return ret;

clear_used_xs:
	clear_bit(SXEVF_IPSEC_SA_ENTRY_USED, &ipsec->rx_table[vf_sa_idx].status);

	return ret;
}

static s32 sxevf_ipsec_tx_xs_add(struct sxevf_adapter *adapter,
					     struct xfrm_state *xs)
{
	struct sxevf_tx_sa sa_entry;
	struct sxevf_ipsec_context *ipsec = &adapter->ipsec_ctxt;
	u32 vf_sa_idx;
	s32 ret;
	u32 pf_sa_idx;

	ret = sxevf_ipsec_sa_idx_get(ipsec, false);
	if (ret < 0) {
		LOG_DEV_ERR("tx_sa_cnt:%d tx sa table no space.(err:%d)\n",
			   ipsec->tx_sa_cnt, ret);
		goto l_out;
	}

	vf_sa_idx = (u32)ret;

	memset(&sa_entry, 0, sizeof(struct sxevf_tx_sa));

	sa_entry.xs = xs;
	sa_entry.status = ipsec->tx_table[vf_sa_idx].status;

	if (xs->id.proto & IPPROTO_ESP) {
		sa_entry.encrypt = !!((xs->ealg) || (xs->aead));
	}

	ret = sxevf_ipsec_key_salt_parse(xs, sa_entry.key, &sa_entry.salt);
	if (ret) {
		LOG_DEV_ERR("ipsec offload key salt param parse fail.(err:%d)\n",
			   ret);
		goto clear_used_xs;
	}

	ret = sxevf_ipsec_sa_add_to_pf(adapter, xs, &pf_sa_idx);
	if (ret) {
		LOG_ERROR("xfrm state flags:0x%x spi:0x%x proto:0x%x "
			  "family:0x%x add to pf fail.(err:%d)\n",
			  xs->xso.flags, xs->id.spi,
			  xs->id.proto, xs->props.family, ret);
		goto clear_used_xs;
	}

	memcpy(&ipsec->tx_table[vf_sa_idx], &sa_entry, sizeof(sa_entry));
	sa_entry.pf_sa_idx = pf_sa_idx;
	xs->xso.offload_handle = vf_sa_idx + SXEVF_IPSEC_TX_INDEX_BASE;

	ipsec->tx_sa_cnt++;

	LOG_INFO("tx_sa_table[%u] add done pf_sa_idx:%u tx_sa_cnt:%u.\n",
		   vf_sa_idx, pf_sa_idx, ipsec->tx_sa_cnt);

l_out:
	return ret;

clear_used_xs:
	clear_bit(SXEVF_IPSEC_SA_ENTRY_USED, &ipsec->tx_table[vf_sa_idx].status);

	return ret;
}

static s32 sxevf_ipsec_state_add(struct xfrm_state *xs)
{
	s32 ret;
	struct net_device *net_dev = xs->xso.dev;
	struct sxevf_adapter *adapter = netdev_priv(net_dev);

	ret = sxevf_ipsec_param_valid(xs);
	if (ret) {
		LOG_ERROR("ipsec offload param invalid.(err:%d)\n",
			  ret);
		goto l_out;
	}

	if (xs->xso.flags & XFRM_OFFLOAD_INBOUND) {
		ret = sxevf_ipsec_rx_xs_add(adapter, xs);
	} else {
		ret = sxevf_ipsec_tx_xs_add(adapter, xs);
	}

	if (ret) {
		LOG_ERROR("offload_handle:%lu flag:0x%x sa add fail.(err:%d)\n",
			  xs->xso.offload_handle,
			  xs->xso.flags, ret);
		goto l_out;
	}

l_out:
	return ret;
}

static void sxevf_ipsec_state_delete(struct xfrm_state *xs)
{
	struct net_device *netdev = xs->xso.dev;
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	struct sxevf_ipsec_context *ipsec = &adapter->ipsec_ctxt;
	u32 vf_sa_idx;
	u32 pf_sa_idx;
	s32 ret;

	if (xs->xso.flags & XFRM_OFFLOAD_INBOUND) {
		vf_sa_idx = xs->xso.offload_handle - SXEVF_IPSEC_RX_INDEX_BASE;
		pf_sa_idx = ipsec->rx_table[vf_sa_idx].pf_sa_idx;

		if (!test_bit(SXEVF_IPSEC_SA_ENTRY_USED,
			      &ipsec->rx_table[vf_sa_idx].status)) {
			LOG_DEV_ERR("vf_sa_idx:%d not in used, offload_handle: %lu.\n",
				vf_sa_idx, xs->xso.offload_handle);
			goto l_end;
		}

		spin_lock_bh(&adapter->mbx_lock);
		ret = sxevf_ipsec_sa_del(adapter, pf_sa_idx);
		spin_unlock_bh(&adapter->mbx_lock);
		if (ret) {
			LOG_ERROR("vf_sa_idx:%u pf_sa_idx:0x%x "
				  "flags:0x%x del fail.(err:%d)\n",
				  vf_sa_idx,
				  pf_sa_idx,
				  xs->xso.flags, ret);
			goto l_end;
		}

		hash_del_rcu(&ipsec->rx_table[vf_sa_idx].hlist);
		memset(&ipsec->rx_table[vf_sa_idx], 0, sizeof(struct sxevf_rx_sa));
		ipsec->rx_sa_cnt--;
	} else {
		vf_sa_idx = xs->xso.offload_handle - SXEVF_IPSEC_TX_INDEX_BASE;
		pf_sa_idx = ipsec->tx_table[vf_sa_idx].pf_sa_idx;
		if (!test_bit(SXEVF_IPSEC_SA_ENTRY_USED,
			      &ipsec->tx_table[vf_sa_idx].status)) {
			LOG_DEV_ERR("vf_sa_idx:%d not in used, offload_handle: %lu.\n",
				vf_sa_idx, xs->xso.offload_handle);
			goto l_end;
		}

		spin_lock_bh(&adapter->mbx_lock);
		ret = sxevf_ipsec_sa_del(adapter, pf_sa_idx);
		spin_unlock_bh(&adapter->mbx_lock);
		if (ret) {
			LOG_ERROR("vf_sa_idx:%u pf_sa_idx:0x%x "
				  "flags:0x%x del fail.(err:%d)\n",
				  vf_sa_idx,
				  pf_sa_idx,
				  xs->xso.flags, ret);
			goto l_end;
		}

		memset(&ipsec->tx_table[vf_sa_idx], 0, sizeof(struct sxevf_tx_sa));
		ipsec->tx_sa_cnt--;
	}

	LOG_ERROR("vf_sa_idx:%u pf_sa_idx:0x%x flags:0x%x del done.\n",
		  vf_sa_idx,
		  pf_sa_idx,
		  xs->xso.flags);

l_end:
	return;
}

static const struct xfrmdev_ops sxevf_xfrmdev_ops = {
	.xdo_dev_offload_ok = sxevf_ipsec_offload_ok,
	.xdo_dev_state_add = sxevf_ipsec_state_add,
	.xdo_dev_state_delete = sxevf_ipsec_state_delete,
};

void sxevf_ipsec_offload_init(struct sxevf_adapter *adapter)
{
	struct sxevf_ipsec_context *ipsec = &adapter->ipsec_ctxt;
	u32 size;

	hash_init(ipsec->rx_table_list);

	size = sizeof(struct sxevf_rx_sa) * SXEVF_IPSEC_SA_CNT_MAX;
	ipsec->rx_table = kzalloc(size, GFP_KERNEL);
	if (ipsec->rx_table == NULL) {
		LOG_DEV_ERR("ipsec rx sa table mem:%uB alloc fail.\n", size);
		goto l_out;
	}

	size = sizeof(struct sxevf_tx_sa) * SXEVF_IPSEC_SA_CNT_MAX;
	ipsec->tx_table = kzalloc(size, GFP_KERNEL);
	if (ipsec->tx_table == NULL) {
		LOG_DEV_ERR("ipsec tx sa table mem:%uB alloc fail.\n", size);
		goto l_free_rx_table;
	}

	ipsec->rx_sa_cnt = 0;
	ipsec->tx_sa_cnt = 0;

	adapter->netdev->xfrmdev_ops = &sxevf_xfrmdev_ops;

	LOG_INFO("ipsec init done.\n");

l_out:
	return;

l_free_rx_table:
	SXEVF_KFREE(ipsec->rx_table);
	return;
}

void sxevf_ipsec_offload_exit(struct sxevf_adapter *adapter)
{
	struct sxevf_ipsec_context *ipsec = &adapter->ipsec_ctxt;

	SXEVF_KFREE(ipsec->rx_table);
	SXEVF_KFREE(ipsec->tx_table);

	LOG_INFO("ipsec exit done.\n");

	return;
}

void sxevf_ipsec_restore(struct sxevf_adapter *adapter)
{
	struct sxevf_ipsec_context *ipsec = &adapter->ipsec_ctxt;
	struct sxevf_rx_sa *rx_sa;
	struct sxevf_tx_sa *tx_sa;
	u16 i;
	s32 ret;

	if (!(adapter->netdev->features & NETIF_F_HW_ESP)) {
		goto l_end;
	}

	for (i = 0; i < SXEVF_IPSEC_SA_CNT_MAX; i++) {
		rx_sa = &ipsec->rx_table[i];
		tx_sa = &ipsec->tx_table[i];

		if (test_bit(SXEVF_IPSEC_SA_ENTRY_USED, &rx_sa->status)) {
			ret = sxevf_ipsec_sa_add_to_pf(adapter, rx_sa->xs, NULL);
			if (ret) {
				LOG_DEV_ERR("rx xfrm state flags:0x%x spi:0x%x proto:0x%x "
					  "family:0x%x add to pf fail.(err:%d)\n",
					  rx_sa->xs->xso.flags,
					  rx_sa->xs->id.spi,
					  rx_sa->xs->id.proto,
					  rx_sa->xs->props.family, ret);
			}
		}

		if (test_bit(SXEVF_IPSEC_SA_ENTRY_USED, &tx_sa->status)) {
			ret = sxevf_ipsec_sa_add_to_pf(adapter, tx_sa->xs, NULL);
			if (ret) {
				LOG_DEV_ERR("tx xfrm state flags:0x%x spi:0x%x proto:0x%x "
					  "family:0x%x add to pf fail.(err:%d)\n",
					  tx_sa->xs->xso.flags,
					  tx_sa->xs->id.spi,
					  tx_sa->xs->id.proto,
					  tx_sa->xs->props.family, ret);
			}
		}

	}

l_end:
	return;
}

#endif

