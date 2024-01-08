/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_DEVICE_H
#define _ASM_SW64_DEVICE_H

struct dev_archdata {
#if defined(CONFIG_SUNWAY_IOMMU) || defined(CONFIG_SUNWAY_IOMMU_V2)
	void *iommu;
#endif
};

struct pdev_archdata {
};
#endif /* _ASM_SW64_DEVICE_H */
