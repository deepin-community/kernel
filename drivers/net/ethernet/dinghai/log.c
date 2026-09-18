#include <linux/module.h>

int debug_print;
module_param(debug_print, int, 0644);