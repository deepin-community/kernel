// SPDX-License-Identifier: GPL-2.0

#include <linux/kconfig.h>
#include <linux/pci.h>
#include <linux/irqchip.h>
#include <linux/init.h>

#include <asm/hw_init.h>
#include <asm/irq_impl.h>
#include <asm/pmc.h>
#include <asm/sw64_init.h>

/**
 * The topology of interrupt controllers of SW64 is as follows:
 *
 * +-----------------------------------------------------------+
 * |                                                           |
 * |                      +-------------+                      |
 * |                      |    Core     |                      |
 * |                      +-------------+                      |
 * |                             |                             |
 * |                 +----------------------+                  |
 * |                 |       SW CINTC       |                  |
 * |                 +----------------------+                  |
 * |                ______|             |______                |
 * |               |                           |               |
 * |          +-----------+             +--------------+       |
 * |          |  SW MSIC  |             |   SW PINTC   |       |
 * |          +-----------+             +--------------+       |
 * |                                           |               |
 * |                                    +--------------+       |
 * |                                    |  SW LPC INTC |       |
 * |                                    +--------------+       |
 * |                                                           |
 * +-----------------------------------------------------------+
 */

#define PREFIX "CINTC: "

struct fwnode_handle *cintc_handle;

static void handle_intx(unsigned int offset)
{
	struct pci_controller *hose;
	unsigned long value;

	hose = hose_head;
	for (hose = hose_head; hose; hose = hose->next) {
		value = read_piu_ior0(hose->node, hose->index, INTACONFIG + (offset << 7));
		if (value >> 63) {
			value = value & (~(1UL << 62));
			write_piu_ior0(hose->node, hose->index, INTACONFIG + (offset << 7), value);
			handle_irq(hose->int_irq);
			value = value | (1UL << 62);
			write_piu_ior0(hose->node, hose->index, INTACONFIG + (offset << 7), value);
		}

		if (IS_ENABLED(CONFIG_PCIE_PME)) {
			value = read_piu_ior0(hose->node, hose->index, PMEINTCONFIG);
			if (value >> 63) {
				handle_irq(hose->service_irq);
				write_piu_ior0(hose->node, hose->index, PMEINTCONFIG, value);
			}
		}

		if (IS_ENABLED(CONFIG_PCIEAER)) {
			value = read_piu_ior0(hose->node, hose->index, AERERRINTCONFIG);
			if (value >> 63) {
				handle_irq(hose->service_irq);
				write_piu_ior0(hose->node, hose->index, AERERRINTCONFIG, value);
			}
		}

		if (hose->iommu_enable) {
			value = read_piu_ior0(hose->node, hose->index, IOMMUEXCPT_STATUS);
			if (value >> 63)
				handle_irq(hose->int_irq);
		}
	}
}

static void handle_device_interrupt(unsigned long irq_info)
{
	unsigned int i;

	if (is_guest_or_emul()) {
		handle_irq(irq_info);
		return;
	}

	for (i = 0; i < 4; i++) {
		if ((irq_info >> i) & 0x1)
			handle_intx(i);
	}
}

/* Performance counter hook.  A module can override this to do something useful. */
static void dummy_perf(unsigned long vector, struct pt_regs *regs)
{
	irq_err_count++;
	pr_crit("Performance counter interrupt!\n");
}

void (*perf_irq)(unsigned long vector, struct pt_regs *regs) = dummy_perf;
EXPORT_SYMBOL(perf_irq);

static void handle_fault_int(void)
{
	int node;
	unsigned long value;

	node = __this_cpu_read(hard_node_id);
	pr_info("enter fault int, si_fault_stat = %#lx\n",
			sw64_io_read(node, SI_FAULT_STAT));
	sw64_io_write(node, SI_FAULT_INT_EN, 0);
	sw64_io_write(node, DLI_RLTD_FAULT_INTEN, 0);
#if defined(CONFIG_UNCORE_XUELANG)
	value = 0;
#elif defined(CONFIG_UNCORE_JUNZHANG)
	value = sw64_io_read(node, FAULT_INT_CONFIG);
	value |= (1 << 8);
#endif
	__io_write_fault_int_en(node, value);
}

static void handle_mt_int(void)
{
	pr_info("enter mt int\n");
}

static void handle_nmi_int(void)
{
	pr_info("enter nmi int\n");
}

static void handle_dev_int(struct pt_regs *regs)
{
	unsigned long config_val, val, stat;
	int node = 0;
	unsigned int hwirq;

	config_val = sw64_io_read(node, DEV_INT_CONFIG);
	val = config_val & (~(1UL << 8));
	sw64_io_write(node, DEV_INT_CONFIG, val);
	stat = sw64_io_read(node, MCU_DVC_INT);

	while (stat) {
		hwirq = ffs(stat) - 1;
		generic_handle_domain_irq(mcu_irq_domain, hwirq);
		stat &= ~(1UL << hwirq);
	}

	sw64_io_write(node, DEV_INT_CONFIG, config_val);
}

asmlinkage void do_entInt(unsigned long type, unsigned long vector,
			  unsigned long irq_arg, struct pt_regs *regs)
{
	struct pt_regs *old_regs;
	extern char __idle_start[], __idle_end[];

	if (is_guest_or_emul()) {
		if ((type & 0xffff) > 15) {
			vector = type;
			if (vector == 16)
				type = INT_INTx;
			else
				type = INT_MSI;
		}
	}

	/* restart idle routine if it is interrupted */
	if (regs->pc > (u64)__idle_start && regs->pc < (u64)__idle_end)
		regs->pc = (u64)__idle_start;

	switch (type & 0xffff) {
	case INT_MSI:
		old_regs = set_irq_regs(regs);
		handle_pci_msi_interrupt(type, vector, irq_arg);
		set_irq_regs(old_regs);
		return;
	case INT_INTx:
		old_regs = set_irq_regs(regs);
		handle_device_interrupt(vector);
		set_irq_regs(old_regs);
		return;

	case INT_IPI:
#ifdef CONFIG_SMP
		handle_ipi(regs);
		return;
#else
		irq_err_count++;
		pr_crit("Interprocessor interrupt? You must be kidding!\n");
#endif
		break;
	case INT_RTC:
		old_regs = set_irq_regs(regs);
		sw64_timer_interrupt();
		set_irq_regs(old_regs);
		return;
	case INT_VT_SERIAL:
		old_regs = set_irq_regs(regs);
		handle_irq(type);
		set_irq_regs(old_regs);
		return;
	case INT_VT_HOTPLUG:
		old_regs = set_irq_regs(regs);
		handle_irq(type);
		set_irq_regs(old_regs);
		return;
	case INT_PC0:
		perf_irq(PMC_PC0, regs);
		return;
	case INT_PC1:
		perf_irq(PMC_PC1, regs);
		return;
	case INT_DEV:
		handle_dev_int(regs);
		return;
	case INT_FAULT:
		old_regs = set_irq_regs(regs);
		handle_fault_int();
		set_irq_regs(old_regs);
		return;
	case INT_MT:
		old_regs = set_irq_regs(regs);
		handle_mt_int();
		set_irq_regs(old_regs);
		return;
	case INT_NMI:
		old_regs = set_irq_regs(regs);
		handle_nmi_int();
		set_irq_regs(old_regs);
		return;
	default:
		pr_crit("Hardware intr	%ld %lx? uh?\n", type, vector);
	}
	pr_crit("PC = %016lx PS = %04lx\n", regs->pc, regs->ps);
}
EXPORT_SYMBOL(do_entInt);

#ifdef CONFIG_ACPI
#define SW_CINTC_FLAG_VIRTUAL  0x4 /* virtual CINTC */

#define is_core_virtual(flags) ((flags) & SW_CINTC_FLAG_VIRTUAL)

struct gsi_domain_map {
	u32 gsi_base;
	u32 gsi_count;
	struct fwnode_handle *handle;
	struct gsi_domain_map *next;
};

static struct gsi_domain_map *gsi_domain_map_list;

int __init sw64_add_gsi_domain_map(u32 gsi_base, u32 gsi_count,
		struct fwnode_handle *handle)
{
	struct gsi_domain_map *map;

	if (WARN_ON(!handle))
		return -EINVAL;

	map = kzalloc(sizeof(struct gsi_domain_map), GFP_KERNEL);
	if (!map)
		return -ENOMEM;

	map->gsi_base = gsi_base;
	map->gsi_count = gsi_count;
	map->handle = handle;

	map->next = gsi_domain_map_list;
	gsi_domain_map_list = map;

	return 0;
}

/**
 * The starting GSI num occupied by different domains are:
 *
 * SW CINTC on Node(x)    : 0   + (512 * x)
 * SW PINTC on Node(x)    : 64  + (512 * x)
 * SW LPC-INTC on Node(x) : 256 + (512 * x)
 */
static struct fwnode_handle *sw64_gsi_to_domain_id(u32 gsi)
{
	struct gsi_domain_map *map;
	u32 base, limit;

	for (map = gsi_domain_map_list; map; map = map->next) {
		base = map->gsi_base;
		limit = map->gsi_base + map->gsi_count;

		if ((gsi >= base) && (gsi < limit))
			return map->handle;
	}

	return NULL;
}

static int __init pintc_parse_madt(union acpi_subtable_headers *header,
		const unsigned long end)
{
	struct acpi_madt_sw_pintc *pintc;

	pintc = (struct acpi_madt_sw_pintc *)header;

	/* Not yet supported */
	if (pintc->node > 0) {
		pr_warn(PREFIX "PINTC and LPC-INTC on node x(x > 0) are not supported\n");
		return 0;
	}

	if ((pintc->version == ACPI_MADT_SW_PINTC_VERSION_NONE) ||
		(pintc->version >= ACPI_MADT_SW_PINTC_VERSION_RESERVED)) {
		pr_err(PREFIX "invalid PINTC version\n");
		return -EINVAL;
	}

	return pintc_acpi_init(NULL, pintc);
}

#ifdef CONFIG_SW64_IRQ_MSI
static int __init msic_parse_madt(union acpi_subtable_headers *header,
		const unsigned long end)
{
	struct acpi_madt_sw_msic *msic;

	msic = (struct acpi_madt_sw_msic *)header;
	if ((msic->version == ACPI_MADT_SW_MSIC_VERSION_NONE) ||
			(msic->version >= ACPI_MADT_SW_MSIC_VERSION_RESERVED)) {
		pr_err(PREFIX "invalid MSIC version\n");
		return -EINVAL;
	}

	return msic_acpi_init(NULL, msic);
}
#endif

static bool __init
acpi_check_sw_cintc_entry(struct acpi_subtable_header *header,
		struct acpi_probe_entry *ape)
{
	struct acpi_madt_sw_cintc *cintc;

	cintc = (struct acpi_madt_sw_cintc *)header;
	if (cintc->version != ape->driver_data)
		return false;

	return true;
}

static __init int cintc_acpi_init(union acpi_subtable_headers *header,
		const unsigned long end)
{
	struct acpi_madt_sw_cintc *cintc;
	bool virtual;

	/**
	 * There are more than one MADT entry of SW CINTC in
	 * multi-core system, but the initialization here only
	 * needs to be performed once per node.
	 */
	if (cintc_handle)
		return 0;

	cintc = (struct acpi_madt_sw_cintc *)header;
	virtual = is_core_virtual(cintc->flags);
	pr_info(PREFIX "version [%u] (%s) found\n", cintc->version,
			virtual ? "virtual" : "physical");

	/**
	 * Currently, no irq_domain created for SW CINTC. The
	 * handle only used to avoid multiple initializations.
	 * Apart from this, there is no other meaning.
	 *
	 * Maybe we will create irq_domain for SW CINTC in the
	 * future to optimize the code.
	 */
	cintc_handle = irq_domain_alloc_named_fwnode("CINTC");
	if (!cintc_handle) {
		pr_err(PREFIX "failed to alloc fwnode\n");
		return -ENOMEM;
	}

	acpi_set_irq_model(ACPI_IRQ_MODEL_SWPIC, sw64_gsi_to_domain_id);

	/* Init SW PINTC */
	acpi_table_parse_madt(ACPI_MADT_TYPE_SW_PINTC,
			pintc_parse_madt, 0);

#ifdef CONFIG_SW64_IRQ_MSI
	/* Init SW MSIC */
	acpi_table_parse_madt(ACPI_MADT_TYPE_SW_MSIC,
			msic_parse_madt, 0);
#endif

	return 0;
}

IRQCHIP_ACPI_DECLARE(cintc_v1, ACPI_MADT_TYPE_SW_CINTC,
		acpi_check_sw_cintc_entry,
		ACPI_MADT_SW_CINTC_VERSION_V1,
		cintc_acpi_init);

IRQCHIP_ACPI_DECLARE(cintc_v2, ACPI_MADT_TYPE_SW_CINTC,
		acpi_check_sw_cintc_entry,
		ACPI_MADT_SW_CINTC_VERSION_V2,
		cintc_acpi_init);
#endif
