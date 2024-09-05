#ifndef __SXE_SRIOV_H__
#define __SXE_SRIOV_H__

#include "sxe.h"

#define SXE_VF_FUNCTION_MAX    64

#define SXE_VF_DRV_MAX         (SXE_VF_FUNCTION_MAX - 1)

#define SXE_MAX_VFS_1TC        SXE_VF_FUNCTION_MAX
#define SXE_MAX_VFS_4TC        32
#define SXE_MAX_VFS_8TC        16

#define SXE_MSG_NUM(size)         DIV_ROUND_UP(size, 4)

#define SXE_MSGTYPE_ACK    0x80000000
#define SXE_MSGTYPE_NACK   0x40000000

#define SXE_VFREQ_RESET               0x01 
#define SXE_VFREQ_MAC_ADDR_SET        0x02 
#define SXE_VFREQ_MC_ADDR_SYNC        0x03 
#define SXE_VFREQ_VLAN_SET            0x04 
#define SXE_VFREQ_LPE_SET             0x05  

#define SXE_VFREQ_UC_ADDR_SYNC        0x06  

#define SXE_VFREQ_API_NEGOTIATE       0x08  

#define SXE_VFREQ_RING_INFO_GET       0x09  
#define SXE_VFREQ_REDIR_TBL_GET       0x0a
#define SXE_VFREQ_RSS_KEY_GET         0x0b
#define SXE_VFREQ_CAST_MODE_SET       0x0c  
#define SXE_VFREQ_LINK_ENABLE_GET     0X0d  
#define SXE_VFREQ_IPSEC_ADD           0x0e
#define SXE_VFREQ_IPSEC_DEL           0x0f
#define SXE_VFREQ_RSS_CONF_GET        0x10

#define SXE_VFREQ_MASK                0xFF

#define SXE_CTRL_MSG_LINK_UPDATE     0x100
#define SXE_CTRL_MSG_NETDEV_DOWN     0x200

#define SXE_CTRL_MSG_REINIT           0x400

#define SXE_PF_CTRL_MSG_MASK          0x700
#define SXE_PFREQ_MASK                0xFF00 

#define SXE_VF_MC_ADDR_NUM_SHIFT      16

#define SXE_VFREQ_MSGINFO_SHIFT       16
#define SXE_VFREQ_MSGINFO_MASK     (0xFF << SXE_VFREQ_MSGINFO_SHIFT)

#define SXE_RETA_ENTRIES_DWORDS (SXE_MAX_RETA_ENTRIES / 16)

#define SXE_VF_DISABLE_WAIT     100

enum sxe_mbx_api_version {
	SXE_MBX_API_10 = 0,
	SXE_MBX_API_11,
	SXE_MBX_API_12,
	SXE_MBX_API_13, 
	SXE_MBX_API_14, 

	SXE_MBX_API_NR, 
};

enum sxe_cast_mode {
	SXE_CAST_MODE_NONE = 0, 
	SXE_CAST_MODE_MULTI,    
	SXE_CAST_MODE_ALLMULTI, 
	SXE_CAST_MODE_PROMISC,  
};

struct sxe_msg_table {
	u32 msg_type;
	s32 (*msg_func)(struct sxe_adapter *adapter, u32 *msg, u8 vf_idx);
};

struct sxe_mbx_api_msg {
	u32 msg_type;
	u32 api_version;
};

struct sxe_uc_addr_msg {
	u32 msg_type;
	u8 uc_addr[ETH_ALEN];
	u16 pad;
};

struct sxe_rst_rcv {
	u32 msg_type;
};

struct sxe_rst_reply {
	u32 msg_type;
	u32 mac_addr[2];
	u32 mc_filter_type;
	u32 sw_mtu;
};

struct sxe_rst_msg {
	union {
		struct sxe_rst_rcv rcv;
		struct sxe_rst_reply reply;
	};
};

struct sxe_ring_info_msg {
	u32 msg_type;
	u8  max_rx_num; 
	u8  max_tx_num; 
	u8  tc_num;     
	u8  default_tc; 
};

struct sxe_mc_sync_msg {
	u16 msg_type;
	u16 mc_cnt;  
	u16 mc_addr_extract[SXE_VF_MC_ENTRY_NUM_MAX];
};

struct sxe_uc_sync_msg {
	u16 msg_type;
	u16 index;
	u32 addr[2];
};

struct sxe_cast_mode_msg {
	u32 msg_type;
	u32 cast_mode;
};

struct sxe_redir_tbl_msg {
	u32 type;
	u32 entries[SXE_RETA_ENTRIES_DWORDS];
};

struct sxe_rss_hsah_key_msg {
	u32 type;
	u8  hash_key[SXE_RSS_KEY_SIZE];
};

struct sxe_rss_hash_msg {
	u32 type;
	u8  hash_key[SXE_RSS_KEY_SIZE];
	u64 rss_hf;
};

struct sxe_ipsec_add_msg {
	u32 msg_type;
	u32 pf_sa_idx;
	__be32 spi;
	u8 flags;
	u8 proto;
	u16 family;
	__be32 ip_addr[4];
	u32 key[5];
};

struct sxe_ipsec_del_msg {
	u32 msg_type;
	u32 pf_sa_idx;
};

struct sxe_link_enable_msg {
	u32 msg_type;
	bool link_enable;
};

s32 sxe_set_vf_vlan(struct net_device *netdev, int vf, u16 vlan,
			  u8 qos, __be16 vlan_proto);

#ifdef HAVE_NDO_SET_VF_LINK_STATE
s32 sxe_set_vf_link_state(struct net_device *netdev, s32 vf_idx, s32 state);

void sxe_vf_enable_and_reinit_notify_vf_all(struct sxe_adapter *adapter);
#endif 

void sxe_sriov_init(struct sxe_adapter *adapter);

void sxe_vf_exit(struct sxe_adapter *adapter);

s32 sxe_sriov_configure(struct pci_dev *pdev, int num_vfs);

void sxe_vt1_configure(struct sxe_adapter *adapter);

void sxe_mailbox_irq_handle(struct sxe_adapter *adapter);

s32 sxe_set_vf_mac(struct net_device *dev, s32 vf_idx, u8 *mac_addr);

s32 sxe_set_vf_spoofchk(struct net_device *dev, s32 vf_idx, bool status);

s32 sxe_set_vf_trust(struct net_device *dev,
					s32 vf_idx, bool status);

int sxe_set_vf_rss_query_en(struct net_device *dev, s32 vf_idx, bool status);

s32 sxe_get_vf_config(struct net_device *dev, s32 vf_idx,
				struct ifla_vf_info *info);

s32 sxe_set_vf_rate(struct net_device *netdev, s32 vf_idx,
				s32 min_rate, s32 max_rate);

s32 sxe_vf_req_task_handle(struct sxe_adapter *adapter, u8 vf_idx);

void sxe_vf_ack_task_handle(struct sxe_adapter *adapter, u8 vf_idx);

void sxe_vf_hw_rst(struct sxe_adapter *adapter, u8 vf_idx);

void sxe_vf_down(struct sxe_adapter *adapter);

void sxe_bad_vf_flr(struct sxe_adapter *adapter);

void sxe_spoof_packets_check(struct sxe_adapter *adapter);

bool sxe_vf_tx_pending(struct sxe_adapter *adapter);

void sxe_vf_rate_update(struct sxe_adapter *adapter);

void sxe_link_update_notify_vf_all(struct sxe_adapter *adapter);

void sxe_netdev_down_notify_vf_all(struct sxe_adapter *adapter);

void sxe_vf_trust_update_notify(struct sxe_adapter *adapter, u16 index);

void sxe_param_sriov_enable(struct sxe_adapter *adapter, u8 user_num_vfs);

void sxe_vf_resource_release(struct sxe_adapter *adapter);

void sxe_vf_disable(struct sxe_adapter *adapter);

#endif
