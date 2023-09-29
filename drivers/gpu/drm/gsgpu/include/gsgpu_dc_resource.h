#ifndef __DC_RESOURCE_H__
#define __DC_RESOURCE_H__
#define ENCODER_DATA_MAX  (1024*8)

enum resource_type {
	GSGPU_RESOURCE_DEFAULT,
	GSGPU_RESOURCE_HEADER,
	GSGPU_RESOURCE_EXT_ENCODER,
	GSGPU_RESOURCE_GPU,
	GSGPU_RESOURCE_GPIO,
	GSGPU_RESOURCE_I2C,
	GSGPU_RESOURCE_PWM,
	GSGPU_RESOURCE_CRTC,
	GSGPU_RESOURCE_ENCODER,
	GSGPU_RESOURCE_CONNECTOR,
	GSGPU_RESOURCE_MAX,
};

struct resource_object {
	u32 link;
	enum resource_type type;
	struct list_head node;
};

struct header_resource {
	struct resource_object base;
	u32 links;
	u32 max_planes;
	u8 ver_majro;
	u8 ver_minor;
	u8 name[16];
	u8 oem_vendor[32];
	u8 oem_product[32];
};

struct gpio_resource {
	struct resource_object base;
	u32 type; //bit Reuse
	u32 level_reg_offset; // offset of DC
	u32 level_reg_mask; // mask of reg
	u32 dir_reg_offset; // offset of DC
	u32 dir_reg_mask; //  mask of reg
};

struct i2c_resource {
	struct resource_object base;
	u32 feature;
	u16 id;
	u8  speed; // KHZ
};

struct pwm_resource {
	struct resource_object base;
	u32 feature;
	u8 pwm;
	u8 polarity;
	u32 period;
};

struct crtc_resource {
	struct resource_object base;
	u32 feature;
	u32 crtc_id;
	u32 encoder_id;
	u32 max_freq;
	u32 max_width;
	u32 max_height;
	bool is_vb_timing;
};

struct encoder_resource {
	struct resource_object base;
	u32 feature;
	u32 i2c_id;
	u32 connector_id;
	u32 type;
	u32 config_type;
	u32 chip;
	u8 chip_addr;
};

struct connector_resource {
	struct resource_object base;
	u32 feature;
	u32 i2c_id;
	u8 internal_edid[256];
	u32 type;
	u32 hotplug;
	u32 edid_method;
	u32 irq_gpio;
	u32 gpio_placement;
};

struct gpu_resource {
	struct resource_object base;
	u32 vram_type;
	u32 bit_width;
	u32 cap;
	u32 count_freq;
	u32 freq;
	u32 shaders_num;
	u32 shaders_freq;
};

struct ext_encoder_resources {
	struct resource_object base;
	u32 data_checksum;
	u32 data_size;
	u8 data[ENCODER_DATA_MAX-8];
};

enum gpio_placement {
	GPIO_PLACEMENT_LS3A = 0,
	GPIO_PLACEMENT_LS7A,
};

enum vbios_edid_method {
	EDID_VIA_NULL = 0,
	EDID_VIA_I2C,
	EDID_VIA_VBIOS,
	EDID_VIA_ENCODER,
	EDID_VIA_MAX = EDID_VIA_I2C,
};

enum vbios_encoder_type {
	ENCODER_NONE,
	ENCODER_DAC,
	ENCODER_TMDS,
	ENCODER_LVDS,
	ENCODER_TVDAC,
	ENCODER_VIRTUAL,
	ENCODER_DSI,
	ENCODER_DPMST,
	ENCODER_DPI
};

enum vbios_encoder_config {
	ENCODER_TRANSPARENT = 0,
	ENCODER_OS_CONFIG,
	ENCODER_BIOS_CONFIG, //BIOS CONFIG ENCODER
	ENCODER_TYPE_MAX = ENCODER_TRANSPARENT,
};

enum vbios_encoder_object {
	UNKNOWN = 0X00,
	INTERNAL_DVO = 0X01,
	INTERNAL_HDMI = 0X02,
	VGA_CH7055 = 0X10,
	VGA_ADV7125 = 0X11,
	DVI_TFP410 = 0X20,
	HDMI_IT66121 = 0X30,
	HDMI_SIL9022 = 0X31,
	HDMI_LT8618 = 0X32,
	EDP_NCS8805 = 0X40,
	EDP_LT9721 = 0X42,
	EDP_LT6711 = 0X43,
	LVDS_LT8619 = 0x50
};

enum vbios_hotplug {
	FORCE_ON = 0,
	POLLING,
	IRQ,
	VBIOS_HOTPLUG_MAX = FORCE_ON
};

enum vram_type {
	DDR3,
	DDR4,
	DDR5
};

enum vbios_connector_type {
	CONNECTOR_UNKNOWN = 0,
	CONNECTOR_VGA,
	CONNECTOR_DVI_I,
	CONNECTOR_DVI_D,
	CONNECTOR_DVI_A,
	CONNECTOR_COMPOSITE,
	CONNECTOR_SVIDEO,
	CONNECTOR_LVDS,
	CONNECTOR_COMPONENT,
	CONNECTOR_9PINDIN,
	CONNECTOR_DISPLAYPORT,
	CONNECTOR_HDMI_A,
	CONNECTOR_HDMI_B,
	CONNECTOR_TV,
	CONNECTOR_EDP,
	CONNECTOR_VIRTUAL,
	CONNECTOR_DSI,
	CONNECTOR_DPI
};

#endif
