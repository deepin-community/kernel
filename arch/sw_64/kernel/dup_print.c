// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/spinlock.h>

#include <asm/platform.h>
#include <asm/io.h>

#ifdef CONFIG_SW64_RRK

#define KERNEL_PRINTK_BUFF_BASE (0x700000UL + __START_KERNEL_map)

static DEFINE_SPINLOCK(printk_lock);

static unsigned long sw64_printk_offset;
#define PRINTK_SIZE	0x100000UL

static bool rrk_last_newline_end;
static unsigned long rrk_last_id;
static const char * const level_str[] = {
	"(0)EMERG",
	"(1)ALERT",
	"(2)CRIT",
	"(3)ERR",
	"(4)WARNING",
	"(5)NOTICE",
	"(6)INFO",
	"(7)DEBUG"
};
#define LEVEL_STR_MAX_LEN	10	// length of "(4)WARNING"

void sw64_rrk_store(const char *text, u16 text_len, u64 ts_nsec, int level,
		unsigned long id, bool newline_end)
{
	char *sw64_printk_buf;
	unsigned long flags;
	size_t __maybe_unused rrk_len;
	char header_buf[128];
	/* same time fmt as print_time() in printk.c */
	char header_fmt[] = "[%5llu.%06llu %-"__stringify(LEVEL_STR_MAX_LEN)"s] ";
	size_t header_len;
	char *newline;
	/* if writing a new entry while the last one did not end with '\n', print '\n' first */
	bool newline_first = rrk_last_id && (rrk_last_id != id) && (!rrk_last_newline_end);
	bool wrap = false;
	unsigned long max_offset_allowed;

	spin_lock_irqsave(&printk_lock, flags);

	header_len = scnprintf(header_buf, sizeof(header_buf), header_fmt,
			ts_nsec / NSEC_PER_SEC, (ts_nsec % NSEC_PER_SEC) / NSEC_PER_USEC,
			level >= 0 ? level_str[level] : "CONT");

	max_offset_allowed = PRINTK_SIZE - text_len - header_len - (newline_first ? 1 : 0);
	if (unlikely(sw64_printk_offset >= max_offset_allowed)) {
		sw64_printk_offset = 0;
		memset(sw64_printk_buf, 0, PRINTK_SIZE);
		wrap = true;
	}
	sw64_printk_buf = (char *)(KERNEL_PRINTK_BUFF_BASE + sw64_printk_offset);

	if (unlikely(newline_first)) {
		sw64_printk_buf[0] = '\n';
		sw64_printk_buf++;
		sw64_printk_offset++;
	}

	if (likely(level != -1) || unlikely(wrap)) {
		memcpy(sw64_printk_buf, header_buf, header_len);
		sw64_printk_offset += header_len;
		sw64_printk_buf += header_len;
	}

	while (unlikely((newline = strnchr(text, text_len, '\n')))) {
		size_t len;

		/* copy the first line */
		newline++;
		len = newline - text;
		memcpy(sw64_printk_buf, text, len);

		/* add padding for next line */
		memset(&sw64_printk_buf[len], ' ', header_len);

		text += len;
		text_len -= len;
		sw64_printk_buf += len + header_len;
		sw64_printk_offset += len + header_len;
	}

	memcpy(sw64_printk_buf, text, text_len);
	sw64_printk_offset += text_len;
	if (likely(sw64_printk_buf[text_len - 1] != '\n' && newline_end)) {
		sw64_printk_buf[text_len] = '\n';
		sw64_printk_offset++;
	}

	if (is_in_emul()) {
		void __iomem *addr = __va(QEMU_PRINTF_BUFF_BASE);
		u64 data = ((u64)sw64_printk_buf & 0xffffffffUL)
			| ((u64)text_len << 32);
		*(u64 *)addr = data;
	}

	rrk_last_id = id;
	rrk_last_newline_end = newline_end;

	spin_unlock_irqrestore(&printk_lock, flags);
}
#endif

#ifdef CONFIG_SW64_RRU
#include <linux/uaccess.h>

static DEFINE_SPINLOCK(printf_lock);
#define USER_PRINT_BUFF_BASE		(0x600000UL + __START_KERNEL_map)
#define USER_PRINT_BUFF_LEN		0x100000UL
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
