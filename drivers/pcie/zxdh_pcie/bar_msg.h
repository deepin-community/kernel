#ifndef _ZXDH_HPF_MSG_CHAN_PUB_H_
#define _ZXDH_HPF_MSG_CHAN_PUB_H_

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
#include <linux/dinghai/dh_cmd.h>

#define PCIE_FUNC_HP_EVENT_ID       44
#define BUF_SIZE            0x1000

/* bar消息通道使用pf信息 */
#define HP_BAR_MSG_OFFSET   0x2000
#define HP_VENDOR_ID        0x1cf2
#define HP_DEVICE_ID        0x8031
// #define HP_VENDOR_ID        0x16c3
// #define HP_DEVICE_ID        0
#define HP_IOREMAP_SIZE     0x3000

/* data type */
typedef unsigned long long int u64;
typedef signed long long int s64;

typedef unsigned int u32;
typedef signed int s32;

typedef unsigned short int u16;
typedef signed short int s16;

typedef unsigned char u8;
typedef signed char s8;

/* printk level */


/*
 * The interface for communication among HOST, RISC-V and ZF drivers
 * is as follows...
 *
 * A. COMMUNICATION THROUGH BAR CHANNEL
 *
 *
 * B. COMMUNICATION THROUGH PKT CHANNEL
 *
 * Make sure you have allocated private queues and MSI-X interrupt for them.
 * Then set callback of the vector with virtnet_poll_private().
 * Choose the proper paramaters and fill them in the zxdh_pkt_chan_msg_send().
 * Enjoying communicating with others whenever you want.
 */

/* Value that the zxdh_pkt_chan_msg_send() can be returned */
#define MSG_CHAN_RET_OK                       0
#define MSG_CHAN_RET_ERR_NULL_PTR             (-1)
#define MSG_CHAN_RET_ERR_INVALID_PARA         (-2)
#define MSG_CHAN_RET_ERR_NO_ENOUGH_MEM        (-4)
#define MSG_CHAN_RET_ERR_CHANNEL_NOT_READY    (-5)
#define MSG_CHAN_RET_ERR_CHAN_BUSY            (-6)
#define MSG_CHAN_RET_ERR_CHAN_BROKEN          (-7)
#define MSG_CHAN_RET_ERR_XMIT_FAIL            (-8)
#define MSG_CHAN_RET_ERR_CALLBACK_OUT_OF_TIME (-9)
#define MSG_CHAN_RET_ERR_NO_PRIV_QUEUE        (-10)
#define MSG_CHAN_RET_ERR_CALLBACK_FAIL        (-11)
#define MSG_CHAN_RET_ERR_FREEPAGE_FAIL        (-12)

typedef enum {
    TYPE_DEBUG = 0,
    DST_RISCV,
    DST_MPF,
    DST_PF_OR_VF,
    DST_ZF,
    MSG_TYPE_NUM,
} MSG_TYPE;

int hpf_send_msg_to_riscv(void *msg_info, u32 msg_size, void *resp_msg, u32 resp_size, struct pci_dev *pdev);

#ifdef __cplusplus
}
#endif

#endif
