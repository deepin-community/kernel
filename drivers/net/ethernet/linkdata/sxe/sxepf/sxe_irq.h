
#ifndef __SXE_IRQ_H__
#define __SXE_IRQ_H__

#include "sxe_ring.h"

struct sxe_adapter;
struct ethtool_coalesce;

#define    SXE_MSIX_IRQ_MAX_NUM    (64)
#define    SXE_EVENT_IRQ_NUM       (1)
#define    SXE_RING_IRQ_MIN_NUM    (1)
#define    SXE_RING_IRQ_MAX_NUM    (SXE_MSIX_IRQ_MAX_NUM)
#define    SXE_MSIX_IRQ_MIN_NUM	   (SXE_EVENT_IRQ_NUM + SXE_RING_IRQ_MIN_NUM)

#define    SXE_PCIE_MSIX_CAPS_OFFSET  (0xB2)  
#define    SXE_PCIE_MSIX_ENTRY_MASK   (0x7FF)  
#define    SXE_NAPI_WEIGHT            (64)

#define    SXE_IRQ_ITR_INC_MIN   (2)
#define    SXE_IRQ_ITR_MIN       (10)
#define    SXE_IRQ_ITR_MAX       (126) 
#define    SXE_IRQ_ITR_LATENCY   (0x80)
#define    SXE_IRQ_ITR_BULK      (0x00)
#define    SXE_IRQ_ITR_MASK      (0x00000FF8)

#define    SXE_IRQ_BULK          (0)
#define    SXE_IRQ_ITR_12K       (336) 
#define    SXE_IRQ_ITR_20K       (200)
#define    SXE_IRQ_ITR_100K      (40)

#define   SXE_IRQ_LRO_ITR_MIN    (24)
#define   SXE_IRQ_ITR_CONSTANT_MODE_VALUE  (1)

#define   SXE_IRQ_ITR_PKT_4			4
#define   SXE_IRQ_ITR_PKT_48		48
#define   SXE_IRQ_ITR_PKT_96		96
#define   SXE_IRQ_ITR_PKT_256		256
#define   SXE_IRQ_ITR_BYTES_9000	9000

enum sxe_irq_mode {
	SXE_IRQ_MSIX_MODE = 0,
	SXE_IRQ_MSI_MODE,
	SXE_IRQ_INTX_MODE,
};

struct sxe_irq_rate {
	unsigned long next_update;   
	unsigned int total_bytes;    
	unsigned int total_packets;  
	u16 irq_interval;             
};

struct sxe_list {
	struct sxe_ring *next;   
	u8 cnt;                  
};

struct sxe_tx_context {
	struct sxe_list  list;         
	struct sxe_ring  *xdp_ring;    
	struct sxe_irq_rate irq_rate;  
	u16 work_limit;                
};

struct sxe_rx_context {
	struct sxe_irq_rate irq_rate; 
	struct sxe_list list; 
};

struct sxe_irq_data {
	struct sxe_adapter           *adapter;    
#ifdef SXE_TPH_CONFIGURE
	s32                     cpu;
#endif
	u16 irq_idx;       
	u16 irq_interval;  
	struct sxe_tx_context tx;             
	struct sxe_rx_context rx;             
	struct napi_struct napi;               
	cpumask_t          affinity_mask;      
	s32                numa_node;          
	struct rcu_head    rcu;                
	s8                 name[IFNAMSIZ + 16]; 
	struct sxe_ring    ring[0] ____cacheline_internodealigned_in_smp;
};

struct sxe_irq_context {
	struct msix_entry    *msix_entries;   
	struct sxe_irq_data  *irq_data[SXE_RING_IRQ_MAX_NUM]; 
	spinlock_t           event_irq_lock;
	u16    max_irq_num;     
	u16    ring_irq_num;    
	u16    total_irq_num;   
	u16    rx_irq_interval; 
	u16    tx_irq_interval; 
};

int sxe_irq_configure(struct sxe_adapter *adapter);

int sxe_poll(struct napi_struct *napi, int weight);

void sxe_napi_disable(struct sxe_adapter *adapter);

void sxe_irq_release(struct sxe_adapter *adapter);

void sxe_hw_irq_disable(struct sxe_adapter *adapter);

void sxe_irq_ctxt_exit(struct sxe_adapter *adapter);

s32 sxe_irq_ctxt_init(struct sxe_adapter *adapter);

void sxe_hw_irq_configure(struct sxe_adapter *adapter);

s32 sxe_config_space_irq_num_get(struct sxe_adapter *adapter);

s32 sxe_irq_coalesce_set(struct net_device *netdev, struct ethtool_coalesce *user);

s32 sxe_irq_coalesce_get(struct net_device * netdev,
				struct ethtool_coalesce *user);

bool sxe_is_irq_msi_mode(void);

bool sxe_is_irq_intx_mode(void);

#endif
