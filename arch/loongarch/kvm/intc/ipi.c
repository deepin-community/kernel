// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Loongson Technology Corporation Limited
 */

#include <linux/kvm_host.h>
#include <asm/kvm_ipi.h>
#include <asm/kvm_vcpu.h>

static void ipi_send(struct kvm *kvm, uint64_t data)
{
	struct kvm_vcpu *vcpu;
	struct kvm_interrupt irq;
	int cpu, action, status;

	cpu = ((data & 0xffffffff) >> 16) & 0x3ff;
	vcpu = kvm_get_vcpu_by_cpuid(kvm, cpu);
	if (unlikely(vcpu == NULL)) {
		kvm_err("%s: invalid target cpu: %d\n", __func__, cpu);
		return;
	}

	action = 1 << (data & 0x1f);

	spin_lock(&vcpu->arch.ipi_state.lock);
	status = vcpu->arch.ipi_state.status;
	vcpu->arch.ipi_state.status |= action;
	if (status == 0) {
		irq.irq = LARCH_INT_IPI;
		kvm_vcpu_ioctl_interrupt(vcpu, &irq);
	}
	spin_unlock(&vcpu->arch.ipi_state.lock);
}

static void ipi_clear(struct kvm_vcpu *vcpu, uint64_t data)
{
	struct kvm_interrupt irq;

	spin_lock(&vcpu->arch.ipi_state.lock);
	vcpu->arch.ipi_state.status &= ~data;
	if (!vcpu->arch.ipi_state.status) {
		irq.irq = -LARCH_INT_IPI;
		kvm_vcpu_ioctl_interrupt(vcpu, &irq);
	}
	spin_unlock(&vcpu->arch.ipi_state.lock);
}

static uint64_t read_mailbox(struct kvm_vcpu *vcpu, int offset, int len)
{
	void *pbuf;
	uint64_t ret = 0;

	spin_lock(&vcpu->arch.ipi_state.lock);
	pbuf = (void *)vcpu->arch.ipi_state.buf + (offset - 0x20);
	if (len == 1)
		ret = *(unsigned char *)pbuf;
	else if (len == 2)
		ret = *(unsigned short *)pbuf;
	else if (len == 4)
		ret = *(unsigned int *)pbuf;
	else if (len == 8)
		ret = *(unsigned long *)pbuf;
	else
		kvm_err("%s: unknown data len: %d\n", __func__, len);
	spin_unlock(&vcpu->arch.ipi_state.lock);

	return ret;
}

static void write_mailbox(struct kvm_vcpu *vcpu, int offset,
			uint64_t data, int len)
{
	void *pbuf;

	spin_lock(&vcpu->arch.ipi_state.lock);
	pbuf = (void *)vcpu->arch.ipi_state.buf + (offset - 0x20);
	if (len == 1)
		*(unsigned char *)pbuf = (unsigned char)data;
	else if (len == 2)
		*(unsigned short *)pbuf = (unsigned short)data;
	else if (len == 4)
		*(unsigned int *)pbuf = (unsigned int)data;
	else if (len == 8)
		*(unsigned long *)pbuf = (unsigned long)data;
	else
		kvm_err("%s: unknown data len: %d\n", __func__, len);
	spin_unlock(&vcpu->arch.ipi_state.lock);
}

static int loongarch_ipi_writel(struct kvm_vcpu *vcpu, gpa_t addr,
				int len, const void *val)
{
	uint64_t data;
	uint32_t offset;
	int ret = 0;

	data = *(uint64_t *)val;

	offset = (uint32_t)(addr & 0xff);
	WARN_ON_ONCE(offset & (len - 1));

	switch (offset) {
	case CORE_STATUS_OFF:
		kvm_err("CORE_SET_OFF Can't be write\n");
		ret = -EINVAL;
		break;
	case CORE_EN_OFF:
		spin_lock(&vcpu->arch.ipi_state.lock);
		vcpu->arch.ipi_state.en = data;
		spin_unlock(&vcpu->arch.ipi_state.lock);
		break;
	case IOCSR_IPI_SEND:
		ipi_send(vcpu->kvm, data);
		break;
	case CORE_SET_OFF:
		kvm_info("CORE_SET_OFF simulation is required\n");
		ret = -EINVAL;
		break;
	case CORE_CLEAR_OFF:
		/* Just clear the status of the current vcpu */
		ipi_clear(vcpu, data);
		break;
	case CORE_BUF_20 ... CORE_BUF_38 + 7:
		if (offset + len > CORE_BUF_38 + 8) {
			kvm_err("%s: invalid offset or len: offset = %d, len = %d\n",
				__func__, offset, len);
			ret = -EINVAL;
			break;
		}
		write_mailbox(vcpu, offset, data, len);
		break;
	default:
		kvm_err("%s: unknown addr: %llx\n", __func__, addr);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int loongarch_ipi_readl(struct kvm_vcpu *vcpu, gpa_t addr,
				int len, void *val)
{
	uint32_t offset;
	uint64_t res = 0;
	int ret = 0;

	offset = (uint32_t)(addr & 0xff);
	WARN_ON_ONCE(offset & (len - 1));

	switch (offset) {
	case CORE_STATUS_OFF:
		spin_lock(&vcpu->arch.ipi_state.lock);
		res = vcpu->arch.ipi_state.status;
		spin_unlock(&vcpu->arch.ipi_state.lock);
		break;
	case CORE_EN_OFF:
		spin_lock(&vcpu->arch.ipi_state.lock);
		res = vcpu->arch.ipi_state.en;
		spin_unlock(&vcpu->arch.ipi_state.lock);
		break;
	case CORE_SET_OFF:
		res = 0;
		break;
	case CORE_CLEAR_OFF:
		res = 0;
		break;
	case CORE_BUF_20 ... CORE_BUF_38 + 7:
		if (offset + len > CORE_BUF_38 + 8) {
			kvm_err("%s: invalid offset or len: offset = %d, len = %d\n",
				__func__, offset, len);
			ret = -EINVAL;
			break;
		}
		res = read_mailbox(vcpu, offset, len);
		break;
	default:
		kvm_err("%s: unknown addr: %llx\n", __func__, addr);
		ret = -EINVAL;
		break;
	}

	*(uint64_t *)val = res;

	return ret;
}

static int kvm_loongarch_ipi_write(struct kvm_vcpu *vcpu,
			struct kvm_io_device *dev,
			gpa_t addr, int len, const void *val)
{
	struct loongarch_ipi *ipi;
	int ret;

	ipi = vcpu->kvm->arch.ipi;
	if (!ipi) {
		kvm_err("%s: ipi irqchip not valid!\n", __func__);
		return -EINVAL;
	}

	ipi->kvm->stat.ipi_write_exits++;
	ret = loongarch_ipi_writel(vcpu, addr, len, val);

	return ret;
}

static int kvm_loongarch_ipi_read(struct kvm_vcpu *vcpu,
			struct kvm_io_device *dev,
			gpa_t addr, int len, void *val)
{
	struct loongarch_ipi *ipi;
	int ret;

	ipi = vcpu->kvm->arch.ipi;
	if (!ipi) {
		kvm_err("%s: ipi irqchip not valid!\n", __func__);
		return -EINVAL;
	}

	ipi->kvm->stat.ipi_read_exits++;
	ret = loongarch_ipi_readl(vcpu, addr, len, val);

	return ret;
}

static void send_ipi_data(struct kvm_vcpu *vcpu, gpa_t addr, uint64_t data)
{
	int i;
	uint32_t val = 0, mask = 0;
	/*
	 * Bit 27-30 is mask for byte writing.
	 * If the mask is 0, we need not to do anything.
	 */
	if ((data >> 27) & 0xf) {
		/* Read the old val */
		kvm_io_bus_read(vcpu, KVM_IOCSR_BUS, addr, sizeof(val), &val);

		/* Construct the mask by scanning the bit 27-30 */
		for (i = 0; i < 4; i++) {
			if (data & (0x1 << (27 + i)))
				mask |= (0xff << (i * 8));
		}
	/* Save the old part of val */
		val &= mask;
	}

	val |= ((uint32_t)(data >> 32) & ~mask);
	kvm_io_bus_write(vcpu, KVM_IOCSR_BUS, addr, sizeof(val), &val);
}

static void mail_send(struct kvm *kvm, uint64_t data)
{
	struct kvm_vcpu *vcpu;
	int cpu, mailbox;
	int offset;

	cpu = ((data & 0xffffffff) >> 16) & 0x3ff;
	vcpu = kvm_get_vcpu_by_cpuid(kvm, cpu);
	if (unlikely(vcpu == NULL)) {
		kvm_err("%s: invalid target cpu: %d\n", __func__, cpu);
		return;
	}

	mailbox = ((data & 0xffffffff) >> 2) & 0x7;
	offset = SMP_MAILBOX + CORE_BUF_20 + mailbox * 4;
	send_ipi_data(vcpu, offset, data);
}

static void any_send(struct kvm *kvm, uint64_t data)
{
	struct kvm_vcpu *vcpu;
	int cpu, offset;

	cpu = ((data & 0xffffffff) >> 16) & 0x3ff;
	vcpu = kvm_get_vcpu_by_cpuid(kvm, cpu);
	if (unlikely(vcpu == NULL)) {
		kvm_err("%s: invalid target cpu: %d\n", __func__, cpu);
		return;
	}

	offset = data & 0xffff;
	send_ipi_data(vcpu, offset, data);
}

static int kvm_loongarch_mail_write(struct kvm_vcpu *vcpu,
			struct kvm_io_device *dev,
			gpa_t addr, int len, const void *val)
{
	struct loongarch_ipi *ipi;

	ipi = vcpu->kvm->arch.ipi;
	if (!ipi) {
		kvm_err("%s: ipi irqchip not valid!\n", __func__);
		return -EINVAL;
	}

	addr &= 0xfff;
	addr -= IOCSR_MAIL_SEND;

	switch (addr) {
	case MAIL_SEND_OFFSET:
		mail_send(vcpu->kvm, *(uint64_t *)val);
		break;
	case ANY_SEND_OFFSET:
		any_send(vcpu->kvm, *(uint64_t *)val);
		break;
	default:
		break;
	}

	return 0;
}

static const struct kvm_io_device_ops kvm_loongarch_ipi_ops = {
	.read	= kvm_loongarch_ipi_read,
	.write	= kvm_loongarch_ipi_write,
};

static const struct kvm_io_device_ops kvm_loongarch_mail_ops = {
	.write	= kvm_loongarch_mail_write,
};

static int kvm_loongarch_ipi_get_attr(struct kvm_device *dev,
			struct kvm_device_attr *attr)
{
	return 0;
}

static int kvm_loongarch_ipi_set_attr(struct kvm_device *dev,
			struct kvm_device_attr *attr)
{
	return 0;
}

static void kvm_loongarch_ipi_destroy(struct kvm_device *dev)
{
	struct kvm *kvm;
	struct loongarch_ipi *ipi;
	struct kvm_io_device *device;

	if (!dev)
		return;

	kvm = dev->kvm;
	if (!kvm)
		return;

	ipi = kvm->arch.ipi;
	if (!ipi)
		return;

	device = &ipi->device;
	kvm_io_bus_unregister_dev(kvm, KVM_IOCSR_BUS, device);

	device = &ipi->mail_dev;
	kvm_io_bus_unregister_dev(kvm, KVM_IOCSR_BUS, device);

	kfree(ipi);
}

static int kvm_loongarch_ipi_create(struct kvm_device *dev, u32 type)
{
	struct kvm *kvm;
	struct loongarch_ipi *s;
	unsigned long addr;
	struct kvm_io_device *device;
	int ret;

	kvm_info("begin create loongarch ipi in kvm ...\n");
	if (!dev) {
		kvm_err("%s: kvm_device ptr is invalid!\n", __func__);
		return -EINVAL;
	}

	kvm = dev->kvm;
	if (kvm->arch.ipi) {
		kvm_err("%s: loongarch ipi has been created!\n", __func__);
		return -EINVAL;
	}

	s = kzalloc(sizeof(struct loongarch_ipi), GFP_KERNEL);
	if (!s)
		return -ENOMEM;
	spin_lock_init(&s->lock);
	s->kvm = kvm;

	/*
	 * Initialize IOCSR device
	 */
	device = &s->device;
	kvm_iodevice_init(device, &kvm_loongarch_ipi_ops);
	addr = SMP_MAILBOX;
	mutex_lock(&kvm->slots_lock);
	ret = kvm_io_bus_register_dev(kvm, KVM_IOCSR_BUS, addr,
			KVM_IOCSR_IPI_ADDR_SIZE, device);
	mutex_unlock(&kvm->slots_lock);
	if (ret < 0) {
		kvm_err("%s: initialize IOCSR dev failed, ret = %d\n", __func__, ret);
		goto err;
	}

	device = &s->mail_dev;
	kvm_iodevice_init(device, &kvm_loongarch_mail_ops);
	addr = MAIL_SEND_ADDR;
	mutex_lock(&kvm->slots_lock);
	ret = kvm_io_bus_register_dev(kvm, KVM_IOCSR_BUS, addr,
			KVM_IOCSR_MAIL_ADDR_SIZE, device);
	mutex_unlock(&kvm->slots_lock);
	if (ret < 0) {
		device = &s->device;
		kvm_io_bus_unregister_dev(kvm, KVM_IOCSR_BUS, device);
		kvm_err("%s: initialize mail box dev failed, ret = %d\n", __func__, ret);
		goto err;
	}

	kvm->arch.ipi = s;
	kvm_info("create loongarch ipi in kvm done!\n");

	return 0;

err:
	kfree(s);
	return -EFAULT;
}

static struct kvm_device_ops kvm_loongarch_ipi_dev_ops = {
	.name = "kvm-loongarch-ipi",
	.create = kvm_loongarch_ipi_create,
	.destroy = kvm_loongarch_ipi_destroy,
	.set_attr = kvm_loongarch_ipi_set_attr,
	.get_attr = kvm_loongarch_ipi_get_attr,
};

int kvm_loongarch_register_ipi_device(void)
{
	return kvm_register_device_ops(&kvm_loongarch_ipi_dev_ops,
					KVM_DEV_TYPE_LOONGARCH_IPI);
}
