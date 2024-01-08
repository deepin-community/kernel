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
	int ret = 0, bus_shift = 0;
	u16 seg = root->segment;

	ret = pci_mcfg_lookup(root, &cfg_res, &ecam_ops);
	if (ret < 0) {
		dev_err(dev, "%04x:%pR ECAM region not found\n", seg, bus_res);
		return NULL;
	}

	/**
	 * Do the quirk of bus shift here, since we can not
	 * know the ECAM addr in MCFG table when fill mcfg_quirks
	 */
	bus_shift     = ecam_ops->bus_shift;
	cfg_res.start = root->mcfg_addr + (bus_res->start << bus_shift);
	cfg_res.end   = cfg_res.start + ((resource_size(bus_res)) << bus_shift) - 1;
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

static int pci_acpi_prepare_root_resources(struct acpi_pci_root_info *ci)
{
	int status = 0;
	acpi_status rc;
	unsigned long long mem_space_base = 0;
	struct resource_entry *entry = NULL, *tmp = NULL;
	struct acpi_device *device = ci->bridge;

	/**
	 * Get host bridge resources via _CRS method, the return value
	 * is the num of resource parsed.
	 */
	status = acpi_pci_probe_root_resources(ci);
	if (status > 0) {
		/**
		 * To distinguish between mem and pre_mem, firmware only pass the
		 * lower 32bits of mem via acpi and use vendor specific "MEMH" to
		 * record the upper 32 bits of mem.
		 *
		 * Get the upper 32 bits here.
		 */
		rc = acpi_evaluate_integer(ci->bridge->handle,
				"MEMH", NULL, &mem_space_base);
		if (rc != AE_OK) {
			dev_err(&device->dev, "unable to retrieve MEMH\n");
			return -EEXIST;
		}

		resource_list_for_each_entry_safe(entry, tmp, &ci->resources) {
			if (entry->res->flags & IORESOURCE_MEM) {
				if (!(entry->res->end & 0xFFFFFFFF00000000ULL)) {
					/* Patch the mem resource with upper 32 bits */
					entry->res->start |= (mem_space_base << 32);
					entry->res->end   |= (mem_space_base << 32);
				} else {
					/**
					 * Add PREFETCH and MEM_64 flags for pre_mem,
					 * so that we can distinguish between mem and
					 * pre_mem.
					 */
					entry->res->flags |= IORESOURCE_PREFETCH;
					entry->res->flags |= IORESOURCE_MEM_64;
				}
			}

			dev_dbg(&device->dev,
				"host bridge resource: 0x%llx-0x%llx flags [0x%lx]\n",
				entry->res->start, entry->res->end, entry->res->flags);
		}
		return status;
	}

	/**
	 * If not successfully parse resources, destroy
	 * resources which have been parsed.
	 */
	resource_list_for_each_entry_safe(entry, tmp, &ci->resources) {
		dev_info(&device->dev,
			"host bridge resource(ignored): 0x%llx-0x%llx flags [0x%lx]\n",
			entry->res->start, entry->res->end, entry->res->flags);
		resource_list_destroy_entry(entry);
	}

	return 0;
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
		memcpy(bus->sysdata, pci_ri->cfg, sizeof(struct pci_config_window));
		kfree(pci_ri->cfg);
		kfree(pci_ri);
		kfree(root_ops);
	} else {
		bus = acpi_pci_root_create(root, root_ops, &pci_ri->info, pci_ri->cfg);

		/**
		 * No need to do kfree here, because acpi_pci_root_create will free
		 * mem alloced when it cannot create pci_bus.
		 */
		if (!bus)
			return NULL;

		/* Some quirks for pci controller of Sunway after scanning Root Complex */
		sw64_pci_root_bridge_scan_finish_up(pci_find_host_bridge(bus));

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

int pcibios_root_bridge_prepare(struct pci_host_bridge *bridge)
{
	if (!acpi_disabled) {
		struct pci_config_window *cfg = bridge->sysdata;
		struct acpi_device *adev = to_acpi_device(cfg->parent);
		struct pci_controller *hose = cfg->priv;
		struct device *bus_dev = &bridge->bus->dev;

		ACPI_COMPANION_SET(&bridge->dev, adev);
		set_dev_node(bus_dev, hose->node);

		/* Some quirks for pci controller of Sunway before scanning Root Complex */
		sw64_pci_root_bridge_prepare(bridge);
	}

	return 0;
}

void pcibios_add_bus(struct pci_bus *bus)
{
	acpi_pci_add_bus(bus);
}

void pcibios_remove_bus(struct pci_bus *bus)
{
	acpi_pci_remove_bus(bus);
}
