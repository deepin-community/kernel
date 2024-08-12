/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_H
#define _NE6X_H

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/aer.h>
#include <linux/netdevice.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/hash.h>
#include <linux/string.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/pkt_sched.h>
#include <linux/ipv6.h>
#include <net/checksum.h>
#include <linux/in6.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/if_ether.h>
#include <net/ip6_checksum.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>
#include <linux/if_bridge.h>
#include <linux/iommu.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/pci_hotplug.h>

#include "reg.h"
#include "feature.h"
#include "txrx.h"
#include "common.h"
#include "ne6x_txrx.h"
#include "ne6x_ethtool.h"
#include "ne6x_procfs.h"
#include "ne6x_virtchnl_pf.h"
#include "version.h"

#define NE6X_MAX_VP_NUM        64
#define NE6X_PF_VP0_NUM        64
#define NE6X_PF_VP1_NUM        65
#define NE6X_MAILBOX_VP_NUM    NE6X_PF_VP0_NUM
#define NE6X_MAX_MSIX_NUM      72
#define NE6X_MIN_MSIX          2

#define NE6X_NIC_INT_VP        71
#define NE6X_NIC_INT_START_BIT 42

#define wr64(a, reg, value) \
	writeq((value), ((void __iomem *)((a)->hw_addr0) + (reg)))
#define rd64(a, reg) \
	readq((void __iomem *)((a)->hw_addr0) + (reg))
#define wr64_bar4(a, reg, value) \
	writeq((value), ((void __iomem *)((a)->hw_addr4) + (reg)))
#define rd64_bar4(a, reg) \
	readq((void __iomem *)((a)->hw_addr4) + (reg))

#define ne6x_pf_to_dev(pf)		(&((pf)->pdev->dev))
#define ne6x_get_vf_by_id(pf, vf_id)	(&((pf)->vf[vf_id]))

#define ADPT_PPORT(adpt)             ((adpt)->port_info->hw_port_id)
#define ADPT_LPORT(adpt)             ((adpt)->port_info->lport)
#define ADPT_VPORT(adpt)             ((adpt)->vport)
#define ADPT_VPORTCOS(adpt)          ((adpt)->base_queue + 160)

enum ne6x_adapter_type {
	NE6X_ADPT_PF = 0,
	NE6X_ADPT_VF,
};

enum ne6x_adapter_flags {
	NE6X_ADPT_F_DISABLE_FW_LLDP,
	NE6X_ADPT_F_LINKDOWN_ON_CLOSE,
	NE6X_ADPT_F_NORFLASH_WRITE_PROTECT,
	NE6X_ADPT_F_DDOS_SWITCH,
	NE6X_ADPT_F_ACL,
	NE6X_ADPT_F_TRUST_VLAN,
	NE6X_ADPT_F_NBITS /* must be last */
};

enum ne6x_pf_state {
	NE6X_TESTING,
	NE6X_DOWN,
	NE6X_SERVICE_SCHED,
	NE6X_INT_INIT_DOWN,
	NE6X_CLIENT_SERVICE_REQUESTED,
	NE6X_LINK_POOLING,
	NE6X_CONFIG_BUSY,
	NE6X_TIMEOUT_RECOVERY_PENDING,
	NE6X_PF_RESET_REQUESTED,
	NE6X_CORE_RESET_REQUESTED,
	NE6X_GLOBAL_RESET_REQUESTED,
	NE6X_RESET_INTR_RECEIVED,
	NE6X_DOWN_REQUESTED,
	NE6X_VF_DIS,
	NE6X_MAILBOXQ_EVENT_PENDING,
	NE6X_PF_INTX,
	NE6X_PF_MSI,
	NE6X_PF_MSIX,
	NE6X_FLAG_SRIOV_ENA,
	NE6X_REMOVE,
	NE6X_STATE_NBITS /* must be last */
};

enum {
	NE6X_ETHTOOL_FLASH_810_LOADER = 0,
	NE6X_ETHTOOL_FLASH_810_APP    = 1,
	NE6X_ETHTOOL_FLASH_807_APP    = 2,
	NE6X_ETHTOOL_FLASH_NP         = 3,
	NE6X_ETHTOOL_FLASH_PXE        = 4,
	NE6X_ETHTOOL_FRU              = 0xf2,
};

/* MAC addr list head node struct */
struct mac_addr_head {
	struct list_head list;
	struct mutex mutex; /* mutex */
};

/* MAC addr list node struct */
struct mac_addr_node {
	struct list_head list;
	u8 addr[32];
};

/* values for UPT1_RSSConf.hashFunc */
enum {
	NE6X_FW_VER_NORMAL = 0x0,
	NE6X_FW_VER_WHITELIST = 0x100,
};

struct ne6x_lump_tracking {
	u16 num_entries;
	u16 list[];
};

struct ne6x_hw_port_stats {
	u64 mac_rx_eth_byte;
	u64 mac_rx_eth;
	u64 mac_rx_eth_undersize;
	u64 mac_rx_eth_crc;
	u64 mac_rx_eth_64b;
	u64 mac_rx_eth_65_127b;
	u64 mac_rx_eth_128_255b;
	u64 mac_rx_eth_256_511b;
	u64 mac_rx_eth_512_1023b;
	u64 mac_rx_eth_1024_15360b;
	u64 mac_tx_eth_byte;
	u64 mac_tx_eth;
	u64 mac_tx_eth_undersize;
	u64 mac_tx_eth_64b;
	u64 mac_tx_eth_65_127b;
	u64 mac_tx_eth_128_255b;
	u64 mac_tx_eth_256_511b;
	u64 mac_tx_eth_512_1023b;
	u64 mac_tx_eth_1024_15360b;
};

/* struct that defines a adapter, associated with a dev */
struct ne6x_adapter {
	struct ne6x_adapt_comm comm;
	struct net_device    *netdev;
	struct ne6x_pf        *back;      /* back pointer to PF */
	struct ne6x_port_info *port_info; /* back pointer to port_info */
	struct ne6x_ring     **rx_rings;  /* Rx ring array */
	struct ne6x_ring     **tx_rings;  /* Tx ring array */
	struct ne6x_ring     **cq_rings;  /* Tx ring array */
	struct ne6x_ring     **tg_rings;  /* Tx tag ring array */
	struct ne6x_q_vector **q_vectors; /* q_vector array */

	/* used for loopback test */
	char                 *send_buffer;
	wait_queue_head_t     recv_notify;
	u8  recv_done;

	irqreturn_t (*irq_handler)(int irq, void *data);

	u32               tx_restart;
	u32               tx_busy;
	u32               rx_buf_failed;
	u32               rx_page_failed;
	u16               num_q_vectors;
	u16               base_vector;  /* IRQ base for OS reserved vectors */
	enum ne6x_adapter_type type;
	struct ne6x_vf     *vf;          /* VF associated with this adapter */
	u16               idx;          /* software index in pf->adpt[] */
	u16 max_frame;
	u16 rx_buf_len;
	struct rtnl_link_stats64 net_stats;
	struct rtnl_link_stats64 net_stats_offsets;
	struct ne6x_eth_stats eth_stats;
	struct ne6x_eth_stats eth_stats_offsets;
	struct ne6x_rss_info  rss_info;
	int rss_size;

	bool irqs_ready;
	bool current_isup; /* Sync 'link up' logging */
	u16  current_speed;
	u16 vport;
	u16 num_queue;  /* Used queues */
	u16 base_queue; /* adapter's first queue in hw array */
	u16 num_tx_desc;
	u16 num_rx_desc;
	u16 num_cq_desc;
	u16 num_tg_desc;

	u32  hw_feature;
	bool netdev_registered;

	/* unicast MAC head node */
	struct mac_addr_head uc_mac_addr;
	/* multicast MAC head node */
	struct mac_addr_head mc_mac_addr;

	struct work_struct set_rx_mode_task;

	struct ne6x_hw_port_stats stats;
	DECLARE_BITMAP(flags, NE6X_ADPT_F_NBITS);

	struct list_head vlan_filter_list;
	struct list_head  macvlan_list;
	/* Lock to protect accesses to MAC and VLAN lists */
	spinlock_t mac_vlan_list_lock;

	/* aRFS members only allocated for the PF ADPT */
#define NE6X_MAX_RFS_FILTERS	0xFFFF
#define NE6X_MAX_ARFS_LIST	1024
#define NE6X_ARFS_LST_MASK	(NE6X_MAX_ARFS_LIST - 1)
	struct hlist_head *arfs_fltr_list;
	struct ne6x_arfs_active_fltr_cntrs *arfs_fltr_cntrs;
	spinlock_t arfs_lock;	/* protects aRFS hash table and filter state */
	atomic_t *arfs_last_fltr_id;
} ____cacheline_internodealigned_in_smp;

struct ne6x_dev_eeprom_info {
	u8  vendor_id[3];
	u8  ocp_record_version;
	u8  max_power_s0;
	u8  max_power_s5;
	u8  hot_card_cooling_passive_tier;
	u8  cold_card_cooling_passive_tier;
	u8  cooling_mode;
	u16 hot_standby_airflow_require;
	u16 cold_standby_airflow_require;
	u8  uart_configuration_1;
	u8  uart_configuration_2;
	u8  usb_present;
	u8  manageability_type;
	u8  fru_write_protection;
	u8  prog_mode_power_state_supported;
	u8  hot_card_cooling_active_tier;
	u8  cold_card_cooling_active_tier;
	u8  transceiver_ref_power_Level;
	u8  transceiver_ref_temp_Level;
	u8  card_thermal_tier_with_local_fan_fail;
	u16 product_mode;
	u8  is_pcie_exist;
	u32 logic_port_to_phyical;
	u8  resv[3];
	u8  number_of_physical_controllers;
	u8  control_1_udid[16];
	u8  control_2_udid[16];
	u8  control_3_udid[16];
	u8  control_4_udid[16];
	u32 hw_feature;
	u32 hw_flag;
	u8  port_0_mac[6];
	u8  port_1_mac[6];
	u8  port_2_mac[6];
	u8  port_3_mac[6];
	u8  rsv[9];
	u32 spd_verify_value;
} __packed;

struct ne6x_hw {
	u64 __iomem *hw_addr0;
	u64 __iomem *hw_addr2;
	u64 __iomem *hw_addr4;

	struct ne6x_port_info *port_info;

	/* pci info */
	u16 device_id;
	u16 vendor_id;
	u16 subsystem_device_id;
	u16 subsystem_vendor_id;
	u8 revision_id;
	u8 dvm_ena; /* double vlan  enable */
	struct ne6x_pf *back;
	struct ne6x_bus_info bus;
	u16 pf_port;

	u32 expect_vp;
	u32 max_queue;

	struct ne6x_mbx_snapshot mbx_snapshot;
	u8 ne6x_mbx_ready_to_send[64];
};

#define ne6x_hw_to_dev(ptr) (&(container_of((ptr), struct ne6x_pf, hw))->pdev->dev)

struct ne6x_firmware_ver_info {
	u32 firmware_soc_ver;
	u32 firmware_np_ver;
	u32 firmware_pxe_ver;
};

/* struct that defines the Ethernet device */
struct ne6x_pf {
	struct pci_dev *pdev;

	/* OS reserved IRQ details */
	struct msix_entry *msix_entries;
	u16 ctrl_adpt_idx; /* control adapter index in pf->adpt array */

	struct ne6x_adapter **adpt; /* adapters created by the driver */

	struct mutex       switch_mutex; /* switch_mutex */
	struct mutex       mbus_comm_mutex; /* mbus_comm_mutex */
	struct timer_list  serv_tmr;
	struct timer_list  linkscan_tmr;
	unsigned long      service_timer_period;
	struct work_struct serv_task;
	struct work_struct linkscan_work;

	/* Virtchnl/SR-IOV config info */
	struct ne6x_vf *vf;
	u16            num_alloc_vfs;
	u16            num_qps_per_vf;

	u16 next_adpt;     /* Next free slot in pf->adpt[] - 0-based! */
	u16 num_alloc_adpt;

	DECLARE_BITMAP(state, NE6X_STATE_NBITS);

	u32           tx_timeout_count;
	u32           tx_timeout_recovery_level;
	unsigned long tx_timeout_last_recovery;
	struct ne6x_firmware_ver_info verinfo;
	struct ne6x_dev_eeprom_info sdk_spd_info;

	struct ne6x_hw             hw;
	struct ne6x_lump_tracking *irq_pile;
#ifdef CONFIG_DEBUG_FS
	struct dentry *ne6x_dbg_pf;
	struct dentry *ne6x_dbg_info_pf;
#endif /* CONFIG_DEBUG_FS */
	struct proc_dir_entry *ne6x_proc_pf;
	struct list_head       key_filter_list;
	spinlock_t             key_list_lock; /* Lock to protect accesses to key filter */

	char link_intname[NE6X_INT_NAME_STR_LEN];
	char mailbox_intname[NE6X_INT_NAME_STR_LEN];
	bool link_int_irq_ready;
	bool mailbox_int_irq_ready;
	bool is_fastmode;
	u32  hw_flag;
	u32  dump_info;
	u16  dev_type;
};

static inline void ne6x_adpt_setup_irqhandler(struct ne6x_adapter *adpt,
					      irqreturn_t (*irq_handler)(int, void *))
{
	adpt->irq_handler = irq_handler;
}

struct ne6x_netdev_priv {
	struct ne6x_adapter *adpt;
};

static inline bool ne6x_is_supported_port_vlan_proto(struct ne6x_hw *hw,
						     u16 vlan_proto)
{
	bool is_supported = false;

	switch (vlan_proto) {
	case ETH_P_8021Q:
		is_supported = true;
		break;
	case ETH_P_8021AD:
		if (hw->dvm_ena)
			is_supported = true;
		break;
	}

	return is_supported;
}

static inline struct ne6x_pf *ne6x_netdev_to_pf(struct net_device *netdev)
{
	struct ne6x_netdev_priv *np = netdev_priv(netdev);

	return np->adpt->back;
}

static inline struct ne6x_adapter *ne6x_netdev_to_adpt(struct net_device *netdev)
{
	struct ne6x_netdev_priv *np = netdev_priv(netdev);

	return np->adpt;
}

#define NE6X_VLAN(tpid, vid, prio) \
	((struct ne6x_vlan){ tpid, vid, prio })

struct rtnl_link_stats64 *ne6x_get_adpt_stats_struct(struct ne6x_adapter *adpt);

void ne6x_switch_pci_write(void *bar_base, u32 base_addr, u32 offset_addr, u64 reg_value);
u64  ne6x_switch_pci_read(void *bar_base, u32 base_addr, u32 offset_addr);
int  ne6x_adpt_restart_vp(struct ne6x_adapter *adpt, bool enable);
void ne6x_update_pf_stats(struct ne6x_adapter *adpt);
void ne6x_service_event_schedule(struct ne6x_pf *pf);

void ne6x_down(struct ne6x_adapter *adpt);
int  ne6x_up(struct ne6x_adapter *adpt);
int  ne6x_adpt_configure(struct ne6x_adapter *adpt);
void ne6x_adpt_close(struct ne6x_adapter *adpt);

int  ne6x_alloc_rings(struct ne6x_adapter *adpt);
int  ne6x_adpt_configure_tx(struct ne6x_adapter *adpt);
int  ne6x_adpt_configure_rx(struct ne6x_adapter *adpt);
int  ne6x_adpt_configure_cq(struct ne6x_adapter *adpt);
void ne6x_adpt_clear_rings(struct ne6x_adapter *adpt);
int  ne6x_adpt_setup_tx_resources(struct ne6x_adapter *adpt);
int  ne6x_adpt_setup_rx_resources(struct ne6x_adapter *adpt);

int  ne6x_close(struct net_device *netdev);
int  ne6x_open(struct net_device *netdev);
int  ne6x_adpt_open(struct ne6x_adapter *adpt);
int  ne6x_adpt_mem_alloc(struct ne6x_pf *pf, struct ne6x_adapter *adpt);
void ne6x_adpt_map_rings_to_vectors(struct ne6x_adapter *adpt);
void ne6x_adpt_reset_stats(struct ne6x_adapter *adpt);
void ne6x_adpt_free_arrays(struct ne6x_adapter *adpt, bool free_qvectors);
int  ne6x_adpt_register_netdev(struct ne6x_adapter *adpt);
bool netif_is_ne6x(struct net_device *dev);

int ne6x_validata_tx_rate(struct ne6x_adapter *adpt, int vf_id, int min_tx_rate, int max_tx_rate);

int ne6x_del_vlan_list(struct ne6x_adapter *adpt, struct ne6x_vlan vlan);
struct ne6x_vlan_filter *ne6x_add_vlan_list(struct ne6x_adapter *adpt, struct ne6x_vlan vlan);

struct ne6x_key_filter *ne6x_add_key_list(struct ne6x_pf *pf, struct ne6x_key key);
int ne6x_del_key_list(struct ne6x_pf *pf, struct ne6x_key key);
int ne6x_add_key(struct ne6x_adapter *adpt, u8 *mac_addr, u8 size);
int ne6x_del_key(struct ne6x_adapter *adpt, u8 *mac_addr, u8 size);

int ne6x_adpt_add_vlan(struct ne6x_adapter *adpt, struct ne6x_vlan vlan);
int ne6x_adpt_del_vlan(struct ne6x_adapter *adpt, struct ne6x_vlan vlan);

void ne6x_sync_features(struct net_device *netdev);

int ne6x_adpt_add_mac(struct ne6x_adapter *adpt, const u8 *addr, bool is_unicast);
int ne6x_adpt_del_mac(struct ne6x_adapter *adpt, const u8 *addr, bool is_unicast);

int ne6x_adpt_clear_mac_vlan(struct ne6x_adapter *adpt);
void ne6x_adpt_clear_ddos(struct ne6x_pf *pf);
void ne6x_linkscan_schedule(struct ne6x_pf *pf);

ssize_t ne6x_proc_tps_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos);

#endif
