// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020-2022 Loongson Technology Corporation Limited
 */
#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/pci-acpi.h>
#include <linux/pci-ecam.h>

#include <asm/pci.h>
#include <asm/numa.h>
#include <asm/loongson.h>

struct pci_root_info {
	struct acpi_pci_root_info common;
	struct pci_config_window *cfg;
};

void pcibios_add_bus(struct pci_bus *bus)
{
	acpi_pci_add_bus(bus);
}

int pcibios_root_bridge_prepare(struct pci_host_bridge *bridge)
{
	struct acpi_device *adev = NULL;
	struct device *bus_dev = &bridge->bus->dev;
	struct pci_config_window *cfg = bridge->bus->sysdata;

	if (!acpi_disabled)
		adev = to_acpi_device(cfg->parent);

	ACPI_COMPANION_SET(&bridge->dev, adev);
	set_dev_node(bus_dev, pa_to_nid(cfg->res.start));

	return 0;
}

int acpi_pci_bus_find_domain_nr(struct pci_bus *bus)
{
	struct pci_config_window *cfg = bus->sysdata;
	struct acpi_device *adev = to_acpi_device(cfg->parent);
	struct acpi_pci_root *root = acpi_driver_data(adev);

	return root->segment;
}

static void acpi_release_root_info(struct acpi_pci_root_info *ci)
{
	struct pci_root_info *info;

	info = container_of(ci, struct pci_root_info, common);
	pci_ecam_free(info->cfg);
	kfree(ci->ops);
	kfree(info);
}

static void arch_pci_root_validate_resources(struct device *dev,
					     struct list_head *resources,
					     unsigned long type)
{
	LIST_HEAD(list);
	struct resource *res1, *res2, *root = NULL;
	struct resource_entry *tmp, *entry, *entry2;

	WARN_ON((type & (IORESOURCE_MEM | IORESOURCE_IO)) == 0);
	root = (type & IORESOURCE_MEM) ? &iomem_resource : &ioport_resource;

	list_splice_init(resources, &list);
	resource_list_for_each_entry_safe(entry, tmp, &list) {
		bool free = false;
		resource_size_t end;

		res1 = entry->res;
		if (!(res1->flags & type))
			goto next;

		/* Exclude non-addressable range or non-addressable portion */
		end = min(res1->end, root->end);
		if (end <= res1->start) {
			dev_info(dev, "host bridge window %pR (ignored, not CPU addressable)\n",
				 res1);
			free = true;
			goto next;
		} else if (res1->end != end) {
			dev_info(dev, "host bridge window %pR ([%#llx-%#llx] ignored, not CPU addressable)\n",
				 res1, (unsigned long long)end + 1,
				 (unsigned long long)res1->end);
			res1->end = end;
		}

		resource_list_for_each_entry(entry2, resources) {
			res2 = entry2->res;
			if (!(res2->flags & type))
				continue;

			/*
			 * I don't like throwing away windows because then
			 * our resources no longer match the ACPI _CRS, but
			 * the kernel resource tree doesn't allow overlaps.
			 */
			if (resource_overlaps(res1, res2)) {
				res2->start = min(res1->start, res2->start);
				res2->end = max(res1->end, res2->end);
				dev_info(dev, "host bridge window expanded to %pR; %pR ignored\n",
					 res2, res1);
				free = true;
				goto next;
			}
		}

next:
		resource_list_del(entry);
		if (free)
			resource_list_free_entry(entry);
		else
			resource_list_add_tail(entry, resources);
	}
}
static void arch_pci_root_remap_iospace(struct fwnode_handle *fwnode,
			struct resource_entry *entry)
{
	struct resource *res = entry->res;
	resource_size_t cpu_addr = res->start;
	resource_size_t pci_addr = cpu_addr - entry->offset;
	resource_size_t length = resource_size(res);
	unsigned long port;

	if (pci_register_io_range(fwnode, cpu_addr, length)) {
		res->start += ISA_IOSIZE;
		cpu_addr = res->start;
		pci_addr = cpu_addr - entry->offset;
		length = resource_size(res);
		if (pci_register_io_range(fwnode, cpu_addr, length))
			goto err;
	}

	port = pci_address_to_pio(cpu_addr);
	if (port == (unsigned long)-1)
		goto err;

	res->start = port;
	res->end = port + length - 1;
	entry->offset = port - pci_addr;

	if (pci_remap_iospace(res, cpu_addr) < 0)
		goto err;

	pr_info("Remapped I/O %pa to %pR\n", &cpu_addr, res);
	return;
err:
	res->flags |= IORESOURCE_DISABLED;
}

static int arch_pci_probe_root_resources(struct acpi_pci_root_info *info)
{
	int ret;
	struct list_head *list = &info->resources;
	struct acpi_device *device = info->bridge;
	struct resource_entry *entry, *tmp;
	unsigned long flags;
	struct resource *res;

	flags = IORESOURCE_IO | IORESOURCE_MEM | IORESOURCE_MEM_8AND16BIT;
	ret = acpi_dev_get_resources(device, list,
				     acpi_dev_filter_resource_type_cb,
				     (void *)flags);
	if (ret < 0)
		dev_warn(&device->dev,
			 "failed to parse _CRS method, error code %d\n", ret);
	else if (ret == 0)
		dev_dbg(&device->dev,
			"no IO and memory resources present in _CRS\n");
	else {
		resource_list_for_each_entry_safe(entry, tmp, list) {
			if (entry->res->flags & IORESOURCE_IO) {
				res = entry->res;
				res->start = PFN_ALIGN(res->start);
				res->end += 1;
				res->end = PFN_ALIGN(res->end);
				res->end -= 1;
				if (!entry->offset) {
					entry->offset = LOONGSON_LIO_BASE;
					res->start |= LOONGSON_LIO_BASE;
					res->end |= LOONGSON_LIO_BASE;
				}
				arch_pci_root_remap_iospace(&device->fwnode,
						entry);
			}
			if (entry->res->flags & IORESOURCE_DISABLED)
				resource_list_destroy_entry(entry);
			else
				entry->res->name = info->name;
		}
		arch_pci_root_validate_resources(&device->dev, list,
						 IORESOURCE_MEM);
		arch_pci_root_validate_resources(&device->dev, list,
						 IORESOURCE_IO);
	}

	return ret;
}

static int acpi_prepare_root_resources(struct acpi_pci_root_info *ci)
{
	int status;
	struct resource_entry *entry, *tmp;
	struct acpi_device *device = ci->bridge;

	status = arch_pci_probe_root_resources(ci);
	if (status > 0) {
		resource_list_for_each_entry_safe(entry, tmp, &ci->resources) {
			if (entry->res->flags & IORESOURCE_MEM) {
				if (!entry->offset) {
					entry->offset = ci->root->mcfg_addr & GENMASK_ULL(63, 40);
					entry->res->start |= entry->offset;
					entry->res->end   |= entry->offset;
				}
			}
		}
		return status;
	}

	resource_list_for_each_entry_safe(entry, tmp, &ci->resources) {
		dev_dbg(&device->dev,
			   "host bridge window %pR (ignored)\n", entry->res);
		resource_list_destroy_entry(entry);
	}

	return 0;
}

/*
 * Create a PCI config space window
 *  - reserve mem region
 *  - alloc struct pci_config_window with space for all mappings
 *  - ioremap the config space
 */
static struct pci_config_window *arch_pci_ecam_create(struct device *dev,
		struct resource *cfgres, struct resource *busr, const struct pci_ecam_ops *ops)
{
	int bsz, bus_range, err;
	struct resource *conflict;
	struct pci_config_window *cfg;

	if (busr->start > busr->end)
		return ERR_PTR(-EINVAL);

	cfg = kzalloc(sizeof(*cfg), GFP_KERNEL);
	if (!cfg)
		return ERR_PTR(-ENOMEM);

	cfg->parent = dev;
	cfg->ops = ops;
	cfg->busr.start = busr->start;
	cfg->busr.end = busr->end;
	cfg->busr.flags = IORESOURCE_BUS;
	bus_range = resource_size(cfgres) >> ops->bus_shift;

	bsz = 1 << ops->bus_shift;

	cfg->res.start = cfgres->start;
	cfg->res.end = cfgres->end;
	cfg->res.flags = IORESOURCE_MEM | IORESOURCE_BUSY;
	cfg->res.name = "PCI ECAM";

	conflict = request_resource_conflict(&iomem_resource, &cfg->res);
	if (conflict) {
		err = -EBUSY;
		dev_err(dev, "can't claim ECAM area %pR: address conflict with %s %pR\n",
			&cfg->res, conflict->name, conflict);
		goto err_exit;
	}

	cfg->win = pci_remap_cfgspace(cfgres->start, bus_range * bsz);
	if (!cfg->win)
		goto err_exit_iomap;

	if (ops->init) {
		err = ops->init(cfg);
		if (err)
			goto err_exit;
	}
	dev_info(dev, "ECAM at %pR for %pR\n", &cfg->res, &cfg->busr);

	return cfg;

err_exit_iomap:
	err = -ENOMEM;
	dev_err(dev, "ECAM ioremap failed\n");
err_exit:
	pci_ecam_free(cfg);
	return ERR_PTR(err);
}

/*
 * Lookup the bus range for the domain in MCFG, and set up config space
 * mapping.
 */
static struct pci_config_window *
pci_acpi_setup_ecam_mapping(struct acpi_pci_root *root)
{
	int ret, bus_shift;
	u16 seg = root->segment;
	struct device *dev = &root->device->dev;
	struct resource cfgres;
	struct resource *bus_res = &root->secondary;
	struct pci_config_window *cfg;
	const struct pci_ecam_ops *ecam_ops;

	ret = pci_mcfg_lookup(root, &cfgres, &ecam_ops);
	if (ret < 0) {
		dev_err(dev, "%04x:%pR ECAM region not found, use default value\n", seg, bus_res);
		ecam_ops = &loongson_pci_ecam_ops;
		root->mcfg_addr = mcfg_addr_init(0);
	}

	bus_shift = ecam_ops->bus_shift ? : 20;

	if (bus_shift == 20)
		cfg = pci_ecam_create(dev, &cfgres, bus_res, ecam_ops);
	else {
		cfgres.start = root->mcfg_addr + (bus_res->start << bus_shift);
		cfgres.end = cfgres.start + (resource_size(bus_res) << bus_shift) - 1;
		cfgres.end |= BIT(28) + (((PCI_CFG_SPACE_EXP_SIZE - 1) & 0xf00) << 16);
		cfgres.flags = IORESOURCE_MEM;
		cfg = arch_pci_ecam_create(dev, &cfgres, bus_res, ecam_ops);
	}

	if (IS_ERR(cfg)) {
		dev_err(dev, "%04x:%pR error %ld mapping ECAM\n", seg, bus_res, PTR_ERR(cfg));
		return NULL;
	}

	return cfg;
}

struct pci_bus *pci_acpi_scan_root(struct acpi_pci_root *root)
{
	struct pci_bus *bus;
	struct pci_root_info *info;
	struct acpi_pci_root_ops *root_ops;
	int domain = root->segment;
	int busnum = root->secondary.start;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		pr_warn("pci_bus %04x:%02x: ignored (out of memory)\n", domain, busnum);
		return NULL;
	}

	root_ops = kzalloc(sizeof(*root_ops), GFP_KERNEL);
	if (!root_ops) {
		kfree(info);
		return NULL;
	}

	info->cfg = pci_acpi_setup_ecam_mapping(root);
	if (!info->cfg) {
		kfree(info);
		kfree(root_ops);
		return NULL;
	}

	root_ops->release_info = acpi_release_root_info;
	root_ops->prepare_resources = acpi_prepare_root_resources;
	root_ops->pci_ops = (struct pci_ops *)&info->cfg->ops->pci_ops;

	bus = pci_find_bus(domain, busnum);
	if (bus) {
		memcpy(bus->sysdata, info->cfg, sizeof(struct pci_config_window));
		kfree(info);
	} else {
		struct pci_bus *child;

		bus = acpi_pci_root_create(root, root_ops,
					   &info->common, info->cfg);
		if (!bus) {
			kfree(info);
			kfree(root_ops);
			return NULL;
		}

		pci_bus_size_bridges(bus);
		pci_bus_assign_resources(bus);
		list_for_each_entry(child, &bus->children, node)
			pcie_bus_configure_settings(child);
	}

	return bus;
}
