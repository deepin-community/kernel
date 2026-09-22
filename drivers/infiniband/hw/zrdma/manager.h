/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef _MGR_H
#define _MGR_H

/* pcie function_id */
#define VF_ID_START_BIT                     0
#define PF_ID_START_BIT                     13
#define BAR_NUM_START_BIT                   20
#define EP_ID_START_BIT                     23
#define FUNCTION_TYPE_START_BIT             27
#define SCENE_CODE_START_BIT                28

#define VF_ID_OFFSET                       (PF_ID_START_BIT - VF_ID_START_BIT)              // 13
#define PF_ID_OFFSET                       (BAR_NUM_START_BIT - PF_ID_START_BIT)            // 7
#define BAR_NUM_OFFSET                     (EP_ID_START_BIT - BAR_NUM_START_BIT)            // 3
#define EP_ID_OFFSET                       (FUNCTION_TYPE_START_BIT - EP_ID_START_BIT)      // 4
#define FUNCTION_TYPE_OFFSET               (SCENE_CODE_START_BIT - FUNCTION_TYPE_START_BIT) // 1
#define SCENE_CODE_OFFSET                  (32 - SCENE_CODE_START_BIT)                      // 4

/* filed: VF_ID/PF_ID/BAR_NUM/EP_ID/FUNCTION_TYPE/SCENE_CODE  */
#define DH_FUNC_ID_EXTRACT(data, filed)                                                 \
        (((data & (~((1UL << filed##_START_BIT) - 1))) &                                \
        ((1UL << (filed##_START_BIT + filed##_OFFSET)) - 1)) >> filed##_START_BIT)

#define DH_FUNC_ID_GEN(type, ep, bar, pf, vf)                                           \
        ( ((type & ((1 << FUNCTION_TYPE_OFFSET) - 1)) << FUNCTION_TYPE_START_BIT)       \
          | ((ep & ((1 << EP_ID_OFFSET) - 1)) << EP_ID_START_BIT)                       \
          | ((bar & ((1 << BAR_NUM_OFFSET) - 1)) << BAR_NUM_START_BIT)                  \
          | ((pf & ((1 << PF_ID_OFFSET) - 1)) << PF_ID_START_BIT)                       \
          | ((vf & ((1 << VF_ID_OFFSET) - 1))) )

/* bar msg status */
#define BAR_MSG_STATUS_OK   (200)
#define BAR_MSG_STATUS_REQ_ERR (400)
#define BAR_MSG_STATUS_RESP_ERR (500)

/* Common configuration */
#define ZXDH_PCI_CAP_COMMON_CFG 1
/* Notifications */
#define ZXDH_PCI_CAP_NOTIFY_CFG 2
/* ISR access */
#define ZXDH_PCI_CAP_ISR_CFG 3
/* Device specific configuration */
#define ZXDH_PCI_CAP_DEVICE_CFG 4
/* PCI configuration access */
#define ZXDH_PCI_CAP_PCI_CFG 5

#define ZXDH_ZF_EPID 4
#define ZXDH_BAR_CHAN_OFFSET 0x2000
#define ZXDH_CHAN_REPS_LEN 4
#define MSG_REP_VALID 0xff
#define MSG_REP_LEN_OFFSET 1

// #define MSG_CHAN_END_PF 1
// #define MSG_CHAN_END_VF 2
// #define MSG_CHAN_END_RISC 3

#define MODULE_RDMA 4

#define RDMA_MGR_INIT (0)
#define RDMA_REG_READ (1)
#define RDMA_REG_WRITE (2)
#define RDMA_MP_DTCM_PARA_GET (3)
#define RDMA_MP_DTCM_PARA_SET (4)
#define RDMA_HWBOND_SPEED_SET (5)
#define RDMA_REQ_VER	    (6)
#define RDMA_RESP_VER	    (7)
#define GET_SRQ_L2D_ADDR    (8)
#define RDMA_VFS_NUM_SET	(9)
#define RDMA_COMMON_OPCODE (10)
#define RDMA_COMMON_RESP_OPCODE (11)
#define RDMA_SWITCH_QUERY	(12)
#define RDMA_SWITCH_CONFIG	(13)
#define RDMA_ETS_CFG            (14)
#define RDMA_MAX_RATE_CONFIG    (15)
#define RDMA_ETS_SWITCH         (16)
#define ZXDH_REQ_PCI_BDF	(17)
#define RDMA_PORT_SPEED_SET     (18)
#define RDMA_GET_EXT_VHCA_INFO  (19)
#define RDMA_UNBOND_SPEED_SET   (20)

#define ZXDH_REQ_MSG_LEN	15
#define ZXDH_RESP_MSG_LEN	80
#define ZXDH_MSG_MIN_LEN	5
#define ZXDH_VER_HEADER_H	0xAA
#define ZXDH_VER_HEADER_L	0x55

#define ZXDH_COMMON_BUF_LEN	        65
#define ZXDH_COMMON_VALID_LEN		60
#define ZXDH_COMMON_TOTAL_MSG_LEN   66
#define ZXDH_COMMON_MSG_HEADER_H	0xAA
#define ZXDH_COMMON_MSG_HEADER_L	0x55

#define ZXDH_CMD_NP_MAC_ADD 1
#define ZXDH_CMD_NP_MAC_DEL 2
#define ZXDH_CMD_SEND_VF_FLR 3
#define ZXDH_CMD_SET_TRUST_TYPE 4
#define ZXDH_CMD_GET_TRUST_TYPE 5

#define ZXDH_DCBNL_MAX_TRAFFIC_CLASS            (8)
#define ZXDH_DCBNL_MAX_PRIORITY                 (8)
#define ZXDH_RDMA_TURN_TO_RR_DISABLE_RL         0x0
#define ZXDH_RDMA_TURN_TO_RR_ENABLE_RL          0x1
#define ZXDH_RDMA_TURN_TO_SP                    0x2
#define ZXDH_PHY_PORT_INIT_VAL                  0xff

#define ZXDH_ETS_MAX_TC_NUM                     8

enum zxdh_ets_config_type {
	ZXDH_ETS_CONFIG_NONE = 0,
	ZXDH_ETS_CONFIG_WEIGHT,
	ZXDH_ETS_CONFIG_MAXRATE,
	ZXDH_ETS_CONFIG_SWITCH_OFF,
};

struct zxdh_ets_token_mgr {
	bool enabled;
	enum zxdh_ets_config_type config_type;
	bool init_done;

	u8 tc_weight[ZXDH_ETS_MAX_TC_NUM];
	u8 tc_tsa[ZXDH_ETS_MAX_TC_NUM];
	u32 ets_tc_bitmap;

	atomic_t tc_qp_cnt[ZXDH_ETS_MAX_TC_NUM];
	u32 active_tc_bitmap;

	struct mutex lock;
};

#if (!defined(TRUE) || (TRUE != 1))
#undef TRUE
#define TRUE 1
#endif

#define	RDMA_DEL_REMOTE_IP  0
#define	RDMA_ADD_REMOTE_IP 1
#define ZXDH_DEV_ID_X512_ROCE_PF_EP0 0x808b
#define ZXDH_DEV_ID_X512_ROCE_PF_EP1 0x808d

enum BAR_DRIVER_TYPE {
	MSG_CHAN_END_MPF = 0,
	MSG_CHAN_END_PF,
	MSG_CHAN_END_VF,
	MSG_CHAN_END_RISC,
	MSG_CHAN_END_ERR,
};

struct zxdh_pci_bar_msg {
	uint64_t virt_addr; /**< 4k空间地址, 若src为MPF该参数不生效>**/
	void *payload_addr; /**< 消息净荷地址>**/
	uint16_t payload_len; /**< 消息净荷长度>**/
	uint16_t emec; /**< 消息紧急类型>**/
	uint16_t src; /**< 消息发送源，参考BAR_DRIVER_TYPE>**/
	uint16_t dst; /**< 消息接收者，参考BAR_DRIVER_TYPE>**/
	uint32_t event_id; /**< 事件id>**/
	uint16_t src_pcieid; /**< 源  pcie_id>**/
	uint16_t dst_pcieid; /**< 目的pcie_id>**/
};

struct zxdh_msg_recviver_mem {
	void *recv_buffer; /**< 消息接收缓存>**/
	uint16_t buffer_len; /**< 消息缓存长度>**/
};

/* This is the PCI capability header: */
struct zxdh_pf_pci_cap {
	__u8 cap_vndr; /* Generic PCI field: PCI_CAP_ID_VNDR */
	__u8 cap_next; /* Generic PCI field: next ptr. */
	__u8 cap_len; /* Generic PCI field: capability length */
	__u8 cfg_type; /* Identifies the structure. */
	__u8 bar; /* Where to find it. */
	__u8 id; /* Multiple capabilities of the same type */
	__u8 padding[2]; /* Pad to full dword. */
	__le32 offset; /* Offset within bar. */
	__le32 length; /* Length of the structure, in bytes. */
};

struct dh_rdma_board_glb_cfg {
	u32 cqp_size; //cqp队列深度
	u32 qp_size; //qp队列深度
	u32 cq_size; //cq队列深度
	u32 ceq_size; //ceq队列深度
	u32 srq_size; //srq队列深度
	u32 wr_cnt; //workrequest个数
	u32 sge_cnt; //sge个数
};

struct dh_rdma_vf_param {
	u32 vf_id;
	u32 vf_vhca_id;
	u32 pf_id;
	u32 pf_vhca_id;

	u32 vf_bar_offset; //bar空间地址偏移

	u32 qp_cnt; // 每个vhca的QP数
	u32 cq_cnt; // 每个vhca的CQ数
	u32 srq_cnt; // 每个vhca的SRQ数
	u32 ceq_cnt; // 每个vhca的CEQ数
	u32 ah_cnt; // 每个vhca的AH数

	u32 qp_id_min; // QP最小队列编号
	u32 cq_id_min; // CQ最小队列编号
	u32 ceq_id_min; // CEQ最小队列编号
	u32 srq_id_min; // SRQ最小队列编号
};

//pf需要知道自己实际能用的最大队列，已经需要分配的最大队列数（包括vf需要用的队列）
struct dh_rdma_pf_param {
	u8 pf_id; // 当前EP下该PF的ID
	u32 max_vf_num; // 当前PF下VF的最大数量
	u8 sid; // PF的SID（0~31）
	//u8  has_vf;         // 判断该PF有没有VF，没有为0，有为1
	u32 vhca_id; // vhca ID

	u32 pf_bar_offset; //bar空间地址偏移

	u32 qp_cnt; // 每个vhca的QP数
	u32 cq_cnt; // 每个vhca的CQ数
	u32 srq_cnt; // 每个vhca的SRQ数
	u32 ceq_cnt; // 每个vhca的CEQ数
	//u32 aeq_cnt;       // 每个vhca的AEQ数
	u32 ah_cnt; // 每个vhca的AH数

	u32 qp_id_min; // QP最小队列编号
	u32 cq_id_min; // CQ最小队列编号
	u32 ceq_id_min; // CEQ最小队列编号
	u32 srq_id_min; // SRQ最小队列编号

	u32 assign_qp_cnt; //pf分配使用的QP数
	u32 assign_cq_cnt; //pf分配使用的CQ数
	u32 assign_ceq_cnt; //pf分配使用的CEQ数
	u32 assign_srq_cnt; //pf分配使用的SRQ数

	u32 qp_size; // QP队列深度
	u32 cq_size; // CQ队列深度
	u32 ceq_size; // CEQ队列深度
	u32 aeq_size; // AEQ队列深度
	u32 srq_size; // SRQ队列深度
};

struct zxdh_mgr_par {
	u16 ftype;
	u16 ep_id;
	u16 pf_id;
	u16 vf_id;
	u32 bar_offset;
	u32 l2d_smmu_l2_offset;
	u64 l2d_smmu_addr;
	u64 nof_ioq_ddr_addr;

	u16 vhca_id;
	u16 vhca_id_pf;
	u32 max_vf_num;

	u32 qp_cnt;
	u32 cq_cnt;
	u32 srq_cnt;
	u32 ceq_cnt;
	u32 ah_cnt;
	u32 mr_cnt;
	u32 pbleq_cnt;
	u32 pblem_cnt;

	u32 vf_qp_cnt;
	u32 vf_cq_cnt;
	u32 vf_srq_cnt;
	u32 vf_ceq_cnt;
	u32 vf_ah_cnt;
	u32 vf_mr_cnt;
	u32 vf_pbleq_cnt;
	u32 vf_pblem_cnt;

	u32 base_qpn;
	u32 base_cqn;
	u32 base_srqn;
	u32 base_ceqn;

	u64 pf_hmc_size;
	u64 qp_hmc_base;
	u64 cq_hmc_base;
	u64 srq_hmc_base;
	u64 txwindow_hmc_base;
	u64 ird_hmc_base;
	u64 ah_hmc_base;
	u64 mr_hmc_base;
	u64 pbleq_hmc_base;
	u64 pblem_hmc_base;

	u8 hmc_sid;
	u8 hmc_use_dpu_ddr;
	u8 np_mode_low_lat;
	u8 mcode_type;
	u8 chip_version;

	u32 max_hw_read_sges;
	u32 max_hw_wq_frags;
	u32 dh_total_vhca;
    u16 vhca_gqp_start;
    u16 vhca_gqp_cnt;
    u16 vhca_8k_index_start;
    u16 vhca_8k_index_cnt;
    u16 vhca_ud_gqp;
    u16 vhca_ud_8k_index;
} __attribute__((__packed__));

struct zxdh_chan_msg {
	u32 msg_len;
	void *msg;
};

enum chan_cmd_type {
	GET_PF_PARAM = 1,
	GET_VF_PARAM = 2,
};

struct zxdh_mgr_msg {
	u32 op_code;
	u8 ep_id;
	u8 pf_id;
	u16 vport_vf_id;
	u8 ftype; // 判断为vf，0是pf， 1是vf
	u8 rsv[3];
};

struct zxdh_mgr {
	//struct irdma_device *iwdev;
	struct pci_dev *pdev;
	u32 pf_id;
	u32 vport_vf_id;
	u32 ep_id;
	u8 ftype; // 判断为vf，0是pf， 1是vf
	u16 pcie_id;
	u16 device_id;
	u8 __iomem *pci_hw_addr;
	struct zxdh_mgr_par param;
};

enum e_dtcm_para_id_dcqcn {
	E_PARA_DCQCN_RPG_TIME_RESET,
	E_PARA_DCQCN_CLAMP_TGT_RAGE,
	E_PARA_DCQCN_CLAMP_TGT_RATE_AFTER_TIME_INC,
	E_PARA_DCQCN_DCE_TCP_RTT,
	E_PARA_DCQCN_DCE_TCP_G,
	E_PARA_DCQCN_RPG_GD,
	E_PARA_DCQCN_INITIAL_ALPHA_VALUE,
	E_PARA_DCQCN_MIN_DEC_FAC,
	E_PARA_DCQCN_RPG_THRESHOLD,
	E_PARA_DCQCN_RPG_RATIO_INCREASE,
	E_PARA_DCQCN_RPG_AI_RATIO,
	E_PARA_DCQCN_RPG_HAI_RATIO,
	E_PARA_DCQCN_NUM
};

enum e_dtcm_para_id_rtt {
	E_PARA_RTT_ALPHA,
	E_PARA_RTT_TLOW,
	E_PARA_RTT_THIGH,
	E_PARA_RTT_MINRTT,
	E_PARA_RTT_BETA,
	E_PARA_RTT_AI_NUM,
	E_PARA_RTT_THRED_GRADIENT,
	E_PARA_RTT_HAI_N,
	E_PARA_RTT_AI_N,
	E_PARA_RTT_NUM
};

enum e_dtcm_para_id_pid {
	E_PARA_PID_TLOW,
	E_PARA_PID_1_TLOW,
	E_PARA_PID_THIGH,
	E_PARA_PID_RPG_AI_RATE_VHCA,
    E_PARA_PID_RPG_HAI_RATE_VHCA,
	E_PARA_PID_NUM
};

struct rdma_chan_msg_para {
	uint8_t *in_buf;
	uint8_t *out_buf;
	size_t in_size;
	size_t out_size;
};

struct dh_rdma_reg_read_req {
	u64 phy_addr;
	u32 reg_num;
} __attribute__((__packed__));

struct dh_rdma_reg_read_resp {
	u64 phy_addr;
	u32 reg_num;
	u32 status_code;
	u32 data[];
} __attribute__((__packed__));

struct dh_rdma_reg_write_req {
	u64 phy_addr;
	u32 reg_num;
	u32 data[];
} __attribute__((__packed__));

struct dh_rdma_reg_write_resp {
	u64 phy_addr;
	u32 reg_num;
	u32 status_code;
} __attribute__((__packed__));

// mp dtcm para set/get messages
struct dh_mp_dtcm_para_set_req {
	u16 mcode_type;
	u16 para_id;
	u32 val;
} __attribute__((__packed__));

struct dh_mp_dtcm_para_set_resp {
	u16 para_id;
	u32 status_code;
} __attribute__((__packed__));

struct dh_mp_dtcm_para_get_req {
	u16 mcode_type;
	u16 para_id;
} __attribute__((__packed__));

struct dh_mp_dtcm_para_get_resp {
	u16 para_id;
	u32 status_code;
	u32 val;
} __attribute__((__packed__));

struct dh_rdma_vf_num_set_resp {
    u64 vf_pblem_cnt;
    u32 status_code;
} __attribute__((__packed__));

enum switch_name_e {
	WQE_RATE_LIMIT,
	ORD_MAX_SIZE,
	RQ_CREDIT,
	FLOWLABEL_ENABLE,
	SWITCH_NAME_MAX
} ;

enum BAR_MSG_STATUS
{
    STATUS_OK = 200,

    STATUS_REQ_ERR = 400,
    STATUS_REQ_LEN_ERR = 401,
    STATUS_REQ_NO_PERMIT = 402,
    STATUS_REQ_ERR_PARAM = 403,

    STATUS_RESP_ERR = 500,
    STATUS_RESP_RES_GET_ERR = 501,
    STATUS_RESP_NO_MEM = 502,
};

struct dh_rdma_query_switch_req {
	u32 opcode;
    enum switch_name_e type;
} __attribute__((__packed__));
struct dh_rdma_query_switch_resp {
    u32 value;
    u32 status_code;
} __attribute__((__packed__));

struct dh_rdma_config_switch_req {
	u32 opcode;
    enum switch_name_e type;
    u32 value;
} __attribute__((__packed__));

struct dh_rdma_config_switch_resp {
    u32 status_code;
} __attribute__((__packed__));

struct dh_hwbond_speed_set_req {
	u32 speed;
	u8 epid_pfid;
} __attribute__((__packed__));

struct dh_port_speed_set_req {
	u32 speed;
	u8 epid_pfid;
} __attribute__((__packed__));

struct dh_rdma_vf_num_set_req {
    u16 ep_id;
    u16 pf_id;
	u16 num_vfs;
} __attribute__((__packed__));

// channel message struct
struct zxdh_reg_read_cmd {
	u32 op_code;
	struct dh_rdma_reg_read_req req;
} __attribute__((__packed__));
struct zxdh_reg_write_cmd {
	u32 op_code;
	struct dh_rdma_reg_write_req req;
} __attribute__((__packed__));

struct zxdh_mp_dtcm_para_get_cmd {
	u32 op_code;
	struct dh_mp_dtcm_para_get_req req;
} __attribute__((__packed__));
struct zxdh_mp_dtcm_para_set_cmd {
	u32 op_code;
	struct dh_mp_dtcm_para_set_req req;
} __attribute__((__packed__));

struct zxdh_hwbond_speed_set_cmd {
	u32 op_code;
	struct dh_hwbond_speed_set_req req;
} __attribute__((__packed__));

struct zxdh_port_speed_set_cmd {
	u32 op_code;
	struct dh_port_speed_set_req req;
} __attribute__((__packed__));

struct zxdh_rdma_vf_num_set_cmd {
	u32 op_code;
	struct dh_rdma_vf_num_set_req req;
} __attribute__((__packed__));

struct zxdh_req_msg {
	u8 op_code;
	u8 buf[ZXDH_REQ_MSG_LEN];
} __attribute__((__packed__));

struct zxdh_resp_msg {
	u8 op_code;
	u8 buf[ZXDH_RESP_MSG_LEN];
} __attribute__((__packed__));

struct zxdh_rdma_bar_req_msg {
	u8 op_code;
	u8 buf[ZXDH_COMMON_BUF_LEN];
} __attribute__((__packed__));

struct zxdh_rdma_bar_resp_msg {
	u8 op_code;
	u8 buf[ZXDH_COMMON_BUF_LEN];
} __attribute__((__packed__));

struct zxdh_dcbnl_ieee_rdma_ets {
    uint32_t    phy_port;
    uint32_t    mode;/* 1 -- on 0 --off */
    uint8_t    tc_tx_bw[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    uint8_t    tc_tsa[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    uint8_t    prio_tc[ZXDH_DCBNL_MAX_PRIORITY];
};

struct  zxdh_dcbnl_ieee_rdma_maxrate {
    uint32_t    phy_port;
    uint32_t    mode;/* 1 -- on 0 --off */
    uint64_t   tc_maxrate[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
};

struct  zxdh_rdma_ets_status {
    uint32_t    phy_port;
    uint32_t    mode;/* 1 -- on 0 --off */
};

struct zxdh_rdma_ets_set_cmd {
    uint32_t   fw_opcode;
    uint8_t    ep_id;
    uint8_t    pf_id;
    uint8_t    ets_cmd_mode;// mode_switch -- 0: RR disable RL, 1: RR enable RL, 2: SP disable RL
	union
    {
        u64   tc_maxrate[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
        u64   tc_ets[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    } tc_rate_limit;
} __attribute__((__packed__));

struct dh_rdma_ets_set_resp {
    u32 status_code;
} __attribute__((__packed__));

struct zxdh_rdma_to_eth_ip_para {
	char *ifname;
	u32 src_ip[4];
	u32 dst_ip[4];
	u64 src_mac;
	u64 dst_mac;
	u32 linked_fid;
	u8 ipv4 : 1;
	u8 mode : 1;
};

struct dh_get_srq_mem_info_req {
	u32 op_code;
    u32 function_id;
} __attribute__((__packed__));

struct dh_get_srq_mem_info_resp {
	u64 srq_mem_paddr;
	u32 srq_mem_size;
    u32 rdma_ext_bar_offset;
    u32 status_code;
} __attribute__((__packed__));

struct zxdh_rdma_sriov_event_info
{
    struct pci_dev *pdev;
    uint64_t bar0_virt_addr;
    uint16_t vport_id;
	uint16_t num_vfs;
};

struct zxdh_rdma_common_req_msg {
	u8 opcode;
	u8 len;
	u8 buf[ZXDH_COMMON_VALID_LEN];
};

struct zxdh_rdma_common_resp_msg {
	u8 opcode;
	u8 len;
	u8 buf[ZXDH_COMMON_VALID_LEN];
};

struct dh_get_ext_vhca_info_req {
	u32 op_code;
    u32 function_id;
} __attribute__((__packed__));

struct dh_get_ext_vhca_info_resp {
    u16 ext_vhca_id;
    u32 ext_vhca_bar_offset;
    u16 ext_vhca_gqp_start;
    u16 ext_vhca_gqp_cnt;
    u16 ext_vhca_8k_index_start;
    u16 ext_vhca_8k_index_cnt;
    u64 ext_l2d_addr_base;
    u32 max_hw_sq_sges;
    u32 max_hw_rq_sges;
    u32 status_code;
} __attribute__((__packed__));

typedef void (*notify_remote_ip_update)(struct zxdh_rdma_to_eth_ip_para *info);

int rdma_chan_msg_send(struct zxdh_pci_f *rf, struct rdma_chan_msg_para *para);

int zxdh_bar_chan_sync_msg_send(struct zxdh_pci_bar_msg *in,
				struct zxdh_msg_recviver_mem *result);
int zxdh_chan_sync_send(struct zxdh_mgr *pmgr, struct zxdh_chan_msg *pmsg,
			u32 *pdata, u32 rep_len);
int zxdh_mgr_par_get(struct zxdh_mgr *dh_mgr);

int zxdh_get_ext_vhca_info(struct zxdh_pci_f *rf);

int zxdh_rdma_reg_read(struct zxdh_pci_f *rf, uint64_t phy_addr,
		       uint32_t *outdata);

int zxdh_rdma_regs_read(struct zxdh_pci_f *rf, uint64_t phy_addr,
		       uint32_t *outdata, uint32_t num);

int zxdh_rdma_reg_write(struct zxdh_pci_f *rf, uint64_t phy_addr, uint32_t val);

int zxdh_mp_dtcm_para_get(struct zxdh_pci_f *rf, uint16_t mcode_type,
			  uint16_t para_id, uint32_t *outdata);

int zxdh_mp_dtcm_para_set(struct zxdh_pci_f *rf, uint16_t mcode_type,
			  uint16_t para_id, uint32_t val);

int dh_rdma_pf_pcie_id_get(struct zxdh_mgr *mgr);

// callback installed to net
int32_t switch_bound_master_netdev(struct net_device *primary_netdev, struct net_device *linux_bond_netdev, bool hb_enable);
int32_t set_rdma_firmware_speed(struct net_device *netdev, uint32_t bps);
int32_t set_rdma_port_speed(struct net_device *netdev, uint32_t bps);
// int32_t set_rdma_hwbond_status(struct net_device *netdev, bool hb_enable);
int zxdh_req_cmd_ver(struct zxdh_pci_f *rf);
int zxdh_rdma_send_common_msg(struct zxdh_pci_f *rf, struct zxdh_rdma_common_req_msg *req, struct zxdh_rdma_common_resp_msg *resp);
int register_remote_ip_event_handler(notify_remote_ip_update handler);
void unregister_remote_ip_event_handler(void);
void rdma_update_remote_ip(struct zxdh_rdma_to_eth_ip_para *info);
int set_rdma_vf_num(struct zxdh_rdma_sriov_event_info *sriov_info, u64 *vf_pblem_cnt);
int zxdh_vm_env_check(struct zxdh_pci_f *rf);
#endif
