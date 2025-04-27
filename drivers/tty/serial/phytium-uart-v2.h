/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2020-2025, Phytium Technology, Co., Ltd.
 *
 */
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/amba/bus.h>
#include <linux/amba/serial.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/sizes.h>
#include <linux/io.h>
#include <linux/acpi.h>
#include <linux/ktime.h>
#include <asm/early_ioremap.h>

#define DRV_NAME                "ttyVS"
#define MSG_COMPLETE            0x1
#define UART_NR                 14

#define TX_DATA_MAXINUM         120
#define RX_DATA_MAXINUM         ((TX_DATA_MAXINUM / 2) + 1)

#define DATA_OE               (1 << 11)
#define DATA_BE               (1 << 10)
#define DATA_PE               (1 << 9)
#define DATA_FE               (1 << 8)
#define DATA_ERROR           (DATA_OE|DATA_BE|DATA_PE|DATA_FE)
#define DATA_DUMMY_RX        (1 << 16)

#define REG_TX_HEAD             0x0     /* tx_ring_buffer head pointer reg */
#define REG_TX_TAIL             0x4     /* tx_ring_buffer tail pointer reg */
#define REG_RX_HEAD             0x8     /* rx_ring_buffer tail pointer reg */
#define REG_RX_TAIL             0xc     /* rx_ring_buffer tail pointer reg */
#define REG_PHYT_INT_STATE     0x24
#define REG_RP_INT_STATE     0x2c
#define REG_RP_INT_STATE_CLR	0x74
#define REG_PHYT_INT_MASK      0x20
#define REG_RP_INT_MASK      0x28
#define REG_FAKE_DR             0x30
#define REG_FAKE_FR             0x34
#define REG_CHECK_TX		0x4c

#define MODEM_CTS               0x1
#define MODEM_DSR               0x2
#define MODEM_DCD               0x004
#define MODEM_CAR               MODEM_DCD
#define MODEM_RNG               0x8
#define MODEM_RTS               0x10
#define MODEM_DTR               0x20
#define MODEM_OUT1              0x40
#define MODEM_OUT2              0x80

#define TX_HEAD_INT             0x1
#define RX_TAIL_INT             0x2
#define PHYT_INT_TRIGGER_BIT   0x10
#define PHYT_MSG_DATA_COMPLETED 0x10
#define MODEM_INT               0x20

#define UART_MODULE_ID          0x1
#define MSG_DEFAULT             0x0
#define MSG_DEFAULT_SUBID	0x10
#define MSG_SET                 0x1
#define MSG_GET                 0x2
#define MSG_DATA                0x3
#define MSG_GET_MODEM           0x3
#define MSG_GET_RX_EMPTY        0x4
#define MSG_TX_DATA             0x0
#define RX_TAIL_INT_ENABLE      0x10000
#define MSG_DATA_MASK		0xff
#define BUFFER_POINTER_MASK	0xffff
#define CHAR_MASK		255

#define TX_BUFFER_SIZE          8
#define RX_BUFFER_SIZE          8
#define BUFFER_SIZE             20
#define UART_FIFOSIZE           64
#define TX_MSG_SIZE             0x80
#define RX_MSG_SIZE             0x80
#define RX_CHARS_MAX		28

/* uart debug mechanism */
#define PHYUART_DBG_REG		0x58
/* uart debug register mask bit */
#define PHYUART_DBG_ENABLE_MASK	0x1
#define PHYUART_DBG_HEARTBEAT_ENABLE_MASK	(0x1 << 1)
#define PHYUART_DBG_HEARTBEAT_MASK	(0x1 << 2)
#define PHYUART_DBG_LOG_EXIST_MASK	(0x1 << 3)
#define PHYUART_DBG_SIZE_MASK	(0xf << 4)
#define PHYUART_DBG_ADDR_MASK	(0x3fff << 8)

/* enum all type-set subid */
enum phytuart_set_subid {
	/* enable/disable */
	PHYTUART_MSG_CMD_SET_DEVICE_EN = 0x0,
	/* set baud */
	PHYTUART_MSG_CMD_SET_BAUD,
	/* set trans bit width 0: 5，1: 6，2: 7，3:8 */
	PHYTUART_MSG_CMD_SET_DATABIT,
	/* set stop bit，0：1 bit, 1: 2bit */
	PHYTUART_MSG_CMD_SET_STOPBIT,
	/* set parity enable, 0:DISABLE, 1:ENABLE */
	PHYTUART_MSG_CMD_SET_PARITY_EN,
	/* set parity bit， 0：ODD, 1: EVEN */
	PHYTUART_MSG_CMD_SET_PARITY_EVEN_SET,
	/* set 0/1 parity enable */
	PHYTUART_MSG_CMD_SET_PARITY_STICK_SET,
	/* set break signal，0:no break 1:send break */
	PHYTUART_MSG_CMD_SET_BREAK_EN,
	/* set RX en， 0:disable， single byte， 1：enable，*/
	PHYTUART_MSG_CMD_SET_RX_BUFFER_EN,
	/* set TX en， 0:disable，single byte， 1：enable */
	PHYTUART_MSG_CMD_SET_TX_BUFFER_EN,
	/* set TX enable， 0:disable， 1：enable */
	PHYTUART_MSG_CMD_SET_TX_EN,
	/* set RX enable， 0:disable， 1：enable */
	PHYTUART_MSG_CMD_SET_RX_EN,
	/* set loop enable， 0:disable， 1：enable */
	PHYTUART_MSG_CMD_SET_LOOP_EN,
	PHYTUART_MSG_CMD_SET_RTS_EN,
	PHYTUART_MSG_CMD_SET_CTS_EN,
	PHYTUART_MSG_CMD_SET_DTR_SET,
	PHYTUART_MSG_CMD_SET_RTS_SET,
	PHYTUART_MSG_CMD_SET_DTE_DCD_SET,
	PHYTUART_MSG_CMD_SET_DTE_RI_SET,
	PHYTUART_MSG_CMD_SET_RX_IM,
	PHYTUART_MSG_CMD_SET_TX_IM,
	PHYTUART_MSG_CMD_SET_ERROR_IM,

	PHYTUART_MSG_CMD_SET_MODEM_IM,
	PHYTUART_MSG_CMD_SET_STARTUP,
	PHYTUART_MSG_CMD_SET_HWINIT,
	PHYTUART_MSG_CMD_SET_MCTRL,
	PHYTUART_MSG_CMD_SET_TERMIOS,
	PHYTUART_MSG_CMD_SET_DISABLE_UART,
};

/* for trans subid to hex */
uint8_t getHexValue(enum phytuart_set_subid cmd)
{
	return (uint8_t)cmd;
}

