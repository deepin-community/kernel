
#ifndef __SXEVF_HW_H__
#define __SXEVF_HW_H__

#if defined (__KERNEL__) || defined (SXE_KERNEL_TEST)
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/if_ether.h>
#else
#include "sxe_compat_platform.h"
#ifdef SXE_HOST_DRIVER
#include "sxe_drv_type.h"
#endif
#endif

#include "sxevf_regs.h"

#if defined (__KERNEL__) || defined (SXE_KERNEL_TEST)
#define SXE_PRIU64  "llu"
#define SXE_PRIX64  "llx"
#define SXE_PRID64  "lld"
#else
#define SXE_PRIU64  PRIu64
#define SXE_PRIX64  PRIx64
#define SXE_PRID64  PRId64
#endif

#define SXEVF_TXRX_RING_NUM_MAX           8  
#define SXEVF_MAX_TXRX_DESC_POLL          (10)
#define SXEVF_TX_DESC_PREFETCH_THRESH_32  (32)
#define SXEVF_TX_DESC_HOST_THRESH_1       (1)
#define SXEVF_TX_DESC_WRITEBACK_THRESH_8  (8)
#define SXEVF_TXDCTL_HTHRESH_SHIFT        (8)
#define SXEVF_TXDCTL_WTHRESH_SHIFT        (16)

#define SXEVF_TXDCTL_THRESH_MASK          (0x7F)

#define SXEVF_RX_RING_POLL_MAX           (10)

#define SXEVF_MAC_HDR_LEN_MAX           (127)
#define SXEVF_NETWORK_HDR_LEN_MAX       (511)

#define SXEVF_LINK_SPEED_UNKNOWN        0
#define SXEVF_LINK_SPEED_1GB_FULL	0x0020
#define SXEVF_LINK_SPEED_10GB_FULL	0x0080
#define SXEVF_LINK_SPEED_100_FULL	0x0008

#define SXEVF_VFT_TBL_SIZE           (128)   
#define SXEVF_HW_TXRX_RING_NUM_MAX   (128)   

#define SXEVF_VLAN_TAG_SIZE          (4)

#define SXEVF_HW_UC_ENTRY_NUM_MAX   128

enum {
	SXEVF_LINK_TO_PHY   = 0,
	SXEVF_LINK_TO_DOWN,
	SXEVF_LINK_TO_REINIT,
};

enum {
	SXEVF_DIAG_TEST_PASSED			= 0,
	SXEVF_DIAG_TEST_BLOCKED			= 1,
	SXEVF_DIAG_REG_PATTERN_TEST_ERR		= 2,
	SXEVF_DIAG_CHECK_REG_TEST_ERR           = 3,
};

struct sxevf_hw;

struct sxevf_hw_stats {
	u64 base_vfgprc;
	u64 base_vfgptc;
	u64 base_vfgorc;
	u64 base_vfgotc;
	u64 base_vfmprc;

	u64 last_vfgprc;
	u64 last_vfgptc;
	u64 last_vfgorc;
	u64 last_vfgotc;
	u64 last_vfmprc;

	u64 vfgprc;      
	u64 vfgptc;      
	u64 vfgorc;      
	u64 vfgotc;      
	u64 vfmprc;      

	u64 saved_reset_vfgprc;
	u64 saved_reset_vfgptc;
	u64 saved_reset_vfgorc;
	u64 saved_reset_vfgotc;
	u64 saved_reset_vfmprc;
};

void sxevf_hw_ops_init(struct sxevf_hw *hw);


struct sxevf_setup_operations {
	void (*reset)(struct sxevf_hw *);
	void (*hw_stop)(struct sxevf_hw *hw);
	s32  (*regs_test)(struct sxevf_hw *hw);
	u32  (*link_state_get)(struct sxevf_hw *hw);
	u32  (*regs_dump)(struct sxevf_hw *hw, u32 *regs_buff, u32 buf_size);
	bool (*reset_done)(struct sxevf_hw *);
};

struct sxevf_hw_setup {
	const struct sxevf_setup_operations *ops;
};

struct sxevf_irq_operations {
	void (*pending_irq_clear)(struct sxevf_hw *hw);
	void (*ring_irq_interval_set)(struct sxevf_hw *hw, u16 irq_idx, u32 interval);
	void (*event_irq_interval_set)(struct sxevf_hw * hw, u16 irq_idx, u32 value);
	void (*ring_irq_map)(struct sxevf_hw *hw, bool is_tx, u16 hw_ring_idx, u16 irq_idx);
	void (*event_irq_map)(struct sxevf_hw *hw, u16 irq_idx);
	void (*ring_irq_trigger)(struct sxevf_hw *hw, u64 eics);
	void (*irq_enable)(struct sxevf_hw * hw, u32 mask);
	void (*specific_irq_enable)(struct sxevf_hw * hw, u32 value);
	void (*irq_disable)(struct sxevf_hw *hw);
	void (*irq_off)(struct sxevf_hw *hw);
};

struct sxevf_irq_info {
	const struct sxevf_irq_operations *ops;
};

struct sxevf_mbx_operations {

	u32 (*mailbox_read)(struct sxevf_hw *hw);
	void (*mailbox_write)(struct sxevf_hw *hw, u32 value);

	void (*msg_write)(struct sxevf_hw *hw, u8 index, u32 msg);
	u32 (*msg_read)(struct sxevf_hw *hw, u8 index);

	void (*pf_req_irq_trigger)(struct sxevf_hw *hw);
	void (*pf_ack_irq_trigger)(struct sxevf_hw *hw);
};

struct sxevf_mbx_stats {
	u32 send_msgs; 
	u32 rcv_msgs;  

	u32 reqs;      
	u32 acks;      
	u32 rsts;      
};

struct sxevf_mbx_info {
	const struct sxevf_mbx_operations *ops; 

	struct sxevf_mbx_stats stats; 
	u32 msg_len;  
	u32 retry;    
	u32 interval; 
	u32 reg_value; 
	u32 api_version; 
};

struct sxevf_dma_operations {
	void (* tx_ring_desc_configure)(struct sxevf_hw *, u32, u64, u8);
	void (* tx_writeback_off)(struct sxevf_hw *, u8);
	void (* tx_desc_thresh_set)(struct sxevf_hw *, u8, u32, u32, u32);
	void (* tx_ring_switch)(struct sxevf_hw *, u8, bool);
	void (* tx_desc_wb_flush)(struct sxevf_hw *, u8);
	void (* tx_ring_info_get)(struct sxevf_hw *hw, u8 reg_idx,
					u32 *head, u32 *tail);
	void (* rx_disable)(struct sxevf_hw *, u8);
	void (* rx_ring_switch)(struct sxevf_hw *, u8, bool);
	void (* rx_ring_desc_configure)(struct sxevf_hw *, u32, u64, u8);
	void (* rx_rcv_ctl_configure)(struct sxevf_hw *hw, u8 reg_idx,
				   u32 header_buf_len, u32 pkg_buf_len, bool drop_en);
};

struct sxevf_dma_info {
	const struct sxevf_dma_operations *ops;
};

struct sxevf_stat_operations {
	void (*packet_stats_get)(struct sxevf_hw *,
			struct sxevf_hw_stats *);
	void (*stats_init_value_get)(struct sxevf_hw *hw,
				struct sxevf_hw_stats *stats);
};

struct sxevf_stat_info {
	const struct sxevf_stat_operations *ops;
};

struct sxevf_dbu_operations {
	void (*rx_max_used_ring_set)(struct sxevf_hw *, u16);

};

struct sxevf_dbu_info {
	const struct sxevf_dbu_operations	*ops;
};

enum sxevf_hw_state {
	SXEVF_HW_STOP,
	SXEVF_HW_FAULT,
};

struct sxevf_hw {
	u8 __iomem *reg_base_addr;      
	void *adapter;

	void *priv;
	unsigned long state;   
	void (*fault_handle)(void *priv);
	u32 (*reg_read)(const volatile void *reg);
	void (*reg_write)(u32 value, volatile void *reg);
	s32	board_type;		

	struct sxevf_hw_setup   setup;   
	struct sxevf_irq_info   irq;     
	struct sxevf_mbx_info   mbx;     

	struct sxevf_dma_info    dma;    
	struct sxevf_stat_info   stat;   
	struct sxevf_dbu_info    dbu;
};

struct sxevf_reg_info {
	u32 addr;        
	u32 count;       
	u32 stride;      
	const s8 *name;  
};

u16 sxevf_reg_dump_num_get(void);

void sxevf_hw_fault_handle(struct sxevf_hw *hw);

static inline bool sxevf_is_hw_fault(struct sxevf_hw *hw)
{
	return test_bit(SXEVF_HW_FAULT, &hw->state);
}

static inline void sxevf_hw_fault_handle_init(struct sxevf_hw *hw,
			void (*handle)(void *), void *priv)
{
	hw->priv = priv;
	hw->fault_handle = handle;

	return;
}

static inline void sxevf_hw_reg_handle_init(struct sxevf_hw *hw,
		u32 (*read)(const volatile void *),
		void (*write)(u32, volatile void *))
{
	hw->reg_read  = read;
	hw->reg_write = write;

	return;
}

#ifdef SXE_DPDK 

void sxevf_irq_disable(struct sxevf_hw *hw);

void sxevf_hw_stop(struct sxevf_hw *hw);

void sxevf_hw_reset(struct sxevf_hw *hw);

void sxevf_msg_write(struct sxevf_hw *hw, u8 index, u32 msg);

u32 sxevf_msg_read(struct sxevf_hw *hw, u8 index);

u32 sxevf_mailbox_read(struct sxevf_hw *hw);

void sxevf_mailbox_write(struct sxevf_hw *hw, u32 value);

void sxevf_pf_req_irq_trigger(struct sxevf_hw *hw);

void sxevf_pf_ack_irq_trigger(struct sxevf_hw *hw);

void sxevf_rxtx_reg_init(struct sxevf_hw *hw);

void sxevf_irq_enable(struct sxevf_hw *hw, u32 mask);

u32 sxevf_irq_cause_get(struct sxevf_hw *hw);

void sxevf_event_irq_map(struct sxevf_hw *hw, u16 vector);

void sxevf_hw_ring_irq_map(struct sxevf_hw *hw, bool is_tx, u16 hw_ring_idx, u16 vector);

void sxevf_ring_irq_interval_set(struct sxevf_hw *hw, u16 irq_idx, u32 interval);

void sxevf_tx_desc_configure(struct sxevf_hw *hw, u32 desc_mem_len,
				u64 desc_dma_addr, u8 reg_idx);

void sxevf_rx_ring_desc_configure(struct sxevf_hw *hw, u32 desc_mem_len,
					u64 desc_dma_addr, u8 reg_idx);

void sxevf_rx_rcv_ctl_configure(struct sxevf_hw *hw, u8 reg_idx,
				   u32 header_buf_len, u32 pkg_buf_len,
				   bool drop_en);

void sxevf_rss_bit_num_set(struct sxevf_hw *hw, u32 value);

void sxevf_hw_vlan_tag_strip_switch(struct sxevf_hw *hw,
					u16 reg_index, bool is_enable);

void sxevf_tx_queue_thresh_set(struct sxevf_hw *hw, u8 reg_idx,
				u32 prefech_thresh, u32 host_thresh, u32 wb_thresh);

void sxevf_tx_ring_switch(struct sxevf_hw *hw, u8 reg_idx, bool is_on);

void sxevf_rx_ring_switch(struct sxevf_hw *hw, u8 reg_idx, bool is_on);

void sxevf_rx_desc_tail_set(struct sxevf_hw *hw, u8 reg_idx, u32 value);

void sxevf_specific_irq_enable(struct sxevf_hw *hw, u32 value);

void sxevf_packet_stats_get(struct sxevf_hw *hw,
				struct sxevf_hw_stats *stats);

void sxevf_stats_init_value_get(struct sxevf_hw *hw,
				struct sxevf_hw_stats *stats);

u32 sxevf_hw_rss_redir_tbl_get(struct sxevf_hw *hw, u16 reg_idx);

void sxevf_hw_rss_redir_tbl_set(struct sxevf_hw *hw,
						u16 reg_idx, u32 value);

u32 sxevf_hw_rss_key_get(struct sxevf_hw *hw, u8 reg_idx);

u32 sxevf_hw_rss_field_get(struct sxevf_hw *hw);

void sxevf_hw_rss_field_set(struct sxevf_hw *hw, u32 rss_field);

void sxevf_hw_rss_cap_switch(struct sxevf_hw *hw, bool is_on);

void sxevf_hw_rss_key_set_all(struct sxevf_hw *hw, u32 *rss_key);

bool sxevf_hw_is_rss_enabled(struct sxevf_hw *hw);

u32 sxevf_link_state_get(struct sxevf_hw *hw);

u32 sxevf_hw_regs_group_read(struct sxevf_hw *hw,
				const struct sxevf_reg_info *regs,
				u32 *reg_buf);

#endif 
#endif
