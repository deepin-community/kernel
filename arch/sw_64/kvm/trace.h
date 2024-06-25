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

TRACE_EVENT(kvm_guest_fault,
	TP_PROTO(unsigned long vcpu_pc, unsigned long as_info,
		 unsigned long fault_entry_addr,
		 phys_addr_t fault_gpa),
	TP_ARGS(vcpu_pc, as_info, fault_entry_addr, fault_gpa),

	TP_STRUCT__entry(
		__field(unsigned long,	vcpu_pc)
		__field(unsigned long,	as_info)
		__field(unsigned long,	fault_entry_addr)
		__field(unsigned long long,	fault_gpa)
	),

	TP_fast_assign(
		__entry->vcpu_pc		= vcpu_pc;
		__entry->as_info		= as_info;
		__entry->fault_entry_addr	= fault_entry_addr;
		__entry->fault_gpa		= fault_gpa;
	),

	TP_printk("fault_gpa %#llx, as_info %#08lx, fault_entry_adr %#08lx, pc 0x%08lx",
		  __entry->fault_gpa, __entry->as_info,
		  __entry->fault_entry_addr, __entry->vcpu_pc)
);

TRACE_EVENT(kvm_access_fault,
	TP_PROTO(unsigned long ipa),
	TP_ARGS(ipa),

	TP_STRUCT__entry(
		__field(unsigned long,	ipa)
	),

	TP_fast_assign(
		__entry->ipa		= ipa;
	),

	TP_printk("IPA: %lx", __entry->ipa)
);

TRACE_EVENT(kvm_irq_line,
	TP_PROTO(int vcpu_idx, int irq_num, int level),
	TP_ARGS(vcpu_idx, irq_num, level),

	TP_STRUCT__entry(
		__field(int,		vcpu_idx)
		__field(int,		irq_num)
		__field(int,		level)
	),

	TP_fast_assign(
		__entry->vcpu_idx	= vcpu_idx;
		__entry->irq_num	= irq_num;
		__entry->level		= level;
	),

	TP_printk("Inject interrupt, vcpu->idx: %d, num: %d, level: %d",
		  __entry->vcpu_idx, __entry->irq_num, __entry->level)
);

TRACE_EVENT(kvm_mmio_emulate,
	TP_PROTO(unsigned long vcpu_pc, unsigned long instr,
		 unsigned long cpsr),
	TP_ARGS(vcpu_pc, instr, cpsr),

	TP_STRUCT__entry(
		__field(unsigned long,	vcpu_pc)
		__field(unsigned long,	instr)
		__field(unsigned long,	cpsr)
	),

	TP_fast_assign(
		__entry->vcpu_pc		= vcpu_pc;
		__entry->instr			= instr;
		__entry->cpsr			= cpsr;
	),

	TP_printk("Emulate MMIO at: 0x%016lx (instr: %08lx, cpsr: %08lx)",
		  __entry->vcpu_pc, __entry->instr, __entry->cpsr)
);

TRACE_EVENT(kvm_set_way_flush,
	    TP_PROTO(unsigned long vcpu_pc, bool cache),
	    TP_ARGS(vcpu_pc, cache),

	    TP_STRUCT__entry(
		__field(unsigned long,	vcpu_pc)
		__field(bool,		cache)
	    ),

	    TP_fast_assign(
		__entry->vcpu_pc		= vcpu_pc;
		__entry->cache		= cache;
	    ),

	    TP_printk("S/W flush at 0x%016lx (cache %s)",
		__entry->vcpu_pc, __entry->cache ? "on" : "off")
);

TRACE_EVENT(kvm_toggle_cache,
	    TP_PROTO(unsigned long vcpu_pc, bool was, bool now),
	    TP_ARGS(vcpu_pc, was, now),

	    TP_STRUCT__entry(
		__field(unsigned long,	vcpu_pc)
		__field(bool,		was)
		__field(bool,		now)
	    ),

	    TP_fast_assign(
		__entry->vcpu_pc		= vcpu_pc;
		__entry->was		= was;
		__entry->now		= now;
	    ),

	    TP_printk("VM op at 0x%016lx (cache was %s, now %s)",
		__entry->vcpu_pc, __entry->was ? "on" : "off",
		__entry->now ? "on" : "off")
);

TRACE_EVENT(kvm_set_guest_debug,
	TP_PROTO(struct kvm_vcpu *vcpu, __u32 guest_debug),
	TP_ARGS(vcpu, guest_debug),

	TP_STRUCT__entry(
		__field(struct kvm_vcpu *, vcpu)
		__field(__u32, guest_debug)
	),

	TP_fast_assign(
		__entry->vcpu = vcpu;
		__entry->guest_debug = guest_debug;
	),

	TP_printk("vcpu: %p, flags: 0x%08x", __entry->vcpu, __entry->guest_debug)
);

#endif /* _TRACE_ARM_SW_64_KVM_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace

/* This part must be outside protection */
#include <trace/define_trace.h>
