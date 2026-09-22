/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_VERBS_H
#define ZXDH_VERBS_H

#define ZXDH_MAX_SAVED_PHY_PGADDR 4
#define ZXDH_FLUSH_DELAY_MS 200

#define ZXDH_MAX_CQ_COUNT 0xFFFF
#define ZXDH_MAX_CQ_PERIOD 0x7FF

#define US_TO_NS(us) ((us) * 1000)
#define NS_TO_US(ns) ((ns) / 1000)

#define ZXDH_PKEY_TBL_SZ 1
#define ZXDH_DEFAULT_PKEY 0xFFFF
#define ZXDH_MAX_AH 0x7FFFFFFF
#define ZXDH_MAX_AH_LIST 0x20000

#define ZRDMA_UDP_SPORT_BASE (50000)
#define ZRDMA_UDP_SPORT_NUM (15500)

#define ZXDH_MAILBOX_ADDR_BUF_LEN 5

#define iwdev_to_idev(iwdev) (&(iwdev)->rf->sc_dev)

struct zxdh_cap_entry_info {
#ifdef RDMA_MMAP_DB_SUPPORT
	struct rdma_user_mmap_entry *cap_mmap_entry;
#else
	struct zxdh_user_mmap_entry *cap_mmap_entry;
#endif
};

struct zxdh_dma_addr {
	dma_addr_t cap_dma_addr;
	void *cap_cpu_addr;
};

struct zxdh_cap_addr_info {
	union
	{
		u64 cap_iova_addr;
		struct zxdh_dma_addr cap_direct_dma_addr;
	} addr_info;
	struct zxdh_cap_entry_info entry_info;
	struct zxdh_dma_addr cap_ext_mem_addr;   /* ext_mem 模式下保存 ext_mem 的 VA 和 DMA 地址 */
};

struct zxdh_hw_data_cap_info {
	struct zxdh_cap_addr_info cap_txrx_use_iova[CAP_NODE_NUM];
	struct zxdh_cap_addr_info cap_tx_use_direct_dma[CAP_NODE_NUM];
	struct zxdh_cap_addr_info cap_rx_use_direct_dma[CAP_NODE_NUM];
	struct zxdh_cap_addr_info mp_cap;
	struct zxdh_cap_addr_info hw_object_mmap;
	u64 mp_cap_media_addr_base;
	u32 object_buffer_size;
};

struct zxdh_ucontext {
	struct ib_ucontext ibucontext;
	struct zxdh_device *iwdev;
#ifdef RDMA_MMAP_DB_SUPPORT
	struct rdma_user_mmap_entry *sq_db_mmap_entry;
	struct rdma_user_mmap_entry *cq_db_mmap_entry;
    struct rdma_user_mmap_entry *srq_db_mmap_entry;
	struct rdma_user_mmap_entry *ext_sq_db_mmap_entry;
	struct rdma_user_mmap_entry *l2d_addr_mmap_entry;
#else
	struct zxdh_user_mmap_entry *sq_db_mmap_entry;
	struct zxdh_user_mmap_entry *cq_db_mmap_entry;
    struct zxdh_user_mmap_entry *srq_db_mmap_entry;
	struct zxdh_user_mmap_entry *ext_sq_db_mmap_entry;
	struct zxdh_user_mmap_entry *l2d_addr_mmap_entry;
	DECLARE_HASHTABLE(mmap_hash_tbl, 6);
	spinlock_t mmap_tbl_lock; /* protect mmap hash table entries */
#endif
    struct zxdh_hw_data_cap_info hw_data_cap;
	struct list_head cq_reg_mem_list;
	spinlock_t cq_reg_mem_list_lock; /* protect CQ memory list */
	struct list_head qp_reg_mem_list;
	spinlock_t qp_reg_mem_list_lock; /* protect QP memory list */
	struct list_head srq_reg_mem_list;
	spinlock_t srq_reg_mem_list_lock; /* protect QP memory list */
	/* FIXME: Move to kcompat ideally. Used < 4.20.0 for old diassasscoaite flow */
	struct list_head vma_list;
	struct mutex vma_list_mutex; /* protect the vma_list */
	int abi_ver;
	bool legacy_mode;
	void *rdma_tool_context;
	atomic_t tx_port_affinity;
};

struct zxdh_pd {
	struct ib_pd ibpd;
	struct zxdh_sc_pd sc_pd;
};

struct zxdh_av {
	u8 macaddr[16];
	struct rdma_ah_attr attrs;
	union {
		struct sockaddr saddr;
		struct sockaddr_in saddr_in;
		struct sockaddr_in6 saddr_in6;
	} sgid_addr, dgid_addr;
	u8 net_type;
};

struct zxdh_ah {
	struct ib_ah ibah;
	struct zxdh_sc_ah sc_ah;
	struct zxdh_pd *pd;
	struct zxdh_av av;
	u8 sgid_index;
	union ib_gid dgid;
	struct list_head list;
	refcount_t refcnt;
	struct zxdh_ah *parent_ah; /* AH from cached list */
};

struct zxdh_hmc_pble {
	union {
		u32 idx;
		dma_addr_t addr;
	};
};

struct zxdh_cq_mr {
	struct zxdh_hmc_pble cq_pbl;
	dma_addr_t shadow;
	bool split;
};

struct zxdh_qp_mr {
	struct zxdh_hmc_pble sq_pbl;
	struct zxdh_hmc_pble rq_pbl;
	dma_addr_t shadow;
	struct page *sq_page;
};

struct zxdh_srq_mr {
	struct zxdh_hmc_pble srq_pbl;
	struct zxdh_hmc_pble srq_list_pbl;
	struct page *srq_page;
	dma_addr_t db_addr;
};

struct zxdh_cq_buf {
	struct zxdh_dma_mem kmem_buf;
	struct zxdh_cq_uk cq_uk;
	struct zxdh_hw *hw;
	struct list_head list;
	struct work_struct work;
};

struct zxdh_pbl {
	struct list_head list;
	union {
		struct zxdh_qp_mr qp_mr;
		struct zxdh_cq_mr cq_mr;
		struct zxdh_srq_mr srq_mr;
	};

	u8 pbl_allocated : 1;
	u8 on_list : 1;
	u64 user_base;
	struct zxdh_pble_alloc pble_alloc;
	struct zxdh_mr *iwmr;
};

struct zxdh_mr {
	union {
		struct ib_mr ibmr;
		struct ib_mw ibmw;
	};
	struct ib_umem *region;
	struct zxdh_sc_dev *sc_dev;
	int access;
	u8 is_hwreg;
	u16 type;
	u32 page_cnt;
	u64 page_size;
	u64 page_msk;
	u32 npages;
	u32 stag;
	u64 len;
	u64 pgaddrmem[ZXDH_MAX_SAVED_PHY_PGADDR];
	struct zxdh_pbl iwpbl;
};

struct zxdh_cq {
	struct ib_cq ibcq;
	struct zxdh_sc_cq sc_cq;
	u16 cq_head;
	u16 cq_size;
	u32 cq_num;
	bool user_mode;
	bool armed;
	enum zxdh_cmpl_notify last_notify;
	u32 polled_cmpls;
	u32 cq_mem_size;
	struct zxdh_dma_mem kmem;
	struct zxdh_dma_mem kmem_shadow;
	struct completion free_cq;
	refcount_t refcnt;
	spinlock_t lock; /* for poll cq */
	struct zxdh_pbl *iwpbl;
	struct zxdh_pbl *iwpbl_shadow;
	struct list_head resize_list;
	struct zxdh_cq_poll_info cur_cqe;
	struct list_head cmpl_generated;
};

struct zxdh_cmpl_gen {
	struct list_head list;
	struct zxdh_cq_poll_info cpi;
};

struct mailbox_work {
	struct work_struct work;
	u64 op_ret_val;
	__le64 addrbuf[ZXDH_MAILBOX_ADDR_BUF_LEN];
	struct zxdh_sc_dev *dev;
};

struct aeq_qp_work {
	struct work_struct work;
	struct zxdh_qp *iwqp;
};

struct iw_cm_id;

struct zxdh_qp_kmode {
	struct zxdh_dma_mem dma_mem;
	struct zxdh_sq_uk_wr_trk_info *sq_wrid_mem;
	u64 *rq_wrid_mem;
};

struct zxdh_srq_kmode {
	struct zxdh_dma_mem dma_mem;
	u64 *srq_wrid_mem;
};

struct zxdh_qp {
	struct ib_qp ibqp;
	struct zxdh_sc_qp sc_qp;
	struct zxdh_device *iwdev;
	struct zxdh_cq *iwscq;
	struct zxdh_cq *iwrcq;
	struct zxdh_pd *iwpd;
	struct zxdh_srq *iwsrq;
#ifdef RDMA_MMAP_DB_SUPPORT
	struct rdma_user_mmap_entry *push_wqe_mmap_entry;
	struct rdma_user_mmap_entry *push_db_mmap_entry;
#else
	struct zxdh_user_mmap_entry *push_wqe_mmap_entry;
	struct zxdh_user_mmap_entry *push_db_mmap_entry;
#endif
	struct zxdh_qp_host_ctx_info ctx_info;
	union {
		struct zxdh_iwarp_offload_info iwarp_info;
		struct zxdh_roce_offload_info roce_info;
	};

	union {
		struct zxdh_tcp_offload_info tcp_info;
		struct zxdh_udp_offload_info udp_info;
	};

	struct zxdh_ah roce_ah;
	struct list_head teardown_entry;
	refcount_t refcnt;
	struct iw_cm_id *cm_id;
	struct zxdh_cm_node *cm_node;
	struct delayed_work dwork_flush;
	struct ib_mr *lsmm_mr;
	atomic_t hw_mod_qp_pend;
	enum ib_qp_state ibqp_state;
	u32 qp_mem_size;
	u32 last_aeq;
	int max_send_wr;
	int max_recv_wr;
	atomic_t close_timer_started;
	spinlock_t lock; /* serialize posting WRs to SQ/RQ */
	struct zxdh_qp_context *iwqp_context;
	void *pbl_vbase;
	dma_addr_t pbl_pbase;
	struct page *page;
	u8 active_conn : 1;
	u8 user_mode : 1;
	u8 hte_added : 1;
	u8 flush_issued : 1;
	u8 sig_all : 1;
	u8 pau_mode : 1;
	u8 rsvd : 1;
	u8 iwarp_state;
	u16 term_sq_flush_code;
	u16 term_rq_flush_code;
	u8 hw_iwarp_state;
	u8 hw_tcp_state;
	u8 is_srq;
	struct zxdh_qp_kmode kqp;
	struct zxdh_dma_mem host_ctx;
	struct timer_list terminate_timer;
	struct zxdh_pbl *iwpbl;
	struct zxdh_sge *sg_list;
	struct zxdh_dma_mem ietf_mem;
	struct completion free_qp;
	wait_queue_head_t waitq;
	wait_queue_head_t mod_qp_waitq;
	u8 rts_ae_rcvd;
	uint8_t inline_data[ZXDH_MAX_INLINE_DATA_SIZE];
};

enum zxdh_mmap_flag {
	ZXDH_MMAP_IO_NC,
	ZXDH_MMAP_IO_WC,
	ZXDH_MMAP_PFN,
	ZXDH_MMAP_HMC,
    ZXDH_MMAP_IO_NC_BY_SIZE,
	ZXDH_MMAP_L2D,
};

struct zxdh_user_mmap_entry {
#ifdef RDMA_MMAP_DB_SUPPORT
	struct rdma_user_mmap_entry rdma_entry;
#else
	struct zxdh_ucontext *ucontext;
	struct hlist_node hlist;
	u64 pgoff_key; /* Used to compute offset (in bytes) returned to user libc's mmap */
#endif
	u64 bar_offset;
	u8 mmap_flag;
};

static inline u16 zxdh_fw_major_ver(struct zxdh_sc_dev *dev)
{
	return (u16)FIELD_GET(ZXDH_FW_VER_MAJOR,
			      dev->feature_info[ZXDH_FEATURE_FW_INFO]);
}

static inline u16 zxdh_fw_minor_ver(struct zxdh_sc_dev *dev)
{
	return (u16)FIELD_GET(ZXDH_FW_VER_MINOR,
			      dev->feature_info[ZXDH_FEATURE_FW_INFO]);
}

/**
 * zxdh_mcast_mac_v4 - Get the multicast MAC for an IP address
 * @ip_addr: IPv4 address
 * @mac: pointer to result MAC address
 *
 */
static inline void zxdh_mcast_mac_v4(u32 *ip_addr, u8 *mac)
{
	u8 *ip = (u8 *)ip_addr;
	unsigned char mac4[ETH_ALEN] = { 0x01,	       0x00,  0x5E,
					 ip[2] & 0x7F, ip[1], ip[0] };

	ether_addr_copy(mac, mac4);
}

/**
 * zxdh_mcast_mac_v6 - Get the multicast MAC for an IP address
 * @ip_addr: IPv6 address
 * @mac: pointer to result MAC address
 *
 */
static inline void zxdh_mcast_mac_v6(u32 *ip_addr, u8 *mac)
{
	u8 *ip = (u8 *)ip_addr;
	unsigned char mac6[ETH_ALEN] = {
		0x33, 0x33, ip[3], ip[2], ip[1], ip[0]
	};

	ether_addr_copy(mac, mac6);
}

void *zxdh_zalloc_mapped(struct zxdh_device *dev, dma_addr_t *dma_addr,
			 size_t size, enum dma_data_direction dir);
void zxdh_free_mapped(struct zxdh_device *dev, void *cpu_addr,
		      dma_addr_t dma_addr, size_t size,
		      enum dma_data_direction dir);

#ifdef RDMA_MMAP_DB_SUPPORT
struct rdma_user_mmap_entry *
zxdh_user_mmap_entry_insert(struct zxdh_ucontext *ucontext, u64 bar_offset,
			    enum zxdh_mmap_flag mmap_flag, u64 *mmap_offset);
struct rdma_user_mmap_entry *
zxdh_cap_mmap_entry_insert(struct zxdh_ucontext *ucontext, void *address,
			   size_t length, enum zxdh_mmap_flag mmap_flag,
			   u64 *mmap_offset);
struct rdma_user_mmap_entry *
zxdh_mp_mmap_entry_insert(struct zxdh_ucontext *ucontext, u64 phy_addr,
			  size_t length, enum zxdh_mmap_flag mmap_flag,
			  u64 *mmap_offset);
#else
struct zxdh_user_mmap_entry *
zxdh_user_mmap_entry_add_hash(struct zxdh_ucontext *ucontext, u64 bar_offset,
			      enum zxdh_mmap_flag mmap_flag, u64 *mmap_offset);
void zxdh_user_mmap_entry_del_hash(struct zxdh_user_mmap_entry *entry);
#endif /* RDMA_MMAP_DB_SUPPORT */
int zxdh_ib_register_device(struct zxdh_device *iwdev);
void zxdh_ib_unregister_device(struct zxdh_device *iwdev);
void zxdh_ib_dealloc_device(struct ib_device *ibdev);
int zxdh_tc_sysfs_init(struct zxdh_device *iwdev);
void zxdh_tc_sysfs_cleanup(struct zxdh_device *iwdev);
void zxdh_ib_qp_event(struct zxdh_qp *iwqp, enum zxdh_qp_event_type event);
void zxdh_generate_flush_completions(struct zxdh_qp *iwqp);
void zxdh_remove_cmpls_list(struct zxdh_cq *iwcq);
enum zxdh_status_code zxdh_generated_cmpls(struct zxdh_cq *iwcq,
			 struct zxdh_cq_poll_info *cq_poll_info);
void zxdh_flush_worker(struct work_struct *work);
void extract_version(const char *input, char *output);
#ifndef ZXDH_UAPI_DEF
int zxdh_get_dri_specs(struct zxdh_device *iwdev);
#endif
int remote_ip_info_process(struct zxdh_device *iwdev, struct zxdh_rdma_to_eth_ip_para *ip_para);
int del_qp_remote_ip_info(struct ib_qp *ibqp);
int qp_remote_ip_info_process(struct ib_qp *ibqp, int op_type);
#endif /* ZXDH_VERBS_H */
