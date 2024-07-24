// SPDX-License-Identifier: GPL-2.0

#include <linux/pci.h>
#include <linux/syscore_ops.h>

#include <asm/cpu.h>
#include <asm/sw64_init.h>

#define OFFSET_CORE_ONLINE		0x780UL
#define OFFSET_MC_ONLINE		0x3780UL
#define OFFSET_I2C0_SRST_L		0x1900UL
#define OFFSET_I2C1_SRST_L		0x1980UL
#define OFFSET_I2C2_SRST_L		0x1a00UL
#define OFFSET_MCU_DVC_INT_EN		0x3080UL
#define OFFSET_LONG_TIME_START_EN	0x9000UL

#define OFFSET_LONG_TIME		0x180UL

#define OFFSET_GPIO_SWPORTA_DDR		0x200UL

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
		return *(unsigned long *)MM_SIZE & MM_SIZE_MASK;

	return __get_node_mem(nodeid);
}

static void __init setup_core_map(void)
{
	int i, j, cpu_num, cpuid, max_cores_per_cpu;
	unsigned long coreonline;
	void __iomem *spbu_base;

	cpu_num = get_cpu_nums();
	cpuid = 0;
	for (i = 0; i < cpu_num; i++) {
		spbu_base = misc_platform_get_spbu_base(i);
		coreonline = readq(spbu_base + OFFSET_CORE_ONLINE);
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
	void __iomem *spbu_base = misc_platform_get_spbu_base(0);

	writeq(0x0, spbu_base + OFFSET_I2C0_SRST_L);
	writeq(0x1, spbu_base + OFFSET_I2C0_SRST_L);

	writeq(0x0, spbu_base + OFFSET_I2C1_SRST_L);
	writeq(0x1, spbu_base + OFFSET_I2C1_SRST_L);

	writeq(0x0, spbu_base + OFFSET_I2C2_SRST_L);
	writeq(0x1, spbu_base + OFFSET_I2C2_SRST_L);
}

static void pcie_save(void)
{
	struct pci_controller *hose;
	struct piu_saved *piu_save;
	unsigned long i;
	void __iomem *piu_ior0_base;
	void __iomem *piu_ior1_base;

	for (hose = hose_head; hose; hose = hose->next) {
		piu_ior0_base = hose->piu_ior0_base;
		piu_ior1_base = hose->piu_ior1_base;

		piu_save = kzalloc(sizeof(*piu_save), GFP_KERNEL);
		hose->sysdata = piu_save;

		piu_save->piuconfig0 = readq(piu_ior0_base + PIUCONFIG0);
		piu_save->piuconfig1 = readq(piu_ior1_base + PIUCONFIG1);
		piu_save->epdmabar = readq(piu_ior0_base + EPDMABAR);
		piu_save->msiaddr = readq(piu_ior0_base + MSIADDR);

		if (IS_ENABLED(CONFIG_UNCORE_XUELANG)) {
			for (i = 0; i < 256; i++) {
				piu_save->msiconfig[i] =
					readq(piu_ior0_base + MSICONFIG0 + (i << 7));
			}
		}

		piu_save->iommuexcpt_ctrl = readq(piu_ior0_base + IOMMUEXCPT_CTRL);
		piu_save->dtbaseaddr = readq(piu_ior0_base + DTBASEADDR);

		piu_save->intaconfig = readq(piu_ior0_base + INTACONFIG);
		piu_save->intbconfig = readq(piu_ior0_base + INTBCONFIG);
		piu_save->intcconfig = readq(piu_ior0_base + INTCCONFIG);
		piu_save->intdconfig = readq(piu_ior0_base + INTDCONFIG);
		piu_save->pmeintconfig = readq(piu_ior0_base + PMEINTCONFIG);
		piu_save->aererrintconfig = readq(piu_ior0_base + AERERRINTCONFIG);
		piu_save->hpintconfig = readq(piu_ior0_base + HPINTCONFIG);

	}
}

static void pcie_restore(void)
{
	struct pci_controller *hose;
	struct piu_saved *piu_save;
	u32 rc_misc_ctrl;
	unsigned int value;
	unsigned long i;
	void __iomem *rc_config_space_base;
	void __iomem *piu_ior0_base;
	void __iomem *piu_ior1_base;

	for (hose = hose_head; hose; hose = hose->next) {
		rc_config_space_base = hose->rc_config_space_base;
		piu_ior0_base = hose->piu_ior0_base;
		piu_ior1_base = hose->piu_ior1_base;
		piu_save = hose->sysdata;

		writeq(piu_save->piuconfig0, (piu_ior0_base + PIUCONFIG0));
		writeq(piu_save->piuconfig1, (piu_ior1_base + PIUCONFIG1));
		writeq(piu_save->epdmabar, (piu_ior0_base + EPDMABAR));
		writeq(piu_save->msiaddr, (piu_ior0_base + MSIADDR));


		if (IS_ENABLED(CONFIG_UNCORE_XUELANG)) {
			for (i = 0; i < 256; i++) {
				writeq(piu_save->msiconfig[i],
						(piu_ior0_base + (MSICONFIG0 + (i << 7))));
			}
		}

		writeq(piu_save->iommuexcpt_ctrl, (piu_ior0_base + IOMMUEXCPT_CTRL));
		writeq(piu_save->dtbaseaddr, (piu_ior0_base + DTBASEADDR));

		writeq(piu_save->intaconfig, (piu_ior0_base + INTACONFIG));
		writeq(piu_save->intbconfig, (piu_ior0_base + INTBCONFIG));
		writeq(piu_save->intcconfig, (piu_ior0_base + INTCCONFIG));
		writeq(piu_save->intdconfig, (piu_ior0_base + INTDCONFIG));
		writeq(piu_save->pmeintconfig, (piu_ior0_base + PMEINTCONFIG));
		writeq(piu_save->aererrintconfig, (piu_ior0_base + AERERRINTCONFIG));
		writeq(piu_save->hpintconfig, (piu_ior0_base + HPINTCONFIG));

		/* Enable DBI_RO_WR_EN */
		rc_misc_ctrl = readl(rc_config_space_base + RC_MISC_CONTROL_1);
		writel((rc_misc_ctrl | 0x1), (rc_config_space_base + RC_MISC_CONTROL_1));

		/* Fix up DEVICE_ID_VENDOR_ID register */
		value = (PCI_DEVICE_ID_SW64_ROOT_BRIDGE << 16) | PCI_VENDOR_ID_JN;
		writel(value, (rc_config_space_base + RC_VENDOR_ID));

		/* Set PCI-E root class code */
		value = readl(rc_config_space_base + RC_REVISION_ID);
		writel((PCI_CLASS_BRIDGE_HOST << 16) | value, (rc_config_space_base + RC_REVISION_ID));

		/* Disable DBI_RO_WR_EN */
		writel(rc_misc_ctrl, (rc_config_space_base + RC_MISC_CONTROL_1));
	}

}

static unsigned long saved_dvc_int, saved_long_time;

static inline void intpu_save(void)
{
	void __iomem *intpu_base = misc_platform_get_intpu_base(0);

	switch (current_cpu_data.model) {
	case CPU_SW831:
		saved_long_time = readq(intpu_base + OFFSET_LONG_TIME);
	default:
		break;
	}
}

static inline void intpu_restore(void)
{
	void __iomem *intpu_base = misc_platform_get_intpu_base(0);
	void __iomem *spbu_base = misc_platform_get_spbu_base(0);
	void __iomem *gpio_base = misc_platform_get_gpio_base(0);

	switch (current_cpu_data.model) {
	case CPU_SW831:
		writeq(saved_long_time, intpu_base + OFFSET_LONG_TIME);
		writeq(0x1, spbu_base + OFFSET_LONG_TIME_START_EN);
		break;
	case CPU_SW8A:
		writeq(0x1, gpio_base + OFFSET_GPIO_SWPORTA_DDR);
		break;
	default:
		pr_info("long time start is disable!");
		break;
	}
}

static inline void spbu_save(void)
{
	void __iomem *spbu_base = misc_platform_get_spbu_base(0);

	saved_dvc_int = readq(spbu_base + OFFSET_MCU_DVC_INT_EN);
}

static inline void spbu_restore(void)
{
	void __iomem *spbu_base = misc_platform_get_spbu_base(0);

	i2c_srst();
	writeq(saved_dvc_int, spbu_base + OFFSET_MCU_DVC_INT_EN);
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
