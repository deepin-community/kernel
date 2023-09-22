#ifndef __GSGPU_RING_H__
#define __GSGPU_RING_H__

#include <drm/gsgpu_drm.h>
#include <drm/gpu_scheduler.h>
#include <drm/drm_print.h>

/* max number of rings */
#define GSGPU_MAX_RINGS		21
#define GSGPU_MAX_GFX_RINGS		1

/* some special values for the owner field */
#define GSGPU_FENCE_OWNER_UNDEFINED	((void *)0ul)
#define GSGPU_FENCE_OWNER_VM		((void *)1ul)
#define GSGPU_FENCE_OWNER_KFD		((void *)2ul)

#define GSGPU_FENCE_FLAG_64BIT         (1 << 0)
#define GSGPU_FENCE_FLAG_INT           (1 << 1)
#define GSGPU_FENCE_FLAG_TC_WB_ONLY    (1 << 2)

#define to_gsgpu_ring(s) container_of((s), struct gsgpu_ring, sched)

enum gsgpu_ring_type {
	GSGPU_RING_TYPE_GFX,
	GSGPU_RING_TYPE_XDMA,
};

struct gsgpu_device;
struct gsgpu_ring;
struct gsgpu_ib;
struct gsgpu_cs_parser;
struct gsgpu_job;

/*
 * Fences.
 */
struct gsgpu_fence_driver {
	uint64_t			gpu_addr;
	volatile uint32_t		*cpu_addr;
	/* sync_seq is protected by ring emission lock */
	uint32_t			sync_seq;
	atomic_t			last_seq;
	bool				initialized;
	struct gsgpu_irq_src		*irq_src;
	unsigned			irq_type;
	struct timer_list		fallback_timer;
	unsigned			num_fences_mask;
	spinlock_t			lock;
	struct dma_fence		**fences;
};

int gsgpu_fence_driver_init(struct gsgpu_device *adev);
void gsgpu_fence_driver_fini(struct gsgpu_device *adev);
void gsgpu_fence_driver_force_completion(struct gsgpu_ring *ring);

int gsgpu_fence_driver_init_ring(struct gsgpu_ring *ring,
				  unsigned num_hw_submission);
int gsgpu_fence_driver_start_ring(struct gsgpu_ring *ring,
				   struct gsgpu_irq_src *irq_src,
				   unsigned irq_type);
void gsgpu_fence_driver_suspend(struct gsgpu_device *adev);
void gsgpu_fence_driver_resume(struct gsgpu_device *adev);
int gsgpu_fence_emit(struct gsgpu_ring *ring, struct dma_fence **fence,
		      unsigned flags);
int gsgpu_fence_emit_polling(struct gsgpu_ring *ring, uint32_t *s);
void gsgpu_fence_process(struct gsgpu_ring *ring);
int gsgpu_fence_wait_empty(struct gsgpu_ring *ring);
signed long gsgpu_fence_wait_polling(struct gsgpu_ring *ring,
				      uint32_t wait_seq,
				      signed long timeout);
unsigned gsgpu_fence_count_emitted(struct gsgpu_ring *ring);

/*
 * Rings.
 */

/* provided by hw blocks that expose a ring buffer for commands */
struct gsgpu_ring_funcs {
	enum gsgpu_ring_type	type;
	uint32_t		align_mask;
	u32			nop;
	bool			support_64bit_ptrs;
	unsigned		vmhub;
	unsigned		extra_dw;

	/* ring read/write ptr handling */
	u64 (*get_rptr)(struct gsgpu_ring *ring);
	u64 (*get_wptr)(struct gsgpu_ring *ring);
	void (*set_wptr)(struct gsgpu_ring *ring);
	/* validating and patching of IBs */
	int (*parse_cs)(struct gsgpu_cs_parser *p, uint32_t ib_idx);
	int (*patch_cs_in_place)(struct gsgpu_cs_parser *p, uint32_t ib_idx);
	/* constants to calculate how many DW are needed for an emit */
	unsigned emit_frame_size;
	unsigned emit_ib_size;
	/* command emit functions */
	void (*emit_ib)(struct gsgpu_ring *ring,
			struct gsgpu_ib *ib,
			unsigned vmid, bool ctx_switch);
	void (*emit_fence)(struct gsgpu_ring *ring, uint64_t addr,
			   uint64_t seq, unsigned flags);
	void (*emit_pipeline_sync)(struct gsgpu_ring *ring);
	void (*emit_vm_flush)(struct gsgpu_ring *ring, unsigned vmid,
			      uint64_t pd_addr);
	/* testing functions */
	int (*test_ring)(struct gsgpu_ring *ring);
	int (*test_ib)(struct gsgpu_ring *ring, long timeout);
	int (*test_xdma)(struct gsgpu_ring *ring, long timeout);
	/* insert NOP packets */
	void (*insert_nop)(struct gsgpu_ring *ring, uint32_t count);
	void (*insert_start)(struct gsgpu_ring *ring);
	void (*insert_end)(struct gsgpu_ring *ring);
	/* pad the indirect buffer to the necessary number of dw */
	void (*pad_ib)(struct gsgpu_ring *ring, struct gsgpu_ib *ib);
	unsigned (*init_cond_exec)(struct gsgpu_ring *ring);
	void (*patch_cond_exec)(struct gsgpu_ring *ring, unsigned offset);
	/* note usage for clock and power gating */
	void (*begin_use)(struct gsgpu_ring *ring);
	void (*end_use)(struct gsgpu_ring *ring);
	void (*emit_switch_buffer) (struct gsgpu_ring *ring);
	void (*emit_cntxcntl) (struct gsgpu_ring *ring, uint32_t flags);
	void (*emit_rreg)(struct gsgpu_ring *ring, uint32_t reg);
	void (*emit_wreg)(struct gsgpu_ring *ring, uint32_t reg, uint32_t val);
	void (*emit_reg_wait)(struct gsgpu_ring *ring, uint32_t reg,
			      uint32_t val, uint32_t mask);
	void (*emit_reg_write_reg_wait)(struct gsgpu_ring *ring,
					uint32_t reg0, uint32_t reg1,
					uint32_t ref, uint32_t mask);
	void (*emit_tmz)(struct gsgpu_ring *ring, bool start);
	/* priority functions */
	void (*set_priority) (struct gsgpu_ring *ring,
			      enum drm_sched_priority priority);
};

struct gsgpu_ring {
	struct gsgpu_device		*adev;
	const struct gsgpu_ring_funcs	*funcs;
	struct gsgpu_fence_driver	fence_drv;
	struct drm_gpu_scheduler	sched;
	struct list_head		lru_list;

	struct gsgpu_bo	*ring_obj;
	volatile uint32_t	*ring;
	unsigned		rptr_offs;
	u64			wptr;
	u64			wptr_old;
	unsigned		ring_size;
	unsigned		max_dw;
	int			count_dw;
	uint64_t		gpu_addr;
	uint64_t		ptr_mask;
	uint32_t		buf_mask;
	bool			ready;
	u32			idx;
	u32			me;
	u32			pipe;
	u32			queue;
	bool			use_pollmem;
	unsigned		wptr_offs;
	unsigned		fence_offs;
	uint64_t		current_ctx;
	char			name[16];
	unsigned		cond_exe_offs;
	u64			cond_exe_gpu_addr;
	volatile u32		*cond_exe_cpu_addr;
	unsigned		vm_inv_eng;
	struct dma_fence	*vmid_wait;

	atomic_t		num_jobs[DRM_SCHED_PRIORITY_MAX];
	struct mutex		priority_mutex;
	/* protected by priority_mutex */
	int			priority;

#if defined(CONFIG_DEBUG_FS)
	struct dentry *ent;
#endif
};

int gsgpu_ring_alloc(struct gsgpu_ring *ring, unsigned ndw);
void gsgpu_ring_insert_nop(struct gsgpu_ring *ring, uint32_t count);
void gsgpu_ring_generic_pad_ib(struct gsgpu_ring *ring, struct gsgpu_ib *ib);
void gsgpu_ring_commit(struct gsgpu_ring *ring);
void gsgpu_ring_undo(struct gsgpu_ring *ring);
void gsgpu_ring_priority_get(struct gsgpu_ring *ring,
			      enum drm_sched_priority priority);
void gsgpu_ring_priority_put(struct gsgpu_ring *ring,
			      enum drm_sched_priority priority);
int gsgpu_ring_init(struct gsgpu_device *adev, struct gsgpu_ring *ring,
		     unsigned ring_size, struct gsgpu_irq_src *irq_src,
		     unsigned irq_type);
void gsgpu_ring_fini(struct gsgpu_ring *ring);
int gsgpu_ring_lru_get(struct gsgpu_device *adev, int type,
			int *blacklist, int num_blacklist,
			bool lru_pipe_order, struct gsgpu_ring **ring);
void gsgpu_ring_lru_touch(struct gsgpu_device *adev, struct gsgpu_ring *ring);
void gsgpu_ring_emit_reg_write_reg_wait_helper(struct gsgpu_ring *ring,
						uint32_t reg0, uint32_t val0,
						uint32_t reg1, uint32_t val1);

static inline void gsgpu_ring_clear_ring(struct gsgpu_ring *ring)
{
	int i = 0;
	while (i <= ring->buf_mask)
		ring->ring[i++] = ring->funcs->nop;

}

static inline void gsgpu_ring_write(struct gsgpu_ring *ring, uint32_t v)
{
	if (ring->count_dw <= 0)
		DRM_ERROR("gsgpu: writing more dwords to the ring than expected!\n");
	//DRM_INFO("dw[0x%x]:%x\n", ring->wptr, v);
	ring->ring[ring->wptr++ & ring->buf_mask] = v;
	ring->wptr &= ring->ptr_mask;
	ring->count_dw--;
}

static inline void gsgpu_ring_write_multiple(struct gsgpu_ring *ring,
					      void *src, int count_dw)
{
	unsigned occupied, chunk1, chunk2;
	void *dst;

	if (unlikely(ring->count_dw < count_dw))
		DRM_ERROR("gsgpu: writing more dwords to the ring than expected!\n");

	occupied = ring->wptr & ring->buf_mask;
	dst = (void *)&ring->ring[occupied];
	chunk1 = ring->buf_mask + 1 - occupied;
	chunk1 = (chunk1 >= count_dw) ? count_dw : chunk1;
	chunk2 = count_dw - chunk1;
	chunk1 <<= 2;
	chunk2 <<= 2;

	if (chunk1)
		memcpy(dst, src, chunk1);

	if (chunk2) {
		src += chunk1;
		dst = (void *)ring->ring;
		memcpy(dst, src, chunk2);
	}

	ring->wptr += count_dw;
	ring->wptr &= ring->ptr_mask;
	ring->count_dw -= count_dw;
}

#endif /* __GSGPU_RING_H__ */
