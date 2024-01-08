// SPDX-License-Identifier: GPL-2.0

#include <linux/init.h>
#include <linux/acpi.h>
#include <linux/irqdomain.h>
#include <linux/memblock.h>

#include <asm/early_ioremap.h>

#ifdef CONFIG_ACPI_HOTPLUG_CPU
#include <acpi/processor.h>
#endif

int acpi_disabled = 1;
EXPORT_SYMBOL(acpi_disabled);

int acpi_noirq = 1;		/* skip ACPI IRQ initialization */
int acpi_pci_disabled = 1;	/* skip ACPI PCI scan and IRQ initialization */
EXPORT_SYMBOL(acpi_pci_disabled);

static bool param_acpi_on  __initdata;
static bool param_acpi_off __initdata;

int acpi_strict;
u64 arch_acpi_wakeup_start;
u64 acpi_saved_sp_s3;

#define MAX_LOCAL_APIC 256

#define PREFIX			"ACPI: "
/*
 * The default interrupt routing model is PIC (8259).  This gets
 * overridden if IOAPICs are enumerated (below).
 */
enum acpi_irq_model_id acpi_irq_model = ACPI_IRQ_MODEL_IOSAPIC;
void __iomem *__init __acpi_map_table(unsigned long phys, unsigned long size)
{
	if (!phys || !size)
		return NULL;

	return early_ioremap(phys, size);
}
void __init __acpi_unmap_table(void __iomem *map, unsigned long size)
{
	if (!map || !size)
		return;

	early_iounmap(map, size);
}
/*
 * Following __acpi_xx functions should be implemented for sepecific cpu.
 */
int acpi_gsi_to_irq(u32 gsi, unsigned int *irqp)
{
	if (irqp != NULL)
		*irqp = acpi_register_gsi(NULL, gsi, -1, -1);

	return 0;
}
EXPORT_SYMBOL_GPL(acpi_gsi_to_irq);

int acpi_isa_irq_to_gsi(unsigned int isa_irq, u32 *gsi)
{
	if (gsi)
		*gsi = isa_irq;

	return 0;
}

int (*acpi_suspend_lowlevel)(void);

/*
 * success: return IRQ number (>=0)
 * failure: return < 0
 */
static struct irq_domain *irq_default_domain;
int acpi_register_gsi(struct device *dev, u32 gsi, int trigger, int polarity)
{
	u32 irq;

	irq = irq_find_mapping(irq_default_domain, gsi);

	return irq;
}
EXPORT_SYMBOL_GPL(acpi_register_gsi);

void acpi_unregister_gsi(u32 gsi)
{

}
EXPORT_SYMBOL_GPL(acpi_unregister_gsi);

/*
 *  ACPI based hotplug support for CPU
 */
#ifdef CONFIG_ACPI_HOTPLUG_CPU
/* wrapper to silence section mismatch warning */
int __ref acpi_map_lsapic(acpi_handle handle, int physid, int *pcpu)
{
	return 0;
}
EXPORT_SYMBOL(acpi_map_lsapic);

int acpi_unmap_lsapic(int cpu)
{
	return 0;
}
EXPORT_SYMBOL(acpi_unmap_lsapic);
#endif /* CONFIG_ACPI_HOTPLUG_CPU */

u8 acpi_checksum(u8 *table, u32 length)
{
	u8 ret = 0;

	while (length--) {
		ret += *table;
		table++;
	}
	return -ret;
}

static int __init parse_acpi(char *arg)
{
	if (!arg)
		return -EINVAL;

	/* disable both ACPI table parsing and interpreter */
	if (strcmp(arg, "off") == 0)
		param_acpi_off = true;
	else if (strcmp(arg, "on") == 0) /* prefer ACPI over device tree */
		param_acpi_on = true;
	else
		return -EINVAL; /* Core will printk when we return error. */

	return 0;
}
early_param("acpi", parse_acpi);

/*
 * __acpi_acquire_global_lock
 * will always return -1 indicating owning the lock.
 *
 * __acpi_release_global_lock will always return 0 indicating
 * no acquring request pending.
 */
int __acpi_acquire_global_lock(unsigned int *lock)
{
	return -1;
}

int __acpi_release_global_lock(unsigned int *lock)
{
	return 0;
}

#ifdef CONFIG_ACPI_NUMA
static int rcid_to_cpu(int physical_id)
{
	int i;

	for (i = 0; i < NR_CPUS; ++i) {
		if (__cpu_to_rcid[i] == physical_id)
			return i;
	}

	/* physical id not found */
	return -1;
}

/* Callback for Proximity Domain -> CPUID mapping */
void __init
acpi_numa_processor_affinity_init(struct acpi_srat_cpu_affinity *pa)
{
	int pxm, node;
	int cpu; // logical core id

	if (srat_disabled())
		return;
	if (pa->header.length != sizeof(struct acpi_srat_cpu_affinity)) {
		bad_srat();
		return;
	}
	if ((pa->flags & ACPI_SRAT_CPU_ENABLED) == 0)
		return;
	pxm = pa->proximity_domain_lo;
	if (acpi_srat_revision >= 2) {
		pxm |= (pa->proximity_domain_hi[0] << 8);
		pxm |= (pa->proximity_domain_hi[1] << 16);
		pxm |= (pa->proximity_domain_hi[2] << 24);
	}

	node = acpi_map_pxm_to_node(pxm);
	if (node < 0) {
		pr_err("SRAT: Too many proximity domains %x\n", pxm);
		bad_srat();
		return;
	}

	if (pa->apic_id >= CONFIG_NR_CPUS) {
		pr_err("SRAT: PXM %u -> CPU 0x%02x -> Node %u skipped apicid that is too big\n", pxm, pa->apic_id, node);
		return;
	}

	/* Record the mapping from logical core id to node id */
	cpu = rcid_to_cpu(pa->apic_id);
	if (cpu < 0) {
		pr_err("SRAT: Can not find the logical id for physical Core 0x%02x\n", pa->apic_id);
		return;
	}

	early_map_cpu_to_node(cpu, node);

	node_set(node, numa_nodes_parsed);
	pr_info("SRAT: PXM %u -> CPU 0x%02x -> Node %u\n", pxm, pa->apic_id, node);
}

#ifdef CONFIG_MEMORY_HOTPLUG
static inline int save_add_info(void) { return 1; }
#else
static inline int save_add_info(void) { return 0; }
#endif

#endif

void __init arch_reserve_mem_area(acpi_physical_address addr, size_t size)
{
}

#ifdef CONFIG_ACPI_HOTPLUG_CPU
static int acpi_map_cpu2node(acpi_handle handle, int cpu, int physid)
{
#ifdef CONFIG_ACPI_NUMA
	int nid;

	nid = acpi_get_node(handle);
	if (nid != NUMA_NO_NODE) {
		set_cpuid_to_node(cpu, nid);
		node_set(nid, numa_nodes_parsed);
	}
#endif
	return 0;
}

int acpi_map_cpu(acpi_handle handle, phys_cpuid_t physid, u32 acpi_id,
			int *pcpu)
{
	int cpu;
	struct acpi_madt_local_apic *processor;

	processor = kzalloc(sizeof(struct acpi_madt_local_apic), GFP_KERNEL);
	processor->id = physid;
	processor->processor_id = acpi_id;
	processor->lapic_flags = ACPI_MADT_ENABLED;

	cpu = set_processor_mask(processor);
	if (cpu < 0) {
		pr_info(PREFIX "Unable to map lapic to logical cpu number\n");
		return cpu;
	}

	acpi_map_cpu2node(handle, cpu, physid);

	*pcpu = cpu;
	return 0;
}
EXPORT_SYMBOL(acpi_map_cpu);

int acpi_unmap_cpu(int cpu)
{
#ifdef CONFIG_ACPI_NUMA
	set_cpuid_to_node(cpu, NUMA_NO_NODE);
#endif
	set_cpu_present(cpu, false);
	num_processors--;

	pr_info("cpu%d hot remove!\n", cpu);

	return 0;
}
EXPORT_SYMBOL(acpi_unmap_cpu);
#endif /* CONFIG_ACPI_HOTPLUG_CPU */

void __init acpi_boot_table_init(void)
{
	/**
	 * ACPI is disabled by default.
	 * ACPI is only enabled when firmware passes ACPI table
	 * and sets boot parameter "acpi=on".
	 */
	if (param_acpi_on)
		enable_acpi();

	/*
	 * If acpi_disabled, bail out
	 */
	if (!acpi_disabled) {
		pr_warn("Currently, ACPI is an experimental feature!\n");
		if (acpi_table_init()) {
			pr_err("Failed to init ACPI tables\n");
			disable_acpi();
		} else
			pr_info("Successfully parsed ACPI table\n");
	}
}
