// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include <linux/inet.h>
#include <linux/ctype.h>
#include "main.h"
#ifdef SET_DMABUF
#include <linux/dma-buf.h>
#include <linux/dma-resv.h>
#endif
#include "icrdma_hw.h"
#include "srq.h"
#include "restrack.h"
#include "private_verbs_cmd.h"
#include "manager.h"
#include "slib.h"

#ifdef ZXDH_UAPI_DEF
extern const struct uapi_definition zxdh_ib_dev_defs[];
#endif

static int zxdh_get_port_traffic_class(struct zxdh_device *iwdev,
				       u8 port_num, u8 *traffic_class);

static void zxdh_pmtu_add_worker(struct work_struct *work)
{
	struct zxdh_pmtu_add_work *dwork =
		container_of(work, struct zxdh_pmtu_add_work, work);

	zxdh_dpp_tbl_pmtu_info_add(dwork->rf, &dwork->pf_info, dwork->pmtu, dwork->qp_num);
	kfree(dwork);
}

void extract_version(const char *input, char *output)
{
	const char *v_start, *p;
	int dots = 0;

	if (!output || !input)
		goto fail;

	/* Check if format is "26.20.01.03 (ZTE_05632)" - version followed by vendor in parentheses */
	if (strstr(input, " (")) {
		v_start = input;
		for (p = v_start; *p && *p != ' '; p++) {
			if (*p == '.' && ++dots == 4)
				break;
			*output++ = *p;
		}
	} else {
		/* Format: "E310-V25.30.103.01" - find 'V' after last '-' */
		const char *last_dash = strrchr(input, '-');
		if (!last_dash)
			goto fail;
		v_start = strstr(last_dash, "V");
		if (!v_start)
			goto fail;
		v_start++;  /* skip 'V' */

		for (p = v_start; *p && dots < 4; p++) {
			if (*p == '.' && ++dots == 4)
				break;
			*output++ = *p;
		}
	}

fail:
	if (output)
		*output = '\0';
}
/**
 * zxdh_query_device - get device attributes
 * @ibdev: device pointer from stack
 * @props: returning device attributes
 * @udata: user data
 */
static int zxdh_query_device(struct ib_device *ibdev,
			     struct ib_device_attr *props,
			     struct ib_udata *udata)
{
	struct zxdh_device *iwdev = to_iwdev(ibdev);
	struct zxdh_pci_f *rf = iwdev->rf;
	struct pci_dev *pcidev = iwdev->rf->pcidev;
	struct zxdh_hw_attrs *hw_attrs = &rf->sc_dev.hw_attrs;

	struct ethtool_drvinfo info;
	int major, sub_major, minor, sub_minor;

	__u32 val;
	__u16 unit_period;
	char extracted_version[32];
	struct net_device *slave = NULL;
	struct list_head *iter;

	memset(&info, 0, sizeof(info));
	if ((iwdev->netdev->priv_flags & IFF_BONDING) == 4) {
		rcu_read_lock();
		netdev_for_each_lower_dev(iwdev->netdev, slave, iter) {
			slave->ethtool_ops->get_drvinfo(slave, &info);
			break;
		}
		rcu_read_unlock();
		if (!slave) {
			iwdev->netdev->ethtool_ops->get_drvinfo(iwdev->netdev, &info);
		}
	} else {
		iwdev->netdev->ethtool_ops->get_drvinfo(iwdev->netdev, &info);
	}
	extract_version(info.fw_version, extracted_version);
	sscanf(extracted_version, "%d.%d.%d.%d", &major, &sub_major, &minor,
	       &sub_minor);

	if (udata->inlen || udata->outlen)
		return -EINVAL;

	memset(props, 0, sizeof(*props));
	ether_addr_copy((u8 *)&props->sys_image_guid, iwdev->netdev->dev_addr);
	props->fw_ver = ((u64)major << 48 | (u64)sub_major << 32 |
			 (u64)minor << 16 | sub_minor);
	props->device_cap_flags =
		IB_DEVICE_MEM_WINDOW | IB_DEVICE_MEM_MGT_EXTENSIONS |
		IB_DEVICE_BAD_QKEY_CNTR | IB_DEVICE_SYS_IMAGE_GUID |
		IB_DEVICE_RC_RNR_NAK_GEN | IB_DEVICE_N_NOTIFY_CQ;
#ifdef IB_DEV_CAPS_VER_2
	props->kernel_cap_flags = IBK_LOCAL_DMA_LKEY;
#else
	props->device_cap_flags |= IB_DEVICE_LOCAL_DMA_LKEY;
#endif
	props->vendor_id = pcidev->vendor;
	props->vendor_part_id = pcidev->device;
	props->hw_ver = pcidev->revision;
	props->page_size_cap = SZ_4K | SZ_2M | SZ_1G;
	props->max_mr_size = hw_attrs->max_mr_size;
	props->max_qp = rf->max_qp - rf->used_qps;
	props->max_qp_wr = hw_attrs->max_qp_wr;
	set_max_sge(props, rf);
	props->max_cq = rf->max_cq - rf->used_cqs;
	props->max_cqe = rf->max_cqe;
	props->max_mr = rf->max_mr - rf->used_mrs;
	props->max_mw = props->max_mr;
	props->max_pd = rf->max_pd - rf->used_pds;
	props->max_sge_rd = hw_attrs->uk_attrs.max_hw_read_sges;
	props->max_qp_rd_atom = hw_attrs->max_hw_ird;
	props->max_res_rd_atom = props->max_qp_rd_atom * props->max_qp;
	props->max_qp_init_rd_atom = hw_attrs->max_hw_ord;
	props->max_srq = rf->max_srq - rf->used_srqs;
	if (rf->rdma_srq_mem_type != USER_L2D_KERNEL_L2D_V3)
		props->max_srq = ZXDH_PF_MAX_SRQ_NUM_USE_L2D - rf->used_srqs;
	pr_debug("[zxdh_rdma] %s[%d]: rf->max_srq=%d rf->used_srqs=%d\n", __func__, __LINE__, rf->max_srq, rf->used_srqs);

	props->max_srq_wr = hw_attrs->max_srq_wr;
	props->max_srq_sge = hw_attrs->uk_attrs.max_hw_wq_frags;
	props->local_ca_ack_delay = 16;
	props->hca_core_clock = 1000 * 1000UL;
	props->max_wq_type_rq = props->max_qp;
	if (rdma_protocol_roce(ibdev, 1)) {
		props->max_pkeys = ZXDH_PKEY_TBL_SZ;
		props->max_ah = rf->max_ah;
		if (hw_attrs->uk_attrs.hw_rev == ZXDH_GEN_2) {
			props->max_mcast_grp = 0;
			props->max_mcast_qp_attach = 0;
			props->max_total_mcast_qp_attach = 0;
		}
	}
	props->max_fast_reg_page_list_len = ZXDH_MAX_PAGES_PER_FMR;
	val = readl(rf->sc_dev.hw->hw_addr + RDMARX_CQ_PERIOD_CFG(rf->sc_dev.ep_id));
	unit_period = (__u16)(val & 0xffff);
	props->cq_caps.max_cq_moderation_count = ZXDH_MAX_CQ_COUNT;
	props->cq_caps.max_cq_moderation_period =
		NS_TO_US(unit_period * ZXDH_MAX_CQ_PERIOD);
#define HCA_CLOCK_TIMESTAMP_MASK 0x1ffff
	if (hw_attrs->uk_attrs.hw_rev >= ZXDH_GEN_2)
		props->timestamp_mask = HCA_CLOCK_TIMESTAMP_MASK;

	return 0;
}

static int zxdh_mmap_legacy(struct zxdh_ucontext *ucontext,
			    struct vm_area_struct *vma)
{
	u64 pfn;

	if (vma->vm_pgoff || vma->vm_end - vma->vm_start != PAGE_SIZE)
		return -EINVAL;

	vma->vm_private_data = ucontext;
	pfn = ((uintptr_t)ucontext->iwdev->rf->sc_dev
		       .hw_regs[ZXDH_DB_ADDR_OFFSET] +
	       pci_resource_start(ucontext->iwdev->rf->pcidev, 0)) >>
	      PAGE_SHIFT;

#ifdef RDMA_MMAP_DB_SUPPORT
	return rdma_user_mmap_io(&ucontext->ibucontext, vma, pfn, PAGE_SIZE,
				 pgprot_noncached(vma->vm_page_prot), NULL);
#else
	return rdma_user_mmap_io(&ucontext->ibucontext, vma, pfn, PAGE_SIZE,
				 pgprot_noncached(vma->vm_page_prot));
#endif
}

void *zxdh_zalloc_mapped(struct zxdh_device *dev, dma_addr_t *dma_addr,
			 size_t size, enum dma_data_direction dir)
{
	void *addr;

	addr = alloc_pages_exact(size, GFP_KERNEL | __GFP_ZERO);
	if (!addr)
		return NULL;
	*dma_addr = dma_map_single(&dev->rf->pcidev->dev, addr, size, dir);
	if (dma_mapping_error(&dev->rf->pcidev->dev, *dma_addr)) {
		pr_err("[zxdh_rdma] failed to map DMA address\n");
		free_pages_exact(addr, size);
		return NULL;
	}
	return addr;
}

void zxdh_free_mapped(struct zxdh_device *dev, void *cpu_addr,
		      dma_addr_t dma_addr, size_t size,
		      enum dma_data_direction dir)
{
	dma_unmap_single(&dev->rf->pcidev->dev, dma_addr, size, dir);
	free_pages_exact(cpu_addr, size);
}

static int zxdh_mmap_for_cap(struct zxdh_ucontext *ucontext,
			     struct vm_area_struct *vma,
			     struct zxdh_user_mmap_entry *entry)
{
	u64 pfn;
	u64 start = vma->vm_start;
	u64 size = vma->vm_end - vma->vm_start;

	pfn = entry->bar_offset >> ZXDH_HW_PAGE_SHIFT;

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		pr_info("[zxdh_rdma] %s error!\n", __func__);
		return -EAGAIN;
	}

	pr_info("[zxdh_rdma] %s remap_pfn_range end.start:%llx,size:%llx\n", __func__,
		start, size);
	return 0;
}

static int zxdh_mmap_hmc_for_cap(struct zxdh_ucontext *ucontext,
				 struct vm_area_struct *vma,
				 struct zxdh_user_mmap_entry *entry)
{
	u64 size;
	u64 i;
	struct zxdh_sc_dev *dev;
	u64 j = 0;
	int ret = 0;
	u64 mem_phy;
	unsigned long addr;
	u64 numbufs;
	dev = &ucontext->iwdev->rf->sc_dev;
	size = vma->vm_end - vma->vm_start;
	if (size > dev->data_cap_sd.data_len) {
		return -EINVAL;
	}
	numbufs = size / ZXDH_HMC_DIRECT_BP_SIZE;
	j = (entry->bar_offset - dev->data_cap_sd.data_cap_base) /
	    ZXDH_HMC_DIRECT_BP_SIZE;
	addr = vma->vm_start;
	for (i = 0; i < numbufs; i++, j++) {
		mem_phy = dev->data_cap_sd.entry[j].u.bp.addr.pa;
		ret = remap_pfn_range(vma, addr, mem_phy >> ZXDH_HW_PAGE_SHIFT,
				      ZXDH_HMC_DIRECT_BP_SIZE,
				      vma->vm_page_prot);
		if (ret < 0)
			break;
		addr += ZXDH_HMC_DIRECT_BP_SIZE;
	}
	pr_info("[zxdh_rdma] zxdh_mmap_hmc_for_cap remap_pfn_range end.start:%lx,size:%llx\n",
		vma->vm_start, size);
	return ret;
}

#ifdef RDMA_MMAP_DB_SUPPORT
static void zxdh_mmap_free(struct rdma_user_mmap_entry *rdma_entry)
{
	struct zxdh_user_mmap_entry *entry = to_zxdh_mmap_entry(rdma_entry);

	kfree(entry);
}

struct rdma_user_mmap_entry *
zxdh_user_mmap_entry_insert(struct zxdh_ucontext *ucontext, u64 bar_offset,
			    enum zxdh_mmap_flag mmap_flag, u64 *mmap_offset)
{
	struct zxdh_user_mmap_entry *entry =
		kzalloc(sizeof(*entry), GFP_KERNEL);
	int ret;

	if (!entry)
		return NULL;

	entry->bar_offset = bar_offset;
	entry->mmap_flag = mmap_flag;

	if (mmap_flag == ZXDH_MMAP_L2D) {
		ret = rdma_user_mmap_entry_insert(&ucontext->ibucontext,
					  	&entry->rdma_entry, ZXDH_BLUE_FLAME_QP_L2D_MEM_SIZE);
	} else {
		ret = rdma_user_mmap_entry_insert(&ucontext->ibucontext,
						&entry->rdma_entry, PAGE_SIZE);
	}

	if (ret) {
		kfree(entry);
		return NULL;
	}
	*mmap_offset = rdma_user_mmap_get_offset(&entry->rdma_entry);

	return &entry->rdma_entry;
}

struct rdma_user_mmap_entry *
zxdh_mp_mmap_entry_insert(struct zxdh_ucontext *ucontext, u64 phy_addr,
			  size_t length, enum zxdh_mmap_flag mmap_flag,
			  u64 *mmap_offset)
{
	struct zxdh_user_mmap_entry *entry =
		kzalloc(sizeof(*entry), GFP_KERNEL);
	int ret;

	if (!entry)
		return NULL;

	entry->bar_offset = phy_addr;
	entry->mmap_flag = mmap_flag;
	pr_debug("[zxdh_rdma] %s[%d]: phy_addr=0x%llx length=%zx mmap_flag=%d, bar_offset=0x%llx\n", __func__, __LINE__, phy_addr, length, mmap_flag, entry->bar_offset);

	ret = rdma_user_mmap_entry_insert(&ucontext->ibucontext,
					  &entry->rdma_entry, length);
	if (ret) {
		kfree(entry);
		return NULL;
	}
	*mmap_offset = rdma_user_mmap_get_offset(&entry->rdma_entry);

	return &entry->rdma_entry;
}

struct rdma_user_mmap_entry *
zxdh_cap_mmap_entry_insert(struct zxdh_ucontext *ucontext, void *address,
			   size_t length, enum zxdh_mmap_flag mmap_flag,
			   u64 *mmap_offset)
{
	struct zxdh_user_mmap_entry *entry =
		kzalloc(sizeof(*entry), GFP_KERNEL);
	int ret;

	if (!entry)
		return NULL;
	if (mmap_flag == ZXDH_MMAP_HMC) {
		entry->bar_offset = (uintptr_t)address;
	} else {
		if (is_vmalloc_addr(address)) {
			struct page *p = vmalloc_to_page(address);
			entry->bar_offset = p ? page_to_phys(p) : virt_to_phys(address);
		} else {
			entry->bar_offset = virt_to_phys(address);
		}
	}
	entry->mmap_flag = mmap_flag;
	pr_debug("[zxdh_rdma] %s[%d]: address=0x%llx length=%zx mmap_flag=%d, bar_offset=0x%llx\n", __func__, __LINE__, (u64)address, length, mmap_flag, entry->bar_offset);

	ret = rdma_user_mmap_entry_insert(&ucontext->ibucontext,
					  &entry->rdma_entry, length);
	if (ret) {
		pr_err("[zxdh_rdma] %s entry insert failed\n", __func__);
		kfree(entry);
		return NULL;
	}
	*mmap_offset = rdma_user_mmap_get_offset(&entry->rdma_entry);

	return &entry->rdma_entry;
}

#else /* RDMA_MMAP_DB_SUPPORT */
static inline bool find_key_in_mmap_tbl(struct zxdh_ucontext *ucontext, u64 key)
{
	struct zxdh_user_mmap_entry *entry;

	hash_for_each_possible(ucontext->mmap_hash_tbl, entry, hlist, key) {
		if (entry->pgoff_key == key)
			return true;
	}

	return false;
}

struct zxdh_user_mmap_entry *
zxdh_user_mmap_entry_add_hash(struct zxdh_ucontext *ucontext, u64 bar_offset,
			      enum zxdh_mmap_flag mmap_flag, u64 *mmap_offset)
{
	struct zxdh_user_mmap_entry *entry =
		kzalloc(sizeof(*entry), GFP_KERNEL);
	unsigned long flags;
	int retry_cnt = 0;

	if (!entry)
		return NULL;

	entry->bar_offset = bar_offset;
	entry->mmap_flag = mmap_flag;
	entry->ucontext = ucontext;
	do {
		get_random_bytes(&entry->pgoff_key, sizeof(entry->pgoff_key));

		/* The key is a page offset */
		entry->pgoff_key >>= PAGE_SHIFT;

		/*In the event of a collision in the hash table, retry a new key */
		spin_lock_irqsave(&ucontext->mmap_tbl_lock, flags);
		if (!find_key_in_mmap_tbl(ucontext, entry->pgoff_key)) {
			hash_add(ucontext->mmap_hash_tbl, &entry->hlist,
				 entry->pgoff_key);
			spin_unlock_irqrestore(&ucontext->mmap_tbl_lock, flags);
			goto hash_add_done;
		}
		spin_unlock_irqrestore(&ucontext->mmap_tbl_lock, flags);
	} while (retry_cnt++ < 10);

	kfree(entry);
	return NULL;

hash_add_done:
	/*libc mmap uses a byte offset */
	*mmap_offset = entry->pgoff_key << PAGE_SHIFT;

	return entry;
}

static struct zxdh_user_mmap_entry *
zxdh_find_user_mmap_entry(struct zxdh_ucontext *ucontext,
			  struct vm_area_struct *vma)
{
	struct zxdh_user_mmap_entry *entry;
	unsigned long flags;

	spin_lock_irqsave(&ucontext->mmap_tbl_lock, flags);
	hash_for_each_possible(ucontext->mmap_hash_tbl, entry, hlist,
			       vma->vm_pgoff) {
		if (entry->pgoff_key == vma->vm_pgoff) {
			spin_unlock_irqrestore(&ucontext->mmap_tbl_lock, flags);
			return entry;
		}
	}

	spin_unlock_irqrestore(&ucontext->mmap_tbl_lock, flags);

	return NULL;
}

void zxdh_user_mmap_entry_del_hash(struct zxdh_user_mmap_entry *entry)
{
	struct zxdh_ucontext *ucontext = entry->ucontext;
	unsigned long flags;

	spin_lock_irqsave(&ucontext->mmap_tbl_lock, flags);
	hash_del(&entry->hlist);
	spin_unlock_irqrestore(&ucontext->mmap_tbl_lock, flags);

	kfree(entry);
}

#endif /* RDMA_MMAP_DB_SUPPORT */
/**
 * zxdh_mmap - user memory map
 * @context: context created during alloc
 * @vma: kernel info for user memory map
 */
static int zxdh_mmap(struct ib_ucontext *context, struct vm_area_struct *vma)
{
#ifdef RDMA_MMAP_DB_SUPPORT
	struct rdma_user_mmap_entry *rdma_entry;
#endif
	struct zxdh_user_mmap_entry *entry;
	struct zxdh_ucontext *ucontext;
	u64 pfn;
	int ret;

	ucontext = to_ucontext(context);

	/* Legacy support with hard-coded mmap key */
	if (ucontext->legacy_mode)
		return zxdh_mmap_legacy(ucontext, vma);

	pr_debug("[zxdh_rdma] VERBS: pgoff[0x%lx] ucontext=0x%llx, size=0x%lx\n",
		vma->vm_pgoff, (u64)ucontext, vma->vm_end - vma->vm_start);
#ifdef RDMA_MMAP_DB_SUPPORT
	rdma_entry = rdma_user_mmap_entry_get(&ucontext->ibucontext, vma);
	if (!rdma_entry) {
		pr_err("[zxdh_rdma] VERBS: pgoff[0x%lx] does not have valid entry\n",
		       vma->vm_pgoff);
		return -EINVAL;
	}

	entry = to_zxdh_mmap_entry(rdma_entry);
#else
	entry = zxdh_find_user_mmap_entry(ucontext, vma);
	if (!entry) {
		pr_err("[zxdh_rdma] VERBS: pgoff[0x%lx] does not have valid entry\n",
		       vma->vm_pgoff);
		return -EINVAL;
	}
#endif

	pfn = (entry->bar_offset +
	       pci_resource_start(ucontext->iwdev->rf->pcidev, 0)) >>
	      PAGE_SHIFT;

	switch (entry->mmap_flag) {
	case ZXDH_MMAP_HMC:
		ret = zxdh_mmap_hmc_for_cap(ucontext, vma, entry);
		break;
	case ZXDH_MMAP_PFN:
		ret = zxdh_mmap_for_cap(ucontext, vma, entry);
		break;
	case ZXDH_MMAP_IO_NC:
#ifdef RDMA_MMAP_DB_SUPPORT
		ret = rdma_user_mmap_io(context, vma, pfn, PAGE_SIZE,
					pgprot_noncached(vma->vm_page_prot),
					rdma_entry);
#else
		ret = rdma_user_mmap_io(context, vma, pfn, PAGE_SIZE,
					pgprot_noncached(vma->vm_page_prot));
#endif
		break;
	case ZXDH_MMAP_IO_NC_BY_SIZE:
#ifdef RDMA_MMAP_DB_SUPPORT
		ret = rdma_user_mmap_io(context, vma, pfn, vma->vm_end - vma->vm_start,
					pgprot_noncached(vma->vm_page_prot),
					rdma_entry);
#else
		ret = rdma_user_mmap_io(context, vma, pfn, vma->vm_end - vma->vm_start,
					pgprot_noncached(vma->vm_page_prot));
#endif
		break;
	case ZXDH_MMAP_IO_WC:
#ifdef RDMA_MMAP_DB_SUPPORT
		ret = rdma_user_mmap_io(context, vma, pfn, PAGE_SIZE,
					pgprot_writecombine(vma->vm_page_prot),
					rdma_entry);
#else
		ret = rdma_user_mmap_io(context, vma, pfn, PAGE_SIZE,
					pgprot_writecombine(vma->vm_page_prot));
#endif
		break;
	case ZXDH_MMAP_L2D:
		#if defined(__x86_64__) || defined(__i386__)
			vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
		#elif defined(__aarch64__) || defined(__arm__)
			vma->vm_page_prot = pgprot_device(vma->vm_page_prot);
		#else
			vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		#endif

#ifdef RDMA_MMAP_DB_SUPPORT
		ret = rdma_user_mmap_io(context, vma, pfn, ZXDH_BLUE_FLAME_QP_L2D_MEM_SIZE,
					vma->vm_page_prot,
					rdma_entry);
#else
		ret = rdma_user_mmap_io(context, vma, pfn, ZXDH_BLUE_FLAME_QP_L2D_MEM_SIZE,
					vma->vm_page_prot);
#endif
		break;
	default:
        pr_err("[zxdh_rdma] VERBS: unsupported mmap_flag[%d]\n", entry->mmap_flag);
		ret = -EINVAL;
	}

	if (ret)
		pr_err("[zxdh_rdma] VERBS: bar_offset [0x%llx] mmap_flag[%d] err[%d]\n",
		       entry->bar_offset, entry->mmap_flag, ret);

#ifdef RDMA_MMAP_DB_SUPPORT
	rdma_user_mmap_entry_put(rdma_entry);
#endif

	return ret;
}

/**
 * zxdh_get_pbl - Retrieve pbl from a list given a virtual
 * address
 * @va: user virtual address
 * @pbl_list: pbl list to search in (QP's or CQ's)
 */
struct zxdh_pbl *zxdh_get_pbl(unsigned long va, struct list_head *pbl_list)
{
	struct zxdh_pbl *iwpbl;

	list_for_each_entry(iwpbl, pbl_list, list) {
		if (iwpbl->user_base == va) {
			list_del(&iwpbl->list);
			iwpbl->on_list = false;
			return iwpbl;
		}
	}

	return NULL;
}

/**
 * zxdh_clean_cqes - clean cq entries for qp
 * @iwqp: qp ptr (user or kernel)
 * @iwcq: cq ptr
 */
void zxdh_clean_cqes(struct zxdh_qp *iwqp, struct zxdh_cq *iwcq)
{
	struct zxdh_cq_uk *ukcq = &iwcq->sc_cq.cq_uk;
	unsigned long flags;

	spin_lock_irqsave(&iwcq->lock, flags);
	zxdh_uk_clean_cq(&iwqp->sc_qp.qp_uk, ukcq);
	spin_unlock_irqrestore(&iwcq->lock, flags);
}

/**
 * zxdh_setup_virt_qp - setup for allocation of virtual qp
 * @iwdev: zrdma device
 * @iwqp: qp ptr
 * @init_info: initialize info to return
 */
void zxdh_setup_virt_qp(struct zxdh_device *iwdev, struct zxdh_qp *iwqp,
			struct zxdh_qp_init_info *init_info)
{
	struct zxdh_pbl *iwpbl = iwqp->iwpbl;
	struct zxdh_qp_mr *qpmr = &iwpbl->qp_mr;
	struct zxdh_pci_f *rf = iwdev->rf;
	u8 __iomem *sq_va;
	u32 bf_idx = iwqp->sc_qp.bf_idx;

	iwqp->page = qpmr->sq_page;
	init_info->shadow_area_pa = qpmr->shadow;
	if (iwpbl->pbl_allocated) {
		init_info->virtual_map = true;
		init_info->sq_pa = qpmr->sq_pbl.idx;
		if (iwqp->is_srq == false)
			init_info->rq_pa = qpmr->rq_pbl.idx;
	} else {
		init_info->sq_pa = qpmr->sq_pbl.addr;
		if (iwqp->is_srq == false)
			init_info->rq_pa = qpmr->rq_pbl.addr;
	}

	if (iwqp->sc_qp.use_blue_flame_flag){

		// (1) 根据bf_qpn计算L2D中分配给当前蓝焰qp的sq和rq物理地址
		sq_va = iwdev->rf->hw.pci_hw_addr + iwdev->rf->ext_base_bar_offset + C_RDMA_BLUE_FLAME_QP_MEM(iwdev->rf->sc_dev.ep_id)
							+ bf_idx * ZXDH_MAX_BLUE_FLAME_SQRQ_SIZE;
		init_info->sq_pa = iwdev->rf->ext_l2d_addr_base + rf->sc_dev.hmc_fn_id * ZXDH_BLUE_FLAME_QP_L2D_MEM_SIZE 
							+ bf_idx * ZXDH_MAX_BLUE_FLAME_SQRQ_SIZE;
		init_info->rq_pa = init_info->sq_pa + ZXDH_MAX_BLUE_FLAME_SQ_SIZE;

		// (2) 蓝焰qp基于物理寻址
		init_info->virtual_map = 0;

		// (3) 清空sq rq的L2D空间
		memset_io(sq_va, 0, ZXDH_MAX_BLUE_FLAME_SQRQ_SIZE);
	}
}

/**
 * zxdh_setup_kmode_qp - setup initialization for kernel mode qp
 * @iwdev: iwarp device
 * @iwqp: qp ptr (user or kernel)
 * @info: initialize info to return
 * @init_attr: Initial QP create attributes
 */
int zxdh_setup_kmode_qp(struct zxdh_device *iwdev, struct zxdh_qp *iwqp,
			struct zxdh_qp_init_info *info,
			struct ib_qp_init_attr *init_attr)
{
	struct zxdh_dma_mem *mem = &iwqp->kqp.dma_mem;
	u32 sqdepth, rqdepth;
	u8 sqshift, rqshift;
	u32 size;
	int status;
	struct zxdh_qp_uk_init_info *ukinfo = &info->qp_uk_init_info;
	struct zxdh_uk_attrs *uk_attrs = &iwdev->rf->sc_dev.hw_attrs.uk_attrs;

	zxdh_get_sq_wqe_shift(uk_attrs, ukinfo->max_sq_frag_cnt,
			      ukinfo->max_inline_data, &sqshift);
	status = zxdh_get_sqdepth(uk_attrs->max_hw_wq_quanta, ukinfo->sq_size,
				  sqshift, &sqdepth);
	if (status)
		return status;
	if (iwqp->is_srq == false) {
		zxdh_get_rq_wqe_shift(uk_attrs, ukinfo->max_rq_frag_cnt,
				      &rqshift);

		status = zxdh_get_rqdepth(uk_attrs->max_hw_rq_quanta,
					  ukinfo->rq_size, rqshift, &rqdepth);
	}

	if (status)
		return status;

	ukinfo->sq_size = sqdepth >> sqshift;
	iwqp->kqp.sq_wrid_mem =
		kcalloc(sqdepth, sizeof(*iwqp->kqp.sq_wrid_mem), GFP_KERNEL);
	if (!iwqp->kqp.sq_wrid_mem) {
		pr_err("[zxdh_rdma] %s[%d]: iwqp->kqp.sq_wrid_mem kcalloc failed\n", __func__, __LINE__);
		return -ENOMEM;
	}
	if (iwqp->is_srq == false) {
		ukinfo->rq_size = rqdepth;
		iwqp->kqp.rq_wrid_mem = kcalloc(ukinfo->rq_size,
						sizeof(*iwqp->kqp.rq_wrid_mem),
						GFP_KERNEL);
		if (!iwqp->kqp.rq_wrid_mem) {
			kfree(iwqp->kqp.sq_wrid_mem);
			iwqp->kqp.sq_wrid_mem = NULL;
			pr_err("[zxdh_rdma] %s[%d]: iwqp->kqp.rq_wrid_mem kcalloc failed\n", __func__, __LINE__);
			return -ENOMEM;
		}
		ukinfo->rq_wrid_array = iwqp->kqp.rq_wrid_mem;
	}

	ukinfo->sq_wrtrk_array = iwqp->kqp.sq_wrid_mem;
	if (iwqp->is_srq == false)
		size = sqdepth * ZXDH_QP_SQ_WQE_MIN_SIZE +
		       (rqdepth << rqshift) * ZXDH_QP_RQ_WQE_MIN_SIZE;
	else
		size = sqdepth * ZXDH_QP_SQ_WQE_MIN_SIZE;
	size += (ZXDH_SHADOW_AREA_SIZE << 3);

	mem->size = ALIGN(size, 4096);
	if (iwdev->rf->use_ext_mem_flag)
	{
		mem->va = (void *)zxdh_get_ext_mem(iwdev->rf, mem->size, &mem->pa);
	}
	else
	    mem->va = dma_alloc_coherent(iwdev->rf->hw.device, mem->size, &mem->pa,
				     GFP_KERNEL);
	if (!mem->va) {
		kfree(iwqp->kqp.sq_wrid_mem);
		iwqp->kqp.sq_wrid_mem = NULL;
		kfree(iwqp->kqp.rq_wrid_mem);
		iwqp->kqp.rq_wrid_mem = NULL;
		pr_err("[zxdh_rdma] %s[%d]: mem->va dma_alloc_coherent failed\n", __func__, __LINE__);
		return -ENOMEM;
	}

	ukinfo->sq = mem->va;
	info->sq_pa = mem->pa;
	if (iwqp->is_srq == false) {
		ukinfo->rq = (struct zxdh_qp_rq_quanta *)&ukinfo->sq[sqdepth];
		info->rq_pa = info->sq_pa + (sqdepth * ZXDH_QP_SQ_WQE_MIN_SIZE);
		ukinfo->shadow_area = ukinfo->rq[rqdepth].elem;
		info->shadow_area_pa =
			info->rq_pa + (rqdepth * ZXDH_QP_RQ_WQE_MIN_SIZE);
	} else {
		ukinfo->shadow_area = (__le64 *)&ukinfo->sq[sqdepth];
		info->shadow_area_pa =
			info->sq_pa + (sqdepth * ZXDH_QP_SQ_WQE_MIN_SIZE);
	}
	set_64bit_val(ukinfo->shadow_area, 0, 0x8000);
	ukinfo->qp_id = iwqp->ibqp.qp_num;

	init_attr->cap.max_send_wr = (sqdepth - ZXDH_SQ_RSVD) >> sqshift;
	if (iwqp->is_srq == false)
		init_attr->cap.max_recv_wr = (rqdepth - ZXDH_RQ_RSVD) >>
					     rqshift;

	return 0;
}

int zxdh_cqp_create_qp_cmd(struct zxdh_qp *iwqp)
{
	struct zxdh_pci_f *rf = iwqp->iwdev->rf;
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request) {
		pr_err("[zxdh_rdma] %s[%d]: cqp request alloc memory failed\n", __func__, __LINE__);
		return -ENOMEM;
	}
	cqp_info = &cqp_request->info;
	cqp_info->cqp_cmd = ZXDH_OP_QP_CREATE;
	cqp_info->post_sq = 1;
	cqp_info->in.u.qp_create.qp = &iwqp->sc_qp;
	cqp_info->in.u.qp_create.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);

	return status;
}

void zxdh_roce_fill_and_set_qpctx_info(struct zxdh_qp *iwqp,
				       struct zxdh_qp_host_ctx_info *ctx_info)
{
	struct zxdh_device *iwdev = iwqp->iwdev;
	struct zxdh_sc_dev *dev = &iwdev->rf->sc_dev;
	struct zxdh_roce_offload_info *roce_info;
	struct zxdh_udp_offload_info *udp_info;

	udp_info = &iwqp->udp_info;
	udp_info->pmtu = zxdh_mtu_int_to_enum(
		iwdev->netdev->mtu);
	udp_info->cwnd = iwdev->roce_cwnd;
	udp_info->rexmit_thresh = 2;
	udp_info->rnr_nak_thresh = 2;
	udp_info->src_port = 0xc000;
	udp_info->dst_port = ROCE_V2_UDP_DPORT;
	if (iwqp->sc_qp.qp_uk.qp_type == ZXDH_QP_TYPE_ROCE_RC)
		udp_info->timeout = 0x1f;
	else
		udp_info->timeout = 0x0;
	roce_info = &iwqp->roce_info;
	ether_addr_copy(roce_info->mac_addr, iwdev->netdev->dev_addr);

	roce_info->rd_en = false;
	roce_info->wr_rdresp_en = false;
	roce_info->bind_en = true;
	roce_info->dcqcn_en = true; //dcqcn/ecn is set to default on
	roce_info->ecn_en = false;
	roce_info->rtomin = 5;

#if IS_ENABLED(CONFIG_CONFIGFS_FS)
	roce_info->dcqcn_en = iwdev->roce_dcqcn_en;
	roce_info->timely_en = iwdev->roce_timely_en;
	roce_info->dctcp_en = iwdev->roce_dctcp_en;
	roce_info->rtomin = iwdev->roce_rtomin;
	roce_info->rcv_no_icrc = iwdev->roce_no_icrc_en;
#endif
	roce_info->ack_credits = iwdev->roce_ackcreds;
	roce_info->ird_size = dev->hw_attrs.max_hw_ird;
	roce_info->ord_size = dev->hw_attrs.max_hw_ord;

	if (!iwqp->user_mode) {
		roce_info->priv_mode_en = true;
		roce_info->fast_reg_en = true;
		roce_info->udprivcq_en = true;
	}
	roce_info->roce_tver = 0;

	ctx_info->roce_info = &iwqp->roce_info;
	ctx_info->udp_info = &iwqp->udp_info;
	zxdh_sc_qp_setctx_roce(&iwqp->sc_qp, iwqp->host_ctx.va, ctx_info);
}

int zxdh_validate_qp_attrs(struct ib_qp_init_attr *init_attr,
			   struct zxdh_device *iwdev)
{
	struct zxdh_sc_dev *dev = &iwdev->rf->sc_dev;
	struct zxdh_uk_attrs *uk_attrs = &dev->hw_attrs.uk_attrs;

	if (init_attr->cap.max_inline_data > uk_attrs->max_hw_inline ||
	    init_attr->cap.max_send_sge > uk_attrs->max_hw_wq_frags) {
		pr_err("[zxdh_rdma] %s[%d]: invalid qp attributes\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (iwdev->rf->use_blue_flame_flag) {
		if (init_attr->cap.max_recv_sge > uk_attrs->max_hw_rq_sges) {
			pr_err("[zxdh_rdma] %s[%d]: "
				"max_recv_sge:%u is over max_hw_rq_sges:%u in blue_flame mode\n"
				"If you want to use RQ max SGE of %u, set use_blue_flame=false via dhtool\n",
				__func__, __LINE__, init_attr->cap.max_recv_sge,
				uk_attrs->max_hw_rq_sges, uk_attrs->max_hw_wq_frags);
			return -EINVAL;
		}
	} else {
		if (init_attr->cap.max_recv_sge > uk_attrs->max_hw_wq_frags) {
			pr_err("[zxdh_rdma] %s[%d]: invalid qp attributes\n", __func__, __LINE__);
			return -EINVAL;
		}
	}

	if (rdma_protocol_roce(&iwdev->ibdev, 1)) {
		if (init_attr->qp_type != IB_QPT_RC &&
		    init_attr->qp_type != IB_QPT_UD &&
		    init_attr->qp_type != IB_QPT_GSI) {
			pr_err("[zxdh_rdma] %s[%d]: invalid qp_type=%d\n", __func__, __LINE__, init_attr->qp_type);
			return -EOPNOTSUPP;
		}
	} else {
		pr_err("[zxdh_rdma] %s[%d]: rdma_protocol_roce failed, not Roce\n", __func__, __LINE__);
		return -EOPNOTSUPP;
	}

	return 0;
}

void zxdh_flush_worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct zxdh_qp *iwqp = container_of(dwork, struct zxdh_qp, dwork_flush);
	unsigned long flags;

	spin_lock_irqsave(
		&iwqp->lock,
		flags); /* Don't allow more posting while generating completions */
	zxdh_generate_flush_completions(iwqp);
	spin_unlock_irqrestore(&iwqp->lock, flags);
}

static int zxdh_get_ib_acc_flags(struct zxdh_qp *iwqp)
{
	int acc_flags = 0;

	if (rdma_protocol_roce(iwqp->ibqp.device, 1)) {
		if (iwqp->roce_info.wr_rdresp_en) {
			acc_flags |= IB_ACCESS_LOCAL_WRITE;
			acc_flags |= IB_ACCESS_REMOTE_WRITE;
		}
		if (iwqp->roce_info.rd_en)
			acc_flags |= IB_ACCESS_REMOTE_READ;
		if (iwqp->roce_info.bind_en)
			acc_flags |= IB_ACCESS_MW_BIND;
	} else {
		if (iwqp->iwarp_info.wr_rdresp_en) {
			acc_flags |= IB_ACCESS_LOCAL_WRITE;
			acc_flags |= IB_ACCESS_REMOTE_WRITE;
		}
		if (iwqp->iwarp_info.rd_en)
			acc_flags |= IB_ACCESS_REMOTE_READ;
		if (iwqp->iwarp_info.bind_en)
			acc_flags |= IB_ACCESS_MW_BIND;
	}
	return acc_flags;
}

/**
 * zxdh_query_qp - query qp attributes
 * @ibqp: qp pointer
 * @attr: attributes pointer
 * @attr_mask: Not used
 * @init_attr: qp attributes to return
 */
static int zxdh_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
			 int attr_mask, struct ib_qp_init_attr *init_attr)
{
	struct zxdh_qp *iwqp = to_iwqp(ibqp);
	struct zxdh_sc_qp *qp = &iwqp->sc_qp;
	struct zxdh_device *iwdev = iwqp->iwdev;
	struct zxdh_dma_mem qpc_buf;
	int err_code = 0;

	memset(attr, 0, sizeof(*attr));
	memset(init_attr, 0, sizeof(*init_attr));
	qpc_buf.va = NULL;

	qpc_buf.size = ALIGN(ZXDH_QP_CTX_SIZE, ZXDH_QPC_ALIGNMENT);
	qpc_buf.va = zxdh_alloc_raw_mem(iwdev, qpc_buf.size, &qpc_buf.pa);

	if (!qpc_buf.va) {
		pr_err("[zxdh_rdma] res qp entry failed:ENOMEM\n");
		return -ENOMEM;
	}

	err_code = zxdh_fill_qpc(&iwdev->rf->sc_dev, iwqp->sc_qp.qp_ctx_num, &qpc_buf);
	if (err_code) {
		pr_err("[zxdh_rdma] res qp entry raw fill qpc failed:%d\n", err_code);
		goto free_rsrc;
	}
	attr->path_mig_state = IB_MIG_MIGRATED;
	attr->qp_state = iwqp->ibqp_state;
	attr->cur_qp_state = iwqp->ibqp_state;
	attr->cap.max_send_wr = iwqp->max_send_wr;
	attr->cap.max_recv_wr = iwqp->max_recv_wr;
	attr->cap.max_inline_data = qp->qp_uk.max_inline_data;
	attr->cap.max_send_sge = qp->qp_uk.max_sq_frag_cnt;
	attr->cap.max_recv_sge = qp->qp_uk.max_rq_frag_cnt;
	attr->qp_access_flags = zxdh_get_ib_acc_flags(iwqp);
	attr->port_num = 1;
	if (iwqp->ibqp.qp_type == IB_QPT_RC) {
		attr->ah_attr = iwqp->roce_ah.av.attrs;
		attr->ah_attr.grh.sgid_attr = NULL;
	}

	if (rdma_protocol_roce(ibqp->device, 1)) {
		attr->path_mtu = iwqp->udp_info.pmtu;
		attr->qkey = iwqp->roce_info.qkey;
		attr->rq_psn = ZXDH_GET_QPC_ITEM(u32, qpc_buf.va,
						 ZXDH_QPC_SEND_EPSN_BYTE_OFFSET,
						 RDMAQPC_RX_EPSN);
		attr->sq_psn = ZXDH_GET_QPC_ITEM(u32, qpc_buf.va,
						 ZXDH_QPC_SEND_PSN_BYTE_OFFSET,
						 RDMAQPC_TX_PSN_NEXT);
		attr->dest_qp_num = iwqp->roce_info.dest_qp;
		attr->pkey_index = 0;//supported pkey_table size ZXDH_PKEY_TBL_SZ is 1
		attr->timeout = iwqp->udp_info.timeout;
		attr->retry_cnt = iwqp->udp_info.rexmit_thresh;
		attr->rnr_retry = iwqp->udp_info.rnr_nak_thresh;
		attr->min_rnr_timer = iwqp->udp_info.min_rnr_timer;
		attr->max_rd_atomic = iwqp->roce_info.ord_size;
		attr->max_dest_rd_atomic = iwqp->roce_info.ird_size;
	}

	init_attr->event_handler = iwqp->ibqp.event_handler;
	init_attr->qp_context = iwqp->ibqp.qp_context;
	init_attr->send_cq = iwqp->ibqp.send_cq;
	init_attr->recv_cq = iwqp->ibqp.recv_cq;
	init_attr->srq = iwqp->ibqp.srq;
	init_attr->cap = attr->cap;
free_rsrc:
	zxdh_free_raw_mem(iwdev, qpc_buf.va, qpc_buf.size, qpc_buf.pa);
	qpc_buf.va = NULL;
	return err_code;
}
#if !defined(KYLIN_RELEASE_CODE) && !defined(RHEL_8_6)
#ifndef __OFED_BUILD__ 
#ifdef ZXDH_SET_UDP_SPORT_BYFLOW_LABLE
#define ZRDMA_ROCE_UDP_ENCAP_VALID_PORT_MIN (0xC000)
#define ZRDMA_ROCE_UDP_ENCAP_VALID_PORT_OFFSET_MASK (0x3FFF)

static u16 rdma_flow_label_to_udp_sport(u32 fl)
{
	u32 fl_low = fl & 0x03FFF;
	u32 fl_high = fl & 0xFC000;

	fl_low ^= fl_high >> 14;

	return (u16)(fl_low | ZRDMA_ROCE_UDP_ENCAP_VALID_PORT_MIN);
}
#if 0
#define ZRDMA_GRH_FLOWLABEL_MASK (0x000FFFFF)

static u32 rdma_calc_flow_label(u32 lqpn, u32 rqpn)
{
	u64 fl = (u64)lqpn * rqpn;

	fl ^= fl >> 20;
	fl ^= fl >> 40;

	return (u32)(fl & ZRDMA_GRH_FLOWLABEL_MASK);
}
#endif
#endif
#endif
#endif

static u16 zxdh_get_udp_sport(struct zxdh_device *iwdev,
		      const struct rdma_ah_attr *ah,
		      struct ib_qp *ibqp, struct ib_udata *udata)
{
	u32 flow_label = ah->grh.flow_label;
	unsigned int port_sel;
	unsigned int step;
	u16 sport;

	if (flow_label)
		return rdma_flow_label_to_udp_sport(flow_label);

	step = atomic_add_return(1, &iwdev->sport_counter);
	if(iwdev->bond_slave_cnt == 1 ||iwdev->bond_slave_cnt == 0) {
		sport = (u16)(ZRDMA_UDP_SPORT_BASE + step % ZRDMA_UDP_SPORT_NUM);
		return sport;
	}

	if (udata) {
		struct zxdh_ucontext *ucontext =
			kc_rdma_udata_to_drv_context(ibqp, udata);

		port_sel = atomic_add_return(1, &ucontext->tx_port_affinity) % iwdev->bond_slave_cnt;
	} else {
		port_sel = atomic_add_return(1, &iwdev->tx_port_affinity) % iwdev->bond_slave_cnt;
	}
	sport = (u16)(ZRDMA_UDP_SPORT_BASE +
			(step * iwdev->bond_slave_cnt + port_sel) % ZRDMA_UDP_SPORT_NUM);

	pr_debug("[zxdh_rdma] port_sel = %d sport_counter = %d sport = %d\n", port_sel, step, sport);

	return sport;
}

/* Started by AICoder, pid:2ea6ei8461ec90014d8608e96018d730a1e59409 */
static void zxdh_init_qp_indices(struct zxdh_qp_uk *qp)
{
	u32 sq_ring_size;
	sq_ring_size = ZXDH_RING_SIZE(qp->sq_ring);
	ZXDH_RING_INIT(qp->sq_ring, sq_ring_size);
	ZXDH_RING_INIT(qp->initial_ring, sq_ring_size);
	qp->swqe_polarity = 0;
	qp->swqe_polarity_deferred = 1;
	qp->rwqe_polarity = 0;
	qp->rwqe_signature = 0;
	ZXDH_RING_INIT(qp->rq_ring, qp->rq_size);
}

static int zxdh_modify_qp_to_reset(struct zxdh_qp *iwqp , struct zxdh_modify_qp_info *info)
{
    struct zxdh_device *iwdev = iwqp->iwdev;
    struct zxdh_qp_host_ctx_info *ctx_info = &iwqp->ctx_info;

    info->qpc_tx_mask_low = RDMAQPC_MASK_INIT;
    info->qpc_tx_mask_high = RDMAQPC_MASK_INIT;
    info->qpc_rx_mask_low = RDMAQPC_MASK_INIT;
    info->qpc_rx_mask_high = RDMAQPC_MASK_INIT;

    ctx_info->next_qp_state = ZXDH_QPS_RESET;
    zxdh_sc_qp_setctx_roce(&iwqp->sc_qp, iwqp->host_ctx.va, ctx_info);

    if (zxdh_hw_modify_qp(iwdev, iwqp, info, true))
        return -EINVAL;

    iwqp->iwarp_state = ZXDH_QPS_RESET;

	if (!iwqp->user_mode) {
		if (iwqp->iwscq) {
			zxdh_clean_cqes(iwqp, iwqp->iwscq);
			if (iwqp->iwrcq != iwqp->iwscq)
				zxdh_clean_cqes(iwqp, iwqp->iwrcq);
		}
		zxdh_init_qp_indices(&iwqp->sc_qp.qp_uk);
	}

    return 0;
}
/* Ended by AICoder, pid:2ea6ei8461ec90014d8608e96018d730a1e59409 */

int remote_ip_info_process(struct zxdh_device *iwdev, struct zxdh_rdma_to_eth_ip_para *ip_para)
{
	int ret = 0;
	struct iidc_core_dev_info *cdev_info;

	cdev_info = iwdev->rf->cdev;
	if (!cdev_info) {
		pr_err("[zxdh_rdma] %s[%d]: cdev_info is null!\n", __func__, __LINE__);
		return -EIO;
	}

	mutex_lock(&iwdev->eth_info_list_mtx_lock);

	ip_para->linked_fid = (cdev_info->slot_id & 0x0000ffff) << 16 | iwdev->rf->pcie_id;

	if (ip_para->mode == RDMA_ADD_REMOTE_IP) {
		ret = zxdh_eth_info_hlist_add(iwdev, ip_para);
	} else if (ip_para->mode == RDMA_DEL_REMOTE_IP) {
		ret = zxdh_eth_info_hlist_delete(iwdev, ip_para);
	} else {
		ret = -1;
		pr_info("[zxdh_rdma] %s[%d]: error mode=%d\n", __func__, __LINE__, ip_para->mode);
	}

	mutex_unlock(&iwdev->eth_info_list_mtx_lock);
	return ret;
}

int qp_remote_ip_info_process(struct ib_qp *ibqp, int op_type)
{
	struct zxdh_qp *iwqp = to_iwqp(ibqp);
	struct zxdh_device *iwdev = iwqp->iwdev;
	struct zxdh_udp_offload_info *udp_info = &iwqp->udp_info;
	struct zxdh_rdma_to_eth_ip_para ip_para = {0};
    u8 src_mac_addr[ETH_ALEN];
	int ret = 0;

	ip_para.ifname = iwdev->netdev->name;
	memcpy(ip_para.src_ip, udp_info->local_ipaddr, sizeof(udp_info->local_ipaddr));
	memcpy(ip_para.dst_ip, udp_info->dest_ip_addr, sizeof(udp_info->dest_ip_addr));

    ether_addr_copy(src_mac_addr, iwdev->netdev->dev_addr);
	ip_para.src_mac = LS_64_1(src_mac_addr[5], 0) | LS_64_1(src_mac_addr[4], 8) |
		LS_64_1(src_mac_addr[3], 16) | LS_64_1(src_mac_addr[2], 24) |
		LS_64_1(src_mac_addr[1], 32) | LS_64_1(src_mac_addr[0], 40);
	ip_para.dst_mac = LS_64_1(iwqp->udp_info.dest_mac[5], 0) | LS_64_1(iwqp->udp_info.dest_mac[4], 8) |
		LS_64_1(iwqp->udp_info.dest_mac[3], 16) | LS_64_1(iwqp->udp_info.dest_mac[2], 24) |
		LS_64_1(iwqp->udp_info.dest_mac[1], 32) | LS_64_1(iwqp->udp_info.dest_mac[0], 40);
	ip_para.ipv4 = udp_info->ipv4;
	ip_para.mode = op_type;

    pr_debug("[zxdh_rdma] %s[%d]: mode=%d qp_id=%d dev_addr=%pM src_mac_addr=%x-%x-%x-%x-%x-%x\n", __func__, __LINE__, op_type, ibqp->qp_num, iwdev->netdev->dev_addr, src_mac_addr[0], src_mac_addr[1], src_mac_addr[2], src_mac_addr[3], src_mac_addr[4], src_mac_addr[5]);
	pr_debug("[zxdh_rdma] %s[%d]: ipv4=%d, name=%s, src_ip=0x%x-0x%x-0x%x-0x%x, dst_ip=0x%x-0x%x-0x%x-0x%x, src_port=0x%x, dst_port=0x%x, src_mac=0x%llx, dst_mac=0x%llx\n",
		__func__, __LINE__, ip_para.ipv4, ip_para.ifname, ip_para.src_ip[0], ip_para.src_ip[1], ip_para.src_ip[2], ip_para.src_ip[3],
		ip_para.dst_ip[0], ip_para.dst_ip[1], ip_para.dst_ip[2], ip_para.dst_ip[3], udp_info->src_port, udp_info->dst_port, ip_para.src_mac, ip_para.dst_mac);

	if (op_type == RDMA_ADD_REMOTE_IP || op_type == RDMA_DEL_REMOTE_IP) {
		ret = remote_ip_info_process(iwdev, &ip_para);
	} else {
		pr_info("[zxdh_rdma] %s[%d]: error op_type=%d\n", __func__, __LINE__, op_type);
		ret = -1;
	}

	return ret;
}

/**
 * zxdh_modify_qp_roce - modify qp request
 * @ibqp: qp's pointer for modify
 * @attr: access attributes
 * @attr_mask: state mask
 * @udata: user data
 */
int zxdh_modify_qp_roce(struct ib_qp *ibqp, struct ib_qp_attr *attr,
			int attr_mask, struct ib_udata *udata)
{
	dpp_pf_info_t pf_info = { 0 };
	struct zxdh_pd *iwpd = to_iwpd(ibqp->pd);
	struct zxdh_qp *iwqp = to_iwqp(ibqp);
	struct zxdh_device *iwdev = iwqp->iwdev;
	struct zxdh_sc_dev *dev = &iwdev->rf->sc_dev;
    struct iidc_core_dev_info *cdev_info = (struct iidc_core_dev_info *)iwdev->rf->cdev;
	struct zxdh_qp_host_ctx_info *ctx_info;
	struct zxdh_roce_offload_info *roce_info;
	struct zxdh_udp_offload_info *udp_info;
	struct zxdh_modify_qp_info info = {};
	struct zxdh_modify_qp_resp uresp = {};
	struct zxdh_modify_qp_req ureq = {};
	char s_straddr[INET6_ADDRSTRLEN + 20] = { 0 };
	char d_straddr[INET6_ADDRSTRLEN + 20] = { 0 };
	int buf_size = 0;
	char *log_buf = NULL;
	enum ib_qp_state tmp_state;
	unsigned long flags = 0;
	u8 issue_modify_qp = 0;
	int ret = 0;
	u64 qpc_tx_mask_low = 0;
	u64 qpc_tx_mask_high = 0;
	u64 qpc_rx_mask_low = 0;
	u64 qpc_rx_mask_high = 0;
    u32 dual_tor_switch = 0xFFFF;
	u16 netdev_pmtu;
	u16 np_pmtu;
	u16 read_wrap_feature = 0;
	struct zxdh_path_info hnode_info = {};
	struct zxdh_pmtu_add_work *dwork;
	u16 pmtu_cfg_threshold = 0;
	u16 retry = 0;

	ctx_info = &iwqp->ctx_info;
	roce_info = &iwqp->roce_info;
	udp_info = &iwqp->udp_info;
	tmp_state = iwqp->ibqp_state;
	if (attr_mask & IB_QP_RATE_LIMIT) {
		if (attr->rate_limit & ZXDH_QP_MODIFY_NVMEOF_FLUSH) {
			iwqp->sc_qp.nvme_flush_qp = 1;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_NVMEOF_IOQ;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_NVMEOF_TGT;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_NVMEOF_QID;
		}

		if (attr->rate_limit & ZXDH_QP_MODIFY_NVMEOF_FLR) {
			writel(1, (u32 __iomem *)(dev->hw->hw_addr +
						  RDMATX_QUEUE_VHCA_FLAG(dev->ep_id)));
		}
		if (attr->rate_limit & ZXDH_QP_MODIFY_NVMEOF_IOQ) {
			iwqp->sc_qp.is_nvmeof_ioq =
				(attr->rate_limit & ZXDH_QP_NVMEOF_IOQ_MASK) ?
					1 :
					      0;
			qpc_tx_mask_low |= RDMAQPC_TX_MASKL_NVMEOF_IOQ;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_NVMEOF_IOQ;
		}
		if (attr->rate_limit & ZXDH_QP_MODIFY_NVMEOF_TGT) {
			iwqp->sc_qp.is_nvmeof_tgt =
				(attr->rate_limit & ZXDH_QP_NVMEOF_TGT_MASK) ?
					1 :
					      0;
			qpc_tx_mask_low |= RDMAQPC_TX_MASKL_NVMEOF_TGT;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_NVMEOF_TGT;
		}
		if (attr->rate_limit & ZXDH_QP_MODIFY_NVMEOF_QID) {
			iwqp->sc_qp.nvmeof_qid = attr->rate_limit &
						 ZXDH_QP_NVMEOF_QID_MASK;
			writel(dev->vhca_id,
			       (u32 __iomem *)(dev->hw->hw_addr +
					       NOF_IOQ_VHCA_ID(iwqp->sc_qp.nvmeof_qid, dev->ep_id)));
			writel(iwpd->sc_pd.pd_id,
			       (u32 __iomem *)(dev->hw->hw_addr +
					       NOF_IOQ_PD_ID(iwqp->sc_qp.nvmeof_qid, dev->ep_id)));
			iwqp->sc_qp.virtual_map = 0;
			iwqp->sc_qp.sq_pa = dev->nof_ioq_ddr_addr +
					    NOF_IOQ_SQ_WQE_SIZE *
						    NOF_IOQ_SQ_SIZE *
						    iwqp->sc_qp.nvmeof_qid;
			iwqp->sc_qp.hw_sq_size = NOF_IOQ_SQ_LOG_SIZE;
			qpc_tx_mask_low |= RDMAQPC_TX_MASKL_ACK_CREDITS;
			qpc_tx_mask_low |= RDMAQPC_TX_MASKL_NVMEOF_QID;
			qpc_tx_mask_low |= RDMAQPC_TX_MASKL_SQ_VMAP;
			qpc_tx_mask_low |= RDMAQPC_TX_MASKL_SQ_LPBL_SIZE;
			qpc_tx_mask_low |= RDMAQPC_TX_MASKL_SQ_PA;
			qpc_tx_mask_low |= RDMAQPC_TX_MASKL_LOG_SQ_SIZE;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_ACK_CREDITS;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_NVMEOF_QID;

			ret = zxdh_clear_nof_ioq(
				dev, NOF_IOQ_SQ_WQE_SIZE * NOF_IOQ_SQ_SIZE,
				iwqp->sc_qp.sq_pa);
			if (ret)
				return ret;
		}
	}

	if (refcount_read(&iwdev->trace_switch.t_switch)) {
		log_buf = vzalloc(ZXDH_LOG_BUF_SIZE);
		if (log_buf == NULL)
			ibdev_notice(&iwdev->ibdev, "alloc log buf failed\n");
	}

	if (attr_mask & IB_QP_DEST_QPN) {
		roce_info->dest_qp = attr->dest_qp_num;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_DEST_QPN;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_DEST_QPN;
		if (log_buf) {
			char qpn_buf[64] = { 0 };

			sprintf(qpn_buf, ", dest_qpn:%d", roce_info->dest_qp);
			strncat(log_buf, qpn_buf,
				ZXDH_LOG_BUF_SIZE - buf_size - 1);
			buf_size += strlen(qpn_buf);
		}
	}

	if (attr_mask & IB_QP_PKEY_INDEX) {
		ret = zxdh_query_pkey(ibqp->device, 0, attr->pkey_index,
				      &roce_info->p_key);
		if (ret) {
			if (log_buf)
				vfree(log_buf);
			return ret;
		}
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_PKEY;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_PKEY;
	}

	if (attr_mask & IB_QP_QKEY) {
		roce_info->qkey = attr->qkey;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_QKEY;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_QKEY;
	}

	if (attr_mask & IB_QP_PATH_MTU) {
		udp_info->pmtu = attr->path_mtu;
		iwqp->sc_qp.qp_uk.pmtu = attr->path_mtu;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_PMTU;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_PMTU;
		netdev_pmtu = zxdh_mtu_int_to_enum(iwdev->netdev->mtu);

		if (attr->path_mtu > netdev_pmtu) {
			pr_info("[zxdh_rdma] WARNING: attr->path_mtu(%d) larger than netdev_pmtu(%d)\n",
				attr->path_mtu, netdev_pmtu);
		}
	}

	if (attr_mask & IB_QP_SQ_PSN) {
		udp_info->psn_nxt = attr->sq_psn;
		udp_info->psn_una = attr->sq_psn;
		udp_info->psn_max = attr->sq_psn - 1;
		iwqp->sc_qp.aeq_entry_err_last_psn = attr->sq_psn - 1;
		iwqp->sc_qp.aeq_retry_err_last_psn = attr->sq_psn - 1;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_LAST_ACK_PSN;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_LSN;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_PSN_MAX;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_PSN_NXT;
	}

	if (attr_mask & IB_QP_RQ_PSN) {
		udp_info->epsn = attr->rq_psn;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_EPSN;
	}

	if (attr_mask & IB_QP_RNR_RETRY) {
		udp_info->rnr_nak_thresh = attr->rnr_retry;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_RNR_RETRY_THRESHOLD;
		qpc_tx_mask_high |= RDMAQPC_TX_MASKH_RNR_RETRY_CNT;
		qpc_tx_mask_high |= RDMAQPC_TX_MASKH_RNR_CUR_RETRY_CNT;
	}

	if (attr_mask & IB_QP_RETRY_CNT) {
		if (attr->retry_cnt == 7)
			attr->retry_cnt = 6;

		udp_info->rexmit_thresh = attr->retry_cnt;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_RETRY_CNT;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_CUR_RETRY_CNT;
	}

	if (attr_mask & IB_QP_TIMEOUT) {
		udp_info->timeout = attr->timeout;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_LOCAL_ACK_TIMEOUT;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_ACK_TIMEOUT;
	}

	if (attr_mask & IB_QP_MIN_RNR_TIMER) {
		udp_info->min_rnr_timer = attr->min_rnr_timer;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_RNR_TIMER;
	}

	ctx_info->roce_info->pd_id = iwpd->sc_pd.pd_id;
	qpc_tx_mask_low |= RDMAQPC_TX_MASKL_PD_ID;
	qpc_rx_mask_low |= RDMAQPC_RX_MASKL_PD_ID;

	if (attr_mask & IB_QP_AV) {
		struct zxdh_av *av = &iwqp->roce_ah.av;
		u16 vlan_id = VLAN_N_VID;
		u32 local_ip[4] = {};
		u8 global_tclass = 0xFF;
		u8 use_port_tc = 0;
		bool is_new_qp = false;

		/* 判断是否为新建QP：根据修改前的QP状态是否为RESET或INIT */
		is_new_qp = (tmp_state == IB_QPS_RESET ||
			     tmp_state == IB_QPS_INIT);

		/* 获取端口级traffic_class配置（仅对RC类型QP生效） */
		if (iwqp->ibqp.qp_type == IB_QPT_RC) {
			zxdh_get_port_traffic_class(iwdev, 1, &global_tclass);
		}

		memset(&iwqp->roce_ah, 0, sizeof(iwqp->roce_ah));
		if (attr->ah_attr.ah_flags & IB_AH_GRH) {
			udp_info->ttl = attr->ah_attr.grh.hop_limit;
			qpc_tx_mask_low |= RDMAQPC_TX_MASKL_TTL;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_TTL;
			udp_info->flow_label = attr->ah_attr.grh.flow_label;
			qpc_tx_mask_low |= RDMAQPC_TX_MASKL_FLOWLABLE;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_FLOWLABLE;
			udp_info->src_port = zxdh_get_udp_sport(
				iwdev, &attr->ah_attr, ibqp, udata);
			qpc_tx_mask_low |= RDMAQPC_TX_MASKL_SRC_PORT;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_SRC_PORT;
		}
		ret = kc_zxdh_set_roce_cm_info(iwqp, attr, &vlan_id);
		if (ret) {
			if (log_buf)
				vfree(log_buf);
			return ret;
		}

		if (iwdev->vf_vlan.vlan_id && vlan_id < VLAN_N_VID && iwdev->rf->ftype) {
				pr_err("[zxdh_rdma] [%s] [%s] has been set sriov_vlan, not support vlan_dev \n",
				__func__, iwdev->rf->iwdev->ibdev.name);
			if (log_buf)
				vfree(log_buf);
			return -EINVAL;
		}

		if (vlan_id >= VLAN_N_VID && iwdev->dcb_vlan_mode)
			vlan_id = 0;
		if (vlan_id < VLAN_N_VID) {
			udp_info->insert_vlan_tag = true;
			udp_info->vlan_tag =
				vlan_id | (attr->ah_attr.sl & 0x7) << VLAN_PRIO_SHIFT;
		} else {
			udp_info->insert_vlan_tag = false;
		}
		/*trust pcp时，采用set vf vlan的qos*/
		if (iwdev->vf_vlan.vlan_id && iwdev->rf->ftype) {
			udp_info->insert_vlan_tag = true;
			udp_info->vlan_tag =
				iwdev->vf_vlan.vlan_id | (iwdev->vf_vlan.qos & 0x7) << VLAN_PRIO_SHIFT;
		}

		if (attr->ah_attr.ah_flags & IB_AH_GRH) {
			/* TOS优先级规则（仅对RC类型QP生效）：
			 * 1. 如果是新建QP且端口级traffic_class已配置，则强制使用端口级配置（忽略用户TOS）
			 * 2. 否则使用用户指定的TOS（为0则表示默认值/未指定）
			 */
			if (iwqp->ibqp.qp_type == IB_QPT_RC &&
			    global_tclass != 0xFF && is_new_qp) {
				/* RC类型QP + 新建QP + 端口级已配置：强制使用端口级traffic_class */
				udp_info->tos = global_tclass;
				use_port_tc = 1;
			} else {
				/* 非RC类型QP或端口级未配置：使用用户指定的TOS（可能为0，表示默认） */
				udp_info->tos = attr->ah_attr.grh.traffic_class;
			}

			if (iwdev->trust_type == ZXDH_TRUST_PCP) {
				if (udp_info->insert_vlan_tag) {
					if (iwdev->vf_vlan.vlan_id && iwdev->rf->ftype) {
						udp_info->tos = (udp_info->tos & 0x1f) | (iwdev->vf_vlan.qos << 5);
					} else {
						udp_info->tos = (udp_info->tos & 0x1f) | (attr->ah_attr.sl << 5);
					}
				} else {
					udp_info->tos = (udp_info->tos & 0x1f);
				}	
			}

			qpc_tx_mask_high |= RDMAQPC_TX_MASKH_TOS;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_TOS;
			zxdh_qp_rem_qos(&iwqp->sc_qp);
			// ctx_info->user_pri = rt_tos2priority(udp_info->tos); //TODO: figure out why they do this
			ctx_info->user_pri = (udp_info->tos >> 2) / 8;
			iwqp->sc_qp.user_pri = ctx_info->user_pri;
			iwqp->sc_qp.qp_uk.user_pri = ctx_info->user_pri;
			// 保存改配之前的优先级
			hnode_info.pri_index = iwqp->sc_qp.pri_index;
			iwqp->sc_qp.modify_to_ets = true;
			zxdh_qp_add_qos(&iwqp->sc_qp);

			if (log_buf && use_port_tc) {
				char tc_buf[32] = { 0 };

				sprintf(tc_buf, ", global_tclass:%u", global_tclass);
				strncat(log_buf, tc_buf,
					ZXDH_LOG_BUF_SIZE - buf_size - 1);
				buf_size += strlen(tc_buf);
			}

			if (log_buf && udp_info->src_port) {
				char port_buf[32] = { 0 };

				sprintf(port_buf, ", src_port:%d",
					udp_info->src_port);
				strncat(log_buf, port_buf,
					ZXDH_LOG_BUF_SIZE - buf_size - 1);
				buf_size += strlen(port_buf);
			}
		}
		
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_INSERT_VLANTAG;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_VLANTAG;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_INSERT_VLANTAG;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_VLANTAG;

		av->attrs = attr->ah_attr;
		rdma_gid2ip((struct sockaddr *)&av->dgid_addr,
			    &attr->ah_attr.grh.dgid);

		// 保存改配之前的dip
		zxdh_dip_copy(hnode_info.dst_ip, udp_info->dest_ip_addr);

		if (av->sgid_addr.saddr.sa_family == AF_INET6) {
			__be32 *daddr = av->dgid_addr.saddr_in6.sin6_addr.in6_u
						.u6_addr32;
			__be32 *saddr = av->sgid_addr.saddr_in6.sin6_addr.in6_u
						.u6_addr32;

			zxdh_copy_ip_ntohl(&udp_info->dest_ip_addr[0], daddr);
			zxdh_copy_ip_ntohl(&udp_info->local_ipaddr[0], saddr);

			udp_info->ipv4 = false;
			zxdh_copy_ip_ntohl(local_ip, daddr);
			sprintf(s_straddr, ", src_ip: %pI6",
				&av->sgid_addr.saddr_in6.sin6_addr);
			sprintf(d_straddr, ", dest_ip: %pI6",
				&av->dgid_addr.saddr_in6.sin6_addr);
		} else {
			__be32 saddr = av->sgid_addr.saddr_in.sin_addr.s_addr;
			__be32 daddr = av->dgid_addr.saddr_in.sin_addr.s_addr;

			local_ip[0] = ntohl(daddr);

			udp_info->ipv4 = true;
			udp_info->dest_ip_addr[0] = 0;
			udp_info->dest_ip_addr[1] = 0;
			udp_info->dest_ip_addr[2] = 0;
			udp_info->dest_ip_addr[3] = local_ip[0];

			udp_info->local_ipaddr[0] = 0;
			udp_info->local_ipaddr[1] = 0;
			udp_info->local_ipaddr[2] = 0;
			udp_info->local_ipaddr[3] = ntohl(saddr);

			sprintf(s_straddr, ", src_ip: %pI4",
				&av->sgid_addr.saddr_in.sin_addr);
			sprintf(d_straddr, ", dest_ip: %pI4",
				&av->dgid_addr.saddr_in.sin_addr);
		}
		ether_addr_copy(udp_info->dest_mac,
				ah_attr_to_dmac(attr->ah_attr));

        dual_tor_switch = readl(cdev_info->hw_addr + ZXDH_DUAL_TOR_SWITCH_OFFSET);
        pr_debug("[zxdh_rdma] %s[%d]: hw_addr=0x%llx, dual_tor_switch=0x%x\n", __func__, __LINE__, (u64)cdev_info->hw_addr, dual_tor_switch);
        if ((dual_tor_switch == ZXDH_DUAL_TOR_SWITCH_OPEN)) {
            ret = qp_remote_ip_info_process(ibqp, RDMA_ADD_REMOTE_IP);
            if (ret) {
                pr_err("[zxdh_rdma] %s[%d]: ipv4=%d, name=%s, op_type=%d, src_ip=0x%x-0x%x-0x%x-0x%x, dst_ip=0x%x-0x%x-0x%x-0x%x, src_port=0x%x, dst_port=0x%x\n",
                    __func__, __LINE__, udp_info->ipv4, iwdev->netdev->name, RDMA_DEL_REMOTE_IP, udp_info->local_ipaddr[0], udp_info->local_ipaddr[1], udp_info->local_ipaddr[2], udp_info->local_ipaddr[3],
                    udp_info->dest_ip_addr[0], udp_info->dest_ip_addr[1], udp_info->dest_ip_addr[2], udp_info->dest_ip_addr[3], udp_info->src_port, udp_info->dst_port);
            }
        }

		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_IPV4;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_DEST_IP_LOW;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_DEST_IP_HIGH;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_LOCAL_IP_LOW;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_LOCAL_IP_HIGH;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_SRC_MAC;
		qpc_tx_mask_low |= RDMAQPC_TX_MASKL_DEST_MAC;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_IPV4;
		qpc_rx_mask_high |= RDMAQPC_RX_MASKH_DEST_IP;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_LOCAL_IP;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_SRC_MAC;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_DEST_MAC;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_HDR_LEN;

		if (log_buf) {
			strncat(log_buf, s_straddr,
				ZXDH_LOG_BUF_SIZE - buf_size - 1);
			buf_size += strlen(s_straddr);
			strncat(log_buf, d_straddr,
				ZXDH_LOG_BUF_SIZE - buf_size - 1);
			buf_size += strlen(d_straddr);
		}

		if (hnode_info.pri_index != iwqp->sc_qp.pri_index){
			if (log_buf) {
				ibdev_notice(&iwdev->ibdev, "[vhca%u][%s] qpid:%u pri_index changed! pri=%u --> pri=%u \n",
					iwqp->sc_qp.dev->vhca_id, __func__, iwqp->sc_qp.qp_uk.qp_id, hnode_info.pri_index, iwqp->sc_qp.pri_index);
			}
			hnode_info.flag = 1;  // 优先级有变更
		} else if (zxdh_dip_cmp(hnode_info.dst_ip, udp_info->dest_ip_addr)) {
			if (log_buf) {
				ibdev_notice(&iwdev->ibdev, "[vhca%u][%s] qpid:%u dip changed! ip=0x%x-0x%x-0x%x-0x%x --> ip=0x%x-0x%x-0x%x-0x%x \n",
					iwqp->sc_qp.dev->vhca_id, __func__, iwqp->sc_qp.qp_uk.qp_id, hnode_info.dst_ip[0], hnode_info.dst_ip[1],
					hnode_info.dst_ip[2], hnode_info.dst_ip[3], udp_info->dest_ip_addr[0], udp_info->dest_ip_addr[1], 
					udp_info->dest_ip_addr[2], udp_info->dest_ip_addr[3]);
			}
			hnode_info.flag = 1;  // dip有变更
		}
	}

	ret = zxdh_get_8k_index(iwdev->rf, &iwqp->sc_qp, udp_info->dest_ip_addr, 
							&iwqp->sc_qp.qp_uk.qp_8k_index, &hnode_info);
	if (ret) {
		pr_err("[zxdh_rdma] [%s][vhca%u][%s] qpid:%u zxdh_get_8k_index fail! \n", 
			iwdev->rf->iwdev->ibdev.name, iwqp->sc_qp.dev->vhca_id, __func__, iwqp->sc_qp.qp_uk.qp_id);
		goto exit;
	}

	qpc_tx_mask_low |= RDMAQPC_TX_MASKL_GQP_ID;
	qpc_tx_mask_high |= RDMAQPC_TX_MASKH_QUEUE_TC;
	qpc_tx_mask_high |= RDMAQPC_TX_MASKH_WS_IDX;
	qpc_rx_mask_high |= RDMAQPC_RX_MASKH_QUEUE_TC;
	qpc_rx_mask_low |= RDMAQPC_RX_MASKL_GQP_ID;
	qpc_rx_mask_low |= RDMAQPC_RX_MASKL_WS_IDX;

	if (attr_mask & IB_QP_MAX_QP_RD_ATOMIC) {
		if (attr->max_rd_atomic > dev->hw_attrs.max_hw_ord) {
			ibdev_err(&iwdev->ibdev,
				  "rd_atomic = %d, above max_hw_ord=%d\n",
				  attr->max_rd_atomic,
				  dev->hw_attrs.max_hw_ord);
			if (log_buf)
				vfree(log_buf);
			return -EINVAL;
		}
		if (attr->max_rd_atomic) {
			roce_info->ord_size = min(attr->max_rd_atomic, (__u8)(ICRDMA_MAX_ORD_SIZE - 1));
			qpc_tx_mask_low |= RDMAQPC_TX_MASKL_ORD_SIZE;
		}
		info.ord_valid = true;
	}

	if (attr_mask & IB_QP_MAX_DEST_RD_ATOMIC) {
		if (attr->max_dest_rd_atomic > dev->hw_attrs.max_hw_ird) {
			ibdev_err(&iwdev->ibdev,
				  "rd_atomic = %d, above max_hw_ird=%d\n",
				  attr->max_dest_rd_atomic,
				  dev->hw_attrs.max_hw_ird);
			if (log_buf)
				vfree(log_buf);
			return -EINVAL;
		}
		if (attr->max_dest_rd_atomic) {
			roce_info->ird_size = dev->hw_attrs.max_hw_ird;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_IRD_SIZE;
		}
	}

	if (attr_mask & IB_QP_ACCESS_FLAGS) {
		if (attr->qp_access_flags & IB_ACCESS_LOCAL_WRITE) {
			roce_info->wr_rdresp_en = true;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_WRITE_EN;
		}

		if (attr->qp_access_flags & IB_ACCESS_REMOTE_WRITE) {
			roce_info->wr_rdresp_en = true;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_WRITE_EN;
		}
		if (attr->qp_access_flags & IB_ACCESS_REMOTE_READ) {
			roce_info->rd_en = true;
			qpc_rx_mask_low |= RDMAQPC_RX_MASKL_READ_EN;
		}
	}

	wait_event(iwqp->mod_qp_waitq, !atomic_read(&iwqp->hw_mod_qp_pend));

	spin_lock_irqsave(&iwqp->lock, flags);
	if (attr_mask & IB_QP_STATE) {
		if (!kc_ib_modify_qp_is_ok(iwqp->ibqp_state, attr->qp_state,
					   iwqp->ibqp.qp_type, attr_mask,
					   IB_LINK_LAYER_ETHERNET)) {
			zxdh_dbg(dev,
				"modify_qp invalid for qp_id=%d, old_state=0x%x, new_state=0x%x\n",
				iwqp->ibqp.qp_num, iwqp->ibqp_state,
				attr->qp_state);
			ret = -EINVAL;
			goto exit;
		}
		info.curr_iwarp_state = iwqp->iwarp_state;

		qpc_tx_mask_high |= RDMAQPC_TX_MASKH_QP_STATE;
		qpc_rx_mask_low |= RDMAQPC_RX_MASKL_QP_STATE;

		switch (attr->qp_state) {
		case IB_QPS_INIT:
			if (iwqp->iwarp_state > ZXDH_QPS_INIT) {
				ret = -EINVAL;
				goto exit;
			}

			if (iwqp->iwarp_state == ZXDH_QPS_INIT) {
				ctx_info->next_qp_state = ZXDH_QPS_INIT;
				issue_modify_qp = 1;
			}

			if (iwqp->iwarp_state == ZXDH_QPS_RESET) {
				ctx_info->next_qp_state = ZXDH_QPS_INIT;
				issue_modify_qp = 1;
			}
			break;
		case IB_QPS_RTR:
			if (iwqp->iwarp_state > ZXDH_QPS_INIT) {
				ret = -EINVAL;
				goto exit;
			}
			ctx_info->next_qp_state = ZXDH_QPS_RTR;
			issue_modify_qp = 1;
			read_wrap_feature = zxdh_fw_feature_get(iwdev->rf, FW_FEATURE_READ_WRAP_SWITCH);
			if (iwdev->rf->ftype){
				pmtu_cfg_threshold = ZXDH_VF_PMTU_CONFIG_THRESHLD;
			} else {
				pmtu_cfg_threshold = ZXDH_PF_PMTU_CONFIG_THRESHLD;
			}

			if (dev->chip_version < 2 && read_wrap_feature == 1) {
				if (!zxdh_psn_wrap_try_inc(iwdev, pmtu_cfg_threshold + 1))
					break;
				pf_info.vport = cdev_info->vport_id;
				pf_info.slot = cdev_info->slot_id;
				np_pmtu = zxdh_mtu_enum_to_int(udp_info->pmtu);

				dwork = kzalloc(sizeof(*dwork), GFP_ATOMIC);
				if (dwork) {
					dwork->rf = iwdev->rf;
					dwork->pf_info = pf_info;
					dwork->pmtu = np_pmtu;
					dwork->qp_num = iwqp->ibqp.qp_num;

					INIT_WORK(&dwork->work, zxdh_pmtu_add_worker);
					queue_work(iwdev->cleanup_wq, &dwork->work);
				} else {
					zxdh_psn_wrap_dec(iwdev);
				}
			}
			break;
		case IB_QPS_RTS:
			if (iwqp->ibqp_state < IB_QPS_RTR ||
			    iwqp->ibqp_state == IB_QPS_ERR) {
				ret = -EINVAL;
				goto exit;
			}

			ctx_info->next_qp_state = ZXDH_QPS_RTS;
			issue_modify_qp = 1;
			
			read_wrap_feature = zxdh_fw_feature_get(iwdev->rf, FW_FEATURE_READ_WRAP_SWITCH);
			if (iwdev->rf->ftype){
				pmtu_cfg_threshold = ZXDH_VF_PMTU_CONFIG_THRESHLD;
			} else {
				pmtu_cfg_threshold = ZXDH_PF_PMTU_CONFIG_THRESHLD;
			}

			if (dev->chip_version < 2 && read_wrap_feature == 1) {
				if (iwqp->ibqp_state != IB_QPS_RTR && iwqp->ibqp_state != IB_QPS_RTS) {
					if (!zxdh_psn_wrap_try_inc(iwdev, pmtu_cfg_threshold + 1))
					    break;
					pf_info.vport = cdev_info->vport_id;
					pf_info.slot = cdev_info->slot_id;
					np_pmtu = zxdh_mtu_enum_to_int(udp_info->pmtu);

					dwork = kzalloc(sizeof(*dwork), GFP_ATOMIC);
					if (dwork) {
						dwork->rf = iwdev->rf;
						dwork->pf_info = pf_info;
						dwork->pmtu = np_pmtu;
						dwork->qp_num = iwqp->ibqp.qp_num;

						INIT_WORK(&dwork->work, zxdh_pmtu_add_worker);
						queue_work(iwdev->cleanup_wq, &dwork->work);
					}
				}
			}
			break;
		case IB_QPS_SQD:
			if (iwqp->iwarp_state == ZXDH_QPS_SQD)
				goto exit;

			if (iwqp->iwarp_state != ZXDH_QPS_RTS) {
				ret = -EINVAL;
				goto exit;
			}

			ctx_info->next_qp_state = ZXDH_QPS_SQD;
			issue_modify_qp = 1;
			break;
		case IB_QPS_SQE:
		case IB_QPS_ERR:
		case IB_QPS_RESET:
			if (iwqp->iwarp_state == ZXDH_QPS_ERR) {
				spin_unlock_irqrestore(&iwqp->lock, flags);
				if (udata) {
					if (ib_copy_from_udata(
						    &ureq, udata,
						    min(sizeof(ureq),
							udata->inlen)))
						return -EINVAL;

					zxdh_flush_wqes(
						iwqp,
						(ureq.sq_flush ? ZXDH_FLUSH_SQ :
								       0) |
							(ureq.rq_flush ?
								 ZXDH_FLUSH_RQ :
								       0) |
							ZXDH_REFLUSH);
				}
				iwqp->ibqp_state = attr->qp_state;
				if (attr->qp_state == IB_QPS_RESET) {
					if (zxdh_modify_qp_to_reset(iwqp, &info)) {
						if (log_buf)
							vfree(log_buf);
						return -EINVAL;
					}
				}
				if (log_buf)
					vfree(log_buf);
				return 0;
			}

			ctx_info->next_qp_state = ZXDH_QPS_ERR;
			issue_modify_qp = 1;
			break;
		default:
			ret = -EINVAL;
			goto exit;
		}

		iwqp->ibqp_state = attr->qp_state;
	}

	zxdh_sc_qp_setctx_roce(&iwqp->sc_qp, iwqp->host_ctx.va, ctx_info);
	spin_unlock_irqrestore(&iwqp->lock, flags);
	if (ctx_info->next_qp_state == ZXDH_QPS_ERR) {
		info.qpc_tx_mask_low = qpc_tx_mask_low;
		info.qpc_tx_mask_high = qpc_tx_mask_high;
		info.qpc_rx_mask_low = qpc_rx_mask_low;
		info.qpc_rx_mask_high = qpc_rx_mask_high;
	} else {
		info.qpc_tx_mask_low = 0x1FFFFFF | qpc_tx_mask_low;
		info.qpc_tx_mask_high = (0x1UL << 18) | qpc_tx_mask_high;
		info.qpc_rx_mask_low = 0xDA3CE8081E7FFCF0 | qpc_rx_mask_low;
		info.qpc_rx_mask_high = 0x1E9 | qpc_rx_mask_high;
	}

	if (attr_mask & IB_QP_RATE_LIMIT) {
		info.qpc_tx_mask_low = 0x1FFFFFF | qpc_tx_mask_low;
		info.qpc_tx_mask_high = (0x1UL << 18) | qpc_tx_mask_high;
		info.qpc_rx_mask_low = 0xDA3CE8081E7FFCF0 | qpc_rx_mask_low;
		info.qpc_rx_mask_high = 0x1E9 | qpc_rx_mask_high;
		if (zxdh_hw_modify_qp(iwdev, iwqp, &info, true)) {
			if (log_buf)
				vfree(log_buf);
			return -EINVAL;
		}
	}

	if (attr_mask & IB_QP_STATE) {
		if (issue_modify_qp) {
			if (zxdh_hw_modify_qp(iwdev, iwqp, &info, true)) {
				if (log_buf)
					vfree(log_buf);
				return -EINVAL;
			}
			spin_lock_irqsave(&iwqp->lock, flags);
			if (iwqp->iwarp_state == info.curr_iwarp_state) {
				iwqp->iwarp_state = ctx_info->next_qp_state;
				iwqp->ibqp_state = attr->qp_state;
			}
			if (iwqp->ibqp_state > IB_QPS_RTS &&
			    !iwqp->flush_issued) {
				iwqp->flush_issued = 1;
				if (!iwqp->user_mode)
					queue_delayed_work(
						iwqp->iwdev->cleanup_wq,
						&iwqp->dwork_flush,
						msecs_to_jiffies(
							ZXDH_FLUSH_DELAY_MS));
				spin_unlock_irqrestore(&iwqp->lock, flags);
				zxdh_flush_wqes(iwqp, ZXDH_FLUSH_SQ |
							      ZXDH_FLUSH_RQ |
							      ZXDH_FLUSH_WAIT);
			} else {
				spin_unlock_irqrestore(&iwqp->lock, flags);
			}

			if (attr->qp_state == IB_QPS_RESET) {
				if (attr->qp_state == IB_QPS_RESET) {
					if (zxdh_modify_qp_to_reset(iwqp, &info)) {
						if (log_buf)
							vfree(log_buf);
						return -EINVAL;
					}
				}
			}
		} else {
			iwqp->ibqp_state = attr->qp_state;
		}
		if (udata) {
			uresp.rd_fence_rate = iwdev->rd_fence_rate;
			ret = ib_copy_to_udata(udata, &uresp,
					       min(sizeof(uresp),
						   udata->outlen));
			if (ret) {
				pr_err("[zxdh_rdma] VERBS: copy_to_udata failed\n");
				if (log_buf)
					vfree(log_buf);
				return ret;
			}
		}
		if (log_buf) {
			ibdev_notice(
				&iwdev->ibdev,
				"QP[%u]: modify QP, type %d, ib qpn 0x%X, state: %s => %s%s\n",
				iwqp->ibqp.qp_num, iwqp->ibqp.qp_type,
				iwqp->ibqp.qp_num,
				zxdh_qp_state_to_string(tmp_state),
				zxdh_qp_state_to_string(attr->qp_state),
				log_buf);
		}
	}

	if (log_buf)
		vfree(log_buf);

	if (iwqp->ibqp.qp_num == 1) {
		retry = 0;
		do {
			ret = check_rdma_tx_package_err(iwqp);
			retry++;
		} while (ret == -EAGAIN && retry < ZXDH_CHECK_TX_PACKAGE_ERR_MAX_CNT);
	}

	return 0;
exit:
	if (log_buf)
		vfree(log_buf);

	spin_unlock_irqrestore(&iwqp->lock, flags);

	return ret;
}

/**
 * zxdh_cq_free_rsrc - free up resources for cq
 * @rf: RDMA PCI function
 * @iwcq: cq ptr
 */
void zxdh_cq_free_rsrc(struct zxdh_pci_f *rf, struct zxdh_cq *iwcq)
{
	struct zxdh_sc_cq *cq = &iwcq->sc_cq;

	if (!iwcq->user_mode) {
		if(!rf->use_ext_mem_flag)
		{
			dma_free_coherent(rf->sc_dev.hw->device, iwcq->kmem.size,
				iwcq->kmem.va, iwcq->kmem.pa);
			dma_free_coherent(rf->sc_dev.hw->device, iwcq->kmem_shadow.size,
					iwcq->kmem_shadow.va, iwcq->kmem_shadow.pa);
		
			iwcq->kmem.va = NULL;
			iwcq->kmem_shadow.va = NULL;
		}
	}
	if (cq->dev) {
		zxdh_free_rsrc(rf, rf->allocated_cqs,
			       iwcq->cq_num - cq->dev->base_cqn);
	}
}

/**
 * zxdh_free_cqbuf - worker to free a cq buffer
 * @work: provides access to the cq buffer to free
 */
static void zxdh_free_cqbuf(struct work_struct *work)
{
	struct zxdh_cq_buf *cq_buf =
		container_of(work, struct zxdh_cq_buf, work);

	dma_free_coherent(cq_buf->hw->device, cq_buf->kmem_buf.size,
			  cq_buf->kmem_buf.va, cq_buf->kmem_buf.pa);
	cq_buf->kmem_buf.va = NULL;
	kfree(cq_buf);
}

/**
 * zxdh_process_resize_list - remove resized cq buffers from the resize_list
 * @iwcq: cq which owns the resize_list
 * @iwdev: zrdma device
 * @lcqe_buf: the buffer where the last cqe is received
 */
int zxdh_process_resize_list(struct zxdh_cq *iwcq, struct zxdh_device *iwdev,
			     struct zxdh_cq_buf *lcqe_buf)
{
	struct list_head *tmp_node, *list_node;
	struct zxdh_cq_buf *cq_buf;
	int cnt = 0;

	list_for_each_safe(list_node, tmp_node, &iwcq->resize_list) {
		cq_buf = list_entry(list_node, struct zxdh_cq_buf, list);
		if (cq_buf == lcqe_buf)
			return cnt;

		list_del(&cq_buf->list);
		queue_work(iwdev->cleanup_wq, &cq_buf->work);
		cnt++;
	}

	return cnt;
}

/**
 * zxdh_resize_cq - resize cq
 * @ibcq: cq to be resized
 * @entries: desired cq size
 * @udata: user data
 */
static int zxdh_resize_cq(struct ib_cq *ibcq, int entries,
			  struct ib_udata *udata)
{
	struct zxdh_cq *iwcq = to_iwcq(ibcq);
	struct zxdh_sc_dev *dev = iwcq->sc_cq.dev;
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_modify_cq_info *m_info;
	struct zxdh_modify_cq_info info = {};
	struct zxdh_dma_mem kmem_buf;
	struct zxdh_cq_mr *cqmr_buf;
	struct zxdh_pbl *iwpbl_buf;
	struct zxdh_device *iwdev;
	struct zxdh_pci_f *rf;
	struct zxdh_cq_buf *cq_buf = NULL;
	unsigned long flags;
	int ret;

	iwdev = to_iwdev(ibcq->device);
	rf = iwdev->rf;

	if (!(rf->sc_dev.hw_attrs.uk_attrs.feature_flags &
	      ZXDH_FEATURE_CQ_RESIZE))
		return -EOPNOTSUPP;

	if (entries > rf->max_cqe)
		return -EINVAL;

	if (!iwcq->user_mode) {
		entries++;
		if (rf->sc_dev.hw_attrs.uk_attrs.hw_rev >= ZXDH_GEN_2)
			entries *= 2;
	}

	info.cq_size = zxdh_cq_round_up(max(entries, 4));

	if (info.cq_size == iwcq->sc_cq.cq_uk.cq_size - 1)
		return 0;

	if (udata) {
		struct zxdh_resize_cq_req req = {};
		struct zxdh_ucontext *ucontext =
			kc_rdma_udata_to_drv_context(ibcq, udata);

		/* CQ resize not supported with legacy GEN_1 lib */
		if (ucontext->legacy_mode)
			return -EOPNOTSUPP;

		if (ib_copy_from_udata(&req, udata,
				       min(sizeof(req), udata->inlen)))
			return -EINVAL;

		spin_lock_irqsave(&ucontext->cq_reg_mem_list_lock, flags);
		iwpbl_buf = zxdh_get_pbl((unsigned long)req.user_cq_buffer,
					 &ucontext->cq_reg_mem_list);
		spin_unlock_irqrestore(&ucontext->cq_reg_mem_list_lock, flags);

		if (!iwpbl_buf)
			return -ENOMEM;

		cqmr_buf = &iwpbl_buf->cq_mr;
		if (iwpbl_buf->pbl_allocated) {
			info.virtual_map = true;
			info.pbl_chunk_size = 1;
			info.first_pm_pbl_idx = cqmr_buf->cq_pbl.idx;
		} else {
			info.cq_pa = cqmr_buf->cq_pbl.addr;
		}
	} else {
		/* Kmode CQ resize */
		int rsize;

		rsize = info.cq_size * sizeof(struct zxdh_cqe);
		kmem_buf.size = ALIGN(round_up(rsize, 256), 256);
		kmem_buf.va = dma_alloc_coherent(dev->hw->device, kmem_buf.size,
						 &kmem_buf.pa, GFP_KERNEL);
		if (!kmem_buf.va)
			return -ENOMEM;

		info.cq_base = kmem_buf.va;
		info.cq_pa = kmem_buf.pa;
		cq_buf = kzalloc(sizeof(*cq_buf), GFP_KERNEL);
		if (!cq_buf) {
			ret = -ENOMEM;
			goto error;
		}
	}

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request) {
		ret = -ENOMEM;
		goto error;
	}

	info.shadow_read_threshold = iwcq->sc_cq.shadow_read_threshold;
	info.cq_resize = true;

	cqp_info = &cqp_request->info;
	m_info = &cqp_info->in.u.cq_modify.info;
	memcpy(m_info, &info, sizeof(*m_info));

	cqp_info->cqp_cmd = ZXDH_OP_CQ_MODIFY;
	cqp_info->in.u.cq_modify.cq = &iwcq->sc_cq;
	cqp_info->in.u.cq_modify.scratch = (uintptr_t)cqp_request;
	cqp_info->post_sq = 1;
	ret = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	if (ret)
		goto error;

	spin_lock_irqsave(&iwcq->lock, flags);
	if (cq_buf) {
		cq_buf->kmem_buf = iwcq->kmem;
		cq_buf->hw = dev->hw;
		memcpy(&cq_buf->cq_uk, &iwcq->sc_cq.cq_uk,
		       sizeof(cq_buf->cq_uk));
		INIT_WORK(&cq_buf->work, zxdh_free_cqbuf);
		list_add_tail(&cq_buf->list, &iwcq->resize_list);
		iwcq->kmem = kmem_buf;
	}

	zxdh_sc_cq_resize(&iwcq->sc_cq, &info);
	ibcq->cqe = info.cq_size - 1;
	spin_unlock_irqrestore(&iwcq->lock, flags);

	return 0;
error:
	if (!udata && kmem_buf.va) {
		dma_free_coherent(dev->hw->device, kmem_buf.size, kmem_buf.va,
				  kmem_buf.pa);
		kmem_buf.va = NULL;
	}
	kfree(cq_buf);
	cq_buf = NULL;

	return ret;
}

static int zxdh_modify_cq(struct ib_cq *ibcq, u16 cq_count, u16 cq_period)
{
	struct zxdh_device *iwdev = to_iwdev(ibcq->device);
	struct zxdh_cq *iwcq = to_iwcq(ibcq);
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf;
	int ret;
	u32 val = 0;
	u16 unit_period = 0;

	rf = iwdev->rf;
	val = readl(rf->sc_dev.hw->hw_addr + RDMARX_CQ_PERIOD_CFG(rf->sc_dev.ep_id));
	unit_period = (u16)(val & 0xffff);

	if ((US_TO_NS(cq_period) / unit_period) > ZXDH_MAX_CQ_PERIOD) {
		pr_info("[zxdh_rdma] cq_count and cq_period validate fail\n");
		return -EINVAL;
	}

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request) {
		pr_err("[zxdh_rdma] alloc and get cqp request failed!\n");
		return -ENOMEM;
	}

	cqp_info = &cqp_request->info;

	cqp_info->cqp_cmd = ZXDH_OP_CQ_MODIFY_MODERATION;
	cqp_info->in.u.cq_modify.cq = &iwcq->sc_cq;
	cqp_info->in.u.cq_modify.scratch = (uintptr_t)cqp_request;
	cqp_info->post_sq = 1;

	cqp_info->in.u.cq_modify.cq->cq_max = cq_count;
	cqp_info->in.u.cq_modify.cq->cq_period =
		(uint16_t)(US_TO_NS(cq_period) / unit_period);
	cqp_info->in.u.cq_modify.cq->scqe_break_moderation_en =
		iwcq->sc_cq.scqe_break_moderation_en;
	ret = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	if (ret)
		zxdh_dbg(iwdev_to_idev(iwdev), "MODIFY CQ: modify_cq failed\n");

	return ret;
}

/**
 * zxdh_get_mr_access - get hw MR access permissions from IB access flags
 * @access: IB access flags
 */
static inline u16 zxdh_get_mr_access(int access)
{
	u16 hw_access = 0;

	hw_access |= (access & IB_ACCESS_LOCAL_WRITE) ?
			     ZXDH_ACCESS_FLAGS_LOCALWRITE :
				   0;
	hw_access |= (access & IB_ACCESS_REMOTE_WRITE) ?
			     ZXDH_ACCESS_FLAGS_REMOTEWRITE :
				   0;
	hw_access |= (access & IB_ACCESS_REMOTE_READ) ?
			     ZXDH_ACCESS_FLAGS_REMOTEREAD :
				   0;
	hw_access |= (access & IB_ACCESS_MW_BIND) ?
			     ZXDH_ACCESS_FLAGS_BIND_WINDOW :
				   0;
	hw_access |= (access & IB_ZERO_BASED) ? ZXDH_ACCESS_FLAGS_ZERO_BASED :
						      0;
	hw_access |= ZXDH_ACCESS_FLAGS_LOCALREAD;

	return hw_access;
}

/**
 * zxdh_free_stag - free stag resource
 * @iwdev: zrdma device
 * @stag: stag to free
 */
void zxdh_free_stag(struct zxdh_device *iwdev, u32 stag)
{
	u32 stag_idx;

	stag_idx = (stag) >> ZXDH_CQPSQ_STAG_IDX_S;
	zxdh_free_rsrc(iwdev->rf, iwdev->rf->allocated_mrs, stag_idx);
}

/**
 * zxdh_create_stag - create random stag
 * @iwdev: zrdma device
 */
u32 zxdh_create_stag(struct zxdh_device *iwdev)
{
	u32 stag = 0;
	u32 stag_index = 0;
	u32 random;
	u8 consumer_key;
	int ret;

	get_random_bytes(&random, sizeof(random));
	consumer_key = (u8)random;

	ret = zxdh_alloc_rsrc(iwdev->rf, iwdev->rf->allocated_mrs,
			      iwdev->rf->max_mr, &stag_index,
			      &iwdev->rf->next_mr);

	if (ret) {
		pr_err("[zxdh_rdma] %s[%d]: alloc_rsrc failed, invalid stag=%d\n", __func__, __LINE__, stag);
		return stag;
	}
	stag = stag_index << ZXDH_CQPSQ_STAG_IDX_S;
	stag |= consumer_key;

	return stag;
}

/**
 * zxdh_check_mem_contiguous - check if pbls stored in arr are contiguous
 * @arr: lvl1 pbl array
 * @npages: page count
 * @pg_size: page size
 *
 */
static bool zxdh_check_mem_contiguous(u64 *arr, u32 npages, u32 pg_size)
{
	u32 pg_idx;

	for (pg_idx = 0; pg_idx < npages; pg_idx++) {
		if ((*arr + (pg_size * pg_idx)) != arr[pg_idx])
			return false;
	}

	return true;
}

/**
 * zxdh_check_mr_contiguous - check if MR is physically contiguous
 * @palloc: pbl allocation struct
 * @pg_size: page size
 */
static bool zxdh_check_mr_contiguous(struct zxdh_pble_alloc *palloc,
				     u32 pg_size)
{
	struct zxdh_pble_level2 *lvl2 = &palloc->level2;
	struct zxdh_pble_info *leaf = lvl2->leaf;
	u64 *arr = NULL;
	u64 *start_addr = NULL;
	int i;
	bool ret;

	if (palloc->level == PBLE_LEVEL_1) {
		arr = palloc->level1.addr;
		ret = zxdh_check_mem_contiguous(arr, palloc->total_cnt,
						pg_size);
		return ret;
	}

	start_addr = leaf->addr;

	for (i = 0; i < lvl2->leaf_cnt; i++, leaf++) {
		arr = leaf->addr;
		if ((*start_addr + (i * pg_size * PBLE_PER_PAGE)) != *arr)
			return false;
		ret = zxdh_check_mem_contiguous(arr, leaf->cnt, pg_size);
		if (!ret)
			return false;
	}

	return true;
}

/**
 * zxdh_setup_pbles - copy user pg address to pble's
 * @rf: RDMA PCI function
 * @iwmr: mr pointer for this memory registration
 * @use_pbles: flag if to use pble's
 * @pble_type: flag if to pble type(mr or queue)
 */
static int zxdh_setup_pbles(struct zxdh_pci_f *rf, struct zxdh_mr *iwmr,
			    bool use_pbles, bool pble_type)
{
	struct zxdh_pbl *iwpbl = &iwmr->iwpbl;
	struct zxdh_pble_alloc *palloc = &iwpbl->pble_alloc;
	struct zxdh_pble_info *pinfo = NULL;
	struct zxdh_hmc_pble_rsrc *pble_rsrc_com;
	u64 *pbl;
	int status;
	enum zxdh_pble_level level = PBLE_LEVEL_1;
	bool b_level1_only = true;
	u32 page_cnt = 0;

	if (use_pbles) {
		if (pble_type == PBLE_QUEUE) {
			pble_rsrc_com = rf->pble_rsrc;
			b_level1_only = true;
			if (iwmr->page_size == SZ_64K)
			    page_cnt = (iwmr->page_cnt * ZXDH_4K_TO_64K_MUL);
			else 
			    page_cnt = iwmr->page_cnt;
		} else {
			pble_rsrc_com = rf->pble_mr_rsrc;
			b_level1_only = false;
			page_cnt = iwmr->page_cnt;
		}

		status = zxdh_get_pble(pble_rsrc_com, palloc, page_cnt,
				       b_level1_only);
		if (status) {
			pr_info("[zxdh_rdma] %s %d get pble failed status:%d\n", __func__, __LINE__, status);
			return status;
		}

		iwpbl->pbl_allocated = true;
		level = palloc->level;
		pinfo = (level == PBLE_LEVEL_1) ? &palloc->level1 :
							palloc->level2.leaf;
		pbl = pinfo->addr;
		pinfo->pble_copy = pble_rsrc_com->pble_copy;
	} else {
		pbl = iwmr->pgaddrmem;
	}

	zxdh_copy_user_pgaddrs(iwmr, pbl, &pinfo, level, use_pbles, pble_type);

	if (use_pbles)
		iwmr->pgaddrmem[0] = *pbl;

	return 0;
}

/**
 * zxdh_handle_q_mem - handle memory for qp and cq
 * @iwdev: zrdma device
 * @req: information for q memory management
 * @iwpbl: pble struct
 * @use_pbles: flag to use pble
 */
static int zxdh_handle_q_mem(struct zxdh_device *iwdev,
			     struct zxdh_mem_reg_req *req,
			     struct zxdh_pbl *iwpbl, bool use_pbles)
{
	struct zxdh_pble_alloc *palloc = &iwpbl->pble_alloc;
	struct zxdh_mr *iwmr = iwpbl->iwmr;
	struct zxdh_qp_mr *qpmr = &iwpbl->qp_mr;
	struct zxdh_cq_mr *cqmr = &iwpbl->cq_mr;
	struct zxdh_srq_mr *srqmr = &iwpbl->srq_mr;
	struct zxdh_hmc_pble *hmc_p;
	u64 *arr = iwmr->pgaddrmem;
	u32 pg_size, total;
	int err = 0;
	bool ret = true;

	pg_size = iwmr->page_size;
	err = zxdh_setup_pbles(iwdev->rf, iwmr, use_pbles,
			       PBLE_QUEUE); // queue mr
	if (err) {
		pr_err("[zxdh_rdma] %s[%d]: setup_pbles failed\n", __func__, __LINE__);
		return err;
	}

	if (use_pbles && palloc->level != PBLE_LEVEL_1) {
		zxdh_free_pble(iwdev->rf->pble_rsrc, palloc);
		iwpbl->pbl_allocated = false;
		return -ENOMEM;
	}

	if (use_pbles)
		arr = palloc->level1.addr;

	switch (iwmr->type) {
	case ZXDH_MEMREG_TYPE_QP:
		total = req->sq_pages + req->rq_pages;
		hmc_p = &qpmr->sq_pbl;
		qpmr->shadow = (dma_addr_t)arr[total];
		if (use_pbles) {
			ret = zxdh_check_mem_contiguous(arr, req->sq_pages,
							pg_size);
			if (ret)
				ret = zxdh_check_mem_contiguous(
					&arr[req->sq_pages], req->rq_pages,
					pg_size);
		}

		if (!ret) {
			hmc_p->idx = palloc->level1.idx;
			hmc_p = &qpmr->rq_pbl;
			hmc_p->idx = palloc->level1.idx + req->sq_pages;
			if (iwdev->rf->use_ext_mem_flag)
			{
				pr_info("[zxdh_rdma] QP buff not contiguous\n");
				err =  -ENOMEM;
			}
		} else {
			hmc_p->addr = arr[0];
			hmc_p = &qpmr->rq_pbl;
			hmc_p->addr = arr[req->sq_pages];
		}
		break;
	case ZXDH_MEMREG_TYPE_CQ:
		hmc_p = &cqmr->cq_pbl;

		if (!cqmr->split)
			cqmr->shadow = (dma_addr_t)arr[req->cq_pages];

		if (use_pbles)
			ret = zxdh_check_mem_contiguous(arr, req->cq_pages,
							pg_size);

		if (!ret) {
			hmc_p->idx = palloc->level1.idx;
			if (iwdev->rf->use_ext_mem_flag)
			{
				pr_info("[zxdh_rdma] CQ buff not contiguous\n");
				err =  -ENOMEM;
			}
		}
		else
			hmc_p->addr = arr[0];
		break;
	case ZXDH_MEMREG_TYPE_SRQ:
		total = req->srq_pages + req->srq_list_pages;
		hmc_p = &srqmr->srq_pbl;
		srqmr->db_addr = (dma_addr_t)arr[total];

		if (use_pbles) {
			ret = zxdh_check_mem_contiguous(arr, req->srq_pages,
							pg_size);
			if (ret)
				ret = zxdh_check_mem_contiguous(
					&arr[req->srq_pages],
					req->srq_list_pages, pg_size);
		}

		if (!ret) {
			hmc_p->idx = palloc->level1.idx;
			hmc_p = &srqmr->srq_list_pbl;
			hmc_p->idx = palloc->level1.idx + req->srq_pages;
			if (iwdev->rf->use_ext_mem_flag)
			{
				pr_info("[zxdh_rdma] SRQ buff not contiguous\n");
				err =  -ENOMEM;
			}
		} else {
			hmc_p->addr = arr[0];
			hmc_p = &srqmr->srq_list_pbl;
			hmc_p->addr = arr[req->srq_pages];
		}
		break;
	default:
		pr_err("[zxdh_rdma] VERBS: MR type error\n");
		err = -EINVAL;
	}

	if (use_pbles && ret) {
		zxdh_free_pble(iwdev->rf->pble_rsrc, palloc);
		iwpbl->pbl_allocated = false;
	}

	return err;
}

/**
 * zxdh_hw_alloc_mw - create the hw memory window
 * @iwdev: zrdma device
 * @iwmr: pointer to memory window info
 */
int zxdh_hw_alloc_mw(struct zxdh_device *iwdev, struct zxdh_mr *iwmr)
{
	struct zxdh_mw_alloc_info *info;
	struct zxdh_pd *iwpd = to_iwpd(iwmr->ibmr.pd);
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&iwdev->rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	info = &cqp_info->in.u.mw_alloc.info;
	memset(info, 0, sizeof(*info));
	if (iwmr->ibmw.type == IB_MW_TYPE_1)
		info->mw_wide = true;

	info->page_size = PAGE_SIZE;
	info->mw_stag_index = iwmr->stag >> ZXDH_CQPSQ_STAG_IDX_S;
	info->pd_id = iwpd->sc_pd.pd_id;
	info->remote_access = true;
	cqp_info->cqp_cmd = ZXDH_OP_MW_ALLOC;
	cqp_info->post_sq = 1;
	cqp_info->in.u.mw_alloc.dev = &iwdev->rf->sc_dev;
	cqp_info->in.u.mw_alloc.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(iwdev->rf, cqp_request);
	zxdh_put_cqp_request(&iwdev->rf->cqp, cqp_request);

	return status;
}

/**
 * zxdh_dealloc_mw - Dealloc memory window
 * @ibmw: memory window structure.
 */
static int zxdh_dealloc_mw(struct ib_mw *ibmw)
{
	struct ib_pd *ibpd = ibmw->pd;
	struct zxdh_pd *iwpd = to_iwpd(ibpd);
	struct zxdh_mr *iwmr = to_iwmr((struct ib_mr *)ibmw);
	struct zxdh_device *iwdev = to_iwdev(ibmw->device);
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_dealloc_stag_info *info;

	cqp_request = zxdh_alloc_and_get_cqp_request(&iwdev->rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	info = &cqp_info->in.u.dealloc_stag.info;
	memset(info, 0, sizeof(*info));
	info->pd_id = iwpd->sc_pd.pd_id;
	info->stag_idx = RS_64_1(ibmw->rkey, ZXDH_CQPSQ_STAG_IDX_S);
	info->mr = false;
	cqp_info->cqp_cmd = ZXDH_OP_DEALLOC_STAG;
	cqp_info->post_sq = 1;
	cqp_info->in.u.dealloc_stag.dev = &iwdev->rf->sc_dev;
	cqp_info->in.u.dealloc_stag.scratch = (uintptr_t)cqp_request;
	zxdh_handle_cqp_op(iwdev->rf, cqp_request);
	zxdh_put_cqp_request(&iwdev->rf->cqp, cqp_request);
	zxdh_free_stag(iwdev, iwmr->stag);
#ifdef ZXDH_ALLOC_MW_VER_1
	kfree(iwmr);
#endif /* ZXDH_ALLOC_MW_VER_1 */

	return 0;
}

/**
 * zxdh_hw_alloc_stag - cqp command to allocate stag
 * @iwdev: zrdma device
 * @iwmr: zrdma mr pointer
 */
int zxdh_hw_alloc_stag(struct zxdh_device *iwdev, struct zxdh_mr *iwmr)
{
	struct zxdh_allocate_stag_info *info;
	struct zxdh_pd *iwpd = to_iwpd(iwmr->ibmr.pd);
	int status;
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;

	cqp_request = zxdh_alloc_and_get_cqp_request(&iwdev->rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	info = &cqp_info->in.u.alloc_stag.info;
	memset(info, 0, sizeof(*info));
	info->page_size = PAGE_SIZE;
	info->stag_key = iwmr->stag & 0xFF;
	info->stag_idx = iwmr->stag >> ZXDH_CQPSQ_STAG_IDX_S;
	info->pd_id = iwpd->sc_pd.pd_id;
	info->total_len = iwmr->len;
	info->remote_access = true;
	cqp_info->cqp_cmd = ZXDH_OP_ALLOC_STAG;
	cqp_info->post_sq = 1;
	cqp_info->in.u.alloc_stag.dev = &iwdev->rf->sc_dev;
	cqp_info->in.u.alloc_stag.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(iwdev->rf, cqp_request);
	zxdh_put_cqp_request(&iwdev->rf->cqp, cqp_request);
	if (!status)
		iwmr->is_hwreg = 1;

	return status;
}

/**
 * zxdh_set_page - populate pbl list for fmr
 * @ibmr: ib mem to access iwarp mr pointer
 * @addr: page dma address fro pbl list
 */
static int zxdh_set_page(struct ib_mr *ibmr, u64 addr)
{
	struct zxdh_mr *iwmr = to_iwmr(ibmr);
	struct zxdh_pbl *iwpbl = &iwmr->iwpbl;
	struct zxdh_pble_alloc *palloc = &iwpbl->pble_alloc;
	u64 *pbl;

	if (unlikely(iwmr->npages == iwmr->page_cnt))
		return -ENOMEM;

	pbl = palloc->level1.addr;
	pbl[iwmr->npages++] = addr;

	return 0;
}

/**
 * zxdh_map_mr_sg - map of sg list for fmr
 * @ibmr: ib mem to access iwarp mr pointer
 * @sg: scatter gather list
 * @sg_nents: number of sg pages
 * @sg_offset: scatter gather list for fmr
 */
static int zxdh_map_mr_sg(struct ib_mr *ibmr, struct scatterlist *sg,
			  int sg_nents, unsigned int *sg_offset)
{
	struct zxdh_mr *iwmr = to_iwmr(ibmr);
	struct zxdh_pble_alloc *palloc = &iwmr->iwpbl.pble_alloc;
	int ret = 0;

	iwmr->npages = 0;

	ret = ib_sg_to_pages(ibmr, sg, sg_nents, sg_offset, zxdh_set_page);

	if (iwmr->npages > 1) {
		zxdh_cqp_config_pble_table_cmd(iwmr->sc_dev, &(palloc->level1),
					       iwmr->npages << 3, PBLE_MR);
	}

	return ret;
}

/**
 * zxdh_hwreg_mr - send cqp command for memory registration
 * @iwdev: zrdma device
 * @iwmr: zrdma mr pointer
 * @access: access for MR
 */
int zxdh_hwreg_mr(struct zxdh_device *iwdev, struct zxdh_mr *iwmr, u16 access)
{
	struct zxdh_pbl *iwpbl = &iwmr->iwpbl;
	struct zxdh_reg_ns_stag_info *stag_info;
	struct zxdh_pd *iwpd = to_iwpd(iwmr->ibmr.pd);
	struct zxdh_pble_alloc *palloc = &iwpbl->pble_alloc;
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	int ret;

	cqp_request = zxdh_alloc_and_get_cqp_request(&iwdev->rf->cqp, true);
	if (!cqp_request) {
		pr_err("[zxdh_rdma] %s[%d]: alloc_cqp_request failed\n", __func__, __LINE__);
		return -ENOMEM;
	}

	cqp_info = &cqp_request->info;
	stag_info = &cqp_info->in.u.mr_reg_non_shared.info;
	memset(stag_info, 0, sizeof(*stag_info));
	stag_info->va = iwpbl->user_base;
	stag_info->stag_idx = iwmr->stag >> ZXDH_CQPSQ_STAG_IDX_S;
	stag_info->stag_key = (u8)iwmr->stag;
	stag_info->total_len = iwmr->len;
	stag_info->access_rights = zxdh_get_mr_access(access);
	stag_info->pd_id = iwpd->sc_pd.pd_id;
	if (stag_info->access_rights & ZXDH_ACCESS_FLAGS_ZERO_BASED)
		stag_info->addr_type = ZXDH_ADDR_TYPE_ZERO_BASED;
	else
		stag_info->addr_type = ZXDH_ADDR_TYPE_VA_BASED;
	stag_info->page_size = iwmr->page_size;

	if (iwpbl->pbl_allocated) {
		if (palloc->level == PBLE_LEVEL_1) {
			stag_info->first_pm_pbl_index = palloc->level1.idx;
			stag_info->chunk_size = 1;
		} else {
			stag_info->first_pm_pbl_index = palloc->level2.root.idx;
			stag_info->chunk_size = 3;
		}
	} else {
		stag_info->reg_addr_pa = iwmr->pgaddrmem[0];
	}

	cqp_info->cqp_cmd = ZXDH_OP_MR_REG_NON_SHARED;
	cqp_info->post_sq = 1;
	cqp_info->in.u.mr_reg_non_shared.dev = &iwdev->rf->sc_dev;
	cqp_info->in.u.mr_reg_non_shared.scratch = (uintptr_t)cqp_request;
	ret = zxdh_handle_cqp_op(iwdev->rf, cqp_request);
	zxdh_put_cqp_request(&iwdev->rf->cqp, cqp_request);

	if (!ret)
		iwmr->is_hwreg = 1;

	return ret;
}

#ifdef SET_BEST_PAGE_SZ_V1
/**
 * zxdh_set_hugetlb_val - set MR pg size and mask to huge pg values.
 * @addr: virtual address
 * @iwmr: mr pointer for this memory registration
 */
static void zxdh_set_hugetlb_val(u64 addr, struct zxdh_mr *iwmr)
{
	struct vm_area_struct *vma;
	struct hstate *h;

	vma = find_vma(current->mm, addr);
	if (vma && is_vm_hugetlb_page(vma)) {
		h = hstate_vma(vma);
		if (huge_page_size(h) == 0x200000 ||
		    huge_page_size(h) == 0x40000000) {
			iwmr->page_size = huge_page_size(h);
			iwmr->page_msk = huge_page_mask(h);
		}
	}
}

#endif

#ifndef SET_BEST_PAGE_SZ_V1
static struct zxdh_mr *zxdh_alloc_iwmr(struct ib_umem *region,
					 struct ib_pd *pd, u64 virt, u64 len,
					 enum zxdh_memreg_type reg_type)
#else
static struct zxdh_mr *zxdh_alloc_iwmr(struct ib_umem *region,
					 struct ib_pd *pd, u64 virt, u64 start,u64 len,
					 enum zxdh_memreg_type reg_type)
#endif /* SET_BEST_PAGE_SZ_V1 */
{
	struct zxdh_pbl *iwpbl;
	struct zxdh_mr *iwmr;

	iwmr = kzalloc(sizeof(*iwmr), GFP_KERNEL);
	if (!iwmr) {
        pr_err("[zxdh_rdma] %s[%d]: kzalloc failed, size=0x%lx\n", __func__, __LINE__, sizeof(*iwmr));
		return ERR_PTR(-ENOMEM);
	}

	iwpbl = &iwmr->iwpbl;
	iwpbl->iwmr = iwmr;
	iwmr->region = region;
	iwmr->ibmr.pd = pd;
	iwmr->ibmr.device = pd->device;
	iwmr->ibmr.iova = virt;
	iwmr->ibmr.length = len;
	iwmr->page_size = PAGE_SIZE;

#ifdef SET_BEST_PAGE_SZ_V1
	iwmr->page_msk = PAGE_MASK;
	if (region->hugetlb && reg_type == ZXDH_MEMREG_TYPE_MEM)
		zxdh_set_hugetlb_val(start, iwmr);
#endif
#ifdef SET_BEST_PAGE_SZ_V2
	if (reg_type == ZXDH_MEMREG_TYPE_MEM) {
		iwmr->page_size = ib_umem_find_best_pgsz(
			region, SZ_4K | SZ_64K | SZ_2M | SZ_1G, virt);
		if (unlikely(!iwmr->page_size)) {
            pr_err("[zxdh_rdma] %s[%d]: find best pgsz failed, page_size=0x%llx\n", __func__, __LINE__, iwmr->page_size);
			kfree(iwmr);
			ib_umem_release(region);
			return ERR_PTR(-EOPNOTSUPP);
		}
	}
#endif
	iwmr->len = region->length;
	iwpbl->user_base = virt;
	iwmr->type = reg_type;
#ifdef rdma_umem_for_each_dma_block
#ifdef ib_umem_num_dma_blocks
	iwmr->page_cnt = ib_umem_num_dma_blocks(region, iwmr->page_size);
#else
	iwmr->page_cnt =
		zxdh_ib_umem_num_dma_blocks(region, iwmr->page_size, virt);
#endif
#else
	iwmr->page_cnt =
		zxdh_ib_umem_num_dma_blocks(region, iwmr->page_size, virt);
#endif
	return iwmr;
}

static void zxdh_free_iwmr(struct zxdh_mr *iwmr)
{
	kfree(iwmr);
}

static int zxdh_reg_user_mr_type_mem(struct zxdh_mr *iwmr, int access,
				      struct ib_udata *udata, bool dmabuf_flag)
{
	struct zxdh_device *iwdev = to_iwdev(iwmr->ibmr.device);
	struct zxdh_pbl *iwpbl = &iwmr->iwpbl;
	struct zxdh_reg_mr_resp resp = {};
	struct zxdh_pble_alloc *palloc;
	bool use_pbles = (iwmr->page_cnt != 1);
	int ret, err = -EINVAL;
	u32 stag = 0;

	palloc = &iwpbl->pble_alloc;
	err = zxdh_setup_pbles(iwdev->rf, iwmr, use_pbles, PBLE_MR); // mr
	if (err) {
		pr_err("[zxdh_rdma] %s[%d]: setup_pbles failed, err=%d, reg_type=%d, use_pbles=0x%x\n", __func__, __LINE__, err, ZXDH_MEMREG_TYPE_MEM, use_pbles);
		return err;
	}

	if (use_pbles) {
		ret = zxdh_check_mr_contiguous(palloc, iwmr->page_size);
		if (ret) {
			zxdh_free_pble(iwdev->rf->pble_mr_rsrc, palloc);
			iwpbl->pbl_allocated = false;
		}
	}

	stag = zxdh_create_stag(iwdev);
	if (!stag) {
		err = -ENOMEM;
		pr_err("[zxdh_rdma] %s[%d]: create_stag failed, err=%d, reg_type=%d, stag=%d\n", __func__, __LINE__, err, ZXDH_MEMREG_TYPE_MEM, stag);
		goto free_pble;
	}

	iwmr->stag = stag;
	iwmr->ibmr.rkey = stag;
	iwmr->ibmr.lkey = stag;
	iwmr->access = access;
	err = zxdh_hwreg_mr(iwdev, iwmr, access);
	if (err) {
		pr_err("[zxdh_rdma] %s[%d]: hwreg_mr failed, err=%d, reg_type=%d, access=0x%x\n", __func__, __LINE__, err, ZXDH_MEMREG_TYPE_MEM, access);
		goto err_hwreg;
	}

	if (dmabuf_flag)
		return 0;

	if (iwpbl->pbl_allocated == true) {
		if (iwpbl->pble_alloc.level == PBLE_LEVEL_1) {
			resp.mr_pa_low = iwpbl->pble_alloc.level1.idx;
			resp.mr_pa_hig = 0;
			resp.leaf_pbl_size = 1;
		} else {
			resp.mr_pa_low =
				iwpbl->pble_alloc.level2.root.idx;
			resp.mr_pa_hig = 0;
			resp.leaf_pbl_size = 3;
		}

	} else {
		resp.mr_pa_low = (u32)(iwmr->pgaddrmem[0] & 0xffffffff);
		resp.mr_pa_hig = (u32)((iwmr->pgaddrmem[0] &
					0xffffffff00000000) >>
						32);
		resp.leaf_pbl_size = 0;
	}

	if (iwmr->page_size == 0x40000000)
		resp.host_page_size = ZXDH_PAGE_SIZE_1G;
	else if (iwmr->page_size == 0x200000)
		resp.host_page_size = ZXDH_PAGE_SIZE_2M;
	else if (iwmr->page_size == 0x10000)
		resp.host_page_size = ZXDH_PAGE_SIZE_64K;
	else if (iwmr->page_size == 0x1000)
		resp.host_page_size = ZXDH_PAGE_SIZE_4K;

	err = ib_copy_to_udata(udata, &resp, min(sizeof(resp), udata->outlen));
	if (err) {
		pr_err("[zxdh_rdma] %s[%d]: copy to udata failed, sizeof(resp)=0x%lx, outlen=%#zx\n", __func__, __LINE__, sizeof(resp), udata->outlen);
		goto err_hwreg;
	}

	return 0;
err_hwreg:
	if (stag)
		zxdh_free_stag(iwdev, stag);
free_pble:
	if (palloc->level != PBLE_LEVEL_0 && iwpbl->pbl_allocated)
		zxdh_free_pble(iwdev->rf->pble_mr_rsrc, palloc);
	pr_err("[zxdh_rdma] %s process failed: err=%d, reg_type=%d\n", __func__, err, ZXDH_MEMREG_TYPE_MEM);
	return err;
}

/**
 * zxdh_reg_user_mr - Register a user memory region
 * @pd: ptr of pd
 * @start: virtual start address
 * @len: length of mr
 * @virt: virtual address
 * @access: access of mr
 * @dmah: DMA handle (only present when ZXDH_REG_USER_MR_VER_2 is defined)
 * @udata: user data
 */
#ifdef ZXDH_REG_USER_MR_VER_2
static struct ib_mr *zxdh_reg_user_mr(struct ib_pd *pd, u64 start, u64 len,
                                      u64 virt, int access,
                                      struct ib_dmah *dmah,
                                      struct ib_udata *udata)
#else
static struct ib_mr *zxdh_reg_user_mr(struct ib_pd *pd, u64 start, u64 len,
                                      u64 virt, int access,
                                      struct ib_udata *udata)
#endif
{
	struct zxdh_device *iwdev = to_iwdev(pd->device);
	struct zxdh_ucontext *ucontext;
	struct zxdh_pble_alloc *palloc = NULL;
	struct zxdh_pbl *iwpbl;
	struct zxdh_mr *iwmr;
	struct ib_umem *region = NULL;
	struct zxdh_mem_reg_req req = {};
	struct zxdh_reg_mr_resp resp = {};
	u32 total = 0, stag = 0;
	u8 shadow_pgcnt = 1;
	bool use_pbles = false;
	unsigned long flags;
	int err = -EINVAL;
	u32 page_cnt = 0;

	if (!len || len > iwdev->rf->sc_dev.hw_attrs.max_mr_size) {
        pr_err("[zxdh_rdma] %s[%d]: error size, start=0x%llx, len=0x%llx, access=0x%x, max_mr_size=0x%llx\n", __func__, __LINE__, 
			start, len, access, iwdev->rf->sc_dev.hw_attrs.max_mr_size);
		return ERR_PTR(-EINVAL);
    }

	if (ib_copy_from_udata(&req, udata, min(sizeof(req), udata->inlen))) {
        pr_err("[zxdh_rdma] %s[%d]: copy from udata failed, sizeof(req)=0x%lx, inlen=%#zx\n", __func__, __LINE__, sizeof(req), udata->inlen);
		ib_umem_release(region);
		return ERR_PTR(-EFAULT);
	}

	if (req.reg_type == ZXDH_MEMREG_TYPE_EXTERNEL_MEM) { //DH使用ext内存,物理寻址，注册MR流程发生变化

		iwmr = kzalloc(sizeof(*iwmr), GFP_KERNEL);
		if (!iwmr)
		{
			pr_err("[zxdh_rdma] %s[%d]: kzalloc failed, size=0x%lx\n", __func__, __LINE__, sizeof(*iwmr));
			return ERR_PTR(-ENOMEM);
		}

		iwpbl = &iwmr->iwpbl;
		iwpbl->iwmr = iwmr;
		iwmr->ibmr.pd = pd;
		iwmr->ibmr.device = pd->device;
		iwmr->ibmr.iova = start;  //start表示cpu可访问的ext内存,virt表示device可访问的ext内存;
		iwmr->ibmr.length = len;
		iwmr->len = len;
		iwmr->page_size = PAGE_SIZE;
		iwpbl->user_base = start;
		iwmr->pgaddrmem[0] = virt;

		stag = zxdh_create_stag(iwdev);
		if (!stag) {
			err = -ENOMEM;
            pr_err("[zxdh_rdma] %s[%d]: create_stag failed, err=%d, reg_type=%d, stag=%d\n", __func__, __LINE__, err, req.reg_type, stag);
			goto error;
		}

		iwmr->stag = stag;
		iwmr->ibmr.rkey = stag;
		iwmr->ibmr.lkey = stag;
		iwmr->access = access;
		err = zxdh_hwreg_mr(iwdev, iwmr, access);
		if (err) {
            pr_err("[zxdh_rdma] %s[%d]: hwreg_mr failed, err=%d, reg_type=%d, access=0x%x\n", __func__, __LINE__, err, req.reg_type, access);
			zxdh_free_stag(iwdev, stag);
			goto error;
		}

		resp.mr_pa_low = (u32)(iwmr->pgaddrmem[0] & 0xffffffff);
		resp.mr_pa_hig = (u32)((iwmr->pgaddrmem[0] &
					0xffffffff00000000) >>
						32);
		resp.leaf_pbl_size = 0;

		err = ib_copy_to_udata(udata, &resp,
						min(sizeof(resp), udata->outlen));
		if (err) {
			zxdh_free_stag(iwdev, stag);
			goto error;
		}

		return &iwmr->ibmr;
	}
	else {	//非外部内存，使用host内存标准流程
#ifdef IB_UMEM_GET_V3
	region = ib_umem_get(pd->device, start, len, access);
#endif
#ifdef IB_UMEM_GET_V2
	region = ib_umem_get(udata, start, len, access);
#endif
#ifdef IB_UMEM_GET_V1
	region = ib_umem_get(udata, start, len, access, 0);
#endif
#ifdef IB_UMEM_GET_V0
	region = ib_umem_get(pd->uobject->context, start, len, access, 0);
#endif

	if (IS_ERR(region)) {
		pr_err("[zxdh_rdma] %s[%d] VERBS: Failed to create ib_umem region, errno=%ld, start=0x%llx, len=0x%llx, access=0x%x\n", __func__, __LINE__, 
			PTR_ERR(region), start, len, access);
		return (struct ib_mr *)region;
	}

#ifndef SET_BEST_PAGE_SZ_V1
	iwmr = zxdh_alloc_iwmr(region, pd, virt, len, req.reg_type);
#else
	iwmr = zxdh_alloc_iwmr(region, pd, virt, start, len, req.reg_type);
#endif
	if (IS_ERR(iwmr)) {
        pr_err("[zxdh_rdma] %s[%d]: kzalloc failed, size=0x%lx\n", __func__, __LINE__, sizeof(*iwmr));
		ib_umem_release(region);
		return ERR_PTR(-ENOMEM);
	}
	iwpbl = &iwmr->iwpbl;
	palloc = &iwpbl->pble_alloc;

    if (req.reg_type != ZXDH_MEMREG_TYPE_MEM) {
        if (iwmr->page_size == SZ_64K)
            page_cnt = (iwmr->page_cnt * ZXDH_4K_TO_64K_MUL);
        else 
            page_cnt = iwmr->page_cnt;
	}

	switch (req.reg_type) {
	case ZXDH_MEMREG_TYPE_QP:
		total = req.sq_pages + req.rq_pages + shadow_pgcnt;
		if (total > page_cnt) {
			err = -EINVAL;
            pr_err("[zxdh_rdma] %s[%d]: page_cnt compare failed, reg_type=%d, total=0x%x, page_cnt=0x%x\n", __func__, __LINE__, req.reg_type, total, page_cnt);
			goto error;
		}
		total = req.sq_pages + req.rq_pages;
		use_pbles = (total > 2);
        if (iwmr->page_size == SZ_64K)
            use_pbles = true;
		err = zxdh_handle_q_mem(iwdev, &req, iwpbl, use_pbles);
		if (err) {
            pr_err("[zxdh_rdma] %s[%d]: handle_q_mem failed, err=%d, reg_type=%d, use_pbles=0x%x\n", __func__, __LINE__, err, req.reg_type, total > 2);
			goto error_free_pble;
        }

		ucontext = kc_rdma_udata_to_drv_context(pd, udata);
		spin_lock_irqsave(&ucontext->qp_reg_mem_list_lock, flags);
		list_add_tail(&iwpbl->list, &ucontext->qp_reg_mem_list);
		iwpbl->on_list = true;
		spin_unlock_irqrestore(&ucontext->qp_reg_mem_list_lock, flags);
		break;
	case ZXDH_MEMREG_TYPE_CQ:
		if (iwdev->rf->sc_dev.hw_attrs.uk_attrs.feature_flags &
		    ZXDH_FEATURE_CQ_RESIZE)
			shadow_pgcnt = 0;
		total = req.cq_pages + shadow_pgcnt;
		if (total > page_cnt) {
			err = -EINVAL;
            pr_err("[zxdh_rdma] %s[%d]: page_cnt compare failed, reg_type=%d, total=0x%x, page_cnt=0x%x\n", __func__, __LINE__, req.reg_type, total, page_cnt);
			goto error_free_pble;
		}

		use_pbles = (req.cq_pages > 1);
        if (iwmr->page_size == SZ_64K)
            use_pbles = true;
		err = zxdh_handle_q_mem(iwdev, &req, iwpbl, use_pbles);
		if (err) {
            pr_err("[zxdh_rdma] %s[%d]: handle_q_mem failed, err=%d, reg_type=%d, use_pbles=0x%x\n", __func__, __LINE__, err, req.reg_type, total > 2);
			goto error_free_pble;
        }

		ucontext = kc_rdma_udata_to_drv_context(pd, udata);
		spin_lock_irqsave(&ucontext->cq_reg_mem_list_lock, flags);
		list_add_tail(&iwpbl->list, &ucontext->cq_reg_mem_list);
		iwpbl->on_list = true;
		spin_unlock_irqrestore(&ucontext->cq_reg_mem_list_lock, flags);
		break;
	case ZXDH_MEMREG_TYPE_SRQ:
		total = req.srq_pages + req.srq_list_pages + shadow_pgcnt;
		if (total > page_cnt) {
			err = -EINVAL;
            pr_err("[zxdh_rdma] %s[%d]: page_cnt compare failed, reg_type=%d, total=0x%x, page_cnt=0x%x\n", __func__, __LINE__, req.reg_type, total, page_cnt);
			goto error_free_pble;
		}

		total = req.srq_pages + req.srq_list_pages;
		use_pbles = (total > 2);
        if (iwmr->page_size == SZ_64K)
            use_pbles = true;
		err = zxdh_handle_q_mem(iwdev, &req, iwpbl, use_pbles);
		if (err) {
            pr_err("[zxdh_rdma] %s[%d]: handle_q_mem failed, err=%d, reg_type=%d, use_pbles=0x%x\n", __func__, __LINE__, err, req.reg_type, total > 2);
			goto error_free_pble;
        }

		ucontext = kc_rdma_udata_to_drv_context(pd, udata);
		spin_lock_irqsave(&ucontext->srq_reg_mem_list_lock, flags);
		list_add_tail(&iwpbl->list, &ucontext->srq_reg_mem_list);
		iwpbl->on_list = true;
		spin_unlock_irqrestore(&ucontext->srq_reg_mem_list_lock, flags);
		break;
	case ZXDH_MEMREG_TYPE_MEM:
		err = zxdh_reg_user_mr_type_mem(iwmr, access, udata, false);
		if (err)
			goto error;
		break;
	default:
        pr_err("[zxdh_rdma] %s[%d]: error reg_type=%d\n", __func__, __LINE__, req.reg_type);
		goto error_free_pble;
	}

	iwmr->type = req.reg_type;

	return &iwmr->ibmr;
	}
error_free_pble:
    pr_err("[zxdh_rdma] %s process failed: err=%d, reg_type=%d\n", __func__, err, req.reg_type);
	if (palloc->level != PBLE_LEVEL_0 && iwpbl->pbl_allocated)
		zxdh_free_pble(iwdev->rf->pble_rsrc, palloc);
error:
	if (req.reg_type != ZXDH_MEMREG_TYPE_EXTERNEL_MEM)
	{
		ib_umem_release(region);
	}
	zxdh_free_iwmr(iwmr);

	return ERR_PTR(err);
}

#ifdef SET_DMABUF
static void
zxdh_dmabuf_unsupported_move_notify(struct dma_buf_attachment *attach)
{
	struct ib_umem_dmabuf *umem_dmabuf = attach->importer_priv;

	ibdev_warn_ratelimited(umem_dmabuf->umem.ibdev,
			       "Invalidate callback should not be called when memory is pinned\n");
}

static struct dma_buf_attach_ops zxdh_dmabuf_attach_pinned_ops = {
	.allow_peer2peer = true,
	.move_notify = zxdh_dmabuf_unsupported_move_notify,
};

static struct ib_umem_dmabuf *
zxdh_dmabuf_get_pinned_with_dma_device(struct ib_device *device,
                      unsigned long offset, size_t size,
                      int fd, int access)
{
	struct ib_umem_dmabuf *umem_dmabuf;
	int err;

	umem_dmabuf = ib_umem_dmabuf_get(device, offset,
							 size, fd, access,
							 &zxdh_dmabuf_attach_pinned_ops);
	if (IS_ERR(umem_dmabuf))
		return umem_dmabuf;

	dma_resv_lock(umem_dmabuf->attach->dmabuf->resv, NULL);
	err = dma_buf_pin(umem_dmabuf->attach);
	if (err)
		goto err_release;

	err = ib_umem_dmabuf_map_pages(umem_dmabuf);
	if (err)
		goto err_unpin;
	dma_resv_unlock(umem_dmabuf->attach->dmabuf->resv);

	return umem_dmabuf;

err_unpin:
	dma_buf_unpin(umem_dmabuf->attach);
err_release:
	dma_resv_unlock(umem_dmabuf->attach->dmabuf->resv);
	ib_umem_release(&umem_dmabuf->umem);
	return ERR_PTR(err);
}

/**
 * zxdh_reg_user_mr_dmabuf - register dma-buf based memory region
 * @pd: ibpd pointer
 * @start: offset
 * @len: size of memory to register
 * @virt: virtual address of memory to register
 * @fd: fd associated with dma_buf
 * @access: access rights
 * @attrs: uverbs attribute bundle carrying the user data. Kernels and OFED
 *         releases from before the attrs conversion pass a
 *         struct ib_udata *udata here instead (REG_USER_MR_DMABUF_VER_1).
 */
#ifdef REG_USER_MR_DMABUF_VER_2
static struct ib_mr *zxdh_reg_user_mr_dmabuf(struct ib_pd *pd, u64 start, u64 len,
				       u64 virt, int fd, int access,
				       struct uverbs_attr_bundle *attrs)
#else
static struct ib_mr *zxdh_reg_user_mr_dmabuf(struct ib_pd *pd, u64 start, u64 len,
				       u64 virt, int fd, int access,
				       struct ib_udata *udata)
#endif
{
#ifdef REG_USER_MR_DMABUF_VER_2
	struct ib_udata *udata = &attrs->driver_udata;
#endif
	struct zxdh_device *iwdev = to_iwdev(pd->device);
	struct ib_umem_dmabuf *umem_dmabuf;
	struct zxdh_mr *iwmr;
	int err;

	pr_debug("[zxdh_rdma] %s[%d]: start=0x%llx, len=0x%llx, virt=0x%llx, fd=%d, access=%d, max_mr_size=0x%llx\n",
		__func__, __LINE__, start, len, virt, fd, access, iwdev->rf->sc_dev.hw_attrs.max_mr_size);
	if (len > iwdev->rf->sc_dev.hw_attrs.max_mr_size)
		return ERR_PTR(-EINVAL);

	umem_dmabuf = zxdh_dmabuf_get_pinned_with_dma_device(pd->device, start, len, fd, access);
	if (IS_ERR(umem_dmabuf)) {
		err = PTR_ERR(umem_dmabuf);
		pr_debug("[zxdh_rdma] Failed to get dmabuf umem[%d]\n", err);
		return ERR_PTR(err);
	}

#ifndef SET_BEST_PAGE_SZ_V1
	iwmr = zxdh_alloc_iwmr(&umem_dmabuf->umem, pd, virt, len, ZXDH_MEMREG_TYPE_MEM);
#else
	iwmr = zxdh_alloc_iwmr(&umem_dmabuf->umem, pd, virt, start, len, ZXDH_MEMREG_TYPE_MEM);
#endif
	if (IS_ERR(iwmr)) {
		pr_err("[zxdh_rdma] %s[%d]: zxdh_alloc_iwmr failed, size=0x%lx\n", __func__, __LINE__, sizeof(*iwmr));
		err = PTR_ERR(iwmr);
		goto err_release;
	}

	err = zxdh_reg_user_mr_type_mem(iwmr, access, udata, true);
	if (err)
		goto err_iwmr;

	return &iwmr->ibmr;

err_iwmr:
	zxdh_free_iwmr(iwmr);

err_release:
	ib_umem_release(&umem_dmabuf->umem);

	return ERR_PTR(err);
}
#endif /* SET_DMABUF */

int zxdh_hwdereg_mr(struct ib_mr *ib_mr)
{
	struct zxdh_device *iwdev = to_iwdev(ib_mr->device);
	struct zxdh_mr *iwmr = to_iwmr(ib_mr);
	struct zxdh_pd *iwpd = to_iwpd(ib_mr->pd);
	struct zxdh_dealloc_stag_info *info;
	struct zxdh_pbl *iwpbl = &iwmr->iwpbl;
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	int status;

	/* Skip HW MR de-register when it is already de-registered
	 * during an MR re-reregister and the re-registration fails
	 */
	if (!iwmr->is_hwreg)
		return 0;

	cqp_request = zxdh_alloc_and_get_cqp_request(&iwdev->rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	cqp_info = &cqp_request->info;
	info = &cqp_info->in.u.dealloc_stag.info;
	memset(info, 0, sizeof(*info));
	info->pd_id = iwpd->sc_pd.pd_id;
	info->stag_idx = RS_64_1(ib_mr->rkey, ZXDH_CQPSQ_STAG_IDX_S);
	info->mr = true;
	if (iwpbl->pbl_allocated)
		info->dealloc_pbl = true;

	cqp_info->cqp_cmd = ZXDH_OP_DEALLOC_STAG;
	cqp_info->post_sq = 1;
	cqp_info->in.u.dealloc_stag.dev = &iwdev->rf->sc_dev;
	cqp_info->in.u.dealloc_stag.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(iwdev->rf, cqp_request);
	zxdh_put_cqp_request(&iwdev->rf->cqp, cqp_request);

	if (!status)
		iwmr->is_hwreg = 0;

	return status;
}

/*
 * zxdh_rereg_mr_trans - Re-register a user MR for a change translation.
 * @iwmr: ptr of iwmr
 * @start: virtual start address
 * @len: length of mr
 * @virt: virtual address
 *
 * Re-register a user memory region when a change translation is requested.
 * Re-register a new region while reusing the stag from the original registration.
 */
struct ib_mr *zxdh_rereg_mr_trans(struct zxdh_mr *iwmr, u64 start, u64 len,
				  u64 virt, struct ib_udata *udata)
{
	struct zxdh_device *iwdev = to_iwdev(iwmr->ibmr.device);
	struct zxdh_pbl *iwpbl = &iwmr->iwpbl;
	struct zxdh_pble_alloc *palloc = &iwpbl->pble_alloc;
	struct ib_pd *pd = iwmr->ibmr.pd;
	struct ib_umem *region;
	bool use_pbles;
	int err;

#ifdef IB_UMEM_GET_V3
	region = ib_umem_get(pd->device, start, len, iwmr->access);
#endif
#ifdef IB_UMEM_GET_V2
	region = ib_umem_get(udata, start, len, iwmr->access);
#endif
#ifdef IB_UMEM_GET_V1
	region = ib_umem_get(udata, start, len, iwmr->access, 0);
#endif
#ifdef IB_UMEM_GET_V0
	region = ib_umem_get(pd->uobject->context, start, len, iwmr->access, 0);
#endif

	if (IS_ERR(region)) {
		pr_err("[zxdh_rdma] VERBS: Failed to create ib_umem region\n");
		return (struct ib_mr *)region;
	}

	iwmr->region = region;
	iwmr->ibmr.iova = virt;
	iwmr->ibmr.pd = pd;
	iwmr->page_size = PAGE_SIZE;

#ifdef SET_BEST_PAGE_SZ_V1
	iwmr->page_msk = PAGE_MASK;
	if (region->hugetlb)
		zxdh_set_hugetlb_val(start, iwmr);
#endif
#ifdef SET_BEST_PAGE_SZ_V2
	iwmr->page_size =
		ib_umem_find_best_pgsz(region, SZ_4K | SZ_2M | SZ_1G, virt);
	if (unlikely(!iwmr->page_size)) {
		ib_umem_release(region);
		return ERR_PTR(-EOPNOTSUPP);
	}
#endif
	iwmr->len = region->length;
	iwpbl->user_base = virt;
#ifdef rdma_umem_for_each_dma_block
#ifdef ib_umem_num_dma_blocks
	iwmr->page_cnt = ib_umem_num_dma_blocks(region, iwmr->page_size);
#else
	iwmr->page_cnt =
		zxdh_ib_umem_num_dma_blocks(region, iwmr->page_size, virt);
#endif
#else
	iwmr->page_cnt =
		zxdh_ib_umem_num_dma_blocks(region, iwmr->page_size, virt);
#endif

	use_pbles = (iwmr->page_cnt != 1);

	err = zxdh_setup_pbles(iwdev->rf, iwmr, use_pbles, PBLE_MR); // mr
	if (err)
		goto error;

	if (use_pbles) {
		err = zxdh_check_mr_contiguous(palloc, iwmr->page_size);
		if (err) {
			zxdh_free_pble(iwdev->rf->pble_mr_rsrc, palloc);
			iwpbl->pbl_allocated = false;
		}
	}

	err = zxdh_hwreg_mr(iwdev, iwmr, iwmr->access);
	if (err)
		goto error;

	return &iwmr->ibmr;

error:
	if (palloc->level != PBLE_LEVEL_0 && iwpbl->pbl_allocated) {
		zxdh_free_pble(iwdev->rf->pble_mr_rsrc, palloc);
		iwpbl->pbl_allocated = false;
	}
	ib_umem_release(region);
	iwmr->region = NULL;

	return ERR_PTR(err);
}

/**
 * zxdh_reg_phys_mr - register kernel physical memory
 * @pd: ibpd pointer
 * @addr: physical address of memory to register
 * @size: size of memory to register
 * @access: Access rights
 * @iova_start: start of virtual address for physical buffers
 */
struct ib_mr *zxdh_reg_phys_mr(struct ib_pd *pd, u64 addr, u64 size, int access,
			       u64 *iova_start)
{
	struct zxdh_device *iwdev = to_iwdev(pd->device);
	struct zxdh_pbl *iwpbl;
	struct zxdh_mr *iwmr;
	u32 stag;
	int ret;

	iwmr = kzalloc(sizeof(*iwmr), GFP_KERNEL);
	if (!iwmr)
		return ERR_PTR(-ENOMEM);

	iwmr->ibmr.pd = pd;
	iwmr->ibmr.device = pd->device;
	iwpbl = &iwmr->iwpbl;
	iwpbl->iwmr = iwmr;
	iwmr->type = ZXDH_MEMREG_TYPE_MEM;
	iwpbl->user_base = *iova_start;
	stag = zxdh_create_stag(iwdev);
	if (!stag) {
		ret = -ENOMEM;
		goto err;
	}

	iwmr->stag = stag;
	iwmr->ibmr.iova = *iova_start;
	iwmr->ibmr.rkey = stag;
	iwmr->ibmr.lkey = stag;
	iwmr->page_cnt = 1;
	iwmr->pgaddrmem[0] = addr;
	iwmr->len = size;
	iwmr->page_size = SZ_4K;
	ret = zxdh_hwreg_mr(iwdev, iwmr, access);
	if (ret) {
		zxdh_free_stag(iwdev, stag);
		goto err;
	}

	return &iwmr->ibmr;

err:
	kfree(iwmr);

	return ERR_PTR(ret);
}

/**
 * zxdh_get_dma_mr - register physical mem
 * @pd: ptr of pd
 * @acc: access for memory
 */
static struct ib_mr *zxdh_get_dma_mr(struct ib_pd *pd, int acc)
{
	u64 kva = 0;

	return zxdh_reg_phys_mr(pd, 0, 0, acc, &kva);
}

/**
 * zxdh_del_memlist - Deleting pbl list entries for CQ/QP
 * @iwmr: iwmr for IB's user page addresses
 * @ucontext: ptr to user context
 */
void zxdh_del_memlist(struct zxdh_mr *iwmr, struct zxdh_ucontext *ucontext)
{
	struct zxdh_pbl *iwpbl = &iwmr->iwpbl;
	unsigned long flags;

	switch (iwmr->type) {
	case ZXDH_MEMREG_TYPE_CQ:
		spin_lock_irqsave(&ucontext->cq_reg_mem_list_lock, flags);
		if (iwpbl->on_list) {
			iwpbl->on_list = false;
			list_del(&iwpbl->list);
		}
		spin_unlock_irqrestore(&ucontext->cq_reg_mem_list_lock, flags);
		break;
	case ZXDH_MEMREG_TYPE_QP:
		spin_lock_irqsave(&ucontext->qp_reg_mem_list_lock, flags);
		if (iwpbl->on_list) {
			iwpbl->on_list = false;
			list_del(&iwpbl->list);
		}
		spin_unlock_irqrestore(&ucontext->qp_reg_mem_list_lock, flags);
		break;
	case ZXDH_MEMREG_TYPE_SRQ:
		spin_lock_irqsave(&ucontext->srq_reg_mem_list_lock, flags);
		if (iwpbl->on_list) {
			iwpbl->on_list = false;
			list_del(&iwpbl->list);
		}
		spin_unlock_irqrestore(&ucontext->srq_reg_mem_list_lock, flags);
		break;
	default:
		break;
	}
}

/**
 * zxdh_copy_sg_list - copy sg list for qp
 * @sg_list: copied into sg_list
 * @sgl: copy from sgl
 * @num_sges: count of sg entries
 */
static void zxdh_copy_sg_list(struct zxdh_sge *sg_list, struct ib_sge *sgl,
			      int num_sges)
{
	unsigned int i;

	for (i = 0; i < num_sges; i++) {
		sg_list[i].tag_off = sgl[i].addr;
		sg_list[i].len = sgl[i].length;
		sg_list[i].stag = sgl[i].lkey;
	}
}

/**
 * zxdh_get_inline_data - get inline_multi_sge data
 * @inline_data: uint8_t*
 * @ib_wr: work request ptr
 * @len: sge total length
 */
static int zxdh_get_inline_data(uint8_t *inline_data,
				const struct ib_send_wr *ib_wr, __u32 *len)
{
	int num = 0;
	int offset = 0;

	while (num < ib_wr->num_sge) {
		*len += ib_wr->sg_list[num].length;
		if (*len > ZXDH_MAX_INLINE_DATA_SIZE) {
			pr_err("[zxdh_rdma] err:inline bytes over max inline length\n");
			return -EINVAL;
		}
		memcpy(inline_data + offset,
		       (void *)(uintptr_t)ib_wr->sg_list[num].addr,
		       ib_wr->sg_list[num].length);
		offset += ib_wr->sg_list[num].length;
		num++;
	}
	return 0;
}

/**
 * zxdh_post_send -  kernel application wr
 * @ibqp: qp ptr for wr
 * @ib_wr: work request ptr
 * @bad_wr: return of bad wr if err
 */
static int zxdh_post_send(struct ib_qp *ibqp,
			  kc_typeq_ib_wr struct ib_send_wr *ib_wr,
			  kc_typeq_ib_wr struct ib_send_wr **bad_wr)
{
	struct zxdh_qp *iwqp;
	struct zxdh_qp_uk *ukqp;
	struct zxdh_sc_dev *dev;
	struct zxdh_post_sq_info info;
	int err = 0;
	unsigned long flags;
	struct zxdh_ah *ah;

	iwqp = to_iwqp(ibqp);
	ukqp = &iwqp->sc_qp.qp_uk;
	dev = &iwqp->iwdev->rf->sc_dev;

	if (iwqp->iwarp_state != ZXDH_QPS_RTS && !iwqp->flush_issued) {
		*bad_wr = ib_wr;
		pr_info("[zxdh_rdma] %s[%d]: err:post send at state:%d\n", __func__, __LINE__, iwqp->iwarp_state);
		return -EINVAL;
	}
	if (dev->hw_attrs.skip_hw == true) {
		return -EINVAL;
	}


	spin_lock_irqsave(&iwqp->lock, flags);
	while (ib_wr) {
		memset(&info, 0, sizeof(info));
		info.wr_id = (ib_wr->wr_id);
		if ((ib_wr->send_flags & IB_SEND_SIGNALED) || iwqp->sig_all)
			info.signaled = true;
		if (ib_wr->send_flags & IB_SEND_FENCE)
			info.read_fence = true;
		switch (ib_wr->opcode) {
		case IB_WR_SEND_WITH_IMM:
			if (ukqp->qp_caps & ZXDH_SEND_WITH_IMM) {
				info.imm_data_valid = true;
				info.imm_data = ntohl(ib_wr->ex.imm_data);
			} else {
				err = -EINVAL;
				break;
			}
			fallthrough;
		case IB_WR_SEND:
		case IB_WR_SEND_WITH_INV:
			if (ib_wr->send_flags & IB_SEND_SOLICITED)
				info.solicited = 1;

			if (ib_wr->opcode == IB_WR_SEND) {
				if (iwqp->ibqp.qp_type == IB_QPT_UD ||
				    iwqp->ibqp.qp_type == IB_QPT_GSI)
					info.op_type = ZXDH_OP_TYPE_UD_SEND;
				else
					info.op_type = ZXDH_OP_TYPE_SEND;
			} else if (ib_wr->opcode == IB_WR_SEND_WITH_IMM) {
				if (iwqp->ibqp.qp_type == IB_QPT_UD ||
				    iwqp->ibqp.qp_type == IB_QPT_GSI)
					info.op_type =
						ZXDH_OP_TYPE_UD_SEND_WITH_IMM;
				else
					info.op_type =
						ZXDH_OP_TYPE_SEND_WITH_IMM;
			} else {
				info.op_type = ZXDH_OP_TYPE_SEND_INV;
				info.stag_to_inv = ib_wr->ex.invalidate_rkey;
			}

			if ((ib_wr->send_flags & IB_SEND_INLINE) &&
			    (ib_wr->num_sge != 0)) {
				err = zxdh_get_inline_data(
					iwqp->inline_data, ib_wr,
					&info.op.inline_send.len);
				if (err) {
					pr_err("[zxdh_rdma] err: get_inline_data failed\n");
					spin_unlock_irqrestore(&iwqp->lock,
							       flags);
					return -EINVAL;
				}
				info.op.inline_send.data = iwqp->inline_data;

				if (iwqp->ibqp.qp_type == IB_QPT_UD ||
				    iwqp->ibqp.qp_type == IB_QPT_GSI) {
					ah = to_iwah(ud_wr(ib_wr)->ah);
					info.op.inline_send.ah_id =
						ah->sc_ah.ah_info.ah_idx;
					info.op.inline_send.qkey =
						ud_wr(ib_wr)->remote_qkey;
					info.op.inline_send.dest_qp =
						ud_wr(ib_wr)->remote_qpn;
					err = zxdh_uk_ud_inline_send(
						ukqp, &info, false);
				} else {
					err = zxdh_uk_rc_inline_send(
						ukqp, &info, false);
				}
			} else {
				info.op.send.num_sges = ib_wr->num_sge;
				info.op.send.sg_list =
					(struct zxdh_sge *)ib_wr->sg_list;
				if (iwqp->ibqp.qp_type == IB_QPT_UD ||
				    iwqp->ibqp.qp_type == IB_QPT_GSI) {
					ah = to_iwah(ud_wr(ib_wr)->ah);
					info.op.send.ah_id =
						ah->sc_ah.ah_info.ah_idx;
					info.op.send.qkey =
						ud_wr(ib_wr)->remote_qkey;
					info.op.send.dest_qp =
						ud_wr(ib_wr)->remote_qpn;
					err = zxdh_uk_ud_send(ukqp, &info,
							      false);
				} else {
					err = zxdh_uk_rc_send(ukqp, &info,
							      false);
				}
			}
			break;
		case IB_WR_RDMA_WRITE_WITH_IMM:
			if (ukqp->qp_caps & ZXDH_WRITE_WITH_IMM) {
				info.imm_data_valid = true;
				info.imm_data = ntohl(ib_wr->ex.imm_data);
			} else {
				err = -EINVAL;
				break;
			}
			fallthrough;
		case IB_WR_RDMA_WRITE:
			if (ib_wr->send_flags & IB_SEND_SOLICITED)
				info.solicited = 1;

			if (ib_wr->opcode == IB_WR_RDMA_WRITE)
				info.op_type = ZXDH_OP_TYPE_WRITE;
			else
				info.op_type = ZXDH_OP_TYPE_WRITE_WITH_IMM;

			if ((ib_wr->send_flags & IB_SEND_INLINE) &&
			    (ib_wr->num_sge != 0)) {
				err = zxdh_get_inline_data(
					iwqp->inline_data, ib_wr,
					&info.op.inline_rdma_write.len);
				if (err) {
					pr_err("[zxdh_rdma] err: get_inline_data failed\n");
					spin_unlock_irqrestore(&iwqp->lock,
							       flags);
					return -EINVAL;
				}
				info.op.inline_rdma_write.data =
					iwqp->inline_data;

				info.op.inline_rdma_write.rem_addr.tag_off =
					rdma_wr(ib_wr)->remote_addr;
				info.op.inline_rdma_write.rem_addr.stag =
					rdma_wr(ib_wr)->rkey;
				err = zxdh_uk_inline_rdma_write(ukqp, &info,
								false);
			} else {
				info.op.rdma_write.lo_sg_list =
					(void *)ib_wr->sg_list;
				info.op.rdma_write.num_lo_sges = ib_wr->num_sge;
				info.op.rdma_write.rem_addr.tag_off =
					rdma_wr(ib_wr)->remote_addr;
				info.op.rdma_write.rem_addr.stag =
					rdma_wr(ib_wr)->rkey;
				err = zxdh_uk_rdma_write(ukqp, &info, false);
			}
			break;
		case IB_WR_RDMA_READ:
			if (ib_wr->num_sge >
			    dev->hw_attrs.uk_attrs.max_hw_read_sges) {
				err = -EINVAL;
				break;
			}
			info.op_type = ZXDH_OP_TYPE_READ;
			info.op.rdma_read.rem_addr.tag_off =
				rdma_wr(ib_wr)->remote_addr;
			info.op.rdma_read.rem_addr.stag = rdma_wr(ib_wr)->rkey;
			info.op.rdma_read.lo_sg_list = (void *)ib_wr->sg_list;
			info.op.rdma_read.num_lo_sges = ib_wr->num_sge;
			err = zxdh_uk_rdma_read(ukqp, &info, false);
			break;
		case IB_WR_LOCAL_INV:
			info.op_type = ZXDH_OP_TYPE_LOCAL_INV;
			info.op.inv_local_stag.target_stag =
				ib_wr->ex.invalidate_rkey;
			err = zxdh_uk_stag_local_invalidate(ukqp, &info, true);
			break;
		case IB_WR_REG_MR: {
			struct zxdh_mr *iwmr = to_iwmr(reg_wr(ib_wr)->mr);
			struct zxdh_pble_alloc *palloc =
				&iwmr->iwpbl.pble_alloc;
			struct zxdh_fast_reg_stag_info stag_info = {};

			stag_info.signaled = info.signaled;
			stag_info.read_fence = info.read_fence;
			stag_info.access_rights =
				zxdh_get_mr_access(reg_wr(ib_wr)->access);
			stag_info.stag_key = reg_wr(ib_wr)->key & 0xff;
			stag_info.stag_idx = reg_wr(ib_wr)->key >> 8;
			stag_info.page_size = reg_wr(ib_wr)->mr->page_size;
			stag_info.wr_id = ib_wr->wr_id;
			stag_info.addr_type = ZXDH_ADDR_TYPE_VA_BASED;
			stag_info.va = (void *)(uintptr_t)iwmr->ibmr.iova;
			stag_info.total_len = iwmr->ibmr.length;
			stag_info.reg_addr_pa = *palloc->level1.addr;
			stag_info.first_pm_pbl_index = palloc->level1.idx;
			stag_info.local_fence = ib_wr->send_flags &
						IB_SEND_FENCE;
			if (iwmr->npages > ZXDH_MIN_PAGES_PER_FMR)
				stag_info.chunk_size = 1;
			err = zxdh_sc_mr_fast_register(&iwqp->sc_qp, &stag_info,
						       true);
			break;
		}
		default:
			err = -EINVAL;
			pr_err("[zxdh_rdma] VERBS: upost_send bad opcode = 0x%x\n",
			       ib_wr->opcode);
			break;
		}

		if (err)
			break;
		ib_wr = ib_wr->next;
	}

	if (!iwqp->flush_issued)
		zxdh_uk_qp_post_wr(ukqp);
	else if (iwqp->flush_issued)
		mod_delayed_work(iwqp->iwdev->cleanup_wq, &iwqp->dwork_flush,
				 ZXDH_FLUSH_DELAY_MS);
	spin_unlock_irqrestore(&iwqp->lock, flags);
	if (err)
		*bad_wr = ib_wr;

	return err;
}

/**
 * zxdh_post_recv - post receive wr for kernel application
 * @ibqp: ib qp pointer
 * @ib_wr: work request for receive
 * @bad_wr: bad wr caused an error
 */
static int zxdh_post_recv(struct ib_qp *ibqp,
			  kc_typeq_ib_wr struct ib_recv_wr *ib_wr,
			  kc_typeq_ib_wr struct ib_recv_wr **bad_wr)
{
	struct zxdh_qp *iwqp = to_iwqp(ibqp);
	struct zxdh_qp_uk *ukqp = &iwqp->sc_qp.qp_uk;
	struct zxdh_post_rq_info post_recv = {};
	struct zxdh_sge *sg_list = iwqp->sg_list;
	unsigned long flags;
	int err = 0;
	struct zxdh_sc_dev *dev = iwqp->sc_qp.dev;
	struct zxdh_pci_f *rf = container_of(dev, struct zxdh_pci_f, sc_dev);

	if (iwqp->iwarp_state == ZXDH_QPS_RESET || iwqp->is_srq) {
		*bad_wr = ib_wr;
		return -EINVAL;
	}
	if (iwqp->sc_qp.dev->hw_attrs.skip_hw == true) {
		return -EINVAL;
	}

	spin_lock_irqsave(&iwqp->lock, flags);

	while (ib_wr) {
		if (rf->use_blue_flame_flag) {
			if (ib_wr->num_sge > dev->hw_attrs.uk_attrs.max_hw_rq_sges) {
				err = -EINVAL;
				pr_err("[zxdh_rdma] VERBS: post_recv error (err=%d),"
					"num_sge:%u is over max_hw_rq_sges:%u in blue_flame mode\n"
					"If you want to use RQ max SGE of %u, set use_blue_flame=false via dhtool\n",
					err,
					ib_wr->num_sge,
					dev->hw_attrs.uk_attrs.max_hw_rq_sges,
					dev->hw_attrs.uk_attrs.max_hw_wq_frags);
				goto out;
			}
		} else {
			if (ib_wr->num_sge > ukqp->max_rq_frag_cnt) {
				err = -EINVAL;
				goto out;
			}
		}
		post_recv.num_sges = ib_wr->num_sge;
		post_recv.wr_id = ib_wr->wr_id;
		zxdh_copy_sg_list(sg_list, ib_wr->sg_list, ib_wr->num_sge);
		post_recv.sg_list = sg_list;
		err = zxdh_uk_post_receive(ukqp, &post_recv);
		if (err) {
			pr_err("[zxdh_rdma] VERBS: post_recv err %d\n", err);
			goto out;
		}

		ib_wr = ib_wr->next;
	}

out:
	if (iwqp->flush_issued)
		mod_delayed_work(iwqp->iwdev->cleanup_wq, &iwqp->dwork_flush,
				 ZXDH_FLUSH_DELAY_MS);
	else
		zxdh_uk_qp_set_shadow_area(ukqp);
	spin_unlock_irqrestore(&iwqp->lock, flags);
	if (err)
		*bad_wr = ib_wr;

	return err;
}

/**
 * zxdh_flush_err_to_ib_wc_status - return change flush error code to IB status
 * @opcode: iwarp flush code
 */
static enum ib_wc_status
zxdh_flush_err_to_ib_wc_status(enum zxdh_flush_opcode opcode)
{
	switch (opcode) {
	case FLUSH_PROT_ERR:
		return IB_WC_LOC_PROT_ERR;
	case FLUSH_REM_ACCESS_ERR:
		return IB_WC_REM_ACCESS_ERR;
	case FLUSH_LOC_QP_OP_ERR:
		return IB_WC_LOC_QP_OP_ERR;
	case FLUSH_REM_OP_ERR:
		return IB_WC_REM_OP_ERR;
	case FLUSH_LOC_LEN_ERR:
		return IB_WC_LOC_LEN_ERR;
	case FLUSH_GENERAL_ERR:
		return IB_WC_WR_FLUSH_ERR;
	case FLUSH_MW_BIND_ERR:
		return IB_WC_MW_BIND_ERR;
	case FLUSH_REM_INV_REQ_ERR:
		return IB_WC_REM_INV_REQ_ERR;
	case FLUSH_RETRY_EXC_ERR:
		return IB_WC_RETRY_EXC_ERR;
	case FLUSH_FATAL_ERR:
	default:
		return IB_WC_FATAL_ERR;
	}
}

/**
 * zxdh_process_cqe - process cqe info
 * @entry: processed cqe
 * @cq_poll_info: cqe info
 */
static void zxdh_process_cqe(struct ib_wc *entry,
			     struct zxdh_cq_poll_info *cq_poll_info)
{
	struct zxdh_qp *iwqp;
	struct zxdh_sc_qp *qp;

	entry->wc_flags = 0;
	entry->pkey_index = 0;
	entry->wr_id = cq_poll_info->wr_id;

	qp = cq_poll_info->qp_handle;
	iwqp = qp->qp_uk.back_qp;
	entry->qp = qp->qp_uk.back_qp;

	if (cq_poll_info->error) {
		entry->status = (cq_poll_info->comp_status ==
				 ZXDH_COMPL_STATUS_FLUSHED) ?
					zxdh_flush_err_to_ib_wc_status(
						cq_poll_info->minor_err) :
					      IB_WC_GENERAL_ERR;

		entry->vendor_err = cq_poll_info->major_err << 16 |
				    cq_poll_info->minor_err;
	} else {
		entry->status = IB_WC_SUCCESS;
		if (cq_poll_info->imm_valid) {
			entry->ex.imm_data = htonl(cq_poll_info->imm_data);
			entry->wc_flags |= IB_WC_WITH_IMM;
		}
		if (cq_poll_info->ud_smac_valid) {
			ether_addr_copy(entry->smac, cq_poll_info->ud_smac);
			entry->wc_flags |= IB_WC_WITH_SMAC;
		}

		if (cq_poll_info->ud_vlan_valid &&
		    iwqp->iwdev->rf->vlan_parse_en) {

			if (iwqp->iwdev->vf_vlan.vlan_id && iwqp->iwdev->rf->ftype) {
				entry->sl = 0;
			} else {
				u16 vlan = cq_poll_info->ud_vlan & VLAN_VID_MASK;
				entry->sl = cq_poll_info->ud_vlan >> VLAN_PRIO_SHIFT;
				if (vlan) {
					entry->vlan_id = vlan;
					entry->wc_flags |= IB_WC_WITH_VLAN;
				}
			}
		} else {
			entry->sl = 0;
		}
	}

	switch (cq_poll_info->op_type) {
	case ZXDH_OP_TYPE_SEND:
	case ZXDH_OP_TYPE_SEND_WITH_IMM:
	case ZXDH_OP_TYPE_SEND_INV:
	case ZXDH_OP_TYPE_UD_SEND:
	case ZXDH_OP_TYPE_UD_SEND_WITH_IMM:
		entry->opcode = IB_WC_SEND;
		break;
	case ZXDH_OP_TYPE_WRITE:
	case ZXDH_OP_TYPE_WRITE_WITH_IMM:
		entry->opcode = IB_WC_RDMA_WRITE;
		break;
	case ZXDH_OP_TYPE_READ:
		entry->opcode = IB_WC_RDMA_READ;
		break;
	case ZXDH_OP_TYPE_FAST_REG_MR:
		entry->opcode = IB_WC_REG_MR;
		break;
	case ZXDH_OP_TYPE_LOCAL_INV:
		entry->opcode = IB_WC_LOCAL_INV;
		break;
	case ZXDH_OP_TYPE_REC_IMM:
	case ZXDH_OP_TYPE_REC:
		entry->opcode = cq_poll_info->op_type == ZXDH_OP_TYPE_REC_IMM ?
					IB_WC_RECV_RDMA_WITH_IMM :
					      IB_WC_RECV;
		if (qp->qp_uk.qp_type != ZXDH_QP_TYPE_ROCE_UD &&
		    cq_poll_info->stag_invalid_set) {
			entry->ex.invalidate_rkey = cq_poll_info->inv_stag;
			entry->wc_flags |= IB_WC_WITH_INVALIDATE;
		}
		break;
	default:
		pr_info("[zxdh_rdma] warnning: opcode = %d in CQE\n", cq_poll_info->op_type);
		entry->status = IB_WC_GENERAL_ERR;
		return;
	}

	if (qp->qp_uk.qp_type == ZXDH_QP_TYPE_ROCE_UD) {
		entry->src_qp = cq_poll_info->ud_src_qpn;
		entry->slid = 0;
		entry->wc_flags |= (IB_WC_GRH | IB_WC_WITH_NETWORK_HDR_TYPE);
		entry->network_hdr_type = cq_poll_info->ipv4 ?
						  RDMA_NETWORK_IPV4 :
							RDMA_NETWORK_IPV6;
	} else {
		entry->src_qp = cq_poll_info->qp_id;
	}

	entry->byte_len = cq_poll_info->bytes_xfered;
}

/**
 * zxdh_poll_one - poll one entry of the CQ
 * @ukcq: ukcq to poll
 * @cur_cqe: current CQE info to be filled in
 * @entry: ibv_wc object to be filled for non-extended CQ or NULL for extended CQ
 *
 * Returns the internal zrdma device error code or 0 on success
 */
static inline enum zxdh_status_code zxdh_poll_one(struct zxdh_cq_uk *ukcq,
				struct zxdh_cq_poll_info *cur_cqe,
				struct ib_wc *entry)
{
	enum zxdh_status_code ret = zxdh_uk_cq_poll_cmpl(ukcq, cur_cqe);

	if (ret != ZXDH_SUCCESS)
		return ret;

	zxdh_process_cqe(entry, cur_cqe);

	return ret;
}

/**
 * __zxdh_poll_cq - poll cq for completion (kernel apps)
 * @iwcq: cq to poll
 * @num_entries: number of entries to poll
 * @entry: wr of a completed entry
 */
static int __zxdh_poll_cq(struct zxdh_cq *iwcq, int num_entries,
			  struct ib_wc *entry)
{
	struct list_head *tmp_node, *list_node;
	struct zxdh_cq_buf *last_buf = NULL;
	struct zxdh_cq_poll_info *cur_cqe = &iwcq->cur_cqe;
	struct zxdh_cq_buf *cq_buf;
	int ret;
	struct zxdh_device *iwdev;
	struct zxdh_cq_uk *ukcq;
	bool cq_new_cqe = false;
	int resized_bufs = 0;
	int npolled = 0;

	iwdev = to_iwdev(iwcq->ibcq.device);
	ukcq = &iwcq->sc_cq.cq_uk;

	/* go through the list of previously resized CQ buffers */
	list_for_each_safe(list_node, tmp_node, &iwcq->resize_list) {
		cq_buf = container_of(list_node, struct zxdh_cq_buf, list);
		while (npolled < num_entries) {
			ret = zxdh_poll_one(&cq_buf->cq_uk, cur_cqe,
					    entry + npolled);
			if (ret == ZXDH_SUCCESS) {
				++npolled;
				cq_new_cqe = true;
				continue;
			}
			if (ret == ZXDH_ERR_Q_EMPTY)
				break;
			if (ret == ZXDH_ERR_RETRY_ACK_NOT_EXCEED_ERR)
				break;
			/* QP using the CQ is destroyed. Skip reporting this CQE */
			if (ret == ZXDH_ERR_Q_DESTROYED) {
				cq_new_cqe = true;
				continue;
			}
			pr_err("[zxdh_rdma] VERBS: %s[%d]: Error polling CQ, err_code: %d\n", __func__, __LINE__, ret);
			goto error;
		}

		/* save the resized CQ buffer which received the last cqe */
		if (cq_new_cqe)
			last_buf = cq_buf;
		cq_new_cqe = false;
	}

	/* check the current CQ for new cqes */
	while (npolled < num_entries) {
		ret = zxdh_poll_one(ukcq, cur_cqe, entry + npolled);
		if (ret == ZXDH_ERR_Q_EMPTY) {
			ret = zxdh_generated_cmpls(iwcq, cur_cqe);
			if (ret == ZXDH_SUCCESS)
				zxdh_process_cqe(entry + npolled, cur_cqe);
		}
		if (ret == ZXDH_SUCCESS) {
			++npolled;
			cq_new_cqe = true;
			continue;
		}

		if (ret == ZXDH_ERR_Q_EMPTY)
			break;
		if (ret == ZXDH_ERR_RETRY_ACK_NOT_EXCEED_ERR)
			break;
		/* QP using the CQ is destroyed. Skip reporting this CQE */
		if (ret == ZXDH_ERR_Q_DESTROYED) {
			cq_new_cqe = true;
			continue;
		}
		pr_err("[zxdh_rdma] VERBS: %s[%d]: Error polling CQ, err_code: %d\n", __func__, __LINE__, ret);
		goto error;
	}

	if (cq_new_cqe)
		/* all previous CQ resizes are complete */
		resized_bufs = zxdh_process_resize_list(iwcq, iwdev, NULL);
	else if (last_buf)
		/* only CQ resizes up to the last_buf are complete */
		resized_bufs = zxdh_process_resize_list(iwcq, iwdev, last_buf);
	if (resized_bufs)
		/* report to the HW the number of complete CQ resizes */
		zxdh_uk_cq_set_resized_cnt(ukcq, resized_bufs);

	return npolled;
error:
	return ret;
}

/**
 * zxdh_poll_cq - poll cq for completion (kernel apps)
 * @ibcq: cq to poll
 * @num_entries: number of entries to poll
 * @entry: wr of a completed entry
 */
static int zxdh_poll_cq(struct ib_cq *ibcq, int num_entries,
			struct ib_wc *entry)
{
	struct zxdh_cq *iwcq;
	unsigned long flags;
	int ret;

	iwcq = to_iwcq(ibcq);
	if ((iwcq == NULL) || (iwcq->sc_cq.cq_uk.valid_cq == false)) {
		return 0;
	}
	spin_lock_irqsave(&iwcq->lock, flags);
	ret = __zxdh_poll_cq(iwcq, num_entries, entry);
	spin_unlock_irqrestore(&iwcq->lock, flags);

	return ret;
}

/**
 * zxdh_req_notify_cq - arm cq kernel application
 * @ibcq: cq to arm
 * @notify_flags: notofication flags
 */
static int zxdh_req_notify_cq(struct ib_cq *ibcq,
			      enum ib_cq_notify_flags notify_flags)
{
	struct zxdh_cq *iwcq;
	struct zxdh_cq_uk *ukcq;
	unsigned long flags;
	enum zxdh_cmpl_notify cq_notify = ZXDH_CQ_COMPL_EVENT;
	bool promo_event = false;
	int ret = 0;

	iwcq = to_iwcq(ibcq);
	ukcq = &iwcq->sc_cq.cq_uk;

	spin_lock_irqsave(&iwcq->lock, flags);
	if (ukcq->valid_cq == false) {
		spin_unlock_irqrestore(&iwcq->lock, flags);
		return 0;
	}
	if (notify_flags == IB_CQ_SOLICITED) {
		cq_notify = ZXDH_CQ_COMPL_SOLICITED;
	} else {
		if (iwcq->last_notify == ZXDH_CQ_COMPL_SOLICITED)
			promo_event = true;
	}

	if (!iwcq->armed || promo_event) {
		iwcq->armed = true;
		iwcq->last_notify = cq_notify;
		zxdh_uk_cq_request_notification(ukcq, cq_notify);
	}

	if ((notify_flags & IB_CQ_REPORT_MISSED_EVENTS) && !zxdh_cq_empty(iwcq))
		ret = 1;
	spin_unlock_irqrestore(&iwcq->lock, flags);

	return ret;
}

#ifdef ALLOC_HW_STATS_STRUCT_V2
const struct rdma_stat_desc zxdh_hw_stat_descs[] = {
	/*32-bit */
	[HW_STAT_DUPLICATE_REQUEST].name = "duplicate_request",
	[HW_STAT_NP_CNP_SENT].name = "np_cnp_sent",
	[HW_STAT_NP_ECN_MARKED_ROCE_PACKETS].name =
		"np_ecn_marked_roce_packets",
	[HW_STAT_OUT_OF_SEQUENCE].name = "out_of_sequence",
	[HW_STAT_PACKET_SEQ_ERR].name = "packet_seq_err",
	[HW_STAT_REQ_CQE_ERROR].name = "req_cqe_error",
	[HW_STAT_REQ_REMOTE_ACCESS_ERRORS].name = "req_remote_access_errors",
	[HW_STAT_REQ_REMOTE_INVALID_REQUEST].name =
		"req_remote_invalid_request",
	[HW_STAT_REQ_REMOTE_OPERATION_ERRORS].name =
		"req_remote_operation_errors",
	[HW_STAT_REQ_LOCAL_LENGTH_ERROR].name = "req_local_length_error",
	[HW_STAT_RESP_CQE_ERROR].name = "resp_cqe_error",
	[HW_STAT_RESP_REMOTE_ACCESS_ERRORS].name = "resp_remote_access_errors",
	[HW_STAT_RESP_REMOTE_INVALID_REQUEST].name =
		"resp_remote_invalid_request",
	[HW_STAT_RESP_REMOTE_OPERATION_ERRORS].name =
		"resp_remote_operation_errors",
	[HW_STAT_RESP_RNR_NAK].name = "resp_rnr_nak",
	[HW_STAT_RNR_NAK_RETRY_ERR].name = "rnr_nak_retry_err",
	[HW_STAT_RP_CNP_HANDLED].name = "rp_cnp_handled",
	[HW_STAT_RX_READ_REQUESTS].name = "rx_read_requests",
	[HW_STAT_RX_WRITE_REQUESTS].name = "rx_write_requests",
	[HW_STAT_RX_ICRC_ENCAPSULATED].name = "rx_icrc_encapsulated",
	[HW_STAT_ROCE_SLOW_RESTART_CNPS].name = "roce_slow_restart_cnps",
	[HW_STAT_RDMA_TX_PKTS].name = "rdma_tx_pkts",
	[HW_STAT_RDMA_TX_BYTES].name = "rdma_tx_bytes",
	[HW_STAT_RDMA_RX_PKTS].name = "rdma_rx_pkts",
	[HW_STAT_RDMA_RX_BYTES].name = "rdma_rx_bytes",
};

#endif /* ALLOC_HW_STATS_STRUCT_V2 */

/**
 * zxdh_query_ah - Query address handle
 * @ibah: pointer to address handle
 * @ah_attr: address handle attributes
 */
static int zxdh_query_ah(struct ib_ah *ibah, struct rdma_ah_attr *ah_attr)
{
	struct zxdh_ah *ah = to_iwah(ibah);

	memset(ah_attr, 0, sizeof(*ah_attr));
	if (ah->av.attrs.ah_flags & IB_AH_GRH) {
		ah_attr->ah_flags = IB_AH_GRH;
		ah_attr->grh.flow_label = ah->sc_ah.ah_info.flow_label;
		ah_attr->grh.traffic_class = ah->sc_ah.ah_info.tc_tos;
		ah_attr->grh.hop_limit = ah->sc_ah.ah_info.hop_ttl;
		ah_attr->grh.sgid_index = ah->sgid_index;
		ah_attr->grh.sgid_index = ah->sgid_index;
		memcpy(&ah_attr->grh.dgid, &ah->dgid,
		       sizeof(ah_attr->grh.dgid));
	}

	return 0;
}

static __be64 zxdh_mac_to_guid(struct net_device *ndev)
{
	const unsigned char *mac = ndev->dev_addr;
	__be64 guid;
	unsigned char *dst = (unsigned char *)&guid;

	dst[0] = mac[0] ^ 2;
	dst[1] = mac[1];
	dst[2] = mac[2];
	dst[3] = 0xff;
	dst[4] = 0xfe;
	dst[5] = mac[3];
	dst[6] = mac[4];
	dst[7] = mac[5];

	return guid;
}

static ssize_t hca_type_show(struct device *device,
			     struct device_attribute *attr, char *buf)
{
	struct zxdh_device *iwdev =
		zxdh_rdma_device_to_drv_device(device, ibdev);

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
	return sysfs_emit(buf, "%d\n", iwdev->rf->pcidev->device);
#else
	return sprintf(buf, "%d\n", iwdev->rf->pcidev->device);
#endif
}

static ssize_t hw_rev_show(struct device *device, struct device_attribute *attr,
			   char *buf)
{
	struct zxdh_device *iwdev =
		zxdh_rdma_device_to_drv_device(device, ibdev);

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
	return sysfs_emit(buf, "%x\n", iwdev->rf->pcidev->revision);
#else
	return sprintf(buf, "%x\n", iwdev->rf->pcidev->revision);
#endif
}

#ifdef ZXDH_UAPI_DEF
static DEVICE_ATTR_RO(hca_type);
static DEVICE_ATTR_RO(hw_rev);

static struct attribute *zxdh_class_attributes[] = {
	&dev_attr_hw_rev.attr,
	&dev_attr_hca_type.attr,
	NULL,
};

static const struct attribute_group zxdh_attr_group = {
	.attrs = zxdh_class_attributes,
};

static inline void
zxdh_set_device_sysfs_group(struct ib_device *dev,
			    const struct attribute_group *group)
{
	dev->groups[1] = group;
}
#else
static DEVICE_ATTR(hw_rev, S_IRUGO, hw_rev_show, NULL);
static DEVICE_ATTR(hca_type, S_IRUGO, hca_type_show, NULL);

static struct device_attribute *zxdh_class_attributes[] = {
	&dev_attr_hw_rev,
	&dev_attr_hca_type,
};

static int zxdh_class_attr_init(struct zxdh_device *iwdev)
{
	int err;
	int i;

	for (i = 0; i < ARRAY_SIZE(zxdh_class_attributes); i++) {
		err = device_create_file(&iwdev->ibdev.dev, zxdh_class_attributes[i]);
		if (err) {
			while (i > 0) {
				i--;
				device_remove_file(&iwdev->ibdev.dev,
						   zxdh_class_attributes[i]);
			}
			return err;
		}
	}
	return 0;
}
#endif

static ssize_t zxdh_tc_traffic_class_show(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   char *buf)
{
	struct zxdh_tc_port *tc_port = container_of(kobj, struct zxdh_tc_port, kobj);
	u8 val;
	bool global_tclass;

	mutex_lock(&tc_port->lock);
	val = tc_port->traffic_class;
	global_tclass = tc_port->global_tclass;
	mutex_unlock(&tc_port->lock);

	if (!global_tclass) {
		return 0;
	}

#if (KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE)
	return sysfs_emit(buf, "Global tclass=%u\n", val);
#else
	return sprintf(buf, "Global tclass=%u\n", val);
#endif
}

static ssize_t zxdh_tc_traffic_class_store(struct kobject *kobj,
					    struct kobj_attribute *attr,
					    const char *buf, size_t count)
{
	struct zxdh_tc_port *tc_port = container_of(kobj, struct zxdh_tc_port, kobj);
	unsigned long val;
	char *endp;

	if (count == 0)
		return -EINVAL;

	while (isspace(*buf))
		buf++;

	mutex_lock(&tc_port->lock);

	if (*buf == '-') {
		tc_port->traffic_class = 0xFF;
		tc_port->global_tclass = false;
		mutex_unlock(&tc_port->lock);
		return count;
	}

	if (strncmp(buf, "tclass=", 7) == 0) {
		const char *value_buf = buf + 7;
		if (*value_buf == '-') {
			tc_port->traffic_class = 0xFF;
			tc_port->global_tclass = false;
			mutex_unlock(&tc_port->lock);
			return count;
		}
		val = simple_strtoul(value_buf, &endp, 10);
	} else {
		val = simple_strtoul(buf, &endp, 10);
	}

	while (isspace(*endp))
		endp++;

	if (*endp != '\0' || val > 255) {
		mutex_unlock(&tc_port->lock);
		return -EINVAL;
	}

	tc_port->traffic_class = (u8)val;
	tc_port->global_tclass = true;
	mutex_unlock(&tc_port->lock);

	return count;
}

/**
 * zxdh_get_port_traffic_class - 获取端口级traffic_class配置
 * @iwdev: RDMA设备
 * @port_num: 端口号
 * @traffic_class: 输出参数，返回traffic_class值（0xFF表示未配置）
 *
 * 返回: 0表示成功，-EINVAL表示参数无效
 */
static int zxdh_get_port_traffic_class(struct zxdh_device *iwdev,
				       u8 port_num, u8 *traffic_class)
{
	if (iwdev == NULL || traffic_class == NULL)
		return -EINVAL;

	*traffic_class = 0xFF;

	if (iwdev->tc_ports != NULL &&
	    iwdev->tc_ports[0].port_num == port_num) {
		mutex_lock(&iwdev->tc_ports[0].lock);
		if (iwdev->tc_ports[0].global_tclass) {
			*traffic_class = iwdev->tc_ports[0].traffic_class;
		}
		mutex_unlock(&iwdev->tc_ports[0].lock);
	}

	return 0;
}

static struct kobj_attribute traffic_class_attr =
	__ATTR(traffic_class, 0644,
	       zxdh_tc_traffic_class_show,
	       zxdh_tc_traffic_class_store);

static struct attribute *zxdh_tc_port_attrs[] = {
	&traffic_class_attr.attr,
	NULL,
};

#if (KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE)
static const struct attribute_group zxdh_tc_port_attr_group = {
	.attrs = zxdh_tc_port_attrs,
};

static const struct attribute_group *zxdh_tc_port_groups[] = {
	&zxdh_tc_port_attr_group,
	NULL,
};
#endif

static void zxdh_tc_port_release(struct kobject *kobj)
{
}

static void zxdh_tc_root_release(struct kobject *kobj)
{
}

static struct kobj_type zxdh_tc_root_ktype = {
	.release = zxdh_tc_root_release,
};

static struct kobj_type zxdh_tc_port_ktype = {
	.release = zxdh_tc_port_release,
	.sysfs_ops = &kobj_sysfs_ops,
#if (KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE)
	.default_groups = zxdh_tc_port_groups,
#else
	.default_attrs = zxdh_tc_port_attrs,
#endif
};

int zxdh_tc_sysfs_init(struct zxdh_device *iwdev)
{
	struct zxdh_tc_port *tc_port;
	char port_name[16];
	int ret;
	u8 port_num = 1;

	if (iwdev == NULL || iwdev->ibdev.dev.kobj.parent == NULL)
		return -EINVAL;

	kobject_init(&iwdev->tc_kobj, &zxdh_tc_root_ktype);
	ret = kobject_add(&iwdev->tc_kobj, &iwdev->ibdev.dev.kobj, "tc");
	if (ret) {
		pr_err("[zxdh_rdma] Failed to add tc kobject: %d\n", ret);
		kobject_put(&iwdev->tc_kobj);
		return ret;
	}

	iwdev->tc_ports = kzalloc(sizeof(struct zxdh_tc_port), GFP_KERNEL);
	if (iwdev->tc_ports == NULL) {
		ret = -ENOMEM;
		goto err_tc_kobj;
	}

	tc_port = &iwdev->tc_ports[0];
	tc_port->port_num = port_num;
	tc_port->traffic_class = 0xFF;
	tc_port->global_tclass = false;
	mutex_init(&tc_port->lock);

	ret = snprintf(port_name, sizeof(port_name), "%u", port_num);
	if (ret < 0 || ret >= sizeof(port_name)) {
		pr_err("[zxdh_rdma] Failed to format port name for port %u\n",
		       port_num);
		ret = -EINVAL;
		goto err_tc_ports;
	}
	ret = kobject_init_and_add(&tc_port->kobj, &zxdh_tc_port_ktype,
				   &iwdev->tc_kobj, port_name);
	if (ret) {
		pr_err("[zxdh_rdma] Failed to create port %u kobject: %d\n",
		       port_num, ret);
		goto err_tc_ports;
	}

	return 0;

err_tc_ports:
	kfree(iwdev->tc_ports);
	iwdev->tc_ports = NULL;
err_tc_kobj:
	kobject_put(&iwdev->tc_kobj);
	return ret;
}

void zxdh_tc_sysfs_cleanup(struct zxdh_device *iwdev)
{
	if (iwdev == NULL)
		return;

	if (iwdev->tc_ports != NULL) {
		kobject_put(&iwdev->tc_ports[0].kobj);
		kfree(iwdev->tc_ports);
		iwdev->tc_ports = NULL;
	}

	if (iwdev->tc_kobj.parent != NULL) {
		kobject_put(&iwdev->tc_kobj);
	}
}

#ifdef IB_GET_NETDEV_OP_NOT_DEPRECATED
static struct net_device *zxdh_get_netdev(struct ib_device *ibdev, u8 port_num)
{
	struct zxdh_device *iwdev = to_iwdev(ibdev);

	if (iwdev->netdev) {
		dev_hold(iwdev->netdev);
		return iwdev->netdev;
	}

	return NULL;
}

#endif
#ifdef HAS_IB_SET_DEVICE_OP
static struct ib_device_ops zxdh_roce_dev_ops;
static const struct ib_device_ops zxdh_dev_ops = {
#if KERNEL_VERSION(5, 3, 0) <= LINUX_VERSION_CODE || defined(RHEL_8_2) || \
	defined(RHEL_8_3) || defined(RHEL_8_4) || defined(RHEL_8_5) || defined(RHEL_8_6) || defined(KYLIN_V10_4) || defined(KYLIN_V11_0) || \
	((KERNEL_VERSION(4, 19, 90) == LINUX_VERSION_CODE) && (defined(__OFED_24_10__))) || \
	((KERNEL_VERSION(4, 19, 0) == LINUX_VERSION_CODE) && (defined(__OFED_5_8__)))
	.owner = THIS_MODULE,
	.driver_id = RDMA_DRIVER_ZXDH,
	.uverbs_abi_ver = ZXDH_ABI_VER,
#endif
#if defined(ALLOC_HW_STATS_V3)
	.alloc_hw_port_stats = zxdh_alloc_hw_port_stats,
#else
	.alloc_hw_stats = zxdh_alloc_hw_stats,
#endif
	.alloc_mr = zxdh_alloc_mr,
	.alloc_mw = zxdh_alloc_mw,
	.alloc_pd = zxdh_alloc_pd,
	.alloc_ucontext = zxdh_alloc_ucontext,
	.create_cq = zxdh_create_cq,
	.create_qp = zxdh_create_qp,
	.create_srq = zxdh_create_srq,
#ifdef IB_DEALLOC_DRIVER_SUPPORT
	.dealloc_driver = zxdh_ib_dealloc_device,
#endif
	.dealloc_mw = zxdh_dealloc_mw,
	.dealloc_pd = zxdh_dealloc_pd,
	.dealloc_ucontext = zxdh_dealloc_ucontext,
	.dereg_mr = zxdh_dereg_mr,
	.destroy_cq = zxdh_destroy_cq,
	.destroy_qp = zxdh_destroy_qp,
	.destroy_srq = zxdh_destroy_srq,
	.disassociate_ucontext = zxdh_disassociate_ucontext,
	.get_dev_fw_str = zxdh_get_dev_fw_str,
	.get_dma_mr = zxdh_get_dma_mr,
	.get_hw_stats = zxdh_get_hw_stats,
#ifdef IB_GET_NETDEV_OP_NOT_DEPRECATED
	.get_netdev = zxdh_get_netdev,
#endif
	.map_mr_sg = zxdh_map_mr_sg,
	.mmap = zxdh_mmap,
#ifdef RDMA_MMAP_DB_SUPPORT
	.mmap_free = zxdh_mmap_free,
#endif
	.poll_cq = zxdh_poll_cq,
	.post_recv = zxdh_post_recv,
	.post_send = zxdh_post_send,
	.post_srq_recv = zxdh_post_srq_recv,
	.process_mad = zxdh_process_mad,
	.query_device = zxdh_query_device,
	.query_port = zxdh_query_port,
	.modify_port = zxdh_modify_port,
	.query_qp = zxdh_query_qp,
	.query_srq = zxdh_query_srq,
	.reg_user_mr = zxdh_reg_user_mr,
#ifdef SET_DMABUF
	.reg_user_mr_dmabuf = zxdh_reg_user_mr_dmabuf,
#endif
	.rereg_user_mr = zxdh_rereg_user_mr,
	.req_notify_cq = zxdh_req_notify_cq,
	.resize_cq = zxdh_resize_cq,
	.modify_srq = zxdh_modify_srq,
	.modify_cq = zxdh_modify_cq,
#if KERNEL_VERSION(5, 14, 0) <= LINUX_VERSION_CODE || defined OFED_USE_DEVICE_GROUP
	.device_group = &zxdh_attr_group,
#endif
#ifdef INIT_RDMA_OBJ_SIZE
	INIT_RDMA_OBJ_SIZE(ib_pd, zxdh_pd, ibpd),
	INIT_RDMA_OBJ_SIZE(ib_ucontext, zxdh_ucontext, ibucontext),
	INIT_RDMA_OBJ_SIZE(ib_srq, zxdh_srq, ibsrq),
#if KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE || defined(RHEL_8_2) || \
	defined(RHEL_8_3) || defined(RHEL_8_4) || defined(RHEL_8_5) || defined(RHEL_8_6) || defined(KYLIN_V10_4) || \
	((KERNEL_VERSION(4, 19, 90) == LINUX_VERSION_CODE) && (defined(__OFED_24_10__))) || \
	((KERNEL_VERSION(4, 19, 0) == LINUX_VERSION_CODE) && (defined(__OFED_5_8__)))
	INIT_RDMA_OBJ_SIZE(ib_ah, zxdh_ah, ibah),
#endif /* 5.2.0 */
#if KERNEL_VERSION(5, 3, 0) <= LINUX_VERSION_CODE || defined(RHEL_8_2) || \
	defined(RHEL_8_3) || defined(RHEL_8_4) || defined(RHEL_8_5) || defined(RHEL_8_6) || defined(KYLIN_V10_4) || \
	((KERNEL_VERSION(4, 19, 90) == LINUX_VERSION_CODE) && (defined(__OFED_24_10__))) || \
	((KERNEL_VERSION(4, 19, 0) == LINUX_VERSION_CODE) && (defined(__OFED_5_8__)))
	INIT_RDMA_OBJ_SIZE(ib_cq, zxdh_cq, ibcq),
#endif /* 5.3.0 */
#if KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE || defined(KYLIN_V10_4) || \
	((KERNEL_VERSION(4, 19, 90) == LINUX_VERSION_CODE) && (defined(__OFED_24_10__))) || \
	((KERNEL_VERSION(4, 19, 0) == LINUX_VERSION_CODE) && (defined(__OFED_5_8__)))
	INIT_RDMA_OBJ_SIZE(ib_mw, zxdh_mr, ibmw),
#endif /* 5.10.0 */
#ifdef GLOBAL_QP_MEM
	INIT_RDMA_OBJ_SIZE(ib_qp, zxdh_qp, ibqp),
#endif /* GLOBAL_QP_MEM */
#endif /* INIT_RDMA_OBJ_SIZE */
};

#endif /* HAS_IB_SET_DEVICE_OP */
static void zxdh_set_device_ops(struct ib_device *ibdev)
{
#ifndef HAS_IB_SET_DEVICE_OP
	struct ib_device *dev_ops = ibdev;

#if defined(RHEL_7_7) || defined(RHEL_7_8) || defined(RHEL_7_9) ||     \
	defined(RHEL_8_2) || defined(RHEL_8_3) || defined(RHEL_8_4) || \
	defined(RHEL_8_5) || defined(RHEL_8_6) || defined(KYLIN_V10_4)
	dev_ops->uverbs_abi_ver = ZXDH_ABI_VER;
	dev_ops->driver_id = RDMA_DRIVER_ZXDH;
	dev_ops->owner = THIS_MODULE;
#endif
	dev_ops->alloc_hw_stats = zxdh_alloc_hw_stats;
	dev_ops->alloc_mr = zxdh_alloc_mr;
	dev_ops->alloc_mw = zxdh_alloc_mw;
	dev_ops->alloc_pd = zxdh_alloc_pd;
	dev_ops->alloc_ucontext = zxdh_alloc_ucontext;
	dev_ops->create_cq = zxdh_create_cq;
	dev_ops->create_qp = zxdh_create_qp;
	dev_ops->create_srq = zxdh_create_srq;
#ifdef IB_DEALLOC_DRIVER_SUPPORT
	dev_ops->dealloc_driver = zxdh_ib_dealloc_device,
#endif
	dev_ops->dealloc_mw = zxdh_dealloc_mw;
	dev_ops->dealloc_pd = zxdh_dealloc_pd;
	dev_ops->dealloc_ucontext = zxdh_dealloc_ucontext;
	dev_ops->dereg_mr = zxdh_dereg_mr;
	dev_ops->destroy_cq = zxdh_destroy_cq;
	dev_ops->destroy_qp = zxdh_destroy_qp;
	dev_ops->destroy_srq = zxdh_destroy_srq;
	dev_ops->disassociate_ucontext = zxdh_disassociate_ucontext;
	dev_ops->get_dev_fw_str = zxdh_get_dev_fw_str;
	dev_ops->get_dma_mr = zxdh_get_dma_mr;
	dev_ops->get_hw_stats = zxdh_get_hw_stats;
#ifndef HAS_IB_SET_DEVICE_OP
	dev_ops->get_netdev = zxdh_get_netdev;
#endif
	dev_ops->map_mr_sg = zxdh_map_mr_sg;
	dev_ops->mmap = zxdh_mmap;
#ifdef RDMA_MMAP_DB_SUPPORT
	dev_ops->mmap_free = zxdh_mmap_free;
#endif
	dev_ops->poll_cq = zxdh_poll_cq;
	dev_ops->post_recv = zxdh_post_recv;
	dev_ops->post_send = zxdh_post_send;
	dev_ops->post_srq_recv = zxdh_post_srq_recv;
	dev_ops->process_mad = zxdh_process_mad;
	dev_ops->query_device = zxdh_query_device;
	dev_ops->query_port = zxdh_query_port;
	dev_ops->modify_port = zxdh_modify_port;
	dev_ops->query_qp = zxdh_query_qp;
	dev_ops->query_srq = zxdh_query_srq;
	dev_ops->reg_user_mr = zxdh_reg_user_mr;
	dev_ops->rereg_user_mr = zxdh_rereg_user_mr;
	dev_ops->req_notify_cq = zxdh_req_notify_cq;
	dev_ops->resize_cq = zxdh_resize_cq;
	dev_ops->modify_cq = zxdh_modify_cq;
#else
	ib_set_device_ops(ibdev, &zxdh_dev_ops);
#endif
#ifdef ZXDH_UAPI_DEF
	zxdh_set_device_sysfs_group(ibdev, &zxdh_attr_group);
#endif
}

static void zxdh_set_device_roce_ops(struct ib_device *ibdev)
{
#ifdef HAS_IB_SET_DEVICE_OP
	struct ib_device_ops *dev_ops = &zxdh_roce_dev_ops;
#else
	struct ib_device *dev_ops = ibdev;
#endif
	dev_ops->create_ah = zxdh_create_ah;
#if KERNEL_VERSION(5, 11, 0) <= LINUX_VERSION_CODE || defined(RHEL_8_5) || defined(RHEL_8_6) || \
	defined(CREATE_USER_AH) || defined(KYLIN_V10_4)
	dev_ops->create_user_ah = zxdh_create_ah;
#endif
	dev_ops->destroy_ah = zxdh_destroy_ah;
	dev_ops->get_link_layer = zxdh_get_link_layer;
	dev_ops->get_port_immutable = zxdh_roce_port_immutable;
	dev_ops->modify_qp = zxdh_modify_qp_roce;
	dev_ops->modify_srq = zxdh_modify_srq;
	dev_ops->query_ah = zxdh_query_ah;
	dev_ops->query_gid = zxdh_query_gid_roce;
	dev_ops->query_pkey = zxdh_query_pkey;
	kc_set_ibdev_add_del_gid(ibdev);
#ifdef HAS_IB_SET_DEVICE_OP
	ib_set_device_ops(ibdev, &zxdh_roce_dev_ops);
#endif
}

/**
 * zxdh_init_roce_device - initialization of roce rdma device
 * @iwdev: zrdma device
 */
static void zxdh_init_roce_device(struct zxdh_device *iwdev)
{
#ifdef UVERBS_CMD_MASK
	kc_set_roce_uverbs_cmd_mask(iwdev);
#endif
	iwdev->ibdev.node_type = RDMA_NODE_IB_CA;
	iwdev->ibdev.node_guid = zxdh_mac_to_guid(iwdev->netdev);
	zxdh_set_device_roce_ops(&iwdev->ibdev);
}

#ifdef ZXDH_UAPI_DEF
static const struct uapi_definition zxdh_ib_defs[] = {
	UAPI_DEF_CHAIN(zxdh_ib_dev_defs),
	{}
};
#endif

/**
 * zxdh_init_rdma_device - initialization of rdma device
 * @iwdev: zrdma device
 */
static int zxdh_init_rdma_device(struct zxdh_device *iwdev)
{
	struct pci_dev *pcidev = iwdev->rf->pcidev;

#if KERNEL_VERSION(5, 3, 0) > LINUX_VERSION_CODE && !defined(RHEL_8_2) && \
	!defined(RHEL_8_3) && !defined(RHEL_8_4) && !defined(RHEL_8_5) && !defined(RHEL_8_6) && !defined(KYLIN_V10_4) && \
	!((KERNEL_VERSION(4, 19, 90) == LINUX_VERSION_CODE) && (defined(__OFED_24_10__))) && \
	!((KERNEL_VERSION(4, 19, 0) == LINUX_VERSION_CODE) && (defined(__OFED_5_8__)))
	iwdev->ibdev.owner = THIS_MODULE;
	iwdev->ibdev.uverbs_abi_ver = ZXDH_ABI_VER;
#endif
#ifdef UVERBS_CMD_MASK
	kc_set_rdma_uverbs_cmd_mask(iwdev);
#endif

	if (iwdev->roce_mode)
		zxdh_init_roce_device(iwdev);
	else
		return -EPFNOSUPPORT;

	iwdev->ibdev.phys_port_cnt = 1;
	iwdev->ibdev.num_comp_vectors = iwdev->rf->ceqs_count;
	iwdev->ibdev.dev.parent = &pcidev->dev;
	set_ibdev_dma_device(iwdev->ibdev, &pcidev->dev);
	zxdh_set_device_ops(&iwdev->ibdev);

#ifdef IB_DEV_OPS_FILL_ENTRY
	zxdh_set_restrack_ops(&iwdev->ibdev);
#endif /* >= 5.9.0 */
#ifdef ZXDH_UAPI_DEF
	iwdev->ibdev.driver_def = zxdh_ib_defs;
#else
	zxdh_get_dri_specs(iwdev);
#endif
	return 0;
}

#ifndef ZXDH_UAPI_DEF
int zxdh_get_dri_specs(struct zxdh_device *iwdev)
{
	const struct uverbs_object_tree_def **trees = iwdev->driver_trees;

	trees[0] = zxdh_ib_get_devx_tree();
	iwdev->ibdev.driver_specs = trees;

	return 0;
}
#endif

/**
 * zxdh_port_ibevent - indicate port event
 * @iwdev: zrdma device
 */
void zxdh_port_ibevent(struct zxdh_device *iwdev)
{
	struct ib_event event;

	event.device = &iwdev->ibdev;
	event.element.port_num = 1;
	event.event = iwdev->iw_status ? IB_EVENT_PORT_ACTIVE :
					       IB_EVENT_PORT_ERR;
	ib_dispatch_event(&event);
}

/**
 * zxdh_ib_unregister_device - unregister rdma device from IB
 * core
 * @iwdev: zrdma device
 */
void zxdh_ib_unregister_device(struct zxdh_device *iwdev)
{
	iwdev->iw_status = 0;
	zxdh_port_ibevent(iwdev);
	zxdh_tc_sysfs_cleanup(iwdev);
	ib_unregister_device(&iwdev->ibdev);
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE && !defined(RHEL_8_2) && \
	!defined(RHEL_8_3) && !defined(RHEL_8_4) && !defined(RHEL_8_5) && !defined(RHEL_8_6) && !defined(KYLIN_V10_4) && \
	!((KERNEL_VERSION(4, 19, 90) == LINUX_VERSION_CODE) && (defined(__OFED_24_10__))) && \
	!((KERNEL_VERSION(4, 19, 0) == LINUX_VERSION_CODE) && (defined(__OFED_5_8__)))
	kfree(iwdev->ibdev.iwcm);
	iwdev->ibdev.iwcm = NULL;
#endif
}

/**
 * zxdh_ib_register_device - register zrdma device to IB core
 * @iwdev: zrdma device
 */
int zxdh_ib_register_device(struct zxdh_device *iwdev)
{
	int ret;
	char name[IB_DEVICE_NAME_MAX] = {0};
	int err;

	ret = zxdh_init_rdma_device(iwdev);
	if (ret)
		return ret;

	ret = zxdh_get_del_rdma_name(pci_name(iwdev->rf->pcidev), name);
	if (ret) {
		zte_snprintf_s(name, IB_DEVICE_NAME_MAX, "%s", "zrdma%d");
	}

	kc_set_driver_id(iwdev->ibdev);
#ifdef NETDEV_TO_IBDEV_SUPPORT
	ret = ib_device_set_netdev(&iwdev->ibdev, iwdev->netdev, 1);
	if (ret)
		goto error;
#endif
#if KERNEL_VERSION(4, 20, 0) > LINUX_VERSION_CODE
#ifdef CONFIG_SUSE_KERNEL
#if SLE_VERSION(15, 0, 0) >= SLE_VERSION_CODE
	strlcpy(iwdev->ibdev.name, name, IB_DEVICE_NAME_MAX);
#endif /* SLE_VERSION_CODE */
#else
	strlcpy(iwdev->ibdev.name, name, IB_DEVICE_NAME_MAX);
#endif /* CONFIG_SUSE_KERNEL */
#endif /* LINUX_VERSION_CODE */
#if KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE
	dma_set_max_seg_size(iwdev->rf->hw.device, SZ_1G);
#endif
	ret = kc_ib_register_device(&iwdev->ibdev, name,
				    iwdev->rf->hw.device);
	if (ret) {
		zte_snprintf_s(name, IB_DEVICE_NAME_MAX, "%s", "zrdma%d");
#if KERNEL_VERSION(4, 20, 0) > LINUX_VERSION_CODE
#ifdef CONFIG_SUSE_KERNEL
#if SLE_VERSION(15, 0, 0) >= SLE_VERSION_CODE
	strlcpy(iwdev->ibdev.name, name, IB_DEVICE_NAME_MAX);
#endif /* SLE_VERSION_CODE */
#else
	strlcpy(iwdev->ibdev.name, name, IB_DEVICE_NAME_MAX);
#endif /* CONFIG_SUSE_KERNEL */
#endif /* LINUX_VERSION_CODE */
        err = kc_ib_register_device(&iwdev->ibdev, name,
				    iwdev->rf->hw.device);
        if (err)
            goto error;
        else 
            pr_debug("[zxdh_rdma] %s[%d] rdma can accept net event!\n", __func__, __LINE__);
	}
	else {
		pr_debug("[zxdh_rdma] %s[%d] rdma can accept net event!\n", __func__, __LINE__);
	}

	iwdev->iw_status = 1;
	zxdh_port_ibevent(iwdev);

#ifndef ZXDH_UAPI_DEF
	ret = zxdh_class_attr_init(iwdev);
	if (ret)
		goto error;
#endif
	ret = zxdh_tc_sysfs_init(iwdev);
	if (ret)
		goto error_tc;
	return 0;

error_tc:
#ifndef ZXDH_UAPI_DEF
	{
		int i;
		for (i = 0; i < ARRAY_SIZE(zxdh_class_attributes); i++) {
			device_remove_file(&iwdev->ibdev.dev, zxdh_class_attributes[i]);
		}
	}
#endif
	goto error;
error:
#if KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE && !defined(RHEL_8_2) && \
	!defined(RHEL_8_3) && !defined(RHEL_8_4) && !defined(RHEL_8_5)  && !defined(RHEL_8_6) && !defined(KYLIN_V10_4) && \
	!((KERNEL_VERSION(4, 19, 90) == LINUX_VERSION_CODE) && (defined(__OFED_24_10__))) && \
	!((KERNEL_VERSION(4, 19, 0) == LINUX_VERSION_CODE) && (defined(__OFED_5_8__)))
	kfree(iwdev->ibdev.iwcm);
	iwdev->ibdev.iwcm = NULL;
#endif
	if (ret)
		pr_err("[zxdh_rdma] VERBS: Register RDMA device fail\n");

	return ret;
}

#ifdef IB_DEALLOC_DRIVER_SUPPORT
/**
 * zxdh_ib_dealloc_device
 * @ibdev: ib device
 *
 * callback from ibdev dealloc_driver to deallocate resources
 * unber zrdma device
 */
void zxdh_ib_dealloc_device(struct ib_device *ibdev)
{
	struct zxdh_device *iwdev = to_iwdev(ibdev);
    zxdh_destory_eth_info_hlist(iwdev);
	/* 反初始化ETS动态令牌管理器 */
	zxdh_ets_token_mgr_deinit(iwdev->rf);
	zxdh_destroy_dip_info_hlist(iwdev->rf);

	if (!iwdev->rf->ftype)
	{
		zxdh_delete_rdma_pf_glb(iwdev->rf);
	}

	zxdh_rt_deinit_hw(iwdev);
	zxdh_ctrl_deinit_hw(iwdev->rf);
	zxdh_del_handler(iwdev->hdl);
	if (iwdev->rf->iw_msixtbl) {
		kfree(iwdev->rf->iw_msixtbl);
		iwdev->rf->iw_msixtbl = NULL;
	}
	if (iwdev->rf->use_ext_mem_flag) {
		zxdh_destroy_ext_mem_pool(iwdev->rf);
		
		zxdh_cleanup_mem_info(iwdev->rf);
	}
	kfree(iwdev->hdl);
	iwdev->hdl = NULL;
}
#endif

/**
 * flush_cache0_all_space
 * @ext_dev: pointer to the device structure
 */
void flush_cache0_all_space(struct zxdh_sc_dev *ext_dev)
{
	writel(CACHE_DEFAULT_VALID_WAY_NUM,
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_RDMA_CACHE_CCUCMWVR_OFFSET(ext_dev->ep_id)));

    /**
     * Insert the wmb() write memory barrier instruction to solve the disorder of write operations and ensure
     * that the register write operations in front of the barrier are executed first.
     */
    wmb();

    /**
     * Flush the valid entries;
     * Evict the dirty lines, and make them invalid.
     */
	writel(CACHE_FLUSH_VALID_ENTRY_MASK,
	       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_RDMA_CACHE_CCUCMCR_OFFSET(ext_dev->ep_id)));
}
