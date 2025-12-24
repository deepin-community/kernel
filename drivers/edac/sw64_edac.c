// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/edac.h>
#include <linux/gfp.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/acpi.h>
#include <linux/platform_device.h>

#include <asm/sw64io.h>

#include "edac_module.h"

#define EDAC_MOD_STR	"sw64_edac"

struct sw64_edac {
	struct device		*dev;
	void __iomem *spbu_base;
	struct list_head	mc;
	int node;
	int edac_idx;
};

struct sw64_edac_mc {
	struct fwnode_handle *fwnode;
	struct list_head	next;
	char			*name;
	struct mem_ctl_info	*mci;
	struct sw64_edac	*edac;
	void __iomem *mc_vbase;
	u32			mc_id;
	int irq;
};

struct sw64_platform_data {
	struct sw64_edac_mc *properties;
	unsigned int nports;
};

static int edac_mc_idx;
static const char *sw64_ctl_name = "SW64";

/*********************** DRAM err device **********************************/

static void sw64_edac_mc_check(struct mem_ctl_info *mci)
{
	struct sw64_edac_mc *mc = mci->pvt_info;
	u32 reg;
	u32 err_addr;

	reg = readq(mc->mc_vbase + MEMSERR) >> 32;
	err_addr = readq(mc->mc_vbase + MERRADDR);

	/* first bit clear in ECC Err Reg, 1 bit error, correctable by HW */
	if (reg & 0x1) {
		edac_mc_handle_error(HW_EVENT_ERR_CORRECTED, mci, 1,
				     err_addr >> PAGE_SHIFT,
				     err_addr & PAGE_MASK, 0,
				     0, 0, -1,
				     mci->ctl_name, "");
		/* clear the error */
		writeq(0x1, mc->mc_vbase + MEMSERR);
		writeq(1UL << 32, mc->mc_vbase + MEMSERR);
	}
	if (reg & 0x2) {	/* 2 bit error, UE */
		edac_mc_handle_error(HW_EVENT_ERR_UNCORRECTED, mci, 1,
				     err_addr >> PAGE_SHIFT,
				     err_addr & PAGE_MASK, 0,
				     0, 0, -1,
				     mci->ctl_name, "");
		/* clear the error */
		writeq(1UL << 33, mc->mc_vbase + MEMSERR);
	}

}

static irqreturn_t sw64_edac_isr(int irq, void *dev_id)
{
	struct sw64_edac *edac = dev_id;
	struct sw64_edac_mc *mc;
	u32 cause;
	void __iomem *spbu_base = edac->spbu_base;

	cause = (readq(spbu_base + OFFSET_FAULT_SOURCE) << 2) & 0x1;
	if (!cause)
		return IRQ_NONE;

	/* writing 0's to the ECC err addr in check function clears irq */
	list_for_each_entry(mc, &edac->mc, next)
		sw64_edac_mc_check(mc->mci);

	return IRQ_HANDLED;
}

static unsigned long get_total_mem(struct sw64_edac *edac)
{
	unsigned long total_mem;
	void __iomem *spbu_base = edac->spbu_base;

	total_mem = readq(spbu_base + OFFSET_CFG_INFO) >> 3;
	total_mem = (total_mem & 0xffff) << 28;

	return total_mem;
}

static void sw64_init_csrows(struct mem_ctl_info *mci,
				struct sw64_edac_mc *mc)
{
	struct csrow_info *csrow;
	struct dimm_info *dimm;
	unsigned long total_mem;

	u32 devtype;

	total_mem = get_total_mem(mc->edac);

	csrow = mci->csrows[0];
	dimm = csrow->channels[0]->dimm;

	dimm->nr_pages = total_mem >> PAGE_SHIFT;
	dimm->grain = 8;

	dimm->mtype = MEM_DDR4;

	devtype = readq(mc->mc_vbase + MC_CTRL) >> 20;
	switch (devtype & 0x3) {
	case 0x0:
		dimm->dtype = DEV_X4;
		break;
	case 0x2:
		dimm->dtype = DEV_X8;
		break;
	case 0x3:
		dimm->dtype = DEV_X16;
		break;
	default:
		dimm->dtype = DEV_UNKNOWN;
		break;
	}

	dimm->edac_mode = EDAC_SECDED;
}

#ifdef CONFIG_NUMA
static void sw64_edac_get_node(struct sw64_edac *edac,
		struct device *dev)
{
	if (numa_off)
		return;

	if (acpi_disabled) {
		if (device_property_read_u32(dev, "numa-node-id", &edac->node))
			dev_warn(dev, "sw64_edac: node ID unknown\n");
	} else
		edac->node = dev_to_node(dev);

	/**
	 * If numa_off is not set, we expect a valid node ID.
	 * If not, fallback to node 0.
	 */
	if (edac->node == NUMA_NO_NODE) {
		pr_warn("Invalid node ID\n");
		edac->node = 0;
	}
}
#endif

static int sw64_edac_mc_add(struct device *dev, struct fwnode_handle *fwnode,
		struct sw64_platform_data *pdata, int i)
{
	struct sw64_edac *edac;
	struct mem_ctl_info *mci;
	struct edac_mc_layer layers[2];
	struct sw64_edac_mc tmp_mc;
	struct sw64_edac_mc *mc;
	struct resource res;
	struct resource *r;
	int ret;
	int numa_node;
	struct platform_device *child_pdev;
	acpi_status status;
	unsigned long long sta;

	edac = dev_get_drvdata(dev);
	memset(&tmp_mc, 0, sizeof(tmp_mc));

	if (!devres_open_group(edac->dev, sw64_edac_mc_add, GFP_KERNEL))
		return -ENOMEM;

	tmp_mc.fwnode = fwnode;
	tmp_mc = pdata->properties[i++];

	if (fwnode_property_read_u32(fwnode, "memory-controller",
				 &tmp_mc.mc_id)) {
		dev_err(dev, "Failed to get memory-controller ID\n");
		ret = -ENODEV;
		goto err_group;
	}

	if (acpi_disabled) {
		unsigned long mc_online;
		void __iomem *spbu_base;

		numa_node = edac->node;
		spbu_base = edac->spbu_base;
		mc_online = readq(spbu_base + OFFSET_MC_ONLINE) & 0xff;

		if (!(mc_online & (1 << tmp_mc.mc_id))) {
			pr_info("mc %d.%d is offline, skip init\n", numa_node, tmp_mc.mc_id);
			goto err_group;
		}
	} else {
		if (fwnode_property_read_u32(fwnode, "numa-node-id",
					     &numa_node)) {
			dev_info(edac->dev, "Failed to get numa node ID\n");
			ret = -ENODEV;
			goto err_group;
		}

		status = acpi_evaluate_integer(ACPI_HANDLE(dev), "_STA", NULL, &sta);
		if (ACPI_FAILURE(status))
			goto err_group;
		if (!sta) {
			pr_info("mc %d.%d is offline, skip init\n", numa_node, tmp_mc.mc_id);
			goto err_group;
		}
	}


	if (acpi_disabled) {
		u64 regs[2];

		if (fwnode_property_read_u64_array(fwnode, "reg", regs, 2)) {
			dev_err(dev, "Failed to get MC registers\n");
			fwnode_handle_put(fwnode);
			ret = -ENODEV;
		}
		res.start = regs[0];
		res.end = regs[0] + regs[1] - 1;
		res.flags = IORESOURCE_MEM;
		tmp_mc.mc_vbase = devm_ioremap_resource(edac->dev, &res);
	} else {
		child_pdev = to_platform_device(fwnode->dev);
		r = platform_get_resource(child_pdev, IORESOURCE_MEM, 0);
		if (!r) {
			dev_err(dev, "Failed to get MC registers\n");
			ret = -ENODEV;
		}
		tmp_mc.mc_vbase = devm_ioremap_resource(edac->dev, r);
	}


	if (IS_ERR(tmp_mc.mc_vbase)) {
		dev_err(dev, "unable to map MCU resource\n");
		devm_kfree(dev, &mc);
		ret = -ENODEV;
		goto err_group;
	}

	layers[0].type = EDAC_MC_LAYER_CHIP_SELECT;
	layers[0].size = 2;
	layers[0].is_virt_csrow = true;
	layers[1].type = EDAC_MC_LAYER_CHANNEL;
	layers[1].size = 1;
	layers[1].is_virt_csrow = false;
	mci = edac_mc_alloc(edac_mc_idx, ARRAY_SIZE(layers), layers,
			    sizeof(struct sw64_edac_mc));
	if (!mci) {
		ret = -ENOMEM;
		goto err_group;
	}

	mc = mci->pvt_info;
	*mc = tmp_mc;		/* Copy over resource value */
	mc->edac = edac;
	mc->name = "sw64_edac_mc_err";
	mc->mci = mci;
	mci->pdev = &mci->dev;
	mci->mtype_cap = MEM_FLAG_RDDR | MEM_FLAG_DDR;
	mci->edac_ctl_cap = EDAC_FLAG_NONE | EDAC_FLAG_SECDED;
	mci->edac_cap = EDAC_FLAG_SECDED;
	mci->mod_name = EDAC_MOD_STR;
	mci->ctl_name = sw64_ctl_name;
	mci->dev_name = mc->name;
	mci->edac_check = sw64_edac_mc_check;
	mci->ctl_page_to_phys = NULL;
	mci->scrub_mode = SCRUB_SW_SRC;
	edac->edac_idx = edac_mc_idx++;

	sw64_init_csrows(mci, mc);

	if (edac_mc_add_mc(mci)) {
		dev_err(dev, "edac_mc_add_mc failed\n");
		ret = -EINVAL;
		goto err_free;
	}

	list_add(&mc->next, &edac->mc);

	devres_remove_group(edac->dev, sw64_edac_mc_add);

	dev_info(dev, "SW64 EDAC MC registered\n");

	return 0;

err_free:
	edac_mc_free(mci);
err_group:
	devres_release_group(edac->dev, sw64_edac_mc_add);
	return ret;
}

static int sw64_edac_probe(struct platform_device *pdev)
{
	struct sw64_edac *edac;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;
	acpi_status status;
	unsigned long long sta;
	struct fwnode_handle *fwnode;
	struct sw64_platform_data *pdata;
	struct sw64_edac_mc *mc;
	int nports;
	int i;

	edac = devm_kzalloc(&pdev->dev, sizeof(*edac), GFP_KERNEL);
	if (!edac)
		return -ENOMEM;

#ifdef CONFIG_NUMA
	sw64_edac_get_node(edac, dev);
#endif

	edac->spbu_base = misc_platform_get_spbu_base(edac->node);
	if (IS_ERR(edac->spbu_base))
		return PTR_ERR(edac->spbu_base);

	edac->dev = &pdev->dev;
	platform_set_drvdata(pdev, edac);
	INIT_LIST_HEAD(&edac->mc);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENOENT;
		goto out_err;
	}

	if (!acpi_disabled) {
		status = acpi_evaluate_integer(ACPI_HANDLE(dev), "_STA", NULL, &sta);
		if (ACPI_FAILURE(status)) {
			ret = -EIO;
			goto out_err;
		}
		if (!sta) {
			ret = -EIO;
			goto out_err;
		}
	}

	if (edac_op_state == EDAC_OPSTATE_INT) {
		int irq;
		/* acquire interrupt that reports errors */
		irq = platform_get_irq(pdev, 0);
		ret = devm_request_irq(&pdev->dev,
				       irq,
				       sw64_edac_isr,
				       IRQF_SHARED,
				       "[EDAC] MC err",
				       edac);
		if (ret < 0) {
			ret = -ENODEV;
			goto out_err;
		}
	}

	i = 0;
	nports = device_get_child_node_count(dev);

	if (nports == 0) {
		ret = -ENODEV;
		goto out_err;
	}

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		ret = -ENOMEM;
		goto out_err;
	}

	pdata->properties = devm_kcalloc(dev, nports, sizeof(*mc), GFP_KERNEL);
	if (!pdata->properties) {
		ret = -ENOMEM;
		goto out_err;
	}

	pdata->nports = nports;

	i = 0;
	device_for_each_child_node(dev, fwnode) {
		sw64_edac_mc_add(dev, fwnode, pdata, i);
	}

	return 0;

out_err:
	return ret;
}

static int sw64_edac_mc_remove(struct sw64_edac_mc *mc)
{
	edac_mc_del_mc(&mc->mci->dev);
	edac_mc_free(mc->mci);
	return 0;
}

static int sw64_edac_remove(struct platform_device *pdev)
{
	struct sw64_edac *edac = dev_get_drvdata(&pdev->dev);
	struct sw64_edac_mc *mc, *tmp_mc;

	list_for_each_entry_safe(mc, tmp_mc, &edac->mc, next)
		sw64_edac_mc_remove(mc);

	return 0;
}

static const struct of_device_id sw64_edac_of_match[] = {
	{ .compatible = "sunway,edac", .data = (void *)0 },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, sw64_edac_of_match);

#ifdef CONFIG_ACPI
static const struct acpi_device_id sw64_edac_acpi_match[] = {
	{ "SUNW0201", 0 },
	{},
};
MODULE_DEVICE_TABLE(acpi, sw64_edac_acpi_match);
#endif

static struct platform_driver sw64_edac_driver = {
	.probe = sw64_edac_probe,
	.remove = sw64_edac_remove,
	.driver = {
		.name = "sw64-edac",
		.of_match_table = of_match_ptr(sw64_edac_of_match),
		.acpi_match_table = ACPI_PTR(sw64_edac_acpi_match),
	}
};

static int __init sw64_edac_init(void)
{
	/* make sure error reporting method is sane */
	switch (edac_op_state) {
	case EDAC_OPSTATE_POLL:
	case EDAC_OPSTATE_INT:
		break;
	default:
		edac_op_state = EDAC_OPSTATE_POLL;
		break;
	}

	return platform_driver_register(&sw64_edac_driver);
}
module_init(sw64_edac_init);

static void __exit sw64_edac_exit(void)
{
	platform_driver_unregister(&sw64_edac_driver);
}
module_exit(sw64_edac_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("He Chuyue <hechuyue@wxiat.com>");
MODULE_DESCRIPTION("SW64 EDAC driver");
module_param(edac_op_state, int, 0444);
MODULE_PARM_DESC(edac_op_state,
		 "EDAC Error Reporting state: 0=Poll, 2=Interrupt");
