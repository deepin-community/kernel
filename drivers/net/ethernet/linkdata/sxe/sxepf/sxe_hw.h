
#ifndef __SXE_HW_H__
#define __SXE_HW_H__

#if defined (__KERNEL__) || defined (SXE_KERNEL_TEST)
#include <linux/types.h>
#include <linux/kernel.h>
#else
#include "sxe_types.h"
#include "sxe_compat_platform.h"
#include "sxe_compat_version.h"
#ifdef SXE_HOST_DRIVER
#include "sxe_drv_type.h"
#endif
#include <inttypes.h>
#endif

#include "sxe_regs.h"

#if defined (__KERNEL__) || defined (SXE_KERNEL_TEST)
#define SXE_PRIU64  "llu"
#define SXE_PRIX64  "llx"
#define SXE_PRID64  "lld"
#define SXE_RMB()     rmb()

#else
#define SXE_PRIU64  PRIu64
#define SXE_PRIX64  PRIx64
#define SXE_PRID64  PRId64
#define SXE_RMB()     rte_rmb()
#endif

struct sxe_hw;
struct sxe_filter_mac;
struct sxe_fc_info;

#define SXE_MAC_ADDR_LEN 6
#define SXE_QUEUE_STATS_MAP_REG_NUM 32

#define SXE_FC_DEFAULT_HIGH_WATER_MARK    0x80
#define SXE_FC_DEFAULT_LOW_WATER_MARK     0x40

#define  SXE_MC_ADDR_EXTRACT_MASK  (0xFFF)
#define  SXE_MC_ADDR_SHIFT         (5)    
#define  SXE_MC_ADDR_REG_MASK      (0x7F) 
#define  SXE_MC_ADDR_BIT_MASK      (0x1F) 

#define SXE_TXTS_POLL_CHECK		3
#define SXE_TXTS_POLL			5
#define SXE_TIME_TO_NS(ns, sec)	(((u64)(ns)) + (u64)(((u64)(sec)) * NSEC_PER_SEC))

enum sxe_strict_prio_type {
	PRIO_NONE = 0, 
	PRIO_GROUP,    
	PRIO_LINK      
};

enum sxe_mc_filter_type {
	SXE_MC_FILTER_TYPE0 = 0,  
	SXE_MC_FILTER_TYPE1,      
	SXE_MC_FILTER_TYPE2,      
	SXE_MC_FILTER_TYPE3       
};

#define SXE_POOLS_NUM_MAX 64
#define SXE_16_POOL 16
#define SXE_32_POOL 32
#define SXE_1_RING_PER_POOL 1
#define SXE_2_RING_PER_POOL 2
#define SXE_3_RING_PER_POOL 3
#define SXE_4_RING_PER_POOL 4

#define SXE_DCB_1_TC 1
#define SXE_DCB_4_TC 4
#define SXE_DCB_8_TC 8

#define SXE_8Q_PER_POOL_MASK   0x78
#define SXE_4Q_PER_POOL_MASK   0x7C
#define SXE_2Q_PER_POOL_MASK   0x7E

#define SXE_VF_NUM_16		16
#define SXE_VF_NUM_32		32

#define SXE_TX_DESC_EOP_MASK  0x01000000   
#define SXE_TX_DESC_RS_MASK   0x08000000   
#define SXE_TX_DESC_STAT_DD   0x00000001   
#define SXE_TX_DESC_CMD       (SXE_TX_DESC_EOP_MASK | SXE_TX_DESC_RS_MASK)
#define SXE_TX_DESC_TYPE_DATA 0x00300000   
#define SXE_TX_DESC_DEXT      0x20000000   
#define SXE_TX_DESC_IFCS      0x02000000   
#define SXE_TX_DESC_VLE       0x40000000 
#define SXE_TX_DESC_TSTAMP    0x00080000 
#define SXE_TX_DESC_FLAGS     (SXE_TX_DESC_TYPE_DATA | \
				SXE_TX_DESC_IFCS | \
				SXE_TX_DESC_DEXT| \
				SXE_TX_DESC_EOP_MASK)
#define SXE_TXD_DTYP_CTXT     0x00200000 
#define SXE_TXD_DCMD_TSE      0x80000000 
#define SXE_TXD_MAC_LINKSEC   0x00040000 
#define SXE_TXD_MAC_1588      0x00080000 
#define SXE_TX_DESC_PAYLEN_SHIFT     14
#define SXE_TX_OUTERIPCS_SHIFT	17 

#define SXE_TX_POPTS_IXSM   0x01
#define SXE_TX_POPTS_TXSM   0x02
#define SXE_TXD_POPTS_SHIFT 8  
#define SXE_TXD_POPTS_IXSM  (SXE_TX_POPTS_IXSM << SXE_TXD_POPTS_SHIFT)
#define SXE_TXD_POPTS_TXSM  (SXE_TX_POPTS_TXSM << SXE_TXD_POPTS_SHIFT)
#define SXE_TXD_POPTS_IPSEC (0x00000400)

#define SXE_TX_CTXTD_DTYP_CTXT      0x00200000 
#define SXE_TX_CTXTD_TUCMD_IPV6     0x00000000 
#define SXE_TX_CTXTD_TUCMD_IPV4     0x00000400 
#define SXE_TX_CTXTD_TUCMD_L4T_UDP  0x00000000 
#define SXE_TX_CTXTD_TUCMD_L4T_TCP  0x00000800 
#define SXE_TX_CTXTD_TUCMD_L4T_SCTP 0x00001000 
#define SXE_TX_CTXTD_TUCMD_L4T_RSV  0x00001800 
#define SXE_TX_CTXTD_TUCMD_IPSEC_TYPE_ESP   0x00002000 
#define SXE_TX_CTXTD_TUCMD_IPSEC_ENCRYPT_EN 0x00004000 

#define SXE_TX_CTXTD_L4LEN_SHIFT          8  
#define SXE_TX_CTXTD_MSS_SHIFT            16 
#define SXE_TX_CTXTD_MACLEN_SHIFT         9  
#define SXE_TX_CTXTD_VLAN_SHIFT           16
#define SXE_TX_CTXTD_VLAN_MASK            0xffff0000
#define SXE_TX_CTXTD_MACLEN_MASK          0x0000fE00
#define SXE_TX_CTXTD_OUTER_IPLEN_SHIFT    16 
#define SXE_TX_CTXTD_TUNNEL_LEN_SHIFT     24 

#define SXE_VLAN_TAG_SIZE     4

#define SXE_RSS_KEY_SIZE                (40)  
#define SXE_MAX_RSS_KEY_ENTRIES		(10)  
#define SXE_MAX_RETA_ENTRIES            (128) 

#define SXE_TIMINC_IV_NS_SHIFT  8
#define SXE_TIMINC_INCPD_SHIFT  24
#define SXE_TIMINC_SET(incpd, iv_ns, iv_sns)   \
	(((incpd) << SXE_TIMINC_INCPD_SHIFT) | \
	((iv_ns) << SXE_TIMINC_IV_NS_SHIFT) | (iv_sns))

#define PBA_STRATEGY_EQUAL       (0)    
#define PBA_STRATEGY_WEIGHTED    (1)	
#define SXE_PKG_BUF_NUM_MAX               (8)
#define SXE_HW_TXRX_RING_NUM_MAX 128
#define SXE_VMDQ_DCB_NUM_QUEUES  SXE_HW_TXRX_RING_NUM_MAX
#define SXE_RX_PKT_BUF_SIZE 			(512)

#define SXE_UC_ENTRY_NUM_MAX   128
#define SXE_HW_TX_NONE_MODE_Q_NUM 64

#define SXE_MBX_MSG_NUM    16
#define SXE_MBX_RETRY_INTERVAL   500
#define SXE_MBX_RETRY_COUNT      2000

#define SXE_VF_UC_ENTRY_NUM_MAX 10
#define SXE_VF_MC_ENTRY_NUM_MAX 30

#define SXE_UTA_ENTRY_NUM_MAX   128
#define SXE_MTA_ENTRY_NUM_MAX   128
#define SXE_HASH_UC_NUM_MAX   4096 

#define  SXE_MAC_ADDR_EXTRACT_MASK  (0xFFF) 
#define  SXE_MAC_ADDR_SHIFT         (5)     
#define  SXE_MAC_ADDR_REG_MASK      (0x7F)  
#define  SXE_MAC_ADDR_BIT_MASK      (0x1F)  

#define  SXE_VFT_TBL_SIZE          (128)   
#define  SXE_VLAN_ID_SHIFT         (5)     
#define  SXE_VLAN_ID_REG_MASK      (0x7F)  
#define  SXE_VLAN_ID_BIT_MASK      (0x1F)  

#define SXE_TX_PBSIZE_MAX    0x00028000 
#define SXE_TX_PKT_SIZE_MAX  0xA        
#define SXE_NODCB_TX_PKT_SIZE_MAX 0x14 
#define SXE_RING_ENABLE_WAIT_LOOP 10

#define VFTA_BLOCK_SIZE 		8
#define VF_BLOCK_BITS 			(32)
#define SXE_MAX_MAC_HDR_LEN		127
#define SXE_MAX_NETWORK_HDR_LEN		511
#define SXE_MAC_ADDR_LEN		6

#define SXE_FNAV_BUCKET_HASH_KEY    0x3DAD14E2
#define SXE_FNAV_SAMPLE_HASH_KEY 0x174D3614
#define SXE_SAMPLE_COMMON_HASH_KEY \
		(SXE_FNAV_BUCKET_HASH_KEY & SXE_FNAV_SAMPLE_HASH_KEY)

#define SXE_SAMPLE_HASH_MASK		0x7fff
#define SXE_SAMPLE_L4TYPE_MASK		0x3
#define SXE_SAMPLE_L4TYPE_UDP		0x1
#define SXE_SAMPLE_L4TYPE_TCP		0x2
#define SXE_SAMPLE_L4TYPE_SCTP		0x3
#define SXE_SAMPLE_L4TYPE_IPV6_MASK	0x4
#define SXE_SAMPLE_L4TYPE_TUNNEL_MASK	0x10
#define SXE_SAMPLE_FLOW_TYPE_MASK	0xF

#define SXE_SAMPLE_VM_POOL_MASK		0x7F
#define SXE_SAMPLE_VLAN_MASK		0xEFFF
#define SXE_SAMPLE_FLEX_BYTES_MASK	0xFFFF

#define SXE_FNAV_INIT_DONE_POLL               10
#define SXE_FNAV_DROP_QUEUE                   127

#define MAX_TRAFFIC_CLASS        8
#define DEF_TRAFFIC_CLASS        1

#define SXE_LINK_SPEED_UNKNOWN   0
#define SXE_LINK_SPEED_10_FULL   0x0002
#define SXE_LINK_SPEED_100_FULL  0x0008
#define SXE_LINK_SPEED_1GB_FULL  0x0020
#define SXE_LINK_SPEED_10GB_FULL 0x0080

typedef u32 sxe_link_speed;
#ifdef SXE_TEST
#define SXE_LINK_MBPS_SPEED_DEFAULT 1000
#else
#define SXE_LINK_MBPS_SPEED_DEFAULT 10000
#endif

#define SXE_LINK_MBPS_SPEED_MIN   (10)

enum sxe_rss_ip_version {
	SXE_RSS_IP_VER_4 = 4,
	SXE_RSS_IP_VER_6 = 6,
};

enum sxe_fnav_mode {
	SXE_FNAV_SAMPLE_MODE = 1,
	SXE_FNAV_SPECIFIC_MODE	= 2,
};

enum sxe_sample_type {
	SXE_SAMPLE_FLOW_TYPE_IPV4   = 0x0,
	SXE_SAMPLE_FLOW_TYPE_UDPV4  = 0x1,
	SXE_SAMPLE_FLOW_TYPE_TCPV4  = 0x2,
	SXE_SAMPLE_FLOW_TYPE_SCTPV4 = 0x3,
	SXE_SAMPLE_FLOW_TYPE_IPV6   = 0x4,
	SXE_SAMPLE_FLOW_TYPE_UDPV6  = 0x5,
	SXE_SAMPLE_FLOW_TYPE_TCPV6  = 0x6,
	SXE_SAMPLE_FLOW_TYPE_SCTPV6 = 0x7,
};

enum {
	SXE_DIAG_TEST_PASSED                = 0,
	SXE_DIAG_TEST_BLOCKED               = 1,
	SXE_DIAG_STATS_REG_TEST_ERR         = 2,
	SXE_DIAG_REG_PATTERN_TEST_ERR       = 3,
	SXE_DIAG_CHECK_REG_TEST_ERR         = 4,
	SXE_DIAG_DISABLE_IRQ_TEST_ERR       = 5,
	SXE_DIAG_ENABLE_IRQ_TEST_ERR        = 6,
	SXE_DIAG_DISABLE_OTHER_IRQ_TEST_ERR = 7,
	SXE_DIAG_TX_RING_CONFIGURE_ERR      = 8,
	SXE_DIAG_RX_RING_CONFIGURE_ERR      = 9,
	SXE_DIAG_ALLOC_SKB_ERR              = 10,
	SXE_DIAG_LOOPBACK_SEND_TEST_ERR     = 11,
	SXE_DIAG_LOOPBACK_RECV_TEST_ERR     = 12,
};

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

#define SXE_RXDADV_STAT_DD            SXE_RXD_STAT_DD  
#define SXE_RXDADV_STAT_EOP           SXE_RXD_STAT_EOP 
#define SXE_RXDADV_STAT_FLM           SXE_RXD_STAT_FLM 
#define SXE_RXDADV_STAT_VP            SXE_RXD_STAT_VP  
#define SXE_RXDADV_STAT_MASK          0x000fffff 
#define SXE_RXDADV_STAT_TS		0x00010000 
#define SXE_RXDADV_STAT_SECP		0x00020000 

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

struct sxe_mac_stats {
	u64 crcerrs;           
	u64 errbc;             
	u64 rlec;              
	u64 prc64;             
	u64 prc127;            
	u64 prc255;            
	u64 prc511;            
	u64 prc1023;           
	u64 prc1522;           
	u64 gprc;              
	u64 bprc;              
	u64 mprc;              
	u64 gptc;              
	u64 gorc;              
	u64 gotc;              
	u64 ruc;               
	u64 rfc;               
	u64 roc;               
	u64 rjc;               
	u64 tor;               
	u64 tpr;               
	u64 tpt;               
	u64 ptc64;             
	u64 ptc127;            
	u64 ptc255;            
	u64 ptc511;            
	u64 ptc1023;           
	u64 ptc1522;           
	u64 mptc;              
	u64 bptc;              
	u64 qprc[16];          
	u64 qptc[16];          
	u64 qbrc[16];          
	u64 qbtc[16];          
	u64 qprdc[16];         
	u64 dburxtcin[8];      
	u64 dburxtcout[8];     
	u64 dburxgdreecnt[8];  
	u64 dburxdrofpcnt[8];  
	u64 dbutxtcin[8];      
	u64 dbutxtcout[8];     
	u64 rxdgpc;            
	u64 rxdgbc;            
	u64 rxddpc;            
	u64 rxddbc;            
	u64 rxtpcing;          
	u64 rxtpceng;          
	u64 rxlpbkpc;          
	u64 rxlpbkbc;          
	u64 rxdlpbkpc;         
	u64 rxdlpbkbc;         
	u64 prddc;             
	u64 txdgpc;            
	u64 txdgbc;            
	u64 txswerr;           
	u64 txswitch;          
	u64 txrepeat;          
	u64 txdescerr;         

	u64 fnavadd;           
	u64 fnavrmv;           
	u64 fnavadderr;        
	u64 fnavrmverr;        
	u64 fnavmatch;         
	u64 fnavmiss;          
	u64 hw_rx_no_dma_resources; 
	u64 prcpf[8];          
	u64 pfct[8];           
	u64 mpc[8];            

	u64 total_tx_pause;    
	u64 total_gptc;        
	u64 total_gotc;        
};

#if defined SXE_DPDK_L4_FEATURES && defined SXE_DPDK_FILTER_CTRL
enum sxe_fivetuple_protocol {
	SXE_FILTER_PROTOCOL_TCP = 0,
	SXE_FILTER_PROTOCOL_UDP,
	SXE_FILTER_PROTOCOL_SCTP,
	SXE_FILTER_PROTOCOL_NONE,
};

struct sxe_fivetuple_filter_info {
	u32 src_ip;
	u32 dst_ip;
	u16 src_port;
	u16 dst_port;
	enum sxe_fivetuple_protocol protocol;
	u8 priority;
	u8 src_ip_mask:1,
	   dst_ip_mask:1,
	   src_port_mask:1,
	   dst_port_mask:1,
	   proto_mask:1;
};

struct sxe_fivetuple_node_info {
	u16 index;  
	u16 queue;  
	struct sxe_fivetuple_filter_info filter_info;
};
#endif

union sxe_fnav_rule_info {
	struct {
		u8     vm_pool;
		u8     flow_type;
		__be16 vlan_id;
		__be32 dst_ip[4];
		__be32 src_ip[4];
		__be16 src_port;
		__be16 dst_port;
		__be16 flex_bytes;
		__be16 bkt_hash;
	} ntuple;
	__be32 fast_access[11];
};

union sxe_sample_hash_dword {
	struct {
		u8 vm_pool;
		u8 flow_type;
		__be16 vlan_id;
	} formatted;
	__be32 ip;
	struct {
		__be16 src;
		__be16 dst;
	} port;
	__be16 flex_bytes;
	__be32 dword;
};

void sxe_hw_ops_init(struct sxe_hw *hw);


struct sxe_reg_info {
	u32 addr;        
	u32 count;       
	u32 stride;      
	const s8 *name;  
};

struct sxe_setup_operations {
	s32  (*reset)(struct sxe_hw *);
	void (*pf_rst_done_set)(struct sxe_hw *);
	void (*no_snoop_disable)(struct sxe_hw *);
	u32  (*reg_read)(struct sxe_hw *, u32);
	void (*reg_write)(struct sxe_hw *, u32, u32);
	void (*regs_dump)(struct sxe_hw *);
	void (*regs_flush)(struct sxe_hw *);
	s32  (*regs_test)(struct sxe_hw *);
};

struct sxe_hw_setup {
	const struct sxe_setup_operations *ops;
};

struct sxe_irq_operations {
	u32  (*pending_irq_read_clear)(struct sxe_hw *hw);
	void (*pending_irq_write_clear)(struct sxe_hw * hw, u32 value);
	void (*irq_general_reg_set)(struct sxe_hw *hw, u32 value);
	u32  (*irq_general_reg_get)(struct sxe_hw *hw);
	void (*ring_irq_auto_disable)(struct sxe_hw *hw, bool is_misx);
	void (*set_eitrsel)(struct sxe_hw *hw, u32 value);
	void (*ring_irq_interval_set)(struct sxe_hw *hw, u16 irq_idx, u32 interval);
	void (*event_irq_interval_set)(struct sxe_hw * hw, u16 irq_idx, u32 value);
	void (*event_irq_auto_clear_set)(struct sxe_hw *hw, u32 value);
	void (*ring_irq_map)(struct sxe_hw *hw, bool is_tx,
                                u16 reg_idx, u16 irq_idx);
	void (*event_irq_map)(struct sxe_hw *hw, u8 offset, u16 irq_idx);
	void (*ring_irq_enable)(struct sxe_hw * hw, u64 qmask);
	u32  (*irq_cause_get)(struct sxe_hw * hw);
	void (*event_irq_trigger)(struct sxe_hw * hw);
	void (*ring_irq_trigger)(struct sxe_hw *hw, u64 eics);
	void (*specific_irq_disable)(struct sxe_hw *hw, u32 value);
	void (*specific_irq_enable)(struct sxe_hw *hw, u32 value);
	void (*all_irq_disable)(struct sxe_hw *hw);
	void (*spp_configure)(struct sxe_hw *hw, u32 value);
	s32  (*irq_test)(struct sxe_hw *hw, u32 *icr, bool shared);
};

struct sxe_irq_info {
	const struct sxe_irq_operations *ops;
};

struct sxe_mac_operations {
	bool (*link_up_1g_check)(struct sxe_hw *);
	bool (*link_state_is_up)(struct sxe_hw *);
	u32  (*link_speed_get)(struct sxe_hw *);
	void (*link_speed_set)(struct sxe_hw *, u32 speed);
	void (*pad_enable)(struct sxe_hw *);
	s32  (*fc_enable)(struct sxe_hw *);
	void (*crc_configure)(struct sxe_hw *);
	void (*loopback_switch)(struct sxe_hw *, bool);
	void (*txrx_enable)(struct sxe_hw *hw);
	void (*max_frame_set)(struct sxe_hw *, u32);
	u32  (*max_frame_get)(struct sxe_hw *);
	void (*fc_autoneg_localcap_set)(struct sxe_hw *);
	void (*fc_tc_high_water_mark_set)(struct sxe_hw *, u8, u32);
	void (*fc_tc_low_water_mark_set)(struct sxe_hw *, u8, u32);
	void (*fc_param_init)(struct sxe_hw *);
	enum sxe_fc_mode (*fc_current_mode_get)(struct sxe_hw *);
	enum sxe_fc_mode (*fc_requested_mode_get)(struct sxe_hw *);
	void (*fc_requested_mode_set)(struct sxe_hw *, enum sxe_fc_mode);
	bool (*is_fc_autoneg_disabled)(struct sxe_hw *);
	void (*fc_autoneg_disable_set)(struct sxe_hw *, bool);
};

#define SXE_FLAGS_DOUBLE_RESET_REQUIRED	0x01

struct sxe_mac_info {
	const struct sxe_mac_operations	*ops;
	u8   flags;
	bool set_lben;
	bool auto_restart;
};

struct sxe_filter_mac_operations {
	u32 (*rx_mode_get)(struct sxe_hw *);
	void (*rx_mode_set)(struct sxe_hw *, u32);
	u32 (*pool_rx_mode_get)(struct sxe_hw *, u16);
	void (*pool_rx_mode_set)(struct sxe_hw *, u32, u16);
	void (*rx_lro_enable) (struct sxe_hw *, bool);
	void (*rx_udp_frag_checksum_disable) (struct sxe_hw *);
	s32  (*uc_addr_add)(struct sxe_hw *, u32, u8 *, u32);
	s32  (*uc_addr_del)(struct sxe_hw *, u32);
	void (*uc_addr_clear)(struct sxe_hw *);
	void (*mta_hash_table_set)(struct sxe_hw *hw, u8 index, u32 value);
	void (*mta_hash_table_update)(struct sxe_hw *hw, u8 reg_idx, u8 bit_idx);
	void (*fc_mac_addr_set)(struct sxe_hw *hw, u8 *mac_addr);

	void (*mc_filter_enable)(struct sxe_hw *);

	void (*mc_filter_disable)(struct sxe_hw *hw);

	void (*rx_nfs_filter_disable)(struct sxe_hw *);
	void (*ethertype_filter_set)(struct sxe_hw *, u8, u32);

	void (*vt_ctrl_configure)(struct sxe_hw *hw, u8 num_vfs);

#ifdef SXE_WOL_CONFIGURE
	void (*wol_mode_set)(struct sxe_hw *hw, u32 wol_status);
	void (*wol_mode_clean)(struct sxe_hw *hw);
	void (*wol_status_set)(struct sxe_hw *hw);
#endif

	void (*vt_disable)(struct sxe_hw *hw);

	s32 (*uc_addr_pool_enable)(struct sxe_hw *hw, u8 rar_idx, u8 pool_idx);
};

struct sxe_filter_mac {
	const struct sxe_filter_mac_operations *ops;
};

struct sxe_filter_vlan_operations {
	u32 (*pool_filter_read)(struct sxe_hw *, u16);
	void (*pool_filter_write)(struct sxe_hw *, u16, u32);
	u32 (*pool_filter_bitmap_read)(struct sxe_hw *, u16);
	void (*pool_filter_bitmap_write)(struct sxe_hw *, u16, u32);
	void (*filter_array_write)(struct sxe_hw *, u16, u32);
	u32  (*filter_array_read)(struct sxe_hw *, u16);
	void (*filter_array_clear)(struct sxe_hw *);
	void (*filter_switch)(struct sxe_hw *,bool);
	void (*untagged_pkts_rcv_switch)(struct sxe_hw *, u32, bool);
	s32  (*filter_configure)(struct sxe_hw *, u32, u32, bool, bool);
};

struct sxe_filter_vlan {
	const struct sxe_filter_vlan_operations *ops;
};

struct sxe_filter_info {
	struct sxe_filter_mac  mac;
	struct sxe_filter_vlan vlan;
};

struct sxe_dbu_operations {
	void (*rx_pkt_buf_size_configure)(struct sxe_hw *, u8, u32, u16);
	void (*rx_pkt_buf_switch)(struct sxe_hw *, bool);
	void (*rx_multi_ring_configure)(struct sxe_hw *, u8, bool, bool);
	void (*rss_key_set_all)(struct sxe_hw *, u32 *);
	void (*rss_redir_tbl_set_all)(struct sxe_hw *, u8 *);
	void (*rx_cap_switch_on)(struct sxe_hw *);
	void (*rss_hash_pkt_type_set)(struct sxe_hw *, u32);
	void (*rss_hash_pkt_type_update)(struct sxe_hw *, u32);
	void (*rss_rings_used_set)(struct sxe_hw *, u32, u16, u16);
	void (*lro_ack_switch)(struct sxe_hw *, bool);
	void (*vf_rx_switch)(struct sxe_hw *, u32, u32, bool);

	s32  (*fnav_mode_init)(struct sxe_hw *, u32, u32);
	s32  (*fnav_specific_rule_mask_set)(struct sxe_hw *,
						union sxe_fnav_rule_info *);
	s32  (*fnav_specific_rule_add)(struct sxe_hw *,
						union sxe_fnav_rule_info *,
						u16, u8);
	s32  (*fnav_specific_rule_del)(struct sxe_hw *,
					  union sxe_fnav_rule_info *, u16);
	s32  (*fnav_sample_hash_cmd_get)(struct sxe_hw *,
						u8, u32, u8, u64 *);
	void (*fnav_sample_stats_reinit)(struct sxe_hw *hw);
	void (*fnav_sample_hash_set)(struct sxe_hw *hw, u64 hash);
	s32  (*fnav_single_sample_rule_del)(struct sxe_hw *,u32);

	void (*ptp_init)(struct sxe_hw *);
	void (*ptp_freq_adjust)(struct sxe_hw *, u32);
	void (*ptp_systime_init)(struct sxe_hw *);
	u64  (*ptp_systime_get)(struct sxe_hw *);
	void (*ptp_tx_timestamp_get)(struct sxe_hw *, u32 *ts_sec, u32 *ts_ns);
	void (*ptp_timestamp_mode_set)(struct sxe_hw *, bool, u32, u32);
	void (*ptp_rx_timestamp_clear)(struct sxe_hw *);
	u64  (*ptp_rx_timestamp_get)(struct sxe_hw *);
	bool (*ptp_is_rx_timestamp_valid)(struct sxe_hw *);
	void (*ptp_timestamp_enable)(struct sxe_hw *);

	void (*tx_pkt_buf_switch)(struct sxe_hw *, bool);

	void (*dcb_tc_rss_configure)(struct sxe_hw *hw, u16 rss_i);

	void (*tx_pkt_buf_size_configure)(struct sxe_hw *, u8);

	void (*rx_cap_switch_off)(struct sxe_hw *);
	u32  (*rx_pkt_buf_size_get)(struct sxe_hw *, u8);
	void (*rx_func_switch_on)(struct sxe_hw *hw);

	void (*tx_ring_disable)(struct sxe_hw *, u8, unsigned long);
	void (*rx_ring_disable)(struct sxe_hw *, u8, unsigned long);

	u32  (*tx_dbu_fc_status_get)(struct sxe_hw *hw);
};

struct sxe_dbu_info {
	const struct sxe_dbu_operations	*ops;
};


struct sxe_dma_operations {
	void (*rx_dma_ctrl_init)(struct sxe_hw *);
	void (*rx_ring_disable)(struct sxe_hw *, u8);
	void (*rx_ring_switch)(struct sxe_hw *, u8, bool);
	void (*rx_ring_switch_not_polling)(struct sxe_hw *, u8, bool);
	void (*rx_ring_desc_configure)(struct sxe_hw *, u32, u64, u8);
	void (*rx_desc_thresh_set)(struct sxe_hw *, u8);
	void (*rx_rcv_ctl_configure)(struct sxe_hw *, u8, u32, u32);
	void (*rx_lro_ctl_configure)(struct sxe_hw *, u8, u32);
	u32  (*rx_desc_ctrl_get)(struct sxe_hw *, u8);
	void (*rx_dma_lro_ctl_set)(struct sxe_hw *);
	void (*rx_drop_switch)(struct sxe_hw *, u8, bool);
	void (*rx_tph_update)(struct sxe_hw *hw, u8 ring_idx, u8 cpu);

	void (*tx_enable)(struct sxe_hw *);
	void (*tx_multi_ring_configure)(struct sxe_hw *, u8, u16, bool, u16);
	void (*tx_ring_desc_configure)(struct sxe_hw *, u32, u64, u8);
	void (*tx_desc_thresh_set)(struct sxe_hw *, u8, u32, u32, u32);
	void (*tx_ring_switch)(struct sxe_hw *, u8, bool);
	void (*tx_ring_switch_not_polling)(struct sxe_hw *, u8, bool);
	void (*tx_pkt_buf_thresh_configure)(struct sxe_hw *, u8, bool);
	u32  (*tx_desc_ctrl_get)(struct sxe_hw *, u8);
	void (*tx_ring_info_get)(struct sxe_hw *, u8, u32 *, u32 *);
	void (*tx_desc_wb_thresh_clear)(struct sxe_hw *, u8);

	void (*vlan_tag_strip_switch)(struct sxe_hw *, u16, bool);
	void (*tx_vlan_tag_set)(struct sxe_hw *, u16, u16, u32);
	void (*tx_vlan_tag_clear)(struct sxe_hw *, u32);
	void (*tx_tph_update)(struct sxe_hw *hw, u8 ring_idx, u8 cpu);

	void (*tph_switch)(struct sxe_hw *hw, bool is_enable);

	void  (*dcb_rx_bw_alloc_configure)(struct sxe_hw *hw,
				      u16 *refill,
				      u16 *max,
				      u8 *bwg_id,
				      u8 *prio_type,
				      u8 *prio_tc,
				      u8 max_priority);
	void  (*dcb_tx_desc_bw_alloc_configure)(struct sxe_hw *hw,
					   u16 *refill,
					   u16 *max,
					   u8 *bwg_id,
					   u8 *prio_type);
	void  (*dcb_tx_data_bw_alloc_configure)(struct sxe_hw *hw,
					   u16 *refill,
					   u16 *max,
					   u8 *bwg_id,
					   u8 *prio_type,
					   u8 *prio_tc,
					   u8 max_priority);
	void  (*dcb_pfc_configure)(struct sxe_hw *hw, u8 pfc_en, u8 *prio_tc,
				  u8 max_priority);
	void  (*dcb_tc_stats_configure)(struct sxe_hw *hw);
	void (*dcb_rx_up_tc_map_set)(struct sxe_hw *hw, u8 tc);
	void (*dcb_rx_up_tc_map_get)(struct sxe_hw *hw, u8 *map);
	void (*dcb_rate_limiter_clear)(struct sxe_hw *hw, u8 ring_max);

	void (*vt_pool_loopback_switch)(struct sxe_hw *hw, bool is_enable);
	u32 (*rx_pool_get)(struct sxe_hw *hw, u8 reg_idx);
	u32 (*tx_pool_get)(struct sxe_hw *hw, u8 reg_idx);
	void (*tx_pool_set)(struct sxe_hw *hw, u8 reg_idx, u32 bitmap);
	void (*rx_pool_set)(struct sxe_hw *hw, u8 reg_idx, u32 bitmap);

	void (*vf_tx_desc_addr_clear)(struct sxe_hw *hw, u8 vf_idx, u8 ring_per_pool);
	void (*pool_mac_anti_spoof_set)(struct sxe_hw *hw, u8 vf_idx, bool status);
	void (*pool_vlan_anti_spoof_set)(struct sxe_hw *hw, u8 vf_idx, bool status);
	void (*spoof_count_enable)(struct sxe_hw *hw, u8 reg_idx, u8 bit_index);
	void (*pool_rx_ring_drop_enable)(struct sxe_hw *hw, u8 vf_idx,
					u16 pf_vlan, u8 ring_per_pool);

	void (*max_dcb_memory_window_set)(struct sxe_hw *hw, u32 value);
	void (*dcb_tx_ring_rate_factor_set)(struct sxe_hw *hw, u32 ring_idx, u32 rate);

	void (*vf_tx_ring_disable)(struct sxe_hw *hw, u8 ring_per_pool, u8 vf_idx);
	void (*all_ring_disable)(struct sxe_hw *hw, u32 ring_max);
	void (*tx_ring_tail_init)(struct sxe_hw *hw, u8 reg_idx);
};

struct sxe_dma_info {
	const struct sxe_dma_operations	*ops;
};

struct sxe_sec_operations {
	void (*ipsec_rx_ip_store)(struct sxe_hw *hw, __be32 *ip_addr, u8 ip_len, u8 ip_idx);
	void (*ipsec_rx_spi_store)(struct sxe_hw *hw, __be32 spi, u8 ip_idx, u16 idx);
	void (*ipsec_rx_key_store)(struct sxe_hw *hw, u32 *key,  u8 key_len, u32 salt, u32 mode, u16 idx);
	void (*ipsec_tx_key_store)(struct sxe_hw *hw, u32 *key,  u8 key_len, u32 salt, u16 idx);
	void (*ipsec_sec_data_stop)(struct sxe_hw *hw, bool is_linkup);
	void (*ipsec_engine_start)(struct sxe_hw *hw, bool is_linkup);
	void (*ipsec_engine_stop)(struct sxe_hw *hw, bool is_linkup);
	bool (*ipsec_offload_is_disable)(struct sxe_hw *hw);
	void (*ipsec_sa_disable)(struct sxe_hw *hw);
};

struct sxe_sec_info {
	const struct sxe_sec_operations *ops;
};

struct sxe_stat_operations {
	void (*stats_clear)(struct sxe_hw *);
	void (*stats_get)(struct sxe_hw *, struct sxe_mac_stats *);

	u32 (*tx_packets_num_get)(struct sxe_hw *hw);
	u32 (*unsecurity_packets_num_get)(struct sxe_hw *hw);
	u32  (*mac_stats_dump)(struct sxe_hw *, u32 *, u32);
	u32  (*tx_dbu_to_mac_stats)(struct sxe_hw *hw);
};

struct sxe_stat_info {
	const struct sxe_stat_operations	*ops;
};

struct sxe_mbx_operations {
	void (*init)(struct sxe_hw *hw);

	s32 (*msg_send)(struct sxe_hw *hw, u32 *msg, u16 len, u16 index);
	s32 (*msg_rcv)(struct sxe_hw *hw, u32 *msg, u16 len, u16 index);

	bool (*req_check)(struct sxe_hw *hw, u8 vf_idx);
	bool (*ack_check)(struct sxe_hw *hw, u8 vf_idx);
	bool (*rst_check)(struct sxe_hw *hw, u8 vf_idx);

	void (*mbx_mem_clear)(struct sxe_hw *hw, u8 vf_idx);
};

struct sxe_mbx_stats {
	u32 send_msgs; 
	u32 rcv_msgs;  

	u32 reqs;      
	u32 acks;      
	u32 rsts;      
};

struct sxe_mbx_info {
	const struct sxe_mbx_operations *ops; 
	struct sxe_mbx_stats stats; 
	u32 retry;    
	u32 interval; 
	u32 msg_len; 
};

struct sxe_pcie_operations {
	void (*vt_mode_set)(struct sxe_hw *hw, u32 value);
};

struct sxe_pcie_info {
	const struct sxe_pcie_operations *ops; 
};

enum sxe_hw_state {
	SXE_HW_STOP,
	SXE_HW_FAULT,
};

enum sxe_fc_mode {
	SXE_FC_NONE = 0,
	SXE_FC_RX_PAUSE,
	SXE_FC_TX_PAUSE,
	SXE_FC_FULL,
	SXE_FC_DEFAULT,
};

struct sxe_fc_info {
	u32 high_water[MAX_TRAFFIC_CLASS]; 
	u32 low_water[MAX_TRAFFIC_CLASS]; 
	u16 pause_time; 
	bool strict_ieee; 
	bool disable_fc_autoneg; 
	u16 send_xon; 
	enum sxe_fc_mode current_mode; 
	enum sxe_fc_mode requested_mode; 
};

struct sxe_fc_nego_mode {
	u32 adv_sym; 
	u32 adv_asm; 
	u32 lp_sym;  
	u32 lp_asm;  

};

struct sxe_hdc_operations {
	s32 (*pf_lock_get)(struct sxe_hw *, u32);
	void (*pf_lock_release)(struct sxe_hw *, u32);
	bool (*is_fw_over_set)(struct sxe_hw *);
	u32 (*fw_ack_header_rcv)(struct sxe_hw *);
	void (*packet_send_done)(struct sxe_hw *);
	void (*packet_header_send)(struct sxe_hw *, u32);
	void (*packet_data_dword_send)(struct sxe_hw *, u16, u32);
	u32  (*packet_data_dword_rcv)(struct sxe_hw *, u16);
	u32 (*fw_status_get)(struct sxe_hw *);
	void (*drv_status_set)(struct sxe_hw *, u32);
	u32 (*irq_event_get)(struct sxe_hw *);
	void (*irq_event_clear)(struct sxe_hw *, u32);
	void (*fw_ov_clear)(struct sxe_hw *);
	u32 (*channel_state_get)(struct sxe_hw *);
	void (*resource_clean)(struct sxe_hw *);
};

struct sxe_hdc_info {
	u32 pf_lock_val;
	const struct sxe_hdc_operations	*ops;
};

struct sxe_phy_operations {
	s32 (*reg_write)(struct sxe_hw *hw, s32 prtad, u32 reg_addr,
				u32 device_type, u16 phy_data);
	s32 (*reg_read)(struct sxe_hw *hw, s32 prtad, u32 reg_addr,
				u32 device_type, u16 *phy_data);
	s32 (*identifier_get)(struct sxe_hw *hw, u32 prtad, u32 *id);
	s32 (*link_cap_get)(struct sxe_hw *hw, u32 prtad, u32 *speed);
	s32 (*reset)(struct sxe_hw *hw, u32 prtad);
};

struct sxe_phy_reg_info {
	const struct sxe_phy_operations	*ops;
};

struct sxe_hw {
	u8 __iomem *reg_base_addr;            

	void *adapter;
	void *priv;
	unsigned long state;   
	void (*fault_handle)(void *priv);
	u32 (*reg_read)(const volatile void *reg);
	void (*reg_write)(u32 value, volatile void *reg);

	struct sxe_hw_setup  setup;           
	struct sxe_irq_info  irq;             
	struct sxe_mac_info  mac;             
	struct sxe_filter_info filter;        
	struct sxe_dbu_info  dbu;             
	struct sxe_dma_info  dma;             
	struct sxe_sec_info  sec;             
	struct sxe_stat_info stat;            
	struct sxe_fc_info   fc;

	struct sxe_mbx_info mbx;              
	struct sxe_pcie_info pcie;            
	struct sxe_hdc_info  hdc;             
	struct sxe_phy_reg_info phy;          
};

u16 sxe_mac_reg_num_get(void);

void sxe_hw_fault_handle(struct sxe_hw *hw);

bool sxe_device_supports_autoneg_fc(struct sxe_hw *hw);

void sxe_hw_ops_init(struct sxe_hw *hw);

u32 sxe_hw_rss_key_get_by_idx(struct sxe_hw *hw, u8 reg_idx);

bool sxe_hw_is_rss_enabled(struct sxe_hw *hw);

u32 sxe_hw_rss_field_get(struct sxe_hw *hw);

static inline bool sxe_is_hw_fault(struct sxe_hw *hw)
{
	return test_bit(SXE_HW_FAULT, &hw->state);
}

static inline void sxe_hw_fault_handle_init(struct sxe_hw *hw,
			void (*handle)(void *), void *priv)
{
	hw->priv = priv;
	hw->fault_handle = handle;

	return;
}

static inline void sxe_hw_reg_handle_init(struct sxe_hw *hw,
		u32 (*read)(const volatile void *),
		void (*write)(u32, volatile void *))
{
	hw->reg_read  = read;
	hw->reg_write = write;

	return;
}

#ifdef SXE_DPDK 

void sxe_hw_crc_strip_config(struct sxe_hw *hw, bool keep_crc);

void sxe_hw_stats_seq_clean(struct sxe_hw *hw, struct sxe_mac_stats *stats);

void sxe_hw_hdc_drv_status_set(struct sxe_hw *hw, u32 value);

s32 sxe_hw_nic_reset(struct sxe_hw *hw);

u16 sxe_hw_fc_pause_time_get(struct sxe_hw *hw);

void sxe_hw_fc_pause_time_set(struct sxe_hw *hw, u16 pause_time);

void sxe_fc_autoneg_localcap_set(struct sxe_hw *hw);

u32 sxe_hw_fc_tc_high_water_mark_get(struct sxe_hw *hw, u8 tc_idx);

u32 sxe_hw_fc_tc_low_water_mark_get(struct sxe_hw *hw, u8 tc_idx);

u16 sxe_hw_fc_send_xon_get(struct sxe_hw *hw);

void sxe_hw_fc_send_xon_set(struct sxe_hw *hw, u16 send_xon);

u32 sxe_hw_rx_mode_get(struct sxe_hw *hw);

void sxe_hw_rx_mode_set(struct sxe_hw *hw, u32 filter_ctrl);

void sxe_hw_specific_irq_enable(struct sxe_hw *hw, u32 value);

void sxe_hw_specific_irq_disable(struct sxe_hw *hw, u32 value);

void sxe_hw_irq_general_reg_set(struct sxe_hw *hw, u32 value);

u32 sxe_hw_irq_general_reg_get(struct sxe_hw *hw);

void sxe_hw_event_irq_map(struct sxe_hw *hw, u8 offset, u16 irq_idx);

void sxe_hw_ring_irq_map(struct sxe_hw *hw, bool is_tx,
						u16 reg_idx, u16 irq_idx);

void sxe_hw_ring_irq_interval_set(struct sxe_hw *hw,
						u16 irq_idx, u32 interval);

void sxe_hw_event_irq_auto_clear_set(struct sxe_hw *hw, u32 value);

void sxe_hw_all_irq_disable(struct sxe_hw *hw);

void sxe_hw_ring_irq_auto_disable(struct sxe_hw *hw,
					bool is_msix);

u32 sxe_hw_irq_cause_get(struct sxe_hw *hw);

void sxe_hw_pending_irq_write_clear(struct sxe_hw *hw, u32 value);

u32 sxe_hw_ring_irq_switch_get(struct sxe_hw *hw, u8 idx);

void sxe_hw_ring_irq_switch_set(struct sxe_hw *hw, u8 idx, u32 value);

s32 sxe_hw_uc_addr_add(struct sxe_hw *hw, u32 rar_idx,
					u8 *addr, u32 pool_idx);

s32 sxe_hw_uc_addr_del(struct sxe_hw *hw, u32 index);

u32 sxe_hw_uta_hash_table_get(struct sxe_hw *hw, u8 reg_idx);

void sxe_hw_uta_hash_table_set(struct sxe_hw *hw,
				u8 reg_idx, u32 value);

void sxe_hw_mta_hash_table_set(struct sxe_hw *hw,
						u8 index, u32 value);

void sxe_hw_mc_filter_enable(struct sxe_hw *hw);

void sxe_hw_vlan_filter_array_write(struct sxe_hw *hw,
					u16 reg_index, u32 value);

u32 sxe_hw_vlan_filter_array_read(struct sxe_hw *hw, u16 reg_index);

void sxe_hw_vlan_filter_switch(struct sxe_hw *hw, bool is_enable);

u32 sxe_hw_vlan_type_get(struct sxe_hw *hw);

void sxe_hw_vlan_type_set(struct sxe_hw *hw, u32 value);

void sxe_hw_vlan_ext_vet_write(struct sxe_hw *hw, u32 value);

void sxe_hw_vlan_tag_strip_switch(struct sxe_hw *hw,
					u16 reg_index, bool is_enable);

void sxe_hw_txctl_vlan_type_set(struct sxe_hw *hw, u32 value);

u32 sxe_hw_txctl_vlan_type_get(struct sxe_hw *hw);

u32 sxe_hw_ext_vlan_get(struct sxe_hw *hw);

void sxe_hw_ext_vlan_set(struct sxe_hw *hw, u32 value);

void sxe_hw_pf_rst_done_set(struct sxe_hw *hw);

u32 sxe_hw_all_regs_group_num_get(void);

void sxe_hw_all_regs_group_read(struct sxe_hw *hw, u32 *data);

s32 sxe_hw_fc_enable(struct sxe_hw *hw);

bool sxe_hw_is_fc_autoneg_disabled(struct sxe_hw *hw);

void sxe_hw_fc_status_get(struct sxe_hw *hw,
					bool *rx_pause_on, bool *tx_pause_on);

void sxe_hw_fc_requested_mode_set(struct sxe_hw *hw,
						enum sxe_fc_mode mode);

void sxe_hw_fc_tc_high_water_mark_set(struct sxe_hw *hw,
							u8 tc_idx, u32 mark);

void sxe_hw_fc_tc_low_water_mark_set(struct sxe_hw *hw,
							u8 tc_idx, u32 mark);

void sxe_hw_fc_autoneg_disable_set(struct sxe_hw *hw,
							bool is_disabled);

u32 sxe_hw_rx_pkt_buf_size_get(struct sxe_hw *hw, u8 pb);

void sxe_hw_ptp_init(struct sxe_hw *hw);

void sxe_hw_ptp_timestamp_mode_set(struct sxe_hw *hw,
					bool is_l2, u32 tsctl, u32 tses);

void sxe_hw_ptp_timestamp_enable(struct sxe_hw *hw);

void sxe_hw_ptp_time_inc_stop(struct sxe_hw *hw);

void sxe_hw_ptp_rx_timestamp_clear(struct sxe_hw *hw);

void sxe_hw_ptp_timestamp_disable(struct sxe_hw *hw);

bool sxe_hw_ptp_is_rx_timestamp_valid(struct sxe_hw *hw);

u64 sxe_hw_ptp_rx_timestamp_get(struct sxe_hw *hw);

void sxe_hw_ptp_tx_timestamp_get(struct sxe_hw *hw,
						u32 *ts_sec, u32 *ts_ns);

u64 sxe_hw_ptp_systime_get(struct sxe_hw *hw);

void sxe_hw_rss_cap_switch(struct sxe_hw *hw, bool is_on);

void sxe_hw_rss_key_set_all(struct sxe_hw *hw, u32 *rss_key);

void sxe_hw_rss_field_set(struct sxe_hw *hw, u32 rss_field);

void sxe_hw_rss_redir_tbl_set_all(struct sxe_hw *hw, u8 *redir_tbl);

u32 sxe_hw_rss_redir_tbl_get_by_idx(struct sxe_hw *hw, u16);

void sxe_hw_rss_redir_tbl_set_by_idx(struct sxe_hw *hw,
						u16 reg_idx, u32 value);

void sxe_hw_rx_dma_ctrl_init(struct sxe_hw *hw);

void sxe_hw_mac_max_frame_set(struct sxe_hw *hw, u32 max_frame);

void sxe_hw_rx_udp_frag_checksum_disable(struct sxe_hw *hw);

void sxe_hw_rx_ip_checksum_offload_switch(struct sxe_hw *hw,
							bool is_on);

void sxe_hw_rx_ring_switch(struct sxe_hw *hw, u8 reg_idx, bool is_on);

void sxe_hw_rx_ring_switch_not_polling(struct sxe_hw *hw, u8 reg_idx, bool is_on);

void sxe_hw_rx_ring_desc_configure(struct sxe_hw *hw,
					u32 desc_mem_len, u64 desc_dma_addr,
					u8 reg_idx);

void sxe_hw_rx_rcv_ctl_configure(struct sxe_hw *hw, u8 reg_idx,
				   u32 header_buf_len, u32 pkg_buf_len
				   );

void sxe_hw_rx_drop_switch(struct sxe_hw *hw, u8 idx, bool is_enable);

void sxe_hw_rx_desc_thresh_set(struct sxe_hw *hw, u8 reg_idx);

void sxe_hw_rx_lro_ack_switch(struct sxe_hw *hw, bool is_on);

void sxe_hw_rx_dma_lro_ctrl_set(struct sxe_hw *hw);

void sxe_hw_rx_nfs_filter_disable(struct sxe_hw *hw);

void sxe_hw_rx_lro_enable(struct sxe_hw *hw, bool is_enable);

void sxe_hw_rx_lro_ctl_configure(struct sxe_hw *hw,
						u8 reg_idx, u32 max_desc);
void sxe_hw_loopback_switch(struct sxe_hw *hw, bool is_enable);

void sxe_hw_rx_cap_switch_off(struct sxe_hw *hw);

void sxe_hw_tx_ring_info_get(struct sxe_hw *hw,
				u8 idx, u32 *head, u32 *tail);

void sxe_hw_tx_ring_switch(struct sxe_hw *hw, u8 reg_idx, bool is_on);

void sxe_hw_tx_ring_switch_not_polling(struct sxe_hw *hw, u8 reg_idx, bool is_on);

void sxe_hw_rx_queue_desc_reg_configure(struct sxe_hw *hw,
					u8 reg_idx, u32 rdh_value,
					u32 rdt_value);

u32 sxe_hw_hdc_fw_status_get(struct sxe_hw *hw);

s32 sxe_hw_hdc_lock_get(struct sxe_hw *hw, u32 trylock);

void sxe_hw_hdc_lock_release(struct sxe_hw *hw, u32 retry_cnt);

bool sxe_hw_hdc_is_fw_over_set(struct sxe_hw *hw);

void sxe_hw_hdc_fw_ov_clear(struct sxe_hw *hw);

u32 sxe_hw_hdc_fw_ack_header_get(struct sxe_hw *hw);

void sxe_hw_hdc_packet_send_done(struct sxe_hw *hw);

void sxe_hw_hdc_packet_header_send(struct sxe_hw *hw, u32 value);

void sxe_hw_hdc_packet_data_dword_send(struct sxe_hw *hw,
						u16 dword_index, u32 value);

u32 sxe_hw_hdc_packet_data_dword_rcv(struct sxe_hw *hw,
						u16 dword_index);

u32 sxe_hw_hdc_channel_state_get(struct sxe_hw *hw);

u32 sxe_hw_pending_irq_read_clear(struct sxe_hw *hw);

void sxe_hw_all_ring_disable(struct sxe_hw *hw, u32 ring_max);

void sxe_hw_tx_ring_head_init(struct sxe_hw *hw, u8 reg_idx);

void sxe_hw_tx_ring_tail_init(struct sxe_hw *hw, u8 reg_idx);

void sxe_hw_tx_enable(struct sxe_hw *hw);

void sxe_hw_tx_desc_thresh_set(
				struct sxe_hw *hw,
				u8 reg_idx,
				u32 wb_thresh,
				u32 host_thresh,
				u32 prefech_thresh);

void sxe_hw_tx_pkt_buf_switch(struct sxe_hw *hw, bool is_on);

void sxe_hw_tx_pkt_buf_size_configure(struct sxe_hw *hw, u8 num_pb);

void sxe_hw_tx_pkt_buf_thresh_configure(struct sxe_hw *hw,
					u8 num_pb, bool dcb_enable);

void sxe_hw_tx_ring_desc_configure(struct sxe_hw *hw,
					u32 desc_mem_len,
					u64 desc_dma_addr, u8 reg_idx);

void sxe_hw_mac_txrx_enable(struct sxe_hw *hw);

void sxe_hw_rx_cap_switch_on(struct sxe_hw *hw);

void sxe_hw_mac_pad_enable(struct sxe_hw *hw);

bool sxe_hw_is_link_state_up(struct sxe_hw *hw);

u32 sxe_hw_link_speed_get(struct sxe_hw *hw);

void sxe_hw_fc_base_init(struct sxe_hw *hw);

void sxe_hw_stats_get(struct sxe_hw *hw, struct sxe_mac_stats *stats);

void sxe_hw_rxq_stat_map_set(struct sxe_hw *hw, u8 idx, u32 value);

void sxe_hw_txq_stat_map_set(struct sxe_hw *hw, u8 idx, u32 value);

void sxe_hw_uc_addr_clear(struct sxe_hw *hw);

void sxe_hw_vt_disable(struct sxe_hw *hw);

void sxe_hw_stats_regs_clean(struct sxe_hw *hw);

void sxe_hw_vlan_ext_type_set(struct sxe_hw *hw, u32 value);

void sxe_hw_link_speed_set(struct sxe_hw *hw, u32 speed);

void sxe_hw_crc_configure(struct sxe_hw *hw);

void sxe_hw_vlan_filter_array_clear(struct sxe_hw *hw);

void sxe_hw_no_snoop_disable(struct sxe_hw *hw);

void sxe_hw_dcb_rate_limiter_clear(struct sxe_hw *hw, u8 ring_max);

s32 sxe_hw_pfc_enable(struct sxe_hw *hw, u8 tc_idx);

void sxe_hw_dcb_vmdq_mq_configure(struct sxe_hw *hw, u8 num_pools);

void sxe_hw_dcb_vmdq_default_pool_configure(struct sxe_hw *hw,
						u8 default_pool_enabled,
						u8 default_pool_idx);

void sxe_hw_dcb_vmdq_up_2_tc_configure(struct sxe_hw *hw,
						u8 *tc_arr);

void sxe_hw_dcb_vmdq_vlan_configure(struct sxe_hw *hw,
						u8 num_pools);

void sxe_hw_dcb_vmdq_pool_configure(struct sxe_hw *hw,
						u8 pool_idx, u16 vlan_id,
						u64 pools_map);

void sxe_hw_dcb_rx_configure(struct sxe_hw *hw, bool is_vt_on,
					u8 sriov_active, u8 pg_tcs);

void sxe_hw_dcb_tx_configure(struct sxe_hw *hw, bool is_vt_on, u8 pg_tcs);

void sxe_hw_pool_xmit_enable(struct sxe_hw *hw, u16 reg_idx, u8 pool_num);

void sxe_hw_rx_pkt_buf_size_set(struct sxe_hw *hw, u8 tc_idx, u16 pbsize);

void sxe_hw_dcb_tc_stats_configure(struct sxe_hw *hw,
					u8 tc_count, bool vmdq_active);

void sxe_hw_dcb_rx_bw_alloc_configure(struct sxe_hw *hw,
				      u16 *refill,
				      u16 *max,
				      u8 *bwg_id,
				      u8 *prio_type,
				      u8 *prio_tc,
				      u8 max_priority);

void sxe_hw_dcb_tx_desc_bw_alloc_configure(struct sxe_hw *hw,
					   u16 *refill,
					   u16 *max,
					   u8 *bwg_id,
					   u8 *prio_type);

void sxe_hw_dcb_tx_data_bw_alloc_configure(struct sxe_hw *hw,
					   u16 *refill,
					   u16 *max,
					   u8 *bwg_id,
					   u8 *prio_type,
					   u8 *prio_tc,
					   u8 max_priority);

void sxe_hw_dcb_pfc_configure(struct sxe_hw *hw,
						u8 pfc_en, u8 *prio_tc,
						u8 max_priority);

void sxe_hw_vmdq_mq_configure(struct sxe_hw *hw);

void sxe_hw_vmdq_default_pool_configure(struct sxe_hw *hw,
						u8 default_pool_enabled,
						u8 default_pool_idx);

void sxe_hw_vmdq_vlan_configure(struct sxe_hw *hw,
						u8 num_pools, u32 rx_mode);

void sxe_hw_vmdq_pool_configure(struct sxe_hw *hw,
						u8 pool_idx, u16 vlan_id,
						u64 pools_map);

void sxe_hw_vmdq_loopback_configure(struct sxe_hw *hw);

void sxe_hw_tx_multi_queue_configure(struct sxe_hw *hw,
				bool vmdq_enable, bool sriov_enable, u16 pools_num);

void sxe_hw_dcb_max_mem_window_set(struct sxe_hw *hw, u32 value);

void sxe_hw_dcb_tx_ring_rate_factor_set(struct sxe_hw *hw,
							u32 ring_idx, u32 rate);

void sxe_hw_mbx_init(struct sxe_hw *hw);

void sxe_hw_vt_ctrl_cfg(struct sxe_hw *hw, u8 num_vfs);

void sxe_hw_tx_pool_bitmap_set(struct sxe_hw *hw,
						u8 reg_idx, u32 bitmap);

void sxe_hw_rx_pool_bitmap_set(struct sxe_hw *hw,
						u8 reg_idx, u32 bitmap);

void sxe_hw_vt_pool_loopback_switch(struct sxe_hw *hw,
						bool is_enable);

void sxe_hw_mac_pool_clear(struct sxe_hw *hw, u8 rar_idx);

s32 sxe_hw_uc_addr_pool_enable(struct sxe_hw *hw,
						u8 rar_idx, u8 pool_idx);

void sxe_hw_pcie_vt_mode_set(struct sxe_hw *hw, u32 value);

u32 sxe_hw_pcie_vt_mode_get(struct sxe_hw *hw);

void sxe_hw_pool_mac_anti_spoof_set(struct sxe_hw *hw,
							u8 vf_idx, bool status);

void sxe_rx_fc_threshold_set(struct sxe_hw *hw);

void sxe_hw_rx_multi_ring_configure(struct sxe_hw *hw,
						u8 tcs, bool is_4Q,
						bool sriov_enable);

void sxe_hw_rx_queue_mode_set(struct sxe_hw *hw, u32 mrqc);

bool sxe_hw_vf_rst_check(struct sxe_hw *hw, u8 vf_idx);

bool sxe_hw_vf_req_check(struct sxe_hw *hw, u8 vf_idx);

bool sxe_hw_vf_ack_check(struct sxe_hw *hw, u8 vf_idx);

s32 sxe_hw_rcv_msg_from_vf(struct sxe_hw *hw, u32 *msg,
				u16 msg_len, u16 index);

s32 sxe_hw_send_msg_to_vf(struct sxe_hw *hw, u32 *msg,
				u16 msg_len, u16 index);

void sxe_hw_mbx_mem_clear(struct sxe_hw *hw, u8 vf_idx);

u32 sxe_hw_pool_rx_mode_get(struct sxe_hw *hw, u16 pool_idx);

void sxe_hw_pool_rx_mode_set(struct sxe_hw *hw,
						u32 vmolr, u16 pool_idx);

void sxe_hw_tx_vlan_tag_clear(struct sxe_hw *hw, u32 vf);

u32 sxe_hw_rx_pool_bitmap_get(struct sxe_hw *hw, u8 reg_idx);

u32 sxe_hw_tx_pool_bitmap_get(struct sxe_hw *hw, u8 reg_idx);

void sxe_hw_pool_rx_ring_drop_enable(struct sxe_hw *hw, u8 vf_idx,
					u16 pf_vlan, u8 ring_per_pool);

void sxe_hw_spoof_count_enable(struct sxe_hw *hw,
						u8 reg_idx, u8 bit_index);

u32 sxe_hw_tx_vlan_insert_get(struct sxe_hw *hw, u32 vf);

bool sxe_hw_vt_status(struct sxe_hw *hw);

s32 sxe_hw_vlvf_slot_find(struct sxe_hw *hw, u32 vlan, bool vlvf_bypass);

u32 sxe_hw_vlan_pool_filter_read(struct sxe_hw *hw, u16 reg_index);

void sxe_hw_mirror_vlan_set(struct sxe_hw *hw, u8 idx,u32 lsb, u32 msb);

void sxe_hw_mirror_virtual_pool_set(struct sxe_hw *hw, u8 idx,u32 lsb, u32 msb);

void sxe_hw_mirror_ctl_set(struct sxe_hw *hw, u8 rule_id,
				    u8 mirror_type, u8 dst_pool, bool on);

void sxe_hw_mirror_rule_clear(struct sxe_hw *hw, u8 rule_id);

u32 sxe_hw_mac_max_frame_get(struct sxe_hw *hw);

void sxe_hw_mta_hash_table_update(struct sxe_hw *hw,
						u8 reg_idx, u8 bit_idx);

void sxe_hw_vf_queue_drop_enable(struct sxe_hw *hw, u8 vf_idx,
					u8 ring_per_pool);
void sxe_hw_fc_mac_addr_set(struct sxe_hw *hw, u8 *mac_addr);

void sxe_hw_macsec_enable(struct sxe_hw *hw, bool is_up, u32 tx_mode,
				u32 rx_mode, u32 pn_trh);

void sxe_hw_macsec_disable(struct sxe_hw *hw, bool is_up);

void sxe_hw_macsec_txsc_set(struct sxe_hw *hw, u32 scl, u32 sch);

void sxe_hw_macsec_rxsc_set(struct sxe_hw *hw, u32 scl, u32 sch, u16 pi);

void sxe_hw_macsec_tx_sa_configure(struct sxe_hw *hw, u8 sa_idx,
				u8 an, u32 pn, u32 *keys);

void sxe_hw_macsec_rx_sa_configure(struct sxe_hw *hw, u8 sa_idx,
				u8 an, u32 pn, u32 *keys);
void sxe_hw_vt_pool_loopback_switch(struct sxe_hw *hw,
							bool is_enable);

#if defined SXE_DPDK_L4_FEATURES && defined SXE_DPDK_FILTER_CTRL
void sxe_hw_fnav_rx_pkt_buf_size_reset(struct sxe_hw *hw, u32 pbsize);

void sxe_hw_fnav_flex_mask_set(struct sxe_hw *hw, u16 flex_mask);

void sxe_hw_fnav_ipv6_mask_set(struct sxe_hw *hw, u16 src_mask, u16 dst_mask);

s32 sxe_hw_fnav_flex_offset_set(struct sxe_hw *hw, u16 offset);

void sxe_hw_fivetuple_filter_add(struct rte_eth_dev *dev,
				struct sxe_fivetuple_node_info *filter);

void sxe_hw_fivetuple_filter_del(struct sxe_hw *hw, u16 reg_index);

void sxe_hw_ethertype_filter_add(struct sxe_hw *hw,
					u8 reg_index, u16 ethertype, u16 queue);

void sxe_hw_ethertype_filter_del(struct sxe_hw *hw, u8 filter_type);

void sxe_hw_syn_filter_add(struct sxe_hw *hw, u16 queue, u8 priority);

void sxe_hw_syn_filter_del(struct sxe_hw *hw);

void sxe_hw_rss_key_set_all(struct sxe_hw *hw, u32 *rss_key);
#endif 

void sxe_hw_fnav_enable(struct sxe_hw *hw, u32 fnavctrl);

s32 sxe_hw_fnav_sample_rules_table_reinit(struct sxe_hw *hw);

s32 sxe_hw_fnav_specific_rule_add(struct sxe_hw *hw,
					  union sxe_fnav_rule_info *input,
					  u16 soft_id, u8 queue);

s32 sxe_hw_fnav_specific_rule_del(struct sxe_hw *hw,
					  union sxe_fnav_rule_info *input,
					  u16 soft_id);

void sxe_hw_fnav_sample_rule_configure(struct sxe_hw *hw,
					  u8 flow_type, u32 hash_value, u8 queue);

void sxe_hw_rss_redir_tbl_reg_write(struct sxe_hw *hw,
						u16 reg_idx, u32 value);

u32 sxe_hw_fnav_port_mask_get(__be16 src_port_mask, __be16 dst_port_mask);

s32 sxe_hw_fnav_specific_rule_mask_set(struct sxe_hw *hw,
				    union sxe_fnav_rule_info *input_mask);

s32 sxe_hw_vlan_filter_configure(struct sxe_hw *hw,
					u32 vid, u32 pool,
					bool vlan_on, bool vlvf_bypass);

void sxe_hw_ptp_systime_init(struct sxe_hw *hw);

#endif 
#endif
