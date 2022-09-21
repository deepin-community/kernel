#include <linux/kernel.h>
#include <linux/module.h>
#include "phytium_ras.h"
#include <linux/init.h>
#include <asm/io.h>
#include <linux/miscdevice.h>
#include <linux/err.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/arm_sdei.h>
#include <linux/uaccess.h>

extern int sdei_event_register(u32 event_num, sdei_event_callback *cb, void *arg);
extern int sdei_interrupt_bind(int irq);
extern int sdei_interrupt_unbind(u32 event_num);
extern int sdei_event_enable(u32 event_num);
extern int sdei_event_disable(u32 event_num);
extern int sdei_event_unregister(u32 event_num);
extern int sdei_pe_mask(void);
extern int sdei_pe_unmask(void);
extern int sdei_event_complete(u32 event_num);
//extern int sdei_init(void);

static DEFINE_SPINLOCK(sdei_lock);

int sdei_event_cb(u32 event, struct pt_regs *regs, void *arg)
{
      unsigned long flags;
      spin_lock_irqsave(&sdei_lock, flags);
      spin_unlock_irqrestore(&sdei_lock, flags);
      asm volatile(
              "ldp     x29, x30, [sp],#32\n"
              "ldr x0, =0xc4000025\n"
              "smc    #0\n"
              :
              :
              :"memory"
              );

      return 0;
}

int ras_bind_event(void)
{
    int ev;
    int ret;
    int irq = 34;
    ev = sdei_interrupt_bind(irq);
    if( ev < 0 ){
        printk("sdei_interrupt_bind failed with 0x%x, irq:%d\n",ev,irq);
        return -1;
    }

    ret = sdei_event_register(ev, sdei_event_cb, "test");
    if( ret < 0 ){
        printk("sdei_event_register failed with 0x%x\n", ret);
        sdei_interrupt_unbind(ev);
        return -1;
    }

    ret = sdei_event_enable(ev);
    if( ret < 0 ){
        printk("sdei_event_enable failed with 0x%x\n",ret);
        sdei_event_unregister(ev);
        sdei_interrupt_unbind(ev);
        return -1;
    }
    ret = sdei_pe_unmask();
    if( ret < 0 ){
        printk("sdei_pe_mask failed with 0x%x\n", ret);
        sdei_event_disable(ev);
        sdei_event_unregister(ev);
        sdei_interrupt_unbind(ev);
        return -1;
    }

    return 0;
}

int ras_nonbind_event(void)
{
    int ev;
    int ret;
    ev = 600;
    ret = sdei_event_register(ev, sdei_event_cb, "test");
    if( ret < 0 ){
        printk("sdei_event_register failed with 0x%x\n", ret);
        return -1;
    }
    ret = sdei_event_enable(ev);
    if( ret < 0 ){
        printk("sdei_event_enable failed with 0x%x\n",ret);
        sdei_event_unregister(ev);
        return -1;
    }
    ret = sdei_pe_unmask();
    if( ret < 0 ){
        printk("sdei_pe_mask failed with 0x%x\n", ret);
        sdei_event_disable(ev);
        sdei_event_unregister(ev);
        return -1;
    }

    return 0;
}


static long ras_misc_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
        void __user *argp;
        unsigned int val = 0;
        argp = (void __user *)arg;
        switch(cmd)
	{
		case RAS_BIND_EVENT:{
                    ras_bind_event();
                    if( copy_to_user(argp, &val, sizeof(val)))
                        return -EFAULT;
                    break;
                }
                case RAS_NONBIND_EVENT:{
                    ras_nonbind_event();
                    if( copy_to_user(argp, &val, sizeof(val)))
                        return -EFAULT;
                    break;
                }
		default:{
                    break;
                }
	}

	return 0;
}

static const struct file_operations ras_misc_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ras_misc_ioctl,
};

static struct miscdevice ras_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "phtium-ras",
	.fops = &ras_misc_fops,
};

static int __init ft_ras_init(void)
{
	int ret;
	ret = misc_register(&ras_misc_device);
	if( ret ){
		printk("unable to register ras driver\n");
		return ret;
	}

	printk("register ras driver success\n");

	return 0;
}

static void __exit ft_ras_exit(void)
{

}
module_init(ft_ras_init);
module_exit(ft_ras_exit);

MODULE_DESCRIPTION("phytium ras");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");
