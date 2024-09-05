
#ifdef SXE_IPSEC_CONFIGURE

#include <net/xfrm.h>
#include <crypto/aead.h>
#include <linux/if_bridge.h>

#include "sxe_ipsec.h"
#include "sxe_tx_proc.h"
#include "sxe_ring.h"
#include "sxe_sriov.h"

static const char ipsec_aes_name[] = "rfc4106(gcm(aes))";

#ifdef CONFIG_XFRM
static inline bool sxe_need_tx_ipsec_offload(struct sk_buff *skb)
{
	struct sec_path *sp = skb->sp;
	bool ret = true;

	if (!sp || !sp->olen || (sp->len != sp->olen)) {
		ret = false;
	}

	return ret;
}

static struct xfrm_state  *sxe_ipsec_rx_sa_match(struct sxe_ipsec_context *ipsec,
				 __be32 spi, u8 proto, __be32 *daddr, u8 daddr_len)
{
	struct sxe_rx_sa *sa;
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

#else
STATIC inline bool sxe_need_tx_ipsec_offload(struct sk_buff *skb)
{
	return false;
}

STATIC struct xfrm_state  *sxe_ipsec_rx_sa_match(struct sxe_ipsec_context *ipsec,
				 __be32 spi, u8 proto, __be32 *daddr, u8 daddr_len)
{
	return NULL;
}

#endif

static s32 sxe_ipsec_tx_offload_param_valid(
				struct sxe_adapter *adapter,
				struct sk_buff *skb,
				struct sxe_tx_sa *sa,
				u16 *sa_idx, struct xfrm_state **xfrm_state)
{
	s32 ret = -SXE_ERR_PARAM;
	u16 idx;
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
	idx = xs->xso.offload_handle - SXE_IPSEC_TX_INDEX_BASE;
	if (idx >= SXE_IPSEC_SA_CNT_MAX) {
		LOG_DEV_ERR("invalid offload_handle:%lu idx:%d.\n",
			   xs->xso.offload_handle, idx);
		goto l_out;
	}

	if (!test_bit(SXE_IPSEC_SA_ENTRY_USED, &sa[idx].status)) {
		LOG_DEV_ERR("tx_table[%d] not used.\n", idx);
		goto l_out;
	}

	*sa_idx = idx;

	LOG_INFO("idx:%d tx ipsec offload valid passed\n",
		  idx);
	ret = 0;

l_out:
	return ret;
}

s32 sxe_tx_ipsec_offload(struct sxe_ring *tx_ring,
			  struct sxe_tx_buffer *first,
			  struct sxe_tx_context_desc *ctxt_desc)
{
	u16 sa_idx;
	s32 ret = 0;
	struct sxe_adapter *adapter = netdev_priv(tx_ring->netdev);
	struct sxe_ipsec_context *ipsec = &adapter->ipsec;
	struct sxe_tx_sa *sa = ipsec->tx_table;
	struct xfrm_state *xfrm_state = NULL;
	u32 tucmd_ipsec = 0;

	if (!sxe_need_tx_ipsec_offload(first->skb)) {
		LOG_DEBUG("ring[%u] no need offload IPsec.\n", tx_ring->idx);
		goto l_out;
	}

	ret = sxe_ipsec_tx_offload_param_valid(adapter, first->skb, sa, &sa_idx, &xfrm_state);
	if (ret) {
		LOG_ERROR("ring[%d ]tx ipsec valid failed.\n", tx_ring->idx);
		goto l_out;
	}

	first->tx_features |= SXE_TX_FEATURE_IPSEC;

	if (xfrm_state->id.proto == IPPROTO_ESP) {
		tucmd_ipsec = SXE_TX_CTXTD_TUCMD_IPSEC_TYPE_ESP |
				SXE_TX_CTXTD_TUCMD_L4T_TCP;
		if (first->protocol == htons(ETH_P_IP)) {
			tucmd_ipsec |= SXE_TX_CTXTD_TUCMD_IPV4;
		}

		if (!skb_is_gso(first->skb)) {
			const u32 auth_len = SXE_IPSEC_AUTH_BIT_LEN / CHAR_BITS;
			u8 pad_len;

			ret = skb_copy_bits(first->skb, first->skb->len - SXE_IPSEC_PADLEN_OFFSET,
						&pad_len, 1);
			if (unlikely(ret)) {
				LOG_ERROR("auth_len:%d offset:%d copy skb "
					  "failed.(err:%d)\n",
					   auth_len,
					   first->skb->len - SXE_IPSEC_PADLEN_OFFSET,
					   ret);
				goto l_out;
			}
			tucmd_ipsec |= (SXE_IPSEC_PADLEN_OFFSET + pad_len);
		}
	} else {
		if (first->protocol == htons(ETH_P_IP)) {
			tucmd_ipsec |= SXE_TX_CTXTD_TUCMD_IPV4;
		}
	}

	if (sa[sa_idx].encrypt) {
		tucmd_ipsec |= SXE_TX_CTXTD_TUCMD_IPSEC_ENCRYPT_EN;
	}

	sxe_ctxt_desc_sa_idx_set(ctxt_desc, sa_idx);

	sxe_ctxt_desc_tucmd_set(ctxt_desc, tucmd_ipsec);

l_out:
	return ret;
}

void sxe_rx_ipsec_proc(struct sxe_ring *tx_ring,
			  union sxe_rx_data_desc *desc,
			  struct sk_buff *skb)
{
	s32 ret = 0;
	struct sxe_adapter *adapter = netdev_priv(tx_ring->netdev);
	struct sxe_ipsec_context *ipsec = &adapter->ipsec;
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

	if (!sxe_status_err_check(desc, SXE_RXD_STAT_SECP)) {
		LOG_DEBUG("not security packet, no need parse "
			  "security header.\n");
		goto l_out;
	}

	if (pkt_info & cpu_to_le16(SXE_RXDADV_PKTTYPE_IPV4)) {
		ip4_hdr = (struct iphdr *)(skb->data + ETH_HLEN);
		daddr = &ip4_hdr->daddr;
		daddr_len = sizeof(ip4_hdr->daddr);
		sec_hdr = (u8 *)ip4_hdr + ip4_hdr->ihl * SXE_IP_HEAD_LEN_UNIT;
	} else if (pkt_info & cpu_to_le16(SXE_RXDADV_PKTTYPE_IPV6)) {
		ip6_hdr = (struct ipv6hdr *)(skb->data + ETH_HLEN);
		daddr = &ip6_hdr->daddr;
		daddr_len = sizeof(ip6_hdr->daddr);
		sec_hdr = (u8 *)ip6_hdr + sizeof(struct ipv6hdr);
	} else {
		ret = -SXE_ERR_DEVICE_NOT_SUPPORTED;
		LOG_ERROR("sxe security not support L3 protocol:0x%x.\n",
			  desc->wb.lower.lo_dword.hs_rss.pkt_info);
		goto l_out;
	};

	if (pkt_info & cpu_to_le16(SXE_RXDADV_PKTTYPE_IPSEC_ESP)) {
		spi = ((struct ip_esp_hdr *)sec_hdr)->spi;
		proto = IPPROTO_ESP;
	} else if (pkt_info & cpu_to_le16(SXE_RXDADV_PKTTYPE_IPSEC_AH)) {
		spi = ((struct ip_auth_hdr *)sec_hdr)->spi;
		proto = IPPROTO_AH;
	} else {
		ret = -SXE_ERR_DEVICE_NOT_SUPPORTED;
		LOG_ERROR("sxe security not support security protocol:0x%x.\n",
			  desc->wb.lower.lo_dword.hs_rss.pkt_info);
		goto l_out;
	}

	xs = sxe_ipsec_rx_sa_match(ipsec, spi, proto, daddr, *(u8 *)&daddr_len);
	if (!xs) {
		ret = -SXE_ERR_IPSEC_SA_STATE_NOT_EXSIT;
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
	if (sxe_status_err_check(desc, SXE_RXDADV_ERR_IPSEC_AUTH_FAILED)) {
		offload->status = (proto == IPPROTO_ESP) ?
			CRYPTO_TRANSPORT_ESP_AUTH_FAILED : CRYPTO_TRANSPORT_AH_AUTH_FAILED;
	}
	else {
		offload->status = CRYPTO_SUCCESS;
	}

	atomic64_inc(&ipsec->rx_ipsec);

l_out:
	return;
}

static s32 sxe_ipsec_param_valid(struct xfrm_state *xs,
					struct sxe_virtual_context *vt)
{
	s32 ret = -EINVAL;
	struct net_device *dev = xs->xso.dev;
	struct sxe_adapter *adapter = netdev_priv(dev);

	if ((xs->id.proto != IPPROTO_ESP) &&
	   (xs->id.proto != IPPROTO_AH)) {
		LOG_DEV_ERR("unsupport security protol:0x%x.\n",
			  xs->id.proto);
		goto l_out;
	}

	if (xs->xso.flags & XFRM_OFFLOAD_INBOUND) {
		if (xs->calg) {
			LOG_DEV_ERR("unsupport compression offload.\n");
			goto l_out;
		}
	} else {
		if (vt->num_vfs && (vt->bridge_mode != BRIDGE_MODE_VEPA)) {
			LOG_ERROR("num_vfs:%d mode:%d not support ipsec add.\n",
				   vt->num_vfs, vt->bridge_mode);
			goto l_out;
		}
	}

	ret = 0;
	LOG_INFO("num_vfs:%d proto:%d flags:%d ipsec param valid pass\n",
		  vt->num_vfs, xs->id.proto,
		  xs->xso.flags);

l_out:
	return ret;
}

static s32 sxe_ipsec_sa_idx_get(struct sxe_ipsec_context *ipsec, bool is_rx)
{
	s32 ret = -ENOSPC;
	u16 i;

	if (is_rx) {
		if (ipsec->rx_sa_cnt == SXE_IPSEC_SA_CNT_MAX) {
			LOG_ERROR("ipsec rx sa cnt reach limit:%u.\n",
				   SXE_IPSEC_SA_CNT_MAX);
			goto l_out;
		}

		for (i = 0; i < SXE_IPSEC_SA_CNT_MAX; i++) {
			if (!test_and_set_bit(SXE_IPSEC_SA_ENTRY_USED,
				&ipsec->rx_table[i].status)) {
				ret = i;
				break;
			}
		}
	} else {
		if (ipsec->tx_sa_cnt == SXE_IPSEC_SA_CNT_MAX) {
			LOG_ERROR("ipsec tx sa cnt reach limit:%u.\n",
				   SXE_IPSEC_SA_CNT_MAX);
			goto l_out;
		}

		for (i = 0; i < SXE_IPSEC_SA_CNT_MAX; i++) {
			if (!test_and_set_bit(SXE_IPSEC_SA_ENTRY_USED,
				&ipsec->tx_table[i].status)) {
				ret = i;
				break;
			}
		}
	}

l_out:
	return ret;
}

static s32 sxe_ipsec_key_salt_parse(struct xfrm_state *xs,
						u32 *key, u32 *salt)
{
	s32 ret = 0;
	s8 *xs_key;
	unsigned long len;
	struct net_device *dev = xs->xso.dev;
	struct sxe_adapter *adapter = netdev_priv(dev);

	if (!xs->aead) {
		ret = -EINVAL;
		LOG_DEV_ERR("ipsec offload algorithm unsupport.(err:%d)\n", ret);
		goto l_out;
	}

	if (xs->aead->alg_icv_len != SXE_IPSEC_AUTH_BIT_LEN) {
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

	if (len == SXE_IPSEC_KEY_SALT_BIT_LEN) {
		*salt = *(u32 *)(xs_key + SXE_IPSEC_KEY_BYTE_LEN);
	} else if (len == SXE_IPSEC_KEY_BIT_LEN) {
		LOG_DEV_INFO("ipsec hw offload parameters missing 32 bit salt value.\n");
		*salt = 0;
	} else {
		ret = -EINVAL;
		LOG_DEV_ERR("unsupport key_salt len:%lu.(err:%d)\n", len, ret);
		goto l_out;
	}

	memcpy(key, xs_key, sizeof(u32) * SXE_IPSEC_KEY_LEN);

	LOG_INFO("ipsec offload flag:0x%x key_salt len:%lu "
		 "salt:%u key:0x%x%x%x%x.\n",
		 xs->xso.flags ,len,
		 *salt, key[0], key[1], key[2], key[3]);

l_out:
	return ret;
}

static void sxe_ipsec_rx_ip_tbl_sync(struct sxe_hw *hw, u16 idx,
					   __be32 *ip_addr)
{
	hw->sec.ops->ipsec_rx_ip_store(hw, ip_addr, SXE_IPSEC_IP_LEN, idx);

	return;
}

static void sxe_ipsec_rx_sa_tbl_sync(struct sxe_hw *hw, u16 sa_idx,
				__be32 spi, struct sxe_rx_sa *sa)
{
	hw->sec.ops->ipsec_rx_spi_store(hw, spi, sa->ip_idx, sa_idx);

	hw->sec.ops->ipsec_rx_key_store(hw, sa->key, SXE_IPSEC_KEY_LEN,
					sa->salt, sa->mode, sa_idx);

	return;
}

static void sxe_ipsec_rx_key_sync(struct sxe_hw *hw, u16 sa_idx,
						struct sxe_rx_sa *sa)
{
	hw->sec.ops->ipsec_rx_key_store(hw, sa->key, SXE_IPSEC_KEY_LEN,
					sa->salt, sa->mode, sa_idx);

	return;
}

static void sxe_ipsec_tx_sa_tbl_sync(struct sxe_hw *hw, u16 sa_idx,
						struct sxe_tx_sa *sa)
{
	hw->sec.ops->ipsec_tx_key_store(hw, sa->key, SXE_IPSEC_KEY_LEN,
					sa->salt, sa_idx);

	return;
}

static s32 sxe_ipsec_rx_sa_entry_fill(struct xfrm_state *xs,
					struct sxe_rx_sa *sa_entry)
{
	s32 ret;
	struct net_device *net_dev = xs->xso.dev;
	struct sxe_adapter *adapter = netdev_priv(net_dev);

	memset(sa_entry, 0, sizeof(*sa_entry));

	sa_entry->xs = xs;

#ifdef CONFIG_SXE_FPGA_SINGLE_PORT
	sa_entry->ip_idx = 120;
#endif

	if (xs->id.proto == IPPROTO_ESP) {
		sa_entry->decrypt = !!((xs->ealg) || (xs->aead));
	}

	ret = sxe_ipsec_key_salt_parse(xs, sa_entry->key, &sa_entry->salt);
	if (ret) {
		LOG_DEV_ERR("ipsec offload key salt param parse fail.(err:%d)\n",
			   ret);
		goto l_out;
	}

	if (xs->props.family == AF_INET6) {
		memcpy(sa_entry->ip_addr, &xs->id.daddr.a6, SXE_IPV6_ADDR_SIZE);
	} else {
		memcpy(&sa_entry->ip_addr[SXE_IPV4_ADDR_SIZE - 1],
			&xs->id.daddr.a4,
			SXE_IPV4_ADDR_SIZE);
	}

	sa_entry->mode = SXE_IPSEC_RXMOD_VALID;
	if (sa_entry->xs->id.proto == IPPROTO_ESP) {
		sa_entry->mode |= SXE_IPSEC_RXMOD_PROTO_ESP;
	}

	if (sa_entry->decrypt) {
		sa_entry->mode |= SXE_IPSEC_RXMOD_DECRYPT;
	}

	if (sa_entry->xs->props.family == AF_INET6) {
		sa_entry->mode |= SXE_IPSEC_RXMOD_IPV6;
	}

l_out:
	return ret;
}

static s32 sxe_ipsec_tx_sa_entry_fill(struct xfrm_state *xs,
					struct sxe_tx_sa *sa_entry)
{
	s32 ret;

	memset(sa_entry, 0, sizeof(struct sxe_tx_sa));

	sa_entry->xs = xs;

	if (xs->id.proto == IPPROTO_ESP) {
		if ((xs->ealg) || (xs->aead)) {
			sa_entry->encrypt = true;
		}
	}

	ret = sxe_ipsec_key_salt_parse(xs, sa_entry->key, &sa_entry->salt);
	if (ret) {
		LOG_DEV_ERR("ipsec offload key salt param parse fail.(err:%d)\n",
			   ret);
	}

	return ret;
}

static s32 sxe_ipsec_rx_ip_addr_add(struct sxe_ipsec_context *ipsec,
			struct sxe_hw *hw,
			struct sxe_rx_sa *sa_entry)
{
	s32 empty_idx = -1;
	s32 match_idx = -1;
	u16 checked = 0;
	u16 i;
	s32 ret = 0;
	struct sxe_adapter *adapter = hw->adapter;

	for (i = 0; i < SXE_IPSEC_IP_CNT_MAX; i++) {
		if ((checked < ipsec->rx_sa_cnt) || (empty_idx < 0)) {
			if (test_bit(SXE_IPSEC_SA_ENTRY_USED, &ipsec->ip_table[i].status)) {
				if (!memcmp(ipsec->ip_table[i].ip_addr, sa_entry->ip_addr,
					    sizeof(sa_entry->ip_addr))) {
					match_idx = i;
					break;
				}
				checked++;
			} else if (empty_idx < 0) {
				if (!test_and_set_bit(SXE_IPSEC_IP_ENTRY_USED,
					           &ipsec->ip_table[i].status)) {
					empty_idx = i;
				}
			}
		}
	}

	if (ipsec->rx_sa_cnt == 0) {
		empty_idx = 0;
	}

	if (match_idx >= 0) {
		if (empty_idx >= 0) {
			clear_bit(SXE_IPSEC_IP_ENTRY_USED,
				  &ipsec->ip_table[empty_idx].status);
		}
		sa_entry->ip_idx = match_idx;
		ipsec->ip_table[match_idx].ref_cnt++;
		LOG_INFO("ip addr:%pI6 matched ip_table[%d] ref_cnt:%d.\n",
			   sa_entry->ip_addr, match_idx,
			   ipsec->ip_table[match_idx].ref_cnt);
	} else if (empty_idx >= 0) {

#ifdef CONFIG_SXE_FPGA_SINGLE_PORT
		sa_entry->ip_idx += empty_idx;
		if (sa_entry->ip_idx > 127) {
			LOG_ERROR("iptbl_ind %d is too big.\n", sa_entry->ip_idx);
			return -ENOSPC;
		}
#else
		sa_entry->ip_idx = empty_idx;
#endif
		memcpy(ipsec->ip_table[empty_idx].ip_addr, sa_entry->ip_addr,
			sizeof(sa_entry->ip_addr));
		ipsec->ip_table[empty_idx].ref_cnt = 1;

		sxe_ipsec_rx_ip_tbl_sync(hw, empty_idx, sa_entry->ip_addr);

		LOG_INFO("ip addr:%pI6 use new entry:%d.\n",
			  sa_entry->ip_addr, empty_idx);
	} else {
		ret = -ENOSPC;
		LOG_DEV_ERR("ip table full, has no space for new ip addr:%pI6."
			  "(err:%d)\n", sa_entry->ip_addr, ret);
	}

	return ret;
}

static void sxe_ipsec_rx_sa_add(struct sxe_ipsec_context *ipsec,
			struct sxe_hw *hw, struct xfrm_state *xs,
			struct sxe_rx_sa *sa_entry, u16 sa_idx)
{
	memcpy(&ipsec->rx_table[sa_idx], sa_entry, sizeof(struct sxe_rx_sa));

	sxe_ipsec_rx_sa_tbl_sync(hw, sa_idx, xs->id.spi, sa_entry);
	xs->xso.offload_handle = sa_idx + SXE_IPSEC_RX_INDEX_BASE;

	ipsec->rx_sa_cnt++;

	hash_add_rcu(ipsec->rx_table_list, &ipsec->rx_table[sa_idx].hlist,
		     (__force u32)sa_entry->xs->id.spi);

	LOG_INFO("rx_sa_table[%u] add done, rx_sa_cnt:%u.\n",
		   sa_idx, ipsec->rx_sa_cnt);

	return;
}

static void sxe_ipsec_tx_sa_add(struct sxe_ipsec_context *ipsec,
			struct sxe_hw *hw, struct xfrm_state *xs,
			struct sxe_tx_sa *sa_entry, u16 sa_idx)
{
	memcpy(&ipsec->tx_table[sa_idx], sa_entry, sizeof(struct sxe_tx_sa));

	sxe_ipsec_tx_sa_tbl_sync(hw, sa_idx, sa_entry);
	xs->xso.offload_handle = sa_idx + SXE_IPSEC_TX_INDEX_BASE;

	ipsec->tx_sa_cnt++;

	LOG_INFO("tx_sa_table[%u] add done, tx_sa_cnt:%u.\n",
		   sa_idx, ipsec->tx_sa_cnt);

	return;
}

static s32 sxe_ipsec_tx_xs_add(struct sxe_ipsec_context *ipsec,
					     struct sxe_hw *hw,
					     struct xfrm_state *xs)
{
	s32 ret;
	u16 sa_idx;
	struct sxe_tx_sa sa_entry;
	struct sxe_adapter *adapter = hw->adapter;

	ret = sxe_ipsec_sa_idx_get(ipsec, false);
	if (ret < 0) {
		LOG_DEV_ERR("tx sa table no space.(err:%d)\n", ret);
		goto l_out;
	}

	sa_idx = (u16)ret;

	ret = sxe_ipsec_tx_sa_entry_fill(xs, &sa_entry);
	if (ret) {
		LOG_ERROR("ipsec offload  param parse fail.(err:%d)\n", ret);
		goto clear_used_xs;
	}

	sa_entry.status = ipsec->tx_table[sa_idx].status;

	sxe_ipsec_tx_sa_add(ipsec, hw, xs, &sa_entry, sa_idx);

l_out:
	return ret;

clear_used_xs:
	clear_bit(SXE_IPSEC_SA_ENTRY_USED, &ipsec->tx_table[sa_idx].status);

	return ret;

}

static s32 sxe_ipsec_rx_xs_add(struct sxe_ipsec_context *ipsec,
					     struct sxe_hw *hw,
					     struct xfrm_state *xs)
{
	struct sxe_rx_sa sa_entry;
	struct sxe_adapter *adapter = hw->adapter;
	u16 sa_idx;
	s32 ret;

	ret = sxe_ipsec_sa_idx_get(ipsec, true);
	if (ret < 0) {
		LOG_DEV_ERR("rx sa table no space.(err:%d)\n", ret);
		goto l_out;
	}

	sa_idx = (u16)ret;

	ret = sxe_ipsec_rx_sa_entry_fill(xs, &sa_entry);
	if (ret) {
		LOG_ERROR("ipsec offload  param parse fail.(err:%d)\n", ret);
		goto clear_used_xs;
	}

	sa_entry.status = ipsec->rx_table[sa_idx].status;

	ret = sxe_ipsec_rx_ip_addr_add(ipsec, hw, &sa_entry);
	if (ret) {
		LOG_ERROR("ip addr:%pI6 add to ip table fail.(err:%d)\n",
			   sa_entry.ip_addr, ret);
		goto clear_used_xs;
	}

#ifdef CONFIG_SXE_FPGA_SINGLE_PORT
		if ((sa_entry.ip_idx > 127) || (sa_idx > 23)) {
			LOG_ERROR("iptbl_ind %d or sa_idx %d is too big.\n",
				  sa_entry.ip_idx, sa_idx);
			return -ENOSPC;
		}
#endif

	sxe_ipsec_rx_sa_add(ipsec, hw, xs, &sa_entry, sa_idx);

l_out:
	return ret;

clear_used_xs:
	clear_bit(SXE_IPSEC_SA_ENTRY_USED, &ipsec->rx_table[sa_idx].status);

	return ret;
}

static s32 sxe_ipsec_state_add(struct xfrm_state *xs)
{
	s32 ret;
	struct net_device *net_dev = xs->xso.dev;
	struct sxe_adapter *adapter = netdev_priv(net_dev);
	struct sxe_ipsec_context *ipsec = &adapter->ipsec;
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_virtual_context *vt = &adapter->vt_ctxt;

	ret = sxe_ipsec_param_valid(xs, vt);
	if (ret) {
		LOG_ERROR("ipsec offload param invalid.(err:%d)\n",
			  ret);
		goto l_out;
	}

	if (xs->xso.flags & XFRM_OFFLOAD_INBOUND) {
		ret = sxe_ipsec_rx_xs_add(ipsec, hw, xs);
	} else {
		ret = sxe_ipsec_tx_xs_add(ipsec, hw, xs);
	}

	if (ret) {
		LOG_ERROR("ipsec flag:0x%x sa add fail.(err:%d)\n",
			  xs->xso.flags, ret);
		goto l_out;
	}

	if (!(adapter->cap & SXE_IPSEC_ENABLED)) {
		hw->sec.ops->ipsec_engine_start(hw, adapter->link.is_up);
		adapter->cap |= SXE_IPSEC_ENABLED;
		LOG_INFO("ipsec engine started.\n");
	}

l_out:
	return ret;
}

static void sxe_ipsec_rx_sa_entry_clear(struct sxe_rx_sa *sa_entry)
{
	u32 clear_len = sizeof(struct sxe_rx_sa) - sizeof(sa_entry->status);

	memset(sa_entry, 0, clear_len);

	return;
}

static void sxe_ipsec_tx_sa_entry_clear(struct sxe_tx_sa *sa_entry)
{
	u32 clear_len = sizeof(struct sxe_tx_sa) - sizeof(sa_entry->status);

	memset(sa_entry->key, 0, clear_len);

	return;
}

static void sxe_ipsec_rx_xs_del(struct sxe_ipsec_context *ipsec,
					     struct sxe_hw *hw,
					     unsigned long offload_handle)
{
	struct sxe_rx_sa *sa_entry;
	struct sxe_rx_ip *ip_entry;
	struct sxe_adapter *adapter = hw->adapter;
	u16 sa_idx = offload_handle - SXE_IPSEC_RX_INDEX_BASE;
	u8 ip_idx;
	__be32 spi;

	sa_entry = &ipsec->rx_table[sa_idx];

	if (!test_bit(SXE_IPSEC_SA_ENTRY_USED, &sa_entry->status)) {
		LOG_DEV_ERR("invalid sa_idx:%u status:0x%lx not in used.\n",
			  sa_idx, sa_entry->status);
		goto l_out;
	}

	ip_idx = sa_entry->ip_idx;
	spi = sa_entry->xs->id.spi;

	sxe_ipsec_rx_sa_entry_clear(sa_entry);
	sxe_ipsec_rx_sa_tbl_sync(hw, sa_idx, spi, sa_entry);
	hash_del_rcu(&sa_entry->hlist);

	ip_entry = &ipsec->ip_table[ip_idx];
	if (ip_entry->ref_cnt > 0) {
		ip_entry->ref_cnt--;
		if (!ip_entry->ref_cnt) {
			LOG_INFO("del rx_ip_table[%d] ip_addr:%pI6.\n",
				   ip_idx, ip_entry->ip_addr);

			memset(ip_entry->ip_addr, 0, sizeof(ip_entry->ip_addr));
			sxe_ipsec_rx_ip_tbl_sync(hw, ip_idx, ip_entry->ip_addr);
			clear_bit(SXE_IPSEC_IP_ENTRY_USED, &ip_entry->status);
		}
	}

	ipsec->rx_sa_cnt--;
	clear_bit(SXE_IPSEC_SA_ENTRY_USED, &sa_entry->status);

	LOG_INFO("del rx_sa_table[%u] success ip_table[%u] "
		  "ref_cnt:%u rx_sa_cnt:%u.\n",
		   sa_idx, ip_idx,
		   ip_entry->ref_cnt,
		   ipsec->rx_sa_cnt);

l_out:
	return;
}

static void sxe_ipsec_tx_xs_del(struct sxe_ipsec_context *ipsec,
					     struct sxe_hw *hw,
					     unsigned long offload_handle)
{
	struct sxe_tx_sa *sa_entry;
	struct sxe_adapter *adapter = hw->adapter;
	u16 sa_idx = offload_handle - SXE_IPSEC_TX_INDEX_BASE;
	sa_entry = &ipsec->tx_table[sa_idx];

	if (!test_bit(SXE_IPSEC_SA_ENTRY_USED, &sa_entry->status)) {
		LOG_DEV_ERR("invalid sa_idx:%u status:0x%lx not in used.\n",
			  sa_idx, sa_entry->status);
		goto l_out;
	}

	sa_entry = &ipsec->tx_table[sa_idx];
	sxe_ipsec_tx_sa_entry_clear(sa_entry);
	sxe_ipsec_tx_sa_tbl_sync(hw, sa_idx, sa_entry);
	ipsec->tx_sa_cnt--;

	clear_bit(SXE_IPSEC_SA_ENTRY_USED, &sa_entry->status);

	LOG_INFO("del tx_sa_table[%u] success tx_sa_cnt:%u.\n",
		   sa_idx, ipsec->tx_sa_cnt);

l_out:
	return;
}

static void sxe_ipsec_state_delete(struct xfrm_state *xs)
{
	struct net_device *net_dev = xs->xso.dev;
	struct sxe_adapter *adapter = netdev_priv(net_dev);
	struct sxe_ipsec_context *ipsec = &adapter->ipsec;
	struct sxe_hw *hw = &adapter->hw;

	if (xs->xso.flags & XFRM_OFFLOAD_INBOUND) {
		sxe_ipsec_rx_xs_del(ipsec, hw, xs->xso.offload_handle);
	} else {
		sxe_ipsec_tx_xs_del(ipsec, hw, xs->xso.offload_handle);
	}

	if ((ipsec->rx_sa_cnt == 0) && (ipsec->tx_sa_cnt == 0)) {
		adapter->cap &= ~SXE_IPSEC_ENABLED;
		hw->sec.ops->ipsec_engine_stop(hw, adapter->link.is_up);
		LOG_INFO("ipsec engine stopped.\n");
	}

	return;
}

static bool sxe_ipsec_offload_ok(struct sk_buff *skb, struct xfrm_state *xs)
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

static void sxe_ipsec_hw_table_clear(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	u16 idx;
	struct sxe_rx_sa rx_sa_entry = {};
	struct sxe_tx_sa tx_sa_entry = {};

	hw->sec.ops->ipsec_sa_disable(hw);

	for (idx = 0; idx < SXE_IPSEC_SA_CNT_MAX; idx++) {
		sxe_ipsec_rx_key_sync(hw, idx, &rx_sa_entry);

		sxe_ipsec_tx_sa_tbl_sync(hw, idx, &tx_sa_entry);
	}

	return;
}

static const struct xfrmdev_ops sxe_xfrmdev_ops = {
	.xdo_dev_offload_ok = sxe_ipsec_offload_ok,
	.xdo_dev_state_add = sxe_ipsec_state_add,
	.xdo_dev_state_delete = sxe_ipsec_state_delete,
};

void sxe_ipsec_offload_init(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_ipsec_context *ipsec = &adapter->ipsec;
	u32 size;

	if (hw->sec.ops->ipsec_offload_is_disable(hw)) {
		LOG_INFO("ipsec rx/tx offload disable, no need init ipsec.\n");
		goto l_out;
	}

	hash_init(ipsec->rx_table_list);

	size = sizeof(struct sxe_rx_sa) * SXE_IPSEC_SA_CNT_MAX;
	ipsec->rx_table = kzalloc(size, GFP_KERNEL);
	if (ipsec->rx_table == NULL) {
		LOG_DEV_ERR("ipsec rx sa table mem:%uB alloc fail.\n", size);
		goto l_out;
	}

	size = sizeof(struct sxe_rx_ip) * SXE_IPSEC_IP_CNT_MAX;
	ipsec->ip_table = kzalloc(size, GFP_KERNEL);
	if (ipsec->ip_table == NULL) {
		LOG_DEV_ERR("ipsec rx ip table mem:%uB alloc fail.\n", size);
		goto l_free_rx_sa_table;
	}

	size = sizeof(struct sxe_tx_sa) * SXE_IPSEC_SA_CNT_MAX;
	ipsec->tx_table = kzalloc(size, GFP_KERNEL);
	if (ipsec->tx_table == NULL) {
		LOG_DEV_ERR("ipsec tx sa table mem:%uB alloc fail.\n", size);
		goto l_free_rx_ip_table;
	}

	ipsec->rx_sa_cnt = 0;
	ipsec->tx_sa_cnt = 0;

	sxe_ipsec_hw_table_clear(adapter);

	adapter->netdev->xfrmdev_ops = &sxe_xfrmdev_ops;

	LOG_INFO("ipsec init done.\n");

l_out:
	return;

l_free_rx_ip_table:
	SXE_KFREE(ipsec->ip_table);

l_free_rx_sa_table:
	SXE_KFREE(ipsec->rx_table);
	return;
}

void sxe_ipsec_table_restore(struct sxe_adapter *adapter)
{
	struct sxe_ipsec_context *ipsec = &adapter->ipsec;
	struct sxe_hw *hw = &adapter->hw;
	u16 idx;

	if (!(adapter->cap & SXE_IPSEC_ENABLED)) {
		LOG_INFO("sxe ipsec disabled, no need reload.\n");
		goto l_out;
	}

	sxe_ipsec_hw_table_clear(adapter);

	for(idx = 0; idx < SXE_IPSEC_SA_CNT_MAX; idx++) {
		struct sxe_rx_sa *rx_sa_entry = &ipsec->rx_table[idx];
		struct sxe_tx_sa *tx_sa_entry = &ipsec->tx_table[idx];

		if (test_bit(SXE_IPSEC_SA_ENTRY_USED, &rx_sa_entry->status)) {
			if (rx_sa_entry->mode & SXE_IPSEC_RXTXMOD_VF) {
				struct xfrm_state *xs = rx_sa_entry->xs;
				sxe_ipsec_rx_xs_del(ipsec, hw,
					rx_sa_entry->xs->xso.offload_handle);
				SXE_KFREE(xs->aead);
				SXE_KFREE(xs);
			} else {
				sxe_ipsec_rx_sa_tbl_sync(hw, idx,
					rx_sa_entry->xs->id.spi,rx_sa_entry);
			}
		}

		if (test_bit(SXE_IPSEC_SA_ENTRY_USED, &tx_sa_entry->status)) {
			if (tx_sa_entry->mode & SXE_IPSEC_RXTXMOD_VF) {
				struct xfrm_state *xs = tx_sa_entry->xs;
				sxe_ipsec_tx_xs_del(ipsec, hw,
					xs->xso.offload_handle);
				SXE_KFREE(xs->aead);
				SXE_KFREE(xs);
			} else {
				sxe_ipsec_tx_sa_tbl_sync(hw, idx, tx_sa_entry);
			}
		}
	}

	for (idx = 0; idx < SXE_IPSEC_IP_CNT_MAX; idx++) {
		struct sxe_rx_ip *ip_entry = &ipsec->ip_table[idx];

		if (test_bit(SXE_IPSEC_IP_ENTRY_USED, &ip_entry->status)) {
			sxe_ipsec_rx_ip_tbl_sync(hw, idx, ip_entry->ip_addr);
		}
	}

	LOG_INFO("ipsec table reload done.\n");

l_out:
	return;
}

static s32 sxe_vf_ipsec_xs_fill(struct net_device *netdev,
					struct xfrm_state *xs,
					struct sxe_ipsec_add_msg *msg)
{
	s32 ret;
	struct xfrm_algo_desc *algo;
	unsigned long aead_len;

	xs->xso.flags = msg->flags;
	xs->id.spi = msg->spi;
	xs->id.proto = msg->proto;
	xs->props.family = msg->family;

	if (xs->props.family == AF_INET6) {
		memcpy(&xs->id.daddr.a6, msg->ip_addr, sizeof(xs->id.daddr.a6));
	} else {
		memcpy(&xs->id.daddr.a4, msg->ip_addr, sizeof(xs->id.daddr.a4));
	}

	xs->xso.dev = netdev;

	algo = xfrm_aead_get_byname(ipsec_aes_name, SXE_IPSEC_AUTH_BIT_LEN, 1);
	if (unlikely(!algo)) {
		ret = -ENOENT;
		LOG_ERROR("algo desc get fail.(err:%d)\n", ret);
		goto l_out;
	}

	aead_len = sizeof(*xs->aead) + SXE_IPSEC_KEY_SALT_BYTE_LEN;
	xs->aead = kzalloc(aead_len, GFP_KERNEL);
	if (unlikely(!xs->aead)) {
		ret = -ENOMEM;
		LOG_ERROR("algo aead mem:%zu alloc fail.(err:%d)\n",
			   aead_len, ret);
		goto l_out;
	}

	xs->props.ealgo = algo->desc.sadb_alg_id;
	xs->geniv = algo->uinfo.aead.geniv;
	xs->aead->alg_icv_len = SXE_IPSEC_AUTH_BIT_LEN;
	xs->aead->alg_key_len = SXE_IPSEC_KEY_SALT_BIT_LEN;
	memcpy(xs->aead->alg_key, msg->key, sizeof(msg->key));
	memcpy(xs->aead->alg_name, ipsec_aes_name, sizeof(ipsec_aes_name));

	ret = 0;

l_out:
	return ret;
}

s32 sxe_vf_ipsec_add(struct sxe_adapter *adapter, u32 *msg, u8 vf_idx)
{
	struct sxe_ipsec_context *ipsec = &adapter->ipsec;
	struct sxe_ipsec_add_msg *ipsec_msg = (struct sxe_ipsec_add_msg *)msg;
	struct xfrm_state *xs;
	s32 ret;
	u16 sa_idx;
	unsigned long offload_handle;

	if ((!adapter->vt_ctxt.vf_info[vf_idx].trusted) ||
		!(adapter->cap & SXE_VF_IPSEC_ENABLED)) {
		ret = -EACCES;
		LOG_MSG_WARN(drv, "vf_idx:%u trusted:%d cap:0x%x no perm to "
			  "add ipsec.(err:%d)\n",
			  vf_idx,
			  adapter->vt_ctxt.vf_info[vf_idx].trusted,
			  adapter->cap, ret);
		goto l_out;
	}

	if (!(ipsec_msg->flags & XFRM_OFFLOAD_INBOUND)) {
		ret = -EOPNOTSUPP;
		LOG_ERROR("vf_idx:%u just support rx ipsec offload.(err:%d)\n",
			   vf_idx, ret);
		goto l_out;
	}

	xs = kzalloc(sizeof(struct xfrm_state), GFP_KERNEL);
	if (unlikely(!xs)) {
		ret = -ENOMEM;
		LOG_ERROR("vf_idx:%d add ipsec, xs alloc %zuB fail.\n",
			  vf_idx, sizeof(struct xfrm_state));
		goto l_out;
	}

	ret = sxe_vf_ipsec_xs_fill(adapter->netdev, xs, ipsec_msg);
	if (ret) {
		LOG_ERROR("vf_idx:%u ipsec msg fill xfrm xs fail.(err:%d)\n",
			  vf_idx, ret);
		goto xfrm_state_free;
	}

	ret = sxe_ipsec_state_add(xs);
	if (ret) {
		LOG_ERROR("vf_idx:%u ipsec xfrm xs add fail.(err:%d)\n",
			  vf_idx, ret);
		goto xfrm_aead_free;
	}

	offload_handle = xs->xso.offload_handle;
	if (offload_handle < SXE_IPSEC_TX_INDEX_BASE) {
		sa_idx = offload_handle - SXE_IPSEC_RX_INDEX_BASE;
		ipsec->rx_table[sa_idx].vf_idx = vf_idx;
		ipsec->rx_table[sa_idx].mode |= SXE_IPSEC_RXTXMOD_VF;
	} else {
		sa_idx = offload_handle - SXE_IPSEC_TX_INDEX_BASE;
		ipsec->tx_table[sa_idx].vf_idx = vf_idx;
		ipsec->tx_table[sa_idx].mode |= SXE_IPSEC_RXTXMOD_VF;
	}

	ipsec_msg->pf_sa_idx = xs->xso.offload_handle;

l_out:
	return ret;

xfrm_aead_free:
	SXE_KFREE(xs->aead);

xfrm_state_free:
	SXE_KFREE(xs);

	return ret;
}

s32 sxe_vf_ipsec_del(struct sxe_adapter *adapter, u32 *msg, u8 vf_idx)
{
	struct sxe_ipsec_context *ipsec = &adapter->ipsec;
	struct sxe_rx_sa *sa_entry;
	struct xfrm_state *xs;
	struct sxe_ipsec_del_msg *del_msg = (struct sxe_ipsec_del_msg *)msg;
	u32 sa_idx = del_msg->pf_sa_idx - SXE_IPSEC_RX_INDEX_BASE;
	s32 ret = 0;

	if (!adapter->vt_ctxt.vf_info[vf_idx].trusted) {
		ret = -EPERM;
		LOG_MSG_ERR(drv, "vf_idx:%u untrusted, no perm del ipsec xs.(err:%d)\n",
			  vf_idx, ret);
		goto l_out;
	}

	if (sa_idx > SXE_IPSEC_SA_CNT_MAX) {
		ret = -EINVAL;
		LOG_MSG_ERR(drv, "vf_idx:%d ipsec msg sa_idx:%u invalid.(err:%d)\n",
			   vf_idx, sa_idx, ret);
		goto l_out;
	}

	sa_entry = &ipsec->rx_table[sa_idx];
	if (!test_bit(SXE_IPSEC_SA_ENTRY_USED, &sa_entry->status)) {
		LOG_INFO("vf_idx:%u sa_idx:%u not in used.\n",
			 vf_idx, sa_idx);
		goto l_out;
	}

	if (!(sa_entry->mode & SXE_IPSEC_RXTXMOD_VF) ||
		(sa_entry->vf_idx != vf_idx)) {
		ret = -ENOENT;
		LOG_MSG_ERR(drv, "vf_idx:%u sa_idx:%u error.(err:%d)\n",
			  vf_idx, sa_idx, ret);
		goto l_out;
	}

	xs = sa_entry->xs;
	sxe_ipsec_state_delete(sa_entry->xs);

	SXE_KFREE(xs->aead);
	SXE_KFREE(xs);

	LOG_INFO("vf_idx:%u sa_idx:%u del success.\n",
		vf_idx, sa_idx);

l_out:
	return ret;
}

void sxe_vf_ipsec_entry_clear(struct sxe_adapter *adapter, u32 vf_idx)
{
	struct sxe_ipsec_context *ipsec = &adapter->ipsec;
	struct sxe_rx_sa *rx_sa;
	struct sxe_tx_sa *tx_sa;
	u16 idx;

	for (idx = 0; idx < SXE_IPSEC_SA_CNT_MAX; idx++) {
		rx_sa = &ipsec->rx_table[idx];
		tx_sa = &ipsec->tx_table[idx];

		if (test_bit(SXE_IPSEC_SA_ENTRY_USED, &rx_sa->status) &&
		   (rx_sa->mode & SXE_IPSEC_RXTXMOD_VF) &&
		    (rx_sa->vf_idx == vf_idx)) {
		        struct xfrm_state *xs = rx_sa->xs;
			LOG_INFO("del vf_idx:%u ipsec entry:%u.\n", vf_idx, idx);
			sxe_ipsec_state_delete(xs);
			SXE_KFREE(xs->aead);
			SXE_KFREE(xs);
		}

		if (test_bit(SXE_IPSEC_SA_ENTRY_USED, &rx_sa->status) &&
		   (tx_sa->mode & SXE_IPSEC_RXTXMOD_VF) &&
		    (tx_sa->vf_idx == vf_idx)) {
		        struct xfrm_state *xs = tx_sa->xs;
			LOG_INFO("del vf_idx:%u ipsec entry:%u.\n", vf_idx, idx);
			sxe_ipsec_state_delete(xs);
			SXE_KFREE(xs->aead);
			SXE_KFREE(xs);
		}
	}

	return;
}

void sxe_ipsec_offload_exit(struct sxe_adapter *adapter)
{
	struct sxe_ipsec_context *ipsec = &adapter->ipsec;

	SXE_KFREE(ipsec->ip_table);
	SXE_KFREE(ipsec->rx_table);
	SXE_KFREE(ipsec->tx_table);

	LOG_INFO("ipsec exit done.\n");

	return;
}

#endif
