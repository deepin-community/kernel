// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Loongson Technology Corporation Limited
 */

#include <asm/kvm_extioi.h>
#include <asm/kvm_vcpu.h>
#include <linux/count_zeros.h>

#define loongarch_ext_irq_lock(s, flags)	spin_lock_irqsave(&s->lock, flags)
#define loongarch_ext_irq_unlock(s, flags)	spin_unlock_irqrestore(&s->lock, flags)

static void extioi_update_irq(struct loongarch_extioi *s, int irq, int level)
{
	int ipnum, cpu, found, irq_index, irq_mask;
	struct kvm_interrupt vcpu_irq;
	struct kvm_vcpu *vcpu;

	ipnum = s->ipmap.reg_u8[irq / 32];
	if (!(s->status & BIT(EIOINTC_ENABLE_INT_ENCODE))) {
		ipnum = count_trailing_zeros(ipnum);
		ipnum = (ipnum >= 0 && ipnum < 4) ? ipnum : 0;
	}

	cpu = s->sw_coremap[irq];
	vcpu = kvm_get_vcpu(s->kvm, cpu);
	irq_index = irq / 32;
	/* length of accessing core isr is 4 bytes */
	irq_mask = 1 << (irq & 0x1f);

	if (level) {
		/* if not enable return false */
		if (((s->enable.reg_u32[irq_index]) & irq_mask) == 0)
			return;
		s->coreisr.reg_u32[cpu][irq_index] |= irq_mask;
		found = find_first_bit(s->sw_coreisr[cpu][ipnum], EXTIOI_IRQS);
		set_bit(irq, s->sw_coreisr[cpu][ipnum]);
	} else {
		s->coreisr.reg_u32[cpu][irq_index] &= ~irq_mask;
		clear_bit(irq, s->sw_coreisr[cpu][ipnum]);
		found = find_first_bit(s->sw_coreisr[cpu][ipnum], EXTIOI_IRQS);
	}

	if (found < EXTIOI_IRQS)
		/* other irq is handling, need not update parent irq level */
		return;

	vcpu_irq.irq = level ? INT_HWI0 + ipnum : -(INT_HWI0 + ipnum);
	kvm_vcpu_ioctl_interrupt(vcpu, &vcpu_irq);
}

static void extioi_set_sw_coreisr(struct loongarch_extioi *s)
{
	int ipnum, cpu, irq_index, irq_mask, irq;

	for (irq = 0; irq < EXTIOI_IRQS; irq++) {
		ipnum = s->ipmap.reg_u8[irq / 32];
		if (!(s->status & BIT(EIOINTC_ENABLE_INT_ENCODE))) {
			ipnum = count_trailing_zeros(ipnum);
			ipnum = (ipnum >= 0 && ipnum < 4) ? ipnum : 0;
		}
		irq_index = irq / 32;
		/* length of accessing core isr is 4 bytes */
		irq_mask = 1 << (irq & 0x1f);

		cpu = s->coremap.reg_u8[irq];
		if (!!(s->coreisr.reg_u32[cpu][irq_index] & irq_mask))
			set_bit(irq, s->sw_coreisr[cpu][ipnum]);
		else
			clear_bit(irq, s->sw_coreisr[cpu][ipnum]);
	}
}

void extioi_set_irq(struct loongarch_extioi *s, int irq, int level)
{
	unsigned long *isr = (unsigned long *)s->isr.reg_u8;
	unsigned long flags;

	level ? set_bit(irq, isr) : clear_bit(irq, isr);
	if (!level)
		return;
	loongarch_ext_irq_lock(s, flags);
	extioi_update_irq(s, irq, level);
	loongarch_ext_irq_unlock(s, flags);
}

static inline void extioi_enable_irq(struct kvm_vcpu *vcpu, struct loongarch_extioi *s,
				int index, u8 mask, int level)
{
	u8 val;
	int irq;

	val = mask & s->isr.reg_u8[index];
	irq = ffs(val);
	while (irq != 0) {
		/*
		 * enable bit change from 0 to 1,
		 * need to update irq by pending bits
		 */
		extioi_update_irq(s, irq - 1 + index * 8, level);
		val &= ~(1 << (irq - 1));
		irq = ffs(val);
	}
}

static inline void extioi_update_sw_coremap(struct loongarch_extioi *s,
					int irq, void *pvalue, u32 len, bool notify)
{
	int i, cpu;
	u64 val = *(u64 *)pvalue;

	for (i = 0; i < len; i++) {
		cpu = val & 0xff;
		val = val >> 8;

		if (!(s->status & BIT(EIOINTC_ENABLE_CPU_ENCODE))) {
			cpu = ffs(cpu) - 1;
			cpu = (cpu >= 4) ? 0 : cpu;
		}

		if (s->sw_coremap[irq + i] == cpu)
			continue;

		if (notify && test_bit(irq + i, (unsigned long *)s->isr.reg_u8)) {
			/* lower irq at old cpu and raise irq at new cpu */
			extioi_update_irq(s, irq + i, 0);
			s->sw_coremap[irq + i] = cpu;
			extioi_update_irq(s, irq + i, 1);
		} else {
			s->sw_coremap[irq + i] = cpu;
		}
	}
}

static int loongarch_extioi_writeb(struct kvm_vcpu *vcpu,
				struct loongarch_extioi *s,
				gpa_t addr, int len, const void *val)
{
	int index, irq, ret = 0;
	u8 data, old_data, cpu;
	u8 coreisr, old_coreisr;
	gpa_t offset;

	data = *(u8 *)val;
	offset = addr - EXTIOI_BASE;

	switch (offset) {
	case EXTIOI_NODETYPE_START ... EXTIOI_NODETYPE_END:
		index = (offset - EXTIOI_NODETYPE_START);
		s->nodetype.reg_u8[index] = data;
		break;
	case EXTIOI_IPMAP_START ... EXTIOI_IPMAP_END:
		/*
		 * ipmap cannot be set at runtime, can be set only at the beginning
		 * of intr driver, need not update upper irq level
		 */
		index = (offset - EXTIOI_IPMAP_START);
		s->ipmap.reg_u8[index] = data;
		break;
	case EXTIOI_ENABLE_START ... EXTIOI_ENABLE_END:
		index = (offset - EXTIOI_ENABLE_START);
		old_data = s->enable.reg_u8[index];
		s->enable.reg_u8[index] = data;
		/*
		 * 1: enable irq.
		 * update irq when isr is set.
		 */
		data = s->enable.reg_u8[index] & ~old_data & s->isr.reg_u8[index];
		extioi_enable_irq(vcpu, s, index, data, 1);
		/*
		 * 0: disable irq.
		 * update irq when isr is set.
		 */
		data = ~s->enable.reg_u8[index] & old_data & s->isr.reg_u8[index];
		extioi_enable_irq(vcpu, s, index, data, 0);
		break;
	case EXTIOI_BOUNCE_START ... EXTIOI_BOUNCE_END:
		/* do not emulate hw bounced irq routing */
		index = offset - EXTIOI_BOUNCE_START;
		s->bounce.reg_u8[index] = data;
		break;
	case EXTIOI_COREISR_START ... EXTIOI_COREISR_END:
		/* length of accessing core isr is 8 bytes */
		index = (offset - EXTIOI_COREISR_START);
		/* using attrs to get current cpu index */
		cpu = vcpu->vcpu_id;
		coreisr = data;
		old_coreisr = s->coreisr.reg_u8[cpu][index];
		/* write 1 to clear interrupt */
		s->coreisr.reg_u8[cpu][index] = old_coreisr & ~coreisr;
		coreisr &= old_coreisr;
		irq = ffs(coreisr);
		while (irq != 0) {
			extioi_update_irq(s, irq - 1 + index * 8, 0);
			coreisr &= ~(1 << (irq - 1));
			irq = ffs(coreisr);
		}
		break;
	case EXTIOI_COREMAP_START ... EXTIOI_COREMAP_END:
		irq = offset - EXTIOI_COREMAP_START;
		index = irq;
		s->coremap.reg_u8[index] = data;
		extioi_update_sw_coremap(s, irq, (void *)&data, sizeof(data), true);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int loongarch_extioi_writew(struct kvm_vcpu *vcpu,
				struct loongarch_extioi *s,
				gpa_t addr, int len, const void *val)
{
	int i, index, irq, ret = 0;
	u8 cpu;
	u32 data, old_data;
	u32 coreisr, old_coreisr;
	gpa_t offset;

	data = *(u32 *)val;
	offset = addr - EXTIOI_BASE;

	switch (offset) {
	case EXTIOI_NODETYPE_START ... EXTIOI_NODETYPE_END:
		index = (offset - EXTIOI_NODETYPE_START) >> 2;
		s->nodetype.reg_u32[index] = data;
		break;
	case EXTIOI_IPMAP_START ... EXTIOI_IPMAP_END:
		/*
		 * ipmap cannot be set at runtime, can be set only at the beginning
		 * of intr driver, need not update upper irq level
		 */
		index = (offset - EXTIOI_IPMAP_START) >> 2;
		s->ipmap.reg_u32[index] = data;
		break;
	case EXTIOI_ENABLE_START ... EXTIOI_ENABLE_END:
		index = (offset - EXTIOI_ENABLE_START) >> 2;
		old_data = s->enable.reg_u32[index];
		s->enable.reg_u32[index] = data;
		/*
		 * 1: enable irq.
		 * update irq when isr is set.
		 */
		data = s->enable.reg_u32[index] & ~old_data & s->isr.reg_u32[index];
		index = index << 2;
		for (i = 0; i < sizeof(data); i++) {
			u8 mask = (data >> (i * 8)) & 0xff;

			extioi_enable_irq(vcpu, s, index + i, mask, 1);
		}
		/*
		 * 0: disable irq.
		 * update irq when isr is set.
		 */
		data = ~s->enable.reg_u32[index] & old_data & s->isr.reg_u32[index];
		for (i = 0; i < sizeof(data); i++) {
			u8 mask = (data >> (i * 8)) & 0xff;

			extioi_enable_irq(vcpu, s, index, mask, 0);
		}
		break;
	case EXTIOI_BOUNCE_START ... EXTIOI_BOUNCE_END:
		/* do not emulate hw bounced irq routing */
		index = (offset - EXTIOI_BOUNCE_START) >> 2;
		s->bounce.reg_u32[index] = data;
		break;
	case EXTIOI_COREISR_START ... EXTIOI_COREISR_END:
		/* length of accessing core isr is 8 bytes */
		index = (offset - EXTIOI_COREISR_START) >> 2;
		/* using attrs to get current cpu index */
		cpu = vcpu->vcpu_id;
		coreisr = data;
		old_coreisr = s->coreisr.reg_u32[cpu][index];
		/* write 1 to clear interrupt */
		s->coreisr.reg_u32[cpu][index] = old_coreisr & ~coreisr;
		coreisr &= old_coreisr;
		irq = ffs(coreisr);
		while (irq != 0) {
			extioi_update_irq(s, irq - 1 + index * 32, 0);
			coreisr &= ~(1 << (irq - 1));
			irq = ffs(coreisr);
		}
		break;
	case EXTIOI_COREMAP_START ... EXTIOI_COREMAP_END:
		irq = offset - EXTIOI_COREMAP_START;
		index = irq >> 2;
		s->coremap.reg_u32[index] = data;
		extioi_update_sw_coremap(s, irq, (void *)&data, sizeof(data), true);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int loongarch_extioi_writel(struct kvm_vcpu *vcpu,
				struct loongarch_extioi *s,
				gpa_t addr, int len, const void *val)
{
	int i, index, irq, bits, ret = 0;
	u8 cpu;
	u64 data, old_data;
	u64 coreisr, old_coreisr;
	gpa_t offset;

	data = *(u64 *)val;
	offset = addr - EXTIOI_BASE;

	switch (offset) {
	case EXTIOI_NODETYPE_START ... EXTIOI_NODETYPE_END:
		index = (offset - EXTIOI_NODETYPE_START) >> 3;
		s->nodetype.reg_u64[index] = data;
		break;
	case EXTIOI_IPMAP_START ... EXTIOI_IPMAP_END:
		/*
		 * ipmap cannot be set at runtime, can be set only at the beginning
		 * of intr driver, need not update upper irq level
		 */
		index = (offset - EXTIOI_IPMAP_START) >> 3;
		s->ipmap.reg_u64 = data;
		break;
	case EXTIOI_ENABLE_START ... EXTIOI_ENABLE_END:
		index = (offset - EXTIOI_ENABLE_START) >> 3;
		old_data = s->enable.reg_u64[index];
		s->enable.reg_u64[index] = data;
		/*
		 * 1: enable irq.
		 * update irq when isr is set.
		 */
		data = s->enable.reg_u64[index] & ~old_data & s->isr.reg_u64[index];
		index = index << 3;
		for (i = 0; i < sizeof(data); i++) {
			u8 mask = (data >> (i * 8)) & 0xff;

			extioi_enable_irq(vcpu, s, index + i, mask, 1);
		}
		/*
		 * 0: disable irq.
		 * update irq when isr is set.
		 */
		data = ~s->enable.reg_u64[index] & old_data & s->isr.reg_u64[index];
		for (i = 0; i < sizeof(data); i++) {
			u8 mask = (data >> (i * 8)) & 0xff;

			extioi_enable_irq(vcpu, s, index, mask, 0);
		}
		break;
	case EXTIOI_BOUNCE_START ... EXTIOI_BOUNCE_END:
		/* do not emulate hw bounced irq routing */
		index = (offset - EXTIOI_BOUNCE_START) >> 3;
		s->bounce.reg_u64[index] = data;
		break;
	case EXTIOI_COREISR_START ... EXTIOI_COREISR_END:
		/* length of accessing core isr is 8 bytes */
		index = (offset - EXTIOI_COREISR_START) >> 3;
		/* using attrs to get current cpu index */
		cpu = vcpu->vcpu_id;
		coreisr = data;
		old_coreisr = s->coreisr.reg_u64[cpu][index];
		/* write 1 to clear interrupt */
		s->coreisr.reg_u64[cpu][index] = old_coreisr & ~coreisr;
		coreisr &= old_coreisr;

		bits = sizeof(u64) * 8;
		irq = find_first_bit((void *)&coreisr, bits);
		while (irq < bits) {
			extioi_update_irq(s, irq + index * bits, 0);
			bitmap_clear((void *)&coreisr, irq, 1);
			irq = find_first_bit((void *)&coreisr, bits);
		}
		break;
	case EXTIOI_COREMAP_START ... EXTIOI_COREMAP_END:
		irq = offset - EXTIOI_COREMAP_START;
		index = irq >> 3;
		s->coremap.reg_u64[index] = data;
		extioi_update_sw_coremap(s, irq, (void *)&data, sizeof(data), true);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int kvm_loongarch_extioi_write(struct kvm_vcpu *vcpu,
				struct kvm_io_device *dev,
				gpa_t addr, int len, const void *val)
{
	int ret;
	struct loongarch_extioi *extioi = vcpu->kvm->arch.extioi;
	unsigned long flags;

	if (!extioi) {
		kvm_err("%s: extioi irqchip not valid!\n", __func__);
		return -EINVAL;
	}

	vcpu->kvm->stat.extioi_write_exits++;
	loongarch_ext_irq_lock(extioi, flags);

	switch (len) {
	case 1:
		ret = loongarch_extioi_writeb(vcpu, extioi, addr, len, val);
		break;
	case 4:
		ret = loongarch_extioi_writew(vcpu, extioi, addr, len, val);
		break;
	case 8:
		ret = loongarch_extioi_writel(vcpu, extioi, addr, len, val);
		break;
	default:
		ret = -EINVAL;
		WARN_ONCE(1, "%s: Abnormal address access:addr 0x%llx,size %d\n",
						__func__, addr, len);
	}
	loongarch_ext_irq_unlock(extioi, flags);

	return ret;
}

static int loongarch_extioi_readb(struct kvm_vcpu *vcpu, struct loongarch_extioi *s,
				gpa_t addr, int len, void *val)
{
	int index, ret = 0;
	gpa_t offset;
	u64 data = 0;

	offset = addr - EXTIOI_BASE;
	switch (offset) {
	case EXTIOI_NODETYPE_START ... EXTIOI_NODETYPE_END:
		index = offset - EXTIOI_NODETYPE_START;
		data = s->nodetype.reg_u8[index];
		break;
	case EXTIOI_IPMAP_START ... EXTIOI_IPMAP_END:
		index = offset - EXTIOI_IPMAP_START;
		data = s->ipmap.reg_u8[index];
		break;
	case EXTIOI_ENABLE_START ... EXTIOI_ENABLE_END:
		index = offset - EXTIOI_ENABLE_START;
		data = s->enable.reg_u8[index];
		break;
	case EXTIOI_BOUNCE_START ... EXTIOI_BOUNCE_END:
		index = offset - EXTIOI_BOUNCE_START;
		data = s->bounce.reg_u8[index];
		break;
	case EXTIOI_COREISR_START ... EXTIOI_COREISR_END:
		/* length of accessing core isr is 8 bytes */
		index = offset - EXTIOI_COREISR_START;
		data = s->coreisr.reg_u8[vcpu->vcpu_id][index];
		break;
	case EXTIOI_COREMAP_START ... EXTIOI_COREMAP_END:
		index = offset - EXTIOI_COREMAP_START;
		data = s->coremap.reg_u8[index];
		break;
	default:
		ret = -EINVAL;
		break;
	}

	*(u8 *)val = data;

	return ret;
}

static int loongarch_extioi_readw(struct kvm_vcpu *vcpu, struct loongarch_extioi *s,
				gpa_t addr, int len, void *val)
{
	int index, ret = 0;
	gpa_t offset;
	u64 data = 0;

	offset = addr - EXTIOI_BASE;
	switch (offset) {
	case EXTIOI_NODETYPE_START ... EXTIOI_NODETYPE_END:
		index = (offset - EXTIOI_NODETYPE_START) >> 2;
		data = s->nodetype.reg_u32[index];
		break;
	case EXTIOI_IPMAP_START ... EXTIOI_IPMAP_END:
		index = (offset - EXTIOI_IPMAP_START) >> 2;
		data = s->ipmap.reg_u32[index];
		break;
	case EXTIOI_ENABLE_START ... EXTIOI_ENABLE_END:
		index = (offset - EXTIOI_ENABLE_START) >> 2;
		data = s->enable.reg_u32[index];
		break;
	case EXTIOI_BOUNCE_START ... EXTIOI_BOUNCE_END:
		index = (offset - EXTIOI_BOUNCE_START) >> 2;
		data = s->bounce.reg_u32[index];
		break;
	case EXTIOI_COREISR_START ... EXTIOI_COREISR_END:
		/* length of accessing core isr is 8 bytes */
		index = (offset - EXTIOI_COREISR_START) >> 2;
		data = s->coreisr.reg_u32[vcpu->vcpu_id][index];
		break;
	case EXTIOI_COREMAP_START ... EXTIOI_COREMAP_END:
		index = (offset - EXTIOI_COREMAP_START) >> 2;
		data = s->coremap.reg_u32[index];
		break;
	default:
		ret = -EINVAL;
		break;
	}

	*(u32 *)val = data;

	return ret;
}

static int loongarch_extioi_readl(struct kvm_vcpu *vcpu, struct loongarch_extioi *s,
				gpa_t addr, int len, void *val)
{
	int index, ret = 0;
	gpa_t offset;
	u64 data = 0;

	offset = addr - EXTIOI_BASE;
	switch (offset) {
	case EXTIOI_NODETYPE_START ... EXTIOI_NODETYPE_END:
		index = (offset - EXTIOI_NODETYPE_START) >> 3;
		data = s->nodetype.reg_u64[index];
		break;
	case EXTIOI_IPMAP_START ... EXTIOI_IPMAP_END:
		index = (offset - EXTIOI_IPMAP_START) >> 3;
		data = s->ipmap.reg_u64;
		break;
	case EXTIOI_ENABLE_START ... EXTIOI_ENABLE_END:
		index = (offset - EXTIOI_ENABLE_START) >> 3;
		data = s->enable.reg_u64[index];
		break;
	case EXTIOI_BOUNCE_START ... EXTIOI_BOUNCE_END:
		index = (offset - EXTIOI_BOUNCE_START) >> 3;
		data = s->bounce.reg_u64[index];
		break;
	case EXTIOI_COREISR_START ... EXTIOI_COREISR_END:
		/* length of accessing core isr is 8 bytes */
		index = (offset - EXTIOI_COREISR_START) >> 3;
		data = s->coreisr.reg_u64[vcpu->vcpu_id][index];
		break;
	case EXTIOI_COREMAP_START ... EXTIOI_COREMAP_END:
		index = (offset - EXTIOI_COREMAP_START) >> 3;
		data = s->coremap.reg_u64[index];
		break;
	default:
		ret = -EINVAL;
		break;
	}

	*(u64 *)val = data;

	return ret;
}

static int kvm_loongarch_extioi_read(struct kvm_vcpu *vcpu,
				struct kvm_io_device *dev,
				gpa_t addr, int len, void *val)
{
	int ret = -EINVAL;
	struct loongarch_extioi *extioi = vcpu->kvm->arch.extioi;
	unsigned long flags;

	if (!extioi) {
		kvm_err("%s: extioi irqchip not valid!\n", __func__);
		return -EINVAL;
	}

	vcpu->kvm->stat.extioi_read_exits++;
	loongarch_ext_irq_lock(extioi, flags);

	switch (len) {
	case 1:
		ret = loongarch_extioi_readb(vcpu, extioi, addr, len, val);
		break;
	case 4:
		ret = loongarch_extioi_readw(vcpu, extioi, addr, len, val);
		break;
	case 8:
		ret = loongarch_extioi_readl(vcpu, extioi, addr, len, val);
		break;
	default:
		WARN_ONCE(1, "%s: Abnormal address access:addr 0x%llx,size %d\n",
						__func__, addr, len);
	}
	loongarch_ext_irq_unlock(extioi, flags);

	return ret;
}

static const struct kvm_io_device_ops kvm_loongarch_extioi_ops = {
	.read	= kvm_loongarch_extioi_read,
	.write	= kvm_loongarch_extioi_write,
};

static int kvm_loongarch_extioi_regs_access(struct kvm_device *dev,
					struct kvm_device_attr *attr,
					bool is_write)
{
	int len, addr;
	void __user *data;
	void *p = NULL;
	struct loongarch_extioi *s;
	unsigned long flags;

	s = dev->kvm->arch.extioi;
	addr = attr->attr;
	data = (void __user *)attr->addr;

	loongarch_ext_irq_lock(s, flags);
	switch (addr) {
	case EXTIOI_NODETYPE_START:
		p = s->nodetype.reg_u8;
		len = sizeof(s->nodetype);
		break;
	case EXTIOI_IPMAP_START:
		p = s->ipmap.reg_u8;
		len = sizeof(s->ipmap);
		break;
	case EXTIOI_ENABLE_START:
		p = s->enable.reg_u8;
		len = sizeof(s->enable);
		break;
	case EXTIOI_BOUNCE_START:
		p = s->bounce.reg_u8;
		len = sizeof(s->bounce);
		break;
	case EXTIOI_ISR_START:
		p = s->isr.reg_u8;
		len = sizeof(s->isr);
		break;
	case EXTIOI_COREISR_START:
		p = s->coreisr.reg_u8;
		len = sizeof(s->coreisr);
		break;
	case EXTIOI_COREMAP_START:
		p = s->coremap.reg_u8;
		len = sizeof(s->coremap);
		break;
	case EXTIOI_SW_COREMAP_FLAG:
		p = s->sw_coremap;
		len = sizeof(s->sw_coremap);
		break;
	default:
		loongarch_ext_irq_unlock(s, flags);
		kvm_err("%s: unknown extioi register, addr = %d\n", __func__, addr);
		return -EINVAL;
	}

	loongarch_ext_irq_unlock(s, flags);

	if (is_write) {
		if (copy_from_user(p, data, len))
			return -EFAULT;
	} else {
		if (copy_to_user(data, p, len))
			return -EFAULT;
	}

	if ((addr == EXTIOI_COREISR_START) && is_write) {
		loongarch_ext_irq_lock(s, flags);
		extioi_set_sw_coreisr(s);
		loongarch_ext_irq_unlock(s, flags);
	}

	return 0;
}

static int kvm_extioi_ctrl_access(struct kvm_device *dev,
					struct kvm_device_attr *attr)
{
	int ret = 0;
	unsigned long flags;
	unsigned long type = (unsigned long)attr->attr;
	u32 i, start_irq;
	void __user *data;
	struct loongarch_extioi *s = dev->kvm->arch.extioi;

	data = (void __user *)attr->addr;
	spin_lock_irqsave(&s->lock, flags);
	switch (type) {
	case KVM_DEV_LOONGARCH_EXTIOI_CTRL_INIT_NUM_CPU:
		if (copy_from_user(&s->num_cpu, data, 4))
			ret = -EFAULT;
		break;
	case KVM_DEV_LOONGARCH_EXTIOI_CTRL_INIT_FEATURE:
		if (copy_from_user(&s->features, data, 4))
			ret = -EFAULT;
		if (!(s->features & BIT(EIOINTC_HAS_VIRT_EXTENSION)))
			s->status |= BIT(EIOINTC_ENABLE);
		break;
	case KVM_DEV_LOONGARCH_EXTIOI_CTRL_LOAD_FINISHED:
		extioi_set_sw_coreisr(s);
		for (i = 0; i < (EXTIOI_IRQS / 4); i++) {
			start_irq = i * 4;
			extioi_update_sw_coremap(s, start_irq,
					(void *)&s->coremap.reg_u32[i], sizeof(u32), false);
		}
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&s->lock, flags);

	return ret;
}

static int kvm_extioi_sw_status_access(struct kvm_device *dev,
					struct kvm_device_attr *attr,
					bool is_write)
{
	int addr, ret = 0;
	unsigned long flags;
	void *p = NULL;
	void __user *data;
	struct loongarch_extioi *s;

	s = dev->kvm->arch.extioi;
	addr = attr->attr;
	addr &= 0xffff;

	data = (void __user *)attr->addr;
	switch (addr) {
	case KVM_DEV_LOONGARCH_EXTIOI_SW_STATUS_NUM_CPU:
		p = &s->num_cpu;
		break;
	case KVM_DEV_LOONGARCH_EXTIOI_SW_STATUS_FEATURE:
		p = &s->features;
		break;
	case KVM_DEV_LOONGARCH_EXTIOI_SW_STATUS_STATE:
		p = &s->status;
		break;
	default:
		kvm_err("%s: unknown extioi register, addr = %d\n", __func__, addr);
		return -EINVAL;
	}
	spin_lock_irqsave(&s->lock, flags);
	if (is_write) {
		if (copy_from_user(p, data, 4))
			ret = -EFAULT;
	} else {
		if (copy_to_user(data, p, 4))
			ret = -EFAULT;
	}
	spin_unlock_irqrestore(&s->lock, flags);

	return ret;
}

static int kvm_loongarch_extioi_get_attr(struct kvm_device *dev,
				struct kvm_device_attr *attr)
{
	switch (attr->group) {
	case KVM_DEV_LOONGARCH_EXTIOI_GRP_REGS:
		return kvm_loongarch_extioi_regs_access(dev, attr, false);
	case KVM_DEV_LOONGARCH_EXTIOI_GRP_SW_STATUS:
		return kvm_extioi_sw_status_access(dev, attr, false);
	default:
		return -EINVAL;
	}
}

static int kvm_loongarch_extioi_set_attr(struct kvm_device *dev,
				struct kvm_device_attr *attr)
{
	switch (attr->group) {

	case KVM_DEV_LOONGARCH_EXTIOI_GRP_CTRL:
		return kvm_extioi_ctrl_access(dev, attr);
	case KVM_DEV_LOONGARCH_EXTIOI_GRP_REGS:
		return kvm_loongarch_extioi_regs_access(dev, attr, true);
	case KVM_DEV_LOONGARCH_EXTIOI_GRP_SW_STATUS:
		return kvm_extioi_sw_status_access(dev, attr, true);
	}


	return -EINVAL;
}

static int kvm_extioi_virt_read(struct kvm_vcpu *vcpu,
				struct kvm_io_device *dev,
				gpa_t addr, int len, void *val)
{
	unsigned long flags;
	u32 *data = val;
	struct loongarch_extioi *extioi = vcpu->kvm->arch.extioi;

	if (!extioi) {
		kvm_err("%s: extioi irqchip not valid!\n", __func__);
		return -EINVAL;
	}

	addr -= EIOINTC_VIRT_BASE;
	spin_lock_irqsave(&extioi->lock, flags);
	switch (addr) {
	case EIOINTC_VIRT_FEATURES:
		*data = extioi->features;
		break;
	case EIOINTC_VIRT_CONFIG:
		*data = extioi->status;
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&extioi->lock, flags);

	return 0;
}

static int kvm_extioi_virt_write(struct kvm_vcpu *vcpu,
				struct kvm_io_device *dev,
				gpa_t addr, int len, const void *val)
{
	int ret = 0;
	unsigned long flags;
	u32 value = *(u32 *)val;
	struct loongarch_extioi *extioi = vcpu->kvm->arch.extioi;

	if (!extioi) {
		kvm_err("%s: extioi irqchip not valid!\n", __func__);
		return -EINVAL;
	}

	addr -= EIOINTC_VIRT_BASE;
	spin_lock_irqsave(&extioi->lock, flags);
	switch (addr) {
	case EIOINTC_VIRT_FEATURES:
		ret = -EPERM;
		break;
	case EIOINTC_VIRT_CONFIG:
		/*
		 * extioi features can only be set at disabled status
		 */
		if ((extioi->status & BIT(EIOINTC_ENABLE)) && value) {
			ret = -EPERM;
			break;
		}
		extioi->status = value & extioi->features;
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&extioi->lock, flags);

	return ret;
}

static const struct kvm_io_device_ops kvm_extioi_virt_ops = {
	.read	= kvm_extioi_virt_read,
	.write	= kvm_extioi_virt_write,
};

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
	kvm_io_bus_unregister_dev(kvm, KVM_IOCSR_BUS, &extioi->device_vext);
	kfree(extioi);
}

static int kvm_loongarch_extioi_create(struct kvm_device *dev, u32 type)
{
	int ret;
	struct loongarch_extioi *s;
	struct kvm_io_device *device, *device1;
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

	device1 = &s->device_vext;
	kvm_iodevice_init(device1, &kvm_extioi_virt_ops);
	ret = kvm_io_bus_register_dev(kvm, KVM_IOCSR_BUS,
			EIOINTC_VIRT_BASE, EIOINTC_VIRT_SIZE, device1);
	if (ret < 0) {
		kvm_io_bus_unregister_dev(kvm, KVM_IOCSR_BUS, &s->device);
		kfree(s);
		return ret;
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

int kvm_loongarch_reset_extioi(struct kvm *kvm)
{
	struct loongarch_extioi *extioi = kvm->arch.extioi;
	unsigned long flags;
	unsigned long offset, size;
	u8 *pstart;

	if (!extioi)
		return -EINVAL;

	pstart = (char *)&extioi->nodetype;
	offset = (char *)&extioi->nodetype - (char *)extioi;
	size = sizeof(struct loongarch_extioi) - offset;

	loongarch_ext_irq_lock(extioi, flags);
	memset(pstart, 0, size);
	loongarch_ext_irq_unlock(extioi, flags);

	return 0;
}
