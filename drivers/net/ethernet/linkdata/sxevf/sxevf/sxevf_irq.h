
#ifndef __SXEVF_IRQ_H__
#define __SXEVF_IRQ_H__

#include "sxevf_ring.h"

struct ethtool_coalesce;

#define SXEVF_NON_QUEUE_IRQ_NUM   (1)
#define SXEVF_NAPI_WEIGHT         (64)

#define SXEVF_MAX_MSIX_IRQ_NUM	  (2)
#define SXEVF_NON_QUEUE_IRQ_NUM   (1)
#define SXEVF_MIN_QUEUE_IRQ_NUM   (1)
#define SXEVF_MAX_QUEUE_IRQ_NUM   (SXEVF_MAX_MSIX_IRQ_NUM)
#define SXEVF_MIN_MSIX_IRQ_NUM	  (SXEVF_NON_QUEUE_IRQ_NUM + SXEVF_MIN_QUEUE_IRQ_NUM)

#define SXEVF_IRQ_INTERVAL_12K    (336) 
#define SXEVF_IRQ_INTERVAL_20K    (200)
#define SXEVF_IRQ_INTERVAL_100K   (40)

#define SXEVF_IRQ_NAME_EXT_LEN    (16)

#define  SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE  (1)

enum {
	SXEVF_LOWEST_LATENCY = 0,
	SXEVF_LOW_LATENCY,
	SXEVF_BULK_LATENCY,
	SXEVF_LATENCY_NR = 255,
};

#define SXEVF_LOW_LATENCY_BYTE_RATE_MIN    10
#define SXEVF_BULK_LATENCY_BYTE_RATE_MIN   20

struct sxevf_irq_rate {
	unsigned long next_update;   
	unsigned int total_bytes;    
	unsigned int total_packets;  
	u16 irq_interval;             
};

struct sxevf_list {
	struct sxevf_ring *next;   
	u8 cnt;                    
};

struct sxevf_tx_context {
	struct sxevf_list  list;        
	struct sxevf_ring  *xdp_ring;   
	struct sxevf_irq_rate irq_rate; 
	u16 work_limit;                 
};

struct sxevf_rx_context {
	struct sxevf_list list;         
	struct sxevf_irq_rate irq_rate; 
};

struct sxevf_irq_data {
	struct sxevf_adapter *adapter;    
	u16 irq_idx;                      
	u16 irq_interval;                 
	struct sxevf_tx_context tx;       
	struct sxevf_rx_context rx;       
	struct napi_struct napi;          
	struct rcu_head    rcu;           
	s8                 name[IFNAMSIZ + SXEVF_IRQ_NAME_EXT_LEN];                  
	struct sxevf_ring  ring[0] ____cacheline_internodealigned_in_smp;            
};

struct sxevf_irq_context {
	struct msix_entry    *msix_entries;   
	struct sxevf_irq_data  *irq_data[SXEVF_MAX_QUEUE_IRQ_NUM]; 
	u16    ring_irq_num;   
	u16    total_irq_num;   
	u16    rx_irq_interval; 
	u16    tx_irq_interval; 
	u32    irq_mask;       
	u32    mailbox_irq;   
};

s32 sxevf_poll(struct napi_struct *napi, int weight);

void sxevf_irq_ctxt_exit(struct sxevf_adapter *adapter);

s32 sxevf_irq_ctxt_init(struct sxevf_adapter *adapter);

void sxevf_irq_release(struct sxevf_adapter *adapter);

void sxevf_hw_irq_configure(struct sxevf_adapter *adapter);

s32 sxevf_irq_configure(struct sxevf_adapter *adapter);

void sxevf_hw_irq_disable(struct sxevf_adapter *adapter);

void sxevf_napi_disable(struct sxevf_adapter *adapter);

void sxevf_configure_msix_hw(struct sxevf_adapter *adapter);

s32 sxevf_irq_coalesce_set(struct net_device *netdev, struct ethtool_coalesce *user);

s32 sxevf_irq_coalesce_get(struct net_device * netdev,
				struct ethtool_coalesce *user);
#endif
