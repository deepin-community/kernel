// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Flash partitions described by the acpi table
 *
 * Author: Wang Hanmo <wanghanmo2242@phytium.com.cn>
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/mtd/mtd.h>
#include <linux/slab.h>
#include <linux/mtd/partitions.h>
#include <linux/property.h>
#include <linux/acpi.h>

static const struct acpi_device_id parse_acpipart_match_table[];

static int parse_acpi_fixed_partitions(struct mtd_info *master,
				  const struct mtd_partition **pparts,
				  struct mtd_part_parser_data *data)
{
	struct mtd_partition *parts;
	struct acpi_device_id *acpi_id;
	const char *partname;
	int nr_parts, i, ret = 0;
	struct acpi_device *adev;
	struct fwnode_handle *child;
	struct fwnode_handle *child_handle;
	bool dedicated = true;
	struct device *dev;

	dev = &master->dev;
	adev = ACPI_COMPANION(&master->dev);

	if (!master->parent) {/*master*/
		device_get_next_child_node(dev, child_handle);
		if (!child_handle) {
			pr_debug("%s: 'partitions' subnode not found on %pOF. Trying to parse direct subnodes as partitions.\n",
				master->name, child_handle);
			dedicated = false;
		}
	}

	acpi_id = acpi_match_device(parse_acpipart_match_table, dev);
	if (dedicated && !acpi_id)
		return 0;

	nr_parts = 0;
	device_for_each_child_node(dev, child_handle) {
		nr_parts++;
	}

	if (nr_parts == 0)
		return 0;
	parts = kcalloc(nr_parts, sizeof(*parts), GFP_KERNEL);
	if (!parts)
		return -ENOMEM;

	i = 0;
	device_for_each_child_node(dev, child_handle) {
		u64 offset, length;
		bool bool_match;

		fwnode_property_read_u64(child_handle, "offset", &offset);
		fwnode_property_read_u64(child_handle, "length", &length);
		if (!offset && !length) {
			if (dedicated) {
				pr_debug("%s: acpipart partition %pOF (%pOF) missing reg property.\n",
					 master->name, child_handle,
					 dev->fwnode);
				goto acpipart_fail;
			} else {
				nr_parts--;
				continue;
			}
		}

		parts[i].offset = offset;
		parts[i].size = length;
		parts[i].fwnode = child_handle;
		if (!fwnode_property_read_string(child_handle, "label", &partname))
			parts[i].name = partname;
		bool_match = fwnode_property_read_bool(child_handle, "read-only");
		if (bool_match)
			parts[i].mask_flags |= MTD_WRITEABLE;
		bool_match = fwnode_property_read_bool(child_handle, "lock");
		if (bool_match)
			parts[i].mask_flags |= MTD_POWERUP_LOCK;
		bool_match = fwnode_property_read_bool(child_handle, "slc-mode");
		if (bool_match)
			parts[i].mask_flags |= MTD_SLC_ON_MLC_EMULATION;
		i++;
	}

	if (!nr_parts)
		goto acpipart_none;

	*pparts = parts;
	ret = nr_parts;
	return ret;

acpipart_fail:
	pr_err("%s: error parsing acpipart partition %pOF (%pOF)\n",
	       master->name, child_handle, dev->fwnode);
	ret = -EINVAL;
acpipart_none:
	kfree(parts);
	return ret;
}

static const struct acpi_device_id parse_acpipart_match_table[] = {
	/* Generic */
	{ "acpi-fixed-partitions", 0 },
	/* Customized */
	{},
};

MODULE_DEVICE_TABLE(acpi, parse_acpipart_match_table);

static struct mtd_part_parser acpipart_parser = {
	.parse_fn = parse_acpi_fixed_partitions,
	.name = "acpi-fixed-partitions",
	.acpi_match_table = ACPI_PTR(parse_acpipart_match_table),
};

static int __init acpipart_parser_init(void)
{
	register_mtd_parser(&acpipart_parser);
	return 0;
}

static void __exit acpipart_parser_exit(void)
{
	deregister_mtd_parser(&acpipart_parser);
}

module_init(acpipart_parser_init);
module_exit(acpipart_parser_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Parser for MTD partitioning information in acpi table");
MODULE_AUTHOR("wanghanmo <wanghanmo2242@cpu.ac>");
MODULE_ALIAS("acpi-fixed-partitions");
