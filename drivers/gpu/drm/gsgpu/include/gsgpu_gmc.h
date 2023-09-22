#ifndef __GSGPU_GMC_H__
#define __GSGPU_GMC_H__

#include <linux/types.h>

#include "gsgpu_irq.h"

struct firmware;

/*
 * GPU MC structures, functions & helpers
 */
struct gsgpu_gmc_funcs {
	/* flush the vm tlb via mmio */
	void (*flush_gpu_tlb)(struct gsgpu_device *adev,
			      uint32_t vmid);
	/* flush the vm tlb via ring */
	uint64_t (*emit_flush_gpu_tlb)(struct gsgpu_ring *ring, unsigned vmid,
				       uint64_t pd_addr);
	/* Change the VMID -> PASID mapping */
	void (*emit_pasid_mapping)(struct gsgpu_ring *ring, unsigned vmid,
				   unsigned pasid);
	/* write pte/pde updates using the cpu */
	int (*set_pte_pde)(struct gsgpu_device *adev,
			   void *cpu_pt_addr, /* cpu addr of page table */
			   uint32_t gpu_page_idx, /* pte/pde to update */
			   uint64_t addr, /* addr to write into pte/pde */
			   uint64_t flags); /* access flags */
	/* set pte flags based per asic */
	uint64_t (*get_vm_pte_flags)(struct gsgpu_device *adev,
				     uint32_t flags);
	/* get the pde for a given mc addr */
	void (*get_vm_pde)(struct gsgpu_device *adev, int level,
			   u64 *dst, u64 *flags);
};

struct gsgpu_vm_fault_info {
	uint64_t	page_addr;
	uint32_t	vmid;
	uint32_t	mc_id;
	uint32_t	status;
	bool		prot_valid;
	bool		prot_read;
	bool		prot_write;
	bool		prot_exec;
};

struct gsgpu_gmc {
	resource_size_t		aper_size;
	resource_size_t		aper_base;
	/* for some chips with <= 32MB we need to lie
	 * about vram size near mc fb location */
	u32 			dma_bits;
	u64			mc_vram_size;
	u64			visible_vram_size;
	u64			gart_size;
	u64			gart_start;
	u64			gart_end;
	u64			vram_start;
	u64			vram_end;
	unsigned		vram_width;
	u64			real_vram_size;
	int			vram_mtrr;
	u64                     mc_mask;
	const struct firmware   *fw;	/* MC firmware */
	uint32_t                fw_version;
	struct gsgpu_irq_src	vm_fault;
	uint32_t		vram_type;
	bool			prt_warning;
	uint64_t		stolen_size;
	/* protects concurrent invalidation */
	spinlock_t		invalidate_lock;
	struct gsgpu_vm_fault_info 	*vm_fault_info;

	const struct gsgpu_gmc_funcs	*gmc_funcs;
};

/**
 * gsgpu_gmc_vram_full_visible - Check if full VRAM is visible through the BAR
 *
 * @adev: gsgpu_device pointer
 *
 * Returns:
 * True if full VRAM is visible through the BAR
 */
static inline bool gsgpu_gmc_vram_full_visible(struct gsgpu_gmc *gmc)
{
	WARN_ON(gmc->real_vram_size < gmc->visible_vram_size);

	return (gmc->real_vram_size == gmc->visible_vram_size);
}

#endif /* __GSGPU_GMC_H__ */
