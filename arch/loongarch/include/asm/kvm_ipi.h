/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2024 Loongson Technology Corporation Limited
 */

#ifndef __ASM_KVM_IPI_H
#define __ASM_KVM_IPI_H

#include <kvm/iodev.h>

#define LARCH_INT_IPI			12

struct loongarch_ipi {
	spinlock_t lock;
	struct kvm *kvm;
	struct kvm_io_device device;
	struct kvm_io_device mail_dev;
};

struct ipi_state {
	spinlock_t lock;
	uint32_t status;
	uint32_t en;
	uint32_t set;
	uint32_t clear;
	uint64_t buf[4];
};

#define SMP_MAILBOX			0x1000
#define KVM_IOCSR_IPI_ADDR_SIZE		0x48

#define CORE_STATUS_OFF			0x000
#define CORE_EN_OFF			0x004
#define CORE_SET_OFF			0x008
#define CORE_CLEAR_OFF			0x00c
#define CORE_BUF_20			0x020
#define CORE_BUF_28			0x028
#define CORE_BUF_30			0x030
#define CORE_BUF_38			0x038
#define IOCSR_IPI_SEND			0x040

#define IOCSR_MAIL_SEND			0x048
#define IOCSR_ANY_SEND			0x158

#define MAIL_SEND_ADDR			(SMP_MAILBOX + IOCSR_MAIL_SEND)
#define KVM_IOCSR_MAIL_ADDR_SIZE	0x118

#define MAIL_SEND_OFFSET		0
#define ANY_SEND_OFFSET			(IOCSR_ANY_SEND - IOCSR_MAIL_SEND)

int kvm_loongarch_register_ipi_device(void);
#endif
