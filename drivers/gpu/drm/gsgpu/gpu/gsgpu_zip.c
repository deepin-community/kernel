#include <linux/firmware.h>
#include <drm/drmP.h>
#include <drm/drm_cache.h>
#include "gsgpu.h"
#include "gsgpu_zip.h"

/**
 * zip_meta_enable - gart enable
 *
 * @adev: gsgpu_device pointer
 *
 * This sets up the TLBs, programs the page tables for VMID0,
 * sets up the hw for VMIDs 1-15 which are allocated on
 * demand, and sets up the global locations for the LDS, GDS,
 * and GPUVM for FSA64 clients ().
 * Returns 0 for success, errors for failure.
 */
static int zip_meta_enable(struct gsgpu_device *adev, bool clear)
{
	int r;

	if (adev->zip_meta.robj == NULL) {
		dev_err(adev->dev, "No VRAM object for PCIE ZIP_META.\n");
		return -EINVAL;
	}
	r = gsgpu_zip_meta_vram_pin(adev);
	if (r)
		return r;

	if (clear)
		memset(adev->zip_meta.ptr, 0x00, adev->zip_meta.table_size);

	gsgpu_cmd_exec(adev, GSCMD(GSCMD_ZIP, ZIP_DISABLE), 0, 0);
	gsgpu_cmd_exec(adev, GSCMDi(GSCMD_ZIP, ZIP_SET_BASE, 0), \
			lower_32_bits(adev->zip_meta.table_addr), upper_32_bits(adev->zip_meta.table_addr));

	gsgpu_cmd_exec(adev, GSCMDi(GSCMD_ZIP, ZIP_SET_MASK, 0), \
			lower_32_bits(adev->zip_meta.mask), upper_32_bits(adev->zip_meta.mask));

	gsgpu_cmd_exec(adev, GSCMD(GSCMD_ZIP, ZIP_ENABLE), 0, 0);

	DRM_INFO("PCIE ZIP META of %uM enabled (table at 0x%016llX).\n",
		 (unsigned)(adev->zip_meta.table_size >> 20),
		 (unsigned long long)adev->zip_meta.table_addr);
	adev->zip_meta.ready = true;
	return 0;
}

static int zip_meta_init(struct gsgpu_device *adev)
{
	int r;

	if (adev->zip_meta.robj) {
		WARN(1, "GSGPU PCIE ZIP_META already initialized\n");
		return 0;
	}
	/* Initialize common zip_meta structure */
	r = gsgpu_zip_meta_init(adev);
	if (r)
		return r;
	adev->zip_meta.table_size = adev->zip_meta.num_gpu_pages *
		GSGPU_GPU_PAGE_SIZE;
	adev->zip_meta.mask = roundup_pow_of_two(adev->zip_meta.table_size) - 1;
	adev->zip_meta.pte_flags = 0;
	return gsgpu_zip_meta_vram_alloc(adev);
}

/**
 * zip_meta_v1_0_gart_disable - zip meta disable
 *
 * @adev: gsgpu_device pointer
 *
 * This disables all zip meta page table ().
 */
static int zip_meta_disable(struct gsgpu_device *adev)
{
	if (adev->zip_meta.robj == NULL) {
		dev_err(adev->dev, "No VRAM object for PCIE ZIP_META.\n");
		return -EINVAL;
	}

	gsgpu_cmd_exec(adev, GSCMD(GSCMD_ZIP, ZIP_DISABLE), 0, 0);

	adev->zip_meta.ready = false;

	gsgpu_zip_meta_vram_unpin(adev);
	return 0;
}

static int zip_early_init(void *handle)
{
	return 0;
}

static int zip_late_init(void *handle)
{
	return 0;
}

static int zip_sw_init(void *handle)
{
	int r;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	r = zip_meta_init(adev);
	if (r)
		return r;

	return 0;
}

static int zip_sw_fini(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	gsgpu_zip_meta_vram_free(adev);
	gsgpu_zip_meta_fini(adev);

	return 0;
}

static int zip_hw_init(void *handle)
{
	int r;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	r = zip_meta_enable(adev, true);
	if (r)
		return r;

	return r;
}

static int zip_hw_fini(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	zip_meta_disable(adev);

	return 0;
}

static int zip_suspend(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	zip_hw_fini(adev);

	return 0;
}

static int zip_resume(void *handle)
{
	int r;
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	r = zip_meta_enable(adev, false);
	if (r)
		return r;

	return 0;
}

static bool zip_is_idle(void *handle)
{
	return true;
}

static bool zip_check_soft_reset(void *handle)
{
	return false;
}

static int zip_pre_soft_reset(void *handle)
{
	return 0;
}

static int zip_soft_reset(void *handle)
{
	return 0;
}

static int zip_post_soft_reset(void *handle)
{
	return 0;
}

static const struct gsgpu_ip_funcs zip_ip_funcs = {
	.name = "zip",
	.early_init = zip_early_init,
	.late_init = zip_late_init,
	.sw_init = zip_sw_init,
	.sw_fini = zip_sw_fini,
	.hw_init = zip_hw_init,
	.hw_fini = zip_hw_fini,
	.suspend = zip_suspend,
	.resume = zip_resume,
	.is_idle = zip_is_idle,
	.wait_for_idle = NULL,
	.check_soft_reset = zip_check_soft_reset,
	.pre_soft_reset = zip_pre_soft_reset,
	.soft_reset = zip_soft_reset,
	.post_soft_reset = zip_post_soft_reset,
};

const struct gsgpu_ip_block_version zip_ip_block = {
	.type = GSGPU_IP_BLOCK_TYPE_ZIP,
	.major = 1,
	.minor = 0,
	.rev = 0,
	.funcs = &zip_ip_funcs,
};
