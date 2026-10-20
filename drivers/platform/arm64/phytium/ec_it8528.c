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

/*
 * Read one EC register. Returns 0 on success or a negative error code so
 * callers can tell a communication failure from a genuine zero reading.
 */
int it8528_read_byte(unsigned char index, u8 *value)
{
	return it8528_transaction(CMD_READ_EC, &index, 1, value, 1);
}
EXPORT_SYMBOL(it8528_read_byte);

int it8528_write(unsigned char index, unsigned char data)
{
	u8 wdata[2] = { index, data };

	return it8528_transaction(CMD_WRITE_EC, wdata, 2, NULL, 0);
}
EXPORT_SYMBOL(it8528_write);

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
	while ((inb(EC_STS_PORT) & EC_IBF) && --timeout)
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
	while ((inb(EC_STS_PORT) & EC_IBF) && --timeout)
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
	while ((inb(EC_STS_PORT) & EC_IBF) && --timeout)
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

/*
 * Acknowledge the EC interrupt source only. The PIO clear register treats a
 * zero bit as "clear this source", so the complemented mask leaves the
 * keyboard and touchpad sources pending for i8042 instead of dropping them.
 */
void lpc_ec_interrupt_clear(void)
{
	phytium_pio_clear_interrupt(~EC_EVENT_BIT);
}
EXPORT_SYMBOL(lpc_ec_interrupt_clear);
