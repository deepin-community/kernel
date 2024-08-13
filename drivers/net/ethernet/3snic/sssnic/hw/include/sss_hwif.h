/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_H
#define SSS_HWIF_H

#include <linux/types.h>
#include <linux/spinlock.h>

struct sss_db_pool {
	unsigned long	*bitmap;
	u32				bit_size;

	/* spinlock for allocating doorbell area */
	spinlock_t		id_lock;
};

struct sss_func_attr {
	enum sss_func_type func_type;

	u16			func_id;
	u8			pf_id;
	u8			pci_intf_id;

	u16			global_vf_off;
	u8			mpf_id;
	u8			ppf_id;

	u16			irq_num; /* max: 2 ^ 15 */
	u8			aeq_num; /* max: 2 ^ 3 */
	u8			ceq_num; /* max: 2 ^ 7 */

	u16			sq_num; /* max: 2 ^ 8 */
	u8			dma_attr_num; /* max: 2 ^ 6 */
	u8			msix_flex_en;
};

struct sss_hwif {
	u8 __iomem					*cfg_reg_base;
	u8 __iomem					*mgmt_reg_base;
	u64							db_base_paddr;
	u64							db_dwqe_len;
	u8 __iomem					*db_base_vaddr;

	void						*pdev;

	struct sss_db_pool			db_pool;

	struct sss_func_attr		attr;
};

#define SSS_GET_HWIF_AEQ_NUM(hwif)				((hwif)->attr.aeq_num)
#define SSS_GET_HWIF_CEQ_NUM(hwif)				((hwif)->attr.ceq_num)
#define SSS_GET_HWIF_IRQ_NUM(hwif)				((hwif)->attr.irq_num)
#define SSS_GET_HWIF_GLOBAL_ID(hwif)			((hwif)->attr.func_id)
#define SSS_GET_HWIF_PF_ID(hwif)				((hwif)->attr.pf_id)
#define SSS_GET_HWIF_GLOBAL_VF_OFFSET(hwif)		((hwif)->attr.global_vf_off)
#define SSS_GET_HWIF_PPF_ID(hwif)				((hwif)->attr.ppf_id)
#define SSS_GET_HWIF_MPF_ID(hwif)				((hwif)->attr.mpf_id)
#define SSS_GET_HWIF_PCI_INTF_ID(hwif)			((hwif)->attr.pci_intf_id)
#define SSS_GET_HWIF_FUNC_TYPE(hwif)			((hwif)->attr.func_type)
#define SSS_GET_HWIF_MSIX_EN(hwif)				((hwif)->attr.msix_flex_en)

#define SSS_SET_HWIF_AEQ_NUM(hwif, val) \
	((hwif)->attr.aeq_num = (val))

#define SSS_SET_HWIF_CEQ_NUM(hwif, val) \
	((hwif)->attr.ceq_num = (val))

#define SSS_SET_HWIF_IRQ_NUM(hwif, val) \
	((hwif)->attr.irq_num = (val))

#define SSS_SET_HWIF_GLOBAL_ID(hwif, val) \
	((hwif)->attr.func_id = (val))

#define SSS_SET_HWIF_PF_ID(hwif, val) \
	((hwif)->attr.pf_id = (val))

#define SSS_SET_HWIF_GLOBAL_VF_OFFSET(hwif, val) \
	((hwif)->attr.global_vf_off = (val))

#define SSS_SET_HWIF_PPF_ID(hwif, val) \
	((hwif)->attr.ppf_id = (val))

#define SSS_SET_HWIF_MPF_ID(hwif, val) \
	((hwif)->attr.mpf_id = (val))

#define SSS_SET_HWIF_PCI_INTF_ID(hwif, val) \
	((hwif)->attr.pci_intf_id = (val))

#define SSS_SET_HWIF_FUNC_TYPE(hwif, val) \
	((hwif)->attr.func_type = (val))

#define SSS_SET_HWIF_DMA_ATTR_NUM(hwif, val) \
	((hwif)->attr.dma_attr_num = (val))

#define SSS_SET_HWIF_MSIX_EN(hwif, val) \
	((hwif)->attr.msix_flex_en = (val))

#define SSS_SET_HWIF_SQ_NUM(hwif, val) \
	((hwif)->attr.sq_num = (val))

#endif
