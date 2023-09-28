#include <linux/delay.h>

#include "gsgpu.h"
#include "gsgpu_xdma.h"
#include "gsgpu_trace.h"
#include "gsgpu_common.h"
#include "gsgpu_irq.h"

static void xdma_set_ring_funcs(struct gsgpu_device *adev);
static void xdma_set_buffer_funcs(struct gsgpu_device *adev);
static void xdma_set_vm_pte_funcs(struct gsgpu_device *adev);
static void xdma_set_irq_funcs(struct gsgpu_device *adev);
static int xdma_set_pte_pde_test(struct gsgpu_ring *ring, long timeout);
static int xdma_copy_pte_test(struct gsgpu_ring *ring, long timeout);
static int xdma_fill_test(struct gsgpu_ring *ring, long timeout);

/*
 * xDMA
 * GSGPU has an asynchronous DMA engine called xDMA.
 * The engine is used for buffer moving.
 *
 * The programming model is very similar to the CP
 * (ring buffer, IBs, etc.), but xDMA has it's own
 * packet format. xDMA supports copying data, writing
 * embedded data, solid fills, and a number of other
 * things.  It also has support for tiling/detiling of
 * buffers.
 */

/**
 * xdma_ring_get_rptr - get the current read pointer
 *
 * @ring: gsgpu ring pointer
 *
 * Get the current rptr from the hardware ().
 */
static u64 xdma_ring_get_rptr(struct gsgpu_ring *ring)
{
	return ring->adev->wb.wb[ring->rptr_offs];
}

/**
 * xdma_ring_get_wptr - get the current write pointer
 *
 * @ring: gsgpu ring pointer
 *
 * Get the current wptr from the hardware ().
 */
static u64 xdma_ring_get_wptr(struct gsgpu_ring *ring)
{
	struct gsgpu_device *adev = ring->adev;

	return RREG32(GSGPU_XDMA_CB_WPTR_OFFSET);
}

/**
 * xdma_ring_set_wptr - commit the write pointer
 *
 * @ring: gsgpu ring pointer
 *
 * Write the wptr back to the hardware ().
 */
static void xdma_ring_set_wptr(struct gsgpu_ring *ring)
{
	struct gsgpu_device *adev = ring->adev;

	WREG32(GSGPU_XDMA_CB_WPTR_OFFSET, lower_32_bits(ring->wptr));
}

static void xdma_ring_insert_nop(struct gsgpu_ring *ring, u32 count)
{
	int i;

	for (i = 0; i < count; i++)
		gsgpu_ring_write(ring, ring->funcs->nop);
}

/**
 * xdma_ring_emit_ib - Schedule an IB on the DMA engine
 *
 * @ring: gsgpu ring pointer
 * @ib: IB object to schedule
 *
 * Schedule an IB in the DMA ring  .
 */
static void xdma_ring_emit_ib(struct gsgpu_ring *ring,
				   struct gsgpu_ib *ib,
				   unsigned vmid, bool ctx_switch)
{
	gsgpu_ring_write(ring, GSPKT(GSPKT_INDIRECT, 3));
	gsgpu_ring_write(ring, lower_32_bits(ib->gpu_addr));
	gsgpu_ring_write(ring, upper_32_bits(ib->gpu_addr));
	gsgpu_ring_write(ring, ib->length_dw | (vmid << 24));
}

/**
 * xdma_ring_emit_fence - emit a fence on the DMA ring
 *
 * @ring: gsgpu ring pointer
 * @fence: gsgpu fence object
 *
 * Add a DMA fence packet to the ring to write
 * the fence seq number and DMA trap packet to generate
 * an interrupt if needed  .
 */
static void xdma_ring_emit_fence(struct gsgpu_ring *ring, u64 addr, u64 seq,
				      unsigned flags)
{
	bool write64bit = flags & GSGPU_FENCE_FLAG_64BIT;
	bool int_sel = flags & GSGPU_FENCE_FLAG_INT;

	gsgpu_ring_write(ring, GSPKT(GSPKT_FENCE, write64bit ? 4 : 3)
			| (write64bit ? 1 << 9 : 0) | (int_sel ? 1 << 8 : 0));
	gsgpu_ring_write(ring, lower_32_bits(addr));
	gsgpu_ring_write(ring, upper_32_bits(addr));
	gsgpu_ring_write(ring, lower_32_bits(seq));
	if (write64bit)
		gsgpu_ring_write(ring, upper_32_bits(seq));
}

/**
 * xdma_gfx_stop - stop the gfx async dma engines
 *
 * @adev: gsgpu_device pointer
 *
 * Stop the gfx async dma ring buffers  .
 */
static void xdma_gfx_stop(struct gsgpu_device *adev)
{
	struct gsgpu_ring *xdma0 = &adev->xdma.instance[0].ring;
	struct gsgpu_ring *xdma1 = &adev->xdma.instance[1].ring;

	if ((adev->mman.buffer_funcs_ring == xdma0) ||
	    (adev->mman.buffer_funcs_ring == xdma1))
		gsgpu_ttm_set_buffer_funcs_status(adev, false);

	xdma0->ready = false;
	xdma1->ready = false;
}

/**
 * xdma_enable - stop the async dma engines
 *
 * @adev: gsgpu_device pointer
 * @enable: enable/disable the DMA MEs.
 *
 * Halt or unhalt the async dma engines  .
 */
static void xdma_enable(struct gsgpu_device *adev, bool enable)
{
	if (!enable) {
		xdma_gfx_stop(adev);
	}
}

/**
 * xdma_gfx_resume - setup and start the async dma engines
 *
 * @adev: gsgpu_device pointer
 *
 * Set up the gfx DMA ring buffers and enable them  .
 * Returns 0 for success, error for failure.
 */
static int xdma_gfx_resume(struct gsgpu_device *adev)
{
	struct gsgpu_ring *ring;
	int i;

	for (i = 0; i < adev->xdma.num_instances; i++) {
		ring = &adev->xdma.instance[i].ring;
		gsgpu_ring_clear_ring(ring);

		/* Initialize the ring buffer's read and write pointers */
		ring->wptr = 0;
		WREG32(GSGPU_XDMA_CB_WPTR_OFFSET, lower_32_bits(ring->wptr));

		/* set the RPTR */
		WREG32(GSGPU_XDMA_CB_RPTR_OFFSET, 0);

		WREG32(GSGPU_XDMA_CB_BASE_LO_OFFSET, lower_32_bits(ring->gpu_addr));
		WREG32(GSGPU_XDMA_CB_BASE_HI_OFFSET, upper_32_bits(ring->gpu_addr));

		ring->ready = true;
	}

	/* unhalt the MEs */
	xdma_enable(adev, true);

	for (i = 0; i < adev->xdma.num_instances; i++) {

		if (adev->mman.buffer_funcs_ring == ring)
			gsgpu_ttm_set_buffer_funcs_status(adev, true);
	}

	return 0;
}

/**
 * xdma_start - setup and start the async dma engines
 *
 * @adev: gsgpu_device pointer
 *
 * Set up the DMA engines and enable them  .
 * Returns 0 for success, error for failure.
 */
static int xdma_start(struct gsgpu_device *adev)
{
	int r;

	/* disable xdma engine before programing it */
	xdma_enable(adev, false);

	/* start the gfx rings and rlc compute queues */
	r = xdma_gfx_resume(adev);
	if (r)
		return r;

	return 0;
}

/**
 * xdma_ring_test_ring - simple async dma engine test
 *
 * @ring: gsgpu_ring structure holding ring information
 *
 * Test the DMA engine by writing using it to write an
 * value to memory.  .
 * Returns 0 for success, error for failure.
 */
static int xdma_ring_test_ring(struct gsgpu_ring *ring)
{
	struct gsgpu_device *adev = ring->adev;
	unsigned i;
	unsigned index;
	int r;
	u32 tmp;
	u64 gpu_addr;

	r = gsgpu_device_wb_get(adev, &index);
	if (r) {
		dev_err(adev->dev, "(%d) failed to allocate wb slot\n", r);
		return r;
	}

	gpu_addr = adev->wb.gpu_addr + (index * 4);
	tmp = 0xCAFEDEAD;
	adev->wb.wb[index] = cpu_to_le32(tmp);

	r = gsgpu_ring_alloc(ring, 4);
	if (r) {
		DRM_ERROR("gsgpu: dma failed to lock ring %d (%d).\n", ring->idx, r);
		gsgpu_device_wb_free(adev, index);
		return r;
	}

	gsgpu_ring_write(ring, GSPKT(GSPKT_WRITE, 3) | WRITE_DST_SEL(1) | WRITE_WAIT);
	gsgpu_ring_write(ring, lower_32_bits(gpu_addr));
	gsgpu_ring_write(ring, upper_32_bits(gpu_addr));
	gsgpu_ring_write(ring, 0xDEADBEEF);
	gsgpu_ring_commit(ring);

	for (i = 0; i < 4 * adev->usec_timeout; i++) {
		tmp = le32_to_cpu(adev->wb.wb[index]);
		if (tmp == 0xDEADBEEF)
			break;
		udelay(1);
	}

	if (i < adev->usec_timeout) {
		DRM_INFO("ring %s test on %d succeeded in %d usecs\n", ring->name, ring->idx, i);
		r = 0;
	} else {
		DRM_ERROR("gsgpu: ring %s %d test failed (0x%08X)\n", ring->name,
			  ring->idx, tmp);
		r = -EINVAL;
	}
	gsgpu_device_wb_free(adev, index);

	return r;
}

/**
 * xdma_ring_test_ib - test an IB on the DMA engine
 *
 * @ring: gsgpu_ring structure holding ring information
 *
 * Test a simple IB in the DMA ring  .
 * Returns 0 on success, error on failure.
 */
static int xdma_ring_test_ib(struct gsgpu_ring *ring, long timeout)
{
	struct gsgpu_device *adev = ring->adev;
	struct gsgpu_ib ib;
	struct dma_fence *f = NULL;
	unsigned index;
	u32 tmp = 0;
	u64 gpu_addr;
	long r;

	r = gsgpu_device_wb_get(adev, &index);
	if (r) {
		dev_err(adev->dev, "(%ld) failed to allocate wb slot\n", r);
		return r;
	}

	gpu_addr = adev->wb.gpu_addr + (index * 4);
	tmp = 0xCAFEDEAD;
	adev->wb.wb[index] = cpu_to_le32(tmp);
	memset(&ib, 0, sizeof(ib));
	r = gsgpu_ib_get(adev, NULL, 256, &ib);
	if (r) {
		DRM_ERROR("gsgpu: failed to get ib (%ld).\n", r);
		goto err0;
	}

	ib.ptr[0] = GSPKT(GSPKT_WRITE, 3) | WRITE_DST_SEL(1) | WRITE_WAIT;
	ib.ptr[1] = lower_32_bits(gpu_addr);
	ib.ptr[2] = upper_32_bits(gpu_addr);
	ib.ptr[3] = 0xDEADBEEF;
	ib.length_dw = 4;

	r = gsgpu_ib_schedule(ring, 1, &ib, NULL, &f);
	if (r)
		goto err1;

	r = dma_fence_wait_timeout(f, false, timeout);
	if (r == 0) {
		DRM_ERROR("gsgpu: %s IB test timed out\n", ring->name);
		r = -ETIMEDOUT;
		goto err1;
	} else if (r < 0) {
		DRM_ERROR("gsgpu: %s fence wait failed (%ld).\n", ring->name, r);
		goto err1;
	}
	tmp = le32_to_cpu(adev->wb.wb[index]);
	if (tmp == 0xDEADBEEF) {
		DRM_INFO("%s ib test on ring %d succeeded\n", ring->name, ring->idx);
		r = 0;
	} else {
		DRM_ERROR("gsgpu:%s ib test failed (0x%08X)\n", ring->name, tmp);
		r = -EINVAL;
	}

err1:
	gsgpu_ib_free(adev, &ib, NULL);
	dma_fence_put(f);
err0:
	gsgpu_device_wb_free(adev, index);

	return r;
}

/**
 * xdma_ring_test_xdma - test xdma on the DMA engine
 *
 * @ring: gsgpu_ring structure holding ring information
 *
 * Test a simple xdma in the DMA ring  .
 * Returns 0 on success, error on failure.
 */
static int xdma_ring_test_xdma(struct gsgpu_ring *ring, long timeout)
{
	int r;

	r = xdma_set_pte_pde_test(ring, timeout);
	if (r <= 0) {
		return r;
	}

	r = xdma_copy_pte_test(ring, timeout);
	if (r <= 0) {
		return r;
	}

	r = xdma_fill_test(ring, timeout);
	if (r > 0) {
		r = 0;
	}

	return r;
}

void xdma_ring_test_xdma_loop(struct gsgpu_ring *ring, long timeout)
{
	xdma_set_pte_pde_test(ring, timeout);
}

/**
 * xdma_vm_copy_pte - update PTEs by copying them from the GART
 *
 * @ib: indirect buffer to fill with commands
 * @pe: addr of the page entry
 * @src: src addr to copy from
 * @count: number of page entries to update
 *
 * Update PTEs by copying them from the GART using xDMA.
 */
static void xdma_vm_copy_pte(struct gsgpu_ib *ib,
				u64 pe, u64 src,
				unsigned count)
{
	struct gsgpu_bo *bo = ib->sa_bo->manager->bo;
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	uint32_t width, height;
	uint32_t dst_umap, src_umap;
	height = 1;
	width = count;
	dst_umap = (pe >= adev->gmc.vram_start && pe < adev->gmc.vram_end) ?
			   GSGPU_XDMA_FLAG_UMAP :
			   0;
	src_umap = (src >= adev->gmc.vram_start && src < adev->gmc.vram_end) ?
			   GSGPU_XDMA_FLAG_UMAP :
			   0;

	/* hardware limit 2^16 pixels per line */
	while (width >= 0x10000) {
		width = width / 2;
		height = height * 2;
	}

	/*XDMA COPY
	 * L2L : 1<<24
	 * RGBA16 : 1<<8
	 * */
	ib->ptr[ib->length_dw++] = GSPKT(GSPKT_XDMA_COPY, 8) | (0x1 << 24) | (1 << 8);
	ib->ptr[ib->length_dw++] =  (height << 16) | width;
	ib->ptr[ib->length_dw++] = lower_32_bits(src);
	ib->ptr[ib->length_dw++] = upper_32_bits(src) | src_umap;
	ib->ptr[ib->length_dw++] = lower_32_bits(pe);
	ib->ptr[ib->length_dw++] = upper_32_bits(pe) | dst_umap;
	ib->ptr[ib->length_dw++] = (width ? width : 1) * GSGPU_VM_PDE_PTE_BYTES;
	ib->ptr[ib->length_dw++] = (width ? width : 1) * GSGPU_VM_PDE_PTE_BYTES;
	ib->ptr[ib->length_dw++] = 0;
}

/**
 * xdma_vm_set_pte_pde - update the page tables using xDMA
 *
 * @ib: indirect buffer to fill with commands
 * @pe: addr of the page entry
 * @addr: dst addr to write into pe
 * @count: number of page entries to update
 * @incr: increase next addr by incr bytes
 * @flags: access flags
 *
 * Update the page tables using xDMA.
 */
static void xdma_vm_set_pte_pde(struct gsgpu_ib *ib, u64 pe,
				     u64 addr, unsigned count,
				     u32 incr, u64 flags)
{
	/* for physically contiguous pages (vram) */
	struct gsgpu_bo *bo = ib->sa_bo->manager->bo;
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	uint32_t width, height;
	uint32_t dst_umap, src_umap;

	dst_umap = (pe >= adev->gmc.vram_start && pe < adev->gmc.vram_end) ?
			   GSGPU_XDMA_FLAG_UMAP :
			   0;
	src_umap = (addr >= adev->gmc.vram_start && addr < adev->gmc.vram_end) ?
			   GSGPU_XDMA_FLAG_UMAP :
			   0;
	height = 1;

	/*RGBA16 == 8 bytes per pixles*/
	width = count;

	/* hardware limit 2^16 pixels per line */
	while (width >= 0x10000) {
		width = width / 2;
		height = height * 2;
	}

	/*XDMA COPY
	 * MEMSET : 7 << 24
	 * RGBA16 : 1 << 8
	 * 16K pf : 3 << 28
	 * */
	ib->ptr[ib->length_dw++] = GSPKT(GSPKT_XDMA_COPY, 8) | (0x7 << 24) | (1 << 8) | (3 << 28);
	ib->ptr[ib->length_dw++] =  (height << 16) | width;
	ib->ptr[ib->length_dw++] = lower_32_bits(addr | flags); /* value */
	ib->ptr[ib->length_dw++] = upper_32_bits(addr | flags) | src_umap;;
	ib->ptr[ib->length_dw++] = lower_32_bits(pe); /* dst addr */
	ib->ptr[ib->length_dw++] = upper_32_bits(pe) | dst_umap;
	ib->ptr[ib->length_dw++] = 0;
	ib->ptr[ib->length_dw++] = (width ? width : 1) * GSGPU_VM_PDE_PTE_BYTES;
	ib->ptr[ib->length_dw++] = 0;
}

/**
 * xdma_ring_pad_ib - pad the IB to the required number of dw
 *
 * @ib: indirect buffer to fill with padding
 *
 */
static void xdma_ring_pad_ib(struct gsgpu_ring *ring, struct gsgpu_ib *ib)
{
	u32 pad_count;
	int i;

	pad_count = (8 - (ib->length_dw & 0x7)) % 8;
	for (i = 0; i < pad_count; i++)
		ib->ptr[ib->length_dw++] = GSPKT(GSPKT_NOP, 0);
}

/**
 * xdma_ring_emit_pipeline_sync - sync the pipeline
 *
 * @ring: gsgpu_ring pointer
 *
 * Make sure all previous operations are completed ().
 */
static void xdma_ring_emit_pipeline_sync(struct gsgpu_ring *ring)
{
	u32 seq = ring->fence_drv.sync_seq;
	u64 addr = ring->fence_drv.gpu_addr;

	/* wait for idle */
	gsgpu_ring_write(ring, GSPKT(GSPKT_POLL, 5) |
				POLL_CONDITION(3) | /* equal */
				POLL_REG_MEM(1)); /* reg/mem */
	gsgpu_ring_write(ring, lower_32_bits(addr));
	gsgpu_ring_write(ring, upper_32_bits(addr));
	gsgpu_ring_write(ring, seq); /* reference */
	gsgpu_ring_write(ring, 0xffffffff); /* mask */
	gsgpu_ring_write(ring, POLL_TIMES_INTERVAL(0xfff, 1)); /* retry count, interval */
}

/**
 * xdma_ring_emit_vm_flush - vm flush using xDMA
 *
 * @ring: gsgpu_ring pointer
 * @vm: gsgpu_vm pointer
 *
 * Update the page table base and flush the VM TLB
 * using xDMA.
 */
static void xdma_ring_emit_vm_flush(struct gsgpu_ring *ring,
					 unsigned vmid, u64 pd_addr)
{
	gsgpu_gmc_emit_flush_gpu_tlb(ring, vmid, pd_addr);
}

static void xdma_ring_emit_wreg(struct gsgpu_ring *ring,
				     u32 reg, u32 val)
{
	gsgpu_ring_write(ring, GSPKT(GSPKT_WRITE, 2) | WRITE_DST_SEL(0) | WRITE_WAIT);
	gsgpu_ring_write(ring, reg);
	gsgpu_ring_write(ring, val);
}

static int xdma_set_pte_pde_test(struct gsgpu_ring *ring, long timeout)
{
	int r = 0;
	u32 align;
	u32 domain;
	u64 gpu_addr;
	u64 *cpu_ptr;
	struct gsgpu_bo *bo;
	unsigned int size;
	struct gsgpu_ib ib;
	struct dma_fence *f = NULL;
	struct gsgpu_device *ldev;

	ldev = ring->adev;

	bo = NULL;
	size = GSGPU_GPU_PAGE_SIZE;
	align = GSGPU_GPU_PAGE_SIZE;
	domain = GSGPU_GEM_DOMAIN_VRAM;

	r = gsgpu_bo_create_kernel(ldev, size, align, domain, &bo, &gpu_addr, (void **)&cpu_ptr);
	if (r) {
		DRM_ERROR("xdma_set_pte_pde_test : gsgpu_bo_create_kernel error\r\n");
		return r;
	}

	memset(cpu_ptr, 0x55, size);
	if (readq(&cpu_ptr[0]) != 0x5555555555555555) {
		DRM_ERROR("xdma_set_pte_pde_test : set vram error through pcie\r\n");
	} else {
		DRM_DEBUG_DRIVER("xdma_set_pte_pde_test : set vram success through pcie\r\n");
	}

	memset(&ib, 0, sizeof(ib));
	r = gsgpu_ib_get(ldev, NULL, 256, &ib);
	if (r) {
		DRM_ERROR("xdma_set_pte_pde_test : gsgpu_ib_get error\r\n");
		goto bo_free;
	}

	xdma_vm_set_pte_pde(&ib, gpu_addr, 0, 2048, 8, 0);
	xdma_ring_pad_ib(ring, &ib);

	r = gsgpu_ib_schedule(ring, 1, &ib, NULL, &f);
	if (r) {
		DRM_ERROR("xdma_set_pte_pde_test : gsgpu_ib_schedule error\r\n");
		goto ib_free;
	}

	r = dma_fence_wait_timeout(f, false, timeout);
	if (r == 0) {
		DRM_ERROR("xdma_set_pte_pde_test : dma_fence_wait_timeout timed out\r\n");
		r = -ETIMEDOUT;
		goto ib_free;
	} else if (r < 0) {
		DRM_ERROR("xdma_set_pte_pde_test : dma_fence_wait_timeout failed\r\n");
		goto ib_free;
	}

	if ((readq(&cpu_ptr[0]) == 0x00) && (readq(&cpu_ptr[1]) == GSGPU_GPU_PAGE_SIZE))   {
		DRM_DEBUG_DRIVER("xdma_set_pte_pde_test : success\r\n");
	} else {
		DRM_ERROR("xdma_set_pte_pde_test : failed\r\n");
	}

ib_free:
	gsgpu_ib_free(ldev, &ib, NULL);
	dma_fence_put(f);

bo_free:
	gsgpu_bo_free_kernel(&bo, &gpu_addr, (void **)&cpu_ptr);

	return r;
}

static int xdma_copy_pte_test(struct gsgpu_ring *ring, long timeout)
{
	int r = 0;
	u32 align;
	u32 domain;
	u64 gpu_addr;
	u64 *cpu_ptr;
	u64 gpu_addr_gtt;
	u64 *cpu_ptr_gtt;
	struct gsgpu_bo *bo;
	struct gsgpu_bo *bo_gtt;
	unsigned int size;
	struct gsgpu_ib ib;
	struct dma_fence *f = NULL;
	struct gsgpu_device *ldev;

	ldev = ring->adev;

	bo = NULL;
	bo_gtt = NULL;
	size = GSGPU_GPU_PAGE_SIZE;
	align = GSGPU_GPU_PAGE_SIZE;

	domain = GSGPU_GEM_DOMAIN_GTT;
	r = gsgpu_bo_create_kernel(ldev, size, align, domain, &bo_gtt, &gpu_addr_gtt, (void **)&cpu_ptr_gtt);
	if (r) {
		DRM_ERROR("xdma_copy_pte_test : gsgpu_bo_create_kernel gtt error\r\n");
		goto bo_free;
	}
	memset(cpu_ptr_gtt, 0xaa, size);
	if (readq(&cpu_ptr_gtt[0]) != 0xaaaaaaaaaaaaaaaa) {
		DRM_ERROR("xdma_copy_pte_test : set gtt error through pcie\r\n");
	} else {
		DRM_INFO("xdma_copy_pte_test : set vram success through pcie\r\n");
	}

	domain = GSGPU_GEM_DOMAIN_VRAM;
	r = gsgpu_bo_create_kernel(ldev, size, align, domain, &bo, &gpu_addr, (void **)&cpu_ptr);
	if (r) {
		DRM_ERROR("xdma_copy_pte_test : gsgpu_bo_create_kernel vram error\r\n");
		return r;
	}

	memset(cpu_ptr, 0x55, size);
	if (readq(&cpu_ptr[0]) != 0x5555555555555555) {
		DRM_ERROR("xdma_copy_pte_test : set vram error through pcie\r\n");
	} else {
		DRM_INFO("xdma_copy_pte_test : set vram success through pcie\r\n");
	}

	memset(&ib, 0, sizeof(ib));
	r = gsgpu_ib_get(ldev, NULL, 256, &ib);
	if (r) {
		DRM_ERROR("xdma_copy_pte_test : gsgpu_ib_get error\r\n");
		goto bo_free_gtt;
	}

	xdma_vm_copy_pte(&ib, gpu_addr, gpu_addr_gtt, 2048);
	xdma_ring_pad_ib(ring, &ib);

	r = gsgpu_ib_schedule(ring, 1, &ib, NULL, &f);
	if (r) {
		DRM_ERROR("xdma_copy_pte_test : gsgpu_ib_schedule error\r\n");
		goto ib_free;
	}

	r = dma_fence_wait_timeout(f, false, timeout);
	if (r == 0) {
		DRM_ERROR("xdma_copy_pte_test : dma_fence_wait_timeout timed out\r\n");
		r = -ETIMEDOUT;
		goto ib_free;
	} else if (r < 0) {
		DRM_ERROR("xdma_copy_pte_test : dma_fence_wait_timeout failed\r\n");
		goto ib_free;
	}

	if (readq(&cpu_ptr[0]) == 0xaaaaaaaaaaaaaaaa) {
		DRM_INFO("xdma_copy_pte_test : success\r\n");
		r = 0;
	} else {
		DRM_ERROR("xdma_copy_pte_test : failed\r\n");
	}

ib_free:
	gsgpu_ib_free(ldev, &ib, NULL);
	dma_fence_put(f);

bo_free_gtt:
	gsgpu_bo_free_kernel(&bo_gtt, &gpu_addr_gtt, (void **)&cpu_ptr_gtt);

bo_free:
	gsgpu_bo_free_kernel(&bo, &gpu_addr, (void **)&cpu_ptr);

	return r;
}

static int xdma_early_init(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	adev->xdma.num_instances = 1;

	xdma_set_ring_funcs(adev);
	xdma_set_buffer_funcs(adev);
	xdma_set_vm_pte_funcs(adev);
	xdma_set_irq_funcs(adev);

	return 0;
}

static int xdma_sw_init(void *handle)
{
	struct gsgpu_ring *ring;
	int r, i;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	/* XDMA trap event */
	r = gsgpu_irq_add_id(adev, GSGPU_IH_CLIENTID_LEGACY, GSGPU_SRCID_XDMA_TRAP,
			      &adev->xdma.trap_irq);
	if (r)
		return r;

	/* XDMA Privileged inst */
	r = gsgpu_irq_add_id(adev, GSGPU_IH_CLIENTID_LEGACY, 241,
			      &adev->xdma.illegal_inst_irq);
	if (r)
		return r;

	/* XDMA Privileged inst */
	r = gsgpu_irq_add_id(adev, GSGPU_IH_CLIENTID_LEGACY, GSGPU_SRCID_XDMA_SRBM_WRITE,
			      &adev->xdma.illegal_inst_irq);
	if (r)
		return r;

	for (i = 0; i < adev->xdma.num_instances; i++) {
		ring = &adev->xdma.instance[i].ring;
		ring->ring_obj = NULL;

		sprintf(ring->name, "xdma%d", i);
		r = gsgpu_ring_init(adev, ring, 256,
				     &adev->xdma.trap_irq,
				     (i == 0) ?
				     GSGPU_XDMA_IRQ_TRAP0 :
				     GSGPU_XDMA_IRQ_TRAP1);
		if (r)
			return r;
	}

	return r;
}

static int xdma_sw_fini(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;
	int i;

	for (i = 0; i < adev->xdma.num_instances; i++)
		gsgpu_ring_fini(&adev->xdma.instance[i].ring);

	return 0;
}

static int xdma_hw_init(void *handle)
{
	int r;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	r = xdma_start(adev);
	if (r)
		return r;

	return r;
}

static int xdma_hw_fini(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	xdma_enable(adev, false);

	return 0;
}

static int xdma_suspend(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	return xdma_hw_fini(adev);
}

static int xdma_resume(void *handle)
{
	int r;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	/* start the gfx rings and rlc compute queues */
	r = xdma_hw_init(adev);

	return r;
}

static bool xdma_is_idle(void *handle)
{
	return true;
}

static int xdma_wait_for_idle(void *handle)
{
	unsigned i;
	u32 tmp;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	for (i = 0; i < adev->usec_timeout; i++) {
		//TODO
		tmp = 0;

		if (!tmp)
			return 0;
		udelay(1);
	}
	return -ETIMEDOUT;
}

static int xdma_set_trap_irq_state(struct gsgpu_device *adev,
					struct gsgpu_irq_src *source,
					unsigned type,
					enum gsgpu_interrupt_state state)
{
	return 0;
}

static int xdma_process_trap_irq(struct gsgpu_device *adev,
				      struct gsgpu_irq_src *source,
				      struct gsgpu_iv_entry *entry)
{
	u8 instance_id, queue_id;

	instance_id = (entry->ring_id & 0x3) >> 0;
	queue_id = (entry->ring_id & 0xc) >> 2;
	DRM_DEBUG("IH: XDMA trap\n");
	switch (instance_id) {
	case 0:
		switch (queue_id) {
		case 0:
			gsgpu_fence_process(&adev->xdma.instance[0].ring);
			break;
		case 1:
			/* XXX compute */
			break;
		case 2:
			/* XXX compute */
			break;
		}
		break;
	case 1:
		switch (queue_id) {
		case 0:
			gsgpu_fence_process(&adev->xdma.instance[1].ring);
			break;
		case 1:
			/* XXX compute */
			break;
		case 2:
			/* XXX compute */
			break;
		}
		break;
	}
	return 0;
}

static int xdma_process_illegal_inst_irq(struct gsgpu_device *adev,
					      struct gsgpu_irq_src *source,
					      struct gsgpu_iv_entry *entry)
{
	DRM_ERROR("Illegal instruction in XDMA command stream\n");
	schedule_work(&adev->reset_work);
	return 0;
}

static const struct gsgpu_ip_funcs xdma_ip_funcs = {
	.name = "xdma",
	.early_init = xdma_early_init,
	.late_init = NULL,
	.sw_init = xdma_sw_init,
	.sw_fini = xdma_sw_fini,
	.hw_init = xdma_hw_init,
	.hw_fini = xdma_hw_fini,
	.suspend = xdma_suspend,
	.resume = xdma_resume,
	.is_idle = xdma_is_idle,
	.wait_for_idle = xdma_wait_for_idle,
};

static const struct gsgpu_ring_funcs xdma_ring_funcs = {
	.type = GSGPU_RING_TYPE_XDMA,
	.align_mask = 0xf,
	.nop = GSPKT(GSPKT_NOP, 0),
	.support_64bit_ptrs = false,
	.get_rptr = xdma_ring_get_rptr,
	.get_wptr = xdma_ring_get_wptr,
	.set_wptr = xdma_ring_set_wptr,
	.emit_frame_size =
		3 + /* hdp invalidate */
		6 + /* xdma_ring_emit_pipeline_sync */
		VI_FLUSH_GPU_TLB_NUM_WREG * 3 + 6 + /* xdma_ring_emit_vm_flush */
		5 + 5 + 5, /* xdma_ring_emit_fence x3 for user fence, vm fence */
	.emit_ib_size = 4, /* xdma_ring_emit_ib */
	.emit_ib = xdma_ring_emit_ib,
	.emit_fence = xdma_ring_emit_fence,
	.emit_pipeline_sync = xdma_ring_emit_pipeline_sync,
	.emit_vm_flush = xdma_ring_emit_vm_flush,
	.test_ring = xdma_ring_test_ring,
	.test_ib = xdma_ring_test_ib,
	.test_xdma = xdma_ring_test_xdma,
	.insert_nop = xdma_ring_insert_nop,
	.pad_ib = xdma_ring_pad_ib,
	.emit_wreg = xdma_ring_emit_wreg,
};

static void xdma_set_ring_funcs(struct gsgpu_device *adev)
{
	int i;

	for (i = 0; i < adev->xdma.num_instances; i++) {
		adev->xdma.instance[i].ring.funcs = &xdma_ring_funcs;
		adev->xdma.instance[i].ring.me = i;
	}
}

static const struct gsgpu_irq_src_funcs xdma_trap_irq_funcs = {
	.set = xdma_set_trap_irq_state,
	.process = xdma_process_trap_irq,
};

static const struct gsgpu_irq_src_funcs xdma_illegal_inst_irq_funcs = {
	.process = xdma_process_illegal_inst_irq,
};

static void xdma_set_irq_funcs(struct gsgpu_device *adev)
{
	adev->xdma.trap_irq.num_types = GSGPU_XDMA_IRQ_LAST;
	adev->xdma.trap_irq.funcs = &xdma_trap_irq_funcs;
	adev->xdma.illegal_inst_irq.funcs = &xdma_illegal_inst_irq_funcs;
}

/**
 * xdma_emit_copy_buffer - copy buffer using the xDMA engine
 *
 * @ring: gsgpu_ring structure holding ring information
 * @src_offset: src GPU address
 * @dst_offset: dst GPU address
 * @byte_count: number of bytes to xfer
 *
 * Copy GPU buffers using the DMA engine.
 * Used by the gsgpu ttm implementation to move pages if
 * registered as the asic copy callback.
 */
static void xdma_emit_copy_buffer(struct gsgpu_ib *ib,
					u64 src_offset,
					u64 dst_offset,
					u32 byte_count)
{
	struct gsgpu_bo *bo = ib->sa_bo->manager->bo;
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	uint32_t cpp = 8;
	uint32_t width, height;
	uint32_t dst_umap, src_umap;

	dst_umap = (dst_offset >= adev->gmc.vram_start &&
		    dst_offset < adev->gmc.vram_end) ?
			   GSGPU_XDMA_FLAG_UMAP :
			   0;
	src_umap = (src_offset >= adev->gmc.vram_start &&
		    src_offset < adev->gmc.vram_end) ?
			   GSGPU_XDMA_FLAG_UMAP :
			   0;
	height = 1;
	width = byte_count / cpp;

	/* hardware limit 2^16 pixels per line */
	while (width >= 0x10000) {
		width = width / 2;
		height = height * 2;
	}

	/*XDMA COPY
	 * L2L : 1<<24
	 * RGBA16 : 1<<8
	 * */
	ib->ptr[ib->length_dw++] = GSPKT(GSPKT_XDMA_COPY, 8) | (0x1 << 24) | (1 << 8);
	ib->ptr[ib->length_dw++] =  (height << 16) | width;
	ib->ptr[ib->length_dw++] = lower_32_bits(src_offset);
	ib->ptr[ib->length_dw++] = upper_32_bits(src_offset) | src_umap;
	ib->ptr[ib->length_dw++] = lower_32_bits(dst_offset);
	ib->ptr[ib->length_dw++] = upper_32_bits(dst_offset) | dst_umap;
	ib->ptr[ib->length_dw++] = (width ? width : 1) * cpp;
	ib->ptr[ib->length_dw++] = (width ? width : 1) * cpp;
	ib->ptr[ib->length_dw++] = 0;
}

/**
 * xdma_emit_fill_buffer - fill buffer using the xDMA engine
 *
 * @ring: gsgpu_ring structure holding ring information
 * @src_data: value to write to buffer
 * @dst_offset: dst GPU address
 * @byte_count: number of bytes to xfer
 *
 * Fill GPU buffers using the DMA engine  .
 */
static void xdma_emit_fill_buffer(struct gsgpu_ib *ib,
					u32 src_data,
					u64 dst_offset,
					u32 byte_count)
{
	struct gsgpu_bo *bo = ib->sa_bo->manager->bo;
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	uint32_t width, height;
	uint32_t dst_umap;

	height = 1;
	dst_umap = (dst_offset >= adev->gmc.vram_start &&
		    dst_offset < adev->gmc.vram_end) ?
			   GSGPU_XDMA_FLAG_UMAP :
			   0;

	/*RGBA8 == 4 bytes per pixles*/
	width = byte_count / 4;

	/* hardware limit 2^16 pixels per line */
	while (width >= 0x10000) {
		width = width / 2;
		height = height * 2;
	}

	/*XDMA COPY
	 * MEMSET : 7 <<24
	 * RGBA8 : 0<<8
	 * */
	ib->ptr[ib->length_dw++] = GSPKT(GSPKT_XDMA_COPY, 8) | (0x7 << 24) | (0 << 8);
	ib->ptr[ib->length_dw++] =  (height << 16) | width;
	ib->ptr[ib->length_dw++] = lower_32_bits(src_data);
	ib->ptr[ib->length_dw++] = 0;
	ib->ptr[ib->length_dw++] = lower_32_bits(dst_offset);
	ib->ptr[ib->length_dw++] = upper_32_bits(dst_offset) | dst_umap;
	ib->ptr[ib->length_dw++] = 0;
	ib->ptr[ib->length_dw++] = (width ? width : 1) * 4;
	ib->ptr[ib->length_dw++] = 0;
}

static int xdma_fill_test(struct gsgpu_ring *ring, long timeout)
{
	int r = 0;
	struct gsgpu_bo *bo;
	unsigned int size;
	u32 align;
	u32 domain;
	u64 gpu_addr;
	u64 *cpu_ptr;
	struct gsgpu_ib ib;
	struct dma_fence *f = NULL;
	struct gsgpu_device *ldev;

	ldev = ring->adev;

	bo = NULL;
	size = GSGPU_GPU_PAGE_SIZE;
	align = GSGPU_GPU_PAGE_SIZE;
	domain = GSGPU_GEM_DOMAIN_VRAM;

	r = gsgpu_bo_create_kernel(ldev, size, align, domain, &bo, &gpu_addr, (void **)&cpu_ptr);
	if (r) {
		DRM_ERROR("xdma_fill_test : gsgpu_bo_create_kernel error\r\n");
		return r;
	}

	memset(cpu_ptr, 0x55, size);
	if (readq(&cpu_ptr[0]) != 0x5555555555555555) {
		DRM_ERROR("xdma_fill_test : set vram error through pcie\r\n");
	} else {
		DRM_INFO("xdma_fill_test : set vram success through pcie\r\n");
	}

	memset(&ib, 0, sizeof(ib));
	r = gsgpu_ib_get(ldev, NULL, 256, &ib);
	if (r) {
		DRM_ERROR("xdma_fill_test : gsgpu_ib_get error\r\n");
		goto bo_free;
	}

	xdma_emit_fill_buffer(&ib, 0xaaaaaaaa, gpu_addr, GSGPU_GPU_PAGE_SIZE);
	xdma_ring_pad_ib(ring, &ib);

	r = gsgpu_ib_schedule(ring, 1, &ib, NULL, &f);
	if (r) {
		DRM_ERROR("xdma_fill_test : gsgpu_ib_schedule error\r\n");
		goto ib_free;
	}

	r = dma_fence_wait_timeout(f, false, timeout);
	if (r == 0) {
		DRM_ERROR("xdma_fill_test : dma_fence_wait_timeout timed out\r\n");
		r = -ETIMEDOUT;
		goto ib_free;
	} else if (r < 0) {
		DRM_ERROR("xdma_fill_test : dma_fence_wait_timeout failed\r\n");
		goto ib_free;
	}

	if (readq(&cpu_ptr[0]) == 0xaaaaaaaaaaaaaaaa) {
		DRM_INFO("xdma_fill_test : success\r\n");
	} else {
		DRM_ERROR("xdma_fill_test : failed\r\n");
	}

ib_free:
	gsgpu_ib_free(ldev, &ib, NULL);
	dma_fence_put(f);

bo_free:
	gsgpu_bo_free_kernel(&bo, &gpu_addr, (void **)&cpu_ptr);

	return r;
}

static const struct gsgpu_buffer_funcs xdma_buffer_funcs = {
	.copy_max_bytes = 0x3fffc0, /* not 0x3fffff due to HW limitation */
	.copy_num_dw = 9,
	.emit_copy_buffer = xdma_emit_copy_buffer,

	.fill_max_bytes = 0x3fffc0, /* not 0x3fffff due to HW limitation */
	.fill_num_dw = 9,
	.emit_fill_buffer = xdma_emit_fill_buffer,
};

static void xdma_set_buffer_funcs(struct gsgpu_device *adev)
{
	if (adev->mman.buffer_funcs == NULL) {
		adev->mman.buffer_funcs = &xdma_buffer_funcs;
		adev->mman.buffer_funcs_ring = &adev->xdma.instance[0].ring;
	}
}

static const struct gsgpu_vm_pte_funcs xdma_vm_pte_funcs = {
	.copy_pte_num_dw = 9,
	.copy_pte = xdma_vm_copy_pte,

	.set_pte_pde_num_dw = 9,
	.set_pte_pde = xdma_vm_set_pte_pde,
};

static void xdma_set_vm_pte_funcs(struct gsgpu_device *adev)
{
	unsigned i;

	if (adev->vm_manager.vm_pte_funcs == NULL) {
		adev->vm_manager.vm_pte_funcs = &xdma_vm_pte_funcs;
		for (i = 0; i < adev->xdma.num_instances; i++)
			adev->vm_manager.vm_pte_rings[i] =
				&adev->xdma.instance[i].ring;

		adev->vm_manager.vm_pte_num_rings = adev->xdma.num_instances;
	}
}

const struct gsgpu_ip_block_version xdma_ip_block = {
	.type = GSGPU_IP_BLOCK_TYPE_XDMA,
	.major = 1,
	.minor = 0,
	.rev = 0,
	.funcs = &xdma_ip_funcs,
};
