#ifndef _ZX_PTP_H
#define _ZX_PTP_H

#include <linux/ptp_clock_kernel.h>
#include "../en_pf.h"

#define PTP_PARA_CHK_POINT_NULL (-1)

#define ZX_CLOCK_TIMER_NUM   5 // 1st: ptp, other: tsn
#define ZX_TSN_TIMER_NUM     (ZX_CLOCK_TIMER_NUM -1)

#define PTP_REG_INFO_NUM     32
#define PTP_ENCRYPTED_MESG_MAX_NUM   64

#define PTP_DRIVER_UNINIT    0
#define PTP_DRIVER_INITED    1

#define X86_ADDR_2_ARRCH64(X86_ADDR) (((X86_ADDR & (~0xFFFF)) << 4) | (X86_ADDR & 0xFFFF))

#define PTPTOP_HOST_BAR_OFFSET  0xc000
#define PTPM_HOST_BAR_OFFSET    0x10000
#define PTPS_HOST_BAR_OFFSET    0x34000

#define PTPTOP_ZF_BAR_OFFSET    PTPTOP_HOST_BAR_OFFSET
#define PTPM_ZF_BAR_OFFSET      X86_ADDR_2_ARRCH64(PTPM_HOST_BAR_OFFSET)
#define PTPS_ZF_BAR_OFFSET      X86_ADDR_2_ARRCH64(PTPS_HOST_BAR_OFFSET)

#define PTPTOP_REGS_LEN         0x4000
#define PTPM_REGS_LEN           0x4000
#define PTPS_REGS_LEN           0x1000

#define EPID(VPORT)                 ((VPORT & 0x7000) >> 12)
#define EPID_4                      (4)

#define PHASE_DETECTION1     1
#define PHASE_DETECTION2     2

typedef enum
{
    PD_SEL_INVALID, // no use
    PD_SEL_1,
    PD_SEL_2
}PD_SEL;
#define PTPM_INTERRUPT_BIT_NUM    5

#ifndef ENABLE
#define ENABLE        1
#endif

#ifndef DISABLE
#define DISABLE       0
#endif


#define TSN_TIMER_NAME_MIN_NO     0
#define TSN_TIMER_NAME_MAX_NO     3

#define INTERRUPT_CAP_TIMER_MIN_NO    0
#define INTERRUPT_CAP_TIMER_MAX_NO    4
#define PTP_CHECK_RANGE_WITH_RETURN(val, min, max, ret) \
    do { \
        if(!(min <= val && val <= max)) \
            return ret; \
    }while(0)

#define PTP_COMM_CHECK_POINT_RET(point,ret) \
    do{\
       if(NULL == point) \
       {\
          PTP_LOG_ERR("\n %s:%d[Error:POINT NULL] ! FUNCTION : %s!\n",__FILE__,__LINE__,__FUNCTION__);\
          return ret;\
       }\
    }while(0)

#define PTP_COMM_CHECK_POINT(point) \
    do{\
       if(NULL == point) \
       {\
          PTP_LOG_ERR("\n %s:%d[Error:POINT NULL] ! FUNCTION : %s!\n",__FILE__,__LINE__,__FUNCTION__);\
          return;\
       }\
    }while(0)

struct time_stamps
{
	u64 s;
	u32 ns;
};

struct pkt_hw_ts
{
	struct time_stamps ts[2];
};

enum
{
    PP1S_OUT,
    PP1S_TEST,
    PP1S_EXTERNAL, // maybe use PTP_CLK_REQ_EXTTS
};

typedef enum
{
    // PPS_OUT / TEST_PP1S / EXTERNAL_PP1S selection:
    PP1S_REF0,
    PP1S_REF1,
    // PPS_OUT / TEST_PP1S  selection:
    PP1S_LOCAL,
    PP1S_1588,
    // TEST PP1S selection only:
    PP1S_TSN0,
    PP1S_TSN1,
    PP1S_TSN2,
    PP1S_TSN3
}PPS_SELECT;

typedef enum
{
    LOCAL_PPS,
    EXTERNAL_PPS
}PPS_INTERRUPT_TYPE;

struct zxdh_ptp_private {
    // void __iomem *ptpm_regs;
    // void __iomem *ptp_top_regs;
    // void __iomem *ptps0_regs;
    // void __iomem *ptps1_regs;
    // void __iomem *ptps2_regs;

    struct mutex ptp_clk_mutex;
    struct zxdh_pf_device *pdev;
    struct ptp_clock *ptp_clock[ZX_CLOCK_TIMER_NUM];
	struct ptp_clock_info ptp_caps[ZX_CLOCK_TIMER_NUM];
#ifdef CGS_V5_693
    int ptp_clock_idx[ZX_CLOCK_TIMER_NUM];  /* 记录每个时钟的 index */
#endif
    unsigned int pps_channel;  // externel pps0/pps1
    unsigned int interrupt_capture_timer;
    unsigned int pps_intr_support;
    uint64_t ptptop_addr;
    uint64_t ptpm_addr;
    uint64_t ptps_addr;

    spinlock_t tmreg_lock;

};

struct ptp_reg_info
{
    uint32_t cfVal[2];
    uint32_t matchInfo;
};

struct ptp_buff
{
    uint32_t cfCount;
    struct ptp_reg_info ptpRegInfo[PTP_REG_INFO_NUM];
};

int zxdh_ptp_init(struct dh_core_dev *zxdev);
void zxdh_ptp_stop(struct dh_core_dev *zxdev);
irqreturn_t msix_extern_pps_irq_from_risc_handler(struct zxdh_pf_device *dev);
irqreturn_t msix_local_pps_irq_from_risc_handler(struct zxdh_pf_device *dev);

#endif /* _ZX_PTP_H */
