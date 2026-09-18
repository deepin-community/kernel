#include "fuc_hotplug_ioctl.h"
#include "fuc_hotplug_commom.h"

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

extern long fuc_hp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static int fuc_hp_open(struct inode *inode, struct file *file)
{
    if (inode->i_private)
    {
        file->private_data = inode->i_private;
    }

    return FUC_HP_OK;
}

static int fuc_hp_dev_release(struct inode *i, struct file *f)
{
    DH_LOG_INFO(MODULE_FUC_HP, "fuc_hp device released!\n");
    return FUC_HP_OK;
}

static struct file_operations fuc_hp_fops =
{
    .owner = THIS_MODULE,
    .open = fuc_hp_open,
    .release = fuc_hp_dev_release,
    .unlocked_ioctl = fuc_hp_ioctl,
};

int zxdh_host_fuc_hotplug_driver_init(void)
{
    int ret = FUC_HP_OK;

    if (alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0)
    {
        return -EBUSY;
    }

    if ((cl = class_create(THIS_MODULE, CLASS_NAME)) == NULL)
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

    cdev_init(&c_dev, &fuc_hp_fops);

    if (cdev_add(&c_dev, dev, 1) == FUC_HP_FAILED)
    {
        device_destroy(cl, dev);
        class_destroy(cl);
        unregister_chrdev_region(dev, 1);
        return -ENOMEM;
    }
    DH_LOG_INFO(MODULE_FUC_HP, "fuction_hotplug device registered\n");
    return ret;
}

void zxdh_host_fuc_hotplug_driver_exit(void)
{
    cdev_del(&c_dev);
    device_destroy(cl, dev);
    class_destroy(cl);
    unregister_chrdev_region(dev, 1);
    DH_LOG_INFO(MODULE_FUC_HP, "fuc_hp device unregistered\n");
}