#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/err.h>
#include <linux/dinghai/dh_cmd.h>
#include "zf_mpf.h"

#define ZXDH_SYSFS_DIR "zxdh_host_reset"
#define ZXDH_SYSFS_FILE_EP_CHECK_REGISTER "ep_check_register"
#define ZXDH_SYSFS_FILE_EP_RESET_INFO "ep_reset_info"

struct zxdh_reset_dev
{
    uint16_t pcie_id;
    uint64_t bar0_base_virt_addr;
    uint64_t is_valid;
} reset_dev = {0};

struct zxdh_reset_dev *zxdh_get_reset_dev(void)
{
    return &reset_dev;
}

int zxdh_init_reset_dev(struct dh_core_dev *core_dev)
{
    struct zxdh_reset_dev *dev = zxdh_get_reset_dev();

    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(core_dev);
    dev->bar0_base_virt_addr = mpf_dev->pci_ioremap_addr;
    dev->is_valid = 1;
    dev->pcie_id = mpf_dev->pcie_id;
    return 0;
}

extern int zxdh_bar_chan_sync_msg_send(struct zxdh_pci_bar_msg *in, struct zxdh_msg_recviver_mem *result);
extern int zxdh_bar_chan_msg_recv_register(uint8_t event_id, zxdh_bar_chan_msg_recv_callback callback);
extern int zxdh_bar_chan_msg_recv_unregister(uint8_t event_id);
extern int pcie_zte_zf_signal_epc_dev_init(uint32_t ep_idx);

enum e_reset_event
{
    EV_HOST_RESET = 0,
    EV_ZF_RESET = 1,
    EV_DINGHAI_RESET = 2,
    EV_ZXDH_RESET_TEST = 3,
    EV_MAX_RESET
};

struct host_reset_ev_info
{
    int ep_no;
};

struct zxdh_reset_priv
{
    enum e_reset_event e_reset_event;
    union
    {
        struct host_reset_ev_info host_reset_ev;
    } ev_info;
    struct work_struct work;
};

/* 本模块在ZF中insmod

使用sysfs接口，主目录：/sys/zxdh_host_reset/，包含下述文件
1、ep_check_register
功能：业务通知risc-v需要进行检测的ep
参数：16进制的字符串。例子：0xffffffff
字符串转为4个字节长度的数值，每个bit位代表一个ep。bit0~bit31代表ep0~ep31。
当bit位设置为1时，表示该ep需要进行复位检测上报。
当bit位设置为0时，表示该ep不需要进行复位检测上报。
用户操作：
1）向  ep_check_register  写入 value
   含义：通知risc-v需要进行检测的ep
2）读取ep_check_register
   返回value，表示当前监测的ep
2、ep_reset_info
功能：ep复位后，业务通过这个文件查询
参数：16进制的字符串。例子：0xffffffff
字符串转为4个字节长度的数值，每个bit位代表一个ep。bit0~bit31代表ep0~ep31。
当bit位为1时，表示该ep发生了reset。
当bit位为0时，表示该ep没有发生reset。
用户操作：
1）读取ep_reset_info
   返回value，获取发生reset的ep信息
2）向  ep_reset_info  写入 value
   将发生reset的ep信息清0
*/
unsigned int ep_check_register = 0;
unsigned int ep_reset_info = 0;

struct kobject *kobj_zxdh_host_reset = NULL;

/*该函数被调用在sysfs文件被读时*/
static ssize_t sysfs_show_ep_check_register(struct kobject *kobj,
                                            struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%08x", ep_check_register);
}

/* 该函数被调用在sysfs文件被写时*/
static ssize_t sysfs_store_ep_check_register(struct kobject *kobj,
                                             struct kobj_attribute *attr, const char *buf, size_t count)
{
    uint16_t ret = 0;

    struct zxdh_pci_bar_msg in = {0};
    struct zxdh_msg_recviver_mem result = {0};
    uint8_t recv_buffer[20] = {0};
    uint16_t recv_buff_len = 20;
    struct zxdh_reset_dev *dev = zxdh_get_reset_dev();

    if (sscanf(buf, "0x%x", &ep_check_register) != 1)
    {
        return ret;
    }

    in.virt_addr = dev->bar0_base_virt_addr + ZXDH_BAR1_CHAN_OFFSET;
    in.payload_addr = &ep_check_register;
    in.payload_len = sizeof(ep_check_register);
    in.src = MSG_CHAN_END_PF; // MSG_CHAN_END_MPF;
    in.dst = MSG_CHAN_END_RISC;
    in.event_id = MODULE_RESET_MSG; /* 事件号 */
    in.src_pcieid = dev->pcie_id;

    result.recv_buffer = recv_buffer;
    result.buffer_len = recv_buff_len;

    ret = zxdh_bar_chan_sync_msg_send(&in, &result);
    if (ret)
    {
        DH_LOG_ERR(MODULE_MPF, "  '%s' zxdh_bar_chan_sync_msg_send failed.\n", __FUNCTION__);
    }
    return count;
}

/*该函数被调用在sysfs文件被读时*/
static ssize_t sysfs_show_ep_reset_info(struct kobject *kobj,
                                        struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%08x", ep_reset_info);
}

/* 该函数被调用在sysfs文件被写时*/
static ssize_t sysfs_store_ep_reset_info(struct kobject *kobj,
                                         struct kobj_attribute *attr, const char *buf, size_t count)
{
    if (sscanf(buf, "0x%x", &ep_reset_info) == 1)
    {
        return (ssize_t)count;
    }
    return 0;
}

/*使用__ATTR宏初始化zxdh_host_reset_attr结构体，该宏定义在include/linux/sysfs.h*/
struct kobj_attribute zxdh_host_reset_attr_ep_check_register = __ATTR(ep_check_register, 0664, sysfs_show_ep_check_register, sysfs_store_ep_check_register);
struct kobj_attribute zxdh_host_reset_attr_ep_reset_info = __ATTR(ep_reset_info, 0664, sysfs_show_ep_reset_info, sysfs_store_ep_reset_info);

int32_t zxdh_reset_zf_rec_risc(void *pay_load, uint16_t len, void *reps_buffer, uint16_t *reps_len, void *dev)
{
    struct zxdh_reset_priv *priv = NULL;
    uint32_t ep_no = 0;
    uint32_t ep_reset_info_tmp = ep_reset_info;

    if (pay_load && len && reps_buffer && reps_len)
    {
        DH_LOG_INFO(MODULE_MPF, "%s: para check ok ok ok.(%p, %x, %p, %p)\n", __func__, pay_load, len, reps_buffer, reps_len);
    }
    else
    {
        DH_LOG_ERR(MODULE_MPF, "%s: para error.(%p, %x, %p, %p)\n", __func__, pay_load, len, reps_buffer, reps_len);
        return (uint16_t)-1;
    }

    priv = pay_load;
    ep_no = priv->ev_info.host_reset_ev.ep_no;
    pcie_zte_zf_signal_epc_dev_init(ep_no);     // 此函数只能放到中断的下半部

    DH_LOG_INFO(MODULE_MPF, "  %s: received msg: event_id[%d] len 0x%x, ep_no=%u\n", __FUNCTION__, MODULE_RESET_MSG, len, ep_no);

    ep_reset_info |= 1 << ep_no;
    DH_LOG_INFO(MODULE_MPF, "  %s: ep_reset_info 0x%08x -> 0x%08x\n", __FUNCTION__, ep_reset_info_tmp, ep_reset_info);

    return 0;
}

/*模块初始化函数*/
int zxdh_host_reset_driver_init(struct dh_core_dev *core_dev)
{
    int32_t ret = 0;
    /*创建一个目录在/sys下 */
    kobj_zxdh_host_reset = kobject_create_and_add(ZXDH_SYSFS_DIR, NULL);

    zxdh_init_reset_dev(core_dev);

    /*在ZXDH_SYSFS_DIR目录下创建文件*/
    if (sysfs_create_file(kobj_zxdh_host_reset, &zxdh_host_reset_attr_ep_check_register.attr))
    {
        DH_LOG_ERR_DEV(MODULE_MPF, core_dev, "  'ep_check_register' sysfs create failed.\n");
        goto error_sysfs;
    }

    if (sysfs_create_file(kobj_zxdh_host_reset, &zxdh_host_reset_attr_ep_reset_info.attr))
    {
        DH_LOG_ERR_DEV(MODULE_MPF, core_dev, "  'ep_reset_info' sysfs create failed.\n");
        goto error_sysfs;
    }

    ret = zxdh_bar_chan_msg_recv_register(MODULE_RESET_MSG, zxdh_reset_zf_rec_risc);
    if (ret != 0)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, core_dev, "  zxdh_bar_chan_msg_recv_register: event_id[%d] register failed: %d\n",
            MODULE_RESET_MSG, ret);
        // return ret;
    }

    DH_LOG_INFO_DEV(MODULE_MPF, core_dev, "  zxdh host reset module init ok.\n");
    return 0;

error_sysfs:
    zxdh_bar_chan_msg_recv_unregister(MODULE_RESET_MSG);
    sysfs_remove_file(kernel_kobj, &zxdh_host_reset_attr_ep_reset_info.attr);
    sysfs_remove_file(kernel_kobj, &zxdh_host_reset_attr_ep_check_register.attr);
    kobject_put(kobj_zxdh_host_reset);
    return -1;
}

/*模块退出函数*/
void zxdh_host_reset_driver_exit(struct dh_core_dev *core_dev)
{
    zxdh_bar_chan_msg_recv_unregister(MODULE_RESET_MSG);
    sysfs_remove_file(kernel_kobj, &zxdh_host_reset_attr_ep_reset_info.attr);
    sysfs_remove_file(kernel_kobj, &zxdh_host_reset_attr_ep_check_register.attr);
    kobject_put(kobj_zxdh_host_reset);
    DH_LOG_INFO_DEV(MODULE_MPF, core_dev, "  zxdh host reset module remove ok.\n");
}
#if 0
module_init(zxdh_host_reset_driver_init);
module_exit(zxdh_host_reset_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zxdh");
MODULE_DESCRIPTION("for zxdh host reset");
#endif
