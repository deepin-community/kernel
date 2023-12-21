// SPDX-License-Identifier: GPL-2.0

#include <linux/init.h>
#include <linux/acpi.h>
#include <linux/irqdomain.h>
#include <linux/memblock.h>
#include <linux/smp.h>

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

static unsigned int possible_cores = 1; /* number of possible cores(at least boot core) */
static unsigned int present_cores = 1;  /* number of present cores(at least boot core) */
static unsigned int disabled_cores;     /* number of disabled cores */

int acpi_strict;
u64 arch_acpi_wakeup_start;
u64 acpi_saved_sp_s3;

#define SW_CINTC_FLAG_ENABLED        ACPI_MADT_ENABLED         /* 0x1 */
#define SW_CINTC_FLAG_ONLINE_CAPABLE 0x2                       /* hotplug capable */

#define is_core_enabled(flags)        ((flags) & SW_CINTC_FLAG_ENABLED)
#define is_core_online_capable(flags) ((flags) & SW_CINTC_FLAG_ONLINE_CAPABLE)

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

	for (i = 0; i < ARRAY_SIZE(__cpu_to_rcid); ++i)
		if (__cpu_to_rcid[i] == physical_id)
			return i;

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
		pr_err("SRAT: PXM %u -> CPU 0x%02x -> Node %u skipped apicid that is too big\n",
				pxm, pa->apic_id, node);
		return;
	}

	/* Record the mapping from logical core id to node id */
	cpu = rcid_to_cpu(pa->apic_id);
	if (cpu < 0) {
		pr_err("SRAT: Can not find the logical id for physical Core 0x%02x\n",
				pa->apic_id);
		return;
	}

	early_map_cpu_to_node(cpu, node);

	node_set(node, numa_nodes_parsed);
	pr_info("SRAT: PXM %u -> CPU 0x%02x -> Node %u\n",
			pxm, pa->apic_id, node);
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
	present_cores--;

	pr_info("cpu%d hot remove!\n", cpu);

	return 0;
}
EXPORT_SYMBOL(acpi_unmap_cpu);
#endif /* CONFIG_ACPI_HOTPLUG_CPU */

static bool __init is_rcid_duplicate(int rcid)
{
	int i;

	for_each_possible_cpu(i) {
		if (cpu_to_rcid(i) == rcid)
			return true;
	}

	return false;
}

static int __init
setup_rcid_and_core_mask(struct acpi_madt_sw_cintc *sw_cintc)
{
	unsigned int logical_core_id;
	int rcid = sw_cintc->hardware_id;

	/**
	 * The initial value of nr_cpu_ids is NR_CPUS, which
	 * represents the maximum number of cores in the system.
	 */
	if (possible_cores >= nr_cpu_ids) {
		pr_err(PREFIX "Max core num [%u] reached, core [0x%x] ignored\n",
			nr_cpu_ids, rcid);
		return -ENODEV;
	}

	/* The rcid of each core is unique */
	if (is_rcid_duplicate(rcid)) {
		pr_err(PREFIX "Duplicate core [0x%x] in MADT\n", rcid);
		return -EINVAL;
	}

	/* We can never disable the boot core, whose rcid is 0 */
	if ((rcid == 0) && !is_core_enabled(sw_cintc->flags)) {
		pr_err(PREFIX "Boot core disabled in MADT\n");
		return -EINVAL;
	}

	/* Online capable makes core possible */
	if (!is_core_enabled(sw_cintc->flags) &&
			!is_core_online_capable(sw_cintc->flags)) {
		disabled_cores++;
		return 0;
	}

	rcid_information_init(sw_cintc->version);

	/* The logical core ID of the boot core must be 0 */
	if (rcid == 0)
		logical_core_id = 0;
	else
		logical_core_id = possible_cores++;

	set_rcid_map(logical_core_id, rcid);
	set_cpu_possible(logical_core_id, true);
	store_cpu_data(logical_core_id);

	/**
	 * Whether the core will finally be online
	 * depends on two conditions:
	 * 1. core is enabled via firmware
	 * 2. core is not disabled by cmdline param(offline)
	 */
	if (is_core_enabled(sw_cintc->flags) &&
			!cpumask_test_cpu(logical_core_id, &cpu_offline)) {
		set_cpu_present(logical_core_id, true);
		if (logical_core_id != 0)
			present_cores++;
	}

	return 0;
}

static int __init acpi_parse_sw_cintc(union acpi_subtable_headers *header,
		const unsigned long end)
{
	struct acpi_madt_sw_cintc *sw_cintc = NULL;
	struct smp_rcb_struct *smp_rcb_base_addr = NULL;
	int ret;

	sw_cintc = (struct acpi_madt_sw_cintc *)header;
	if (BAD_MADT_ENTRY(sw_cintc, end)) {
		pr_err(PREFIX "SW CINTC entry error\n");
		return -EINVAL;
	}

	acpi_table_print_madt_entry(&header->common);

	ret = setup_rcid_and_core_mask(sw_cintc);
	if (ret)
		return ret;

	/**
	 * We use smp_rcb to help SMP boot. Its base
	 * address is hold in the MADT entry of SW CINTC.
	 */
	smp_rcb_base_addr = __va(sw_cintc->boot_flag_address);
	smp_rcb_init(smp_rcb_base_addr);

	return 0;
}

static int __init acpi_process_madt_sw_cintc(void)
{
	int i, ret;

	/* Clean the map from logical core ID to physical core ID */
	for (i = 0; i < ARRAY_SIZE(__cpu_to_rcid); ++i)
		set_rcid_map(i, -1);

	/* Clean core mask */
	init_cpu_possible(cpu_none_mask);
	init_cpu_present(cpu_none_mask);

	/* Parse SW CINTC entries one by one */
	ret = acpi_table_parse_madt(ACPI_MADT_TYPE_SW_CINTC,
			acpi_parse_sw_cintc, 0);
	if (ret < 0)
		return ret;

#if NR_CPUS > 1
	/* It's time to update nr_cpu_ids */
	nr_cpu_ids = possible_cores;
#endif

	pr_info(PREFIX "Detected %u possible CPU(s), %u CPU(s) are present\n",
			possible_cores, present_cores);

	return 0;
}

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
	if (acpi_disabled)
		return;

	pr_warn("Currently, ACPI is an experimental feature!\n");
	if (acpi_table_init()) {
		pr_err("Failed to init ACPI tables\n");
		disable_acpi();
		return;
	}

	pr_info("Successfully parsed ACPI table\n");

	/**
	 * Process SW64 Core Interrupt Controller(SW CINTC) in MADT table.
	 * No initialization of the interrupt controller here, mainly used
	 * to establish the mapping from logical core IDs to physical core
	 * IDs and set cpu mask.
	 */
	if (acpi_process_madt_sw_cintc()) {
		/* May be fatal error in MADT table */
		pr_err("Failed to parse SW CINTC\n");
		disable_acpi();
		return;
	}
}
