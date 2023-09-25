#include <linux/irq.h>
#include <linux/pm_runtime.h>

#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>
#include <drm/gsgpu_drm.h>

#include "gsgpu.h"
#include "gsgpu_ih.h"
#include "gsgpu_trace.h"
#include "gsgpu_dc_irq.h"
#include "gsgpu_dc_reg.h"
#include "gsgpu_irq.h"

#define GSGPU_WAIT_IDLE_TIMEOUT 200

/**
 * gsgpu_irq_reset_work_func - execute GPU reset
 *
 * @work: work struct pointer
 *
 * Execute scheduled GPU reset (Cayman+).
 * This function is called when the IRQ handler thinks we need a GPU reset.
 */
static void gsgpu_irq_reset_work_func(struct work_struct *work)
{
	struct gsgpu_device *adev = container_of(work, struct gsgpu_device,
						  reset_work);

	gsgpu_device_gpu_recover(adev, NULL, false);
}

/**
 * gsgpu_irq_disable_all - disable *all* interrupts
 *
 * @adev: gsgpu device pointer
 *
 * Disable all types of interrupts from all sources.
 */
void gsgpu_irq_disable_all(struct gsgpu_device *adev)
{
	unsigned long irqflags;
	unsigned i, j, k;
	int r;

	spin_lock_irqsave(&adev->irq.lock, irqflags);
	for (i = 0; i < GSGPU_IH_CLIENTID_MAX; ++i) {
		if (!adev->irq.client[i].sources)
			continue;

		for (j = 0; j < GSGPU_MAX_IRQ_SRC_ID; ++j) {
			struct gsgpu_irq_src *src = adev->irq.client[i].sources[j];

			if (!src || !src->funcs->set || !src->num_types)
				continue;

			for (k = 0; k < src->num_types; ++k) {
				atomic_set(&src->enabled_types[k], 0);
				r = src->funcs->set(adev, src, k,
						    GSGPU_IRQ_STATE_DISABLE);
				if (r)
					DRM_ERROR("error disabling interrupt (%d)\n",
						  r);
			}
		}
	}
	spin_unlock_irqrestore(&adev->irq.lock, irqflags);
}

/**
 * gsgpu_irq_handler - IRQ handler
 *
 * @irq: IRQ number (unused)
 * @arg: pointer to DRM device
 *
 * IRQ handler for gsgpu driver (all ASICs).
 *
 * Returns:
 * result of handling the IRQ, as defined by &irqreturn_t
 */
static irqreturn_t gsgpu_irq_handler(int irq, void *arg)
{
	struct drm_device *dev = (struct drm_device *) arg;
	struct gsgpu_device *adev = dev->dev_private;
	irqreturn_t ret;

	ret = gsgpu_ih_process(adev);
	if (ret == IRQ_HANDLED)
		pm_runtime_mark_last_busy(dev->dev);
	return ret;
}

static irqreturn_t gsgpu_dc_irq_handler(int irq, void *arg)
{
	u32 int_reg;
	unsigned long base;
	struct gsgpu_iv_entry entry;
	struct gsgpu_device *adev = (struct gsgpu_device *)arg;
	int i = 1;

	base = (unsigned long)(adev->loongson_dc_rmmio_base);

	int_reg = dc_readl(adev, DC_INT_REG);
	dc_writel(adev, DC_INT_REG, int_reg);
	entry.client_id = SOC15_IH_CLIENTID_DCE;

	int_reg &= 0xffff;
	while (int_reg && (i < DC_INT_ID_MAX)) {
		if (!(int_reg & 0x1)) {
			int_reg = int_reg >> 1;
			i++;
			continue;
		}

		entry.src_id = i;
		gsgpu_irq_dispatch(adev, &entry);

		int_reg = int_reg >> 1;
		i++;
	}

	return IRQ_HANDLED;
}

static int gsgpu_irq_install(struct gsgpu_device *adev, int irq)
{
	struct drm_device *dev = adev->ddev;
	int ret;

	if (irq == IRQ_NOTCONNECTED)
		return -ENOTCONN;

	/* PCI devices require shared interrupts. */
	ret = request_irq(irq, gsgpu_irq_handler,
			  IRQF_SHARED, dev->driver->name, dev);
	if (ret)
		return ret;

	return 0;
}

static void gsgpu_irq_uninstall(struct gsgpu_device *adev)
{
	struct drm_device *dev = adev->ddev;
	struct pci_dev *pdev = to_pci_dev(dev->dev);

	free_irq(pdev->irq, dev);
}

/**
 * gsgpu_msi_ok - check whether MSI functionality is enabled
 *
 * @adev: gsgpu device pointer (unused)
 *
 * Checks whether MSI functionality has been disabled via module parameter
 * (all ASICs).
 *
 * Returns:
 * *true* if MSIs are allowed to be enabled or *false* otherwise
 */
static bool gsgpu_msi_ok(struct gsgpu_device *adev)
{
	if (gsgpu_msi == 1)
		return true;
	else if (gsgpu_msi == 0)
		return false;

	return true;
}

/**
 * gsgpu_irq_init - initialize interrupt handling
 *
 * @adev: gsgpu device pointer
 *
 * Sets up work functions for hotplug and reset interrupts, enables MSI
 * functionality, initializes vblank, hotplug and reset interrupt handling.
 *
 * Returns:
 * 0 on success or error code on failure
 */
int gsgpu_irq_init(struct gsgpu_device *adev)
{
	int r = 0;
	struct pci_dev *loongson_dc;

	spin_lock_init(&adev->irq.lock);

	/* Enable MSI if not disabled by module parameter */
	adev->irq.msi_enabled = false;

	if (gsgpu_msi_ok(adev)) {
		int ret = pci_enable_msi(adev->pdev);
		if (!ret) {
			adev->irq.msi_enabled = true;
			dev_dbg(adev->dev, "gsgpu: using MSI.\n");
		}
	}

	INIT_WORK(&adev->reset_work, gsgpu_irq_reset_work_func);

	adev->irq.installed = true;
	r = gsgpu_irq_install(adev, adev->ddev->pdev->irq);
	if (r) {
		adev->irq.installed = false;
		cancel_work_sync(&adev->reset_work);
		return r;
	}
	adev->ddev->max_vblank_count = 0x00ffffff;

	loongson_dc = adev->loongson_dc;
	if (loongson_dc) {
		u32 dc_irq = loongson_dc->irq;
		r = request_irq(dc_irq, gsgpu_dc_irq_handler, 0,
				loongson_dc->driver->name, adev);
		if (r) {
			DRM_ERROR("gsgpu register dc irq failed\n");
			return r;
		}
	} else {
		DRM_ERROR("gsgpu dc device is null\n");
		return ENODEV;
	}

	DRM_DEBUG("gsgpu: irq initialized.\n");
	return 0;
}

/**
 * gsgpu_irq_fini - shut down interrupt handling
 *
 * @adev: gsgpu device pointer
 *
 * Tears down work functions for hotplug and reset interrupts, disables MSI
 * functionality, shuts down vblank, hotplug and reset interrupt handling,
 * turns off interrupts from all sources (all ASICs).
 */
void gsgpu_irq_fini(struct gsgpu_device *adev)
{
	unsigned i, j;

	if (adev->irq.installed) {
		gsgpu_irq_uninstall(adev);
		adev->irq.installed = false;
		if (adev->irq.msi_enabled)
			pci_disable_msi(adev->pdev);
		cancel_work_sync(&adev->reset_work);
	}

	for (i = 0; i < GSGPU_IH_CLIENTID_MAX; ++i) {
		if (!adev->irq.client[i].sources)
			continue;

		for (j = 0; j < GSGPU_MAX_IRQ_SRC_ID; ++j) {
			struct gsgpu_irq_src *src = adev->irq.client[i].sources[j];

			if (!src)
				continue;

			kfree(src->enabled_types);
			src->enabled_types = NULL;
			if (src->data) {
				kfree(src->data);
				kfree(src);
				adev->irq.client[i].sources[j] = NULL;
			}
		}
		kfree(adev->irq.client[i].sources);
		adev->irq.client[i].sources = NULL;
	}
}

/**
 * gsgpu_irq_add_id - register IRQ source
 *
 * @adev: gsgpu device pointer
 * @client_id: client id
 * @src_id: source id
 * @source: IRQ source pointer
 *
 * Registers IRQ source on a client.
 *
 * Returns:
 * 0 on success or error code otherwise
 */
int gsgpu_irq_add_id(struct gsgpu_device *adev,
		      unsigned client_id, unsigned src_id,
		      struct gsgpu_irq_src *source)
{
	if (client_id >= GSGPU_IH_CLIENTID_MAX)
		return -EINVAL;

	if (src_id >= GSGPU_MAX_IRQ_SRC_ID)
		return -EINVAL;

	if (!source->funcs)
		return -EINVAL;

	if (!adev->irq.client[client_id].sources) {
		adev->irq.client[client_id].sources =
			kcalloc(GSGPU_MAX_IRQ_SRC_ID,
				sizeof(struct gsgpu_irq_src *),
				GFP_KERNEL);
		if (!adev->irq.client[client_id].sources)
			return -ENOMEM;
	}

	if (adev->irq.client[client_id].sources[src_id] != NULL)
		return -EINVAL;

	if (source->num_types && !source->enabled_types) {
		atomic_t *types;

		types = kcalloc(source->num_types, sizeof(atomic_t),
				GFP_KERNEL);
		if (!types)
			return -ENOMEM;

		source->enabled_types = types;
	}

	adev->irq.client[client_id].sources[src_id] = source;
	return 0;
}

/**
 * gsgpu_irq_dispatch - dispatch IRQ to IP blocks
 *
 * @adev: gsgpu device pointer
 * @entry: interrupt vector pointer
 *
 * Dispatches IRQ to IP blocks.
 */
void gsgpu_irq_dispatch(struct gsgpu_device *adev,
			 struct gsgpu_iv_entry *entry)
{
	unsigned client_id = entry->client_id;
	unsigned src_id = entry->src_id;
	struct gsgpu_irq_src *src;
	int r;

	trace_gsgpu_iv(entry);

	if (client_id >= GSGPU_IH_CLIENTID_MAX) {
		DRM_DEBUG("Invalid client_id in IV: %d\n", client_id);
		return;
	}

	if (src_id >= GSGPU_MAX_IRQ_SRC_ID) {
		DRM_DEBUG("Invalid src_id in IV: %d\n", src_id);
		return;
	}

	if (!adev->irq.client[client_id].sources) {
		DRM_DEBUG("Unregistered interrupt client_id: %d src_id: %d\n",
			  client_id, src_id);
		return;
	}

	src = adev->irq.client[client_id].sources[src_id];
	if (!src) {
		DRM_DEBUG("Unhandled interrupt src_id: %d\n", src_id);
		return;
	}

	r = src->funcs->process(adev, src, entry);
	if (r)
		DRM_ERROR("error processing interrupt (%d)\n", r);
}

/**
 * gsgpu_irq_update - update hardware interrupt state
 *
 * @adev: gsgpu device pointer
 * @src: interrupt source pointer
 * @type: type of interrupt
 *
 * Updates interrupt state for the specific source (all ASICs).
 */
int gsgpu_irq_update(struct gsgpu_device *adev,
			     struct gsgpu_irq_src *src, unsigned type)
{
	unsigned long irqflags;
	enum gsgpu_interrupt_state state;
	int r;

	spin_lock_irqsave(&adev->irq.lock, irqflags);

	/* We need to determine after taking the lock, otherwise
	   we might disable just enabled interrupts again */
	if (gsgpu_irq_enabled(adev, src, type))
		state = GSGPU_IRQ_STATE_ENABLE;
	else
		state = GSGPU_IRQ_STATE_DISABLE;

	r = src->funcs->set(adev, src, type, state);
	spin_unlock_irqrestore(&adev->irq.lock, irqflags);
	return r;
}

/**
 * gsgpu_irq_gpu_reset_resume_helper - update interrupt states on all sources
 *
 * @adev: gsgpu device pointer
 *
 * Updates state of all types of interrupts on all sources on resume after
 * reset.
 */
void gsgpu_irq_gpu_reset_resume_helper(struct gsgpu_device *adev)
{
	int i, j, k;

	for (i = 0; i < GSGPU_IH_CLIENTID_MAX; ++i) {
		if (!adev->irq.client[i].sources)
			continue;

		for (j = 0; j < GSGPU_MAX_IRQ_SRC_ID; ++j) {
			struct gsgpu_irq_src *src = adev->irq.client[i].sources[j];

			if (!src)
				continue;
			for (k = 0; k < src->num_types; k++)
				gsgpu_irq_update(adev, src, k);
		}
	}
}

/**
 * gsgpu_irq_get - enable interrupt
 *
 * @adev: gsgpu device pointer
 * @src: interrupt source pointer
 * @type: type of interrupt
 *
 * Enables specified type of interrupt on the specified source (all ASICs).
 *
 * Returns:
 * 0 on success or error code otherwise
 */
int gsgpu_irq_get(struct gsgpu_device *adev, struct gsgpu_irq_src *src,
		   unsigned type)
{
	if (!adev->ddev->irq_enabled)
		return -ENOENT;

	if (type >= src->num_types)
		return -EINVAL;

	if (!src->enabled_types || !src->funcs->set)
		return -EINVAL;

	if (atomic_inc_return(&src->enabled_types[type]) == 1)
		return gsgpu_irq_update(adev, src, type);

	return 0;
}

/**
 * gsgpu_irq_put - disable interrupt
 *
 * @adev: gsgpu device pointer
 * @src: interrupt source pointer
 * @type: type of interrupt
 *
 * Enables specified type of interrupt on the specified source (all ASICs).
 *
 * Returns:
 * 0 on success or error code otherwise
 */
int gsgpu_irq_put(struct gsgpu_device *adev, struct gsgpu_irq_src *src,
		   unsigned type)
{
	if (!adev->ddev->irq_enabled)
		return -ENOENT;

	if (type >= src->num_types)
		return -EINVAL;

	if (!src->enabled_types || !src->funcs->set)
		return -EINVAL;

	if (atomic_dec_and_test(&src->enabled_types[type]))
		return gsgpu_irq_update(adev, src, type);

	return 0;
}

/**
 * gsgpu_irq_enabled - check whether interrupt is enabled or not
 *
 * @adev: gsgpu device pointer
 * @src: interrupt source pointer
 * @type: type of interrupt
 *
 * Checks whether the given type of interrupt is enabled on the given source.
 *
 * Returns:
 * *true* if interrupt is enabled, *false* if interrupt is disabled or on
 * invalid parameters
 */
bool gsgpu_irq_enabled(struct gsgpu_device *adev, struct gsgpu_irq_src *src,
			unsigned type)
{
	if (!adev->ddev->irq_enabled)
		return false;

	if (type >= src->num_types)
		return false;

	if (!src->enabled_types || !src->funcs->set)
		return false;

	return !!atomic_read(&src->enabled_types[type]);
}
