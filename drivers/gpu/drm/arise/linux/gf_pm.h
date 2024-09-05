//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd..
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China.
//*****************************************************************************

#ifndef __GF_PM_H__
#define __GF_PM_H__

#include <linux/version.h>

static inline int gf_rpm_get_sync(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    return pm_runtime_get_sync(dev);
#else
    return 0;
#endif
}

static inline int gf_rpm_put_autosuspend(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    return pm_runtime_put_autosuspend(dev);
#else
    return 0;
#endif
}

static inline void gf_rpm_use_autosuspend(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    pm_runtime_use_autosuspend(dev);
#endif
}

static inline void gf_rpm_set_autosuspend_delay(struct device *dev, int delay)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    pm_runtime_set_autosuspend_delay(dev, delay); //5s
#endif
}

static inline int gf_rpm_set_active(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    return pm_runtime_set_active(dev);
#else
    return 0;
#endif
}

static inline void gf_rpm_allow(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    pm_runtime_allow(dev);             //-1
#endif
}

static inline void gf_rpm_forbid(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    pm_runtime_forbid(dev);
#endif
}

static inline void gf_rpm_mark_last_busy(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    pm_runtime_mark_last_busy(dev);
#endif
}

static inline void gf_rpm_enable(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    pm_runtime_enable(dev);
#endif
}

static inline void gf_rpm_disable(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    pm_runtime_disable(dev);
#endif
}

static inline int gf_rpm_autosuspend(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    return pm_runtime_autosuspend(dev);
#else
    return 0;
#endif
}

static inline void gf_rpm_get_noresume(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    pm_runtime_get_noresume(dev);
#endif
}

static inline void gf_rpm_put_noidle(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    pm_runtime_put_noidle(dev);
#endif
}

static inline void gf_rpm_set_driver_flags(struct device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    dev_pm_set_driver_flags(dev, DPM_FLAG_NO_DIRECT_COMPLETE);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
    dev_pm_set_driver_flags(dev, DPM_FLAG_NEVER_SKIP);
#endif
}

#endif
