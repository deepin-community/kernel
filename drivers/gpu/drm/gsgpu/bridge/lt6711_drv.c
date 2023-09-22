
#include "gsgpu.h"
#include "gsgpu_dc.h"
#include "gsgpu_dc_vbios.h"
#include "bridge_phy.h"

static enum drm_mode_status lt6711_mode_valid(struct drm_connector *connector,
					      struct drm_display_mode *mode)
{
	if (mode->hdisplay < 1920)
		return MODE_BAD;
	if (mode->vdisplay < 1080)
		return MODE_BAD;

	return MODE_OK;
}

static int lt6711_get_modes(struct gsgpu_bridge_phy *phy,
			    struct drm_connector *connector)
{
	struct gsgpu_dc_i2c *i2c = phy->li2c;
	struct edid *edid;
	unsigned int count = 0;

	edid = drm_get_edid(connector, &i2c->adapter);
	if (edid) {
		drm_connector_update_edid_property(connector, edid);
		count = drm_add_edid_modes(connector, edid);
		kfree(edid);
	} else {
		DRM_ERROR("LT6711 edid is invalid.\n");
	}

	return count;
}

static enum hpd_status lt6711_get_hpd_status(struct gsgpu_bridge_phy *phy)
{
	return hpd_status_plug_on;
}

static const struct bridge_phy_cfg_funcs lt6711_cfg_funcs = {
	.mode_valid = lt6711_mode_valid,
};

static struct bridge_phy_ddc_funcs lt6711_ddc_funcs = {
	.get_modes = lt6711_get_modes,
};

static struct bridge_phy_hpd_funcs lt6711_hpd_funcs = {
	.get_hpd_status = lt6711_get_hpd_status,
};

static struct bridge_phy_helper lt6711_helper_funcs = {
	.ddc_funcs = &lt6711_ddc_funcs,
	.hpd_funcs = &lt6711_hpd_funcs,
};

int bridge_phy_lt6711_init(struct gsgpu_dc_bridge *dc_bridge)
{
	struct gsgpu_bridge_phy *lt6711_phy;
	int ret = -1;
	u32 feature;

	feature = SUPPORT_DDC | SUPPORT_HPD;
	lt6711_phy = bridge_phy_alloc(dc_bridge);
	ret = bridge_phy_register(lt6711_phy, &lt6711_cfg_funcs, feature,
				  &lt6711_helper_funcs);
	return ret;
}

int bridge_phy_lt6711_remove(struct gsgpu_dc_bridge *phy)
{
	return 0;
}
