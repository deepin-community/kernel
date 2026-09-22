/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */
#ifndef ZXDH_CM_H
#define ZXDH_CM_H

#define ZXDH_MPA_REQUEST_ACCEPT 1
#define ZXDH_MPA_REQUEST_REJECT 2

/* IETF MPA -- defines */
#define IEFT_MPA_KEY_REQ "MPA ID Req Frame"
#define IEFT_MPA_KEY_REP "MPA ID Rep Frame"
#define IETF_MPA_KEY_SIZE 16
#define IETF_MPA_VER 1
#define IETF_MAX_PRIV_DATA_LEN 512
#define IETF_MPA_FRAME_SIZE 20
#define IETF_RTR_MSG_SIZE 4
#define IETF_MPA_V2_FLAG 0x10
#define SNDMARKER_SEQNMASK 0x000001ff
#define ZXDH_MAX_IETF_SIZE 32

/* IETF RTR MSG Fields */
#define IETF_PEER_TO_PEER 0x8000
#define IETF_FLPDU_ZERO_LEN 0x4000
#define IETF_RDMA0_WRITE 0x8000
#define IETF_RDMA0_READ 0x4000
#define IETF_NO_IRD_ORD 0x3fff

#define MAX_PORTS 65536

#define ZXDH_PASSIVE_STATE_INDICATED 0
#define ZXDH_DO_NOT_SEND_RESET_EVENT 1
#define ZXDH_SEND_RESET_EVENT 2

#define MAX_ZXDH_IFS 4

#define SET_ACK 1
#define SET_SYN 2
#define SET_FIN 4
#define SET_RST 8

#define TCP_OPTIONS_PADDING 3

#define ZXDH_DEFAULT_RETRYS 64
#define ZXDH_DEFAULT_RETRANS 8
#define ZXDH_DEFAULT_TTL 0x40
#define ZXDH_DEFAULT_RTT_VAR 6
#define ZXDH_DEFAULT_SS_THRESH 0x3fffffff
#define ZXDH_DEFAULT_REXMIT_THRESH 8

#define ZXDH_RETRY_TIMEOUT HZ
#define ZXDH_SHORT_TIME 10
#define ZXDH_LONG_TIME (2 * HZ)
#define ZXDH_MAX_TIMEOUT ((unsigned long)(12 * HZ))

#define ZXDH_CM_HASHTABLE_SIZE 1024
#define ZXDH_CM_TCP_TIMER_INTERVAL 3000
#define ZXDH_CM_DEFAULT_MTU 1540
#define ZXDH_CM_DEFAULT_FRAME_CNT 10
#define ZXDH_CM_THREAD_STACK_SIZE 256
#define ZXDH_CM_DEFAULT_RCV_WND 64240
#define ZXDH_CM_DEFAULT_RCV_WND_SCALED 0x3FFFC
#define ZXDH_CM_DEFAULT_RCV_WND_SCALE 2
#define ZXDH_CM_DEFAULT_FREE_PKTS 10
#define ZXDH_CM_FREE_PKT_LO_WATERMARK 2
#define ZXDH_CM_DEFAULT_MSS 536
#define ZXDH_CM_DEFAULT_MPA_VER 2
#define ZXDH_CM_DEFAULT_SEQ 0x159bf75f
#define ZXDH_CM_DEFAULT_LOCAL_ID 0x3b47
#define ZXDH_CM_DEFAULT_SEQ2 0x18ed5740
#define ZXDH_CM_DEFAULT_LOCAL_ID2 0xb807
#define ZXDH_MAX_CM_BUF (ZXDH_MAX_IETF_SIZE + IETF_MAX_PRIV_DATA_LEN)

/* cm node transition states */
enum zxdh_cm_node_state {
	ZXDH_CM_STATE_UNKNOWN,
	ZXDH_CM_STATE_INITED,
	ZXDH_CM_STATE_LISTENING,
	ZXDH_CM_STATE_SYN_RCVD,
	ZXDH_CM_STATE_SYN_SENT,
	ZXDH_CM_STATE_ONE_SIDE_ESTABLISHED,
	ZXDH_CM_STATE_ESTABLISHED,
	ZXDH_CM_STATE_ACCEPTING,
	ZXDH_CM_STATE_MPAREQ_SENT,
	ZXDH_CM_STATE_MPAREQ_RCVD,
	ZXDH_CM_STATE_MPAREJ_RCVD,
	ZXDH_CM_STATE_OFFLOADED,
	ZXDH_CM_STATE_FIN_WAIT1,
	ZXDH_CM_STATE_FIN_WAIT2,
	ZXDH_CM_STATE_CLOSE_WAIT,
	ZXDH_CM_STATE_TIME_WAIT,
	ZXDH_CM_STATE_LAST_ACK,
	ZXDH_CM_STATE_CLOSING,
	ZXDH_CM_STATE_LISTENER_DESTROYED,
	ZXDH_CM_STATE_CLOSED,
};

enum mpa_frame_ver {
	IETF_MPA_V1 = 1,
	IETF_MPA_V2 = 2,
};

enum mpa_frame_key {
	MPA_KEY_REQUEST,
	MPA_KEY_REPLY,
};

enum send_rdma0 {
	SEND_RDMA_READ_ZERO = 1,
	SEND_RDMA_WRITE_ZERO = 2,
};

enum zxdh_tcpip_pkt_type {
	ZXDH_PKT_TYPE_UNKNOWN,
	ZXDH_PKT_TYPE_SYN,
	ZXDH_PKT_TYPE_SYNACK,
	ZXDH_PKT_TYPE_ACK,
	ZXDH_PKT_TYPE_FIN,
	ZXDH_PKT_TYPE_RST,
};

enum zxdh_cm_listener_state {
	ZXDH_CM_LISTENER_PASSIVE_STATE = 1,
	ZXDH_CM_LISTENER_ACTIVE_STATE = 2,
	ZXDH_CM_LISTENER_EITHER_STATE = 3,
};

/* CM event codes */
enum zxdh_cm_event_type {
	ZXDH_CM_EVENT_UNKNOWN,
	ZXDH_CM_EVENT_ESTABLISHED,
	ZXDH_CM_EVENT_MPA_REQ,
	ZXDH_CM_EVENT_MPA_CONNECT,
	ZXDH_CM_EVENT_MPA_ACCEPT,
	ZXDH_CM_EVENT_MPA_REJECT,
	ZXDH_CM_EVENT_MPA_ESTABLISHED,
	ZXDH_CM_EVENT_CONNECTED,
	ZXDH_CM_EVENT_RESET,
	ZXDH_CM_EVENT_ABORTED,
};

struct ietf_mpa_v1 {
	u8 key[IETF_MPA_KEY_SIZE];
	u8 flags;
	u8 rev;
	__be16 priv_data_len;
	u8 priv_data[];
};

struct ietf_rtr_msg {
	__be16 ctrl_ird;
	__be16 ctrl_ord;
};

struct ietf_mpa_v2 {
	u8 key[IETF_MPA_KEY_SIZE];
	u8 flags;
	u8 rev;
	__be16 priv_data_len;
	struct ietf_rtr_msg rtr_msg;
	u8 priv_data[];
};

struct option_base {
	u8 optionnum;
	u8 len;
};

struct option_mss {
	u8 optionnum;
	u8 len;
	__be16 mss;
};

struct option_windowscale {
	u8 optionnum;
	u8 len;
	u8 shiftcount;
};

union all_known_options {
	char eol;
	struct option_base base;
	struct option_mss mss;
	struct option_windowscale windowscale;
};

struct zxdh_timer_entry {
	struct list_head list;
	unsigned long timetosend; /* jiffies */
	struct zxdh_puda_buf *sqbuf;
	u32 type;
	u32 retrycount;
	u32 retranscount;
	u32 context;
	u32 send_retrans;
	int close_when_complete;
};

/* CM context params */
struct zxdh_cm_tcp_context {
	u8 client;
	u32 loc_seq_num;
	u32 loc_ack_num;
	u32 rem_ack_num;
	u32 rcv_nxt;
	u32 loc_id;
	u32 rem_id;
	u32 snd_wnd;
	u32 max_snd_wnd;
	u32 rcv_wnd;
	u32 mss;
	u8 snd_wscale;
	u8 rcv_wscale;
};

struct zxdh_apbvt_entry {
	struct hlist_node hlist;
	u32 use_cnt;
	u16 port;
};

struct zxdh_cm_listener {
	struct list_head list;
	struct iw_cm_id *cm_id;
	struct zxdh_cm_core *cm_core;
	struct zxdh_device *iwdev;
	struct list_head child_listen_list;
	struct zxdh_apbvt_entry *apbvt_entry;
	enum zxdh_cm_listener_state listener_state;
	refcount_t refcnt;
	atomic_t pend_accepts_cnt;
	u32 loc_addr[4];
	u32 reused_node;
	int backlog;
	u16 loc_port;
	u16 vlan_id;
	u8 loc_mac[ETH_ALEN];
	u8 user_pri;
	u8 tos;
	u8 qhash_set : 1;
	u8 ipv4 : 1;
};

struct zxdh_kmem_info {
	void *addr;
	u32 size;
};

struct zxdh_mpa_priv_info {
	const void *addr;
	u32 size;
};

struct zxdh_cm_node {
	struct zxdh_qp *iwqp;
	struct zxdh_device *iwdev;
	struct zxdh_sc_dev *dev;
	struct zxdh_cm_tcp_context tcp_cntxt;
	struct zxdh_cm_core *cm_core;
	struct zxdh_timer_entry *send_entry;
	struct zxdh_timer_entry *close_entry;
	struct zxdh_cm_listener *listener;
	struct list_head timer_entry;
	struct list_head reset_entry;
	struct list_head teardown_entry;
	struct zxdh_apbvt_entry *apbvt_entry;
	struct rcu_head rcu_head;
	struct zxdh_mpa_priv_info pdata;
	struct zxdh_sc_ah *ah;
	struct ietf_mpa_v2 mpa_v2_frame;
	struct zxdh_kmem_info mpa_hdr;
	struct iw_cm_id *cm_id;
	struct hlist_node list;
	struct completion establish_comp;
	spinlock_t retrans_list_lock; /* protect CM node rexmit updates*/
	atomic_t passive_state;
	refcount_t refcnt;
	enum zxdh_cm_node_state state;
	enum send_rdma0 send_rdma0_op;
	enum mpa_frame_ver mpa_frame_rev;
	u32 loc_addr[4], rem_addr[4];
	u16 loc_port, rem_port;
	int apbvt_set;
	int accept_pend;
	u16 vlan_id;
	u16 ird_size;
	u16 ord_size;
	u16 mpav2_ird_ord;
	u16 lsmm_size;
	u8 pdata_buf[IETF_MAX_PRIV_DATA_LEN];
	u8 loc_mac[ETH_ALEN];
	u8 rem_mac[ETH_ALEN];
	u8 user_pri;
	u8 tos;
	u8 ack_rcvd : 1;
	u8 qhash_set : 1;
	u8 ipv4 : 1;
	u8 snd_mark_en : 1;
	u8 rcv_mark_en : 1;
	u8 do_lpb : 1;
	u8 accelerated : 1;
};

struct zxdh_cm_core {
	struct zxdh_device *iwdev;
	struct zxdh_sc_dev *dev;
	struct list_head listen_list;
	DECLARE_HASHTABLE(cm_hash_tbl, 8);
	DECLARE_HASHTABLE(apbvt_hash_tbl, 8);
	struct timer_list tcp_timer;
	struct workqueue_struct *event_wq;
	spinlock_t ht_lock; /* protect CM node (active side) list */
	spinlock_t listen_list_lock; /* protect listener list */
	spinlock_t apbvt_lock; /*serialize apbvt add/del entries*/
	u64 stats_nodes_created;
	u64 stats_nodes_destroyed;
	u64 stats_listen_created;
	u64 stats_listen_destroyed;
	u64 stats_listen_nodes_created;
	u64 stats_listen_nodes_destroyed;
	u64 stats_lpbs;
	u64 stats_accepts;
	u64 stats_rejects;
	u64 stats_connect_errs;
	u64 stats_passive_errs;
	u64 stats_pkt_retrans;
	u64 stats_backlog_drops;
	struct zxdh_puda_buf *(*form_cm_frame)(struct zxdh_cm_node *cm_node,
					       struct zxdh_kmem_info *options,
					       struct zxdh_kmem_info *hdr,
					       struct zxdh_mpa_priv_info *pdata,
					       u8 flags);
	int (*cm_create_ah)(struct zxdh_cm_node *cm_node, bool wait);
	void (*cm_free_ah)(struct zxdh_cm_node *cm_node);
};

bool zxdh_ipv4_is_lpb(u32 loc_addr, u32 rem_addr);
bool zxdh_ipv6_is_lpb(u32 *loc_addr, u32 *rem_addr);
#endif /* ZXDH_CM_H */
