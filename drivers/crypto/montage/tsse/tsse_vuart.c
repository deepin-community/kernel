// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <linux/mutex.h>
#include <linux/nmi.h>
#include <linux/version.h>
#include <linux/bitops.h>
#include <linux/slab.h>

#include "tsse_dev.h"
#include "tsse_vuart_regs.h"
#include "tsse_vuart.h"

#ifdef DEBUG
#define VUART_PRINT(fmt, ...) pr_info(fmt, ##__VA_ARGS__)
#else
#define VUART_PRINT(fmt, ...)
#endif

#define TSSE_VUART_BAUD (38400)
#define TSSE_VUART_MAX_RX_COUNT (256)
#define BOTH_EMPTY (VUART_FSR_TXFIFOE | VUART_FSR_RXFIFO)
struct tsse_vuart {
	struct uart_port port;
	unsigned int tx_threshold;
	unsigned int rx_threshold;
	unsigned int tx_loadsz;
	unsigned char shutdown;
	unsigned char confige_done;
};

#define SERIAL_LSR_NAME "tsse_vuart"

static struct uart_driver g_vuart_reg = {
	.owner = THIS_MODULE,
	.driver_name = SERIAL_LSR_NAME,
	.dev_name = "ttyTSSE",
	.nr = TSSE_VUART_MAX_DEV,
};

static unsigned int g_trigger_level[4] = { 0, 31, 63, 111 };
static unsigned long g_line[TSSE_VUART_BITMAP_SIZE];

static unsigned int vuart_serial_in(struct uart_port *port, int offset)
{
	unsigned int ret = le32_to_cpu(readl(port->membase + offset));
#ifdef DEBUG
	pr_debug("%s offset 0x%x, v 0x%x\n", __func__, offset, ret);
#endif
	return ret;
}

static void vuart_serial_out(struct uart_port *port, int offset, int value)
{
#ifdef DEBUG
	pr_debug("%s offset 0x%x, v 0x%x\n", __func__, offset, value);
#endif
	value = cpu_to_le32(value);
	writel(value, port->membase + offset);
}

static void vuart_wait_for_xmitr(struct uart_port *port)
{
	unsigned int status, tmout = 10000;

	for (;;) {
		status = vuart_serial_in(port, VUART_FSR);
		if (FIELD_GET(VUART_FSR_TXFIFOE, status))
			break;
		if (--tmout == 0) {
			pr_err("%s:timeout(10ms), TX is not empty.\n",
			       __func__);
			break;
		}
		udelay(1);
		touch_nmi_watchdog();
	}
}

static unsigned int vuart_tx_empty(struct uart_port *port)
{
	unsigned long flags;
	unsigned int lsr;

	spin_lock_irqsave(&port->lock, flags);
	lsr = vuart_serial_in(port, VUART_FSR);
	spin_unlock_irqrestore(&port->lock, flags);

	return (lsr & BOTH_EMPTY) == BOTH_EMPTY ? TIOCSER_TEMT : 0;
}

static void vuart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
}

static unsigned int vuart_get_mctrl(struct uart_port *port)
{
	return 0;
}

static void vuart_stop_tx(struct uart_port *port)
{
	unsigned int ier;
	struct tsse_vuart *vuart = (struct tsse_vuart *)port;

	if (!vuart->confige_done)
		return;

	ier = vuart_serial_in(port, VUART_IER);
	ier &= ~VUART_IER_HETXEI;
	vuart_serial_out(port, VUART_IER, ier);
}

static void vuart_tx_chars(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;
	struct tsse_vuart *vuart = (struct tsse_vuart *)port;
	int count;

	if (port->x_char) {
		pr_err("x_char %d\n", port->x_char);
		return;
	}

	if (uart_tx_stopped(port) || uart_circ_empty(xmit)) {
		vuart_stop_tx(port);
		return;
	}

	count = vuart->tx_loadsz;
	do {
		vuart_serial_out(port, VUART_TX, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);
}

static void vuart_start_tx(struct uart_port *port)
{
	unsigned int ier, fsr;
	struct tsse_vuart *vuart = (struct tsse_vuart *)port;

	if (!vuart->confige_done)
		return;

	if (uart_tx_stopped(port)) {
		vuart_stop_tx(port);
		return;
	}

	fsr = vuart_serial_in(port, VUART_FSR);
	VUART_PRINT("==>Existing Data number in TX FIFO %ld\n",
		    FIELD_GET(VUART_FSR_TFIFODN, fsr));
	VUART_PRINT("==>Existing Data number in RX FIFO %ld\n",
		    FIELD_GET(VUART_FSR_RFIFODN, fsr));
	if (fsr & VUART_FSR_TXFIFOE)
		vuart_tx_chars(port);
	ier = vuart_serial_in(port, VUART_IER);
	ier |= VUART_IER_HETXEI | VUART_IER_HETXUI;
	vuart_serial_out(port, VUART_IER, ier);
}

static void vuart_throttle(struct uart_port *port)
{
}

static void vuart_unthrottle(struct uart_port *port)
{
}

static void vuart_stop_rx(struct uart_port *port)
{
	unsigned int ier;
	struct tsse_vuart *vuart = (struct tsse_vuart *)port;

	if (!vuart->confige_done)
		return;

	ier = vuart_serial_in(port, VUART_IER);
	ier &= ~(VUART_IER_HERXTOI | VUART_IER_HETXDRI | VUART_IER_HERXOI);
	vuart_serial_out(port, VUART_IER, ier);
}

static void vuart_enable_ms(struct uart_port *port)
{
}

static void vuart_break_ctl(struct uart_port *port, int ctl)
{
}

static irqreturn_t vuart_interrupt(int irq, void *port)
{
	int handled = 0;
	struct uart_port *p = (struct uart_port *)port;

	if (p->handle_irq(p))
		handled = 1;

	return IRQ_RETVAL(handled);
}

static void vuart_check_config_done(struct uart_port *port)
{
	struct tsse_vuart *vuart = (struct tsse_vuart *)port;

	if (vuart_serial_in(port, VUART_CFG) == 1)
		vuart->confige_done = 1;
}

static int vuart_startup(struct uart_port *port)
{
	unsigned int ret, hcr, ier, fcr = 0;
	struct tsse_vuart *vuart = (struct tsse_vuart *)port;

	if (port->flags & UPF_SHARE_IRQ)
		port->irqflags |= IRQF_SHARED;
	ret = request_irq(port->irq, vuart_interrupt, port->irqflags,
			  "tsse_uart", port);
	if (ret)
		return ret;

	hcr = vuart_serial_in(port, VUART_HCR);
	vuart->rx_threshold = FIELD_GET(VUART_HCR_RFIFOT, hcr);
	vuart->tx_threshold = FIELD_GET(VUART_HCR_TFIFOT, hcr);
	fcr |= FIELD_PREP(VUART_FCR_RFIFOT, vuart->rx_threshold);
	fcr |= FIELD_PREP(VUART_FCR_TFIFOT, vuart->tx_threshold);
	fcr |= FIELD_PREP(VUART_FCR_TFIFORST, 1);
	fcr |= FIELD_PREP(VUART_FCR_RFIFORST, 1);
	vuart_serial_out(port, VUART_FCR, fcr);

	vuart->rx_threshold = g_trigger_level[vuart->rx_threshold];
	vuart->tx_threshold = g_trigger_level[vuart->tx_threshold];

	vuart_check_config_done(port);
	ier = vuart_serial_in(port, VUART_IER);
	ier |= VUART_IER_CCFGDI | VUART_IER_HETXDRI | VUART_IER_HERXTOI;
	vuart_serial_out(port, VUART_IER, ier);

	vuart_serial_out(port, VUART_SCR, FIELD_PREP(VUART_SCR_SCR, 1));

	vuart->shutdown = 0;

	return 0;
}

static void vuart_shutdown(struct uart_port *port)
{
	struct tsse_vuart *vuart = (struct tsse_vuart *)port;

	vuart->shutdown = 1;
	vuart_stop_rx(port);
	vuart_stop_tx(port);
	free_irq(port->irq, port);
	vuart_serial_out(port, VUART_SCR, 0);
}

static void vuart_set_termios(struct uart_port *port, struct ktermios *termios,
			      const struct ktermios *old)
{
	unsigned int baud;
	unsigned long flags;

	if ((termios->c_cflag & CSIZE) != CS8)
		pr_err("Warning:termios is not CS8.\n");

	baud = uart_get_baud_rate(port, termios, old, 0, TSSE_VUART_BAUD);

	spin_lock_irqsave(&port->lock, flags);
	uart_update_timeout(port, termios->c_cflag, baud);

	port->read_status_mask =
		VUART_FSR_TXFIFOE | VUART_FSR_TXOE | VUART_FSR_RXDR;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= VUART_FSR_RXUE;

	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= VUART_FSR_RXUE;
	if (termios->c_iflag & (IGNBRK | IGNPAR))
		port->ignore_status_mask |= VUART_FSR_TXFIFOE;

	if ((termios->c_cflag & CREAD) == 0) {
		port->ignore_status_mask |= VUART_FSR_RXDR;
		pr_err("Warning:termios is not set CREAD.\n");
	}

	spin_unlock_irqrestore(&port->lock, flags);

	if (tty_termios_baud_rate(termios))
		tty_termios_encode_baud_rate(termios, baud, baud);
}

static void vuart_set_ldisc(struct uart_port *port, struct ktermios *ktermios)
{
}

static void vuart_pm(struct uart_port *port, unsigned int state,
		     unsigned int oldstate)
{
}

static void vuart_release_port(struct uart_port *port)
{
}

static int vuart_request_port(struct uart_port *port)
{
	return 0;
}

static void vuart_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE)
		port->type = PORT_16550A;
}

static int vuart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	if (port->type != PORT_16550A)
		return -EINVAL;
	return 0;
}

#ifdef CONFIG_CONSOLE_POLL
static void vuart_poll_put_char(struct uart_port *port, unsigned char c)
{
	unsigned int ier_save;

	ier_save = vuart_serial_in(port, VUART_IER);
	vuart_wait_for_xmitr(port);
	vuart_serial_out(port, VUART_TX, c);

	vuart_wait_for_xmitr(port);
	vuart_serial_out(port, VUART_IER, ier_save);
}

static int vuart_poll_get_char(struct uart_port *port)
{
	int status;

	status = vuart_serial_in(port, VUART_FSR);
	if (!FIELD_GET(VUART_FSR_RXDR, status))
		return NO_POLL_CHAR;

	return vuart_serial_in(port, VUART_RX);
}

#endif

static const char *vuart_type(struct uart_port *port)
{
	return "tsse_vuart";
}

static const struct uart_ops vuart_ops = {
	.tx_empty = vuart_tx_empty,
	.set_mctrl = vuart_set_mctrl,
	.get_mctrl = vuart_get_mctrl,
	.stop_tx = vuart_stop_tx,
	.start_tx = vuart_start_tx,
	.throttle = vuart_throttle,
	.unthrottle = vuart_unthrottle,
	.stop_rx = vuart_stop_rx,
	.enable_ms = vuart_enable_ms,
	.break_ctl = vuart_break_ctl,
	.startup = vuart_startup,
	.shutdown = vuart_shutdown,
	.set_termios = vuart_set_termios,
	.set_ldisc = vuart_set_ldisc,
	.pm = vuart_pm,
	.type = vuart_type,
	.release_port = vuart_release_port,
	.request_port = vuart_request_port,
	.config_port = vuart_config_port,
	.verify_port = vuart_verify_port,
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char = vuart_poll_get_char,
	.poll_put_char = vuart_poll_put_char,
#endif
};

static unsigned int vuart_rx_chars(struct uart_port *port, unsigned int lsr)
{
	int max_count = TSSE_VUART_MAX_RX_COUNT;
	unsigned char ch;
	struct tty_port *tport = &port->state->port;

	do {
		if (lsr & VUART_FSR_RXDR)
			ch = vuart_serial_in(port, VUART_RX);
		else
			ch = 0;
		port->icount.rx++;
		if (lsr & VUART_FSR_RXUE) {
			port->icount.overrun++;
			pr_err("income byte underflow, record and clear int.\n");
			vuart_serial_out(port, VUART_IIR, VUART_IIR_RXUE);
		}

		if (!uart_prepare_sysrq_char(port, ch)) {
			if (tty_insert_flip_char(tport, ch, TTY_NORMAL) == 0)
				++port->icount.buf_overrun;
		}

		if (--max_count == 0)
			break;
		lsr = vuart_serial_in(port, VUART_FSR);
	} while (lsr & VUART_FSR_RXDR);

	tty_flip_buffer_push(&port->state->port);
	return lsr;
}

static int vuart_deal_irq(struct uart_port *port, unsigned int iir)
{
	unsigned char status;
	unsigned int ier;
	struct tsse_vuart *vuart = (struct tsse_vuart *)port;

	if (iir & VUART_IIR_CPUCD)
		vuart->confige_done = 1;

	status = vuart_serial_in(port, VUART_FSR);
	if (port->read_status_mask & VUART_FSR_RXDR)
		vuart_rx_chars(port, status);
	else
		pr_err("read_status_mask not set VUART_FSR_RXDR, ignor rx.\n");

	ier = vuart_serial_in(port, VUART_IER);
	if (!(status & VUART_FSR_TXOE) && (status & VUART_FSR_TXFIFOE) &&
	    (ier & VUART_IER_HETXEI))
		vuart_tx_chars(port);

	return 1;
}

#ifdef DEBUG
static void vuart_debug_iir(unsigned int iir)
{
	VUART_PRINT("%s called iir %u.\n", __func__, iir);
	if (iir & VUART_IIR_TXEI)
		pr_err("TX FIFO empty interrupt.\n");

	if (iir & VUART_IIR_RXTOI)
		pr_err("Host RX FIFO character timeout interrupt.\n");

	if (iir & VUART_IIR_RXDAI)
		pr_err("Host RX FIFO data available interrupt.\n");

	if (iir & VUART_IIR_RXUE)
		pr_err("HOST RX FIFO Underflow error.\n");

	if (iir & VUART_IIR_TXOE)
		pr_err("HOST TX FIFO Overrun error.\n");

	if (iir & VUART_IIR_CPUCD)
		pr_err("CPU has finished configuration for virtual UART");

	if (iir & VUART_IIR_TXFI)
		pr_err("Host TX FIFO full interrupt.\n");
}
#endif

static int vuart_handle_irq(struct uart_port *port)
{
	unsigned int iir;
	unsigned long flags;
	int ret;

	iir = vuart_serial_in(port, VUART_IIR);
	vuart_serial_out(port, VUART_IIR, iir);
#ifdef DEBUG
	vuart_debug_iir(iir);
#endif
	spin_lock_irqsave(&port->lock, flags);
	ret = vuart_deal_irq(port, iir);

	uart_unlock_and_check_sysrq_irqrestore(port, flags);

	return ret;
}

static int vuart_get_line(void)
{
	int bit = 0;

	bit = find_first_zero_bit(&g_line[0], TSSE_VUART_MAX_DEV);
	if (bit >= TSSE_VUART_MAX_DEV)
		return -ENOSPC;
	set_bit(bit, &g_line[0]);
	return bit;
}

static void vuart_free_line(int line)
{
	clear_bit(line, &g_line[0]);
}

int vuart_init_port(struct pci_dev *pdev)
{
	struct tsse_dev *tdev = pci_to_tsse_dev(pdev);
	struct tsse_vuart *vuart = NULL;
	struct uart_port *p = NULL;
	int ret = 0;
	int line = vuart_get_line();

	if (line == -ENOSPC) {
		dev_err(&pdev->dev, "device too more, max is 64.\n");
		return -ENOMEM;
	}

	vuart = kzalloc_node(sizeof(struct tsse_vuart), GFP_KERNEL,
			     dev_to_node(&pdev->dev));
	if (!vuart) {
		ret = -ENOMEM;
		goto zalloc_fail;
	}
	vuart->shutdown = 1;
	p = &(vuart->port);
	p->mapbase = 0;
	p->mapsize = 0;
	p->membase = TSSE_DEV_BARS(tdev)[2].virt_addr + RLS_VUART_OFFSET;
	p->irq = pci_irq_vector(pdev, RLS_VUART_IRQ_NUM);
	p->handle_irq = vuart_handle_irq;
	spin_lock_init(&p->lock);
	p->line = line;
	p->type = PORT_16550A;
	p->uartclk = TSSE_VUART_BAUD * 16;
	p->iotype = UPIO_MEM;
	p->ops = &vuart_ops;
	p->fifosize = 128;
	vuart->tx_loadsz = 128;
	p->flags = UPF_BOOT_AUTOCONF | UPF_FIXED_TYPE | UPF_FIXED_PORT |
		   UPF_SHARE_IRQ;
	p->dev = &pdev->dev;
	p->private_data = tdev;

	tdev->port = (struct uart_port *)vuart;
	ret = uart_add_one_port(&g_vuart_reg, p);
	if (ret != 0) {
		dev_err(&pdev->dev, "add port fialed.[%d]\n", ret);
		goto add_port_fail;
	}
	return 0;
add_port_fail:
	kfree(vuart);
zalloc_fail:
	vuart_free_line(line);

	return ret;
}

void vuart_uninit_port(struct pci_dev *pdev)
{
	struct tsse_dev *tdev = pci_to_tsse_dev(pdev);
	struct tsse_vuart *vuart = (struct tsse_vuart *)(tdev->port);

	if (tdev->port) {
		if (!vuart->shutdown)
			free_irq(tdev->port->irq, tdev->port);
		vuart_free_line(tdev->port->line);
		uart_remove_one_port(&g_vuart_reg, tdev->port);
		kfree(vuart);
	}
}

int vuart_register(void)
{
	return uart_register_driver(&g_vuart_reg);
}

void vuart_unregister(void)
{
	uart_unregister_driver(&g_vuart_reg);
}
