#ifndef __GSGPU_HW_SEMA_H__
#define __GSGPU_HW_SEMA_H__

#define GSGPU_NUM_SEMA 32

struct gsgpu_vm;

struct gsgpu_hw_sema {
	struct list_head	list;
	struct gsgpu_vm		*vm;
    bool		own;
	unsigned	pasid;
	unsigned	ctx;
};

struct gsgpu_hw_sema_mgr {
	struct mutex		lock;
	unsigned		num_ids;
	struct list_head	sema_list;
	struct gsgpu_hw_sema	sema[GSGPU_NUM_SEMA];
};

void gsgpu_sema_free(struct gsgpu_device *adev, struct gsgpu_vm *vm);

int gsgpu_hw_sema_mgr_init(struct gsgpu_device *adev);
void gsgpu_hw_sema_mgr_fini(struct gsgpu_device *adev);

#endif /* __GSGPU_HW_SEMA_H__ */
