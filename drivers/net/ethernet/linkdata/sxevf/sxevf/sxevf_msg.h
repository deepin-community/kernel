#ifndef __SXEVF_MSG_H__
#define __SXEVF_MSG_H__

struct sxevf_adapter;

#define SXEVF_UC_ENTRY_NUM_MAX 10
#define SXEVF_MC_ENTRY_NUM_MAX 30

#define SXEVF_MBX_MSG_NUM        16
#define SXEVF_MBX_RETRY_INTERVAL 500
#define SXEVF_MBX_RETRY_COUNT    2000

#define SXEVF_RST_CHECK_NUM          200
#define SXEVF_RST_CHECK_NUM_HV       200000		

#define SXEVF_DEFAULT_ADDR_LEN       4
#define SXEVF_MC_FILTER_TYPE_WORD    3

#define SXEVF_RESET               0x01 
#define SXEVF_DEV_MAC_ADDR_SET    0x02 
#define SXEVF_MC_ADDR_SYNC        0x03 
#define SXEVF_VLAN_SET            0x04 
#define SXEVF_LPE_SET             0x05  

#define SXEVF_UC_ADDR_SYNC        0x06  

#define SXEVF_API_NEGOTIATE       0x08  

#define SXEVF_RING_INFO_GET       0x09  

#define SXEVF_REDIR_TBL_GET       0x0a 
#define SXEVF_RSS_KEY_GET         0x0b 
#define SXEVF_CAST_MODE_SET       0x0c 
#define SXEVF_LINK_ENABLE_GET     0X0d 
#define SXEVF_IPSEC_ADD           0x0e 
#define SXEVF_IPSEC_DEL           0x0f 
#define SXEVF_RSS_CONF_GET		  0x10 

#define SXEVF_PF_CTRL_MSG_LINK_UPDATE  0x100
#define SXEVF_PF_CTRL_MSG_NETDEV_DOWN   0x200

#define SXEVF_PF_CTRL_MSG_REINIT        0x400

#define SXEVF_PF_CTRL_MSG_MASK          0x700
#define SXEVF_PFREQ_MASK                0xFF00 

#define SXEVF_RSS_HASH_KEY_SIZE   (40)  
#define SXEVF_MAX_RETA_ENTRIES    (128) 
#define SXEVF_RETA_ENTRIES_DWORDS (SXEVF_MAX_RETA_ENTRIES / 16)

#define SXEVF_TX_QUEUES      1 
#define SXEVF_RX_QUEUES      2 
#define SXEVF_TRANS_VLAN     3 
#define SXEVF_DEF_QUEUE      4 

#define SXEVF_MSGTYPE_ACK    0x80000000
#define SXEVF_MSGTYPE_NACK   0x40000000

#define SXEVF_MSGINFO_SHIFT  16
#define SXEVF_MSGINFO_MASK   (0xFF << SXEVF_MSGINFO_SHIFT)

#define SXEVF_MSG_NUM(size)   DIV_ROUND_UP(size, 4)

enum sxevf_mbx_api_version {
	SXEVF_MBX_API_10 = 0,
	SXEVF_MBX_API_11,
	SXEVF_MBX_API_12,
	SXEVF_MBX_API_13, 
	SXEVF_MBX_API_14, 

	SXEVF_MBX_API_NR, 
};

enum sxevf_cast_mode {
	SXEVF_CAST_MODE_NONE = 0, 
	SXEVF_CAST_MODE_MULTI,    
	SXEVF_CAST_MODE_ALLMULTI, 
	SXEVF_CAST_MODE_PROMISC,  
};

struct sxevf_rst_msg {
	u32 msg_type;
	u32 mac_addr[2];
	u32 mc_fiter_type;
	u32 sw_mtu;
};

struct sxevf_mbx_api_msg {
	u32 msg_type;
	u32 api_version;
};

struct sxevf_ring_info_msg {
	u32 msg_type;
	u8  max_rx_num;
	u8  max_tx_num;
	u8  tc_num;
	u8  default_tc;
};

struct sxevf_uc_addr_msg {
	u32 msg_type;
	u8 uc_addr[ETH_ALEN];
	u16 pad;
};

struct sxevf_cast_mode_msg {
	u32 msg_type;
	u32 cast_mode;
};

struct sxevf_mc_sync_msg {
	u16 msg_type;
	u16 mc_cnt;  
	u16 mc_addr_extract[SXEVF_MC_ENTRY_NUM_MAX];
};

struct sxevf_uc_sync_msg {
	u16 msg_type;
	u16 index;
	u32 addr[2];
};

struct sxevf_max_frame_msg {
	u32 msg_type;
	u32 max_frame;
};

struct sxevf_vlan_filter_msg {
	u32 msg_type;
	u32 vlan_id;
};

struct sxevf_redir_tbl_msg {
	u32 type;
	u32 entries[SXEVF_RETA_ENTRIES_DWORDS];
};

struct sxevf_rss_hsah_key_msg {
	u32 type;
	u8  hash_key[SXEVF_RSS_HASH_KEY_SIZE];
};

struct sxevf_ipsec_add_msg {
	u32 msg_type;
	u32 pf_sa_idx;
	__be32 spi;
	u8 flags;
	u8 proto;
	u16 family;
	__be32 addr[4];
	u32 key[5];
};

struct sxevf_ipsec_del_msg {
	u32 msg_type;
	u32 sa_idx;
};

struct sxevf_link_enable_msg {
	u32 msg_type;
	bool link_enable;
};

struct sxevf_ctrl_msg {
	u32 msg_type;
};

s32 sxevf_redir_tbl_get(struct sxevf_hw *hw, int rx_ring_num, u32 *redir_tbl);

s32 sxevf_rss_hash_key_get(struct sxevf_hw *hw, u8 *rss_key);

s32 sxevf_mbx_msg_rcv(struct sxevf_hw *hw, u32 *msg, u16 msg_len);

s32 sxevf_ctrl_msg_rcv(struct sxevf_hw *hw, u32 *msg, u16 msg_len);

s32 sxevf_send_and_rcv_msg(struct sxevf_hw *hw, u32 *msg, u8 msg_len);

s32 sxevf_uc_addr_set(struct sxevf_hw *hw, u8 *uc_addr);

s32 sxevf_cast_mode_set(struct sxevf_hw *hw, enum sxevf_cast_mode mode);

s32 sxevf_mc_addr_sync(struct sxevf_hw *hw, struct net_device *netdev);

s32 sxevf_uc_addr_sync(struct sxevf_hw *hw, struct net_device *netdev);

void sxevf_mbx_init(struct sxevf_hw *hw);

bool sxevf_pf_rst_check(struct sxevf_hw *hw);

s32 sxevf_rx_max_frame_set(struct sxevf_hw *hw, u32 max_size);

s32 sxe_vf_filter_array_vid_update(struct sxevf_hw *hw, u32 vlan,
						 bool vlan_on);

s32 sxevf_ctrl_msg_rcv_and_clear(struct sxevf_hw *hw, u32 *msg, u16 msg_len);

#endif 
