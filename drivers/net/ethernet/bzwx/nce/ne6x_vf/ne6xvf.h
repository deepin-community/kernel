/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6XVF_H
#define _NE6XVF_H

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/aer.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/sctp.h>
#include <linux/ipv6.h>
#include <linux/kernel.h>
#include <linux/bitops.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/gfp.h>
#include <linux/skbuff.h>
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/socket.h>
#include <linux/jiffies.h>
#include <net/ipv6.h>
#include <net/ip6_checksum.h>
#include <net/udp.h>
#include <linux/sctp.h>
#include <linux/pci_hotplug.h>
#include <net/pkt_cls.h>
#include <net/tc_act/tc_gact.h>
#include <net/tc_act/tc_mirred.h>
#include <linux/bitmap.h>

#include "reg.h"
#include "common.h"
#include "feature.h"
#include "txrx.h"
#include "mailbox.h"
#include "ne6xvf_virtchnl.h"

#define NE6XVF_MAX_AQ_BUF_SIZE         4096
#define NE6XVF_AQ_LEN                  32
#define NE6XVF_AQ_MAX_ERR              20 /* times to try before resetting AQ */

#define NE6XVF_REG_ADDR(_VPID, _OFST)  (((_VPID) << 12) + ((_OFST) << 4))

#define NE6XVF_DB_STATE                0x1a
#define NE6XVF_MAILBOX_DATA            0x19
#define NE6XVF_PF_MAILBOX_DATA         0x18

#define NE6XVF_QC_TAIL1(_Q)  (((_Q) << 12) | (NE6X_CQ_HD_POINTER << 4)) /* _i=0...15 Reset: PFR */
#define NE6XVF_QTX_TAIL1(_Q) (((_Q) << 12) | (0 << 11) | 0)             /* _i=0...15 Reset: PFR */
#define NE6XVF_QRX_TAIL1(_Q) (((_Q) << 12) | (1 << 11) | 0)             /* _i=0...15 Reset: PFR */

#define ne6xvf_debug(h, m, s, ...)                          \
do {                                                      \
	if (((m) & (h)->debug_mask))                      \
		pr_info("ncevf %02x:%02x.%x " s,          \
			(h)->bus.bus_id, (h)->bus.device, \
			(h)->bus.func, ##__VA_ARGS__);    \
} while (0)

#define hw_dbg(h, s, ...)                          \
	pr_debug("ncevf %02x:%02x.%x " s,          \
		 (h)->bus.bus_id, (h)->bus.device, \
		 (h)->bus.func, ##__VA_ARGS__)

extern char                     ne6xvf_driver_name[];
extern const char               ne6xvf_driver_version[];
extern struct workqueue_struct *ne6xvf_wq;

#define ne6xvf_init_spinlock(_sp)        ne6xvf_init_spinlock_d(_sp)
#define ne6xvf_acquire_spinlock(_sp)     ne6xvf_acquire_spinlock_d(_sp)
#define ne6xvf_release_spinlock(_sp)     ne6xvf_release_spinlock_d(_sp)
#define ne6xvf_destroy_spinlock(_sp)     ne6xvf_destroy_spinlock_d(_sp)

#define wr64(a, reg, value)            writeq((value), ((a)->hw_addr0 + (reg)))
#define rd64(a, reg)                   readq((a)->hw_addr0 + (reg))

#define NE6XVF_READ_REG(hw, reg)         rd64(hw, reg)
#define NE6XVF_WRITE_REG(hw, reg, value) wr64(hw, reg, value)

#define NE6XVF_MAX_REQ_QUEUES            32

#define NE6XVF_RESET_WAIT_MS             10
#define NE6XVF_RESET_WAIT_DETECTED_COUNT 50
#define NE6XVF_RESET_WAIT_COMPLETE_COUNT 2000

enum ne6xvf_critical_section_t {
	__NE6XVF_IN_CRITICAL_TASK,	/* cannot be interrupted */
	__NE6XVF_IN_REMOVE_TASK,	/* device being removed */
	__NE6XVF_TX_TSTAMP_IN_PROGRESS,	/* PTP Tx timestamp request in progress */
};

struct ne6xvf_vlan_filter {
	struct list_head list;
	struct ne6x_vf_vlan vlan;
	struct {
		u8 is_new_vlan : 1; /* filter is new, wait for PF answer */
		u8 remove      : 1; /* filter needs to be removed */
		u8 add         : 1; /* filter needs to be added */
		u8 padding     : 5;
	};
};

struct ne6xvf_mac_filter {
	struct list_head list;
	u8 macaddr[ETH_ALEN];
	struct {
		u8 is_new_mac  : 1; /* filter is new, wait for PF decision */
		u8 remove      : 1; /* filter needs to be removed */
		u8 add         : 1; /* filter needs to be added */
		u8 is_primary  : 1; /* filter is a default VF MAC */
		u8 add_handled : 1; /* received response from PF for filter add */
		u8 padding     : 3;
	};
};

/* Driver state. The order of these is important! */
enum ne6xvf_state_t {
	__NE6XVF_STARTUP,		   /* driver loaded, probe complete */
	__NE6XVF_REMOVE,		   /* driver is being unloaded */
	__NE6XVF_INIT_GET_RESOURCES, /* aq msg sent, awaiting reply */
	__NE6XVF_INIT_EXTENDED_CAPS, /* process extended caps which require aq msg exchange */
	__NE6XVF_INIT_CONFIG_ADAPTER,
	__NE6XVF_INIT_SW,	           /* got resources, setting up structs */
	__NE6XVF_INIT_FAILED,        /* init failed, restarting procedure */
	__NE6XVF_RESETTING,          /* in reset */
	__NE6XVF_COMM_FAILED,        /* communication with PF failed */
	/* Below here, watchdog is running */
	__NE6XVF_DOWN,	           /* ready, can be opened */
	__NE6XVF_DOWN_PENDING,       /* descending, waiting for watchdog */
	__NE6XVF_TESTING,	           /* in ethtool self-test */
	__NE6XVF_RUNNING	           /* opened, working */
};

struct ne6xvf_mac_info {
	u8                 addr[ETH_ALEN];
	u8                 perm_addr[ETH_ALEN];
	u8                 san_addr[ETH_ALEN];
	u8                 port_addr[ETH_ALEN];
	u16                max_fcoeq;
};

enum ne6xvf_bus_speed {
	ne6xvf_bus_speed_unknown = 0,
	ne6xvf_bus_speed_33      = 33,
	ne6xvf_bus_speed_66      = 66,
	ne6xvf_bus_speed_100     = 100,
	ne6xvf_bus_speed_120     = 120,
	ne6xvf_bus_speed_133     = 133,
	ne6xvf_bus_speed_2500    = 2500,
	ne6xvf_bus_speed_5000    = 5000,
	ne6xvf_bus_speed_8000    = 8000,
	ne6xvf_bus_speed_reserved
};

enum ne6xvf_bus_width {
	ne6xvf_bus_width_unknown = 0,
	ne6xvf_bus_width_pcie_x1 = 1,
	ne6xvf_bus_width_pcie_x2 = 2,
	ne6xvf_bus_width_pcie_x4 = 4,
	ne6xvf_bus_width_pcie_x8  = 8,
	ne6xvf_bus_width_32      = 32,
	ne6xvf_bus_width_64      = 64,
	ne6xvf_bus_width_reserved
};

enum ne6xvf_bus_type {
	ne6xvf_bus_type_unknown = 0,
	ne6xvf_bus_type_pci,
	ne6xvf_bus_type_pcix,
	ne6xvf_bus_type_pci_express,
	ne6xvf_bus_type_reserved
};

struct ne6xvf_bus_info {
	enum ne6xvf_bus_speed speed;
	enum ne6xvf_bus_width width;
	enum ne6xvf_bus_type type;

	u16 func;
	u16 device;
	u16 lan_id;
	u16 bus_id;
};

struct ne6xvf_hw_capabilities {
	u32 num_vsis;
	u32 num_rx_qp;
	u32 num_tx_qp;
	u32 base_queue;
	u32 num_msix_vectors_vf;
	u32 max_mtu;
	u32 chip_id;
	u32 mac_id;
	u32 lport;
	u32 vf_id;
	u32 num_vf_per_pf;
};

struct ne6xvf_hw {
	u8 __iomem *hw_addr0;
	u8 __iomem *hw_addr2;
	void *back;

	/* subsystem structs */
	struct ne6xvf_mac_info mac;
	struct ne6xvf_bus_info bus;

	/* pci info */
	u16 device_id;
	u16 vendor_id;
	u16 subsystem_device_id;
	u16 subsystem_vendor_id;
	u8 revision_id;

	/* capabilities for entire device and PCI func */
	struct ne6xvf_hw_capabilities dev_caps;

	struct ne6xvf_sdk_mbx_info mbx;

	/* debug mask */
	u32 debug_mask;
	char err_str[16];
};

struct ne6xvf_eth_stats {
	u64 rx_bytes;		 /* gorc */
	u64 rx_unicast;		 /* uprc */
	u64 rx_multicast;	 /* mprc */
	u64 rx_broadcast;	 /* bprc */
	u64 rx_discards;	 /* rdpc */
	u64 rx_unknown_protocol; /* rupp */
	u64 tx_bytes;		 /* gotc */
	u64 tx_unicast;		 /* uptc */
	u64 tx_multicast;	 /* mptc */
	u64 tx_broadcast;	 /* bptc */
	u64 tx_discards;	 /* tdpc */
	u64 tx_errors;		 /* tepc */
};

#define NE6XVF_FLAG_RX_CSUM_ENABLED                BIT(0)
#define NE6XVF_FLAG_PF_COMMS_FAILED                BIT(3)
#define NE6XVF_FLAG_RESET_PENDING                  BIT(4)
#define NE6XVF_FLAG_RESET_NEEDED                   BIT(5)
#define NE6XVF_FLAG_WB_ON_ITR_CAPABLE              BIT(6)
#define NE6XVF_FLAG_PROMISC_ON                     BIT(13)
#define NE6XVF_FLAG_ALLMULTI_ON                    BIT(14)

#define NE6XVF_FLAG_LEGACY_RX                      BIT(15)
#define NE6XVF_FLAG_REINIT_ITR_NEEDED              BIT(16)
#define NE6XVF_FLAG_QUEUES_ENABLED                 BIT(17)
#define NE6XVF_FLAG_QUEUES_DISABLED                BIT(18)
#define NE6XVF_FLAG_REINIT_MSIX_NEEDED             BIT(20)
#define NE6XF_FLAG_REINIT_CHNL_NEEDED              BIT(21)
#define NE6XF_FLAG_RESET_DETECTED                  BIT(22)
#define NE6XF_FLAG_INITIAL_MAC_SET                 BIT(23)

#define NE6XVF_FLAG_AQ_ENABLE_QUEUES               BIT_ULL(0)
#define NE6XVF_FLAG_AQ_ADD_MAC_FILTER              BIT_ULL(2)
#define NE6XVF_FLAG_AQ_ADD_VLAN_FILTER             BIT_ULL(3)
#define NE6XVF_FLAG_AQ_DEL_MAC_FILTER              BIT_ULL(4)
#define NE6XVF_FLAG_AQ_DEL_VLAN_FILTER             BIT_ULL(5)
#define NE6XVF_FLAG_AQ_CONFIGURE_QUEUES            BIT_ULL(6)
#define NE6XVF_FLAG_AQ_MAP_VECTORS                 BIT_ULL(7)
#define NE6XVF_FLAG_AQ_HANDLE_RESET                BIT_ULL(8)
#define NE6XVF_FLAG_AQ_CONFIGURE_RSS               BIT_ULL(9) /* direct AQ config */
#define NE6XVF_FLAG_AQ_GET_CONFIG                  BIT_ULL(10)
/* Newer style, RSS done by the PF so we can ignore hardware vagaries. */
#define NE6XVF_FLAG_AQ_GET_HENA                    BIT_ULL(11)
#define NE6XVF_FLAG_AQ_SET_HENA                    BIT_ULL(12)
#define NE6XVF_FLAG_AQ_SET_RSS_KEY                 BIT_ULL(13)
#define NE6XVF_FLAG_AQ_SET_RSS_LUT                 BIT_ULL(14)
#define NE6XVF_FLAG_AQ_REQUEST_PROMISC             BIT_ULL(15)
#define NE6XVF_FLAG_AQ_RELEASE_PROMISC             BIT_ULL(16)
#define NE6XVF_FLAG_AQ_REQUEST_ALLMULTI            BIT_ULL(17)
#define NE6XVF_FLAG_AQ_RELEASE_ALLMULTI            BIT_ULL(18)

#define NE6XVF_FLAG_AQ_CONFIGURE_HW_OFFLOAD        BIT_ULL(38)
#define NE6XVF_FLAG_AQ_GET_FEATURE                 BIT_ULL(39)
#define NE6XVF_FLAG_AQ_GET_PORT_LINK_STATUS        BIT_ULL(40)
#define NE6XVF_FLAG_AQ_SET_VF_MAC                  BIT_ULL(41)
#define NE6XVF_FLAG_AQ_CHANGED_RSS                 BIT_ULL(42)

struct ne6xvf_adapter {
	struct ne6x_adapt_comm comm;
	struct work_struct    sdk_task;
	struct delayed_work   watchdog_task;
	wait_queue_head_t     down_waitqueue;
	wait_queue_head_t     vc_waitqueue;
	struct ne6x_q_vector *q_vectors;
	struct list_head      vlan_filter_list;
	struct list_head      mac_filter_list;
	struct list_head      macvlan_list;
	/* Lock to protect accesses to MAC and VLAN lists */
	spinlock_t            mac_vlan_list_lock;
	char                  misc_vector_name[IFNAMSIZ + 9];
	u16                   max_queues;
	u16                   num_active_queues;
	u16                   num_req_queues;
	u32                   hw_feature;
	struct ne6x_ring     *tg_rings; /* TG */
	struct ne6x_ring     *cq_rings; /* CQ */
	u32                   cq_desc_count;

	/* TX */
	struct ne6x_ring     *tx_rings;
	u32                   tx_timeout_count;
	u32                   tx_desc_count;

	/* RX */
	struct ne6x_ring     *rx_rings;
	u64                   hw_csum_rx_error;
	u32                   rx_desc_count;
	int                   num_msix_vectors;
	struct msix_entry    *msix_entries;

	u32                   flags;

	/* duplicates for common code */
#define NE6XVF_FLAG_DCB_ENABLED 0

	/* flags for admin queue service task */
	u64                   aq_required;

	/* Lock to prevent possible clobbering of
	 * current_netdev_promisc_flags
	 */
	spinlock_t            current_netdev_promisc_flags_lock;

	netdev_features_t     current_netdev_promisc_flags;

	/* OS defined structs */
	struct net_device    *netdev;
	struct pci_dev       *pdev;

	struct net_device_stats net_stats;

	struct ne6xvf_hw hw; /* defined in ne6xvf.h */

	enum ne6xvf_state_t state;
	enum ne6xvf_state_t last_state;
	unsigned long crit_section;

	bool netdev_registered;
	bool link_up;
	enum ne6x_sdk_link_speed link_speed;
	enum virtchnl_ops current_op;
	struct virtchnl_vf_resource *vf_res;
	struct virtchnl_vsi_resource *vsi_res; /* our LAN VSI */

	struct ne6xvf_eth_stats current_stats;
	//struct ne6xvf_vsi vsi;
	u16 msg_enable;
	struct ne6x_rss_info rss_info;
	u8 trusted;

#ifdef CONFIG_DEBUG_FS
	struct dentry *ne6xvf_dbg_pf;
#endif /* CONFIG_DEBUG_FS */
};

#ifdef CONFIG_DEBUG_FS
#define NCE_DEBUG_CHAR_LEN 1024

struct ne6xvf_dbg_cmd_wr {
	char command[NCE_DEBUG_CHAR_LEN];
	void (*command_proc)(struct ne6xvf_adapter *pf);
};

void ne6xvf_dbg_pf_init(struct ne6xvf_adapter *pf);
void ne6xvf_dbg_pf_exit(struct ne6xvf_adapter *pf);
void ne6xvf_dbg_init(void);
void ne6xvf_dbg_exit(void);

#endif

/* Error Codes */
enum ne6xvf_status {
	NE6XVF_SUCCESS                        = 0,
	NE6XVF_ERR_NVM                        = -1,
	NE6XVF_ERR_NVM_CHECKSUM               = -2,
	NE6XVF_ERR_PHY                        = -3,
	NE6XVF_ERR_CONFIG                     = -4,
	NE6XVF_ERR_PARAM                      = -5,
	NE6XVF_ERR_MAC_TYPE                   = -6,
	NE6XVF_ERR_UNKNOWN_PHY                = -7,
	NE6XVF_ERR_LINK_SETUP                 = -8,
	NE6XVF_ERR_ADAPTER_STOPPED            = -9,
	NE6XVF_ERR_INVALID_MAC_ADDR           = -10,
	NE6XVF_ERR_DEVICE_NOT_SUPPORTED       = -11,
	NE6XVF_ERR_MASTER_REQUESTS_PENDING    = -12,
	NE6XVF_ERR_INVALID_LINK_SETTINGS      = -13,
	NE6XVF_ERR_AUTONEG_NOT_COMPLETE       = -14,
	NE6XVF_ERR_RESET_FAILED               = -15,
	NE6XVF_ERR_SWFW_SYNC                  = -16,
	NE6XVF_ERR_NO_AVAILABLE_VSI           = -17,
	NE6XVF_ERR_NO_MEMORY                  = -18,
	NE6XVF_ERR_BAD_PTR                    = -19,
	NE6XVF_ERR_RING_FULL                  = -20,
	NE6XVF_ERR_INVALID_PD_ID              = -21,
	NE6XVF_ERR_INVALID_QP_ID              = -22,
	NE6XVF_ERR_INVALID_CQ_ID              = -23,
	NE6XVF_ERR_INVALID_CEQ_ID             = -24,
	NE6XVF_ERR_INVALID_AEQ_ID             = -25,
	NE6XVF_ERR_INVALID_SIZE               = -26,
	NE6XVF_ERR_INVALID_ARP_INDEX          = -27,
	NE6XVF_ERR_INVALID_FPM_FUNC_ID        = -28,
	NE6XVF_ERR_QP_INVALID_MSG_SIZE        = -29,
	NE6XVF_ERR_QP_TOOMANY_WRS_POSTED      = -30,
	NE6XVF_ERR_INVALID_FRAG_COUNT         = -31,
	NE6XVF_ERR_QUEUE_EMPTY                = -32,
	NE6XVF_ERR_INVALID_ALIGNMENT          = -33,
	NE6XVF_ERR_FLUSHED_QUEUE              = -34,
	NE6XVF_ERR_INVALID_PUSH_PAGE_INDEX    = -35,
	NE6XVF_ERR_INVALID_IMM_DATA_SIZE      = -36,
	NE6XVF_ERR_TIMEOUT                    = -37,
	NE6XVF_ERR_OPCODE_MISMATCH            = -38,
	NE6XVF_ERR_CQP_COMPL_ERROR            = -39,
	NE6XVF_ERR_INVALID_VF_ID              = -40,
	NE6XVF_ERR_INVALID_HMCFN_ID           = -41,
	NE6XVF_ERR_BACKING_PAGE_ERROR         = -42,
	NE6XVF_ERR_NO_PBLCHUNKS_AVAILABLE     = -43,
	NE6XVF_ERR_INVALID_PBLE_INDEX         = -44,
	NE6XVF_ERR_INVALID_SD_INDEX           = -45,
	NE6XVF_ERR_INVALID_PAGE_DESC_INDEX    = -46,
	NE6XVF_ERR_INVALID_SD_TYPE            = -47,
	NE6XVF_ERR_MEMCPY_FAILED              = -48,
	NE6XVF_ERR_INVALID_HMC_OBJ_INDEX      = -49,
	NE6XVF_ERR_INVALID_HMC_OBJ_COUNT      = -50,
	NE6XVF_ERR_INVALID_SRQ_ARM_LIMIT      = -51,
	NE6XVF_ERR_SRQ_ENABLED                = -52,
	NE6XVF_ERR_ADMIN_QUEUE_ERROR          = -53,
	NE6XVF_ERR_ADMIN_QUEUE_TIMEOUT        = -54,
	NE6XVF_ERR_BUF_TOO_SHORT              = -55,
	NE6XVF_ERR_ADMIN_QUEUE_FULL           = -56,
	NE6XVF_ERR_ADMIN_QUEUE_NO_WORK        = -57,
	NE6XVF_ERR_BAD_IWARP_CQE              = -58,
	NE6XVF_ERR_NVM_BLANK_MODE             = -59,
	NE6XVF_ERR_NOT_IMPLEMENTED            = -60,
	NE6XVF_ERR_PE_DOORBELL_NOT_ENABLED    = -61,
	NE6XVF_ERR_DIAG_TEST_FAILED           = -62,
	NE6XVF_ERR_NOT_READY                  = -63,
	NE6XVF_NOT_SUPPORTED                  = -64,
	NE6XVF_ERR_FIRMWARE_API_VERSION       = -65,
	NE6XVF_ERR_ADMIN_QUEUE_CRITICAL_ERROR = -66,
};

static inline const char *ne6xvf_state_str(enum ne6xvf_state_t state)
{
	switch (state) {
	case __NE6XVF_STARTUP:
		return "__NE6XVF_STARTUP";
	case __NE6XVF_REMOVE:
		return "__NE6XVF_REMOVE";
	case __NE6XVF_INIT_GET_RESOURCES:
		return "__NE6XVF_INIT_GET_RESOURCES";
	case __NE6XVF_INIT_EXTENDED_CAPS:
		return "__NE6XVF_INIT_EXTENDED_CAPS";
	case __NE6XVF_INIT_CONFIG_ADAPTER:
		return "__NE6XVF_INIT_CONFIG_ADAPTER";
	case __NE6XVF_INIT_SW:
		return "__NE6XVF_INIT_SW";
	case __NE6XVF_INIT_FAILED:
		return "__NE6XVF_INIT_FAILED";
	case __NE6XVF_RESETTING:
		return "__NE6XVF_RESETTING";
	case __NE6XVF_COMM_FAILED:
		return "__NE6XVF_COMM_FAILED";
	case __NE6XVF_DOWN:
		return "__NE6XVF_DOWN";
	case __NE6XVF_DOWN_PENDING:
		return "__NE6XVF_DOWN_PENDING";
	case __NE6XVF_TESTING:
		return "__NE6XVF_TESTING";
	case __NE6XVF_RUNNING:
		return "__NE6XVF_RUNNING";
	default:
		return "__NE6XVF_UNKNOWN_STATE";
	}
}

static inline void ne6xvf_change_state(struct ne6xvf_adapter *adapter, enum ne6xvf_state_t state)
{
	if (adapter->state != state) {
		adapter->last_state = adapter->state;
		adapter->state = state;
	}
}

static inline struct device *pci_dev_to_dev(struct pci_dev *pdev)
{
	return &pdev->dev;
}

int ne6xvf_send_api_ver(struct ne6xvf_adapter *adapter);
int ne6xvf_send_vf_config_msg(struct ne6xvf_adapter *adapter, bool b_init);
int ne6xvf_send_vf_offload_msg(struct ne6xvf_adapter *adapter);
int ne6xvf_send_vf_feature_msg(struct ne6xvf_adapter *adapter);
int ne6xvf_get_vf_config(struct ne6xvf_adapter *adapter);
int ne6xvf_request_reset(struct ne6xvf_adapter *adapter);
void ne6xvf_free_all_tg_resources(struct ne6xvf_adapter *adapter);
void ne6xvf_free_all_cq_resources(struct ne6xvf_adapter *adapter);
void ne6xvf_free_all_tx_resources(struct ne6xvf_adapter *adapter);
void ne6xvf_free_all_rx_resources(struct ne6xvf_adapter *adapter);
void ne6xvf_reset_interrupt_capability(struct ne6xvf_adapter *adapter);
bool ne6x_alloc_rx_buffers(struct ne6x_ring *rx_ring, u16 cleaned_count);
void ne6xvf_set_ethtool_ops(struct net_device *netdev);
void ne6xvf_request_stats(struct ne6xvf_adapter *adapter);
void ne6xvf_irq_enable(struct ne6xvf_adapter *adapter, bool flush);
int ne6xvf_get_vf_feature(struct ne6xvf_adapter *adapter);
enum ne6xvf_status ne6xvf_clean_arq_element(struct ne6xvf_hw *hw, struct ne6xvf_arq_event_info *e,
					    u16 *pending);
void ne6xvf_virtchnl_completion(struct ne6xvf_adapter *adapter, enum virtchnl_ops v_opcode,
				enum ne6xvf_status v_retval, u8 *msg, u16 msglen);
int ne6xvf_get_vf_feature(struct ne6xvf_adapter *adapter);
int ne6xvf_request_feature(struct ne6xvf_adapter *adapter);
int ne6xvf_config_default_vlan(struct ne6xvf_adapter *adapter);
void ne6xvf_config_rss_info(struct ne6xvf_adapter *adapter);
void ne6xvf_changed_rss(struct ne6xvf_adapter *adapter);

void ne6xvf_add_vlans(struct ne6xvf_adapter *adapter);
void ne6xvf_del_vlans(struct ne6xvf_adapter *adapter);
void ne6xvf_schedule_reset(struct ne6xvf_adapter *adapter);
int ne6xvf_parse_vf_resource_msg(struct ne6xvf_adapter *adapter);
int ne6xvf_request_queues(struct ne6xvf_adapter *adapter, int num);
void ne6xvf_add_ether_addrs(struct ne6xvf_adapter *adapter);
void ne6xvf_del_ether_addrs(struct ne6xvf_adapter *adapter);
void ne6xvf_set_promiscuous(struct ne6xvf_adapter *adapter);
int ne6xvf_poll_virtchnl_msg(struct ne6xvf_adapter *adapter, struct ne6xvf_arq_event_info *event,
			     enum virtchnl_ops op_to_poll);
int ne6xvf_enable_queues(struct ne6xvf_adapter *adapter);
void ne6xvf_update_pf_stats(struct ne6xvf_adapter *adapter);
int ne6xvf_send_pf_msg(struct ne6xvf_adapter *adapter, enum virtchnl_ops op, u8 *msg, u16 len);
void ne6xvf_vchanel_get_port_link_status(struct ne6xvf_adapter *adapter);
void ne6xvf_set_vf_addr(struct ne6xvf_adapter *adapter);
int ne6xvf_close(struct net_device *netdev);
int ne6xvf_open(struct net_device *netdev);
void ne6xvf_fill_rss_lut(struct ne6xvf_adapter *adapter);
void ne6xvf_tail_update(struct ne6x_ring *ring, int val);
int ne6xvf_register_netdev(struct ne6xvf_adapter *adapter);

#endif /* _NE6XVF_H */
