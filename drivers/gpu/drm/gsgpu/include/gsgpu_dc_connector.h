#ifndef __GSGPU_DC_CONNECTOR__
#define __GSGPU_DC_CONNECTOR__

#include <drm/drmP.h>
#include "gsgpu.h"
#include "gsgpu_dc_resource.h"

struct dc_connector_state {
	struct drm_connector_state base;
	enum gsgpu_rmx_type scaling;
	uint8_t underscan_vborder;
	uint8_t underscan_hborder;
	uint8_t max_bpc;
	bool underscan_enable;
};

struct gsgpu_dc_connector {
	struct connector_resource *resource;
	struct gsgpu_dc *dc;
	struct list_head node;
};

#define to_dc_connector_state(x)\
	container_of((x), struct dc_connector_state, base)

struct gsgpu_dc_connector *dc_connector_construct(struct gsgpu_dc *dc, struct connector_resource *resource);
int gsgpu_dc_connector_init(struct gsgpu_device *adev, uint32_t link_index);

#endif /* __GSGPU_DC_CONNECTOR__ */
