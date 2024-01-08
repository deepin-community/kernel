/* SPDX-License-Identifier: GPL-2.0 */
#if !defined(_SW64_KVM_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _SW64_KVM_TRACE_H

#include <linux/tracepoint.h>

#undef TRACE_SYSTEM
#define TRACE_SYSTEM kvm

/*
 * Tracepoint for guest mode entry.
 */
TRACE_EVENT(kvm_sw64_entry,
	TP_PROTO(unsigned int vcpu_id, unsigned int vcpu_pc),
	TP_ARGS(vcpu_id, vcpu_pc),

	TP_STRUCT__entry(
		__field(unsigned int,   vcpu_id)
		__field(unsigned int,	vcpu_pc)
	),

	TP_fast_assign(
		__entry->vcpu_id = vcpu_id;
		__entry->vcpu_pc = vcpu_pc;
	),

	TP_printk("VCPU %u: PC: 0x%08x", __entry->vcpu_id, __entry->vcpu_pc)
);

/*
 * Tracepoint for guest mode exit.
 */

TRACE_EVENT(kvm_sw64_exit,
	TP_PROTO(unsigned int exit_reason, unsigned long vcpu_pc),
	TP_ARGS(exit_reason, vcpu_pc),

	TP_STRUCT__entry(
		__field(unsigned int,	exit_reason)
		__field(unsigned long,	vcpu_pc)
	),

	TP_fast_assign(
		__entry->exit_reason = exit_reason;
		__entry->vcpu_pc = vcpu_pc;
	),

	TP_printk("exit_reason: 0x%04x (%11s),  PC: 0x%08lx",
		__entry->exit_reason,
		__print_symbolic(__entry->exit_reason, kvm_sw64_exception_type),
		__entry->vcpu_pc)
);

#endif /* _SW64_KVM_TRACE_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace

/* This part must be outside protection */
#include <trace/define_trace.h>
