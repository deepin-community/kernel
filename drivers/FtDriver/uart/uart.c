/*
 * Uart for phytium platform
 *
 */
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/cputypes.h>

// PL011 Registers
#define UARTDR                    0x000
#define UARTRSR                   0x004
#define UARTECR                   0x004
#define UARTFR                    0x018
#define UARTILPR                  0x020
#define UARTIBRD                  0x024
#define UARTFBRD                  0x028
#define UARTLCR_H                 0x02C
#define UARTCR                    0x030
#define UARTIFLS                  0x034
#define UARTIMSC                  0x038
#define UARTRIS                   0x03C
#define UARTMIS                   0x040
#define UARTICR                   0x044
#define UARTDMACR                 0x048

#define UARTPID0                  0xFE0
#define UARTPID1                  0xFE4
#define UARTPID2                  0xFE8
#define UARTPID3                  0xFEC

// Data status bits
#define UART_DATA_ERROR_MASK      0x0F00

// Status reg bits
#define UART_STATUS_ERROR_MASK    0x0F

// Flag reg bits
#define PL011_UARTFR_RI           (1 << 8)  // Ring indicator
#define PL011_UARTFR_TXFE         (1 << 7)  // Transmit FIFO empty
#define PL011_UARTFR_RXFF         (1 << 6)  // Receive  FIFO full
#define PL011_UARTFR_TXFF         (1 << 5)  // Transmit FIFO full
#define PL011_UARTFR_RXFE         (1 << 4)  // Receive  FIFO empty
#define PL011_UARTFR_BUSY         (1 << 3)  // UART busy
#define PL011_UARTFR_DCD          (1 << 2)  // Data carrier detect
#define PL011_UARTFR_DSR          (1 << 1)  // Data set ready
#define PL011_UARTFR_CTS          (1 << 0)  // Clear to send

// Flag reg bits - alternative names
#define UART_TX_EMPTY_FLAG_MASK   PL011_UARTFR_TXFE
#define UART_RX_FULL_FLAG_MASK    PL011_UARTFR_RXFF
#define UART_TX_FULL_FLAG_MASK    PL011_UARTFR_TXFF
#define UART_RX_EMPTY_FLAG_MASK   PL011_UARTFR_RXFE
#define UART_BUSY_FLAG_MASK       PL011_UARTFR_BUSY

// Control reg bits
#define PL011_UARTCR_CTSEN        (1 << 15) // CTS hardware flow control enable
#define PL011_UARTCR_RTSEN        (1 << 14) // RTS hardware flow control enable
#define PL011_UARTCR_RTS          (1 << 11) // Request to send
#define PL011_UARTCR_DTR          (1 << 10) // Data transmit ready.
#define PL011_UARTCR_RXE          (1 << 9)  // Receive enable
#define PL011_UARTCR_TXE          (1 << 8)  // Transmit enable
#define PL011_UARTCR_LBE          (1 << 7)  // Loopback enable
#define PL011_UARTCR_UARTEN       (1 << 0)  // UART Enable

// Line Control Register Bits
#define PL011_UARTLCR_H_SPS       (1 << 7)  // Stick parity select
#define PL011_UARTLCR_H_WLEN_8    (3 << 5)
#define PL011_UARTLCR_H_WLEN_7    (2 << 5)
#define PL011_UARTLCR_H_WLEN_6    (1 << 5)
#define PL011_UARTLCR_H_WLEN_5    (0 << 5)
#define PL011_UARTLCR_H_FEN       (1 << 4)  // FIFOs Enable
#define PL011_UARTLCR_H_STP2      (1 << 3)  // Two stop bits select
#define PL011_UARTLCR_H_EPS       (1 << 2)  // Even parity select
#define PL011_UARTLCR_H_PEN       (1 << 1)  // Parity Enable
#define PL011_UARTLCR_H_BRK       (1 << 0)  // Send break

#define PL011_UARTPID2_VER(X)     (((X) >> 4) & 0xF)
#define PL011_VER_R1P4            0x2

#define PHYTIUM_UART_BASE 0x28000000
#define PHYTIUM_UART_PORT_REG_SIZE 0x1000
#define UART_PORT_MAX 0x4

void __iomem *ioaddr = NULL;

struct uart_register_info
{
    unsigned int cr;
    unsigned int ibrd;
    unsigned int fbrd;
    unsigned int lcr_h;
    unsigned int ifsl;
    unsigned int imsc;
    unsigned int dma;
} phytium_uart_reg_val[UART_PORT_MAX];

static int init_flag = 0;

int phytium_uart_info_save (unsigned int port_id)
{
    void __iomem * uart_reg_base = NULL;

    if ((ioaddr == NULL) || (port_id >= UART_PORT_MAX)) {
        printk(KERN_ERR "%s port_id error, port_id = 0x%x \n\r", __FUNCTION__, port_id);
        return -1;
    }

    uart_reg_base = ioaddr + PHYTIUM_UART_PORT_REG_SIZE * port_id;

    phytium_uart_reg_val[port_id].cr = readl(uart_reg_base + UARTCR);
    phytium_uart_reg_val[port_id].ibrd = readl(uart_reg_base + UARTIBRD);
    phytium_uart_reg_val[port_id].fbrd = readl(uart_reg_base + UARTFBRD);
    phytium_uart_reg_val[port_id].lcr_h = readl(uart_reg_base + UARTLCR_H);
    phytium_uart_reg_val[port_id].ifsl = readl(uart_reg_base + UARTIFLS);
    phytium_uart_reg_val[port_id].imsc = readl(uart_reg_base + UARTIMSC);
    phytium_uart_reg_val[port_id].dma = readl(uart_reg_base + UARTDMACR);

    return 0;
}

int phytium_uart_info_resume (unsigned int port_id)
{
    void __iomem * uart_reg_base = NULL;

    if ((ioaddr == NULL) || (port_id >= UART_PORT_MAX)) {
        printk(KERN_ERR "%s port_id error, port_id = 0x%x \n\r", __FUNCTION__, port_id);
        return -1;
    }

    uart_reg_base = ioaddr + PHYTIUM_UART_PORT_REG_SIZE * port_id;

    writel (0, uart_reg_base + UARTCR);

    // Set Baud Rate Registers
    writel (phytium_uart_reg_val[port_id].ibrd, uart_reg_base + UARTIBRD);
    writel (phytium_uart_reg_val[port_id].fbrd, uart_reg_base + UARTFBRD);

    // No parity, 1 stop, no fifo, 8 data bits
    writel (phytium_uart_reg_val[port_id].lcr_h, uart_reg_base + UARTLCR_H);

    // enable irq, fifo size, dma
    writel (phytium_uart_reg_val[port_id].ifsl, uart_reg_base + UARTIFLS);
    writel (phytium_uart_reg_val[port_id].imsc, uart_reg_base + UARTIMSC);
    writel (phytium_uart_reg_val[port_id].dma, uart_reg_base + UARTDMACR);



    // Clear any pending errors
    writel (0, ioaddr + UARTECR);

    // Enable Tx, Rx, and UART overall
    writel (phytium_uart_reg_val[port_id].cr, uart_reg_base + UARTCR);

    return 0;
}

/**
  Initialise the serial port to the default settings.
**/
int phytium_uart_initialize_port (unsigned int port_id)
{
/* No parity, 1 stop, no fifo, 8 data bits */
    unsigned int line_control = 0x70;
/*
    BaudRate is 115200, UartClkInHz is 48MHz
    divisor = (UartClkInHz * 4) / *BaudRate;
    integer = divisor >> 6;
    fractional = divisor & (1<<6);
*/
    unsigned int integer = 0x1a;
    unsigned int fractional = 0x2;

    /* waiting loop counter */
    unsigned int timeout_counter = 0x200;

    void __iomem * uart_reg_base = NULL;

    if ((ioaddr == NULL) || (port_id >= UART_PORT_MAX)) {
        printk(KERN_ERR "%s port_id error, port_id = 0x%x \n\r", __FUNCTION__, port_id);
        return -1;
    }

    uart_reg_base = ioaddr + PHYTIUM_UART_PORT_REG_SIZE * port_id;


    /* If PL011 is already initialized, check the current settings */
    if ((readl (uart_reg_base + UARTCR) & PL011_UARTCR_UARTEN) != 0) {
    /* Nothing to do - already initialized with correct attributes */
        return 0;
    }

//    Wait for the end of transmission
//    while (((readl (uart_reg_base + UARTFR) & PL011_UARTFR_TXFE) == 0) && timeout_counter) {
//        timeout_counter--;
//    }

    // Disable UART: "The UARTLCR_H, UARTIBRD, and UARTFBRD registers must not be changed
    // when the UART is enabled"
    writel (0, uart_reg_base + UARTCR);

    // Set Baud Rate Registers
    writel (integer,uart_reg_base + UARTIBRD);
    writel (fractional, uart_reg_base + UARTFBRD);

    // No parity, 1 stop, no fifo, 8 data bits
    writel (line_control, uart_reg_base + UARTLCR_H);

    // Clear any pending errors
    writel (0, ioaddr + UARTECR);

    // Enable Tx, Rx, and UART overall
    writel (PL011_UARTCR_RXE | PL011_UARTCR_TXE | PL011_UARTCR_UARTEN, uart_reg_base + UARTCR);

    return 0;
}

void uart_reg_show(int port_id)
{
    unsigned int val = 0;
    void __iomem * uart_reg_base = NULL;

    if (port_id >= UART_PORT_MAX) {
        printk(KERN_ERR "uart_reg_show port_id error,   port_id = 0x%x \n\r", port_id);
        return;
    }

    uart_reg_base = ioaddr + PHYTIUM_UART_PORT_REG_SIZE * port_id;

    val = readl(uart_reg_base + UARTCR);
    printk(KERN_ERR "uart_%d reg = 0x%x  val = 0x%x\n\r",port_id, UARTCR, val);
    val = readl(uart_reg_base + UARTIBRD);
    printk(KERN_ERR "uart_%d reg = 0x%x  val = 0x%x\n\r",port_id, UARTIBRD, val);
    val = readl(uart_reg_base + UARTFBRD);
    printk(KERN_ERR "uart_%d reg = 0x%x  val = 0x%x\n\r",port_id, UARTFBRD, val);
    val = readl(uart_reg_base + UARTLCR_H);
    printk(KERN_ERR "uart_%d reg = 0x%x  val = 0x%x\n\r",port_id, UARTLCR_H, val);
    val = readl(uart_reg_base + UARTECR);
    printk(KERN_ERR "uart_%d reg = 0x%x  val = 0x%x\n\r",port_id, UARTECR, val);
    val = readl(uart_reg_base + UARTIMSC);
    printk(KERN_ERR "uart_%d reg = 0x%x  val = 0x%x\n\r",port_id, UARTIMSC, val);

    return;
}

int phytium_uart_port_init(void)
{
    if (!cpu_is_phytium()) {
        return 0;
    }

    if(init_flag) {
        return 0;
    }

    ioaddr = ioremap(PHYTIUM_UART_BASE, PHYTIUM_UART_PORT_REG_SIZE * UART_PORT_MAX);
    if(ioaddr == NULL) {
        printk(KERN_ERR "phytium_uart_port_init ioremap error \n\r");
        return -1;
    }

    phytium_uart_initialize_port(0);
    phytium_uart_initialize_port(1);
    phytium_uart_initialize_port(2);
    phytium_uart_initialize_port(3);
#if 0 /* show uart register */
    uart_reg_show(0);
    printk(KERN_ERR "\n\r\n\r");
    uart_reg_show(1);
    printk(KERN_ERR "\n\r\n\r");
    uart_reg_show(2);
    printk(KERN_ERR "\n\r\n\r");
    uart_reg_show(3);
#endif

    init_flag = 1;
	return 0;
}
EXPORT_SYMBOL(phytium_uart_port_init);

#ifdef CONFIG_PM
int phytium_uart_suspend(void)
{
    phytium_uart_info_save (0);
    phytium_uart_info_save (1);
    phytium_uart_info_save (2);
    phytium_uart_info_save (3);
    return 0;
}
EXPORT_SYMBOL(phytium_uart_suspend);

int phytium_uart_resume(void)
{
    phytium_uart_info_resume(0);
    phytium_uart_info_resume(1);
    phytium_uart_info_resume(2);
    phytium_uart_info_resume(3);

    return 0;
}
EXPORT_SYMBOL(phytium_uart_resume);
#endif
