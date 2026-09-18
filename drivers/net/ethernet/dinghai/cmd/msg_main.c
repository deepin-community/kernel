#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>

#include "msg_chan_ver.h"
#include "msg_chan_priv.h"

static int __init msg_chan_init(void)
{
    BAR_LOG_DEBUG("%s enter\n", __func__);
    zxdh_bar_msg_chan_init();

#ifdef TEST
    BAR_TestApp();
#endif

    return 0;
}

static void __exit msg_chan_exit(void)
{
    zxdh_bar_msg_chan_remove();
    BAR_LOG_DEBUG("msg chan exit\n");
}

module_init(msg_chan_init);
module_exit(msg_chan_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_DESCRIPTION(DRV_DESCRIPTION ", v" DRV_VERSION);
MODULE_AUTHOR("ZTE");
