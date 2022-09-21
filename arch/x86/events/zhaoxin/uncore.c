// SPDX-License-Identifier: GPL-2.0-only
#include <asm/cpu_device_id.h>
#include "uncore.h"

static struct zhaoxin_uncore_type *empty_uncore[] = { NULL, };
static struct zhaoxin_uncore_type **uncore_msr_uncores = empty_uncore;
static struct zhaoxin_uncore_type **uncore_pci_uncores = empty_uncore;

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

/* get CPU topology register */
#define BJ_GLOBAL_STATU_MSR	0x1610
#define BJ_HDW_CONFIG_MSR	0X1628

/* CHX event control */
#define CHX_UNC_CTL_EV_SEL_MASK		0x000000ff
#define CHX_UNC_CTL_UMASK_MASK		0x0000ff00
#define CHX_UNC_CTL_EDGE_DET		(1 << 18)
#define CHX_UNC_CTL_EN				(1 << 22)
#define CHX_UNC_CTL_INVERT			(1 << 23)
#define CHX_UNC_CTL_CMASK_MASK		0x7000000
#define CHX_UNC_FIXED_CTR_CTL_EN	(1 << 0)

#define CHX_UNC_RAW_EVENT_MASK			(CHX_UNC_CTL_EV_SEL_MASK | \
							CHX_UNC_CTL_UMASK_MASK | \
							CHX_UNC_CTL_EDGE_DET | \
							CHX_UNC_CTL_INVERT | \
							CHX_UNC_CTL_CMASK_MASK)

/* CHX uncore global register */
#define CHX_UNC_PERF_GLOBAL_CTL		0x391
#define CHX_UNC_FIXED_CTR			0x394
#define CHX_UNC_FIXED_CTR_CTRL		0x395

/* CHX uncore global control */
#define CHX_UNC_GLOBAL_CTL_EN_PC_ALL	((1ULL << 4) - 1)
#define CHX_UNC_GLOBAL_CTL_EN_FC		(1ULL << 32)

/* CHX uncore register */
#define CHX_UNC_PERFEVTSEL0			0x3c0
#define CHX_UNC_UNCORE_PMC0			0x3b0

/* CNX event control */
#define CNX_PMON_CTL_EV_SEL_MASK	0x000000ff
#define CNX_PMON_CTL_UMASK_MASK		0x0000ff00
#define CNX_PMON_CTL_RST			(1 << 17)
#define CNX_PMON_CTL_EDGE_DET		(1 << 18)
#define CNX_PMON_CTL_EV_SEL_EXT		(1 << 21)
#define CNX_PMON_CTL_EN				(1 << 22)
#define CNX_PMON_CTL_INVERT			(1 << 23)
#define CNX_PMON_CTL_TRESH_MASK		0xff000000
#define CNX_PMON_RAW_EVENT_MASK		(CNX_PMON_CTL_EV_SEL_MASK | \
									CNX_PMON_CTL_UMASK_MASK | \
									CNX_PMON_CTL_EDGE_DET | \
									CNX_PMON_CTL_INVERT | \
									CNX_PMON_CTL_TRESH_MASK)

/* CNX LLC register*/
#define CNX_LLC_MSR_PMON_CTL0			0x1660
#define CNX_LLC_MSR_PMON_CTR0			0x165c
#define CNX_LLC_MSR_PMON_BLK_CTL		0x1665

/* CNX HIF register*/
#define CNX_HIF_MSR_PMON_CTL0			0x1656
#define CNX_HIF_MSR_PMON_CTR0			0x1651
#define CHX_HIF_MSR_PMON_FIXED_CTL		0x1655
#define CHX_HIF_MSR_PMON_FIXED_CTR		0x1650
#define CNX_HIF_MSR_PMON_BLK_CTL		0x165b

/* CNX ZZI(ZPI+ZOI+INI) register*/
#define CNX_ZZI_MSR_PMON_CTL0			0x166A
#define CNX_ZZI_MSR_PMON_CTR0			0x1666
#define CNX_ZZI_MSR_PMON_BLK_CTL		0x166f

/* CNX MC register*/
#define CNX_MC0_CHy_PMON_FIXED_CTL              0xf40
#define CNX_MC0_CHy_PMON_FIXED_CTR              0xf20
#define CNX_MC0_CHy_PMON_CTR0                   0xf00
#define CNX_MC0_CHy_PMON_CTL0                   0xf28
#define CNX_MC0_CHy_PMON_BLK_CTL                0xf44

#define CNX_MC1_CHy_PMON_FIXED_CTL              0xf90
#define CNX_MC1_CHy_PMON_FIXED_CTR              0xf70
#define CNX_MC1_CHy_PMON_CTR0                   0xf50
#define CNX_MC1_CHy_PMON_CTL0                   0xf78
#define CNX_MC1_CHy_PMON_BLK_CTL                0xf94

/* CNX PCI register*/
#define CNX_PCI_PMON_CTR0                       0xf00
#define CNX_PCI_PMON_CTL0                       0xf28
#define CNX_PCI_PMON_BLK_CTL                    0xf44

/* CNX ZPI_DLL register*/
#define CNX_ZPI_DLL_PMON_FIXED_CTL              0xf40
#define CNX_ZPI_DLL_PMON_FIXED_CTR              0xf20
#define CNX_ZPI_DLL_PMON_CTR0                   0xf00
#define CNX_ZPI_DLL_PMON_CTL0                   0xf28
#define CNX_ZPI_DLL_PMON_BLK_CTL                0xf44

/* CNX ZDI_DLL register*/
#define CNX_ZDI_DLL_PMON_FIXED_CTL              0xf40
#define CNX_ZDI_DLL_PMON_FIXED_CTR              0xf20
#define CNX_ZDI_DLL_PMON_CTR0                   0xf00
#define CNX_ZDI_DLL_PMON_CTL0                   0xf28
#define CNX_ZDI_DLL_PMON_BLK_CTL                0xf44

/* CNX PXPTRF register*/
#define CNX_PXPTRF_PMON_CTR0                    0xf00
#define CNX_PXPTRF_PMON_CTL0                    0xf28
#define CNX_PXPTRF_PMON_BLK_CTL                 0xf44

/* CNX Box level control */
#define CNX_PMON_BOX_CTL_RST_CTRL	(1 << 0)
#define CNX_PMON_BOX_CTL_RST_CTRS	(1 << 1)
#define CNX_PMON_BOX_CTL_FRZ            (1 << 8)
//#define CNX_PMON_BOX_CTL_FRZ_EN       (1 << 16)
#define CNX_PMON_PCI_BOX_PMON_EN	(1 << 31)

#define CNX_PMON_BOX_CTL_INT		(CNX_PMON_BOX_CTL_RST_CTRL | \
						CNX_PMON_BOX_CTL_RST_CTRS)

#define CNX_PMON_PCI_BOX_CTL_INT	(CNX_PMON_BOX_CTL_RST_CTRL | \
						CNX_PMON_BOX_CTL_RST_CTRS | \
						CNX_PMON_PCI_BOX_PMON_EN)

DEFINE_UNCORE_FORMAT_ATTR(event, event, "config:0-7");
DEFINE_UNCORE_FORMAT_ATTR(umask, umask, "config:8-15");
DEFINE_UNCORE_FORMAT_ATTR(edge, edge, "config:18");
DEFINE_UNCORE_FORMAT_ATTR(inv, inv, "config:23");
DEFINE_UNCORE_FORMAT_ATTR(cmask3, cmask, "config:24-26");
DEFINE_UNCORE_FORMAT_ATTR(cmask8, cmask, "config:24-31");

static void get_hw_info_msr(void *info)
{
	struct hw_info *data = info;

	rdmsrl(BJ_HDW_CONFIG_MSR, data->config_info);
	rdmsrl(BJ_GLOBAL_STATU_MSR, data->active_state);
}

/*topology info : get max cluster*/
static int topology_clusters(void)
{
	int cpu;
	int clusters = 0;
	int tmp_clusters;
	struct hw_info data;

	u64 sdnc = ~0ULL;  //socket_die_subnode_cluster
	u64 config;
	u64 state;

	for_each_present_cpu(cpu) {
		smp_call_function_single(cpu, get_hw_info_msr, &data, 1);
		config = data.config_info;
		state = data.active_state;

		config &= 0x3f << 18;
		state &= 0x3 << 6;
		state >>= 6;

		if (state == 0)
			tmp_clusters = 0;
		else if (state == 0x1 || state == 0x2)
			tmp_clusters = 1;
		else
			tmp_clusters = 2;

		if (clusters_per_subnode < tmp_clusters)
			clusters_per_subnode = tmp_clusters;

		if (config != sdnc)
			clusters++;
		sdnc = config;
	}

	return clusters;
}

static int topology_subnodes(void)
{
	int cpu;
	int subnodes = 0;
	struct hw_info data;

	u64 sdn = ~0ULL;  //socket_die_subnode
	u64 config;

	int die_info;
	int tmp_dies;
	int subnode_info;
	int tmp_subnodes;

	for_each_present_cpu(cpu) {
		smp_call_function_single(cpu, get_hw_info_msr, &data, 1);
		config = data.config_info;

		die_info = (int)(config & (0x3 << 21));
		tmp_dies = (die_info >> 21) + 1;
		if (dies_per_socket < tmp_dies)
			dies_per_socket = tmp_dies;

		subnode_info = (int)(config & (0x1 << 20));
		tmp_subnodes = (subnode_info >> 20) + 1;
		if (subnodes_per_die < tmp_subnodes)
			subnodes_per_die = tmp_subnodes;

		config &= 0xf << 20;  //bit20~bit23

		if (config != sdn)
			subnodes++;
		sdn = config;
	}

	return subnodes;
}

static inline int uncore_pcibus_to_subnodeid(struct pci_bus *bus)
{
	int numbers_per_subnodes = 256/max_subnodes;

	return bus->number/numbers_per_subnodes;
}

DEFINE_PER_CPU(int, zx_subnode_id);
DEFINE_PER_CPU(int, zx_cluster_id);

static void get_cluster_info(void)
{
	int cpu;
	int cluster_id;
	int socket_id;
	int die_id;
	int subnode_id;
	struct hw_info data;

	int socket_info;
	int die_info;
	int subnode_info;
	int cluster_info;

	u64 config;

	for_each_present_cpu(cpu) {
		smp_call_function_single(cpu, get_hw_info_msr, &data, 1);
		config = data.config_info;

		socket_info = (int)(config & (0x1 << 23));
		socket_info >>= 23;
		socket_id = socket_info;

		die_info = (int)(config & (0x3 << 21));
		die_info >>= 21;
		die_id = socket_id * dies_per_socket + die_info;

		subnode_info = (int)(config & (0x1 << 20));
		subnode_info >>= 20;
		subnode_id = die_id * subnodes_per_die + subnode_info;

		cluster_info = (int)(config & (0x3 << 18));
		cluster_info >>= 18;

		cluster_id = subnode_id * clusters_per_subnode + cluster_info;

		per_cpu(zx_cluster_id, cpu) = cluster_id;
	}
}

static void get_subnode_info(void)
{
	int cpu;
	int socket_id;
	int die_id;
	int subnode_id;
	struct hw_info data;

	int socket_info;
	int die_info;
	int subnode_info;

	u64 config;

	for_each_present_cpu(cpu) {
		smp_call_function_single(cpu, get_hw_info_msr, &data, 1);
		config = data.config_info;

		socket_info = (int)(config & (0x1 << 23));
		socket_info >>= 23;
		socket_id = socket_info;

		die_info = (int)(config & (0x3 << 21));
		die_info >>= 21;
		die_id = socket_id * dies_per_socket + die_info;

		subnode_info = (int)(config & (0x1 << 20));
		subnode_info >>= 20;
		subnode_id = die_id * subnodes_per_die + subnode_info;

		per_cpu(zx_subnode_id, cpu) = subnode_id;
	}
}

static int topology_cluster_id(int cpu)
{
	int cluster_id;

	cluster_id = per_cpu(zx_cluster_id, cpu);

	return cluster_id;
}

static int topology_subnode_id(int cpu)
{
	int subnode_id;

	subnode_id = per_cpu(zx_subnode_id, cpu);

	return subnode_id;
}

static cpumask_t cluster_core_bits[NR_CPUS];
static cpumask_t subnode_core_bits[NR_CPUS];

static void zx_gen_core_map(void)
{
	int i, nr, cpu;
	int cluster_id, subnode_id;

	for_each_present_cpu(cpu) {
		cluster_id = topology_cluster_id(cpu);
		for (i = 0; i < 4; i++) {
			nr = (cluster_id << 2) + i;
			cpumask_set_cpu(nr, &cluster_core_bits[cpu]);
		}
	}

	for_each_present_cpu(cpu) {
		subnode_id = topology_subnode_id(cpu);
		for (i = 0; i < 8; i++) {
			nr = (subnode_id << 3) + i;
			cpumask_set_cpu(nr, &subnode_core_bits[cpu]);
		}
	}
}

static struct cpumask *topology_cluster_core_cpumask(int cpu)
{
	return &cluster_core_bits[cpu];
}

static struct cpumask *topology_subnode_core_cpumask(int cpu)
{
	return &subnode_core_bits[cpu];
}

static void uncore_free_pcibus_map(void)
{

}

static int cnx_pci2node_map_init(void)
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
	if (boot_cpu_data.x86_model == 0x5b) {
		if (!strcmp(pmu->type->name, "llc"))
			return pmu->boxes[topology_cluster_id(cpu)];
		else
			return pmu->boxes[topology_subnode_id(cpu)];
	} else {
		return pmu->boxes[topology_logical_package_id(cpu)];
	}
}

static u64 uncore_msr_read_counter(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	u64 count;

	WARN_ON_ONCE(box->cpu != smp_processor_id());
	rdmsrl(event->hw.event_base, count);
	return count;
}

static void uncore_assign_hw_event(struct zhaoxin_uncore_box *box,
				   struct perf_event *event, int idx)
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

/*CHX uncore ops start*/
static void chx_uncore_msr_disable_event(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	wrmsrl(event->hw.config_base, 0);
}

static void chx_uncore_msr_disable_box(struct zhaoxin_uncore_box *box)
{
	wrmsrl(CHX_UNC_PERF_GLOBAL_CTL, 0);
}

static void chx_uncore_msr_enable_box(struct zhaoxin_uncore_box *box)
{
	wrmsrl(CHX_UNC_PERF_GLOBAL_CTL, CHX_UNC_GLOBAL_CTL_EN_PC_ALL | CHX_UNC_GLOBAL_CTL_EN_FC);
}

static void chx_uncore_msr_enable_event(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;

	if (hwc->idx < UNCORE_PMC_IDX_FIXED)
		wrmsrl(hwc->config_base, hwc->config | CHX_UNC_CTL_EN);
	else
		wrmsrl(hwc->config_base, CHX_UNC_FIXED_CTR_CTL_EN);
}

static struct attribute *chx_uncore_formats_attr[] = {
	&format_attr_event.attr,
	&format_attr_umask.attr,
	&format_attr_edge.attr,
	&format_attr_inv.attr,
	&format_attr_cmask3.attr,
	NULL,
};

static struct attribute_group chx_uncore_format_group = {
	.name = "format",
	.attrs = chx_uncore_formats_attr,
};

static struct uncore_event_desc chx_uncore_events[] = {
	{ /* end: all zeroes */ },
};

static struct zhaoxin_uncore_ops chx_uncore_msr_ops = {
	.disable_box	= chx_uncore_msr_disable_box,
	.enable_box	= chx_uncore_msr_enable_box,
	.disable_event	= chx_uncore_msr_disable_event,
	.enable_event	= chx_uncore_msr_enable_event,
	.read_counter	= uncore_msr_read_counter,
};

static struct zhaoxin_uncore_type chx_uncore_box = {
	.name		= "chx",
	.num_counters   = 4,
	.num_boxes	= 1,
	.perf_ctr_bits	= 48,
	.fixed_ctr_bits	= 48,
	.event_ctl	= CHX_UNC_PERFEVTSEL0,
	.perf_ctr	= CHX_UNC_UNCORE_PMC0,
	.fixed_ctr	= CHX_UNC_FIXED_CTR,
	.fixed_ctl	= CHX_UNC_FIXED_CTR_CTRL,
	.event_mask	= CHX_UNC_RAW_EVENT_MASK,
	.event_descs	= chx_uncore_events,
	.ops		= &chx_uncore_msr_ops,
	.format_group	= &chx_uncore_format_group,
};

static struct zhaoxin_uncore_type *chx_msr_uncores[] = {
	&chx_uncore_box,
	NULL,
};
/*CHX uncore ops end*/

/*CNX msr ops start*/
static void cnx_uncore_msr_disable_event(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;

	wrmsrl(hwc->config_base, hwc->config);
}

static void cnx_uncore_msr_enable_event(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;

	wrmsrl(hwc->config_base, hwc->config | CNX_PMON_CTL_EN);
}

static void cnx_uncore_msr_disable_box(struct zhaoxin_uncore_box *box)
{
	u64 config;
	unsigned int msr;

	msr = uncore_msr_box_ctl(box);
	if (msr) {
		rdmsrl(msr, config);
		config |= CNX_PMON_BOX_CTL_FRZ;
		wrmsrl(msr, config);
	}
}

static void cnx_uncore_msr_enable_box(struct zhaoxin_uncore_box *box)
{
	u64 config;
	unsigned int msr;

	msr = uncore_msr_box_ctl(box);
	if (msr) {
		rdmsrl(msr, config);
		config &= ~CNX_PMON_BOX_CTL_FRZ;
		wrmsrl(msr, config);
	}
}

static void cnx_uncore_msr_init_box(struct zhaoxin_uncore_box *box)
{
	unsigned int msr = uncore_msr_box_ctl(box);

	if (msr) {
		wrmsrl(msr, CNX_PMON_BOX_CTL_INT);
		wrmsrl(msr, 0);
	}
}

static struct attribute *cnx_uncore_formats_attr[] = {
	&format_attr_event.attr,
	&format_attr_umask.attr,
	&format_attr_edge.attr,
	&format_attr_inv.attr,
	&format_attr_cmask8.attr,
	NULL,
};

static struct attribute_group cnx_uncore_format_group = {
	.name = "format",
	.attrs = cnx_uncore_formats_attr,
};

static struct uncore_event_desc cnx_uncore_llc_box_events[] = {
	{ /* end: all zeroes */ },
};

static struct uncore_event_desc cnx_uncore_hif_box_events[] = {
	{ /* end: all zeroes */ },
};

static struct uncore_event_desc cnx_uncore_zzi_box_events[] = {
	{ /* end: all zeroes */ },
};

static struct zhaoxin_uncore_ops cnx_uncore_msr_ops = {
	.init_box       = cnx_uncore_msr_init_box,
	.disable_box    = cnx_uncore_msr_disable_box,
	.enable_box     = cnx_uncore_msr_enable_box,
	.disable_event  = cnx_uncore_msr_disable_event,
	.enable_event   = cnx_uncore_msr_enable_event,
	.read_counter   = uncore_msr_read_counter,
};

static struct zhaoxin_uncore_type cnx_uncore_llc_box = {
	.name           = "llc",
	.num_counters   = 4,
	.num_boxes      = 1,
	.perf_ctr_bits  = 48,
	.event_ctl      = CNX_LLC_MSR_PMON_CTL0,
	.perf_ctr       = CNX_LLC_MSR_PMON_CTR0,
	.event_mask     = CNX_PMON_RAW_EVENT_MASK,
	.box_ctl        = CNX_LLC_MSR_PMON_BLK_CTL,
	.event_descs    = cnx_uncore_llc_box_events,
	.ops            = &cnx_uncore_msr_ops,
	.format_group   = &cnx_uncore_format_group,
};

static struct zhaoxin_uncore_type cnx_uncore_hif_box = {
	.name           = "hif",
	.num_counters   = 4,
	.num_boxes      = 1,
	.perf_ctr_bits  = 48,
	.fixed_ctr_bits = 48,
	.event_ctl      = CNX_HIF_MSR_PMON_CTL0,
	.perf_ctr       = CNX_HIF_MSR_PMON_CTR0,
	.fixed_ctr      = CHX_HIF_MSR_PMON_FIXED_CTR,
	.fixed_ctl      = CHX_HIF_MSR_PMON_FIXED_CTL,
	.event_mask     = CNX_PMON_RAW_EVENT_MASK,
	.box_ctl        = CNX_HIF_MSR_PMON_BLK_CTL,
	.event_descs    = cnx_uncore_hif_box_events,
	.ops            = &cnx_uncore_msr_ops,
	.format_group   = &cnx_uncore_format_group,
};

static struct zhaoxin_uncore_type cnx_uncore_zzi_box = {
	.name           = "zzi",
	.num_counters   = 4,
	.num_boxes      = 1,
	.perf_ctr_bits  = 48,
	.event_ctl      = CNX_ZZI_MSR_PMON_CTL0,
	.perf_ctr       = CNX_ZZI_MSR_PMON_CTR0,
	.event_mask     = CNX_PMON_RAW_EVENT_MASK,
	.box_ctl        = CNX_ZZI_MSR_PMON_BLK_CTL,
	.event_descs    = cnx_uncore_zzi_box_events,
	.ops            = &cnx_uncore_msr_ops,
	.format_group   = &cnx_uncore_format_group,
};

static struct zhaoxin_uncore_type *cnx_msr_uncores[] = {
	&cnx_uncore_llc_box,
	&cnx_uncore_hif_box,
	&cnx_uncore_zzi_box,
	NULL,
};
/*CNX msr ops end*/

/*CNX pci ops start*/
static void cnx_uncore_pci_disable_event(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	struct pci_dev *pdev = box->pci_dev;
	struct hw_perf_event *hwc = &event->hw;

	pci_write_config_dword(pdev, hwc->config_base, hwc->config);
}

static void cnx_uncore_pci_enable_event(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	struct pci_dev *pdev = box->pci_dev;
	struct hw_perf_event *hwc = &event->hw;

	pci_write_config_dword(pdev, hwc->config_base, hwc->config | CNX_PMON_CTL_EN);
}

static void cnx_uncore_pci_disable_box(struct zhaoxin_uncore_box *box)
{
	struct pci_dev *pdev = box->pci_dev;
	int box_ctl = uncore_pci_box_ctl(box);
	u32 config = 0;

	if (!pci_read_config_dword(pdev, box_ctl, &config)) {
		config |= CNX_PMON_BOX_CTL_FRZ;
		pci_write_config_dword(pdev, box_ctl, config);
	}
}

static void cnx_uncore_pci_enable_box(struct zhaoxin_uncore_box *box)
{
	struct pci_dev *pdev = box->pci_dev;
	int box_ctl = uncore_pci_box_ctl(box);
	u32 config = 0;

	if (!pci_read_config_dword(pdev, box_ctl, &config)) {
		config &= ~CNX_PMON_BOX_CTL_FRZ;
		pci_write_config_dword(pdev, box_ctl, config);
	}
}

static u64 cnx_uncore_pci_read_counter(struct zhaoxin_uncore_box *box, struct perf_event *event)
{
	struct pci_dev *pdev = box->pci_dev;
	struct hw_perf_event *hwc = &event->hw;
	u64 count = 0;

	pci_read_config_dword(pdev, hwc->event_base, (u32 *)&count + 1);
	pci_read_config_dword(pdev, hwc->event_base + 4, (u32 *)&count);
	return count;
}

static void cnx_uncore_pci_init_box(struct zhaoxin_uncore_box *box)
{
	struct pci_dev *pdev = box->pci_dev;
	int box_ctl = uncore_pci_box_ctl(box);

	pci_write_config_dword(pdev, box_ctl, CNX_PMON_PCI_BOX_CTL_INT);
}

static struct uncore_event_desc cnx_uncore_imc_events[] = {
	{ /* end: all zeroes */ },
};

static struct uncore_event_desc cnx_uncore_pci_events[] = {
	{ /* end: all zeroes */ },
};

static struct uncore_event_desc cnx_uncore_zpi_dll_events[] = {
	{ /* end: all zeroes */ },
};

static struct uncore_event_desc cnx_uncore_zdi_dll_events[] = {
	{ /* end: all zeroes */ },
};

static struct uncore_event_desc cnx_uncore_pxptrf_events[] = {
	{ /* end: all zeroes */ },
};

static struct zhaoxin_uncore_ops cnx_uncore_pci_ops = {
	.init_box       = cnx_uncore_pci_init_box,
	.disable_box    = cnx_uncore_pci_disable_box,
	.enable_box     = cnx_uncore_pci_enable_box,
	.disable_event  = cnx_uncore_pci_disable_event,
	.enable_event   = cnx_uncore_pci_enable_event,
	.read_counter   = cnx_uncore_pci_read_counter
};

static struct zhaoxin_uncore_type cnx_uncore_mc0 = {
	.name           = "mc0",
	.num_counters   = 4,
	.num_boxes      = 1,
	.perf_ctr_bits  = 48,
	.fixed_ctr_bits = 48,
	.fixed_ctr      = CNX_MC0_CHy_PMON_FIXED_CTR,
	.fixed_ctl      = CNX_MC0_CHy_PMON_FIXED_CTL,
	.event_descs    = cnx_uncore_imc_events,
	.perf_ctr       = CNX_MC0_CHy_PMON_CTR0,
	.event_ctl      = CNX_MC0_CHy_PMON_CTL0,
	.event_mask     = CNX_PMON_RAW_EVENT_MASK,
	.box_ctl        = CNX_MC0_CHy_PMON_BLK_CTL,
	.ops            = &cnx_uncore_pci_ops,
	.format_group   = &cnx_uncore_format_group
};

static struct zhaoxin_uncore_type cnx_uncore_mc1 = {
	.name           = "mc1",
	.num_counters   = 4,
	.num_boxes      = 1,
	.perf_ctr_bits  = 48,
	.fixed_ctr_bits = 48,
	.fixed_ctr      = CNX_MC1_CHy_PMON_FIXED_CTR,
	.fixed_ctl      = CNX_MC1_CHy_PMON_FIXED_CTL,
	.event_descs    = cnx_uncore_imc_events,
	.perf_ctr       = CNX_MC1_CHy_PMON_CTR0,
	.event_ctl      = CNX_MC1_CHy_PMON_CTL0,
	.event_mask     = CNX_PMON_RAW_EVENT_MASK,
	.box_ctl        = CNX_MC1_CHy_PMON_BLK_CTL,
	.ops            = &cnx_uncore_pci_ops,
	.format_group   = &cnx_uncore_format_group
};

static struct zhaoxin_uncore_type cnx_uncore_pci = {
	.name           = "pci",
	.num_counters   = 4,
	.num_boxes      = 2,
	.perf_ctr_bits  = 48,
	.event_descs    = cnx_uncore_pci_events,
	.perf_ctr       = CNX_PCI_PMON_CTR0,
	.event_ctl      = CNX_PCI_PMON_CTL0,
	.event_mask     = CNX_PMON_RAW_EVENT_MASK,
	.box_ctl        = CNX_PCI_PMON_BLK_CTL,
	.ops            = &cnx_uncore_pci_ops,
	.format_group   = &cnx_uncore_format_group
};

static struct zhaoxin_uncore_type cnx_uncore_zpi_dll = {
	.name           = "zpi_dll",
	.num_counters   = 4,
	.num_boxes      = 1,
	.perf_ctr_bits  = 48,
	.event_descs    = cnx_uncore_zpi_dll_events,
	.perf_ctr       = CNX_ZPI_DLL_PMON_CTR0,
	.event_ctl      = CNX_ZPI_DLL_PMON_CTL0,
	.event_mask     = CNX_PMON_RAW_EVENT_MASK,
	.box_ctl        = CNX_ZPI_DLL_PMON_BLK_CTL,
	.ops            = &cnx_uncore_pci_ops,
	.format_group   = &cnx_uncore_format_group
};

static struct zhaoxin_uncore_type cnx_uncore_zdi_dll = {
	.name           = "zdi_dll",
	.num_counters   = 4,
	.num_boxes      = 1,
	.perf_ctr_bits  = 48,
	.event_descs    = cnx_uncore_zdi_dll_events,
	.perf_ctr       = CNX_ZDI_DLL_PMON_CTR0,
	.event_ctl      = CNX_ZDI_DLL_PMON_CTL0,
	.event_mask     = CNX_PMON_RAW_EVENT_MASK,
	.box_ctl        = CNX_ZDI_DLL_PMON_BLK_CTL,
	.ops            = &cnx_uncore_pci_ops,
	.format_group   = &cnx_uncore_format_group
};

static struct zhaoxin_uncore_type cnx_uncore_pxptrf = {
	.name           = "pxptrf",
	.num_counters   = 4,
	.num_boxes      = 1,
	.perf_ctr_bits  = 48,
	.event_descs    = cnx_uncore_pxptrf_events,
	.perf_ctr       = CNX_PXPTRF_PMON_CTR0,
	.event_ctl      = CNX_PXPTRF_PMON_CTL0,
	.event_mask     = CNX_PMON_RAW_EVENT_MASK,
	.box_ctl        = CNX_PXPTRF_PMON_BLK_CTL,
	.ops            = &cnx_uncore_pci_ops,
	.format_group   = &cnx_uncore_format_group
};

enum {
	CNX_PCI_UNCORE_MC0,
	CNX_PCI_UNCORE_MC1,
	CNX_PCI_UNCORE_PCI,
	CNX_PCI_UNCORE_ZPI_DLL,
	CNX_PCI_UNCORE_ZDI_DLL,
	CNX_PCI_UNCORE_PXPTRF,
};

static struct zhaoxin_uncore_type *cnx_pci_uncores[] = {
	[CNX_PCI_UNCORE_MC0]            = &cnx_uncore_mc0,
	[CNX_PCI_UNCORE_MC1]            = &cnx_uncore_mc1,
	[CNX_PCI_UNCORE_PCI]            = &cnx_uncore_pci,
	[CNX_PCI_UNCORE_ZPI_DLL]        = &cnx_uncore_zpi_dll,
	[CNX_PCI_UNCORE_ZDI_DLL]        = &cnx_uncore_zdi_dll,
	[CNX_PCI_UNCORE_PXPTRF]         = &cnx_uncore_pxptrf,
	NULL,
};

static const struct pci_device_id cnx_uncore_pci_ids[] = {
	{ /* MC Channe0/1 */
		PCI_DEVICE(0x1D17, 0x31b2),
		.driver_data = UNCORE_PCI_DEV_DATA(CNX_PCI_UNCORE_MC0, 0),
	},

	{ /* PCIA */
		PCI_DEVICE(0x1D17, 0x0717),
		.driver_data = UNCORE_PCI_DEV_DATA(CNX_PCI_UNCORE_PCI, 0),
	},

	{ /* PCIB */
		PCI_DEVICE(0x1D17, 0x071c),
		.driver_data = UNCORE_PCI_DEV_DATA(CNX_PCI_UNCORE_PCI, 1),
	},

	{ /* ZPI_DLL */
		PCI_DEVICE(0x1D17, 0x91c1),
		.driver_data = UNCORE_PCI_DEV_DATA(CNX_PCI_UNCORE_ZPI_DLL, 0),
	},

	{ /* ZDI_DLL */
		PCI_DEVICE(0x1D17, 0x3b03),
		.driver_data = UNCORE_PCI_DEV_DATA(CNX_PCI_UNCORE_ZDI_DLL, 0),
	},

	{ /* PXPTRF */
		PCI_DEVICE(0x1D17, 0x31B4),
		.driver_data = UNCORE_PCI_DEV_DATA(CNX_PCI_UNCORE_PXPTRF, 0),
	},

	{ /* end: all zeroes */ }
};

static struct pci_driver cnx_uncore_pci_driver = {
	.name           = "cnx_uncore",
	.id_table       = cnx_uncore_pci_ids,
};
/*CNX pci ops end*/

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
	hrtimer_start(&box->hrtimer, ns_to_ktime(box->hrtimer_duration),
				HRTIMER_MODE_REL_PINNED);
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

static struct zhaoxin_uncore_box *uncore_alloc_box(struct zhaoxin_uncore_type *type,
						int node)
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

static int
uncore_collect_events(struct zhaoxin_uncore_box *box, struct perf_event *leader,
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

static struct event_constraint *
uncore_get_event_constraint(struct zhaoxin_uncore_box *box, struct perf_event *event)
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

static void uncore_put_event_constraint(struct zhaoxin_uncore_box *box,
					struct perf_event *event)
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

static int uncore_validate_group(struct zhaoxin_uncore_pmu *pmu,
				struct perf_event *event)
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

static ssize_t uncore_get_attr_cpumask(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	cpumask_t *active_mask;
	struct pmu *pmu;
	struct zhaoxin_uncore_pmu *uncore_pmu;

	pmu = dev_get_drvdata(dev);
	uncore_pmu = container_of(pmu, struct zhaoxin_uncore_pmu, pmu);

	if (boot_cpu_data.x86_model == 0x5b) {
		if (!strcmp(uncore_pmu->type->name, "llc"))
			active_mask = &uncore_cpu_cluster_mask;
		else
			active_mask = &uncore_cpu_subnode_mask;
	} else {
		active_mask = &uncore_cpu_mask;
	}
	return cpumap_print_to_pagebuf(true, buf, active_mask);
}
static DEVICE_ATTR(cpumask, S_IRUGO, uncore_get_attr_cpumask, NULL);

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
		sprintf(pmu->name, "uncore_%s_%d", pmu->type->name,
			pmu->pmu_idx);
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

	if (boot_cpu_data.x86_model == 0x5b) {
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

	if (boot_cpu_data.x86_model == 0x5b) {
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
		__EVENT_CONSTRAINT(0, (1ULL << type->num_counters) - 1,
				0, type->num_counters, 0, 0);

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

static int __init
uncore_types_init(struct zhaoxin_uncore_type **types, bool setid)
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
	struct zhaoxin_uncore_box *boxes[2];

	int loop, i, j = 0;

	int subnode_id, ret = 0;

	subnode_id = uncore_pcibus_to_subnodeid(pdev->bus);
	if (subnode_id < 0)
		return -EINVAL;

	type = uncore_pci_uncores[UNCORE_PCI_DEV_TYPE(id->driver_data)];

	if (!strcmp(type->name, "mc0"))
		loop = 2;
	else
		loop = 1;

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
			if (!strcmp(type->name, "mc0"))
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
	struct zhaoxin_uncore_box **boxes = pci_get_drvdata(pdev);
	struct zhaoxin_uncore_box *box;
	struct zhaoxin_uncore_pmu *pmu;
	int subnode_id;
	int i = 0;

	subnode_id = uncore_pcibus_to_subnodeid(pdev->bus);

	boxes = pci_get_drvdata(pdev);

again:
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

	if (!strcmp(box->pmu->type->name, "mc0")) {
		i++;
		goto again;
	}
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

static void uncore_change_type_ctx(struct zhaoxin_uncore_type *type, int old_cpu,
				int new_cpu)
{
	struct zhaoxin_uncore_pmu *pmu = type->pmus;
	struct zhaoxin_uncore_box *box;
	int i, package_id, cluster_id, subnode_id;

	package_id = topology_logical_package_id(old_cpu < 0 ? new_cpu : old_cpu);
	cluster_id = topology_cluster_id(old_cpu < 0 ? new_cpu : old_cpu);
	subnode_id = topology_subnode_id(old_cpu < 0 ? new_cpu : old_cpu);

	for (i = 0; i < type->num_boxes; i++, pmu++) {

		if (boot_cpu_data.x86_model == 0x5b) {
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

static void uncore_change_context(struct zhaoxin_uncore_type **uncores,
				int old_cpu, int new_cpu)
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
	&cnx_uncore_llc_box,
	NULL,
};

struct zhaoxin_uncore_type *uncore_msr_subnode_uncores[] = {
	&cnx_uncore_hif_box,
	&cnx_uncore_zzi_box,
	NULL,
};

struct zhaoxin_uncore_type *uncore_pci_subnode_uncores[] = {
	&cnx_uncore_mc0,
	&cnx_uncore_mc1,
	&cnx_uncore_pci,
	&cnx_uncore_zpi_dll,
	&cnx_uncore_zdi_dll,
	&cnx_uncore_pxptrf,
	NULL,
};

static void chx_event_cpu_offline(int cpu)
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

unref_cpu_mask:
	/*clear the references*/
	package = topology_logical_package_id(cpu);
	uncore_box_unref(uncore_msr_uncores, package);
}

static void cnx_event_cpu_offline(int cpu)
{
	int cluster_target, subnode_target;
	int cluster_id, subnode_id;

	cluster_id = topology_cluster_id(cpu);
	subnode_id = topology_subnode_id(cpu);

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

	if (x86_model == 0x5b)
		cnx_event_cpu_offline(cpu);
	else
		chx_event_cpu_offline(cpu);

	return 0;
}

static int chx_allocate_boxes(struct zhaoxin_uncore_type **types,
			unsigned int id, unsigned int cpu)
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

static int cnx_allocate_boxes(struct zhaoxin_uncore_type **types,
			unsigned int id, unsigned int cpu)
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

static int uncore_box_ref(struct zhaoxin_uncore_type **types,
			int id, unsigned int cpu)
{
	struct zhaoxin_uncore_type *type;
	struct zhaoxin_uncore_pmu *pmu;
	struct zhaoxin_uncore_box *box;
	int i, ret = 0;

	int x86_model;

	x86_model = boot_cpu_data.x86_model;

	if (x86_model == 0x5b)
		ret = cnx_allocate_boxes(types, id, cpu);
	else
		ret = chx_allocate_boxes(types, id, cpu);

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

static int chx_event_cpu_online(unsigned int cpu)
{
	int package, target, msr_ret;

	package = topology_logical_package_id(cpu);
	msr_ret = uncore_box_ref(uncore_msr_uncores, package, cpu);

	if (msr_ret)
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

	return 0;
}

static int cnx_event_cpu_online(unsigned int cpu)
{
	int cluster_target, subnode_target;
	int cluster_id, subnode_id;
	int cluster_ret, subnode_ret;

	cluster_id = topology_cluster_id(cpu);
	subnode_id = topology_subnode_id(cpu);

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
	int chx_ret = 0, cnx_ret = 0;

	x86_model = boot_cpu_data.x86_model;

	if (x86_model == 0x5b)
		cnx_ret = cnx_event_cpu_online(cpu);
	else
		chx_ret = chx_event_cpu_online(cpu);

	if (chx_ret || cnx_ret)
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

struct zhaoxin_uncore_init_fun {
	void	(*cpu_init)(void);
	int	(*pci_init)(void);
};

void chx_uncore_cpu_init(void)
{
	uncore_msr_uncores = chx_msr_uncores;
}

static const struct zhaoxin_uncore_init_fun chx_uncore_init __initconst = {
	.cpu_init = chx_uncore_cpu_init,
};

void cnx_uncore_cpu_init(void)
{
	uncore_msr_uncores = cnx_msr_uncores;
}

int cnx_uncore_pci_init(void)
{
	/* pci_bus to package mapping, do nothing */
	int ret = cnx_pci2node_map_init();

	if (ret)
		return ret;
	uncore_pci_uncores = cnx_pci_uncores;
	uncore_pci_driver = &cnx_uncore_pci_driver;
	return 0;
}

static const struct zhaoxin_uncore_init_fun cnx_uncore_init __initconst = {
	.cpu_init = cnx_uncore_cpu_init,
	.pci_init = cnx_uncore_pci_init,
};

static const struct x86_cpu_id zhaoxin_uncore_match[] __initconst = {
	X86_MATCH_VENDOR_FAM_MODEL(ZHAOXIN, 7, ZHAOXIN_FAM7_ZXD, &chx_uncore_init),
	X86_MATCH_VENDOR_FAM_MODEL(CENTAUR, 7, ZHAOXIN_FAM7_ZXD, &chx_uncore_init),
	X86_MATCH_VENDOR_FAM_MODEL(ZHAOXIN, 7, ZHAOXIN_FAM7_ZXE, &chx_uncore_init),
	X86_MATCH_VENDOR_FAM_MODEL(CENTAUR, 7, ZHAOXIN_FAM7_ZXE, &chx_uncore_init),
	X86_MATCH_VENDOR_FAM_MODEL(ZHAOXIN, 7, ZHAOXIN_FAM7_CNX, &cnx_uncore_init),
	X86_MATCH_VENDOR_FAM_MODEL(CENTAUR, 7, ZHAOXIN_FAM7_CNX, &cnx_uncore_init),
	{},
};
MODULE_DEVICE_TABLE(x86cpu, zhaoxin_uncore_match);

static int __init zhaoxin_uncore_init(void)
{
	const struct x86_cpu_id *id;
	struct zhaoxin_uncore_init_fun *uncore_init;
	int pret = 0, cret = 0, ret;

	id = x86_match_cpu(zhaoxin_uncore_match);

	if (!id)
		return -ENODEV;

	if (boot_cpu_has(X86_FEATURE_HYPERVISOR))
		return -ENODEV;

	pr_info("welcome to uncore!\n");

	max_packages = topology_max_packages();
	max_clusters = topology_clusters();
	max_subnodes = topology_subnodes();

	get_cluster_info();
	get_subnode_info();
	zx_gen_core_map();

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

	if (cret && pret)
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
	uncore_pci_exit();
	return ret;
}
module_init(zhaoxin_uncore_init);

static void __exit zhaoxin_uncore_exit(void)
{
	cpuhp_remove_state(CPUHP_AP_PERF_X86_UNCORE_ONLINE);
	uncore_types_exit(uncore_msr_uncores);
	uncore_pci_exit();
}
module_exit(zhaoxin_uncore_exit);
