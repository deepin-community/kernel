// SPDX-License-Identifier: GPL-2.0
#include <linux/irqdomain.h>
#include <linux/irqchip.h>
#include <linux/acpi.h>
#include <linux/acpi_iort.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/topology.h>

#include <asm/platform.h>

/**
 * Currently, Peripheral interrupt control logic of Sunway is mainly
 * distributed on the device side, which are hardware entities
 * corresponding to SW sub PINTC structures.
 *
 * At the same time, there are some interrupt configuration registers
 * concentrated in INTPU, which is hardware entity corresponding to
 * SW PINTC, excluding SW sub PINTC structures.
 *
 * The topology of SW PINTC(physical) is as follows:
 *
 * +----------------------------------------------------------------+
 * |                                                                |
 * |                 +--------------------------------+             |
 * |                 |            SW CINTC            |             |
 * |                 +--------------------------------+             |
 * |                    |        |        |        |                |
 * |           _________|      __|        |__      |______          |
 * |          |               |              |            |         |
 * |  +-------|---------------|--------------|------------|------+  |
 * |  |       |               |              |            |      |  |
 * |  |  +----------+    +--------+     +--------+    +--------+ |  |
 * |  |  |   MCU    |    |   MT   |     |  ADR   |    | ...... | |  |
 * |  |  +----------+    +--------+     +--------+    +--------+ |  |
 * |  |       |                 SW PINTC                         |  |
 * |  +-------|--------------------------------------------------+  |
 * |          |                                                     |
 * |   +--------------+                                             |
 * |   |  SW LPC INTC |                                             |
 * |   +--------------+                                             |
 * |                                                                |
 * +----------------------------------------------------------------+
 */

#define PREFIX  "PINTC: "

#define OFFSET_DLI_RLTD_FAULT_INTEN  0xa80UL
#define OFFSET_MCU_DVC_INT           0x3000UL
#define OFFSET_MCU_DVC_INT_EN        0x3080UL
#define OFFSET_SI_FAULT_STAT         0x3100UL
#define OFFSET_SI_FAULT_INT_EN       0x3200UL
#define OFFSET_ADR_CTL               0x3600UL /* PINTC version >= 2 */
#define OFFSET_DUAL_CG0_FAULT_INTEN  0x7580UL /* PINTC version 1 only */
#define OFFSET_DUAL_CG1_FAULT_INTEN  0x7600UL /* PINTC version 1 only */
#define OFFSET_DUAL_CG2_FAULT_INTEN  0x7680UL /* PINTC version 1 only */
#define OFFSET_DUAL_CG3_FAULT_INTEN  0x7700UL /* PINTC version 1 only */
#define OFFSET_DUAL_CG4_FAULT_INTEN  0x7780UL /* PINTC version 1 only */
#define OFFSET_DUAL_CG5_FAULT_INTEN  0x7800UL /* PINTC version 1 only */
#define OFFSET_DUAL_CG6_FAULT_INTEN  0x7880UL /* PINTC version 1 only */
#define OFFSET_DUAL_CG7_FAULT_INTEN  0x7900UL /* PINTC version 1 only */

#define OFFSET_DEV_INT_CONFIG        0x480UL
#define OFFSET_FAULT_INT_CONFIG      0x780UL
#define OFFSET_DEVINT_WKEN           0x1500UL
#define OFFSET_ADR_INT_CONFIG        0x1580UL /* PINTC version >= 2 */
#define OFFSET_DEVINTWK_INTEN        0x1600UL

#define SW_PINTC_MCU_GSI_BASE  64

#define INTPU_BASE_V1 0x802a00000000
#define INTPU_SIZE_V1 0x1680

#define MCU_BASE_V1   0x803000000000
#define MCU_SIZE_V1   0x8f00

DECLARE_PER_CPU(unsigned long, hard_node_id);

struct pintc_chip_data {
	bool vt;                  /* virtual pintc */
	u32 node;                 /* node ID */
	u32 version;              /* PINTC version */
	void __iomem *pintc_base; /* INTPU base address */
	void __iomem *mcu_base;   /* MCU/SPBU base address */
	struct irq_chip *mcu_chip;
	u32 mcu_irq_num;
	raw_spinlock_t pintc_lock;
	raw_spinlock_t mcu_lock;
};

static struct pintc_chip_data *chip_datas[MAX_NUMNODES];

static struct pintc_chip_data *pintc_alloc_chip_data(u32 node)
{
	struct pintc_chip_data *chip_data;

	if (WARN_ON(node >= MAX_NUMNODES))
		return NULL;

	chip_data = kzalloc_node(sizeof(struct pintc_chip_data),
			GFP_KERNEL, node);

	chip_datas[node] = chip_data;

	return chip_data;
}

static void pintc_free_chip_data(struct pintc_chip_data *chip_data)
{
	if (!chip_data)
		return;

	if (WARN_ON((chip_data->node >= MAX_NUMNODES) ||
		(chip_datas[chip_data->node] != chip_data)))
		return;

	chip_datas[chip_data->node] = NULL;
	kfree(chip_data);
}

static void mcu_irq_disable(struct irq_data *data)
{
	struct pintc_chip_data *chip_data = data->chip_data;
	unsigned long mask, flags;
	int hwirq = data->hwirq;

	raw_spin_lock_irqsave(&chip_data->mcu_lock, flags);

	mask = readq(chip_data->mcu_base + OFFSET_MCU_DVC_INT_EN);
	mask &= ~(0x1UL << hwirq);
	writeq(mask, chip_data->mcu_base + OFFSET_MCU_DVC_INT_EN);

	raw_spin_unlock_irqrestore(&chip_data->mcu_lock, flags);
}

static void mcu_irq_enable(struct irq_data *data)
{
	struct pintc_chip_data *chip_data = data->chip_data;
	unsigned long mask, flags;
	int hwirq = data->hwirq;

	raw_spin_lock_irqsave(&chip_data->mcu_lock, flags);

	mask = readq(chip_data->mcu_base + OFFSET_MCU_DVC_INT_EN);
	mask |= (0x1UL << hwirq);
	writeq(mask, chip_data->mcu_base + OFFSET_MCU_DVC_INT_EN);

	raw_spin_unlock_irqrestore(&chip_data->mcu_lock, flags);
}

static void pintc_mcu_enable(void __iomem *pintc_base)
{
	unsigned long devint_conf;

	devint_conf = readq(pintc_base + OFFSET_DEV_INT_CONFIG);
	devint_conf |= (1UL << 8);
	writeq(devint_conf, pintc_base + OFFSET_DEV_INT_CONFIG);
}

static void pintc_mcu_disable(void __iomem *pintc_base)
{
	unsigned long devint_conf;

	devint_conf = readq(pintc_base + OFFSET_DEV_INT_CONFIG);
	devint_conf &= ~(1UL << 8);
	writeq(devint_conf, pintc_base + OFFSET_DEV_INT_CONFIG);
}

static unsigned long
pintc_mcu_disable_and_save(struct pintc_chip_data *chip_data)
{
	unsigned long val;

	raw_spin_lock(&chip_data->pintc_lock);

	val = readq(chip_data->pintc_base + OFFSET_DEV_INT_CONFIG);
	pintc_mcu_disable(chip_data->pintc_base);

	raw_spin_unlock(&chip_data->pintc_lock);

	return val & (1UL << 8);
}

static void
pintc_mcu_restore(struct pintc_chip_data *chip_data, unsigned long val)
{
	unsigned long current_val;

	raw_spin_lock(&chip_data->pintc_lock);

	current_val = readq(chip_data->pintc_base + OFFSET_DEV_INT_CONFIG);
	current_val &= ~(1UL << 8);
	current_val |= val;
	writeq(current_val, chip_data->pintc_base + OFFSET_DEV_INT_CONFIG);

	raw_spin_unlock(&chip_data->pintc_lock);
}

static unsigned long make_pintc_int_target(u32 version, int rcid)
{
	int node, core, thread;
	unsigned long target = 0;

	thread = rcid_to_thread_id(rcid);
	core = rcid_to_core_id(rcid);
	node = rcid_to_domain_id(rcid);

	switch (version) {
	case 0x1: /* PINTC v1 */
		target = core | (thread << 5) | (node << 6);
		break;
	case 0x2: /* PINTC v2 */
	case 0x3: /* PINTC v3 */
		target = core | (thread << 6) | (node << 7);
		break;
	default:
		break;
	}

	return target;
}

static void update_pintc_mcu_target(struct pintc_chip_data *chip_data,
		unsigned long target)
{
	unsigned long val, flags;

	raw_spin_lock_irqsave(&chip_data->pintc_lock, flags);

	val = readq(chip_data->pintc_base + OFFSET_DEV_INT_CONFIG);

	/* Disable MCU irqs until affinity setting is completed */
	pintc_mcu_disable(chip_data->pintc_base);

	val &= 0xffff;
	val |= (target << 16);

	writeq(val, chip_data->pintc_base + OFFSET_DEV_INT_CONFIG);

	raw_spin_unlock_irqrestore(&chip_data->pintc_lock, flags);
}

static int assign_mcu_irq_config(struct pintc_chip_data *chip_data,
		cpumask_t *targets)
{
	unsigned long dev_int_tar;
	unsigned int cpu;
	int rcid;

	for_each_cpu(cpu, targets) {
		/*
		 * Hardware requires dev ints be redirected to on-node
		 * cores only. Thus, we remove all off-node cpu in the
		 * target mask.
		 */
		if (cpu_to_node(cpu) != chip_data->node)
			cpumask_clear_cpu(cpu, targets);
	}

	/* Use the last one in valid cpus to avoid core 0. */
	cpu = cpumask_last(targets);
	if (cpu >= nr_cpu_ids)
		return -EPERM;

	rcid = cpu_to_rcid(cpu);

	dev_int_tar = make_pintc_int_target(chip_data->version, rcid);
	update_pintc_mcu_target(chip_data, dev_int_tar);

	return 0;
}

static int mcu_irq_set_affinity(struct irq_data *irq_data,
				 const struct cpumask *dest, bool force)
{
	struct pintc_chip_data *chip_data = irq_data->chip_data;
	cpumask_t targets;

	if (cpumask_any_and(dest, cpu_online_mask) >= nr_cpu_ids)
		return -EINVAL;

	cpumask_and(&targets, dest, cpu_online_mask);

	return assign_mcu_irq_config(chip_data, &targets);
}

static struct irq_chip pintc_mcu_chip = {
	.name			= "MCU-INT",
	.irq_enable		= mcu_irq_enable,
	.irq_disable		= mcu_irq_disable,
	.irq_mask		= mcu_irq_disable,
	.irq_unmask		= mcu_irq_enable,
	.irq_set_affinity	= mcu_irq_set_affinity,
};

static struct irq_chip pintc_mcu_vt_chip = {
	.name                   = "VMCU-INT",
};

static int pintc_mcu_translate(struct irq_domain *domain,
		struct irq_fwspec *fwspec,
		unsigned long *hwirq,
		unsigned int *type)
{
	if (WARN_ON(fwspec->param_count < 1))
		return -EINVAL;

	/* Device tree */
	if (is_of_node(fwspec->fwnode)) {
		*hwirq = fwspec->param[0];
		*type = IRQ_TYPE_NONE;
		return 0;
	}

	/* ACPI */
	if (is_fwnode_irqchip(fwspec->fwnode)) {
		if (WARN_ON(fwspec->param[0] < SW_PINTC_MCU_GSI_BASE))
			return -EINVAL;
		*hwirq = fwspec->param[0] - SW_PINTC_MCU_GSI_BASE;
		*type = IRQ_TYPE_NONE;
		return 0;
	}

	return -EINVAL;
}

static void pintc_mcu_free_irqs(struct irq_domain *irq_domain,
				unsigned int virq, unsigned int nr_irqs)
{
	int i = 0;

	irq_domain_free_irqs_top(irq_domain, virq, nr_irqs);

	for (i = 0; i < nr_irqs; i++)
		irq_clear_status_flags(virq + i, IRQ_LEVEL);
}

static int pintc_mcu_map_irq(struct irq_domain *domain,
		unsigned int virq, irq_hw_number_t hwirq)
{
	struct pintc_chip_data *chip_data = domain->host_data;

	irq_domain_set_info(domain, virq, hwirq,
			chip_data->mcu_chip, chip_data,
			handle_level_irq, NULL, NULL);
	irq_set_status_flags(virq, IRQ_LEVEL);

	return 0;
}

static int pintc_mcu_alloc_irqs(struct irq_domain *domain,
				unsigned int virq,
				unsigned int nr_irqs,
				void *arg)
{
	struct irq_fwspec *fwspec = arg;
	irq_hw_number_t hwirq;
	unsigned int type;
	int i, ret;

	ret = pintc_mcu_translate(domain, fwspec, &hwirq, &type);
	if (ret)
		return ret;

	for (i = 0; i < nr_irqs; i++)
		pintc_mcu_map_irq(domain, virq + i, hwirq + i);

	return 0;
}

static const struct irq_domain_ops pintc_mcu_domain_ops = {
	.map = pintc_mcu_map_irq,
	.translate = pintc_mcu_translate,
	.alloc = pintc_mcu_alloc_irqs,
	.free = pintc_mcu_free_irqs,
};

struct irq_domain *mcu_irq_domain;
EXPORT_SYMBOL(mcu_irq_domain);

static int __init pintc_init_mcu(struct pintc_chip_data *chip_data,
		struct fwnode_handle *handle)
{
	unsigned int mcu_irq_num = chip_data->mcu_irq_num;

	if (chip_data->vt) {
		chip_data->mcu_chip = &pintc_mcu_vt_chip;

		/**
		 * The current kernel does not support the API
		 * "irq_domain_create_legacy", we have to call
		 * "__irq_domain_add" directly.
		 */
		mcu_irq_domain = __irq_domain_add(handle, mcu_irq_num,
				mcu_irq_num, 0, &pintc_mcu_domain_ops,
				chip_data);
		if (mcu_irq_domain)
			irq_domain_associate_many(mcu_irq_domain,
					0, 0, mcu_irq_num);
	} else {
		chip_data->mcu_chip = &pintc_mcu_chip;
		mcu_irq_domain = irq_domain_create_linear(handle, mcu_irq_num,
				&pintc_mcu_domain_ops, chip_data);
		/* Mask all interrupts for now */
		writeq(0x0, chip_data->mcu_base + OFFSET_MCU_DVC_INT_EN);

		/* When building the root domain, move it to a better location */
		if (mcu_irq_domain)
			pintc_mcu_enable(chip_data->pintc_base);
	}

	if (!mcu_irq_domain) {
		pr_err(PREFIX "failed to create MCU irq domain\n");
		return -ENOMEM;
	}

	raw_spin_lock_init(&chip_data->pintc_lock);
	raw_spin_lock_init(&chip_data->mcu_lock);

	pr_info(PREFIX "MCU version [%u] on node [%u] initialized\n",
			chip_data->version, chip_data->node);

	return 0;
}

/* Currently, only MCU controller on node 0 is supported */
void handle_dev_int(struct pt_regs *regs)
{
	unsigned long stat, val;
	unsigned int hwirq;

	/* Disable global irq of MCU due to some hardware reasons */
	val = pintc_mcu_disable_and_save(chip_datas[0]);

	stat = readq(chip_datas[0]->mcu_base + OFFSET_MCU_DVC_INT);

	while (stat) {
		hwirq = ffs(stat) - 1;
		generic_handle_domain_irq(mcu_irq_domain, hwirq);
		stat &= ~(1UL << hwirq);
	}

	pintc_mcu_restore(chip_datas[0], val);
}

void handle_fault_int(void)
{
	int node;
	unsigned long value;
	void __iomem *mcu_base, *intpu_base;

	node = __this_cpu_read(hard_node_id);

#if defined(CONFIG_UNCORE_XUELANG)
	mcu_base = misc_platform_get_spbu_base(node);
	intpu_base = misc_platform_get_intpu_base(node);
#elif defined(CONFIG_UNCORE_JUNZHANG)
	mcu_base = chip_datas[node]->mcu_base;
	intpu_base = chip_datas[node]->pintc_base;
#endif

	pr_info("Enter fault int, si_fault_stat = %#llx\n",
			readq(mcu_base + OFFSET_SI_FAULT_STAT));

	writeq(0, mcu_base + OFFSET_SI_FAULT_INT_EN);
	writeq(0, mcu_base + OFFSET_DLI_RLTD_FAULT_INTEN);

#if defined(CONFIG_UNCORE_XUELANG)
	value = 0;
	writeq(value, mcu_base + OFFSET_DUAL_CG0_FAULT_INTEN);
	writeq(value, mcu_base + OFFSET_DUAL_CG1_FAULT_INTEN);
	writeq(value, mcu_base + OFFSET_DUAL_CG2_FAULT_INTEN);
	writeq(value, mcu_base + OFFSET_DUAL_CG3_FAULT_INTEN);
	writeq(value, mcu_base + OFFSET_DUAL_CG4_FAULT_INTEN);
	writeq(value, mcu_base + OFFSET_DUAL_CG5_FAULT_INTEN);
	writeq(value, mcu_base + OFFSET_DUAL_CG6_FAULT_INTEN);
	writeq(value, mcu_base + OFFSET_DUAL_CG7_FAULT_INTEN);
#elif defined(CONFIG_UNCORE_JUNZHANG)
	value = readq(intpu_base + OFFSET_FAULT_INT_CONFIG);
	value |= (1 << 8);
	writeq(value, intpu_base + OFFSET_FAULT_INT_CONFIG);
#endif
}

#ifdef CONFIG_OF
static int __init pintc_of_init_mcu(struct pintc_chip_data *chip_data,
		struct device_node *pintc)
{
	/* Not yet supported */
	if (chip_data->node > 0) {
		pr_info(PREFIX "MCU version [%u] on node [%u] skipped\n",
				chip_data->version, chip_data->node);
		return 0;
	}

	return pintc_init_mcu(chip_data, of_node_to_fwnode(pintc));
}

static int __init
pintc_of_init_common(struct device_node *pintc,
		struct device_node *parent, bool vt)
{
	int ret;
	u32 nr_irqs, node, version;
	void __iomem *pintc_base;
	void __iomem *mcu_base;
	struct pintc_chip_data *chip_data;

	if (WARN_ON(!pintc))
		return -ENODEV;

	if (vt && parent) {
		pr_err(PREFIX "virtual pintc has no parent controller\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(pintc, "sw64,node", &node);
	if (ret) {
		node = 0;
		pr_warn(PREFIX "\"sw64,node\" fallback to %u\n",
				node);
	}

	ret = of_property_read_u32(pintc, "sw64,irq-num", &nr_irqs);
	if (ret) {
		nr_irqs = vt ? 16 : 8;
		pr_warn(PREFIX "\"sw64,irq-num\" fallback to %u\n",
				nr_irqs);
	}

	ret = of_property_read_u32(pintc, "sw64,ver", &version);
	if (ret) {
		version = 1;
		pr_warn(PREFIX "\"sw64,ver\" fallback to %u\n",
				version);
	}

	pintc_base = of_iomap(pintc, 0);
	if (!vt && !pintc_base) {
		pintc_base = ioremap(INTPU_BASE_V1, INTPU_SIZE_V1);
		pr_warn(PREFIX "pintc base address fallback to 0x%lx\n",
				INTPU_BASE_V1);
	}

	mcu_base = of_iomap(pintc, 1);
	if (!vt && !mcu_base) {
		mcu_base = ioremap(MCU_BASE_V1, MCU_SIZE_V1);
		pr_warn(PREFIX "mcu base address fallback to 0x%lx\n",
				MCU_BASE_V1);
	}

	chip_data = pintc_alloc_chip_data(node);
	if (!chip_data) {
		ret = -ENOMEM;
		goto out_unmap;
	}

	chip_data->vt = vt;
	chip_data->node = node;
	chip_data->version = version;
	chip_data->pintc_base = pintc_base;
	chip_data->mcu_base = mcu_base;
	chip_data->mcu_irq_num = nr_irqs;

	/* Enable S3 wakeup interrupt for physical environment */
	if (!vt && IS_ENABLED(CONFIG_SUSPEND)) {
		writeq(0x80, chip_data->pintc_base + OFFSET_DEVINT_WKEN);
		writeq(0x80, chip_data->pintc_base + OFFSET_DEVINTWK_INTEN);
	}

	ret = pintc_of_init_mcu(chip_data, pintc);
	if (ret)
		goto out_free_mem;

	return 0;

out_free_mem:
	pintc_free_chip_data(chip_data);
out_unmap:
	iounmap(mcu_base);
	iounmap(pintc_base);
	return ret;
}

static int __init
pintc_of_init(struct device_node *pintc, struct device_node *parent)
{
	return pintc_of_init_common(pintc, parent, false);
}

IRQCHIP_DECLARE(sw64_pintc, "sw64,pintc", pintc_of_init);
IRQCHIP_DECLARE(sw64_pintc_legacy, "sw64,sw6_irq_controller", pintc_of_init);

static int __init
pintc_vt_of_init(struct device_node *pintc, struct device_node *parent)
{
	return pintc_of_init_common(pintc, parent, true);
}

IRQCHIP_DECLARE(sw64_pintc_vt, "sw64,pintc_vt", pintc_vt_of_init);
IRQCHIP_DECLARE(sw64_pintc_vt_legacy, "sw64,sw6_irq_vt_controller", pintc_vt_of_init);
#endif

#ifdef CONFIG_ACPI
#define SW_PINTC_FLAG_ENABLED    ACPI_MADT_ENABLED /* 0x1 */
#define SW_PINTC_FLAG_VIRTUAL    0x2               /* virtual PINTC */

#define is_pintc_enabled(flags)  ((flags) & SW_PINTC_FLAG_ENABLED)
#define is_pintc_virtual(flags)  ((flags) & SW_PINTC_FLAG_VIRTUAL)

/* Physical sub interrupt controllers */
enum sw_pintc_sub_type {
	SW_PINTC_SUB_TYPE_MCU    = 0x00,
	SW_PINTC_SUB_TYPE_MT     = 0x01,
	SW_PINTC_SUB_TYPE_FAULT  = 0x02,
	SW_PINTC_SUB_TYPE_NMI    = 0x03,
	SW_PINTC_SUB_TYPE_S3     = 0x04,
	SW_PINTC_SUB_TYPE_ADR    = 0x05,
	SW_PINTC_SUB_TYPE_COUNT
};

static int __init lpc_intc_parse_madt(union acpi_subtable_headers *header,
		const unsigned long end)
{
	struct acpi_madt_sw_lpc_intc *lpc_intc;

	lpc_intc = (struct acpi_madt_sw_lpc_intc *)header;

	/* Not yet supported */
	if (lpc_intc->node > 0)
		return 0;

	if ((lpc_intc->version == ACPI_MADT_SW_LPC_INTC_VERSION_NONE) ||
		(lpc_intc->version >= ACPI_MADT_SW_LPC_INTC_VERSION_RESERVED)) {
		pr_err(PREFIX "invalid LPC-INTC version\n");
		return -EINVAL;
	}

	return lpc_intc_acpi_init(mcu_irq_domain, lpc_intc);
}

static bool __init
pintc_sub_type_check(const struct acpi_madt_sw_pintc *pintc)
{
	int i, count = 0;

	for (i = 0; i < pintc->sub_num; ++i) {
		if (pintc->sub[i].type >= SW_PINTC_SUB_TYPE_COUNT)
			count++;
	}

	return count;
}

static int __init pintc_acpi_init_mcu(struct pintc_chip_data *chip_data,
		struct acpi_madt_sw_sub_pintc *mcu)
{
	struct fwnode_handle *handle;
	int ret;

	/* Not yet supported */
	if (chip_data->node > 0) {
		pr_info(PREFIX "MCU version [%u] on node [%u] skipped\n",
				chip_data->version, chip_data->node);
		return 0;
	}

	if (!mcu->status) {
		pr_info(PREFIX "MCU version [%u] on node [%u] disabled\n",
				chip_data->version, chip_data->node);
		return 0;
	}

	if (mcu->gsi_base != SW_PINTC_MCU_GSI_BASE) {
		pr_err(PREFIX "invalid MCU GSI\n");
		return -EINVAL;
	}

	handle = irq_domain_alloc_named_id_fwnode("PINTC-MCU", chip_data->node);
	if (!handle) {
		pr_err(PREFIX "failed to alloc fwnode\n");
		return -ENOMEM;
	}

	chip_data->mcu_irq_num = mcu->gsi_count;

	chip_data->mcu_base = ioremap(mcu->address, mcu->size);
	if (!chip_data->mcu_base) {
		pr_err(PREFIX "failed to map mcu base address\n");
		ret = -ENXIO;
		goto out_acpi_free_fwnode;
	}

	ret = pintc_init_mcu(chip_data, handle);
	if (ret)
		goto out_acpi_unmap_mcu;

	ret = sw64_add_gsi_domain_map(mcu->gsi_base, mcu->gsi_count, handle);
	if (ret) {
		pr_info(PREFIX "failed to add GSI map\n");
		goto out_acpi_free_mcu_domain;
	}

	/* Init SW LPC INTC */
	acpi_table_parse_madt(ACPI_MADT_TYPE_SW_LPC_INTC,
			lpc_intc_parse_madt, 0);

	return 0;

out_acpi_free_mcu_domain:
	irq_domain_remove(mcu_irq_domain);
out_acpi_unmap_mcu:
	iounmap(chip_data->mcu_base);
out_acpi_free_fwnode:
	irq_domain_free_fwnode(handle);
	return ret;
}

static int __init pintc_acpi_init_fault(struct pintc_chip_data *chip_data,
		struct acpi_madt_sw_sub_pintc *fault)
{
	if (!fault->status) {
		pr_info(PREFIX "Fault version [%u] on node [%u] disabled\n",
				chip_data->version, chip_data->node);
		return 0;
	}

	/* Fault share the same base address with MCU currently */
	chip_data->mcu_base = ioremap(fault->address, fault->size);
	if (!chip_data->mcu_base) {
		pr_err(PREFIX "failed to map fault base address\n");
		return -ENXIO;
	}

	pr_info(PREFIX "Fault version [%u] on node [%u] initialized\n",
			chip_data->version, chip_data->node);

	return 0;
}

int __init pintc_acpi_init(struct irq_domain *parent,
		struct acpi_madt_sw_pintc *pintc)
{
	struct pintc_chip_data *chip_data;
	int ret, i;
	bool enabled, virtual;

	enabled = is_pintc_enabled(pintc->flags);
	virtual = is_pintc_virtual(pintc->flags);

	pr_info(PREFIX "version [%u] on node [%u] (%s) %s\n",
			pintc->version, pintc->node,
			virtual ? "virtual" : "physical",
			enabled ? "found" : "disabled");

	if (!enabled)
		return 0;

	if (pintc_sub_type_check(pintc)) {
		pr_err(PREFIX "invalid sub type\n");
		return -EINVAL;
	}

	chip_data = pintc_alloc_chip_data(pintc->node);
	if (!chip_data)
		return -ENOMEM;

	/**
	 * The topology of interrupt controllers in Qemu is
	 * different from physical environment. We need to
	 * distinguish between them.
	 */
	chip_data->vt = virtual;

	chip_data->node = pintc->node;
	chip_data->version = pintc->version;

	chip_data->pintc_base = ioremap(pintc->address, pintc->size);
	if (!chip_data->pintc_base) {
		pr_err(PREFIX "failed to map pintc base address\n");
		ret = -ENXIO;
		goto out_acpi_free_chip_data;
	}

	/* Enable S3 wakeup interrupt for physical environment */
	if (!virtual && IS_ENABLED(CONFIG_SUSPEND)) {
		writeq(0x80, chip_data->pintc_base + OFFSET_DEVINT_WKEN);
		writeq(0x80, chip_data->pintc_base + OFFSET_DEVINTWK_INTEN);
	}

	for (i = 0; i < pintc->sub_num; ++i) {
		switch (pintc->sub[i].type) {
		case SW_PINTC_SUB_TYPE_MCU:
			pintc_acpi_init_mcu(chip_data, &pintc->sub[i]);
			break;
		case SW_PINTC_SUB_TYPE_FAULT:
			pintc_acpi_init_fault(chip_data, &pintc->sub[i]);
			break;
		default:
			break;
		}
	}

	return 0;

out_acpi_free_chip_data:
	pintc_free_chip_data(chip_data);
	return ret;
}
#endif
