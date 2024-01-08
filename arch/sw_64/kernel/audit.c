// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/audit.h>

#include <asm/unistd.h>

static unsigned int dir_class[] = {
#include <asm-generic/audit_dir_write.h>
~0U
};

static unsigned int read_class[] = {
#include <asm-generic/audit_read.h>
~0U
};

static unsigned int write_class[] = {
#include <asm-generic/audit_write.h>
~0U
};

static unsigned int chattr_class[] = {
#include <asm-generic/audit_change_attr.h>
~0U
};

static unsigned int signal_class[] = {
#include <asm-generic/audit_signal.h>
~0U
};

int audit_classify_arch(int arch)
{
	return 0;
}

int audit_classify_syscall(int abi, unsigned int syscall)
{
	switch (syscall) {
	case __NR_open:
		return 2;
	case __NR_openat:
		return 3;
	case __NR_execve:
		return 5;
	default:
		return 0;
	}
}

static int __init audit_classes_init(void)
{
	audit_register_class(AUDIT_CLASS_WRITE, write_class);
	audit_register_class(AUDIT_CLASS_READ, read_class);
	audit_register_class(AUDIT_CLASS_DIR_WRITE, dir_class);
	audit_register_class(AUDIT_CLASS_CHATTR, chattr_class);
	audit_register_class(AUDIT_CLASS_SIGNAL, signal_class);
	return 0;
}

device_initcall(audit_classes_init);
