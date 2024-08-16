// SPDX-License-Identifier: GPL-2.0
#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/init.h>
#include <linux/pci-acpi.h>
#include <linux/pci-ecam.h>

struct pci_root_info {
	struct acpi_pci_root_info info;
	struct pci_config_window *cfg;
};

static void pci_acpi_release_root_info(struct acpi_pci_root_info *ci)
{
	struct pci_root_info *pci_ri;

	pci_ri = container_of(ci, struct pci_root_info, info);
	pci_ecam_free(pci_ri->cfg);
	kfree(ci->ops);
	kfree(pci_ri);
}

int acpi_pci_bus_find_domain_nr(struct pci_bus *bus)
{
	struct pci_config_window *cfg = bus->sysdata;
	struct acpi_device *adev = to_acpi_device(cfg->parent);
	struct acpi_pci_root *root = acpi_driver_data(adev);

	return root->segment;
}

/**
 * Lookup the MCFG table entry corresponding to the current
 * PCI host controller, and set up config space mapping.
 */
static struct pci_config_window *
pci_acpi_setup_ecam_mapping(struct acpi_pci_root *root)
{
	struct device *dev = &root->device->dev;
	struct pci_config_window *cfg = NULL;
	const struct pci_ecam_ops *ecam_ops = NULL;
	struct resource *bus_res = &root->secondary;
	struct resource cfg_res;
	struct acpi_device *adev = NULL;
	resource_size_t bus_res_size;
	int ret = 0, bus_shift = 0;
	u16 seg = root->segment;

	ret = pci_mcfg_lookup(root, &cfg_res, &ecam_ops);
	if (ret < 0) {
		dev_err(dev, "%04x:%pR ECAM region not found\n", seg, bus_res);
		return NULL;
	}

	/**
	 * Do the quirk of bus shift here, since we can not
	 * get the ECAM addr when fill mcfg_quirks.
	 */
	bus_shift     = ecam_ops->bus_shift;
	cfg_res.start = root->mcfg_addr + (bus_res->start << bus_shift);
	bus_res_size  = resource_size(bus_res);
	cfg_res.end   = cfg_res.start + (bus_res_size << bus_shift) - 1;
	cfg_res.flags = IORESOURCE_MEM;

	/**
	 * ECAM area considered as the mem resource of the current
	 * PCI host controller, we'd better record this resource
	 * in ACPI namespace(_CRS).
	 */
	adev = acpi_resource_consumer(&cfg_res);
	if (adev)
		dev_info(dev, "ECAM area %pR reserved by %s\n", &cfg_res,
				dev_name(&adev->dev));
	else
		dev_info(dev, "Note: ECAM area %pR not reserved in ACPI namespace\n",
				&cfg_res);

	cfg = pci_ecam_create(dev, &cfg_res, bus_res, ecam_ops);
	if (IS_ERR(cfg)) {
		dev_err(dev, "%04x:%pR error %ld mapping ECAM\n", seg, bus_res,
				PTR_ERR(cfg));
		return NULL;
	}

	return cfg;
}

static int ep_32bits_memio_base(struct acpi_device *adev, u64 *memh)
{
	int status = 0;
	u64 val;
	const char *prop = "sunway,ep-mem-32-base";
	const char *legacy_prop = "sw64,ep_mem_32_base";

	status = fwnode_property_read_u64(&adev->fwnode, prop, &val);

	/* Fallback to legacy property name and try again */
	if (status)
		status = fwnode_property_read_u64(&adev->fwnode,
			legacy_prop, &val);

	/* This property is necessary */
	if (status) {
		dev_err(&adev->dev, "failed to retrieve %s or %s\n",
				prop, legacy_prop);
		return status;
	}

	*memh = upper_32_bits(val);
	*memh <<= 32;

	return 0;
}

static int pci_acpi_prepare_root_resources(struct acpi_pci_root_info *ci)
{
	int status = 0;
	u64 memh;
	struct resource_entry *entry = NULL, *tmp = NULL;
	struct acpi_device *device = ci->bridge;

	/**
	 * To distinguish between mem and pre_mem, firmware
	 * only pass the lower 32bits of mem via _CRS method.
	 *
	 * Get the upper 32 bits here.
	 */
	status = ep_32bits_memio_base(device, &memh);
	if (status)
		return status;

	/**
	 * Get host bridge resources via _CRS method, the return value
	 * is the num of resource parsed.
	 */
	status = acpi_pci_probe_root_resources(ci);
	if (status <= 0) {
		/**
		 * If not successfully parse resources, destroy
		 * resources which have been parsed.
		 */
		resource_list_for_each_entry_safe(entry, tmp, &ci->resources) {
			dev_info(&device->dev,
				"host bridge resource(ignored): %pR\n",
				entry->res);
			resource_list_destroy_entry(entry);
		}

		return 0;
	}

	resource_list_for_each_entry_safe(entry, tmp, &ci->resources) {
		if (entry->res->flags & IORESOURCE_MEM) {
			if (!upper_32_bits(entry->res->end)) {
				/* Patch mem res with upper 32 bits */
				entry->res->start |= memh;
				entry->res->end   |= memh;
			} else {
				/**
				 * Add PREFETCH and MEM_64 flags for
				 * pre_mem, so that we can distinguish
				 * between mem and pre_mem.
				 */
				entry->res->flags |= IORESOURCE_PREFETCH;
				entry->res->flags |= IORESOURCE_MEM_64;
			}
		}

		dev_dbg(&device->dev,
			"host bridge resource: %pR\n", entry->res);
	}

	return status;
}

/**
 * This function is called from ACPI code and used to
 * setup PCI host controller.
 */
struct pci_bus *pci_acpi_scan_root(struct acpi_pci_root *root)
{
	struct pci_bus *bus = NULL, *child = NULL;
	struct pci_root_info *pci_ri = NULL;
	struct acpi_pci_root_ops *root_ops = NULL;
	int domain = root->segment;
	int busnum = root->secondary.start;

	pci_ri = kzalloc(sizeof(*pci_ri), GFP_KERNEL);
	if (!pci_ri)
		goto out_of_mem_0;

	root_ops = kzalloc(sizeof(*root_ops), GFP_KERNEL);
	if (!root_ops)
		goto out_of_mem_1;

	pci_ri->cfg = pci_acpi_setup_ecam_mapping(root);
	if (!pci_ri->cfg)
		goto setup_ecam_err;

	root_ops->release_info = pci_acpi_release_root_info;
	root_ops->prepare_resources = pci_acpi_prepare_root_resources;
	root_ops->pci_ops = (struct pci_ops *)&pci_ri->cfg->ops->pci_ops;

	bus = pci_find_bus(domain, busnum);
	if (bus) {
		memcpy(bus->sysdata, pci_ri->cfg,
				sizeof(struct pci_config_window));
		kfree(pci_ri->cfg);
		kfree(pci_ri);
		kfree(root_ops);
	} else {
		bus = acpi_pci_root_create(root, root_ops,
				&pci_ri->info, pci_ri->cfg);

		/**
		 * No need to do kfree here, because acpi_pci_root_create
		 * will free mem alloced when it cannot create pci_bus.
		 */
		if (!bus)
			return NULL;

		/* Some quirks for Sunway PCIe controller after scanning */
		sunway_pci_root_bridge_scan_finish(pci_find_host_bridge(bus));

		pci_bus_size_bridges(bus);
		pci_bus_assign_resources(bus);

		list_for_each_entry(child, &bus->children, node)
			pcie_bus_configure_settings(child);
	}

	return bus;

setup_ecam_err:
	kfree(root_ops);
out_of_mem_1:
	kfree(pci_ri);
out_of_mem_0:
	pr_warn("RC [%04x:%02x:] failed (out of memory or setup ecam error)!\n",
			domain, busnum);

	return NULL;
}

void pcibios_add_bus(struct pci_bus *bus)
{
	acpi_pci_add_bus(bus);
}

void pcibios_remove_bus(struct pci_bus *bus)
{
	acpi_pci_remove_bus(bus);
}
