
#ifndef __SXE_H__
#define __SXE_H__

#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/cpumask.h>
#include <linux/if_vlan.h>

#include <linux/timecounter.h>
#include <linux/net_tstamp.h>
#include <linux/ptp_clock_kernel.h>

#include "sxe_log.h"
#include "sxe_hw.h"
#include "sxe_irq.h"
#include "sxe_phy.h"
#include "sxe_monitor.h"
#include "sxe_ipsec.h"
#include "sxe_dcb.h"
#include "sxe_errno.h"
#include "drv_msg.h"
#include "sxe_compat.h"

#define SXE_ETH_DEAD_LOAD (ETH_HLEN + ETH_FCS_LEN + 2 * VLAN_HLEN)
#define SXE_MAX_JUMBO_FRAME_SIZE	9728 
#define DEV_NAME_LEN                    16

#define CHAR_BITS				(8)

#define SXE_HZ_TRANSTO_MS		1000

#define PCI_BDF_DEV_SHIFT		(3)
#define PCI_BDF_DEV_MASK		(0x1F)
#define PCI_BDF_FUNC_MASK		(0x7)

#ifdef SXE_TEST
#define SXE_MAX_MACVLANS   3
#else
#define SXE_MAX_MACVLANS   63
#endif

#define SXE_KFREE(addr)              \
	do {                         \
		if (addr != NULL) {  \
			kfree(addr); \
			addr = NULL; \
		}                    \
	} while(0)

enum adapter_cap {
	SXE_DCB_ENABLE           = BIT(0),
	SXE_SRIOV_ENABLE         = BIT(1),
	SXE_FNAV_SAMPLE_ENABLE   = BIT(2),
	SXE_FNAV_SPECIFIC_ENABLE = BIT(3),
	SXE_SRIOV_DCB_ENABLE     = (SXE_DCB_ENABLE | SXE_SRIOV_ENABLE),
	SXE_LRO_ENABLE           = BIT(4),
	SXE_RSS_FIELD_IPV4_UDP   = BIT(5),
	SXE_RSS_FIELD_IPV6_UDP   = BIT(6),
	SXE_RX_LEGACY            = BIT(7),
	SXE_RX_HWTSTAMP_ENABLED  = BIT(8),
	SXE_MSI_ENABLED          = BIT(9),
	SXE_MSIX_ENABLED         = BIT(10),
	SXE_VLAN_PROMISC         = BIT(11),
	SXE_LRO_CAPABLE          = BIT(12),
	SXE_RSS_ENABLE           = BIT(13),
	SXE_MACVLAN_ENABLE       = BIT(14),
	SXE_1588V2_ONE_STEP      = BIT(15),
	SXE_PTP_PPS_ENABLED      = BIT(16),
	SXE_RX_HWTSTAMP_IN_REGISTER = BIT(17),
	SXE_TPH_CAPABLE          = BIT(18),
	SXE_TPH_ENABLE           = BIT(19),
#ifdef SXE_IPSEC_CONFIGURE
	SXE_IPSEC_ENABLED        = BIT(20),
	SXE_VF_IPSEC_ENABLED     = BIT(21),
#endif
};

enum sxe_nic_state {
	SXE_RESETTING,
	SXE_TESTING,
	SXE_DOWN,
	SXE_DISABLED,
	SXE_REMOVING,
	SXE_PTP_RUNNING,
	SXE_PTP_TX_IN_PROGRESS,
	SXE_IN_SFP_INIT,
	SXE_SFP_MULTI_SPEED_SETTING,
};

struct sxe_sw_stats {
	u64 tx_busy;                
	u64 non_eop_descs;          
	u64 lro_total_count;        
	u64 lro_total_flush;        
	u64 fnav_overflow;          
	u64 reset_work_trigger_cnt; 
	u64 restart_queue;          
	u64 hw_csum_rx_error;       
	u64 alloc_rx_page;          
	u64 alloc_rx_page_failed;   
	u64 alloc_rx_buff_failed;   
	u64 tx_hwtstamp_timeouts;   
	u64 tx_hwtstamp_skipped;    
	u64 rx_hwtstamp_cleared;    
	u64 tx_ipsec;               
	u64 rx_ipsec;               
	u64 link_state_change_cnt;  
};

struct sxe_stats_info {
	struct sxe_sw_stats  sw;  
	struct sxe_mac_stats hw;  
	struct mutex stats_mutex;
};

struct sxe_macvlan {
	struct net_device *netdev;  
	u32 tx_ring_offset;         
	u32 rx_ring_offset;         
	s32 pool;                   
};

struct sxe_fnav_rule_node {
	struct hlist_node node;
	union sxe_fnav_rule_info rule_info;
	u16 sw_idx;
	u64 ring_cookie;
};

struct sxe_fnav_context {
	u32 rules_table_size;                   

	u32 sample_rate;                        
	spinlock_t sample_lock;               
	u32 sample_rules_cnt;
	time64_t fdir_overflow_time;
	bool is_sample_table_overflowed;
	DECLARE_HASHTABLE(sample_list, 13);

	spinlock_t specific_lock;               
	u32 rule_cnt;                           
	struct hlist_head rules_list;           
	union sxe_fnav_rule_info rules_mask;    
};


struct sxe_vf_uc_addr_list{
	struct list_head list;
	u8 vf_idx;
	bool free;
	bool is_macvlan;
	u8 uc_addr[ETH_ALEN];
};

struct sxe_vf_info {
	u8 mac_addr[ETH_ALEN]; 
	u16 mc_hash[SXE_VF_MC_ENTRY_NUM_MAX]; 
	u8  mc_hash_used; 
	u16 pf_vlan; 
	u16 pf_qos; 
	u8  cast_mode; 
	u8  trusted :1;  
	u8  is_ready :1; 
	u8  spoof_chk_enabled :1; 
	u8  rss_query_enabled :1; 
	u8  mac_from_pf :1; 
	u8  reserved :3;  
	u16 tx_rate;    
	s32 link_enable;  
#ifdef HAVE_NDO_SET_VF_LINK_STATE
	s32 link_state;   
#endif
	struct pci_dev *vf_dev; 
	u32 mbx_version; 
};

struct sxe_virtual_context {
	u8 num_vfs;            
	u16 bridge_mode;       
	u32 mbps_link_speed;   
	bool is_rate_set;
	struct sxe_vf_uc_addr_list head;
	struct sxe_vf_uc_addr_list *vf_uc_list; 
	struct sxe_vf_info *vf_info; 

	u32 err_refcount;
	spinlock_t vfs_lock;

	DECLARE_BITMAP(pf_pool_bitmap, SXE_MAX_MACVLANS + 1);
};

struct sxe_ptp_context {
	struct cyclecounter hw_cc;
	struct timecounter hw_tc;
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_clock_info;
	struct work_struct ptp_tx_work;
	struct sk_buff *ptp_tx_skb;
	struct hwtstamp_config tstamp_config;
	unsigned long ptp_tx_start;
	unsigned long last_overflow_check;
	unsigned long last_rx_ptp_check;
	spinlock_t ptp_timer_lock;
	void (*ptp_setup_spp)(struct sxe_adapter *);
	u32 tx_hwtstamp_sec;
	u32 tx_hwtstamp_nsec;
};

struct sxe_dcb_context {
#ifdef SXE_DCB_CONFIGURE
	struct ieee_pfc *ieee_pfc;
	struct ieee_ets *ieee_ets;
#endif
	struct sxe_dcb_cee_config cee_cfg;
	struct sxe_dcb_cee_config cee_temp_cfg;
	u8 cee_cfg_bitmap;
	u8 hw_tcs;
	u8 dcbx_cap;
	u8 default_up;
	enum sxe_fc_mode last_lfc_mode;
};

struct sxe_uc_addr_table {
	u8 addr[ETH_ALEN];
	u16 pool;
	unsigned long state;
};

struct sxe_mac_filter_context {
	u8  cur_mac_addr[ETH_ALEN]; 
	u8  def_mac_addr[ETH_ALEN]; 
	struct sxe_uc_addr_table *uc_addr_table; 

	u32 mc_hash_table[SXE_MTA_ENTRY_NUM_MAX]; 
	u32 mc_hash_table_used; 
};

struct sxe_vlan_context {
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
	u32  vlan_table_size;
};

struct sxe_cdev_info {
	struct cdev         cdev;	
	dev_t               dev_no;	
	struct device       *device;	
};

struct sxe_self_test_context {
	u32 icr;                               
	struct sxe_ring tx_ring;               
	struct sxe_ring rx_ring;               
};

struct sxe_hdc_context {
	struct completion sync_done;		
	struct work_struct time_sync_work;	
	u16 time_sync_failed;			
};

struct sxe_fw_info {
	u8 fw_version[SXE_VERSION_LEN];		
};

struct sxe_adapter {
	char dev_name[DEV_NAME_LEN];
	struct net_device *netdev;                  
	struct pci_dev *pdev;                       
	struct sxe_hw hw;                           

	u32 cap;                                    
	u32 cap2;                                   

	unsigned long state;                        
	struct sxe_link_info link;                  

	u16 msg_enable;                             

	struct sxe_ring_feature  ring_f;            
	struct sxe_pool_feature  pool_f;            

	struct sxe_ring_context rx_ring_ctxt;       
	struct sxe_ring_context tx_ring_ctxt;       
	struct sxe_ring_context xdp_ring_ctxt;      
	struct sxe_monitor_context monitor_ctxt;    

	struct sxe_irq_context  irq_ctxt;           

	struct sxe_fnav_context fnav_ctxt;	    

	struct sxe_virtual_context vt_ctxt;         

#ifdef SXE_IPSEC_CONFIGURE
	struct sxe_ipsec_context ipsec;             
#endif

	struct bpf_prog *xdp_prog;
#ifdef HAVE_AF_XDP_ZERO_COPY
	unsigned long *af_xdp_zc_qps;               
#endif

	u32 *rss_key;                               
	u8  rss_indir_tbl[SXE_MAX_RETA_ENTRIES];    

	struct sxe_ptp_context  ptp_ctxt;	    

	struct sxe_dcb_context dcb_ctxt;            

	struct sxe_vlan_context vlan_ctxt;

	struct sxe_mac_filter_context mac_filter_ctxt;

	struct sxe_stats_info stats;

	struct sxe_fw_info fw_info;

	struct dentry *debugfs_entries;

	struct sxe_phy_context phy_ctxt;

	struct sxe_self_test_context test_ctxt;

	struct sxe_cdev_info cdev_info;
	struct sxe_hdc_context hdc_ctxt;

	u16 bridge_mode;

#ifdef SXE_WOL_CONFIGURE
	u32 wol;
#endif
};

struct sxe_fnav_sample_work_info {
	struct work_struct work_st;
	struct sxe_adapter *adapter;
	u64 hash;
};

struct sxe_fnav_sample_filter {
	struct hlist_node hlist;
	u32 hash;
};

static inline u8 sxe_dcb_tc_get(struct sxe_adapter *adapter)
{
	return adapter->dcb_ctxt.hw_tcs;
}

static inline void sxe_dcb_tc_set(struct sxe_adapter *adapter, u8 tcs)
{
	adapter->dcb_ctxt.hw_tcs = tcs;
}

static inline u8 sxe_rxtx_pkt_buf_max(struct sxe_adapter *adapter)
{
	return (adapter->cap & SXE_DCB_ENABLE) ? SXE_PKG_BUF_NUM_MAX : 1;
}

struct workqueue_struct *sxe_workqueue_get(void);

void sxe_fw_version_get(struct sxe_adapter *adapter);

s32 sxe_ring_irq_init(struct sxe_adapter *adapter);

void sxe_ring_irq_exit(struct sxe_adapter *adapter);

s32 sxe_hw_reset(struct sxe_adapter *adapter);

void sxe_hw_start(struct sxe_hw *hw);

static inline void stats_lock(struct sxe_adapter *adapter)
{
	mutex_lock(&adapter->stats.stats_mutex);
	return;
}

static inline void stats_unlock(struct sxe_adapter *adapter)
{
	mutex_unlock(&adapter->stats.stats_mutex);
	return;
}

static inline void carrier_lock(struct sxe_adapter *adapter)
{
	mutex_lock(&adapter->link.carrier_mutex);
	return;
}

static inline void carrier_unlock(struct sxe_adapter *adapter)
{
	mutex_unlock(&adapter->link.carrier_mutex);
	return;
}

void sxe_tph_update(struct sxe_irq_data *irq_data);

void sxe_tph_setup(struct sxe_adapter *adapter);

#endif
