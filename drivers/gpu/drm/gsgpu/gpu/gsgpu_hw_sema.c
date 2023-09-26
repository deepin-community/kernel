#include <drm/gsgpu_drm.h>
#include "gsgpu.h"
#include "gsgpu_hw_sema.h"


static void gsgpu_sema_reset(struct gsgpu_device *adev, unsigned id)
{
	struct gsgpu_hw_sema_mgr *sema_mgr = &adev->hw_sema_mgr;
	struct gsgpu_hw_sema *sema = &sema_mgr->sema[id];

	mutex_lock(&sema_mgr->lock);
	sema->own = false;
	sema->pasid = 0ULL;
	sema->ctx = 0ULL;
	sema->vm = NULL;
	mutex_unlock(&sema_mgr->lock);
}

static int gsgpu_sema_grab_new(struct gsgpu_device *adev,
			       struct gsgpu_vm *vm,
			       struct drm_gsgpu_hw_sema *drm_sema)
{

	struct gsgpu_hw_sema_mgr *sema_mgr = &adev->hw_sema_mgr;
	struct gsgpu_hw_sema *idle;
	uint32_t ret = 0;

	mutex_lock(&sema_mgr->lock);

	list_for_each_entry(idle, &sema_mgr->sema_list, list) {
		if (idle->own == false)
			goto get_id;
	}

	if (&idle->list == &sema_mgr->sema_list) {
		ret = -ENODATA;
		drm_sema->id = ~0ULL;
		goto error;
	}

get_id:
	if (vm) {
		gsgpu_vm_set_task_info(vm);
		idle->pasid = vm->pasid;
		idle->vm = vm;
	}

	idle->ctx = drm_sema->ctx_id;
	idle->own = true;

	drm_sema->id = (idle - sema_mgr->sema);

error:
	mutex_unlock(&sema_mgr->lock);

	return ret;
}

int gsgpu_hw_sema_op_ioctl(struct drm_device *dev, void *data, struct drm_file *filp)
{
	uint32_t ret = 0;
	uint64_t ops;
	struct gsgpu_device *adev = dev->dev_private;
	struct drm_gsgpu_hw_sema  *drm_sema = data;
	struct gsgpu_fpriv *fpriv = filp->driver_priv;
	struct gsgpu_vm *vm = &fpriv->vm;

	ops = drm_sema->ops;

	switch (ops) {
	case GSGPU_HW_SEMA_GET:
		ret = gsgpu_sema_grab_new(adev, vm, drm_sema);
		break;
	case GSGPU_HW_SEMA_PUT:
		gsgpu_sema_reset(adev, drm_sema->id);
		ret = 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

void gsgpu_sema_free(struct gsgpu_device *adev, struct gsgpu_vm *vm)
{
	struct gsgpu_hw_sema_mgr *sema_mgr = &adev->hw_sema_mgr;
	struct gsgpu_hw_sema *hw_sema;

	list_for_each_entry(hw_sema, &sema_mgr->sema_list, list) {
		if (hw_sema->vm == vm)
			gsgpu_sema_reset(adev, (hw_sema-sema_mgr->sema));
	}
}

int gsgpu_hw_sema_mgr_init(struct gsgpu_device *adev)
{
	unsigned i;

	struct gsgpu_hw_sema_mgr *sema_mgr = &adev->hw_sema_mgr;

	/* TODO Should get from EC*/
	sema_mgr->num_ids = GSGPU_NUM_SEMA;

	mutex_init(&sema_mgr->lock);
	INIT_LIST_HEAD(&sema_mgr->sema_list);

	for (i = 0; i < sema_mgr->num_ids; ++i) {
		gsgpu_sema_reset(adev, i);
		list_add_tail(&sema_mgr->sema[i].list, &sema_mgr->sema_list);
	}

	return 0;
}

void gsgpu_hw_sema_mgr_fini(struct gsgpu_device *adev)
{

	unsigned i;

	struct gsgpu_hw_sema_mgr *sema_mgr = &adev->hw_sema_mgr;

	mutex_destroy(&sema_mgr->lock);
	for (i = 0; i < GSGPU_NUM_SEMA; ++i) {
		gsgpu_sema_reset(adev, i);
	}

}


#if defined(CONFIG_DEBUG_FS)

static int gsgpu_debugfs_sema_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *)m->private;
	struct drm_device *dev = node->minor->dev;
	struct gsgpu_device *adev = dev->dev_private;
	struct gsgpu_hw_sema_mgr *sema_mgr = &adev->hw_sema_mgr;
	struct gsgpu_hw_sema *sema;


	mutex_lock(&sema_mgr->lock);

	rcu_read_lock();
	list_for_each_entry(sema, &sema_mgr->sema_list, list) {

		if (sema->own) {

			if (sema->vm) {
				struct gsgpu_task_info task_info;
				gsgpu_vm_get_task_info(adev, sema->pasid, &task_info);
				seq_printf(m, "pid %8d process:%s \t",
					   task_info.pid,
					   task_info.task_name);
			}

			seq_printf(m, "id %d \tctx_id %d\n",
				   (u32)(sema - sema_mgr->sema), sema->ctx);

		} else
			seq_printf(m, "id %d available\n", (u32)(sema - sema_mgr->sema));

	}

	rcu_read_unlock();

	mutex_unlock(&sema_mgr->lock);

	return 0;
}

static const struct drm_info_list gsgpu_debugfs_sema_list[] = {
	{"gsgpu_hw_sema_info", &gsgpu_debugfs_sema_info, 0, NULL},
};
#endif
int gsgpu_debugfs_sema_init(struct gsgpu_device *adev)
{
#if defined(CONFIG_DEBUG_FS)
	return gsgpu_debugfs_add_files(adev, gsgpu_debugfs_sema_list, 1);
#endif
	return 0;
}
