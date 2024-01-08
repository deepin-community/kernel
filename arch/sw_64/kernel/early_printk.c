// SPDX-License-Identifier: GPL-2.0
#include <linux/console.h>
#include <linux/kernel.h>

#include <asm/io.h>

static unsigned long early_serial_base;  /* ttyS0 */

#define XMTRDY          0x20

#define DLAB            0x80

#define TXR             0       /*  Transmit register (WRITE) */
#define RXR             0       /*  Receive register  (READ)  */
#define IER             1       /*  Interrupt Enable          */
#define IIR             2       /*  Interrupt ID              */
#define FCR             2       /*  FIFO control              */
#define LCR             3       /*  Line control              */
#define MCR             4       /*  Modem control             */
#define LSR             5       /*  Line Status               */
#define MSR             6       /*  Modem Status              */
#define DLL             0       /*  Divisor Latch Low         */
#define DLH             1       /*  Divisor latch High        */

static void mem32_serial_out(unsigned long addr, int offset, int value)
{
	void __iomem *vaddr = (void __iomem *)addr;

	offset = offset << 9;

	writel(value, vaddr + offset);
}

static unsigned int mem32_serial_in(unsigned long addr, int offset)
{
	void __iomem *vaddr = (void __iomem *)addr;

	offset = offset << 9;

	return readl(vaddr + offset);
}

static unsigned int (*serial_in)(unsigned long addr, int offset) = mem32_serial_in;
static void (*serial_out)(unsigned long addr, int offset, int value) = mem32_serial_out;

static int early_serial_putc(unsigned char ch)
{
	unsigned int timeout = 0xffff;

	while ((serial_in(early_serial_base, LSR) & XMTRDY) == 0 && --timeout)
		cpu_relax();
	serial_out(early_serial_base, TXR, ch);

	return timeout ? 0 : -1;
}

static void early_serial_write(struct console *con, const char *s, unsigned int n)
{
	while (*s && n-- > 0) {
		if (*s == '\n')
			early_serial_putc('\r');
		early_serial_putc(*s);
		s++;
	}
}

static unsigned int uart_get_refclk(void)
{
	return 24000000UL;
}

static unsigned int uart_calculate_baudrate_divisor(unsigned long baudrate)
{
	unsigned int refclk = uart_get_refclk();

	return (1 + (2 * refclk) / (baudrate * 16)) / 2;
}

static __init void early_serial_hw_init(unsigned long baud)
{
	unsigned char c;
	unsigned long divisor = uart_calculate_baudrate_divisor(baud);

	serial_out(early_serial_base, LCR, 0x3);        /* 8n1 */
	serial_out(early_serial_base, IER, 0);  /* no interrupt */
	serial_out(early_serial_base, FCR, 0);  /* no fifo */
	serial_out(early_serial_base, MCR, 0x3);        /* DTR + RTS */

	c = serial_in(early_serial_base, LCR);
	serial_out(early_serial_base, LCR, c | DLAB);
	serial_out(early_serial_base, DLL, divisor & 0xff);
	serial_out(early_serial_base, DLH, (divisor >> 8) & 0xff);
	serial_out(early_serial_base, LCR, c & ~DLAB);
}

#define DEFAULT_BAUD 115200

static __init void early_serial_init(char *s)
{
	unsigned long baud = DEFAULT_BAUD;
	int err;

	if (*s == ',')
		++s;

	if (*s) {
		unsigned int port;
		static const long bases[] __initconst = { 0xfff0803300000000ULL,
			0xfff0903300000000ULL };

		if (!strncmp(s, "ttyS", 4))
			s += 4;
		err = kstrtouint(s, 10, &port);
		if (err || port > 1)
			port = 0;
		early_serial_base = bases[port];
		s += strcspn(s, ",");
		if (*s == ',')
			s++;
	}

	if (*s) {
		err = kstrtoul(s, 0, &baud);
		if (err || baud == 0)
			baud = DEFAULT_BAUD;
	}

	/* These will always be IO based ports */
	serial_in = mem32_serial_in;
	serial_out = mem32_serial_out;

	/* Set up the HW */
	early_serial_hw_init(baud);
}

static struct console early_serial_console = {
	.name =         "early",
	.write =        early_serial_write,
	.flags =        CON_PRINTBUFFER,
	.index =        -1,
};

static void early_console_register(struct console *con, int keep_early)
{
	if (con->index != -1) {
		pr_crit("ERROR: earlyprintk= %s already used\n",
				con->name);
		return;
	}
	early_console = con;

	if (keep_early)
		early_console->flags &= ~CON_BOOT;
	else
		early_console->flags |= CON_BOOT;

	register_console(early_console);
}

static int __init setup_early_printk(char *buf)
{
	int keep;

	if (!buf)
		return 0;

	if (early_console)
		return 0;

	keep = (strstr(buf, "keep") != NULL);

	if (!strncmp(buf, "serial", 6)) {
		buf += 6;
		early_serial_init(buf);
		early_console_register(&early_serial_console, keep);
		if (!strncmp(buf, ",ttyS", 5))
			buf += 5;
	}

	return 0;
}

early_param("earlyprintk", setup_early_printk);
