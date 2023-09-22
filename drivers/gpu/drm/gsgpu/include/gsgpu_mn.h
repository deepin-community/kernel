#ifndef __GSGPU_MN_H__
#define __GSGPU_MN_H__

/*
 * MMU Notifier
 */
struct gsgpu_mn;

enum gsgpu_mn_type {
	GSGPU_MN_TYPE_GFX,
	GSGPU_MN_TYPE_HSA,
};

#if defined(CONFIG_MMU_NOTIFIER)
void gsgpu_mn_lock(struct gsgpu_mn *mn);
void gsgpu_mn_unlock(struct gsgpu_mn *mn);
struct gsgpu_mn *gsgpu_mn_get(struct gsgpu_device *adev,
				enum gsgpu_mn_type type);
int gsgpu_mn_register(struct gsgpu_bo *bo, unsigned long addr);
void gsgpu_mn_unregister(struct gsgpu_bo *bo);
#else
static inline void gsgpu_mn_lock(struct gsgpu_mn *mn) {}
static inline void gsgpu_mn_unlock(struct gsgpu_mn *mn) {}
static inline struct gsgpu_mn *gsgpu_mn_get(struct gsgpu_device *adev,
					      enum gsgpu_mn_type type)
{
	return NULL;
}
static inline int gsgpu_mn_register(struct gsgpu_bo *bo, unsigned long addr)
{
	return -ENODEV;
}
static inline void gsgpu_mn_unregister(struct gsgpu_bo *bo) {}
#endif

#endif /* __GSGPU_MN_H__ */
