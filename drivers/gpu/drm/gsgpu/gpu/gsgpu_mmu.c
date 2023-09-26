#include <linux/firmware.h>
#include <drm/drm_cache.h>
#include "gsgpu.h"
#include "gsgpu_mmu.h"
#include "gsgpu_irq.h"

#define GSGPU_NUM_OF_VMIDS			4

static void mmu_set_gmc_funcs(struct gsgpu_device *adev);
static void mmu_set_irq_funcs(struct gsgpu_device *adev);

/**
 * mmu_vram_gtt_location  gmc locations ram info set
 *
 * @adev pionter of struct gsgpu_device
 * @mc  pionter of gsgpu_gmc
 */
static void mmu_vram_gtt_location(struct gsgpu_device *adev,
				       struct gsgpu_gmc *mc)
{
	u64 base = 0;

	/*TODO Use generic register to get vram base address*/

	/*refrence from LS7A2000 page data*/
	if (adev->chip == dev_7a2000)
		base = 0x1000000000000;
	else if (adev->chip == dev_2k2000)
		base = adev->gmc.aper_base;

	gsgpu_device_vram_location(adev, &adev->gmc, base);
	gsgpu_device_gart_location(adev, mc);
}

/**
 * mmu_mc_init - initialize the memory controller driver params
 *
 * @adev: gsgpu_device pointer
 *
 * Look up the amount of vram, vram width, and decide how to place
 * vram and gart within the GPU's physical address space ().
 * Returns 0 for success.
 */
static int mmu_mc_init(struct gsgpu_device *adev)
{
	adev->gmc.vram_width = 32;

	adev->gmc.mc_mask = 0xffffffffffULL; /* 40 bit MC */

	/* size in MB on gpu ram*/
	adev->gmc.mc_vram_size = 256 * 1024ULL * 1024ULL;
	adev->gmc.real_vram_size = 256 * 1024ULL * 1024ULL;

	/*TODO 	pci_resource_start(adev->pdev, 0)
	 * 		pci_resource_len(adev->pdev, 0)
	 * */

	if (gsgpu_using_ram) {
		adev->gmc.aper_base =  0x460000000;
		adev->gmc.aper_size = 0x10000000;
	} else {
		adev->gmc.aper_base = pci_resource_start(adev->pdev, 2);
		adev->gmc.aper_size = pci_resource_len(adev->pdev, 2);
	}

	DRM_INFO("aper_base %#llx SIZE %#llx bytes \n", adev->gmc.aper_base, adev->gmc.aper_size);
	/* In case the PCI BAR is larger than the actual amount of vram */
	adev->gmc.visible_vram_size = adev->gmc.aper_size;
	if (adev->gmc.visible_vram_size > adev->gmc.real_vram_size)
		adev->gmc.visible_vram_size = adev->gmc.real_vram_size;

	/* set the gart size */
	if (gsgpu_gart_size == -1) {
		adev->gmc.gart_size = 256ULL << 20;
	/* base = 0x1000000000000; */
	} else {
		adev->gmc.gart_size = (u64)gsgpu_gart_size << 20;
	}

	adev->gmc.vm_fault_info = kmalloc(sizeof(struct gsgpu_vm_fault_info), GFP_KERNEL);

	if (!adev->gmc.vm_fault_info)
		return -ENOMEM;

	return 0;
}

/*
 * GART
 * VMID 0 is the physical GPU addresses as used by the kernel.
 * VMIDs 1-15 are used for userspace clients and are handled
 * by the gsgpu vm/hsa code.
 */

/**
 * mmu_flush_gpu_tlb - gart tlb flush callback
 *
 * @adev: gsgpu_device pointer
 * @vmid: vm instance to flush
 *
 * Flush the TLB for the requested page table ().
 */
static void mmu_flush_gpu_tlb(struct gsgpu_device *adev,
					uint32_t vmid)
{
	gsgpu_cmd_exec(adev, GSCMD(GSCMD_MMU, MMU_FLUSH), GSGPU_MMU_FLUSH_PKT(vmid, GSGPU_MMU_FLUSH_VMID), 0);
}

static uint64_t mmu_emit_flush_gpu_tlb(struct gsgpu_ring *ring,
					    unsigned vmid, uint64_t pd_addr)
{
	uint32_t reg;
	reg = GSGPU_MMU_VMID_OF_PGD(vmid);

	gsgpu_ring_emit_wreg(ring, reg, lower_32_bits(pd_addr));
	gsgpu_ring_emit_wreg(ring, reg + 4, upper_32_bits(pd_addr));

	gsgpu_ring_emit_wreg(ring, GSGPU_MMU_FLUSH_CTRL_OFFSET, GSGPU_MMU_FLUSH_PKT(vmid, GSGPU_MMU_FLUSH_VMID));

	return pd_addr;
}

/**
 * mmu_set_pte_pde - update the page tables using MMIO
 *
 * @adev: gsgpu_device pointer
 * @cpu_pt_addr: cpu address of the page table
 * @gpu_page_idx: entry in the page table to update
 * @addr: dst addr to write into pte/pde
 * @flags: access flags
 *
 * Update the page tables using the CPU.
 */
static int mmu_set_pte_pde(struct gsgpu_device *adev, void *cpu_pt_addr,
				uint32_t gpu_page_idx, uint64_t addr,
				uint64_t flags)
{
	void __iomem *ptr = (void *)cpu_pt_addr;
	uint64_t value;

	/*
	 * PTE format:
	 * 63:40 reserved
	 * 39:12 physical page base address
	 * 7:4 zinf
	 * 3 writeable
	 * 2 exception
	 * 1 hugepage
	 * 0 present
	 *
	 */
	//value = addr & adev->vm_manager.dir_mask;
	//value = addr & 0x000000FFFFFFF000ULL;
	value = addr & 0xFFFFFFFFFFFFF000ULL;
	value |= flags;

	writeq(value, ptr + (gpu_page_idx * GSGPU_MMU_PTE_SIZE));

	return 0;
}

static uint64_t mmu_get_vm_pte_flags(struct gsgpu_device *adev,
					  uint32_t flags)
{
	uint64_t pte_flag = 0;

	if (flags & GSGPU_VM_PAGE_READABLE)
		pte_flag |= GSGPU_PTE_PRESENT;
	if (flags & GSGPU_VM_PAGE_WRITEABLE)
		pte_flag |= GSGPU_PTE_WRITEABLE;

	return pte_flag;
}

static void mmu_get_vm_pde(struct gsgpu_device *adev, int level,
				uint64_t *addr, uint64_t *flags)
{
	//BUG_ON(*addr & ~adev->vm_manager.dir_mask);
	//BUG_ON(*addr & 0xFFFFFF0000000FFFULL);
}

/**
 * mmu_gart_enable - gart enable
 *
 * @adev: gsgpu_device pointer
 *
 * This sets up the TLBs, programs the page tables for VMID0,
 * sets up the hw for VMIDs 1-15 which are allocated on
 * demand, and sets up the global locations for the LDS, GDS,
 * and GPUVM for FSA64 clients ().
 * Returns 0 for success, errors for failure.
 */
static int mmu_gart_enable(struct gsgpu_device *adev)
{
	int r, i;

	if (adev->gart.robj == NULL) {
		dev_err(adev->dev, "No VRAM object for PCIE GART.\n");
		return -EINVAL;
	}
	r = gsgpu_gart_table_vram_pin(adev);
	if (r)
		return r;

	gsgpu_cmd_exec(adev, GSCMD(GSCMD_MMU, MMU_SET_EXC), 3, 0);
	gsgpu_cmd_exec(adev, GSCMDi(GSCMD_MMU, MMU_SET_PGD, 0), \
			lower_32_bits(adev->gart.table_addr), upper_32_bits(adev->gart.table_addr));

	gsgpu_cmd_exec(adev, GSCMD(GSCMD_MMU, MMU_SET_SAFE), \
			lower_32_bits(adev->dummy_page_addr), upper_32_bits(adev->dummy_page_addr));

	gsgpu_cmd_exec(adev, GSCMDi(GSCMD_MMU, MMU_SET_DIR, 0),
			GSGPU_MMU_DIR_CTRL_256M_1LVL, 0);

	for (i = 1; i < GSGPU_NUM_OF_VMIDS; i++) {
		gsgpu_cmd_exec(adev, GSCMDi(GSCMD_MMU, MMU_SET_DIR, i),
				GSGPU_MMU_DIR_CTRL_1T_3LVL, 0);
	}

	gsgpu_cmd_exec(adev, GSCMD(GSCMD_MMU, MMU_ENABLE), MMU_ENABLE, ~1);

	gsgpu_cmd_exec(adev, GSCMD(GSCMD_MMU, MMU_FLUSH), GSGPU_MMU_FLUSH_PKT(0, GSGPU_MMU_FLUSH_ALL), 0);

	DRM_INFO("PCIE GART of %uM enabled (table at 0x%016llX).\n",
		 (unsigned)(adev->gmc.gart_size >> 20),
		 (unsigned long long)adev->gart.table_addr);
	adev->gart.ready = true;
	return 0;
}

static int mmu_gart_init(struct gsgpu_device *adev)
{
	int r;

	if (adev->gart.robj) {
		WARN(1, "GSGPU PCIE GART already initialized\n");
		return 0;
	}
	/* Initialize common gart structure */
	r = gsgpu_gart_init(adev);
	if (r)
		return r;
	adev->gart.table_size = adev->gart.num_gpu_pages * GSGPU_MMU_PTE_SIZE;
	adev->gart.gart_pte_flags = 0;
	return gsgpu_gart_table_vram_alloc(adev);
}

/**
 * mmu_gart_disable - gart disable
 *
 * @adev: gsgpu_device pointer
 *
 * This disables all VM page table ().
 */
static void mmu_gart_disable(struct gsgpu_device *adev)
{
	gsgpu_gart_table_vram_unpin(adev);
}

static int mmu_early_init(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	mmu_set_gmc_funcs(adev);
	mmu_set_irq_funcs(adev);

	spin_lock_init(&adev->gmc.invalidate_lock);

	return 0;
}

static int mmu_late_init(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	gsgpu_bo_late_init(adev);

	if (gsgpu_vm_fault_stop != GSGPU_VM_FAULT_STOP_ALWAYS)
		return gsgpu_irq_get(adev, &adev->gmc.vm_fault, 0);
	else
		return 0;
}

static inline int mmu_irq_set(struct gsgpu_device *adev)
{
	int r;

	r = gsgpu_irq_add_id(adev, GSGPU_IH_CLIENTID_LEGACY, GSGPU_SRCID_GFX_PAGE_INV_FAULT, &adev->gmc.vm_fault);
	if (r)
		return r;

	r = gsgpu_irq_add_id(adev, GSGPU_IH_CLIENTID_LEGACY, GSGPU_SRCID_GFX_MEM_PROT_FAULT, &adev->gmc.vm_fault);
	if (r)
		return r;

	return 0;
}

static inline void mmu_set_dma_consistent(struct gsgpu_device *adev)
{
	int r;
	int dma_bits;

	/* set DMA mask + need_dma32 flags.
	 * PCIE - can handle 40-bits.
	 * PCI - dma32 for legacy pci gart, 40 bits on newer asics
	 */
	adev->need_dma32 = false;
	dma_bits = 40;
	r = pci_set_dma_mask(adev->pdev, DMA_BIT_MASK(dma_bits));
	if (r) {
		adev->need_dma32 = true;
		dma_bits = 32;
		pr_warn("gsgpu: No suitable DMA available\n");
	}
	r = pci_set_consistent_dma_mask(adev->pdev, DMA_BIT_MASK(dma_bits));
	if (r) {
		pci_set_consistent_dma_mask(adev->pdev, DMA_BIT_MASK(32));
		pr_warn("gsgpu: No coherent DMA available\n");
	}

	adev->gmc.dma_bits = dma_bits;
}

static inline int mmu_vm_manager_init(struct gsgpu_device *adev)
{

	/* Adjust VM size here.
	 * Currently set to 4GB ((1 << 20) 4k pages).
	 * Max GPUVM size for cayman and SI is 40 bits.
	 */
	gsgpu_vm_adjust_size(adev, 64, 2, 40);

	/*
	 * number of VMs
	 * VMID 0 is reserved for System
	 * gsgpu graphics/compute will use VMIDs 1-7
	 */
	adev->vm_manager.id_mgr.num_ids = GSGPU_NUM_OF_VMIDS;
	gsgpu_vm_manager_init(adev);

	/* base offset of vram pages */
	if (gsgpu_using_ram)
		adev->vm_manager.vram_base_offset = adev->gmc.aper_base;
	else
		adev->vm_manager.vram_base_offset = 0x1000000000000;

	return 0;
}

static int mmu_sw_init(void *handle)
{
	int r;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	r = mmu_irq_set(adev);
	if (r)
		return r;

	mmu_set_dma_consistent(adev);

	adev->need_swiotlb = drm_get_max_iomem() > (u64)BIT(adev->gmc.dma_bits);

	r = mmu_mc_init(adev);
	if (r)
		return r;

	mmu_vram_gtt_location(adev, &adev->gmc);

	/* Initialize the TTM memory manager */
	r = gsgpu_bo_init(adev);
	if (r)
		return r;

	r = mmu_gart_init(adev);
	if (r)
		return r;

	r = mmu_vm_manager_init(adev);
	if (r)
		return r;

	return 0;
}

static int mmu_sw_fini(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	gsgpu_gem_force_release(adev);
	gsgpu_vm_manager_fini(adev);
	kfree(adev->gmc.vm_fault_info);
	gsgpu_gart_table_vram_free(adev);
	gsgpu_bo_fini(adev);
	gsgpu_gart_fini(adev);
	release_firmware(adev->gmc.fw);
	adev->gmc.fw = NULL;

	return 0;
}

static int mmu_hw_init(void *handle)
{
	int r;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	r = mmu_gart_enable(adev);
	if (r)
		return r;

	return r;
}

static int mmu_hw_fini(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	gsgpu_irq_put(adev, &adev->gmc.vm_fault, 0);
	mmu_gart_disable(adev);

	return 0;
}

static int mmu_suspend(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	mmu_hw_fini(adev);

	return 0;
}

static int mmu_resume(void *handle)
{
	int r;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	r = mmu_hw_init(adev);
	if (r)
		return r;

	gsgpu_vmid_reset_all(adev);

	return 0;
}

static bool mmu_is_idle(void *handle)
{
	return true;
}

static bool mmu_check_soft_reset(void *handle)
{
	return false;
}

static int mmu_pre_soft_reset(void *handle)
{
	return 0;
}

static int mmu_soft_reset(void *handle)
{
	return 0;
}

static int mmu_post_soft_reset(void *handle)
{
	return 0;
}

static int mmu_vm_fault_interrupt_state(struct gsgpu_device *adev,
					     struct gsgpu_irq_src *src,
					     unsigned type,
					     enum gsgpu_interrupt_state state)
{
	switch (state) {
	case GSGPU_IRQ_STATE_DISABLE:
		gsgpu_cmd_exec(adev, GSCMD(GSCMD_MMU, MMU_SET_EXC), 0, ~1);
		break;
	case GSGPU_IRQ_STATE_ENABLE:
		gsgpu_cmd_exec(adev, GSCMD(GSCMD_MMU, MMU_SET_EXC), 1, ~1);
		break;
	default:
		break;
	}

	return 0;
}

/**
 * mmu_process_interrupt
 * TODO
 * @adev
 * @source
 * @entry
 *
 * Return:

 */
static int mmu_process_interrupt(struct gsgpu_device *adev,
				      struct gsgpu_irq_src *source,
				      struct gsgpu_iv_entry *entry)
{
	u32 addr_hi, addr_lo, status;

	addr_lo = entry->src_data[0];
	addr_hi = entry->src_data[1];
	status =  (entry->src_data[1] >> 16);

	if (printk_ratelimit()) {
		struct gsgpu_task_info task_info;

		gsgpu_vm_get_task_info(adev, entry->pasid, &task_info);

		dev_err(adev->dev, "GPU fault detected: %d  vmid %d pasid %d for process %s pid %d thread %s pid %d\n",
			entry->src_id, entry->vmid, entry->pasid, task_info.process_name,
			task_info.tgid, task_info.task_name, task_info.pid);
		dev_err(adev->dev, "  VM_CONTEXT1_PROTECTION_FAULT_ADDR   0x%08x%08x\n",
			addr_hi, addr_lo);
		dev_err(adev->dev, "  VM_CONTEXT1_PROTECTION_FAULT_STATUS 0x%04X\n",
			status);
	}

	return 0;
}

static void mmu_emit_pasid_mapping(struct gsgpu_ring *ring, unsigned vmid,
					unsigned pasid)
{
	/**
	 * let the firmware save the mapping
	 * relationship between vmid and pasid in DRAM
	 * By simulating a command stream.
	 * The true ways is Set regs instead of this way
	**/

	gsgpu_ring_write(ring, GSPKT(GSPKT_VM_BIND, 3));
	gsgpu_ring_write(ring, ring->funcs->type);
	gsgpu_ring_write(ring, vmid);
	gsgpu_ring_write(ring, pasid);
}

static const struct gsgpu_ip_funcs mmu_ip_funcs = {
	.name = "mmu",
	.early_init = mmu_early_init,
	.late_init = mmu_late_init,
	.sw_init = mmu_sw_init,
	.sw_fini = mmu_sw_fini,
	.hw_init = mmu_hw_init,
	.hw_fini = mmu_hw_fini,
	.suspend = mmu_suspend,
	.resume = mmu_resume,
	.is_idle = mmu_is_idle,
	.wait_for_idle = NULL,
	.check_soft_reset = mmu_check_soft_reset,
	.pre_soft_reset = mmu_pre_soft_reset,
	.soft_reset = mmu_soft_reset,
	.post_soft_reset = mmu_post_soft_reset,
};

static const struct gsgpu_gmc_funcs mmu_gmc_funcs = {
	.flush_gpu_tlb = mmu_flush_gpu_tlb,
	.emit_flush_gpu_tlb = mmu_emit_flush_gpu_tlb,
	.emit_pasid_mapping = mmu_emit_pasid_mapping,
	.set_pte_pde = mmu_set_pte_pde,
	.get_vm_pte_flags = mmu_get_vm_pte_flags,
	.get_vm_pde = mmu_get_vm_pde
};

static const struct gsgpu_irq_src_funcs mmu_irq_funcs = {
	.set = mmu_vm_fault_interrupt_state,
	.process = mmu_process_interrupt,
};

static void mmu_set_gmc_funcs(struct gsgpu_device *adev)
{
	if (adev->gmc.gmc_funcs == NULL)
		adev->gmc.gmc_funcs = &mmu_gmc_funcs;
}

static void mmu_set_irq_funcs(struct gsgpu_device *adev)
{
	adev->gmc.vm_fault.num_types = 1;
	adev->gmc.vm_fault.funcs = &mmu_irq_funcs;
}
const struct gsgpu_ip_block_version mmu_ip_block = {
	.type = GSGPU_IP_BLOCK_TYPE_GMC,
	.major = 1,
	.minor = 0,
	.rev = 0,
	.funcs = &mmu_ip_funcs,
};
