/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <cpu_ctrl.h>
#include <topo_ctrl.h>

#include <linux/soc/mediatek/mtk-pm-qos.h>
#define pm_qos_add_request(_req, _class, _value) \
    mtk_pm_qos_add_request(_req, _class, _value)
#define pm_qos_update_request(_req, _value) \
    mtk_pm_qos_update_request(_req, _value)
#define pm_qos_remove_request(_req) \
    mtk_pm_qos_remove_request(_req)
#define pm_qos_request mtk_pm_qos_request
#define PM_QOS_DDR_OPP MTK_PM_QOS_DDR_OPP

#include <linux/sched.h>

#include "precomp.h"

#define MAX_CPU_FREQ (2 * 1024 * 1024) /* in kHZ */
#define MAX_CLUSTER_NUM  3
#define CPU_BIG_CORE (0xf0)
#define CPU_SMALL_CORE (0xff - CPU_BIG_CORE)

enum ENUM_CPU_BOOST_STATUS {
	ENUM_CPU_BOOST_STATUS_INIT = 0,
	ENUM_CPU_BOOST_STATUS_START,
	ENUM_CPU_BOOST_STATUS_STOP,
	ENUM_CPU_BOOST_STATUS_NUM
};

extern int set_task_util_min(pid_t pid, unsigned int min);

/* mimic store_rps_map as net-sysfs.c does */
int wlan_set_rps_map(struct netdev_rx_queue *queue, unsigned long rps_value)
{
#if KERNEL_VERSION(4, 14, 0) <= CFG80211_VERSION_CODE
	struct rps_map *old_map, *map;
	cpumask_var_t mask;
	int cpu, i;
	static DEFINE_MUTEX(rps_map_mutex);

	if (!alloc_cpumask_var(&mask, GFP_KERNEL))
		return -ENOMEM;

	*cpumask_bits(mask) = rps_value;
	map = kzalloc(max_t(unsigned int,
			RPS_MAP_SIZE(cpumask_weight(mask)), L1_CACHE_BYTES),
			GFP_KERNEL);
	if (!map) {
		free_cpumask_var(mask);
		return -ENOMEM;
	}

	i = 0;
	for_each_cpu_and(cpu, mask, cpu_online_mask)
		map->cpus[i++] = cpu;

	if (i) {
		map->len = i;
	} else {
		kfree(map);
		map = NULL;
	}

	mutex_lock(&rps_map_mutex);
	old_map = rcu_dereference_protected(queue->rps_map,
				mutex_is_locked(&rps_map_mutex));
	rcu_assign_pointer(queue->rps_map, map);
	if (map)
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		static_branch_inc(&rps_needed);
#else
		static_key_slow_inc(&rps_needed);
#endif
	if (old_map)
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		static_branch_dec(&rps_needed);
#else
		static_key_slow_dec(&rps_needed);
#endif
	mutex_unlock(&rps_map_mutex);

	if (old_map)
		kfree_rcu(old_map, rcu);
	free_cpumask_var(mask);

	return 0;
#else
	return 0;
#endif
}

void kalSetRpsMap(IN struct GLUE_INFO *glue, IN unsigned long value)
{
	int32_t i = 0, j = 0;
	struct net_device *dev = NULL;

	for (i = 0; i < BSS_DEFAULT_NUM; i++) {
		dev = wlanGetNetDev(glue, i);
		if (dev) {
			for (j = 0; j < dev->real_num_rx_queues; ++j)
				wlan_set_rps_map(&dev->_rx[j], value);
		}
	}
}

uint32_t kalGetCpuBoostThreshold(void)
{
	DBGLOG(SW4, TRACE, "enter kalGetCpuBoostThreshold\n");
	/* 5, stands for 250Mbps */
	return 5;
}

int32_t kalBoostCpu(IN struct ADAPTER *prAdapter,
		    IN uint32_t u4TarPerfLevel,
		    IN uint32_t u4BoostCpuTh)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct cpu_ctrl_data *freq_to_set = NULL;
	int32_t i = 0, i4Freq = -1;

	static struct pm_qos_request wifi_qos_request;
	static u_int8_t fgRequested = ENUM_CPU_BOOST_STATUS_INIT;

	uint32_t u4ClusterNum = topo_ctrl_get_nr_clusters();

	DBGLOG(SW4, INFO, "ClusterNum:%u TarPerfLevel:%u BoostCpuTh:%u\n",
			u4ClusterNum, u4TarPerfLevel, u4BoostCpuTh);
	freq_to_set = kcalloc(u4ClusterNum,
			sizeof(struct cpu_ctrl_data), GFP_KERNEL);

	if (!freq_to_set) {
		DBGLOG(SW4, ERROR, "alloc mem (%d) fail!\n",
			sizeof(struct cpu_ctrl_data));
		return -1;
	}

	prGlueInfo = (struct GLUE_INFO *)wiphy_priv(wlanGetWiphy());
	ASSERT(u4ClusterNum <= MAX_CLUSTER_NUM);
	/* ACAO, we dont have to set core number */
	i4Freq = (u4TarPerfLevel >= u4BoostCpuTh) ? MAX_CPU_FREQ : -1;
	for (i = 0; i < u4ClusterNum; i++) {
		freq_to_set[i].min = i4Freq;
		freq_to_set[i].max = i4Freq;
	}

	if (fgRequested == ENUM_CPU_BOOST_STATUS_INIT) {
		/* initially enable rps working at small cores */
		kalSetRpsMap(prGlueInfo, CPU_SMALL_CORE);
		fgRequested = ENUM_CPU_BOOST_STATUS_STOP;
	}

	if (u4TarPerfLevel >= u4BoostCpuTh) {
		if (fgRequested == ENUM_CPU_BOOST_STATUS_STOP) {
			pr_info("kalBoostCpu start (%d>=%d)\n",
				u4TarPerfLevel, u4BoostCpuTh);
			fgRequested = ENUM_CPU_BOOST_STATUS_START;

			set_task_util_min(prGlueInfo->u4TxThreadPid, SCHED_FIXEDPOINT_SCALE);
			set_task_util_min(prGlueInfo->u4RxThreadPid, SCHED_FIXEDPOINT_SCALE);
			set_task_util_min(prGlueInfo->u4HifThreadPid, SCHED_FIXEDPOINT_SCALE);
			kalSetRpsMap(prGlueInfo, CPU_BIG_CORE);
			update_userlimit_cpu_freq(CPU_KIR_WIFI,
				u4ClusterNum, freq_to_set);

			pr_info("Max Dram Freq start\n");
			pm_qos_add_request(&wifi_qos_request,
					   PM_QOS_DDR_OPP,
					   DDR_OPP_0);
			pm_qos_update_request(&wifi_qos_request, DDR_OPP_0);
		}
	} else {
		if (fgRequested == ENUM_CPU_BOOST_STATUS_START) {
			pr_info("kalBoostCpu stop (%d<%d)\n",
				u4TarPerfLevel, u4BoostCpuTh);
			fgRequested = ENUM_CPU_BOOST_STATUS_STOP;

			set_task_util_min(prGlueInfo->u4TxThreadPid, 0);
			set_task_util_min(prGlueInfo->u4RxThreadPid, 0);
			set_task_util_min(prGlueInfo->u4HifThreadPid, 0);
			kalSetRpsMap(prGlueInfo, CPU_SMALL_CORE);
			update_userlimit_cpu_freq(CPU_KIR_WIFI,
				u4ClusterNum, freq_to_set);

			pr_info("Max Dram Freq end\n");
			pm_qos_update_request(&wifi_qos_request, DDR_OPP_UNREQ);
			pm_qos_remove_request(&wifi_qos_request);
		}
	}

	kfree(freq_to_set);
	return 0;
}

