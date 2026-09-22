/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef IOCLT_MMU600_H
#define IOCLT_MMU600_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>
#include <linux/ioctl.h>

#define MMU600TEST_DEV_NAME "mmu600test_device"
#define MMU600TEST_CLASS_NAME "mmu600test_class"
#define MMU600TEST_NAME "mmu600test"
#define MMU600TEST_DEV_COUNT 1

#define MMU600TEST_IOCTL_BASE 'M'
#define MMU600TEST_API_CMD_EXAMPLE _IOWR(MMU600TEST_IOCTL_BASE, 1, int)
#define MMU600TEST_API_CMD_SHOW_ALL_INFO _IOWR(MMU600TEST_IOCTL_BASE, 2, int)
#define MMU600TEST_API_CMD_SHOW_PTE_INFO _IOWR(MMU600TEST_IOCTL_BASE, 5, int)

#define MMU600TEST_API_CMD_SET_STAGE1_ENABLE \
	_IOWR(MMU600TEST_IOCTL_BASE, 20, int)
#define MMU600TEST_API_CMD_SET_STAGE1_BYPASS \
	_IOWR(MMU600TEST_IOCTL_BASE, 21, int)
#define MMU600TEST_API_CMD_SET_PTE _IOWR(MMU600TEST_IOCTL_BASE, 22, int)
#define MMU600TEST_API_CMD_DELETE_PTE _IOWR(MMU600TEST_IOCTL_BASE, 23, int)
#define MMU600TEST_API_CMD_CLEAN_TLB_BY_VA _IOWR(MMU600TEST_IOCTL_BASE, 24, int)
#define MMU600TEST_API_CMD_SYNC_TLB _IOWR(MMU600TEST_IOCTL_BASE, 26, int)

struct mmu600test_para {
	u32 udSid;
	u32 udSsid;
	u64 uddVa;
	u64 uddSize;
	u32 udPageLvl;
	u32 udNum;
	u32 udRegOffset;
	u32 udRegVal32;
	u64 uddRegVal64;
};

void zxdh_smmu_test_dbg_init(struct zxdh_sc_dev *dev);
void mmu600test_dbg_exit(struct zxdh_sc_dev *dev);

#ifdef __cplusplus
}
#endif

#endif /* IOCLT_MMU600_H */
