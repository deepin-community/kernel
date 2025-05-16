/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_CRM_H
#define HINIC3_CRM_H

#include <linux/pci.h>

#include "mpu_cmd_base_defs.h"

#define HINIC3_DRV_VERSION "17.7.8.1"
#define HINIC3_DRV_DESC "Intelligent Network Interface Card Driver"
#define HIUDK_DRV_DESC "Intelligent Network Unified Driver"

#define ARRAY_LEN(arr) ((int)((int)sizeof(arr) / (int)sizeof((arr)[0])))

#define HINIC3_MGMT_VERSION_MAX_LEN 32

#define HINIC3_FW_VERSION_NAME 16
#define HINIC3_FW_VERSION_SECTION_CNT 4
#define HINIC3_FW_VERSION_SECTION_BORDER 0xFF
struct hinic3_fw_version {
	u8 mgmt_ver[HINIC3_FW_VERSION_NAME];
	u8 microcode_ver[HINIC3_FW_VERSION_NAME];
	u8 boot_ver[HINIC3_FW_VERSION_NAME];
};

#define HINIC3_MGMT_CMD_UNSUPPORTED 0xFF

/* show each drivers only such as nic_service_cap,
 * toe_service_cap structure, but not show service_cap
 */
enum hinic3_service_type {
	SERVICE_T_NIC = 0,
	SERVICE_T_OVS,
	SERVICE_T_ROCE,
	SERVICE_T_TOE,
	SERVICE_T_IOE,
	SERVICE_T_FC,
	SERVICE_T_VBS,
	SERVICE_T_IPSEC,
	SERVICE_T_VIRTIO,
	SERVICE_T_MIGRATE,
	SERVICE_T_PPA,
	SERVICE_T_CUSTOM,
	SERVICE_T_VROCE,
	SERVICE_T_CRYPT,
	SERVICE_T_VSOCK,
	SERVICE_T_BIFUR,
	SERVICE_T_MAX,

	/* Only used for interruption resource management,
	 * mark the request module
	 */
	SERVICE_T_INTF = (1 << 15),
	SERVICE_T_CQM = (1 << 16),
};

enum hinic3_ppf_flr_type {
	STATELESS_FLR_TYPE,
	STATEFUL_FLR_TYPE,
};

struct nic_service_cap {
	u16 max_sqs;
	u16 max_rqs;
	u16 default_num_queues;
	u16 outband_vlan_cfg_en;
	u8 lro_enable;
	u8 rsvd1[3];
};

struct ppa_service_cap {
	u16 qpc_fake_vf_start;
	u16 qpc_fake_vf_num;
	u32 qpc_fake_vf_ctx_num;
	u32 pctx_sz; /* 512B */
	u32 bloomfilter_length;
	u8 bloomfilter_en;
	u8 rsvd;
	u16 rsvd1;
};

struct bifur_service_cap {
	u8 rsvd;
};

struct vbs_service_cap {
	u16 vbs_max_volq;
	u8  vbs_main_pf_enable;
	u8  vbs_vsock_pf_enable;
	u8  vbs_fushion_queue_pf_enable;
};

struct migr_service_cap {
	u8 master_host_id;
	u8 rsvd[3];
};

/* PF/VF ToE service resource structure */
struct dev_toe_svc_cap {
	/* PF resources */
	u32 max_pctxs; /* Parent Context: max specifications 1M */
	u32 max_cctxt;
	u32 max_cqs;
	u16 max_srqs;
	u32 srq_id_start;
	u32 max_mpts;
};

/* ToE services */
struct toe_service_cap {
	struct dev_toe_svc_cap dev_toe_cap;

	bool alloc_flag;
	u32 pctx_sz; /* 1KB */
	u32 scqc_sz; /* 64B */
};

/* PF FC service resource structure defined */
struct dev_fc_svc_cap {
	/* PF Parent QPC */
	u32 max_parent_qpc_num; /* max number is 2048 */

	/* PF Child QPC */
	u32 max_child_qpc_num; /* max number is 2048 */
	u32 child_qpc_id_start;

	/* PF SCQ */
	u32 scq_num; /* 16 */

	/* PF supports SRQ */
	u32 srq_num; /* Number of SRQ is 2 */

	u8 vp_id_start;
	u8 vp_id_end;
};

/* FC services */
struct fc_service_cap {
	struct dev_fc_svc_cap dev_fc_cap;

	/* Parent QPC */
	u32 parent_qpc_size; /* 256B */

	/* Child QPC */
	u32 child_qpc_size; /* 256B */

	/* SQ */
	u32 sqe_size; /* 128B(in linked list mode) */

	/* SCQ */
	u32 scqc_size; /* Size of the Context 32B */
	u32 scqe_size; /* 64B */

	/* SRQ */
	u32 srqc_size; /* Size of SRQ Context (64B) */
	u32 srqe_size; /* 32B */
};

struct dev_roce_svc_own_cap {
	u32 max_qps;
	u32 max_cqs;
	u32 max_srqs;
	u32 max_mpts;
	u32 max_drc_qps;

	u32 cmtt_cl_start;
	u32 cmtt_cl_end;
	u32 cmtt_cl_sz;

	u32 dmtt_cl_start;
	u32 dmtt_cl_end;
	u32 dmtt_cl_sz;

	u32 wqe_cl_start;
	u32 wqe_cl_end;
	u32 wqe_cl_sz;

	u32 qpc_entry_sz;
	u32 max_wqes;
	u32 max_rq_sg;
	u32 max_sq_inline_data_sz;
	u32 max_rq_desc_sz;

	u32 rdmarc_entry_sz;
	u32 max_qp_init_rdma;
	u32 max_qp_dest_rdma;

	u32 max_srq_wqes;
	u32 reserved_srqs;
	u32 max_srq_sge;
	u32 srqc_entry_sz;

	u32 max_msg_sz; /* Message size 2GB */
};

/* RDMA service capability structure */
struct dev_rdma_svc_cap {
	/* ROCE service unique parameter structure */
	struct dev_roce_svc_own_cap roce_own_cap;
};

/* Defines the RDMA service capability flag */
enum {
	RDMA_BMME_FLAG_LOCAL_INV = (1 << 0),
	RDMA_BMME_FLAG_REMOTE_INV = (1 << 1),
	RDMA_BMME_FLAG_FAST_REG_WR = (1 << 2),
	RDMA_BMME_FLAG_RESERVED_LKEY = (1 << 3),
	RDMA_BMME_FLAG_TYPE_2_WIN = (1 << 4),
	RDMA_BMME_FLAG_WIN_TYPE_2B = (1 << 5),

	RDMA_DEV_CAP_FLAG_XRC = (1 << 6),
	RDMA_DEV_CAP_FLAG_MEM_WINDOW = (1 << 7),
	RDMA_DEV_CAP_FLAG_ATOMIC = (1 << 8),
	RDMA_DEV_CAP_FLAG_APM = (1 << 9),
};

/* RDMA services */
struct rdma_service_cap {
	struct dev_rdma_svc_cap dev_rdma_cap;

	u8 log_mtt;   /* 1. the number of MTT PA must be integer power of 2
		       * 2. represented by logarithm. Each MTT table can
		       * contain 1, 2, 4, 8, and 16 PA)
		       */
	/* todo: need to check whether related to max_mtt_seg */
	u32 num_mtts; /* Number of MTT table (4M),
		       * is actually MTT seg number
		       */
	u32 log_mtt_seg;
	u32 mtt_entry_sz; /* MTT table size 8B, including 1 PA(64bits) */
	u32 mpt_entry_sz; /* MPT table size (64B) */

	u32 dmtt_cl_start;
	u32 dmtt_cl_end;
	u32 dmtt_cl_sz;

	u8 log_rdmarc; /* 1. the number of RDMArc PA must be integer power of 2
			* 2. represented by logarithm. Each MTT table can
			* contain 1, 2, 4, 8, and 16 PA)
			*/

	u32 reserved_qps;   /* Number of reserved QP */
	u32 max_sq_sg;	    /* Maximum SGE number of SQ (8) */
	u32 max_sq_desc_sz; /* WQE maximum size of SQ(1024B), inline maximum
			     * size if 960B(944B aligned to the 960B),
			     * 960B=>wqebb alignment=>1024B
			     */
	u32 wqebb_size;	    /* Currently, the supports 64B and 128B,
			     * defined as 64Bytes
			     */

	u32 max_cqes;	  /* Size of the depth of the CQ (64K-1) */
	u32 reserved_cqs; /* Number of reserved CQ */
	u32 cqc_entry_sz; /* Size of the CQC (64B/128B) */
	u32 cqe_size;	  /* Size of CQE (32B) */

	u32 reserved_mrws; /* Number of reserved MR/MR Window */

	u32 max_fmr_maps; /* max MAP of FMR,
			   * (1 << (32-ilog2(num_mpt)))-1;
			   */

	/* todo: max value needs to be confirmed */
	/* MTT table number of Each MTT seg(3) */

	u32 log_rdmarc_seg; /* table number of each RDMArc seg(3) */

	/* Timeout time. Formula:Tr=4.096us*2(local_ca_ack_delay), [Tr,4Tr] */
	u32 local_ca_ack_delay;
	u32 num_ports; /* Physical port number */

	u32 db_page_size;    /* Size of the DB (4KB) */
	u32 direct_wqe_size; /* Size of the DWQE (256B) */

	u32 num_pds;	    /* Maximum number of PD (128K) */
	u32 reserved_pds;   /* Number of reserved PD */
	u32 max_xrcds;	    /* Maximum number of xrcd (64K) */
	u32 reserved_xrcds; /* Number of reserved xrcd */

	u32 max_gid_per_port; /* gid number (16) of each port */
	u32 gid_entry_sz;     /* RoCE v2 GID table is 32B,
			       * compatible RoCE v1 expansion
			       */

	u32 reserved_lkey;    /* local_dma_lkey */
	u32 num_comp_vectors; /* Number of complete vector (32) */
	u32 page_size_cap;    /* Supports 4K,8K,64K,256K,1M and 4M page_size */

	u32 flags;	  /* RDMA some identity */
	u32 max_frpl_len; /* Maximum number of pages frmr registration */
	u32 max_pkeys;	  /* Number of supported pkey group */
};

/* PF OVS service resource structure defined */
struct dev_ovs_svc_cap {
	u32 max_pctxs; /* Parent Context: max specifications 1M */
	u32 fake_vf_max_pctx;
	u16 fake_vf_num;
	u16 fake_vf_start_id;
	u8 dynamic_qp_en;
};

/* OVS services */
struct ovs_service_cap {
	struct dev_ovs_svc_cap dev_ovs_cap;

	u32 pctx_sz; /* 512B */
};

/* PF IPsec service resource structure defined */
struct dev_ipsec_svc_cap {
	u32 max_sactxs;	/* max IPsec SA context num */
	u16 max_cqs;	/* max IPsec SCQC num */
	u16 rsvd0;
};

/* IPsec services */
struct ipsec_service_cap {
	struct dev_ipsec_svc_cap dev_ipsec_cap;
	u32 sactx_sz; /* 512B */
};

/* Defines the IRQ information structure */
struct irq_info {
	u16 msix_entry_idx;	/* IRQ corresponding index number */
	u32 irq_id;		/* the IRQ number from OS */
};

struct interrupt_info {
	u32 lli_set;
	u32 interrupt_coalesc_set;
	u16 msix_index;
	u8 lli_credit_limit;
	u8 lli_timer_cfg;
	u8 pending_limt;
	u8 coalesc_timer_cfg;
	u8 resend_timer_cfg;
};

enum hinic3_msix_state {
	HINIC3_MSIX_ENABLE,
	HINIC3_MSIX_DISABLE,
};

enum hinic3_msix_auto_mask {
	HINIC3_CLR_MSIX_AUTO_MASK,
	HINIC3_SET_MSIX_AUTO_MASK,
};

enum func_type {
	TYPE_PF,
	TYPE_VF,
	TYPE_PPF,
	TYPE_UNKNOWN,
};

enum func_nic_state {
	HINIC3_FUNC_NIC_DEL,
	HINIC3_FUNC_NIC_ADD,
};

struct hinic3_init_para {
	/* Record hinic_pcidev or NDIS_Adapter pointer address */
	void *adapter_hdl;
	/* Record pcidev or Handler pointer address
	 * for example: ioremap interface input parameter
	 */
	void *pcidev_hdl;
	/* Record pcidev->dev or Handler pointer address which used to
	 * dma address application or dev_err print the parameter
	 */
	void *dev_hdl;

	/* Configure virtual address, PF is bar1, VF is bar0/1 */
	void *cfg_reg_base;
	/* interrupt configuration register address, PF is bar2, VF is bar2/3
	 */
	void *intr_reg_base;
	/* for PF bar3 virtual address, if function is VF should set to NULL */
	void *mgmt_reg_base;

	u64 db_dwqe_len;
	u64 db_base_phy;
	/* the doorbell address, bar4/5 higher 4M space */
	void *db_base;
	/* direct wqe 4M, follow the doorbell address space */
	void *dwqe_mapping;
	void **hwdev;
	void *chip_node;
	/* if use polling mode, set it true */
	bool poll;

	u16 probe_fault_level;
};

/* B200 config BAR45 4MB, DB & DWQE both 2MB */
#define HINIC3_DB_DWQE_SIZE 0x00400000

/* db/dwqe page size: 4K */
#define HINIC3_DB_PAGE_SIZE 0x00001000ULL
#define HINIC3_DWQE_OFFSET 0x00000800ULL

#define HINIC3_DB_MAX_AREAS (HINIC3_DB_DWQE_SIZE / HINIC3_DB_PAGE_SIZE)

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif
#define MAX_FUNCTION_NUM 4096

struct card_node {
	struct list_head node;
	struct list_head func_list;
	char chip_name[IFNAMSIZ];
	int chip_id;
	void *log_info;
	void *dbgtool_info;
	void *func_handle_array[MAX_FUNCTION_NUM];
	unsigned char bus_num;
	u16 func_num;
	u32 rsvd1;
	atomic_t channel_busy_cnt;
	void *priv_data;
	u64 rsvd2;
};

#define HINIC3_SYNFW_TIME_PERIOD	(60 * 60 * 1000)
#define HINIC3_SYNC_YEAR_OFFSET		1900
#define HINIC3_SYNC_MONTH_OFFSET	1

#define FAULT_SHOW_STR_LEN 16

enum hinic3_fault_source_type {
	/* same as FAULT_TYPE_CHIP */
	HINIC3_FAULT_SRC_HW_MGMT_CHIP = 0,
	/* same as FAULT_TYPE_UCODE */
	HINIC3_FAULT_SRC_HW_MGMT_UCODE,
	/* same as FAULT_TYPE_MEM_RD_TIMEOUT */
	HINIC3_FAULT_SRC_HW_MGMT_MEM_RD_TIMEOUT,
	/* same as FAULT_TYPE_MEM_WR_TIMEOUT */
	HINIC3_FAULT_SRC_HW_MGMT_MEM_WR_TIMEOUT,
	/* same as FAULT_TYPE_REG_RD_TIMEOUT */
	HINIC3_FAULT_SRC_HW_MGMT_REG_RD_TIMEOUT,
	/* same as FAULT_TYPE_REG_WR_TIMEOUT */
	HINIC3_FAULT_SRC_HW_MGMT_REG_WR_TIMEOUT,
	HINIC3_FAULT_SRC_SW_MGMT_UCODE,
	HINIC3_FAULT_SRC_MGMT_WATCHDOG,
	HINIC3_FAULT_SRC_MGMT_RESET = 8,
	HINIC3_FAULT_SRC_HW_PHY_FAULT,
	HINIC3_FAULT_SRC_TX_PAUSE_EXCP,
	HINIC3_FAULT_SRC_PCIE_LINK_DOWN = 20,
	HINIC3_FAULT_SRC_HOST_HEARTBEAT_LOST = 21,
	HINIC3_FAULT_SRC_TX_TIMEOUT,
	HINIC3_FAULT_SRC_TYPE_MAX,
};

union hinic3_fault_hw_mgmt {
	u32 val[4];
	/* valid only type == FAULT_TYPE_CHIP */
	struct {
		u8 node_id;
		/* enum hinic_fault_err_level */
		u8 err_level;
		u16 err_type;
		u32 err_csr_addr;
		u32 err_csr_value;
		/* func_id valid only if err_level == FAULT_LEVEL_SERIOUS_FLR */
		u8 rsvd1;
		u8 host_id;
		u16 func_id;
	} chip;

	/* valid only if type == FAULT_TYPE_UCODE */
	struct {
		u8 cause_id;
		u8 core_id;
		u8 c_id;
		u8 rsvd3;
		u32 epc;
		u32 rsvd4;
		u32 rsvd5;
	} ucode;

	/* valid only if type == FAULT_TYPE_MEM_RD_TIMEOUT ||
	 * FAULT_TYPE_MEM_WR_TIMEOUT
	 */
	struct {
		u32 err_csr_ctrl;
		u32 err_csr_data;
		u32 ctrl_tab;
		u32 mem_index;
	} mem_timeout;

	/* valid only if type == FAULT_TYPE_REG_RD_TIMEOUT ||
	 * FAULT_TYPE_REG_WR_TIMEOUT
	 */
	struct {
		u32 err_csr;
		u32 rsvd6;
		u32 rsvd7;
		u32 rsvd8;
	} reg_timeout;

	struct {
		/* 0: read; 1: write */
		u8 op_type;
		u8 port_id;
		u8 dev_ad;
		u8 rsvd9;
		u32 csr_addr;
		u32 op_data;
		u32 rsvd10;
	} phy_fault;
};

/* defined by chip */
struct hinic3_fault_event {
	/* enum hinic_fault_type */
	u8 type;
	u8 fault_level; /* sdk write fault level for uld event */
	u8 rsvd0[2];
	union hinic3_fault_hw_mgmt event;
};

struct hinic3_cmd_fault_event {
	u8 status;
	u8 version;
	u8 rsvd0[6];
	struct hinic3_fault_event event;
};

struct hinic3_sriov_state_info {
	u8 enable;
	u16 num_vfs;
};

enum hinic3_comm_event_type {
	EVENT_COMM_PCIE_LINK_DOWN,
	EVENT_COMM_HEART_LOST,
	EVENT_COMM_FAULT,
	EVENT_COMM_SRIOV_STATE_CHANGE,
	EVENT_COMM_CARD_REMOVE,
	EVENT_COMM_MGMT_WATCHDOG,
	EVENT_COMM_MULTI_HOST_MGMT,
};

enum hinic3_event_service_type {
	EVENT_SRV_COMM = 0,
#define SERVICE_EVENT_BASE	(EVENT_SRV_COMM + 1)
	EVENT_SRV_NIC = SERVICE_EVENT_BASE + SERVICE_T_NIC,
	EVENT_SRV_MIGRATE = SERVICE_EVENT_BASE + SERVICE_T_MIGRATE,
};

#define HINIC3_SRV_EVENT_TYPE(svc, type)	((((u32)(svc)) << 16) | (type))
#ifndef HINIC3_EVENT_DATA_SIZE
#define HINIC3_EVENT_DATA_SIZE 104
#endif
struct hinic3_event_info {
	u16 service; /* enum hinic3_event_service_type */
	u16 type;
	u8 event_data[HINIC3_EVENT_DATA_SIZE];
};

typedef void (*hinic3_event_handler)(void *handle, struct hinic3_event_info *event);

struct hinic3_func_nic_state {
	u8 state;
	u8 rsvd0;
	u16 func_idx;

	u8 vroce_flag;
	u8 rsvd1[15];
};

/* *
 * @brief hinic3_event_register - register hardware event
 * @param dev: device pointer to hwdev
 * @param pri_handle: private data will be used by the callback
 * @param callback: callback function
 */
void hinic3_event_register(void *dev, void *pri_handle,
			   hinic3_event_handler callback);

/* *
 * @brief hinic3_event_unregister - unregister hardware event
 * @param dev: device pointer to hwdev
 */
void hinic3_event_unregister(void *dev);

/* *
 * @brief hinic3_set_msix_auto_mask - set msix auto mask function
 * @param hwdev: device pointer to hwdev
 * @param msix_idx: msix id
 * @param flag: msix auto_mask flag, 1-enable, 2-clear
 */
void hinic3_set_msix_auto_mask_state(void *hwdev, u16 msix_idx,
				     enum hinic3_msix_auto_mask flag);

/* *
 * @brief hinic3_set_msix_state - set msix state
 * @param hwdev: device pointer to hwdev
 * @param msix_idx: msix id
 * @param flag: msix state flag, 0-enable, 1-disable
 */
void hinic3_set_msix_state(void *hwdev, u16 msix_idx,
			   enum hinic3_msix_state flag);

/* *
 * @brief hinic3_misx_intr_clear_resend_bit - clear msix resend bit
 * @param hwdev: device pointer to hwdev
 * @param msix_idx: msix id
 * @param clear_resend_en: 1-clear
 */
void hinic3_misx_intr_clear_resend_bit(void *hwdev, u16 msix_idx,
				       u8 clear_resend_en);

/* *
 * @brief hinic3_set_interrupt_cfg_direct - set interrupt cfg
 * @param hwdev: device pointer to hwdev
 * @param interrupt_para: interrupt info
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_interrupt_cfg_direct(void *hwdev,
				    struct interrupt_info *info,
				    u16 channel);

int hinic3_set_interrupt_cfg(void *dev, struct interrupt_info info,
			     u16 channel);

/* *
 * @brief hinic3_get_interrupt_cfg - get interrupt cfg
 * @param dev: device pointer to hwdev
 * @param info: interrupt info
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_interrupt_cfg(void *dev, struct interrupt_info *info,
			     u16 channel);

/* *
 * @brief hinic3_alloc_irqs - alloc irq
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 * @param num: alloc number
 * @param irq_info_array: alloc irq info
 * @param act_num: alloc actual number
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_alloc_irqs(void *hwdev, enum hinic3_service_type type, u16 num,
		      struct irq_info *irq_info_array, u16 *act_num);

/* *
 * @brief hinic3_free_irq - free irq
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 * @param irq_id: irq id
 */
void hinic3_free_irq(void *hwdev, enum hinic3_service_type type, u32 irq_id);

/* *
 * @brief hinic3_alloc_ceqs - alloc ceqs
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 * @param num: alloc ceq number
 * @param ceq_id_array: alloc ceq_id_array
 * @param act_num: alloc actual number
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_alloc_ceqs(void *hwdev, enum hinic3_service_type type, int num,
		      int *ceq_id_array, int *act_num);

/* *
 * @brief hinic3_free_irq - free ceq
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 * @param irq_id: ceq id
 */
void hinic3_free_ceq(void *hwdev, enum hinic3_service_type type, int ceq_id);

/* *
 * @brief hinic3_get_pcidev_hdl - get pcidev_hdl
 * @param hwdev: device pointer to hwdev
 * @retval non-null: success
 * @retval null: failure
 */
void *hinic3_get_pcidev_hdl(void *hwdev);

/* *
 * @brief hinic3_ppf_idx - get ppf id
 * @param hwdev: device pointer to hwdev
 * @retval ppf id
 */
u8 hinic3_ppf_idx(void *hwdev);

/* *
 * @brief hinic3_get_chip_present_flag - get chip present flag
 * @param hwdev: device pointer to hwdev
 * @retval 1: chip is present
 * @retval 0: chip is absent
 */
int hinic3_get_chip_present_flag(const void *hwdev);

/* *
 * @brief hinic3_get_heartbeat_status - get heartbeat status
 * @param hwdev: device pointer to hwdev
 * @retval heartbeat status
 */
u32 hinic3_get_heartbeat_status(void *hwdev);

/* *
 * @brief hinic3_support_nic - function support nic
 * @param hwdev: device pointer to hwdev
 * @param cap: nic service capbility
 * @retval true: function support nic
 * @retval false: function not support nic
 */
bool hinic3_support_nic(void *hwdev, struct nic_service_cap *cap);

/* *
 * @brief hinic3_support_ipsec - function support ipsec
 * @param hwdev: device pointer to hwdev
 * @param cap: ipsec service capbility
 * @retval true: function support ipsec
 * @retval false: function not support ipsec
 */
bool hinic3_support_ipsec(void *hwdev, struct ipsec_service_cap *cap);

/* *
 * @brief hinic3_support_roce - function support roce
 * @param hwdev: device pointer to hwdev
 * @param cap: roce service capbility
 * @retval true: function support roce
 * @retval false: function not support roce
 */
bool hinic3_support_roce(void *hwdev, struct rdma_service_cap *cap);

/* *
 * @brief hinic3_support_fc - function support fc
 * @param hwdev: device pointer to hwdev
 * @param cap: fc service capbility
 * @retval true: function support fc
 * @retval false: function not support fc
 */
bool hinic3_support_fc(void *hwdev, struct fc_service_cap *cap);

/* *
 * @brief hinic3_support_rdma - function support rdma
 * @param hwdev: device pointer to hwdev
 * @param cap: rdma service capbility
 * @retval true: function support rdma
 * @retval false: function not support rdma
 */
bool hinic3_support_rdma(void *hwdev, struct rdma_service_cap *cap);

/* *
 * @brief hinic3_support_ovs - function support ovs
 * @param hwdev: device pointer to hwdev
 * @param cap: ovs service capbility
 * @retval true: function support ovs
 * @retval false: function not support ovs
 */
bool hinic3_support_ovs(void *hwdev, struct ovs_service_cap *cap);

/* *
 * @brief hinic3_support_vbs - function support vbs
 * @param hwdev: device pointer to hwdev
 * @param cap: vbs service capbility
 * @retval true: function support vbs
 * @retval false: function not support vbs
 */
bool hinic3_support_vbs(void *hwdev, struct vbs_service_cap *cap);

/* *
 * @brief hinic3_support_toe - sync time to hardware
 * @param hwdev: device pointer to hwdev
 * @param cap: toe service capbility
 * @retval zero: success
 * @retval non-zero: failure
 */
bool hinic3_support_toe(void *hwdev, struct toe_service_cap *cap);

/* *
 * @brief hinic3_support_ppa - function support ppa
 * @param hwdev: device pointer to hwdev
 * @param cap: ppa service capbility
 * @retval zero: success
 * @retval non-zero: failure
 */
bool hinic3_support_ppa(void *hwdev, struct ppa_service_cap *cap);

/* *
 * @brief hinic3_support_bifur - function support bifur
 * @param hwdev: device pointer to hwdev
 * @param cap: bifur service capbility
 * @retval zero: success
 * @retval non-zero: failure
 */
bool hinic3_support_bifur(void *hwdev, struct bifur_service_cap *cap);

/* *
 * @brief hinic3_support_migr - function support migrate
 * @param hwdev: device pointer to hwdev
 * @param cap: migrate service capbility
 * @retval zero: success
 * @retval non-zero: failure
 */
bool hinic3_support_migr(void *hwdev, struct migr_service_cap *cap);

/* *
 * @brief hinic3_sync_time - sync time to hardware
 * @param hwdev: device pointer to hwdev
 * @param time: time to sync
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_sync_time(void *hwdev, u64 time);

/* *
 * @brief hinic3_disable_mgmt_msg_report - disable mgmt report msg
 * @param hwdev: device pointer to hwdev
 */
void hinic3_disable_mgmt_msg_report(void *hwdev);

/* *
 * @brief hinic3_func_for_mgmt - get function service type
 * @param hwdev: device pointer to hwdev
 * @retval true: function for mgmt
 * @retval false: function is not for mgmt
 */
bool hinic3_func_for_mgmt(void *hwdev);

/* *
 * @brief hinic3_set_pcie_order_cfg - set pcie order cfg
 * @param handle: device pointer to hwdev
 */
void hinic3_set_pcie_order_cfg(void *handle);

/* *
 * @brief hinic3_init_hwdev - call to init hwdev
 * @param para: device pointer to para
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_init_hwdev(struct hinic3_init_para *para);

/* *
 * @brief hinic3_free_hwdev - free hwdev
 * @param hwdev: device pointer to hwdev
 */
void hinic3_free_hwdev(void *hwdev);

/* *
 * @brief hinic3_detect_hw_present - detect hardware present
 * @param hwdev: device pointer to hwdev
 */
void hinic3_detect_hw_present(void *hwdev);

/* *
 * @brief hinic3_record_pcie_error - record pcie error
 * @param hwdev: device pointer to hwdev
 */
void hinic3_record_pcie_error(void *hwdev);

/* *
 * @brief hinic3_shutdown_hwdev - shutdown hwdev
 * @param hwdev: device pointer to hwdev
 */
void hinic3_shutdown_hwdev(void *hwdev);

/* *
 * @brief hinic3_set_ppf_flr_type - set ppf flr type
 * @param hwdev: device pointer to hwdev
 * @param ppf_flr_type: ppf flr type
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_ppf_flr_type(void *hwdev, enum hinic3_ppf_flr_type flr_type);

/* *
 * @brief hinic3_set_ppf_tbl_hotreplace_flag - set os hotreplace flag in ppf function table
 * @param hwdev: device pointer to hwdev
 * @param flag : os hotreplace flag : 0-not in os hotreplace 1-in os hotreplace
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_ppf_tbl_hotreplace_flag(void *hwdev, u8 flag);

/* *
 * @brief hinic3_get_mgmt_version - get management cpu version
 * @param hwdev: device pointer to hwdev
 * @param mgmt_ver: output management version
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_mgmt_version(void *hwdev, u8 *mgmt_ver, u8 version_size,
			    u16 channel);

/* *
 * @brief hinic3_get_fw_version - get firmware version
 * @param hwdev: device pointer to hwdev
 * @param fw_ver: firmware version
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_fw_version(void *hwdev, struct hinic3_fw_version *fw_ver,
			  u16 channel);

/* *
 * @brief hinic3_get_bond_create_mode - get bond create mode
 * @param hwdev: device pointer to hwdev
 * @retval global function id
 */
u8 hinic3_get_bond_create_mode(void *udkdev);

/* *
 * @brief hinic3_global_func_id - get global function id
 * @param hwdev: device pointer to hwdev
 * @retval global function id
 */
u16 hinic3_global_func_id(void *hwdev);

/* *
 * @brief hinic3_vector_to_eqn - vector to eq id
 * @param hwdev: device pointer to hwdev
 * @param type: service type
 * @param vector: vertor
 * @retval eq id
 */
int hinic3_vector_to_eqn(void *hwdev, enum hinic3_service_type type,
			 int vector);

/* *
 * @brief hinic3_glb_pf_vf_offset - get vf offset id of pf
 * @param hwdev: device pointer to hwdev
 * @retval vf offset id
 */
u16 hinic3_glb_pf_vf_offset(void *hwdev);

/* *
 * @brief hinic3_pf_id_of_vf - get pf id of vf
 * @param hwdev: device pointer to hwdev
 * @retval pf id
 */
u8 hinic3_pf_id_of_vf(void *hwdev);

/* *
 * @brief hinic3_func_type - get function type
 * @param hwdev: device pointer to hwdev
 * @retval function type
 */
enum func_type hinic3_func_type(void *hwdev);

/* *
 * @brief hinic3_get_stateful_enable - get stateful status
 * @param hwdev: device pointer to hwdev
 * @retval stateful enabel status
 */
bool hinic3_get_stateful_enable(void *hwdev);

/* *
 * @brief hinic3_get_timer_enable - get timer status
 * @param hwdev: device pointer to hwdev
 * @retval timer enabel status
 */
bool hinic3_get_timer_enable(void *hwdev);

/* *
 * @brief hinic3_host_oq_id_mask - get oq id
 * @param hwdev: device pointer to hwdev
 * @retval oq id
 */
u8 hinic3_host_oq_id_mask(void *hwdev);

/* *
 * @brief hinic3_host_id - get host id
 * @param hwdev: device pointer to hwdev
 * @retval host id
 */
u8 hinic3_host_id(void *hwdev);

/* *
 * @brief hinic3_func_max_qnum - get host total function number
 * @param hwdev: device pointer to hwdev
 * @retval non-zero: host total function number
 * @retval zero: failure
 */
u16 hinic3_host_total_func(void *hwdev);

/* *
 * @brief hinic3_func_max_qnum - get max nic queue number
 * @param hwdev: device pointer to hwdev
 * @retval non-zero: max nic queue number
 * @retval zero: failure
 */
u16 hinic3_func_max_nic_qnum(void *hwdev);

/* *
 * @brief hinic3_func_max_qnum - get max queue number
 * @param hwdev: device pointer to hwdev
 * @retval non-zero: max queue number
 * @retval zero: failure
 */
u16 hinic3_func_max_qnum(void *hwdev);

/* *
 * @brief hinic3_er_id - get ep id
 * @param hwdev: device pointer to hwdev
 * @retval ep id
 */
u8 hinic3_ep_id(void *hwdev); /* Obtain service_cap.ep_id */

/* *
 * @brief hinic3_er_id - get er id
 * @param hwdev: device pointer to hwdev
 * @retval er id
 */
u8 hinic3_er_id(void *hwdev); /* Obtain service_cap.er_id */

/* *
 * @brief hinic3_physical_port_id - get physical port id
 * @param hwdev: device pointer to hwdev
 * @retval physical port id
 */
u8 hinic3_physical_port_id(void *hwdev); /* Obtain service_cap.port_id */

/* *
 * @brief hinic3_func_max_vf - get vf number
 * @param hwdev: device pointer to hwdev
 * @retval non-zero: vf number
 * @retval zero: failure
 */
u16 hinic3_func_max_vf(void *hwdev); /* Obtain service_cap.max_vf */

/* *
 * @brief hinic3_max_pf_num - get global max pf number
 */
u8 hinic3_max_pf_num(void *hwdev);

/* *
 * @brief hinic3_host_pf_num - get current host pf number
 * @param hwdev: device pointer to hwdev
 * @retval non-zero: pf number
 * @retval zero: failure
 */
u32 hinic3_host_pf_num(void *hwdev); /* Obtain service_cap.pf_num */

/* *
 * @brief hinic3_host_pf_id_start - get current host pf id start
 * @param hwdev: device pointer to hwdev
 * @retval non-zero: pf id start
 * @retval zero: failure
 */
u32 hinic3_host_pf_id_start(void *hwdev); /* Obtain service_cap.pf_num */

/* *
 * @brief hinic3_pcie_itf_id - get pcie port id
 * @param hwdev: device pointer to hwdev
 * @retval pcie port id
 */
u8 hinic3_pcie_itf_id(void *hwdev);

/* *
 * @brief hinic3_vf_in_pf - get vf offset in pf
 * @param hwdev: device pointer to hwdev
 * @retval vf offset in pf
 */
u8 hinic3_vf_in_pf(void *hwdev);

/* *
 * @brief hinic3_cos_valid_bitmap - get cos valid bitmap
 * @param hwdev: device pointer to hwdev
 * @retval non-zero: valid cos bit map
 * @retval zero: failure
 */
int hinic3_cos_valid_bitmap(void *hwdev, u8 *func_dft_cos, u8 *port_cos_bitmap);

/* *
 * @brief hinic3_stateful_init - init stateful resource
 * @param hwdev: device pointer to hwdev
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_stateful_init(void *hwdev);

/* *
 * @brief hinic3_stateful_deinit - deinit stateful resource
 * @param hwdev: device pointer to hwdev
 */
void hinic3_stateful_deinit(void *hwdev);

/* *
 * @brief hinic3_free_stateful - sdk remove free stateful resource
 * @param hwdev: device pointer to hwdev
 */
void hinic3_free_stateful(void *hwdev);

/* *
 * @brief hinic3_need_init_stateful_default - get need init stateful default
 * @param hwdev: device pointer to hwdev
 */
bool hinic3_need_init_stateful_default(void *hwdev);

/* *
 * @brief hinic3_get_card_present_state - get card present state
 * @param hwdev: device pointer to hwdev
 * @param card_present_state: return card present state
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_card_present_state(void *hwdev, bool *card_present_state);

/* *
 * @brief hinic3_func_rx_tx_flush - function flush
 * @param hwdev: device pointer to hwdev
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_func_rx_tx_flush(void *hwdev, u16 channel, bool wait_io);

/* *
 * @brief hinic3_flush_mgmt_workq - when remove function should flush work queue
 * @param hwdev: device pointer to hwdev
 */
void hinic3_flush_mgmt_workq(void *hwdev);

/* *
 * @brief hinic3_ceq_num get toe ceq num
 */
u8 hinic3_ceq_num(void *hwdev);

/* *
 * @brief hinic3_intr_num get intr num
 */
u16 hinic3_intr_num(void *hwdev);

/* *
 * @brief hinic3_flexq_en get flexq en
 */
u8 hinic3_flexq_en(void *hwdev);

/* *
 * @brief hinic3_get_fake_vf_info get fake_vf info
 */
int hinic3_get_fake_vf_info(void *hwdev, u8 *fake_vf_vld,
			    u8 *page_bit, u8 *pf_start_bit, u8 *map_host_id);

/* *
 * @brief hinic3_fault_event_report - report fault event
 * @param hwdev: device pointer to hwdev
 * @param src: fault event source, reference to enum hinic3_fault_source_type
 * @param level: fault level, reference to enum hinic3_fault_err_level
 */
void hinic3_fault_event_report(void *hwdev, u16 src, u16 level);

/* *
 * @brief hinic3_probe_success - notify device probe successful
 * @param hwdev: device pointer to hwdev
 */
void hinic3_probe_success(void *hwdev);

/* *
 * @brief hinic3_set_func_svc_used_state - set function service used state
 * @param hwdev: device pointer to hwdev
 * @param svc_type: service type
 * @param state: function used state
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_func_svc_used_state(void *hwdev, u16 svc_type, u8 state,
				   u16 channel);

/* *
 * @brief hinic3_get_self_test_result - get self test result
 * @param hwdev: device pointer to hwdev
 * @retval self test result
 */
u32 hinic3_get_self_test_result(void *hwdev);

/* *
 * @brief hinic3_get_slave_host_enable - get slave host enable
 * @param hwdev: device pointer to hwdev
 * @param host_id: set host id
 * @param slave_en-zero: slave is enable
 * @retval zero: failure
 */
void hinic3_set_slave_host_enable(void *hwdev, u8 host_id, bool enable);

/* *
 * @brief hinic3_get_slave_bitmap - get slave host bitmap
 * @param hwdev: device pointer to hwdev
 * @param slave_host_bitmap-zero: slave host bitmap
 * @retval zero: failure
 */
int hinic3_get_slave_bitmap(void *hwdev, u8 *slave_host_bitmap);

/* *
 * @brief hinic3_get_slave_host_enable - get slave host enable
 * @param hwdev: device pointer to hwdev
 * @param host_id: get host id
 * @param slave_en-zero: slave is enable
 * @retval zero: failure
 */
int hinic3_get_slave_host_enable(void *hwdev, u8 host_id, u8 *slave_en);

/* *
 * @brief hinic3_set_host_migrate_enable - set migrate host enable
 * @param hwdev: device pointer to hwdev
 * @param host_id: get host id
 * @param slave_en-zero: migrate is enable
 * @retval zero: failure
 */
int hinic3_set_host_migrate_enable(void *hwdev, u8 host_id, bool enable);

/* *
 * @brief hinic3_get_host_migrate_enable - get migrate host enable
 * @param hwdev: device pointer to hwdev
 * @param host_id: get host id
 * @param slave_en-zero: migrte enable ptr
 * @retval zero: failure
 */
int hinic3_get_host_migrate_enable(void *hwdev, u8 host_id, u8 *migrate_en);

/* *
 * @brief hinic3_is_slave_func - hwdev is slave func
 * @param dev: device pointer to hwdev
 * @param is_slave_func: slave func
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_is_slave_func(const void *hwdev, bool *is_slave_func);

/* *
 * @brief hinic3_is_master_func - hwdev is master func
 * @param dev: device pointer to hwdev
 * @param is_master_func: master func
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_is_master_func(const void *hwdev, bool *is_master_func);

bool hinic3_is_multi_bm(void *hwdev);

bool hinic3_is_slave_host(void *hwdev);

bool hinic3_is_vm_slave_host(void *hwdev);

bool hinic3_is_bm_slave_host(void *hwdev);

bool hinic3_is_guest_vmsec_enable(void *hwdev);

int hinic3_get_vfid_by_vfpci(void *hwdev, struct pci_dev *pdev, u16 *global_func_id);

int hinic3_set_func_nic_state(void *hwdev, struct hinic3_func_nic_state *state);

int hinic3_get_netdev_state(void *hwdev, u16 func_idx, int *opened);

int hinic3_get_mhost_func_nic_enable(void *hwdev, u16 func_id, bool *en);

int hinic3_get_dev_cap(void *hwdev);

int hinic3_mbox_to_host_sync(void *hwdev, enum hinic3_mod_type mod,
			     u8 cmd, void *buf_in, u16 in_size,
			     void *buf_out, u16 *out_size, u32 timeout, u16 channel);

int hinic3_get_func_vroce_enable(void *hwdev, u16 glb_func_idx, u8 *en);

void hinic3_module_get(void *hwdev, enum hinic3_service_type type);
void hinic3_module_put(void *hwdev, enum hinic3_service_type type);

#endif
