#ifndef _ZX_PTP_COMMON_H
#define _ZX_PTP_COMMON_H

#include <linux/dinghai/log.h>

#define PTP_LOG_ERR(fmt, arg...) DH_LOG_ERR(MODULE_PTP, fmt, ##arg);
#define PTP_LOG_INFO(fmt, arg...) DH_LOG_INFO(MODULE_PTP, fmt, ##arg);
#define PTP_LOG_DEBUG(fmt, arg...) DH_LOG_DEBUG(MODULE_PTP, fmt, ##arg);
#define PTP_LOG_WARN(fmt, arg...) DH_LOG_WARNING(MODULE_PTP, fmt, ##arg);

#define PTP_LOG_ERR_DEV(dev, fmt, arg...) DH_LOG_ERR_DEV(MODULE_PTP, dev, fmt, ##arg);
#define PTP_LOG_INFO_DEV(dev, fmt, arg...) DH_LOG_INFO_DEV(MODULE_PTP, dev, fmt, ##arg);
#define PTP_LOG_DEBUG_DEV(dev, fmt, arg...) DH_LOG_DEBUG_DEV(MODULE_PTP, dev, fmt, ##arg);
#define PTP_LOG_WARN_DEV(dev, fmt, arg...) DH_LOG_WARNING_DEV(MODULE_PTP, dev, fmt, ##arg);

#endif