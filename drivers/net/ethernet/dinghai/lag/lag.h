#ifndef _ZXDH_ETH_LAG_H_
#define _ZXDH_ETH_LAG_H_

#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/lag.h>
#include <linux/dinghai/log.h>

#define LAG_LOG_ERR(fmt, arg...)    DH_LOG_ERR(MODULE_LAG, fmt, ##arg);
#define LAG_LOG_INFO(fmt, arg...)   DH_LOG_INFO(MODULE_LAG, fmt, ##arg);
#define LAG_LOG_DEBUG(fmt, arg...)  DH_LOG_DEBUG(MODULE_LAG, fmt, ##arg);
#define LAG_LOG_WARN(fmt, arg...)   DH_LOG_WARNING(MODULE_LAG, fmt, ##arg);

#define LAG_LOG_ERR_DEV(dev, fmt, arg...)    DH_LOG_ERR_DEV(MODULE_LAG, dev, fmt, ##arg);
#define LAG_LOG_INFO_DEV(dev, fmt, arg...)   DH_LOG_INFO_DEV(MODULE_LAG, dev, fmt, ##arg);
#define LAG_LOG_DEBUG_DEV(dev, fmt, arg...)  DH_LOG_DEBUG_DEV(MODULE_LAG, dev, fmt, ##arg);
#define LAG_LOG_WARN_DEV(dev, fmt, arg...)   DH_LOG_WARNING_DEV(MODULE_LAG, dev, fmt, ##arg);

/* max panel port */
#define ZXDH_MAX_PORTS              (10)

#define ZXDH_ACTIVE_PHY_PORT_NA     (0xFF)

#define PCIE_ID_PF_INDEX_MASK       (0x7)
#define PCIE_ID_EP_INDEX_MASK       (0x7)

enum
{
    ZXDH_LAG_FLAG_BACKUP = 1 << 0,
    ZXDH_LAG_FLAG_HASH   = 1 << 1,
    ZXDH_LAG_FLAG_READY  = 1 << 2,
};

#define ZXDH_LAG_MODE_FLAGS (ZXDH_LAG_FLAG_BACKUP | ZXDH_LAG_FLAG_HASH)

enum
{
    LAG_FLAGS_DISABLE = 0,
    LAG_FLAGS_ENABLE = 1,
};

enum
{
    LAG_MODE_ACTIVE_BACKUP = 1,
    LAG_MODE_802_3AD = 2,
};

/* define hash type for NP SDK */
enum
{
    ZXDH_NP_HASH_TYPE_DEFAULT   = 0,
    ZXDH_NP_HASH_TYPE_L2        = 1,
    ZXDH_NP_HASH_TYPE_L23       = 2,
    ZXDH_NP_HASH_TYPE_L34       = 4,
};

struct lag_func
{
    bool valid;
    struct dh_core_dev   *dev;
    struct net_device    *netdev;
    struct zxdh_lag_attrs attrs;
};

#ifdef CGS_V5_693
enum netdev_lag_hash {
	NETDEV_LAG_HASH_NONE,
	NETDEV_LAG_HASH_L2,
	NETDEV_LAG_HASH_L34,
	NETDEV_LAG_HASH_L23,
	NETDEV_LAG_HASH_E23,
	NETDEV_LAG_HASH_E34,
	NETDEV_LAG_HASH_UNKNOWN,
};
#endif
struct lag_tracker
{
    enum   netdev_lag_tx_type           tx_type;
    struct netdev_lag_lower_state_info  netdev_state[ZXDH_MAX_PORTS];
    bool is_bonded;
    enum netdev_lag_hash hash_type;
    char master_name[IFNAMSIZ];
};

struct zxdh_lag
{
    uint32_t flags;
    uint32_t lag_func_index;
    uint32_t slaves;
    int32_t  mode_changes_in_progress;
    uint8_t lag_id;
    struct kref ref;
    struct lag_func           lagfunc[ZXDH_MAX_PORTS];
    struct workqueue_struct   *wq;
    struct delayed_work       bond_work;
    struct notifier_block     nb;
    struct lag_tracker        tracker;
    struct dh_core_dev        *parent;

    struct zxdh_lag_if        *ops;
};

static inline bool lag_is_ready(struct zxdh_lag *ldev)
{
    return true;
}

static inline bool lag_is_backup(struct zxdh_lag *ldev)
{
    return ldev->tracker.tx_type == NETDEV_LAG_TX_TYPE_ACTIVEBACKUP ? true : false;
}

static inline bool lag_is_hash(struct zxdh_lag *ldev)
{
    return ldev->tracker.tx_type == NETDEV_LAG_TX_TYPE_HASH ? true : false;
}

static inline bool lag_is_port_invalid(struct zxdh_lag *ldev, uint32_t index)
{
    BUG_ON(index >= ZXDH_MAX_PORTS);
    return !ldev->lagfunc[index].dev || !ldev->lagfunc[index].netdev;
}

void ldev_kref_free(struct kref *ref);
static inline void get_ldev_kref(struct zxdh_lag *ldev)
{
    kref_get(&ldev->ref);
}

static inline void put_ldev_kref(struct zxdh_lag *ldev)
{
    kref_put(&ldev->ref, ldev_kref_free);
}

#endif /* _ZXDH_ETH_LAG_H_ */