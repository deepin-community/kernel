#ifndef __DC_VBIOS_H__
#define __DC_VBIOS_H__

#include "gsgpu_dc_resource.h"

#define VBIOS_VERSION_V1_1 (11)

enum gsgpu_edid_method {
	via_null = 0,
	via_i2c,
	via_vbios,
	via_encoder,
	via_max = 0xffff,
} __packed;

struct gsgpu_vbios;
struct vbios_desc;
struct gsgpu_dc;

void *dc_get_vbios_resource(struct gsgpu_vbios *vbios, u32 link,
			    enum resource_type type);
bool dc_vbios_init(struct gsgpu_dc *dc);
void dc_vbios_exit(struct gsgpu_vbios *vbios);
u8 gsgpu_vbios_checksum(const u8 *data, int size);
u32 gsgpu_vbios_version(struct gsgpu_vbios *vbios);
bool check_vbios_info(void);

#endif
