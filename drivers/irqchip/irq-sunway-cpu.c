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

static void handle_pci_intx_interrupt(unsigned long irq_info)
{
	unsigned int i;

	if (is_guest_or_emul()) {
		handle_irq(irq_info);
		return;
	}

	for (i = 0; i < PCI_NUM_INTX; i++) {
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

static void handle_mt_int(void)
{
	pr_info("enter mt int\n");
}

static void handle_nmi_int(void)
{
	pr_info("enter nmi int\n");
}

int pme_state;

asmlinkage void do_entInt(unsigned long type, unsigned long vector,
			  unsigned long irq_arg, struct pt_regs *regs)
{
	struct pt_regs *old_regs;
	extern char __idle_start[], __idle_end[];

	/* restart idle routine if it is interrupted */
	if (regs->pc > (u64)__idle_start && regs->pc < (u64)__idle_end)
		regs->pc = (u64)__idle_start;
	if (regs->cause != -2)
		irq_enter();
	else
		nmi_enter();
	old_regs = set_irq_regs(regs);

#ifdef CONFIG_PM
	if (is_junzhang_v1()) {
		if (pme_state == PME_WFW) {
			pme_state = PME_PENDING;
			goto out;
		}

		if (pme_state == PME_PENDING) {
			handle_pci_intx_interrupt(vector);
			pme_state = PME_CLEAR;
		}
	}
#endif

	if (is_guest_or_emul()) {
		if ((type & 0xffff) > 15) {
			vector = type;
			if (vector == 16 || vector == 17)
				type = INT_INTx;
			else
				type = INT_MSI;
		}
	}

	switch (type & 0xffff) {
	case INT_MSI:
		if (is_guest_or_emul())
			vt_handle_pci_msi_interrupt(type, vector, irq_arg);
		else
			handle_pci_msi_interrupt(type, vector, irq_arg);
		goto out;
	case INT_INTx:
		handle_pci_intx_interrupt(vector);
		goto out;

	case INT_IPI:
#ifdef CONFIG_SMP
		handle_ipi(regs);
		goto out;
#else
		irq_err_count++;
		pr_crit("Interprocessor interrupt? You must be kidding!\n");
		break;
#endif
	case INT_RTC:
		sw64_timer_interrupt();
		goto out;
	case INT_VT_SERIAL:
	case INT_VT_HOTPLUG:
	case INT_VT_GPIOA_PIN0:
		handle_irq(type);
		goto out;
#if defined(CONFIG_SUBARCH_C3B)
	case INT_PC0:
		perf_irq(PMC_PC0, regs);
		goto out;
	case INT_PC1:
		perf_irq(PMC_PC1, regs);
		goto out;
#elif defined(CONFIG_SUBARCH_C4)
	case INT_PC:
		perf_irq(PMC_PC0, regs);
		goto out;
#endif
	case INT_DEV:
		handle_dev_int(regs);
		goto out;
	case INT_FAULT:
		handle_fault_int();
		goto out;
	case INT_MT:
		handle_mt_int();
		goto out;
	case INT_NMI:
		handle_nmi_int();
		goto out;
	default:
		pr_crit("Hardware intr	%ld %lx? uh?\n", type, vector);
	}
	pr_crit("PC = %016lx PS = %04lx\n", regs->pc, regs->ps);

out:
	set_irq_regs(old_regs);
	if (regs->cause != -2)
		irq_exit();
	else
		nmi_exit();
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
