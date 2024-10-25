#define VERSTR  "v6.8-backport-6.6-1-g809f4dd07"

static char *drv_ver = VERSTR;
#include <linux/module.h>
module_param_string(drv_ver, drv_ver, sizeof(drv_ver), 0444);

MODULE_PARM_DESC(drv_ver, VERSTR);

static int __init rtw89_drv_init(void)
{
	printk("rtw89 drv init version: %s\n", VERSTR);

	return 0;
}

static void __exit rtw89_drv_exit(void)
{
	printk("rtw89 drv exit\n");
}

module_init(rtw89_drv_init);
module_exit(rtw89_drv_exit);
