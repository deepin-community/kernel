/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_PUDA_H
#define ZXDH_PUDA_H

#define ZXDH_IEQ_MPA_FRAMING 6
#define ZXDH_TCP_OFFSET 40
#define ZXDH_IPV4_PAD 20
#define ZXDH_MRK_BLK_SZ 512

enum puda_rsrc_type {
	ZXDH_PUDA_RSRC_TYPE_ILQ = 1,
	ZXDH_PUDA_RSRC_TYPE_IEQ,
	ZXDH_PUDA_RSRC_TYPE_MAX, /* Must be last entry */
};

enum puda_rsrc_complete {
	PUDA_CQ_CREATED = 1,
	PUDA_QP_CREATED,
	PUDA_TX_COMPLETE,
	PUDA_RX_COMPLETE,
	PUDA_HASH_CRC_COMPLETE,
};

struct zxdh_sc_dev;
struct zxdh_sc_qp;
struct zxdh_sc_cq;

struct zxdh_puda_cmpl_info {
	struct zxdh_qp_uk *qp;
	u8 q_type;
	u8 l3proto;
	u8 l4proto;
	u16 vlan;
	u32 payload_len;
	u32 compl_error; /* No_err=0, else major and minor err code */
	u32 qp_id;
	u32 wqe_idx;
	u8 ipv4 : 1;
	u8 smac_valid : 1;
	u8 vlan_valid : 1;
	u8 smac[ETH_ALEN];
};

struct zxdh_puda_send_info {
	u64 paddr; /* Physical address */
	u32 len;
	u32 ah_id;
	u8 tcplen;
	u8 maclen;
	u8 ipv4 : 1;
	u8 do_lpb : 1;
	void *scratch;
};

struct zxdh_puda_buf {
	struct list_head list; /* MUST be first entry */
	struct zxdh_dma_mem mem; /* DMA memory for the buffer */
	struct zxdh_puda_buf *next; /* for alloclist in rsrc struct */
	struct zxdh_virt_mem buf_mem; /* Buffer memory for this buffer */
	void *scratch;
	u8 *iph;
	u8 *tcph;
	u8 *data;
	u16 datalen;
	u16 vlan_id;
	u8 tcphlen; /* tcp length in bytes */
	u8 maclen; /* mac length in bytes */
	u32 totallen; /* machlen+iphlen+tcphlen+datalen */
	refcount_t refcount;
	u8 hdrlen;
	u8 ipv4 : 1;
	u8 vlan_valid : 1;
	u8 do_lpb : 1; /* Loopback buffer */
	u8 smac_valid : 1;
	u32 seqnum;
	u32 ah_id;
	u8 smac[ETH_ALEN];
	struct zxdh_sc_vsi *vsi;
};

struct zxdh_puda_rsrc_info {
	void (*receive)(struct zxdh_sc_vsi *vsi, struct zxdh_puda_buf *buf);
	void (*xmit_complete)(struct zxdh_sc_vsi *vsi, void *sqwrid);
	enum puda_rsrc_type type; /* ILQ or IEQ */
	u32 count;
	u32 pd_id;
	u32 cq_id;
	u32 qp_id;
	u32 sq_size;
	u32 rq_size;
	u32 tx_buf_cnt; /* total bufs allocated will be rq_size + tx_buf_cnt */
	u16 buf_size;
	u8 stats_idx;
	u8 stats_idx_valid : 1;
	int abi_ver;
};

struct zxdh_puda_rsrc {
	struct zxdh_sc_cq cq;
	struct zxdh_sc_qp qp;
	struct zxdh_sc_pd sc_pd;
	struct zxdh_sc_dev *dev;
	struct zxdh_sc_vsi *vsi;
	struct zxdh_dma_mem cqmem;
	struct zxdh_dma_mem qpmem;
	struct zxdh_virt_mem ilq_mem;
	enum puda_rsrc_complete cmpl;
	enum puda_rsrc_type type;
	u16 buf_size; /*buf must be max datalen + tcpip hdr + mac */
	u32 cq_id;
	u32 qp_id;
	u32 sq_size;
	u32 rq_size;
	u32 cq_size;
	struct zxdh_sq_uk_wr_trk_info *sq_wrtrk_array;
	u64 *rq_wrid_array;
	u32 compl_rxwqe_idx;
	u32 rx_wqe_idx;
	u32 rxq_invalid_cnt;
	u32 tx_wqe_avail_cnt;
	struct shash_desc *hash_desc;
	struct list_head txpend;
	struct list_head bufpool; /* free buffers pool list for recv and xmit */
	u32 alloc_buf_count;
	u32 avail_buf_count; /* snapshot of currently available buffers */
	spinlock_t bufpool_lock;
	struct zxdh_puda_buf *alloclist;
	void (*receive)(struct zxdh_sc_vsi *vsi, struct zxdh_puda_buf *buf);
	void (*xmit_complete)(struct zxdh_sc_vsi *vsi, void *sqwrid);
	/* puda stats */
	u64 stats_buf_alloc_fail;
	u64 stats_pkt_rcvd;
	u64 stats_pkt_sent;
	u64 stats_rcvd_pkt_err;
	u64 stats_sent_pkt_q;
	u64 stats_bad_qp_id;
	/* IEQ stats */
	u64 fpdu_processed;
	u64 bad_seq_num;
	u64 crc_err;
	u64 pmode_count;
	u64 partials_handled;
	u8 stats_idx;
	u8 check_crc : 1;
	u8 stats_idx_valid : 1;
};

void zxdh_puda_ret_bufpool(struct zxdh_puda_rsrc *rsrc,
			   struct zxdh_puda_buf *buf);
int zxdh_puda_send(struct zxdh_sc_qp *qp, struct zxdh_puda_send_info *info);

int zxdh_cqp_qp_destroy_cmd(struct zxdh_sc_dev *dev, struct zxdh_sc_qp *qp);
#endif /*ZXDH_PROTOS_H */
