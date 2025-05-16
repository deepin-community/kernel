/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_HWIF_H
#define HINIC3_HWIF_H

#include "hinic3_hwdev.h"

#define HINIC3_PCIE_LINK_DOWN		0xFFFFFFFF

struct hinic3_free_db_area {
	unsigned long		*db_bitmap_array;
	u32			db_max_areas;
	/* spinlock for allocating doorbell area */
	spinlock_t		idx_lock;
};

struct hinic3_func_attr {
	u16			func_global_idx;
	u8			port_to_port_idx;
	u8			pci_intf_idx;
	u8			vf_in_pf;
	u8			rsvd1;
	u16			rsvd2;
	enum func_type		func_type;

	u8			mpf_idx;

	u8			ppf_idx;

	u16			num_irqs; /* max: 2 ^ 15 */
	u8			num_aeqs; /* max: 2 ^ 3 */
	u8			num_ceqs; /* max: 2 ^ 7 */

	u16			num_sq; /* max: 2 ^ 8 */
	u8			num_dma_attr; /* max: 2 ^ 6 */
	u8			msix_flex_en;

	u16			global_vf_id_of_pf;
};

struct hinic3_hwif {
	u8 __iomem			*cfg_regs_base;
	u8 __iomem			*intr_regs_base;
	u8 __iomem			*mgmt_regs_base;
	u64				db_base_phy;
	u64				db_dwqe_len;
	u8 __iomem			*db_base;

	struct hinic3_free_db_area	free_db_area;

	struct hinic3_func_attr		attr;

	void				*pdev;
	u64				rsvd;
};

enum hinic3_outbound_ctrl {
	ENABLE_OUTBOUND  = 0x0,
	DISABLE_OUTBOUND = 0x1,
};

enum hinic3_doorbell_ctrl {
	ENABLE_DOORBELL  = 0x0,
	DISABLE_DOORBELL = 0x1,
};

enum hinic3_pf_status {
	HINIC3_PF_STATUS_INIT = 0X0,
	HINIC3_PF_STATUS_ACTIVE_FLAG = 0x11,
	HINIC3_PF_STATUS_FLR_START_FLAG = 0x12,
	HINIC3_PF_STATUS_FLR_FINISH_FLAG = 0x13,
};

#define HINIC3_HWIF_NUM_AEQS(hwif)		((hwif)->attr.num_aeqs)
#define HINIC3_HWIF_NUM_CEQS(hwif)		((hwif)->attr.num_ceqs)
#define HINIC3_HWIF_NUM_IRQS(hwif)		((hwif)->attr.num_irqs)
#define HINIC3_HWIF_GLOBAL_IDX(hwif)		((hwif)->attr.func_global_idx)
#define HINIC3_HWIF_GLOBAL_VF_OFFSET(hwif) ((hwif)->attr.global_vf_id_of_pf)
#define HINIC3_HWIF_PPF_IDX(hwif)		((hwif)->attr.ppf_idx)
#define HINIC3_PCI_INTF_IDX(hwif)		((hwif)->attr.pci_intf_idx)

#define HINIC3_FUNC_TYPE(dev)		((dev)->hwif->attr.func_type)
#define HINIC3_IS_PF(dev)		(HINIC3_FUNC_TYPE(dev) == TYPE_PF)
#define HINIC3_IS_VF(dev)		(HINIC3_FUNC_TYPE(dev) == TYPE_VF)
#define HINIC3_IS_PPF(dev)		(HINIC3_FUNC_TYPE(dev) == TYPE_PPF)

u32 hinic3_hwif_read_reg(struct hinic3_hwif *hwif, u32 reg);

void hinic3_hwif_write_reg(struct hinic3_hwif *hwif, u32 reg, u32 val);

void hinic3_set_pf_status(struct hinic3_hwif *hwif,
			  enum hinic3_pf_status status);

enum hinic3_pf_status hinic3_get_pf_status(struct hinic3_hwif *hwif);

void hinic3_disable_doorbell(struct hinic3_hwif *hwif);

void hinic3_enable_doorbell(struct hinic3_hwif *hwif);

int hinic3_init_hwif(struct hinic3_hwdev *hwdev, void *cfg_reg_base,
		     void *intr_reg_base, void *mgmt_regs_base, u64 db_base_phy,
		     void *db_base, u64 db_dwqe_len);

void hinic3_free_hwif(struct hinic3_hwdev *hwdev);

void hinic3_show_chip_err_info(struct hinic3_hwdev *hwdev);

u8 hinic3_host_ppf_idx(struct hinic3_hwdev *hwdev, u8 host_id);

bool get_card_present_state(struct hinic3_hwdev *hwdev);

#endif
