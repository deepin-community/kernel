// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Loongson Technology Corporation Limited
 */

#include <linux/kvm_host.h>
#include <asm/kvm_ipi.h>
#include <asm/kvm_vcpu.h>

static int kvm_loongarch_ipi_write(struct kvm_vcpu *vcpu,
			struct kvm_io_device *dev,
			gpa_t addr, int len, const void *val)
{
	return 0;
}

static int kvm_loongarch_ipi_read(struct kvm_vcpu *vcpu,
			struct kvm_io_device *dev,
			gpa_t addr, int len, void *val)
{
	return 0;
}

static int kvm_loongarch_mail_write(struct kvm_vcpu *vcpu,
			struct kvm_io_device *dev,
			gpa_t addr, int len, const void *val)
{
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
