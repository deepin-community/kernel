/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZRDMA_H
#define ZRDMA_H

#define RDMA_BIT2(type, a) ((u##type)1UL << a)
#define RDMA_MASK3(type, mask, shift) ((u##type)mask << shift)
#define MAKEMASK(m, s) ((m) << (s))

#define ZXDH_WQEALLOC_WQE_DESC_INDEX_S 20
#define ZXDH_WQEALLOC_WQE_DESC_INDEX GENMASK(31, 20)

#define ZXDH_CQPTAIL_WQTAIL_S 0
#define ZXDH_CQPTAIL_WQTAIL GENMASK(10, 0)
#define ZXDH_CQPTAIL_CQP_OP_ERR_S 31
#define ZXDH_CQPTAIL_CQP_OP_ERR BIT(31)

#define ZXDH_CQPERRCODES_CQP_MINOR_CODE_S 0
#define ZXDH_CQPERRCODES_CQP_MINOR_CODE GENMASK(15, 0)
#define ZXDH_CQPERRCODES_CQP_MAJOR_CODE_S 16
#define ZXDH_CQPERRCODES_CQP_MAJOR_CODE GENMASK(31, 16)
// CQP Address Masks
#define ZXDH_CQPADDR_HIGH_S 32
#define ZXDH_CQPADDR_HIGH GENMASK_ULL(63, 32)
#define ZXDH_CQPADDR_LOW_S 0
#define ZXDH_CQPADDR_LOW GENMASK_ULL(31, 0)

#define ZXDH_GLPCI_LBARCTRL_PE_DB_SIZE_S 4
#define ZXDH_GLPCI_LBARCTRL_PE_DB_SIZE GENMASK(5, 4)
#define ZXDH_GLINT_RATE_INTERVAL_S 0
#define ZXDH_GLINT_RATE_INTERVAL GENMASK(4, 0)
#define ZXDH_GLINT_RATE_INTRL_ENA_S 6
#define ZXDH_GLINT_RATE_INTRL_ENA_M BIT(6)
#define ZXDH_GLINT_RATE_INTRL_ENA BIT(6)

#define ZXDH_GLINT_DYN_CTL_INTENA_S 0
#define ZXDH_GLINT_DYN_CTL_INTENA BIT(0)
#define ZXDH_GLINT_DYN_CTL_CLEARPBA_S 1
#define ZXDH_GLINT_DYN_CTL_CLEARPBA BIT(1)
#define ZXDH_GLINT_DYN_CTL_ITR_INDX_S 3
#define ZXDH_GLINT_DYN_CTL_ITR_INDX GENMASK(4, 3)
#define ZXDH_GLINT_DYN_CTL_INTERVAL_S 5
#define ZXDH_GLINT_DYN_CTL_INTERVAL GENMASK(16, 5)
#define ZXDH_GLINT_CEQCTL_ITR_INDX_S 11
#define ZXDH_GLINT_CEQCTL_ITR_INDX GENMASK(12, 11)
#define ZXDH_GLINT_CEQCTL_CAUSE_ENA_S 30
#define ZXDH_GLINT_CEQCTL_CAUSE_ENA BIT(30)
#define ZXDH_GLINT_CEQCTL_MSIX_INDX_S 0
#define ZXDH_GLINT_CEQCTL_MSIX_INDX GENMASK(10, 0)
#define ZXDH_PFINT_AEQCTL_MSIX_INDX_S 0
#define ZXDH_PFINT_AEQCTL_MSIX_INDX GENMASK(10, 0)
#define ZXDH_PFINT_AEQCTL_ITR_INDX_S 11
#define ZXDH_PFINT_AEQCTL_ITR_INDX GENMASK(12, 11)
#define ZXDH_PFINT_AEQCTL_CAUSE_ENA_S 30
#define ZXDH_PFINT_AEQCTL_CAUSE_ENA BIT(30)
#define ZXDH_PFHMC_PDINV_PMSDIDX_S 0
#define ZXDH_PFHMC_PDINV_PMSDIDX GENMASK(11, 0)
#define ZXDH_PFHMC_PDINV_PMSDPARTSEL_S 15
#define ZXDH_PFHMC_PDINV_PMSDPARTSEL BIT(15)
#define ZXDH_PFHMC_PDINV_PMPDIDX_S 16
#define ZXDH_PFHMC_PDINV_PMPDIDX GENMASK(24, 16)
#define ZXDH_PFHMC_SDDATALOW_PMSDVALID_S 0
#define ZXDH_PFHMC_SDDATALOW_PMSDVALID BIT(0)
#define ZXDH_PFHMC_SDDATALOW_PMSDTYPE_S 1
#define ZXDH_PFHMC_SDDATALOW_PMSDTYPE BIT(1)
#define ZXDH_PFHMC_SDDATALOW_PMSDBPCOUNT_S 2
#define ZXDH_PFHMC_SDDATALOW_PMSDBPCOUNT GENMASK(11, 2)
#define ZXDH_PFHMC_SDDATALOW_PMSDDATALOW_S 12
#define ZXDH_PFHMC_SDDATALOW_PMSDDATALOW GENMASK(31, 12)
#define ZXDH_PFHMC_SDCMD_PMSDWR_S 31
#define ZXDH_PFHMC_SDCMD_PMSDWR BIT(31)
#define ZXDH_PFHMC_SDCMD_PMSDPARTSEL_S 15
#define ZXDH_PFHMC_SDCMD_PMSDPARTSEL BIT(15)

#define ZXDH_INVALID_CQ_IDX 0xffffffff

enum zxdh_dyn_idx_t {
	ZXDH_IDX_ITR0 = 0,
	ZXDH_IDX_ITR1 = 1,
	ZXDH_IDX_ITR2 = 2,
	ZXDH_IDX_NOITR = 3,
};

enum zxdh_registers {
	ZXDH_CQPTAIL,
	ZXDH_CQPDB,
	ZXDH_CCQPSTATUS,
	ZXDH_CCQPHIGH,
	ZXDH_CCQPLOW,
	ZXDH_CQARM,
	ZXDH_CQACK,
	ZXDH_AEQALLOC,
	ZXDH_CQPERRCODES,
	ZXDH_WQEALLOC,
	ZXDH_GLINT_DYN_CTL,
	ZXDH_DB_ADDR_OFFSET,
	ZXDH_GLPCI_LBARCTRL,
	ZXDH_GLPE_CPUSTATUS0,
	ZXDH_GLPE_CPUSTATUS1,
	ZXDH_GLPE_CPUSTATUS2,
	ZXDH_PFINT_AEQCTL,
	ZXDH_GLINT_CEQCTL,
	ZXDH_VSIQF_PE_CTL1,
	ZXDH_PFHMC_PDINV,
	ZXDH_GLHMC_VFPDINV,
	ZXDH_GLPE_CRITERR,
	ZXDH_GLINT_RATE,
	ZXDH_MAX_REGS, /* Must be last entry */
};

enum zxdh_shifts {
	ZXDH_CCQPSTATUS_CCQP_DONE_S,
	ZXDH_CCQPSTATUS_CCQP_ERR_S,
	ZXDH_CQPSQ_STAG_PDID_S,
	ZXDH_CQPSQ_CQ_CEQID_S,
	ZXDH_CQPSQ_CQ_CQID_S,
	ZXDH_COMMIT_FPM_CQCNT_S,
	ZXDH_MAX_SHIFTS,
};

enum zxdh_masks {
	ZXDH_CCQPSTATUS_CCQP_DONE_M,
	ZXDH_CCQPSTATUS_CCQP_ERR_M,
	ZXDH_CQPSQ_STAG_PDID_M,
	ZXDH_CQPSQ_CQ_CEQID_M,
	ZXDH_CQPSQ_CQ_CQID_M,
	ZXDH_COMMIT_FPM_CQCNT_M,
	ZXDH_MAX_MASKS, /* Must be last entry */
};

#define ZXDH_MAX_MGS_PER_CTX 1022

struct zxdh_mcast_grp_ctx_entry_info {
	u32 qp_id;
	bool valid_entry;
	u16 dest_port;
	u32 use_cnt;
};

struct zxdh_mcast_grp_info {
	u8 dest_mac_addr[ETH_ALEN];
	u16 vlan_id;
	u8 hmc_fcn_id;
	u8 ipv4_valid : 1;
	u8 vlan_valid : 1;
	u16 mg_id;
	u32 no_of_mgs;
	u32 dest_ip_addr[4];
	u16 qs_handle;
	struct zxdh_dma_mem dma_mem_mc;
	struct zxdh_mcast_grp_ctx_entry_info mg_ctx_info[ZXDH_MAX_MGS_PER_CTX];
};

enum zxdh_rdma_vers {
	ZXDH_GEN_RSVD,
	ZXDH_GEN_1,
	ZXDH_GEN_2,
};

struct zxdh_uk_attrs {
	u64 feature_flags;
	u32 max_hw_wq_frags;
	u32 max_hw_read_sges;
	u32 max_hw_inline;
	u32 max_hw_srq_quanta;
	u32 max_hw_rq_quanta;
	u32 max_hw_wq_quanta;
	u32 min_hw_cq_size;
	u32 max_hw_cq_size;
	u16 max_hw_sq_chunk;
	u32 max_hw_srq_wr;
	u8 hw_rev;
    u32 max_hw_sq_sges;
    u32 max_hw_rq_sges;
};

struct zxdh_hw_attrs {
	struct zxdh_uk_attrs uk_attrs;
	u64 max_hw_outbound_msg_size;
	u64 max_hw_inbound_msg_size;
	u64 max_mr_size;
	u32 min_hw_qp_id;
	u32 min_hw_aeq_size;
	u32 max_hw_aeq_size;
	u32 min_hw_ceq_size;
	u32 max_hw_ceq_size;
	u32 max_hw_device_pages;
	u32 max_hw_vf_fpm_id;
	u32 first_hw_vf_fpm_id;
	u32 max_hw_ird;
	u32 max_hw_ord;
	u32 max_hw_wqes;
	u32 max_hw_pds;
	u32 max_hw_ena_vf_count;
	u32 max_qp_wr;
	u32 max_srq_wr;
	u32 max_pe_ready_count;
	u32 max_done_count;
	u32 max_sleep_count;
	u32 max_cqp_compl_wait_time_ms;
	u16 max_stat_inst;
	u16 max_stat_idx;
	u32 cqp_timeout_threshold;
	u8 skip_hw;
};

void zxdh_init_hw(struct zxdh_sc_dev *dev);
void zxdh_check_fc_for_qp(struct zxdh_sc_vsi *vsi, struct zxdh_sc_qp *sc_qp);
#endif /* ZXDH_H*/
