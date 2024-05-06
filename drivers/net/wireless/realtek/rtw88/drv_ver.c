static char drv_ver[] = "v6.8-backport-6.6-5-g7f3382d9";
#include <linux/module.h>
module_param_string(drv_ver, drv_ver, sizeof(drv_ver), 0444);
