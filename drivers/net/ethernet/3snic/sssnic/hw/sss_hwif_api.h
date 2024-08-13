/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_API_H
#define SSS_HWIF_API_H

#include "sss_hwdev.h"

enum sss_pf_status {
	SSS_PF_STATUS_INIT = 0X0,
	SSS_PF_STATUS_ACTIVE_FLAG = 0x11,
	SSS_PF_STATUS_FLR_START_FLAG = 0x12,
	SSS_PF_STATUS_FLR_FINISH_FLAG = 0x13,
};

enum sss_doorbell_ctrl {
	DB_ENABLE,
	DB_DISABLE,
};

enum sss_outbound_ctrl {
	OUTBOUND_ENABLE,
	OUTBOUND_DISABLE,
};

#define SSS_PCIE_LINK_DOWN					0xFFFFFFFF
#define SSS_PCIE_LINK_UP					0

#define SSS_AF1_PPF_ID_SHIFT				0
#define SSS_AF1_AEQ_PER_FUNC_SHIFT			8
#define SSS_AF1_MGMT_INIT_STATUS_SHIFT		30
#define SSS_AF1_PF_INIT_STATUS_SHIFT		31

#define SSS_AF1_PPF_ID_MASK					0x3F
#define SSS_AF1_AEQ_PER_FUNC_MASK			0x3
#define SSS_AF1_MGMT_INIT_STATUS_MASK		0x1
#define SSS_AF1_PF_INIT_STATUS_MASK			0x1

#define SSS_GET_AF1(val, member)			\
	(((val) >> SSS_AF1_##member##_SHIFT) & SSS_AF1_##member##_MASK)

#define SSS_AF4_DOORBELL_CTRL_SHIFT			0
#define SSS_AF4_DOORBELL_CTRL_MASK			0x1

#define SSS_GET_AF4(val, member)			\
		(((val) >> SSS_AF4_##member##_SHIFT) & SSS_AF4_##member##_MASK)

#define SSS_SET_AF4(val, member)			\
		(((val) & SSS_AF4_##member##_MASK) << SSS_AF4_##member##_SHIFT)

#define SSS_CLEAR_AF4(val, member)			\
		((val) & (~(SSS_AF4_##member##_MASK << SSS_AF4_##member##_SHIFT)))

#define SSS_AF6_PF_STATUS_SHIFT				0
#define SSS_AF6_PF_STATUS_MASK				0xFFFF

#define SSS_AF6_FUNC_MAX_SQ_SHIFT			23
#define SSS_AF6_FUNC_MAX_SQ_MASK			0x1FF

#define SSS_AF6_MSIX_FLEX_EN_SHIFT			22
#define SSS_AF6_MSIX_FLEX_EN_MASK			0x1

#define SSS_SET_AF6(val, member)					\
	((((u32)(val)) & SSS_AF6_##member##_MASK) <<	\
		SSS_AF6_##member##_SHIFT)

#define SSS_GET_AF6(val, member)					\
	(((u32)(val) >> SSS_AF6_##member##_SHIFT) & SSS_AF6_##member##_MASK)

#define SSS_CLEAR_AF6(val, member)					\
	((u32)(val) & (~(SSS_AF6_##member##_MASK <<		\
		SSS_AF6_##member##_SHIFT)))

#define SSS_PPF_ELECT_PORT_ID_SHIFT			0

#define SSS_PPF_ELECT_PORT_ID_MASK			0x3F

#define SSS_GET_PPF_ELECT_PORT(val, member)				\
	(((val) >> SSS_PPF_ELECT_PORT_##member##_SHIFT) &	\
		SSS_PPF_ELECT_PORT_##member##_MASK)

#define SSS_PPF_ELECTION_ID_SHIFT			0

#define SSS_PPF_ELECTION_ID_MASK			0x3F

#define SSS_SET_PPF(val, member)					\
	(((val) & SSS_PPF_ELECTION_##member##_MASK) <<	\
		SSS_PPF_ELECTION_##member##_SHIFT)

#define SSS_GET_PPF(val, member)					\
	(((val) >> SSS_PPF_ELECTION_##member##_SHIFT) &	\
		SSS_PPF_ELECTION_##member##_MASK)

#define SSS_CLEAR_PPF(val, member)					\
	((val) & (~(SSS_PPF_ELECTION_##member##_MASK <<	\
		SSS_PPF_ELECTION_##member##_SHIFT)))

#define SSS_DB_DWQE_SIZE	0x00400000

/* db/dwqe page size: 4K */
#define SSS_DB_PAGE_SIZE	0x00001000ULL
#define SSS_DWQE_OFFSET		0x00000800ULL

#define SSS_DB_MAX_AREAS	(SSS_DB_DWQE_SIZE / SSS_DB_PAGE_SIZE)

#define SSS_DB_ID(db, db_base)	\
	((u32)(((ulong)(db) - (ulong)(db_base)) / SSS_DB_PAGE_SIZE))

u32 sss_chip_read_reg(struct sss_hwif *hwif, u32 reg);
void sss_chip_write_reg(struct sss_hwif *hwif, u32 reg, u32 val);
bool sss_chip_get_present_state(void *hwdev);
u32 sss_chip_get_pcie_link_status(void *hwdev);
void sss_chip_set_pf_status(struct sss_hwif *hwif, enum sss_pf_status status);
enum sss_pf_status sss_chip_get_pf_status(struct sss_hwif *hwif);
void sss_chip_enable_doorbell(struct sss_hwif *hwif);
void sss_chip_disable_doorbell(struct sss_hwif *hwif);
int sss_alloc_db_id(struct sss_hwif *hwif, u32 *id);
void sss_free_db_id(struct sss_hwif *hwif, u32 id);
void sss_dump_chip_err_info(struct sss_hwdev *hwdev);
u8 sss_chip_get_host_ppf_id(struct sss_hwdev *hwdev, u8 host_id);
int sss_chip_set_eq_msix_attr(void *hwdev, struct sss_irq_cfg *info, u16 channel);
int sss_chip_set_wq_page_size(void *hwdev, u16 func_id, u32 page_size);
int sss_chip_set_ceq_attr(struct sss_hwdev *hwdev, u16 qid,
			  u32 attr0, u32 attr1);
void sss_chip_set_slave_host_status(void *hwdev, u8 host_id, bool enable);

#endif
