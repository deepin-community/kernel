// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/spinlock.h>

#include <asm/platform.h>
#include <asm/io.h>

#ifdef CONFIG_SW64_RRK

#define KERNEL_PRINTK_BUFF_BASE (0x700000UL + __START_KERNEL_map)

static DEFINE_SPINLOCK(printk_lock);

unsigned long sw64_printk_offset;
#define PRINTK_SIZE	0x100000UL

void sw64_printk(const char *fmt, va_list args)
{
	char *sw64_printk_buf;
	unsigned long flags;
	char textbuf[1024];
	const char *text;
	size_t text_len;

	spin_lock_irqsave(&printk_lock, flags);

	sw64_printk_buf = (char *)(KERNEL_PRINTK_BUFF_BASE  + sw64_printk_offset);

	text_len = vscnprintf(textbuf, sizeof(textbuf), fmt, args);
	text = printk_skip_headers(textbuf);
	text_len -= text - textbuf;

	if (sw64_printk_offset >= (PRINTK_SIZE - 1024)) {
		sw64_printk_offset = 0;
		sw64_printk_buf = (char *)(KERNEL_PRINTK_BUFF_BASE  + sw64_printk_offset);
		memset(sw64_printk_buf, 0, PRINTK_SIZE);
	}

	memcpy(sw64_printk_buf, text, text_len);
	sw64_printk_offset += text_len;

	if (is_in_emul()) {
		void __iomem *addr = __va(QEMU_PRINTF_BUFF_BASE);
		u64 data = ((u64)sw64_printk_buf & 0xffffffffUL)
			| ((u64)text_len << 32);
		*(u64 *)addr = data;
	}

	spin_unlock_irqrestore(&printk_lock, flags);
}
#endif

#ifdef CONFIG_SW64_RRU
#include <linux/uaccess.h>

static DEFINE_SPINLOCK(printf_lock);
#define USER_PRINT_BUFF_BASE            (0x600000UL + __START_KERNEL_map)
#define USER_PRINT_BUFF_LEN             0x100000UL
#define USER_MESSAGE_MAX_LEN		0x100000UL
unsigned long sw64_printf_offset;
int sw64_user_printf(const char __user *buf, int len)
{
	static char *user_printf_buf;
	unsigned long flags;

	if (current->pid <= 0)
		return 0;

	/*
	 * do not write large (fake) message which may not be from
	 * STDOUT/STDERR any more as file descriptor could be duplicated
	 * in a pipe.
	 */
	if (len > USER_MESSAGE_MAX_LEN)
		return 0;

	spin_lock_irqsave(&printf_lock, flags);
	user_printf_buf = (char *)(USER_PRINT_BUFF_BASE + sw64_printf_offset);

	if (sw64_printf_offset == 0)
		memset(user_printf_buf, 0, USER_PRINT_BUFF_LEN);

	if ((sw64_printf_offset + len) > USER_PRINT_BUFF_LEN) {
		sw64_printf_offset = 0;
		user_printf_buf = (char *)(USER_PRINT_BUFF_BASE + sw64_printf_offset);
		memset(user_printf_buf, 0, USER_PRINT_BUFF_LEN);
	}
	copy_from_user(user_printf_buf, buf, len);
	sw64_printf_offset += len;
	spin_unlock_irqrestore(&printf_lock, flags);
	return 0;
}
#endif
