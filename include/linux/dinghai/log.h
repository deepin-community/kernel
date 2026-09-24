#ifndef __KERNEL_LOG_H__
#define __KERNEL_LOG_H__

#include <linux/kernel.h>
#include <linux/printk.h>

#define MODULE_CMD            "zxdh_cmd"
#define MODULE_NP             "zxdh_np"
#define MODULE_PF             "zxdh_pf"
#define MODULE_PTP            "zxdh_ptp"
#define MODULE_TSN            "zxdh_tsn"
#define MODULE_LAG            "zxdh_lag"
#define MODULE_DHTOOLS        "zxdh_tool"
#define MODULE_SEC            "zxdh_sec"
#define MODULE_MPF            "zxdh_mpf"
#define MODULE_FUC_HP         "zxdh_func_hp"
#define MODULE_UACCE          "zxdh_uacce"
#define MODULE_HEAL           "zxdh_health"

/* Started by AICoder, pid:l14fd6786ev003c145ea0b8800f782633a09fc8a */
extern int debug_print;
#define DH_LOG_EMERG(module, fmt, arg...)       \
    printk(KERN_EMERG "%s:%s:%d: "fmt"", KBUILD_MODNAME, __func__, __LINE__, ##arg);

#define DH_LOG_ALERT(module, fmt, arg...)       \
    printk(KERN_ALERT "%s:%s:%d: "fmt"", KBUILD_MODNAME, __func__, __LINE__, ##arg);

#define DH_LOG_CRIT(module, fmt, arg...)       \
    printk(KERN_CRIT "%s:%s:%d: "fmt"", KBUILD_MODNAME, __func__, __LINE__, ##arg);

#define DH_LOG_ERR(module, fmt, arg...)        \
    printk(KERN_ERR "%s:%s:%d: "fmt"", KBUILD_MODNAME, __func__, __LINE__, ##arg);

#define DH_LOG_WARNING(module, fmt, arg...)    \
    printk(KERN_WARNING "%s:%s:%d: "fmt"", KBUILD_MODNAME, __func__, __LINE__, ##arg);

#define DH_LOG_INFO(module, fmt, arg...)       \
    printk(KERN_INFO "%s: "fmt"", KBUILD_MODNAME, ##arg);

#define DH_LOG_DEBUG(module, fmt, arg...)      \
do {                                       \
    if (debug_print)                     \
        printk(KERN_DEBUG "%s:%s:%d: "fmt"", KBUILD_MODNAME, __func__, __LINE__, ##arg); \
} while (0)

#define dh_dev_printk_info(level, dev, fmt, arg...)                                   \
    do {                                                                              \
        if ((dev) != NULL && (dev->pdev) != NULL) {                                   \
            printk(level "%s %s: "fmt"", KBUILD_MODNAME, pci_name(dev->pdev), ##arg); \
        } else {                                                                      \
            printk(level "%s: "fmt"", KBUILD_MODNAME, ##arg);                         \
        }                                                                             \
    } while (0)

#define dh_dev_printk(level, dev, fmt, arg...)                                      \
    do {                                                                            \
        if ((dev) != NULL && (dev->pdev) != NULL) {                                 \
            printk(level "%s %s:%s:%d: "fmt"", KBUILD_MODNAME, pci_name(dev->pdev), \
                    __func__, __LINE__, ##arg);                                     \
        } else {                                                                    \
            printk(level "%s:%s:%d: "fmt"", KBUILD_MODNAME, __func__,               \
                    __LINE__, ##arg);                                               \
        }                                                                           \
    } while (0)

#define DH_LOG_EMERG_DEV(module, dev, fmt, arg...)             \
    dh_dev_printk(KERN_EMERG, dev, fmt, ##arg)

#define DH_LOG_ALERT_DEV(module, dev, fmt, arg...)             \
    dh_dev_printk(KERN_ALERT, dev, fmt, ##arg)

#define DH_LOG_CRIT_DEV(module, dev, fmt, arg...)             \
    dh_dev_printk(KERN_CRIT, dev, fmt, ##arg)

#define DH_LOG_ERR_DEV(module, dev, fmt, arg...)              \
    dh_dev_printk(KERN_ERR, dev, fmt, ##arg)

#define DH_LOG_WARNING_DEV(module, dev, fmt, arg...)           \
    dh_dev_printk(KERN_WARNING, dev, fmt, ##arg)

#define DH_LOG_INFO_DEV(module, dev, fmt, arg...)             \
    dh_dev_printk_info(KERN_INFO, dev, fmt, ##arg)

#define DH_LOG_DEBUG_DEV(module, dev, fmt, arg...)            \
    do {                                                       \
        if (debug_print)                                      \
            dh_dev_printk(KERN_DEBUG, dev, fmt, ##arg);       \
    } while (0)
/* Ended by AICoder, pid:l14fd6786ev003c145ea0b8800f782633a09fc8a */

#endif /* __KERNEL_LOG_H__ */
