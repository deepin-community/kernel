
#ifndef __SXEVF_IPSEC_H__
#define __SXEVF_IPSEC_H__

#include "sxevf_ring.h"

struct sxevf_adapter;

#define SXEVF_IPSEC_SA_CNT_MAX           (1024)

#define SXEVF_IPSEC_RX_INDEX_BASE        (0)
#define SXEVF_IPSEC_TX_INDEX_BASE        (SXEVF_IPSEC_SA_CNT_MAX)

#define SXEVF_IPSEC_AUTH_BIT_LEN         (128)
#define SXEVF_IPSEC_SA_ENTRY_USED        (0x1)

#define SXEVF_IPSEC_PADLEN_OFFSET        \
	((SXEVF_IPSEC_AUTH_BIT_LEN / 8) + 2)
#define SXEVF_IPSEC_PADLEN_BYTE          (1)

#define SXEVF_IPSEC_IP_LEN               (4)
#define SXEVF_IPSEC_KEY_LEN              (4)
#define SXEVF_IPSEC_KEY_SALT_BIT_LEN     (160)
#define SXEVF_IPSEC_KEY_BIT_LEN          (128)
#define SXEVF_IPSEC_KEY_SALT_BYTE_LEN    (SXEVF_IPSEC_KEY_SALT_BIT_LEN / 8)
#define SXEVF_IPSEC_KEY_BYTE_LEN         (SXEVF_IPSEC_KEY_BIT_LEN / 8)

#define SXEVF_IPV4_ADDR_SIZE             (4)
#define SXEVF_IPV6_ADDR_SIZE             (16)

#define SXEVF_IPSEC_RXMOD_VALID          0x00000001
#define SXEVF_IPSEC_RXMOD_PROTO_ESP      0x00000004
#define SXEVF_IPSEC_RXMOD_DECRYPT        0x00000008
#define SXEVF_IPSEC_RXMOD_IPV6           0x00000010
#define SXEVF_IPSEC_RXTXMOD_VF           0x00000020

#define SXEVF_ESP_FEATURES	(NETIF_F_HW_ESP | \
				 NETIF_F_HW_ESP_TX_CSUM | \
				 NETIF_F_GSO_ESP)

struct sxevf_tx_sa   {
	struct xfrm_state *xs;
	u32 key[SXEVF_IPSEC_KEY_LEN];  
	u32 salt;                    
	u32 mode;                    
	bool encrypt;                
	u32 pf_sa_idx;               
	unsigned long status;        
};

struct sxevf_rx_sa   {
	struct hlist_node hlist;
	struct xfrm_state *xs;

	u32 key[SXEVF_IPSEC_KEY_LEN];       
	u32 salt;                         
	__be32 ip_addr[SXEVF_IPSEC_IP_LEN]; 
	u32 mode;                         

	u32 pf_sa_idx;                   
	bool decrypt;                    
	unsigned long status;            
};

struct sxevf_ipsec_context {
	u16 rx_sa_cnt;     
	u16 tx_sa_cnt;     
	u64 rx_ipsec;      

	struct sxevf_rx_sa  *rx_table;  

	struct sxevf_tx_sa  *tx_table;  

	DECLARE_HASHTABLE(rx_table_list, 10); 
};

void sxevf_ipsec_offload_init(struct sxevf_adapter *adapter);

void sxevf_ipsec_offload_exit(struct sxevf_adapter *adapter);

void sxevf_rx_ipsec_proc(struct sxevf_ring *tx_ring,
			  union sxevf_rx_data_desc *desc,
			  struct sk_buff *skb);

s32 sxevf_tx_ipsec_offload(struct sxevf_ring *tx_ring,
			  struct sxevf_tx_buffer *first,
			  struct sxevf_tx_context_desc *ctxt_desc);

void sxevf_ipsec_restore(struct sxevf_adapter *adapter);

#endif
