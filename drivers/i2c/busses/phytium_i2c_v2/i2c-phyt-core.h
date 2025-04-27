/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Phytium I2C adapter driver.
 *
 * Copyright (C) 2023-2024, Phytium Technology Co., Ltd.
 */
#ifndef I2C_PHYT_CORE_H__
#define I2C_PHYT_CORE_H__

#include <linux/i2c-smbus.h>
#include <linux/i2c.h>
#include <linux/pm_qos.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>

#define I2C_PHYTIUM_V2_DRV_VERSION		"1.0.1"

#define FT_I2C_MSG_UNIT_SIZE			10
#define FT_I2C_DATA_RESV_LEN			2
#define FT_I2C_SHMEM_RX_ADDR_OFFSET		384
#define FT_I2C_MSG_CMDDATA_SIZE			64
#define FT_I2C_IN_INTERRUPT_MODE		0
#define FT_I2C_ENABLE_INTERRUPT			1
#define FT_I2C_REGFILE_AP2RV_INTR_MASK		0x20
#define FT_I2C_REGFILE_AP2RV_INTR_STATE		0x24
#define FT_I2C_REGFILE_RV2AP_INTR_MASK		0x28
#define FT_I2C_REGFILE_RV2AP_INTR_STAT		0x2C
#define FT_I2C_REGFILE_RING			0x48
#define FT_I2C_REGFILE_DEBUG			0x58
#define FT_I2C_REGFILE_RV2AP_INTR_CLEAR		0x74
#define FT_I2C_TRANS_FRAME_START		(BIT(0))
#define FT_I2C_TRANS_FRAME_END			(BIT(1))
#define FT_I2C_TRANS_FRAME_RESTART		(BIT(2))
#define FT_I2C_REGFILE_TX_RING_OFFSET		8
#define FT_I2C_REGFILE_TX_RING_MASK             GENMASK(13, 8)

#define FT_I2C_REGFILE_HEARTBIT_VAL		BIT(2)
#define FT_I2C_LOG_SIZE_LOW_SHIFT		4
#define FT_I2C_LOG_SIZE_MASK			GENMASK(7, FT_I2C_LOG_SIZE_LOW_SHIFT)
#define FT_I2C_LOG_ADDR_SHIFT			10
#define FT_I2C_LOG_ADDR_LOW_SHIFT		8
#define FT_I2C_LOG_ADDR_MASK			GENMASK(29, FT_I2C_LOG_ADDR_LOW_SHIFT)
#define FT_I2C_LOG_ADDR_LOCK_VAL		BIT(31)
#define FT_I2C_LOG_ADDR_LOG_FLAG		BIT(3)
#define FT_I2C_LOG_ADDR_LOG_ALIVE		BIT(1)
#define FT_I2C_LOG_ADDR_LOG_DEBUG		BIT(0)
#define FT_I2C_REGFILE_DISABLE_INTR_VAL		GENMASK(31, 0)
#define FT_I2C_REGFILE_ENABLE_INTR_VAL		0
#define FT_I2C_REGFILE_AP2RV_SET_INTR_VAL	BIT(4)

#define FT_I2C_RV2AP_INTR_BIT4			BIT(4)

#define FT_I2C_ADAPTER_MODULE_ON	1
#define FT_I2C_ADAPTER_MODULE_OFF	2
#define FT_I2C_ADAPTER_MODULE_RESET	3

#define FT_I2C_BUS_SPEED_STANARD_MODE	1
#define FT_I2C_BUS_SPEED_FAST_MODE	2
#define FT_I2C_BUS_SPEED_HIGH_MODE	3
#define FT_I2C_BUS_SPEED_TRANS_PARAM_MODE	0
#define FT_I2C_BUS_SPEED_CALC_MODE		1

#define FT_I2C_REGFILE_TX_HEAD			0
#define FT_I2C_REGFILE_TX_TAIL			0x04
#define FT_I2C_REGFILE_HEAD			0x08
#define FT_I2C_REGFILE_TAIL			0x0C

#define FT_IC_DEFAULT_FUNCTIONALITY                      \
	(I2C_FUNC_I2C | I2C_FUNC_SMBUS_QUICK | I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_BYTE_DATA |\
	I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_BLOCK_DATA |\
	I2C_FUNC_SMBUS_I2C_BLOCK)

#define FT_I2C_VIRT_RV_SHMEM_TX_MSG_MAX_CNT	1

#define FT_I2C_MASTER_MODE_FLAG			1
#define FT_I2C_SLAVE_MODE_FLAG			0

#define FT_I2C_RESTART_FLAG			1
#define FT_I2C_NOT_RESTART_FLAG			0

#define FT_I2C_SLAVE_DATA_IN			0
#define FT_I2C_SLAVE_DATA_OUT			1

#define FT_I2C_SPEED_100K			100000
#define FT_I2C_SPEED_400K			400000
#define FT_I2C_SPEED_1000K			1000000
#define FT_I2C_SPEED_3400K			3400000

#define FT_IC_CON_MASTER			0x1
#define FT_IC_CON_SPEED_STD			0x2
#define FT_IC_CON_SPEED_FAST			0x4
#define FT_IC_CON_SPEED_HIGH			0x6
#define FT_IC_CON_SPEED_MASK			0x6
#define FT_IC_CON_10BITADDR_SLAVE		0x8
#define FT_IC_CON_10BITADDR_MASTER		0x10
#define FT_IC_CON_RESTART_EN			0x20
#define FT_IC_CON_SLAVE_DISABLE			0x40
#define FT_IC_CON_STOP_DET_IFADDRESSED		0x80
#define FT_I2C_CON_STOP_DET_IFADDR_MASK		0x80
#define FT_IC_CON_TX_EMPTY_CTRL			0x100
#define FT_IC_CON_RX_FIFO_FULL_HLD_CTRL		0x200
#define FT_I2C_CON_RX_FIFO_FULL_HLD_MASK	0x200

#define FT_IC_CON				0x0
#define FT_IC_TAR				0x4
#define FT_IC_SAR				0x8
#define FT_IC_DATA_CMD				0x10
#define FT_IC_SS_SCL_HCNT			0x14
#define FT_IC_SS_SCL_LCNT			0x18
#define FT_IC_FS_SCL_HCNT			0x1c
#define FT_IC_FS_SCL_LCNT			0x20
#define FT_IC_HS_SCL_HCNT			0x24
#define FT_IC_HS_SCL_LCNT			0x28
#define FT_IC_INTR_STAT				0x2c
#define FT_IC_INTR_MASK				0x30
#define FT_IC_RAW_INTR_STAT			0x34
#define FT_IC_RX_TL				0x38
#define FT_IC_TX_TL				0x3c
#define FT_IC_CLR_INTR				0x40
#define FT_IC_CLR_RX_UNDER			0x44
#define FT_IC_CLR_RX_OVER			0x48
#define FT_IC_CLR_TX_OVER			0x4c
#define FT_IC_CLR_RD_REQ			0x50
#define FT_IC_CLR_TX_ABRT			0x54
#define FT_IC_CLR_RX_DONE			0x58
#define FT_IC_CLR_ACTIVITY			0x5c
#define FT_IC_CLR_STOP_DET			0x60
#define FT_IC_CLR_START_DET			0x64
#define FT_IC_CLR_GEN_CALL			0x68
#define FT_IC_ENABLE				0x6c
#define FT_IC_STATUS				0x70
#define FT_IC_TXFLR				0x74
#define FT_IC_RXFLR				0x78
#define FT_IC_SDA_HOLD				0x7c
#define FT_IC_TX_ABRT_SOURCE			0x80
#define FT_IC_ENABLE_STATUS			0x9c
#define FT_IC_SMBCLK_LOW_MEXT			0xa8
#define FT_IC_SMBCLK_LOW_TIMEOUT		0xac
#define FT_IC_SMBDAT_STUCK_TIMEOUT		0xb4
#define FT_IC_CLR_SMBCLK_EXT_LOW_TIMEOUT	0xbc
#define FT_IC_CLR_SMBCLK_TMO_LOW_TIMEOUT	0xc0
#define FT_IC_CLR_SMBDAT_LOW_TIMEOUT		0xc4
#define FT_IC_CLR_SMBALERT_IN_N			0xd0

#define FT_IC_INTR_RX_UNDER			0x001
#define FT_IC_INTR_RX_OVER			0x002
#define FT_IC_INTR_RX_FULL			0x004
#define FT_IC_INTR_TX_OVER			0x008
#define FT_IC_INTR_TX_EMPTY			0x010
#define FT_IC_INTR_RD_REQ			0x020
#define FT_IC_INTR_TX_ABRT			0x040
#define FT_IC_INTR_RX_DONE			0x080
#define FT_IC_INTR_ACTIVITY			0x100
#define FT_IC_INTR_STOP_DET			0x200
#define FT_IC_INTR_START_DET			0x400
#define FT_IC_INTR_GEN_CALL			0x800
#define FT_IC_INTR_SMBCLK_EXT_LOW_TIMEOUT	0x1000
#define FT_IC_INTR_SMBCLK_TMO_LOW_TIMEOUT	0x2000
#define FT_IC_INTR_SMBSDA_LOW_TIMEOUT		0x4000
#define FT_IC_INTR_SMBALERT_IN_N		0x20000

#define FT_IC_INTR_DEFAULT_MASK                                                   \
	(FT_IC_INTR_RX_FULL | FT_IC_INTR_TX_ABRT | FT_IC_INTR_STOP_DET)
#define FT_IC_INTR_MASTER_MASK (FT_IC_INTR_DEFAULT_MASK | FT_IC_INTR_TX_EMPTY)
#define FT_IC_INTR_SLAVE_MASK                                                     \
	(FT_IC_INTR_DEFAULT_MASK | FT_IC_INTR_RX_DONE | FT_IC_INTR_RX_UNDER | FT_IC_INTR_RD_REQ)
#define FT_IC_INTR_SMBUS_MASK                                                     \
	(FT_IC_INTR_MASTER_MASK | FT_IC_INTR_SMBCLK_EXT_LOW_TIMEOUT |                      \
	FT_IC_INTR_SMBCLK_TMO_LOW_TIMEOUT | FT_IC_INTR_SMBSDA_LOW_TIMEOUT)

#define FT_IC_INTR_SMBUS_TIME_MASK          \
	(FT_IC_INTR_SMBCLK_EXT_LOW_TIMEOUT | \
	 FT_IC_INTR_SMBCLK_TMO_LOW_TIMEOUT | FT_IC_INTR_SMBSDA_LOW_TIMEOUT)

#define FT_IC_STATUS_ACTIVITY			0x1
#define FT_IC_STATUS_TFE					BIT(2)
#define FT_IC_STATUS_MASTER_ACTIVITY		BIT(5)
#define FT_IC_STATUS_SLAVE_ACTIVITY		BIT(6)

#define FT_IC_SDA_HOLD_RX_SHIFT			16
#define FT_IC_SDA_HOLD_RX_MASK			GENMASK(23, IC_SDA_HOLD_RX_SHIFT)

#define FT_IC_ERR_TX_ABRT			3

#define FT_IC_TAR_10BITADDR_MASTER		BIT(12)

#define FT_IC_COMP_PARAM_1_SPEED_MODE_HIGH	(BIT(2) | BIT(3))
#define FT_IC_COMP_PARAM_1_SPEED_MODE_MASK	GENMASK(3, 2)

#define FT_STATUS_IDLE				0x0
#define FT_STATUS_WRITE_IN_PROGRESS		0x1
#define FT_STATUS_READ_IN_PROGRESS		0x2

/*
 * operation modes
 */
#define phyt_IC_MASTER			0
#define phyt_IC_SLAVE			1

#define FT_ABRT_7B_ADDR_NOACK			0
#define FT_ABRT_10ADDR1_NOACK			1
#define FT_ABRT_10ADDR2_NOACK			2
#define FT_ABRT_TXDATA_NOACK			3
#define FT_ABRT_GCALL_NOACK			4
#define FT_ABRT_GCALL_READ			5
#define FT_ABRT_SBYTE_ACKDET			7
#define FT_ABRT_SBYTE_NORSTRT			9
#define FT_ABRT_10B_RD_NORSTRT			10
#define FT_ABRT_MASTER_DIS			11
#define FT_ARB_LOST				12
#define FT_ABRT_SLAVE_FLUSH_TXFIFO		13
#define FT_ABRT_SLAVE_ARBLOST			14
#define FT_ABRT_SLAVE_RD_INTX			15

#define FT_IC_TX_ABRT_7B_ADDR_NOACK		(1UL << FT_ABRT_7B_ADDR_NOACK)
#define FT_IC_TX_ABRT_10ADDR1_NOACK		(1UL << FT_ABRT_10ADDR1_NOACK)
#define FT_IC_TX_ABRT_10ADDR2_NOACK		(1UL << FT_ABRT_10ADDR2_NOACK)
#define FT_IC_TX_ABRT_TXDATA_NOACK		(1UL << FT_ABRT_TXDATA_NOACK)
#define FT_IC_TX_ABRT_GCALL_NOACK		(1UL << FT_ABRT_GCALL_NOACK)
#define FT_IC_TX_ABRT_GCALL_READ		(1UL << FT_ABRT_GCALL_READ)
#define FT_IC_TX_ABRT_SBYTE_ACKDET		(1UL << FT_ABRT_SBYTE_ACKDET)
#define FT_IC_TX_ABRT_SBYTE_NORSTRT		(1UL << FT_ABRT_SBYTE_NORSTRT)
#define FT_IC_TX_ABRT_10B_RD_NORSTRT		(1UL << FT_ABRT_10B_RD_NORSTRT)
#define FT_IC_TX_ABRT_MASTER_DIS		(1UL << FT_ABRT_MASTER_DIS)
#define FT_IC_TX_ARB_LOST			(1UL << FT_ARB_LOST)
#define FT_IC_RX_ABRT_SLAVE_RD_INTX		(1UL << FT_ABRT_SLAVE_RD_INTX)
#define FT_IC_RX_ABRT_SLAVE_ARBLOST		(1UL << FT_ABRT_SLAVE_ARBLOST)
#define FT_IC_RX_ABRT_SLAVE_FLUSH_TXFIFO	(1UL << FT_ABRT_SLAVE_FLUSH_TXFIFO)

#define FT_IC_TX_ABRT_NOACK                                         \
	(FT_IC_TX_ABRT_7B_ADDR_NOACK | FT_IC_TX_ABRT_10ADDR1_NOACK |\
	FT_IC_TX_ABRT_10ADDR2_NOACK | FT_IC_TX_ABRT_TXDATA_NOACK |\
	FT_IC_TX_ABRT_GCALL_NOACK)
#define FT_CONTROLLER_TYPE_IIC			0
#define FT_CONTROLLER_TYPE_SMBUS		1

#define FT_I2C_MSG_COMPLETE_OK			1
#define FT_I2C_MSG_COMPLETE_UNKNOW		0

#define FT_I2C_CON_MASTER_MODE_MASK		0x01
#define FT_I2C_CON_SLAVE_MODE_MASK		(1 << 6)
#define FT_I2C_CON_RESTART_MASK			0x20

#define FT_I2C_CON_ADDR_MODE_MASK		(1 << 3)
#define FT_I2C_ADDR_7BIT_MODE			(0)

#define FT_I2C_TRANS_TIMEOUT		0xF0

#define FT_ACCESS_INTR_MASK			0x00000004
#define FT_DEFAULT_CLOCK_FREQUENCY		100000000

#define FT_I2C_MSG_DATA_LEN			56
#define FT_I2C_SINGLE_BUF_LEN			51
#define FT_I2C_SINGLE_FRAME_CNT			32

#define RV_ANSWER_DATA_SIZE			122
#define RV_ANSWER_DATA_CONTINUE			1
#define RV_ANSWER_DATA_END			0

enum phyti2c_status_code {
	FT_I2C_SUCCESS = 0,
	FT_I2C_TIMEOUT,
	FT_I2C_CNT_ERR,
	FT_I2C_TX_ABRT,
	FT_I2C_INT_ERR,
	FT_I2C_BLOCK_SIZE,
	FT_I2C_INVALID_ADDR,
	FT_I2C_TRANS_PACKET_FAIL,
	/*The RV result must put above*/
	FT_I2C_RUNNING,
	FT_I2C_CHECK_STATUS_ERR
};

struct i2c_ft_rv_event_mng {
	u8 event_cnt;
};

enum phyti2c_msg_cmd_id {
	PHYTI2C_MSG_CMD_DEFAULT = 0,
	PHYTI2C_MSG_CMD_SET,
	PHYTI2C_MSG_CMD_GET,
	PHYTI2C_MSG_CMD_DATA,
	PHYTI2C_MSG_CMD_REPORT,
	PHYTI2C_SYS_PROTOCOL,
};

enum phyti2c_msg_default_subid {
	PHYTI2C_MSG_CMD_DEFAULT_RESV = 0,
	PHYTI2C_MSG_CMD_DEFAULT_RESUME,
	PHYTI2C_MSG_CMD_DEFAULT_SLV
};

struct i2c_ft_default_cfg_msg {
	u32 ss_hcnt;
	u32 ss_lcnt;
	u32 fs_hcnt;
	u32 fs_lcnt;
	u32 hs_hcnt;
	u32 hs_lcnt;
	u32 sda_hold;
	u32 tx_fifo_thr;
	u32 rx_fifo_thr;
	u32 smbclk_mext;
	u32 smbclk_timeout;
	u32 smbdat_timeout;
	u32 cfg;
	u32 intr_mask;
};

enum phyti2c_set_subid {
	PHYTI2C_MSG_CMD_SET_MODULE_EN = 0,
	PHYTI2C_MSG_CMD_SET_MODE,
	PHYTI2C_MSG_CMD_SET_RESTART,
	PHYTI2C_MSG_CMD_SET_ADDR_MODE,
	PHYTI2C_MSG_CMD_SET_SPEED,
	PHYTI2C_MSG_CMD_SET_INT_TL,
	PHYTI2C_MSG_CMD_SET_SDA_HOLD,
	PHYTI2C_MSG_CMD_SET_INTERRUPT,
	PHYTI2C_MSG_CMD_SET_RX_FIFO_FULL,
	PHYTI2C_MSG_CMD_SET_STOP_DET_IF_ADDRESSED,
	PHYTI2C_MSG_CMD_SET_WRITE_PROTECT,
	PHYTI2C_MSG_CMD_SET_SMBCLK_LOW_MEXT,
	PHYTI2C_MSG_CMD_SET_SMBCLK_LOW_TIMEOUT,
	PHYTI2C_MSG_CMD_SET_SMBDAT_STUCK_TIMEOUT,
	PHYTI2C_MSG_CMD_SET_ADDR,
	PHYTI2C_MSG_CMD_SET_SUSPEND
};

enum phyti2c_data_subid {
	PHYTI2C_MSG_CMD_DATA_XFER = 0,
	PHYTI2C_MSG_CMD_DATA_SLAVE
};

enum phyti2c_report_subid {
	PHYTI2C_MSG_CMD_SMBCLK_EXT_LOW_TIMEOUT = 0,
	PHYTI2C_MSG_CMD_SMBCLK_TMO_LOW_TIMEOUT,
	PHYTI2C_MSG_CMD_SMBSDA_LOW_TIMEOUT,
	PHYTI2C_MSG_CMD_SMBALERT_IN_N,
	PHYTI2C_MSG_CMD_SLAVE_EVENT,
};

struct i2c_phyt_tranfer {
	u32 tx_cmd_cnt;
	u32 cur_cmd_cnt;
	u32 cur_index;
	u32 opt_finish_len;
	u32 tx_ring_cnt;
	bool is_need_check;
	bool is_last_frame;
};

struct i2c_phyt_dev {
	struct device *dev;
	void __iomem *base;
	void __iomem  *log_addr;
	void *tx_shmem_addr;
	void *rx_shmem_addr;
	u8 *tx_buf;
	u8 *rx_buf;
	struct clk *clk;
	struct reset_control *rst;
	struct i2c_client *slave;
	struct i2c_client *ara;
	struct i2c_msg *msgs;
	struct timer_list timer;
	struct phytium_pci_i2c *controller;

	int module;
	int irq;
	u32 intr_mask;
	u32 flags;
	u32 total_shmem_len;
	u32 total_cnt;
	int mode;
	struct i2c_phyt_tranfer mng;
	struct completion cmd_complete;
	struct i2c_adapter adapter;
	struct i2c_smbus_alert_setup alert_data;

	bool complete_flag;
	u32 abort_source;

	int msgs_num;
	int msg_err;
	u32 tx_buf_len;
	u32 rx_buf_len;

	u32 master_cfg;
	u32 slave_cfg;
	u32 functionality;

	u8 real_index[FT_I2C_SINGLE_FRAME_CNT];
	struct i2c_timings timings;
	u32 sda_hold_time;
	u16 ss_hcnt;
	u16 ss_lcnt;
	u16 fs_hcnt;
	u16 fs_lcnt;
	u16 fp_hcnt;
	u16 fp_lcnt;
	u16 hs_hcnt;
	u16 hs_lcnt;

	bool alive_enabled;
	bool debug_enabled;
	u32 log_size;
	bool pm_disabled;
	spinlock_t shmem_spin;

	u32 (*get_clk_rate_khz)(struct i2c_phyt_dev *dev);
	void (*disable)(struct i2c_phyt_dev *dev);
	void (*disable_int)(struct i2c_phyt_dev *dev);
	void (*watchdog)(struct i2c_phyt_dev *dev);
	int (*init)(struct i2c_phyt_dev *dev);
};

struct phyt_msg_head {
	u8 resv;
	u8 seq;
	u8 cmd_type;
	u8 cmd_subid;
	u16 len;
	u8 status1;
	u8 status0;
};

struct phyt_msg_info {
	struct phyt_msg_head head;
	u8 data[FT_I2C_MSG_DATA_LEN];
};

struct i2c_ft_trans_msg_info {
	u16 addr;
	u16 flags;
	u8 type;
} __packed;

struct i2c_phyt_bus_speed_info {
	u8   speed_mode;
	u8   calc_en;
	u16  scl_hcnt;
	u16  scl_lcnt;
	u32  sda_hold;
};

struct i2c_phyt_fifo_threshold {
	u8 rx_fifo_threshold;
	u8 tx_fifo_threshold;
};

struct i2c_phyt_sda_hold_info {
	u32 sda_hold;
};

static inline void i2c_phyt_write_reg(struct i2c_phyt_dev *dev,
					u32 reg_offset, u32 reg_value)
{
	writel_relaxed(reg_value, dev->base + reg_offset);
}

static inline u32 i2c_phyt_read_reg(struct i2c_phyt_dev *dev, u32 reg_offset)
{
	return readl_relaxed(dev->base + reg_offset);
}

unsigned long i2c_phyt_clk_rate(struct i2c_phyt_dev *dev);
int i2c_phyt_prepare_clk(struct i2c_phyt_dev *dev, bool prepare);
u32 i2c_phyt_func(struct i2c_adapter *adap);
u32 i2c_phyt_scl_hcnt(u32 ic_clk, u32 tSYMBOL, u32 tf, int cond, int offset);
u32 i2c_phyt_scl_lcnt(u32 ic_clk, u32 tLOW, u32 tf, int offset);
void i2c_phyt_default_cfg(struct i2c_phyt_dev *dev,
					struct i2c_ft_default_cfg_msg *buf);
void i2c_phyt_set_suspend(struct i2c_phyt_dev *dev);
void i2c_phyt_enable_debug(struct i2c_phyt_dev *dev);
void i2c_phyt_disable_debug(struct i2c_phyt_dev *dev);
void i2c_phyt_show_log(struct i2c_phyt_dev *dev);
void i2c_phyt_enable_alive(struct i2c_phyt_dev *dev);
void i2c_phyt_disable_alive(struct i2c_phyt_dev *dev);
void i2c_phyt_common_watchdog(struct i2c_phyt_dev *dev);
int  i2c_phyt_malloc_log_mem(struct i2c_phyt_dev *dev);
int i2c_phyt_rv_data_cmd_handle(struct phyt_msg_info *rx_msg);
int i2c_phyt_master_probe(struct i2c_phyt_dev *dev);
void i2c_phyt_handle_tx_abort(struct i2c_phyt_dev *dev);
int i2c_phyt_check_result(struct i2c_phyt_dev *dev);
void i2c_phyt_set_cmd8(struct i2c_phyt_dev *dev, u8 sub_cmd, u8 data);
void i2c_phyt_set_cmd16(struct i2c_phyt_dev *dev, u8 sub_cmd, u16 data);
void i2c_phyt_set_cmd32(struct i2c_phyt_dev *dev, u8 sub_cmd, u32 data);
void i2c_phyt_common_set_cmd(struct i2c_phyt_dev *dev,
				struct phyt_msg_info *i2c_mng_msg, u8 cmd, u8 sub_cmd);
void i2c_phyt_disable_int(struct i2c_phyt_dev *dev);
void i2c_phyt_set_int_tl(struct i2c_phyt_dev *dev, u8 tx_threshold,
					u8 rx_threshold);
int i2c_phyt_slave_probe(struct i2c_phyt_dev *dev);
void i2c_phyt_set_module_en(struct i2c_phyt_dev *dev, u8 data);
void i2c_phyt_set_sda_hold(struct i2c_phyt_dev *dev, u32 data);
void i2c_phyt_disable(struct i2c_phyt_dev *dev);
int i2c_phyt_slave_event_process(struct i2c_phyt_dev *dev,
					struct phyt_msg_info *rx_msg, u32 head, u32 tail);
void i2c_phyt_data_cmd8_array(struct i2c_phyt_dev *dev, u8 sub_cmd, u8 *data,
							int len);
void i2c_phyt_slave_isr_handle(struct i2c_phyt_dev *dev);
void i2c_phyt_master_isr_handle(struct i2c_phyt_dev *dev);
int i2c_phyt_master_smbus_alert_process(struct i2c_phyt_dev *dev);
void i2c_phyt_common_regfile_enable_int(struct i2c_phyt_dev *dev);
void i2c_phyt_common_regfile_disable_int(struct i2c_phyt_dev *dev);
void i2c_phyt_common_regfile_clear_rv2ap_int(struct i2c_phyt_dev *dev, u32 stat);
void i2c_phyt_notify_rv(struct i2c_phyt_dev *dev, bool need_check);
int i2c_phyt_check_status(struct i2c_phyt_dev *dev, struct phyt_msg_info *msg);
void i2c_phyt_set_int_interrupt(struct i2c_phyt_dev *dev,
				u32 is_enable, u32 intr_mask);
#endif
