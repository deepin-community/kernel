/*
 * EC (Embedded Controller) for phytium platform
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <asm/io.h>
#include "ec_it8528.h"

/* This spinlock is dedicated for 62&66 ports and super io port access. */
DEFINE_SPINLOCK(index_access_lock);

static void __iomem *lpc_iobase;

static int send_ec_command(unsigned char command)
{
	int timeout, ret = 0;

	timeout = EC_SEND_TIMEOUT;
	while ((readb(lpc_iobase + EC_STS_PORT) & EC_IBF) && --timeout)
		udelay(50);
	if (!timeout) {
		printk(KERN_ERR "Timeout while sending command 0x%02x to EC!\n", command);
		ret = -1;
		goto out;
	}

	writeb(command, lpc_iobase + EC_CMD_PORT);

out:
	return ret;
}

static int send_ec_data(unsigned char data)
{
	int timeout, ret = 0;

	timeout = EC_SEND_TIMEOUT;
	while ((readb(lpc_iobase + EC_STS_PORT) & EC_IBF) && --timeout)
		udelay(50);
	if (!timeout) {
		printk(KERN_ERR "Timeout while sending data 0x%02x to EC!\n", data);
		ret = -1;
		goto out;
	}

	writeb(data, lpc_iobase + EC_DAT_PORT);

out:
	return ret;
}

static unsigned char recv_ec_data(void)
{
	int timeout;
	unsigned char data;

	timeout = EC_RECV_TIMEOUT;
	while (!(readb(lpc_iobase + EC_STS_PORT) & EC_OBF) && --timeout)
		udelay(50);
	if (!timeout) {
		printk(KERN_ERR "Timeout while receiving data from EC! status 0x%x.\n", readb(lpc_iobase + EC_STS_PORT));
		data = 0;
		goto skip_data;
	}

	data = readb(lpc_iobase + EC_DAT_PORT);

skip_data:
	return data;
}

unsigned char it8528_read(unsigned char index)
{
	unsigned char value = 0;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&index_access_lock, flags);
	ret = send_ec_command(CMD_READ_EC);
	if (ret < 0) {
		printk(KERN_ERR "Send command fail!\n");
		value = 0;
		goto out;
	}
	ret = send_ec_data(index);
	if (ret < 0) {
		printk(KERN_ERR "Send data fail!\n");
		value = 0;
		goto out;
	}
	value = recv_ec_data();
out:
	spin_unlock_irqrestore(&index_access_lock, flags);

	return value;
}
EXPORT_SYMBOL(it8528_read);

unsigned char it8528_read_all(unsigned char command, unsigned char index)
{
	unsigned char value = 0;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&index_access_lock, flags);
	ret = send_ec_command(command);
	if (ret < 0) {
		printk(KERN_ERR "Send command fail!\n");
		goto out;
	}
	ret = send_ec_data(index);
	if (ret < 0) {
		printk(KERN_ERR "Send data fail!\n");
		goto out;
	}
	value = recv_ec_data();
out:
	spin_unlock_irqrestore(&index_access_lock, flags);

	return value;
}
EXPORT_SYMBOL(it8528_read_all);

unsigned char it8528_read_noindex(unsigned char command)
{
	unsigned char value = 0;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&index_access_lock, flags);
	ret = send_ec_command(command);
	if (ret < 0) {
		printk(KERN_ERR "Send command fail!\n");
		goto out;
	}
	value = recv_ec_data();
out:
	spin_unlock_irqrestore(&index_access_lock, flags);

	return value;
}
EXPORT_SYMBOL(it8528_read_noindex);

int it8528_write(unsigned char index, unsigned char data)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&index_access_lock, flags);
	ret = send_ec_command(CMD_WRITE_EC);
	if (ret < 0) {
		printk(KERN_ERR "Send command 0x81 fail!\n");
		goto out;
	}
	ret = send_ec_data(index);
	if (ret < 0) {
		printk(KERN_ERR "Send index 0x%x fail!\n", index);
		goto out;
	}

	ret = send_ec_data(data);
	if (ret < 0) {
		printk(KERN_ERR "Send data 0x%x fail!\n", data);
	}
out:
	spin_unlock_irqrestore(&index_access_lock, flags);

	return ret;
}
EXPORT_SYMBOL(it8528_write);

int it8528_write_all(unsigned char command, unsigned char index, unsigned char data)
{
	unsigned long flags;

	spin_lock_irqsave(&index_access_lock, flags);
	send_ec_command(command);
	send_ec_data(index);
	send_ec_data(data);
	spin_unlock_irqrestore(&index_access_lock, flags);

	return 0;
}
EXPORT_SYMBOL(it8528_write_all);

int it8528_write_noindex(unsigned char command, unsigned char data)
{
	unsigned long flags;

	spin_lock_irqsave(&index_access_lock, flags);
	send_ec_command(command);
	send_ec_data(data);
	spin_unlock_irqrestore(&index_access_lock, flags);

	return 0;
}
EXPORT_SYMBOL(it8528_write_noindex);

bool it8528_get_ec_ibf_flags(void)
{
    return !!(readb(lpc_iobase + EC_STS_PORT) & EC_IBF);
}
EXPORT_SYMBOL(it8528_get_ec_ibf_flags);

bool it8528_get_ec_obf_flags(void)
{
    return !!(readb(lpc_iobase + EC_STS_PORT) & EC_OBF);
}
EXPORT_SYMBOL(it8528_get_ec_obf_flags);

bool it8528_get_ec_evt_flags(void)
{
	return !!(readb(lpc_iobase + EC_STS_PORT) & EC_SCI_EVT);
}
EXPORT_SYMBOL(it8528_get_ec_evt_flags);

int it8528_query_get_event_num(void)
{
	unsigned char value = 0;
	unsigned long flags;
	int ret = 0;
	unsigned int timeout;

	spin_lock_irqsave(&index_access_lock, flags);
	ret = send_ec_command(CMD_GET_EVENT_NUM);
	if (ret < 0) {
		printk(KERN_ERR "Send command fail!\n");
		goto out;
	}

	/* check if the command is received by ec */
	timeout = EC_CMD_TIMEOUT;
	while ((readb(lpc_iobase + EC_STS_PORT) & EC_IBF) && timeout--)
		udelay(50);
	if (timeout <= 0) {
		printk(KERN_ERR "EC QUERY SEQ: deadable error : timeout...\n");
		ret = -EINVAL;
		goto out;
	}

	value = recv_ec_data();
out:
	spin_unlock_irqrestore(&index_access_lock, flags);
	return value;
}
EXPORT_SYMBOL(it8528_query_get_event_num);

int it8528_query_clean_event(void)
{
	int ret;
	while(readb(lpc_iobase + EC_STS_PORT) & EC_SCI_EVT){
		ret = it8528_query_get_event_num();
		if (ret < 0){
			printk(KERN_ERR "Clean event fail!\n");
			return ret;
		}
	}

	return 0;
}
EXPORT_SYMBOL(it8528_query_clean_event);

void it8528_ec_event_int_enable(void)
{
	unsigned char value = 0;
	unsigned long flags;
	int ret = 0;
	unsigned int timeout;

	spin_lock_irqsave(&index_access_lock, flags);

	ret = send_ec_command(CMD_ENABLE_EVENT_EC);
	if (ret < 0) {
		printk(KERN_ERR "Send command fail!\n");
		goto out;
	}

	/* check if the command is received by ec */
	timeout = EC_CMD_TIMEOUT;
	while ((readb(lpc_iobase + EC_STS_PORT) & EC_IBF) && timeout--)
		udelay(50);
	if (timeout <= 0) {
		printk(KERN_ERR "EC ENABLE EVENT INTERRUPT: deadable error : timeout...\n");
		ret = -EINVAL;
		goto out;
	}
out:
	spin_unlock_irqrestore(&index_access_lock, flags);

	return;
}
EXPORT_SYMBOL(it8528_ec_event_int_enable);


extern void get_lpc_iobase(void __iomem **iobase);
extern void get_lpc_reg_base(void __iomem **iobase);
extern int get_lpc_irq(void);
static void __iomem *lpc_reg_base;
int it8528_init(void)
{
	get_lpc_iobase(&lpc_iobase);
	if (!lpc_iobase) {
		printk(KERN_INFO "it8528_init  lpc device not found! \n");
        return -1;
	}
	get_lpc_reg_base(&lpc_reg_base);
	return 0;
}
EXPORT_SYMBOL(it8528_init);

int lpc_ec_interrupt_occurs(void)
{
	int data;

	data = readl(lpc_reg_base + LPC_STATUS_REG);

	if (data & EC_EVENT_BIT) {
		return 1;
	}
	return 0;
}
EXPORT_SYMBOL(lpc_ec_interrupt_occurs);

void lpc_ec_interrupt_clear(void)
{
	int data;
	int int_mask = 	(EC_EVENT_BIT | I8042_KEY_BIT | I8042_TOUCH_BIT);

	data = readl(lpc_reg_base + LPC_STATUS_REG);

#if 0
    if ( data & ~int_mask) {
        printk(KERN_ERR "lpc_ec_interrupt_clear int_status = 0x%x \r\n", data);
    }
#endif

    /*clear unknow interrupt, if have */
    data &= int_mask;

    /*clear ec interrupt */
	data &= (~EC_EVENT_BIT);
	writel(data, lpc_reg_base + LPC_INTERRUPT_REG);

	data = readl(lpc_reg_base + LPC_STATUS_REG);
}
EXPORT_SYMBOL(lpc_ec_interrupt_clear);

void lpc_interrupt_clear_all(void)
{
    writel(0, lpc_reg_base + LPC_INTERRUPT_REG);
}
EXPORT_SYMBOL(lpc_interrupt_clear_all);



