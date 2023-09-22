#ifndef __BRIDGE_PHY_H__
#define __BRIDGE_PHY_H__

#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <linux/regmap.h>
#include <linux/hdmi.h>
#include <drm/drm_edid.h>
#include <drm/drm_bridge.h>
#include <drm/drm_connector.h>
#include <video/videomode.h>
#include "gsgpu.h"

/* MISC: 0x00~-0x0f */
#define ENCODER_OBJECT_ID_NONE 0x00
/* VGA: 0x10~-0x1f */
#define ENCODER_OBJECT_ID_VGA_TRANSPARENT 0x1F
/* DVI: 0x20~-0x2f */
#define ENCODER_OBJECT_ID_DVI_TRANSPARENT 0x2f
/* HDMI: 0x30~-0x3f */
#define ENCODER_OBJECT_ID_HDMI_IT66121 0x30
#define ENCODER_OBJECT_ID_HDMI_LT8618 0x32
#define ENCODER_OBJECT_ID_HDMI_MS7210 0x33
#define ENCODER_OBJECT_ID_HDMI_TRANSPARENT 0x3F
/* EDP: 0x40~-0x4f */
#define ENCODER_OBJECT_ID_EDP_NCS8805 0x40
#define ENCODER_OBJECT_ID_EDP_NCS8803 0x41
#define ENCODER_OBJECT_ID_EDP_LT9721 0x42
#define ENCODER_OBJECT_ID_EDP_LT6711 0x43
#define ENCODER_OBJECT_ID_EDP_TRANSPARENT 0x4F
/* HDMI to LVDS */
#define ENCODER_OBJECT_ID_LVDS_LT8619 0x50
#define ENCODER_OBJECT_ID_MAX 0xFF

#define NAME_SIZE_MAX 50U
#define LS7A_GPIO_OFFSET 16U

#define to_bridge_phy(drm_bridge)                                            \
	container_of(drm_bridge, struct gsgpu_bridge_phy, bridge)

struct gsgpu_bridge_phy;

struct gsgpu_dc_bridge {
	struct gsgpu_device *adev;
	struct gsgpu_dc *dc;
	struct list_head node;
	int display_pipe_index;
	int encoder_obj;
	char chip_name[NAME_SIZE_MAX];
	char vendor_str[NAME_SIZE_MAX];
	unsigned int i2c_bus_num;
	unsigned short i2c_dev_addr;
	unsigned short hotplug;
	unsigned short edid_method;
	unsigned int irq_gpio;
	unsigned int gpio_placement;
};

enum encoder_type {
	encoder_none,
	encoder_dac,
	encoder_tmds,
	encoder_lvds,
	encoder_tvdac,
	encoder_virtual,
	encoder_dsi,
	encoder_dpmst,
	encoder_dpi,
};

enum hpd_status {
	hpd_status_plug_off = 0,
	hpd_status_plug_on = 1,
};

enum int_type {
	interrupt_all = 0,
	interrupt_hpd = 1,
	interrupt_max = 0xff,
};

struct reg_mask_seq {
	unsigned int reg;
	unsigned int mask;
	unsigned int val;
};

#define encoder_type_to_str(index)\
	(index == encoder_none ? "none" :\
	(index == encoder_dac ? "dac" :\
	(index == encoder_tmds ? "tmds" :\
	(index == encoder_lvds ? "lvds" :\
	(index == encoder_virtual ? "virtual" :\
	(index == encoder_dsi ? "dsi" :\
	(index == encoder_dpmst ? "dpmst" :\
	(index == encoder_dpi ? "dpi" :\
	"other"))))))))

#define hotplug_to_str(index)\
	(index == FORCE_ON ? "connected" :\
	(index == POLLING ? "polling" :\
	(index == IRQ ? "irq" :\
	"Unknown")))

enum encoder_config {
	encoder_transparent = 0,
	encoder_os_config, /* vbios_config */
	encoder_bios_config,
	encoder_timing_filling, /* legacy */
	encoder_kernel_driver,  /* Driver */
	encoder_type_max = 0xffffffff,
} __packed;

#define encoder_config_to_str(index)\
	(index == encoder_transparent ? "transparent" :\
	(index == encoder_os_config ? "os" :\
	(index == encoder_bios_config ? "bios" :\
	(index == encoder_timing_filling ? "timing" :\
	(index == encoder_kernel_driver ? "kernel" :\
	"Unknown")))))

#define edid_method_to_str(index)\
	(index == via_null ? "null" :\
	(index == via_i2c ? "i2c" :\
	(index == via_vbios ? "vbios" :\
	(index == via_encoder ? "encoder" :\
	"Unknown"))))

enum bridge_phy_feature {
	SUPPORT_HPD = BIT(0),
	SUPPORT_DDC = BIT(1),
	SUPPORT_HDMI_AUX = BIT(2),
};

static const char *const feature_str[] = {
	[SUPPORT_HPD] = "HPD",
	[SUPPORT_DDC] = "DDC",
};

enum input_signal_sample_type {
	SDR_CLK = 0,
	DDR_CLK,
};

struct ddc_status {
	struct mutex ddc_bus_mutex;
	bool ddc_bus_idle;
	bool ddc_bus_error;
	bool ddc_fifo_empty;
};

struct bridge_phy_mode_config {
	bool edid_read;
	u8 edid_buf[256];
	union hdmi_infoframe hdmi_frame;
	struct {
		enum input_signal_sample_type input_signal_type;
		bool gen_sync;
		struct drm_display_mode *mode;
		struct videomode vmode;
	} input_mode;
};

struct bridge_phy_cfg_funcs {
	int (*reg_init)(struct gsgpu_bridge_phy *phy);
	int (*hw_reset)(struct gsgpu_bridge_phy *phy);
	int (*sw_enable)(struct gsgpu_bridge_phy *phy);
	int (*sw_reset)(struct gsgpu_bridge_phy *phy);
	int (*suspend)(struct gsgpu_bridge_phy *phy);
	int (*resume)(struct gsgpu_bridge_phy *phy);
	void (*prepare)(struct gsgpu_bridge_phy *phy);
	void (*commit)(struct gsgpu_bridge_phy *phy);
	int (*backlight_ctrl)(struct gsgpu_bridge_phy *phy, int mode);
	int (*video_input_cfg)(struct gsgpu_bridge_phy *phy);
	int (*video_input_check)(struct gsgpu_bridge_phy *phy);
	int (*video_output_cfg)(struct gsgpu_bridge_phy *phy);
	int (*video_output_timing)(struct gsgpu_bridge_phy *phy,
				   const struct drm_display_mode *mode);
	int (*hdmi_output_mode)(struct gsgpu_bridge_phy *phy);
	int (*hdmi_audio)(struct gsgpu_bridge_phy *phy);
	int (*hdmi_csc)(struct gsgpu_bridge_phy *phy);
	int (*hdmi_hdcp_init)(struct gsgpu_bridge_phy *phy);
	int (*afe_high)(struct gsgpu_bridge_phy *phy);
	int (*afe_low)(struct gsgpu_bridge_phy *phy);
	int (*afe_set_tx)(struct gsgpu_bridge_phy *phy, bool enable);
	int (*mode_set_pre)(struct drm_bridge *bridge,
			    const struct drm_display_mode *mode,
			    const struct drm_display_mode *adj_mode);
	int (*mode_set)(struct gsgpu_bridge_phy *phy,
			const struct drm_display_mode *mode,
			const struct drm_display_mode *adj_mode);
	int (*mode_set_post)(struct drm_bridge *bridge,
			     const struct drm_display_mode *mode,
			     const struct drm_display_mode *adj_mode);
	enum drm_mode_status (*mode_valid)(struct drm_connector *connector,
					   struct drm_display_mode *mode);
};

struct bridge_phy_misc_funcs {
	bool (*chip_id_verify)(struct gsgpu_bridge_phy *phy, char *id);
	int (*debugfs_init)(struct gsgpu_bridge_phy *phy);
	void (*dpms_ctrl)(struct gsgpu_bridge_phy *phy, int mode);
};

struct bridge_phy_hpd_funcs {
	enum hpd_status (*get_hpd_status)(struct gsgpu_bridge_phy *phy);
	int (*int_enable)(struct gsgpu_bridge_phy *phy,
			  enum int_type interrut);
	int (*int_disable)(struct gsgpu_bridge_phy *phy,
			   enum int_type interrut);
	int (*int_clear)(struct gsgpu_bridge_phy *phy, enum int_type interrut);
	irqreturn_t (*irq_handler)(int irq, void *dev);
	irqreturn_t (*isr_thread)(int irq, void *dev);
};

struct bridge_phy_ddc_funcs {
	int (*ddc_fifo_fetch)(struct gsgpu_bridge_phy *phy, u8 *buf, u8 block,
			      size_t len, size_t offset);
	int (*ddc_fifo_abort)(struct gsgpu_bridge_phy *phy);
	int (*ddc_fifo_clear)(struct gsgpu_bridge_phy *phy);
	int (*get_edid_block)(void *data, u8 *buf, unsigned int block,
			      size_t len);
	int (*get_modes)(struct gsgpu_bridge_phy *phy,
			 struct drm_connector *connector);
};

struct bridge_phy_hdmi_aux_funcs {
	int (*set_gcp_avmute)(struct gsgpu_bridge_phy *phy, bool enable,
			      bool blue_screen);
	int (*set_avi_infoframe)(struct gsgpu_bridge_phy *phy,
				 const struct drm_display_mode *mode);
	int (*set_hdcp)(struct gsgpu_bridge_phy *phy);
};

struct bridge_phy_helper {
	const struct regmap_config *regmap_cfg;
	struct bridge_phy_misc_funcs *misc_funcs;
	struct bridge_phy_hpd_funcs *hpd_funcs;
	struct bridge_phy_ddc_funcs *ddc_funcs;
	struct bridge_phy_hdmi_aux_funcs *hdmi_aux_funcs;
};

struct gsgpu_bridge_phy {
	int display_pipe_index;
	struct drm_bridge bridge;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	enum drm_connector_status status;

	struct gsgpu_dc_bridge *res;
	struct gsgpu_device *adev;
	struct gsgpu_dc_i2c *li2c;
	struct i2c_client *i2c_phy;
	struct regmap *phy_regmap;

	void *priv;
	u8 chip_version;
	u32 feature;
	u32 connector_type;

	u8 sys_status;
	int irq_num;
	atomic_t irq_status;
	struct ddc_status ddc_status;
	struct bridge_phy_mode_config mode_config;
	struct bridge_phy_helper *helper;
	const struct bridge_phy_cfg_funcs *cfg_funcs;
	const struct bridge_phy_hpd_funcs *hpd_funcs;
	const struct bridge_phy_ddc_funcs *ddc_funcs;
	const struct bridge_phy_hdmi_aux_funcs *hdmi_aux_funcs;
};

struct gsgpu_dc_bridge
*dc_bridge_construct(struct gsgpu_dc *dc,
		     struct encoder_resource *encoder_res,
		     struct connector_resource *connector_res);
int gsgpu_dc_bridge_init(struct gsgpu_device *adev, int link_index);
struct gsgpu_bridge_phy *bridge_phy_alloc(struct gsgpu_dc_bridge *dc_bridge);
int bridge_phy_register(struct gsgpu_bridge_phy *phy,
			const struct bridge_phy_cfg_funcs *cfg_funcs,
			u32 feature, struct bridge_phy_helper *helper);

int bridge_phy_lt6711_init(struct gsgpu_dc_bridge *dc_bridge);
int bridge_phy_lt9721_init(struct gsgpu_dc_bridge *res);
int bridge_phy_lt8619_init(struct gsgpu_dc_bridge *dc_bridge);
int bridge_phy_ncs8805_init(struct gsgpu_dc_bridge *dc_bridge);
void bridge_phy_mode_set(struct gsgpu_bridge_phy *phy,
				struct drm_display_mode *mode,
				struct drm_display_mode *adj_mode);
#endif /* __BRIDGE_PHY_H__ */
