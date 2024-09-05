
#ifndef __SXE_IPSEC_H__
#define __SXE_IPSEC_H__

#include "sxe_ring.h"

#ifdef CONFIG_SXE_FPGA_SINGLE_PORT
#undef SXE_IPSEC_MAX_SA_COUNT
#undef SXE_IPSEC_MAX_RX_IP_COUNT
#undef SXE_IPSEC_BASE_TX_INDEX
#define SXE_IPSEC_MAX_SA_COUNT          24
#define SXE_IPSEC_MAX_RX_IP_COUNT       8
#define SXE_IPSEC_BASE_TX_INDEX         SXE_IPSEC_MAX_SA_COUNT
#endif

#define SXE_IPSEC_SA_CNT_MAX           (1024)
#define SXE_IPSEC_IP_CNT_MAX           (128)

#define SXE_IPSEC_RX_INDEX_BASE        (0)
#define SXE_IPSEC_TX_INDEX_BASE        (SXE_IPSEC_SA_CNT_MAX)

#define SXE_IPSEC_AUTH_BIT_LEN         (128)
#define SXE_IPSEC_SA_ENTRY_USED        (0x1)
#define SXE_IPSEC_IP_ENTRY_USED        (0x1)

#define SXE_IPSEC_IP_LEN               (4)
#define SXE_IPSEC_KEY_LEN              (4)
#define SXE_IPSEC_KEY_SALT_BIT_LEN     (160)
#define SXE_IPSEC_KEY_BIT_LEN          (128)
#define SXE_IPSEC_KEY_SALT_BYTE_LEN    (SXE_IPSEC_KEY_SALT_BIT_LEN / 8)
#define SXE_IPSEC_KEY_BYTE_LEN         (SXE_IPSEC_KEY_BIT_LEN / 8)

#define SXE_IPSEC_PADLEN_OFFSET        (SXE_IPSEC_KEY_BYTE_LEN + 2)

#define SXE_IPV4_ADDR_SIZE             (4)
#define SXE_IPV6_ADDR_SIZE             (16)

#define SXE_IPSEC_RXMOD_VALID          0x00000001
#define SXE_IPSEC_RXMOD_PROTO_ESP      0x00000004
#define SXE_IPSEC_RXMOD_DECRYPT        0x00000008
#define SXE_IPSEC_RXMOD_IPV6           0x00000010
#define SXE_IPSEC_RXTXMOD_VF           0x00000020

struct sxe_tx_sa   {
	struct xfrm_state *xs;
	u32 key[SXE_IPSEC_KEY_LEN];  
	u32 salt;                    
	u32 mode;                    
	bool encrypt;                
	u16 vf_idx;                  
	unsigned long status;        
};

struct sxe_rx_sa   {
	struct hlist_node hlist;
	struct xfrm_state *xs;

	u32 key[SXE_IPSEC_KEY_LEN];       
	u32 salt;                         
	__be32 ip_addr[SXE_IPSEC_IP_LEN]; 
	u32 mode;                         
	u8 ip_idx;                        

	u16 vf_idx;                      
	bool decrypt;                    
	unsigned long status;            
};

struct sxe_rx_ip {
	__be32 ip_addr[SXE_IPSEC_IP_LEN];
	u16 ref_cnt;
	unsigned long status;         
};

struct sxe_ipsec_context {
	u16 rx_sa_cnt;     
	u16 tx_sa_cnt;    
	atomic64_t rx_ipsec;     

	struct sxe_rx_ip  *ip_table;  
	struct sxe_rx_sa  *rx_table;  

	struct sxe_tx_sa  *tx_table;  

	DECLARE_HASHTABLE(rx_table_list, 10); 
};

s32 sxe_tx_ipsec_offload(struct sxe_ring *tx_ring,
			  struct sxe_tx_buffer *first,
			  struct sxe_tx_context_desc *ctxt_desc);

void sxe_rx_ipsec_proc(struct sxe_ring *tx_ring,
			  union sxe_rx_data_desc *desc,
			  struct sk_buff *skb);

void sxe_ipsec_offload_init(struct sxe_adapter *adapter);

void sxe_ipsec_table_restore(struct sxe_adapter *adapter);

void sxe_ipsec_offload_exit(struct sxe_adapter *adapter);

s32 sxe_vf_ipsec_add(struct sxe_adapter *adapter, u32 *msg, u8 vf_idx);

s32 sxe_vf_ipsec_del(struct sxe_adapter *adapter, u32 *msg, u8 vf_idx);

void sxe_vf_ipsec_entry_clear(struct sxe_adapter *adapter, u32 vf_idx);

#endif

