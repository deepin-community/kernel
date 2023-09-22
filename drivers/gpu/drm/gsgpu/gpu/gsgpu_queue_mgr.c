#include "gsgpu.h"
#include "gsgpu_ring.h"

static int gsgpu_queue_mapper_init(struct gsgpu_queue_mapper *mapper,
				    int hw_ip)
{
	if (!mapper)
		return -EINVAL;

	if (hw_ip > GSGPU_MAX_IP_NUM)
		return -EINVAL;

	mapper->hw_ip = hw_ip;
	mutex_init(&mapper->lock);

	memset(mapper->queue_map, 0, sizeof(mapper->queue_map));

	return 0;
}

static struct gsgpu_ring *gsgpu_get_cached_map(struct gsgpu_queue_mapper *mapper,
					  int ring)
{
	return mapper->queue_map[ring];
}

static int gsgpu_update_cached_map(struct gsgpu_queue_mapper *mapper,
			     int ring, struct gsgpu_ring *pring)
{
	if (WARN_ON(mapper->queue_map[ring])) {
		DRM_ERROR("Un-expected ring re-map\n");
		return -EINVAL;
	}

	mapper->queue_map[ring] = pring;

	return 0;
}

static int gsgpu_identity_map(struct gsgpu_device *adev,
			       struct gsgpu_queue_mapper *mapper,
			       u32 ring,
			       struct gsgpu_ring **out_ring)
{
	switch (mapper->hw_ip) {
	case GSGPU_HW_IP_GFX:
		*out_ring = &adev->gfx.gfx_ring[ring];
		break;
	case GSGPU_HW_IP_DMA:
		*out_ring = &adev->xdma.instance[ring].ring;
		break;
	default:
		*out_ring = NULL;
		DRM_ERROR("unknown HW IP type: %d\n", mapper->hw_ip);
		return -EINVAL;
	}

	return gsgpu_update_cached_map(mapper, ring, *out_ring);
}

static enum gsgpu_ring_type gsgpu_hw_ip_to_ring_type(int hw_ip)
{
	switch (hw_ip) {
	case GSGPU_HW_IP_GFX:
		return GSGPU_RING_TYPE_GFX;
	case GSGPU_HW_IP_DMA:
		return GSGPU_RING_TYPE_XDMA;
	default:
		DRM_ERROR("Invalid HW IP specified %d\n", hw_ip);
		return -1;
	}
}

static int gsgpu_lru_map(struct gsgpu_device *adev,
			  struct gsgpu_queue_mapper *mapper,
			  u32 user_ring, bool lru_pipe_order,
			  struct gsgpu_ring **out_ring)
{
	int r, i, j;
	int ring_type = gsgpu_hw_ip_to_ring_type(mapper->hw_ip);
	int ring_blacklist[GSGPU_MAX_RINGS];
	struct gsgpu_ring *ring;

	/* 0 is a valid ring index, so initialize to -1 */
	memset(ring_blacklist, 0xff, sizeof(ring_blacklist));

	for (i = 0, j = 0; i < GSGPU_MAX_RINGS; i++) {
		ring = mapper->queue_map[i];
		if (ring)
			ring_blacklist[j++] = ring->idx;
	}

	r = gsgpu_ring_lru_get(adev, ring_type, ring_blacklist,
				j, lru_pipe_order, out_ring);
	if (r)
		return r;

	return gsgpu_update_cached_map(mapper, user_ring, *out_ring);
}

/**
 * gsgpu_queue_mgr_init - init an gsgpu_queue_mgr struct
 *
 * @adev: gsgpu_device pointer
 * @mgr: gsgpu_queue_mgr structure holding queue information
 *
 * Initialize the the selected @mgr (all asics).
 *
 * Returns 0 on success, error on failure.
 */
int gsgpu_queue_mgr_init(struct gsgpu_device *adev,
			  struct gsgpu_queue_mgr *mgr)
{
	int i, r;

	if (!adev || !mgr)
		return -EINVAL;

	memset(mgr, 0, sizeof(*mgr));

	for (i = 0; i < GSGPU_MAX_IP_NUM; ++i) {
		r = gsgpu_queue_mapper_init(&mgr->mapper[i], i);
		if (r)
			return r;
	}

	return 0;
}

/**
 * gsgpu_queue_mgr_fini - de-initialize an gsgpu_queue_mgr struct
 *
 * @adev: gsgpu_device pointer
 * @mgr: gsgpu_queue_mgr structure holding queue information
 *
 * De-initialize the the selected @mgr (all asics).
 *
 * Returns 0 on success, error on failure.
 */
int gsgpu_queue_mgr_fini(struct gsgpu_device *adev,
			  struct gsgpu_queue_mgr *mgr)
{
	return 0;
}

/**
 * gsgpu_queue_mgr_map - Map a userspace ring id to an gsgpu_ring
 *
 * @adev: gsgpu_device pointer
 * @mgr: gsgpu_queue_mgr structure holding queue information
 * @hw_ip: HW IP enum
 * @instance: HW instance
 * @ring: user ring id
 * @our_ring: pointer to mapped gsgpu_ring
 *
 * Map a userspace ring id to an appropriate kernel ring. Different
 * policies are configurable at a HW IP level.
 *
 * Returns 0 on success, error on failure.
 */
int gsgpu_queue_mgr_map(struct gsgpu_device *adev,
			 struct gsgpu_queue_mgr *mgr,
			 u32 hw_ip, u32 instance, u32 ring,
			 struct gsgpu_ring **out_ring)
{
	int r, ip_num_rings = 0;
	struct gsgpu_queue_mapper *mapper = &mgr->mapper[hw_ip];

	if (!adev || !mgr || !out_ring)
		return -EINVAL;

	if (hw_ip >= GSGPU_MAX_IP_NUM)
		return -EINVAL;

	if (ring >= GSGPU_MAX_RINGS)
		return -EINVAL;

	/* Right now all IPs have only one instance - multiple rings. */
	if (instance != 0) {
		DRM_DEBUG("invalid ip instance: %d\n", instance);
		return -EINVAL;
	}

	switch (hw_ip) {
	case GSGPU_HW_IP_GFX:
		ip_num_rings = adev->gfx.num_gfx_rings;
		break;
	case GSGPU_HW_IP_DMA:
		ip_num_rings = adev->xdma.num_instances;
		break;
	default:
		DRM_DEBUG("unknown ip type: %d\n", hw_ip);
		return -EINVAL;
	}

	if (ring >= ip_num_rings) {
		DRM_DEBUG("Ring index:%d exceeds maximum:%d for ip:%d\n",
			  ring, ip_num_rings, hw_ip);
		return -EINVAL;
	}

	mutex_lock(&mapper->lock);

	*out_ring = gsgpu_get_cached_map(mapper, ring);
	if (*out_ring) {
		/* cache hit */
		r = 0;
		goto out_unlock;
	}

	switch (mapper->hw_ip) {
	case GSGPU_HW_IP_GFX:
		r = gsgpu_identity_map(adev, mapper, ring, out_ring);
		break;
	case GSGPU_HW_IP_DMA:
		r = gsgpu_lru_map(adev, mapper, ring, false, out_ring);
		break;
	default:
		*out_ring = NULL;
		r = -EINVAL;
		DRM_DEBUG("unknown HW IP type: %d\n", mapper->hw_ip);
	}

out_unlock:
	mutex_unlock(&mapper->lock);
	return r;
}
