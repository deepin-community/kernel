#include "gsgpu.h"
#include "gsgpu_trace.h"

#define GSGPU_BO_LIST_MAX_PRIORITY	32u
#define GSGPU_BO_LIST_NUM_BUCKETS	(GSGPU_BO_LIST_MAX_PRIORITY + 1)

static void gsgpu_bo_list_free_rcu(struct rcu_head *rcu)
{
	struct gsgpu_bo_list *list = container_of(rcu, struct gsgpu_bo_list,
						   rhead);

	kvfree(list);
}

static void gsgpu_bo_list_free(struct kref *ref)
{
	struct gsgpu_bo_list *list = container_of(ref, struct gsgpu_bo_list,
						   refcount);
	struct gsgpu_bo_list_entry *e;

	gsgpu_bo_list_for_each_entry(e, list)
		gsgpu_bo_unref(&e->robj);

	call_rcu(&list->rhead, gsgpu_bo_list_free_rcu);
}

int gsgpu_bo_list_create(struct gsgpu_device *adev, struct drm_file *filp,
			  struct drm_gsgpu_bo_list_entry *info,
			  unsigned num_entries, struct gsgpu_bo_list **result)
{
	unsigned last_entry = 0, first_userptr = num_entries;
	struct gsgpu_bo_list_entry *array;
	struct gsgpu_bo_list *list;
	uint64_t total_size = 0;
	size_t size;
	unsigned i;
	int r;

	if (num_entries > (SIZE_MAX - sizeof(struct gsgpu_bo_list))
				/ sizeof(struct gsgpu_bo_list_entry))
		return -EINVAL;

	size = sizeof(struct gsgpu_bo_list);
	size += num_entries * sizeof(struct gsgpu_bo_list_entry);
	list = kvmalloc(size, GFP_KERNEL);
	if (!list)
		return -ENOMEM;

	kref_init(&list->refcount);

	array = gsgpu_bo_list_array_entry(list, 0);
	memset(array, 0, num_entries * sizeof(struct gsgpu_bo_list_entry));

	for (i = 0; i < num_entries; ++i) {
		struct gsgpu_bo_list_entry *entry;
		struct drm_gem_object *gobj;
		struct gsgpu_bo *bo;
		struct mm_struct *usermm;

		gobj = drm_gem_object_lookup(filp, info[i].bo_handle);
		if (!gobj) {
			r = -ENOENT;
			goto error_free;
		}

		bo = gsgpu_bo_ref(gem_to_gsgpu_bo(gobj));
		drm_gem_object_put(gobj);

		usermm = gsgpu_ttm_tt_get_usermm(bo->tbo.ttm);
		if (usermm) {
			if (usermm != current->mm) {
				gsgpu_bo_unref(&bo);
				r = -EPERM;
				goto error_free;
			}
			entry = &array[--first_userptr];
		} else {
			entry = &array[last_entry++];
		}

		entry->robj = bo;
		entry->priority = min(info[i].bo_priority,
				      GSGPU_BO_LIST_MAX_PRIORITY);
		entry->tv.bo = &entry->robj->tbo;

		total_size += gsgpu_bo_size(entry->robj);
		trace_gsgpu_bo_list_set(list, entry->robj);
	}

	list->first_userptr = first_userptr;
	list->num_entries = num_entries;

	trace_gsgpu_cs_bo_status(list->num_entries, total_size);

	*result = list;
	return 0;

error_free:
	while (i--)
		gsgpu_bo_unref(&array[i].robj);
	kvfree(list);
	return r;

}

static void gsgpu_bo_list_destroy(struct gsgpu_fpriv *fpriv, int id)
{
	struct gsgpu_bo_list *list;

	mutex_lock(&fpriv->bo_list_lock);
	list = idr_remove(&fpriv->bo_list_handles, id);
	mutex_unlock(&fpriv->bo_list_lock);
	if (list)
		kref_put(&list->refcount, gsgpu_bo_list_free);
}

int gsgpu_bo_list_get(struct gsgpu_fpriv *fpriv, int id,
		       struct gsgpu_bo_list **result)
{
	rcu_read_lock();
	*result = idr_find(&fpriv->bo_list_handles, id);

	if (*result && kref_get_unless_zero(&(*result)->refcount)) {
		rcu_read_unlock();
		return 0;
	}

	rcu_read_unlock();
	return -ENOENT;
}

void gsgpu_bo_list_get_list(struct gsgpu_bo_list *list,
			     struct list_head *validated)
{
	/* This is based on the bucket sort with O(n) time complexity.
	 * An item with priority "i" is added to bucket[i]. The lists are then
	 * concatenated in descending order.
	 */
	struct list_head bucket[GSGPU_BO_LIST_NUM_BUCKETS];
	struct gsgpu_bo_list_entry *e;
	unsigned i;

	for (i = 0; i < GSGPU_BO_LIST_NUM_BUCKETS; i++)
		INIT_LIST_HEAD(&bucket[i]);

	/* Since buffers which appear sooner in the relocation list are
	 * likely to be used more often than buffers which appear later
	 * in the list, the sort mustn't change the ordering of buffers
	 * with the same priority, i.e. it must be stable.
	 */
	gsgpu_bo_list_for_each_entry(e, list) {
		unsigned priority = e->priority;

		if (!e->robj->parent)
			list_add_tail(&e->tv.head, &bucket[priority]);

		e->user_pages = NULL;
	}

	/* Connect the sorted buckets in the output list. */
	for (i = 0; i < GSGPU_BO_LIST_NUM_BUCKETS; i++)
		list_splice(&bucket[i], validated);
}

void gsgpu_bo_list_put(struct gsgpu_bo_list *list)
{
	kref_put(&list->refcount, gsgpu_bo_list_free);
}

int gsgpu_bo_create_list_entry_array(struct drm_gsgpu_bo_list_in *in,
				      struct drm_gsgpu_bo_list_entry **info_param)
{
	const void __user *uptr = u64_to_user_ptr(in->bo_info_ptr);
	const uint32_t info_size = sizeof(struct drm_gsgpu_bo_list_entry);
	struct drm_gsgpu_bo_list_entry *info;
	int r;

	info = kvmalloc_array(in->bo_number, info_size, GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	/* copy the handle array from userspace to a kernel buffer */
	r = -EFAULT;
	if (likely(info_size == in->bo_info_size)) {
		unsigned long bytes = in->bo_number *
			in->bo_info_size;

		if (copy_from_user(info, uptr, bytes))
			goto error_free;

	} else {
		unsigned long bytes = min(in->bo_info_size, info_size);
		unsigned i;

		memset(info, 0, in->bo_number * info_size);
		for (i = 0; i < in->bo_number; ++i) {
			if (copy_from_user(&info[i], uptr, bytes))
				goto error_free;

			uptr += in->bo_info_size;
		}
	}

	*info_param = info;
	return 0;

error_free:
	kvfree(info);
	return r;
}

int gsgpu_bo_list_ioctl(struct drm_device *dev, void *data,
				struct drm_file *filp)
{
	struct gsgpu_device *adev = dev->dev_private;
	struct gsgpu_fpriv *fpriv = filp->driver_priv;
	union drm_gsgpu_bo_list *args = data;
	uint32_t handle = args->in.list_handle;
	struct drm_gsgpu_bo_list_entry *info = NULL;
	struct gsgpu_bo_list *list, *old;
	int r;

	r = gsgpu_bo_create_list_entry_array(&args->in, &info);
	if (r)
		goto error_free;

	switch (args->in.operation) {
	case GSGPU_BO_LIST_OP_CREATE:
		r = gsgpu_bo_list_create(adev, filp, info, args->in.bo_number,
					  &list);
		if (r)
			goto error_free;

		mutex_lock(&fpriv->bo_list_lock);
		r = idr_alloc(&fpriv->bo_list_handles, list, 1, 0, GFP_KERNEL);
		mutex_unlock(&fpriv->bo_list_lock);
		if (r < 0) {
			gsgpu_bo_list_put(list);
			return r;
		}

		handle = r;
		break;

	case GSGPU_BO_LIST_OP_DESTROY:
		gsgpu_bo_list_destroy(fpriv, handle);
		handle = 0;
		break;

	case GSGPU_BO_LIST_OP_UPDATE:
		r = gsgpu_bo_list_create(adev, filp, info, args->in.bo_number,
					  &list);
		if (r)
			goto error_free;

		mutex_lock(&fpriv->bo_list_lock);
		old = idr_replace(&fpriv->bo_list_handles, list, handle);
		mutex_unlock(&fpriv->bo_list_lock);

		if (IS_ERR(old)) {
			gsgpu_bo_list_put(list);
			r = PTR_ERR(old);
			goto error_free;
		}

		gsgpu_bo_list_put(old);
		break;

	default:
		r = -EINVAL;
		goto error_free;
	}

	memset(args, 0, sizeof(*args));
	args->out.list_handle = handle;
	kvfree(info);

	return 0;

error_free:
	if (info)
		kvfree(info);
	return r;
}
