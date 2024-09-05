#include <linux/moduleparam.h>
#include <linux/kthread.h>
#include <linux/pagemap.h>
#include <linux/fsnotify.h>
#include <linux/rtc.h>
#include <linux/vmalloc.h>
#include "sxe_log.h"
#include "sxe_compat.h"

#if (defined SXE_DRIVER_DEBUG && defined __KERNEL__) || (defined SXE_DRIVER_TRACE)

int time_for_file_name(char *buff, int buf_len)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	struct timeval tv;
	struct tm td;

	do_gettimeofday(&tv);
	time_to_tm(tv.tv_sec, -sys_tz.tz_minuteswest*60, &td);
#else
	struct timespec64 tv;
	struct tm td;
	ktime_get_real_ts64(&tv);
	time64_to_tm(tv.tv_sec, -sys_tz.tz_minuteswest*60, &td);
#endif
	return snprintf(buff, buf_len, "%04ld-%02d-%02d_%02d:%02d:%02d",
		td.tm_year + 1900, td.tm_mon + 1, td.tm_mday,
		td.tm_hour, td.tm_min, td.tm_sec);
}

int sxe_file_write(struct file *file, char *buf, int len)
{
	int ret = 0;

	void *journal;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
	mm_segment_t old_fs;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
	old_fs = get_fs();
	set_fs(get_ds());
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
#else
	old_fs = force_uaccess_begin();
#endif

	journal = current->journal_info;
	current->journal_info = NULL;

	if (!file){
		return 0;
	}

	do{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,10,0)
		ret = file->f_op->write(file, buf, len, &file->f_pos);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
		ret = vfs_write(file, buf, len, &file->f_pos);
#else
		ret = kernel_write(file, buf, len, &file->f_pos);
#endif
	}while( ret == -EINTR );

	if (ret >= 0) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
		fsnotify_modify(file);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
		if ( file->f_path.dentry) {
			fsnotify_modify(file->f_path.dentry);
		}
#endif
	}

	current->journal_info = journal;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
	set_fs(old_fs);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
#else
	force_uaccess_end(old_fs);
#endif

	return ret;
}
#endif 

#if defined SXE_DRIVER_DEBUG && defined __KERNEL__

#define FILE_NAME_SIZE 128
#define SXE_KLOG_OUT_WAIT (5 * HZ)
#define SWITCH_FILE
#define LOG_PATH_LEN 100
#define DRV_LOG_FILE_SIZE_MIN_MB 10
#define DRV_LOG_FILE_SIZE_MAX_MB 200

sxe_debug_t g_sxe_debug;
char g_log_path_str[LOG_PATH_LEN] = {0};
char g_log_path_bin[LOG_PATH_LEN] = {0};

static char g_log_path[80] = {0};
module_param_string(g_log_path, g_log_path, 80, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_log_path, "the path host driver will be saved(<80 chars) Default: /var/log");

static u32 g_log_file_size = 200;
module_param(g_log_file_size, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_log_file_size,
	"single driver log file size(10MB ~ 200MB), Default: 200, Unit: MB");

static u32 g_log_space_size = 0;
module_param(g_log_space_size, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_log_space_size,
	"the space allowed host driver log to be store, Default: 0(unlimited), Unit: MB");

static u32 g_log_tty = 0;
module_param(g_log_tty, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_log_tty,
	"allow driver log(ERROR, WARN, INFO) output to tty console, Default: 0(not allowed)");

static inline int time_for_log(char *buff, int buf_len)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	struct timeval tv;
	struct tm td;

	do_gettimeofday(&tv);
	time_to_tm(tv.tv_sec, -sys_tz.tz_minuteswest*60, &td);

	return snprintf(buff, buf_len, "[%04ld-%02d-%02d;%02d:%02d:%02d.%ld]",
			td.tm_year + 1900,
			td.tm_mon + 1, td.tm_mday, td.tm_hour,
			td.tm_min, td.tm_sec, tv.tv_usec);
#else
	struct timespec64 tv;
	struct tm td;
	ktime_get_real_ts64(&tv);
	time64_to_tm(tv.tv_sec, -sys_tz.tz_minuteswest*60, &td);
	return snprintf(buff, buf_len, "[%04ld-%02d-%02d;%02d:%02d:%02d.%ld]",
			td.tm_year + 1900,
			td.tm_mon + 1, td.tm_mday, td.tm_hour,
			td.tm_min, td.tm_sec, tv.tv_nsec*1000);
#endif
}

static inline char *sxe_stack_top(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
	ULong *ptr = (ULong*)(current->thread_info + 1);
#else
	ULong *ptr = (ULong*)(task_thread_info(current) + 1 );
#endif
	return (char*)(ptr + 1); 
}

static inline sxe_thread_local_t *sxe_thread_local_get(sxe_thread_key_t * key)
{
	return (sxe_thread_local_t*)(sxe_stack_top() + key->offset);
}

void sxe_thread_key_create(int size, sxe_thread_key_t *key)
{
	key->offset = g_sxe_debug.key_offset;
	g_sxe_debug.key_offset += sizeof(sxe_thread_local_t) + size;
}

void *sxe_thread_get_specific(sxe_thread_key_t *key)
{
	sxe_thread_local_t *local = sxe_thread_local_get(key);
	if (local->magic != DEBUG_TRACE_MAGIC)
	{
		return NULL;
	}
	return (void*)local->data;
}

void sxe_thread_clear_specific(sxe_thread_key_t *key)
{
	sxe_thread_local_t *local = sxe_thread_local_get(key);
	local->magic = 0;
}

int sxe_filter_file_add(char *name)
{
	debug_file_t *file = NULL;

	file = (debug_file_t*)kmalloc(sizeof(debug_file_t), GFP_ATOMIC);
	if (!file){
		sxe_print(KERN_ERR, NULL, "kmalloc size %lu failed\n", PAGE_SIZE);
		return -ENOMEM;
	}
	strncpy(file->name, name, sizeof(file->name));
	INIT_LIST_HEAD(&file->list);

	list_add_rcu(&file->list, &g_sxe_debug.filter_file);
	return 0;
}

void sxe_filter_file_del(char *filename)
{
	debug_file_t *file = NULL;

	list_for_each_entry_rcu(file, &g_sxe_debug.filter_file, list){
		if(!strcmp(file->name, filename)){
			list_del_rcu(&file->list);
			synchronize_rcu();
			kfree(file);
			return;
		}
	}
	return;
}

void sxe_log_level_modify(u32 level)
{
    sxe_level_set(level);
}

char* sxe_log_path_query(void)
{
#ifndef __cplusplus
    return g_log_path;
#else
    return NULL;
#endif
}

u32 sxe_log_space_size_query(void)
{
    return g_log_space_size;
}

u32 sxe_log_file_size_query(void)
{
    return g_log_file_size;
}

void sxe_log_file_size_modify(u32 size)
{
    g_log_file_size = size;
}

u32 sxe_log_tty_query(void)
{
    return g_log_tty;
}

#ifndef SXE_CFG_RELEASE
static inline int sxe_filter_file_print(const char *filename)
{
	debug_file_t *file;
	rcu_read_lock();
	list_for_each_entry_rcu(file, &g_sxe_debug.filter_file, list){
		if(!strcmp(file->name, filename)){
			rcu_read_unlock();
			return 1;
		}
	}
	rcu_read_unlock();
	return 0;
}

static inline int sxe_filter_func_print(const char *name)
{
	debug_func_t *func;

	rcu_read_lock();
	list_for_each_entry_rcu(func, &g_sxe_debug.filter_func, list){
		if(!strcmp(func->name, name)){
			rcu_read_unlock();
			return 1;
		}
	}
	rcu_read_unlock();
	return 0;
}

#endif
void sxe_filter_file_clear(void)
{
	debug_file_t *file = NULL;

	do{
		file = list_first_or_null_rcu(
					&g_sxe_debug.filter_file,
					debug_file_t,
					list);
		if (file){
			list_del_rcu(&file->list);
			synchronize_rcu();
			kfree(file);
		}
	}while(file);

	return;
}

int sxe_filter_func_add(char *name)
{
	debug_func_t *func = NULL;

	func = (debug_func_t *)kmalloc(sizeof(debug_func_t), GFP_ATOMIC);
	if (!func){
		sxe_print(KERN_ERR,NULL, "kmalloc size %lu failed\n", PAGE_SIZE);
		return -ENOMEM;
	}
	strncpy(func->name, name, sizeof(func->name));
	INIT_LIST_HEAD(&func->list);

	list_add_rcu(&func->list, &g_sxe_debug.filter_func);
	return 0;
}

void sxe_filter_func_del(char *name)
{
	debug_func_t *func = NULL;

	list_for_each_entry_rcu(func, &g_sxe_debug.filter_func, list){
		if(!strcmp(func->name, name)){
			list_del_rcu(&func->list);
			synchronize_rcu();
			kfree(func);
			return;
		}
	}
	return;
}

void sxe_filter_func_clear(void)
{
	debug_func_t *func = NULL;

	do{
		func = list_first_or_null_rcu(
					&g_sxe_debug.filter_func,
					debug_func_t,
					list);
		if (func){
			list_del_rcu(&func->list);
			synchronize_rcu();
			kfree(func);
		}
	}while(func);

	return;
}

static void sxe_file_close(struct file **file)
{
	filp_close(*file, NULL);
	*file = NULL;
}

static int sxe_file_open(sxe_log_t *log, struct file **pp_file)
{
	struct file *file;
	int flags_new     = O_CREAT | O_RDWR | O_APPEND | O_LARGEFILE;
	int flags_rewrite = O_CREAT | O_RDWR | O_LARGEFILE | O_TRUNC;
	int err = 0;
	int len = 0;
	char filename[FILE_NAME_SIZE];

#ifdef SWITCH_FILE
	memset(filename, 0, FILE_NAME_SIZE);
	len += snprintf(filename, PAGE_SIZE, "%s", log->file_path);
	if (log->file_num == 0) {
		time_for_file_name(filename + len, FILE_NAME_SIZE - len);
	} else {
		snprintf(filename + len, FILE_NAME_SIZE - len, "%04d", log->index++);
		log->index = log->index % log->file_num;
	}

	if(log->file_num == 1 && log->file != NULL) {
		sxe_file_close(&log->file);
		log->file_pos = 0;
	}
#else
	memset(filename, 0, FILE_NAME_SIZE);
	strncpy(filename, path, FILE_NAME_SIZE);
#endif
	if (log->file_num == 0) {
		file = filp_open(filename, flags_new, 0666);
	} else {
		file = filp_open(filename, flags_rewrite, 0666);
		if (IS_ERR(file)) {
			err = (int)PTR_ERR(file);
			if (err == -ENOENT) {
				file = filp_open(filename, flags_new, 0666);
			}
		}
	}
	if (IS_ERR(file)){
		err = (int)PTR_ERR(file);
		sxe_print(KERN_ERR, NULL, "open file:%s failed[errno:%d]\n", filename, err);
		goto l_out;
	}
	mapping_set_gfp_mask(file->f_path.dentry->d_inode->i_mapping, GFP_NOFS);

	sxe_print(,NULL,"redirect file %s\n", filename);

	*pp_file = file;

l_out:
	return err;
}

static void sxe_file_sync(struct file *file)
{
	struct address_space *mapping;
	void				 *journal;
	int ret = 0;
	int err;

	(void)ret;
	(void)err;

	if( !file || !file->f_op || !file->f_op->fsync ){
		goto l_end;
	}

	journal				  = current->journal_info;
	current->journal_info = NULL;

	mapping = file->f_mapping;

	ret = filemap_fdatawrite(mapping);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
	mutex_lock(&mapping->host->i_mutex);
	err = file->f_op->fsync(file, file->f_path.dentry, 1);
	if( !ret ){
		ret = err;
	}
	mutex_unlock(&mapping->host->i_mutex);
	err = filemap_fdatawait(mapping);
	if( !ret ){
		ret = err;
	}

#else
	err = file->f_op->fsync(file, 0, file->f_mapping->host->i_size, 1);
#endif

	current->journal_info = journal;

l_end:
	return;
}

static void sxe_klog_in(sxe_log_t *log, char *buf, const int len)
{
	int begin = 0;
	int end   = 0;
	int free_size;
	ULong flags;

	spin_lock_irqsave(&log->lock, flags);

	if (log->head > log->tail) {
		sxe_print(KERN_WARNING, NULL, "FAILURE: log head exceeds log tail\n");
		SXE_BUG_NO_SYNC();
	}

	free_size = log->buf_size - (log->tail - log->head);

	if (free_size <= len){
		log->is_drop = 1;
		spin_unlock_irqrestore(&log->lock, flags);
		return;
	}

	begin = log->tail % log->buf_size;
	end   = (log->tail + len) % log->buf_size;

	if (begin < end){
		memcpy(log->buf + begin, buf, len);
	}
	else{
		memcpy(log->buf + begin, buf, log->buf_size - begin);
		memcpy(log->buf, buf + log->buf_size - begin, end);
	}

	log->tail = log->tail + len;

	spin_unlock_irqrestore(&log->lock, flags);

	return;
}

static void sxe_klog_out(sxe_log_t *log)
{
	int len = 0;
	int rc = 0;
	long long tail;
	int begin;
	int end;
	int schedule_count_th = 0;
	const int max_loop = 4096;

#ifdef SWITCH_FILE
	struct file *file = NULL;
#endif

	if (log->file == NULL) {
		rc = sxe_file_open(log, &log->file);
		if (log->file != NULL) {
			log->file_pos = 0;
		} else {
			return;
		}
	}

	do {
		tail  = log->tail;
		begin = log->head % log->buf_size;
		end   = tail % log->buf_size;
		len = 0;
		rc = 0;

		schedule_count_th++;
		if ((schedule_count_th >= max_loop)) {
			schedule_count_th = 0;
			schedule_timeout_interruptible(SXE_KLOG_OUT_WAIT);
		}

		if (log->is_drop) {
			rc = sxe_file_write(
					log->file,
					DEBUG_DROP_LOG_STRING,
					strlen(DEBUG_DROP_LOG_STRING));
			if (rc < 0) {
				break;
			}
			log->is_drop = 0;
		}

		if (begin < end) {
			rc = sxe_file_write(
					log->file,
					log->buf + begin,
					end - begin);
			if (rc > 0) {
				len += rc;
			}
		} else if(begin > end) {
			rc = sxe_file_write(
					log->file,
					log->buf + begin,
					log->buf_size - begin);
			if (rc > 0) {
				len += rc;
				rc = sxe_file_write(log->file, log->buf, end);
				if (rc > 0) {
					len += rc;
				}
			}
		}
		log->head += len;
		log->file_pos += len;

		LOG_BUG_ON(log->head > log->tail, "FAILURE: log head exceeds log tail\n");
	}while (log->head != log->tail && rc > 0);

	if (rc < 0) {
		sxe_print(KERN_ERR,  NULL, "write file %s error %d\n", log->file_path, rc);
		return ;
	}

#ifdef SWITCH_FILE
	if (log->file_pos >= log->file_size) {
		rc = sxe_file_open(log, &file);
		if (rc >= 0 && log->file != NULL && log->file_num != 1) {
			sxe_file_close(&log->file);
			log->file = file;
			log->file_pos = 0;
		}
	}
#endif
	return ;
}

static int sxe_klog_flush(void *arg)
{
	int i;

	while (!kthread_should_stop()){
		schedule_timeout_interruptible(SXE_KLOG_OUT_WAIT);

		for (i = 0; i < ARRAY_SIZE(g_sxe_debug.log); i++){
			sxe_klog_out(&g_sxe_debug.log[i]);
		}
	}
	return 0;
}

static int sxe_klog_init(
			sxe_log_t *log,
			long long buf_size,
			char	 *file_path,
			long long file_size,
			u32        file_num)
{
	int rc = 0;

	memset(log, 0, sizeof(*log));
	spin_lock_init(&log->lock);

	log->buf = (char*)vmalloc(buf_size+PER_CPU_PAGE_SIZE);
	if (!log->buf){
		rc = -ENOMEM;
		goto l_end;
	}

	log->file = NULL;
	log->head = 0;
	log->tail = 0;
	log->buf_size  = buf_size;

	log->file_path = file_path;
	log->file_pos  = 0;
	log->file_size = file_size;
	log->file_num  = file_num;
	log->index     = 0;
l_end:
	return rc;
}

static void sxe_klog_exit(sxe_log_t *log)
{
	if (log->buf) {
		vfree(log->buf);
	}
	if (log->file) {
		sxe_file_close(&log->file);
	}
}

static inline char *sxe_file_name_locale(char *file)
{
	char *p_slash = strrchr(file, '/');
	return (p_slash == NULL)?file:(p_slash+1);
}

void sxe_level_set(int level)
{
	g_sxe_debug.level = level;
}

s32 sxe_level_get(void)
{
	return (s32)g_sxe_debug.level;
}

void sxe_bin_status_set(bool status)
{
	g_sxe_debug.status = status;
}

s32 sxe_bin_status_get(void)
{
	return (s32)g_sxe_debug.status;
}

void sxe_log_string(
		debug_level_e level,
		const char *dev_name,
		const char *file,
		const char *func,
		int line,
		const char *fmt,...)
{
	sxe_ctxt_t *ctxt = NULL;
	char *buf = NULL;
	int len = 0;
	ULong flags = 0;
	const char *name = dev_name ? dev_name : "";

	va_list args;

	if (level > g_sxe_debug.level){
#ifndef SXE_CFG_RELEASE
		if (!sxe_filter_file_print(file)
				&& !sxe_filter_func_print(func)){
			return;
		}
#else
		return;
#endif
	}

	if (!in_interrupt()){
		local_irq_save(flags);
	}

	ctxt = per_cpu_ptr(g_sxe_debug.ctxt, get_cpu());
	put_cpu();

	buf  = ctxt->buff;

	len  = snprintf(buf, PAGE_SIZE, "%s", sxe_debug_level_name(level));
	len += time_for_log(buf+len, PAGE_SIZE - len);
	len += snprintf(buf+len, PAGE_SIZE - len, "[%d][%d][%s]%s:%4d:%s:",
		raw_smp_processor_id(), current->pid,
		name,
		sxe_file_name_locale((char*)file), line, func);

	va_start(args, fmt);
	len += vsnprintf(
			buf + len,
			PAGE_SIZE - len,
			fmt,
			args);
	va_end(args);


	if (!in_interrupt()){
		local_irq_restore(flags);
	}

	if (sxe_log_tty_query()) {
		if (buf[0] == 'I' || buf[0] == 'W') {
			printk_ratelimited(KERN_WARNING"%s", buf + LOG_INFO_PREFIX_LEN);
		} else if (buf[0] == 'E') {
			printk_ratelimited(KERN_WARNING"%s", buf + LOG_ERROR_PREFIX_LEN);
		}
	}
	sxe_klog_in(&g_sxe_debug.log[DEBUG_TYPE_STRING], buf, len);

	wake_up_process(g_sxe_debug.task);

	return;
}

void sxe_log_binary(
		const char *file,
		const char *func,
		int line,
		u8 *ptr,
		u64 addr,
		u32 size,
		char *str)
{
#define LINE_TOTAL 16 
	sxe_ctxt_t *ctxt = NULL;
	char *buf = NULL;
	int len = 0;
	ULong flags = 0;
	u32 i = 0;
	u32 j = 0;
	u32 max;
	u32 mod;

	if (sxe_bin_status_get() != true) {
		return;
	}

	max  = size / LINE_TOTAL;
	mod   = size % LINE_TOTAL;

	if (!in_interrupt()){
		local_irq_save(flags);
	}

	ctxt = per_cpu_ptr(g_sxe_debug.ctxt, get_cpu());
	put_cpu();

	buf  = ctxt->buff;

	len += time_for_log(buf+len, PER_CPU_PAGE_SIZE - len);
	len += snprintf(buf+len, PER_CPU_PAGE_SIZE - len,
		"[%d] %s %s():%d %s size:%d\n",
		current->pid, sxe_file_name_locale((char*)file), func,
		line, str, size);

	for (i = 0; i < max; i++) {
		j = i * LINE_TOTAL;

		len += snprintf(buf + len, PER_CPU_PAGE_SIZE - len,
				"0x%llx 0x%llx: ",
				addr, (u64)&ptr[j]);

		for (; j < (i + 1) * LINE_TOTAL; j++) {
			len += snprintf(buf + len, PER_CPU_PAGE_SIZE - len,
					"0x%02x%c ", ptr[j], ',');
		}
		len += snprintf(buf + len, PER_CPU_PAGE_SIZE -len, "%c", '\n');
	}

	if (mod) {
		len += snprintf(buf + len, PER_CPU_PAGE_SIZE - len,
				"0x%llx  0x%llx: ",
				addr, (u64)&ptr[j]);

		for (; j < size; j++) {
			len += snprintf(buf + len, PER_CPU_PAGE_SIZE - len,
					"0x%02x%c ", ptr[j], ',');
		}

		len += snprintf(buf + len, PER_CPU_PAGE_SIZE -len, "%c", '\n');
	}

	if (!in_interrupt()) {
		local_irq_restore(flags);
	}

	sxe_klog_in(&g_sxe_debug.log[DEBUG_TYPE_BINARY], buf, len);

	wake_up_process(g_sxe_debug.task);

	return;
}

void sxe_log_sync(void)
{
	sxe_file_sync(g_sxe_debug.log[DEBUG_TYPE_STRING].file);
	sxe_file_sync(g_sxe_debug.log[DEBUG_TYPE_BINARY].file);
}

static void sxe_log_file_prefix_add(bool is_vf, char *log_path_p)
{
	if (is_vf) {
		snprintf(g_log_path_str, LOG_PATH_LEN, "%s%s.",  log_path_p, VF_LOG_FILE_PREFIX);
		snprintf(g_log_path_bin, LOG_PATH_LEN, "%s%s.",  log_path_p, VF_BINARY_FILE_PREFIX);
	} else {
		snprintf(g_log_path_str, LOG_PATH_LEN, "%s%s.",  log_path_p, LOG_FILE_PREFIX);
		snprintf(g_log_path_bin, LOG_PATH_LEN, "%s%s.",  log_path_p, BINARY_FILE_PREFIX);
	}

	return;
}

static void sxe_log_file_prefix_add_default(bool is_vf, char *log_path_p)
{
	if (is_vf) {
		snprintf(g_log_path_str, LOG_PATH_LEN, "%s/%s.", log_path_p, VF_LOG_FILE_PREFIX);
		snprintf(g_log_path_bin, LOG_PATH_LEN, "%s/%s.", log_path_p, VF_BINARY_FILE_PREFIX);
	} else {
		snprintf(g_log_path_str, LOG_PATH_LEN, "%s/%s.", log_path_p, LOG_FILE_PREFIX);
		snprintf(g_log_path_bin, LOG_PATH_LEN, "%s/%s.", log_path_p, BINARY_FILE_PREFIX);
	}

	return;
}

static void sxe_log_file_path_set(bool is_vf)
{
	if (is_vf) {
		snprintf(g_log_path_str, LOG_PATH_LEN, "%s.", VF_LOG_FILE_PATH);
		snprintf(g_log_path_bin, LOG_PATH_LEN, "%s.", VF_BINARY_FILE_PATH);
	} else {
		snprintf(g_log_path_str, LOG_PATH_LEN, "%s.", LOG_FILE_PATH);
		snprintf(g_log_path_bin, LOG_PATH_LEN, "%s.", BINARY_FILE_PATH);
	}

	return;
}

int sxe_log_init(bool is_vf)
{
	struct task_struct *task = NULL;
	sxe_ctxt_t *ctxt = NULL;
	int rc = 0;
	int i;
	int nid;
	u32		file_num = 0;
	u32		log_path_len = 0;
	u32		input_log_space = sxe_log_space_size_query();
	u32		input_log_file_size = sxe_log_file_size_query();
	unsigned int	log_file_size = 0;
	char		*log_path_p = NULL;
	sxe_log_t 	*log_bin = &g_sxe_debug.log[DEBUG_TYPE_BINARY];
	sxe_log_t 	*log_str = &g_sxe_debug.log[DEBUG_TYPE_STRING];

	INIT_LIST_HEAD(&g_sxe_debug.filter_file);
	INIT_LIST_HEAD(&g_sxe_debug.filter_func);

#ifdef SXE_CFG_RELEASE
	g_sxe_debug.level = LEVEL_INFO;
	g_sxe_debug.status = false;
#else
	g_sxe_debug.level = LEVEL_DEBUG;
	g_sxe_debug.status = true;
#endif

	g_sxe_debug.ctxt = alloc_percpu(sxe_ctxt_t);
	if (!g_sxe_debug.ctxt) {
		rc = -ENOMEM;
		sxe_print(KERN_ERR, NULL, "alloc percpu failed\n");
		goto l_end;
	}

	for_each_possible_cpu(i) {
		ctxt = per_cpu_ptr(g_sxe_debug.ctxt, i);
		memset(ctxt, 0, sizeof(*ctxt));
	}

	for_each_possible_cpu(i) {
		ctxt = per_cpu_ptr(g_sxe_debug.ctxt, i);
		nid  = cpu_to_node(i);

		ctxt->page = alloc_pages_node(nid, GFP_ATOMIC, PAGE_ORDER);
		if (!ctxt->page) {
			rc = -ENOMEM;
			sxe_print(KERN_ERR,  NULL, "kmalloc size %lu failed\n",
				  PER_CPU_PAGE_SIZE);
			goto l_free_cpu_buff;
		}
		ctxt->buff = page_address(ctxt->page);
	}

	log_path_p = sxe_log_path_query();
	log_path_len = strlen(log_path_p);
	if (log_path_p != NULL && log_path_p[0] == '/') {
		if (log_path_p[log_path_len] == '/') {
			sxe_log_file_prefix_add(is_vf, log_path_p);
		} else {
			sxe_log_file_prefix_add_default(is_vf, log_path_p);
		}
	} else {
		sxe_log_file_path_set(is_vf);
	}
	if (input_log_file_size < DRV_LOG_FILE_SIZE_MIN_MB ||
			input_log_file_size > DRV_LOG_FILE_SIZE_MAX_MB) {
		sxe_log_file_size_modify(LOG_FILE_SIZE >> MEGABYTE);
		input_log_file_size = LOG_FILE_SIZE >> MEGABYTE;
	}
	if (input_log_space && input_log_space < input_log_file_size) {
		sxe_log_file_size_modify(input_log_space);
		input_log_file_size = input_log_space;
	}
	log_file_size = input_log_file_size << MEGABYTE;

	if (input_log_space) {
		file_num = input_log_space / input_log_file_size;
		if (file_num == 0) {
			sxe_print(KERN_ERR,  NULL, "filenum shouldnot be 0\n");
			SXE_BUG();
		}
	} else {
		file_num = 0;
	}

	rc = sxe_klog_init(
			log_str,
			BUF_SIZE,
			g_log_path_str,
			log_file_size,
			file_num);
	if (rc < 0) {
		goto l_free_cpu_buff;
	}

	rc = sxe_klog_init(
			log_bin,
			BUF_SIZE,
			g_log_path_bin,
			BINARY_FILE_SIZE,
			0);
	if (rc < 0) {
		goto l_free_string;
	}

	task = kthread_create(sxe_klog_flush, NULL, "sxe_klog_flush");
	if (IS_ERR(task)) {
		rc = (int)PTR_ERR(task);
		sxe_print(KERN_ERR, NULL, "Create kernel thread, err: %d\n", rc);
		goto l_free_binary;
	}
	wake_up_process(task);
	g_sxe_debug.task = task;
	rc = 0;
	sxe_print(KERN_INFO, NULL, "sxe debug init logpath[%s] strlogsize[%dM] filenum[%d]\n",
		g_log_path_str, (log_file_size >> MEGABYTE), log_str->file_num);
l_end:
	return rc;

l_free_binary:
	sxe_klog_exit(&g_sxe_debug.log[DEBUG_TYPE_BINARY]);

l_free_string:
	sxe_klog_exit(&g_sxe_debug.log[DEBUG_TYPE_STRING]);

l_free_cpu_buff:
	for_each_possible_cpu(i) {
		ctxt = per_cpu_ptr(g_sxe_debug.ctxt, i);
		if (ctxt && ctxt->page) {
			__free_page(ctxt->page);
		}
	}
	free_percpu(g_sxe_debug.ctxt);
	goto l_end;
}

void sxe_log_exit(void)
{
	int i = 0;
	sxe_ctxt_t *ctxt;

	if (g_sxe_debug.task == NULL) {
		return;
	}

	kthread_stop(g_sxe_debug.task);

	for (i = 0; i < ARRAY_SIZE(g_sxe_debug.log); i++) {
		sxe_klog_exit(&g_sxe_debug.log[i]);
	}

	if (g_sxe_debug.ctxt) {
		for_each_possible_cpu(i) {
			ctxt = per_cpu_ptr(g_sxe_debug.ctxt, i);
			if (ctxt && ctxt->page) {
				__free_page(ctxt->page);
			}
		}

		free_percpu(g_sxe_debug.ctxt);
		g_sxe_debug.ctxt = NULL;
	}
}

#elif !defined SXE_DRIVER_RELEASE 

s32 g_sxe_log_level = LEVEL_INFO;
s32 g_sxe_bin_status = false;
char *test_bin_buf = NULL;

s32 sxe_log_init(bool is_vf)
{
	return 0;
}

void sxe_level_set(s32 level)
{
	g_sxe_log_level = level;
}

s32 sxe_level_get(void)
{
	return g_sxe_log_level;
}

void sxe_bin_status_set(bool status)
{
	g_sxe_bin_status = status;
}

s32 sxe_bin_status_get(void)
{
	return g_sxe_bin_status;
}

void sxe_log_sync(void)
{
}

void sxe_log_exit(void)
{
	if (test_bin_buf != NULL) {
		free(test_bin_buf);
	}
}

void sxe_log_binary(
		const char *file,
		const char *func,
		int line,
		u8 *ptr,
		u64 addr,
		u32 size,
		char *str)
{
#define LINE_TOTAL 16 
	u32 i = 0;
	u32 j = 0;
	u32 iMax;
	u32 mod;
	char *buf = NULL;
	int len = 0;

	if (sxe_bin_status_get() != true) {
		return;
	}

	buf = zalloc(PER_CPU_PAGE_SIZE);
	test_bin_buf = buf;

	iMax  = size / LINE_TOTAL;
	mod   = size % LINE_TOTAL;

	len += snprintf(buf+len, PER_CPU_PAGE_SIZE - len,
		"%s size:%d\n", str, size);

	for (i = 0; i < iMax; i++) {
		j = i * LINE_TOTAL;

		len += snprintf(buf + len, PER_CPU_PAGE_SIZE - len,
				"0x%llx 0x%llx: ",
				addr, (u64)&ptr[j]);

		for (; j < (i + 1) * LINE_TOTAL; j++) {
			len += snprintf(buf + len, PER_CPU_PAGE_SIZE - len,
					"0x%02x%c ", ptr[j], ',');
		}
		len += snprintf(buf + len, PER_CPU_PAGE_SIZE -len, "%c", '\n');
	}

	if (mod) {
		len += snprintf(buf + len, PER_CPU_PAGE_SIZE - len,
				"0x%llx  0x%llx: ",
				addr, (u64)&ptr[j]);

		for (; j < size; j++) {
			len += snprintf(buf + len, PER_CPU_PAGE_SIZE - len,
					"0x%02x%c ", ptr[j], ',');
		}

		len += snprintf(buf + len, PER_CPU_PAGE_SIZE -len, "%c", '\n');
	}

	printf("buf:%s", buf);

	return;
}

#endif 

