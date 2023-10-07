#include <linux/delay.h>
#include "gsgpu.h"
#include "gsgpu_ih.h"
#include "gsgpu_irq.h"

static void gsgpu_ih_set_interrupt_funcs(struct gsgpu_device *adev);

/**
 * gsgpu_ih_enable_interrupts - Enable the interrupt ring buffer
 *
 * @adev: gsgpu_device pointer
 *
 * Enable the interrupt ring buffer  .
 */
static void gsgpu_ih_enable_interrupts(struct gsgpu_device *adev)
{
	adev->irq.ih.enabled = true;
}

/**
 * gsgpu_ih_disable_interrupts - Disable the interrupt ring buffer
 *
 * @adev: gsgpu_device pointer
 *
 * Disable the interrupt ring buffer  .
 */
static void gsgpu_ih_disable_interrupts(struct gsgpu_device *adev)
{
	adev->irq.ih.enabled = false;
	adev->irq.ih.rptr = 0;
}

/**
 * gsgpu_ih_irq_init - init and enable the interrupt ring
 *
 * @adev: gsgpu_device pointer
 *
 * Allocate a ring buffer for the interrupt controller,
 * enable the RLC, disable interrupts, enable the IH
 * ring buffer and enable it  .
 * Called at device load and reume.
 * Returns 0 for success, errors for failure.
 */
static int gsgpu_ih_irq_init(struct gsgpu_device *adev)
{
	int rb_bufsz;
	u64 wptr_off;

	/* disable irqs */
	gsgpu_ih_disable_interrupts(adev);

	rb_bufsz = adev->irq.ih.ring_size / 4;
	WREG32(GSGPU_INT_CB_SIZE_OFFSET, rb_bufsz);

	/* set the writeback address whether it's enabled or not */
	if (adev->irq.ih.use_bus_addr)
		wptr_off = adev->irq.ih.rb_dma_addr;
	else
		wptr_off = adev->wb.gpu_addr;
	WREG32(GSGPU_INT_CB_BASE_LO_OFFSET, lower_32_bits(wptr_off));
	WREG32(GSGPU_INT_CB_BASE_HI_OFFSET, upper_32_bits(wptr_off));

	/* set rptr, wptr to 0 */
	WREG32(GSGPU_INT_CB_WPTR_OFFSET, 0);
	WREG32(GSGPU_INT_CB_RPTR_OFFSET, 0);

	pci_set_master(adev->pdev);

	/* enable interrupts */
	gsgpu_ih_enable_interrupts(adev);

	return 0;
}

/**
 * gsgpu_ih_irq_disable - disable interrupts
 *
 * @adev: gsgpu_device pointer
 *
 * Disable interrupts on the hw  .
 */
static void gsgpu_ih_irq_disable(struct gsgpu_device *adev)
{
	gsgpu_ih_disable_interrupts(adev);

	/* Wait and acknowledge irq */
	mdelay(1);
}

static u32 ih_func_get_wptr(struct gsgpu_device *adev)
{
	u32 wptr;

	if (adev->irq.ih.use_bus_addr)
		wptr = le32_to_cpu(RREG32(GSGPU_INT_CB_WPTR_OFFSET));
	else
		wptr = le32_to_cpu(RREG32(GSGPU_INT_CB_WPTR_OFFSET));

	return (wptr & adev->irq.ih.ptr_mask);
}

static bool ih_func_prescreen_iv(struct gsgpu_device *adev)
{
	u32 ring_index = adev->irq.ih.rptr;
	u16 pasid;

	switch (le32_to_cpu(adev->irq.ih.ring[ring_index]) & 0xff) {
	case GSGPU_SRCID_GFX_PAGE_INV_FAULT:
	case GSGPU_SRCID_GFX_MEM_PROT_FAULT:
		pasid = le32_to_cpu(adev->irq.ih.ring[ring_index + 2]) >> 16;
		if (!pasid || gsgpu_vm_pasid_fault_credit(adev, pasid))
			return true;
		break;
	default:
		/* Not a VM fault */
		return true;
	}

	adev->irq.ih.rptr += 4;
	return false;
}

static void ih_func_decode_iv(struct gsgpu_device *adev,
				 struct gsgpu_iv_entry *entry)
{
	/* wptr/rptr are in bytes! */
	u32 ring_index = adev->irq.ih.rptr;
	uint32_t dw[4];

	dw[0] = le32_to_cpu(adev->irq.ih.ring[ring_index + 0]);
	dw[1] = le32_to_cpu(adev->irq.ih.ring[ring_index + 1]);
	dw[2] = le32_to_cpu(adev->irq.ih.ring[ring_index + 2]);
	dw[3] = le32_to_cpu(adev->irq.ih.ring[ring_index + 3]);

	DRM_DEBUG("ih_func_decode_iv dw0 %x dw1 %x dw2 %x dw3 %x \n", dw[0], dw[1], dw[2], dw[3]);

	entry->client_id = GSGPU_IH_CLIENTID_LEGACY;
	entry->src_id = dw[0] & 0xff;
	entry->src_data[0] = dw[1] & 0xffffffff;
	entry->ring_id = dw[2] & 0xff;
	entry->vmid = (dw[2] >> 8) & 0xff;
	entry->pasid = (dw[2] >> 16) & 0xffff;
	entry->src_data[1] = dw[3];

	/* wptr/rptr are in bytes! */
	adev->irq.ih.rptr += 4;
}

static void ih_func_set_rptr(struct gsgpu_device *adev)
{
	WREG32(GSGPU_INT_CB_RPTR_OFFSET, adev->irq.ih.rptr);
}

/**
 * gsgpu_ih_ring_alloc - allocate memory for the IH ring
 *
 * @adev: gsgpu_device pointer
 *
 * Allocate a ring buffer for the interrupt controller.
 * Returns 0 for success, errors for failure.
 */
static int gsgpu_ih_ring_alloc(struct gsgpu_device *adev)
{
	int r;

	/* Allocate ring buffer */
	if (adev->irq.ih.ring_obj == NULL) {
		r = gsgpu_bo_create_kernel(adev, adev->irq.ih.ring_size,
					    PAGE_SIZE, GSGPU_GEM_DOMAIN_GTT,
					    &adev->irq.ih.ring_obj,
					    &adev->irq.ih.gpu_addr,
					    (void **)&adev->irq.ih.ring);
		if (r) {
			DRM_ERROR("gsgpu: failed to create ih ring buffer (%d).\n", r);
			return r;
		}
	}
	return 0;
}

/**
 * gsgpu_ih_ring_init - initialize the IH state
 *
 * @adev: gsgpu_device pointer
 *
 * Initializes the IH state and allocates a buffer
 * for the IH ring buffer.
 * Returns 0 for success, errors for failure.
 */
static int gsgpu_ih_ring_init(struct gsgpu_device *adev, unsigned ring_size,
			      bool use_bus_addr)
{
	u32 rb_bufsz;
	int r;

	/* Align ring size */
	rb_bufsz = order_base_2(ring_size / 4);
	ring_size = (1 << rb_bufsz) * 4;
	adev->irq.ih.ring_size = ring_size;
	adev->irq.ih.ptr_mask = adev->irq.ih.ring_size / 4 - 1;
	adev->irq.ih.rptr = 0;
	adev->irq.ih.use_bus_addr = use_bus_addr;

	if (adev->irq.ih.use_bus_addr) {
		if (!adev->irq.ih.ring) {
			/* add 8 bytes for the rptr/wptr shadows and
			 * add them to the end of the ring allocation.
			 */
			adev->irq.ih.ring = dma_alloc_coherent(&adev->pdev->dev,
							       adev->irq.ih.ring_size + 8,
							       &adev->irq.ih.rb_dma_addr,
							       GFP_KERNEL);
			if (adev->irq.ih.ring == NULL)
				return -ENOMEM;
			memset((void *)adev->irq.ih.ring, 0, adev->irq.ih.ring_size + 8);
			adev->irq.ih.wptr_offs = (adev->irq.ih.ring_size / 4) + 0;
			adev->irq.ih.rptr_offs = (adev->irq.ih.ring_size / 4) + 1;
		}
		return 0;
	} else {
		r = gsgpu_device_wb_get(adev, &adev->irq.ih.wptr_offs);
		if (r) {
			dev_err(adev->dev, "(%d) ih wptr_offs wb alloc failed\n", r);
			return r;
		}

		r = gsgpu_device_wb_get(adev, &adev->irq.ih.rptr_offs);
		if (r) {
			gsgpu_device_wb_free(adev, adev->irq.ih.wptr_offs);
			dev_err(adev->dev, "(%d) ih rptr_offs wb alloc failed\n", r);
			return r;
		}

		return gsgpu_ih_ring_alloc(adev);
	}
}

/**
 * gsgpu_ih_ring_fini - tear down the IH state
 *
 * @adev: gsgpu_device pointer
 *
 * Tears down the IH state and frees buffer
 * used for the IH ring buffer.
 */
static void gsgpu_ih_ring_fini(struct gsgpu_device *adev)
{
	if (adev->irq.ih.use_bus_addr) {
		if (adev->irq.ih.ring) {
			/* add 8 bytes for the rptr/wptr shadows and
			 * add them to the end of the ring allocation.
			 */
			dma_free_coherent(&adev->pdev->dev, adev->irq.ih.ring_size + 8,
					  (void *)adev->irq.ih.ring,
					  adev->irq.ih.rb_dma_addr);
			adev->irq.ih.ring = NULL;
		}
	} else {
		gsgpu_bo_free_kernel(&adev->irq.ih.ring_obj,
				      &adev->irq.ih.gpu_addr,
				      (void **)&adev->irq.ih.ring);
		gsgpu_device_wb_free(adev, adev->irq.ih.wptr_offs);
		gsgpu_device_wb_free(adev, adev->irq.ih.rptr_offs);
	}
}

static int gsgpu_ih_early_init(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	gsgpu_ih_set_interrupt_funcs(adev);

	return 0;
}

static int gsgpu_ih_sw_init(void *handle)
{
	int r;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	r = gsgpu_ih_ring_init(adev, 4 * 1024, true);
	if (r)
		return r;

	r = gsgpu_irq_init(adev);
	if (r)
		return r;

	r = gsgpu_hw_sema_mgr_init(adev);

	return r;
}

static int gsgpu_ih_sw_fini(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	gsgpu_irq_fini(adev);
	gsgpu_ih_ring_fini(adev);
	gsgpu_hw_sema_mgr_fini(adev);

	return 0;
}

static int gsgpu_ih_hw_init(void *handle)
{
	int r;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	r = gsgpu_ih_irq_init(adev);
	if (r)
		return r;

	return 0;
}

static int gsgpu_ih_hw_fini(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	gsgpu_ih_irq_disable(adev);

	return 0;
}

static int gsgpu_ih_suspend(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	return gsgpu_ih_hw_fini(adev);
}

static int gsgpu_ih_resume(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	return gsgpu_ih_hw_init(adev);
}

static bool gsgpu_ih_is_idle(void *handle)
{

	return true;
}

static int gsgpu_ih_wait_for_idle(void *handle)
{
	return 0;
}

static const struct gsgpu_ip_funcs gsgpu_ih_ip_funcs = {
	.name = "gsgpu_ih",
	.early_init = gsgpu_ih_early_init,
	.late_init = NULL,
	.sw_init = gsgpu_ih_sw_init,
	.sw_fini = gsgpu_ih_sw_fini,
	.hw_init = gsgpu_ih_hw_init,
	.hw_fini = gsgpu_ih_hw_fini,
	.suspend = gsgpu_ih_suspend,
	.resume = gsgpu_ih_resume,
	.is_idle = gsgpu_ih_is_idle,
	.wait_for_idle = gsgpu_ih_wait_for_idle,
};

static const struct gsgpu_ih_funcs gsgpu_ih_funcs = {
	.get_wptr = ih_func_get_wptr,
	.prescreen_iv = ih_func_prescreen_iv,
	.decode_iv = ih_func_decode_iv,
	.set_rptr = ih_func_set_rptr
};

static void gsgpu_ih_set_interrupt_funcs(struct gsgpu_device *adev)
{
	if (adev->irq.ih_funcs == NULL)
		adev->irq.ih_funcs = &gsgpu_ih_funcs;
}

const struct gsgpu_ip_block_version gsgpu_ih_ip_block = {
	.type = GSGPU_IP_BLOCK_TYPE_IH,
	.major = 3,
	.minor = 0,
	.rev = 0,
	.funcs = &gsgpu_ih_ip_funcs,
};

/**
 * gsgpu_ih_process - interrupt handler
 *
 * @adev: gsgpu_device pointer
 *
 * Interrupt hander  , walk the IH ring.
 * Returns irq process return code.
 */
int gsgpu_ih_process(struct gsgpu_device *adev)
{
	struct gsgpu_iv_entry entry;
	u32 wptr;

	if (!adev->irq.ih.enabled || adev->shutdown)
		return IRQ_NONE;

	if (!adev->irq.msi_enabled)
		WREG32(GSGPU_HOST_INT, 0);

	wptr = gsgpu_ih_get_wptr(adev);

restart_ih:
	/* is somebody else already processing irqs? */
	if (atomic_xchg(&adev->irq.ih.lock, 1))
		return IRQ_NONE;

	DRM_DEBUG("%s: rptr %d, wptr %d\n", __func__, adev->irq.ih.rptr, wptr);

	/* Order reading of wptr vs. reading of IH ring data */
	rmb();

	while (adev->irq.ih.rptr != wptr) {
		u32 ring_index = adev->irq.ih.rptr;

		/* Prescreening of high-frequency interrupts */
		if (!gsgpu_ih_prescreen_iv(adev)) {
			adev->irq.ih.rptr &= adev->irq.ih.ptr_mask;
			continue;
		}

		entry.iv_entry = (const uint32_t *)
			&adev->irq.ih.ring[ring_index];
		gsgpu_ih_decode_iv(adev, &entry);
		adev->irq.ih.rptr &= adev->irq.ih.ptr_mask;

		gsgpu_irq_dispatch(adev, &entry);
	}
	gsgpu_ih_set_rptr(adev);
	atomic_set(&adev->irq.ih.lock, 0);

	/* make sure wptr hasn't changed while processing */
	wptr = gsgpu_ih_get_wptr(adev);
	if (wptr != adev->irq.ih.rptr)
		goto restart_ih;

	return IRQ_HANDLED;
}
