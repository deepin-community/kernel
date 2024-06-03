// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Loongson Technology Corporation Limited
 */

#include <asm/kvm_extioi.h>
#include <asm/kvm_vcpu.h>
#include <linux/count_zeros.h>

static int kvm_loongarch_extioi_write(struct kvm_vcpu *vcpu,
				struct kvm_io_device *dev,
				gpa_t addr, int len, const void *val)
{
	return 0;
}

static int kvm_loongarch_extioi_read(struct kvm_vcpu *vcpu,
				struct kvm_io_device *dev,
				gpa_t addr, int len, void *val)
{
	return 0;
}

static const struct kvm_io_device_ops kvm_loongarch_extioi_ops = {
	.read	= kvm_loongarch_extioi_read,
	.write	= kvm_loongarch_extioi_write,
};

static int kvm_loongarch_extioi_get_attr(struct kvm_device *dev,
				struct kvm_device_attr *attr)
{
	return 0;
}

static int kvm_loongarch_extioi_set_attr(struct kvm_device *dev,
				struct kvm_device_attr *attr)
{
	return 0;
}

static void kvm_loongarch_extioi_destroy(struct kvm_device *dev)
{
	struct kvm *kvm;
	struct loongarch_extioi *extioi;
	struct kvm_io_device *device;

	if (!dev)
		return;

	kvm = dev->kvm;
	if (!kvm)
		return;

	extioi = kvm->arch.extioi;
	if (!extioi)
		return;

	device = &extioi->device;
	kvm_io_bus_unregister_dev(kvm, KVM_IOCSR_BUS, device);
	kfree(extioi);
}

static int kvm_loongarch_extioi_create(struct kvm_device *dev, u32 type)
{
	int ret;
	struct loongarch_extioi *s;
	struct kvm_io_device *device;
	struct kvm *kvm = dev->kvm;

	/* extioi has been created */
	if (kvm->arch.extioi)
		return -EINVAL;

	s = kzalloc(sizeof(struct loongarch_extioi), GFP_KERNEL);
	if (!s)
		return -ENOMEM;
	spin_lock_init(&s->lock);
	s->kvm = kvm;

	/*
	 * Initialize IOCSR device
	 */
	device = &s->device;
	kvm_iodevice_init(device, &kvm_loongarch_extioi_ops);
	mutex_lock(&kvm->slots_lock);
	ret = kvm_io_bus_register_dev(kvm, KVM_IOCSR_BUS, EXTIOI_BASE, EXTIOI_SIZE, device);
	mutex_unlock(&kvm->slots_lock);
	if (ret < 0) {
		kfree(s);
		return -EFAULT;
	}

	kvm->arch.extioi = s;

	kvm_info("create extioi device successfully\n");
	return 0;
}

static struct kvm_device_ops kvm_loongarch_extioi_dev_ops = {
	.name = "kvm-loongarch-extioi",
	.create = kvm_loongarch_extioi_create,
	.destroy = kvm_loongarch_extioi_destroy,
	.set_attr = kvm_loongarch_extioi_set_attr,
	.get_attr = kvm_loongarch_extioi_get_attr,
};

int kvm_loongarch_register_extioi_device(void)
{
	return kvm_register_device_ops(&kvm_loongarch_extioi_dev_ops,
					KVM_DEV_TYPE_LOONGARCH_EIOINTC);
}
