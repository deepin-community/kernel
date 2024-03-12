// SPDX-License-Identifier: GPL-2.0

#include <drm/drm_atomic_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>

#include "inspur_drm_drv.h"
#include "inspur_drm_regs.h"

static int inspur_connector_get_modes(struct drm_connector *connector)
{
	int count;

	count = drm_add_modes_noedid(connector,
				     connector->dev->mode_config.max_width,
				     connector->dev->mode_config.max_height);
	drm_set_preferred_mode(connector, 1024, 768);
	return count;
}

static int inspur_connector_mode_valid(struct drm_connector *connector,
				       struct drm_display_mode *mode)
{
	return MODE_OK;
}

static const struct drm_connector_helper_funcs inspur_connector_helper_funcs = {
	.get_modes = inspur_connector_get_modes,
	.mode_valid = inspur_connector_mode_valid,
};

static const struct drm_connector_funcs inspur_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static void inspur_encoder_mode_set(struct drm_encoder *encoder,
				    struct drm_display_mode *mode,
				    struct drm_display_mode *adj_mode)
{
	u32 reg;
	struct drm_device *dev = encoder->dev;
	struct inspur_drm_private *priv = dev->dev_private;

	reg = readl(priv->mmio + INSPUR_DISPLAY_CONTROL_HISILE);
	reg |= INSPUR_DISPLAY_CONTROL_FPVDDEN(1);
	reg |= INSPUR_DISPLAY_CONTROL_PANELDATE(1);
	reg |= INSPUR_DISPLAY_CONTROL_FPEN(1);
	reg |= INSPUR_DISPLAY_CONTROL_VBIASEN(1);
	writel(reg, priv->mmio + INSPUR_DISPLAY_CONTROL_HISILE);
}

static const struct drm_encoder_helper_funcs inspur_encoder_helper_funcs = {
	.mode_set = inspur_encoder_mode_set,
};

static const struct drm_encoder_funcs inspur_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

int inspur_vdac_init(struct inspur_drm_private *priv)
{
	struct drm_device *dev = priv->dev;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	int ret;

	encoder = devm_kzalloc(dev->dev, sizeof(*encoder), GFP_KERNEL);
	if (!encoder) {
		DRM_ERROR("failed to alloc memory when init encoder\n");
		return -ENOMEM;
	}

	encoder->possible_crtcs = 0x1;
	ret = drm_encoder_init(dev, encoder, &inspur_encoder_funcs,
			       DRM_MODE_ENCODER_DAC, NULL);
	if (ret) {
		DRM_ERROR("failed to init encoder: %d\n", ret);
		return ret;
	}

	drm_encoder_helper_add(encoder, &inspur_encoder_helper_funcs);

	connector = devm_kzalloc(dev->dev, sizeof(*connector), GFP_KERNEL);
	if (!connector) {
		DRM_ERROR("failed to alloc memory when init connector\n");
		return -ENOMEM;
	}

	ret = drm_connector_init(dev, connector,
				 &inspur_connector_funcs,
				 DRM_MODE_CONNECTOR_VGA);
	if (ret) {
		DRM_ERROR("failed to init connector: %d\n", ret);
		return ret;
	}
	drm_connector_helper_add(connector, &inspur_connector_helper_funcs);

	drm_connector_register(connector);
	drm_connector_attach_encoder(connector, encoder);
	return 0;
}
