/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_DRIVER_LOG_H_
#define _PS3_DRIVER_LOG_H_

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/sched.h>

#include "ps3_htp_def.h"
#include "ps3_platform_utils.h"
#ifdef __cplusplus
extern "C" {
#endif

#define PS3_HOST(ins) ((ins)->host->host_no)
#define DRIVER_DEBUG

#define LOG_INFO_PREFIX_LEN 32
#define LOG_ERROR_PREFIX_LEN 33
#define MEGABYTE 20

enum debug_level {
	LEVEL_ERROR,
	LEVEL_WARN,
	LEVEL_INFO,
	LEVEL_DEBUG,
};

static inline const char *ps3_debug_level_name(enum debug_level lv)
{
	static const char * const level[] = {
		[LEVEL_ERROR] = "ERROR",
		[LEVEL_WARN] = "WARN",
		[LEVEL_INFO] = "INFO",
		[LEVEL_DEBUG] = "DBG",
	};

	return level[lv];
}

int ps3_level_get(void);
#define PS3_LOG_LIMIT_INTERVAL_MSEC (24 * 60 * 60 * 1000)
#ifdef __KERNEL__

#define PRINT_DEBUG LEVEL_DEBUG
#define PRINT_INFO LEVEL_INFO
#define PRINT_WARN LEVEL_WARN
#define PRINT_ERR LEVEL_ERROR

#define ps3_print(level, fmt, ...)						\
	do {									\
		if (level == LEVEL_DEBUG) {					\
			pr_debug("[PS3STOR]%s():%d;" fmt,			\
				__func__, __LINE__, ##__VA_ARGS__);		\
		} else if (level == LEVEL_INFO) {				\
			pr_info("[PS3STOR]%s():%d;" fmt,			\
				__func__, __LINE__, ##__VA_ARGS__);		\
		} else if (level == LEVEL_WARN) {				\
			pr_warn("[PS3STOR]%s():%d;" fmt,			\
				__func__, __LINE__, ##__VA_ARGS__);		\
		} else if (level == LEVEL_ERROR) {				\
			pr_err("[PS3STOR]%s():%d;" fmt,			\
				__func__, __LINE__, ##__VA_ARGS__);		\
		}								\
	} while (0)

#define ps3_printk(level, fmt, ...)                                            \
	do {                                                                   \
		if (level <= ps3_level_get()) {                                \
			if (level == LEVEL_DEBUG) {                            \
				pr_debug("[PS3STOR][%u]%d;" fmt,      \
				       current->pid, __LINE__, ##__VA_ARGS__); \
			} else if (level == LEVEL_INFO) {                      \
				pr_info("[PS3STOR][%u]%d;" fmt,       \
				       current->pid, __LINE__, ##__VA_ARGS__); \
			} else if (level == LEVEL_WARN) {                      \
				pr_warn("[PS3STOR][%u]%d;" fmt,    \
				       current->pid, __LINE__, ##__VA_ARGS__); \
			} else if (level == LEVEL_ERROR) {                     \
				pr_err("[PS3STOR][%u]%d;" fmt,        \
				       current->pid, __LINE__, ##__VA_ARGS__); \
			}                                                      \
		}                                                              \
	} while (0)

#define ps3_printk_ratelimited(level, fmt, ...)                                \
	do {                                                                   \
		if (level <= ps3_level_get()) {                                \
			if (level == LEVEL_INFO) {                             \
				pr_info_ratelimited(                           \
						    "[PS3STOR][%u]%d;" fmt,    \
						    current->pid, __LINE__,    \
						    ##__VA_ARGS__);            \
			} else if (level == LEVEL_WARN) {                      \
				pr_warn_ratelimited(                           \
						   "[PS3STOR][%u]%d;" fmt,     \
						   current->pid, __LINE__,     \
						   ##__VA_ARGS__);             \
			} else if (level == LEVEL_ERROR) {                     \
				pr_err_ratelimited(                            \
						   "[PS3STOR][%u]%d;" fmt,     \
						   current->pid, __LINE__,     \
						   ##__VA_ARGS__);             \
			}                                                      \
		}                                                              \
	} while (0)

#else

#define PRINT_DEBUG LEVEL_DEBUG
#define PRINT_INFO LEVEL_INFO
#define PRINT_WARN LEVEL_WARN
#define PRINT_ERR LEVEL_ERROR

#include <assert.h>
#include <sys/time.h>
#include <linux/mutex.h>
#define __percpu

static inline unsigned long long get_now_ms(void)
{
	struct timeval tv;
	unsigned long long timestamp = 0;

	gettimeofday(&tv, NULL);
	timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return timestamp;
}

#define filename_printf(x) (strrchr((x), '/') ? strrchr((x), '/') + 1 : (x))

#define ps3_print(lock_chk, level, fmt, ...)                                   \
	do {                                                                   \
		if (level != LEVEL_DEBUG && lock_chk) {                        \
			EXPECT_EQ(ps3_get_irq_spin_lock_count(), 0);           \
		}                                                              \
		if (level <= ps3_level_get()) {                                \
			if (level == LEVEL_DEBUG) {                            \
				printf("DEBUG:%llu:%s:%s():%d:[%lu];" fmt,     \
				       get_now_ms(),                           \
				       filename_printf(__FILE__),              \
				       __func__, __LINE__, pthread_self(), \
				       ##__VA_ARGS__);                         \
			} else if (level == LEVEL_INFO) {                      \
				printf("INFO:%llu:%s:%s():%d:[%lu];" fmt,      \
				       get_now_ms(),                           \
				       filename_printf(__FILE__),              \
				       __func__, __LINE__, pthread_self(), \
				       ##__VA_ARGS__);                         \
			} else if (level == LEVEL_WARN) {                      \
				printf("WARN:%llu:%s:%s():%d:[%lu];" fmt,      \
				       get_now_ms(),                           \
				       filename_printf(__FILE__),              \
				       __func__, __LINE__, pthread_self(), \
				       ##__VA_ARGS__);                         \
			} else if (level == LEVEL_ERROR) {                     \
				printf("ERROR:%llu:%s:%s():%d:[%lu];" fmt,     \
				       get_now_ms(),                           \
				       filename_printf(__FILE__),              \
				       __func__, __LINE__, pthread_self(), \
				       ##__VA_ARGS__);                         \
			}                                                      \
		}                                                              \
	} while (0)

#endif

#define LOG_BUG_ON(cond, fmt, ...)                                             \
	do {                                                                   \
		if ((cond)) {                                                  \
			LOG_ERROR(fmt, ##__VA_ARGS__);                         \
			LOG_SYNC();                                            \
			BUG();                                                 \
		}                                                              \
	} while (0)

#define DEBUG_TRACE_MAGIC 0x456789
#define LOG_BUF_SIZE (1024LL << 11)
#define BIN_BUF_SIZE (1024LL << 10)

#define LOG_FILE_SIZE (200LL << 20)
#define LOG_FILE_PATH "/var/log/ps3sas_drv.log"
#define LOG_FILE_PREFIX "ps3sas_drv.log"
#define BINARY_FILE_PREFIX "ps3sas_drv.bin"

#define BINARY_FILE_SIZE (200LL << 20)
#define BINARY_FILE_PATH "/var/log/ps3sas_drv.bin"

#define DEBUG_DROP_LOG_STRING "\nwarnning:drop some logs\n\n"

enum {
	DEBUG_TYPE_STRING,
	DEBUG_TYPE_BINARY,
	DEBUG_TYPE_NR,
};

struct debug_func {
	struct list_head list;
	char name[64];
};

struct debug_file {
	struct list_head list;
	char name[64];
};

struct ps3_log {
	struct {
		char *buf;
		int buf_size;
		long long head;
		long long tail;
		spinlock_t lock;
		unsigned char is_drop;
	};
	struct {
		char *file_path;
		struct file *file;
		long long file_pos;
		long long file_size;
		unsigned int file_num;
		unsigned int index;
	};
};

struct ps3_thread_local {
	int magic;
	char data[];
};

struct ps3_ctxt {
	struct page *page;
	void *buff;
};

struct ps3_thread_key {
	int offset;
};

struct ps3_debug {
	enum debug_level level;
	unsigned short key_offset;
	struct ps3_ctxt __percpu *ctxt;
	struct list_head filter_func;
	struct list_head filter_file;
	struct task_struct *task;
	struct ps3_log log[DEBUG_TYPE_NR];
};

void ps3_thread_key_create(int size, struct ps3_thread_key *key);
void *ps3_thread_get_specific(struct ps3_thread_key *key);
void ps3_thread_clear_specific(struct ps3_thread_key *key);

int ps3_filter_file_add(char *name);
void ps3_filter_file_del(char *name);
void ps3_filter_file_clear(void);

int ps3_filter_func_add(char *name);
void ps3_filter_func_del(char *name);
void ps3_filter_func_clear(void);
void ps3_level_set(int level);
int ps3_level_get(void);

int ps3_debug_init(void);
void ps3_debug_exit(void);

void ps3_log_string(enum debug_level level, const char *file, int line,
		    const char *fmt, ...);

void ps3_log_binary(const char *file, int line, char *ptr, int size, char *str);

void ps3_log_sync(void);

int ps3_ramfs_test_query(void);
void ps3_ramfs_test_store(int val);

#if defined DRIVER_DEBUG && defined __KERNEL__

#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))

#define WRITE_LOG(level, fmt, ...)                                             \
	ps3_log_string(level, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...) WRITE_LOG(LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) WRITE_LOG(LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) WRITE_LOG(LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) WRITE_LOG(LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define LOG2_DEBUG(fmt, ...)                                                   \
	do {                                                                   \
		WRITE_LOG(LEVEL_DEBUG, fmt, ##__VA_ARGS__);                            \
		ps3_print(PRINT_DEBUG, fmt, ##__VA_ARGS__);                            \
	} while (0)
#define LOG2_INFO(fmt, ...)                                                    \
	do {                                                                   \
		WRITE_LOG(LEVEL_INFO, fmt, ##__VA_ARGS__);                             \
		ps3_print(PRINT_INFO, fmt, ##__VA_ARGS__);                            \
	} while (0)
#define LOG2_WARN(fmt, ...)                                                    \
	do {                                                                   \
		WRITE_LOG(LEVEL_WARN, fmt, ##__VA_ARGS__);                             \
		ps3_print(PRINT_WARN, fmt, ##__VA_ARGS__);                            \
	} while (0)
#define LOG2_ERROR(fmt, ...)                                                   \
	do {                                                                   \
		WRITE_LOG(LEVEL_ERROR, fmt, ##__VA_ARGS__);                            \
		ps3_print(PRINT_ERR, fmt, ##__VA_ARGS__);                            \
	} while (0)

#define LOG_LEVEL(log_lvl, fmt, ...) WRITE_LOG(log_lvl, fmt, ##__VA_ARGS__)

#define LOG_INFO_LIM(fmt, ...) WRITE_LOG(LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN_LIM(fmt, ...) WRITE_LOG(LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR_LIM(fmt, ...) WRITE_LOG(LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define LOG_INFO_TIME_LIM(caller_jiffies, time, fmt, ...)                      \
	do {                                                                   \
		(void)caller_jiffies;                                          \
		(void)time;                                                    \
		WRITE_LOG(LEVEL_INFO, fmt, ##__VA_ARGS__);                     \
	} while (0)
#define LOG_WARN_TIME_LIM(caller_jiffies, time, fmt, ...)                      \
	do {                                                                   \
		(void)caller_jiffies;                                          \
		(void)time;                                                    \
		WRITE_LOG(LEVEL_WARN, fmt, ##__VA_ARGS__);                     \
	} while (0)
#define LOG_ERROR_TIME_LIM(caller_jiffies, time, fmt, ...)                     \
	do {                                                                   \
		(void)caller_jiffies;                                          \
		(void)time;                                                    \
		WRITE_LOG(LEVEL_ERROR, fmt, ##__VA_ARGS__);                    \
	} while (0)

#define LOG2_INFO_LIM(fmt, ...) WRITE_LOG(LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG2_WARN_LIM(fmt, ...) WRITE_LOG(LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG2_ERROR_LIM(fmt, ...) WRITE_LOG(LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define LOG_FILE_INFO(fmt, ...) WRITE_LOG(LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_FILE_WARN(fmt, ...) WRITE_LOG(LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_FILE_ERROR(fmt, ...) WRITE_LOG(LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define LOG_SPC_ERROR(fmt, ...) WRITE_LOG(LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define LOG_INFO_IN_IRQ(ins, fmt, ...) WRITE_LOG(LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN_IN_IRQ(ins, fmt, ...) WRITE_LOG(LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR_IN_IRQ(ins, fmt, ...)                                        \
	WRITE_LOG(LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define LOG_INFO_LIM_WITH_CHECK(ins, need_prk_err, fmt, ...)                   \
	WRITE_LOG(LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN_LIM_WITH_CHECK(ins, need_prk_err, fmt, ...)                   \
	WRITE_LOG(LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR_LIM_WITH_CHECK(ins, need_prk_err, fmt, ...)                  \
	WRITE_LOG(LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define LOG_SYNC() ps3_log_sync()
#define DATA_DUMP(ptr, size, str)                                              \
	ps3_log_binary(__FILE__, __LINE__, (char *)ptr, size, str)
#else

#define LOG_DEBUG(fmt, ...) ps3_printk(LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) ps3_printk(LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) ps3_printk(LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) ps3_printk(LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define LOG2_DEBUG(fmt, ...) ps3_print(PRINT_DEBUG, fmt, ##__VA_ARGS__)
#define LOG2_INFO(fmt, ...) ps3_print(PRINT_INFO, fmt, ##__VA_ARGS__)
#define LOG2_WARN(fmt, ...) ps3_print(PRINT_WARN, fmt, ##__VA_ARGS__)
#define LOG2_ERROR(fmt, ...) ps3_print(PRINT_ERR, fmt, ##__VA_ARGS__)

#define LOG_INFO_LIM(fmt, ...)                                                 \
	ps3_printk_ratelimited(LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN_LIM(fmt, ...)                                                 \
	ps3_printk_ratelimited(LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR_LIM(fmt, ...)                                                \
	ps3_printk_ratelimited(LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define LOG_INFO_TIME_LIM(caller_jiffies, time, fmt, ...)                      \
	do {                                                                   \
		if (printk_timed_ratelimit(caller_jiffies, time)) {            \
			ps3_printk(LEVEL_INFO, fmt, ##__VA_ARGS__);            \
		}                                                              \
	} while (0)
#define LOG_WARN_TIME_LIM(caller_jiffies, time, fmt, ...)                      \
	do {                                                                   \
		if (printk_timed_ratelimit(caller_jiffies, time)) {            \
			ps3_printk(LEVEL_WARN, fmt, ##__VA_ARGS__);            \
		}                                                              \
	} while (0)
#define LOG_ERROR_TIME_LIM(caller_jiffies, time, fmt, ...)                     \
	do {                                                                   \
		if (printk_timed_ratelimit(caller_jiffies, time)) {            \
			ps3_printk(LEVEL_ERROR, fmt, ##__VA_ARGS__);           \
		}                                                              \
	} while (0)

#define LOG2_INFO_LIM(fmt, ...) LOG_INFO_LIM(fmt, ##__VA_ARGS__)
#define LOG2_WARN_LIM(fmt, ...) LOG_WARN_LIM(fmt, ##__VA_ARGS__)
#define LOG2_ERROR_LIM(fmt, ...) LOG_ERROR_LIM(fmt, ##__VA_ARGS__)
#define LOG_SPC_ERROR(fmt, ...) LOG_INFO(fmt, ##__VA_ARGS__)

#define LOG_FILE_INFO(fmt, ...)
#define LOG_FILE_WARN(fmt, ...)
#define LOG_FILE_ERROR(fmt, ...)

#define LOG_LEVEL(log_lvl, fmt, ...) ps3_printk(log_lvl, fmt, ##__VA_ARGS__)

#define LOG_INFO_IN_IRQ(ins, fmt, ...)                                         \
	do {                                                                   \
		if ((ins)->is_irq_prk_support) {                               \
			ps3_printk(LEVEL_INFO, fmt, ##__VA_ARGS__);            \
		}                                                              \
	} while (0)

#define LOG_WARN_IN_IRQ(ins, fmt, ...)                                         \
	do {                                                                   \
		if ((ins)->is_irq_prk_support) {                               \
			ps3_printk(LEVEL_WARN, fmt, ##__VA_ARGS__);            \
		}                                                              \
	} while (0)

#define LOG_ERROR_IN_IRQ(ins, fmt, ...)                                        \
	do {                                                                   \
		if ((ins)->is_irq_prk_support) {                               \
			ps3_printk(LEVEL_ERROR, fmt, ##__VA_ARGS__);           \
		}                                                              \
	} while (0)
#define LOG_INFO_LIM_WITH_CHECK(ins, need_prk_err, fmt, ...)                   \
	do {                                                                   \
		if (need_prk_err) {                                            \
			LOG_INFO_LIM(fmt, ##__VA_ARGS__);                      \
		} else {                                                       \
			LOG_INFO_IN_IRQ((ins), fmt, ##__VA_ARGS__);            \
		}                                                              \
	} while (0)

#define LOG_WARN_LIM_WITH_CHECK(ins, need_prk_err, fmt, ...)                   \
	do {                                                                   \
		if (need_prk_err) {                                            \
			LOG_WARN_LIM(fmt, ##__VA_ARGS__);                      \
		} else {                                                       \
			LOG_WARN_IN_IRQ((ins), fmt, ##__VA_ARGS__);            \
		}                                                              \
	} while (0)

#define LOG_ERROR_LIM_WITH_CHECK(ins, need_prk_err, fmt, ...)                  \
	do {                                                                   \
		if (need_prk_err) {                                            \
			LOG_ERROR_LIM(fmt, ##__VA_ARGS__);                     \
		} else {                                                       \
			LOG_ERROR_IN_IRQ((ins), fmt, ##__VA_ARGS__);           \
		}                                                              \
	} while (0)

#define LOG_SYNC()
#define DATA_DUMP(ptr, size, str)
#endif

static inline unsigned char ps3_fs_requires_dev(struct file *fp)
{
	if (ps3_ramfs_test_query())
		return PS3_FALSE;
	return (fp->f_inode->i_sb->s_type->fs_flags & FS_REQUIRES_DEV);
}

#else

#define LOG_DEBUG(fmt, ...) ps3_print(PS3_TRUE, PRINT_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) ps3_print(PS3_TRUE, PRINT_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) ps3_print(PS3_TRUE, PRINT_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) ps3_print(PS3_TRUE, PRINT_ERR, fmt, ##__VA_ARGS__)

#define LOG2_DEBUG(fmt, ...)                                                   \
	ps3_print(PS3_TRUE, PRINT_DEBUG, fmt, ##__VA_ARGS__)
#define LOG2_INFO(fmt, ...) ps3_print(PS3_TRUE, PRINT_INFO, fmt, ##__VA_ARGS__)
#define LOG2_WARN(fmt, ...) ps3_print(PS3_TRUE, PRINT_WARN, fmt, ##__VA_ARGS__)
#define LOG2_ERROR(fmt, ...) ps3_print(PS3_TRUE, PRINT_ERR, fmt, ##__VA_ARGS__)

#define LOG_LEVEL(log_lvl, fmt, ...)                                           \
	ps3_print(PS3_TRUE, log_lvl, fmt, ##__VA_ARGS__)

#define LOG_INFO_LIM(fmt, ...)                                                 \
	ps3_print(PS3_TRUE, LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN_LIM(fmt, ...)                                                 \
	ps3_print(PS3_TRUE, LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR_LIM(fmt, ...)                                                \
	ps3_print(PS3_TRUE, LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define LOG_INFO_TIME_LIM(caller_jiffies, time, fmt, ...)                      \
	do {                                                                   \
		(void)caller_jiffies;                                          \
		(void)time;                                                    \
		ps3_print(PS3_TRUE, LEVEL_INFO, fmt, ##__VA_ARGS__);           \
	} while (0)
#define LOG_WARN_TIME_LIM(caller_jiffies, time, fmt, ...)                      \
	do {                                                                   \
		(void)caller_jiffies;                                          \
		(void)time;                                                    \
		ps3_print(PS3_TRUE, LEVEL_WARN, fmt, ##__VA_ARGS__);           \
	} while (0)
#define LOG_ERROR_TIME_LIM(caller_jiffies, time, fmt, ...)                     \
	do {                                                                   \
		(void)caller_jiffies;                                          \
		(void)time;                                                    \
		ps3_print(PS3_TRUE, LEVEL_ERROR, fmt, ##__VA_ARGS__);          \
	} while (0)

#define LOG2_INFO_LIM(fmt, ...) LOG_INFO_LIM(fmt, ##__VA_ARGS__)
#define LOG2_WARN_LIM(fmt, ...) LOG_WARN_LIM(fmt, ##__VA_ARGS__)
#define LOG2_ERROR_LIM(fmt, ...) LOG_ERROR_LIM(fmt, ##__VA_ARGS__)

#define LOG_FILE_INFO(fmt, ...)
#define LOG_FILE_WARN(fmt, ...)
#define LOG_FILE_ERROR(fmt, ...)

#define LOG_SPC_ERROR(fmt, ...) LOG_INFO(fmt, ##__VA_ARGS__)

#define LOG_INFO_IN_IRQ(ins, fmt, ...)                                         \
	do {                                                                   \
		(void)ins;                                                     \
		ps3_print(PS3_FALSE, LEVEL_INFO, fmt, ##__VA_ARGS__);          \
	} while (0)
#define LOG_WARN_IN_IRQ(ins, fmt, ...)                                         \
	do {                                                                   \
		(void)ins;                                                     \
		ps3_print(PS3_FALSE, LEVEL_WARN, fmt, ##__VA_ARGS__);          \
	} while (0)
#define LOG_ERROR_IN_IRQ(ins, fmt, ...)                                        \
	do {                                                                   \
		(void)ins;                                                     \
		ps3_print(PS3_FALSE, LEVEL_ERROR, fmt, ##__VA_ARGS__);         \
	} while (0)
#define LOG_INFO_LIM_WITH_CHECK(ins, need_prk_err, fmt, ...)                   \
	do {                                                                   \
		(void)ins;                                                     \
		ps3_print((need_prk_err), LEVEL_INFO, fmt, ##__VA_ARGS__);     \
	} while (0)

#define LOG_WARN_LIM_WITH_CHECK(ins, need_prk_err, fmt, ...)                   \
	do {                                                                   \
		(void)ins;                                                     \
		ps3_print((need_prk_err), LEVEL_WARN, fmt, ##__VA_ARGS__);     \
	} while (0)

#define LOG_ERROR_LIM_WITH_CHECK(ins, need_prk_err, fmt, ...)                  \
	do {                                                                   \
		(void)ins;                                                     \
		ps3_print((need_prk_err), LEVEL_ERROR, fmt, ##__VA_ARGS__);    \
	} while (0)

#define LOG_SYNC()
#define DATA_DUMP(ptr, size, str)

static inline unsigned char ps3_fs_requires_dev(struct file *fp)
{
	(void)fp;
	return PS3_TRUE;
}
#endif

#ifdef PS3_CFG_RELEASE

#define PS3_BUG()
#define PS3_BUG_NO_SYNC()

#if defined(PS3_CFG_OCM_DBGBUG) || defined(PS3_CFG_OCM_RELEASE)

#define PS3_BUG_ON(cond)                                                       \
	do {                                                                   \
		if ((cond)) {                                                  \
			pr_err("BUG_ON's condition(%s) has been triggered\n",  \
			       #cond);                                         \
			LOG_ERROR(                                             \
				"BUG_ON's condition(%s) has been triggered\n", \
				#cond);                                        \
		}                                                              \
	} while (0)

#define PS3_BUG_ON_NO_SYNC(cond)                                               \
	do {                                                                   \
		if ((cond)) {                                                  \
			pr_err("BUG_ON's condition(%s) has been triggered\n",  \
			       #cond);                                         \
			LOG_ERROR(                                             \
				"BUG_ON's condition(%s) has been triggered\n", \
				#cond);                                        \
		}                                                              \
	} while (0)

#else
#define PS3_BUG_ON(cond)                                                       \
	do {                                                                   \
		if ((cond)) {                                                  \
			pr_err("BUG_ON's condition(%s) has been triggered\n",  \
			       #cond);                                         \
		}                                                              \
	} while (0)

#define PS3_BUG_ON_NO_SYNC(cond)                                               \
	do {                                                                   \
		if ((cond)) {                                                  \
			pr_err("BUG_ON's condition(%s) has been triggered\n",  \
			       #cond);                                         \
		}                                                              \
	} while (0)

#endif

#else
#define PS3_BUG_ON(cond)                                                       \
	do {                                                                   \
		if ((cond)) {                                                  \
			pr_err("BUG_ON's condition(%s) has been triggered\n",  \
			       #cond);                                         \
			LOG_ERROR(                                             \
				"BUG_ON's condition(%s) has been triggered\n", \
				#cond);                                        \
			LOG_SYNC();                                            \
		}                                                              \
		BUG_ON(cond);                                                  \
	} while (0)

#define PS3_BUG(void)                                                          \
	do {                                                                   \
		LOG_SYNC();                                                    \
		BUG(void);                                                     \
	} while (0)

#define PS3_BUG_ON_NO_SYNC(cond)                                               \
	do {                                                                   \
		if ((cond)) {                                                  \
			pr_err(                                                \
			       "BUG_ON's condition(%s) has been triggered\n",  \
			       #cond);                                         \
			LOG_ERROR(                                             \
				"BUG_ON's condition(%s) has been triggered\n", \
				#cond);                                        \
		}                                                              \
		BUG_ON(cond);                                                  \
	} while (0)

#define PS3_BUG_NO_SYNC(void) BUG(void)

#endif

#define PS3_WARN_ON(cond) WARN_ON(cond)

#ifdef __cplusplus
}
#endif
#endif
