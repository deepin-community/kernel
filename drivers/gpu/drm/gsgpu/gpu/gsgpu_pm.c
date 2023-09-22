#include "gsgpu.h"
#include "gsgpu_dc_resource.h"
#include "gsgpu_dc_vbios.h"

#define FREQ_LEVEL0 0xf
#define FREQ_LEVEL1 0xd
#define FREQ_LEVEL2 0xb
#define FREQ_LEVEL3 0x9
#define FREQ_LEVEL4 0x7
#define FREQ_LEVEL5 0x5
#define FREQ_LEVEL6 0x3
#define FREQ_LEVEL7 0x1

#define SET_LEVEL0  0
#define SET_LEVEL1  1
#define SET_LEVEL2  2
#define SET_LEVEL3  3
#define SET_LEVEL4  4
#define SET_LEVEL5  5
#define SET_LEVEL6  6
#define SET_LEVEL7  7

#define STATIC_FREQ						0x00

static ssize_t gsgpu_get_gpu_clk(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct gsgpu_device *adev = ddev->dev_private;
	struct gpu_resource *gpu_res = NULL;

	int i, now, size = 0;
	int number = 8;
	uint32_t regular_freq_count;
	uint32_t max_freq_value;
	uint32_t value;
	uint32_t freq;
	uint32_t default_freq_count = 3;
	uint32_t default_freq_value = 480;

	gpu_res = dc_get_vbios_resource(adev->dc->vbios, 0, GSGPU_RESOURCE_GPU);

	if (NULL == gpu_res) {
		regular_freq_count = default_freq_count;
		max_freq_value = default_freq_value;
	} else {
		regular_freq_count = gpu_res->count_freq;
		max_freq_value = gpu_res->shaders_freq;

	}
	value = max_freq_value;
	freq = RREG32(GSGPU_FREQ_SCALE);

	switch (freq) {
	case FREQ_LEVEL0:
		now = SET_LEVEL0;
		break;
	case FREQ_LEVEL1:
		now = SET_LEVEL1;
		break;
	case FREQ_LEVEL2:
		now = SET_LEVEL2;
		break;
	case FREQ_LEVEL3:
		now = SET_LEVEL3;
		break;
	case FREQ_LEVEL4:
		now = SET_LEVEL4;
		break;
	case FREQ_LEVEL5:
		now = SET_LEVEL5;
		break;
	case FREQ_LEVEL6:
		now = SET_LEVEL6;
		break;
	case FREQ_LEVEL7:
		now = SET_LEVEL7;
		break;
	default:
		now = 0;
		break;
	}

	for (i = 0; i < regular_freq_count; i++) {
		size += sprintf(buf + size, "%d: %uMhz %s\n",
				i, value,
				(i == now) ? "*" : "");
		number = number - 1;
		value = max_freq_value / 8 * number;

	}

	return size;
}

static ssize_t gsgpu_read_level(const char *buf, size_t count, uint32_t max_level)
{
	int ret;
	char *tmp;
	char **str;
	char *sub_str = NULL;
	char buf_cpy[100];
	const char delimiter[3] = {' ', '\n', '\0'};

	memcpy(buf_cpy, buf, count);
	tmp = buf_cpy;

	if (NULL == tmp)
		return -EINVAL;

	sub_str = strsep(&tmp, delimiter);
	if (0 == strlen(sub_str))
		return -EINVAL;

	ret = simple_strtoul(sub_str, str, 10);
	if (ret >= 0 && ret <= (max_level - 1))
		return ret;
	else
		return -EINVAL;
}

static ssize_t gsgpu_set_gpu_clk(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct gsgpu_device *adev = ddev->dev_private;
	struct gpu_resource *gpu_res = NULL;
	uint32_t level;
	uint32_t regular_freq_count;
	uint32_t default_freq_count = 3;

	gpu_res = dc_get_vbios_resource(adev->dc->vbios, 0, GSGPU_RESOURCE_GPU);

	if (NULL == gpu_res)
		regular_freq_count = default_freq_count;
	else
		regular_freq_count = gpu_res->count_freq;

	level = gsgpu_read_level(buf, count, regular_freq_count);

	switch (level) {
	case SET_LEVEL0:
		level = FREQ_LEVEL0;
		break;
	case SET_LEVEL1:
		level = FREQ_LEVEL1;
		break;
	case SET_LEVEL2:
		level = FREQ_LEVEL2;
		break;
	case SET_LEVEL3:
		level = FREQ_LEVEL3;
		break;
	case SET_LEVEL4:
		level = FREQ_LEVEL4;
		break;
	case SET_LEVEL5:
		level = FREQ_LEVEL5;
		break;
	case SET_LEVEL6:
		level = FREQ_LEVEL6;
		break;
	case SET_LEVEL7:
		level = FREQ_LEVEL7;
		break;
	default:
		level = FREQ_LEVEL0;
		break;
	}

	gsgpu_cmd_exec(adev, GSCMD(GSCMD_FREQ, STATIC_FREQ), level, 0);

	return count;
}

static DEVICE_ATTR(gpu_clk, S_IRUGO | S_IWUSR,
		gsgpu_get_gpu_clk,
		gsgpu_set_gpu_clk);

int gsgpu_pm_sysfs_init(struct gsgpu_device *adev)
{
	int ret;

    ret = device_create_file(adev->dev, &dev_attr_gpu_clk);
	if (ret) {
		DRM_ERROR("failed to create device file for gpu clk\n");
		return ret;
	}

    return 0;
}
