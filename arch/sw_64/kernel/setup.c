// SPDX-License-Identifier: GPL-2.0

/*
 * Bootup setup stuff.
 */

#include <linux/screen_info.h>
#include <linux/delay.h>
#include <linux/kexec.h>
#include <linux/console.h>
#include <linux/memblock.h>
#include <linux/root_dev.h>
#include <linux/initrd.h>
#ifdef CONFIG_MAGIC_SYSRQ
#include <linux/sysrq.h>
#include <linux/reboot.h>
#endif
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <linux/genalloc.h>
#include <linux/acpi.h>
#include <linux/cpu.h>

#include <asm/efi.h>
#include <asm/kvm_cma.h>
#include <asm/mmu_context.h>
#include <asm/sw64_init.h>
#include <asm/timer.h>
#include <asm/pci_impl.h>

#include "proto.h"

#undef DEBUG_DISCONTIG
#ifdef DEBUG_DISCONTIG
#define DBGDCONT(args...) pr_debug(args)
#else
#define DBGDCONT(args...)
#endif

int __cpu_to_rcid[NR_CPUS];		/* Map logical to physical */
EXPORT_SYMBOL(__cpu_to_rcid);

DEFINE_PER_CPU(unsigned long, hard_node_id) = { 0 };
static DEFINE_PER_CPU(struct cpu, cpu_devices);

#ifdef CONFIG_SUBARCH_C3B
#if defined(CONFIG_KVM) || defined(CONFIG_KVM_MODULE)
struct cma *sw64_kvm_cma;
EXPORT_SYMBOL(sw64_kvm_cma);

static phys_addr_t kvm_mem_size;
static phys_addr_t kvm_mem_base;

struct gen_pool *sw64_kvm_pool;
EXPORT_SYMBOL(sw64_kvm_pool);
#endif
#endif

static inline int phys_addr_valid(unsigned long addr)
{
	/*
	 * At this point memory probe has not been done such that max_pfn
	 * and other physical address variables cannot be used, so let's
	 * roughly judge physical address based on arch specific bit.
	 */
	return !(addr >> (cpu_desc.pa_bits - 1));
}

extern struct atomic_notifier_head panic_notifier_list;
static int sw64_panic_event(struct notifier_block *, unsigned long, void *);
static struct notifier_block sw64_panic_block = {
	sw64_panic_event,
	NULL,
	INT_MAX /* try to do it first */
};

/* the value is IOR: CORE_ONLIE*/
cpumask_t core_start = CPU_MASK_NONE;

static struct resource data_resource = {
	.name   = "Kernel data",
	.start  = 0,
	.end    = 0,
	.flags  = IORESOURCE_BUSY | IORESOURCE_SYSTEM_RAM
};

static struct resource code_resource = {
	.name   = "Kernel code",
	.start  = 0,
	.end    = 0,
	.flags  = IORESOURCE_BUSY | IORESOURCE_SYSTEM_RAM
};

static struct resource bss_resource = {
	.name   = "Kernel bss",
	.start  = 0,
	.end    = 0,
	.flags  = IORESOURCE_BUSY | IORESOURCE_SYSTEM_RAM
};

/* A collection of per-processor data.  */
struct cpuinfo_sw64 cpu_data[NR_CPUS];
EXPORT_SYMBOL(cpu_data);

DEFINE_STATIC_KEY_TRUE(run_mode_host_key);
DEFINE_STATIC_KEY_FALSE(run_mode_guest_key);
DEFINE_STATIC_KEY_FALSE(run_mode_emul_key);
struct cpu_desc_t cpu_desc;
struct socket_desc_t socket_desc[MAX_NUMSOCKETS];
int memmap_nr;
struct memmap_entry memmap_map[MAX_NUMMEMMAPS];
bool memblock_initialized;

cpumask_t cpu_offline = CPU_MASK_NONE;

static char command_line[COMMAND_LINE_SIZE] __initdata;
#ifdef CONFIG_CMDLINE_BOOL
static char builtin_cmdline[COMMAND_LINE_SIZE] __initdata = CONFIG_CMDLINE;
#endif

/* boot_params */
struct boot_params *sunway_boot_params = (struct boot_params *) (PARAM + 0x100);

/*
 * The format of "screen_info" is strange, and due to early
 * i386-setup code. This is just enough to make the console
 * code think we're on a VGA color display.
 */

struct screen_info screen_info = {
	.orig_x = 0,
	.orig_y = 25,
	.orig_video_cols = 80,
	.orig_video_lines = 25,
	.orig_video_isVGA = 1,
	.orig_video_points = 16
};
EXPORT_SYMBOL(screen_info);

/*
 * Move global data into per-processor storage.
 */
void store_cpu_data(int cpu)
{
	cpu_data[cpu].last_asid = ASID_FIRST_VERSION;
}

#ifdef CONFIG_KEXEC

void *kexec_control_page;

#define KTEXT_MAX	KERNEL_IMAGE_SIZE

static void __init kexec_control_page_init(void)
{
	phys_addr_t addr;

	addr = memblock_phys_alloc_range(KEXEC_CONTROL_PAGE_SIZE, PAGE_SIZE,
					0, KTEXT_MAX);
	kexec_control_page = (void *)(__START_KERNEL_map + addr);
}

/*
 * reserve_crashkernel() - reserves memory are for crash kernel
 *
 * This function reserves memory area given in "crashkernel=" kernel command
 * line parameter. The memory reserved is used by a dump capture kernel when
 * primary kernel is crashing.
 */
static void __init reserve_crashkernel(void)
{
	unsigned long long crash_size, crash_base;
	int ret;

	ret = parse_crashkernel(boot_command_line, mem_desc.size,
			&crash_size, &crash_base);
	if (ret || !crash_size)
		return;

	if (!crash_size) {
		pr_warn("size of crash kernel memory unspecified, no memory reserved for crash kernel\n");
		return;
	}
	if (!crash_base) {
		pr_warn("base of crash kernel memory unspecified, no memory reserved for crash kernel\n");
		return;
	}

	if (!memblock_is_region_memory(crash_base, crash_size))
		memblock_add(crash_base, crash_size);

	ret = memblock_reserve(crash_base, crash_size);
	if (ret < 0) {
		pr_warn("crashkernel reservation failed - memory is in use [mem %#018llx-%#018llx]\n",
				crash_base, crash_base + crash_size - 1);
		return;
	}

	pr_info("Reserving %ldMB of memory at %ldMB for crashkernel (System RAM: %ldMB)\n",
			(unsigned long)(crash_size >> 20),
			(unsigned long)(crash_base >> 20),
			(unsigned long)(mem_desc.size >> 20));

	ret = add_memmap_region(crash_base, crash_size, memmap_crashkernel);
	if (ret)
		pr_warn("Add crash kernel area [mem %#018llx-%#018llx] to memmap region failed.\n",
				crash_base, crash_base + crash_size - 1);

	if (crash_base >= KERNEL_IMAGE_SIZE)
		pr_warn("Crash base should be less than %#x\n", KERNEL_IMAGE_SIZE);

	crashk_res.start = crash_base;
	crashk_res.end = crash_base + crash_size - 1;
	insert_resource(&iomem_resource, &crashk_res);
}
#else /* !defined(CONFIG_KEXEC)         */
static void __init reserve_crashkernel(void) {}
static void __init kexec_control_page_init(void) {}
#endif /* !defined(CONFIG_KEXEC)  */

/*
 * I/O resources inherited from PeeCees. Except for perhaps the
 * turbochannel SWs, everyone has these on some sort of SuperIO chip.
 *
 * ??? If this becomes less standard, move the struct out into the
 * machine vector.
 */

static void __init
reserve_std_resources(void)
{
	static struct resource standard_io_resources[] = {
		{ .name = "rtc", .start = -1, .end = -1 },
		{ .name = "dma1", .start = 0x00, .end = 0x1f },
		{ .name = "pic1", .start = 0x20, .end = 0x3f },
		{ .name = "timer", .start = 0x40, .end = 0x5f },
		{ .name = "keyboard", .start = 0x60, .end = 0x6f },
		{ .name = "dma page reg", .start = 0x80, .end = 0x8f },
		{ .name = "pic2", .start = 0xa0, .end = 0xbf },
		{ .name = "dma2", .start = 0xc0, .end = 0xdf },
	};

	struct resource *io = &ioport_resource;
	size_t i;

	if (hose_head) {
		struct pci_controller *hose;

		for (hose = hose_head; hose; hose = hose->next)
			if (hose->index == 0) {
				io = hose->io_space;
				break;
			}
	}

	/* Fix up for the Jensen's queer RTC placement.  */
	standard_io_resources[0].start = RTC_PORT(0);
	standard_io_resources[0].end = RTC_PORT(0) + 0x10;

	for (i = 0; i < ARRAY_SIZE(standard_io_resources); ++i)
		request_resource(io, standard_io_resources+i);
}

static int __init parse_memmap_one(char *p)
{
	char *oldp;
	u64 start_at, mem_size;
	int ret;

	if (!p)
		return -EINVAL;

	if (!strncmp(p, "exactmap", 8)) {
		pr_err("\"memmap=exactmap\" not valid on sw64\n");
		return 0;
	}

	oldp = p;
	mem_size = memparse(p, &p);
	if (p == oldp)
		return -EINVAL;

	if (*p == '@') {
		pr_err("\"memmap=nn@ss\" invalid on sw64\n");
	} else if (*p == '#') {
		pr_err("\"memmap=nn#ss\" (force ACPI data) invalid on sw64\n");
	} else if (*p == '$') {
		start_at = memparse(p + 1, &p);
		ret = add_memmap_region(start_at, mem_size, memmap_reserved);
		if (ret)
			return ret;
	} else {
		return -EINVAL;
	}
	return *p == '\0' ? 0 : -EINVAL;
}

static int __init setup_memmap(char *str)
{
	while (str) {
		char *k = strchr(str, ',');

		if (k)
			*k++ = 0;

		parse_memmap_one(str);
		str = k;
	}

	return 0;
}
early_param("memmap", setup_memmap);

static int __init setup_cpuoffline(char *p)
{
	cpulist_parse(p, &cpu_offline);
	cpumask_clear_cpu(0, &cpu_offline);
	return 0;
}
early_param("cpuoffline", setup_cpuoffline);

#ifdef CONFIG_BLK_DEV_INITRD
static void * __init move_initrd(unsigned long mem_limit)
{
	void *start;
	unsigned long size;

	size = initrd_end - initrd_start;
	start = memblock_alloc_from(PAGE_ALIGN(size), PAGE_SIZE, 0);
	if (!start || __pa(start) + size > mem_limit) {
		initrd_start = initrd_end = 0;
		return NULL;
	}
	memmove(start, (void *)initrd_start, size);
	initrd_start = (unsigned long)start;
	initrd_end = initrd_start + size;
	pr_info("initrd moved to 0x%px\n", start);
	return start;
}
#else
static void * __init move_initrd(unsigned long mem_limit)
{
	return NULL;
}
#endif

static bool __init memmap_range_valid(phys_addr_t base, phys_addr_t *size)
{
	if (base > memblock_end_of_DRAM())
		return false;

	if ((base + *size) > memblock_end_of_DRAM())
		*size = memblock_end_of_DRAM() - base;

	return true;
}

void __init process_memmap(void)
{
	static int i;	// Make it static so we won't start over again every time.
	int ret;
	phys_addr_t base, size;
	unsigned long dma_end __maybe_unused = (MAX_DMA32_PFN << PAGE_SHIFT);

	if (!memblock_initialized)
		return;

	for (; i < memmap_nr; i++) {
		base = memmap_map[i].addr;
		size = memmap_map[i].size;
		switch (memmap_map[i].type) {
		case memmap_reserved:
			if (!memmap_range_valid(base, &size)) {
				pr_err("reserved memmap region [mem %#018llx-%#018llx] beyond end of memory (%#018llx)\n",
						base, base + size - 1, memblock_end_of_DRAM());
			} else {
				pr_info("reserved memmap region [mem %#018llx-%#018llx]\n",
						base, base + size - 1);
				ret = memblock_mark_nomap(base, size);
				if (ret)
					pr_err("reserve memmap region [mem %#018llx-%#018llx] failed\n",
							base, base + size - 1);
				else if (IS_ENABLED(CONFIG_ZONE_DMA32) && (base < dma_end))
					pr_warn("memmap region [mem %#018llx-%#018llx] overlapped with DMA32 region\n",
							base, base + size - 1);
			}
			break;
		case memmap_pci:
			if (!memmap_range_valid(base, &size)) {
				pr_err("pci memmap region [mem %#018llx-%#018llx] beyond end of memory (%#018llx)\n",
						base, base + size - 1, memblock_end_of_DRAM());
			} else {
				pr_info("pci memmap region [mem %#018llx-%#018llx]\n",
						base, base + size - 1);
				ret = memblock_mark_nomap(base, size);
				if (ret)
					pr_err("reserve memmap region [mem %#018llx-%#018llx] failed\n",
							base, base + size - 1);
			}
			break;
		case memmap_initrd:
			if ((base + size) > memblock_end_of_DRAM()) {
				phys_addr_t old_base = base;

				base = (unsigned long) move_initrd(memblock_end_of_DRAM());
				if (!base) {
					pr_err("initrd memmap region [mem %#018llx-%#018llx] extends beyond end of memory (%#018llx)\n",
							old_base, old_base + size - 1, memblock_end_of_DRAM());
					break;
				}
				memmap_map[i].addr = base;
			}
			pr_info("initrd memmap region [mem %#018llx-%#018llx]\n", base, base + size - 1);
			ret = memblock_reserve(base, size);
			if (ret)
				pr_err("reserve memmap region [mem %#018llx-%#018llx] failed\n",
						base, base + size - 1);
			break;
		case memmap_kvm:
		case memmap_crashkernel:
			/* kvm and crashkernel are handled elsewhere, skip */
			break;
		case memmap_acpi:
			pr_err("ACPI memmap region is not supported.\n");
			break;
		case memmap_use:
			pr_err("Force usage memmap region is not supported.\n");
			break;
		case memmap_protected:
			pr_err("Protected memmap region is not supported.\n");
			break;
		default:
			pr_err("Unknown type of memmap region.\n");
		}
	}
}

int __init add_memmap_region(u64 addr, u64 size, enum memmap_types type)
{
	if (memmap_nr >= ARRAY_SIZE(memmap_map)) {
		pr_err("Ooops! Too many entries in the memory map!\n");
		return -EPERM;
	}

	if (addr + size <= addr) {
		pr_warn("Trying to add an invalid memory region, skipped\n");
		return -EINVAL;
	}

	memmap_map[memmap_nr].addr = addr;
	memmap_map[memmap_nr].size = size;
	memmap_map[memmap_nr].type = type;
	memmap_nr++;

	process_memmap();

	return 0;
}

static struct resource* __init
insert_ram_resource(u64 start, u64 end, bool reserved)
{
	struct resource *res =
		kzalloc(sizeof(struct resource), GFP_ATOMIC);
	if (!res)
		return NULL;
	if (reserved) {
		res->name = "reserved";
		res->flags = IORESOURCE_MEM;
	} else {
		res->name = "System RAM";
		res->flags = IORESOURCE_SYSTEM_RAM | IORESOURCE_BUSY;
	}
	res->start = start;
	res->end = end;
	if (insert_resource(&iomem_resource, res)) {
		kfree(res);
		return NULL;
	}
	return res;
}

static int __init request_standard_resources(void)
{
	struct memblock_region *mblk;

	extern char _text[], _etext[];
	extern char _sdata[], _edata[];
	extern char __bss_start[], __bss_stop[];

	for_each_mem_region(mblk) {
		if (!memblock_is_nomap(mblk))
			insert_ram_resource(mblk->base,
					mblk->base + mblk->size - 1, 0);
		else
			insert_ram_resource(mblk->base,
					mblk->base + mblk->size - 1, 1);
	}

	code_resource.start = __pa_symbol(_text);
	code_resource.end = __pa_symbol(_etext)-1;
	data_resource.start = __pa_symbol(_sdata);
	data_resource.end = __pa_symbol(_edata)-1;
	bss_resource.start = __pa_symbol(__bss_start);
	bss_resource.end = __pa_symbol(__bss_stop)-1;

	insert_resource(&iomem_resource, &code_resource);
	insert_resource(&iomem_resource, &data_resource);
	insert_resource(&iomem_resource, &bss_resource);

	return 0;
}
subsys_initcall(request_standard_resources);

#ifdef CONFIG_NUMA
extern void cpu_set_node(void);
#endif

static void __init show_socket_mem_layout(void)
{
	int i;
	phys_addr_t base, size, end;

	base = 0;

	pr_info("Socket memory layout:\n");
	for (i = 0; i < MAX_NUMSOCKETS; i++) {
		if (socket_desc[i].is_online) {
			size = socket_desc[i].socket_mem;
			end = base + size - 1;
			pr_info("Socket %d: [mem %#018llx-%#018llx], size %llu\n",
					i, base, end, size);
			base = end + 1;
		}
	}
	pr_info("Reserved memory size for Socket 0: %#lx\n", NODE0_START);
}

int page_is_ram(unsigned long pfn)
{
	pfn <<= PAGE_SHIFT;

	return pfn >= mem_desc.base && pfn < (mem_desc.base + mem_desc.size);
}

static int __init topology_init(void)
{
	int i, ret;

	for_each_possible_cpu(i) {
		struct cpu *cpu = &per_cpu(cpu_devices, i);

#ifdef CONFIG_HOTPLUG_CPU
		if (i != 0)
			cpu->hotpluggable = 1;
#endif
		ret = register_cpu(cpu, i);
		if (unlikely(ret))
			pr_warn("Warning: %s: register_cpu %d failed (%d)\n",
			       __func__, i, ret);
	}

	return 0;
}
subsys_initcall(topology_init);

static void __init setup_machine_fdt(void)
{
#ifdef CONFIG_USE_OF
	void *dt_virt;
	const char *name;

	/* Give a chance to select kernel builtin DTB firstly */
	if (IS_ENABLED(CONFIG_BUILTIN_DTB))
		dt_virt = (void *)__dtb_start;
	else {
		dt_virt = (void *)sunway_boot_params->dtb_start;
		if (virt_to_phys(dt_virt) < virt_to_phys(__bss_stop)) {
			pr_emerg("BUG: DTB has been corrupted by kernel image!\n");
			while (true)
				cpu_relax();
		}
	}

	if (!phys_addr_valid(__boot_pa(dt_virt)) ||
			!early_init_dt_scan(dt_virt)) {
		pr_crit("\n"
			"Error: invalid device tree blob at virtual address %px\n"
			"The dtb must be 8-byte aligned and must not exceed 2 MB in size\n"
			"\nPlease check your bootloader.",
			dt_virt);

		while (true)
			cpu_relax();
	}

	name = of_flat_dt_get_machine_name();
	if (!name)
		return;

	pr_info("Machine model: %s\n", name);
#else
	pr_info("Kernel disable device tree support.\n");
	return;
#endif
}

void __init device_tree_init(void)
{
	unflatten_and_copy_device_tree();
	sunway_boot_params->dtb_start = (__u64)initial_boot_params;
}

static void __init setup_cpu_info(void)
{
	int i;
	struct cache_desc *c;
	unsigned long val;

	val = cpuid(GET_TABLE_ENTRY, 0);
	cpu_desc.model = CPUID_MODEL(val);
	cpu_desc.family = CPUID_FAMILY(val);
	cpu_desc.chip_var = CPUID_CHIP_VAR(val);
	cpu_desc.arch_var = CPUID_ARCH_VAR(val);
	cpu_desc.arch_rev = CPUID_ARCH_REV(val);
	cpu_desc.pa_bits = CPUID_PA_BITS(val);
	cpu_desc.va_bits = CPUID_VA_BITS(val);

	for (i = 0; i < VENDOR_ID_MAX; i++) {
		val = cpuid(GET_VENDOR_ID, i);
		memcpy(cpu_desc.vendor_id + (i * 8), &val, 8);
	}

	for (i = 0; i < MODEL_MAX; i++) {
		val = cpuid(GET_MODEL, i);
		memcpy(cpu_desc.model_id + (i * 8), &val, 8);
	}

	cpu_desc.frequency = cpuid(GET_CPU_FREQ, 0) * 1000UL * 1000UL;

	for (i = 0; i < NR_CPUS; i++) {
		c = &(cpu_data[i].icache);
		val = cpuid(GET_CACHE_INFO, L1_ICACHE);
		c->size = CACHE_SIZE(val);
		c->linesz = 1 << (CACHE_LINE_BITS(val));
		c->sets = 1 << (CACHE_INDEX_BITS(val));
		c->ways = c->size / c->sets / c->linesz;

		c = &(cpu_data[i].dcache);
		val = cpuid(GET_CACHE_INFO, L1_DCACHE);
		c->size = CACHE_SIZE(val);
		c->linesz = 1 << (CACHE_LINE_BITS(val));
		c->sets = 1 << (CACHE_INDEX_BITS(val));
		c->ways = c->size / c->sets / c->linesz;

		c = &(cpu_data[i].scache);
		val = cpuid(GET_CACHE_INFO, L2_CACHE);
		c->size = CACHE_SIZE(val);
		c->linesz = 1 << (CACHE_LINE_BITS(val));
		c->sets = 1 << (CACHE_INDEX_BITS(val));
		c->ways = c->size / c->sets / c->linesz;

		c = &(cpu_data[i].tcache);
		val = cpuid(GET_CACHE_INFO, L3_CACHE);
		c->size = CACHE_SIZE(val);
		c->linesz = 1 << (CACHE_LINE_BITS(val));
		c->sets = 1 << (CACHE_INDEX_BITS(val));
		c->ways = c->size / c->sets / c->linesz;
	}
}

static void __init setup_run_mode(void)
{
	if (*(unsigned long *)MMSIZE) {
		static_branch_disable(&run_mode_host_key);
		if (*(unsigned long *)MMSIZE & EMUL_FLAG) {
			pr_info("run mode: emul\n");
			static_branch_disable(&run_mode_guest_key);
			static_branch_enable(&run_mode_emul_key);

		} else {
			pr_info("run mode: guest\n");
			static_branch_enable(&run_mode_guest_key);
			static_branch_disable(&run_mode_emul_key);
		}
	} else {
		pr_info("run mode: host\n");
		static_branch_enable(&run_mode_host_key);
		static_branch_disable(&run_mode_guest_key);
		static_branch_disable(&run_mode_emul_key);
	}
}

static void __init setup_socket_info(void)
{
	int i;
	int numsockets = sw64_chip->get_cpu_num();

	memset(socket_desc, 0, MAX_NUMSOCKETS * sizeof(struct socket_desc_t));

	for (i = 0; i < numsockets; i++) {
		socket_desc[i].is_online = 1;
		if (sw64_chip_init->early_init.get_node_mem)
			socket_desc[i].socket_mem = sw64_chip_init->early_init.get_node_mem(i);
	}
}

#ifdef CONFIG_BLK_DEV_INITRD
static void __init reserve_mem_for_initrd(void)
{
	int ret;

	initrd_start = sunway_boot_params->initrd_start;
	if (initrd_start) {
		initrd_start = __pa(initrd_start) + PAGE_OFFSET;
		initrd_end = initrd_start + sunway_boot_params->initrd_size;
		pr_info("Initial ramdisk at: 0x%px (%llu bytes)\n",
				(void *)initrd_start, sunway_boot_params->initrd_size);

		ret = add_memmap_region(__pa(initrd_start), initrd_end - initrd_start, memmap_initrd);
		if (ret)
			pr_err("Add initrd area [mem %#018lx-%#018lx] to memmap region failed.\n",
				__pa(initrd_start), __pa(initrd_end - 1));
	}
}
#endif /* CONFIG_BLK_DEV_INITRD */

#ifdef CONFIG_SUBARCH_C3B
#if defined(CONFIG_KVM) || defined(CONFIG_KVM_MODULE)
static int __init early_kvm_reserved_mem(char *p)
{
	if (!p) {
		pr_err("Config string not provided\n");
		return -EINVAL;
	}

	kvm_mem_size = memparse(p, &p);
	if (*p != '@')
		return -EINVAL;
	kvm_mem_base = memparse(p + 1, &p);
	return 0;
}
early_param("kvm_mem", early_kvm_reserved_mem);

void __init sw64_kvm_reserve(void)
{
	kvm_cma_declare_contiguous(kvm_mem_base, kvm_mem_size, 0,
			PAGE_SIZE, 0, "sw64_kvm_cma", &sw64_kvm_cma);
}
#endif
#endif

void __init
setup_arch(char **cmdline_p)
{
	/**
	 * Work around the unaligned access exception to parse ACPI
	 * tables in the following function acpi_boot_table_init().
	 */
	trap_init();

	jump_label_init();
	setup_cpu_info();
	setup_run_mode();
	setup_chip_ops();
	setup_socket_info();
	show_socket_mem_layout();
	sw64_chip_init->early_init.setup_core_map(&core_start);
	if (is_guest_or_emul())
		get_vt_smp_info();

	setup_sched_clock();

	setup_machine_fdt();

	/* Register a call for panic conditions. */
	atomic_notifier_chain_register(&panic_notifier_list,
			&sw64_panic_block);

	callback_init();

	/* command line */
	if (!sunway_boot_params->cmdline)
		sunway_boot_params->cmdline = (unsigned long)COMMAND_LINE;

	strscpy(boot_command_line, (char *)sunway_boot_params->cmdline, COMMAND_LINE_SIZE);

#if IS_ENABLED(CONFIG_CMDLINE_BOOL)
#if IS_ENABLED(CONFIG_CMDLINE_OVERRIDE)
	strscpy(boot_command_line, builtin_cmdline, COMMAND_LINE_SIZE);
	strscpy((char *)sunway_boot_params->cmdline, boot_command_line, COMMAND_LINE_SIZE);
#else
	if (builtin_cmdline[0]) {
		/* append builtin to boot loader cmdline */
		strlcat(boot_command_line, " ", COMMAND_LINE_SIZE);
		strlcat(boot_command_line, builtin_cmdline, COMMAND_LINE_SIZE);
	}
#endif /* CMDLINE_EXTEND */
#endif

	strscpy(command_line, boot_command_line, COMMAND_LINE_SIZE);
	*cmdline_p = command_line;

	/*
	 * Process command-line arguments.
	 */
	parse_early_param();

	/* Find our memory.  */
	mem_detect();

#ifdef CONFIG_PCI
	reserve_mem_for_pci();
#endif

#ifdef CONFIG_BLK_DEV_INITRD
	reserve_mem_for_initrd();
#endif

	sw64_memblock_init();

	reserve_crashkernel();

	/* Reserve large chunks of memory for use by CMA for KVM. */
#ifdef CONFIG_SUBARCH_C3B
#if defined(CONFIG_KVM) || defined(CONFIG_KVM_MODULE)
	sw64_kvm_reserve();
#endif
#endif

	efi_init();

	/* Try to upgrade ACPI tables via initrd */
	acpi_table_upgrade();

	/* Parse the ACPI tables for possible boot-time configuration */
	acpi_boot_table_init();

#ifdef CONFIG_SMP
	setup_smp();
#else
	store_cpu_data(0);
#endif

	sw64_numa_init();

	memblock_dump_all();

	sparse_init();

	zone_sizes_init();

	paging_init();

	kexec_control_page_init();

	/*
	 * Initialize the machine. Usually has to do with setting up
	 * DMA windows and the like.
	 */
	sw64_init_arch();

	/* Reserve standard resources.  */
	reserve_std_resources();

	/*
	 * Give us a default console. TGA users will see nothing until
	 * chr_dev_init is called, rather late in the boot sequence.
	 */

#ifdef CONFIG_VT
#if defined(CONFIG_VGA_CONSOLE)
	conswitchp = &vga_con;
#elif defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif
#endif

	/* Default root filesystem to sda2.  */
	ROOT_DEV = MKDEV(SCSI_DISK0_MAJOR, 2);

	if (acpi_disabled) {
#ifdef CONFIG_NUMA
		cpu_set_node();
#endif
		device_tree_init();
	}
}


static int
show_cpuinfo(struct seq_file *f, void *slot)
{
	int i;
	unsigned long cpu_freq;

	cpu_freq = cpuid(GET_CPU_FREQ, 0);

	for_each_online_cpu(i) {
		/*
		 * glibc reads /proc/cpuinfo to determine the number of
		 * online processors, looking for lines beginning with
		 * "processor".  Give glibc what it expects.
		 */
		seq_printf(f, "processor\t: %u\n"
				"vendor_id\t: %s\n"
				"cpu family\t: %d\n"
				"model\t\t: %u\n"
				"model name\t: %s CPU @ %lu.%lu%luGHz\n"
				"cpu variation\t: %u\n"
				"cpu revision\t: %u\n",
				i, cpu_desc.vendor_id, cpu_desc.family,
				cpu_desc.model, cpu_desc.model_id,
				cpu_freq / 1000, (cpu_freq % 1000) / 100,
				(cpu_freq % 100) / 10,
				cpu_desc.arch_var, cpu_desc.arch_rev);
		seq_printf(f, "cpu MHz\t\t: %lu.00\n"
				"cache size\t: %u KB\n"
				"physical id\t: %d\n"
				"bogomips\t: %lu.%02lu\n",
				get_cpu_freq() / 1000 / 1000, cpu_data[i].tcache.size >> 10,
				cpu_topology[i].package_id,
				loops_per_jiffy / (500000/HZ),
				(loops_per_jiffy / (5000/HZ)) % 100);

		seq_printf(f, "flags\t\t: fpu simd vpn upn cpuid\n");
		seq_printf(f, "page size\t: %d\n", 8192);
		seq_printf(f, "cache_alignment\t: %d\n", cpu_data[i].tcache.linesz);
		seq_printf(f, "address sizes\t: %u bits physical, %u bits virtual\n\n",
				cpu_desc.pa_bits, cpu_desc.va_bits);
	}
	return 0;
}

/*
 * We show only CPU #0 info.
 */
static void *
c_start(struct seq_file *f, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

static void *
c_next(struct seq_file *f, void *v, loff_t *pos)
{
	(*pos)++;
	return NULL;
}

static void
c_stop(struct seq_file *f, void *v)
{
}

const struct seq_operations cpuinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= show_cpuinfo,
};


static int
sw64_panic_event(struct notifier_block *this, unsigned long event, void *ptr)
{
	return NOTIFY_DONE;
}

static __init int add_pcspkr(void)
{
	struct platform_device *pd;
	int ret;

	pd = platform_device_alloc("pcspkr", -1);
	if (!pd)
		return -ENOMEM;

	ret = platform_device_add(pd);
	if (ret)
		platform_device_put(pd);

	return ret;
}
device_initcall(add_pcspkr);

#ifdef CONFIG_DEBUG_FS
struct dentry *sw64_debugfs_dir;
EXPORT_SYMBOL(sw64_debugfs_dir);

static int __init debugfs_sw64(void)
{
	struct dentry *d;

	d = debugfs_create_dir("sw64", NULL);
	if (!d)
		return -ENOMEM;
	sw64_debugfs_dir = d;
	return 0;
}
arch_initcall(debugfs_sw64);
#endif

#ifdef CONFIG_OF
static int __init sw64_of_init(void)
{
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
	return 0;
}
core_initcall(sw64_of_init);
#endif

#ifdef CONFIG_SUBARCH_C3B
#if defined(CONFIG_KVM) || defined(CONFIG_KVM_MODULE)
static int __init sw64_kvm_pool_init(void)
{
	int status = 0;
	unsigned long kvm_pool_virt;
	struct page *base_page, *end_page, *p;

	if (!sw64_kvm_cma)
		goto out;

	kvm_pool_virt = (unsigned long)kvm_mem_base;

	sw64_kvm_pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!sw64_kvm_pool)
		goto out;

	status = gen_pool_add_virt(sw64_kvm_pool, kvm_pool_virt, kvm_mem_base,
			kvm_mem_size, -1);
	if (status < 0) {
		pr_err("failed to add memory chunks to sw64 kvm pool\n");
		gen_pool_destroy(sw64_kvm_pool);
		sw64_kvm_pool = NULL;
		goto out;
	}
	gen_pool_set_algo(sw64_kvm_pool, gen_pool_best_fit, NULL);

	base_page = pfn_to_page(kvm_mem_base >> PAGE_SHIFT);
	end_page  = pfn_to_page((kvm_mem_base + kvm_mem_size - 1) >> PAGE_SHIFT);

	p = base_page;
	while (p <= end_page && page_ref_count(p) == 0) {
		set_page_count(p, 1);
		page_mapcount_reset(p);
		SetPageReserved(p);
		p++;
	}

	return status;

out:
	return -ENOMEM;
}
core_initcall_sync(sw64_kvm_pool_init);
#endif
#endif
