
#ifndef _PS3_PLATFORM_UTILS_H_
#define _PS3_PLATFORM_UTILS_H_

#ifdef _WINDOWS
#include "ps3_def.h"
#else
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#endif

#include "ps3_err_def.h"

struct ps3_instance;
struct scsi_device;
struct ps3_cmd;

#define scsi_cmnd_cdb(scmnd) ((scmnd)->cmnd)
#define scsi_device_private_data(scmnd) (PS3_SDEV_PRI_DATA((scmnd)->device))
#ifdef _WINDOWS
#define scsi_host_data(scmnd) (struct ps3_instance*)((scmnd)->instance)
#else
#define scsi_host_data(scmnd) (struct ps3_instance*)((scmnd)->device->host->hostdata)
#endif

#ifdef _WINDOWS
#define ps3_container_of(ptr, type, member) \
	((type*)((char*)ptr - offsetof(type,member)))
#else
#define ps3_container_of container_of
#endif
#define MAX_MDELAY (1)

#ifdef _WINDOWS
typedef struct {
    FAST_MUTEX mutex;
}ps3_mutex, *pps3_mutex;
#else

typedef struct mutex ps3_mutex;

#endif

static inline void ps3_mutex_init(ps3_mutex *mutex_lock)
{
#ifdef _WINDOWS
    ExInitializeFastMutex(&mutex_lock->mutex);
#else
	mutex_init(mutex_lock);
#endif
}

static inline void ps3_mutex_destroy(ps3_mutex *mutex_lock)
{
#ifdef _WINDOWS
    (void)mutex_lock;
#else
	mutex_destroy(mutex_lock);
#endif
    return;
}

static inline S32 ps3_mutex_lock(ps3_mutex *mtx_lock)
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
static inline S32 ps3_mutex_trylock(ps3_mutex *mutex_lock)
{
    S32 ret = PS3_SUCCESS;
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

static inline S32 ps3_mutex_unlock(ps3_mutex *mutex_lock)
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

#ifdef _WINDOWS
typedef struct {
    volatile S32 value;
}ps3_atomic32, *pps3_atomic32;

typedef struct {
    volatile S64 value;
}ps3_atomic64, *pps3_atomic64;
#else
typedef atomic_t ps3_atomic32;
typedef atomic64_t ps3_atomic64;
#endif

static inline S32 ps3_atomic_read(ps3_atomic32 *value) {
#ifdef _WINDOWS
    return value->value;
#else
	return atomic_read(value);
#endif
}

static inline S32 ps3_atomic_dec(ps3_atomic32 *value) {
#ifdef _WINDOWS
    return (S32)InterlockedDecrement((LONG*)(&value->value));
#else
	atomic_dec(value);
	return PS3_SUCCESS;
#endif
}

static inline S32 ps3_atomic_add(S32 i, ps3_atomic32 *value) {
#ifdef _WINDOWS
	return (S32)_InlineInterlockedAdd((LONG*)&value->value, (LONG)i);
#else
	atomic_add(i, value);
	return PS3_SUCCESS;
#endif
}

static inline S32 ps3_atomic_sub(S32 i, ps3_atomic32 *value) {
#ifdef _WINDOWS
	return (S32)_InlineInterlockedAdd((LONG*)&value->value, (LONG)-i);
#else
	atomic_sub(i, value);
	return PS3_SUCCESS;
#endif
}

static inline S32 ps3_atomic_cmpxchg(ps3_atomic32 *value, S32 old, S32 cur)
{
#ifdef _WINDOWS
    return (S32)InterlockedCompareExchange((LONG*)&value->value, (LONG)cur, (LONG)old);
#else
	return atomic_cmpxchg(value, cur, old);
#endif
}

static inline Bool ps3_atomic_add_unless(ps3_atomic32 *value, S32 a, S32 u) {
#ifdef _WINDOWS

    S32 c = 0;
    S32 old = 0;
    c = value->value;
    while (c != u &&
        (old = ps3_atomic_cmpxchg(value, c, c + a)) != c) {
        c = old;
    }

    return c != u;
#else
	return atomic_add_unless(value, a, u);
#endif
}

static inline S32 ps3_atomic_inc(ps3_atomic32 *value) {
#ifdef _WINDOWS
    return (S32)InterlockedIncrement((LONG*)(&value->value));
#else
	atomic_inc(value);
	return PS3_SUCCESS;
#endif
}

static inline S32 ps3_atomic_inc_return(ps3_atomic32 *value) {
#ifdef _WINDOWS
    return (S32)InterlockedIncrement((LONG*)(&value->value));
#else
	return atomic_inc_return(value);
#endif
}

static inline S32 ps3_atomic_dec_return(ps3_atomic32 *value) {
#ifdef _WINDOWS
    return (S32)InterlockedDecrement((LONG*)(&value->value));
#else
	return atomic_dec_return(value);
#endif
}

static inline S64 ps3_atomic64_inc(ps3_atomic64 *value) {
#ifdef _WINDOWS
    return (S64)InterlockedIncrement64((LONG64*)(&value->value));
#else
	atomic64_inc(value);
	return PS3_SUCCESS;
#endif
}

static inline S64 ps3_atomic64_inc_return(ps3_atomic64 *value) {
#ifdef _WINDOWS
    return (S64)InterlockedIncrement64((LONG64*)(&value->value));
#else
	return atomic64_inc_return(value);
#endif
}

static inline S64 ps3_atomic64_read(ps3_atomic64 *value) {
#ifdef _WINDOWS
	return value->value;
#else
	return atomic64_read(value);
#endif
}

static inline void ps3_atomic64_set(ps3_atomic64 *value, S64 i) {
#ifdef _WINDOWS
	value->value = i;
#else
	atomic64_set(value, i);
#endif
}

static inline void ps3_atomic_set(ps3_atomic32 *value, S32 i) {
#ifdef _WINDOWS
	value->value = i;
#else
	atomic_set(value, i);
#endif
}

static inline S64 ps3_atomic64_add(S64 i, ps3_atomic64 *value) {
#ifdef _WINDOWS
	return (S64)_InlineInterlockedAdd64((LONG64*)&value->value, (LONG64)i);
#else
	atomic64_add(i, value);
	return PS3_SUCCESS;
#endif
}

static inline S64 ps3_atomic64_dec(ps3_atomic64 *value) {
#ifdef _WINDOWS
	return (S64)InterlockedDecrement64((LONG64*)&value->value);
#else
	atomic64_dec(value);
	return PS3_SUCCESS;
#endif
}

#ifdef _WINDOWS
typedef struct {
    KSPIN_LOCK lock;
}ps3_spinlock, *pps3_spinlock;
#else
typedef spinlock_t ps3_spinlock;
#endif

static inline void ps3_spin_lock_init(ps3_spinlock *lock)
{
#ifdef _WINDOWS
    KeInitializeSpinLock(&lock->lock);
#else
	spin_lock_init(lock);
#endif
}

static inline void ps3_spin_lock(ps3_spinlock *lock, ULong*flag)
{
#ifdef _WINDOWS
    KeAcquireSpinLock(&lock->lock, (PKIRQL)flag);
#else
	(void)flag;
    spin_lock(lock);
#endif
}

static inline void ps3_spin_lock_irqsave(ps3_spinlock *lock, ULong *flag)
{
#ifdef _WINDOWS
    KeAcquireSpinLock(&lock->lock, (PKIRQL)flag);
#else
    spin_lock_irqsave(lock, *flag);
#endif
}

static inline void ps3_spin_unlock(ps3_spinlock *lock, ULong flag)
{
#ifdef _WINDOWS
    KeReleaseSpinLock(&lock->lock, (KIRQL)flag);
#else
	(void)flag;
    spin_unlock(lock);
#endif
}

static inline void ps3_spin_unlock_irqrestore(ps3_spinlock *lock, ULong flag)
{
#ifdef _WINDOWS
    KeReleaseSpinLock(&lock->lock, (KIRQL)flag);
#else
    spin_unlock_irqrestore(lock, flag);
#endif
}

S32 ps3_wait_for_completion_timeout(void *sync_done, ULong Timeout);
S32 ps3_wait_cmd_for_completion_timeout(struct ps3_instance *instance,
	struct ps3_cmd *cmd, ULong timeout);

#ifdef _WINDOWS
typedef LIST_ENTRY ps3_list_head;
#else
typedef struct list_head ps3_list_head;
#endif

#ifdef _WINDOWS

#define complete(x) KeSetEvent(x, IO_NO_INCREMENT, FALSE);
#define init_completion(x) KeInitializeEvent(x, SynchronizationEvent, FALSE);

static inline int list_empty(const ps3_list_head* head)
{
    return head->Blink == head;
}

#define list_entry(ptr, type, member) \
	CONTAINING_RECORD(ptr, type, member)

#define list_for_each(pos, head) \
	for (pos = (head)->Blink; pos != (head); pos = pos->Blink)

#define list_first_entry(ptr, type, member) \
		list_entry((ptr)->Blink, type, member)

#define list_next_entry(pos, type, member) \
		list_entry((pos)->member.Blink, type, member)

#define list_for_each_entry(pos, type, head, member)				\
	for (pos = list_first_entry(head, type, member);	\
	     &pos->member != (head);					\
	     pos = list_next_entry(pos, type, member))

#define list_for_each_entry_safe(pos, type, tmp, head, member)			\
	for (pos = list_first_entry(head, type, member),		\
		tmp = list_next_entry(pos, type, member);	\
		&pos->member != (head);						\
		pos = tmp, tmp = list_next_entry(tmp, type, member))

static inline void INIT_LIST_HEAD(ps3_list_head* list)
{
    InitializeListHead((PLIST_ENTRY)list);
}

static inline void list_del(ps3_list_head* entry)
{
    RemoveEntryList((PLIST_ENTRY)entry);
}

static inline void list_del_init(ps3_list_head* entry)
{
    list_del(entry);
    INIT_LIST_HEAD(entry);
}

static inline void list_add_tail(ps3_list_head* entry, ps3_list_head* head)
{
    InsertTailList((PLIST_ENTRY)head, (PLIST_ENTRY)entry);
}

static inline ps3_list_head* list_remove_head(ps3_list_head* head)
{
    return (ps3_list_head*)RemoveHeadList((PLIST_ENTRY)head);
}

inline S32 kstrtou16(const S8 *s, U32 base, U16 *res)
{
    ULong tmp = 0;
    int ret = RtlCharToInteger(s, base, &tmp);
    if (ret != STATUS_SUCCESS) {
        goto l_out;
    }

    if (tmp != (U64)(U16)tmp) {
        ret = -34; 
        goto l_out;
    }

    *res = (U16)tmp;
l_out:
    return ret;
}

inline S32 kstrtoint(const S8 *s, U32 base, S32* res)
{
    ULong tmp = 0;
    int ret = RtlCharToInteger(s, base, &tmp);
    if (ret != STATUS_SUCCESS) {

    }

    if (tmp != (U64)(int)tmp) {
        ret = -34; 
        goto l_out;
    }

    *res = (int)tmp;
l_out:
    return ret;
}

inline S32 kstrtouint(const S8 *s, U32 base, U32* res)
{
    ULong tmp = 0;
    int ret = RtlCharToInteger(s, base, &tmp);
    if (ret != STATUS_SUCCESS) {
        goto l_out;
    }

    if (tmp != (U64)(U32)tmp) {
        ret = -34; 
        goto l_out;
    }

    *res = (unsigned int)tmp;
l_out:
    return ret;
}

inline S32 kstrtou64(const S8 *s, U64 base, U64* res)
{
    ULong tmp = 0;
    int ret = RtlCharToInteger(s, base, &tmp);
    if (ret != STATUS_SUCCESS) {
        goto l_out;
    }

    if (tmp != (U64)tmp) {
        ret = -34; 
        goto l_out;
    }

    *res = (ULong)tmp;
l_out:
    return ret;
}

S32 ps3_dma_free(
    struct ps3_instance *instance,
    size_t   length,
    void *buffer
);

S32 ps3_dma_alloc(
    struct ps3_instance *instance,
    size_t   length,
    void **buffer,
    U64 *phy_addr
);

#endif

static inline void ps3_msleep(U32 ms)
{
#ifdef _WINDOWS
    StorPortStallExecution((ULong)ms*1000);
#else
	msleep(ms);
#endif
}

static inline void ps3_mdelay(U32 ms)
{
#ifndef _WINDOWS
	U32 count = (ms/MAX_MDELAY);
	U32 remain = (ms%MAX_MDELAY);
	do {
		udelay(1000*MAX_MDELAY);
		count--;
	} while(count);

	if(remain != 0){
		udelay(remain * 1000);
	}
#else
    StorPortStallExecution((ULong)ms*1000);
#endif
}
void *ps3_kcalloc(struct ps3_instance *instance, U32 blocks, U32 block_size);
void ps3_kfree(struct ps3_instance *instance, void *buffer);
void *ps3_kzalloc(struct ps3_instance *instance, U32 size);

void ps3_vfree(struct ps3_instance *instance, void *buffer);
void *ps3_vzalloc(struct ps3_instance *instance, U32 size);

int ps3_scsi_device_get(struct ps3_instance *instance, struct scsi_device *sdev);
void ps3_scsi_device_put(struct ps3_instance *instance, struct scsi_device *sdev);
struct scsi_device *ps3_scsi_device_lookup(struct ps3_instance *instance, U8 channel, U16 target_id, U8 lun);
void ps3_scsi_remove_device(struct ps3_instance *instance, struct scsi_device *sdev);
S32 ps3_scsi_add_device(struct ps3_instance *instance, U8 channel, U16 target_id, U8 lun);

U64 ps3_now_ms_get(void);
#ifdef _WINDOWS
S32 ps3_now_format_get(char *buff, S32 buf_len);
#endif
U64 ps3_1970_now_ms_get(void);
U64 ps3_tick_count_get(void);

#endif
