#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>

#include "gsgpu_dc_connector.h"
#include "gsgpu_dc_crtc.h"
#include "gsgpu_dc_encoder.h"
#include "gsgpu_dc_irq.h"
#include "gsgpu_dc_i2c.h"
#include "gsgpu_dc_vbios.h"
#include "gsgpu_backlight.h"

static struct drm_encoder *best_encoder(struct drm_connector *connector)
{
	int enc_id = connector->encoder_ids[0];
	struct drm_mode_object *obj;
	struct drm_encoder *encoder;

	DRM_DEBUG_DRIVER("Finding the best encoder\n");

	/* pick the encoder ids */
	if (enc_id) {
		obj = drm_mode_object_find(connector->dev, NULL, enc_id, DRM_MODE_OBJECT_ENCODER);
		if (!obj) {
			DRM_ERROR("Couldn't find a matching encoder for our connector\n");
			return NULL;
		}
		encoder = obj_to_encoder(obj);
		return encoder;
	}

	DRM_ERROR("No encoder id\n");

	return NULL;
}

static int gsgpu_dc_connector_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct gsgpu_device *lgdev = dev->dev_private;
	struct gsgpu_dc_i2c *i2c = lgdev->i2c[connector->index];
	struct edid *edid = NULL;
	int ret;

	edid = drm_get_edid(connector, &i2c->adapter);

	if (edid) {
		INIT_LIST_HEAD(&connector->probed_modes); /*empty probed_modes*/
		drm_connector_update_edid_property(connector, edid);
		ret = drm_add_edid_modes(connector, edid);
		kfree(edid);
	} else {
		ret = drm_add_modes_noedid(connector, 1920, 1080);
		drm_set_preferred_mode(connector, 1024, 768);
	}

	return ret;
}

static const struct drm_connector_helper_funcs dc_connector_helper_funcs = {
	.get_modes = gsgpu_dc_connector_get_modes,
	.best_encoder = best_encoder
};

static bool is_connected(struct drm_connector *connector)
{
	struct gsgpu_device *adev = connector->dev->dev_private;
	struct gsgpu_dc_i2c *i2c = adev->i2c[connector->index];
	unsigned char start = 0x0;
	struct i2c_adapter *adapter;
	struct i2c_msg msgs = {
		.addr = DDC_ADDR,
		.flags = I2C_M_RD,
		.len = 1,
		.buf = &start,
	};

	if (!i2c)
		return false;

	adapter = &i2c->adapter;
	if (i2c_transfer(adapter, &msgs, 1) != 1) {
		DRM_DEBUG_KMS("display-%d not connect\n", connector->index);
		return false;
	}

	return true;
}

static enum drm_connector_status
gsgpu_2k2000_detect(struct drm_connector *connector)
{
	struct gsgpu_device *adev = connector->dev->dev_private;
	enum drm_connector_status status = connector_status_disconnected;
	u32 reg_val = dc_readl(adev, DC_HDMI_HOTPLUG_STATUS);

	switch (connector->index) {
	case 0:
		if (reg_val & 0x1)
			status = connector_status_connected;
		break;
	case 1:
		if (is_connected(connector))
			status = connector_status_connected;
		else
			status = connector_status_disconnected;
		break;
	}

	return status;
}

static enum drm_connector_status
gsgpu_7a2000_detect(struct drm_connector *connector)
{
	struct gsgpu_device *adev = connector->dev->dev_private;
	enum drm_connector_status status = connector_status_disconnected;
	u32 reg_val = dc_readl(adev, DC_HDMI_HOTPLUG_STATUS);

	switch (connector->index) {
	case 0:
		if (adev->vga_hpd_status == connector_status_unknown)
			status = connector_status_unknown;

		if (reg_val & 0x1)
			status = connector_status_connected;
		else if (status != adev->vga_hpd_status)
			status = connector_status_connected;
		break;
	case 1:
		if (reg_val & 0x2)
			status = connector_status_connected;
		break;
	}

	return status;
}

static enum drm_connector_status
gsgpu_dc_connector_detect(struct drm_connector *connector, bool force)
{
	struct gsgpu_device *adev = connector->dev->dev_private;
	enum drm_connector_status status = connector_status_disconnected;

	if (connector->polled == 0)
		status = connector_status_connected;
	else if (connector->polled == (DRM_CONNECTOR_POLL_CONNECT
				      | DRM_CONNECTOR_POLL_DISCONNECT)) {
		if (is_connected(connector))
			status = connector_status_connected;
		else
			status = connector_status_disconnected;
	} else if (connector->polled == DRM_CONNECTOR_POLL_HPD) {
		switch (adev->chip) {
		case dev_7a2000:
			status = gsgpu_7a2000_detect(connector);
			break;
		case dev_2k2000:
			status = gsgpu_2k2000_detect(connector);
			break;
		}
	}

	return status;
}

static void gsgpu_dc_connector_funcs_reset(struct drm_connector *connector)
{
	struct dc_connector_state *state =
		to_dc_connector_state(connector->state);

	if (connector->state)
		__drm_atomic_helper_connector_destroy_state(connector->state);

	kfree(state);

	state = kzalloc(sizeof(*state), GFP_KERNEL);

	if (state) {
		state->max_bpc = 8;
		__drm_atomic_helper_connector_reset(connector, &state->base);
	}
}

static void gsgpu_dc_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
	kfree(connector);
}

struct drm_connector_state *
gsgpu_dc_connector_atomic_duplicate_state(struct drm_connector *connector)
{
	struct dc_connector_state *state =
		to_dc_connector_state(connector->state);

	struct dc_connector_state *new_state =
			kmemdup(state, sizeof(*state), GFP_KERNEL);

	if (new_state) {
		__drm_atomic_helper_connector_duplicate_state(connector,
							      &new_state->base);
		new_state->max_bpc = state->max_bpc;
		return &new_state->base;
	}

	return NULL;
}

static const struct drm_connector_funcs gsgpu_dc_connector_funcs = {
	.detect = gsgpu_dc_connector_detect,
	.reset = gsgpu_dc_connector_funcs_reset,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = gsgpu_dc_connector_destroy,
	.atomic_duplicate_state = gsgpu_dc_connector_atomic_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.late_register = gsgpu_backlight_register
};

struct gsgpu_dc_connector *dc_connector_construct(struct gsgpu_dc *dc, struct connector_resource *resource)
{
	struct gsgpu_dc_connector *connector;
	u32 link;

	if (IS_ERR_OR_NULL(dc) || IS_ERR_OR_NULL(resource))
		return NULL;

	connector = kzalloc(sizeof(*connector), GFP_KERNEL);
	if (IS_ERR_OR_NULL(connector))
		return NULL;

	connector->dc = dc;
	connector->resource = resource;

	link = connector->resource->base.link;
	if (link >= DC_DVO_MAXLINK)
		return false;

	list_add_tail(&connector->node, &dc->connector_list);

	return connector;
}

int gsgpu_dc_connector_init(struct gsgpu_device *adev, uint32_t link_index)
{
	struct gsgpu_dc_connector *dc_connector;
	struct gsgpu_connector *lconnector;
	int res = 0;

	DRM_DEBUG_DRIVER("%s()\n", __func__);

	if (adev->dc->link_info[link_index].encoder->has_ext_encoder)
		return 0;

	if (link_index >= 2)
		return -1;

	lconnector = kzalloc(sizeof(*lconnector), GFP_KERNEL);
	if (!lconnector)
		return -ENOMEM;

	dc_connector = adev->dc->link_info[link_index].connector;
	res = drm_connector_init(adev->ddev, &lconnector->base,
				 &gsgpu_dc_connector_funcs,
				 dc_connector->resource->type);
	if (res) {
		DRM_ERROR("connector_init failed\n");
		lconnector->connector_id = -1;
		goto out_free;
	}

	drm_connector_helper_add(&lconnector->base, &dc_connector_helper_funcs);

	adev->mode_info.connectors[link_index] = lconnector;
	lconnector->connector_id = link_index;
	lconnector->base.dpms = DRM_MODE_DPMS_OFF;

	mutex_init(&lconnector->hpd_lock);

	switch (adev->chip) {
	case dev_7a2000:
		if (link_index == 0) {
			lconnector->irq_source_i2c = DC_IRQ_SOURCE_I2C0;
			lconnector->irq_source_hpd = DC_IRQ_SOURCE_HPD_HDMI0;
			lconnector->irq_source_vga_hpd = DC_IRQ_SOURCE_HPD_VGA;
		} else if (link_index == 1) {
			lconnector->irq_source_i2c = DC_IRQ_SOURCE_I2C1;
			lconnector->irq_source_hpd = DC_IRQ_SOURCE_HPD_HDMI1;
		}
		if (dc_connector->resource->type == DRM_MODE_CONNECTOR_VGA)
			dc_connector->resource->hotplug = POLLING;
		break;
	case dev_2k2000:
		if (link_index == 0) {
			lconnector->irq_source_i2c = DC_IRQ_SOURCE_I2C0;
			lconnector->irq_source_hpd = DC_IRQ_SOURCE_HPD_HDMI0;
		} else if (link_index == 1) {
			lconnector->irq_source_i2c = DC_IRQ_SOURCE_I2C1;
			lconnector->irq_source_hpd = DC_IRQ_SOURCE_HPD_HDMI1_NULL;
		}
		break;
	}

	switch (dc_connector->resource->hotplug) {
	case IRQ:
		lconnector->base.polled = DRM_CONNECTOR_POLL_HPD;
		break;
	case POLLING:
	default:
		lconnector->base.polled = DRM_CONNECTOR_POLL_CONNECT |
					  DRM_CONNECTOR_POLL_DISCONNECT;
		break;
	case FORCE_ON:
		lconnector->base.polled = 0;
		break;
	}

	drm_connector_register(&lconnector->base);

out_free:
	if (res) {
		kfree(lconnector);
	}

	return res;
}

