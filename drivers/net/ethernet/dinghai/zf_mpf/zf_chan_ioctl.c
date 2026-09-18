#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/dinghai/dh_cmd.h>
#include "zf_chan_ioctl.h"

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

#define BAR_IOCTL_CMD_NORMAL           _IOW('a', 1, msg_entity_t)
#define BAR_IOCTL_CMD_SINGLE_DEV       _IOW('a', 2, pci_res_st)
#define BAR_IOCTL_CMD_ALL_DEV          _IOW('a', 3, pci_res_st)
#define BAR_IOCTL_CMD_SEND_REGISTER    _IOW('a', 4, msg_entity_t)
#define BAR_IOCTL_CMD_SEND_UNREGISTER  _IOW('a', 5, msg_entity_t)
#define BAR_IOCTL_CMD_RECV_MSG         _IOW('a', 6, msg_entity_t)

struct mpf_message_node
{
    struct normal_msg_entity msg;
    struct list_head node;
    uint16_t event_id;
};
struct mpf_wait_queue
{
    wait_queue_head_t wq;
    struct list_head msg_list;
    spinlock_t lock;
};
struct zxdh_bar_ioctl_dev
{
    uint16_t pcie_id;
    uint64_t bar0_base_virt_addr;
    struct mpf_wait_queue wait_queues;
}ioctl_dev = {0};

struct zxdh_bar_ioctl_dev *zxdh_get_bar_ioctl_dev(void)
{
    return &ioctl_dev;
}

int zxdh_init_bar_ioctl_resource(struct dh_core_dev *core_dev)
{
    struct zxdh_bar_ioctl_dev *dev = zxdh_get_bar_ioctl_dev();
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(core_dev);

    dev->pcie_id = mpf_dev->pcie_id;
    dev->bar0_base_virt_addr = mpf_dev->pci_ioremap_addr;

    spin_lock_init(&dev->wait_queues.lock);
    init_waitqueue_head(&dev->wait_queues.wq);
    INIT_LIST_HEAD(&(dev->wait_queues.msg_list));
    return 0;
}

static struct mpf_wait_queue* wait_queue_get(void)
{
    struct zxdh_bar_ioctl_dev *dev = zxdh_get_bar_ioctl_dev();

    return &dev->wait_queues;
}

void zxdh_remove_bar_ioctl_resource(struct dh_core_dev *core_dev)
{
    struct mpf_wait_queue *wait_queue = wait_queue_get();
    struct mpf_message_node *msg_node, *tmp;

    spin_lock(&wait_queue->lock);

    list_for_each_entry_safe(msg_node, tmp, &wait_queue->msg_list, node)
    {
        list_del(&msg_node->node);
        kfree(msg_node);
    }

    spin_unlock(&wait_queue->lock);
    return;
}

int info_usr_by_err_code(uint16_t event_id, unsigned long arg, uint16_t err_code)
{
    int ret = 0;
    struct normal_msg_entity *msg_to_usr = NULL;

    msg_to_usr = kzalloc(sizeof(struct normal_msg_entity), GFP_KERNEL);
    if (!msg_to_usr)
    {
        return -EFAULT;
    }
    msg_to_usr->hdr.recv_hdr_out.event_id = event_id;
    msg_to_usr->hdr.recv_hdr_out.state = err_code;
    if(copy_to_user((struct normal_msg_entity __user *)arg, msg_to_usr, sizeof(struct normal_msg_entity)))
    {
        ret = -EFAULT;
    }

    kfree(msg_to_usr);
    return ret;
}


static int zxdh_bar_recv_func_noop(void *pay_load, uint16_t len, void *reps_buffer, uint16_t *reps_len, void *dev)
{
    *reps_len = sizeof(uint16_t);
    return 0;
}

static uint16_t zxdh_err_code_convert(int bar_err_code)
{
    uint16_t err_code = 0;

    switch (bar_err_code)
    {
        case BAR_MSG_ERR_MODULE:
            err_code = IOCTRL_ERR_SEND_EVENTID_EXCCED;
            break;
        case BAR_MSG_ERR_REPEAT_REGISTER:
            err_code = IOCTRL_ERR_RECV_REPEAT_REGISTER;
            break;
        case BAR_MSG_ERR_UNGISTER:
            err_code = IOCTRL_ERR_RECV_NOT_REGISTER;
            break;
        default:
            err_code = IOCTRL_OK;
            break;
    }
    return err_code;
}

static uint16_t zxdh_fill_host_recv_func(uint16_t event_id)
{
    int bar_err_code = 0;

    bar_err_code = zxdh_bar_chan_msg_recv_register(event_id, zxdh_bar_recv_func_noop);
    return zxdh_err_code_convert(bar_err_code);
}

static int zxdh_strip_host_recv_func(uint16_t event_id)
{
    int bar_err_code = 0;

    bar_err_code = zxdh_bar_chan_msg_recv_unregister(event_id);
    return zxdh_err_code_convert(bar_err_code);
}

/********************************************************************
 * ioctl cmd
 * 1 bar_chan_ioctl_register_eventid     接收消息方注册event_id
 * 2 bar_chan_ioctl_unregister_eventid   接收消息方解注册event_id
 * 3 bar_chan_ioctl_recv_msg             接收消息方主动监听接收消息
 * 4 bar_chan_ioctl_send_msg             消息发送方发送同步消息
 * 5 bar_chan_ioctl_risc_send_simulation 模拟risc发送消息到wait_ques
 * 6 bar_chan_ioctl_pci_res_get_single   单个设备pcie资源查询
 * 7 bar_chan_ioctl_pci_res_get_all      全量设备pcie资源查询
 * *******************************************************************/
int bar_chan_ioctl_register_eventid(unsigned int cmd, unsigned long arg)
{
    uint16_t cmd_state = 0;
    uint16_t event_id = 0;

    if (copy_from_user(&event_id, (uint16_t __user *)arg, sizeof(uint16_t)))
    {
        LOG_ERR("MPF_IOCTL_REGISTER: copy_from_user failed\n");
        return -EFAULT;
    }

    cmd_state = zxdh_fill_host_recv_func(event_id);
    return info_usr_by_err_code(event_id, arg, cmd_state);
}

int bar_chan_ioctl_unregister_eventid(unsigned int cmd, unsigned long arg)
{
    uint16_t cmd_state = 0;
    uint16_t event_id = 0;

    if (copy_from_user(&event_id, (uint16_t __user *)arg, sizeof(uint16_t)))
    {
        LOG_ERR(KERN_ERR "MPF_IOCTL_REGISTER: copy_from_user failed\n");
        return -EFAULT;
    }

    cmd_state = zxdh_strip_host_recv_func(event_id);
    return info_usr_by_err_code(event_id, arg, cmd_state);;
}

/* 判断消息队列里面是佛有指定event_id消息*/
bool is_msg_list_contain_event_id(uint16_t event_id)
{
    bool found = false;
    unsigned long flags;
    struct mpf_message_node *node;
    struct mpf_wait_queue *wait_queue = wait_queue_get();

    spin_lock_irqsave(&wait_queue->lock, flags);

    list_for_each_entry(node, &wait_queue->msg_list, node)
    {
        if (node->event_id == event_id)
        {
            found = true;
            break;
        }
    }
    spin_unlock_irqrestore(&wait_queue->lock, flags);
    return found;
}

int bar_chan_ioctl_recv_msg(unsigned int cmd, unsigned long arg)
{
    int func_state = 0;
    uint16_t event_id;
    struct mpf_message_node *msg_ptr = NULL;
    struct mpf_message_node *found_msg = NULL;
    unsigned long flags;
    struct mpf_wait_queue *wait_queue = NULL;

    /* 1 用户态拷贝cmd中的event_id*/
    if (copy_from_user(&event_id, (uint16_t __user *)arg, sizeof(uint16_t)))
    {
        LOG_ERR(KERN_ERR "MPF_IOCTL_GET_MSG: copy_from_user failed\n");
        return -EFAULT;
    }

    /* 2 获得消息队列地址*/
    wait_queue = wait_queue_get();
    if (!wait_queue)
    {
        LOG_ERR( "wait_queue not found.\n");
        return info_usr_by_err_code(event_id, arg, IOCTRL_ERR_SEND_EVENTID_EXCCED);
    }

    /* 3 接收之前检查event_id是否注册*/
    func_state = zxdh_bar_callback_register_state(event_id);
    if (func_state != BAR_MSG_OK)
    {
        return info_usr_by_err_code(event_id, arg, zxdh_err_code_convert(func_state));
    }

    /* 4 消息list不含对应event_id消息则阻塞， 被唤醒之后重新判断*/
    wait_event_interruptible(wait_queue->wq,
                                is_msg_list_contain_event_id(event_id));

    /* 5 如果msg list中有对应event_id消息， 遍历链表， 从中取出来*/
    spin_lock_irqsave(&wait_queue->lock, flags);
    list_for_each_entry(msg_ptr, &wait_queue->msg_list, node)
    {
        if (msg_ptr->event_id == event_id)
        {
            list_del(&msg_ptr->node);//摘链表
            found_msg = msg_ptr;      // 保存指针，稍后处理
            break;
        }
    }
    spin_unlock_irqrestore(&wait_queue->lock, flags);

    if (!found_msg) {
        return -ENOENT;
    }

    if (copy_to_user((struct normal_msg_entity __user *)arg, found_msg, sizeof(struct normal_msg_entity)))
    {
        LOG_ERR(KERN_ERR "MPF_IOCTL_GET_MSG: copy_to_user failed\n");
        kfree(found_msg);
        return -EFAULT;
    }

    kfree(found_msg);

    return 0;
}

int bar_chan_common_sync_send(unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct normal_msg_entity *user_msg = NULL;
    struct zxdh_ioctl_send_in *send_paras = NULL;
    struct zxdh_pci_bar_msg in = {0};
    struct zxdh_msg_recviver_mem result = {0};
    struct zxdh_bar_ioctl_dev *dev = zxdh_get_bar_ioctl_dev();

    /* 按照既定的申请内存*/
    user_msg = kzalloc(sizeof(struct normal_msg_entity), GFP_KERNEL);
    if (NULL == user_msg)
    {
        LOG_ERR("malloc failed.\n");
        return -1;
    }
    /* 初始化并且按照约定的大小从用户态拷贝数据，保证不拷贝到脏数据*/
    if (copy_from_user(user_msg, (msg_entity_t __user *)arg, sizeof(struct normal_msg_entity)))
    {
        user_msg->hdr.send_hdr_out.ioctl_state = IOCTRL_ERR_MSG_GET;
        goto out;
    }

    send_paras = &user_msg->hdr.send_hdr_in;
    if (send_paras->pload_len > BAR_CHAN_PLOAD_SIZE)
    {
        user_msg->hdr.send_hdr_out.bar_state = IOCTRL_ERR_MSG_GET;
        goto out;
    }

    in.virt_addr = dev->bar0_base_virt_addr + ZXDH_BAR1_CHAN_OFFSET;
    in.payload_addr = user_msg->pload;
    in.payload_len = send_paras->pload_len;
    in.src = MSG_CHAN_END_PF;
    in.dst = MSG_CHAN_END_RISC;
    in.event_id = send_paras->event_id;
    in.src_pcieid = dev->pcie_id;

    /* pload直接透传上去*/
    result.buffer_len = BAR_CHAN_PLOAD_SIZE;
    result.recv_buffer = (void *)user_msg->pload;

    ret = zxdh_bar_send_without_reps_hdr(&in, &result);
    if (ret != 0)
    {
        LOG_ERR("pcie send msg failed, ret:%d.\n", ret);
    }

    user_msg->hdr.send_hdr_out.bar_state = ret;
    user_msg->hdr.send_hdr_out.ioctl_state = 0;
out:
    ret = copy_to_user((int *)arg, user_msg, sizeof(*user_msg));
    if (0 != ret)
    {
        /* msg may not reply to user, but we can locate by demsg*/
        LOG_ERR("reply ioctl msg failed, ret: %d.\n", ret);
    }
    /* free the user_msg_data*/
    if (NULL != user_msg)
    {
        kfree(user_msg);
    }
    return 0;
}

int bar_chan_pci_res_get(unsigned int cmd, unsigned long arg, uint16_t mode)
{
    int ret = 0;
    struct zxdh_mpf_query_bar_msg *entity = NULL;
    struct zxdh_pci_query_hdr query_hdr = {0};
    struct zxdh_pci_bar_msg in = {0};
    struct zxdh_msg_recviver_mem result = {0};
    struct zxdh_bar_ioctl_dev *dev = zxdh_get_bar_ioctl_dev();

    entity = kmalloc(sizeof(*entity), GFP_KERNEL);
    if (NULL == entity)
    {
        LOG_ERR("malloc failed.\n");
        return -1;
    }
    memset(entity, 0,sizeof(*entity));
    if (copy_from_user(entity, (pci_res_st __user *)arg, sizeof(*entity)))
    {
        entity->ioctl_state = IOCTRL_ERR_MSG_GET;
        goto out;
    }

    query_hdr.mode = mode;
    query_hdr.pcie_id = entity->pci_res_msg.pcie_id;

    in.virt_addr = dev->bar0_base_virt_addr + ZXDH_BAR1_CHAN_OFFSET;
    in.payload_addr = &query_hdr;
    in.payload_len = sizeof(query_hdr);
    in.src = MSG_CHAN_END_PF;
    in.dst = MSG_CHAN_END_RISC;
    in.event_id = MODULE_PCIE_RES_QUERY;
    in.src_pcieid = dev->pcie_id;

    result.recv_buffer = &entity->pci_res_msg.reply;;
    result.buffer_len = sizeof(entity->pci_res_msg.reply);;

    ret = zxdh_bar_send_without_reps_hdr(&in, &result);
    if (ret != 0)
    {
        LOG_ERR("pcie send msg failed, ret:%d.\n", ret);
        goto out;
    }

    entity->ioctl_state = 0;
out:
    entity->bar_state = ret;
    ret = copy_to_user((int *)arg, entity, sizeof(*entity));
    if (0 != ret)
    {
        LOG_ERR("reply tp user failed.\n");
    }
    if (NULL != entity)
    {
        kfree(entity);
    }
    return 0;
}

int bar_chan_pci_res_get_single(unsigned int cmd, unsigned long arg)
{
    return bar_chan_pci_res_get(cmd, arg, PCI_QUERY_TYPE_SINGLE);
}

int bar_chan_pci_res_get_all(unsigned int cmd, unsigned long arg)
{
    return bar_chan_pci_res_get(cmd, arg, PCI_QUERY_TYPE_ALL);
}

struct bar_chan_func_sel
{
    unsigned int cmd;
    int (*ioctl_bar_chan_func)(unsigned int cmd, unsigned long arg);
}ioctl_func_arr[] = {
    {BAR_IOCTL_CMD_NORMAL,          bar_chan_common_sync_send},
    {BAR_IOCTL_CMD_SINGLE_DEV,      bar_chan_pci_res_get_single},
    {BAR_IOCTL_CMD_ALL_DEV,         bar_chan_pci_res_get_all},
    {BAR_IOCTL_CMD_SEND_REGISTER,   bar_chan_ioctl_register_eventid},
    {BAR_IOCTL_CMD_SEND_UNREGISTER, bar_chan_ioctl_unregister_eventid},
    {BAR_IOCTL_CMD_RECV_MSG,        bar_chan_ioctl_recv_msg},
};

static long bar_msg_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int i, ret = 0;
    int ioctl_func_nums = sizeof(ioctl_func_arr)/sizeof(ioctl_func_arr[0]);

    for (i = 0; i < ioctl_func_nums; i++)
    {
        if (ioctl_func_arr[i].cmd == cmd)
        {
            ret = ioctl_func_arr[i].ioctl_bar_chan_func(cmd, arg);
            break;
        }
    }
    return 0;
}

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .unlocked_ioctl = bar_msg_ioctl,
};

/* before called, get lock firstly*/
static bool is_msg_list_over_limit(struct list_head *list, int max_limit)
{
    int count = 0;
    struct list_head *pos;

    list_for_each(pos, list)
    {
        if (++count > max_limit)
            return true;
    }
    return false;
}

/*
将长度为msg_len的msg放到event_id为对应的wait-queue中去
如果没有注册， 就直接退出或者跳过

*/
int push_usr_msg_to_wait_queue(uint16_t event_id, void *msg, uint16_t msg_len)
{
    struct mpf_message_node *msg_ptr;
    struct mpf_wait_queue *wait_queue = NULL;
    unsigned long flags;

    if (msg_len >= BAR_CHAN_PLOAD_SIZE)
    {
        LOG_ERR("msg_len:%u is too long .\n", msg_len);
        return -EINVAL;
    }

    wait_queue = wait_queue_get();
    if (!wait_queue)
    {
        LOG_ERR( "wait_queue not found.\n");
        return -EINVAL;
    }

    /* will be free in ioctl cmd "bar_chan_ioctl_recv_msg", or free in module exit func*/
    msg_ptr = kzalloc(sizeof(struct mpf_message_node), GFP_KERNEL);
    if (!msg_ptr)
    {
        return -ENOMEM;
    }
    msg_ptr->event_id = event_id;
    msg_ptr->msg.hdr.recv_hdr_out.event_id = event_id;
    memcpy(msg_ptr->msg.pload, msg, msg_len);

    /* 加入消息链表并唤醒等待队列*/
    spin_lock_irqsave(&wait_queue->lock, flags);
    if (is_msg_list_over_limit(&wait_queue->msg_list, MSG_LIST_MAX_LEN))
    {
        spin_unlock_irqrestore(&wait_queue->lock, flags);
        kfree(msg_ptr);
        return -1;
    }
    list_add_tail(&msg_ptr->node, &wait_queue->msg_list);
    spin_unlock_irqrestore(&wait_queue->lock, flags);
    wake_up_interruptible(&wait_queue->wq);
    return 0;
}


int bar_msg_ioctl_dev_init(void)
{
    int ret = 0;

    if (alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0)
    {
        return -EBUSY;
    }

    if ((cl = class_create(THIS_MODULE, DEVICE_NAME)) == NULL)
    {
        unregister_chrdev_region(dev, 1);
        return -ENOMEM;
    }

    if (device_create(cl, NULL, dev, NULL, DEVICE_NAME) == NULL)
    {
        class_destroy(cl);
        unregister_chrdev_region(dev, 1);
        return -ENOMEM;
    }

    cdev_init(&c_dev, &fops);
    if (cdev_add(&c_dev, dev, 1) == -1)
    {
        device_destroy(cl, dev);
        class_destroy(cl);
        unregister_chrdev_region(dev, 1);
        return -ENOMEM;
    }

    LOG_INFO("Custom device registered\n");
    return ret;
}

void bar_msg_ioctl_dev_exit(void)
{
    cdev_del(&c_dev);
    device_destroy(cl, dev);
    class_destroy(cl);
    unregister_chrdev_region(dev, 1);
    LOG_INFO("Custom device unregistered\n");
}


int zxdh_bar_ioctl_msg_mdl_init(struct dh_core_dev *core_dev)
{
    int ret = 0;

    ret = zxdh_init_bar_ioctl_resource(core_dev);
    if (ret != 0)
    {
        return -1;
    }
    /* 在ioctl设备注册之前初始化了wait_queues, 后续的ioctl访问不会空指针*/
    ret = bar_msg_ioctl_dev_init();
    if (ret != 0)
    {
        LOG_ERR_DEV(core_dev, "custom init failed, ret:%d.\n", ret);
        zxdh_remove_bar_ioctl_resource(core_dev);
        return -1;
    }
    /* 在资源初始化之后， 才挂上用户态接收钩子， risc消息不会访问空指针*/
    zxdh_usr_msg_cache_func_register(push_usr_msg_to_wait_queue);
    return 0;
}

void zxdh_bar_ioctl_msg_mdl_exit(struct dh_core_dev *core_dev)
{
    /* 用户态接收消息钩子置空后释放资源， risc来的消息访问wait_queues安全*/
    zxdh_usr_msg_cache_func_register(NULL);

    /* ioctl设备解注册之后释放资源， 所有的ioctl cmd访问wait-queus安全*/
    bar_msg_ioctl_dev_exit();
    /* 释放资源*/
    zxdh_remove_bar_ioctl_resource(core_dev);
}
