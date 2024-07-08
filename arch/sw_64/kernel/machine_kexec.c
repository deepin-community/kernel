// SPDX-License-Identifier: GPL-2.0
/*
 * machine_kexec.c for kexec
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */
#include <linux/kexec.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/reboot.h>
#include <linux/libfdt.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/efi.h>
#include <linux/memblock.h>

#include <asm/cacheflush.h>
#include <asm/platform.h>

extern const unsigned char relocate_new_kernel[];
extern const size_t relocate_new_kernel_size;

extern unsigned long kexec_start_address;
extern unsigned long kexec_indirection_page;

static atomic_t waiting_for_crash_ipi;
static void *kexec_control_page;

#ifdef CONFIG_SMP
extern struct smp_rcb_struct *smp_rcb;

/*
 * Wait for relocation code is prepared and send
 * secondary CPUs to spin until kernel is relocated.
 */
static void kexec_smp_down(void *ignored)
{
	int cpu = smp_processor_id();

	local_irq_disable();
	while (READ_ONCE(smp_rcb->ready) != 0)
		mdelay(1);
	set_cpu_online(cpu, false);
	reset_cpu(cpu);
}
#endif

#define KTEXT_MAX	KERNEL_IMAGE_SIZE

void __init kexec_control_page_init(void)
{
	phys_addr_t addr;

	addr = memblock_phys_alloc_range(KEXEC_CONTROL_PAGE_SIZE, PAGE_SIZE,
					0, 0);
	kexec_control_page = (void *)(__START_KERNEL_map + addr);
}

/*
 * reserve_crashkernel() - reserves memory are for crash kernel
 *
 * This function reserves memory area given in "crashkernel=" kernel command
 * line parameter. The memory reserved is used by a dump capture kernel when
 * primary kernel is crashing.
 */
void __init reserve_crashkernel(void)
{
	unsigned long long crash_size, crash_base;
	unsigned long long mem_size = memblock_phys_mem_size();
	int ret;

	ret = parse_crashkernel(boot_command_line, mem_size,
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
			(unsigned long)(mem_size >> 20));

	ret = add_memmap_region(crash_base, crash_size, memmap_crashkernel);
	if (ret)
		pr_warn("Add crash kernel area [mem %#018llx-%#018llx] to memmap region failed.\n",
				crash_base, crash_base + crash_size - 1);

	if (crash_base < PCI_LEGACY_IO_SIZE)
		pr_warn("Crash base should be greater than or equal to %#lx\n", PCI_LEGACY_IO_SIZE);

	crashk_res.start = crash_base;
	crashk_res.end = crash_base + crash_size - 1;
	insert_resource(&iomem_resource, &crashk_res);
}

int machine_kexec_prepare(struct kimage *kimage)
{
	return 0;
}

void machine_kexec_cleanup(struct kimage *kimage)
{
}

void machine_shutdown(void)
{
#ifdef CONFIG_SMP
	WRITE_ONCE(smp_rcb->ready, 0);
	smp_call_function(kexec_smp_down, NULL, 0);
	smp_wmb();
	while (num_online_cpus() > 1) {
		cpu_relax();
		mdelay(1);
	}
#endif
}

#ifdef CONFIG_SMP
static void machine_crash_nonpanic_core(void *unused)
{
	int cpu;
	struct pt_regs regs;

	cpu = smp_processor_id();

	local_irq_disable();
	crash_setup_regs(&regs, NULL);
	pr_debug("CPU %u will stop doing anything useful since another CPU has crashed\n", cpu);
	crash_save_cpu(&regs, cpu);
	flush_cache_all();

	set_cpu_online(cpu, false);
	atomic_dec(&waiting_for_crash_ipi);
	while (READ_ONCE(smp_rcb->ready) != 0)
		mdelay(1);
	if (cpu != 0)
		reset_cpu(cpu);
	else
		machine_kexec(kexec_crash_image);
}
#else
static inline void machine_crash_nonpanic_core(void *unused) { }
#endif

static void machine_kexec_mask_interrupts(void)
{
	unsigned int i;
	struct irq_desc *desc;

	for_each_irq_desc(i, desc) {
		struct irq_chip *chip;

		chip = irq_desc_get_chip(desc);
		if (!chip)
			continue;

		if (chip->irq_eoi && irqd_irq_inprogress(&desc->irq_data))
			chip->irq_eoi(&desc->irq_data);

		if (chip->irq_mask)
			chip->irq_mask(&desc->irq_data);

		if (chip->irq_disable && !irqd_irq_disabled(&desc->irq_data))
			chip->irq_disable(&desc->irq_data);
	}
}

void machine_crash_shutdown(struct pt_regs *regs)
{
	int cpu;
	unsigned long msecs;

	cpu = smp_processor_id();
	local_irq_disable();
	atomic_set(&waiting_for_crash_ipi, num_online_cpus() - 1);
	smp_call_function(machine_crash_nonpanic_core, NULL, false);
	msecs = 1000; /* Wait at most a second for the other cpus to stop */
	while ((atomic_read(&waiting_for_crash_ipi) > 0) && msecs) {
		mdelay(1);
		msecs--;
	}
	if (atomic_read(&waiting_for_crash_ipi) > 0)
		pr_warn("Non-crashing CPUs did not react to IPI\n");

	crash_save_cpu(regs, cpu);
	machine_kexec_mask_interrupts();
	pr_info("Loading crashdump kernel...\n");
#ifdef CONFIG_SMP
	WRITE_ONCE(smp_rcb->ready, 0);
	if (cpu != 0)
		reset_cpu(cpu);
#endif
}

#define phys_to_ktext(pa)    (__START_KERNEL_map + (pa))

typedef void (*noretfun_t)(unsigned long, unsigned long) __noreturn;

/**
 * Current kernel does not yet have a common implementation for
 * this function. So, make an arch-specific one.
 */
static void *arch_kexec_alloc_and_setup_fdt(unsigned long initrd_start,
		unsigned long initrd_size, const char *cmdline)
{
	void *fdt;
	int ret, chosen_node;
	size_t fdt_size;

	fdt_size = fdt_totalsize(initial_boot_params) +
		(cmdline ? strlen(cmdline) : 0) + 0x1000;
	fdt = kzalloc(fdt_size, GFP_KERNEL);
	if (!fdt)
		return NULL;

	ret = fdt_open_into(initial_boot_params, fdt, fdt_size);
	if (ret < 0) {
		pr_err("Error %d setting up the new device tree\n", ret);
		goto out;
	}

	chosen_node = fdt_path_offset(fdt, "/chosen");
	if (chosen_node < 0) {
		pr_err("Failed to find chosen node\n");
		goto out;
	}

	/* update initrd params */
	if (initrd_size) {
		ret = fdt_setprop_u64(fdt, chosen_node, "linux,initrd-start",
				initrd_start);
		if (ret)
			goto out;

		ret = fdt_setprop_u64(fdt, chosen_node, "linux,initrd-end",
				initrd_start + initrd_size);
		if (ret)
			goto out;
	} else {
		ret = fdt_delprop(fdt, chosen_node, "linux,initrd-start");
		if (ret)
			goto out;

		ret = fdt_delprop(fdt, chosen_node, "linux,initrd-end");
		if (ret)
			goto out;
	}

	/* update cmdline */
	if (cmdline) {
		ret = fdt_setprop_string(fdt, chosen_node, "bootargs", cmdline);
		if (ret)
			goto out;
	} else {
		ret = fdt_delprop(fdt, chosen_node, "bootargs");
		if (ret)
			goto out;
	}

	return fdt;

out:
	kfree(fdt);
	return NULL;
}

static void update_boot_params(void)
{
	struct boot_params params = { 0 };

	/* Cmdline and initrd can be new */
	params.cmdline = kexec_start_address - COMMAND_LINE_OFF;
	params.initrd_start = *(__u64 *)(kexec_start_address - INITRD_START_OFF);
	params.initrd_size = *(__u64 *)(kexec_start_address - INITRD_SIZE_OFF);

	if (sunway_boot_magic != 0xDEED2024UL) {
		sunway_boot_params->cmdline = params.cmdline;
		sunway_boot_params->initrd_start = params.initrd_start;
		sunway_boot_params->initrd_size = params.initrd_size;

		params.dtb_start = sunway_boot_params->dtb_start;
		params.efi_systab = sunway_boot_params->efi_systab;
		params.efi_memmap = sunway_boot_params->efi_memmap;
		params.efi_memmap_size = sunway_boot_params->efi_memmap_size;
		params.efi_memdesc_size = sunway_boot_params->efi_memdesc_size;
		params.efi_memdesc_version = sunway_boot_params->efi_memdesc_version;
	} else {
		params.dtb_start = (unsigned long)arch_kexec_alloc_and_setup_fdt(
				params.initrd_start, params.initrd_size,
				(const char *)params.cmdline);

#ifdef CONFIG_EFI
		early_parse_fdt_property((void *)sunway_dtb_address, "/chosen",
			"linux,uefi-system-table", &params.efi_systab, sizeof(u64));
		params.efi_memmap = efi.memmap.phys_map;
		params.efi_memmap_size = efi.memmap.map_end - efi.memmap.map;
		params.efi_memdesc_size = efi.memmap.desc_size;
		params.efi_memdesc_version = efi.memmap.desc_version;
#endif
		/* update dtb base address */
		sunway_dtb_address = params.dtb_start;
	}

	pr_info("initrd_start     = %#llx, initrd_size         = %#llx\n"
		"dtb_start        = %#llx, efi_systab          = %#llx\n"
		"efi_memmap       = %#llx, efi_memmap_size     = %#llx\n"
		"efi_memdesc_size = %#llx, efi_memdesc_version = %#llx\n"
		"cmdline          = %s\n",
		params.initrd_start, params.initrd_size,
		params.dtb_start, params.efi_systab,
		params.efi_memmap, params.efi_memmap_size,
		params.efi_memdesc_size, params.efi_memdesc_version,
		(char *)params.cmdline);
}

void machine_kexec(struct kimage *image)
{
	void *reboot_code_buffer;
	unsigned long entry;
	unsigned long *ptr;

	reboot_code_buffer = kexec_control_page;
	pr_info("reboot_code_buffer = %px\n", reboot_code_buffer);
	kexec_start_address = phys_to_ktext(image->start);
	pr_info("kexec_start_address = %#lx\n", kexec_start_address);
	if (image->type == KEXEC_TYPE_DEFAULT)
		kexec_indirection_page =
			(unsigned long) phys_to_virt(image->head & PAGE_MASK);
	else
		kexec_indirection_page = (unsigned long)&image->head;

	pr_info("kexec_indirection_page = %#lx, image->head=%#lx\n",
			kexec_indirection_page, image->head);

	update_boot_params();

	memcpy(reboot_code_buffer, relocate_new_kernel, relocate_new_kernel_size);

	/*
	 * The generic kexec code builds a page list with physical
	 * addresses. they are directly accessible through KSEG0 (or
	 * CKSEG0 or XPHYS if on 64bit system), hence the
	 * phys_to_virt() call.
	 */
	for (ptr = &image->head; (entry = *ptr) && !(entry & IND_DONE);
	     ptr = (entry & IND_INDIRECTION) ?
	       phys_to_virt(entry & PAGE_MASK) : ptr + 1) {
		if (*ptr & IND_SOURCE || *ptr & IND_INDIRECTION ||
		    *ptr & IND_DESTINATION)
			*ptr = (unsigned long) phys_to_virt(*ptr);
	}

	/*
	 * we do not want to be bothered.
	 */
	local_irq_disable();

	pr_info("Will call new kernel at %08lx\n", image->start);
	pr_info("Bye ...\n");
	smp_wmb();
	((noretfun_t) reboot_code_buffer)(sunway_boot_magic,
		sunway_dtb_address);
}
