
#ifndef __SXE_FILTER_H__
#define __SXE_FILTER_H__

#include <net/ipv6.h>
#include <net/ip.h>

#include "sxe.h"

#define  SXE_DEFAULT_UC_ADDR_IDX    (0)

#define  SXE_DEFAULT_MAC_POOL_IDX   (0)
#define  SXE_UC_ADDR_ENTRY_USED     (0x1)

#define SXE_FNAV_DEFAULT_SAMPLE_RATE  (200)

#define SXE_FNAV_RULES_TABLE_PKT_SIZE	(32)

enum sxe_fnav_rules_table_size {
	SXE_FNAV_RULES_TABLE_SIZE_NONE = 0,
	SXE_FNAV_RULES_TABLE_SIZE_64K  = 1,
	SXE_FNAV_RULES_TABLE_SIZE_128K = 2,
	SXE_FNAV_RULES_TABLE_SIZE_256K = 3,
};

union sxe_sample_data_hdr {
	unsigned char *network;
	struct iphdr *ipv4;
	struct ipv6hdr *ipv6;
};

s32 sxe_uc_addr_add(struct sxe_hw *hw,struct sxe_uc_addr_table *uc_table,
				const u8 *addr, u16 pool);

s32 sxe_uc_addr_del(struct sxe_hw *hw,struct sxe_uc_addr_table *uc_table,
				const u8 *addr, u16 pool);

s32 sxe_mc_addr_add(struct net_device *netdev);

s32 sxe_mac_filter_init(struct sxe_adapter *adapter);

void sxe_mac_filter_destroy(struct sxe_adapter *adapter);

s32 sxe_uc_sync(struct net_device *netdev, const u8 *addr);

s32 sxe_uc_unsync(struct net_device *netdev, const u8 *addr);

void sxe_mac_filter_reset(struct sxe_adapter *adapter);

void sxe_mac_addr_set(struct sxe_adapter *adapter);

void sxe_fnav_rules_restore(struct sxe_adapter *adapter);

void sxe_fnav_rules_clean(struct sxe_adapter *adapter);

s32 sxe_fnav_sample_rule_get(struct sxe_ring *ring,
		      struct sxe_tx_buffer *first_buffer);

s32 sxe_fnav_specific_rule_add_process(
					struct sxe_adapter *adapter,
					struct sxe_fnav_rule_node *input_rule,
					union  sxe_fnav_rule_info *mask,
					u8 queue);

int sxe_fnav_sw_specific_rule_del(
					struct sxe_adapter *adapter,
					u16 sw_idx);

u64 sxe_fnav_max_rule_num_get(u32 rules_table_size);

struct sxe_fnav_rule_node* sxe_fnav_specific_rule_find(
						struct sxe_adapter *adapter,
						u32 location);

void sxe_fc_mac_addr_set(struct sxe_adapter *adapter);

void sxe_vf_mc_addr_restore(struct sxe_adapter *adapter);

#endif
