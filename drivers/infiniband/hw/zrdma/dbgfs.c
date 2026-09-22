// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */
#include "main.h"
#include "icrdma_hw.h"
#include "slib.h"

struct mutex zrdma_debugfs_mutex;
struct dentry *zrdma_debugfs_root;
EXPORT_SYMBOL(zrdma_debugfs_root);

#define ZXDH_NP_PSN_WRAPAROUND_ENABLE_BIT 30
#define ZXDH_NP_PMTU_START_BIT 0
#define ZXDH_NP_PMTU_END_BIT 2
#define SET_32_REG_VAL(rf, reg, offset, var)                                 \
	do {                                                                 \
		u32 tmp = rd32((rf)->sc_dev.hw, (reg)) & ~(offset);          \
		wr32((rf)->sc_dev.hw, (reg), tmp | FIELD_PREP(offset, var)); \
	} while (0)

int read_np_cnp_dscp(struct zxdh_pci_f *rf, u32 *var)
{
	u32 tmp = 0;

	tmp = rd32(rf->sc_dev.hw, RDMA_DCQCN_NP_CNP_DSCP(rf->sc_dev.ep_id));
	*var = FIELD_GET(ZXDH_DCQCN_NP_CNP_DSCP, tmp);
	return 0;
}

int write_np_cnp_dscp(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_NP_CNP_DSCP)
		return -EINVAL;

	SET_32_REG_VAL(rf, RDMA_DCQCN_NP_CNP_DSCP(rf->sc_dev.ep_id), ZXDH_DCQCN_NP_CNP_DSCP, var);
	return 0;
}

int read_np_cnp_prio(struct zxdh_pci_f *rf, u32 *var)
{
	u32 tmp = 0;

	tmp = rd32(rf->sc_dev.hw, RDMA_DCQCN_NP_CNP_PRIO(rf->sc_dev.ep_id));
	*var = FIELD_GET(ZXDH_DCQCN_NP_CNP_PRIO, tmp);
	return 0;
}

int write_np_cnp_prio(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_NP_CNP_PRIO)
		return -EINVAL;

	SET_32_REG_VAL(rf, RDMA_DCQCN_NP_CNP_PRIO(rf->sc_dev.ep_id), ZXDH_DCQCN_NP_CNP_PRIO, var);
	return 0;
}

int read_np_cnp_prio_mode(struct zxdh_pci_f *rf, u32 *var)
{
	u32 tmp = 0;

	tmp = rd32(rf->sc_dev.hw, RDMA_DCQCN_NP_CNP_PRIO_MODE(rf->sc_dev.ep_id));
	*var = FIELD_GET(ZXDH_DCQCN_NP_CNP_PRIO_MODE, tmp);
	return 0;
}

int write_np_cnp_prio_mode(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_NP_CNP_PRIO_MODE)
		return -EINVAL;

	SET_32_REG_VAL(rf, RDMA_DCQCN_NP_CNP_PRIO_MODE(rf->sc_dev.ep_id),
		       ZXDH_DCQCN_NP_CNP_PRIO_MODE, var);
	return 0;
}

int read_np_min_time_between_cnps(struct zxdh_pci_f *rf, u32 *var)
{
	u32 tmp_x = 0;
	u32 tmp_y = 0;

	tmp_x = rd32(rf->sc_dev.hw, RDMA_DCQCN_NP_MIN_TIME_BETWEEN_CNPS_X(rf->sc_dev.ep_id));
	tmp_y = rd32(rf->sc_dev.hw, RDMA_DCQCN_NP_MIN_TIME_BETWEEN_CNPS_Y(rf->sc_dev.ep_id));
	*var = FIELD_GET(ZXDH_DCQCN_NP_MIN_TIME_BETWEEN_CNPS_X, tmp_x) *
	       FIELD_GET(ZXDH_DCQCN_NP_MIN_TIME_BETWEEN_CNPS_Y, tmp_y);
	return 0;
}

int write_np_min_time_between_cnps(struct zxdh_pci_f *rf, u32 var)
{
	u32 y = 0;
	u32 y_ex = 0;
	u16 x = 0;

	if (var > RDMA_FLOW_MAX_NP_MIN_TIME_BETWEEN_CNPS ||
	    (var < RDMA_FLOW_MIN_NP_MIN_TIME_BETWEEN_CNPS)) {
		return -EINVAL;
	}
	y = FIELD_GET(ZXDH_DCQCN_NP_MIN_TIME_BETWEEN_CNPS_Y,
		      rd32(rf->sc_dev.hw,
			   RDMA_DCQCN_NP_MIN_TIME_BETWEEN_CNPS_Y(rf->sc_dev.ep_id)));
	y_ex = FIELD_GET(ZXDH_DCQCN_NP_MIN_TIME_BETWEEN_CNPS_Y_EX,
			 rd32(rf->sc_dev.hw,
			      RDMA_DCQCN_NP_MIN_TIME_BETWEEN_CNPS_Y_EX(rf->sc_dev.ep_id)));
	if (y != RDMA_FLOW_NP_MIN_TIME_BETWEEN_CNPS_Y ||
	    y_ex != RDMA_FLOW_NP_MIN_TIME_BETWEEN_CNPS_Y_EX) {
		return -EPERM;
	}
	x = var / RDMA_FLOW_NP_MIN_TIME_BETWEEN_CNPS_Y;
	SET_32_REG_VAL(rf, RDMA_DCQCN_NP_MIN_TIME_BETWEEN_CNPS_X(rf->sc_dev.ep_id),
		       ZXDH_DCQCN_NP_MIN_TIME_BETWEEN_CNPS_X, x);
	return 0;
}

int read_prg_time_reset(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_RPG_TIME_RESET, var);
}

int write_prg_time_reset(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_PRG_TIME_RESET ||
	    var < RDMA_FLOW_MIN_PRG_TIME_RESET) {
		return -EINVAL;
	}
	return zxdh_mp_dtcm_para_set(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_RPG_TIME_RESET, var);
}

int read_rpg_clamp_tgt_rate(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_CLAMP_TGT_RAGE, var);
}

int write_rpg_clamp_tgt_rate(struct zxdh_pci_f *rf, u32 var)
{
	if (var != 1 && var != 0)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_CLAMP_TGT_RAGE, var);
}

int read_rpg_clamp_tgt_rate_after_time_inc(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_CLAMP_TGT_RATE_AFTER_TIME_INC,
				     var);
}

int write_rpg_clamp_tgt_rate_after_time_inc(struct zxdh_pci_f *rf, u32 var)
{
	if (var != 1 && var != 0)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_CLAMP_TGT_RATE_AFTER_TIME_INC,
				     var);
}

int read_rp_dce_tcp_rtt(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_DCE_TCP_RTT, var);
}

int write_rp_dce_tcp_rtt(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_RP_DCE_TCP_RTT ||
	    var < RDMA_FLOW_MIN_RP_DCE_TCP_RTT) {
		return -EINVAL;
	}
	return zxdh_mp_dtcm_para_set(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_DCE_TCP_RTT, var);
}

int read_dce_tcp_g(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, MCODE_TYPE_DCQCN, E_PARA_DCQCN_DCE_TCP_G,
				     var);
}

int write_dce_tcp_g(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_DCE_TCP_G)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, MCODE_TYPE_DCQCN, E_PARA_DCQCN_DCE_TCP_G,
				     var);
}

int read_rpg_gd(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, MCODE_TYPE_DCQCN, E_PARA_DCQCN_RPG_GD,
				     var);
}

int write_rpg_gd(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_RPG_GD)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, MCODE_TYPE_DCQCN, E_PARA_DCQCN_RPG_GD,
				     var);
}

int read_rpg_initial_alpha_value(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_INITIAL_ALPHA_VALUE, var);
}

int write_rpg_initial_alpha_value(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_RPG_INITIAL_ALPHA_VALUE)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_INITIAL_ALPHA_VALUE, var);
}

int read_rpg_min_dec_fac(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_MIN_DEC_FAC, var);
}

int write_rpg_min_dec_fac(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_RPG_MIN_DEC_FAC)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_MIN_DEC_FAC, var);
}

int read_rpg_threshold(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_RPG_THRESHOLD, var);
}

int write_rpg_threshold(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_RPG_THRESHOLD)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_RPG_THRESHOLD, var);
}

int read_rpg_ratio_increase(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_RPG_RATIO_INCREASE, var);
}

int write_rpg_ratio_increase(struct zxdh_pci_f *rf, u32 var)
{
	if (var != 1 && var != 0)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, rf->mcode_type,
				     E_PARA_DCQCN_RPG_RATIO_INCREASE, var);
}

int read_rpg_ai_ratio(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_RPG_AI_RATIO, var);
}

int write_rpg_ai_ratio(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_RPG_AI_RATIO)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_RPG_AI_RATIO, var);
}

int read_rpg_hai_ratio(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_RPG_HAI_RATIO, var);
}

int write_rpg_hai_ratio(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_RPG_HAI_RATIO)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, MCODE_TYPE_DCQCN,
				     E_PARA_DCQCN_RPG_HAI_RATIO, var);
}

int read_rpg_byte_reset(struct zxdh_pci_f *rf, u32 *var)
{
	u32 tmp = 0;

	tmp = rd32(rf->sc_dev.hw, RDMA_DCQCN_RPG_BYTE_RESET(rf->sc_dev.ep_id));
	*var = FIELD_GET(ZXDH_DCQCN_RPG_BYTE_RESET, tmp);
	return 0;
}

int write_rpg_byte_reset(struct zxdh_pci_f *rf, u32 var)
{
	if (var < RDMA_FLOW_BYTE_RESET_THRESHOLD)
		return -EINVAL;

	SET_32_REG_VAL(rf, RDMA_DCQCN_RPG_BYTE_RESET(rf->sc_dev.ep_id), ZXDH_DCQCN_RPG_BYTE_RESET,
		       var);
	return 0;
}

int read_rpg_ai_rate(struct zxdh_pci_f *rf, u32 *var)
{
	u32 tmp = 0;

	tmp = rd32(rf->sc_dev.hw, RDMA_DCQCN_RPG_AI_RATE(rf->sc_dev.ep_id));
	*var = FIELD_GET(ZXDH_DCQCN_RPG_AI_RATE, tmp);
	return 0;
}

int write_rpg_ai_rate(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_CONTROL_RATE_1G)
		return -EINVAL;

	SET_32_REG_VAL(rf, RDMA_DCQCN_RPG_AI_RATE(rf->sc_dev.ep_id), ZXDH_DCQCN_RPG_AI_RATE, var);
	return 0;
}

int read_rpg_hai_rate(struct zxdh_pci_f *rf, u32 *var)
{
	u32 tmp = 0;

	tmp = rd32(rf->sc_dev.hw, RDMA_DCQCN_RPG_HAI_RATE(rf->sc_dev.ep_id));
	*var = FIELD_GET(ZXDH_DCQCN_RPG_HAI_RATE, tmp);
	return 0;
}

int write_rpg_hai_rate(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_CONTROL_RATE_10G)
		return -EINVAL;

	SET_32_REG_VAL(rf, RDMA_DCQCN_RPG_HAI_RATE(rf->sc_dev.ep_id), ZXDH_DCQCN_RPG_HAI_RATE,
		       var);
	return 0;
}

int read_rpg_max_rate(struct zxdh_pci_f *rf, u32 *var)
{
	u32 tmp = 0;

	tmp = rd32(rf->sc_dev.hw, RDMA_RPG_MAX_RATE(rf->sc_dev.ep_id));
	*var = FIELD_GET(ZXDH_RPG_MAX_RATE, tmp);
	return 0;
}

int write_rpg_max_rate(struct zxdh_pci_f *rf, u32 var)
{
	int ret;
	u32 tmp = 0;

	if (var < RDMA_FLOW_CONTROL_RATE_10M ||
	    var > RDMA_FLOW_CONTROL_RATE_200G) {
		return -EINVAL;
	}
	ret = read_rpg_min_rate(rf, &tmp);
	if (ret || tmp > var)
		return -EINVAL;

	SET_32_REG_VAL(rf, RDMA_RPG_MAX_RATE(rf->sc_dev.ep_id), ZXDH_RPG_MAX_RATE, var);
	return 0;
}

int read_rpg_min_rate(struct zxdh_pci_f *rf, u32 *var)
{
	u32 tmp = 0;

	tmp = rd32(rf->sc_dev.hw, RDMA_RPG_MIN_RATE(rf->sc_dev.ep_id));
	*var = FIELD_GET(ZXDH_RPG_MIN_RATE, tmp);
	return 0;
}

int write_rpg_min_rate(struct zxdh_pci_f *rf, u32 var)
{
	int ret;
	u32 tmp = 0;

	if (var < RDMA_FLOW_CONTROL_RATE_10M ||
	    var > RDMA_FLOW_CONTROL_RATE_200G) {
		return -EINVAL;
	}
	ret = read_rpg_max_rate(rf, &tmp);
	if (ret || tmp < var)
		return -EINVAL;

	SET_32_REG_VAL(rf, RDMA_RPG_MIN_RATE(rf->sc_dev.ep_id), ZXDH_RPG_MIN_RATE, var);
	return 0;
}

struct parameter_t zrdma_dcqcn_params[] = {
	{ "np_cnp_dscp", ZRDMA_DBG_DCQCN_NP_CNP_DSCP, &read_np_cnp_dscp,
	  &write_np_cnp_dscp },
	{ "np_cnp_prio", ZRDMA_DBG_DCQCN_NP_CNP_PRIO, &read_np_cnp_prio,
	  &write_np_cnp_prio },
	{ "np_cnp_prio_mode", ZRDMA_DBG_DCQCN_NP_CNP_PRIO_MODE,
	  &read_np_cnp_prio_mode, &write_np_cnp_prio_mode },
	{ "np_min_time_between_cnps", ZRDMA_DBG_DCQCN_NP_MIN_TIME_BETWEEN_CNPS,
	  &read_np_min_time_between_cnps, &write_np_min_time_between_cnps },
	{ "prg_time_reset", ZRDMA_DBG_DCQCN_PRG_TIME_RESET,
	  &read_prg_time_reset, &write_prg_time_reset },
	{ "clamp_tgt_rate", ZRDMA_DBG_DCQCN_RPG_CLAMP_TGT_RATE,
	  &read_rpg_clamp_tgt_rate, &write_rpg_clamp_tgt_rate },
	{ "clamp_tgt_rate_after_time_inc",
	  ZRDMA_DBG_DCQCN_RPG_CLAMP_TGT_RATE_AFTER_TIME_INC,
	  &read_rpg_clamp_tgt_rate_after_time_inc,
	  &write_rpg_clamp_tgt_rate_after_time_inc },
	{ "dce_tcp_rtt", ZRDMA_DBG_DCQCN_RP_DCE_TCP_RTT, &read_rp_dce_tcp_rtt,
	  &write_rp_dce_tcp_rtt },
	{ "dce_tcp_g", ZRDMA_DBG_DCQCN_DCE_TCP_G, &read_dce_tcp_g,
	  &write_dce_tcp_g },
	{ "rpg_gd", ZRDMA_DBG_DCQCN_RPG_GD, &read_rpg_gd, &write_rpg_gd },
	{ "initial_alpha_value", ZRDMA_DBG_DCQCN_RPG_INITIAL_ALPHA_VALUE,
	  &read_rpg_initial_alpha_value, &write_rpg_initial_alpha_value },
	{ "min_dec_fac", ZRDMA_DBG_DCQCN_RPG_MIN_DEC_FAC, &read_rpg_min_dec_fac,
	  &write_rpg_min_dec_fac },
	{ "rpg_threshold", ZRDMA_DBG_DCQCN_RPG_THRESHOLD, &read_rpg_threshold,
	  &write_rpg_threshold },
	{ "rpg_ratio_increase", ZRDMA_DBG_DCQCN_RPG_RATIO_INCREASE,
	  &read_rpg_ratio_increase, &write_rpg_ratio_increase },
	{ "rpg_ai_ratio", ZRDMA_DBG_DCQCN_RPG_AI_RATIO, &read_rpg_ai_ratio,
	  &write_rpg_ai_ratio },
	{ "rpg_hai_ratio", ZRDMA_DBG_DCQCN_RPG_HAI_RATIO, &read_rpg_hai_ratio,
	  &write_rpg_hai_ratio },
	{ "rpg_byte_reset", ZRDMA_DBG_DCQCN_RPG_BYTE_RESET,
	  &read_rpg_byte_reset, &write_rpg_byte_reset },
	{ "rpg_ai_rate", ZRDMA_DBG_DCQCN_RPG_AI_RATE, &read_rpg_ai_rate,
	  &write_rpg_ai_rate },
	{ "rpg_hai_rate", ZRDMA_DBG_DCQCN_RPG_HAI_RATE, &read_rpg_hai_rate,
	  &write_rpg_hai_rate },
	{ "rpg_max_rate", ZRDMA_DBG_DCQCN_RPG_MAX_RATE, &read_rpg_max_rate,
	  &write_rpg_max_rate },
	{ "rpg_min_rate", ZRDMA_DBG_DCQCN_RPG_MIN_RATE, &read_rpg_min_rate,
	  &write_rpg_min_rate },
};

int read_alpha(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, rf->mcode_type, E_PARA_RTT_ALPHA, var);
}

int write_alpha(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_ALPHA_VALUE || var == 0)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, rf->mcode_type, E_PARA_RTT_ALPHA, var);
}

int read_tlow(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, rf->mcode_type, E_PARA_RTT_TLOW, var);
}

int write_tlow(struct zxdh_pci_f *rf, u32 var)
{
	u32 tmp = 0;
	int ret;

	if (var > RDMA_FLOW_MAX_TLOW_VALUE || var == 0)
		return -EINVAL;

	ret = read_thigh(rf, &tmp);
	if (ret || tmp < var)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, rf->mcode_type, E_PARA_RTT_TLOW, var);
}

int read_thigh(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, rf->mcode_type, E_PARA_RTT_THIGH, var);
}

int write_thigh(struct zxdh_pci_f *rf, u32 var)
{
	u32 tmp = 0;
	int ret;

	if (var > RDMA_FLOW_MAX_THIGH_VALUE || var == 0)
		return -EINVAL;

	ret = read_tlow(rf, &tmp);
	if (ret || tmp > var)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, rf->mcode_type, E_PARA_RTT_THIGH, var);
}

int read_ai_num(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, rf->mcode_type, E_PARA_RTT_AI_NUM,
				     var);
}

int write_ai_num(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_AI_NUM_VALUE || var == 0)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, rf->mcode_type, E_PARA_RTT_AI_NUM,
				     var);
}

int read_thred_gradient(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, rf->mcode_type,
				     E_PARA_RTT_THRED_GRADIENT, var);
}

int write_thred_gradient(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_THRED_GRADIENT)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, rf->mcode_type,
				     E_PARA_RTT_THRED_GRADIENT, var);
}

int read_hai_n(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, rf->mcode_type, E_PARA_RTT_HAI_N, var);
}

int write_hai_n(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_HAI_N_VALUE || var == 0)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, rf->mcode_type, E_PARA_RTT_HAI_N, var);
}

int read_ai_n(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, rf->mcode_type, E_PARA_RTT_AI_N, var);
}

int write_ai_n(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_AI_N_VALUE || var == 0)
		return -EINVAL;

	return zxdh_mp_dtcm_para_set(rf, rf->mcode_type, E_PARA_RTT_AI_N, var);
}

int read_vf_delta(struct zxdh_pci_f *rf, u32 *var)
{
	u32 tmp = 0;

	tmp = rd32(rf->sc_dev.hw, RDMA_RPG_VF_DELTA(rf->sc_dev.ep_id));
	*var = FIELD_GET(ZXDH_RTT_VF_DELTA, tmp);
	return 0;
}

int write_vf_delta(struct zxdh_pci_f *rf, u32 var)
{
	if (var > RDMA_FLOW_MAX_VF_DELTA_VALUE || var == 0)
		return -EINVAL;

	SET_32_REG_VAL(rf, RDMA_RPG_VF_DELTA(rf->sc_dev.ep_id), ZXDH_RTT_VF_DELTA, var);
	return 0;
}

struct parameter_t zrdma_rtt_params[] = {
	{ "alpha", ZRDMA_DBG_RTT_ALPHA, &read_alpha, &write_alpha },
	{ "tlow", ZRDMA_DBG_RTT_TLOW, &read_tlow, &write_tlow },
	{ "thigh", ZRDMA_DBG_RTT_THIGH, &read_thigh, &write_thigh },
	{ "ai_num", ZRDMA_DBG_RTT_AI_NUM, &read_ai_num, &write_ai_num },
	{ "thred_gradient", ZRDMA_DBG_RTT_THRED_GRADIENT, &read_thred_gradient,
	  &write_thred_gradient },
	{ "hai_n", ZRDMA_DBG_RTT_HAI_N, &read_hai_n, &write_hai_n },
	{ "ai_n", ZRDMA_DBG_RTT_AI_N, &read_ai_n, &write_ai_n },
	{ "rpg_max_rate", ZRDMA_DBG_RTT_RPG_MAX_RATE, &read_rpg_max_rate,
	  &write_rpg_max_rate },
	{ "rpg_min_rate", ZRDMA_DBG_RTT_RPG_MIN_RATE, &read_rpg_min_rate,
	  &write_rpg_min_rate },
	{ "delta", ZRDMA_DBG_RTT_VF_DELTA, &read_vf_delta, &write_vf_delta },
};

static bool check_psn_wraparound_enable_input(u32 input, u32 *output)
{
	if (input <= 1) {
		*output = input;
		return false;
	}
	return true;
}

static bool check_pmtu_input(u32 input, u32 *output)
{
	if (input <= IB_MTU_4096) {
		*output = input;
		return false;
	}
	return true;
}

static bool check_flow_label_enable_input(u32 input, u32 *output)
{
	if (input <= 1) {
		*output = input;
		return false;
	}
	return true;
}

int read_psn_wraparound_enable(struct zxdh_pci_f *rf, u32 *var)
{
	u32 glb_cfg_data_1 = 0, np_ret = 0, psn_wraparound_enable = 0;
	struct iidc_core_dev_info *cdev_info;
	dpp_pf_info_t pf_info = { 0 };
	cdev_info = rf->cdev;
	if (!cdev_info) {
		*var = 0;
		pr_err("[zxdh_rdma] read_psn_wraparound_enable: cdev_info is null!\n");
		return -EIO;
	}
	pf_info.vport = cdev_info->vport_id;
	pf_info.slot = cdev_info->slot_id;

	np_ret = dpp_glb_cfg_get_1(&pf_info, &glb_cfg_data_1);
	if (np_ret) {
		*var = 0;
		pr_err("[zxdh_rdma] read_psn_wraparound_enable: query dpp_glb_cfg_get_1 failed!\n");
		return -EIO;
	}
	psn_wraparound_enable = FIELD_GET(
		ZXDH_NP_PSN_WRAPAROUND_PSN_WRAPAROUND_ENABLE, glb_cfg_data_1);
	if (check_psn_wraparound_enable_input(psn_wraparound_enable, var))
		return -EINVAL;

	return 0;
}

int write_psn_wraparound_enable(struct zxdh_pci_f *rf, u32 var)
{

	u32 dpp_glb_cfg_psn_wraparound_enable = 0,
	    np_ret = 0;
	struct iidc_core_dev_info *cdev_info;
	dpp_pf_info_t pf_info = { 0 };
	cdev_info = rf->cdev;
	if (!cdev_info) {
		pr_err("[zxdh_rdma] write_psn_wraparound_enable: cdev_info is null!\n");
		return -EIO;
	}
	pf_info.vport = cdev_info->vport_id;
	pf_info.slot = cdev_info->slot_id;

	if (check_psn_wraparound_enable_input(
		    var, &dpp_glb_cfg_psn_wraparound_enable))
		return -EINVAL;

	np_ret = dpp_pktrx_mcode_glb_cfg_write(
		&pf_info, ZXDH_NP_PSN_WRAPAROUND_ENABLE_BIT,
		ZXDH_NP_PSN_WRAPAROUND_ENABLE_BIT,
		dpp_glb_cfg_psn_wraparound_enable);
	if (np_ret) {
		pr_err("[zxdh_rdma] write_psn_wraparound_enable: query dpp_pktrx_mcode_glb_cfg_write failed!\n");
		return -EIO;
	}
	return 0;
}

int read_pmtu(struct zxdh_pci_f *rf, u32 *var)
{
	u32 glb_cfg_data_1 = 0, np_ret = 0, pmtu_in_table = 0;
	struct iidc_core_dev_info *cdev_info;
	dpp_pf_info_t pf_info = { 0 };
	cdev_info = rf->cdev;
	if (!cdev_info) {
		*var = 0;
		pr_err("[zxdh_rdma] read_pmtu: cdev_info is null!\n");
		return -EIO;
	}
	pf_info.vport = cdev_info->vport_id;
	pf_info.slot = cdev_info->slot_id;

	np_ret = dpp_glb_cfg_get_1(&pf_info, &glb_cfg_data_1);
	if (np_ret) {
		*var = 0;
		pr_err("[zxdh_rdma] read_pmtu: query dpp_glb_cfg_get_1 failed!\n");
		return -EIO;
	}
	pmtu_in_table = FIELD_GET(ZXDH_NP_PMTU, glb_cfg_data_1);
	if (check_pmtu_input(pmtu_in_table, var))
		return -EINVAL;
	return 0;
}

int write_pmtu(struct zxdh_pci_f *rf, u32 var)
{

	u32 dpp_glb_cfg_pmtu = 0, np_ret = 0;
	struct iidc_core_dev_info *cdev_info;
	dpp_pf_info_t pf_info = { 0 };
	cdev_info = rf->cdev;
	if (!cdev_info) {
		pr_err("[zxdh_rdma] write_pmtu: cdev_info is null!\n");
		return -EIO;
	}
	pf_info.vport = cdev_info->vport_id;
	pf_info.slot = cdev_info->slot_id;
	if (check_pmtu_input(var, &dpp_glb_cfg_pmtu))
		return -EINVAL;

	np_ret = dpp_pktrx_mcode_glb_cfg_write(&pf_info, ZXDH_NP_PMTU_START_BIT,
					       ZXDH_NP_PMTU_END_BIT,
					       dpp_glb_cfg_pmtu);
	if (np_ret) {
		pr_err("[zxdh_rdma] write_pmtu: query dpp_pktrx_mcode_glb_cfg_write failed!\n");
		return -EIO;
	}
	return 0;
}

struct parameter_t zrdma_np_psn_wraparound_params[] = {
	{ "psn_wraparound_enable", ZRDMA_DBG_NP_PSN_WRAPAROUND_ENABLE_PARA, &read_psn_wraparound_enable, &write_psn_wraparound_enable },
	{ "pmtu", ZRDMA_DBG_NP_PMTU_PARA, &read_pmtu, &write_pmtu },
};

int zrdma_ib_write_rtt_params(struct zxdh_pci_f *rf, int offset, u32 var)
{
	int ret;

	if (offset >= ZRDMA_DBG_RTT_MAX || offset < 0)
		return -EINVAL;

	ret = zrdma_rtt_params[offset].wfunc(rf, var);
	return ret;
}

int zrdma_ib_read_rtt_params(struct zxdh_pci_f *rf, int offset, u32 *var)
{
	int ret;

	if (offset >= ZRDMA_DBG_RTT_MAX || offset < 0)
		return -EINVAL;

	ret = zrdma_rtt_params[offset].rfunc(rf, var);
	return ret;
}

int zrdma_ib_write_dcqcn_params(struct zxdh_pci_f *rf, int offset, u32 var)
{
	int ret;

	if (offset >= ZRDMA_DBG_DCQCN_MAX || offset < 0)
		return -EINVAL;

	ret = zrdma_dcqcn_params[offset].wfunc(rf, var);
	return ret;
}

int zrdma_ib_read_dcqcn_params(struct zxdh_pci_f *rf, int offset, u32 *var)
{
	int ret;

	if (offset >= ZRDMA_DBG_DCQCN_MAX || offset < 0)
		return -EINVAL;

	ret = zrdma_dcqcn_params[offset].rfunc(rf, var);
	return ret;
}

int zrdma_ib_write_np_psn_wraparound_params(struct zxdh_pci_f *rf, int offset, u32 var)
{
	int ret;
	if (offset >= ZRDMA_DBG_NP_PARA_MAX || offset < 0)
		return -EINVAL;
	ret = zrdma_np_psn_wraparound_params[offset].wfunc(rf, var);
	return ret;
}

int zrdma_ib_read_np_psn_wraparound_params(struct zxdh_pci_f *rf, int offset, u32 *var)
{
	int ret;
	if (offset >= ZRDMA_DBG_NP_PARA_MAX || offset < 0)
		return -EINVAL;
	ret = zrdma_np_psn_wraparound_params[offset].rfunc(rf, var);
	return ret;
}

static ssize_t check_write_param(const char __user *buf, size_t count, u32 *var)
{
	char lbuf[ZRDMA_DEBUGFS_MAX_BUF_LEN] = { 0 };

	if (count > sizeof(lbuf))
		return -EINVAL;
	if (copy_from_user(lbuf, buf, count))
		return -EFAULT;
	lbuf[sizeof(lbuf) - 1] = '\0';
	if (kstrtou32(lbuf, 0, var))
		return -EINVAL;
	return 0;
}

static ssize_t dcqcn_write_param(struct file *filp, const char __user *buf,
				 size_t count, loff_t *pos)

{
	struct zrdma_dbg_param *param = filp->private_data;
	int offset = param->offset;
	int ret;
	u32 var = 0;

	ret = check_write_param(buf, count, &var);
	if (ret)
		return ret;

	ret = zrdma_ib_write_dcqcn_params(param->dev, offset, var);
	return ret ? ret : count;
}

static ssize_t dcqcn_read_param(struct file *filp, char __user *buf,
				size_t count, loff_t *pos)
{
	struct zrdma_dbg_param *param = filp->private_data;
	int offset = param->offset;
	u32 var = 0;
	int ret;
	char lbuf[ZRDMA_DEBUGFS_MAX_BUF_LEN] = { 0 };

	ret = zrdma_ib_read_dcqcn_params(param->dev, offset, &var);
	if (ret)
		return ret;

	ret = snprintf(lbuf, sizeof(lbuf), "%d\n", var);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(buf, count, pos, lbuf, ret);
}

static ssize_t np_psn_wraparound_params_write_param(struct file *filp, const char __user *buf,
				 size_t count, loff_t *pos)

{
	struct zrdma_dbg_param *param = filp->private_data;
	int offset = param->offset;
	int ret;
	u32 var = 0;
	ret = check_write_param(buf, count, &var);
	if (ret)
		return ret;
	ret = zrdma_ib_write_np_psn_wraparound_params(param->dev, offset, var);
	return ret ? ret : count;
}

static ssize_t np_psn_wraparound_params_read_param(struct file *filp, char __user *buf,
				size_t count, loff_t *pos)
{
	struct zrdma_dbg_param *param = filp->private_data;
	int offset = param->offset;
	u32 var = 0;
	int ret;
	char lbuf[ZRDMA_DEBUGFS_MAX_BUF_LEN] = { 0 };
	ret = zrdma_ib_read_np_psn_wraparound_params(param->dev, offset, &var);
	if (ret)
		return ret;
	ret = snprintf(lbuf, sizeof(lbuf), "%d\n", var);
	if (ret < 0)
		return ret;
	return simple_read_from_buffer(buf, count, pos, lbuf, ret);
}

static ssize_t rtt_write_param(struct file *filp, const char __user *buf,
			       size_t count, loff_t *pos)

{
	struct zrdma_dbg_param *param = filp->private_data;
	int offset = param->offset;
	int ret;
	u32 var = 0;

	ret = check_write_param(buf, count, &var);
	if (ret)
		return ret;

	ret = zrdma_ib_write_rtt_params(param->dev, offset, var);
	return ret ? ret : count;
}

static ssize_t rtt_read_param(struct file *filp, char __user *buf, size_t count,
			      loff_t *pos)
{
	struct zrdma_dbg_param *param = filp->private_data;
	int offset = param->offset;
	u32 var = 0;
	int ret;
	char lbuf[ZRDMA_DEBUGFS_MAX_BUF_LEN] = { 0 };

	ret = zrdma_ib_read_rtt_params(param->dev, offset, &var);
	if (ret)
		return ret;

	ret = snprintf(lbuf, sizeof(lbuf), "%d\n", var);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(buf, count, pos, lbuf, ret);
}

static ssize_t flow_label_write_param(struct file *filp, const char __user *buf,
			       size_t count, loff_t *pos)
{
	struct zxdh_pci_f *rf = filp->private_data;
	int ret;
	u32 flow_label_enable, var = 0;

	ret = check_write_param(buf, count, &var);
	if (ret)
		return ret;

	if (check_flow_label_enable_input(var, &flow_label_enable))
		return -EINVAL;

	rf->iwdev->flowlabel_enable = flow_label_enable;

	return count;
}

static ssize_t flow_label_read_param(struct file *filp, char __user *buf, size_t count,
			      loff_t *pos)
{
	struct zxdh_pci_f *rf = filp->private_data;
	int ret;
	char lbuf[ZRDMA_DEBUGFS_MAX_BUF_LEN] = { 0 };

	ret = zte_snprintf_s(lbuf, sizeof(lbuf), "%d\n", rf->iwdev->flowlabel_enable);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(buf, count, pos, lbuf, ret);
}

static const struct file_operations dbg_dcqcn_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = dcqcn_write_param,
	.read = dcqcn_read_param,
};

static const struct file_operations dbg_rtt_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = rtt_write_param,
	.read = rtt_read_param,
};

static const struct file_operations dbg_np_psn_wraparound_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = np_psn_wraparound_params_write_param,
	.read = np_psn_wraparound_params_read_param,
};

static const struct file_operations flow_label_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = flow_label_write_param,
	.read = flow_label_read_param,
};

void zrdma_cleanup_np_psn_wraparound_params_debugfs_entry(struct zxdh_pci_f *rf)
{
	int ret;
	struct dentry *dentry_pci_board_bdf = NULL;
	struct dentry *dentry = NULL;
	char pci_board_bdf[64] = { 0 };

	if (!rf) {
		pr_info("[zxdh_rdma] zrdma_cleanup_debugfs rf is null\n");
		return;
	}
	if (!zrdma_debugfs_root)
		return;
	if (rf->debugfs_entry.board_root && !rf->ftype) {
		mutex_lock(&zrdma_debugfs_mutex);
		ret = get_pci_board_bdf(pci_board_bdf, rf);
		if (!ret) {
			dentry_pci_board_bdf = debugfs_lookup(
				pci_board_bdf, zrdma_debugfs_root);
			if (dentry_pci_board_bdf) {
				dentry = debugfs_lookup(
					ZRDMA_DEBUGFS_NP_PSN_WRAPAROUND,
					dentry_pci_board_bdf);
				if (dentry) {
					debugfs_remove_recursive(dentry);
					rf->debugfs_entry
						.board_np_psn_wraparound_root =
						NULL;
				} else {
					rf->debugfs_entry
						.board_np_psn_wraparound_root =
						NULL;
				}
			}
		}
		mutex_unlock(&zrdma_debugfs_mutex);
	}
	if (rf->debugfs_entry.board_params.board_np_psn_wraparound_params && !rf->ftype) {
		kfree(rf->debugfs_entry.board_params.board_np_psn_wraparound_params);
		rf->debugfs_entry.board_params.board_np_psn_wraparound_params = NULL;
	}
}

static void cleanup_board_debugfs_entry(struct zxdh_pci_f *rf, int type, char *pci_board_bdf) {
	const char *sub_dir = NULL;
	struct dentry **board_mcode_root_ptr = NULL;
	struct dentry *dentry_pci_board_bdf = NULL;
	struct dentry *dentry = NULL;

	if (type == MCODE_TYPE_DCQCN) {
		sub_dir = ZRDMA_DEBUGFS_DCQCN_DIR;
		board_mcode_root_ptr = &rf->debugfs_entry.board_dcqcn_root;
	} else if (type == MCODE_TYPE_WUMENG) {
		sub_dir = ZRDMA_DEBUGFS_WUMENG_DIR;
		board_mcode_root_ptr = &rf->debugfs_entry.board_dcqcn_root;
	} else if (type == MCODE_TYPE_PID) {
		sub_dir = ZRDMA_DEBUGFS_PID_DIR;
		board_mcode_root_ptr = &rf->debugfs_entry.board_pid_root;
	} else if (type == MCODE_TYPE_RTT) {
		sub_dir = ZRDMA_DEBUGFS_RTT_DIR;
		board_mcode_root_ptr = &rf->debugfs_entry.board_rtt_root;
	} else {
		return;
	}

	dentry_pci_board_bdf = debugfs_lookup(pci_board_bdf, zrdma_debugfs_root);
	if (dentry_pci_board_bdf) {
		dentry = debugfs_lookup(sub_dir, dentry_pci_board_bdf);
		if (dentry) {
			debugfs_remove_recursive(dentry);
		}
		*board_mcode_root_ptr = NULL;
	}
}

void zrdma_cleanup_mcode_type_debugfs_entry(struct zxdh_pci_f *rf, int type)
{
	int ret;
	char pci_board_bdf[64] = { 0 };

	if (!rf) {
		pr_info("[zxdh_rdma] zrdma_cleanup_debugfs rf is null\n");
		return;
	}
	if (!zrdma_debugfs_root || !rf->debugfs_entry.vhca_root)
		return;
	if (rf->debugfs_entry.board_root && !rf->ftype) {
		mutex_lock(&zrdma_debugfs_mutex);
		ret = get_pci_board_bdf(pci_board_bdf, rf);
		if (!ret) {
			cleanup_board_debugfs_entry(rf, type, pci_board_bdf);
		}

		mutex_unlock(&zrdma_debugfs_mutex);
	}
	debugfs_remove_recursive(rf->debugfs_entry.vhca_root);
	rf->debugfs_entry.vhca_root = NULL;
	if (rf->debugfs_entry.board_params.mcode_board_params.base && !rf->ftype) {
		kfree(rf->debugfs_entry.board_params.mcode_board_params.base);
		rf->debugfs_entry.board_params.mcode_board_params.base = NULL;
	}
	if (rf->debugfs_entry.vhca_params.mcode_vhca_params.base) {
		kfree(rf->debugfs_entry.vhca_params.mcode_vhca_params.base);
		rf->debugfs_entry.vhca_params.mcode_vhca_params.base = NULL;
	}
}

void zrdma_cleanup_debugfs_entry(struct zxdh_pci_f *rf)
{
	int ret;
	struct dentry *dentry = NULL;
	char pci_board_bdf[64] = { 0 };

	if (!rf) {
		pr_info("[zxdh_rdma] zrdma_cleanup_debugfs rf is null\n");
		return;
	}
	if (!zrdma_debugfs_root || !rf->debugfs_entry.vhca_root)
		return;
	if (rf->debugfs_entry.board_root && !rf->ftype) {
		mutex_lock(&zrdma_debugfs_mutex);
		ret = get_pci_board_bdf(pci_board_bdf, rf);
		if (!ret) {
			dentry = debugfs_lookup(pci_board_bdf,
						zrdma_debugfs_root);
			if (dentry) {
				debugfs_remove_recursive(dentry);
				rf->debugfs_entry.board_root = NULL;
			} else {
				rf->debugfs_entry.board_root = NULL;
			}
		}
		mutex_unlock(&zrdma_debugfs_mutex);
	}
	debugfs_remove_recursive(rf->debugfs_entry.vhca_root);
	rf->debugfs_entry.vhca_root = NULL;
	if (rf->debugfs_entry.board_params.mcode_board_params.base && !rf->ftype) {
		kfree(rf->debugfs_entry.board_params.mcode_board_params.base);
		rf->debugfs_entry.board_params.mcode_board_params.base = NULL;
	}
	if (rf->debugfs_entry.board_params.board_np_psn_wraparound_params && !rf->ftype) {
		kfree(rf->debugfs_entry.board_params.board_np_psn_wraparound_params);
		rf->debugfs_entry.board_params.board_np_psn_wraparound_params = NULL;
	}
	if (rf->debugfs_entry.vhca_params.mcode_vhca_params.base) {
		kfree(rf->debugfs_entry.vhca_params.mcode_vhca_params.base);
		rf->debugfs_entry.vhca_params.mcode_vhca_params.base = NULL;
	}
}

int zrdma_create_board_root_debugfs(struct zxdh_pci_f *rf, const char *pci_bdf, enum zrdma_debugfs_mode mode)
{
	struct dentry *dentry;

	switch (mode) {
	case ZRDMA_DEBUGFS_MODE_NORMAL:
		if (!rf->debugfs_entry.board_root && !rf->ftype) {
			dentry = debugfs_lookup(pci_bdf, zrdma_debugfs_root);
			if (!dentry) {
				rf->debugfs_entry.board_root =
					debugfs_create_dir(pci_bdf, zrdma_debugfs_root);
			} else {
				rf->debugfs_entry.board_root = dentry;
			}
		}
		break;
	case ZRDMA_DEBUGFS_MODE_BOND:
		dentry = debugfs_lookup(pci_bdf, zrdma_debugfs_root);
		if (!dentry) {
			rf->debugfs_entry.board_root =
				debugfs_create_dir(pci_bdf, zrdma_debugfs_root);
		} else {
			rf->debugfs_entry.board_root = dentry;
		}
		break;
	default:
		break;
	}

	return 0;
}

int zrdma_create_board_subdir_debugfs(struct zxdh_pci_f *rf, 
					    const char *subdir_name,
					    struct dentry **board_subdir_ptr,
					    int (*create_file_func)(struct zxdh_pci_f *),
					    enum zrdma_debugfs_mode mode)
{
	struct dentry *dentry;
	int ret;

	switch (mode) {
	case ZRDMA_DEBUGFS_MODE_NORMAL:
		if (!*board_subdir_ptr && !rf->ftype) {
			dentry = debugfs_lookup(subdir_name, rf->debugfs_entry.board_root);
			if (!dentry) {
				*board_subdir_ptr = debugfs_create_dir(subdir_name,
								      rf->debugfs_entry.board_root);
				ret = create_file_func(rf);
				if (ret) {
					return ret;
				}
			} else {
				*board_subdir_ptr = dentry;
			}
		}
		break;
	case ZRDMA_DEBUGFS_MODE_BOND:
		dentry = debugfs_lookup(subdir_name, rf->debugfs_entry.board_root);
		if (!dentry) {
			*board_subdir_ptr = debugfs_create_dir(subdir_name,
							      rf->debugfs_entry.board_root);
			ret = create_file_func(rf);
			if (ret) {
				return ret;
			}
		} else {
			*board_subdir_ptr = dentry;
		}
		break;
	default:
		break;
	}
	
	return 0;
}

static int zrdma_create_vhca_debugfs(struct zxdh_pci_f *rf, 
				     const char *subdir_name,
				     struct dentry **vhca_subdir_ptr,
				     int (*create_file_func)(struct zxdh_pci_f *),
				     int mcode_type)
{
	int ret;

	rf->debugfs_entry.vhca_root = debugfs_create_dir(
		dev_name(&rf->pcidev->dev), zrdma_debugfs_root);
	*vhca_subdir_ptr = debugfs_create_dir(subdir_name, rf->debugfs_entry.vhca_root);
	ret = create_file_func(rf);
	if (ret)
		goto err;
	return 0;
err:
	zrdma_cleanup_mcode_type_debugfs_entry(rf, mcode_type);
	return ret;
}

int create_debugfs_file_vhca_dcqcn(struct zxdh_pci_f *rf)
{
	int i;
	int offset;
	struct zrdma_dbg_vhca_dcqcn_params *dbg_cc_params;

	dbg_cc_params = kzalloc(sizeof(*dbg_cc_params), GFP_KERNEL);
	if (!dbg_cc_params)
		return -ENOMEM;
	rf->debugfs_entry.vhca_params.mcode_vhca_params.vhca_dcqcn_params = dbg_cc_params;
	for (i = 0, offset = ZRDMA_DBG_DCQCN_RPG_BYTE_RESET;
	     i < ZRDMA_VHCA_DCQCN_CC_MAX; i++, offset++) {
		dbg_cc_params->params[i].offset = offset;
		dbg_cc_params->params[i].dev = rf;
		debugfs_create_file(zrdma_dcqcn_params[offset].name, 0600,
				    rf->debugfs_entry.vhca_dcqcn_root,
				    &dbg_cc_params->params[i], &dbg_dcqcn_fops);
	}
	return 0;
}

int create_debugfs_file_board_dcqcn(struct zxdh_pci_f *rf)
{
	int i;
	struct zrdma_dbg_board_dcqcn_params *dbg_cc_params;

	dbg_cc_params = kzalloc(sizeof(*dbg_cc_params), GFP_KERNEL);
	if (!dbg_cc_params)
		return -ENOMEM;
	rf->debugfs_entry.board_params.mcode_board_params.board_dcqcn_params = dbg_cc_params;
	for (i = 0; i < ZRDMA_BOARD_DCQCN_CC_MAX; i++) {
		dbg_cc_params->params[i].offset = i;
		dbg_cc_params->params[i].dev = rf;
		debugfs_create_file(zrdma_dcqcn_params[i].name, 0600,
				    rf->debugfs_entry.board_dcqcn_root,
				    &dbg_cc_params->params[i], &dbg_dcqcn_fops);
	}
	return 0;
}

int create_debugfs_file_board_np_psn_wraparound(struct zxdh_pci_f *rf)
{
	int i;
	struct zrdma_dbg_board_np_psn_wraparound_params *dbg_cc_params;

	dbg_cc_params = kzalloc(sizeof(*dbg_cc_params), GFP_KERNEL);
	if (!dbg_cc_params)
		return -ENOMEM;
	rf->debugfs_entry.board_params.board_np_psn_wraparound_params = dbg_cc_params;
	for (i = 0; i < ZRDMA_BOARD_NP_PSN_WRAPAROUND_CC_MAX; i++) {
		dbg_cc_params->params[i].offset = i;
		dbg_cc_params->params[i].dev = rf;
		debugfs_create_file(zrdma_np_psn_wraparound_params[i].name, 0600,
				    rf->debugfs_entry.board_np_psn_wraparound_root,
				    &dbg_cc_params->params[i], &dbg_np_psn_wraparound_fops);
	}
	return 0;
}


int create_debugfs_file_vhca_rtt(struct zxdh_pci_f *rf)
{
	int i = 0;
	int offset = 0;
	struct zrdma_dbg_vhca_rtt_params *dbg_cc_params;

	dbg_cc_params = kzalloc(sizeof(*dbg_cc_params), GFP_KERNEL);
	if (!dbg_cc_params)
		return -ENOMEM;
	rf->debugfs_entry.vhca_params.mcode_vhca_params.vhca_rtt_params = dbg_cc_params;
	for (i = 0, offset = ZRDMA_DBG_RTT_RPG_MAX_RATE;
	     i < ZRDMA_VHCA_RTT_CC_MAX; i++, offset++) {
		dbg_cc_params->params[i].offset = offset;
		dbg_cc_params->params[i].dev = rf;
		debugfs_create_file(zrdma_rtt_params[offset].name, 0600,
				    rf->debugfs_entry.vhca_rtt_root,
				    &dbg_cc_params->params[i], &dbg_rtt_fops);
	}
	return 0;
}

int create_debugfs_file_board_rtt(struct zxdh_pci_f *rf)
{
	int i = 0;
	struct zrdma_dbg_board_rtt_params *dbg_cc_params;

	dbg_cc_params = kzalloc(sizeof(*dbg_cc_params), GFP_KERNEL);
	if (!dbg_cc_params)
		return -ENOMEM;
	rf->debugfs_entry.board_params.mcode_board_params.board_rtt_params = dbg_cc_params;
	for (i = 0; i < ZRDMA_BOARD_RTT_CC_MAX; i++) {
		dbg_cc_params->params[i].offset = i;
		dbg_cc_params->params[i].dev = rf;
		debugfs_create_file(zrdma_rtt_params[i].name, 0600,
				    rf->debugfs_entry.board_rtt_root,
				    &dbg_cc_params->params[i], &dbg_rtt_fops);
	}
	return 0;
}

void create_debugfs_dcqcn_entry(const char *pci_bdf, struct zxdh_pci_f *rf, enum zrdma_debugfs_mode mode)
{
	int ret;
	const char *debugfs_dir = NULL;

	mutex_lock(&zrdma_debugfs_mutex);
	ret = zrdma_create_board_root_debugfs(rf, pci_bdf, mode);
	if (ret) {
		mutex_unlock(&zrdma_debugfs_mutex);
		goto err;
	}

	if (rf->mcode_type == MCODE_TYPE_DCQCN) {
		debugfs_dir = ZRDMA_DEBUGFS_DCQCN_DIR;
	} else {
		debugfs_dir = ZRDMA_DEBUGFS_WUMENG_DIR;
	}

	ret = zrdma_create_board_subdir_debugfs(rf, debugfs_dir,
					       &rf->debugfs_entry.board_dcqcn_root,
					       create_debugfs_file_board_dcqcn, mode);
	if (ret) {
		mutex_unlock(&zrdma_debugfs_mutex);
		goto err;
	}
	mutex_unlock(&zrdma_debugfs_mutex);

	if (mode == ZRDMA_DEBUGFS_MODE_NORMAL) {
		ret = zrdma_create_vhca_debugfs(rf, debugfs_dir,
						&rf->debugfs_entry.vhca_dcqcn_root,
						create_debugfs_file_vhca_dcqcn,
						rf->mcode_type);
		if (ret)
			goto err;
	}
	return;
err:
	zrdma_cleanup_mcode_type_debugfs_entry(rf, rf->mcode_type);
}

void create_debugfs_rtt_entry(const char *pci_bdf, struct zxdh_pci_f *rf, enum zrdma_debugfs_mode mode)
{
	int ret;

	mutex_lock(&zrdma_debugfs_mutex);
	ret = zrdma_create_board_root_debugfs(rf, pci_bdf, mode);
	if (ret) {
		mutex_unlock(&zrdma_debugfs_mutex);
		goto err;
	}
	
	ret = zrdma_create_board_subdir_debugfs(rf, ZRDMA_DEBUGFS_RTT_DIR,
					       &rf->debugfs_entry.board_rtt_root,
					       create_debugfs_file_board_rtt, mode);
	if (ret) {
		mutex_unlock(&zrdma_debugfs_mutex);
		goto err;
	}
	mutex_unlock(&zrdma_debugfs_mutex);
	
	if (mode == ZRDMA_DEBUGFS_MODE_NORMAL) {
		ret = zrdma_create_vhca_debugfs(rf, ZRDMA_DEBUGFS_RTT_DIR,
						&rf->debugfs_entry.vhca_rtt_root,
						create_debugfs_file_vhca_rtt,
						MCODE_TYPE_RTT);
		if (ret)
			goto err;
	}
	return;
err:
	zrdma_cleanup_mcode_type_debugfs_entry(rf, MCODE_TYPE_RTT);
}

void create_debugfs_np_psn_wraparound_entry(const char *pci_bdf,
					    struct zxdh_pci_f *rf, enum zrdma_debugfs_mode mode)
{
	int ret;
	if (rf->ftype) {
		return;
	}
	mutex_lock(&zrdma_debugfs_mutex);
	ret = zrdma_create_board_root_debugfs(rf, pci_bdf, mode);
	if (ret) {
		mutex_unlock(&zrdma_debugfs_mutex);
		goto err;
	}

	ret = zrdma_create_board_subdir_debugfs(rf, ZRDMA_DEBUGFS_NP_PSN_WRAPAROUND,
					       &rf->debugfs_entry.board_np_psn_wraparound_root,
					       create_debugfs_file_board_np_psn_wraparound, mode);
	if (ret) {
		mutex_unlock(&zrdma_debugfs_mutex);
		goto err;
	}
	mutex_unlock(&zrdma_debugfs_mutex);
	return;
err:
	zrdma_cleanup_np_psn_wraparound_params_debugfs_entry(rf);
}

int read_pid_t_low(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, rf->mcode_type, E_PARA_PID_TLOW, var);
}

int write_pid_t_low(struct zxdh_pci_f *rf, u32 var)
{
	int ret;
	u32 pid_1_tlow = 0, thigh = 0;

	ret = zxdh_mp_dtcm_para_get(rf, rf->mcode_type, E_PARA_PID_THIGH,
				    &thigh);
	if (ret)
		return ret;
	if (var > thigh || var <= 0 || var > RDMA_PID_T_LOW)
		return -EINVAL;

	ret = zxdh_mp_dtcm_para_set(rf, rf->mcode_type, E_PARA_PID_TLOW, var);
	if (ret)
		return ret;
	pid_1_tlow = RDMA_PID_T_LOW / var;
	return zxdh_mp_dtcm_para_set(rf, rf->mcode_type, E_PARA_PID_1_TLOW,
				    pid_1_tlow);
}

int read_pid_rpg_ai_rate(struct zxdh_pci_f *rf, u32 *var)
{
	return zxdh_mp_dtcm_para_get(rf, rf->mcode_type, E_PARA_PID_RPG_AI_RATE_VHCA,
				     var);
}

int write_pid_rpg_ai_rate(struct zxdh_pci_f *rf, u32 var)
{
	return zxdh_mp_dtcm_para_set(rf, rf->mcode_type, E_PARA_PID_RPG_AI_RATE_VHCA,
				     var);
}

struct parameter_t zrdma_pid_params[] = {
    { "t_low", ZRDMA_DBG_T_LOW_PARA, &read_pid_t_low, &write_pid_t_low },
    { "rpg_ai_rate", ZRDMA_DBG_RPG_AI_RATE_PARA, &read_pid_rpg_ai_rate, &write_pid_rpg_ai_rate },
};

int zrdma_ib_write_pid_params(struct zxdh_pci_f *rf, int offset, u32 var)
{
    if (offset >= ZRDMA_DBG_PID_PARA_MAX || offset < 0)
        return -EINVAL;
    return zrdma_pid_params[offset].wfunc(rf, var);
}

int zrdma_ib_read_pid_params(struct zxdh_pci_f *rf, int offset, u32 *var)
{
    if (offset >= ZRDMA_DBG_PID_PARA_MAX || offset < 0)
        return -EINVAL;
    return zrdma_pid_params[offset].rfunc(rf, var);
}

static ssize_t pid_params_read_param(struct file *filp, char __user *buf,
                                      size_t count, loff_t *pos)
{
    struct zrdma_dbg_param *param;
    int offset;
    u32 var = 0;
    int ret;
    char lbuf[ZRDMA_DEBUGFS_MAX_BUF_LEN] = { 0 };

    if(!filp)
        return -EINVAL;

    param = filp->private_data;
    if (!param)
        return -EINVAL;
    offset = param->offset;

    ret = zrdma_ib_read_pid_params(param->dev, offset, &var);
    if (ret)
        return ret;
    ret = snprintf(lbuf, sizeof(lbuf), "%u\n", var);
    if (ret < 0)
        return ret;
    return simple_read_from_buffer(buf, count, pos, lbuf, ret);
}

static ssize_t pid_params_write_param(struct file *filp, const char __user *buf,
                                       size_t count, loff_t *pos)
{
    struct zrdma_dbg_param *param = filp->private_data;
    int offset = param->offset;
    int ret;
    u32 var = 0;
    ret = check_write_param(buf, count, &var);
    if (ret)
        return ret;
    ret = zrdma_ib_write_pid_params(param->dev, offset, var);
    return ret ? ret : count;
}

static const struct file_operations dbg_pid_fops = {
    .owner = THIS_MODULE,
    .open = simple_open,
    .write = pid_params_write_param,
    .read = pid_params_read_param,
};

int create_debugfs_file_board_pid(struct zxdh_pci_f *rf)
{
    int i;
    struct zrdma_dbg_board_pid_params *dbg_cc_params;

    dbg_cc_params = kzalloc(sizeof(*dbg_cc_params), GFP_KERNEL);
    if (!dbg_cc_params)
        return -ENOMEM;
    rf->debugfs_entry.board_params.mcode_board_params.board_pid_params = dbg_cc_params;
    for (i = 0; i < ZRDMA_BOARD_PID_MAX; i++) {
        dbg_cc_params->params[i].offset = i;
        dbg_cc_params->params[i].dev = rf;
        debugfs_create_file(zrdma_pid_params[i].name, 0600,
                            rf->debugfs_entry.board_pid_root,
                            &dbg_cc_params->params[i], &dbg_pid_fops);
    }
    return 0;
}

void create_debugfs_pid_entry(const char *pci_bdf,
                               struct zxdh_pci_f *rf, enum zrdma_debugfs_mode mode)
{
	int ret;
	if (rf->ftype) {
		return;
	}
	mutex_lock(&zrdma_debugfs_mutex);
	ret = zrdma_create_board_root_debugfs(rf, pci_bdf, mode);
	if (ret) {
		mutex_unlock(&zrdma_debugfs_mutex);
		goto err;
	}

	ret = zrdma_create_board_subdir_debugfs(rf, ZRDMA_DEBUGFS_PID_DIR,
					       &rf->debugfs_entry.board_pid_root,
					       create_debugfs_file_board_pid, mode);
	if (ret) {
		mutex_unlock(&zrdma_debugfs_mutex);
		goto err;
	}
	mutex_unlock(&zrdma_debugfs_mutex);
	return;
err:
    zrdma_cleanup_mcode_type_debugfs_entry(rf, MCODE_TYPE_PID);
}

static void create_debugfs_flowlabel_debugfs_entry(struct zxdh_pci_f *rf, enum zrdma_debugfs_mode mode)
{
	struct dentry *dentry;

	if (mode == ZRDMA_DEBUGFS_MODE_NORMAL) {
		if (!rf->debugfs_entry.vhca_root) {
			dentry = debugfs_lookup(dev_name(&rf->pcidev->dev), zrdma_debugfs_root);
			if (!dentry) {
				rf->debugfs_entry.vhca_root =
					debugfs_create_dir(dev_name(&rf->pcidev->dev), zrdma_debugfs_root);
			} else {
				rf->debugfs_entry.vhca_root = dentry;
			}
		}
		debugfs_create_file(ZRDMA_DEBUGFS_FLOW_LABEL, 0600,
			rf->debugfs_entry.vhca_root,
			rf, &flow_label_fops);
	}
	return;
}

void create_debugfs_default_entry(struct zxdh_pci_f *rf, enum zrdma_debugfs_mode mode)
{
	int ret;
	char pci_board_bdf[64] = { 0 };

	if (!zrdma_debugfs_root) {
		pr_info("[zxdh_rdma] %s debugfs zrdma_debugfs_root is null\n", __func__);
		return;
	}
	ret = get_pci_board_bdf(pci_board_bdf, rf);
	if (ret) {
		pr_info("[zxdh_rdma] create_debugfs_entry debugfs pci_board_bdf is null\n");
		return;
	}
	switch (rf->mcode_type) {
	case MCODE_TYPE_DCQCN:
	case MCODE_TYPE_WUMENG:
		create_debugfs_dcqcn_entry(pci_board_bdf, rf, mode);
		break;
	case MCODE_TYPE_RTT:
		create_debugfs_rtt_entry(pci_board_bdf, rf, mode);
		break;
	case MCODE_TYPE_PID:
		create_debugfs_pid_entry(pci_board_bdf, rf, mode);
		break;
	default:
		break;
	}
	create_debugfs_np_psn_wraparound_entry(pci_board_bdf, rf, mode);
	create_debugfs_flowlabel_debugfs_entry(rf, mode);
}

void create_debugfs_entry(struct zxdh_pci_f *rf)
{
	create_debugfs_default_entry(rf, ZRDMA_DEBUGFS_MODE_NORMAL);
}

void zrdma_register_debugfs(void)
{
	zrdma_debugfs_root = debugfs_create_dir("zrdma", NULL);
	mutex_init(&zrdma_debugfs_mutex);
}

void zrdma_unregister_debugfs(void)
{
	debugfs_remove(zrdma_debugfs_root);
}

/* AEQ polling mode debugfs interfaces */
static ssize_t aeq_poll_interval_read(struct file *filp, char __user *buf,
                                      size_t count, loff_t *ppos)
{
	struct zxdh_pci_f *rf = filp->private_data;
	char tmp[32];
	int len;

	len = zte_snprintf_s(tmp, sizeof(tmp), "%u\n", rf->aeq_poll_interval_ms);
	if (len < 0)
		return len;
	return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static ssize_t aeq_poll_interval_write(struct file *filp,
                                       const char __user *buf,
                                       size_t count, loff_t *ppos)
{
	struct zxdh_pci_f *rf = filp->private_data;
	u32 interval;
	int ret;

	ret = kstrtou32_from_user(buf, count, 10, &interval);
	if (ret)
		return ret;

	if (interval < ZXDH_AEQ_POLL_INTERVAL_MIN ||
	    interval > ZXDH_AEQ_POLL_INTERVAL_MAX)
		return -EINVAL;

	rf->aeq_poll_interval_ms = interval;

	/* If polling is running, dynamically update the interval */
	if (rf->aeq.work_mode == ZXDH_AEQ_MODE_POLLING) {
		rf->aeq.poll_interval_ms = interval;
		if (rf->use_blue_flame_flag && (!rf->ftype)) {
			rf->ext_aeq.poll_interval_ms = interval;
		}
	}

	return count;
}

static const struct file_operations aeq_poll_interval_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = aeq_poll_interval_read,
	.write = aeq_poll_interval_write,
};

static ssize_t aeq_mode_read(struct file *filp, char __user *buf,
                             size_t count, loff_t *ppos)
{
	struct zxdh_pci_f *rf = filp->private_data;
	char tmp[32];
	int len;
	const char *mode_str;

	switch (rf->aeq.work_mode) {
	case ZXDH_AEQ_MODE_POLLING:
		mode_str = "polling";
		break;
	case ZXDH_AEQ_MODE_INTERRUPT:
	default:
		mode_str = "interrupt";
		break;
	}

	len = zte_snprintf_s(tmp, sizeof(tmp), "%s\n", mode_str);
	if (len < 0)
		return len;
	return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static const struct file_operations aeq_mode_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = aeq_mode_read,
};

static ssize_t aeq_event_count_read(struct file *filp, char __user *buf,
                                    size_t count, loff_t *ppos)
{
	struct zxdh_pci_f *rf = filp->private_data;
	char tmp[32];
	int len;

	len = zte_snprintf_s(tmp, sizeof(tmp), "%u\n",
		     atomic_read(&rf->aeq.aeq_event_count));
	if (len < 0)
		return len;
	return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static const struct file_operations aeq_event_count_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = aeq_event_count_read,
};

/* Ext AEQ event count debugfs interface */
static ssize_t ext_aeq_event_count_read(struct file *filp, char __user *buf,
                                        size_t count, loff_t *ppos)
{
	struct zxdh_pci_f *rf = filp->private_data;
	char tmp[32];
	int len;

	len = zte_snprintf_s(tmp, sizeof(tmp), "%u\n",
		     atomic_read(&rf->ext_aeq.aeq_event_count));
	if (len < 0)
		return len;
	return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static const struct file_operations ext_aeq_event_count_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ext_aeq_event_count_read,
};

/* AEQ status debugfs interface */
static ssize_t aeq_status_read(struct file *filp, char __user *buf,
                               size_t count, loff_t *ppos)
{
	struct zxdh_pci_f *rf = filp->private_data;
	char tmp[256];
	ssize_t len;

	len = zxdh_aeq_get_status(rf, tmp, sizeof(tmp));
	return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static const struct file_operations aeq_status_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = aeq_status_read,
};

/**
 * zxdh_create_aeq_poll_debugfs - Create AEQ polling debugfs entries
 * @rf: RDMA PCI function
 *
 * Create debugfs files for AEQ polling mode monitoring and control
 * NOTE: AEQ belongs to RDMA device, use vhca_root instead of board_root
 */
void zxdh_create_aeq_poll_debugfs(struct zxdh_pci_f *rf)
{
	struct dentry *file_entry;
	bool need_cleanup = false;

	/* Use vhca_root - AEQ belongs to RDMA device, not NIC device */
	if (!rf->debugfs_entry.vhca_root) {
		pr_debug("[zxdh_rdma] vhca_root is NULL, skipping AEQ debugfs creation\n");
		return;
	}

	/* Check if debugfs files already exist to avoid duplicate creation */
	file_entry = debugfs_lookup("aeq_poll_interval", rf->debugfs_entry.vhca_root);
	if (!IS_ERR_OR_NULL(file_entry)) {
		dput(file_entry);
		pr_debug("[zxdh_rdma] AEQ poll debugfs files already exist, cleaning up first\n");
		need_cleanup = true;
	}

	if (need_cleanup) {
		/* Clean up existing files first */
		zxdh_remove_aeq_poll_debugfs(rf);
		/* Small delay to ensure cleanup completes */
		usleep_range(1000, 2000);
	}

	/* Create the debugfs files in vhca_root (RDMA device directory) */
	debugfs_create_file("aeq_poll_interval", 0644, rf->debugfs_entry.vhca_root, rf,
	                   &aeq_poll_interval_fops);

	debugfs_create_file("aeq_mode", 0444, rf->debugfs_entry.vhca_root, rf,
	                   &aeq_mode_fops);

	debugfs_create_file("aeq_event_count", 0444, rf->debugfs_entry.vhca_root, rf,
	                   &aeq_event_count_fops);

	/* Only create ext_aeq_event_count if ext_mem is enabled */
	if (rf->use_blue_flame_flag && !rf->ftype) {
		debugfs_create_file("ext_aeq_event_count", 0444, rf->debugfs_entry.vhca_root, rf,
		                   &ext_aeq_event_count_fops);
	}

	debugfs_create_file("aeq_status", 0444, rf->debugfs_entry.vhca_root, rf,
	                   &aeq_status_fops);

	pr_debug("[zxdh_rdma] AEQ poll debugfs files created successfully in vhca_root\n");
}
EXPORT_SYMBOL(zxdh_create_aeq_poll_debugfs);

/**
 * zxdh_remove_aeq_poll_debugfs - Remove AEQ polling debugfs entries
 * @rf: RDMA PCI function
 *
 * Remove debugfs files for AEQ polling mode from vhca_root
 */
void zxdh_remove_aeq_poll_debugfs(struct zxdh_pci_f *rf)
{
	if (!rf->debugfs_entry.vhca_root)
		return;

	/* Remove AEQ polling debugfs files from vhca_root */
	debugfs_remove(debugfs_lookup("aeq_poll_interval", rf->debugfs_entry.vhca_root));
	debugfs_remove(debugfs_lookup("aeq_mode", rf->debugfs_entry.vhca_root));
	debugfs_remove(debugfs_lookup("aeq_event_count", rf->debugfs_entry.vhca_root));
	debugfs_remove(debugfs_lookup("ext_aeq_event_count", rf->debugfs_entry.vhca_root));
	debugfs_remove(debugfs_lookup("aeq_status", rf->debugfs_entry.vhca_root));

	pr_debug("[zxdh_rdma] AEQ poll debugfs files removed from vhca_root\n");
}
EXPORT_SYMBOL(zxdh_remove_aeq_poll_debugfs);
