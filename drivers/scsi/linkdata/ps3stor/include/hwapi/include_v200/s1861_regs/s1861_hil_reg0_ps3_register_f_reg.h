/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __S1861_HIL_REG0_PS3_REGISTER_F_REG_H__
#define __S1861_HIL_REG0_PS3_REGISTER_F_REG_H__
#include "s1861_global_baseaddr.h"
#ifndef __S1861_HIL_REG0_PS3_REGISTER_F_REG_MACRO__
#define HIL_REG0_PS3_REGISTER_F_PS3_DOORBELL_ADDR                              \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x40)
#define HIL_REG0_PS3_REGISTER_F_PS3_DOORBELL_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DOORBELL_IRQ_CLEAR_ADDR                    \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x48)
#define HIL_REG0_PS3_REGISTER_F_PS3_DOORBELL_IRQ_CLEAR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DOORBELL_IRQ_MASK_ADDR                     \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x50)
#define HIL_REG0_PS3_REGISTER_F_PS3_DOORBELL_IRQ_MASK_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_IRQ_CONTROL_ADDR                           \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x58)
#define HIL_REG0_PS3_REGISTER_F_PS3_IRQ_CONTROL_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_KEY_ADDR                         \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x100)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_KEY_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_STATE_ADDR                       \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x108)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_STATE_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_ADDR                             \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x110)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_IRQ_CLEAR_ADDR                   \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x118)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_IRQ_CLEAR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_IRQ_MASK_ADDR                    \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x120)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_IRQ_MASK_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_KEY_SHIFT_REG_LOW_ADDR           \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x128)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_KEY_SHIFT_REG_LOW_RST            \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_KEY_SHIFT_REG_HIGH_ADDR          \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x130)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_KEY_SHIFT_REG_HIGH_RST           \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_TIME_CNT_ADDR                    \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x138)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_TIME_CNT_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_TIME_OUT_EN_ADDR                 \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x140)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_TIME_OUT_EN_RST                  \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_KEY_ADDR                         \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x200)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_KEY_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_STATE_ADDR                       \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x208)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_STATE_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_ADDR                             \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x210)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_KEY_SHIFT_REG_LOW_ADDR           \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x218)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_KEY_SHIFT_REG_LOW_RST            \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_KEY_SHIFT_REG_HIGH_ADDR          \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x220)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_KEY_SHIFT_REG_HIGH_RST           \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_TIME_CNT_ADDR                    \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x228)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_TIME_CNT_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_TIME_OUT_EN_ADDR                 \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x230)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_TIME_OUT_EN_RST                  \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_KEY_GAP_CFG_ADDR                           \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x238)
#define HIL_REG0_PS3_REGISTER_F_PS3_KEY_GAP_CFG_RST (0x0000000002FAF080)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_IRQ_CLEAR_ADDR                   \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x240)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_IRQ_CLEAR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_IRQ_MASK_ADDR                    \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x248)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDRESET_IRQ_MASK_RST (0x0000000000000001)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOC_FW_STATE_ADDR                          \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x300)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOC_FW_STATE_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_FW_CMD_ADDR                            \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x308)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_FW_CMD_RST (0x0000000000001FFF)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_CHAIN_SIZE_ADDR                        \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x310)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_CHAIN_SIZE_RST (0x0000000000000FFF)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_VD_INFO_SIZE_ADDR                      \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x318)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_VD_INFO_SIZE_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_NVME_PAGE_SIZE_ADDR                    \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x320)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_NVME_PAGE_SIZE_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_FEATURE_SUPPORT_ADDR                       \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x328)
#define HIL_REG0_PS3_REGISTER_F_PS3_FEATURE_SUPPORT_RST (0x0000000000000007)
#define HIL_REG0_PS3_REGISTER_F_PS3_FIRMWARE_VERSION_ADDR                      \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x330)
#define HIL_REG0_PS3_REGISTER_F_PS3_FIRMWARE_VERSION_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_REPLYQUE_ADDR                          \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x338)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_REPLYQUE_RST (0x000000000000007F)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDWARE_VERSION_ADDR                      \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x340)
#define HIL_REG0_PS3_REGISTER_F_PS3_HARDWARE_VERSION_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_MGR_QUEUE_DEPTH_ADDR                       \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x348)
#define HIL_REG0_PS3_REGISTER_F_PS3_MGR_QUEUE_DEPTH_RST (0x0000000000000400)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_QUEUE_DEPTH_ADDR                       \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x350)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_QUEUE_DEPTH_RST (0x0000000000001000)
#define HIL_REG0_PS3_REGISTER_F_PS3_TFIFO_DEPTH_ADDR                           \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x358)
#define HIL_REG0_PS3_REGISTER_F_PS3_TFIFO_DEPTH_RST (0x0000000000000400)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_SEC_R1X_CMDS_ADDR                      \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x360)
#define HIL_REG0_PS3_REGISTER_F_PS3_MAX_SEC_R1X_CMDS_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT0_ADDR                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x400)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT0_RST                 \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT1_ADDR                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x408)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT1_RST                 \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT2_ADDR                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x410)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT2_RST                 \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT3_ADDR                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x418)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT3_RST                 \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT_ALL_ADDR             \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x420)
#define HIL_REG0_PS3_REGISTER_F_PS3_HIL_ADVICE2DIRECT_CNT_ALL_RST              \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_IRQ_STATUS_RPT_ADDR                        \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x440)
#define HIL_REG0_PS3_REGISTER_F_PS3_IRQ_STATUS_RPT_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_CTRL_ADDR                             \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x500)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_CTRL_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_CTRL_IRQ_CLEAR_ADDR                   \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x508)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_CTRL_IRQ_CLEAR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_CTRL_IRQ_MASK_ADDR                    \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x510)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_CTRL_IRQ_MASK_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_STATUS_ADDR                           \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x518)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_STATUS_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_DATA_SIZE_ADDR                        \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x520)
#define HIL_REG0_PS3_REGISTER_F_PS3_DUMP_DATA_SIZE_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_TRIGGER_ADDR                           \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x600)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_TRIGGER_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_TRIGGER_IRQ_CLEAR_ADDR                 \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x608)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_TRIGGER_IRQ_CLEAR_RST                  \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_TRIGGER_IRQ_MASK_ADDR                  \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x610)
#define HIL_REG0_PS3_REGISTER_F_PS3_CMD_TRIGGER_IRQ_MASK_RST                   \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_COUNTER_ADDR                     \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x618)
#define HIL_REG0_PS3_REGISTER_F_PS3_SOFTRESET_COUNTER_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_REG_CMD_STATE_ADDR                         \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x620)
#define HIL_REG0_PS3_REGISTER_F_PS3_REG_CMD_STATE_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG0_ADDR                                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x628)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG0_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG0_IRQ_CLEAR_ADDR                      \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x630)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG0_IRQ_CLEAR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG0_IRQ_MASK_ADDR                       \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x638)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG0_IRQ_MASK_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG1_ADDR                                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x640)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG1_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG1_IRQ_CLEAR_ADDR                      \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x648)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG1_IRQ_CLEAR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG1_IRQ_MASK_ADDR                       \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x650)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG1_IRQ_MASK_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG2_ADDR                                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x658)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG2_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG2_IRQ_CLEAR_ADDR                      \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x660)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG2_IRQ_CLEAR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG2_IRQ_MASK_ADDR                       \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x668)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG2_IRQ_MASK_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG3_ADDR                                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x670)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG3_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG3_IRQ_CLEAR_ADDR                      \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x678)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG3_IRQ_CLEAR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG3_IRQ_MASK_ADDR                       \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x680)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG3_IRQ_MASK_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG4_ADDR                                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x688)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG4_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG4_IRQ_CLEAR_ADDR                      \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x690)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG4_IRQ_CLEAR_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG4_IRQ_MASK_ADDR                       \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x698)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG4_IRQ_MASK_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG5_ADDR                                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6a0)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG5_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG6_ADDR                                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6a8)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG6_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG7_ADDR                                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6b0)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG7_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG8_ADDR                                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6b8)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG8_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG9_ADDR                                \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6c0)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG9_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG10_ADDR                               \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6c8)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG10_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG11_ADDR                               \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6d0)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG11_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG12_ADDR                               \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x6d8)
#define HIL_REG0_PS3_REGISTER_F_PS3_DEBUG12_RST (0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SESSIONCMD_ADDR_ADDR                       \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x700)
#define HIL_REG0_PS3_REGISTER_F_PS3_SESSIONCMD_ADDR_RST (0xFFFFFFFFFFFFFFFF)
#define HIL_REG0_PS3_REGISTER_F_PS3_SESSIONCMD_ADDR_IRQ_CLEAR_ADDR             \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x708)
#define HIL_REG0_PS3_REGISTER_F_PS3_SESSIONCMD_ADDR_IRQ_CLEAR_RST              \
	(0x0000000000000000)
#define HIL_REG0_PS3_REGISTER_F_PS3_SESSIONCMD_ADDR_IRQ_MASK_ADDR              \
	(HIL_REG0_PS3_REGISTER_F_BASEADDR + 0x710)
#define HIL_REG0_PS3_REGISTER_F_PS3_SESSIONCMD_ADDR_IRQ_MASK_RST               \
	(0x0000000000000000)
#endif

#ifndef __S1861_HIL_REG0_PS3_REGISTER_F_REG_STRUCT__
union HilReg0Ps3RegisterFPs3Doorbell {
	unsigned long long val;
	struct {

		unsigned long long cmd : 16;
		unsigned long long reserved1 : 48;
	} reg;
};

union HilReg0Ps3RegisterFPs3DoorbellIrqClear {
	unsigned long long val;
	struct {

		unsigned long long pulse : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3DoorbellIrqMask {
	unsigned long long val;
	struct {

		unsigned long long level : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3IrqControl {
	unsigned long long val;
	struct {

		unsigned long long global : 1;
		unsigned long long fwState : 1;
		unsigned long long tbd : 30;
		unsigned long long reserved3 : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3SoftresetKey {
	unsigned long long val;
	struct {

		unsigned long long ps3SoftresetKey : 8;
		unsigned long long reserved1 : 56;
	} reg;
};

union HilReg0Ps3RegisterFPs3SoftresetState {
	unsigned long long val;
	struct {

		unsigned long long rpt : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3Softreset {
	unsigned long long val;
	struct {

		unsigned long long cmd : 8;
		unsigned long long reserved1 : 56;
	} reg;
};

union HilReg0Ps3RegisterFPs3SoftresetIrqClear {
	unsigned long long val;
	struct {

		unsigned long long pulse : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3SoftresetIrqMask {
	unsigned long long val;
	struct {

		unsigned long long level : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3SoftresetKeyShiftRegLow {
	unsigned long long val;
	struct {

		unsigned long long rpt : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3SoftresetKeyShiftRegHigh {
	unsigned long long val;
	struct {

		unsigned long long rpt : 8;
		unsigned long long reserved1 : 56;
	} reg;
};

union HilReg0Ps3RegisterFPs3SoftresetTimeCnt {
	unsigned long long val;
	struct {

		unsigned long long rpt : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3SoftresetTimeOutEn {
	unsigned long long val;
	struct {

		unsigned long long rpt : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3HardresetKey {
	unsigned long long val;
	struct {

		unsigned long long ps3HardresetKey : 8;
		unsigned long long reserved1 : 56;
	} reg;
};

union HilReg0Ps3RegisterFPs3HardresetState {
	unsigned long long val;
	struct {

		unsigned long long rpt : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3Hardreset {
	unsigned long long val;
	struct {

		unsigned long long config : 8;
		unsigned long long reserved1 : 56;
	} reg;
};

union HilReg0Ps3RegisterFPs3HardresetKeyShiftRegLow {
	unsigned long long val;
	struct {

		unsigned long long rpt : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3HardresetKeyShiftRegHigh {
	unsigned long long val;
	struct {

		unsigned long long rpt : 8;
		unsigned long long reserved1 : 56;
	} reg;
};

union HilReg0Ps3RegisterFPs3HardresetTimeCnt {
	unsigned long long val;
	struct {

		unsigned long long rpt : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3HardresetTimeOutEn {
	unsigned long long val;
	struct {

		unsigned long long rpt : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3KeyGapCfg {
	unsigned long long val;
	struct {

		unsigned long long ps3KeyGapCfg : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3HardresetIrqClear {
	unsigned long long val;
	struct {

		unsigned long long pulse : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3HardresetIrqMask {
	unsigned long long val;
	struct {

		unsigned long long level : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3SocFwState {
	unsigned long long val;
	struct {

		unsigned long long ps3SocFwState : 8;
		unsigned long long ps3SocFwStartState : 8;
		unsigned long long ps3SocBootState : 8;
		unsigned long long tbd : 8;
		unsigned long long reserved4 : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3MaxFwCmd {
	unsigned long long val;
	struct {

		unsigned long long ps3MaxFwCmd : 16;
		unsigned long long reserved1 : 48;
	} reg;
};

union HilReg0Ps3RegisterFPs3MaxChainSize {
	unsigned long long val;
	struct {

		unsigned long long ps3MaxChainSize : 32;
		unsigned long long reserved1 : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3MaxVdInfoSize {
	unsigned long long val;
	struct {

		unsigned long long ps3MaxVdInfoSize : 32;
		unsigned long long reserved1 : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3MaxNvmePageSize {
	unsigned long long val;
	struct {

		unsigned long long ps3MaxNvmePageSize : 32;
		unsigned long long reserved1 : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3FeatureSupport {
	unsigned long long val;
	struct {

		unsigned long long multiDevfnSupport : 1;
		unsigned long long dmaBit64Support : 1;
		unsigned long long debugOcmSupport : 1;
		unsigned long long tbd1 : 13;
		unsigned long long fwHaltSupport : 1;
		unsigned long long sglModeSupport : 1;
		unsigned long long dumpCrashSupport : 1;
		unsigned long long shallowSoftRecoverySupport : 1;
		unsigned long long deepSoftRecoverySupport : 1;
		unsigned long long hardRecoverySupport : 1;
		unsigned long long tbd2 : 42;
	} reg;
};

union HilReg0Ps3RegisterFPs3FirmwareVersion {
	unsigned long long val;
	struct {

		unsigned long long ps3FmVer : 8;
		unsigned long long tbd : 24;
		unsigned long long reserved2 : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3MaxReplyque {
	unsigned long long val;
	struct {

		unsigned long long ps3MaxReplyque : 16;
		unsigned long long reserved1 : 48;
	} reg;
};

union HilReg0Ps3RegisterFPs3HardwareVersion {
	unsigned long long val;
	struct {

		unsigned long long chipId : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3MgrQueueDepth {
	unsigned long long val;
	struct {

		unsigned long long ps3MgrQueueDepth : 16;
		unsigned long long reserved1 : 48;
	} reg;
};

union HilReg0Ps3RegisterFPs3CmdQueueDepth {
	unsigned long long val;
	struct {

		unsigned long long ps3CmdQueueDepth : 16;
		unsigned long long reserved1 : 48;
	} reg;
};

union HilReg0Ps3RegisterFPs3TfifoDepth {
	unsigned long long val;
	struct {

		unsigned long long ps3TfifoDepth : 16;
		unsigned long long reserved1 : 48;
	} reg;
};

union HilReg0Ps3RegisterFPs3MaxSecR1xCmds {
	unsigned long long val;
	struct {

		unsigned long long ps3MaxSecR1xCmds : 16;
		unsigned long long reserved1 : 48;
	} reg;
};

union HilReg0Ps3RegisterFPs3HilAdvice2directCnt0 {
	unsigned long long val;
	struct {

		unsigned long long rpt : 32;
		unsigned long long reserved1 : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3HilAdvice2directCnt1 {
	unsigned long long val;
	struct {

		unsigned long long rpt : 32;
		unsigned long long reserved1 : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3HilAdvice2directCnt2 {
	unsigned long long val;
	struct {

		unsigned long long rpt : 32;
		unsigned long long reserved1 : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3HilAdvice2directCnt3 {
	unsigned long long val;
	struct {

		unsigned long long rpt : 32;
		unsigned long long reserved1 : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3HilAdvice2directCntAll {
	unsigned long long val;
	struct {

		unsigned long long rpt : 32;
		unsigned long long reserved1 : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3IrqStatusRpt {
	unsigned long long val;
	struct {

		unsigned long long doorbell : 1;
		unsigned long long reserved1 : 3;
		unsigned long long softreset : 1;
		unsigned long long reserved3 : 3;
		unsigned long long dumpCtrl : 1;
		unsigned long long reserved5 : 3;
		unsigned long long debug0 : 1;
		unsigned long long reserved7 : 3;
		unsigned long long debug1 : 1;
		unsigned long long reserved9 : 3;
		unsigned long long debug2 : 1;
		unsigned long long reserved11 : 3;
		unsigned long long debug3 : 1;
		unsigned long long reserved13 : 3;
		unsigned long long debug4 : 1;
		unsigned long long reserved15 : 3;
		unsigned long long cmdTrigger : 1;
		unsigned long long reserved17 : 3;
		unsigned long long hardreset : 1;
		unsigned long long reserved19 : 3;
		unsigned long long sessioncmdAddr : 1;
		unsigned long long reserved21 : 23;
	} reg;
};

union HilReg0Ps3RegisterFPs3DumpCtrl {
	unsigned long long val;
	struct {

		unsigned long long cmd : 16;
		unsigned long long reserved1 : 48;
	} reg;
};

union HilReg0Ps3RegisterFPs3DumpCtrlIrqClear {
	unsigned long long val;
	struct {

		unsigned long long pulse : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3DumpCtrlIrqMask {
	unsigned long long val;
	struct {

		unsigned long long level : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3DumpStatus {
	unsigned long long val;
	struct {

		unsigned long long dmaFinish : 1;
		unsigned long long hasCrashDump : 1;
		unsigned long long hasFwDump : 1;
		unsigned long long hasBarDump : 1;
		unsigned long long hasAutoDump : 2;
		unsigned long long tbd : 10;
		unsigned long long reserved6 : 48;
	} reg;
};

union HilReg0Ps3RegisterFPs3DumpDataSize {
	unsigned long long val;
	struct {

		unsigned long long ps3DumpDataSize : 32;
		unsigned long long reserved1 : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3CmdTrigger {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3CmdTriggerIrqClear {
	unsigned long long val;
	struct {

		unsigned long long pulse : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3CmdTriggerIrqMask {
	unsigned long long val;
	struct {

		unsigned long long level : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3SoftresetCounter {
	unsigned long long val;
	struct {

		unsigned long long rpt : 32;
		unsigned long long tbd : 32;
	} reg;
};

union HilReg0Ps3RegisterFPs3RegCmdState {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug0 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug0IrqClear {
	unsigned long long val;
	struct {

		unsigned long long pulse : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug0IrqMask {
	unsigned long long val;
	struct {

		unsigned long long level : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug1 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug1IrqClear {
	unsigned long long val;
	struct {

		unsigned long long pulse : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug1IrqMask {
	unsigned long long val;
	struct {

		unsigned long long level : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug2 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug2IrqClear {
	unsigned long long val;
	struct {

		unsigned long long pulse : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug2IrqMask {
	unsigned long long val;
	struct {

		unsigned long long level : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug3 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug3IrqClear {
	unsigned long long val;
	struct {

		unsigned long long pulse : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug3IrqMask {
	unsigned long long val;
	struct {

		unsigned long long level : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug4 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug4IrqClear {
	unsigned long long val;
	struct {

		unsigned long long pulse : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug4IrqMask {
	unsigned long long val;
	struct {

		unsigned long long level : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug5 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug6 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug7 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug8 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug9 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug10 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug11 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3Debug12 {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3SessioncmdAddr {
	unsigned long long val;
	struct {

		unsigned long long cmd : 64;
	} reg;
};

union HilReg0Ps3RegisterFPs3SessioncmdAddrIrqClear {
	unsigned long long val;
	struct {

		unsigned long long pulse : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

union HilReg0Ps3RegisterFPs3SessioncmdAddrIrqMask {
	unsigned long long val;
	struct {

		unsigned long long level : 1;
		unsigned long long reserved1 : 63;
	} reg;
};

struct HilReg0Ps3RegisterF {

	unsigned long long reserved0[8];
	union HilReg0Ps3RegisterFPs3Doorbell ps3Doorbell;
	union HilReg0Ps3RegisterFPs3DoorbellIrqClear ps3DoorbellIrqClear;
	union HilReg0Ps3RegisterFPs3DoorbellIrqMask ps3DoorbellIrqMask;
	union HilReg0Ps3RegisterFPs3IrqControl ps3IrqControl;
	unsigned long long reserved1[20];
	union HilReg0Ps3RegisterFPs3SoftresetKey ps3SoftresetKey;
	union HilReg0Ps3RegisterFPs3SoftresetState ps3SoftresetState;
	union HilReg0Ps3RegisterFPs3Softreset ps3Softreset;
	union HilReg0Ps3RegisterFPs3SoftresetIrqClear ps3SoftresetIrqClear;
	union HilReg0Ps3RegisterFPs3SoftresetIrqMask ps3SoftresetIrqMask;

	union HilReg0Ps3RegisterFPs3SoftresetKeyShiftRegLow
		ps3SoftresetKeyShiftRegLow;
	union HilReg0Ps3RegisterFPs3SoftresetKeyShiftRegHigh
		ps3SoftresetKeyShiftRegHigh;
	union HilReg0Ps3RegisterFPs3SoftresetTimeCnt ps3SoftresetTimeCnt;
	union HilReg0Ps3RegisterFPs3SoftresetTimeOutEn ps3SoftresetTimeOutEn;
	unsigned long long reserved2[23];
	union HilReg0Ps3RegisterFPs3HardresetKey ps3HardresetKey;
	union HilReg0Ps3RegisterFPs3HardresetState ps3HardresetState;
	union HilReg0Ps3RegisterFPs3Hardreset ps3Hardreset;

	union HilReg0Ps3RegisterFPs3HardresetKeyShiftRegLow
		ps3HardresetKeyShiftRegLow;
	union HilReg0Ps3RegisterFPs3HardresetKeyShiftRegHigh
		ps3HardresetKeyShiftRegHigh;
	union HilReg0Ps3RegisterFPs3HardresetTimeCnt ps3HardresetTimeCnt;
	union HilReg0Ps3RegisterFPs3HardresetTimeOutEn ps3HardresetTimeOutEn;
	union HilReg0Ps3RegisterFPs3KeyGapCfg ps3KeyGapCfg;
	union HilReg0Ps3RegisterFPs3HardresetIrqClear ps3HardresetIrqClear;
	union HilReg0Ps3RegisterFPs3HardresetIrqMask ps3HardresetIrqMask;
	unsigned long long reserved3[22];
	union HilReg0Ps3RegisterFPs3SocFwState ps3SocFwState;
	union HilReg0Ps3RegisterFPs3MaxFwCmd ps3MaxFwCmd;
	union HilReg0Ps3RegisterFPs3MaxChainSize ps3MaxChainSize;
	union HilReg0Ps3RegisterFPs3MaxVdInfoSize ps3MaxVdInfoSize;
	union HilReg0Ps3RegisterFPs3MaxNvmePageSize ps3MaxNvmePageSize;
	union HilReg0Ps3RegisterFPs3FeatureSupport ps3FeatureSupport;
	union HilReg0Ps3RegisterFPs3FirmwareVersion ps3FirmwareVersion;
	union HilReg0Ps3RegisterFPs3MaxReplyque ps3MaxReplyque;
	union HilReg0Ps3RegisterFPs3HardwareVersion ps3HardwareVersion;
	union HilReg0Ps3RegisterFPs3MgrQueueDepth ps3MgrQueueDepth;
	union HilReg0Ps3RegisterFPs3CmdQueueDepth ps3CmdQueueDepth;
	union HilReg0Ps3RegisterFPs3TfifoDepth ps3TfifoDepth;
	union HilReg0Ps3RegisterFPs3MaxSecR1xCmds ps3MaxSecR1xCmds;
	unsigned long long reserved4[19];
	union HilReg0Ps3RegisterFPs3HilAdvice2directCnt0 ps3HilAdvice2directCnt0;
	union HilReg0Ps3RegisterFPs3HilAdvice2directCnt1 ps3HilAdvice2directCnt1;
	union HilReg0Ps3RegisterFPs3HilAdvice2directCnt2 ps3HilAdvice2directCnt2;
	union HilReg0Ps3RegisterFPs3HilAdvice2directCnt3 ps3HilAdvice2directCnt3;
	union HilReg0Ps3RegisterFPs3HilAdvice2directCntAll
		ps3HilAdvice2directCntAll;
	unsigned long long reserved5[3];
	union HilReg0Ps3RegisterFPs3IrqStatusRpt ps3IrqStatusRpt;
	unsigned long long reserved6[23];
	union HilReg0Ps3RegisterFPs3DumpCtrl ps3DumpCtrl;
	union HilReg0Ps3RegisterFPs3DumpCtrlIrqClear ps3DumpCtrlIrqClear;
	union HilReg0Ps3RegisterFPs3DumpCtrlIrqMask ps3DumpCtrlIrqMask;
	union HilReg0Ps3RegisterFPs3DumpStatus ps3DumpStatus;
	union HilReg0Ps3RegisterFPs3DumpDataSize ps3DumpDataSize;
	unsigned long long reserved7[27];
	union HilReg0Ps3RegisterFPs3CmdTrigger ps3CmdTrigger;
	union HilReg0Ps3RegisterFPs3CmdTriggerIrqClear ps3CmdTriggerIrqClear;
	union HilReg0Ps3RegisterFPs3CmdTriggerIrqMask ps3CmdTriggerIrqMask;
	union HilReg0Ps3RegisterFPs3SoftresetCounter ps3SoftresetCounter;
	union HilReg0Ps3RegisterFPs3RegCmdState ps3RegCmdState;
	union HilReg0Ps3RegisterFPs3Debug0 ps3Debug0;
	union HilReg0Ps3RegisterFPs3Debug0IrqClear ps3Debug0IrqClear;
	union HilReg0Ps3RegisterFPs3Debug0IrqMask ps3Debug0IrqMask;
	union HilReg0Ps3RegisterFPs3Debug1 ps3Debug1;
	union HilReg0Ps3RegisterFPs3Debug1IrqClear ps3Debug1IrqClear;
	union HilReg0Ps3RegisterFPs3Debug1IrqMask ps3Debug1IrqMask;
	union HilReg0Ps3RegisterFPs3Debug2 ps3Debug2;
	union HilReg0Ps3RegisterFPs3Debug2IrqClear ps3Debug2IrqClear;
	union HilReg0Ps3RegisterFPs3Debug2IrqMask ps3Debug2IrqMask;
	union HilReg0Ps3RegisterFPs3Debug3 ps3Debug3;
	union HilReg0Ps3RegisterFPs3Debug3IrqClear ps3Debug3IrqClear;
	union HilReg0Ps3RegisterFPs3Debug3IrqMask ps3Debug3IrqMask;
	union HilReg0Ps3RegisterFPs3Debug4 ps3Debug4;
	union HilReg0Ps3RegisterFPs3Debug4IrqClear ps3Debug4IrqClear;
	union HilReg0Ps3RegisterFPs3Debug4IrqMask ps3Debug4IrqMask;
	union HilReg0Ps3RegisterFPs3Debug5 ps3Debug5;
	union HilReg0Ps3RegisterFPs3Debug6 ps3Debug6;
	union HilReg0Ps3RegisterFPs3Debug7 ps3Debug7;
	union HilReg0Ps3RegisterFPs3Debug8 ps3Debug8;
	union HilReg0Ps3RegisterFPs3Debug9 ps3Debug9;
	union HilReg0Ps3RegisterFPs3Debug10 ps3Debug10;
	union HilReg0Ps3RegisterFPs3Debug11 ps3Debug11;
	union HilReg0Ps3RegisterFPs3Debug12 ps3Debug12;
	unsigned long long reserved8[4];
	union HilReg0Ps3RegisterFPs3SessioncmdAddr ps3SessioncmdAddr;
	union HilReg0Ps3RegisterFPs3SessioncmdAddrIrqClear
		ps3SessioncmdAddrIrqClear;
	union HilReg0Ps3RegisterFPs3SessioncmdAddrIrqMask
		ps3SessioncmdAddrIrqMask;
};
#endif
#endif
