/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CQM_DB_H
#define CQM_DB_H

#include <linux/types.h>

struct tag_cqm_db_common {
#if (BYTE_ORDER == LITTLE_ENDIAN)
	u32 rsvd1 : 23;
	u32 c : 1;
	u32 cos : 3;
	u32 service_type : 5;
#else
	u32 service_type : 5;
	u32 cos : 3;
	u32 c : 1;
	u32 rsvd1 : 23;
#endif

	u32 rsvd2;
};

/* Only for test */
s32 cqm_db_addr_alloc(void *ex_handle, void __iomem **db_addr,
		      void __iomem **dwqe_addr);
s32 cqm_db_phy_addr_alloc(void *ex_handle, u64 *db_paddr, u64 *dwqe_addr);

s32 cqm_db_init(void *ex_handle);
void cqm_db_uninit(void *ex_handle);

s32 cqm_ring_hardware_db(void *ex_handle, u32 service_type, u8 db_count,
			 u64 db);

#endif /* CQM_DB_H */
