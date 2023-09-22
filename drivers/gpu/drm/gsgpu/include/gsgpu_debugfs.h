#ifndef __GSGPU_DEBUGFS_H__
#define __GSGPU_DEBUGFS_H__

struct gsgpu_debugfs {
	const struct drm_info_list	*files;
	unsigned		num_files;
};

int gsgpu_debugfs_regs_init(struct gsgpu_device *adev);
void gsgpu_debugfs_regs_cleanup(struct gsgpu_device *adev);
int gsgpu_debugfs_init(struct gsgpu_device *adev);
int gsgpu_debugfs_add_files(struct gsgpu_device *adev,
			     const struct drm_info_list *files,
			     unsigned nfiles);
int gsgpu_debugfs_fence_init(struct gsgpu_device *adev);
int gsgpu_debugfs_firmware_init(struct gsgpu_device *adev);
int gsgpu_debugfs_gem_init(struct gsgpu_device *adev);
int gsgpu_debugfs_sema_init(struct gsgpu_device *adev);

#endif /* __GSGPU_DEBUGFS_H__ */
