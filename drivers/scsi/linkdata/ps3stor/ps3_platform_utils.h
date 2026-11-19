/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_PLATFORM_UTILS_H_
#define _PS3_PLATFORM_UTILS_H_

#ifdef _WINDOWS
#include "ps3_def.h"
#else
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include "linux/kernel.h"
#include <linux/version.h>
#include <linux/workqueue.h>
#endif

#include "ps3_err_def.h"

struct ps3_instance;
struct scsi_device;
struct ps3_cmd;

#define scsi_cmnd_cdb(scmnd) ((scmnd)->cmnd)
#define scsi_device_private_data(scmnd) (PS3_SDEV_PRI_DATA((scmnd)->device))
#ifdef _WINDOWS
#define scsi_host_data(scmnd) ((struct ps3_instance *)((scmnd)->instance))
#else
#define scsi_host_data(scmnd)                                                  \
	((struct ps3_instance *)((scmnd)->device->host->hostdata))
#endif

#ifdef _WINDOWS
#define ps3_container_of(ptr, type, member)                                    \
	((type *)((char *)ptr - offsetof(type, member)))
#else
#define ps3_container_of container_of
#endif
#define MAX_MDELAY (1)
#ifndef PS3_FALSE
#define PS3_FALSE (0)
#endif
#ifndef PS3_TRUE
#define PS3_TRUE (1)
#endif
#ifndef PS3_MAX
#define PS3_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef PS3_MIN
#define PS3_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef PS3_DESC
#define PS3_DESC(a) 1
#endif

static inline unsigned char ps3_cancel_work_sync(struct work_struct *work)
{
#if defined(PS3_WORK_FUNC_CHECK)
	if (work->func != NULL)
		return cancel_work_sync(work);
	return PS3_FALSE;
#else
	return cancel_work_sync(work);
#endif
}

static inline void ps3_mutex_init(struct mutex *mutex_lock)
{
#ifdef _WINDOWS
	ExInitializeFastMutex(&mutex_lock->mutex);
#else
	mutex_init(mutex_lock);
#endif
}

static inline void ps3_mutex_destroy(struct mutex *mutex_lock)
{
#ifdef _WINDOWS
	(void)mutex_lock;
#else
	mutex_destroy(mutex_lock);
#endif
}

static inline int ps3_mutex_lock(struct mutex *mtx_lock)
{
#ifdef _WINDOWS
	if (KeGetCurrentIrql() <= APC_LEVEL) {
		ExAcquireFastMutex(&mtx_lock->mutex);

		return PS3_SUCCESS;
	}
	return -PS3_FAILED;
#else
	mutex_lock(mtx_lock);
	return PS3_SUCCESS;
#endif
}
static inline int ps3_mutex_trylock(struct mutex *mutex_lock)
{
	int ret = PS3_SUCCESS;
#ifdef _WINDOWS
	if (KeGetCurrentIrql() > APC_LEVEL) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	if (!ExTryToAcquireFastMutex(&mutex_lock->mutex)) {
		ret = -PS3_FAILED;
		goto l_out;
	}
l_out:
#else
	ret = mutex_trylock(mutex_lock);
#endif
	return ret;
}

static inline int ps3_mutex_unlock(struct mutex *mutex_lock)
{
#ifdef _WINDOWS
	if (KeGetCurrentIrql() <= APC_LEVEL) {
		ExReleaseFastMutex(&mutex_lock->mutex);

		return PS3_SUCCESS;
	}

	return -PS3_FAILED;
#else
	mutex_unlock(mutex_lock);
	return PS3_SUCCESS;

#endif
}

static inline int ps3_atomic_read(atomic_t *value)
{
#ifdef _WINDOWS
	return value->value;
#else
	return atomic_read(value);
#endif
}

static inline int ps3_atomic_dec(atomic_t *value)
{
#ifdef _WINDOWS
	return (int)InterlockedDecrement((LONG *)(&value->value));
#else
	atomic_dec(value);
	return PS3_SUCCESS;
#endif
}

static inline int ps3_atomic_add(int i, atomic_t *value)
{
#ifdef _WINDOWS
	return (int)_InlineInterlockedAdd((LONG *)&value->value, (LONG)i);
#else
	atomic_add(i, value);
	return PS3_SUCCESS;
#endif
}

static inline int ps3_atomic_sub(int i, atomic_t *value)
{
#ifdef _WINDOWS
	return (int)_InlineInterlockedAdd((LONG *)&value->value, (LONG)-i);
#else
	atomic_sub(i, value);
	return PS3_SUCCESS;
#endif
}

static inline int ps3_atomic_cmpxchg(atomic_t *value, int old, int cur)
{
#ifdef _WINDOWS
	return (int)InterlockedCompareExchange((LONG *)&value->value, (LONG)cur,
					       (LONG)old);
#else
	return atomic_cmpxchg(value, cur, old);
#endif
}

static inline unsigned char ps3_atomic_add_unless(atomic_t *value, int a, int u)
{
#ifdef _WINDOWS

	int c = 0;
	int old = 0;

	c = value->value;
	while (c != u && (old = ps3_atomic_cmpxchg(value, c, c + a)) != c)
		c = old;

	return c != u;
#else
	return atomic_add_unless(value, a, u);
#endif
}

static inline int ps3_atomic_inc(atomic_t *value)
{
#ifdef _WINDOWS
	return (int)InterlockedIncrement((LONG *)(&value->value));
#else
	atomic_inc(value);
	return PS3_SUCCESS;
#endif
}

static inline int ps3_atomic_inc_return(atomic_t *value)
{
#ifdef _WINDOWS
	return (int)InterlockedIncrement((LONG *)(&value->value));
#else
	return atomic_inc_return(value);
#endif
}

static inline int ps3_atomic_dec_return(atomic_t *value)
{
#ifdef _WINDOWS
	return (int)InterlockedDecrement((LONG *)(&value->value));
#else
	return atomic_dec_return(value);
#endif
}

static inline long long ps3_atomic64_inc(atomic64_t *value)
{
#ifdef _WINDOWS
	return (long long)InterlockedIncrement64((LONG64 *)(&value->value));
#else
	atomic64_inc(value);
	return PS3_SUCCESS;
#endif
}

static inline long long ps3_atomic64_inc_return(atomic64_t *value)
{
#ifdef _WINDOWS
	return (long long)InterlockedIncrement64((LONG64 *)(&value->value));
#else
	return atomic64_inc_return(value);
#endif
}

static inline long long ps3_atomic64_read(atomic64_t *value)
{
#ifdef _WINDOWS
	return value->value;
#else
	return atomic64_read(value);
#endif
}

static inline void ps3_atomic64_set(atomic64_t *value, long long i)
{
#ifdef _WINDOWS
	value->value = i;
#else
	atomic64_set(value, i);
#endif
}

static inline void ps3_atomic_set(atomic_t *value, int i)
{
#ifdef _WINDOWS
	value->value = i;
#else
	atomic_set(value, i);
#endif
}

static inline long long ps3_atomic64_add(long long i, atomic64_t *value)
{
#ifdef _WINDOWS
	return (long long)_InlineInterlockedAdd64((LONG64 *)&value->value,
						  (LONG64)i);
#else
	atomic64_add(i, value);
	return PS3_SUCCESS;
#endif
}

static inline long long ps3_atomic64_dec(atomic64_t *value)
{
#ifdef _WINDOWS
	return (long long)InterlockedDecrement64((LONG64 *)&value->value);
#else
	atomic64_dec(value);
	return PS3_SUCCESS;
#endif
}

static inline void ps3_spin_lock_init(spinlock_t *lock)
{
#ifdef _WINDOWS
	KeInitializeSpinLock(&lock->lock);
#else
	spin_lock_init(lock);
#endif
}

static inline void ps3_spin_lock(spinlock_t *lock, unsigned long *flag)
{
#ifdef _WINDOWS
	KeAcquireSpinLock(&lock->lock, (PKIRQL)flag);
#else
	(void)flag;
	spin_lock(lock);
#endif
}

static inline void ps3_spin_lock_irqsave(spinlock_t *lock, unsigned long *flag)
{
#ifdef _WINDOWS
	KeAcquireSpinLock(&lock->lock, (PKIRQL)flag);
#else
	spin_lock_irqsave(lock, *flag);
#endif
}

static inline void ps3_spin_unlock(spinlock_t *lock, unsigned long flag)
{
#ifdef _WINDOWS
	KeReleaseSpinLock(&lock->lock, (KIRQL)flag);
#else
	(void)flag;
	spin_unlock(lock);
#endif
}

static inline void ps3_spin_unlock_irqrestore(spinlock_t *lock,
					      unsigned long flag)
{
#ifdef _WINDOWS
	KeReleaseSpinLock(&lock->lock, (KIRQL)flag);
#else
	spin_unlock_irqrestore(lock, flag);
#endif
}

int ps3_wait_for_completion_timeout(void *sync_done, unsigned long Timeout);
int ps3_wait_cmd_for_completion_timeout(struct ps3_instance *instance,
					struct ps3_cmd *cmd,
					unsigned long timeout);

#ifdef _WINDOWS

#define complete(x) KeSetEvent(x, IO_NO_INCREMENT, FALSE)
#define init_completion(x) KeInitializeEvent(x, SynchronizationEvent, FALSE)

static inline int list_empty(const struct list_head *head)
{
	return head->Blink == head;
}

#define list_entry(ptr, type, member) CONTAINING_RECORD(ptr, type, member)

#define list_for_each(pos, head)                                               \
	for (pos = (head)->Blink; pos != (head); pos = pos->Blink)

#define list_first_entry(ptr, type, member)                                    \
	list_entry((ptr)->Blink, type, member)
#define list_next_entry(pos, type, member)                                     \
	list_entry((pos)->member.Blink, type, member)
#define list_for_each_entry(pos, type, head, member)                           \
	for (pos = list_first_entry(head, type, member);                       \
	     &pos->member != (head); pos = list_next_entry(pos, type, member))
#define list_for_each_entry_safe(pos, type, tmp, head, member)                 \
	for (pos = list_first_entry(head, type, member),                       \
	    tmp = list_next_entry(pos, type, member);                          \
	     &pos->member != (head);                                           \
	     pos = tmp, tmp = list_next_entry(tmp, type, member))

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	InitializeListHead((PLIST_ENTRY)list);
}

static inline void list_del(struct list_head *entry)
{
	RemoveEntryList((PLIST_ENTRY)entry);
}

static inline void list_del_init(struct list_head *entry)
{
	list_del(entry);
	INIT_LIST_HEAD(entry);
}

static inline void list_add_tail(struct list_head *entry,
				 struct list_head *head)
{
	InsertTailList((PLIST_ENTRY)head, (PLIST_ENTRY)entry);
}

static inline struct list_head *list_remove_head(struct list_head *head)
{
	return (struct list_head *)RemoveHeadList((PLIST_ENTRY)head);
}

inline int kstrtou16(const char *s, unsigned int base, unsigned short *res)
{
	unsigned long tmp = 0;
	int ret = RtlCharToInteger(s, base, &tmp);

	if (ret != STATUS_SUCCESS)
		goto l_out;

	if (tmp != (unsigned long long)(unsigned short)tmp) {
		ret = -34;
		goto l_out;
	}

	*res = (unsigned short)tmp;
l_out:
	return ret;
}

inline int kstrtoint(const char *s, unsigned int base, int *res)
{
	unsigned long tmp = 0;
	int ret = RtlCharToInteger(s, base, &tmp);

	if (ret != STATUS_SUCCESS)
		goto l_out;

	if (tmp != (unsigned long long)(int)tmp) {
		ret = -34;
		goto l_out;
	}

	*res = (int)tmp;
l_out:
	return ret;
}

inline int kstrtouint(const char *s, unsigned int base, unsigned int *res)
{
	unsigned long tmp = 0;
	int ret = RtlCharToInteger(s, base, &tmp);

	if (ret != STATUS_SUCCESS)
		goto l_out;

	if (tmp != (unsigned long long)(unsigned int)tmp) {
		ret = -34;
		goto l_out;
	}

	*res = (unsigned int)tmp;
l_out:
	return ret;
}

inline int kstrtou64(const char *s, unsigned long long base,
		     unsigned long long *res)
{
	unsigned long tmp = 0;
	int ret = RtlCharToInteger(s, base, &tmp);

	if (ret != STATUS_SUCCESS)
		goto l_out;

	if (tmp != (unsigned long long)tmp) {
		ret = -34;
		goto l_out;
	}

	*res = (unsigned long)tmp;
l_out:
	return ret;
}

int ps3_dma_free(struct ps3_instance *instance, size_t length, void *buffer);

int ps3_dma_alloc(struct ps3_instance *instance, size_t length, void **buffer,
		  unsigned long long *phy_addr);

#endif

static inline void ps3_msleep(unsigned int ms)
{
#ifdef _WINDOWS
	StorPortStallExecution((unsigned long)ms * 1000);
#else
	msleep(ms);
#endif
}

static inline void ps3_mdelay(unsigned int ms)
{
#ifndef _WINDOWS
	unsigned int count = (ms / MAX_MDELAY);
	unsigned int remain = (ms % MAX_MDELAY);

	do {
		udelay(1000 * MAX_MDELAY);
		count--;
	} while (count);

	if (remain != 0)
		udelay(remain * 1000);
#else
	StorPortStallExecution((unsigned long)ms * 1000);
#endif
}
void *ps3_kcalloc(struct ps3_instance *instance, unsigned int blocks,
		  unsigned int block_size);
void ps3_kfree(struct ps3_instance *instance, void *buffer);
void *ps3_kzalloc(struct ps3_instance *instance, unsigned int size);

void ps3_vfree(struct ps3_instance *instance, void *buffer);
void *ps3_vzalloc(struct ps3_instance *instance, unsigned int size);

int ps3_scsi_device_get(struct ps3_instance *instance,
			struct scsi_device *sdev);
void ps3_scsi_device_put(struct ps3_instance *instance,
			 struct scsi_device *sdev);
struct scsi_device *ps3_scsi_device_lookup(struct ps3_instance *instance,
					   unsigned char channel,
					   unsigned short target_id,
					   unsigned char lun);
void ps3_scsi_remove_device(struct ps3_instance *instance,
			    struct scsi_device *sdev);
int ps3_scsi_add_device(struct ps3_instance *instance, unsigned char channel,
			unsigned short target_id, unsigned char lun);

unsigned long long ps3_now_ms_get(void);
#ifdef _WINDOWS
int ps3_now_format_get(char *buff, int buf_len);
#endif
unsigned long long ps3_1970_now_ms_get(void);
unsigned long long ps3_tick_count_get(void);

#endif
