// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Loongson Technology Corporation Limited
 */

#include <asm/kvm_extioi.h>
#include <asm/kvm_pch_pic.h>
#include <asm/kvm_vcpu.h>
#include <linux/count_zeros.h>

static int kvm_loongarch_pch_pic_write(struct kvm_vcpu *vcpu,
					struct kvm_io_device *dev,
					gpa_t addr, int len, const void *val)
{
	return 0;
}

static int kvm_loongarch_pch_pic_read(struct kvm_vcpu *vcpu,
					struct kvm_io_device *dev,
					gpa_t addr, int len, void *val)
{
	return 0;
}

static const struct kvm_io_device_ops kvm_loongarch_pch_pic_ops = {
	.read	= kvm_loongarch_pch_pic_read,
	.write	= kvm_loongarch_pch_pic_write,
};

static int kvm_loongarch_pch_pic_get_attr(struct kvm_device *dev,
				struct kvm_device_attr *attr)
{
	return 0;
}

static int kvm_loongarch_pch_pic_set_attr(struct kvm_device *dev,
				struct kvm_device_attr *attr)
{
	return 0;
}

static void kvm_loongarch_pch_pic_destroy(struct kvm_device *dev)
{
	struct kvm *kvm;
	struct loongarch_pch_pic *s;
	struct kvm_io_device *device;

	if (!dev)
		return;

	kvm = dev->kvm;
	if (!kvm)
		return;

	s = kvm->arch.pch_pic;
	if (!s)
		return;

	device = &s->device;
	/* unregister pch pic device and free it's memory */
	kvm_io_bus_unregister_dev(kvm, KVM_MMIO_BUS, device);
	kfree(s);
}

static int kvm_loongarch_pch_pic_create(struct kvm_device *dev, u32 type)
{
	struct loongarch_pch_pic *s;
	struct kvm *kvm = dev->kvm;

	/* pch pic should not has been created */
	if (kvm->arch.pch_pic)
		return -EINVAL;

	s = kzalloc(sizeof(struct loongarch_pch_pic), GFP_KERNEL);
	if (!s)
		return -ENOMEM;

	spin_lock_init(&s->lock);
	s->kvm = kvm;


	kvm->arch.pch_pic = s;

	kvm_info("create pch pic device successfully\n");
	return 0;
}

static struct kvm_device_ops kvm_loongarch_pch_pic_dev_ops = {
	.name = "kvm-loongarch-pch-pic",
	.create = kvm_loongarch_pch_pic_create,
	.destroy = kvm_loongarch_pch_pic_destroy,
	.set_attr = kvm_loongarch_pch_pic_set_attr,
	.get_attr = kvm_loongarch_pch_pic_get_attr,
};

int kvm_loongarch_register_pch_pic_device(void)
{
	return kvm_register_device_ops(&kvm_loongarch_pch_pic_dev_ops,
					KVM_DEV_TYPE_LOONGARCH_PCHPIC);
}
