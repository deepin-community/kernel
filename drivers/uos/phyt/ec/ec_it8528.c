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

static int send_ec_command(unsigned char command)
{
	int timeout, ret = 0;

	timeout = EC_SEND_TIMEOUT;
	while ((inb(EC_STS_PORT) & EC_IBF) && --timeout)
		udelay(1);
	if (!timeout) {
		pr_err("Timeout while sending command 0x%02x to EC!\n", command);
		ret = -1;
		goto out;
	}

	outb(command, EC_CMD_PORT);

out:
	return ret;
}

static int send_ec_data(unsigned char data)
{
	int timeout, ret = 0;

	timeout = EC_SEND_TIMEOUT;
	while ((inb(EC_STS_PORT) & EC_IBF) && --timeout)
		udelay(1);
	if (!timeout) {
		pr_err("Timeout while sending data 0x%02x to EC!\n", data);
		ret = -1;
		goto out;
	}

	outb(data, EC_DAT_PORT);

out:
	return ret;
}

static unsigned char recv_ec_data(void)
{
	int timeout;
	unsigned char data;

	timeout = EC_RECV_TIMEOUT;
	while (!(inb(EC_STS_PORT) & EC_OBF) && --timeout)
		udelay(1);
	if (!timeout) {
		pr_err("Timeout while receiving data from EC! status 0x%x.\n", inb(EC_STS_PORT));
		data = 0;
		goto skip_data;
	}

	data = inb(EC_DAT_PORT);

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
		pr_err("Send command fail!\n");
		value = 0;
		goto out;
	}
	ret = send_ec_data(index);
	if (ret < 0) {
		pr_err("Send data fail!\n");
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
		pr_err("Send command fail!\n");
		goto out;
	}
	ret = send_ec_data(index);
	if (ret < 0) {
		pr_err("Send data fail!\n");
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
		pr_err("Send command fail!\n");
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
		pr_err("Send command 0x81 fail!\n");
		goto out;
	}
	ret = send_ec_data(index);
	if (ret < 0) {
		pr_err("Send index 0x%x fail!\n", index);
		goto out;
	}

	ret = send_ec_data(data);
	if (ret < 0)
		pr_err("Send data 0x%x fail!\n", data);
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
	unsigned char value = 0;
	unsigned long flags;
	int ret = 0;
	unsigned int timeout;

	spin_lock_irqsave(&index_access_lock, flags);
	ret = send_ec_command(CMD_GET_EVENT_NUM);
	if (ret < 0) {
		pr_err("Send command fail!\n");
		goto out;
	}

	/* check if the command is received by ec */
	timeout = EC_CMD_TIMEOUT;
	while ((inb(EC_STS_PORT) & EC_IBF) && timeout--)
		udelay(1);
	if (timeout <= 0) {
		pr_err("EC QUERY SEQ: deadable error : timeout...\n");
		ret = -EINVAL;
		goto out;
	}

	value = recv_ec_data();
	spin_unlock_irqrestore(&index_access_lock, flags);
	return value;
out:
	spin_unlock_irqrestore(&index_access_lock, flags);
	return ret;
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
	int ret = 0;
	unsigned int timeout;

	spin_lock_irqsave(&index_access_lock, flags);

	ret = send_ec_command(CMD_ENABLE_EVENT_EC);
	if (ret < 0) {
		pr_err("Send command fail!\n");
		goto out;
	}

	/* check if the command is received by ec */
	timeout = EC_CMD_TIMEOUT;
	while ((inb(EC_STS_PORT) & EC_IBF) && timeout--)
		udelay(1);
	if (timeout <= 0) {
		pr_err("EC ENABLE EVENT INTERRUPT: deadable error : timeout...\n");
		ret = -EINVAL;
		goto out;
	}
out:
	spin_unlock_irqrestore(&index_access_lock, flags);
}
EXPORT_SYMBOL(it8528_ec_event_int_enable);

void it8528_ec_event_int_disable(void)
{
	unsigned long flags;
	int ret = 0;
	unsigned int timeout;

	spin_lock_irqsave(&index_access_lock, flags);

	ret = send_ec_command(CMD_DISABLE_EVENT_EC);
	if (ret < 0) {
		pr_err("Send command fail!\n");
		goto out;
	}

	/* check if the command is received by ec */
	timeout = EC_CMD_TIMEOUT;
	while ((inb(EC_STS_PORT) & EC_IBF) && timeout--)
		udelay(1);

	if (timeout <= 0) {
		pr_err("EC ENABLE EVENT INTERRUPT: deadable error : timeout...\n");
		ret = -EINVAL;
		goto out;
	}

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
