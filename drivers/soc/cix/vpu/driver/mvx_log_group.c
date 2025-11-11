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

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <linux/vmalloc.h>
#include <linux/timer.h>
#include "mvx_log.h"

/******************************************************************************
 * Defines
 ******************************************************************************/

#define MVX_TIME_NUM (1 << 11)	// 2048
#define MVX_UTIL_INTERVAL_SEC 1
#define MVX_UTIL_INTERVAL_MSEC (MVX_UTIL_INTERVAL_SEC * MSEC_PER_SEC)
#define MVX_UTIL_INTERVAL_NSEC (MVX_UTIL_INTERVAL_SEC * NSEC_PER_SEC)
#define MVX_MHZ_TO_TICKS(freq) (freq / 1000 / 1000 * 62500)
#define MVX_UTIL_INTERVAL_TICKS(freq) (MVX_MHZ_TO_TICKS(freq) * MVX_UTIL_INTERVAL_SEC)

/******************************************************************************
 * Private variables
 ******************************************************************************/

static struct mvx_log log;

static struct mvx_log_drain drain_dmesg_if;
static struct mvx_log_drain_ram drain_ram0_if;
static struct mvx_log_drain_ram drain_ram1_if;

#ifdef MVX_LOG_FTRACE_ENABLE
static struct mvx_log_drain drain_ftrace_if;
#endif /* MVX_LOG_FTRACE_ENABLE */

struct mvx_log_group mvx_log_if;
struct mvx_log_group mvx_log_fwif_if;
struct mvx_log_group mvx_log_perf;
struct mvx_log_group mvx_log_session_if;
struct mvx_log_group mvx_log_dev;

struct mvx_duration *dur_buf;
#if MVX_USE_UTILIZATION_TIMER
void mvx_log_get_util(struct timer_list *timer);
DEFINE_TIMER(util_timer, mvx_log_get_util);
#endif

/******************************************************************************
 * Static functions
 ******************************************************************************/

static int mvx_log_get_time_range(struct timespec64 *start,
				  struct mvx_duration *crange, int *count)
{
	unsigned int i, j;
	unsigned int interval_in_dticks;
	int sem_taken;
	int not_found_count = 0;
	struct mvx_duration range;
	struct mvx_time *tbuf = (struct mvx_time *)drain_ram1_if.buf;
	int ofirst =
	    (drain_ram1_if.write_pos /
	     sizeof(struct mvx_time)) & (MVX_TIME_NUM - 1);
	int olast = ofirst == 0 ? MVX_TIME_NUM - 1 : ofirst - 1;

	sem_taken = down_interruptible(&drain_ram1_if.sem);
	if (tbuf[olast].timespec.tv_sec == 0
	    && tbuf[olast].timespec.tv_nsec == 0) {
		/* VPU hasn't processed any workload */
		if (sem_taken == 0)
			up(&drain_ram1_if.sem);
		mvx_log_perf.utilization = 0;
		return 1;
	}

	if (tbuf[ofirst].timespec.tv_sec == 0
	    && tbuf[ofirst].timespec.tv_nsec == 0)
		ofirst = 0;

	/* Make sure the last workload is valid. (parse.start could be 0 for repeat frames) */
	while (tbuf[olast].parse.start == 0 && olast != ofirst)
		olast = olast == 0 ? MVX_TIME_NUM - 1 : olast - 1;

	if (timespec64_compare(start, &tbuf[olast].timespec) >= 0 ||
	    (tbuf[olast].parse.start == 0 && olast == ofirst)) {
		/* No valid workload in past one second */
		if (sem_taken == 0)
			up(&drain_ram1_if.sem);
		mvx_log_perf.utilization = 0;
		return 1;
	}

	/* Search for the first valid workload in range */
	i = ofirst;
	while (timespec64_compare(start, &tbuf[i].timespec) >= 0 ||
	       tbuf[i].parse.start == 0) {
		if (i == olast)
			break;
		i++;
		i &= (MVX_TIME_NUM - 1);
	}
	/* Collect all the time frames in range */
	interval_in_dticks =
	    MVX_UTIL_INTERVAL_TICKS(atomic_read(&mvx_log_perf.freq)) >> 1;
	range.end = tbuf[olast].pipe.start;
	if (range.end < interval_in_dticks)
		range.end += 0x80000000;
	range.start = range.end - interval_in_dticks;
	j = 0;
	i = olast;
	do {
		i--;
		i &= (MVX_TIME_NUM - 1);
		if (range.end > 0x80000000
		    && tbuf[i].pipe.start < interval_in_dticks)
			tbuf[i].pipe.start += 0x80000000;
		if (range.end > 0x80000000
		    && tbuf[i].pipe.end < interval_in_dticks)
			tbuf[i].pipe.end += 0x80000000;
		if (tbuf[i].pipe.end > range.start
		    && tbuf[i].pipe.start < range.end) {
			dur_buf[j].start = max(tbuf[i].pipe.start, range.start);
			dur_buf[j].end = min(tbuf[i].pipe.end, range.end);
			j++;
			not_found_count = 0;
		} else {
			not_found_count++;
		}
		/*
		 * Data in buffer might not in time order. So if got one data out of range,
		 * the next one could be still in the range. Just try, but limit retry count
		 * to 20 which should be a reasonable number, even in 40-session case.
		 */
	} while (i != ofirst
		 && (not_found_count < 20 || tbuf[i].parse.start == 0));

	if (sem_taken == 0)
		up(&drain_ram1_if.sem);

	*crange = range;
	*count = j;

	return 0;
}

/******************************************************************************
 * External interface
 ******************************************************************************/

int mvx_log_group_init(const char *entry_name)
{
	int ret;
	struct mvx_log_drain *drain_default = &drain_dmesg_if;
	struct mvx_log_drain *drain_ram = &drain_ram0_if.base;
	struct mvx_log_drain *drain_ram1 = &drain_ram1_if.base;

#ifdef MVX_LOG_FTRACE_ENABLE
	drain_default = &drain_ftrace_if;
#endif /* MVX_LOG_FTRACE_ENABLE */

	/* Construct log object. */
	ret = mvx_log_construct(&log, entry_name);
	if (ret != 0)
		return ret;

	/* Construct drain objects and add them to log. */
	mvx_log_drain_dmesg_construct(&drain_dmesg_if);
	ret = mvx_log_drain_add(&log, "dmesg", &drain_dmesg_if);
	if (ret != 0)
		goto delete_log_entry;

	mvx_log_drain_ram_construct(&drain_ram0_if, 64 * 1024);
	ret = mvx_log_drain_ram_add(&log, "ram0", &drain_ram0_if);
	if (ret != 0)
		goto delete_dmesg_drain;

	mvx_log_drain_ram_construct(&drain_ram1_if,
				    sizeof(struct mvx_time) * MVX_TIME_NUM);
	ret = mvx_log_drain_ram_add(&log, "ram1", &drain_ram1_if);
	if (ret != 0)
		goto delete_ram_drain;

#ifdef MVX_LOG_FTRACE_ENABLE
	mvx_log_drain_ftrace_construct(&drain_ftrace_if);
	mvx_log_drain_add(&log, "ftrace", &drain_ftrace_if);
	if (ret != 0)
		goto delete_ram1_drain;

#endif /* MVX_LOG_FTRACE_ENABLE */

	/* Construct group objects. */
	mvx_log_group_construct(&mvx_log_if, "MVX if", MVX_LOG_WARNING,
				drain_default);
	ret = mvx_log_group_add(&log, "generic", &mvx_log_if);
	if (ret != 0)
		goto delete_ftrace_drain;

	mvx_log_group_construct(&mvx_log_fwif_if, "MVX fwif", MVX_LOG_INFO,
				drain_ram);
	ret = mvx_log_group_add(&log, "firmware_interface", &mvx_log_fwif_if);
	if (ret != 0)
		goto delete_generic_group;

	mvx_log_group_construct(&mvx_log_session_if, "MVX session",
				MVX_LOG_WARNING, drain_default);
	ret = mvx_log_group_add(&log, "session", &mvx_log_session_if);
	if (ret != 0)
		goto delete_fwif_group;

	mvx_log_group_construct(&mvx_log_dev, "MVX dev", MVX_LOG_WARNING,
				drain_default);
	ret = mvx_log_group_add(&log, "dev", &mvx_log_dev);
	if (ret != 0)
		goto delete_session_group;

	mvx_log_group_construct(&mvx_log_perf, "MVX perf", MVX_LOG_INFO,
				drain_ram1);
	ret = mvx_log_group_add(&log, "perf", &mvx_log_perf);
	if (ret != 0)
		goto delete_generic_group;

#if MVX_USE_UTILIZATION_TIMER
	util_timer.function = mvx_log_get_util;
	util_timer.expires = jiffies + msecs_to_jiffies(MVX_UTIL_INTERVAL_MSEC);
	add_timer(&util_timer);
#endif

	dur_buf = vmalloc(sizeof(struct mvx_duration) * 2 * MVX_TIME_NUM);

	return 0;

delete_session_group:
	mvx_log_group_destruct(&mvx_log_session_if);

delete_fwif_group:
	mvx_log_group_destruct(&mvx_log_fwif_if);

delete_generic_group:
	mvx_log_group_destruct(&mvx_log_if);

delete_ftrace_drain:

#ifdef MVX_LOG_FTRACE_ENABLE
	mvx_log_drain_ftrace_destruct(&drain_ftrace_if);

delete_ram1_drain:
#endif /* MVX_LOG_FTRACE_ENABLE */

	mvx_log_drain_ram_destruct(&drain_ram1_if);

delete_ram_drain:
	mvx_log_drain_ram_destruct(&drain_ram0_if);

delete_dmesg_drain:
	mvx_log_drain_dmesg_destruct(&drain_dmesg_if);

delete_log_entry:
	mvx_log_destruct(&log);

	if (dur_buf)
		vfree(dur_buf);
	dur_buf = NULL;

	return ret;
}

void mvx_log_group_deinit(void)
{
	/* Destroy objects in reverse order. */
	if (dur_buf)
		vfree(dur_buf);
	dur_buf = NULL;
#if MVX_USE_UTILIZATION_TIMER
	del_timer(&util_timer);
#endif
	mvx_log_group_destruct(&mvx_log_dev);
	mvx_log_group_destruct(&mvx_log_session_if);
	mvx_log_group_destruct(&mvx_log_fwif_if);
	mvx_log_group_destruct(&mvx_log_if);

#ifdef MVX_LOG_FTRACE_ENABLE
	mvx_log_drain_ftrace_destruct(&drain_ftrace_if);
#endif /* MVX_LOG_FTRACE_ENABLE */

	mvx_log_drain_ram_destruct(&drain_ram1_if);
	mvx_log_drain_ram_destruct(&drain_ram0_if);
	mvx_log_drain_dmesg_destruct(&drain_dmesg_if);

	mvx_log_destruct(&log);
}

void mvx_log_get_util(struct timer_list *timer)
{
	int n, i, j;
	int util;
	uint32_t min, max, range, range1, range2;
	struct mvx_duration crange;
	struct mvx_duration *duration;
	struct timespec64 now, start;
	struct mvx_duration *dbuf = dur_buf;

#if MVX_USE_UTILIZATION_TIMER
	if (timer)
		mod_timer(timer,
			  jiffies + msecs_to_jiffies(MVX_UTIL_INTERVAL_MSEC));
#endif

	if (!(mvx_log_perf.enabled & MVX_LOG_PERF_UTILIZATION)
	    || dur_buf == NULL) {
		mvx_log_perf.enabled &= ~MVX_LOG_PERF_UTILIZATION;
		mvx_log_perf.utilization = -1;
		return;
	}

	ktime_get_real_ts64(&now);
	start.tv_sec = now.tv_sec > MVX_UTIL_INTERVAL_SEC ?
	    now.tv_sec - MVX_UTIL_INTERVAL_SEC : 0;
	start.tv_nsec = now.tv_nsec;

	/* To avoid too frequent refresh */
	if (timespec64_compare(&start, &mvx_log_perf.ts) < 0)
		return;
	mvx_log_perf.ts = now;

	if (mvx_log_get_time_range(&start, &crange, &n) != 0)
		return;

	/* There should be workload in VPU in past one second, calculate utilization. */
	/* Try to merge time frames */
	for (i = 0; i < n - 1; i++) {
		duration = dbuf + i;
		if (duration->start == duration->end)
			continue;
		for (j = i + 1; j < n; j++) {
			if (dbuf[j].start == dbuf[j].end)
				continue;
			min = min(duration->start, dbuf[j].start);
			max = max(duration->end, dbuf[j].end);
			range = max - min;
			range1 = duration->end - duration->start;
			range2 = dbuf[j].end - dbuf[j].start;
			if (range <= range1 + range2) {
				/* the two durations have overlap, so can be merged */
				duration->start = min;
				duration->end = max;
				dbuf[j].start = 0;
				dbuf[j].end = 0;
			}
		}
	}

	util = 0;
	for (i = 0; i < n; i++)
		util += dbuf[i].end - dbuf[i].start;
	/* Calculate utilization in unit of 0.01 percent */
	mvx_log_perf.utilization =
	    min(10000,
		(int)((uint64_t) util * 20000 /
		      MVX_UTIL_INTERVAL_TICKS(atomic_read
					      (&mvx_log_perf.freq))));
}
