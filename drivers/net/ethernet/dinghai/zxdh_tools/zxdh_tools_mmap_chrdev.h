#ifndef __ZXDH_DHTOOLS_MMAP_CHRDEV_H__
#define __ZXDH_DHTOOLS_MMAP_CHRDEV_H__

#include <linux/dinghai/log.h>

int __init dhtool_char_init(void);
void dhtool_char_exit(void);

#define DHTOOLS_CHRDEV_LOG_ERR(fmt, arg...) DH_LOG_ERR(MODULE_DHTOOLS, fmt, ##arg);
#define DHTOOLS_CHRDEV_LOG_INFO(fmt, arg...) DH_LOG_INFO(MODULE_DHTOOLS, fmt, ##arg);

#define DHTOOLS_CHRDEV_LOG_ERR_DEV(dev, fmt, arg...) DH_LOG_ERR_DEV(MODULE_DHTOOLS, dev, fmt, ##arg);
#define DHTOOLS_CHRDEV_LOG_INFO_DEV(dev, fmt, arg...) DH_LOG_INFO_DEV(MODULE_DHTOOLS, dev, fmt, ##arg);

#endif
