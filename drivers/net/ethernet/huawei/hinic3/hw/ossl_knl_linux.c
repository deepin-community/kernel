// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#include <linux/vmalloc.h>
#include "ossl_knl_linux.h"

#define OSSL_MINUTE_BASE (60)

struct file *file_creat(const char *file_name)
{
	return filp_open(file_name, O_CREAT | O_RDWR | O_APPEND, 0);
}

struct file *file_open(const char *file_name)
{
	return filp_open(file_name, O_RDONLY, 0);
}

void file_close(struct file *file_handle)
{
	(void)filp_close(file_handle, NULL);
}

u32 get_file_size(struct file *file_handle)
{
	struct inode *file_inode = NULL;

	file_inode = file_handle->f_inode;

	return (u32)(file_inode->i_size);
}

void set_file_position(struct file *file_handle, u32 position)
{
	file_handle->f_pos = position;
}

int file_read(struct file *file_handle, char *log_buffer, u32 rd_length,
	      u32 *file_pos)
{
	return (int)kernel_read(file_handle, log_buffer, rd_length,
				&file_handle->f_pos);
}

u32 file_write(struct file *file_handle, const char *log_buffer, u32 wr_length)
{
	return (u32)kernel_write(file_handle, log_buffer, wr_length,
				 &file_handle->f_pos);
}

static int _linux_thread_func(void *thread)
{
	struct sdk_thread_info *info = (struct sdk_thread_info *)thread;

	while (!kthread_should_stop())
		info->thread_fn(info->data);

	return 0;
}

int creat_thread(struct sdk_thread_info *thread_info)
{
	thread_info->thread_obj = kthread_run(_linux_thread_func, thread_info,
					      thread_info->name);
	if (!thread_info->thread_obj)
		return -EFAULT;

	return 0;
}

void stop_thread(struct sdk_thread_info *thread_info)
{
	if (thread_info->thread_obj)
		(void)kthread_stop(thread_info->thread_obj);
}

void utctime_to_localtime(u64 utctime, u64 *localtime)
{
	*localtime = utctime - (u64)(sys_tz.tz_minuteswest * OSSL_MINUTE_BASE); /*lint !e647 !e571*/
}

#ifndef HAVE_TIMER_SETUP
void initialize_timer(const void *adapter_hdl, struct timer_list *timer)
{
	if (!adapter_hdl || !timer)
		return;

	init_timer(timer);
}
#endif

void add_to_timer(struct timer_list *timer, u64 period)
{
	if (!timer)
		return;

	add_timer(timer);
}

void stop_timer(struct timer_list *timer) {}

void delete_timer(struct timer_list *timer)
{
	if (!timer)
		return;

	del_timer_sync(timer);
}

u64 ossl_get_real_time(void)
{
	struct timeval tv = {0};
	u64 tv_msec;

	do_gettimeofday(&tv);

	tv_msec = (u64)tv.tv_sec * MSEC_PER_SEC + (u64)tv.tv_usec / USEC_PER_MSEC;
	return tv_msec;
}

