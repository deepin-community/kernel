#include <linux/kernel.h>
#include "gsgpu.h"
#include "gsgpu_common.h"
#include "gsgpu_cp.h"
#include "gsgpu_irq.h"

#define GFX8_NUM_GFX_RINGS     1

MODULE_FIRMWARE("loongson/lg100_cp.bin");

static void gfx_set_ring_funcs(struct gsgpu_device *adev);
static void gfx_set_irq_funcs(struct gsgpu_device *adev);

static int gfx_ring_test_ring(struct gsgpu_ring *ring)
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

	for (i = 0; i < adev->usec_timeout; i++) {
		tmp = le32_to_cpu(adev->wb.wb[index]);
		if (tmp == 0xDEADBEEF)
			break;
		DRM_UDELAY(1);
	}

	if (i < adev->usec_timeout) {
		//DRM_DEBUG("ring test on %d succeeded in %d usecs\n", ring->idx, i);
		DRM_INFO("ring test on %d succeeded in %d usecs\n", ring->idx, i);
	} else {
		DRM_ERROR("gsgpu: ring %d test failed (0x%08X)\n",
			  ring->idx, tmp);
		r = -EINVAL;
	}
	gsgpu_device_wb_free(adev, index);

	return r;
}

static int gfx_ring_test_ib(struct gsgpu_ring *ring, long timeout)
{
	struct gsgpu_device *adev = ring->adev;
	struct gsgpu_ib ib;
	struct dma_fence *f = NULL;

	unsigned int index;
	uint64_t gpu_addr;
	uint32_t tmp;
	long r;

	r = gsgpu_device_wb_get(adev, &index);
	if (r) {
		dev_err(adev->dev, "(%ld) failed to allocate wb slot\n", r);
		return r;
	}

	gpu_addr = adev->wb.gpu_addr + (index * 4);
	adev->wb.wb[index] = cpu_to_le32(0xCAFEDEAD);
	memset(&ib, 0, sizeof(ib));
	r = gsgpu_ib_get(adev, NULL, 16, &ib);
	if (r) {
		DRM_ERROR("gsgpu: failed to get ib (%ld).\n", r);
		goto err1;
	}
	ib.ptr[0] = GSPKT(GSPKT_WRITE, 3) | WRITE_DST_SEL(1) | WRITE_WAIT;
	ib.ptr[1] = lower_32_bits(gpu_addr);
	ib.ptr[2] = upper_32_bits(gpu_addr);
	ib.ptr[3] = 0xDEADBEEF;
	ib.length_dw = 4;

	r = gsgpu_ib_schedule(ring, 1, &ib, NULL, &f);
	if (r)
		goto err2;

	r = dma_fence_wait_timeout(f, false, timeout);
	if (r == 0) {
		DRM_ERROR("gsgpu: IB test timed out.\n");
		r = -ETIMEDOUT;
		goto err2;
	} else if (r < 0) {
		DRM_ERROR("gsgpu: fence wait failed (%ld).\n", r);
		goto err2;
	}

	tmp = adev->wb.wb[index];
	if (tmp == 0xDEADBEEF) {
		DRM_DEBUG("ib test on ring %d succeeded\n", ring->idx);
		r = 0;
	} else {
		DRM_ERROR("ib test on ring %d failed\n", ring->idx);
		r = -EINVAL;
	}

err2:
	gsgpu_ib_free(adev, &ib, NULL);
	dma_fence_put(f);
err1:
	gsgpu_device_wb_free(adev, index);
	return r;
}

static int gfx_gpu_early_init(struct gsgpu_device *adev)
{
	u32 gb_addr_config;
	u32 tmp;

	adev->gfx.config.max_shader_engines = 2;
	adev->gfx.config.max_tile_pipes = 4;
	adev->gfx.config.max_cu_per_sh = 2;
	adev->gfx.config.max_sh_per_se = 1;
	adev->gfx.config.max_backends_per_se = 2;
	adev->gfx.config.max_texture_channel_caches = 4;
	adev->gfx.config.max_gprs = 256;
	adev->gfx.config.max_gs_threads = 32;
	adev->gfx.config.max_hw_contexts = 8;

	adev->gfx.config.sc_prim_fifo_size_frontend = 0x20;
	adev->gfx.config.sc_prim_fifo_size_backend = 0x100;
	adev->gfx.config.sc_hiz_tile_fifo_size = 0x30;
	adev->gfx.config.sc_earlyz_tile_fifo_size = 0x130;

	adev->gfx.config.mc_arb_ramcfg = 0;//RREG32(mmMC_ARB_RAMCFG);

	adev->gfx.config.num_tile_pipes = adev->gfx.config.max_tile_pipes;
	adev->gfx.config.mem_max_burst_length_bytes = 256;

	tmp = 0; //REG_GET_FIELD(mc_arb_ramcfg, MC_ARB_RAMCFG, NOOFCOLS);
	adev->gfx.config.mem_row_size_in_kb = (4 * (1 << (8 + tmp))) / 1024;
	if (adev->gfx.config.mem_row_size_in_kb > 4)
		adev->gfx.config.mem_row_size_in_kb = 4;

	adev->gfx.config.shader_engine_tile_size = 32;
	adev->gfx.config.num_gpus = 1;
	adev->gfx.config.multi_gpu_tile_size = 64;

	/* fix up row size */
	switch (adev->gfx.config.mem_row_size_in_kb) {
	case 1:
	default:
		gb_addr_config = 0;//REG_SET_FIELD(gb_addr_config, GB_ADDR_CONFIG, ROW_SIZE, 0);
		break;
	case 2:
		gb_addr_config = 0;//REG_SET_FIELD(gb_addr_config, GB_ADDR_CONFIG, ROW_SIZE, 1);
		break;
	case 4:
		gb_addr_config = 0;//REG_SET_FIELD(gb_addr_config, GB_ADDR_CONFIG, ROW_SIZE, 2);
		break;
	}
	adev->gfx.config.gb_addr_config = gb_addr_config;

	return 0;
}

static int gfx_sw_init(void *handle)
{
	int i, r;
	struct gsgpu_ring *ring;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	/* EOP Event */
	r = gsgpu_irq_add_id(adev, GSGPU_IH_CLIENTID_LEGACY, GSGPU_SRCID_CP_END_OF_PIPE, &adev->gfx.eop_irq);
	if (r)
		return r;

	/* Privileged reg */
	r = gsgpu_irq_add_id(adev, GSGPU_IH_CLIENTID_LEGACY, GSGPU_SRCID_CP_PRIV_REG_FAULT,
			      &adev->gfx.priv_reg_irq);
	if (r)
		return r;

	/* Privileged inst */
	r = gsgpu_irq_add_id(adev, GSGPU_IH_CLIENTID_LEGACY, GSGPU_SRCID_CP_PRIV_INSTR_FAULT,
			      &adev->gfx.priv_inst_irq);
	if (r)
		return r;

	adev->gfx.gfx_current_status = GSGPU_GFX_NORMAL_MODE;

	//gfx_scratch_init(adev);

	/* set up the gfx ring */
	for (i = 0; i < adev->gfx.num_gfx_rings; i++) {
		ring = &adev->gfx.gfx_ring[i];
		ring->ring_obj = NULL;
		sprintf(ring->name, "gfx");

		r = gsgpu_ring_init(adev, ring, 256, &adev->gfx.eop_irq,
				     GSGPU_CP_IRQ_GFX_EOP);
		if (r)
			return r;
	}

	adev->gfx.ce_ram_size = 0x8000;

	r = gfx_gpu_early_init(adev);
	if (r)
		return r;

	return 0;
}

static int gfx_sw_fini(void *handle)
{
	int i;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	for (i = 0; i < adev->gfx.num_gfx_rings; i++)
		gsgpu_ring_fini(&adev->gfx.gfx_ring[i]);

	return 0;
}

static void gfx_parse_ind_reg_list(int *register_list_format,
				int ind_offset,
				int list_size,
				int *unique_indices,
				int *indices_count,
				int max_indices,
				int *ind_start_offsets,
				int *offset_count,
				int max_offset)
{
	int indices;
	bool new_entry = true;

	for (; ind_offset < list_size; ind_offset++) {

		if (new_entry) {
			new_entry = false;
			ind_start_offsets[*offset_count] = ind_offset;
			*offset_count = *offset_count + 1;
			BUG_ON(*offset_count >= max_offset);
		}

		if (register_list_format[ind_offset] == 0xFFFFFFFF) {
			new_entry = true;
			continue;
		}

		ind_offset += 2;

		/* look for the matching indice */
		for (indices = 0;
			indices < *indices_count;
			indices++) {
			if (unique_indices[indices] ==
				register_list_format[ind_offset])
				break;
		}

		if (indices >= *indices_count) {
			unique_indices[*indices_count] =
				register_list_format[ind_offset];
			indices = *indices_count;
			*indices_count = *indices_count + 1;
			BUG_ON(*indices_count >= max_indices);
		}

		register_list_format[ind_offset] = indices;
	}
}

static int gfx_cp_gfx_resume(struct gsgpu_device *adev)
{
	struct gsgpu_ring *ring;
	u64 cb_addr;//, rptr_addr, wptr_gpu_addr;
	int r = 0;

	/*Flush pipeline*/
	gsgpu_cmd_exec(adev, GSCMD(GSCMD_PIPE, GSCMD_PIPE_FLUSH), 1, ~1);
	/* Wait a little for things to flush pipeline */
	mdelay(1000);

	/* Set ring buffer size */
	ring = &adev->gfx.gfx_ring[0];

	/* Initialize the ring buffer's read and write pointers */
	ring->wptr = 0;
	WREG32(GSGPU_GFX_CB_WPTR_OFFSET, lower_32_bits(ring->wptr));

	/* set the RPTR */
	WREG32(GSGPU_GFX_CB_RPTR_OFFSET, 0);

	mdelay(1);

	cb_addr = ring->gpu_addr;
	WREG32(GSGPU_GFX_CB_BASE_LO_OFFSET, cb_addr);
	WREG32(GSGPU_GFX_CB_BASE_HI_OFFSET, upper_32_bits(cb_addr));

	/* start the ring */
	gsgpu_ring_clear_ring(ring);

	ring->ready = true;

	return r;
}

static int gfx_cp_resume(struct gsgpu_device *adev)
{
	int r;

	r = gfx_cp_gfx_resume(adev);
	if (r)
		return r;

	return 0;
}

static int gfx_hw_init(void *handle)
{
	int r;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	r = gfx_cp_resume(adev);

	return r;
}

static int gfx_hw_fini(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	gsgpu_irq_put(adev, &adev->gfx.priv_reg_irq, 0);
	gsgpu_irq_put(adev, &adev->gfx.priv_inst_irq, 0);

	return 0;
}

static int gfx_suspend(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;
	adev->gfx.in_suspend = true;
	return gfx_hw_fini(adev);
}

static int gfx_resume(void *handle)
{
	int r;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	r = gfx_hw_init(adev);
	adev->gfx.in_suspend = false;
	return r;
}

static bool gfx_is_idle(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	return (RREG32(GSGPU_STATUS) == GSCMD_STS_DONE);
}

static int gfx_wait_for_idle(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	if (gsgpu_cp_wait_done(adev) == true)
			return 0;

	return -ETIMEDOUT;
}

/**
 * gfx_get_gpu_clock_counter - return GPU clock counter snapshot
 *
 * @adev: gsgpu_device pointer
 *
 * Fetches a GPU clock counter snapshot.
 * Returns the 64 bit clock counter snapshot.
 */
static uint64_t gfx_get_gpu_clock_counter(struct gsgpu_device *adev)
{
	uint64_t clock = 0;

	//TODO
	mutex_lock(&adev->gfx.gpu_clock_mutex);

	DRM_DEBUG("%s Not impelet\n", __func__);

	mutex_unlock(&adev->gfx.gpu_clock_mutex);
	return clock;
}

static const struct gsgpu_gfx_funcs gfx_gfx_funcs = {
	.get_gpu_clock_counter = &gfx_get_gpu_clock_counter,
	.read_wave_data = NULL,//&gfx_read_wave_data,
	.read_wave_sgprs = NULL,//&gfx_read_wave_sgprs,
	.select_me_pipe_q = NULL//&gfx_select_me_pipe_q
};

static int gfx_early_init(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	adev->gfx.num_gfx_rings = GFX8_NUM_GFX_RINGS;
	adev->gfx.funcs = &gfx_gfx_funcs;
	gfx_set_ring_funcs(adev);
	gfx_set_irq_funcs(adev);

	return 0;
}

static int gfx_late_init(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;
	int r;

	r = gsgpu_irq_get(adev, &adev->gfx.priv_reg_irq, 0);
	if (r)
		return r;

	r = gsgpu_irq_get(adev, &adev->gfx.priv_inst_irq, 0);
	if (r)
		return r;

	return 0;
}

static u64 gfx_ring_get_rptr(struct gsgpu_ring *ring)
{
	return ring->adev->wb.wb[ring->rptr_offs];
}

static u64 gfx_ring_get_wptr_gfx(struct gsgpu_ring *ring)
{
	struct gsgpu_device *adev = ring->adev;

	return RREG32(GSGPU_GFX_CB_WPTR_OFFSET);
}

static void gfx_ring_set_wptr_gfx(struct gsgpu_ring *ring)
{
	struct gsgpu_device *adev = ring->adev;

	WREG32(GSGPU_GFX_CB_WPTR_OFFSET, lower_32_bits(ring->wptr));
}

static void gfx_ring_emit_ib_gfx(struct gsgpu_ring *ring,
				      struct gsgpu_ib *ib,
				      unsigned vmid, bool ctx_switch)
{
	u32 header, control = 0;

	header = GSPKT(GSPKT_INDIRECT, 3);

	control |= ib->length_dw | (vmid << 24);

	gsgpu_ring_write(ring, header);
	gsgpu_ring_write(ring, lower_32_bits(ib->gpu_addr));
	gsgpu_ring_write(ring, upper_32_bits(ib->gpu_addr));
	gsgpu_ring_write(ring, control);
}

static void gfx_ring_emit_fence_gfx(struct gsgpu_ring *ring, u64 addr,
					 u64 seq, unsigned flags)
{
	bool write64bit = flags & GSGPU_FENCE_FLAG_64BIT;
	bool int_sel = flags & GSGPU_FENCE_FLAG_INT;
	u32 body_size = write64bit ? 4 : 3;

	/* EVENT_WRITE_EOP - flush caches, send int */
	gsgpu_ring_write(ring, GSPKT(GSPKT_FENCE, body_size)
			| (write64bit ? 1 << 9 : 0) | (int_sel ? 1 << 8 : 0));
	gsgpu_ring_write(ring, lower_32_bits(addr));
	gsgpu_ring_write(ring, upper_32_bits(addr));
	gsgpu_ring_write(ring, lower_32_bits(seq));
	if (write64bit)
		gsgpu_ring_write(ring, upper_32_bits(seq));

}

static void gfx_ring_emit_pipeline_sync(struct gsgpu_ring *ring)
{
	uint32_t seq = ring->fence_drv.sync_seq;
	uint64_t addr = ring->fence_drv.gpu_addr;

	gsgpu_ring_write(ring, GSPKT(GSPKT_POLL, 5) |
				POLL_CONDITION(3) | /* equal */
				POLL_REG_MEM(1)); /* reg/mem */
	gsgpu_ring_write(ring, lower_32_bits(addr));
	gsgpu_ring_write(ring, upper_32_bits(addr));
	gsgpu_ring_write(ring, seq); /* reference */
	gsgpu_ring_write(ring, 0xffffffff); /* mask */
	gsgpu_ring_write(ring, POLL_TIMES_INTERVAL(0xfff, 1)); /* retry count, poll interval */
}

static void gfx_ring_emit_vm_flush(struct gsgpu_ring *ring,
					unsigned vmid, uint64_t pd_addr)
{
	gsgpu_gmc_emit_flush_gpu_tlb(ring, vmid, pd_addr);
}

static void gfx_ring_emit_rreg(struct gsgpu_ring *ring, uint32_t reg)
{
	struct gsgpu_device *adev = ring->adev;

	gsgpu_ring_write(ring, GSPKT(GSPKT_READ, 3) | READ_SRC_SEL(0) | WRITE_DST_SEL(1) | WRITE_WAIT);
	gsgpu_ring_write(ring, reg);
	gsgpu_ring_write(ring, lower_32_bits(adev->wb.gpu_addr +
				adev->reg_val_offs * 4));
	gsgpu_ring_write(ring, upper_32_bits(adev->wb.gpu_addr +
				adev->reg_val_offs * 4));
}

static void gfx_ring_emit_wreg(struct gsgpu_ring *ring, uint32_t reg,
				  uint32_t val)
{
	gsgpu_ring_write(ring, GSPKT(GSPKT_WRITE, 2) | WRITE_DST_SEL(0) | WRITE_WAIT);
	gsgpu_ring_write(ring, reg);
	gsgpu_ring_write(ring, val);
}

static void gfx_set_gfx_eop_interrupt_state(struct gsgpu_device *adev,
						 enum gsgpu_interrupt_state state)
{
}

static int gfx_set_priv_reg_fault_state(struct gsgpu_device *adev,
					     struct gsgpu_irq_src *source,
					     unsigned type,
					     enum gsgpu_interrupt_state state)
{

	return 0;
}

static int gfx_set_priv_inst_fault_state(struct gsgpu_device *adev,
					      struct gsgpu_irq_src *source,
					      unsigned type,
					      enum gsgpu_interrupt_state state)
{

	return 0;
}

static int gfx_set_eop_interrupt_state(struct gsgpu_device *adev,
					    struct gsgpu_irq_src *src,
					    unsigned type,
					    enum gsgpu_interrupt_state state)
{
	gfx_set_gfx_eop_interrupt_state(adev, state);

	return 0;
}

static int gfx_eop_irq(struct gsgpu_device *adev,
			    struct gsgpu_irq_src *source,
			    struct gsgpu_iv_entry *entry)
{
	u8 me_id, pipe_id, queue_id;

	DRM_DEBUG("IH: CP EOP\n");
	me_id = (entry->ring_id & 0x0c) >> 2;
	pipe_id = (entry->ring_id & 0x03) >> 0;
	queue_id = (entry->ring_id & 0x70) >> 4;

	switch (me_id) {
	case 0:
		gsgpu_fence_process(&adev->gfx.gfx_ring[0]);
		break;
	case 1:
	case 2:
		break;
	}
	return 0;
}

static int gfx_priv_reg_irq(struct gsgpu_device *adev,
				 struct gsgpu_irq_src *source,
				 struct gsgpu_iv_entry *entry)
{
	DRM_ERROR("Illegal register access in command stream\n");
	schedule_work(&adev->reset_work);
	return 0;
}

static int gfx_priv_inst_irq(struct gsgpu_device *adev,
				  struct gsgpu_irq_src *source,
				  struct gsgpu_iv_entry *entry)
{
	DRM_ERROR("Illegal instruction in command stream\n");
	schedule_work(&adev->reset_work);
	return 0;
}

static const struct gsgpu_ip_funcs gfx_ip_funcs = {
	.name = "gfx",
	.early_init = gfx_early_init,
	.late_init = gfx_late_init,
	.sw_init = gfx_sw_init,
	.sw_fini = gfx_sw_fini,
	.hw_init = gfx_hw_init,
	.hw_fini = gfx_hw_fini,
	.suspend = gfx_suspend,
	.resume = gfx_resume,
	.is_idle = gfx_is_idle,
	.wait_for_idle = gfx_wait_for_idle,
};

static const struct gsgpu_ring_funcs gfx_ring_funcs_gfx = {
	.type = GSGPU_RING_TYPE_GFX,
	.align_mask = 0xf,
	.nop = GSPKT(GSPKT_NOP, 0),
	.support_64bit_ptrs = false,
	.get_rptr = gfx_ring_get_rptr,
	.get_wptr = gfx_ring_get_wptr_gfx,
	.set_wptr = gfx_ring_set_wptr_gfx,
	.emit_frame_size = /* maximum 215dw if count 16 IBs in */
		7 +  /* COND_EXEC */
		1 +  /* PIPELINE_SYNC */
		VI_FLUSH_GPU_TLB_NUM_WREG * 5 + 9 + /* VM_FLUSH */
		5 +  /* FENCE for VM_FLUSH */
		3 + /* CNTX_CTRL */
		5 + 5,/* FENCE x2 */
	.emit_ib_size =	4, /* gfx_ring_emit_ib_gfx */
	.emit_ib = gfx_ring_emit_ib_gfx,
	.emit_fence = gfx_ring_emit_fence_gfx,
	.emit_pipeline_sync = gfx_ring_emit_pipeline_sync,
	.emit_vm_flush = gfx_ring_emit_vm_flush,
	.test_ring = gfx_ring_test_ring,
	.test_ib = gfx_ring_test_ib,
	.insert_nop = gsgpu_ring_insert_nop,
	.pad_ib = gsgpu_ring_generic_pad_ib,
	.emit_wreg = gfx_ring_emit_wreg,
};

static void gfx_set_ring_funcs(struct gsgpu_device *adev)
{
	int i;

	for (i = 0; i < adev->gfx.num_gfx_rings; i++)
		adev->gfx.gfx_ring[i].funcs = &gfx_ring_funcs_gfx;
}

static const struct gsgpu_irq_src_funcs gfx_eop_irq_funcs = {
	.set = gfx_set_eop_interrupt_state,
	.process = gfx_eop_irq,
};

static const struct gsgpu_irq_src_funcs gfx_priv_reg_irq_funcs = {
	.set = gfx_set_priv_reg_fault_state,
	.process = gfx_priv_reg_irq,
};

static const struct gsgpu_irq_src_funcs gfx_priv_inst_irq_funcs = {
	.set = gfx_set_priv_inst_fault_state,
	.process = gfx_priv_inst_irq,
};

static void gfx_set_irq_funcs(struct gsgpu_device *adev)
{
	adev->gfx.eop_irq.num_types = GSGPU_CP_IRQ_LAST;
	adev->gfx.eop_irq.funcs = &gfx_eop_irq_funcs;

	adev->gfx.priv_reg_irq.num_types = 1;
	adev->gfx.priv_reg_irq.funcs = &gfx_priv_reg_irq_funcs;

	adev->gfx.priv_inst_irq.num_types = 1;
	adev->gfx.priv_inst_irq.funcs = &gfx_priv_inst_irq_funcs;
}

const struct gsgpu_ip_block_version gfx_ip_block = {
	.type = GSGPU_IP_BLOCK_TYPE_GFX,
	.major = 8,
	.minor = 0,
	.rev = 0,
	.funcs = &gfx_ip_funcs,
};
