/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_MT_H
#define HINIC3_MT_H

#define HINIC3_DRV_NAME "hinic3"
#define HINIC3_CHIP_NAME "hinic"
/* Interrupt at most records, interrupt will be recorded in the FFM */

#define NICTOOL_CMD_TYPE (0x18)
#define HINIC3_CARD_NAME_MAX_LEN (128)

struct api_cmd_rd {
	u32 pf_id;
	u8 dest;
	u8 *cmd;
	u16 size;
	void *ack;
	u16 ack_size;
};

struct api_cmd_wr {
	u32 pf_id;
	u8 dest;
	u8 *cmd;
	u16 size;
};

#define PF_DEV_INFO_NUM  32

struct pf_dev_info {
	u64 bar0_size;
	u8 bus;
	u8 slot;
	u8 func;
	u64 phy_addr;
};

/* Indicates the maximum number of interrupts that can be recorded.
 * Subsequent interrupts are not recorded in FFM.
 */
#define FFM_RECORD_NUM_MAX 64

struct ffm_intr_info {
	u8 node_id;
	/* error level of the interrupt source */
	u8 err_level;
	/* Classification by interrupt source properties */
	u16 err_type;
	u32 err_csr_addr;
	u32 err_csr_value;
};

struct ffm_intr_tm_info {
	struct ffm_intr_info intr_info;
	u8 times;
	u8 sec;
	u8 min;
	u8 hour;
	u8 mday;
	u8 mon;
	u16 year;
};

struct ffm_record_info {
	u32 ffm_num;
	u32 last_err_csr_addr;
	u32 last_err_csr_value;
	struct ffm_intr_tm_info ffm[FFM_RECORD_NUM_MAX];
};

struct dbgtool_k_glb_info {
	struct semaphore dbgtool_sem;
	struct ffm_record_info *ffm;
};

struct msg_2_up {
	u8 pf_id;
	u8 mod;
	u8 cmd;
	void *buf_in;
	u16 in_size;
	void *buf_out;
	u16 *out_size;
};

struct dbgtool_param {
	union {
		struct api_cmd_rd api_rd;
		struct api_cmd_wr api_wr;
		struct pf_dev_info *dev_info;
		struct ffm_record_info *ffm_rd;
		struct msg_2_up msg2up;
	} param;
	char chip_name[16];
};

/* dbgtool command type */
/* You can add commands as required. The dbgtool command can be
 * used to invoke all interfaces of the kernel-mode x86 driver.
 */
enum dbgtool_cmd {
	DBGTOOL_CMD_API_RD = 0,
	DBGTOOL_CMD_API_WR,
	DBGTOOL_CMD_FFM_RD,
	DBGTOOL_CMD_FFM_CLR,
	DBGTOOL_CMD_PF_DEV_INFO_GET,
	DBGTOOL_CMD_MSG_2_UP,
	DBGTOOL_CMD_FREE_MEM,
	DBGTOOL_CMD_NUM
};

#define HINIC_PF_MAX_SIZE (16)
#define HINIC_VF_MAX_SIZE (4096)
#define BUSINFO_LEN (32)

enum module_name {
	SEND_TO_NPU = 1,
	SEND_TO_MPU,
	SEND_TO_SM,

	SEND_TO_HW_DRIVER,
#define SEND_TO_SRV_DRV_BASE (SEND_TO_HW_DRIVER + 1)
	SEND_TO_NIC_DRIVER = SEND_TO_SRV_DRV_BASE,
	SEND_TO_OVS_DRIVER,
	SEND_TO_ROCE_DRIVER,
	SEND_TO_TOE_DRIVER,
	SEND_TO_IOE_DRIVER,
	SEND_TO_FC_DRIVER,
	SEND_TO_VBS_DRIVER,
	SEND_TO_IPSEC_DRIVER,
	SEND_TO_VIRTIO_DRIVER,
	SEND_TO_MIGRATE_DRIVER,
	SEND_TO_PPA_DRIVER,
	SEND_TO_CUSTOM_DRIVER = SEND_TO_SRV_DRV_BASE + 11,
	SEND_TO_VSOCK_DRIVER = SEND_TO_SRV_DRV_BASE + 14,
	SEND_TO_BIFUR_DRIVER,
	SEND_TO_DRIVER_MAX = SEND_TO_SRV_DRV_BASE + 16, /* reserved */
};

enum driver_cmd_type {
	TX_INFO = 1,
	Q_NUM,
	TX_WQE_INFO,
	TX_MAPPING,
	RX_INFO,
	RX_WQE_INFO,
	RX_CQE_INFO,
	UPRINT_FUNC_EN,
	UPRINT_FUNC_RESET,
	UPRINT_SET_PATH,
	UPRINT_GET_STATISTICS,
	FUNC_TYPE,
	GET_FUNC_IDX,
	GET_INTER_NUM,
	CLOSE_TX_STREAM,
	GET_DRV_VERSION,
	CLEAR_FUNC_STASTIC,
	GET_HW_STATS,
	CLEAR_HW_STATS,
	GET_SELF_TEST_RES,
	GET_CHIP_FAULT_STATS,
	NIC_RSVD1,
	NIC_RSVD2,
	GET_OS_HOT_REPLACE_INFO,
	GET_CHIP_ID,
	GET_SINGLE_CARD_INFO,
	GET_FIRMWARE_ACTIVE_STATUS,
	ROCE_DFX_FUNC,
	GET_DEVICE_ID,
	GET_PF_DEV_INFO,
	CMD_FREE_MEM,
	GET_LOOPBACK_MODE = 32,
	SET_LOOPBACK_MODE,
	SET_LINK_MODE,
	SET_TX_PF_BW_LIMIT,
	GET_PF_BW_LIMIT,
	ROCE_CMD,
	GET_POLL_WEIGHT,
	SET_POLL_WEIGHT,
	GET_HOMOLOGUE,
	SET_HOMOLOGUE,
	GET_SSET_COUNT,
	GET_SSET_ITEMS,
	IS_DRV_IN_VM,
	LRO_ADPT_MGMT,
	SET_INTER_COAL_PARAM,
	GET_INTER_COAL_PARAM,
	GET_CHIP_INFO,
	GET_NIC_STATS_LEN,
	GET_NIC_STATS_STRING,
	GET_NIC_STATS_INFO,
	GET_PF_ID,
	GET_MBOX_CNT,
	NIC_RSVD4,
	NIC_RSVD5,
	DCB_QOS_INFO,
	DCB_PFC_STATE,
	DCB_ETS_STATE,
	DCB_STATE,
	QOS_DEV,
	GET_QOS_COS,
	GET_ULD_DEV_NAME,
	GET_TX_TIMEOUT,
	SET_TX_TIMEOUT,

	RSS_CFG = 0x40,
	RSS_INDIR,
	PORT_ID,

	SET_RX_PF_BW_LIMIT = 0x43,

	GET_FUNC_CAP = 0x50,
	GET_XSFP_PRESENT = 0x51,
	GET_XSFP_INFO = 0x52,
	DEV_NAME_TEST = 0x53,
	GET_XSFP_INFO_COMP_CMIS = 0x54,

	GET_WIN_STAT = 0x60,
	WIN_CSR_READ = 0x61,
	WIN_CSR_WRITE = 0x62,
	WIN_API_CMD_RD = 0x63,

	GET_FUSION_Q = 0x64,

	ROCE_CMD_BOND_HASH_TYPE_SET = 0xb2,

	BIFUR_SET_ENABLE = 0xc0,
	BIFUR_GET_ENABLE = 0xc1,

	VM_COMPAT_TEST = 0xFF
};

enum api_chain_cmd_type {
	API_CSR_READ,
	API_CSR_WRITE
};

enum sm_cmd_type {
	SM_CTR_RD16 = 1,
	SM_CTR_RD32,
	SM_CTR_RD64_PAIR,
	SM_CTR_RD64,
	SM_CTR_RD32_CLEAR,
	SM_CTR_RD64_PAIR_CLEAR,
	SM_CTR_RD64_CLEAR,
	SM_CTR_RD16_CLEAR,
};

struct cqm_stats {
	atomic_t cqm_cmd_alloc_cnt;
	atomic_t cqm_cmd_free_cnt;
	atomic_t cqm_send_cmd_box_cnt;
	atomic_t cqm_send_cmd_imm_cnt;
	atomic_t cqm_db_addr_alloc_cnt;
	atomic_t cqm_db_addr_free_cnt;
	atomic_t cqm_fc_srq_create_cnt;
	atomic_t cqm_srq_create_cnt;
	atomic_t cqm_rq_create_cnt;
	atomic_t cqm_qpc_mpt_create_cnt;
	atomic_t cqm_nonrdma_queue_create_cnt;
	atomic_t cqm_rdma_queue_create_cnt;
	atomic_t cqm_rdma_table_create_cnt;
	atomic_t cqm_qpc_mpt_delete_cnt;
	atomic_t cqm_nonrdma_queue_delete_cnt;
	atomic_t cqm_rdma_queue_delete_cnt;
	atomic_t cqm_rdma_table_delete_cnt;
	atomic_t cqm_func_timer_clear_cnt;
	atomic_t cqm_func_hash_buf_clear_cnt;
	atomic_t cqm_scq_callback_cnt;
	atomic_t cqm_ecq_callback_cnt;
	atomic_t cqm_nocq_callback_cnt;
	atomic_t cqm_aeq_callback_cnt[112];
};

struct link_event_stats {
	atomic_t link_down_stats;
	atomic_t link_up_stats;
};

enum hinic3_fault_err_level {
	FAULT_LEVEL_FATAL,
	FAULT_LEVEL_SERIOUS_RESET,
	FAULT_LEVEL_HOST,
	FAULT_LEVEL_SERIOUS_FLR,
	FAULT_LEVEL_GENERAL,
	FAULT_LEVEL_SUGGESTION,
	FAULT_LEVEL_MAX,
};

enum hinic3_fault_type {
	FAULT_TYPE_CHIP,
	FAULT_TYPE_UCODE,
	FAULT_TYPE_MEM_RD_TIMEOUT,
	FAULT_TYPE_MEM_WR_TIMEOUT,
	FAULT_TYPE_REG_RD_TIMEOUT,
	FAULT_TYPE_REG_WR_TIMEOUT,
	FAULT_TYPE_PHY_FAULT,
	FAULT_TYPE_TSENSOR_FAULT,
	FAULT_TYPE_MAX,
};

struct fault_event_stats {
	atomic_t chip_fault_stats[22][FAULT_LEVEL_MAX];
	atomic_t fault_type_stat[FAULT_TYPE_MAX];
	atomic_t pcie_fault_stats;
};

enum hinic3_ucode_event_type {
	HINIC3_INTERNAL_OTHER_FATAL_ERROR = 0x0,
	HINIC3_CHANNEL_BUSY = 0x7,
	HINIC3_NIC_FATAL_ERROR_MAX = 0x8,
};

struct hinic3_hw_stats {
	atomic_t heart_lost_stats;
	struct cqm_stats cqm_stats;
	struct link_event_stats link_event_stats;
	struct fault_event_stats fault_event_stats;
	atomic_t nic_ucode_event_stats[HINIC3_NIC_FATAL_ERROR_MAX];
};

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

struct pf_info {
	char name[IFNAMSIZ];
	char bus_info[BUSINFO_LEN];
	u32 pf_type;
};

struct card_info {
	struct pf_info pf[HINIC_PF_MAX_SIZE];
	u32 pf_num;
};

struct func_mbox_cnt_info {
	char bus_info[BUSINFO_LEN];
	u64 send_cnt;
	u64 ack_cnt;
};

struct card_mbox_cnt_info {
	struct func_mbox_cnt_info func_info[HINIC_PF_MAX_SIZE +
					    HINIC_VF_MAX_SIZE];
	u32 func_num;
};

struct hinic3_nic_loop_mode {
	u32 loop_mode;
	u32 loop_ctrl;
};

struct hinic3_pf_info {
	u32 isvalid;
	u32 pf_id;
};

enum hinic3_show_set {
	HINIC3_SHOW_SSET_IO_STATS = 1,
};

#define HINIC3_SHOW_ITEM_LEN 32
struct hinic3_show_item {
	char name[HINIC3_SHOW_ITEM_LEN];
	u8 hexadecimal; /* 0: decimal , 1: Hexadecimal */
	u8 rsvd[7];
	u64 value;
};

#define HINIC3_CHIP_FAULT_SIZE (110 * 1024)
#define MAX_DRV_BUF_SIZE 4096

struct nic_cmd_chip_fault_stats {
	u32 offset;
	u8 chip_fault_stats[MAX_DRV_BUF_SIZE];
};

#define NIC_TOOL_MAGIC 'x'

#define CARD_MAX_SIZE (64)

struct nic_card_id {
	u32 id[CARD_MAX_SIZE];
	u32 num;
};

struct func_pdev_info {
	u64 bar0_phy_addr;
	u64 bar0_size;
	u64 bar1_phy_addr;
	u64 bar1_size;
	u64 bar3_phy_addr;
	u64 bar3_size;
	u64 rsvd1[4];
};

struct hinic3_card_func_info {
	u32 num_pf;
	u32 rsvd0;
	u64 usr_api_phy_addr;
	struct func_pdev_info pdev_info[CARD_MAX_SIZE];
};

struct wqe_info {
	int q_id;
	void *slq_handle;
	unsigned int wqe_id;
};

#define MAX_VER_INFO_LEN 128
struct drv_version_info {
	char ver[MAX_VER_INFO_LEN];
};

struct hinic3_tx_hw_page {
	u64 phy_addr;
	u64 *map_addr;
};

struct nic_sq_info {
	u16 q_id;
	u16 pi;
	u16 ci; /* sw_ci */
	u16 fi; /* hw_ci */
	u32 q_depth;
	u16 pi_reverse; /* TODO: what is this? */
	u16 wqebb_size;
	u8 priority;
	u16 *ci_addr;
	u64 cla_addr;
	void *slq_handle;
	/* TODO: NIC don't use direct wqe */
	struct hinic3_tx_hw_page direct_wqe;
	struct hinic3_tx_hw_page doorbell;
	u32 page_idx;
	u32 glb_sq_id;
};

struct nic_rq_info {
	u16 q_id;
	u16 delta;
	u16 hw_pi;
	u16 ci; /* sw_ci */
	u16 sw_pi;
	u16 wqebb_size;
	u16 q_depth;
	u16 buf_len;

	void *slq_handle;
	u64 ci_wqe_page_addr;
	u64 ci_cla_tbl_addr;

	u8 coalesc_timer_cfg;
	u8 pending_limt;
	u16 msix_idx;
	u32 msix_vector;
};

#define MT_EPERM        1       /* Operation not permitted */
#define MT_EIO          2       /* I/O error */
#define MT_EINVAL       3       /* Invalid argument */
#define	MT_EBUSY        4       /* Device or resource busy */
#define MT_EOPNOTSUPP   0xFF    /* Operation not supported */

struct mt_msg_head {
	u8 status;
	u8 rsvd1[3];
};

#define MT_DCB_OPCODE_WR   BIT(0)  /* 1 - write, 0 - read */
struct hinic3_mt_qos_info { /* delete */
	struct mt_msg_head head;

	u16 op_code;
	u8 valid_cos_bitmap;
	u8 valid_up_bitmap;
	u32 rsvd1;
};

struct hinic3_mt_dcb_state {
	struct mt_msg_head head;

	u16 op_code; /* 0 - get dcb state, 1 - set dcb state */
	u8 state;    /* 0 - disable,       1 - enable dcb  */
	u8 rsvd;
};

#define MT_DCB_ETS_UP_TC      BIT(1)
#define MT_DCB_ETS_UP_BW      BIT(2)
#define MT_DCB_ETS_UP_PRIO    BIT(3)
#define MT_DCB_ETS_TC_BW      BIT(4)
#define MT_DCB_ETS_TC_PRIO    BIT(5)

#define DCB_UP_TC_NUM         0x8
struct hinic3_mt_ets_state { /* delete */
	struct mt_msg_head head;

	u16 op_code;
	u8 up_tc[DCB_UP_TC_NUM];
	u8 up_bw[DCB_UP_TC_NUM];
	u8 tc_bw[DCB_UP_TC_NUM];
	u8 up_prio_bitmap;
	u8 tc_prio_bitmap;
	u32 rsvd;
};

#define MT_DCB_PFC_PFC_STATE  BIT(1)
#define MT_DCB_PFC_PFC_PRI_EN BIT(2)

struct hinic3_mt_pfc_state { /* delete */
	struct mt_msg_head head;

	u16 op_code;
	u8 state;
	u8 pfc_en_bitpamp;
	u32 rsvd;
};

#define CMD_QOS_DEV_TRUST     BIT(0)
#define CMD_QOS_DEV_DFT_COS   BIT(1)
#define CMD_QOS_DEV_PCP2COS   BIT(2)
#define CMD_QOS_DEV_DSCP2COS  BIT(3)

struct hinic3_mt_qos_dev_cfg {
	struct mt_msg_head head;

	u8 op_code;       /* 0：get 1: set */
	u8 rsvd0;
	/* bit0 - trust, bit1 - dft_cos, bit2 - pcp2cos, bit3 - dscp2cos */
	u16 cfg_bitmap;

	u8 trust;         /* 0 - pcp, 1 - dscp */
	u8 dft_cos;
	u16 rsvd1;
	u8 pcp2cos[8];    /* 必须8个一起配置 */
	/* 配置dscp2cos时，若cos值设置为0xFF，驱动则忽略此dscp优先级的配置，
	 * 允许一次性配置多个dscp跟cos的映射关系
	 */
	u8 dscp2cos[64];
	u32 rsvd2[4];
};

enum mt_api_type {
	API_TYPE_MBOX = 1,
	API_TYPE_API_CHAIN_BYPASS,
	API_TYPE_API_CHAIN_TO_MPU,
	API_TYPE_CLP,
};

struct npu_cmd_st {
	u32 mod : 8;
	u32 cmd : 8;
	u32 ack_type : 3;
	u32 direct_resp : 1;
	u32 len : 12;
};

struct mpu_cmd_st {
	u32 api_type : 8;
	u32 mod : 8;
	u32 cmd : 16;
};

struct msg_module {
	char device_name[IFNAMSIZ];
	u32 module;
	union {
		u32 msg_formate; /* for driver */
		struct npu_cmd_st npu_cmd;
		struct mpu_cmd_st mpu_cmd;
	};
	u32 timeout; /* for mpu/npu cmd */
	u32 func_idx;
	u32 buf_in_size;
	u32 buf_out_size;
	void *in_buf;
	void *out_buf;
	int bus_num;
	u8 port_id;
	u8 rsvd1[3];
	u32 rsvd2[4];
};

struct hinic3_mt_qos_cos_cfg {
	struct mt_msg_head head;

	u8 port_id;
	u8 func_cos_bitmap;
	u8 port_cos_bitmap;
	u8 func_max_cos_num;
	u32 rsvd2[4];
};

#define MAX_NETDEV_NUM 4

enum hinic3_bond_cmd_to_custom_e {
	CMD_CUSTOM_BOND_DEV_CREATE = 1,
	CMD_CUSTOM_BOND_DEV_DELETE,
	CMD_CUSTOM_BOND_GET_CHIP_NAME,
	CMD_CUSTOM_BOND_GET_CARD_INFO
};

enum xmit_hash_policy {
	HASH_POLICY_L2   = 0, /* SMAC_DMAC */
	HASH_POLICY_L23  = 1, /* SIP_DIP_SPORT_DPORT */
	HASH_POLICY_L34  = 2, /* SMAC_DMAC_SIP_DIP */
	HASH_POLICY_MAX  = 3  /* MAX */
};

/* bond mode */
enum tag_bond_mode {
	BOND_MODE_NONE      = 0, /**< bond disable */
	BOND_MODE_BACKUP    = 1, /**< 1 for active-backup */
	BOND_MODE_BALANCE   = 2, /**< 2 for balance-xor */
	BOND_MODE_LACP      = 4, /**< 4 for 802.3ad */
	BOND_MODE_MAX
};

struct add_bond_dev_s {
	struct mt_msg_head head;
	/* input can be empty, indicates that the value
	 * is assigned by the driver
	 */
	char bond_name[IFNAMSIZ];
	u8 slave_cnt;
	u8 rsvd[3];
	char slave_name[MAX_NETDEV_NUM][IFNAMSIZ]; /* unit : ms */
	u32 poll_timeout; /* default value = 100 */
	u32 up_delay;     /* default value = 0 */
	u32 down_delay;   /* default value = 0 */
	u32 bond_mode;    /* default value = BOND_MODE_LACP */

	/* maximum number of active bond member interfaces,
	 * default value = 0
	 */
	u32 active_port_max_num;
	/* minimum number of active bond member interfaces,
	 * default value = 0
	 */
	u32 active_port_min_num;
	/* hash policy, which is used for microcode routing logic,
	 * default value = 0
	 */
	enum xmit_hash_policy xmit_hash_policy;
};

struct del_bond_dev_s {
	struct mt_msg_head head;
	char bond_name[IFNAMSIZ];
};

struct get_bond_chip_name_s {
	char bond_name[IFNAMSIZ];
	char chip_name[IFNAMSIZ];
};

struct bond_drv_msg_s {
	u32 bond_id;
	u32 slave_cnt;
	u32 master_slave_index;
	char bond_name[IFNAMSIZ];
	char slave_name[MAX_NETDEV_NUM][IFNAMSIZ];
};

#define MAX_BONDING_CNT_PER_CARD (2)

struct bond_negotiate_status {
	u8 status;
	u8 version;
	u8 rsvd0[6];
	u32 bond_id;
	u32 bond_mmi_status; /* 该bond子设备的链路状态 */
	u32 active_bitmap;   /* 该bond子设备的slave port状态 */

	u8 rsvd[16];
};

struct bond_all_msg_s {
	struct bond_drv_msg_s drv_msg;
	struct bond_negotiate_status active_info;
};

struct get_card_bond_msg_s {
	u32 bond_cnt;
	struct bond_all_msg_s all_msg[MAX_BONDING_CNT_PER_CARD];
};

#define MAX_FUSION_Q_STATS_STR_LEN	16
#define MAX_FUSION_Q_NUM		256
struct queue_status_s {
	pid_t tgid;
	char status[MAX_FUSION_Q_STATS_STR_LEN];
};

struct fusion_q_status_s {
	u16 queue_num;
	struct queue_status_s queue[MAX_FUSION_Q_NUM];
};

struct fusion_q_tx_hw_page {
	u64 phy_addr;
	u64 *map_addr;
};

struct fusion_sq_info {
	u16 q_id;
	u16 pi;
	u16 ci; /* sw_ci */
	u16 fi; /* hw_ci */
	u32 q_depth;
	u16 pi_reverse;
	u16 wqebb_size;
	u8 priority;
	u16 *ci_addr;
	u64 cla_addr;
	void *slq_handle;
	struct fusion_q_tx_hw_page direct_wqe;
	struct fusion_q_tx_hw_page doorbell;
	u32 page_idx;
	u32 glb_sq_id;
};

struct fusion_q_tx_wqe {
	u32 data[4];
};

struct fusion_rq_info {
	u16 q_id;
	u16 delta;
	u16 hw_pi;
	u16 ci; /* sw_ci */
	u16 sw_pi;
	u16 wqebb_size;
	u16 q_depth;
	u16 buf_len;

	void *slq_handle;
	u64 ci_wqe_page_addr;
	u64 ci_cla_tbl_addr;

	u8 coalesc_timer_cfg;
	u8 pending_limt;
	u16 msix_idx;
	u32 msix_vector;
};

struct fusion_q_rx_wqe {
	u32 data[8];
};

struct fusion_q_rx_cqe {
	union {
		struct {
			unsigned int checksum_err : 16;
			unsigned int lro_num : 8;
			unsigned int rsvd1 : 7;
			unsigned int rx_done : 1;
		} bs;
		unsigned int value;
	} dw0;

	union {
		struct {
			unsigned int vlan : 16;
			unsigned int length : 16;
		} bs;
		unsigned int value;
	} dw1;

	union {
		struct {
			unsigned int pkt_types : 12;
			unsigned int rsvd : 4;
			unsigned int udp_0 : 1;
			unsigned int ipv6_ex_add : 1;
			unsigned int loopback : 1;
			unsigned int umbcast : 2;
			unsigned int vlan_offload_en : 1;
			unsigned int tag_num : 2;
			unsigned int rss_type : 8;
		} bs;
		unsigned int value;
	} dw2;

	union {
		struct {
			unsigned int rss_hash_value;
		} bs;
		unsigned int value;
	} dw3;

	union {
		struct {
			unsigned int tx_ts_seq : 16;
			unsigned int message_1588_offset : 8;
			unsigned int message_1588_type : 4;
			unsigned int rsvd : 1;
			unsigned int if_rx_ts : 1;
			unsigned int if_tx_ts : 1;
			unsigned int if_1588 : 1;
		} bs;
		unsigned int value;
	} dw4;

	union {
		struct {
			unsigned int ts;
		} bs;
		unsigned int value;
	} dw5;

	union {
		struct {
			unsigned int lro_ts;
		} bs;
		unsigned int value;
	} dw6;

	union {
		struct {
			unsigned int rsvd0;
		} bs;
		unsigned int value;
	} dw7; /* 16Bytes Align */
};

struct os_hot_repalce_func_info {
	char card_name[HINIC3_CARD_NAME_MAX_LEN];
	u32 bus_num;
	u32 valid;
	u32 bdf;
	int partition;
	u16 backup_pf;
	u16 pf_idx;
	int port_id;
};

#define ALL_CARD_PF_NUM 2048 /* 64 card * 32 pf */
struct os_hot_replace_info {
	struct os_hot_repalce_func_info func_infos[ALL_CARD_PF_NUM];
	u32 func_cnt;
};

int alloc_buff_in(void *hwdev, struct msg_module *nt_msg, u32 in_size, void **buf_in);

int alloc_buff_out(void *hwdev, struct msg_module *nt_msg, u32 out_size, void **buf_out);

void free_buff_in(void *hwdev, const struct msg_module *nt_msg, void *buf_in);

void free_buff_out(void *hwdev, struct msg_module *nt_msg, void *buf_out);

int copy_buf_out_to_user(struct msg_module *nt_msg, u32 out_size, void *buf_out);

int send_to_mpu(void *hwdev, struct msg_module *nt_msg, void *buf_in, u32 in_size,
		void *buf_out, u32 *out_size);
int send_to_npu(void *hwdev, struct msg_module *nt_msg, void *buf_in,
		u32 in_size, void *buf_out, u32 *out_size);
int send_to_sm(void *hwdev, struct msg_module *nt_msg, void *buf_in, u32 in_size,
	       void *buf_out, u32 *out_size);

#endif /* _HINIC3_MT_H_ */
