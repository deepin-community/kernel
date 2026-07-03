// SPDX-License-Identifier: GPL-2.0
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
#include <linux/io.h>
#include "ec_it8528.h"

/* This spinlock is dedicated for 62&66 ports and super io port access. */
DEFINE_SPINLOCK(index_access_lock);

static int ec_wait_ibf_clear(void)
{
	unsigned int timeout = EC_SEND_TIMEOUT;

	while ((inb(EC_STS_PORT) & EC_IBF) && --timeout)
		udelay(1);
	if (!timeout) {
		pr_err("Timeout waiting for EC IBF clear (status 0x%x)\n",
			inb(EC_STS_PORT));
		return -ETIMEDOUT;
	}
	return 0;
}

static int ec_wait_obf_set(u8 *data)
{
	unsigned int timeout = EC_RECV_TIMEOUT;

	while (!(inb(EC_STS_PORT) & EC_OBF) && --timeout)
		udelay(1);
	if (!timeout) {
		pr_err("Timeout waiting for EC OBF set (status 0x%x)\n",
			inb(EC_STS_PORT));
		return -ETIMEDOUT;
	}
	*data = inb(EC_DAT_PORT);
	return 0;
}

static int ec_send_byte(u8 byte, u16 port)
{
	int ret = ec_wait_ibf_clear();

	if (ret)
		return ret;
	outb(byte, port);
	return 0;
}

/*
 * One EC transaction: command + optional write data + optional read data.
 * The whole sequence is protected by index_access_lock.
 */
static int it8528_transaction(u8 command, const u8 *wdata, int wlen,
			      u8 *rdata, int rlen)
{
	unsigned long flags;
	int i, ret;
	u8 val;

	spin_lock_irqsave(&index_access_lock, flags);

	ret = ec_send_byte(command, EC_CMD_PORT);
	if (ret)
		goto out;

	for (i = 0; i < wlen; i++) {
		ret = ec_send_byte(wdata[i], EC_DAT_PORT);
		if (ret)
			goto out;
	}

	for (i = 0; i < rlen; i++) {
		ret = ec_wait_obf_set(&val);
		if (ret)
			goto out;
		rdata[i] = val;
	}

out:
	spin_unlock_irqrestore(&index_access_lock, flags);
	return ret;
}

unsigned char it8528_read(unsigned char index)
{
	u8 value = 0;

	it8528_transaction(CMD_READ_EC, &index, 1, &value, 1);
	return value;
}
EXPORT_SYMBOL(it8528_read);

unsigned char it8528_read_all(unsigned char command, unsigned char index)
{
	u8 value = 0;

	it8528_transaction(command, &index, 1, &value, 1);
	return value;
}
EXPORT_SYMBOL(it8528_read_all);

unsigned char it8528_read_noindex(unsigned char command)
{
	u8 value = 0;

	it8528_transaction(command, NULL, 0, &value, 1);
	return value;
}
EXPORT_SYMBOL(it8528_read_noindex);

int it8528_write(unsigned char index, unsigned char data)
{
	u8 wdata[2] = { index, data };

	return it8528_transaction(CMD_WRITE_EC, wdata, 2, NULL, 0);
}
EXPORT_SYMBOL(it8528_write);

int it8528_write_all(unsigned char command, unsigned char index,
		     unsigned char data)
{
	u8 wdata[2] = { index, data };

	return it8528_transaction(command, wdata, 2, NULL, 0);
}
EXPORT_SYMBOL(it8528_write_all);

int it8528_write_noindex(unsigned char command, unsigned char data)
{
	return it8528_transaction(command, &data, 1, NULL, 0);
}
EXPORT_SYMBOL(it8528_write_noindex);

bool it8528_get_ec_ibf_flags(void)
{
	return !!(inb(EC_STS_PORT) & EC_IBF);
}
EXPORT_SYMBOL(it8528_get_ec_ibf_flags);

bool it8528_get_ec_obf_flags(void)
{
	return !!(inb(EC_STS_PORT) & EC_OBF);
}
EXPORT_SYMBOL(it8528_get_ec_obf_flags);

bool it8528_get_ec_evt_flags(void)
{
	return !!(inb(EC_STS_PORT) & EC_SCI_EVT);
}
EXPORT_SYMBOL(it8528_get_ec_evt_flags);

int it8528_query_get_event_num(void)
{
	unsigned long flags;
	unsigned int timeout;
	u8 value = 0;
	int ret;

	spin_lock_irqsave(&index_access_lock, flags);

	ret = ec_send_byte(CMD_GET_EVENT_NUM, EC_CMD_PORT);
	if (ret)
		goto out;

	/* Give the EC a moment to consume the command. */
	timeout = EC_CMD_TIMEOUT;
	while ((inb(EC_STS_PORT) & EC_IBF) && timeout--)
		udelay(1);
	if (!timeout) {
		pr_err("EC QUERY SEQ: timeout\n");
		ret = -ETIMEDOUT;
		goto out;
	}

	ret = ec_wait_obf_set(&value);
out:
	spin_unlock_irqrestore(&index_access_lock, flags);
	return ret ? ret : value;
}
EXPORT_SYMBOL(it8528_query_get_event_num);

int it8528_query_clean_event(void)
{
	int ret;

	while (inb(EC_STS_PORT) & EC_SCI_EVT) {
		ret = it8528_query_get_event_num();
		if (ret <= 0) {
			pr_info("Clean sci event done!\n");
			return ret;
		}
	}

	return 0;
}
EXPORT_SYMBOL(it8528_query_clean_event);

void it8528_ec_event_int_enable(void)
{
	unsigned long flags;
	unsigned int timeout;
	int ret;

	spin_lock_irqsave(&index_access_lock, flags);

	ret = ec_send_byte(CMD_ENABLE_EVENT_EC, EC_CMD_PORT);
	if (ret)
		goto out;

	timeout = EC_CMD_TIMEOUT;
	while ((inb(EC_STS_PORT) & EC_IBF) && timeout--)
		udelay(1);
	if (!timeout)
		pr_err("EC ENABLE EVENT INTERRUPT: timeout\n");

out:
	spin_unlock_irqrestore(&index_access_lock, flags);
}
EXPORT_SYMBOL(it8528_ec_event_int_enable);

void it8528_ec_event_int_disable(void)
{
	unsigned long flags;
	unsigned int timeout;
	int ret;

	spin_lock_irqsave(&index_access_lock, flags);

	ret = ec_send_byte(CMD_DISABLE_EVENT_EC, EC_CMD_PORT);
	if (ret)
		goto out;

	timeout = EC_CMD_TIMEOUT;
	while ((inb(EC_STS_PORT) & EC_IBF) && timeout--)
		udelay(1);
	if (!timeout)
		pr_err("EC DISABLE EVENT INTERRUPT: timeout\n");

out:
	spin_unlock_irqrestore(&index_access_lock, flags);
}
EXPORT_SYMBOL(it8528_ec_event_int_disable);

int it8528_init(void)
{
	if (unlikely(!check_cpu_type())) {
		pr_err("Stop init on non-Phytium CPU!\n");
		return -ENODEV;
	}
	return 0;
}
EXPORT_SYMBOL(it8528_init);

int lpc_ec_interrupt_occurs(void)
{
	int data;

	data = phytium_pio_get_int_status();

	if (data & EC_EVENT_BIT)
		return 1;
	return 0;
}
EXPORT_SYMBOL(lpc_ec_interrupt_occurs);

void lpc_interrupt_clear_all(void)
{
	phytium_pio_clear_interrupt(0);
}
EXPORT_SYMBOL(lpc_interrupt_clear_all);
