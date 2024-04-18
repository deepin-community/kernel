/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and * confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <linux/devfreq.h>
#include <linux/math64.h>
#include "mwv207_devfreq.h"
#include "mwv207_vbios.h"
#include "mwv207_sched.h"

#define MIN_FREQ 50
#define MAX_FREQ 600

static void mwv207_devfreq_update_utilization(struct mwv207_pipe *pipe)
{
	ktime_t now;
	ktime_t last;

	now = ktime_get();
	last = pipe->time_last_update;

	dev_dbg(pipe->dev, "pipe [%s] busy_count: %d", pipe->fname, pipe->busy_count);

	if (pipe->busy_count > 0)
		pipe->busy_time += ktime_sub(now, last);
	else
		pipe->idle_time += ktime_sub(now, last);

	pipe->time_last_update = now;
}

static int mwv207_devfreq_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct mwv207_pipe *pipe = dev_get_drvdata(dev);
	unsigned long freq_khz;
	int ret;

	ret = mwv207_vbios_get_pll(pipe->jdev, pipe->pll_id, &freq_khz);
	if (ret)
		*freq = MIN_FREQ;
	else
		*freq = freq_khz / 1000;

	return 0;
}

static int mwv207_devfreq_target(struct device *dev, unsigned long *freq, u32 flags)
{
	struct mwv207_pipe *pipe = dev_get_drvdata(dev);
	struct dev_pm_opp *opp;
	unsigned long target_freq;
	int ret;

	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp))
		return PTR_ERR(opp);

	target_freq = dev_pm_opp_get_freq(opp);
	dev_pm_opp_put(opp);

	dev_dbg(dev, "pipe [%s] target freq: %lu", pipe->fname, target_freq);

	ret = mwv207_vbios_set_pll(pipe->jdev, pipe->pll_id, target_freq * 1000);
	if (ret)
		return ret;

	return 0;
}

static void mwv207_devfreq_pipe_reset(struct mwv207_pipe *pipe)
{
	pipe->busy_time = 0;
	pipe->idle_time = 0;
	pipe->time_last_update = ktime_get();
}

static int mwv207_devfreq_get_dev_status(struct device *dev, struct devfreq_dev_status *status)
{
	struct mwv207_pipe *pipe = dev_get_drvdata(dev);
	unsigned long flags;

	mwv207_devfreq_get_cur_freq(dev, &status->current_frequency);

	spin_lock_irqsave(&pipe->devfreq_lock, flags);

	mwv207_devfreq_update_utilization(pipe);

	status->total_time = ktime_to_ns(ktime_add(pipe->busy_time,
						   pipe->idle_time));

	status->busy_time = ktime_to_ns(pipe->busy_time);

	mwv207_devfreq_pipe_reset(pipe);

	spin_unlock_irqrestore(&pipe->devfreq_lock, flags);

	return 0;
}

static struct devfreq_dev_profile mwv207_devfreq_profile = {
	.polling_ms = 50,
	.target = mwv207_devfreq_target,
	.get_dev_status = mwv207_devfreq_get_dev_status,
	.get_cur_freq = mwv207_devfreq_get_cur_freq,
};

static ssize_t utilization_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mwv207_pipe *pipe = dev_get_drvdata(dev);
	unsigned long busy_time, total_time, flags;

	spin_lock_irqsave(&pipe->devfreq_lock, flags);

	mwv207_devfreq_update_utilization(pipe);

	total_time = ktime_to_ns(ktime_add(pipe->busy_time, pipe->idle_time));

	busy_time = ktime_to_ns(pipe->busy_time);

	mwv207_devfreq_pipe_reset(pipe);

	spin_unlock_irqrestore(&pipe->devfreq_lock, flags);

	if (total_time == 0)
		return sprintf(buf, "%d\n", 100);
	else
		return sprintf(buf, "%llu\n", div_u64(busy_time, total_time / 100));
}

static DEVICE_ATTR_RO(utilization);

void mwv207_devfreq_unregister(struct mwv207_pipe *pipe)
{
	if (!pipe->devfreq)
		return;

	device_remove_file(pipe->dev, &dev_attr_utilization);
	sysfs_remove_link(&pipe->dev->kobj, "stats");
	dev_pm_opp_remove_all_dynamic(pipe->dev);
	device_unregister(pipe->dev);
}

static void mwv207_devfreq_dev_release(struct device *dev)
{
	kfree(dev);
}

int mwv207_devfreq_register(struct mwv207_pipe *pipe, unsigned long max_freq)
{
	struct device  *dev;
	unsigned long freq;
	int ret;

	dev = kzalloc(sizeof(struct device), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->parent = pipe->jdev->dev;
	dev->release = mwv207_devfreq_dev_release;
	dev_set_name(dev, "devfreq_%s", pipe->fname);
	dev_set_drvdata(dev, pipe);

	ret = device_register(dev);
	if (ret) {
		put_device(dev);
		goto err_out;
	}

	pipe->dev = dev;
	pipe->busy_count = 0;
	spin_lock_init(&pipe->devfreq_lock);

	mwv207_devfreq_profile.initial_freq = max_freq;

	for (freq = MIN_FREQ; freq <= max_freq; freq += 50) {
		ret = dev_pm_opp_add(dev, freq, 0);
		if (ret)
			goto remove_opps;
	}

	pipe->devfreq = devm_devfreq_add_device(dev, &mwv207_devfreq_profile,
					  DEVFREQ_GOV_PERFORMANCE, NULL);
	if (IS_ERR(pipe->devfreq)) {
		ret = PTR_ERR(pipe->devfreq);
		dev_dbg(dev, "Couldn't initialize devfreq: %d", ret);
		goto remove_opps;
	}

	mwv207_devfreq_pipe_reset(pipe);

	pipe->devfreq->suspend_freq = MIN_FREQ;

	ret = sysfs_create_link(&dev->kobj, &pipe->devfreq->dev.kobj, "stats");
	if (ret) {
		dev_dbg(dev, "Couldn't create sys link for stats: %d", ret);
		goto remove_opps;
	}

	ret = device_create_file(dev, &dev_attr_utilization);
	if (ret) {
		dev_dbg(dev, "Couldn't create device file for utilization: %d", ret);
		goto remove_link;
	}

	return 0;

remove_link:
	sysfs_remove_link(&dev->kobj, "stats");
remove_opps:
	dev_pm_opp_remove_all_dynamic(dev);
	device_unregister(dev);
err_out:
	return ret;
}

int mwv207_devfreq_resume(struct mwv207_pipe *pipe)
{
	unsigned long flags;

	if (!pipe->devfreq)
		return 0;

	spin_lock_irqsave(&pipe->devfreq_lock, flags);

	mwv207_devfreq_pipe_reset(pipe);

	spin_unlock_irqrestore(&pipe->devfreq_lock, flags);

	return devfreq_resume_device(pipe->devfreq);
}

int mwv207_devfreq_suspend(struct mwv207_pipe *pipe)
{
	if (!pipe->devfreq)
		return 0;

	return devfreq_suspend_device(pipe->devfreq);
}

void mwv207_devfreq_record_busy(struct mwv207_pipe *pipe)
{
	unsigned long flags;

	if (!pipe->devfreq)
		return;

	spin_lock_irqsave(&pipe->devfreq_lock, flags);

	mwv207_devfreq_update_utilization(pipe);
	pipe->busy_count++;

	spin_unlock_irqrestore(&pipe->devfreq_lock, flags);
}

void mwv207_devfreq_record_idle(struct mwv207_pipe *pipe)
{
	unsigned long flags;

	if (!pipe->devfreq)
		return;

	spin_lock_irqsave(&pipe->devfreq_lock, flags);

	mwv207_devfreq_update_utilization(pipe);
	WARN_ON(--pipe->busy_count < 0);

	spin_unlock_irqrestore(&pipe->devfreq_lock, flags);
}
