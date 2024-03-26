// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 - os kernal
 * Author: fire3 <fire3@example.com> yangzh <yangzh@gmail.com>
 * linhn <linhn@example.com>
 */
#include <linux/kvm_host.h>
#include <asm/kvm_mmio.h>
#include <asm/kvm_emulate.h>
#include <asm/kvm_asm.h>
#include <trace/events/kvm.h>

#include "trace.h"

static unsigned long mmio_read_buf(char *buf, unsigned int len)
{
	unsigned long data = 0;
	union {
		u16	hword;
		u32	word;
		u64	dword;
	} tmp;

	switch (len) {
	case 1:
		data = buf[0];
		break;
	case 2:
		memcpy(&tmp.hword, buf, len);
		data = tmp.hword;
		break;
	case 4:
		memcpy(&tmp.word, buf, len);
		data = tmp.word;
		break;
	case 8:
		memcpy(&tmp.dword, buf, len);
		data = tmp.dword;
		break;
	}

	return data;
}

int kvm_handle_mmio_return(struct kvm_vcpu *vcpu, struct kvm_run *run)
{
	unsigned long data;
	unsigned int len;

	if (!run->mmio.is_write) {
		len = run->mmio.len;
		if (len > sizeof(unsigned long))
			return -EINVAL;

		data = mmio_read_buf(run->mmio.data, len);
		trace_kvm_mmio(KVM_TRACE_MMIO_READ, len, run->mmio.phys_addr, &data);
		vcpu_set_reg(vcpu, vcpu->arch.mmio_decode.rt, data);
	}

	vcpu->stat.mmio_exits++;
	vcpu->arch.regs.pc += 4;

	return 0;
}

int io_mem_abort(struct kvm_vcpu *vcpu, struct kvm_run *run,
		struct hcall_args *hargs)
{
	int ret;

	run->mmio.phys_addr = hargs->arg1 & 0xfffffffffffffUL;
	sw64_decode(vcpu, hargs->arg2, run);

	if (run->mmio.is_write) {
		trace_kvm_mmio(KVM_TRACE_MMIO_WRITE, run->mmio.len,
				run->mmio.phys_addr, run->mmio.data);
		ret = kvm_io_bus_write(vcpu, KVM_MMIO_BUS, run->mmio.phys_addr,
				run->mmio.len, run->mmio.data);
	} else {
		trace_kvm_mmio(KVM_TRACE_MMIO_READ_UNSATISFIED, run->mmio.len,
				run->mmio.phys_addr, NULL);
		ret = kvm_io_bus_read(vcpu, KVM_MMIO_BUS, run->mmio.phys_addr,
				run->mmio.len, run->mmio.data);
	}

	if (!ret) {
		/* We handled the access successfully in the kernel. */
		kvm_handle_mmio_return(vcpu, run);
		return 1;
	}

	run->exit_reason = KVM_EXIT_MMIO;
	return 0;
}
