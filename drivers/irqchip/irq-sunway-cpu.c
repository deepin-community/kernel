// SPDX-License-Identifier: GPL-2.0

#include <linux/kconfig.h>
#include <linux/pci.h>

#include <asm/hw_init.h>
#include <asm/irq_impl.h>
#include <asm/pmc.h>
#include <asm/sw64_init.h>

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
		generic_handle_domain_irq(NULL, hwirq);
		stat &= ~(1UL << hwirq);
	}
	/*do handle irq */

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
		old_regs = set_irq_regs(regs);
		handle_dev_int(regs);
		set_irq_regs(old_regs);
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
