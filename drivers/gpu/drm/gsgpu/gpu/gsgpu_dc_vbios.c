#include "gsgpu.h"
#include "gsgpu_dc_vbios.h"
#include <asm/loongson.h>
#include <linux/acpi.h>

#define VBIOS_START 0x1000
#define VBIOS_SIZE 0x40000
#define VBIOS_OFFSET 0x100000
#define VBIOS_DESC_OFFSET 0x6000
#define LOONGSON_VBIOS_TITLE "Loongson-VBIOS"

#define VBIOS_DATA_INVAL 0xFF

enum desc_ver {
	ver_v1,
};

struct vbios_info {
	char title[VBIOS_HEADER_MAX_TITLE_SIZE];
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
	u32 size; /* Size of the resource data. Note size of the descriptor! */
	u64 ext[2];
} __packed;

struct vbios_header {
	u32 feature;
	u8 oem_vendor[VBIOS_OEM_VENDOR_STRING_SIZE];
	u8 oem_product[VBIOS_OEM_VENDOR_STRING_SIZE];
	u32 legacy_offset;
	u32 legacy_size;
	u32 desc_offset;
	u32 desc_size;
	u32 data_offset;
	u32 data_size;
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
	u32 period;
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
	u32 freq_count;
	u32 freq;
	u32 shaders_num;
	u32 shaders_freq;
} __packed;

struct vbios_ext_encoder {
	u32 data_checksum;
	u32 data_size;
	u8 data[EXT_ENCODER_DATA_MAX];
} __packed;

struct gsgpu_vbios {
	struct gsgpu_dc *dc;
	u8 *vbios_ptr;
	struct list_head resource_list;
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
	end_of_desc_table = 0xffff
};

#ifdef CONFIG_ACPI
struct acpi_viat_table {
    struct acpi_table_header header;
    u64 vbios_addr;
} __packed;
#endif

static bool vbios_create_header_resource(struct gsgpu_vbios *vbios,
					 void *data, u32 link, u32 size)
{
	struct vbios_header vb_header;
	struct header_resource *header;
	u32 header_size = sizeof(struct vbios_header);

	header = kvmalloc(sizeof(*header), GFP_KERNEL);
	if (IS_ERR_OR_NULL(header))
		return false;

	memset(&vb_header, VBIOS_DATA_INVAL, header_size);
	memcpy(&vb_header, data, min(size, header_size));

	memcpy(header->oem_product, vb_header.oem_product, sizeof(vb_header.oem_product));
	memcpy(header->oem_vendor, vb_header.oem_vendor, sizeof(vb_header.oem_vendor));
	header->oem_product[sizeof(vb_header.oem_product)-1] = '\0';
	header->oem_vendor[sizeof(vb_header.oem_vendor)-1] = '\0';
	header->base.type = GSGPU_RESOURCE_HEADER;

	list_add_tail(&header->base.node, &vbios->resource_list);

	return true;
}

static bool vbios_create_crtc_resource(struct gsgpu_vbios *vbios,
				       void *data, u32 link, u32 size)
{
	struct vbios_crtc vb_crtc;
	struct crtc_resource *crtc;
	u32 crtc_size = sizeof(struct vbios_crtc);

	if (IS_ERR_OR_NULL(vbios) || IS_ERR_OR_NULL(data))
		return false;

	crtc = kvmalloc(sizeof(*crtc), GFP_KERNEL);
	if (IS_ERR_OR_NULL(crtc))
		return false;

	memset(&vb_crtc, VBIOS_DATA_INVAL, crtc_size);
	memcpy(&vb_crtc, data, min(size, crtc_size));

	crtc->base.link = link;
	crtc->base.type = GSGPU_RESOURCE_CRTC;
	crtc->feature = vb_crtc.feature;
	crtc->crtc_id = vb_crtc.crtc_id;
	crtc->encoder_id = vb_crtc.encoder_id;
	crtc->max_freq = vb_crtc.max_freq;
	crtc->max_width = vb_crtc.max_width;
	crtc->max_height = vb_crtc.max_height;
	crtc->is_vb_timing = vb_crtc.is_vb_timing;

	list_add_tail(&crtc->base.node, &vbios->resource_list);

	return true;
}

static bool vbios_create_encoder_resource(struct gsgpu_vbios *vbios, void *data,
					  u32 link, u32 size)
{
	struct vbios_encoder vb_encoder;
	struct encoder_resource *encoder;
	u32 encoder_size;

	if (IS_ERR_OR_NULL(vbios) || IS_ERR_OR_NULL(data))
		return false;

	encoder = kvmalloc(sizeof(*encoder), GFP_KERNEL);
	if (IS_ERR_OR_NULL(encoder))
		return false;

	encoder_size = sizeof(struct vbios_encoder);
	memset(&vb_encoder, VBIOS_DATA_INVAL, encoder_size);
	memcpy(&vb_encoder, data, min(size, encoder_size));

	encoder->base.link = link;
	encoder->base.type = GSGPU_RESOURCE_ENCODER;
	encoder->feature = vb_encoder.feature;
	encoder->i2c_id = vb_encoder.i2c_id;
	encoder->connector_id = vb_encoder.connector_id;
	encoder->type = vb_encoder.type;
	encoder->config_type = vb_encoder.config_type;
	encoder->chip_addr = vb_encoder.chip_addr;
	encoder->chip = vb_encoder.chip;

	list_add_tail(&encoder->base.node, &vbios->resource_list);

	return true;
}

static bool vbios_create_connector_resource(struct gsgpu_vbios *vbios,
					    void *data, u32 link, u32 size)
{
	struct vbios_connector vb_connector;
	struct connector_resource *connector;
	u32 connector_size;

	if (IS_ERR_OR_NULL(vbios) || IS_ERR_OR_NULL(data))
		return false;

	connector = kvmalloc(sizeof(*connector), GFP_KERNEL);
	if (IS_ERR_OR_NULL(connector))
		return false;

	connector_size = sizeof(struct vbios_connector);
	memset(&vb_connector, VBIOS_DATA_INVAL, connector_size);
	memcpy(&vb_connector, data, min(size, connector_size));

	connector->base.link = link;
	connector->base.type = GSGPU_RESOURCE_CONNECTOR;

	connector->feature = vb_connector.feature;
	connector->i2c_id = vb_connector.i2c_id;

	connector->type = vb_connector.type;
	connector->hotplug = vb_connector.hotplug;
	connector->edid_method = vb_connector.edid_method;
	connector->irq_gpio = vb_connector.irq_gpio;
	connector->gpio_placement = vb_connector.gpio_placement;

	list_add_tail(&connector->base.node, &vbios->resource_list);

	return true;
}

static bool vbios_create_i2c_resource(struct gsgpu_vbios *vbios, void *data,
				      u32 link, u32 size)
{
	struct i2c_resource *i2c_resource;
	struct vbios_i2c vb_i2c;
	u32 i2c_size;

	if (IS_ERR_OR_NULL(vbios) || IS_ERR_OR_NULL(data))
		return false;

	i2c_resource = kvmalloc(sizeof(*i2c_resource), GFP_KERNEL);
	if (IS_ERR_OR_NULL(i2c_resource))
		return false;

	i2c_size = sizeof(struct vbios_i2c);
	memset(&vb_i2c, VBIOS_DATA_INVAL, i2c_size);
	memcpy(&vb_i2c, data, min(size, i2c_size));

	i2c_resource->speed = vb_i2c.speed == VBIOS_DATA_INVAL ? 10 : vb_i2c.speed;
	i2c_resource->base.link = link;
	i2c_resource->base.type = GSGPU_RESOURCE_I2C;

	i2c_resource->feature = vb_i2c.feature;
	i2c_resource->id = vb_i2c.id;

	list_add_tail(&i2c_resource->base.node, &vbios->resource_list);

	return true;
}

static bool vbios_create_gpio_resource(struct gsgpu_vbios *vbios,
				       void *data, u32 link, u32 size)
{
	return false;
}

static bool vbios_create_pwm_resource(struct gsgpu_vbios *vbios, void *data,
				      u32 link, u32 size)
{
	struct pwm_resource *pwm_resource;
	struct vbios_pwm vb_pwm;
	u32 pwm_size;

	if (IS_ERR_OR_NULL(vbios) || IS_ERR_OR_NULL(data))
		return false;

	pwm_resource = kvmalloc(sizeof(*pwm_resource), GFP_KERNEL);
	if (IS_ERR_OR_NULL(pwm_resource))
		return false;

	pwm_size = sizeof(struct vbios_pwm);
	memset(&vb_pwm, VBIOS_DATA_INVAL, pwm_size);
	memcpy(&vb_pwm, data, min(size, pwm_size));

	pwm_resource->base.link = link;
	pwm_resource->base.type = GSGPU_RESOURCE_PWM;

	pwm_resource->feature = vb_pwm.feature;
	pwm_resource->pwm = vb_pwm.pwm;
	pwm_resource->polarity = vb_pwm.polarity;
	pwm_resource->period = vb_pwm.period;

	list_add_tail(&pwm_resource->base.node, &vbios->resource_list);

	return true;
}

static bool vbios_create_gpu_resource(struct gsgpu_vbios *vbios, void *data,
				      u32 link, u32 size)
{
	struct gpu_resource *gpu_resource;
	struct vbios_gpu vb_gpu;
	u32 gpu_size;

	if (IS_ERR_OR_NULL(vbios) || IS_ERR_OR_NULL(data))
		return false;

	gpu_resource = kvmalloc(sizeof(*gpu_resource), GFP_KERNEL);
	if (IS_ERR_OR_NULL(gpu_resource))
		return false;

	gpu_size = sizeof(struct vbios_gpu);
	memset(&vb_gpu, VBIOS_DATA_INVAL, gpu_size);
	memcpy(&vb_gpu, data, min(size, gpu_size));

	gpu_resource->base.link = 0;
	gpu_resource->base.type = GSGPU_RESOURCE_GPU;

	gpu_resource->vram_type = vb_gpu.type;
	gpu_resource->bit_width = vb_gpu.bit_width;
	gpu_resource->cap = vb_gpu.cap;
	gpu_resource->freq_count = vb_gpu.freq_count;
	gpu_resource->freq = vb_gpu.freq;
	gpu_resource->shaders_num = vb_gpu.shaders_num;
	gpu_resource->shaders_freq = vb_gpu.shaders_freq;

	list_add_tail(&gpu_resource->base.node, &vbios->resource_list);

	return true;
}

static bool vbios_create_ext_encoder_resource(struct gsgpu_vbios *vbios,
					      void *data, u32 link, u32 size)
{
	struct ext_encoder_resource *ext_encoder_resource;
	struct vbios_ext_encoder *vb_ext_encoder = kvmalloc(sizeof(struct vbios_ext_encoder),
							    GFP_KERNEL);
	if (IS_ERR_OR_NULL(vb_ext_encoder)) {
		DRM_ERROR("Unable to allocate kernel memory, size = 0x%zx\n",
			  sizeof(struct vbios_ext_encoder));
		return false;
	}
	u32 ext_encoder_size;

	if (IS_ERR_OR_NULL(vbios) || IS_ERR_OR_NULL(data))
		return false;

	ext_encoder_resource = kvmalloc(sizeof(*ext_encoder_resource), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ext_encoder_resource))
		return false;

	ext_encoder_size = sizeof(struct vbios_ext_encoder);
	memset(vb_ext_encoder, VBIOS_DATA_INVAL, ext_encoder_size);
	memcpy(vb_ext_encoder, data, min(size, ext_encoder_size));

	ext_encoder_resource->base.link = link;
	ext_encoder_resource->base.type = GSGPU_RESOURCE_EXT_ENCODER;

	ext_encoder_resource->data_checksum = vb_ext_encoder->data_checksum;
	ext_encoder_resource->data_size = vb_ext_encoder->data_size;
	memcpy(ext_encoder_resource->data, vb_ext_encoder->data, vb_ext_encoder->data_size);

	list_add_tail(&ext_encoder_resource->base.node, &vbios->resource_list);
	kfree(vb_ext_encoder);
	return true;
}

typedef bool (*vbios_resource_create)(struct gsgpu_vbios *vbios,
				      void *data, u32 link, u32 size);

struct vbios_resource_ops {
	enum desc_type desc_type;
	u16 desc_ver;
	enum resource_type res_type;
	size_t res_offset;
	bool check_link;
	vbios_resource_create create;
};

#define VBIOS_RESOURCE_OPS(dty, dver, rty_enum, rty, cl, c)		\
	{ .desc_type = dty, .desc_ver = dver, .res_type = rty_enum,	\
	  .res_offset = offsetof(struct rty, base),			\
	  .check_link = cl, .create = c }

static struct vbios_resource_ops vbios_resource_table[] = {
	VBIOS_RESOURCE_OPS(desc_header, ver_v1, GSGPU_RESOURCE_HEADER,
			   header_resource, false, vbios_create_header_resource),
	VBIOS_RESOURCE_OPS(desc_crtc, ver_v1, GSGPU_RESOURCE_CRTC,
			   crtc_resource, true, vbios_create_crtc_resource),
	VBIOS_RESOURCE_OPS(desc_encoder, ver_v1, GSGPU_RESOURCE_ENCODER,
			   encoder_resource, true, vbios_create_encoder_resource),
	VBIOS_RESOURCE_OPS(desc_connector, ver_v1, GSGPU_RESOURCE_CONNECTOR,
			   connector_resource, true, vbios_create_connector_resource),
	VBIOS_RESOURCE_OPS(desc_i2c, ver_v1, GSGPU_RESOURCE_I2C,
			   i2c_resource, true, vbios_create_i2c_resource),
	VBIOS_RESOURCE_OPS(desc_gpio, ver_v1, GSGPU_RESOURCE_GPIO,
			   gpio_resource, true, vbios_create_gpio_resource),
	VBIOS_RESOURCE_OPS(desc_pwm, ver_v1, GSGPU_RESOURCE_PWM,
			   pwm_resource, true, vbios_create_pwm_resource),
	VBIOS_RESOURCE_OPS(desc_gpu, ver_v1, GSGPU_RESOURCE_GPU,
			   gpu_resource, false, vbios_create_gpu_resource),
	VBIOS_RESOURCE_OPS(desc_res_encoder, ver_v1, GSGPU_RESOURCE_EXT_ENCODER,
			   ext_encoder_resource, true, vbios_create_ext_encoder_resource)
};

static bool parse_vbios_resource(struct gsgpu_vbios *vbios, struct vbios_desc *desc)
{
	u8 *data = vbios->vbios_ptr + desc->offset;
	if (!data) {
		pr_err("Invalid vbios descriptor: data offset 0x%x overflows\n",
		       desc->offset);
		return false;
	}

	for (int i = 0; i < ARRAY_SIZE(vbios_resource_table); i++) {
		if ((vbios_resource_table[i].desc_ver == desc->ver) &&
		    (vbios_resource_table[i].desc_type == desc->type)) {
			int r = vbios_resource_table[i].create(vbios, data, desc->link,
							       desc->size);
			if (!r) {
				pr_err("Error parsing vbios data. Error = %d", r);
				return false;
			}
			return true;
		}
	}

	DRM_ERROR("No parser for descriptor type %d, version %d, data 0x%px.\n",
		  desc->type, desc->ver, data);
	return false;
}

static void parse_legacy_vbios_info(struct gsgpu_vbios *vbios)
{
	struct vbios_info *vb_info;
	struct header_resource *header;

	if (IS_ERR_OR_NULL(vbios))
		return;

	header = dc_get_vbios_resource(vbios, 0, GSGPU_RESOURCE_HEADER);
	if (IS_ERR_OR_NULL(header))
		return;

	vb_info = (struct vbios_info *)vbios->vbios_ptr;
	header->links = vb_info->link_num;
	header->ver_major = vb_info->version_major;
	header->ver_minor = vb_info->version_minor;
	memcpy(header->name, vb_info->title, 16);
}

/* Return the pointer to the beginning of the vbios resource descriptor table.
 * The zeroth desciptor always points to the header resource. Diagramatically,
 *
 *  |-------------------------------|------------|
 *  |vbios_desc[0]|vbios_desc[1]|...|vbios_header|
 *  |-------------------------------|------------|
 *  |  .offset                      ^
 *  |     |                         | vbios_header.desc_offset == VBIOS_DESC_OFFSET
 *  |     |_________________________|
 *  |     == VBIOS_DESC_OFFSET + vbios_header.desc_size
 *  |                                            |
 *  |  .size = size of the resource object,      |
 *  |          in this case sizeof(vbios_header) |
 *  |  (Note: this is not sizeof(vbios_desc[0])) |
 *  |                                            |
 *  ^                                            |
 *  VBIOS_DESC_OFFSET                            |
 *  ^                                            |
 *  |____________________________________________|
 *
 * This table is placed at the fixed offset VBIOS_DESC_OFFSET from the beginning
 * of the VBIOS. vbios_header.desc_offset points back to the beginning of the
 * descriptor table, so vbios_header.desc_offset must equal VBIOS_DESC_OFFSET.
 *
 * All descriptors have the same size sizeof(vbios_desc[0]). The vbios_header structure
 * immediately follows the descriptor table. The size of the full descriptor
 * table is stored in vbios_header.desc_size. This implies that vbios_desc[0].offset
 * must equal VBIOS_DESC_OFFSET + vbios_header.desc_size. The number of descriptors
 * is vbios_header.desc_size / sizeof(vbios_desc[0]). vbios_header.desc_size must
 * be able to divide sizeof(vbios_desc[0]).
 */
static inline struct vbios_desc *get_vbios_desc_table(u8 *vbios)
{
	return (struct vbios_desc *)(vbios + VBIOS_DESC_OFFSET);
}

static inline struct vbios_header *get_vbios_header(u8 *vbios) {
	return (struct vbios_header *)(vbios + get_vbios_desc_table(vbios)->offset);
}

static inline bool validate_vbios_desc_table(u8 *vbios)
{
	struct vbios_desc *desc = get_vbios_desc_table(vbios);
	/* check if the first vbios resource descriptor is the header */
	if (desc->type != desc_header) {
		DRM_ERROR("Invalid vbios: first desciptor is not of header type\n");
		return false;
	}

	struct vbios_header *vb_header = get_vbios_header(vbios);
	if (vb_header->desc_offset != VBIOS_DESC_OFFSET) {
		DRM_ERROR("Invalid vbios: vbios_header->desc_offset (0x%x) != VBIOS_DESC_OFFSET\n",
			  vb_header->desc_offset);
		return false;
	}
	if (vb_header->desc_size % sizeof(struct vbios_desc) != 0) {
		DRM_ERROR("Invalid vbios: vb_header->desc_size (0x%x) does not divide 0x%lx\n",
			  vb_header->desc_size, sizeof(struct vbios_desc));
		return false;
	}
	if (desc->offset != (VBIOS_DESC_OFFSET + vb_header->desc_size)) {
		DRM_ERROR("Invalid vbios: desc->offset (0x%x) != (VBIOS_DESC_OFFSET + vb_header->desc_size (0x%x))\n",
			  desc->offset, vb_header->desc_size);
		return false;
	}
	return true;
}

static bool dc_vbios_parse(struct gsgpu_vbios *vbios)
{
	if (IS_ERR_OR_NULL(vbios)) {
		BUG();
		return false;
	}

	u8 *vbios_ptr = (u8 *)vbios->vbios_ptr;
	if (IS_ERR_OR_NULL(vbios_ptr)) {
		BUG();
		return false;
	}

	if (!validate_vbios_desc_table(vbios_ptr)) {
		BUG();
		return false;
	}

	/* parse vbios resources */
	struct vbios_desc *desc = get_vbios_desc_table(vbios_ptr);
	struct vbios_header *vb_header = get_vbios_header(vbios_ptr);
	int num_desc = vb_header->desc_size / sizeof(struct vbios_desc);
	for (int i = 0; i < num_desc; i++) {
		if (desc[i].type == end_of_desc_table) {
			break;
		}
		parse_vbios_resource(vbios, &desc[i]);
	}

	/* append legacy information to the header resource */
	parse_legacy_vbios_info(vbios);

	return true;
}

u8 gsgpu_vbios_checksum(const u8 *data, int size)
{
	u8 sum = 0;

	while (size--)
		sum += *data++;
	return sum;
}

u32 gsgpu_vbios_version(struct gsgpu_vbios *vbios)
{
	struct vbios_info *vb_info = (struct vbios_info *)vbios->vbios_ptr;
	u32 minor, major, version;

	major = vb_info->version_major;
	minor = vb_info->version_minor;
	version = major * 10 + minor;

	return version;
}

void *dc_get_vbios_resource(struct gsgpu_vbios *vbios, u32 link,
			    enum resource_type type)
{
	BUG_ON(IS_ERR_OR_NULL(vbios));

	for (int i = 0; i < ARRAY_SIZE(vbios_resource_table); i++) {
		if (type == vbios_resource_table[i].res_type) {
			bool check_link = vbios_resource_table[i].check_link;
			struct resource_object *entry;
			list_for_each_entry(entry, &vbios->resource_list, node) {
				if ((!check_link || entry->link == link) && entry->type == type) {
					return (void *)((u8 *)entry - vbios_resource_table[i].res_offset);
				}
			}
		}
	}

	return NULL;
}

static void dc_vbios_show(struct gsgpu_vbios *vbios)
{
	struct crtc_resource *crtc_res;
	struct gpu_resource *gpu_res;
	char *vram_type[] = {"DDR3", "DDR4", "DDR5"};
	int i;

	struct header_resource *header_res = dc_get_vbios_resource(vbios, 0,
								   GSGPU_RESOURCE_HEADER);
	if (header_res == NULL)
		return;

	DRM_INFO("GSGPU vbios header info:\n");
	DRM_INFO("ver: %d.%d, links: %d, max_planes: %d, name: %s\n",
		 header_res->ver_major, header_res->ver_minor,
		 header_res->links, header_res->max_planes, header_res->name);
	DRM_INFO("oem-vendor: %s, oem-product: %s\n", header_res->oem_vendor,
		 header_res->oem_product);

	for (i = 0; i < header_res->links; i++) {
		crtc_res = dc_get_vbios_resource(vbios, i, GSGPU_RESOURCE_CRTC);
		DRM_INFO("GSGPU vbios crtc: %d, max freq: %dHz, width: %d, height: %d\n",
			 i, crtc_res->max_freq, crtc_res->max_width, crtc_res->max_height);
	}

	gpu_res = dc_get_vbios_resource(vbios, 0, GSGPU_RESOURCE_GPU);
	if (!gpu_res)
		DRM_WARN("Failed to obtain video memory and gpu information from VBIOS!\n");
	else {
		dev_info(vbios->dc->adev->dev, "VRAM: %dM %s %dbit %dMhz.\n",
			 gpu_res->cap, vram_type[gpu_res->vram_type],
			 gpu_res->bit_width, gpu_res->freq);
		dev_info(vbios->dc->adev->dev,
			 "GSGPU: shaders_num: %d, shaders_freq: %dMHz, freq_count: %d.\n",
			 gpu_res->shaders_num, gpu_res->shaders_freq, gpu_res->freq_count);
	}
}

static bool is_valid_vbios(u8 *vbios)
{
	struct vbios_info *vb_info = (struct vbios_info *)vbios;
	char header[VBIOS_HEADER_MAX_TITLE_SIZE] = {0};
	memcpy(header, vb_info->title, VBIOS_HEADER_MAX_TITLE_SIZE);

	if (memcmp(header, LOONGSON_VBIOS_TITLE, strlen(LOONGSON_VBIOS_TITLE))) {
		header[VBIOS_HEADER_MAX_TITLE_SIZE-1] = '\0';
		DRM_WARN("invalid vbios signature, expected %s, got %s\n",
			 LOONGSON_VBIOS_TITLE, header);
		return false;
	}

	return true;
}

/*
 * Read the VBIOS from mapped PCI ROM
 */
static u8 *read_vbios_from_mapped_io(u64 phy_addr)
{
	void *mapped_vbios = ioremap(phy_addr, VBIOS_SIZE);
	if (!mapped_vbios) {
		DRM_WARN("Unable to map VBIOS physical address 0x%px.\n",
			 (void *)(phy_addr));
		return false;
	}

	u8 *vbios = kmalloc(VBIOS_SIZE, GFP_KERNEL);
	if (IS_ERR_OR_NULL(vbios)) {
		DRM_WARN("Unable to allocate kernel memory for VBIOS.\n");
		return false;
	}

	memcpy(vbios, mapped_vbios, VBIOS_SIZE);
	iounmap(mapped_vbios);

	DRM_INFO("Got VBIOS from mapped PCI ROM!\n");
	return vbios;
}

#ifdef CONFIG_ACPI
/*
 * Attempt to read from the ACPI VIAT table and copy the VBIOS data into a kernel
 * buffer. The caller is responsible for freeing the returned buffer using kfree.
 */
static u8 *read_vbios_from_acpi(void)
{
	struct acpi_table_header *hdr;
	if (!ACPI_SUCCESS(acpi_get_table("VIAT", 1, &hdr)))
		return false;

	acpi_size tbl_size = hdr->length;
	if (tbl_size != sizeof(struct acpi_viat_table)) {
		DRM_WARN("ACPI viat table present but broken (length error #1)\n");
		return false;
	}

	struct acpi_viat_table *viat = (struct acpi_viat_table *)hdr;
	u8 *vbios = kmalloc(VBIOS_SIZE, GFP_KERNEL);
	if (IS_ERR_OR_NULL(vbios)) {
		DRM_WARN("Unable to allocate kernel memory for VBIOS!\n");
		return false;
	}

	void *vaddr = phys_to_virt(viat->vbios_addr);
	memcpy(vbios, vaddr, VBIOS_SIZE);
	DRM_INFO("Got vbios from ACPI!\n");
	return vbios;
}
#else
static u8 *read_vbios_from_acpi(void)
{
	return NULL;
}
#endif

static u8 *read_and_validate_vbios(u64 phy_addr)
{
	/* Try reading from mapped PCI ROM first */
	u8 *vbios = read_vbios_from_mapped_io(phy_addr);

	/* If that didn't work, try ACPI */
	if (!vbios) {
		vbios = read_vbios_from_acpi();
	}

	/* TODO: Add a default VBIOS in case of failure. */
	if (!vbios) {
		DRM_ERROR("Unable to locate VBIOS ROM!\n");
		return NULL;
	}

	if (!is_valid_vbios(vbios)) {
		DRM_ERROR("VBIOS Signature Invalid!\n");
		kfree(vbios);
		return NULL;
	}

	return vbios;
}

static bool dc_get_vbios_data(struct gsgpu_dc *dc)
{
	u64 phy_addr = dc->adev->gmc.aper_base + dc->adev->gmc.aper_size - VBIOS_OFFSET;

	dc->vbios->vbios_ptr = read_and_validate_vbios(phy_addr);

	return dc->vbios->vbios_ptr != NULL;
}

bool dc_vbios_init(struct gsgpu_dc *dc)
{
	bool status;
	bool ret;

	if (IS_ERR_OR_NULL(dc))
		return false;

	dc->vbios = kmalloc(sizeof(*dc->vbios), GFP_KERNEL);
	if (IS_ERR_OR_NULL(dc->vbios))
		return false;

	dc->vbios->dc = dc;
	INIT_LIST_HEAD(&dc->vbios->resource_list);

	status = dc_get_vbios_data(dc);
	if (!status) {
		DRM_ERROR("Failed to get DC VBIOS data!\n");
		return false;
	}

	ret = dc_vbios_parse(dc->vbios);
	if (!ret) {
		pr_err("Failed to parse DC VBIOS!\n");
		kvfree(dc->vbios);
		dc->vbios = NULL;
	}

	dc_vbios_show(dc->vbios);

	/* Check if the encoder is one of the chips we support */
	struct header_resource *header = dc_get_vbios_resource(dc->vbios, 0,
							       GSGPU_RESOURCE_HEADER);
	for (int i = 0; i < header->links; i++) {
		struct encoder_resource *enc = dc_get_vbios_resource(dc->vbios, i,
								     GSGPU_RESOURCE_ENCODER);
		switch (enc->chip) {
		case INTERNAL_DVO:
		case INTERNAL_HDMI:
		case EDP_LT9721:
		case EDP_LT6711:
		case LVDS_LT8619:
		case EDP_NCS8805:
			/* We support them, so continue. */
			break;
		default:
			DRM_ERROR("Unsupported encoder chip type %d\n", enc->chip);
			return false;
		}
	}

	return true;
}

static void vbios_resource_pool_destory(struct gsgpu_vbios *vbios)
{
	struct resource_object *entry, *tmp;
	list_for_each_entry_safe(entry, tmp, &vbios->resource_list, node) {
		list_del(&entry->node);
		kvfree(entry);
		entry = NULL;
	}
}

void dc_vbios_exit(struct gsgpu_vbios *vbios)
{
	if (IS_ERR_OR_NULL(vbios))
		return;

	if (!IS_ERR_OR_NULL(vbios->vbios_ptr)) {
		kvfree(vbios->vbios_ptr);
		vbios->vbios_ptr = NULL;
	}

	vbios_resource_pool_destory(vbios);

	kvfree(vbios);
	vbios = NULL;
}
