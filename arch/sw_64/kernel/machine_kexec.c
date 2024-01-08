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

#include <asm/cacheflush.h>

extern void *kexec_control_page;
extern const unsigned char relocate_new_kernel[];
extern const size_t relocate_new_kernel_size;

extern unsigned long kexec_start_address;
extern unsigned long kexec_indirection_page;

static atomic_t waiting_for_crash_ipi;

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
	kernel_restart_prepare(NULL);
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

typedef void (*noretfun_t)(void) __noreturn;

void machine_kexec(struct kimage *image)
{
	void *reboot_code_buffer;
	unsigned long entry;
	unsigned long *ptr;
	struct boot_params *params = sunway_boot_params;


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

	params->cmdline = kexec_start_address - COMMAND_LINE_OFF;
	params->initrd_start = *(__u64 *)(kexec_start_address - INITRD_START_OFF);
	params->initrd_size = *(__u64 *)(kexec_start_address - INITRD_SIZE_OFF);

	pr_info("initrd_start = %#llx, initrd_size = %#llx\n"
		"dtb_start = %#llx, efi_systab = %#llx\n"
		"efi_memmap = %#llx, efi_memmap_size = %#llx\n"
		"efi_memdesc_size = %#llx, efi_memdesc_version = %#llx\n"
		"cmdline = %#llx\n",
		params->initrd_start, params->initrd_size,
		params->dtb_start, params->efi_systab,
		params->efi_memmap, params->efi_memmap_size,
		params->efi_memdesc_size, params->efi_memdesc_version,
		params->cmdline);

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
	((noretfun_t) reboot_code_buffer)();
}
