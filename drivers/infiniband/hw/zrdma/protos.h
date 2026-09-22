/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_PROTOS_H
#define ZXDH_PROTOS_H
#include <rdma/ib_mad.h>

#define PAUSE_TIMER_VAL 0xffff
#define REFRESH_THRESHOLD 0x7fff
#define HIGH_THRESHOLD 0x800
#define LOW_THRESHOLD 0x200
#define ALL_TC2PFC 0xff
#define CQP_COMPL_WAIT_TIME_MS 6
#define CQP_TIMEOUT_THRESHOLD 20000
#define CQP_MIN_TIMEOUT_THRESHOLD 1
#define CQP_STATUS_TRUE 1
#define CQP_DONE_TRUE   1
#define CQP_STATUS_CHECK_CNT 350
#define CQP_ERR_CNT_VALUE    6

struct read_ram_info {
	u32 ram_num;
	u32 ram_width;
	u32 ram_read_cnt;
	u32 ram_addr;
	u32 offset_idx;
};

/* init operations */
int zxdh_sc_dev_init(enum zxdh_rdma_vers ver, struct zxdh_sc_dev *dev,
		      struct zxdh_device_init_info *info);
void zxdh_sc_cqp_post_sq(struct zxdh_sc_cqp *cqp);
__le64 *zxdh_sc_cqp_get_next_send_wqe(struct zxdh_sc_cqp *cqp, u64 scratch);
int zxdh_sc_mr_fast_register(struct zxdh_sc_qp *qp,
			     struct zxdh_fast_reg_stag_info *info,
			     bool post_sq);
void zxdh_init_config_check(struct zxdh_config_check *cc, u8 traffic_class,
			    u16 qs_handle);
/* HMC/FPM functions */

/* stats misc */
int zxdh_rdma_stats_read(struct zxdh_sc_dev *dev,
			  struct zxdh_rdma_stats_get *rdma_stats);

int zxdh_process_pma_cmd(struct zxdh_sc_dev *dev, u8 port,
			 const struct ib_mad *in_mad, struct ib_mad *out_mad);
void zxdh_hw_stats_read_all(struct zxdh_vsi_pestat *stats,
			    const u64 *hw_stats_regs);
int zxdh_cqp_up_map_cmd(struct zxdh_sc_dev *dev, u8 cmd,
			struct zxdh_up_info *map_info);
int zxdh_cqp_ceq_cmd(struct zxdh_sc_dev *dev, struct zxdh_sc_ceq *sc_ceq,
		     u8 op);
int zxdh_cqp_aeq_cmd(struct zxdh_sc_dev *dev, struct zxdh_sc_aeq *sc_aeq,
		     u8 op);
void zxdh_sc_dev_qplist_init(struct zxdh_sc_dev *dev);
int zxdh_sc_add_cq_ctx(struct zxdh_sc_ceq *ceq, struct zxdh_sc_cq *cq);
void zxdh_sc_remove_cq_ctx(struct zxdh_sc_ceq *ceq, struct zxdh_sc_cq *cq);
/* misc L2 param change functions */
void zxdh_qp_add_qos(struct zxdh_sc_qp *qp);
void zxdh_qp_rem_qos(struct zxdh_sc_qp *qp);
struct zxdh_sc_qp *zxdh_get_qp_from_list(struct list_head *head,
					 struct zxdh_sc_qp *qp);
/* dynamic memory allocation */
/* misc */
u8 zxdh_get_encoded_wqe_size(u32 wqsize, enum zxdh_queue_type queue_type);
void zxdh_modify_qp_to_err(struct zxdh_sc_qp *sc_qp);
int zxdh_cfg_fpm_val(struct zxdh_sc_dev *dev);
void free_sd_mem(struct zxdh_sc_dev *dev);
int zxdh_process_cqp_cmd(struct zxdh_sc_dev *dev,
			 struct cqp_cmds_info *pcmdinfo);
int zxdh_process_bh(struct zxdh_sc_dev *dev);
extern void dump_ctx(struct zxdh_sc_dev *dev, u32 pf_num, u32 qp_num);
void dumpCSR(struct zxdh_sc_dev *dev);
void dumpCSRx(struct zxdh_sc_dev *dev);
void dumpcls(struct zxdh_sc_dev *dev);
void *zxdh_remove_cqp_head(struct zxdh_sc_dev *dev);

int zxdh_sc_config_pte_table(struct zxdh_sc_dev *dev,
			     struct zxdh_src_copy_dest scr_dest);
int zxdh_ext_sc_config_pte_table(struct zxdh_sc_dev *ext_dev,
			     struct zxdh_src_copy_dest src_dest);
int zxdh_cqp_config_pte_table_cmd(struct zxdh_sc_dev *dev,
				  struct zxdh_src_copy_dest scr_dest);

void zxdh_hmc_dpu_capability(struct zxdh_sc_dev *dev);
u32 zxdh_hmc_register_config_comval(struct zxdh_pci_f *rf, struct zxdh_sc_dev *dev, u32 rsrc_type);
u32 zxdh_hmc_register_config_cqpval(struct zxdh_sc_dev *dev, u32 max_cnt,
				    u32 rsrc_type);
u64 zxdh_get_path_index(struct zxdh_path_index *path_index);
int zxdh_cqp_config_pble_table_cmd(struct zxdh_sc_dev *dev,
				   struct zxdh_pble_info *pbleinfo, u32 len,
				   bool pbletype);
int zxdh_sc_send_mailbox_cmd(struct zxdh_sc_dev *dev, u8 opt, u64 msg2,
			     u64 msg3, u64 msg4, u16 dst_vf_id);
int zxdh_sc_commit_hmc_register_val(struct zxdh_sc_cqp *cqp, u64 scratch,
				    struct zxdh_path_index *dpath_index,
				    struct zxdh_dma_write32_date *dma_data,
				    bool post_sq, u8 wait_type);

int zxdh_sc_dma_read_usecqe(struct zxdh_sc_cqp *cqp, u64 scratch,
			    struct zxdh_dam_read_bycqe *readbuf,
			    struct zxdh_path_index *spath_index, bool post_sq);

int zxdh_sc_dma_read(struct zxdh_sc_cqp *cqp, u64 scratch,
		     struct zxdh_src_copy_dest *src_dest,
		     struct zxdh_path_index *spath_index,
		     struct zxdh_path_index *dpath_index, bool post_sq);

int zxdh_sc_dma_write64(struct zxdh_sc_cqp *cqp, u64 scratch,
			struct zxdh_path_index *dpath_index,
			struct zxdh_dma_write64_date *dma_data, bool post_sq);

int zxdh_sc_dma_write32(struct zxdh_sc_cqp *cqp, u64 scratch,
			struct zxdh_path_index *dpath_index,
			struct zxdh_dma_write32_date *dma_data, bool post_sq);

int zxdh_sc_dma_write(struct zxdh_sc_cqp *cqp, u64 scratch,
		      struct zxdh_src_copy_dest *src_dest,
		      struct zxdh_path_index *spath_index,
		      struct zxdh_path_index *dpath_index, bool post_sq);

int zxdh_sc_mb_create(struct zxdh_sc_cqp *cqp, u64 scratch,
		      struct zxdh_mailboxhead_data *mbhead_data, bool post_sq,
		      u32 dst_vf_id);
int zxdh_sc_query_mkey_cmd(struct zxdh_sc_dev *dev, u32 mekyindex);

int zxdh_clear_dpuddr(struct zxdh_sc_dev *dev, bool clear);
int zxdh_vf_clear_dpuddr(struct zxdh_sc_dev *dev, u64 size, bool clear);

int zxdh_clear_nof_ioq(struct zxdh_sc_dev *dev, u64 size, u64 ioq_pa);

int zxdh_dpuddr_to_host_cmd(struct zxdh_sc_dev *dev,
			    struct zxdh_src_copy_dest *src_dest);
int zxdh_cqp_rdma_write_cmd(struct zxdh_sc_dev *dev,
			    struct zxdh_src_copy_dest *src_dest, u8 src_dir,
			    u8 dest_dir);
int zxdh_cqp_rdma_read_cmd(struct zxdh_sc_dev *dev,
			   struct zxdh_src_copy_dest *src_dest, u8 src_dir,
			   u8 dest_dir);
int zxdh_cqp_damreadbycqe_cmd(struct zxdh_sc_dev *dev,
			      struct zxdh_dam_read_bycqe *dmadata,
			      struct zxdh_path_index *src_path_index, u64 *arr);
int zxdh_cqp_rdma_write32_cmd(struct zxdh_sc_dev *dev,
			      struct zxdh_dma_write32_date *dma_data);
int zxdh_cqp_rdma_readreg_cmd(struct zxdh_sc_dev *dev,
			      struct zxdh_src_copy_dest *src_dest);
int zxdh_cqp_rdma_read_mrte_cmd(struct zxdh_sc_dev *dev,
				struct zxdh_src_copy_dest *src_dest);
int zxdh_cqp_rdma_read_tx_window_cmd(struct zxdh_sc_dev *dev,
				     struct zxdh_src_copy_dest *src_dest);

int zxdh_read_ram_32bit_value(struct zxdh_sc_dev *dev, u32 ram_num,
				  u32 ram_width, u32 ram_read_cnt,
				  u32 offset_idx, u32 *value);

int zxdh_read_ram_tx_values(struct zxdh_sc_dev *dev,
			    struct read_ram_info *ram_info, u32 *value);
int zxdh_read_ram_rx_values(struct zxdh_sc_dev *dev,
			    struct read_ram_info *ram_info, u32 *value);
int zxdh_read_ram_cqp_values(struct zxdh_sc_dev *dev,
			    struct read_ram_info *ram_info, u32 *value);

u64 zxdh_get_hmc_align_512(u64 paaddr);
u16 zxdh_txwind_ddr_size(u8 num);

u64 zxdh_get_hmc_align_2M(u64 paaddr);
u64 zxdh_get_hmc_align_4K(u64 paaddr);
int zxdh_create_vf_pblehmc_entry(struct zxdh_sc_dev *dev);
int zxdh_sc_query_mkey(struct zxdh_sc_cqp *cqp, u32 mkeyindex, u64 scratch,
		       bool post_sq);

int zxdh_sc_query_qpc(struct zxdh_sc_dev *dev, u32 qpn, u64 qpc_buf_pa, u64 scratch,
		      bool post_sq);
int zxdh_sc_query_cqc(struct zxdh_sc_dev *dev, u32 cqn, u64 cqc_buf_pa, u64 scratch,
		      bool post_sq);
int zxdh_sc_query_ceqc(struct zxdh_sc_dev *dev, u32 ceqn, u64 ceqc_buf_pa, u64 scratch,
		       bool post_sq);
int zxdh_sc_query_aeqc(struct zxdh_sc_dev *dev, u16 aeqn, u64 aeqc_buf_pa, u64 scratch,
		       bool post_sq);

int zxdh_cq_round_up(u32 wqdepth);

int zxdh_cqp_aeq_create(struct zxdh_sc_aeq *aeq);
int zxdh_cqp_ext_aeq_create(struct zxdh_sc_aeq *ext_aeq);
int zxdh_init_destroy_aeq(struct zxdh_pci_f *rf);
int zxdh_init_destroy_ext_aeq(struct zxdh_pci_f *rf);
void zxdh_aeq_ena_intr(struct zxdh_sc_dev *dev, bool enable);
int zxdh_create_cqp_qp(struct zxdh_pci_f *rf);
int zxdh_create_ext_cqp_qp(struct zxdh_pci_f *rf);
int zxdh_destroy_cqp_qp(struct zxdh_pci_f *rf);
int zxdh_destroy_ext_cqp_qp(struct zxdh_pci_f *rf);
const char *zxdh_qp_state_to_string(enum ib_qp_state state);
int get_pci_board_bdf(char *pci_board_bdf, struct zxdh_pci_f *rf);
#endif /* ZXDH_PROTOS_H */
