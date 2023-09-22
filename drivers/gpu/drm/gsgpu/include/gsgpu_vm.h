#ifndef __GSGPU_VM_H__
#define __GSGPU_VM_H__

#include <linux/idr.h>
#include <linux/kfifo.h>
#include <linux/rbtree.h>
#include <drm/gpu_scheduler.h>
#include <drm/drm_file.h>

#include "gsgpu_sync.h"
#include "gsgpu_ring.h"
#include "gsgpu_ids.h"

struct gsgpu_bo_va;
struct gsgpu_job;
struct gsgpu_bo_list_entry;

#define GSGPU_VM_PDE_PTE_BYTES      8

/* Maximum number of PTEs can write with one job */
#define GSGPU_VM_MAX_UPDATE_SIZE	(1024ull)

/* The number of PTEs a block contains */
#define GSGPU_VM_PTE_COUNT(adev) (1 << (adev)->vm_manager.block_size)

/* PTBs (Page Table Blocks) need to be aligned to 32K */
#define GSGPU_VM_PTB_ALIGN_SIZE   32768

#define GSGPU_PTE_PRESENT	(1ULL << 0)
#define GSGPU_PTE_HUGEPAGE	(1ULL << 1)
#define GSGPU_PTE_EXCEPTION	(1ULL << 2)
#define GSGPU_PTE_WRITEABLE	(1ULL << 3)
/* comprssed pte flags use bit 4 to 6 */
#define GSGPU_PTE_COMPRESSED_SHIFT (5)

/* How to programm VM fault handling */
#define GSGPU_VM_FAULT_STOP_NEVER	0
#define GSGPU_VM_FAULT_STOP_FIRST	1
#define GSGPU_VM_FAULT_STOP_ALWAYS	2

/* hardcode that limit for now */
#define GSGPU_VA_RESERVED_SIZE			(1ULL << 20)

/* VA hole for 48bit addresses on Vega10 */
#define GSGPU_VA_HOLE_START			0x0000800000000000ULL
#define GSGPU_VA_HOLE_END			0xffff800000000000ULL

/*
 * Hardware is programmed as if the hole doesn't exists with start and end
 * address values.
 *
 * This mask is used to remove the upper 16bits of the VA and so come up with
 * the linear addr value.
 */
#define GSGPU_VA_HOLE_MASK			0x0000ffffffffffffULL

/* max vmids dedicated for process */
#define GSGPU_VM_MAX_RESERVED_VMID	1

#define GSGPU_VM_CONTEXT_GFX 0
#define GSGPU_VM_CONTEXT_COMPUTE 1

/* VMPT level enumerate, and the hiberachy is:
 * DIR0->DIR1->DIR2
 */
enum gsgpu_vm_level {
	GSGPU_VM_DIR0,
	GSGPU_VM_DIR1,
	GSGPU_VM_DIR2
};

/* base structure for tracking BO usage in a VM */
struct gsgpu_vm_bo_base {
	/* constant after initialization */
	struct gsgpu_vm		*vm;
	struct gsgpu_bo		*bo;

	/* protected by bo being reserved */
	struct list_head		bo_list;

	/* protected by spinlock */
	struct list_head		vm_status;

	/* protected by the BO being reserved */
	bool				moved;
};

struct gsgpu_vm_pt {
	struct gsgpu_vm_bo_base	base;
	bool				huge;

	/* array of page tables, one for each directory entry */
	struct gsgpu_vm_pt		*entries;
};

#define GSGPU_VM_FAULT(pasid, addr) (((u64)(pasid) << 48) | (addr))
#define GSGPU_VM_FAULT_PASID(fault) ((u64)(fault) >> 48)
#define GSGPU_VM_FAULT_ADDR(fault)  ((u64)(fault) & 0xfffffffff000ULL)


struct gsgpu_task_info {
	char	process_name[TASK_COMM_LEN];
	char	task_name[TASK_COMM_LEN];
	pid_t	pid;
	pid_t	tgid;
};

struct gsgpu_vm {
	/* tree of virtual addresses mapped */
	struct rb_root_cached	va;

	/* BOs who needs a validation */
	struct list_head	evicted;

	/* PT BOs which relocated and their parent need an update */
	struct list_head	relocated;

	/* BOs moved, but not yet updated in the PT */
	struct list_head	moved;
	spinlock_t		moved_lock;

	/* All BOs of this VM not currently in the state machine */
	struct list_head	idle;

	/* BO mappings freed, but not yet updated in the PT */
	struct list_head	freed;

	/* contains the page directory */
	struct gsgpu_vm_pt     root;
	struct dma_fence	*last_update;

	/* Scheduler entity for page table updates */
	struct drm_sched_entity	entity;

	unsigned int		pasid;
	/* dedicated to vm */
	struct gsgpu_vmid	*reserved_vmid;

	/* Flag to indicate if VM tables are updated by CPU or GPU (XDMA) */
	bool                    use_cpu_for_update;

	/* Flag to indicate ATS support from PTE for GFX9 */
	bool			pte_support_ats;

	/* Up to 128 pending retry page faults */
	DECLARE_KFIFO(faults, u64, 128);

	/* Limit non-retry fault storms */
	unsigned int		fault_credit;

	/* Valid while the PD is reserved or fenced */
	uint64_t		pd_phys_addr;

	/* Some basic info about the task */
	struct gsgpu_task_info task_info;
};

struct gsgpu_vm_manager {
	/* Handling of VMIDs */
	struct gsgpu_vmid_mgr			id_mgr;

	/* Handling of VM fences */
	u64					fence_context;
	unsigned				seqno[GSGPU_MAX_RINGS];

    uint32_t                pde_pte_bytes;

	uint64_t				max_pfn;
	uint32_t				num_level;
	uint32_t				block_size;
	uint32_t                fragment_size;
	enum gsgpu_vm_level		root_level;
	uint32_t dir0_shift, dir0_width;
	uint32_t dir1_shift, dir1_width;
	uint32_t dir2_shift, dir2_width;

	/* vram base address for page table entry  */
	u64					vram_base_offset;
	/* vm pte handling */
	const struct gsgpu_vm_pte_funcs        *vm_pte_funcs;
	struct gsgpu_ring                      *vm_pte_rings[GSGPU_MAX_RINGS];
	unsigned				vm_pte_num_rings;
	atomic_t				vm_pte_next_ring;

	/* partial resident texture handling */
	spinlock_t				prt_lock;
	atomic_t				num_prt_users;

	/* controls how VM page tables are updated for Graphics and Compute.
	 * BIT0[= 0] Graphics updated by XDMA [= 1] by CPU
	 * BIT1[= 0] Compute updated by XDMA [= 1] by CPU
	 */
	int					vm_update_mode;

	/* PASID to VM mapping, will be used in interrupt context to
	 * look up VM of a page fault
	 */
	struct idr				pasid_idr;
	spinlock_t				pasid_lock;
};

void gsgpu_vm_manager_init(struct gsgpu_device *adev);
void gsgpu_vm_manager_fini(struct gsgpu_device *adev);
int gsgpu_vm_init(struct gsgpu_device *adev, struct gsgpu_vm *vm,
		   int vm_context, unsigned int pasid);
void gsgpu_vm_fini(struct gsgpu_device *adev, struct gsgpu_vm *vm);
bool gsgpu_vm_pasid_fault_credit(struct gsgpu_device *adev,
				  unsigned int pasid);
void gsgpu_vm_get_pd_bo(struct gsgpu_vm *vm,
			 struct list_head *validated,
			 struct gsgpu_bo_list_entry *entry);
bool gsgpu_vm_ready(struct gsgpu_vm *vm);
int gsgpu_vm_validate_pt_bos(struct gsgpu_device *adev, struct gsgpu_vm *vm,
			      int (*callback)(void *p, struct gsgpu_bo *bo),
			      void *param);
int gsgpu_vm_alloc_pts(struct gsgpu_device *adev,
			struct gsgpu_vm *vm,
			uint64_t saddr, uint64_t size);
int gsgpu_vm_flush(struct gsgpu_ring *ring, struct gsgpu_job *job, bool need_pipe_sync);
int gsgpu_vm_update_directories(struct gsgpu_device *adev,
				 struct gsgpu_vm *vm);
int gsgpu_vm_clear_freed(struct gsgpu_device *adev,
			  struct gsgpu_vm *vm,
			  struct dma_fence **fence);
int gsgpu_vm_handle_moved(struct gsgpu_device *adev,
			   struct gsgpu_vm *vm);
int gsgpu_vm_bo_update(struct gsgpu_device *adev,
			struct gsgpu_bo_va *bo_va,
			bool clear);
void gsgpu_vm_bo_invalidate(struct gsgpu_device *adev,
			     struct gsgpu_bo *bo, bool evicted);
struct gsgpu_bo_va *gsgpu_vm_bo_find(struct gsgpu_vm *vm,
				       struct gsgpu_bo *bo);
struct gsgpu_bo_va *gsgpu_vm_bo_add(struct gsgpu_device *adev,
				      struct gsgpu_vm *vm,
				      struct gsgpu_bo *bo);
int gsgpu_vm_bo_map(struct gsgpu_device *adev,
		     struct gsgpu_bo_va *bo_va,
		     uint64_t addr, uint64_t offset,
		     uint64_t size, uint64_t flags);
int gsgpu_vm_bo_replace_map(struct gsgpu_device *adev,
			     struct gsgpu_bo_va *bo_va,
			     uint64_t addr, uint64_t offset,
			     uint64_t size, uint64_t flags);
int gsgpu_vm_bo_unmap(struct gsgpu_device *adev,
		       struct gsgpu_bo_va *bo_va,
		       uint64_t addr);
int gsgpu_vm_bo_clear_mappings(struct gsgpu_device *adev,
				struct gsgpu_vm *vm,
				uint64_t saddr, uint64_t size);
struct gsgpu_bo_va_mapping *gsgpu_vm_bo_lookup_mapping(struct gsgpu_vm *vm,
							 uint64_t addr);
void gsgpu_vm_bo_trace_cs(struct gsgpu_vm *vm, struct ww_acquire_ctx *ticket);
void gsgpu_vm_bo_rmv(struct gsgpu_device *adev,
		      struct gsgpu_bo_va *bo_va);
void gsgpu_vm_adjust_size(struct gsgpu_device *adev, uint32_t min_vm_size,
			   unsigned max_level, unsigned max_bits);
int gsgpu_vm_ioctl(struct drm_device *dev, void *data, struct drm_file *filp);
bool gsgpu_vm_need_pipeline_sync(struct gsgpu_ring *ring,
				  struct gsgpu_job *job);

void gsgpu_vm_get_task_info(struct gsgpu_device *adev, unsigned int pasid,
			 struct gsgpu_task_info *task_info);

void gsgpu_vm_set_task_info(struct gsgpu_vm *vm);

#endif /* __GSGPU_VM_H__ */
