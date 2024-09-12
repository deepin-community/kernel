// SPDX-License-Identifier: GPL-2.0
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/pci-ecam.h>
#include <linux/acpi.h>
#include <linux/reboot.h>

#include <asm/pci.h>
#include <asm/sw64_init.h>

/*
 * raw_pci_read/write - Platform-specific PCI config space access.
 */
int raw_pci_read(unsigned int domain, unsigned int bus, unsigned int devfn,
			int reg, int len, u32 *val)
{
	struct pci_bus *bus_tmp = pci_find_bus(domain, bus);

	if (bus_tmp)
		return bus_tmp->ops->read(bus_tmp, devfn, reg, len, val);

	return -EINVAL;
}

int raw_pci_write(unsigned int domain, unsigned int bus, unsigned int devfn,
			int reg, int len, u32 val)
{
	struct pci_bus *bus_tmp = pci_find_bus(domain, bus);

	if (bus_tmp)
		return bus_tmp->ops->write(bus_tmp, devfn, reg, len, val);

	return -EINVAL;
}

resource_size_t pcibios_default_alignment(void)
{
	if (is_in_guest())
		return PAGE_SIZE;
	else
		return 0;
}

/**
 * Just declaring that the power-of-ten prefixes are actually the
 * power-of-two ones doesn't make it true :)
 */
#define KB			1024
#define MB			(1024*KB)
#define GB			(1024*MB)

resource_size_t pcibios_align_resource(void *data, const struct resource *res,
		resource_size_t size, resource_size_t align)
{
	struct pci_dev *dev = data;
	struct pci_controller *hose = pci_bus_to_pci_controller(dev->bus);
	unsigned long alignto;
	resource_size_t start = res->start;

	if (res->flags & IORESOURCE_IO) {
		/* Make sure we start at our min on all hoses */
		if (start - hose->io_space->start < PCIBIOS_MIN_IO)
			start = PCIBIOS_MIN_IO + hose->io_space->start;
		/*
		 * Put everything into 0x00-0xff region modulo 0x400
		 */
		if (start & 0x300)
			start = (start + 0x3ff) & ~0x3ff;
	} else if (res->flags & IORESOURCE_MEM) {
		/* Make sure we start at our min on all hoses */
		if (start - hose->mem_space->start < PCIBIOS_MIN_MEM)
			start = PCIBIOS_MIN_MEM + hose->mem_space->start;
		/*
		 * The following holds at least for the Low Cost
		 * SW64 implementation of the PCI interface:
		 *
		 * In sparse memory address space, the first
		 * octant (16MB) of every 128MB segment is
		 * aliased to the very first 16 MB of the
		 * address space (i.e., it aliases the ISA
		 * memory address space).  Thus, we try to
		 * avoid allocating PCI devices in that range.
		 * Can be allocated in 2nd-7th octant only.
		 * Devices that need more than 112MB of
		 * address space must be accessed through
		 * dense memory space only!
		 */

		/* Align to multiple of size of minimum base.  */
		alignto = max_t(resource_size_t, 0x1000UL, align);
		start = ALIGN(start, alignto);
		if (hose->sparse_mem_base && size <= 7 * 16*MB) {
			if (((start / (16*MB)) & 0x7) == 0) {
				start &= ~(128*MB - 1);
				start += 16*MB;
				start  = ALIGN(start, alignto);
			}
			if (start/(128*MB) != (start + size - 1)/(128*MB)) {
				start &= ~(128*MB - 1);
				start += (128 + 16)*MB;
				start  = ALIGN(start, alignto);
			}
		}
	}

	return start;
}

#undef KB
#undef MB
#undef GB

char *pcibios_setup(char *str)
{
	return str;
}

void pcibios_fixup_bus(struct pci_bus *bus)
{
	/* Propagate hose info into the subordinate devices.  */
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);
	struct pci_dev *dev = bus->self;

	if (!dev) {
		bus->resource[0] = hose->io_space;
		bus->resource[1] = hose->mem_space;
		bus->resource[2] = hose->pre_mem_space;
	}
}

/**
 * Provide information on locations of various I/O regions in physical
 * memory.  Do this on a per-card basis so that we choose the right hose.
 */
asmlinkage long
sys_pciconfig_iobase(long which, unsigned long bus, unsigned long dfn)
{
	struct pci_controller *hose;

	hose = bus_num_to_pci_controller(bus);
	if (hose == NULL)
		return -ENODEV;

	switch (which & ~IOBASE_FROM_HOSE) {
	case IOBASE_HOSE:
		return hose->index;
	case IOBASE_SPARSE_MEM:
		return hose->sparse_mem_base;
	case IOBASE_DENSE_MEM:
		return hose->dense_mem_base;
	case IOBASE_SPARSE_IO:
		return hose->sparse_io_base;
	case IOBASE_DENSE_IO:
		return hose->dense_io_base;
	case IOBASE_ROOT_BUS:
		return hose->bus->number;
	}

	return -EOPNOTSUPP;
}

void pci_iounmap(struct pci_dev *dev, void __iomem *addr)
{
}
EXPORT_SYMBOL(pci_iounmap);

void __init reserve_mem_for_pci(void)
{
	int ret;
	unsigned long base = PCI_32BIT_MEMIO;

	ret = add_memmap_region(base, PCI_32BIT_MEMIO_SIZE, memmap_pci);
	if (ret) {
		pr_err("reserved pages for pcie memory space failed\n");
		return;
	}

	pr_info("reserved pages for pcie memory space %lx:%lx\n",
			base >> PAGE_SHIFT,
			(base + PCI_32BIT_MEMIO_SIZE) >> PAGE_SHIFT);
}

const struct dma_map_ops *dma_ops;
EXPORT_SYMBOL(dma_ops);

/* Quirks */
static void quirk_isa_bridge(struct pci_dev *dev)
{
	dev->class = PCI_CLASS_BRIDGE_ISA << 8;
}
DECLARE_PCI_FIXUP_HEADER(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82378,
		quirk_isa_bridge);

/*
 * Early fix up the Root Complex settings
 */
static void fixup_root_complex(struct pci_dev *dev)
{
	struct pci_bus *bus = dev->bus;
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);

	hose->self_busno = hose->busn_space->start;

	if (likely(bus->number == hose->self_busno)) {
		if (IS_ENABLED(CONFIG_HOTPLUG_PCI_PCIE_SUNWAY)) {
			/* Check Root Complex port again */
			dev->is_hotplug_bridge = 1;
			dev->current_state = PCI_D0;
		}

		dev->class &= 0xff;
		dev->class |= PCI_CLASS_BRIDGE_PCI << 8;
	}

	dev->no_msi = 1;
}

DECLARE_PCI_FIXUP_HEADER(PCI_VENDOR_ID_JN, PCI_DEVICE_ID_SW64_ROOT_BRIDGE, fixup_root_complex);

static int setup_bus_dma_cb(struct pci_dev *pdev, void *data)
{
	pdev->dev.bus_dma_limit = DMA_BIT_MASK(32);
	return 0;
}

static void fix_bus_dma_limit(struct pci_dev *dev)
{
	pci_walk_bus(dev->subordinate, setup_bus_dma_cb, NULL);
	pr_info("Set zx200 bus_dma_limit to 32-bit\n");
}
DECLARE_PCI_FIXUP_FINAL(PCI_VENDOR_ID_ZHAOXIN, 0x071f, fix_bus_dma_limit);

#ifdef CONFIG_DCA
static void enable_sw_dca(struct pci_dev *dev)
{
	struct pci_controller *hose = pci_bus_to_pci_controller(dev->bus);
	unsigned long node, rc_index, dca_ctl, dca_conf;
	int i;

	if (dev->class >> 8 != PCI_CLASS_NETWORK_ETHERNET)
		return;

	node = hose->node;
	rc_index = hose->index;

	for (i = 0; i < 256; i++) {
		dca_conf = readq(hose->piu_ior1_base + DEVICEID0 + (i << 7));
		if (dca_conf >> 63)
			continue;
		else {
			dca_conf = (1UL << 63) | (dev->bus->number << 8) | dev->devfn;
			pr_info("dca device index %d, dca_conf = %#lx\n",
					i, dca_conf);
			writeq(dca_conf, (hose->piu_ior1_base + DEVICEID0 + (i << 7)));
			break;
		}
	}

	dca_ctl = readq(hose->piu_ior1_base + DCACONTROL);
	if (dca_ctl & 0x1) {
		dca_ctl = 0x2;
		writeq(dca_ctl, (hose->piu_ior1_base + DCACONTROL));
		pr_info("Node %ld RC %ld enable DCA 1.0\n", node, rc_index);
	}
}
DECLARE_PCI_FIXUP_FINAL(PCI_VENDOR_ID_INTEL, PCI_ANY_ID, enable_sw_dca);
#endif

static int jm585_restart_notify(struct notifier_block *nb, unsigned long action,
		void *data)
{
	struct pci_dev *pdev;
	struct pci_controller *hose;
	int val;

	pdev = pci_get_device(PCI_VENDOR_ID_JMICRON, 0x0585, NULL);
	if (pdev) {
		hose = pci_bus_to_pci_controller(pdev->bus);
		val = readl(hose->rc_config_space_base + RC_PORT_LINK_CTL);
		writel((val | 0x8), (hose->rc_config_space_base + RC_PORT_LINK_CTL));
		writel(val, (hose->rc_config_space_base + RC_PORT_LINK_CTL));
	}

	return NOTIFY_DONE;
}

static void quirk_jm585_restart(struct pci_dev *dev)
{
	static struct notifier_block jm585_restart_nb = {
		.notifier_call = jm585_restart_notify,
		.priority = 128,
	};

	register_restart_handler(&jm585_restart_nb);
}
DECLARE_PCI_FIXUP_FINAL(PCI_VENDOR_ID_JMICRON, 0x0585, quirk_jm585_restart);

/**
 * There are some special aspects to the Root Complex of Sunway:
 * 1. Root Complex config space base addr is different
 *    from EP config space base addr.
 * 2. For the case of multiple Root Complex, different
 *    Root Complex have config space base addr.
 *
 * These means that even if multiple Root Complex share
 * the same segment group number, their bus numbers can
 * still overlap.
 *
 * But due to a Xorg related issue, we can not overlap
 * the bus numbers of multiple Root Complex. So, after
 * scanning the Root Complex, use "last_bus" to record
 * the next bus number of the current maximum used bus
 * number, and use it as the start bus number of the
 * next Root Complex to be scanned.
 *
 * A question: when there is too much RCs, may 256 bus
 * numbers be insufficient?
 */
static unsigned char last_bus;

static void sunway_pci_root_bridge_prepare(struct pci_host_bridge *bridge)
{
	struct pci_controller *hose = NULL;
	struct resource_entry *entry = NULL;
	struct pci_bus *bus = bridge->bus;
	unsigned long flags = 0;
	unsigned int init_busnr = 0;

	hose = pci_bus_to_pci_controller(bus);

	resource_list_for_each_entry(entry, &bridge->windows) {
		flags = entry->res->flags;
		if (flags & IORESOURCE_IO) {
			entry->offset = entry->res->start;
			hose->io_space = entry->res;
		} else if (flags & IORESOURCE_BUS) {
			entry->res->start = last_bus;
			hose->busn_space = entry->res;
		} else if (flags & IORESOURCE_MEM) {
			if (!(flags & IORESOURCE_PREFETCH)) {
				entry->offset = entry->res->start - PCI_32BIT_MEMIO;
				hose->mem_space = entry->res;
			} else
				hose->pre_mem_space = entry->res;
		}
	}

	/**
	 * We scan Root Complex and update bus num in kernel,
	 * not in firmware. Firmware just pass 0x0-0xff via _CRS.
	 *
	 * So, need to update bus num of pci host bridge here.
	 */
	bridge->busnr = last_bus;
	dev_set_name(&bridge->dev, "pci%04x:%02x",
			pci_domain_nr(bus), last_bus);

	/**
	 * At this point, pci_bus has been created and use old
	 * bridge->busnr, so need to update bus->number here.
	 */
	bus->number = last_bus;

	bridge->swizzle_irq = pci_common_swizzle;
	bridge->map_irq = sunway_pci_map_irq;

	init_busnr = (0xff << 16) + ((last_bus + 1) << 8) + (last_bus);
	writel(init_busnr, (hose->rc_config_space_base + RC_PRIMARY_BUS));

	hose->first_busno = last_bus + (is_in_host() ? 1 : 0);

	pci_add_flags(PCI_REASSIGN_ALL_BUS);
}

void sunway_pci_root_bridge_scan_finish(struct pci_host_bridge *bridge)
{
	struct pci_controller *hose = NULL;
	struct pci_bus *bus = NULL;
	unsigned int init_busnr = 0;

	bus = bridge->bus;

	hose = pci_bus_to_pci_controller(bus);
	hose->bus = bus;

	if (is_in_host())
		last_bus = chip_pcie_configure(hose);
	else {
		while (pci_find_bus(pci_domain_nr(bus), last_bus))
			last_bus++;
	}

	hose->last_busno = last_bus;
	hose->busn_space->end = last_bus;

	init_busnr = readl(hose->rc_config_space_base + RC_PRIMARY_BUS);
	init_busnr &= ~(0xff << 16);
	init_busnr |= last_bus << 16;
	writel(init_busnr, (hose->rc_config_space_base + RC_PRIMARY_BUS));

	pci_bus_update_busn_res_end(bus, last_bus);
	last_bus++;

	/**
	 * Root Complex of SW64 does not support ASPM, causing
	 * control field(_OSC) unable to be updated.
	 *
	 * Related logic can be found in "negotiate_os_control".
	 */
	bridge->native_aer = 1;
	bridge->native_pme = 1;

	/**
	 * Since some buggy firmwares may configure invalid bridge bus numbers,
	 * the kernel re-assigns all PCI bus numbers when scan Root Complex.
	 *
	 * However, users may trigger a pci bus rescan in the userspace by the
	 * command below:
	 *
	 * > echo 1 > /sys/bus/pci/rescan
	 *
	 * Unexpected errors may occur on the endpoint devices due to the
	 * re-assign bus numbers of upstream bridges.
	 *
	 * To work around this problem, the flag PCI_REASSIGN_ALL_BUS is set
	 * before scanning Root Complex and cleared after scanning Root Complex.
	 */
	pci_clear_flags(PCI_REASSIGN_ALL_BUS);
}

int pcibios_root_bridge_prepare(struct pci_host_bridge *bridge)
{
	struct pci_config_window *cfg = bridge->sysdata;
	struct acpi_device *adev = NULL;
	struct pci_controller *hose = cfg->priv;
	struct device *bus_dev = &bridge->bus->dev;

	if (sunway_legacy_pci)
		return 0;

	if (!acpi_disabled)
		adev = to_acpi_device(cfg->parent);

	ACPI_COMPANION_SET(&bridge->dev, adev);
	set_dev_node(bus_dev, hose->node);

	/* Some quirks for Sunway PCIe controller before scanning */
	sunway_pci_root_bridge_prepare(bridge);

	return 0;
}

