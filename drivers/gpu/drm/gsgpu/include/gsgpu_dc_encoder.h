#ifndef STREAM_ENCODER_H_
#define STREAM_ENCODER_H_

#include "gsgpu_dc_resource.h"

struct gsgpu_dc_encoder {
	struct encoder_resource *resource;
	struct gsgpu_dc *dc;
	bool has_ext_encoder;
};

struct gsgpu_dc_encoder *dc_encoder_construct(struct gsgpu_dc *dc, struct encoder_resource *resource);
int gsgpu_dc_encoder_init(struct gsgpu_device *adev, int link_index);

#endif /* STREAM_ENCODER_H_ */
