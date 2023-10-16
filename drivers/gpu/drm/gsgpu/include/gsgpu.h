#ifndef __GSGPU_H__
#define __GSGPU_H__

#include <linux/atomic.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/file.h>
#include <linux/kref.h>
#include <linux/rbtree.h>
#include <linux/hashtable.h>
#include <linux/dma-resv.h>
#include <linux/dma-fence.h>
#include <linux/pci.h>
#include <linux/hmm.h>

#include <drm/ttm/ttm_bo.h>
#include <drm/ttm/ttm_tt.h>
#include <drm/ttm/ttm_placement.h>
#include <drm/ttm/ttm_execbuf_util.h>
#include <drm/drm_gem_ttm_helper.h>
#include <drm/drm_probe_helper.h>

#include <drm/drm_gem.h>
#include <drm/gsgpu_drm.h>
#include <drm/gpu_scheduler.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_modeset_helper.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_drv.h>
#include <drm/drm_vblank.h>
#include <drm/drm_debugfs.h>

#include "gsgpu_shared.h"
#include "gsgpu_mode.h"
#include "gsgpu_ih.h"
#include "gsgpu_irq.h"
#include "gsgpu_vram_mgr.h"
#include "gsgpu_ttm.h"
#include "gsgpu_sync.h"
#include "gsgpu_ring.h"
#include "gsgpu_vm.h"
#include "gsgpu_hmm.h"
#include "gsgpu_gmc.h"
#include "gsgpu_dc.h"
#include "gsgpu_dc_resource.h"
#include "gsgpu_gart.h"
#include "gsgpu_zip_meta.h"
#include "gsgpu_debugfs.h"
#include "gsgpu_job.h"
#include "gsgpu_bo_list.h"
#include "gsgpu_hw_sema.h"

/*
 * Modules parameters.
 */
extern int gsgpu_modeset;
extern int gsgpu_vram_limit;
extern int gsgpu_vis_vram_limit;
extern int gsgpu_gart_size;
extern int gsgpu_gtt_size;
extern int gsgpu_moverate;
extern int gsgpu_benchmarking;
extern int gsgpu_testing;
extern int gsgpu_disp_priority;
extern int gsgpu_msi;
extern int gsgpu_lockup_timeout;
extern int gsgpu_runtime_pm;
extern int gsgpu_vm_size;
extern int gsgpu_vm_block_size;
extern int gsgpu_vm_fragment_size;
extern int gsgpu_vm_fault_stop;
extern int gsgpu_vm_debug;
extern int gsgpu_vm_update_mode;
extern int gsgpu_sched_jobs;
extern int gsgpu_sched_hw_submission;
extern int gsgpu_vram_page_split;
extern int gsgpu_job_hang_limit;
extern int gsgpu_gpu_recovery;
extern int gsgpu_using_ram;
extern int gsgpu_enable_dma40;

#define GSGPU_BYTES_PER_DW           4

#define GSGPU_KB_SHIFT_BITS          10
#define GSGPU_MB_SHIFT_BITS          20
#define GSGPU_GB_SHIFT_BITS          30

#define GSGPU_SG_THRESHOLD			(256*1024*1024)
#define GSGPU_DEFAULT_GTT_SIZE_MB		3072ULL /* 3GB by default */
#define GSGPU_WAIT_IDLE_TIMEOUT_IN_MS	        3000
#define GSGPU_MAX_USEC_TIMEOUT			100000	/* 100 ms */
#define GSGPU_FENCE_JIFFIES_TIMEOUT		(HZ / 2)
/* GSGPU_IB_POOL_SIZE must be a power of 2 */
#define GSGPU_IB_POOL_SIZE			16
#define GSGPU_DEBUGFS_MAX_COMPONENTS		32
#define GSGPUFB_CONN_LIMIT			4
#define GSGPU_BIOS_NUM_SCRATCH			16

/* max number of IP instances */
#define GSGPU_MAX_XDMA_INSTANCES		2

/* hard reset data */
#define GSGPU_ASIC_RESET_DATA                  0x39d5e86b

/* GFX current status */
#define GSGPU_GFX_NORMAL_MODE			0x00000000L
#define GSGPU_GFX_SAFE_MODE			0x00000001L
#define GSGPU_GFX_PG_DISABLED_MODE		0x00000002L
#define GSGPU_GFX_CG_DISABLED_MODE		0x00000004L
#define GSGPU_GFX_LBPW_DISABLED_MODE		0x00000008L

/* PCI IDs for supported devices */
#define LG100_DEVICE_ID		0x7A25
#define LG100_VGA_DEVICE_ID	0x7A36

struct gsgpu_device;
struct gsgpu_ib;
struct gsgpu_cs_parser;
struct gsgpu_job;
struct gsgpu_irq_src;
struct gsgpu_fpriv;
struct gsgpu_bo_va_mapping;

enum gsgpu_chip {
	dev_7a2000,
	dev_2k2000
};

enum gsgpu_cp_irq {
	GSGPU_CP_IRQ_GFX_EOP = 0,
	GSGPU_CP_IRQ_LAST
};

enum gsgpu_xdma_irq {
	GSGPU_XDMA_IRQ_TRAP0 = 0,
	GSGPU_XDMA_IRQ_TRAP1,
	GSGPU_XDMA_IRQ_LAST
};

int gsgpu_device_ip_wait_for_idle(struct gsgpu_device *adev,
				  enum gsgpu_ip_block_type block_type);
bool gsgpu_device_ip_is_idle(struct gsgpu_device *adev,
			     enum gsgpu_ip_block_type block_type);

#define GSGPU_MAX_IP_NUM 16

struct gsgpu_ip_block_status {
	bool valid;
	bool sw;
	bool hw;
	bool late_initialized;
	bool hang;
};

struct gsgpu_ip_block_version {
	const enum gsgpu_ip_block_type type;
	const u32 major;
	const u32 minor;
	const u32 rev;
	const struct gsgpu_ip_funcs *funcs;
};

struct gsgpu_ip_block {
	struct gsgpu_ip_block_status status;
	const struct gsgpu_ip_block_version *version;
};

int gsgpu_device_ip_block_version_cmp(struct gsgpu_device *adev,
				      enum gsgpu_ip_block_type type,
				      u32 major, u32 minor);

struct gsgpu_ip_block *
gsgpu_device_ip_get_ip_block(struct gsgpu_device *adev,
			     enum gsgpu_ip_block_type type);

int gsgpu_device_ip_block_add(struct gsgpu_device *adev,
			      const struct gsgpu_ip_block_version *ip_block_version);

/* provided by hw blocks that can move/clear data.  e.g., gfx or xdma */
struct gsgpu_buffer_funcs {
	/* maximum bytes in a single operation */
	uint32_t	copy_max_bytes;

	/* number of dw to reserve per operation */
	unsigned	copy_num_dw;

	/* used for buffer migration */
	void (*emit_copy_buffer)(struct gsgpu_ib *ib,
				 /* src addr in bytes */
				 uint64_t src_offset,
				 /* dst addr in bytes */
				 uint64_t dst_offset,
				 /* number of byte to transfer */
				 uint32_t byte_count);

	/* maximum bytes in a single operation */
	uint32_t	fill_max_bytes;

	/* number of dw to reserve per operation */
	unsigned	fill_num_dw;

	/* used for buffer clearing */
	void (*emit_fill_buffer)(struct gsgpu_ib *ib,
				 /* value to write to memory */
				 uint32_t src_data,
				 /* dst addr in bytes */
				 uint64_t dst_offset,
				 /* number of byte to fill */
				 uint32_t byte_count);
};

/* provided by hw blocks that can write ptes, e.g., xdma */
struct gsgpu_vm_pte_funcs {
	/* number of dw to reserve per operation */
	unsigned	copy_pte_num_dw;

	/* number of dw to reserve per operation */
	unsigned	set_pte_pde_num_dw;

	/* copy pte entries from GART */
	void (*copy_pte)(struct gsgpu_ib *ib,
			 uint64_t pe, uint64_t src,
			 unsigned count);

	/* write pte one entry at a time with addr mapping */
	void (*write_pte)(struct gsgpu_ib *ib, uint64_t pe,
			  uint64_t value, unsigned count,
			  uint32_t incr);
	/* for linear pte/pde updates without addr mapping */
	void (*set_pte_pde)(struct gsgpu_ib *ib,
			    uint64_t pe,
			    uint64_t addr, unsigned count,
			    uint32_t incr, uint64_t flags);
};

/* provided by the ih block */
struct gsgpu_ih_funcs {
	/* ring read/write ptr handling, called from interrupt context */
	u32 (*get_wptr)(struct gsgpu_device *adev);
	bool (*prescreen_iv)(struct gsgpu_device *adev);
	void (*decode_iv)(struct gsgpu_device *adev,
			  struct gsgpu_iv_entry *entry);
	void (*set_rptr)(struct gsgpu_device *adev);
};

/*
 * BIOS.
 */
bool gsgpu_get_bios(struct gsgpu_device *adev);
bool gsgpu_read_bios(struct gsgpu_device *adev);

/*
 * Clocks
 */

#define GSGPU_MAX_PPLL 3

struct gsgpu_clock {
	/* 10 Khz units */
	uint32_t default_mclk;
	uint32_t default_sclk;
	uint32_t default_dispclk;
	uint32_t current_dispclk;
	uint32_t dp_extclk;
	uint32_t max_pixel_clock;
};

/*
 * GEM.
 */

#define GSGPU_GEM_DOMAIN_MAX		0x3
#define gem_to_gsgpu_bo(gobj) container_of((gobj), struct gsgpu_bo, tbo.base)

unsigned long gsgpu_gem_timeout(uint64_t timeout_ns);
struct drm_gem_object *
gsgpu_gem_prime_import_sg_table(struct drm_device *dev,
				struct dma_buf_attachment *attach,
				struct sg_table *sg);
struct sg_table *gsgpu_gem_prime_get_sg_table(struct drm_gem_object *obj);
struct dma_buf *gsgpu_gem_prime_export(struct drm_gem_object *gobj,
				       int flags);

/* sub-allocation manager, it has to be protected by another lock.
 * By conception this is an helper for other part of the driver
 * like the indirect buffer or semaphore, which both have their
 * locking.
 *
 * Principe is simple, we keep a list of sub allocation in offset
 * order (first entry has offset == 0, last entry has the highest
 * offset).
 *
 * When allocating new object we first check if there is room at
 * the end total_size - (last_object_offset + last_object_size) >=
 * alloc_size. If so we allocate new object there.
 *
 * When there is not enough room at the end, we start waiting for
 * each sub object until we reach object_offset+object_size >=
 * alloc_size, this object then become the sub object we return.
 *
 * Alignment can't be bigger than page size.
 *
 * Hole are not considered for allocation to keep things simple.
 * Assumption is that there won't be hole (all object on same
 * alignment).
 */

#define GSGPU_SA_NUM_FENCE_LISTS	32

struct gsgpu_sa_manager {
	wait_queue_head_t	wq;
	struct gsgpu_bo	*bo;
	struct list_head	*hole;
	struct list_head	flist[GSGPU_SA_NUM_FENCE_LISTS];
	struct list_head	olist;
	unsigned		size;
	uint64_t		gpu_addr;
	void			*cpu_ptr;
	uint32_t		domain;
	uint32_t		align;
};

/* sub-allocation buffer */
struct gsgpu_sa_bo {
	struct list_head		olist;
	struct list_head		flist;
	struct gsgpu_sa_manager	*manager;
	unsigned			soffset;
	unsigned			eoffset;
	struct dma_fence	        *fence;
};

/*
 * GEM objects.
 */
void gsgpu_gem_force_release(struct gsgpu_device *adev);
int gsgpu_gem_object_create(struct gsgpu_device *adev, unsigned long size,
			    int alignment, u32 initial_domain,
			    u64 flags, enum ttm_bo_type type,
			    struct dma_resv *resv,
			    struct drm_gem_object **obj);

int gsgpu_mode_dumb_create(struct drm_file *file_priv,
			   struct drm_device *dev,
			   struct drm_mode_create_dumb *args);
int gsgpu_mode_dumb_mmap(struct drm_file *filp,
			 struct drm_device *dev,
			 uint32_t handle, uint64_t *offset_p);
int gsgpu_fence_slab_init(void);
void gsgpu_fence_slab_fini(void);

//GS Registers
#define GSGPU_COMMAND				0x0
#define GSGPU_STATUS				0x4
#define GSGPU_ARGUMENT0				0x8
#define GSGPU_ARGUMENT1				0xc
#define GSGPU_RETURN0				0x10
#define GSGPU_RETURN1				0x14
#define GSGPU_GFX_CB_BASE_LO_OFFSET		0x18
#define GSGPU_GFX_CB_BASE_HI_OFFSET		0x1c
#define GSGPU_GFX_CB_SIZE_OFFSET		0x20
#define GSGPU_GFX_CB_WPTR_OFFSET		0x24
#define GSGPU_GFX_CB_RPTR_OFFSET		0x28
#define GSGPU_XDMA_CB_BASE_LO_OFFSET		0x2c
#define GSGPU_XDMA_CB_BASE_HI_OFFSET		0x30
#define GSGPU_XDMA_CB_SIZE_OFFSET		0x34
#define GSGPU_XDMA_CB_WPTR_OFFSET		0x38
#define GSGPU_XDMA_CB_RPTR_OFFSET		0x3c
#define GSGPU_INT_CB_BASE_LO_OFFSET		0x40
#define GSGPU_INT_CB_BASE_HI_OFFSET		0x44
#define GSGPU_INT_CB_SIZE_OFFSET		0x48
#define GSGPU_INT_CB_WPTR_OFFSET		0x4c
#define GSGPU_INT_CB_RPTR_OFFSET		0x50
/* reserved 0x54 ~ 0x74 */
#define GSGPU_RESERVE_START_OFFSET		0x54
#define GSGPU_RESERVE_END_OFFSET		0x74
#define GSGPU_FW_VERSION_OFFSET			0x78
#define GSGPU_HW_FEATURE_OFFSET			0x7c

#define GSGPU_EC_CTRL				0x80
#define GSGPU_EC_INT				0x84
#define GSGPU_HOST_INT				0x88
#define GSGPU_HWINF				0x8c
#define GSGPU_FREQ_SCALE			0x9c

#define GSGPU_FW_WPORT				0xf0
#define GSGPU_FW_WPTR				0xf4
#define GSGPU_FW_CKSUM				0xf8

//GS Commands
#define GSCMD(cmd, subcmd)	(((cmd) & 0xFF)  | ((subcmd) & 0xFF) << 8)
#define GSCMDi(cmd, subcmd, i)	(((cmd) & 0xFF)  | ((subcmd) & 0xFF) << 8 | ((i) & 0xF) << 16)

#define GSCMD_HALT		0x00000000 // stop jobs in GPU, return
#define GSCMD_PING_5A		0x00000001 // return 5a5a5a5a in status
#define GSCMD_PING_A5		0x00000002 // return a5a5a5a5 in status
#define GSCMD_LOOP_DRAM		0x00000003 // loop through DRAM
#define GSCMD_LOOP_SSRV		0x00000004 // loop through SSRV
#define GSCMD_START		0x00000005 // start processing command buffer
#define GSCMD_STOP		0x00000006 // stop processing command buffer
#define GSCMD_SYNC		0x00000007 // wait pipeline empty
#define GSCMD_MMU		0x00000008 // mmu related op
#define GSCMD_SETREG		0x00000009 // internal reg op
#define GSCMD_PIPE		0x0000000A // op pipeline
#define GSCMD_ZIP		0x0000000B // op zip
#define GSCMD_PIPE_FLUSH	1 // op pipeline
#define GSCMD_FREQ		0x0000000C

#define GSCMD_STS_NULL		0x00000000
#define GSCMD_STS_BOOT		0xB007B007 // BOOT
#define GSCMD_STS_DONE		0xD02ED02E // DONE
#define GSCMD_STS_RUN		0xFFFF0000 // RUNING, lower 16bit can store total command count

#define EC_CTRL_RUN		0x01
#define EC_CTRL_STOP		0x00

//GS Packets
#define GSPKT(op, n)	(((op) & 0xFF) | ((n) & 0xFFFF) << 16)

#define	GSPKT_NOP			0x80
#define	GSPKT_WRITE			0x81
#define GSPKT_INDIRECT			0x82
#define GSPKT_FENCE			0x83
#define GSPKT_TRAP			0x84
#define GSPKT_POLL			0x85
#define 	POLL_CONDITION(x)		((x) << 8)
		/* 0 - true
		 * 1 - <
		 * 2 - <=
		 * 3 - ==
		 * 4 - !=
		 * 5 - >=
		 * 6 - >
		 */
#define 	POLL_REG_MEM(x)			((x) << 12)
		/* 0 - reg
		 * 1 - mem
		 */
#define		POLL_TIMES_INTERVAL(t, i)	((t) << 16 | (i))
#define GSPKT_WPOLL			0x86
#define	GSPKT_READ			0x87

//DRAW 0x89
#define GSPKT_VM_BIND			0x8A

#define GSPKT_XDMA_COPY			0xc0

/* 0 - register
 * 1 - memory
 */
#define	READ_SRC_SEL(x)			((x) << 9)
#define	WRITE_DST_SEL(x)		((x) << 8)
#define	WRITE_WAIT			(1 << 15)

/*
 * IRQS.
 */

struct gsgpu_flip_work {
	struct delayed_work		flip_work;
	struct work_struct		unpin_work;
	struct gsgpu_device		*adev;
	int				crtc_id;
	u32				target_vblank;
	uint64_t			base;
	struct drm_pending_vblank_event *event;
	struct gsgpu_bo		*old_abo;
	struct dma_fence		*excl;
	unsigned			shared_count;
	struct dma_fence		**shared;
	struct dma_fence_cb		cb;
	bool				async;
};


/*
 * CP & rings.
 */

struct gsgpu_ib {
	struct gsgpu_sa_bo		*sa_bo;
	uint32_t			length_dw;
	uint64_t			gpu_addr;
	uint32_t			*ptr;
	uint32_t			flags;
};

extern const struct drm_sched_backend_ops gsgpu_sched_ops;

/*
 * Queue manager
 */
struct gsgpu_queue_mapper {
	int 		hw_ip;
	struct mutex	lock;
	/* protected by lock */
	struct gsgpu_ring *queue_map[GSGPU_MAX_RINGS];
};

struct gsgpu_queue_mgr {
	struct gsgpu_queue_mapper mapper[GSGPU_MAX_IP_NUM];
};

int gsgpu_queue_mgr_init(struct gsgpu_device *adev,
			 struct gsgpu_queue_mgr *mgr);
int gsgpu_queue_mgr_fini(struct gsgpu_device *adev,
			 struct gsgpu_queue_mgr *mgr);
int gsgpu_queue_mgr_map(struct gsgpu_device *adev,
			struct gsgpu_queue_mgr *mgr,
			u32 hw_ip, u32 instance, u32 ring,
			struct gsgpu_ring **out_ring);

/*
 * context related structures
 */

struct gsgpu_ctx_ring {
	uint64_t		sequence;
	struct dma_fence	**fences;
	struct drm_sched_entity	entity;
};

struct gsgpu_ctx {
	struct kref		refcount;
	struct gsgpu_device    *adev;
	struct gsgpu_queue_mgr queue_mgr;
	unsigned		reset_counter;
	unsigned        reset_counter_query;
	uint32_t		vram_lost_counter;
	spinlock_t		ring_lock;
	struct dma_fence	**fences;
	struct gsgpu_ctx_ring	rings[GSGPU_MAX_RINGS];
	bool			preamble_presented;
	enum drm_sched_priority init_priority;
	enum drm_sched_priority override_priority;
	struct mutex            lock;
	atomic_t	guilty;
};

struct gsgpu_ctx_mgr {
	struct gsgpu_device	*adev;
	struct mutex		lock;
	/* protected by lock */
	struct idr		ctx_handles;
};

struct gsgpu_ctx *gsgpu_ctx_get(struct gsgpu_fpriv *fpriv, uint32_t id);
int gsgpu_ctx_put(struct gsgpu_ctx *ctx);

int gsgpu_ctx_add_fence(struct gsgpu_ctx *ctx, struct gsgpu_ring *ring,
			struct dma_fence *fence, uint64_t *seq);
struct dma_fence *gsgpu_ctx_get_fence(struct gsgpu_ctx *ctx,
				      struct gsgpu_ring *ring, uint64_t seq);
void gsgpu_ctx_priority_override(struct gsgpu_ctx *ctx,
				 enum drm_sched_priority priority);

int gsgpu_ctx_ioctl(struct drm_device *dev, void *data,
		    struct drm_file *filp);

int gsgpu_ctx_wait_prev_fence(struct gsgpu_ctx *ctx, unsigned ring_id);

void gsgpu_ctx_mgr_init(struct gsgpu_ctx_mgr *mgr);
void gsgpu_ctx_mgr_entity_fini(struct gsgpu_ctx_mgr *mgr);
void gsgpu_ctx_mgr_entity_flush(struct gsgpu_ctx_mgr *mgr);
void gsgpu_ctx_mgr_fini(struct gsgpu_ctx_mgr *mgr);


/*
 * file private structure
 */

struct gsgpu_fpriv {
	struct gsgpu_vm	vm;
	struct gsgpu_bo_va	*prt_va;
	struct gsgpu_bo_va	*csa_va;
	struct mutex		bo_list_lock;
	struct idr		bo_list_handles;
	struct gsgpu_ctx_mgr	ctx_mgr;
};

struct gsgpu_rlc_funcs {
	void (*enter_safe_mode)(struct gsgpu_device *adev);
	void (*exit_safe_mode)(struct gsgpu_device *adev);
};

struct gsgpu_rlc {
	/* for power gating */
	struct gsgpu_bo	*save_restore_obj;
	uint64_t		save_restore_gpu_addr;
	volatile uint32_t	*sr_ptr;
	const u32               *reg_list;
	u32                     reg_list_size;
	/* for clear state */
	struct gsgpu_bo	*clear_state_obj;
	uint64_t		clear_state_gpu_addr;
	volatile uint32_t	*cs_ptr;
	const struct cs_section_def   *cs_data;
	u32                     clear_state_size;
	/* for cp tables */
	struct gsgpu_bo	*cp_table_obj;
	uint64_t		cp_table_gpu_addr;
	volatile uint32_t	*cp_table_ptr;
	u32                     cp_table_size;

	/* safe mode for updating CG/PG state */
	bool in_safe_mode;
	const struct gsgpu_rlc_funcs *funcs;

	/* for firmware data */
	u32 save_and_restore_offset;
	u32 clear_state_descriptor_offset;
	u32 avail_scratch_ram_locations;
	u32 reg_restore_list_size;
	u32 reg_list_format_start;
	u32 reg_list_format_separate_start;
	u32 starting_offsets_start;
	u32 reg_list_format_size_bytes;
	u32 reg_list_size_bytes;
	u32 reg_list_format_direct_reg_list_length;
	u32 save_restore_list_cntl_size_bytes;
	u32 save_restore_list_gpm_size_bytes;
	u32 save_restore_list_srm_size_bytes;

	u32 *register_list_format;
	u32 *register_restore;
	u8 *save_restore_list_cntl;
	u8 *save_restore_list_gpm;
	u8 *save_restore_list_srm;

	bool is_rlc_v2_1;
};

/*
 * GFX configurations
 */
#define GSGPU_GFX_MAX_SE 4
#define GSGPU_GFX_MAX_SH_PER_SE 2

struct gsgpu_rb_config {
	uint32_t rb_backend_disable;
	uint32_t user_rb_backend_disable;
	uint32_t raster_config;
	uint32_t raster_config_1;
};

struct gb_addr_config {
	uint16_t pipe_interleave_size;
	uint8_t num_pipes;
	uint8_t max_compress_frags;
	uint8_t num_banks;
	uint8_t num_se;
	uint8_t num_rb_per_se;
};

struct gsgpu_gfx_config {
	unsigned max_shader_engines;
	unsigned max_tile_pipes;
	unsigned max_cu_per_sh;
	unsigned max_sh_per_se;
	unsigned max_backends_per_se;
	unsigned max_texture_channel_caches;
	unsigned max_gprs;
	unsigned max_gs_threads;
	unsigned max_hw_contexts;
	unsigned sc_prim_fifo_size_frontend;
	unsigned sc_prim_fifo_size_backend;
	unsigned sc_hiz_tile_fifo_size;
	unsigned sc_earlyz_tile_fifo_size;

	unsigned num_tile_pipes;
	unsigned backend_enable_mask;
	unsigned mem_max_burst_length_bytes;
	unsigned mem_row_size_in_kb;
	unsigned shader_engine_tile_size;
	unsigned num_gpus;
	unsigned multi_gpu_tile_size;
	unsigned mc_arb_ramcfg;
	unsigned gb_addr_config;
	unsigned num_rbs;
	unsigned gs_vgt_table_depth;
	unsigned gs_prim_buffer_depth;

	uint32_t tile_mode_array[32];
	uint32_t macrotile_mode_array[16];

	struct gb_addr_config gb_addr_config_fields;
	struct gsgpu_rb_config rb_config[GSGPU_GFX_MAX_SE][GSGPU_GFX_MAX_SH_PER_SE];

	/* gfx configure feature */
	uint32_t double_offchip_lds_buf;
	/* cached value of DB_DEBUG2 */
	uint32_t db_debug2;
};

struct gsgpu_cu_info {
	uint32_t simd_per_cu;
	uint32_t max_waves_per_simd;
	uint32_t wave_front_size;
	uint32_t max_scratch_slots_per_cu;
	uint32_t lds_size;

	/* total active CU number */
	uint32_t number;
	uint32_t ao_cu_mask;
	uint32_t ao_cu_bitmap[4][4];
	uint32_t bitmap[4][4];
};

struct gsgpu_gfx_funcs {
	/* get the gpu clock counter */
	uint64_t (*get_gpu_clock_counter)(struct gsgpu_device *adev);
	void (*read_wave_data)(struct gsgpu_device *adev, uint32_t simd, uint32_t wave, uint32_t *dst, int *no_fields);
	void (*read_wave_vgprs)(struct gsgpu_device *adev, uint32_t simd, uint32_t wave, uint32_t thread, uint32_t start, uint32_t size, uint32_t *dst);
	void (*read_wave_sgprs)(struct gsgpu_device *adev, uint32_t simd, uint32_t wave, uint32_t start, uint32_t size, uint32_t *dst);
	void (*select_me_pipe_q)(struct gsgpu_device *adev, u32 me, u32 pipe, u32 queue);
};

struct sq_work {
	struct work_struct	work;
	unsigned ih_data;
};

struct gsgpu_gfx {
	struct mutex			gpu_clock_mutex;
	struct gsgpu_gfx_config		config;
	struct gsgpu_rlc		rlc;
	const struct firmware		*cp_fw;	/* CP firmware */
	uint32_t			cp_fw_version;
	uint32_t			cp_feature_version;
	struct gsgpu_ring		gfx_ring[GSGPU_MAX_GFX_RINGS];
	unsigned			num_gfx_rings;
	struct gsgpu_irq_src		eop_irq;
	struct gsgpu_irq_src		priv_reg_irq;
	struct gsgpu_irq_src		priv_inst_irq;
	struct gsgpu_irq_src		cp_ecc_error_irq;

	/* gfx status */
	uint32_t			gfx_current_status;
	/* ce ram size*/
	unsigned			ce_ram_size;
	struct gsgpu_cu_info		cu_info;
	const struct gsgpu_gfx_funcs	*funcs;

	/* s3/s4 mask */
	bool                            in_suspend;
};

int gsgpu_ib_get(struct gsgpu_device *adev, struct gsgpu_vm *vm,
		  unsigned size, struct gsgpu_ib *ib);
void gsgpu_ib_free(struct gsgpu_device *adev, struct gsgpu_ib *ib,
		    struct dma_fence *f);
int gsgpu_ib_schedule(struct gsgpu_ring *ring, unsigned num_ibs,
		       struct gsgpu_ib *ibs, struct gsgpu_job *job,
		       struct dma_fence **f);
int gsgpu_ib_pool_init(struct gsgpu_device *adev);
void gsgpu_ib_pool_fini(struct gsgpu_device *adev);
int gsgpu_ib_ring_tests(struct gsgpu_device *adev);

/*
 * CS.
 */
struct gsgpu_cs_chunk {
	uint32_t		chunk_id;
	uint32_t		length_dw;
	void			*kdata;
};

struct gsgpu_cs_parser {
	struct gsgpu_device	*adev;
	struct drm_file		*filp;
	struct gsgpu_ctx	*ctx;

	/* chunks */
	unsigned		nchunks;
	struct gsgpu_cs_chunk	*chunks;

	/* scheduler job object */
	struct gsgpu_job	*job;
	struct gsgpu_ring	*ring;

	/* buffer objects */
	struct ww_acquire_ctx		ticket;
	struct gsgpu_bo_list		*bo_list;
	struct gsgpu_bo_list_entry	vm_pd;
	struct list_head		validated;
	struct dma_fence		*fence;
	uint64_t			bytes_moved_threshold;
	uint64_t			bytes_moved_vis_threshold;
	uint64_t			bytes_moved;
	uint64_t			bytes_moved_vis;
	struct gsgpu_bo_list_entry	*evictable;

	/* user fence */
	struct gsgpu_bo_list_entry	uf_entry;

	unsigned num_post_dep_syncobjs;
	struct drm_syncobj **post_dep_syncobjs;
};

static inline u32 gsgpu_get_ib_value(struct gsgpu_cs_parser *p,
				      uint32_t ib_idx, int idx)
{
	return p->job->ibs[ib_idx].ptr[idx];
}

static inline void gsgpu_set_ib_value(struct gsgpu_cs_parser *p,
				       uint32_t ib_idx, int idx,
				       uint32_t value)
{
	p->job->ibs[ib_idx].ptr[idx] = value;
}

/*
 * Writeback
 */
#define GSGPU_MAX_WB 128	/* Reserve at most 128 WB slots for gsgpu-owned rings. */

struct gsgpu_wb {
	struct gsgpu_bo	*wb_obj;
	volatile uint32_t	*wb;
	uint64_t		gpu_addr;
	u32			num_wb;	/* Number of wb slots actually reserved for gsgpu. */
	unsigned long		used[DIV_ROUND_UP(GSGPU_MAX_WB, BITS_PER_LONG)];
};

int gsgpu_device_wb_get(struct gsgpu_device *adev, u32 *wb);
void gsgpu_device_wb_free(struct gsgpu_device *adev, u32 wb);

/*
 * XDMA
 */
struct gsgpu_xdma_instance {
	/* SDMA firmware */
	const struct firmware	*fw;
	uint32_t		fw_version;
	uint32_t		feature_version;

	struct gsgpu_ring	ring;
	bool			burst_nop;
};

struct gsgpu_xdma {
	struct gsgpu_xdma_instance instance[GSGPU_MAX_XDMA_INSTANCES];
	struct gsgpu_irq_src	trap_irq;
	struct gsgpu_irq_src	illegal_inst_irq;
	int			num_instances;
};

/*
 * Firmware
 */
struct gsgpu_firmware {
	struct gsgpu_bo *fw_buf;
	unsigned int fw_size;
	unsigned int max_ucodes;
	struct gsgpu_bo *rbuf;
	struct mutex mutex;

	/* gpu info firmware data pointer */
	const struct firmware *gpu_info_fw;

	void *fw_buf_ptr;
	uint64_t fw_buf_mc;
};

/*
 * Benchmarking
 */
void gsgpu_benchmark(struct gsgpu_device *adev, int test_number);


/*
 * Testing
 */
void gsgpu_test_moves(struct gsgpu_device *adev);

/*
 * ASIC specific functions.
 */
struct gsgpu_asic_funcs {
	bool (*read_bios_from_rom)(struct gsgpu_device *adev,
				   u8 *bios, u32 length_bytes);
	int (*read_register)(struct gsgpu_device *adev, u32 se_num,
			     u32 sh_num, u32 reg_offset, u32 *value);
	void (*set_vga_state)(struct gsgpu_device *adev, bool state);
	int (*reset)(struct gsgpu_device *adev);
	/* get the reference clock */
	u32 (*get_clk)(struct gsgpu_device *adev);
	/* static power management */
	int (*get_pcie_lanes)(struct gsgpu_device *adev);
	void (*set_pcie_lanes)(struct gsgpu_device *adev, int lanes);
	/* check if the asic needs a full reset of if soft reset will work */
	bool (*need_full_reset)(struct gsgpu_device *adev);
};

/*
 * IOCTL.
 */
int gsgpu_gem_create_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *filp);
int gsgpu_bo_list_ioctl(struct drm_device *dev, void *data,
				struct drm_file *filp);

int gsgpu_gem_info_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *filp);
int gsgpu_gem_userptr_ioctl(struct drm_device *dev, void *data,
			struct drm_file *filp);
int gsgpu_gem_mmap_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *filp);
int gsgpu_gem_wait_idle_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *filp);
int gsgpu_gem_va_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *filp);
int gsgpu_gem_op_ioctl(struct drm_device *dev, void *data,
			struct drm_file *filp);
int gsgpu_cs_ioctl(struct drm_device *dev, void *data, struct drm_file *filp);
int gsgpu_cs_fence_to_handle_ioctl(struct drm_device *dev, void *data,
				    struct drm_file *filp);
int gsgpu_cs_wait_ioctl(struct drm_device *dev, void *data, struct drm_file *filp);
int gsgpu_cs_wait_fences_ioctl(struct drm_device *dev, void *data,
				struct drm_file *filp);

int gsgpu_gem_metadata_ioctl(struct drm_device *dev, void *data,
				struct drm_file *filp);

int gsgpu_hw_sema_op_ioctl(struct drm_device *dev, void *data, struct drm_file *filp);

/* VRAM scratch page for HDP bug, default vram page */
struct gsgpu_vram_scratch {
	struct gsgpu_bo		*robj;
	volatile uint32_t		*ptr;
	u64				gpu_addr;
};

/*
 * Firmware VRAM reservation
 */
struct gsgpu_fw_vram_usage {
	u64 start_offset;
	u64 size;
	struct gsgpu_bo *reserved_bo;
	void *va;
};

/*
 * Core structure, functions and helpers.
 */
typedef uint32_t (*gsgpu_rreg_t)(struct gsgpu_device*, uint32_t);
typedef void (*gsgpu_wreg_t)(struct gsgpu_device*, uint32_t, uint32_t);

typedef uint32_t (*gsgpu_block_rreg_t)(struct gsgpu_device*, uint32_t, uint32_t);
typedef void (*gsgpu_block_wreg_t)(struct gsgpu_device*, uint32_t, uint32_t, uint32_t);

#define GSGPU_RESET_MAGIC_NUM 64
struct gsgpu_device {
	struct device			*dev;
	struct drm_device		*ddev;
	struct pci_dev			*pdev;
	struct pci_dev			*loongson_dc;
	u8				dc_revision;
	u8				chip;

	/* ASIC */
	enum gsgpu_family_type		family_type;
	uint32_t			family;
	unsigned long			flags;
	int				usec_timeout;
	const struct gsgpu_asic_funcs	*asic_funcs;
	bool				shutdown;
	bool				need_dma32;
	bool				need_swiotlb;
	bool				accel_working;
	struct work_struct		reset_work;
	struct notifier_block		acpi_nb;
	struct gsgpu_debugfs		debugfs[GSGPU_DEBUGFS_MAX_COMPONENTS];
	unsigned			debugfs_count;
#if defined(CONFIG_DEBUG_FS)
	struct dentry			*debugfs_regs[GSGPU_DEBUGFS_MAX_COMPONENTS];
#endif
	struct mutex			srbm_mutex;
	/* GRBM index mutex. Protects concurrent access to GRBM index */
	struct mutex                    grbm_idx_mutex;
	struct dev_pm_domain		vga_pm_domain;
	bool				have_disp_power_ref;

	/* BIOS */
	bool				is_atom_fw;
	uint8_t				*bios;
	uint32_t			bios_size;
	struct gsgpu_bo		*stolen_vga_memory;
	uint32_t			bios_scratch_reg_offset;
	uint32_t			bios_scratch[GSGPU_BIOS_NUM_SCRATCH];

	/* Register mmio */
	resource_size_t			rmmio_base;
	resource_size_t			rmmio_size;
	void __iomem			*rmmio;

	/* loongson dc mmio */
	resource_size_t			loongson_dc_rmmio_base;
	resource_size_t			loongson_dc_rmmio_size;
	void __iomem			*loongson_dc_rmmio;
	void __iomem			*io_base;

	/* protects concurrent MM_INDEX/DATA based register access */
	spinlock_t mmio_idx_lock;
	spinlock_t dc_mmio_lock;
	/* protects concurrent PCIE register access */
	spinlock_t pcie_idx_lock;

	/* protects concurrent se_cac register access */
	spinlock_t se_cac_idx_lock;
	gsgpu_rreg_t			se_cac_rreg;
	gsgpu_wreg_t			se_cac_wreg;

	/* clock/pll info */
	struct gsgpu_clock            clock;

	/* MC */
	struct gsgpu_gmc		gmc;
	struct gsgpu_gart		gart;
	struct gsgpu_zip_meta	zip_meta;
	dma_addr_t			dummy_page_addr;
	struct gsgpu_vm_manager	vm_manager;

	/* memory management */
	struct gsgpu_mman		mman;
	struct gsgpu_vram_scratch	vram_scratch;
	struct gsgpu_wb		wb;
	atomic64_t			num_bytes_moved;
	atomic64_t			num_evictions;
	atomic64_t			num_vram_cpu_page_faults;
	atomic_t			gpu_reset_counter;
	atomic_t			vram_lost_counter;

	struct gsgpu_hw_sema_mgr hw_sema_mgr;

	/* data for buffer migration throttling */
	struct {
		spinlock_t		lock;
		s64			last_update_us;
		s64			accum_us; /* accumulated microseconds */
		s64			accum_us_vis; /* for visible VRAM */
		u32			log2_max_MBps;
	} mm_stats;

	struct gsgpu_dc			*dc;
	struct gsgpu_mode_info		mode_info;
	struct gsgpu_dc_i2c		*i2c[2];
	struct work_struct		hotplug_work;
	struct gsgpu_irq_src		vsync_irq;
	struct gsgpu_irq_src		i2c_irq;
	struct gsgpu_irq_src		hpd_irq;

	/* rings */
	u64				fence_context;
	unsigned			num_rings;
	struct gsgpu_ring		*rings[GSGPU_MAX_RINGS];
	bool				ib_pool_ready;
	struct gsgpu_sa_manager	ring_tmp_bo;

	/* interrupts */
	struct gsgpu_irq		irq;

	/* HPD */
	int				vga_hpd_status;

	u32				cg_flags;
	u32				pg_flags;

	/* gfx */
	struct gsgpu_gfx		gfx;

	/* xdma */
	struct gsgpu_xdma		xdma;

	/* firmwares */
	struct gsgpu_firmware		firmware;

	struct gsgpu_ip_block          ip_blocks[GSGPU_MAX_IP_NUM];
	int				num_ip_blocks;
	struct mutex	notifier_lock;

	/* tracking pinned memory */
	atomic64_t vram_pin_size;
	atomic64_t visible_pin_size;
	atomic64_t gart_pin_size;

	/* delayed work_func for deferring clockgating during resume */
	struct delayed_work     late_init_work;

	//zl prior virt
	uint32_t			reg_val_offs;
	/* firmware VRAM reservation */
	struct gsgpu_fw_vram_usage fw_vram_usage;

	/* link all shadow bo */
	struct list_head                shadow_list;
	struct mutex                    shadow_list_lock;
	/* keep an lru list of rings by HW IP */
	struct list_head		ring_lru_list;
	spinlock_t			ring_lru_list_lock;

	/* record hw reset is performed */
	bool has_hw_reset;
	u8				reset_magic[GSGPU_RESET_MAGIC_NUM];

	/* record last mm index being written through WREG32*/
	unsigned long last_mm_index;
	bool                            in_gpu_reset;
	struct mutex  lock_reset;

	struct loongson_vbios *vbios;
	bool cursor_showed;
	bool clone_mode;
	int cursor_crtc_id;
	bool inited;
};

/* TODO: We need to embed drm_device into gsgpu_device. Also, these should
 * probably be called drm_to_gsdev, but I can't be bothered to change all
 * adev to gsdev in the code. */
static inline struct gsgpu_device *drm_to_adev(struct drm_device *ddev)
{
        return ddev->dev_private;
}

static inline struct drm_device *adev_to_drm(struct gsgpu_device *adev)
{
        return adev->ddev;
}

static inline struct gsgpu_device *gsgpu_ttm_adev(struct ttm_device *bdev)
{
	return container_of(bdev, struct gsgpu_device, mman.bdev);
}

int gsgpu_device_init(struct gsgpu_device *adev,
		       struct drm_device *ddev,
		       struct pci_dev *pdev,
		       uint32_t flags);
void gsgpu_device_fini(struct gsgpu_device *adev);
int gsgpu_gpu_wait_for_idle(struct gsgpu_device *adev);

uint64_t gsgpu_cmd_exec(struct gsgpu_device *adev, uint32_t cmd,
			uint32_t arg0, uint32_t arg1);
uint32_t gsgpu_mm_rreg(struct gsgpu_device *adev, uint32_t reg,
			uint32_t acc_flags);
void gsgpu_mm_wreg(struct gsgpu_device *adev, uint32_t reg, uint32_t v);
void gsgpu_mm_wreg8(struct gsgpu_device *adev, uint32_t offset, uint8_t value);
uint8_t gsgpu_mm_rreg8(struct gsgpu_device *adev, uint32_t offset);

/*
 * Registers read & write functions.
 */

#define GSGPU_REGS_IDX       (1<<0)
#define GSGPU_REGS_NO_KIQ    (1<<1)

#define RREG32_NO_KIQ(reg) gsgpu_mm_rreg(adev, (reg), GSGPU_REGS_NO_KIQ)
#define WREG32_NO_KIQ(reg, v) gsgpu_mm_wreg(adev, (reg), (v))

#define RREG8(reg) gsgpu_mm_rreg8(adev, (reg))
#define WREG8(reg, v) gsgpu_mm_wreg8(adev, (reg), (v))

#define RREG32(reg) gsgpu_mm_rreg(adev, (reg), 0)
#define RREG32_IDX(reg) gsgpu_mm_rreg(adev, (reg), GSGPU_REGS_IDX)
#define DREG32(reg) printk(KERN_INFO "REGISTER: " #reg " : 0x%08X\n", gsgpu_mm_rreg(adev, (reg), 0))
#define WREG32(reg, v) gsgpu_mm_wreg(adev, (reg), (v))
#define WREG32_IDX(reg, v) gsgpu_mm_wreg(adev, (reg), (v))
#define REG_SET(FIELD, v) (((v) << FIELD##_SHIFT) & FIELD##_MASK)
#define REG_GET(FIELD, v) (((v) << FIELD##_SHIFT) & FIELD##_MASK)
#define RREG32_SE_CAC(reg) adev->se_cac_rreg(adev, (reg))
#define WREG32_SE_CAC(reg, v) adev->se_cac_wreg(adev, (reg), (v))
#define WREG32_P(reg, val, mask)				\
	do {							\
		uint32_t tmp_ = RREG32(reg);			\
		tmp_ &= (mask);					\
		tmp_ |= ((val) & ~(mask));			\
		WREG32(reg, tmp_);				\
	} while (0)
#define WREG32_AND(reg, and) WREG32_P(reg, 0, and)
#define WREG32_OR(reg, or) WREG32_P(reg, or, ~(or))
#define WREG32_PLL_P(reg, val, mask)				\
	do {							\
		uint32_t tmp_ = RREG32_PLL(reg);		\
		tmp_ &= (mask);					\
		tmp_ |= ((val) & ~(mask));			\
		WREG32_PLL(reg, tmp_);				\
	} while (0)
#define DREG32_SYS(sqf, adev, reg) seq_printf((sqf), #reg " : 0x%08X\n", gsgpu_mm_rreg((adev), (reg), false))

#define REG_FIELD_SHIFT(reg, field) reg##__##field##__SHIFT
#define REG_FIELD_MASK(reg, field) reg##__##field##_MASK

#define REG_SET_FIELD(orig_val, reg, field, field_val)			\
	(((orig_val) & ~REG_FIELD_MASK(reg, field)) |			\
	 (REG_FIELD_MASK(reg, field) & ((field_val) << REG_FIELD_SHIFT(reg, field))))

#define REG_GET_FIELD(value, reg, field)				\
	(((value) & REG_FIELD_MASK(reg, field)) >> REG_FIELD_SHIFT(reg, field))

#define WREG32_FIELD(reg, field, val)	\
	WREG32(mm##reg, (RREG32(mm##reg) & ~REG_FIELD_MASK(reg, field)) | (val) << REG_FIELD_SHIFT(reg, field))

#define WREG32_FIELD_OFFSET(reg, offset, field, val)	\
	WREG32(mm##reg + offset, (RREG32(mm##reg + offset) & ~REG_FIELD_MASK(reg, field)) | (val) << REG_FIELD_SHIFT(reg, field))

/*
 * BIOS helpers.
 */
#define RBIOS8(i) (adev->bios[i])
#define RBIOS16(i) (RBIOS8(i) | (RBIOS8((i)+1) << 8))
#define RBIOS32(i) ((RBIOS16(i)) | (RBIOS16((i)+2) << 16))

static inline struct gsgpu_xdma_instance *
gsgpu_get_xdma_instance(struct gsgpu_ring *ring)
{
	struct gsgpu_device *adev = ring->adev;
	int i;

	for (i = 0; i < adev->xdma.num_instances; i++)
		if (&adev->xdma.instance[i].ring == ring)
			break;

	if (i < GSGPU_MAX_XDMA_INSTANCES)
		return &adev->xdma.instance[i];
	else
		return NULL;
}

/*
 * ASICs macro.
 */
#define gsgpu_asic_set_vga_state(adev, state) ((adev)->asic_funcs->set_vga_state((adev), (state)))
#define gsgpu_asic_reset(adev) ((adev)->asic_funcs->reset((adev)))
#define gsgpu_asic_get_clk(adev) ((adev)->asic_funcs->get_clk((adev)))
#define gsgpu_get_pcie_lanes(adev) ((adev)->asic_funcs->get_pcie_lanes((adev)))
#define gsgpu_set_pcie_lanes(adev, l) ((adev)->asic_funcs->set_pcie_lanes((adev), (l)))
#define gsgpu_asic_read_bios_from_rom(adev, b, l) ((adev)->asic_funcs->read_bios_from_rom((adev), (b), (l)))
#define gsgpu_asic_read_register(adev, se, sh, offset, v) ((adev)->asic_funcs->read_register((adev), (se), (sh), (offset), (v)))
#define gsgpu_asic_need_full_reset(adev) ((adev)->asic_funcs->need_full_reset((adev)))
#define gsgpu_gmc_flush_gpu_tlb(adev, vmid) ((adev)->gmc.gmc_funcs->flush_gpu_tlb((adev), (vmid)))
#define gsgpu_gmc_emit_flush_gpu_tlb(r, vmid, addr) ((r)->adev->gmc.gmc_funcs->emit_flush_gpu_tlb((r), (vmid), (addr)))
#define gsgpu_gmc_emit_pasid_mapping(r, vmid, pasid) ((r)->adev->gmc.gmc_funcs->emit_pasid_mapping((r), (vmid), (pasid)))
#define gsgpu_gmc_set_pte_pde(adev, pt, idx, addr, flags) ((adev)->gmc.gmc_funcs->set_pte_pde((adev), (pt), (idx), (addr), (flags)))
#define gsgpu_gmc_get_vm_pde(adev, level, dst, flags) ((adev)->gmc.gmc_funcs->get_vm_pde((adev), (level), (dst), (flags)))
#define gsgpu_gmc_get_pte_flags(adev, flags) ((adev)->gmc.gmc_funcs->get_vm_pte_flags((adev), (flags)))
#define gsgpu_vm_copy_pte(adev, ib, pe, src, count) ((adev)->vm_manager.vm_pte_funcs->copy_pte((ib), (pe), (src), (count)))
#define gsgpu_vm_write_pte(adev, ib, pe, value, count, incr) ((adev)->vm_manager.vm_pte_funcs->write_pte((ib), (pe), (value), (count), (incr)))
#define gsgpu_vm_set_pte_pde(adev, ib, pe, addr, count, incr, flags) ((adev)->vm_manager.vm_pte_funcs->set_pte_pde((ib), (pe), (addr), (count), (incr), (flags)))
#define gsgpu_ring_parse_cs(r, p, ib) ((r)->funcs->parse_cs((p), (ib)))
#define gsgpu_ring_patch_cs_in_place(r, p, ib) ((r)->funcs->patch_cs_in_place((p), (ib)))
#define gsgpu_ring_test_ring(r) ((r)->funcs->test_ring((r)))
#define gsgpu_ring_test_ib(r, t) ((r)->funcs->test_ib((r), (t)))
#define gsgpu_ring_test_xdma(r, t) ((r)->funcs->test_xdma((r), (t)))
#define gsgpu_ring_get_rptr(r) ((r)->funcs->get_rptr((r)))
#define gsgpu_ring_get_wptr(r) ((r)->funcs->get_wptr((r)))
#define gsgpu_ring_set_wptr(r) ((r)->funcs->set_wptr((r)))
#define gsgpu_ring_emit_ib(r, ib, vmid, c) ((r)->funcs->emit_ib((r), (ib), (vmid), (c)))
#define gsgpu_ring_emit_pipeline_sync(r) ((r)->funcs->emit_pipeline_sync((r)))
#define gsgpu_ring_emit_vm_flush(r, vmid, addr) ((r)->funcs->emit_vm_flush((r), (vmid), (addr)))
#define gsgpu_ring_emit_fence(r, addr, seq, flags) ((r)->funcs->emit_fence((r), (addr), (seq), (flags)))
#define gsgpu_ring_emit_switch_buffer(r) ((r)->funcs->emit_switch_buffer((r)))
#define gsgpu_ring_emit_cntxcntl(r, d) ((r)->funcs->emit_cntxcntl((r), (d)))
#define gsgpu_ring_emit_rreg(r, d) ((r)->funcs->emit_rreg((r), (d)))
#define gsgpu_ring_emit_wreg(r, d, v) ((r)->funcs->emit_wreg((r), (d), (v)))
#define gsgpu_ring_emit_reg_wait(r, d, v, m) ((r)->funcs->emit_reg_wait((r), (d), (v), (m)))
#define gsgpu_ring_emit_reg_write_reg_wait(r, d0, d1, v, m) ((r)->funcs->emit_reg_write_reg_wait((r), (d0), (d1), (v), (m)))
#define gsgpu_ring_emit_tmz(r, b) ((r)->funcs->emit_tmz((r), (b)))
#define gsgpu_ring_pad_ib(r, ib) ((r)->funcs->pad_ib((r), (ib)))
#define gsgpu_ring_init_cond_exec(r) ((r)->funcs->init_cond_exec((r)))
#define gsgpu_ring_patch_cond_exec(r, o) ((r)->funcs->patch_cond_exec((r), (o)))
#define gsgpu_ih_get_wptr(adev) ((adev)->irq.ih_funcs->get_wptr((adev)))
#define gsgpu_ih_prescreen_iv(adev) ((adev)->irq.ih_funcs->prescreen_iv((adev)))
#define gsgpu_ih_decode_iv(adev, iv) ((adev)->irq.ih_funcs->decode_iv((adev), (iv)))
#define gsgpu_ih_set_rptr(adev) ((adev)->irq.ih_funcs->set_rptr((adev)))
#define gsgpu_display_vblank_get_counter(adev, crtc) ((adev)->mode_info.funcs->vblank_get_counter((adev), (crtc)))
#define gsgpu_display_backlight_set_level(adev, e, l) ((adev)->mode_info.funcs->backlight_set_level((e), (l)))
#define gsgpu_display_backlight_get_level(adev, e) ((adev)->mode_info.funcs->backlight_get_level((e)))
#define gsgpu_display_hpd_sense(adev, h) ((adev)->mode_info.funcs->hpd_sense((adev), (h)))
#define gsgpu_display_hpd_set_polarity(adev, h) ((adev)->mode_info.funcs->hpd_set_polarity((adev), (h)))
#define gsgpu_display_page_flip(adev, crtc, base, async) ((adev)->mode_info.funcs->page_flip((adev), (crtc), (base), (async)))
#define gsgpu_display_page_flip_get_scanoutpos(adev, crtc, vbl, pos) ((adev)->mode_info.funcs->page_flip_get_scanoutpos((adev), (crtc), (vbl), (pos)))
#define gsgpu_emit_copy_buffer(adev, ib, s, d, b) ((adev)->mman.buffer_funcs->emit_copy_buffer((ib),  (s), (d), (b)))
#define gsgpu_emit_fill_buffer(adev, ib, s, d, b) ((adev)->mman.buffer_funcs->emit_fill_buffer((ib), (s), (d), (b)))
#define gsgpu_gfx_get_gpu_clock_counter(adev) ((adev)->gfx.funcs->get_gpu_clock_counter((adev)))
#define gsgpu_psp_check_fw_loading_status(adev, i) ((adev)->firmware.funcs->check_fw_loading_status((adev), (i)))
#define gsgpu_gfx_select_me_pipe_q(adev, me, pipe, q) ((adev)->gfx.funcs->select_me_pipe_q((adev), (me), (pipe), (q)))

/* Common functions */
int gsgpu_device_gpu_recover(struct gsgpu_device *adev,
			     struct gsgpu_job *job, bool force);
void gsgpu_device_pci_config_reset(struct gsgpu_device *adev);
bool gsgpu_device_need_post(struct gsgpu_device *adev);
void gsgpu_display_update_priority(struct gsgpu_device *adev);

void gsgpu_cs_report_moved_bytes(struct gsgpu_device *adev, u64 num_bytes,
				 u64 num_vis_bytes);
void gsgpu_device_vram_location(struct gsgpu_device *adev,
				struct gsgpu_gmc *mc, u64 base);
void gsgpu_device_gart_location(struct gsgpu_device *adev,
				struct gsgpu_gmc *mc);
int gsgpu_device_resize_fb_bar(struct gsgpu_device *adev);
void gsgpu_device_program_register_sequence(struct gsgpu_device *adev,
					    const u32 *registers,
					    const u32 array_size);

/*
 * KMS
 */
extern const struct drm_ioctl_desc gsgpu_ioctls_kms[];
extern const int gsgpu_max_kms_ioctl;

int gsgpu_driver_load_kms(struct drm_device *dev, unsigned long flags);
void gsgpu_driver_unload_kms(struct drm_device *dev);
void gsgpu_driver_lastclose_kms(struct drm_device *dev);
int gsgpu_driver_open_kms(struct drm_device *dev, struct drm_file *file_priv);
void gsgpu_driver_postclose_kms(struct drm_device *dev,
				struct drm_file *file_priv);
int gsgpu_device_ip_suspend(struct gsgpu_device *adev);
int gsgpu_device_suspend(struct drm_device *dev, bool suspend, bool fbcon);
int gsgpu_device_resume(struct drm_device *dev, bool resume, bool fbcon);
u32 gsgpu_get_vblank_counter_kms(struct drm_crtc *crtc);
long gsgpu_kms_compat_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg);

/*
 * functions used by gsgpu_encoder.c
 */
struct gsgpu_afmt_acr {
	u32 clock;

	int n_32khz;
	int cts_32khz;

	int n_44_1khz;
	int cts_44_1khz;

	int n_48khz;
	int cts_48khz;

};

struct gsgpu_afmt_acr gsgpu_afmt_acr(uint32_t clock);

/* gsgpu_acpi.c */
#if defined(CONFIG_ACPI)
int gsgpu_acpi_init(struct gsgpu_device *adev);
void gsgpu_acpi_fini(struct gsgpu_device *adev);
bool gsgpu_acpi_is_pcie_performance_request_supported(struct gsgpu_device *adev);
int gsgpu_acpi_pcie_performance_request(struct gsgpu_device *adev,
					u8 perf_req, bool advertise);
int gsgpu_acpi_pcie_notify_device_ready(struct gsgpu_device *adev);
#else
static inline int gsgpu_acpi_init(struct gsgpu_device *adev) { return 0; }
static inline void gsgpu_acpi_fini(struct gsgpu_device *adev) { }
#endif

int gsgpu_cs_find_mapping(struct gsgpu_cs_parser *parser,
			  uint64_t addr, struct gsgpu_bo **bo,
			  struct gsgpu_bo_va_mapping **mapping);

#include "gsgpu_object.h"
#endif
