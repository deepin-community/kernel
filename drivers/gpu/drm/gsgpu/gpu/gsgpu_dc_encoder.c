#include <drm/drm_encoder.h>
#include <drm/drm_atomic_helper.h>
#include "gsgpu.h"
#include "gsgpu_dc.h"
#include "gsgpu_dc_encoder.h"
#include "bridge_phy.h"

static void dc_encoder_destroy(struct drm_encoder *encoder)
{
	struct gsgpu_encoder *lencoder = to_gsgpu_encoder(encoder);

	drm_encoder_cleanup(encoder);
	kfree(lencoder);
}

static const struct drm_encoder_funcs dc_encoder_funcs = {
	.destroy = dc_encoder_destroy,
};

static void dc_encoder_helper_disable(struct drm_encoder *encoder)
{
}

static int dc_encoder_helper_atomic_check(struct drm_encoder *encoder,
					  struct drm_crtc_state *crtc_state,
					  struct drm_connector_state *conn_state)
{
	struct gsgpu_encoder *lencoder = to_gsgpu_encoder(encoder);

	bridge_phy_mode_set(lencoder->bridge, &crtc_state->mode, NULL);

	return 0;
}

const struct drm_encoder_helper_funcs dc_encoder_helper_funcs = {
	.disable = dc_encoder_helper_disable,
	.atomic_check = dc_encoder_helper_atomic_check
};

struct gsgpu_dc_encoder *dc_encoder_construct(struct gsgpu_dc *dc, struct encoder_resource *resource)
{
	struct gsgpu_dc_encoder *encoder;
	u32 link;

	if (IS_ERR_OR_NULL(dc) || IS_ERR_OR_NULL(resource))
		return NULL;

	encoder = kzalloc(sizeof(*encoder), GFP_KERNEL);

	if (IS_ERR_OR_NULL(encoder))
		return NULL;

	encoder->dc = dc;
	encoder->resource = resource;
	encoder->has_ext_encoder = false;

	link = encoder->resource->base.link;
	if (link >= DC_DVO_MAXLINK)
		return false;

	return encoder;
}

int gsgpu_dc_encoder_init(struct gsgpu_device *adev, int link_index)
{
	struct gsgpu_encoder *lencoder;
	int res;

	if (link_index >= 2)
		return -1;

	lencoder = kzalloc(sizeof(*lencoder), GFP_KERNEL);
	if (!lencoder)
		return -ENOMEM;

	res = drm_encoder_init(adev->ddev, &lencoder->base,
			       &dc_encoder_funcs,
			       DRM_MODE_ENCODER_TMDS, NULL);
	if (!res)
		lencoder->encoder_id = link_index;
	else
		lencoder->encoder_id = -1;

	lencoder->base.possible_crtcs = 1 << link_index;

	adev->mode_info.encoders[link_index] = lencoder;
	adev->mode_info.encoders[link_index]->bridge = NULL;

	drm_encoder_helper_add(&lencoder->base, &dc_encoder_helper_funcs);

	return res;
}
