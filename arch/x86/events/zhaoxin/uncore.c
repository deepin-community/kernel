// SPDX-License-Identifier: GPL-2.0-only
#include <asm/cpu_device_id.h>
#include "uncore.h"

static struct zhaoxin_uncore_type *empty_uncore[] = { NULL, };
static struct zhaoxin_uncore_type **uncore_msr_uncores = empty_uncore;
static struct zhaoxin_uncore_type **uncore_pci_uncores = empty_uncore;
static struct zhaoxin_uncore_type **uncore_mmio_uncores = empty_uncore;

static bool pcidrv_registered;
static struct pci_driver *uncore_pci_driver;

/* mask of cpus that collect uncore events */
static cpumask_t uncore_cpu_mask;
static cpumask_t uncore_cpu_subnode_mask;
static cpumask_t uncore_cpu_cluster_mask;

/* constraint for the fixed counter */
static struct event_constraint uncore_constraint_fixed =
	EVENT_CONSTRAINT(~0ULL, 1 << UNCORE_PMC_IDX_FIXED, ~0ULL);

static int max_packages, max_subnodes, max_clusters;
static int clusters_per_subnode;
static int subnodes_per_die;
static int dies_per_socket;

#define KH40000_MAX_SUBNODE_NUMBER    8
static int kh40000_pcibus_limit[KH40000_MAX_SUBNODE_NUMBER];

/* get CPU topology register */
#define BJ_GLOBAL_STATUS_MSR	0x1610
#define BJ_HDW_CONFIG_MSR	0X1628

/* KX5000/KX6000 event control */
#define KX5000_UNC_CTL_EV_SEL_MASK		0x000000ff
#define KX5000_UNC_CTL_UMASK_MASK		0x0000ff00
#define KX5000_UNC_CTL_EDGE_DET			(1 << 18)
#define KX5000_UNC_CTL_EN			(1 << 22)
#define KX5000_UNC_CTL_INVERT			(1 << 23)
#define KX5000_UNC_CTL_CMASK_MASK		0x7000000
#define KX5000_UNC_FIXED_CTR_CTL_EN		(1 << 0)

#define KX5000_UNC_RAW_EVENT_MASK		(KX5000_UNC_CTL_EV_SEL_MASK | \
						KX5000_UNC_CTL_UMASK_MASK | \
						KX5000_UNC_CTL_EDGE_DET | \
						KX5000_UNC_CTL_INVERT | \
						KX5000_UNC_CTL_CMASK_MASK)

/* KX5000/KX6000 uncore global register */
#define KX5000_UNC_PERF_GLOBAL_CTL		0x391
#define KX5000_UNC_FIXED_CTR			0x394
#define KX5000_UNC_FIXED_CTR_CTRL		0x395

/* KX5000/KX6000 uncore global control */
#define KX5000_UNC_GLOBAL_CTL_EN_PC_ALL		((1ULL << 4) - 1)
#define KX5000_UNC_GLOBAL_CTL_EN_FC		(1ULL << 32)

/* KX5000/KX6000 uncore register */
#define KX5000_UNC_PERFEVTSEL0		0x3c0
#define KX5000_UNC_UNCORE_PMC0		0x3b0

/* KH40000 event control */
#define KH40000_PMON_CTL_EV_SEL_MASK		0x000000ff
#define KH40000_PMON_CTL_UMASK_MASK		0x0000ff00
#define KH40000_PMON_CTL_RST			(1 << 17)
#define KH40000_PMON_CTL_EDGE_DET		(1 << 18)
#define KH40000_PMON_CTL_EN			(1 << 22)
#define KH40000_PMON_CTL_INVERT			(1 << 23)
#define KH40000_PMON_CTL_THRESH_MASK		0xff000000
#define KH40000_PMON_RAW_EVENT_MASK		(KH40000_PMON_CTL_EV_SEL_MASK | \
						KH40000_PMON_CTL_UMASK_MASK | \
						KH40000_PMON_CTL_EDGE_DET | \
						KH40000_PMON_CTL_INVERT | \
						KH40000_PMON_CTL_THRESH_MASK)

/* KH40000 LLC register*/
#define KH40000_LLC_MSR_PMON_CTL0		0x1660
#define KH40000_LLC_MSR_PMON_CTR0		0x165c
#define KH40000_LLC_MSR_PMON_BLK_CTL		0x1665

/* KH40000 HIF register*/
#define KH40000_HIF_MSR_PMON_CTL0		0x1656
#define KH40000_HIF_MSR_PMON_CTR0		0x1651
#define KH40000_HIF_MSR_PMON_FIXED_CTL		0x1655
#define KH40000_HIF_MSR_PMON_FIXED_CTR		0x1650
#define KH40000_HIF_MSR_PMON_BLK_CTL		0x165b

/* KH40000 ZZI(ZPI+ZOI+INI) register*/
#define KH40000_ZZI_MSR_PMON_CTL0		0x166A
#define KH40000_ZZI_MSR_PMON_CTR0		0x1666
#define KH40000_ZZI_MSR_PMON_BLK_CTL		0x166f

/* KH40000 MC register*/
#define KH40000_MC0_CHy_PMON_FIXED_CTL		0xf40
#define KH40000_MC0_CHy_PMON_FIXED_CTR		0xf20
#define KH40000_MC0_CHy_PMON_CTR0		0xf00
#define KH40000_MC0_CHy_PMON_CTL0		0xf28
#define KH40000_MC0_CHy_PMON_BLK_CTL		0xf44

#define KH40000_MC1_CHy_PMON_FIXED_CTL		0xf90
#define KH40000_MC1_CHy_PMON_FIXED_CTR		0xf70
#define KH40000_MC1_CHy_PMON_CTR0		0xf50
#define KH40000_MC1_CHy_PMON_CTL0		0xf78
#define KH40000_MC1_CHy_PMON_BLK_CTL		0xf94

/* KH40000 PCI register*/
#define KH40000_PCI_PMON_CTR0			0xf00
#define KH40000_PCI_PMON_CTL0			0xf28
#define KH40000_PCI_PMON_BLK_CTL		0xf44

/* KH40000 ZPI_DLL register*/
#define KH40000_ZPI_DLL_PMON_FIXED_CTL		0xf40
#define KH40000_ZPI_DLL_PMON_FIXED_CTR		0xf20
#define KH40000_ZPI_DLL_PMON_CTR0		0xf00
#define KH40000_ZPI_DLL_PMON_CTL0		0xf28
#define KH40000_ZPI_DLL_PMON_BLK_CTL		0xf44

/* KH40000 ZDI_DLL register*/
#define KH40000_ZDI_DLL_PMON_FIXED_CTL		0xf40
#define KH40000_ZDI_DLL_PMON_FIXED_CTR		0xf20
#define KH40000_ZDI_DLL_PMON_CTR0		0xf00
#define KH40000_ZDI_DLL_PMON_CTL0		0xf28
#define KH40000_ZDI_DLL_PMON_BLK_CTL		0xf44

/* KH40000 PXPTRF register*/
#define KH40000_PXPTRF_PMON_CTR0		0xf00
#define KH40000_PXPTRF_PMON_CTL0		0xf28
#define KH40000_PXPTRF_PMON_BLK_CTL		0xf44

/* KH40000 Box level control */
#define KH40000_PMON_BOX_CTL_RST_CTRL		(1 << 0)
#define KH40000_PMON_BOX_CTL_RST_CTRS		(1 << 1)
#define KH40000_PMON_BOX_CTL_FRZ		(1 << 8)
#define KH40000_PMON_PCI_BOX_PMON_EN		(1 << 31)

#define KH40000_PMON_BOX_CTL_INT		(KH40000_PMON_BOX_CTL_RST_CTRL | \
						KH40000_PMON_BOX_CTL_RST_CTRS)

#define KH40000_PMON_PCI_BOX_CTL_INT		(KH40000_PMON_BOX_CTL_RST_CTRL | \
						KH40000_PMON_BOX_CTL_RST_CTRS | \
						KH40000_PMON_PCI_BOX_PMON_EN)

/* KX7000 event control */
#define KX7000_PMON_CTL_EV_SEL_MASK		0x000000ff
#define KX7000_PMON_CTL_UMASK_MASK		0x0000ff00
#define KX7000_PMON_CTL_RST			(1 << 17)
#define KX7000_PMON_CTL_EDGE_DET		(1 << 18)
#define KX7000_PMON_CTL_LOGIC_OP0		(1 << 19)
#define KX7000_PMON_CTL_LOGIC_OP1		(1 << 21)
#define KX7000_PMON_CTL_EN			(1 << 22)
#define KX7000_PMON_CTL_INVERT			(1 << 23)
#define KX7000_PMON_CTL_THRESH_MASK		0xff000000
#define KX7000_PMON_RAW_EVENT_MASK		(KX7000_PMON_CTL_EV_SEL_MASK | \
						KX7000_PMON_CTL_UMASK_MASK | \
						KX7000_PMON_CTL_EDGE_DET | \
						KX7000_PMON_CTL_LOGIC_OP0 | \
						KX7000_PMON_CTL_LOGIC_OP1 | \
						KX7000_PMON_CTL_INVERT | \
						KX7000_PMON_CTL_THRESH_MASK)

/* KX7000 LLC register*/
#define KX7000_LLC_MSR_PMON_CTL0		0x1979
#define KX7000_LLC_MSR_PMON_CTR0		0x1975
#define KX7000_LLC_MSR_PMON_BLK_CTL		0x197e

/* KX7000 MESH register*/
#define KX7000_MESH_MSR_PMON_CTL0		0x1983
#define KX7000_MESH_MSR_PMON_CTR0		0x197f
#define KX7000_MESH_MSR_PMON_BLK_CTL		0x1987

/* KX7000 HOMESTOP register*/
#define KX7000_HOMESTOP_MSR_PMON_CTL0		0x196a
#define KX7000_HOMESTOP_MSR_PMON_CTR0		0x1966
#define KX7000_HOMESTOP_MSR_PMON_BLK_CTL	0x196e
#define KX7000_HOMESTOP_MSR_PMON_FIXED_CTR	0x1970
#define KX7000_HOMESTOP_MSR_PMON_FIXED_CTL	0x1971

/* KX7000 CCDie ZDI_PL register*/
#define KX7000_CCD_ZDI_PL_MSR_PMON_CTL0		0x1960
#define KX7000_CCD_ZDI_PL_MSR_PMON_CTR0		0x195c
#define KX7000_CCD_ZDI_PL_MSR_PMON_BLK_CTL	0x1964

/* KX7000 cIODie ZDI_PL register*/
#define KX7000_IOD_ZDI_PL_MSR_PMON_CTL0		0x1894
#define KX7000_IOD_ZDI_PL_MSR_PMON_CTR0		0x1890
#define KX7000_IOD_ZDI_PL_MSR_PMON_BLK_CTL	0x1898
#define KX7000_IOD_ZDI_PL_MSR_PMON_FIXED_CTR	0x189A
#define KX7000_IOD_ZDI_PL_MSR_PMON_FIXED_CTL	0x189B

/* KX7000 MC register*/
#define KX7000_MC_A0_CHy_PMON_FIXED_CTL		0xe30
#define KX7000_MC_A0_CHy_PMON_FIXED_CTR		0xe08
#define KX7000_MC_A0_CHy_PMON_CTR0		0xe00
#define KX7000_MC_A0_CHy_PMON_CTL0		0xe20
#define KX7000_MC_A0_CHy_PMON_BLK_CTL		0xe34

#define KX7000_MC_A1_CHy_PMON_FIXED_CTL		0xe70
#define KX7000_MC_A1_CHy_PMON_FIXED_CTR		0xe48
#define KX7000_MC_A1_CHy_PMON_CTR0		0xe40
#define KX7000_MC_A1_CHy_PMON_CTL0		0xe60
#define KX7000_MC_A1_CHy_PMON_BLK_CTL		0xe74

#define KX7000_MC_B0_CHy_PMON_FIXED_CTL		0xeb0
#define KX7000_MC_B0_CHy_PMON_FIXED_CTR		0xe88
#define KX7000_MC_B0_CHy_PMON_CTR0		0xe80
#define KX7000_MC_B0_CHy_PMON_CTL0		0xea0
#define KX7000_MC_B0_CHy_PMON_BLK_CTL		0xeb4

#define KX7000_MC_B1_CHy_PMON_FIXED_CTL		0xef0
#define KX7000_MC_B1_CHy_PMON_FIXED_CTR		0xec8
#define KX7000_MC_B1_CHy_PMON_CTR0		0xec0
#define KX7000_MC_B1_CHy_PMON_CTL0		0xee0
#define KX7000_MC_B1_CHy_PMON_BLK_CTL		0xef4

#define	KX7000_ZDI_DL_MMIO_PMON_CTR0		0xf00
#define KX7000_ZDI_DL_MMIO_PMON_CTL0		0xf28
#define KX7000_ZDI_DL_MMIO_PMON_BLK_CTL		0xf44
#define KX7000_IOD_ZDI_DL_MMIO_BASE_OFFSET	0x168
#define KX7000_CCD_ZDI_DL_MMIO_BASE_OFFSET	0x170
#define KX7000_ZDI_DL_MMIO_BASE_MASK		0x3fff
#define KX7000_ZDI_DL_MMIO_BASE_MASK		0x3fff
#define KX7000_ZDI_DL_MMIO_MEM0_MASK		0xfffff000
#define KX7000_ZDI_DL_MMIO_SIZE			0x1000

DEFINE_UNCORE_FORMAT_ATTR(event, event, "config:0-7");
DEFINE_UNCORE_FORMAT_ATTR(umask, umask, "config:8-15");
DEFINE_UNCORE_FORMAT_ATTR(edge, edge, "config:18");
DEFINE_UNCORE_FORMAT_ATTR(logic_op0, logic_op0, "config:19");
DEFINE_UNCORE_FORMAT_ATTR(logic_op1, logic_op1, "config:21");
DEFINE_UNCORE_FORMAT_ATTR(inv, inv, "config:23");
DEFINE_UNCORE_FORMAT_ATTR(cmask3, cmask, "config:24-26");
DEFINE_UNCORE_FORMAT_ATTR(thresh8, thresh, "config:24-31");

static void get_hdw_config_msr(void *config)
{
	u64 *data = (u64 *)config;

	rdmsrl(BJ_HDW_CONFIG_MSR, *data);
}

static void get_global_status_msr(void *status)
{
	u64 *data = (u64 *)status;

	rdmsrl(BJ_GLOBAL_STATUS_MSR, *data);
}

/*topology number : get max packages/subnode/clusters number*/
static void get_topology_number(void)
{
	int clusters;
	int subnodes;
	int dies;
	int packages;
	u64 data;

	rdmsrl(BJ_GLOBAL_STATUS_MSR, data);

	/* check packages number */
	packages = data & 0x1;
	if (packages)
		max_packages = 2;
	else
		max_packages = 1;

	/* only Yongfeng needs die/subnode/cluster info */
	if (boot_cpu_data.x86_model != ZHAOXIN_FAM7_KH40000)
		return;

	/* check dies_per_socket */
	dies = (data >> 12) & 0x1;
	if (dies)
		dies_per_socket = 2;
	else
		dies_per_socket = 1;

	/* check subnodes_per_die */
	subnodes = (data >> 32) & 0x3;
	if (subnodes == 0x3)
		subnodes_per_die = 2;
	else
		subnodes_per_die = 1;

	/* check clusters_per_subnode */
	clusters = (data >> 6) & 0x3;
	if (clusters == 0x3)
		clusters_per_subnode = 2;
	else
		clusters_per_subnode = 1;

	max_subnodes = max_packages * dies_per_socket * subnodes_per_die;
	max_clusters = clusters_per_subnode * max_subnodes;
}

static int get_pcibus_limit(void)
{
	struct pci_dev *dev;
	u32 val;
	int i = 0;

	dev = pci_get_device(0x1D17, 0x31B1, NULL);
	if (dev == NULL)
		return -ENODEV;

	pci_read_config_dword(dev, 0x94, &val);
	kh40000_pcibus_limit[i++] = (val & 0x1f) << 3 | 0x7;
	kh40000_pcibus_limit[i++] = (val >> 8 & 0x1f) << 3 | 0x7;
	if (dies_per_socket == 2) {
		kh40000_pcibus_limit[i++] = (val >> 16 & 0x1f) << 3 | 0x7;
		kh40000_pcibus_limit[i++] = (val >> 24 & 0x1f) << 3 | 0x7;
	}

	if (max_packages == 2) {
		pci_read_config_dword(dev, 0x9c, &val);
		kh40000_pcibus_limit[i++] = (val & 0x1f) << 3 | 0x7;
		kh40000_pcibus_limit[i++] = (val >> 8 & 0x1f) << 3 | 0x7;
		if (dies_per_socket == 2) {
			kh40000_pcibus_limit[i++] = (val >> 16 & 0x1f) << 3 | 0x7;
			kh40000_pcibus_limit[i++] = (val >> 24 & 0x1f) << 3 | 0x7;
		}
	}

	return 0;
}

static int uncore_pcibus_to_subnodeid(struct pci_bus *bus)
{
	int i;

	for (i = 0; i < KH40000_MAX_SUBNODE_NUMBER; i++) {
		if (bus->number < kh40000_pcibus_limit[i])
			break;
	}

	return i;
}

DEFINE_PER_CPU(int, zx_package_id);
DEFINE_PER_CPU(int, zx_subnode_id);
DEFINE_PER_CPU(int, zx_cluster_id);

static void get_topology_info(void)
{
	int cpu;
	int cluster_id;
	int socket_id;
	int die_id;
	int subnode_id;

	int die_info;
	int subnode_info;
	int cluster_info;

	u64 config;

	for_each_present_cpu(cpu) {
		smp_call_function_single(cpu, get_global_status_msr, &config, 1);
		socket_id = (int)((config >> 3) & 0x1);
		per_cpu(zx_package_id, cpu) = socket_id;

		/* only kh40000 needs cluster and subnode info */
		if (boot_cpu_data.x86_model != ZHAOXIN_FAM7_KH40000)
			continue;

		smp_call_function_single(cpu, get_hdw_config_msr, &config, 1);

		die_info = (int)((config >> 21) & 0x3);
		die_id = socket_id * dies_per_socket + die_info;

		subnode_info = (int)((config >> 20) & 0x1);
		subnode_id = die_id * subnodes_per_die + subnode_info;
		per_cpu(zx_subnode_id, cpu) = subnode_id;

		cluster_info = (int)((config >> 18) & 0x3);
		cluster_id = subnode_id * clusters_per_subnode + cluster_info;
		per_cpu(zx_cluster_id, cpu) = cluster_id;
	}
}

static int zx_topology_cluster_id(int cpu)
{
	return per_cpu(zx_cluster_id, cpu);
}

static int zx_topology_subnode_id(int cpu)
{
	return per_cpu(zx_subnode_id, cpu);
}

static int zx_topology_package_id(int cpu)
{
	return per_cpu(zx_package_id, cpu);
}

DEFINE_PER_CPU(cpumask_t, zx_cluster_core_bits);
DEFINE_PER_CPU(cpumask_t, zx_subnode_core_bits);

static void zx_gen_core_map(void)
{
	int cpu, i;
	int cluster_id, subnode_id;

	for_each_present_cpu(cpu) {
		cluster_id = zx_topology_cluster_id(cpu);

		for_each_present_cpu(i) {
			if (zx_topology_cluster_id(i) == cluster_id)
				cpumask_set_cpu(i, &per_cpu(zx_cluster_core_bits, cpu));
		}
	}

	for_each_present_cpu(cpu) {
		subnode_id = zx_topology_subnode_id(cpu);

		for_each_present_cpu(i) {
			if (zx_topology_subnode_id(i) == subnode_id)
				cpumask_set_cpu(i, &per_cpu(zx_subnode_core_bits, cpu));
		}
	}
}

static struct cpumask *topology_cluster_core_cpumask(int cpu)
{
	return &per_cpu(zx_cluster_core_bits, cpu);
}

static struct cpumask *topology_subnode_core_cpumask(int cpu)
{
	return &per_cpu(zx_subnode_core_bits, cpu);
}

static void uncore_free_pcibus_map(void)
{

}

static int kh40000_pci2node_map_init(void)
{
	return 0;
}

ssize_t zx_uncore_event_show(struct device *dev, struct device_attribute *attr,  char *buf)
{
	struct uncore_event_desc *event =
		container_of(attr, struct uncore_event_desc, attr);
	return sprintf(buf, "%s", event->config);
}

static struct zhaoxin_uncore_box *uncore_pmu_to_box(struct zhaoxin_uncore_pmu *pmu, int cpu)
{
	if (boot_cpu_data.x86_model == ZHAOXIN_FAM7_KH40000) {
		if (!strcmp(pmu->type->name, "llc"))
			return pmu->boxes[zx_topology_cluster_id(cpu)];
		else
			return pmu->boxes[zx_topology_subnode_id(cpu)];
	} else {
		return pmu->boxes[zx_topology_package_id(cpu)];
	}
}

static u64 uncore_msr_read_counter(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	u64 count;

	WARN_ON_ONCE(box->cpu != smp_processor_id());
	rdmsrl(event->hw.event_base, count);
	return count;
}

static void uncore_assign_hw_event(struct zhaoxin_uncore_box *box, struct perf_event *event,
				   int idx)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->idx = idx;
	hwc->last_tag = ++box->tags[idx];

	if (uncore_pmc_fixed(hwc->idx)) {
		hwc->event_base = uncore_fixed_ctr(box);
		hwc->config_base = uncore_fixed_ctl(box);
		return;
	}

	hwc->config_base = uncore_event_ctl(box, hwc->idx);
	hwc->event_base  = uncore_perf_ctr(box, hwc->idx);
}

void uncore_perf_event_update(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	u64 prev_count, new_count, delta;
	int shift;

	if (uncore_pmc_fixed(event->hw.idx))
		shift = 64 - uncore_fixed_ctr_bits(box);
	else
		shift = 64 - uncore_perf_ctr_bits(box);

	/* the hrtimer might modify the previous event value */
again:
	prev_count = local64_read(&event->hw.prev_count);
	new_count = uncore_read_counter(box, event);
	if (local64_xchg(&event->hw.prev_count, new_count) != prev_count)
		goto again;

	delta = (new_count << shift) - (prev_count << shift);
	delta >>= shift;

	local64_add(delta, &event->count);
}

/*KX5000/KX6000 uncore ops start*/
static void kx5000_uncore_msr_disable_event(struct zhaoxin_uncore_box *box,
					    struct perf_event *event)
{
	wrmsrl(event->hw.config_base, 0);
}

static void kx5000_uncore_msr_disable_box(struct zhaoxin_uncore_box *box)
{
	wrmsrl(KX5000_UNC_PERF_GLOBAL_CTL, 0);
}

static void kx5000_uncore_msr_enable_box(struct zhaoxin_uncore_box *box)
{
	wrmsrl(KX5000_UNC_PERF_GLOBAL_CTL,
	       KX5000_UNC_GLOBAL_CTL_EN_PC_ALL | KX5000_UNC_GLOBAL_CTL_EN_FC);
}

static void kx5000_uncore_msr_enable_event(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;

	if (hwc->idx < UNCORE_PMC_IDX_FIXED)
		wrmsrl(hwc->config_base, hwc->config | KX5000_UNC_CTL_EN);
	else
		wrmsrl(hwc->config_base, KX5000_UNC_FIXED_CTR_CTL_EN);
}

static struct attribute *kx5000_uncore_formats_attr[] = {
	&format_attr_event.attr,
	&format_attr_umask.attr,
	&format_attr_edge.attr,
	&format_attr_inv.attr,
	&format_attr_cmask3.attr,
	NULL,
};

static struct attribute_group kx5000_uncore_format_group = {
	.name = "format",
	.attrs = kx5000_uncore_formats_attr,
};

static struct uncore_event_desc kx5000_uncore_events[] = {
	{ /* end: all zeroes */ },
};

static struct zhaoxin_uncore_ops kx5000_uncore_msr_ops = {
	.disable_box	= kx5000_uncore_msr_disable_box,
	.enable_box	= kx5000_uncore_msr_enable_box,
	.disable_event	= kx5000_uncore_msr_disable_event,
	.enable_event	= kx5000_uncore_msr_enable_event,
	.read_counter	= uncore_msr_read_counter,
};

static struct zhaoxin_uncore_type kx5000_uncore_box = {
	.name		= "",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.event_ctl	= KX5000_UNC_PERFEVTSEL0,
	.perf_ctr	= KX5000_UNC_UNCORE_PMC0,
	.fixed_ctr	= KX5000_UNC_FIXED_CTR,
	.fixed_ctl	= KX5000_UNC_FIXED_CTR_CTRL,
	.event_mask	= KX5000_UNC_RAW_EVENT_MASK,
	.event_descs	= kx5000_uncore_events,
	.ops		= &kx5000_uncore_msr_ops,
	.format_group	= &kx5000_uncore_format_group,
};

static struct zhaoxin_uncore_type *kx5000_msr_uncores[] = {
	&kx5000_uncore_box,
	NULL,
};
/*KX5000/KX6000 uncore ops end*/

/*KH40000 msr ops start*/
static void kh40000_uncore_msr_disable_event(struct zhaoxin_uncore_box *box,
					     struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;

	wrmsrl(hwc->config_base, hwc->config);
}

static void kh40000_uncore_msr_enable_event(struct zhaoxin_uncore_box *box,
					    struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;

	wrmsrl(hwc->config_base, hwc->config | KH40000_PMON_CTL_EN);
}

static void kh40000_uncore_msr_disable_box(struct zhaoxin_uncore_box *box)
{
	u64 config;
	unsigned int msr;

	msr = uncore_msr_box_ctl(box);
	if (msr) {
		rdmsrl(msr, config);
		config |= KH40000_PMON_BOX_CTL_FRZ;
		wrmsrl(msr, config);
	}
}

static void kh40000_uncore_msr_enable_box(struct zhaoxin_uncore_box *box)
{
	u64 config;
	unsigned int msr;

	msr = uncore_msr_box_ctl(box);
	if (msr) {
		rdmsrl(msr, config);
		config &= ~KH40000_PMON_BOX_CTL_FRZ;
		wrmsrl(msr, config);
	}
}

static void kh40000_uncore_msr_init_box(struct zhaoxin_uncore_box *box)
{
	unsigned int msr = uncore_msr_box_ctl(box);

	if (msr) {
		wrmsrl(msr, KH40000_PMON_BOX_CTL_INT);
		wrmsrl(msr, 0);
	}
}

static struct attribute *kh40000_uncore_formats_attr[] = {
	&format_attr_event.attr,
	&format_attr_umask.attr,
	&format_attr_edge.attr,
	&format_attr_inv.attr,
	&format_attr_thresh8.attr,
	NULL,
};

static struct attribute_group kh40000_uncore_format_group = {
	.name = "format",
	.attrs = kh40000_uncore_formats_attr,
};

static struct uncore_event_desc kh40000_uncore_llc_box_events[] = {
	{ /* end: all zeroes */ },
};

static struct uncore_event_desc kh40000_uncore_hif_box_events[] = {
	{ /* end: all zeroes */ },
};

static struct uncore_event_desc kh40000_uncore_zzi_box_events[] = {
	{ /* end: all zeroes */ },
};

static struct zhaoxin_uncore_ops kh40000_uncore_msr_ops = {
	.init_box	= kh40000_uncore_msr_init_box,
	.disable_box	= kh40000_uncore_msr_disable_box,
	.enable_box	= kh40000_uncore_msr_enable_box,
	.disable_event	= kh40000_uncore_msr_disable_event,
	.enable_event	= kh40000_uncore_msr_enable_event,
	.read_counter	= uncore_msr_read_counter,
};

static struct zhaoxin_uncore_type kh40000_uncore_llc_box = {
	.name		= "llc",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.event_ctl	= KH40000_LLC_MSR_PMON_CTL0,
	.perf_ctr	= KH40000_LLC_MSR_PMON_CTR0,
	.event_mask	= KH40000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KH40000_LLC_MSR_PMON_BLK_CTL,
	.event_descs	= kh40000_uncore_llc_box_events,
	.ops		= &kh40000_uncore_msr_ops,
	.format_group	= &kh40000_uncore_format_group,
};

static struct zhaoxin_uncore_type kh40000_uncore_hif_box = {
	.name		= "hif",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits = 48,
	.event_ctl	= KH40000_HIF_MSR_PMON_CTL0,
	.perf_ctr	= KH40000_HIF_MSR_PMON_CTR0,
	.fixed_ctr	= KH40000_HIF_MSR_PMON_FIXED_CTR,
	.fixed_ctl	= KH40000_HIF_MSR_PMON_FIXED_CTL,
	.event_mask	= KH40000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KH40000_HIF_MSR_PMON_BLK_CTL,
	.event_descs	= kh40000_uncore_hif_box_events,
	.ops		= &kh40000_uncore_msr_ops,
	.format_group	= &kh40000_uncore_format_group,
};

static struct zhaoxin_uncore_type kh40000_uncore_zzi_box = {
	.name		= "zzi",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.event_ctl	= KH40000_ZZI_MSR_PMON_CTL0,
	.perf_ctr	= KH40000_ZZI_MSR_PMON_CTR0,
	.event_mask	= KH40000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KH40000_ZZI_MSR_PMON_BLK_CTL,
	.event_descs	= kh40000_uncore_zzi_box_events,
	.ops		= &kh40000_uncore_msr_ops,
	.format_group	= &kh40000_uncore_format_group,
};

static struct zhaoxin_uncore_type *kh40000_msr_uncores[] = {
	&kh40000_uncore_llc_box,
	&kh40000_uncore_hif_box,
	&kh40000_uncore_zzi_box,
	NULL,
};
/*KH40000 msr ops end*/

/*KH40000 pci ops start*/
static void kh40000_uncore_pci_disable_event(struct zhaoxin_uncore_box *box,
					     struct perf_event *event)
{
	struct pci_dev *pdev = box->pci_dev;
	struct hw_perf_event *hwc = &event->hw;

	pci_write_config_dword(pdev, hwc->config_base, hwc->config);
}

static void kh40000_uncore_pci_enable_event(struct zhaoxin_uncore_box *box,
					    struct perf_event *event)
{
	struct pci_dev *pdev = box->pci_dev;
	struct hw_perf_event *hwc = &event->hw;

	pci_write_config_dword(pdev, hwc->config_base, hwc->config | KH40000_PMON_CTL_EN);
}

static void kh40000_uncore_pci_disable_box(struct zhaoxin_uncore_box *box)
{
	struct pci_dev *pdev = box->pci_dev;
	int box_ctl = uncore_pci_box_ctl(box);
	u32 config = 0;

	if (!pci_read_config_dword(pdev, box_ctl, &config)) {
		config |= KH40000_PMON_BOX_CTL_FRZ;
		pci_write_config_dword(pdev, box_ctl, config);
	}
}

static void kh40000_uncore_pci_enable_box(struct zhaoxin_uncore_box *box)
{
	struct pci_dev *pdev = box->pci_dev;
	int box_ctl = uncore_pci_box_ctl(box);
	u32 config = 0;

	if (!pci_read_config_dword(pdev, box_ctl, &config)) {
		config &= ~KH40000_PMON_BOX_CTL_FRZ;
		pci_write_config_dword(pdev, box_ctl, config);
	}
}

static u64 kh40000_uncore_pci_read_counter(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	struct pci_dev *pdev = box->pci_dev;
	struct hw_perf_event *hwc = &event->hw;
	u64 count = 0;

	pci_read_config_dword(pdev, hwc->event_base, (u32 *)&count + 1);
	pci_read_config_dword(pdev, hwc->event_base + 4, (u32 *)&count);

	return count;
}

static void kh40000_uncore_pci_init_box(struct zhaoxin_uncore_box *box)
{
	struct pci_dev *pdev = box->pci_dev;
	int box_ctl = uncore_pci_box_ctl(box);

	pci_write_config_dword(pdev, box_ctl, KH40000_PMON_PCI_BOX_CTL_INT);
}

static struct uncore_event_desc kh40000_uncore_imc_events[] = {
	{ /* end: all zeroes */ },
};

static struct uncore_event_desc kh40000_uncore_pci_events[] = {
	{ /* end: all zeroes */ },
};

static struct uncore_event_desc kh40000_uncore_zpi_dll_events[] = {
	{ /* end: all zeroes */ },
};

static struct uncore_event_desc kh40000_uncore_zdi_dll_events[] = {
	{ /* end: all zeroes */ },
};

static struct uncore_event_desc kh40000_uncore_pxptrf_events[] = {
	{ /* end: all zeroes */ },
};

static struct zhaoxin_uncore_ops kh40000_uncore_pci_ops = {
	.init_box	= kh40000_uncore_pci_init_box,
	.disable_box	= kh40000_uncore_pci_disable_box,
	.enable_box	= kh40000_uncore_pci_enable_box,
	.disable_event	= kh40000_uncore_pci_disable_event,
	.enable_event	= kh40000_uncore_pci_enable_event,
	.read_counter	= kh40000_uncore_pci_read_counter
};

static struct zhaoxin_uncore_type kh40000_uncore_mc0 = {
	.name		= "mc0",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.fixed_ctr	= KH40000_MC0_CHy_PMON_FIXED_CTR,
	.fixed_ctl	= KH40000_MC0_CHy_PMON_FIXED_CTL,
	.event_descs	= kh40000_uncore_imc_events,
	.perf_ctr	= KH40000_MC0_CHy_PMON_CTR0,
	.event_ctl	= KH40000_MC0_CHy_PMON_CTL0,
	.event_mask	= KH40000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KH40000_MC0_CHy_PMON_BLK_CTL,
	.ops		= &kh40000_uncore_pci_ops,
	.format_group	= &kh40000_uncore_format_group
};

static struct zhaoxin_uncore_type kh40000_uncore_mc1 = {
	.name		= "mc1",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.fixed_ctr	= KH40000_MC1_CHy_PMON_FIXED_CTR,
	.fixed_ctl	= KH40000_MC1_CHy_PMON_FIXED_CTL,
	.event_descs	= kh40000_uncore_imc_events,
	.perf_ctr	= KH40000_MC1_CHy_PMON_CTR0,
	.event_ctl	= KH40000_MC1_CHy_PMON_CTL0,
	.event_mask	= KH40000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KH40000_MC1_CHy_PMON_BLK_CTL,
	.ops		= &kh40000_uncore_pci_ops,
	.format_group	= &kh40000_uncore_format_group
};

static struct zhaoxin_uncore_type kh40000_uncore_pci = {
	.name		= "pci",
	.num_counters	= 4,
	.num_boxes	= 10,
	.perf_ctr_bits	= 48,
	.event_descs	= kh40000_uncore_pci_events,
	.perf_ctr	= KH40000_PCI_PMON_CTR0,
	.event_ctl	= KH40000_PCI_PMON_CTL0,
	.event_mask	= KH40000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KH40000_PCI_PMON_BLK_CTL,
	.ops		= &kh40000_uncore_pci_ops,
	.format_group	= &kh40000_uncore_format_group
};

static struct zhaoxin_uncore_type kh40000_uncore_zpi_dll = {
	.name		= "zpi_dll",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.event_descs	= kh40000_uncore_zpi_dll_events,
	.perf_ctr	= KH40000_ZPI_DLL_PMON_CTR0,
	.event_ctl	= KH40000_ZPI_DLL_PMON_CTL0,
	.event_mask	= KH40000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KH40000_ZPI_DLL_PMON_BLK_CTL,
	.ops		= &kh40000_uncore_pci_ops,
	.format_group	= &kh40000_uncore_format_group
};

static struct zhaoxin_uncore_type kh40000_uncore_zdi_dll = {
	.name		= "zdi_dll",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.event_descs	= kh40000_uncore_zdi_dll_events,
	.perf_ctr	= KH40000_ZDI_DLL_PMON_CTR0,
	.event_ctl	= KH40000_ZDI_DLL_PMON_CTL0,
	.event_mask	= KH40000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KH40000_ZDI_DLL_PMON_BLK_CTL,
	.ops		= &kh40000_uncore_pci_ops,
	.format_group	= &kh40000_uncore_format_group
};

static struct zhaoxin_uncore_type kh40000_uncore_pxptrf = {
	.name		= "pxptrf",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.event_descs	= kh40000_uncore_pxptrf_events,
	.perf_ctr	= KH40000_PXPTRF_PMON_CTR0,
	.event_ctl	= KH40000_PXPTRF_PMON_CTL0,
	.event_mask	= KH40000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KH40000_PXPTRF_PMON_BLK_CTL,
	.ops		= &kh40000_uncore_pci_ops,
	.format_group	= &kh40000_uncore_format_group
};

enum {
	KH40000_PCI_UNCORE_MC0,
	KH40000_PCI_UNCORE_MC1,
	KH40000_PCI_UNCORE_PCI,
	KH40000_PCI_UNCORE_ZPI_DLL,
	KH40000_PCI_UNCORE_ZDI_DLL,
	KH40000_PCI_UNCORE_PXPTRF,
};

static struct zhaoxin_uncore_type *kh40000_pci_uncores[] = {
	[KH40000_PCI_UNCORE_MC0]	= &kh40000_uncore_mc0,
	[KH40000_PCI_UNCORE_MC1]	= &kh40000_uncore_mc1,
	[KH40000_PCI_UNCORE_PCI]	= &kh40000_uncore_pci,
	[KH40000_PCI_UNCORE_ZPI_DLL]	= &kh40000_uncore_zpi_dll,
	[KH40000_PCI_UNCORE_ZDI_DLL]	= &kh40000_uncore_zdi_dll,
	[KH40000_PCI_UNCORE_PXPTRF]	= &kh40000_uncore_pxptrf,
	NULL,
};

static const struct pci_device_id kh40000_uncore_pci_ids[] = {
	{ /* MC Channe0/1 */
		PCI_DEVICE(0x1D17, 0x31b2),
		.driver_data = UNCORE_PCI_DEV_DATA(KH40000_PCI_UNCORE_MC0, 0),
	},

	{ /* ZPI_DLL */
		PCI_DEVICE(0x1D17, 0x91c1),
		.driver_data = UNCORE_PCI_DEV_DATA(KH40000_PCI_UNCORE_ZPI_DLL, 0),
	},

	{ /* ZDI_DLL */
		PCI_DEVICE(0x1D17, 0x3b03),
		.driver_data = UNCORE_PCI_DEV_DATA(KH40000_PCI_UNCORE_ZDI_DLL, 0),
	},

	{ /* PXPTRF */
		PCI_DEVICE(0x1D17, 0x31B4),
		.driver_data = UNCORE_PCI_DEV_DATA(KH40000_PCI_UNCORE_PXPTRF, 0),
	},

	{ /* end: all zeroes */ }
};

static struct pci_driver kh40000_uncore_pci_driver = {
	.name = "kh40000_uncore",
	.id_table = kh40000_uncore_pci_ids,
};
/*KH40000 pci ops end*/

/*KX7000 msr ops start*/
static unsigned int kx7000_uncore_msr_offsets[] = {
	0x0, 0x13, 0x27, 0x3b, 0x4f, 0x63, 0x77, 0x8b
};

static struct attribute *kx7000_uncore_formats_attr[] = {
	&format_attr_event.attr,
	&format_attr_umask.attr,
	&format_attr_edge.attr,
	&format_attr_logic_op0.attr,
	&format_attr_logic_op1.attr,
	&format_attr_inv.attr,
	&format_attr_thresh8.attr,
	NULL,
};

static struct attribute_group kx7000_uncore_format_group = {
	.name = "format",
	.attrs = kx7000_uncore_formats_attr,
};

static struct zhaoxin_uncore_type kx7000_uncore_mesh_box = {
	.name		= "mesh",
	.num_counters	= 4,
	.num_boxes	= 8,
	.perf_ctr_bits	= 48,
	.event_ctl	= KX7000_MESH_MSR_PMON_CTL0,
	.perf_ctr	= KX7000_MESH_MSR_PMON_CTR0,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KX7000_MESH_MSR_PMON_BLK_CTL,
	.msr_offsets	= kx7000_uncore_msr_offsets,
	.ops		= &kh40000_uncore_msr_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type kx7000_uncore_llc_box = {
	.name		= "llc",
	.num_counters	= 4,
	.num_boxes	= 8,
	.perf_ctr_bits	= 48,
	.event_ctl	= KX7000_LLC_MSR_PMON_CTL0,
	.perf_ctr	= KX7000_LLC_MSR_PMON_CTR0,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KX7000_LLC_MSR_PMON_BLK_CTL,
	.msr_offsets	= kx7000_uncore_msr_offsets,
	.ops		= &kh40000_uncore_msr_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type kx7000_uncore_hif_box = {
	.name		= "hif",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.event_ctl	= KH40000_HIF_MSR_PMON_CTL0,
	.perf_ctr	= KH40000_HIF_MSR_PMON_CTR0,
	.fixed_ctr	= KH40000_HIF_MSR_PMON_FIXED_CTR,
	.fixed_ctl	= KH40000_HIF_MSR_PMON_FIXED_CTL,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KH40000_HIF_MSR_PMON_BLK_CTL,
	.ops		= &kh40000_uncore_msr_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type kx7000_uncore_homestop = {
	.name		= "homestop",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.event_ctl	= KX7000_HOMESTOP_MSR_PMON_CTL0,
	.perf_ctr	= KX7000_HOMESTOP_MSR_PMON_CTR0,
	.fixed_ctr	= KX7000_HOMESTOP_MSR_PMON_FIXED_CTR,
	.fixed_ctl	= KX7000_HOMESTOP_MSR_PMON_FIXED_CTL,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KX7000_HOMESTOP_MSR_PMON_BLK_CTL,
	.ops		= &kh40000_uncore_msr_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type kx7000_uncore_ccd_zdi_pl = {
	.name		= "ccd_zdi_pl",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.event_ctl	= KX7000_CCD_ZDI_PL_MSR_PMON_CTL0,
	.perf_ctr	= KX7000_CCD_ZDI_PL_MSR_PMON_CTR0,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KX7000_CCD_ZDI_PL_MSR_PMON_BLK_CTL,
	.ops		= &kh40000_uncore_msr_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type kx7000_uncore_iod_zdi_pl = {
	.name		= "iod_zdi_pl",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.event_ctl	= KX7000_IOD_ZDI_PL_MSR_PMON_CTL0,
	.perf_ctr	= KX7000_IOD_ZDI_PL_MSR_PMON_CTR0,
	.fixed_ctr	= KX7000_IOD_ZDI_PL_MSR_PMON_FIXED_CTR,
	.fixed_ctl	= KX7000_IOD_ZDI_PL_MSR_PMON_FIXED_CTL,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KX7000_IOD_ZDI_PL_MSR_PMON_BLK_CTL,
	.ops		= &kh40000_uncore_msr_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type *kx7000_msr_uncores[] = {
	&kx7000_uncore_llc_box,
	&kx7000_uncore_mesh_box,
	&kx7000_uncore_hif_box,
	&kx7000_uncore_homestop,
	&kx7000_uncore_ccd_zdi_pl,
	&kx7000_uncore_iod_zdi_pl,
	NULL,
};
/*KX7000 msr ops end*/

/*KX7000 pci ops start*/
static unsigned int kx7000_mc_ctr_lh_offsets[] = {
	0xc, 0xe, 0x10, 0x12, 0x14
};

static u64 kx7000_uncore_pci_mc_read_counter(struct zhaoxin_uncore_box *box,
					     struct perf_event *event)
{
	struct pci_dev *pdev = box->pci_dev;
	struct hw_perf_event *hwc = &event->hw;
	u64 count = 0;

	pci_read_config_word(pdev, hwc->event_base, (u16 *)&count + 3);
	pci_read_config_dword(pdev, hwc->event_base + kx7000_mc_ctr_lh_offsets[hwc->idx],
			      (u32 *)&count);

	return count;
}

static struct zhaoxin_uncore_ops kx7000_uncore_pci_mc_ops = {
	.init_box	= kh40000_uncore_pci_init_box,
	.disable_box	= kh40000_uncore_pci_disable_box,
	.enable_box	= kh40000_uncore_pci_enable_box,
	.disable_event	= kh40000_uncore_pci_disable_event,
	.enable_event	= kh40000_uncore_pci_enable_event,
	.read_counter	= kx7000_uncore_pci_mc_read_counter
};

static struct zhaoxin_uncore_type kx7000_uncore_mc_a0 = {
	.name		= "mc_a0",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.fixed_ctr	= KX7000_MC_A0_CHy_PMON_FIXED_CTR,
	.fixed_ctl	= KX7000_MC_A0_CHy_PMON_FIXED_CTL,
	.perf_ctr	= KX7000_MC_A0_CHy_PMON_CTR0,
	.event_ctl	= KX7000_MC_A0_CHy_PMON_CTL0,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KX7000_MC_A0_CHy_PMON_BLK_CTL,
	.ops		= &kx7000_uncore_pci_mc_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type kx7000_uncore_mc_a1 = {
	.name		= "mc_a1",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.fixed_ctr	= KX7000_MC_A1_CHy_PMON_FIXED_CTR,
	.fixed_ctl	= KX7000_MC_A1_CHy_PMON_FIXED_CTL,
	.perf_ctr	= KX7000_MC_A1_CHy_PMON_CTR0,
	.event_ctl	= KX7000_MC_A1_CHy_PMON_CTL0,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KX7000_MC_A1_CHy_PMON_BLK_CTL,
	.ops		= &kx7000_uncore_pci_mc_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type kx7000_uncore_mc_b0 = {
	.name		= "mc_b0",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.fixed_ctr	= KX7000_MC_B0_CHy_PMON_FIXED_CTR,
	.fixed_ctl	= KX7000_MC_B0_CHy_PMON_FIXED_CTL,
	.perf_ctr	= KX7000_MC_B0_CHy_PMON_CTR0,
	.event_ctl	= KX7000_MC_B0_CHy_PMON_CTL0,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KX7000_MC_B0_CHy_PMON_BLK_CTL,
	.ops		= &kx7000_uncore_pci_mc_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type kx7000_uncore_mc_b1 = {
	.name		= "mc_b1",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.fixed_ctr	= KX7000_MC_B1_CHy_PMON_FIXED_CTR,
	.fixed_ctl	= KX7000_MC_B1_CHy_PMON_FIXED_CTL,
	.perf_ctr	= KX7000_MC_B1_CHy_PMON_CTR0,
	.event_ctl	= KX7000_MC_B1_CHy_PMON_CTL0,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KX7000_MC_B1_CHy_PMON_BLK_CTL,
	.ops		= &kx7000_uncore_pci_mc_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type kx7000_uncore_pci = {
	.name		= "pci",
	.num_counters	= 4,
	.num_boxes	= 17,
	.perf_ctr_bits	= 48,
	.perf_ctr	= KH40000_PCI_PMON_CTR0,
	.event_ctl	= KH40000_PCI_PMON_CTL0,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KH40000_PCI_PMON_BLK_CTL,
	.ops		= &kh40000_uncore_pci_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type kx7000_uncore_pxptrf = {
	.name		= "pxptrf",
	.num_counters	= 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.event_descs	= kh40000_uncore_pxptrf_events,
	.perf_ctr	= KH40000_PXPTRF_PMON_CTR0,
	.event_ctl	= KH40000_PXPTRF_PMON_CTL0,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KH40000_PXPTRF_PMON_BLK_CTL,
	.ops		= &kh40000_uncore_pci_ops,
	.format_group	= &kx7000_uncore_format_group,
};

enum {
	KX7000_PCI_UNCORE_MC_A0,
	KX7000_PCI_UNCORE_MC_A1,
	KX7000_PCI_UNCORE_MC_B0,
	KX7000_PCI_UNCORE_MC_B1,
	KX7000_PCI_UNCORE_PCI,
	KX7000_PCI_UNCORE_PXPTRF,
};

static struct zhaoxin_uncore_type *kx7000_pci_uncores[] = {
	[KX7000_PCI_UNCORE_MC_A0]	= &kx7000_uncore_mc_a0,
	[KX7000_PCI_UNCORE_MC_A1]	= &kx7000_uncore_mc_a1,
	[KX7000_PCI_UNCORE_MC_B0]	= &kx7000_uncore_mc_b0,
	[KX7000_PCI_UNCORE_MC_B1]	= &kx7000_uncore_mc_b1,
	[KX7000_PCI_UNCORE_PCI]		= &kx7000_uncore_pci,
	[KX7000_PCI_UNCORE_PXPTRF]	= &kx7000_uncore_pxptrf,
	NULL,
};

static const struct pci_device_id kx7000_uncore_pci_ids[] = {
	{ /* MC Channe A0/A1/B0/B1 */
		PCI_DEVICE(0x1D17, 0x31B2),
		.driver_data = UNCORE_PCI_DEV_DATA(KX7000_PCI_UNCORE_MC_A0, 0),
	},

	{ /* PXPTRF */
		PCI_DEVICE(0x1D17, 0x31B4),
		.driver_data = UNCORE_PCI_DEV_DATA(KX7000_PCI_UNCORE_PXPTRF, 0),
	},

	{ /* end: all zeroes */ }
};

static struct pci_driver kx7000_uncore_pci_driver = {
	.name           = "kx7000_uncore",
	.id_table       = kx7000_uncore_pci_ids,
};
/*KX7000 pci ops end*/

/*KX7000 mmio ops start*/
static void kx7000_uncore_mmio_init_box(struct zhaoxin_uncore_box *box)
{
	struct pci_dev *pdev = NULL;
	unsigned int box_ctl = uncore_mmio_box_ctl(box);
	resource_size_t addr;
	u32 pci_dword;
	int mmio_base_offset;

	pdev = pci_get_device(0x1d17, 0x31b1, pdev);
	if (!pdev)
		return;

	if (!strcmp(box->pmu->name, "iod_zdi_dl"))
		mmio_base_offset = KX7000_IOD_ZDI_DL_MMIO_BASE_OFFSET;
	else
		mmio_base_offset = KX7000_CCD_ZDI_DL_MMIO_BASE_OFFSET;

	pci_read_config_dword(pdev, mmio_base_offset, &pci_dword);
	addr = (u64)(pci_dword & KX7000_ZDI_DL_MMIO_BASE_MASK) << 32;

	pci_read_config_dword(pdev, mmio_base_offset + 4, &pci_dword);
	addr |= pci_dword & KX7000_ZDI_DL_MMIO_MEM0_MASK;

	box->io_addr = ioremap(addr, KX7000_ZDI_DL_MMIO_SIZE);
	if (!box->io_addr)
		return;

	writel(KH40000_PMON_PCI_BOX_CTL_INT, box->io_addr + box_ctl);
}

static void kx7000_uncore_mmio_disable_box(struct zhaoxin_uncore_box *box)
{
	u32 config;
	unsigned int box_ctl = uncore_mmio_box_ctl(box);

	if (!box->io_addr)
		return;

	config = readl(box->io_addr + box_ctl);
	config |= KH40000_PMON_BOX_CTL_FRZ;
	writel(config, box->io_addr + box_ctl);
}

static void kx7000_uncore_mmio_enable_box(struct zhaoxin_uncore_box *box)
{
	u32 config;
	unsigned int box_ctl = uncore_mmio_box_ctl(box);

	if (!box->io_addr)
		return;

	config = readl(box->io_addr + box_ctl);
	config &= ~KH40000_PMON_BOX_CTL_FRZ;
	writel(config, box->io_addr + box_ctl);
}

static void kx7000_uncore_mmio_enable_event(struct zhaoxin_uncore_box *box,
					    struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;

	if (!box->io_addr)
		return;

	writel(hwc->config | KH40000_PMON_CTL_EN, box->io_addr + hwc->config_base);
}

static void kx7000_uncore_mmio_disable_event(struct zhaoxin_uncore_box *box,
					     struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;

	if (!box->io_addr)
		return;

	writel(hwc->config, box->io_addr + hwc->config_base);
}

static void uncore_mmio_exit_box(struct zhaoxin_uncore_box *box)
{
	if (box->io_addr)
		iounmap(box->io_addr);
}

static u64 uncore_mmio_read_counter(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	u64 count = 0;
	u64 count_low = 0;
	u64 count_high = 0;

	if (!box->io_addr)
		return 0;

	count_high = readl(box->io_addr + event->hw.event_base) & 0xffff;
	count_low = readl(box->io_addr + event->hw.event_base + 4);
	count = (count_high << 32) + count_low;

	return count;
}

static struct zhaoxin_uncore_ops kx7000_uncore_mmio_ops = {
	.init_box	= kx7000_uncore_mmio_init_box,
	.exit_box	= uncore_mmio_exit_box,
	.disable_box	= kx7000_uncore_mmio_disable_box,
	.enable_box	= kx7000_uncore_mmio_enable_box,
	.disable_event	= kx7000_uncore_mmio_disable_event,
	.enable_event	= kx7000_uncore_mmio_enable_event,
	.read_counter	= uncore_mmio_read_counter,
};

static struct zhaoxin_uncore_type kx7000_uncore_iod_zdi_dl = {
	.name		= "iod_zdi_dl",
	.num_counters   = 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.perf_ctr	= KX7000_ZDI_DL_MMIO_PMON_CTR0,
	.event_ctl	= KX7000_ZDI_DL_MMIO_PMON_CTL0,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KX7000_ZDI_DL_MMIO_PMON_BLK_CTL,
	.ops		= &kx7000_uncore_mmio_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type kx7000_uncore_ccd_zdi_dl = {
	.name		= "ccd_zdi_dl",
	.num_counters   = 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.perf_ctr	= KX7000_ZDI_DL_MMIO_PMON_CTR0,
	.event_ctl	= KX7000_ZDI_DL_MMIO_PMON_CTL0,
	.event_mask	= KX7000_PMON_RAW_EVENT_MASK,
	.box_ctl	= KX7000_ZDI_DL_MMIO_PMON_BLK_CTL,
	.ops		= &kx7000_uncore_mmio_ops,
	.format_group	= &kx7000_uncore_format_group,
};

static struct zhaoxin_uncore_type *kx7000_mmio_uncores[] = {
	&kx7000_uncore_iod_zdi_dl,
	&kx7000_uncore_ccd_zdi_dl,
	NULL,
};

/*KX7000 mmio ops end*/
static enum hrtimer_restart uncore_pmu_hrtimer(struct hrtimer *hrtimer)
{
	struct zhaoxin_uncore_box *box;
	struct perf_event *event;
	unsigned long flags;
	int bit;

	box = container_of(hrtimer, struct zhaoxin_uncore_box, hrtimer);
	if (!box->n_active || box->cpu != smp_processor_id())
		return HRTIMER_NORESTART;
	/*
	 * disable local interrupt to prevent uncore_pmu_event_start/stop
	 * to interrupt the update process
	 */
	local_irq_save(flags);

	/*
	 * handle boxes with an active event list as opposed to active
	 * counters
	 */
	list_for_each_entry(event, &box->active_list, active_entry) {
		uncore_perf_event_update(box, event);
	}

	for_each_set_bit(bit, box->active_mask, UNCORE_PMC_IDX_MAX)
		uncore_perf_event_update(box, box->events[bit]);

	local_irq_restore(flags);

	hrtimer_forward_now(hrtimer, ns_to_ktime(box->hrtimer_duration));
	return HRTIMER_RESTART;
}

static void uncore_pmu_start_hrtimer(struct zhaoxin_uncore_box *box)
{
	hrtimer_start(&box->hrtimer, ns_to_ktime(box->hrtimer_duration), HRTIMER_MODE_REL_PINNED);
}

static void uncore_pmu_cancel_hrtimer(struct zhaoxin_uncore_box *box)
{
	hrtimer_cancel(&box->hrtimer);
}

static void uncore_pmu_init_hrtimer(struct zhaoxin_uncore_box *box)
{
	hrtimer_init(&box->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	box->hrtimer.function = uncore_pmu_hrtimer;
}

static struct zhaoxin_uncore_box *uncore_alloc_box(struct zhaoxin_uncore_type *type, int node)
{
	int i, size, numshared = type->num_shared_regs;
	struct zhaoxin_uncore_box *box;

	size = sizeof(*box) + numshared * sizeof(struct zhaoxin_uncore_extra_reg);

	box = kzalloc_node(size, GFP_KERNEL, node);
	if (!box)
		return NULL;

	for (i = 0; i < numshared; i++)
		raw_spin_lock_init(&box->shared_regs[i].lock);

	uncore_pmu_init_hrtimer(box);
	box->cpu = -1;
	box->package_id = -1;
	box->cluster_id = -1;
	box->subnode_id = -1;

	/* set default hrtimer timeout */
	box->hrtimer_duration = UNCORE_PMU_HRTIMER_INTERVAL;

	INIT_LIST_HEAD(&box->active_list);

	return box;
}

static bool is_box_event(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	return &box->pmu->pmu == event->pmu;
}

static int uncore_collect_events(struct zhaoxin_uncore_box *box, struct perf_event *leader,
				 bool dogrp)
{
	struct perf_event *event;
	int n, max_count;

	max_count = box->pmu->type->num_counters;
	if (box->pmu->type->fixed_ctl)
		max_count++;

	if (box->n_events >= max_count)
		return -EINVAL;

	n = box->n_events;

	if (is_box_event(box, leader)) {
		box->event_list[n] = leader;
		n++;
	}

	if (!dogrp)
		return n;

	for_each_sibling_event(event, leader) {
		if (!is_box_event(box, event) ||
			event->state <= PERF_EVENT_STATE_OFF)
			continue;

		if (n >= max_count)
			return -EINVAL;

		box->event_list[n] = event;
		n++;
	}
	return n;
}

static struct event_constraint *uncore_get_event_constraint(struct zhaoxin_uncore_box *box,
							    struct perf_event *event)
{
	struct zhaoxin_uncore_type *type = box->pmu->type;
	struct event_constraint *c;

	if (type->ops->get_constraint) {
		c = type->ops->get_constraint(box, event);
		if (c)
			return c;
	}

	if (event->attr.config == UNCORE_FIXED_EVENT)
		return &uncore_constraint_fixed;

	if (type->constraints) {
		for_each_event_constraint(c, type->constraints) {
			if ((event->hw.config & c->cmask) == c->code)
				return c;
		}
	}

	return &type->unconstrainted;
}

static void uncore_put_event_constraint(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	if (box->pmu->type->ops->put_constraint)
		box->pmu->type->ops->put_constraint(box, event);
}

static int uncore_assign_events(struct zhaoxin_uncore_box *box, int assign[], int n)
{
	unsigned long used_mask[BITS_TO_LONGS(UNCORE_PMC_IDX_MAX)];
	struct event_constraint *c;
	int i, wmin, wmax, ret = 0;
	struct hw_perf_event *hwc;

	bitmap_zero(used_mask, UNCORE_PMC_IDX_MAX);

	for (i = 0, wmin = UNCORE_PMC_IDX_MAX, wmax = 0; i < n; i++) {
		c = uncore_get_event_constraint(box, box->event_list[i]);
		box->event_constraint[i] = c;
		wmin = min(wmin, c->weight);
		wmax = max(wmax, c->weight);
	}

	/* fastpath, try to reuse previous register */
	for (i = 0; i < n; i++) {
		hwc = &box->event_list[i]->hw;
		c = box->event_constraint[i];

		/* never assigned */
		if (hwc->idx == -1)
			break;

		/* constraint still honored */
		if (!test_bit(hwc->idx, c->idxmsk))
			break;

		/* not already used */
		if (test_bit(hwc->idx, used_mask))
			break;

		__set_bit(hwc->idx, used_mask);
		if (assign)
			assign[i] = hwc->idx;
	}
	/* slow path */
	if (i != n)
		ret = perf_assign_events(box->event_constraint, n,
					wmin, wmax, n, assign);

	if (!assign || ret) {
		for (i = 0; i < n; i++)
			uncore_put_event_constraint(box, box->event_list[i]);
	}
	return ret ? -EINVAL : 0;
}

static void uncore_pmu_event_start(struct perf_event *event, int flags)
{
	struct zhaoxin_uncore_box *box = uncore_event_to_box(event);
	int idx = event->hw.idx;


	if (WARN_ON_ONCE(idx == -1 || idx >= UNCORE_PMC_IDX_MAX))
		return;

	if (WARN_ON_ONCE(!(event->hw.state & PERF_HES_STOPPED)))
		return;

	event->hw.state = 0;
	box->events[idx] = event;
	box->n_active++;
	__set_bit(idx, box->active_mask);

	local64_set(&event->hw.prev_count, uncore_read_counter(box, event));
	uncore_enable_event(box, event);

	if (box->n_active == 1)
		uncore_pmu_start_hrtimer(box);
}

static void uncore_pmu_event_stop(struct perf_event *event, int flags)
{
	struct zhaoxin_uncore_box *box = uncore_event_to_box(event);
	struct hw_perf_event *hwc = &event->hw;

	if (__test_and_clear_bit(hwc->idx, box->active_mask)) {
		uncore_disable_event(box, event);
		box->n_active--;
		box->events[hwc->idx] = NULL;
		WARN_ON_ONCE(hwc->state & PERF_HES_STOPPED);
		hwc->state |= PERF_HES_STOPPED;

		if (box->n_active == 0)
			uncore_pmu_cancel_hrtimer(box);
	}

	if ((flags & PERF_EF_UPDATE) && !(hwc->state & PERF_HES_UPTODATE)) {
		/*
		 * Drain the remaining delta count out of a event
		 * that we are disabling:
		 */
		uncore_perf_event_update(box, event);
		hwc->state |= PERF_HES_UPTODATE;
	}
}

static int uncore_pmu_event_add(struct perf_event *event, int flags)
{
	struct zhaoxin_uncore_box *box = uncore_event_to_box(event);
	struct hw_perf_event *hwc = &event->hw;
	int assign[UNCORE_PMC_IDX_MAX];
	int i, n, ret;

	if (!box)
		return -ENODEV;

	ret = n = uncore_collect_events(box, event, false);
	if (ret < 0)
		return ret;

	hwc->state = PERF_HES_UPTODATE | PERF_HES_STOPPED;

	if (!(flags & PERF_EF_START))
		hwc->state |= PERF_HES_ARCH;

	ret = uncore_assign_events(box, assign, n);
	if (ret)
		return ret;

	/* save events moving to new counters */
	for (i = 0; i < box->n_events; i++) {
		event = box->event_list[i];
		hwc = &event->hw;

		if (hwc->idx == assign[i] &&
			hwc->last_tag == box->tags[assign[i]])
			continue;
		/*
		 * Ensure we don't accidentally enable a stopped
		 * counter simply because we rescheduled.
		 */
		if (hwc->state & PERF_HES_STOPPED)
			hwc->state |= PERF_HES_ARCH;

		uncore_pmu_event_stop(event, PERF_EF_UPDATE);
	}

	/* reprogram moved events into new counters */
	for (i = 0; i < n; i++) {
		event = box->event_list[i];
		hwc = &event->hw;

		if (hwc->idx != assign[i] ||
			hwc->last_tag != box->tags[assign[i]])
			uncore_assign_hw_event(box, event, assign[i]);
		else if (i < box->n_events)
			continue;

		if (hwc->state & PERF_HES_ARCH)
			continue;

		uncore_pmu_event_start(event, 0);
	}
	box->n_events = n;

	return 0;
}

static void uncore_pmu_event_del(struct perf_event *event, int flags)
{
	struct zhaoxin_uncore_box *box = uncore_event_to_box(event);
	int i;

	uncore_pmu_event_stop(event, PERF_EF_UPDATE);

	for (i = 0; i < box->n_events; i++) {
		if (event == box->event_list[i]) {
			uncore_put_event_constraint(box, event);

			for (++i; i < box->n_events; i++)
				box->event_list[i - 1] = box->event_list[i];

			--box->n_events;
			break;
		}
	}

	event->hw.idx = -1;
	event->hw.last_tag = ~0ULL;
}

static void uncore_pmu_event_read(struct perf_event *event)
{
	struct zhaoxin_uncore_box *box = uncore_event_to_box(event);

	uncore_perf_event_update(box, event);
}

static int uncore_validate_group(struct zhaoxin_uncore_pmu *pmu, struct perf_event *event)
{
	struct perf_event *leader = event->group_leader;
	struct zhaoxin_uncore_box *fake_box;
	int ret = -EINVAL, n;

	fake_box = uncore_alloc_box(pmu->type, NUMA_NO_NODE);
	if (!fake_box)
		return -ENOMEM;

	fake_box->pmu = pmu;
	/*
	 * the event is not yet connected with its
	 * siblings therefore we must first collect
	 * existing siblings, then add the new event
	 * before we can simulate the scheduling
	 */
	n = uncore_collect_events(fake_box, leader, true);
	if (n < 0)
		goto out;

	fake_box->n_events = n;
	n = uncore_collect_events(fake_box, event, false);
	if (n < 0)
		goto out;

	fake_box->n_events = n;

	ret = uncore_assign_events(fake_box, NULL, n);
out:
	kfree(fake_box);
	return ret;
}

static int uncore_pmu_event_init(struct perf_event *event)
{
	struct zhaoxin_uncore_pmu *pmu;
	struct zhaoxin_uncore_box *box;
	struct hw_perf_event *hwc = &event->hw;
	int ret;

	if (event->attr.type != event->pmu->type)
		return -ENOENT;

	pmu = uncore_event_to_pmu(event);
	/* no device found for this pmu */
	if (pmu->func_id < 0)
		return -ENOENT;

	/* Sampling not supported yet */
	if (hwc->sample_period)
		return -EINVAL;

	/*
	 * Place all uncore events for a particular physical package
	 * onto a single cpu
	 */
	if (event->cpu < 0)
		return -EINVAL;
	box = uncore_pmu_to_box(pmu, event->cpu);
	if (!box || box->cpu < 0)
		return -EINVAL;
	event->cpu = box->cpu;
	event->pmu_private = box;

	//event->event_caps |= PERF_EV_CAP_READ_ACTIVE_PKG;

	event->hw.idx = -1;
	event->hw.last_tag = ~0ULL;
	event->hw.extra_reg.idx = EXTRA_REG_NONE;
	event->hw.branch_reg.idx = EXTRA_REG_NONE;

	if (event->attr.config == UNCORE_FIXED_EVENT) {
		/* no fixed counter */
		if (!pmu->type->fixed_ctl)
			return -EINVAL;
		/*
		 * if there is only one fixed counter, only the first pmu
		 * can access the fixed counter
		 */
		if (pmu->type->single_fixed && pmu->pmu_idx > 0)
			return -EINVAL;

		/* fixed counters have event field hardcoded to zero */
		hwc->config = 0ULL;
	} else {
		hwc->config = event->attr.config &
			      (pmu->type->event_mask | ((u64)pmu->type->event_mask_ext << 32));
		if (pmu->type->ops->hw_config) {
			ret = pmu->type->ops->hw_config(box, event);
			if (ret)
				return ret;
		}
	}

	if (event->group_leader != event)
		ret = uncore_validate_group(pmu, event);
	else
		ret = 0;

	return ret;
}

static void uncore_pmu_enable(struct pmu *pmu)
{
	struct zhaoxin_uncore_pmu *uncore_pmu;
	struct zhaoxin_uncore_box *box;

	uncore_pmu = container_of(pmu, struct zhaoxin_uncore_pmu, pmu);
	if (!uncore_pmu)
		return;

	box = uncore_pmu_to_box(uncore_pmu, smp_processor_id());
	if (!box)
		return;

	if (uncore_pmu->type->ops->enable_box)
		uncore_pmu->type->ops->enable_box(box);
}

static void uncore_pmu_disable(struct pmu *pmu)
{
	struct zhaoxin_uncore_pmu *uncore_pmu;
	struct zhaoxin_uncore_box *box;

	uncore_pmu = container_of(pmu, struct zhaoxin_uncore_pmu, pmu);
	if (!uncore_pmu)
		return;

	box = uncore_pmu_to_box(uncore_pmu, smp_processor_id());
	if (!box)
		return;

	if (uncore_pmu->type->ops->disable_box)
		uncore_pmu->type->ops->disable_box(box);
}

static ssize_t cpumask_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	cpumask_t *active_mask;
	struct pmu *pmu;
	struct zhaoxin_uncore_pmu *uncore_pmu;

	pmu = dev_get_drvdata(dev);
	uncore_pmu = container_of(pmu, struct zhaoxin_uncore_pmu, pmu);

	if (boot_cpu_data.x86_model == ZHAOXIN_FAM7_KH40000) {
		if (!strcmp(uncore_pmu->type->name, "llc"))
			active_mask = &uncore_cpu_cluster_mask;
		else
			active_mask = &uncore_cpu_subnode_mask;
	} else {
		active_mask = &uncore_cpu_mask;
	}
	return cpumap_print_to_pagebuf(true, buf, active_mask);
}
static DEVICE_ATTR_RO(cpumask);

static struct attribute *uncore_pmu_attrs[] = {
	&dev_attr_cpumask.attr,
	NULL,
};

static const struct attribute_group uncore_pmu_attr_group = {
	.attrs = uncore_pmu_attrs,
};

static int uncore_pmu_register(struct zhaoxin_uncore_pmu *pmu)
{
	int ret;

	if (!pmu->type->pmu) {
		pmu->pmu = (struct pmu) {
			.attr_groups	= pmu->type->attr_groups,
			.task_ctx_nr	= perf_invalid_context,
			.pmu_enable	= uncore_pmu_enable,
			.pmu_disable	= uncore_pmu_disable,
			.event_init	= uncore_pmu_event_init,
			.add		= uncore_pmu_event_add,
			.del		= uncore_pmu_event_del,
			.start		= uncore_pmu_event_start,
			.stop		= uncore_pmu_event_stop,
			.read		= uncore_pmu_event_read,
			.module		= THIS_MODULE,
			.capabilities	= PERF_PMU_CAP_NO_EXCLUDE,
		};
	} else {
		pmu->pmu = *pmu->type->pmu;
		pmu->pmu.attr_groups = pmu->type->attr_groups;
	}

	if (pmu->type->num_boxes == 1) {
		if (strlen(pmu->type->name) > 0)
			sprintf(pmu->name, "uncore_%s", pmu->type->name);
		else
			sprintf(pmu->name, "uncore");
	} else {
		sprintf(pmu->name, "uncore_%s_%d", pmu->type->name, pmu->pmu_idx);
	}

	ret = perf_pmu_register(&pmu->pmu, pmu->name, -1);
	if (!ret)
		pmu->registered = true;
	return ret;
}

static void uncore_pmu_unregister(struct zhaoxin_uncore_pmu *pmu)
{
	if (!pmu->registered)
		return;
	perf_pmu_unregister(&pmu->pmu);
	pmu->registered = false;
}

static void uncore_free_boxes(struct zhaoxin_uncore_pmu *pmu)
{
	int i, max;

	if (boot_cpu_data.x86_model == ZHAOXIN_FAM7_KH40000) {
		if (!strcmp(pmu->type->name, "llc"))
			max = max_clusters;
		else
			max = max_subnodes;
	} else {
		max = max_packages;
	}

	for (i = 0; i < max; i++)
		kfree(pmu->boxes[i]);
	kfree(pmu->boxes);
}

static void uncore_type_exit(struct zhaoxin_uncore_type *type)
{
	struct zhaoxin_uncore_pmu *pmu = type->pmus;
	int i;

	if (pmu) {
		for (i = 0; i < type->num_boxes; i++, pmu++) {
			uncore_pmu_unregister(pmu);
			uncore_free_boxes(pmu);
		}
		kfree(type->pmus);
		type->pmus = NULL;
	}
	kfree(type->events_group);
	type->events_group = NULL;
}

static void uncore_types_exit(struct zhaoxin_uncore_type **types)
{
	for (; *types; types++)
		uncore_type_exit(*types);
}

static int __init uncore_type_init(struct zhaoxin_uncore_type *type, bool setid)
{
	struct zhaoxin_uncore_pmu *pmus;
	size_t size;
	int i, j;

	pmus = kcalloc(type->num_boxes, sizeof(*pmus), GFP_KERNEL);
	if (!pmus)
		return -ENOMEM;

	if (boot_cpu_data.x86_model == ZHAOXIN_FAM7_KH40000) {
		if (!strcmp(type->name, "llc"))
			size = max_clusters * sizeof(struct zhaoxin_uncore_box *);
		else
			size = max_subnodes * sizeof(struct zhaoxin_uncore_box *);
	} else {
		size = max_packages * sizeof(struct zhaoxin_uncore_box *);
	}

	for (i = 0; i < type->num_boxes; i++) {
		pmus[i].func_id	= setid ? i : -1;
		pmus[i].pmu_idx	= i;
		pmus[i].type	= type;
		pmus[i].boxes	= kzalloc(size, GFP_KERNEL);
		if (!pmus[i].boxes)
			goto err;
	}

	type->pmus = pmus;
	type->unconstrainted = (struct event_constraint)
				__EVENT_CONSTRAINT(0, (1ULL << type->num_counters) - 1, 0,
				type->num_counters, 0, 0);

	if (type->event_descs) {
		struct {
			struct attribute_group group;
			struct attribute *attrs[];
		} *attr_group;
		for (i = 0; type->event_descs[i].attr.attr.name; i++)
			;

		attr_group = kzalloc(struct_size(attr_group, attrs, i + 1), GFP_KERNEL);
		if (!attr_group)
			goto err;

		attr_group->group.name = "events";
		attr_group->group.attrs = attr_group->attrs;

		for (j = 0; j < i; j++)
			attr_group->attrs[j] = &type->event_descs[j].attr.attr;

		type->events_group = &attr_group->group;
	}

	type->pmu_group = &uncore_pmu_attr_group;

	return 0;

err:
	for (i = 0; i < type->num_boxes; i++)
		kfree(pmus[i].boxes);
	kfree(pmus);

	return -ENOMEM;
}

static int __init uncore_types_init(struct zhaoxin_uncore_type **types, bool setid)
{
	int ret;

	for (; *types; types++) {
		ret = uncore_type_init(*types, setid);
		if (ret)
			return ret;
	}
	return 0;
}

/*
 * add a pci uncore device
 */
static int uncore_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct zhaoxin_uncore_type *type;
	struct zhaoxin_uncore_pmu *pmu;
	struct zhaoxin_uncore_box *box;
	struct zhaoxin_uncore_box **boxes;
	char mc_dev[10];
	int loop = 1;
	int i, j = 0;
	int subnode_id = 0;
	int ret = 0;

	if (boot_cpu_data.x86_model == ZHAOXIN_FAM7_KH40000)
		subnode_id = uncore_pcibus_to_subnodeid(pdev->bus);

	type = uncore_pci_uncores[UNCORE_PCI_DEV_TYPE(id->driver_data)];

	if (boot_cpu_data.x86_model == ZHAOXIN_FAM7_KH40000) {
		strscpy(mc_dev, "mc0", sizeof("mc0"));
		if (!strcmp(type->name, mc_dev))
			loop = 2;
	} else if (boot_cpu_data.x86_model == ZHAOXIN_FAM7_KX7000) {
		strscpy(mc_dev, "mc_a0", sizeof("mc_a0"));
		if (!strcmp(type->name, mc_dev))
			loop = 4;
	}

	boxes = kcalloc(loop, sizeof(struct zhaoxin_uncore_box *), GFP_KERNEL);
	if (!boxes)
		return -ENOMEM;

	for (i = 0; i < loop; i++) {
		type = uncore_pci_uncores[UNCORE_PCI_DEV_TYPE(id->driver_data) + j];

		if (!type)
			continue;
		/*
		 * for performance monitoring unit with multiple boxes,
		 * each box has a different function id.
		 */
		pmu = &type->pmus[UNCORE_PCI_DEV_IDX(id->driver_data)];

		if (WARN_ON_ONCE(pmu->boxes[subnode_id] != NULL))
			return -EINVAL;

		box = uncore_alloc_box(type, NUMA_NO_NODE);
		if (!box)
			return -ENOMEM;

		if (pmu->func_id < 0)
			pmu->func_id = pdev->devfn;
		else
			WARN_ON_ONCE(pmu->func_id != pdev->devfn);

		atomic_inc(&box->refcnt);
		box->subnode_id = subnode_id;
		box->pci_dev = pdev;
		box->pmu = pmu;
		uncore_box_init(box);
		boxes[i] = box;

		pci_set_drvdata(pdev, boxes);
		pmu->boxes[subnode_id] = box;
		if (atomic_inc_return(&pmu->activeboxes) > 1) {
			if (!strcmp(type->name, mc_dev))
				goto next_loop;
			else
				return 0;
		}
		/* First active box registers the pmu */
		ret = uncore_pmu_register(pmu);
		if (ret) {
			pci_set_drvdata(pdev, NULL);
			pmu->boxes[subnode_id] = NULL;
			uncore_box_exit(box);
			kfree(box);
		}
next_loop:
		j++;
	}

	return ret;
}

static void uncore_pci_remove(struct pci_dev *pdev)
{
	struct zhaoxin_uncore_box **boxes;
	struct zhaoxin_uncore_box *box;
	struct zhaoxin_uncore_pmu *pmu;
	int subnode_id = 0;
	int i = 0;
	int loop = 1;

	boxes = pci_get_drvdata(pdev);

	if (boot_cpu_data.x86_model == ZHAOXIN_FAM7_KH40000) {
		if (!strcmp(boxes[0]->pmu->type->name, "mc0"))
			loop = 2;
		else
			loop = 1;
	} else if (boot_cpu_data.x86_model == ZHAOXIN_FAM7_KX7000) {
		if (!strcmp(boxes[0]->pmu->type->name, "mc_a0"))
			loop = 4;
		else
			loop = 1;
	}

	for (i = 0; i < loop; i++) {
		box = boxes[i];
		pmu = box->pmu;
		if (WARN_ON_ONCE(subnode_id != box->subnode_id))
			return;

		pci_set_drvdata(pdev, NULL);
		pmu->boxes[subnode_id] = NULL;
		if (atomic_dec_return(&pmu->activeboxes) == 0)
			uncore_pmu_unregister(pmu);

		uncore_box_exit(box);
		kfree(box);
	}

	kfree(boxes);
}

static int __init uncore_pci_init(void)
{
	int ret;

	ret = uncore_types_init(uncore_pci_uncores, false);
	if (ret)
		goto errtype;

	uncore_pci_driver->probe = uncore_pci_probe;
	uncore_pci_driver->remove = uncore_pci_remove;

	ret = pci_register_driver(uncore_pci_driver);
	if (ret)
		goto errtype;

	pcidrv_registered = true;
	return 0;

errtype:
	uncore_types_exit(uncore_pci_uncores);
	uncore_free_pcibus_map();
	uncore_pci_uncores = empty_uncore;
	return ret;
}

static void uncore_pci_exit(void)
{
	if (pcidrv_registered) {
		pcidrv_registered = false;
		pci_unregister_driver(uncore_pci_driver);
		uncore_types_exit(uncore_pci_uncores);
		uncore_free_pcibus_map();
	}
}

static void uncore_change_type_ctx(struct zhaoxin_uncore_type *type, int old_cpu, int new_cpu)
{
	struct zhaoxin_uncore_pmu *pmu = type->pmus;
	struct zhaoxin_uncore_box *box;
	int i, package_id, cluster_id = 0, subnode_id = 0;

	package_id = zx_topology_package_id(old_cpu < 0 ? new_cpu : old_cpu);
	if (boot_cpu_data.x86_model == ZHAOXIN_FAM7_KH40000) {
		cluster_id = zx_topology_cluster_id(old_cpu < 0 ? new_cpu : old_cpu);
		subnode_id = zx_topology_subnode_id(old_cpu < 0 ? new_cpu : old_cpu);
	}

	for (i = 0; i < type->num_boxes; i++, pmu++) {
		if (boot_cpu_data.x86_model == ZHAOXIN_FAM7_KH40000) {
			if (!strcmp(type->name, "llc")) {
				box = pmu->boxes[cluster_id];
				if (!box)
					continue;
			} else {
				box = pmu->boxes[subnode_id];
				if (!box)
					continue;
			}
		} else {
			box = pmu->boxes[package_id];
			if (!box)
				continue;
		}

		if (old_cpu < 0) {
			WARN_ON_ONCE(box->cpu != -1);
			box->cpu = new_cpu;
			continue;
		}
		WARN_ON_ONCE(box->cpu != old_cpu);
		box->cpu = -1;
		if (new_cpu < 0)
			continue;

		uncore_pmu_cancel_hrtimer(box);
		perf_pmu_migrate_context(&pmu->pmu, old_cpu, new_cpu);
		box->cpu = new_cpu;
	}
}

static void uncore_change_context(struct zhaoxin_uncore_type **uncores, int old_cpu, int new_cpu)
{
	for (; *uncores; uncores++)
		uncore_change_type_ctx(*uncores, old_cpu, new_cpu);
}

static void uncore_box_unref(struct zhaoxin_uncore_type **types, int id)
{
	struct zhaoxin_uncore_type *type;
	struct zhaoxin_uncore_pmu *pmu;
	struct zhaoxin_uncore_box *box;
	int i;

	for (; *types; types++) {
		type = *types;
		pmu = type->pmus;
		for (i = 0; i < type->num_boxes; i++, pmu++) {
			box = pmu->boxes[id];
			if (box && atomic_dec_return(&box->refcnt) == 0)
				uncore_box_exit(box);
		}
	}
}

struct zhaoxin_uncore_type *uncore_msr_cluster_uncores[] = {
	&kh40000_uncore_llc_box,
	NULL,
};

struct zhaoxin_uncore_type *uncore_msr_subnode_uncores[] = {
	&kh40000_uncore_hif_box,
	&kh40000_uncore_zzi_box,
	NULL,
};

struct zhaoxin_uncore_type *uncore_pci_subnode_uncores[] = {
	&kh40000_uncore_mc0,
	&kh40000_uncore_mc1,
	&kh40000_uncore_pci,
	&kh40000_uncore_zpi_dll,
	&kh40000_uncore_zdi_dll,
	&kh40000_uncore_pxptrf,
	NULL,
};

static void kx5000_event_cpu_offline(int cpu)
{
	int package, target;

	/* Check if exiting cpu is used for collecting uncore events */

	if (!cpumask_test_and_clear_cpu(cpu, &uncore_cpu_mask))
		goto unref_cpu_mask;

	/* Find a new cpu to collect uncore events */
	target = cpumask_any_but(topology_core_cpumask(cpu), cpu);

	/* Migrate uncore events to the new target */
	if (target < nr_cpu_ids)
		cpumask_set_cpu(target, &uncore_cpu_mask);
	else
		target = -1;

	uncore_change_context(uncore_msr_uncores, cpu, target);
	uncore_change_context(uncore_mmio_uncores, cpu, target);
	uncore_change_context(uncore_pci_uncores, cpu, target);

unref_cpu_mask:
	/*clear the references*/
	package = zx_topology_package_id(cpu);
	uncore_box_unref(uncore_msr_uncores, package);
	uncore_box_unref(uncore_mmio_uncores, package);
}

static void kh40000_event_cpu_offline(int cpu)
{
	int cluster_target, subnode_target;
	int cluster_id, subnode_id;

	cluster_id = zx_topology_cluster_id(cpu);
	subnode_id = zx_topology_subnode_id(cpu);

	/* Check if exiting cpu is used for collecting uncore events */

	if (cpumask_test_and_clear_cpu(cpu, &uncore_cpu_cluster_mask)) {
		cluster_target = cpumask_any_but(topology_cluster_core_cpumask(cpu), cpu);
		if (cluster_target < nr_cpu_ids)
			cpumask_set_cpu(cluster_target, &uncore_cpu_cluster_mask);
		else
			cluster_target = -1;
		uncore_change_context(uncore_msr_cluster_uncores, cpu, cluster_target);
	} else {
		uncore_box_unref(uncore_msr_cluster_uncores, cluster_id);
	}

	if (cpumask_test_and_clear_cpu(cpu, &uncore_cpu_subnode_mask)) {
		subnode_target = cpumask_any_but(topology_subnode_core_cpumask(cpu), cpu);
		if (subnode_target < nr_cpu_ids)
			cpumask_set_cpu(subnode_target, &uncore_cpu_subnode_mask);
		else
			subnode_target = -1;
		uncore_change_context(uncore_msr_subnode_uncores, cpu, subnode_target);
		uncore_change_context(uncore_pci_subnode_uncores, cpu, subnode_target);
	} else {
		uncore_box_unref(uncore_msr_subnode_uncores, subnode_id);
	}

}

static int uncore_event_cpu_offline(unsigned int cpu)
{
	unsigned int x86_model;

	x86_model = boot_cpu_data.x86_model;

	if (x86_model == ZHAOXIN_FAM7_KH40000)
		kh40000_event_cpu_offline(cpu);
	else
		kx5000_event_cpu_offline(cpu);

	return 0;
}

static int kx5000_allocate_boxes(struct zhaoxin_uncore_type **types, unsigned int id,
				 unsigned int cpu)
{
	struct zhaoxin_uncore_box *box, *tmp;
	struct zhaoxin_uncore_type *type;
	struct zhaoxin_uncore_pmu *pmu;
	LIST_HEAD(allocated);
	int i;

	/* Try to allocate all required boxes */
	for (; *types; types++) {
		type = *types;
		pmu = type->pmus;

		for (i = 0; i < type->num_boxes; i++, pmu++) {
			if (pmu->boxes[id])
				continue;
			box = uncore_alloc_box(type, cpu_to_node(cpu));
			if (!box)
				goto cleanup;
			box->pmu = pmu;
			box->package_id = id;
			list_add(&box->active_list, &allocated);
		}
	}

	/* Install them in the pmus */
	list_for_each_entry_safe(box, tmp, &allocated, active_list) {
		list_del_init(&box->active_list);
		box->pmu->boxes[id] = box;
	}
	return 0;

cleanup:
	list_for_each_entry_safe(box, tmp, &allocated, active_list) {
		list_del_init(&box->active_list);
		kfree(box);
	}
	return -ENOMEM;
}

static int kh40000_allocate_boxes(struct zhaoxin_uncore_type **types, unsigned int id,
				  unsigned int cpu)
{
	struct zhaoxin_uncore_box *box, *tmp;
	struct zhaoxin_uncore_type *type;
	struct zhaoxin_uncore_pmu *pmu;
	LIST_HEAD(allocated);
	int i;

	/* Try to allocate all required boxes */
	for (; *types; types++) {
		type = *types;
		pmu = type->pmus;

		for (i = 0; i < type->num_boxes; i++, pmu++) {
			if (pmu->boxes[id])
				continue;
			box = uncore_alloc_box(type, cpu_to_node(cpu));
			if (!box)
				goto cleanup;
			box->pmu = pmu;
			if (!strcmp(type->name, "llc"))
				box->cluster_id = id;
			else
				box->subnode_id = id;
			list_add(&box->active_list, &allocated);
		}
	}
	/* Install them in the pmus */
	list_for_each_entry_safe(box, tmp, &allocated, active_list) {
		list_del_init(&box->active_list);
		box->pmu->boxes[id] = box;
	}
	return 0;

cleanup:
	list_for_each_entry_safe(box, tmp, &allocated, active_list) {
		list_del_init(&box->active_list);
		kfree(box);
	}
	return -ENOMEM;
}

static int uncore_box_ref(struct zhaoxin_uncore_type **types, int id, unsigned int cpu)
{
	struct zhaoxin_uncore_type *type;
	struct zhaoxin_uncore_pmu *pmu;
	struct zhaoxin_uncore_box *box;
	int i, ret = 0;

	int x86_model;

	x86_model = boot_cpu_data.x86_model;

	if (x86_model == ZHAOXIN_FAM7_KH40000)
		ret = kh40000_allocate_boxes(types, id, cpu);
	else
		ret = kx5000_allocate_boxes(types, id, cpu);

	if (ret)
		return ret;

	for (; *types; types++) {
		type = *types;
		pmu = type->pmus;
		for (i = 0; i < type->num_boxes; i++, pmu++) {
			box = pmu->boxes[id];
			if (box && atomic_inc_return(&box->refcnt) == 1)
				uncore_box_init(box);
		}
	}
	return 0;
}

static int kx5000_event_cpu_online(unsigned int cpu)
{
	int package, target, msr_ret, mmio_ret;

	package = zx_topology_package_id(cpu);
	msr_ret = uncore_box_ref(uncore_msr_uncores, package, cpu);
	mmio_ret = uncore_box_ref(uncore_mmio_uncores, package, cpu);
	if (msr_ret && mmio_ret)
		return -ENOMEM;

	/*
	 * Check if there is an online cpu in the package
	 * which collects uncore events already.
	 */
	target = cpumask_any_and(&uncore_cpu_mask, topology_core_cpumask(cpu));
	if (target < nr_cpu_ids)
		return 0;

	cpumask_set_cpu(cpu, &uncore_cpu_mask);

	if (!msr_ret)
		uncore_change_context(uncore_msr_uncores, -1, cpu);
	if (!mmio_ret)
		uncore_change_context(uncore_mmio_uncores, -1, cpu);
	uncore_change_context(uncore_pci_uncores, -1, cpu);

	return 0;
}

static int kh40000_event_cpu_online(unsigned int cpu)
{
	int cluster_target, subnode_target;
	int cluster_id, subnode_id;
	int cluster_ret, subnode_ret;

	cluster_id = zx_topology_cluster_id(cpu);
	subnode_id = zx_topology_subnode_id(cpu);

	cluster_ret = uncore_box_ref(uncore_msr_cluster_uncores, cluster_id, cpu);
	subnode_ret = uncore_box_ref(uncore_msr_subnode_uncores, subnode_id, cpu);

	if (cluster_ret && subnode_ret)
		return -ENOMEM;

	/*
	 * Check if there is an online cpu in the cluster or subnode
	 * which collects uncore events already.
	 */
	cluster_target =
		cpumask_any_and(&uncore_cpu_cluster_mask, topology_cluster_core_cpumask(cpu));
	subnode_target =
		cpumask_any_and(&uncore_cpu_subnode_mask, topology_subnode_core_cpumask(cpu));

	if (cluster_target < nr_cpu_ids && subnode_target < nr_cpu_ids)
		return 0;

	if (!cluster_ret && cluster_target >= nr_cpu_ids) {
		cpumask_set_cpu(cpu, &uncore_cpu_cluster_mask);
		uncore_change_context(uncore_msr_cluster_uncores, -1, cpu);
	}

	if (!subnode_ret && subnode_target >= nr_cpu_ids) {
		cpumask_set_cpu(cpu, &uncore_cpu_subnode_mask);
		uncore_change_context(uncore_msr_subnode_uncores, -1, cpu);
		uncore_change_context(uncore_pci_subnode_uncores, -1, cpu);
	}

	return 0;
}

static int uncore_event_cpu_online(unsigned int cpu)
{
	int x86_model;
	int kx5000_ret = 0, kh40000_ret = 0;

	x86_model = boot_cpu_data.x86_model;

	if (x86_model == ZHAOXIN_FAM7_KH40000)
		kh40000_ret = kh40000_event_cpu_online(cpu);
	else
		kx5000_ret = kx5000_event_cpu_online(cpu);

	if (kx5000_ret || kh40000_ret)
		return -ENOMEM;

	return 0;
}

static int __init type_pmu_register(struct zhaoxin_uncore_type *type)
{
	int i, ret;

	for (i = 0; i < type->num_boxes; i++) {
		ret = uncore_pmu_register(&type->pmus[i]);
		if (ret)
			return ret;
	}
	return 0;
}

static int __init uncore_msr_pmus_register(void)
{
	struct zhaoxin_uncore_type **types = uncore_msr_uncores;
	int ret;

	for (; *types; types++) {
		ret = type_pmu_register(*types);
		if (ret)
			return ret;
	}
	return 0;
}

static int __init uncore_cpu_init(void)
{
	int ret;

	ret = uncore_types_init(uncore_msr_uncores, true);
	if (ret)
		goto err;

	ret = uncore_msr_pmus_register();
	if (ret)
		goto err;
	return 0;
err:
	uncore_types_exit(uncore_msr_uncores);
	uncore_msr_uncores = empty_uncore;
	return ret;
}

static int __init uncore_mmio_init(void)
{
	struct zhaoxin_uncore_type **types = uncore_mmio_uncores;
	int ret;

	ret = uncore_types_init(types, true);
	if (ret)
		goto err;

	for (; *types; types++) {
		ret = type_pmu_register(*types);
		if (ret)
			goto err;
	}
	return 0;
err:
	uncore_types_exit(uncore_mmio_uncores);
	uncore_mmio_uncores = empty_uncore;
	return ret;
}

struct zhaoxin_uncore_init_fun {
	void (*cpu_init)(void);
	int (*pci_init)(void);
	void (*mmio_init)(void);
};

void kx5000_uncore_cpu_init(void)
{
	uncore_msr_uncores = kx5000_msr_uncores;
}

static const struct zhaoxin_uncore_init_fun kx5000_uncore_init __initconst = {
	.cpu_init = kx5000_uncore_cpu_init,
};

void kh40000_uncore_cpu_init(void)
{
	uncore_msr_uncores = kh40000_msr_uncores;
}

int kh40000_uncore_pci_init(void)
{
	int ret = kh40000_pci2node_map_init(); /*pci_bus to package mapping, do nothing*/

	if (ret)
		return ret;
	uncore_pci_uncores = kh40000_pci_uncores;
	uncore_pci_driver = &kh40000_uncore_pci_driver;
	return 0;
}

static const struct zhaoxin_uncore_init_fun kh40000_uncore_init __initconst = {
	.cpu_init = kh40000_uncore_cpu_init,
	.pci_init = kh40000_uncore_pci_init,
};

void kx7000_uncore_cpu_init(void)
{
	u64 val;
	int cpu;

	uncore_msr_uncores = kx7000_msr_uncores;

	/* clear bit 16 of MSR 0x1877 so that HIF can work normally */
	for_each_present_cpu(cpu) {
		rdmsrl_on_cpu(cpu, 0x1877, &val);
		val = val & 0xfffffffffffeffffULL;
		wrmsrl_on_cpu(cpu, 0x1877, val);
	}
}

int kx7000_uncore_pci_init(void)
{
	uncore_pci_uncores = kx7000_pci_uncores;
	uncore_pci_driver = &kx7000_uncore_pci_driver;

	return 0;
}

void kx7000_uncore_mmio_init(void)
{
	uncore_mmio_uncores = kx7000_mmio_uncores;
}

static const struct zhaoxin_uncore_init_fun kx7000_uncore_init __initconst = {
	.cpu_init = kx7000_uncore_cpu_init,
	.pci_init = kx7000_uncore_pci_init,
	.mmio_init = kx7000_uncore_mmio_init,
};

static const struct x86_cpu_id zhaoxin_uncore_match[] __initconst = {
	X86_MATCH_VENDOR_FAM_MODEL(CENTAUR, 7, ZHAOXIN_FAM7_KX5000, &kx5000_uncore_init),
	X86_MATCH_VENDOR_FAM_MODEL(CENTAUR, 7, ZHAOXIN_FAM7_KX6000, &kx5000_uncore_init),
	X86_MATCH_VENDOR_FAM_MODEL(CENTAUR, 7, ZHAOXIN_FAM7_KH40000, &kh40000_uncore_init),
	X86_MATCH_VENDOR_FAM_MODEL(CENTAUR, 7, ZHAOXIN_FAM7_KX7000, &kx7000_uncore_init),
	X86_MATCH_VENDOR_FAM_MODEL(ZHAOXIN, 7, ZHAOXIN_FAM7_KX5000, &kx5000_uncore_init),
	X86_MATCH_VENDOR_FAM_MODEL(ZHAOXIN, 7, ZHAOXIN_FAM7_KX6000, &kx5000_uncore_init),
	X86_MATCH_VENDOR_FAM_MODEL(ZHAOXIN, 7, ZHAOXIN_FAM7_KH40000, &kh40000_uncore_init),
	X86_MATCH_VENDOR_FAM_MODEL(ZHAOXIN, 7, ZHAOXIN_FAM7_KX7000, &kx7000_uncore_init),
	{},
};
MODULE_DEVICE_TABLE(x86cpu, zhaoxin_uncore_match);

static int __init zhaoxin_uncore_init(void)
{
	const struct x86_cpu_id *id = NULL;
	struct zhaoxin_uncore_init_fun *uncore_init;
	int pret = 0, cret = 0, mret = 0, ret;

	id = x86_match_cpu(zhaoxin_uncore_match);
	if (!id)
		return -ENODEV;

	if (boot_cpu_has(X86_FEATURE_HYPERVISOR))
		return -ENODEV;

	pr_info("welcome to uncore.\n");

	get_topology_number();
	get_topology_info();

	if (boot_cpu_data.x86_model == ZHAOXIN_FAM7_KH40000) {
		zx_gen_core_map();
		get_pcibus_limit();
	}

	uncore_init = (struct zhaoxin_uncore_init_fun *)id->driver_data;

	if (uncore_init->pci_init) {
		pret = uncore_init->pci_init();
		if (!pret)
			pret = uncore_pci_init();
	}

	if (uncore_init->cpu_init) {
		uncore_init->cpu_init();
		cret = uncore_cpu_init();
	}

	if (uncore_init->mmio_init) {
		uncore_init->mmio_init();
		mret = uncore_mmio_init();
	}

	if (cret && pret && mret)
		return -ENODEV;

	ret = cpuhp_setup_state(CPUHP_AP_PERF_X86_UNCORE_ONLINE,
				"perf/x86/zhaoxin/uncore:online",
				uncore_event_cpu_online,
				uncore_event_cpu_offline);
	if (ret)
		goto err;
	pr_info("uncore init success!\n");

	return 0;

err:
	uncore_types_exit(uncore_msr_uncores);
	uncore_types_exit(uncore_mmio_uncores);
	uncore_pci_exit();
	pr_info("uncore init fail!\n");

	return ret;
}
module_init(zhaoxin_uncore_init);

static void __exit zhaoxin_uncore_exit(void)
{
	cpuhp_remove_state(CPUHP_AP_PERF_X86_UNCORE_ONLINE);
	uncore_types_exit(uncore_msr_uncores);
	uncore_types_exit(uncore_mmio_uncores);
	uncore_pci_exit();
}
module_exit(zhaoxin_uncore_exit);
