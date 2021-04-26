/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * CSV driver for KVM
 *
 * HYGON CSV support
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 */

#ifndef __SVM_CSV_H
#define __SVM_CSV_H

#include <asm/processor-hygon.h>

#ifdef CONFIG_HYGON_CSV

/*
 * Hooks table: a table of function and variable pointers filled in
 * when module init.
 */
extern struct hygon_kvm_hooks_table {
	bool sev_hooks_installed;
	int (*sev_issue_cmd)(struct kvm *kvm, int id, void *data, int *error);
	unsigned long (*get_num_contig_pages)(unsigned long idx,
					      struct page **inpages,
					      unsigned long npages);
	struct page **(*sev_pin_memory)(struct kvm *kvm, unsigned long uaddr,
					unsigned long ulen, unsigned long *n,
					int write);
	void (*sev_unpin_memory)(struct kvm *kvm, struct page **pages,
				 unsigned long npages);
} hygon_kvm_hooks;

void __init csv_init(struct kvm_x86_ops *ops);
void csv_exit(void);

#else	/* !CONFIG_HYGON_CSV */

static inline void __init csv_init(struct kvm_x86_ops *ops) { }
static inline void csv_exit(void) { }

#endif	/* CONFIG_HYGON_CSV */

#endif	/* __SVM_CSV_H */
