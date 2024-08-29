// SPDX-License-Identifier: GPL-2.0
#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/syscore_ops.h>

#include <asm/pci.h>
#include <asm/sw64_init.h>
#include <asm/pci_impl.h>

#define OFFSET_DEVINT_WKEN	0x1500UL
#define OFFSET_DEVINTWK_INTEN	0x1600UL

bool sunway_legacy_pci;

/*
 * The PCI controller list.
 */

struct pci_controller *hose_head, **hose_tail = &hose_head;

static int __init
pcibios_init(void)
{
	if (sunway_legacy_pci)
		sw64_init_pci();
	return 0;
}
subsys_initcall(pcibios_init);

void __init pcibios_claim_one_bus(struct pci_bus *b)
{
	struct pci_dev *dev;
	struct pci_bus *child_bus;

	list_for_each_entry(dev, &b->devices, bus_list) {
		int i;

		for (i = 0; i < PCI_NUM_RESOURCES; i++) {
			struct resource *r = &dev->resource[i];

			if (r->parent || !r->start || !r->flags)
				continue;
			if (r->flags & IORESOURCE_PCI_FIXED) {
				if (pci_claim_resource(dev, i) == 0)
					continue;

				pci_claim_bridge_resource(dev, i);
			}
		}
	}

	list_for_each_entry(child_bus, &b->children, node)
		pcibios_claim_one_bus(child_bus);
}

static void __init
pcibios_claim_console_setup(void)
{
	struct pci_bus *b;

	list_for_each_entry(b, &pci_root_buses, node)
		pcibios_claim_one_bus(b);
}

int __weak chip_pcie_configure(struct pci_controller *hose)
{
	return 0;
}

static struct pci_ops sunway_pci_ops = {
	.map_bus = sunway_pci_map_bus,
	.read    = sunway_pci_config_read,
	.write   = sunway_pci_config_write,
};

unsigned char last_bus = PCI0_BUS;
void __init common_init_pci(void)
{
	struct pci_controller *hose;
	struct pci_host_bridge *bridge;
	struct pci_bus *bus;
	unsigned int init_busnr;
	int need_domain_info = 0;
	int ret;
	unsigned long offset;

	/* Scan all of the recorded PCI controllers. */
	hose = hose_head;
	for (hose = hose_head; hose; hose = hose->next) {
		bridge = pci_alloc_host_bridge(0);
		if (!bridge)
			continue;
		hose->busn_space->start = last_bus;
		init_busnr = (0xff << 16) + ((last_bus + 1) << 8) + (last_bus);
		writel(init_busnr, (hose->rc_config_space_base + RC_PRIMARY_BUS));
		offset = hose->mem_space->start - PCI_32BIT_MEMIO;
		if (is_in_host())
			hose->first_busno = last_bus + 1;
		else
			hose->first_busno = last_bus;
		pci_add_resource_offset(&bridge->windows, hose->mem_space, offset);
		pci_add_resource_offset(&bridge->windows, hose->io_space, hose->io_space->start);
		pci_add_resource_offset(&bridge->windows, hose->pre_mem_space, 0);
		pci_add_resource_offset(&bridge->windows, hose->busn_space, 0);
		bridge->dev.parent = NULL;
		bridge->sysdata = hose;
		bridge->busnr = hose->busn_space->start;
		bridge->ops = &sunway_pci_ops;
		bridge->swizzle_irq = pci_common_swizzle;
		bridge->map_irq = sunway_pci_map_irq;

		ret = pci_scan_root_bus_bridge(bridge);
		if (ret) {
			pci_free_host_bridge(bridge);
			continue;
		}

		bus = hose->bus = bridge->bus;
		hose->need_domain_info = need_domain_info;

		if (is_in_host())
			last_bus = chip_pcie_configure(hose);
		else
			while (pci_find_bus(pci_domain_nr(bus), last_bus))
				last_bus++;

		hose->last_busno = hose->busn_space->end = last_bus;
		init_busnr = readl(hose->rc_config_space_base + RC_PRIMARY_BUS);
		init_busnr &= ~(0xff << 16);
		init_busnr |= last_bus << 16;
		writel(init_busnr, (hose->rc_config_space_base + RC_PRIMARY_BUS));
		pci_bus_update_busn_res_end(bus, last_bus);
		last_bus++;
	}

	pcibios_claim_console_setup();

	pr_info("SW arch assign unassigned resources.\n");
	pci_assign_unassigned_resources();

	for (hose = hose_head; hose; hose = hose->next) {
		bus = hose->bus;
		if (bus)
			pci_bus_add_devices(bus);
	}
}

struct pci_controller * __init
alloc_pci_controller(void)
{
	struct pci_controller *hose;

	hose = memblock_alloc(sizeof(*hose), SMP_CACHE_BYTES);

	*hose_tail = hose;
	hose_tail = &hose->next;

	return hose;
}

struct resource * __init
alloc_resource(void)
{
	struct resource *res;

	res = memblock_alloc(sizeof(*res), SMP_CACHE_BYTES);

	return res;
}

static bool rc_linkup[MAX_NUMNODES][MAX_NR_RCS_PER_NODE];

static void __init
sw64_init_host(unsigned long node, unsigned long index)
{
	struct pci_controller *hose;
	int ret = 0;

	hose = alloc_pci_controller();
	if (!hose) {
		pr_warn("alloc NODE %ld RC %ld hose failed\n", node, index);
		return;
	}
	hose->iommu_enable = false;
	hose->io_space = alloc_resource();
	hose->mem_space = alloc_resource();
	hose->pre_mem_space = alloc_resource();
	hose->busn_space = alloc_resource();
	hose->index = index;
	hose->node = node;

	sw64_chip_init->pci_init.hose_init(hose);

	if (sw64_chip_init->pci_init.set_rc_piu)
		sw64_chip_init->pci_init.set_rc_piu(hose);

	ret = sw64_chip_init->pci_init.check_pci_linkup(hose);
	if (ret == 0) {
		/* Root Complex downstream port is link up */
		pci_mark_rc_linkup(hose); // 8-bit per node
		rc_linkup[node][index] = true;
	}
}

static void set_devint_wken(int node)
{
	unsigned long val;
	void __iomem *intpu_base = misc_platform_get_intpu_base(node);

	/* enable INTD wakeup */
	val = 0x80;
	writeq(val, intpu_base + OFFSET_DEVINT_WKEN);
	writeq(val, intpu_base + OFFSET_DEVINTWK_INTEN);
}

static bool __init is_any_rc_linkup_one_node(unsigned long node)
{
	int i;

	for (i = 0; i < MAX_NR_RCS_PER_NODE; ++i) {
		if (rc_linkup[node][i])
			return true;
	}

	return false;
}

static bool __init is_sunway_legacy_pci(void)
{
	if (IS_ENABLED(CONFIG_SUBARCH_C3B))
		return true;

	if (sunway_machine_is_compatible("sunway,chip4"))
		return true;

	if (is_in_host() && sunway_machine_is_compatible("sunway,junzhang"))
		return true;

	return false;
}

void __init sw64_init_arch(void)
{
	if (IS_ENABLED(CONFIG_PCI)) {
		unsigned long node, cpu_num;
		unsigned long rc_enable;
		char id[8], msg[64];
		int i;

		if (!acpi_disabled)
			return;

		if (!is_sunway_legacy_pci())
			return;

		sunway_legacy_pci = true;

		cpu_num = sw64_chip->get_cpu_num();

		for (node = 0; node < cpu_num; node++) {
			if (is_in_host())
				set_devint_wken(node);
		}

		pr_info("SW arch PCI initialize!\n");
		for (node = 0; node < cpu_num; node++) {
			rc_enable = sw64_chip_init->pci_init.get_rc_enable(node);
			if (rc_enable == 0) {
				pr_notice("PCIe is disabled on node %ld\n", node);
				continue;
			}
			for (i = 0; i < MAX_NR_RCS_PER_NODE; i++) {
				if ((rc_enable >> i) & 0x1)
					sw64_init_host(node, i);
			}
			if (is_any_rc_linkup_one_node(node)) {
				memset(msg, 0, 64);
				sprintf(msg, "Node %ld: RC [ ", node);
				for (i = 0; i < MAX_NR_RCS_PER_NODE; i++) {
					if (rc_linkup[node][i]) {
						memset(id, 0, 8);
						sprintf(id, "%d ", i);
						strcat(msg, id);
					}
				}
				strcat(msg, "] link up");
				pr_info("%s\n", msg);
			} else {
				pr_info("Node %ld: no RC link up\n", node);
			}
		}
	}
}

void __weak set_pcieport_service_irq(int node, int index) {}

void __init
sw64_init_pci(void)
{
	pci_add_flags(PCI_REASSIGN_ALL_BUS);
	common_init_pci();
	pci_clear_flags(PCI_REASSIGN_ALL_BUS);
}
