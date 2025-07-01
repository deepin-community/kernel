#define VERSTR "v6.8-backport-6.6-5-g7f3382d9"

static char drv_ver[] = VERSTR;
#include <linux/module.h>
module_param_string(drv_ver, drv_ver, sizeof(drv_ver), 0444);

MODULE_PARM_DESC(drv_ver, VERSTR);

static int __init rtw88_drv_init(void)
{
	printk("rtw88 drv init version: %s\n", VERSTR);

	return 0;
}

static void __exit rtw88_drv_exit(void)
{
	printk("rtw88 drv exit\n");
}

module_init(rtw88_drv_init);
module_exit(rtw88_drv_exit);
