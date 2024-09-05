
#ifndef _SXE_LOG_H_
#define _SXE_LOG_H_

#include "sxe_log_types.h"

#ifdef SXE_TEST
#define STATIC
#else
#define STATIC static
#endif

#ifdef __cplusplus
extern "C"{
#endif

#define SXE_HOST(ins) (ins)->host->host_no

#define LOG_INFO_PREFIX_LEN	32
#define LOG_ERROR_PREFIX_LEN	33
#define MEGABYTE		20

typedef enum {
	LEVEL_ERROR,
	LEVEL_WARN,
	LEVEL_INFO,
	LEVEL_DEBUG,
}debug_level_e;

static inline const S8 *sxe_debug_level_name(debug_level_e lv)
{
	static const S8 *level[] = {
		[LEVEL_ERROR] = "ERROR",
		[LEVEL_WARN]  = "WARN",
		[LEVEL_INFO]  = "INFO",
		[LEVEL_DEBUG] = "DEBUG",
	};

	return level[lv];
}

#ifdef __KERNEL__

#define PRINT_DEBUG KERN_DEBUG
#define PRINT_INFO  KERN_INFO
#define PRINT_WARN  KERN_WARNING
#define PRINT_ERR   KERN_ERR

#define sxe_print(level,bdf, fmt,...) \
	printk(level"[SXE]%s():%d:" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else

#define PRINT_DEBUG    LEVEL_DEBUG
#define PRINT_INFO     LEVEL_INFO
#define PRINT_WARN     LEVEL_WARN
#define PRINT_ERR      LEVEL_ERROR

#include <assert.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>

#define __percpu

static inline U64 get_now_ms() {
	struct timeval tv;
	U64 timestamp = 0;
	gettimeofday(&tv, NULL);
	timestamp = tv.tv_sec * 1000 + tv.tv_usec/1000;
	return timestamp;
}

#define filename_printf(x) strrchr((x),'/')?strrchr((x),'/')+1:(x)

#define sxe_print(level,bdf, fmt,...) do { \
	if (level <= sxe_level_get()) { \
		if (level == LEVEL_DEBUG) { \
			printf("DEBUG:%llu:%s:%s():%d:[%lu][%s];" fmt, get_now_ms(), filename_printf(__FILE__), \
				__FUNCTION__, __LINE__, pthread_self(), bdf ? bdf : "", ##__VA_ARGS__); \
		} else if (level == LEVEL_INFO) { \
			printf("INFO:%llu:%s:%s():%d:[%lu][%s];" fmt, get_now_ms(), filename_printf(__FILE__), \
				__FUNCTION__, __LINE__, pthread_self(), bdf ? bdf : "",  ##__VA_ARGS__); \
		} else if (level == LEVEL_WARN) { \
			printf("WARN:%llu:%s:%s():%d:[%lu][%s];" fmt, get_now_ms(), filename_printf(__FILE__), \
				__FUNCTION__, __LINE__, pthread_self(), bdf ? bdf : "",  ##__VA_ARGS__); \
		}else if (level == LEVEL_ERROR) { \
			printf("ERROR:%llu:%s:%s():%d:[%lu][%s];" fmt, get_now_ms(), filename_printf(__FILE__), \
				__FUNCTION__, __LINE__, pthread_self(), bdf ? bdf : "",  ##__VA_ARGS__); \
		} \
	} \
} while(0)

#endif

#define LOG_BUG_ON(cond, fmt, ...) do { \
	if((cond)) { \
		LOG_ERROR(fmt, ##__VA_ARGS__); \
		LOG_SYNC();							\
		BUG();								\
	}									\
}while(0)

#define DEBUG_TRACE_MAGIC 0x456789
#define BUF_SIZE		 (1024LL << 10)

#define PAGE_ORDER   2
#define PER_CPU_PAGE_SIZE  (PAGE_SIZE * (1 << 2))

#define LOG_FILE_SIZE	 (200LL << 20)  
#define BINARY_FILE_SIZE (200LL << 20) 

#define VF_LOG_FILE_PATH	 "/var/log/sxevf.log"
#define VF_LOG_FILE_PREFIX       "sxevf.log"
#define VF_BINARY_FILE_PATH      "/var/log/sxevf.bin"
#define VF_BINARY_FILE_PREFIX    "sxevf.bin"

#define LOG_FILE_PATH	         "/var/log/sxe.log"
#define LOG_FILE_PREFIX          "sxe.log"
#define BINARY_FILE_PATH         "/var/log/sxe.bin"
#define BINARY_FILE_PREFIX       "sxe.bin"

#define DEBUG_DROP_LOG_STRING "\nwarnning:drop some logs\n\n"

enum {
	DEBUG_TYPE_STRING,
	DEBUG_TYPE_BINARY,
	DEBUG_TYPE_NR,
};

typedef struct {
	struct list_head list;
	char name[64];
} debug_func_t;

typedef struct {
	struct list_head list;
	char name[64];
} debug_file_t;

typedef struct {
	struct {
		char		*buf;			
		int		buf_size;
		long long  head;			
		long long  tail;			
		spinlock_t lock;
		unsigned char	   is_drop;		
	};
	struct {
		char		*file_path;		
		struct file	*file;			
		long long	file_pos;		
		long long	file_size;		
		U32		file_num;		
		U32		index;			
	};
} sxe_log_t;

typedef struct {
	s32  magic;
	char data[0];
} sxe_thread_local_t;

typedef struct {
	struct page *page;
	void		*buff;
} sxe_ctxt_t;

typedef struct {
	s32  offset;
} sxe_thread_key_t;

typedef struct {
   debug_level_e	level;
   bool 		status;
   u16					key_offset;
   sxe_ctxt_t __percpu *ctxt;
   struct list_head		filter_func;
   struct list_head		filter_file;
   struct task_struct  *task;
   sxe_log_t			log[DEBUG_TYPE_NR];
} sxe_debug_t;

void sxe_level_set(int level);
s32 sxe_level_get(void);

void sxe_bin_status_set(bool status);
s32 sxe_bin_status_get(void);

int  sxe_log_init(bool is_vf);
void sxe_log_exit(void);

void sxe_log_string(debug_level_e level, const char *dev_name, const char *file, const char *func,
	int line, const char *fmt, ...);

void sxe_log_binary(const char *file, const char *func, int line, u8 *ptr,
	u64 addr, u32 size, char *str);

#define DATA_DUMP(ptr, size, str) \
	sxe_log_binary(__FILE__, __FUNCTION__, __LINE__, (u8*)ptr, 0, size, str)

void sxe_log_sync(void);

#ifdef SXE_DRIVER_TRACE
int time_for_file_name(char *buff, int buf_len);
int sxe_file_write(struct file *file, char *buf, int len);
#endif 

#if defined SXE_DRIVER_DEBUG && defined __KERNEL__

#define WRITE_LOG(level, bdf, fmt, ...) \
	sxe_log_string(level, bdf, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...)     WRITE_LOG(LEVEL_DEBUG, NULL, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)	WRITE_LOG(LEVEL_INFO,NULL,  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)	WRITE_LOG(LEVEL_WARN, NULL,  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)     WRITE_LOG(LEVEL_ERROR, NULL, fmt, ##__VA_ARGS__)

#define LOG_DEBUG_BDF(fmt, ...) WRITE_LOG(LEVEL_DEBUG, adapter->dev_name, fmt, ##__VA_ARGS__)
#define LOG_INFO_BDF(fmt, ...)	WRITE_LOG(LEVEL_INFO,adapter->dev_name,  fmt, ##__VA_ARGS__)
#define LOG_WARN_BDF(fmt, ...)	WRITE_LOG(LEVEL_WARN, adapter->dev_name,  fmt, ##__VA_ARGS__)
#define LOG_ERROR_BDF(fmt, ...) WRITE_LOG(LEVEL_ERROR, adapter->dev_name, fmt, ##__VA_ARGS__)

#define LOG_SYNC()			sxe_log_sync()

#define LOG_DEV_DEBUG(format, arg...) \
				dev_dbg(&adapter->pdev->dev, format, ## arg); \
				LOG_DEBUG_BDF(format, ## arg)

#define LOG_DEV_INFO(format, arg...) \
				dev_info(&adapter->pdev->dev, format, ## arg); \
				LOG_INFO_BDF(format, ## arg)

#define LOG_DEV_WARN(format, arg...) \
				dev_warn(&adapter->pdev->dev, format, ## arg); \
				LOG_WARN_BDF(format, ## arg)

#define LOG_DEV_ERR(format, arg...) \
				dev_err(&adapter->pdev->dev, format, ## arg); \
				LOG_ERROR_BDF(format, ## arg)

#define LOG_MSG_DEBUG(msglvl, format, arg...) \
	netif_dbg(adapter, msglvl, adapter->netdev, format, ## arg); \
	LOG_DEBUG_BDF(format, ## arg)

#define LOG_MSG_INFO(msglvl, format, arg...) \
	netif_info(adapter, msglvl, adapter->netdev, format, ## arg); \
	LOG_INFO_BDF(format, ## arg)

#define LOG_MSG_WARN(msglvl, format, arg...) \
	netif_warn(adapter, msglvl, adapter->netdev, format, ## arg); \
	LOG_WARN_BDF(format, ## arg)

#define LOG_MSG_ERR(msglvl, format, arg...) \
	netif_err(adapter, msglvl, adapter->netdev, format, ## arg); \
	LOG_ERROR_BDF(format, ## arg)

#define LOG_PR_DEBUG(format, arg...)    pr_debug("sxe: "format, ## arg);
#define LOG_PR_INFO(format, arg...)     pr_info("sxe: "format, ## arg);
#define LOG_PR_WARN(format, arg...)     pr_warn("sxe: "format, ## arg);
#define LOG_PR_ERR(format, arg...)      pr_err("sxe: "format, ## arg);
#define LOG_PRVF_DEBUG(format, arg...)  pr_debug("sxevf: "format, ## arg);
#define LOG_PRVF_INFO(format, arg...)   pr_info("sxevf: "format, ## arg);
#define LOG_PRVF_WARN(format, arg...)   pr_warn("sxevf: "format, ## arg);
#define LOG_PRVF_ERR(format, arg...)    pr_err("sxevf: "format, ## arg);

#else

#if defined SXE_DRIVER_RELEASE

#define LOG_DEBUG(fmt, ...)
#define LOG_INFO(fmt, ...)
#define LOG_WARN(fmt, ...)
#define LOG_ERROR(fmt, ...)

#define UNUSED(x) (void)(x)

#define LOG_DEBUG_BDF(fmt, ...)   UNUSED(adapter)
#define LOG_INFO_BDF(fmt, ...)    UNUSED(adapter)
#define LOG_WARN_BDF(fmt, ...)    UNUSED(adapter)
#define LOG_ERROR_BDF(fmt, ...)   UNUSED(adapter)

#define LOG_DEV_DEBUG(format, arg...) \
				dev_dbg(&adapter->pdev->dev, format, ## arg);

#define LOG_DEV_INFO(format, arg...) \
				dev_info(&adapter->pdev->dev, format, ## arg);

#define LOG_DEV_WARN(format, arg...) \
				dev_warn(&adapter->pdev->dev, format, ## arg);

#define LOG_DEV_ERR(format, arg...) \
				dev_err(&adapter->pdev->dev, format, ## arg);

#define LOG_MSG_DEBUG(msglvl, format, arg...) \
	netif_dbg(adapter, msglvl, adapter->netdev, format, ## arg);

#define LOG_MSG_INFO(msglvl, format, arg...) \
	netif_info(adapter, msglvl, adapter->netdev, format, ## arg);

#define LOG_MSG_WARN(msglvl, format, arg...) \
	netif_warn(adapter, msglvl, adapter->netdev, format, ## arg);

#define LOG_MSG_ERR(msglvl, format, arg...) \
	netif_err(adapter, msglvl, adapter->netdev, format, ## arg);

#define LOG_PR_DEBUG(format, arg...)    pr_debug("sxe: "format, ## arg);
#define LOG_PR_INFO(format, arg...)     pr_info("sxe: "format, ## arg);
#define LOG_PR_WARN(format, arg...)     pr_warn("sxe: "format, ## arg);
#define LOG_PR_ERR(format, arg...)      pr_err("sxe: "format, ## arg);
#define LOG_PRVF_DEBUG(format, arg...)  pr_debug("sxevf: "format, ## arg);
#define LOG_PRVF_INFO(format, arg...)   pr_info("sxevf: "format, ## arg);
#define LOG_PRVF_WARN(format, arg...)   pr_warn("sxevf: "format, ## arg);
#define LOG_PRVF_ERR(format, arg...)    pr_err("sxevf: "format, ## arg);

#else

#define LOG_DEBUG(fmt, ...)       sxe_print(PRINT_DEBUG,  "",  fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)	  sxe_print(PRINT_INFO,	  "", fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)	  sxe_print(PRINT_WARN, "", fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)	  sxe_print(PRINT_ERR,	"",   fmt, ##__VA_ARGS__)

#define LOG_DEBUG_BDF(fmt, ...)   sxe_print(LEVEL_DEBUG, adapter->dev_name, fmt, ##__VA_ARGS__)
#define LOG_INFO_BDF(fmt, ...)	  sxe_print(LEVEL_INFO,adapter->dev_name,  fmt, ##__VA_ARGS__)
#define LOG_WARN_BDF(fmt, ...)	  sxe_print(LEVEL_WARN, adapter->dev_name,  fmt, ##__VA_ARGS__)
#define LOG_ERROR_BDF(fmt, ...)   sxe_print(LEVEL_ERROR, adapter->dev_name, fmt, ##__VA_ARGS__)

#define LOG_DEV_DEBUG(fmt, ...) \
				sxe_print(LEVEL_DEBUG, adapter->dev_name, fmt, ##__VA_ARGS__)
#define LOG_DEV_INFO(fmt, ...) \
				sxe_print(LEVEL_INFO, adapter->dev_name,  fmt, ##__VA_ARGS__)
#define LOG_DEV_WARN(fmt, ...) \
				sxe_print(LEVEL_WARN, adapter->dev_name,  fmt, ##__VA_ARGS__)
#define LOG_DEV_ERR(fmt, ...) \
				sxe_print(LEVEL_ERROR, adapter->dev_name, fmt, ##__VA_ARGS__)

#define LOG_MSG_DEBUG(msglvl, fmt, ...) \
				sxe_print(LEVEL_DEBUG, adapter->dev_name, fmt, ##__VA_ARGS__)
#define LOG_MSG_INFO(msglvl, fmt, ...) \
				sxe_print(LEVEL_INFO, adapter->dev_name, fmt, ##__VA_ARGS__)
#define LOG_MSG_WARN(msglvl, fmt, ...) \
				sxe_print(LEVEL_WARN, adapter->dev_name, fmt, ##__VA_ARGS__)
#define LOG_MSG_ERR(msglvl, fmt, ...) \
				sxe_print(LEVEL_ERROR, adapter->dev_name, fmt, ##__VA_ARGS__)

#define LOG_PR_DEBUG(fmt, ...) \
	sxe_print(PRINT_DEBUG,	"sxe",  fmt, ##__VA_ARGS__);

#define LOG_PR_INFO(fmt, ...) \
	sxe_print(PRINT_INFO,	"sxe",  fmt, ##__VA_ARGS__);

#define LOG_PR_WARN(fmt, ...) \
	sxe_print(PRINT_WARN,	"sxe",  fmt, ##__VA_ARGS__);

#define LOG_PR_ERR(fmt, ...) \
	sxe_print(PRINT_ERR,	"sxe",  fmt, ##__VA_ARGS__);
#define LOG_PRVF_DEBUG(fmt, ...) \
		sxe_print(PRINT_DEBUG,	"sxevf",	fmt, ##__VA_ARGS__);

#define LOG_PRVF_INFO(fmt, ...) \
		sxe_print(PRINT_INFO,	"sxevf",	fmt, ##__VA_ARGS__);

#define LOG_PRVF_WARN(fmt, ...) \
		sxe_print(PRINT_WARN,	"sxevf",	fmt, ##__VA_ARGS__);

#define LOG_PRVF_ERR(fmt, ...) \
		sxe_print(PRINT_ERR,	"sxevf",	fmt, ##__VA_ARGS__);

#endif

#define LOG_SYNC()

#endif

#if defined SXE_DRIVER_RELEASE
#define SXE_BUG_ON(cond) do { \
	if((cond)) { \
		printk(KERN_ERR "BUG_ON's condition(%s) has been triggered\n", #cond); \
		LOG_ERROR("BUG_ON's condition(%s) has been triggered\n", #cond); \
	}									\
}while(0)

#define SXE_BUG()
#define SXE_BUG_ON_NO_SYNC(cond) do {	\
	if((cond)) { \
		printk(KERN_ERR "BUG_ON's condition(%s) has been triggered\n", #cond); \
		LOG_ERROR("BUG_ON's condition(%s) has been triggered\n", #cond); \
	}									\
}while(0)

#define SXE_BUG_NO_SYNC()
#else
#define SXE_BUG_ON(cond) do { \
	if((cond)) { \
		printk(KERN_ERR "BUG_ON's condition(%s) has been triggered\n", #cond); \
		LOG_ERROR("BUG_ON's condition(%s) has been triggered\n", #cond); \
		LOG_SYNC();							\
	}									\
	BUG_ON(cond);							\
}while(0)

#define SXE_BUG(void) do {	\
	LOG_SYNC();		\
	BUG(void);		\
}while(0)

#define SXE_BUG_ON_NO_SYNC(cond) do {	\
	if((cond)) { \
		printk(KERN_ERR "BUG_ON's condition(%s) has been triggered\n", #cond); \
		LOG_ERROR("BUG_ON's condition(%s) has been triggered\n", #cond); \
	}									\
	BUG_ON(cond);								\
}while(0)

#define SXE_BUG_NO_SYNC(void) do {	\
	BUG(void);		\
}while(0)

#endif

#ifdef __cplusplus
}
#endif
#endif 

