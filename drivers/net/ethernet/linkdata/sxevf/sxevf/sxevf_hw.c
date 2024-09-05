#if defined (__KERNEL__) || defined (SXE_KERNEL_TEST) 
#include <linux/etherdevice.h>

#include "sxevf_hw.h"
#include "sxevf_regs.h"
#include "sxe_log.h"
#include "sxevf_irq.h"
#include "sxevf_msg.h"
#include "sxevf_ring.h"
#include "sxevf.h"
#include "sxevf_rx_proc.h"
#else 
#include "sxe_errno.h"
#include "sxe_logs.h"
#include "sxe_dpdk_version.h"
#include "sxe_compat_version.h"
#include "sxevf.h"
#include "sxevf_hw.h"
#endif

#if defined SXE_DPDK_L4_FEATURES && defined SXE_DPDK_SRIOV
struct sxevf_adapter;
#endif

#define DMA_BIT_MASK(n)	(((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#define DMA_MASK_NONE	0x0ULL

#define  SXEVF_REG_READ_CNT    5

#define SXE_REG_READ_FAIL       0xffffffffU

#define SXEVF_RING_WAIT_LOOP                   (100)
#define SXEVF_MAX_RX_DESC_POLL                 (10)


#define SXEVF_REG_READ(hw, addr)        sxevf_reg_read(hw, addr)
#define SXEVF_REG_WRITE(hw, reg, value) sxevf_reg_write(hw, reg, value)
#define SXEVF_WRITE_FLUSH(a) sxevf_reg_read(a, SXE_VFSTATUS)

#ifndef SXE_DPDK 
void sxevf_hw_fault_handle(struct sxevf_hw *hw)
{
	struct sxevf_adapter *adapter = hw->adapter;

	if (test_bit(SXEVF_HW_FAULT, &hw->state)) {
		goto l_ret;
	}

	set_bit(SXEVF_HW_FAULT, &hw->state);

	LOG_DEV_ERR("sxe nic hw fault\n");

	if ((hw->fault_handle != NULL) && (hw->priv != NULL) ) {
		hw->fault_handle(hw->priv);
	}

l_ret:
	return;
}

static void sxevf_hw_fault_check(struct sxevf_hw *hw, u32 reg)
{
	u32 value;
	u8  __iomem *base_addr = hw->reg_base_addr;
	struct sxevf_adapter *adapter = hw->adapter;
	u8 i;

	if (reg == SXE_VFSTATUS) {
		sxevf_hw_fault_handle(hw);
		return;
	}


	for (i = 0; i < SXEVF_REG_READ_CNT; i++) {
		value = hw->reg_read(base_addr + SXE_VFSTATUS);

		if (value != SXEVF_REG_READ_FAIL) {
			break;
		}

		mdelay(20);
	}

	LOG_INFO_BDF("retry done i:%d value:0x%x\n", i, value);

	if (value == SXEVF_REG_READ_FAIL) {
		sxevf_hw_fault_handle(hw);
	}

	return;
}

STATIC u32 sxevf_reg_read(struct sxevf_hw *hw, u32 reg)
{
	u32 value;
	u8  __iomem *base_addr = hw->reg_base_addr;
	struct sxevf_adapter *adapter = hw->adapter;

	if (sxevf_is_hw_fault(hw)) {
		value = SXEVF_REG_READ_FAIL;
		goto l_ret;
	}

	value = hw->reg_read(base_addr + reg);
	if (unlikely(SXEVF_REG_READ_FAIL == value)) {
		LOG_ERROR_BDF("reg[0x%x] read failed, value=%#x\n", reg, value);
		sxevf_hw_fault_check(hw, reg);
	}

l_ret:
	return value;
}

STATIC void sxevf_reg_write(struct sxevf_hw *hw, u32 reg, u32 value)
{
	u8 __iomem *base_addr = hw->reg_base_addr;

	if (sxevf_is_hw_fault(hw)) {
		goto l_ret;
	}

	hw->reg_write(value, base_addr + reg);

l_ret:
	return;
}

#else 

STATIC u32 sxevf_reg_read(struct sxevf_hw *hw, u32 reg)
{
	u32 i, value;
	u8  __iomem *base_addr = hw->reg_base_addr;

	value = rte_le_to_cpu_32(rte_read32(base_addr + reg));
	if (unlikely(SXEVF_REG_READ_FAIL == value)) {
		for (i = 0; i < SXEVF_REG_READ_CNT; i++) {
			LOG_ERROR("reg[0x%x] read failed, value=%#x\n",
							reg, value);
			value = rte_le_to_cpu_32(rte_read32(base_addr + reg));
			if (value != SXEVF_REG_READ_FAIL) {
				LOG_INFO("reg[0x%x] read ok, value=%#x\n",
								reg, value);
				break;
			}

			mdelay(3);
		}
	}

	return value;
}

STATIC void sxevf_reg_write(struct sxevf_hw *hw, u32 reg, u32 value)
{
	u8 __iomem *base_addr = hw->reg_base_addr;

	rte_write32((rte_cpu_to_le_32(value)), (base_addr + reg));

	return;
}
#endif

void sxevf_hw_stop(struct sxevf_hw *hw)
{
	u8 i;
	u32 value;

	for (i = 0; i < SXEVF_TXRX_RING_NUM_MAX; i++) {
		value = SXEVF_REG_READ(hw, SXE_VFRXDCTL(i));
		if (value & SXE_VFRXDCTL_ENABLE) {
			value &= ~SXE_VFRXDCTL_ENABLE;
			SXEVF_REG_WRITE(hw, SXE_VFRXDCTL(i), value);
		}
	}

	SXEVF_WRITE_FLUSH(hw);

	SXEVF_REG_WRITE(hw, SXE_VFEIMC, SXEVF_VFEIMC_IRQ_MASK);
	SXEVF_REG_READ(hw, SXE_VFEICR);

	for (i = 0; i < SXEVF_TXRX_RING_NUM_MAX; i++) {
		value = SXEVF_REG_READ(hw, SXE_VFTXDCTL(i));
		if (value & SXE_VFTXDCTL_ENABLE) {
			value &= ~SXE_VFTXDCTL_ENABLE;
			SXEVF_REG_WRITE(hw, SXE_VFTXDCTL(i), value);
		}
	}

	return;
}

void sxevf_msg_write(struct sxevf_hw *hw, u8 index, u32 msg)
{
	struct sxevf_adapter *adapter = hw->adapter;

	SXEVF_REG_WRITE(hw, SXE_VFMBMEM + (index << 2), msg);

	LOG_DEBUG_BDF("index:%u write mbx mem:0x%x.\n", index, msg);

	return;
}

u32 sxevf_msg_read(struct sxevf_hw *hw, u8 index)
{
	u32 value = SXEVF_REG_READ(hw, SXE_VFMBMEM + (index << 2));
	struct sxevf_adapter *adapter = hw->adapter;

	LOG_DEBUG_BDF("index:%u read mbx mem:0x%x.\n", index, value);

	return value;
}

u32 sxevf_mailbox_read(struct sxevf_hw *hw)
{
	return SXEVF_REG_READ(hw, SXE_VFMAILBOX);
}

void sxevf_mailbox_write(struct sxevf_hw *hw, u32 value)
{
	SXEVF_REG_WRITE(hw, SXE_VFMAILBOX, value);
	return;
}

void sxevf_pf_req_irq_trigger(struct sxevf_hw *hw)
{
	SXEVF_REG_WRITE(hw, SXE_VFMAILBOX, SXE_VFMAILBOX_REQ);

	return;
}

void sxevf_pf_ack_irq_trigger(struct sxevf_hw *hw)
{
	SXEVF_REG_WRITE(hw, SXE_VFMAILBOX, SXE_VFMAILBOX_ACK);

	return;
}

void sxevf_event_irq_map(struct sxevf_hw *hw, u16 vector)
{
	u8  allocation;
	u32 ivar;

	allocation = vector | SXEVF_IVAR_ALLOC_VALID;

	ivar = SXEVF_REG_READ(hw, SXE_VFIVAR_MISC);
	ivar &= ~0xFF;
	ivar |= allocation;

	SXEVF_REG_WRITE(hw, SXE_VFIVAR_MISC, ivar);

	return;
}

void sxevf_specific_irq_enable(struct sxevf_hw *hw, u32 value)
{
	SXEVF_REG_WRITE(hw, SXE_VFEIMS, value);

	return;
}

void sxevf_irq_enable(struct sxevf_hw *hw, u32 mask)
{
	SXEVF_REG_WRITE(hw, SXE_VFEIAM, mask);
	SXEVF_REG_WRITE(hw, SXE_VFEIMS, mask);

	return;
}

void sxevf_irq_disable(struct sxevf_hw *hw)
{
	SXEVF_REG_WRITE(hw, SXE_VFEIAM, 0);
	SXEVF_REG_WRITE(hw, SXE_VFEIMC, ~0);

	SXEVF_WRITE_FLUSH(hw);

	return;
}

void sxevf_hw_ring_irq_map(struct sxevf_hw *hw, bool is_tx, u16 hw_ring_idx, u16 vector)
{
	u8  allocation;
	u32 ivar, position;

	allocation = vector | SXEVF_IVAR_ALLOC_VALID;

	position = ((hw_ring_idx & 1) * 16) + (8 * is_tx);

	ivar = SXEVF_REG_READ(hw, SXE_VFIVAR(hw_ring_idx >> 1));
	ivar &= ~(0xFF << position);
	ivar |= (allocation << position);

	SXEVF_REG_WRITE(hw, SXE_VFIVAR(hw_ring_idx >> 1), ivar);

	return;
}

void sxevf_ring_irq_interval_set(struct sxevf_hw *hw, u16 irq_idx, u32 interval)
{
	u32 eitr = interval & SXEVF_EITR_ITR_MASK;

	eitr |= SXEVF_EITR_CNT_WDIS;

	SXEVF_REG_WRITE(hw, SXE_VFEITR(irq_idx), eitr);

	return;
}

static void sxevf_event_irq_interval_set(struct sxevf_hw *hw, u16 irq_idx, u32 value)
{
	SXEVF_REG_WRITE(hw, SXE_VFEITR(irq_idx), value);

	return;
}

static void sxevf_pending_irq_clear(struct sxevf_hw *hw)
{
	SXEVF_REG_READ(hw, SXE_VFEICR);

	return;
}

static void sxevf_ring_irq_trigger(struct sxevf_hw *hw, u64 eics)
{
	SXEVF_REG_WRITE(hw, SXE_VFEICS, eics);

	return;
}

static const struct sxevf_irq_operations sxevf_irq_ops = {
	.ring_irq_interval_set   = sxevf_ring_irq_interval_set,
	.event_irq_interval_set  = sxevf_event_irq_interval_set,
	.ring_irq_map         = sxevf_hw_ring_irq_map,
	.event_irq_map           = sxevf_event_irq_map,
	.pending_irq_clear       = sxevf_pending_irq_clear,
	.ring_irq_trigger        = sxevf_ring_irq_trigger,
	.specific_irq_enable     = sxevf_specific_irq_enable,
	.irq_enable              = sxevf_irq_enable,
	.irq_disable             = sxevf_irq_disable,
};

void sxevf_hw_reset(struct sxevf_hw *hw)
{
	SXEVF_REG_WRITE(hw, SXE_VFCTRL, SXE_VFCTRL_RST);
	SXEVF_WRITE_FLUSH(hw);

	return;
}

STATIC bool sxevf_hw_rst_done(struct sxevf_hw *hw)
{
	return !(SXEVF_REG_READ(hw, SXE_VFCTRL) & SXE_VFCTRL_RST);
}

u32 sxevf_link_state_get(struct sxevf_hw *hw)
{
	return SXEVF_REG_READ(hw, SXE_VFLINKS);
}

u32 dump_regs[] = {
	SXE_VFCTRL,
};

u16 sxevf_reg_dump_num_get(void)
{
	return ARRAY_SIZE(dump_regs);
}

static u32 sxevf_reg_dump(struct sxevf_hw *hw, u32 *regs_buff, u32 buf_size)
{
	u32 i;
	u32 regs_num = buf_size / sizeof(u32);

	for (i = 0; i < regs_num; i++) {
		regs_buff[i] = SXEVF_REG_READ(hw, dump_regs[i]);
	}

	return i;
}

#define PATTERN_TEST	1
#define SET_READ_TEST	2
#define WRITE_NO_TEST	3
#define TABLE32_TEST	4
#define TABLE64_TEST_LO	5
#define TABLE64_TEST_HI	6

struct sxevf_self_test_reg {
	u32 reg;
	u8  array_len;
	u8  test_type;
	u32 mask;
	u32 write;
};

static const struct sxevf_self_test_reg self_test_reg[] = {
	{ SXE_VFRDBAL(0), 2, PATTERN_TEST, 0xFFFFFF80, 0xFFFFFF80 },
	{ SXE_VFRDBAH(0), 2, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ SXE_VFRDLEN(0), 2, PATTERN_TEST, 0x000FFFFF, 0x000FFFFF },
	{ SXE_VFRXDCTL(0), 2, WRITE_NO_TEST, 0, SXEVF_RXDCTL_ENABLE },
	{ SXE_VFRDT(0), 2, PATTERN_TEST, 0x0000FFFF, 0x0000FFFF },
	{ SXE_VFRXDCTL(0), 2, WRITE_NO_TEST, 0, 0 },
	{ SXE_VFTDBAL(0), 2, PATTERN_TEST, 0xFFFFFF80, 0xFFFFFFFF },
	{ SXE_VFTDBAH(0), 2, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ SXE_VFTDLEN(0), 2, PATTERN_TEST, 0x000FFF80, 0x000FFF80 },
	{ .reg = 0 }
};

static s32 sxevf_reg_pattern_test(struct sxevf_hw *hw, u32 reg,
				u32 mask, u32 write)
{
	s32 ret = 0;
	u32 pat, val, before;
	static const u32 test_pattern[] = {
		0x5A5A5A5A, 0xA5A5A5A5, 0x00000000, 0xFFFFFFFE};
	struct sxevf_adapter *adapter = hw->adapter;

	if (sxevf_is_hw_fault(hw)) {
		LOG_ERROR_BDF("hw fault\n");
		ret = -SXEVF_DIAG_TEST_BLOCKED;
		goto l_end;
	}

	for (pat = 0; pat < ARRAY_SIZE(test_pattern); pat++) {
		before = SXEVF_REG_READ(hw, reg);

		SXEVF_REG_WRITE(hw, reg, test_pattern[pat] & write);
		val = SXEVF_REG_READ(hw, reg);
		if (val != (test_pattern[pat] & write & mask)) {
			LOG_MSG_ERR(drv, "pattern test reg %04X failed: "
					"got 0x%08X expected 0x%08X\n",
				reg, val, (test_pattern[pat] & write & mask));
			SXEVF_REG_WRITE(hw, reg, before);
			ret = -SXEVF_DIAG_REG_PATTERN_TEST_ERR;
			goto l_end;
		}

		SXEVF_REG_WRITE(hw, reg, before);
	}

l_end:
	return ret;
}

static s32 sxevf_reg_set_and_check(struct sxevf_hw *hw, int reg,
				u32 mask, u32 write)
{
	s32 ret = 0;
	u32 val, before;
	struct sxevf_adapter *adapter = hw->adapter;

	if (sxevf_is_hw_fault(hw)) {
		LOG_ERROR_BDF("hw fault\n");
		ret = -SXEVF_DIAG_TEST_BLOCKED;
		goto l_end;
	}

	before = SXEVF_REG_READ(hw, reg);
	SXEVF_REG_WRITE(hw, reg, write & mask);
	val = SXEVF_REG_READ(hw, reg);
	if ((write & mask) != (val & mask)) {
		LOG_DEV_ERR("set/check reg %04X test failed: "
				"got 0x%08X expected 0x%08X\n",
			reg, (val & mask), (write & mask));
		SXEVF_REG_WRITE(hw, reg, before);
		ret = -SXEVF_DIAG_CHECK_REG_TEST_ERR;
		goto l_end;
	}

	SXEVF_REG_WRITE(hw, reg, before);

l_end:
	return ret;
}

STATIC s32 sxevf_regs_test(struct sxevf_hw *hw)
{
	u32 i;
	s32 ret = 0;
	const struct sxevf_self_test_reg *test = self_test_reg;
	struct sxevf_adapter *adapter = hw->adapter;

	while (test->reg) {
		for (i = 0; i < test->array_len; i++) {
			switch (test->test_type) {
			case PATTERN_TEST:
				ret = sxevf_reg_pattern_test(hw,
					test->reg + (i * 0x40),
					test->mask, test->write);
				break;
			case TABLE32_TEST:
				ret = sxevf_reg_pattern_test(hw,
					test->reg + (i * 4),
					test->mask, test->write);
				break;
			case TABLE64_TEST_LO:
				ret = sxevf_reg_pattern_test(hw,
					test->reg + (i * 8),
					test->mask, test->write);
				break;
			case TABLE64_TEST_HI:
				ret = sxevf_reg_pattern_test(hw,
					(test->reg + 4) + (i * 8),
					test->mask, test->write);
				break;
			case SET_READ_TEST:
				ret = sxevf_reg_set_and_check(hw,
					test->reg + (i * 0x40),
					test->mask, test->write);
				break;
			case WRITE_NO_TEST:
				SXEVF_REG_WRITE(hw, test->reg + (i * 0x40),
						test->write);
				break;
			default:
				LOG_ERROR_BDF("reg test mod err, type=%d\n",
						test->test_type);
				break;
			}

			if (ret) {
				goto l_end;
			}

		}
		test++;
	}

l_end:
	return ret;
}

static const struct sxevf_setup_operations sxevf_setup_ops = {
	.reset		= sxevf_hw_reset,
	.hw_stop	= sxevf_hw_stop,
	.regs_test	= sxevf_regs_test,
	.regs_dump	= sxevf_reg_dump,
	.link_state_get	= sxevf_link_state_get,
	.reset_done = sxevf_hw_rst_done,
};

static void sxevf_tx_ring_desc_configure(struct sxevf_hw *hw, u32 desc_mem_len,
				u64 desc_dma_addr, u8 reg_idx)
{
	SXEVF_REG_WRITE(hw, SXEVF_TDBAL(reg_idx), (desc_dma_addr & \
						DMA_BIT_MASK(32)));
	SXEVF_REG_WRITE(hw, SXEVF_TDBAH(reg_idx), (desc_dma_addr >> 32));
	SXEVF_REG_WRITE(hw, SXEVF_TDLEN(reg_idx), desc_mem_len);
	SXEVF_REG_WRITE(hw, SXEVF_TDH(reg_idx), 0);
	SXEVF_REG_WRITE(hw, SXEVF_TDT(reg_idx), 0);

	return;
}

static void sxevf_tx_writeback_off(struct sxevf_hw *hw, u8 reg_idx)
{
	SXEVF_REG_WRITE(hw, SXEVF_TDWBAH(reg_idx), 0);
	SXEVF_REG_WRITE(hw, SXEVF_TDWBAL(reg_idx), 0);

	return;
}

static void sxevf_tx_desc_thresh_set(
				struct sxevf_hw *hw,
				u8 reg_idx,
				u32 wb_thresh,
				u32 host_thresh,
				u32 prefech_thresh)
{
	u32 txdctl = 0;

	txdctl |= (wb_thresh << SXEVF_TXDCTL_WTHRESH_SHIFT);
	txdctl |= (host_thresh << SXEVF_TXDCTL_HTHRESH_SHIFT) |
			prefech_thresh;

	SXEVF_REG_WRITE(hw, SXEVF_TXDCTL(reg_idx), txdctl);

	return;
}

void sxevf_tx_ring_switch(struct sxevf_hw *hw, u8 reg_idx, bool is_on)
{
	u32 wait_loop = SXEVF_MAX_TXRX_DESC_POLL;
	struct sxevf_adapter *adapter = hw->adapter;

	u32 txdctl = SXEVF_REG_READ(hw, SXEVF_TXDCTL(reg_idx));
	if (is_on) {
		txdctl |= SXEVF_TXDCTL_ENABLE;
		SXEVF_REG_WRITE(hw, SXEVF_TXDCTL(reg_idx), txdctl);

		do {
			usleep_range(1000, 2000);
			txdctl = SXEVF_REG_READ(hw, SXEVF_TXDCTL(reg_idx));
		} while (--wait_loop && !(txdctl & SXEVF_TXDCTL_ENABLE));
	} else {
		txdctl &= ~SXEVF_TXDCTL_ENABLE;
		SXEVF_REG_WRITE(hw, SXEVF_TXDCTL(reg_idx), txdctl);

		do {
			usleep_range(1000, 2000);
			txdctl = SXEVF_REG_READ(hw, SXEVF_TXDCTL(reg_idx));
		} while (--wait_loop && (txdctl & SXEVF_TXDCTL_ENABLE));
	}

	if (!wait_loop) {
		LOG_DEV_ERR("tx ring %u switch %u failed within "
			  "the polling period\n", reg_idx, is_on);
	}

	return;
}

static void sxevf_rx_disable(struct sxevf_hw *hw, u8 reg_idx)
{
	u32 rxdctl;
	u32 wait_loop = SXEVF_RX_RING_POLL_MAX;
	struct sxevf_adapter *adapter = hw->adapter;

	if (!hw->reg_base_addr) {
		goto l_end;
	}

	rxdctl = SXEVF_REG_READ(hw, SXE_VFRXDCTL(reg_idx));
	rxdctl &= ~SXE_VFRXDCTL_ENABLE;
	SXEVF_REG_WRITE(hw, SXE_VFRXDCTL(reg_idx), rxdctl);

	do {
		udelay(10);
		rxdctl = SXEVF_REG_READ(hw, SXE_VFRXDCTL(reg_idx));
	} while (--wait_loop && (rxdctl & SXE_VFRXDCTL_ENABLE));

	if (!wait_loop) {
		LOG_ERROR_BDF("RXDCTL.ENABLE queue %d not cleared while polling\n",
				reg_idx);
	}

l_end:
	return;
}

void sxevf_rx_ring_switch(struct sxevf_hw *hw, u8 reg_idx, bool is_on)
{
	u32 rxdctl;
	u32 wait_loop = SXEVF_RING_WAIT_LOOP;
	struct sxevf_adapter *adapter = hw->adapter;

	rxdctl = SXEVF_REG_READ(hw, SXE_VFRXDCTL(reg_idx));
	if (is_on) {
		rxdctl |= SXEVF_RXDCTL_ENABLE | SXEVF_RXDCTL_VME;
		SXEVF_REG_WRITE(hw, SXE_VFRXDCTL(reg_idx), rxdctl);

		do {
			usleep_range(1000, 2000);
			rxdctl = SXEVF_REG_READ(hw, SXE_VFRXDCTL(reg_idx));
		} while (--wait_loop && !(rxdctl & SXEVF_RXDCTL_ENABLE));
	} else {
		rxdctl &= ~SXEVF_RXDCTL_ENABLE;
		SXEVF_REG_WRITE(hw, SXE_VFRXDCTL(reg_idx), rxdctl);

		do {
			usleep_range(1000, 2000);
			rxdctl = SXEVF_REG_READ(hw, SXE_VFRXDCTL(reg_idx));
		} while (--wait_loop && (rxdctl & SXEVF_RXDCTL_ENABLE));
	}

	SXEVF_WRITE_FLUSH(hw);

	if (!wait_loop) {
		LOG_DEV_ERR("rx ring %u switch %u failed within "
			  "the polling period\n", reg_idx, is_on);
	}

	return;
}

void sxevf_rx_ring_desc_configure(struct sxevf_hw *hw, u32 desc_mem_len,
					u64 desc_dma_addr, u8 reg_idx)
{
	SXEVF_REG_WRITE(hw, SXE_VFRDBAL(reg_idx),
			(desc_dma_addr & DMA_BIT_MASK(32)));
	SXEVF_REG_WRITE(hw, SXE_VFRDBAH(reg_idx), (desc_dma_addr >> 32));
	SXEVF_REG_WRITE(hw, SXE_VFRDLEN(reg_idx), desc_mem_len);

	SXEVF_WRITE_FLUSH(hw);

	SXEVF_REG_WRITE(hw, SXE_VFRDH(reg_idx), 0);
	SXEVF_REG_WRITE(hw, SXE_VFRDT(reg_idx), 0);

	return;
}

void sxevf_rx_rcv_ctl_configure(struct sxevf_hw *hw, u8 reg_idx,
				   u32 header_buf_len, u32 pkg_buf_len, bool drop_en)
{
	u32 srrctl = 0;

	if (drop_en) {
		srrctl = SXEVF_SRRCTL_DROP_EN;
	}

	srrctl |= ((header_buf_len << SXEVF_SRRCTL_BSIZEHDRSIZE_SHIFT) &
			SXEVF_SRRCTL_BSIZEHDR_MASK);
	srrctl |= ((pkg_buf_len >> SXEVF_SRRCTL_BSIZEPKT_SHIFT) &
			SXEVF_SRRCTL_BSIZEPKT_MASK);

	SXEVF_REG_WRITE(hw, SXE_VFSRRCTL(reg_idx), srrctl);

	return;
}

static void sxevf_tx_ring_info_get(struct sxevf_hw *hw,
				u8 idx, u32 *head, u32 *tail)
{
	*head = SXEVF_REG_READ(hw, SXE_VFTDH(idx));
	*tail = SXEVF_REG_READ(hw, SXE_VFTDT(idx));

	return;
}

static const struct sxevf_dma_operations sxevf_dma_ops = {
	.tx_ring_desc_configure  = sxevf_tx_ring_desc_configure,
	.tx_writeback_off      = sxevf_tx_writeback_off,
	.tx_desc_thresh_set    = sxevf_tx_desc_thresh_set,
	.tx_ring_switch        = sxevf_tx_ring_switch,
	.tx_ring_info_get      = sxevf_tx_ring_info_get,

	.rx_disable          = sxevf_rx_disable,
	.rx_ring_switch      = sxevf_rx_ring_switch,
	.rx_ring_desc_configure= sxevf_rx_ring_desc_configure,
	.rx_rcv_ctl_configure  = sxevf_rx_rcv_ctl_configure,
};

#ifdef SXE_DPDK
#define SXEVF_32BIT_COUNTER_UPDATE(reg, last, cur)                          \
	{																\
		u32 latest = SXEVF_REG_READ(hw, reg);				\
		cur += (latest - last) & UINT_MAX;						\
		last = latest;											\
	}
	
#define SXEVF_36BIT_COUNTER_UPDATE(lsb, msb, last, cur)                \
	{																 \
		u64 new_lsb = SXEVF_REG_READ(hw, lsb);					 \
		u64 new_msb = SXEVF_REG_READ(hw, msb);					 \
		u64 latest = ((new_msb << 32) | new_lsb);				 \
		cur += (0x1000000000LL + latest - last) & 0xFFFFFFFFFLL; \
		last = latest;											 \
	}

#else
#define SXEVF_32BIT_COUNTER_UPDATE(reg, last_counter, counter)	\
	{							\
		u32 current_counter = SXEVF_REG_READ(hw, reg);	\
		if (current_counter < last_counter)		\
			counter += 0x100000000LL;		\
		last_counter = current_counter;			\
		counter &= 0xFFFFFFFF00000000LL;		\
		counter |= current_counter;			\
	}

#define SXEVF_36BIT_COUNTER_UPDATE(reg_lsb, reg_msb, last_counter, counter) \
	{								 \
		u64 current_counter_lsb = SXEVF_REG_READ(hw, reg_lsb);	 \
		u64 current_counter_msb = SXEVF_REG_READ(hw, reg_msb);	 \
		u64 current_counter = (current_counter_msb << 32) |	 \
			current_counter_lsb;				 \
		if (current_counter < last_counter)			 \
			counter += 0x1000000000LL;			 \
		last_counter = current_counter;				 \
		counter &= 0xFFFFFFF000000000LL;			 \
		counter |= current_counter;				 \
	}
#endif

void sxevf_packet_stats_get(struct sxevf_hw *hw,
				struct sxevf_hw_stats *stats)
{
	SXEVF_32BIT_COUNTER_UPDATE(SXEVF_VFGPRC, stats->last_vfgprc,
				stats->vfgprc);
	SXEVF_32BIT_COUNTER_UPDATE(SXEVF_VFGPTC, stats->last_vfgptc,
				stats->vfgptc);
	SXEVF_36BIT_COUNTER_UPDATE(SXEVF_VFGORC_LSB, SXEVF_VFGORC_MSB,
				stats->last_vfgorc,
				stats->vfgorc);
	SXEVF_36BIT_COUNTER_UPDATE(SXEVF_VFGOTC_LSB, SXEVF_VFGOTC_MSB,
				stats->last_vfgotc,
				stats->vfgotc);
	SXEVF_32BIT_COUNTER_UPDATE(SXEVF_VFMPRC, stats->last_vfmprc,
				stats->vfmprc);

	return;
}

void sxevf_stats_init_value_get(struct sxevf_hw *hw,
				struct sxevf_hw_stats *stats)
{
	stats->last_vfgprc = SXEVF_REG_READ(hw, SXE_VFGPRC);
	stats->last_vfgorc = SXEVF_REG_READ(hw, SXE_VFGORC_LSB);
	stats->last_vfgorc |= (((u64)(SXEVF_REG_READ(hw, SXE_VFGORC_MSB))) << 32);
	stats->last_vfgptc = SXEVF_REG_READ(hw, SXE_VFGPTC);
	stats->last_vfgotc = SXEVF_REG_READ(hw, SXE_VFGOTC_LSB);
	stats->last_vfgotc |= (((u64)(SXEVF_REG_READ(hw, SXE_VFGOTC_MSB))) << 32);
	stats->last_vfmprc = SXEVF_REG_READ(hw, SXE_VFMPRC);

	return;
}
static const struct sxevf_stat_operations sxevf_stat_ops = {
	.packet_stats_get	= sxevf_packet_stats_get,
	.stats_init_value_get	= sxevf_stats_init_value_get,
};

static void sxevf_rx_max_used_ring_set(struct sxevf_hw *hw, u16 max_rx_ring)
{
	u32 rqpl = 0;

	if (max_rx_ring > 1) {
		rqpl |= BIT(29);
	}

	SXEVF_REG_WRITE(hw, SXE_VFPSRTYPE, rqpl);

	return;
}

static const struct sxevf_dbu_operations sxevf_dbu_ops = {
	.rx_max_used_ring_set = sxevf_rx_max_used_ring_set,
};

static const struct sxevf_mbx_operations sxevf_mbx_ops = {

	.mailbox_read = sxevf_mailbox_read,
	.mailbox_write = sxevf_mailbox_write,

	.msg_write = sxevf_msg_write,
	.msg_read = sxevf_msg_read,

	.pf_req_irq_trigger = sxevf_pf_req_irq_trigger,
	.pf_ack_irq_trigger = sxevf_pf_ack_irq_trigger,
};

void sxevf_hw_ops_init(struct sxevf_hw *hw)
{
	hw->setup.ops   = &sxevf_setup_ops;
	hw->irq.ops     = &sxevf_irq_ops;
	hw->mbx.ops     = &sxevf_mbx_ops;
	hw->dma.ops     = &sxevf_dma_ops;
	hw->stat.ops    = &sxevf_stat_ops;
	hw->dbu.ops     = &sxevf_dbu_ops;

	return;
}

#ifdef SXE_DPDK 

#define SXEVF_RSS_FIELD_MASK        0xffff0000
#define SXEVF_MRQC_RSSEN            0x00000001 

#define SXEVF_RSS_KEY_SIZE                (40)  
#define SXEVF_MAX_RSS_KEY_ENTRIES	  (10)  
#define SXEVF_MAX_RETA_ENTRIES            (128) 

void sxevf_rxtx_reg_init(struct sxevf_hw *hw)
{
	int i;
	u32 vfsrrctl;

	vfsrrctl = 0x100 << SXEVF_SRRCTL_BSIZEHDRSIZE_SHIFT;
	vfsrrctl |= 0x800 >> SXEVF_SRRCTL_BSIZEPKT_SHIFT;

	SXEVF_REG_WRITE(hw, SXE_VFPSRTYPE, 0);

	for (i = 0; i < 7; i++) {
		SXEVF_REG_WRITE(hw, SXE_VFRDH(i), 0);
		SXEVF_REG_WRITE(hw, SXE_VFRDT(i), 0);
		SXEVF_REG_WRITE(hw, SXE_VFRXDCTL(i), 0);
		SXEVF_REG_WRITE(hw, SXE_VFSRRCTL(i), vfsrrctl);
		SXEVF_REG_WRITE(hw, SXE_VFTDH(i), 0);
		SXEVF_REG_WRITE(hw, SXE_VFTDT(i), 0);
		SXEVF_REG_WRITE(hw, SXE_VFTXDCTL(i), 0);
		SXEVF_REG_WRITE(hw, SXE_VFTDWBAH(i), 0);
		SXEVF_REG_WRITE(hw, SXE_VFTDWBAL(i), 0);
	}

	SXEVF_WRITE_FLUSH(hw);

	return;
}

u32 sxevf_irq_cause_get(struct sxevf_hw *hw)
{
	return SXEVF_REG_READ(hw, SXE_VFEICR);
}

void sxevf_tx_desc_configure(struct sxevf_hw *hw, u32 desc_mem_len,
				u64 desc_dma_addr, u8 reg_idx)
{

	SXEVF_REG_WRITE(hw, SXEVF_TDBAL(reg_idx), (desc_dma_addr & \
						DMA_BIT_MASK(32)));
	SXEVF_REG_WRITE(hw, SXEVF_TDBAH(reg_idx), (desc_dma_addr >> 32));
	SXEVF_REG_WRITE(hw, SXEVF_TDLEN(reg_idx), desc_mem_len);
	SXEVF_REG_WRITE(hw, SXEVF_TDH(reg_idx), 0);
	SXEVF_REG_WRITE(hw, SXEVF_TDT(reg_idx), 0);

	return;
}

void sxevf_rss_bit_num_set(struct sxevf_hw *hw, u32 value)
{
	SXEVF_REG_WRITE(hw, SXE_VFPSRTYPE, value);

	return;
}

void sxevf_hw_vlan_tag_strip_switch(struct sxevf_hw *hw,
					u16 reg_index, bool is_enable)
{
	u32 vlnctrl;

	vlnctrl = SXEVF_REG_READ(hw, SXE_VFRXDCTL(reg_index));

	if (is_enable) {
		vlnctrl |= SXEVF_RXDCTL_VME;
	} else {
		vlnctrl &= ~SXEVF_RXDCTL_VME;
	}

	SXEVF_REG_WRITE(hw, SXE_VFRXDCTL(reg_index), vlnctrl);

	return;
}

void sxevf_tx_queue_thresh_set(struct sxevf_hw *hw, u8 reg_idx,
				u32 prefech_thresh, u32 host_thresh, u32 wb_thresh)
{
	u32 txdctl = SXEVF_REG_READ(hw, SXEVF_TXDCTL(reg_idx));

	txdctl |= (prefech_thresh & SXEVF_TXDCTL_THRESH_MASK);
	txdctl |= ((host_thresh & SXEVF_TXDCTL_THRESH_MASK) << SXEVF_TXDCTL_HTHRESH_SHIFT);
	txdctl |= ((wb_thresh & SXEVF_TXDCTL_THRESH_MASK)<< SXEVF_TXDCTL_WTHRESH_SHIFT);

	SXEVF_REG_WRITE(hw, SXEVF_TXDCTL(reg_idx), txdctl);

	return;
}

void sxevf_rx_desc_tail_set(struct sxevf_hw *hw, u8 reg_idx, u32 value)
{
	SXEVF_REG_WRITE(hw, SXE_VFRDT(reg_idx), value);

	return;
}

u32 sxevf_hw_rss_redir_tbl_get(struct sxevf_hw *hw, u16 reg_idx)
{
	return SXEVF_REG_READ(hw, SXE_VFRETA(reg_idx >> 2));
}

void sxevf_hw_rss_redir_tbl_set(struct sxevf_hw *hw,
						u16 reg_idx, u32 value)
{
	SXEVF_REG_WRITE(hw, SXE_VFRETA(reg_idx >> 2), value);
	return;
}

u32 sxevf_hw_rss_key_get(struct sxevf_hw *hw, u8 reg_idx)
{
	u32 rss_key;

	if (reg_idx >= SXEVF_MAX_RSS_KEY_ENTRIES) {
		rss_key = 0;
	} else {
		rss_key = SXEVF_REG_READ(hw, SXE_VFRSSRK(reg_idx));
	}

	return rss_key;
}

u32 sxevf_hw_rss_field_get(struct sxevf_hw *hw)
{
	u32 mrqc = SXEVF_REG_READ(hw, SXE_VFMRQC);
	return (mrqc & SXEVF_RSS_FIELD_MASK);
}

bool sxevf_hw_is_rss_enabled(struct sxevf_hw *hw)
{
	bool rss_enable = false;
	u32 mrqc = SXEVF_REG_READ(hw, SXE_VFMRQC);
	if (mrqc & SXEVF_MRQC_RSSEN) {
		rss_enable = true;
	}

	return rss_enable;
}

void sxevf_hw_rss_key_set_all(struct sxevf_hw *hw, u32 *rss_key)
{
	u32 i;

	for (i = 0; i < SXEVF_MAX_RSS_KEY_ENTRIES; i++) {
		SXEVF_REG_WRITE(hw, SXE_VFRSSRK(i), rss_key[i]);
	}

	return;
}

void sxevf_hw_rss_cap_switch(struct sxevf_hw *hw, bool is_on)
{
	u32 mrqc = SXEVF_REG_READ(hw, SXE_VFMRQC);
	if (is_on) {
		mrqc |= SXEVF_MRQC_RSSEN;
	} else {
		mrqc &= ~SXEVF_MRQC_RSSEN;
	}

	SXEVF_REG_WRITE(hw, SXE_VFMRQC, mrqc);

	return;
}

void sxevf_hw_rss_field_set(struct sxevf_hw *hw, u32 rss_field)
{
	u32 mrqc = SXEVF_REG_READ(hw, SXE_VFMRQC);

	mrqc &= ~SXEVF_RSS_FIELD_MASK;
	mrqc |= rss_field;
	SXEVF_REG_WRITE(hw, SXE_VFMRQC, mrqc);

	return;
}

u32 sxevf_hw_regs_group_read(struct sxevf_hw *hw,
				const struct sxevf_reg_info *regs,
				u32 *reg_buf)
{
	u32 j, i = 0;
	int count = 0;

	while (regs[i].count) {
		for (j = 0; j < regs[i].count; j++) {
			reg_buf[count + j] = SXEVF_REG_READ(hw,
					regs[i].addr + j * regs[i].stride);
			LOG_INFO("regs= %s, regs_addr=%x, regs_value=%04x\n",
				regs[i].name , regs[i].addr, reg_buf[count + j]);
		}

		i++;
		count += j;
	}

	return count;
};

#endif
