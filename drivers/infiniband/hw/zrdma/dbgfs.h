/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */
#ifndef ZXDH_DEBUGFS_H
#define ZXDH_DEBUGFS_H
#include <linux/debugfs.h>
#include "type.h"

enum zrdma_debugfs_mode {
	ZRDMA_DEBUGFS_MODE_NORMAL = 0,  /* 正常模式，只创建board相关debugfs */
	ZRDMA_DEBUGFS_MODE_BOND,        /* bond模式，同时创建board和vhca相关debugfs */
};

#define ZRDMA_DEBUGFS_MAX_BUF_LEN 35

#define ZRDMA_BOARD_DCQCN_CC_MAX 16
#define ZRDMA_VHCA_DCQCN_CC_MAX 5
#define ZRDMA_BOARD_RTT_CC_MAX 7
#define ZRDMA_VHCA_RTT_CC_MAX 3
#define ZRDMA_BOARD_NP_PSN_WRAPAROUND_CC_MAX 2
#define ZRDMA_BOARD_PID_MAX 2

#define ZRDMA_DEBUGFS_DCQCN_DIR "dcqcn"
#define ZRDMA_DEBUGFS_RTT_DIR "rtt"
#define ZRDMA_DEBUGFS_WUMENG_DIR "wumeng"
#define ZRDMA_DEBUGFS_PID_DIR "pid"
#define ZRDMA_DEBUGFS_NP_PSN_WRAPAROUND "wrap_params"
#define ZRDMA_DEBUGFS_FLOW_LABEL "flow_label"


enum zrdma_dbg_dcqcn_types {
	ZRDMA_DBG_DCQCN_NP_CNP_DSCP = 0,
	ZRDMA_DBG_DCQCN_NP_CNP_PRIO,
	ZRDMA_DBG_DCQCN_NP_CNP_PRIO_MODE,
	ZRDMA_DBG_DCQCN_NP_MIN_TIME_BETWEEN_CNPS,
	ZRDMA_DBG_DCQCN_PRG_TIME_RESET,
	ZRDMA_DBG_DCQCN_RPG_CLAMP_TGT_RATE,
	ZRDMA_DBG_DCQCN_RPG_CLAMP_TGT_RATE_AFTER_TIME_INC,
	ZRDMA_DBG_DCQCN_RP_DCE_TCP_RTT,
	ZRDMA_DBG_DCQCN_DCE_TCP_G,
	ZRDMA_DBG_DCQCN_RPG_GD,
	ZRDMA_DBG_DCQCN_RPG_INITIAL_ALPHA_VALUE,
	ZRDMA_DBG_DCQCN_RPG_MIN_DEC_FAC,
	ZRDMA_DBG_DCQCN_RPG_THRESHOLD,
	ZRDMA_DBG_DCQCN_RPG_RATIO_INCREASE,
	ZRDMA_DBG_DCQCN_RPG_AI_RATIO,
	ZRDMA_DBG_DCQCN_RPG_HAI_RATIO,
	ZRDMA_DBG_DCQCN_RPG_BYTE_RESET,
	ZRDMA_DBG_DCQCN_RPG_AI_RATE,
	ZRDMA_DBG_DCQCN_RPG_HAI_RATE,
	ZRDMA_DBG_DCQCN_RPG_MAX_RATE,
	ZRDMA_DBG_DCQCN_RPG_MIN_RATE,
	ZRDMA_DBG_DCQCN_MAX,
};

enum zrdma_dbg_rtt_types {
	ZRDMA_DBG_RTT_ALPHA = 0,
	ZRDMA_DBG_RTT_TLOW,
	ZRDMA_DBG_RTT_THIGH,
	ZRDMA_DBG_RTT_AI_NUM,
	ZRDMA_DBG_RTT_THRED_GRADIENT,
	ZRDMA_DBG_RTT_HAI_N,
	ZRDMA_DBG_RTT_AI_N,
	ZRDMA_DBG_RTT_RPG_MAX_RATE,
	ZRDMA_DBG_RTT_RPG_MIN_RATE,
	ZRDMA_DBG_RTT_VF_DELTA,
	ZRDMA_DBG_RTT_MAX,
};

enum zrdma_dbg_np_psn_wraparound_types {
	ZRDMA_DBG_NP_PSN_WRAPAROUND_ENABLE_PARA = 0,
	ZRDMA_DBG_NP_PMTU_PARA,
	ZRDMA_DBG_NP_PARA_MAX,
};

enum zrdma_dbg_pid_types {
	ZRDMA_DBG_T_LOW_PARA = 0,
	ZRDMA_DBG_RPG_AI_RATE_PARA,
	ZRDMA_DBG_PID_PARA_MAX,
};

struct parameter_t {
	const char *name;
	uint8_t types;
	int (*rfunc)(struct zxdh_pci_f *, u32 *);
	int (*wfunc)(struct zxdh_pci_f *, u32);
};

struct zrdma_dbg_param {
	int offset;
	struct zxdh_pci_f *dev;
};

struct zrdma_dbg_board_dcqcn_params {
	struct dentry *root;
	struct zrdma_dbg_param params[ZRDMA_BOARD_DCQCN_CC_MAX];
};

struct zrdma_dbg_vhca_dcqcn_params {
	struct dentry *root;
	struct zrdma_dbg_param params[ZRDMA_VHCA_DCQCN_CC_MAX];
};

struct zrdma_dbg_board_np_psn_wraparound_params {
	struct dentry *root;
	struct zrdma_dbg_param params[ZRDMA_BOARD_NP_PSN_WRAPAROUND_CC_MAX];
};

struct zrdma_dbg_board_pid_params {
	struct dentry *root;
	struct zrdma_dbg_param params[ZRDMA_BOARD_PID_MAX];
};

struct zrdma_dbg_board_rtt_params {
	struct dentry *root;
	struct zrdma_dbg_param params[ZRDMA_BOARD_RTT_CC_MAX];
};

struct zrdma_dbg_vhca_rtt_params {
	struct dentry *root;
	struct zrdma_dbg_param params[ZRDMA_VHCA_RTT_CC_MAX];
};

struct zrdma_board_params {
	union {
		void *base;
		struct zrdma_dbg_board_dcqcn_params *board_dcqcn_params;
		struct zrdma_dbg_board_rtt_params *board_rtt_params;
		struct zrdma_dbg_board_pid_params *board_pid_params;
	} mcode_board_params;
	struct zrdma_dbg_board_np_psn_wraparound_params
		*board_np_psn_wraparound_params;
};

struct zrdma_vhca_params {
	union {
		void *base;
		struct zrdma_dbg_vhca_dcqcn_params *vhca_dcqcn_params;
		struct zrdma_dbg_vhca_rtt_params *vhca_rtt_params;
	} mcode_vhca_params;
};

struct zrdma_debugfs_entries {
	struct dentry *board_root;
	struct dentry *vhca_root;
	struct dentry *board_dcqcn_root;
	struct dentry *board_rtt_root;
	struct dentry *board_np_psn_wraparound_root;
	struct dentry *board_pid_root;
	struct dentry *vhca_dcqcn_root;
	struct dentry *vhca_rtt_root;
	struct zrdma_board_params board_params;
	struct zrdma_vhca_params vhca_params;
};

void create_debugfs_entry(struct zxdh_pci_f *rf);
void zrdma_register_debugfs(void);
void zrdma_unregister_debugfs(void);
void zrdma_cleanup_debugfs_entry(struct zxdh_pci_f *rf);
void zrdma_cleanup_mcode_type_debugfs_entry(struct zxdh_pci_f *rf, int type);
void zrdma_cleanup_np_psn_wraparound_params_debugfs_entry(struct zxdh_pci_f *rf);
int zrdma_ib_write_rtt_params(struct zxdh_pci_f *rf, int offset, u32 var);
int zrdma_ib_read_rtt_params(struct zxdh_pci_f *rf, int offset, u32 *var);
int zrdma_ib_write_dcqcn_params(struct zxdh_pci_f *rf, int offset, u32 var);
int zrdma_ib_read_dcqcn_params(struct zxdh_pci_f *rf, int offset, u32 *var);
int zrdma_ib_write_np_psn_wraparound_params(struct zxdh_pci_f *rf, int offset, u32 var);
int zrdma_ib_read_np_psn_wraparound_params(struct zxdh_pci_f *rf, int offset, u32 *var);
int zrdma_ib_write_pid_params(struct zxdh_pci_f *rf, int offset, u32 var);
int zrdma_ib_read_pid_params(struct zxdh_pci_f *rf, int offset, u32 *var);
int create_debugfs_file_vhca_dcqcn(struct zxdh_pci_f *rf);
int create_debugfs_file_board_dcqcn(struct zxdh_pci_f *rf);
int create_debugfs_file_vhca_rtt(struct zxdh_pci_f *rf);
int create_debugfs_file_board_rtt(struct zxdh_pci_f *rf);
int create_debugfs_file_board_np_psn_wraparound(struct zxdh_pci_f *rf);
void create_debugfs_dcqcn_entry(const char *pci_bdf, struct zxdh_pci_f *rf, enum zrdma_debugfs_mode mode);
void create_debugfs_rtt_entry(const char *pci_bdf, struct zxdh_pci_f *rf, enum zrdma_debugfs_mode mode);
void create_debugfs_np_psn_wraparound_entry(const char *pci_bdf, struct zxdh_pci_f *rf, enum zrdma_debugfs_mode mode);
void create_debugfs_pid_entry(const char *pci_bdf, struct zxdh_pci_f *rf, enum zrdma_debugfs_mode mode);
int create_debugfs_file_board_pid(struct zxdh_pci_f *rf);
int zrdma_create_board_root_debugfs(struct zxdh_pci_f *rf, const char *pci_bdf, enum zrdma_debugfs_mode mode);
int zrdma_create_board_subdir_debugfs(struct zxdh_pci_f *rf, 
					    const char *subdir_name,
					    struct dentry **board_subdir_ptr,
					    int (*create_file_func)(struct zxdh_pci_f *),
					    enum zrdma_debugfs_mode mode);
void create_debugfs_default_entry(struct zxdh_pci_f *rf, enum zrdma_debugfs_mode mode);

int read_np_cnp_dscp(struct zxdh_pci_f *rf, u32 *var);
int read_np_cnp_prio(struct zxdh_pci_f *rf, u32 *var);
int read_np_cnp_prio_mode(struct zxdh_pci_f *rf, u32 *var);
int read_np_min_time_between_cnps(struct zxdh_pci_f *rf, u32 *var);
int read_prg_time_reset(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_clamp_tgt_rate(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_clamp_tgt_rate_after_time_inc(struct zxdh_pci_f *rf, u32 *var);
int read_rp_dce_tcp_rtt(struct zxdh_pci_f *rf, u32 *var);
int read_dce_tcp_g(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_gd(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_initial_alpha_value(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_min_dec_fac(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_threshold(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_ratio_increase(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_ai_ratio(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_hai_ratio(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_byte_reset(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_ai_rate(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_hai_rate(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_max_rate(struct zxdh_pci_f *rf, u32 *var);
int read_rpg_min_rate(struct zxdh_pci_f *rf, u32 *var);
int read_alpha(struct zxdh_pci_f *rf, u32 *var);
int read_tlow(struct zxdh_pci_f *rf, u32 *var);
int read_thigh(struct zxdh_pci_f *rf, u32 *var);
int read_ai_num(struct zxdh_pci_f *rf, u32 *var);
int read_thred_gradient(struct zxdh_pci_f *rf, u32 *var);
int read_hai_n(struct zxdh_pci_f *rf, u32 *var);
int read_ai_n(struct zxdh_pci_f *rf, u32 *var);
int read_vf_delta(struct zxdh_pci_f *rf, u32 *var);
int read_psn_wraparound_enable(struct zxdh_pci_f *rf, u32 *var);
int read_pmtu(struct zxdh_pci_f *rf, u32 *var);
int read_pid_t_low(struct zxdh_pci_f *rf, u32 *var);
int read_pid_rpg_ai_rate(struct zxdh_pci_f *rf, u32 *var);

int write_np_cnp_dscp(struct zxdh_pci_f *rf, u32 var);
int write_np_cnp_prio(struct zxdh_pci_f *rf, u32 var);
int write_np_cnp_prio_mode(struct zxdh_pci_f *rf, u32 var);
int write_np_min_time_between_cnps(struct zxdh_pci_f *rf, u32 var);
int write_prg_time_reset(struct zxdh_pci_f *rf, u32 var);
int write_rpg_clamp_tgt_rate(struct zxdh_pci_f *rf, u32 var);
int write_rpg_clamp_tgt_rate_after_time_inc(struct zxdh_pci_f *rf, u32 var);
int write_rp_dce_tcp_rtt(struct zxdh_pci_f *rf, u32 var);
int write_dce_tcp_g(struct zxdh_pci_f *rf, u32 var);
int write_rpg_gd(struct zxdh_pci_f *rf, u32 var);
int write_rpg_initial_alpha_value(struct zxdh_pci_f *rf, u32 var);
int write_rpg_min_dec_fac(struct zxdh_pci_f *rf, u32 var);
int write_rpg_threshold(struct zxdh_pci_f *rf, u32 var);
int write_rpg_ratio_increase(struct zxdh_pci_f *rf, u32 var);
int write_rpg_ai_ratio(struct zxdh_pci_f *rf, u32 var);
int write_rpg_hai_ratio(struct zxdh_pci_f *rf, u32 var);
int write_rpg_byte_reset(struct zxdh_pci_f *rf, u32 var);
int write_rpg_ai_rate(struct zxdh_pci_f *rf, u32 var);
int write_rpg_hai_rate(struct zxdh_pci_f *rf, u32 var);
int write_rpg_max_rate(struct zxdh_pci_f *rf, u32 var);
int write_rpg_min_rate(struct zxdh_pci_f *rf, u32 var);
int write_alpha(struct zxdh_pci_f *rf, u32 var);
int write_tlow(struct zxdh_pci_f *rf, u32 var);
int write_thigh(struct zxdh_pci_f *rf, u32 var);
int write_ai_num(struct zxdh_pci_f *rf, u32 var);
int write_thred_gradient(struct zxdh_pci_f *rf, u32 var);
int write_hai_n(struct zxdh_pci_f *rf, u32 var);
int write_ai_n(struct zxdh_pci_f *rf, u32 var);
int write_vf_delta(struct zxdh_pci_f *rf, u32 var);
int write_psn_wraparound_enable(struct zxdh_pci_f *rf, u32 var);
int write_pmtu(struct zxdh_pci_f *rf, u32 var);
int write_pid_t_low(struct zxdh_pci_f *rf, u32 var);
int write_pid_rpg_ai_rate(struct zxdh_pci_f *rf, u32 var);

#endif
