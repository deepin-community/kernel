#ifndef _FUC_HOTPLUG_H_
#define _FUC_HOTPLUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/netdevice.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/err.h>
#include <linux/msi.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/dinghai/log.h>

#define FUC_HP_EVENT_ID       44
#define BUF_SIZE              0x1000

/* bar消息通道使用pf信息 */
#define FUC_HP_BAR_MSG_OFFSET (0x2000)
#define FUC_HP_VENDOR_ID      (0x1cf2)
#define FUC_HP_DEVICE_ID      (0x8044)
#define FUC_HP_IOREMAP_SIZE   (0x3000)

#define FUC_HP_POLLING_SPAN   100
#define FUC_HP_TIMEOUT_TH     3000

/* data type */
typedef unsigned long long int u64;
typedef signed long long int s64;

typedef unsigned int u32;
typedef signed int s32;

typedef unsigned short int u16;
typedef signed short int s16;

typedef unsigned char u8;
typedef signed char s8;

struct func_sel {
    unsigned int cmd;
    int (*ioctl_func)(unsigned long long arg);
};

struct zte_id_info {
    char *name;
    u16 device_id;
    u16 vendor_id;
};

#ifdef __cplusplus
}
#endif

#endif
