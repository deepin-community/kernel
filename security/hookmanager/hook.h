#ifndef __LINUX_HOOK_H
#define __LINUX_HOOK_H

#include <linux/list.h>
#include <linux/mutex.h>

struct uos_hook_entry {
	struct uos_hook_cb_entry *cb;
	int run_flag;
	int del_flag;
	struct list_head link;
};

struct uos_hook_entry_list {
	struct mutex lock;
	struct list_head entries;
	struct srcu_struct uksi_srcu;
};

void uos_hook_entry_free(struct uos_hook_entry *uos_entry);

#endif
