// SPDX-License-Identifier: GPL-2.0
#include <linux/pci.h>
#include <linux/pci-acpi.h>
#include <linux/pci-ecam.h>
#include <linux/acpi.h>

#include <asm/sw64_init.h>

void set_devint_wken(int node)
{
	unsigned long val;

	/* enable INTD wakeup */
	val = 0x80;
	sw64_io_write(node, DEVINT_WKEN, val);
	sw64_io_write(node, DEVINTWK_INTEN, val);
}

#ifdef CONFIG_UNCORE_JUNZHANG
void set_adr_int(int node)
{
	sw64_io_write(node, ADR_INT_CONFIG, (CORE0_CID << 16 | 0x3f));
	sw64_io_write(node, ADR_CTL, 0xc);
}
#endif

void set_pcieport_service_irq(struct pci_controller *hose)
{
	if (IS_ENABLED(CONFIG_PCIE_PME))
		writeq(PME_ENABLE_INTD_CORE0, (hose->piu_ior0_base + PMEINTCONFIG));

	if (IS_ENABLED(CONFIG_PCIEAER))
		writeq(AER_ENABLE_INTD_CORE0, (hose->piu_ior0_base + AERERRINTCONFIG));

#ifdef CONFIG_UNCORE_JUNZHANG
	if (IS_ENABLED(CONFIG_HOTPLUG_PCI_PCIE_SUNWAY))
		writeq(HP_ENABLE_INTD_CORE0, (hose->piu_ior0_base + HPINTCONFIG));
#endif
}

int pcibios_enable_device(struct pci_dev *dev, int bars)
{
	struct pci_bus *bus = dev->bus;
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);

	if (!is_guest_or_emul() && unlikely(bus->number == hose->self_busno))
		return 0;
	else
		return pci_enable_resources(dev, bars);
}

int chip_pcie_configure(struct pci_controller *hose)
{
	struct pci_dev *dev;
	struct pci_bus *bus, *top;
	struct list_head *next;
	unsigned int max_read_size, smallest_max_payload;
	int max_payloadsize;
	unsigned long piuconfig0, value;
	unsigned int pcie_caps_offset;
	unsigned int rc_conf_value;
	u16 devctl, new_values;
	bool rc_ari_disabled = false, found = false;
	unsigned char bus_max_num;
	void __iomem *rc_config_space_base;

	rc_config_space_base = hose->rc_config_space_base;
	smallest_max_payload = readl(rc_config_space_base + RC_EXP_DEVCAP);
	smallest_max_payload &= PCI_EXP_DEVCAP_PAYLOAD;
	bus_max_num = hose->busn_space->start;

	top = hose->bus;
	bus = top;
	next = top->devices.next;

	for (;;) {
		if (next == &bus->devices) {
			/* end of this bus, go up or finish */
			if (bus == top)
				break;

			next = bus->self->bus_list.next;
			bus = bus->self->bus;
			continue;
		}
		dev = list_entry(next, struct pci_dev, bus_list);
		if (dev->subordinate) {
			/* this is a pci-pci bridge, do its devices next */
			next = dev->subordinate->devices.next;
			bus = dev->subordinate;
		} else
			next = dev->bus_list.next;

		if (!found) {
			if (pci_is_root_bus(dev->bus)) {
				if (list_empty(&dev->subordinate->devices))
					rc_ari_disabled = true;
			} else {
				if (!pci_ari_enabled(dev->bus)) {
					rc_ari_disabled = true;
					found = true;
				}
			}
		}

		if (bus->busn_res.end > bus_max_num)
			bus_max_num = bus->busn_res.end;

		/* Query device PCIe capability register  */
		pcie_caps_offset = dev->pcie_cap;
		if (pcie_caps_offset == 0)
			continue;
		max_payloadsize = dev->pcie_mpss;
		if (max_payloadsize < smallest_max_payload)
			smallest_max_payload = max_payloadsize;
	}

	if (rc_ari_disabled) {
		rc_conf_value = readl(rc_config_space_base + RC_EXP_DEVCTL2);
		rc_conf_value &= ~PCI_EXP_DEVCTL2_ARI;
		writel(rc_conf_value, (rc_config_space_base + RC_EXP_DEVCTL2));
	} else {
		rc_conf_value = readl(rc_config_space_base + RC_EXP_DEVCTL2);
		rc_conf_value |= PCI_EXP_DEVCTL2_ARI;
		writel(rc_conf_value, (rc_config_space_base + RC_EXP_DEVCTL2));
	}

	rc_conf_value = readl(rc_config_space_base + RC_EXP_DEVCAP);
	rc_conf_value &= PCI_EXP_DEVCAP_PAYLOAD;
	max_payloadsize = rc_conf_value;
	if (max_payloadsize < smallest_max_payload)
		smallest_max_payload = max_payloadsize;

	max_read_size = 0x2;   /* Limit to 512B */
	value = readl(rc_config_space_base + RC_EXP_DEVCTL);
	value &= ~(PCI_EXP_DEVCTL_PAYLOAD | PCI_EXP_DEVCTL_READRQ);
	value |= (max_read_size << 12) | (smallest_max_payload << 5);
	writel(value, (rc_config_space_base + RC_EXP_DEVCTL));
	new_values = (max_read_size << 12) | (smallest_max_payload << 5);

	piuconfig0 = readq(hose->piu_ior0_base + PIUCONFIG0);
	piuconfig0 &= ~(0x7fUL << 9);
	if (smallest_max_payload == 0x2) {
		piuconfig0 |= (0x20UL << 9);
		writeq(piuconfig0, (hose->piu_ior0_base + PIUCONFIG0));
	} else {
		piuconfig0 |= (0x40UL << 9);
		writeq(piuconfig0, (hose->piu_ior0_base + PIUCONFIG0));
	}

	pr_info("Node%ld RC%ld MPSS %luB, MRRS %luB, Piuconfig0 %#lx, ARI %s\n",
			hose->node, hose->index, (1UL << smallest_max_payload) << 7,
			(1UL << max_read_size) << 7, piuconfig0,
			rc_ari_disabled ? "disabled" : "enabled");

	/* Now, set the max_payload_size for all devices to that value. */
	bus = top;
	next = top->devices.next;
	for (;;) {
		if (next == &bus->devices) {
			/* end of this bus, go up or finish */
			if (bus == top)
				break;
			next = bus->self->bus_list.next;
			bus = bus->self->bus;
			continue;
		}
		dev = list_entry(next, struct pci_dev, bus_list);
		if (dev->subordinate) {
			/* this is a pci-pci bridge, do its devices next */
			next = dev->subordinate->devices.next;
			bus = dev->subordinate;
		} else
			next = dev->bus_list.next;

		pcie_caps_offset = dev->pcie_cap;
		if (pcie_caps_offset == 0)
			continue;

		pci_read_config_word(dev,
				pcie_caps_offset + PCI_EXP_DEVCTL, &devctl);
		devctl &= ~(PCI_EXP_DEVCTL_PAYLOAD | PCI_EXP_DEVCTL_READRQ);
		devctl |= new_values;
		pci_write_config_word(dev,
				pcie_caps_offset + PCI_EXP_DEVCTL, devctl);
	}

	return bus_max_num;
}

static int check_pci_linkup(struct pci_controller *hose)
{
	unsigned long rc_debug;

	if (is_guest_or_emul()) {
		if (hose->node == 0 && hose->index == 0)
			return 0;
		else
			return 1;
	}

	rc_debug = readq(hose->piu_ior1_base + RCDEBUGINF1);

	return !((rc_debug & 0x3fful) == 0x111);
}

static void set_rc_piu(struct pci_controller *hose)
{
	unsigned int i __maybe_unused;
	unsigned int value;
	u32 rc_misc_ctrl;
	void __iomem *rc_config_space_base;
	void __iomem *piu_ior0_base;
	void __iomem *piu_ior1_base;

	if (is_guest_or_emul())
		return;

	rc_config_space_base = hose->rc_config_space_base;
	piu_ior0_base = hose->piu_ior0_base;
	piu_ior1_base = hose->piu_ior1_base;

	/* configure RC, set PCI-E root controller */
	writel(0x00100007, (rc_config_space_base + RC_COMMAND));
	writel(0x1f0020, (rc_config_space_base + RC_PORT_LINK_CTL));
	writel(0x2850, (rc_config_space_base + RC_EXP_DEVCTL));
	writel(0x6, (rc_config_space_base + RC_EXP_DEVCTL2));
#ifdef CONFIG_UNCORE_XUELANG
	writel(0x0100, (rc_config_space_base + RC_ORDER_RULE_CTL));
#endif

	/* enable DBI_RO_WR_EN */
	rc_misc_ctrl = readl(rc_config_space_base + RC_MISC_CONTROL_1);
	writel(rc_misc_ctrl | 0x1, (rc_config_space_base + RC_MISC_CONTROL_1));

	/* fix up DEVICE_ID_VENDOR_ID register */
	value = (PCI_DEVICE_ID_SW64_ROOT_BRIDGE << 16) | PCI_VENDOR_ID_JN;
	writel(value, (rc_config_space_base + RC_VENDOR_ID));

	/* set PCI-E root class code */
	value = readl(rc_config_space_base + RC_REVISION_ID);
	writel((PCI_CLASS_BRIDGE_HOST << 16) | value, (rc_config_space_base + RC_REVISION_ID));

	/* disable DBI_RO_WR_EN */
	writel(rc_misc_ctrl, (rc_config_space_base + RC_MISC_CONTROL_1));

	writeq(PIUCONFIG0_INIT_VAL, (piu_ior0_base + PIUCONFIG0));

	writeq(0x2, (piu_ior1_base + PIUCONFIG1));
	writeq(-1, (piu_ior1_base + ERRENABLE));

	/* set DMA offset value PCITODMA_OFFSET */
	writeq(PCITODMA_OFFSET, (piu_ior0_base + EPDMABAR));
	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		writeq(MSIX_MSG_ADDR, (piu_ior0_base + MSIADDR));
#ifdef CONFIG_UNCORE_XUELANG
		for (i = 0; i < 256; i++)
			writeq(0, (piu_ior0_base + MSICONFIG0 + (i << 7)));
#endif
	}
}

static unsigned long get_rc_enable(unsigned long node)
{
	unsigned long rc_enable;

	if (is_guest_or_emul())
		return 1;

	rc_enable = sw64_io_read(node, IO_START);

	return rc_enable;
}

static int map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	struct pci_controller *hose = pci_bus_to_pci_controller(dev->bus);

	if (pci_pcie_type(dev) == PCI_EXP_TYPE_ROOT_PORT)
		return hose->service_irq;
	else
		return hose->int_irq;
}

static void hose_init(struct pci_controller *hose)
{
	unsigned long pci_io_base;

	hose->sparse_mem_base = 0;
	hose->sparse_io_base = 0;
	pci_io_base = IO_BASE | (hose->node << IO_NODE_SHIFT)
			      | PCI_BASE | (hose->index << IO_RC_SHIFT);

	hose->dense_mem_base = pci_io_base;
	hose->dense_io_base = pci_io_base | PCI_LEGACY_IO;
	hose->ep_config_space_base = __va(pci_io_base | PCI_EP_CFG);
	hose->rc_config_space_base = __va(pci_io_base | PCI_RC_CFG);
	hose->piu_ior0_base = __va(MK_PIU_IOR0(hose->node, hose->index));
	hose->piu_ior1_base = __va(MK_PIU_IOR1(hose->node, hose->index));

	hose->mem_space->start = pci_io_base + PCI_32BIT_MEMIO;
	hose->mem_space->end = hose->mem_space->start + PCI_32BIT_MEMIO_SIZE - 1;
	hose->mem_space->name = "pci memory space";
	hose->mem_space->flags = IORESOURCE_MEM;

	if (request_resource(&iomem_resource, hose->mem_space) < 0)
		pr_err("Failed to request MEM on hose %ld\n", hose->index);
	hose->pre_mem_space->start =  pci_io_base | PCI_64BIT_MEMIO;
	hose->pre_mem_space->end = hose->pre_mem_space->start + PCI_64BIT_MEMIO_SIZE - 1;
	hose->pre_mem_space->name = "pci pre mem space";
	hose->pre_mem_space->flags = IORESOURCE_MEM | IORESOURCE_PREFETCH | IORESOURCE_MEM_64;

	if (request_resource(&iomem_resource, hose->pre_mem_space) < 0)
		pr_err("Failed to request 64bit MEM on hose %ld\n",
				hose->index);
	hose->io_space->start = pci_io_base | PCI_LEGACY_IO;
	hose->io_space->end = hose->io_space->start + PCI_LEGACY_IO_SIZE - 1;
	hose->io_space->name = "pci io space";
	hose->io_space->flags = IORESOURCE_IO;

	if (request_resource(&ioport_resource, hose->io_space) < 0)
		pr_err("Failed to request IO on hose %ld\n", hose->index);
	hose->busn_space->name = "PCI busn";
	hose->busn_space->start = 0xff;
	hose->busn_space->end = 0xff;
	hose->busn_space->flags = IORESOURCE_BUS;
	hose->first_busno = hose->self_busno = hose->busn_space->start;
	hose->last_busno  = hose->busn_space->end;

	if (is_in_host()) {
		if (IS_ENABLED(CONFIG_PCI_MSI))
			memset(hose->piu_msiconfig, 0, 256/8);
	}
};

static struct sw64_pci_init_ops chip_pci_init_ops = {
	.map_irq = map_irq,
	.get_rc_enable = get_rc_enable,
	.hose_init = hose_init,
	.set_rc_piu = set_rc_piu,
	.check_pci_linkup = check_pci_linkup,
};

void __init setup_chip_pci_ops(void)
{
	sw64_chip_init->pci_init = chip_pci_init_ops;
}

static struct pci_controller *head, **tail = &head;

static DECLARE_BITMAP(rc_linkup, (MAX_NUMNODES * MAX_NR_RCS_PER_NODE));

void pci_mark_rc_linkup(unsigned long node, unsigned long index)
{
	set_bit(node * MAX_NR_RCS_PER_NODE + index, rc_linkup);
}
EXPORT_SYMBOL(pci_mark_rc_linkup);

void pci_clear_rc_linkup(unsigned long node, unsigned long index)
{
	clear_bit(node * MAX_NR_RCS_PER_NODE + index, rc_linkup);
}
EXPORT_SYMBOL(pci_clear_rc_linkup);

int pci_get_rc_linkup(unsigned long node, unsigned long index)
{
	return test_bit(node * MAX_NR_RCS_PER_NODE + index, rc_linkup);
}
EXPORT_SYMBOL(pci_get_rc_linkup);

/**
 * Link the specified pci controller to list
 */
extern struct pci_controller *hose_head;
static void pci_link_controller(struct pci_controller *hose)
{
	if (unlikely(!hose))
		return;

	*tail = hose;
	tail = &hose->next;

	if (!hose_head)
		hose_head = head;
}

struct pci_controller *bus_num_to_pci_controller(unsigned long bus_num)
{
	struct pci_controller *hose;

	for (hose = head; hose; hose = hose->next) {
		if (bus_num >= hose->first_busno && bus_num <= hose->last_busno)
			return hose;
	}

	return NULL;
}
EXPORT_SYMBOL(bus_num_to_pci_controller);

struct pci_controller *pci_bus_to_pci_controller(const struct pci_bus *bus)
{
	struct pci_config_window *cfg = NULL;

	if (unlikely(!bus))
		return NULL;

	if (acpi_disabled)
		return (struct pci_controller *)(bus->sysdata);

	cfg = (struct pci_config_window *)bus->sysdata;
	return (struct pci_controller *)(cfg->priv);
}
EXPORT_SYMBOL(pci_bus_to_pci_controller);

/**
 *  PCIe Root Complex read config space operations
 */
static int sw64_pcie_read_rc_cfg(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 *val)
{
	u32 data;
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);
	void __iomem *cfg_iobase = hose->rc_config_space_base;

	if (IS_ENABLED(CONFIG_PCI_DEBUG))
		pr_debug("rc read addr:%px bus %d, devfn %#x, where %#x size=%d\t",
				cfg_iobase + ((where & ~3) << 5),
				bus->number,
				devfn, where, size);

	if ((uintptr_t)where & (size - 1)) {
		*val = 0;
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}

	/**
	 * Workaround for sw6a chipset due to only support scan with devfn = 0,
	 * while sw6b does not have this limit.
	 */
	if (unlikely(devfn > 0)) {
		*val = ~0;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	data = readl(cfg_iobase + ((where & ~3) << 5));

	switch (size) {
	case 1:
		*val = (data >> (8 * (where & 0x3))) & 0xff;
		break;
	case 2:
		*val = (data >> (8 * (where & 0x2))) & 0xffff;
		break;
	default:
		*val = data;
		break;
	}

	if (IS_ENABLED(CONFIG_PCI_DEBUG))
		pr_debug("*val %#x\n ", *val);

	return PCIBIOS_SUCCESSFUL;
}

/**
 * PCIe Root Complex write config space operations
 */
static int sw64_pcie_write_rc_cfg(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 val)
{
	u32 data;
	u32 shift = 8 * (where & 3);
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);
	void __iomem *cfg_iobase = (void *)hose->rc_config_space_base;

	if ((uintptr_t)where & (size - 1))
		return PCIBIOS_BAD_REGISTER_NUMBER;

	switch (size) {
	case 1:
		data = readl(cfg_iobase + ((where & ~3) << 5));
		data &= ~(0xff << shift);
		data |= (val & 0xff) << shift;
		break;
	case 2:
		data = readl(cfg_iobase + ((where & ~3) << 5));
		data &= ~(0xffff << shift);
		data |= (val & 0xffff) << shift;
		break;
	default:
		data = val;
		break;
	}

	if (IS_ENABLED(CONFIG_PCI_DEBUG))
		pr_debug("rc write addr:%px bus %d, devfn %#x, where %#x *val %#x size %d\n",
				cfg_iobase + ((where & ~3) << 5),
				bus->number,
				devfn, where, val, size);

	writel(data, cfg_iobase + ((where & ~3) << 5));

	return PCIBIOS_SUCCESSFUL;
}

/**
 * sw64_pcie_valid_device - check if a valid device is present
 *                          on bus
 *
 * @bus  : PCI bus structure
 * @devfn: device/function
 *
 * @return: 'true' on success and 'false' if invalid device is found
 */
static bool sw64_pcie_valid_device(struct pci_bus *bus, unsigned int devfn)
{
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);

	if (is_in_host()) {
		/* Only one device down on each root complex */
		if (bus->number == hose->self_busno && devfn > 0)
			return false;
	}

	return true;
}

/**
 * sw64_pcie_config_read - read val from config space of
 *                         PCI host controller or device
 *
 * @bus  : PCI bus structure
 * @devfn: device/function
 * @where: offset from base
 * @size : size of val
 * @val[out]: the value read from PCI host controller or device
 *
 * @return: Whether read operation success
 */
int sw64_pcie_config_read(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 *val)
{
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);
	int ret = PCIBIOS_DEVICE_NOT_FOUND;

	if (is_guest_or_emul())
		return pci_generic_config_read(bus, devfn, where, size, val);

	hose->self_busno = hose->busn_space->start;

	if (unlikely(bus->number == hose->self_busno)) {
		ret = sw64_pcie_read_rc_cfg(bus, devfn, where, size, val);
	} else {
		if (pci_get_rc_linkup(hose->node, hose->index))
			ret = pci_generic_config_read(bus, devfn, where, size, val);
		else
			return ret;
	}
	return ret;
}
EXPORT_SYMBOL(sw64_pcie_config_read);

/**
 * sw64_pcie_config_write - write val to config space of PCI
 *                          host controller or device
 *
 * @bus  : PCI bus structure
 * @devfn: device/function
 * @where: offset from base
 * @size : size of val
 * @val  : the value write to PCI host controller or device
 *
 * @return: Whether write operation success
 */
int sw64_pcie_config_write(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 val)
{
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);

	if (is_guest_or_emul())
		return pci_generic_config_write(bus, devfn, where, size, val);

	hose->self_busno = hose->busn_space->start;

	if (unlikely(bus->number == hose->self_busno))
		return sw64_pcie_write_rc_cfg(bus, devfn, where, size, val);
	else
		return pci_generic_config_write(bus, devfn, where, size, val);
}
EXPORT_SYMBOL(sw64_pcie_config_write);

/**
 * sw64_pcie_map_bus - get configuration base address
 * @bus  : PCI bus structure
 * @devfn: device/function
 * @where: offset from base
 *
 * @return: base address of the configuration space needed to be
 * accessed.
 */
void __iomem *sw64_pcie_map_bus(struct pci_bus *bus,
		unsigned int devfn, int where)
{
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);
	void __iomem *cfg_iobase;
	unsigned long relbus;

	if (!sw64_pcie_valid_device(bus, devfn))
		return NULL;

	/**
	 * ECAM of Sunway PCI host controller is slightly
	 * different from the standrad:
	 * [31:24]: bus number
	 * [23:19]: device number
	 * [18:16]: function number
	 * [15:12]: reserved
	 * [11:8] : extended config space registers
	 * [7:2]  : legacy config space registers
	 */
	relbus = (bus->number << 24) | (devfn << 16) | where;

	cfg_iobase = hose->ep_config_space_base + relbus;

	if (IS_ENABLED(CONFIG_PCI_DEBUG))
		pr_debug("addr:%px bus %d, devfn %d, where %d\n",
				cfg_iobase, bus->number, devfn, where);
	return cfg_iobase;
}
EXPORT_SYMBOL(sw64_pcie_map_bus);

int sw64_pci_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	return map_irq(dev, slot, pin);
}

#ifdef CONFIG_ACPI

enum pci_props {
	PROP_RC_CONFIG_BASE = 0,
	PROP_EP_CONFIG_BASE,
	PROP_EP_MEM_32_BASE,
	PROP_EP_MEM_64_BASE,
	PROP_EP_IO_BASE,
	PROP_PIU_IOR0_BASE,
	PROP_PIU_IOR1_BASE,
	PROP_RC_INDEX,
	PROP_PCIE_IO_BASE,
	PROP_NUM
};

const char *prop_names[PROP_NUM] = {
	"sw64,rc_config_base",
	"sw64,ep_config_base",
	"sw64,ep_mem_32_base",
	"sw64,ep_mem_64_base",
	"sw64,ep_io_base",
	"sw64,piu_ior0_base",
	"sw64,piu_ior1_base",
	"sw64,rc_index",
	"sw64,pcie_io_base"
};

static int sw64_pci_prepare_controller(struct pci_controller *hose,
		struct fwnode_handle *fwnode)
{
	u64 props[PROP_NUM];
	int i, ret;

	/* Get properties of Root Complex */
	for (i = 0; i < PROP_NUM; ++i) {
		ret = fwnode_property_read_u64_array(fwnode, prop_names[i],
			&props[i], 1);
		if (ret) {
			pr_err("unable to retrieve \"%s\"\n",
					prop_names[i]);
			return ret;
		}
	}

	hose->iommu_enable = false;

	hose->index = props[PROP_RC_INDEX];

	hose->sparse_mem_base = 0;
	hose->sparse_io_base  = 0;
	hose->dense_mem_base  = props[PROP_PCIE_IO_BASE];
	hose->dense_io_base   = props[PROP_EP_IO_BASE];

	hose->rc_config_space_base = __va(props[PROP_RC_CONFIG_BASE]);
	hose->ep_config_space_base = __va(props[PROP_EP_CONFIG_BASE]);
	hose->piu_ior0_base = __va(props[PROP_PIU_IOR0_BASE]);
	hose->piu_ior1_base = __va(props[PROP_PIU_IOR1_BASE]);

	hose->first_busno = 0xff;
	hose->last_busno  = 0xff;
	hose->self_busno  = 0xff;

	hose->need_domain_info = 0;

#if IS_ENABLED(CONFIG_PCI_MSI)
	if (is_in_host())
		memset(hose->piu_msiconfig, 0, 256 / 8); /* 256 bits bitmap */
#endif

	/**
	 * There are two prerequisites for Root Complex
	 * of Sunway to work:
	 * 1. Root Complex enable
	 * 2. Root Complex link up
	 */
	set_rc_piu(hose);
	if (check_pci_linkup(hose)) {
		/**
		 * Root Complex link up failed.
		 * This usually means that no device on the slot.
		 */
		pr_info("<Node [%ld], RC [%ld]>: link down\n",
				hose->node, hose->index);
	} else {
		pci_mark_rc_linkup(hose->node, hose->index);
		pr_info("<Node [%ld], RC [%ld]>: successfully link up\n",
				hose->node, hose->index);
	}

	setup_intx_irqs(hose);

	pci_link_controller(hose);

	return 0;
}

/**
 * Use the info from ACPI to init pci_controller
 */
static int sw64_pci_ecam_init(struct pci_config_window *cfg)
{
	struct pci_controller *hose = NULL;
	struct device *dev = cfg->parent;
	struct acpi_device *adev = to_acpi_device(dev);
	int ret;

	/**
	 * First, check whether Root Complex is enabled.
	 * If Root Complex disabled, there's no need to continue.
	 *
	 * In ACPI namespace, we use _STA method to indicate
	 * whether Root Complex is enabled.
	 *
	 * The _STA has been checked when creating acpi_device.
	 * Double check here to get the latest hardware status.
	 */
	ret = acpi_bus_get_status(adev);
	if (ret) {
		dev_err(dev, "unable to retrieve _STA\n");
		return ret;
	}

	if (!adev->status.present) {
		dev_err(dev, "RC is not enabled\n");
		return -ENODEV;
	}

	hose = kzalloc(sizeof(*hose), GFP_KERNEL);
	if (!hose)
		return -ENOMEM;

	/* Get node from ACPI namespace (_PXM) */
	hose->node = acpi_get_node(adev->handle);
	if (hose->node == NUMA_NO_NODE) {
		kfree(hose);
		dev_err(&adev->dev, "unable to get node ID\n");
		return -EINVAL;
	}

	/* Init pci_controller */
	ret = sw64_pci_prepare_controller(hose, &adev->fwnode);
	if (ret) {
		kfree(hose);
		dev_err(&adev->dev, "failed to init pci controller\n");
		return ret;
	}

	cfg->priv = (void *)hose;

	return 0;
}

const struct pci_ecam_ops sw64_pci_ecam_ops = {
	.bus_shift = 24,
	.init      = sw64_pci_ecam_init,
	.pci_ops   = {
		.map_bus = sw64_pcie_map_bus,
		.read    = sw64_pcie_config_read,
		.write   = sw64_pcie_config_write,
	}
};
#endif
