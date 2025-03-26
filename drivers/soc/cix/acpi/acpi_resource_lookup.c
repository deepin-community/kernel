// SPDX-License-Identifier: GPL-2.0
/*
 *Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/acpi.h>
#include <linux/cma.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/memblock.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/version.h>

#include <linux/../../mm/cma.h>

extern int dma_declare_coherent_memory(struct device *dev,
		phys_addr_t phys_addr, dma_addr_t device_addr, size_t size);

static struct acpi_device *acpi_obj_path_to_adev(union acpi_object *obj)
{
	struct acpi_device *adev = NULL;
	char *path;
	acpi_handle handle;
	acpi_status status;

	if (!obj || !obj->string.length)
		return NULL;

	path = obj->string.pointer;
	status = acpi_get_handle(NULL, path, &handle);
	if (ACPI_FAILURE(status))
		return NULL;

	adev = acpi_fetch_acpi_dev(handle);

	return adev;
}

static struct device *acpi_obj_str_platform_device(const union acpi_object *obj)
{
	struct device *dev;
	struct acpi_device *adev;

	adev = acpi_obj_path_to_adev((union acpi_object *)obj);
	if (!adev)
		return NULL;

	dev = bus_find_device_by_acpi_dev(&platform_bus_type, adev);
	if (!dev)
		return NULL;

	return dev;
}

static const char *acpi_obj_str_to_devname(const union acpi_object *obj)
{
	struct device *dev;

	dev = acpi_obj_str_platform_device(obj);
	if (!dev)
		return NULL;

	return dev_name(dev);
}

static struct device *acpi_obj_ref_platform_device(const union acpi_object *obj)
{
	struct device *dev;
	struct acpi_device *adev;

	adev = acpi_fetch_acpi_dev(obj->reference.handle);
	if (!adev)
		return NULL;

	dev = bus_find_device_by_acpi_dev(&platform_bus_type, adev);
	if (!dev)
		return NULL;

	return dev;
}

static const char *acpi_obj_ref_to_devname(const union acpi_object *obj)
{
	struct device *dev;

	dev = acpi_obj_ref_platform_device(obj);
	if (!dev)
		return NULL;

	return dev_name(dev);
}

static struct device *acpi_obj_to_platform_device(const union acpi_object *obj)
{
	if (!obj)
		return NULL;

	if (obj->type == ACPI_TYPE_LOCAL_REFERENCE)
		return acpi_obj_ref_platform_device(obj);
	else if (obj->type == ACPI_TYPE_STRING)
		return acpi_obj_str_platform_device(obj);
	else
		return NULL;
}

static const char *acpi_obj_to_devname(const union acpi_object *obj)
{
	if (!obj)
		return NULL;

	if (obj->type == ACPI_TYPE_LOCAL_REFERENCE)
		return acpi_obj_ref_to_devname(obj);
	else if (obj->type == ACPI_TYPE_STRING)
		return acpi_obj_str_to_devname(obj);
	else
		return NULL;
}

static int reset_lookup_handle(const union acpi_object *obj, void *data)
{
	struct device *dev = data;
	const union acpi_object *child, *el;
	struct reset_control_lookup *lookup;
	int i, pcnt, count = 0;

	for (i = 0; i < obj->package.count; i++) {
		child = &obj->package.elements[i];
		pcnt = child->package.count;

		/* package: <provider,ref>, <index>, <user,ref>, [<con-id>] */
		if (pcnt < 3)
			continue;

		el = child->package.elements;

		if ((el[0].type != ACPI_TYPE_LOCAL_REFERENCE
			    && el[0].type != ACPI_TYPE_STRING)
		    || (el[1].type != ACPI_TYPE_INTEGER)
		    || (el[2].type != ACPI_TYPE_LOCAL_REFERENCE
			    && el[2].type != ACPI_TYPE_STRING)
		    || (pcnt > 3 && (el[3].type != ACPI_TYPE_STRING)))
			continue;

		lookup = devm_kzalloc(dev, sizeof(*lookup), GFP_KERNEL);
		if (!lookup)
			return -ENOMEM;
		INIT_LIST_HEAD(&lookup->list);
		lookup->provider = acpi_obj_to_devname(&el[0]);
		lookup->index = el[1].integer.value;
		lookup->dev_id = acpi_obj_to_devname(&el[2]);
		lookup->con_id = pcnt > 3 ?
			devm_kstrdup(dev, el[3].string.pointer, GFP_KERNEL)
			: NULL;

		if (!lookup->provider || (!lookup->dev_id && !lookup->con_id))
			continue;

		reset_controller_add_lookup(lookup, 1);

		dev_dbg(dev, "rst lookup: prov[%s] idx[%d] dev[%s] con[%s]\n",
				lookup->provider, lookup->index,
				lookup->dev_id, lookup->con_id);
		count++;
	}

	if (count != obj->package.count)
		dev_err(dev, "reset lookup handle done %d/%d",
				count, obj->package.count);

	return count;
}

static int rmem_dev_set_dma(struct device *dev, phys_addr_t base, size_t size)
{
	if (!dev || dev->dma_mem)
		return -EINVAL;

	if (!memblock_is_region_memory(base, size)
		|| memblock_is_map_memory(base))
		return -EFAULT;

	return dma_declare_coherent_memory(dev, base, base, size);
}

static int reserved_mem_lookup_handle(const union acpi_object *obj, void *data)
{
	struct device *rdev, *dev = data;
	const union acpi_object *child, *el;
	int i, pcnt, count = 0, ret;
	phys_addr_t base, size;

	for (i = 0; i < obj->package.count; i++) {
		child = &obj->package.elements[i];
		pcnt = child->package.count;

		/* package: <base>, <size>, <type,string>, <dev,ref> */
		if (pcnt != 4)
			continue;

		el = child->package.elements;

		if ((el[0].type != ACPI_TYPE_INTEGER)
		    || (el[1].type != ACPI_TYPE_INTEGER)
		    || (el[2].type != ACPI_TYPE_STRING)
		    || (el[3].type != ACPI_TYPE_LOCAL_REFERENCE
			    && el[3].type != ACPI_TYPE_STRING))
			continue;

		base = el[0].integer.value;
		size = el[1].integer.value;

		rdev = acpi_obj_to_platform_device(&el[3]);
		if (!rdev)
			continue;

		if (!strcmp(el[2].string.pointer, "no-map"))
			ret = rmem_dev_set_dma(rdev, base, (size_t)size);
		else
			ret = -EFAULT;
		if (ret)
			dev_err(dev, "rsv mem[%d] err: 0x%llX-0x%llX[%s]->[%s]",
					i, base, size, el[2].string.pointer,
					dev_name(rdev));

		/*
		 * To make it simple, there is no duplicates check, and the
		 * dma memory won't be released in the device's life.
		 */

		dev_dbg(dev, "rsv mem [%d][%s]: 0x%llX-0x%llX[%s]->[%s]",
				i, ret ? "fail" : "ok", base, size,
				el[2].string.pointer, dev_name(rdev));

		if (!ret)
			count++;
	}
	if (count != obj->package.count)
		dev_err(dev, "reserved memory lookup handle done %d/%d",
				count, obj->package.count);

	return count;
}

static int resource_name_lookup_handle(const union acpi_object *obj, void *data)
{
	struct device *rdev, *dev = data;
	const union acpi_object *child, *el;
	int i, pcnt, count = 0, index;
	unsigned int type;
	struct resource *res;
	const char *name;

	for (i = 0; i < obj->package.count; i++) {
		child = &obj->package.elements[i];
		pcnt = child->package.count;

		/* package: <rdev>, <type>, <index>, <name> */
		if (pcnt != 4)
			continue;

		el = child->package.elements;

		if ((el[0].type != ACPI_TYPE_LOCAL_REFERENCE
			    && el[0].type != ACPI_TYPE_STRING)
		    || (el[1].type != ACPI_TYPE_INTEGER)
		    || (el[2].type != ACPI_TYPE_INTEGER)
		    || (el[3].type != ACPI_TYPE_STRING))
			continue;

		rdev = acpi_obj_to_platform_device(&el[0]);
		type = el[1].integer.value;
		index = el[2].integer.value;
		name = devm_kstrdup(dev, el[3].string.pointer, GFP_KERNEL);
		if (!rdev || !name)
			continue;

		res = platform_get_resource(to_platform_device(rdev),
					type, index);
		if (!res)
			continue;
		res->name = name;

		if ((res->flags & IORESOURCE_IRQ)
				&& (res->flags & IORESOURCE_DISABLED))
			acpi_irq_get(ACPI_HANDLE(rdev), index, res);

		dev_dbg(dev, "resource set name [%s][0x%x][%d]->[%s]",
				dev_name(rdev), type, index, name);

		count++;
	}

	if (count != obj->package.count)
		dev_err(dev, "resource name lookup handle done %d/%d",
				count, obj->package.count);

	return count;
}

static int device_link_lookup_handle(const union acpi_object *obj, void *data)
{
	struct device *consumer, *supplier, *dev = data;
	const union acpi_object *child, *el;
	int i, pcnt, count = 0;
	u32 flags;

	for (i = 0; i < obj->package.count; i++) {
		child = &obj->package.elements[i];
		pcnt = child->package.count;

		/* package: <supplier>, <consumer>, <flags> */
		if (pcnt != 3)
			continue;

		el = child->package.elements;

		if ((el[0].type != ACPI_TYPE_LOCAL_REFERENCE
			    && el[0].type != ACPI_TYPE_STRING)
		    || (el[1].type != ACPI_TYPE_LOCAL_REFERENCE
			    && el[1].type != ACPI_TYPE_STRING)
		    || (el[2].type != ACPI_TYPE_INTEGER))
			continue;

		supplier = acpi_obj_to_platform_device(&el[0]);
		consumer = acpi_obj_to_platform_device(&el[1]);
		flags = (u32)el[2].integer.value;

		if (!supplier || !consumer)
			continue;

		device_link_add(consumer, supplier, flags);

		dev_dbg(dev, "device link [%s]->[%s], flags[%u]",
				dev_name(consumer), dev_name(supplier), flags);

		count++;
	}

	if (count != obj->package.count)
		dev_err(dev, "device link lookup handle done %d/%d",
				count, obj->package.count);

	return count;
}

static int acpi_res_lookup_handle(acpi_handle handle, const char *name,
		int (*func)(const union acpi_object *obj, void *data),
		void *data)
{
	struct acpi_buffer output = {ACPI_ALLOCATE_BUFFER, NULL};
	int ret;
	acpi_status status;

	if (!name || !func)
		return -EINVAL;

	status = acpi_evaluate_object_typed(handle,
			(acpi_string)name, NULL, &output, ACPI_TYPE_PACKAGE);
	if (ACPI_FAILURE(status))
		return -ENODEV;

	ret = func((const union acpi_object *)output.pointer, data);
	kfree(output.pointer);

	return ret;
}

static int acpi_resource_lookup_device(acpi_handle handle, void *data)
{
	struct device *dev = data;
	int ret;

	/* reset lookup */
	ret = acpi_res_lookup_handle(handle,
				"RSTL", reset_lookup_handle, dev);
	if (ret < 0 && ret != -ENODEV)
		dev_err(dev, "reset lookup handle fail");

	/* reserved memory lookup */
	ret = acpi_res_lookup_handle(handle,
				"RSVL", reserved_mem_lookup_handle, dev);
	if (ret < 0 && ret != -ENODEV)
		dev_err(dev, "reserved memmory lookup handle fail");

	/* irq name lookup */
	ret = acpi_res_lookup_handle(handle,
				"IRQL", resource_name_lookup_handle, dev);
	if (ret < 0 && ret != -ENODEV)
		dev_err(dev, "irq name lookup handle fail");

	/* resource name lookup */
	ret = acpi_res_lookup_handle(handle,
				"RSNL", resource_name_lookup_handle, dev);
	if (ret < 0 && ret != -ENODEV)
		dev_err(dev, "resource name lookup handle fail");

	/* device link lookup */
	ret = acpi_res_lookup_handle(handle,
				"DLKL", device_link_lookup_handle, dev);
	if (ret < 0 && ret != -ENODEV)
		dev_err(dev, "device link lookup handle fail");

	/*
	 * Since some resources maybe disabled in some platform, and the
	 * handles in the lookup process with the hardware maybe fail.
	 * But other handles witch bind some resources to the device driver
	 * by using "devm_xxx" functions is still needed by other drivers.
	 *
	 * So just print error message instead of return error value to the
	 * "probe" function, witch cause the resoruces release in the device
	 * driver.
	 */

	return 0;
}

static acpi_status acpi_bus_res_scan(acpi_handle handle, u32 level,
					void *context, void **ret_p)
{
	struct device *dev = context;
	acpi_object_type acpi_type;

	if (!dev)
		return AE_OK;

	if (ACPI_FAILURE(acpi_get_type(handle, &acpi_type)))
		return AE_OK;

	if (acpi_type != ACPI_TYPE_DEVICE)
		return AE_OK;

	acpi_resource_lookup_device(handle, dev);

	return AE_OK;
}

static int acpi_resource_lookup_probe(struct platform_device *pdev)
{
	acpi_status status;

	status = acpi_walk_namespace(ACPI_TYPE_ANY,
				ACPI_ROOT_OBJECT, ACPI_UINT32_MAX,
				acpi_bus_res_scan, NULL, &pdev->dev, NULL);

	/* do not ensure every resource since differnt configs */
	return 0;
}

static const struct acpi_device_id acpi_resource_lookup_match[] = {
	{ "CIXA1019" },
	{},
};
MODULE_DEVICE_TABLE(acpi, acpi_resource_lookup_match);

static struct platform_driver acpi_resource_lookup_driver = {
	.driver = {
		.name = "acpi_resource_lookup",
		.acpi_match_table = ACPI_PTR(acpi_resource_lookup_match),
	},
	.probe = acpi_resource_lookup_probe,
};

static int __init acpi_resource_lookup_init(void)
{
	if (acpi_disabled)
		return -ENODEV;

	return platform_driver_register(&acpi_resource_lookup_driver);
}
subsys_initcall(acpi_resource_lookup_init);

MODULE_AUTHOR("Zichar Zhang <zichar.zhang@cixtech.com>");
MODULE_DESCRIPTION("cix resource tables");
MODULE_LICENSE("GPL");
