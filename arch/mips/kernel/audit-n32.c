#define __WANT_SYSCALL_NUMBERS _MIPS_SIM_NABI32

#include <linux/init.h>
#include <linux/types.h>
#include <linux/audit.h>
#include <asm/unistd.h>
#include "audit-n32.h"

static unsigned dir_class_n32[] = {
#include <asm-generic/audit_dir_write.h>
~0U
};

static unsigned read_class_n32[] = {
#include <asm-generic/audit_read.h>
~0U
};

static unsigned write_class_n32[] = {
#include <asm-generic/audit_write.h>
~0U
};

static unsigned chattr_class_n32[] = {
#include <asm-generic/audit_change_attr.h>
~0U
};

static unsigned signal_class_n32[] = {
#include <asm-generic/audit_signal.h>
~0U
};

int audit_classify_syscall_n32(int abi, unsigned syscall)
{
	switch (syscall) {
	case __NR_open:
		return AUDITSC_OPEN;
	case __NR_openat:
		return AUDITSC_OPENAT;
	case __NR_execve:
		return AUDITSC_EXECVE;
	default:
		return AUDITSC_NATIVE;
	}
}

static int __init audit_classes_n32_init(void)
{
	audit_register_class(AUDIT_CLASS_WRITE_N32, write_class_n32);
	audit_register_class(AUDIT_CLASS_READ_N32, read_class_n32);
	audit_register_class(AUDIT_CLASS_DIR_WRITE_N32, dir_class_n32);
	audit_register_class(AUDIT_CLASS_CHATTR_N32, chattr_class_n32);
	audit_register_class(AUDIT_CLASS_SIGNAL_N32, signal_class_n32);

	return 0;
}

__initcall(audit_classes_n32_init);
