// SPDX-License-Identifier: GPL-2.0+
/* Driver for phytium uart v2
 * Copyright (c) 2020-2025, Phytium Technology, Co., Ltd.
 *
 */
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
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

#include "phytium-uart-v2.h"
#define cmd_id_type     uint8_t
#define cmd_subid_type  uint8_t
#define DEFAULT_CLK	10000000
#define	PHYT_UART_DRV_VER	"1.1.0"
/*
 * We wrap our port structure around the generic uart_port.
 */
struct phytium_uart_port {
	struct uart_port	port;
	unsigned int		old_cr;		/* state during shutdown */
	unsigned int		old_status;
	char			type[12];
	struct device		*dev;
	struct clk		*clk;
	void __iomem		*shmem_base;
	bool			m_buf_empty;
	bool			heartbeat_enable_flag;
	bool			debug_enable_flag;
	struct			timer_list alive_timer;
};

/* define msg format */
struct msg {
	u16 module_id;
	u8 cmd_id;
	u8 cmd_subid;
	u16 length;
	u16 complete;
	u8 data[120];
};

static unsigned int phytium_uart_read(const struct phytium_uart_port *pup,
	unsigned int reg)
{
	void __iomem *addr = pup->port.membase + reg;

	return readl_relaxed(addr);
}

static void phytium_uart_write(unsigned int val,
	const struct phytium_uart_port *pup, unsigned int reg)
{
	void __iomem *addr = pup->port.membase + reg;

	writel_relaxed(val, addr);
}

/* func for filling with msg */
static void msg_fill(struct msg *msg, u16 module_id,
		u8 cmd_id, u8 cmd_subid, u16 complete)
{
	msg->module_id = module_id;
	msg->cmd_id = cmd_id;
	msg->cmd_subid = cmd_subid;
	msg->complete = complete;

	memset(msg->data, 0, sizeof(msg->data));
}

static int tx_ring_buffer_is_full(struct phytium_uart_port *pup)
{
	u16 tx_tail, tx_head;

	tx_tail = phytium_uart_read(pup, REG_TX_TAIL) & BUFFER_POINTER_MASK;
	tx_head = phytium_uart_read(pup, REG_TX_HEAD) & BUFFER_POINTER_MASK;

	if (((tx_tail + 1) % TX_BUFFER_SIZE) == tx_head)
		return 1;

	return 0;
}

static int tx_ring_buffer_is_empty(struct phytium_uart_port *pup)
{
	u16 tx_tail, tx_head;

	tx_tail = phytium_uart_read(pup, REG_TX_TAIL) & BUFFER_POINTER_MASK;
	tx_head = phytium_uart_read(pup, REG_TX_HEAD) & BUFFER_POINTER_MASK;

	if (tx_tail == tx_head)
		return 1;

	return 0;
}

static void PHYT_MSG_INSERT(struct phytium_uart_port *pup, struct msg *msg)
{
	u16 tx_tail, tx_head;

	tx_tail = phytium_uart_read(pup, REG_TX_TAIL) & BUFFER_POINTER_MASK;
	tx_head = phytium_uart_read(pup, REG_TX_HEAD) & BUFFER_POINTER_MASK;

	pr_debug("TX_TAIL:%x, TX_HEAD:%x", tx_tail, tx_head);

	while (tx_ring_buffer_is_full(pup))
		cpu_relax();

	memcpy(pup->shmem_base + TX_MSG_SIZE * tx_tail,
			msg, sizeof(struct msg));

	/* updata tx tail pointer */
	tx_tail = (tx_tail + 1) % TX_BUFFER_SIZE;
	phytium_uart_write(tx_tail, pup, REG_TX_TAIL);

	phytium_uart_write(PHYT_INT_TRIGGER_BIT, pup, REG_PHYT_INT_STATE);
}


static int rx_ring_buffer_is_empty(struct phytium_uart_port *pup)
{
	u16 rx_tail, rx_head;

	rx_tail = phytium_uart_read(pup, REG_RX_TAIL) & BUFFER_POINTER_MASK;
	rx_head = phytium_uart_read(pup, REG_RX_HEAD) & BUFFER_POINTER_MASK;
	if (rx_tail == rx_head)
		return 1;
	return 0;
}


static void phytium_fifo_to_tty(struct phytium_uart_port *pup)
{
	struct msg *handler_msg;
	int sysrq;
	unsigned int ch, flag, fifotaken;
	u16 count = 0;
	u16 rx_head, rx_tail;

	while (!rx_ring_buffer_is_empty(pup)) {
		rx_head = (phytium_uart_read(pup, REG_RX_HEAD)
				& BUFFER_POINTER_MASK);
		rx_tail = (phytium_uart_read(pup, REG_RX_TAIL)
				& BUFFER_POINTER_MASK);

		/* get operator pointer of rv data msg */
		handler_msg = (struct msg *)(pup->shmem_base + TX_MSG_SIZE
				* TX_BUFFER_SIZE + RX_MSG_SIZE * rx_head);
		pr_debug("handler_msg = %p, rx_head=%d, rx_tail=%d\n",
				handler_msg, rx_head, rx_tail);
		if (!handler_msg) {
			pr_err("%s cannot get msg!\n", __func__);
			return;
		}
		count = handler_msg->length / 2;

		rx_head = (rx_head + 1) % RX_BUFFER_SIZE;
		phytium_uart_write(rx_head, pup, REG_RX_HEAD);

		for (fifotaken = 0; fifotaken != RX_DATA_MAXINUM; fifotaken++) {
			if (count == 0)
				break;
			count--;
			/* Take chars maxinum 60 * 2 bytes from the MSG */
			ch = (handler_msg->data[2 * fifotaken]);
			ch |= (handler_msg->data[2 * fifotaken + 1] << 8);
			ch |= DATA_DUMMY_RX;
			flag = TTY_NORMAL;
			pup->port.icount.rx++;

			if (unlikely(ch & DATA_ERROR)) {
				if (ch & DATA_BE) {
					ch &= ~(DATA_FE | DATA_PE);
					pup->port.icount.brk++;
					if (uart_handle_break(&pup->port))
						continue;
				} else if (ch & DATA_PE)
					pup->port.icount.parity++;
				else if (ch & DATA_FE)
					pup->port.icount.frame++;
				if (ch & DATA_OE)
					pup->port.icount.overrun++;

				ch &= pup->port.read_status_mask;

				if (ch & DATA_BE)
					flag = TTY_BREAK;
				else if (ch & DATA_PE)
					flag = TTY_PARITY;
				else if (ch & DATA_FE)
					flag = TTY_FRAME;
			}

			spin_unlock(&pup->port.lock);
			sysrq = uart_handle_sysrq_char(&pup->port,
					ch & CHAR_MASK);
			spin_lock(&pup->port.lock);

			if (!sysrq)
				uart_insert_char(&pup->port, ch,
						DATA_OE, ch, flag);
		}
	}
}

static void phytium_rx_chars(struct phytium_uart_port *pup)
__releases(&pup->port.lock)
__acquires(&pup->port.lock)
{
	phytium_fifo_to_tty(pup);

	spin_unlock(&pup->port.lock);

	tty_flip_buffer_push(&pup->port.state->port);

	spin_lock(&pup->port.lock);
}

static void phytium_stop_tx(struct uart_port *port)
{
	struct phytium_uart_port *pup =
	    container_of(port, struct phytium_uart_port, port);

	unsigned int int_mask;

	if (!tx_ring_buffer_is_empty(pup))
		return;
	/* mask tx msg tail pointer int*/
	int_mask = phytium_uart_read(pup, REG_RP_INT_MASK);
	phytium_uart_write(int_mask | PHYT_MSG_DATA_COMPLETED,
			pup, REG_RP_INT_MASK);

}

static bool phytium_tx_xchar(struct phytium_uart_port *pup, unsigned char c,
			    bool from_irq)
{
	struct msg msg;
	struct msg *handler_msg;
	u16 count = 1;

	msg_fill(&msg, UART_MODULE_ID, MSG_DATA, MSG_TX_DATA, 0);

	handler_msg = (struct msg *)(pup->shmem_base);

	if (unlikely(!from_irq) && !(handler_msg->complete & MSG_COMPLETE))
		return false;
	msg.data[0] = c;
	msg.length = count;

	/* set 1 to wait rv clear out */
	phytium_uart_write(1, pup, REG_CHECK_TX);

	PHYT_MSG_INSERT(pup, &msg);

	pup->port.icount.tx++;

	return true;

}

static bool phytium_tx_chars(struct phytium_uart_port *pup, bool from_irq)
{
	struct circ_buf *xmit = &pup->port.state->xmit;
	struct msg msg;
	int i = 0;
	int count = TX_DATA_MAXINUM;
	u16 datasize = 0;

	msg_fill(&msg, UART_MODULE_ID, MSG_DATA, MSG_TX_DATA, 0);

	if (pup->port.x_char) {
		if (!phytium_tx_xchar(pup, pup->port.x_char, from_irq))
			return true;
		pup->port.x_char = 0;
		--count;
	}

	if (uart_circ_empty(xmit) || uart_tx_stopped(&pup->port)) {
		phytium_stop_tx(&pup->port);
		return false;
	}

	if (tx_ring_buffer_is_full(pup))
		return false;
	do {
		if (count-- == 0)
			break;
		datasize++;

		msg.length = datasize;
		msg.data[datasize - 1] = xmit->buf[xmit->tail];

		pup->port.icount.tx++;
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
	} while (!uart_circ_empty(xmit));

	PHYT_MSG_INSERT(pup, &msg);

	/* if xmit is empty, break */
	if (uart_circ_empty(xmit))
		pr_debug("xmit is empty at i=%d\n", i);
	/* If tx_chars is called by start_tx or xmit
	 * data is full enough to fill msg, don't wakeup
	 * write again.
	 */
	if (uart_circ_chars_pending(xmit) <= TX_DATA_MAXINUM)
		uart_write_wakeup(&pup->port);
	if (uart_circ_empty(xmit)) {
		phytium_stop_tx(&pup->port);
		return false;
	}
	return true;
}

static void phytium_modem_status(struct phytium_uart_port *pup)
{
	struct msg msg;
	struct msg *handler_msg;
	unsigned int status, delta;
	u16 old_tx_tail;

	msg_fill(&msg, UART_MODULE_ID, MSG_GET, MSG_GET_MODEM, 0);

	old_tx_tail = phytium_uart_read(pup, REG_TX_TAIL) & BUFFER_POINTER_MASK;

	PHYT_MSG_INSERT(pup, &msg);

	handler_msg =
		(struct msg *)(pup->shmem_base + TX_MSG_SIZE * old_tx_tail);

	while (!tx_ring_buffer_is_empty(pup))
		cpu_relax();

	status = handler_msg->data[0];

	delta = status ^ pup->old_status;
	pup->old_status = status;

	if (!delta)
		return;

	if (delta & MODEM_DCD)
		uart_handle_dcd_change(&pup->port, status & MODEM_DCD);

	if (delta & MODEM_DSR)
		pup->port.icount.dsr++;

	if (delta & MODEM_CTS)
		uart_handle_cts_change(&pup->port, status & MODEM_CTS);

	wake_up_interruptible(&pup->port.state->port.delta_msr_wait);
}

static irqreturn_t phytium_uart_interrupt(int irq, void *uart_port)
{
	struct phytium_uart_port *pup = uart_port;
	unsigned long flags;
	unsigned int status;
	int handled = 0;

	spin_lock_irqsave(&pup->port.lock, flags);
	status = phytium_uart_read(pup, REG_RP_INT_STATE);
	if (status) {
		/* clear rx_tail,tx_data_complete interrupts */
		phytium_uart_write(status & ~(TX_HEAD_INT | RX_TAIL_INT |
			PHYT_MSG_DATA_COMPLETED | MODEM_INT),
				pup, REG_RP_INT_STATE);

		phytium_uart_write(TX_HEAD_INT | RX_TAIL_INT |
			PHYT_MSG_DATA_COMPLETED | MODEM_INT,
			pup, REG_RP_INT_STATE_CLR);

		if (status & RX_TAIL_INT)
			phytium_rx_chars(pup);

		if (status & MODEM_INT)
			phytium_modem_status(pup);

		if (status & PHYT_MSG_DATA_COMPLETED)
			phytium_tx_chars(pup, true);

		handled = 1;
	}
	spin_unlock_irqrestore(&pup->port.lock, flags);

	return IRQ_RETVAL(handled);
}

static unsigned int phytium_tx_empty(struct uart_port *port)
{
	struct phytium_uart_port *pup =
		container_of(port, struct phytium_uart_port, port);

	unsigned int status;

	status = tx_ring_buffer_is_full(pup);
	return status ? 0 : TIOCSER_TEMT;
}


static void phytium_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct phytium_uart_port *pup =
		container_of(port, struct phytium_uart_port, port);

	struct msg msg;
	unsigned int status;
	cmd_subid_type cmd_subid;
	enum phytuart_set_subid cmd = PHYTUART_MSG_CMD_SET_MCTRL;

	cmd_subid = getHexValue(cmd);

	msg_fill(&msg, UART_MODULE_ID, MSG_SET, cmd_subid, 0);

	msg.data[3] = (mctrl >> 24) & MSG_DATA_MASK;
	msg.data[2] = (mctrl >> 16) & MSG_DATA_MASK;
	msg.data[1] = (mctrl >> 8) & MSG_DATA_MASK;
	msg.data[0] = (mctrl) & MSG_DATA_MASK;

	if (port->status & UPSTAT_AUTORTS) {
		status = 1;
		msg.data[7] = (status >> 24) & MSG_DATA_MASK;
		msg.data[6] = (status >> 16) & MSG_DATA_MASK;
		msg.data[5] = (status >> 8) & MSG_DATA_MASK;
		msg.data[4] = (status) & MSG_DATA_MASK;
	}

	PHYT_MSG_INSERT(pup, &msg);

}

static unsigned int phytium_get_mctrl(struct uart_port *port)
{
	struct phytium_uart_port *pup =
	    container_of(port, struct phytium_uart_port, port);
	unsigned int cr = 0;
	unsigned int status;
	struct msg msg;
	struct msg *handler_msg;
	u16 old_tx_tail;

	old_tx_tail = phytium_uart_read(pup, REG_TX_TAIL) & BUFFER_POINTER_MASK;

	msg_fill(&msg, UART_MODULE_ID, MSG_GET, MSG_GET_MODEM, 0);

	PHYT_MSG_INSERT(pup, &msg);
	handler_msg =
		(struct msg *)(pup->shmem_base + TX_MSG_SIZE * old_tx_tail);
	while (!tx_ring_buffer_is_empty(pup))
		cpu_relax();
	status = handler_msg->data[0];

	if (status & MODEM_CTS)
		cr |= TIOCM_CTS;
	if (status & MODEM_DSR)
		cr |= TIOCM_DSR;
	if (status & MODEM_CAR)
		cr |= TIOCM_CAR;
	if (status & MODEM_RNG)
		cr |= TIOCM_RNG;
	if (status & MODEM_RTS)
		cr |= TIOCM_RTS;
	if (status & MODEM_DTR)
		cr |= TIOCM_DTR;
	if (status & MODEM_OUT1)
		cr |= TIOCM_OUT1;
	if (status & MODEM_OUT2)
		cr |= TIOCM_OUT2;

	return cr;
}

static void phytium_start_tx(struct uart_port *port)
{
	struct phytium_uart_port *pup =
		container_of(port, struct phytium_uart_port, port);
	unsigned int int_mask = 0;

	/* unmask tx_ring_buffer interrupt */
	if (phytium_tx_chars(pup, false)) {
		int_mask = phytium_uart_read(pup, REG_RP_INT_MASK);
		phytium_uart_write(int_mask & ~PHYT_MSG_DATA_COMPLETED,
				pup, REG_RP_INT_MASK);
	}
}

static void phytium_stop_rx(struct uart_port *port)
{

	struct phytium_uart_port *pup =
		container_of(port, struct phytium_uart_port, port);
	struct msg msg;
	cmd_subid_type cmd_subid;
	unsigned int int_mask;
	enum phytuart_set_subid cmd = PHYTUART_MSG_CMD_SET_ERROR_IM;

	cmd_subid = getHexValue(cmd);

	/* mask rx tail int */
	int_mask = phytium_uart_read(pup, REG_RP_INT_MASK);
	phytium_uart_write(int_mask | RX_TAIL_INT, pup, REG_RP_INT_MASK);

	msg_fill(&msg, UART_MODULE_ID, MSG_SET, cmd_subid, 0);

	PHYT_MSG_INSERT(pup, &msg);

}

static void phytium_throttle_rx(struct uart_port *port)
{
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	phytium_stop_rx(port);

	spin_unlock_irqrestore(&port->lock, flags);
}


static void phytium_enable_ms(struct uart_port *port)
{
	struct phytium_uart_port *pup =
		container_of(port, struct phytium_uart_port, port);
	struct msg msg;
	cmd_subid_type cmd_subid;

	enum phytuart_set_subid cmd = PHYTUART_MSG_CMD_SET_MODEM_IM;

	cmd_subid = getHexValue(cmd);

	msg_fill(&msg, UART_MODULE_ID, MSG_SET, cmd_subid, 0);

	msg.data[0] = 0xf;

	PHYT_MSG_INSERT(pup, &msg);

}

static void phytium_break_ctl(struct uart_port *port, int break_state)
{
	struct phytium_uart_port *pup =
		container_of(port, struct phytium_uart_port, port);
	struct msg msg;
	unsigned long flags;
	unsigned int ctrl = 1;
	cmd_subid_type cmd_subid;
	enum phytuart_set_subid cmd = PHYTUART_MSG_CMD_SET_BREAK_EN;

	cmd_subid = getHexValue(cmd);
	msg_fill(&msg, UART_MODULE_ID, MSG_SET, cmd_subid, 0);
	spin_lock_irqsave(&pup->port.lock, flags);
	if (break_state == -1) {

		msg.data[3] = (ctrl >> 24) & MSG_DATA_MASK;
		msg.data[2] = (ctrl >> 16) & MSG_DATA_MASK;
		msg.data[1] = (ctrl >> 8) & MSG_DATA_MASK;
		msg.data[0] = (ctrl) & MSG_DATA_MASK;
	}

	PHYT_MSG_INSERT(pup, &msg);

	spin_unlock_irqrestore(&pup->port.lock, flags);

}

static int phytium_hwinit(struct uart_port *port)
{
	struct phytium_uart_port *pup =
		container_of(port, struct phytium_uart_port, port);
	struct msg msg;
	int retval;
	u64 uart_clk;
	unsigned int int_mask;
	unsigned int status;
	cmd_subid_type cmd_subid;
	enum phytuart_set_subid cmd = PHYTUART_MSG_CMD_SET_HWINIT;

	cmd_subid = getHexValue(cmd);

	msg_fill(&msg, UART_MODULE_ID, MSG_SET, cmd_subid, 0);

	pinctrl_pm_select_default_state(port->dev);

	retval = clk_prepare_enable(pup->clk);
	if (retval)
		return retval;
	if (has_acpi_companion(pup->port.dev)) {
		device_property_read_u64(pup->port.dev, "clock-frequency",
				&uart_clk);
		if (uart_clk) {
			if (uart_clk > 100000000 || ('n' == uart_clk)) {
				dev_err(pup->port.dev, "uartclk get from acpi is error or NULL!\n");
				pup->port.uartclk = DEFAULT_CLK;
			} else
				pup->port.uartclk = (uint32_t)uart_clk;
		} else {
			dev_err(pup->port.dev, "have no acpi clk, use default clk!\n");
			pup->port.uartclk = DEFAULT_CLK;
		}
	} else
		pup->port.uartclk = clk_get_rate(pup->clk);

	PHYT_MSG_INSERT(pup, &msg);

	status = phytium_uart_read(pup, REG_RP_INT_STATE);
	phytium_uart_write(status & ~RX_TAIL_INT, pup, REG_RP_INT_STATE);
	phytium_uart_write(RX_TAIL_INT, pup, REG_RP_INT_STATE_CLR);
	/* unmask rx tail int */
	int_mask = phytium_uart_read(pup, REG_RP_INT_MASK);
	phytium_uart_write(int_mask & ~RX_TAIL_INT, pup, REG_RP_INT_MASK);

	return 0;
}

static int phytium_uart_allocate_irq(struct phytium_uart_port *pup)
{
	if (!pup->port.irq)
		return -EINVAL;

	return request_irq(pup->port.irq, phytium_uart_interrupt,
			IRQF_SHARED, DRV_NAME, pup);
}

static void phytium_enable_interrupts(struct phytium_uart_port *pup)
{
	unsigned long flags;
	unsigned int i;
	u16 rx_head, rx_tail;
	unsigned int status;
	unsigned int int_mask;

	unsigned int buffer_size = RX_BUFFER_SIZE;

	spin_lock_irqsave(&pup->port.lock, flags);

	/* clear out rx-int-status */
	status = phytium_uart_read(pup, REG_RP_INT_STATE);
	phytium_uart_write(status & ~RX_TAIL_INT, pup, REG_RP_INT_STATE);
	/* when enable int, first empty rx ring buffer */
	for (i = 0; i < buffer_size * 2; i++) {
		if (rx_ring_buffer_is_empty(pup))
			break;
		rx_head = phytium_uart_read(pup, REG_RX_HEAD)
				& BUFFER_POINTER_MASK;
		rx_head = (rx_head + 1) % buffer_size;
		phytium_uart_write(rx_head, pup, REG_RX_HEAD);
	}

	/* enable rx_tail int */
	rx_tail = phytium_uart_read(pup, REG_RX_TAIL);
	phytium_uart_write(rx_tail | RX_TAIL_INT_ENABLE, pup, REG_RX_TAIL);

	/* unmask rx tail int */
	int_mask = phytium_uart_read(pup, REG_RP_INT_MASK);
	phytium_uart_write(int_mask & ~RX_TAIL_INT, pup, REG_RP_INT_MASK);
	spin_unlock_irqrestore(&pup->port.lock, flags);
}

static void phytium_unthrottle_rx(struct uart_port *port)
{
	struct phytium_uart_port *pup =
		container_of(port, struct phytium_uart_port, port);

	phytium_enable_interrupts(pup);
}


static int phytium_startup(struct uart_port *port)
{
	struct phytium_uart_port *pup =
		container_of(port, struct phytium_uart_port, port);
	struct msg msg;
	struct msg *handler_msg;
	cmd_subid_type cmd_subid;
	int ret = 0;
	u16 old_tx_tail;

	enum phytuart_set_subid cmd = PHYTUART_MSG_CMD_SET_STARTUP;

	cmd_subid = getHexValue(cmd);

	ret = phytium_hwinit(port);
	if (ret)
		goto out;

	ret = phytium_uart_allocate_irq(pup);
	if (ret)
		goto out;

	spin_lock_irq(&pup->port.lock);

	msg_fill(&msg, UART_MODULE_ID, MSG_SET, cmd_subid, 0);

	PHYT_MSG_INSERT(pup, &msg);

	spin_unlock_irq(&pup->port.lock);

	msg_fill(&msg, UART_MODULE_ID, MSG_GET, MSG_GET_MODEM, 0);

	old_tx_tail = phytium_uart_read(pup, REG_TX_TAIL) & BUFFER_POINTER_MASK;

	PHYT_MSG_INSERT(pup, &msg);

	handler_msg = (struct msg *)
		(pup->shmem_base + TX_MSG_SIZE * old_tx_tail);

	while (!tx_ring_buffer_is_empty(pup))
		cpu_relax();

	pup->old_status = handler_msg->data[0];

	phytium_enable_interrupts(pup);

	return 0;
out:
	return ret;
}

static void phytium_disable_uart(struct phytium_uart_port *pup)
{

	struct msg msg;
	cmd_subid_type cmd_subid;

	enum phytuart_set_subid cmd = PHYTUART_MSG_CMD_SET_DISABLE_UART;

	cmd_subid = getHexValue(cmd);

	msg_fill(&msg, UART_MODULE_ID, MSG_SET, cmd_subid, 0);

	/* set none flow control status */
	pup->port.status &= ~(UPSTAT_AUTOCTS | UPSTAT_AUTORTS);
	spin_lock_irq(&pup->port.lock);

	PHYT_MSG_INSERT(pup, &msg);

	spin_unlock_irq(&pup->port.lock);

}

static void phytium_disable_interrupts(struct phytium_uart_port *pup)
{
	struct msg msg;

	/* reset uart */
	msg_fill(&msg, UART_MODULE_ID, MSG_DEFAULT, MSG_DEFAULT_SUBID, 0);

	spin_lock_irq(&pup->port.lock);

	PHYT_MSG_INSERT(pup, &msg);

	/* clear all RP INT STATUS */
	phytium_uart_write(0, pup, REG_RP_INT_STATE);
	phytium_uart_write(0xffff, pup, REG_RP_INT_STATE_CLR);
	spin_unlock_irq(&pup->port.lock);
}

static void phytium_shutdown(struct uart_port *port)
{
	struct phytium_uart_port *pup =
		container_of(port, struct phytium_uart_port, port);

	phytium_disable_interrupts(pup);

	free_irq(pup->port.irq, pup);

	phytium_disable_uart(pup);

	/* Shut down the clock producer */
	clk_disable_unprepare(pup->clk);

	/* Optionally let pins go into sleep states */
	pinctrl_pm_select_sleep_state(port->dev);

}

static void
phytium_setup_status_masks(struct uart_port *port, struct ktermios *termios)
{
	port->read_status_mask = DATA_OE | CHAR_MASK;

	if (termios->c_iflag & INPCK)
		port->read_status_mask |= DATA_FE | DATA_PE;

	if (termios->c_iflag & (IGNBRK | BRKINT | PARMRK))
		port->read_status_mask |= DATA_BE;

	/*
	 * Characters to ignore
	 */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= DATA_FE | DATA_PE;

	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |= DATA_BE;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |= DATA_OE;
	}

	/*
	 * Ignore all characters if CREAD is not set.
	 */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= DATA_DUMMY_RX;
}

static void
phytium_set_termios(struct uart_port *port, struct ktermios *termios,
		const struct ktermios *old)
{
	struct phytium_uart_port *pup =
		container_of(port, struct phytium_uart_port, port);

	struct msg msg;
	u64 uart_clk;
	unsigned long flags;
	unsigned int baud, quot;
	u8 databits = 0;
	u8 stopbits = 0;
	u8 parity_en = 0;
	u8 parodd = 1;
	u8 cmspar = 0;
	u8 crtscts = 0;
	cmd_subid_type cmd_subid1, cmd_subid2;

	enum phytuart_set_subid cmd1 = PHYTUART_MSG_CMD_SET_BAUD;
	enum phytuart_set_subid cmd2 = PHYTUART_MSG_CMD_SET_TERMIOS;

	cmd_subid1 = getHexValue(cmd1);
	cmd_subid2 = getHexValue(cmd2);

	msg_fill(&msg, UART_MODULE_ID, MSG_SET, cmd_subid1, 0);

	spin_lock_irqsave(&port->lock, flags);
	/* Ask the core to calculate the divisor for us. */
	if (has_acpi_companion(pup->port.dev)) {
		device_property_read_u64(pup->port.dev, "clock-frequency",
				&uart_clk);
		if (uart_clk) {
			if (uart_clk > 100000000 || ('n' == uart_clk)) {
				dev_err(pup->port.dev, "uartclk get from acpi is error or NULL!\n");
				port->uartclk = DEFAULT_CLK;
			} else
				port->uartclk = (uint32_t)uart_clk;
		} else {
			dev_err(pup->port.dev, "have no acpi clk, use default clk!\n");
			port->uartclk = DEFAULT_CLK;
		}
	}
	baud = uart_get_baud_rate(port, termios, old, 0,
					port->uartclk/16);
	dev_info(pup->port.dev, "get baud to set:%d\n", baud);
	if (baud > port->uartclk/16)
		quot = DIV_ROUND_CLOSEST(port->uartclk * 8, baud);
	else
		quot = DIV_ROUND_CLOSEST(port->uartclk * 4, baud);

	/* set baud */
	msg.data[7] = (port->uartclk >> 24) & MSG_DATA_MASK;
	msg.data[6] = (port->uartclk >> 16) & MSG_DATA_MASK;
	msg.data[5] = (port->uartclk >> 8) & MSG_DATA_MASK;
	msg.data[4] = (port->uartclk) & MSG_DATA_MASK;
	msg.data[3] = (baud >> 24) & MSG_DATA_MASK;
	msg.data[2] = (baud >> 16) & MSG_DATA_MASK;
	msg.data[1] = (baud >> 8) & MSG_DATA_MASK;
	msg.data[0] = (baud) & MSG_DATA_MASK;

	PHYT_MSG_INSERT(pup, &msg);

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		databits = 0;
		break;
	case CS6:
		databits = 1;
		break;
	case CS7:
		databits = 2;
		break;
	default: /* CS8 */
		databits = 3;
		break;
	}

	if (termios->c_cflag & CSTOPB)
		stopbits = 1;
	if (termios->c_cflag & PARENB) {
		parity_en = 1;
		if (!(termios->c_cflag & PARODD))
			parodd = 0;
		if (termios->c_cflag & CMSPAR)
			cmspar = 1;


	}

	/*
	 * Update the per-port timeout.
	 */

	uart_update_timeout(port, termios->c_cflag, baud);

	phytium_setup_status_masks(port, termios);

	if (UART_ENABLE_MS(port, termios->c_cflag))
		phytium_enable_ms(port);

	if (termios->c_cflag & CRTSCTS) {
		crtscts = 1;
		port->status |= UPSTAT_AUTOCTS | UPSTAT_AUTORTS;
	} else {
		crtscts = 0;
		port->status &= ~(UPSTAT_AUTOCTS | UPSTAT_AUTORTS);
	}


	msg_fill(&msg, UART_MODULE_ID, MSG_SET, cmd_subid2, 0);

	msg.data[0] = databits & MSG_DATA_MASK;
	msg.data[1] = stopbits & MSG_DATA_MASK;
	msg.data[2] = parity_en & MSG_DATA_MASK;
	msg.data[3] = parodd & MSG_DATA_MASK;
	msg.data[4] = cmspar & MSG_DATA_MASK;
	msg.data[5] = crtscts & MSG_DATA_MASK;

	PHYT_MSG_INSERT(pup, &msg);

	spin_unlock_irqrestore(&port->lock, flags);
}

static const char *phytium_type(struct uart_port *port)
{
	struct phytium_uart_port *pup =
		container_of(port, struct phytium_uart_port, port);

	return pup->port.type == PORT_PHYTIUM ? pup->type : NULL;
}

static void phytium_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE)
		port->type = PORT_PHYTIUM;
}

static int phytium_verify_port(struct uart_port *port,
		struct serial_struct *ser)
{
	int ret = 0;

	if (ser->type != PORT_UNKNOWN && ser->type != PORT_PHYTIUM)
		ret = -EINVAL;
	if (ser->irq < 0 || ser->irq >= nr_irqs)
		ret = -EINVAL;
	if (ser->baud_base < 9600)
		ret = -EINVAL;
	if (port->mapbase != (unsigned long) ser->iomem_base)
		ret = -EINVAL;
	return ret;
}

static const struct uart_ops phytium_uart_ops = {
	.tx_empty	= phytium_tx_empty,
	.set_mctrl	= phytium_set_mctrl,
	.get_mctrl	= phytium_get_mctrl,
	.stop_tx	= phytium_stop_tx,
	.start_tx	= phytium_start_tx,
	.stop_rx	= phytium_stop_rx,
	.throttle	= phytium_throttle_rx,
	.unthrottle	= phytium_unthrottle_rx,
	.enable_ms	= phytium_enable_ms,
	.break_ctl	= phytium_break_ctl,
	.startup	= phytium_startup,
	.shutdown	= phytium_shutdown,
	.set_termios	= phytium_set_termios,
	.type		= phytium_type,
	.config_port	= phytium_config_port,
	.verify_port	= phytium_verify_port,
};

static struct phytium_uart_port *uart_ports[UART_NR];

static struct uart_driver phytium_uart = {
	.owner		= THIS_MODULE,
	.driver_name	= DRV_NAME,
	.dev_name	= "ttyVS",
	.nr		= UART_NR,
};

static void phytium_unregister_port(struct phytium_uart_port *pup)
{
	int i;
	bool busy = false;

	for (i = 0; i < ARRAY_SIZE(uart_ports); i++) {
		if (uart_ports[i] == pup)
			uart_ports[i] = NULL;
		else if (uart_ports[i])
			busy = true;
	}

	if (!busy)
		uart_unregister_driver(&phytium_uart);
}

static int phytium_find_free_port(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(uart_ports); i++)
		if (uart_ports[i] == NULL)
			return i;

	return -EBUSY;
}

static int phytium_register_port(struct phytium_uart_port *pup)
{
	struct msg msg;
	int rc, i;

	/* mask TX_HEAD_INT at the beginning, because now we are
	 * unable to deal with it yet
	 */
	phytium_uart_write(TX_HEAD_INT, pup, REG_RP_INT_MASK);

	msg_fill(&msg, UART_MODULE_ID, MSG_DEFAULT, MSG_DEFAULT, 0);
	PHYT_MSG_INSERT(pup, &msg);

	/* clear all redundant RP INT STATUS at the beginning */
	phytium_uart_write(0, pup, REG_RP_INT_STATE);
	phytium_uart_write(0xffff, pup, REG_RP_INT_STATE_CLR);
	if (!phytium_uart.state) {
		rc = uart_register_driver(&phytium_uart);
		if (rc < 0) {
			dev_err(pup->port.dev,
				"Failed to register Phytium platform UART driver\n");
			for (i = 0; i < ARRAY_SIZE(uart_ports); i++)
				if (uart_ports[i] == pup)
					uart_ports[i] = NULL;
			return rc;
		}
	}

	rc = uart_add_one_port(&phytium_uart, &pup->port);
	if (rc)
		phytium_unregister_port(pup);

	return rc;
}

#if defined(SERIAL_PHYTIUM_V2_DEBUG)
static int phytium_uart_enable_debug(struct phytium_uart_port *pup,
		bool new_enable_flag)
{
	u32 dbg_regval;
	static bool old_enable_flag;

	if (old_enable_flag == new_enable_flag) {
		pr_warn("PHYUART:set enable debug with repeative operation.\n");
		return -1;
	}
	old_enable_flag = new_enable_flag;
	dbg_regval = phytium_uart_read(pup, PHYUART_DBG_REG);
	pr_info("PHYUART: %s debug_regval %x\n", __func__, dbg_regval);
	if (!old_enable_flag && (dbg_regval & PHYUART_DBG_ENABLE_MASK))
		dbg_regval &= ~PHYUART_DBG_ENABLE_MASK;
	else if (dbg_regval && !(dbg_regval & PHYUART_DBG_ENABLE_MASK))
		dbg_regval |= PHYUART_DBG_ENABLE_MASK;

	pr_info("final PHYUART: %s debug_regval %x\n", __func__, dbg_regval);
	phytium_uart_write(dbg_regval, pup, PHYUART_DBG_REG);
	return 0;
}

static int phytium_uart_enable_heartbeat(struct phytium_uart_port *pup,
		bool new_heartbeat_flag)
{
	u32 dbg_regval;
	static bool old_heartbeat_flag;

	if (old_heartbeat_flag == new_heartbeat_flag) {
		pr_warn("PHYUART:set heartbeat with repeative operation.\n");
		return -1;
	}
	old_heartbeat_flag = new_heartbeat_flag;
	dbg_regval = phytium_uart_read(pup, PHYUART_DBG_REG);
	pr_info("PHYUART: %s dbg_regval %x\n", __func__, dbg_regval);
	if (!old_heartbeat_flag && (dbg_regval & PHYUART_DBG_HEARTBEAT_MASK))
		dbg_regval &= ~PHYUART_DBG_HEARTBEAT_MASK;
	else if (dbg_regval && !(dbg_regval & PHYUART_DBG_HEARTBEAT_MASK))
		dbg_regval |= PHYUART_DBG_HEARTBEAT_MASK
			| PHYUART_DBG_HEARTBEAT_ENABLE_MASK;

	pr_info("final PHYUART: %s dbg_regval %x\n", __func__, dbg_regval);
	phytium_uart_write(dbg_regval, pup, PHYUART_DBG_REG);
	return 0;
}

static void alive_timer_routine(struct timer_list *tlist)
{
	struct phytium_uart_port *pup;
	u32 dbg_regval;

	pup = from_timer(pup, tlist, alive_timer);
	if (!pup) {
		pr_err("PHYUART: get uart port error.\n");
		return;
	}

	dbg_regval = phytium_uart_read(pup, PHYUART_DBG_REG);
	pr_debug("PHYUART: %s debug_regval 0x%x\n", __func__, dbg_regval);
	phytium_uart_write(dbg_regval | PHYUART_DBG_HEARTBEAT_MASK,
			pup, PHYUART_DBG_REG);
	mod_timer(&pup->alive_timer, jiffies + msecs_to_jiffies(5000));
}

static ssize_t debug_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct phytium_uart_port *pup = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", pup->debug_enable_flag);
}

static ssize_t debug_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct phytium_uart_port *pup = dev_get_drvdata(dev);
	int enable;
	int ret;

	ret = sscanf(buf, "%d\n", &enable);
	if (ret == 0) {
		ret = -EINVAL;
		return ret;
	}
	pup->debug_enable_flag = enable;
	phytium_uart_enable_debug(pup, pup->debug_enable_flag);
	return count;
}

static ssize_t heartbeat_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct phytium_uart_port *pup = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", pup->heartbeat_enable_flag);
}

static ssize_t heartbeat_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct phytium_uart_port *pup = dev_get_drvdata(dev);
	int heartbeat_enable;
	int ret;

	ret = sscanf(buf, "%d\n", &heartbeat_enable);
	if (ret == 0) {
		ret = -EINVAL;
		return ret;
	}
	pup->heartbeat_enable_flag = heartbeat_enable;
	phytium_uart_enable_heartbeat(pup, pup->heartbeat_enable_flag);
	return count;
}
static DEVICE_ATTR_RW(debug_enable);
static DEVICE_ATTR_RW(heartbeat_enable);
#endif
static int phytium_uart_probe(struct platform_device *pdev)
{
	struct phytium_uart_port *pup;
	struct resource *res_rf, *res_sm;
	int portnr, ret;
	u64 uart_clk;

	portnr = phytium_find_free_port();
	if (portnr < 0)
		return portnr;

	pup = devm_kzalloc(&pdev->dev, sizeof(struct phytium_uart_port),
			GFP_KERNEL);
	if (!pup)
		return -ENOMEM;

	if (pdev->dev.of_node) {
		pup->clk = devm_clk_get(&pdev->dev, "uartclk");
		if (IS_ERR(pup->clk)) {
			dev_err(&pdev->dev, "Device clock not found.\n");
			ret = PTR_ERR(pup->clk);
			goto free;
		}
	}

	res_rf = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_rf) {
		dev_err(&pdev->dev, "UNABLE TO GET REGFILE RESOURCE!\n");
		goto free;
	}

	res_sm = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res_sm) {
		dev_err(&pdev->dev, "UNABLE TO GET SHMEM RESOURCE!\n");
		goto free;
	}

	pup->shmem_base = devm_ioremap_resource(&pdev->dev, res_sm);

	pup->dev = &pdev->dev;
	pup->port.irq = platform_get_irq(pdev, 0);

	ret = clk_prepare_enable(pup->clk);
	if (ret) {
		dev_err(&pdev->dev, "PHYUART:clk_prepare_enable error.\n");
		goto free;
	}

	pup->port.dev = &pdev->dev;
	if (has_acpi_companion(pup->port.dev)) {
		device_property_read_u64(pup->port.dev, "clock-frequency",
					&uart_clk);
		if (uart_clk) {
			if (uart_clk > 100000000 || ('n' == uart_clk)) {
				dev_err(pup->port.dev, "uartclk get from acpi is error or NULL!\n");
				pup->port.uartclk = DEFAULT_CLK;
			} else
				pup->port.uartclk = (uint32_t)uart_clk;
		} else {
			dev_err(pup->port.dev, "have no acpi clk, use default clk!\n");
			pup->port.uartclk = DEFAULT_CLK;
		}
	} else
		pup->port.uartclk = clk_get_rate(pup->clk);
	pup->port.mapbase = res_rf->start;
	pup->port.membase = devm_ioremap_resource(&pdev->dev, res_rf);
	pup->port.iotype = UPIO_MEM32;
	pup->port.ops = &phytium_uart_ops;
	pup->port.dev = &pdev->dev;
	pup->port.fifosize = UART_FIFOSIZE;
	pup->port.flags = UPF_BOOT_AUTOCONF;
	pup->port.line = portnr;
	uart_ports[portnr] = pup;
	pup->old_cr = 0;
	pup->m_buf_empty = true;
	snprintf(pup->type, sizeof(pup->type), "phytium,uart-v2");
#if defined(SERIAL_PHYTIUM_V2_DEBUG)
	pup->debug_enable_flag = false;
	pup->heartbeat_enable_flag = false;

	phytium_uart_enable_heartbeat(pup, true);
	phytium_uart_enable_debug(pup, true);

	pup->alive_timer.expires = jiffies + msecs_to_jiffies(5000);
	timer_setup(&pup->alive_timer, alive_timer_routine, 0);
	add_timer(&pup->alive_timer);
	ret = device_create_file(&pdev->dev,
				&dev_attr_debug_enable);
	if (ret < 0) {
		dev_err(&pdev->dev, "PHYUART: device_create_debug file error.\n");
		goto debug_enable_free;
	}
	ret = device_create_file(&pdev->dev,
				&dev_attr_heartbeat_enable);
	if (ret < 0) {
		dev_err(&pdev->dev, "PHYUART: device_create_heartbeat file error.\n");
		goto heartbeat_enable_free;
	}
#endif
	platform_set_drvdata(pdev, pup);
	return phytium_register_port(pup);

#if defined(SERIAL_PHYTIUM_V2_DEBUG)
heartbeat_enable_free:
	device_remove_file(pup->dev, &dev_attr_heartbeat_enable);
debug_enable_free:
	device_remove_file(pup->dev, &dev_attr_debug_enable);
#endif
free:
	kfree(pup);
	return -1;
}

static int phytium_uart_remove(struct platform_device *pdev)
{
	struct phytium_uart_port *pup = platform_get_drvdata(pdev);

	uart_remove_one_port(&phytium_uart, &pup->port);

	phytium_unregister_port(pup);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int phytium_uart_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct phytium_uart_port *pup = platform_get_drvdata(pdev);

	if (pup)
		uart_suspend_port(&phytium_uart, &pup->port);

	return 0;
}

static int phytium_uart_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct phytium_uart_port *pup = platform_get_drvdata(pdev);

	if (pup)
		uart_resume_port(&phytium_uart, &pup->port);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(phytium_dev_pm_ops,
		phytium_uart_suspend, phytium_uart_resume);

/* Match table for OF platform binding */
static const struct of_device_id phytium_uart_of_ids[] = {
	{ .compatible = "phytium,uart-2.0", },
	{ /* end of list */ },
};
MODULE_DEVICE_TABLE(of, phytium_uart_of_ids);

static const struct acpi_device_id __maybe_unused phytium_uart_acpi_match[] = {
	{ "PHYT0055", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, phytium_uart_acpi_match);

static struct platform_driver phytium_uart_driver = {
	.probe          = phytium_uart_probe,
	.remove         = phytium_uart_remove,
	.driver	= {
		.name = DRV_NAME,
		.of_match_table	= phytium_uart_of_ids,
		.acpi_match_table = ACPI_PTR(phytium_uart_acpi_match),
		.pm	= &phytium_dev_pm_ops,
	},
};

static int __init phytium_uart_init(void)
{
	pr_info("Serial: Phytium uart v2 driver\n");
	return platform_driver_register(&phytium_uart_driver);
}

static void __exit phytium_uart_exit(void)
{
	platform_driver_unregister(&phytium_uart_driver);
}
arch_initcall(phytium_uart_init);
module_exit(phytium_uart_exit);

MODULE_AUTHOR("Lan Hengyu <lanhengyu1395@phytium.com.cn>");
MODULE_DESCRIPTION("Phytium serial port driver for uart v2");
MODULE_VERSION(PHYT_UART_DRV_VER);
MODULE_LICENSE("GPL");
