#include <linux/init.h>
#include <linux/types.h>
#include <linux/audit.h>
#include <asm/unistd.h>
#include "audit-n32.h"
#include "audit-o32.h"

static unsigned dir_class[] = {
#include <asm-generic/audit_dir_write.h>
~0U
};

static unsigned read_class[] = {
#include <asm-generic/audit_read.h>
~0U
};

static unsigned write_class[] = {
#include <asm-generic/audit_write.h>
~0U
};

static unsigned chattr_class[] = {
#include <asm-generic/audit_change_attr.h>
~0U
};

static unsigned signal_class[] = {
#include <asm-generic/audit_signal.h>
~0U
};


/*
 * Pretend to be a single architecture
 */
int audit_classify_arch(int arch)
{
	return 0;
}

int audit_classify_syscall(int abi, unsigned syscall)
{
	switch (syscall) {
	case __NR_open:
		return AUDITSC_OPEN;
	case __NR_openat:
		return AUDITSC_OPEN;
#ifdef __NR_socketcall		/* Only exists on O32 */
	case __NR_socketcall:
		return AUDITSC_SOCKETCALL;
#endif
	case __NR_execve:
		return AUDITSC_EXECVE;
	default:
#ifdef CONFIG_AUDITSYSCALL_O32
		return audit_classify_syscall_o32(abi, syscall);
#endif
#ifdef CONFIG_AUDITSYSCALL_N32
		return audit_classify_syscall_n32(abi, syscall);
#endif
		return AUDITSC_NATIVE;
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

__initcall(audit_classes_init);
