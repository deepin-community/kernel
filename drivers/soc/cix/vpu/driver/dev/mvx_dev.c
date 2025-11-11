// SPDX-License-Identifier: GPL-2.0-only
/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/acpi.h>
#include <linux/bitops.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/workqueue.h>
#include <linux/reset.h>
#include <linux/reset-controller.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/devfreq.h>
#include <linux/devfreq-event.h>
#include <linux/scmi_protocol.h>

#include "mvx_bitops.h"
#include "mvx_dev.h"
#include "mvx_hwreg.h"
#include "mvx_if.h"
#include "mvx_scheduler.h"
#include "mvx_session.h"
#include "mvx_log_group.h"
#include "mvx_pm_runtime.h"

#include "../../../scmi/domain.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

/**
 * Name of the MVx dev device.
 */
#define MVX_DEV_NAME    "amvx_dev"

#define MVX_PCI_VENDOR 0x13b5
#define MVX_PCI_DEVICE 0x0001

#define MVE_CLK_NAME        "vpu_clk"
#define MVE_RST_NAME        "vpu_reset"
#define MVE_RCSU_RST_NAME   "vpu_rcsu_reset"

#define MVX_MAX_NUMBER_OF_PMDOMAINS 5

#define VPU_CORE_ACPI_REF_POWERSOURCE   1
#define VPU_CORE_ACPI_NAME_PREFIX       "CRE"
#define VPU_CORE_ACPI_MEMREPAIR_FUNC    "REPR"
#define to_acpi_device(d)  container_of(d, struct acpi_device, dev)

static uint busctrl_ref = MVE_BUSTCTRL_REF_DEFAULT;
static uint busctrl_split = MVE_BUSTCTRL_SPLIT_512;
module_param(busctrl_ref, uint, 0660);
module_param(busctrl_split, uint, 0660);

static bool disable_dfs;
module_param(disable_dfs, bool, 0660);

/****************************************************************************
 * Types
 ****************************************************************************/

struct mvx_freq_table {
	unsigned int cores;
	unsigned long load;
	unsigned long freq;
};

/**
 * struct mvx_dev_ctx - Private context for the MVx dev device.
 */
struct mvx_dev_ctx {
	struct device *dev;
	struct device *pmdomains[MVX_MAX_NUMBER_OF_PMDOMAINS];
	unsigned int pmdomains_cnt;
	struct clk *clk;
	struct reset_control *rstc;
	struct mvx_if_ops *if_ops;
	struct mvx_client_ops client_ops;
	struct mvx_hwreg hwreg;
	struct mvx_sched scheduler;
	unsigned int irq;
	struct workqueue_struct *work_queue;
	struct work_struct work;
	unsigned long irqve;
	struct dentry *dentry;

	struct device *opp_pmdomain;
	struct device_link *opp_dl;
	struct devfreq_dev_profile devfreq_profile;
	struct devfreq *devfreq;
	unsigned long target_freq;
};

/**
 * struct mvx_client_session - Device session.
 *
 * When the if module registers a session this structure is returned.
 */
struct mvx_client_session {
	struct mvx_dev_ctx *ctx;
	struct mvx_sched_session session;
};

/****************************************************************************
 * Static variables and functions
 ****************************************************************************/

const char *const vpu_pmdomains[MVX_MAX_NUMBER_OF_PMDOMAINS] = {
	"vpu_top", "vpu_core0", "vpu_core1", "vpu_core2", "vpu_core3"
};

static const struct mvx_freq_table sky1_mvx_freq_table[] = {
	{ 4, 486000, 150000000 },	// 1080P@60
	{ 4, 972000, 300000000 },	// 1080P@120
	{ 4, 1458000, 480000000 },	// 1080P@180
	{ 4, 2073600, 600000000 },	// 4K@60
	{ 4, 4147200, 800000000 },	// 4K@120
	{ 4, 8294400, 1200000000 },	// 8K@60
};

static struct mvx_dev_ctx *client_ops_to_ctx(struct mvx_client_ops *client)
{
	return container_of(client, struct mvx_dev_ctx, client_ops);
}

static void get_hw_ver(struct mvx_client_ops *client, struct mvx_hw_ver *hw_ver)
{
	struct mvx_dev_ctx *ctx = client_ops_to_ctx(client);

	if (IS_ERR_OR_NULL(hw_ver)) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "hw_ver pointer is invalid.");
		return;
	}
	mvx_hwreg_get_hw_ver(&ctx->hwreg, hw_ver);
}

static void get_formats(struct mvx_client_ops *client,
			enum mvx_direction direction, uint64_t *formats)
{
	struct mvx_dev_ctx *ctx = client_ops_to_ctx(client);
	uint32_t fuses;
	*formats = 0;

	ctx->hwreg.ops.get_formats(direction, formats);

	/* Remove formats based on fuses. */
	fuses = mvx_hwreg_get_fuse(&ctx->hwreg);

	if (fuses & MVX_HWREG_FUSE_DISABLE_AFBC) {
		mvx_clear_bit(MVX_FORMAT_YUV420_AFBC_8, formats);
		mvx_clear_bit(MVX_FORMAT_YUV420_AFBC_10, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_AFBC_8, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_AFBC_10, formats);
		mvx_clear_bit(MVX_FORMAT_Y_AFBC_8, formats);
		mvx_clear_bit(MVX_FORMAT_Y_AFBC_10, formats);
	}

	if (fuses & MVX_HWREG_FUSE_DISABLE_REAL)
		mvx_clear_bit(MVX_FORMAT_RV, formats);

	if (fuses & MVX_HWREG_FUSE_DISABLE_VPX) {
		mvx_clear_bit(MVX_FORMAT_VP8, formats);
		mvx_clear_bit(MVX_FORMAT_VP9, formats);
	}

	if (fuses & MVX_HWREG_FUSE_DISABLE_HEVC)
		mvx_clear_bit(MVX_FORMAT_HEVC, formats);
}

static unsigned int get_core_mask(struct mvx_client_ops *client)
{
	struct mvx_dev_ctx *ctx = client_ops_to_ctx(client);

	return mvx_hwreg_get_core_mask(&ctx->hwreg);
}

static int update_freq(struct mvx_dev_ctx *ctx)
{
	int active_ncores;
	unsigned long mbs_per_sec;
	struct mvx_freq_table highest =
	    sky1_mvx_freq_table[ARRAY_SIZE(sky1_mvx_freq_table) - 1];
	int freq = 0;
	int ret;
	int i, j;

	ret = mvx_sched_calculate_load(&ctx->scheduler, &mbs_per_sec);
	if (ret != 0)
		return ret;

	if (mbs_per_sec > highest.load) {
		freq = highest.freq;
	} else {
		active_ncores = mvx_hwreg_get_ncores(&ctx->hwreg);
		for (i = 0; i < ARRAY_SIZE(sky1_mvx_freq_table); i++) {
			if (sky1_mvx_freq_table[i].load >= mbs_per_sec) {
				freq =
				    sky1_mvx_freq_table[i].freq *
				    sky1_mvx_freq_table[i].cores /
				    active_ncores;
				if (active_ncores ==
				    sky1_mvx_freq_table[i].cores)
					break;
				if (freq > highest.freq) {
					freq = highest.freq;
					break;
				}
				for (j = 0; j < ARRAY_SIZE(sky1_mvx_freq_table);
				     j++) {
					if (freq <= sky1_mvx_freq_table[j].freq) {
						freq =
						    sky1_mvx_freq_table[j].freq;
						break;
					}
				}
				break;
			}
		}
	}

	ctx->target_freq = freq;

	return ret;
}

static int update_load(struct mvx_client_session *csession)
{
	struct mvx_dev_ctx *ctx = csession->ctx;
	int ret;

	if (disable_dfs)
		return 0;

	ret = update_freq(ctx);
	if (ret != 0)
		return ret;

	if (!mutex_trylock(&ctx->devfreq->lock))
		return ret;

	ret = update_devfreq(ctx->devfreq);
	mutex_unlock(&ctx->devfreq->lock);

	return ret;
}

static struct mvx_client_session *register_session(struct mvx_client_ops
						   *client, struct mvx_if_session
						   *isession)
{
	struct mvx_dev_ctx *ctx = client_ops_to_ctx(client);
	struct mvx_client_session *csession;
	int ret;

	csession = devm_kzalloc(ctx->dev, sizeof(*csession), GFP_KERNEL);
	if (csession == NULL)
		return ERR_PTR(-ENOMEM);

	csession->ctx = ctx;

	ret = mvx_pm_runtime_get_sync(ctx->dev);
	if (ret < 0)
		goto free_session;

	ret = mvx_sched_session_construct(&csession->session, isession);
	if (ret != 0)
		goto runtime_put;

	ret =
	    mvx_sched_add_session(&ctx->scheduler, &csession->session.session);
	if (ret != 0)
		goto destruct_session;
	update_load(csession);

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
		      "Register client session. csession=0x%p, isession=0x%p.",
		      csession, isession);

	return csession;

destruct_session:
	mvx_sched_session_destruct(&csession->session);
runtime_put:
	mvx_pm_runtime_put_sync(csession->ctx->dev);
free_session:
	devm_kfree(ctx->dev, csession);

	return ERR_PTR(ret);
}

static void unregister_session(struct mvx_client_session *csession)
{
	mvx_sched_remove_session(&csession->ctx->scheduler,
				 &csession->session.session);
	mvx_sched_terminate(&csession->ctx->scheduler, &csession->session);
	mvx_sched_session_destruct(&csession->session);
	update_load(csession);

	mvx_pm_runtime_put_sync(csession->ctx->dev);

	devm_kfree(csession->ctx->dev, csession);
}

static int switch_in(struct mvx_client_session *csession)
{
	struct mvx_dev_ctx *ctx = csession->ctx;
	int ret;

	ret = mvx_sched_switch_in(&ctx->scheduler, &csession->session);

	return ret;
}

static int switch_out_rsp(struct mvx_client_session *csession)
{
	return mvx_sched_switch_out_rsp(&csession->ctx->scheduler,
					&csession->session);
}

static void terminate(struct mvx_client_session *csession)
{
	return mvx_sched_terminate(&csession->ctx->scheduler,
				   &csession->session);
}

static void reset_priority(struct mvx_client_session *csession)
{
	return mvx_sched_reset_priority(&csession->ctx->scheduler,
					&csession->session);
}

static int send_irq(struct mvx_client_session *csession)
{
	struct mvx_dev_ctx *ctx = csession->ctx;
	int ret;

	ret = mvx_sched_send_irq(&ctx->scheduler, &csession->session);

	return ret;
}

int soft_irq(struct mvx_client_session *csession)
{
	struct mvx_dev_ctx *ctx = csession->ctx;
	int ret;

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
		      "%p soft trigger irq. csession=0x%px.",
		      mvx_if_session_to_session(csession->session.isession),
		      csession);

	ret = mvx_sched_trigger_irq(&ctx->scheduler, &csession->session);

	return ret;
}

static int flush_mmu(struct mvx_client_session *csession)
{
	struct mvx_dev_ctx *ctx = csession->ctx;
	int ret;

	ret = mvx_sched_flush_mmu(&ctx->scheduler, &csession->session);

	return ret;
}

static void print_debug(struct mvx_client_session *csession)
{
	struct mvx_dev_ctx *ctx = csession->ctx;

	mvx_sched_print_debug(&ctx->scheduler, &csession->session);
}

static struct mvx_dev_ctx *work_to_ctx(struct work_struct *work)
{
	return container_of(work, struct mvx_dev_ctx, work);
}

/**
 * irq_bottom() - Handle IRQ bottom.
 * @work:    Work struct that is part of the context structure.
 *
 * This function is called from a work queue and id doing the actual work of
 * handling the interrupt.
 */
static void irq_bottom(struct work_struct *work)
{
	struct mvx_dev_ctx *ctx = work_to_ctx(work);
	uint32_t nlsid;
	uint32_t i;

	nlsid = mvx_hwreg_get_nlsid(&ctx->hwreg);
	for (i = 0; i < nlsid; i++)
		if (test_and_clear_bit(i, &ctx->irqve))
			mvx_sched_handle_irq(&ctx->scheduler, i);
}

/**
 * irq_top() - Handle IRQ top.
 * @irq:    IRQ number.
 * @dev_id:    Pointer to context.
 *
 * This function is called in interrupt context. It should be short and must not
 * block.
 *
 * Return: IRQ status if the IRQ was handled or not.
 */
static irqreturn_t irq_top(int irq, void *dev_id)
{
	struct mvx_dev_ctx *ctx = dev_id;
	uint32_t nlsid;
	uint32_t irqve;
	int ret = IRQ_NONE;

	nlsid = mvx_hwreg_get_nlsid(&ctx->hwreg);
	irqve = mvx_hwreg_read(&ctx->hwreg, MVX_HWREG_IRQVE);
	while (nlsid-- > 0)
		if ((irqve >> nlsid) & 0x1) {
			mvx_hwreg_write_lsid(&ctx->hwreg,
					     nlsid, MVX_HWREG_LIRQVE, 0);
			mb();
			set_bit(nlsid, &ctx->irqve);
			ret = IRQ_HANDLED;
		}

	queue_work(ctx->work_queue, &ctx->work);

	return ret;
}

static int mvx_devfreq_target(struct device *dev, unsigned long *freq,
			      u32 flags)
{
	struct mvx_dev_ctx *ctx = dev_get_drvdata(dev);
	struct dev_pm_opp *opp;
	unsigned long pre_freq;
	unsigned long target_freq = *freq;
	int ret;

	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp)) {
		dev_err(dev, "Failed to get recommended opp instance\n");
		ret = PTR_ERR(opp);
		return ret;
	}
	dev_pm_opp_put(opp);
	pre_freq = scmi_device_get_freq(ctx->opp_pmdomain);
	ret = scmi_device_set_freq(ctx->opp_pmdomain, *freq);
	atomic_set(&mvx_log_perf.freq, *freq);

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_DEBUG,
		      "%s() target=%ld, previous=%ld, current=%ld.", __func__,
		      target_freq, pre_freq, *freq);

	return ret;
}

static int mvx_devfreq_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct mvx_dev_ctx *ctx = dev_get_drvdata(dev);

	*freq = scmi_device_get_freq(ctx->opp_pmdomain);
	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_DEBUG, "%s() %ld", __func__, *freq);

	return 0;
}

static int mvx_devfreq_get_dev_status(struct device *dev,
				      struct devfreq_dev_status *stat)
{
	struct mvx_dev_ctx *ctx = dev_get_drvdata(dev);

	update_freq(ctx);
	stat->current_frequency = scmi_device_get_freq(ctx->opp_pmdomain);
	stat->busy_time = ctx->target_freq;
	stat->total_time = stat->current_frequency;

	return 0;
}

static int mvx_devfreq_init(struct mvx_dev_ctx *ctx)
{
	struct dev_pm_opp *opp;
	struct devfreq_dev_profile *profile;
	unsigned long freq;
	int opp_count;
	struct devfreq_simple_ondemand_data *ondemand_data;
	int i;
	int ret;

	if (disable_dfs)
		return 0;

	if (!ctx)
		return -EINVAL;

	ondemand_data =
	    devm_kzalloc(ctx->dev, sizeof(*ondemand_data), GFP_KERNEL);
	if (!ondemand_data)
		return -ENOMEM;

	profile = &ctx->devfreq_profile;

	if (has_acpi_companion(ctx->dev))
		ctx->opp_pmdomain =
		    fwnode_dev_pm_domain_attach_by_name(ctx->dev, "perf");
	else
		ctx->opp_pmdomain =
		    dev_pm_domain_attach_by_name(ctx->dev, "perf");

	if (IS_ERR_OR_NULL(ctx->opp_pmdomain)) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to get perf domain");
		return -EFAULT;
	}
	ctx->opp_dl = device_link_add(ctx->dev, ctx->opp_pmdomain,
				      DL_FLAG_RPM_ACTIVE |
				      DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
	if (IS_ERR_OR_NULL(ctx->opp_dl)) {
		ret = -ENODEV;
		goto detach_opp;
	}

	/* Add opps to opp power domain. */
	ret = scmi_device_opp_table_parse(ctx->opp_pmdomain);
	if (ret) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to add opps to the device");
		ret = -ENODEV;
		goto unlink_opp;
	}
	opp_count = dev_pm_opp_get_opp_count(ctx->opp_pmdomain);
	if (opp_count <= 0) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to get opps count.");
		ret = -EINVAL;
		goto unlink_opp;
	}
	profile->freq_table =
	    kmalloc_array(opp_count, sizeof(unsigned long), GFP_KERNEL);
	for (i = 0, freq = 0; i < opp_count; i++, freq++) {
		opp = dev_pm_opp_find_freq_ceil(ctx->opp_pmdomain, &freq);
		if (IS_ERR(opp))
			break;
		dev_pm_opp_put(opp);
		profile->freq_table[i] = freq;

		/* Add opps to ctx->dev, since register devfreq device as ctx->dev */
		ret = dev_pm_opp_add(ctx->dev, freq, 0);
		if (ret) {
			MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
				      "Failed to add opp %lu Hz", freq);
			while (i-- > 0) {
				dev_pm_opp_remove(ctx->dev,
						  profile->freq_table[i]);
			}
			ret = -ENODEV;
			goto free_table;
		}
	}

	profile->max_state = i;
	profile->polling_ms = 100;
	profile->target = mvx_devfreq_target;
	profile->get_dev_status = mvx_devfreq_get_dev_status;
	profile->get_cur_freq = mvx_devfreq_get_cur_freq;
	ondemand_data->downdifferential = 1;
	ondemand_data->upthreshold = 100;
	ctx->devfreq =
	    devm_devfreq_add_device(ctx->dev, profile,
				    DEVFREQ_GOV_SIMPLE_ONDEMAND, ondemand_data);
	if (IS_ERR(ctx->devfreq)) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to add devfreq device");
		ret = PTR_ERR(ctx->devfreq);
		goto remove_table;
	}

	ret = devm_devfreq_register_opp_notifier(ctx->dev, ctx->devfreq);
	if (ret < 0) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to register opp notifier");
		goto remove_device;
	}

	return ret;

remove_device:
	devm_devfreq_remove_device(ctx->dev, ctx->devfreq);
	ctx->devfreq = NULL;
remove_table:
	dev_pm_opp_remove_table(ctx->dev);
	profile->max_state = 0;
free_table:
	kfree(profile->freq_table);
	profile->freq_table = NULL;
unlink_opp:
	device_link_del(ctx->opp_dl);
	ctx->opp_dl = NULL;
detach_opp:
	dev_pm_domain_detach(ctx->opp_pmdomain, true);
	devm_kfree(ctx->dev, ondemand_data);

	return ret;
}

static int mvx_devfreq_remove(struct mvx_dev_ctx *ctx)
{

	if (disable_dfs)
		return 0;

	if (ctx->devfreq) {
		devm_devfreq_unregister_opp_notifier(ctx->dev, ctx->devfreq);
		devm_kfree(ctx->dev, ctx->devfreq->data);
		devm_devfreq_remove_device(ctx->dev, ctx->devfreq);
		ctx->devfreq = NULL;
	}
	if (ctx->devfreq_profile.max_state > 0) {
		dev_pm_opp_remove_table(ctx->dev);
		ctx->devfreq_profile.max_state = 0;
	}
	if (ctx->devfreq_profile.freq_table) {
		kfree(ctx->devfreq_profile.freq_table);
		ctx->devfreq_profile.freq_table = NULL;
	}
	if (ctx->opp_dl) {
		device_link_del(ctx->opp_dl);
		ctx->opp_dl = NULL;
	}
	dev_pm_domain_detach(ctx->opp_pmdomain, true);

	return 0;
}

#ifdef CONFIG_ACPI
#if VPU_CORE_ACPI_REF_POWERSOURCE
static void acpi_dev_pm_detach(struct device *dev, bool power_off)
{
	dev_pm_domain_set(dev, NULL);
}

static struct dev_pm_domain acpi_vpu_pm_domain = {
	.ops = {
		SET_RUNTIME_PM_OPS(acpi_subsys_runtime_suspend,
				   acpi_subsys_runtime_resume, NULL)
#ifdef CONFIG_PM_SLEEP
		SET_SYSTEM_SLEEP_PM_OPS(acpi_subsys_runtime_suspend,
					acpi_subsys_runtime_resume)
#endif
		 },
	.detach = acpi_dev_pm_detach
};
#endif
#endif

static int mvx_dev_probe(struct device *dev,
			 struct resource *rcsu_res,
			 struct resource *res, int irq)
{
	struct mvx_dev_ctx *ctx;
	int ret;
	int i;
	struct device *pd;
#ifdef CONFIG_ACPI
	struct fwnode_handle *child;
#endif

	/* Create device context and store pointer in device private data. */
	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (ctx == NULL)
		return -EINVAL;

	ctx->dev = dev;
	dev_set_drvdata(dev, ctx);

	/* Setup client ops callbacks. */
	ctx->client_ops.get_hw_ver = get_hw_ver;
	ctx->client_ops.get_formats = get_formats;
	ctx->client_ops.get_core_mask = get_core_mask;
	ctx->client_ops.register_session = register_session;
	ctx->client_ops.unregister_session = unregister_session;
	ctx->client_ops.switch_in = switch_in;
	ctx->client_ops.switch_out_rsp = switch_out_rsp;
	ctx->client_ops.send_irq = send_irq;
	ctx->client_ops.soft_irq = soft_irq;
	ctx->client_ops.flush_mmu = flush_mmu;
	ctx->client_ops.print_debug = print_debug;
	ctx->client_ops.update_load = update_load;
	ctx->client_ops.terminate = terminate;
	ctx->client_ops.reset_priority = reset_priority;

	/* Create if context. */
	ctx->if_ops = mvx_if_create(dev, &ctx->client_ops, ctx);
	if (IS_ERR(ctx->if_ops)) {
		ret = -EINVAL;
		goto free_ctx;
	}

	/* Create debugfs entry */
	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		char name[20];

		scnprintf(name, sizeof(name), "%s%u", MVX_DEV_NAME, dev->id);
		ctx->dentry = debugfs_create_dir(name, NULL);
		if (IS_ERR_OR_NULL(ctx->dentry)) {
			ret = -EINVAL;
			goto destroy_if;
		}
	}

	/* Construct hw register context. */
	ret = mvx_hwreg_construct(&ctx->hwreg, dev, rcsu_res, res, ctx->dentry);
	if (ret != 0)
		goto destruct_dentry;

	ctx->clk = devm_clk_get_optional(dev, MVE_CLK_NAME);
	if (IS_ERR_OR_NULL(ctx->clk)) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to get clock.");
		ret = -EFAULT;
		goto destruct_hwreg;
	}
	ctx->rstc = devm_reset_control_get(dev, MVE_RST_NAME);
	if (IS_ERR(ctx->rstc)) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to get reset_control, %s.", MVE_RST_NAME);
		ret = -EFAULT;
		goto destruct_hwreg;
	}

	/* Request IRQ handler. */
	ctx->irq = irq;
	irq_set_status_flags(ctx->irq, IRQ_DISABLE_UNLAZY);
	ret = request_irq(ctx->irq, irq_top, IRQF_SHARED, dev_name(dev), ctx);
	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to request IRQ. irq=%u, ret=%d.",
			      ctx->irq, ret);
		goto destruct_hwreg;
	}
	disable_irq(ctx->irq);

	if (has_acpi_companion(dev)) {
#ifdef	CONFIG_ACPI
		ctx->pmdomains[0] = dev;
		i = 1;
		fwnode_for_each_child_node(dev->fwnode, child) {
			if (is_acpi_data_node(child))
				continue;
			if (!strncmp
			    (acpi_device_bid(to_acpi_device_node(child)),
			     VPU_CORE_ACPI_NAME_PREFIX,
			     ACPI_NAMESEG_SIZE - 1)) {
				if (i >= MVX_MAX_NUMBER_OF_PMDOMAINS) {
					MVX_LOG_PRINT(&mvx_log_dev,
						      MVX_LOG_ERROR,
						      "pmDomains more than limits, Num:limits=[%d:%d].",
						      i + 1,
						      MVX_MAX_NUMBER_OF_PMDOMAINS);
					ret = -EFAULT;
					goto irq_free;
				}
				ACPI_COMPANION_SET(&
						   (to_acpi_device_node
						    (child)->dev),
						   to_acpi_device_node(child));
				ctx->pmdomains[i] =
				    &(to_acpi_device_node(child)->dev);
				to_acpi_device_node(child)->power.flags.ignore_parent = 1;
#if VPU_CORE_ACPI_REF_POWERSOURCE
				pm_runtime_enable(ctx->pmdomains[i]);
				dev_pm_domain_set(ctx->pmdomains[i],
						  &acpi_vpu_pm_domain);
#endif
				i++;
			}
		}
		ctx->pmdomains_cnt = i;
#endif
	} else {
		ctx->pmdomains_cnt =
		    of_count_phandle_with_args(dev->of_node, "power-domains",
					       "#power-domain-cells");
		/*Ignore the latest opp_pmdomain which handled by devfreq */
		if (ctx->pmdomains_cnt > 1)
			ctx->pmdomains_cnt -= 1;
		if (ctx->pmdomains_cnt < 0
		    || ctx->pmdomains_cnt > MVX_MAX_NUMBER_OF_PMDOMAINS) {
			MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
				      "Failed to get pmdomains count %d",
				      ctx->pmdomains_cnt);
			ret = -EFAULT;
			goto irq_free;
		}
		for (i = 0; i < ctx->pmdomains_cnt; i++) {
			pd = dev_pm_domain_attach_by_name(dev,
							  vpu_pmdomains[i]);
			if (IS_ERR_OR_NULL(pd)) {
				ret = -EFAULT;
				goto irq_free;
			}
			ctx->pmdomains[i] = pd;
		}
	}
	pm_runtime_set_autosuspend_delay(ctx->pmdomains[0], 1000);
	pm_runtime_use_autosuspend(ctx->pmdomains[0]);

	pm_runtime_enable(dev);
	ret = pm_runtime_resume_and_get(dev);
	if (ret) {
		pm_runtime_set_suspended(dev);
		goto runtime_disable;
	}

	ret = mvx_sched_construct(&ctx->scheduler, dev, ctx->if_ops,
				  &ctx->hwreg, ctx->dentry);
	if (ret != 0)
		goto runtime_put;

	/* Create work queue for IRQ handler. */
	ctx->work_queue = alloc_workqueue(dev_name(dev), WQ_UNBOUND, 1);
	if (ctx->work_queue == NULL) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to create work queue.");
		ret = -EINVAL;
		goto destruct_sched;
	}

	INIT_WORK(&ctx->work, irq_bottom);

	ret = mvx_devfreq_init(ctx);
	if (ret)
		goto workqueue_destroy;

	if (mvx_hwreg_get_core_mask(&ctx->hwreg) <= 0) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "No vpu cores available");
		ret = -ENODEV;
		goto devfreq_remove;
	}

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
		      "Linlon v%x identified. cores=%u, nlsid=%u, id=%u.",
		      mvx_hwreg_get_hw_id(&ctx->hwreg),
		      mvx_hwreg_get_ncores(&ctx->hwreg),
		      mvx_hwreg_get_nlsid(&ctx->hwreg), dev->id);

	mvx_pm_runtime_put_sync(ctx->dev);
	return 0;

devfreq_remove:
	mvx_devfreq_remove(ctx);

workqueue_destroy:
	destroy_workqueue(ctx->work_queue);

destruct_sched:
	mvx_sched_destruct(&ctx->scheduler);

runtime_put:
	pm_runtime_put_sync(dev);

runtime_disable:
	if (has_acpi_companion(dev)) {
#if VPU_CORE_ACPI_REF_POWERSOURCE
		for (i = 1; i < ctx->pmdomains_cnt; i++)
			pm_runtime_disable(ctx->pmdomains[i]);
#endif
	}
	pm_runtime_disable(dev);

	for (i = 0; i < ctx->pmdomains_cnt; i++)
		dev_pm_domain_detach(ctx->pmdomains[i], true);

irq_free:
	free_irq(ctx->irq, ctx);

destruct_hwreg:
	mvx_hwreg_destruct(&ctx->hwreg);

destruct_dentry:
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		debugfs_remove_recursive(ctx->dentry);

destroy_if:
	mvx_if_destroy(ctx->if_ops);

free_ctx:
	devm_kfree(dev, ctx);
	return ret;
}

static int mvx_dev_remove(struct mvx_dev_ctx *ctx)
{
	int i;

	free_irq(ctx->irq, ctx);
	mvx_devfreq_remove(ctx);
	if (has_acpi_companion(ctx->dev)) {
#if VPU_CORE_ACPI_REF_POWERSOURCE
		for (i = 1; i < ctx->pmdomains_cnt; i++)
			pm_runtime_disable(ctx->pmdomains[i]);
#endif
	}
	pm_runtime_disable(ctx->dev);
	for (i = 0; i < ctx->pmdomains_cnt; i++)
		dev_pm_domain_detach(ctx->pmdomains[i], true);

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "remove");
	mvx_if_destroy(ctx->if_ops);
	destroy_workqueue(ctx->work_queue);
	mvx_sched_destruct(&ctx->scheduler);
	mvx_hwreg_destruct(&ctx->hwreg);
	dev_set_drvdata(ctx->dev, NULL);
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		debugfs_remove_recursive(ctx->dentry);
	devm_kfree(ctx->dev, ctx);
	return 0;
}

/****************************************************************************
 * Platform driver
 ****************************************************************************/

static int mvx_pdev_probe(struct platform_device *pdev)
{
	struct resource *rcsu_res = NULL;
	struct resource *res = NULL;
	int irq = 0;

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "probe");
	rcsu_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (IS_ERR_OR_NULL(rcsu_res) || IS_ERR_OR_NULL(res)) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to get address of resource.");
		return -ENXIO;
	}
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to get IRQ resource.");
		return -ENXIO;
	}

	return mvx_dev_probe(&pdev->dev, rcsu_res, res, irq);
}

static int mvx_pdev_remove(struct platform_device *pdev)
{
	struct mvx_dev_ctx *ctx = platform_get_drvdata(pdev);

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "pdev remove");

	return mvx_dev_remove(ctx);
}

static int mvx_hw_init(struct device *dev)
{
	struct mvx_dev_ctx *ctx = dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(ctx->hwreg.dev))
		return 0;

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "hardware init");
	mvx_hwreg_write(&ctx->hwreg, MVX_HWREG_BUSCTRL,
			busctrl_ref << MVE_BUSTCTRL_REF_SHIFT | busctrl_split);
	return 0;
}

static int mvx_switch_enpwoff(struct mvx_dev_ctx *ctx, bool enable)
{
	uint32_t reg;
	uint32_t val;
	uint32_t core_mask;

	core_mask = mvx_hwreg_get_core_mask(&ctx->hwreg);
	if (enable)
		val = MVX_RCSU_HWREG_ENPWOFF_MASK;
	else
		val = ~core_mask & MVX_RCSU_HWREG_ENPWOFF_MASK;
	reg = mvx_hwreg_read_rcsu(&ctx->hwreg, MVX_RCSU_HWREG_STRAP_PIN0);
	reg =
	    (reg & ((1 << MVX_RCSU_HWREG_ENPWOFF_SHIFT) - 1)) | (val <<
								 MVX_RCSU_HWREG_ENPWOFF_SHIFT);
	mvx_hwreg_write_rcsu(&ctx->hwreg, MVX_RCSU_HWREG_STRAP_PIN0, reg);

	/* MVE soft reset. */
	mvx_hwreg_write(&ctx->hwreg, MVX_HWREG_RESET, 1);
	/* Clear CLKFORCE, then vpu can automatically power off core if ENPWOFF is enable. */
	if (mvx_log_perf.enabled & MVX_LOG_PERF_UTILIZATION) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
			      "Force enable core scheduler clock for performance profiling.");
		mvx_hwreg_write(&ctx->hwreg, MVX_HWREG_CLKFORCE,
				1 << MVE_CLKFORCE_SCHED_CLK_SHIFT);
		if (!enable && mvx_log_perf.drain && mvx_log_perf.drain->reset)
			mvx_log_perf.drain->reset(mvx_log_perf.drain);
	} else {
		mvx_hwreg_write(&ctx->hwreg, MVX_HWREG_CLKFORCE, 0);
	}

	return 0;
}

static int mvx_switch_qchannel_clock_gating(struct mvx_dev_ctx *ctx,
					    bool enable)
{
	uint32_t reg;

	reg = mvx_hwreg_read_rcsu(&ctx->hwreg, MVX_RCSU_HWREG_PGCTRL);
	reg = enable ? (reg | MVX_RCSU_HWREG_CLOCK_QCHANNEL_ENABLE) :
	    (reg & ~(MVX_RCSU_HWREG_CLOCK_QCHANNEL_ENABLE));
	mvx_hwreg_write_rcsu(&ctx->hwreg, MVX_RCSU_HWREG_PGCTRL, reg);

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "%s enable=%d",
		      __func__, enable);

	return 0;
}

#ifdef CONFIG_PM
static int mvx_pm_runtime_suspend(struct device *dev)
{
	int ret = 0;
	struct mvx_dev_ctx *ctx = dev_get_drvdata(dev);
	int i;
	uint64_t mask = mvx_hwreg_get_core_mask(&ctx->hwreg) << 1;

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "mvx_pm_runtime_suspend");

	mvx_if_flush_work(ctx->if_ops);
	ret = mvx_sched_suspend(&ctx->scheduler);
	disable_irq(ctx->irq);

    /**
     * There could be called by unregister_session() from irq_bottom() -- ctx->work,
     * hence, do not use cancel_work_sync() to avoid deadlock, and cancle_work()
     * is safe in this case.
     */
	if (current->flags & PF_WQ_WORKER)
		cancel_work(&ctx->work);
	else
		cancel_work_sync(&ctx->work);
	mvx_sched_cancel_work(&ctx->scheduler);

	mvx_switch_qchannel_clock_gating(ctx, false);

	if (ctx->hwreg.hw_ver.svn_revision == MVE_SVN_ENPWOFF) {
		/* Ensure enpwoff take effect, hw reset is needed. */
		reset_control_assert(ctx->rstc);
		usleep_range(10, 20);
		reset_control_deassert(ctx->rstc);
		mvx_switch_enpwoff(ctx, true);
	}

	if (!IS_ERR_OR_NULL(ctx->clk))
		clk_disable_unprepare(ctx->clk);

	for (i = 1; i < ctx->pmdomains_cnt; i++) {
		if (mvx_test_bit(i, &mask)) {
			if (has_acpi_companion(ctx->pmdomains[i])) {
#if VPU_CORE_ACPI_REF_POWERSOURCE
				pm_runtime_put_sync(ctx->pmdomains[i]);
#endif
			} else {
				pm_runtime_put_sync(ctx->pmdomains[i]);
			}
		}
	}

	if (!has_acpi_companion(ctx->pmdomains[0]))
		pm_runtime_put_autosuspend(ctx->pmdomains[0]);

	return ret;
}

static int mvx_pm_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct mvx_dev_ctx *ctx = dev_get_drvdata(dev);
	int i;
	uint64_t mask;

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "mvx_pm_runtime_resume");
	if (!has_acpi_companion(dev))
		pm_runtime_get_sync(ctx->pmdomains[0]);

	ret = clk_prepare_enable(ctx->clk);
	if (ret) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to enable clock, %d.", ret);
		return ret;
	}

	enable_irq(ctx->irq);

	reset_control_assert(ctx->rstc);
	usleep_range(10, 20);
	reset_control_deassert(ctx->rstc);

	mvx_switch_qchannel_clock_gating(ctx, true);

	/*Initialize hwreg when vpu_top power on for the first time. */
	if (ctx->hwreg.hw_ver.revision == 0) {
		ret = mvx_hwreg_init(&ctx->hwreg);
		if (ret)
			return ret;
	}

	mask = mvx_hwreg_get_core_mask(&ctx->hwreg) << 1;
	for (i = 1; i < ctx->pmdomains_cnt; i++) {
		if (mvx_test_bit(i, &mask)) {
			if (has_acpi_companion(ctx->pmdomains[i])) {
#if VPU_CORE_ACPI_REF_POWERSOURCE
				pm_runtime_get_sync(ctx->pmdomains[i]);
#else
				acpi_evaluate_object(to_acpi_device
						     (ctx->pmdomains[i])->handle,
						     VPU_CORE_ACPI_MEMREPAIR_FUNC,
						     NULL, NULL);
#endif
			} else {
				pm_runtime_get_sync(ctx->pmdomains[i]);
			}
		}
	}

	if (ctx->hwreg.hw_ver.svn_revision == MVE_SVN_ENPWOFF) {
		/*
		 * Memory repair is needed after each power on.
		 * VPU core will be power off when assert hwreset, so, memory repair must be
		 * after hwreset, in other words, memory repair is also needed after do
		 * hwreset.
		 * PM do memory repair when core power on.
		 */
		mvx_switch_enpwoff(ctx, false);
	}

	ret = mvx_hw_init(dev);
	if (ret)
		return ret;

	if (IS_ERR_OR_NULL(ctx->scheduler.dev))
		return ret;

	queue_work(ctx->work_queue, &ctx->work);
	ret = mvx_sched_resume(&ctx->scheduler);

	return ret;
}

static int mvx_pm_runtime_idle(struct device *dev)
{
	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "mvx_pm_runtime_idle");
	return 0;
}

static const struct dev_pm_ops mvx_dev_pm_ops = {
	SET_RUNTIME_PM_OPS(mvx_pm_runtime_suspend, mvx_pm_runtime_resume,
			   mvx_pm_runtime_idle)
	    SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				    pm_runtime_force_resume)
};
#endif /* CONFIG_PM */

static const struct of_device_id mvx_dev_match_table[] = {
	{.compatible = "arm,mali-mve" },
	{.compatible = "arm,mali-v500" },
	{.compatible = "arm,mali-v550" },
	{.compatible = "arm,mali-v61" },
	{.compatible = "armChina,linlon-v5" },
	{.compatible = "armChina,linlon-v6" },
	{.compatible = "armChina,linlon-v7" },
	{.compatible = "armChina,linlon-v8" },
	{ { 0} }
};

MODULE_DEVICE_TABLE(of, mvx_dev_match_table);

static const struct acpi_device_id mvx_dev_acpi_match_table[] = {
	{ "CIXH3010", 0 },
	{ }
};

MODULE_DEVICE_TABLE(acpi, mvx_dev_acpi_match_table);

static struct platform_driver mvx_dev_driver = {
	.probe = mvx_pdev_probe,
	.remove = mvx_pdev_remove,
	.driver = {
		   .name = MVX_DEV_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = mvx_dev_match_table,
		   .acpi_match_table = mvx_dev_acpi_match_table,
#ifdef CONFIG_PM
		   .pm = &mvx_dev_pm_ops
#endif /* CONFIG_PM */
		}
};

/****************************************************************************
 * PCI driver
 ****************************************************************************/

/* LCOV_EXCL_START */
static int mvx_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	static unsigned int dev_id;

	pdev->dev.id = dev_id++;
	return mvx_dev_probe(&pdev->dev, NULL, &pdev->resource[1], pdev->irq);
}

static void mvx_pci_remove(struct pci_dev *pdev)
{
	struct mvx_dev_ctx *ctx = pci_get_drvdata(pdev);

	mvx_dev_remove(ctx);
}

static struct pci_device_id mvx_pci_device_id[] = {
	{ PCI_DEVICE(MVX_PCI_VENDOR,
		     MVX_PCI_DEVICE) },
	{ 0, }
};

MODULE_DEVICE_TABLE(pci, mvx_pci_device_id);

static struct pci_driver mvx_pci_driver = {
	.name = MVX_DEV_NAME,
	.id_table = mvx_pci_device_id,
	.probe = mvx_pci_probe,
	.remove = mvx_pci_remove
};

/* LCOV_EXCL_STOP */

/****************************************************************************
 * Exported variables and functions
 ****************************************************************************/

int mvx_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&mvx_dev_driver);
	if (ret != 0) {
		pr_err("mvx_dev: Failed to register driver.\n");
		return ret;
	}

	/* LCOV_EXCL_START */
	ret = pci_register_driver(&mvx_pci_driver);
	if (ret != 0) {
		pr_err("mvx_dev: Failed to register PCI driver.\n");
		goto unregister_driver;
	}

	/* LCOV_EXCL_STOP */

	return 0;

unregister_driver:
	platform_driver_unregister(&mvx_dev_driver);	/* LCOV_EXCL_LINE */

	return ret;
}

void mvx_dev_exit(void)
{
	pci_unregister_driver(&mvx_pci_driver);	/* LCOV_EXCL_LINE */
	platform_driver_unregister(&mvx_dev_driver);
}
