#ifndef DINGHAI_HELPER_H
#define DINGHAI_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_DEV_PRINTK_OPS
#include <linux/dev_printk.h>
#endif
#include <linux/types.h>
#include <linux/dinghai/driver.h>

extern uint32_t dh_debug_mask;
#define dh_dbg(__dev, format, ...)                \
    dev_dbg((__dev)->device, "%s:%d:(pid %d): " format,        \
         __func__, __LINE__, current->pid,            \
         ##__VA_ARGS__)

#define dh_dbg_once(__dev, format, ...)        \
    dev_dbg_once((__dev)->device,        \
             "%s:%d:(pid %d): " format,        \
             __func__, __LINE__, current->pid,    \
             ##__VA_ARGS__)

#define dh_dbg_mask(__dev, mask, format, ...)        \
do {                                \
    if ((mask) & dh_debug_mask)            \
        dh_dbg(__dev, format, ##__VA_ARGS__);    \
} while (0)

#define dh_err(__dev, format, ...)            \
    dev_err((__dev)->device, "%s:%d:(pid %d): " format,    \
        __func__, __LINE__, current->pid,        \
           ##__VA_ARGS__)

#define dh_err_rl(__dev, format, ...)            \
    dev_err_ratelimited((__dev)->device,            \
                "%s:%d:(pid %d): " format,        \
                __func__, __LINE__, current->pid,    \
                ##__VA_ARGS__)

#define dh_warn(__dev, format, ...)            \
    dev_warn((__dev)->device, "%s:%d:(pid %d): " format,    \
         __func__, __LINE__, current->pid,        \
         ##__VA_ARGS__)

#define dh_warn_once(__dev, format, ...)                \
    dev_warn_once((__dev)->device, "%s:%d:(pid %d): " format,    \
              __func__, __LINE__, current->pid,            \
              ##__VA_ARGS__)

#define dh_warn_rl(__dev, format, ...)            \
    dev_warn_ratelimited((__dev)->device,            \
                 "%s:%d:(pid %d): " format,        \
                 __func__, __LINE__, current->pid,    \
                 ##__VA_ARGS__)

#define dh_info(__dev, format, ...)        \
    dev_info((__dev)->device, format, ##__VA_ARGS__)

#define dh_info_rl(__dev, format, ...)            \
    dev_info_ratelimited((__dev)->device,            \
                 "%s:%d:(pid %d): " format,        \
                 __func__, __LINE__, current->pid,    \
                 ##__VA_ARGS__)

enum {
    ZXDH_PCI_DEV_IS_VF = 1 << 0,
};

static inline bool dh_core_is_sf(const struct dh_core_dev *dev)
{
    return dev->coredev_type == DH_COREDEV_SF;
}

#ifdef __cplusplus
}
#endif

#endif