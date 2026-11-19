// SPDX-License-Identifier: GPL-2.0
#include <linux/types.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/pagemap.h>
#include <linux/uaccess.h>
#include <linux/fsnotify.h>
#include <linux/rtc.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/compiler.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#ifdef __KERNEL__
#include <linux/init.h>
#endif

#include "ps3_driver_log.h"
#include "ps3_module_para.h"
#include "ps3_kernel_version.h"
#include "ps3_util.h"

#if defined DRIVER_DEBUG && defined __KERNEL__

#define FILE_NAME_SIZE 128
#define PS3_KLOG_OUT_WAIT (5 * HZ)
#define DRIVER_SWITCH_FILE
#define LOG_PATH_LEN 100
#define DRV_LOG_FILE_SIZE_MIN_MB 10
#define DRV_LOG_FILE_SIZE_MAX_MB 200

struct ps3_debug g_ps3_debug;
char g_log_path_str[LOG_PATH_LEN] = { 0 };
char g_log_path_bin[LOG_PATH_LEN] = { 0 };

static inline int time_for_log(char *buff, int buf_len)
{
#if defined(PS3_DUMP_TIME_32)
	struct timeval tv;
	struct tm td;

	do_gettimeofday(&tv);
	time_to_tm(tv.tv_sec, -sys_tz.tz_minuteswest * 60, &td);

	return snprintf(buff, buf_len, "[%04ld-%02d-%02d;%02d:%02d:%02d.%ld]",
			td.tm_year + 1900, td.tm_mon + 1, td.tm_mday,
			td.tm_hour, td.tm_min, td.tm_sec, tv.tv_usec);
#else
	struct timespec64 tv;
	struct tm td;

	ktime_get_real_ts64(&tv);
	time64_to_tm(tv.tv_sec, -sys_tz.tz_minuteswest * 60, &td);
	return snprintf(buff, buf_len, "[%04ld-%02d-%02d;%02d:%02d:%02d.%ld]",
			td.tm_year + 1900, td.tm_mon + 1, td.tm_mday,
			td.tm_hour, td.tm_min, td.tm_sec, tv.tv_nsec * 1000);
#endif
}

static inline int time_for_file_name(char *buff, int buf_len)
{
#if defined(PS3_DUMP_TIME_32)
	struct timeval tv;
	struct tm td;

	do_gettimeofday(&tv);
	time_to_tm(tv.tv_sec, -sys_tz.tz_minuteswest * 60, &td);
#else
	struct timespec64 tv;
	struct tm td;

	ktime_get_real_ts64(&tv);
	time64_to_tm(tv.tv_sec, -sys_tz.tz_minuteswest * 60, &td);
#endif
	return snprintf(buff, buf_len, "%04ld-%02d-%02d_%02d:%02d:%02d",
			td.tm_year + 1900, td.tm_mon + 1, td.tm_mday,
			td.tm_hour, td.tm_min, td.tm_sec);
}

static inline char *ps3_stack_top(void)
{
#if defined(PS3_THREAD_INFO)
	unsigned long *ptr = (unsigned long *)(current->thread_info + 1);
#else
	unsigned long *ptr = (unsigned long *)(task_thread_info(current) + 1);
#endif
	return (char *)(ptr + 1);
}

static inline struct ps3_thread_local *ps3_thread_local_get(struct ps3_thread_key *key)
{
	return (struct ps3_thread_local *)(ps3_stack_top() + key->offset);
}

void ps3_thread_key_create(int size, struct ps3_thread_key *key)
{
	key->offset = g_ps3_debug.key_offset;
	g_ps3_debug.key_offset += sizeof(struct ps3_thread_local) + size;
}

void *ps3_thread_get_specific(struct ps3_thread_key *key)
{
	struct ps3_thread_local *local = ps3_thread_local_get(key);

	if (local->magic != DEBUG_TRACE_MAGIC)
		return NULL;
	return (void *)local->data;
}

void ps3_thread_clear_specific(struct ps3_thread_key *key)
{
	struct ps3_thread_local *local = ps3_thread_local_get(key);

	local->magic = 0;
}

int ps3_filter_file_add(char *name)
{
	struct debug_file *file = NULL;

	file = kmalloc(sizeof(struct debug_file), GFP_ATOMIC);
	if (!file) {
		ps3_print(PRINT_ERR, "kmalloc size %lu failed\n", PAGE_SIZE);
		return -ENOMEM;
	}
	PS3_STRCPY(file->name, name, sizeof(file->name));
	INIT_LIST_HEAD(&file->list);

	list_add_rcu(&file->list, &g_ps3_debug.filter_file);
	return 0;
}

void ps3_filter_file_del(char *filename)
{
	struct debug_file *file = NULL;

	list_for_each_entry_rcu(file, &g_ps3_debug.filter_file, list) {
		if (!strcmp(file->name, filename)) {
			list_del_rcu(&file->list);
			synchronize_rcu();
			kfree(file);
			return;
		}
	}
}

#ifndef PS3_CFG_RELEASE
static inline int ps3_filter_file_print(const char *filename)
{
	struct debug_file *file;

	rcu_read_lock();
	list_for_each_entry_rcu(file, &g_ps3_debug.filter_file, list) {
		if (!strcmp(file->name, filename)) {
			rcu_read_unlock();
			return 1;
		}
	}
	rcu_read_unlock();
	return 0;
}

#endif
void ps3_filter_file_clear(void)
{
	struct debug_file *file = NULL;

	do {
		file = list_first_or_null_rcu(&g_ps3_debug.filter_file,
					      struct debug_file, list);
		if (file) {
			list_del_rcu(&file->list);
			synchronize_rcu();
			kfree(file);
		}
	} while (file);
}

int ps3_filter_func_add(char *name)
{
	struct debug_func *func = NULL;

	func = kmalloc(sizeof(struct debug_func), GFP_ATOMIC);
	if (!func) {
		ps3_print(PRINT_ERR, "kmalloc size %lu failed\n", PAGE_SIZE);
		return -ENOMEM;
	}
	PS3_STRCPY(func->name, name, sizeof(func->name));
	INIT_LIST_HEAD(&func->list);

	list_add_rcu(&func->list, &g_ps3_debug.filter_func);
	return 0;
}

void ps3_filter_func_del(char *name)
{
	struct debug_func *func = NULL;

	list_for_each_entry_rcu(func, &g_ps3_debug.filter_func, list) {
		if (!strcmp(func->name, name)) {
			list_del_rcu(&func->list);
			synchronize_rcu();
			kfree(func);
			return;
		}
	}
}

void ps3_filter_func_clear(void)
{
	struct debug_func *func = NULL;

	do {
		func = list_first_or_null_rcu(&g_ps3_debug.filter_func,
					      struct debug_func, list);
		if (func) {
			list_del_rcu(&func->list);
			synchronize_rcu();
			kfree(func);
		}
	} while (func);
}

void ps3_level_set(int level)
{
	g_ps3_debug.level = level;
}

int ps3_level_get(void)
{
	return (int)g_ps3_debug.level;
}

#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))

static void ps3_file_close(struct file **file)
{
	filp_close(*file, NULL);
	*file = NULL;
}

static int ps3_file_open(struct ps3_log *log, struct file **pp_file)
{
	struct file *file;
	int flags_new = O_CREAT | O_RDWR | O_APPEND | O_LARGEFILE;
	int flags_rewrite = O_CREAT | O_RDWR | O_LARGEFILE | O_TRUNC;
	int err = 0;
	int len = 0;
	char filename[FILE_NAME_SIZE];
	static unsigned long j;
#ifdef DRIVER_SWITCH_FILE
	memset(filename, 0, FILE_NAME_SIZE);
	len += snprintf(filename, FILE_NAME_SIZE, "%s", log->file_path);
	if (log->file_num == 0) {
		time_for_file_name(filename + len, FILE_NAME_SIZE - len);
	} else {
		snprintf(filename + len, FILE_NAME_SIZE - len, "%04d",
			 log->index++);
		log->index = log->index % log->file_num;
	}

	if (log->file_num == 1 && log->file != NULL) {
		ps3_file_close(&log->file);
		log->file_pos = 0;
	}
#else
	memset(filename, 0, FILE_NAME_SIZE);
	PS3_STRCPY(filename, path, FILE_NAME_SIZE);
#endif
	if (log->file_num == 0) {
		file = filp_open(filename, flags_new, 0666);
	} else {
		file = filp_open(filename, flags_rewrite, 0666);
		if (IS_ERR(file)) {
			err = (int)PTR_ERR(file);
			if (err == -ENOENT)
				file = filp_open(filename, flags_new, 0666);
		}
	}
	if (IS_ERR(file)) {
		err = (int)PTR_ERR(file);
		if (printk_timed_ratelimit(&j, PS3_LOG_LIMIT_INTERVAL_MSEC)) {
			ps3_print(PRINT_INFO, "open file:%s failed[errno:%d]\n",
				  filename, err);
		}
		goto l_out;
	}

	if (!ps3_fs_requires_dev(file)) {
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
		if (printk_timed_ratelimit(&j, PS3_LOG_LIMIT_INTERVAL_MSEC)) {
			ps3_print(
				PRINT_INFO,
				"unexpected filesystem, superblock flags: 0x%x\n",
				file->f_inode->i_sb->s_type->fs_flags);
		}
#endif
		ps3_file_close(&file);
		err = -EINVAL;
		goto l_out;
	}

	mapping_set_gfp_mask(file->f_path.dentry->d_inode->i_mapping, GFP_NOFS);

	ps3_print(PRINT_INFO, "redirect file %s\n", filename);

	*pp_file = file;

l_out:
	return err;
}

static void ps3_file_sync(struct file *file)
{
	struct address_space *mapping;
	void *journal;
	int ret = 0;
	int err;

	(void)ret;
	(void)err;

	if (!file || !file->f_op || !file->f_op->fsync)
		goto l_end;

	journal = current->journal_info;
	current->journal_info = NULL;

	mapping = file->f_mapping;

	ret = filemap_fdatawrite(mapping);

#if defined(PS3_FILEMAP_WAIT)
	mutex_lock(&mapping->host->i_mutex);
	err = file->f_op->fsync(file, file->f_path.dentry, 1);
	if (!ret)
		ret = err;
	mutex_unlock(&mapping->host->i_mutex);
	err = filemap_fdatawait(mapping);
	if (!ret)
		ret = err;

#else
	err = file->f_op->fsync(file, 0, file->f_mapping->host->i_size, 1);
#endif

	current->journal_info = journal;

l_end:
	return;
}

static int ps3_file_write(struct file *file, char *buf, int len)
{
	int ret = 0;

	void *journal;
#if defined(PS3_SUPPORT_FS)
	mm_segment_t old_fs;
#endif

#if defined(PS3_KERNEL_WRITE_GET_DS)
	old_fs = get_fs();
	set_fs(get_ds());
#elif defined(PS3_KERNEL_WRITE)
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#elif defined(PS3_VFS_WRITE)
#else
	old_fs = force_uaccess_begin();
#endif

	journal = current->journal_info;
	current->journal_info = NULL;

	if (!file) {
		return 0;
	}

	do {
#if defined(PS3_KERNEL_WRITE_FILE_OP)
		ret = file->f_op->write(file, buf, len, &file->f_pos);
#elif defined(PS3_KERNEL_WRITE_FILE_VFS)
		ret = vfs_write(file, buf, len, &file->f_pos);
#else
		ret = kernel_write(file, buf, len, &file->f_pos);
#endif
	} while (ret == -EINTR);

	if (ret >= 0) {
#if defined(PS3_FSNOTIFY_FILE)
		fsnotify_modify(file);
#elif defined(PS3_FSNOTIFY_FILE_PATH)
		if (file->f_path.dentry)
			fsnotify_modify(file->f_path.dentry);
#endif
	}

	current->journal_info = journal;
#if defined(PS3_SET_FS)
	set_fs(old_fs);
#elif defined(PS3_FORCE_UACCESS)
#else
	force_uaccess_end(old_fs);
#endif

	return ret;
}

static void ps3_klog_in(struct ps3_log *log, char *buf, const int len)
{
	int begin = 0;
	int end = 0;
	int free_size;
	unsigned long flags;
	static unsigned long j;

	spin_lock_irqsave(&log->lock, flags);

	if (log->head > log->tail) {
		if (printk_timed_ratelimit(&j, PS3_LOG_LIMIT_INTERVAL_MSEC)) {
			ps3_print(PRINT_INFO,
				  "FAILURE: log head exceeds log tail\n");
			PS3_BUG_NO_SYNC();
		}
	}

	free_size = log->buf_size - (log->tail - log->head);

	if (free_size <= len) {
		log->is_drop = 1;
		spin_unlock_irqrestore(&log->lock, flags);
		return;
	}

	begin = log->tail % log->buf_size;
	end = (log->tail + len) % log->buf_size;

	if (begin < end) {
		memcpy(log->buf + begin, buf, len);
	} else {
		memcpy(log->buf + begin, buf, log->buf_size - begin);
		memcpy(log->buf, buf + log->buf_size - begin, end);
	}

	log->tail = log->tail + len;

	spin_unlock_irqrestore(&log->lock, flags);
}

static void ps3_klog_out(struct ps3_log *log)
{
	int len = 0;
	int rc = 0;
	long long tail;
	int begin;
	int end;
	int schedule_count_th = 0;
	const int max_loop = 4096;
	static unsigned long j;

#ifdef DRIVER_SWITCH_FILE
	struct file *file = NULL;
#endif

	if (log->file == NULL) {
		rc = ps3_file_open(log, &log->file);
		if (log->file != NULL)
			log->file_pos = 0;
		else
			return;
	}

	do {
		tail = log->tail;
		begin = log->head % log->buf_size;
		end = tail % log->buf_size;
		len = 0;
		rc = 0;

		schedule_count_th++;
		if (schedule_count_th >= max_loop) {
			schedule_count_th = 0;
			schedule_timeout_interruptible(PS3_KLOG_OUT_WAIT);
		}

		if (log->is_drop) {
			rc = ps3_file_write(log->file, DEBUG_DROP_LOG_STRING,
					    strlen(DEBUG_DROP_LOG_STRING));
			if (rc < 0)
				break;
			log->is_drop = 0;
		}

		if (begin < end) {
			rc = ps3_file_write(log->file, log->buf + begin,
					    end - begin);
			if (rc > 0)
				len += rc;
		} else if (begin > end) {
			rc = ps3_file_write(log->file, log->buf + begin,
					    log->buf_size - begin);
			if (rc > 0) {
				len += rc;
				rc = ps3_file_write(log->file, log->buf, end);
				if (rc > 0)
					len += rc;
			}
		}
		log->head += len;
		log->file_pos += len;

		LOG_BUG_ON(log->head > log->tail,
			   "FAILURE: log head exceeds log tail\n");
	} while (log->head != log->tail && rc > 0);

	if (rc < 0) {
		if (printk_timed_ratelimit(&j, PS3_LOG_LIMIT_INTERVAL_MSEC)) {
			ps3_print(PRINT_INFO, "write file %s error %d\n",
				  log->file_path, rc);
		}
		return;
	}

#ifdef DRIVER_SWITCH_FILE
	if (log->file_pos >= log->file_size) {
		rc = ps3_file_open(log, &file);
		if (rc >= 0 && log->file != NULL && log->file_num != 1) {
			ps3_file_close(&log->file);
			log->file = file;
			log->file_pos = 0;
		}
	}
#endif
}

static int ps3_klog_flush(void *arg)
{
	int i;

	while (!kthread_should_stop()) {
		schedule_timeout_interruptible(PS3_KLOG_OUT_WAIT);

		for (i = 0; i < ARRAY_SIZE(g_ps3_debug.log); i++)
			ps3_klog_out(&g_ps3_debug.log[i]);
	}
	return 0;
}

static int ps3_klog_init(struct ps3_log *log, long long buf_size, char *file_path,
			 long long file_size, unsigned int file_num)
{
	int rc = 0;

	memset(log, 0, sizeof(*log));
	spin_lock_init(&log->lock);

	log->buf = vmalloc(buf_size + PAGE_SIZE);
	if (!log->buf) {
		rc = -ENOMEM;
		goto l_end;
	}

	log->file = NULL;
	log->head = 0;
	log->tail = 0;
	log->buf_size = buf_size;

	log->file_path = file_path;
	log->file_pos = 0;
	log->file_size = file_size;
	log->file_num = file_num;
	log->index = 0;
l_end:
	return rc;
}

static void ps3_klog_exit(struct ps3_log *log)
{
	if (log->buf)
		vfree(log->buf);
	if (log->file)
		ps3_file_close(&log->file);
}

static inline char *ps3_file_name_locale(char *file)
{
	char *p_slash = strrchr(file, '/');

	return (p_slash == NULL) ? file : (p_slash + 1);
}

void ps3_log_string(enum debug_level level, const char *file, int line,
		    const char *fmt, ...)
{
	struct ps3_ctxt *ctxt = NULL;
	char *buf = NULL;
	int len = 0;
	unsigned long flags = 0;

	va_list args;

	if (level > g_ps3_debug.level) {
#ifndef PS3_CFG_RELEASE
		if (!ps3_filter_file_print(file))
			return;
#else
		return;
#endif
	}

	if (!in_interrupt())
		local_irq_save(flags);

	ctxt = per_cpu_ptr(g_ps3_debug.ctxt, get_cpu());
	put_cpu();

	buf = ctxt->buff;

	len = snprintf(buf, PAGE_SIZE, "%s", ps3_debug_level_name(level));
	len += time_for_log(buf + len, PAGE_SIZE - len);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"[%d][%d]%s:%4d:", raw_smp_processor_id(), current->pid,
			ps3_file_name_locale((char *)file), line);

	va_start(args, fmt);
	len += vsnprintf(buf + len, PAGE_SIZE - len, fmt, args);
	va_end(args);

	if (!in_interrupt())
		local_irq_restore(flags);

	if (ps3_log_tty_query()) {
		if (buf[0] == 'I' || buf[0] == 'W') {
			pr_warn_ratelimited("%s",
					   buf + LOG_INFO_PREFIX_LEN);
		} else if (buf[0] == 'E') {
			pr_warn_ratelimited("%s",
					   buf + LOG_ERROR_PREFIX_LEN);
		}
	}
	ps3_klog_in(&g_ps3_debug.log[DEBUG_TYPE_STRING], buf, len);

	wake_up_process(g_ps3_debug.task);
}

void ps3_log_binary(const char *file, int line, char *ptr, int size, char *str)
{
#define LINE_TOTAL 16
	struct ps3_ctxt *ctxt = NULL;
	char *buf = NULL;
	int len = 0;
	int i;
	unsigned long flags = 0;

	if (!in_interrupt())
		local_irq_save(flags);

	ctxt = per_cpu_ptr(g_ps3_debug.ctxt, get_cpu());
	put_cpu();

	buf = ctxt->buff;

	len += time_for_log(buf + len, PAGE_SIZE - len);
	len += snprintf(buf + len, PAGE_SIZE - len, "[%d]%s:%d, size:%d, ",
			current->pid, ps3_file_name_locale((char *)file), line,
			size);

	len += snprintf(buf + len, PAGE_SIZE - len, "%s", str);

	for (i = 0; i < size; i++) {
		if (i % LINE_TOTAL == 0)
			len += snprintf(buf + len, PAGE_SIZE - len, "%08x ", i);

		len += snprintf(buf + len, PAGE_SIZE - len, "%02hhx", ptr[i]);
		if ((i % LINE_TOTAL) == (LINE_TOTAL - 1) || i == (size - 1))
			len += snprintf(buf + len, PAGE_SIZE - len, "\n");
		else if ((i % LINE_TOTAL) == (LINE_TOTAL / 2 - 1))
			len += snprintf(buf + len, PAGE_SIZE - len, "   ");
		else
			len += snprintf(buf + len, PAGE_SIZE - len, " ");
	}

	if (!in_interrupt())
		local_irq_restore(flags);

	ps3_klog_in(&g_ps3_debug.log[DEBUG_TYPE_BINARY], buf, len);

	wake_up_process(g_ps3_debug.task);
}

void ps3_log_sync(void)
{
	ps3_file_sync(g_ps3_debug.log[DEBUG_TYPE_STRING].file);
	ps3_file_sync(g_ps3_debug.log[DEBUG_TYPE_BINARY].file);
}

int ps3_debug_init(void)
{
	struct task_struct *task = NULL;
	struct ps3_ctxt *ctxt = NULL;
	int rc = 0;
	int i;
	int nid;
	unsigned int file_num = 0;
	unsigned int log_path_len = 0;
	unsigned int input_log_space = ps3_log_space_size_query();
	unsigned int input_log_file_size = ps3_log_file_size_query();
	unsigned int log_file_size = 0;
	char *log_path_p = NULL;
	struct ps3_log *log_bin = &g_ps3_debug.log[DEBUG_TYPE_BINARY];
	struct ps3_log *log_str = &g_ps3_debug.log[DEBUG_TYPE_STRING];

	INIT_LIST_HEAD(&g_ps3_debug.filter_file);
	INIT_LIST_HEAD(&g_ps3_debug.filter_func);

	g_ps3_debug.level = ps3_log_level_query();
	g_ps3_debug.ctxt = alloc_percpu(struct ps3_ctxt);
	if (!g_ps3_debug.ctxt) {
		rc = -ENOMEM;
		ps3_print(PRINT_ERR, "alloc percpu failed\n");
		goto l_end;
	}

	for_each_possible_cpu(i) {
		ctxt = per_cpu_ptr(g_ps3_debug.ctxt, i);
		memset(ctxt, 0, sizeof(*ctxt));
	}

	for_each_possible_cpu(i) {
		ctxt = per_cpu_ptr(g_ps3_debug.ctxt, i);
		nid = cpu_to_node(i);

		ctxt->page = alloc_pages_node(nid, GFP_ATOMIC, 0);
		if (!ctxt->page) {
			rc = -ENOMEM;
			ps3_print(PRINT_ERR, "kmalloc size %lu failed\n",
				  PAGE_SIZE);
			goto l_free_cpu_buff;
		}
		ctxt->buff = PS3_KMAP(ctxt->page);
	}

	log_path_p = ps3_log_path_query();
	log_path_len = strlen(log_path_p);
	if (log_path_p != NULL && log_path_p[0] == '/') {
		if (log_path_p[log_path_len - 1] == '/') {
			snprintf(g_log_path_str, LOG_PATH_LEN, "%s%s.",
				 log_path_p, LOG_FILE_PREFIX);
			snprintf(g_log_path_bin, LOG_PATH_LEN, "%s%s.",
				 log_path_p, BINARY_FILE_PREFIX);
		} else {
			snprintf(g_log_path_str, LOG_PATH_LEN, "%s/%s.",
				 log_path_p, LOG_FILE_PREFIX);
			snprintf(g_log_path_bin, LOG_PATH_LEN, "%s/%s.",
				 log_path_p, BINARY_FILE_PREFIX);
		}
	} else {
		snprintf(g_log_path_str, LOG_PATH_LEN, "%s.", LOG_FILE_PATH);
		snprintf(g_log_path_bin, LOG_PATH_LEN, "%s.", BINARY_FILE_PATH);
	}
	if (input_log_file_size < DRV_LOG_FILE_SIZE_MIN_MB ||
	    input_log_file_size > DRV_LOG_FILE_SIZE_MAX_MB) {
		ps3_log_file_size_modify(LOG_FILE_SIZE >> MEGABYTE);
		input_log_file_size = LOG_FILE_SIZE >> MEGABYTE;
	}
	if (input_log_space && input_log_space < input_log_file_size) {
		ps3_log_file_size_modify(input_log_space);
		input_log_file_size = input_log_space;
	}
	log_file_size = input_log_file_size << MEGABYTE;

	if (input_log_space) {
		file_num = input_log_space / input_log_file_size;
		if (file_num == 0) {
			ps3_print(PRINT_ERR, "filenum shouldnot be 0\n");
			PS3_BUG();
		}
	} else {
		file_num = 0;
	}

	rc = ps3_klog_init(log_str, LOG_BUF_SIZE, g_log_path_str, log_file_size,
			   file_num);
	if (rc < 0)
		goto l_free_cpu_buff;

	rc = ps3_klog_init(log_bin, BIN_BUF_SIZE, g_log_path_bin,
			   BINARY_FILE_SIZE, 0);
	if (rc < 0)
		goto l_free_string;

	task = kthread_create(ps3_klog_flush, NULL, "ps3_klog_flush");
	if (IS_ERR(task)) {
		rc = (int)PTR_ERR(task);
		ps3_print(PRINT_ERR, "Create kernel thread, err: %d\n", rc);
		goto l_free_binary;
	}
	wake_up_process(task);
	g_ps3_debug.task = task;
	rc = 0;
	ps3_print(PRINT_INFO,
		  "PS3 debug init logpath[%s] strlogsize[%dM] filenum[%d]\n",
		  g_log_path_str, (log_file_size >> MEGABYTE),
		  log_str->file_num);
l_end:
	return rc;

l_free_binary:
	ps3_klog_exit(&g_ps3_debug.log[DEBUG_TYPE_BINARY]);

l_free_string:
	ps3_klog_exit(&g_ps3_debug.log[DEBUG_TYPE_STRING]);

l_free_cpu_buff:
	for_each_possible_cpu(i) {
		ctxt = per_cpu_ptr(g_ps3_debug.ctxt, i);
		if (ctxt && ctxt->page) {
			PS3_KUNMAP(ctxt->page, ctxt->buff);
			__free_page(ctxt->page);
		}
	}
	free_percpu(g_ps3_debug.ctxt);
	goto l_end;
}

void ps3_debug_exit(void)
{
	int i = 0;
	struct ps3_ctxt *ctxt;

	if (g_ps3_debug.task == NULL)
		return;

	kthread_stop(g_ps3_debug.task);

	for (i = 0; i < ARRAY_SIZE(g_ps3_debug.log); i++)
		ps3_klog_exit(&g_ps3_debug.log[i]);

	if (g_ps3_debug.ctxt) {
		for_each_possible_cpu(i) {
			ctxt = per_cpu_ptr(g_ps3_debug.ctxt, i);
			if (ctxt && ctxt->page) {
				PS3_KUNMAP(ctxt->page, ctxt->buff);
				__free_page(ctxt->page);
			}
		}

		free_percpu(g_ps3_debug.ctxt);
		g_ps3_debug.ctxt = NULL;
	}
}
#else

void ps3_log_sync(void)
{
}

int ps3_debug_init(void)
{
	g_ps3_debug.level = LEVEL_WARN;
	return 0;
};

void ps3_debug_exit(void)
{
}

#endif

#else

int g_ps3_log_level = LEVEL_INFO;

void ps3_thread_key_create(int size, struct ps3_thread_key *key)
{
	(void)size;
	(void)key;
}

void *ps3_thread_get_specific(struct ps3_thread_key *key)
{
	return key;
}

void ps3_thread_clear_specific(struct ps3_thread_key *key)
{
	(void)key;
}

int ps3_filter_file_add(char *name)
{
	(void)name;
	return 0;
}
void ps3_filter_file_del(char *name)
{
	(void)name;
}
void ps3_filter_file_clear(void)
{
}

int ps3_filter_func_add(char *name)
{
	(void)name;
	return 0;
}
void ps3_filter_func_del(char *name)
{
	(void)name;
}
void ps3_filter_func_clear(void)
{
}
void ps3_level_set(int level)
{
	g_ps3_log_level = level;
}

int ps3_level_get(void)
{
	return g_ps3_log_level;
}

void ps3_log_sync(void)
{
}

int ps3_debug_init(void)
{
	g_ps3_log_level = LEVEL_WARN;
	return 0;
};

void ps3_debug_exit(void)
{
}

#endif

static int g_ramfs_test_enable;
int ps3_ramfs_test_query(void)
{
	return g_ramfs_test_enable;
}

void ps3_ramfs_test_store(int val)
{
	g_ramfs_test_enable = val;
}
