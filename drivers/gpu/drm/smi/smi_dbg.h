#ifndef __SMI_DBG_H__
#define __SMI_DBG_H__
#include <linux/version.h>

#include <drm/drm_print.h>


extern int smi_indent;
extern int smi_debug;

#define ENTER()	do { \
	if (smi_debug) {\
		printk("%*c %s\n", smi_indent++, '>', __func__); \
	} \
} while (0)


#define LEAVE(...)                                                                                 \
	do {             \
	if (smi_debug) {\
		printk("%*c %s\n", --smi_indent, '<', __func__);                         \
	} \
		return __VA_ARGS__;                                                                \
} while (0)

#define dbg_msg(fmt,args...)	do { \
		if (smi_debug) {\
			printk(fmt, ##args); \
		} \
} while (0)


#endif