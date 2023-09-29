#include <drm/drm_atomic_helper.h>
#include "gsgpu.h"
#include "gsgpu_dc.h"
#include "gsgpu_dc_connector.h"
#include "gsgpu_dc_encoder.h"
#include "gsgpu_dc_resource.h"
#include "gsgpu_dc_vbios.h"
#include "bridge_phy.h"
#include "gsgpu_backlight.h"

/**
 * @section Bridge-phy connector functions
 */
static int bridge_phy_connector_get_modes(struct drm_connector *connector)
{
	struct edid *edid = NULL;
	unsigned int count = 0;
	struct gsgpu_device *adev = connector->dev->dev_private;
	struct gsgpu_bridge_phy *phy =
		adev->mode_info.encoders[connector->index]->bridge;
	unsigned short used_method = phy->res->edid_method;

	switch (used_method) {
	case via_i2c:
		edid = drm_get_edid(connector, &phy->li2c->adapter);
		if (edid) {
			drm_connector_update_edid_property(connector, edid);
			count = drm_add_edid_modes(connector, edid);
			kfree(edid);
		}
		break;
	case via_encoder:
		if (phy->ddc_funcs && phy->ddc_funcs->get_modes)
			count = phy->ddc_funcs->get_modes(phy, connector);
		break;
	default:
		break;
	}

	if (!count) {
		count = drm_add_modes_noedid(connector, 1920, 1080);
		drm_set_preferred_mode(connector, 1024, 768);
		DRM_DEBUG_DRIVER("[Bridge_phy] Setting %s edid.\n",
				 phy->res->chip_name);
	}

	return count;
}

static enum drm_mode_status
bridge_phy_connector_mode_valid(struct drm_connector *connector,
				struct drm_display_mode *mode)
{
	struct gsgpu_device *adev = connector->dev->dev_private;
	struct gsgpu_bridge_phy *phy =
		adev->mode_info.encoders[connector->index]->bridge;

	if (phy->cfg_funcs && phy->cfg_funcs->mode_valid)
		return phy->cfg_funcs->mode_valid(connector, mode);

	return MODE_OK;
}

static struct drm_connector_helper_funcs bridge_phy_connector_helper_funcs = {
	.get_modes = bridge_phy_connector_get_modes,
	.mode_valid = bridge_phy_connector_mode_valid,
	.best_encoder = gsgpu_dc_get_best_single_encoder,
};

static enum drm_connector_status
bridge_phy_connector_detect(struct drm_connector *connector, bool force)
{
	return connector_status_connected;
}

static const struct drm_connector_funcs bridge_phy_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = bridge_phy_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.late_register = gsgpu_backlight_register
};

/**
 * @section Bridge-phy core functions
 */
static void bridge_phy_enable(struct drm_bridge *bridge)
{
	struct gsgpu_bridge_phy *phy = to_bridge_phy(bridge);

	DRM_DEBUG("[Bridge_phy] [%s] enable\n", phy->res->chip_name);
	if (phy->cfg_funcs && phy->cfg_funcs->afe_high)
		phy->cfg_funcs->afe_high(phy);
	if (phy->cfg_funcs && phy->cfg_funcs->afe_set_tx)
		phy->cfg_funcs->afe_set_tx(phy, TRUE);
	if (phy->cfg_funcs && phy->cfg_funcs->hdmi_audio)
		phy->cfg_funcs->hdmi_audio(phy);
}

static void bridge_phy_disable(struct drm_bridge *bridge)
{
	struct gsgpu_bridge_phy *phy = to_bridge_phy(bridge);

	DRM_DEBUG("[Bridge_phy] [%s] disable\n", phy->res->chip_name);
	if (phy->cfg_funcs && phy->cfg_funcs->afe_low)
		phy->cfg_funcs->afe_low(phy);
	if (phy->cfg_funcs && phy->cfg_funcs->afe_set_tx)
		phy->cfg_funcs->afe_set_tx(phy, FALSE);
}

static int __bridge_phy_mode_set(struct gsgpu_bridge_phy *phy,
				 const struct drm_display_mode *mode,
				 const struct drm_display_mode *adj_mode)
{
	if (phy->mode_config.input_mode.gen_sync)
		DRM_DEBUG("[Bridge_phy] [%s] bridge_phy gen_sync\n",
				phy->res->chip_name);
	if (phy->cfg_funcs && phy->cfg_funcs->mode_set_pre)
		phy->cfg_funcs->mode_set_pre(&phy->bridge, mode, adj_mode);
	if (phy->cfg_funcs && phy->cfg_funcs->mode_set)
		phy->cfg_funcs->mode_set(phy, mode, adj_mode);
	if (phy->cfg_funcs && phy->cfg_funcs->mode_set_post)
		phy->cfg_funcs->mode_set_post(&phy->bridge, mode, adj_mode);

	return 0;
}

void bridge_phy_mode_set(struct gsgpu_bridge_phy *phy,
				struct drm_display_mode *mode,
				struct drm_display_mode *adj_mode)
{
	if (!phy)
		return;

	DRM_DEBUG("[Bridge_phy] [%s] mode set\n", phy->res->chip_name);
	drm_mode_debug_printmodeline(mode);

	__bridge_phy_mode_set(phy, mode, adj_mode);
}

static int bridge_phy_attach(struct drm_bridge *bridge,
			     enum drm_bridge_attach_flags flags)
{
	struct gsgpu_bridge_phy *phy = to_bridge_phy(bridge);
	struct gsgpu_connector *lconnector;
	int link_index = phy->display_pipe_index;
	int ret;

	DRM_DEBUG("[Bridge_phy] %s attach\n", phy->res->chip_name);
	if (!bridge->encoder) {
		DRM_ERROR("Parent encoder object not found\n");
		return -ENODEV;
	}

	lconnector = kzalloc(sizeof(*lconnector), GFP_KERNEL);
	if (!lconnector)
		return -ENOMEM;

	ret = drm_connector_init(bridge->dev, &lconnector->base,
				 &bridge_phy_connector_funcs,
				 phy->connector_type);
	if (ret) {
		DRM_ERROR("[Bridge_phy] %s Failed to initialize connector\n",
				phy->res->chip_name);
		return ret;
	}

	lconnector->connector_id = link_index;
	phy->connector = &lconnector->base;
	phy->adev->mode_info.connectors[link_index] = lconnector;
	phy->connector->dpms = DRM_MODE_DPMS_OFF;

	drm_connector_helper_add(&lconnector->base,
				 &bridge_phy_connector_helper_funcs);

	if (link_index == 0) {
		lconnector->irq_source_i2c = DC_IRQ_SOURCE_I2C0;
		lconnector->irq_source_hpd = DC_IRQ_SOURCE_HPD_HDMI0_NULL;
	} else if (link_index == 1) {
		lconnector->irq_source_i2c = DC_IRQ_SOURCE_I2C1;
		lconnector->irq_source_hpd = DC_IRQ_SOURCE_HPD_HDMI1_NULL;
	}

	switch (phy->res->hotplug) {
	case IRQ:
		phy->connector->polled = DRM_CONNECTOR_POLL_HPD;
		if (link_index == 0)
			lconnector->irq_source_hpd = DC_IRQ_SOURCE_HPD_HDMI0;
		else if (link_index == 1)
			lconnector->irq_source_hpd = DC_IRQ_SOURCE_HPD_HDMI1;
		break;
	case FORCE_ON:
		phy->connector->polled = 0;
		break;
	case POLLING:
	default:
		phy->connector->polled = DRM_CONNECTOR_POLL_CONNECT |
					 DRM_CONNECTOR_POLL_DISCONNECT;
		break;
	}

	DRM_DEBUG_DRIVER("[Bridge_phy] %s Set connector poll=0x%x.\n",
			 phy->res->chip_name, phy->connector->polled);

	return ret;
}

static const struct drm_bridge_funcs bridge_funcs = {
	.enable = bridge_phy_enable,
	.disable = bridge_phy_disable,
	.attach = bridge_phy_attach,
};

static int bridge_phy_bind(struct gsgpu_bridge_phy *phy)
{
	int ret;

	phy->bridge.funcs = &bridge_funcs;
	drm_bridge_add(&phy->bridge);
	ret = drm_bridge_attach(phy->encoder, &phy->bridge, NULL, 0);
	if (ret) {
		DRM_ERROR("[Bridge_phy] %s Failed to attach phy ret %d\n",
			  phy->res->chip_name, ret);
		return ret;
	}

	DRM_INFO("[Bridge_phy] %s encoder-%d be attach to this bridge.\n",
		 phy->res->chip_name, phy->encoder->index);

	return 0;
}

/**
 * @section Bridge-phy helper functions
 */
void bridge_phy_reg_mask_seq(struct gsgpu_bridge_phy *phy,
			     const struct reg_mask_seq *seq, size_t seq_size)
{
	unsigned int i;
	struct regmap *regmap;

	regmap = phy->phy_regmap;
	for (i = 0; i < seq_size; i++)
		regmap_update_bits(regmap, seq[i].reg, seq[i].mask, seq[i].val);
}

void bridge_phy_reg_update_bits(struct gsgpu_bridge_phy *phy, unsigned int reg,
				unsigned int mask, unsigned int val)
{
	unsigned int reg_val;

	regmap_read(phy->phy_regmap, reg, &reg_val);
	val ? (reg_val |= (mask)) : (reg_val &= ~(mask));
	regmap_write(phy->phy_regmap, reg, reg_val);
}

int bridge_phy_reg_dump(struct gsgpu_bridge_phy *phy, size_t start,
			size_t count)
{
	u8 *buf;
	int ret;
	unsigned int i;

	buf = kzalloc(count, GFP_KERNEL);
	if (IS_ERR(buf)) {
		ret = PTR_ERR(buf);
		return -ENOMEM;
	}
	ret = regmap_raw_read(phy->phy_regmap, start, buf, count);
	for (i = 0; i < count; i++)
		pr_info("[%lx]=%02x", start + i, buf[i]);

	kfree(buf);
	return ret;
}

static char *get_encoder_chip_name(int encoder_obj)
{
	switch (encoder_obj) {
	case ENCODER_OBJECT_ID_NONE:
		return "none";
	case ENCODER_OBJECT_ID_EDP_NCS8803:
		return "ncs8803";
	case ENCODER_OBJECT_ID_EDP_NCS8805:
		return "ncs8805";
	case ENCODER_OBJECT_ID_EDP_LT9721:
		return "lt9721";
	case ENCODER_OBJECT_ID_EDP_LT6711:
		return "lt6711";
	case ENCODER_OBJECT_ID_LVDS_LT8619:
		return "lt8619";
	case ENCODER_OBJECT_ID_VGA_TRANSPARENT:
		return "vga";
	case ENCODER_OBJECT_ID_DVI_TRANSPARENT:
		return "dvi";
	case ENCODER_OBJECT_ID_HDMI_TRANSPARENT:
		return "hdmi";
	case ENCODER_OBJECT_ID_EDP_TRANSPARENT:
		return "edp";
	default:
		DRM_WARN("No ext encoder chip 0x%x.\n", encoder_obj);
		return "Unknown";
	}
}

static bool bridge_phy_check_feature(const struct gsgpu_bridge_phy *phy,
				     u32 feature)
{
	return phy->feature & feature;
}

static int bridge_phy_add_hpd_funcs(struct gsgpu_bridge_phy *phy, void *funcs)
{
	phy->hpd_funcs = (struct bridge_phy_hpd_funcs *)funcs;

	return 0;
}

static int bridge_phy_add_ddc_funcs(struct gsgpu_bridge_phy *phy, void *funcs)
{
	phy->ddc_funcs = (struct bridge_phy_ddc_funcs *)funcs;

	return 0;
}

static int bridge_phy_add_hdmi_aux_funcs(struct gsgpu_bridge_phy *phy,
					 void *funcs)
{
	phy->hdmi_aux_funcs = (struct bridge_phy_hdmi_aux_funcs *)funcs;

	return 0;
}

static void  bridge_phy_add_helper_funcs(struct gsgpu_bridge_phy *phy,
					 struct bridge_phy_helper *helper)
{
	u32 feature, check_feature;

	feature = phy->feature;
	phy->helper = helper;

	DRM_DEBUG_DRIVER("[Bridge_phy] %s features=%#x, add helper funcs\n",
			 phy->res->chip_name, feature);

	check_feature = SUPPORT_HPD;
	if (bridge_phy_check_feature(phy, check_feature))
		bridge_phy_add_hpd_funcs(phy, helper->hpd_funcs);

	check_feature = SUPPORT_DDC;
	if (bridge_phy_check_feature(phy, check_feature))
		bridge_phy_add_ddc_funcs(phy, helper->ddc_funcs);

	check_feature = SUPPORT_HDMI_AUX;
	if (bridge_phy_check_feature(phy, check_feature))
		bridge_phy_add_hdmi_aux_funcs(phy, helper->hdmi_aux_funcs);
}

static int bridge_phy_register_irq_num(struct gsgpu_bridge_phy *phy)
{
	int ret = -1;
	char irq_name[NAME_SIZE_MAX];
	struct gsgpu_dc_bridge *res;

	res = phy->res;
	phy->irq_num = -1;

	if (res->adev->chip == dev_7a2000 || res->adev->chip == dev_2k2000)
		return ret;

	if (phy->res->hotplug != IRQ)
		return ret;

	if (res->gpio_placement)
		res->irq_gpio += LS7A_GPIO_OFFSET;

	ret = gpio_is_valid(res->irq_gpio);
	if (!ret)
		goto error_gpio_valid;
	sprintf(irq_name, "%s-irq", res->chip_name);

	ret = gpio_request(res->irq_gpio, irq_name);
	if (ret)
		goto error_gpio_req;
	ret = gpio_direction_input(res->irq_gpio);
	if (ret)
		goto error_gpio_cfg;

	phy->irq_num = gpio_to_irq(res->irq_gpio);
	if (phy->irq_num < 0) {
		ret = phy->irq_num;
		DRM_ERROR("GPIO %d has no interrupt\n", res->irq_gpio);
		return ret;
	}

	DRM_DEBUG("[Bridge_phy] %s register irq num %d.\n", res->chip_name,
		  phy->irq_num);
	return 0;

error_gpio_cfg:
	DRM_ERROR("Failed to config gpio %d free it %d\n", res->irq_gpio, ret);
	gpio_free(res->irq_gpio);
error_gpio_req:
	DRM_ERROR("Failed to request gpio %d, %d\n", res->irq_gpio, ret);
error_gpio_valid:
	DRM_ERROR("Invalid gpio %d, %d\n", res->irq_gpio, ret);
	return ret;
}

static int bridge_phy_register_irq_handle(struct gsgpu_bridge_phy *phy)
{
	int ret = -1;
	irqreturn_t (*irq_handler)(int irq, void *dev);
	irqreturn_t (*isr_thread)(int irq, void *dev);

	if (phy->adev->chip == dev_7a2000 || phy->adev->chip == dev_2k2000)
		return ret;

	if (phy->res->hotplug != IRQ)
		return ret;

	if (phy->irq_num <= 0) {
		phy->connector->polled = DRM_CONNECTOR_POLL_CONNECT |
					 DRM_CONNECTOR_POLL_DISCONNECT;
		return ret;
	}

	if (phy->hpd_funcs && phy->hpd_funcs->isr_thread) {
		irq_handler = phy->hpd_funcs->irq_handler;
		isr_thread = phy->hpd_funcs->isr_thread;
	}

	ret = devm_request_threaded_irq(
		&phy->i2c_phy->dev, phy->irq_num, irq_handler, isr_thread,
		IRQF_ONESHOT | IRQF_SHARED | IRQF_TRIGGER_HIGH,
		phy->res->chip_name, phy);
	if (ret)
		goto error_irq;

	DRM_DEBUG_DRIVER("[Bridge_phy] %s register irq handler succeed %d.\n",
			 phy->res->chip_name, phy->irq_num);
	return 0;

error_irq:
	gpio_free(phy->res->irq_gpio);
	DRM_ERROR("Failed to request irq handler for irq %d.\n", phy->irq_num);
	return ret;
}

static int bridge_phy_hw_reset(struct gsgpu_bridge_phy *phy)
{
	if (phy->cfg_funcs && phy->cfg_funcs->hw_reset)
		phy->cfg_funcs->hw_reset(phy);

	return 0;
}

static int bridge_phy_misc_init(struct gsgpu_bridge_phy *phy)
{
	if (phy->helper && phy->helper->misc_funcs
			&& phy->helper->misc_funcs->debugfs_init)
		phy->helper->misc_funcs->debugfs_init(phy);

	return 0;
}

static int bridge_phy_regmap_init(struct gsgpu_bridge_phy *phy)
{
	int ret;
	struct regmap *regmap;

	if (!phy->helper)
		return 0;

	mutex_init(&phy->ddc_status.ddc_bus_mutex);
	atomic_set(&phy->irq_status, 0);

	if (!phy->helper->regmap_cfg)
		return 0;

	regmap = devm_regmap_init_i2c(phy->li2c->ddc_client,
				      phy->helper->regmap_cfg);
	if (IS_ERR(regmap)) {
		ret = PTR_ERR(regmap);
		return -ret;
	}
	phy->phy_regmap = regmap;

	return 0;
}

static int bridge_phy_chip_id_verify(struct gsgpu_bridge_phy *phy)
{
	int ret;
	char str[NAME_SIZE_MAX] = "";

	if (phy->helper && phy->helper->misc_funcs
			&& phy->helper->misc_funcs->chip_id_verify) {
		ret = phy->helper->misc_funcs->chip_id_verify(phy, str);
		if (!ret)
			DRM_ERROR("Failed to verify chip %s, return [%s]\n",
				  phy->res->chip_name, str);
		strncpy(phy->res->vendor_str, str, NAME_SIZE_MAX - 1);
		return ret;
	}

	return -ENODEV;
}

static int bridge_phy_video_config(struct gsgpu_bridge_phy *phy)
{
	if (phy->cfg_funcs && phy->cfg_funcs->video_input_cfg)
		phy->cfg_funcs->video_input_cfg(phy);
	if (phy->cfg_funcs && phy->cfg_funcs->video_output_cfg)
		phy->cfg_funcs->video_output_cfg(phy);

	return 0;
}

static int bridge_phy_hdmi_config(struct gsgpu_bridge_phy *phy)
{
	if (phy->cfg_funcs && phy->cfg_funcs->hdmi_audio)
		phy->cfg_funcs->hdmi_audio(phy);
	if (phy->cfg_funcs && phy->cfg_funcs->hdmi_csc)
		phy->cfg_funcs->hdmi_csc(phy);
	if (phy->cfg_funcs && phy->cfg_funcs->hdmi_hdcp_init)
		phy->cfg_funcs->hdmi_hdcp_init(phy);

	return 0;
}

static int bridge_phy_sw_init(struct gsgpu_bridge_phy *phy)
{
	bridge_phy_chip_id_verify(phy);
	if (phy->cfg_funcs && phy->cfg_funcs->sw_reset)
		phy->cfg_funcs->sw_reset(phy);
	if (phy->cfg_funcs && phy->cfg_funcs->reg_init)
		phy->cfg_funcs->reg_init(phy);
	if (phy->cfg_funcs && phy->cfg_funcs->sw_enable)
		phy->cfg_funcs->sw_enable(phy);
	bridge_phy_video_config(phy);
	bridge_phy_hdmi_config(phy);

	DRM_DEBUG_DRIVER("[Bridge_phy] %s sw init completed\n",
			 phy->res->chip_name);

	return 0;
}

static int bridge_phy_encoder_obj_select(struct gsgpu_dc_bridge *dc_bridge)
{
	int ret = 0;

	switch (dc_bridge->encoder_obj) {
	case ENCODER_OBJECT_ID_EDP_LT6711:
		ret = bridge_phy_lt6711_init(dc_bridge);
		break;
	case ENCODER_OBJECT_ID_EDP_LT9721:
		ret = bridge_phy_lt9721_init(dc_bridge);
		break;
	case ENCODER_OBJECT_ID_LVDS_LT8619:
		ret = bridge_phy_lt8619_init(dc_bridge);
		break;
	case ENCODER_OBJECT_ID_EDP_NCS8805:
		ret = bridge_phy_ncs8805_init(dc_bridge);
		break;
	case ENCODER_OBJECT_ID_NONE:
	case ENCODER_OBJECT_ID_VGA_TRANSPARENT:
	case ENCODER_OBJECT_ID_HDMI_TRANSPARENT:
	case ENCODER_OBJECT_ID_EDP_TRANSPARENT:
	default:
		ret = 0;
		DRM_DEBUG_DRIVER("No matching chip! Skip bridge phy init\n");
		break;
	}

	return ret;
}

static int bridge_phy_init(struct gsgpu_bridge_phy *phy)
{
	bridge_phy_hw_reset(phy);
	bridge_phy_regmap_init(phy);
	bridge_phy_register_irq_num(phy);

	bridge_phy_misc_init(phy);
	bridge_phy_bind(phy);
	bridge_phy_sw_init(phy);
	bridge_phy_register_irq_handle(phy);
	DRM_INFO("[Bridge_phy] %s init finish.\n", phy->res->chip_name);

	return 0;
}

/**
 * @section Bridge-phy interface
 */
int bridge_phy_register(struct gsgpu_bridge_phy *phy,
			const struct bridge_phy_cfg_funcs *cfg_funcs,
			u32 feature, struct bridge_phy_helper *helper)
{
	phy->feature = feature;
	if (cfg_funcs)
		phy->cfg_funcs = cfg_funcs;

	if (helper)
		bridge_phy_add_helper_funcs(phy, helper);

	bridge_phy_init(phy);
	DRM_DEBUG_DRIVER("bridge phy register success!\n");

	return 0;
}

struct gsgpu_bridge_phy *bridge_phy_alloc(struct gsgpu_dc_bridge *dc_bridge)
{
	struct gsgpu_bridge_phy *bridge_phy;
	int index = dc_bridge->display_pipe_index;
	struct gsgpu_dc_connector *connector =
			dc_bridge->adev->dc->link_info[index].connector;

	bridge_phy = kzalloc(sizeof(*bridge_phy), GFP_KERNEL);
	if (IS_ERR(bridge_phy)) {
		DRM_ERROR("Failed to alloc gsgpu bridge phy!\n");
		return NULL;
	}

	bridge_phy->display_pipe_index = dc_bridge->display_pipe_index;
	bridge_phy->bridge.driver_private = bridge_phy;
	bridge_phy->adev = dc_bridge->adev;
	bridge_phy->res = dc_bridge;
	bridge_phy->encoder = &dc_bridge->adev->mode_info.encoders[index]->base;
	bridge_phy->li2c = dc_bridge->adev->i2c[index];
	bridge_phy->connector_type = connector->resource->type;
	dc_bridge->adev->mode_info.encoders[index]->bridge = bridge_phy;

	return bridge_phy;
}

struct gsgpu_dc_bridge
*dc_bridge_construct(struct gsgpu_dc *dc,
		     struct encoder_resource *encoder_res,
		     struct connector_resource *connector_res)
{
	struct gsgpu_dc_bridge *dc_bridge;
	const char *chip_name;
	u32 link;

	if (IS_ERR_OR_NULL(dc) ||
	    IS_ERR_OR_NULL(encoder_res) ||
	    IS_ERR_OR_NULL(connector_res))
		return NULL;

	dc_bridge = kzalloc(sizeof(*dc_bridge), GFP_KERNEL);
	if (IS_ERR_OR_NULL(dc_bridge))
		return NULL;

	link = encoder_res->base.link;
	if (link >= DC_DVO_MAXLINK)
		return false;

	dc_bridge->adev = dc->adev;
	dc_bridge->dc = dc;
	dc_bridge->display_pipe_index = link;

	dc_bridge->encoder_obj = encoder_res->chip;
	chip_name = get_encoder_chip_name(dc_bridge->encoder_obj);
	snprintf(dc_bridge->chip_name, NAME_SIZE_MAX, "%s", chip_name);
	dc_bridge->i2c_bus_num = encoder_res->i2c_id;
	dc_bridge->i2c_dev_addr = encoder_res->chip_addr;

	dc_bridge->hotplug = connector_res->hotplug;
	dc_bridge->edid_method = connector_res->edid_method;
	dc_bridge->gpio_placement = connector_res->gpio_placement;
	dc_bridge->irq_gpio = connector_res->irq_gpio;

	switch (dc_bridge->encoder_obj) {
	case ENCODER_OBJECT_ID_EDP_LT6711:
	case ENCODER_OBJECT_ID_EDP_LT9721:
	case ENCODER_OBJECT_ID_EDP_NCS8805:
		dc->link_info[link].encoder->has_ext_encoder = true;
		break;
	default:
		dc->link_info[link].encoder->has_ext_encoder = false;
		break;
	}

	DRM_INFO("Encoder Parse: #0x%02x-%s type:%s hotplug:%s.\n",
		 dc_bridge->encoder_obj, dc_bridge->chip_name,
		 encoder_type_to_str(encoder_res->type),
		 hotplug_to_str(dc_bridge->hotplug));
	DRM_INFO("Encoder Parse: config_type:%s, edid_method:%s.\n",
		 encoder_config_to_str(encoder_res->config_type),
		 edid_method_to_str(dc_bridge->edid_method));

	return dc_bridge;
}

int gsgpu_dc_bridge_init(struct gsgpu_device *adev, int link_index)
{
	struct gsgpu_dc_bridge *dc_bridge =
					adev->dc->link_info[link_index].bridge;
	int ret;

	if (link_index >= 2)
		return -1;

	ret = bridge_phy_encoder_obj_select(dc_bridge);
	if (ret)
		return ret;

	return ret;
}

void gsgpu_bridge_suspend(struct gsgpu_device *adev)
{
	struct gsgpu_bridge_phy *phy;
	int i;

	for (i = 0; i < 2; i++) {
		phy = adev->mode_info.encoders[i]->bridge;
		if (phy && phy->cfg_funcs && phy->cfg_funcs->suspend) {
			phy->cfg_funcs->suspend(phy);
			DRM_INFO("[Bridge_phy] %s suspend completed.\n",
					phy->res->chip_name);
		}
	}
}

void gsgpu_bridge_resume(struct gsgpu_device *adev)
{
	struct gsgpu_bridge_phy *phy = NULL;
	int i;

	for (i = 0; i < 2; i++) {
		phy = adev->mode_info.encoders[i]->bridge;
		if (phy) {
			if (phy->cfg_funcs &&  phy->cfg_funcs->resume)
				phy->cfg_funcs->resume(phy);
			else {
				bridge_phy_hw_reset(phy);
				bridge_phy_sw_init(phy);
			}
			DRM_INFO("[Bridge_phy] %s resume completed.\n",
					phy->res->chip_name);
		}
	}
}
