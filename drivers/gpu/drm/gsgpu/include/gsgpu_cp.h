#ifndef __GSGPU_CP_H__
#define __GSGPU_CP_H__

#include <linux/delay.h>

int gsgpu_cp_init(struct gsgpu_device *adev);

int gsgpu_cp_gfx_load_microcode(struct gsgpu_device *adev);
int gsgpu_cp_enable(struct gsgpu_device *adev, bool enable);

int gsgpu_cp_fini(struct gsgpu_device *adev);

static inline bool gsgpu_cp_wait_done(struct gsgpu_device *adev)
{
	int i;
	for (i = 0; i < adev->usec_timeout; i++) {

		if (RREG32(GSGPU_STATUS) == GSCMD_STS_DONE)
			return true;

		msleep(1);
	}

	dev_err(adev->dev, "\n gsgpu cp hang!!! \n");
	return false;
}

#endif /* __GSGPU_CP_H__ */
