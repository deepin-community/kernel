// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2008 - 2023 Xel Technology. */

#include "xlnid.h"
#include "xlnid_type.h"
#include "xlnid_debug.h"
#include "xlnid_macsec.h"
#include "xlnid_macsec_struct.h"

#include <linux/rtnetlink.h>
#include <linux/rcupdate.h>

#if (defined(NETIF_F_HW_MACSEC) && \
	IS_ENABLED(CONFIG_MACSEC))

enum xlnid_clear_type {
	/* update HW configuration */
	XLNID_CLEAR_HW = BIT(0),
	/* update SW configuration (busy bits, pointers) */
	XLNID_CLEAR_SW = BIT(1),
	/* update both HW and SW configuration */
	XLNID_CLEAR_ALL = XLNID_CLEAR_HW | XLNID_CLEAR_SW,
};

static int xlnid_clear_txsc(struct xlnid_adapter *adapter, const int txsc_idx,
							enum xlnid_clear_type clear_type);
static int xlnid_clear_txsa(struct xlnid_adapter *adapter,
							struct xlnid_macsec_txsc *xlnid_txsc, const int sa_num,
							enum xlnid_clear_type clear_type);
static int xlnid_clear_rxsc(struct xlnid_adapter *adapter, const int rxsc_idx,
							enum xlnid_clear_type clear_type);
static int xlnid_clear_rxsa(struct xlnid_adapter *adapter,
							struct xlnid_macsec_rxsc *xlnid_rxsc, const int sa_num,
							enum xlnid_clear_type clear_type);
static int xlnid_clear_secy(struct xlnid_adapter *adapter,
							const struct macsec_secy *secy, enum xlnid_clear_type clear_type);
static int xlnid_apply_macsec_cfg(struct xlnid_adapter *adapter);
static int xlnid_apply_secy_cfg(struct xlnid_adapter *adapter,
								const struct macsec_secy *secy);
static void xlnid_clear_rx_stats(struct xlnid_hw *hw);
static void xlnid_clear_tx_stats(struct xlnid_hw *hw);

static void xlnid_ether_addr_to_mac(u32 mac[2], const unsigned char *emac)
{
	u32 tmp[2] = { 0 };

	memcpy(((u8 *) tmp) + 2, emac, ETH_ALEN);

	mac[0] = swab32(tmp[1]);
	mac[1] = swab32(tmp[0]);
}

/* There's a 1:1 mapping between SecY and TX SC */
static int xlnid_get_txsc_idx_from_secy(struct xlnid_macsec_cfg *macsec_cfg,
										const struct macsec_secy *secy)
{
	int i;

	if (unlikely(!secy)) {
		return -1;
	}

	for (i = 0; i < XLNID_MACSEC_MAX_SC; i++) {
		if (macsec_cfg->xlnid_txsc[i].sw_secy == secy) {
			return i;
		}
	}

	return -1;
}

static int xlnid_get_rxsc_idx_from_rxsc(struct xlnid_macsec_cfg *macsec_cfg,
										const struct macsec_rx_sc *rxsc)
{
	return 0;
}

/* Rotate keys u32[8] */
static void xlnid_rotate_keys(u32(*key)[8], const int key_len)
{
	u32 tmp[8] = { 0 };

	memcpy(&tmp, key, sizeof(tmp));
	memset(*key, 0, sizeof(*key));

	if (key_len == XLNID_MACSEC_KEY_LEN_128_BIT) {
		(*key)[0] = swab32(tmp[3]);
		(*key)[1] = swab32(tmp[2]);
		(*key)[2] = swab32(tmp[1]);
		(*key)[3] = swab32(tmp[0]);
	}

	else {
		pr_warn("Rotate_keys: invalid key_len\n");
	}
}

#define STATS_2x32_TO_64(stat_field)                                           \
	(((u64)stat_field[1] << 32) | stat_field[0])

static int xlnid_get_macsec_common_stats(struct xlnid_hw *hw,
		struct xlnid_macsec_common_stats *stats, u8 strict)
{
	/* MACSEC RX counters */
	stats->in.ctl_pkts = 0;
	stats->in.tagged_miss_pkts = 0;
	stats->in.untagged_miss_pkts = 0;

	/* if (validateFrames == Strict); ctrl.InPktsNoTag++ */
	if (strict) {
		stats->in.notag_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_UT);
	}

	else {
		stats->in.untagged_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_UT);
	}

	stats->in.bad_tag_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_BAD);
	stats->in.no_sci_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_NOSCI);
	stats->in.unknown_sci_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_UNSCI);
	stats->in.ctrl_prt_pass_pkts = 0;
	stats->in.unctrl_prt_pass_pkts = 0;
	stats->in.ctrl_prt_fail_pkts = 0;
	stats->in.unctrl_prt_fail_pkts = 0;
	stats->in.too_long_pkts = 0;
	stats->in.igpoc_ctl_pkts = 0;
	stats->in.ecc_error_pkts = 0;
	stats->in.unctrl_hit_drop_redir = 0;

	/* MACSEC TX counters */
	stats->out.ctl_pkts = 0;
	stats->out.unknown_sa_pkts = 0;
	stats->out.untagged_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_UT);
	stats->out.too_long = 0;
	stats->out.ecc_error_pkts = 0;
	stats->out.unctrl_hit_drop_redir = 0;

	return 0;
}

static int xlnid_get_rxsa_stats(struct xlnid_hw *hw, const int sa_idx,
								struct xlnid_macsec_rx_sa_stats *stats)
{
	stats->untagged_hit_pkts = 0;
	stats->ctrl_hit_drop_redir_pkts = 0;
	stats->not_using_sa += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_NUSA);
	stats->unused_sa += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_UNSA);
	stats->late_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_LATE);
	stats->delayed_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_DELAY);
	stats->unchecked_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_UC);
	stats->validated_octets += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_OCTP);
	stats->decrypted_octets += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_OCTE);

	if (sa_idx == 0) {
		stats->invalid_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_INV0);
		stats->not_valid_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_NV0);
		stats->ok_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_OK0);
	}

	else if (sa_idx == 1) {
		stats->invalid_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_INV1);
		stats->not_valid_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_NV1);
		stats->ok_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_OK1);
	}

	else {
		return -EINVAL;
	}

	return 0;
}

static int xlnid_get_txsa_stats(struct xlnid_hw *hw, const int sa_idx,
								struct xlnid_macsec_tx_sa_stats *stats)
{
	stats->sa_hit_drop_redirect = 0;
	stats->sa_protected2_pkts = 0;
	stats->sa_protected_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_PKTP);
	stats->sa_encrypted_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_PKTE);

	return 0;
}

static int xlnid_get_txsa_next_pn(struct xlnid_hw *hw, const int sa_idx,
									u32 *pn)
{
	u32 regval = 0;
	u32 pn_shift = 0;

	if (sa_idx == 0) {

		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_PN0);
	}

	else if (sa_idx == 1) {
		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_PN1);
	}

	else {
		return -EINVAL;
	}

	pn_shift = cpu_to_be32(regval);
	*pn = pn_shift;

	return 0;
}

static int xlnid_get_rxsa_next_pn(struct xlnid_hw *hw, const int sa_idx,
									u32 *pn)
{
	u32 regval = 0;
	u32 pn_shift = 0;

	if (sa_idx == 0) {

		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_PN0);
	}

	else if (sa_idx == 1) {
		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_PN1);
	}

	else {
		return -EINVAL;
	}

	pn_shift = cpu_to_be32(regval);
	*pn = pn_shift;

	return 0;
}

static void xlnid_get_txsc_stats(struct xlnid_hw *hw, const int sc_idx,
									struct xlnid_macsec_tx_sc_stats *stats)
{
	stats->sc_protected_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_PKTP);
	stats->sc_encrypted_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_PKTE);
	stats->sc_protected_octets += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_OCTP);
	stats->sc_encrypted_octets += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_OCTE);
}

static int xlnid_mdo_dev_open(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	int ret = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	if (netif_carrier_ok(adapter->netdev)) {
		ret = xlnid_apply_secy_cfg(adapter, ctx->secy);
	}

	return ret;
}

static int xlnid_mdo_dev_stop(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	xlnid_clear_secy(adapter, adapter->macsec_cfg->xlnid_txsc[0].sw_secy,
						XLNID_CLEAR_HW);
	return 0;
}

static int xlnid_set_txsc(struct xlnid_adapter *adapter, const int txsc_idx)
{
	struct xlnid_macsec_txsc *xlnid_txsc =
			&adapter->macsec_cfg->xlnid_txsc[txsc_idx];
	struct xlnid_mss_egress_class_record tx_class_rec = { 0 };
	struct macsec_secy *secy = (struct macsec_secy *)xlnid_txsc->sw_secy;
	struct xlnid_mss_egress_sc_record sc_rec = { 0 };
	unsigned int sc_idx = xlnid_txsc->hw_sc_idx;
	struct xlnid_hw *hw = &adapter->hw;
	u32 regval = 0;

	xlnid_ether_addr_to_mac(tx_class_rec.mac_sa, secy->netdev->dev_addr);

	put_unaligned_be64((__force u64) secy->sci, tx_class_rec.sci);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_SCL, secy->sci);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_SCH, secy->sci >> 32);

	/* westlake only support port 0 */
	secy->sci &= 0x0000FFFFFFFFFFFFULL;

	tx_class_rec.sci_mask = 0;

	tx_class_rec.sa_mask = 0x3f;

	/* forward to encrypt */
	tx_class_rec.action = 0;
	tx_class_rec.valid = 1;

	tx_class_rec.sc_idx = sc_idx;

	/* 1SC 2SA */
	tx_class_rec.sc_sa = 0;

	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_CTL);
	regval &= XLNID_MACSEC_RXCTL_MASK;

	if (secy->validate_frames == MACSEC_VALIDATE_CHECK) {
		regval |= XLNID_MACSEC_RXCTL_RXEN_CHECK;
	}

	else if (secy->validate_frames == MACSEC_VALIDATE_DISABLED) {
		regval |= XLNID_MACSEC_RXCTL_RXEN_DISABLED;
	}

	else {
		regval |= XLNID_MACSEC_RXCTL_RXEN_STRICT;
	}

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_CTL, regval);

	sc_rec.protect = secy->protect_frames;
	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_CTL);
	regval &= XLNID_MACSEC_TXCTL_MASK;

	if (secy->protect_frames) {
		/* Send macsec enable */
		adapter->hw.macsec_enable = true;

		if (secy->tx_sc.encrypt) {
			regval |= XLNID_MACSEC_TXCTL_ENCRYPT;
			sc_rec.tci |= BIT(1);
		}

		else {
			regval |= XLNID_MACSEC_TXCTL_ICV;
		}

		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_CTL, regval);
	}

	else {
		adapter->hw.macsec_enable = false;
	}

	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_CTL);

	if (secy->tx_sc.send_sci) {
		sc_rec.tci |= BIT(3);
		regval |= XLNID_MACSEC_TXCTL_AISCI;
	}

	else {
		sc_rec.tci &= ~BIT(3);
		regval &= ~XLNID_MACSEC_TXCTL_AISCI;
	}

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_CTL, regval);

	/*  The C bit is clear if and only if the Secure Data is
		exactly the same as the User Data and the ICV is 16 octets long.
	*/
	if (!(secy->icv_len == 16 && !secy->tx_sc.encrypt)) {
		sc_rec.tci |= BIT(0);
	}

	sc_rec.an_roll = 0;

	switch (secy->key_len) {
	case XLNID_MACSEC_KEY_LEN_128_BIT:
		/* AES-128 */
		sc_rec.sak_len = 16;
		break;

	default:
		WARN_ONCE(true, "Invalid sc_sa");
		return -EINVAL;
	}

	if (secy->replay_protect) {
		if (secy->replay_window != 0) {
			return -EINVAL;
		}

		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_CTL);
		regval |= XLNID_MACSEC_RXCTL_PLSH;
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_CTL, regval);
	}

	else {
		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_CTL);
		regval &= ~XLNID_MACSEC_RXCTL_PLSH;
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_CTL, regval);
	}

	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_SA);

	if (secy->tx_sc.encoding_sa == 0) {
		regval &= ~XLNID_MACSEC_TXCTL_SA;
		sc_rec.curr_an = 0;
	}

	else if (secy->tx_sc.encoding_sa == 1) {
		regval |= XLNID_MACSEC_TXCTL_SA;
		sc_rec.curr_an = 1;
	}

	else {
		return -EINVAL;
	}

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_SA, regval);

	if (secy->tx_sc.end_station) {
		sc_rec.tci &= ~BIT(4);
		return -EINVAL;
	}

	if (secy->tx_sc.scb) {
		sc_rec.tci &= ~BIT(2);
		return -EINVAL;
	}

	if (secy->icv_len != 16) {
		return -EINVAL;
	}

	sc_rec.valid = 1;
	sc_rec.fresh = 1;

	return 0;
}

static u32 xlnid_sc_idx_max(const enum xlnid_macsec_sc_sa sc_sa)
{
	u32 result = 0;

	switch (sc_sa) {
	case xlnid_macsec_sa_sc_2sa_1sc:
		result = 1;
		break;

	default:
		break;
	}

	return result;
}

static enum xlnid_macsec_sc_sa sc_sa_from_num_an(const int num_an)
{
	enum xlnid_macsec_sc_sa sc_sa = xlnid_macsec_sa_sc_not_used;

	sc_sa = xlnid_macsec_sa_sc_2sa_1sc;

	return sc_sa;
}

static int xlnid_mdo_add_secy(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	const struct macsec_secy *secy = ctx->secy;
	enum xlnid_macsec_sc_sa sc_sa;
	u32 txsc_idx;
	int ret = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)

	if (secy->xpn) {
		return -EOPNOTSUPP;
	}

#endif

	sc_sa = sc_sa_from_num_an(MACSEC_NUM_AN);

	if (sc_sa == xlnid_macsec_sa_sc_not_used) {
		return -EINVAL;
	}

	txsc_idx = 0;

	if (txsc_idx == XLNID_MACSEC_MAX_SC) {
		return -ENOSPC;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	cfg->sc_sa = sc_sa;
	cfg->xlnid_txsc[txsc_idx].hw_sc_idx = 0;
	cfg->xlnid_txsc[txsc_idx].sw_secy = secy;

	if (netif_carrier_ok(adapter->netdev) && netif_running(secy->netdev)) {
		ret = xlnid_set_txsc(adapter, txsc_idx);
	}

	set_bit(txsc_idx, &cfg->txsc_idx_busy);

	return ret;
}

static int xlnid_mdo_upd_secy(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	const struct macsec_secy *secy = ctx->secy;
	int txsc_idx;
	int ret = 0;

	txsc_idx = xlnid_get_txsc_idx_from_secy(adapter->macsec_cfg, secy);

	if (txsc_idx < 0) {
		return -ENOENT;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	if (netif_carrier_ok(adapter->netdev) && netif_running(adapter->netdev)) {
		ret = xlnid_set_txsc(adapter, txsc_idx);
	}

	return ret;
}

static void xlnid_clear_tx_stats(struct xlnid_hw *hw)
{
	int i = 0;
	u32 regval = 0;

	/* tx has 5 pkt-counters */
	for (i = 0; i < 5; i++) {
		regval = XLNID_READ_REG_MAC(hw, (0xc4 + i * 4));
	}
}

static int xlnid_clear_txsc(struct xlnid_adapter *adapter, const int txsc_idx,
							enum xlnid_clear_type clear_type)
{
	struct xlnid_macsec_txsc *tx_sc = &adapter->macsec_cfg->xlnid_txsc[txsc_idx];
	struct xlnid_mss_egress_sc_record sc_rec = { 0 };
	struct xlnid_hw *hw = &adapter->hw;
	int ret = 0;
	int sa_num = 0;
	u32 regval = 0;

	if (!adapter->macsec_cfg) {
		return 0;
	}

	clear_bit(txsc_idx, &adapter->macsec_cfg->txsc_idx_busy);
	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_TX_SA);
	regval &= XLNID_MACSEC_TXCTL_SA;

	if (regval != 0) {
		sa_num = 1;
	}

	ret = xlnid_clear_txsa(adapter, tx_sc, sa_num, clear_type);

	if (ret) {
		return ret;
	}

	if (clear_type & XLNID_CLEAR_HW) {
		xlnid_clear_tx_stats(hw);
		sc_rec.fresh = 1;
	}

	if (clear_type & XLNID_CLEAR_SW) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_CTL,
							XLNID_MACSEC_TXCTL_TXEN_DISABLED);
		adapter->macsec_cfg->xlnid_txsc[txsc_idx].sw_secy = NULL;
	}

	return ret;
}

static int xlnid_mdo_del_secy(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	int ret = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	if (!adapter->macsec_cfg) {
		return 0;
	}

	adapter->hw.macsec_enable = false;
	ret = xlnid_clear_secy(adapter, ctx->secy, XLNID_CLEAR_ALL);

	return ret;
}

static int xlnid_update_txsa(struct xlnid_adapter *adapter,
								const unsigned int sc_idx,
								const struct macsec_secy *secy, const struct macsec_tx_sa *tx_sa,
								const unsigned char *key, const unsigned char an)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
	const u32 next_pn = tx_sa->next_pn;
#else
	const u32 next_pn = tx_sa->next_pn_halves.lower;
#endif
	struct xlnid_mss_egress_sakey_record key_rec;
	const unsigned int sa_idx = an;
	struct xlnid_mss_egress_sa_record sa_rec;
	struct xlnid_hw *hw = &adapter->hw;
	int ret = 0;
	u32 regval = 0;

	memset(&sa_rec, 0, sizeof(sa_rec));
	sa_rec.valid = tx_sa->active;
	sa_rec.fresh = 1;

	regval = cpu_to_be32(next_pn);
	sa_rec.next_pn = next_pn;

	if (an == 0) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_PN0, regval);
	}

	else if (an == 1) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_PN1, regval);
	}

	else {
		return -EINVAL;
	}

	if (!key) {
		return ret;
	}

	memset(&key_rec, 0, sizeof(key_rec));
	memcpy(&key_rec.key, key, secy->key_len);

	xlnid_rotate_keys(&key_rec.key, secy->key_len);

	if (sa_idx == 0) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY0_0, (key_rec.key)[3]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY0_1, (key_rec.key)[2]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY0_2, (key_rec.key)[1]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY0_3, (key_rec.key)[0]);
	}

	else if (sa_idx == 1) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY1_0, (key_rec.key)[3]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY1_1, (key_rec.key)[2]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY1_2, (key_rec.key)[1]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY1_3, (key_rec.key)[0]);
	}

	else {
		return -EINVAL;
	}

	memzero_explicit(&key_rec, sizeof(key_rec));
	return ret;
}

static int xlnid_mdo_add_txsa(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	const struct macsec_secy *secy = ctx->secy;
	struct xlnid_macsec_txsc *xlnid_txsc;
	int txsc_idx = 0;
	int ret = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	xlnid_txsc = &cfg->xlnid_txsc[txsc_idx];
	set_bit(ctx->sa.assoc_num, &xlnid_txsc->tx_sa_idx_busy);

	memcpy(xlnid_txsc->tx_sa_key[ctx->sa.assoc_num], ctx->sa.key, secy->key_len);

	if (netif_carrier_ok(adapter->netdev) && netif_running(secy->netdev)) {
		ret = xlnid_update_txsa(adapter, xlnid_txsc->hw_sc_idx, secy, ctx->sa.tx_sa,
								ctx->sa.key, ctx->sa.assoc_num);
	}

	return ret;
}

static int xlnid_mdo_upd_txsa(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	const struct macsec_secy *secy = ctx->secy;
	struct xlnid_macsec_txsc *xlnid_txsc;
	int txsc_idx = 0;
	int ret = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	xlnid_txsc = &cfg->xlnid_txsc[txsc_idx];

	if (netif_carrier_ok(adapter->netdev) && netif_running(secy->netdev)) {
		ret = xlnid_update_txsa(adapter, xlnid_txsc->hw_sc_idx, secy, ctx->sa.tx_sa,
								NULL, ctx->sa.assoc_num);
	}

	return ret;
}

static int xlnid_clear_txsa(struct xlnid_adapter *adapter,
							struct xlnid_macsec_txsc *xlnid_txsc, const int sa_num,
							enum xlnid_clear_type clear_type)
{
	struct xlnid_hw *hw = &adapter->hw;

	if (!adapter->macsec_cfg) {
		return 0;
	}

	if (clear_type & XLNID_CLEAR_SW) {
		clear_bit(sa_num, &xlnid_txsc->tx_sa_idx_busy);

		if (sa_num == 0) {
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_PN0, 0x0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY0_0, 0x0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY0_1, 0x0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY0_2, 0x0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY0_3, 0x0);
		}

		else if (sa_num == 1) {
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_PN1, 0x0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY1_0, 0x0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY1_1, 0x0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY1_2, 0x0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_KEY1_3, 0x0);
		}

		else {
			return -EINVAL;
		}
	}

	if ((clear_type & XLNID_CLEAR_HW) && netif_carrier_ok(adapter->netdev)) {
		xlnid_clear_tx_stats(hw);
	}

	return 0;
}

static int xlnid_mdo_del_txsa(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	int ret = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	ret = xlnid_clear_txsa(adapter, &cfg->xlnid_txsc[0], ctx->sa.assoc_num,
							XLNID_CLEAR_ALL);

	return ret;
}

static int xlnid_rxsc_validate_frames(struct xlnid_adapter *adapter,
										enum macsec_validation_type validate)
{
	struct xlnid_macsec_common_stats *stats = &adapter->macsec_cfg->stats;
	struct xlnid_hw *hw = &adapter->hw;
	u32 regval = 0;
	int i = 0;

	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_CTL);

	if ((regval & XLNID_MACSEC_RXCTL_RXEN_MASK) == XLNID_MACSEC_RXCTL_RXEN_STRICT) {
		stats->in.notag_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_UT);
	}

	else {
		stats->in.untagged_pkts += XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_UT);
	}

	switch (validate) {
	case MACSEC_VALIDATE_DISABLED:
		regval &= ~XLNID_MACSEC_RXCTL_RXEN_MASK;
		regval |= XLNID_MACSEC_RXCTL_RXEN_DISABLED;
		i = 2;
		break;

	case MACSEC_VALIDATE_CHECK:
		regval &= ~XLNID_MACSEC_RXCTL_RXEN_MASK;
		regval |= XLNID_MACSEC_RXCTL_RXEN_CHECK;
		i = 1;
		break;

	case MACSEC_VALIDATE_STRICT:
		regval &= ~XLNID_MACSEC_RXCTL_RXEN_MASK;
		regval |= XLNID_MACSEC_RXCTL_RXEN_STRICT;
		i = 0;
		break;

	default:
		WARN_ONCE(true, "Invalid validation type");
		break;
	}

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_CTL, regval);
	return i;
}

static int xlnid_set_rxsc(struct xlnid_adapter *adapter, const u32 rxsc_idx)
{
	const struct xlnid_macsec_rxsc *xlnid_rxsc =
			&adapter->macsec_cfg->xlnid_rxsc[rxsc_idx];
	struct xlnid_mss_ingress_preclass_record pre_class_record;
	const struct macsec_rx_sc *rx_sc = xlnid_rxsc->sw_rxsc;
	const struct macsec_secy *secy = xlnid_rxsc->sw_secy;
	const u32 hw_sc_idx = xlnid_rxsc->hw_sc_idx;
	struct xlnid_mss_ingress_sc_record sc_record;
	struct xlnid_hw *hw = &adapter->hw;
	u32 regval = 0;

	memset(&pre_class_record, 0, sizeof(pre_class_record));

	put_unaligned_be64((__force u64) rx_sc->sci, pre_class_record.sci);
	regval = rx_sc->sci >> 32;
	regval &= XLNID_MACSEC_RX_SCI_PORT_MASK;
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SCL, rx_sc->sci);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SCH, regval);

	pre_class_record.sci_mask = 0xff;

	/* match all MACSEC ethertype packets */
	pre_class_record.eth_type = ETH_P_MACSEC;
	pre_class_record.eth_type_mask = 0x3;

	xlnid_ether_addr_to_mac(pre_class_record.mac_sa, (char *)&rx_sc->sci);
	pre_class_record.sa_mask = 0x3f;

	pre_class_record.an_mask = adapter->macsec_cfg->sc_sa;
	pre_class_record.sc_idx = hw_sc_idx;

	/* strip SecTAG & forward for decryption */
	pre_class_record.action = 0x0;
	pre_class_record.valid = 1;

	/* If SCI is absent, then match by SA alone */
	pre_class_record.sci_mask = 0;
	pre_class_record.sci_from_table = 1;

	memset(&sc_record, 0, sizeof(sc_record));

	/* set receive mode */
	sc_record.validate_frames = xlnid_rxsc_validate_frames(adapter,
								secy->validate_frames);

	if (secy->replay_protect) {
		sc_record.replay_protect = 1;
		sc_record.anti_replay_window = 0;
	}

	sc_record.valid = 1;
	sc_record.fresh = 1;

	return 0;
}

static int xlnid_mdo_add_rxsc(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	const u32 rxsc_idx_max = xlnid_sc_idx_max(cfg->sc_sa);
	u32 rxsc_idx;
	int ret = 0;

	if (hweight32(cfg->rxsc_idx_busy) >= rxsc_idx_max) {
		return -ENOSPC;
	}

	rxsc_idx = ffz(cfg->rxsc_idx_busy);

	if (rxsc_idx >= rxsc_idx_max) {
		return -ENOSPC;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	cfg->xlnid_rxsc[rxsc_idx].hw_sc_idx = 0;
	cfg->xlnid_rxsc[rxsc_idx].sw_secy = ctx->secy;
	cfg->xlnid_rxsc[rxsc_idx].sw_rxsc = ctx->rx_sc;

	if (netif_carrier_ok(adapter->netdev) && netif_running(ctx->secy->netdev)) {
		ret = xlnid_set_rxsc(adapter, rxsc_idx);
	}

	if (ret < 0) {
		return ret;
	}

	set_bit(rxsc_idx, &cfg->rxsc_idx_busy);

	return 0;
}

static int xlnid_mdo_upd_rxsc(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	int rxsc_idx;
	int ret = 0;

	rxsc_idx = xlnid_get_rxsc_idx_from_rxsc(adapter->macsec_cfg, ctx->rx_sc);

	if (rxsc_idx < 0) {
		return -ENOENT;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	if (netif_carrier_ok(adapter->netdev) && netif_running(ctx->secy->netdev)) {
		ret = xlnid_set_rxsc(adapter, rxsc_idx);
	}

	return ret;
}

static int xlnid_clear_rxsc(struct xlnid_adapter *adapter, const int rxsc_idx,
							enum xlnid_clear_type clear_type)
{
	struct xlnid_macsec_rxsc *rx_sc = &adapter->macsec_cfg->xlnid_rxsc[rxsc_idx];
	struct xlnid_hw *hw = &adapter->hw;
	int ret = 0;
	int sa_num = 0;
	u32 regval_sa0 = 0;
	u32 regval_sa1 = 0;

	if (!adapter->macsec_cfg) {
		return 0;
	}

	clear_bit(rxsc_idx, &adapter->macsec_cfg->rxsc_idx_busy);

	regval_sa0 = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA0);
	regval_sa1 = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA1);

	if (regval_sa0 & XLNID_MACSEC_RXCTL_SA) {
		sa_num = 0;
	}

	else if (regval_sa1 & XLNID_MACSEC_RXCTL_SA) {
		sa_num = 1;
	}

	else {
		return 0;
	}

	ret = xlnid_clear_rxsa(adapter, rx_sc, sa_num, clear_type);

	if (ret) {
		return ret;
	}

	if (clear_type & XLNID_CLEAR_HW) {
		xlnid_clear_rx_stats(hw);
	}

	if (clear_type & XLNID_CLEAR_SW) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_CTL,
							XLNID_MACSEC_RXCTL_RXEN_DISABLED);
		rx_sc->sw_secy = NULL;
		rx_sc->sw_rxsc = NULL;
	}

	return ret;
}

static int xlnid_mdo_del_rxsc(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	enum xlnid_clear_type clear_type = XLNID_CLEAR_SW;
	int rxsc_idx = 0;
	int ret = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	if (netif_carrier_ok(adapter->netdev)) {
		clear_type = XLNID_CLEAR_ALL;
	}

	ret = xlnid_clear_rxsc(adapter, rxsc_idx, clear_type);

	return ret;
}

static int xlnid_update_rxsa(struct xlnid_adapter *adapter,
								const unsigned int sc_idx,
								const struct macsec_secy *secy, const struct macsec_rx_sa *rx_sa,
								const unsigned char *key, const unsigned char an, bool add_rxsa)
{
	struct xlnid_mss_ingress_sakey_record sa_key_record;
	const u32 next_pn = rx_sa->next_pn_halves.lower;
	struct xlnid_mss_ingress_sa_record sa_record;
	struct xlnid_hw *hw = &adapter->hw;
	const int sa_idx = an;
	u32 regval = 0;
	u32 valid_sa0 = 0;
	u32 valid_sa1 = 0;

	memset(&sa_record, 0, sizeof(sa_record));
	memset(&sa_key_record, 0, sizeof(sa_key_record));

	sa_record.valid = rx_sa->active;
	sa_record.fresh = 1;

	regval = cpu_to_be32(next_pn);
	sa_record.next_pn = next_pn;

	valid_sa0 = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA0);
	valid_sa1 = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA1);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA0, XLNID_MACSEC_RXCTL_SA_INVALID);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA0, XLNID_MACSEC_RXCTL_SA_VALID);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA1, XLNID_MACSEC_RXCTL_SA_INVALID);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA1, XLNID_MACSEC_RXCTL_SA_VALID);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA0, valid_sa0);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA1, valid_sa1);

	if (an == 0) {
		if (rx_sa->active) {
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA0, XLNID_MACSEC_RXCTL_SA_VALID);
		}

		else {
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA0, XLNID_MACSEC_RXCTL_SA_INVALID);
		}

		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_PN0, regval);
	}

	else if (an == 1) {
		if (rx_sa->active) {
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA1, XLNID_MACSEC_RXCTL_SA_VALID);
		}

		else {
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA1, XLNID_MACSEC_RXCTL_SA_INVALID);
		}

		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_PN1, regval);
	}

	else {
		return -EINVAL;
	}

	if (!key) {
		if (add_rxsa) {
			return -1;
		}

		else {
			return 0;
		}
	}

	memcpy(&sa_key_record.key, key, secy->key_len);

	switch (secy->key_len) {
	case XLNID_MACSEC_KEY_LEN_128_BIT:
		sa_key_record.key_len = 0;
		break;

	default:
		return -1;
	}

	xlnid_rotate_keys(&sa_key_record.key, secy->key_len);

	if (sa_idx == 0) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY0_0, (sa_key_record.key)[3]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY0_1, (sa_key_record.key)[2]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY0_2, (sa_key_record.key)[1]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY0_3, (sa_key_record.key)[0]);

		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA0, XLNID_MACSEC_RXCTL_SA);
	}

	else if (sa_idx == 1) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY1_0, (sa_key_record.key)[3]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY1_1, (sa_key_record.key)[2]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY1_2, (sa_key_record.key)[1]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY1_3, (sa_key_record.key)[0]);

		XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA1, XLNID_MACSEC_RXCTL_SA);
	}

	else {
		return -EINVAL;
	}

	memzero_explicit(&sa_key_record, sizeof(sa_key_record));
	return 0;
}

static int xlnid_mdo_add_rxsa(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	const struct macsec_secy *secy = ctx->secy;
	struct xlnid_macsec_rxsc *xlnid_rxsc;
	int rxsc_idx = 0;
	int ret = 0;
	bool add_rxsa = true;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	xlnid_rxsc = &adapter->macsec_cfg->xlnid_rxsc[rxsc_idx];

	set_bit(ctx->sa.assoc_num, &xlnid_rxsc->rx_sa_idx_busy);

	memcpy(xlnid_rxsc->rx_sa_key[ctx->sa.assoc_num], ctx->sa.key, secy->key_len);

	if (netif_carrier_ok(adapter->netdev) && netif_running(secy->netdev)) {
		ret = xlnid_update_rxsa(adapter, xlnid_rxsc->hw_sc_idx, secy, ctx->sa.rx_sa,
								ctx->sa.key, ctx->sa.assoc_num, add_rxsa);
	}

	return ret;
}

static int xlnid_mdo_upd_rxsa(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	const struct macsec_secy *secy = ctx->secy;
	int rxsc_idx = 0;
	int ret = 0;
	bool add_rxsa = false;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	if (netif_carrier_ok(adapter->netdev) && netif_running(secy->netdev)) {
		ret = xlnid_update_rxsa(adapter, cfg->xlnid_rxsc[rxsc_idx].hw_sc_idx, secy,
								ctx->sa.rx_sa, NULL, ctx->sa.assoc_num, add_rxsa);
	}

	return ret;
}

static void xlnid_clear_rx_stats(struct xlnid_hw *hw)
{
	int i = 0;

	/* rx has 17 pkt-counters */
	for (i = 0; i < 17; i++) {
		XLNID_READ_REG_MAC(hw, (0x40 + i * 4));
	}
}

static int xlnid_clear_rxsa(struct xlnid_adapter *adapter,
							struct xlnid_macsec_rxsc *xlnid_rxsc, const int sa_num,
							enum xlnid_clear_type clear_type)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 regval = 0;

	if (clear_type & XLNID_CLEAR_SW) {
		clear_bit(sa_num, &xlnid_rxsc->rx_sa_idx_busy);

		if (sa_num == 0) {
			regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA0);
			regval &= ~XLNID_MACSEC_RXCTL_SA_INVALID;
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA0, regval);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_PN0, 0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY0_0, 0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY0_1, 0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY0_2, 0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY0_3, 0);
		}

		else if (sa_num == 1) {
			regval = XLNID_READ_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA1);
			regval &= ~XLNID_MACSEC_RXCTL_SA_INVALID;
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_SA1, regval);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_PN1, 0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY1_0, 0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY1_1, 0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY1_2, 0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_KEY1_3, 0);
		}

		else {
			return -EINVAL;
		}
	}

	if ((clear_type & XLNID_CLEAR_HW) && netif_carrier_ok(adapter->netdev)) {
		xlnid_clear_rx_stats(hw);
	}

	return 0;
}

static int xlnid_mdo_del_rxsa(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	int rxsc_idx = 0;
	int ret = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	ret = xlnid_clear_rxsa(adapter, &cfg->xlnid_rxsc[rxsc_idx], ctx->sa.assoc_num,
							XLNID_CLEAR_ALL);

	return ret;
}

static int xlnid_mdo_get_dev_stats(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	struct xlnid_macsec_common_stats *stats = &adapter->macsec_cfg->stats;
	struct xlnid_hw *hw = &adapter->hw;
	struct macsec_secy *secy = ctx->secy;
	u8 strict = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	if (secy != NULL && secy->validate_frames == MACSEC_VALIDATE_STRICT) {
		strict = 1;
	}

	xlnid_get_macsec_common_stats(hw, stats, strict);

	ctx->stats.dev_stats->OutPktsUntagged = stats->out.untagged_pkts;
	ctx->stats.dev_stats->InPktsUntagged = stats->in.untagged_pkts;
	ctx->stats.dev_stats->OutPktsTooLong = 0;
	ctx->stats.dev_stats->InPktsNoTag = stats->in.notag_pkts;
	ctx->stats.dev_stats->InPktsBadTag = stats->in.bad_tag_pkts;
	ctx->stats.dev_stats->InPktsUnknownSCI = stats->in.unknown_sci_pkts;
	ctx->stats.dev_stats->InPktsNoSCI = stats->in.no_sci_pkts;
	ctx->stats.dev_stats->InPktsOverrun = 0;

	return 0;
}

static int xlnid_mdo_get_tx_sc_stats(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	struct xlnid_macsec_tx_sc_stats *stats;
	struct xlnid_hw *hw = &adapter->hw;
	struct xlnid_macsec_txsc *xlnid_txsc;
	int txsc_idx;

	txsc_idx = xlnid_get_txsc_idx_from_secy(adapter->macsec_cfg, ctx->secy);

	if (txsc_idx < 0) {
		return -ENOENT;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	xlnid_txsc = &adapter->macsec_cfg->xlnid_txsc[txsc_idx];
	stats = &xlnid_txsc->stats;
	xlnid_get_txsc_stats(hw, xlnid_txsc->hw_sc_idx, stats);

	ctx->stats.tx_sc_stats->OutPktsProtected = stats->sc_protected_pkts;
	ctx->stats.tx_sc_stats->OutPktsEncrypted = stats->sc_encrypted_pkts;
	ctx->stats.tx_sc_stats->OutOctetsProtected = stats->sc_protected_octets;
	ctx->stats.tx_sc_stats->OutOctetsEncrypted = stats->sc_encrypted_octets;

	return 0;
}

static int xlnid_mdo_get_tx_sa_stats(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	struct xlnid_macsec_tx_sa_stats *stats;
	struct xlnid_hw *hw = &adapter->hw;
	const struct macsec_secy *secy;
	struct xlnid_macsec_txsc *xlnid_txsc;
	struct macsec_tx_sa *tx_sa;
	unsigned int sa_idx;
	int txsc_idx;
	u32 next_pn;
	int ret;

	txsc_idx = xlnid_get_txsc_idx_from_secy(cfg, ctx->secy);

	if (txsc_idx < 0) {
		return -EINVAL;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	xlnid_txsc = &cfg->xlnid_txsc[txsc_idx];
	sa_idx = ctx->sa.assoc_num;
	stats = &xlnid_txsc->tx_sa_stats[ctx->sa.assoc_num];
	ret = xlnid_get_txsa_stats(hw, sa_idx, stats);

	if (ret) {
		return ret;
	}

	ctx->stats.tx_sa_stats->OutPktsProtected = stats->sa_protected_pkts;
	ctx->stats.tx_sa_stats->OutPktsEncrypted = stats->sa_encrypted_pkts;

	secy = xlnid_txsc->sw_secy;
	tx_sa = rcu_dereference_bh(secy->tx_sc.sa[ctx->sa.assoc_num]);
	ret = xlnid_get_txsa_next_pn(hw, sa_idx, &next_pn);

	if (ret == 0) {
		spin_lock_bh(&tx_sa->lock);
		tx_sa->next_pn = next_pn;
		spin_unlock_bh(&tx_sa->lock);
	}

	return ret;
}

static int xlnid_mdo_get_rx_sc_stats(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	struct xlnid_macsec_rx_sa_stats *stats;
	struct xlnid_hw *hw = &adapter->hw;
	struct xlnid_macsec_rxsc *xlnid_rxsc;
	unsigned int sa_idx;
	int rxsc_idx;
	int ret = 0;
	int i;

	rxsc_idx = xlnid_get_rxsc_idx_from_rxsc(cfg, ctx->rx_sc);

	if (rxsc_idx < 0) {
		return -ENOENT;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	xlnid_rxsc = &cfg->xlnid_rxsc[rxsc_idx];

	for (i = 0; i < MACSEC_NUM_AN; i++) {
		if (!test_bit(i, &xlnid_rxsc->rx_sa_idx_busy)) {
			continue;
		}

		stats = &xlnid_rxsc->rx_sa_stats[i];
		sa_idx = xlnid_rxsc->hw_sc_idx | i;
		ret = xlnid_get_rxsa_stats(hw, sa_idx, stats);

		if (ret) {
			break;
		}

		ctx->stats.rx_sc_stats->InOctetsValidated += stats->validated_octets;
		ctx->stats.rx_sc_stats->InOctetsDecrypted += stats->decrypted_octets;
		ctx->stats.rx_sc_stats->InPktsUnchecked += stats->unchecked_pkts;
		ctx->stats.rx_sc_stats->InPktsDelayed += stats->delayed_pkts;
		ctx->stats.rx_sc_stats->InPktsOK += stats->ok_pkts;
		ctx->stats.rx_sc_stats->InPktsInvalid += stats->invalid_pkts;
		ctx->stats.rx_sc_stats->InPktsLate += stats->late_pkts;
		ctx->stats.rx_sc_stats->InPktsNotValid += stats->not_valid_pkts;
		ctx->stats.rx_sc_stats->InPktsNotUsingSA += stats->not_using_sa;
		ctx->stats.rx_sc_stats->InPktsUnusedSA += stats->unused_sa;
	}

	return ret;
}

static int xlnid_mdo_get_rx_sa_stats(struct macsec_context *ctx)
{
	struct xlnid_adapter *adapter = netdev_priv(ctx->netdev);
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	struct xlnid_macsec_rx_sa_stats *stats;
	struct xlnid_hw *hw = &adapter->hw;
	struct xlnid_macsec_rxsc *xlnid_rxsc;
	struct macsec_rx_sa *rx_sa;
	unsigned int sa_idx = 0;
	int rxsc_idx = 0;
	u32 next_pn = 0;
	int ret = 0;

	rxsc_idx = xlnid_get_rxsc_idx_from_rxsc(cfg, ctx->rx_sc);

	if (rxsc_idx < 0) {
		return -EINVAL;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))

	if (ctx->prepare) {
		return 0;
	}

#endif

	xlnid_rxsc = &cfg->xlnid_rxsc[rxsc_idx];
	stats = &xlnid_rxsc->rx_sa_stats[ctx->sa.assoc_num];
	sa_idx = xlnid_rxsc->hw_sc_idx | ctx->sa.assoc_num;

	ret = xlnid_get_rxsa_stats(hw, sa_idx, stats);

	if (ret) {
		return ret;
	}

	ctx->stats.rx_sa_stats->InPktsOK = stats->ok_pkts;
	ctx->stats.rx_sa_stats->InPktsInvalid = stats->invalid_pkts;
	ctx->stats.rx_sa_stats->InPktsNotValid = stats->not_valid_pkts;
	ctx->stats.rx_sa_stats->InPktsNotUsingSA = stats->not_using_sa;
	ctx->stats.rx_sa_stats->InPktsUnusedSA = stats->unused_sa;

	rx_sa = rcu_dereference_bh(xlnid_rxsc->sw_rxsc->sa[ctx->sa.assoc_num]);
	ret = xlnid_get_rxsa_next_pn(hw, sa_idx, &next_pn);

	if (ret == 0) {
		spin_lock_bh(&rx_sa->lock);
		rx_sa->next_pn = next_pn;
		spin_unlock_bh(&rx_sa->lock);
	}

	return ret;
}

static int apply_txsc_cfg(struct xlnid_adapter *adapter, const int txsc_idx)
{
	struct xlnid_macsec_txsc *xlnid_txsc =
			&adapter->macsec_cfg->xlnid_txsc[txsc_idx];
	const struct macsec_secy *secy = xlnid_txsc->sw_secy;
	struct macsec_tx_sa *tx_sa;
	int ret = 0;
	int i;

	if (!netif_running(secy->netdev)) {
		return ret;
	}

	ret = xlnid_set_txsc(adapter, txsc_idx);

	if (ret) {
		return ret;
	}

	for (i = 0; i < MACSEC_NUM_AN; i++) {
		tx_sa = rcu_dereference_bh(secy->tx_sc.sa[i]);

		if (tx_sa) {
			ret = xlnid_update_txsa(adapter, xlnid_txsc->hw_sc_idx, secy, tx_sa,
									xlnid_txsc->tx_sa_key[i], i);

			if (ret) {
				return ret;
			}
		}
	}

	return ret;
}

static int apply_rxsc_cfg(struct xlnid_adapter *adapter, const int rxsc_idx)
{
	struct xlnid_macsec_rxsc *xlnid_rxsc =
			&adapter->macsec_cfg->xlnid_rxsc[rxsc_idx];
	const struct macsec_secy *secy = xlnid_rxsc->sw_secy;
	struct macsec_rx_sa *rx_sa;
	int ret = 0;
	int i = 0;
	bool add_rxsa = true;

	if (!netif_running(secy->netdev)) {
		return ret;
	}

	ret = xlnid_set_rxsc(adapter, rxsc_idx);

	if (ret) {
		return ret;
	}

	for (i = 0; i < MACSEC_NUM_AN; i++) {
		rx_sa = rcu_dereference_bh(xlnid_rxsc->sw_rxsc->sa[i]);

		if (rx_sa) {
			ret = xlnid_update_rxsa(adapter, xlnid_rxsc->hw_sc_idx, secy, rx_sa,
									xlnid_rxsc->rx_sa_key[i], i, add_rxsa);

			if (ret) {
				return ret;
			}
		}
	}

	return ret;
}

static int xlnid_clear_secy(struct xlnid_adapter *adapter,
							const struct macsec_secy *secy, enum xlnid_clear_type clear_type)
{
	int txsc_idx = 0;
	int rxsc_idx = 0;
	int ret = 0;
	u32 regval = 0;

	/* disable TX MACsec */
	regval = XLNID_READ_REG_MAC(&adapter->hw, WESTLAKE_MACSEC_TX_CTL);
	regval &= XLNID_MACSEC_TXCTL_TXEN_DISABLED;
	XLNID_WRITE_REG_MAC(&adapter->hw, WESTLAKE_MACSEC_TX_CTL, regval);

	ret = xlnid_clear_txsc(adapter, txsc_idx, clear_type);

	if (ret) {
		return ret;
	}

	/* disable RX MACsec */
	regval = XLNID_READ_REG_MAC(&adapter->hw, WESTLAKE_MACSEC_RX_CTL);
	regval &= XLNID_MACSEC_RXCTL_RXEN_DISABLED;
	XLNID_WRITE_REG_MAC(&adapter->hw, WESTLAKE_MACSEC_RX_CTL, regval);

	ret = xlnid_clear_rxsc(adapter, rxsc_idx, clear_type);

	if (ret) {
		return ret;
	}

	return ret;
}

static int xlnid_apply_secy_cfg(struct xlnid_adapter *adapter,
								const struct macsec_secy *secy)
{
	struct macsec_rx_sc *rx_sc;
	int txsc_idx;
	int rxsc_idx;
	int ret = 0;

	txsc_idx = xlnid_get_txsc_idx_from_secy(adapter->macsec_cfg, secy);

	if (txsc_idx >= 0) {
		apply_txsc_cfg(adapter, txsc_idx);
	}

	for (rx_sc = rcu_dereference_bh(secy->rx_sc); rx_sc
			&& rx_sc->active; rx_sc = rcu_dereference_bh(rx_sc->next)) {
		rxsc_idx = xlnid_get_rxsc_idx_from_rxsc(adapter->macsec_cfg, rx_sc);

		if (unlikely(rxsc_idx < 0)) {
			continue;
		}

		ret = apply_rxsc_cfg(adapter, rxsc_idx);

		if (ret) {
			return ret;
		}
	}

	return ret;
}

static int xlnid_apply_macsec_cfg(struct xlnid_adapter *adapter)
{
	int ret = 0;
	int i = 0;

	for (i = 0; i < XLNID_MACSEC_MAX_SC; i++) {
		if (adapter->macsec_cfg->txsc_idx_busy & BIT(i)) {
			ret = apply_txsc_cfg(adapter, i);

			if (ret) {
				return ret;
			}
		}
	}

	for (i = 0; i < XLNID_MACSEC_MAX_SC; i++) {
		if (adapter->macsec_cfg->rxsc_idx_busy & BIT(i)) {
			ret = apply_rxsc_cfg(adapter, i);

			if (ret) {
				return ret;
			}
		}
	}

	return ret;
}

static int xlnid_sa_from_sa_idx(const enum xlnid_macsec_sc_sa sc_sa,
								const int sa_idx)
{
	switch (sc_sa) {
	case xlnid_macsec_sa_sc_2sa_1sc:
		return sa_idx;

	default:
		WARN_ONCE(true, "Invalid sc_sa");
	}

	return -EINVAL;
}

static int xlnid_sc_idx_from_sa_idx(const enum xlnid_macsec_sc_sa sc_sa,
									const int sa_idx)
{
	switch (sc_sa) {
	case xlnid_macsec_sa_sc_2sa_1sc:
		return 1;

	default:
		WARN_ONCE(true, "Invalid sc_sa");
	}

	return -EINVAL;
}

static void xlnid_check_txsa_expiration(struct xlnid_adapter *adapter)
{
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	struct xlnid_macsec_txsc *xlnid_txsc;
	const struct macsec_secy *secy;
	int sc_idx = 0, txsc_idx = 0;
	enum xlnid_macsec_sc_sa sc_sa;
	struct macsec_tx_sa *tx_sa;
	unsigned char an = 0;
	int i = 0;
	u32 egress_sa_expired = 0;

	sc_sa = cfg->sc_sa;

	for (i = 0; i < XLNID_MACSEC_MAX_SA; i++) {
		if (egress_sa_expired & BIT(i)) {
			an = xlnid_sa_from_sa_idx(sc_sa, i);
			sc_idx = xlnid_sc_idx_from_sa_idx(sc_sa, i);
			txsc_idx = 0;

			if (txsc_idx < 0) {
				continue;
			}

			xlnid_txsc = &cfg->xlnid_txsc[txsc_idx];

			if (!(cfg->txsc_idx_busy & BIT(txsc_idx))) {
				netdev_warn(adapter->netdev, "PN threshold expired on invalid TX SC");
				continue;
			}

			secy = xlnid_txsc->sw_secy;

			if (!netif_running(secy->netdev)) {
				netdev_warn(adapter->netdev, "PN threshold expired on down TX SC");
				continue;
			}

			if (unlikely(!(xlnid_txsc->tx_sa_idx_busy & BIT(an)))) {
				netdev_warn(adapter->netdev, "PN threshold expired on invalid TX SA");
				continue;
			}

			tx_sa = rcu_dereference_bh(secy->tx_sc.sa[an]);
			macsec_pn_wrapped((struct macsec_secy *)secy, tx_sa);
		}
	}
}

const struct macsec_ops xlnid_macsec_ops = {
	.mdo_dev_open = xlnid_mdo_dev_open,
	.mdo_dev_stop = xlnid_mdo_dev_stop,
	.mdo_add_secy = xlnid_mdo_add_secy,
	.mdo_upd_secy = xlnid_mdo_upd_secy,
	.mdo_del_secy = xlnid_mdo_del_secy,
	.mdo_add_rxsc = xlnid_mdo_add_rxsc,
	.mdo_upd_rxsc = xlnid_mdo_upd_rxsc,
	.mdo_del_rxsc = xlnid_mdo_del_rxsc,
	.mdo_add_rxsa = xlnid_mdo_add_rxsa,
	.mdo_upd_rxsa = xlnid_mdo_upd_rxsa,
	.mdo_del_rxsa = xlnid_mdo_del_rxsa,
	.mdo_add_txsa = xlnid_mdo_add_txsa,
	.mdo_upd_txsa = xlnid_mdo_upd_txsa,
	.mdo_del_txsa = xlnid_mdo_del_txsa,
	.mdo_get_dev_stats = xlnid_mdo_get_dev_stats,
	.mdo_get_tx_sc_stats = xlnid_mdo_get_tx_sc_stats,
	.mdo_get_tx_sa_stats = xlnid_mdo_get_tx_sa_stats,
	.mdo_get_rx_sc_stats = xlnid_mdo_get_rx_sc_stats,
	.mdo_get_rx_sa_stats = xlnid_mdo_get_rx_sa_stats,
};

int xlnid_macsec_init(struct xlnid_adapter *adapter)
{
	u32 mask;
	struct xlnid_macsec_cfg *cfg;

	adapter->macsec_cfg = kzalloc(sizeof(*cfg), GFP_KERNEL);

	if (!adapter->macsec_cfg) {
		return -ENOMEM;
	}

	adapter->netdev->macsec_ops = &xlnid_macsec_ops;
	mutex_init(&adapter->macsec_mutex);

	/* macsec do not drop undersize frames */
	mask = XLNID_READ_REG_MAC(&adapter->hw, WESTLAKE_RX_DROP_EN_MASK);
	mask |= XLNID_RX_UNDERSIZE_DROP_MASK;
	XLNID_WRITE_REG_MAC(&adapter->hw, WESTLAKE_RX_DROP_EN_MASK, mask);

	return 0;
}

void xlnid_macsec_free(struct xlnid_adapter *adapter)
{
	if (!adapter->macsec_cfg) {
		return;
	}

	kfree(adapter->macsec_cfg);
	adapter->hw.macsec_enable = false;
	adapter->macsec_cfg = NULL;

}

int xlnid_macsec_enable(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	int ret = 0;
	u32 regval = 0;

	if (!adapter->macsec_cfg) {
		return 0;
	}

	mutex_lock(&adapter->macsec_mutex);

	/* Set PN threshold */
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_TX_CTL,
						XLNID_MACSEC_TXCTL_PN_THRESHOLD);

	/* Set interrupt */
	regval = XLNID_READ_REG_DIRECT(hw, EIMS);
	regval |= XLNID_EIMS_LINKSEC;
	XLNID_WRITE_REG_DIRECT(hw, EIMS, regval);
	XLNID_WRITE_FLUSH(hw);

	ret = xlnid_apply_macsec_cfg(adapter);
	mutex_unlock(&adapter->macsec_mutex);
	return ret;
}

void xlnid_macsec_work(struct xlnid_adapter *adapter)
{
	if (!adapter->macsec_cfg) {
		return;
	}

	if (!netif_carrier_ok(adapter->netdev)) {
		return;
	}

	mutex_lock(&adapter->macsec_mutex);
	xlnid_check_txsa_expiration(adapter);
	mutex_unlock(&adapter->macsec_mutex);
}

int xlnid_macsec_rx_sa_cnt(struct xlnid_adapter *adapter)
{
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	int i = 0;
	int cnt = 0;

	if (!cfg) {
		return 0;
	}

	mutex_lock(&adapter->macsec_mutex);

	for (i = 0; i < XLNID_MACSEC_MAX_SC; i++) {
		if (!test_bit(i, &cfg->rxsc_idx_busy)) {
			continue;
		}

		cnt += hweight_long(cfg->xlnid_rxsc[i].rx_sa_idx_busy);
	}

	mutex_unlock(&adapter->macsec_mutex);
	return cnt;
}

int xlnid_macsec_tx_sc_cnt(struct xlnid_adapter *adapter)
{
	int cnt;

	if (!adapter->macsec_cfg) {
		return 0;
	}

	mutex_lock(&adapter->macsec_mutex);
	cnt = hweight_long(adapter->macsec_cfg->txsc_idx_busy);
	mutex_unlock(&adapter->macsec_mutex);

	return cnt;
}

int xlnid_macsec_tx_sa_cnt(struct xlnid_adapter *adapter)
{
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	int i = 0;
	int cnt = 0;

	if (!cfg) {
		return 0;
	}

	mutex_lock(&adapter->macsec_mutex);

	for (i = 0; i < XLNID_MACSEC_MAX_SC; i++) {
		if (!test_bit(i, &cfg->txsc_idx_busy)) {
			continue;
		}

		cnt += hweight_long(cfg->xlnid_txsc[i].tx_sa_idx_busy);
	}

	mutex_unlock(&adapter->macsec_mutex);
	return cnt;
}

static int xlnid_macsec_update_stats(struct xlnid_adapter *adapter)
{
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	struct xlnid_hw *hw = &adapter->hw;
	struct xlnid_macsec_txsc *xlnid_txsc;
	struct xlnid_macsec_rxsc *xlnid_rxsc;
	const struct macsec_secy *secy = cfg->xlnid_txsc[0].sw_secy;
	int i, sa_idx, assoc_num;
	int ret = 0;
	u8 strict = 0;

	if (secy != NULL && secy->validate_frames == MACSEC_VALIDATE_STRICT) {
		strict = 1;
	}

	xlnid_get_macsec_common_stats(hw, &cfg->stats, strict);

	for (i = 0; i < XLNID_MACSEC_MAX_SC; i++) {
		if (!(cfg->txsc_idx_busy & BIT(i))) {
			continue;
		}

		xlnid_txsc = &cfg->xlnid_txsc[i];

		xlnid_get_txsc_stats(hw, xlnid_txsc->hw_sc_idx, &xlnid_txsc->stats);

		for (assoc_num = 0; assoc_num < MACSEC_NUM_AN; assoc_num++) {
			if (!test_bit(assoc_num, &xlnid_txsc->tx_sa_idx_busy)) {
				continue;
			}

			sa_idx = assoc_num;
			xlnid_get_txsa_stats(hw, sa_idx, &xlnid_txsc->tx_sa_stats[assoc_num]);
		}
	}

	for (i = 0; i < XLNID_MACSEC_MAX_SC; i++) {
		if (!(test_bit(i, &cfg->rxsc_idx_busy))) {
			continue;
		}

		xlnid_rxsc = &cfg->xlnid_rxsc[i];

		for (assoc_num = 0; assoc_num < MACSEC_NUM_AN; assoc_num++) {
			if (!test_bit(assoc_num, &xlnid_rxsc->rx_sa_idx_busy)) {
				continue;
			}

			sa_idx = assoc_num;

			ret = xlnid_get_rxsa_stats(hw, sa_idx, &xlnid_rxsc->rx_sa_stats[assoc_num]);

			if (ret) {
				return ret;
			}
		}
	}

	return ret;
}

u64 *xlnid_macsec_get_stats(struct xlnid_adapter *adapter, u64 *data)
{
	struct xlnid_macsec_cfg *cfg = adapter->macsec_cfg;
	struct xlnid_macsec_common_stats *common_stats;
	struct xlnid_macsec_tx_sc_stats *txsc_stats;
	struct xlnid_macsec_tx_sa_stats *txsa_stats;
	struct xlnid_macsec_rx_sa_stats *rxsa_stats;
	struct xlnid_macsec_txsc *xlnid_txsc;
	struct xlnid_macsec_rxsc *xlnid_rxsc;
	unsigned int assoc_num;
	unsigned int sc_num;
	unsigned int i = 0U;

	if (!cfg) {
		return data;
	}

	mutex_lock(&adapter->macsec_mutex);

	xlnid_macsec_update_stats(adapter);

	common_stats = &cfg->stats;
	data[i] = common_stats->in.ctl_pkts;
	data[++i] = common_stats->in.tagged_miss_pkts;
	data[++i] = common_stats->in.untagged_miss_pkts;
	data[++i] = common_stats->in.notag_pkts;
	data[++i] = common_stats->in.untagged_pkts;
	data[++i] = common_stats->in.bad_tag_pkts;
	data[++i] = common_stats->in.no_sci_pkts;
	data[++i] = common_stats->in.unknown_sci_pkts;
	data[++i] = common_stats->in.ctrl_prt_pass_pkts;
	data[++i] = common_stats->in.unctrl_prt_pass_pkts;
	data[++i] = common_stats->in.ctrl_prt_fail_pkts;
	data[++i] = common_stats->in.unctrl_prt_fail_pkts;
	data[++i] = common_stats->in.too_long_pkts;
	data[++i] = common_stats->in.igpoc_ctl_pkts;
	data[++i] = common_stats->in.ecc_error_pkts;
	data[++i] = common_stats->in.unctrl_hit_drop_redir;
	data[++i] = common_stats->out.ctl_pkts;
	data[++i] = common_stats->out.unknown_sa_pkts;
	data[++i] = common_stats->out.untagged_pkts;
	data[++i] = common_stats->out.too_long;
	data[++i] = common_stats->out.ecc_error_pkts;
	data[++i] = common_stats->out.unctrl_hit_drop_redir;

	for (sc_num = 0; sc_num < XLNID_MACSEC_MAX_SC; sc_num++) {
		if (!(test_bit(sc_num, &cfg->txsc_idx_busy))) {
			continue;
		}

		xlnid_txsc = &cfg->xlnid_txsc[sc_num];
		txsc_stats = &xlnid_txsc->stats;

		data[++i] = txsc_stats->sc_protected_pkts;
		data[++i] = txsc_stats->sc_encrypted_pkts;
		data[++i] = txsc_stats->sc_protected_octets;
		data[++i] = txsc_stats->sc_encrypted_octets;

		for (assoc_num = 0; assoc_num < MACSEC_NUM_AN; assoc_num++) {
			if (!test_bit(assoc_num, &xlnid_txsc->tx_sa_idx_busy)) {
				continue;
			}

			txsa_stats = &xlnid_txsc->tx_sa_stats[assoc_num];

			data[++i] = txsa_stats->sa_hit_drop_redirect;
			data[++i] = txsa_stats->sa_protected2_pkts;
			data[++i] = txsa_stats->sa_protected_pkts;
			data[++i] = txsa_stats->sa_encrypted_pkts;
		}
	}

	for (sc_num = 0; sc_num < XLNID_MACSEC_MAX_SC; sc_num++) {
		if (!(test_bit(sc_num, &cfg->rxsc_idx_busy))) {
			continue;
		}

		xlnid_rxsc = &cfg->xlnid_rxsc[sc_num];

		for (assoc_num = 0; assoc_num < MACSEC_NUM_AN; assoc_num++) {
			if (!test_bit(assoc_num, &xlnid_rxsc->rx_sa_idx_busy)) {
				continue;
			}

			rxsa_stats = &xlnid_rxsc->rx_sa_stats[assoc_num];

			data[++i] = rxsa_stats->untagged_hit_pkts;
			data[++i] = rxsa_stats->ctrl_hit_drop_redir_pkts;
			data[++i] = rxsa_stats->not_using_sa;
			data[++i] = rxsa_stats->unused_sa;
			data[++i] = rxsa_stats->not_valid_pkts;
			data[++i] = rxsa_stats->invalid_pkts;
			data[++i] = rxsa_stats->ok_pkts;
			data[++i] = rxsa_stats->late_pkts;
			data[++i] = rxsa_stats->delayed_pkts;
			data[++i] = rxsa_stats->unchecked_pkts;
			data[++i] = rxsa_stats->validated_octets;
			data[++i] = rxsa_stats->decrypted_octets;
		}
	}

	i++;

	data += i;

	mutex_unlock(&adapter->macsec_mutex);

	return data;
}
#endif
