// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2020 WXIAT Platform Software
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 * The drivers in this file are synchronous/blocking. In addition,
 * use poll mode to read/write slave devices on the I2C bus instead
 * of the interrupt mode.
 */

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fb.h>

#include <asm/sw64io.h>

#define CPLD_BUSNR	 2

#define IC_CLK_KHZ			25000

/* I2C register definitions */
#define DW_IC_CON			0x0
#define DW_IC_STATUS			0x3800
#define DW_IC_DATA_CMD			0x0800
#define DW_IC_TAR			0x00200
#define DW_IC_ENABLE			0x3600
#define DW_IC_CMD			0x0100
#define DW_IC_STOP			0x0200
#define DW_IC_SDA_HOLD			0x3e00
#define DW_IC_SDA_SETUP			0x4a00
#define DW_IC_SS_SCL_HCNT		0x0a00
#define DW_IC_SS_SCL_LCNT		0x0c00
#define DW_IC_FS_SCL_HCNT		0x0e00
#define DW_IC_FS_SCL_LCNT		0x1000
#define DW_IC_TX_TL			0x1e00
#define DW_IC_RX_TL			0x1c00
#define DW_IC_INTR_MASK			0x1800

#define MAX_RETRY			10000000

#define DW_IC_STATUS_ACTIVITY		0x1
#define DW_IC_STATUS_TFNF		0x2
#define DW_IC_STATUS_TFE		0x4
#define DW_IC_STATUS_RFNE		0x8
#define DW_IC_STATUS_RFF		0x10

#define DW_IC_CON_MASTER		0x1
#define DW_IC_CON_SPEED_STD	        0x2
#define DW_IC_CON_SPEED_FAST		0x4
#define DW_IC_CON_10BITADDR_MASTER	0x10
#define DW_IC_CON_RESTART_EN		0x20
#define DW_IC_CON_SLAVE_DISABLE		0x40

#define INTEL_MID_STD_CFG (DW_IC_CON_MASTER | \
		DW_IC_CON_SLAVE_DISABLE | \
		DW_IC_CON_RESTART_EN)

#define DW_IC_INTR_RX_UNDER		0x001
#define DW_IC_INTR_RX_OVER		0x002
#define DW_IC_INTR_RX_FULL		0x004
#define DW_IC_INTR_TX_OVER		0x008
#define DW_IC_INTR_TX_EMPTY		0x010
#define DW_IC_INTR_RD_REQ		0x020
#define DW_IC_INTR_TX_ABRT		0x040
#define DW_IC_INTR_RX_DONE		0x080
#define DW_IC_INTR_ACTIVITY		0x100
#define DW_IC_INTR_STOP_DET		0x200
#define DW_IC_INTR_START_DET	        0x400
#define DW_IC_INTR_GEN_CALL		0x800

#define DW_IC_INTR_DEFAULT_MASK (DW_IC_INTR_RX_FULL | \
		DW_IC_INTR_TX_EMPTY | \
		DW_IC_INTR_TX_ABRT | \
		DW_IC_INTR_STOP_DET)

enum i2c_bus_operation {
	I2C_BUS_READ,
	I2C_BUS_WRITE,
};

static void __iomem *m_i2c_base_address;

/*
 * This function get I2Cx controller base address
 *
 * @param i2c_controller_index  Bus Number of I2C controller.
 * @return I2C BAR.
 */
void __iomem *get_i2c_bar_addr(uint8_t i2c_controller_index)
{
	switch (i2c_controller_index) {
	case 0:
		return __va(IO_BASE | IIC0_BASE);
	case 1:
		return __va(IO_BASE | IIC1_BASE);
	case 2:
		return __va(IO_BASE | IIC2_BASE);
	default:
		return NULL;
	}
}

static inline void write_cpu_i2c_controller(uint64_t offset, uint32_t data)
{
	writel(data, m_i2c_base_address + offset);
}

static inline uint32_t read_cpu_i2c_controller(uint64_t offset)
{
	return readl(m_i2c_base_address + offset);
}

static int poll_for_status_set0(uint16_t status_bit)
{
	uint64_t retry = 0;
	uint32_t temp = read_cpu_i2c_controller(DW_IC_STATUS);

	temp = read_cpu_i2c_controller(DW_IC_STATUS);

	while (retry < MAX_RETRY) {
		if (read_cpu_i2c_controller(DW_IC_STATUS) & status_bit)
			break;
		retry++;
	}

	if (retry == MAX_RETRY)
		return -ETIME;

	return 0;
}

static uint32_t i2c_dw_scl_lcnt(uint32_t ic_clk, uint32_t t_low,
				uint32_t tf, uint32_t offset)
{
	/*
	 * Conditional expression:
	 *
	 *   IC_[FS]S_SCL_LCNT + 1 >= IC_CLK * (t_low + tf)
	 *
	 * DW I2C core starts counting the SCL CNTs for the LOW period
	 * of the SCL clock (t_low) as soon as it pulls the SCL line.
	 * In order to meet the t_low timing spec, we need to take into
	 * account the fall time of SCL signal (tf).  Default tf value
	 * should be 0.3 us, for safety.
	 */
	return ((ic_clk * (t_low + tf) + 500000) / 1000000) - 1 + offset;
}

static uint32_t i2c_dw_scl_hcnt(uint32_t ic_clk, uint32_t t_symbol,
				uint32_t tf, uint32_t cond, uint32_t offset)
{
	/*
	 * DesignWare I2C core doesn't seem to have solid strategy to meet
	 * the tHD;STA timing spec. Configuring _HCNT based on tHIGH spec
	 * will result in violation of the tHD;STA spec.
	 */
	if (cond)
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + (1+4+3) >= IC_CLK * tHIGH
		 *
		 * This is based on the DW manuals, and represents an ideal
		 * configuration. The resulting I2C bus speed will be faster
		 * than any of the others.
		 *
		 * If your hardware is free from tHD;STA issue, try this one.
		 */
		return (ic_clk * t_symbol + 500000) / 1000000 - 8 + offset;
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + 3 >= IC_CLK * (tHD;STA + tf)
		 *
		 * This is just experimental rule; the tHD;STA period turned
		 * out to be proportinal to (_HCNT + 3). With this setting,
		 * we could meet both tHIGH and tHD;STA timing specs.
		 *
		 * If unsure, you'd better to take this alternative.
		 *
		 * The reason why we need to take into account "tf" here,
		 * is the same as described in i2c_dw_scl_lcnt().
		 */
	return (ic_clk * (t_symbol + tf) + 500000) / 1000000 - 3 + offset;
}

static int wait_for_cpu_i2c_bus_busy(void)
{
	uint64_t retry = 0;
	uint32_t status = 0;

	do {
		retry++;
		status = !!(read_cpu_i2c_controller(DW_IC_STATUS) & DW_IC_STATUS_ACTIVITY);
	} while ((retry < MAX_RETRY) && status);

	if (retry == MAX_RETRY)
		return -ETIME;

	return 0;
}

static int i2c_read(uint8_t reg_offset, uint8_t *buffer, uint32_t length)
{
	int status;
	uint32_t i;

	status = poll_for_status_set0(DW_IC_STATUS_TFE);
	if (status)
		return status;

	write_cpu_i2c_controller(DW_IC_DATA_CMD, reg_offset);

	for (i = 0; i < length; i++) {
		if (i == length - 1)
			write_cpu_i2c_controller(DW_IC_DATA_CMD, DW_IC_CMD | DW_IC_STOP);
		else
			write_cpu_i2c_controller(DW_IC_DATA_CMD, DW_IC_CMD);

		if (poll_for_status_set0(DW_IC_STATUS_RFNE) == 0)
			buffer[i] = readb(m_i2c_base_address + DW_IC_DATA_CMD);
		else
			pr_err("Read timeout line %d.\n", __LINE__);
	}

	return 0;
}

static int i2c_write(uint8_t reg_offset, uint8_t *buffer, uint32_t length)
{
	int status;
	uint32_t i;

	/* Data transfer, poll till transmit ready bit is set */
	status = poll_for_status_set0(DW_IC_STATUS_TFE);
	if (status) {
		pr_err("In i2c-lib.c, line %d.\n", __LINE__);
		return status;
	}

	write_cpu_i2c_controller(DW_IC_DATA_CMD, reg_offset);

	for (i = 0; i < length; i++) {
		if (poll_for_status_set0(DW_IC_STATUS_TFNF) == 0) {
			if (i == length - 1)
				write_cpu_i2c_controller(DW_IC_DATA_CMD, buffer[i] | DW_IC_STOP);
			else
				write_cpu_i2c_controller(DW_IC_DATA_CMD, buffer[i]);
		} else {
			pr_err("Write timeout %d.\n", __LINE__);
		}
	}

	mdelay(200);
	status = poll_for_status_set0(DW_IC_STATUS_TFE);
	if (status) {
		pr_err("In i2c-lib.c, line %d.\n", __LINE__);
		return status;
	}

	return 0;
}

/* Initialize I2c controller */
void init_cpu_i2c_controller(void)
{
	uint32_t h_cnt;
	uint32_t l_cnt;
	uint32_t input_ic_clk_rate = IC_CLK_KHZ;	/* by unit KHz ie. 25MHz */
	uint32_t sda_falling_time = 300;
	uint32_t scl_falling_time = 300;

	/*
	 * The I2C protocol specification requires 300ns of hold time on the
	 * SDA signal (tHD;DAT) in standard and fast speed modes, and a hold
	 * time long enough to bridge the undefined part between logic 1 and
	 * logic 0 of the falling edge of SCL in high speed mode.
	 */
	uint32_t sda_hold_time = 432;
	uint32_t sda_hold = 0;

	/* Firstly disable the controller. */
	pr_debug("Initialize CPU I2C controller\n");

	write_cpu_i2c_controller(DW_IC_ENABLE, 0);

	sda_hold = (input_ic_clk_rate * sda_hold_time + 500000) / 1000000;
	write_cpu_i2c_controller(DW_IC_SDA_HOLD, sda_hold);

	/* Set standard and fast speed deviders for high/low periods. */
	/* Standard-mode */
	h_cnt = i2c_dw_scl_hcnt(input_ic_clk_rate, 4000, sda_falling_time, 0, 0);
	l_cnt = i2c_dw_scl_lcnt(input_ic_clk_rate, 4700, scl_falling_time, 0);

	write_cpu_i2c_controller(DW_IC_SS_SCL_HCNT, h_cnt);
	write_cpu_i2c_controller(DW_IC_SS_SCL_LCNT, l_cnt);

	pr_debug("Standard-mode HCNT=%x, LCNT=%x\n", h_cnt, l_cnt);

	/* Fast-mode */
	h_cnt = i2c_dw_scl_hcnt(input_ic_clk_rate, 600, sda_falling_time, 0, 0);
	l_cnt = i2c_dw_scl_lcnt(input_ic_clk_rate, 1300, scl_falling_time, 0);

	write_cpu_i2c_controller(DW_IC_FS_SCL_HCNT, h_cnt);
	write_cpu_i2c_controller(DW_IC_FS_SCL_LCNT, l_cnt);

	pr_debug("Fast-mode HCNT=%x, LCNT=%d\n\n", h_cnt, l_cnt);

	/* Configure Tx/Rx FIFO threshold levels, since we will be working
	 * in polling mode set both thresholds to their minimum
	 */
	write_cpu_i2c_controller(DW_IC_TX_TL, 0);
	write_cpu_i2c_controller(DW_IC_RX_TL, 0);
	write_cpu_i2c_controller(DW_IC_INTR_MASK, DW_IC_INTR_DEFAULT_MASK);

	/* Configure the i2c master */
	write_cpu_i2c_controller(DW_IC_CON,
			      INTEL_MID_STD_CFG | DW_IC_CON_SPEED_STD);

}

/*
 * This function enables I2C controllers.
 *
 * @param i2c_controller_index  Bus Number of I2C controllers.
 */
void enable_i2c_controller(uint8_t i2c_controller_index)
{
	m_i2c_base_address = get_i2c_bar_addr(i2c_controller_index);
	init_cpu_i2c_controller();
}

/*
 * Write/Read data from I2C device.
 *
 * @i2c_controller_index: i2c bus number
 * @slave_address: slave address
 * @operation: to read or write
 * @length: number of bytes
 * @reg_offset: register offset
 * @buffer: in/out buffer
 */
int i2c_bus_rw(uint8_t i2c_controller_index, uint8_t slave_address,
	       enum i2c_bus_operation operation, uint32_t length,
	       uint8_t reg_offset, void *buffer)
{
	uint8_t *byte_buffer = buffer;
	int status = 0;
	uint32_t databuffer, temp;

	m_i2c_base_address = get_i2c_bar_addr(i2c_controller_index);
	status = wait_for_cpu_i2c_bus_busy();
	if (status) {
		pr_err("%d\n", __LINE__);
		return status;
	}

	mdelay(1000);

	/* Set the slave address. */
	write_cpu_i2c_controller(DW_IC_ENABLE, 0x0);	/* Disable controller */
	databuffer = read_cpu_i2c_controller(DW_IC_CON);
	databuffer &= ~DW_IC_CON_10BITADDR_MASTER;
	write_cpu_i2c_controller(DW_IC_CON, databuffer);

	/* Fill the target addr. */
	write_cpu_i2c_controller(DW_IC_TAR, slave_address);

	temp = read_cpu_i2c_controller(DW_IC_TAR);

	/* Configure Tx/Rx FIFO threshold levels. */
	write_cpu_i2c_controller(DW_IC_ENABLE, 0x1);	/* Enable the adapter */
	write_cpu_i2c_controller(DW_IC_INTR_MASK, DW_IC_INTR_DEFAULT_MASK);

	if (operation == I2C_BUS_READ)
		status = i2c_read(reg_offset, byte_buffer, length);
	else if (operation == I2C_BUS_WRITE)
		status = i2c_write(reg_offset, byte_buffer, length);

	/* Disable controller */
	write_cpu_i2c_controller(DW_IC_ENABLE, 0x0);

	return status;
}

void disable_i2c_controller(uint8_t i2c_controller_index)
{
	m_i2c_base_address = get_i2c_bar_addr(i2c_controller_index);

	/* Disable controller */
	write_cpu_i2c_controller(DW_IC_ENABLE, 0x0);
	m_i2c_base_address = 0;
}

void cpld_write(uint8_t slave_addr, uint8_t reg, uint8_t data)
{
	enable_i2c_controller(CPLD_BUSNR);
	i2c_bus_rw(CPLD_BUSNR, slave_addr, I2C_BUS_WRITE, sizeof(uint8_t), reg, &data);
	disable_i2c_controller(CPLD_BUSNR);
}
