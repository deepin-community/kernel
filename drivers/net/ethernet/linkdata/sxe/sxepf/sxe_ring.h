
#ifndef __SXE_RING_H__
#define __SXE_RING_H__

#include "sxe_compat.h"
#include <linux/skbuff.h>
#ifndef HAVE_NO_XDP_BUFF_RXQ
#include <net/xdp.h>
#endif

struct sxe_adapter;
struct sxe_irq_data;

#define SXE_NON_RSS_RING_NUM      1
#define MIN_QUEUES_IN_SRIOV       4
#define SXE_RSS_RING_NUM_MAX      16
#define SXE_TXRX_RING_NUM_MAX     64  
#define SXE_XDP_RING_NUM_MAX      SXE_TXRX_RING_NUM_MAX
#define SXE_FNAV_RING_NUM_MAX     (SXE_TXRX_RING_NUM_MAX - 1)

#define SXE_NUM_PF_POOL_DEFAULT 1

#define SXE_PAGE_SIZE_4KB         4096
#define SXE_PAGE_SIZE_8KB         8192
#define SXE_PAGE_SIZE_64KB        65536

#define SXE_IP_HEAD_LEN_UNIT	  4

#define SXE_TC_IDX2 2
#define SXE_TC_IDX3 3
#define SXE_TC_IDX5 5
#define SXE_DCB_TC_MAX 8
#define SXE_8_RING_PER_TC 8
#define SXE_16_RING_PER_TC 16
#define SXE_TX_RING_OFFSET_2 2
#define SXE_TX_RING_OFFSET_4 4
#define SXE_TX_RING_OFFSET_8 8
#define SXE_TC1_TX_RING_SHIFT_6 6
#define SXE_TC2_TX_RING_SHIFT_5 5
#define SXE_TC4_TX_RING_SHIFT_4 4
#define SXE_TC5_TX_RING_SHIFT_3 3
#define SXE_8TC_RX_RING_SHIFT_4 4
#define SXE_4TC_RX_RING_SHIFT_5 5
#define SXE_MAX_QOS_IDX			7

#define SXE_DESC_CNT_MAX       4096
#define SXE_DESC_CNT_MIN       64
#define SXE_DEFAULT_DESC_CNT   512
#define SXE_REQ_DESCRIPTOR_MULTIPLE 8

#define SXE_TX_WORK_LIMIT    256

#define SXE_RSS_16Q_MASK      0xF
#define SXE_RSS_8Q_MASK	      0x7
#define SXE_RSS_4Q_MASK	      0x3
#define SXE_RSS_2Q_MASK	      0x1
#define SXE_RSS_DISABLED_MASK 0x0

#define SXE_RXD_STAT_DD       0x01    
#define SXE_RXD_STAT_EOP      0x02    
#define SXE_RXD_STAT_FLM      0x04    
#define SXE_RXD_STAT_VP       0x08    
#define SXE_RXDADV_NEXTP_MASK   0x000FFFF0 
#define SXE_RXDADV_NEXTP_SHIFT  0x00000004
#define SXE_RXD_STAT_UDPCS    0x10    
#define SXE_RXD_STAT_L4CS     0x20    
#define SXE_RXD_STAT_IPCS     0x40    
#define SXE_RXD_STAT_PIF      0x80    
#define SXE_RXD_STAT_CRCV     0x100   
#define SXE_RXD_STAT_OUTERIPCS  0x100 
#define SXE_RXD_STAT_VEXT     0x200   
#define SXE_RXD_STAT_UDPV     0x400   
#define SXE_RXD_STAT_DYNINT   0x800   
#define SXE_RXD_STAT_LLINT    0x800   
#define SXE_RXD_STAT_TSIP     0x08000 
#define SXE_RXD_STAT_TS       0x10000 
#define SXE_RXD_STAT_SECP     0x20000 
#define SXE_RXD_STAT_LB       0x40000 
#define SXE_RXD_STAT_ACK      0x8000  
#define SXE_RXD_ERR_CE        0x01    
#define SXE_RXD_ERR_LE        0x02    
#define SXE_RXD_ERR_PE        0x08    
#define SXE_RXD_ERR_OSE       0x10    
#define SXE_RXD_ERR_USE       0x20    
#define SXE_RXD_ERR_TCPE      0x40    
#define SXE_RXD_ERR_IPE       0x80    
#define SXE_RXDADV_ERR_MASK           0xfff00000 
#define SXE_RXDADV_ERR_SHIFT          20         
#define SXE_RXDADV_ERR_OUTERIPER	0x04000000 
#define SXE_RXDADV_ERR_FCEOFE         0x80000000 
#define SXE_RXDADV_ERR_FCERR          0x00700000 
#define SXE_RXDADV_ERR_FNAV_LEN       0x00100000 
#define SXE_RXDADV_ERR_FNAV_DROP      0x00200000 
#define SXE_RXDADV_ERR_FNAV_COLL      0x00400000 
#define SXE_RXDADV_ERR_HBO    0x00800000 
#define SXE_RXDADV_ERR_CE     0x01000000 
#define SXE_RXDADV_ERR_LE     0x02000000 
#define SXE_RXDADV_ERR_PE     0x08000000 
#define SXE_RXDADV_ERR_OSE    0x10000000 
#define SXE_RXDADV_ERR_IPSEC_INV_PROTOCOL  0x08000000 
#define SXE_RXDADV_ERR_IPSEC_INV_LENGTH    0x10000000 
#define SXE_RXDADV_ERR_IPSEC_AUTH_FAILED   0x18000000
#define SXE_RXDADV_ERR_USE    0x20000000 
#define SXE_RXDADV_ERR_L4E    0x40000000 
#define SXE_RXDADV_ERR_IPE    0x80000000 
#define SXE_RXD_VLAN_ID_MASK  0x0FFF  
#define SXE_RXD_PRI_MASK      0xE000  
#define SXE_RXD_PRI_SHIFT     13
#define SXE_RXD_CFI_MASK      0x1000  
#define SXE_RXD_CFI_SHIFT     12
#define SXE_RXDADV_LROCNT_MASK        0x001E0000
#define SXE_RXDADV_LROCNT_SHIFT       17
#define SXE_MAX_VLAN_IDX			  4095

#define SXE_RXDADV_STAT_DD            SXE_RXD_STAT_DD  
#define SXE_RXDADV_STAT_EOP           SXE_RXD_STAT_EOP 
#define SXE_RXDADV_STAT_FLM           SXE_RXD_STAT_FLM 
#define SXE_RXDADV_STAT_VP            SXE_RXD_STAT_VP  
#define SXE_RXDADV_STAT_MASK          0x000fffff 
#define SXE_RXDADV_STAT_TS		0x00010000 
#define SXE_RXDADV_STAT_SECP		0x00020000 

#define SXE_RXDADV_RSSTYPE_NONE       0x00000000
#define SXE_RXDADV_RSSTYPE_IPV4_TCP   0x00000001
#define SXE_RXDADV_RSSTYPE_IPV4       0x00000002
#define SXE_RXDADV_RSSTYPE_IPV6_TCP   0x00000003
#define SXE_RXDADV_RSSTYPE_IPV6_EX    0x00000004
#define SXE_RXDADV_RSSTYPE_IPV6       0x00000005
#define SXE_RXDADV_RSSTYPE_IPV6_TCP_EX 0x00000006
#define SXE_RXDADV_RSSTYPE_IPV4_UDP   0x00000007
#define SXE_RXDADV_RSSTYPE_IPV6_UDP   0x00000008
#define SXE_RXDADV_RSSTYPE_IPV6_UDP_EX 0x00000009

#define SXE_RSS_L4_TYPES_MASK \
	((1ul << SXE_RXDADV_RSSTYPE_IPV4_TCP) | \
	 (1ul << SXE_RXDADV_RSSTYPE_IPV4_UDP) | \
	 (1ul << SXE_RXDADV_RSSTYPE_IPV6_TCP) | \
	 (1ul << SXE_RXDADV_RSSTYPE_IPV6_UDP))

#define SXE_RXDADV_PKTTYPE_NONE       0x00000000
#define SXE_RXDADV_PKTTYPE_IPV4       0x00000010 
#define SXE_RXDADV_PKTTYPE_IPV4_EX    0x00000020 
#define SXE_RXDADV_PKTTYPE_IPV6       0x00000040 
#define SXE_RXDADV_PKTTYPE_IPV6_EX    0x00000080 
#define SXE_RXDADV_PKTTYPE_TCP        0x00000100 
#define SXE_RXDADV_PKTTYPE_UDP        0x00000200 
#define SXE_RXDADV_PKTTYPE_SCTP       0x00000400 
#define SXE_RXDADV_PKTTYPE_NFS        0x00000800 
#define SXE_RXDADV_PKTTYPE_VXLAN      0x00000800 
#define SXE_RXDADV_PKTTYPE_TUNNEL     0x00010000 
#define SXE_RXDADV_PKTTYPE_IPSEC_ESP  0x00001000 
#define SXE_RXDADV_PKTTYPE_IPSEC_AH   0x00002000 
#define SXE_RXDADV_PKTTYPE_LINKSEC    0x00004000 
#define SXE_RXDADV_PKTTYPE_ETQF       0x00008000 
#define SXE_RXDADV_PKTTYPE_ETQF_MASK  0x00000070 
#define SXE_RXDADV_PKTTYPE_ETQF_SHIFT 4          

#define SXE_HW_RING_IN_POOL(POOL_MASK) (__ALIGN_MASK(1, ~(POOL_MASK)))

#define PF_POOL_INDEX(p)   ((p) + adapter->pool_f.vf_num_used)
#define SXE_RX_DESC(R, i)    \
	(&(((union sxe_rx_data_desc *)((R)->desc.base_addr))[i]))

#define ring_is_lro_enabled(ring) \
	test_bit(SXE_RX_LRO_ENABLED, &(ring)->state)
#define set_ring_lro_enabled(ring) \
	set_bit(SXE_RX_LRO_ENABLED, &(ring)->state)
#define clear_ring_lro_enabled(ring) \
	clear_bit(SXE_RX_LRO_ENABLED, &(ring)->state)

#define ring_is_xdp(ring) \
	test_bit(SXE_TX_XDP_RING, &(ring)->state)
#define set_ring_xdp(ring) \
	set_bit(SXE_TX_XDP_RING, &(ring)->state)
#define clear_ring_xdp(ring) \
	clear_bit(SXE_TX_XDP_RING, &(ring)->state)

#define sxe_for_each_ring(post, head) \
	for (post = (head).next; post != NULL; post = post->next)

#define SXE_TX_DESC(R, i)	    \
	(&(((union sxe_tx_data_desc *)((R)->desc.base_addr))[i]))
#define SXE_TX_CTXTDESC(R, i)	    \
	(&(((struct sxe_tx_context_desc *)((R)->desc.base_addr))[i]))

#define SXE_TX_DESC_NEEDED    (MAX_SKB_FRAGS + 4)
#define SXE_TX_WAKE_THRESHOLD (SXE_TX_DESC_NEEDED * 2)

#define SXE_TX_NON_DATA_DESC_NUM     3
#define SXE_DATA_PER_DESC_SIZE_SHIFT 14
#define SXE_DATA_PER_DESC_SIZE_MAX   (1u << SXE_DATA_PER_DESC_SIZE_SHIFT)
#define SXE_TX_DESC_USE_COUNT(S) DIV_ROUND_UP((S), SXE_DATA_PER_DESC_SIZE_MAX)
#define SXE_TX_DESC_PREFETCH_THRESH_1 1
#define SXE_TX_DESC_PREFETCH_THRESH_8 8
#define SXE_TX_DESC_HOST_THRESH       1
#define SXE_TX_DESC_WRITEBACK_THRESH  32
#define SXE_MAX_TXRX_DESC_POLL        10

enum sxe_ring_state {
	SXE_RX_3K_BUFFER,
	SXE_RX_BUILD_SKB_ENABLED,
	SXE_RX_LRO_ENABLED,
	SXE_TX_FNAV_INIT_DONE,
	SXE_TX_XPS_INIT_DONE,
	SXE_TX_DETECT_HANG,
	SXE_HANG_CHECK_ARMED,
	SXE_TX_XDP_RING,
	SXE_TX_DISABLED,
};

#define SXE_TX_HANG_CHECK_ACTIVE(ring) \
	set_bit(SXE_TX_DETECT_HANG, &(ring)->state)
#define SXE_TX_HANG_CHECK_COMPLETE(ring) \
	clear_bit(SXE_TX_DETECT_HANG, &(ring)->state)
#define SXE_DETECT_TX_HANG_NEED(ring) \
	test_bit(SXE_TX_DETECT_HANG, &(ring)->state)

struct sxe_ring_stats {
	u64 packets;           
	u64 bytes;             
};

struct sxe_tx_ring_stats {
	u64 restart_queue;     
	u64 tx_busy;           
	u64 tx_done_old;       
};

struct sxe_rx_ring_stats {
	u64 lro_count;				
	u64 lro_flush;				
	u64 non_eop_descs;			
	u64 alloc_rx_page;			
	u64 alloc_rx_page_failed;	
	u64 alloc_rx_buff_failed;	
	u64 csum_err;
};

struct sxe_ring_desc {
	void *base_addr;     
	u8 __iomem *tail;    
	dma_addr_t dma;      
};

struct sxe_ring {
	struct sxe_ring *next;         
	struct sxe_irq_data *irq_data; 
	struct net_device *netdev;     
	struct device *dev;	       

	u8  idx;                       
	u8  reg_idx;                   
	u8  tc_idx;                    
	u16 ring_idx;                  

	unsigned long state;           

	u16 next_to_use;	       
	u16 next_to_clean;    	       

	u16   depth;                   
	u32   size;                    
	struct sxe_ring_desc desc;     

	union {
		struct sxe_tx_buffer *tx_buffer_info;  
		struct sxe_rx_buffer *rx_buffer_info;  
	};

	union {
		u16 next_to_alloc;                 

		struct {
			u8 fnav_sample_rate;             
			u8 fnav_sample_count;                   
		};
	};

	unsigned long last_rx_timestamp;

	u16 rx_offset;                   

	struct bpf_prog *xdp_prog;
#ifndef HAVE_NO_XDP_BUFF_RXQ
	struct xdp_rxq_info xdp_rxq;       
#endif
	spinlock_t tx_lock;

#ifdef HAVE_AF_XDP_ZERO_COPY
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
	struct zero_copy_allocator zca;    
#endif
#ifndef HAVE_NETDEV_BPF_XSK_BUFF_POOL
	struct xdp_umem *xsk_pool;
#else
	struct xsk_buff_pool *xsk_pool;
#endif
	u16 rx_buf_len; 		   
#endif
	struct sxe_ring_stats stats;            
	struct u64_stats_sync syncp;            
	union {
		struct sxe_tx_ring_stats tx_stats;  
		struct sxe_rx_ring_stats rx_stats;  
	};
} ____cacheline_internodealigned_in_smp;

struct sxe_ring_context {
	u16 num;                                      
	u16 depth;                                    
	struct sxe_ring *ring[SXE_TXRX_RING_NUM_MAX] ____cacheline_aligned_in_smp; 
};

struct sxe_ring_feature {
	u16 rss_limit;           
	union {
		u16 tc_per_pool;     
		u16 ring_per_pool;   
		u16 ring_per_tc;     
		u16 rss;             
	};

	u16 fnav_limit;
	u16 fnav_num;
} ____cacheline_internodealigned_in_smp;

struct sxe_pool_feature {
	u16 total_num;           
	u16 pf_num_limit;        
	u16 pf_num_used;         
	u16 vf_num_used;         
} ____cacheline_internodealigned_in_smp;

union sxe_tx_data_desc {
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

struct sxe_tx_context_desc {
	__le32 vlan_macip_lens;
	__le32 sa_idx;
	__le32 type_tucmd_mlhl;
	__le32 mss_l4len_idx;
};

union sxe_rx_data_desc {
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
struct sxe_tx_buffer {
	union sxe_tx_data_desc *next_to_watch;
	unsigned long time_stamp;
	union {
		struct sk_buff *skb;
#ifdef HAVE_XDP_SUPPORT
		struct xdp_frame *xdpf;
#endif
	};
	u32 bytecount;        
	u16 gso_segs;         
	__be16 protocol;      
	DEFINE_DMA_UNMAP_ADDR(dma);
	DEFINE_DMA_UNMAP_LEN(len);
	u32 tx_features;
};

struct sxe_rx_buffer {
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
	struct sk_buff *skb;
	dma_addr_t dma;
#endif
	union {
		struct {
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
			struct sk_buff *skb;
			dma_addr_t dma;
#endif
			struct page *page;
			u32 page_offset;
			u16 pagecnt_bias;
		};
#ifdef HAVE_AF_XDP_ZERO_COPY
		struct {
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
			bool discard;
			struct xdp_buff *xdp;
#else
			void *addr;
			u64 handle;
#endif
		};
#endif
	};
};

u16 sxe_rss_num_get(struct sxe_adapter *adapter);

u16 sxe_rss_mask_get(struct sxe_adapter *adapter);

u16 sxe_pool_mask_get(struct sxe_adapter *adapter);

void sxe_ring_num_set(struct sxe_adapter *adapter);

void sxe_ring_reg_map(struct sxe_adapter *adapter);

void sxe_ring_feature_init(struct sxe_adapter *adapter);

void sxe_ring_stats_init(struct sxe_adapter *adapter);

static inline __le32 sxe_status_err_check(union sxe_rx_data_desc *rx_desc,
					const u32 stat_err_bits)
{
	return rx_desc->wb.upper.status_error & cpu_to_le32(stat_err_bits);
}

void sxe_tx_ring_init(struct sxe_adapter *adapter, u16 base,
				    u16 cnt, u16 ring_idx, u16 irq_idx);

void sxe_xdp_ring_init(struct sxe_adapter *adapter, u16 base,
				      u16 cnt, u16 ring_idx, u16 irq_idx);

void sxe_rx_ring_init(struct sxe_adapter *adapter, u16 base,
				     u16  cnt, u16 ring_idx, u16 irq_idx);
#endif 
