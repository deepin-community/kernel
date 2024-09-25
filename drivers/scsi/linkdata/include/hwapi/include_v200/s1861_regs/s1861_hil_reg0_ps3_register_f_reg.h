#ifndef __S1861_HIL_REG0_PS3_REGISTER_F_REG_H__ 
#define __S1861_HIL_REG0_PS3_REGISTER_F_REG_H__ 
#include "s1861_global_baseaddr.h"
#ifndef __S1861_HIL_REG0_PS3_REGISTER_F_REG_MACRO__
#define HIL_REG0_PS3_REGISTER_F_PS3_DOORBELL_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x40)
#define HIL_REG0_PS3_REGISTER_F_PS3_DOORBELL_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DOORBELL_IRQ_CLEAR_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x48)
#define HIL_REG0_PS3_REGISTER_F_PS3_DOORBELL_IRQ_CLEAR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DOORBELL_IRQ_MASK_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x50)
#define HIL_REG0_PS3_REGISTER_F_PS3_DOORBELL_IRQ_MASK_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_IRQ_CONTROL_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x58)
#define HIL_REG0_PS3_REGISTER_F_PS3_IRQ_CONTROL_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_KEY_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x100)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_KEY_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_STATE_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x108)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_STATE_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x110)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_IRQ_CLEAR_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x118)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_IRQ_CLEAR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_IRQ_MASK_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x120)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_IRQ_MASK_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_KEY_SHIFT_REG_LOW_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x128)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_KEY_SHIFT_REG_LOW_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_KEY_SHIFT_REG_HIGH_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x130)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_KEY_SHIFT_REG_HIGH_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_TIME_CNT_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x138)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_TIME_CNT_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_TIME_OUT_EN_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x140)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_TIME_OUT_EN_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_KEY_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x200)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_KEY_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_STATE_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x208)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_STATE_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x210)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_KEY_SHIFT_REG_LOW_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x218)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_KEY_SHIFT_REG_LOW_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_KEY_SHIFT_REG_HIGH_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x220)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_KEY_SHIFT_REG_HIGH_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_TIME_CNT_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x228)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_TIME_CNT_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_TIME_OUT_EN_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x230)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_TIME_OUT_EN_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_KEY_GAP_CFG_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x238)
#define HIL_REG0_PS3_REGISTER_F_PS3_KEY_GAP_CFG_RST   (0x0000000002FAF080)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_IRQ_CLEAR_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x240)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_IRQ_CLEAR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_IRQ_MASK_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x248)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_IRQ_MASK_RST   (0x0000000000000001)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOC_FW_STATE_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x300)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOC_FW_STATE_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_FW_CMD_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x308)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_FW_CMD_RST   (0x0000000000001FFF)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_CHAIN_SIZE_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x310)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_CHAIN_SIZE_RST   (0x0000000000000FFF)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_VD_INFO_SIZE_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x318)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_VD_INFO_SIZE_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_NVME_PAGE_SIZE_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x320)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_NVME_PAGE_SIZE_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_FEATURE_SUPPORT_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x328)
#define HIL_REG0_PS3_REGISTER_F_PS3_FEATURE_SUPPORT_RST   (0x0000000000000007)
#define HIL_REG0_PS3_REGISTER_F_PS3_FIRMWARE_VERSION_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x330)
#define HIL_REG0_PS3_REGISTER_F_PS3_FIRMWARE_VERSION_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_REPLYQUE_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x338)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_REPLYQUE_RST   (0x000000000000007F)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDWARE_VERSION_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x340)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDWARE_VERSION_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_MGR_QUEUE_DEPTH_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x348)
#define HIL_REG0_PS3_REGISTER_F_PS3_MGR_QUEUE_DEPTH_RST   (0x0000000000000400)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_QUEUE_DEPTH_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x350)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_QUEUE_DEPTH_RST   (0x0000000000001000)
#define HIL_REG0_PS3_REGISTER_F_PS3_TFIFO_DEPTH_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x358)
#define HIL_REG0_PS3_REGISTER_F_PS3_TFIFO_DEPTH_RST   (0x0000000000000400)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_SEC_R1X_CMDS_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x360)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_SEC_R1X_CMDS_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT0_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x400)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT0_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT1_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x408)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT1_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT2_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x410)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT2_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT3_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x418)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT3_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT_ALL_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x420)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT_ALL_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_IRQ_STATUS_RPT_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x440)
#define HIL_REG0_PS3_REGISTER_F_PS3_IRQ_STATUS_RPT_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_CTRL_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x500)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_CTRL_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_CTRL_IRQ_CLEAR_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x508)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_CTRL_IRQ_CLEAR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_CTRL_IRQ_MASK_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x510)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_CTRL_IRQ_MASK_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_STATUS_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x518)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_STATUS_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_DATA_SIZE_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x520)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_DATA_SIZE_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_TRIGGER_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x600)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_TRIGGER_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_TRIGGER_IRQ_CLEAR_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x608)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_TRIGGER_IRQ_CLEAR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_TRIGGER_IRQ_MASK_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x610)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_TRIGGER_IRQ_MASK_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_COUNTER_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x618)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_COUNTER_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_REG_CMD_STATE_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x620)
#define HIL_REG0_PS3_REGISTER_F_PS3_REG_CMD_STATE_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG0_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x628)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG0_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG0_IRQ_CLEAR_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x630)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG0_IRQ_CLEAR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG0_IRQ_MASK_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x638)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG0_IRQ_MASK_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG1_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x640)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG1_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG1_IRQ_CLEAR_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x648)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG1_IRQ_CLEAR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG1_IRQ_MASK_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x650)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG1_IRQ_MASK_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG2_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x658)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG2_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG2_IRQ_CLEAR_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x660)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG2_IRQ_CLEAR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG2_IRQ_MASK_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x668)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG2_IRQ_MASK_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG3_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x670)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG3_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG3_IRQ_CLEAR_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x678)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG3_IRQ_CLEAR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG3_IRQ_MASK_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x680)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG3_IRQ_MASK_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG4_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x688)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG4_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG4_IRQ_CLEAR_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x690)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG4_IRQ_CLEAR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG4_IRQ_MASK_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x698)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG4_IRQ_MASK_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG5_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6a0)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG5_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG6_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6a8)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG6_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG7_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6b0)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG7_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG8_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6b8)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG8_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG9_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6c0)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG9_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG10_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6c8)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG10_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG11_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6d0)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG11_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG12_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6d8)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG12_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SESSIONCMD_ADDR_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x700)
#define HIL_REG0_PS3_REGISTER_F_PS3_SESSIONCMD_ADDR_RST   (0xFFFFFFFFFFFFFFFF)
#define HIL_REG0_PS3_REGISTER_F_PS3_SESSIONCMD_ADDR_IRQ_CLEAR_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x708)
#define HIL_REG0_PS3_REGISTER_F_PS3_SESSIONCMD_ADDR_IRQ_CLEAR_RST   (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SESSIONCMD_ADDR_IRQ_MASK_ADDR   (HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x710)
#define HIL_REG0_PS3_REGISTER_F_PS3_SESSIONCMD_ADDR_IRQ_MASK_RST   (0x0000000000000000)
#endif

#ifndef __S1861_HIL_REG0_PS3_REGISTER_F_REG_STRUCT__ 
typedef union HilReg0Ps3RegisterFPs3Doorbell{

    volatile U64 val;
    struct{

        U64 cmd                            : 16;  
        U64 reserved1                      : 48;  
    }reg;
}HilReg0Ps3RegisterFPs3Doorbell_u;

typedef union HilReg0Ps3RegisterFPs3DoorbellIrqClear{

    volatile U64 val;
    struct{

        U64 pulse                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3DoorbellIrqClear_u;

typedef union HilReg0Ps3RegisterFPs3DoorbellIrqMask{

    volatile U64 val;
    struct{

        U64 level                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3DoorbellIrqMask_u;

typedef union HilReg0Ps3RegisterFPs3IrqControl{

    volatile U64 val;
    struct{

        U64 global                         : 1;   
        U64 fwState                        : 1;   
        U64 tbd                            : 30;  
        U64 reserved3                      : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3IrqControl_u;

typedef union HilReg0Ps3RegisterFPs3SoftresetKey{

    volatile U64 val;
    struct{

        U64 ps3SoftresetKey                : 8;   
        U64 reserved1                      : 56;  
    }reg;
}HilReg0Ps3RegisterFPs3SoftresetKey_u;

typedef union HilReg0Ps3RegisterFPs3SoftresetState{

    volatile U64 val;
    struct{

        U64 rpt                            : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3SoftresetState_u;

typedef union HilReg0Ps3RegisterFPs3Softreset{

    volatile U64 val;
    struct{

        U64 cmd                            : 8;   
        U64 reserved1                      : 56;  
    }reg;
}HilReg0Ps3RegisterFPs3Softreset_u;

typedef union HilReg0Ps3RegisterFPs3SoftresetIrqClear{

    volatile U64 val;
    struct{

        U64 pulse                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3SoftresetIrqClear_u;

typedef union HilReg0Ps3RegisterFPs3SoftresetIrqMask{

    volatile U64 val;
    struct{

        U64 level                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3SoftresetIrqMask_u;

typedef union HilReg0Ps3RegisterFPs3SoftresetKeyShiftRegLow{

    volatile U64 val;
    struct{

        U64 rpt                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3SoftresetKeyShiftRegLow_u;

typedef union HilReg0Ps3RegisterFPs3SoftresetKeyShiftRegHigh{

    volatile U64 val;
    struct{

        U64 rpt                            : 8;   
        U64 reserved1                      : 56;  
    }reg;
}HilReg0Ps3RegisterFPs3SoftresetKeyShiftRegHigh_u;

typedef union HilReg0Ps3RegisterFPs3SoftresetTimeCnt{

    volatile U64 val;
    struct{

        U64 rpt                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3SoftresetTimeCnt_u;

typedef union HilReg0Ps3RegisterFPs3SoftresetTimeOutEn{

    volatile U64 val;
    struct{

        U64 rpt                            : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3SoftresetTimeOutEn_u;

typedef union HilReg0Ps3RegisterFPs3HardresetKey{

    volatile U64 val;
    struct{

        U64 ps3HardresetKey                : 8;   
        U64 reserved1                      : 56;  
    }reg;
}HilReg0Ps3RegisterFPs3HardresetKey_u;

typedef union HilReg0Ps3RegisterFPs3HardresetState{

    volatile U64 val;
    struct{

        U64 rpt                            : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3HardresetState_u;

typedef union HilReg0Ps3RegisterFPs3Hardreset{

    volatile U64 val;
    struct{

        U64 config                         : 8;   
        U64 reserved1                      : 56;  
    }reg;
}HilReg0Ps3RegisterFPs3Hardreset_u;

typedef union HilReg0Ps3RegisterFPs3HardresetKeyShiftRegLow{

    volatile U64 val;
    struct{

        U64 rpt                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3HardresetKeyShiftRegLow_u;

typedef union HilReg0Ps3RegisterFPs3HardresetKeyShiftRegHigh{

    volatile U64 val;
    struct{

        U64 rpt                            : 8;   
        U64 reserved1                      : 56;  
    }reg;
}HilReg0Ps3RegisterFPs3HardresetKeyShiftRegHigh_u;

typedef union HilReg0Ps3RegisterFPs3HardresetTimeCnt{

    volatile U64 val;
    struct{

        U64 rpt                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3HardresetTimeCnt_u;

typedef union HilReg0Ps3RegisterFPs3HardresetTimeOutEn{

    volatile U64 val;
    struct{

        U64 rpt                            : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3HardresetTimeOutEn_u;

typedef union HilReg0Ps3RegisterFPs3KeyGapCfg{

    volatile U64 val;
    struct{

        U64 ps3KeyGapCfg                   : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3KeyGapCfg_u;

typedef union HilReg0Ps3RegisterFPs3HardresetIrqClear{

    volatile U64 val;
    struct{

        U64 pulse                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3HardresetIrqClear_u;

typedef union HilReg0Ps3RegisterFPs3HardresetIrqMask{

    volatile U64 val;
    struct{

        U64 level                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3HardresetIrqMask_u;

typedef union HilReg0Ps3RegisterFPs3SocFwState{

    volatile U64 val;
    struct{

        U64 ps3SocFwState                  : 8;   
        U64 ps3SocFwStartState             : 8;   
        U64 ps3SocBootState                : 8;   
        U64 tbd                            : 8;   
        U64 reserved4                      : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3SocFwState_u;

typedef union HilReg0Ps3RegisterFPs3MaxFwCmd{

    volatile U64 val;
    struct{

        U64 ps3MaxFwCmd                    : 16;  
        U64 reserved1                      : 48;  
    }reg;
}HilReg0Ps3RegisterFPs3MaxFwCmd_u;

typedef union HilReg0Ps3RegisterFPs3MaxChainSize{

    volatile U64 val;
    struct{

        U64 ps3MaxChainSize                : 32;  
        U64 reserved1                      : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3MaxChainSize_u;

typedef union HilReg0Ps3RegisterFPs3MaxVdInfoSize{

    volatile U64 val;
    struct{

        U64 ps3MaxVdInfoSize               : 32;  
        U64 reserved1                      : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3MaxVdInfoSize_u;

typedef union HilReg0Ps3RegisterFPs3MaxNvmePageSize{

    volatile U64 val;
    struct{

        U64 ps3MaxNvmePageSize             : 32;  
        U64 reserved1                      : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3MaxNvmePageSize_u;

typedef union HilReg0Ps3RegisterFPs3FeatureSupport{

    volatile U64 val;
    struct{

        U64 multiDevfnSupport              : 1;   
        U64 dmaBit64Support                : 1;   
        U64 debugOcmSupport                : 1;   
        U64 tbd1                           : 13;  
        U64 fwHaltSupport                  : 1;   
        U64 sglModeSupport                 : 1;   
        U64 dumpCrashSupport               : 1;   
        U64 shallowSoftRecoverySupport     : 1;   
        U64 deepSoftRecoverySupport        : 1;   
        U64 hardRecoverySupport            : 1;   
        U64 tbd2                           : 42;  
    }reg;
}HilReg0Ps3RegisterFPs3FeatureSupport_u;

typedef union HilReg0Ps3RegisterFPs3FirmwareVersion{

    volatile U64 val;
    struct{

        U64 ps3FmVer                       : 8;   
        U64 tbd                            : 24;  
        U64 reserved2                      : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3FirmwareVersion_u;

typedef union HilReg0Ps3RegisterFPs3MaxReplyque{

    volatile U64 val;
    struct{

        U64 ps3MaxReplyque                 : 16;  
        U64 reserved1                      : 48;  
    }reg;
}HilReg0Ps3RegisterFPs3MaxReplyque_u;

typedef union HilReg0Ps3RegisterFPs3HardwareVersion{

    volatile U64 val;
    struct{

        U64 chipId                         : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3HardwareVersion_u;

typedef union HilReg0Ps3RegisterFPs3MgrQueueDepth{

    volatile U64 val;
    struct{

        U64 ps3MgrQueueDepth               : 16;  
        U64 reserved1                      : 48;  
    }reg;
}HilReg0Ps3RegisterFPs3MgrQueueDepth_u;

typedef union HilReg0Ps3RegisterFPs3CmdQueueDepth{

    volatile U64 val;
    struct{

        U64 ps3CmdQueueDepth               : 16;  
        U64 reserved1                      : 48;  
    }reg;
}HilReg0Ps3RegisterFPs3CmdQueueDepth_u;

typedef union HilReg0Ps3RegisterFPs3TfifoDepth{

    volatile U64 val;
    struct{

        U64 ps3TfifoDepth                  : 16;  
        U64 reserved1                      : 48;  
    }reg;
}HilReg0Ps3RegisterFPs3TfifoDepth_u;

typedef union HilReg0Ps3RegisterFPs3MaxSecR1xCmds{

    volatile U64 val;
    struct{

        U64 ps3MaxSecR1xCmds               : 16;  
        U64 reserved1                      : 48;  
    }reg;
}HilReg0Ps3RegisterFPs3MaxSecR1xCmds_u;

typedef union HilReg0Ps3RegisterFPs3HilAdvice2directCnt0{

    volatile U64 val;
    struct{

        U64 rpt                            : 32;  
        U64 reserved1                      : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3HilAdvice2directCnt0_u;

typedef union HilReg0Ps3RegisterFPs3HilAdvice2directCnt1{

    volatile U64 val;
    struct{

        U64 rpt                            : 32;  
        U64 reserved1                      : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3HilAdvice2directCnt1_u;

typedef union HilReg0Ps3RegisterFPs3HilAdvice2directCnt2{

    volatile U64 val;
    struct{

        U64 rpt                            : 32;  
        U64 reserved1                      : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3HilAdvice2directCnt2_u;

typedef union HilReg0Ps3RegisterFPs3HilAdvice2directCnt3{

    volatile U64 val;
    struct{

        U64 rpt                            : 32;  
        U64 reserved1                      : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3HilAdvice2directCnt3_u;

typedef union HilReg0Ps3RegisterFPs3HilAdvice2directCntAll{

    volatile U64 val;
    struct{

        U64 rpt                            : 32;  
        U64 reserved1                      : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3HilAdvice2directCntAll_u;

typedef union HilReg0Ps3RegisterFPs3IrqStatusRpt{

    volatile U64 val;
    struct{

        U64 doorbell                       : 1;   
        U64 reserved1                      : 3;   
        U64 softreset                      : 1;   
        U64 reserved3                      : 3;   
        U64 dumpCtrl                       : 1;   
        U64 reserved5                      : 3;   
        U64 debug0                         : 1;   
        U64 reserved7                      : 3;   
        U64 debug1                         : 1;   
        U64 reserved9                      : 3;   
        U64 debug2                         : 1;   
        U64 reserved11                     : 3;   
        U64 debug3                         : 1;   
        U64 reserved13                     : 3;   
        U64 debug4                         : 1;   
        U64 reserved15                     : 3;   
        U64 cmdTrigger                     : 1;   
        U64 reserved17                     : 3;   
        U64 hardreset                      : 1;   
        U64 reserved19                     : 3;   
        U64 sessioncmdAddr                 : 1;   
        U64 reserved21                     : 23;  
    }reg;
}HilReg0Ps3RegisterFPs3IrqStatusRpt_u;

typedef union HilReg0Ps3RegisterFPs3DumpCtrl{

    volatile U64 val;
    struct{

        U64 cmd                            : 16;  
        U64 reserved1                      : 48;  
    }reg;
}HilReg0Ps3RegisterFPs3DumpCtrl_u;

typedef union HilReg0Ps3RegisterFPs3DumpCtrlIrqClear{

    volatile U64 val;
    struct{

        U64 pulse                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3DumpCtrlIrqClear_u;

typedef union HilReg0Ps3RegisterFPs3DumpCtrlIrqMask{

    volatile U64 val;
    struct{

        U64 level                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3DumpCtrlIrqMask_u;

typedef union HilReg0Ps3RegisterFPs3DumpStatus{

    volatile U64 val;
    struct{

        U64 dmaFinish                      : 1;   
        U64 hasCrashDump                   : 1;   
        U64 hasFwDump                      : 1;   
        U64 hasBarDump                     : 1;   
        U64 hasAutoDump                    : 2;   
        U64 tbd                            : 10;  
        U64 reserved6                      : 48;  
    }reg;
}HilReg0Ps3RegisterFPs3DumpStatus_u;

typedef union HilReg0Ps3RegisterFPs3DumpDataSize{

    volatile U64 val;
    struct{

        U64 ps3DumpDataSize                : 32;  
        U64 reserved1                      : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3DumpDataSize_u;

typedef union HilReg0Ps3RegisterFPs3CmdTrigger{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3CmdTrigger_u;

typedef union HilReg0Ps3RegisterFPs3CmdTriggerIrqClear{

    volatile U64 val;
    struct{

        U64 pulse                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3CmdTriggerIrqClear_u;

typedef union HilReg0Ps3RegisterFPs3CmdTriggerIrqMask{

    volatile U64 val;
    struct{

        U64 level                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3CmdTriggerIrqMask_u;

typedef union HilReg0Ps3RegisterFPs3SoftresetCounter{

    volatile U64 val;
    struct{

        U64 rpt                            : 32;  
        U64 tbd                            : 32;  
    }reg;
}HilReg0Ps3RegisterFPs3SoftresetCounter_u;

typedef union HilReg0Ps3RegisterFPs3RegCmdState{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3RegCmdState_u;

typedef union HilReg0Ps3RegisterFPs3Debug0{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug0_u;

typedef union HilReg0Ps3RegisterFPs3Debug0IrqClear{

    volatile U64 val;
    struct{

        U64 pulse                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug0IrqClear_u;

typedef union HilReg0Ps3RegisterFPs3Debug0IrqMask{

    volatile U64 val;
    struct{

        U64 level                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug0IrqMask_u;

typedef union HilReg0Ps3RegisterFPs3Debug1{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug1_u;

typedef union HilReg0Ps3RegisterFPs3Debug1IrqClear{

    volatile U64 val;
    struct{

        U64 pulse                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug1IrqClear_u;

typedef union HilReg0Ps3RegisterFPs3Debug1IrqMask{

    volatile U64 val;
    struct{

        U64 level                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug1IrqMask_u;

typedef union HilReg0Ps3RegisterFPs3Debug2{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug2_u;

typedef union HilReg0Ps3RegisterFPs3Debug2IrqClear{

    volatile U64 val;
    struct{

        U64 pulse                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug2IrqClear_u;

typedef union HilReg0Ps3RegisterFPs3Debug2IrqMask{

    volatile U64 val;
    struct{

        U64 level                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug2IrqMask_u;

typedef union HilReg0Ps3RegisterFPs3Debug3{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug3_u;

typedef union HilReg0Ps3RegisterFPs3Debug3IrqClear{

    volatile U64 val;
    struct{

        U64 pulse                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug3IrqClear_u;

typedef union HilReg0Ps3RegisterFPs3Debug3IrqMask{

    volatile U64 val;
    struct{

        U64 level                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug3IrqMask_u;

typedef union HilReg0Ps3RegisterFPs3Debug4{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug4_u;

typedef union HilReg0Ps3RegisterFPs3Debug4IrqClear{

    volatile U64 val;
    struct{

        U64 pulse                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug4IrqClear_u;

typedef union HilReg0Ps3RegisterFPs3Debug4IrqMask{

    volatile U64 val;
    struct{

        U64 level                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug4IrqMask_u;

typedef union HilReg0Ps3RegisterFPs3Debug5{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug5_u;

typedef union HilReg0Ps3RegisterFPs3Debug6{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug6_u;

typedef union HilReg0Ps3RegisterFPs3Debug7{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug7_u;

typedef union HilReg0Ps3RegisterFPs3Debug8{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug8_u;

typedef union HilReg0Ps3RegisterFPs3Debug9{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug9_u;

typedef union HilReg0Ps3RegisterFPs3Debug10{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug10_u;

typedef union HilReg0Ps3RegisterFPs3Debug11{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug11_u;

typedef union HilReg0Ps3RegisterFPs3Debug12{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3Debug12_u;

typedef union HilReg0Ps3RegisterFPs3SessioncmdAddr{

    volatile U64 val;
    struct{

        U64 cmd                            : 64;  
    }reg;
}HilReg0Ps3RegisterFPs3SessioncmdAddr_u;

typedef union HilReg0Ps3RegisterFPs3SessioncmdAddrIrqClear{

    volatile U64 val;
    struct{

        U64 pulse                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3SessioncmdAddrIrqClear_u;

typedef union HilReg0Ps3RegisterFPs3SessioncmdAddrIrqMask{

    volatile U64 val;
    struct{

        U64 level                          : 1;   
        U64 reserved1                      : 63;  
    }reg;
}HilReg0Ps3RegisterFPs3SessioncmdAddrIrqMask_u;

typedef struct HilReg0Ps3RegisterF{

    U64                                        reserved0[8];                 
    HilReg0Ps3RegisterFPs3Doorbell_u           ps3Doorbell;                  
    HilReg0Ps3RegisterFPs3DoorbellIrqClear_u   ps3DoorbellIrqClear;          
    HilReg0Ps3RegisterFPs3DoorbellIrqMask_u    ps3DoorbellIrqMask;           
    HilReg0Ps3RegisterFPs3IrqControl_u         ps3IrqControl;                
    U64                                        reserved1[20];                
    HilReg0Ps3RegisterFPs3SoftresetKey_u       ps3SoftresetKey;              
    HilReg0Ps3RegisterFPs3SoftresetState_u     ps3SoftresetState;            
    HilReg0Ps3RegisterFPs3Softreset_u          ps3Softreset;                 
    HilReg0Ps3RegisterFPs3SoftresetIrqClear_u   ps3SoftresetIrqClear;         
    HilReg0Ps3RegisterFPs3SoftresetIrqMask_u   ps3SoftresetIrqMask;          
    HilReg0Ps3RegisterFPs3SoftresetKeyShiftRegLow_u   ps3SoftresetKeyShiftRegLow;   
    HilReg0Ps3RegisterFPs3SoftresetKeyShiftRegHigh_u   ps3SoftresetKeyShiftRegHigh;  
    HilReg0Ps3RegisterFPs3SoftresetTimeCnt_u   ps3SoftresetTimeCnt;          
    HilReg0Ps3RegisterFPs3SoftresetTimeOutEn_u   ps3SoftresetTimeOutEn;        
    U64                                        reserved2[23];                
    HilReg0Ps3RegisterFPs3HardresetKey_u       ps3HardresetKey;              
    HilReg0Ps3RegisterFPs3HardresetState_u     ps3HardresetState;            
    HilReg0Ps3RegisterFPs3Hardreset_u          ps3Hardreset;                 
    HilReg0Ps3RegisterFPs3HardresetKeyShiftRegLow_u   ps3HardresetKeyShiftRegLow;   
    HilReg0Ps3RegisterFPs3HardresetKeyShiftRegHigh_u   ps3HardresetKeyShiftRegHigh;  
    HilReg0Ps3RegisterFPs3HardresetTimeCnt_u   ps3HardresetTimeCnt;          
    HilReg0Ps3RegisterFPs3HardresetTimeOutEn_u   ps3HardresetTimeOutEn;        
    HilReg0Ps3RegisterFPs3KeyGapCfg_u          ps3KeyGapCfg;                 
    HilReg0Ps3RegisterFPs3HardresetIrqClear_u   ps3HardresetIrqClear;         
    HilReg0Ps3RegisterFPs3HardresetIrqMask_u   ps3HardresetIrqMask;          
    U64                                        reserved3[22];                
    HilReg0Ps3RegisterFPs3SocFwState_u         ps3SocFwState;                
    HilReg0Ps3RegisterFPs3MaxFwCmd_u           ps3MaxFwCmd;                  
    HilReg0Ps3RegisterFPs3MaxChainSize_u       ps3MaxChainSize;              
    HilReg0Ps3RegisterFPs3MaxVdInfoSize_u      ps3MaxVdInfoSize;             
    HilReg0Ps3RegisterFPs3MaxNvmePageSize_u    ps3MaxNvmePageSize;           
    HilReg0Ps3RegisterFPs3FeatureSupport_u     ps3FeatureSupport;            
    HilReg0Ps3RegisterFPs3FirmwareVersion_u    ps3FirmwareVersion;           
    HilReg0Ps3RegisterFPs3MaxReplyque_u        ps3MaxReplyque;               
    HilReg0Ps3RegisterFPs3HardwareVersion_u    ps3HardwareVersion;           
    HilReg0Ps3RegisterFPs3MgrQueueDepth_u      ps3MgrQueueDepth;             
    HilReg0Ps3RegisterFPs3CmdQueueDepth_u      ps3CmdQueueDepth;             
    HilReg0Ps3RegisterFPs3TfifoDepth_u         ps3TfifoDepth;                
    HilReg0Ps3RegisterFPs3MaxSecR1xCmds_u      ps3MaxSecR1xCmds;             
    U64                                        reserved4[19];                
    HilReg0Ps3RegisterFPs3HilAdvice2directCnt0_u   ps3HilAdvice2directCnt0;      
    HilReg0Ps3RegisterFPs3HilAdvice2directCnt1_u   ps3HilAdvice2directCnt1;      
    HilReg0Ps3RegisterFPs3HilAdvice2directCnt2_u   ps3HilAdvice2directCnt2;      
    HilReg0Ps3RegisterFPs3HilAdvice2directCnt3_u   ps3HilAdvice2directCnt3;      
    HilReg0Ps3RegisterFPs3HilAdvice2directCntAll_u   ps3HilAdvice2directCntAll;    
    U64                                        reserved5[3];                 
    HilReg0Ps3RegisterFPs3IrqStatusRpt_u       ps3IrqStatusRpt;              
    U64                                        reserved6[23];                
    HilReg0Ps3RegisterFPs3DumpCtrl_u           ps3DumpCtrl;                  
    HilReg0Ps3RegisterFPs3DumpCtrlIrqClear_u   ps3DumpCtrlIrqClear;          
    HilReg0Ps3RegisterFPs3DumpCtrlIrqMask_u    ps3DumpCtrlIrqMask;           
    HilReg0Ps3RegisterFPs3DumpStatus_u         ps3DumpStatus;                
    HilReg0Ps3RegisterFPs3DumpDataSize_u       ps3DumpDataSize;              
    U64                                        reserved7[27];                
    HilReg0Ps3RegisterFPs3CmdTrigger_u         ps3CmdTrigger;                
    HilReg0Ps3RegisterFPs3CmdTriggerIrqClear_u   ps3CmdTriggerIrqClear;        
    HilReg0Ps3RegisterFPs3CmdTriggerIrqMask_u   ps3CmdTriggerIrqMask;         
    HilReg0Ps3RegisterFPs3SoftresetCounter_u   ps3SoftresetCounter;          
    HilReg0Ps3RegisterFPs3RegCmdState_u        ps3RegCmdState;               
    HilReg0Ps3RegisterFPs3Debug0_u             ps3Debug0;                    
    HilReg0Ps3RegisterFPs3Debug0IrqClear_u     ps3Debug0IrqClear;            
    HilReg0Ps3RegisterFPs3Debug0IrqMask_u      ps3Debug0IrqMask;             
    HilReg0Ps3RegisterFPs3Debug1_u             ps3Debug1;                    
    HilReg0Ps3RegisterFPs3Debug1IrqClear_u     ps3Debug1IrqClear;            
    HilReg0Ps3RegisterFPs3Debug1IrqMask_u      ps3Debug1IrqMask;             
    HilReg0Ps3RegisterFPs3Debug2_u             ps3Debug2;                    
    HilReg0Ps3RegisterFPs3Debug2IrqClear_u     ps3Debug2IrqClear;            
    HilReg0Ps3RegisterFPs3Debug2IrqMask_u      ps3Debug2IrqMask;             
    HilReg0Ps3RegisterFPs3Debug3_u             ps3Debug3;                    
    HilReg0Ps3RegisterFPs3Debug3IrqClear_u     ps3Debug3IrqClear;            
    HilReg0Ps3RegisterFPs3Debug3IrqMask_u      ps3Debug3IrqMask;             
    HilReg0Ps3RegisterFPs3Debug4_u             ps3Debug4;                    
    HilReg0Ps3RegisterFPs3Debug4IrqClear_u     ps3Debug4IrqClear;            
    HilReg0Ps3RegisterFPs3Debug4IrqMask_u      ps3Debug4IrqMask;             
    HilReg0Ps3RegisterFPs3Debug5_u             ps3Debug5;                    
    HilReg0Ps3RegisterFPs3Debug6_u             ps3Debug6;                    
    HilReg0Ps3RegisterFPs3Debug7_u             ps3Debug7;                    
    HilReg0Ps3RegisterFPs3Debug8_u             ps3Debug8;                    
    HilReg0Ps3RegisterFPs3Debug9_u             ps3Debug9;                    
    HilReg0Ps3RegisterFPs3Debug10_u            ps3Debug10;                   
    HilReg0Ps3RegisterFPs3Debug11_u            ps3Debug11;                   
    HilReg0Ps3RegisterFPs3Debug12_u            ps3Debug12;                   
    U64                                        reserved8[4];                 
    HilReg0Ps3RegisterFPs3SessioncmdAddr_u     ps3SessioncmdAddr;            
    HilReg0Ps3RegisterFPs3SessioncmdAddrIrqClear_u   ps3SessioncmdAddrIrqClear;    
    HilReg0Ps3RegisterFPs3SessioncmdAddrIrqMask_u   ps3SessioncmdAddrIrqMask;     
}HilReg0Ps3RegisterF_s;
#endif
#endif