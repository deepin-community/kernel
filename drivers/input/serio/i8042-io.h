/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _I8042_IO_H
#define _I8042_IO_H


/*
 * Names.
 */

#define I8042_KBD_PHYS_DESC "isa0060/serio0"
#define I8042_AUX_PHYS_DESC "isa0060/serio1"
#define I8042_MUX_PHYS_DESC "isa0060/serio%d"

/*
 * IRQs.
 */
#include <linux/cputypes.h>
#include <linux/classtypes.h>
#ifdef __alpha__
# define I8042_KBD_IRQ	1
# define I8042_AUX_IRQ	(RTC_PORT(0) == 0x170 ? 9 : 12)	/* Jensen is special */
#elif defined(__arm__)
/* defined in include/asm-arm/arch-xxx/irqs.h */
#include <asm/irq.h>
#elif defined(CONFIG_PPC)
extern int of_i8042_kbd_irq;
extern int of_i8042_aux_irq;
# define I8042_KBD_IRQ  of_i8042_kbd_irq
# define I8042_AUX_IRQ  of_i8042_aux_irq
#else
#ifdef CONFIG_ARCH_PHYTIUM_LPC
# define I8042_KBD_IRQ	0x41
# define I8042_AUX_IRQ	0x4c
#else
# define I8042_KBD_IRQ	1
# define I8042_AUX_IRQ	12
#endif
#endif


/*
 * Register numbers.
 */

#define I8042_COMMAND_REG	0x64
#define I8042_STATUS_REG	0x64
#define I8042_DATA_REG		0x60

#ifdef CONFIG_ARCH_PHYTIUM_LPC
static int i8042_command_reg = I8042_COMMAND_REG;
static int i8042_status_reg = I8042_STATUS_REG;
static int i8042_data_reg = I8042_DATA_REG;

static int i8042_kbd_irq = 1;
#define I8042_KBD_IRQ_FT	65
#ifdef I8042_KBD_IRQ
#undef I8042_KBD_IRQ
#define I8042_KBD_IRQ i8042_kbd_irq
#endif

#define EC_EVENT_BIT		(1 << 11)
#define I8042_KEY_BIT		(1 << 1)
#define I8042_TOUCH_BIT		(1 << 12)

#define LPC_STATUS_REG		0xF4
#define LPC_INTERRUPT_REG	0xF0
#endif

#ifdef CONFIG_ARCH_PHYTIUM_LPC
static void __iomem *I8042_iobase;
static void __iomem *I8042_reg_base;

static inline u32 i8042_read_lpc_status(void)
{
	return readl(I8042_reg_base + LPC_STATUS_REG);
}

static unsigned long irq_count_ec = 0;
static unsigned long irq_count_kbd = 0;
static unsigned long irq_count_aux = 0;
static unsigned long irq_count_other = 0;
static unsigned long irq_count_total = 0;
static unsigned long print_count = 1000;

static inline int i8042_write_lpc_interrupt_clear(void)
{
	u32 data;
    int int_mask = 	(EC_EVENT_BIT | I8042_KEY_BIT | I8042_TOUCH_BIT);

	data = readl(I8042_reg_base + LPC_STATUS_REG);

//    irq_count_total++;
#if 0
    if ( data & ~int_mask){
        printk(KERN_ERR "i8042_write_lpc_interrupt_clear: interrrupts status abnormal, int_status = 0x%x", data);
        irq_count_other++;
    }
#endif
    /* clear abnormal interrupt */
    data &= int_mask;

	if( data & I8042_KEY_BIT ) {
//    irq_count_kbd++;
		data = data & (~I8042_KEY_BIT);
	}
	else if( data & I8042_TOUCH_BIT ) {
//    irq_count_aux++;
		data = data & (~I8042_TOUCH_BIT);
	}
	else if( data & EC_EVENT_BIT ) {
//    irq_count_ec++;
		if( !chassis_types_is_laptop() ) {
			data = data & (~EC_EVENT_BIT);
			writel(data, I8042_reg_base + LPC_INTERRUPT_REG);
		}
		return 1;
	}

	writel(data, I8042_reg_base + LPC_INTERRUPT_REG);
	data = readl(I8042_reg_base + LPC_STATUS_REG);

	return 0;
}

static inline int i8042_ec_interrupt_clear(void)
{
	u32 data;

	data = readl(I8042_reg_base + LPC_STATUS_REG);

	if( data & EC_EVENT_BIT ) {
        irq_count_total++;
        irq_count_ec++;
        data = data & (~EC_EVENT_BIT);
        writel(data, I8042_reg_base + LPC_INTERRUPT_REG);
        return 1;
	}
	data = readl(I8042_reg_base + LPC_STATUS_REG);
	return 0;
}

static inline void lpc_interrupt_info_print(void)
{
    if (irq_count_total >= print_count) {
        print_count = print_count + 1000;
        printk(KERN_ERR "\n\r--- LPC interrpt Info:  irq_count_total = %ld \n\r", irq_count_total);
        printk(KERN_ERR "\n\r irq_count_ec = %ld \n\r"     \
                "irq_count_kbd = %ld \n\r"   \
                "irq_count_aux  = %ld \n\r"  \
                "irq_count_other  = %ld \n\r", irq_count_ec,irq_count_kbd,irq_count_aux,irq_count_other);

    }
}
static inline void i8042_write_lpc_interrupt_clear_all(void)
{
    return	writel(0, I8042_reg_base + LPC_INTERRUPT_REG);
}
#endif

static inline int i8042_read_data(void)
{
#ifdef CONFIG_ARCH_PHYTIUM_LPC
	return readb(I8042_iobase + i8042_data_reg);
#else
	return inb(I8042_DATA_REG);
#endif
}

static inline int i8042_read_status(void)
{
#ifdef CONFIG_ARCH_PHYTIUM_LPC
	return readb(I8042_iobase + i8042_status_reg);
#else
	return inb(I8042_STATUS_REG);
#endif
}

static inline void i8042_write_data(int val)
{
#ifdef CONFIG_ARCH_PHYTIUM_LPC
	writeb(val, I8042_iobase + i8042_data_reg);
#else
	outb(val, I8042_DATA_REG);
#endif
}

static inline void i8042_write_command(int val)
{
#ifdef CONFIG_ARCH_PHYTIUM_LPC
		writeb(val, I8042_iobase + i8042_command_reg);
#else
	outb(val, I8042_COMMAND_REG);
#endif
}

#ifdef CONFIG_ARCH_PHYTIUM_LPC
/*
 Super I/O Control Reg  read/write interface

Logical Device table:
*/
#define SIO_ADDR_PORT 0x4e
#define SIO_DATA_PORT 0x4f
#define SIO_LDN_REG 0x7

#define SIO_LD_IRQ_NUM_REG 0x70
#define SIO_LD_IRQ_TYPE_REG 0x71
#define SIO_LD_ACTIVATE_REG 0x30
#define SIO_ENABLE  0x1
#define SIO_DISABLE 0

#define MOUSE_LDN   0x5
#define KEYBOARD_LDN   0x6
#define PM1_LDN   0x11

void delay_test(void)
{
    int i,j;
    for(i=0; i<100;i++) {
        j = i;
    }
}

static char sio_ldc_reg_read(char ldn, char reg)
{
    char val;

    writeb(SIO_LDN_REG, I8042_iobase + SIO_ADDR_PORT);
    delay_test();
    writeb(ldn, I8042_iobase + SIO_DATA_PORT);
    delay_test();
    writeb(reg, I8042_iobase + SIO_ADDR_PORT);
    delay_test();
    val = readb(I8042_iobase + SIO_DATA_PORT);
    delay_test();

    return val;
}

static void sio_ldc_reg_write(char ldn, char reg, char val)
{
    writeb(SIO_LDN_REG, I8042_iobase + SIO_ADDR_PORT);
    delay_test();
    writeb(ldn, I8042_iobase + SIO_DATA_PORT);
    delay_test();
    writeb(reg, I8042_iobase + SIO_ADDR_PORT);
    delay_test();
    writeb(val, I8042_iobase + SIO_DATA_PORT);
    delay_test();
}
static void sio_ld_ebable(void)
{
    writeb(SIO_LD_ACTIVATE_REG, I8042_iobase + SIO_ADDR_PORT);
    delay_test();
    writeb(SIO_ENABLE, I8042_iobase + SIO_DATA_PORT);
    delay_test();
}
static void i8042_reg_read_test(void)
{
    char reg_70 =0;
    char reg_71 =0;

    reg_70 = sio_ldc_reg_read(MOUSE_LDN, SIO_LD_IRQ_NUM_REG);
    reg_71 = sio_ldc_reg_read(MOUSE_LDN, SIO_LD_IRQ_TYPE_REG);

    printk(KERN_INFO "i8042_reg_read_test --> mouse reg_70:0x%x reg_71:0x%x\n",reg_70, reg_71);


    reg_70 = sio_ldc_reg_read(KEYBOARD_LDN, SIO_LD_IRQ_NUM_REG);
    reg_71 = sio_ldc_reg_read(KEYBOARD_LDN, SIO_LD_IRQ_TYPE_REG);

    printk(KERN_INFO "i8042_reg_read_test --> keyboard reg_70:0x%x reg_71:0x%x\n",reg_70, reg_71);

    reg_70 = sio_ldc_reg_read(PM1_LDN, SIO_LD_IRQ_NUM_REG);
    reg_71 = sio_ldc_reg_read(PM1_LDN, SIO_LD_IRQ_TYPE_REG);

    printk(KERN_INFO "i8042_reg_read_test --> pm reg_70:0x%x reg_71:0x%x\n",reg_70, reg_71);

    return;
}

static inline void i8042_czc_config(void)
{

    printk(KERN_INFO "i8042_czc_config --> i8042_czc_config start ... \n\r");
	if (cpu_is_phytium()) {
        //mouse
        writeb(0x07, I8042_iobase + 0x4e);
    delay_test();
        writeb(0x05, I8042_iobase + 0x4f);
    delay_test();
        writeb(0x71, I8042_iobase + 0x4e);
    delay_test();
        writeb(0x0,  I8042_iobase + 0x4f);
    delay_test();
        writeb(0x30, I8042_iobase + 0x4e);
    delay_test();
        writeb(0x1,  I8042_iobase + 0x4f);
    delay_test();

        //kbd
        writeb(0x07, I8042_iobase + 0x4e);
    delay_test();
        writeb(0x06, I8042_iobase + 0x4f);
    delay_test();
        writeb(0x71, I8042_iobase + 0x4e);
    delay_test();
        writeb(0x0,  I8042_iobase + 0x4f);
    delay_test();
        writeb(0x30, I8042_iobase + 0x4e);
    delay_test();
        writeb(0x1,  I8042_iobase + 0x4f);
    delay_test();

      //pm1
        writeb(0x07, I8042_iobase + 0x4e);
    delay_test();
        writeb(0x11, I8042_iobase + 0x4f);
    delay_test();
        writeb(0x71, I8042_iobase + 0x4e);
    delay_test();
        writeb(0x0,  I8042_iobase + 0x4f);
    delay_test();
        writeb(0x30, I8042_iobase + 0x4e);
    delay_test();
        writeb(0x1,  I8042_iobase + 0x4f);
    delay_test();

	}
    printk(KERN_INFO "i8042_czc_config --> i8042_czc_config end ... \n\r");
    return;
}
#endif


#ifdef CONFIG_ARCH_PHYTIUM_LPC
static inline int i8042_platform_init(struct device *dev)
#else
static inline int i8042_platform_init(void)
#endif
{
/*
 * On some platforms touching the i8042 data register region can do really
 * bad things. Because of this the region is always reserved on such boxes.
 */
#if defined(CONFIG_PPC)
	if (check_legacy_ioport(I8042_DATA_REG))
		return -ENODEV;
#endif
#if !defined(__sh__) && !defined(__alpha__)
	if (!request_region(I8042_DATA_REG, 16, "i8042"))
		return -EBUSY;
#endif

	i8042_reset = I8042_RESET_ALWAYS;

#ifdef CONFIG_ARCH_PHYTIUM_LPC
    int ret;
	if (cpu_is_phytium()) {
		I8042_KBD_IRQ = I8042_KBD_IRQ_FT;

        ret = device_property_read_u32(dev, "i8042_command_reg", &i8042_command_reg);
        if( ret )
            return -ENODEV;

        i8042_status_reg = i8042_command_reg;
        ret = device_property_read_u32(dev, "i8042_data_reg", &i8042_data_reg);
        if( ret )
            return -ENODEV;

        //printk("czh debug --> %s %d command_reg:0x%x data_reg:0x%x\n",__func__,__LINE__,i8042_command_reg, i8042_data_reg);
	} else {
		return -ENODEV;
	}
#endif
	return 0;
}

static inline void i8042_platform_exit(void)
{
#if !defined(__sh__) && !defined(__alpha__)
#ifdef CONFIG_ARCH_PHYTIUM_LPC
	release_region(i8042_data_reg, 16);
#else
	release_region(I8042_DATA_REG, 16);
#endif
#endif
}

#endif /* _I8042_IO_H */
