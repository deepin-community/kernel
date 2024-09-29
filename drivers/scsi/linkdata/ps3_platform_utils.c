

#include "ps3_platform_utils.h"
#include "ps3_instance_manager.h"
#include "ps3_mgr_channel.h"
#include "ps3_driver_log.h"
#ifndef _WINDOWS
#include <linux/vmalloc.h>
#endif
#ifdef _WINDOWS
S32 ps3_dma_free(
    struct ps3_instance *instance,
    size_t   length,
    void *buffer
)
{
    S32 ret = PS3_SUCCESS;
    ULong status;

    if (buffer == NULL || length == 0) {
        ret = -PS3_FAILED;
        goto l_out;
    }

    status = StorPortFreeContiguousMemorySpecifyCache(instance,
        buffer,
        length,
        MmCached);

    if (status != STOR_STATUS_SUCCESS) {
        ret = -PS3_FAILED;
    }
l_out:
    return ret;
}

S32 ps3_dma_alloc(
    struct ps3_instance *instance,
    size_t length,
    void **buffer,
    U64 *phy_addr
)
{
    S32 ret = PS3_SUCCESS;
    ULong           len;
    ULong            status;
    PHYSICAL_ADDRESS minPhysicalAddress;
    PHYSICAL_ADDRESS maxPhysicalAddress;
    PHYSICAL_ADDRESS boundaryPhysicalAddress;
    STOR_PHYSICAL_ADDRESS PhysicalAddress;

    minPhysicalAddress.QuadPart = 0;
    maxPhysicalAddress.QuadPart = 0xFFFFFFFFFFFF;   
    boundaryPhysicalAddress.QuadPart = 0;

    status = StorPortAllocateContiguousMemorySpecifyCacheNode(instance,
        length,
        minPhysicalAddress,
        maxPhysicalAddress,
        boundaryPhysicalAddress,
        MmCached,
        MM_ANY_NODE_OK,
        buffer);

    if (status != STOR_STATUS_SUCCESS) {
        LOG_ERROR("alloc dma buffer failed, length:%d, status:0x%x\n", length, status);
        ret = -PS3_FAILED;
        goto l_out;
    }

    PhysicalAddress = StorPortGetPhysicalAddress(instance, NULL, *buffer, &len);
    *phy_addr = (U64)PhysicalAddress.QuadPart;
    if (PhysicalAddress.QuadPart == 0) {
        LOG_ERROR("dma buffer remap fail\n");
        ps3_dma_free(instance, length, *buffer);
        *buffer = NULL;
        ret = -PS3_FAILED;
    }

l_out:
    return ret;
}

#endif

void *ps3_kcalloc(struct ps3_instance *instance, U32 blocks, U32 block_size)
{
    void *ret = NULL;
#ifndef _WINDOWS
	(void)instance;
    ret = kcalloc(blocks, block_size, GFP_KERNEL);
#else
    ULong status = StorPortAllocatePool(instance,
        blocks * block_size, 'd3sp', &ret);

	if (status != STOR_STATUS_SUCCESS) {
		LOG_ERROR("host_no:%d, memory alloc failed, status0x%x\n", PS3_HOST(instance), status);
		ret = NULL;
	} else {
		memset(ret, 0, blocks * block_size);
	}

#endif

	if (ret == NULL) {
		LOG_ERROR("host_no:%u, memory:%u %u alloc failed\n", PS3_HOST(instance), blocks, block_size);
	}

    return ret;
}

void ps3_kfree(struct ps3_instance *instance, void *buffer)
{
#ifndef _WINDOWS
	(void)instance;
	if(buffer != NULL){
	    kfree(buffer);
	}
#else
    ULong status = StorPortFreePool(instance, buffer);
    if (status != STOR_STATUS_SUCCESS) {
        LOG_ERROR("host_no:%u, memory free failed, status0x%x\n", PS3_HOST(instance), status);
    }

#endif

    return;
}

void *ps3_kzalloc(struct ps3_instance *instance, U32 size) {
	return ps3_kcalloc(instance, 1, size);
}

void ps3_vfree(struct ps3_instance *instance, void *buffer)
{
#ifndef _WINDOWS
	(void)instance;
	vfree(buffer);
#else
	ULong status = StorPortFreePool(instance, buffer);
	if (status != STOR_STATUS_SUCCESS) {
		LOG_ERROR("host_no:%u, memory free failed, status0x%x\n", PS3_HOST(instance), status);
	}

#endif

	return;
}

void *ps3_vzalloc(struct ps3_instance *instance, U32 size)
{
	void *ret = NULL;
#ifndef _WINDOWS
	(void)instance;
	ret = vzalloc(size);
#else
	ULong status = StorPortAllocatePool(instance,
		size, 'd3sp', &ret);

	if (status != STOR_STATUS_SUCCESS) {
		LOG_ERROR("host_no:%d, memory alloc failed, status0x%x\n", PS3_HOST(instance), status);
		ret = NULL;
	} else {
		memset(ret, 0, size);
	}

#endif

	if (ret == NULL) {
		LOG_ERROR("host_no:%u, memory:%u alloc failed\n", PS3_HOST(instance), size);
	}

	return ret;
}

S32 ps3_wait_for_completion_timeout(void *sync_done, ULong time_out)
{
	S32 ret = PS3_SUCCESS;
#ifdef _WINDOWS
	NTSTATUS wait_ret = STATUS_SUCCESS;
	LARGE_INTEGER win_timeout = { 0 };
	if (time_out > 0) {
		win_timeout.QuadPart = (S64)(time_out * (-10000000LL));
		wait_ret = KeWaitForSingleObject(sync_done, Executive,
			KernelMode, FALSE, &win_timeout);
	}
	else {
		wait_ret = KeWaitForSingleObject(sync_done, Executive,
			KernelMode, FALSE, NULL);
	}

	if (wait_ret == STATUS_TIMEOUT) {
		ret = -PS3_TIMEOUT;
	}
#else
	U16 timeout = 0;
	if (time_out > 0) {
		timeout = wait_for_completion_timeout((struct completion *)sync_done, time_out * HZ);
		if (timeout == 0) {
			ret = -PS3_TIMEOUT;
		}
	} else {
		wait_for_completion((struct completion *)sync_done);
	}
#endif
	return ret;
}

S32 ps3_wait_cmd_for_completion_timeout(struct ps3_instance *instance, struct ps3_cmd *cmd, ULong timeout) {
	S32 ret = PS3_SUCCESS;
	ULong time_out;
#ifdef _WINDOWS
	(void)instance;
	time_out = max((ULong)cmd->time_out, timeout);
	ret = ps3_wait_for_completion_timeout(&cmd->sync_done, time_out);
#else
	if (cmd->time_out == 0 && cmd->is_interrupt) {
		ps3_wait_cmd_for_completion_interrupt(instance, cmd);
	} else {
		time_out = max((ULong)cmd->time_out, timeout);
		ret = ps3_wait_for_completion_timeout(&cmd->sync_done, time_out);
	}
#endif
    return ret;
}

S32 ps3_scsi_device_get(struct ps3_instance *instance, struct scsi_device *sdev)
{
#ifdef _WINDOWS
    return ps3_scsi_device_get_win(instance, sdev);
#else
	(void)instance;
    return scsi_device_get(sdev);
#endif
}

void ps3_scsi_device_put(struct ps3_instance *instance, struct scsi_device *sdev)
{
#ifdef _WINDOWS
    ps3_scsi_device_put_win(instance, sdev);
#else
	(void)instance;
    scsi_device_put(sdev);
#endif
}

#ifndef _WINDOWS

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
struct scsi_device *__ps3_scsi_device_lookup_check(struct Scsi_Host *shost,
	U32 channel, U32 id, U32 lun)
{
	struct scsi_device *sdev;

	list_for_each_entry(sdev, &shost->__devices, siblings) {
		if (sdev->sdev_state == SDEV_DEL)
			continue;
		if (sdev->channel == channel && sdev->id == id &&
				sdev->lun == lun)
			return sdev;
	}

	return NULL;
}

struct scsi_device *ps3_scsi_device_lookup_check(struct Scsi_Host *shost,
	U32 channel, U32 id, U32 lun)
{
	struct scsi_device *sdev;
	unsigned long flags;

	spin_lock_irqsave(shost->host_lock, flags);
	sdev = __ps3_scsi_device_lookup_check(shost, channel, id, lun);
	if (sdev && scsi_device_get(sdev))
		sdev = NULL;
	spin_unlock_irqrestore(shost->host_lock, flags);

	return sdev;
}

#endif

#endif

struct scsi_device *ps3_scsi_device_lookup(struct ps3_instance *instance, U8 channel, U16 target_id, U8 lun)
{
#ifdef _WINDOWS
    (void)lun;
    struct scsi_device *sdev = ps3_scsi_device_lookup_win(instance, channel, (U8)target_id);
    if (sdev != NULL && sdev->unit_start == 1) {
        return sdev;
    }
    return NULL;
#else
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
	return ps3_scsi_device_lookup_check(instance->host, channel, target_id, lun);
#else
	return scsi_device_lookup(instance->host, channel, target_id, lun);
#endif
#endif
}

void ps3_scsi_remove_device(struct ps3_instance *instance, struct scsi_device *sdev)
{
#ifdef _WINDOWS
    ps3_scsi_remove_device_win(instance, sdev);
#else
	(void)instance;
    scsi_remove_device(sdev);
#endif
}

S32 ps3_scsi_add_device(struct ps3_instance *instance, U8 channel, U16 target_id, U8 lun)
{
#ifdef _WINDOWS
    (void)lun;
    return ps3_scsi_add_device_win(instance, channel, (U8)target_id);
#else
    return scsi_add_device(instance->host, channel, target_id, lun);
#endif
}

U64 ps3_now_ms_get(void)
{
#ifdef _WINDOWS
    LARGE_INTEGER timestamp;
    KeQuerySystemTime(&timestamp);
    return timestamp.QuadPart / 10000; 
#else
	return ktime_to_ms(ktime_get_real());
#endif
}

U64 ps3_1970_now_ms_get(void)
{
#ifdef _WINDOWS
    U64 timestamp;
    LARGE_INTEGER timestamp1970;
    TIME_FIELDS  timefiled;

    timefiled.Year = 1970;
    timefiled.Month = 1;
    timefiled.Day = 1;
    timefiled.Hour = 0;
    timefiled.Minute = 0;
    timefiled.Second = 0;
    timefiled.Milliseconds = 0;

    RtlTimeFieldsToTime(&timefiled, &timestamp1970);
    timestamp = ps3_now_ms_get();
    return timestamp - (timestamp1970.QuadPart / 10000); 
#else
    return ps3_now_ms_get();
#endif
}
#ifdef _WINDOWS
S32 ps3_now_format_get(char *buff, S32 buf_len)
{
#ifdef _WINDOWS
    LARGE_INTEGER timestamp;
    KeQuerySystemTime(&timestamp);
    LARGE_INTEGER localtime;
    TIME_FIELDS  timefiled;

    ExSystemTimeToLocalTime(&timestamp, &localtime);
    RtlTimeToTimeFields(&localtime, &timefiled);

    return snprintf(buff, buf_len, "%04ld-%02d-%02d_%02d:%02d:%02d.%03d",
        timefiled.Year, timefiled.Month, timefiled.Day,
        timefiled.Hour, timefiled.Minute, timefiled.Second,
        timefiled.Milliseconds);
#else
    struct timeval	tv;
    struct tm td;

    do_gettimeofday(&tv);
    time_to_tm(tv.tv_sec, -sys_tz.tz_minuteswest * 60, &td);

    return snprintf(buff, buf_len, "%04ld-%02d-%02d_%02d:%02d:%02d",
        td.tm_year + 1900, td.tm_mon + 1, td.tm_mday,
        td.tm_hour, td.tm_min, td.tm_sec);
#endif
}
#endif
U64 ps3_tick_count_get(void)
{
#ifdef _WINDOWS
    LARGE_INTEGER tick_count;
    LARGE_INTEGER tick_frequency;

    tick_count = KeQueryPerformanceCounter(&tick_frequency);
    return (U64)(tick_count.QuadPart * 1000000 / tick_frequency.QuadPart);
#else
    return (U64)ktime_to_us(ktime_get_real());
#endif
}
