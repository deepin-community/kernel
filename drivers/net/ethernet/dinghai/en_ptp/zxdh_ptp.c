#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/dinghai/driver.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_address.h>

#ifdef CGS_V5_693
MODULE_SOFTDEP("pre: ptp");
#endif

#include "en_aux.h"
#include "slib.h"
#include "zxdh_ptp.h"
#include "zxdh_ptp_regs.h"
#include "zxdh_ptp_common.h"

#ifdef CGS_V5_693
/* 老内核缺少 scaled_ppm_to_ppb() 助手，这里按新内核算法补一份：
 * scaled_ppm 单位为 ppm<<16，先转成 ppb，再右移 16。
 */
static s32 scaled_ppm_to_ppb(long scaled_ppm)
{
   return (s32)((scaled_ppm * 1000L) >> 16);
}
/* ptp_clock_index() 在 3.10.0-693.21.1.el7.x86_64 内核中未导出，这里提供一个本地实现
 * 由于 struct ptp_clock 是不完整类型，我们使用我们自己的 index 数组
 * 注意：所有 ptp_clock_index(adapter->ptp_clock[idx]) 调用已替换为 zxdh_ptp_get_clock_idx(adapter, idx)
 */
static inline int zxdh_ptp_get_clock_idx(struct zxdh_ptp_private *adapter, int idx)
{
	return adapter->ptp_clock_idx[idx];
}

/* 覆盖 kernel 的 ptp_clock_index 声明，避免链接错误 */
#define ptp_clock_index(x) (-1)
#endif

#define ZXDH_PF_BAR0 0

char pps[3][15] = {
    "pp1s_out",
    "pp1s_1588",
    "pp1s_external"
};

spinlock_t global_ptpm_lock;
u_int32_t ptpm_lock_init_stat = 0;
// all tsn timer name: tsn0 tsn1 tsn2 tsn3, so the return value is 0/1/2/3.
static int get_tsn_timer_no(char *clock_name)
{
    int ret;
    int timer_no;
    ret = sscanf(clock_name, "tsn%d", &timer_no);
    if(ret != 1)
    {
        PTP_LOG_ERR(" tsn: %s get timer no fail!\n", clock_name);
        return -1;
    }
    return timer_no;
}

static uint32_t tsn_clock_cycle_integer_reg(int timer_no)
{
    if (0 == timer_no)
        return TSN_CLOCK_CYCLE_INTEGER(0);
    else if (1 == timer_no)
        return TSN_CLOCK_CYCLE_INTEGER(1);
    else if (2 == timer_no)
        return TSN_CLOCK_CYCLE_INTEGER(2);
    else if (3 == timer_no)
        return TSN_CLOCK_CYCLE_INTEGER(3);

    return 0;
}

static uint32_t tsn_clock_cycle_fraction_reg(int timer_no)
{
    if (0 == timer_no)
        return TSN_CLOCK_CYCLE_FRACTION(0);
    else if (1 == timer_no)
        return TSN_CLOCK_CYCLE_FRACTION(1);
    else if (2 == timer_no)
        return TSN_CLOCK_CYCLE_FRACTION(2);
    else if (3 == timer_no)
        return TSN_CLOCK_CYCLE_FRACTION(3);

    return 0;
}

#define GET_TSN_ADJUST_NANO_REG(tsn_no)            \
    ({                                             \
        if (0 == tsn_no)                           \
            nano_sec_reg = TSN_ADJUST_NANO_SEC(0); \
        else if (1 == tsn_no)                      \
            nano_sec_reg = TSN_ADJUST_NANO_SEC(1); \
        else if (2 == tsn_no)                      \
            nano_sec_reg = TSN_ADJUST_NANO_SEC(2); \
        else if (3 == tsn_no)                      \
            nano_sec_reg = TSN_ADJUST_NANO_SEC(3); \
    })

#define GET_TSN_ADJUST_LOW_SEC_REG(tsn_no)          \
    ({                                              \
        if (0 == tsn_no)                            \
            low_sec_reg = TSN_ADJUST_LOW_SECOND(0); \
        else if (1 == tsn_no)                       \
            low_sec_reg = TSN_ADJUST_LOW_SECOND(1); \
        else if (2 == tsn_no)                       \
            low_sec_reg = TSN_ADJUST_LOW_SECOND(2); \
        else if (3 == tsn_no)                       \
            low_sec_reg = TSN_ADJUST_LOW_SECOND(3); \
    })

#define GET_TSN_ADJUST_HIGH_SEC_REG(tsn_no)           \
    ({                                                \
        if (0 == tsn_no)                              \
            high_sec_reg = TSN_ADJUST_HIGH_SECOND(0); \
        else if (1 == tsn_no)                         \
            high_sec_reg = TSN_ADJUST_HIGH_SECOND(1); \
        else if (2 == tsn_no)                         \
            high_sec_reg = TSN_ADJUST_HIGH_SECOND(2); \
        else if (3 == tsn_no)                         \
            high_sec_reg = TSN_ADJUST_HIGH_SECOND(3); \
    })

#define GET_TSN_ADJUST_FRAC_NANO_SEC_REG(tsn_no)         \
    ({                                                   \
        if (0 == tsn_no)                                 \
            frac_nano_reg = TSN_ADJUST_FRAC_NANO_SEC(0); \
        else if (1 == tsn_no)                            \
            frac_nano_reg = TSN_ADJUST_FRAC_NANO_SEC(1); \
        else if (2 == tsn_no)                            \
            frac_nano_reg = TSN_ADJUST_FRAC_NANO_SEC(2); \
        else if (3 == tsn_no)                            \
            frac_nano_reg = TSN_ADJUST_FRAC_NANO_SEC(3); \
    })

#define GET_TSN_LATCH_NANO_REG(tsn_no)            \
    ({                                            \
        if (0 == tsn_no)                          \
            nano_sec_reg = TSN_LATCH_NANO_SEC(0); \
        else if (1 == tsn_no)                     \
            nano_sec_reg = TSN_LATCH_NANO_SEC(1); \
        else if (2 == tsn_no)                     \
            nano_sec_reg = TSN_LATCH_NANO_SEC(2); \
        else if (3 == tsn_no)                     \
            nano_sec_reg = TSN_LATCH_NANO_SEC(3); \
    })

#define GET_TSN_LATCH_LOW_SEC_REG(tsn_no)          \
    ({                                             \
        if (0 == tsn_no)                           \
            low_sec_reg = TSN_LATCH_LOW_SECOND(0); \
        else if (1 == tsn_no)                      \
            low_sec_reg = TSN_LATCH_LOW_SECOND(1); \
        else if (2 == tsn_no)                      \
            low_sec_reg = TSN_LATCH_LOW_SECOND(2); \
        else if (3 == tsn_no)                      \
            low_sec_reg = TSN_LATCH_LOW_SECOND(3); \
    })

#define GET_TSN_LATCH_HIGH_SEC_REG(tsn_no)           \
    ({                                               \
        if (0 == tsn_no)                             \
            high_sec_reg = TSN_LATCH_HIGH_SECOND(0); \
        else if (1 == tsn_no)                        \
            high_sec_reg = TSN_LATCH_HIGH_SECOND(1); \
        else if (2 == tsn_no)                        \
            high_sec_reg = TSN_LATCH_HIGH_SECOND(2); \
        else if (3 == tsn_no)                        \
            high_sec_reg = TSN_LATCH_HIGH_SECOND(3); \
    })

#define GET_TSN_LATCH_FRAC_NANO_SEC_REG(tsn_no)         \
    ({                                                  \
        if (0 == tsn_no)                                \
            frac_nano_reg = TSN_LATCH_FRAC_NANO_SEC(0); \
        else if (1 == tsn_no)                           \
            frac_nano_reg = TSN_LATCH_FRAC_NANO_SEC(1); \
        else if (2 == tsn_no)                           \
            frac_nano_reg = TSN_LATCH_FRAC_NANO_SEC(2); \
        else if (3 == tsn_no)                           \
            frac_nano_reg = TSN_LATCH_FRAC_NANO_SEC(3); \
    })

#define GET_PPS_LATCH_TSN_NANO_REG(tsn_no)            \
    ({                                                \
        if (1 == tsn_no)                              \
            nano_sec_reg = PPS_LATCH_TSN_NANO_SEC(0); \
        else if (2 == tsn_no)                         \
            nano_sec_reg = PPS_LATCH_TSN_NANO_SEC(1); \
        else if (3 == tsn_no)                         \
            nano_sec_reg = PPS_LATCH_TSN_NANO_SEC(2); \
        else if (4 == tsn_no)                         \
            nano_sec_reg = PPS_LATCH_TSN_NANO_SEC(3); \
    })

#define GET_PPS_LATCH_TSN_LOW_SEC_REG(tsn_no)          \
    ({                                                 \
        if (1 == tsn_no)                               \
            low_sec_reg = PPS_LATCH_TSN_LOW_SECOND(0); \
        else if (2 == tsn_no)                          \
            low_sec_reg = PPS_LATCH_TSN_LOW_SECOND(1); \
        else if (3 == tsn_no)                          \
            low_sec_reg = PPS_LATCH_TSN_LOW_SECOND(2); \
        else if (4 == tsn_no)                          \
            low_sec_reg = PPS_LATCH_TSN_LOW_SECOND(3); \
    })

#define GET_PPS_LATCH_TSN_HIGH_SEC_REG(tsn_no)           \
    ({                                                   \
        if (1 == tsn_no)                                 \
            high_sec_reg = PPS_LATCH_TSN_HIGH_SECOND(0); \
        else if (2 == tsn_no)                            \
            high_sec_reg = PPS_LATCH_TSN_HIGH_SECOND(1); \
        else if (3 == tsn_no)                            \
            high_sec_reg = PPS_LATCH_TSN_HIGH_SECOND(2); \
        else if (4 == tsn_no)                            \
            high_sec_reg = PPS_LATCH_TSN_HIGH_SECOND(3); \
    })

#define GET_PPS_LATCH_TSN_FRAC_NANO_SEC_REG(tsn_no)         \
    ({                                                      \
        if (1 == tsn_no)                                    \
            frac_nano_reg = PPS_LATCH_TSN_FRAC_NANO_SEC(0); \
        else if (2 == tsn_no)                               \
            frac_nano_reg = PPS_LATCH_TSN_FRAC_NANO_SEC(1); \
        else if (3 == tsn_no)                               \
            frac_nano_reg = PPS_LATCH_TSN_FRAC_NANO_SEC(2); \
        else if (4 == tsn_no)                               \
            frac_nano_reg = PPS_LATCH_TSN_FRAC_NANO_SEC(3); \
    })

enum reg_module
{
    PTP_TOP,
    PTP_M,
    PTP_S0,
    PTP_S1,
    PTP_S2,
};

static inline uint32_t zxdh_read_reg(uint64_t base_addr, uint32_t offset)
{
    return readl((const volatile void *)(base_addr + offset));
}

static inline void zxdh_write_reg(uint64_t base_addr, uint32_t offset, uint32_t val)
{
    writel(val, (volatile void *)(base_addr + offset));
}

static struct zxdh_ptp_private* zxdh_ptp_get_ptp_private(struct zxdh_en_device *en_dev)
{
    struct zxdh_pf_device *pf_dev;
    /* Started by AICoder, pid:xb4bc4896fi268d144270abd6054e701bd21ae4f */
    PTP_COMM_CHECK_POINT_RET(en_dev,NULL);
    /* Ended by AICoder, pid:xb4bc4896fi268d144270abd6054e701bd21ae4f */

    pf_dev = dh_core_priv(en_dev->parent->parent);
    PTP_COMM_CHECK_POINT_RET(pf_dev,NULL);

    return pf_dev->ptp;
}

static bool zxdh_pf_is_evb(struct zxdh_pf_device *pf_dev)
{
    uint8_t product;
    uint64_t vaddr = 0;
    
    PTP_COMM_CHECK_POINT_RET(pf_dev, false);
    vaddr = (uint64_t)ZXDH_BAR_FWCAP(pf_dev->pci_ioremap_addr[0]);

    product = readb((const volatile void __iomem *)(vaddr + ZXDH_PRODUCT_TYPE));
    if ((product == ZXDH_PRODUCT_EVB_EP0)
        || (product == ZXDH_PRODUCT_EVB_EP0_EP4))
    {
        return true;
    }
    return false;
}
static int zx_ptp_adjfine(struct ptp_clock_info *ptp, long scaled_ppm)
{
    unsigned long flags;
    int neg_adj = 0;
    uint64_t diff_nano;
    uint64_t adjust_nano;
    uint64_t nominal_nano;
    uint64_t base_addr;
    s32 ppb;
    struct zxdh_ptp_private *adapter = NULL;

    PTP_COMM_CHECK_POINT_RET(ptp, PTP_PARA_CHK_POINT_NULL);

    adapter = container_of(ptp, struct zxdh_ptp_private, ptp_caps[0]);
    if(adapter == NULL) 
    {
        PTP_LOG_ERR("zx_ptp_adjfine adapter null\n");
        return -1;
    }

    nominal_nano = ((uint64_t)(1) << 32) + 0;

    ppb = scaled_ppm_to_ppb(scaled_ppm);
    PTP_LOG_DEBUG("name: %s, ppb: %d\n", ptp->name, ppb);
    if (ppb < 0)
    {
        ppb     = -ppb;
        neg_adj = 1;
    }
    diff_nano = ((uint64_t)(ppb) << 32) / 1000000000;

    /* positive adjust */
    if (0 == neg_adj)
    {
        adjust_nano = nominal_nano + diff_nano;
    }
    else /* negative adjust */
    {
        adjust_nano = (nominal_nano > diff_nano) ? (nominal_nano - diff_nano) : 0;
    }
    PTP_LOG_DEBUG("adjust_nano: 0x%llx\n", adjust_nano);

    base_addr = adapter->ptpm_addr;

    spin_lock_irqsave(&global_ptpm_lock, flags);
    zxdh_write_reg(base_addr, PTP_CLOCK_CYCLE_INTEGER, (uint32_t)(adjust_nano >> 32));
    zxdh_write_reg(base_addr, PTP_CLOCK_CYCLE_FRACTION, (uint32_t)(adjust_nano & 0xffffffff));
    zxdh_write_reg(base_addr, CLOCK_CYCLE_UPDATE, 1);
    spin_unlock_irqrestore(&global_ptpm_lock, flags);

    return 0;
}


static int zxdh_ptp_adjtime(struct ptp_clock_info *ptp_clock, s64 delta)
{
    unsigned long flags;
    uint32_t run_mode;
    uint32_t reg_val;
    uint64_t adjust;
    // s32 rem;
    uint64_t sec;
    uint32_t nsec;

    uint64_t base_addr;
    struct zxdh_ptp_private *adapter = NULL;


    PTP_COMM_CHECK_POINT_RET(ptp_clock,PTP_PARA_CHK_POINT_NULL);
    PTP_LOG_DEBUG("name: %s, delta: %lld\n", ptp_clock->name, delta);

    adapter   = container_of(ptp_clock, struct zxdh_ptp_private, ptp_caps[0]);
    if(adapter == NULL) 
    {
        PTP_LOG_ERR("zxdh_ptp_adjtime adapter null\n");
        return -1;
    }
    base_addr = adapter->ptpm_addr;

    spin_lock_irqsave(&global_ptpm_lock, flags);
    // timecounter_adjtime(&adapter->tc, delta);

    /* 1588 timer, update mode */
    if (delta > 0)
    {
        run_mode = INCRE_MODE;
        adjust   = delta;
    }
    else
    {
        run_mode = DECRE_MODE;
        adjust   = -delta;
    }

    /* adjust value */
    sec = div_u64_rem(adjust, NSEC_PER_SEC, &nsec);
    PTP_LOG_DEBUG("sec: %llu, nsec: %u\n", sec, nsec);
    // nsec = rem;
    zxdh_write_reg(base_addr, ADJUST_HIGH_TOD_SECOND, (uint32_t)(sec >> 32));
    zxdh_write_reg(base_addr, ADJUST_LOWER_TOD_SECOND, (uint32_t)(sec & 0xffffffff));
    zxdh_write_reg(base_addr, ADJUST_TOD_NANO_SECOND, nsec);

    reg_val = zxdh_read_reg(base_addr, PTP_CONFIGURATION);
    reg_val &= ~(0x3 << 4);
    reg_val |= ((run_mode << 4) | (1 << TIMER_1588_UPT_EN_BIT));
    zxdh_write_reg(base_addr, PTP_CONFIGURATION, reg_val);

    /* enable adjust */
    zxdh_write_reg(base_addr, TIMER_CONTROL, 1 << 1);

    spin_unlock_irqrestore(&global_ptpm_lock, flags);

    return 0;
}


static int zxdh_ptp_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
    uint32_t ns;
    uint64_t s;
    uint32_t reg_val;
    unsigned long flags;
    struct zxdh_ptp_private *adapter = NULL;
    uint64_t base_addr;
    //ptp，ts
    PTP_COMM_CHECK_POINT_RET(ptp,PTP_PARA_CHK_POINT_NULL);
    PTP_COMM_CHECK_POINT_RET(ts,PTP_PARA_CHK_POINT_NULL);

    adapter = container_of(ptp, struct zxdh_ptp_private, ptp_caps[0]);
    PTP_LOG_DEBUG("name: %s\n", ptp->name);

    if(adapter == NULL) 
    {
        PTP_LOG_ERR("zxdh_ptp_gettime adapter null\n");
        return -1;
    }
    base_addr = adapter->ptpm_addr;

    mutex_lock(&adapter->ptp_clk_mutex);
    spin_lock_irqsave(&global_ptpm_lock, flags);

    // normal mode.
    reg_val = zxdh_read_reg(base_addr, PTP_CONFIGURATION);
    reg_val &= ~(0x3 << PPS_RUN_MODE_BIT);
    reg_val |= (NORMAL_MODE << PPS_RUN_MODE_BIT);
    zxdh_write_reg(base_addr, PTP_CONFIGURATION, reg_val);
    zxdh_write_reg(base_addr, TIMER_LACTH_SEL, 1 << LATCH_1588_TIMER);
    zxdh_write_reg(base_addr, TIMER_LATCH_EN, 1);

    ns = zxdh_read_reg(base_addr, LATCH_TOD_NANO_SECOND);
    s  = zxdh_read_reg(base_addr, LATCH_LOWER_TOD_SECOND);
    s |= ((uint64_t)(zxdh_read_reg(base_addr, LATCH_HIGH_TOD_SECOND)) << 32);  

    spin_unlock_irqrestore(&global_ptpm_lock, flags);
    mutex_unlock(&adapter->ptp_clk_mutex);

    // *ts = ns_to_timespec64(ns);
    ts->tv_sec  = s;
    ts->tv_nsec = ns;
#ifdef CGS_V5_693
    PTP_LOG_DEBUG("kernel get clock time: %lld.%09ld\n", (long long)ts->tv_sec, ts->tv_nsec);
#else
    PTP_LOG_DEBUG("kernel get clock time: %lld.%09ld\n", ts->tv_sec, ts->tv_nsec);
#endif

    return 0;
}

static int zxdh_ptp_settime(struct ptp_clock_info *ptp, const struct timespec64 *ts)
{
    unsigned long flags;
    uint32_t reg_val;

    uint64_t base_addr;
    struct zxdh_ptp_private *adapter = NULL;

    PTP_COMM_CHECK_POINT_RET(ptp,PTP_PARA_CHK_POINT_NULL);
    PTP_COMM_CHECK_POINT_RET(ts,PTP_PARA_CHK_POINT_NULL);

    adapter = container_of(ptp, struct zxdh_ptp_private, ptp_caps[0]);
#ifdef CGS_V5_693
    PTP_LOG_DEBUG("name: %s, sec: %lld, nsec: %ld\n", ptp->name, (long long)ts->tv_sec, ts->tv_nsec);
#else
    PTP_LOG_DEBUG("name: %s, sec: %lld, nsec: %ld\n", ptp->name, ts->tv_sec, ts->tv_nsec);
#endif

    if(adapter == NULL)
    {
        PTP_LOG_ERR("zxdh_ptp_settime adapter null\n");
        return -1;
    }
    base_addr = adapter->ptpm_addr;

    mutex_lock(&adapter->ptp_clk_mutex);

    spin_lock_irqsave(&global_ptpm_lock, flags);

    /* adjust value */
    zxdh_write_reg(base_addr, ADJUST_HIGH_TOD_SECOND, (uint32_t)(ts->tv_sec >> 32) & 0xffff);
    zxdh_write_reg(base_addr, ADJUST_LOWER_TOD_SECOND, (uint32_t)(ts->tv_sec & 0xffffffff));
    zxdh_write_reg(base_addr, ADJUST_TOD_NANO_SECOND, ts->tv_nsec);

    /* 1588 timer, update mode */
    reg_val = zxdh_read_reg(base_addr, PTP_CONFIGURATION);
    reg_val &= ~(0x3 << 4);
    reg_val |= UPDATE_MODE << 4 | 1 << TIMER_1588_UPT_EN_BIT;
    zxdh_write_reg(base_addr, PTP_CONFIGURATION, reg_val);

    /* enable adjust */
    zxdh_write_reg(base_addr, TIMER_CONTROL, 1 << 1);

    spin_unlock_irqrestore(&global_ptpm_lock, flags);
    mutex_unlock(&adapter->ptp_clk_mutex);

    return 0;
}


static int zxdh_ptp_enable_pps(uint64_t base_addr, int on)
{
    uint32_t reg_val;
    uint64_t ptpm_base_addr;

    ptpm_base_addr = base_addr;

    // bit14 enable ptp pps output
    reg_val = zxdh_read_reg(ptpm_base_addr, PTP_CONFIGURATION);
    reg_val &= ~(1 << 14);
    reg_val |= on << 14;
    zxdh_write_reg(ptpm_base_addr, PTP_CONFIGURATION, reg_val);
    return 0;
}

/**
 * zxdh_ptp_enable
 * @ptp: the ptp clock structure
 * @rq: the requested feature to change
 * @on: whether to enable or disable the feature
 *
 */
static int zxdh_ptp_enable(struct ptp_clock_info *ptp, struct ptp_clock_request *rq, int on)
{
    int ret;
    //int pin;
    struct zxdh_ptp_private *adapter = NULL;

    PTP_COMM_CHECK_POINT_RET(ptp,PTP_PARA_CHK_POINT_NULL);
    PTP_COMM_CHECK_POINT_RET(rq,PTP_PARA_CHK_POINT_NULL);

    PTP_LOG_DEBUG("name: %s, rq->type: %d\n", ptp->name, rq->type);
    ret     = 0;
    //pin     = -1;
    adapter = container_of(ptp, struct zxdh_ptp_private, ptp_caps[0]);
    if(adapter == NULL) 
    {
        PTP_LOG_ERR("zxdh_ptp_enable adapter null\n");
        return -1;
    }
    switch (rq->type)
    {
        case PTP_CLK_REQ_PPS:
            ret = zxdh_ptp_enable_pps(adapter->ptpm_addr, on);
            return ret;
        case PTP_CLK_REQ_EXTTS:
            if (on)
            {
                // pin = ptp_find_pin(adapter->ptp_clock[0], PTP_PF_EXTTS,
                // 	   rq->extts.index);
                // if (pin < 0)
                //     return -EBUSY;
                if (rq->extts.index == 0 || rq->extts.index == 1)  // 选择pps0 or pps1做为capture和中断源
                {
                    adapter->pps_channel = rq->extts.index;
                    zxdh_write_reg(adapter->ptptop_addr, PP1S_EXTERNAL_SEL, rq->extts.index);
                }
            }
            return 0;

        case PTP_CLK_REQ_PEROUT:
            return 0;
        default:
            return -EOPNOTSUPP;
    }

    return 0;
}

static int zxdh_tsn_adjfine(struct ptp_clock_info *ptp, long scaled_ppm)
{
    unsigned long flags;
    int neg_adj = 0;
    uint64_t diff_nano;
    uint64_t adjust_nano = 0;
    uint64_t nominal_nano = 0;
    int timer_no;
    uint32_t integer_reg = 0;
    uint32_t fraction_reg = 0;
    uint64_t base_addr;
    s32 ppb;
    struct zxdh_ptp_private *adapter = NULL;

    PTP_COMM_CHECK_POINT_RET(ptp, PTP_PARA_CHK_POINT_NULL);

    timer_no = get_tsn_timer_no(ptp->name);
    PTP_CHECK_RANGE_WITH_RETURN(timer_no, TSN_TIMER_NAME_MIN_NO, TSN_TIMER_NAME_MAX_NO, -1);

    adapter = container_of(ptp, struct zxdh_ptp_private, ptp_caps[timer_no + 1]);
    if(adapter == NULL) 
    {
        PTP_LOG_ERR("zxdh_tsn_adjfine adapter null\n");
        return -1;
    }

    nominal_nano = ((uint64_t)(1) << 32) + 0;

    ppb = scaled_ppm_to_ppb(scaled_ppm);
    PTP_LOG_DEBUG("name: %s, ppb: %d\n", ptp->name, ppb);
    if (ppb < 0)
    {
        ppb     = -ppb;
        neg_adj = 1;
    }
    diff_nano = ((uint64_t)(ppb) << 32) / 1000000000;

    /* positive adjust */
    if (0 == neg_adj)
    {
        adjust_nano = nominal_nano + diff_nano;
    }
    else /* negative adjust */
    {
        adjust_nano = (nominal_nano > diff_nano) ? (nominal_nano - diff_nano) : 0;
    }
    PTP_LOG_DEBUG("adjust_nano: 0x%llx\n", adjust_nano);

    base_addr = adapter->ptpm_addr;

    integer_reg  = tsn_clock_cycle_integer_reg(timer_no);
    fraction_reg = tsn_clock_cycle_fraction_reg(timer_no);

    spin_lock_irqsave(&adapter->tmreg_lock, flags);
    zxdh_write_reg(base_addr, integer_reg, (uint32_t)(adjust_nano >> 32));
    zxdh_write_reg(base_addr, fraction_reg, (uint32_t)(adjust_nano & 0xffffffff));
    zxdh_write_reg(base_addr, CLOCK_CYCLE_UPDATE, 1 << (timer_no + 1));
    spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

    return 0;
}


static int zxdh_tsn_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
    unsigned long flags =  0;
    uint32_t run_mode;
    uint32_t reg_val;
    uint64_t adjust;
    uint64_t sec;
    uint32_t nsec;
    int timer_no;
    uint32_t nano_sec_reg;
    uint32_t low_sec_reg;
    uint32_t high_sec_reg;
    int run_mode_bit_shift;
    uint64_t base_addr;
    struct zxdh_ptp_private *adapter = NULL;

    PTP_COMM_CHECK_POINT_RET(ptp,PTP_PARA_CHK_POINT_NULL);
    PTP_LOG_DEBUG("name: %s, delta: %lld\n", ptp->name, delta);
    timer_no = get_tsn_timer_no(ptp->name);
    PTP_CHECK_RANGE_WITH_RETURN(timer_no, TSN_TIMER_NAME_MIN_NO, TSN_TIMER_NAME_MAX_NO, -1);

    adapter = container_of(ptp, struct zxdh_ptp_private, ptp_caps[timer_no + 1]);
    if(adapter == NULL) 
    {
        PTP_LOG_ERR("zxdh_tsn_adjtime adapter null\n");
        return -1;
    }
    base_addr = adapter->ptpm_addr;

    spin_lock_irqsave(&adapter->tmreg_lock, flags);

    /* 1588 timer, update mode */
    if (delta > 0)
    {
        run_mode = INCRE_MODE;
        adjust   = delta;
    }
    else
    {
        run_mode = DECRE_MODE;
        adjust   = -delta;
    }

    GET_TSN_ADJUST_NANO_REG(timer_no);
    GET_TSN_ADJUST_LOW_SEC_REG(timer_no);
    GET_TSN_ADJUST_HIGH_SEC_REG(timer_no);

    /* adjust value */
    sec = div_u64_rem(adjust, NSEC_PER_SEC, &nsec);

    PTP_LOG_DEBUG("sec: %llu, nsec: %u\n", sec, nsec);
    // nsec = rem;
    zxdh_write_reg(base_addr, high_sec_reg, (uint32_t)(sec >> 32));
    zxdh_write_reg(base_addr, low_sec_reg, (uint32_t)(sec & 0xffffffff));
    zxdh_write_reg(base_addr, nano_sec_reg, nsec);

    run_mode_bit_shift = 4 + timer_no * 2;
    reg_val            = zxdh_read_reg(base_addr, TSN_TIME_CONFIGURATION);
    reg_val &= ~(0x3 << run_mode_bit_shift);
    reg_val |= run_mode << run_mode_bit_shift;
    zxdh_write_reg(base_addr, TSN_TIME_CONFIGURATION, reg_val);
    /* enable adjust */
    zxdh_write_reg(base_addr, TSN_TIMER_CONTROL, 1 << timer_no);

    spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

    return 0;
}


static int zxdh_tsn_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
    uint32_t ns;
    uint64_t s;
    unsigned long flags;
    int timer_no;
    uint32_t reg_val;
    int run_mode_bit_shift;
    uint32_t nano_sec_reg;
    uint32_t low_sec_reg;
    uint32_t high_sec_reg;
    uint64_t base_addr;
    struct zxdh_ptp_private *adapter = NULL;

    PTP_COMM_CHECK_POINT_RET(ptp,PTP_PARA_CHK_POINT_NULL);
    PTP_COMM_CHECK_POINT_RET(ts,PTP_PARA_CHK_POINT_NULL);

    PTP_LOG_DEBUG("name: %s\n", ptp->name);
    timer_no = get_tsn_timer_no(ptp->name);
    PTP_CHECK_RANGE_WITH_RETURN(timer_no, TSN_TIMER_NAME_MIN_NO, TSN_TIMER_NAME_MAX_NO, -1);

    adapter = container_of(ptp, struct zxdh_ptp_private, ptp_caps[timer_no + 1]);
    if(adapter == NULL) 
    {
        PTP_LOG_ERR("zxdh_tsn_gettime adapter null\n");
        return -1;
    }
    base_addr = adapter->ptpm_addr;

    mutex_lock(&adapter->ptp_clk_mutex);

    spin_lock_irqsave(&adapter->tmreg_lock, flags);

    // bit11~bit4, configure normal mode, should make sure bit15~bit12 enable
    // first.
    run_mode_bit_shift = 4 + timer_no * 2;
    reg_val            = zxdh_read_reg(base_addr, TSN_TIME_CONFIGURATION);
    reg_val &= ~(0x3 << run_mode_bit_shift);
    reg_val |= NORMAL_MODE << run_mode_bit_shift;
    zxdh_write_reg(base_addr, TSN_TIME_CONFIGURATION, reg_val);

    // config latch one tsn timer
    reg_val = 0;
    reg_val = 1 << (timer_no + 2);
    zxdh_write_reg(base_addr, TIMER_LACTH_SEL, reg_val);

    // enable latch
    zxdh_write_reg(base_addr, TIMER_LATCH_EN, 1);

    GET_TSN_LATCH_NANO_REG(timer_no);
    GET_TSN_LATCH_LOW_SEC_REG(timer_no);
    GET_TSN_LATCH_HIGH_SEC_REG(timer_no);

    ns = zxdh_read_reg(base_addr, nano_sec_reg);
    s  = zxdh_read_reg(base_addr, low_sec_reg);
    s |= (uint64_t)zxdh_read_reg(base_addr, high_sec_reg) << 32;

    spin_unlock_irqrestore(&adapter->tmreg_lock, flags);
    mutex_unlock(&adapter->ptp_clk_mutex);

    // *ts = ns_to_timespec64(ns);
    ts->tv_sec  = s;
    ts->tv_nsec = ns;
#ifdef CGS_V5_693
    PTP_LOG_DEBUG("kernel get %s clock time: %lld.%09ld\n", ptp->name, (long long)ts->tv_sec, ts->tv_nsec);
#else
    PTP_LOG_DEBUG("kernel get %s clock time: %lld.%09ld\n", ptp->name, ts->tv_sec, ts->tv_nsec);
#endif

    return 0;
}

/**
 * zxdh_tsn_settime
 * @ptp: the ptp  clock struct
 * @ts: the timespec  containing the new time
 */
static int zxdh_tsn_settime(struct ptp_clock_info *ptp, const struct timespec64 *ts)
{
    unsigned long flags; //检查irq库函数 flag用法
    uint32_t reg_val;
    int timer_no;
    int run_mode_bit_shift;
    uint32_t nano_sec_reg;
    uint32_t low_sec_reg;
    uint32_t high_sec_reg;
    uint64_t base_addr;
    struct zxdh_ptp_private *adapter = NULL;

    PTP_COMM_CHECK_POINT_RET(ptp,PTP_PARA_CHK_POINT_NULL);
    PTP_COMM_CHECK_POINT_RET(ts,PTP_PARA_CHK_POINT_NULL);
#ifdef CGS_V5_693
    PTP_LOG_DEBUG("name: %s, sec: %lld, nsec: %ld\n", ptp->name, (long long)ts->tv_sec, ts->tv_nsec);
#else
    PTP_LOG_DEBUG("name: %s, sec: %lld, nsec: %ld\n", ptp->name, ts->tv_sec, ts->tv_nsec);
#endif
    timer_no = get_tsn_timer_no(ptp->name);
    PTP_CHECK_RANGE_WITH_RETURN(timer_no, TSN_TIMER_NAME_MIN_NO, TSN_TIMER_NAME_MAX_NO, -1);

    adapter = container_of(ptp, struct zxdh_ptp_private, ptp_caps[timer_no + 1]);
    if(adapter == NULL)
    {
        PTP_LOG_ERR("zxdh_tsn_settime adapter null\n");
        return -1;
    }
    base_addr = adapter->ptpm_addr;

    mutex_lock(&adapter->ptp_clk_mutex);

    spin_lock_irqsave(&adapter->tmreg_lock, flags);

    GET_TSN_ADJUST_NANO_REG(timer_no);
    GET_TSN_ADJUST_LOW_SEC_REG(timer_no);
    GET_TSN_ADJUST_HIGH_SEC_REG(timer_no);

    /* adjust value */
    zxdh_write_reg(base_addr, high_sec_reg, (uint32_t)(ts->tv_sec >> 32));
    zxdh_write_reg(base_addr, low_sec_reg, (uint32_t)(ts->tv_sec & 0xffffffff));
    zxdh_write_reg(base_addr, nano_sec_reg, ts->tv_nsec);

    run_mode_bit_shift = 4 + timer_no * 2;
    reg_val            = zxdh_read_reg(base_addr, TSN_TIME_CONFIGURATION);
    reg_val &= ~(0x3 << run_mode_bit_shift);
    reg_val |= UPDATE_MODE << run_mode_bit_shift;
    zxdh_write_reg(base_addr, TSN_TIME_CONFIGURATION, reg_val);
    /* enable adjust */
    zxdh_write_reg(base_addr, TSN_TIMER_CONTROL, 1 << timer_no);

    spin_unlock_irqrestore(&adapter->tmreg_lock, flags);
    mutex_unlock(&adapter->ptp_clk_mutex);

    return 0;
}

static int zxdh_tsn_enable_pps(uint64_t base_addr, int tsn_timer, int on)
{
    uint32_t reg_val = 0;

    uint64_t ptpm_base_addr;

    ptpm_base_addr = base_addr;

    // enable or disable tsn pps
    reg_val = zxdh_read_reg(ptpm_base_addr, TSN_TIME_CONFIGURATION);
    reg_val &= ~(1 << (16 + tsn_timer));
    reg_val |= on << (16 + tsn_timer);
    zxdh_write_reg(ptpm_base_addr, TSN_TIME_CONFIGURATION, reg_val);

    // select tsn pps output from test_1pps
    // reg_val = 0x4 + tsn_timer;
    // zxdh_write_reg(base_addr, TEST_PP1S_SEL, reg_val);
    // 这里先不关联配置test_pps输出，单独用ioctl配置test_pps输出
    return 0;
}

/**
 * zxdh_tsn_enable
 * @ptp: the ptp clock structure
 * @rq: the requested feature to change
 * @on: whether to enable or disable the feature
 *
 */
static int zxdh_tsn_enable(struct ptp_clock_info *ptp, struct ptp_clock_request *rq, int on)
{
    int timer_no;
    int ret = 0;
    //int pin = -1;
    struct zxdh_ptp_private *adapter = NULL;

    PTP_COMM_CHECK_POINT_RET(ptp,PTP_PARA_CHK_POINT_NULL);
    PTP_COMM_CHECK_POINT_RET(rq,PTP_PARA_CHK_POINT_NULL);
    timer_no = get_tsn_timer_no(ptp->name);
    PTP_CHECK_RANGE_WITH_RETURN(timer_no, TSN_TIMER_NAME_MIN_NO, TSN_TIMER_NAME_MAX_NO, -1);

    adapter = container_of(ptp, struct zxdh_ptp_private, ptp_caps[timer_no + 1]);
    if(adapter == NULL) 
    {
        PTP_LOG_ERR("zxdh_tsn_enable adapter null\n");
        return -1;
    }
    PTP_LOG_DEBUG("name: %s, timer_no: %u, rq->type: %d\n", ptp->name, timer_no, rq->type);

    switch (rq->type)
    {
        case PTP_CLK_REQ_PPS:
            ret = zxdh_tsn_enable_pps(adapter->ptpm_addr, timer_no, on);
            return ret;
        case PTP_CLK_REQ_EXTTS:
            if (on)
            {
                // pin = ptp_find_pin(adapter->ptp_clock[timer_no+1], PTP_PF_EXTTS,
                // 	   rq->extts.index);
                // if (pin < 0)
                //     return -EBUSY;
                if ((rq->extts.index == 0) || (rq->extts.index == 1) ) // 选择pps0 or pps1做为capture和中断源
                {
                    adapter->pps_channel = rq->extts.index;
                    zxdh_write_reg(adapter->ptptop_addr, PP1S_EXTERNAL_SEL, rq->extts.index);
                }
            }
            return 0;

        case PTP_CLK_REQ_PEROUT:
            return 0;
        default:
            return -EOPNOTSUPP;
    }


}

/* This function handle the pps interrupt event. */
irqreturn_t msix_extern_pps_irq_from_risc_handler(struct zxdh_pf_device *dev)
{
    uint32_t high_sec, low_sec, nsec;
    struct ptp_clock_event event;
    int i;
    struct zxdh_pf_device *zxdev = dev;
    struct zxdh_ptp_private *adapter = NULL;
    uint64_t base_addr = 0x0;

    __u32 pps_event;
    __u32 clear_event;
    __u32 pps_mask;

    uint32_t nano_sec_reg;
    uint32_t low_sec_reg;
    uint32_t high_sec_reg;
    // PTP_LOG_INFO("irq: %d\n", irq);
    if(dev == NULL)
    {
        return -1;
    }

    adapter = zxdev->ptp;
    if(adapter == NULL) 
    {
        PTP_LOG_ERR("msix_extern_pps_irq_from_risc_handler adapter\n");
        return -1;
    }
    if(!zxdh_pf_is_evb(zxdev))
    {
        return IRQ_HANDLED;
    }
    base_addr = adapter->ptpm_addr;

    pps_event = zxdh_read_reg(base_addr, INTERRUPT_EVENT); // 0x10
    pps_mask = zxdh_read_reg(base_addr, INTERRUPT_MASK);
    PTP_LOG_DEBUG("pps_event: 0x%x,pps_mask: 0x%x, capture_timer: %d\n",pps_event, pps_mask, zxdev->ptp->interrupt_capture_timer);

    // disable int
    for(i = 0; i < PTPM_INTERRUPT_BIT_NUM; i++)
    {
        if(pps_event & (1 << i))
            zxdh_write_reg(base_addr, INTERRUPT_MASK, pps_mask & (~(1 << i)));
    }

    // 清中断event
	clear_event = pps_mask & pps_event;   // 0x10
    zxdh_write_reg(base_addr, INTERRUPT_EVENT, clear_event);

    if(zxdev->ptp->interrupt_capture_timer > INTERRUPT_CAP_TIMER_MAX_NO)
    {
        PTP_LOG_ERR("capture_timer: %u out of range!\n", zxdev->ptp->interrupt_capture_timer);
        return -1;
    }
    // 用status不如用event准
    if (pps_event & (1 << EXTERNAL_PPS_BIT))  // 外部PPS信号产生的中断，Capture模式抓到的TOD
    {
        // 1588 timer
        if (zxdev->ptp->interrupt_capture_timer == 0)
        {
            nsec     = zxdh_read_reg(base_addr, PPS_LATCH_TOD_NANO_SECOND);
            low_sec  = zxdh_read_reg(base_addr, PPS_LATCH_LOWER_TOD_SECOND);
            high_sec = zxdh_read_reg(base_addr, PPS_LATCH_HIGH_TOD_SECOND);
        }
        else  // tsn timer
        {
            GET_PPS_LATCH_TSN_HIGH_SEC_REG(zxdev->ptp->interrupt_capture_timer);
            GET_PPS_LATCH_TSN_LOW_SEC_REG(zxdev->ptp->interrupt_capture_timer);
            GET_PPS_LATCH_TSN_NANO_REG(zxdev->ptp->interrupt_capture_timer);
            nsec     = zxdh_read_reg(base_addr, nano_sec_reg);
            low_sec  = zxdh_read_reg(base_addr, low_sec_reg);
            high_sec = zxdh_read_reg(base_addr, high_sec_reg);
        }
    }
    else if (pps_event & (1 << TRIGGER_IN_BIT))  // trigger in信号捕捉的TOD
    {
        nsec     = zxdh_read_reg(base_addr, TRIGGER_IN_TOD_NANO_SECOND);
        low_sec  = zxdh_read_reg(base_addr, TRIGGER_IN_LOWER_TOD_SECOND);
        high_sec = zxdh_read_reg(base_addr, TRIGGER_IN_HIGH_TOD_SECOND);
    }
    else
    {
        PTP_LOG_ERR("unknown pps irq\n");
        nsec     = 0;
        low_sec  = 0;
        high_sec = 0;
    }

    event.type      = PTP_CLOCK_EXTTS;
    event.index     = adapter->pps_channel;
    event.timestamp = (((uint64_t)high_sec << 32) | low_sec) * 1000000000ULL + nsec;
    PTP_LOG_DEBUG("nsec: %u, low_sec: %u, high_sec: %u\n", nsec,  low_sec, high_sec);
    PTP_LOG_DEBUG("capture_timer: %u, timestamp: %llu\n", zxdev->ptp->interrupt_capture_timer, event.timestamp);
    ptp_clock_event(zxdev->ptp->ptp_clock[zxdev->ptp->interrupt_capture_timer], &event);

    // enable int
    zxdh_write_reg(base_addr, INTERRUPT_MASK, pps_mask);

    return IRQ_HANDLED;
}
EXPORT_SYMBOL(msix_extern_pps_irq_from_risc_handler);

irqreturn_t msix_local_pps_irq_from_risc_handler(struct zxdh_pf_device *dev)
{
    struct ptp_clock_event event;
    __u32 reg_int;
    struct zxdh_pf_device *zxdev = dev;
    struct zxdh_ptp_private *adapter = NULL;
    uint64_t base_addr = 0x0;

    if(dev == NULL)
    {
        return -1;
    }

    adapter = zxdev->ptp;
    if(adapter == NULL) 
    {
        PTP_LOG_ERR("msix_local_pps_irq_from_risc_handler adapter\n");
        return -1;
    }
    if(!zxdh_pf_is_evb(zxdev))
    {
        return IRQ_HANDLED;
    }
    base_addr = adapter->ptptop_addr;

    // PTP_LOG_INFO("irq: %d\n", irq);
    event.type = PTP_CLOCK_PPS;

    /* adapter->ptp_clock[0] 中创建 pps, 用 pps 对应 local_pps 的中断 */
    if (!zxdev->ptp->ptp_clock[0])
        return IRQ_HANDLED;

    ptp_clock_event(zxdev->ptp->ptp_clock[0], &event);

    // base_addr = bar_addr + PTP_HOST_BAR_OFFSET;
    reg_int = zxdh_read_reg(base_addr, LOCAL_PPS_INTERRUPT); // 0x10
    PTP_LOG_DEBUG("reg_int: 0x%x\n", reg_int);
	reg_int |= 1 << 1;
	zxdh_write_reg(base_addr, LOCAL_PPS_INTERRUPT, reg_int);

#if 0  // only debug for local pps 
        uint64_t ns;
		uint32_t s;
		static uint64_t last_ns;
		static uint32_t last_s;

		reg_val = zxdh_read_reg(adapter, PTP_CONFIGURATION, PTP_M);
		reg_val &= ~(0x3 << 4);
		reg_val |= NORMAL_MODE << 4;
		zxdh_write_reg(adapter, PTP_CONFIGURATION, reg_val, PTP_M);

		// config latch 1588 timer
		zxdh_write_reg(adapter, TIMER_LACTH_SEL, 1 << LATCH_1588_TIMER, PTP_M);

		// enable latch
		zxdh_write_reg(adapter, TIMER_LATCH_EN, 1, PTP_M);


		ns = zxdh_read_reg(adapter, LATCH_TOD_NANO_SECOND, PTP_M);
		s = zxdh_read_reg(adapter, LATCH_LOWER_TOD_SECOND, PTP_M);
		// hwts->s |= (uint64_t)zxdh_read_reg(adapter, LATCH_HIGH_TOD_SECOND, PTP_M) << 32;

		// ns += s * 1000000000ULL;
		last_ns = ns;
		last_s = s;
        PTP_LOG_INFO("timestamp: cur:%llu.%09lu\n", low_sec*200, nsec);

#endif

    return IRQ_HANDLED;
}
EXPORT_SYMBOL(msix_local_pps_irq_from_risc_handler);

// for net_device driver get timestamp, param ptp get from get_tsn_clock
// function. hw timestamp only use 32 bit.
int get_pkt_timestamp(int32_t clock_no, struct zxdh_en_device *en_dev, struct time_stamps *ts, u32 *hwts)
{
    u32 nano_sec_reg;
    u32 low_sec_reg;
    u32 high_sec_reg;
    int timer_no;
    int run_mode_bit_shift;
    u32 reg_val;
    struct time_stamps temp_ts;
    unsigned long flags;
    uint64_t base_addr;
    struct zxdh_ptp_private *adapter = NULL;
    struct ptp_clock_info *ptp;
    int phcidx;
    //uint64_t latch_time = 0;
    //uint64_t read_time = 0;
    //uint64_t end_time = 0;
    if (NULL == en_dev || NULL == ts || NULL == hwts)
        return -1;

    adapter   = zxdh_ptp_get_ptp_private(en_dev);

    /* Started by AICoder, pid:z8d8d0b8d9ecc7614bcb09b9001e960b5135e639 */
    if(adapter == NULL) 
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "get_pkt_timestamp adapter vport 0x%x\n", en_dev->vport);
        return -1;
    }
    /* Ended by AICoder, pid:z8d8d0b8d9ecc7614bcb09b9001e960b5135e639 */
    for(phcidx = 0; phcidx < ZX_CLOCK_TIMER_NUM; phcidx++)
    {
#ifndef CGS_V5_693
        if(clock_no == ptp_clock_index(adapter->ptp_clock[phcidx]))
#else
        if(clock_no == zxdh_ptp_get_clock_idx(adapter, phcidx))
#endif
        {
            ptp = &adapter->ptp_caps[phcidx];
            break;
        }
    }
    if(phcidx == ZX_CLOCK_TIMER_NUM)
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "get phcindex fail\n");
        return -1;
    }
    base_addr = adapter->ptpm_addr;



    PTP_LOG_DEBUG_DEV(en_dev->parent, "ptp->name: %s\n", ptp->name);
    // first 80bit and second 80bit: both are 1588 timestamp
    if (0 == strcmp(ptp->name, "ptp0"))
    {
        PTP_LOG_DEBUG_DEV(en_dev->parent, "ptp0\n");
        //spin_lock_irqsave(&adapter->tmreg_lock, flags);
        spin_lock_irqsave(&global_ptpm_lock,flags);
        // latch_time = ktime_get_ns();
        // PTP_LOG_INFO("latch time %llu  pid %d\n",latch_time,current->pid);

        reg_val = zxdh_read_reg(base_addr, PTP_CONFIGURATION);
        reg_val &= ~(0x3 << PPS_RUN_MODE_BIT);
        reg_val |= NORMAL_MODE << PPS_RUN_MODE_BIT;
        zxdh_write_reg(base_addr, PTP_CONFIGURATION, reg_val);

        

        // config latch 1588 timer and hw timer
        zxdh_write_reg(base_addr, TIMER_LACTH_SEL, 1 << LATCH_1588_TIMER | 1 << LATCH_HW_TIMER);

        // enable latch
        zxdh_write_reg(base_addr, TIMER_LATCH_EN, 1);

        // read_time = ktime_get_ns();
        // PTP_LOG_INFO("read time %llu  pid %d\n",read_time,current->pid);

        temp_ts.ns = zxdh_read_reg(base_addr, LATCH_TOD_NANO_SECOND);
        temp_ts.s  = zxdh_read_reg(base_addr, LATCH_LOWER_TOD_SECOND);
        temp_ts.s |= (u64)zxdh_read_reg(base_addr, LATCH_HIGH_TOD_SECOND) << 32;

        ts->ns = temp_ts.ns;
        ts->s  = temp_ts.s;

        ts++;
        ts->ns = temp_ts.ns;
        ts->s  = temp_ts.s;

        *hwts = zxdh_read_reg(base_addr, LATCH_HARDWARE_TIME_LOW);
         //global_ptpm_lock确认其他函数中的使用方式，应该按照全局锁来操作
        //spin_unlock_irqrestore(&adapter->tmreg_lock, flags);
        spin_unlock_irqrestore(&global_ptpm_lock,flags);
        // end_time =  ktime_get_ns();
        // PTP_LOG_INFO("end_time  %llu pid %d\n",end_time,current->pid);
        // PTP_LOG_INFO("ts[]: 0x%x.%x, hwts: 0x%x\n", ts->s, ts->ns, *hwts);
    }
    else  // first 80bit is 1588 timestamp, second 80bit is one tsn timestamp
    {
        timer_no = get_tsn_timer_no(ptp->name);
        PTP_LOG_DEBUG_DEV(en_dev->parent, "tsn: %d\n", timer_no);
        PTP_CHECK_RANGE_WITH_RETURN(timer_no, TSN_TIMER_NAME_MIN_NO, TSN_TIMER_NAME_MAX_NO, -1);

        spin_lock_irqsave(&adapter->tmreg_lock, flags);

        reg_val = zxdh_read_reg(base_addr, PTP_CONFIGURATION);
        reg_val &= ~(0x3 << PPS_RUN_MODE_BIT);
        reg_val |= NORMAL_MODE << PPS_RUN_MODE_BIT;
        zxdh_write_reg(base_addr, PTP_CONFIGURATION, reg_val);

        // bit11~bit4, configure normal mode, should make sure bit15~bit12 enable
        // first.
        run_mode_bit_shift = 4 + timer_no * 2;
        reg_val            = zxdh_read_reg(base_addr, TSN_TIME_CONFIGURATION);
        reg_val &= ~(0x3 << run_mode_bit_shift);
        reg_val |= NORMAL_MODE << run_mode_bit_shift;
        zxdh_write_reg(base_addr, TSN_TIME_CONFIGURATION, reg_val);

        // config latch 1588 and one tsn timer and hw timer
        zxdh_write_reg(base_addr, TIMER_LACTH_SEL, (1 << LATCH_1588_TIMER) | (1 << LATCH_HW_TIMER) | (1 << (timer_no + 2)));

        // enable latch
        zxdh_write_reg(base_addr, TIMER_LATCH_EN, 1);

        // read 1588 timer
        ts->ns = zxdh_read_reg(base_addr, LATCH_TOD_NANO_SECOND);
        ts->s  = zxdh_read_reg(base_addr, LATCH_LOWER_TOD_SECOND);
        ts->s |= (u64)zxdh_read_reg(base_addr, LATCH_HIGH_TOD_SECOND) << 32;

        ts++;

        GET_TSN_LATCH_NANO_REG(timer_no);
        GET_TSN_LATCH_LOW_SEC_REG(timer_no);
        GET_TSN_LATCH_HIGH_SEC_REG(timer_no);
        // read one tsn timer
        ts->ns = zxdh_read_reg(base_addr, nano_sec_reg);
        ts->s  = zxdh_read_reg(base_addr, low_sec_reg);
        ts->s |= (u64)zxdh_read_reg(base_addr, high_sec_reg) << 32;

        *hwts = zxdh_read_reg(base_addr, LATCH_HARDWARE_TIME_LOW);

        spin_unlock_irqrestore(&adapter->tmreg_lock, flags);
        // PTP_LOG_INFO("ts[]: 0x%x.%x, hwts: 0x%x\n", ts->s, ts->ns, *hwts);
    }

    return 0;
}
EXPORT_SYMBOL(get_pkt_timestamp);

#define PTPS_NUMS 3
int enable_write_ts_to_fifo(struct zxdh_en_device *en_dev, u32 enable, u32 mac_number)
{
    uint64_t base_addr;
    struct zxdh_ptp_private *adapter = NULL;

    if (PTPS_NUMS <= mac_number)
    {
        PTP_LOG_ERR("mac number out of range\n");
        return -1;
    }

    if (NULL == en_dev)
        return -1;

    adapter   = zxdh_ptp_get_ptp_private(en_dev);
    if(adapter == NULL) 
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "enable_write_ts_to_fifo ptp adapter null\n");
        return -1;
    }
    base_addr = adapter->ptps_addr;


    PTP_LOG_DEBUG_DEV(en_dev->parent, "enable: %u, mac: %u\n", enable, mac_number);
    zxdh_write_reg(base_addr, PTPS_CONFIGURATION, enable);

    return 0;
}
EXPORT_SYMBOL(enable_write_ts_to_fifo);

int get_event_ts_info(struct zxdh_en_device *en_dev, struct ptp_buff *p_tsInfo, u32 mac_number)
{
    u32 count = 0;
    int i;
	uint64_t base_addr;
    // enum reg_module ptps_module;
    struct zxdh_ptp_private *adapter = NULL;

    if (PTPS_NUMS <= mac_number)
    {
        PTP_LOG_ERR("mac number out of range\n");
        return -1;
    }

    if (NULL == en_dev || NULL == p_tsInfo)
    {
        PTP_LOG_ERR("input pointer null\n");
        return -1;
    }
   
    adapter   = zxdh_ptp_get_ptp_private(en_dev);
    if(adapter == NULL) 
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "get_event_ts_info ptp adapter null\n");
        return -1;
    }
    base_addr = adapter->ptps_addr;


    // the maximum count is 64
    count = zxdh_read_reg(base_addr, PTP1588_EVENT_MESSAGE_FIFO_STATUS) & 0xff;
    // half is timestamp and half is match info(messageType, sourcePortIdentity,
    // sequenceId)
    if(count > PTP_ENCRYPTED_MESG_MAX_NUM)
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "encrypted ptp message out of range!\n");
        return -1;
    }
    count /= 2;
    PTP_LOG_DEBUG_DEV(en_dev->parent, "count: %d\n", count);

    for (i = 0; i < count; i++)
    {
        zxdh_write_reg(base_addr, PTPS_TIMER_CONTROL, 1);
        // read timestamp
        p_tsInfo->ptpRegInfo[i].cfVal[0] = zxdh_read_reg(base_addr, PTP1588_EVENT_MESSAGE_TS_LOW);
        p_tsInfo->ptpRegInfo[i].cfVal[1] = zxdh_read_reg(base_addr, PTP1588_EVENT_MESSAGE_TS_HIGH);

        PTP_LOG_DEBUG_DEV(en_dev->parent, "i: %d, low: 0x%x, high: 0x%x\n", i, p_tsInfo->ptpRegInfo[i].cfVal[0], p_tsInfo->ptpRegInfo[i].cfVal[1]);
        zxdh_write_reg(base_addr, PTPS_TIMER_CONTROL, 1);
        // read messageType, sourcePortIdentity, sequenceId
        p_tsInfo->ptpRegInfo[i].matchInfo = zxdh_read_reg(base_addr, PTP1588_EVENT_MESSAGE_TS_LOW);
        PTP_LOG_DEBUG_DEV(en_dev->parent, "i: %d, matchInfo: 0x%x\n", i, p_tsInfo->ptpRegInfo[i].matchInfo);
    }
    p_tsInfo->cfCount = count;
    PTP_LOG_DEBUG_DEV(en_dev->parent, "success\n");
    return 0;
}
EXPORT_SYMBOL(get_event_ts_info);

int32_t set_interrupt_capture_timer(struct zxdh_en_device *en_dev, uint32_t index)
{
    struct zxdh_ptp_private *adapter = NULL;
    if (NULL == en_dev)
        return -1;

    if(index > INTERRUPT_CAP_TIMER_MAX_NO)
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "capture_timer: %u out of range!\n", index);
        return -1;
    }
    adapter   = zxdh_ptp_get_ptp_private(en_dev);
    if(adapter == NULL) 
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "set_interrupt_capture_timer ptp adapter null\n");
        return -1;
    }
    adapter->interrupt_capture_timer = index;
    PTP_LOG_DEBUG_DEV(en_dev->parent, "index: %u\n",index);
    PTP_LOG_DEBUG_DEV(en_dev->parent, "pcie_id: 0x%x, vport: 0x%x, phy_port: %u\n", adapter->pdev->pcie_id, adapter->pdev->vport, adapter->pdev->phy_port);
    return 0;
}
EXPORT_SYMBOL(set_interrupt_capture_timer);

int32_t zxdh_set_pps_selection(struct zxdh_en_device *en_dev, uint32_t pps_type, uint32_t selection)
{
    struct zxdh_ptp_private *adapter = NULL;
    uint64_t base_addr;
    if (NULL == en_dev)
        return -1;

    if(pps_type > PP1S_EXTERNAL || selection > PP1S_TSN3)
        return -1;

    adapter   = zxdh_ptp_get_ptp_private(en_dev);
    if(adapter == NULL)
    {
       PTP_LOG_ERR_DEV(en_dev->parent, "zxdh_get_pd_value ptp adapter fail\n"); 
        return -1;
    }
    base_addr = adapter->ptptop_addr;


    PTP_LOG_DEBUG_DEV(en_dev->parent, "pps_type: %s, selection: %u\n", pps[pps_type], selection);

    switch (pps_type)
    {
        case PP1S_OUT:
            zxdh_write_reg(base_addr, PP1S_OUT_SEL, selection);
            break;
        case PP1S_TEST:
            zxdh_write_reg(base_addr, TEST_PP1S_SEL, selection);
            break;
        case PP1S_EXTERNAL:
            zxdh_write_reg(base_addr, PP1S_EXTERNAL_SEL, selection);
            // 这个用PTP_CLK_REQ_EXTTS来配置
            break;
        default:
            break;
    }

    return 0;
}
EXPORT_SYMBOL(zxdh_set_pps_selection);

int32_t zxdh_set_pd_detection(struct zxdh_en_device *en_dev, uint32_t pd_index, uint32_t pd_input1, uint32_t pd_input2)
{
    struct zxdh_ptp_private *adapter = NULL;
    uint64_t base_addr;
    uint32_t reg_val;
    if (NULL == en_dev)
        return -1;

    adapter   = zxdh_ptp_get_ptp_private(en_dev);
    if(adapter == NULL)
    {
       PTP_LOG_ERR_DEV(en_dev->parent, "zxdh_get_pd_value ptp adapter fail\n"); 
        return -1;
    }
    base_addr = adapter->ptptop_addr;

    PTP_LOG_DEBUG_DEV(en_dev->parent, "pd_index: %u, pd_input1: %u, pd_input2: %u\n", pd_index, pd_input1, pd_input2);

    // bit1~0: Pd_U1_Sel0   bit3~2: Pd_U1_Sel1.
    reg_val = ((pd_input1) | (pd_input2 << 3)); 

    if (PHASE_DETECTION1 == pd_index)
    {
        zxdh_write_reg(base_addr, PD_U1_SEL, reg_val);
    }
    else if(PHASE_DETECTION2 == pd_index)
    {
        zxdh_write_reg(base_addr, PD_U2_SEL, reg_val);
    }
    else
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "pd_index error\n");
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(zxdh_set_pd_detection);

int32_t zxdh_get_pd_value(struct zxdh_en_device *en_dev, uint32_t pd_index, uint32_t *pd_result)
{
    struct zxdh_ptp_private *adapter = NULL;
    uint64_t base_addr;
    if (NULL == en_dev)
        return -1;

    adapter   = zxdh_ptp_get_ptp_private(en_dev);
    if(adapter == NULL)
    {
       PTP_LOG_ERR_DEV(en_dev->parent, "zxdh_get_pd_value ptp adapter fail\n"); 
        return -1;
    }
    base_addr = adapter->ptptop_addr;

    if (PHASE_DETECTION1 == pd_index)
    {
        *pd_result = zxdh_read_reg(base_addr, PD_U1_RESULT);
    }
    else if(PHASE_DETECTION2 == pd_index)
    {
        *pd_result = zxdh_read_reg(base_addr, PD_U2_RESULT);
    }
    else
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "pd_index error\n");
        return -1;
    }
    PTP_LOG_DEBUG_DEV(en_dev->parent, "pd_index: %u, pd_result: 0x%x\n", pd_index, *pd_result);

    return 0;
}
EXPORT_SYMBOL(zxdh_get_pd_value);

int32_t zxdh_get_ptp_clock_index(struct zxdh_en_device *en_dev, uint32_t *ptp_clock_idx)
{
    struct zxdh_ptp_private *adapter = NULL;
    if (NULL == en_dev)
        return -1;
    PTP_COMM_CHECK_POINT_RET(ptp_clock_idx,PTP_PARA_CHK_POINT_NULL);
    PTP_COMM_CHECK_POINT_RET(en_dev->ops,PTP_PARA_CHK_POINT_NULL);

    if(en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        return 0;
    }
    adapter   = zxdh_ptp_get_ptp_private(en_dev);

    if(adapter == NULL)
    {
       PTP_LOG_ERR_DEV(en_dev->parent, "zxdh_get_ptp_clock_index ptp adapter fail\n"); 
        return -1;
    }

    PTP_COMM_CHECK_POINT_RET(adapter->ptp_clock[0],PTP_PARA_CHK_POINT_NULL);
#ifndef CGS_V5_693
    *ptp_clock_idx = ptp_clock_index(adapter->ptp_clock[0]);
#else
    *ptp_clock_idx = zxdh_ptp_get_clock_idx(adapter, 0);
#endif
    PTP_LOG_DEBUG_DEV(en_dev->parent, "first ptp_clock_idx: %u\n", *ptp_clock_idx );

    return 0;
}
EXPORT_SYMBOL(zxdh_get_ptp_clock_index);

int32_t zxdh_set_pps_interrupt_support(struct zxdh_en_device *en_dev, uint32_t support)
{
    struct zxdh_ptp_private *adapter = NULL;
    if (NULL == en_dev)
        return -1;

    adapter = zxdh_ptp_get_ptp_private(en_dev);
    if(adapter == NULL)
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "zxdh_set_pps_interrupt_support ptp adapter fail\n"); 
        return -1;
    }
    /* message from riscv, triggered by SIOCDEVPRIVATE_PPS_FUNC */
    adapter->pps_intr_support = support;
    PTP_LOG_DEBUG_DEV(en_dev->parent, "set pps interrupt support: %u\n", support);

    return 0;
}
EXPORT_SYMBOL(zxdh_set_pps_interrupt_support);

int32_t zxdh_get_pps_interrupt_support(struct zxdh_en_device *en_dev, uint32_t *support)
{
    struct zxdh_ptp_private *adapter = NULL;
    if (NULL == en_dev)
        return -1;
    PTP_COMM_CHECK_POINT_RET(support,PTP_PARA_CHK_POINT_NULL);
    adapter = zxdh_ptp_get_ptp_private(en_dev);

    if(adapter == NULL)
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "zxdh_get_pps_interrupt_support ptp adapter fail\n"); 
        return -1;
    }
    /* message from riscv, triggered by SIOCDEVPRIVATE_PPS_FUNC */
   
    *support = adapter->pps_intr_support;
    PTP_LOG_DEBUG_DEV(en_dev->parent, "get pps interrupt support: %u\n", *support);

    return 0;
}
EXPORT_SYMBOL(zxdh_get_pps_interrupt_support);

int32_t zxdh_set_local_pps_interrupt_enable(struct zxdh_en_device *en_dev, uint32_t enable)
{
    struct zxdh_ptp_private *adapter = NULL;
    uint64_t base_addr;
    uint32_t reg_val;
    if (NULL == en_dev)
        return -1;

    if(enable != ENABLE && enable != DISABLE)
        return -1;
    adapter   = zxdh_ptp_get_ptp_private(en_dev);

   
    if(adapter == NULL)
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "zxdh_set_local_pps_interrupt_enable ptp adapter fail\n"); 
        return -1;
    }
    base_addr = adapter->ptptop_addr;

    reg_val = zxdh_read_reg(base_addr, LOCAL_PPS_INTERRUPT);
    reg_val &= ~0x1;
    reg_val |= enable;
    zxdh_write_reg(base_addr, LOCAL_PPS_INTERRUPT, reg_val);

    PTP_LOG_DEBUG_DEV(en_dev->parent, "set local pps interrupt enable: %u\n", enable);

    return 0;
}
EXPORT_SYMBOL(zxdh_set_local_pps_interrupt_enable);

int32_t zxdh_set_ext_pps_interrupt_enable(struct zxdh_en_device *en_dev, uint32_t pps_src, uint32_t enable)
{
    struct zxdh_ptp_private *adapter =  NULL;
    uint64_t base_addr;
    uint32_t reg_val;
    if (NULL == en_dev)
        return -1;

    if(enable != ENABLE && enable != DISABLE)
        return -1;
    /*
    pps_src: 
        0: trigger in
        1: trigger out
        2: FIFO no empty
        3: FIFO almost full
        4: pps in
    */
    if(4 < pps_src)
        return -1;

    adapter   = zxdh_ptp_get_ptp_private(en_dev);

    if(adapter == NULL)
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "zxdh_set_ext_pps_interrupt_enable ptp adapter fail\n"); 
        return -1;
    }
    base_addr = adapter->ptpm_addr;

    reg_val = zxdh_read_reg(base_addr, INTERRUPT_MASK);
    //PTP_LOG_INFO("reg_val: 0x%08x\n", reg_val);
    reg_val &= ~(1 << pps_src);
    //PTP_LOG_INFO("reg_val: 0x%08x\n", reg_val);
    reg_val |= enable << pps_src;
    //PTP_LOG_INFO("reg_val: 0x%08x\n", reg_val);
    zxdh_write_reg(base_addr, INTERRUPT_MASK, reg_val);

    if(enable == ENABLE)
    {
        reg_val = zxdh_read_reg(base_addr, PTP_CONFIGURATION);
        //PTP_LOG_INFO("reg_val: 0x%08x\n", reg_val);
        reg_val |= (1 << PPS_INPUT_SEL_BIT);
        //PTP_LOG_INFO("reg_val: 0x%08x\n", reg_val);
        zxdh_write_reg(base_addr, PTP_CONFIGURATION, reg_val);
    }

    PTP_LOG_DEBUG_DEV(en_dev->parent, "set ext pps interrupt pps_src: %u, enable: %u\n", pps_src, enable);

    return 0;
}
EXPORT_SYMBOL(zxdh_set_ext_pps_interrupt_enable);

int32_t zxdh_set_pd_sel_shift(struct zxdh_en_device *en_dev, uint32_t pd_index, uint32_t sel, uint32_t shift)
{
    struct zxdh_ptp_private *adapter = NULL;
    uint64_t base_addr;
    uint32_t reg_addr;

    if (NULL == en_dev)
        return -1;

    if(PHASE_DETECTION1 != pd_index && PHASE_DETECTION2 != pd_index)
        return -1;

    if(PD_SEL_1 != sel && PD_SEL_2 != sel)
        return -1;

    adapter   = zxdh_ptp_get_ptp_private(en_dev);
    base_addr = adapter->ptptop_addr;

    if(PHASE_DETECTION1 == pd_index)
    {
        reg_addr = (PD_SEL_1 == sel) ? PD_U1_PD0_SHIFT : PD_U1_PD1_SHIFT;
    }
    else
    {
        reg_addr = (PD_SEL_1 == sel) ? PD_U2_PD0_SHIFT : PD_U2_PD1_SHIFT;
    }

    zxdh_write_reg(base_addr, reg_addr, shift);

    PTP_LOG_DEBUG_DEV(en_dev->parent, "set ext pd%u sel%u shift: %u\n", pd_index, sel, shift);

    return 0;
}
EXPORT_SYMBOL(zxdh_set_pd_sel_shift);

int get_hw_timestamp(struct zxdh_en_device *en_dev, u32 *hwts)
{
    u32 reg_val;
    unsigned long flags;
    uint64_t base_addr;
    struct zxdh_ptp_private *adapter = NULL;


    if (NULL == en_dev || NULL == hwts)
        return -1;

    adapter   = zxdh_ptp_get_ptp_private(en_dev);

    if(adapter == NULL)
    {
        PTP_LOG_ERR_DEV(en_dev->parent, "get_hw_timestamp ptp adapter fail\n"); 
        return -1;
    }
    base_addr = adapter->ptpm_addr;

    spin_lock_irqsave(&global_ptpm_lock, flags);


    reg_val = zxdh_read_reg(base_addr, PTP_CONFIGURATION);
    reg_val &= ~(0x3 << PPS_RUN_MODE_BIT);
    reg_val |= (NORMAL_MODE << PPS_RUN_MODE_BIT);
    zxdh_write_reg(base_addr, PTP_CONFIGURATION, reg_val);

    // config latch  hw timer
    zxdh_write_reg(base_addr, TIMER_LACTH_SEL, 1 << LATCH_HW_TIMER);

    // enable latch
    zxdh_write_reg(base_addr, TIMER_LATCH_EN, 1);

    *hwts = zxdh_read_reg(base_addr, LATCH_HARDWARE_TIME_LOW);


    spin_unlock_irqrestore(&global_ptpm_lock, flags);
    // PTP_LOG_INFO("ts[]: 0x%x.%x, hwts: 0x%x\n", ts->s, ts->ns, *hwts);

    return 0;
}
EXPORT_SYMBOL(get_hw_timestamp);


int zxdh_ptp_init(struct dh_core_dev *zxdev)
{
    struct zxdh_ptp_private *zxp;
    int err = -ENOMEM;
    int i;

    uint32_t reg;
    int size;
    uint16_t ep_no;
    uint64_t pci_addr;
    uint64_t ptptop_paddr;
    uint64_t ptpm_paddr;
    uint64_t ptps_paddr;
    struct zxdh_pf_device *pf_dev = NULL;
    
    PTP_COMM_CHECK_POINT_RET(zxdev,PTP_PARA_CHK_POINT_NULL);

    pf_dev = dh_core_priv(zxdev);
    PTP_COMM_CHECK_POINT_RET(pf_dev,PTP_PARA_CHK_POINT_NULL);

    PTP_LOG_DEBUG_DEV(zxdev, "enter\n");

    zxp = kzalloc(sizeof(*zxp), GFP_KERNEL);
    if (!zxp)
    {
        PTP_LOG_ERR_DEV(zxdev, "zxp kzalloc failed\n");
        goto no_memory;
    }

    err = -ENODEV;

    zxp->ptp_caps[0].owner = THIS_MODULE;
    zte_strncpy_s(zxp->ptp_caps[0].name, "ptp0", sizeof(zxp->ptp_caps[0].name)-1);

    zxp->ptp_caps[0].max_adj           = 999999999;
    zxp->ptp_caps[0].n_alarm           = 0;
    zxp->ptp_caps[0].n_ext_ts          = 2;  // ptp_chardev.c: ptp_ioctl: if (req.extts.index >= ops->n_ext_ts) 需要改初值
    zxp->ptp_caps[0].n_per_out         = 0;
    zxp->ptp_caps[0].n_pins            = 0;  //
    if(zxdh_pf_is_evb(pf_dev))
    {
        zxp->ptp_caps[0].pps           = 1;
    }
    else
    {
        zxp->ptp_caps[0].pps           = 0;
    }
    zxp->ptp_caps[0].adjfine           = zx_ptp_adjfine;
    zxp->ptp_caps[0].adjtime           = zxdh_ptp_adjtime;
    zxp->ptp_caps[0].gettime64         = zxdh_ptp_gettime;
    zxp->ptp_caps[0].settime64         = zxdh_ptp_settime;
    zxp->ptp_caps[0].enable            = zxdh_ptp_enable;

    zxp->ptp_clock[0] = ptp_clock_register(&zxp->ptp_caps[0], &zxdev->pdev->dev);
    if (IS_ERR(zxp->ptp_clock[0]))
    {
        zxp->ptp_clock[0] = NULL;
        PTP_LOG_ERR_DEV(zxdev, "ptp_clock_register ptp0 failed\n");
        goto no_ptp_clock;
    }

    for (i = 0; i < ZX_TSN_TIMER_NUM; i++)
    {
        zxp->ptp_caps[i+1].owner = THIS_MODULE;
        size = snprintf(zxp->ptp_caps[i + 1].name, sizeof(zxp->ptp_caps[i+1].name), "tsn%d", i);
        if(size >= sizeof(zxp->ptp_caps[i+1].name))
    		zxp->ptp_caps[i + 1].name[sizeof(zxp->ptp_caps[i+1].name)-1] = '\0';

        zxp->ptp_caps[i + 1].max_adj   = 999999999;
        zxp->ptp_caps[i + 1].n_alarm   = 0;
        zxp->ptp_caps[i + 1].n_ext_ts  = 2;
        zxp->ptp_caps[i + 1].n_per_out = 0;
        zxp->ptp_caps[i + 1].n_pins    = 0;
        zxp->ptp_caps[i + 1].pps = 0;
        zxp->ptp_caps[i + 1].adjfine           = zxdh_tsn_adjfine;
        zxp->ptp_caps[i + 1].adjtime           = zxdh_tsn_adjtime;
        zxp->ptp_caps[i + 1].gettime64         = zxdh_tsn_gettime;
        zxp->ptp_caps[i + 1].settime64         = zxdh_tsn_settime;
        zxp->ptp_caps[i + 1].enable            = zxdh_tsn_enable;
        zxp->ptp_clock[i + 1] = ptp_clock_register(&zxp->ptp_caps[i + 1], &zxdev->pdev->dev);
        if (IS_ERR(zxp->ptp_clock[i + 1]))
        {
            zxp->ptp_clock[i + 1] = NULL;
            PTP_LOG_ERR_DEV(zxdev, "ptp_clock_register tsn%d failed\n", i);
            goto no_tsn_clock;
        }
    }
    /* not support pps interrupt by default */
    zxp->pps_intr_support = 0;

    spin_lock_init(&zxp->tmreg_lock);
    mutex_init(&zxp->ptp_clk_mutex);
    if(ptpm_lock_init_stat == 0)
    {
        PTP_LOG_DEBUG_DEV(zxdev, "global_ptpm_lock init\n");
        spin_lock_init(&global_ptpm_lock);
        ptpm_lock_init_stat = 1;
    }



    ep_no = EPID(pf_dev->vport); 
    pci_addr = pci_resource_start(zxdev->pdev, 0);
    PTP_LOG_DEBUG_DEV(zxdev, "ep_no: %u, pci_addr: 0x%llx\n", ep_no, pci_addr);

    if(ep_no == EPID_4)
    {
        ptptop_paddr = pci_addr + PTPTOP_ZF_BAR_OFFSET;
        ptpm_paddr   = pci_addr + PTPM_ZF_BAR_OFFSET;
        ptps_paddr   = pci_addr + PTPS_ZF_BAR_OFFSET;
    }
    else
    {
        ptptop_paddr = pci_addr + PTPTOP_HOST_BAR_OFFSET;
        ptpm_paddr   = pci_addr + PTPM_HOST_BAR_OFFSET;
        ptps_paddr   = pci_addr + PTPS_HOST_BAR_OFFSET;
    }

    zxp->ptptop_addr = (uint64_t)ioremap(ptptop_paddr, PTPTOP_REGS_LEN);
    if(zxp->ptptop_addr == 0)
    {
        PTP_LOG_ERR_DEV(zxdev, "ptptop ioremap failed\n");
        goto ptptop_ioremap_fail;
    }

    zxp->ptpm_addr = (uint64_t)ioremap(ptpm_paddr, PTPM_REGS_LEN);
    if(zxp->ptpm_addr == 0)
    {
        PTP_LOG_ERR_DEV(zxdev, "ptpm ioremap failed\n");
        goto ptpm_ioremap_fail;
    }

    zxp->ptps_addr = (uint64_t)ioremap(ptps_paddr, PTPS_REGS_LEN);
    if(zxp->ptps_addr == 0)
    {
        PTP_LOG_ERR_DEV(zxdev, "ptps ioremap failed\n");
        goto ptps_ioremap_fail;
    }

    pf_dev->ptp = zxp;
    zxp->pdev = pf_dev;

    reg = zxdh_read_reg(zxp->ptpm_addr, PTP_CONFIGURATION);
    reg |= (1 << 15); 
    zxdh_write_reg(zxp->ptpm_addr, PTP_CONFIGURATION, reg);
    // enable four tsn timer and tsn pps enable
    zxdh_write_reg(zxp->ptpm_addr, TSN_TIME_CONFIGURATION, 0xff000);

    // timesync delay
    zxdh_write_reg(zxp->ptptop_addr, TSN_GROUP_NANO_SEC_DELAY0, 0x1);
    zxdh_write_reg(zxp->ptptop_addr, TSN_GROUP_NANO_SEC_DELAY1, 0x1);
    zxdh_write_reg(zxp->ptptop_addr, TSN_GROUP_NANO_SEC_DELAY2, 0x1);
    zxdh_write_reg(zxp->ptptop_addr, TSN_GROUP_NANO_SEC_DELAY3, 0x1);
    zxdh_write_reg(zxp->ptptop_addr, PTP1588_NP_NANO_SEC_DELAY, 0x1);
    zxdh_write_reg(zxp->ptptop_addr, PTP1588_NVME_NANO_SEC_DELAY1, 0x13);
    zxdh_write_reg(zxp->ptptop_addr, PTP1588_NVME_NANO_SEC_DELAY2, 0x13);
    zxdh_write_reg(zxp->ptptop_addr, PTP1588_RDMA_NANO_SEC_DELAY, 0xC);

    return 0;


ptps_ioremap_fail:
    iounmap((void *)zxp->ptpm_addr);
ptpm_ioremap_fail:
    iounmap((void *)zxp->ptptop_addr);
ptptop_ioremap_fail:
no_tsn_clock:
    for (i = 0; i < ZX_CLOCK_TIMER_NUM; i++)
    {
        if (zxp->ptp_clock[i])
            ptp_clock_unregister(zxp->ptp_clock[i]);
    }
no_ptp_clock:
    kfree(zxp);
no_memory:
    return err;
}
EXPORT_SYMBOL(zxdh_ptp_init);

void zxdh_ptp_stop(struct dh_core_dev *zxdev)
{
    int i;

    struct zxdh_pf_device *pf_dev;
    struct zxdh_ptp_private *zxp = NULL;
    PTP_COMM_CHECK_POINT(zxdev);

    pf_dev = dh_core_priv(zxdev);
    PTP_COMM_CHECK_POINT(pf_dev);

    zxp = pf_dev->ptp;

    if (NULL == zxp)
        return;

    iounmap((void *)zxp->ptptop_addr);
    iounmap((void *)zxp->ptpm_addr);
    iounmap((void *)zxp->ptps_addr);

    for (i = 0; i < ZX_CLOCK_TIMER_NUM; i++)
    {
        if (zxp->ptp_clock[i])
            ptp_clock_unregister(zxp->ptp_clock[i]);
    }

    kfree(zxp);
}
EXPORT_SYMBOL(zxdh_ptp_stop);
