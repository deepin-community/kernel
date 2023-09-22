#ifndef __DC_VBIOS_H__
#define __DC_VBIOS_H__

#include <linux/acpi.h>
#include "gsgpu_dc_resource.h"

struct gsgpu_vbios;
struct vbios_desc;
struct gsgpu_dc;

#define VBIOS_VERSION_V1_1 (11)
#define VBIOS_DATA_INVAL 0xFF

enum desc_ver {
	ver_v1,
};

struct vbios_info {
	char title[16];
	u32 version_major;
	u32 version_minor;
	char information[20];
	u32 link_num;
	u32 crtc_offset;
	u32 connector_num;
	u32 connector_offset;
	u32 encoder_num;
	u32 encoder_offset;
} __packed;

struct vbios_desc {
	u16 type;
	u8 ver;
	u8 link;
	u32 offset;
	u32 size;
	u64 ext[2];
} __packed;

struct vbios_header {
	u32 feature;
	u8 oem_vendor[32];
	u8 oem_product[32];
	u32 legacy_offset;
	u32 legacy_size;
	u32 desc_offset;
	u32 desc_size;
	u32 data_offset;
	u32 data_size;
} __packed;

enum gsgpu_edid_method {
	via_null = 0,
	via_i2c,
	via_vbios,
	via_encoder,
	via_max = 0xffff,
} __packed;

struct vbios_gpio {
	u32 feature;
	u32 type;
	u32 level_reg_offset; /* offset of DC */
	u32 level_reg_mask; /* mask of reg */
	u32 dir_reg_offset; /* offset of DC */
	u32 dir_reg_mask; /* mask of reg */
} __packed;

struct vbios_i2c {
	u32 feature;
	u16 id;
	u8  speed; /* KHZ */
} __packed;

struct vbios_pwm {
	u32 feature;
	u8 pwm;
	u8 polarity;
	u32 peroid;
} __packed;

struct vbios_encoder {
	u32 feature;
	u32 i2c_id;
	u32 connector_id;
	enum vbios_encoder_type type;
	enum vbios_encoder_config config_type;
	enum vbios_encoder_object chip;
	u8 chip_addr;
} __packed;

struct vbios_connector {
	u32 feature;
	u32 i2c_id;
	u8 internal_edid[256];
	enum vbios_connector_type type;
	enum vbios_hotplug hotplug;
	enum vbios_edid_method edid_method;
	u32 irq_gpio;
	enum gpio_placement gpio_placement;
} __packed;

struct vbios_crtc {
	u32 feature;
	u32 crtc_id;
	u32 encoder_id;
	u32 max_freq;
	u32 max_width;
	u32 max_height;
	bool is_vb_timing;
} __packed;

struct vbios_gpu {
	enum vram_type type;
	u32 bit_width;
	u32 cap;
	u32 count_freq;
	u32 freq;
	u32 shaders_num;
	u32 shaders_freq;
} __packed;

struct vbios_ext_encoder {
	u32 data_checksum;
	u32 data_size;
	u8 data[ENCODER_DATA_MAX-8];
} __packed;

struct gsgpu_vbios;

struct vbios_funcs {
	bool (*resource_pool_create)(struct gsgpu_vbios *vbios);
	bool (*resource_pool_destory)(struct gsgpu_vbios *vbios);

	bool (*create_header_resource)(struct gsgpu_vbios *vbios, void *data, u32 size);
	bool (*create_i2c_resource)(struct gsgpu_vbios *vbios, void *data, u32 link, u32 size);
	bool (*create_gpio_resource)(struct gsgpu_vbios *vbios, void *data, u32 link, u32 size);
	bool (*create_gpu_resource)(struct gsgpu_vbios *vbios, void *data, u32 link, u32 size);
	bool (*create_pwm_resource)(struct gsgpu_vbios *vbios, void *data, u32 link, u32 size);
	bool (*create_crtc_resource)(struct gsgpu_vbios *vbios, void *data, u32 link, u32 size);
	bool (*create_encoder_resource)(struct gsgpu_vbios *vbios, void *data, u32 link, u32 size);
	bool (*create_connecor_resource)(struct gsgpu_vbios *vbios, void *data, u32 link, u32 size);
	bool (*create_ext_encoder_resource)(struct gsgpu_vbios *vbios, void *data, u32 link, u32 size);

	struct header_resource *(*get_header_resource)(struct gsgpu_vbios *vbios);
	struct i2c_resource *(*get_i2c_resource)(struct gsgpu_vbios *vbios, u32 link);
	struct pwm_resource *(*get_pwm_resource)(struct gsgpu_vbios *vbios, u32 link);
	struct gpu_resource *(*get_gpu_resource)(struct gsgpu_vbios *vbios, u32 link);
	struct gpio_resource *(*get_gpio_resource)(struct gsgpu_vbios *vbios, u32 link);
	struct crtc_resource *(*get_crtc_resource)(struct gsgpu_vbios *vbios, u32 link);
	struct connector_resource *(*get_connector_resource)(struct gsgpu_vbios *vbios, u32 link);
	struct encoder_resource *(*get_encoder_resource)(struct gsgpu_vbios *vbios, u32 link);
	struct ext_encoder_resources *(*get_ext_encoder_resource)(struct gsgpu_vbios *vbios, u32 link);
};

struct gsgpu_vbios {
	struct gsgpu_dc *dc;
	void *vbios_ptr;
	struct list_head resource_list;
	struct vbios_funcs *funcs;
};

enum desc_type {
	desc_header = 0,
	desc_crtc,
	desc_encoder,
	desc_connector,
	desc_i2c,
	desc_pwm,
	desc_gpio,
	desc_backlight,
	desc_fan,
	desc_irq_vblank,
	desc_cfg_encoder,
	desc_res_encoder,
	desc_gpu,
	desc_max = 0xffff
};

#ifdef CONFIG_ACPI
struct acpi_viat_table {
    struct acpi_table_header header;
    u64 vbios_addr;
} __packed;
#endif

typedef bool(parse_func)(struct vbios_desc *, struct gsgpu_vbios *);

struct desc_func {
	enum desc_type type;
	u16 ver;
	parse_func *func;
};

void *dc_get_vbios_resource(struct gsgpu_vbios *vbios, u32 link,
			    enum resource_type type);
bool dc_vbios_init(struct gsgpu_dc *dc);
void dc_vbios_exit(struct gsgpu_vbios *vbios);
u8 gsgpu_vbios_checksum(const u8 *data, int size);
u32 gsgpu_vbios_version(struct gsgpu_vbios *vbios);
bool check_vbios_info(void);

#endif
