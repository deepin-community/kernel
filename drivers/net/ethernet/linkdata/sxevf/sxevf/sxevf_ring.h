#ifndef __SXEVF_RING_H__
#define __SXEVF_RING_H__

#include "sxe_compat.h"
#include <linux/skbuff.h>
#ifndef HAVE_NO_XDP_BUFF_RXQ
#include <net/xdp.h>
#endif

struct sxevf_adapter;
struct sxevf_irq_data;

#define SXEVF_LONG_32BITS           32
#define SXEVF_PAGE_SIZE_4KB         4096
#define SXEVF_PAGE_SIZE_8KB         8192
#define SXEVF_PAGE_SIZE_64KB        65536

#define SXEVF_DESC_ALIGN_4K         4096

#define SXEVF_BUFFER_PER_4KPAGE     2

#define SXEVF_TXRX_RING_NUM_DEFAULT 1

#define SXEVF_DESC_CNT_MIN          64
#define SXEVF_DESC_CNT_MAX          4096
#define SXEVF_TX_DEFAULT_DESC_CNT   1024
#define SXEVF_RX_DEFAULT_DESC_CNT   512
#define SXEVF_REQ_DESCRIPTOR_MULTIPLE 8
#define SXEVF_DEFAULT_TC_NUM        1

#define SXEVF_RSS_RING_NUM_MAX      2

enum sxevf_ring_state {
	SXEVF_RX_3K_BUFFER,
	SXEVF_RX_BUILD_SKB_ENABLED,
	SXEVF_TX_XDP_RING,
	SXEVF_TX_DETECT_HANG,
	SXEVF_HANG_CHECK_ARMED,
	SXEVF_TX_XDP_RING_PRIMED
};

#define vf_ring_is_xdp(ring) 	test_bit(SXEVF_TX_XDP_RING, &(ring)->state)
#define vf_set_ring_xdp(ring) 	set_bit(SXEVF_TX_XDP_RING, &(ring)->state)
#define vf_clear_ring_xdp(ring) clear_bit(SXEVF_TX_XDP_RING, &(ring)->state)

#define sxevf_for_each_ring(post, head) \
	for (post = (head).next; post != NULL; post = post->next)

#define SXEVF_TX_DESC(R, i)	    \
	(&(((union sxevf_tx_data_desc *)((R)->desc.base_addr))[i]))
#define SXEVF_TX_CTXTDESC(R, i)	    \
	(&(((struct sxevf_tx_context_desc *)((R)->desc.base_addr))[i]))

#define SXEVF_RX_DESC(R, i)    \
	(&(((union sxevf_rx_data_desc *)((R)->desc.base_addr))[i]))

#define SXEVF_RXD_STAT_LB       0x40000 
#define SXEVF_RXD_STAT_L4CS     0x20    
#define SXEVF_RXD_STAT_IPCS     0x40    
#define SXEVF_RXD_STAT_VP       0x08    
#define SXEVF_RXD_STAT_EOP      0x02    

#define SXEVF_TX_POPTS_IXSM   0x01
#define SXEVF_TX_POPTS_TXSM   0x02
#define SXEVF_TXD_POPTS_SHIFT 8
#define SXEVF_TXD_POPTS_IXSM  (SXEVF_TX_POPTS_IXSM << SXEVF_TXD_POPTS_SHIFT)
#define SXEVF_TXD_POPTS_TXSM  (SXEVF_TX_POPTS_TXSM << SXEVF_TXD_POPTS_SHIFT)
#define SXEVF_TXD_IDX         (1u << 4)
#define SXEVF_TXD_CC          0x00000080

#define SXEVF_TX_NON_DATA_DESC_NUM     3
#define SXEVF_TX_DESC_PAYLEN_SHIFT     14
#define SXEVF_DATA_PER_DESC_SIZE_SHIFT 14
#define SXEVF_DATA_PER_DESC_SIZE_MAX   (1u << SXEVF_DATA_PER_DESC_SIZE_SHIFT)
#define SXEVF_TX_DESC_USE_COUNT(S) DIV_ROUND_UP((S), SXEVF_DATA_PER_DESC_SIZE_MAX)

#define SXEVF_TX_DESC_NEEDED    (long int)(MAX_SKB_FRAGS + 4)
#define SXEVF_TX_WAKE_THRESHOLD (SXEVF_TX_DESC_NEEDED * 2)
#define SXEVF_TX_DESC_EOP_MASK  0x01000000   
#define SXEVF_TX_DESC_RS_MASK   0x08000000   
#define SXEVF_TX_DESC_STAT_DD   0x00000001   
#define SXEVF_TX_DESC_CMD       (SXEVF_TX_DESC_EOP_MASK | SXEVF_TX_DESC_RS_MASK)
#define SXEVF_TX_DESC_TYPE_DATA 0x00300000   
#define SXEVF_TX_DESC_DEXT      0x20000000   
#define SXEVF_TX_DESC_IFCS      0x02000000   
#define SXEVF_TX_DESC_VLE       0x40000000   

#define SXEVF_TXD_DCMD_VLE  SXEVF_TX_DESC_VLE
#define SXEVF_TXD_DTYP_CTXT	0x00200000 
#define SXEVF_TXD_DCMD_TSE      0x80000000 

#define SXEVF_RXD_STAT_SECP     0x20000 

#define SXEVF_RXDADV_PKTTYPE_NONE       0x00000000
#define SXEVF_RXDADV_PKTTYPE_IPV4       0x00000010 
#define SXEVF_RXDADV_PKTTYPE_IPV4_EX    0x00000020 
#define SXEVF_RXDADV_PKTTYPE_IPV6       0x00000040 
#define SXEVF_RXDADV_PKTTYPE_IPV6_EX    0x00000080 
#define SXEVF_RXDADV_PKTTYPE_TCP        0x00000100 
#define SXEVF_RXDADV_PKTTYPE_UDP        0x00000200 
#define SXEVF_RXDADV_PKTTYPE_SCTP       0x00000400 
#define SXEVF_RXDADV_PKTTYPE_IPSEC_ESP  0x00001000 
#define SXEVF_RXDADV_PKTTYPE_IPSEC_AH   0x00002000 
#define SXEVF_RXDADV_PKTTYPE_LINKSEC    0x00004000 

#define SXEVF_IP_HEAD_LEN_UNIT          4

#define SXEVF_RXDADV_ERR_L4E    0x40000000 
#define SXEVF_RXDADV_ERR_IPE    0x80000000 

#define SXEVF_TX_CTXTD_MACLEN_SHIFT   9  
#define SXEVF_TX_CTXTD_VLAN_SHIFT     16
#define SXEVF_TX_CTXTD_VLAN_MASK      0xffff0000
#define SXEVF_TX_CTXTD_MACLEN_MASK    0x0000fE00

#define SXEVF_TX_HANG_PROC_ACTIVE(ring) \
	set_bit(SXEVF_TX_DETECT_HANG, &(ring)->state)
#define SXEVF_TX_HANG_CHECK_COMPLETE(ring) \
	clear_bit(SXEVF_TX_DETECT_HANG, &(ring)->state)
#define SXEVF_DETECT_TX_HANG_NEED(ring) \
	test_bit(SXEVF_TX_DETECT_HANG, &(ring)->state)

struct sxevf_ring_stats {
	u64 packets;           
	u64 bytes;             
};

struct sxevf_tx_ring_stats {
	u64 restart_queue;     
	u64 tx_busy;           
	u64 tx_done_old;       
};

struct sxevf_rx_ring_stats {
	u64 alloc_rx_page;            
	u64 alloc_rx_page_failed;     
	u64 alloc_rx_buff_failed;     
	u64 csum_err;                 
};

struct sxevf_ring_desc {
	void *base_addr;     
	u8 __iomem *tail;    
	dma_addr_t dma;      
};
struct sxevf_ring {
	struct sxevf_ring *next;         
	struct sxevf_irq_data *irq_data; 
	struct net_device *netdev;       
	struct device *dev;	         
	struct sk_buff *skb;

	u8  idx;                       
	u8  reg_idx;                   
	u8  tc_idx;                    

	unsigned long state;           

	u16 next_to_use;	       
	u16 next_to_clean;    	       
	u16 next_to_alloc;             

	u16   depth;                   
	u32   size;                    
	struct sxevf_ring_desc desc;     

	union {
		struct sxevf_tx_buffer *tx_buffer_info;  
		struct sxevf_rx_buffer *rx_buffer_info;  
	};

	struct bpf_prog *xdp_prog;
#ifndef HAVE_NO_XDP_BUFF_RXQ
	struct xdp_rxq_info xdp_rxq;
#endif

	struct sxevf_ring_stats stats;            
	struct u64_stats_sync syncp;            
	union {
		struct sxevf_tx_ring_stats tx_stats;  
		struct sxevf_rx_ring_stats rx_stats;  
	};

} ____cacheline_internodealigned_in_smp;

struct sxevf_ring_feature {
	u16 rss_limit;           
	union {
		u16 tc_per_pool;     
		u16 ring_per_pool;   
	};
} ____cacheline_internodealigned_in_smp;

struct sxevf_ring_context {
	u16 max_tx_num;  
	u16 max_rx_num;  
	u16 num;         
	u16 depth;       
	struct sxevf_ring *ring[SXEVF_TXRX_RING_NUM_MAX]; 
};

struct sxevf_tx_buffer {
	union sxevf_tx_data_desc *next_to_watch;
	unsigned long time_stamp;

	union {
		struct sk_buff *skb;
		void *data;     
	};
	unsigned int bytecount;
	unsigned short gso_segs;
	__be16 protocol;
	DEFINE_DMA_UNMAP_ADDR(dma);
	DEFINE_DMA_UNMAP_LEN(len);
	u32 tx_features;
};

union sxevf_tx_data_desc {
	struct {
		__le64 buffer_addr;      
		__le32 cmd_type_len;
		__le32 olinfo_status;
	} read;
	struct {
		__le64 rsvd;       
		__le32 nxtseq_seed;
		__le32 status;
	} wb;
};

struct sxevf_tx_context_desc {
	__le32 vlan_macip_lens;
	__le32 sa_idx;
	__le32 type_tucmd_mlhl;
	__le32 mss_l4len_idx;
};

struct sxevf_rx_buffer {
	dma_addr_t dma;
	struct page *page;
#if (BITS_PER_LONG > SXEVF_LONG_32BITS) || (PAGE_SIZE >= SXEVF_PAGE_SIZE_64KB)
	__u32 page_offset;
#else
	__u16 page_offset;
#endif
	__u16 pagecnt_bias;
};

union sxevf_rx_data_desc {
	struct {
		__le64 pkt_addr; 
		__le64 hdr_addr; 
	} read;
	struct {
		struct {
			union {
				__le32 data;
				struct {
					__le16 pkt_info; 
					__le16 hdr_info; 
				} hs_rss;
			} lo_dword;
			union {
				__le32 rss; 
				struct {
					__le16 ip_id; 
					__le16 csum; 
				} csum_ip;
			} hi_dword;
		} lower;
		struct {
			__le32 status_error; 
			__le16 length; 
			__le16 vlan; 
		} upper;
	} wb;  
};

static inline __le32 sxevf_status_err_check(union sxevf_rx_data_desc *rx_desc,
					const u32 stat_err_bits)
{
	return rx_desc->wb.upper.status_error & cpu_to_le32(stat_err_bits);
}

s32 sxevf_ring_info_get(struct sxevf_adapter *adapter,
			u8 *tc_num, u8 *default_tc, u8 *max_tx_num);

void sxevf_tx_ring_init(struct sxevf_adapter *adapter, u16 base,
		    u16  txr_cnt, u16 ring_idx, u16 irq_idx, u16 reg_idx);

void sxevf_xdp_ring_init(struct sxevf_adapter *adapter, u16 base,
		      u16  xdp_cnt, u16 ring_idx, u16 irq_idx, u16 reg_idx);

void sxevf_rx_ring_init(struct sxevf_adapter *adapter, u16 base,
		     u16  rxr_cnt, u16 ring_idx, u16 irq_idx, u16 reg_idx);

void sxevf_ring_num_set(struct sxevf_adapter *adapter);

void sxevf_ring_feature_init(struct sxevf_adapter *adapter);

#endif 
