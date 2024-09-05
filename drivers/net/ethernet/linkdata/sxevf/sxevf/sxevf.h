#ifndef __SXEVF_H__
#define __SXEVF_H__

#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/cpumask.h>
#include <linux/if_vlan.h>

#include "sxe_log.h"
#include "sxevf_hw.h"
#include "sxevf_ring.h"
#include "sxevf_irq.h"
#include "sxevf_monitor.h"
#include "sxevf_ipsec.h"
#include "sxe_errno.h"
#include "sxe_compat.h"
#include "sxe_errno.h"

#define SXEVF_JUMBO_FRAME_SIZE_MAX    9728
#define SXEVF_ETH_DEAD_LOAD           (ETH_HLEN + ETH_FCS_LEN + 2 * VLAN_HLEN)
#define DEV_NAME_LEN                  16

#define CHAR_BITS                     8

#define SXEVF_HZ_TRANSTO_MS	      1000

#define SXEVF_KFREE(addr)              \
        do {                         \
		if (addr != NULL) {  \
			kfree(addr); \
			addr = NULL; \
		}                    \
	} while(0)

enum {
	SXEVF_DCB_ENABLE           = BIT(0),
	SXEVF_RX_LEGACY_ENABLE     = BIT(1),
};

enum sxevf_boards {
	SXE_BOARD_VF,
	SXE_BOARD_VF_HV,		
};

struct sxevf_sw_stats {
	u64 tx_busy;    
	u64 restart_queue;  
	u64 tx_timeout_count; 
	u64 hw_csum_rx_error; 
	u64 alloc_rx_page;   
	u64 alloc_rx_page_failed; 
	u64 alloc_rx_buff_failed; 
};

struct sxevf_stats {
	struct sxevf_sw_stats sw;
	struct sxevf_hw_stats hw;
};

enum sxevf_nic_state {
	SXEVF_RESETTING,
	SXEVF_TESTING,
	SXEVF_DOWN,
	SXEVF_DISABLED,
	SXEVF_REMOVING,
};

struct sxevf_mac_filter_context {
	u8  cur_uc_addr[ETH_ALEN]; 
	u8  def_uc_addr[ETH_ALEN]; 
	u8  mc_filter_type;        
};

struct sxevf_adapter {
	char dev_name[DEV_NAME_LEN];

	struct net_device *netdev; 
	struct pci_dev *pdev;      

	u32 sw_mtu;                
	u16 msg_enable;            

	struct sxevf_ring_context rx_ring_ctxt;  
	struct sxevf_ring_context tx_ring_ctxt;  
	struct sxevf_ring_context xdp_ring_ctxt; 

#ifdef SXE_IPSEC_CONFIGURE
	struct sxevf_ipsec_context ipsec_ctxt;
#endif

	struct sxevf_mac_filter_context mac_filter_ctxt;
	struct sxevf_irq_context  irq_ctxt;        
	struct sxevf_monitor_context monitor_ctxt; 
	struct sxevf_ring_feature  ring_f;         

	u32 cap;	           
	u32 cap2;	           
	u8 tcs;                    

	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];

	struct sxevf_hw hw;           

	struct bpf_prog *xdp_prog;    
	spinlock_t mbx_lock;         
	u32 mbx_version;             

	unsigned long state;         
	struct sxevf_stats stats;    
	struct sxevf_link_info link; 
};

struct workqueue_struct *sxevf_wq_get(void);

s32 sxevf_dev_reset(struct sxevf_hw *hw);

void sxevf_start_adapter(struct sxevf_adapter *adapter);

void sxevf_mbx_api_version_init(struct sxevf_adapter *adapter);

s32 sxevf_ring_irq_init(struct sxevf_adapter *adapter);

void sxevf_ring_irq_exit(struct sxevf_adapter *adapter);

void sxevf_save_reset_stats(struct sxevf_adapter *adapter);

void sxevf_last_counter_stats_init(struct sxevf_adapter *adapter);

#endif
