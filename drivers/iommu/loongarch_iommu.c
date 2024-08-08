// SPDX-License-Identifier: GPL-2.0
/*
 * Loongson IOMMU Driver
 *
 * Copyright (C) 2024 Loongson Technology Ltd.
 * Author:	Lv Chen <lvchen@loongson.cn>
 *		Wang Yang <wangyang@loongson.cn>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/printk.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/iommu.h>
#include <linux/sizes.h>
#include <asm/addrspace.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/pci_regs.h>
#include "loongarch_iommu.h"

MODULE_LICENSE("GPL");

#define LOOP_TIMEOUT			100000

#define IVRS_HEADER_LENGTH		48
#define ACPI_IVHD_TYPE_MAX_SUPPORTED	0x40
#define IVHD_DEV_ALL			0x01
#define IVHD_DEV_SELECT			0x02
#define IVHD_DEV_SELECT_RANGE_START	0x03
#define IVHD_DEV_RANGE_END		0x04
#define IVHD_DEV_ALIAS			0x42
#define IVHD_DEV_EXT_SELECT		0x46
#define IVHD_DEV_ACPI_HID		0xf0

#define IVHD_HEAD_TYPE10		0x10
#define IVHD_HEAD_TYPE11		0x11
#define IVHD_HEAD_TYPE40		0x40

#define MAX_BDF_NUM			0xffff

#define RLOOKUP_TABLE_ENTRY_SIZE	(sizeof(void *))

/*
 * structure describing one IOMMU in the ACPI table. Typically followed by one
 * or more ivhd_entrys.
 */
struct ivhd_header {
	u8 type;
	u8 flags;
	u16 length;
	u16 devid;
	u16 cap_ptr;
	u64 mmio_phys;
	u16 pci_seg;
	u16 info;
	u32 efr_attr;

	/* Following only valid on IVHD type 11h and 40h */
	u64 efr_reg; /* Exact copy of MMIO_EXT_FEATURES */
	u64 res;
} __packed;

/*
 * A device entry describing which devices a specific IOMMU translates and
 * which requestor ids they use.
 */
struct ivhd_entry {
	u8 type;
	u16 devid;
	u8 flags;
	u32 ext;
	u32 hidh;
	u64 cid;
	u8 uidf;
	u8 uidl;
	u8 uid;
} __packed;

struct iommu_callback_data {
	const struct iommu_ops *ops;
};

LIST_HEAD(la_rlookup_iommu_list);
LIST_HEAD(la_iommu_list);			/* list of all loongarch
						 * IOMMUs in the system
						 */

static u32 rlookup_table_size;			/* size if the rlookup table */
static int la_iommu_target_ivhd_type;
u16	la_iommu_last_bdf;			/* largest PCI device id
						 *  we have to handle
						 */

int loongarch_iommu_disable;

#define iommu_write_regl(iommu, off, val) \
	writel(val, iommu->confbase + off)
#define iommu_read_regl(iommu, off)	readl(iommu->confbase + off)

static void switch_huge_to_page(unsigned long *ptep, unsigned long start);

static void iommu_translate_disable(struct loongarch_iommu *iommu)
{
	u32 val;

	if (iommu == NULL) {
		pr_err("%s iommu is NULL", __func__);
		return;
	}

	/* Disable */
	val = iommu_read_regl(iommu, LA_IOMMU_PFM_CNT_EN);
	val &= ~(1 << 31);
	iommu_write_regl(iommu, LA_IOMMU_PFM_CNT_EN, val);

	/* Write cmd */
	val = iommu_read_regl(iommu, LA_IOMMU_CMD);
	val &= 0xfffffffc;
	iommu_write_regl(iommu, LA_IOMMU_CMD, val);
}

static void iommu_translate_enable(struct loongarch_iommu *iommu)
{
	u32 val = 0;

	if (iommu == NULL) {
		pr_err("%s iommu is NULL", __func__);
		return;
	}

	/* Enable use mem */
	val = iommu_read_regl(iommu, LA_IOMMU_PFM_CNT_EN);
	val |= (1 << 29);
	iommu_write_regl(iommu, LA_IOMMU_PFM_CNT_EN, val);

	/* Enable */
	val = iommu_read_regl(iommu, LA_IOMMU_PFM_CNT_EN);
	val |= (1 << 31);
	iommu_write_regl(iommu, LA_IOMMU_PFM_CNT_EN, val);

	/* Write cmd */
	val = iommu_read_regl(iommu, LA_IOMMU_CMD);
	val &= 0xfffffffc;
	iommu_write_regl(iommu, LA_IOMMU_CMD, val);
}

static bool la_iommu_capable(struct device *dev, enum iommu_cap cap)
{
	switch (cap) {
	case IOMMU_CAP_CACHE_COHERENCY:
		return true;
	default:
		return false;
	}
}

static struct dom_info *to_dom_info(struct iommu_domain *dom)
{
	return container_of(dom, struct dom_info, domain);
}

static void flush_iotlb_by_domain_id(struct loongarch_iommu *iommu, u16 domain_id, bool read)
{
	u32 val;
	u32 flush_read_tlb = read ? 1 : 0;

	if (iommu == NULL) {
		pr_err("%s iommu is NULL", __func__);
		return;
	}

	val = iommu_read_regl(iommu, LA_IOMMU_EIVDB);
	val &= ~0xf0000;
	val |= ((u32)domain_id) << 16;
	iommu_write_regl(iommu, LA_IOMMU_EIVDB, val);

	/* Flush all  */
	val = iommu_read_regl(iommu, LA_IOMMU_VBTC);
	val &= ~0x10f;
	val |= (flush_read_tlb << 8) | 4;
	iommu_write_regl(iommu, LA_IOMMU_VBTC, val);
}

static void flush_iotlb(struct loongarch_iommu *iommu)
{
	u32 val;

	if (iommu == NULL) {
		pr_err("%s iommu is NULL", __func__);
		return;
	}

	/* Flush all tlb */
	val = iommu_read_regl(iommu, LA_IOMMU_VBTC);
	val &= ~0x1f;
	val |= 0x5;
	iommu_write_regl(iommu, LA_IOMMU_VBTC, val);
}

static int flush_pgtable_is_busy(struct loongarch_iommu *iommu)
{
	u32 val;

	val = iommu_read_regl(iommu, LA_IOMMU_VBTC);
	return val & IOMMU_PGTABLE_BUSY;
}

static int iommu_flush_iotlb_by_domain(struct la_iommu_dev_data *dev_data)
{
	u32 retry = 0;
	struct loongarch_iommu *iommu;
	u16 domain_id;

	if (dev_data == NULL) {
		pr_err("%s dev_data is NULL", __func__);
		return 0;
	}

	if (dev_data->iommu == NULL) {
		pr_err("%s iommu is NULL", __func__);
		return 0;
	}

	if (dev_data->iommu_entry == NULL) {
		pr_err("%s iommu_entry is NULL", __func__);
		return 0;
	}

	iommu = dev_data->iommu;
	domain_id = dev_data->iommu_entry->id;

	flush_iotlb_by_domain_id(iommu, domain_id, 0);
	while (flush_pgtable_is_busy(iommu)) {
		if (retry == LOOP_TIMEOUT) {
			pr_err("LA-IOMMU: %s %d iotlb flush busy\n",
					__func__, __LINE__);
			return -EIO;
		}
		retry++;
		udelay(1);
	}

	flush_iotlb_by_domain_id(iommu, domain_id, 1);
	while (flush_pgtable_is_busy(iommu)) {
		if (retry == LOOP_TIMEOUT) {
			pr_err("LA-IOMMU: %s %d iotlb flush busy\n",
					__func__, __LINE__);
			return -EIO;
		}
		retry++;
		udelay(1);
	}
	iommu_translate_enable(iommu);
	return 0;
}

static int update_dev_table(struct la_iommu_dev_data *dev_data, int flag)
{
	u32 val = 0;
	int index;
	unsigned short bdf;
	struct loongarch_iommu *iommu;
	u16 domain_id;

	if (dev_data == NULL) {
		pr_err("%s dev_data is NULL", __func__);
		return 0;
	}

	if (dev_data->iommu == NULL) {
		pr_err("%s iommu is NULL", __func__);
		return 0;
	}

	if (dev_data->iommu_entry == NULL) {
		pr_err("%s iommu_entry is NULL", __func__);
		return 0;
	}

	iommu = dev_data->iommu;
	domain_id = dev_data->iommu_entry->id;
	bdf = dev_data->bdf;

	/* Set device table */
	if (flag) {
		index = find_first_zero_bit(iommu->devtable_bitmap,
						MAX_ATTACHED_DEV_ID);
		if (index < MAX_ATTACHED_DEV_ID) {
			__set_bit(index, iommu->devtable_bitmap);
			dev_data->index = index;
		} else {
			pr_err("%s get id from dev table failed\n", __func__);
			return 0;
		}

		pr_info("%s bdf %x domain_id %d iommu devid %x iommu segment %d flag %x\n",
				__func__, bdf, domain_id, iommu->devid,
				iommu->segment, flag);

		val = bdf & 0xffff;
		val |= ((domain_id & 0xf) << 16);	/* domain id */
		val |= ((index & 0xf) << 24);		/* index */
		val |= (0x1 << 20);			/* valid */
		iommu_write_regl(iommu, LA_IOMMU_EIVDB, val);

		val = (0x1 << 31) | (0xf << 0);
		val |= (0x1 << 29);			/* 1: use main memory */
		iommu_write_regl(iommu, LA_IOMMU_PFM_CNT_EN, val);

		val = iommu_read_regl(iommu, LA_IOMMU_CMD);
		val &= 0xfffffffc;
		iommu_write_regl(iommu, LA_IOMMU_CMD, val);
	} else {
		/* Flush device table */
		index = dev_data->index;
		pr_info("%s bdf %x domain_id %d iommu devid %x iommu segment %d flag %x\n",
				__func__, bdf, domain_id, iommu->devid,
				iommu->segment, flag);

		val = iommu_read_regl(iommu, LA_IOMMU_EIVDB);
		val &= ~(0xffffffff);
		val |= ((index & 0xf) << 24);	/* index */
		iommu_write_regl(iommu, LA_IOMMU_EIVDB, val);

		val = iommu_read_regl(iommu, LA_IOMMU_PFM_CNT_EN);
		val |= (0x1 << 29);			/* 1: use main memory */
		iommu_write_regl(iommu, LA_IOMMU_PFM_CNT_EN, val);

		if (index < MAX_ATTACHED_DEV_ID)
			__clear_bit(index, iommu->devtable_bitmap);
	}

	iommu_flush_iotlb_by_domain(dev_data);
	return 0;
}

static int iommu_flush_iotlb(struct loongarch_iommu *iommu)
{
	u32 retry = 0;

	if (iommu == NULL) {
		pr_err("%s iommu is NULL", __func__);
		return 0;
	}

	flush_iotlb(iommu);
	while (flush_pgtable_is_busy(iommu)) {
		if (retry == LOOP_TIMEOUT) {
			pr_err("LA-IOMMU: iotlb flush busy\n");
			return -EIO;
		}
		retry++;
		udelay(1);
	}
	iommu_translate_enable(iommu);
	return 0;
}

static void la_iommu_flush_iotlb_all(struct iommu_domain *domain)
{
	struct dom_info *priv = to_dom_info(domain);
	struct iommu_info *info;

	spin_lock(&priv->lock);
	list_for_each_entry(info, &priv->iommu_devlist, list)
		iommu_flush_iotlb(info->iommu);
	spin_unlock(&priv->lock);
}

static void do_attach(struct iommu_info *info, struct la_iommu_dev_data *dev_data)
{
	if (dev_data->count)
		return;

	dev_data->count++;
	dev_data->iommu_entry = info;

	spin_lock(&info->devlock);
	list_add(&dev_data->list, &info->dev_list);
	info->dev_cnt += 1;
	spin_unlock(&info->devlock);

	update_dev_table(dev_data, 1);
}

static void do_detach(struct la_iommu_dev_data *dev_data)
{
	struct iommu_info *info;

	if (!dev_data || !dev_data->iommu_entry || (dev_data->count == 0)) {
		pr_err("%s dev_data or iommu_entry is NULL", __func__);
		return;
	}
	dev_data->count--;
	info = dev_data->iommu_entry;
	list_del(&dev_data->list);
	info->dev_cnt -= 1;
	update_dev_table(dev_data, 0);
	dev_data->iommu_entry = NULL;
}

static void detach_all_dev_by_domain(struct iommu_info *info)
{
	struct la_iommu_dev_data *dev_data = NULL;

	spin_lock(&info->devlock);
	while (!list_empty(&info->dev_list)) {
		dev_data = list_first_entry(&info->dev_list,
				struct la_iommu_dev_data, list);
		do_detach(dev_data);
	}
	spin_unlock(&info->devlock);
}

static int domain_id_alloc(struct loongarch_iommu *iommu)
{
	int id = -1;

	if (iommu == NULL) {
		pr_err("%s iommu is NULL", __func__);
		return id;
	}
	spin_lock(&iommu->domain_bitmap_lock);
	id = find_first_zero_bit(iommu->domain_bitmap, MAX_DOMAIN_ID);
	if (id < MAX_DOMAIN_ID)
		__set_bit(id, iommu->domain_bitmap);
	spin_unlock(&iommu->domain_bitmap_lock);
	if (id >= MAX_DOMAIN_ID) {
		id = -1;
		pr_err("LA-IOMMU: Alloc domain id over max domain id\n");
	}
	return id;
}

static void domain_id_free(struct loongarch_iommu *iommu, int id)
{
	if (iommu == NULL) {
		pr_err("%s iommu is NULL", __func__);
		return;
	}
	if ((id >= 0) && (id < MAX_DOMAIN_ID)) {
		spin_lock(&iommu->domain_bitmap_lock);
		__clear_bit(id, iommu->domain_bitmap);
		spin_unlock(&iommu->domain_bitmap_lock);
	}
}

/*
 * Check whether the system has a priv.
 * If yes, it returns 1 and if not, it returns 0
 */
static int has_dom(struct loongarch_iommu *iommu)
{
	int ret = 0;

	spin_lock(&iommu->dom_info_lock);
	while (!list_empty(&iommu->dom_list)) {
		ret = 1;
		break;
	}
	spin_unlock(&iommu->dom_info_lock);
	return ret;
}

/*
 *  This function adds a private domain to the global domain list
 */
static struct dom_entry *find_domain_in_list(struct loongarch_iommu *iommu, struct dom_info *priv)
{
	struct dom_entry *entry, *found = NULL;

	if (priv == NULL)
		return found;
	spin_lock(&iommu->dom_info_lock);
	list_for_each_entry(entry, &iommu->dom_list, list) {
		if (entry->domain_info == priv) {
			found = entry;
			break;
		}
	}
	spin_unlock(&iommu->dom_info_lock);
	return found;
}

static void add_domain_to_list(struct loongarch_iommu *iommu, struct dom_info *priv)
{
	struct dom_entry *entry;

	if (priv == NULL)
		return;
	entry = find_domain_in_list(iommu, priv);
	if (entry != NULL)
		return;
	entry = kzalloc(sizeof(struct dom_entry), GFP_KERNEL);
	entry->domain_info = priv;
	spin_lock(&iommu->dom_info_lock);
	list_add(&entry->list, &iommu->dom_list);
	spin_unlock(&iommu->dom_info_lock);
}

static void del_domain_from_list(struct loongarch_iommu *iommu, struct dom_info *priv)
{
	struct dom_entry *entry;

	entry = find_domain_in_list(iommu, priv);
	if (entry == NULL)
		return;
	spin_lock(&iommu->dom_info_lock);
	list_del(&entry->list);
	spin_unlock(&iommu->dom_info_lock);
	kfree(entry);
}

static void free_pagetable(void *pt_base, int level)
{
	int i;
	unsigned long *ptep, *pgtable;

	ptep = pt_base;
	if (level == IOMMU_PT_LEVEL1) {
		kfree(pt_base);
		return;
	}
	for (i = 0; i < IOMMU_PTRS_PER_LEVEL; i++, ptep++) {
		if (!iommu_pte_present(ptep))
			continue;

		if (((level - 1) == IOMMU_PT_LEVEL1) && iommu_pte_huge(ptep)) {
			*ptep = 0;
			continue;
		}

		pgtable = phys_to_virt(*ptep & IOMMU_PAGE_MASK);
		free_pagetable(pgtable, level - 1);
	}
	kfree(pt_base);
}

static void iommu_free_pagetable(struct dom_info *info)
{
	free_pagetable(info->pgd, IOMMU_LEVEL_MAX);
	info->pgd = NULL;
}

static struct dom_info *alloc_dom_info(void)
{
	struct dom_info *info;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL)
		return NULL;

	info->pgd = kzalloc(IOMMU_PAGE_SIZE, GFP_KERNEL_ACCOUNT);
	if (info->pgd == NULL) {
		kfree(info);
		return NULL;
	}
	INIT_LIST_HEAD(&info->iommu_devlist);
	spin_lock_init(&info->lock);
	mutex_init(&info->ptl_lock);
	info->domain.geometry.aperture_start = 0;
	info->domain.geometry.aperture_end = ~0ULL;
	info->domain.geometry.force_aperture = true;

	return info;
}

static void dom_info_free(struct dom_info *info)
{
	if (info->pgd != NULL) {
		kfree(info->pgd);
		info->pgd = NULL;
	}
	kfree(info);
}

static struct iommu_domain *la_iommu_domain_alloc(unsigned int type)
{
	struct dom_info *info;

	switch (type) {
	case IOMMU_DOMAIN_BLOCKED:
	case IOMMU_DOMAIN_UNMANAGED:
		info = alloc_dom_info();
		if (info == NULL)
			return NULL;
		break;
	default:
		return NULL;
	}
	return &info->domain;
}

void domain_deattach_iommu(struct dom_info *priv, struct iommu_info *info)
{
	if ((priv == NULL) || (info == NULL) ||
		(info->dev_cnt != 0) || (info->iommu == NULL)) {
		pr_err("%s invalid parameter", __func__);
		return;
	}
	del_domain_from_list(info->iommu, priv);
	domain_id_free(info->iommu, info->id);
	spin_lock(&priv->lock);
	list_del(&info->list);
	spin_unlock(&priv->lock);
	kfree(info);
}

static void la_iommu_domain_free(struct iommu_domain *domain)
{
	struct dom_info *priv;
	struct loongarch_iommu *iommu = NULL;
	struct iommu_info *info, *tmp;

	priv = to_dom_info(domain);
	spin_lock(&priv->lock);
	list_for_each_entry_safe(info, tmp, &priv->iommu_devlist, list) {
		if (info->dev_cnt > 0)
			detach_all_dev_by_domain(info);
		iommu = info->iommu;
		spin_unlock(&priv->lock);
		domain_deattach_iommu(priv, info);
		spin_lock(&priv->lock);
		iommu_flush_iotlb(iommu);
		if (!has_dom(iommu))
			iommu_translate_disable(iommu);
	}
	spin_unlock(&priv->lock);
	mutex_lock(&priv->ptl_lock);
	iommu_free_pagetable(priv);
	mutex_unlock(&priv->ptl_lock);
	dom_info_free(priv);
}

struct iommu_rlookup_entry *lookup_rlooptable(int pcisegment)
{
	struct iommu_rlookup_entry *rlookupentry = NULL;

	list_for_each_entry(rlookupentry, &la_rlookup_iommu_list, list) {
		if (rlookupentry->pcisegment == pcisegment)
			return rlookupentry;
	}
	return NULL;
}

struct loongarch_iommu *find_iommu_by_dev(struct pci_dev  *pdev)
{
	int pcisegment;
	unsigned short devid;
	struct iommu_rlookup_entry *rlookupentry = NULL;
	struct loongarch_iommu *iommu = NULL;
	struct pci_bus	*bus = pdev->bus;

	devid = PCI_DEVID(bus->number, pdev->devfn);
	pcisegment = pci_domain_nr(bus);
	rlookupentry = lookup_rlooptable(pcisegment);
	if (rlookupentry == NULL) {
		pr_debug("%s find segment %d rlookupentry failed\n", __func__,
				pcisegment);
		return iommu;
	}

	if (devid > la_iommu_last_bdf) {
		pci_warn(pdev, "The deviceid %x is greater than the maximum bdf number %x\n",
			 devid, la_iommu_last_bdf);
		return iommu;
	}

	iommu = rlookupentry->rlookup_table[devid];
	if (iommu && (!iommu->confbase))
		iommu = NULL;
	return iommu;
}

struct iommu_device *iommu_init_device(struct device *dev)
{
	struct la_iommu_dev_data *dev_data;
	struct pci_dev	*pdev = to_pci_dev(dev);
	struct pci_bus	*bus = pdev->bus;
	unsigned short devid;
	struct loongarch_iommu *iommu = NULL;
	struct iommu_device *iommu_dev = ERR_PTR(-ENODEV);

	if (!dev_is_pci(dev))
		return iommu_dev;

	if (dev->archdata.iommu != NULL || bus == NULL) {
		pr_info("LA-IOMMU: bdf:0x%x has added\n", pdev->devfn);
		return iommu_dev;
	}
	iommu = find_iommu_by_dev(pdev);
	if (iommu == NULL) {
		pci_dbg(pdev, "%s the device is not managed by iommu\n", __func__);
		return iommu_dev;
	}

	if (iommu->disabled) {
		pci_dbg(pdev, "%s the iommu[seg:%d devid:%#x] is disabled\n",
			__func__, iommu->segment, iommu->devid);
		return iommu_dev;
	}
	dev_data = kzalloc(sizeof(*dev_data), GFP_KERNEL);
	if (!dev_data)
		return iommu_dev;
	devid = PCI_DEVID(bus->number, pdev->devfn);
	dev_data->bdf = devid;

	pci_info(pdev, "%s bdf %#x iommu dev id %#x\n", __func__, dev_data->bdf, iommu->devid);
	/* The initial state is 0, and 1 is added only when attach dev */
	dev_data->count = 0;
	dev_data->iommu = iommu;
	dev_data->dev = dev;
	dev->archdata.iommu = dev_data;
	iommu_dev = &iommu->iommu_dev;
	return iommu_dev;
}

struct iommu_device *la_iommu_probe_device(struct device *dev)
{
	return iommu_init_device(dev);
}

static struct iommu_group *la_iommu_device_group(struct device *dev)
{
	struct iommu_group *group;

	/*
	 * We don't support devices sharing stream IDs other than PCI RID
	 * aliases, since the necessary ID-to-device lookup becomes rather
	 * impractical given a potential sparse 32-bit stream ID space.
	 */
	if (dev_is_pci(dev))
		group = pci_device_group(dev);
	else
		group = generic_device_group(dev);
	return group;
}

static void la_iommu_remove_device(struct device *dev)
{
	struct la_iommu_dev_data *dev_data;

	iommu_group_remove_device(dev);
	dev_data = dev->archdata.iommu;
	dev->archdata.iommu = NULL;
	kfree(dev_data);
}

struct iommu_info *get_iommu_info_from_dom(struct dom_info *priv, struct loongarch_iommu *iommu)
{
	struct iommu_info *info;

	spin_lock(&priv->lock);
	list_for_each_entry(info, &priv->iommu_devlist, list) {
		if (info->iommu == iommu) {
			spin_unlock(&priv->lock);
			return info;
		}
	}
	spin_unlock(&priv->lock);
	return NULL;
}

struct iommu_info *domain_attach_iommu(struct dom_info *priv, struct loongarch_iommu *iommu)
{
	u32 dir_ctrl;
	struct iommu_info *info;
	unsigned long phys;

	info = get_iommu_info_from_dom(priv, iommu);
	if (info)
		return info;

	info = kzalloc(sizeof(struct iommu_info), GFP_KERNEL_ACCOUNT);
	if (!info)
		return NULL;

	INIT_LIST_HEAD(&info->dev_list);
	info->iommu = iommu;
	info->id = domain_id_alloc(iommu);
	if (info->id == -1) {
		pr_info("%s alloc id for domain failed\n", __func__);
		kfree(info);
		return NULL;
	}

	phys = virt_to_phys(priv->pgd);
	dir_ctrl = (IOMMU_LEVEL_STRIDE << 26) | (IOMMU_LEVEL_SHIFT(2) << 20);
	dir_ctrl |= (IOMMU_LEVEL_STRIDE <<  16) | (IOMMU_LEVEL_SHIFT(1) << 10);
	dir_ctrl |= (IOMMU_LEVEL_STRIDE << 6) | IOMMU_LEVEL_SHIFT(0);
	iommu_write_regl(iommu, LA_IOMMU_DIR_CTRL(info->id), dir_ctrl);
	iommu_write_regl(iommu, LA_IOMMU_PGD_HI(info->id), phys >> 32);
	iommu_write_regl(iommu, LA_IOMMU_PGD_LO(info->id), phys & UINT_MAX);

	spin_lock(&priv->lock);
	list_add(&info->list, &priv->iommu_devlist);
	spin_unlock(&priv->lock);
	add_domain_to_list(iommu, priv);
	return info;
}

static struct la_iommu_dev_data *get_devdata_from_iommu_info(struct dom_info *info,
		struct loongarch_iommu *iommu, unsigned long bdf)
{
	struct iommu_info *entry;
	struct la_iommu_dev_data *dev_data, *found = NULL;

	entry = get_iommu_info_from_dom(info, iommu);
	if (!entry)
		return found;
	spin_lock(&entry->devlock);
	list_for_each_entry(dev_data, &entry->dev_list, list) {
		if (dev_data->bdf == bdf) {
			found = dev_data;
			break;
		}
	}
	spin_unlock(&entry->devlock);
	return found;
}
static void la_iommu_detach_dev(struct device *dev);

static int la_iommu_attach_dev(struct iommu_domain *domain, struct device *dev)
{
	struct dom_info *priv = to_dom_info(domain);
	struct pci_dev  *pdev = to_pci_dev(dev);
	unsigned char busnum = pdev->bus->number;
	struct la_iommu_dev_data *dev_data;
	struct loongarch_iommu *iommu;
	struct iommu_info *info;
	unsigned short bdf;

	la_iommu_detach_dev(dev);

	if (domain != NULL && domain->type == IOMMU_DOMAIN_BLOCKED)
		return 0;

	if (domain == NULL)
		return 0;

	bdf = PCI_DEVID(busnum, pdev->devfn);
	dev_data = (struct la_iommu_dev_data *)dev->archdata.iommu;
	if (dev_data == NULL) {
		pci_info(pdev, "%s dev_data is Invalid\n", __func__);
		return 0;
	}

	iommu = dev_data->iommu;
	if (iommu == NULL) {
		pci_info(pdev, "%s iommu is Invalid\n", __func__);
		return 0;
	}

	pci_info(pdev, "%s bdf %#x priv %lx iommu devid %#x\n", __func__,
			bdf, (unsigned long)priv, iommu->devid);
	dev_data = get_devdata_from_iommu_info(priv, iommu, bdf);
	if (dev_data) {
		pci_info(pdev, "LA-IOMMU: bdf 0x%x devfn %x has attached, count:0x%x\n",
			bdf, pdev->devfn, dev_data->count);
		return 0;
	}
	dev_data = (struct la_iommu_dev_data *)dev->archdata.iommu;

	info = domain_attach_iommu(priv, iommu);
	if (!info) {
		pci_info(pdev, "domain attach iommu failed\n");
		return 0;
	}
	dev_data->domain = domain;
	do_attach(info, dev_data);
	return 0;
}

static void la_iommu_detach_dev(struct device *dev)
{
	struct iommu_domain *domain;
	struct dom_info *priv;
	struct pci_dev *pdev = to_pci_dev(dev);
	unsigned char busnum = pdev->bus->number;
	struct la_iommu_dev_data *dev_data;
	struct loongarch_iommu *iommu;
	struct iommu_info *iommu_entry = NULL;
	unsigned short bdf;

	bdf = PCI_DEVID(busnum, pdev->devfn);
	dev_data = (struct la_iommu_dev_data *)dev->archdata.iommu;
	if (dev_data == NULL) {
		pci_info(pdev, "%s dev_data is Invalid\n", __func__);
		return;
	}

	domain = dev_data->domain;
	if (domain == NULL)
		return;

	priv = to_dom_info(domain);
	iommu = dev_data->iommu;
	if (iommu == NULL) {
		pci_info(pdev, "%s iommu is Invalid\n", __func__);
		return;
	}
	dev_data = get_devdata_from_iommu_info(priv, iommu, bdf);
	if (dev_data == NULL) {
		pci_info(pdev, "%s bdf %#x hasn't attached\n",
			__func__, bdf);
			return;
	}

	iommu = dev_data->iommu;
	dev_data->dev = NULL;
	iommu_entry = get_iommu_info_from_dom(priv, iommu);
	if (iommu_entry == NULL) {
		pci_info(pdev, "%s get iommu_entry failed\n", __func__);
		return;
	}

	spin_lock(&iommu_entry->devlock);
	do_detach(dev_data);
	spin_unlock(&iommu_entry->devlock);
	dev_data->domain = NULL;

	pci_info(pdev, "%s iommu devid  %x sigment %x\n", __func__,
			iommu->devid, iommu->segment);
}

static unsigned long *iommu_get_pte(void *pt_base, unsigned long vaddr, int level)
{
	int i;
	unsigned long *ptep, *pgtable;

	if (level > (IOMMU_LEVEL_MAX - 1))
		return NULL;
	pgtable = pt_base;
	for (i = IOMMU_LEVEL_MAX - 1; i >= level; i--) {
		ptep = iommu_pte_offset(pgtable, vaddr, i);
		if (!iommu_pte_present(ptep))
			break;
		if (iommu_pte_huge(ptep))
			break;
		pgtable = phys_to_virt(*ptep & IOMMU_PAGE_MASK);
	}
	return ptep;
}

static int iommu_get_page_table(unsigned long *ptep)
{
	void *addr;
	unsigned long pte;

	if (!iommu_pte_present(ptep)) {
		addr = kzalloc(IOMMU_PAGE_SIZE, GFP_KERNEL_ACCOUNT);
		if (!addr)
			return -ENOMEM;
		pte = virt_to_phys(addr) & IOMMU_PAGE_MASK;
		pte |= IOMMU_PTE_RW;
		*ptep = pte;
	}
	return 0;
}

static size_t iommu_page_map(void *pt_base,
		unsigned long start, unsigned long end,
		phys_addr_t paddr, int level)
{
	unsigned long next, old, step;
	unsigned long pte, *ptep, *pgtable;
	int ret, huge, switch_page;

	old = start;
	ptep = iommu_pte_offset(pt_base, start, level);
	if (level == IOMMU_PT_LEVEL0) {
		paddr = paddr & IOMMU_PAGE_MASK;
		do {
			pte =  paddr | IOMMU_PTE_RW;
			*ptep = pte;
			ptep++;
			start += IOMMU_PAGE_SIZE;
			paddr += IOMMU_PAGE_SIZE;
		} while (start < end);

		return start - old;
	}

	do {
		next = iommu_ptable_end(start, end, level);
		step = next - start;
		huge = 0;
		switch_page = 0;
		if (level == IOMMU_PT_LEVEL1) {
			if ((step == IOMMU_HPAGE_SIZE) &&
			    (!iommu_pte_present(ptep) ||
			    iommu_pte_huge(ptep)))
				huge = 1;
			else if (iommu_pte_present(ptep) &&
				 iommu_pte_huge(ptep))
				switch_page = 1;
		}

		if (switch_page)
			switch_huge_to_page(ptep, start);

		if (huge) {
			pte =  (paddr & IOMMU_HPAGE_MASK) |
				IOMMU_PTE_RW | IOMMU_PTE_HP;
			*ptep = pte;
		} else {
			ret = iommu_get_page_table(ptep);
			if (ret != 0)
				break;
			pgtable = phys_to_virt(*ptep & IOMMU_PAGE_MASK);
			iommu_page_map(pgtable, start, next, paddr, level - 1);
		}

		ptep++;
		paddr += step;
		start = next;
	} while (start < end);
	return start - old;
}

static void switch_huge_to_page(unsigned long *ptep, unsigned long start)
{
	phys_addr_t paddr = *ptep & IOMMU_HPAGE_MASK;
	unsigned long next = start + IOMMU_HPAGE_SIZE;
	unsigned long *pgtable;
	int ret;

	*ptep = 0;
	ret = iommu_get_page_table(ptep);
	if (ret == 0) {
		pgtable = phys_to_virt(*ptep & IOMMU_PAGE_MASK);
		iommu_page_map(pgtable, start, next, paddr, 0);
	}
}

static int domain_map_page(struct dom_info *priv, unsigned long start,
			phys_addr_t paddr, size_t size)
{
	int ret = 0;
	phys_addr_t end;
	size_t map_size;

	end = start + size;
	mutex_lock(&priv->ptl_lock);
	map_size = iommu_page_map(priv->pgd, start,
			end, paddr, IOMMU_LEVEL_MAX - 1);
	if (map_size != size)
		ret = -EFAULT;
	mutex_unlock(&priv->ptl_lock);
	la_iommu_flush_iotlb_all(&priv->domain);
	return ret;
}

static size_t iommu_page_unmap(void *pt_base,
		unsigned long start, unsigned long end, int level)
{
	unsigned long next, old;
	unsigned long *ptep, *pgtable;

	old = start;
	ptep = iommu_pte_offset(pt_base, start, level);
	if (level == IOMMU_PT_LEVEL0) {
		do {
			*ptep++ = 0;
			start += IOMMU_PAGE_SIZE;
		} while (start < end);
	} else {
		do {
			next = iommu_ptable_end(start, end, level);
			if (!iommu_pte_present(ptep))
				continue;

			if ((level == IOMMU_PT_LEVEL1) &&
			    iommu_pte_huge(ptep) &&
			    ((next - start) < IOMMU_HPAGE_SIZE))
				switch_huge_to_page(ptep, start);

			if (iommu_pte_huge(ptep)) {
				if ((next - start) != IOMMU_HPAGE_SIZE)
					pr_err(
				"Map pte on hugepage not supported now\n");
				*ptep = 0;
			} else {
				pgtable = phys_to_virt(*ptep & IOMMU_PAGE_MASK);
				iommu_page_unmap(pgtable, start,
						next, level - 1);
			}
		} while (ptep++, start = next, start < end);
	}
	return start - old;
}

static size_t domain_unmap_page(struct dom_info *priv,
		unsigned long start, size_t size)
{
	size_t unmap_len;
	unsigned long end;

	end = start + size;
	mutex_lock(&priv->ptl_lock);
	unmap_len = iommu_page_unmap(priv->pgd, start,
			end, (IOMMU_LEVEL_MAX - 1));
	mutex_unlock(&priv->ptl_lock);
	la_iommu_flush_iotlb_all(&priv->domain);
	return unmap_len;
}

static int la_iommu_map(struct iommu_domain *domain, unsigned long vaddr,
			 phys_addr_t paddr, size_t len, int prot, gfp_t gfp)
{
	int ret;
	struct dom_info *priv = to_dom_info(domain);

	ret = domain_map_page(priv, vaddr, paddr, len);
	return ret;
}

static size_t la_iommu_unmap(struct iommu_domain *domain, unsigned long vaddr,
				size_t len, struct iommu_iotlb_gather *iotlb_gather)
{
	struct dom_info *priv = to_dom_info(domain);

	return domain_unmap_page(priv, vaddr, len);
}

static phys_addr_t _iommu_iova_to_phys(struct dom_info *info, dma_addr_t vaddr)
{
	unsigned long *ptep;
	unsigned long page_size, page_mask;
	phys_addr_t paddr;

	mutex_lock(&info->ptl_lock);
	ptep = iommu_get_pte(info->pgd, vaddr, IOMMU_PT_LEVEL0);
	mutex_unlock(&info->ptl_lock);

	if (!ptep || !iommu_pte_present(ptep)) {
		pr_warn_once(
	"LA-IOMMU: shadow pte is null or not present with vaddr %llx\n",
	vaddr);
		paddr = 0;
		return paddr;
	}

	if (iommu_pte_huge(ptep)) {
		page_size = IOMMU_HPAGE_SIZE;
		page_mask = IOMMU_HPAGE_MASK;
	} else {
		page_size = IOMMU_PAGE_SIZE;
		page_mask = IOMMU_PAGE_MASK;
	}
	paddr = *ptep & page_mask;
	paddr |= vaddr & (page_size - 1);
	return paddr;
}

static phys_addr_t la_iommu_iova_to_phys(struct iommu_domain *domain,
					dma_addr_t vaddr)
{
	struct dom_info *priv = to_dom_info(domain);
	phys_addr_t phys;

	spin_lock(&priv->lock);
	phys = _iommu_iova_to_phys(priv, vaddr);
	spin_unlock(&priv->lock);
	return phys;
}

static void la_domain_set_plaform_dma_ops(struct device *dev)
{
	/*
	 * loongarch doesn't setup default domains because we can't hook into the
	 * normal probe path
	 */
}

const struct iommu_ops la_iommu_ops = {
	.capable = la_iommu_capable,
	.domain_alloc = la_iommu_domain_alloc,
	.probe_device = la_iommu_probe_device,
	.release_device = la_iommu_remove_device,
	.device_group = la_iommu_device_group,
	.pgsize_bitmap	= LA_IOMMU_PGSIZE,
	.owner = THIS_MODULE,
	.set_platform_dma_ops = la_domain_set_plaform_dma_ops,
	.default_domain_ops = &(const struct iommu_domain_ops) {
		.attach_dev	= la_iommu_attach_dev,
		.map = la_iommu_map,
		.unmap = la_iommu_unmap,
		.iova_to_phys	= la_iommu_iova_to_phys,
		.flush_iotlb_all = la_iommu_flush_iotlb_all,
		.free		= la_iommu_domain_free,
	}
};


struct loongarch_iommu *loongarch_get_iommu_by_devid(struct pci_dev *pdev)
{
	int pcisegment;
	unsigned short devid;
	struct loongarch_iommu *iommu = NULL;
	struct pci_bus	*bus = pdev->bus;

	devid = PCI_DEVID(bus->number, pdev->devfn);
	pcisegment = pci_domain_nr(pdev->bus);
	list_for_each_entry(iommu, &la_iommu_list, list) {
		if ((iommu->segment == pcisegment) &&
		    (iommu->devid == devid)) {
			return iommu;
		}
	}
	return NULL;
}

bool check_device_compat(struct pci_dev *pdev)
{
	bool compat = true;

	if ((pdev->revision == 0) && (pdev->device == 0x7a1f))
		compat = false;
	return compat;
}

static int loongarch_iommu_probe(struct pci_dev *pdev,
				const struct pci_device_id *ent)
{
	int ret = 1;
	int bitmap_sz = 0;
	int tmp;
	bool compat = false;
	struct loongarch_iommu *iommu = NULL;
	resource_size_t base, size;

	iommu = loongarch_get_iommu_by_devid(pdev);
	if (iommu == NULL) {
		pci_info(pdev, "%s can't find iommu\n", __func__);
		return -ENODEV;
	}

	compat = check_device_compat(pdev);
	if (!compat) {
		iommu->disabled = true;
		pci_info(pdev,
		"%s The iommu driver is not compatible with this device, disabled!!!\n",
		__func__);
		return -ENODEV;
	}

	iommu->pdev = pdev;
	base = pci_resource_start(pdev, 0);
	size = pci_resource_len(pdev, 0);
	if (!request_mem_region(base, size, "loongarch_iommu")) {
		pci_err(pdev,
		"%d can't reserve mmio registers base %llx size %llx\n",
		__LINE__, base, size);
		return -ENOMEM;
	}
	iommu->confbase_phy = base;
	iommu->conf_size = size;
	iommu->confbase = ioremap(base, size);
	if (iommu->confbase == NULL) {
		pci_info(pdev, "%s iommu pci dev bar0 is NULL\n", __func__);
		return ret;
	}

	pr_info("iommu confbase %llx pgtsize %llx\n",
			(u64)iommu->confbase, size);
	tmp = MAX_DOMAIN_ID / 8;
	bitmap_sz = (MAX_DOMAIN_ID % 8) ? (tmp + 1) : tmp;
	iommu->domain_bitmap = bitmap_zalloc(bitmap_sz, GFP_KERNEL);
	if (iommu->domain_bitmap == NULL) {
		pr_err("LA-IOMMU: domain bitmap alloc err bitmap_sz:%d\n",
								bitmap_sz);
		goto out_err;
	}

	tmp = MAX_ATTACHED_DEV_ID / 8;
	bitmap_sz = (MAX_ATTACHED_DEV_ID % 8) ? (tmp + 1) : tmp;
	iommu->devtable_bitmap = bitmap_zalloc(bitmap_sz, GFP_KERNEL);
	if (iommu->devtable_bitmap == NULL) {
		pr_err("LA-IOMMU: devtable bitmap alloc err bitmap_sz:%d\n",
								bitmap_sz);
		goto out_err_1;
	}

	ret = iommu_device_sysfs_add(&iommu->iommu_dev, &pdev->dev,
			NULL, "ivhd-%#x-%#x", iommu->segment, iommu->devid);
	iommu_device_register(&iommu->iommu_dev, &la_iommu_ops, NULL);
	return 0;

out_err_1:
	iommu->pdev = NULL;
	iounmap(iommu->confbase);
	iommu->confbase = NULL;
	release_mem_region(iommu->confbase_phy, iommu->conf_size);
	iommu->confbase_phy = 0;
	iommu->conf_size = 0;
	kfree(iommu->domain_bitmap);
	iommu->domain_bitmap = NULL;
out_err:
	return ret;
}

static void loongarch_iommu_remove(struct pci_dev *pdev)
{
	struct  loongarch_iommu *iommu = NULL;

	iommu = loongarch_get_iommu_by_devid(pdev);
	if (iommu == NULL)
		return;
	if (iommu->domain_bitmap != NULL) {
		kfree(iommu->domain_bitmap);
		iommu->domain_bitmap = NULL;
	}
	if (iommu->devtable_bitmap != NULL) {
		kfree(iommu->devtable_bitmap);
		iommu->devtable_bitmap = NULL;
	}
	if (iommu->confbase != NULL) {
		iounmap(iommu->confbase);
		iommu->confbase = NULL;
	}
	if (iommu->confbase_phy != 0) {
		release_mem_region(iommu->confbase_phy, iommu->conf_size);
		iommu->confbase_phy = 0;
		iommu->conf_size = 0;
	}
}

static int __init check_ivrs_checksum(struct acpi_table_header *table)
{
	int i;
	u8 checksum = 0, *p = (u8 *)table;

	for (i = 0; i < table->length; ++i)
		checksum += p[i];
	if (checksum != 0) {
		/* ACPI table corrupt */
		pr_err("IVRS invalid checksum\n");
		return -ENODEV;
	}
	return 0;
}

struct iommu_rlookup_entry *create_rlookup_entry(int pcisegment)
{
	struct iommu_rlookup_entry *rlookupentry = NULL;

	rlookupentry = kzalloc(sizeof(struct iommu_rlookup_entry),
			GFP_KERNEL);
	if (rlookupentry == NULL)
		return rlookupentry;

	rlookupentry->pcisegment = pcisegment;
	/* IOMMU rlookup table - find the IOMMU for a specific device */
	rlookupentry->rlookup_table = (void *)__get_free_pages(
			GFP_KERNEL | __GFP_ZERO,
			get_order(rlookup_table_size));
	if (rlookupentry->rlookup_table == NULL) {
		kfree(rlookupentry);
		rlookupentry = NULL;
	} else {
		list_add(&rlookupentry->list, &la_rlookup_iommu_list);
	}
	return rlookupentry;
}

/* Writes the specific IOMMU for a device into the rlookup table */
static void __init set_iommu_for_device(struct loongarch_iommu *iommu,
		u16 devid)
{
	struct iommu_rlookup_entry *rlookupentry = NULL;

	rlookupentry = lookup_rlooptable(iommu->segment);
	if (rlookupentry == NULL)
		rlookupentry = create_rlookup_entry(iommu->segment);
	if (rlookupentry != NULL)
		rlookupentry->rlookup_table[devid] = iommu;
}

static inline u32 get_ivhd_header_size(struct ivhd_header *h)
{
	u32 size = 0;

	switch (h->type) {
	case IVHD_HEAD_TYPE10:
		size = 24;
		break;
	case IVHD_HEAD_TYPE11:
	case IVHD_HEAD_TYPE40:
		size = 40;
		break;
	}
	return size;
}

static inline void update_last_devid(u16 devid)
{
	if (devid > la_iommu_last_bdf)
		la_iommu_last_bdf = devid;
}

/*
 * This function calculates the length of a given IVHD entry
 */
static inline int ivhd_entry_length(u8 *ivhd)
{
	u32 type = ((struct ivhd_entry *)ivhd)->type;

	if (type < 0x80) {
		return 0x04 << (*ivhd >> 6);
	} else if (type == IVHD_DEV_ACPI_HID) {
		/* For ACPI_HID, offset 21 is uid len */
		return *((u8 *)ivhd + 21) + 22;
	}
	return 0;
}

/*
 * After reading the highest device id from the IOMMU PCI capability header
 * this function looks if there is a higher device id defined in the ACPI table
 */
static int __init find_last_devid_from_ivhd(struct ivhd_header *h)
{
	u8 *p = (void *)h, *end = (void *)h;
	struct ivhd_entry *dev;

	u32 ivhd_size = get_ivhd_header_size(h);

	if (!ivhd_size) {
		pr_err("la-iommu: Unsupported IVHD type %#x\n", h->type);
		return -EINVAL;
	}

	p += ivhd_size;
	end += h->length;

	while (p < end) {
		dev = (struct ivhd_entry *)p;
		switch (dev->type) {
		case IVHD_DEV_ALL:
			/* Use maximum BDF value for DEV_ALL */
			update_last_devid(MAX_BDF_NUM);
			break;
		case IVHD_DEV_SELECT:
		case IVHD_DEV_RANGE_END:
		case IVHD_DEV_ALIAS:
		case IVHD_DEV_EXT_SELECT:
			/* all the above subfield types refer to device ids */
			update_last_devid(dev->devid);
			break;
		default:
			break;
		}
		p += ivhd_entry_length(p);
	}

	WARN_ON(p != end);

	return 0;
}

/*
 * Iterate over all IVHD entries in the ACPI table and find the highest device
 * id which we need to handle. This is the first of three functions which parse
 * the ACPI table. So we check the checksum here.
 */
static int __init find_last_devid_acpi(struct acpi_table_header *table)
{
	u8 *p = (u8 *)table, *end = (u8 *)table;
	struct ivhd_header *h;

	p += IVRS_HEADER_LENGTH;

	end += table->length;
	while (p < end) {
		h = (struct ivhd_header *)p;

		if (h->length == 0)
			break;

		if (h->type == la_iommu_target_ivhd_type) {
			int ret = find_last_devid_from_ivhd(h);

			if (ret)
				return ret;
		}

		p += h->length;
	}

	if (p != end)
		return -EINVAL;
	return 0;
}

/*
 * Takes a pointer to an loongarch IOMMU entry in the ACPI table and
 * initializes the hardware and our data structures with it.
 */
static int __init init_iommu_from_acpi(struct loongarch_iommu *iommu,
					struct ivhd_header *h)
{
	u8 *p = (u8 *)h;
	u8 *end = p;
	u16 devid = 0, devid_start = 0;
	u32 dev_i;
	struct ivhd_entry *e;
	u32 ivhd_size;

	/*
	 * Done. Now parse the device entries
	 */
	ivhd_size = get_ivhd_header_size(h);
	if (!ivhd_size) {
		pr_err("loongarch iommu: Unsupported IVHD type %#x\n", h->type);
		return -EINVAL;
	}

	if (h->length == 0)
		return -EINVAL;

	p += ivhd_size;
	end += h->length;

	while (p < end) {
		e = (struct ivhd_entry *)p;
		switch (e->type) {
		case IVHD_DEV_ALL:
			for (dev_i = 0; dev_i <= la_iommu_last_bdf; ++dev_i)
				set_iommu_for_device(iommu, dev_i);
			break;
		case IVHD_DEV_SELECT:

			pr_info("  DEV_SELECT\t\t\t devid: %02x:%02x.%x\n",
				    PCI_BUS_NUM(e->devid),
				    PCI_SLOT(e->devid),
				    PCI_FUNC(e->devid));

			devid = e->devid;
			set_iommu_for_device(iommu, devid);
			break;
		case IVHD_DEV_SELECT_RANGE_START:

			pr_info("  DEV_SELECT_RANGE_START\t devid: %02x:%02x.%x\n",
				    PCI_BUS_NUM(e->devid),
				    PCI_SLOT(e->devid),
				    PCI_FUNC(e->devid));

			devid_start = e->devid;
			break;
		case IVHD_DEV_RANGE_END:

			pr_info("  DEV_RANGE_END\t\t devid: %02x:%02x.%x\n",
				    PCI_BUS_NUM(e->devid),
				    PCI_SLOT(e->devid),
				    PCI_FUNC(e->devid));

			devid = e->devid;
			for (dev_i = devid_start; dev_i <= devid; ++dev_i)
				set_iommu_for_device(iommu, dev_i);
			break;
		default:
			break;
		}

		p += ivhd_entry_length(p);
	}

	return 0;
}

/*
 * This function clues the initialization function for one IOMMU
 * together and also allocates the command buffer and programs the
 * hardware. It does NOT enable the IOMMU. This is done afterwards.
 */
static int __init init_iommu_one(struct loongarch_iommu *iommu,
		struct ivhd_header *h)
{
	int ret;
	struct iommu_rlookup_entry *rlookupentry = NULL;

	spin_lock_init(&iommu->domain_bitmap_lock);
	spin_lock_init(&iommu->dom_info_lock);

	/* Add IOMMU to internal data structures */
	INIT_LIST_HEAD(&iommu->dom_list);

	list_add_tail(&iommu->list, &la_iommu_list);

	/*
	 * Copy data from ACPI table entry to the iommu struct
	 */
	iommu->devid   = h->devid;
	iommu->segment = h->pci_seg;
	ret = init_iommu_from_acpi(iommu, h);
	if (ret) {
		pr_err("%s init iommu from acpi failed\n", __func__);
		return ret;
	}
	rlookupentry = lookup_rlooptable(iommu->segment);
	if (rlookupentry != NULL) {
		/*
		 * Make sure IOMMU is not considered to translate itself.
		 * The IVRS table tells us so, but this is a lie!
		 */
		rlookupentry->rlookup_table[iommu->devid] = NULL;
	}
	return 0;
}

/*
 * Iterates over all IOMMU entries in the ACPI table, allocates the
 * IOMMU structure and initializes it with init_iommu_one()
 */
static int __init init_iommu_all(struct acpi_table_header *table)
{
	u8 *p = (u8 *)table, *end = (u8 *)table;
	struct ivhd_header *h;
	struct loongarch_iommu *iommu;
	int ret;

	end += table->length;
	p += IVRS_HEADER_LENGTH;

	while (p < end) {
		h = (struct ivhd_header *)p;

		if (h->length == 0)
			break;

		if (h->type == la_iommu_target_ivhd_type) {

			pr_info("device: %02x:%02x.%01x seg: %d\n",
				    PCI_BUS_NUM(h->devid), PCI_SLOT(h->devid),
				    PCI_FUNC(h->devid), h->pci_seg);

			iommu = kzalloc(sizeof(struct loongarch_iommu),
					GFP_KERNEL);
			if (iommu == NULL)
				return -ENOMEM;

			ret = init_iommu_one(iommu, h);
			if (ret) {
				kfree(iommu);
				pr_info("%s init iommu failed\n", __func__);
				return ret;
			}
		}
		p += h->length;
	}
	if (p != end)
		return -EINVAL;
	return 0;
}

/**
 * get_highest_supported_ivhd_type - Look up the appropriate IVHD type
 * @ivrs	Pointer to the IVRS header
 *
 * This function search through all IVDB of the maximum supported IVHD
 */
static u8 get_highest_supported_ivhd_type(struct acpi_table_header *ivrs)
{
	u8 *base = (u8 *)ivrs;
	struct ivhd_header *ivhd = (struct ivhd_header *)
					(base + IVRS_HEADER_LENGTH);
	u8 last_type = ivhd->type;
	u16 devid = ivhd->devid;

	while (((u8 *)ivhd - base < ivrs->length) &&
	       (ivhd->type <= ACPI_IVHD_TYPE_MAX_SUPPORTED) &&
	       (ivhd->length > 0)) {
		u8 *p = (u8 *) ivhd;

		if (ivhd->devid == devid)
			last_type = ivhd->type;
		ivhd = (struct ivhd_header *)(p + ivhd->length);
	}
	return last_type;
}

static inline unsigned long tbl_size(int entry_size)
{
	unsigned int shift = PAGE_SHIFT +
			 get_order(((int)la_iommu_last_bdf + 1) * entry_size);

	return 1UL << shift;
}

static int __init loongarch_iommu_ivrs_init(void)
{
	struct acpi_table_header *ivrs_base;
	acpi_status status;
	int ret = 0;

	status = acpi_get_table("IVRS", 0, &ivrs_base);
	if (status == AE_NOT_FOUND) {
		pr_info("%s get ivrs table failed\n", __func__);
		return -ENODEV;
	}

	/*
	 * Validate checksum here so we don't need to do it when
	 * we actually parse the table
	 */
	ret = check_ivrs_checksum(ivrs_base);
	if (ret)
		goto out;

	la_iommu_target_ivhd_type = get_highest_supported_ivhd_type(ivrs_base);
	pr_info("Using IVHD type %#x\n", la_iommu_target_ivhd_type);

	/*
	 * First parse ACPI tables to find the largest Bus/Dev/Func
	 * we need to handle. Upon this information the shared data
	 * structures for the IOMMUs in the system will be allocated
	 */
	ret = find_last_devid_acpi(ivrs_base);
	if (ret) {
		pr_err("%s find last devid failed\n", __func__);
		goto out;
	}

	rlookup_table_size = tbl_size(RLOOKUP_TABLE_ENTRY_SIZE);

	/*
	 * now the data structures are allocated and basically initialized
	 * start the real acpi table scan
	 */
	ret = init_iommu_all(ivrs_base);
out:
	/* Don't leak any ACPI memory */
	acpi_put_table(ivrs_base);
	ivrs_base = NULL;
	return ret;
}

static void free_iommu_rlookup_entry(void)
{
	struct loongarch_iommu *iommu = NULL;
	struct iommu_rlookup_entry *rlookupentry = NULL;

	while (!list_empty(&la_iommu_list)) {
		iommu = list_first_entry(&la_iommu_list, struct loongarch_iommu, list);
		list_del(&iommu->list);
		kfree(iommu);
	}

	while (!list_empty(&la_rlookup_iommu_list)) {
		rlookupentry = list_first_entry(&la_rlookup_iommu_list,
				struct iommu_rlookup_entry, list);

		list_del(&rlookupentry->list);
		if (rlookupentry->rlookup_table != NULL) {
			free_pages(
			(unsigned long)rlookupentry->rlookup_table,
			get_order(rlookup_table_size));

			rlookupentry->rlookup_table = NULL;
		}
		kfree(rlookupentry);
	}
}

static int __init __maybe_unused la_iommu_setup(char *str)
{
	if (!str)
		return -EINVAL;
	while (*str) {
		if (!strncmp(str, "on", 2)) {
			loongarch_iommu_disable = 0;
			pr_info("IOMMU enabled\n");
		} else if (!strncmp(str, "off", 3)) {
			loongarch_iommu_disable = 1;
			pr_info("IOMMU disabled\n");
		}
		str += strcspn(str, ",");
		while (*str == ',')
			str++;
	}
	return 0;
}
__setup("loongarch_iommu=", la_iommu_setup);

static const struct pci_device_id loongson_iommu_pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_LOONGSON, 0x3c0f) },
	{ PCI_DEVICE(PCI_VENDOR_ID_LOONGSON, 0x7a1f) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, loongson_iommu_pci_tbl);

static struct pci_driver loongarch_iommu_driver = {
	.name = "loongarch-iommu",
	.id_table = loongson_iommu_pci_tbl,
	.probe	= loongarch_iommu_probe,
	.remove	= loongarch_iommu_remove,
};

static int __init loongarch_iommu_driver_init(void)
{
	int ret = 0;

	if (loongarch_iommu_disable == 0) {
		ret = loongarch_iommu_ivrs_init();
		if (ret != 0) {
			free_iommu_rlookup_entry();
			pr_err("Failed to init iommu by ivrs\n");
		}

		ret = pci_register_driver(&loongarch_iommu_driver);
		if (ret != 0) {
			pr_err("Failed to register IOMMU driver\n");
			return ret;
		}
	}
	return ret;
}

static void __exit loongarch_iommu_driver_exit(void)
{
	struct loongarch_iommu *iommu = NULL;

	if (loongarch_iommu_disable == 0) {
		list_for_each_entry(iommu, &la_iommu_list, list) {
			iommu_device_sysfs_remove(&iommu->iommu_dev);
			iommu_device_unregister(&iommu->iommu_dev);
			loongarch_iommu_remove(iommu->pdev);
		}
		free_iommu_rlookup_entry();
		pci_unregister_driver(&loongarch_iommu_driver);
	}
}

module_init(loongarch_iommu_driver_init);
module_exit(loongarch_iommu_driver_exit);
