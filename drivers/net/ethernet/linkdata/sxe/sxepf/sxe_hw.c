#ifdef SXE_PHY_CONFIGURE
#include <linux/mdio.h>
#endif
#if defined (__KERNEL__) || defined (SXE_KERNEL_TEST)
#include "sxe_pci.h"
#include "sxe_log.h"
#include "sxe_debug.h"
#include "sxe_host_hdc.h"
#include "sxe_sriov.h"
#include "sxe_compat.h"
#else
#include "sxe_errno.h"
#include "sxe_logs.h"
#include "sxe.h"

#include "sxe_hw.h"
#endif


#define SXE_PFMSG_MASK  (0xFF00)

#define SXE_MSGID_MASK  (0xFFFFFFFF)

#define SXE_CTRL_MSG_MASK          (0x700)

#define SXE_RING_WAIT_LOOP        10
#define SXE_REG_NAME_LEN          16
#define SXE_DUMP_REG_STRING_LEN   73
#define SXE_DUMP_REGS_NUM         64
#define SXE_MAX_RX_DESC_POLL      10
#define SXE_LPBK_EN               0x00000001
#define SXE_MACADDR_LOW_4_BYTE    4
#define SXE_MACADDR_HIGH_2_BYTE   2
#define SXE_RSS_FIELD_MASK        0xffff0000
#define SXE_MRQE_MASK             0x0000000f

#define SXE_HDC_DATA_LEN_MAX         256

#define SXE_8_TC_MSB				(0x11111111)

STATIC u32 sxe_read_reg(struct sxe_hw *hw, u32 reg);
STATIC void sxe_write_reg(struct sxe_hw *hw, u32 reg, u32 value);
static void sxe_write_reg64(struct sxe_hw *hw, u32 reg, u64 value);

#define SXE_WRITE_REG_ARRAY_32(a, reg, offset, value) \
	sxe_write_reg(a, reg + (offset << 2), value)
#define SXE_READ_REG_ARRAY_32(a, reg, offset) \
	sxe_read_reg(a, reg + (offset << 2))

#define SXE_REG_READ(hw, addr)        sxe_read_reg(hw, addr)
#define SXE_REG_WRITE(hw, reg, value) sxe_write_reg(hw, reg, value)
#define SXE_WRITE_FLUSH(a) sxe_read_reg(a, SXE_STATUS)
#define SXE_REG_WRITE_ARRAY(hw, reg, offset, value) \
	sxe_write_reg(hw, (reg) + ((offset) << 2), (value))

#define SXE_SWAP_32(_value) __swab32((_value))

#define SXE_REG_WRITE_BE32(a, reg, value) \
	SXE_REG_WRITE((a), (reg), SXE_SWAP_32(ntohl(value)))

#define SXE_SWAP_16(_value) __swab16((_value))

#define SXE_REG64_WRITE(a, reg, value) sxe_write_reg64((a), (reg), (value))

enum sxe_ipsec_table {
	SXE_IPSEC_IP_TABLE = 0,
	SXE_IPSEC_SPI_TABLE,
	SXE_IPSEC_KEY_TABLE,
};

u32 mac_regs[] = {
	SXE_COMCTRL,
	SXE_PCCTRL,
	SXE_LPBKCTRL,
	SXE_MAXFS,
	SXE_VLANCTRL,
	SXE_VLANID,
	SXE_LINKS,
	SXE_HLREG0,
	SXE_MFLCN,
	SXE_MACC,
};

u16 sxe_mac_reg_num_get(void)
{
	return ARRAY_SIZE(mac_regs);
}


#ifndef SXE_DPDK 

void sxe_hw_fault_handle(struct sxe_hw *hw)
{
	struct sxe_adapter *adapter = hw->adapter;

	if (test_bit(SXE_HW_FAULT, &hw->state)) {
		goto l_ret;
	}

	set_bit(SXE_HW_FAULT, &hw->state);

	LOG_DEV_ERR("sxe nic hw fault\n");

	if ((hw->fault_handle != NULL) && (hw->priv != NULL) ) {
		hw->fault_handle(hw->priv);
	}

l_ret:
	return;
}

static u32 sxe_hw_fault_check(struct sxe_hw *hw, u32 reg)
{
	u32 i, value;
	u8  __iomem *base_addr = hw->reg_base_addr;
	struct sxe_adapter *adapter = hw->adapter;

	if (sxe_is_hw_fault(hw)) {
		goto l_out;
	}

	for (i = 0; i < SXE_REG_READ_RETRY; i++) {
		value = hw->reg_read(base_addr + SXE_STATUS);
		if (value != SXE_REG_READ_FAIL) {
			break;
		}

		mdelay(3);
	}

	if (SXE_REG_READ_FAIL == value) {
		LOG_ERROR_BDF("read registers multiple times failed, ret=%#x\n", value);
		sxe_hw_fault_handle(hw);
	} else {
		value = hw->reg_read(base_addr + reg);
	}

	return value;
l_out:
	return SXE_REG_READ_FAIL;
}

STATIC u32 sxe_read_reg(struct sxe_hw *hw, u32 reg)
{
	u32 value;
	u8  __iomem *base_addr = hw->reg_base_addr;
	struct sxe_adapter *adapter = hw->adapter;

	if (sxe_is_hw_fault(hw)) {
		value = SXE_REG_READ_FAIL;
		goto l_ret;
	}

	value = hw->reg_read(base_addr + reg);
	if (unlikely(SXE_REG_READ_FAIL == value)) {
		LOG_ERROR_BDF("reg[0x%x] read failed, ret=%#x\n", reg, value);
		value = sxe_hw_fault_check(hw, reg);
	}

l_ret:
	return value;
}

STATIC void sxe_write_reg(struct sxe_hw *hw, u32 reg, u32 value)
{
	u8 __iomem *base_addr = hw->reg_base_addr;

	if (sxe_is_hw_fault(hw)) {
		goto l_ret;
	}

	hw->reg_write(value, base_addr + reg);

l_ret:
	return;
}

#else 

STATIC u32 sxe_read_reg(struct sxe_hw *hw, u32 reg)
{
	u32 i, value;
	u8  __iomem *base_addr = hw->reg_base_addr;

	value = rte_le_to_cpu_32(rte_read32(base_addr + reg));
	if (unlikely(SXE_REG_READ_FAIL == value)) {

		value = rte_le_to_cpu_32(rte_read32(base_addr + SXE_STATUS));
		if (unlikely(SXE_REG_READ_FAIL != value)) {

			value = rte_le_to_cpu_32(rte_read32(base_addr + reg));
		} else {
			LOG_ERROR("reg[0x%x] and reg[0x%x] read failed, ret=%#x\n",
							reg, SXE_STATUS, value);
			for (i = 0; i < SXE_REG_READ_RETRY; i++) {

				value = rte_le_to_cpu_32(rte_read32(base_addr + SXE_STATUS));
				if (unlikely(SXE_REG_READ_FAIL != value)) {

					value = rte_le_to_cpu_32(rte_read32(base_addr + reg));
					LOG_INFO("reg[0x%x] read ok, value=%#x\n",
									reg, value);
					break;
				} else {
					LOG_ERROR("reg[0x%x] and reg[0x%x] read failed, ret=%#x\n",
							reg, SXE_STATUS, value);
				}

				mdelay(3);
			}
		}
	}

	return value;
}

STATIC void sxe_write_reg(struct sxe_hw *hw, u32 reg, u32 value)
{
	u8 __iomem *base_addr = hw->reg_base_addr;

	rte_write32((rte_cpu_to_le_32(value)), (base_addr + reg));

	return;
}
#endif

static void sxe_write_reg64(struct sxe_hw *hw, u32 reg, u64 value)
{
	u8 __iomem *reg_addr = hw->reg_base_addr;

	if (sxe_is_hw_fault(hw)) {
		goto l_ret;
	}

	writeq(value, reg_addr + reg);

l_ret:
	return;
}


void sxe_hw_no_snoop_disable(struct sxe_hw *hw)
{
	u32 ctrl_ext;

	ctrl_ext = SXE_REG_READ(hw, SXE_CTRL_EXT);
	ctrl_ext |= SXE_CTRL_EXT_NS_DIS;
	SXE_REG_WRITE(hw, SXE_CTRL_EXT, ctrl_ext);
	SXE_WRITE_FLUSH(hw);

	return;
}

s32 sxe_hw_uc_addr_pool_enable(struct sxe_hw *hw,
						u8 rar_idx, u8 pool_idx)
{
	s32 ret = 0;
	u32 value;
	struct sxe_adapter *adapter = hw->adapter;

	if (rar_idx > SXE_UC_ENTRY_NUM_MAX) {
		ret = -SXE_ERR_PARAM;
		LOG_DEV_ERR("pool_idx:%d rar_idx:%d invalid.\n",
			  pool_idx, rar_idx);
		goto l_end;
	}

	if (pool_idx < 32) {
		value = SXE_REG_READ(hw, SXE_MPSAR_LOW(rar_idx));
		value |= BIT(pool_idx);
		SXE_REG_WRITE(hw, SXE_MPSAR_LOW(rar_idx), value);
	} else {
		value = SXE_REG_READ(hw, SXE_MPSAR_HIGH(rar_idx));
		value |= BIT(pool_idx - 32);
		SXE_REG_WRITE(hw, SXE_MPSAR_HIGH(rar_idx), value);
	}

l_end:
	return ret;
}

static s32 sxe_hw_uc_addr_pool_disable(struct sxe_hw *hw, u8 rar_idx)
{
	u32 hi;
	u32 low;
	struct sxe_adapter *adapter = hw->adapter;

	hi = SXE_REG_READ(hw, SXE_MPSAR_HIGH(rar_idx));
	low = SXE_REG_READ(hw, SXE_MPSAR_LOW(rar_idx));

	if (sxe_is_hw_fault(hw)) {
		goto l_end;
	}

	if (!hi & !low) {
		LOG_DEBUG_BDF("no need clear rar-pool relation register.\n");
		goto l_end;
	}

	if (low) {
		SXE_REG_WRITE(hw, SXE_MPSAR_LOW(rar_idx), 0);
	}
	if (hi) {
		SXE_REG_WRITE(hw, SXE_MPSAR_HIGH(rar_idx), 0);
	}


l_end:
	return 0;
}

s32 sxe_hw_nic_reset(struct sxe_hw *hw)
{
	s32 ret = 0;
	u32 ctrl, i;
	struct sxe_adapter *adapter = hw->adapter;

	ctrl = SXE_CTRL_RST;
	ctrl |= SXE_REG_READ(hw, SXE_CTRL);
	ctrl &= ~SXE_CTRL_GIO_DIS;
	SXE_REG_WRITE(hw, SXE_CTRL, ctrl);

	SXE_WRITE_FLUSH(hw);
	usleep_range(1000, 1200);

	for (i = 0; i < 10; i++) {
		ctrl = SXE_REG_READ(hw, SXE_CTRL);
		if (!(ctrl & SXE_CTRL_RST_MASK)) {
			break;
		}
		udelay(1);
	}

	if (ctrl & SXE_CTRL_RST_MASK) {
		ret = -SXE_ERR_RESET_FAILED;
		LOG_DEV_ERR("reset polling failed to complete\n");
	}

	return ret;
}

void sxe_hw_pf_rst_done_set(struct sxe_hw *hw)
{
	u32 value;

	value = SXE_REG_READ(hw, SXE_CTRL_EXT);
	value |= SXE_CTRL_EXT_PFRSTD;
	SXE_REG_WRITE(hw, SXE_CTRL_EXT, value);

	return;
}

static void sxe_hw_regs_flush(struct sxe_hw *hw)
{
	SXE_WRITE_FLUSH(hw);
	return;
}

static const struct sxe_reg_info sxe_reg_info_tbl[] = {

	{SXE_CTRL, 1, 1, "CTRL"},
	{SXE_STATUS, 1, 1, "STATUS"},
	{SXE_CTRL_EXT, 1, 1, "CTRL_EXT"},

	{SXE_EICR, 1, 1, "EICR"},

	{SXE_SRRCTL(0), 16, 0x4, "SRRCTL"},
	{SXE_RDH(0), 64, 0x40, "RDH"},
	{SXE_RDT(0), 64, 0x40, "RDT"},
	{SXE_RXDCTL(0), 64, 0x40, "RXDCTL"},
	{SXE_RDBAL(0), 64, 0x40, "RDBAL"},
	{SXE_RDBAH(0), 64, 0x40, "RDBAH"},

	{SXE_TDBAL(0), 32, 0x40, "TDBAL"},
	{SXE_TDBAH(0), 32, 0x40, "TDBAH"},
	{SXE_TDLEN(0), 32, 0x40, "TDLEN"},
	{SXE_TDH(0), 32, 0x40, "TDH"},
	{SXE_TDT(0), 32, 0x40, "TDT"},
	{SXE_TXDCTL(0), 32, 0x40, "TXDCTL"},

	{ .name = NULL }
};

static void sxe_hw_reg_print(struct sxe_hw *hw,
				const struct sxe_reg_info *reginfo)
{
	u32 i, j;
	s8 *value;
	u32 first_reg_idx = 0;
	u32 regs[SXE_DUMP_REGS_NUM];
	s8 reg_name[SXE_REG_NAME_LEN];
	s8 buf[SXE_DUMP_REG_STRING_LEN];
	struct sxe_adapter *adapter = hw->adapter;

	switch (reginfo->addr) {
	case SXE_SRRCTL(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_SRRCTL(i));
		}
		break;
	case SXE_RDLEN(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_RDLEN(i));
		}
		break;
	case SXE_RDH(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_RDH(i));
		}
		break;
	case SXE_RDT(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_RDT(i));
		}
		break;
	case SXE_RXDCTL(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_RXDCTL(i));
		}
		break;
	case SXE_RDBAL(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_RDBAL(i));
		}
		break;
	case SXE_RDBAH(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_RDBAH(i));
		}
		break;
	case SXE_TDBAL(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_TDBAL(i));
		}
		break;
	case SXE_TDBAH(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_TDBAH(i));
		}
		break;
	case SXE_TDLEN(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_TDLEN(i));
		}
		break;
	case SXE_TDH(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_TDH(i));
		}
		break;
	case SXE_TDT(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_TDT(i));
		}
		break;
	case SXE_TXDCTL(0):
		for (i = 0; i < SXE_DUMP_REGS_NUM; i++) {
			regs[i] = SXE_REG_READ(hw, SXE_TXDCTL(i));
		}
		break;
	default:
		LOG_DEV_INFO("%-15s %08x\n",
			reginfo->name, SXE_REG_READ(hw, reginfo->addr));
		goto l_end;
	}

	while (first_reg_idx < SXE_DUMP_REGS_NUM) {
		value = buf;
		snprintf(reg_name, SXE_REG_NAME_LEN,
			"%s[%d-%d]", reginfo->name,
			first_reg_idx, (first_reg_idx + 7));

		for (j = 0; j < 8; j++) {
			value += sprintf(value, " %08x", regs[first_reg_idx++]);
		}

		LOG_DEV_ERR("%-15s%s\n", reg_name, buf);
	}

l_end:
	return;
}

static void sxe_hw_reg_dump(struct sxe_hw *hw)
{
	const struct sxe_reg_info *reginfo;

	for (reginfo = (const struct sxe_reg_info *)sxe_reg_info_tbl;
	     reginfo->name; reginfo++) {
		sxe_hw_reg_print(hw, reginfo);
	}

	return;
}

static s32 sxe_hw_status_reg_test(struct sxe_hw *hw)
{
	s32 ret = 0;
	u32 value, before, after;
	u32 toggle = 0x7FFFF30F;
	struct sxe_adapter *adapter = hw->adapter;

	before = SXE_REG_READ(hw, SXE_STATUS);
	value = (SXE_REG_READ(hw, SXE_STATUS) & toggle);
	SXE_REG_WRITE(hw, SXE_STATUS, toggle);
	after = SXE_REG_READ(hw, SXE_STATUS) & toggle;
	if (value != after) {
		LOG_MSG_ERR(drv, "failed status register test got: "
				"0x%08X expected: 0x%08X\n",
					after, value);
		ret = -SXE_DIAG_TEST_BLOCKED;
		goto l_end;
	}

	SXE_REG_WRITE(hw, SXE_STATUS, before);

l_end:
	return ret;
}

#define PATTERN_TEST	1
#define SET_READ_TEST	2
#define WRITE_NO_TEST	3
#define TABLE32_TEST	4
#define TABLE64_TEST_LO	5
#define TABLE64_TEST_HI	6

struct sxe_self_test_reg {
	u32 reg;
	u8  array_len;
	u8  test_type;
	u32 mask;
	u32 write;
};

static const struct sxe_self_test_reg self_test_reg[] = {
	{ SXE_FCRTL(0),  1,   PATTERN_TEST, 0x8007FFE0, 0x8007FFF0 },
	{ SXE_FCRTH(0),  1,   PATTERN_TEST, 0x8007FFE0, 0x8007FFF0 },
	{ SXE_PFCTOP,    1,   PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ SXE_FCTTV(0),  1,   PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ SXE_VLNCTRL,   1,   PATTERN_TEST, 0x00000000, 0x00000000 },
	{ SXE_RDBAL(0),  4,   PATTERN_TEST, 0xFFFFFF80, 0xFFFFFF80 },
	{ SXE_RDBAH(0),  4,   PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ SXE_RDLEN(0),  4,   PATTERN_TEST, 0x000FFFFF, 0x000FFFFF },
	{ SXE_RXDCTL(0), 4,   WRITE_NO_TEST, 0, SXE_RXDCTL_ENABLE },
	{ SXE_RDT(0),    4,   PATTERN_TEST, 0x0000FFFF, 0x0000FFFF },
	{ SXE_RXDCTL(0), 4,   WRITE_NO_TEST, 0, 0 },
	{ SXE_TDBAL(0),  4,   PATTERN_TEST, 0xFFFFFF80, 0xFFFFFFFF },
	{ SXE_TDBAH(0),  4,   PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ SXE_TDLEN(0),  4,   PATTERN_TEST, 0x000FFF80, 0x000FFF80 },
	{ SXE_RXCTRL,    1,   SET_READ_TEST, 0x00000001, 0x00000001 },
	{ SXE_RAL(0),    16,  TABLE64_TEST_LO, 0xFFFFFFFF, 0xFFFFFFFF },
	{ SXE_RAL(0),    16,  TABLE64_TEST_HI, 0x8001FFFF, 0x800CFFFF },
	{ SXE_MTA(0),    128, TABLE32_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ .reg = 0 }
};

static s32 sxe_hw_reg_pattern_test(struct sxe_hw *hw, u32 reg,
				u32 mask, u32 write)
{
	s32 ret = 0;
	u32 pat, val, before;
	struct sxe_adapter *adapter = hw->adapter;
	static const u32 test_pattern[] = {
		0x5A5A5A5A, 0xA5A5A5A5, 0x00000000, 0xFFFFFFFE};

	if (sxe_is_hw_fault(hw)) {
		LOG_ERROR_BDF("hw fault\n");
		ret = -SXE_DIAG_TEST_BLOCKED;
		goto l_end;
	}

	for (pat = 0; pat < ARRAY_SIZE(test_pattern); pat++) {
		before = SXE_REG_READ(hw, reg);

		SXE_REG_WRITE(hw, reg, test_pattern[pat] & write);
		val = SXE_REG_READ(hw, reg);
		if (val != (test_pattern[pat] & write & mask)) {
			LOG_MSG_ERR(drv, "pattern test reg %04X failed: "
					"got 0x%08X expected 0x%08X\n",
				reg, val, (test_pattern[pat] & write & mask));
			SXE_REG_WRITE(hw, reg, before);
			ret = -SXE_DIAG_REG_PATTERN_TEST_ERR;
			goto l_end;
		}

		SXE_REG_WRITE(hw, reg, before);
	}

l_end:
	return ret;
}

static s32 sxe_hw_reg_set_and_check(struct sxe_hw *hw, int reg,
				u32 mask, u32 write)
{
	s32 ret = 0;
	u32 val, before;
	struct sxe_adapter *adapter = hw->adapter;

	if (sxe_is_hw_fault(hw)) {
		LOG_ERROR_BDF("hw fault\n");
		ret = -SXE_DIAG_TEST_BLOCKED;
		goto l_end;
	}

	before = SXE_REG_READ(hw, reg);
	SXE_REG_WRITE(hw, reg, write & mask);
	val = SXE_REG_READ(hw, reg);
	if ((write & mask) != (val & mask)) {
		LOG_MSG_ERR(drv, "set/check reg %04X test failed: "
				"got 0x%08X expected 0x%08X\n",
			reg, (val & mask), (write & mask));
		SXE_REG_WRITE(hw, reg, before);
		ret = -SXE_DIAG_CHECK_REG_TEST_ERR;
		goto l_end;
	}

	SXE_REG_WRITE(hw, reg, before);

l_end:
	return ret;
}

STATIC s32 sxe_hw_regs_test(struct sxe_hw *hw)
{
	u32 i;
	s32 ret = 0;
	const struct sxe_self_test_reg *test = self_test_reg;
	struct sxe_adapter *adapter = hw->adapter;

	ret = sxe_hw_status_reg_test(hw);
	if (ret) {
		LOG_MSG_ERR(drv, "status register test failed\n");
		goto l_end;
	}

	while (test->reg) {
		for (i = 0; i < test->array_len; i++) {
			switch (test->test_type) {
			case PATTERN_TEST:
				ret = sxe_hw_reg_pattern_test(hw,
					test->reg + (i * 0x40),
					test->mask, test->write);
				break;
			case TABLE32_TEST:
				ret = sxe_hw_reg_pattern_test(hw,
					test->reg + (i * 4),
					test->mask, test->write);
				break;
			case TABLE64_TEST_LO:
				ret = sxe_hw_reg_pattern_test(hw,
					test->reg + (i * 8),
					test->mask, test->write);
				break;
			case TABLE64_TEST_HI:
				ret = sxe_hw_reg_pattern_test(hw,
					(test->reg + 4) + (i * 8),
					test->mask, test->write);
				break;
			case SET_READ_TEST:
				ret = sxe_hw_reg_set_and_check(hw,
					test->reg + (i * 0x40),
					test->mask, test->write);
				break;
			case WRITE_NO_TEST:
				SXE_REG_WRITE(hw, test->reg + (i * 0x40),
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

static const struct sxe_setup_operations sxe_setup_ops = {
	.regs_dump		= sxe_hw_reg_dump,
	.reg_read		= sxe_read_reg,
	.reg_write		= sxe_write_reg,
	.regs_test		= sxe_hw_regs_test,
	.reset			= sxe_hw_nic_reset,
	.regs_flush		= sxe_hw_regs_flush,
	.pf_rst_done_set	= sxe_hw_pf_rst_done_set,
	.no_snoop_disable	= sxe_hw_no_snoop_disable,
};


static void sxe_hw_ring_irq_enable(struct sxe_hw *hw, u64 qmask)
{
	u32 mask0, mask1;

	mask0 = qmask & 0xFFFFFFFF;
	mask1 = qmask >> 32;

	if (mask0 && mask1) {
		SXE_REG_WRITE(hw, SXE_EIMS_EX(0), mask0);
		SXE_REG_WRITE(hw, SXE_EIMS_EX(1), mask1);
	} else if (mask0) {
		SXE_REG_WRITE(hw, SXE_EIMS_EX(0), mask0);
	} else if (mask1) {
		SXE_REG_WRITE(hw, SXE_EIMS_EX(1), mask1);
	}

	return;
}

u32 sxe_hw_pending_irq_read_clear(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_EICR);
}

void sxe_hw_pending_irq_write_clear(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_EICR, value);
	return;
}

u32 sxe_hw_irq_cause_get(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_EICS);
}

static void sxe_hw_event_irq_trigger(struct sxe_hw *hw)
{
	SXE_REG_WRITE(hw, SXE_EICS, (SXE_EICS_TCP_TIMER | SXE_EICS_OTHER));

	return;
}

static void sxe_hw_ring_irq_trigger(struct sxe_hw *hw, u64 eics)
{
	u32 mask;

	mask = (eics & 0xFFFFFFFF);
	SXE_REG_WRITE(hw, SXE_EICS_EX(0), mask);
	mask = (eics >> 32);
	SXE_REG_WRITE(hw, SXE_EICS_EX(1), mask);
	return;
}

void sxe_hw_ring_irq_auto_disable(struct sxe_hw *hw,
					bool is_msix)
{
	if (true == is_msix) {
		SXE_REG_WRITE(hw, SXE_EIAM_EX(0), 0xFFFFFFFF);
		SXE_REG_WRITE(hw, SXE_EIAM_EX(1), 0xFFFFFFFF);
	} else {
		SXE_REG_WRITE(hw, SXE_EIAM, SXE_EICS_RTX_QUEUE);
	}

	return;
}

void sxe_hw_irq_general_reg_set(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_GPIE, value);

	return;
}

u32 sxe_hw_irq_general_reg_get(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_GPIE);
}

static void sxe_hw_set_eitrsel(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_EITRSEL, value);

	return;
}

void sxe_hw_event_irq_map(struct sxe_hw *hw, u8 offset, u16 irq_idx)
{
	u8  allocation;
	u32 ivar, position;

	allocation = irq_idx | SXE_IVAR_ALLOC_VALID;

	position = (offset & 1) * 8;

	ivar = SXE_REG_READ(hw, SXE_IVAR_MISC);
	ivar &= ~(0xFF << position);
	ivar |= (allocation << position);

	SXE_REG_WRITE(hw, SXE_IVAR_MISC, ivar);

	return;
}

void sxe_hw_ring_irq_map(struct sxe_hw *hw, bool is_tx,
						u16 reg_idx, u16 irq_idx)
{
	u8  allocation;
	u32 ivar, position;

	allocation = irq_idx | SXE_IVAR_ALLOC_VALID;

	position = ((reg_idx & 1) * 16) + (8 * is_tx);

	ivar = SXE_REG_READ(hw, SXE_IVAR(reg_idx >> 1));
	ivar &= ~(0xFF << position);
	ivar |= (allocation << position);

	SXE_REG_WRITE(hw, SXE_IVAR(reg_idx >> 1), ivar);

	return;
}

void sxe_hw_ring_irq_interval_set(struct sxe_hw *hw,
						u16 irq_idx, u32 interval)
{
	u32 eitr = interval & SXE_EITR_ITR_MASK;

	eitr |= SXE_EITR_CNT_WDIS;

	SXE_REG_WRITE(hw, SXE_EITR(irq_idx), eitr);

	return;
}

static void sxe_hw_event_irq_interval_set(struct sxe_hw *hw,
						u16 irq_idx, u32 value)
{
	SXE_REG_WRITE(hw, SXE_EITR(irq_idx), value);

	return;
}

void sxe_hw_event_irq_auto_clear_set(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_EIAC, value);

	return;
}

void sxe_hw_specific_irq_disable(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_EIMC, value);

	return;
}

void sxe_hw_specific_irq_enable(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_EIMS, value);

	return;
}

void sxe_hw_all_irq_disable(struct sxe_hw *hw)
{
	SXE_REG_WRITE(hw, SXE_EIMC, 0xFFFF0000);

	SXE_REG_WRITE(hw, SXE_EIMC_EX(0), ~0);
	SXE_REG_WRITE(hw, SXE_EIMC_EX(1), ~0);

	SXE_WRITE_FLUSH(hw);

	return;
}

static void sxe_hw_spp_configure(struct sxe_hw *hw, u32 hw_spp_proc_delay_us)
{
	SXE_REG_WRITE(hw, SXE_SPP_PROC,
			(SXE_REG_READ(hw, SXE_SPP_PROC) &
			~SXE_SPP_PROC_DELAY_US_MASK) |
			hw_spp_proc_delay_us);

	return;
}

static s32 sxe_hw_irq_test(struct sxe_hw *hw, u32 *icr, bool shared)
{
	s32 ret = 0;
	u32 i, mask;
	struct sxe_adapter *adapter = hw->adapter;

	sxe_hw_specific_irq_disable(hw, 0xFFFFFFFF);
	sxe_hw_regs_flush(hw);
	usleep_range(10000, 20000);

	for (i = 0; i < 10; i++) {
		mask = BIT(i);
		if (!shared) {
			LOG_INFO_BDF("test irq: irq test start\n");
			*icr = 0;
			SXE_REG_WRITE(hw, SXE_EIMC, ~mask & 0x00007FFF);
			SXE_REG_WRITE(hw, SXE_EICS, ~mask & 0x00007FFF);
			sxe_hw_regs_flush(hw);
			usleep_range(10000, 20000);

			if (*icr & mask) {
				LOG_ERROR_BDF("test irq: failed, eicr = %x\n", *icr);
				ret = -SXE_DIAG_DISABLE_IRQ_TEST_ERR;
				break;
			}
			LOG_INFO_BDF("test irq: irq test end\n");
		}

		LOG_INFO_BDF("test irq: mask irq test start\n");
		*icr = 0;
		SXE_REG_WRITE(hw, SXE_EIMS, mask);
		SXE_REG_WRITE(hw, SXE_EICS, mask);
		sxe_hw_regs_flush(hw);
		usleep_range(10000, 20000);

		if (!(*icr & mask)) {
			LOG_ERROR_BDF("test irq: mask failed, eicr = %x\n", *icr);
			ret = -SXE_DIAG_ENABLE_IRQ_TEST_ERR;
			break;
		}
		LOG_INFO_BDF("test irq: mask irq test end\n");

		sxe_hw_specific_irq_disable(hw, mask);
		sxe_hw_regs_flush(hw);
		usleep_range(10000, 20000);

		if (!shared) {
			LOG_INFO_BDF("test irq: other irq test start\n");
			*icr = 0;
			SXE_REG_WRITE(hw, SXE_EIMC, ~mask & 0x00007FFF);
			SXE_REG_WRITE(hw, SXE_EICS, ~mask & 0x00007FFF);
			sxe_hw_regs_flush(hw);
			usleep_range(10000, 20000);

			if (*icr) {
				LOG_ERROR_BDF("test irq: other irq failed, eicr = %x\n", *icr);
				ret = -SXE_DIAG_DISABLE_OTHER_IRQ_TEST_ERR;
				break;
			}
			LOG_INFO_BDF("test irq: other irq test end\n");
		}
	}

	sxe_hw_specific_irq_disable(hw, 0xFFFFFFFF);
	sxe_hw_regs_flush(hw);
	usleep_range(10000, 20000);

	return ret;
}

static const struct sxe_irq_operations sxe_irq_ops = {
	.event_irq_auto_clear_set	= sxe_hw_event_irq_auto_clear_set,
	.ring_irq_interval_set		= sxe_hw_ring_irq_interval_set,
	.event_irq_interval_set		= sxe_hw_event_irq_interval_set,
	.set_eitrsel			= sxe_hw_set_eitrsel,
	.ring_irq_map			= sxe_hw_ring_irq_map,
	.event_irq_map			= sxe_hw_event_irq_map,
	.irq_general_reg_set		= sxe_hw_irq_general_reg_set,
	.irq_general_reg_get		= sxe_hw_irq_general_reg_get,
	.ring_irq_auto_disable		= sxe_hw_ring_irq_auto_disable,
	.pending_irq_read_clear		= sxe_hw_pending_irq_read_clear,
	.pending_irq_write_clear	= sxe_hw_pending_irq_write_clear,
	.ring_irq_enable		= sxe_hw_ring_irq_enable,
	.irq_cause_get			= sxe_hw_irq_cause_get,
	.event_irq_trigger		= sxe_hw_event_irq_trigger,
	.ring_irq_trigger		= sxe_hw_ring_irq_trigger,
	.specific_irq_disable		= sxe_hw_specific_irq_disable,
	.specific_irq_enable		= sxe_hw_specific_irq_enable,
	.all_irq_disable		= sxe_hw_all_irq_disable,
	.spp_configure			= sxe_hw_spp_configure,
	.irq_test			= sxe_hw_irq_test,
};


u32 sxe_hw_link_speed_get(struct sxe_hw *hw)
{
	u32 speed, value;
	struct sxe_adapter *adapter = hw->adapter;
	value = SXE_REG_READ(hw, SXE_COMCTRL);

	if ((value & SXE_COMCTRL_SPEED_10G) == SXE_COMCTRL_SPEED_10G) {
		speed = SXE_LINK_SPEED_10GB_FULL;
	} else if ((value & SXE_COMCTRL_SPEED_1G) == SXE_COMCTRL_SPEED_1G) {
		speed = SXE_LINK_SPEED_1GB_FULL;
	} else {
		speed = SXE_LINK_SPEED_UNKNOWN;
	}

	LOG_DEBUG_BDF("hw link speed=%x, (0x80=10G, 0x20=1G)\n, reg=%x",
			speed, value);

	return speed;
}

void sxe_hw_link_speed_set(struct sxe_hw *hw, u32 speed)
{
	u32 ctrl;

	ctrl = SXE_REG_READ(hw, SXE_COMCTRL);

	if (SXE_LINK_SPEED_1GB_FULL == speed) {
		ctrl |= SXE_COMCTRL_SPEED_1G;
	} else if (SXE_LINK_SPEED_10GB_FULL == speed) {
		ctrl |= SXE_COMCTRL_SPEED_10G;
	}

	SXE_REG_WRITE(hw, SXE_COMCTRL, ctrl);

	return;
}

STATIC bool sxe_hw_1g_link_up_check(struct sxe_hw *hw)
{
	return (SXE_REG_READ(hw, SXE_LINKS) & SXE_LINKS_UP) ? true : false;
}

bool sxe_hw_is_link_state_up(struct sxe_hw *hw)
{
	bool ret = false;
	u32 links_reg, link_speed;
	struct sxe_adapter *adapter = hw->adapter;

	links_reg  = SXE_REG_READ(hw, SXE_LINKS);

	LOG_DEBUG_BDF("nic link reg: 0x%x\n", links_reg);

	if (links_reg & SXE_LINKS_UP) {
		ret = true;

		link_speed = sxe_hw_link_speed_get(hw);
		if ((link_speed == SXE_LINK_SPEED_10GB_FULL) &&
		    (links_reg & SXE_10G_LINKS_DOWN)) {
			ret = false;
		}
	}

	return ret;
}

void sxe_hw_mac_pad_enable(struct sxe_hw *hw)
{
	u32 ctl;

	ctl = SXE_REG_READ(hw, SXE_MACCFG);
	ctl |= SXE_MACCFG_PAD_EN;
	SXE_REG_WRITE(hw, SXE_MACCFG, ctl);

	return;
}

s32 sxe_hw_fc_enable(struct sxe_hw *hw)
{
	s32 ret = 0;
	u8  i;
	u32 reg;
	u32 flctrl_val;
	u32 fcrtl, fcrth;
	struct sxe_adapter *adapter = hw->adapter;

	flctrl_val = SXE_REG_READ(hw, SXE_FLCTRL);
	flctrl_val &= ~(SXE_FCTRL_TFCE_MASK | SXE_FCTRL_RFCE_MASK |
		       SXE_FCTRL_TFCE_FCEN_MASK | SXE_FCTRL_TFCE_XONE_MASK);

	switch (hw->fc.current_mode) {
	case SXE_FC_NONE:
		break;
	case SXE_FC_RX_PAUSE:
		flctrl_val |= SXE_FCTRL_RFCE_LFC_EN;
		break;
	case SXE_FC_TX_PAUSE:
		flctrl_val |= SXE_FCTRL_TFCE_LFC_EN;
		break;
	case SXE_FC_FULL:
		flctrl_val |= SXE_FCTRL_RFCE_LFC_EN;
		flctrl_val |= SXE_FCTRL_TFCE_LFC_EN;
		break;
	default:
		LOG_DEV_DEBUG("flow control param set incorrectly\n");
		ret = -SXE_ERR_CONFIG;
		goto l_ret;
	}

	for (i = 0; i < MAX_TRAFFIC_CLASS; i++) {
		if ((hw->fc.current_mode & SXE_FC_TX_PAUSE) &&
		    hw->fc.high_water[i]) {
			fcrtl = (hw->fc.low_water[i] << 9) | SXE_FCRTL_XONE;
			SXE_REG_WRITE(hw, SXE_FCRTL(i), fcrtl);
			fcrth = (hw->fc.high_water[i] << 9) | SXE_FCRTH_FCEN;
		} else {
			SXE_REG_WRITE(hw, SXE_FCRTL(i), 0);
			fcrth = (SXE_REG_READ(hw, SXE_RXPBSIZE(i)) - 24576) >> 1;
		}

		SXE_REG_WRITE(hw, SXE_FCRTH(i), fcrth);
	}

	flctrl_val |= SXE_FCTRL_TFCE_DPF_EN;

	if ((hw->fc.current_mode & SXE_FC_TX_PAUSE)) {
		flctrl_val |= (SXE_FCTRL_TFCE_FCEN_MASK | SXE_FCTRL_TFCE_XONE_MASK);
	}

	SXE_REG_WRITE(hw, SXE_FLCTRL, flctrl_val);

	reg = SXE_REG_READ(hw, SXE_PFCTOP);
	reg &= ~SXE_PFCTOP_FCOP_MASK;
	reg |= SXE_PFCTOP_FCT;
	reg |= SXE_PFCTOP_FCOP_LFC;
	SXE_REG_WRITE(hw, SXE_PFCTOP, reg);

	reg = hw->fc.pause_time * 0x00010001U;
	for (i = 0; i < (MAX_TRAFFIC_CLASS / 2); i++) {
		SXE_REG_WRITE(hw, SXE_FCTTV(i), reg);
	}

	SXE_REG_WRITE(hw, SXE_FCRTV, hw->fc.pause_time / 2);

l_ret:
	return ret;
}

void sxe_fc_autoneg_localcap_set(struct sxe_hw *hw)
{
	u32 reg = 0;

	if (hw->fc.requested_mode == SXE_FC_DEFAULT) {
		hw->fc.requested_mode = SXE_FC_FULL;
	}

	reg = SXE_REG_READ(hw, SXE_PCS1GANA);

	switch (hw->fc.requested_mode) {
	case SXE_FC_NONE:
		reg &= ~(SXE_PCS1GANA_SYM_PAUSE | SXE_PCS1GANA_ASM_PAUSE);
		break;
	case SXE_FC_TX_PAUSE:
		reg |= SXE_PCS1GANA_ASM_PAUSE;
		reg &= ~SXE_PCS1GANA_SYM_PAUSE;
		break;
	case SXE_FC_RX_PAUSE:
	case SXE_FC_FULL:
		reg |= SXE_PCS1GANA_SYM_PAUSE | SXE_PCS1GANA_ASM_PAUSE;
		break;
	default:
		LOG_ERROR("Flow control param set incorrectly.");
		break;
	}

	SXE_REG_WRITE(hw, SXE_PCS1GANA, reg);
	return;
}

s32 sxe_hw_pfc_enable(struct sxe_hw *hw, u8 tc_idx)
{
	s32 ret = 0;
	u8  i;
	u32 reg;
	u32 flctrl_val;
	u32 fcrtl, fcrth;
	struct sxe_adapter *adapter = hw->adapter;
	u8 rx_en_num;

	flctrl_val = SXE_REG_READ(hw, SXE_FLCTRL);
	flctrl_val &= ~(SXE_FCTRL_TFCE_MASK | SXE_FCTRL_RFCE_MASK |
		       SXE_FCTRL_TFCE_FCEN_MASK | SXE_FCTRL_TFCE_XONE_MASK);

	switch (hw->fc.current_mode) {
	case SXE_FC_NONE:
		rx_en_num = 0;
		for (i = 0; i < MAX_TRAFFIC_CLASS; i++) {
			reg = SXE_REG_READ(hw, SXE_FCRTH(i));
			if (reg & SXE_FCRTH_FCEN) {
				rx_en_num++;
			}
		}
		if (rx_en_num > 1) {
			flctrl_val |= SXE_FCTRL_TFCE_PFC_EN;
		}

		break;

	case SXE_FC_RX_PAUSE:
		flctrl_val |= SXE_FCTRL_RFCE_PFC_EN;

		rx_en_num = 0;
		for (i = 0; i < MAX_TRAFFIC_CLASS; i++) {
			reg = SXE_REG_READ(hw, SXE_FCRTH(i));
			if (reg & SXE_FCRTH_FCEN) {
				rx_en_num++;
			}
		}

		if (rx_en_num > 1) {
			flctrl_val |= SXE_FCTRL_TFCE_PFC_EN;
		}

		break;
	case SXE_FC_TX_PAUSE:
		flctrl_val |= SXE_FCTRL_TFCE_PFC_EN;
		break;
	case SXE_FC_FULL:
		flctrl_val |= SXE_FCTRL_RFCE_PFC_EN;
		flctrl_val |= SXE_FCTRL_TFCE_PFC_EN;
		break;
	default:
		LOG_DEV_DEBUG("flow control param set incorrectly\n");
		ret = -SXE_ERR_CONFIG;
		goto l_ret;
	}

	if ((hw->fc.current_mode & SXE_FC_TX_PAUSE) &&
	    hw->fc.high_water[tc_idx]) {
		fcrtl = (hw->fc.low_water[tc_idx] << 9) | SXE_FCRTL_XONE;
		SXE_REG_WRITE(hw, SXE_FCRTL(tc_idx), fcrtl);
		fcrth = (hw->fc.high_water[tc_idx] << 9) | SXE_FCRTH_FCEN;
	} else {
		SXE_REG_WRITE(hw, SXE_FCRTL(tc_idx), 0);
		fcrth = (SXE_REG_READ(hw, SXE_RXPBSIZE(tc_idx)) - 24576) >> 1;
	}

	SXE_REG_WRITE(hw, SXE_FCRTH(tc_idx), fcrth);

	flctrl_val |= SXE_FCTRL_TFCE_DPF_EN;

	if ((hw->fc.current_mode & SXE_FC_TX_PAUSE)) {
		flctrl_val |= (BIT(tc_idx) << 16) & SXE_FCTRL_TFCE_FCEN_MASK;
		flctrl_val |= (BIT(tc_idx) << 24) & SXE_FCTRL_TFCE_XONE_MASK;
	}

	SXE_REG_WRITE(hw, SXE_FLCTRL, flctrl_val);

	reg = SXE_REG_READ(hw, SXE_PFCTOP);
	reg &= ~SXE_PFCTOP_FCOP_MASK;
	reg |= SXE_PFCTOP_FCT;
	reg |= SXE_PFCTOP_FCOP_PFC;
	SXE_REG_WRITE(hw, SXE_PFCTOP, reg);

	reg = hw->fc.pause_time * 0x00010001U;
	for (i = 0; i < (MAX_TRAFFIC_CLASS / 2); i++) {
		SXE_REG_WRITE(hw, SXE_FCTTV(i), reg);
	}

	SXE_REG_WRITE(hw, SXE_FCRTV, hw->fc.pause_time / 2);

l_ret:
	return ret;
}

void sxe_hw_crc_configure(struct sxe_hw *hw)
{
	u32 ctrl = SXE_REG_READ(hw, SXE_PCCTRL);

	ctrl |=  SXE_PCCTRL_TXCE | SXE_PCCTRL_RXCE | SXE_PCCTRL_PCSC_ALL;
	SXE_REG_WRITE(hw, SXE_PCCTRL, ctrl);

	return;
}

void sxe_hw_loopback_switch(struct sxe_hw *hw, bool is_enable)
{
	u32 value;

	value = (true == is_enable) ? SXE_LPBK_EN : 0;

	SXE_REG_WRITE(hw, SXE_LPBKCTRL, value);

	return;
}

void sxe_hw_mac_txrx_enable(struct sxe_hw *hw)
{
	u32 ctl;

	ctl = SXE_REG_READ(hw, SXE_COMCTRL);
	ctl |= SXE_COMCTRL_TXEN | SXE_COMCTRL_RXEN | SXE_COMCTRL_EDSEL;
	SXE_REG_WRITE(hw, SXE_COMCTRL, ctl);

	return;
}

void sxe_hw_mac_max_frame_set(struct sxe_hw *hw, u32 max_frame)
{
	u32 maxfs = SXE_REG_READ(hw, SXE_MAXFS);

	if (max_frame != (maxfs >> SXE_MAXFS_MFS_SHIFT)) {
		maxfs &= ~SXE_MAXFS_MFS_MASK;
		maxfs |= max_frame << SXE_MAXFS_MFS_SHIFT;
	}

	maxfs |=  SXE_MAXFS_RFSEL | SXE_MAXFS_TFSEL;
	SXE_REG_WRITE(hw, SXE_MAXFS, maxfs);

	return;
}

u32 sxe_hw_mac_max_frame_get(struct sxe_hw *hw)
{
	u32 maxfs = SXE_REG_READ(hw, SXE_MAXFS);

	maxfs &= SXE_MAXFS_MFS_MASK;
	maxfs >>= SXE_MAXFS_MFS_SHIFT;

	return maxfs;
}

bool sxe_device_supports_autoneg_fc(struct sxe_hw *hw)
{
	bool supported = true;
	bool link_up = sxe_hw_is_link_state_up(hw);
	u32  link_speed = sxe_hw_link_speed_get(hw);

	if (link_up) {
		supported = (link_speed == SXE_LINK_SPEED_1GB_FULL) ?
				true : false;
	}

	return supported;
}

STATIC void sxe_hw_fc_param_init(struct sxe_hw *hw)
{
	hw->fc.requested_mode = SXE_FC_FULL;
	hw->fc.current_mode = SXE_FC_FULL;	
	hw->fc.pause_time = SXE_DEFAULT_FCPAUSE;

	hw->fc.disable_fc_autoneg = true;
	return;
}

void sxe_hw_fc_tc_high_water_mark_set(struct sxe_hw *hw,
							u8 tc_idx, u32 mark)
{
	hw->fc.high_water[tc_idx] = mark;

	return;
}

void sxe_hw_fc_tc_low_water_mark_set(struct sxe_hw *hw,
							u8 tc_idx, u32 mark)
{
	hw->fc.low_water[tc_idx] = mark;

	return;
}

bool sxe_hw_is_fc_autoneg_disabled(struct sxe_hw *hw)
{
	return hw->fc.disable_fc_autoneg;
}

void sxe_hw_fc_autoneg_disable_set(struct sxe_hw *hw,
							bool is_disabled)
{
	hw->fc.disable_fc_autoneg = is_disabled;
	return;
}

static enum sxe_fc_mode sxe_hw_fc_current_mode_get(struct sxe_hw *hw)
{
	return hw->fc.current_mode;
}

static enum sxe_fc_mode sxe_hw_fc_requested_mode_get(struct sxe_hw *hw)
{
	return hw->fc.requested_mode;
}

void sxe_hw_fc_requested_mode_set(struct sxe_hw *hw,
						enum sxe_fc_mode mode)
{
	hw->fc.requested_mode = mode;
	return;
}

static const struct sxe_mac_operations sxe_mac_ops = {
	.link_up_1g_check			= sxe_hw_1g_link_up_check,
	.link_state_is_up		= sxe_hw_is_link_state_up,
	.link_speed_get			= sxe_hw_link_speed_get,
	.link_speed_set			= sxe_hw_link_speed_set,
	.pad_enable			= sxe_hw_mac_pad_enable,
	.crc_configure			= sxe_hw_crc_configure,
	.loopback_switch		= sxe_hw_loopback_switch,
	.txrx_enable			= sxe_hw_mac_txrx_enable,
	.max_frame_set			= sxe_hw_mac_max_frame_set,
	.max_frame_get			= sxe_hw_mac_max_frame_get,
	.fc_enable			= sxe_hw_fc_enable,
	.fc_autoneg_localcap_set	= sxe_fc_autoneg_localcap_set,
	.fc_tc_high_water_mark_set	= sxe_hw_fc_tc_high_water_mark_set,
	.fc_tc_low_water_mark_set	= sxe_hw_fc_tc_low_water_mark_set,
	.fc_param_init			= sxe_hw_fc_param_init,
	.fc_current_mode_get		= sxe_hw_fc_current_mode_get,
	.fc_requested_mode_get		= sxe_hw_fc_requested_mode_get,
	.fc_requested_mode_set		= sxe_hw_fc_requested_mode_set,
	.is_fc_autoneg_disabled		= sxe_hw_is_fc_autoneg_disabled,
	.fc_autoneg_disable_set		= sxe_hw_fc_autoneg_disable_set,
};

u32 sxe_hw_rx_mode_get(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_FCTRL);
}

u32 sxe_hw_pool_rx_mode_get(struct sxe_hw *hw, u16 pool_idx)
{
	return SXE_REG_READ(hw, SXE_VMOLR(pool_idx));
}

void sxe_hw_rx_mode_set(struct sxe_hw *hw, u32 filter_ctrl)
{
	SXE_REG_WRITE(hw, SXE_FCTRL, filter_ctrl);
	return;
}

void sxe_hw_pool_rx_mode_set(struct sxe_hw *hw,
						u32 vmolr, u16 pool_idx)
{
	SXE_REG_WRITE(hw, SXE_VMOLR(pool_idx), vmolr);
	return;
}

void sxe_hw_rx_lro_enable(struct sxe_hw *hw, bool is_enable)
{
	u32 rfctl = SXE_REG_READ(hw, SXE_RFCTL);
	rfctl &= ~SXE_RFCTL_LRO_DIS;

	if (!is_enable) {
		rfctl |= SXE_RFCTL_LRO_DIS;
	}

	SXE_REG_WRITE(hw, SXE_RFCTL, rfctl);
	return;
}

void sxe_hw_rx_nfs_filter_disable(struct sxe_hw *hw)
{
	u32 rfctl = 0;

	rfctl |= (SXE_RFCTL_NFSW_DIS | SXE_RFCTL_NFSR_DIS);
	SXE_REG_WRITE(hw, SXE_RFCTL, rfctl);
	return;
}

void sxe_hw_rx_udp_frag_checksum_disable(struct sxe_hw *hw)
{
	u32 rxcsum;

	rxcsum = SXE_REG_READ(hw, SXE_RXCSUM);
	rxcsum |= SXE_RXCSUM_PCSD;
	SXE_REG_WRITE(hw, SXE_RXCSUM, rxcsum);
	return;
}

void sxe_hw_fc_mac_addr_set(struct sxe_hw *hw, u8 *mac_addr)
{
	u32 mac_addr_h, mac_addr_l;

	mac_addr_l = ((u32)mac_addr[5] |
		    ((u32)mac_addr[4] << 8) |
		    ((u32)mac_addr[3] << 16) |
		    ((u32)mac_addr[2] << 24));
	mac_addr_h = (((u32)mac_addr[1] << 16) |
		    ((u32)mac_addr[0] << 24));

	SXE_REG_WRITE(hw, SXE_SACONH, mac_addr_h);
	SXE_REG_WRITE(hw, SXE_SACONL, mac_addr_l);

	return;
}

s32 sxe_hw_uc_addr_add(struct sxe_hw *hw, u32 rar_idx,
					u8 *addr, u32 pool_idx)
{
	s32 ret = 0;
	u32 rar_low, rar_high;
	struct sxe_adapter *adapter = hw->adapter;

	if (rar_idx >= SXE_UC_ENTRY_NUM_MAX) {
		LOG_DEV_DEBUG("RAR rar_idx %d is out of range:%u.\n",
			rar_idx, SXE_UC_ENTRY_NUM_MAX);
		ret = -SXE_ERR_PARAM;
		goto l_end;
	}

	sxe_hw_uc_addr_pool_enable(hw, rar_idx, pool_idx);

	rar_low = ((u32)addr[0] |
		   ((u32)addr[1] << 8) |
		   ((u32)addr[2] << 16) |
		   ((u32)addr[3] << 24));

	rar_high = SXE_REG_READ(hw, SXE_RAH(rar_idx));
	rar_high &= ~(0x0000FFFF | SXE_RAH_AV);
	rar_high |= ((u32)addr[4] | ((u32)addr[5] << 8));

	rar_high |= SXE_RAH_AV;

	SXE_REG_WRITE(hw, SXE_RAL(rar_idx), rar_low);
	SXE_WRITE_FLUSH(hw);
	SXE_REG_WRITE(hw, SXE_RAH(rar_idx), rar_high);

	LOG_DEBUG_BDF("rar_idx:%d pool_idx:%u addr:%pM add to rar done\n",
		rar_idx, pool_idx, addr);

l_end:
	return ret;
}

s32 sxe_hw_uc_addr_del(struct sxe_hw *hw, u32 index)
{
	s32 ret = 0;
	u32 rar_high;
	struct sxe_adapter *adapter = hw->adapter;

	if (index >= SXE_UC_ENTRY_NUM_MAX) {
		ret = -SXE_ERR_PARAM;
		LOG_ERROR_BDF("uc_entry_num:%d index:%u invalid.(err:%d)\n",
			  SXE_UC_ENTRY_NUM_MAX, index, ret);
		goto l_end;
	}

	rar_high = SXE_REG_READ(hw, SXE_RAH(index));
	rar_high &= ~(0x0000FFFF | SXE_RAH_AV);

	SXE_REG_WRITE(hw, SXE_RAH(index), rar_high);
	SXE_WRITE_FLUSH(hw);
	SXE_REG_WRITE(hw, SXE_RAL(index), 0);

	sxe_hw_uc_addr_pool_disable(hw, index);

l_end:
	return ret;
}

void sxe_hw_mta_hash_table_set(struct sxe_hw *hw,
						u8 index, u32 value)
{
	SXE_REG_WRITE(hw, SXE_MTA(index), value);
	return;
}

void sxe_hw_mta_hash_table_update(struct sxe_hw *hw,
						u8 reg_idx, u8 bit_idx)
{
	u32 value = SXE_REG_READ(hw, SXE_MTA(reg_idx));

	value |= BIT(bit_idx);

	LOG_INFO("mta update value:0x%x.\n", value);
	SXE_REG_WRITE(hw, SXE_MTA(reg_idx), value);

	return;
}

void sxe_hw_mc_filter_enable(struct sxe_hw *hw)
{
	u32 value = SXE_MC_FILTER_TYPE0 | SXE_MCSTCTRL_MFE;

	SXE_REG_WRITE(hw, SXE_MCSTCTRL, value);

	return;
}

static void sxe_hw_mc_filter_disable(struct sxe_hw *hw)
{
	u32 value = SXE_REG_READ(hw, SXE_MCSTCTRL);

	value &= ~SXE_MCSTCTRL_MFE;

	SXE_REG_WRITE(hw, SXE_MCSTCTRL, value);

	return;
}

void sxe_hw_uc_addr_clear(struct sxe_hw *hw)
{
	u32 i;
	struct sxe_adapter *adapter = hw->adapter;

	sxe_hw_uc_addr_pool_disable(hw, 0);

	LOG_DEV_DEBUG("clear uc filter addr register:0-%d\n",
		   SXE_UC_ENTRY_NUM_MAX - 1);
	for (i = 0; i < SXE_UC_ENTRY_NUM_MAX; i++) {
		SXE_REG_WRITE(hw, SXE_RAL(i), 0);
		SXE_REG_WRITE(hw, SXE_RAH(i), 0);
	}

	LOG_DEV_DEBUG("clear %u uta filter addr register\n",
			SXE_UTA_ENTRY_NUM_MAX);
	for (i = 0; i < SXE_UTA_ENTRY_NUM_MAX; i++) {
		SXE_REG_WRITE(hw, SXE_UTA(i), 0);
	}

	SXE_REG_WRITE(hw, SXE_MCSTCTRL, SXE_MC_FILTER_TYPE0);

	LOG_DEV_DEBUG("clear %u mta filter addr register\n",
			SXE_MTA_ENTRY_NUM_MAX);
	for (i = 0; i < SXE_MTA_ENTRY_NUM_MAX; i++) {
		SXE_REG_WRITE(hw, SXE_MTA(i), 0);
	}

	return;
}

static void sxe_hw_ethertype_filter_set(struct sxe_hw *hw,
						u8 filter_type, u32 value)
{
	SXE_REG_WRITE(hw, SXE_ETQF(filter_type), value);
	return;
}

void sxe_hw_vt_ctrl_cfg(struct sxe_hw *hw, u8 default_pool)
{
	u32 ctrl;

	ctrl = SXE_REG_READ(hw, SXE_VT_CTL);

	ctrl |= SXE_VT_CTL_VT_ENABLE; 
	ctrl &= ~SXE_VT_CTL_POOL_MASK;
	ctrl |= default_pool << SXE_VT_CTL_POOL_SHIFT;
	ctrl |= SXE_VT_CTL_REPLEN; 

	SXE_REG_WRITE(hw, SXE_VT_CTL, ctrl);

	return;
}

void sxe_hw_vt_disable(struct sxe_hw *hw)
{
	u32 vmdctl;

	vmdctl = SXE_REG_READ(hw, SXE_VT_CTL);
	vmdctl &= ~SXE_VMD_CTL_POOL_EN;
	SXE_REG_WRITE(hw, SXE_VT_CTL, vmdctl);

	return;
}

#ifdef SXE_WOL_CONFIGURE

static void sxe_hw_wol_status_set(struct sxe_hw *hw)
{
	SXE_REG_WRITE(hw, SXE_WUS, ~0);

	return;
}

static void sxe_hw_wol_mode_set(struct sxe_hw *hw, u32 wol_status)
{
	u32 fctrl;

	SXE_REG_WRITE(hw, SXE_WUC, SXE_WUC_PME_EN);

	fctrl = SXE_REG_READ(hw, SXE_FCTRL);
	fctrl |= SXE_FCTRL_BAM;
	if (wol_status & SXE_WUFC_MC) {
		fctrl |= SXE_FCTRL_MPE;
	}

	SXE_REG_WRITE(hw, SXE_FCTRL, fctrl);

	SXE_REG_WRITE(hw, SXE_WUFC, wol_status);
	sxe_hw_wol_status_set(hw);

	return;
}

static void sxe_hw_wol_mode_clean(struct sxe_hw *hw)
{
	SXE_REG_WRITE(hw, SXE_WUC, 0);
	SXE_REG_WRITE(hw, SXE_WUFC, 0);

	return;
}
#endif

static const struct sxe_filter_mac_operations sxe_filter_mac_ops = {
	.rx_mode_get			= sxe_hw_rx_mode_get,
	.rx_mode_set			= sxe_hw_rx_mode_set,
	.pool_rx_mode_get		= sxe_hw_pool_rx_mode_get,
	.pool_rx_mode_set		= sxe_hw_pool_rx_mode_set,
	.rx_lro_enable			= sxe_hw_rx_lro_enable,
	.uc_addr_add			= sxe_hw_uc_addr_add,
	.uc_addr_del			= sxe_hw_uc_addr_del,
	.uc_addr_clear			= sxe_hw_uc_addr_clear,
	.fc_mac_addr_set		= sxe_hw_fc_mac_addr_set,
	.mta_hash_table_set		= sxe_hw_mta_hash_table_set,
	.mta_hash_table_update		= sxe_hw_mta_hash_table_update,

	.mc_filter_enable		= sxe_hw_mc_filter_enable,
	.mc_filter_disable		= sxe_hw_mc_filter_disable,
	.rx_nfs_filter_disable		= sxe_hw_rx_nfs_filter_disable,
	.ethertype_filter_set		= sxe_hw_ethertype_filter_set,
	.vt_ctrl_configure		= sxe_hw_vt_ctrl_cfg,
	.uc_addr_pool_enable		= sxe_hw_uc_addr_pool_enable,
	.rx_udp_frag_checksum_disable	= sxe_hw_rx_udp_frag_checksum_disable,

#ifdef SXE_WOL_CONFIGURE
	.wol_mode_set			= sxe_hw_wol_mode_set,
	.wol_mode_clean			= sxe_hw_wol_mode_clean,
	.wol_status_set			= sxe_hw_wol_status_set,
#endif

	.vt_disable                     = sxe_hw_vt_disable,
};

u32 sxe_hw_vlan_pool_filter_read(struct sxe_hw *hw, u16 reg_index)
{
	return SXE_REG_READ(hw, SXE_VLVF(reg_index));
}

static void sxe_hw_vlan_pool_filter_write(struct sxe_hw *hw,
						u16 reg_index, u32 value)
{
	SXE_REG_WRITE(hw, SXE_VLVF(reg_index), value);
	return;
}

static u32 sxe_hw_vlan_pool_filter_bitmap_read(struct sxe_hw *hw,
							u16 reg_index)
{
	return SXE_REG_READ(hw, SXE_VLVFB(reg_index));
}

static void sxe_hw_vlan_pool_filter_bitmap_write(struct sxe_hw *hw,
						u16 reg_index, u32 value)
{
	SXE_REG_WRITE(hw, SXE_VLVFB(reg_index), value);
	return;
}

void sxe_hw_vlan_filter_array_write(struct sxe_hw *hw,
					u16 reg_index, u32 value)
{
	SXE_REG_WRITE(hw, SXE_VFTA(reg_index), value);
	return;
}

u32 sxe_hw_vlan_filter_array_read(struct sxe_hw *hw, u16 reg_index)
{
	return SXE_REG_READ(hw, SXE_VFTA(reg_index));
}

void sxe_hw_vlan_filter_switch(struct sxe_hw *hw, bool is_enable)
{
	u32 vlnctrl;

	vlnctrl = SXE_REG_READ(hw, SXE_VLNCTRL);
	if (is_enable) {
		vlnctrl |= SXE_VLNCTRL_VFE;
	} else {
		vlnctrl &= ~SXE_VLNCTRL_VFE;
	}

	SXE_REG_WRITE(hw, SXE_VLNCTRL, vlnctrl);
	return;
}

static void sxe_hw_vlan_untagged_pkts_rcv_switch(struct sxe_hw *hw,
							u32 vf, bool accept)
{
	u32 vmolr = SXE_REG_READ(hw, SXE_VMOLR(vf));
	vmolr |= SXE_VMOLR_BAM;
	if (accept) {
		vmolr |= SXE_VMOLR_AUPE;
	} else {
		vmolr &= ~SXE_VMOLR_AUPE;
	}

	LOG_WARN("vf:%u value:0x%x.\n", vf, vmolr);
	SXE_REG_WRITE(hw, SXE_VMOLR(vf), vmolr);
	return;
}

s32 sxe_hw_vlvf_slot_find(struct sxe_hw *hw, u32 vlan, bool vlvf_bypass)
{
	s32 ret, regindex, first_empty_slot;
	u32 bits;
	struct sxe_adapter *adapter = hw->adapter;

	if (vlan == 0) {
		ret = 0;
		goto l_end;
	}

	first_empty_slot = vlvf_bypass ? -SXE_ERR_NO_SPACE : 0;

	vlan |= SXE_VLVF_VIEN;

	for (regindex = SXE_VLVF_ENTRIES; --regindex;) {
		bits = SXE_REG_READ(hw, SXE_VLVF(regindex));
		if (bits == vlan) {
			ret = regindex;
			goto l_end;
		}

		if (!first_empty_slot && !bits) {
			first_empty_slot = regindex;
		}
	}

	if (!first_empty_slot) {
		LOG_DEV_WARN("no space in VLVF.\n");
	}

	ret = first_empty_slot ? : -SXE_ERR_NO_SPACE;
l_end:
	return ret;
}

s32 sxe_hw_vlan_filter_configure(struct sxe_hw *hw,
					u32 vid, u32 pool,
					bool vlan_on, bool vlvf_bypass)
{
	s32 ret = 0;
	u32 regidx, vfta_delta, vfta, bits;
	s32 vlvf_index;

	LOG_DEBUG("vid: %u, pool: %u, vlan_on: %d, vlvf_bypass: %d",
		vid, pool, vlan_on, vlvf_bypass);

	if ((vid > 4095) || (pool > 63)) {
		ret = -SXE_ERR_PARAM;
		goto l_end;
	}


	regidx = vid / 32;
	vfta_delta = BIT(vid % 32);
	vfta = SXE_REG_READ(hw, SXE_VFTA(regidx));

	vfta_delta &= vlan_on ? ~vfta : vfta;
	vfta ^= vfta_delta;

	if (!(SXE_REG_READ(hw, SXE_VT_CTL) & SXE_VT_CTL_VT_ENABLE)) {
		goto vfta_update;
	}

	vlvf_index = sxe_hw_vlvf_slot_find(hw, vid, vlvf_bypass);
	if (vlvf_index < 0) {
		if (vlvf_bypass) {
			goto vfta_update;
		}

		ret = vlvf_index;
		goto l_end;
	}

	bits = SXE_REG_READ(hw, SXE_VLVFB(vlvf_index * 2 + pool / 32));

	bits |= BIT(pool % 32);
	if (vlan_on) {
		goto vlvf_update;
	}

	bits ^= BIT(pool % 32);

	if (!bits &&
	    !SXE_REG_READ(hw, SXE_VLVFB(vlvf_index * 2 + 1 - pool / 32))) {
		if (vfta_delta) {
			SXE_REG_WRITE(hw, SXE_VFTA(regidx), vfta);
		}

		SXE_REG_WRITE(hw, SXE_VLVF(vlvf_index), 0);
		SXE_REG_WRITE(hw, SXE_VLVFB(vlvf_index * 2 + pool / 32), 0);

		goto l_end;
	}

	vfta_delta = 0;

vlvf_update:
	SXE_REG_WRITE(hw, SXE_VLVFB(vlvf_index * 2 + pool / 32), bits);
	SXE_REG_WRITE(hw, SXE_VLVF(vlvf_index), SXE_VLVF_VIEN | vid);

vfta_update:
	if (vfta_delta) {
		SXE_REG_WRITE(hw, SXE_VFTA(regidx), vfta);
	}

l_end:
	return ret;
}

void sxe_hw_vlan_filter_array_clear(struct sxe_hw *hw)
{
	u32 offset;

	for (offset = 0; offset < SXE_VFT_TBL_SIZE; offset++) {
		SXE_REG_WRITE(hw, SXE_VFTA(offset), 0);
	}

	for (offset = 0; offset < SXE_VLVF_ENTRIES; offset++) {
		SXE_REG_WRITE(hw, SXE_VLVF(offset), 0);
		SXE_REG_WRITE(hw, SXE_VLVFB(offset * 2), 0);
		SXE_REG_WRITE(hw, SXE_VLVFB(offset * 2 + 1), 0);
	}

	return;
}

static const struct sxe_filter_vlan_operations sxe_filter_vlan_ops = {
	.pool_filter_read		= sxe_hw_vlan_pool_filter_read,
	.pool_filter_write		= sxe_hw_vlan_pool_filter_write,
	.pool_filter_bitmap_read	= sxe_hw_vlan_pool_filter_bitmap_read,
	.pool_filter_bitmap_write	= sxe_hw_vlan_pool_filter_bitmap_write,
	.filter_array_write		= sxe_hw_vlan_filter_array_write,
	.filter_array_read		= sxe_hw_vlan_filter_array_read,
	.filter_array_clear		= sxe_hw_vlan_filter_array_clear,
	.filter_switch			= sxe_hw_vlan_filter_switch,
	.untagged_pkts_rcv_switch	= sxe_hw_vlan_untagged_pkts_rcv_switch,
	.filter_configure		= sxe_hw_vlan_filter_configure,
};


static void sxe_hw_rx_pkt_buf_switch(struct sxe_hw *hw, bool is_on)
{
	u32 dbucfg = SXE_REG_READ(hw, SXE_DRXCFG);

	if (is_on) {
		dbucfg |= SXE_DRXCFG_DBURX_START;
	} else {
		dbucfg &= ~SXE_DRXCFG_DBURX_START;
	}

	SXE_REG_WRITE(hw, SXE_DRXCFG, dbucfg);

	return;
}

static void sxe_hw_rx_pkt_buf_size_configure(struct sxe_hw *hw,
			     u8 num_pb,
			     u32 headroom,
			     u16 strategy)
{
	u16 total_buf_size = (SXE_RX_PKT_BUF_SIZE - headroom);
	u32 rx_buf_size;
	u16 i = 0;

	if (!num_pb) {
		num_pb = 1;
	}

	switch (strategy) {
	case (PBA_STRATEGY_WEIGHTED):
		rx_buf_size = ((total_buf_size * 5 * 2) / (num_pb * 8));
		total_buf_size -= rx_buf_size * (num_pb / 2);
		rx_buf_size <<= SXE_RX_PKT_BUF_SIZE_SHIFT;
		for (i = 0; i < (num_pb / 2); i++) {
			SXE_REG_WRITE(hw, SXE_RXPBSIZE(i), rx_buf_size);
		}
		fallthrough;
	case (PBA_STRATEGY_EQUAL):
		rx_buf_size = (total_buf_size / (num_pb - i))
				<< SXE_RX_PKT_BUF_SIZE_SHIFT;
		for (; i < num_pb; i++) {
			SXE_REG_WRITE(hw, SXE_RXPBSIZE(i), rx_buf_size);
		}
		break;

	default:
		break;
	}

	for (; i < SXE_PKG_BUF_NUM_MAX; i++) {
		SXE_REG_WRITE(hw, SXE_RXPBSIZE(i), 0);
	}

	return;
}

u32 sxe_hw_rx_pkt_buf_size_get(struct sxe_hw *hw, u8 pb)
{
	return SXE_REG_READ(hw, SXE_RXPBSIZE(pb));
}

void sxe_hw_rx_multi_ring_configure(struct sxe_hw *hw,
						u8 tcs, bool is_4q_per_pool,
						bool sriov_enable)
{
	u32 mrqc = SXE_REG_READ(hw, SXE_MRQC);

	mrqc &= ~SXE_MRQE_MASK;

	if (sriov_enable) {
		if (tcs > 4) {
			mrqc |= SXE_MRQC_VMDQRT8TCEN;	
		} else if (tcs > 1) {
			mrqc |= SXE_MRQC_VMDQRT4TCEN;	
		} else if (is_4q_per_pool == true) {
			mrqc |= SXE_MRQC_VMDQRSS32EN;
		} else {
			mrqc |= SXE_MRQC_VMDQRSS64EN;
		}
	} else {
		if (tcs > 4) {
			mrqc |= SXE_MRQC_RTRSS8TCEN;
		} else if (tcs > 1) {
			mrqc |= SXE_MRQC_RTRSS4TCEN;
		} else {
			mrqc |= SXE_MRQC_RSSEN;
		}
	}

	SXE_REG_WRITE(hw, SXE_MRQC, mrqc);

	return;
}

static void sxe_hw_rss_hash_pkt_type_set(struct sxe_hw *hw, u32 version)
{
	u32 mrqc = 0;
	u32 rss_field = 0;

	rss_field |= SXE_MRQC_RSS_FIELD_IPV4 |
		     SXE_MRQC_RSS_FIELD_IPV4_TCP |
		     SXE_MRQC_RSS_FIELD_IPV6 |
		     SXE_MRQC_RSS_FIELD_IPV6_TCP;

	if (version == SXE_RSS_IP_VER_4) {
		rss_field |= SXE_MRQC_RSS_FIELD_IPV4_UDP;
	}
	if (version == SXE_RSS_IP_VER_6) {
		rss_field |= SXE_MRQC_RSS_FIELD_IPV6_UDP;
	}

	mrqc |= rss_field;
	SXE_REG_WRITE(hw, SXE_MRQC, mrqc);

	return;
}

static void sxe_hw_rss_hash_pkt_type_update(struct sxe_hw *hw,
							u32 version)
{
	u32 mrqc;

	mrqc = SXE_REG_READ(hw, SXE_MRQC);

	mrqc |= SXE_MRQC_RSS_FIELD_IPV4
	      | SXE_MRQC_RSS_FIELD_IPV4_TCP
	      | SXE_MRQC_RSS_FIELD_IPV6
	      | SXE_MRQC_RSS_FIELD_IPV6_TCP;

	mrqc &= ~(SXE_MRQC_RSS_FIELD_IPV4_UDP |
		  SXE_MRQC_RSS_FIELD_IPV6_UDP);

	if (version == SXE_RSS_IP_VER_4) {
		mrqc |= SXE_MRQC_RSS_FIELD_IPV4_UDP;
	}
	if (version == SXE_RSS_IP_VER_6) {
		mrqc |= SXE_MRQC_RSS_FIELD_IPV6_UDP;
	}

	SXE_REG_WRITE(hw, SXE_MRQC, mrqc);

	return;
}

static void sxe_hw_rss_rings_used_set(struct sxe_hw *hw, u32 rss_num,
						u16 pool, u16 pf_offset)
{
	u32 psrtype = 0;

	if (rss_num > 3) {
		psrtype |= 2u << 29;
	} else if (rss_num > 1) {
		psrtype |= 1u << 29;
	}

	while (pool--) {
		SXE_REG_WRITE(hw, SXE_PSRTYPE(pf_offset + pool), psrtype);
	}

	return;
}

void sxe_hw_rss_key_set_all(struct sxe_hw *hw, u32 *rss_key)
{
	u32 i;

	for (i = 0; i < SXE_MAX_RSS_KEY_ENTRIES; i++) {
		SXE_REG_WRITE(hw, SXE_RSSRK(i), rss_key[i]);
	}

	return;
}

void sxe_hw_rss_redir_tbl_reg_write(struct sxe_hw *hw,
						u16 reg_idx, u32 value)
{
	SXE_REG_WRITE(hw, SXE_RETA(reg_idx >> 2), value);
	return;
}

void sxe_hw_rss_redir_tbl_set_all(struct sxe_hw *hw, u8 *redir_tbl)
{
	u32 i;
	u32 tbl = 0;
	u32 indices_multi = 0x1;


	for (i = 0; i < SXE_MAX_RETA_ENTRIES; i++) {
		tbl |= indices_multi * redir_tbl[i] << (i & 0x3) * 8;
		if ((i & 3) == 3) {
			sxe_hw_rss_redir_tbl_reg_write(hw, i, tbl);
			tbl = 0;
		}
	}
	return;
}

void sxe_hw_rx_cap_switch_on(struct sxe_hw *hw)
{
	u32 rxctrl;

	if (hw->mac.set_lben) {
		u32 pfdtxgswc = SXE_REG_READ(hw, SXE_PFDTXGSWC);
		pfdtxgswc |= SXE_PFDTXGSWC_VT_LBEN;
		SXE_REG_WRITE(hw, SXE_PFDTXGSWC, pfdtxgswc);
		hw->mac.set_lben = false;
	}

	rxctrl = SXE_REG_READ(hw, SXE_RXCTRL);
	rxctrl |= SXE_RXCTRL_RXEN;
	SXE_REG_WRITE(hw, SXE_RXCTRL, rxctrl);

	return;
}

void sxe_hw_rx_cap_switch_off(struct sxe_hw *hw)
{
	u32 rxctrl;

	rxctrl = SXE_REG_READ(hw, SXE_RXCTRL);
	if (rxctrl & SXE_RXCTRL_RXEN) {
		u32 pfdtxgswc = SXE_REG_READ(hw, SXE_PFDTXGSWC);
		if (pfdtxgswc & SXE_PFDTXGSWC_VT_LBEN) {
			pfdtxgswc &= ~SXE_PFDTXGSWC_VT_LBEN;
			SXE_REG_WRITE(hw, SXE_PFDTXGSWC, pfdtxgswc);
			hw->mac.set_lben = true;
		} else {
			hw->mac.set_lben = false;
		}
		rxctrl &= ~SXE_RXCTRL_RXEN;
		SXE_REG_WRITE(hw, SXE_RXCTRL, rxctrl);
	}

	return;
}

static void sxe_hw_rx_func_switch_on(struct sxe_hw *hw)
{
	u32 rxctrl;

	rxctrl = SXE_REG_READ(hw, SXE_COMCTRL);
	rxctrl |= SXE_COMCTRL_RXEN | SXE_COMCTRL_EDSEL;
	SXE_REG_WRITE(hw, SXE_COMCTRL, rxctrl);

	return;
}

void sxe_hw_tx_pkt_buf_switch(struct sxe_hw *hw, bool is_on)
{
	u32 dbucfg;

	dbucfg = SXE_REG_READ(hw, SXE_DTXCFG);

	if (is_on) {
		dbucfg |= SXE_DTXCFG_DBUTX_START;
		dbucfg |= SXE_DTXCFG_DBUTX_BUF_ALFUL_CFG;
		SXE_REG_WRITE(hw, SXE_DTXCFG, dbucfg);
	} else {
		dbucfg &= ~SXE_DTXCFG_DBUTX_START;
		SXE_REG_WRITE(hw, SXE_DTXCFG, dbucfg);
	}

	return;
}

void sxe_hw_tx_pkt_buf_size_configure(struct sxe_hw *hw, u8 num_pb)
{
	u32 i, tx_pkt_size;

	if (!num_pb){
		num_pb = 1;
	}

	tx_pkt_size = SXE_TX_PBSIZE_MAX / num_pb;
	for (i = 0; i < num_pb; i++) {
		SXE_REG_WRITE(hw, SXE_TXPBSIZE(i), tx_pkt_size);
	}

	for (; i < SXE_PKG_BUF_NUM_MAX; i++) {
		SXE_REG_WRITE(hw, SXE_TXPBSIZE(i), 0);
	}

	return;
}

void sxe_hw_rx_lro_ack_switch(struct sxe_hw *hw, bool is_on)
{
	u32 lro_dbu = SXE_REG_READ(hw, SXE_LRODBU);

	if (is_on) {
		lro_dbu &= ~SXE_LRODBU_LROACKDIS;
	} else {
		lro_dbu |= SXE_LRODBU_LROACKDIS;
	}

	SXE_REG_WRITE(hw, SXE_LRODBU, lro_dbu);

	return;
}

static void sxe_hw_vf_rx_switch(struct sxe_hw *hw,
				u32 reg_offset, u32 vf_index, bool is_off)
{
	u32 vfre = SXE_REG_READ(hw, SXE_VFRE(reg_offset));
	if (is_off) {
		vfre &= ~BIT(vf_index);
	} else {
		vfre |= BIT(vf_index);
	}

	SXE_REG_WRITE(hw, SXE_VFRE(reg_offset), vfre);

	return;
}

STATIC s32 sxe_hw_fnav_wait_init_done(struct sxe_hw *hw)
{
	u32 i;
	s32 ret = 0;
	struct sxe_adapter *adapter = hw->adapter;
	for (i = 0; i < SXE_FNAV_INIT_DONE_POLL; i++) {
		if (SXE_REG_READ(hw, SXE_FNAVCTRL) &
				   SXE_FNAVCTRL_INIT_DONE) {
			break;
		}

		usleep_range(1000, 2000);
	}

	if (i >= SXE_FNAV_INIT_DONE_POLL) {
		LOG_DEV_DEBUG("flow navigator poll time exceeded!\n");
		ret = -SXE_ERR_FNAV_REINIT_FAILED;
	}

	return ret;
}

void sxe_hw_fnav_enable(struct sxe_hw *hw, u32 fnavctrl)
{
	u32 fnavctrl_ori;
	bool is_clear_stat = false;

	SXE_REG_WRITE(hw, SXE_FNAVHKEY, SXE_FNAV_BUCKET_HASH_KEY);
	SXE_REG_WRITE(hw, SXE_FNAVSKEY, SXE_FNAV_SAMPLE_HASH_KEY);

	fnavctrl_ori = SXE_REG_READ(hw, SXE_FNAVCTRL);
	if((fnavctrl_ori & 0x13) != (fnavctrl & 0x13)) {
		is_clear_stat = true;
	}

	SXE_REG_WRITE(hw, SXE_FNAVCTRL, fnavctrl);
	SXE_WRITE_FLUSH(hw);

	sxe_hw_fnav_wait_init_done(hw);

	if(is_clear_stat) {
		SXE_REG_READ(hw, SXE_FNAVUSTAT);
		SXE_REG_READ(hw, SXE_FNAVFSTAT);
		SXE_REG_READ(hw, SXE_FNAVMATCH);
		SXE_REG_READ(hw, SXE_FNAVMISS);
		SXE_REG_READ(hw, SXE_FNAVLEN);
	}

	return;
}

static s32 sxe_hw_fnav_mode_init(struct sxe_hw *hw,
					u32 fnavctrl, u32 sxe_fnav_mode)
{
	struct sxe_adapter *adapter = hw->adapter;

	LOG_DEBUG_BDF("fnavctrl=0x%x, sxe_fnav_mode=%u\n", fnavctrl, sxe_fnav_mode);

	if ((sxe_fnav_mode != SXE_FNAV_SAMPLE_MODE) &&
		(sxe_fnav_mode != SXE_FNAV_SPECIFIC_MODE)) {
		LOG_ERROR_BDF("mode[%u] a error fnav mode, fnav do not work. please use"
			"SXE_FNAV_SAMPLE_MODE or SXE_FNAV_SPECIFIC_MODE\n",
			sxe_fnav_mode);
		goto l_end;
	}

	if (sxe_fnav_mode == SXE_FNAV_SPECIFIC_MODE) {
		fnavctrl |= SXE_FNAVCTRL_SPECIFIC_MATCH |
			 (SXE_FNAV_DROP_QUEUE << SXE_FNAVCTRL_DROP_Q_SHIFT);
	}

	fnavctrl |= (0x6 << SXE_FNAVCTRL_FLEX_SHIFT) |
		    (0xA << SXE_FNAVCTRL_MAX_LENGTH_SHIFT) |
		    (4 << SXE_FNAVCTRL_FULL_THRESH_SHIFT);

	sxe_hw_fnav_enable(hw, fnavctrl);

l_end:
	return 0;
}

u32 sxe_hw_fnav_port_mask_get(__be16 src_port_mask, __be16 dst_port_mask)
{
	u32 mask = ntohs(dst_port_mask);

	mask <<= SXE_FNAVTCPM_DPORTM_SHIFT;
	mask |= ntohs(src_port_mask);
	mask = ((mask & 0x55555555) << 1) | ((mask & 0xAAAAAAAA) >> 1);
	mask = ((mask & 0x33333333) << 2) | ((mask & 0xCCCCCCCC) >> 2);
	mask = ((mask & 0x0F0F0F0F) << 4) | ((mask & 0xF0F0F0F0) >> 4);
	return ((mask & 0x00FF00FF) << 8) | ((mask & 0xFF00FF00) >> 8);
}

static s32 sxe_hw_fnav_vm_pool_mask_get(struct sxe_hw *hw,
					u8 vm_pool, u32 *fnavm)
{
	s32 ret = 0;
	struct sxe_adapter *adapter = hw->adapter;

	switch (vm_pool & SXE_SAMPLE_VM_POOL_MASK) {
	case 0x0:
		*fnavm |= SXE_FNAVM_POOL;
		fallthrough;
	case 0x7F:
		break;
	default:
		LOG_DEV_ERR("error on vm pool mask\n");
		ret = -SXE_ERR_CONFIG;
	}

	return ret;
}

static s32 sxe_hw_fnav_flow_type_mask_get(struct sxe_hw *hw,
					union sxe_fnav_rule_info *input_mask,
					u32 *fnavm)
{
	s32 ret = 0;
	struct sxe_adapter *adapter = hw->adapter;

	switch (input_mask->ntuple.flow_type & SXE_SAMPLE_L4TYPE_MASK) {
	case 0x0:
		*fnavm |= SXE_FNAVM_L4P;
		if (input_mask->ntuple.dst_port ||
		    input_mask->ntuple.src_port) {
			LOG_DEV_ERR("error on src/dst port mask\n");
			ret = -SXE_ERR_CONFIG;
			goto l_ret;
		}
		break;
	case SXE_SAMPLE_L4TYPE_MASK:
		break;
	default:
		LOG_DEV_ERR("error on flow type mask\n");
		ret = -SXE_ERR_CONFIG;
	}

l_ret:
	return ret;
}

static s32 sxe_hw_fnav_vlan_mask_get(struct sxe_hw *hw,
					__be16 vlan_id, u32 *fnavm)
{
	s32 ret = 0;
	struct sxe_adapter *adapter = hw->adapter;

	switch (ntohs(vlan_id) & SXE_SAMPLE_VLAN_MASK) {
	case 0x0000:
		*fnavm |= SXE_FNAVM_VLANID;
		fallthrough;
	case 0x0FFF:
		*fnavm |= SXE_FNAVM_VLANP;
		break;
	case 0xE000:
		*fnavm |= SXE_FNAVM_VLANID;
		fallthrough;
	case 0xEFFF:
		break;
	default:
		LOG_DEV_ERR("error on VLAN mask\n");
		ret = -SXE_ERR_CONFIG;
	}

	return ret;
}

static s32 sxe_hw_fnav_flex_bytes_mask_get(struct sxe_hw *hw,
					__be16 flex_bytes, u32 *fnavm)
{
	s32 ret = 0;
	struct sxe_adapter *adapter = hw->adapter;

	switch ((__force u16)flex_bytes & SXE_SAMPLE_FLEX_BYTES_MASK) {
	case 0x0000:
		*fnavm |= SXE_FNAVM_FLEX;
		fallthrough;
	case 0xFFFF:
		break;
	default:
		LOG_DEV_ERR("error on flexible byte mask\n");
		ret = -SXE_ERR_CONFIG;
	}

	return ret;
}

s32 sxe_hw_fnav_specific_rule_mask_set(struct sxe_hw *hw,
				    union sxe_fnav_rule_info *input_mask)
{
	s32 ret;
	u32 fnavm = SXE_FNAVM_DIPv6;
	u32 fnavtcpm;
	struct sxe_adapter *adapter = hw->adapter;


	if (input_mask->ntuple.bkt_hash) {
		LOG_DEV_ERR("bucket hash should always be 0 in mask\n");
	}

	ret = sxe_hw_fnav_vm_pool_mask_get(hw, input_mask->ntuple.vm_pool, &fnavm);
	if (ret) {
		goto l_err_config;
	}

	ret = sxe_hw_fnav_flow_type_mask_get(hw, input_mask,  &fnavm);
	if (ret) {
		goto l_err_config;
	}

	ret = sxe_hw_fnav_vlan_mask_get(hw, input_mask->ntuple.vlan_id, &fnavm);
	if (ret) {
		goto l_err_config;
	}

	ret = sxe_hw_fnav_flex_bytes_mask_get(hw, input_mask->ntuple.flex_bytes, &fnavm);
	if (ret) {
		goto l_err_config;
	}

	LOG_DEBUG_BDF("fnavm = 0x%x\n", fnavm);
	SXE_REG_WRITE(hw, SXE_FNAVM, fnavm);

	fnavtcpm = sxe_hw_fnav_port_mask_get(input_mask->ntuple.src_port,
					     input_mask->ntuple.dst_port);

	LOG_DEBUG_BDF("fnavtcpm = 0x%x\n", fnavtcpm);
	SXE_REG_WRITE(hw, SXE_FNAVTCPM, ~fnavtcpm);
	SXE_REG_WRITE(hw, SXE_FNAVUDPM, ~fnavtcpm);

	SXE_REG_WRITE_BE32(hw, SXE_FNAVSIP4M,
			     ~input_mask->ntuple.src_ip[0]);
	SXE_REG_WRITE_BE32(hw, SXE_FNAVDIP4M,
			     ~input_mask->ntuple.dst_ip[0]);

	return 0;

l_err_config:
	return -SXE_ERR_CONFIG;
}

STATIC s32 sxe_hw_fnav_cmd_complete_check(struct sxe_hw *hw,
							u32 *fnavcmd)
{
	u32 i;

	for (i = 0; i < SXE_FNAVCMD_CMD_POLL * 10; i++) {
		*fnavcmd = SXE_REG_READ(hw, SXE_FNAVCMD);
		if (!(*fnavcmd & SXE_FNAVCMD_CMD_MASK)) {
			return 0;
		}

		udelay(10);
	}

	return -SXE_ERR_FNAV_CMD_INCOMPLETE;
}

static void sxe_hw_fnav_filter_ip_set(struct sxe_hw *hw,
					union sxe_fnav_rule_info *input)
{
	SXE_REG_WRITE_BE32(hw, SXE_FNAVSIPv6(0),
			     input->ntuple.src_ip[0]);
	SXE_REG_WRITE_BE32(hw, SXE_FNAVSIPv6(1),
			     input->ntuple.src_ip[1]);
	SXE_REG_WRITE_BE32(hw, SXE_FNAVSIPv6(2),
			     input->ntuple.src_ip[2]);

	SXE_REG_WRITE_BE32(hw, SXE_FNAVIPSA, input->ntuple.src_ip[0]);

	SXE_REG_WRITE_BE32(hw, SXE_FNAVIPDA, input->ntuple.dst_ip[0]);

	return;
}

static void sxe_hw_fnav_filter_port_set(struct sxe_hw *hw,
					union sxe_fnav_rule_info *input)
{
	u32 fnavport;

	fnavport = be16_to_cpu(input->ntuple.dst_port);
	fnavport <<= SXE_FNAVPORT_DESTINATION_SHIFT;
	fnavport |= be16_to_cpu(input->ntuple.src_port);
	SXE_REG_WRITE(hw, SXE_FNAVPORT, fnavport);

	return;
}

static void sxe_hw_fnav_filter_vlan_set(struct sxe_hw *hw,
					union sxe_fnav_rule_info *input)
{
	u32 fnavvlan;

	fnavvlan = ntohs(SXE_SWAP_16(input->ntuple.flex_bytes));
	fnavvlan <<= SXE_FNAVVLAN_FLEX_SHIFT;
	fnavvlan |= ntohs(input->ntuple.vlan_id);
	SXE_REG_WRITE(hw, SXE_FNAVVLAN, fnavvlan);

	return;
}

static void sxe_hw_fnav_filter_bkt_hash_set(struct sxe_hw *hw,
					union sxe_fnav_rule_info *input,
					u16 soft_id)
{
	u32 fnavhash;

	fnavhash = (__force u32)input->ntuple.bkt_hash;
	fnavhash |= soft_id << SXE_FNAVHASH_SIG_SW_INDEX_SHIFT;
	SXE_REG_WRITE(hw, SXE_FNAVHASH, fnavhash);

	return;
}

static s32 sxe_hw_fnav_filter_cmd_set(struct sxe_hw *hw,
					union sxe_fnav_rule_info *input,
					u8 queue)
{
	u32 fnavcmd;
	s32 ret;
	struct sxe_adapter *adapter = hw->adapter;

	fnavcmd = SXE_FNAVCMD_CMD_ADD_FLOW | SXE_FNAVCMD_FILTER_UPDATE |
		  SXE_FNAVCMD_LAST | SXE_FNAVCMD_QUEUE_EN;

#ifndef SXE_DPDK
	if (queue == SXE_FNAV_DROP_QUEUE) {
		fnavcmd |= SXE_FNAVCMD_DROP;
	}
#endif

	fnavcmd |= input->ntuple.flow_type << SXE_FNAVCMD_FLOW_TYPE_SHIFT;
	fnavcmd |= (u32)queue << SXE_FNAVCMD_RX_QUEUE_SHIFT;
	fnavcmd |= (u32)input->ntuple.vm_pool << SXE_FNAVCMD_VT_POOL_SHIFT;

	SXE_REG_WRITE(hw, SXE_FNAVCMD, fnavcmd);
	ret = sxe_hw_fnav_cmd_complete_check(hw, &fnavcmd);
	if (ret) {
		LOG_DEV_ERR("flow navigator command did not complete!\n");
	}

	return ret;
}

s32 sxe_hw_fnav_specific_rule_add(struct sxe_hw *hw,
					  union sxe_fnav_rule_info *input,
					  u16 soft_id, u8 queue)
{
	s32 ret;
	struct sxe_adapter *adapter = hw->adapter;

	sxe_hw_fnav_filter_ip_set(hw, input);

	sxe_hw_fnav_filter_port_set(hw, input);

	sxe_hw_fnav_filter_vlan_set(hw, input);

	sxe_hw_fnav_filter_bkt_hash_set(hw, input, soft_id);

	SXE_WRITE_FLUSH(hw);

	ret = sxe_hw_fnav_filter_cmd_set(hw, input, queue);
	if (ret) {
		LOG_ERROR_BDF("set fnav filter cmd error. ret=%d\n", ret);
	}

	return ret;
}

s32 sxe_hw_fnav_specific_rule_del(struct sxe_hw *hw,
					  union sxe_fnav_rule_info *input,
					  u16 soft_id)
{
	u32 fnavhash;
	u32 fnavcmd;
	s32 ret;
	struct sxe_adapter *adapter = hw->adapter;


	fnavhash = (__force u32)input->ntuple.bkt_hash;
	fnavhash |= soft_id << SXE_FNAVHASH_SIG_SW_INDEX_SHIFT;
	SXE_REG_WRITE(hw, SXE_FNAVHASH, fnavhash);

	SXE_WRITE_FLUSH(hw);

	SXE_REG_WRITE(hw, SXE_FNAVCMD, SXE_FNAVCMD_CMD_QUERY_REM_FILT);

	ret = sxe_hw_fnav_cmd_complete_check(hw, &fnavcmd);
	if (ret) {
		LOG_DEV_ERR("flow navigator command did not complete!\n");
		return ret;
	}

	if (fnavcmd & SXE_FNAVCMD_FILTER_VALID) {
		SXE_REG_WRITE(hw, SXE_FNAVHASH, fnavhash);
		SXE_WRITE_FLUSH(hw);
		SXE_REG_WRITE(hw, SXE_FNAVCMD,
				SXE_FNAVCMD_CMD_REMOVE_FLOW);
	}

	return 0;
}

void sxe_hw_fnav_sample_rule_configure(struct sxe_hw *hw,
					  u8 flow_type, u32 hash_value, u8 queue)
{
	u32 fnavcmd;
	u64 fnavhashcmd;
	struct sxe_adapter *adapter = hw->adapter;

	fnavcmd = SXE_FNAVCMD_CMD_ADD_FLOW | SXE_FNAVCMD_FILTER_UPDATE |
		  SXE_FNAVCMD_LAST | SXE_FNAVCMD_QUEUE_EN;
	fnavcmd |= (u32)flow_type << SXE_FNAVCMD_FLOW_TYPE_SHIFT;
	fnavcmd |= (u32)queue << SXE_FNAVCMD_RX_QUEUE_SHIFT;

	fnavhashcmd = (u64)fnavcmd << 32;
	fnavhashcmd |= hash_value;
	SXE_REG64_WRITE(hw, SXE_FNAVHASH, fnavhashcmd);

	LOG_DEV_DEBUG("tx queue=%x hash=%x\n", queue, (u32)fnavhashcmd);

	return;
}

static u64 sxe_hw_fnav_sample_rule_hash_get(struct sxe_hw *hw,
					  u8 flow_type, u32 hash_value, u8 queue)
{
	u32 fnavcmd;
	u64 fnavhashcmd;
	struct sxe_adapter *adapter = hw->adapter;

	fnavcmd = SXE_FNAVCMD_CMD_ADD_FLOW | SXE_FNAVCMD_FILTER_UPDATE |
		  SXE_FNAVCMD_LAST | SXE_FNAVCMD_QUEUE_EN;
	fnavcmd |= (u32)flow_type << SXE_FNAVCMD_FLOW_TYPE_SHIFT;
	fnavcmd |= (u32)queue << SXE_FNAVCMD_RX_QUEUE_SHIFT;

	fnavhashcmd = (u64)fnavcmd << 32;
	fnavhashcmd |= hash_value;

	LOG_DEV_DEBUG("tx queue=%x hash=%x\n", queue, (u32)fnavhashcmd);

	return fnavhashcmd;
}

static s32 sxe_hw_fnav_sample_hash_cmd_get(struct sxe_hw *hw,
					  u8  flow_type,
					  u32 hash_value,
					  u8  queue, u64 *hash_cmd)
{
	s32 ret = 0;
	u8 pkg_type;
	struct sxe_adapter *adapter = hw->adapter;

	pkg_type = flow_type & SXE_SAMPLE_FLOW_TYPE_MASK;
	switch (pkg_type) {
	case SXE_SAMPLE_FLOW_TYPE_TCPV4:
	case SXE_SAMPLE_FLOW_TYPE_UDPV4:
	case SXE_SAMPLE_FLOW_TYPE_SCTPV4:
	case SXE_SAMPLE_FLOW_TYPE_TCPV6:
	case SXE_SAMPLE_FLOW_TYPE_UDPV6:
	case SXE_SAMPLE_FLOW_TYPE_SCTPV6:
		break;
	default:
		LOG_DEV_ERR("error on flow type input\n");
		ret = -SXE_ERR_CONFIG;
		goto l_end;
	}

	*hash_cmd = sxe_hw_fnav_sample_rule_hash_get(hw, pkg_type, hash_value, queue);

l_end:
	return ret;
}

static s32 sxe_hw_fnav_single_sample_rule_del(struct sxe_hw *hw,
						u32 hash)
{
	u32 fdircmd;
	s32 ret;
	struct sxe_adapter *adapter = hw->adapter;

	SXE_REG_WRITE(hw, SXE_FNAVHASH, hash);
	SXE_WRITE_FLUSH(hw);

	SXE_REG_WRITE(hw, SXE_FNAVCMD, SXE_FNAVCMD_CMD_REMOVE_FLOW);
	ret = sxe_hw_fnav_cmd_complete_check(hw, &fdircmd);
	if (ret) {
		LOG_DEV_ERR("flow navigator previous command did not complete,"
			"aborting table re-initialization.\n");
	}

	return ret;
}

s32 sxe_hw_fnav_sample_rules_table_reinit(struct sxe_hw *hw)
{
	u32 fnavctrl = SXE_REG_READ(hw, SXE_FNAVCTRL);
	u32 fnavcmd;
	s32 ret;
	struct sxe_adapter *adapter = hw->adapter;

	fnavctrl &= ~SXE_FNAVCTRL_INIT_DONE;

	ret = sxe_hw_fnav_cmd_complete_check(hw, &fnavcmd);
	if (ret) {
		LOG_DEV_ERR("flow navigator previous command did not complete,"
			"aborting table re-initialization.\n");
		goto l_ret;
	}

	SXE_REG_WRITE(hw, SXE_FNAVFREE, 0);
	SXE_WRITE_FLUSH(hw);

	SXE_REG_WRITE(hw, SXE_FNAVCMD,
			(SXE_REG_READ(hw, SXE_FNAVCMD) |
			 SXE_FNAVCMD_CLEARHT));
	SXE_WRITE_FLUSH(hw);
	SXE_REG_WRITE(hw, SXE_FNAVCMD,
			(SXE_REG_READ(hw, SXE_FNAVCMD) &
			 ~SXE_FNAVCMD_CLEARHT));
	SXE_WRITE_FLUSH(hw);

	SXE_REG_WRITE(hw, SXE_FNAVHASH, 0x00);
	SXE_WRITE_FLUSH(hw);

	SXE_REG_WRITE(hw, SXE_FNAVCTRL, fnavctrl);
	SXE_WRITE_FLUSH(hw);

	ret = sxe_hw_fnav_wait_init_done(hw);
	if (ret) {
		LOG_ERROR_BDF("flow navigator simple poll time exceeded!\n");
		goto l_ret;
	}

	SXE_REG_READ(hw, SXE_FNAVUSTAT);
	SXE_REG_READ(hw, SXE_FNAVFSTAT);
	SXE_REG_READ(hw, SXE_FNAVMATCH);
	SXE_REG_READ(hw, SXE_FNAVMISS);
	SXE_REG_READ(hw, SXE_FNAVLEN);

l_ret:
	return ret;
}

static void sxe_hw_fnav_sample_stats_reinit(struct sxe_hw *hw)
{
	SXE_REG_READ(hw, SXE_FNAVUSTAT);
	SXE_REG_READ(hw, SXE_FNAVFSTAT);
	SXE_REG_READ(hw, SXE_FNAVMATCH);
	SXE_REG_READ(hw, SXE_FNAVMISS);
	SXE_REG_READ(hw, SXE_FNAVLEN);

	return;
}

static void sxe_hw_ptp_freq_adjust(struct sxe_hw *hw, u32 adj_freq)
{
	SXE_REG_WRITE(hw, SXE_TIMADJL, 0);
	SXE_REG_WRITE(hw, SXE_TIMADJH, adj_freq);
	SXE_WRITE_FLUSH(hw);

	return;
}

u64 sxe_hw_ptp_systime_get(struct sxe_hw *hw)
{
	struct sxe_adapter *adapter = hw->adapter;
	u32 systiml;
	u32 systimm;
	u64 ns;

	systiml = SXE_REG_READ(hw, SXE_SYSTIML);
	systimm = SXE_REG_READ(hw, SXE_SYSTIMM);
	ns = SXE_TIME_TO_NS(systiml, systimm);

	LOG_DEBUG_BDF("get ptp hw systime systiml=%u, systimm=%u, ns=%"SXE_PRIU64"\n",
			systiml, systimm, ns);
	return ns;
}

void sxe_hw_ptp_systime_init(struct sxe_hw *hw)
{
	SXE_REG_WRITE(hw, SXE_SYSTIML, 0);
	SXE_REG_WRITE(hw, SXE_SYSTIMM, 0);
	SXE_REG_WRITE(hw, SXE_SYSTIMH, 0);

	SXE_WRITE_FLUSH(hw);
	return;
}

void sxe_hw_ptp_init(struct sxe_hw *hw)
{
	u32 regval;
	u32 tsctl = SXE_TSCTRL_TSEN |
	SXE_TSCTRL_VER_2 |
	SXE_TSCTRL_PTYP_ALL |
	SXE_TSCTRL_L4_UNICAST;

	regval = SXE_REG_READ(hw, SXE_TSCTRL);
	regval &= ~SXE_TSCTRL_ONESTEP;	
	regval &= ~SXE_TSCTRL_CSEN;	
	regval |= tsctl;
	SXE_REG_WRITE(hw, SXE_TSCTRL, regval);

	SXE_REG_WRITE(hw, SXE_TIMINC,
			SXE_TIMINC_SET(SXE_INCPD, SXE_IV_NS, SXE_IV_SNS));

	return;
}

void sxe_hw_ptp_rx_timestamp_clear(struct sxe_hw *hw)
{
	SXE_REG_READ(hw, SXE_RXSTMPH);
	return;
}

void sxe_hw_ptp_tx_timestamp_get(struct sxe_hw *hw,
						u32 *ts_sec, u32 *ts_ns)
{
	u32 reg_sec;
	u32 reg_ns;
	u32 sec_8bit;
	u32 sec_24bit;
	u32 systimm;
	u32 systimm_8bit;
	u32 systimm_24bit;

	SXE_REG64_WRITE(hw, SXE_TXSTMP_SEL, SXE_TXTS_MAGIC0);
	reg_ns = SXE_REG_READ(hw, SXE_TXSTMP_VAL);
	SXE_REG64_WRITE(hw, SXE_TXSTMP_SEL, SXE_TXTS_MAGIC1);
	reg_sec = SXE_REG_READ(hw, SXE_TXSTMP_VAL);
	systimm = SXE_REG_READ(hw, SXE_SYSTIMM);


	sec_8bit  = reg_sec & 0x000000FF;
	sec_24bit = (reg_sec >> 8) & 0x00FFFFFF;

	systimm_24bit = systimm & 0x00FFFFFF;
	systimm_8bit  = systimm & 0xFF000000;

	*ts_ns  = (sec_8bit << 24) | ((reg_ns & 0xFFFFFF00) >> 8);

	if (unlikely((sec_24bit - systimm_24bit) >= 0x00FFFFF0)) {
		if (systimm_8bit >= 1) {
			systimm_8bit -= 1;
		}
	}

	*ts_sec = systimm_8bit | sec_24bit;
	return;
}

u64 sxe_hw_ptp_rx_timestamp_get(struct sxe_hw *hw)
{
	struct sxe_adapter *adapter = hw->adapter;
	u32 rxtsl;
	u32 rxtsh;
	u64 ns;

	rxtsl = SXE_REG_READ(hw, SXE_RXSTMPL);
	rxtsh = SXE_REG_READ(hw, SXE_RXSTMPH);
	ns = SXE_TIME_TO_NS(rxtsl, rxtsh);

	LOG_DEBUG_BDF("ptp get rx ptp timestamp low=%u, high=%u, ns=%"SXE_PRIU64"\n",
			rxtsl, rxtsh, ns);
	return ns;
}

bool sxe_hw_ptp_is_rx_timestamp_valid(struct sxe_hw *hw)
{
	bool rx_tmstamp_valid = false;
	u32 tsyncrxctl;

	tsyncrxctl = SXE_REG_READ(hw, SXE_TSYNCRXCTL);
	if (tsyncrxctl & SXE_TSYNCRXCTL_RXTT) {
		rx_tmstamp_valid = true;
	}

	return rx_tmstamp_valid;
}

void sxe_hw_ptp_timestamp_mode_set(struct sxe_hw *hw,
					bool is_l2, u32 tsctl, u32 tses)
{
	u32 regval;

	if (is_l2) {
		SXE_REG_WRITE(hw, SXE_ETQF(SXE_ETQF_FILTER_1588),
			(SXE_ETQF_FILTER_EN |   
			 SXE_ETQF_1588 |	
			 ETH_P_1588));		
	} else {
		SXE_REG_WRITE(hw, SXE_ETQF(SXE_ETQF_FILTER_1588), 0);
	}

	if (tsctl) {
		regval = SXE_REG_READ(hw, SXE_TSCTRL);
		regval |= tsctl;
		SXE_REG_WRITE(hw, SXE_TSCTRL, regval);
	}

	SXE_REG_WRITE(hw, SXE_TSES, tses);

	SXE_WRITE_FLUSH(hw);

	return;
}

void sxe_hw_ptp_timestamp_enable(struct sxe_hw *hw)
{
	SXE_REG_WRITE(hw, SXE_TSYNCTXCTL,
			(SXE_REG_READ(hw, SXE_TSYNCTXCTL) |
			SXE_TSYNCTXCTL_TEN));

	SXE_REG_WRITE(hw, SXE_TSYNCRXCTL,
			(SXE_REG_READ(hw, SXE_TSYNCRXCTL) |
			SXE_TSYNCRXCTL_REN));
	SXE_WRITE_FLUSH(hw);

	return;
}

static void sxe_hw_dcb_tc_rss_configure(struct sxe_hw *hw, u16 rss)
{
	u32 msb = 0;

	while (rss) {
		msb++;
		rss >>= 1;
	}

	SXE_REG_WRITE(hw, SXE_RQTC, msb * SXE_8_TC_MSB);
}

static void sxe_hw_tx_ring_disable(struct sxe_hw *hw, u8 reg_idx,
				 unsigned long timeout)
{
	unsigned long wait_delay, delay_interval;
	int wait_loop;
	u32 txdctl;
	struct sxe_adapter *adapter = hw->adapter;

	txdctl = SXE_REG_READ(hw, SXE_TXDCTL(reg_idx));
	txdctl &= ~SXE_TXDCTL_ENABLE;
	SXE_REG_WRITE(hw, SXE_TXDCTL(reg_idx), txdctl);

	delay_interval = timeout / 100;

	wait_loop = SXE_MAX_RX_DESC_POLL;
	wait_delay = delay_interval;

	while (wait_loop--) {
		usleep_range(wait_delay, wait_delay + 10);
		wait_delay += delay_interval * 2;
		txdctl = SXE_REG_READ(hw, SXE_TXDCTL(reg_idx));

		if (!(txdctl & SXE_TXDCTL_ENABLE)) {
			return;
		}
	}

	LOG_MSG_ERR(drv, "register TXDCTL.ENABLE not cleared within the polling period\n");
}

static void sxe_hw_rx_ring_disable(struct sxe_hw *hw, u8 reg_idx,
				 unsigned long timeout)
{
	unsigned long wait_delay, delay_interval;
	int wait_loop;
	u32 rxdctl;
	struct sxe_adapter *adapter = hw->adapter;

	rxdctl = SXE_REG_READ(hw, SXE_RXDCTL(reg_idx));
	rxdctl &= ~SXE_RXDCTL_ENABLE;

	SXE_REG_WRITE(hw, SXE_RXDCTL(reg_idx), rxdctl);

	delay_interval = timeout / 100;

	wait_loop = SXE_MAX_RX_DESC_POLL;
	wait_delay = delay_interval;

	while (wait_loop--) {
		usleep_range(wait_delay, wait_delay + 10);
		wait_delay += delay_interval * 2;
		rxdctl = SXE_REG_READ(hw, SXE_RXDCTL(reg_idx));

		if (!(rxdctl & SXE_RXDCTL_ENABLE))
			return;
	}

	LOG_MSG_ERR(drv, "register RXDCTL.ENABLE not cleared within the polling period\n");
}

static u32 sxe_hw_tx_dbu_fc_status_get(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_TXPBFCS);
}

static void sxe_hw_fnav_sample_hash_set(struct sxe_hw *hw, u64 hash)
{
	SXE_REG64_WRITE(hw, SXE_FNAVHASH, hash);
	return;
}

static const struct sxe_dbu_operations sxe_dbu_ops = {
	.rx_pkt_buf_size_configure	= sxe_hw_rx_pkt_buf_size_configure,
	.rx_pkt_buf_switch		= sxe_hw_rx_pkt_buf_switch,
	.rx_multi_ring_configure	= sxe_hw_rx_multi_ring_configure,
	.rss_key_set_all		= sxe_hw_rss_key_set_all,
	.rss_redir_tbl_set_all		= sxe_hw_rss_redir_tbl_set_all,
	.rx_cap_switch_on		= sxe_hw_rx_cap_switch_on,
	.rx_cap_switch_off		= sxe_hw_rx_cap_switch_off,
	.rss_hash_pkt_type_set		= sxe_hw_rss_hash_pkt_type_set,
	.rss_hash_pkt_type_update	= sxe_hw_rss_hash_pkt_type_update,
	.rss_rings_used_set		= sxe_hw_rss_rings_used_set,
	.lro_ack_switch			= sxe_hw_rx_lro_ack_switch,

	.fnav_mode_init			= sxe_hw_fnav_mode_init,
	.fnav_specific_rule_mask_set	= sxe_hw_fnav_specific_rule_mask_set,
	.fnav_specific_rule_add		= sxe_hw_fnav_specific_rule_add,
	.fnav_specific_rule_del		= sxe_hw_fnav_specific_rule_del,
	.fnav_sample_hash_cmd_get	= sxe_hw_fnav_sample_hash_cmd_get,
	.fnav_sample_stats_reinit	= sxe_hw_fnav_sample_stats_reinit,
	.fnav_sample_hash_set		= sxe_hw_fnav_sample_hash_set,
	.fnav_single_sample_rule_del	= sxe_hw_fnav_single_sample_rule_del,

	.tx_pkt_buf_switch		= sxe_hw_tx_pkt_buf_switch,
	.tx_pkt_buf_size_configure	= sxe_hw_tx_pkt_buf_size_configure,

	.ptp_init			= sxe_hw_ptp_init,
	.ptp_freq_adjust		= sxe_hw_ptp_freq_adjust,
	.ptp_systime_init		= sxe_hw_ptp_systime_init,
	.ptp_systime_get		= sxe_hw_ptp_systime_get,
	.ptp_tx_timestamp_get		= sxe_hw_ptp_tx_timestamp_get,
	.ptp_timestamp_mode_set		= sxe_hw_ptp_timestamp_mode_set,
	.ptp_timestamp_enable		= sxe_hw_ptp_timestamp_enable,
	.ptp_rx_timestamp_clear		= sxe_hw_ptp_rx_timestamp_clear,
	.ptp_rx_timestamp_get		= sxe_hw_ptp_rx_timestamp_get,
	.ptp_is_rx_timestamp_valid	= sxe_hw_ptp_is_rx_timestamp_valid,

	.dcb_tc_rss_configure		= sxe_hw_dcb_tc_rss_configure,
	.vf_rx_switch			= sxe_hw_vf_rx_switch,
	.rx_pkt_buf_size_get		= sxe_hw_rx_pkt_buf_size_get,
	.rx_func_switch_on		= sxe_hw_rx_func_switch_on,

	.tx_ring_disable		= sxe_hw_tx_ring_disable,
	.rx_ring_disable		= sxe_hw_rx_ring_disable,

	.tx_dbu_fc_status_get		= sxe_hw_tx_dbu_fc_status_get,
};


void sxe_hw_rx_dma_ctrl_init(struct sxe_hw *hw)
{
	u32 rx_dma_ctrl = SXE_REG_READ(hw, SXE_RDRXCTL);

	rx_dma_ctrl &= ~SXE_RDRXCTL_LROFRSTSIZE;
	SXE_REG_WRITE(hw, SXE_RDRXCTL, rx_dma_ctrl);
	return;
}

void sxe_hw_rx_dma_lro_ctrl_set(struct sxe_hw *hw)
{
	u32 rx_dma_ctrl = SXE_REG_READ(hw, SXE_RDRXCTL);

	rx_dma_ctrl |= SXE_RDRXCTL_LROACKC;
	SXE_REG_WRITE(hw, SXE_RDRXCTL, rx_dma_ctrl);
	return;
}

void sxe_hw_rx_desc_thresh_set(struct sxe_hw *hw, u8 reg_idx)
{
	u32 rxdctl;
	rxdctl = SXE_REG_READ(hw, SXE_RXDCTL(reg_idx));
	rxdctl |= 0x40 << SXE_RXDCTL_PREFETCH_NUM_CFG_SHIFT;
	rxdctl |= 0x2 << SXE_RXDCTL_DESC_FIFO_AE_TH_SHIFT;
	rxdctl |= 0x10;
	SXE_REG_WRITE(hw, SXE_RXDCTL(reg_idx), rxdctl);

	return;
}

void sxe_hw_rx_ring_switch(struct sxe_hw *hw, u8 reg_idx, bool is_on)
{
	u32 rxdctl;
	u32 wait_loop = SXE_RING_WAIT_LOOP;
	struct sxe_adapter *adapter = hw->adapter;

	rxdctl = SXE_REG_READ(hw, SXE_RXDCTL(reg_idx));
	if (is_on) {
		rxdctl |= SXE_RXDCTL_ENABLE;
		SXE_REG_WRITE(hw, SXE_RXDCTL(reg_idx), rxdctl);

		do {
			usleep_range(1000, 2000);
			rxdctl = SXE_REG_READ(hw, SXE_RXDCTL(reg_idx));
		} while (--wait_loop && !(rxdctl & SXE_RXDCTL_ENABLE));
	} else {
		rxdctl &= ~SXE_RXDCTL_ENABLE;
		SXE_REG_WRITE(hw, SXE_RXDCTL(reg_idx), rxdctl);

		do {
			usleep_range(1000, 2000);
			rxdctl = SXE_REG_READ(hw, SXE_RXDCTL(reg_idx));
		} while (--wait_loop && (rxdctl & SXE_RXDCTL_ENABLE));
	}

	SXE_WRITE_FLUSH(hw);

	if (!wait_loop) {
		LOG_MSG_ERR(drv, "rx ring %u switch %u failed within "
			  "the polling period\n", reg_idx, is_on);
	}

	return;
}

void sxe_hw_rx_ring_switch_not_polling(struct sxe_hw *hw, u8 reg_idx, bool is_on)
{
	u32 rxdctl = SXE_REG_READ(hw, SXE_RXDCTL(reg_idx));
	if (is_on) {
		rxdctl |= SXE_RXDCTL_ENABLE;
		SXE_REG_WRITE(hw, SXE_RXDCTL(reg_idx), rxdctl);
	} else {
		rxdctl &= ~SXE_RXDCTL_ENABLE;
		SXE_REG_WRITE(hw, SXE_RXDCTL(reg_idx), rxdctl);
	}

	SXE_WRITE_FLUSH(hw);

	return;
}

void sxe_hw_rx_queue_desc_reg_configure(struct sxe_hw *hw,
					u8 reg_idx, u32 rdh_value,
					u32 rdt_value)
{
	SXE_REG_WRITE(hw, SXE_RDH(reg_idx), rdh_value);
	SXE_REG_WRITE(hw, SXE_RDT(reg_idx), rdt_value);
	return;
}

static void sxe_hw_rx_ring_head_init(struct sxe_hw *hw, u8 reg_idx)
{
	SXE_REG_WRITE(hw, SXE_RDH(reg_idx), 0);

	return;
}

static void sxe_hw_rx_ring_tail_init(struct sxe_hw *hw, u8 reg_idx)
{
	SXE_REG_WRITE(hw, SXE_RDT(reg_idx), 0);

	return;
}

void sxe_hw_rx_ring_desc_configure(struct sxe_hw *hw,
					u32 desc_mem_len, u64 desc_dma_addr,
					u8 reg_idx)
{
	SXE_REG_WRITE(hw, SXE_RDBAL(reg_idx),
			(desc_dma_addr & DMA_BIT_MASK(32)));
	SXE_REG_WRITE(hw, SXE_RDBAH(reg_idx), (desc_dma_addr >> 32));
	SXE_REG_WRITE(hw, SXE_RDLEN(reg_idx), desc_mem_len);

	SXE_WRITE_FLUSH(hw);

	sxe_hw_rx_ring_head_init(hw, reg_idx);
	sxe_hw_rx_ring_tail_init(hw, reg_idx);

	return;
}

void sxe_hw_rx_rcv_ctl_configure(struct sxe_hw *hw, u8 reg_idx,
				   u32 header_buf_len, u32 pkg_buf_len
				   )
{
	u32 srrctl;

	srrctl = ((header_buf_len << SXE_SRRCTL_BSIZEHDRSIZE_SHIFT) &
			SXE_SRRCTL_BSIZEHDR_MASK);
	srrctl |= ((pkg_buf_len >> SXE_SRRCTL_BSIZEPKT_SHIFT) &
			SXE_SRRCTL_BSIZEPKT_MASK);

	SXE_REG_WRITE(hw, SXE_SRRCTL(reg_idx), srrctl);

	return;
}

void sxe_hw_rx_lro_ctl_configure(struct sxe_hw *hw,
						u8 reg_idx, u32 max_desc)
{
	u32 lroctrl;
	lroctrl = SXE_REG_READ(hw, SXE_LROCTL(reg_idx));
	lroctrl |= SXE_LROCTL_LROEN;
	lroctrl |= max_desc;
	SXE_REG_WRITE(hw, SXE_LROCTL(reg_idx), lroctrl);

	return;
}

static u32 sxe_hw_rx_desc_ctrl_get(struct sxe_hw *hw, u8 reg_idx)
{
	return SXE_REG_READ(hw, SXE_RXDCTL(reg_idx));
}

static void sxe_hw_dcb_arbiter_set(struct sxe_hw *hw, bool is_enable)
{
	u32 rttdcs;

	rttdcs = SXE_REG_READ(hw, SXE_RTTDCS);

	if (true == is_enable) {
		rttdcs &= ~SXE_RTTDCS_ARBDIS;
		rttdcs &= ~SXE_RTTDCS_BPBFSM;

		SXE_REG_WRITE(hw, SXE_RTTDCS, rttdcs);
	} else {
		rttdcs |= SXE_RTTDCS_ARBDIS;
		SXE_REG_WRITE(hw, SXE_RTTDCS, rttdcs);
	}

	return;
}


static void sxe_hw_tx_multi_ring_configure(struct sxe_hw *hw, u8 tcs,
				u16 pool_mask, bool sriov_enable, u16 max_txq)
{
	u32 mtqc;

	sxe_hw_dcb_arbiter_set(hw, false);

	if (true == sriov_enable) {
		mtqc = SXE_MTQC_VT_ENA;
		if (tcs > SXE_DCB_4_TC)
			mtqc |= SXE_MTQC_RT_ENA | SXE_MTQC_8TC_8TQ;
		else if (tcs > SXE_DCB_1_TC)
			mtqc |= SXE_MTQC_RT_ENA | SXE_MTQC_4TC_4TQ;
		else if (pool_mask == SXE_4Q_PER_POOL_MASK)
			mtqc |= SXE_MTQC_32VF;
		else
			mtqc |= SXE_MTQC_64VF;
	} else {
		if (tcs > SXE_DCB_4_TC) {
			mtqc = SXE_MTQC_RT_ENA | SXE_MTQC_8TC_8TQ;
		} else if (tcs > SXE_DCB_1_TC) {
			mtqc = SXE_MTQC_RT_ENA | SXE_MTQC_4TC_4TQ;
		} else {
			if (max_txq > 63) {
				mtqc = SXE_MTQC_RT_ENA | SXE_MTQC_4TC_4TQ;
			} else {
				mtqc = SXE_MTQC_64Q_1PB;
			}
		}
	}

	SXE_REG_WRITE(hw, SXE_MTQC, mtqc);

	sxe_hw_dcb_arbiter_set(hw, true);

	return;
}

void sxe_hw_tx_ring_head_init(struct sxe_hw *hw, u8 reg_idx)
{
	SXE_REG_WRITE(hw, SXE_TDH(reg_idx), 0);

	return;
}

void sxe_hw_tx_ring_tail_init(struct sxe_hw *hw, u8 reg_idx)
{
	SXE_REG_WRITE(hw, SXE_TDT(reg_idx), 0);

	return;
}

void sxe_hw_tx_ring_desc_configure(struct sxe_hw *hw,
					u32 desc_mem_len,
					u64 desc_dma_addr, u8 reg_idx)
{
	SXE_REG_WRITE(hw, SXE_TXDCTL(reg_idx), 0);

	SXE_WRITE_FLUSH(hw);

	SXE_REG_WRITE(hw, SXE_TDBAL(reg_idx), (desc_dma_addr & \
						DMA_BIT_MASK(32)));
	SXE_REG_WRITE(hw, SXE_TDBAH(reg_idx), (desc_dma_addr >> 32));
	SXE_REG_WRITE(hw, SXE_TDLEN(reg_idx), desc_mem_len);
	sxe_hw_tx_ring_head_init(hw, reg_idx);
	sxe_hw_tx_ring_tail_init(hw, reg_idx);

	return;
}

void sxe_hw_tx_desc_thresh_set(
				struct sxe_hw *hw,
				u8 reg_idx,
				u32 wb_thresh,
				u32 host_thresh,
				u32 prefech_thresh)
{
	u32 txdctl = 0;

	txdctl |= (wb_thresh << SXE_TXDCTL_WTHRESH_SHIFT);
	txdctl |= (host_thresh << SXE_TXDCTL_HTHRESH_SHIFT) | prefech_thresh;

	SXE_REG_WRITE(hw, SXE_TXDCTL(reg_idx), txdctl);

	return;
}

void sxe_hw_all_ring_disable(struct sxe_hw *hw, u32 ring_max)
{
	u32 i, value;

	for (i = 0; i < ring_max; i++) {
		value = SXE_REG_READ(hw, SXE_TXDCTL(i));
		value &= ~SXE_TXDCTL_ENABLE;
		SXE_REG_WRITE(hw, SXE_TXDCTL(i), value);

		value = SXE_REG_READ(hw, SXE_RXDCTL(i));
		value &= ~SXE_RXDCTL_ENABLE;
		SXE_REG_WRITE(hw, SXE_RXDCTL(i), value);
	}

	SXE_WRITE_FLUSH(hw);
	usleep_range(1000, 2000);

	return;
}

void sxe_hw_tx_ring_switch(struct sxe_hw *hw, u8 reg_idx, bool is_on)
{
	u32 wait_loop = SXE_RING_WAIT_LOOP;
	struct sxe_adapter *adapter = hw->adapter;

	u32 txdctl = SXE_REG_READ(hw, SXE_TXDCTL(reg_idx));
	if (is_on) {
		txdctl |= SXE_TXDCTL_ENABLE;
		SXE_REG_WRITE(hw, SXE_TXDCTL(reg_idx), txdctl);

		do {
			usleep_range(1000, 2000);
			txdctl = SXE_REG_READ(hw, SXE_TXDCTL(reg_idx));
		} while (--wait_loop && !(txdctl & SXE_TXDCTL_ENABLE));
	} else {
		txdctl &= ~SXE_TXDCTL_ENABLE;
		SXE_REG_WRITE(hw, SXE_TXDCTL(reg_idx), txdctl);

		do {
			usleep_range(1000, 2000);
			txdctl = SXE_REG_READ(hw, SXE_TXDCTL(reg_idx));
		} while (--wait_loop && (txdctl & SXE_TXDCTL_ENABLE));
	}

	if (!wait_loop) {
		LOG_DEV_ERR("tx ring %u switch %u failed within "
			  "the polling period\n", reg_idx, is_on);
	}

	return;
}

void sxe_hw_tx_ring_switch_not_polling(struct sxe_hw *hw, u8 reg_idx, bool is_on)
{
	u32 txdctl = SXE_REG_READ(hw, SXE_TXDCTL(reg_idx));
	if (is_on) {
		txdctl |= SXE_TXDCTL_ENABLE;
		SXE_REG_WRITE(hw, SXE_TXDCTL(reg_idx), txdctl);
	} else {
		txdctl &= ~SXE_TXDCTL_ENABLE;
		SXE_REG_WRITE(hw, SXE_TXDCTL(reg_idx), txdctl);
	}

	return;
}

void sxe_hw_tx_pkt_buf_thresh_configure(struct sxe_hw *hw,
					u8 num_pb, bool dcb_enable)
{
	u32 i, tx_pkt_size, tx_pb_thresh;

	if (!num_pb){
		num_pb = 1;
	}

	tx_pkt_size = SXE_TX_PBSIZE_MAX / num_pb;
	if (true == dcb_enable) {
		tx_pb_thresh = (tx_pkt_size / 1024) - SXE_TX_PKT_SIZE_MAX;
	} else {
		tx_pb_thresh = (tx_pkt_size / 1024) - SXE_NODCB_TX_PKT_SIZE_MAX;
	}

	for (i = 0; i < num_pb; i++) {
		SXE_REG_WRITE(hw, SXE_TXPBTHRESH(i), tx_pb_thresh);
	}

	for (; i < SXE_PKG_BUF_NUM_MAX; i++) {
		SXE_REG_WRITE(hw, SXE_TXPBTHRESH(i), 0);
	}

	return;
}

void sxe_hw_tx_enable(struct sxe_hw *hw)
{
	u32 ctl;

	ctl = SXE_REG_READ(hw, SXE_DMATXCTL);
	ctl |= SXE_DMATXCTL_TE;
	SXE_REG_WRITE(hw, SXE_DMATXCTL, ctl);

	return;
}

static u32 sxe_hw_tx_desc_ctrl_get(struct sxe_hw *hw, u8 reg_idx)
{
	return SXE_REG_READ(hw, SXE_TXDCTL(reg_idx));
}

static void sxe_hw_tx_desc_wb_thresh_clear(struct sxe_hw *hw, u8 reg_idx)
{
	u32 reg_data;

	reg_data = SXE_REG_READ(hw, SXE_TXDCTL(reg_idx));
	reg_data &= ~SXE_TXDCTL_ENABLE;
	SXE_REG_WRITE(hw, SXE_TXDCTL(reg_idx), reg_data);
	SXE_WRITE_FLUSH(hw);
	reg_data &= ~(0x7f<<16);
	reg_data |= SXE_TXDCTL_ENABLE;
	SXE_REG_WRITE(hw, SXE_TXDCTL(reg_idx), reg_data);

	return;
}

void sxe_hw_vlan_tag_strip_switch(struct sxe_hw *hw,
					u16 reg_index, bool is_enable)
{
	u32 rxdctl;

	rxdctl = SXE_REG_READ(hw, SXE_RXDCTL(reg_index));

	if (is_enable) {
		rxdctl |= SXE_RXDCTL_VME;
	} else {
		rxdctl &= ~SXE_RXDCTL_VME;
	}

	SXE_REG_WRITE(hw, SXE_RXDCTL(reg_index), rxdctl);

	return;
}

static void sxe_hw_tx_vlan_tag_set(struct sxe_hw *hw,
				   u16 vid, u16 qos, u32 vf)
{
	u32 vmvir = vid | (qos << VLAN_PRIO_SHIFT) | SXE_VMVIR_VLANA_DEFAULT;

	SXE_REG_WRITE(hw, SXE_VMVIR(vf), vmvir);
	return;
}

void sxe_hw_tx_vlan_tag_clear(struct sxe_hw *hw, u32 vf)
{
	SXE_REG_WRITE(hw, SXE_VMVIR(vf), 0);
	return;
}

u32 sxe_hw_tx_vlan_insert_get(struct sxe_hw *hw, u32 vf)
{
	return SXE_REG_READ(hw, SXE_VMVIR(vf));
}

void sxe_hw_tx_ring_info_get(struct sxe_hw *hw,
				u8 idx, u32 *head, u32 *tail)
{
	*head = SXE_REG_READ(hw, SXE_TDH(idx));
	*tail = SXE_REG_READ(hw, SXE_TDT(idx));

	return;
}

void sxe_hw_dcb_rx_bw_alloc_configure(struct sxe_hw *hw,
				      u16 *refill,
				      u16 *max,
				      u8 *bwg_id,
				      u8 *prio_type,
				      u8 *prio_tc,
				      u8 max_priority)
{
	u32    reg;
	u32    credit_refill;
	u32    credit_max;
	u8     i;

	reg = SXE_RTRPCS_RRM | SXE_RTRPCS_RAC | SXE_RTRPCS_ARBDIS;
	SXE_REG_WRITE(hw, SXE_RTRPCS, reg);

	reg = 0;
	for (i = 0; i < max_priority; i++) {
		reg |= (prio_tc[i] << (i * SXE_RTRUP2TC_UP_SHIFT));
	}

	SXE_REG_WRITE(hw, SXE_RTRUP2TC, reg);

	for (i = 0; i < MAX_TRAFFIC_CLASS; i++) {
		credit_refill = refill[i];
		credit_max    = max[i];
		reg = credit_refill | (credit_max << SXE_RTRPT4C_MCL_SHIFT);

		reg |= (u32)(bwg_id[i]) << SXE_RTRPT4C_BWG_SHIFT;

		if (prio_type[i] == PRIO_LINK) {
			reg |= SXE_RTRPT4C_LSP;
		}

		SXE_REG_WRITE(hw, SXE_RTRPT4C(i), reg);
	}

	reg = SXE_RTRPCS_RRM | SXE_RTRPCS_RAC;
	SXE_REG_WRITE(hw, SXE_RTRPCS, reg);

	return;
}

void sxe_hw_dcb_tx_desc_bw_alloc_configure(struct sxe_hw *hw,
					   u16 *refill,
					   u16 *max,
					   u8 *bwg_id,
					   u8 *prio_type)
{
	u32    reg, max_credits;
	u8     i;

	for (i = 0; i < 128; i++) {
		SXE_REG_WRITE(hw, SXE_RTTDQSEL, i);
		SXE_REG_WRITE(hw, SXE_RTTDT1C, 0);
	}

	for (i = 0; i < MAX_TRAFFIC_CLASS; i++) {
		max_credits = max[i];
		reg = max_credits << SXE_RTTDT2C_MCL_SHIFT;
		reg |= refill[i];
		reg |= (u32)(bwg_id[i]) << SXE_RTTDT2C_BWG_SHIFT;

		if (prio_type[i] == PRIO_GROUP) {
			reg |= SXE_RTTDT2C_GSP;
		}

		if (prio_type[i] == PRIO_LINK) {
			reg |= SXE_RTTDT2C_LSP;
		}

		SXE_REG_WRITE(hw, SXE_RTTDT2C(i), reg);
	}

	reg = SXE_RTTDCS_TDPAC | SXE_RTTDCS_TDRM;
	SXE_REG_WRITE(hw, SXE_RTTDCS, reg);

	return;
}

void sxe_hw_dcb_tx_data_bw_alloc_configure(struct sxe_hw *hw,
					   u16 *refill,
					   u16 *max,
					   u8 *bwg_id,
					   u8 *prio_type,
					   u8 *prio_tc,
					   u8 max_priority)
{
	u32 reg;
	u8 i;

	reg = SXE_RTTPCS_TPPAC | SXE_RTTPCS_TPRM |
	      (SXE_RTTPCS_ARBD_DCB << SXE_RTTPCS_ARBD_SHIFT) |
	      SXE_RTTPCS_ARBDIS;
	SXE_REG_WRITE(hw, SXE_RTTPCS, reg);

	reg = 0;
	for (i = 0; i < max_priority; i++) {
		reg |= (prio_tc[i] << (i * SXE_RTTUP2TC_UP_SHIFT));
	}

	SXE_REG_WRITE(hw, SXE_RTTUP2TC, reg);

	for (i = 0; i < MAX_TRAFFIC_CLASS; i++) {
		reg = refill[i];
		reg |= (u32)(max[i]) << SXE_RTTPT2C_MCL_SHIFT;
		reg |= (u32)(bwg_id[i]) << SXE_RTTPT2C_BWG_SHIFT;

		if (prio_type[i] == PRIO_GROUP) {
			reg |= SXE_RTTPT2C_GSP;
		}

		if (prio_type[i] == PRIO_LINK) {
			reg |= SXE_RTTPT2C_LSP;
		}

		SXE_REG_WRITE(hw, SXE_RTTPT2C(i), reg);
	}

	reg = SXE_RTTPCS_TPPAC | SXE_RTTPCS_TPRM |
	      (SXE_RTTPCS_ARBD_DCB << SXE_RTTPCS_ARBD_SHIFT);
	SXE_REG_WRITE(hw, SXE_RTTPCS, reg);

	return;
}

void sxe_hw_dcb_pfc_configure(struct sxe_hw *hw,
						u8 pfc_en, u8 *prio_tc,
						u8 max_priority)
{
	u32 i, j, fcrtl, reg;
	u8 max_tc = 0;
	u32 reg_val;

	reg_val = SXE_REG_READ(hw, SXE_FLCTRL);

	reg_val &= ~SXE_FCTRL_TFCE_MASK;
	reg_val |= SXE_FCTRL_TFCE_PFC_EN;

	reg_val |= SXE_FCTRL_TFCE_DPF_EN;

	reg_val &= ~(SXE_FCTRL_TFCE_FCEN_MASK | SXE_FCTRL_TFCE_XONE_MASK);
	reg_val |= (pfc_en << 16) & SXE_FCTRL_TFCE_FCEN_MASK;
	reg_val |= (pfc_en << 24) & SXE_FCTRL_TFCE_XONE_MASK;

	reg_val &= ~SXE_FCTRL_RFCE_MASK;
	reg_val |= SXE_FCTRL_RFCE_PFC_EN;
	SXE_REG_WRITE(hw, SXE_FLCTRL, reg_val);

	reg_val = SXE_REG_READ(hw, SXE_PFCTOP);
	reg_val &= ~SXE_PFCTOP_FCOP_MASK;
	reg_val |= SXE_PFCTOP_FCT;
	reg_val |= SXE_PFCTOP_FCOP_PFC;
	SXE_REG_WRITE(hw, SXE_PFCTOP, reg_val);

	for (i = 0; i < max_priority; i++) {
		if (prio_tc[i] > max_tc) {
			max_tc = prio_tc[i];
		}
	}

	for (i = 0; i <= max_tc; i++) {
		int enabled = 0;

		for (j = 0; j < max_priority; j++) {
			if ((prio_tc[j] == i) && (pfc_en & BIT(j))) {
				enabled = 1;
				break;
			}
		}

		if (enabled) {
			reg = (hw->fc.high_water[i] << 9) | SXE_FCRTH_FCEN;
			fcrtl = (hw->fc.low_water[i] << 9) | SXE_FCRTL_XONE;
			SXE_REG_WRITE(hw, SXE_FCRTL(i), fcrtl);
		} else {

			reg = (SXE_REG_READ(hw, SXE_RXPBSIZE(i)) - 24576) >> 1;
			SXE_REG_WRITE(hw, SXE_FCRTL(i), 0);
		}

		SXE_REG_WRITE(hw, SXE_FCRTH(i), reg);
	}

	for (; i < MAX_TRAFFIC_CLASS; i++) {
		SXE_REG_WRITE(hw, SXE_FCRTL(i), 0);
		SXE_REG_WRITE(hw, SXE_FCRTH(i), 0);
	}

	reg = hw->fc.pause_time * 0x00010001;
	for (i = 0; i < (MAX_TRAFFIC_CLASS / 2); i++) {
		SXE_REG_WRITE(hw, SXE_FCTTV(i), reg);
	}

	SXE_REG_WRITE(hw, SXE_FCRTV, hw->fc.pause_time / 2);

	return;
}

static void sxe_hw_dcb_8tc_vmdq_off_stats_configure(struct sxe_hw *hw)
{
	u32 reg;
	u8  i;

	for (i = 0; i < 32; i++) {
		reg = 0x01010101 * (i / 4);
		SXE_REG_WRITE(hw, SXE_RQSMR(i), reg);
	}

	for (i = 0; i < 32; i++) {
		if (i < 8) {
			reg = 0x00000000;
		} else if (i < 16) {
			reg = 0x01010101;
		} else if (i < 20) {
			reg = 0x02020202;
		} else if (i < 24) {
			reg = 0x03030303;
		} else if (i < 26) {
			reg = 0x04040404;
		} else if (i < 28) {
			reg = 0x05050505;
		} else if (i < 30) {
			reg = 0x06060606;
		} else {
			reg = 0x07070707;
		}

		SXE_REG_WRITE(hw, SXE_TQSM(i), reg);
	}

	return;
}

static void sxe_hw_dcb_rx_up_tc_map_set(struct sxe_hw *hw, u8 tc)
{
	u8 i;
	u32 reg, rsave;

	reg = SXE_REG_READ(hw, SXE_RTRUP2TC);
	rsave = reg;

	for (i = 0; i < MAX_TRAFFIC_CLASS; i++) {
		u8 up2tc = reg >> (i * SXE_RTRUP2TC_UP_SHIFT);

		if (up2tc > tc) {
			reg &= ~(0x7 << SXE_RTRUP2TC_UP_MASK);
		}
	}

	if (reg != rsave) {
		SXE_REG_WRITE(hw, SXE_RTRUP2TC, reg);
	}

	return;
}

void sxe_hw_vt_pool_loopback_switch(struct sxe_hw *hw,
							bool is_enable)
{
	if (true == is_enable) {
		SXE_REG_WRITE(hw, SXE_PFDTXGSWC, SXE_PFDTXGSWC_VT_LBEN);
	} else {
		SXE_REG_WRITE(hw, SXE_PFDTXGSWC, 0);
	}

	return;
}

void sxe_hw_pool_rx_ring_drop_enable(struct sxe_hw *hw, u8 vf_idx,
					u16 pf_vlan, u8 ring_per_pool)
{
	u32 qde = SXE_QDE_ENABLE;
	u8 i;

	if (pf_vlan) {
		qde |= SXE_QDE_HIDE_VLAN;
	}

	for (i = (vf_idx * ring_per_pool); i < ((vf_idx + 1) * ring_per_pool); i++)
	{
		u32 value;

		SXE_WRITE_FLUSH(hw);

		value = i << SXE_QDE_IDX_SHIFT;
		value |= qde | SXE_QDE_WRITE;

		SXE_REG_WRITE(hw, SXE_QDE, value);
	}

	return;
}

u32 sxe_hw_rx_pool_bitmap_get(struct sxe_hw *hw, u8 reg_idx)
{
	return SXE_REG_READ(hw, SXE_VFRE(reg_idx));
}

void sxe_hw_rx_pool_bitmap_set(struct sxe_hw *hw,
						u8 reg_idx, u32 bitmap)
{
	SXE_REG_WRITE(hw, SXE_VFRE(reg_idx), bitmap);

	return;
}

u32 sxe_hw_tx_pool_bitmap_get(struct sxe_hw *hw, u8 reg_idx)
{
	return SXE_REG_READ(hw, SXE_VFTE(reg_idx));
}

void sxe_hw_tx_pool_bitmap_set(struct sxe_hw *hw,
						u8 reg_idx, u32 bitmap)
{
	SXE_REG_WRITE(hw, SXE_VFTE(reg_idx), bitmap);

	return;
}

void sxe_hw_dcb_max_mem_window_set(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_RTTBCNRM, value);

	return;
}

void sxe_hw_dcb_tx_ring_rate_factor_set(struct sxe_hw *hw,
							u32 ring_idx, u32 rate)
{
	SXE_REG_WRITE(hw, SXE_RTTDQSEL, ring_idx);
	SXE_REG_WRITE(hw, SXE_RTTBCNRC, rate);

	return;
}

void sxe_hw_spoof_count_enable(struct sxe_hw *hw,
						u8 reg_idx, u8 bit_index)
{
	u32 value = SXE_REG_READ(hw, SXE_VMECM(reg_idx));

	value |= BIT(bit_index);

	SXE_REG_WRITE(hw, SXE_VMECM(reg_idx), value);

	return;
}

void sxe_hw_pool_mac_anti_spoof_set(struct sxe_hw *hw,
							u8 vf_idx, bool status)
{
	u8 reg_index = vf_idx >> 3;
	u8 bit_index = vf_idx % 8;
	u32 value;

	value = SXE_REG_READ(hw, SXE_SPOOF(reg_index));

	if (status) {
		value |= BIT(bit_index);
	} else {
		value &= ~BIT(bit_index);
	}

	SXE_REG_WRITE(hw, SXE_SPOOF(reg_index), value);

	return;
}

static void sxe_hw_dcb_rx_up_tc_map_get(struct sxe_hw *hw, u8 *map)
{
	u32 reg, i;

	reg = SXE_REG_READ(hw, SXE_RTRUP2TC);
	for (i = 0; i < MAX_TRAFFIC_CLASS; i++) {
		map[i] = SXE_RTRUP2TC_UP_MASK &
			(reg >> (i * SXE_RTRUP2TC_UP_SHIFT));
	}

	return;
}

void sxe_hw_rx_drop_switch(struct sxe_hw *hw, u8 idx, bool is_enable)
{
	u32 srrctl = SXE_REG_READ(hw, SXE_SRRCTL(idx));

	if (true == is_enable) {
		srrctl |= SXE_SRRCTL_DROP_EN;
	} else {
		srrctl &= ~SXE_SRRCTL_DROP_EN;
	}

	SXE_REG_WRITE(hw, SXE_SRRCTL(idx), srrctl);

	return;
}

static void sxe_hw_pool_vlan_anti_spoof_set(struct sxe_hw *hw,
							u8 vf_idx, bool status)
{
	u8 reg_index = vf_idx >> 3;
	u8 bit_index = (vf_idx % 8) + SXE_SPOOF_VLAN_SHIFT;
	u32 value;

	value = SXE_REG_READ(hw, SXE_SPOOF(reg_index));

	if (status) {
		value |= BIT(bit_index);
	} else {
		value &= ~BIT(bit_index);
	}

	SXE_REG_WRITE(hw, SXE_SPOOF(reg_index), value);

	return;
}

static void sxe_hw_vf_tx_desc_addr_clear(struct sxe_hw *hw,
						u8 vf_idx, u8 ring_per_pool)
{
	u8 i;

	for (i = 0; i < ring_per_pool; i++) {
		SXE_REG_WRITE(hw, SXE_PVFTDWBAL_N(ring_per_pool, vf_idx, i), 0);
		SXE_REG_WRITE(hw, SXE_PVFTDWBAH_N(ring_per_pool, vf_idx, i), 0);
	}

	return;
}

static void sxe_hw_vf_tx_ring_disable(struct sxe_hw *hw,
						u8 ring_per_pool, u8 vf_idx)
{
	u32 ring_idx;
	u32 reg;

	for (ring_idx = 0; ring_idx < ring_per_pool; ring_idx++) {
		u32 reg_idx = vf_idx * ring_per_pool + ring_idx;
		reg = SXE_REG_READ(hw, SXE_TXDCTL(reg_idx));
		if (reg) {
			reg |= SXE_TXDCTL_ENABLE;
			SXE_REG_WRITE(hw, SXE_TXDCTL(reg_idx), reg);
			reg &= ~SXE_TXDCTL_ENABLE;
			SXE_REG_WRITE(hw, SXE_TXDCTL(reg_idx), reg);
		}
	}

	SXE_WRITE_FLUSH(hw);

	return;
}

void sxe_hw_dcb_rate_limiter_clear(struct sxe_hw *hw, u8 ring_max)
{
	u32 i;

	for (i = 0; i < ring_max; i++) {
		SXE_REG_WRITE(hw, SXE_RTTDQSEL, i);
		SXE_REG_WRITE(hw, SXE_RTTBCNRC, 0);
	}
	SXE_WRITE_FLUSH(hw);

	return;
}

static void sxe_hw_tx_tph_update(struct sxe_hw *hw, u8 ring_idx, u8 cpu)
{
	u32 value = cpu;

	value <<= SXE_TPH_TXCTRL_CPUID_SHIFT;

	value |= SXE_TPH_TXCTRL_DESC_RRO_EN | \
		 SXE_TPH_TXCTRL_DATA_RRO_EN | \
		 SXE_TPH_TXCTRL_DESC_TPH_EN;

	SXE_REG_WRITE(hw, SXE_TPH_TXCTRL(ring_idx), value);
	return;
}

static void sxe_hw_rx_tph_update(struct sxe_hw *hw, u8 ring_idx, u8 cpu)
{
	u32 value = cpu;

	value <<= SXE_TPH_RXCTRL_CPUID_SHIFT;

	value |= SXE_TPH_RXCTRL_DESC_RRO_EN | \
		 SXE_TPH_RXCTRL_DATA_TPH_EN | \
		 SXE_TPH_RXCTRL_DESC_TPH_EN;

	SXE_REG_WRITE(hw, SXE_TPH_RXCTRL(ring_idx), value);
	return;
}

static void sxe_hw_tph_switch(struct sxe_hw *hw, bool is_enable)
{
	if (is_enable == true) {
		SXE_REG_WRITE(hw, SXE_TPH_CTRL, SXE_TPH_CTRL_MODE_CB2);
	} else {
		SXE_REG_WRITE(hw, SXE_TPH_CTRL, SXE_TPH_CTRL_DISABLE);
	}

	return;
}

static const struct sxe_dma_operations sxe_dma_ops = {
	.rx_dma_ctrl_init		= sxe_hw_rx_dma_ctrl_init,
	.rx_ring_switch			= sxe_hw_rx_ring_switch,
	.rx_ring_switch_not_polling	= sxe_hw_rx_ring_switch_not_polling,
	.rx_ring_desc_configure		= sxe_hw_rx_ring_desc_configure,
	.rx_desc_thresh_set		= sxe_hw_rx_desc_thresh_set,
	.rx_rcv_ctl_configure		= sxe_hw_rx_rcv_ctl_configure,
	.rx_lro_ctl_configure		= sxe_hw_rx_lro_ctl_configure,
	.rx_desc_ctrl_get		= sxe_hw_rx_desc_ctrl_get,
	.rx_dma_lro_ctl_set		= sxe_hw_rx_dma_lro_ctrl_set,
	.rx_drop_switch			= sxe_hw_rx_drop_switch,
	.pool_rx_ring_drop_enable	= sxe_hw_pool_rx_ring_drop_enable,
	.rx_tph_update			= sxe_hw_rx_tph_update,

	.tx_enable			= sxe_hw_tx_enable,
	.tx_multi_ring_configure	= sxe_hw_tx_multi_ring_configure,
	.tx_ring_desc_configure		= sxe_hw_tx_ring_desc_configure,
	.tx_desc_thresh_set		= sxe_hw_tx_desc_thresh_set,
	.tx_desc_wb_thresh_clear	= sxe_hw_tx_desc_wb_thresh_clear,
	.tx_ring_switch			= sxe_hw_tx_ring_switch,
	.tx_ring_switch_not_polling     = sxe_hw_tx_ring_switch_not_polling,
	.tx_pkt_buf_thresh_configure	= sxe_hw_tx_pkt_buf_thresh_configure,
	.tx_desc_ctrl_get		= sxe_hw_tx_desc_ctrl_get,
	.tx_ring_info_get		= sxe_hw_tx_ring_info_get,
	.tx_tph_update			= sxe_hw_tx_tph_update,

	.tph_switch			= sxe_hw_tph_switch,

	.vlan_tag_strip_switch		= sxe_hw_vlan_tag_strip_switch,
	.tx_vlan_tag_set		= sxe_hw_tx_vlan_tag_set,
	.tx_vlan_tag_clear		= sxe_hw_tx_vlan_tag_clear,

	.dcb_rx_bw_alloc_configure	= sxe_hw_dcb_rx_bw_alloc_configure,
	.dcb_tx_desc_bw_alloc_configure	= sxe_hw_dcb_tx_desc_bw_alloc_configure,
	.dcb_tx_data_bw_alloc_configure	= sxe_hw_dcb_tx_data_bw_alloc_configure,
	.dcb_pfc_configure		= sxe_hw_dcb_pfc_configure,
	.dcb_tc_stats_configure		= sxe_hw_dcb_8tc_vmdq_off_stats_configure,
	.dcb_rx_up_tc_map_set		= sxe_hw_dcb_rx_up_tc_map_set,
	.dcb_rx_up_tc_map_get		= sxe_hw_dcb_rx_up_tc_map_get,
	.dcb_rate_limiter_clear		= sxe_hw_dcb_rate_limiter_clear,
	.dcb_tx_ring_rate_factor_set	= sxe_hw_dcb_tx_ring_rate_factor_set,

	.vt_pool_loopback_switch	= sxe_hw_vt_pool_loopback_switch,
	.rx_pool_get			= sxe_hw_rx_pool_bitmap_get,
	.rx_pool_set			= sxe_hw_rx_pool_bitmap_set,
	.tx_pool_get			= sxe_hw_tx_pool_bitmap_get,
	.tx_pool_set			= sxe_hw_tx_pool_bitmap_set,

	.vf_tx_desc_addr_clear		= sxe_hw_vf_tx_desc_addr_clear,
	.pool_mac_anti_spoof_set	= sxe_hw_pool_mac_anti_spoof_set,
	.pool_vlan_anti_spoof_set	= sxe_hw_pool_vlan_anti_spoof_set,

	.max_dcb_memory_window_set	= sxe_hw_dcb_max_mem_window_set,
	.spoof_count_enable		= sxe_hw_spoof_count_enable,

	.vf_tx_ring_disable	        = sxe_hw_vf_tx_ring_disable,
	.all_ring_disable               = sxe_hw_all_ring_disable,
	.tx_ring_tail_init 	        = sxe_hw_tx_ring_tail_init,
};


#ifdef SXE_IPSEC_CONFIGURE

static void sxe_hw_ipsec_rx_sa_load(struct sxe_hw *hw, u16 idx,
					u8 type)
{
	u32 reg = SXE_REG_READ(hw, SXE_IPSRXIDX);

	reg &= SXE_RXTXIDX_IPS_EN;
	reg |= type << SXE_RXIDX_TBL_SHIFT |
	       idx << SXE_RXTXIDX_IDX_SHIFT |
	       SXE_RXTXIDX_WRITE;
	SXE_REG_WRITE(hw, SXE_IPSRXIDX, reg);
	SXE_WRITE_FLUSH(hw);

	return;
}

static void sxe_hw_ipsec_rx_ip_store(struct sxe_hw *hw,
					     __be32 *ip_addr, u8 ip_len, u8 ip_idx)
{
	u8 i;

	for (i = 0; i < ip_len; i++) {
		SXE_REG_WRITE(hw, SXE_IPSRXIPADDR(i),
				(__force u32)cpu_to_le32((__force u32)ip_addr[i]));
	}
	SXE_WRITE_FLUSH(hw);
	sxe_hw_ipsec_rx_sa_load(hw, ip_idx, SXE_IPSEC_IP_TABLE);

	return;
}

static void sxe_hw_ipsec_rx_spi_store(struct sxe_hw *hw,
					     __be32 spi, u8 ip_idx, u16 sa_idx)
{
	SXE_REG_WRITE(hw, SXE_IPSRXSPI, (__force u32)cpu_to_le32((__force u32)spi));

	SXE_REG_WRITE(hw, SXE_IPSRXIPIDX, ip_idx);

	SXE_WRITE_FLUSH(hw);

	sxe_hw_ipsec_rx_sa_load(hw, sa_idx, SXE_IPSEC_SPI_TABLE);

	return;
}

static void sxe_hw_ipsec_rx_key_store(struct sxe_hw *hw,
			u32 *key,  u8 key_len, u32 salt, u32 mode, u16 sa_idx)
{
	u8 i;

	for (i = 0; i < key_len; i++) {
		SXE_REG_WRITE(hw, SXE_IPSRXKEY(i),
				(__force u32)cpu_to_be32(key[(key_len - 1) - i]));
	}

	SXE_REG_WRITE(hw, SXE_IPSRXSALT, (__force u32)cpu_to_be32(salt));
	SXE_REG_WRITE(hw, SXE_IPSRXMOD, mode);
	SXE_WRITE_FLUSH(hw);

	sxe_hw_ipsec_rx_sa_load(hw, sa_idx, SXE_IPSEC_KEY_TABLE);

	return;
}

static void sxe_hw_ipsec_tx_sa_load(struct sxe_hw *hw, u16 idx)
{
	u32 reg = SXE_REG_READ(hw, SXE_IPSTXIDX);

	reg &= SXE_RXTXIDX_IPS_EN;
	reg |= idx << SXE_RXTXIDX_IDX_SHIFT | SXE_RXTXIDX_WRITE;
	SXE_REG_WRITE(hw, SXE_IPSTXIDX, reg);
	SXE_WRITE_FLUSH(hw);

	return;
}

static void sxe_hw_ipsec_tx_key_store(struct sxe_hw *hw, u32 *key,
						u8 key_len, u32 salt, u16 sa_idx)
{
	u8 i;

	for (i = 0; i < key_len; i++) {
		SXE_REG_WRITE(hw, SXE_IPSTXKEY(i),
			(__force u32)cpu_to_be32(key[(key_len - 1) - i]));
	}
	SXE_REG_WRITE(hw, SXE_IPSTXSALT, (__force u32)cpu_to_be32(salt));
	SXE_WRITE_FLUSH(hw);

	sxe_hw_ipsec_tx_sa_load(hw, sa_idx);

	return;
}

static void sxe_hw_ipsec_sec_data_stop(struct sxe_hw *hw, bool is_linkup)
{
	u32 tx_empty, rx_empty;
	u32 limit;
	u32 reg;

	reg = SXE_REG_READ(hw, SXE_SECTXCTRL);
	reg |= SXE_SECTXCTRL_TX_DIS;
	SXE_REG_WRITE(hw, SXE_SECTXCTRL, reg);

	reg = SXE_REG_READ(hw, SXE_SECRXCTRL);
	reg |= SXE_SECRXCTRL_RX_DIS;
	SXE_REG_WRITE(hw, SXE_SECRXCTRL, reg);

	tx_empty = SXE_REG_READ(hw, SXE_SECTXSTAT) & SXE_SECTXSTAT_SECTX_RDY;
	rx_empty = SXE_REG_READ(hw, SXE_SECRXSTAT) & SXE_SECRXSTAT_SECRX_RDY;
	if (tx_empty && rx_empty) {
		goto l_out;
	}

	if (!is_linkup) {
		SXE_REG_WRITE(hw, SXE_LPBKCTRL, SXE_LPBKCTRL_EN);

		SXE_WRITE_FLUSH(hw);
		mdelay(3);
	}

	limit = 20;
	do {
	   mdelay(10);
	   tx_empty = SXE_REG_READ(hw, SXE_SECTXSTAT) &
		   SXE_SECTXSTAT_SECTX_RDY;
	   rx_empty = SXE_REG_READ(hw, SXE_SECRXSTAT) &
		   SXE_SECRXSTAT_SECRX_RDY;
	} while (!(tx_empty && rx_empty) && limit--);

	if (!is_linkup) {
		SXE_REG_WRITE(hw, SXE_LPBKCTRL, 0);

		SXE_WRITE_FLUSH(hw);
	}

l_out:
	return;
}

static void sxe_hw_ipsec_engine_start(struct sxe_hw *hw, bool is_linkup)
{
	u32 reg;

	sxe_hw_ipsec_sec_data_stop(hw, is_linkup);

	reg = SXE_REG_READ(hw, SXE_SECTXMINIFG);
	reg = (reg & 0xfffffff0) | 0x3;
	SXE_REG_WRITE(hw, SXE_SECTXMINIFG, reg);

	reg = SXE_REG_READ(hw, SXE_SECTXBUFFAF);
	reg = (reg & 0xfffffc00) | 0x15;
	SXE_REG_WRITE(hw, SXE_SECTXBUFFAF, reg);

	SXE_REG_WRITE(hw, SXE_SECRXCTRL, 0);
	SXE_REG_WRITE(hw, SXE_SECTXCTRL, SXE_SECTXCTRL_STORE_FORWARD);

	SXE_REG_WRITE(hw, SXE_IPSTXIDX, SXE_RXTXIDX_IPS_EN);
	SXE_REG_WRITE(hw, SXE_IPSRXIDX, SXE_RXTXIDX_IPS_EN);

	SXE_WRITE_FLUSH(hw);

	return;
}

static void sxe_hw_ipsec_engine_stop(struct sxe_hw *hw, bool is_linkup)
{
	u32 reg;

	sxe_hw_ipsec_sec_data_stop(hw, is_linkup);

	SXE_REG_WRITE(hw, SXE_IPSTXIDX, 0);
	SXE_REG_WRITE(hw, SXE_IPSRXIDX, 0);

	reg = SXE_REG_READ(hw, SXE_SECTXCTRL);
	reg |= SXE_SECTXCTRL_SECTX_DIS;
	reg &= ~SXE_SECTXCTRL_STORE_FORWARD;
	SXE_REG_WRITE(hw, SXE_SECTXCTRL, reg);

	reg = SXE_REG_READ(hw, SXE_SECRXCTRL);
	reg |= SXE_SECRXCTRL_SECRX_DIS;
	SXE_REG_WRITE(hw, SXE_SECRXCTRL, reg);

	 SXE_REG_WRITE(hw, SXE_SECTXBUFFAF, 0x250);

	 reg = SXE_REG_READ(hw, SXE_SECTXMINIFG);
	 reg = (reg & 0xfffffff0) | 0x1;
	 SXE_REG_WRITE(hw, SXE_SECTXMINIFG, reg);

	 SXE_REG_WRITE(hw, SXE_SECTXCTRL, SXE_SECTXCTRL_SECTX_DIS);
	 SXE_REG_WRITE(hw, SXE_SECRXCTRL, SXE_SECRXCTRL_SECRX_DIS);

	 SXE_WRITE_FLUSH(hw);

	 return;
}

bool sxe_hw_ipsec_offload_is_disable(struct sxe_hw *hw)
{
	u32 tx_dis = SXE_REG_READ(hw, SXE_SECTXSTAT);
	u32 rx_dis = SXE_REG_READ(hw, SXE_SECRXSTAT);
	bool ret = false;

	if ((tx_dis & SXE_SECTXSTAT_SECTX_OFF_DIS) ||
	    (rx_dis & SXE_SECRXSTAT_SECRX_OFF_DIS)) {
		ret = true;
	}

	return ret;
}

void sxe_hw_ipsec_sa_disable(struct sxe_hw *hw)
{
	SXE_REG_WRITE(hw, SXE_IPSRXIDX, 0);
	SXE_REG_WRITE(hw, SXE_IPSTXIDX, 0);

	return;
}

static const struct sxe_sec_operations sxe_sec_ops = {
	.ipsec_rx_ip_store		= sxe_hw_ipsec_rx_ip_store,
	.ipsec_rx_spi_store		= sxe_hw_ipsec_rx_spi_store,
	.ipsec_rx_key_store		= sxe_hw_ipsec_rx_key_store,
	.ipsec_tx_key_store		= sxe_hw_ipsec_tx_key_store,
	.ipsec_sec_data_stop		= sxe_hw_ipsec_sec_data_stop,
	.ipsec_engine_start		= sxe_hw_ipsec_engine_start,
	.ipsec_engine_stop		= sxe_hw_ipsec_engine_stop,
	.ipsec_sa_disable		= sxe_hw_ipsec_sa_disable,
	.ipsec_offload_is_disable	= sxe_hw_ipsec_offload_is_disable,
};
#endif

static const struct sxe_sec_operations sxe_sec_ops = { 0 };


void sxe_hw_stats_regs_clean(struct sxe_hw *hw)
{
	u16 i;
	for (i = 0; i < 16; i++) {
		SXE_REG_READ(hw, SXE_QPTC(i));
		SXE_REG_READ(hw, SXE_QPRC(i));
		SXE_REG_READ(hw, SXE_QBTC_H(i));
		SXE_REG_READ(hw, SXE_QBTC_L(i));
		SXE_REG_READ(hw, SXE_QBRC_H(i));
		SXE_REG_READ(hw, SXE_QBRC_L(i));
		SXE_REG_READ(hw, SXE_QPRDC(i));
	}

	SXE_REG_READ(hw, SXE_RXDGBCH);
	SXE_REG_READ(hw, SXE_RXDGBCL);
	SXE_REG_READ(hw, SXE_RXDGPC);
	SXE_REG_READ(hw, SXE_TXDGPC);
	SXE_REG_READ(hw, SXE_TXDGBCH);
	SXE_REG_READ(hw, SXE_TXDGBCL);
	SXE_REG_READ(hw,SXE_RXDDGPC);
	SXE_REG_READ(hw, SXE_RXDDGBCH);
	SXE_REG_READ(hw,SXE_RXDDGBCL);
	SXE_REG_READ(hw,SXE_RXLPBKGPC);
	SXE_REG_READ(hw, SXE_RXLPBKGBCH);
	SXE_REG_READ(hw,SXE_RXLPBKGBCL);
	SXE_REG_READ(hw,SXE_RXDLPBKGPC);
	SXE_REG_READ(hw, SXE_RXDLPBKGBCH);
	SXE_REG_READ(hw,SXE_RXDLPBKGBCL);
	SXE_REG_READ(hw,SXE_RXTPCIN);
	SXE_REG_READ(hw,SXE_RXTPCOUT);
	SXE_REG_READ(hw,SXE_RXPRDDC);
	SXE_REG_READ(hw, SXE_TXSWERR);
	SXE_REG_READ(hw, SXE_TXSWITCH);
	SXE_REG_READ(hw, SXE_TXREPEAT);
	SXE_REG_READ(hw, SXE_TXDESCERR);

	SXE_REG_READ(hw, SXE_CRCERRS);
	SXE_REG_READ(hw, SXE_ERRBC);
	SXE_REG_READ(hw, SXE_RLEC);
	SXE_REG_READ(hw, SXE_PRC64);
	SXE_REG_READ(hw, SXE_PRC127);
	SXE_REG_READ(hw, SXE_PRC255);
	SXE_REG_READ(hw, SXE_PRC511);
	SXE_REG_READ(hw, SXE_PRC1023);
	SXE_REG_READ(hw, SXE_PRC1522);
	SXE_REG_READ(hw, SXE_GPRC);
	SXE_REG_READ(hw, SXE_BPRC);
	SXE_REG_READ(hw, SXE_MPRC);
	SXE_REG_READ(hw, SXE_GPTC);
	SXE_REG_READ(hw, SXE_GORCL);
	SXE_REG_READ(hw, SXE_GORCH);
	SXE_REG_READ(hw, SXE_GOTCL);
	SXE_REG_READ(hw, SXE_GOTCH);
	SXE_REG_READ(hw, SXE_RUC);
	SXE_REG_READ(hw, SXE_RFC);
	SXE_REG_READ(hw, SXE_ROC);
	SXE_REG_READ(hw, SXE_RJC);
	for (i = 0; i < 8; i++) {
		SXE_REG_READ(hw, SXE_PRCPF(i));
	}
	SXE_REG_READ(hw, SXE_TORL);
	SXE_REG_READ(hw, SXE_TORH);
	SXE_REG_READ(hw, SXE_TPR);
	SXE_REG_READ(hw, SXE_TPT);
	SXE_REG_READ(hw, SXE_PTC64);
	SXE_REG_READ(hw, SXE_PTC127);
	SXE_REG_READ(hw, SXE_PTC255);
	SXE_REG_READ(hw, SXE_PTC511);
	SXE_REG_READ(hw, SXE_PTC1023);
	SXE_REG_READ(hw, SXE_PTC1522);
	SXE_REG_READ(hw, SXE_MPTC);
	SXE_REG_READ(hw, SXE_BPTC);
	for (i = 0; i < 8; i++) {
		SXE_REG_READ(hw, SXE_PFCT(i));
	}

	return;
}

static void sxe_hw_stats_seq_get(struct sxe_hw *hw, struct sxe_mac_stats *stats)
{
	u8 i;
	u64 tx_pfc_num = 0;
#ifdef SXE_DPDK
	u64 gotch = 0;
	u32 rycle_cnt = 10;
#endif

	for (i = 0; i < 8; i++) {
		stats->prcpf[i] += SXE_REG_READ(hw,SXE_PRCPF(i));
		tx_pfc_num = SXE_REG_READ(hw,SXE_PFCT(i));
		stats->pfct[i] += tx_pfc_num;
		stats->total_tx_pause += tx_pfc_num;
	}

	stats->total_gptc += SXE_REG_READ(hw, SXE_GPTC);
	stats->total_gotc += (SXE_REG_READ(hw, SXE_GOTCL) |
			((u64)SXE_REG_READ(hw, SXE_GOTCH) << 32));
#ifdef SXE_DPDK
	do {
		gotch = SXE_REG_READ(hw, SXE_GOTCH);
		rycle_cnt--;
	} while (gotch != 0 && rycle_cnt != 0);
	if (gotch != 0) {
		LOG_INFO("GOTCH is not clear!\n");
	}
#endif

	return;
}

void sxe_hw_stats_seq_clean(struct sxe_hw *hw, struct sxe_mac_stats *stats)
{
	u8 i;
	u64 tx_pfc_num = 0;
	u64 gotch = 0;
	u32 rycle_cnt = 10;

	stats->total_gotc += (SXE_REG_READ(hw, SXE_GOTCL) |
			((u64)SXE_REG_READ(hw, SXE_GOTCH) << 32));
	stats->total_gptc += SXE_REG_READ(hw, SXE_GPTC);
	do {
		gotch = SXE_REG_READ(hw, SXE_GOTCH);
		rycle_cnt--;
	} while (gotch != 0 && rycle_cnt != 0);
	if (gotch != 0) {
		LOG_INFO("GOTCH is not clear!\n");
	}
	
	for (i = 0; i < 8; i++) {
		stats->prcpf[i] += SXE_REG_READ(hw,SXE_PRCPF(i));
		tx_pfc_num = SXE_REG_READ(hw,SXE_PFCT(i));
		stats->pfct[i] += tx_pfc_num;
		stats->total_tx_pause += tx_pfc_num;
	}

	return;
}

void sxe_hw_stats_get(struct sxe_hw *hw, struct sxe_mac_stats *stats)
{
	u64 rjc;
	u32 i, rx_dbu_drop, ring_drop = 0;
	u64 tpr = 0;
#ifdef SXE_DPDK
	u32 rycle_cnt = 10;
	u64 gorch, torh = 0;
#endif

	for (i = 0; i < 16; i++) {
		stats->qptc[i] += SXE_REG_READ(hw, SXE_QPTC(i));
		stats->qprc[i] += SXE_REG_READ(hw, SXE_QPRC(i));
		ring_drop = SXE_REG_READ(hw, SXE_QPRDC(i));
		stats->qprdc[i] += ring_drop;
		stats->hw_rx_no_dma_resources += ring_drop;

		stats->qbtc[i] += ((u64)SXE_REG_READ(hw, SXE_QBTC_H(i)) << 32);
		SXE_RMB();
		stats->qbtc[i] += SXE_REG_READ(hw, SXE_QBTC_L(i));

		stats->qbrc[i] += ((u64)SXE_REG_READ(hw, SXE_QBRC_H(i)) << 32);
		SXE_RMB();
		stats->qbrc[i] += SXE_REG_READ(hw, SXE_QBRC_L(i));
	}
	stats->rxdgbc += ((u64)SXE_REG_READ(hw, SXE_RXDGBCH) << 32) +
				(SXE_REG_READ(hw, SXE_RXDGBCL));

	stats->rxdgpc += SXE_REG_READ(hw, SXE_RXDGPC);
	stats->txdgpc += SXE_REG_READ(hw, SXE_TXDGPC);
	stats->txdgbc += (((u64)SXE_REG_READ(hw, SXE_TXDGBCH) << 32) +
				SXE_REG_READ(hw, SXE_TXDGBCL));

	stats->rxddpc += SXE_REG_READ(hw,SXE_RXDDGPC);
	stats->rxddbc += ((u64)SXE_REG_READ(hw, SXE_RXDDGBCH) << 32) +
				(SXE_REG_READ(hw,SXE_RXDDGBCL));

	stats->rxlpbkpc += SXE_REG_READ(hw,SXE_RXLPBKGPC);
	stats->rxlpbkbc += ((u64)SXE_REG_READ(hw, SXE_RXLPBKGBCH) << 32) +
			(SXE_REG_READ(hw,SXE_RXLPBKGBCL));

	stats->rxdlpbkpc += SXE_REG_READ(hw,SXE_RXDLPBKGPC);
	stats->rxdlpbkbc += ((u64)SXE_REG_READ(hw, SXE_RXDLPBKGBCH) << 32) +
				(SXE_REG_READ(hw,SXE_RXDLPBKGBCL));
	stats->rxtpcing += SXE_REG_READ(hw,SXE_RXTPCIN);
	stats->rxtpceng += SXE_REG_READ(hw,SXE_RXTPCOUT);
	stats->prddc += SXE_REG_READ(hw,SXE_RXPRDDC);
	stats->txswerr += SXE_REG_READ(hw, SXE_TXSWERR);
	stats->txswitch += SXE_REG_READ(hw, SXE_TXSWITCH);
	stats->txrepeat += SXE_REG_READ(hw, SXE_TXREPEAT);
	stats->txdescerr += SXE_REG_READ(hw, SXE_TXDESCERR);

	for (i = 0; i < 8; i++) {
		stats->dburxtcin[i] += SXE_REG_READ(hw, SXE_DBUDRTCICNT(i));
		stats->dburxtcout[i] += SXE_REG_READ(hw, SXE_DBUDRTCOCNT(i));
		stats->dburxgdreecnt[i] += SXE_REG_READ(hw, SXE_DBUDREECNT(i));
		rx_dbu_drop = SXE_REG_READ(hw, SXE_DBUDROFPCNT(i));
		stats->dburxdrofpcnt[i] += rx_dbu_drop;
		stats->dbutxtcin[i] += SXE_REG_READ(hw,SXE_DBUDTTCICNT(i));
		stats->dbutxtcout[i] += SXE_REG_READ(hw,SXE_DBUDTTCOCNT(i));
	}

	stats->fnavadd += (SXE_REG_READ(hw, SXE_FNAVUSTAT) & 0xFFFF);
	stats->fnavrmv += ((SXE_REG_READ(hw, SXE_FNAVUSTAT) >> 16) & 0xFFFF);
	stats->fnavadderr += (SXE_REG_READ(hw, SXE_FNAVFSTAT) & 0xFFFF);
	stats->fnavrmverr += ((SXE_REG_READ(hw, SXE_FNAVFSTAT) >> 16) & 0xFFFF);
	stats->fnavmatch += SXE_REG_READ(hw, SXE_FNAVMATCH);
	stats->fnavmiss += SXE_REG_READ(hw, SXE_FNAVMISS);

	sxe_hw_stats_seq_get(hw, stats);

	stats->crcerrs += SXE_REG_READ(hw, SXE_CRCERRS);
	stats->errbc   += SXE_REG_READ(hw, SXE_ERRBC);
	stats->bprc += SXE_REG_READ(hw, SXE_BPRC);
	stats->mprc += SXE_REG_READ(hw, SXE_MPRC);
	stats->roc += SXE_REG_READ(hw, SXE_ROC);
	stats->prc64 += SXE_REG_READ(hw, SXE_PRC64);
	stats->prc127 += SXE_REG_READ(hw, SXE_PRC127);
	stats->prc255 += SXE_REG_READ(hw, SXE_PRC255);
	stats->prc511 += SXE_REG_READ(hw, SXE_PRC511);
	stats->prc1023 += SXE_REG_READ(hw, SXE_PRC1023);
	stats->prc1522 += SXE_REG_READ(hw, SXE_PRC1522);
	stats->rlec += SXE_REG_READ(hw, SXE_RLEC);
	stats->mptc += SXE_REG_READ(hw, SXE_MPTC);
	stats->ruc += SXE_REG_READ(hw, SXE_RUC);
	stats->rfc += SXE_REG_READ(hw, SXE_RFC);

	rjc = SXE_REG_READ(hw, SXE_RJC);
	stats->rjc += rjc;
	stats->roc += rjc;

	tpr = SXE_REG_READ(hw, SXE_TPR);
	stats->tpr += tpr;
	stats->tpt += SXE_REG_READ(hw, SXE_TPT);
	stats->ptc64 += SXE_REG_READ(hw, SXE_PTC64);
	stats->ptc127 += SXE_REG_READ(hw, SXE_PTC127);
	stats->ptc255 += SXE_REG_READ(hw, SXE_PTC255);
	stats->ptc511 += SXE_REG_READ(hw, SXE_PTC511);
	stats->ptc1023 += SXE_REG_READ(hw, SXE_PTC1023);
	stats->ptc1522 += SXE_REG_READ(hw, SXE_PTC1522);
	stats->bptc += SXE_REG_READ(hw, SXE_BPTC);

	stats->gprc += SXE_REG_READ(hw, SXE_GPRC);
	stats->gorc += (SXE_REG_READ(hw, SXE_GORCL) |
			((u64)SXE_REG_READ(hw, SXE_GORCH) << 32));
#ifdef SXE_DPDK
	do {
		gorch = SXE_REG_READ(hw, SXE_GORCH);
		rycle_cnt--;
	} while (gorch != 0 && rycle_cnt != 0);
	if (gorch != 0) {
		LOG_INFO("GORCH is not clear!\n");
	}
#endif

	stats->tor += (SXE_REG_READ(hw, SXE_TORL) |
			((u64)SXE_REG_READ(hw, SXE_TORH) << 32));
#ifdef SXE_DPDK
	rycle_cnt = 10;
	do {
		torh = SXE_REG_READ(hw, SXE_TORH);
		rycle_cnt--;
	} while (torh != 0 && rycle_cnt != 0);
	if (torh != 0) {
		LOG_INFO("TORH is not clear!\n");
	}
#endif

#ifdef SXE_DPDK
	stats->tor -= tpr * RTE_ETHER_CRC_LEN;
	stats->gptc = stats->total_gptc - stats->total_tx_pause;
	stats->gotc = stats->total_gotc - stats->total_tx_pause * RTE_ETHER_MIN_LEN
			- stats->gptc * RTE_ETHER_CRC_LEN;
#else
	stats->gptc = stats->total_gptc;
	stats->gotc = stats->total_gotc;
#endif

	return;
}

static u32 sxe_hw_tx_packets_num_get(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_TXDGPC);
}

static u32 sxe_hw_unsec_packets_num_get(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_SSVPC);
}

static u32 sxe_hw_mac_stats_dump(struct sxe_hw *hw, u32 *regs_buff, u32 buf_size)
{
	u32 i;
	u32 regs_num = buf_size / sizeof(u32);

	for (i = 0; i < regs_num; i++) {
		regs_buff[i] = SXE_REG_READ(hw, mac_regs[i]);
	}

	return i;
}

static u32 sxe_hw_tx_dbu_to_mac_stats(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_DTMPCNT);
}

static const struct sxe_stat_operations sxe_stat_ops = {
	.stats_get			= sxe_hw_stats_get,
	.stats_clear			= sxe_hw_stats_regs_clean,
	.mac_stats_dump			= sxe_hw_mac_stats_dump,
	.tx_packets_num_get		= sxe_hw_tx_packets_num_get,
	.unsecurity_packets_num_get	= sxe_hw_unsec_packets_num_get,
	.tx_dbu_to_mac_stats		= sxe_hw_tx_dbu_to_mac_stats,
};

void sxe_hw_mbx_init(struct sxe_hw *hw)
{
	hw->mbx.msg_len = SXE_MBX_MSG_NUM;
	hw->mbx.interval = SXE_MBX_RETRY_INTERVAL;
	hw->mbx.retry = SXE_MBX_RETRY_COUNT;

	hw->mbx.stats.rcv_msgs   = 0;
	hw->mbx.stats.send_msgs  = 0;
	hw->mbx.stats.acks       = 0;
	hw->mbx.stats.reqs       = 0;
	hw->mbx.stats.rsts       = 0;

	return;
}

static bool sxe_hw_vf_irq_check(struct sxe_hw *hw, u32 mask, u32 index)
{
	u32 value = SXE_REG_READ(hw, SXE_PFMBICR(index));

	if (value & mask) {
		SXE_REG_WRITE(hw, SXE_PFMBICR(index), mask);
		return true;
	}

	return false;
}

bool sxe_hw_vf_rst_check(struct sxe_hw *hw, u8 vf_idx)
{
	u32 index = vf_idx >> 5;
	u32 bit = vf_idx % 32;
	u32 value;

	value = SXE_REG_READ(hw, SXE_VFLRE(index));
	if (value & BIT(bit)) {
		SXE_REG_WRITE(hw, SXE_VFLREC(index), BIT(bit));
		hw->mbx.stats.rsts++;
		return true;
	}

	return false;
}

bool sxe_hw_vf_req_check(struct sxe_hw *hw, u8 vf_idx)
{
	u8 index = vf_idx >> 4;
	u8 bit = vf_idx % 16;

	if (sxe_hw_vf_irq_check(hw, SXE_PFMBICR_VFREQ << bit, index)) {
		hw->mbx.stats.reqs++;
		return true;
	}

	return false;
}

bool sxe_hw_vf_ack_check(struct sxe_hw *hw, u8 vf_idx)
{
	u8 index = vf_idx >> 4;
	u8 bit = vf_idx % 16;

	if (sxe_hw_vf_irq_check(hw, SXE_PFMBICR_VFACK << bit, index)) {
		hw->mbx.stats.acks++;
		return true;
	}

	return false;
}

static bool sxe_hw_mbx_lock(struct sxe_hw *hw, u8 vf_idx)
{
	u32 value;
	bool ret = false;
	u32 retry = hw->mbx.retry;

	while (retry--) {
		SXE_REG_WRITE(hw, SXE_PFMAILBOX(vf_idx), SXE_PFMAILBOX_PFU);

		value = SXE_REG_READ(hw, SXE_PFMAILBOX(vf_idx));
		if (value & SXE_PFMAILBOX_PFU) {
			ret = true;
			break;
		}

		udelay(hw->mbx.interval);
	}

	return ret;
}

s32 sxe_hw_rcv_msg_from_vf(struct sxe_hw *hw, u32 *msg,
				u16 msg_len, u16 index)
{
	struct sxe_mbx_info *mbx = &hw->mbx;
	u8 i;
	s32 ret = 0;
	u16 msg_entry;
	struct sxe_adapter *adapter = hw->adapter;

	msg_entry = (msg_len > mbx->msg_len) ? mbx->msg_len : msg_len;

	if (!sxe_hw_mbx_lock(hw, index)) {
		ret = -SXE_ERR_MBX_LOCK_FAIL;
		LOG_ERROR_BDF("vf idx:%d msg_len:%d rcv lock mailbox fail.(err:%d)\n",
			   index, msg_len, ret);
		goto l_out;
	}

	for (i = 0; i < msg_entry; i++) {
		msg[i] = SXE_REG_READ(hw, (SXE_PFMBMEM(index) + (i << 2)));
		LOG_DEBUG_BDF("vf_idx:%u read mbx mem[%u]:0x%x.\n",
			      index, i, msg[i]);
	}

	SXE_REG_WRITE(hw, SXE_PFMAILBOX(index), SXE_PFMAILBOX_ACK);
	mbx->stats.rcv_msgs++;

l_out:
	return ret;
}

s32 sxe_hw_send_msg_to_vf(struct sxe_hw *hw, u32 *msg,
				u16 msg_len, u16 index)
{
	struct sxe_mbx_info *mbx = &hw->mbx;
	u8 i;
	s32 ret = 0;
	u32 old;
	struct sxe_adapter *adapter = hw->adapter;

	if (msg_len > mbx->msg_len) {
		ret = -EINVAL;
		LOG_ERROR_BDF("pf reply msg num:%d exceed limit:%d reply fail.(err:%d)\n",
			  msg_len, mbx->msg_len, ret);
		goto l_out;
	}

	if (!sxe_hw_mbx_lock(hw, index)) {
		ret = -SXE_ERR_MBX_LOCK_FAIL;
		LOG_ERROR_BDF("send msg len:%u to vf idx:%u msg[0]:0x%x "
			   "lock mailbox fail.(err:%d)\n",
			   msg_len, index, msg[0], ret);
		goto l_out;
	}

	old = SXE_REG_READ(hw, (SXE_PFMBMEM(index)));
	LOG_DEBUG_BDF("original send msg:0x%x. mbx mem[0]:0x%x\n", *msg, old);
	if (msg[0] & SXE_CTRL_MSG_MASK) {
		msg[0] |= (old & SXE_MSGID_MASK);
	} else {
		msg[0] |= (old & SXE_PFMSG_MASK);
	}

	for (i = 0; i < msg_len; i++) {
		SXE_REG_WRITE(hw, (SXE_PFMBMEM(index) + (i << 2)), msg[i]);
		LOG_DEBUG_BDF("vf_idx:%u write mbx mem[%u]:0x%x.\n",
			      index, i, msg[i]);
	}

	SXE_REG_WRITE(hw, SXE_PFMAILBOX(index), SXE_PFMAILBOX_STS);
	mbx->stats.send_msgs++;

l_out:
	return ret;
}

void sxe_hw_mbx_mem_clear(struct sxe_hw *hw, u8 vf_idx)
{
	u8 msg_idx;
	struct sxe_adapter *adapter = hw->adapter;
	for (msg_idx = 0; msg_idx < hw->mbx.msg_len; msg_idx++) {
		SXE_REG_WRITE_ARRAY(hw, SXE_PFMBMEM(vf_idx), msg_idx, 0);
	}

	SXE_WRITE_FLUSH(hw);

	LOG_INFO_BDF("vf_idx:%u clear mbx mem.\n", vf_idx);
	return;
}

static const struct sxe_mbx_operations sxe_mbx_ops = {
	.init		= sxe_hw_mbx_init,

	.req_check	= sxe_hw_vf_req_check,
	.ack_check	= sxe_hw_vf_ack_check,
	.rst_check	= sxe_hw_vf_rst_check,

	.msg_send	= sxe_hw_send_msg_to_vf,
	.msg_rcv	= sxe_hw_rcv_msg_from_vf,

	.mbx_mem_clear	= sxe_hw_mbx_mem_clear,
};

void sxe_hw_pcie_vt_mode_set(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_GCR_EXT, value);

	return;
}

static const struct sxe_pcie_operations sxe_pcie_ops = {
	.vt_mode_set	= sxe_hw_pcie_vt_mode_set,
};

s32 sxe_hw_hdc_lock_get(struct sxe_hw *hw, u32 trylock)
{
	u32 val;
	u16 i;
	s32 ret = 0;
	struct sxe_adapter *adapter = hw->adapter;

	SXE_REG_WRITE(hw, SXE_HDC_SW_LK, SXE_HDC_RELEASE_SW_LK);
	SXE_WRITE_FLUSH(hw);

	for (i = 0; i < trylock; i++) {
		val = SXE_REG_READ(hw, SXE_HDC_SW_LK) & SXE_HDC_SW_LK_BIT;
		if (!val) {
			break;
		}

		udelay(10);
	}

	if (i >= trylock) {
		LOG_ERROR_BDF("hdc is busy, reg: 0x%x\n", val);
		ret = -SXE_ERR_HDC_LOCK_BUSY;
		goto l_out;
	}

	val = SXE_REG_READ(hw, SXE_HDC_PF_LK) & SXE_HDC_PF_LK_BIT;
	if (!val) {
		SXE_REG_WRITE(hw, SXE_HDC_SW_LK, SXE_HDC_RELEASE_SW_LK);
		LOG_ERROR_BDF("get hdc lock fail, reg: 0x%x\n", val);
		ret = -SXE_ERR_HDC_LOCK_BUSY;
		goto l_out;
	}

	hw->hdc.pf_lock_val = val;
	LOG_DEBUG_BDF("hw[%p]'s port[%u] got pf lock\n", hw, val);

l_out:
	return ret;
}

void sxe_hw_hdc_lock_release(struct sxe_hw *hw, u32 retry_cnt)
{
	struct sxe_adapter *adapter = hw->adapter;

	do {
		SXE_REG_WRITE(hw, SXE_HDC_SW_LK, SXE_HDC_RELEASE_SW_LK);
		udelay(1);
		if (!(SXE_REG_READ(hw, SXE_HDC_PF_LK) & hw->hdc.pf_lock_val)) {
			LOG_DEBUG_BDF("hw[%p]'s port[%u] release pf lock\n", hw,
				hw->hdc.pf_lock_val);
			hw->hdc.pf_lock_val = 0;
			break;
		}
	} while((retry_cnt--) > 0);

	return;
}

void sxe_hw_hdc_fw_ov_clear(struct sxe_hw *hw)
{
	SXE_REG_WRITE(hw, SXE_HDC_FW_OV, 0);
}

bool sxe_hw_hdc_is_fw_over_set(struct sxe_hw *hw)
{
	bool fw_ov = false;

	if (SXE_REG_READ(hw, SXE_HDC_FW_OV) & SXE_HDC_FW_OV_BIT) {
		fw_ov = true;
	}

	return fw_ov;
}

void sxe_hw_hdc_packet_send_done(struct sxe_hw *hw)
{
	SXE_REG_WRITE(hw, SXE_HDC_SW_OV, SXE_HDC_SW_OV_BIT);
	SXE_WRITE_FLUSH(hw);

	return;
}

void sxe_hw_hdc_packet_header_send(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_HDC_PACKET_HEAD0, value);

	return;
}

void sxe_hw_hdc_packet_data_dword_send(struct sxe_hw *hw,
						u16 dword_index, u32 value)
{
	SXE_WRITE_REG_ARRAY_32(hw, SXE_HDC_PACKET_DATA0, dword_index, value);
	return;
}

u32 sxe_hw_hdc_fw_ack_header_get(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_HDC_PACKET_HEAD0);
}

u32 sxe_hw_hdc_packet_data_dword_rcv(struct sxe_hw *hw,
						u16 dword_index)
{
	return SXE_READ_REG_ARRAY_32(hw, SXE_HDC_PACKET_DATA0, dword_index);
}

u32 sxe_hw_hdc_fw_status_get(struct sxe_hw *hw)
{
	struct sxe_adapter *adapter = hw->adapter;
	u32 status = SXE_REG_READ(hw, SXE_FW_STATUS_REG);

	LOG_DEBUG_BDF("fw status[0x%x]\n", status);

	return status;
}

void sxe_hw_hdc_drv_status_set(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_DRV_STATUS_REG, value);
	return;
}

u32 sxe_hw_hdc_channel_state_get(struct sxe_hw *hw)
{
	struct sxe_adapter *adapter = hw->adapter;

	u32 state = SXE_REG_READ(hw, SXE_FW_HDC_STATE_REG);

	LOG_DEBUG_BDF("hdc channel state[0x%x]\n", state);

	return state;
}

STATIC u32 sxe_hw_hdc_irq_event_get(struct sxe_hw *hw)
{
	u32 status = SXE_REG_READ(hw, SXE_HDC_MSI_STATUS_REG);
	struct sxe_adapter *adapter = hw->adapter;

	LOG_DEBUG_BDF("msi status[0x%x]\n", status);

	return status;
}

static void sxe_hw_hdc_irq_event_clear(struct sxe_hw *hw, u32 event)
{
	u32 status = SXE_REG_READ(hw, SXE_HDC_MSI_STATUS_REG);
	struct sxe_adapter *adapter = hw->adapter;

	LOG_DEBUG_BDF("msi status[0x%x] and clear bit=[0x%x]\n", status, event);

	status &= ~event;
	SXE_REG_WRITE(hw, SXE_HDC_MSI_STATUS_REG, status);

	return;
}

static void sxe_hw_hdc_resource_clean(struct sxe_hw *hw)
{
	u16 i;

	SXE_REG_WRITE(hw, SXE_HDC_SW_LK, 0x0);
	SXE_REG_WRITE(hw, SXE_HDC_PACKET_HEAD0, 0x0);
	for (i = 0; i < SXE_HDC_DATA_LEN_MAX; i++) {
		SXE_WRITE_REG_ARRAY_32(hw, SXE_HDC_PACKET_DATA0, i, 0x0);
	}

	return;
}

static const struct sxe_hdc_operations sxe_hdc_ops = {
	.pf_lock_get            = sxe_hw_hdc_lock_get,
	.pf_lock_release        = sxe_hw_hdc_lock_release,
	.is_fw_over_set         = sxe_hw_hdc_is_fw_over_set,
	.fw_ack_header_rcv      = sxe_hw_hdc_fw_ack_header_get,
	.packet_send_done       = sxe_hw_hdc_packet_send_done,
	.packet_header_send     = sxe_hw_hdc_packet_header_send,
	.packet_data_dword_send = sxe_hw_hdc_packet_data_dword_send,
	.packet_data_dword_rcv  = sxe_hw_hdc_packet_data_dword_rcv,
	.fw_status_get          = sxe_hw_hdc_fw_status_get,
	.drv_status_set         = sxe_hw_hdc_drv_status_set,
	.irq_event_get          = sxe_hw_hdc_irq_event_get,
	.irq_event_clear        = sxe_hw_hdc_irq_event_clear,
	.fw_ov_clear            = sxe_hw_hdc_fw_ov_clear,
	.channel_state_get      = sxe_hw_hdc_channel_state_get,
	.resource_clean         = sxe_hw_hdc_resource_clean,
};

#ifdef SXE_PHY_CONFIGURE
#define SXE_MDIO_COMMAND_TIMEOUT 100 

static s32 sxe_hw_phy_reg_write(struct sxe_hw *hw, s32 prtad, u32 reg_addr,
				u32 device_type, u16 phy_data)
{
	s32 ret;
	u32 i, command;
	struct sxe_adapter *adapter = hw->adapter;

	SXE_REG_WRITE(hw, SXE_MSCD, (u32)phy_data);

	command = ((reg_addr << SXE_MSCA_NP_ADDR_SHIFT)  |
		   (device_type << SXE_MSCA_DEV_TYPE_SHIFT) |
		   (prtad << SXE_MSCA_PHY_ADDR_SHIFT) |
		   (SXE_MSCA_ADDR_CYCLE | SXE_MSCA_MDI_CMD_ON_PROG));

	SXE_REG_WRITE(hw, SXE_MSCA, command);

	for (i = 0; i < SXE_MDIO_COMMAND_TIMEOUT; i++) {
		udelay(10);

		command = SXE_REG_READ(hw, SXE_MSCA);
		if ((command & SXE_MSCA_MDI_CMD_ON_PROG) == 0) {
			break;
		}
	}

	if ((command & SXE_MSCA_MDI_CMD_ON_PROG) != 0) {
		LOG_DEV_ERR("phy write cmd didn't complete, "
			"reg_addr=%u, device_type=%u\n", reg_addr, device_type);
		ret = -SXE_ERR_MDIO_CMD_TIMEOUT;
		goto l_end;
	}

	command = ((reg_addr << SXE_MSCA_NP_ADDR_SHIFT)  |
		   (device_type << SXE_MSCA_DEV_TYPE_SHIFT) |
		   (prtad << SXE_MSCA_PHY_ADDR_SHIFT) |
		   (SXE_MSCA_WRITE | SXE_MSCA_MDI_CMD_ON_PROG));

	SXE_REG_WRITE(hw, SXE_MSCA, command);

	for (i = 0; i < SXE_MDIO_COMMAND_TIMEOUT; i++) {
		udelay(10);

		command = SXE_REG_READ(hw, SXE_MSCA);
		if ((command & SXE_MSCA_MDI_CMD_ON_PROG) == 0) {
			break;
		}
	}

	if ((command & SXE_MSCA_MDI_CMD_ON_PROG) != 0) {
		LOG_DEV_ERR("phy write cmd didn't complete, "
			"reg_addr=%u, device_type=%u\n", reg_addr, device_type);
		ret = -SXE_ERR_MDIO_CMD_TIMEOUT;
	}

l_end:
	return ret;
}

static s32 sxe_hw_phy_reg_read(struct sxe_hw *hw, s32 prtad, u32 reg_addr,
				u32 device_type, u16 *phy_data)
{
	s32 ret = 0;
	u32 i, data, command;
	struct sxe_adapter *adapter = hw->adapter;

	command = ((reg_addr << SXE_MSCA_NP_ADDR_SHIFT)  |
		   (device_type << SXE_MSCA_DEV_TYPE_SHIFT) |
		   (prtad << SXE_MSCA_PHY_ADDR_SHIFT) |
		   (SXE_MSCA_ADDR_CYCLE | SXE_MSCA_MDI_CMD_ON_PROG));

	SXE_REG_WRITE(hw, SXE_MSCA, command);

	for (i = 0; i < SXE_MDIO_COMMAND_TIMEOUT; i++) {
		udelay(10);

		command = SXE_REG_READ(hw, SXE_MSCA);
		if ((command & SXE_MSCA_MDI_CMD_ON_PROG) == 0) {
			break;
		}
	}

	if ((command & SXE_MSCA_MDI_CMD_ON_PROG) != 0) {
		LOG_DEV_ERR("phy read cmd didn't complete, "
			"reg_addr=%u, device_type=%u\n", reg_addr, device_type);
		ret = -SXE_ERR_MDIO_CMD_TIMEOUT;
		goto l_end;
	}

	command = ((reg_addr << SXE_MSCA_NP_ADDR_SHIFT)  |
		   (device_type << SXE_MSCA_DEV_TYPE_SHIFT) |
		   (prtad << SXE_MSCA_PHY_ADDR_SHIFT) |
		   (SXE_MSCA_READ | SXE_MSCA_MDI_CMD_ON_PROG));

	SXE_REG_WRITE(hw, SXE_MSCA, command);

	for (i = 0; i < SXE_MDIO_COMMAND_TIMEOUT; i++) {
		udelay(10);

		command = SXE_REG_READ(hw, SXE_MSCA);
		if ((command & SXE_MSCA_MDI_CMD_ON_PROG) == 0)
			break;
	}

	if ((command & SXE_MSCA_MDI_CMD_ON_PROG) != 0) {
		LOG_DEV_ERR("phy write cmd didn't complete, "
			"reg_addr=%u, device_type=%u\n", reg_addr, device_type);
		ret = -SXE_ERR_MDIO_CMD_TIMEOUT;
		goto l_end;
	}

	data = SXE_REG_READ(hw, SXE_MSCD);
	data >>= MDIO_MSCD_RDATA_SHIFT;
	*phy_data = (u16)(data);

l_end:
	return ret;
}

#define SXE_PHY_REVISION_MASK		0x000F
#define SXE_PHY_ID_HIGH_5_BIT_MASK	0xFC00
#define SXE_PHY_ID_HIGH_SHIFT		10

static s32 sxe_hw_phy_id_get(struct sxe_hw *hw, u32 prtad, u32 *id)
{
	s32 ret;
	u16 phy_id_high = 0;
	u16 phy_id_low = 0;


	ret = sxe_hw_phy_reg_read(hw, prtad, MDIO_DEVID1, MDIO_MMD_PMAPMD,
				      &phy_id_low);

	if (ret) {
		LOG_ERROR("get phy id upper 16 bits failed, prtad=%d\n", prtad);
		goto l_end;
	}

	ret = sxe_hw_phy_reg_read(hw, prtad, MDIO_DEVID2, MDIO_MMD_PMAPMD,
					&phy_id_high);
	if (ret) {
		LOG_ERROR("get phy id lower 4 bits failed, prtad=%d\n", prtad);
		goto l_end;
	}

	*id = (u32)((phy_id_high >> SXE_PHY_ID_HIGH_SHIFT) << 16);
	*id |= (u32)phy_id_low;

l_end:
	return ret;
}

s32 sxe_hw_phy_link_cap_get(struct sxe_hw *hw, u32 prtad, u32 *speed)
{
	s32 ret;
	u16 speed_ability;

	ret = hw->phy.ops->reg_read(hw, prtad, MDIO_SPEED, MDIO_MMD_PMAPMD,
				      &speed_ability);
	if (ret) {
		*speed = 0;
		LOG_ERROR("get phy link cap failed, ret=%d, prtad=%d\n",
							ret, prtad);
		goto l_end;
	}

	if (speed_ability & MDIO_SPEED_10G) {
		*speed |= SXE_LINK_SPEED_10GB_FULL;
	}

	if (speed_ability & MDIO_PMA_SPEED_1000) {
		*speed |= SXE_LINK_SPEED_1GB_FULL;
	}

	if (speed_ability & MDIO_PMA_SPEED_100) {
		*speed |= SXE_LINK_SPEED_100_FULL;
	}

l_end:
	return ret;
}

static s32 sxe_hw_phy_ctrl_reset(struct sxe_hw *hw, u32 prtad)
{
	u32 i;
	s32 ret;
	u16 ctrl;

	ret = sxe_hw_phy_reg_write(hw, prtad, MDIO_CTRL1,
			 MDIO_MMD_PHYXS, MDIO_CTRL1_RESET);
	if (ret) {
		LOG_ERROR("phy reset failed, ret=%d\n", ret);
		goto l_end;
	}

	for (i = 0; i < 30; i++) {
		msleep(100);
		ret = sxe_hw_phy_reg_read(hw, prtad, MDIO_CTRL1,
					MDIO_MMD_PHYXS, &ctrl);
		if (ret) {
			goto l_end;
		}

		if (!(ctrl & MDIO_CTRL1_RESET)) {
			udelay(2);
			break;
		}
	}

	if (ctrl & MDIO_CTRL1_RESET) {
		LOG_DEV_ERR("phy reset polling failed to complete\n");
		return -SXE_ERR_PHY_RESET_FAIL;
	}

l_end:
	return ret;
}

static const struct sxe_phy_operations sxe_phy_hw_ops = {
	.reg_write	= sxe_hw_phy_reg_write,
	.reg_read	= sxe_hw_phy_reg_read,
	.identifier_get	= sxe_hw_phy_id_get,
	.link_cap_get	= sxe_hw_phy_link_cap_get,
	.reset		= sxe_hw_phy_ctrl_reset,
};
#endif

void sxe_hw_ops_init(struct sxe_hw *hw)
{
	hw->setup.ops	= &sxe_setup_ops;
	hw->irq.ops	= &sxe_irq_ops;
	hw->mac.ops	= &sxe_mac_ops;
	hw->dbu.ops	= &sxe_dbu_ops;
	hw->dma.ops	= &sxe_dma_ops;
	hw->sec.ops	= &sxe_sec_ops;
	hw->stat.ops	= &sxe_stat_ops;
	hw->mbx.ops	= &sxe_mbx_ops;
	hw->pcie.ops	= &sxe_pcie_ops;
	hw->hdc.ops	= &sxe_hdc_ops;
#ifdef SXE_PHY_CONFIGURE
	hw->phy.ops     = &sxe_phy_hw_ops;
#endif

	hw->filter.mac.ops	= &sxe_filter_mac_ops;
	hw->filter.vlan.ops	= &sxe_filter_vlan_ops;
	return;
}

u32 sxe_hw_rss_key_get_by_idx(struct sxe_hw *hw, u8 reg_idx)
{
	u32 rss_key;

	if (reg_idx >= SXE_MAX_RSS_KEY_ENTRIES) {
		rss_key = 0;
	} else {
		rss_key = SXE_REG_READ(hw, SXE_RSSRK(reg_idx));
	}

	return rss_key;
}

bool sxe_hw_is_rss_enabled(struct sxe_hw *hw)
{
	bool rss_enable = false;
	u32 mrqc = SXE_REG_READ(hw, SXE_MRQC);
	if (mrqc & SXE_MRQC_RSSEN) {
		rss_enable = true;
	}

	return rss_enable;
}

static u32 sxe_hw_mrqc_reg_get(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_MRQC);
}

u32 sxe_hw_rss_field_get(struct sxe_hw *hw)
{
	u32 mrqc = sxe_hw_mrqc_reg_get(hw);
	return (mrqc & SXE_RSS_FIELD_MASK);
}

#ifdef SXE_DPDK 

#define SXE_TRAFFIC_CLASS_MAX  8

#define SXE_MR_VLAN_MSB_REG_OFFSET         4
#define SXE_MR_VIRTUAL_POOL_MSB_REG_OFFSET 4

#define SXE_MR_TYPE_MASK                   0x0F
#define SXE_MR_DST_POOL_OFFSET             8

void sxe_hw_crc_strip_config(struct sxe_hw *hw, bool keep_crc)
{
	u32 crcflag = SXE_REG_READ(hw, SXE_CRC_STRIP_REG);

	if (keep_crc) {
		crcflag |= SXE_KEEP_CRC_EN;
	} else {
		crcflag &= ~SXE_KEEP_CRC_EN;
	}

	SXE_REG_WRITE(hw, SXE_CRC_STRIP_REG, crcflag);
	return;
}

void sxe_hw_rx_pkt_buf_size_set(struct sxe_hw *hw, u8 tc_idx, u16 pbsize)
{
	u32 rxpbsize = pbsize << SXE_RX_PKT_BUF_SIZE_SHIFT;

	sxe_hw_rx_pkt_buf_switch(hw, false);
	SXE_REG_WRITE(hw, SXE_RXPBSIZE(tc_idx), rxpbsize);
	sxe_hw_rx_pkt_buf_switch(hw, true);

	return;
}

void sxe_hw_dcb_vmdq_mq_configure(struct sxe_hw *hw, u8 num_pools)
{
	u16 pbsize;
	u8 i, nb_tcs;
	u32 mrqc;

	nb_tcs = SXE_VMDQ_DCB_NUM_QUEUES / num_pools;

	pbsize = (u8)(SXE_RX_PKT_BUF_SIZE / nb_tcs);

	for (i = 0; i < nb_tcs; i++) {
		sxe_hw_rx_pkt_buf_size_set(hw, i, pbsize);
	}

	for (i = nb_tcs; i < ETH_DCB_NUM_USER_PRIORITIES; i++) {
		sxe_hw_rx_pkt_buf_size_set(hw, i, 0);
	}

	mrqc = (num_pools == RTE_ETH_16_POOLS) ?
		SXE_MRQC_VMDQRT8TCEN : SXE_MRQC_VMDQRT4TCEN;
	SXE_REG_WRITE(hw, SXE_MRQC, mrqc);

	SXE_REG_WRITE(hw, SXE_RTRPCS, SXE_RTRPCS_RRM);

	return;
}

static const struct sxe_reg_info sxe_regs_general_group[] = {
	{SXE_CTRL, 1, 1, "SXE_CTRL"},
	{SXE_STATUS, 1, 1, "SXE_STATUS"},
	{SXE_CTRL_EXT, 1, 1, "SXE_CTRL_EXT"},
	{0, 0, 0, ""}
};

static const struct sxe_reg_info sxe_regs_interrupt_group[] = {
	{SXE_EICS, 1, 1, "SXE_EICS"},
	{SXE_EIMS, 1, 1, "SXE_EIMS"},
	{SXE_EIMC, 1, 1, "SXE_EIMC"},
	{SXE_EIAC, 1, 1, "SXE_EIAC"},
	{SXE_EIAM, 1, 1, "SXE_EIAM"},
	{SXE_EITR(0), 24, 4, "SXE_EITR"},
	{SXE_IVAR(0), 24, 4, "SXE_IVAR"},
	{SXE_GPIE, 1, 1, "SXE_GPIE"},
	{0, 0, 0, ""}
};

static const struct sxe_reg_info sxe_regs_fctl_group[] = {
	{SXE_PFCTOP, 1, 1, "SXE_PFCTOP"},
	{SXE_FCRTV, 1, 1, "SXE_FCRTV"},
	{SXE_TFCS, 1, 1, "SXE_TFCS"},
	{0, 0, 0, ""}
};

static const struct sxe_reg_info sxe_regs_rxdma_group[] = {
	{SXE_RDBAL(0), 64, 0x40, "SXE_RDBAL"},
	{SXE_RDBAH(0), 64, 0x40, "SXE_RDBAH"},
	{SXE_RDLEN(0), 64, 0x40, "SXE_RDLEN"},
	{SXE_RDH(0), 64, 0x40, "SXE_RDH"},
	{SXE_RDT(0), 64, 0x40, "SXE_RDT"},
	{SXE_RXDCTL(0), 64, 0x40, "SXE_RXDCTL"},
	{SXE_SRRCTL(0), 16, 0x4, "SXE_SRRCTL"},
	{SXE_TPH_RXCTRL(0), 16, 4, "SXE_TPH_RXCTRL"},
	{SXE_RDRXCTL, 1, 1, "SXE_RDRXCTL"},
	{SXE_RXPBSIZE(0), 8, 4, "SXE_RXPBSIZE"},
	{SXE_RXCTRL, 1, 1, "SXE_RXCTRL"},
	{0, 0, 0, ""}
};

static const struct sxe_reg_info sxe_regs_rx_group[] = {
	{SXE_RXCSUM, 1, 1, "SXE_RXCSUM"},
	{SXE_RFCTL, 1, 1, "SXE_RFCTL"},
	{SXE_RAL(0), 16, 8, "SXE_RAL"},
	{SXE_RAH(0), 16, 8, "SXE_RAH"},
	{SXE_PSRTYPE(0), 1, 4, "SXE_PSRTYPE"},
	{SXE_FCTRL, 1, 1, "SXE_FCTRL"},
	{SXE_VLNCTRL, 1, 1, "SXE_VLNCTRL"},
	{SXE_MCSTCTRL, 1, 1, "SXE_MCSTCTRL"},
	{SXE_MRQC, 1, 1, "SXE_MRQC"},
	{SXE_VMD_CTL, 1, 1, "SXE_VMD_CTL"},

	{0, 0, 0, ""}
};

static struct sxe_reg_info sxe_regs_tx_group[] = {
	{SXE_TDBAL(0), 32, 0x40, "SXE_TDBAL"},
	{SXE_TDBAH(0), 32, 0x40, "SXE_TDBAH"},
	{SXE_TDLEN(0), 32, 0x40, "SXE_TDLEN"},
	{SXE_TDH(0), 32, 0x40, "SXE_TDH"},
	{SXE_TDT(0), 32, 0x40, "SXE_TDT"},
	{SXE_TXDCTL(0), 32, 0x40, "SXE_TXDCTL"},
	{SXE_TPH_TXCTRL(0), 16, 4, "SXE_TPH_TXCTRL"},
	{SXE_TXPBSIZE(0), 8, 4, "SXE_TXPBSIZE"},
	{0, 0, 0, ""}
};

static const struct sxe_reg_info sxe_regs_wakeup_group[] = {
	{SXE_WUC, 1, 1, "SXE_WUC"},
	{SXE_WUFC, 1, 1, "SXE_WUFC"},
	{SXE_WUS, 1, 1, "SXE_WUS"},
	{0, 0, 0, ""}
};

static const struct sxe_reg_info sxe_regs_dcb_group[] = {
	{0, 0, 0, ""}
};

static const struct sxe_reg_info sxe_regs_diagnostic_group[] = {

	{SXE_MFLCN, 1, 1, "SXE_MFLCN"},
	{0, 0, 0, ""},
};

static const struct sxe_reg_info *sxe_regs_group[] = {
				sxe_regs_general_group,
				sxe_regs_interrupt_group,
				sxe_regs_fctl_group,
				sxe_regs_rxdma_group,
				sxe_regs_rx_group,
				sxe_regs_tx_group,
				sxe_regs_wakeup_group,
				sxe_regs_dcb_group,
				sxe_regs_diagnostic_group,
				NULL};

static u32 sxe_regs_group_count(const struct sxe_reg_info *regs)
{
	int i = 0;
	int count = 0;

	while (regs[i].count) {
		count += regs[i++].count;
	}

	return count;
};

static u32 sxe_hw_regs_group_read(struct sxe_hw *hw,
				const struct sxe_reg_info *regs,
				u32 *reg_buf)
{
	u32 j, i = 0;
	int count = 0;

	while (regs[i].count) {
		for (j = 0; j < regs[i].count; j++) {
			reg_buf[count + j] = SXE_REG_READ(hw,
					regs[i].addr + j * regs[i].stride);
			LOG_INFO("regs= %s, regs_addr=%x, regs_value=%04x\n",
				regs[i].name , regs[i].addr, reg_buf[count + j]);
		}

		i++;
		count += j;
	}

	return count;
};

u32 sxe_hw_all_regs_group_num_get(void)
{
	u32 i = 0;
	u32 count = 0;
	const struct sxe_reg_info *reg_group;
	const struct sxe_reg_info **reg_set = sxe_regs_group;

	while ((reg_group = reg_set[i++])) {
		count += sxe_regs_group_count(reg_group);
	}

	return count;
}

void sxe_hw_all_regs_group_read(struct sxe_hw *hw, u32 *data)
{
	u32 count = 0, i = 0;
	const struct sxe_reg_info *reg_group;
	const struct sxe_reg_info **reg_set = sxe_regs_group;

	while ((reg_group = reg_set[i++])) {
		count += sxe_hw_regs_group_read(hw, reg_group, &data[count]);
	}

	LOG_INFO("read regs cnt=%u, regs num=%u\n",
				count, sxe_hw_all_regs_group_num_get());

	return;
}

static void sxe_hw_default_pool_configure(struct sxe_hw *hw,
						u8 default_pool_enabled,
						u8 default_pool_idx)
{
	u32 vt_ctl;

	vt_ctl = SXE_VT_CTL_VT_ENABLE | SXE_VT_CTL_REPLEN;
	if (default_pool_enabled) {
		vt_ctl |= (default_pool_idx << SXE_VT_CTL_POOL_SHIFT);
	} else {
		vt_ctl |= SXE_VT_CTL_DIS_DEFPL;
	}

	SXE_REG_WRITE(hw, SXE_VT_CTL, vt_ctl);
	return;
}

void sxe_hw_dcb_vmdq_default_pool_configure(struct sxe_hw *hw,
						u8 default_pool_enabled,
						u8 default_pool_idx)
{
	sxe_hw_default_pool_configure(hw, default_pool_enabled, default_pool_idx);
	return;
}

u32 sxe_hw_ring_irq_switch_get(struct sxe_hw *hw, u8 idx)
{
	u32 mask;

	if (idx == 0) {
		mask = SXE_REG_READ(hw, SXE_EIMS_EX(0));
	} else {
		mask = SXE_REG_READ(hw, SXE_EIMS_EX(1));
	}

	return mask;
}

void sxe_hw_ring_irq_switch_set(struct sxe_hw *hw, u8 idx, u32 value)
{
	if (idx == 0) {
		SXE_REG_WRITE(hw, SXE_EIMS_EX(0), value);
	} else {
		SXE_REG_WRITE(hw, SXE_EIMS_EX(1), value);
	}

	return;
}

void sxe_hw_dcb_vmdq_up_2_tc_configure(struct sxe_hw *hw,
						u8 *tc_arr)
{
	u32 up2tc;
	u8 i;

	up2tc = 0;
	for (i = 0; i < MAX_USER_PRIORITY; i++) {
		up2tc |= ((tc_arr[i] & 0x07) << (i * 3));
	}

	SXE_REG_WRITE(hw, SXE_RTRUP2TC, up2tc);

	return;
}

u32 sxe_hw_uta_hash_table_get(struct sxe_hw *hw, u8 reg_idx)
{
	return SXE_REG_READ(hw, SXE_UTA(reg_idx));
}

void sxe_hw_uta_hash_table_set(struct sxe_hw *hw,
				u8 reg_idx, u32 value)
{
	SXE_REG_WRITE(hw, SXE_UTA(reg_idx), value);

	return;
}

u32 sxe_hw_vlan_type_get(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_VLNCTRL);
}

void sxe_hw_vlan_type_set(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_VLNCTRL, value);
	return;
}

void sxe_hw_dcb_vmdq_vlan_configure(struct sxe_hw *hw,
						u8 num_pools)
{
	u32 vlanctrl;
	u8 i;

	vlanctrl = SXE_REG_READ(hw, SXE_VLNCTRL);
	vlanctrl |= SXE_VLNCTRL_VFE;
	SXE_REG_WRITE(hw, SXE_VLNCTRL, vlanctrl);

	for (i = 0; i < SXE_VFT_TBL_SIZE; i++) {
		SXE_REG_WRITE(hw, SXE_VFTA(i), 0xFFFFFFFF);
	}

	SXE_REG_WRITE(hw, SXE_VFRE(0),
			num_pools == RTE_ETH_16_POOLS ? 0xFFFF : 0xFFFFFFFF);

	SXE_REG_WRITE(hw, SXE_MPSAR_LOW(0), 0xFFFFFFFF);
	SXE_REG_WRITE(hw, SXE_MPSAR_HIGH(0), 0xFFFFFFFF);

	return;
}

void sxe_hw_vlan_ext_type_set(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_EXVET, value);
	return;
}

u32 sxe_hw_txctl_vlan_type_get(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_DMATXCTL);
}

void sxe_hw_txctl_vlan_type_set(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_DMATXCTL, value);
	return;
}

u32 sxe_hw_ext_vlan_get(struct sxe_hw *hw)
{
	return SXE_REG_READ(hw, SXE_CTRL_EXT);
}

void sxe_hw_ext_vlan_set(struct sxe_hw *hw, u32 value)
{
	SXE_REG_WRITE(hw, SXE_CTRL_EXT, value);
	return;
}

void sxe_hw_rxq_stat_map_set(struct sxe_hw *hw, u8 idx, u32 value)
{
	SXE_REG_WRITE(hw, SXE_RQSMR(idx), value);
	return;
}

void sxe_hw_dcb_vmdq_pool_configure(struct sxe_hw *hw,
						u8 pool_idx, u16 vlan_id,
						u64 pools_map)
{
	SXE_REG_WRITE(hw, SXE_VLVF(pool_idx), (SXE_VLVF_VIEN |
			(vlan_id & 0xFFF)));

	SXE_REG_WRITE(hw, SXE_VLVFB(pool_idx * 2), pools_map);

	return;
}

void sxe_hw_txq_stat_map_set(struct sxe_hw *hw, u8 idx, u32 value)
{
	SXE_REG_WRITE(hw, SXE_TQSM(idx), value);
	return;
}

void sxe_hw_dcb_rx_configure(struct sxe_hw *hw, bool is_vt_on,
					u8 sriov_active, u8 tc_num)
{
	u32 reg;
	u32 vlanctrl;
	u8 i;
	u32 q;

	reg = SXE_RTRPCS_RRM | SXE_RTRPCS_RAC | SXE_RTRPCS_ARBDIS;
	SXE_REG_WRITE(hw, SXE_RTRPCS, reg);

	reg = SXE_REG_READ(hw, SXE_MRQC);
	if (tc_num == 4) {
		if (is_vt_on) {
			reg = (reg & ~SXE_MRQC_MRQE_MASK) |
				SXE_MRQC_VMDQRT4TCEN;
		} else {
			SXE_REG_WRITE(hw, SXE_VT_CTL, 0);
			reg = (reg & ~SXE_MRQC_MRQE_MASK) |
				SXE_MRQC_RTRSS4TCEN;
		}
	}

	if (tc_num == 8) {
		if (is_vt_on) {
			reg = (reg & ~SXE_MRQC_MRQE_MASK) |
				SXE_MRQC_VMDQRT8TCEN;
		} else {
			SXE_REG_WRITE(hw, SXE_VT_CTL, 0);
			reg = (reg & ~SXE_MRQC_MRQE_MASK) |
				SXE_MRQC_RTRSS8TCEN;
		}
	}

	SXE_REG_WRITE(hw, SXE_MRQC, reg);

	if (sriov_active == 0) {
		for (q = 0; q < SXE_HW_TXRX_RING_NUM_MAX; q++) {
			SXE_REG_WRITE(hw, SXE_QDE,
				(SXE_QDE_WRITE |
				 (q << SXE_QDE_IDX_SHIFT)));
		}
	} else {
		for (q = 0; q < SXE_HW_TXRX_RING_NUM_MAX; q++) {
			SXE_REG_WRITE(hw, SXE_QDE,
				(SXE_QDE_WRITE |
				 (q << SXE_QDE_IDX_SHIFT) |
				 SXE_QDE_ENABLE));
		}
	}

	vlanctrl = SXE_REG_READ(hw, SXE_VLNCTRL);
	vlanctrl |= SXE_VLNCTRL_VFE;
	SXE_REG_WRITE(hw, SXE_VLNCTRL, vlanctrl);

	for (i = 0; i < SXE_VFT_TBL_SIZE; i++) {
		SXE_REG_WRITE(hw, SXE_VFTA(i), 0xFFFFFFFF);
	}

	reg = SXE_RTRPCS_RRM | SXE_RTRPCS_RAC;
	SXE_REG_WRITE(hw, SXE_RTRPCS, reg);

	return;
}

void sxe_hw_fc_status_get(struct sxe_hw *hw,
					bool *rx_pause_on, bool *tx_pause_on)
{
	u32 flctrl;

	flctrl = SXE_REG_READ(hw, SXE_FLCTRL);
	if (flctrl & (SXE_FCTRL_RFCE_PFC_EN | SXE_FCTRL_RFCE_LFC_EN)) {
		*rx_pause_on = true;
	} else {
		*rx_pause_on = false;
	}

	if (flctrl & (SXE_FCTRL_TFCE_PFC_EN | SXE_FCTRL_TFCE_LFC_EN)) {
		*tx_pause_on = true;
	} else {
		*tx_pause_on = false;
	}

	return;
}

void sxe_hw_fc_base_init(struct sxe_hw *hw)
{
	u8 i;

	hw->fc.requested_mode = SXE_FC_NONE;
	hw->fc.current_mode = SXE_FC_NONE;
	hw->fc.pause_time = SXE_DEFAULT_FCPAUSE;
	hw->fc.disable_fc_autoneg = false;

	for (i = 0; i < MAX_TRAFFIC_CLASS; i++) {
		hw->fc.low_water[i]  = SXE_FC_DEFAULT_LOW_WATER_MARK;
		hw->fc.high_water[i] = SXE_FC_DEFAULT_HIGH_WATER_MARK;
	}

	hw->fc.send_xon = 1;
	return;
}

u32 sxe_hw_fc_tc_high_water_mark_get(struct sxe_hw *hw, u8 tc_idx)
{
	return hw->fc.high_water[tc_idx];
}

u32 sxe_hw_fc_tc_low_water_mark_get(struct sxe_hw *hw, u8 tc_idx)
{
	return hw->fc.low_water[tc_idx];
}

u16 sxe_hw_fc_send_xon_get(struct sxe_hw *hw)
{
	return hw->fc.send_xon;
}

void sxe_hw_fc_send_xon_set(struct sxe_hw *hw, u16 send_xon)
{
	hw->fc.send_xon = send_xon;
	return;
}

u16 sxe_hw_fc_pause_time_get(struct sxe_hw *hw)
{
	return hw->fc.pause_time;
}

void sxe_hw_fc_pause_time_set(struct sxe_hw *hw, u16 pause_time)
{
	hw->fc.pause_time = pause_time;
	return;
}

void sxe_hw_dcb_tx_configure(struct sxe_hw *hw, bool is_vt_on, u8 tc_num)
{
	u32 reg;

	reg = SXE_REG_READ(hw, SXE_RTTDCS);
	reg |= SXE_RTTDCS_ARBDIS;
	SXE_REG_WRITE(hw, SXE_RTTDCS, reg);

	if (tc_num == 8) {
		reg = SXE_MTQC_RT_ENA | SXE_MTQC_8TC_8TQ;
	} else {
		reg = SXE_MTQC_RT_ENA | SXE_MTQC_4TC_4TQ;
	}

	if (is_vt_on) {
		reg |= SXE_MTQC_VT_ENA;
	}

	SXE_REG_WRITE(hw, SXE_MTQC, reg);

	reg = SXE_REG_READ(hw, SXE_RTTDCS);
	reg &= ~SXE_RTTDCS_ARBDIS;
	SXE_REG_WRITE(hw, SXE_RTTDCS, reg);


	return;
}

void sxe_hw_rx_ip_checksum_offload_switch(struct sxe_hw *hw,
							bool is_on)
{
	u32 rxcsum;

	rxcsum = SXE_REG_READ(hw, SXE_RXCSUM);
	if (is_on) {
		rxcsum |= SXE_RXCSUM_IPPCSE;
	} else {
		rxcsum &= ~SXE_RXCSUM_IPPCSE;
	}

	SXE_REG_WRITE(hw, SXE_RXCSUM, rxcsum);

	return;
}

void sxe_hw_rss_cap_switch(struct sxe_hw *hw, bool is_on)
{
	u32 mrqc = SXE_REG_READ(hw, SXE_MRQC);
	if (is_on) {
		mrqc |= SXE_MRQC_RSSEN;
	} else {
		mrqc &= ~SXE_MRQC_RSSEN;
	}

	SXE_REG_WRITE(hw, SXE_MRQC, mrqc);

	return;
}

void sxe_hw_pool_xmit_enable(struct sxe_hw *hw, u16 reg_idx, u8 pool_num)
{
	SXE_REG_WRITE(hw, SXE_VFTE(reg_idx),
		pool_num == RTE_ETH_16_POOLS ? 0xFFFF : 0xFFFFFFFF);
	return;
}

void sxe_hw_rss_field_set(struct sxe_hw *hw, u32 rss_field)
{
	u32 mrqc = SXE_REG_READ(hw, SXE_MRQC);

	mrqc &= ~SXE_RSS_FIELD_MASK;
	mrqc |= rss_field;
	SXE_REG_WRITE(hw, SXE_MRQC, mrqc);

	return;
}

static void sxe_hw_dcb_4tc_vmdq_off_stats_configure(struct sxe_hw *hw)
{
	u32 reg;
	u8  i;

	for (i = 0; i < 32; i++) {
		if (i % 8 > 3) {
			continue;
		}

		reg = 0x01010101 * (i / 8);
		SXE_REG_WRITE(hw, SXE_RQSMR(i), reg);
	}
	for (i = 0; i < 32; i++) {
		if (i < 16) {
			reg = 0x00000000;
		} else if (i < 24) {
			reg = 0x01010101;
		} else if (i < 28) {
			reg = 0x02020202;
		} else {
			reg = 0x03030303;
		}

		SXE_REG_WRITE(hw, SXE_TQSM(i), reg);
	}

	return;
}

static void sxe_hw_dcb_4tc_vmdq_on_stats_configure(struct sxe_hw *hw)
{
	u8  i;

	for (i = 0; i < 32; i++) {
		SXE_REG_WRITE(hw, SXE_RQSMR(i), 0x03020100);
	}


	for (i = 0; i < 32; i++) {
		SXE_REG_WRITE(hw, SXE_TQSM(i), 0x03020100);
	}

	return;
}

void sxe_hw_rss_redir_tbl_set_by_idx(struct sxe_hw *hw,
						u16 reg_idx, u32 value)
{
	return sxe_hw_rss_redir_tbl_reg_write(hw, reg_idx, value);
}

static u32 sxe_hw_rss_redir_tbl_reg_read(struct sxe_hw *hw, u16 reg_idx)
{
	return SXE_REG_READ(hw, SXE_RETA(reg_idx >> 2));
}

u32 sxe_hw_rss_redir_tbl_get_by_idx(struct sxe_hw *hw, u16 reg_idx)
{
	return sxe_hw_rss_redir_tbl_reg_read(hw, reg_idx);
}

void sxe_hw_ptp_time_inc_stop(struct sxe_hw *hw)
{
	SXE_REG_WRITE(hw, SXE_TIMINC, 0);
	return;
}

void sxe_hw_dcb_tc_stats_configure(struct sxe_hw *hw,
					u8 tc_num, bool vmdq_active)
{
	if (tc_num == 8 && vmdq_active == false) {
		sxe_hw_dcb_8tc_vmdq_off_stats_configure(hw);
	} else if (tc_num == 4 && vmdq_active == false) {
		sxe_hw_dcb_4tc_vmdq_off_stats_configure(hw);
	} else if (tc_num == 4 && vmdq_active == true) {
		sxe_hw_dcb_4tc_vmdq_on_stats_configure(hw);
	}

	return;
}

void sxe_hw_ptp_timestamp_disable(struct sxe_hw *hw)
{
	SXE_REG_WRITE(hw, SXE_TSYNCTXCTL,
			(SXE_REG_READ(hw, SXE_TSYNCTXCTL) &
			~SXE_TSYNCTXCTL_TEN));

	SXE_REG_WRITE(hw, SXE_TSYNCRXCTL,
			(SXE_REG_READ(hw, SXE_TSYNCRXCTL) &
			~SXE_TSYNCRXCTL_REN));
	SXE_WRITE_FLUSH(hw);

	return;
}

void sxe_hw_mac_pool_clear(struct sxe_hw *hw, u8 rar_idx)
{
	struct sxe_adapter *adapter = hw->adapter;

	if (rar_idx > SXE_UC_ENTRY_NUM_MAX) {
		LOG_ERROR_BDF("rar_idx:%d invalid.(err:%d)\n",
			  rar_idx, SXE_ERR_PARAM);
		goto l_end;
	}

	SXE_REG_WRITE(hw, SXE_MPSAR_LOW(rar_idx), 0);
	SXE_REG_WRITE(hw, SXE_MPSAR_HIGH(rar_idx), 0);

l_end:
	return;
}

void sxe_hw_vmdq_mq_configure(struct sxe_hw *hw)
{
	u32 mrqc;

	mrqc = SXE_MRQC_VMDQEN;
	SXE_REG_WRITE(hw, SXE_MRQC, mrqc);

	return;
}

void sxe_hw_vmdq_default_pool_configure(struct sxe_hw *hw,
						u8 default_pool_enabled,
						u8 default_pool_idx)
{
	sxe_hw_default_pool_configure(hw, default_pool_enabled, default_pool_idx);
	return;
}

void sxe_hw_vmdq_vlan_configure(struct sxe_hw *hw,
						u8 num_pools, u32 rx_mode)
{
	u32 vlanctrl;
	u8 i;

	vlanctrl = SXE_REG_READ(hw, SXE_VLNCTRL);
	vlanctrl |= SXE_VLNCTRL_VFE;
	SXE_REG_WRITE(hw, SXE_VLNCTRL, vlanctrl);

	for (i = 0; i < SXE_VFT_TBL_SIZE; i++) {
		SXE_REG_WRITE(hw, SXE_VFTA(i), 0xFFFFFFFF);
	}

	SXE_REG_WRITE(hw, SXE_VFRE(0), 0xFFFFFFFF);
	if (num_pools == RTE_ETH_64_POOLS) {
		SXE_REG_WRITE(hw, SXE_VFRE(1), 0xFFFFFFFF);
	}

	for (i = 0; i < num_pools; i++) {
		SXE_REG_WRITE(hw, SXE_VMOLR(i), rx_mode);
	}

	SXE_REG_WRITE(hw, SXE_MPSAR_LOW(0), 0xFFFFFFFF);
	SXE_REG_WRITE(hw, SXE_MPSAR_HIGH(0), 0xFFFFFFFF);

	SXE_WRITE_FLUSH(hw);
	return;
}

u32 sxe_hw_pcie_vt_mode_get(struct sxe_hw *hw)
{

	return SXE_REG_READ(hw, SXE_GCR_EXT);
}

void sxe_rx_fc_threshold_set(struct sxe_hw *hw)
{
	u8 i;
	u32 high;

	for (i = 0; i < SXE_TRAFFIC_CLASS_MAX; i++) {
		SXE_REG_WRITE(hw, SXE_FCRTL(i), 0);
		high = SXE_REG_READ(hw, SXE_RXPBSIZE(i)) - 32;
		SXE_REG_WRITE(hw, SXE_FCRTH(i), high);
	}

	return;
}

void sxe_hw_vmdq_pool_configure(struct sxe_hw *hw,
						u8 pool_idx, u16 vlan_id,
						u64 pools_map)
{
	SXE_REG_WRITE(hw, SXE_VLVF(pool_idx), (SXE_VLVF_VIEN |
			(vlan_id & SXE_RXD_VLAN_ID_MASK)));

	if (((pools_map >> 32) & 0xFFFFFFFF) == 0) {
		SXE_REG_WRITE(hw, SXE_VLVFB(pool_idx * 2),
			(pools_map & 0xFFFFFFFF));
	} else {
		SXE_REG_WRITE(hw, SXE_VLVFB((pool_idx * 2 + 1)),
			((pools_map >> 32) & 0xFFFFFFFF));
	}

	SXE_WRITE_FLUSH(hw);
	return;
}

void sxe_hw_vmdq_loopback_configure(struct sxe_hw *hw)
{
	u8 i;
	SXE_REG_WRITE(hw, SXE_PFDTXGSWC, SXE_PFDTXGSWC_VT_LBEN);
	for (i = 0; i < SXE_VMTXSW_REGISTER_COUNT; i++) {
		SXE_REG_WRITE(hw, SXE_VMTXSW(i), 0xFFFFFFFF);
	}

	SXE_WRITE_FLUSH(hw);
	return;
}

void sxe_hw_tx_multi_queue_configure(struct sxe_hw *hw,
				bool vmdq_enable, bool sriov_enable, u16 pools_num)
{
	u32 mtqc;

	sxe_hw_dcb_arbiter_set(hw, false);

	if (sriov_enable) {
		switch (pools_num) {
		case RTE_ETH_64_POOLS:
			mtqc = SXE_MTQC_VT_ENA | SXE_MTQC_64VF;
			break;
		case RTE_ETH_32_POOLS:
			mtqc = SXE_MTQC_VT_ENA | SXE_MTQC_32VF;
			break;
		case RTE_ETH_16_POOLS:
			mtqc = SXE_MTQC_VT_ENA | SXE_MTQC_RT_ENA |
				SXE_MTQC_8TC_8TQ;
			break;
		default:
			mtqc = SXE_MTQC_64Q_1PB;
		}
	} else {
		if (vmdq_enable) {
			u8 queue_idx;
			SXE_REG_WRITE(hw, SXE_VFTE(0), UINT32_MAX);
			SXE_REG_WRITE(hw, SXE_VFTE(1), UINT32_MAX);

			for (queue_idx = 0; queue_idx < SXE_HW_TXRX_RING_NUM_MAX;
			    queue_idx++) {
				SXE_REG_WRITE(hw, SXE_QDE,
					(SXE_QDE_WRITE |
					(queue_idx << SXE_QDE_IDX_SHIFT)));
			}

			mtqc = SXE_MTQC_VT_ENA | SXE_MTQC_64VF;
		} else {
			mtqc = SXE_MTQC_64Q_1PB;
		}
	}

	SXE_REG_WRITE(hw, SXE_MTQC, mtqc);

	sxe_hw_dcb_arbiter_set(hw, true);

	return;
}

void sxe_hw_vf_queue_drop_enable(struct sxe_hw *hw, u8 vf_idx,
					u8 ring_per_pool)
{
	u32 value;
	u8 i;

	for (i = (vf_idx * ring_per_pool); i < ((vf_idx + 1) * ring_per_pool); i++)
	{
		value = SXE_QDE_ENABLE | SXE_QDE_WRITE;
		SXE_WRITE_FLUSH(hw);

		value |= i << SXE_QDE_IDX_SHIFT;

		SXE_REG_WRITE(hw, SXE_QDE, value);
	}

	return;
}

bool sxe_hw_vt_status(struct sxe_hw *hw)
{
	bool ret;
	u32 vt_ctl = SXE_REG_READ(hw, SXE_VT_CTL);

	if (vt_ctl & SXE_VMD_CTL_POOL_EN) {
		ret = true;
	} else {
		ret = false;
	}

	return ret;
}

void sxe_hw_mirror_ctl_set(struct sxe_hw *hw, u8 rule_id,
				    u8 mirror_type, u8 dst_pool, bool on)
{
	u32 mr_ctl;

	mr_ctl = SXE_REG_READ(hw, SXE_MRCTL(rule_id));

	if (on) {
		mr_ctl |= mirror_type;
		mr_ctl &= SXE_MR_TYPE_MASK;
		mr_ctl |= dst_pool << SXE_MR_DST_POOL_OFFSET;
	} else {
		mr_ctl &= ~(mirror_type & SXE_MR_TYPE_MASK);
	}

	SXE_REG_WRITE(hw, SXE_MRCTL(rule_id), mr_ctl);

	return;
}

void sxe_hw_mirror_virtual_pool_set(struct sxe_hw *hw, u8 rule_id,u32 lsb, u32 msb)
{
	SXE_REG_WRITE(hw, SXE_VMRVM(rule_id), lsb);
	SXE_REG_WRITE(hw, SXE_VMRVM(rule_id  + SXE_MR_VIRTUAL_POOL_MSB_REG_OFFSET), msb);

	return;
}

void sxe_hw_mirror_vlan_set(struct sxe_hw *hw, u8 rule_id,u32 lsb, u32 msb)
{
	SXE_REG_WRITE(hw, SXE_VMRVLAN(rule_id), lsb);
	SXE_REG_WRITE(hw, SXE_VMRVLAN(rule_id  + SXE_MR_VLAN_MSB_REG_OFFSET), msb);

	return;
}

void sxe_hw_mirror_rule_clear(struct sxe_hw *hw, u8 rule_id)
{
	SXE_REG_WRITE(hw, SXE_MRCTL(rule_id), 0);

	SXE_REG_WRITE(hw, SXE_VMRVLAN(rule_id), 0);
	SXE_REG_WRITE(hw, SXE_VMRVLAN(rule_id  + SXE_MR_VLAN_MSB_REG_OFFSET), 0);

	SXE_REG_WRITE(hw, SXE_VMRVM(rule_id), 0);
	SXE_REG_WRITE(hw, SXE_VMRVM(rule_id  + SXE_MR_VIRTUAL_POOL_MSB_REG_OFFSET), 0);

	return;
}

#if defined SXE_DPDK_L4_FEATURES && defined SXE_DPDK_FILTER_CTRL
void sxe_hw_fivetuple_filter_add(struct rte_eth_dev *dev,
					struct sxe_fivetuple_node_info *filter)
{
	struct sxe_adapter *adapter = dev->data->dev_private;
	struct sxe_hw *hw = &adapter->hw;
	u16 i;
	u32 ftqf, sdpqf;
	u32 l34timir = 0;
	u8 mask = 0xff;

	i = filter->index;

	sdpqf = (u32)(filter->filter_info.dst_port << SXE_SDPQF_DSTPORT_SHIFT);
	sdpqf = sdpqf | (filter->filter_info.src_port & SXE_SDPQF_SRCPORT);

	ftqf = (u32)(filter->filter_info.protocol & SXE_FTQF_PROTOCOL_MASK);
	ftqf |= (u32)((filter->filter_info.priority &
			SXE_FTQF_PRIORITY_MASK) << SXE_FTQF_PRIORITY_SHIFT);

	if (filter->filter_info.src_ip_mask == 0) {
		mask &= SXE_FTQF_SOURCE_ADDR_MASK;
	}
	if (filter->filter_info.dst_ip_mask == 0) {
		mask &= SXE_FTQF_DEST_ADDR_MASK;
	}
	if (filter->filter_info.src_port_mask == 0) {
		mask &= SXE_FTQF_SOURCE_PORT_MASK;
	}
	if (filter->filter_info.dst_port_mask == 0) {
		mask &= SXE_FTQF_DEST_PORT_MASK;
	}
	if (filter->filter_info.proto_mask == 0) {
		mask &= SXE_FTQF_PROTOCOL_COMP_MASK;
	}
	ftqf |= mask << SXE_FTQF_5TUPLE_MASK_SHIFT;
	ftqf |= SXE_FTQF_POOL_MASK_EN;
	ftqf |= SXE_FTQF_QUEUE_ENABLE;

	LOG_DEBUG("add fivetuple filter, index[%u], src_ip[0x%x], dst_ip[0x%x]"
		"src_port[%u], dst_port[%u], ftqf[0x%x], queue[%u]", i, filter->filter_info.src_ip,
		filter->filter_info.dst_ip, filter->filter_info.src_port, filter->filter_info.dst_port,
		ftqf, filter->queue);

	SXE_REG_WRITE(hw, SXE_DAQF(i), filter->filter_info.dst_ip);
	SXE_REG_WRITE(hw, SXE_SAQF(i), filter->filter_info.src_ip);
	SXE_REG_WRITE(hw, SXE_SDPQF(i), sdpqf);
	SXE_REG_WRITE(hw, SXE_FTQF(i), ftqf);

	l34timir |= SXE_L34T_IMIR_RESERVE;
	l34timir |= (u32)(filter->queue << SXE_L34T_IMIR_QUEUE_SHIFT);
	SXE_REG_WRITE(hw, SXE_L34T_IMIR(i), l34timir);

	return;
}

void sxe_hw_fivetuple_filter_del(struct sxe_hw *hw, u16 reg_index)
{
	SXE_REG_WRITE(hw, SXE_DAQF(reg_index), 0);
	SXE_REG_WRITE(hw, SXE_SAQF(reg_index), 0);
	SXE_REG_WRITE(hw, SXE_SDPQF(reg_index), 0);
	SXE_REG_WRITE(hw, SXE_FTQF(reg_index), 0);
	SXE_REG_WRITE(hw, SXE_L34T_IMIR(reg_index), 0);

	return;
}

void sxe_hw_ethertype_filter_add(struct sxe_hw *hw,
					u8 reg_index, u16 ethertype, u16 queue)
{
	u32 etqf = 0;
	u32 etqs = 0;

	etqf = SXE_ETQF_FILTER_EN;
	etqf |= (u32)ethertype;
	etqs |= (u32)((queue << SXE_ETQS_RX_QUEUE_SHIFT) &
			SXE_ETQS_RX_QUEUE);
	etqs |= SXE_ETQS_QUEUE_EN;

	SXE_REG_WRITE(hw, SXE_ETQF(reg_index), etqf);
	SXE_REG_WRITE(hw, SXE_ETQS(reg_index), etqs);
	SXE_WRITE_FLUSH(hw);

	return;
}

void sxe_hw_ethertype_filter_del(struct sxe_hw *hw, u8 filter_type)
{
	SXE_REG_WRITE(hw, SXE_ETQF(filter_type), 0);
	SXE_REG_WRITE(hw, SXE_ETQS(filter_type), 0);
	SXE_WRITE_FLUSH(hw);

	return;
}

void sxe_hw_syn_filter_add(struct sxe_hw *hw, u16 queue, u8 priority)
{
	u32 synqf;

	synqf = (u32)(((queue << SXE_SYN_FILTER_QUEUE_SHIFT) &
			SXE_SYN_FILTER_QUEUE) | SXE_SYN_FILTER_ENABLE);

	if (priority) {
		synqf |= SXE_SYN_FILTER_SYNQFP;
	} else {
		synqf &= ~SXE_SYN_FILTER_SYNQFP;
	}

	SXE_REG_WRITE(hw, SXE_SYNQF, synqf);
	SXE_WRITE_FLUSH(hw);

	return;
}

void sxe_hw_syn_filter_del(struct sxe_hw *hw)
{
	u32 synqf;

	synqf = SXE_REG_READ(hw, SXE_SYNQF);

	synqf &= ~(SXE_SYN_FILTER_QUEUE | SXE_SYN_FILTER_ENABLE);
	SXE_REG_WRITE(hw, SXE_SYNQF, synqf);
	SXE_WRITE_FLUSH(hw);

	return;
}

void sxe_hw_fnav_rx_pkt_buf_size_reset(struct sxe_hw *hw, u32 pbsize)
{
	S32 i;

	SXE_REG_WRITE(hw, SXE_RXPBSIZE(0), (SXE_REG_READ(hw, SXE_RXPBSIZE(0)) - pbsize));
	for (i = 1; i < 8; i++) {
		SXE_REG_WRITE(hw, SXE_RXPBSIZE(i), 0);
	}

	return;
}

void sxe_hw_fnav_flex_mask_set(struct sxe_hw *hw, u16 flex_mask)
{
	u32 fnavm;

	fnavm = SXE_REG_READ(hw, SXE_FNAVM);
	if (flex_mask == UINT16_MAX) {
		fnavm &= ~SXE_FNAVM_FLEX;
	}

	SXE_REG_WRITE(hw, SXE_FNAVM, fnavm);
	return;
}

void sxe_hw_fnav_ipv6_mask_set(struct sxe_hw *hw, u16 src_mask, u16 dst_mask)
{
	u32 fnavipv6m;

	fnavipv6m = (dst_mask << 16) | src_mask;
	SXE_REG_WRITE(hw, SXE_FNAVIP6M, ~fnavipv6m);

	return;
}

s32 sxe_hw_fnav_flex_offset_set(struct sxe_hw *hw, u16 offset)
{
	u32 fnavctrl;
	s32 ret;

	fnavctrl = SXE_REG_READ(hw, SXE_FNAVCTRL);
	fnavctrl &= ~SXE_FNAVCTRL_FLEX_MASK;
	fnavctrl |= ((offset >> 1)
		<< SXE_FNAVCTRL_FLEX_SHIFT);

	SXE_REG_WRITE(hw, SXE_FNAVCTRL, fnavctrl);
	SXE_WRITE_FLUSH(hw);

	ret = sxe_hw_fnav_wait_init_done(hw);
	if (ret) {
		LOG_ERROR("flow director signature poll time exceeded!\n");
	}

	return ret;
}
#endif

#if defined SXE_DPDK_L4_FEATURES && defined SXE_DPDK_MACSEC
static void sxe_macsec_stop_data(struct sxe_hw *hw, bool link)
{
	u32 t_rdy, r_rdy;
	u32 limit;
	u32 reg;

	reg = SXE_REG_READ(hw, SXE_SECTXCTRL);
	reg |= SXE_SECTXCTRL_TX_DIS;
	SXE_REG_WRITE(hw, SXE_SECTXCTRL, reg);

	reg = SXE_REG_READ(hw, SXE_SECRXCTRL);
	reg |= SXE_SECRXCTRL_RX_DIS;
	SXE_REG_WRITE(hw, SXE_SECRXCTRL, reg);
	SXE_WRITE_FLUSH(hw);

	t_rdy = SXE_REG_READ(hw, SXE_SECTXSTAT) &
		SXE_SECTXSTAT_SECTX_RDY;
	r_rdy = SXE_REG_READ(hw, SXE_SECRXSTAT) &
		SXE_SECRXSTAT_SECRX_RDY;
	if (t_rdy && r_rdy)
		return;

	if (!link) {
		SXE_REG_WRITE(hw, SXE_LPBKCTRL, 0x1);

		SXE_WRITE_FLUSH(hw);
		mdelay(3);
	}

	limit = 20;
	do {
		mdelay(10);
		t_rdy = SXE_REG_READ(hw, SXE_SECTXSTAT) &
			SXE_SECTXSTAT_SECTX_RDY;
		r_rdy = SXE_REG_READ(hw, SXE_SECRXSTAT) &
			SXE_SECRXSTAT_SECRX_RDY;
	} while (!(t_rdy && r_rdy) && limit--);

	if (!link) {
		SXE_REG_WRITE(hw, SXE_LPBKCTRL, 0x0);
		SXE_WRITE_FLUSH(hw);
	}

	return;
}
void sxe_hw_rx_queue_mode_set(struct sxe_hw *hw, u32 mrqc)
{
	SXE_REG_WRITE(hw, SXE_MRQC, mrqc);

	return;
}

void sxe_hw_macsec_enable(struct sxe_hw *hw, bool is_up, u32 tx_mode,
				u32 rx_mode, u32 pn_trh)
{
	u32 reg;

	sxe_macsec_stop_data(hw, is_up);

	reg = SXE_REG_READ(hw, SXE_SECTXCTRL);
	reg &= ~SXE_SECTXCTRL_SECTX_DIS;
	reg &= ~SXE_SECTXCTRL_STORE_FORWARD;
	SXE_REG_WRITE(hw, SXE_SECTXCTRL, reg);

	SXE_REG_WRITE(hw, SXE_SECTXBUFFAF, 0x250);

	reg = SXE_REG_READ(hw, SXE_SECTXMINIFG);
	reg = (reg & 0xfffffff0) | 0x3;
	SXE_REG_WRITE(hw, SXE_SECTXMINIFG, reg);

	reg = SXE_REG_READ(hw, SXE_SECRXCTRL);
	reg &= ~SXE_SECRXCTRL_SECRX_DIS;
	reg |= SXE_SECRXCTRL_RP;
	SXE_REG_WRITE(hw, SXE_SECRXCTRL, reg);

	reg = tx_mode & SXE_LSECTXCTRL_EN_MASK;
	reg |= SXE_LSECTXCTRL_AISCI;
	reg &= ~SXE_LSECTXCTRL_PNTHRSH_MASK;
	reg |= (pn_trh << SXE_LSECTXCTRL_PNTHRSH_SHIFT);
	SXE_REG_WRITE(hw, SXE_LSECTXCTRL, reg);

	reg = (rx_mode << SXE_LSECRXCTRL_EN_SHIFT) & SXE_LSECRXCTRL_EN_MASK;
	reg |= SXE_LSECRXCTRL_RP;
	reg |= SXE_LSECRXCTRL_DROP_EN;
	SXE_REG_WRITE(hw, SXE_LSECRXCTRL, reg);

	reg = SXE_REG_READ(hw, SXE_SECTXCTRL);
	reg &= ~SXE_SECTXCTRL_TX_DIS;
	SXE_REG_WRITE(hw, SXE_SECTXCTRL, reg);

	reg = SXE_REG_READ(hw, SXE_SECRXCTRL);
	reg &= ~SXE_SECRXCTRL_RX_DIS;
	SXE_REG_WRITE(hw, SXE_SECRXCTRL, reg);

	SXE_WRITE_FLUSH(hw);

	return;
}

void sxe_hw_macsec_disable(struct sxe_hw *hw, bool is_up)
{
	u32 reg;

	sxe_macsec_stop_data(hw, is_up);

	reg = SXE_REG_READ(hw, SXE_SECTXCTRL);
	reg |= SXE_SECTXCTRL_SECTX_DIS;
	reg &= ~SXE_SECTXCTRL_STORE_FORWARD;
	SXE_REG_WRITE(hw, SXE_SECTXCTRL, reg);

	reg = SXE_REG_READ(hw, SXE_SECRXCTRL);
	reg |= SXE_SECRXCTRL_SECRX_DIS;
	SXE_REG_WRITE(hw, SXE_SECRXCTRL, reg);

	SXE_REG_WRITE(hw, SXE_SECTXBUFFAF, 0x250);

	reg = SXE_REG_READ(hw, SXE_SECTXMINIFG);
	reg = (reg & 0xfffffff0) | 0x1;
	SXE_REG_WRITE(hw, SXE_SECTXMINIFG, reg);

	SXE_REG_WRITE(hw, SXE_SECTXCTRL, SXE_SECTXCTRL_SECTX_DIS);
	SXE_REG_WRITE(hw, SXE_SECRXCTRL, SXE_SECRXCTRL_SECRX_DIS);

	SXE_WRITE_FLUSH(hw);
	return;
}

void sxe_hw_macsec_txsc_set(struct sxe_hw *hw, u32 scl, u32 sch)
{
	SXE_REG_WRITE(hw, SXE_LSECTXSCL, scl);
	SXE_REG_WRITE(hw, SXE_LSECTXSCH, sch);

	SXE_WRITE_FLUSH(hw);
	return;
}

void sxe_hw_macsec_rxsc_set(struct sxe_hw *hw, u32 scl, u32 sch, u16 pi)
{
	u32 reg = sch;

	SXE_REG_WRITE(hw, SXE_LSECRXSCL, scl);

	reg |= (pi << SXE_LSECRXSCH_PI_SHIFT) & SXE_LSECRXSCH_PI_MASK;
	SXE_REG_WRITE(hw, SXE_LSECRXSCH, reg);

	SXE_WRITE_FLUSH(hw);
	return;

}

void sxe_hw_macsec_tx_sa_configure(struct sxe_hw *hw, u8 sa_idx,
				u8 an, u32 pn, u32 *keys)
{
	u32 reg;
	u8 i;

	reg = SXE_REG_READ(hw, SXE_LSECTXSA);
	reg &= ~SXE_LSECTXSA_SELSA;
	reg |= (sa_idx << SXE_LSECTXSA_SELSA_SHIFT) & SXE_LSECTXSA_SELSA;
	SXE_REG_WRITE(hw, SXE_LSECTXSA, reg);
	SXE_WRITE_FLUSH(hw);

	SXE_REG_WRITE(hw, SXE_LSECTXPN(sa_idx), pn);
	for (i = 0; i < 4; i++) {
		SXE_REG_WRITE(hw, SXE_LSECTXKEY(sa_idx, i), keys[i]);
	}
	SXE_WRITE_FLUSH(hw);

	reg = SXE_REG_READ(hw, SXE_LSECTXSA);
	if (sa_idx == 0) {
		reg &= ~SXE_LSECTXSA_AN0_MASK;
		reg |= (an << SXE_LSECTXSA_AN0_SHIFT) & SXE_LSECTXSA_AN0_MASK;
		reg &= ~SXE_LSECTXSA_SELSA;
		SXE_REG_WRITE(hw, SXE_LSECTXSA, reg);
	} else if (sa_idx == 1) {
		reg &= ~SXE_LSECTXSA_AN1_MASK;
		reg |= (an << SXE_LSECTXSA_AN1_SHIFT) & SXE_LSECTXSA_AN1_MASK;
		reg |= SXE_LSECTXSA_SELSA;
		SXE_REG_WRITE(hw, SXE_LSECTXSA, reg);
	}

	SXE_WRITE_FLUSH(hw);
	return;
}

void sxe_hw_macsec_rx_sa_configure(struct sxe_hw *hw, u8 sa_idx,
				u8 an, u32 pn, u32 *keys)
{
	u32 reg;
	u8 i;

	reg = SXE_REG_READ(hw, SXE_LSECRXSA(sa_idx));
	reg &= ~SXE_LSECRXSA_SAV;
	reg |= (0 << SXE_LSECRXSA_SAV_SHIFT) & SXE_LSECRXSA_SAV;

	SXE_REG_WRITE(hw, SXE_LSECRXSA(sa_idx), reg);

	SXE_WRITE_FLUSH(hw);

	SXE_REG_WRITE(hw, SXE_LSECRXPN(sa_idx), pn);

	for (i = 0; i < 4; i++) {
		SXE_REG_WRITE(hw, SXE_LSECRXKEY(sa_idx, i), keys[i]);
	}
	SXE_WRITE_FLUSH(hw);

	reg = ((an << SXE_LSECRXSA_AN_SHIFT) & SXE_LSECRXSA_AN_MASK) | SXE_LSECRXSA_SAV;
	SXE_REG_WRITE(hw, SXE_LSECRXSA(sa_idx), reg);
	SXE_WRITE_FLUSH(hw);
	return;
}
				
#endif 
#endif 
