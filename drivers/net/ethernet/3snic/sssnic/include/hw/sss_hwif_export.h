/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_EXPORT_H
#define SSS_HWIF_EXPORT_H

#include <linux/types.h>

#include "sss_hw_common.h"
#include "sss_hw_irq.h"

/**
 * @brief sss_alloc_db_addr - alloc doorbell
 * @param hwdev: device pointer to hwdev
 * @param db_base: pointer to alloc doorbell base address
 * @retval zero: success
 * @retval non-zero: failure
 **/
int sss_alloc_db_addr(void *hwdev, void __iomem **db_base);

/**
 * @brief sss_free_db_addr - free doorbell
 * @param hwdev: device pointer to hwdev
 * @param db_base: pointer to free doorbell base address
 **/
void sss_free_db_addr(void *hwdev, const void __iomem *db_base);

/* *
 * @brief sss_nic_set_msix_auto_mask - set msix auto mask function
 * @param hwdev: device pointer to hwdev
 * @param msix_idx: msix id
 * @param flag: msix auto_mask flag, 1-enable, 2-clear
 */
void sss_chip_set_msix_auto_mask(void *hwdev, u16 msix_id,
				 enum sss_msix_auto_mask flag);

/* *
 * @brief sss_chip_set_msix_state - set msix state
 * @param hwdev: device pointer to hwdev
 * @param msix_id: msix id
 * @param flag: msix state flag, 0-enable, 1-disable
 */
void sss_chip_set_msix_state(void *hwdev, u16 msix_id,
			     enum sss_msix_state flag);

/* *
 * @brief sss_get_global_func_id - get global function id
 * @param hwdev: device pointer to hwdev
 * @retval global function id
 */
u16 sss_get_global_func_id(void *hwdev);

/* *
 * @brief sss_get_pf_id_of_vf - get pf id of vf
 * @param hwdev: device pointer to hwdev
 * @retval pf id
 */
u8 sss_get_pf_id_of_vf(void *hwdev);

/* *
 * @brief sss_get_pcie_itf_id - get pcie port id
 * @param hwdev: device pointer to hwdev
 * @retval pcie port id
 */
u8 sss_get_pcie_itf_id(void *hwdev);

/* *
 * @brief sss_get_func_type - get function type
 * @param hwdev: device pointer to hwdev
 * @retval function type
 */
enum sss_func_type sss_get_func_type(void *hwdev);

enum sss_func_type sss_get_func_id(void *hwdev);

/* *
 * @brief sss_get_glb_pf_vf_offset - get vf offset id of pf
 * @param hwdev: device pointer to hwdev
 * @retval vf offset id
 */
u16 sss_get_glb_pf_vf_offset(void *hwdev);

/* *
 * @brief sss_get_ppf_id - get ppf id
 * @param hwdev: device pointer to hwdev
 * @retval ppf id
 */
u8 sss_get_ppf_id(void *hwdev);
#endif
