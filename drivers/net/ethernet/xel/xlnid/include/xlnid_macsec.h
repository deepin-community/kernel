/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2008 - 2023 Xel Technology. */

#ifndef XLNID_MACSEC_H
#define XLNID_MACSEC_H

#include <linux/netdevice.h>
#if IS_ENABLED(CONFIG_MACSEC)

#include "net/macsec.h"

struct xlnid_adapter;

#define XLNID_MACSEC_MAX_SC 1
#define XLNID_MACSEC_MAX_SA 2

enum xlnid_macsec_sc_sa {
	xlnid_macsec_sa_sc_2sa_1sc,
	xlnid_macsec_sa_sc_not_used,
};

struct xlnid_macsec_common_stats {
	/* Ingress Common Counters */
	struct {
		u64 ctl_pkts;
		u64 tagged_miss_pkts;
		u64 untagged_miss_pkts;
		u64 notag_pkts;
		u64 untagged_pkts;
		u64 bad_tag_pkts;
		u64 no_sci_pkts;
		u64 unknown_sci_pkts;
		u64 ctrl_prt_pass_pkts;
		u64 unctrl_prt_pass_pkts;
		u64 ctrl_prt_fail_pkts;
		u64 unctrl_prt_fail_pkts;
		u64 too_long_pkts;
		u64 igpoc_ctl_pkts;
		u64 ecc_error_pkts;
		u64 unctrl_hit_drop_redir;
	} in;

	/* Egress Common Counters */
	struct {
		u64 ctl_pkts;
		u64 unknown_sa_pkts;
		u64 untagged_pkts;
		u64 too_long;
		u64 ecc_error_pkts;
		u64 unctrl_hit_drop_redir;
	} out;
};

/* Ingress SA Counters */
struct xlnid_macsec_rx_sa_stats {
	u64 untagged_hit_pkts;
	u64 ctrl_hit_drop_redir_pkts;
	u64 not_using_sa;
	u64 unused_sa;
	u64 not_valid_pkts;
	u64 invalid_pkts;
	u64 ok_pkts;
	u64 late_pkts;
	u64 delayed_pkts;
	u64 unchecked_pkts;
	u64 validated_octets;
	u64 decrypted_octets;
};

/* Egress SA Counters */
struct xlnid_macsec_tx_sa_stats {
	u64 sa_hit_drop_redirect;
	u64 sa_protected2_pkts;
	u64 sa_protected_pkts;
	u64 sa_encrypted_pkts;
};

/* Egress SC Counters */
struct xlnid_macsec_tx_sc_stats {
	u64 sc_protected_pkts;
	u64 sc_encrypted_pkts;
	u64 sc_protected_octets;
	u64 sc_encrypted_octets;
};

struct xlnid_macsec_txsc {
	u32 hw_sc_idx;
	unsigned long tx_sa_idx_busy;
	const struct macsec_secy *sw_secy;
	u8 tx_sa_key[MACSEC_NUM_AN][MACSEC_MAX_KEY_LEN];
	struct xlnid_macsec_tx_sc_stats stats;
	struct xlnid_macsec_tx_sa_stats tx_sa_stats[MACSEC_NUM_AN];
};

struct xlnid_macsec_rxsc {
	u32 hw_sc_idx;
	unsigned long rx_sa_idx_busy;
	const struct macsec_secy *sw_secy;
	const struct macsec_rx_sc *sw_rxsc;
	u8 rx_sa_key[MACSEC_NUM_AN][MACSEC_MAX_KEY_LEN];
	struct xlnid_macsec_rx_sa_stats rx_sa_stats[MACSEC_NUM_AN];
};

struct xlnid_macsec_cfg {
	enum xlnid_macsec_sc_sa sc_sa;
	/* Egress channel configuration */
	unsigned long txsc_idx_busy;
	struct xlnid_macsec_txsc xlnid_txsc[XLNID_MACSEC_MAX_SC];
	/* Ingress channel configuration */
	unsigned long rxsc_idx_busy;
	struct xlnid_macsec_rxsc xlnid_rxsc[XLNID_MACSEC_MAX_SC];
	/* Statistics / counters */
	struct xlnid_macsec_common_stats stats;
};

#if 0
enum xlnid_macsec_validation_type {
	XLNID_MACSEC_VALIDATE_DISABLED = 0,
	XLNID_MACSEC_VALIDATE_CHECK = 1,
	XLNID_MACSEC_VALIDATE_STRICT = 2,
	__XLNID_MACSEC_VALIDATE_END,
	XLNID_MACSEC_VALIDATE_MAX = __XLNID_MACSEC_VALIDATE_END - 1,
};
#endif

extern const struct macsec_ops xlnid_macsec_ops;

int xlnid_macsec_init(struct xlnid_adapter *nic);
void xlnid_macsec_free(struct xlnid_adapter *nic);
int xlnid_macsec_enable(struct xlnid_adapter *nic);
void xlnid_macsec_work(struct xlnid_adapter *nic);
u64 *xlnid_macsec_get_stats(struct xlnid_adapter *nic, u64 *data);
int xlnid_macsec_rx_sa_cnt(struct xlnid_adapter *nic);
int xlnid_macsec_tx_sc_cnt(struct xlnid_adapter *nic);
int xlnid_macsec_tx_sa_cnt(struct xlnid_adapter *nic);

#endif

#endif /* XLNID_MACSEC_H */
