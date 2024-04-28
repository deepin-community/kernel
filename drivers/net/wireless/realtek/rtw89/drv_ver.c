static const char drv_ver[]="v6.8-backport-6.6-1-g809f4dd07";
#include <linux/module.h>
module_param_string(drv_ver, drv_ver, sizeof(drv_ver), 0444);
