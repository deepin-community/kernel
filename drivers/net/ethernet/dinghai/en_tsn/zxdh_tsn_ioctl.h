#ifndef __ZXDH_TSN_IOCTL_H__
#define __ZXDH_TSN_IOCTL_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <linux/types.h>
#include <linux/hrtimer.h>

#define TSN_MSG_LEN                         (4096 - 8)

#define TSN_PORT_ID_SET                     (0)
#define TSN_PORT_ID_GET                     (1)
#define TSN_TIMER_ID_SET                    (2)
#define TSN_TIMER_ID_GET                    (3)
#define TSN_QBV_CONF_SET                    (4)
#define TSN_QBV_STATUS_GET                  (5)

#define TSN_SOFT_RESERVED_TIME              (500000)
#define TSN_HW_RESERVED_TIME                (200)
#define TSN_TIMER_RESERVED_TIME             (300000)
#define TSN_RESERVED_TIME(CT)               ((((CT) < TSN_SOFT_RESERVED_TIME)? \
                                            ((TSN_SOFT_RESERVED_TIME / (CT)) + 1) : 1) * (CT))
#define TSN_RAM_N_IN_SERVICE(RAM_N_IDLE)    ((~(RAM_N_IDLE)) & 1)

#define TSN_CYCLE_TIME_EXTENSION_MIN        (500000)

struct zxdh_tsn_msg
{
    uint32_t cmd;
    uint32_t len;
    uint8_t  data[TSN_MSG_LEN];
};

struct zxdh_tsn_ioctl_table
{
    int32_t cmd;
    int32_t (*func)(struct zxdh_tsn_private* tsn, struct zxdh_tsn_msg* msg);
};

enum hrtimer_restart zxdh_tsn_qbv_change_timer_callback(struct hrtimer* t);
int32_t zxdh_en_tsn_func(struct net_device* netdev, struct ifreq* ifr);

#ifdef __cplusplus
}
#endif
#endif /* __ZXDH_TSN_IOCTL_H__ */
