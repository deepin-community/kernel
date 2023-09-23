#ifndef __GSGPU_IDS_H__
#define __GSGPU_IDS_H__

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/dma-fence.h>

#include "gsgpu_sync.h"

/* maximum number of VMIDs */
#define GSGPU_NUM_VMID	4

struct gsgpu_device;
struct gsgpu_vm;
struct gsgpu_ring;
struct gsgpu_sync;
struct gsgpu_job;

struct gsgpu_vmid {
	struct list_head	list;
	struct gsgpu_sync	active;
	struct dma_fence	*last_flush;
	uint64_t		owner;

	uint64_t		pd_gpu_addr;
	/* last flushed PD/PT update */
	struct dma_fence	*flushed_updates;

	uint32_t                current_gpu_reset_count;

	unsigned		pasid;
	struct dma_fence	*pasid_mapping;
};

struct gsgpu_vmid_mgr {
	struct mutex		lock;
	unsigned		num_ids;
	struct list_head	ids_lru;
	struct gsgpu_vmid	ids[GSGPU_NUM_VMID];
	atomic_t		reserved_vmid_num;
};

int gsgpu_pasid_alloc(unsigned int bits);
void gsgpu_pasid_free(unsigned int pasid);
void gsgpu_pasid_free_delayed(struct dma_resv *resv,
			       unsigned int pasid);

bool gsgpu_vmid_had_gpu_reset(struct gsgpu_device *adev,
			       struct gsgpu_vmid *id);
int gsgpu_vmid_alloc_reserved(struct gsgpu_device *adev, struct gsgpu_vm *vm);
void gsgpu_vmid_free_reserved(struct gsgpu_device *adev, struct gsgpu_vm *vm);
int gsgpu_vmid_grab(struct gsgpu_vm *vm, struct gsgpu_ring *ring,
		     struct gsgpu_sync *sync, struct dma_fence *fence,
		     struct gsgpu_job *job);
void gsgpu_vmid_reset(struct gsgpu_device *adev, unsigned vmid);
void gsgpu_vmid_reset_all(struct gsgpu_device *adev);

void gsgpu_vmid_mgr_init(struct gsgpu_device *adev);
void gsgpu_vmid_mgr_fini(struct gsgpu_device *adev);

#endif /* __GSGPU_IDS_H__ */
