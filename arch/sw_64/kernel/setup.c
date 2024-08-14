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
#ifdef CONFIG_MAGIC_SYSRQ
#include <linux/sysrq.h>
#include <linux/reboot.h>
#endif
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <linux/libfdt.h>
#include <linux/acpi.h>
#include <linux/cpu.h>

#include <asm/efi.h>
#include <asm/mmu_context.h>
#include <asm/sw64_init.h>
#include <asm/timer.h>
#include <asm/pci_impl.h>
#include <asm/kexec.h>

#include "proto.h"

#undef DEBUG_DISCONTIG
#ifdef DEBUG_DISCONTIG
#define DBGDCONT(args...) pr_debug(args)
#else
#define DBGDCONT(args...)
#endif

DEFINE_PER_CPU(unsigned long, hard_node_id) = { 0 };
static DEFINE_PER_CPU(struct cpu, cpu_devices);

static inline int phys_addr_valid(unsigned long addr)
{
	/*
	 * At this point memory probe has not been done such that max_pfn
	 * and other physical address variables cannot be used, so let's
	 * roughly judge physical address based on arch specific bit.
	 */
	return !(addr >> (current_cpu_data.pa_bits - 1));
}

extern struct atomic_notifier_head panic_notifier_list;
static int sw64_panic_event(struct notifier_block *, unsigned long, void *);
static struct notifier_block sw64_panic_block = {
	sw64_panic_event,
	NULL,
	INT_MAX /* try to do it first */
};

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

DEFINE_STATIC_KEY_TRUE(run_mode_host_key);
DEFINE_STATIC_KEY_FALSE(run_mode_guest_key);
DEFINE_STATIC_KEY_FALSE(run_mode_emul_key);

DEFINE_STATIC_KEY_FALSE(hw_una_enabled);
DEFINE_STATIC_KEY_FALSE(junzhang_v1_key);
DEFINE_STATIC_KEY_FALSE(junzhang_v2_key);
DEFINE_STATIC_KEY_FALSE(junzhang_v3_key);

struct socket_desc_t socket_desc[MAX_NUMSOCKETS];
int memmap_nr;
struct memmap_entry memmap_map[MAX_NUMMEMMAPS];
bool memblock_initialized;

/* boot_params */
/**
 * Keep sunway_boot_params for backward compatibility. All related code
 * will be removed when kernel no longer support C3B(xuelang).
 */
struct boot_params *sunway_boot_params = (struct boot_params *) (PARAM + 0x100);

unsigned long sunway_boot_magic;
EXPORT_SYMBOL(sunway_boot_magic);

unsigned long sunway_dtb_address;
EXPORT_SYMBOL(sunway_dtb_address);

unsigned long legacy_io_base;
unsigned long legacy_io_shift;

u64 sunway_mclk_hz;
u64 sunway_extclk_hz;

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

#ifdef CONFIG_HARDLOCKUP_DETECTOR_PERF
u64 hw_nmi_get_sample_period(int watchdog_thresh)
{
	return get_cpu_freq() * watchdog_thresh;
}
#endif

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
		case memmap_kvm:
		case memmap_crashkernel:
			/* initrd, kvm and crashkernel are handled elsewhere, skip */
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

static bool __init arch_dtb_verify(void *dt_virt, bool from_firmware)
{
	unsigned long dt_phys = __boot_pa(dt_virt);

	if (!phys_addr_valid(dt_phys)) {
		pr_crit("Invalid physical DTB address 0x%lx\n", dt_phys);
		return false;
	}

	/* Only for non built-in DTB */
	if (from_firmware &&
		(dt_phys < virt_to_phys((void *)__bss_stop)))
		pr_warn("DTB(from firmware) may have been corrupted by kernel image!\n");

	return true;
}

void early_parse_fdt_property(const void *fdt, const char *path,
		const char *prop_name, u64 *property, int size)
{
	int node, prop_len;
	const __be32 *prop;

	if (!path || !prop_name)
		return;

	node = fdt_path_offset(fdt, path);
	if (node < 0) {
		pr_err("Failed to get node [%s]\n", path);
		return;
	}

	prop = fdt_getprop(initial_boot_params, node, prop_name, &prop_len);
	if (!prop) {
		pr_err("Failed to get property [%s]\n", prop_name);
		return;
	}

	if (prop_len != size)
		pr_warn("Expect [%s] %d bytes, but %d bytes\n",
				prop_name, size, prop_len);

	*property = of_read_number(prop, size / 4);
}

bool sunway_machine_is_compatible(const char *compat)
{
	const void *fdt = initial_boot_params;
	int offset;

	offset = fdt_path_offset(fdt, "/");
	if (offset < 0)
		return false;

	return !fdt_node_check_compatible(fdt, offset, compat);
}

static void __init setup_firmware_fdt(void)
{
	void *dt_virt;
	const char *name;

	if (sunway_boot_magic != 0xDEED2024UL) {
		/* Bypass DTB from firmware if built-in DTB configured */
		if (IS_ENABLED(CONFIG_BUILTIN_DTB))
			goto cmd_handle;
		dt_virt = (void *)sunway_boot_params->dtb_start;
	} else {
		/* Use DTB provided by firmware for early initialization */
		pr_info("Parse boot params in DTB chosen node\n");
		dt_virt = (void *)sunway_dtb_address;
	}

	/* reserve the DTB from firmware in case it is used later */
	memblock_reserve(__boot_pa(dt_virt), fdt_totalsize(dt_virt));

	if (!arch_dtb_verify(dt_virt, true) ||
			!early_init_dt_scan(dt_virt)) {
		pr_crit("Invalid DTB(from firmware) at virtual address 0x%lx\n",
				(unsigned long)dt_virt);

		while (true)
			cpu_relax();
	}

	if (sunway_boot_magic == 0xDEED2024UL) {
		/* Parse MCLK(Hz) from firmware DTB */
		early_parse_fdt_property(dt_virt, "/soc/clocks/mclk",
				"clock-frequency", &sunway_mclk_hz, sizeof(u32));
		pr_info("MCLK: %llu Hz\n", sunway_mclk_hz);

		/* Parse EXTCLK(Hz) from firmware DTB */
		early_parse_fdt_property(dt_virt, "/soc/clocks/extclk",
				"clock-frequency", &sunway_extclk_hz, sizeof(u32));
		pr_info("EXTCLK: %llu Hz\n", sunway_extclk_hz);
	}

	if (sunway_machine_is_compatible("sunway,junzhang")) {
		static_branch_enable(&junzhang_v1_key);
		static_branch_disable(&junzhang_v2_key);
		static_branch_disable(&junzhang_v3_key);
	} else if (sunway_machine_is_compatible("sunway,junzhang_v2")) {
		static_branch_enable(&junzhang_v2_key);
		static_branch_disable(&junzhang_v1_key);
		static_branch_disable(&junzhang_v3_key);
	} else if (sunway_machine_is_compatible("sunway,junzhang_v3")) {
		static_branch_enable(&junzhang_v3_key);
		static_branch_disable(&junzhang_v1_key);
		static_branch_disable(&junzhang_v2_key);
	}

	name = of_flat_dt_get_machine_name();
	if (name)
		pr_info("DTB(from firmware): Machine model: %s\n", name);

cmd_handle:
	/**
	 * For C3B(xuelang), kernel command line always comes from
	 * "sunway_boot_params->cmdline". These code can be removed
	 * when no longer support C3B(xuelang).
	 */
	if (sunway_boot_magic != 0xDEED2024UL) {
		if (!sunway_boot_params->cmdline)
			sunway_boot_params->cmdline = (unsigned long)COMMAND_LINE;
		strlcpy(boot_command_line, (char *)sunway_boot_params->cmdline,
				COMMAND_LINE_SIZE);
#ifdef CONFIG_CMDLINE
#if defined(CONFIG_CMDLINE_EXTEND)
		strlcat(boot_command_line, " ", COMMAND_LINE_SIZE);
		strlcat(boot_command_line, CONFIG_CMDLINE, COMMAND_LINE_SIZE);
#elif defined(CONFIG_CMDLINE_FORCE)
		strlcpy(boot_command_line, CONFIG_CMDLINE, COMMAND_LINE_SIZE);
#else
		/* No arguments from firmware, use kernel's built-in cmdline */
		if (!((char *)boot_command_line)[0])
			strlcpy(boot_command_line, CONFIG_CMDLINE, COMMAND_LINE_SIZE);
#endif
#endif /* CONFIG_CMDLINE */
	}
}

static void __init setup_legacy_io(void)
{
	if (is_guest_or_emul()) {
		legacy_io_base = PCI_VT_LEGACY_IO;
		legacy_io_shift = 0;
		return;
	}

	if (sunway_machine_is_compatible("sunway,junzhang") ||
	    sunway_machine_is_compatible("sunway,junzhang_v2")) {
		/*
		 * Due to a hardware defect, chip junzhang and junzhang_v2 cannot
		 * recognize accesses to LPC legacy IO. The workaround is using some
		 * of the LPC MEMIO space to access Legacy IO space. Thus,
		 * legacy_io_base should be LPC_MEM_IO instead on these chips.
		 */
		legacy_io_base = LPC_MEM_IO;
		legacy_io_shift = 12;
	} else {
		legacy_io_base = LPC_LEGACY_IO;
		legacy_io_shift = 0;
	}
}

static void __init setup_builtin_fdt(void)
{
	void *dt_virt;
	const char *name;

	dt_virt = (void *)__dtb_start;
	if (!arch_dtb_verify(dt_virt, false) ||
			!early_init_dt_verify(dt_virt)) {
		pr_crit("Invalid DTB(built-in) at virtual address 0x%lx\n",
				(unsigned long)dt_virt);
		while (true)
			cpu_relax();
	}

	/* Parse {size,address}-cells */
	early_init_dt_scan_root();

	name = of_flat_dt_get_machine_name();
	if (name)
		pr_info("DTB(built-in): Machine model: %s\n", name);
}

static void __init device_tree_init(void)
{
	/**
	 * Built-in DTB is placed in init data, so we need
	 * to copy it.
	 */
	if (IS_ENABLED(CONFIG_BUILTIN_DTB))
		unflatten_and_copy_device_tree();
	else
		unflatten_device_tree();
}

#ifdef CONFIG_SUBARCH_C3B
static void __init setup_run_mode(void)
{
	if (*(unsigned long *)MM_SIZE) {
		static_branch_disable(&run_mode_host_key);
		if (*(unsigned long *)MM_SIZE & EMUL_FLAG) {
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
#elif CONFIG_SUBARCH_C4
static void __init setup_run_mode(void)
{
	if (rvpcr() >> VPCR_SHIFT) {
		pr_info("run mode: guest\n");
		static_branch_disable(&run_mode_host_key);
		static_branch_disable(&run_mode_emul_key);
		static_branch_enable(&run_mode_guest_key);
	} else if (sunway_boot_magic == 0xA2024) {
		pr_info("run mode: emul\n");
		static_branch_disable(&run_mode_host_key);
		static_branch_disable(&run_mode_guest_key);
		static_branch_enable(&run_mode_emul_key);
		sunway_boot_magic = 0xDEED2024;
	} else {
		pr_info("run mode: host\n");
		static_branch_disable(&run_mode_guest_key);
		static_branch_disable(&run_mode_emul_key);
		static_branch_enable(&run_mode_host_key);
	}

}
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
	setup_run_mode();
	setup_chip_ops();

	setup_sched_clock();

	/* Early initialization for device tree */
	setup_firmware_fdt();

	/* Now we get the final boot_command_line */
	*cmdline_p = boot_command_line;

	/* Register a call for panic conditions. */
	atomic_notifier_chain_register(&panic_notifier_list,
			&sw64_panic_block);

	callback_init();

	/*
	 * Process command-line arguments.
	 */
	parse_early_param();

	efi_init();

	/**
	 * Switch to builtin-in DTB if configured.
	 * Must be placed after efi_init(), Since
	 * efi_init() may parse boot params from DTB
	 * provided by firmware.
	 */
	if (IS_ENABLED(CONFIG_BUILTIN_DTB))
		setup_builtin_fdt();

	/* Decide legacy IO base addr based on chips */
	setup_legacy_io();

	sw64_memblock_init();

	/* Try to upgrade ACPI tables via initrd */
	acpi_table_upgrade();

	/* Parse the ACPI tables for possible boot-time configuration */
	acpi_boot_table_init();

	if (acpi_disabled)
		device_tree_init();

	setup_smp();

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
}

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

static int __init debugfs_mclk_init(void)
{
	struct dentry *dir = sw64_debugfs_dir;
	static u64 mclk_mhz, mclk_hz;

	if (!dir)
		return -ENODEV;

	if (sunway_boot_magic != 0xDEED2024UL) {
		mclk_mhz = *((unsigned char *)__va(MB_MCLK));
		mclk_hz = mclk_mhz * 1000000;
		debugfs_create_u64("mclk", 0644, dir, &mclk_mhz);
		debugfs_create_u64("mclk_hz", 0644, dir, &mclk_hz);
	} else {
		mclk_hz = sunway_mclk_hz;
		debugfs_create_u64("mclk_hz", 0644, dir, &mclk_hz);
	}

	return 0;
}
late_initcall(debugfs_mclk_init);
#endif

#ifdef CONFIG_OF
static int __init sw64_of_init(void)
{
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
	return 0;
}
core_initcall(sw64_of_init);
#endif

