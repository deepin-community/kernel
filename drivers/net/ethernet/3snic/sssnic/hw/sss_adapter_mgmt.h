/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_ADAPTER_MGMT_H
#define SSS_ADAPTER_MGMT_H

#include <linux/types.h>
#include <linux/bitops.h>

#include "sss_version.h"
#include "sss_adapter.h"

#define SSS_DRV_VERSION SSS_VERSION_STR

#define SSS_DRV_NAME	"sssnic"
#define SSS_CHIP_NAME	"sssnic"

#define SSS_VF_PCI_CFG_REG_BAR	0
#define SSS_PF_PCI_CFG_REG_BAR	1

#define SSS_PCI_INTR_REG_BAR	2
#define SSS_PCI_MGMT_REG_BAR	3 /* Only PF have mgmt bar */
#define SSS_PCI_DB_BAR			4

#define SSS_IS_VF_DEV(pdev)		((pdev)->device == SSS_DEV_ID_VF)

#define SSS_CARD_MAX_SIZE (64)

struct sss_card_id {
	u32 id[SSS_CARD_MAX_SIZE];
	u32 num;
};

struct sss_func_pdev_info {
	u64 bar0_pa;
	u64 bar0_size;
	u64 bar1_pa;
	u64 bar1_size;
	u64 bar3_pa;
	u64 bar3_size;
	u64 rsvd[4];
};

struct sss_card_func_info {
	u32 pf_num;
	u32 rsvd;
	u64 usr_adm_pa;
	struct sss_func_pdev_info pdev_info[SSS_CARD_MAX_SIZE];
};

enum {
	SSS_NO_PROBE = 1,
	SSS_PROBE_START = 2,
	SSS_PROBE_OK = 3,
	SSS_IN_REMOVE = 4,
};

struct list_head *sss_get_chip_list(void);
int sss_alloc_chip_node(struct sss_pci_adapter *adapter);
void sss_free_chip_node(struct sss_pci_adapter *adapter);
void sss_pre_init(void);
struct sss_pci_adapter *sss_get_adapter_by_pcidev(struct pci_dev *pdev);
void sss_add_func_list(struct sss_pci_adapter *adapter);
void sss_del_func_list(struct sss_pci_adapter *adapter);
void sss_hold_chip_node(void);
void sss_put_chip_node(void);

void sss_set_adapter_probe_state(struct sss_pci_adapter *adapter, int state);

void lld_dev_hold(struct sss_hal_dev *dev);
void lld_dev_put(struct sss_hal_dev *dev);

void sss_chip_node_lock(void);
void sss_chip_node_unlock(void);

void *sss_get_pcidev_hdl(void *hwdev);
void *sss_get_uld_dev(struct sss_hal_dev *hal_dev, enum sss_service_type type);

void sss_uld_dev_put(struct sss_hal_dev *hal_dev, enum sss_service_type type);

struct sss_hal_dev *sss_get_lld_dev_by_dev_name(const char *dev_name, enum sss_service_type type);

struct sss_hal_dev *sss_get_lld_dev_by_chip_name(const char *chip_name);

struct sss_hal_dev *sss_get_lld_dev_by_chip_and_port(const char *chip_name, u8 port_id);

void sss_get_all_chip_id(void *id_info);

void sss_get_card_func_info
	(const char *chip_name, struct sss_card_func_info *card_func);

void sss_get_card_info(const void *hwdev, void *bufin);

bool sss_is_in_host(void);

int sss_get_pf_id(struct sss_card_node *chip_node, u32 port_id, u32 *pf_id, u32 *valid);

struct sss_card_node *sss_get_card_node(struct sss_hal_dev *hal_dev);

#endif
