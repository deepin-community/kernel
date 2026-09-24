#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/termios.h>
#include "tod_driver.h"
//#include "../msg_chan_driver/msg_chan_pub.h"
#include <linux/dinghai/dh_cmd.h>
#include <linux/dinghai/kcompat.h>
#include "zxdh_ptp_common.h"

#define DEVICE_NUM   3
#define TOD_AGENT_NAME_LEN    15
#define TOD_DEVICE_NAME             "tod-dev"
#define TOD_DEVICE_CLASS            "tod_class"

static uint64_t virt_addr = 0;
static uint64_t pcie_id = 0;
static dev_t tod_device_no = 0;
static struct class  *tod_device_class = NULL;


struct tod_device
{
    struct cdev tod_cdev;
    // gps:"/dev/ttyAMA1", recv tod: "/dev/ttyAMA2", send tod: "/dev/ttyAMA3"
    char tod_agent_name[TOD_AGENT_NAME_LEN];
    struct file *tod_device_file;
};

struct tod_device tod_dev_array[DEVICE_NUM];

int32_t tod_device_set_bar_virtual_addr(uint64_t virtaddr, uint16_t pcieid)
{
    virt_addr = virtaddr;
    pcie_id = pcieid;
    PTP_LOG_INFO("%s: bar msg virtaddr: 0x%llx\n", __FUNCTION__, virtaddr);
    return 0;
}
EXPORT_SYMBOL(tod_device_set_bar_virtual_addr);

static int32_t tod_device_sync_msg_send(uint8_t *req, uint32_t req_len, uint8_t *resp, uint32_t resp_len)
{
    int32_t  result = 0;
    uint32_t payload_len = 0;
    struct zxdh_pci_bar_msg in = {0};
    struct zxdh_msg_recviver_mem out = {0};

    if (req == NULL || req_len < 4)
    {
        PTP_LOG_ERR("%s: arg invalid, req: %px, req_len: %u.\n", __FUNCTION__, req, req_len);
        return -EINVAL;
    }

    out.buffer_len = 4 + 4 + resp_len; // 4B 消息头 + 4B result + resp_len实际应答消息
    out.recv_buffer = (uint8_t*)kmalloc(out.buffer_len, GFP_KERNEL);
    if (out.recv_buffer == NULL)
    {
        PTP_LOG_ERR("%s: no space left on device.\n", __FUNCTION__);
        return -ENOSPC;
    }
    memset(out.recv_buffer, 0, out.buffer_len);

    in.virt_addr = virt_addr;
    in.event_id  = MODULE_TOD;
    in.src = MSG_CHAN_END_PF;
    in.dst = MSG_CHAN_END_RISC;
    in.payload_addr = req;
    in.payload_len  = req_len;
    in.src_pcieid = pcie_id;

    if(zxdh_bar_chan_sync_msg_send(&in, &out) != BAR_MSG_OK)
    {
        kfree(out.recv_buffer);
        PTP_LOG_ERR("%s: zxdh_bar_chan_sync_msg_send failed.\n", __FUNCTION__);
        return -EINVAL;
    }

    // 消息应答格式: header(4B) + payload(nB), payload最大2048 - 12
    // header格式: 0xFF(1B) + payload_len(2B) + rsv(1B)
    // payload格式: result(4B) + msg((n - 4)B)
    payload_len = *(uint16_t*)((uint8_t*)out.recv_buffer + 1);
    if (payload_len < 4)
    {
        kfree(out.recv_buffer);
        PTP_LOG_ERR("%s: payload_len: %u check failed.\n", __FUNCTION__, payload_len);
        return -EINVAL;
    }

    result = *(int32_t*)((uint8_t*)out.recv_buffer + 4);
    if (result != 0)
    {
        kfree(out.recv_buffer);
        PTP_LOG_ERR("%s: result: %d check failed.\n", __FUNCTION__, result);
        return result;
    }

    if (payload_len > 4 && resp != NULL)
    {
        memcpy(resp, out.recv_buffer + 8, (((payload_len - 4) > resp_len) ? resp_len : (payload_len - 4)));
    }

    kfree(out.recv_buffer);

    return 0;
}

static int tod_device_open(struct inode *inode, struct file *file)
{
    int32_t result = 0;
    struct tod_device_msg msg;
    struct tod_device *tod;
    file->private_data = (void *)(container_of(inode->i_cdev, struct tod_device, tod_cdev));
    tod = (struct tod_device *)file->private_data;

    PTP_LOG_INFO("%s. dev_name: %s\n", __FUNCTION__, (tod->tod_agent_name));

    if (tod->tod_device_file != NULL)
    {
        PTP_LOG_INFO("%s: device already open.\n", __FUNCTION__);
        return 0;
    }

    memset(&msg, 0x00, sizeof(struct tod_device_msg));
    msg.type = TOD_DEVICE_MSG_OPEN;
    memcpy(msg.data, tod->tod_agent_name, strlen(tod->tod_agent_name) + 1);

    result = tod_device_sync_msg_send((uint8_t*)(&msg), sizeof(struct tod_device_msg), (uint8_t*)(&tod->tod_device_file), sizeof(struct file*));
    if (result != 0)
    {
        tod->tod_device_file = NULL;
        PTP_LOG_ERR("%s: tod_device_sync_msg_send failed, result: %d.\n", __FUNCTION__, result);
        return -EINVAL;
    }
    PTP_LOG_INFO("%s: file %px open success.\n", __FUNCTION__, tod->tod_device_file);

    return 0;
}

static int tod_device_release(struct inode *inode, struct file *file)
{
    int32_t result = 0;
    struct tod_device_msg msg;
    struct tod_device *tod;

    tod = (struct tod_device *)file->private_data;

    PTP_LOG_INFO("%s  tod_agent_name: %s.\n", __FUNCTION__, tod->tod_agent_name);

    if (tod->tod_device_file == NULL)
    {
        PTP_LOG_ERR("%s: device already close.\n", __FUNCTION__);
        return 0;
    }

    memset(&msg, 0x00, sizeof(struct tod_device_msg));
    msg.type = TOD_DEVICE_MSG_CLOSE;
    msg.file = tod->tod_device_file;

    result = tod_device_sync_msg_send((uint8_t*)(&msg), sizeof(struct tod_device_msg), NULL, 0);
    if (result != 0)
    {
        PTP_LOG_ERR("%s: tod_device_sync_msg_send failed, result: %d.\n", __FUNCTION__, result);
        return -EINVAL;
    }
    PTP_LOG_INFO("%s: file %px close success.\n", __FUNCTION__, tod->tod_device_file);
    tod->tod_device_file = NULL;

    return 0;
}

static ssize_t tod_device_read(struct file *file, char *buf, size_t count, loff_t *f_pos)
{
    int32_t result = 0;
    uint8_t *resp = NULL;
    struct tod_device_msg msg;
    struct tod_device *tod;

    tod = (struct tod_device *)file->private_data;

    PTP_LOG_INFO("%s  tod_agent_name: %s.\n", __FUNCTION__, tod->tod_agent_name);

    if (tod->tod_device_file == NULL)
    {
        PTP_LOG_ERR("%s: no such device.\n", __FUNCTION__);
        return -ENODEV;
    }

    if (count > (2048 - 12 - sizeof(int32_t) - sizeof(size_t))) // common bar: 2048 - 12, result: 4, count: 8.
    {
        PTP_LOG_ERR("%s: no space left on device.\n", __FUNCTION__);
        return -ENOSPC;
    }

    resp = (uint8_t*)kmalloc(sizeof(size_t) + count, GFP_KERNEL);
    if (resp == NULL)
    {
        PTP_LOG_ERR("%s: no space left on device.\n", __FUNCTION__);
        return -ENOSPC;
    }

    memset(&msg, 0x00, sizeof(struct tod_device_msg));
    msg.type = TOD_DEVICE_MSG_READ;
    msg.count = count;
    msg.file = tod->tod_device_file;

    result = tod_device_sync_msg_send((uint8_t*)(&msg), sizeof(struct tod_device_msg), resp, sizeof(size_t) + count);
    if (result != 0)
    {
        kfree(resp);
        PTP_LOG_ERR("%s: tod_device_sync_msg_send failed, result: %d.\n", __FUNCTION__, result);
        return -EINVAL;
    }

    count = *((size_t*)(resp));
    if (count > msg.count)
    {
        PTP_LOG_ERR("%s: no space left on device.\n", __FUNCTION__);
        kfree(resp);
        return -ENOSPC;
    }

    result = copy_to_user(buf, resp + sizeof(size_t), count);
    if (result != 0)
    {
        kfree(resp);
        PTP_LOG_ERR("%s: copy_to_user failed, result: %d.\n", __FUNCTION__, result);
        return -EINVAL;
    }

    kfree(resp);
    PTP_LOG_INFO("%s: file %px read %lu bytes success.\n", __FUNCTION__, tod->tod_device_file, count);

    return count;
}

static ssize_t tod_device_write(struct file *file, const char *buf, size_t count, loff_t *f_pos)
{
    int32_t result = 0;
    struct tod_device_msg msg;
    struct tod_device *tod;

    tod = (struct tod_device *)file->private_data;

    PTP_LOG_INFO("%s  tod_agent_name: %s.\n", __FUNCTION__, tod->tod_agent_name);

    if (tod->tod_device_file == NULL)
    {
        PTP_LOG_ERR("%s: no such device.\n", __FUNCTION__);
        return -ENODEV;
    }

    if (count > sizeof(msg.data))
    {
        PTP_LOG_ERR("%s: no space left on device.\n", __FUNCTION__);
        return -ENOSPC;
    }

    memset(&msg, 0x00, sizeof(struct tod_device_msg));
    msg.type = TOD_DEVICE_MSG_WRITE;
    msg.count = count;
    msg.file = tod->tod_device_file;
    result = copy_from_user(msg.data, buf, count);
    if (result != 0)
    {
        PTP_LOG_ERR("%s: copy_from_user failed, result: %d.\n", __FUNCTION__, result);
        return -EINVAL;
    }

    result = tod_device_sync_msg_send((uint8_t*)(&msg), sizeof(struct tod_device_msg), (uint8_t*)(&count), sizeof(size_t));
    if (result != 0)
    {
        PTP_LOG_ERR("%s: tod_device_sync_msg_send failed, result: %d.\n", __FUNCTION__, result);
        return -EINVAL;
    }
    PTP_LOG_INFO("%s: file %px write %lu bytes success.\n", __FUNCTION__, tod->tod_device_file, count);

    return count;
}

#ifdef CGS_V5_693
/* CGS_V5_693 所在的 3.10 内核尚未引入 __poll_t，file_operations.poll
 * 的原型为 unsigned int (*poll)(struct file *, struct poll_table_struct *)。
 */
static unsigned int tod_device_poll(struct file *file, struct poll_table_struct *wait)
#else
static __poll_t tod_device_poll(struct file *file, struct poll_table_struct *wait)
#endif
{
    int32_t result = 0;
    uint16_t poll_mask = 0;
    struct tod_device_msg msg;
    struct tod_device *tod;

    tod = (struct tod_device *)file->private_data;
    PTP_LOG_INFO("%s  tod_agent_name: %s.\n", __FUNCTION__, tod->tod_agent_name);

    if (tod->tod_device_file == NULL)
    {
        PTP_LOG_ERR("%s: no such device.\n", __FUNCTION__);
        return POLLERR;
    }

    memset(&msg, 0x00, sizeof(struct tod_device_msg));
    msg.type = TOD_DEVICE_MSG_POLL;
    msg.file = tod->tod_device_file;

    result = tod_device_sync_msg_send((uint8_t*)(&msg), sizeof(struct tod_device_msg), (uint8_t*)(&poll_mask), sizeof(uint16_t));
    if (result != 0)
    {
        PTP_LOG_ERR("%s: tod_device_sync_msg_send failed, result: %d.\n", __FUNCTION__, result);
        return POLLERR;
    }
    PTP_LOG_INFO("%s: file %px poll mask 0x%04x success.\n", __FUNCTION__, tod->tod_device_file, poll_mask);

    return poll_mask;
}

static long tod_device_ioctl(struct file *file, uint32_t request, unsigned long args)
{
    int32_t result = 0;
    uint8_t *resp = NULL;
    struct tod_device_msg msg;
    struct tod_device *tod;

    tod = (struct tod_device *)file->private_data;

    PTP_LOG_INFO("%s  tod_agent_name: %s.\n", __FUNCTION__, tod->tod_agent_name);

    if (tod->tod_device_file == NULL)
    {
        PTP_LOG_ERR("%s: no such device.\n", __FUNCTION__);
        return -ENODEV;
    }

    resp = (uint8_t*)kmalloc(sizeof(struct termios), GFP_KERNEL);
    if (resp == NULL)
    {
        PTP_LOG_ERR("%s: no space left on device.\n", __FUNCTION__);
        return -ENOSPC;
    }

    memset(&msg, 0x00, sizeof(struct tod_device_msg));
    msg.type = TOD_DEVICE_MSG_IOCTL;
    msg.file = tod->tod_device_file;
    msg.command = request;

    if ((struct termios*)args != NULL)
    {
        result = copy_from_user(msg.data, (uint8_t*)args, sizeof(struct termios));
        if (result != 0)
        {
            PTP_LOG_ERR("%s: copy_from_user failed, result: %d.\n", __FUNCTION__, result);
            kfree(resp);
            return -EINVAL;
        }
    }

    result = tod_device_sync_msg_send((uint8_t*)(&msg), sizeof(struct tod_device_msg), resp, sizeof(struct termios));
    if (result != 0)
    {
        kfree(resp);
        PTP_LOG_ERR("%s: tod_device_sync_msg_send failed, result: %d.\n", __FUNCTION__, result);
        return -EINVAL;
    }

    if ((struct termios*)args != NULL)
    {
        result = copy_to_user((uint8_t*)args, resp, sizeof(struct termios));
        if (result != 0)
        {
            kfree(resp);
            PTP_LOG_ERR("%s: copy_to_user failed, result: %d.\n", __FUNCTION__, result);
            return -EINVAL;
        }
    }

    kfree(resp);
    PTP_LOG_INFO("%s: file %px ioctl success.\n", __FUNCTION__, tod->tod_device_file);

    return 0;
}

struct file_operations tod_device_ops = {
    .owner          = THIS_MODULE,
    .open           = tod_device_open,
    .release        = tod_device_release,
    .read           = tod_device_read,
    .write          = tod_device_write,
    .poll           = tod_device_poll,
    .unlocked_ioctl = tod_device_ioctl
};

static int32_t __init tod_device_init(void)
{
    int32_t result = 0;
    int32_t i = 0;
    int32_t j = 0;
    struct device *dev = NULL;

    result = alloc_chrdev_region(&tod_device_no, 0, DEVICE_NUM, TOD_DEVICE_NAME);
    if (result < 0)
    {
        PTP_LOG_ERR("%s: alloc_chrdev_region failed, result: %d.\n", __FUNCTION__, result);
        return -EINVAL;
    }

    for(i = 0; i < DEVICE_NUM; i++)
    {
        // 给 struct cdev对象制定操作函数集
        cdev_init(&tod_dev_array[i].tod_cdev, &tod_device_ops);
        tod_dev_array[i].tod_cdev.owner = THIS_MODULE;
        snprintf(tod_dev_array[i].tod_agent_name, TOD_AGENT_NAME_LEN, "/dev/ttyAMA%d", i+1);

        // 将struct cdev对象添加到内核对应的数据结构里
        result = cdev_add(&tod_dev_array[i].tod_cdev, MKDEV(MAJOR(tod_device_no), MINOR(tod_device_no) + i), 1);
        if (result != 0)
        {
            PTP_LOG_ERR("%s: cdev_add failed, result: %d.\n", __FUNCTION__, result);
            if(i > 0)
            {
                for(j = i-1; j >= 0; j--)
                {
                    cdev_del(&tod_dev_array[j].tod_cdev);
                }
            }
            goto cdev_add_fail;
        }
    }
#if defined(HAVE_NO_MODULE_PARAM) || defined(CLASS_CREATE_NO_MODULE)
    tod_device_class = class_create(TOD_DEVICE_CLASS);
#else
    tod_device_class = class_create(THIS_MODULE, TOD_DEVICE_CLASS);
#endif
    if (IS_ERR(tod_device_class))
    {
        PTP_LOG_ERR("%s: class_create failed, err: %lu.\n", __FUNCTION__, PTR_ERR(tod_device_class));

        goto class_create_fail;
    }

    for(i = 0; i < DEVICE_NUM; i++)
    {
        dev = device_create(tod_device_class, NULL, MKDEV(MAJOR(tod_device_no), MINOR(tod_device_no) + i), NULL, "tod_device%d", i);
        if (IS_ERR(dev))
        {
            PTP_LOG_ERR("%s: device_create failed, err: %lu.\n", __FUNCTION__, PTR_ERR(dev));
            if(i > 0)
            {
                for(j = i-1; j >= 0; j--)
                {
                    device_destroy(tod_device_class, MKDEV(MAJOR(tod_device_no), MINOR(tod_device_no) + j));
                }
            }

            goto device_create_fail;
        }
    }

    return 0;


device_create_fail:
    class_destroy(tod_device_class);
class_create_fail:
    for(i = 0; i < DEVICE_NUM; i++)
    {
        cdev_del(&tod_dev_array[i].tod_cdev);
    }
cdev_add_fail:
    unregister_chrdev_region(tod_device_no, DEVICE_NUM);
    return -EINVAL;
}

static void __exit tod_device_exit(void)
{
    int32_t i;
    PTP_LOG_INFO("%s: start.\n", __FUNCTION__);

    for(i = 0; i < DEVICE_NUM; i++)
    {
        device_destroy(tod_device_class, MKDEV(MAJOR(tod_device_no), MINOR(tod_device_no) + i));
    }

    class_destroy(tod_device_class);

    for(i = 0; i < DEVICE_NUM; i++)
    {
        cdev_del(&tod_dev_array[i].tod_cdev);
    }

    unregister_chrdev_region(tod_device_no, DEVICE_NUM);

    PTP_LOG_INFO("%s: success.\n", __FUNCTION__);
}

module_init(tod_device_init);
module_exit(tod_device_exit);

MODULE_LICENSE("GPL");
