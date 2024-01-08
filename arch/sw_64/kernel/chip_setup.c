// SPDX-License-Identifier: GPL-2.0

#include <linux/pci.h>
#include <linux/syscore_ops.h>

#include <asm/hw_init.h>
#include <asm/sw64_init.h>

struct sw64_chip_ops *sw64_chip;
struct sw64_chip_init_ops *sw64_chip_init;

static int get_cpu_nums(void)
{
	if (is_guest_or_emul())
		return 1;

	return __get_cpu_nums();
}

static unsigned long __init get_node_mem(int nodeid)
{

	if (is_guest_or_emul())
		return *(unsigned long *)MMSIZE & MMSIZE_MASK;

	return __get_node_mem(nodeid);
}

static void __init setup_core_map(struct cpumask *cpumask)
{
	int i, j, cpu_num, cpuid, max_cores_per_cpu;
	unsigned long coreonline;

	cpu_num = get_cpu_nums();
	cpuid = 0;
	for (i = 0; i < cpu_num; i++) {
		coreonline = sw64_io_read(i, CORE_ONLINE);
		max_cores_per_cpu = MAX_CORES_PER_CPU;

		if (is_guest_or_emul())
			max_cores_per_cpu = 64;

		for (j = 0; j < max_cores_per_cpu; j++) {
			if (coreonline & (1UL << j)) {
				__cpu_to_rcid[cpuid] = (i << DOMAIN_ID_SHIFT) | (j << CORE_ID_SHIFT);
				cpuid++;
			}
		}
	}

	if (is_in_host() && core_is_ht()) {
		for (i = 0; i < cpuid; i++)
			__cpu_to_rcid[cpuid + i] = __cpu_to_rcid[i] | (1 << THREAD_ID_SHIFT);

		cpuid = cpuid + i;
	}

	while (cpuid < NR_CPUS) {
		__cpu_to_rcid[cpuid] = -1;
		cpuid++;
	}
}

#ifdef CONFIG_PM
static void i2c_srst(void)
{
	sw64_io_write(0, I2C0_SRST_L, 0x0);
	sw64_io_write(0, I2C0_SRST_L, 0x1);

	sw64_io_write(0, I2C1_SRST_L, 0x0);
	sw64_io_write(0, I2C1_SRST_L, 0x1);

	sw64_io_write(0, I2C2_SRST_L, 0x0);
	sw64_io_write(0, I2C2_SRST_L, 0x1);
}

static void pcie_save(void)
{
	struct pci_controller *hose;
	struct piu_saved *piu_save;
	unsigned long node, index;
	unsigned long i;

	for (hose = hose_head; hose; hose = hose->next) {
		piu_save = kzalloc(sizeof(*piu_save), GFP_KERNEL);

		node = hose->node;
		index = hose->index;
		hose->sysdata = piu_save;

		piu_save->piuconfig0 = read_piu_ior0(node, index, PIUCONFIG0);
		piu_save->piuconfig1 = read_piu_ior1(node, index, PIUCONFIG1);
		piu_save->epdmabar = read_piu_ior0(node, index, EPDMABAR);
		piu_save->msiaddr = read_piu_ior0(node, index, MSIADDR);

		if (IS_ENABLED(CONFIG_UNCORE_XUELANG)) {
			for (i = 0; i < 256; i++) {
				piu_save->msiconfig[i] = read_piu_ior0(node, index,
						MSICONFIG0 + (i << 7));
			}
		}

		piu_save->iommuexcpt_ctrl = read_piu_ior0(node, index, IOMMUEXCPT_CTRL);
		piu_save->dtbaseaddr = read_piu_ior0(node, index, DTBASEADDR);

		piu_save->intaconfig = read_piu_ior0(node, index, INTACONFIG);
		piu_save->intbconfig = read_piu_ior0(node, index, INTBCONFIG);
		piu_save->intcconfig = read_piu_ior0(node, index, INTCCONFIG);
		piu_save->intdconfig = read_piu_ior0(node, index, INTDCONFIG);
		piu_save->pmeintconfig = read_piu_ior0(node, index, PMEINTCONFIG);
		piu_save->aererrintconfig = read_piu_ior0(node, index, AERERRINTCONFIG);
		piu_save->hpintconfig = read_piu_ior0(node, index, HPINTCONFIG);

	}
}

static void pcie_restore(void)
{
	struct pci_controller *hose;
	struct piu_saved *piu_save;
	unsigned long node, index;
	u32 rc_misc_ctrl;
	unsigned int value;
	unsigned long i;

	for (hose = hose_head; hose; hose = hose->next) {
		node = hose->node;
		index = hose->index;
		piu_save = hose->sysdata;

		write_piu_ior0(node, index, PIUCONFIG0, piu_save->piuconfig0);
		write_piu_ior1(node, index, PIUCONFIG1, piu_save->piuconfig1);
		write_piu_ior0(node, index, EPDMABAR, piu_save->epdmabar);
		write_piu_ior0(node, index, MSIADDR, piu_save->msiaddr);

		if (IS_ENABLED(CONFIG_UNCORE_XUELANG)) {
			for (i = 0; i < 256; i++) {
				write_piu_ior0(node, index, MSICONFIG0 + (i << 7),
						piu_save->msiconfig[i]);
			}
		}

		write_piu_ior0(node, index, IOMMUEXCPT_CTRL, piu_save->iommuexcpt_ctrl);
		write_piu_ior0(node, index, DTBASEADDR, piu_save->dtbaseaddr);

		write_piu_ior0(node, index, INTACONFIG, piu_save->intaconfig);
		write_piu_ior0(node, index, INTBCONFIG, piu_save->intbconfig);
		write_piu_ior0(node, index, INTCCONFIG, piu_save->intcconfig);
		write_piu_ior0(node, index, INTDCONFIG, piu_save->intdconfig);
		write_piu_ior0(node, index, PMEINTCONFIG, piu_save->pmeintconfig);
		write_piu_ior0(node, index, AERERRINTCONFIG, piu_save->aererrintconfig);
		write_piu_ior0(node, index, HPINTCONFIG, piu_save->hpintconfig);

		/* Enable DBI_RO_WR_EN */
		rc_misc_ctrl = read_rc_conf(node, index, RC_MISC_CONTROL_1);
		write_rc_conf(node, index, RC_MISC_CONTROL_1, rc_misc_ctrl | 0x1);

		/* Fix up DEVICE_ID_VENDOR_ID register */
		value = (PCI_DEVICE_ID_SW64_ROOT_BRIDGE << 16) | PCI_VENDOR_ID_JN;
		write_rc_conf(node, index, RC_VENDOR_ID, value);

		/* Set PCI-E root class code */
		value = read_rc_conf(node, index, RC_REVISION_ID);
		write_rc_conf(node, index, RC_REVISION_ID, (PCI_CLASS_BRIDGE_HOST << 16) | value);

		/* Disable DBI_RO_WR_EN */
		write_rc_conf(node, index, RC_MISC_CONTROL_1, rc_misc_ctrl);
	}

}

static unsigned long saved_dvc_int, saved_long_time;

static inline void intpu_save(void)
{
	switch (cpu_desc.model) {
	case CPU_SW831:
		saved_long_time = __io_read_longtime(0);
	default:
		break;
	}
}

static inline void intpu_restore(void)
{
	switch (cpu_desc.model) {
	case CPU_SW831:
		__io_write_longtime(0, saved_long_time);
		__io_write_longtime_start_en(0, 0x1);
		break;
	default:
		pr_info("long time start is disable!");
		break;
	}
}

static inline void spbu_save(void)
{
	saved_dvc_int = sw64_io_read(0, MCU_DVC_INT_EN);
}

static inline void spbu_restore(void)
{
	i2c_srst();
	sw64_io_write(0, MCU_DVC_INT_EN, saved_dvc_int);
}

static int io_suspend(void)
{
	spbu_save();
	intpu_save();
	pcie_save();

	return 0;
}

static void io_resume(void)
{
	pcie_restore();
	intpu_restore();
	spbu_restore();
}
#endif /* CONFIG_PM */

static struct sw64_chip_init_ops chip_init_ops = {
	.early_init = {
		.setup_core_map = setup_core_map,
		.get_node_mem = get_node_mem,
	},
};

static struct sw64_chip_ops chip_ops = {
	.get_cpu_num = get_cpu_nums,
};

void __init setup_chip_ops(void)
{
	sw64_chip_init = &chip_init_ops;
	sw64_chip = &chip_ops;
	setup_chip_pci_ops();
#ifdef CONFIG_PM
	io_syscore_ops.suspend = io_suspend;
	io_syscore_ops.resume = io_resume;
#endif
}
