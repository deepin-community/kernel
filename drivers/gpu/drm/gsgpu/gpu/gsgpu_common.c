#include <linux/slab.h>
#include "gsgpu.h"
#include "gsgpu_ih.h"
#include "gsgpu_common.h"
#include "gsgpu_mmu.h"
#include "gsgpu_zip.h"
#include "gsgpu_gfx.h"
#include "gsgpu_xdma.h"
#include "gsgpu_dc_reg.h"

static u32 gsgpu_get_clk(struct gsgpu_device *adev)
{
	DRM_DEBUG_DRIVER("%s Not implemented\n", __func__);

	return 0;
}

static void gsgpu_vga_set_state(struct gsgpu_device *adev, bool state)
{
	return;
/* TODO: In this function we should be enable\disable GPU&DC */
/*	u32 conf_reg;
	u32 i;

	for (i = 0; i < adev->mode_info.num_crtc; i++) {
		conf_reg = dc_readl(adev, CURRENT_REG(DC_CRTC_CFG_REG, i));
		if (state)
			conf_reg |= CRTC_CFG_ENABLE;
		else
			conf_reg &= ~CRTC_CFG_ENABLE;
		dc_writel(adev, CURRENT_REG(DC_CRTC_CFG_REG, i), conf_reg);
	}
*/
}

static bool gsgpu_read_bios_from_rom(struct gsgpu_device *adev,
				  u8 *bios, u32 length_bytes)
{
	DRM_DEBUG_DRIVER("%s Not implemented\n", __func__);

	return true;
}

static int gsgpu_read_register(struct gsgpu_device *adev, u32 se_num,
			    u32 sh_num, u32 reg_offset, u32 *value)
{
	DRM_DEBUG_DRIVER("%s Not implemented\n", __func__);
	*value = 0;
	return 0;
}

static int gsgpu_gpu_pci_config_reset(struct gsgpu_device *adev)
{
	u32 i;

	dev_info(adev->dev, "GPU pci config reset\n");

	/* disable BM */
	pci_clear_master(adev->pdev);
	/* reset */
	gsgpu_device_pci_config_reset(adev);

	udelay(100);

	/* wait for asic to come out of reset */
	for (i = 0; i < adev->usec_timeout; i++) {
		if (1) {
			/* enable BM */
			pci_set_master(adev->pdev);
			adev->has_hw_reset = true;
			return 0;
		}
		udelay(1);
	}
	return -EINVAL;
}

static int gsgpu_reset(struct gsgpu_device *adev)
{
	int r;

	/*XXX Set pcie config regs not Need*/
	return 0;

	r = gsgpu_gpu_pci_config_reset(adev);

	return r;
}

static bool gsgpu_need_full_reset(struct gsgpu_device *adev)
{
	switch (adev->family_type) {
	case CHIP_LG100:
	default:
		/* change this when we support soft reset */
		return true;
	}
}

static const struct gsgpu_asic_funcs gsgpu_asic_funcs = {
	.read_bios_from_rom = &gsgpu_read_bios_from_rom,
	.read_register = &gsgpu_read_register,
	.reset = &gsgpu_reset,
	.set_vga_state = &gsgpu_vga_set_state,
	.get_clk = &gsgpu_get_clk,
	.need_full_reset = &gsgpu_need_full_reset,
};

static int gsgpu_common_early_init(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	DRM_DEBUG_DRIVER("%s Not implemented\n", __func__);

	adev->asic_funcs = &gsgpu_asic_funcs;

	return 0;
}

static int gsgpu_common_late_init(void *handle)
{
	return 0;
}

static int gsgpu_common_sw_init(void *handle)
{
	return 0;
}

static int gsgpu_common_sw_fini(void *handle)
{
	return 0;
}

static int gsgpu_common_hw_init(void *handle)
{
	return 0;
}

static int gsgpu_common_hw_fini(void *handle)
{
	return 0;
}

static int gsgpu_common_suspend(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	return gsgpu_common_hw_fini(adev);
}

static int gsgpu_common_resume(void *handle)
{
	struct gsgpu_device *adev = (struct gsgpu_device *)handle;

	return gsgpu_common_hw_init(adev);
}

static bool gsgpu_common_is_idle(void *handle)
{
	return true;
}

static int gsgpu_common_wait_for_idle(void *handle)
{
	return 0;
}

static int gsgpu_common_soft_reset(void *handle)
{
	return 0;
}

static const struct gsgpu_ip_funcs gsgpu_common_ip_funcs = {
	.name = "gsgpu_common",
	.early_init = gsgpu_common_early_init,
	.late_init = gsgpu_common_late_init,
	.sw_init = gsgpu_common_sw_init,
	.sw_fini = gsgpu_common_sw_fini,
	.hw_init = gsgpu_common_hw_init,
	.hw_fini = gsgpu_common_hw_fini,
	.suspend = gsgpu_common_suspend,
	.resume = gsgpu_common_resume,
	.is_idle = gsgpu_common_is_idle,
	.wait_for_idle = gsgpu_common_wait_for_idle,
	.soft_reset = gsgpu_common_soft_reset,
};

static const struct gsgpu_ip_block_version gsgpu_common_ip_block = {
	.type = GSGPU_IP_BLOCK_TYPE_COMMON,
	.major = 1,
	.minor = 0,
	.rev = 0,
	.funcs = &gsgpu_common_ip_funcs,
};

int gsgpu_set_ip_blocks(struct gsgpu_device *adev)
{
	switch (adev->family_type) {
	case CHIP_LG100:
		gsgpu_device_ip_block_add(adev, &gsgpu_common_ip_block);
		gsgpu_device_ip_block_add(adev, &mmu_ip_block);
		gsgpu_device_ip_block_add(adev, &zip_ip_block);
		gsgpu_device_ip_block_add(adev, &gsgpu_ih_ip_block);
		gsgpu_device_ip_block_add(adev, &dc_ip_block);
		gsgpu_device_ip_block_add(adev, &gfx_ip_block);
		gsgpu_device_ip_block_add(adev, &xdma_ip_block);
		break;
	default:
		/* FIXME: not supported yet */
		return -EINVAL;
	}

	return 0;
}
